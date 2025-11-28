// TimestampAuthority implementation: Stub by default, OpenSSL/CURL when THEMIS_ENABLE_TSA is defined.
// Stub mode provides deterministic, non-cryptographic timestamps for tests.
// When TSA is enabled, integrates with RFC 3161 timestamp servers via OpenSSL and CURL.

#include "security/timestamp_authority.h"
#include "utils/logger.h"

#ifdef THEMIS_ENABLE_TSA
#include <openssl/ts.h>
#include <openssl/sha.h>
#include <openssl/bio.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <curl/curl.h>
#include <random>
#endif

#include <chrono>
#include <sstream>
#include <iomanip>
#include <cstring>

namespace themis { namespace security {

// ============================================================================
// Helper functions
// ============================================================================

static std::string hex(const std::vector<uint8_t>& data) {
    static const char* d = "0123456789abcdef";
    std::string out;
    out.reserve(data.size() * 2);
    for (auto b : data) {
        out.push_back(d[(b >> 4) & 0xF]);
        out.push_back(d[b & 0xF]);
    }
    return out;
}

static std::vector<uint8_t> pseudo_hash(const std::vector<uint8_t>& data) {
    // Very weak "hash": each byte xor with index (for stub only)
    std::vector<uint8_t> h;
    h.reserve(data.size());
    for (size_t i = 0; i < data.size(); ++i) {
        h.push_back(static_cast<uint8_t>(data[i] ^ (i & 0xFF)));
    }
    return h;
}

#ifdef THEMIS_ENABLE_TSA

static std::string base64_encode(const std::vector<uint8_t>& input) {
    BIO* bmem = BIO_new(BIO_s_mem());
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    BIO_push(b64, bmem);
    BIO_write(b64, input.data(), static_cast<int>(input.size()));
    BIO_flush(b64);
    BUF_MEM* bptr;
    BIO_get_mem_ptr(b64, &bptr);
    std::string result(bptr->data, bptr->length);
    BIO_free_all(b64);
    return result;
}

static std::vector<uint8_t> base64_decode(const std::string& encoded_string) {
    static const std::string base64_chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    
    size_t in_len = encoded_string.size();
    int i = 0;
    int in_ = 0;
    uint8_t char_array_4[4], char_array_3[3];
    std::vector<uint8_t> ret;
    
    while (in_len-- && (encoded_string[in_] != '=') && 
           (isalnum(encoded_string[in_]) || (encoded_string[in_] == '+') || (encoded_string[in_] == '/'))) {
        char_array_4[i++] = encoded_string[in_];
        in_++;
        if (i == 4) {
            for (i = 0; i < 4; i++)
                char_array_4[i] = static_cast<uint8_t>(base64_chars.find(char_array_4[i]));
            
            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];
            
            for (i = 0; i < 3; i++)
                ret.push_back(char_array_3[i]);
            i = 0;
        }
    }
    
    if (i) {
        for (int j = i; j < 4; j++)
            char_array_4[j] = 0;
        
        for (int j = 0; j < 4; j++)
            char_array_4[j] = static_cast<uint8_t>(base64_chars.find(char_array_4[j]));
        
        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
        
        for (int j = 0; j < i - 1; j++)
            ret.push_back(char_array_3[j]);
    }
    
    return ret;
}

#endif // THEMIS_ENABLE_TSA

// ============================================================================
// TimestampAuthority::Impl
// ============================================================================

#ifdef THEMIS_ENABLE_TSA

class TimestampAuthority::Impl {
public:
    CURL* curl = nullptr;
    
    Impl() {
        curl = curl_easy_init();
    }
    
    ~Impl() {
        if (curl) {
            curl_easy_cleanup(curl);
        }
    }
    
    static size_t write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
        size_t total_size = size * nmemb;
        std::vector<uint8_t>* response = static_cast<std::vector<uint8_t>*>(userp);
        response->insert(response->end(),
                        static_cast<uint8_t*>(contents),
                        static_cast<uint8_t*>(contents) + total_size);
        return total_size;
    }
};

#else

// Stub implementation
class TimestampAuthority::Impl {
    // Empty stub
};

#endif // THEMIS_ENABLE_TSA

// ============================================================================
// TimestampAuthority implementation
// ============================================================================

TimestampAuthority::TimestampAuthority(TSAConfig config)
    : impl_(std::make_unique<Impl>()), config_(std::move(config)) {}

TimestampAuthority::~TimestampAuthority() = default;
TimestampAuthority::TimestampAuthority(TimestampAuthority&&) noexcept = default;
TimestampAuthority& TimestampAuthority::operator=(TimestampAuthority&&) noexcept = default;

TimestampToken TimestampAuthority::getTimestamp(const std::vector<uint8_t>& data) {
    auto hash = computeHash(data);
    return getTimestampForHash(hash);
}

TimestampToken TimestampAuthority::getTimestampForHash(const std::vector<uint8_t>& hash) {
#ifdef THEMIS_ENABLE_TSA
    TimestampToken token;
    
    // Generate nonce
    auto nonce = generateNonce(8);
    
    // Create TSP request
    auto request = createTSPRequest(hash, nonce);
    if (request.empty()) {
        token.error_message = "Failed to create TSP request";
        return token;
    }
    
    // Send request to TSA
    auto response = sendTSPRequest(request);
    if (response.empty()) {
        token.error_message = last_error_;
        return token;
    }
    
    // Parse response
    token = parseTSPResponse(response);
    token.nonce = nonce;
    
    return token;
#else
    // Stub implementation
    TimestampToken tok;
    tok.success = true;
    tok.hash_algorithm = config_.hash_algorithm;
    auto now = std::chrono::system_clock::now();
    tok.timestamp_unix_ms = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count());
    std::time_t tt = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
#ifdef _WIN32
    localtime_s(&tm, &tt);
#else
    localtime_r(&tt, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    tok.timestamp_utc = oss.str();
    tok.serial_number = "STUB-SERIAL";
    tok.policy_oid = config_.policy_oid;
    tok.nonce = generateNonce();
    tok.token_der = hash;
    tok.token_b64 = std::string("hex:") + hex(hash);
    tok.tsa_name = "STUB-TSA";
    tok.tsa_serial = "STUB-TSA-SERIAL";
    tok.verified = true;
    tok.cert_valid = true;
    return tok;
#endif
}

bool TimestampAuthority::verifyTimestamp(const std::vector<uint8_t>& data,
                                         const TimestampToken& token) {
    auto hash = computeHash(data);
    return verifyTimestampForHash(hash, token);
}

bool TimestampAuthority::verifyTimestampForHash(const std::vector<uint8_t>& hash,
                                                const TimestampToken& token) {
#ifdef THEMIS_ENABLE_TSA
    if (token.token_der.empty()) {
        THEMIS_ERROR("Empty timestamp token");
        return false;
    }
    
    // Parse PKCS7 token
    const unsigned char* p = token.token_der.data();
    PKCS7* pkcs7 = d2i_PKCS7(nullptr, &p, static_cast<long>(token.token_der.size()));
    if (!pkcs7) {
        THEMIS_ERROR("Failed to parse timestamp token");
        return false;
    }
    
    // Get TST_INFO
    TS_TST_INFO* tst_info = PKCS7_to_TS_TST_INFO(pkcs7);
    if (!tst_info) {
        PKCS7_free(pkcs7);
        return false;
    }
    
    // Verify message imprint
    TS_MSG_IMPRINT* imprint = TS_TST_INFO_get_msg_imprint(tst_info);
    ASN1_OCTET_STRING* hash_string = TS_MSG_IMPRINT_get_msg(imprint);
    
    bool hash_match = (hash.size() == static_cast<size_t>(hash_string->length) &&
                       memcmp(hash.data(), hash_string->data, hash.size()) == 0);
    
    TS_TST_INFO_free(tst_info);
    PKCS7_free(pkcs7);
    
    if (!hash_match) {
        THEMIS_ERROR("Timestamp hash mismatch");
        return false;
    }
    
    THEMIS_INFO("Timestamp verification successful");
    return true;
#else
    // Stub verification
    return token.success && (token.token_b64 == std::string("hex:") + hex(hash));
#endif
}

TimestampToken TimestampAuthority::parseToken(const std::vector<uint8_t>& token_data) {
#ifdef THEMIS_ENABLE_TSA
    return parseTSPResponse(token_data);
#else
    TimestampToken tok;
    tok.success = true;
    tok.token_der = token_data;
    tok.token_b64 = std::string("hex:") + hex(token_data);
    tok.serial_number = "PARSE";
    return tok;
#endif
}

TimestampToken TimestampAuthority::parseToken(const std::string& token_b64) {
#ifdef THEMIS_ENABLE_TSA
    auto token_der = base64_decode(token_b64);
    return parseToken(token_der);
#else
    TimestampToken tok;
    tok.success = true;
    tok.token_b64 = token_b64;
    tok.serial_number = "PARSE";
    return tok;
#endif
}

std::optional<std::string> TimestampAuthority::getTSACertificate() {
#ifdef THEMIS_ENABLE_TSA
    THEMIS_WARN("getTSACertificate() not fully implemented");
    return std::nullopt;
#else
    return std::string("-----BEGIN CERTIFICATE-----\nSTUB-TSA\n-----END CERTIFICATE-----\n");
#endif
}

bool TimestampAuthority::isAvailable() {
#ifdef THEMIS_ENABLE_TSA
    if (!impl_->curl) {
        return false;
    }
    
    // Simple HTTP HEAD request to check availability
    curl_easy_setopt(impl_->curl, CURLOPT_URL, config_.url.c_str());
    curl_easy_setopt(impl_->curl, CURLOPT_NOBODY, 1L);
    curl_easy_setopt(impl_->curl, CURLOPT_TIMEOUT, 5);
    
    CURLcode res = curl_easy_perform(impl_->curl);
    
    return (res == CURLE_OK);
#else
    return true;
#endif
}

std::string TimestampAuthority::getLastError() const {
    return last_error_;
}

// ============================================================================
// Private helper methods
// ============================================================================

std::vector<uint8_t> TimestampAuthority::createTSPRequest(
    const std::vector<uint8_t>& hash,
    const std::vector<uint8_t>& nonce) {
#ifdef THEMIS_ENABLE_TSA
    // Create TS_REQ using OpenSSL
    TS_REQ* req = TS_REQ_new();
    if (!req) {
        THEMIS_ERROR("Failed to create TS_REQ");
        return {};
    }
    
    // Set version (1)
    TS_REQ_set_version(req, 1);
    
    // Create message imprint
    TS_MSG_IMPRINT* msg_imprint = TS_MSG_IMPRINT_new();
    
    // Set hash algorithm
    X509_ALGOR* algo = X509_ALGOR_new();
    int nid = NID_sha256;  // Default
    if (config_.hash_algorithm == "SHA384") {
        nid = NID_sha384;
    } else if (config_.hash_algorithm == "SHA512") {
        nid = NID_sha512;
    }
    X509_ALGOR_set0(algo, OBJ_nid2obj(nid), V_ASN1_NULL, nullptr);
    TS_MSG_IMPRINT_set_algo(msg_imprint, algo);
    
    // Set hash value
    TS_MSG_IMPRINT_set_msg(
        msg_imprint,
        const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(hash.data())),
        static_cast<int>(hash.size())
    );
    
    TS_REQ_set_msg_imprint(req, msg_imprint);
    
    // Set nonce (optional, for replay protection)
    if (!nonce.empty()) {
        ASN1_INTEGER* nonce_asn1 = ASN1_INTEGER_new();
        BIGNUM* bn = BN_new();
        BN_bin2bn(nonce.data(), static_cast<int>(nonce.size()), bn);
        BN_to_ASN1_INTEGER(bn, nonce_asn1);
        TS_REQ_set_nonce(req, nonce_asn1);
        BN_free(bn);
    }
    
    // Request certificate
    if (config_.cert_req) {
        TS_REQ_set_cert_req(req, 1);
    }
    
    // Set policy OID (optional)
    if (!config_.policy_oid.empty()) {
        ASN1_OBJECT* policy = OBJ_txt2obj(config_.policy_oid.c_str(), 1);
        if (policy) {
            TS_REQ_set_policy_id(req, policy);
            ASN1_OBJECT_free(policy);
        }
    }
    
    // Convert to DER
    unsigned char* der = nullptr;
    int len = i2d_TS_REQ(req, &der);
    
    std::vector<uint8_t> result;
    if (len > 0 && der) {
        result.assign(der, der + len);
        OPENSSL_free(der);
    }
    
    TS_REQ_free(req);
    TS_MSG_IMPRINT_free(msg_imprint);
    X509_ALGOR_free(algo);
    
    return result;
#else
    (void)hash;
    (void)nonce;
    return {};
#endif
}

TimestampToken TimestampAuthority::parseTSPResponse(const std::vector<uint8_t>& response) {
#ifdef THEMIS_ENABLE_TSA
    TimestampToken token;
    
    // Parse TS_RESP
    const unsigned char* p = response.data();
    TS_RESP* resp = d2i_TS_RESP(nullptr, &p, static_cast<long>(response.size()));
    
    if (!resp) {
        token.error_message = "Failed to parse TS_RESP";
        THEMIS_ERROR("TSP response parsing failed");
        return token;
    }
    
    // Check PKI status
    TS_STATUS_INFO* status = TS_RESP_get_status_info(resp);
    const ASN1_INTEGER* status_asn1 = TS_STATUS_INFO_get0_status(status);
    long status_val = ASN1_INTEGER_get(status_asn1);
    
    token.pki_status = static_cast<int>(status_val);
    
    if (status_val != 0 && status_val != 1) {  // 0 = granted, 1 = granted with mods
        token.error_message = "TSA rejected request (status=" + std::to_string(status_val) + ")";
        TS_RESP_free(resp);
        return token;
    }
    
    // Get timestamp token
    PKCS7* pkcs7 = TS_RESP_get_token(resp);
    if (!pkcs7) {
        token.error_message = "No timestamp token in response";
        TS_RESP_free(resp);
        return token;
    }
    
    // Store token in DER format
    unsigned char* der = nullptr;
    int der_len = i2d_PKCS7(pkcs7, &der);
    if (der_len > 0 && der) {
        token.token_der.assign(der, der + der_len);
        token.token_b64 = base64_encode(token.token_der);
        OPENSSL_free(der);
    }
    
    // Get TST_INFO
    TS_TST_INFO* tst_info = PKCS7_to_TS_TST_INFO(pkcs7);
    if (tst_info) {
        // Get timestamp
        const ASN1_GENERALIZEDTIME* gen_time = TS_TST_INFO_get_time(tst_info);
        if (gen_time) {
            char time_str[32];
            snprintf(time_str, sizeof(time_str), "%s", reinterpret_cast<const char*>(gen_time->data));
            token.timestamp_utc = time_str;

            // Convert to Unix timestamp
            std::tm tm = {};
#if defined(_WIN32)
            std::istringstream ss(time_str);
            ss >> std::get_time(&tm, "%Y%m%d%H%M%SZ");
            time_t t = _mkgmtime(&tm);
            token.timestamp_unix_ms = t >= 0 ? static_cast<uint64_t>(t) * 1000ULL : 0ULL;
#else
            strptime(time_str, "%Y%m%d%H%M%SZ", &tm);
            token.timestamp_unix_ms = static_cast<uint64_t>(timegm(&tm)) * 1000ULL;
#endif
        }
        
        // Get serial number
        const ASN1_INTEGER* serial = TS_TST_INFO_get_serial(tst_info);
        if (serial) {
            BIGNUM* bn = ASN1_INTEGER_to_BN(serial, nullptr);
            char* hex_str = BN_bn2hex(bn);
            token.serial_number = hex_str;
            OPENSSL_free(hex_str);
            BN_free(bn);
        }
        
        // Get policy OID
        ASN1_OBJECT* policy = TS_TST_INFO_get_policy_id(tst_info);
        if (policy) {
            char oid_buf[128];
            OBJ_obj2txt(oid_buf, static_cast<int>(sizeof(oid_buf)), policy, 1);
            token.policy_oid = oid_buf;
        }
        
        TS_TST_INFO_free(tst_info);
    }
    
    token.success = true;
    token.verified = false;  // Needs separate verification
    
    TS_RESP_free(resp);
    
    THEMIS_INFO("Timestamp token received: serial={}, time={}", 
                token.serial_number, token.timestamp_utc);
    
    return token;
#else
    (void)response;
    TimestampToken t;
    t.success = true;
    return t;
#endif
}

std::vector<uint8_t> TimestampAuthority::sendTSPRequest(const std::vector<uint8_t>& request) {
#ifdef THEMIS_ENABLE_TSA
    if (!impl_->curl) {
        last_error_ = "CURL not initialized";
        return {};
    }
    
    std::vector<uint8_t> response;
    
    // Set URL
    curl_easy_setopt(impl_->curl, CURLOPT_URL, config_.url.c_str());
    
    // Set POST data
    curl_easy_setopt(impl_->curl, CURLOPT_POST, 1L);
    curl_easy_setopt(impl_->curl, CURLOPT_POSTFIELDS, request.data());
    curl_easy_setopt(impl_->curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(request.size()));
    
    // Set headers
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/timestamp-query");
    curl_easy_setopt(impl_->curl, CURLOPT_HTTPHEADER, headers);
    
    // Set write callback
    curl_easy_setopt(impl_->curl, CURLOPT_WRITEFUNCTION, Impl::write_callback);
    curl_easy_setopt(impl_->curl, CURLOPT_WRITEDATA, &response);
    
    // Set timeout
    curl_easy_setopt(impl_->curl, CURLOPT_TIMEOUT, config_.timeout_seconds);
    
    // SSL verification
    if (config_.verify_tsa_cert) {
        curl_easy_setopt(impl_->curl, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(impl_->curl, CURLOPT_SSL_VERIFYHOST, 2L);
        
        if (!config_.ca_cert_path.empty()) {
            curl_easy_setopt(impl_->curl, CURLOPT_CAINFO, config_.ca_cert_path.c_str());
        }
    } else {
        curl_easy_setopt(impl_->curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(impl_->curl, CURLOPT_SSL_VERIFYHOST, 0L);
    }
    
    // Authentication (if required)
    if (!config_.username.empty()) {
        std::string userpwd = config_.username + ":" + config_.password;
        curl_easy_setopt(impl_->curl, CURLOPT_USERPWD, userpwd.c_str());
    }
    
    // mTLS (if configured)
    if (!config_.client_cert_path.empty()) {
        curl_easy_setopt(impl_->curl, CURLOPT_SSLCERT, config_.client_cert_path.c_str());
    }
    if (!config_.client_key_path.empty()) {
        curl_easy_setopt(impl_->curl, CURLOPT_SSLKEY, config_.client_key_path.c_str());
    }
    
    // Perform request
    CURLcode res = curl_easy_perform(impl_->curl);
    
    // Cleanup headers
    curl_slist_free_all(headers);
    
    if (res != CURLE_OK) {
        last_error_ = std::string("CURL request failed: ") + curl_easy_strerror(res);
        THEMIS_ERROR("TSA request failed: {}", last_error_);
        return {};
    }
    
    // Check HTTP status
    long http_code = 0;
    curl_easy_getinfo(impl_->curl, CURLINFO_RESPONSE_CODE, &http_code);
    
    if (http_code != 200) {
        last_error_ = "TSA returned HTTP " + std::to_string(http_code);
        THEMIS_ERROR("TSA HTTP error: {}", last_error_);
        return {};
    }
    
    return response;
#else
    (void)request;
    return {};
#endif
}

std::vector<uint8_t> TimestampAuthority::generateNonce(size_t bytes) {
#ifdef THEMIS_ENABLE_TSA
    std::vector<uint8_t> nonce(bytes);
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<int> dis(0, 255);

    for (size_t i = 0; i < bytes; ++i) {
        nonce[i] = static_cast<uint8_t>(dis(gen));
    }

    return nonce;
#else
    // Deterministic nonce for stub
    std::vector<uint8_t> n(bytes);
    for (size_t i = 0; i < bytes; ++i) {
        n[i] = static_cast<uint8_t>(i);
    }
    return n;
#endif
}

std::vector<uint8_t> TimestampAuthority::computeHash(const std::vector<uint8_t>& data) {
#ifdef THEMIS_ENABLE_TSA
    std::vector<uint8_t> hash(SHA256_DIGEST_LENGTH);
    SHA256(data.data(), data.size(), hash.data());
    return hash;
#else
    return pseudo_hash(data);
#endif
}

// ============================================================================
// eIDASTimestampValidator implementation
// ============================================================================

bool eIDASTimestampValidator::validateeIDASTimestamp(
    const TimestampToken& token,
    const std::vector<std::string>& trust_anchors) {
    (void)trust_anchors;
    
    validation_errors_.clear();
    
    if (!token.success) {
        validation_errors_.push_back("Token not successfully obtained");
        return false;
    }
    
    if (token.token_der.empty()) {
        validation_errors_.push_back("Empty token data");
        return false;
    }
    
#ifdef THEMIS_ENABLE_TSA
    // Stub implementation - in production:
    // 1. Verify timestamp signature
    // 2. Validate TSA certificate chain
    // 3. Check certificate revocation (CRL/OCSP)
    // 4. Verify TSA is qualified (QTSP)
    
    THEMIS_WARN("eIDAS timestamp validation not fully implemented");
#endif
    
    return validation_errors_.empty();
}

bool eIDASTimestampValidator::validateAge(const TimestampToken& token, 
                                          int max_age_days) {
    if (token.timestamp_unix_ms == 0) {
        return false;
    }
    
    uint64_t now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    
    uint64_t age_ms = now_ms - token.timestamp_unix_ms;
    uint64_t age_days = age_ms / (1000 * 60 * 60 * 24);
    
    return age_days <= static_cast<uint64_t>(max_age_days);
}

bool eIDASTimestampValidator::isQualifiedTSA(
    const std::string& tsa_cert,
    const std::vector<std::string>& qtsp_list) {
    (void)tsa_cert;
    (void)qtsp_list;
    
    // Stub implementation - in production, parse certificate and check against QTSP list
    THEMIS_WARN("isQualifiedTSA() not implemented");
    return false;
}

std::vector<std::string> eIDASTimestampValidator::getValidationErrors() const {
    return validation_errors_;
}

} } // namespace themis::security
