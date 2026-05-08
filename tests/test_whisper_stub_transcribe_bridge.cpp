/**
 * @file test_whisper_stub_transcribe_bridge.cpp
 * @brief Unit tests for the TranscribeFn injection bridge on
 *        WhisperStubTranscriber (STUB #96).
 *
 * Test IDs:
 *   WST-01  Injected TranscribeFn is called instead of empty stub
 *   WST-02  nullptr fn restores the empty-result default
 *   WST-03  TranscribeFn result is propagated through WhisperPlugin
 */

#include <gtest/gtest.h>
#include "whisper/whisper_transcriber.h"
#include "whisper/whisper_plugin.h"
#include "whisper/whisper_config.h"
#include "whisper/audio_chunk_reader.h"

#include <atomic>
#include <memory>
#include <string>
#include <vector>

using namespace themis::whisper;
using namespace themis::audio;

// ─────────────────────────────────────────────────────────────────────────────
// WST-01: Injected TranscribeFn is called instead of empty stub
// ─────────────────────────────────────────────────────────────────────────────
TEST(WhisperStubTranscribeBridge, WST_01_InjectedFnCalledInsteadOfEmpty) {
    auto stub = std::make_unique<WhisperStubTranscriber>();
    WhisperConfig cfg;
    cfg.model_path = "/tmp/stub_model";
    stub->initialize(cfg);

    std::atomic<int> call_count{0};
    const std::string expected_text = "injected transcription result";
    const float       expected_conf = 0.93f;

    stub->setTranscribeFn(
        [&](const std::vector<float>& /*pcm*/, float /*sr*/)
            -> TranscriptionResult {
            ++call_count;
            TranscriptionResult r;
            r.text       = expected_text;
            r.confidence = expected_conf;
            r.success    = true;
            return r;
        });

    const std::vector<float> pcm(1024, 0.0f);
    const auto result = stub->transcribe(pcm, 16000.0f);

    EXPECT_EQ(call_count.load(), 1) << "Injected fn should be called once";
    EXPECT_EQ(result.text, expected_text)
        << "Transcribed text should match injected fn output";
    EXPECT_FLOAT_EQ(result.confidence, expected_conf);
    // Provenance fields should be overridden by the stub.
    EXPECT_EQ(result.ingestion_source_type, "WHISPER");
    EXPECT_EQ(result.plugin_version, "2.0.0");
}

// ─────────────────────────────────────────────────────────────────────────────
// WST-02: nullptr fn restores the empty-result default
// ─────────────────────────────────────────────────────────────────────────────
TEST(WhisperStubTranscribeBridge, WST_02_NullFnRestoresEmptyResult) {
    auto stub = std::make_unique<WhisperStubTranscriber>();
    WhisperConfig cfg;
    stub->initialize(cfg);

    // Set fn then clear it.
    stub->setTranscribeFn(
        [](const std::vector<float>&, float) -> TranscriptionResult {
            TranscriptionResult r;
            r.text = "should not appear";
            return r;
        });
    stub->setTranscribeFn(nullptr);

    const std::vector<float> pcm(512, 0.1f);
    const auto result = stub->transcribe(pcm, 16000.0f);

    EXPECT_EQ(result.text, "") << "Cleared fn should restore empty-text stub result";
    EXPECT_EQ(result.language, "unknown");
    EXPECT_FLOAT_EQ(result.confidence, 0.0f);
}

// ─────────────────────────────────────────────────────────────────────────────
// WST-03: TranscribeFn result propagated through WhisperPlugin
// ─────────────────────────────────────────────────────────────────────────────
TEST(WhisperStubTranscribeBridge, WST_03_ResultPropagatedThroughPlugin) {
    const std::string expected_text = "hello from injected transcriber";

    // Build a WhisperStubTranscriber with a TranscribeFn injected.
    auto stub = std::make_unique<WhisperStubTranscriber>();
    WhisperConfig cfg;
    cfg.model_path = "/tmp/stub_model";
    stub->initialize(cfg);
    stub->setTranscribeFn(
        [&](const std::vector<float>&, float) -> TranscriptionResult {
            TranscriptionResult r;
            r.text       = expected_text;
            r.success    = true;
            r.confidence = 0.85f;
            return r;
        });

    // Inject into WhisperPlugin via the injection constructor.
    auto reader = std::make_unique<WavAudioChunkReader>();
    WhisperPlugin plugin(std::move(stub), std::move(reader));

    // Initialize the plugin (model path arbitrary for stub path).
    const bool ok = plugin.initialize("/tmp/stub_model", {});
    ASSERT_TRUE(ok) << "Plugin should initialize with stub transcriber";

    const std::vector<float> pcm(2048, 0.0f);
    const auto result = plugin.transcribe(pcm, 16000.0f);

    EXPECT_EQ(result.text, expected_text)
        << "TranscribeFn result should propagate through WhisperPlugin";
    EXPECT_EQ(result.ingestion_source_type, "WHISPER");
}
