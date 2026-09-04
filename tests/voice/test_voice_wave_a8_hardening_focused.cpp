#include <gtest/gtest.h>

#include "voice/voice_anti_spoof_engine.h"
#include "voice/voice_audit_logger.h"
#include "voice/voice_liveness_detector.h"
#include "voice/voice_session_manager.h"

#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <string>
#include <thread>
#include <vector>

using namespace themis::voice;

namespace {

std::string makePcmSine(int duration_ms, float amplitude = 0.35f, float frequency_hz = 220.0f) {
    const int sample_rate = 16000;
    const int sample_count = (sample_rate * duration_ms) / 1000;
    std::string pcm = {};
    pcm.reserve(static_cast<size_t>(sample_count) * 2);
    for (int i = 0; i < sample_count; ++i) {
        const float sample = amplitude * std::sin(
            2.0f * 3.1415926535f * frequency_hz * static_cast<float>(i) / static_cast<float>(sample_rate));
        const auto quantized = static_cast<int16_t>(sample * 32767.0f);
        pcm.push_back(static_cast<char>(quantized & 0xFF));
        pcm.push_back(static_cast<char>((quantized >> 8) & 0xFF));
    }
    return pcm;
}

std::string makeReplayLikePcm(int repeated_frames = 80) {
    const auto frame = makePcmSine(20, 0.35f, 220.0f);
    std::string pcm = {};
    pcm.reserve(frame.size() * static_cast<size_t>(repeated_frames));
    for (int i = 0; i < repeated_frames; ++i) {
        pcm.append(frame);
    }
    return pcm;
}

std::string makeSplicedPcm() {
    std::string pcm = makePcmSine(400, 0.35f, 220.0f);
    pcm.append(makePcmSine(400, 0.80f, 510.0f));
    pcm.append(makePcmSine(400, 0.15f, 110.0f));
    return pcm;
}

} // namespace

TEST(VoiceLivenessDetectorHardening, ValidChallengeTranscriptPasses) {
    VoiceLivenessDetector detector;
    const auto challenge = detector.issueChallenge("user-a");
    ASSERT_TRUE(challenge.has_value());

    const auto result = detector.verifyResponse(
        "user-a",
        *challenge,
        "transcript:" + challenge->text);

    EXPECT_TRUE(result.passed);
    EXPECT_EQ(result.reason, "Liveness verified");
    EXPECT_TRUE(detector.isReplayedChallenge(challenge->id));
}

TEST(VoiceLivenessDetectorHardening, StaleChallengeRejectedAndCleanedUp) {
    VoiceLivenessDetector::Config config;
    config.challenge_timeout_ms = 5;
    VoiceLivenessDetector detector(config);

    const auto challenge = detector.issueChallenge("user-b");
    ASSERT_TRUE(challenge.has_value());

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    const auto result = detector.verifyResponse("user-b", *challenge, "transcript:" + challenge->text);

    EXPECT_FALSE(result.passed);
    EXPECT_EQ(result.reason, "Challenge expired (stale)");
    EXPECT_EQ(detector.getActiveChallengeCount(), 0u);
}

TEST(VoiceLivenessDetectorHardening, ReplayAttemptIsRejectedAfterSuccessfulVerification) {
    VoiceLivenessDetector detector;
    const auto challenge = detector.issueChallenge("user-c");
    ASSERT_TRUE(challenge.has_value());

    const auto first = detector.verifyResponse("user-c", *challenge, challenge->text);
    ASSERT_TRUE(first.passed);

    const auto replay = detector.verifyResponse("user-c", *challenge, challenge->text);
    EXPECT_FALSE(replay.passed);
    EXPECT_EQ(replay.reason, "Replay attack detected (challenge already consumed)");
}

TEST(VoiceAntiSpoofEngineHardening, LivePcmBaselinePasses) {
    VoiceAntiSpoofEngine::Config config;
    config.freshness_threshold = 0.45;
    config.speaker_match_threshold = 0.70;
    config.noise_consistency_threshold = 0.45;
    VoiceAntiSpoofEngine engine(config);

    const std::string baseline = makePcmSine(800, 0.35f, 220.0f);
    const auto analysis = engine.analyzeSpoofRisk(baseline, baseline);

    EXPECT_FALSE(analysis.is_likely_spoofed);
    EXPECT_GE(analysis.audio_freshness_score, config.freshness_threshold);
    EXPECT_GE(analysis.speaker_match_score, config.speaker_match_threshold);
}

TEST(VoiceAntiSpoofEngineHardening, ReplayLikeAudioIsFlaggedAsSpoofed) {
    VoiceAntiSpoofEngine engine;
    const std::string baseline = makePcmSine(1200, 0.35f, 220.0f);
    const auto analysis = engine.analyzeSpoofRisk(makeReplayLikePcm(), baseline);

    EXPECT_TRUE(analysis.is_likely_spoofed);
    EXPECT_LT(analysis.audio_freshness_score, 0.70);
    EXPECT_NE(analysis.reason.find("freshness"), std::string::npos);
}

TEST(VoiceAntiSpoofEngineHardening, SpeakerMismatchIsFlagged) {
    VoiceAntiSpoofEngine::Config config;
    config.freshness_threshold = 0.40;
    config.speaker_match_threshold = 0.85;
    config.noise_consistency_threshold = 0.40;
    VoiceAntiSpoofEngine engine(config);

    const std::string audio = makeSplicedPcm();
    const std::string baseline = "-1.0,1.0,-1.0,1.0,-1.0,1.0,-1.0,1.0,-1.0,1.0,-1.0,1.0,-1.0,1.0,-1.0,1.0";
    const auto analysis = engine.analyzeSpoofRisk(audio, baseline);

    EXPECT_TRUE(analysis.is_likely_spoofed);
    EXPECT_LT(analysis.speaker_match_score, config.speaker_match_threshold);
}

TEST(VoiceAuditLoggerHardening, CallbackCanInspectLogWithoutDeadlock) {
    VoiceAuditLogger logger;
    std::atomic<size_t> observed_count{0};

    logger.setEventCallback([&](const nlohmann::json&) {
        observed_count.store(logger.getEventCount(), std::memory_order_relaxed);
    });

    logger.logAuthenticationAttempt("audit-user", "liveness", true, "ok", 12, "sess-1");

    EXPECT_EQ(logger.getEventCount(), 1u);
    EXPECT_EQ(observed_count.load(std::memory_order_relaxed), 1u);
}

TEST(VoiceSessionManagerHardening, TerminatedSessionsAreRemovedFailClosed) {
    VoiceSessionManager manager;
    const auto session = manager.createSession("teardown-user", "device-1");
    ASSERT_FALSE(session.session_id.empty());

    EXPECT_TRUE(manager.terminateSession(session.session_id));
    EXPECT_FALSE(manager.getSession(session.session_id).has_value());
    EXPECT_TRUE(manager.isDoubleCloseAttempt(session.session_id));
    EXPECT_TRUE(manager.isUseAfterFreeAttempt(session.session_id));
    EXPECT_GT(manager.getStateChangeTimestamp(session.session_id), 0);
}

TEST(VoiceSessionManagerHardening, MultiSessionTeardownLeavesNoActiveSessionsForUser) {
    VoiceSessionManager manager;
    std::vector<std::string> session_ids = {};

    for (int i = 0; i < 4; ++i) {
        auto session = manager.createSession("shared-user", "device-" + std::to_string(i));
        ASSERT_FALSE(session.session_id.empty());
        session_ids.push_back(session.session_id);
    }

    std::vector<std::thread> workers = {};

    for (const auto& session_id : session_ids) {
        workers.emplace_back([&manager, session_id]() {
            EXPECT_TRUE(manager.terminateSession(session_id));
        });
    }
    for (auto& worker : workers) {
        worker.join();
    }

    EXPECT_TRUE(manager.getSessionsForUser("shared-user").empty());
    for (const auto& session_id : session_ids) {
        EXPECT_FALSE(manager.sessionIdExists(session_id));
    }
}
