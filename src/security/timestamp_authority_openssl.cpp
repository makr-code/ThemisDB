/**
 * @file timestamp_authority_openssl.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=8; TODO=1, Stub=5, Unimpl=0, Mock=1, Sim=1, Debt=0, C=0, H=2, M=12, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// STUB/SIMULATION NOTE:
// Purpose: Allow the security module to be built without OpenSSL TSA or libcurl.
//   When `THEMIS_USE_OPENSSL_TSA` is not defined, this entire translation unit
//   is excluded from compilation.  Any RFC 3161 timestamp request (sign, verify,
//   eIDAS qualified validation) is expected to be handled by an alternative stub
//   implementation in `security/timestamp_authority.cpp` (or be unavailable).
// Activation: `THEMIS_USE_OPENSSL_TSA` not defined at compile time (default for
//   builds without libcurl and OpenSSL TS headers, or without
//   `-DTHEMIS_USE_OPENSSL_TSA=1`).
// Production Delta: RFC 3161 timestamp stamping and verification are unavailable.
//   All `TimestampAuthority::stamp()` / `TimestampAuthority::verify()` calls will
//   fall through to stub returns (unsigned/unverifiable timestamps).  eIDAS
//   qualified timestamp validation (`eIDASTimestampValidator`) is also absent.
// Removal Plan: Install libcurl + OpenSSL with TS support (≥ OpenSSL 1.0.2)
//   and set `-DTHEMIS_USE_OPENSSL_TSA=1` in CMake.  Ensure `libcurl.h` and
//   `<openssl/ts.h>` are on the include path.
// Roadmap ref: src/security/FUTURE_ENHANCEMENTS.md §"OpenSSL TSA Activation"

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
#include <climits>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <ctime>
#include <algorithm>
#include <cctype>
#include <sstream>
#include <iomanip>
#include <memory>

namespace themis { namespace security {

class TimestampAuthority::Impl {
public:
    CURL* curl = nullptr;
    Impl() { curl = curl_easy_init(); }
    ~Impl() { if (curl) curl_easy_cleanup(curl); }
};

// ============================================================================
// RAII Wrappers for OpenSSL Objects (Automatic Cleanup on Scope Exit)
// ============================================================================

namespace {

// Custom deleters for unique_ptr
struct TS_RESP_Deleter { void operator()(TS_RESP* p) const { if (p) TS_RESP_free(p); } };
struct PKCS7_Deleter { void operator()(PKCS7* p) const { if (p) PKCS7_free(p); } };
struct X509_Deleter { void operator()(X509* p) const { if (p) X509_free(p); } };
struct STACK_OF_X509_Deleter { 
    void operator()(STACK_OF(X509)* p) const { if (p) sk_X509_pop_free(p, X509_free); } 
};
struct TSA_BIO_Deleter { void operator()(BIO* p) const { if (p) BIO_free_all(p); } };
struct TSA_BIGNUM_Deleter { void operator()(BIGNUM* p) const { if (p) BN_free(p); } };
struct TSA_TS_TST_INFO_Deleter { void operator()(TS_TST_INFO* p) const { if (p) TS_TST_INFO_free(p); } };
struct TSA_EVP_MD_CTX_Deleter { void operator()(EVP_MD_CTX* p) const { if (p) EVP_MD_CTX_free(p); } };
struct TSA_EVP_PKEY_Deleter { void operator()(EVP_PKEY* p) const { if (p) EVP_PKEY_free(p); } };
struct TSA_TS_REQ_Deleter { void operator()(TS_REQ* p) const { if (p) TS_REQ_free(p); } };
struct TSA_TS_MSG_IMPRINT_Deleter { void operator()(TS_MSG_IMPRINT* p) const { if (p) TS_MSG_IMPRINT_free(p); } };
struct TSA_X509_ALGOR_Deleter { void operator()(X509_ALGOR* p) const { if (p) X509_ALGOR_free(p); } };
struct TSA_ASN1_OCTET_STRING_Deleter { void operator()(ASN1_OCTET_STRING* p) const { if (p) ASN1_OCTET_STRING_free(p); } };
struct TSA_ASN1_INTEGER_Deleter { void operator()(ASN1_INTEGER* p) const { if (p) ASN1_INTEGER_free(p); } };
struct TSA_ASN1_OBJECT_Deleter { void operator()(ASN1_OBJECT* p) const { if (p) ASN1_OBJECT_free(p); } };
struct TSA_OPENSSL_Deleter { void operator()(unsigned char* p) const { if (p) OPENSSL_free(p); } };
struct TSA_OPENSSL_CStr_Deleter { void operator()(char* p) const { if (p) OPENSSL_free(p); } };
struct TSA_curl_slist_Deleter { void operator()(struct curl_slist* p) const { if (p) curl_slist_free_all(p); } };

// Type aliases for RAII-managed pointers
using TS_RESP_ptr        = std::unique_ptr<TS_RESP, TS_RESP_Deleter>;
using PKCS7_ptr          = std::unique_ptr<PKCS7, PKCS7_Deleter>;
using X509_ptr           = std::unique_ptr<X509, X509_Deleter>;
using STACK_OF_X509_ptr  = std::unique_ptr<STACK_OF(X509), STACK_OF_X509_Deleter>;
using TSA_BIO_ptr            = std::unique_ptr<BIO, TSA_BIO_Deleter>;
using TSA_BIGNUM_ptr         = std::unique_ptr<BIGNUM, TSA_BIGNUM_Deleter>;
using TSA_TS_TST_INFO_ptr    = std::unique_ptr<TS_TST_INFO, TSA_TS_TST_INFO_Deleter>;
using TSA_EVP_MD_CTX_ptr     = std::unique_ptr<EVP_MD_CTX, TSA_EVP_MD_CTX_Deleter>;
using TSA_EVP_PKEY_ptr       = std::unique_ptr<EVP_PKEY, TSA_EVP_PKEY_Deleter>;
using TSA_TS_REQ_ptr         = std::unique_ptr<TS_REQ, TSA_TS_REQ_Deleter>;
using TSA_TS_MSG_IMPRINT_ptr = std::unique_ptr<TS_MSG_IMPRINT, TSA_TS_MSG_IMPRINT_Deleter>;
using TSA_X509_ALGOR_ptr     = std::unique_ptr<X509_ALGOR, TSA_X509_ALGOR_Deleter>;
using TSA_ASN1_OCTET_STRING_ptr = std::unique_ptr<ASN1_OCTET_STRING, TSA_ASN1_OCTET_STRING_Deleter>;
using TSA_ASN1_INTEGER_ptr   = std::unique_ptr<ASN1_INTEGER, TSA_ASN1_INTEGER_Deleter>;
using TSA_ASN1_OBJECT_ptr    = std::unique_ptr<ASN1_OBJECT, TSA_ASN1_OBJECT_Deleter>;
using TSA_OPENSSL_Buffer_ptr = std::unique_ptr<unsigned char, TSA_OPENSSL_Deleter>;
using TSA_OPENSSL_CStr_ptr   = std::unique_ptr<char, TSA_OPENSSL_CStr_Deleter>;
using TSA_curl_slist_ptr     = std::unique_ptr<struct curl_slist, TSA_curl_slist_Deleter>;

} // namespace

namespace {

bool startsWithHttps(const std::string& url) {
    constexpr char kHttpsPrefix[] = "https://";
    constexpr std::size_t kPrefixLen = sizeof(kHttpsPrefix) - 1;
    if (url.size() < kPrefixLen) {
        return false;
    }
    return std::equal(
        kHttpsPrefix, kHttpsPrefix + kPrefixLen, url.begin(),
        [](char lhs, char rhs) {
            return std::tolower(static_cast<unsigned char>(lhs)) ==
                   std::tolower(static_cast<unsigned char>(rhs));
        });
}

bool applyTSATransportHardening(CURL* curl, const TSAConfig& config, std::string& lastError) {
    if (curl == nullptr) {
        lastError = "CURL init failed";
        return false;
    }
    if (!startsWithHttps(config.url)) {
        lastError = "TSA URL must use HTTPS";
        return false;
    }

    const long timeoutSeconds = config.timeout_seconds > 0 ? static_cast<long>(config.timeout_seconds) : 30L;
    const long connectTimeoutSeconds = std::clamp(timeoutSeconds / 3, 2L, 10L);

    curl_easy_setopt(curl, CURLOPT_PROTOCOLS, static_cast<long>(CURLPROTO_HTTPS));
    curl_easy_setopt(curl, CURLOPT_REDIR_PROTOCOLS, static_cast<long>(CURLPROTO_HTTPS));
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, connectTimeoutSeconds);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeoutSeconds);
    curl_easy_setopt(curl, CURLOPT_SSLVERSION, static_cast<long>(CURL_SSLVERSION_TLSv1_2));
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, config.verify_tsa_cert ? 1L : 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, config.verify_tsa_cert ? 2L : 0L);
    curl_easy_setopt(curl, CURLOPT_HTTPAUTH, config.username.empty() ? 0L : static_cast<long>(CURLAUTH_BASIC));
    return true;
}

} // namespace

static const EVP_MD* selectDigest(const std::string& algo){
    if(algo == "SHA384") {
      return EVP_sha384();
    }
    if(algo == "SHA512") {
      return EVP_sha512();
    }
    return EVP_sha256();
}

[[maybe_unused]] static std::string hex(const std::vector<uint8_t>& data){
    static const char* d = "0123456789abcdef"; std::string out; out.reserve(data.size()*2);
    for(auto b: data){ out.push_back(d[(b>>4)&0xF]); out.push_back(d[b&0xF]); }
    return out;
}

static std::string b64Encode(const std::vector<uint8_t>& data){
    TSA_BIO_ptr b64_ptr(BIO_new(BIO_f_base64()));
    TSA_BIO_ptr mem_ptr(BIO_new(BIO_s_mem()));
    
    if (!b64_ptr.get() || !mem_ptr.get()) {
        throw std::runtime_error("Failed to create BIO objects for base64 encoding");
    }
    
    BIO* b64 = b64_ptr.get();
    BIO* mem = mem_ptr.get();
    
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    b64 = BIO_push(b64, mem);
    mem_ptr.release();  // BIO_push takes ownership; prevent double-free
    
    BIO_write(b64, data.data(), (int)data.size());
    BIO_flush(b64);
    BUF_MEM* ptr = nullptr; 
    BIO_get_mem_ptr(b64, &ptr);
    std::string out = {};
    if (ptr && ptr->data && ptr->length > 0) {
        out.assign(ptr->data, ptr->length);
    }
    // b64_ptr destructor automatically cleans up the BIO chain
    return out;
}

static std::vector<uint8_t> b64Decode(const std::string& s){
    TSA_BIO_ptr b64_ptr(BIO_new(BIO_f_base64()));
    TSA_BIO_ptr mem_ptr(BIO_new_mem_buf(s.data(), (int)s.size()));
    
    if (!b64_ptr.get() || !mem_ptr.get()) {
        throw std::runtime_error("Failed to create BIO objects for base64 decoding");
    }
    
    BIO* b64 = b64_ptr.get();
    BIO* mem = mem_ptr.get();
    
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    mem = BIO_push(b64, mem);
    mem_ptr.release();  // BIO_push takes ownership; prevent double-free
    
    std::vector<uint8_t> out(s.size());
    int len = BIO_read(mem, out.data(), (int)out.size());
    if(len < 0) {
      len = 0;
    }
    out.resize(len);
    // b64_ptr destructor automatically cleans up the BIO chain
    return out;
}

// Helper: Convert ASN1_GENERALIZEDTIME to Unix milliseconds
static uint64_t asn1TimeToUnixMs(ASN1_GENERALIZEDTIME* gen) {
    if (!gen || !gen->data) {
      return 0;
    }
    
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
        const char* old_tz = getenv("TZ");
        std::string old_tz_copy = {};
        if (old_tz) {
            old_tz_copy = old_tz;
        }
        
        if (setenv("TZ", "UTC", 1) != 0) {
            return 0;  // setenv failed
        }
        tzset();
        time_t t = mktime(&tm_time);
        
        // Restore original TZ (even if mktime failed)
        if (!old_tz_copy.empty()) {
            setenv("TZ", old_tz_copy.c_str(), 1);
        } else {
            unsetenv("TZ");
        }
        tzset();
    #endif
#endif
    
    if (t == -1) {
      return 0;
    }
    
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
    TSA_EVP_MD_CTX_ptr ctx(EVP_MD_CTX_new());
    if (!ctx.get()) {
      throw std::runtime_error("EVP_MD_CTX_new failed");
    }
    
    std::vector<uint8_t> out(EVP_MD_size(md));
    unsigned int outlen=0;
    
    if (EVP_DigestInit_ex(ctx.get(), md, nullptr) != 1) {
        throw std::runtime_error("EVP_DigestInit_ex failed");
    }
    EVP_DigestUpdate(ctx.get(), data.data(), data.size());
    if (EVP_DigestFinal_ex(ctx.get(), out.data(), &outlen) != 1) {
        throw std::runtime_error("EVP_DigestFinal_ex failed");
    }
    out.resize(outlen);
    return out;
}

std::vector<uint8_t> TimestampAuthority::generateNonce([[maybe_unused]] size_t bytes){
    if (bytes == 0) {
        return {};
    }

    constexpr size_t kMaxRandBytes = static_cast<size_t>(std::numeric_limits<int>::max());
    if (bytes > kMaxRandBytes) {
        THEMIS_ERROR("TimestampAuthority::generateNonce: requested size {} exceeds RAND_bytes "
                     "limit {}", bytes, kMaxRandBytes);
        return {};
    }

    std::vector<uint8_t> n(bytes);
    if(RAND_bytes(n.data(), static_cast<int>(bytes)) != 1){
        for(size_t i=0;i<bytes;++i) n[i] = (uint8_t)i; // fallback deterministic
    }
    return n;
}

std::vector<uint8_t> TimestampAuthority::createTSPRequest(const std::vector<uint8_t>& hash,const std::vector<uint8_t>& nonce){
    TSA_TS_REQ_ptr req(TS_REQ_new()); 
    if(!req.get()){ last_error_="TS_REQ_new failed"; return {}; }
    
    TS_REQ_set_version(req.get(), 1);
    
    // Message imprint - use RAII wrappers for all components
    TSA_TS_MSG_IMPRINT_ptr imprint(TS_MSG_IMPRINT_new());
    if(!imprint.get()){ last_error_="TS_MSG_IMPRINT_new failed"; return {}; }
    
    const EVP_MD* md = selectDigest(config_.hash_algorithm);
    TSA_X509_ALGOR_ptr algo(X509_ALGOR_new());
    if(!algo.get()){ last_error_="X509_ALGOR_new failed"; return {}; }
    
    X509_ALGOR_set0(algo.get(), OBJ_nid2obj(EVP_MD_type(md)), V_ASN1_NULL, nullptr);
    TS_MSG_IMPRINT_set_algo(imprint.get(), algo.get());
    
    // Note: TS_MSG_IMPRINT_set_msg takes ownership of the hash data reference,
    // so we pass it directly without wrapping in unique_ptr
    TS_MSG_IMPRINT_set_msg(imprint.get(), const_cast<unsigned char*>(hash.data()), (int)hash.size());
    TS_REQ_set_msg_imprint(req.get(), imprint.get());
    
    // Nonce handling with RAII
    if(!nonce.empty()){
        TSA_ASN1_INTEGER_ptr nonce_i(ASN1_INTEGER_new());
        if(nonce_i.get()){
            TSA_BIGNUM_ptr bn(BN_bin2bn(nonce.data(), (int)nonce.size(), nullptr));
            if(bn.get()){
                BN_to_ASN1_INTEGER(bn.get(), nonce_i.get());
                TS_REQ_set_nonce(req.get(), nonce_i.get());
            }
        }
    }
    
    if(config_.cert_req) {
      TS_REQ_set_cert_req(req.get(), 1);
    }
    
    if(!config_.policy_oid.empty()){
        TSA_ASN1_OBJECT_ptr policy(OBJ_txt2obj(config_.policy_oid.c_str(), 1));
        if(policy.get()){ 
            TS_REQ_set_policy_id(req.get(), policy.get());
        }
    }
    
    unsigned char* der_raw = nullptr;
    int len = i2d_TS_REQ(req.get(), &der_raw);
    TSA_OPENSSL_Buffer_ptr der(der_raw);
    
    std::vector<uint8_t> out;
    if(len > 0 && der.get()){ 
        out.assign(der.get(), der.get() + len); 
    }
    // RAII wrappers automatically clean up req, imprint, algo, and other allocated objects
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
    if(!applyTSATransportHardening(impl_->curl, config_, last_error_)){ return {}; }
    std::vector<uint8_t> response;
    curl_easy_setopt(impl_->curl, CURLOPT_URL, config_.url.c_str());
    curl_easy_setopt(impl_->curl, CURLOPT_POST, 1L);
    curl_easy_setopt(impl_->curl, CURLOPT_NOBODY, 0L);
    curl_easy_setopt(impl_->curl, CURLOPT_POSTFIELDS, request.data());
    curl_easy_setopt(impl_->curl, CURLOPT_POSTFIELDSIZE, (long)request.size());
    curl_easy_setopt(impl_->curl, CURLOPT_WRITEFUNCTION, curlWrite);
    curl_easy_setopt(impl_->curl, CURLOPT_WRITEDATA, &response);

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

    TSA_curl_slist_ptr headers(nullptr);
    headers.reset(curl_slist_append(headers.get(), "Content-Type: application/timestamp-query"));
    headers.reset(curl_slist_append(headers.get(), "Accept: application/timestamp-reply"));
    curl_easy_setopt(impl_->curl, CURLOPT_HTTPHEADER, headers.get());
    CURLcode res = curl_easy_perform(impl_->curl);
    if(res!=CURLE_OK){ last_error_ = std::string("curl error: ")+curl_easy_strerror(res); return {}; }
    long code=0; curl_easy_getinfo(impl_->curl, CURLINFO_RESPONSE_CODE, &code);
    if(code!=200){ last_error_ = "HTTP status "+std::to_string(code); return {}; }
    return response;
}

TimestampToken TimestampAuthority::parseTSPResponse(const std::vector<uint8_t>& respBytes){
    TimestampToken token;
    const unsigned char* p = respBytes.data();
    
    // Use RAII wrapper to ensure TS_RESP is freed on all paths
    TS_RESP_ptr resp(d2i_TS_RESP(nullptr, &p, (long)respBytes.size()));
    if(!resp.get()){ token.error_message="d2i_TS_RESP failed"; return token; }
    
    TS_STATUS_INFO* status = TS_RESP_get_status_info(resp.get());
    const ASN1_INTEGER* st_int = TS_STATUS_INFO_get0_status(status);
    token.pki_status = ASN1_INTEGER_get(st_int);
    if(token.pki_status != 0 && token.pki_status != 1){ 
        token.error_message="TSA rejected"; 
        return token; 
    }
    
    PKCS7* pkcs7 = TS_RESP_get_token(resp.get());
    if(!pkcs7){ token.error_message="No PKCS7"; return token; }
    
    unsigned char* der_raw = nullptr; 
    int der_len = i2d_PKCS7(pkcs7, &der_raw);
    TSA_OPENSSL_Buffer_ptr der(der_raw);
    
    if(der_len > 0 && der.get()){ 
        token.token_der.assign(der.get(), der.get() + der_len); 
    }
    token.token_b64 = b64Encode(token.token_der);
    
    // Extract TSA certificate from PKCS7 (if cert_req was true)
    // Note: PKCS7_get0_signers returns a pointer to the PKCS7's internal stack,
    // so we don't own it. Use STACK_OF(X509)_ptr only as a view to manage within scope.
    STACK_OF(X509)* certs = PKCS7_get0_signers(pkcs7, nullptr, 0);
    if(certs && sk_X509_num(certs) > 0){
        X509* tsa_x509 = sk_X509_value(certs, 0);
        if(tsa_x509){
            // Store certificate in DER format
            unsigned char* cert_der_raw = nullptr;
            int cert_len = i2d_X509(tsa_x509, &cert_der_raw);
            TSA_OPENSSL_Buffer_ptr cert_der(cert_der_raw);
            
            if(cert_len > 0 && cert_der.get()){
                token.tsa_cert.assign(cert_der.get(), cert_der.get() + cert_len);
            }
            
            // Extract certificate serial number
            const ASN1_INTEGER* cert_serial = X509_get0_serialNumber(tsa_x509);
            if(cert_serial){
                TSA_BIGNUM_ptr bn(ASN1_INTEGER_to_BN(cert_serial, nullptr));
                if(bn.get()){
                    TSA_OPENSSL_CStr_ptr hexStr(BN_bn2hex(bn.get()));
                    if(hexStr.get()){
                        token.tsa_serial = hexStr.get();
                    }
                }
            }
            
            // Extract subject name
            X509_NAME* subject = X509_get_subject_name(tsa_x509);
            if(subject){
                TSA_BIO_ptr name_bio(BIO_new(BIO_s_mem()));
                if(name_bio.get()){
                    X509_NAME_print_ex(name_bio.get(), subject, 0, XN_FLAG_RFC2253);
                    BUF_MEM* name_buf = nullptr;
                    BIO_get_mem_ptr(name_bio.get(), &name_buf);
                    if (name_buf && name_buf->data && name_buf->length > 0) {
                        token.tsa_name.assign(name_buf->data, name_buf->length);
                    }
                }
            }
        }
    }
    // Note: certs is owned by pkcs7, don't free it explicitly
    
    TSA_TS_TST_INFO_ptr tst(PKCS7_to_TS_TST_INFO(pkcs7));
    if(tst.get()){
        const ASN1_GENERALIZEDTIME* gen = TS_TST_INFO_get_time(tst.get());
        if(gen){ 
            std::string g(reinterpret_cast<const char*>(gen->data), gen->length); 
            token.timestamp_utc = g;
            token.timestamp_unix_ms = asn1TimeToUnixMs(const_cast<ASN1_GENERALIZEDTIME*>(gen));
        }
        const ASN1_INTEGER* serial = TS_TST_INFO_get_serial(tst.get());
        if(serial){ 
            TSA_BIGNUM_ptr bn(ASN1_INTEGER_to_BN(serial, nullptr));
            if(bn.get()){
                char* hexStr = BN_bn2hex(bn.get());
                if(hexStr){
                    token.serial_number = hexStr; 
                    OPENSSL_free(hexStr);
                }
            }
        }
        ASN1_OBJECT* policy = TS_TST_INFO_get_policy_id(tst.get());
        if(policy){ 
            char buf[128]; 
            OBJ_obj2txt(buf,sizeof(buf),policy,1); 
            token.policy_oid=buf; 
        }
        
        // Extract accuracy metadata (RFC 3161 - optional)
        const TS_ACCURACY* accuracy = TS_TST_INFO_get_accuracy(tst.get());
        if(accuracy){
            token.has_accuracy = true;
            const ASN1_INTEGER* seconds = TS_ACCURACY_get_seconds(accuracy);
            const ASN1_INTEGER* millis = TS_ACCURACY_get_millis(accuracy);
            const ASN1_INTEGER* micros = TS_ACCURACY_get_micros(accuracy);
            if(seconds) {
              token.accuracy_seconds = ASN1_INTEGER_get(seconds);
            }
            if(millis) {
              token.accuracy_millis = ASN1_INTEGER_get(millis);
            }
            if(micros) {
              token.accuracy_micros = ASN1_INTEGER_get(micros);
            }
        }
        
        // Extract ordering hint (RFC 3161 - optional, default FALSE)
        int ordering = TS_TST_INFO_get_ordering(tst.get());
        token.ordering = (ordering != 0);
    }
    
    token.success=true; token.verified=false; // separate verification step
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
    if(token.token_der.empty()) {
      return false;
    }
    try {
        const unsigned char* p = token.token_der.data();
        PKCS7_ptr pkcs7(d2i_PKCS7(nullptr,&p,(long)token.token_der.size()));
        if(!pkcs7) {
          return false;
        }
        
        TSA_TS_TST_INFO_ptr tst(PKCS7_to_TS_TST_INFO(pkcs7.get()));
        if(!tst) {
          return false;
        }
        
        TS_MSG_IMPRINT* imprint = TS_TST_INFO_get_msg_imprint(tst.get());
        if (!imprint) {
          return false;
        }
        
        ASN1_OCTET_STRING* os = TS_MSG_IMPRINT_get_msg(imprint);
        if (!os) {
          return false;
        }
        
        bool match = (os->length == (int)hash.size() && std::memcmp(os->data, hash.data(), hash.size())==0);
        return match;
    } catch (const std::exception& e) {
        THEMIS_ERROR("verifyTimestampForHash error: {}", e.what());
        return false;
    }
}

bool TimestampAuthority::verifyTimestamp(const std::vector<uint8_t>& data,const TimestampToken& token){
    auto h = computeHash(data);
    return verifyTimestampForHash(h, token);
}

TimestampToken TimestampAuthority::parseToken(const std::vector<uint8_t>& der){ return parseTSPResponse(der); }
TimestampToken TimestampAuthority::parseToken(const std::string& b64){ auto der = b64Decode(b64); return parseTSPResponse(der); }

std::optional<std::string> TimestampAuthority::getTSACertificate(){
    if(cached_tsa_cert_.empty()) {
      return std::nullopt;
    }
    
    try {
        // Convert DER to PEM format
        const unsigned char* p = cached_tsa_cert_.data();
        if (cached_tsa_cert_.size() > static_cast<std::size_t>(LONG_MAX)) {
            THEMIS_ERROR("getTSACertificate error: cached TSA cert exceeds OpenSSL size limit");
            return std::nullopt;
        }
        X509_ptr cert(d2i_X509(nullptr, &p, static_cast<long>(cached_tsa_cert_.size())));
        if(!cert) {
          return std::nullopt;
        }
        
        TSA_BIO_ptr bio(BIO_new(BIO_s_mem()));
        if(!bio) {
          return std::nullopt;
        }
        
        PEM_write_bio_X509(bio.get(), cert.get());
        BUF_MEM* mem = nullptr;
        BIO_get_mem_ptr(bio.get(), &mem);
        std::string pem = {};
        if (mem && mem->data && mem->length > 0) {
            pem.assign(mem->data, mem->length);
        }
        return pem;
    } catch (const std::exception& e) {
        THEMIS_ERROR("getTSACertificate error: {}", e.what());
        return std::nullopt;
    }
}

bool TimestampAuthority::isAvailable(){
    if(!impl_->curl) {
      return false;
    }
    if(!applyTSATransportHardening(impl_->curl, config_, last_error_)){ return false; }
    curl_easy_setopt(impl_->curl, CURLOPT_URL, config_.url.c_str());
    curl_easy_setopt(impl_->curl, CURLOPT_NOBODY, 1L);
    curl_easy_setopt(impl_->curl, CURLOPT_TIMEOUT, 5L);
    if(!config_.ca_cert_path.empty()){
        curl_easy_setopt(impl_->curl, CURLOPT_CAINFO, config_.ca_cert_path.c_str());
    }
    CURLcode res = curl_easy_perform(impl_->curl);
    // Reset NOBODY so subsequent POST requests work correctly
    curl_easy_setopt(impl_->curl, CURLOPT_NOBODY, 0L);
    applyTSATransportHardening(impl_->curl, config_, last_error_);
    return res == CURLE_OK;
}

std::string TimestampAuthority::getLastError() const { return last_error_; }

// ============================================================================
// eIDAS Timestamp Validator Implementation
// ============================================================================

bool eIDASTimestampValidator::validateeIDASTimestamp(
    const TimestampToken& token,
    const std::vector<std::string>& trust_anchors) {
    (void)trust_anchors;
    
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
    constexpr uint64_t MS_PER_DAY = 86400000;
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
    
    try {
        // Parse TSA certificate
        TSA_BIO_ptr bio(BIO_new_mem_buf(tsa_cert.data(), static_cast<int>(tsa_cert.size())));
        if (!bio) {
            validation_errors_.push_back("Failed to create BIO for certificate");
            return false;
        }
        
        X509_ptr cert(PEM_read_bio_X509(bio.get(), nullptr, nullptr, nullptr));
        
        if (!cert) {
            validation_errors_.push_back("Failed to parse TSA certificate");
            return false;
        }
        
        // Extract subject name
        X509_NAME* subject = X509_get_subject_name(cert.get());
        if (!subject) {
            validation_errors_.push_back("Certificate has no subject");
            return false;
        }
        
        // Extract subject name using dynamic allocation
        TSA_BIO_ptr name_bio(BIO_new(BIO_s_mem()));
        if (!name_bio) {
            validation_errors_.push_back("Failed to create BIO for subject name");
            return false;
        }
        
        X509_NAME_print_ex(name_bio.get(), subject, 0, XN_FLAG_RFC2253);
        BUF_MEM* name_buf = nullptr;
        BIO_get_mem_ptr(name_bio.get(), &name_buf);
        std::string subject_name = {};
        if (name_buf && name_buf->data && name_buf->length > 0) {
            subject_name.assign(name_buf->data, name_buf->length);
        }
        
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
    } catch (const std::exception& e) {
        validation_errors_.push_back(std::string("isQualifiedTSA error: ") + e.what());
        return false;
    }
}

std::vector<std::string> eIDASTimestampValidator::getValidationErrors() const {
    return validation_errors_;
}

} } // namespace themis::security

#endif // THEMIS_USE_OPENSSL_TSA
