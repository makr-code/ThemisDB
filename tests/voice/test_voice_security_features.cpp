/**
 * @file test_voice_security_features.cpp
 * @brief Focused tests for voice module Q2 2026 security features:
 *   - SSML injection sanitization (VoiceTTSCustomizer::sanitizeSSML / isSSMLSafe)
 *   - Path traversal protection (VoiceModelCache::isSafeModelPath)
 *   - WebRTC origin allowlist (VoiceStreamingSession::checkOrigin)
 */

#include <gtest/gtest.h>
#include "voice/voice_tts_customizer.h"
#include "voice/voice_model_cache.h"
#include "voice/voice_browser_streaming.h"

// Provide stub definitions for Logger static members so the test binary links
// without the full themis_core library.  The templates check `if (logger_)`
// before dereferencing, so a null shared_ptr produces safe no-ops.
#if defined(_WIN32) && !defined(THEMIS_BASE_EXPORTS)
#define THEMIS_BASE_EXPORTS
#endif
#include "utils/logger.h"

using namespace themis::voice;

// =============================================================================
// SSML Injection Sanitization – VoiceTTSCustomizer::sanitizeSSML
// =============================================================================

class SSMLSanitizationTest : public ::testing::Test {
protected:
    VoiceTTSCustomizer customizer;
};

TEST_F(SSMLSanitizationTest, CleanSSMLPassesThrough) {
    std::string input = "<speak><p>Hello world.</p></speak>";
    auto result = customizer.sanitizeSSML(input);
    EXPECT_FALSE(result.had_injection_attempt);
    EXPECT_TRUE(result.rejected_tags.empty());
    // Clean input should round-trip intact
    EXPECT_EQ(result.sanitized_text, input);
}

TEST_F(SSMLSanitizationTest, AllowedProsodyTagKept) {
    std::string input = R"(<prosody rate="1.2" pitch="0.9">Fast speech.</prosody>)";
    auto result = customizer.sanitizeSSML(input);
    EXPECT_FALSE(result.had_injection_attempt);
    EXPECT_NE(result.sanitized_text.find("<prosody"), std::string::npos);
}

TEST_F(SSMLSanitizationTest, AllowedBreakTagKept) {
    std::string input = R"(<break time="500ms"/>)";
    auto result = customizer.sanitizeSSML(input);
    EXPECT_FALSE(result.had_injection_attempt);
    EXPECT_NE(result.sanitized_text.find("<break"), std::string::npos);
}

TEST_F(SSMLSanitizationTest, AllowedEmphasisTagKept) {
    std::string input = R"(<emphasis level="strong">Important!</emphasis>)";
    auto result = customizer.sanitizeSSML(input);
    EXPECT_FALSE(result.had_injection_attempt);
    EXPECT_NE(result.sanitized_text.find("<emphasis"), std::string::npos);
}

TEST_F(SSMLSanitizationTest, ScriptTagIsRejected) {
    std::string input = "<script>alert('xss')</script>";
    auto result = customizer.sanitizeSSML(input);
    EXPECT_TRUE(result.had_injection_attempt);
    // Sanitized output must NOT contain the script tag
    EXPECT_EQ(result.sanitized_text.find("<script"), std::string::npos);
    // Tag name should appear in rejected list
    EXPECT_EQ(std::count(result.rejected_tags.begin(), result.rejected_tags.end(), "script"), 1);
}

TEST_F(SSMLSanitizationTest, HTMLStyleTagIsRejected) {
    std::string input = "<style>body{display:none}</style>";
    auto result = customizer.sanitizeSSML(input);
    EXPECT_TRUE(result.had_injection_attempt);
    EXPECT_EQ(result.sanitized_text.find("<style"), std::string::npos);
}

TEST_F(SSMLSanitizationTest, IframeTagIsRejected) {
    std::string input = "<iframe src=\"evil.com\"/>";
    auto result = customizer.sanitizeSSML(input);
    EXPECT_TRUE(result.had_injection_attempt);
    EXPECT_EQ(result.sanitized_text.find("<iframe"), std::string::npos);
}

TEST_F(SSMLSanitizationTest, UnknownAttributeIsStripped) {
    // 'onmouseover' is not in the prosody allowlist
    std::string input = R"DEL(<prosody rate="1.0" onmouseover="evil()">text</prosody>)DEL";
    auto result = customizer.sanitizeSSML(input);
    EXPECT_TRUE(result.had_injection_attempt);
    // The tag itself should still be present
    EXPECT_NE(result.sanitized_text.find("<prosody"), std::string::npos);
    // But the injected attribute must be gone
    EXPECT_EQ(result.sanitized_text.find("onmouseover"), std::string::npos);
}

TEST_F(SSMLSanitizationTest, AttributeValueWithAngleBracketsIsRejected) {
    std::string input = R"(<prosody rate="<bad>">text</prosody>)";
    auto result = customizer.sanitizeSSML(input);
    EXPECT_TRUE(result.had_injection_attempt);
}

TEST_F(SSMLSanitizationTest, AttributeValueWithScriptKeywordIsRejected) {
    std::string input = R"DEL(<prosody rate="javascript:void(0)">text</prosody>)DEL";
    auto result = customizer.sanitizeSSML(input);
    EXPECT_TRUE(result.had_injection_attempt);
}

TEST_F(SSMLSanitizationTest, PlainTextPassesThrough) {
    std::string input = "Hello, how are you?";
    auto result = customizer.sanitizeSSML(input);
    EXPECT_FALSE(result.had_injection_attempt);
    EXPECT_EQ(result.sanitized_text, input);
}

TEST_F(SSMLSanitizationTest, MixedAllowedAndDisallowedTags) {
    std::string input = "<speak><script>bad</script><p>good</p></speak>";
    auto result = customizer.sanitizeSSML(input);
    EXPECT_TRUE(result.had_injection_attempt);
    // speak and p must survive; script must be stripped
    EXPECT_NE(result.sanitized_text.find("<speak>"), std::string::npos);
    EXPECT_NE(result.sanitized_text.find("<p>"), std::string::npos);
    EXPECT_EQ(result.sanitized_text.find("<script"), std::string::npos);
}

TEST_F(SSMLSanitizationTest, IsSSMLSafeReturnsTrueForCleanInput) {
    EXPECT_TRUE(customizer.isSSMLSafe("<speak><p>OK</p></speak>"));
}

TEST_F(SSMLSanitizationTest, IsSSMLSafeReturnsFalseForMalicious) {
    EXPECT_FALSE(customizer.isSSMLSafe("<script>alert(1)</script>"));
}

// =============================================================================
// Path Traversal Protection – VoiceModelCache::isSafeModelPath
// =============================================================================

TEST(ModelCachePathTest, SafeRelativePathAccepted) {
    EXPECT_TRUE(VoiceModelCache::isSafeModelPath("models/whisper-base.bin"));
}

TEST(ModelCachePathTest, SafeAbsolutePathAccepted) {
    EXPECT_TRUE(VoiceModelCache::isSafeModelPath("/opt/models/whisper.bin"));
}

TEST(ModelCachePathTest, EmptyPathRejected) {
    EXPECT_FALSE(VoiceModelCache::isSafeModelPath(""));
}

TEST(ModelCachePathTest, DotDotTraversalRejected) {
    EXPECT_FALSE(VoiceModelCache::isSafeModelPath("../../etc/passwd"));
}

TEST(ModelCachePathTest, EmbeddedDotDotRejected) {
    EXPECT_FALSE(VoiceModelCache::isSafeModelPath("/opt/models/../../../etc/shadow"));
}

TEST(ModelCachePathTest, NullByteRejected) {
    std::string path = "/opt/models/model.bin";
    path.push_back('\0');
    path += ".evil";
    EXPECT_FALSE(VoiceModelCache::isSafeModelPath(path));
}

TEST(ModelCachePathTest, SemicolonShellMetacharRejected) {
    EXPECT_FALSE(VoiceModelCache::isSafeModelPath("/models/good.bin;rm -rf /"));
}

TEST(ModelCachePathTest, PipeShellMetacharRejected) {
    EXPECT_FALSE(VoiceModelCache::isSafeModelPath("/models/good.bin|cat /etc/passwd"));
}

TEST(ModelCachePathTest, BacktickShellMetacharRejected) {
    EXPECT_FALSE(VoiceModelCache::isSafeModelPath("/models/`id`"));
}

TEST(ModelCachePathTest, DollarShellMetacharRejected) {
    EXPECT_FALSE(VoiceModelCache::isSafeModelPath("/models/$HOME/.ssh/key"));
}

TEST(ModelCachePathTest, PathWithSpaceAccepted) {
    // Spaces are legitimate in paths
    EXPECT_TRUE(VoiceModelCache::isSafeModelPath("/opt/my models/whisper.bin"));
}

// Verify that get() rejects traversal paths even when a loader is registered
TEST(ModelCachePathTest, GetRejectsUnsafePathWithoutLoader) {
    VoiceModelCache cache;
    cache.registerLoader("stt",
        [](const std::string&, const nlohmann::json&) -> void* { return nullptr; },
        [](void*) {});
    auto result = cache.get("id1", "../../etc/passwd", "stt");
    EXPECT_FALSE(result.has_value());
}

// =============================================================================
// WebRTC Origin Allowlist – VoiceStreamingSession::checkOrigin
// =============================================================================

static VoiceStreamingSession::Config makeConfig() {
    VoiceStreamingSession::Config cfg;
    cfg.session_id      = "test-session";
    cfg.user_id         = "test-user";
    cfg.max_frame_bytes = 65536;
    cfg.max_duration_s  = 600;
    return cfg;
}

TEST(OriginAllowlistTest, EmptyAllowlistPermitsAnyOrigin) {
    auto cfg = makeConfig();
    // origin_allowlist is empty by default
    auto session = VoiceStreamingSession::create(cfg);
    EXPECT_TRUE(session->checkOrigin("https://anything.example.com"));
    EXPECT_TRUE(session->checkOrigin("https://evil.example.com"));
}

TEST(OriginAllowlistTest, NonEmptyAllowlistPermitsListedOrigin) {
    auto cfg = makeConfig();
    cfg.origin_allowlist = {"https://app.example.com", "https://admin.example.com"};
    auto session = VoiceStreamingSession::create(cfg);
    EXPECT_TRUE(session->checkOrigin("https://app.example.com"));
    EXPECT_TRUE(session->checkOrigin("https://admin.example.com"));
}

TEST(OriginAllowlistTest, NonEmptyAllowlistBlocksUnlistedOrigin) {
    auto cfg = makeConfig();
    cfg.origin_allowlist = {"https://app.example.com"};
    auto session = VoiceStreamingSession::create(cfg);
    EXPECT_FALSE(session->checkOrigin("https://evil.example.com"));
    EXPECT_FALSE(session->checkOrigin("http://app.example.com")); // wrong scheme
    EXPECT_FALSE(session->checkOrigin("https://app.example.com:8443")); // wrong port
}

TEST(OriginAllowlistTest, ComparisonIsCaseSensitive) {
    auto cfg = makeConfig();
    cfg.origin_allowlist = {"https://App.Example.COM"};
    auto session = VoiceStreamingSession::create(cfg);
    EXPECT_TRUE(session->checkOrigin("https://App.Example.COM"));
    EXPECT_FALSE(session->checkOrigin("https://app.example.com")); // different case
}

TEST(OriginAllowlistTest, EmptyOriginStringBlockedWhenAllowlistSet) {
    auto cfg = makeConfig();
    cfg.origin_allowlist = {"https://app.example.com"};
    auto session = VoiceStreamingSession::create(cfg);
    EXPECT_FALSE(session->checkOrigin(""));
}

TEST(OriginAllowlistTest, SingleWildcardStyleNotSupported) {
    // The implementation requires exact matches; "*" is NOT treated as wildcard
    auto cfg = makeConfig();
    cfg.origin_allowlist = {"*"};
    auto session = VoiceStreamingSession::create(cfg);
    EXPECT_FALSE(session->checkOrigin("https://app.example.com"));
    EXPECT_TRUE(session->checkOrigin("*")); // literal match only
}

TEST(OriginAllowlistTest, SessionStartsAndAudioFlowsWithAllowedOrigin) {
    auto cfg = makeConfig();
    cfg.origin_allowlist = {"https://trusted.example.com"};
    auto session = VoiceStreamingSession::create(cfg);
    ASSERT_TRUE(session->checkOrigin("https://trusted.example.com"));
    // Session should start and accept audio normally
    auto id = session->start();
    EXPECT_FALSE(id.empty());
    EXPECT_TRUE(session->isActive());
    std::vector<uint8_t> chunk(1024, 0);
    auto pt = session->sendAudioChunk(chunk);
    EXPECT_EQ(session->bytesReceived(), 1024u);
    session->end();
}
