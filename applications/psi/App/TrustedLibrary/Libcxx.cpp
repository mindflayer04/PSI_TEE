#include <stdio.h>
#include <vector>
#include <fstream>
#include <string>
#include <thread>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/bn.h>
#include <openssl/evp.h>
#include <openssl/core_names.h>
#include <openssl/err.h>
#include <algorithm>

//random headers

#include <unordered_set>
#include <random>
#include <cstdint>


// Network headers
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

#include "../App.h"
#include "Enclave_u.h"
#ifdef DISK_IO
#include "external_memory/server/enclaveFileServer_untrusted.hpp"
#else
#include "external_memory/server/enclaveMemServer_untrusted.hpp"
#endif

#define XXH_INLINE_ALL
#include "xxhash.h"

#define SERVER_PORT 8080


std::vector<uint64_t> generateDistinctRandom(uint64_t n) {
    std::unordered_set<uint64_t> unique_vals;
    
    // Random number generator
    std::random_device rd;              // seed
    std::mt19937_64 gen(rd());         // 64-bit Mersenne Twister
    std::uniform_int_distribution<uint64_t> dist;

    while (unique_vals.size() < n) {
        unique_vals.insert(dist(gen));
    }

    // Convert to vector
    return std::vector<uint64_t>(unique_vals.begin(), unique_vals.end());
}

std::vector<uint8_t> encrypt_256bit(const uint8_t* public_modulus, const uint8_t* secret_data_32bytes) {
    unsigned char e_val[4] = {0x01, 0x00, 0x01, 0x00};

    OSSL_PARAM params[3];
    params[0] = OSSL_PARAM_construct_BN(OSSL_PKEY_PARAM_RSA_N, (unsigned char*)public_modulus, 256);
    params[1] = OSSL_PARAM_construct_BN(OSSL_PKEY_PARAM_RSA_E, e_val, sizeof(e_val));
    params[2] = OSSL_PARAM_construct_end();

    EVP_PKEY_CTX* ctx_key = EVP_PKEY_CTX_new_from_name(NULL, "RSA", NULL);
    if (!ctx_key || EVP_PKEY_fromdata_init(ctx_key) <= 0) {
        std::cerr << "Failed to initialize key creation." << std::endl;
        if (ctx_key) EVP_PKEY_CTX_free(ctx_key);
        return {};
    }

    EVP_PKEY* pkey = NULL;
    if (EVP_PKEY_fromdata(ctx_key, &pkey, EVP_PKEY_PUBLIC_KEY, params) <= 0) {
        std::cerr << "Failed to build EVP_PKEY." << std::endl;
        ERR_print_errors_fp(stderr);
        EVP_PKEY_CTX_free(ctx_key);
        return {};
    }
    EVP_PKEY_CTX_free(ctx_key);

    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(pkey, NULL);
    if (!ctx || EVP_PKEY_encrypt_init(ctx) <= 0) {
        std::cerr << "Failed to initialize OpenSSL encryption context." << std::endl;
        EVP_PKEY_free(pkey);
        return {};
    }

    EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING);
    EVP_PKEY_CTX_set_rsa_oaep_md(ctx, EVP_sha256());
    EVP_PKEY_CTX_set_rsa_mgf1_md(ctx, EVP_sha256());

    size_t outlen = 0;
    if (EVP_PKEY_encrypt(ctx, NULL, &outlen, secret_data_32bytes, 32) <= 0) {
        std::cerr << "OpenSSL size calculation failed." << std::endl;
        ERR_print_errors_fp(stderr);
        EVP_PKEY_CTX_free(ctx); EVP_PKEY_free(pkey);
        return {};
    }
    
    std::vector<uint8_t> ciphertext(outlen); 
    if (EVP_PKEY_encrypt(ctx, ciphertext.data(), &outlen, secret_data_32bytes, 32) <= 0) {
        std::cerr << "OpenSSL RSA encryption failed." << std::endl;
        ERR_print_errors_fp(stderr);
    }

    EVP_PKEY_CTX_free(ctx);
    EVP_PKEY_free(pkey);

    return ciphertext;
}


bool save_to_file(const std::string& filename, const uint8_t* data, size_t size) {
    std::ofstream file(filename, std::ios::out | std::ios::binary);
    if (!file) return false;
    file.write(reinterpret_cast<const char*>(data), size);
    return true;
}

bool load_from_file(const std::string& filename, std::vector<uint8_t>& buffer) {
    std::ifstream file(filename, std::ios::in | std::ios::binary | std::ios::ate);
    if (!file) return false;
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);
    buffer.resize(size);
    file.read(reinterpret_cast<char*>(buffer.data()), size);
    return true;
}

bool save_rsa_public_key_to_pem(const std::string& filename, const uint8_t* public_modulus) {
    // Standard RSA public exponent (65537) represented as a big-endian byte array
    unsigned char e_val[] = {0x01, 0x00, 0x01, 0x00}; 

    // 1. Build the parameter array for OpenSSL 3.0
    OSSL_PARAM params[3];
    params[0] = OSSL_PARAM_construct_BN(OSSL_PKEY_PARAM_RSA_N, (unsigned char*)public_modulus, 256);
    params[1] = OSSL_PARAM_construct_BN(OSSL_PKEY_PARAM_RSA_E, e_val, sizeof(e_val));
    params[2] = OSSL_PARAM_construct_end();

    // 2. Create the Context for an RSA key
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_from_name(NULL, "RSA", NULL);
    if (!ctx) {
        std::cerr << "Failed to create EVP_PKEY_CTX." << std::endl;
        return false;
    }

    if (EVP_PKEY_fromdata_init(ctx) <= 0) {
        std::cerr << "Failed to initialize key creation." << std::endl;
        EVP_PKEY_CTX_free(ctx);
        return false;
    }

    // 3. Generate the key directly from the parameters
    EVP_PKEY* pkey = NULL;
    if (EVP_PKEY_fromdata(ctx, &pkey, EVP_PKEY_PUBLIC_KEY, params) <= 0) {
        std::cerr << "Failed to build EVP_PKEY from raw data." << std::endl;
        EVP_PKEY_CTX_free(ctx);
        return false;
    }
    
    EVP_PKEY_CTX_free(ctx);

    // 4. Write to the PEM file
    FILE* pem_file = fopen(filename.c_str(), "w");
    if (!pem_file) {
        std::cerr << "Failed to open file for writing: " << filename << std::endl;
        EVP_PKEY_free(pkey);
        return false;
    }

    int result = PEM_write_PUBKEY(pem_file, pkey);
    
    // 5. Cleanup
    fclose(pem_file);
    EVP_PKEY_free(pkey);

    if (result != 1) {
        std::cerr << "Failed to write PEM formatting." << std::endl;
        return false;
    }

    return true;
}

__uint128_t hash(const std::string& str) {
    XXH128_hash_t hash = XXH3_128bits(str.data(), str.size());
    return ((__uint128_t)hash.high64 << 64) | hash.low64;
}

void ActualMain(void) {
    sgx_status_t ret = SGX_SUCCESS;
    sgx_status_t status = SGX_SUCCESS;
    sgx_status_t ecall_status;

    std::vector<uint64_t> server_set = generateDistinctRandom(1<<16);
    // std::vector<uint64_t> server_set = {10,30,50,90};
    std::vector<__uint128_t> hashed_set;

    for(const auto& val : server_set){
        hashed_set.push_back(hash(std::to_string(val)));
    }

    uint32_t rsa_sealed_size = 0;
    status = ecall_get_rsa_sealed_size(global_eid, &rsa_sealed_size);
    if (status != SGX_SUCCESS || rsa_sealed_size == 0xFFFFFFFF) {
        std::cerr << "[Host] Failed to calculate sealed RSA size." << std::endl;
        return;
    }

    std::vector<uint8_t> sealed_rsa_buffer(rsa_sealed_size);
    std::vector<uint8_t> public_modulus(256);

    std::cout << "[Host] Requesting enclave to generate RSA-2048 keys..." << std::endl;
    status = ecall_generate_rsa_key(global_eid, &ecall_status, 
                                    sealed_rsa_buffer.data(), 
                                    rsa_sealed_size, 
                                    public_modulus.data());

    if (status == SGX_SUCCESS && ecall_status == SGX_SUCCESS) {
        std::cout << "[Host] Keys generated successfully!" << std::endl;

        // 5. Save the encrypted/sealed private key components to a binary file
        if (save_to_file("sealed_rsa_private_key.bin", sealed_rsa_buffer.data(), rsa_sealed_size)) {
            std::cout << "[Host] Saved sealed private key to 'sealed_rsa_private_key.bin'." << std::endl;
        }
        else {
            std::cerr << "[Host] Failed to save sealed private key." << std::endl;
        }

        // 6. Format the 256-byte modulus into a standard PEM file using OpenSSL
        if (save_rsa_public_key_to_pem("public_key.pem", public_modulus.data())) {
            std::cout << "[Host] Saved public key to 'public_key.pem'." << std::endl;
        }
        else {
            std::cerr << "[Host] Failed to save public key in PEM format." << std::endl;
        }
    } 

    else {
        std::cerr << "[Host] Enclave failed to generate RSA keys. Error: " << std::hex << status << std::endl;
    }

    sgx_status_t map_status;
    ret = ecall_createMap(global_eid,&map_status, hashed_set.data(), hashed_set.size());

    if(ret==SGX_SUCCESS && map_status==SGX_SUCCESS){
        std::cout << "[Host] Map created successfully in enclave." << std::endl;
    }
    else{
        std::cerr << "[Host] Failed to create map in enclave. Error: " << std::hex << ret << " " << std::hex << map_status << std::endl;
    }

    // Encryption-Decryption Sanity Check
    // std::vector<uint8_t> plaintext(32, 0);
    // plaintext[0] = 123; // This should match the secret data used in the client
    // plaintext[31] = 50;

    // auto ciphertext = encrypt_256bit(public_modulus.data(), plaintext.data());

    // sgx_status_t decryt_status;
    // status = ecall_check_decryption(
    //     global_eid, 
    //     &decryt_status, 
    //     sealed_rsa_buffer.data(), 
    //     rsa_sealed_size, 
    //     ciphertext.data()
    // );

    // if (status != SGX_SUCCESS || decryt_status != SGX_SUCCESS) {
    //     std::cerr << "[Host] Enclave failed to decrypt data." << std::endl;
    // } else {
    //     std::cout << "[Host] Decryption check completed successfully." << std::endl;
    // }

    // Network code
    int server_fd, client_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("[Host] Socket creation failed");
        return;
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("[Host] setsockopt failed");
        return;
    }
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(SERVER_PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("[Host] Bind failed");
        return;
    }
    
    if (listen(server_fd, 5) < 0) {
        perror("[Host] Listen failed");
        return;
    }

    std::cout << "\n[Host] Server is listening for clients on port " << SERVER_PORT << "..." << std::endl;


    int cnt = 0;
    while (cnt<1) {
        if ((client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("[Host] Accept failed");
            continue;
        }

        // send the public key to the client.
        std::cout << "[Host] Client connected. Sending public key..." << std::endl;
        int sent_bytes = send(client_socket, public_modulus.data(), 256, 0);
        if (sent_bytes != 256) {
            std::cerr << "[Host] Failed to send complete public key to client." << std::endl;
            close(client_socket);
            continue;
        }

        // 1. Read the network-formatted set size from the client
        uint32_t net_set_size = 0;
        int size_read = read(client_socket, &net_set_size, sizeof(net_set_size));

        if (size_read == sizeof(net_set_size)) {
            uint32_t set_size = ntohl(net_set_size); // Convert to native integer
            std::cout << "[Host] Client connected. Incoming set size: " << set_size << std::endl;

            for (uint32_t i = 0; i < set_size; i++) {
                std::vector<uint8_t> incoming_ciphertext(256, 0);
                
                int total_read = 0;
                while (total_read < 256) {
                    int bytes_read = read(client_socket, incoming_ciphertext.data() + total_read, 256 - total_read);
                    if (bytes_read <= 0) break; // Client disconnected or network error
                    total_read += bytes_read;
                }

                if (total_read == 256) {
                    sgx_status_t intersection_status;
                    uint8_t intersection_found = 0;
                    status = ecall_check_intersection(
                        global_eid, 
                        &intersection_status, 
                        sealed_rsa_buffer.data(), 
                        rsa_sealed_size, 
                        incoming_ciphertext.data(),
                        &intersection_found
                    );

                    if (status != SGX_SUCCESS || intersection_status != SGX_SUCCESS) {
                        std::cerr << "[Host] Enclave failed to process query " << (i + 1) << "." << std::endl;
                    } else {
                        // if(intersection_found){
                        //     std::cout << "[Host] Query " << (i + 1) << ": Element is in the intersection." << std::endl;
                        // }
                        // else{
                        //     std::cout << "[Host] Query " << (i + 1) << ": Element is NOT in the intersection." << std::endl;
                        // }
                    }
                } else {
                    std::cerr << "[Host] Incomplete or invalid data received for query " << (i + 1) << ". Aborting batch." << std::endl;
                    break;
                }
            }
        } else {
            std::cerr << "[Host] Failed to read valid set size from client." << std::endl;
        }

        close(client_socket);
        std::cout << "[Host] Client disconnected. Waiting for next connection...\n" << std::endl;
        cnt++;
    }
}
