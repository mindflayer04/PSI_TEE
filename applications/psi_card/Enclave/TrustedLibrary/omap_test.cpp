#include "../Enclave.h"
#include "Enclave_t.h"
////
#include <omp.h>

#include <functional>
#include <unordered_map>

#include "odsl/omap.hpp"
#include "odsl/omap_short_kv.hpp"
#include "odsl/page_oram.hpp"
#include "odsl/par_omap.hpp"
#include "odsl/recursive_oram.hpp"
#include "sgx_thread.h"
#include "sgx_trts.h"
#include "algorithm/kway_butterfly_sort.hpp"
#include "algorithm/kway_distri_sort.hpp"

#include "sgx_tcrypto.h"
#include "sgx_tseal.h"

#define MB << 20
#define RSA_2048_SIZE 256
#define RSA_2048_HALF 128

#define ASSERT_TRUE(expr)                           \
  if (!expr) {                                      \
    printf("assert failed at line %d\n", __LINE__); \
    abort();                                        \
  }

using namespace ODSL;
EM::Backend::MemServerBackend* EM::Backend::g_DefaultBackend = nullptr;

OMap<__uint128_t, int64_t> *g_globalMap = nullptr;

#define ASSERT_EQ(a, b)                             \
  if ((a) != (b)) {                                 \
    printf("assert failed at line %d\n", __LINE__); \
    printf("%lu != %lu\n", (a), (b));               \
    abort();                                        \
  }

struct rsa_priv_components_t {
    unsigned char n[RSA_2048_SIZE];    
    unsigned char d[RSA_2048_SIZE];    
    unsigned char p[RSA_2048_HALF];    
    unsigned char q[RSA_2048_HALF];    
    unsigned char dmp1[RSA_2048_HALF]; 
    unsigned char dmq1[RSA_2048_HALF]; 
    unsigned char iqmp[RSA_2048_HALF]; 
    unsigned char e[4]; 
};

uint32_t ecall_get_rsa_sealed_size() {
    // Calculates the exact sealed size for our custom rsa_priv_components_t struct
    return sgx_calc_sealed_data_size(0, sizeof(rsa_priv_components_t));
}

static sgx_status_t internal_decrypt_rsa_256bit(
    const uint8_t* p_sealed_buffer, 
    const uint8_t* ciphertext, 
    uint8_t* decrypted_value_32bytes) 
{
    sgx_status_t status;
    rsa_priv_components_t priv_key;
    void* rsa_key_ctx = NULL;
    uint32_t unsealed_size = sizeof(rsa_priv_components_t);

    status = sgx_unseal_data((const sgx_sealed_data_t*)p_sealed_buffer, 
                             NULL, 0, (uint8_t*)&priv_key, &unsealed_size);
    if (status != SGX_SUCCESS) {
        printf("[Enclave] Error 1: sgx_unseal_data failed with code %x\n", status);
        return status;
    }

    status = sgx_create_rsa_priv2_key(
        RSA_2048_SIZE, 
        4, priv_key.e, 
        priv_key.p, priv_key.q, priv_key.dmp1, priv_key.dmq1, priv_key.iqmp,
        &rsa_key_ctx
    );
    
    if (status != SGX_SUCCESS) {
        printf("[Enclave] Error 1: sgx_create_rsa_priv2_key failed with code %x\n", status);
        return status;
    }

    uint8_t temp_decrypt_buffer[RSA_2048_SIZE] = {0};
    size_t out_len = RSA_2048_SIZE; 
    
    status = sgx_rsa_priv_decrypt_sha256(
        rsa_key_ctx, 
        temp_decrypt_buffer, 
        &out_len,
        ciphertext, 
        RSA_2048_SIZE
    );
    
    if (status == SGX_SUCCESS) {
        // Successful decryption!
        memcpy(decrypted_value_32bytes, temp_decrypt_buffer, 32);
    } else {
        printf("[Enclave] Error 1: sgx_rsa_priv_decrypt_sha256 math failed! Code: %x\n", status);
    }
    
    // Secure cleanup
    memset_s(temp_decrypt_buffer, sizeof(temp_decrypt_buffer), 0, sizeof(temp_decrypt_buffer));
    sgx_free_rsa_key(rsa_key_ctx, SGX_RSA_PRIVATE_KEY, RSA_2048_SIZE, 4);
    memset_s(&priv_key, sizeof(priv_key), 0, sizeof(priv_key));
    
    return status;
}

sgx_status_t ecall_generate_rsa_key(uint8_t* p_sealed_buffer, uint32_t sealed_size, uint8_t* p_public_modulus) {
    sgx_status_t status;
    rsa_priv_components_t priv_key;
    
    unsigned char e_val[4] = {0x01, 0x00, 0x01, 0x00}; // Little-Endian 65537
    memcpy(priv_key.e, e_val, 4);

    status = sgx_create_rsa_key_pair(
        RSA_2048_SIZE, 
        4, 
        priv_key.n, 
        priv_key.d, 
        priv_key.e, 
        priv_key.p, 
        priv_key.q, 
        priv_key.dmp1, 
        priv_key.dmq1, 
        priv_key.iqmp
    );

    if (status != SGX_SUCCESS) return status;

    memcpy(p_public_modulus, priv_key.n, RSA_2048_SIZE);

    uint32_t calc_size = sgx_calc_sealed_data_size(0, sizeof(rsa_priv_components_t));
    if (sealed_size < calc_size) {
        memset_s(&priv_key, sizeof(priv_key), 0, sizeof(priv_key));
        return SGX_ERROR_INVALID_PARAMETER;
    }

    status = sgx_seal_data(0, NULL, sizeof(priv_key), (uint8_t*)&priv_key, 
                           sealed_size, (sgx_sealed_data_t*)p_sealed_buffer);

    memset_s(&priv_key, sizeof(priv_key), 0, sizeof(priv_key));
    return status;
}


sgx_status_t ecall_check_decryption(const uint8_t* p_sealed_buffer, uint32_t sealed_size, const uint8_t* ciphertext){
  uint8_t internal_256bit_val[32] = {0};
  sgx_status_t status = internal_decrypt_rsa_256bit(p_sealed_buffer, ciphertext, internal_256bit_val);
  if (status != SGX_SUCCESS) {
    return status;
  }

  printf("[Enclave] Decrypted value: ");
  for (int i = 0; i < 32; i++) {
    printf("%d ", internal_256bit_val[i]);
  }
  printf("\n");

  return status;
}

uint64_t convert_256bit_to_uint64(const uint8_t* val) {
    uint64_t result = 0;
    for (int i = 0; i < 8; i++) {
        result |= (((uint64_t)val[i]) << (i * 8));
    }
    return result;
}

__uint128_t convert_256bit_to_uint128(const uint8_t* val) {
    __uint128_t result = 0;
    for (int i = 0; i < 16; i++) {
        result |= (((__uint128_t)val[i]) << (i * 8));
    }
    return result;
}

sgx_status_t ecall_check_intersection(const uint8_t* p_sealed_buffer, uint32_t sealed_size, const uint8_t* ciphertext, uint8_t* found){
  uint8_t internal_256bit_val[32] = {0};
  sgx_status_t status = internal_decrypt_rsa_256bit(p_sealed_buffer, ciphertext, internal_256bit_val);
  if (status != SGX_SUCCESS) {
    return status;
  }

  __uint128_t key = convert_256bit_to_uint128(internal_256bit_val);

  int64_t value;
  bool found_intersection = g_globalMap->Find(key, value);

  if(found_intersection){
    *found = 1;
  }
  else *found = 0;
  
  // printf("[Enclave] Element %s in the intersection\n", found ? "is" : "is not");

  return status;
}


void createMap(const __uint128_t* input_set, size_t set_size){
  if(g_globalMap != nullptr){
    delete g_globalMap;
    g_globalMap = nullptr;
  }

  uint64_t start, end, start_insert;
  // int mx_size = (1<<15);

  std::vector<__uint128_t> v(set_size);
  for(int i=0;i<set_size;i++){
    v[i] = input_set[i];
  }

  EM::NonCachedVector::Vector<__uint128_t> myvec(v.begin(),v.end()); 

  ocall_measure_time(&start);
  EM::Algorithm::KWayButterflyOShuffle(myvec.begin(),myvec.end());
  ocall_measure_time(&end);
  uint64_t timediff = end - start;
  printf("[Enclave] Shuffle time %f s\n", (double)timediff * 1e-9);

  size_t mapCapacity = set_size * 2;
  // Limit in-enclave cache (bytes). Keep this modest for machines with 8GB RAM.
  // Example: 128MB cache (adjust down if you need more headroom)
  const uint64_t CACHE_BYTES = (uint64_t)128ULL << 20; // 128 MiB
  g_globalMap = new OMap<__uint128_t,int64_t>(mapCapacity, CACHE_BYTES);

  EM::NonCachedVector::Vector<__uint128_t>::Reader reader(myvec.begin(), myvec.end(), /*inAuth=*/1);

  ocall_measure_time(&start_insert);
  for(int i=0;i<set_size;i++){
    __uint128_t val = reader.read();
    g_globalMap->Insert(val,1);
  }
  ocall_measure_time(&end);
  timediff = end - start_insert;
  printf("[Enclave] Insert time %f s\n", (double)timediff * 1e-9);
  printf("[Enclave] Total Preprocessing for set size %d time %f s\n", set_size, (double)(end - start) * 1e-9);
}

sgx_status_t ecall_createMap(const __uint128_t* input_set, size_t set_size){
  if (EM::Backend::g_DefaultBackend) {
    delete EM::Backend::g_DefaultBackend;
  }
  // Estimate backend bytes needed (conservative): ~64 bytes per element
  const size_t PER_ELEMENT_ESTIMATE = 64; // tune this for tighter sizing
  size_t BackendSize = std::max((size_t)(set_size * PER_ELEMENT_ESTIMATE),
                                (size_t)(1 << 20)); // at least 1MB
  EM::Backend::g_DefaultBackend =
      new EM::Backend::MemServerBackend(BackendSize);
  try {
    createMap(input_set, set_size);
  } catch (std::exception& e) {
    printf("exception: %s\n", e.what());
  }

  return SGX_SUCCESS;
}
