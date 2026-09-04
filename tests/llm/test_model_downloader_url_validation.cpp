/*
 * Wave 2-C: insecure_model_url gap closure tests.
 *
 * Verifies that ModelDownloader::pullFromOllama() and related API entry points
 * reject unsafe Ollama endpoint URLs before making any outbound CURL request.
 *
 * Test IDs: URL_VAL_01..07
 *
 * These tests exercise ModelDownloadConfig/ModelDownloader through a thin
 * white-box path: we call the public download API with a deliberately
 * unreachable URL and assert the returned error codes — rather than trying to
 * mock libcurl — so they remain network-independent and fast.
 */

#include <gtest/gtest.h>
#include "llm/model_downloader.h"
#include <string>

namespace themis::llm {

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

static ModelDownloadResult attemptPull(const std::string& url) {
    ModelDownloadConfig cfg;
    cfg.ollama_url      = url;
    cfg.model_name      = "test-model";
    cfg.download_dir    = "/tmp/themis_url_val_test";
    cfg.timeout_seconds = 1;   // 1 s — will fail fast if URL slips through
    cfg.use_cache       = false;

    ModelDownloader dl = {};
    return dl.downloadFromOllama(cfg);
}

static ModelDownloadResult attemptPullInsecure(const std::string& url) {
    ModelDownloadConfig cfg;
    cfg.ollama_url           = url;
    cfg.model_name           = "test-model";
    cfg.download_dir         = "/tmp/themis_url_val_test";
    cfg.timeout_seconds      = 1;
    cfg.use_cache            = false;
    cfg.allow_insecure_http  = true;  // explicit opt-in for testing

    ModelDownloader dl = {};
    return dl.downloadFromOllama(cfg);
}

static std::vector<std::string> listModels(const std::string& url) {
    return ModelDownloader::listOllamaModels(url);
}

// ─────────────────────────────────────────────────────────────────────────────
// URL_VAL_01: empty URL is rejected
// ─────────────────────────────────────────────────────────────────────────────
TEST(ModelDownloaderUrlValidation, URL_VAL_01_EmptyUrlRejected) {
    const auto result = attemptPull("");
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error_message.empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// URL_VAL_02: file:// scheme is rejected
// ─────────────────────────────────────────────────────────────────────────────
TEST(ModelDownloaderUrlValidation, URL_VAL_02_FileSchemeRejected) {
    const auto result = attemptPull("file:///etc/passwd");
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error_message.empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// URL_VAL_03: ftp:// scheme is rejected
// ─────────────────────────────────────────────────────────────────────────────
TEST(ModelDownloaderUrlValidation, URL_VAL_03_FtpSchemeRejected) {
    const auto result = attemptPull("ftp://malicious.example.com/model");
    EXPECT_FALSE(result.success);
}

// ─────────────────────────────────────────────────────────────────────────────
// URL_VAL_04: URL with embedded credentials (userinfo@host) is rejected
// ─────────────────────────────────────────────────────────────────────────────
TEST(ModelDownloaderUrlValidation, URL_VAL_04_EmbeddedCredentialsRejected) {
    // Credential injection pattern: http://userinfo@host/
    const auto result = attemptPull("http://user@localhost:11434");
    EXPECT_FALSE(result.success);
}

// ─────────────────────────────────────────────────────────────────────────────
// URL_VAL_04b: HTTPS with embedded credentials is also rejected
// ─────────────────────────────────────────────────────────────────────────────
TEST(ModelDownloaderUrlValidation, URL_VAL_04b_HttpsEmbeddedCredentialsRejected) {
    const auto result = attemptPull("https://user@ollama.example.com:11434");
    EXPECT_FALSE(result.success);
}

// ─────────────────────────────────────────────────────────────────────────────
// URL_VAL_05: plain http:// non-localhost is REJECTED by default (W3-SEC-01)
//   ModelDownloadConfig::allow_insecure_http defaults to false; non-local HTTP
//   must return the "Invalid ollama_url" error to prevent MITM weight injection.
// ─────────────────────────────────────────────────────────────────────────────
TEST(ModelDownloaderUrlValidation, URL_VAL_05_PlainHttpNonLocalRejectedByDefault) {
    const auto result = attemptPull("http://192.0.2.1:11434");  // TEST-NET-1 (RFC 5737)
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error_message.empty());
    EXPECT_NE(result.error_message.find("Invalid ollama_url"), std::string::npos)
        << "Non-local plain HTTP must be rejected by URL validator when "
           "allow_insecure_http is false; got: " << result.error_message;
}

// ─────────────────────────────────────────────────────────────────────────────
// URL_VAL_06: http://localhost is accepted (common dev setup)
// ─────────────────────────────────────────────────────────────────────────────
TEST(ModelDownloaderUrlValidation, URL_VAL_06_HttpLocalhostAccepted) {
    // Port 1 is reserved; connection will be refused instantly — what matters
    // is that the validator passes and CURL fires (error != validator error).
    const auto result = attemptPull("http://localhost:1");
    if (!result.success) {
        EXPECT_TRUE(result.error_message.find("Invalid ollama_url") == std::string::npos)
            << "Localhost HTTP should pass the validator; got: " << result.error_message;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// URL_VAL_07: https:// URL is accepted
// ─────────────────────────────────────────────────────────────────────────────
TEST(ModelDownloaderUrlValidation, URL_VAL_07_HttpsAccepted) {
    const auto result = attemptPull("https://192.0.2.1:11434");
    if (!result.success) {
        EXPECT_TRUE(result.error_message.find("Invalid ollama_url") == std::string::npos)
            << "HTTPS URL should pass the validator; got: " << result.error_message;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// URL_VAL_08: listOllamaModels() also rejects invalid scheme
// ─────────────────────────────────────────────────────────────────────────────
TEST(ModelDownloaderUrlValidation, URL_VAL_08_ListModelsRejectsInvalidScheme) {
    const auto models = listModels("ftp://internal-store/api/tags");
    EXPECT_TRUE(models.empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// URL_VAL_09: listOllamaModels() also rejects embedded credentials
// ─────────────────────────────────────────────────────────────────────────────
TEST(ModelDownloaderUrlValidation, URL_VAL_09_ListModelsRejectsCredentials) {
    const auto models = listModels("http://user@localhost:11434");
    EXPECT_TRUE(models.empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// URL_VAL_10: plain http:// non-localhost is accepted when allow_insecure_http=true
//   Confirms the explicit opt-in path works (non-production controlled networks).
// ─────────────────────────────────────────────────────────────────────────────
TEST(ModelDownloaderUrlValidation, URL_VAL_10_PlainHttpNonLocalAllowedWithExplicitFlag) {
    const auto result = attemptPullInsecure("http://192.0.2.1:11434");  // TEST-NET-1
    // Validator should pass; failure (if any) must come from CURL — not the URL check.
    if (!result.success) {
        EXPECT_EQ(result.error_message.find("Invalid ollama_url"), std::string::npos)
            << "URL validator should not reject non-local HTTP when allow_insecure_http=true; "
               "got: " << result.error_message;
    }
}

} // namespace themis::llm
