/**
 * @file shard_rpc_tls_validation.h
 * @brief Strict local validation for shard RPC TLS PEM material.
 */

#pragma once

#include <limits>
#include <memory>
#include <string>

#include <openssl/pem.h>
#include <openssl/x509.h>

namespace themis::sharding::detail {

/**
 * @brief Check that a PEM buffer contains an X.509 certificate.
 * @param pem Certificate or CA bundle data.
 * @return True when the first certificate can be parsed.
 */
inline bool containsPemCertificate(const std::string& pem) {
    if (pem.empty() || pem.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        return false;
    }

    std::unique_ptr<BIO, decltype(&BIO_free)> bio(
        BIO_new_mem_buf(pem.data(), static_cast<int>(pem.size())), BIO_free);
    if (!bio) {
        return false;
    }

    std::unique_ptr<X509, decltype(&X509_free)> certificate(
        PEM_read_bio_X509(bio.get(), nullptr, nullptr, nullptr), X509_free);
    return certificate != nullptr;
}

/**
 * @brief Check that a PEM certificate and private key form a pair.
 * @param certificate_pem Certificate-chain data.
 * @param private_key_pem Private-key data.
 * @return True when both PEM values parse and the key matches the first certificate.
 */
inline bool containsMatchingPemKeyPair(const std::string& certificate_pem,
                                      const std::string& private_key_pem) {
    if (certificate_pem.empty() || private_key_pem.empty() ||
        certificate_pem.size() >
            static_cast<std::size_t>(std::numeric_limits<int>::max()) ||
        private_key_pem.size() >
            static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        return false;
    }

    std::unique_ptr<BIO, decltype(&BIO_free)> certificate_bio(
        BIO_new_mem_buf(certificate_pem.data(), static_cast<int>(certificate_pem.size())),
        BIO_free);
    std::unique_ptr<BIO, decltype(&BIO_free)> key_bio(
        BIO_new_mem_buf(private_key_pem.data(), static_cast<int>(private_key_pem.size())),
        BIO_free);
    if (!certificate_bio || !key_bio) {
        return false;
    }

    std::unique_ptr<X509, decltype(&X509_free)> certificate(
        PEM_read_bio_X509(certificate_bio.get(), nullptr, nullptr, nullptr), X509_free);
    std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)> private_key(
        PEM_read_bio_PrivateKey(key_bio.get(), nullptr, nullptr, nullptr), EVP_PKEY_free);
    return certificate && private_key &&
           X509_check_private_key(certificate.get(), private_key.get()) == 1;
}

} // namespace themis::sharding::detail
