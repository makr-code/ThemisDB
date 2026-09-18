/**
 * @file crypto_capabilities.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include <iostream>
#include <sstream>
#include <chrono>
#include <openssl/evp.h>
#include <openssl/engine.h>

#ifdef _WIN32
#include <intrin.h>
#else
#include <cpuid.h>
#endif

namespace themis {

/**
 * @brief Has AESNI.
 * @return True when the operation succeeds.
 * @details Calls: __cpuid().
 */
bool hasAESNI() {
    unsigned int cpuInfo[4];
    
#ifdef _WIN32
    __cpuid((int*)cpuInfo, 1);
#else
    __cpuid(1, cpuInfo[0], cpuInfo[1], cpuInfo[2], cpuInfo[3]);
#endif
    
    // Check ECX bit 25 (AES-NI)
    return (cpuInfo[2] & (1 << 25)) != 0;
}

/**
 * @brief Get Encryption Capabilities.
 * @return Return value.
 * @details Calls: OpenSSL_version(), hasAESNI(), ENGINE_get_default_cipher(), ENGINE_get_name(), str().
 */
std::string getEncryptionCapabilities() {
    std::ostringstream oss = {};
    
    oss << "OpenSSL Version: " << OpenSSL_version(OPENSSL_VERSION) << "\n";
    
    // Check AES-NI support
    if (hasAESNI()) {
        oss << "AES-NI: Available (Hardware Acceleration Enabled)\n";
    } else {
        oss << "AES-NI: Not Available (Software Fallback)\n";
    }
    
    // Check engine
    ENGINE* engine = ENGINE_get_default_cipher();
    if (engine) {
        oss << "Active Engine: " << ENGINE_get_name(engine) << "\n";
    } else {
        oss << "Active Engine: Default (Software)\n";
    }
    
    return oss.str();
}

/**
 * @brief Benchmark Encryption.
 * @return Return value.
 * @details Calls: EVP_CIPHER_CTX_new(), clock::now(), std::chrono::seconds(), EVP_EncryptInit_ex(), EVP_aes_256_gcm(), EVP_EncryptUpdate(), EVP_EncryptFinal_ex(), EVP_CIPHER_CTX_ctrl().
 */
double benchmarkEncryption() {
    // 256-bit test key and 96-bit IV — fixed values, not used for real data.
    const unsigned char key[32] = {
        0x60, 0x3d, 0xeb, 0x10, 0x15, 0xca, 0x71, 0xbe,
        0x2b, 0x73, 0xae, 0xf0, 0x85, 0x7d, 0x77, 0x81,
        0x1f, 0x35, 0x2c, 0x07, 0x3b, 0x61, 0x08, 0xd7,
        0x2d, 0x98, 0x10, 0xa3, 0x09, 0x14, 0xdf, 0xf4
    };
    const unsigned char iv[12] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
        0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b
    };

    constexpr int payload_size = 1024;
    unsigned char plaintext[payload_size] = {};
    unsigned char ciphertext[payload_size + 16] = {};
    unsigned char tag[16] = {};

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
      return 0.0;
    }

    using clock = std::chrono::steady_clock;
    const auto deadline = clock::now() + std::chrono::seconds(1);
    long long ops = 0;

    while (clock::now() < deadline) {
        int len = 0;
        if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, key, iv) != 1) {
          break;
        }
        if (EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, payload_size) != 1) {
          break;
        }
        int final_len = 0;
        if (EVP_EncryptFinal_ex(ctx, ciphertext + len, &final_len) != 1) {
          break;
        }
        EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag);
        ++ops;
    }

    EVP_CIPHER_CTX_free(ctx);
    return static_cast<double>(ops);
}

}  // namespace themis
