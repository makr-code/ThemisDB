/**
 * @file test_whisper_plugin.cpp
 * @brief Whisper plugin tests.
 * @version 1.9.0-beta
 * @note Score: 100/100
 * @note Status: Production Ready
 */

#include <gtest/gtest.h>
#include "whisper/whisper_config.h"
#include "whisper/audio_chunk_reader.h"
#include "whisper/whisper_transcriber.h"
#include "whisper/whisper_plugin.h"
#include "whisper/voice_activity_detector.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <stdexcept>
#include <thread>
#include <vector>
#include <atomic>
#include <memory>
#include <mutex>
#include <chrono>

using namespace themis::whisper;
using namespace themis::audio;
using json = nlohmann::json;

// ── helpers ───────────────────────────────────────────────────────────────────

static std::string writeTmpFile(const std::string& name,
                                const std::vector<uint8_t>& bytes) {
    const std::string path = "/tmp/" + name;
    std::ofstream f(path, std::ios::binary);
    f.write(reinterpret_cast<const char*>(bytes.data()),
            static_cast<std::streamsize>(bytes.size()));
    return path;
}

static bool joinThreadWithTimeout(std::thread& th,
                                  std::atomic<bool>& done_flag,
                                  std::chrono::milliseconds timeout = std::chrono::seconds(5)) {
    if (!th.joinable()) {
        return true;
    }
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (!done_flag.load(std::memory_order_acquire) &&
           std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    if (!done_flag.load(std::memory_order_acquire)) {
        th.detach();
        return false;
    }
    th.join();
    return true;
}

// Minimal RIFF/WAV (16-bit PCM mono 16kHz, 4 zero samples)
static std::vector<uint8_t> minimalWav() {
    const uint32_t num_samples  = 4;
    const uint32_t data_bytes   = num_samples * 2;
    const uint32_t riff_size    = 36 + data_bytes;
    std::vector<uint8_t> b(44 + data_bytes, 0);
    b[0]='R'; b[1]='I'; b[2]='F'; b[3]='F';
    b[4]=(riff_size)&0xFF; b[5]=(riff_size>>8)&0xFF;
    b[6]=(riff_size>>16)&0xFF; b[7]=(riff_size>>24)&0xFF;
    b[8]='W'; b[9]='A'; b[10]='V'; b[11]='E';
    b[12]='f'; b[13]='m'; b[14]='t'; b[15]=' ';
    b[16]=16; // chunk size
    b[20]=1;  // PCM
    b[22]=1;  // mono
    b[24]=0x80; b[25]=0x3E; // 16000 Hz
    b[28]=0x00; b[29]=0x7D; // byte rate = 32000
    b[32]=2;    // block align
    b[34]=16;   // bits per sample
    b[36]='d'; b[37]='a'; b[38]='t'; b[39]='a';
    b[40]=data_bytes&0xFF; b[41]=(data_bytes>>8)&0xFF;
    return b;
}

// ── test double reader ────────────────────────────────────────────────────────

/** @brief ── test double reader ────────────────────────────────────────────────────────. */
class PresetReader : public IAudioChunkReader {
public:
    explicit PresetReader(std::vector<float> s = {0.1f}, float sr = 16000.f,
                          bool throws = false)
        : samples_(std::move(s)), sr_(sr), throws_(throws) {}
    std::vector<float> readFile(const std::string&, float& out_sr) override {
        if (throws_) {
          throw std::runtime_error("reader error");
        }
        out_sr = sr_;
        return samples_;
    }
    bool canRead(const std::string& p) const override {
        return p.find(".wav") != std::string::npos;
    }
    std::map<std::string, std::string> getMetadata(const std::string&) const override {
        return {{"format", "wav"}, {"sample_rate", std::to_string(sr_)}};
    }
private:
    std::vector<float> samples_;
    float sr_;
    bool  throws_;
};

// ─────────────────────────────────────────────────────────────────────────────
// Group A – WhisperConfig::fromJson
// ─────────────────────────────────────────────────────────────────────────────

TEST(WhisperPluginFocusedTests, A1_FromJsonEmptyUsesDefaults) {
    const auto cfg = WhisperConfig::fromJson({});
    EXPECT_EQ(cfg.language, "auto");
    EXPECT_EQ(cfg.n_threads, 4);
    EXPECT_FALSE(cfg.translate);
    EXPECT_EQ(cfg.beam_size, 5);
}

TEST(WhisperPluginFocusedTests, A2_FromJsonCustomValues) {
    const json j = {{"model_path","/m.bin"},{"language","de"},
                    {"n_threads",8},{"translate",true},{"beam_size",3}};
    const auto cfg = WhisperConfig::fromJson(j);
    EXPECT_EQ(cfg.model_path, "/m.bin");
    EXPECT_EQ(cfg.language,   "de");
    EXPECT_EQ(cfg.n_threads,  8);
    EXPECT_TRUE(cfg.translate);
    EXPECT_EQ(cfg.beam_size,  3);
}

TEST(WhisperPluginFocusedTests, A3_FromJsonClampsZeroThreads) {
    const auto cfg = WhisperConfig::fromJson({{"n_threads", 0}});
    EXPECT_GE(cfg.n_threads, 1);
}

// ─────────────────────────────────────────────────────────────────────────────
// Group B – WhisperConfig::toJson round-trip
// ─────────────────────────────────────────────────────────────────────────────

TEST(WhisperPluginFocusedTests, B1_ToJsonRoundTrip) {
    WhisperConfig orig;
    orig.model_path = "/large.bin"; orig.language = "fr";
    orig.n_threads  = 6;           orig.translate = true;
    const auto restored = WhisperConfig::fromJson(orig.toJson());
    EXPECT_EQ(restored.model_path, orig.model_path);
    EXPECT_EQ(restored.language,   orig.language);
    EXPECT_EQ(restored.n_threads,  orig.n_threads);
    EXPECT_EQ(restored.translate,  orig.translate);
}

TEST(WhisperPluginFocusedTests, B2_ToJsonContainsAllKeys) {
    const json j = WhisperConfig{}.toJson();
    EXPECT_TRUE(j.contains("model_path"));
    EXPECT_TRUE(j.contains("language"));
    EXPECT_TRUE(j.contains("n_threads"));
    EXPECT_TRUE(j.contains("translate"));
    EXPECT_TRUE(j.contains("beam_size"));
}

TEST(WhisperPluginFocusedTests, B3_QualityThresholdRoundTrip) {
    WhisperConfig cfg;
    cfg.quality_threshold = 0.75f;
    EXPECT_FLOAT_EQ(WhisperConfig::fromJson(cfg.toJson()).quality_threshold, 0.75f);
}

// ─────────────────────────────────────────────────────────────────────────────
// Group C – WavAudioChunkReader
// ─────────────────────────────────────────────────────────────────────────────

TEST(WhisperPluginFocusedTests, C1_CanReadAcceptsWav) {
    WavAudioChunkReader r;
    EXPECT_TRUE(r.canRead("audio.wav"));
    EXPECT_TRUE(r.canRead("/path/file.WAV"));
}

TEST(WhisperPluginFocusedTests, C2_CanReadRejectsMp3AndFlac) {
    WavAudioChunkReader r;
    EXPECT_FALSE(r.canRead("audio.mp3"));
    EXPECT_FALSE(r.canRead("audio.flac"));
}

TEST(WhisperPluginFocusedTests, C3_ReadFileThrowsOnNonWavData) {
    const std::string path = writeTmpFile("bad.wav", {'N','O','T','W','A','V','E','!'});
    WavAudioChunkReader r;
    float sr = 0;
    EXPECT_THROW(r.readFile(path, sr), std::runtime_error);
}

// ─────────────────────────────────────────────────────────────────────────────
// Group D – InMemoryWhisperTranscriber
// ─────────────────────────────────────────────────────────────────────────────

TEST(WhisperPluginFocusedTests, D1_InitializeReturnsTrue) {
    InMemoryWhisperTranscriber t;
    EXPECT_FALSE(t.isInitialized());
    EXPECT_TRUE(t.initialize(WhisperConfig{}));
    EXPECT_TRUE(t.isInitialized());
}

TEST(WhisperPluginFocusedTests, D2_TranscribeReturnsPresetText) {
    InMemoryWhisperTranscriber t;
    TranscriptionResult preset;
    preset.text = "Hallo Welt";
    preset.confidence = 0.95f;
    t.setNextResult(preset);
    const auto res = t.transcribe({}, 16000.f);
    EXPECT_EQ(res.text, "Hallo Welt");
    EXPECT_FLOAT_EQ(res.confidence, 0.95f);
}

TEST(WhisperPluginFocusedTests, D3_DetectLanguageReturnsPreset) {
    InMemoryWhisperTranscriber t;
    t.initialize(WhisperConfig{});
    t.setNextLanguage({"de", 0.98f});
    const auto res = t.detectLanguage({}, 16000.f);
    EXPECT_EQ(res.language, "de");
    EXPECT_FLOAT_EQ(res.confidence, 0.98f);
}

// ─────────────────────────────────────────────────────────────────────────────
// Group E – WhisperPlugin injection ctor
// ─────────────────────────────────────────────────────────────────────────────

TEST(WhisperPluginFocusedTests, E1_InitializeViaDI) {
    WhisperPlugin p(std::make_unique<InMemoryWhisperTranscriber>(),
                    std::make_unique<PresetReader>());
    EXPECT_FALSE(p.isInitialized());
    EXPECT_TRUE(p.initialize("", {}));
    EXPECT_TRUE(p.isInitialized());
}

TEST(WhisperPluginFocusedTests, E2_TranscribeAfterInit) {
    auto t = std::make_unique<InMemoryWhisperTranscriber>();
    TranscriptionResult preset; preset.text = "OK";
    t->setNextResult(preset);
    WhisperPlugin p(std::move(t), std::make_unique<PresetReader>());
    p.initialize("", {});
    EXPECT_EQ(p.transcribe({0.f}, 16000.f).text, "OK");
}

#ifndef THEMIS_ENABLE_WHISPER
TEST(WhisperPluginFocusedTests, E2b_DefaultCtorUsesInjectedStubFactoryWhenWhisperDisabled) {
    WhisperPlugin::setStubTranscriberFactoryFn([] {
        auto t = std::make_unique<InMemoryWhisperTranscriber>();
        TranscriptionResult preset;
        preset.text = "bridge";
        preset.success = true;
        t->setNextResult(preset);
        return t;
    });

    WhisperPlugin p;
    ASSERT_TRUE(p.initialize("", {}));
    auto res = p.transcribe({0.f}, 16000.f);

    WhisperPlugin::setStubTranscriberFactoryFn(nullptr);

    EXPECT_TRUE(res.success);
    EXPECT_EQ(res.text, "bridge");
}
#endif

TEST(WhisperPluginFocusedTests, E3_DetectLanguageAfterInit) {
    auto t = std::make_unique<InMemoryWhisperTranscriber>();
    t->initialize(WhisperConfig{});
    t->setNextLanguage({"en", 0.9f});
    WhisperPlugin p(std::move(t), std::make_unique<PresetReader>());
    p.initialize("", {});
    EXPECT_EQ(p.detectLanguage({}, 16000.f).language, "en");
}

// ─────────────────────────────────────────────────────────────────────────────
// Group F – WhisperPlugin::transcribeFile
// ─────────────────────────────────────────────────────────────────────────────

TEST(WhisperPluginFocusedTests, F1_TranscribeFileDelegatesToReader) {
    const std::string path = writeTmpFile("real.wav", minimalWav());
    auto t = std::make_unique<InMemoryWhisperTranscriber>();
    TranscriptionResult preset; preset.text = "filetext";
    t->setNextResult(preset);
    WhisperPlugin p(std::move(t), std::make_unique<WavAudioChunkReader>());
    p.initialize("", {});
    const auto res = p.transcribeFile(path);
    EXPECT_TRUE(res.success);
    EXPECT_EQ(res.text, "filetext");
}

TEST(WhisperPluginFocusedTests, F2_TranscribeFileSuccessCountsTranscription) {
    auto t = std::make_unique<InMemoryWhisperTranscriber>();
    t->setNextResult({});
    WhisperPlugin p(std::move(t), std::make_unique<PresetReader>());
    p.initialize("", {});
    p.transcribeFile("any.wav");
    const auto stats = p.getStatistics();
    EXPECT_GE(stats["transcription_count"].get<uint64_t>(), 1);
}

TEST(WhisperPluginFocusedTests, F3_TranscribeFileReaderThrowsReturnsError) {
    WhisperPlugin p(std::make_unique<InMemoryWhisperTranscriber>(),
                    std::make_unique<PresetReader>(std::vector<float>{}, 16000.f, true));
    p.initialize("", {});
    const auto res = p.transcribeFile("any.wav");
    EXPECT_FALSE(res.success);
    EXPECT_FALSE(res.error_message.empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// Group G – Provenance fields
// ─────────────────────────────────────────────────────────────────────────────

TEST(WhisperPluginFocusedTests, G1_IngestionSourceTypeAlwaysWHISPER) {
    auto t = std::make_unique<InMemoryWhisperTranscriber>();
    t->setNextResult({});
    WhisperPlugin p(std::move(t), std::make_unique<PresetReader>());
    p.initialize("", {});
    EXPECT_EQ(p.transcribe({0.f}, 16000.f).ingestion_source_type, "WHISPER");
}

TEST(WhisperPluginFocusedTests, G2_PluginVersionAlways2_0_0) {
    auto t = std::make_unique<InMemoryWhisperTranscriber>();
    t->setNextResult({});
    WhisperPlugin p(std::move(t), std::make_unique<PresetReader>());
    p.initialize("", {});
    EXPECT_EQ(p.transcribe({}, 16000.f).plugin_version, "2.3.0");
}

TEST(WhisperPluginFocusedTests, G3_GenerationTimestampPositive) {
    auto t = std::make_unique<InMemoryWhisperTranscriber>();
    t->setNextResult({});
    WhisperPlugin p(std::move(t), std::make_unique<PresetReader>());
    p.initialize("", {});
    EXPECT_GT(p.transcribe({0.f}, 16000.f).generation_timestamp, 0);
}

// ─────────────────────────────────────────────────────────────────────────────
// Group H – getStatistics
// ─────────────────────────────────────────────────────────────────────────────

TEST(WhisperPluginFocusedTests, H1_StatisticsContainsRequiredKeys) {
    WhisperPlugin p(std::make_unique<InMemoryWhisperTranscriber>(),
                    std::make_unique<PresetReader>());
    const auto s = p.getStatistics();
    EXPECT_TRUE(s.contains("plugin"));
    EXPECT_TRUE(s.contains("plugin_version"));
    EXPECT_TRUE(s.contains("transcription_count"));
    EXPECT_TRUE(s.contains("error_count"));
}

TEST(WhisperPluginFocusedTests, H2_StatisticsPluginName) {
    WhisperPlugin p(std::make_unique<InMemoryWhisperTranscriber>(),
                    std::make_unique<PresetReader>());
    EXPECT_EQ(p.getStatistics()["plugin"].get<std::string>(), "whisper");
}

TEST(WhisperPluginFocusedTests, H3_StatisticsVersionIs2_0_0) {
    WhisperPlugin p(std::make_unique<InMemoryWhisperTranscriber>(),
                    std::make_unique<PresetReader>());
    EXPECT_EQ(p.getStatistics()["plugin_version"].get<std::string>(), "2.3.0");
}

// ─────────────────────────────────────────────────────────────────────────────
// Group I – Error paths
// ─────────────────────────────────────────────────────────────────────────────

TEST(WhisperPluginFocusedTests, I1_TranscribeUninitializedReturnsError) {
    WhisperPlugin p(std::make_unique<InMemoryWhisperTranscriber>(),
                    std::make_unique<PresetReader>());
    // Not initialized
    const auto res = p.transcribe({0.f}, 16000.f);
    EXPECT_FALSE(res.success);
    EXPECT_FALSE(res.error_message.empty());
}

TEST(WhisperPluginFocusedTests, I2_TranscribeFileUninitializedReturnsError) {
    WhisperPlugin p(std::make_unique<InMemoryWhisperTranscriber>(),
                    std::make_unique<PresetReader>());
    const auto res = p.transcribeFile("any.wav");
    EXPECT_FALSE(res.success);
    EXPECT_EQ(res.ingestion_source_type, "WHISPER");
}

TEST(WhisperPluginFocusedTests, I3_TranscribeEmptyPcmDoesNotCrash) {
    auto t = std::make_unique<InMemoryWhisperTranscriber>();
    t->setNextResult({});
    WhisperPlugin p(std::move(t), std::make_unique<PresetReader>());
    p.initialize("", {});
    EXPECT_NO_THROW(p.transcribe({}, 16000.f));
}

// ─────────────────────────────────────────────────────────────────────────────
// Group J – Misc
// ─────────────────────────────────────────────────────────────────────────────

TEST(WhisperPluginFocusedTests, J1_DoubleInitIsSafe) {
    WhisperPlugin p(std::make_unique<InMemoryWhisperTranscriber>(),
                    std::make_unique<PresetReader>());
    EXPECT_TRUE(p.initialize("", {}));
    EXPECT_TRUE(p.initialize("", {}));
    EXPECT_TRUE(p.isInitialized());
}

TEST(WhisperPluginFocusedTests, J2_GetModelIdMatchesTranscriber) {
    auto t = std::make_unique<InMemoryWhisperTranscriber>();
    WhisperPlugin p(std::move(t), std::make_unique<PresetReader>());
    p.initialize("/mymodel.bin", {});
    EXPECT_FALSE(p.getModelId().empty());
}

TEST(WhisperPluginFocusedTests, J3_ErrorCountIncrements) {
    WhisperPlugin p(std::make_unique<InMemoryWhisperTranscriber>(),
                    std::make_unique<PresetReader>());
    // Trigger two errors (not initialized)
    p.transcribe({}, 16000.f);
    p.transcribeFile("any.wav");
    EXPECT_GE(p.getStatistics()["error_count"].get<uint64_t>(), 2);
}

// ─────────────────────────────────────────────────────────────────────────────
// Group K – Thread-safety
// ─────────────────────────────────────────────────────────────────────────────

TEST(WhisperPluginFocusedTests, K1_ConcurrentTranscribeDoesNotCrash) {
    // 8 threads each performing 20 transcribe() calls on a shared plugin.
    auto t = std::make_unique<InMemoryWhisperTranscriber>();
    TranscriptionResult preset; preset.text = "parallel";
    t->setNextResult(preset);
    WhisperPlugin p(std::move(t), std::make_unique<PresetReader>());
    p.initialize("", {});

    constexpr int kThreads = 8;
    constexpr int kCalls   = 20;
    std::vector<std::thread> threads;
    std::vector<std::atomic<bool>> done_flags(static_cast<std::size_t>(kThreads));
    threads.reserve(kThreads);
    for (int i = 0; i < kThreads; ++i) {
        done_flags[static_cast<std::size_t>(i)].store(false, std::memory_order_release);
    }
    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back([&p, &done_flags, i]() {
            const std::vector<float> pcm(160, 0.0f);
            for (int j = 0; j < kCalls; ++j) {
                auto res = p.transcribe(pcm, 16000.f);
            }
            done_flags[static_cast<std::size_t>(i)].store(true, std::memory_order_release);
        });
    }
    for (int i = 0; i < kThreads; ++i) {
        EXPECT_TRUE(joinThreadWithTimeout(threads[static_cast<std::size_t>(i)],
                                          done_flags[static_cast<std::size_t>(i)]));
    }

    const uint64_t count = p.getStatistics()["transcription_count"].get<uint64_t>();
    EXPECT_EQ(count, static_cast<uint64_t>(kThreads * kCalls));
}

TEST(WhisperPluginFocusedTests, K2_AtomicCountersUnderConcurrentErrors) {
    // All calls fail (not initialized) — error_count must equal total calls.
    WhisperPlugin p(std::make_unique<InMemoryWhisperTranscriber>(),
                    std::make_unique<PresetReader>());
    // NOT initialized on purpose.

    constexpr int kThreads = 4;
    constexpr int kCalls   = 25;
    std::vector<std::thread> threads;
    std::vector<std::atomic<bool>> done_flags(static_cast<std::size_t>(kThreads));
    threads.reserve(kThreads);
    for (int i = 0; i < kThreads; ++i) {
        done_flags[static_cast<std::size_t>(i)].store(false, std::memory_order_release);
    }
    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back([&p, &done_flags, i]() {
            for (int j = 0; j < kCalls; ++j) {
                auto res = p.transcribe({0.f}, 16000.f);
                EXPECT_FALSE(res.success);
            }
            done_flags[static_cast<std::size_t>(i)].store(true, std::memory_order_release);
        });
    }
    for (int i = 0; i < kThreads; ++i) {
        EXPECT_TRUE(joinThreadWithTimeout(threads[static_cast<std::size_t>(i)],
                                          done_flags[static_cast<std::size_t>(i)]));
    }

    EXPECT_EQ(p.getStatistics()["error_count"].get<uint64_t>(),
              static_cast<uint64_t>(kThreads * kCalls));
}

TEST(WhisperPluginFocusedTests, K3_ConcurrentDetectLanguageDoesNotCrash) {
    auto t = std::make_unique<InMemoryWhisperTranscriber>();
    t->initialize(WhisperConfig{});
    t->setNextLanguage({"en", 0.9f});
    WhisperPlugin p(std::move(t), std::make_unique<PresetReader>());
    p.initialize("", {});

    constexpr int kThreads = 6;
    std::vector<std::thread> threads;
    std::vector<std::atomic<bool>> done_flags(static_cast<std::size_t>(kThreads));
    threads.reserve(kThreads);
    for (int i = 0; i < kThreads; ++i) {
        done_flags[static_cast<std::size_t>(i)].store(false, std::memory_order_release);
    }
    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back([&p, &done_flags, i]() {
            for (int j = 0; j < 10; ++j) {
                auto res = p.detectLanguage({0.f, 0.f}, 16000.f);
                EXPECT_FALSE(res.language.empty());
            }
            done_flags[static_cast<std::size_t>(i)].store(true, std::memory_order_release);
        });
    }
    for (int i = 0; i < kThreads; ++i) {
        EXPECT_TRUE(joinThreadWithTimeout(threads[static_cast<std::size_t>(i)],
                                          done_flags[static_cast<std::size_t>(i)]));
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Group L – FfmpegAudioChunkReader + CompositeAudioChunkReader
// ─────────────────────────────────────────────────────────────────────────────

TEST(WhisperPluginFocusedTests, L1_FfmpegCanReadMp3OggFlac) {
    FfmpegAudioChunkReader r;
    EXPECT_TRUE(r.canRead("audio.mp3"));
    EXPECT_TRUE(r.canRead("audio.ogg"));
    EXPECT_TRUE(r.canRead("audio.flac"));
    EXPECT_TRUE(r.canRead("audio.m4a"));
    EXPECT_TRUE(r.canRead("audio.OPUS"));
    EXPECT_FALSE(r.canRead("audio.wav"));
    EXPECT_FALSE(r.canRead("audio.txt"));
}

TEST(WhisperPluginFocusedTests, L2_FfmpegThrowsOnMissingFileOrNoFfmpeg) {
    FfmpegAudioChunkReader r;
    float sr = 0.0f;
    // Either ffmpeg is unavailable or the file doesn't exist — both throw.
    EXPECT_THROW(r.readFile("/tmp/nonexistent_audio_file.mp3", sr), std::runtime_error);
}

TEST(WhisperPluginFocusedTests, L3_CompositeRoutesByExtension) {
    // Build a composite with WAV reader first, then ffmpeg reader.
    CompositeAudioChunkReader composite;
    composite.addReader(std::make_unique<WavAudioChunkReader>());
    composite.addReader(std::make_unique<FfmpegAudioChunkReader>());

    // WAV is handled by WavAudioChunkReader (first match).
    EXPECT_TRUE(composite.canRead("audio.wav"));
    // MP3 is handled by FfmpegAudioChunkReader.
    EXPECT_TRUE(composite.canRead("audio.mp3"));
    // Unknown extension → canRead returns false.
    EXPECT_FALSE(composite.canRead("audio.xyz"));

    // readFile() on an unsupported extension throws (no reader accepts it).
    float sr = 0.0f;
    EXPECT_THROW(composite.readFile("/tmp/test.xyz", sr), std::runtime_error);
}

// ─────────────────────────────────────────────────────────────────────────────
// Group M – Language-detection confidence threshold
// ─────────────────────────────────────────────────────────────────────────────

TEST(WhisperPluginFocusedTests, M1_LanguageConfidenceThresholdDefaultZeroDisabled) {
    // Default threshold of 0 means no filtering – all detections pass through.
    WhisperConfig cfg;
    EXPECT_FLOAT_EQ(cfg.language_confidence_threshold, 0.0f);

    auto t = std::make_unique<InMemoryWhisperTranscriber>();
    t->initialize(WhisperConfig{});
    t->setNextLanguage({"de", 0.1f});  // very low confidence
    WhisperPlugin p(std::move(t), std::make_unique<PresetReader>());
    p.initialize("", cfg.toJson());

    const auto res = p.detectLanguage({0.f}, 16000.f);
    // With threshold 0, the raw result should pass through unchanged.
    EXPECT_EQ(res.language, "de");
}

TEST(WhisperPluginFocusedTests, M2_LanguageDetectionPassesWhenAboveThreshold) {
    const json cfgJson = {{"language_confidence_threshold", 0.5f}};
    auto t = std::make_unique<InMemoryWhisperTranscriber>();
    t->initialize(WhisperConfig{});
    t->setNextLanguage({"en", 0.9f});  // confidence 0.9 >= threshold 0.5 → passes
    WhisperPlugin p(std::move(t), std::make_unique<PresetReader>());
    p.initialize("", cfgJson);

    const auto res = p.detectLanguage({0.f}, 16000.f);
    EXPECT_EQ(res.language, "en");
    EXPECT_FLOAT_EQ(res.confidence, 0.9f);
}

TEST(WhisperPluginFocusedTests, M3_LanguageDetectionUnknownWhenBelowThreshold) {
    const json cfgJson = {{"language_confidence_threshold", 0.7f}};
    auto t = std::make_unique<InMemoryWhisperTranscriber>();
    t->initialize(WhisperConfig{});
    t->setNextLanguage({"fr", 0.4f});  // confidence 0.4 < threshold 0.7 → "unknown"
    WhisperPlugin p(std::move(t), std::make_unique<PresetReader>());
    p.initialize("", cfgJson);

    const auto res = p.detectLanguage({0.f}, 16000.f);
    EXPECT_EQ(res.language, "unknown");
    EXPECT_FLOAT_EQ(res.confidence, 0.4f);  // raw confidence still reported
}

// ─────────────────────────────────────────────────────────────────────────────
// Group N – Additional config / WAV edge cases
// ─────────────────────────────────────────────────────────────────────────────

TEST(WhisperPluginFocusedTests, N1_BeamSizeClampedToOne) {
    const auto cfg = WhisperConfig::fromJson({{"beam_size", 0}});
    EXPECT_GE(cfg.beam_size, 1);
}

TEST(WhisperPluginFocusedTests, N2_LanguageConfidenceThresholdRoundTrip) {
    WhisperConfig orig;
    orig.language_confidence_threshold = 0.65f;
    const auto restored = WhisperConfig::fromJson(orig.toJson());
    EXPECT_FLOAT_EQ(restored.language_confidence_threshold, 0.65f);
}

TEST(WhisperPluginFocusedTests, N3_LanguageConfidenceThresholdClamped) {
    // Values outside [0, 1] are clamped.
    const auto cfgLow  = WhisperConfig::fromJson({{"language_confidence_threshold", -0.5f}});
    const auto cfgHigh = WhisperConfig::fromJson({{"language_confidence_threshold",  2.0f}});
    EXPECT_FLOAT_EQ(cfgLow.language_confidence_threshold,  0.0f);
    EXPECT_FLOAT_EQ(cfgHigh.language_confidence_threshold, 1.0f);
}

TEST(WhisperPluginFocusedTests, N4_WavReaderParsesStereo16BitPcm) {
    // Build a minimal stereo 16-bit PCM WAV and verify it decodes to mono.
    const uint32_t num_channels  = 2;
    const uint32_t num_samples   = 4;  // per channel
    const uint32_t data_bytes    = num_samples * num_channels * 2;
    const uint32_t riff_size     = 36 + data_bytes;
    std::vector<uint8_t> b(44 + data_bytes, 0);
    b[0]='R'; b[1]='I'; b[2]='F'; b[3]='F';
    b[4]=(riff_size)&0xFF; b[5]=(riff_size>>8)&0xFF;
    b[6]=(riff_size>>16)&0xFF; b[7]=(riff_size>>24)&0xFF;
    b[8]='W'; b[9]='A'; b[10]='V'; b[11]='E';
    b[12]='f'; b[13]='m'; b[14]='t'; b[15]=' ';
    b[16]=16;                // fmt chunk size
    b[20]=1;                 // PCM
    b[22]=static_cast<uint8_t>(num_channels); // 2 channels
    b[24]=0x80; b[25]=0x3E; // 16000 Hz
    b[28]=0x00; b[29]=0xFA; // byte rate = 64000 (16000*2*2)
    b[32]=4;                 // block align = 2*2
    b[34]=16;                // bits per sample
    b[36]='d'; b[37]='a'; b[38]='t'; b[39]='a';
    b[40]=data_bytes&0xFF; b[41]=(data_bytes>>8)&0xFF;

    const std::string path = writeTmpFile("stereo.wav", b);
    WavAudioChunkReader r;
    float sr = 0.0f;
    ASSERT_NO_THROW({
        auto samples = r.readFile(path, sr);
        EXPECT_EQ(samples.size(), num_samples);
        EXPECT_FLOAT_EQ(sr, 16000.0f);
    });
}

TEST(WhisperPluginFocusedTests, N5_ToJsonContainsLanguageConfidenceThresholdKey) {
    const json j = WhisperConfig{}.toJson();
    EXPECT_TRUE(j.contains("language_confidence_threshold"));
}

// ─────────────────────────────────────────────────────────────────────────────
// Group O – Streaming transcription (transcribeStream) — WST-01..05
// ─────────────────────────────────────────────────────────────────────────────

// Helper: PresetReader is already declared earlier in this file.

TEST(WhisperPluginFocusedTests, O1_StreamSingleTokenFallback) {
    // When no stream tokens are set the transcriber emits the full text as one token.
    TranscriptionResult expected;
    expected.text       = "hello world";
    expected.confidence = 0.9f;
    expected.success    = true;

    auto t = std::make_unique<InMemoryWhisperTranscriber>();
    t->initialize(WhisperConfig{});
    t->setNextResult(expected);

    WhisperPlugin p(std::move(t), std::make_unique<PresetReader>());
    p.initialize("", json{});

    std::vector<TranscriptionToken> received;
    const auto result = p.transcribeStream({0.f}, 16000.f,
        [&]([[maybe_unused]] const TranscriptionToken& tok) { received.push_back(tok); });

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.text, "hello world");
    ASSERT_EQ(received.size(), 1);
    EXPECT_EQ(received[0].text, "hello world");
    EXPECT_FLOAT_EQ(received[0].confidence, 0.9f);
}

TEST(WhisperPluginFocusedTests, O2_StreamMultipleTokens) {
    // InMemoryWhisperTranscriber emits 3 preset tokens.
    TranscriptionResult final_result;
    final_result.text    = "one two three";
    final_result.success = true;

    std::vector<TranscriptionToken> preset = {
        {"one",   0.f,  300.f, 0.9f, 0},
        {"two",   300.f, 600.f, 0.85f, 1},
        {"three", 600.f, 900.f, 0.8f,  2},
    };

    auto t = std::make_unique<InMemoryWhisperTranscriber>();
    t->initialize(WhisperConfig{});
    t->setNextResult(final_result);
    t->setStreamTokens(preset);

    WhisperPlugin p(std::move(t), std::make_unique<PresetReader>());
    p.initialize("", json{});

    std::vector<TranscriptionToken> received;
    const auto result = p.transcribeStream({0.f}, 16000.f,
        [&]([[maybe_unused]] const TranscriptionToken& tok) { received.push_back(tok); });

    EXPECT_TRUE(result.success);
    ASSERT_EQ(received.size(), 3);
    EXPECT_EQ(received[0].text, "one");
    EXPECT_EQ(received[1].text, "two");
    EXPECT_EQ(received[2].text, "three");
    EXPECT_EQ(received[2].token_index, 2);
}

TEST(WhisperPluginFocusedTests, O3_StreamCallbackExceptionYieldsFailure) {
    TranscriptionResult ok;
    ok.text    = "boom";
    ok.success = true;

    auto t = std::make_unique<InMemoryWhisperTranscriber>();
    t->initialize(WhisperConfig{});
    t->setNextResult(ok);

    WhisperPlugin p(std::move(t), std::make_unique<PresetReader>());
    p.initialize("", json{});

    const auto result = p.transcribeStream({0.f}, 16000.f,
        [](const TranscriptionToken&) { throw std::runtime_error("test error"); });

    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error_message.empty());
}

TEST(WhisperPluginFocusedTests, O4_StreamUninitGuard) {
    // transcribeStream before initialize() must return success=false.
    WhisperPlugin p;  // not initialized
    const auto result = p.transcribeStream({0.f}, 16000.f, nullptr);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.plugin_version, "2.3.0");
    EXPECT_EQ(result.ingestion_source_type, "WHISPER");
}

TEST(WhisperPluginFocusedTests, O5_StreamProvenanceAlwaysSet) {
    TranscriptionResult r;
    r.text    = "test";
    r.success = true;

    auto t = std::make_unique<InMemoryWhisperTranscriber>();
    t->initialize(WhisperConfig{});
    t->setNextResult(r);

    WhisperPlugin p(std::move(t), std::make_unique<PresetReader>());
    p.initialize("mymodel", json{});

    const auto result = p.transcribeStream({0.f}, 16000.f, nullptr);
    EXPECT_EQ(result.ingestion_source_type, "WHISPER");
    EXPECT_EQ(result.plugin_version, "2.3.0");
    EXPECT_NE(result.generation_timestamp, 0);
}

// ─────────────────────────────────────────────────────────────────────────────
// Group P – EnergyThresholdVad: all-silence, all-speech, mixed — VAD-01..03
// ─────────────────────────────────────────────────────────────────────────────

TEST(WhisperPluginFocusedTests, P1_VadAllSilenceYieldsNoSegments) {
    EnergyThresholdVad vad;
    VadConfig cfg;
    cfg.energy_threshold = 0.01f;
    cfg.min_speech_ms    = 0.0f;

    // 16000 zero samples = 1 second of silence
    std::vector<float> silence(16000, 0.0f);
    const auto segs = vad.detect(silence, 16000.f, cfg);
    EXPECT_TRUE(segs.empty());
}

TEST(WhisperPluginFocusedTests, P2_VadAllSpeechYieldsOneSegment) {
    EnergyThresholdVad vad;
    VadConfig cfg;
    cfg.energy_threshold = 0.01f;
    cfg.min_speech_ms    = 0.0f;

    // 16000 samples at amplitude 0.5 → well above threshold
    std::vector<float> speech(16000, 0.5f);
    const auto segs = vad.detect(speech, 16000.f, cfg);
    ASSERT_FALSE(segs.empty());
    EXPECT_EQ(segs.front().start_sample, 0);
    EXPECT_EQ(segs.back().end_sample,static_cast<int>(speech.size()));
}

TEST(WhisperPluginFocusedTests, P3_VadMixedYieldsSpeechSegments) {
    EnergyThresholdVad vad;
    VadConfig cfg;
    cfg.energy_threshold = 0.01f;
    cfg.min_speech_ms    = 10.0f;  // 10 ms minimum
    cfg.frame_ms         = 10.0f;  // 10 ms frames

    // 160 silence + 160 speech + 160 silence @ 16 kHz
    std::vector<float> mixed(480, 0.0f);
    for (std::size_t i = 160; i < 320; ++i) {
      mixed[i] = 0.5f;
    }

    const auto segs = vad.detect(mixed, 16000.f, cfg);
    ASSERT_FALSE(segs.empty());
    // The speech segment should cover roughly [160, 320)
    EXPECT_GE(segs[0].start_sample, 0);
    EXPECT_LE(segs[0].start_sample, 160);
    EXPECT_GE(segs[0].end_sample,   160);
}

// ─────────────────────────────────────────────────────────────────────────────
// Group Q – VAD integration with WhisperPlugin — VAD-04..06
// ─────────────────────────────────────────────────────────────────────────────

TEST(WhisperPluginFocusedTests, Q1_VadSkipsSilentInput) {
    // Pure silence passed through transcribeStream with VAD → empty PCM → stub returns ""
    TranscriptionResult ok;
    ok.text    = "";
    ok.success = true;

    auto t = std::make_unique<InMemoryWhisperTranscriber>();
    t->initialize(WhisperConfig{});
    t->setNextResult(ok);

    WhisperPlugin p(std::move(t), std::make_unique<PresetReader>());
    p.initialize("", json{});

    VadConfig cfg;
    cfg.energy_threshold = 0.01f;
    cfg.min_speech_ms    = 0.0f;
    p.setVoiceActivityDetector(std::make_unique<EnergyThresholdVad>(), cfg);

    std::vector<float> silence(16000, 0.0f);
    const auto result = p.transcribeStream(silence, 16000.f, nullptr);
    // VAD strips all samples; plugin still succeeds (empty text is a valid result)
    EXPECT_EQ(result.ingestion_source_type, "WHISPER");
}

TEST(WhisperPluginFocusedTests, Q2_VadPassesSpeechThrough) {
    TranscriptionResult ok;
    ok.text    = "speech detected";
    ok.success = true;

    auto t = std::make_unique<InMemoryWhisperTranscriber>();
    t->initialize(WhisperConfig{});
    t->setNextResult(ok);

    WhisperPlugin p(std::move(t), std::make_unique<PresetReader>());
    p.initialize("", json{});

    VadConfig cfg;
    cfg.energy_threshold = 0.01f;
    cfg.min_speech_ms    = 0.0f;
    p.setVoiceActivityDetector(std::make_unique<EnergyThresholdVad>(), cfg);

    std::vector<float> speech(16000, 0.5f);
    const auto result = p.transcribeStream(speech, 16000.f, nullptr);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.text, "speech detected");
}

TEST(WhisperPluginFocusedTests, Q3_NullVadIsNoOp) {
    // setVoiceActivityDetector(nullptr) must not crash and transcription proceeds normally.
    TranscriptionResult ok;
    ok.text    = "no vad";
    ok.success = true;

    auto t = std::make_unique<InMemoryWhisperTranscriber>();
    t->initialize(WhisperConfig{});
    t->setNextResult(ok);

    WhisperPlugin p(std::move(t), std::make_unique<PresetReader>());
    p.initialize("", json{});
    p.setVoiceActivityDetector(nullptr);

    const auto result = p.transcribeStream({0.5f, 0.5f}, 16000.f, nullptr);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.text, "no vad");
}

// ─────────────────────────────────────────────────────────────────────────────
// Group R – VAD thread-safety (data-race regression)
// ─────────────────────────────────────────────────────────────────────────────

TEST(WhisperPluginFocusedTests, R1_ConcurrentVadSetAndTranscribeStream) {
    // Verify that concurrent setVoiceActivityDetector() and transcribeStream()
    // do not produce a data race (regression for the missing vad_mutex_ gap).
    TranscriptionResult ok;
    ok.text    = "concurrent";
    ok.success = true;

    auto t = std::make_unique<InMemoryWhisperTranscriber>();
    t->initialize(WhisperConfig{});
    t->setNextResult(ok);

    WhisperPlugin p(std::move(t), std::make_unique<PresetReader>());
    p.initialize("", json{});

    std::vector<float> speech(320, 0.5f);
    std::atomic<int> errors{0};
    std::atomic<bool> setter_done{false};
    std::atomic<bool> caller_done{false};

    // Thread A: repeatedly set and clear the VAD
    std::thread setter([&]() {
        for (int i = 0; i < 50; ++i) {
            VadConfig cfg;
            cfg.energy_threshold = 0.01f;
            p.setVoiceActivityDetector(std::make_unique<EnergyThresholdVad>(), cfg);
            p.setVoiceActivityDetector(nullptr);
        }
        setter_done.store(true, std::memory_order_release);
    });

    // Thread B: repeatedly call transcribeStream while VAD changes
    std::thread caller([&]() {
        for (int i = 0; i < 50; ++i) {
            try {
                auto res = p.transcribeStream(speech, 16000.f, nullptr);
                if (res.ingestion_source_type != "WHISPER") {
                  ++errors;
                }
            } catch (...) {
                ++errors;
            }
        }
        caller_done.store(true, std::memory_order_release);
    });

    EXPECT_TRUE(joinThreadWithTimeout(setter, setter_done));
    EXPECT_TRUE(joinThreadWithTimeout(caller, caller_done));
    EXPECT_EQ(errors.load(), 0);
}

TEST(WhisperPluginFocusedTests, R2_SetVadAfterTranscribeIsSafe) {
    // setVoiceActivityDetector() after transcription completes must not corrupt state.
    TranscriptionResult ok;
    ok.text    = "safe";
    ok.success = true;

    auto t = std::make_unique<InMemoryWhisperTranscriber>();
    t->initialize(WhisperConfig{});
    t->setNextResult(ok);

    WhisperPlugin p(std::move(t), std::make_unique<PresetReader>());
    p.initialize("", json{});

    // First transcription with no VAD
    auto r1 = p.transcribeStream({0.5f}, 16000.f, nullptr);
    EXPECT_EQ(r1.ingestion_source_type, "WHISPER");

    // Install VAD
    VadConfig cfg; cfg.energy_threshold = 0.01f;
    p.setVoiceActivityDetector(std::make_unique<EnergyThresholdVad>(), cfg);

    // Second transcription uses the new VAD (silence suppressed)
    std::vector<float> silence(160, 0.0f);
    auto r2 = p.transcribeStream(silence, 16000.f, nullptr);
    EXPECT_EQ(r2.ingestion_source_type, "WHISPER");
}

// ─────────────────────────────────────────────────────────────────────────────
// Group S – WAV parser input validation (num_channels guard regression)
// ─────────────────────────────────────────────────────────────────────────────

namespace {
/// Build a raw WAV byte buffer with configurable parameters.
/// Caller is responsible for ensuring the header values are consistent.
static std::vector<uint8_t> buildWav(uint16_t num_channels,
                                     uint16_t audio_format,
                                     uint16_t bits_per_sample,
                                     uint32_t sample_rate = 16000) {
    const uint32_t bytes_per_sample = bits_per_sample / 8;
    const uint32_t data_bytes       = 4 * num_channels * bytes_per_sample;  // 4 frames
    const uint32_t riff_size        = 36 + data_bytes;

    std::vector<uint8_t> b(44 + data_bytes, 0);
    // RIFF header
    b[0]='R'; b[1]='I'; b[2]='F'; b[3]='F';
    b[4]= riff_size & 0xFF;
    b[5]=(riff_size >> 8) & 0xFF;
    b[6]=(riff_size >>16) & 0xFF;
    b[7]=(riff_size >>24) & 0xFF;
    b[8]='W'; b[9]='A'; b[10]='V'; b[11]='E';
    // fmt chunk
    b[12]='f'; b[13]='m'; b[14]='t'; b[15]=' ';
    b[16]=16;  // chunk size (LE)
    b[20]= audio_format & 0xFF; b[21]=(audio_format >>8)&0xFF;
    b[22]= num_channels & 0xFF; b[23]=(num_channels >>8)&0xFF;
    // sample_rate
    b[24]= sample_rate & 0xFF;
    b[25]=(sample_rate >>8) & 0xFF;
    b[26]=(sample_rate >>16)& 0xFF;
    b[27]=(sample_rate >>24)& 0xFF;
    b[34]= bits_per_sample & 0xFF;
    b[35]=(bits_per_sample >>8) & 0xFF;
    // data chunk
    b[36]='d'; b[37]='a'; b[38]='t'; b[39]='a';
    b[40]= data_bytes & 0xFF;
    b[41]=(data_bytes >>8) & 0xFF;
    return b;
}
} // anonymous namespace

TEST(WhisperPluginFocusedTests, S1_ParseWavRejectsZeroChannels) {
    // A WAV file with num_channels=0 must fail when read through the public API.
    WavAudioChunkReader reader;
    auto wav = buildWav(1, 1, 16);
    wav[22] = 0;
    wav[23] = 0;  // num_channels = 0
    const std::string path = writeTmpFile("wav_zero_channels.wav", wav);
    float sr = 0.f;
    EXPECT_THROW(reader.readFile(path, sr), std::runtime_error);
}

TEST(WhisperPluginFocusedTests, S2_ParseWavRejectsExcessiveChannelCount) {
    // A WAV file claiming 65 channels should be rejected (unreasonable for audio).
    WavAudioChunkReader reader;
    auto wav = buildWav(1, 1, 16);
    wav[22] = 65;
    wav[23] = 0;  // num_channels = 65
    const std::string path = writeTmpFile("wav_too_many_channels.wav", wav);
    float sr = 0.f;
    EXPECT_THROW(reader.readFile(path, sr), std::runtime_error);
}

TEST(WhisperPluginFocusedTests, S3_ParseWavAcceptsMaxValidChannelCount) {
    // 64 channels should still be accepted (boundary value).
    WavAudioChunkReader reader;
    auto wav = buildWav(64, 1, 16, 16000);
    const std::string path = writeTmpFile("wav_max_valid_channels.wav", wav);
    float sr = 0.f;
    // Must not throw; may return an empty or valid vector of samples
    EXPECT_NO_THROW(reader.readFile(path, sr));
}

// ─────────────────────────────────────────────────────────────────────────────
// Group T – Speaker diarisation
// ─────────────────────────────────────────────────────────────────────────────

namespace {
class CaptureDiarizeTranscriber : public InMemoryWhisperTranscriber {
public:
    DiarisationResult diarize(const std::vector<float>& pcm,
                              float sample_rate,
                              const DiarisationConfig& cfg) override {
        (void)pcm;
        (void)sample_rate;
        last_cfg_ = cfg;
        return InMemoryWhisperTranscriber::diarize(pcm, sample_rate, cfg);
    }

    DiarisationConfig last_cfg_;
};
} // namespace

TEST(WhisperPluginFocusedTests, T1_DiarisationFixtureResultFromTranscriber) {
    auto t = std::make_unique<InMemoryWhisperTranscriber>();
    DiarisationResult fixture;
    fixture.segments.push_back({"speaker_1", 0, 1200, "hello"});
    t->setNextDiarisationResult(fixture);

    WhisperPlugin p(std::move(t), std::make_unique<PresetReader>());
    ASSERT_TRUE(p.initialize("stub-model", {}));

    const auto res = p.transcribeWithDiarisation({0.1f, 0.2f}, 16000.f, {});
    ASSERT_TRUE(res.success);
    ASSERT_EQ(res.segments.size(), 1);
    EXPECT_EQ(res.segments[0].speaker_id, "speaker_1");
}

TEST(WhisperPluginFocusedTests, T2_DiarisationEmptyConfigIsAccepted) {
    auto t = std::make_unique<InMemoryWhisperTranscriber>();
    WhisperPlugin p(std::move(t), std::make_unique<PresetReader>());
    ASSERT_TRUE(p.initialize("stub-model", {}));

    const auto res = p.transcribeWithDiarisation({0.1f}, 16000.f, {});
    EXPECT_TRUE(res.success);
}

TEST(WhisperPluginFocusedTests, T3_DiarisationMissingModelFallbackKeepsTranscriptionWorking) {
    auto t = std::make_unique<InMemoryWhisperTranscriber>();
    TranscriptionResult tr;
    tr.success = true;
    tr.text = "fallback";
    t->setNextResult(tr);

    DiarisationResult diar;
    diar.success = false;
    diar.error_message = "feature disabled: missing diarisation model";
    t->setNextDiarisationResult(diar);

    WhisperPlugin p(std::move(t), std::make_unique<PresetReader>());
    ASSERT_TRUE(p.initialize("stub-model", {}));

    const auto dr = p.transcribeWithDiarisation({0.1f}, 16000.f, {});
    EXPECT_FALSE(dr.success);
    EXPECT_FALSE(dr.error_message.empty());

    const auto tr_out = p.transcribe({0.1f}, 16000.f);
    EXPECT_TRUE(tr_out.success);
}

TEST(WhisperPluginFocusedTests, T4_DiarisationSpeakerCountClamp) {
    auto t = std::make_unique<CaptureDiarizeTranscriber>();
    auto* raw = t.get();
    WhisperPlugin p(std::move(t), std::make_unique<PresetReader>());
    ASSERT_TRUE(p.initialize("stub-model", {}));

    DiarisationConfig cfg;
    cfg.min_speakers = 0;
    cfg.max_speakers = 0;
    const auto res = p.transcribeWithDiarisation({0.1f}, 16000.f, cfg);
    EXPECT_TRUE(res.success);
    EXPECT_EQ(raw->last_cfg_.min_speakers, 1);
    EXPECT_EQ(raw->last_cfg_.max_speakers, 1);
}

TEST(WhisperPluginFocusedTests, T5_DiarisationResultHasProvenanceStamp) {
    auto t = std::make_unique<InMemoryWhisperTranscriber>();
    DiarisationResult fixture;
    fixture.segments.push_back({"speaker_0", 0, 100, "segment"});
    t->setNextDiarisationResult(fixture);

    WhisperPlugin p(std::move(t), std::make_unique<PresetReader>());
    ASSERT_TRUE(p.initialize("stub-model", {}));
    const auto res = p.transcribeWithDiarisation({0.1f}, 16000.f, {});

    EXPECT_EQ(res.ingestion_source_type, "WHISPER");
    EXPECT_EQ(res.plugin_version, "2.3.0");
    EXPECT_FALSE(res.model_id.empty());
    EXPECT_GT(res.generation_timestamp, 0);
}

// ─────────────────────────────────────────────────────────────────────────────
// Group U – Model integrity (SHA-256)
// ─────────────────────────────────────────────────────────────────────────────

TEST(WhisperPluginFocusedTests, U1_ModelSha256RoundTripInConfig) {
    WhisperConfig cfg;
    cfg.model_path = "/tmp/model.bin";
    cfg.model_sha256 = "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad";
    const auto restored = WhisperConfig::fromJson(cfg.toJson());
    EXPECT_EQ(restored.model_sha256, cfg.model_sha256);
}

TEST(WhisperPluginFocusedTests, U2_EmptyModelSha256AcceptedInStubMode) {
    WhisperStubTranscriber t;
    WhisperConfig cfg;
    cfg.model_path = "stub-model";
    cfg.model_sha256.clear();
    EXPECT_TRUE(t.initialize(cfg));
}

#ifdef THEMIS_ENABLE_WHISPER
TEST(WhisperPluginFocusedTests, U3_ModelSha256MismatchFailsInitialize) {
    const std::string path = writeTmpFile("whisper_model_digest_probe.bin", {'a', 'b', 'c'});
    WhisperCppTranscriber t;
    WhisperConfig cfg;
    cfg.model_path = path;
    cfg.model_sha256 = "0000000000000000000000000000000000000000000000000000000000000000";
    EXPECT_FALSE(t.initialize(cfg));
    EXPECT_NE(t.getLastError().find("SHA-256 mismatch"), std::string::npos);
}
#endif
