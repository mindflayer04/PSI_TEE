#include <iostream>
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

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

#define SERVER_PORT 8080
#define SERVER_IP "127.0.0.1"


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

std::vector<uint8_t> load_key(const std::string& filename) {
    FILE* pem_file = fopen(filename.c_str(), "r");
    if (!pem_file) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return {};
    }

    EVP_PKEY* pkey = PEM_read_PUBKEY(pem_file, NULL, NULL, NULL);
    fclose(pem_file);

    if (!pkey) {
        std::cerr << "Failed to parse public key from PEM file." << std::endl;
        ERR_print_errors_fp(stderr);
        return {};
    }

    BIGNUM* n = NULL;
    if (!EVP_PKEY_get_bn_param(pkey, OSSL_PKEY_PARAM_RSA_N, &n)) {
        std::cerr << "Failed to extract RSA modulus from public key." << std::endl;
        EVP_PKEY_free(pkey);
        return {};
    }

    std::vector<uint8_t> public_modulus(256, 0);
    
    // BN_bn2binpad guarantees the output is strictly padded to 256 bytes (Big-Endian)
    if (BN_bn2binpad(n, public_modulus.data(), 256) <= 0) {
        std::cerr << "Failed to convert BIGNUM to binary array." << std::endl;
        BN_free(n);
        EVP_PKEY_free(pkey);
        return {};
    }

    // --- CRITICAL FIX: Convert OpenSSL Big-Endian back to Native Little-Endian ---
    std::reverse(public_modulus.begin(), public_modulus.end());

    BN_free(n);
    EVP_PKEY_free(pkey);

    return public_modulus;
}

int main(){
    auto public_modulus = load_key("public_key.pem");
    if (public_modulus.empty()) {
        std::cerr << "Failed to load public key." << std::endl;
        return 1;
    }

    std::vector<uint8_t> secret_data(32,0);
    secret_data[0] = 123;

    auto ciphertext = encrypt_256bit(public_modulus.data(), secret_data.data());


    int sock = 0;
    struct sockaddr_in serv_addr;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "Socket creation error" << std::endl;
        return 1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);

    // Convert IPv4 address from text to binary form
    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address / Address not supported" << std::endl;
        return 1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Connection Failed. Is the SGX Host running?" << std::endl;
        return 1;
    }

    // Send exactly 256 bytes
    send(sock, ciphertext.data(), ciphertext.size(), 0);
    std::cout << "Encrypted query sent to Enclave Host successfully." << std::endl;

    close(sock);
    
    return 0;
}