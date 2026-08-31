/**
 * @file test_voice_batch_a8_focused.cpp
 * @brief Focused smoke tests for the current voice streaming session API.
 */

#include <gtest/gtest.h>

#include "voice/voice_browser_streaming.h"

#include <memory>
#include <vector>

using namespace themis::voice;

TEST(VoiceBatchA8Focused, SessionCreateStartAndEndSmoke) {
    VoiceStreamingSession::Config config;
    config.session_id = "session-1";
    config.user_id = "user-1";
    config.audio_format = StreamAudioFormat{};
    config.partial_results = true;

    auto session = VoiceStreamingSession::create(config);
    ASSERT_TRUE(session);
    EXPECT_FALSE(session->isActive());

    const auto stream_id = session->start();
    EXPECT_FALSE(stream_id.empty());
    EXPECT_TRUE(session->isActive());

    const std::vector<uint8_t> audio_chunk = {0, 0, 0, 0};
    EXPECT_NO_THROW({
        auto transcript = session->sendAudioChunk(audio_chunk);
        (void)transcript;
    });

    session->end();
    EXPECT_FALSE(session->isActive());
}

TEST(VoiceBatchA8Focused, MultipleSessionsCanBeCreated) {
    VoiceStreamingSession::Config config;
    config.session_id = "session-2";
    config.audio_format = StreamAudioFormat{};

    auto first = VoiceStreamingSession::create(config);
    auto second = VoiceStreamingSession::create(config);

    ASSERT_TRUE(first);
    ASSERT_TRUE(second);

    const auto first_id = first->start();
    const auto second_id = second->start();
    EXPECT_FALSE(first_id.empty());
    EXPECT_FALSE(second_id.empty());

    first->end();
    second->end();
}
