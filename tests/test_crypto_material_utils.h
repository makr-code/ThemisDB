#pragma once

#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/bn.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>

#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>

namespace themis::tests {

struct PemMaterial {
    std::string cert_serial;
    std::string private_key_pem;
    std::string certificate_pem;
};

[[nodiscard]] inline std::string bioToString(BIO* bio) {
    BUF_MEM* mem = nullptr;
    BIO_get_mem_ptr(bio, &mem);
    return mem ? std::string(mem->data, mem->length) : std::string{};
}

[[nodiscard]] inline std::string makeDeterministicHexKey(
    std::size_t byte_count,
    std::size_t multiplier = 7,
    std::size_t offset = 1) {
    static constexpr char kHex[] = "0123456789abcdef";

    std::string key = {};
    key.reserve(byte_count * 2);
    for (std::size_t i = 0; i < byte_count * 2; ++i) {
        key.push_back(kHex[(i * multiplier + offset) % 16]);
    }
    return key;
}

[[nodiscard]] inline std::string generateRsaPrivateKeyPem() {
    EVP_PKEY_CTX* keygen_ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr);
    if (!keygen_ctx) {
        throw std::runtime_error("failed to allocate RSA keygen context");
    }

    EVP_PKEY* pkey = nullptr;
    BIO* key_bio = nullptr;
    try {
        if (EVP_PKEY_keygen_init(keygen_ctx) != 1 ||
            EVP_PKEY_CTX_set_rsa_keygen_bits(keygen_ctx, 2048) != 1 ||
            EVP_PKEY_keygen(keygen_ctx, &pkey) != 1) {
            throw std::runtime_error("failed to generate RSA keypair");
        }

        key_bio = BIO_new(BIO_s_mem());
        if (!key_bio ||
            PEM_write_bio_PrivateKey(key_bio, pkey, nullptr, nullptr, 0, nullptr, nullptr) != 1) {
            throw std::runtime_error("failed to serialize RSA private key");
        }

        auto pem = bioToString(key_bio);
        BIO_free(key_bio);
        EVP_PKEY_free(pkey);
        EVP_PKEY_CTX_free(keygen_ctx);
        return pem;
    } catch (...) {
        BIO_free(key_bio);
        EVP_PKEY_free(pkey);
        EVP_PKEY_CTX_free(keygen_ctx);
        throw;
    }
}

[[nodiscard]] inline PemMaterial generateSelfSignedPemMaterial(
    std::string_view serial_hex,
    std::string_view common_name) {
    PemMaterial material;
    material.cert_serial = std::string(serial_hex);

    EVP_PKEY_CTX* keygen_ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr);
    if (!keygen_ctx) {
        throw std::runtime_error("failed to allocate RSA keygen context");
    }

    EVP_PKEY* pkey = nullptr;
    if (EVP_PKEY_keygen_init(keygen_ctx) != 1 ||
        EVP_PKEY_CTX_set_rsa_keygen_bits(keygen_ctx, 2048) != 1 ||
        EVP_PKEY_keygen(keygen_ctx, &pkey) != 1) {
        EVP_PKEY_CTX_free(keygen_ctx);
        throw std::runtime_error("failed to generate RSA keypair");
    }
    EVP_PKEY_CTX_free(keygen_ctx);

    X509* cert = X509_new();
    if (!cert) {
        EVP_PKEY_free(pkey);
        throw std::runtime_error("failed to allocate X509 certificate");
    }

    BIGNUM* serial_bn = nullptr;
    ASN1_INTEGER* serial_asn1 = nullptr;
    BIO* key_bio = nullptr;
    BIO* cert_bio = nullptr;

    try {
        if (X509_set_version(cert, 2) != 1) {
            throw std::runtime_error("failed to set certificate version");
        }
        if (BN_hex2bn(&serial_bn, material.cert_serial.c_str()) == 0) {
            throw std::runtime_error("failed to parse test certificate serial");
        }
        serial_asn1 = BN_to_ASN1_INTEGER(serial_bn, nullptr);
        if (!serial_asn1 || X509_set_serialNumber(cert, serial_asn1) != 1) {
            throw std::runtime_error("failed to set certificate serial");
        }
        if (!X509_gmtime_adj(X509_get_notBefore(cert), 0) ||
            !X509_gmtime_adj(X509_get_notAfter(cert), 31536000L)) {
            throw std::runtime_error("failed to set certificate validity");
        }
        if (X509_set_pubkey(cert, pkey) != 1) {
            throw std::runtime_error("failed to set certificate public key");
        }

        X509_NAME* subject = X509_get_subject_name(cert);
        if (!subject ||
            X509_NAME_add_entry_by_txt(
                subject,
                "CN",
                MBSTRING_ASC,
                reinterpret_cast<const unsigned char*>(common_name.data()),
                static_cast<int>(common_name.size()),
                -1,
                0) != 1 ||
            X509_set_issuer_name(cert, subject) != 1) {
            throw std::runtime_error("failed to configure certificate subject");
        }

        if (X509_sign(cert, pkey, EVP_sha256()) <= 0) {
            throw std::runtime_error("failed to self-sign certificate");
        }

        key_bio = BIO_new(BIO_s_mem());
        cert_bio = BIO_new(BIO_s_mem());
        if (!key_bio || !cert_bio ||
            PEM_write_bio_PrivateKey(key_bio, pkey, nullptr, nullptr, 0, nullptr, nullptr) != 1 ||
            PEM_write_bio_X509(cert_bio, cert) != 1) {
            throw std::runtime_error("failed to serialize test signing material");
        }

        material.private_key_pem = bioToString(key_bio);
        material.certificate_pem = bioToString(cert_bio);
    } catch (...) {
        BIO_free(key_bio);
        BIO_free(cert_bio);
        ASN1_INTEGER_free(serial_asn1);
        BN_free(serial_bn);
        X509_free(cert);
        EVP_PKEY_free(pkey);
        throw;
    }

    BIO_free(key_bio);
    BIO_free(cert_bio);
    ASN1_INTEGER_free(serial_asn1);
    BN_free(serial_bn);
    X509_free(cert);
    EVP_PKEY_free(pkey);
    return material;
}

[[nodiscard]] inline const PemMaterial& getSignedRequestPemMaterial() {
    static const PemMaterial material =
        generateSelfSignedPemMaterial("7E57A110C0DE1234", "themis-test");
    return material;
}

[[nodiscard]] inline const std::string& getSamlIdpCertificatePem() {
    static const std::string pem =
        generateSelfSignedPemMaterial("1D90C0DE20260819", "test-idp").certificate_pem;
    return pem;
}

[[nodiscard]] inline const std::string& getTestSpPrivateKeyPem() {
    static const std::string pem = generateRsaPrivateKeyPem();
    return pem;
}

[[nodiscard]] inline const std::string& getWrongSpPrivateKeyPem() {
    static const std::string pem = generateRsaPrivateKeyPem();
    return pem;
}

}  // namespace themis::tests
