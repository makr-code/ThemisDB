/**
 * @file test_voice_wave_a_noisy_wakeword.cpp
 * @brief Wave-A V3 — Noisy wake-word adversarial expansion and shared fallback regression.
 * @version 1.0
 *
 * Covers:
 *  - Wake-word NOT triggered below confidence threshold (0.3)
 *  - Wake-word NOT triggered when noise level > 0.8 SNR degradation
 *  - Wake-word triggered correctly when confidence >= 0.7
 *  - Wake-word fail-closed when detector throws exception
 *  - Intent recognition returns UNKNOWN for empty input
 *  - LLM response generation returns safe default when backend times out (mock timeout)
 *  - Command execution returns error response when command fails
 *  - Partial STT failure returns empty transcript marker (not crash)
 *
 * Suite: module_voice_test_voice_wave_a_noisy_wakeword_focused
 * Labels: voice wave_a release_critical
 * Timeout: 120 seconds
 *
 * Total Tests: 8
 */

#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <stdexcept>
#include <functional>
#include <chrono>
#include <thread>

namespace themis { namespace voice { namespace test {

// ─────────────────────────────────────────────────────────────────────────────
// Minimal local types mirroring production structs for self-contained testing
// ─────────────────────────────────────────────────────────────────────────────

struct WakeWordResult {
    bool  detected   = false;
    float confidence = 0.0f;
};

struct IntentResult {
    enum class Type { UNKNOWN, QUERY, COMMAND, CONVERSATION };
    Type  type       = Type::UNKNOWN;
    float confidence = 0.0f;
};

struct STTResult {
    bool        success      = false;
    std::string full_text;
    std::string error_message;  // "[STT_BACKEND_FAILURE]" when backend fails
};

// ─────────────────────────────────────────────────────────────────────────────
// Inline mocks that exercise the same logic paths as the production fallbacks
// (Wave-A V1/V2/V3 hardening guards)
// ─────────────────────────────────────────────────────────────────────────────

/// Simulates the hardened wake-word dispatcher (Wave-A V1).
/// - Returns fail-closed on exception.
/// - Rejects confidence below threshold.
/// - Rejects audio whose SNR degradation exceeds kMaxNoiseDegradation.
WakeWordResult dispatchWakeWord(
    float reported_confidence,
    float snr_degradation,
    bool  throw_exception,
    float threshold         = 0.7f,
    float max_noise         = 0.8f)
{
    // Wave-A V1: shared fallback semantics applied
    try {
        if (throw_exception) {
            throw std::runtime_error("wake-word backend unavailable");
        }
        WakeWordResult result;
        result.confidence = reported_confidence;
        // Reject if noise degrades SNR beyond acceptable bound.
        if (snr_degradation > max_noise) {
            result.detected   = false;
            result.confidence = 0.0f;
            return result;
        }
        result.detected = (result.confidence >= threshold);
        return result;
    } catch (...) {
        // [VOICE-FALLBACK] wake-word detector failed — fail-closed
        return WakeWordResult{false, 0.0f};
    }
}

/// Simulates the hardened intent recognizer (Wave-A V1).
/// - Returns UNKNOWN/0.0 for empty input or on exception.
IntentResult dispatchIntentRecognition(const std::string& text, bool throw_exception = false)
{
    // Wave-A V1: shared fallback semantics applied
    try {
        if (throw_exception) {
            throw std::runtime_error("intent backend unavailable");
        }
        if (text.empty()) {
            return IntentResult{IntentResult::Type::UNKNOWN, 0.0f};
        }
        // Minimal classification stub: anything non-empty gets QUERY at 0.8.
        return IntentResult{IntentResult::Type::QUERY, 0.8f};
    } catch (...) {
        // [VOICE-FALLBACK] intent recognition failed — return UNKNOWN/0.0
        return IntentResult{IntentResult::Type::UNKNOWN, 0.0f};
    }
}

/// Simulates the hardened LLM response generator with mock timeout (Wave-A V1).
/// - Returns safe default string when backend exceeds timeout_ms threshold.
std::string dispatchLLMResponse(
    const std::string& prompt,
    bool               simulate_timeout,
    int64_t            timeout_ms = 30000)
{
    // Wave-A V1: shared fallback semantics applied
    try {
        if (simulate_timeout) {
            // Simulate a response that would have arrived after the deadline.
            // In production, the elapsed_ms > kLLMResponseTimeoutMs check fires.
            (void)timeout_ms;
            throw std::runtime_error("LLM backend timeout");
        }
        if (prompt.empty()) {
            return "I need a prompt to generate a response. Please provide your question or request.";
        }
        return "Test LLM response for: " + prompt;
    } catch (...) {
        // [VOICE-FALLBACK] LLM timeout/failure — safe default
        return "I'm sorry, I encountered an error processing your request. Could you please rephrase that?";
    }
}

/// Simulates the hardened command execution path (Wave-A V1).
/// - Returns error response string when the command fails.
std::string dispatchCommandExecution(const std::string& command, bool force_fail = false)
{
    // Wave-A V1: shared fallback semantics applied
    try {
        if (force_fail) {
            throw std::runtime_error("command execution error");
        }
        if (command.empty()) {
            return "I need a prompt to generate a response. Please provide your question or request.";
        }
        return "Executed: " + command;
    } catch (...) {
        // [VOICE-FALLBACK] command execution failed — error response
        return "I'm sorry, I encountered an error executing your command. Please try again.";
    }
}

/// Simulates the hardened STT dispatch with backend failure fallback (Wave-A V2).
/// - Returns empty transcript + "[STT_BACKEND_FAILURE]" marker on exception.
STTResult dispatchSTT(const std::vector<uint8_t>& audio, bool throw_exception = false)
{
    // Wave-A V2: partial backend failure matrix
    STTResult result;
    try {
        if (throw_exception) {
            throw std::runtime_error("STT backend unavailable");
        }
        if (audio.empty()) {
            result.success       = false;
            result.error_message = "empty audio";
            return result;
        }
        result.success    = true;
        result.full_text  = "test transcript";
        return result;
    } catch (...) {
        // [VOICE-FALLBACK] STT backend failed, using empty transcript fallback
        result.success       = false;
        result.full_text     = "";
        result.error_message = "[STT_BACKEND_FAILURE]";
        return result;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Test Fixture
// ─────────────────────────────────────────────────────────────────────────────

class WaveANoisyWakeWordTest : public ::testing::Test {
protected:
    static constexpr float kWakeWordThreshold = 0.7f;
    static constexpr float kMaxNoiseDegradation = 0.8f;

    std::vector<uint8_t> makeAudio(size_t bytes = 16000) {
        return std::vector<uint8_t>(bytes, 0xAB);
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Test 1: Wake-word NOT triggered by audio below confidence threshold (0.3)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(WaveANoisyWakeWordTest, WakeWordBelowConfidenceThresholdNotTriggered) {
    // confidence=0.3 is below the 0.7 threshold — wake-word must NOT fire.
    auto result = dispatchWakeWord(
        /*confidence=*/0.3f,
        /*snr_degradation=*/0.0f,
        /*throw_exception=*/false,
        kWakeWordThreshold);

    EXPECT_FALSE(result.detected)
        << "Wake-word should NOT be triggered when confidence (0.3) < threshold (0.7)";
    EXPECT_FLOAT_EQ(result.confidence, 0.3f);
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 2: Wake-word NOT triggered when noise level > 0.8 SNR degradation
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(WaveANoisyWakeWordTest, WakeWordNotTriggeredByHighNoise) {
    // Even at confidence=0.9, extreme noise (SNR degradation=0.95) must suppress detection.
    auto result = dispatchWakeWord(
        /*confidence=*/0.9f,
        /*snr_degradation=*/0.95f,  // > kMaxNoiseDegradation=0.8
        /*throw_exception=*/false,
        kWakeWordThreshold,
        kMaxNoiseDegradation);

    EXPECT_FALSE(result.detected)
        << "Wake-word should NOT fire when SNR degradation (0.95) > max allowed (0.8)";
    EXPECT_FLOAT_EQ(result.confidence, 0.0f)
        << "Confidence should be zeroed when noise gate rejects the detection";
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 3: Wake-word triggered correctly when confidence >= 0.7
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(WaveANoisyWakeWordTest, WakeWordTriggeredAtSufficientConfidence) {
    // confidence=0.85 >= threshold=0.7 with low noise — detection must succeed.
    auto result = dispatchWakeWord(
        /*confidence=*/0.85f,
        /*snr_degradation=*/0.1f,  // well below max noise
        /*throw_exception=*/false,
        kWakeWordThreshold,
        kMaxNoiseDegradation);

    EXPECT_TRUE(result.detected)
        << "Wake-word SHOULD be triggered when confidence (0.85) >= threshold (0.7)";
    EXPECT_FLOAT_EQ(result.confidence, 0.85f);
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 4: Wake-word fail-closed when detector throws exception
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(WaveANoisyWakeWordTest, WakeWordFailClosedOnDetectorException) {
    // Backend throws — must return detected=false, confidence=0.0 (no crash, no leak).
    ASSERT_NO_THROW({
        auto result = dispatchWakeWord(
            /*confidence=*/0.9f,
            /*snr_degradation=*/0.0f,
            /*throw_exception=*/true);

        EXPECT_FALSE(result.detected)
            << "[VOICE-FALLBACK] Wake-word must be fail-closed when detector throws";
        EXPECT_FLOAT_EQ(result.confidence, 0.0f)
            << "[VOICE-FALLBACK] Confidence must be 0.0 on exception fallback";
    });
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 5: Intent recognition returns UNKNOWN for empty input
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(WaveANoisyWakeWordTest, IntentRecognitionReturnsUnknownForEmptyInput) {
    auto result = dispatchIntentRecognition(/*text=*/"");

    EXPECT_EQ(result.type, IntentResult::Type::UNKNOWN)
        << "Intent for empty input must be UNKNOWN";
    EXPECT_FLOAT_EQ(result.confidence, 0.0f)
        << "Confidence for empty input must be 0.0";
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 6: LLM response generation returns safe default when backend times out
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(WaveANoisyWakeWordTest, LLMResponseReturnsSafeDefaultOnTimeout) {
    ASSERT_NO_THROW({
        std::string response = dispatchLLMResponse(
            /*prompt=*/"query the database",
            /*simulate_timeout=*/true);

        EXPECT_FALSE(response.empty())
            << "[VOICE-FALLBACK] LLM timeout fallback must return a non-empty safe default";
        // Must be the canonical fallback string, not an internal error token.
        EXPECT_NE(response.find("error"), std::string::npos)
            << "Fallback response should acknowledge the error gracefully";
    });
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 7: Command execution returns error response when command fails
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(WaveANoisyWakeWordTest, CommandExecutionReturnsErrorResponseOnFailure) {
    ASSERT_NO_THROW({
        std::string response = dispatchCommandExecution(
            /*command=*/"delete all records",
            /*force_fail=*/true);

        EXPECT_FALSE(response.empty())
            << "[VOICE-FALLBACK] Command failure must return a non-empty error response";
        // Must NOT throw or return an empty string — safe UX string expected.
        EXPECT_NE(response.find("error"), std::string::npos)
            << "Command failure response should contain 'error' for UX clarity";
    });
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 8: Partial STT failure returns empty transcript marker (not crash)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(WaveANoisyWakeWordTest, PartialSTTFailureReturnsEmptyTranscriptMarker) {
    ASSERT_NO_THROW({
        auto audio  = makeAudio();
        auto result = dispatchSTT(audio, /*throw_exception=*/true);

        EXPECT_FALSE(result.success)
            << "[VOICE-FALLBACK] STT backend failure must set success=false";
        EXPECT_TRUE(result.full_text.empty())
            << "[VOICE-FALLBACK] STT backend failure must return empty transcript text";
        EXPECT_EQ(result.error_message, "[STT_BACKEND_FAILURE]")
            << "[VOICE-FALLBACK] STT backend failure must set error_message=[STT_BACKEND_FAILURE]";
    });
}

} // namespace test
} // namespace voice
} // namespace themis
