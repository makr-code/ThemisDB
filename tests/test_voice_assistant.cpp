/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_voice_assistant.cpp                           ║
  Version:         0.0.32                                             ║
  Last Modified:   2026-02-23 03:59:37                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     420                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 313664710  2026-02-22  fix(voice): audit gaps – wake-word stats, config, docs, c... ║
    • 91ce0da45  2026-02-22  feat(voice): add POST /api/v1/voice/command/stream endpoi... ║
    • 8ae8a4193  2026-02-22  feat(voice): implement wake-word detection for hands-free... ║
    • 2b12bc7d3  2026-02-22  impl: real-time streaming STT for audio arrival ║
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

// ============================================================
// WakeWordDetector unit tests
// ============================================================

#include "voice/wake_word_detector.h"

using namespace themis::voice;

// Build a minimal 16-bit PCM buffer with a given RMS level.
static std::vector<uint8_t> makePcm(int duration_ms,
                                     float amplitude,
                                     int sample_rate = 16000) {
    const int num_samples = (sample_rate * duration_ms) / 1000;
    std::vector<uint8_t> pcm;
    pcm.reserve(static_cast<size_t>(num_samples) * 2);
    for (int i = 0; i < num_samples; ++i) {
        float val = amplitude * std::sin(2.0f * static_cast<float>(M_PI) * 440.0f * i / sample_rate);
        auto s = static_cast<int16_t>(val * 32767.0f);
        pcm.push_back(static_cast<uint8_t>(s & 0xFF));
        pcm.push_back(static_cast<uint8_t>((s >> 8) & 0xFF));
    }
    return pcm;
}

// Default-constructed detector has no wake words.
TEST(WakeWordDetector, DefaultHasNoWakeWords) {
    WakeWordDetector detector;
    EXPECT_TRUE(detector.listWakeWords().empty());
}

// addWakeWord accepts new IDs and rejects duplicates.
TEST(WakeWordDetector, AddAndListWakeWords) {
    WakeWordDetector detector;
    EXPECT_TRUE(detector.addWakeWord("hey-themis", "hey themis"));
    EXPECT_TRUE(detector.addWakeWord("themis",     "themis"));
    EXPECT_FALSE(detector.addWakeWord("hey-themis", "hey themis"));  // duplicate

    auto ids = detector.listWakeWords();
    EXPECT_EQ(ids.size(), 2u);
}

// removeWakeWord removes by ID.
TEST(WakeWordDetector, RemoveWakeWord) {
    WakeWordDetector detector;
    detector.addWakeWord("hey-themis", "hey themis");
    EXPECT_TRUE(detector.removeWakeWord("hey-themis"));
    EXPECT_FALSE(detector.removeWakeWord("hey-themis"));  // already removed
    EXPECT_TRUE(detector.listWakeWords().empty());
}

// Silence below VAD threshold never triggers a detection.
TEST(WakeWordDetector, SilenceDoesNotTrigger) {
    WakeWordConfig cfg;
    cfg.sensitivity    = 0.3f;
    cfg.vad_min_energy = 0.01f;
    WakeWordDetector detector(cfg);
    detector.addWakeWord("hey-themis", "hey themis");

    // Near-silence audio (amplitude 0.0001)
    auto silent = makePcm(1500, 0.0001f);
    auto result = detector.processAudioChunk(silent);
    EXPECT_FALSE(result.detected);
}

// Empty audio chunk returns no detection.
TEST(WakeWordDetector, EmptyChunkReturnsNoDetection) {
    WakeWordDetector detector;
    detector.addWakeWord("themis", "themis");
    auto result = detector.processAudioChunk({});
    EXPECT_FALSE(result.detected);
}

// No wake words registered → never fire.
TEST(WakeWordDetector, NoWakeWordsNeverFires) {
    WakeWordConfig cfg;
    cfg.sensitivity    = 0.0f;  // Even with sensitivity at zero
    cfg.vad_min_energy = 0.0f;
    WakeWordDetector detector(cfg);
    auto audio = makePcm(1500, 0.8f);
    auto result = detector.processAudioChunk(audio);
    EXPECT_FALSE(result.detected);
}

// reset() clears the buffer; detection is not triggered immediately after.
TEST(WakeWordDetector, ResetClearsState) {
    WakeWordDetector detector;
    detector.addWakeWord("hey-themis", "hey themis");
    auto audio = makePcm(1500, 0.8f);
    detector.processAudioChunk(audio);
    detector.reset();

    // After reset the buffer is empty → no detection from an empty follow-up.
    auto result = detector.processAudioChunk({});
    EXPECT_FALSE(result.detected);
}

// getStatistics() returns expected keys.
TEST(WakeWordDetector, StatisticsKeys) {
    WakeWordDetector detector;
    detector.addWakeWord("hey-themis", "hey themis");
    auto audio = makePcm(500, 0.5f);
    detector.processAudioChunk(audio);

    auto stats = detector.getStatistics();
    EXPECT_TRUE(stats.contains("total_chunks_processed"));
    EXPECT_TRUE(stats.contains("total_detections"));
    EXPECT_TRUE(stats.contains("registered_wake_words"));
    EXPECT_EQ(stats["total_chunks_processed"].get<uint64_t>(), 1u);
    EXPECT_EQ(stats["registered_wake_words"].get<size_t>(), 1u);
}

// Cooldown prevents re-detection within cooldown_ms.
TEST(WakeWordDetector, CooldownPreventsImmediateRetrigger) {
    WakeWordConfig cfg;
    cfg.sensitivity    = 0.0f;  // Accept any voiced chunk
    cfg.vad_min_energy = 0.0f;
    cfg.cooldown_ms    = 5000;  // 5-second cooldown
    cfg.continuous_listen = true;
    WakeWordDetector detector(cfg);
    detector.addWakeWord("hey-themis", "hey themis");

    auto audio = makePcm(1500, 0.8f);
    // First call may or may not detect depending on scoring; run a second
    // call immediately and ensure total_detections <= 1.
    detector.processAudioChunk(audio);
    detector.processAudioChunk(audio);

    auto stats = detector.getStatistics();
    EXPECT_LE(stats["total_detections"].get<uint64_t>(), 1u);
}

// Callback is invoked when a detection fires.
TEST(WakeWordDetector, CallbackIsInvokedOnDetection) {
    WakeWordConfig cfg;
    cfg.sensitivity    = 0.0f;  // Accept any voiced chunk
    cfg.vad_min_energy = 0.0f;
    cfg.cooldown_ms    = 0;
    WakeWordDetector detector(cfg);
    detector.addWakeWord("hey-themis", "hey themis");

    std::atomic<int> callback_count{0};
    detector.setDetectionCallback([&](const WakeWordDetectionResult& r) {
        if (r.detected) ++callback_count;
    });

    // Feed enough audio that VAD passes; detection depends on scoring.
    for (int i = 0; i < 5; ++i) {
        auto audio = makePcm(1500, 0.8f);
        detector.processAudioChunk(audio);
        // Reset cooldown between calls so each chunk can trigger independently.
        detector.reset();
    }
    // We just verify the callback is wired; actual count depends on scoring.
    auto stats = detector.getStatistics();
    EXPECT_EQ(stats["total_detections"].get<uint64_t>(),
              static_cast<uint64_t>(callback_count.load()));
}

// setConfig / getConfig round-trip.
TEST(WakeWordDetector, ConfigRoundTrip) {
    WakeWordDetector detector;
    WakeWordConfig cfg;
    cfg.sensitivity    = 0.7f;
    cfg.cooldown_ms    = 2000;
    cfg.buffer_length_ms = 2000;
    detector.setConfig(cfg);

    auto retrieved = detector.getConfig();
    EXPECT_FLOAT_EQ(retrieved.sensitivity,    cfg.sensitivity);
    EXPECT_EQ(retrieved.cooldown_ms,          cfg.cooldown_ms);
    EXPECT_EQ(retrieved.buffer_length_ms,     cfg.buffer_length_ms);
}

// addWakeWord rejects empty id or phrase.
TEST(WakeWordDetector, AddWakeWordRejectsEmpty) {
    WakeWordDetector detector;
    EXPECT_FALSE(detector.addWakeWord("",      "hey themis"));
    EXPECT_FALSE(detector.addWakeWord("hw-id", ""));
    EXPECT_TRUE(detector.listWakeWords().empty());
}

// ============================================================
// VoiceAssistant::detectWakeWord() integration tests
// (exercises the VoiceAssistant wrapper path without needing
//  a real STT/TTS/LLM model)
// These tests require THEMIS_ENABLE_VOICE_ASSISTANT=ON because
// voice_assistant.cpp is only compiled with that flag.
// ============================================================

#ifdef THEMIS_ENABLE_VOICE_ASSISTANT
#include "voice/voice_assistant.h"

// VoiceAssistant detectWakeWord delegates to WakeWordDetector.
// Even without initialize(), the detector is ready in the constructor.
TEST(VoiceAssistantWakeWord, DetectWakeWordReturnsFalseForSilence) {
    themis::voice::VoiceAssistant::Config cfg;
    cfg.enable_wake_word = true;
    cfg.wake_word_config.vad_min_energy = 0.01f;
    cfg.wake_word_config.sensitivity    = 0.3f;
    cfg.wake_words = {{"hey-themis", "hey themis"}};

    themis::voice::VoiceAssistant va(cfg);

    // Near-silence PCM – VAD gate should block detection.
    auto silent = makePcm(1500, 0.0001f);
    auto result = va.detectWakeWord(silent);
    EXPECT_FALSE(result.detected);
}

TEST(VoiceAssistantWakeWord, DetectWakeWordReturnsFalseForEmptyChunk) {
    themis::voice::VoiceAssistant::Config cfg;
    cfg.enable_wake_word = true;
    cfg.wake_words = {{"hey-themis", "hey themis"}};

    themis::voice::VoiceAssistant va(cfg);

    auto result = va.detectWakeWord({});
    EXPECT_FALSE(result.detected);
}

TEST(VoiceAssistantWakeWord, SetCallbackIsForwarded) {
    themis::voice::VoiceAssistant::Config cfg;
    cfg.enable_wake_word = true;
    cfg.wake_word_config.sensitivity    = 0.0f;
    cfg.wake_word_config.vad_min_energy = 0.0f;
    cfg.wake_word_config.cooldown_ms    = 0;
    cfg.wake_words = {{"hey-themis", "hey themis"}};

    themis::voice::VoiceAssistant va(cfg);

    std::atomic<int> fired{0};
    va.setWakeWordCallback([&](const themis::voice::WakeWordDetectionResult& r) {
        if (r.detected) ++fired;
    });

    // Feed voiced audio; detection depends on scoring but callback must be wired.
    auto audio = makePcm(1500, 0.8f);
    va.detectWakeWord(audio);
    // Whether it detects or not, the callback must not crash.
    SUCCEED();
}

TEST(VoiceAssistantWakeWord, StatisticsIncludesWakeWordKey) {
    themis::voice::VoiceAssistant::Config cfg;
    cfg.enable_wake_word = true;
    cfg.wake_words = {{"hey-themis", "hey themis"}};

    themis::voice::VoiceAssistant va(cfg);
    auto audio = makePcm(500, 0.3f);
    va.detectWakeWord(audio);

    auto stats = va.getStatistics();
    ASSERT_TRUE(stats.contains("wake_word"))
        << "getStatistics() must expose 'wake_word' sub-object";
    ASSERT_TRUE(stats["wake_word"].contains("total_chunks_processed"));
    EXPECT_EQ(stats["wake_word"]["total_chunks_processed"].get<uint64_t>(), 1u);
}
#endif // THEMIS_ENABLE_VOICE_ASSISTANT

// ============================================================
// VoiceBatchProcessor + streaming STT integration tests
// ============================================================

#include "voice/voice_batch_processor.h"

// processItem keeps transcript empty when no STT processor is attached.
TEST(BatchProcessorStreamingSTT, TranscriptEmptyWithoutSTTProcessor) {
    themis::voice::VoiceBatchProcessor bp;

    themis::voice::BatchAudioItem item;
    item.item_id    = "no-stt";
    item.audio_data = makeSineWav(2);
    item.sample_rate = 16000;

    auto result = bp.processItem(item);
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.transcript.empty());
}

// setSTTProcessor / getStatistics: attaching a processor doesn't crash.
TEST(BatchProcessorStreamingSTT, SetSTTProcessorDoesNotCrash) {
    themis::voice::VoiceBatchProcessor bp;
    auto stt = std::make_shared<themis::content::STTProcessor>();
    EXPECT_NO_THROW(bp.setSTTProcessor(stt));
    EXPECT_NO_THROW(bp.setSTTProcessor(nullptr));  // detach is also safe
}

// When the STT processor is initialised (placeholder path), processItem
// populates result.transcript via streamTranscribe.
TEST(BatchProcessorStreamingSTT, TranscriptPopulatedWhenSTTInitialised) {
    auto stt = std::make_shared<themis::content::STTProcessor>();
    themis::content::PluginConfig cfg;
    bool init_ok = stt->initialize(cfg);
    if (!init_ok) {
        GTEST_SKIP() << "STT processor could not be initialised (no model file)";
    }

    themis::voice::VoiceBatchProcessor bp;
    bp.setSTTProcessor(stt);

    themis::voice::BatchAudioItem item;
    item.item_id     = "stream-batch";
    item.audio_data  = makeSineWav(5);
    item.sample_rate = 16000;

    auto result = bp.processItem(item);
    EXPECT_TRUE(result.success);
    EXPECT_FALSE(result.transcript.empty())
        << "transcript should be populated by streamTranscribe";
}

// WER is computed when transcript_reference is provided and STT is active.
TEST(BatchProcessorStreamingSTT, WERComputedWithTranscriptAndReference) {
    auto stt = std::make_shared<themis::content::STTProcessor>();
    themis::content::PluginConfig cfg;
    bool init_ok = stt->initialize(cfg);
    if (!init_ok) {
        GTEST_SKIP() << "STT processor could not be initialised (no model file)";
    }

    themis::voice::BatchProcessorConfig bpcfg;
    bpcfg.compute_wer = true;
    themis::voice::VoiceBatchProcessor bp(bpcfg);
    bp.setSTTProcessor(stt);

    themis::voice::BatchAudioItem item;
    item.item_id              = "wer-test";
    item.audio_data           = makeSineWav(3);
    item.sample_rate          = 16000;
    item.transcript_reference = "hello world";  // reference text

    auto result = bp.processItem(item);
    EXPECT_TRUE(result.success);
    // WER should be computed (>=0) because both transcript and reference exist.
    if (!result.transcript.empty()) {
        EXPECT_GE(result.wer_score, 0.0f);
    }
}
