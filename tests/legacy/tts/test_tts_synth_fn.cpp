/*
 * Tests for TTSProcessor::setSynthFn() injection API (Stub #209).
 *
 * These tests run in builds WITHOUT THEMIS_ENABLE_PIPER_TTS (the default CI
 * build) and verify that:
 *   TTS-SFN-01  No fn set          → PCM portion of WAV output is all-zero
 *                                    (silence).
 *   TTS-SFN-02  Fn set             → delegates to fn; non-zero PCM in output.
 *   TTS-SFN-03  Fn cleared (null)  → reverts to silence stub.
 *
 * Note: synthesize() wraps PCM bytes in a 44-byte WAV header via
 * convertToFormat("wav").  Checks are performed on the PCM payload starting
 * at byte offset 44 of audio_data.
 */

#include <gtest/gtest.h>
#include "content/tts_processor.h"

using namespace themis::content;

namespace {

// Returns true when all PCM payload bytes (after 44-byte WAV header) are zero.
bool allSilence(const std::vector<uint8_t>& wav) {
    if (wav.size() <= 44u) {
      return true;
    }
    for (size_t i = 44u; i < wav.size(); ++i) {
        if (wav[i] != 0u) {
          return false;
        }
    }
    return true;
}

// Returns true when at least one PCM payload byte is non-zero.
bool hasNonZero(const std::vector<uint8_t>& wav) {
    return !allSilence(wav);
}

} // namespace

// ────────────────────────────────────────────────────────────────────────────
// TTS-SFN-01 — no injection → silence stub (all-zero PCM)
// ────────────────────────────────────────────────────────────────────────────
TEST(TTSSynthFnTest, NoInjectionReturnsSilence) {
#ifdef THEMIS_ENABLE_PIPER_TTS
    GTEST_SKIP() << "THEMIS_ENABLE_PIPER_TTS is ON — stub path is dead; skip.";
#endif

    TTSProcessor proc;
    proc.initialize({});

    TTSOptions opts;
    auto result = proc.synthesize("hello world", opts);

    EXPECT_TRUE(result.success);
    // The WAV header (44 bytes) is followed by silence (zeros).
    ASSERT_GT(result.audio_data.size(), 44u);
    EXPECT_TRUE(allSilence(result.audio_data))
        << "Expected all-zero PCM payload from silence stub";
}

// ────────────────────────────────────────────────────────────────────────────
// TTS-SFN-02 — injected fn → non-zero PCM bytes returned
// ────────────────────────────────────────────────────────────────────────────
TEST(TTSSynthFnTest, InjectedFnProducesNonZeroPcm) {
#ifdef THEMIS_ENABLE_PIPER_TTS
    GTEST_SKIP() << "THEMIS_ENABLE_PIPER_TTS is ON — stub path is dead; skip.";
#endif

    TTSProcessor proc;
    proc.initialize({});

    // Inject a fn that returns a distinct non-zero PCM buffer (>0 samples).
    proc.setSynthFn([](const std::string& text,
                       const TTSOptions& /*opts*/) -> std::vector<uint8_t> {
        // 200 bytes of non-zero 16-bit PCM (fake "audio" for the text length).
        return std::vector<uint8_t>(std::max<size_t>(200u, text.size() * 2u), 0x42u);
    });

    TTSOptions opts;
    auto result = proc.synthesize("hello world", opts);

    EXPECT_TRUE(result.success);
    ASSERT_GT(result.audio_data.size(), 44u);
    EXPECT_TRUE(hasNonZero(result.audio_data))
        << "Expected non-zero PCM bytes from injected fn";
}

// ────────────────────────────────────────────────────────────────────────────
// TTS-SFN-03 — clear fn → reverts to silence stub
// ────────────────────────────────────────────────────────────────────────────
TEST(TTSSynthFnTest, ClearFnRevertsToSilenceStub) {
#ifdef THEMIS_ENABLE_PIPER_TTS
    GTEST_SKIP() << "THEMIS_ENABLE_PIPER_TTS is ON — stub path is dead; skip.";
#endif

    TTSProcessor proc;
    proc.initialize({});

    // Inject a non-silence fn.
    proc.setSynthFn([](const std::string& /*text*/,
                       const TTSOptions& /*opts*/) -> std::vector<uint8_t> {
        return std::vector<uint8_t>(200u, 0xFF);
    });

    {
        TTSOptions opts;
        auto r = proc.synthesize("test", opts);
        EXPECT_TRUE(r.success);
        EXPECT_TRUE(hasNonZero(r.audio_data));
    }

    // Clear the fn.
    proc.setSynthFn(nullptr);

    TTSOptions opts;
    auto result = proc.synthesize("test", opts);
    EXPECT_TRUE(result.success);
    ASSERT_GT(result.audio_data.size(), 44u);
    EXPECT_TRUE(allSilence(result.audio_data))
        << "Expected silence after clearing synth fn";
}
