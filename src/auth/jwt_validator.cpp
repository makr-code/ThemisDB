/**
 * @file jwt_validator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=4, M=4, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "auth/jwt_validator.h"

#include <algorithm>
#include <cstring>
#include <curl/curl.h>
#include <mutex>
#include <openssl/bio.h>
#include <openssl/bn.h>
#include <openssl/buffer.h>
#include <openssl/crypto.h>
#include <openssl/ec.h>
#include <openssl/ecdsa.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/sha.h>
#include <sstream>
#include <stdexcept>
#include <thread>

#include "auth/jwks_validator.h"
#include "utils/audit_logger.h"
#include "utils/hkdf_helper.h"
#include "utils/logger.h"
#include "utils/openssl_deleter.h"

namespace themis {
namespace auth {

namespace {
/**
 * @brief libcurl write callback that appends received bytes to a std::string.
 * @param ptr Input buffer from libcurl.
 * @param size Element size.
 * @param nmemb Element count.
 * @param userdata std::string* output accumulator.
 * @return Number of bytes consumed.
 */
size_t curlWriteToString(char *ptr, size_t size, size_t nmemb, void *userdata) {
    auto total = size * nmemb;
    auto *out  = static_cast<std::string *>(userdata);
    out->append(ptr, total);
    return total;
}
} // namespace

/**
 * @brief Construct validator from JWKS URL with permissive issuer/audience checks.
 * @param jwks_url JWKS endpoint URL.
 */
JWTValidator::JWTValidator(const std::string &jwks_url)
    : cfg_{JWTValidatorConfig{
          .jwks_url                    = jwks_url,
          .expected_issuer             = std::nullopt,
          .expected_audience           = std::nullopt,
          .cache_ttl                   = std::chrono::seconds(600),
          .clock_skew                  = std::chrono::seconds(60),
          .revoked_kids                = {},
          .require_issuer_validation   = false,
          .require_audience_validation = false,
      }},
      jwks_url_(jwks_url), jwks_cache_time_(std::chrono::system_clock::time_point::min()),
      worker_pool_(std::make_unique<AuthWorkerThreadPool>(AuthWorkerThreadPool::kMinThreads,
                                                          AuthWorkerThreadPool::kMaxThreads)) {}

/**
 * @brief Construct validator from explicit runtime configuration.
 * @param cfg Validation and cache configuration.
 * @throws std::runtime_error if required issuer/audience constraints are missing.
 */
JWTValidator::JWTValidator(const JWTValidatorConfig &cfg)
    : cfg_(cfg), jwks_url_(cfg.jwks_url), jwks_cache_time_(std::chrono::system_clock::time_point::min()),
      worker_pool_(std::make_unique<AuthWorkerThreadPool>(AuthWorkerThreadPool::kMinThreads,
                                                          AuthWorkerThreadPool::kMaxThreads)) {
    // Normalize empty string values to nullopt so empty strings are treated as 'unset'
    auto normalizeOptional = [](std::optional<std::string> &opt) {
        if (opt.has_value() && opt->empty()) {
            opt = std::nullopt;
        }
    };
    normalizeOptional(cfg_.expected_issuer);
    normalizeOptional(cfg_.expected_audience);
    if (cfg_.require_issuer_validation && !cfg_.expected_issuer.has_value()) {
        throw std::runtime_error("Issuer validation not configured");
    }
    if (cfg_.require_audience_validation && !cfg_.expected_audience.has_value()) {
        throw std::runtime_error("Audience validation not configured");
    }
    if (!cfg_.require_issuer_validation && !cfg_.expected_issuer.has_value()) {
        utils::Logger::warn("JWT issuer validation is disabled - tokens from any issuer will be accepted");
    }
    if (!cfg_.require_audience_validation && !cfg_.expected_audience.has_value()) {
        utils::Logger::warn("JWT audience validation is disabled - tokens with any audience will be accepted");
    }
}

/**
 * @brief Decode Base64URL text into raw bytes.
 * @param input Base64URL encoded payload.
 * @return Decoded bytes, or empty vector on decode failure.
 */
std::vector<uint8_t> JWTValidator::decodeBase64Url(const std::string &input) {
    std::string base64 = input;
    std::replace(base64.begin(), base64.end(), '-', '+');
    std::replace(base64.begin(), base64.end(), '_', '/');
    while (base64.size() % 4 != 0) {
        base64 += '=';
    }

    BIO *bmem = BIO_new_mem_buf(base64.data(), static_cast<int>(base64.size()));
    if (!bmem) {
        return {};
    }
    BIO *b64 = BIO_new(BIO_f_base64());
    if (!b64) {
        BIO_free(bmem);
        return {};
    }
    auto bio = utils::BIOPtr(BIO_push(b64, bmem)); // BIO_push returns top of chain
    BIO_set_flags(bio.get(), BIO_FLAGS_BASE64_NO_NL);

    std::vector<uint8_t> decoded(base64.size());
    int len = BIO_read(bio.get(), decoded.data(), static_cast<int>(decoded.size()));
    if (len < 0) {
        return {};
    }
    decoded.resize(len);
    return decoded;
}

/**
 * @brief Decode Base64URL text into string payload.
 * @param input Base64URL encoded text.
 * @return Decoded string.
 */
std::string JWTValidator::decodeBase64UrlToString(const std::string &input) {
    auto bytes = decodeBase64Url(input);
    return std::string(reinterpret_cast<const char*>(bytes.data()), bytes.size());
}

/**
 * @brief Fetch JWKS with cache TTL and single-flight refresh behavior.
 * @return Validated JWKS JSON document.
 * @throws std::runtime_error if JWKS retrieval or validation fails.
 */
nlohmann::json JWTValidator::fetchJWKS() {
    auto now = std::chrono::system_clock::now();

    // Fast path: shared (reader) lock — concurrent reads proceed in parallel.
    {
        std::shared_lock<std::shared_mutex> read_lock(jwks_cache_mutex_);
        if (!jwks_cache_.empty() && now - jwks_cache_time_ < cfg_.cache_ttl) {
            return jwks_cache_;
        }
    }

    // Single-flight: only one thread performs the HTTP fetch at a time.
    // Others wait for it to finish and then read from the (now-fresh) cache.
    {
        std::unique_lock<std::mutex> refresh_lock(jwks_refresh_mutex_);
        // Wait for the in-progress refresh to complete; bounded by
        // refresh_wait_timeout to prevent indefinite blocking if the refresher
        // thread stalls on a slow or unresponsive JWKS endpoint (Phase 8.2).
        const bool refresher_done = jwks_refresh_cv_.wait_for(
            refresh_lock,
            cfg_.refresh_wait_timeout,
            [this] { return !jwks_refreshing_; });

        if (!refresher_done) {
            // Timeout expired — return stale cache or empty set (fail-open
            // is avoided: callers must handle an empty JWKS set as an error).
            THEMIS_WARN("JWKS single-flight wait timed out after {}ms; "
                        "returning stale/empty cache",
                        cfg_.refresh_wait_timeout.count());
            std::shared_lock<std::shared_mutex> read_lock(jwks_cache_mutex_);
            return jwks_cache_;
        }

        // Double-check: the refreshing thread may have just updated the cache.
        {
            std::shared_lock<std::shared_mutex> read_lock(jwks_cache_mutex_);
            const auto recheck_now = std::chrono::system_clock::now();
            if (!jwks_cache_.empty() && recheck_now - jwks_cache_time_ < cfg_.cache_ttl) {
                return jwks_cache_;
            }
        }

        // We are the designated refresher for this round.
        jwks_refreshing_ = true;
    }

    struct ScopedRefreshReset final {
        explicit ScopedRefreshReset(JWTValidator *v) : validator(v) {}
        JWTValidator *const validator;
        ScopedRefreshReset(const ScopedRefreshReset &)            = delete;
        ScopedRefreshReset &operator=(const ScopedRefreshReset &) = delete;
        ScopedRefreshReset(ScopedRefreshReset &&)                 = delete;
        ScopedRefreshReset &operator=(ScopedRefreshReset &&)      = delete;
        ~ScopedRefreshReset() noexcept {
            std::lock_guard<std::mutex> refresh_lock(validator->jwks_refresh_mutex_);
            validator->jwks_refreshing_ = false;
            validator->jwks_refresh_cv_.notify_all();
        }
    } refresh_guard{this};

    // Perform the HTTP fetch completely outside all locks so concurrent
    // readers/validators are never stalled.
    nlohmann::json fetched_json;
    std::exception_ptr fetch_exc = {};

    try {
        std::string response = {};
        int attempt        = 0;
        int retry_delay_ms = 100; // Start with 100 ms; doubles each attempt
        CURLcode rc        = CURLE_FAILED_INIT;
        long http_code     = 0;

        while (attempt < cfg_.jwks_max_retries) {
            attempt++;
            response.clear();

            CURL *curl = curl_easy_init();
            if (!curl) {
                utils::Logger::error("Failed to init curl for JWKS fetch");
            } else {
                curl_easy_setopt(curl, CURLOPT_URL, jwks_url_.c_str());
                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteToString);
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
                curl_easy_setopt(curl, CURLOPT_TIMEOUT, static_cast<long>(cfg_.jwks_timeout_seconds));
                curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, static_cast<long>(cfg_.jwks_timeout_seconds));

                CURLM *multi = curl_multi_init();
                if (!multi) {
                    curl_easy_cleanup(curl);
                    curl = nullptr;
                } else {
                    CURLMcode add_rc = curl_multi_add_handle(multi, curl);
                    if (add_rc != CURLM_OK) {
                        curl_multi_cleanup(multi);
                        curl_easy_cleanup(curl);
                        curl = nullptr;
                    } else {
                        int still_running = 0;
                        CURLMcode mc      = CURLM_OK;
                        do {
                            mc = curl_multi_perform(multi, &still_running);
                            if (mc != CURLM_OK) {
                                break;
                            }
                            if (still_running) {
                                curl_multi_wait(multi, nullptr, 0, 1000, nullptr);
                            }
                        } while (still_running && mc == CURLM_OK);

                        // Read per-transfer result via curl_multi_info_read().
                        if (mc == CURLM_OK) {
                            CURLMsg *msg  = nullptr;
                            int msgs_left = 0;
                            while ((msg = curl_multi_info_read(multi, &msgs_left))) {
                                if (msg->msg == CURLMSG_DONE && msg->easy_handle == curl) {
                                    rc = msg->data.result;
                                }
                            }
                        }

                        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
                        curl_multi_remove_handle(multi, curl);
                        curl_multi_cleanup(multi);
                        curl_easy_cleanup(curl);
                        curl = nullptr;
                    }
                }
            }

            if (rc == CURLE_OK && http_code == 200) {
                break;
            }

            if (attempt < cfg_.jwks_max_retries) {
                utils::Logger::warn("JWKS fetch attempt " + std::to_string(attempt) + " failed (HTTP "
                                    + std::to_string(http_code) + ", curl error " + std::to_string(rc)
                                    + "), retrying...");
                // Back-off: no lock held, so readers/validators remain unblocked.
                std::this_thread::sleep_for(std::chrono::milliseconds(retry_delay_ms));
                retry_delay_ms *= 2;
            }
        }

        if (rc != CURLE_OK || http_code != 200) {
            utils::Logger::error("JWKS HTTP error after " + std::to_string(attempt) + " attempts: HTTP "
                                 + std::to_string(http_code) + ", curl error " + std::to_string(rc));
            throw std::runtime_error("JWKS HTTP error: " + std::to_string(http_code) + " (after "
                                     + std::to_string(attempt) + " attempts)");
        }

        auto json = nlohmann::json::parse(response);
        if (!json.is_object() || !json.contains("keys")) {
            utils::Logger::error("Invalid JWKS document (missing keys)");
            throw std::runtime_error("Invalid JWKS document (missing keys)");
        }

        JWKSValidator jwks_validator;
        jwks_validator.validateOrThrow(json);

        fetched_json = std::move(json);
        utils::Logger::info("JWKS fetched successfully on attempt " +
                            std::to_string(attempt));
    } catch (const nlohmann::json::exception &) {
        fetch_exc = std::current_exception();
    } catch (const std::exception &) {
        fetch_exc = std::current_exception();
    } catch (const std::string &) {
        fetch_exc = std::current_exception();
    } catch (const char *) {
        fetch_exc = std::current_exception();
    }

    // Briefly acquire the write lock only to update the cache (or discard if
    // another thread already refreshed while we were fetching).
    if (!fetch_exc) {
        std::unique_lock<std::shared_mutex> write_lock(jwks_cache_mutex_);
        const auto cache_now = std::chrono::system_clock::now();
        if (jwks_cache_.empty() || cache_now - jwks_cache_time_ >= cfg_.cache_ttl) {
            jwks_cache_      = fetched_json;
            jwks_cache_time_ = cache_now;
        }
    }

    if (fetch_exc) {
        std::rethrow_exception(fetch_exc);
    }

    // Return whatever is now in the cache (may be from a concurrent refresher).
    std::shared_lock<std::shared_mutex> read_lock(jwks_cache_mutex_);
    return jwks_cache_;
}

/**
 * @brief Locate JWK by key id in a JWKS document.
 * @param jwks JWKS JSON.
 * @param kid Key id to match.
 * @return Pointer to matching JWK object, or nullptr.
 */
const nlohmann::json *JWTValidator::findJwkForKid(const nlohmann::json &jwks, const std::string &kid) const {
    if (!jwks.contains("keys")) {
        return nullptr;
    }
    // Use CRYPTO_memcmp for constant-time key-ID comparison to prevent timing
    // side-channels that could allow an attacker to enumerate valid kid values
    // through response-time differences.  Length mismatch is resolved by always
    // comparing max(len_a, len_b) bytes against a zero-padded scratch buffer so
    // that the loop time is independent of whether the lengths match.
    const std::string::size_type target_len = kid.size();
    const nlohmann::json *match = nullptr;
    for (auto &k : jwks["keys"]) {
        if (!k.is_object()) {
            continue;
        }
        const std::string stored_kid = k.value("kid", std::string());
        // Always compare the same number of bytes regardless of stored_kid length
        // to avoid a length-based early exit that leaks partial information.
        const std::string::size_type cmp_len =
            std::max(stored_kid.size(), target_len);
        if (cmp_len == 0) {
            // Both empty → equal; record but continue scanning (no early exit).
            if (match == nullptr) {
                match = &k;
            }
            continue;
        }
        // Pad shorter strings to cmp_len with NUL bytes before comparing.
        std::string a_padded(cmp_len, '\0');
        std::string b_padded(cmp_len, '\0');
        std::memcpy(a_padded.data(), stored_kid.data(),static_cast<int>(stored_kid.size()));
        std::memcpy(b_padded.data(), kid.data(),        target_len);
        if (CRYPTO_memcmp(a_padded.data(), b_padded.data(), cmp_len) == 0
                && static_cast<int>(stored_kid.size()) == target_len) {
            // Record first match but keep iterating to avoid early-exit leakage.
            if (match == nullptr) {
                match = &k;
            }
        }
    }
    return match;
}

/**
 * @brief Compatibility wrapper for RSA SHA-256 signature verification.
 * @param header_payload JWT signing input.
 * @param signature Decoded signature bytes.
 * @param jwk RSA key material.
 * @return true when signature verification succeeds.
 */
bool JWTValidator::verifySignatureRS256(const std::string &header_payload, const std::vector<uint8_t> &signature,
                                        const nlohmann::json &jwk) {
    return verifySignatureRSA(header_payload, signature, jwk, "RS256");
}

bool JWTValidator::verifySignatureRSA(const std::string &header_payload, const std::vector<uint8_t> &signature,
                                      const nlohmann::json &jwk, const std::string &alg) {
    if (jwk.value("kty", "") != "RSA") {
        return false;
    }
    auto n_b64 = jwk.value("n", "");
    auto e_b64 = jwk.value("e", "");
    if (n_b64.empty() || e_b64.empty()) {
        return false;
    }
    auto n_bytes = decodeBase64Url(n_b64);
    auto e_bytes = decodeBase64Url(e_b64);
    auto n       = utils::BIGNUMPtr(BN_bin2bn(n_bytes.data(), (int)n_bytes.size(), nullptr));
    auto e       = utils::BIGNUMPtr(BN_bin2bn(e_bytes.data(), (int)e_bytes.size(), nullptr));
    if (!n || !e) {
        return false;
    }

    // Use EVP_PKEY directly instead of deprecated RSA_new()
    auto pkey = utils::make_evp_key();
    if (!pkey) {
        return false;
    }

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996) // OpenSSL deprecated APIs
#endif
    auto rsa = utils::make_rsa();
    if (!rsa) {
        return false;
    }

    // RSA_set0_key takes ownership only on success, so we need to release after success
    if (RSA_set0_key(rsa.get(), n.get(), e.get(), nullptr) != 1) {
        // Failed - n and e will be cleaned up by unique_ptr
        return false;
    }
    // Success - RSA now owns n and e, so release them from unique_ptr
    n.release();
    e.release();

    if (EVP_PKEY_assign_RSA(pkey.get(), rsa.get()) != 1) {
        return false;
    }
    // Success - pkey now owns rsa, so release it from unique_ptr
    rsa.release();
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // Select digest based on algorithm (RS256 → SHA-256, RS384 → SHA-384, RS512 → SHA-512)
    const EVP_MD *md = nullptr;
    if (alg == "RS256") {
        md = EVP_sha256();
    } else if (alg == "RS384") {
        md = EVP_sha384();
    } else if (alg == "RS512") {
        md = EVP_sha512();
    } else {
        return false;
    }
    // Verify using EVP_DigestVerify with selected digest and PKCS#1 v1.5
    auto mctx = utils::make_evp_md_ctx();
    if (!mctx) {
        return false;
    }
    int ok = EVP_DigestVerifyInit(mctx.get(), nullptr, md, nullptr, pkey.get());
    if (ok != 1) {
        return false;
    }
    ok = EVP_DigestVerifyUpdate(mctx.get(), header_payload.data(),static_cast<int>(header_payload.size()));
    if (ok != 1) {
        return false;
    }
    ok = EVP_DigestVerifyFinal(mctx.get(), signature.data(),static_cast<int>(signature.size()));
    return ok == 1;
}

bool JWTValidator::verifySignatureES256(const std::string &header_payload, const std::vector<uint8_t> &signature,
                                        const nlohmann::json &jwk) {
    return verifySignatureEC(header_payload, signature, jwk, "ES256");
}

bool JWTValidator::verifySignatureEC(const std::string &header_payload, const std::vector<uint8_t> &signature,
                                     const nlohmann::json &jwk, const std::string &alg) {
    // Verify ECDSA signature for ES256 (P-256/SHA-256), ES384 (P-384/SHA-384),
    // or ES512 (P-521/SHA-512).
    // JWK format: {"kty":"EC","crv":"P-256"|"P-384"|"P-521","x":"...","y":"..."}
    if (jwk.value("kty", "") != "EC") {
        return false;
    }
    const std::string crv = jwk.value("crv", "");

    // Determine curve NID, expected coordinate size (bytes), and digest from alg.
    int nid           = 0;
    size_t coord_size = 0;
    const EVP_MD *md  = nullptr;

    if (alg == "ES256") {
        if (crv != "P-256") {
            return false;
        }
        nid        = NID_X9_62_prime256v1;
        coord_size = 32;
        md         = EVP_sha256();
    } else if (alg == "ES384") {
        if (crv != "P-384") {
            return false;
        }
        nid        = NID_secp384r1;
        coord_size = 48;
        md         = EVP_sha384();
    } else if (alg == "ES512") {
        if (crv != "P-521") {
            return false;
        }
        nid        = NID_secp521r1;
        coord_size = 66;
        md         = EVP_sha512();
    } else {
        return false;
    }

    auto x_b64 = jwk.value("x", "");
    auto y_b64 = jwk.value("y", "");
    if (x_b64.empty() || y_b64.empty()) {
        return false;
    }

    auto x_bytes = decodeBase64Url(x_b64);
    auto y_bytes = decodeBase64Url(y_b64);
    if (static_cast<int>(x_bytes.size()) != coord_size || static_cast<int>(y_bytes.size()) != coord_size) {
        return false;
    }

    // Build EC_KEY for the target curve.
    using ECKeyPtr   = std::unique_ptr<EC_KEY, decltype(&EC_KEY_free)>;
    using ECGroupPtr = std::unique_ptr<EC_GROUP, decltype(&EC_GROUP_free)>;
    using ECPointPtr = std::unique_ptr<EC_POINT, decltype(&EC_POINT_free)>;

    ECGroupPtr group(EC_GROUP_new_by_curve_name(nid), &EC_GROUP_free);
    if (!group) {
        return false;
    }

    ECKeyPtr ec_key(EC_KEY_new(), &EC_KEY_free);
    if (!ec_key) {
        return false;
    }
    if (EC_KEY_set_group(ec_key.get(), group.get()) != 1) {
        return false;
    }

    ECPointPtr pub_point(EC_POINT_new(group.get()), &EC_POINT_free);
    if (!pub_point) {
        return false;
    }

    auto x_bn = utils::BIGNUMPtr(BN_bin2bn(x_bytes.data(), (int)x_bytes.size(), nullptr));
    auto y_bn = utils::BIGNUMPtr(BN_bin2bn(y_bytes.data(), (int)y_bytes.size(), nullptr));
    if (!x_bn || !y_bn) {
        return false;
    }

    if (EC_POINT_set_affine_coordinates_GFp(group.get(), pub_point.get(), x_bn.get(), y_bn.get(), nullptr) != 1) {
        return false;
    }
    if (EC_KEY_set_public_key(ec_key.get(), pub_point.get()) != 1) {
        return false;
    }

    // Set EC_KEY into EVP_PKEY
    auto pkey = utils::make_evp_key();
    if (!pkey) {
        return false;
    }
    if (EVP_PKEY_set1_EC_KEY(pkey.get(), ec_key.get()) != 1) {
        return false;
    }

    // JWT ECDSA signature is raw (r || s) encoding (coord_size bytes each).
    // OpenSSL ECDSA_verify expects DER-encoded ECDSA_SIG.  Convert r||s → DER.
    if (static_cast<int>(signature.size()) != coord_size * 2) {
        return false;
    }

    using ECDSASIGPtr = std::unique_ptr<ECDSA_SIG, decltype(&ECDSA_SIG_free)>;
    ECDSASIGPtr ecdsa_sig(ECDSA_SIG_new(), &ECDSA_SIG_free);
    if (!ecdsa_sig) {
        return false;
    }

    auto r_bn = utils::BIGNUMPtr(BN_bin2bn(signature.data(), (int)coord_size, nullptr));
    auto s_bn = utils::BIGNUMPtr(BN_bin2bn(signature.data() + coord_size, (int)coord_size, nullptr));
    if (!r_bn || !s_bn) {
        return false;
    }

    // ECDSA_SIG_set0 takes ownership on success
    if (ECDSA_SIG_set0(ecdsa_sig.get(), r_bn.get(), s_bn.get()) != 1) {
        return false;
    }
    r_bn.release();
    s_bn.release();

    // Encode to DER into managed memory.
    int der_len = i2d_ECDSA_SIG(ecdsa_sig.get(), nullptr);
    if (der_len <= 0) {
        return false;
    }
    std::vector<unsigned char> der_buf(static_cast<size_t>(der_len));
    unsigned char *der_ptr = der_buf.data();
    int encoded_len        = i2d_ECDSA_SIG(ecdsa_sig.get(), &der_ptr);
    if (encoded_len != der_len) {
        return false;
    }

    // Verify using EVP_DigestVerify with the selected digest.
    auto mctx = utils::make_evp_md_ctx();
    if (!mctx) {
        return false;
    }
    if (EVP_DigestVerifyInit(mctx.get(), nullptr, md, nullptr, pkey.get()) != 1) {
        return false;
    }
    if (EVP_DigestVerifyUpdate(mctx.get(), header_payload.data(),static_cast<int>(header_payload.size())) != 1) {
        return false;
    }
    return EVP_DigestVerifyFinal(mctx.get(), der_buf.data(), static_cast<size_t>(der_len)) == 1;
}

bool JWTValidator::verifySignatureEdDSA(const std::string &header_payload, const std::vector<uint8_t> &signature,
                                        const nlohmann::json &jwk) {
    // JWK format: {"kty":"OKP","crv":"Ed25519","x":"<base64url-32-bytes>"}
    auto it_crv = jwk.find("crv");
    if (it_crv == jwk.end() || it_crv->get<std::string>() != "Ed25519") {
        return false;
    }

    auto it_x = jwk.find("x");
    if (it_x == jwk.end()) {
        return false;
    }
    auto pub_bytes = decodeBase64Url(it_x->get<std::string>());
    if (static_cast<int>(pub_bytes.size()) != 32) {
        return false; // Ed25519 public key is exactly 32 bytes
    }

    EVP_PKEY *raw_pkey = EVP_PKEY_new_raw_public_key(EVP_PKEY_ED25519, nullptr, pub_bytes.data(),static_cast<int>(pub_bytes.size()));
    if (!raw_pkey) {
        return false;
    }
    std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)> pkey(raw_pkey, EVP_PKEY_free);

    EVP_MD_CTX *raw_ctx = EVP_MD_CTX_new();
    if (!raw_ctx) {
        return false;
    }
    std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> ctx(raw_ctx, EVP_MD_CTX_free);

    // Ed25519 uses a single-pass DigestVerify with md=nullptr
    if (EVP_DigestVerifyInit(ctx.get(), nullptr, nullptr, nullptr, pkey.get()) != 1) {
        return false;
    }
    int result
        = EVP_DigestVerify(ctx.get(), signature.data(),static_cast<int>(signature.size()),
                           reinterpret_cast<const unsigned char *>(header_payload.data()),static_cast<int>(header_payload.size()));
    return result == 1;
}

bool JWTValidator::checkAudience(const nlohmann::json &payload) const {
    // Mandatory audience validation: if require_audience_validation is set, aud claim must be present
    if (cfg_.require_audience_validation) {
        if (!payload.contains("aud")) {
            return false;
        }
        // Check if expected_audience matches
        if (!cfg_.expected_audience.has_value()) {
            return false;
        }
        if (payload["aud"].is_string()) {
            return payload["aud"].get<std::string>() == *cfg_.expected_audience;
        }
        if (payload["aud"].is_array()) {
            for (auto &v : payload["aud"]) {
                if (v.is_string() && v.get<std::string>() == *cfg_.expected_audience) {
                    return true;
                }
            }
            return false;
        }
        return false;
    }
    
    // Optional audience validation: if expected_audience is set but not required
    if (!cfg_.expected_audience.has_value()) {
        return true;
    }
    if (!payload.contains("aud")) {
        return false;
    }
    if (payload["aud"].is_string()) {
        return payload["aud"].get<std::string>() == *cfg_.expected_audience;
    }
    if (payload["aud"].is_array()) {
        for (auto &v : payload["aud"]) {
            if (v.is_string() && v.get<std::string>() == *cfg_.expected_audience) {
                return true;
            }
        }
        return false;
    }
    return false;
}

void JWTValidator::setJWKSForTesting(const nlohmann::json &jwks, std::chrono::system_clock::time_point t) {
    std::unique_lock<std::shared_mutex> lock(jwks_cache_mutex_);
    jwks_cache_      = jwks;
    jwks_cache_time_ = t;
}

JWTClaims JWTValidator::parseAndValidate(const std::string &token) {
    std::string jwt = token;
    if (jwt.rfind("Bearer ", 0) == 0) {
        jwt = jwt.substr(7);
    }

    // Input validation: Check token size limit
    if (static_cast<int>(jwt.size()) > MAX_JWT_TOKEN_SIZE) {
        utils::Logger::warn("JWT validation failed: Token exceeds maximum size");
        if (audit_logger_) {
            audit_logger_->logSecurityEvent(utils::SecurityEventType::LOGIN_FAILED, "", "jwt/token",
                                            {{"reason", "token_too_large"}});
        }
        throw std::runtime_error("Token exceeds maximum size limit");
    }

    // Input validation: Check for empty token
    if (jwt.empty()) {
        utils::Logger::warn("JWT validation failed: Empty token");
        if (audit_logger_) {
            audit_logger_->logSecurityEvent(utils::SecurityEventType::LOGIN_FAILED, "", "jwt/token",
                                            {{"reason", "empty_token"}});
        }
        throw std::runtime_error("Empty token");
    }

    std::vector<std::string> parts;
    std::stringstream ss(jwt);
    std::string part = {};
    while (std::getline(ss, part, '.')) {
        parts.push_back(part);
    }
    if (static_cast<int>(parts.size()) != 3) {
        utils::Logger::warn("JWT validation failed: Invalid format (expected 3 parts)");
        if (audit_logger_) {
            audit_logger_->logSecurityEvent(utils::SecurityEventType::LOGIN_FAILED, "", "jwt/token",
                                            {{"reason", "invalid_format"}});
        }
        throw std::runtime_error("Invalid JWT format (expected 3 parts)");
    }
    auto header_json  = decodeBase64UrlToString(parts[0]);
    auto payload_json = decodeBase64UrlToString(parts[1]);
    auto header       = nlohmann::json::parse(header_json);
    auto payload      = nlohmann::json::parse(payload_json);
    std::string alg   = header.value("alg", "");
    std::string kid   = header.value("kid", "");

    // Check algorithm - support RS256/RS384/RS512, ES256/ES384/ES512, and EdDSA
    if (alg != "RS256" && alg != "RS384" && alg != "RS512" && alg != "ES256" && alg != "ES384" && alg != "ES512"
        && alg != "EdDSA") {
        utils::Logger::warn("JWT validation failed: Unsupported algorithm: " + alg);
        if (audit_logger_) {
            audit_logger_->logSecurityEvent(utils::SecurityEventType::LOGIN_FAILED, "", "jwt/token",
                                            {{"reason", "unsupported_algorithm"}, {"alg", alg}});
        }
        throw std::runtime_error("Unsupported alg: " + alg
                                 + " (supported: RS256, RS384, RS512, ES256, ES384, ES512, EdDSA)");
    }

    // Check kid revocation
    if (!kid.empty() && isKidRevoked(kid)) {
        utils::Logger::warn("JWT validation failed: Revoked kid: " + kid);
        if (audit_logger_) {
            audit_logger_->logSecurityEvent(utils::SecurityEventType::LOGIN_FAILED, "", "jwt/token",
                                            {{"reason", "revoked_kid"}, {"kid", kid}});
        }
        throw std::runtime_error("Token signed with revoked key (kid: " + kid + ")");
    }

    JWTClaims claims;
    claims.sub = payload.value("sub", "");

    // Input validation: Check principal/subject length
    if (static_cast<int>(claims.sub.size()) > MAX_PRINCIPAL_NAME_LENGTH) {
        utils::Logger::warn("JWT validation failed: Subject exceeds maximum length");
        if (audit_logger_) {
            audit_logger_->logSecurityEvent(utils::SecurityEventType::LOGIN_FAILED, "", "jwt/token",
                                            {{"reason", "subject_too_long"}});
        }
        throw std::runtime_error("Subject (principal) exceeds maximum length");
    }

    claims.email = payload.value("email", "");
    claims.jti   = payload.value("jti", ""); // JWT ID – used for per-token revocation
    if (cfg_.require_jti && claims.jti.empty()) {
        utils::Logger::warn("JWT validation failed: Missing required jti claim");
        if (audit_logger_) {
            // Use empty subject: token is not yet signature-verified so claims.sub is untrusted
            audit_logger_->logSecurityEvent(utils::SecurityEventType::LOGIN_FAILED, "", "jwt/token",
                                            {{"reason", "missing_jti"}});
        }
        throw std::runtime_error("Missing required jti claim");
    }
    claims.tenant_id = payload.value("tenant_id", ""); // Extract tenant_id from JWT
    claims.issuer    = payload.value("iss", "");
    
    // Mandatory issuer validation: if require_issuer_validation is set, iss claim must be present
    if (cfg_.require_issuer_validation && claims.issuer.empty()) {
        utils::Logger::warn("JWT validation failed: Missing required iss claim");
        if (audit_logger_) {
            audit_logger_->logSecurityEvent(utils::SecurityEventType::LOGIN_FAILED, claims.sub, "jwt/token",
                                            {{"reason", "missing_iss"}});
        }
        throw std::runtime_error("Missing required iss claim");
    }
    
    if (payload.contains("groups")) {
        claims.groups = payload["groups"].get<std::vector<std::string>>();
    }
    if (payload.contains("roles")) {
        claims.roles = payload["roles"].get<std::vector<std::string>>();
    }
    // Extract OAuth2 scopes from `scope` (space-separated string) or `scp` (array).
    // Some IdPs (Keycloak, Auth0) use `scope`; Azure AD uses `scp`.
    if (payload.contains("scope") && payload["scope"].is_string()) {
        const std::string scope_str = payload["scope"].get<std::string>();
        std::istringstream iss(scope_str);
        std::string token_item = {};
        while (iss >> token_item) {
            if (!token_item.empty()) {
                claims.scopes.push_back(token_item);
            }
        }
    } else if (payload.contains("scp")) {
        if (payload["scp"].is_array()) {
            claims.scopes = payload["scp"].get<std::vector<std::string>>();
        } else if (payload["scp"].is_string()) {
            // Some implementations use space-separated string in scp as well
            const std::string scp_str = payload["scp"].get<std::string>();
            std::istringstream iss(scp_str);
            std::string token_item = {};
            while (iss >> token_item) {
                if (!token_item.empty()) {
                    claims.scopes.push_back(token_item);
                }
            }
        }
    }
    auto now = std::chrono::system_clock::now();
    if (payload.contains("exp")) {
        int64_t exp       = payload["exp"].get<int64_t>();
        claims.expiration = std::chrono::system_clock::time_point{std::chrono::seconds{exp}};
    } else {
        utils::Logger::warn("JWT validation failed: Missing exp claim");
        if (audit_logger_) {
            audit_logger_->logSecurityEvent(utils::SecurityEventType::LOGIN_FAILED, claims.sub, "jwt/token",
                                            {{"reason", "missing_exp"}});
        }
        throw std::runtime_error("Missing exp claim");
    }
    if (payload.contains("nbf")) {
        int64_t nbf       = payload["nbf"].get<int64_t>();
        claims.not_before = std::chrono::system_clock::time_point{std::chrono::seconds{nbf}};
        if (now + cfg_.clock_skew < *claims.not_before) {
            utils::Logger::warn("JWT validation failed: Token not yet valid (nbf)");
            if (audit_logger_) {
                audit_logger_->logSecurityEvent(utils::SecurityEventType::LOGIN_FAILED, claims.sub, "jwt/token",
                                                {{"reason", "not_yet_valid"}});
            }
            throw std::runtime_error("Token not yet valid (nbf)");
        }
    }
    if (payload.contains("iat")) {
        int64_t iat      = payload["iat"].get<int64_t>();
        claims.issued_at = std::chrono::system_clock::time_point{std::chrono::seconds{iat}};
        if (now + cfg_.clock_skew < *claims.issued_at) {
            utils::Logger::warn("JWT validation failed: iat in future");
            if (audit_logger_) {
                audit_logger_->logSecurityEvent(utils::SecurityEventType::LOGIN_FAILED, claims.sub, "jwt/token",
                                                {{"reason", "iat_in_future"}});
            }
            throw std::runtime_error("iat in future");
        }
    }
    if (payload.contains("aud")) {
        if (payload["aud"].is_string()) {
            claims.audience.push_back(payload["aud"].get<std::string>());
        } else if (payload["aud"].is_array()) {
            for (auto &v : payload["aud"]) {
                if (v.is_string()) {
                    claims.audience.push_back(v.get<std::string>());
                }
            }
        }
    }
    if (claims.isExpired() && now > claims.expiration + cfg_.clock_skew) {
        utils::Logger::warn("JWT validation failed: Token expired");
        if (audit_logger_) {
            audit_logger_->logSecurityEvent(utils::SecurityEventType::LOGIN_FAILED, claims.sub, "jwt/token",
                                            {{"reason", "token_expired"}});
        }
        throw std::runtime_error("Token expired");
    }
    if (cfg_.expected_issuer.has_value() && claims.issuer != *cfg_.expected_issuer) {
        utils::Logger::warn("JWT validation failed: Issuer mismatch (expected: " + *cfg_.expected_issuer
                            + ", got: " + claims.issuer + ")");
        if (audit_logger_) {
            audit_logger_->logSecurityEvent(utils::SecurityEventType::LOGIN_FAILED, claims.sub, "jwt/token",
                                            {{"reason", "issuer_mismatch"}, {"issuer", claims.issuer}});
        }
        throw std::runtime_error("Issuer mismatch");
    }
    if (!checkAudience(payload)) {
        utils::Logger::warn("JWT validation failed: Audience mismatch");
        if (audit_logger_) {
            audit_logger_->logSecurityEvent(utils::SecurityEventType::LOGIN_FAILED, claims.sub, "jwt/token",
                                            {{"reason", "audience_mismatch"}});
        }
        throw std::runtime_error("Audience mismatch");
    }
    auto jwks                  = fetchJWKS();
    auto sig_bytes             = decodeBase64Url(parts[2]);
    std::string header_payload = parts[0] + "." + parts[1];
    const nlohmann::json *jwk  = nullptr;
    if (!kid.empty()) {
        jwk = findJwkForKid(jwks, kid);
        if (!jwk) {
            // Force cache expiry under the write lock so other threads see the invalidation
            {
                std::unique_lock<std::shared_mutex> lock(jwks_cache_mutex_);
                jwks_cache_time_ = std::chrono::system_clock::time_point::min();
            }
            jwks = fetchJWKS();
            jwk  = findJwkForKid(jwks, kid);
        }
    }
    if (!jwk) {
        utils::Logger::warn("JWT validation failed: JWK not found for kid: " + kid);
        if (audit_logger_) {
            audit_logger_->logSecurityEvent(utils::SecurityEventType::LOGIN_FAILED, claims.sub, "jwt/token",
                                            {{"reason", "jwk_not_found"}, {"kid", kid}});
        }
        throw std::runtime_error("JWK not found for kid");
    }
    bool sig_ok = false;
    if (alg == "RS256" || alg == "RS384" || alg == "RS512") {
        sig_ok = verifySignatureRSA(header_payload, sig_bytes, *jwk, alg);
    } else if (alg == "ES256" || alg == "ES384" || alg == "ES512") {
        sig_ok = verifySignatureEC(header_payload, sig_bytes, *jwk, alg);
    } else if (alg == "EdDSA") {
        sig_ok = verifySignatureEdDSA(header_payload, sig_bytes, *jwk);
    }
    if (!sig_ok) {
        utils::Logger::warn("JWT validation failed: Signature verification failed for kid: " + kid);
        if (audit_logger_) {
            audit_logger_->logSecurityEvent(utils::SecurityEventType::LOGIN_FAILED, claims.sub, "jwt/token",
                                            {{"reason", "signature_invalid"}, {"kid", kid}});
        }
        throw std::runtime_error("Signature verification failed");
    }
    // Per-token revocation check: reject if the JTI is in the blacklist
    if (token_blacklist_ && claims.jti.empty()) {
        // Warn once per validator lifecycle to avoid per-request log flooding
        bool already_warned = warned_blacklist_no_jti_.exchange(true, std::memory_order_relaxed);
        if (!already_warned) {
            utils::Logger::warn("JWT has no jti; per-token revocation impossible for this token");
        }
    }
    if (token_blacklist_ && !claims.jti.empty() && token_blacklist_->isRevoked(claims.jti)) {
        utils::Logger::warn("JWT validation failed: Token revoked (jti: " + claims.jti + ")");
        if (audit_logger_) {
            audit_logger_->logSecurityEvent(utils::SecurityEventType::LOGIN_FAILED, claims.sub,
                                            "jwt/token/" + claims.jti,
                                            {{"reason", "token_revoked"}, {"jti", claims.jti}});
        }
        throw std::runtime_error("Token has been revoked");
    }
    if (audit_logger_) {
        nlohmann::json d;
        d["jti"]    = claims.jti;
        d["issuer"] = claims.issuer;
        d["kid"]    = kid;
        audit_logger_->logSecurityEvent(utils::SecurityEventType::LOGIN_SUCCESS, claims.sub, "jwt/token", d);
    }
    return claims;
}

std::vector<uint8_t> JWTValidator::deriveUserKey(const std::vector<uint8_t> &dek, const JWTClaims &claims,
                                                 const std::string &field_name) {
    std::vector<uint8_t> salt(claims.sub.begin(), claims.sub.end());
    std::string info = "user-field:" + field_name;
    return themis::utils::HKDFHelper::derive(dek, salt, info, 32);
}

bool JWTValidator::hasAccess(const JWTClaims &claims, const std::string &encryption_context) {
    if (claims.sub == encryption_context) {
        return true;
    }
    for (const auto &group : claims.groups) {
        if (group == encryption_context) {
            return true;
        }
    }
    return false;
}

void JWTValidator::setTokenBlacklist(TokenBlacklist *bl) {
    token_blacklist_ = bl;
}

void JWTValidator::revokeKid(const std::string &kid) {
    revoked_kids_runtime_.push_back(kid);
    utils::Logger::info("JWT kid revoked: " + kid);
}

bool JWTValidator::isKidRevoked(const std::string &kid) const {
    // Check config denylist
    for (const auto &revoked : cfg_.revoked_kids) {
        if (revoked == kid) {
            return true;
        }
    }
    // Check runtime denylist
    for (const auto &revoked : revoked_kids_runtime_) {
        if (revoked == kid) {
            return true;
        }
    }
    return false;
}

std::future<JWTClaims> JWTValidator::validateAsync(const std::string &token) {
    // Dispatch parseAndValidate() — which includes any JWKS refresh — to the
    // worker pool so the caller's thread is never blocked by network I/O.
    // The exponential back-off sleep in fetchJWKS() executes on the worker
    // thread, not on the caller's thread.
    return worker_pool_->submit([this, token]() { return this->parseAndValidate(token); });
}

} // namespace auth
} // namespace themis

