/**
 * @file checksum_utils.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "utils/checksum_utils.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <openssl/evp.h>

namespace themis {
namespace utils {

/**
 * @brief Internal helper: compute a hash using the given EVP message-digest algorithm.
 * @param[in] file_path Input parameter.
 * @param[in] md Input parameter.
 * @return Return value.
 * @details Returns a lowercase hex string, or "" on I/O or OpenSSL error. Calls: file(), is_open(), EVP_MD_CTX_new(), EVP_DigestInit_ex(), EVP_MD_CTX_free(), buffer(), read(), data().
 */
static std::string computeFileHash(const std::string& file_path, const EVP_MD* md) {
    /**
     * @brief File.
     * @param[in] file_path Input parameter.
     * @param[in] binary Input parameter.
     * @return Return value.
     */
    std::ifstream file(file_path, std::ios::binary);
    if (!file.is_open()) {
        return "";
    }

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) {
      return "";
    }

    if (EVP_DigestInit_ex(ctx, md, nullptr) != 1) {
        EVP_MD_CTX_free(ctx);
        return "";
    }

    constexpr size_t buffer_size = 32768;
    /**
     * @brief Buffer.
     * @param[in] buffer_size Input parameter.
     * @return Return value.
     */
    std::vector<char> buffer(buffer_size);

    while (file.read(buffer.data(), buffer_size) || file.gcount() > 0) {
        if (EVP_DigestUpdate(ctx, buffer.data(), static_cast<size_t>(file.gcount())) != 1) {
            EVP_MD_CTX_free(ctx);
            return "";
        }
    }

    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len = 0;
    if (EVP_DigestFinal_ex(ctx, hash, &hash_len) != 1) {
        EVP_MD_CTX_free(ctx);
        return "";
    }
    EVP_MD_CTX_free(ctx);

    std::ostringstream oss = {};
    for (unsigned int i = 0; i < hash_len; i++) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<unsigned int>(hash[i]);
    }
    return oss.str();
}

/**
 * @brief Calculate SHA256.
 * @param[in] file_path Input parameter.
 * @return Return value.
 * @details Calls: computeFileHash(), EVP_sha256().
 */
std::string calculateSHA256(const std::string& file_path) {
    return computeFileHash(file_path, EVP_sha256());
}

/**
 * @brief calculateMD5 remains available only for legacy compatibility.
 * @param[in] file_path Input parameter.
 * @return Return value.
 * @details New code must use SHA-256 via calculateSHA256(). Calls: computeFileHash(), EVP_md5().
 */
std::string calculateMD5(const std::string& file_path) {
    return computeFileHash(file_path, EVP_md5());
}

} // namespace utils
} // namespace themis
