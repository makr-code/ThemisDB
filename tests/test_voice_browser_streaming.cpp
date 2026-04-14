/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_voice_browser_streaming.cpp                   ║
  Version:         0.0.8                                              ║
  Last Modified:   2026-04-14 19:09:07                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     215                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 15e6e31437  2026-03-09  feat: implement all features from problem statement ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <gtest/gtest.h>
#include "voice/voice_browser_streaming.h"
#include <thread>

namespace themis {
namespace voice {
namespace {

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

static VoiceStreamingSession::Config makeConfig() {
    VoiceStreamingSession::Config cfg;
    cfg.session_id      = "test-session";
    cfg.user_id         = "test-user";
    cfg.max_frame_bytes = 65536;
    cfg.max_duration_s  = 600;
    cfg.partial_results = true;
    return cfg;
}

// ─────────────────────────────────────────────────────────────────────────────
// Construction
// ─────────────────────────────────────────────────────────────────────────────

TEST(VoiceStreamingSession, CreateSucceeds) {
    auto session = VoiceStreamingSession::create(makeConfig());
    ASSERT_NE(session, nullptr);
    EXPECT_FALSE(session->isActive());
}

TEST(VoiceStreamingSession, InvalidMaxFrameBytesThrows) {
    auto cfg = makeConfig();
    cfg.max_frame_bytes = 0;
    EXPECT_THROW(VoiceStreamingSession::create(cfg), std::invalid_argument);
}

TEST(VoiceStreamingSession, InvalidMaxDurationThrows) {
    auto cfg = makeConfig();
    cfg.max_duration_s = 0;
    EXPECT_THROW(VoiceStreamingSession::create(cfg), std::invalid_argument);
}

// ─────────────────────────────────────────────────────────────────────────────
// Lifecycle
// ─────────────────────────────────────────────────────────────────────────────

TEST(VoiceStreamingSession, StartReturnsNonEmptyStreamId) {
    auto session = VoiceStreamingSession::create(makeConfig());
    auto id = session->start();
    EXPECT_FALSE(id.empty());
    EXPECT_TRUE(session->isActive());
}

TEST(VoiceStreamingSession, StartTwiceReturnsSameId) {
    auto session = VoiceStreamingSession::create(makeConfig());
    auto id1 = session->start();
    auto id2 = session->start();
    EXPECT_EQ(id1, id2);
}

TEST(VoiceStreamingSession, EndDeactivatesSession) {
    auto session = VoiceStreamingSession::create(makeConfig());
    session->start();
    ASSERT_TRUE(session->isActive());
    session->end();
    EXPECT_FALSE(session->isActive());
}

// ─────────────────────────────────────────────────────────────────────────────
// Audio ingestion
// ─────────────────────────────────────────────────────────────────────────────

TEST(VoiceStreamingSession, SendAudioChunkTracksBytes) {
    auto session = VoiceStreamingSession::create(makeConfig());
    session->start();

    std::vector<uint8_t> chunk(1024, 0xAA);
    session->sendAudioChunk(chunk);
    EXPECT_EQ(session->bytesReceived(), 1024u);

    session->sendAudioChunk(chunk);
    EXPECT_EQ(session->bytesReceived(), 2048u);
}

TEST(VoiceStreamingSession, OversizedChunkDoesNotCrash) {
    auto cfg = makeConfig();
    cfg.max_frame_bytes = 512;
    auto session = VoiceStreamingSession::create(cfg);
    session->start();

    bool error_called = false;
    session->onError([&error_called](const std::string&){ error_called = true; });

    std::vector<uint8_t> big_chunk(1024, 0xFF);
    auto pt = session->sendAudioChunk(big_chunk);
    EXPECT_TRUE(error_called);
    EXPECT_EQ(session->bytesReceived(), 0u);  // Chunk was rejected
}

TEST(VoiceStreamingSession, PartialTranscriptCallbackInvoked) {
    auto cfg = makeConfig();
    cfg.partial_results = true;
    auto session = VoiceStreamingSession::create(cfg);
    session->start();

    bool callback_called = false;
    session->onPartialTranscript([&callback_called](const PartialTranscript& pt){
        callback_called = true;
        EXPECT_FALSE(pt.stream_id.empty());
        EXPECT_FALSE(pt.is_final);
    });

    std::vector<uint8_t> chunk(256, 0x10);
    session->sendAudioChunk(chunk);
    EXPECT_TRUE(callback_called);
}

TEST(VoiceStreamingSession, FinalTranscriptCallbackOnEndOfUtterance) {
    auto session = VoiceStreamingSession::create(makeConfig());
    session->start();

    bool final_called = false;
    session->onFinalTranscript([&final_called](const FinalTranscript& ft){
        final_called = true;
        EXPECT_FALSE(ft.stream_id.empty());
    });

    std::vector<uint8_t> chunk(512, 0x20);
    session->sendAudioChunk(chunk);
    session->endOfUtterance();
    EXPECT_TRUE(final_called);
}

TEST(VoiceStreamingSession, TtsChunkCallbackWhenEnabled) {
    auto cfg = makeConfig();
    cfg.enable_tts = true;
    auto session = VoiceStreamingSession::create(cfg);
    session->start();

    bool tts_called = false;
    session->onTtsChunk([&tts_called](const std::vector<uint8_t>&){
        tts_called = true;
    });

    std::vector<uint8_t> chunk(256, 0x30);
    session->sendAudioChunk(chunk);
    session->endOfUtterance();
    EXPECT_TRUE(tts_called);
}

// ─────────────────────────────────────────────────────────────────────────────
// VoiceStreamingManager
// ─────────────────────────────────────────────────────────────────────────────

TEST(VoiceStreamingManager, CreateAndRouteSession) {
    VoiceStreamingManager mgr(10);
    auto id = mgr.createSession(makeConfig());
    EXPECT_FALSE(id.empty());
    EXPECT_EQ(mgr.activeSessionCount(), 1u);

    std::vector<uint8_t> chunk(128, 0x55);
    auto pt = mgr.routeAudio(id, chunk);
    // Partial transcript should be returned
    EXPECT_EQ(pt.stream_id, id);

    mgr.closeSession(id);
    EXPECT_EQ(mgr.activeSessionCount(), 0u);
}

TEST(VoiceStreamingManager, MaxConcurrentSessionsEnforced) {
    VoiceStreamingManager mgr(2);
    auto id1 = mgr.createSession(makeConfig());
    auto id2 = mgr.createSession(makeConfig());
    auto id3 = mgr.createSession(makeConfig());  // Should be rejected

    EXPECT_FALSE(id1.empty());
    EXPECT_FALSE(id2.empty());
    EXPECT_TRUE(id3.empty());  // rejected
    EXPECT_EQ(mgr.activeSessionCount(), 2u);
}

TEST(VoiceStreamingManager, RoutingUnknownIdReturnsEmptyTranscript) {
    VoiceStreamingManager mgr(10);
    std::vector<uint8_t> chunk(64, 0x00);
    auto pt = mgr.routeAudio("nonexistent-id", chunk);
    EXPECT_TRUE(pt.stream_id.empty());
}

} // anonymous namespace
} // namespace voice
} // namespace themis
