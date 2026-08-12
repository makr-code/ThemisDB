/**
 * @file test_voice_coverage.cpp
 * @brief Additional unit tests to push voice module coverage above 80%.
 *
 * Covers the following previously untested public methods:
 *  - VoiceSessionManager:     touchSession, expireOldSessions, getSessionState
 *  - VoiceAudioStorage:       checkDuplicate, computeTier, promoteTier
 *  - VoiceMeetingSupport:     isCompliantForJurisdiction
 *  - VoiceFallbackStrategy:   ttsFallback
 *  - VoiceSecurityManager:    hasTranscriptionConsent, logEvent
 *  - VoiceTTSCustomizer:      getSupportedLanguages, registerLanguageVoice
 *  - ConversationContext:      turnCount, hasEntity
 *  - VoiceIntentDetector:     stringToIntent, detect (with context)
 *  - VoiceAccessibility:      mergeSortCues
 */

#include <gtest/gtest.h>
#include "voice/voice_session_manager.h"
#include "voice/voice_audio_storage.h"
#include "voice/voice_meeting_support.h"
#include "voice/voice_error_handler.h"
#include "voice/voice_security.h"
#include "voice/voice_tts_customizer.h"
#include "voice/voice_intent_detector.h"
#include "voice/voice_accessibility.h"

#include <thread>
#include <chrono>

using namespace themis::voice;

// ===========================================================================
// VoiceSessionManager – untested methods
// ===========================================================================

TEST(VoiceSessionManagerCoverage, TouchSessionUpdatesActivity) {
    VoiceSessionManager mgr;
    auto session = mgr.createSession("touch_user");
    int64_t before = session.last_activity_ms;

    // Small sleep to ensure the timestamp advances
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    bool ok = mgr.touchSession(session.session_id);
    EXPECT_TRUE(ok);

    auto updated = mgr.getSession(session.session_id);
    ASSERT_TRUE(updated.has_value());
    EXPECT_GE(updated->last_activity_ms, before);
}

TEST(VoiceSessionManagerCoverage, TouchSessionReturnsFalseForUnknownId) {
    VoiceSessionManager mgr;
    EXPECT_FALSE(mgr.touchSession("does_not_exist"));
}

TEST(VoiceSessionManagerCoverage, ExpireOldSessionsReturnsCountOfExpired) {
    // Configure a very short idle timeout so sessions expire immediately.
    SessionTimeoutConfig cfg;
    cfg.idle_timeout_ms = 1;            // 1 ms idle → expires after 1 ms
    cfg.max_session_duration_ms = 60000; // 1 minute max
    cfg.auto_expire = true;

    VoiceSessionManager mgr(cfg);
    // Small sleep between creates to guarantee distinct session IDs
    // (generateSessionId seeds the RNG from the current millisecond).
    mgr.createSession("expire_user_1");
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    mgr.createSession("expire_user_2");

    // Count how many sessions are currently active before expiry
    size_t before = mgr.getAnalytics().active_sessions;
    ASSERT_GE(before, 1u);

    // Wait long enough for the idle timeout to fire on all sessions
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    size_t expired = mgr.expireOldSessions();
    // All sessions that were active before must now be expired
    EXPECT_EQ(expired, before);
}

TEST(VoiceSessionManagerCoverage, ExpireOldSessionsDoesNotExpireActiveSession) {
    VoiceSessionManager mgr;   // default 5-min idle timeout
    auto session = mgr.createSession("active_user");

    size_t expired = mgr.expireOldSessions();
    EXPECT_EQ(expired, 0u);

    // Session should still be retrievable
    EXPECT_TRUE(mgr.getSession(session.session_id).has_value());
}

TEST(VoiceSessionManagerCoverage, GetSessionStateReturnsActiveForNewSession) {
    VoiceSessionManager mgr;
    auto session = mgr.createSession("state_user");
    EXPECT_EQ(mgr.getSessionState(session.session_id), SessionState::ACTIVE);
}

TEST(VoiceSessionManagerCoverage, GetSessionStateReturnsExpiredForUnknownId) {
    VoiceSessionManager mgr;
    // Unknown session ID → falls back to EXPIRED sentinel
    EXPECT_EQ(mgr.getSessionState("nonexistent_id"), SessionState::EXPIRED);
}

// ===========================================================================
// VoiceAudioStorage – untested methods
// ===========================================================================

TEST(VoiceAudioStorageCoverage, CheckDuplicateReturnsFalseForNewData) {
    VoiceAudioStorage storage;
    std::vector<uint8_t> data = {10, 20, 30, 40};
    auto result = storage.checkDuplicate(data);
    EXPECT_FALSE(result.is_duplicate);
    EXPECT_FALSE(result.content_hash.empty());
    EXPECT_EQ(result.existing_record_id, "");
}

TEST(VoiceAudioStorageCoverage, CheckDuplicateReturnsTrueAfterStore) {
    VoiceAudioStorage storage;
    std::vector<uint8_t> data = {1, 2, 3, 4, 5};
    AudioFormat fmt;
    fmt.codec = "pcm";
    std::string id = storage.store(data, fmt);

    auto result = storage.checkDuplicate(data);
    EXPECT_TRUE(result.is_duplicate);
    EXPECT_EQ(result.existing_record_id, id);
    EXPECT_EQ(result.bytes_saved, data.size());
}

TEST(VoiceAudioStorageCoverage, ComputeTierReturnsHotForFreshRecord) {
    VoiceAudioStorage storage;
    AudioFormat fmt;
    fmt.codec = "pcm";
    std::string id = storage.store({9, 8, 7}, fmt);
    auto rec = storage.getRecord(id);
    ASSERT_TRUE(rec.has_value());

    // Fresh record – should compute as HOT
    StorageTier tier = storage.computeTier(*rec);
    EXPECT_EQ(tier, StorageTier::HOT);
}

TEST(VoiceAudioStorageCoverage, PromoteTierFromWarmToHot) {
    VoiceAudioStorage storage;
    AudioFormat fmt;
    fmt.codec = "pcm";
    std::string id = storage.store({1, 2, 3}, fmt);

    // Demote first so we have something to promote
    EXPECT_TRUE(storage.demoteTier(id));  // HOT → WARM
    auto rec = storage.getRecord(id);
    ASSERT_TRUE(rec.has_value());
    EXPECT_EQ(rec->tier, StorageTier::WARM);

    // Now promote back
    EXPECT_TRUE(storage.promoteTier(id));  // WARM → HOT
    rec = storage.getRecord(id);
    ASSERT_TRUE(rec.has_value());
    EXPECT_EQ(rec->tier, StorageTier::HOT);
}

TEST(VoiceAudioStorageCoverage, PromoteTierReturnsFalseWhenAlreadyHot) {
    VoiceAudioStorage storage;
    AudioFormat fmt;
    std::string id = storage.store({5, 6}, fmt);
    // Record starts HOT – promoting should return false
    EXPECT_FALSE(storage.promoteTier(id));
}

TEST(VoiceAudioStorageCoverage, PromoteTierReturnsFalseForUnknownId) {
    VoiceAudioStorage storage;
    EXPECT_FALSE(storage.promoteTier("does_not_exist"));
}

TEST(VoiceAudioStorageCoverage, PromoteTierFromColdToWarm) {
    VoiceAudioStorage storage;
    AudioFormat fmt;
    std::string id = storage.store({7, 8, 9}, fmt);
    // HOT → WARM → COLD
    storage.demoteTier(id);
    storage.demoteTier(id);
    auto rec = storage.getRecord(id);
    ASSERT_TRUE(rec.has_value());
    EXPECT_EQ(rec->tier, StorageTier::COLD);

    EXPECT_TRUE(storage.promoteTier(id));  // COLD → WARM
    rec = storage.getRecord(id);
    ASSERT_TRUE(rec.has_value());
    EXPECT_EQ(rec->tier, StorageTier::WARM);
}

// ===========================================================================
// VoiceMeetingSupport – isCompliantForJurisdiction
// ===========================================================================

TEST(VoiceMeetingSupportCoverage, IsCompliantForJurisdictionUS_WithConsent) {
    VoiceMeetingSupport mgr;
    ComplianceRecord rec;
    rec.consent_obtained = true;
    rec.gdpr_compliant   = true;
    rec.ccpa_compliant   = true;
    EXPECT_TRUE(mgr.isCompliantForJurisdiction(rec, "US"));
}

TEST(VoiceMeetingSupportCoverage, IsCompliantForJurisdictionUS_WithoutConsent) {
    VoiceMeetingSupport mgr;
    ComplianceRecord rec;
    rec.consent_obtained = false;
    EXPECT_FALSE(mgr.isCompliantForJurisdiction(rec, "US"));
}

TEST(VoiceMeetingSupportCoverage, IsCompliantForJurisdictionEU_RequiresGDPR) {
    VoiceMeetingSupport mgr;
    ComplianceRecord rec;
    rec.consent_obtained = true;
    rec.gdpr_compliant   = true;
    EXPECT_TRUE(mgr.isCompliantForJurisdiction(rec, "EU"));
}

TEST(VoiceMeetingSupportCoverage, IsCompliantForJurisdictionEU_FailsWithoutGDPR) {
    VoiceMeetingSupport mgr;
    ComplianceRecord rec;
    rec.consent_obtained = true;
    rec.gdpr_compliant   = false;
    EXPECT_FALSE(mgr.isCompliantForJurisdiction(rec, "EU"));
}

TEST(VoiceMeetingSupportCoverage, IsCompliantForJurisdictionEEA_AliasTreatedAsEU) {
    VoiceMeetingSupport mgr;
    ComplianceRecord rec;
    rec.consent_obtained = true;
    rec.gdpr_compliant   = true;
    EXPECT_TRUE(mgr.isCompliantForJurisdiction(rec, "EEA"));
}

TEST(VoiceMeetingSupportCoverage, IsCompliantForJurisdictionCA_RequiresCCPA) {
    VoiceMeetingSupport mgr;
    ComplianceRecord rec_ok;
    rec_ok.consent_obtained = true;
    rec_ok.ccpa_compliant   = true;
    EXPECT_TRUE(mgr.isCompliantForJurisdiction(rec_ok, "CA"));

    ComplianceRecord rec_fail;
    rec_fail.consent_obtained = true;
    rec_fail.ccpa_compliant   = false;
    EXPECT_FALSE(mgr.isCompliantForJurisdiction(rec_fail, "CA"));
}

// ===========================================================================
// VoiceFallbackStrategy – ttsFallback
// ===========================================================================

TEST(VoiceFallbackStrategyCoverage, TTSFallbackReturnsValidResult) {
    auto result = VoiceFallbackStrategy::ttsFallback("engine_unavailable");
    EXPECT_TRUE(result.used_fallback);
    EXPECT_EQ(result.fallback_type, "tts_silent_audio");
}

TEST(VoiceFallbackStrategyCoverage, TTSFallbackEmptyErrorContext) {
    auto result = VoiceFallbackStrategy::ttsFallback("");
    EXPECT_TRUE(result.used_fallback);
    EXPECT_FALSE(result.fallback_type.empty());
}

// ===========================================================================
// VoiceSecurityManager – hasTranscriptionConsent, logEvent
// ===========================================================================

TEST(VoiceSecurityManagerCoverage, HasTranscriptionConsentTrue) {
    VoiceSecurityManager mgr;
    ConsentRecord record;
    record.user_id = "transcription_user";
    record.recording_consent      = false;
    record.transcription_consent  = true;
    mgr.recordConsent(record);
    EXPECT_TRUE(mgr.hasTranscriptionConsent("transcription_user"));
}

TEST(VoiceSecurityManagerCoverage, HasTranscriptionConsentFalseForNoConsent) {
    VoiceSecurityManager mgr;
    EXPECT_FALSE(mgr.hasTranscriptionConsent("unknown_user"));
}

TEST(VoiceSecurityManagerCoverage, HasTranscriptionConsentFalseWhenOnlyRecordingSet) {
    VoiceSecurityManager mgr;
    ConsentRecord record;
    record.user_id = "rec_only_user";
    record.recording_consent     = true;
    record.transcription_consent = false;
    mgr.recordConsent(record);
    EXPECT_FALSE(mgr.hasTranscriptionConsent("rec_only_user"));
}

TEST(VoiceSecurityManagerCoverage, LogEventAppearsInAuditLog) {
    VoiceSecurityManager mgr;
    VoiceAuditEntry entry;
    entry.event_type  = "CUSTOM_EVENT";
    entry.user_id     = "evt_user";
    entry.session_id  = "sess_evt";
    entry.action      = "CUSTOM_ACTION";
    entry.resource    = "custom_resource";
    entry.success     = true;
    mgr.logEvent(entry);

    auto log = mgr.getAuditLog("evt_user");
    ASSERT_EQ(log.size(), 1u);
    EXPECT_EQ(log[0].event_type, "CUSTOM_EVENT");
    EXPECT_EQ(log[0].action,     "CUSTOM_ACTION");
}

TEST(VoiceSecurityManagerCoverage, LogEventNotStoredWhenAuditDisabled) {
    VoiceSecurityConfig cfg;
    cfg.enable_audit_logging = false;
    VoiceSecurityManager mgr(cfg);

    VoiceAuditEntry entry;
    entry.user_id = "no_log_user";
    mgr.logEvent(entry);

    auto log = mgr.getAuditLog("no_log_user");
    EXPECT_TRUE(log.empty());
}

// ===========================================================================
// VoiceTTSCustomizer – getSupportedLanguages, registerLanguageVoice
// ===========================================================================

TEST(VoiceTTSCustomizerCoverage, GetSupportedLanguagesReturnsDefaultLanguages) {
    VoiceTTSCustomizer tts;
    auto langs = tts.getSupportedLanguages();
    // Default set includes at least en, de, fr, es
    EXPECT_GE(langs.size(), 4u);
}

TEST(VoiceTTSCustomizerCoverage, RegisterLanguageVoiceAddsEntry) {
    VoiceTTSCustomizer tts;
    LanguageVoice lv;
    lv.language_code    = "it-IT";
    lv.language_name    = "Italian";
    lv.default_voice_id = "it-default";
    lv.voice_ids        = {"it-default"};
    tts.registerLanguageVoice(lv);

    // supportsLanguage and getBestVoice should now work for Italian
    EXPECT_TRUE(tts.supportsLanguage("it-IT"));
    EXPECT_EQ(tts.getBestVoiceForLanguage("it-IT"), "it-default");
}

TEST(VoiceTTSCustomizerCoverage, GetSupportedLanguagesIncludesRegisteredLanguage) {
    VoiceTTSCustomizer tts;
    LanguageVoice lv;
    lv.language_code    = "ja-JP";
    lv.language_name    = "Japanese";
    lv.default_voice_id = "ja-default";
    tts.registerLanguageVoice(lv);

    auto langs = tts.getSupportedLanguages();
    bool found = false;
    for (const auto& l : langs) {
        if (l.language_code == "ja-JP") { found = true; break; }
    }
    EXPECT_TRUE(found);
}

// ===========================================================================
// ConversationContext – turnCount, hasEntity; stringToIntent
// ===========================================================================

TEST(ConversationContextCoverage, TurnCountIsZeroInitially) {
    ConversationContext ctx;
    EXPECT_EQ(ctx.turnCount(), 0u);
}

TEST(ConversationContextCoverage, TurnCountIncrementsWithAddTurn) {
    ConversationContext ctx;
    ctx.addTurn("hello", "hi");
    EXPECT_EQ(ctx.turnCount(), 1u);
    ctx.addTurn("how are you", "fine thanks");
    EXPECT_EQ(ctx.turnCount(), 2u);
}

TEST(ConversationContextCoverage, HasEntityReturnsFalseForMissingKey) {
    ConversationContext ctx;
    EXPECT_FALSE(ctx.hasEntity("nonexistent_key"));
}

TEST(ConversationContextCoverage, HasEntityReturnsTrueAfterSetEntity) {
    ConversationContext ctx;
    ctx.setEntity("location", "Berlin");
    EXPECT_TRUE(ctx.hasEntity("location"));
    EXPECT_FALSE(ctx.hasEntity("country"));
}

TEST(ConversationContextCoverage, HasEntityReturnsFalseAfterClearEntities) {
    ConversationContext ctx;
    ctx.setEntity("item", "report");
    ctx.clearEntities();
    EXPECT_FALSE(ctx.hasEntity("item"));
}

TEST(VoiceIntentDetectorCoverage, StringToIntentRoundTrip) {
    EXPECT_EQ(stringToIntent("QUERY"),        IntentCategory::QUERY);
    EXPECT_EQ(stringToIntent("COMMAND"),      IntentCategory::COMMAND);
    EXPECT_EQ(stringToIntent("QUESTION"),     IntentCategory::QUESTION);
    EXPECT_EQ(stringToIntent("CONVERSATION"), IntentCategory::CONVERSATION);
    EXPECT_EQ(stringToIntent("UNKNOWN"),      IntentCategory::UNKNOWN);
    EXPECT_EQ(stringToIntent("garbage"),      IntentCategory::UNKNOWN);
}

TEST(VoiceIntentDetectorCoverage, DetectWithContextDoesNotCrash) {
    VoiceIntentDetector detector;
    ConversationContext ctx;
    ctx.addTurn("show me the sales data", "Here is the sales data.");
    ctx.setEntity("topic", "sales");

    IntentResult result = detector.detect("give me more details", &ctx);
    // Just verify it runs and returns a valid intent
    EXPECT_NE(result.intent, static_cast<IntentCategory>(-1));
}

TEST(VoiceIntentDetectorCoverage, DetectWithNullContextDoesNotCrash) {
    VoiceIntentDetector detector;
    IntentResult result = detector.detect("list all customers");
    EXPECT_GE(result.confidence, 0.0f);
    EXPECT_LE(result.confidence, 1.0f);
}

// ===========================================================================
// VoiceAccessibility – mergeSortCues
// ===========================================================================

TEST(VoiceAccessibilityCoverage, MergeSortCuesSortsChronologically) {
    VoiceAccessibility acc;
    std::vector<CaptionCue> cues;

    CaptionCue c1; c1.start_ms = 5000; c1.end_ms = 7000; c1.text = "Third";  c1.sequence = 3;
    CaptionCue c2; c2.start_ms = 0;    c2.end_ms = 2000; c2.text = "First";  c2.sequence = 1;
    CaptionCue c3; c3.start_ms = 2500; c3.end_ms = 4500; c3.text = "Second"; c3.sequence = 2;
    cues.push_back(c1);
    cues.push_back(c2);
    cues.push_back(c3);

    auto sorted = acc.mergeSortCues(cues);
    ASSERT_EQ(sorted.size(), 3u);
    EXPECT_EQ(sorted[0].text, "First");
    EXPECT_EQ(sorted[1].text, "Second");
    EXPECT_EQ(sorted[2].text, "Third");
}

TEST(VoiceAccessibilityCoverage, MergeSortCuesFixesZeroDurationCue) {
    VoiceAccessibility acc;
    CaptionCue c; c.start_ms = 1000; c.end_ms = 1000; c.text = "ZeroDuration";
    auto sorted = acc.mergeSortCues({c});
    ASSERT_EQ(sorted.size(), 1u);
    EXPECT_GT(sorted[0].end_ms, sorted[0].start_ms);
}

TEST(VoiceAccessibilityCoverage, MergeSortCuesTruncatesOverlappingEnd) {
    VoiceAccessibility acc;
    CaptionCue c1; c1.start_ms = 0;    c1.end_ms = 3000; c1.text = "A";
    CaptionCue c2; c2.start_ms = 2000; c2.end_ms = 4000; c2.text = "B";

    auto sorted = acc.mergeSortCues({c1, c2});
    ASSERT_EQ(sorted.size(), 2u);
    // c1 end_ms should be clamped to c2 start_ms
    EXPECT_EQ(sorted[0].end_ms, 2000);
}

TEST(VoiceAccessibilityCoverage, MergeSortCuesEmptyInput) {
    VoiceAccessibility acc;
    auto sorted = acc.mergeSortCues({});
    EXPECT_TRUE(sorted.empty());
}
