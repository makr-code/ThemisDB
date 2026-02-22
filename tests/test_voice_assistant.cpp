/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_voice_assistant.cpp                           ║
  Version:         0.0.28                                             ║
  Last Modified:   2026-02-22 18:42:56                                ║
  Author:          copilot-swe-agent[bot]                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <gtest/gtest.h>

// Full VoiceAssistant integration tests require THEMIS_ENABLE_VOICE_ASSISTANT=ON.
// Standalone component tests are in test_voice_production.cpp.
TEST(VoiceAssistantModule, StubPlaceholder) {
    SUCCEED();
}

// ============================================================
// Streaming STT – STTProcessor::streamTranscribe unit tests
// (no Whisper model required; the processor is initialised
//  without a model file and the placeholder path is exercised)
// ============================================================

#include "content/stt_processor.h"
#include <vector>
#include <atomic>
#include <cstdint>
#include <cmath>

namespace {

// Build a minimal valid 16-bit PCM WAV buffer at 16 kHz, mono.
std::vector<uint8_t> makeSineWav(int duration_seconds, int sample_rate = 16000) {
    const int num_samples = duration_seconds * sample_rate;
    const int data_size   = num_samples * 2;  // 16-bit = 2 bytes/sample

    std::vector<uint8_t> wav;
    wav.reserve(44 + data_size);

    auto pushU32 = [&](uint32_t v) {
        wav.push_back(static_cast<uint8_t>(v & 0xFF));
        wav.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
        wav.push_back(static_cast<uint8_t>((v >> 16) & 0xFF));
        wav.push_back(static_cast<uint8_t>((v >> 24) & 0xFF));
    };
    auto pushU16 = [&](uint16_t v) {
        wav.push_back(static_cast<uint8_t>(v & 0xFF));
        wav.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
    };

    // RIFF header
    wav.push_back('R'); wav.push_back('I'); wav.push_back('F'); wav.push_back('F');
    pushU32(static_cast<uint32_t>(36 + data_size));
    wav.push_back('W'); wav.push_back('A'); wav.push_back('V'); wav.push_back('E');

    // fmt chunk
    wav.push_back('f'); wav.push_back('m'); wav.push_back('t'); wav.push_back(' ');
    pushU32(16);                          // chunk size
    pushU16(1);                           // PCM
    pushU16(1);                           // mono
    pushU32(static_cast<uint32_t>(sample_rate));
    pushU32(static_cast<uint32_t>(sample_rate * 2));  // byte rate
    pushU16(2);                           // block align
    pushU16(16);                          // bits per sample

    // data chunk
    wav.push_back('d'); wav.push_back('a'); wav.push_back('t'); wav.push_back('a');
    pushU32(static_cast<uint32_t>(data_size));

    for (int i = 0; i < num_samples; ++i) {
        float val = std::sin(2.0f * static_cast<float>(M_PI) * 440.0f * i / sample_rate);
        auto sample = static_cast<int16_t>(val * 16000.0f);
        wav.push_back(static_cast<uint8_t>(sample & 0xFF));
        wav.push_back(static_cast<uint8_t>((sample >> 8) & 0xFF));
    }

    return wav;
}

}  // namespace

// streamTranscribe returns false when processor is not initialised.
TEST(STTStreamTranscribe, ReturnsFalseWhenNotInitialised) {
    themis::content::STTProcessor stt;
    bool called = false;
    bool result = stt.streamTranscribe({0x00}, [&](const themis::content::TranscriptionSegment&) {
        called = true;
    });
    EXPECT_FALSE(result);
    EXPECT_FALSE(called);
}

// streamTranscribe returns false for an empty audio buffer even when initialised.
TEST(STTStreamTranscribe, ReturnsFalseForEmptyAudio) {
    themis::content::STTProcessor stt;
    // Attempt initialisation (will fail without model file; that is expected).
    themis::content::PluginConfig cfg;
    stt.initialize(cfg);  // may return false – that is fine for this test

    bool result = stt.streamTranscribe({}, [](const themis::content::TranscriptionSegment&) {});
    EXPECT_FALSE(result);
}

// streamTranscribe returns false when callback is nullptr.
TEST(STTStreamTranscribe, ReturnsFalseForNullCallback) {
    themis::content::STTProcessor stt;
    auto wav = makeSineWav(1);
    bool result = stt.streamTranscribe(wav, nullptr);
    EXPECT_FALSE(result);
}

// When Whisper is disabled the placeholder transcription path is taken.
// Verify that streamTranscribe produces at least one segment callback
// and returns true when the processor is initialised successfully.
TEST(STTStreamTranscribe, PlaceholderPathEmitsSegments) {
    themis::content::STTProcessor stt;
    themis::content::PluginConfig cfg;
    bool init_ok = stt.initialize(cfg);

    // If initialisation succeeded (placeholder path), streaming should work.
    if (!init_ok) {
        GTEST_SKIP() << "STT processor could not be initialised (no model file)";
    }

    auto wav = makeSineWav(5);
    std::atomic<int> callback_count{0};

    bool result = stt.streamTranscribe(wav, [&](const themis::content::TranscriptionSegment& seg) {
        callback_count++;
        // Timestamps must be non-negative and end >= start.
        EXPECT_GE(seg.start_ms, 0);
        EXPECT_GE(seg.end_ms, seg.start_ms);
    });

    EXPECT_TRUE(result);
    EXPECT_GT(callback_count.load(), 0);
}

// Verify that segment timestamps produced by streamTranscribe are monotonically
// increasing across successive windows (watermark logic).
TEST(STTStreamTranscribe, SegmentTimestampsAreMonotonic) {
    themis::content::STTProcessor stt;
    themis::content::PluginConfig cfg;
    bool init_ok = stt.initialize(cfg);

    if (!init_ok) {
        GTEST_SKIP() << "STT processor could not be initialised (no model file)";
    }

    auto wav = makeSineWav(9);  // 9 seconds → multiple 3-second windows
    int64_t prev_start = -1;

    stt.streamTranscribe(wav, [&](const themis::content::TranscriptionSegment& seg) {
        EXPECT_GE(seg.start_ms, prev_start) << "segment timestamps must not go backwards";
        prev_start = seg.start_ms;
    });
}
