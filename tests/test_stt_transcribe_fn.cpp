/*
 * Tests for STTProcessor::setTranscribeFn() injection API (Stub #184).
 *
 * These tests run in builds WITHOUT THEMIS_ENABLE_WHISPER (the default CI
 * build) and verify that:
 *   STT-TFN-01  No fn set          → returns the built-in notice string.
 *   STT-TFN-02  Fn set             → delegates to fn; real text returned.
 *   STT-TFN-03  Fn cleared (null)  → reverts to built-in notice string.
 */

#include <gtest/gtest.h>
#include "content/stt_processor.h"

using namespace themis::content;

namespace {

// Minimal 16-kHz PCM chunk (1 s of silence, as the stub does not decode it).
std::vector<uint8_t> silentWav() {
    // 16-kHz mono 16-bit: 16000 samples × 2 bytes = 32000 bytes of zeros.
    return std::vector<uint8_t>(32000, 0u);
}

} // namespace

// ────────────────────────────────────────────────────────────────────────────
// STT-TFN-01 — no injection → built-in notice string
// ────────────────────────────────────────────────────────────────────────────
TEST(STTTranscribeFnTest, NoInjectionReturnsNoticeStub) {
#ifdef THEMIS_ENABLE_WHISPER
    GTEST_SKIP() << "THEMIS_ENABLE_WHISPER is ON — stub path is dead; skip.";
#endif

    STTProcessor proc;
    nlohmann::json cfg;
    cfg["model_path"] = "does_not_exist.bin";
    proc.initialize({});

    auto result = proc.transcribe(silentWav());

    // The built-in notice string must appear somewhere in the output.
    EXPECT_TRUE(result.success);
    EXPECT_NE(result.full_text.find("THEMIS_ENABLE_WHISPER"), std::string::npos)
        << "Expected notice-string stub text, got: " << result.full_text;
    EXPECT_FLOAT_EQ(result.average_confidence, 0.0f);
}

// ────────────────────────────────────────────────────────────────────────────
// STT-TFN-02 — injected fn → real text returned
// ────────────────────────────────────────────────────────────────────────────
TEST(STTTranscribeFnTest, InjectedFnIsUsedAndReturnsRealText) {
#ifdef THEMIS_ENABLE_WHISPER
    GTEST_SKIP() << "THEMIS_ENABLE_WHISPER is ON — stub path is dead; skip.";
#endif

    STTProcessor proc;
    proc.initialize({});

    const std::string expected_text = "Hello from injected transcriber";

    proc.setTranscribeFn([&](const std::vector<float>& /*pcm*/,
                             const nlohmann::json& /*opts*/) -> TranscriptionResult {
        TranscriptionResult r;
        r.success             = true;
        r.full_text           = expected_text;
        r.detected_language   = "en";
        r.average_confidence  = 0.95f;
        r.audio_duration_ms   = 1000;
        TranscriptionSegment seg;
        seg.text       = expected_text;
        seg.start_ms   = 0;
        seg.end_ms     = 1000;
        seg.confidence = 0.95f;
        r.segments.push_back(seg);
        return r;
    });

    auto result = proc.transcribe(silentWav());

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.full_text, expected_text);
    EXPECT_FLOAT_EQ(result.average_confidence, 0.95f);
    ASSERT_FALSE(result.segments.empty());
    EXPECT_EQ(result.segments.front().text, expected_text);
}

// ────────────────────────────────────────────────────────────────────────────
// STT-TFN-03 — clear fn → reverts to notice stub
// ────────────────────────────────────────────────────────────────────────────
TEST(STTTranscribeFnTest, ClearFnRevertsToNoticeStub) {
#ifdef THEMIS_ENABLE_WHISPER
    GTEST_SKIP() << "THEMIS_ENABLE_WHISPER is ON — stub path is dead; skip.";
#endif

    STTProcessor proc;
    proc.initialize({});

    // Inject, verify it works.
    proc.setTranscribeFn([](const std::vector<float>&,
                            const nlohmann::json&) -> TranscriptionResult {
        TranscriptionResult r;
        r.success   = true;
        r.full_text = "injected";
        return r;
    });
    {
        auto r = proc.transcribe(silentWav());
        EXPECT_EQ(r.full_text, "injected");
    }

    // Clear the fn.
    proc.setTranscribeFn(nullptr);

    auto result = proc.transcribe(silentWav());
    EXPECT_TRUE(result.success);
    EXPECT_NE(result.full_text.find("THEMIS_ENABLE_WHISPER"), std::string::npos)
        << "Expected notice-string after clearing fn, got: " << result.full_text;
}
