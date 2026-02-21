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
