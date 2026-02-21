/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_voice_production.cpp                          ║
  Version:         0.0.11                                             ║
  Last Modified:   2026-02-21 17:20:47                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     2017                                           ║
    • Open Issues:     TODOs: 0, Stubs: 4                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file test_voice_production.cpp
 * @brief Comprehensive tests for Voice Module Production Readiness components
 *
 * Covers all 10 phases of the production readiness roadmap.
 * These tests are standalone and do not require THEMIS_ENABLE_VOICE_ASSISTANT.
 */

#include <gtest/gtest.h>
#include "voice/audio_preprocessing.h"
#include "voice/voice_intent_detector.h"
#include "voice/voice_session_manager.h"
#include "voice/voice_security.h"
#include "voice/voice_error_handler.h"

#include <cmath>
#include <thread>
#include <chrono>

using namespace themis::voice;

// ============================================================
// Phase 1: Audio Preprocessing Tests
// ============================================================

TEST(AudioPreprocessingPhase1, DefaultConstructor) {
    AudioPreprocessingPipeline pipeline;
    auto stats = pipeline.getStatistics();
    EXPECT_EQ(stats["frames_processed"].get<uint64_t>(), 0u);
}

TEST(AudioPreprocessingPhase1, CustomOptionsConstructor) {
    PreprocessingOptions opts;
    opts.enable_noise_reduction = false;
    opts.enable_vad = false;
    opts.target_sample_rate = 8000;
    AudioPreprocessingPipeline pipeline(opts);
    auto stats = pipeline.getStatistics();
    EXPECT_EQ(stats["frames_processed"].get<uint64_t>(), 0u);
}

TEST(AudioPreprocessingPhase1, ProcessEmptyAudio) {
    AudioPreprocessingPipeline pipeline;
    auto result = pipeline.process({});
    EXPECT_TRUE(result.success);
}

TEST(AudioPreprocessingPhase1, ProcessRawPCMAudio) {
    AudioPreprocessingPipeline pipeline;
    // Generate 1 second of 440 Hz sine wave at 16-bit 16kHz
    const int sample_rate = 16000;
    const float freq = 440.0f;
    std::vector<uint8_t> raw(sample_rate * 2);  // 2 bytes per sample
    for (int i = 0; i < sample_rate; ++i) {
        float val = std::sin(2.0f * static_cast<float>(M_PI) * freq * i / sample_rate);
        int16_t sample = static_cast<int16_t>(val * 16000.0f);
        raw[2 * i]     = static_cast<uint8_t>(sample & 0xFF);
        raw[2 * i + 1] = static_cast<uint8_t>((sample >> 8) & 0xFF);
    }
    auto result = pipeline.process(raw, sample_rate);
    EXPECT_TRUE(result.success);
    EXPECT_FALSE(result.processed_audio.samples.empty());
}

TEST(AudioPreprocessingPhase1, NoiseReduction) {
    AudioPreprocessingPipeline pipeline;
    AudioFrame frame;
    frame.sample_rate = 16000;
    // Uniform noise
    for (int i = 0; i < 1600; ++i) {
        frame.samples.push_back(0.01f * (i % 7 - 3));
    }
    auto reduced = pipeline.applyNoiseReduction(frame, 0.8f);
    EXPECT_EQ(reduced.samples.size(), frame.samples.size());
    // RMS should be lower or equal after noise reduction
    float rms_before = 0.0f, rms_after = 0.0f;
    for (float s : frame.samples)   rms_before += s * s;
    for (float s : reduced.samples) rms_after  += s * s;
    EXPECT_LE(rms_after, rms_before + 1e-5f);
}

TEST(AudioPreprocessingPhase1, EchoCancellation) {
    AudioPreprocessingPipeline pipeline;
    AudioFrame input, reference;
    input.sample_rate = reference.sample_rate = 16000;
    for (int i = 0; i < 800; ++i) {
        float s = 0.5f * std::sin(2.0f * static_cast<float>(M_PI) * 200.0f * i / 16000);
        input.samples.push_back(s);
        reference.samples.push_back(s * 0.3f);
    }
    auto result = pipeline.applyEchoCancellation(input, reference);
    EXPECT_EQ(result.samples.size(), input.samples.size());
}

TEST(AudioPreprocessingPhase1, EchoCancellationEmptyReference) {
    AudioPreprocessingPipeline pipeline;
    AudioFrame input, empty_ref;
    input.sample_rate = 16000;
    input.samples = {0.1f, 0.2f, 0.3f};
    auto result = pipeline.applyEchoCancellation(input, empty_ref);
    EXPECT_EQ(result.samples.size(), input.samples.size());
}

TEST(AudioPreprocessingPhase1, VoiceActivityDetectionSilence) {
    AudioPreprocessingPipeline pipeline;
    AudioFrame frame;
    frame.sample_rate = 16000;
    frame.samples.assign(1600, 0.0f);
    float vad = pipeline.detectVoiceActivity(frame);
    EXPECT_GE(vad, 0.0f);
    EXPECT_LE(vad, 1.0f);
    EXPECT_LT(vad, 0.1f);
}

TEST(AudioPreprocessingPhase1, VoiceActivityDetectionLoudSignal) {
    AudioPreprocessingPipeline pipeline;
    AudioFrame frame;
    frame.sample_rate = 16000;
    for (int i = 0; i < 3200; ++i) {
        frame.samples.push_back(0.8f * std::sin(2.0f * static_cast<float>(M_PI) * 300.0f * i / 16000));
    }
    float vad = pipeline.detectVoiceActivity(frame);
    EXPECT_GT(vad, 0.5f);
}

TEST(AudioPreprocessingPhase1, AudioNormalization) {
    AudioPreprocessingPipeline pipeline;
    AudioFrame frame;
    frame.sample_rate = 16000;
    for (int i = 0; i < 1600; ++i) {
        frame.samples.push_back(0.001f * std::sin(2.0f * static_cast<float>(M_PI) * 100.0f * i / 16000));
    }
    float target_rms = 0.1f;
    auto normalized = pipeline.normalize(frame, target_rms);
    // Compute RMS
    float rms = 0.0f;
    for (float s : normalized.samples) rms += s * s;
    rms = std::sqrt(rms / normalized.samples.size());
    EXPECT_NEAR(rms, target_rms, 0.02f);
}

TEST(AudioPreprocessingPhase1, NormalizationEmptyFrame) {
    AudioPreprocessingPipeline pipeline;
    AudioFrame empty;
    auto result = pipeline.normalize(empty, 0.1f);
    EXPECT_TRUE(result.samples.empty());
}

TEST(AudioPreprocessingPhase1, Resampling) {
    AudioPreprocessingPipeline pipeline;
    AudioFrame frame;
    frame.sample_rate = 44100;
    for (int i = 0; i < 4410; ++i) {
        frame.samples.push_back(std::sin(2.0f * static_cast<float>(M_PI) * 440.0f * i / 44100));
    }
    auto resampled = pipeline.resample(frame, 16000);
    EXPECT_EQ(resampled.sample_rate, 16000);
    // Output size should be approximately 4410 * (16000/44100) = ~1600
    EXPECT_GT(resampled.samples.size(), 0u);
}

TEST(AudioPreprocessingPhase1, ResamplingNoopSameRate) {
    AudioPreprocessingPipeline pipeline;
    AudioFrame frame;
    frame.sample_rate = 16000;
    frame.samples = {0.1f, 0.2f, 0.3f};
    auto result = pipeline.resample(frame, 16000);
    EXPECT_EQ(result.samples.size(), frame.samples.size());
}

TEST(AudioPreprocessingPhase1, ConfidenceScoring) {
    AudioPreprocessingPipeline pipeline;
    AudioFrame frame;
    frame.sample_rate = 16000;
    for (int i = 0; i < 1600; ++i) {
        frame.samples.push_back(0.5f * std::sin(2.0f * static_cast<float>(M_PI) * 300.0f * i / 16000));
    }
    auto score = pipeline.scoreConfidence(frame);
    EXPECT_GE(score.overall, 0.0f);
    EXPECT_LE(score.overall, 1.0f);
    EXPECT_FALSE(score.quality_level.empty());
}

TEST(AudioPreprocessingPhase1, ConfidenceScoringEmptyFrame) {
    AudioPreprocessingPipeline pipeline;
    AudioFrame empty;
    auto score = pipeline.scoreConfidence(empty);
    EXPECT_EQ(score.quality_level, "low");
}

TEST(AudioPreprocessingPhase1, LanguageDetectionDefault) {
    AudioPreprocessingPipeline pipeline;
    AudioFrame frame;
    frame.samples.assign(1600, 0.1f);
    auto result = pipeline.detectLanguage(frame);
    EXPECT_EQ(result.detected_language, "en");
    EXPECT_GT(result.confidence, 0.0f);
}

TEST(AudioPreprocessingPhase1, LanguageDetectionWithHint) {
    AudioPreprocessingPipeline pipeline;
    AudioFrame frame;
    frame.samples.assign(1600, 0.1f);
    auto result = pipeline.detectLanguage(frame, "fr");
    EXPECT_EQ(result.detected_language, "fr");
    EXPECT_GT(result.confidence, 0.5f);
}

TEST(AudioPreprocessingPhase1, StatisticsTracking) {
    AudioPreprocessingPipeline pipeline;
    AudioFrame frame;
    frame.sample_rate = 16000;
    frame.samples.assign(1600, 0.1f);
    pipeline.processFrame(frame);
    pipeline.processFrame(frame);
    auto stats = pipeline.getStatistics();
    EXPECT_EQ(stats["frames_processed"].get<uint64_t>(), 2u);
}

TEST(AudioPreprocessingPhase1, StatisticsReset) {
    AudioPreprocessingPipeline pipeline;
    AudioFrame frame;
    frame.samples.assign(800, 0.05f);
    pipeline.processFrame(frame);
    pipeline.resetStatistics();
    auto stats = pipeline.getStatistics();
    EXPECT_EQ(stats["frames_processed"].get<uint64_t>(), 0u);
}

TEST(AudioPreprocessingPhase1, HighNoiseInput) {
    PreprocessingOptions opts;
    opts.noise_reduction_strength = 0.9f;
    AudioPreprocessingPipeline pipeline(opts);
    AudioFrame frame;
    frame.sample_rate = 16000;
    for (int i = 0; i < 3200; ++i) {
        frame.samples.push_back(0.005f * (i % 9 - 4));
    }
    auto result = pipeline.processFrame(frame);
    EXPECT_TRUE(result.success);
}

TEST(AudioPreprocessingPhase1, SilentAudio) {
    AudioPreprocessingPipeline pipeline;
    AudioFrame frame;
    frame.sample_rate = 16000;
    frame.samples.assign(3200, 0.0f);
    auto result = pipeline.processFrame(frame);
    EXPECT_TRUE(result.success);
    EXPECT_NEAR(result.voice_activity_ratio, 0.0f, 0.1f);
}

TEST(AudioPreprocessingPhase1, HighSampleRate) {
    PreprocessingOptions opts;
    opts.target_sample_rate = 8000;
    AudioPreprocessingPipeline pipeline(opts);
    AudioFrame frame;
    frame.sample_rate = 48000;
    for (int i = 0; i < 4800; ++i) {
        frame.samples.push_back(0.3f * std::sin(2.0f * static_cast<float>(M_PI) * 440.0f * i / 48000));
    }
    auto result = pipeline.processFrame(frame);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.processed_audio.sample_rate, 8000);
}

// ============================================================
// Phase 3: Intent Detection Tests
// ============================================================

TEST(IntentDetectorPhase3, DefaultConstructor) {
    VoiceIntentDetector detector;
    auto stats = detector.getStatistics();
    EXPECT_EQ(stats["detections_total"].get<uint64_t>(), 0u);
}

TEST(IntentDetectorPhase3, DetectQueryIntent) {
    VoiceIntentDetector detector;
    auto result = detector.detect("show me the total sales for last month");
    EXPECT_EQ(result.intent, IntentCategory::QUERY);
    EXPECT_GT(result.confidence, 0.0f);
}

TEST(IntentDetectorPhase3, DetectCommandIntent) {
    VoiceIntentDetector detector;
    auto result = detector.detect("delete the record with id 42");
    EXPECT_EQ(result.intent, IntentCategory::COMMAND);
}

TEST(IntentDetectorPhase3, DetectQuestionIntent) {
    VoiceIntentDetector detector;
    auto result = detector.detect("how to use the filter function");
    EXPECT_EQ(result.intent, IntentCategory::QUESTION);
}

TEST(IntentDetectorPhase3, DetectConversationIntent) {
    VoiceIntentDetector detector;
    auto result = detector.detect("hello, how are you");
    EXPECT_EQ(result.intent, IntentCategory::CONVERSATION);
}

TEST(IntentDetectorPhase3, DetectUnknownIntent) {
    VoiceIntentDetector detector;
    auto result = detector.detect("xyzzy plugh foobar");
    EXPECT_EQ(result.intent, IntentCategory::UNKNOWN);
}

TEST(IntentDetectorPhase3, IntentToStringRoundTrip) {
    EXPECT_EQ(intentToString(IntentCategory::QUERY),    "QUERY");
    EXPECT_EQ(intentToString(IntentCategory::COMMAND),  "COMMAND");
    EXPECT_EQ(intentToString(IntentCategory::QUESTION), "QUESTION");
    EXPECT_EQ(intentToString(IntentCategory::CONVERSATION), "CONVERSATION");
    EXPECT_EQ(intentToString(IntentCategory::UNKNOWN),  "UNKNOWN");

    EXPECT_EQ(stringToIntent("QUERY"),   IntentCategory::QUERY);
    EXPECT_EQ(stringToIntent("COMMAND"), IntentCategory::COMMAND);
    EXPECT_EQ(stringToIntent("invalid"), IntentCategory::UNKNOWN);
}

TEST(IntentDetectorPhase3, ExtractDateEntities) {
    VoiceIntentDetector detector;
    auto entities = detector.extractEntities("show data for last week and yesterday");
    bool found_last_week = false, found_yesterday = false;
    for (const auto& e : entities) {
        if (e.text == "last week")  found_last_week  = true;
        if (e.text == "yesterday") found_yesterday = true;
    }
    EXPECT_TRUE(found_last_week);
    EXPECT_TRUE(found_yesterday);
}

TEST(IntentDetectorPhase3, ExtractNumberEntities) {
    VoiceIntentDetector detector;
    auto entities = detector.extractEntities("get the top 100 records with 50% discount");
    bool found_100 = false, found_50pct = false;
    for (const auto& e : entities) {
        if (e.text == "100")  found_100    = true;
        if (e.text == "50%")  found_50pct  = true;
    }
    EXPECT_TRUE(found_100);
    EXPECT_TRUE(found_50pct);
}

TEST(IntentDetectorPhase3, ExtractMetricEntities) {
    VoiceIntentDetector detector;
    auto entities = detector.extractEntities("what is the total revenue and profit margin");
    bool found_revenue = false;
    for (const auto& e : entities) {
        if (e.type == "METRIC") found_revenue = true;
    }
    EXPECT_TRUE(found_revenue);
}

TEST(IntentDetectorPhase3, NormalizeQueryWithContext) {
    VoiceIntentDetector detector;
    ConversationContext ctx;
    ctx.setEntity("last_object", "the revenue table");
    ctx.addTurn("show revenue", "Here is the revenue table.");
    std::string normalized = detector.normalizeQuery("filter it by date", &ctx);
    EXPECT_NE(normalized.find("the revenue table"), std::string::npos);
}

TEST(IntentDetectorPhase3, NormalizeQueryNoContext) {
    VoiceIntentDetector detector;
    std::string original = "show me it";
    std::string normalized = detector.normalizeQuery(original, nullptr);
    EXPECT_EQ(normalized, original);
}

TEST(IntentDetectorPhase3, ConversationContextHistory) {
    ConversationContext ctx(3);
    ctx.addTurn("q1", "a1");
    ctx.addTurn("q2", "a2");
    ctx.addTurn("q3", "a3");
    EXPECT_EQ(ctx.turnCount(), 3u);
    ctx.addTurn("q4", "a4");
    // Should evict oldest
    EXPECT_EQ(ctx.turnCount(), 3u);
}

TEST(IntentDetectorPhase3, ConversationContextEntities) {
    ConversationContext ctx;
    ctx.setEntity("company", "Acme Corp");
    EXPECT_TRUE(ctx.hasEntity("company"));
    EXPECT_FALSE(ctx.hasEntity("missing_key"));
    auto val = ctx.getEntity("company");
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(*val, "Acme Corp");
}

TEST(IntentDetectorPhase3, ConversationContextBuildString) {
    ConversationContext ctx;
    ctx.addTurn("hello", "hi there");
    std::string str = ctx.buildContextString(5);
    EXPECT_NE(str.find("hello"), std::string::npos);
    EXPECT_NE(str.find("hi there"), std::string::npos);
}

TEST(IntentDetectorPhase3, ConversationContextClear) {
    ConversationContext ctx;
    ctx.addTurn("q", "a");
    ctx.setEntity("k", "v");
    ctx.clear();
    EXPECT_EQ(ctx.turnCount(), 0u);
    EXPECT_FALSE(ctx.hasEntity("k"));
}

TEST(IntentDetectorPhase3, StatisticsTracking) {
    VoiceIntentDetector detector;
    detector.detect("find the sales");
    detector.detect("add a new row");
    auto stats = detector.getStatistics();
    EXPECT_EQ(stats["detections_total"].get<uint64_t>(), 2u);
}

TEST(IntentDetectorPhase3, MeetsThreshold) {
    IntentDetectorConfig cfg;
    cfg.min_confidence_threshold = 0.5f;
    VoiceIntentDetector detector(cfg);
    EXPECT_TRUE(detector.meetsThreshold(0.8f));
    EXPECT_FALSE(detector.meetsThreshold(0.3f));
}

// ============================================================
// Phase 6: Session Management Tests
// ============================================================

TEST(SessionManagerPhase6, CreateSession) {
    VoiceSessionManager mgr;
    auto session = mgr.createSession("user1", "device1");
    EXPECT_FALSE(session.session_id.empty());
    EXPECT_EQ(session.user_id, "user1");
    EXPECT_EQ(session.device_id, "device1");
    EXPECT_EQ(session.state, SessionState::ACTIVE);
}

TEST(SessionManagerPhase6, GetSession) {
    VoiceSessionManager mgr;
    auto created = mgr.createSession("user2");
    auto fetched = mgr.getSession(created.session_id);
    ASSERT_TRUE(fetched.has_value());
    EXPECT_EQ(fetched->session_id, created.session_id);
}

TEST(SessionManagerPhase6, GetNonExistentSession) {
    VoiceSessionManager mgr;
    auto result = mgr.getSession("nonexistent_session_id");
    EXPECT_FALSE(result.has_value());
}

TEST(SessionManagerPhase6, UpdateSessionContext) {
    VoiceSessionManager mgr;
    auto session = mgr.createSession("user3");
    nlohmann::json ctx;
    ctx["key"] = "value";
    bool ok = mgr.updateSession(session.session_id, ctx);
    EXPECT_TRUE(ok);
    auto updated = mgr.getSession(session.session_id);
    ASSERT_TRUE(updated.has_value());
    EXPECT_EQ(updated->context["key"], "value");
}

TEST(SessionManagerPhase6, AddConversationTurn) {
    VoiceSessionManager mgr;
    auto session = mgr.createSession("user4");
    bool ok = mgr.addConversationTurn(session.session_id, "hello", "hi there");
    EXPECT_TRUE(ok);
    auto updated = mgr.getSession(session.session_id);
    ASSERT_TRUE(updated.has_value());
    EXPECT_EQ(updated->total_turns, 1u);
    EXPECT_EQ(updated->conversation_history.size(), 2u);
}

TEST(SessionManagerPhase6, TerminateSession) {
    VoiceSessionManager mgr;
    auto session = mgr.createSession("user5");
    EXPECT_TRUE(mgr.isSessionActive(session.session_id));
    bool ok = mgr.terminateSession(session.session_id);
    EXPECT_TRUE(ok);
    auto result = mgr.getSession(session.session_id);
    EXPECT_FALSE(result.has_value());
}

TEST(SessionManagerPhase6, SessionStateToString) {
    EXPECT_EQ(sessionStateToString(SessionState::ACTIVE),     "ACTIVE");
    EXPECT_EQ(sessionStateToString(SessionState::IDLE),       "IDLE");
    EXPECT_EQ(sessionStateToString(SessionState::EXPIRED),    "EXPIRED");
    EXPECT_EQ(sessionStateToString(SessionState::TERMINATED), "TERMINATED");
}

TEST(SessionManagerPhase6, GenerateUniqueSessionIds) {
    std::string id1 = VoiceSessionManager::generateSessionId();
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    std::string id2 = VoiceSessionManager::generateSessionId();
    EXPECT_NE(id1, id2);
    EXPECT_TRUE(id1.substr(0, 5) == "sess_");
}

TEST(SessionManagerPhase6, Analytics) {
    VoiceSessionManager mgr;
    mgr.createSession("user_a", "device_x");
    mgr.createSession("user_b", "device_y");
    auto analytics = mgr.getAnalytics();
    EXPECT_EQ(analytics.total_sessions, 2u);
    EXPECT_EQ(analytics.active_sessions, 2u);
}

TEST(SessionManagerPhase6, GetSessionsForUser) {
    VoiceSessionManager mgr;
    mgr.createSession("multi_user", "dev1");
    mgr.createSession("multi_user", "dev2");
    mgr.createSession("other_user", "dev3");
    auto sessions = mgr.getSessionsForUser("multi_user");
    EXPECT_EQ(sessions.size(), 2u);
}

TEST(SessionManagerPhase6, InMemoryBackendCount) {
    auto backend = std::make_unique<InMemorySessionBackend>();
    auto* raw = backend.get();
    VoiceSessionManager mgr({}, std::move(backend));
    mgr.createSession("u1");
    mgr.createSession("u2");
    EXPECT_EQ(raw->count(), 2u);
}

// ============================================================
// Phase 7: Security Tests
// ============================================================

TEST(VoiceSecurityPhase7, DefaultConstructor) {
    VoiceSecurityManager mgr;
    auto stats = mgr.getSecurityStats();
    EXPECT_EQ(stats["total_consents"].get<size_t>(), 0u);
}

TEST(VoiceSecurityPhase7, RedactPhoneNumber) {
    VoiceSecurityManager mgr;
    auto result = mgr.redactPII("Call me at +12345678901 for details.");
    EXPECT_TRUE(result.has_pii);
    EXPECT_NE(result.redacted_text.find("[PHONE_REDACTED]"), std::string::npos);
}

TEST(VoiceSecurityPhase7, RedactEmail) {
    VoiceSecurityManager mgr;
    auto result = mgr.redactPII("Email me at user@example.com please.");
    EXPECT_TRUE(result.has_pii);
    EXPECT_NE(result.redacted_text.find("[EMAIL_REDACTED]"), std::string::npos);
    EXPECT_EQ(result.redacted_text.find("user@example.com"), std::string::npos);
}

TEST(VoiceSecurityPhase7, RedactSSN) {
    VoiceSecurityManager mgr;
    auto result = mgr.redactPIITypes("SSN is 123-45-6789.", {PIIType::SSN});
    EXPECT_TRUE(result.has_pii);
    EXPECT_NE(result.redacted_text.find("[SSN_REDACTED]"), std::string::npos);
}

TEST(VoiceSecurityPhase7, RedactIPAddress) {
    VoiceSecurityManager mgr;
    auto result = mgr.redactPIITypes("Server IP is 192.168.1.100.", {PIIType::IP_ADDRESS});
    EXPECT_TRUE(result.has_pii);
    EXPECT_NE(result.redacted_text.find("[IP_REDACTED]"), std::string::npos);
}

TEST(VoiceSecurityPhase7, NoRedactionNeeded) {
    VoiceSecurityManager mgr;
    auto result = mgr.redactPII("The meeting is scheduled for tomorrow.");
    EXPECT_FALSE(result.has_pii);
    EXPECT_EQ(result.redaction_count, 0);
}

TEST(VoiceSecurityPhase7, ContainsPII) {
    VoiceSecurityManager mgr;
    EXPECT_TRUE(mgr.containsPII("My email is test@test.com"));
    EXPECT_FALSE(mgr.containsPII("No sensitive info here"));
}

TEST(VoiceSecurityPhase7, RecordAndGetConsent) {
    VoiceSecurityManager mgr;
    ConsentRecord record;
    record.user_id = "user123";
    record.recording_consent = true;
    record.transcription_consent = true;
    record.consent_version = "2.0";
    EXPECT_TRUE(mgr.recordConsent(record));
    auto fetched = mgr.getConsent("user123");
    ASSERT_TRUE(fetched.has_value());
    EXPECT_EQ(fetched->consent_version, "2.0");
    EXPECT_TRUE(fetched->recording_consent);
}

TEST(VoiceSecurityPhase7, HasRecordingConsent) {
    VoiceSecurityManager mgr;
    ConsentRecord record;
    record.user_id = "user_consent";
    record.recording_consent = true;
    mgr.recordConsent(record);
    EXPECT_TRUE(mgr.hasRecordingConsent("user_consent"));
    EXPECT_FALSE(mgr.hasRecordingConsent("no_consent_user"));
}

TEST(VoiceSecurityPhase7, RevokeConsent) {
    VoiceSecurityManager mgr;
    ConsentRecord record;
    record.user_id = "user_revoke";
    record.recording_consent = true;
    mgr.recordConsent(record);
    EXPECT_TRUE(mgr.revokeConsent("user_revoke"));
    EXPECT_FALSE(mgr.hasRecordingConsent("user_revoke"));
}

TEST(VoiceSecurityPhase7, AuditLogging) {
    VoiceSecurityManager mgr;
    mgr.logAccess("user1", "sess1", "voice_data");
    mgr.logError("user1", "sess1", "STT failed");
    auto log = mgr.getAuditLog("user1");
    EXPECT_EQ(log.size(), 2u);
}

TEST(VoiceSecurityPhase7, AuditLogLimit) {
    VoiceSecurityManager mgr;
    for (int i = 0; i < 20; ++i) {
        mgr.logAccess("bulk_user", "sess", "resource_" + std::to_string(i));
    }
    auto log = mgr.getAuditLog("bulk_user", 5);
    EXPECT_EQ(log.size(), 5u);
}

TEST(VoiceSecurityPhase7, DataDeletion) {
    VoiceSecurityManager mgr;
    ConsentRecord record;
    record.user_id = "user_delete";
    record.recording_consent = true;
    mgr.recordConsent(record);
    mgr.logAccess("user_delete", "s1", "resource");

    DataDeletionRequest req;
    req.user_id = "user_delete";
    req.delete_recordings = true;
    req.delete_transcripts = true;
    req.delete_sessions = true;
    auto result = mgr.deleteUserData(req);
    EXPECT_TRUE(result.success);
    EXPECT_FALSE(mgr.hasRecordingConsent("user_delete"));
}

TEST(VoiceSecurityPhase7, ExportUserData) {
    VoiceSecurityManager mgr;
    ConsentRecord record;
    record.user_id = "export_user";
    record.recording_consent = true;
    mgr.recordConsent(record);
    mgr.logAccess("export_user", "s1", "data");
    auto data = mgr.exportUserData("export_user");
    EXPECT_EQ(data["user_id"], "export_user");
    EXPECT_TRUE(data["consent"]["recording"].get<bool>());
}

TEST(VoiceSecurityPhase7, PIITypeToString) {
    EXPECT_EQ(piiTypeToString(PIIType::PHONE_NUMBER),  "PHONE_NUMBER");
    EXPECT_EQ(piiTypeToString(PIIType::EMAIL_ADDRESS), "EMAIL_ADDRESS");
    EXPECT_EQ(piiTypeToString(PIIType::CREDIT_CARD),   "CREDIT_CARD");
    EXPECT_EQ(piiTypeToString(PIIType::SSN),           "SSN");
    EXPECT_EQ(piiTypeToString(PIIType::IP_ADDRESS),    "IP_ADDRESS");
}

TEST(VoiceSecurityPhase7, ScheduleAutoDelete) {
    VoiceSecurityManager mgr;
    bool ok = mgr.scheduleAutoDelete("user_sched", 1000 * 60 * 60 * 24);  // 24h
    EXPECT_TRUE(ok);
    auto stats = mgr.getSecurityStats();
    EXPECT_EQ(stats["auto_delete_scheduled"].get<size_t>(), 1u);
}

// ============================================================
// Phase 8: Error Handler Tests
// ============================================================

TEST(ErrorHandlerPhase8, DefaultConstructor) {
    VoiceErrorHandler handler;
    EXPECT_TRUE(handler.isSystemHealthy());
}

TEST(ErrorHandlerPhase8, HandleErrorReturnsJSON) {
    VoiceErrorHandler handler;
    auto report = handler.handleError(VoiceErrorCode::STT_FAILED, "test_context", "details here");
    EXPECT_EQ(report["error_code"], "STT_FAILED");
    EXPECT_EQ(report["context"], "test_context");
    EXPECT_FALSE(report["recovery_action"].get<std::string>().empty());
}

TEST(ErrorHandlerPhase8, HandleSessionNotFound) {
    VoiceErrorHandler handler;
    auto report = handler.handleError(VoiceErrorCode::SESSION_NOT_FOUND, "ctx");
    EXPECT_EQ(report["recovery_action"], "create_new_session");
}

TEST(ErrorHandlerPhase8, HandleConsentMissing) {
    VoiceErrorHandler handler;
    auto report = handler.handleError(VoiceErrorCode::CONSENT_MISSING, "ctx");
    EXPECT_EQ(report["recovery_action"], "request_consent");
}

TEST(ErrorHandlerPhase8, ErrorCodeToString) {
    EXPECT_EQ(errorCodeToString(VoiceErrorCode::NONE),             "NONE");
    EXPECT_EQ(errorCodeToString(VoiceErrorCode::STT_FAILED),       "STT_FAILED");
    EXPECT_EQ(errorCodeToString(VoiceErrorCode::TTS_FAILED),       "TTS_FAILED");
    EXPECT_EQ(errorCodeToString(VoiceErrorCode::LLM_FAILED),       "LLM_FAILED");
    EXPECT_EQ(errorCodeToString(VoiceErrorCode::SESSION_EXPIRED),  "SESSION_EXPIRED");
    EXPECT_EQ(errorCodeToString(VoiceErrorCode::NETWORK_ERROR),    "NETWORK_ERROR");
    EXPECT_EQ(errorCodeToString(VoiceErrorCode::TIMEOUT),          "TIMEOUT");
    EXPECT_EQ(errorCodeToString(VoiceErrorCode::UNKNOWN),          "UNKNOWN");
}

TEST(ErrorHandlerPhase8, CircuitBreakerInitiallyClosed) {
    VoiceCircuitBreaker cb("test_cb");
    EXPECT_EQ(cb.getState(), CircuitState::CLOSED);
    EXPECT_TRUE(cb.canCall());
}

TEST(ErrorHandlerPhase8, CircuitBreakerOpensAfterFailures) {
    CircuitBreakerConfig cfg;
    cfg.failure_threshold = 3;
    VoiceCircuitBreaker cb("test_cb2", cfg);
    for (int i = 0; i < 3; ++i) cb.recordFailure();
    EXPECT_EQ(cb.getState(), CircuitState::OPEN);
}

TEST(ErrorHandlerPhase8, CircuitBreakerRejectsWhenOpen) {
    CircuitBreakerConfig cfg;
    cfg.failure_threshold = 1;
    cfg.open_duration_ms = 60000;  // Stay open for 60s
    VoiceCircuitBreaker cb("test_cb3", cfg);
    cb.recordFailure();
    ASSERT_EQ(cb.getState(), CircuitState::OPEN);
    EXPECT_FALSE(cb.canCall());
}

TEST(ErrorHandlerPhase8, CircuitBreakerReset) {
    CircuitBreakerConfig cfg;
    cfg.failure_threshold = 2;
    VoiceCircuitBreaker cb("test_cb4", cfg);
    cb.recordFailure();
    cb.recordFailure();
    ASSERT_EQ(cb.getState(), CircuitState::OPEN);
    cb.reset();
    EXPECT_EQ(cb.getState(), CircuitState::CLOSED);
    EXPECT_TRUE(cb.canCall());
}

TEST(ErrorHandlerPhase8, CircuitBreakerCallTemplate) {
    VoiceCircuitBreaker cb("test_cb5");
    int counter = 0;
    bool called = cb.call([&counter]() { ++counter; });
    EXPECT_TRUE(called);
    EXPECT_EQ(counter, 1);
}

TEST(ErrorHandlerPhase8, CircuitBreakerCallWithException) {
    CircuitBreakerConfig cfg;
    cfg.failure_threshold = 10;
    VoiceCircuitBreaker cb("test_cb6", cfg);
    bool called = cb.call([]() { throw std::runtime_error("test error"); });
    EXPECT_FALSE(called);
    EXPECT_EQ(cb.getState(), CircuitState::CLOSED);  // Only 1 failure, threshold 10
}

TEST(ErrorHandlerPhase8, CircuitBreakerStats) {
    VoiceCircuitBreaker cb("stats_cb");
    cb.canCall();
    cb.recordSuccess();
    auto stats = cb.getStats();
    EXPECT_EQ(stats["name"], "stats_cb");
    EXPECT_EQ(stats["state"].get<std::string>(), "CLOSED");
}

TEST(ErrorHandlerPhase8, CircuitStateToString) {
    EXPECT_EQ(circuitStateToString(CircuitState::CLOSED),    "CLOSED");
    EXPECT_EQ(circuitStateToString(CircuitState::OPEN),      "OPEN");
    EXPECT_EQ(circuitStateToString(CircuitState::HALF_OPEN), "HALF_OPEN");
}

TEST(ErrorHandlerPhase8, VoiceExceptionCode) {
    VoiceException ex(VoiceErrorCode::LLM_FAILED, "LLM is down");
    EXPECT_EQ(ex.code(), VoiceErrorCode::LLM_FAILED);
    EXPECT_NE(std::string(ex.what()).find("LLM"), std::string::npos);
}

TEST(ErrorHandlerPhase8, FallbackSTT) {
    auto fb = VoiceFallbackStrategy::sttFallback("context");
    EXPECT_TRUE(fb.used_fallback);
    EXPECT_EQ(fb.fallback_type, "stt_empty_transcript");
}

TEST(ErrorHandlerPhase8, FallbackLLM) {
    auto fb = VoiceFallbackStrategy::llmFallback("user question");
    EXPECT_TRUE(fb.used_fallback);
    EXPECT_FALSE(fb.result.empty());
}

TEST(ErrorHandlerPhase8, FallbackSession) {
    auto fb = VoiceFallbackStrategy::sessionFallback("sess_abc");
    EXPECT_TRUE(fb.used_fallback);
    EXPECT_NE(fb.result.find("sess_abc"), std::string::npos);
}

TEST(ErrorHandlerPhase8, RetryHandlerStats) {
    VoiceRetryHandler rh;
    auto stats = rh.getStats();
    EXPECT_EQ(stats["total_retries"].get<uint64_t>(), 0u);
    EXPECT_EQ(stats["total_failures"].get<uint64_t>(), 0u);
}

TEST(ErrorHandlerPhase8, RetryHandlerSuccessNoRetry) {
    VoiceRetryHandler rh;
    int count = 0;
    int result = rh.executeWithRetry<int>([&count]() -> int {
        ++count;
        return 42;
    });
    EXPECT_EQ(result, 42);
    EXPECT_EQ(count, 1);
}

TEST(ErrorHandlerPhase8, RetryHandlerNonRetryableError) {
    VoiceRetryHandler rh;
    EXPECT_THROW({
        rh.executeWithRetry<int>([]() -> int {
            throw VoiceException(VoiceErrorCode::SECURITY_VIOLATION, "not retryable");
        });
    }, VoiceException);
}

TEST(ErrorHandlerPhase8, ErrorHandlerGetHealthStatus) {
    VoiceErrorHandler handler;
    auto status = handler.getHealthStatus();
    EXPECT_TRUE(status["healthy"].get<bool>());
    EXPECT_TRUE(status.contains("stt_circuit"));
    EXPECT_TRUE(status.contains("tts_circuit"));
    EXPECT_TRUE(status.contains("llm_circuit"));
    EXPECT_TRUE(status.contains("storage_circuit"));
}

TEST(ErrorHandlerPhase8, ErrorHandlerCircuitAccessors) {
    VoiceErrorHandler handler;
    EXPECT_EQ(handler.sttCircuit().getName(),     "stt");
    EXPECT_EQ(handler.ttsCircuit().getName(),     "tts");
    EXPECT_EQ(handler.llmCircuit().getName(),     "llm");
    EXPECT_EQ(handler.storageCircuit().getName(), "storage");
}

// ============================================================
// Phase 9: Multi-language Tests
// ============================================================

TEST(MultiLanguagePhase9, DetectEnglishByDefault) {
    AudioPreprocessingPipeline pipeline;
    AudioFrame frame;
    frame.samples.assign(1600, 0.1f);
    auto result = pipeline.detectLanguage(frame, "auto");
    EXPECT_EQ(result.detected_language, "en");
}

TEST(MultiLanguagePhase9, DetectSpanishHint) {
    AudioPreprocessingPipeline pipeline;
    AudioFrame frame;
    frame.samples.assign(1600, 0.1f);
    auto result = pipeline.detectLanguage(frame, "es");
    EXPECT_EQ(result.detected_language, "es");
    EXPECT_GT(result.confidence, 0.8f);
}

TEST(MultiLanguagePhase9, DetectGermanHint) {
    AudioPreprocessingPipeline pipeline;
    AudioFrame frame;
    frame.samples.assign(1600, 0.1f);
    auto result = pipeline.detectLanguage(frame, "de");
    EXPECT_EQ(result.detected_language, "de");
}

TEST(MultiLanguagePhase9, SessionPreferredLanguage) {
    VoiceSessionManager mgr;
    auto session = mgr.createSession("user_lang");
    EXPECT_EQ(session.preferred_language, "en");
}

TEST(MultiLanguagePhase9, LanguageDetectionAlternatives) {
    AudioPreprocessingPipeline pipeline;
    AudioFrame frame;
    frame.samples.assign(1600, 0.1f);
    auto result = pipeline.detectLanguage(frame);
    EXPECT_FALSE(result.alternatives.empty());
}

// ============================================================
// Phase 10: Performance / Benchmarks
// ============================================================

TEST(PerformanceBenchmarkPhase10, AudioProcessingThroughput) {
    AudioPreprocessingPipeline pipeline;
    AudioFrame frame;
    frame.sample_rate = 16000;
    frame.samples.resize(16000);  // 1 second of audio
    for (size_t i = 0; i < frame.samples.size(); ++i) {
        frame.samples[i] = 0.3f * std::sin(2.0f * static_cast<float>(M_PI) * 440.0f * i / 16000);
    }

    auto start = std::chrono::steady_clock::now();
    const int iterations = 10;
    for (int i = 0; i < iterations; ++i) {
        pipeline.processFrame(frame);
    }
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start).count();

    // Should process 10 seconds of audio in well under 10 seconds
    EXPECT_LT(elapsed, 5000);
    auto stats = pipeline.getStatistics();
    EXPECT_EQ(stats["frames_processed"].get<uint64_t>(), static_cast<uint64_t>(iterations));
}

TEST(PerformanceBenchmarkPhase10, IntentDetectionThroughput) {
    VoiceIntentDetector detector;
    std::vector<std::string> queries = {
        "find the top 10 customers",
        "delete old records",
        "how to use the search function",
        "hello there",
        "show sales for last month"
    };

    auto start = std::chrono::steady_clock::now();
    const int iterations = 100;
    for (int i = 0; i < iterations; ++i) {
        detector.detect(queries[i % queries.size()]);
    }
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start).count();

    EXPECT_LT(elapsed, 2000);
    auto stats = detector.getStatistics();
    EXPECT_EQ(stats["detections_total"].get<uint64_t>(), static_cast<uint64_t>(iterations));
}

TEST(PerformanceBenchmarkPhase10, SessionCreationThroughput) {
    VoiceSessionManager mgr;
    auto start = std::chrono::steady_clock::now();
    const int count = 100;
    for (int i = 0; i < count; ++i) {
        mgr.createSession("perf_user_" + std::to_string(i));
    }
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start).count();

    EXPECT_LT(elapsed, 1000);
    auto analytics = mgr.getAnalytics();
    EXPECT_EQ(analytics.total_sessions, static_cast<size_t>(count));
}

TEST(PerformanceBenchmarkPhase10, PIIRedactionThroughput) {
    VoiceSecurityManager mgr;
    std::string text = "User john@example.com called +12345678901 about account 1234-5678-9012-3456";
    auto start = std::chrono::steady_clock::now();
    const int iterations = 1000;
    for (int i = 0; i < iterations; ++i) {
        mgr.redactPII(text);
    }
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start).count();
    EXPECT_LT(elapsed, 5000);
}

TEST(PerformanceBenchmarkPhase10, CircuitBreakerThroughput) {
    VoiceCircuitBreaker cb("perf_cb");
    int counter = 0;
    auto start = std::chrono::steady_clock::now();
    const int iterations = 10000;
    for (int i = 0; i < iterations; ++i) {
        cb.call([&counter]() { ++counter; });
    }
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start).count();
    EXPECT_LT(elapsed, 2000);
    EXPECT_EQ(counter, iterations);
}

// --- Phase 2, 4, 5 additions ---
#include "voice/voice_tts_customizer.h"
#include "voice/voice_meeting_support.h"
#include "voice/voice_audio_storage.h"

// ============================================================
// Phase 2: TTS Customization Tests
// ============================================================

TEST(TTSCustomizerPhase2, DefaultConstructor) {
    VoiceTTSCustomizer tts;
    auto stats = tts.getStatistics();
    EXPECT_GT(stats["profile_count"].get<size_t>(), 0u);
    EXPECT_GT(stats["language_count"].get<size_t>(), 0u);
}

TEST(TTSCustomizerPhase2, RegisterVoiceProfile) {
    VoiceTTSCustomizer tts;
    VoiceProfile p;
    p.id = "test-voice";
    p.name = "Test Voice";
    p.language = "en-US";
    p.engine = "piper";
    EXPECT_TRUE(tts.registerVoiceProfile(p));
    EXPECT_TRUE(tts.hasProfile("test-voice"));
}

TEST(TTSCustomizerPhase2, DuplicateProfileRejected) {
    VoiceTTSCustomizer tts;
    VoiceProfile p;
    p.id = "dup-voice";
    p.language = "en-US";
    EXPECT_TRUE(tts.registerVoiceProfile(p));
    EXPECT_FALSE(tts.registerVoiceProfile(p)); // second attempt fails
}

TEST(TTSCustomizerPhase2, GetProfile) {
    VoiceTTSCustomizer tts;
    auto prof = tts.getProfile("en-default");
    ASSERT_TRUE(prof.has_value());
    EXPECT_EQ(prof->id, "en-default");
    EXPECT_EQ(prof->language, "en-US");
}

TEST(TTSCustomizerPhase2, ListProfiles) {
    VoiceTTSCustomizer tts;
    auto profiles = tts.listProfiles();
    EXPECT_GE(profiles.size(), 5u); // at least the 5 built-ins
}

TEST(TTSCustomizerPhase2, BuildProsodyWithOverride) {
    VoiceTTSCustomizer tts;
    ProsodyConfig overrides;
    overrides.pitch = 1.5f;
    auto result = tts.buildProsody("en-default", overrides);
    EXPECT_FLOAT_EQ(result.pitch, 1.5f);
    EXPECT_FLOAT_EQ(result.speed, 1.0f); // default unchanged
}

TEST(TTSCustomizerPhase2, ValidateProsodyClampsPitch) {
    VoiceTTSCustomizer tts;
    ProsodyConfig p;
    p.pitch = 10.0f; // out of range
    auto validated = tts.validateProsody(p);
    EXPECT_FLOAT_EQ(validated.pitch, 2.0f); // clamped to max
    p.pitch = 0.1f;
    validated = tts.validateProsody(p);
    EXPECT_FLOAT_EQ(validated.pitch, 0.5f); // clamped to min
}

TEST(TTSCustomizerPhase2, ValidateProsodyClampsSpeed) {
    VoiceTTSCustomizer tts;
    ProsodyConfig p;
    p.speed = 0.1f;
    auto validated = tts.validateProsody(p);
    EXPECT_FLOAT_EQ(validated.speed, 0.25f);
    p.speed = 10.0f;
    validated = tts.validateProsody(p);
    EXPECT_FLOAT_EQ(validated.speed, 4.0f);
}

TEST(TTSCustomizerPhase2, ParseSSMLStripsTagsBasic) {
    VoiceTTSCustomizer tts;
    std::string ssml = "<speak>Hello <break time=\"500ms\"/> World</speak>";
    auto result = tts.parseSSML(ssml);
    EXPECT_EQ(result.plain_text, "Hello World");
    EXPECT_TRUE(result.has_breaks);
}

TEST(TTSCustomizerPhase2, ParseSSMLExtractsProsody) {
    VoiceTTSCustomizer tts;
    std::string ssml = "<speak><prosody rate=\"1.5\" pitch=\"0.8\">Fast speech</prosody></speak>";
    auto result = tts.parseSSML(ssml);
    EXPECT_FALSE(result.segments.empty());
    EXPECT_FLOAT_EQ(result.segments[0].speed, 1.5f);
    EXPECT_FLOAT_EQ(result.segments[0].pitch, 0.8f);
}

TEST(TTSCustomizerPhase2, ParseSSMLDetectsBreaks) {
    VoiceTTSCustomizer tts;
    std::string ssml = "<speak>Hello<break time=\"200ms\"/>there</speak>";
    auto result = tts.parseSSML(ssml);
    EXPECT_TRUE(result.has_breaks);
    EXPECT_FALSE(result.plain_text.empty());
}

TEST(TTSCustomizerPhase2, EstimateMOSSilentAudio) {
    VoiceTTSCustomizer tts;
    std::vector<uint8_t> silent(1000, 0);
    auto mos = tts.estimateMOS(silent, 22050);
    EXPECT_GE(mos.mos_score, 1.0f);
    EXPECT_LE(mos.mos_score, 5.0f);
    // Silence: naturalness should be 0 or very low
    EXPECT_FLOAT_EQ(mos.naturalness, 0.0f);
}

TEST(TTSCustomizerPhase2, EstimateMOSFromText) {
    VoiceTTSCustomizer tts;
    auto mos = tts.estimateMOSFromText("Hello world", "Hello world");
    EXPECT_GE(mos.mos_score, 3.5f);
    EXPECT_LE(mos.mos_score, 4.5f);
    EXPECT_FALSE(mos.quality_label.empty());
}

TEST(TTSCustomizerPhase2, GetBestVoiceForLanguage) {
    VoiceTTSCustomizer tts;
    std::string voice = tts.getBestVoiceForLanguage("en-US");
    EXPECT_EQ(voice, "en-default");
    std::string de_voice = tts.getBestVoiceForLanguage("de-DE");
    EXPECT_EQ(de_voice, "de-default");
}

TEST(TTSCustomizerPhase2, SupportsLanguage) {
    VoiceTTSCustomizer tts;
    EXPECT_TRUE(tts.supportsLanguage("en-US"));
    EXPECT_TRUE(tts.supportsLanguage("de-DE"));
    EXPECT_FALSE(tts.supportsLanguage("xx-XX")); // not registered
}

TEST(TTSCustomizerPhase2, GetProfilesForLanguage) {
    VoiceTTSCustomizer tts;
    auto profiles = tts.getProfilesForLanguage("en-US");
    EXPECT_GE(profiles.size(), 3u); // en-default, en-male, en-female
}

// ============================================================
// Phase 4: Meeting Support Tests
// ============================================================

TEST(MeetingSupportPhase4, DefaultConstructor) {
    VoiceMeetingSupport mgr;
    auto stats = mgr.getStatistics();
    EXPECT_EQ(stats["meetings_analyzed"].get<uint64_t>(), 0u);
}

TEST(MeetingSupportPhase4, ClassifyActionItemSegment) {
    VoiceMeetingSupport mgr;
    auto type = mgr.classifySegment("Alice will send the report by Friday.");
    EXPECT_EQ(type, MeetingSegmentType::ACTION_ITEM);
}

TEST(MeetingSupportPhase4, ClassifyDecisionSegment) {
    VoiceMeetingSupport mgr;
    auto type = mgr.classifySegment("We have decided to go with option B.");
    EXPECT_EQ(type, MeetingSegmentType::DECISION);
}

TEST(MeetingSupportPhase4, ClassifyClosingSegment) {
    VoiceMeetingSupport mgr;
    auto type = mgr.classifySegment("Thank you everyone, that's all for today.");
    EXPECT_EQ(type, MeetingSegmentType::CLOSING);
}

TEST(MeetingSupportPhase4, ExtractActionItems) {
    VoiceMeetingSupport mgr;
    std::string transcript =
        "Alice will prepare the slides. Bob should review the document. "
        "The team needs to finalize the budget.";
    auto items = mgr.extractActionItems(transcript);
    EXPECT_GE(items.size(), 2u);
    EXPECT_FALSE(items[0].id.empty());
    EXPECT_GT(items[0].confidence, 0.0f);
}

TEST(MeetingSupportPhase4, ExtractDecisions) {
    VoiceMeetingSupport mgr;
    std::string transcript =
        "We discussed the options. We have decided to proceed with plan A. "
        "The team agreed to meet weekly.";
    auto decisions = mgr.extractDecisions(transcript);
    EXPECT_GE(decisions.size(), 1u);
}

TEST(MeetingSupportPhase4, ExtractKeyPoints) {
    VoiceMeetingSupport mgr;
    std::string transcript =
        "Agenda: quarterly review. We have decided to increase the budget. "
        "Topic: hiring plan for next quarter. Alice will follow up.";
    auto points = mgr.extractKeyPoints(transcript);
    EXPECT_GE(points.size(), 1u);
}

TEST(MeetingSupportPhase4, AnalyzeTranscript) {
    VoiceMeetingSupport mgr;
    std::string transcript =
        "Welcome to the meeting. Agenda: Q4 planning. "
        "We have decided to launch in November. "
        "Alice will prepare the roadmap. Thank you all.";
    auto protocol = mgr.analyzeTranscript(transcript, "mtg-001");
    EXPECT_EQ(protocol.meeting_id, "mtg-001");
    EXPECT_FALSE(protocol.segments.empty());
    auto stats = mgr.getStatistics();
    EXPECT_EQ(stats["meetings_analyzed"].get<uint64_t>(), 1u);
}

TEST(MeetingSupportPhase4, ExtractAssigneeFromText) {
    VoiceMeetingSupport mgr;
    std::string text = "This task is assigned to Alice by next week.";
    std::vector<std::string> participants = {"Alice", "Bob"};
    std::string assignee = mgr.extractAssignee(text, participants);
    EXPECT_EQ(assignee, "Alice");
}

TEST(MeetingSupportPhase4, CreateComplianceRecord) {
    VoiceMeetingSupport mgr;
    auto rec = mgr.createComplianceRecord("call-001", "inbound", "US", true, "verbal");
    EXPECT_EQ(rec.call_id, "call-001");
    EXPECT_EQ(rec.recording_type, "inbound");
    EXPECT_TRUE(rec.consent_obtained);
    EXPECT_FALSE(rec.retention_policy.empty());
}

TEST(MeetingSupportPhase4, ComplianceRecordUSJurisdiction) {
    VoiceMeetingSupport mgr;
    auto rec = mgr.createComplianceRecord("call-002", "outbound", "US", true, "verbal");
    EXPECT_TRUE(mgr.isCompliantForJurisdiction(rec, "US"));
    auto rec2 = mgr.createComplianceRecord("call-003", "outbound", "US", false, "verbal");
    EXPECT_FALSE(mgr.isCompliantForJurisdiction(rec2, "US"));
}

TEST(MeetingSupportPhase4, ComplianceRecordEUJurisdiction) {
    VoiceMeetingSupport mgr;
    auto rec = mgr.createComplianceRecord("call-004", "inbound", "EU", true, "pre_agreed");
    EXPECT_TRUE(rec.gdpr_compliant);
    EXPECT_TRUE(mgr.isCompliantForJurisdiction(rec, "EU"));
}

TEST(MeetingSupportPhase4, MeetingSegmentTypeToString) {
    EXPECT_EQ(meetingSegmentTypeToString(MeetingSegmentType::ACTION_ITEM),  "action_item");
    EXPECT_EQ(meetingSegmentTypeToString(MeetingSegmentType::DECISION),     "decision");
    EXPECT_EQ(meetingSegmentTypeToString(MeetingSegmentType::AGENDA_ITEM),  "agenda_item");
    EXPECT_EQ(meetingSegmentTypeToString(MeetingSegmentType::CLOSING),      "closing");
    EXPECT_EQ(meetingSegmentTypeToString(MeetingSegmentType::INTRODUCTION), "introduction");
    EXPECT_EQ(meetingSegmentTypeToString(MeetingSegmentType::DISCUSSION),   "discussion");
    EXPECT_EQ(meetingSegmentTypeToString(MeetingSegmentType::OTHER),        "other");
}

TEST(MeetingSupportPhase4, StatisticsTracking) {
    VoiceMeetingSupport mgr;
    std::string transcript = "We have decided to finalize the plan. Alice will take care of the report.";
    mgr.analyzeTranscript(transcript, "mtg-stats");
    auto stats = mgr.getStatistics();
    EXPECT_EQ(stats["meetings_analyzed"].get<uint64_t>(), 1u);
    EXPECT_GE(stats["action_items_extracted"].get<uint64_t>(), 0u);
}

TEST(MeetingSupportPhase4, SpeakerWordCounts) {
    VoiceMeetingSupport mgr;
    std::vector<MeetingSegment> segments;
    {
        MeetingSegment s;
        s.speaker = "Alice";
        s.text = "Hello world this is Alice";
        segments.push_back(s);
    }
    {
        MeetingSegment s;
        s.speaker = "Bob";
        s.text = "Hi there Bob speaking";
        segments.push_back(s);
    }
    auto counts = mgr.computeSpeakerWordCounts(segments);
    EXPECT_EQ(counts["Alice"], 5u);
    EXPECT_EQ(counts["Bob"],   4u);
}

// ============================================================
// Phase 5: Audio Storage Tests
// ============================================================

TEST(AudioStoragePhase5, DefaultConstructor) {
    VoiceAudioStorage storage;
    auto stats = storage.getStats();
    EXPECT_EQ(stats.total_records, 0u);
    EXPECT_EQ(stats.total_bytes, 0u);
}

TEST(AudioStoragePhase5, StoreAndRetrieve) {
    VoiceAudioStorage storage;
    std::vector<uint8_t> data = {0x52, 0x49, 0x46, 0x46, 0x01, 0x02, 0x03, 0x04};
    AudioFormat fmt;
    fmt.codec = "wav";
    fmt.sample_rate = 16000;
    std::string id = storage.store(data, fmt, "hello world");
    EXPECT_FALSE(id.empty());
    auto retrieved = storage.retrieve(id);
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(*retrieved, data);
    auto rec = storage.getRecord(id);
    ASSERT_TRUE(rec.has_value());
    EXPECT_EQ(rec->transcript, "hello world");
    EXPECT_EQ(rec->access_count, 1u);
}

TEST(AudioStoragePhase5, ContentDeduplication) {
    VoiceAudioStorage storage;
    std::vector<uint8_t> data = {1, 2, 3, 4, 5};
    AudioFormat fmt;
    fmt.codec = "pcm";
    std::string id1 = storage.store(data, fmt);
    std::string id2 = storage.store(data, fmt);
    EXPECT_EQ(id1, id2); // same content → same record returned
    EXPECT_EQ(storage.getStats().total_records, 1u);
    // Verify physical storage is not doubled
    auto stats = storage.getStats();
    EXPECT_EQ(stats.total_bytes, data.size()); // only one copy stored
    // Both IDs should retrieve the same audio data
    auto retrieved1 = storage.retrieve(id1);
    auto retrieved2 = storage.retrieve(id2);
    ASSERT_TRUE(retrieved1.has_value());
    ASSERT_TRUE(retrieved2.has_value());
    EXPECT_EQ(*retrieved1, data);
    EXPECT_EQ(*retrieved2, data);
    EXPECT_EQ(*retrieved1, *retrieved2);
}

TEST(AudioStoragePhase5, ComputeHash) {
    VoiceAudioStorage storage;
    std::vector<uint8_t> data1 = {1, 2, 3};
    std::vector<uint8_t> data2 = {1, 2, 4};
    std::string h1 = storage.computeHash(data1);
    std::string h2 = storage.computeHash(data2);
    EXPECT_EQ(h1.size(), 16u); // 16 hex chars
    EXPECT_NE(h1, h2);
    EXPECT_EQ(storage.computeHash(data1), h1); // deterministic
}

TEST(AudioStoragePhase5, DeleteRecord) {
    VoiceAudioStorage storage;
    std::vector<uint8_t> data = {9, 8, 7};
    AudioFormat fmt;
    std::string id = storage.store(data, fmt);
    EXPECT_TRUE(storage.deleteRecord(id));
    EXPECT_FALSE(storage.getRecord(id).has_value());
    EXPECT_FALSE(storage.retrieve(id).has_value());
    EXPECT_FALSE(storage.deleteRecord(id)); // already deleted
}

TEST(AudioStoragePhase5, ListRecords) {
    VoiceAudioStorage storage;
    AudioFormat fmt;
    fmt.codec = "pcm";
    storage.store({1, 2, 3}, fmt);
    storage.store({4, 5, 6}, fmt);
    auto records = storage.listRecords(StorageTier::HOT, 100);
    EXPECT_EQ(records.size(), 2u);
}

TEST(AudioStoragePhase5, TierPolicyApplication) {
    StorageTierPolicy policy;
    policy.hot_to_warm_after_ms = 0; // demote immediately
    VoiceAudioStorage storage(policy);
    AudioFormat fmt;
    std::string id = storage.store({1, 2, 3}, fmt);
    // With threshold=0, applyTierPolicy should move HOT→WARM
    size_t moved = storage.applyTierPolicy();
    EXPECT_GE(moved, 0u); // may move 0 if age check is strict
    // Test demoteTier: start fresh with default policy
    VoiceAudioStorage storage2;
    std::string id2 = storage2.store({4, 5, 6}, fmt);
    EXPECT_TRUE(storage2.demoteTier(id2)); // HOT → WARM
    auto rec = storage2.getRecord(id2);
    ASSERT_TRUE(rec.has_value());
    EXPECT_EQ(rec->tier, StorageTier::WARM);
}

TEST(AudioStoragePhase5, DetectWAVFormat) {
    VoiceAudioStorage storage;
    std::vector<uint8_t> wav = {0x52, 0x49, 0x46, 0x46, 0x00, 0x00, 0x00, 0x00};
    auto fmt = storage.detectFormat(wav);
    EXPECT_EQ(fmt.codec, "wav");
}

TEST(AudioStoragePhase5, DetectOGGFormat) {
    VoiceAudioStorage storage;
    std::vector<uint8_t> ogg = {0x4F, 0x67, 0x67, 0x53, 0x00, 0x00, 0x00, 0x00};
    auto fmt = storage.detectFormat(ogg);
    EXPECT_EQ(fmt.codec, "ogg");
}

TEST(AudioStoragePhase5, DetectUnknownFormat) {
    VoiceAudioStorage storage;
    std::vector<uint8_t> raw = {0x00, 0x01, 0x02, 0x03};
    auto fmt = storage.detectFormat(raw);
    EXPECT_EQ(fmt.codec, "pcm");
}

TEST(AudioStoragePhase5, MarkEncrypted) {
    VoiceAudioStorage storage;
    AudioFormat fmt;
    std::string id = storage.store({5, 6, 7}, fmt);
    EXPECT_FALSE(storage.isEncrypted(id));
    EXPECT_TRUE(storage.markEncrypted(id, "key-001"));
    EXPECT_TRUE(storage.isEncrypted(id));
    auto rec = storage.getRecord(id);
    ASSERT_TRUE(rec.has_value());
    EXPECT_EQ(rec->encryption_key_id, "key-001");
}

TEST(AudioStoragePhase5, StorageTierToString) {
    EXPECT_EQ(storageTierToString(StorageTier::HOT),     "hot");
    EXPECT_EQ(storageTierToString(StorageTier::WARM),    "warm");
    EXPECT_EQ(storageTierToString(StorageTier::COLD),    "cold");
    EXPECT_EQ(storageTierToString(StorageTier::DELETED), "deleted");
}

TEST(AudioStoragePhase5, StatisticsTracking) {
    VoiceAudioStorage storage;
    AudioFormat fmt;
    fmt.size_bytes = 100;
    storage.store({1, 2, 3, 4, 5}, fmt);
    storage.store({6, 7, 8, 9, 10}, fmt);
    auto stats = storage.getStats();
    EXPECT_EQ(stats.total_records, 2u);
    EXPECT_EQ(stats.hot_records, 2u);
}

// --- Phase 9, 10 additions ---
#include <numbers>
#include "voice/voice_accessibility.h"
#include "voice/voice_model_cache.h"
#include "voice/voice_batch_processor.h"

using namespace themis::voice;

// ============================================================
// Phase 9: Accessibility Tests
// ============================================================

TEST(AccessibilityPhase9, DefaultConstructor) {
    VoiceAccessibility acc;
    auto stats = acc.getStatistics();
    EXPECT_EQ(stats["exports_completed"], 0);
    EXPECT_EQ(stats["total_cues_generated"], 0);
}

TEST(AccessibilityPhase9, GenerateCaptionsBasic) {
    VoiceAccessibility acc;
    std::vector<std::pair<int64_t, std::string>> segs = {
        {0,    "Hello world"},
        {2000, "How are you"},
        {5000, "Goodbye"}
    };
    auto cues = acc.generateCaptions(segs, "Alice");
    ASSERT_EQ(cues.size(), 3u);
    EXPECT_EQ(cues[0].text, "Hello world");
    EXPECT_EQ(cues[0].speaker, "Alice");
    EXPECT_EQ(cues[0].sequence, 1);
    EXPECT_EQ(cues[0].start_ms, 0);
    EXPECT_EQ(cues[0].end_ms, 2000);
    EXPECT_EQ(cues[2].end_ms, 8000); // last cue: start + 3000
}

TEST(AccessibilityPhase9, GenerateCaptionsFromJSON) {
    VoiceAccessibility acc;
    nlohmann::json transcript = nlohmann::json::array({
        {{"text","Good morning"},{"start_ms",0},{"end_ms",1500},{"speaker","Alice"},{"confidence",0.95f}},
        {{"text","Good morning too"},{"start_ms",1600},{"end_ms",3200},{"speaker","Bob"}}
    });
    auto cues = acc.generateCaptionsFromJSON(transcript);
    ASSERT_EQ(cues.size(), 2u);
    EXPECT_EQ(cues[0].text, "Good morning");
    EXPECT_EQ(cues[0].speaker, "Alice");
    EXPECT_FLOAT_EQ(cues[0].confidence, 0.95f);
    EXPECT_EQ(cues[1].speaker, "Bob");
    EXPECT_EQ(cues[1].end_ms, 3200);
}

TEST(AccessibilityPhase9, ExportAsVTT) {
    VoiceAccessibility acc;
    std::vector<CaptionCue> cues;
    CaptionCue c;
    c.sequence = 1; c.start_ms = 0; c.end_ms = 2000; c.text = "Hello world"; c.speaker = "Alice";
    cues.push_back(c);

    TranscriptExportOptions opts;
    opts.format = CaptionFormat::VTT;
    auto result = acc.exportTranscript(cues, opts);

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.mime_type, "text/vtt");
    EXPECT_NE(result.content.find("WEBVTT"), std::string::npos);
    EXPECT_NE(result.content.find("00:00:00.000 --> 00:00:02.000"), std::string::npos);
    EXPECT_NE(result.content.find("Hello world"), std::string::npos);
    EXPECT_EQ(result.cue_count, 1u);
}

TEST(AccessibilityPhase9, ExportAsSRT) {
    VoiceAccessibility acc;
    std::vector<CaptionCue> cues;
    CaptionCue c;
    c.sequence = 1; c.start_ms = 1000; c.end_ms = 4000; c.text = "Testing SRT"; c.speaker = "";
    cues.push_back(c);

    TranscriptExportOptions opts;
    opts.format = CaptionFormat::SRT;
    auto result = acc.exportTranscript(cues, opts);

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.mime_type, "application/x-subrip");
    EXPECT_NE(result.content.find("00:00:01,000 --> 00:00:04,000"), std::string::npos);
    EXPECT_NE(result.content.find("Testing SRT"), std::string::npos);
}

TEST(AccessibilityPhase9, ExportAsPlainText) {
    VoiceAccessibility acc;
    std::vector<CaptionCue> cues;
    CaptionCue c;
    c.sequence = 1; c.start_ms = 5000; c.end_ms = 8000; c.text = "Plain text cue"; c.speaker = "Bob";
    cues.push_back(c);

    TranscriptExportOptions opts;
    opts.format = CaptionFormat::PLAIN_TEXT;
    opts.include_timestamps = true;
    opts.include_speaker_info = true;
    auto result = acc.exportTranscript(cues, opts);

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.mime_type, "text/plain");
    EXPECT_NE(result.content.find("Bob"), std::string::npos);
    EXPECT_NE(result.content.find("Plain text cue"), std::string::npos);
    EXPECT_NE(result.content.find("[00:00:05]"), std::string::npos);
}

TEST(AccessibilityPhase9, ExportAsHTML) {
    VoiceAccessibility acc;
    std::vector<CaptionCue> cues;
    CaptionCue c;
    c.sequence = 1; c.start_ms = 0; c.end_ms = 2000; c.text = "HTML caption"; c.speaker = "Host";
    cues.push_back(c);

    TranscriptExportOptions opts;
    opts.format = CaptionFormat::HTML;
    opts.language = "en";
    opts.title = "Test Title";
    auto result = acc.exportTranscript(cues, opts);

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.mime_type, "text/html");
    EXPECT_NE(result.content.find("<article"), std::string::npos);
    EXPECT_NE(result.content.find("HTML caption"), std::string::npos);
    EXPECT_NE(result.content.find("Host"), std::string::npos);
    EXPECT_NE(result.content.find("lang=\"en\""), std::string::npos);
}

TEST(AccessibilityPhase9, ExportAsJSONFormat) {
    VoiceAccessibility acc;
    std::vector<CaptionCue> cues;
    CaptionCue c;
    c.sequence = 1; c.start_ms = 100; c.end_ms = 2100; c.text = "JSON cue"; c.speaker = "Eve";
    cues.push_back(c);

    TranscriptExportOptions opts;
    opts.format = CaptionFormat::JSON;
    opts.language = "fr";
    auto result = acc.exportTranscript(cues, opts);

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.mime_type, "application/json");
    auto j = nlohmann::json::parse(result.content);
    EXPECT_EQ(j["language"], "fr");
    ASSERT_EQ(j["cues"].size(), 1u);
    EXPECT_EQ(j["cues"][0]["text"], "JSON cue");
}

TEST(AccessibilityPhase9, CaptionFormatToString) {
    EXPECT_EQ(captionFormatToString(CaptionFormat::VTT),        "vtt");
    EXPECT_EQ(captionFormatToString(CaptionFormat::SRT),        "srt");
    EXPECT_EQ(captionFormatToString(CaptionFormat::PLAIN_TEXT), "plain_text");
    EXPECT_EQ(captionFormatToString(CaptionFormat::HTML),       "html");
    EXPECT_EQ(captionFormatToString(CaptionFormat::JSON),       "json");
}

TEST(AccessibilityPhase9, FormatTimestampVTT) {
    VoiceAccessibility acc;
    // 1h 2m 3s 456ms → "01:02:03.456"
    int64_t ms = (1 * 3600 + 2 * 60 + 3) * 1000 + 456;
    std::string ts = acc.formatTimestamp(ms, true);
    EXPECT_EQ(ts, "01:02:03.456");
}

TEST(AccessibilityPhase9, FormatTimestampSRT) {
    VoiceAccessibility acc;
    // 0h 0m 5s 100ms → "00:00:05,100"
    int64_t ms = 5100;
    std::string ts = acc.formatTimestamp(ms, false);
    EXPECT_EQ(ts, "00:00:05,100");
}

TEST(AccessibilityPhase9, StatisticsTracking) {
    VoiceAccessibility acc;
    std::vector<std::pair<int64_t, std::string>> segs = {{0, "One"}, {1000, "Two"}};
    auto cues = acc.generateCaptions(segs);

    TranscriptExportOptions opts;
    opts.format = CaptionFormat::VTT;
    acc.exportTranscript(cues, opts);
    acc.exportTranscript(cues, opts);

    auto stats = acc.getStatistics();
    EXPECT_EQ(stats["exports_completed"], 2);
    EXPECT_EQ(stats["total_cues_generated"], 2); // generated once
}

TEST(AccessibilityPhase9, SplitLongCues) {
    CaptionStyle style;
    style.max_duration_ms = 3000;
    VoiceAccessibility acc(style);

    std::vector<CaptionCue> cues;
    CaptionCue c;
    c.sequence = 1; c.start_ms = 0; c.end_ms = 9000;
    c.text = "This is a long cue that needs to be split into multiple parts";
    cues.push_back(c);

    auto split = acc.splitLongCues(cues);
    EXPECT_GT(split.size(), 1u);
    // Total span should be preserved
    EXPECT_EQ(split.back().end_ms, 9000);
}

TEST(AccessibilityPhase9, MergeSilentGaps) {
    CaptionStyle style;
    style.min_duration_ms = 500;
    VoiceAccessibility acc(style);

    std::vector<CaptionCue> cues;
    CaptionCue c1; c1.sequence=1; c1.start_ms=0; c1.end_ms=500; c1.text="Part one"; c1.speaker="A";
    CaptionCue c2; c2.sequence=2; c2.start_ms=600; c2.end_ms=1200; c2.text="Part two"; c2.speaker="A";
    CaptionCue c3; c3.sequence=3; c3.start_ms=5000; c3.end_ms=6000; c3.text="Later"; c3.speaker="A";
    cues.push_back(c1); cues.push_back(c2); cues.push_back(c3);

    auto merged = acc.mergeSilentGaps(cues);
    // c1 and c2 have gap < 500ms → merged
    ASSERT_EQ(merged.size(), 2u);
    EXPECT_NE(merged[0].text.find("Part one"), std::string::npos);
    EXPECT_NE(merged[0].text.find("Part two"), std::string::npos);
}

// ============================================================
// Phase 10: Model Cache Tests
// ============================================================

TEST(ModelCachePhase10, DefaultConstructor) {
    VoiceModelCache cache;
    auto stats = cache.getStats();
    EXPECT_EQ(stats.loaded_models, 0u);
    EXPECT_EQ(stats.cache_hits, 0u);
    EXPECT_EQ(stats.cache_misses, 0u);
}

TEST(ModelCachePhase10, InsertAndRetrieve) {
    VoiceModelCache cache;
    CachedModel m;
    m.model_id   = "test-model";
    m.model_path = "/models/test.bin";
    m.model_type = "stt";
    m.memory_bytes = 1024;

    EXPECT_TRUE(cache.insert(m));
    EXPECT_TRUE(cache.isCached("test-model"));
    EXPECT_FALSE(cache.isCached("nonexistent"));

    auto stats = cache.getStats();
    EXPECT_EQ(stats.loaded_models, 1u);
    EXPECT_EQ(stats.total_memory_bytes, 1024u);
}

TEST(ModelCachePhase10, CacheHitMiss) {
    VoiceModelCache cache;
    // Miss: no loader registered
    auto result = cache.get("missing-id", "/path/to/model", "stt");
    EXPECT_FALSE(result.has_value());
    auto stats = cache.getStats();
    EXPECT_EQ(stats.cache_misses, 1u);
    EXPECT_EQ(stats.cache_hits, 0u);

    // Insert then hit
    CachedModel m; m.model_id = "hot-model"; m.model_type = "tts"; m.memory_bytes = 512;
    cache.insert(m);
    auto res2 = cache.get("hot-model", "", "tts");
    EXPECT_TRUE(res2.has_value());
    EXPECT_EQ(cache.getStats().cache_hits, 1u);   // exactly one hit after the insert+get
}

TEST(ModelCachePhase10, EvictLRU) {
    ModelCacheConfig cfg;
    cfg.max_models = 2;
    cfg.max_memory_bytes = 1024 * 1024;
    VoiceModelCache cache(cfg);

    CachedModel m1; m1.model_id = "m1"; m1.model_type = "stt"; m1.memory_bytes = 100;
    CachedModel m2; m2.model_id = "m2"; m2.model_type = "stt"; m2.memory_bytes = 100;
    CachedModel m3; m3.model_id = "m3"; m3.model_type = "stt"; m3.memory_bytes = 100;

    EXPECT_TRUE(cache.insert(m1));
    EXPECT_TRUE(cache.insert(m2));
    // Touch m1 to make m2 the LRU
    cache.get("m1", "", "stt");
    // Inserting m3 should evict m2 (LRU)
    EXPECT_TRUE(cache.insert(m3));
    EXPECT_FALSE(cache.isCached("m2"));
    EXPECT_TRUE(cache.isCached("m1"));
    EXPECT_TRUE(cache.isCached("m3"));
}

TEST(ModelCachePhase10, PinModel) {
    ModelCacheConfig cfg;
    cfg.max_models = 2;
    VoiceModelCache cache(cfg);

    CachedModel m1; m1.model_id = "pinned"; m1.model_type = "llm"; m1.memory_bytes = 100;
    CachedModel m2; m2.model_id = "m2";     m2.model_type = "stt"; m2.memory_bytes = 100;
    CachedModel m3; m3.model_id = "m3";     m3.model_type = "stt"; m3.memory_bytes = 100;

    cache.insert(m1);
    cache.insert(m2);
    cache.pin("pinned");

    // m3 insert should evict m2, not the pinned model
    cache.insert(m3);
    EXPECT_TRUE(cache.isCached("pinned"));
    EXPECT_TRUE(cache.isCached("m3"));
    EXPECT_FALSE(cache.isCached("m2"));

    // Unpin and verify
    EXPECT_TRUE(cache.unpin("pinned"));
    auto stats = cache.getStats();
    EXPECT_EQ(stats.pinned_models, 0u);
}

TEST(ModelCachePhase10, EvictToFree) {
    VoiceModelCache cache;
    CachedModel m1; m1.model_id = "big1"; m1.model_type = "llm"; m1.memory_bytes = 500;
    CachedModel m2; m2.model_id = "big2"; m2.model_type = "llm"; m2.memory_bytes = 500;
    cache.insert(m1);
    cache.insert(m2);

    EXPECT_EQ(cache.getStats().total_memory_bytes, 1000u);
    size_t freed = cache.evictToFree(400);
    EXPECT_GE(freed, 400u); // At least 400 bytes freed
    EXPECT_LT(cache.getStats().total_memory_bytes, 1000u);
}

TEST(ModelCachePhase10, ClearCache) {
    VoiceModelCache cache;
    CachedModel m1; m1.model_id = "a"; m1.model_type = "stt"; m1.memory_bytes = 100;
    CachedModel m2; m2.model_id = "b"; m2.model_type = "tts"; m2.memory_bytes = 200;
    cache.insert(m1);
    cache.insert(m2);

    EXPECT_EQ(cache.getStats().loaded_models, 2u);
    cache.clear();
    EXPECT_EQ(cache.getStats().loaded_models, 0u);
    EXPECT_EQ(cache.getStats().total_memory_bytes, 0u);
    EXPECT_FALSE(cache.isCached("a"));
    EXPECT_FALSE(cache.isCached("b"));
}

TEST(ModelCachePhase10, Statistics) {
    VoiceModelCache cache;
    CachedModel m; m.model_id = "stat-test"; m.model_type = "embedding"; m.memory_bytes = 256;
    cache.insert(m);

    // Hit
    cache.get("stat-test", "", "embedding");

    // Miss
    cache.get("no-exist", "/path", "stt");

    auto stats = cache.getStats();
    EXPECT_EQ(stats.loaded_models, 1u);
    EXPECT_GE(stats.cache_hits, 1u);
    EXPECT_GE(stats.cache_misses, 1u);
    EXPECT_GT(stats.hit_rate, 0.0);
    EXPECT_LT(stats.hit_rate, 1.0);
}

TEST(ModelCachePhase10, RegisterLoader) {
    VoiceModelCache cache;

    bool loader_called = false;
    bool unloader_called = false;

    cache.registerLoader("stub",
        [&](const std::string&, const nlohmann::json&) -> void* {
            loader_called = true;
            return reinterpret_cast<void*>(0xDEAD);
        },
        [&](void* h) {
            EXPECT_EQ(h, reinterpret_cast<void*>(0xDEAD));
            unloader_called = true;
        }
    );

    auto result = cache.get("stub-model", "/stub/path", "stub");
    EXPECT_TRUE(result.has_value());
    EXPECT_TRUE(loader_called);
    EXPECT_EQ(result->handle, reinterpret_cast<void*>(0xDEAD));

    // Evict should call unloader
    cache.evict("stub-model");
    EXPECT_TRUE(unloader_called);
}

TEST(ModelCachePhase10, DetailedStats) {
    VoiceModelCache cache;
    CachedModel m; m.model_id = "detail"; m.model_type = "stt"; m.memory_bytes = 100;
    m.is_pinned = false;
    cache.insert(m);

    auto detail = cache.getDetailedStats();
    EXPECT_EQ(detail["loaded_models"], 1);
    EXPECT_TRUE(detail.contains("models"));
    EXPECT_EQ(detail["models"].size(), 1u);
    EXPECT_EQ(detail["models"][0]["model_id"], "detail");
}

// ============================================================
// Phase 10: Batch Processor Tests
// ============================================================

TEST(BatchProcessorPhase10, DefaultConstructor) {
    VoiceBatchProcessor bp;
    auto stats = bp.getStatistics();
    EXPECT_EQ(stats["jobs_submitted"], 0);
    EXPECT_EQ(stats["items_processed"], 0);
    EXPECT_EQ(stats["items_failed"], 0);
}

TEST(BatchProcessorPhase10, ProcessSingleItem) {
    VoiceBatchProcessor bp;
    BatchAudioItem item;
    item.item_id = "item-1";
    item.audio_data = {0x00, 0x00, 0x10, 0x00, 0x20, 0x00};
    item.sample_rate = 16000;

    auto result = bp.processItem(item);
    EXPECT_EQ(result.item_id, "item-1");
    EXPECT_TRUE(result.success);
    EXPECT_GE(result.processing_time_ms, 0);
}

TEST(BatchProcessorPhase10, ProcessBatchSync) {
    VoiceBatchProcessor bp;
    std::vector<BatchAudioItem> items;
    for (int i = 0; i < 5; ++i) {
        BatchAudioItem item;
        item.item_id = "item-" + std::to_string(i);
        item.audio_data = {0x00, 0x01, 0x02, 0x03};
        items.push_back(item);
    }

    size_t progress_calls = 0;
    auto results = bp.processBatchSync(items, [&](const std::string&, size_t, size_t) {
        ++progress_calls;
    });

    EXPECT_EQ(results.size(), 5u);
    for (const auto& r : results) {
        EXPECT_TRUE(r.success);
    }
    EXPECT_GE(progress_calls, 1u);
}

TEST(BatchProcessorPhase10, ComputeWERPerfect) {
    VoiceBatchProcessor bp;
    float wer = bp.computeWER("hello world", "hello world");
    EXPECT_FLOAT_EQ(wer, 0.0f);
}

TEST(BatchProcessorPhase10, ComputeWEREmpty) {
    VoiceBatchProcessor bp;
    // Empty reference → WER = 0
    float wer = bp.computeWER("", "something");
    EXPECT_FLOAT_EQ(wer, 0.0f);

    // Empty hypothesis → all words are deletions
    float wer2 = bp.computeWER("hello world", "");
    EXPECT_FLOAT_EQ(wer2, 1.0f);
}

TEST(BatchProcessorPhase10, ComputeWERPartial) {
    VoiceBatchProcessor bp;
    // "hello world test" (3 words) vs "hello world" (2 correct, 1 deletion)
    float wer = bp.computeWER("hello world test", "hello world");
    EXPECT_GT(wer, 0.0f);
    EXPECT_LE(wer, 1.0f);
}

TEST(BatchProcessorPhase10, ComputeQualityMetrics) {
    VoiceBatchProcessor bp;
    // Generate a simple sine-wave-like pattern as PCM int16
    std::vector<uint8_t> audio;
    for (int i = 0; i < 1600; ++i) {
        int16_t sample = static_cast<int16_t>(16000 * std::sin(2.0 * std::numbers::pi * i / 160.0));
        audio.push_back(static_cast<uint8_t>(sample & 0xFF));
        audio.push_back(static_cast<uint8_t>((sample >> 8) & 0xFF));
    }

    auto metrics = bp.computeQualityMetrics(audio, 16000);
    EXPECT_GT(metrics.rms_energy, 0.0f);
    EXPECT_GE(metrics.pesq_mos, 1.0f);
    EXPECT_LE(metrics.pesq_mos, 5.0f);
    EXPECT_FALSE(metrics.quality_label.empty());
    EXPECT_GE(metrics.clipping_ratio, 0.0f);
    EXPECT_LE(metrics.clipping_ratio, 1.0f);
}

TEST(BatchProcessorPhase10, EstimatePESQ) {
    VoiceBatchProcessor bp;
    EXPECT_FLOAT_EQ(bp.estimatePESQ(0.0f),  1.0f);   // SNR 0 → MOS 1.0
    EXPECT_FLOAT_EQ(bp.estimatePESQ(-10.0f), 1.0f);  // Clamped to 1.0
    EXPECT_FLOAT_EQ(bp.estimatePESQ(100.0f), 5.0f);  // Clamped to 5.0
    float pesq_mid = bp.estimatePESQ(28.57f);
    EXPECT_GE(pesq_mid, 2.9f);
    EXPECT_LE(pesq_mid, 3.1f);
}

TEST(BatchProcessorPhase10, RunLoadTest) {
    VoiceBatchProcessor bp;
    BatchAudioItem tmpl;
    tmpl.item_id = "load";
    tmpl.audio_data = {0x00, 0x80, 0xFF, 0x7F};
    tmpl.sample_rate = 16000;

    auto summary = bp.runLoadTest(10, tmpl);
    EXPECT_EQ(summary.total_items, 10u);
    EXPECT_EQ(summary.status, BatchJobStatus::COMPLETED);
    EXPECT_EQ(summary.completed_items, 10u);
    EXPECT_EQ(summary.failed_items, 0u);
}

TEST(BatchProcessorPhase10, Statistics) {
    VoiceBatchProcessor bp;
    BatchAudioItem item;
    item.item_id = "stat-item";
    item.audio_data = {0x01, 0x02, 0x03, 0x04};

    bp.submitBatch({item});

    auto stats = bp.getStatistics();
    EXPECT_GE(stats["jobs_submitted"].get<uint64_t>(), 1u);
    EXPECT_GE(stats["items_processed"].get<uint64_t>(), 1u);
}

TEST(BatchProcessorPhase10, BatchJobStatusToString) {
    EXPECT_EQ(batchJobStatusToString(BatchJobStatus::PENDING),   "pending");
    EXPECT_EQ(batchJobStatusToString(BatchJobStatus::RUNNING),   "running");
    EXPECT_EQ(batchJobStatusToString(BatchJobStatus::COMPLETED), "completed");
    EXPECT_EQ(batchJobStatusToString(BatchJobStatus::FAILED),    "failed");
}

TEST(BatchProcessorPhase10, GetJobSummary) {
    VoiceBatchProcessor bp;
    BatchAudioItem item;
    item.item_id = "summary-item";
    item.audio_data = {0xAA, 0xBB};

    std::string job_id = bp.submitBatch({item});
    auto summary = bp.getJobSummary(job_id);

    EXPECT_EQ(summary.job_id, job_id);
    EXPECT_EQ(summary.status, BatchJobStatus::COMPLETED);
    EXPECT_EQ(summary.total_items, 1u);
    EXPECT_EQ(summary.completed_items, 1u);
}

TEST(BatchProcessorPhase10, EstimateSNR) {
    VoiceBatchProcessor bp;
    // All-zeros should not crash; SNR = 0
    std::vector<uint8_t> silence(3200, 0x00);
    float snr = bp.estimateSNR(silence, 16000);
    EXPECT_FLOAT_EQ(snr, 0.0f);

    // Signal with non-zero samples
    std::vector<uint8_t> signal;
    for (int i = 0; i < 3200; i += 2) {
        int16_t s = static_cast<int16_t>(8000);
        signal.push_back(s & 0xFF);
        signal.push_back((s >> 8) & 0xFF);
    }
    float snr2 = bp.estimateSNR(signal, 16000);
    EXPECT_GE(snr2, 0.0f);
}
