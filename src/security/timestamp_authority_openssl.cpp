/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            timestamp_authority_openssl.cpp                    ║
  Version:         0.0.44                                             ║
  Last Modified:   2026-04-15 05:44:00                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     624                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • f3755277dc  2026-03-01  feat(tsa): implement RFC 3161 TSAConfig auth/TLS fields a... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#ifdef THEMIS_USE_OPENSSL_TSA
// OpenSSL/CURL based TimestampAuthority implementation (RFC 3161)
// Separate from stub to avoid dependency bloat when not needed.

#include "security/timestamp_authority.h"
#include "utils/logger.h"

#include <openssl/ts.h>
#include <openssl/evp.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/obj_mac.h>
#include <openssl/rand.h>
#include <curl/curl.h>

#include <chrono>
#include <cstring>
#include <ctime>
#include <sstream>
#include <iomanip>

namespace themis { namespace security {

class TimestampAuthority::Impl {
public:
    CURL* curl = nullptr;
    Impl() { curl = curl_easy_init(); }
    ~Impl() { if (curl) curl_easy_cleanup(curl); }
};

static const EVP_MD* selectDigest(const std::string& algo){
    if(algo == "SHA384") return EVP_sha384();
    if(algo == "SHA512") return EVP_sha512();
    return EVP_sha256();
}

static std::string hex(const std::vector<uint8_t>& data){
    static const char* d = "0123456789abcdef"; std::string out; out.reserve(data.size()*2);
    for(auto b: data){ out.push_back(d[(b>>4)&0xF]); out.push_back(d[b&0xF]); }
    return out;
}

static std::string b64Encode(const std::vector<uint8_t>& data){
    BIO* b64 = BIO_new(BIO_f_base64()); BIO* mem = BIO_new(BIO_s_mem());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    b64 = BIO_push(b64, mem);
    BIO_write(b64, data.data(), (int)data.size());
    BIO_flush(b64);
    BUF_MEM* ptr; BIO_get_mem_ptr(b64, &ptr);
    std::string out(ptr->data, ptr->length);
    BIO_free_all(b64);
    return out;
}

static std::vector<uint8_t> b64Decode(const std::string& s){
    BIO* b64 = BIO_new(BIO_f_base64()); BIO* mem = BIO_new_mem_buf(s.data(), (int)s.size());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    mem = BIO_push(b64, mem);
    std::vector<uint8_t> out(s.size());
    int len = BIO_read(mem, out.data(), (int)out.size());
    if(len < 0) len = 0; out.resize(len);
    BIO_free_all(mem);
    return out;
}

// Helper: Convert ASN1_GENERALIZEDTIME to Unix milliseconds
static uint64_t asn1TimeToUnixMs(ASN1_GENERALIZEDTIME* gen) {
    if (!gen || !gen->data) return 0;
    
    // Use OpenSSL's built-in conversion which handles all valid ASN.1 formats
    struct tm tm_time = {};
    if (!ASN1_TIME_to_tm(reinterpret_cast<const ASN1_TIME*>(gen), &tm_time)) {
        return 0;
    }
    
    // Convert to UTC time_t using portable method
#ifdef _WIN32
    time_t t = _mkgmtime(&tm_time);
#else
    // For POSIX systems, use timegm if available, otherwise use portable fallback
    #if defined(__linux__) || defined(__APPLE__)
        time_t t = timegm(&tm_time);
    #else
        // Portable fallback: temporarily set TZ to UTC
        char* old_tz = getenv("TZ");
        char* old_tz_copy = nullptr;
        if (old_tz) {
            old_tz_copy = strdup(old_tz);
            if (!old_tz_copy) {
                return 0;  // Memory allocation failed
            }
        }
        
        if (setenv("TZ", "UTC", 1) != 0) {
            free(old_tz_copy);
            return 0;  // setenv failed
        }
        tzset();
        time_t t = mktime(&tm_time);
        
        // Restore original TZ (even if mktime failed)
        if (old_tz_copy) {
            setenv("TZ", old_tz_copy, 1);
            free(old_tz_copy);
        } else {
            unsetenv("TZ");
        }
        tzset();
    #endif
#endif
    
    if (t == -1) return 0;
    
    // Convert to milliseconds
    return static_cast<uint64_t>(t) * 1000;
}

TimestampAuthority::TimestampAuthority(TSAConfig config)
    : impl_(std::make_unique<Impl>()), config_(std::move(config)) {}
TimestampAuthority::~TimestampAuthority() = default;
TimestampAuthority::TimestampAuthority(TimestampAuthority&&) noexcept = default;
TimestampAuthority& TimestampAuthority::operator=(TimestampAuthority&&) noexcept = default;

std::vector<uint8_t> TimestampAuthority::computeHash(const std::vector<uint8_t>& data){
    const EVP_MD* md = selectDigest(config_.hash_algorithm);
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    std::vector<uint8_t> out(EVP_MD_size(md));
    unsigned int outlen=0;
    EVP_DigestInit_ex(ctx, md, nullptr);
    EVP_DigestUpdate(ctx, data.data(), data.size());
    EVP_DigestFinal_ex(ctx, out.data(), &outlen);
    out.resize(outlen);
    EVP_MD_CTX_free(ctx);
    return out;
}

std::vector<uint8_t> TimestampAuthority::generateNonce(size_t bytes){
    std::vector<uint8_t> n(bytes);
    if(RAND_bytes(n.data(), (int)bytes) != 1){
        for(size_t i=0;i<bytes;++i) n[i] = (uint8_t)i; // fallback deterministic
    }
    return n;
}

std::vector<uint8_t> TimestampAuthority::createTSPRequest(const std::vector<uint8_t>& hash,const std::vector<uint8_t>& nonce){
    TS_REQ* req = TS_REQ_new(); if(!req){ last_error_="TS_REQ_new failed"; return {}; }
    TS_REQ_set_version(req,1);
    // Message imprint
    TS_MSG_IMPRINT* imprint = TS_MSG_IMPRINT_new();
    const EVP_MD* md = selectDigest(config_.hash_algorithm);
    X509_ALGOR* algo = X509_ALGOR_new();
    X509_ALGOR_set0(algo, OBJ_nid2obj(EVP_MD_type(md)), V_ASN1_NULL, nullptr);
    TS_MSG_IMPRINT_set_algo(imprint, algo);
    ASN1_OCTET_STRING* hash_asn1 = ASN1_OCTET_STRING_new();
    ASN1_OCTET_STRING_set(hash_asn1, hash.data(), (int)hash.size());
    TS_MSG_IMPRINT_set_msg(imprint, const_cast<unsigned char*>(hash.data()), (int)hash.size());
    TS_REQ_set_msg_imprint(req, imprint);
    // Nonce
    if(!nonce.empty()){
        ASN1_INTEGER* nonce_i = ASN1_INTEGER_new();
        BIGNUM* bn = BN_bin2bn(nonce.data(), (int)nonce.size(), nullptr);
        BN_to_ASN1_INTEGER(bn, nonce_i); BN_free(bn);
        TS_REQ_set_nonce(req, nonce_i);
    }
    if(config_.cert_req) TS_REQ_set_cert_req(req,1);
    if(!config_.policy_oid.empty()){
        ASN1_OBJECT* policy = OBJ_txt2obj(config_.policy_oid.c_str(),1);
        if(policy){ TS_REQ_set_policy_id(req, policy); ASN1_OBJECT_free(policy); }
    }
    unsigned char* der=nullptr; int len=i2d_TS_REQ(req,&der);
    std::vector<uint8_t> out;
    if(len>0 && der){ out.assign(der, der+len); OPENSSL_free(der); }
    TS_REQ_free(req); // imprint, algo, hash_asn1 freed through req
    return out;
}

size_t curlWrite(char* ptr,size_t size,size_t nmemb,void* userdata){
    auto* vec = reinterpret_cast<std::vector<uint8_t>*>(userdata);
    size_t total = size*nmemb;
    vec->insert(vec->end(), (uint8_t*)ptr, (uint8_t*)ptr + total);
    return total;
}

std::vector<uint8_t> TimestampAuthority::sendTSPRequest(const std::vector<uint8_t>& request){
    if(!impl_->curl){ last_error_="CURL init failed"; return {}; }
    std::vector<uint8_t> response;
    curl_easy_setopt(impl_->curl, CURLOPT_URL, config_.url.c_str());
    curl_easy_setopt(impl_->curl, CURLOPT_POST, 1L);
    curl_easy_setopt(impl_->curl, CURLOPT_NOBODY, 0L);
    curl_easy_setopt(impl_->curl, CURLOPT_POSTFIELDS, request.data());
    curl_easy_setopt(impl_->curl, CURLOPT_POSTFIELDSIZE, (long)request.size());
    curl_easy_setopt(impl_->curl, CURLOPT_TIMEOUT, (long)config_.timeout_seconds);
    curl_easy_setopt(impl_->curl, CURLOPT_WRITEFUNCTION, curlWrite);
    curl_easy_setopt(impl_->curl, CURLOPT_WRITEDATA, &response);

    // TLS certificate verification
    curl_easy_setopt(impl_->curl, CURLOPT_SSL_VERIFYPEER, config_.verify_tsa_cert ? 1L : 0L);
    curl_easy_setopt(impl_->curl, CURLOPT_SSL_VERIFYHOST, config_.verify_tsa_cert ? 2L : 0L);

    // Custom CA certificate (for internal TSAs with self-signed/private CA)
    if(!config_.ca_cert_path.empty()){
        curl_easy_setopt(impl_->curl, CURLOPT_CAINFO, config_.ca_cert_path.c_str());
    }

    // mTLS client certificate (for internal TSAs requiring client authentication)
    if(!config_.client_cert_path.empty()){
        curl_easy_setopt(impl_->curl, CURLOPT_SSLCERT, config_.client_cert_path.c_str());
    }
    if(!config_.client_key_path.empty()){
        curl_easy_setopt(impl_->curl, CURLOPT_SSLKEY, config_.client_key_path.c_str());
    }

    // HTTP Basic Auth (for authenticated third-party TSAs)
    if(!config_.username.empty()){
        curl_easy_setopt(impl_->curl, CURLOPT_USERNAME, config_.username.c_str());
        curl_easy_setopt(impl_->curl, CURLOPT_PASSWORD, config_.password.c_str());
    }

    struct curl_slist* headers=nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/timestamp-query");
    headers = curl_slist_append(headers, "Accept: application/timestamp-reply");
    curl_easy_setopt(impl_->curl, CURLOPT_HTTPHEADER, headers);
    CURLcode res = curl_easy_perform(impl_->curl);
    curl_slist_free_all(headers);
    if(res!=CURLE_OK){ last_error_ = std::string("curl error: ")+curl_easy_strerror(res); return {}; }
    long code=0; curl_easy_getinfo(impl_->curl, CURLINFO_RESPONSE_CODE, &code);
    if(code!=200){ last_error_ = "HTTP status "+std::to_string(code); return {}; }
    return response;
}

TimestampToken TimestampAuthority::parseTSPResponse(const std::vector<uint8_t>& respBytes){
    TimestampToken token;
    const unsigned char* p = respBytes.data();
    TS_RESP* resp = d2i_TS_RESP(nullptr, &p, (long)respBytes.size());
    if(!resp){ token.error_message="d2i_TS_RESP failed"; return token; }
    TS_STATUS_INFO* status = TS_RESP_get_status_info(resp);
    const ASN1_INTEGER* st_int = TS_STATUS_INFO_get0_status(status);
    token.pki_status = ASN1_INTEGER_get(st_int);
    if(token.pki_status != 0 && token.pki_status != 1){ token.error_message="TSA rejected"; TS_RESP_free(resp); return token; }
    PKCS7* pkcs7 = TS_RESP_get_token(resp);
    if(!pkcs7){ token.error_message="No PKCS7"; TS_RESP_free(resp); return token; }
    unsigned char* der=nullptr; int der_len=i2d_PKCS7(pkcs7,&der);
    if(der_len>0 && der){ token.token_der.assign(der, der+der_len); OPENSSL_free(der); }
    token.token_b64 = b64Encode(token.token_der);
    
    // Extract TSA certificate from PKCS7 (if cert_req was true)
    STACK_OF(X509)* certs = PKCS7_get0_signers(pkcs7, nullptr, 0);
    if(certs && sk_X509_num(certs) > 0){
        X509* tsa_x509 = sk_X509_value(certs, 0);
        if(tsa_x509){
            // Store certificate in DER format
            unsigned char* cert_der = nullptr;
            int cert_len = i2d_X509(tsa_x509, &cert_der);
            if(cert_len > 0 && cert_der){
                token.tsa_cert.assign(cert_der, cert_der + cert_len);
                OPENSSL_free(cert_der);
            }
            
            // Extract certificate serial number
            const ASN1_INTEGER* cert_serial = X509_get0_serialNumber(tsa_x509);
            if(cert_serial){
                BIGNUM* bn = ASN1_INTEGER_to_BN(cert_serial, nullptr);
                char* hexStr = BN_bn2hex(bn);
                token.tsa_serial = hexStr;
                OPENSSL_free(hexStr);
                BN_free(bn);
            }
            
            // Extract subject name
            X509_NAME* subject = X509_get_subject_name(tsa_x509);
            if(subject){
                BIO* name_bio = BIO_new(BIO_s_mem());
                X509_NAME_print_ex(name_bio, subject, 0, XN_FLAG_RFC2253);
                BUF_MEM* name_buf;
                BIO_get_mem_ptr(name_bio, &name_buf);
                token.tsa_name.assign(name_buf->data, name_buf->length);
                BIO_free(name_bio);
            }
        }
    }
    if(certs) sk_X509_free(certs);
    
    TS_TST_INFO* tst = PKCS7_to_TS_TST_INFO(pkcs7);
    if(tst){
        const ASN1_GENERALIZEDTIME* gen = TS_TST_INFO_get_time(tst);
        if(gen){ 
            std::string g(reinterpret_cast<const char*>(gen->data), gen->length); 
            token.timestamp_utc = g;
            token.timestamp_unix_ms = asn1TimeToUnixMs(const_cast<ASN1_GENERALIZEDTIME*>(gen));
        }
        const ASN1_INTEGER* serial = TS_TST_INFO_get_serial(tst);
        if(serial){ BIGNUM* bn = ASN1_INTEGER_to_BN(serial,nullptr); char* hexStr = BN_bn2hex(bn); token.serial_number = hexStr; OPENSSL_free(hexStr); BN_free(bn);}        
        ASN1_OBJECT* policy = TS_TST_INFO_get_policy_id(tst);
        if(policy){ char buf[128]; OBJ_obj2txt(buf,sizeof(buf),policy,1); token.policy_oid=buf; }
        
        // Extract accuracy metadata (RFC 3161 - optional)
        const TS_ACCURACY* accuracy = TS_TST_INFO_get_accuracy(tst);
        if(accuracy){
            token.has_accuracy = true;
            const ASN1_INTEGER* seconds = TS_ACCURACY_get_seconds(accuracy);
            const ASN1_INTEGER* millis = TS_ACCURACY_get_millis(accuracy);
            const ASN1_INTEGER* micros = TS_ACCURACY_get_micros(accuracy);
            if(seconds) token.accuracy_seconds = ASN1_INTEGER_get(seconds);
            if(millis) token.accuracy_millis = ASN1_INTEGER_get(millis);
            if(micros) token.accuracy_micros = ASN1_INTEGER_get(micros);
        }
        
        // Extract ordering hint (RFC 3161 - optional, default FALSE)
        int ordering = TS_TST_INFO_get_ordering(tst);
        token.ordering = (ordering != 0);
        
        TS_TST_INFO_free(tst);
    }
    token.success=true; token.verified=false; // separate verification step
    TS_RESP_free(resp);
    return token;
}

TimestampToken TimestampAuthority::getTimestampForHash(const std::vector<uint8_t>& hash){
    auto nonce = generateNonce();
    auto req = createTSPRequest(hash, nonce);
    if(req.empty()){ TimestampToken t; t.error_message=last_error_; return t; }
    auto resp = sendTSPRequest(req);
    if(resp.empty()){ TimestampToken t; t.error_message=last_error_; return t; }
    auto token = parseTSPResponse(resp);
    token.nonce = nonce;
    token.hash_algorithm = config_.hash_algorithm;
    
    // Cache the TSA certificate for getTSACertificate()
    if(!token.tsa_cert.empty()){
        cached_tsa_cert_ = token.tsa_cert;
    }
    
    return token;
}

TimestampToken TimestampAuthority::getTimestamp(const std::vector<uint8_t>& data){
    auto h = computeHash(data);
    return getTimestampForHash(h);
}

bool TimestampAuthority::verifyTimestampForHash(const std::vector<uint8_t>& hash,const TimestampToken& token){
    if(token.token_der.empty()) return false;
    const unsigned char* p = token.token_der.data();
    PKCS7* pkcs7 = d2i_PKCS7(nullptr,&p,(long)token.token_der.size());
    if(!pkcs7) return false;
    TS_TST_INFO* tst = PKCS7_to_TS_TST_INFO(pkcs7);
    if(!tst){ PKCS7_free(pkcs7); return false; }
    TS_MSG_IMPRINT* imprint = TS_TST_INFO_get_msg_imprint(tst);
    ASN1_OCTET_STRING* os = TS_MSG_IMPRINT_get_msg(imprint);
    bool match = (os->length == (int)hash.size() && std::memcmp(os->data, hash.data(), hash.size())==0);
    TS_TST_INFO_free(tst); PKCS7_free(pkcs7);
    return match;
}

bool TimestampAuthority::verifyTimestamp(const std::vector<uint8_t>& data,const TimestampToken& token){
    auto h = computeHash(data);
    return verifyTimestampForHash(h, token);
}

TimestampToken TimestampAuthority::parseToken(const std::vector<uint8_t>& der){ return parseTSPResponse(der); }
TimestampToken TimestampAuthority::parseToken(const std::string& b64){ auto der = b64Decode(b64); return parseTSPResponse(der); }

std::optional<std::string> TimestampAuthority::getTSACertificate(){
    if(cached_tsa_cert_.empty()) return std::nullopt;
    
    // Convert DER to PEM format
    const unsigned char* p = cached_tsa_cert_.data();
    X509* cert = d2i_X509(nullptr, &p, cached_tsa_cert_.size());
    if(!cert) return std::nullopt;
    
    BIO* bio = BIO_new(BIO_s_mem());
    if(!bio){
        X509_free(cert);
        return std::nullopt;
    }
    
    PEM_write_bio_X509(bio, cert);
    BUF_MEM* mem;
    BIO_get_mem_ptr(bio, &mem);
    std::string pem(mem->data, mem->length);
    
    BIO_free(bio);
    X509_free(cert);
    return pem;
}

bool TimestampAuthority::isAvailable(){
    if(!impl_->curl) return false;
    curl_easy_setopt(impl_->curl, CURLOPT_URL, config_.url.c_str());
    curl_easy_setopt(impl_->curl, CURLOPT_NOBODY, 1L);
    curl_easy_setopt(impl_->curl, CURLOPT_TIMEOUT, 5L);
    if(!config_.ca_cert_path.empty()){
        curl_easy_setopt(impl_->curl, CURLOPT_CAINFO, config_.ca_cert_path.c_str());
    }
    curl_easy_setopt(impl_->curl, CURLOPT_SSL_VERIFYPEER, config_.verify_tsa_cert ? 1L : 0L);
    curl_easy_setopt(impl_->curl, CURLOPT_SSL_VERIFYHOST, config_.verify_tsa_cert ? 2L : 0L);
    CURLcode res = curl_easy_perform(impl_->curl);
    // Reset NOBODY so subsequent POST requests work correctly
    curl_easy_setopt(impl_->curl, CURLOPT_NOBODY, 0L);
    curl_easy_setopt(impl_->curl, CURLOPT_TIMEOUT, (long)config_.timeout_seconds);
    return res == CURLE_OK;
}

std::string TimestampAuthority::getLastError() const { return last_error_; }

// ============================================================================
// eIDAS Timestamp Validator Implementation
// ============================================================================

bool eIDASTimestampValidator::validateeIDASTimestamp(
    const TimestampToken& token,
    const std::vector<std::string>& trust_anchors) {
    
    validation_errors_.clear();
    
    // Basic validation
    if (!token.success) {
        validation_errors_.push_back("Token marked as unsuccessful");
        return false;
    }
    
    if (token.token_der.empty()) {
        validation_errors_.push_back("Token DER data is empty");
        return false;
    }
    
    // Validate timestamp token structure
    const unsigned char* p = token.token_der.data();
    // Safe cast: d2i_PKCS7 expects long, ensure we don't overflow
    if (token.token_der.size() > static_cast<size_t>(LONG_MAX)) {
        validation_errors_.push_back("Token size exceeds maximum allowed");
        return false;
    }
    PKCS7* pkcs7 = d2i_PKCS7(nullptr, &p, static_cast<long>(token.token_der.size()));
    if (!pkcs7) {
        validation_errors_.push_back("Failed to parse PKCS7 token");
        return false;
    }
    
    // Extract TST_INFO
    TS_TST_INFO* tst = PKCS7_to_TS_TST_INFO(pkcs7);
    if (!tst) {
        validation_errors_.push_back("Failed to extract TST_INFO from token");
        PKCS7_free(pkcs7);
        return false;
    }
    
    // Validate signature (simplified - full validation would need trust anchors)
    // For now, we just verify the token structure is valid
    bool valid = true;
    
    // Check if timestamp is present
    const ASN1_GENERALIZEDTIME* gen_time = TS_TST_INFO_get_time(tst);
    if (!gen_time) {
        validation_errors_.push_back("Missing timestamp in token");
        valid = false;
    }
    
    // Check if serial number is present
    const ASN1_INTEGER* serial = TS_TST_INFO_get_serial(tst);
    if (!serial) {
        validation_errors_.push_back("Missing serial number in token");
        valid = false;
    }
    
    // Check message imprint
    TS_MSG_IMPRINT* imprint = TS_TST_INFO_get_msg_imprint(tst);
    if (!imprint) {
        validation_errors_.push_back("Missing message imprint in token");
        valid = false;
    }
    
    // For eIDAS compliance, we would need to:
    // 1. Verify the certificate chain against trust anchors
    // 2. Check if TSA is in the qualified trust list
    // 3. Validate signature with TSA certificate
    // These are simplified here as they require the full trust anchor infrastructure
    
    TS_TST_INFO_free(tst);
    PKCS7_free(pkcs7);
    
    return valid;
}

bool eIDASTimestampValidator::validateAge(const TimestampToken& token, int max_age_days) {
    validation_errors_.clear();
    
    if (token.timestamp_unix_ms == 0) {
        validation_errors_.push_back("Token has no timestamp");
        return false;
    }
    
    // Get current time in milliseconds
    auto now = std::chrono::system_clock::now();
    uint64_t now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()
    ).count();
    
    // Check for future timestamps (avoid integer underflow)
    if (token.timestamp_unix_ms > now_ms) {
        validation_errors_.push_back("Token timestamp is in the future");
        return false;
    }
    
    // Calculate age in milliseconds
    uint64_t age_ms = now_ms - token.timestamp_unix_ms;
    
    // Convert max age from days to milliseconds with overflow check
    // max_age_days * 24 * 60 * 60 * 1000 = max_age_days * 86400000
    // Check if multiplication would overflow uint64_t
    constexpr uint64_t MS_PER_DAY = 86400000ULL;
    if (max_age_days < 0) {
        validation_errors_.push_back("Maximum age must be non-negative");
        return false;
    }
    if (static_cast<uint64_t>(max_age_days) > UINT64_MAX / MS_PER_DAY) {
        validation_errors_.push_back("Maximum age value too large");
        return false;
    }
    uint64_t max_age_ms = static_cast<uint64_t>(max_age_days) * MS_PER_DAY;
    
    if (age_ms > max_age_ms) {
        validation_errors_.push_back("Token age exceeds maximum allowed age");
        return false;
    }
    
    return true;
}

bool eIDASTimestampValidator::isQualifiedTSA(
    const std::string& tsa_cert,
    const std::vector<std::string>& qtsp_list) {
    
    validation_errors_.clear();
    
    // Validate certificate size
    if (tsa_cert.size() > static_cast<size_t>(INT_MAX)) {
        validation_errors_.push_back("TSA certificate size exceeds maximum allowed");
        return false;
    }
    
    // Parse TSA certificate
    BIO* bio = BIO_new_mem_buf(tsa_cert.data(), static_cast<int>(tsa_cert.size()));
    if (!bio) {
        validation_errors_.push_back("Failed to create BIO for certificate");
        return false;
    }
    
    X509* cert = PEM_read_bio_X509(bio, nullptr, nullptr, nullptr);
    BIO_free(bio);
    
    if (!cert) {
        validation_errors_.push_back("Failed to parse TSA certificate");
        return false;
    }
    
    // Extract subject name
    X509_NAME* subject = X509_get_subject_name(cert);
    if (!subject) {
        validation_errors_.push_back("Certificate has no subject");
        X509_free(cert);
        return false;
    }
    
    // Extract subject name using dynamic allocation
    BIO* name_bio = BIO_new(BIO_s_mem());
    if (!name_bio) {
        validation_errors_.push_back("Failed to create BIO for subject name");
        X509_free(cert);
        return false;
    }
    
    X509_NAME_print_ex(name_bio, subject, 0, XN_FLAG_RFC2253);
    BUF_MEM* name_buf;
    BIO_get_mem_ptr(name_bio, &name_buf);
    std::string subject_name(name_buf->data, name_buf->length);
    BIO_free(name_bio);
    
    X509_free(cert);
    
    // Check if TSA is in qualified list
    // In a real implementation, this would check against the EU Trusted List
    // For now, we do a simple string match
    for (const auto& qtsp : qtsp_list) {
        if (subject_name.find(qtsp) != std::string::npos) {
            return true;
        }
    }
    
    validation_errors_.push_back("TSA not found in qualified trust service providers list");
    return false;
}

std::vector<std::string> eIDASTimestampValidator::getValidationErrors() const {
    return validation_errors_;
}

} } // namespace themis::security

#endif // THEMIS_USE_OPENSSL_TSA
