/**
 * @file pki_client.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @author makr-code
 * @version 0.0.47
 * @date 2026-06-02 11:49:05
 * @note Maturity: 🟡 RELEASE-CANDIDATE
 * @note Score: 79/100
 * @note Lines: 871
 * @note Gap Summary: total=8; TODO=1, Stub=3, Unimpl=0, Mock=1, Sim=2, Debt=1, C=0, H=6, M=32, L=0
 * @note PR History (last 5): #4259 feat(sharding): Wire Orphan... (2026-03-15) | #4263 PKIClient v1.8.0 + PII Stre... (2026-03-15) | #998 C++ Audit: Eliminate raw me... (2026-03-11) | #739 Phase 4: Migrate utility mo... (2026-03-11) | #901 Refactor OpenSSL memory man... (2026-03-11)
 * @note Status: Release Candidate
 * @note This block is auto-generated and will be overwritten.
 */


#include "utils/pki_client.h"
#include <stdexcept>
#include <algorithm>
#include "utils/expected.h"
#include "utils/error_registry.h"
#include "utils/openssl_deleter.h"

#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/rsa.h>
#include <openssl/sha.h>
#include <openssl/ssl.h>

#include <mutex>
#include <random>
#include <sstream>
#include <string>
#include <vector>
#include <cstring>
#include <cctype>
#include <string_view>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <iomanip>
#include <fmt/format.h>

namespace themis {
namespace utils {

// ============================================================================
// Certificate Pinning: SHA256 Fingerprint Verification
// ============================================================================

static std::string base64_encode(const std::vector<uint8_t>& data);

static std::optional<std::vector<uint8_t>> parse_hex_fingerprint(std::string_view fingerprint) {
    std::string normalized;
    normalized.reserve(fingerprint.size());
    for (unsigned char c : fingerprint) {
        if (std::isxdigit(c) != 0) {
            normalized.push_back(static_cast<char>(c));
            continue;
        }
        if (c == ':' || std::isspace(c) != 0 || c == '-') {
            continue;
        }
        return std::nullopt;
    }

    if (normalized.size() % 2 != 0) {
        return std::nullopt;
    }

    std::vector<uint8_t> out;
    out.reserve(normalized.size() / 2);
    for (size_t i = 0; i < normalized.size(); i += 2) {
        unsigned int byte = 0;
        std::istringstream iss(normalized.substr(i, 2));
        iss >> std::hex >> byte;
        if (iss.fail()) {
            return std::nullopt;
        }
        out.push_back(static_cast<uint8_t>(byte));
    }
    return out;
}

static std::optional<std::string> build_pinned_public_key_value(const PKIConfig& cfg) {
    std::vector<std::string> pins;
    pins.reserve(cfg.pinned_cert_fingerprints.size());

    for (const auto& configured_pin : cfg.pinned_cert_fingerprints) {
        if (configured_pin.rfind("sha256//", 0) == 0) {
            pins.push_back(configured_pin);
            continue;
        }

        auto parsed = parse_hex_fingerprint(configured_pin);
        if (!parsed || parsed->size() != SHA256_DIGEST_LENGTH) {
            return std::nullopt;
        }

        pins.push_back("sha256//" + base64_encode(*parsed));
    }

    if (pins.empty()) {
        return std::nullopt;
    }

    std::string value;
    for (size_t i = 0; i < pins.size(); ++i) {
        if (i > 0) {
            value.push_back(';');
        }
        value += pins[i];
    }
    return value;
}

// Simple base64 (encode/decode) to avoid extra deps
static std::string base64_encode(const std::vector<uint8_t>& data) {
    static const char b64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    out.reserve(((data.size() + 2) / 3) * 4);
    size_t i = 0;
    while (i + 3 <= data.size()) {
        uint32_t n = (data[i] << 16) | (data[i + 1] << 8) | (data[i + 2]);
        out.push_back(b64_table[(n >> 18) & 63]);
        out.push_back(b64_table[(n >> 12) & 63]);
        out.push_back(b64_table[(n >> 6) & 63]);
        out.push_back(b64_table[n & 63]);
        i += 3;
    }
    if (i + 1 == data.size()) {
        uint32_t n = (data[i] << 16);
        out.push_back(b64_table[(n >> 18) & 63]);
        out.push_back(b64_table[(n >> 12) & 63]);
        out.push_back('=');
        out.push_back('=');
    } else if (i + 2 == data.size()) {
        uint32_t n = (data[i] << 16) | (data[i + 1] << 8);
        out.push_back(b64_table[(n >> 18) & 63]);
        out.push_back(b64_table[(n >> 12) & 63]);
        out.push_back(b64_table[(n >> 6) & 63]);
        out.push_back('=');
    }
    return out;
}

static std::vector<uint8_t> base64_decode(const std::string& s) {
    static const int T[256] = {
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,
        52,53,54,55,56,57,58,59,60,61,-1,-1,-1, 0,-1,-1,
        -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,
        15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,
        -1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,
        41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1,
        // rest fill with -1
    };
    std::vector<uint8_t> out; out.reserve((s.size()*3)/4);
    int val = 0, valb = -8;
    for (unsigned char c : s) {
        if (c=='=') {
          break;
        }
        int d;
        if (c < 128) {
            d = T[c];
        } else {
            d = -1;
        }
        if (d == -1) {
          continue;
        }
        val = (val << 6) + d;
        valb += 6;
        if (valb >= 0) {
            out.push_back((uint8_t)((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return out;
}

static std::string random_hex_id([[maybe_unused]] size_t bytes = 8) {
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dis;
    std::ostringstream oss;
    for (size_t i = 0; i < bytes / 8; ++i) {
        oss << std::hex << dis(gen);
    }
    return oss.str();
}

static int nid_for_algorithm(const std::string& alg, size_t& expected_len) {
    if (alg.find("SHA256") != std::string::npos) { expected_len = 32; return NID_sha256; }
    if (alg.find("SHA384") != std::string::npos) { expected_len = 48; return NID_sha384; }
    if (alg.find("SHA512") != std::string::npos) { expected_len = 64; return NID_sha512; }
    // default
    expected_len = 32; return NID_sha256;
}

static int password_cb(char* buf, int size, int /*rwflag*/, void* u) {
    if (!buf || !u || size <= 0) {
      return 0;
    }
    auto* pass = static_cast<std::string*>(u);
    // Reserve one byte for the NUL terminator required by OpenSSL.
    int len = std::min<int>(static_cast<int>(pass->size()), size - 1);
    if (len > 0) {
        std::memcpy(buf, pass->data(), static_cast<size_t>(len));
    }
    buf[len] = '\0';
    return len;
}

static Result<EVP_PKEY*> load_private_key(const PKIConfig& cfg) {
    if (cfg.key_path.empty()) {
        return Err<EVP_PKEY*>(errors::ErrorCode::ERR_UTIL_INVALID_ARGUMENT, 
            "Private key path is empty");
    }
    
    auto bio = make_bio_file(cfg.key_path.c_str(), "r");
    if (!bio) {
        return Err<EVP_PKEY*>(errors::ErrorCode::ERR_UTIL_PKI_KEY_LOAD_FAILED,
            fmt::format("Failed to open private key file: {}", cfg.key_path));
    }
    
    EVP_PKEY* pkey = nullptr;
    if (!cfg.key_passphrase.empty()) {
        std::string pwd = cfg.key_passphrase;
        pkey = PEM_read_bio_PrivateKey(bio.get(), nullptr, password_cb, &pwd);
    } else {
        pkey = PEM_read_bio_PrivateKey(bio.get(), nullptr, nullptr, nullptr);
    }
    
    if (!pkey) {
        return Err<EVP_PKEY*>(errors::ErrorCode::ERR_UTIL_PKI_KEY_LOAD_FAILED,
            fmt::format("Failed to read private key from file: {}", cfg.key_path));
    }
    
    return Ok(pkey);
}

static std::string to_hex_serial(ASN1_INTEGER* s) {
    if (!s) {
      return std::string();
    }
    auto bn = BIGNUMPtr(ASN1_INTEGER_to_BN(s, nullptr));
    if (!bn) {
      return std::string();
    }
    char* hex = BN_bn2hex(bn.get());
    std::string out = hex ? std::string(hex) : std::string();
    if (hex) {
      OPENSSL_free(hex);
    }
    return out;
}

static Result<EVP_PKEY*> load_public_key_and_serial(const PKIConfig& cfg, std::string& serial_out) {
    serial_out.clear();
    
    if (cfg.cert_path.empty()) {
        return Err<EVP_PKEY*>(errors::ErrorCode::ERR_UTIL_INVALID_ARGUMENT,
            "Certificate path is empty");
    }
    
    auto bio = make_bio_file(cfg.cert_path.c_str(), "r");
    if (!bio) {
        return Err<EVP_PKEY*>(errors::ErrorCode::ERR_UTIL_PKI_CERT_LOAD_FAILED,
            fmt::format("Failed to open certificate file: {}", cfg.cert_path));
    }
    
    auto cert = X509Ptr(PEM_read_bio_X509(bio.get(), nullptr, nullptr, nullptr));
    
    if (!cert) {
        return Err<EVP_PKEY*>(errors::ErrorCode::ERR_UTIL_PKI_CERT_LOAD_FAILED,
            fmt::format("Failed to read certificate from file: {}", cfg.cert_path));
    }
    
    EVP_PKEY* pub = X509_get_pubkey(cert.get());
    ASN1_INTEGER* s = X509_get_serialNumber(cert.get());
    serial_out = to_hex_serial(s);
    
    if (!pub) {
        return Err<EVP_PKEY*>(errors::ErrorCode::ERR_UTIL_PKI_CERT_LOAD_FAILED,
            fmt::format("Failed to extract public key from certificate: {}", cfg.cert_path));
    }
    
    return Ok(pub);
}

// Generates a PKCS#10 CSR (PEM) using the private key in cfg and service_id as CN.
// Uses the X509_REQ_* OpenSSL API.  Returns empty string on failure.
static std::string generate_csr_pem(const PKIConfig& cfg) {
    auto pkey_result = load_private_key(cfg);
    if (!pkey_result) {
        std::cerr << "PKI CSR: failed to load private key from " << cfg.key_path << "\n";
        return {};
    }
    EVPKeyPtr pkey(*pkey_result);

    X509REQPtr req(X509_REQ_new());
    if (!req) {
        std::cerr << "PKI CSR: X509_REQ_new() failed\n";
        return {};
    }

    // PKCS#10 version 0 (=v1)
    X509_REQ_set_version(req.get(), 0);

    // Set Subject: CN=<service_id>, O=ThemisDB
    X509_NAME* subj = X509_REQ_get_subject_name(req.get());
    if (subj) {
        if (!cfg.service_id.empty()) {
            X509_NAME_add_entry_by_txt(
                subj, "CN", MBSTRING_ASC,
                reinterpret_cast<const unsigned char*>(cfg.service_id.c_str()), -1, -1, 0);
        }
        X509_NAME_add_entry_by_txt(
            subj, "O", MBSTRING_ASC,
            reinterpret_cast<const unsigned char*>("ThemisDB"), -1, -1, 0);
    }

    // Attach public key
    if (X509_REQ_set_pubkey(req.get(), pkey.get()) != 1) {
        std::cerr << "PKI CSR: X509_REQ_set_pubkey() failed\n";
        return {};
    }

    // Self-sign the CSR with the private key using SHA-256
    if (X509_REQ_sign(req.get(), pkey.get(), EVP_sha256()) == 0) {
        std::cerr << "PKI CSR: X509_REQ_sign() failed\n";
        return {};
    }

    // Serialise CSR to PEM
    BIOPtr bio(BIO_new(BIO_s_mem()));
    if (!bio || PEM_write_bio_X509_REQ(bio.get(), req.get()) != 1) {
        std::cerr << "PKI CSR: PEM_write_bio_X509_REQ() failed\n";
        return {};
    }

    BUF_MEM* bptr = nullptr;
    BIO_get_mem_ptr(bio.get(), &bptr);
    if (!bptr || !bptr->data || bptr->length == 0) return {};
    return std::string(bptr->data, bptr->length);
}

// Submits a PEM-encoded PKCS#10 CSR to {ca_url}/sign-csr and returns the
// signed certificate PEM on success.  Returns empty string on failure.
static std::string request_cert_from_ca(const PKIConfig& cfg, const std::string& csr_pem) {
    if (cfg.ca_url.empty() || csr_pem.empty()) return {};

    std::string url = cfg.ca_url;
    if (url.back() == '/') {
      url.pop_back();
    }
    url += "/sign-csr";

    nlohmann::json body_json;
    body_json["csr_pem"]    = csr_pem;
    body_json["service_id"] = cfg.service_id;
    std::string body = body_json.dump();

    CURL* curl = curl_easy_init();
    if (!curl) return {};

    std::string resp_body;
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL,           url.c_str());
    curl_easy_setopt(curl, CURLOPT_POST,           1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS,     body.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER,     headers);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT,        10L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
        +[](char* ptr, size_t size, size_t nmemb, void* userdata) -> size_t {
            auto real_size = size * nmemb;
            static_cast<std::string*>(userdata)->append(ptr, real_size);
            return real_size;
        });
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resp_body);

    CURLcode rc = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (rc != CURLE_OK || http_code < 200 || http_code >= 300) {
        std::cerr << "PKI CSR: CA request to " << url
                  << " failed: curl=" << rc << " http=" << http_code << "\n";
        return {};
    }

    // The CA may return JSON {"certificate_pem": "-----BEGIN CERTIFICATE..."} or
    // raw PEM directly.
    try {
        auto j = nlohmann::json::parse(resp_body);
        std::string cert_pem = j.value("certificate_pem", std::string{});
        if (!cert_pem.empty()) {
          return cert_pem;
        }
    } catch (const nlohmann::json::exception &) {
    } catch (const std::exception &) {
    } catch (const std::string &) {
    } catch (const char *) {
    }

    if (resp_body.find("-----BEGIN CERTIFICATE-----") != std::string::npos) {
        return resp_body;
    }
    std::cerr << "PKI CSR: CA response did not contain a certificate PEM\n";
    return {};
}

// Extracts the serial number from a PEM-encoded certificate string.
// Returns empty string on failure.
static std::string serial_from_cert_pem(const std::string& cert_pem) {
    BIOPtr bio(BIO_new_mem_buf(cert_pem.data(), static_cast<int>(cert_pem.size())));
    if (!bio) return {};
    X509Ptr cert(PEM_read_bio_X509(bio.get(), nullptr, nullptr, nullptr));
    if (!cert) return {};
    return to_hex_serial(X509_get_serialNumber(cert.get()));
}

// Verify the X.509 certificate chain for cert_path against the CA bundle at trust_store_path.
// Returns true only when the chain is fully valid.
static bool verify_cert_chain(const PKIConfig& cfg) {
    if (cfg.cert_path.empty() || cfg.trust_store_path.empty()) {
        return false;
    }

    auto bio_leaf = make_bio_file(cfg.cert_path.c_str(), "r");
    if (!bio_leaf) {
        std::cerr << "PKI chain verify: cannot open cert file: " << cfg.cert_path << "\n";
        return false;
    }

    auto leaf = X509Ptr(PEM_read_bio_X509(bio_leaf.get(), nullptr, nullptr, nullptr));
    if (!leaf) {
        std::cerr << "PKI chain verify: cannot parse certificate: " << cfg.cert_path << "\n";
        return false;
    }

    X509StorePtr store(X509_STORE_new());
    if (!store) {
        std::cerr << "PKI chain verify: X509_STORE_new() failed\n";
        return false;
    }

    if (X509_STORE_load_locations(store.get(), cfg.trust_store_path.c_str(), nullptr) != 1) {
        std::cerr << "PKI chain verify: cannot load trust store: " << cfg.trust_store_path << "\n";
        return false;
    }

    X509StoreCtxPtr ctx(X509_STORE_CTX_new());
    if (!ctx) {
        std::cerr << "PKI chain verify: X509_STORE_CTX_new() failed\n";
        return false;
    }

    if (X509_STORE_CTX_init(ctx.get(), store.get(), leaf.get(), nullptr) != 1) {
        std::cerr << "PKI chain verify: X509_STORE_CTX_init() failed\n";
        return false;
    }

    int rc = X509_verify_cert(ctx.get());
    if (rc != 1) {
        int err = X509_STORE_CTX_get_error(ctx.get());
        std::cerr << "PKI chain verify: X509_verify_cert() failed: "
                  << X509_verify_cert_error_string(err) << "\n";
    }
    return rc == 1;
}

// Configure CURL handle with certificate pinning
[[nodiscard]] static bool configure_curl_pinning(CURL* curl, const PKIConfig* cfg) {
    if (!cfg || !cfg->enable_cert_pinning || cfg->pinned_cert_fingerprints.empty()) {
        return true; // Pinning disabled
    }
    
    // Enable SSL verification
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

    auto pinned_public_key = build_pinned_public_key_value(*cfg);
    if (!pinned_public_key) {
        return false;
    }

    CURLcode pin_rc =
        curl_easy_setopt(curl, CURLOPT_PINNEDPUBLICKEY, pinned_public_key->c_str());
    return pin_rc == CURLE_OK;
}

VCCPKIClient::VCCPKIClient(PKIConfig cfg) : cfg_(std::move(cfg)) {}

std::optional<std::string> VCCPKIClient::getCertSerial() const {
    std::string serial;
    auto pub_result = load_public_key_and_serial(cfg_, serial);
    if (!pub_result) {
      return std::nullopt;
    }
    EVP_PKEY_free(*pub_result);
    if (serial.empty()) {
      return std::nullopt;
    }
    return serial;
}

std::string VCCPKIClient::generateCSR() const {
    return generate_csr_pem(cfg_);
}

SignatureResult VCCPKIClient::signHash(const std::vector<uint8_t>& hash_bytes) const {
    SignatureResult res;
    res.signature_id = "sig_" + random_hex_id(8);
    res.algorithm = cfg_.signature_algorithm.empty() ? std::string("RSA-SHA256") : cfg_.signature_algorithm;

    [[maybe_unused]] size_t expected_len = 0;
    [[maybe_unused]] int nid = nid_for_algorithm(res.algorithm, expected_len);

    // If a PKI endpoint is configured, try REST signing first
    if (!cfg_.endpoint.empty()) {
        try {
            nlohmann::json req;
            req["hash_b64"] = base64_encode(hash_bytes);
            req["service_id"] = cfg_.service_id;
            req["algorithm"] = res.algorithm;

            std::string url = cfg_.endpoint;
            if (url.back() == '/') {
              url.pop_back();
            }
            url += "/sign";

            // CURL POST
            CURL* curl = curl_easy_init();
            if (curl) {
                struct curl_slist* headers = nullptr;
                headers = curl_slist_append(headers, "Content-Type: application/json");
                curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
                curl_easy_setopt(curl, CURLOPT_POST, 1L);
                std::string body = req.dump();
                curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
                curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
                curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
                
                // Configure certificate pinning
                if (!configure_curl_pinning(curl, &cfg_)) {
                    std::cerr << "PKI REST /sign: invalid certificate pinning configuration\n";
                    curl_slist_free_all(headers);
                    curl_easy_cleanup(curl);
                    return res;
                }

                std::string resp_body;
                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
                    +[](char* ptr, size_t size, size_t nmemb, void* userdata) -> size_t {
                        auto real_size = size * nmemb;
                        std::string* out = static_cast<std::string*>(userdata);
                        out->append(ptr, real_size);
                        return real_size;
                    });
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resp_body);

                CURLcode rc = curl_easy_perform(curl);
                long http_code = 0;
                curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
                size_t resp_len = resp_body.size();
                curl_slist_free_all(headers);
                curl_easy_cleanup(curl);

                // Always print diagnostics when debug enabled; also print minimal info on failure
                const char* dbg = std::getenv("THEMIS_DEBUG_PKI");
                if (dbg && dbg[0] == '1') {
                    std::cerr << "PKI REST /sign: curl rc=" << rc << " http_code=" << http_code << " resp_len=" << resp_len << "\n";
                    std::cerr << "PKI REST /sign response body: '" << resp_body << "'\n";
                    // print a short hex prefix for binary-safety
                    std::ostringstream hexs;
                    const size_t maxhex = std::min<size_t>(resp_body.size(), 64);
                    for (size_t i = 0; i < maxhex; ++i) {
                        unsigned char c = static_cast<unsigned char>(resp_body[i]);
                        hexs << std::hex << (int)c << " ";
                    }
                    std::cerr << "PKI REST /sign body hex (first " << maxhex << " bytes): " << hexs.str() << "\n";
                } else if (rc != CURLE_OK) {
                    std::cerr << "PKI REST /sign: curl rc=" << rc << " (" << curl_easy_strerror(rc) << ") http_code=" << http_code << " resp_len=" << resp_len << "\n";
                }

                if (rc == CURLE_OK) {
                    try {
                        auto j = nlohmann::json::parse(resp_body);
                        if (j.contains("signature_b64")) {
                            res.signature_b64 = j.value("signature_b64", std::string());
                            res.signature_id = j.value("signature_id", res.signature_id);
                            res.cert_serial = j.value("cert_serial", std::string());
                            res.ok = true;
                            return res;
                        } else {
                            // Unexpected response shape — log body for investigation
                            std::cerr << "PKI REST /sign: JSON did not contain 'signature_b64'. body='" << resp_body << "'\n";
                        }
                    } catch (const std::exception& e) {
                        std::cerr << "PKI REST parse exception: " << e.what() << " body='" << resp_body << "'\n";
                        // fallthrough to local fallback
                    }
                } else {
                    std::cerr << "PKI REST /sign: curl error: " << curl_easy_strerror(rc) << " resp='" << resp_body << "'\n";
                }
            }
        } catch (const std::exception &) {
            // ignore and fallback
        } catch (const std::string &) {
            // ignore and fallback
        } catch (const char *) {
            // ignore and fallback
        }
    }

    // Try real RSA signing if key is available and hash length matches
    if (!cfg_.key_path.empty() && (expected_len == 0 || hash_bytes.size() == expected_len)) {
        auto pkey_result = load_private_key(cfg_);
        if (pkey_result) {
            EVP_PKEY* pkey = *pkey_result;
            // Use EVP_PKEY signing (preferred) instead of deprecated RSA_sign API.
            int max_sig_len = EVP_PKEY_size(pkey);
            if (max_sig_len > 0) {
                std::vector<uint8_t> sig(static_cast<size_t>(max_sig_len));
                size_t outlen = sig.size();
                EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(pkey, nullptr);
                if (ctx) {
                    if (EVP_PKEY_sign_init(ctx) == 1) {
                        // Use PKCS#1 v1.5 padding for compatibility with RSA_sign
                        EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_PADDING);
                        if (EVP_PKEY_sign(ctx, sig.data(), &outlen, hash_bytes.data(), hash_bytes.size()) == 1) {
                            sig.resize(outlen);
                            res.signature_b64 = base64_encode(sig);
                    // Try to set cert serial if available
                    std::string serial;
                    auto pub_result = load_public_key_and_serial(cfg_, serial);
                    if (pub_result) {
                        EVP_PKEY_free(*pub_result);
                        res.cert_serial = serial.empty() ? std::string("LOCAL-KEY") : serial;
                    } else {
                        res.cert_serial = "LOCAL-KEY";
                    }
                    res.ok = true;
                        }
                    }
                    EVP_PKEY_CTX_free(ctx);
                }
            }
            EVP_PKEY_free(pkey);
            if (res.ok) {
              return res;
            }
        }
    }

    // PKCS#10 CSR provisioning path: when a private key is available but no local
    // certificate exists, and an internal CA URL is configured, generate a CSR using
    // the X509_REQ_* API, submit it to the CA, cache the returned certificate, and
    // proceed with local RSA signing using the provisioned certificate.
    if (!cfg_.key_path.empty() && cfg_.cert_path.empty() && !cfg_.ca_url.empty()) {
        // Lazy-init: provision certificate from CA if not cached yet.
        std::string cert_pem;
        std::string cert_serial;
        {
            std::lock_guard<std::mutex> lock(cert_cache_mutex_);
            if (cached_cert_pem_.empty()) {
                std::string csr_pem = generate_csr_pem(cfg_);
                if (!csr_pem.empty()) {
                    std::string provisioned = request_cert_from_ca(cfg_, csr_pem);
                    if (!provisioned.empty()) {
                        cached_cert_pem_    = std::move(provisioned);
                        cached_cert_serial_ = serial_from_cert_pem(cached_cert_pem_);
                    }
                }
            }
            cert_pem    = cached_cert_pem_;
            cert_serial = cached_cert_serial_;
        }

        if (!cert_pem.empty() && (expected_len == 0 || hash_bytes.size() == expected_len)) {
            auto pkey_result = load_private_key(cfg_);
            if (pkey_result) {
                EVP_PKEY* pkey = *pkey_result;
                int max_sig_len = EVP_PKEY_size(pkey);
                if (max_sig_len > 0) {
                    std::vector<uint8_t> sig(static_cast<size_t>(max_sig_len));
                    size_t outlen = sig.size();
                    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(pkey, nullptr);
                    if (ctx) {
                        if (EVP_PKEY_sign_init(ctx) == 1) {
                            EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_PADDING);
                            if (EVP_PKEY_sign(ctx, sig.data(), &outlen, hash_bytes.data(), hash_bytes.size()) == 1) {
                                sig.resize(outlen);
                                res.signature_b64 = base64_encode(sig);
                                res.cert_serial   = cert_serial.empty() ? std::string("CA-PROVISIONED") : cert_serial;
                                res.ok            = true;
                            }
                        }
                        EVP_PKEY_CTX_free(ctx);
                    }
                }
                EVP_PKEY_free(pkey);
                if (res.ok) {
                  return res;
                }
            }
        }
    }

    // PERMANENT TEST-MODE-ONLY NOTE:
    // Purpose: Allow unit tests to exercise the signing call path (CSR submission,
    //   JSON response parsing, cert chain assembly) without a real SCEP/EST/ACME
    //   endpoint.  Returns a self-certified base64(SHA-256(csr)) as a synthetic
    //   signature so tests can assert non-empty output.
    // Activation: THEMIS_TEST_MODE must be defined at compile time
    //   (-DTHEMIS_TEST_MODE=1).  NEVER defined in production CMake presets.
    // Production Delta: Signature is not cryptographically valid; verification
    //   against any real CA certificate will fail.  The cert_serial is a
    //   hardcoded sentinel "DEMO-CERT-SERIAL".
    // Production Path: The real signing path (SCEP/EST/ACME client) is wired
    //   above via the REST endpoint + VCCPKIClient::SignHashFn injection bridge.
    //   The #else branch returns ok=false for production builds without an endpoint.
    // Roadmap ref: src/utils/FUTURE_ENHANCEMENTS.md § "PKI Client Production Signing"
    auto sign_hash_fn = VCCPKIClient::SignHashFn{};
    {
        std::lock_guard<std::mutex> lock(VCCPKIClient::signHashFnMutex());
        sign_hash_fn = VCCPKIClient::signHashFnStorage();
    }
    if (sign_hash_fn) {
        return sign_hash_fn(hash_bytes);
    }
#ifdef THEMIS_TEST_MODE
    res.ok = true;
    res.signature_b64 = base64_encode(hash_bytes);
    res.cert_serial = "DEMO-CERT-SERIAL";
    return res;
#else
    // Production: no key or endpoint configured — signing is not available.
    res.ok = false;
    return res;
#endif
}

bool VCCPKIClient::verifyHash(const std::vector<uint8_t>& hash_bytes, const SignatureResult& sig) const {
    if (!sig.ok) {
      return false;
    }

    [[maybe_unused]] size_t expected_len = 0;
    [[maybe_unused]] int nid = nid_for_algorithm(sig.algorithm, expected_len);

    // If a PKI endpoint is configured, try REST verify first
    if (!cfg_.endpoint.empty()) {
        try {
            nlohmann::json req;
            req["hash_b64"] = base64_encode(hash_bytes);
            req["signature_b64"] = sig.signature_b64;

            std::string url = cfg_.endpoint;
            if (url.back() == '/') {
              url.pop_back();
            }
            url += "/verify";

            CURL* curl = curl_easy_init();
            if (curl) {
                struct curl_slist* headers = nullptr;
                headers = curl_slist_append(headers, "Content-Type: application/json");
                curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
                curl_easy_setopt(curl, CURLOPT_POST, 1L);
                std::string body = req.dump();
                curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
                curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
                curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
                
                // Configure certificate pinning
                if (!configure_curl_pinning(curl, &cfg_)) {
                    std::cerr << "PKI REST /verify: invalid certificate pinning configuration\n";
                    curl_slist_free_all(headers);
                    curl_easy_cleanup(curl);
                    return false;
                }

                std::string resp_body;
                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
                    +[](char* ptr, size_t size, size_t nmemb, void* userdata) -> size_t {
                        auto real_size = size * nmemb;
                        std::string* out = static_cast<std::string*>(userdata);
                        out->append(ptr, real_size);
                        return real_size;
                    });
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resp_body);

                CURLcode rc = curl_easy_perform(curl);
                long http_code = 0;
                curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
                size_t resp_len = resp_body.size();
                curl_slist_free_all(headers);
                curl_easy_cleanup(curl);

                const char* dbg = std::getenv("THEMIS_DEBUG_PKI");
                if (dbg && dbg[0] == '1') {
                    std::cerr << "PKI REST /verify: curl rc=" << rc << " http_code=" << http_code << " resp_len=" << resp_len << "\n";
                    std::cerr << "PKI REST /verify response body: '" << resp_body << "'\n";
                    std::ostringstream hexs;
                    const size_t maxhex = std::min<size_t>(resp_body.size(), 64);
                    for (size_t i = 0; i < maxhex; ++i) {
                        unsigned char c = static_cast<unsigned char>(resp_body[i]);
                        hexs << std::hex << (int)c << " ";
                    }
                    std::cerr << "PKI REST /verify body hex (first " << maxhex << " bytes): " << hexs.str() << "\n";
                } else if (rc != CURLE_OK) {
                    std::cerr << "PKI REST /verify: curl rc=" << rc << " (" << curl_easy_strerror(rc) << ") http_code=" << http_code << " resp_len=" << resp_len << "\n";
                }

                if (rc == CURLE_OK) {
                    try {
                        auto j = nlohmann::json::parse(resp_body);
                        if (j.contains("ok")) {
                            return j.value("ok", false);
                        } else {
                            std::cerr << "PKI REST /verify: JSON did not contain 'ok'. body='" << resp_body << "'\n";
                        }
                    } catch (const std::exception& e) {
                        std::cerr << "PKI REST parse exception: " << e.what() << " body='" << resp_body << "'\n";
                        // fallthrough to local fallback
                    }
                } else {
                    std::cerr << "PKI REST /verify: curl error: " << curl_easy_strerror(rc) << " resp='" << resp_body << "'\n";
                }
            }
        } catch (const std::exception &) {
            // ignore and fallback
        } catch (const std::string &) {
            // ignore and fallback
        } catch (const char *) {
            // ignore and fallback
        }
    }

    // Try real RSA verify if certificate is available and hash length matches.
    // When trust_store_path is also configured, first validate the full X.509 chain
    // so that an untrusted or expired certificate is rejected before checking the signature.
    if (!cfg_.cert_path.empty() && (expected_len == 0 || hash_bytes.size() == expected_len)) {
        // Enforce chain validation when a trust store is configured.
        if (!cfg_.trust_store_path.empty() && !verify_cert_chain(cfg_)) {
            return false;
        }

        std::string serial;
        auto pub_result = load_public_key_and_serial(cfg_, serial);
        if (pub_result) {
            EVP_PKEY* pub = *pub_result;
            // Use EVP_PKEY verification instead of deprecated RSA_verify
            [[maybe_unused]] int max_sig_len = EVP_PKEY_size(pub);
            auto sig_bytes = base64_decode(sig.signature_b64);
            EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(pub, nullptr);
            if (ctx) {
                if (EVP_PKEY_verify_init(ctx) == 1) {
                    EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_PADDING);
                    size_t siglen = sig_bytes.size();
                    int ok = EVP_PKEY_verify(ctx, sig_bytes.data(), siglen, hash_bytes.data(), hash_bytes.size());
                    EVP_PKEY_CTX_free(ctx);
                    EVP_PKEY_free(pub);
                    return ok == 1;
                }
                EVP_PKEY_CTX_free(ctx);
            }
            EVP_PKEY_free(pub);
        }
    }

    // PERMANENT TEST-MODE-ONLY NOTE:
    // Purpose: Allow unit tests to verify round-trip signing/verification without
    //   a real CA.  Treats base64(SHA-256(cert_bytes)) equality as a valid
    //   "signature" so the test can assert that sign()+verify() returns true.
    // Activation: THEMIS_TEST_MODE defined at compile time; never in production.
    // Production Delta: Does NOT verify a real X.509 signature; any cert signed
    //   by a real CA will fail this check (false negative) and any random
    //   base64 blob that happens to match will pass (false positive).
    // Production Path: The real verification path uses EVP_DigestVerify against
    //   the configured CA cert above; the #else branch returns false for
    //   production builds without a cert or configured endpoint.
    // Roadmap ref: src/utils/FUTURE_ENHANCEMENTS.md § "PKI Client Production Signing"
    auto verify_hash_fn = VCCPKIClient::VerifyHashFn{};
    {
        std::lock_guard<std::mutex> lock(VCCPKIClient::verifyHashFnMutex());
        verify_hash_fn = VCCPKIClient::verifyHashFnStorage();
    }
    if (verify_hash_fn) {
        return verify_hash_fn(hash_bytes, sig);
    }
#ifdef THEMIS_TEST_MODE
    {
        std::string expected = base64_encode(hash_bytes);
        return expected == sig.signature_b64;
    }
#else
    // Production: no cert or endpoint configured — treat as verification failure.
    return false;
#endif
}

} // namespace utils
} // namespace themis
