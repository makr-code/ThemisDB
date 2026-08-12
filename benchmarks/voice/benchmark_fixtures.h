/**
 * @file benchmark_fixtures.h
 * @brief Common fixtures, mocks, and utilities for Voice Module Phase 5 benchmarks.
 *
 * Provides:
 * - Deterministic RNG seed (kCanonicalRngSeed=42)
 * - Mock implementations of STT, TTS, LLM, streaming
 * - Shared benchmark fixtures
 * - Utility functions for audio/timing generation
 * - SLA gate definitions
 */

#pragma once

#include <benchmark/benchmark.h>
#include <chrono>
#include <cstring>
#include <random>
#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <cmath>

// =============================================================================
// Constants
// =============================================================================

namespace themis::voice::benchmark {

// RNG seed (per MEASUREMENT_HYGIENE.md)
static constexpr uint64_t kCanonicalRngSeed = 42;

// Timing constants
static constexpr int64_t kMicrosecondToNanosecond = 1000;
static constexpr int64_t kMillisecondToMicrosecond = 1000;
static constexpr int64_t kSecondToMillisecond = 1000;

// Audio format constants
static constexpr int kAudioSampleRate = 16000;       // 16 kHz
static constexpr int kAudioChannels = 1;             // mono
static constexpr int kAudioBitsPerSample = 16;       // 16-bit PCM
static constexpr int kAudioBytesPerSecond = kAudioSampleRate * kAudioChannels * (kAudioBitsPerSample / 8);

// Default mock latencies (deterministic)
static constexpr int64_t kMockSTTLatencyMicroseconds = 500'000;     // 500ms base
static constexpr int64_t kMockTTSLatencyMicroseconds = 300'000;     // 300ms base
static constexpr int64_t kMockLLMLatencyMicroseconds = 1'500'000;   // 1.5s base
static constexpr int64_t kMockStreamChunkLatencyMicroseconds = 10'000; // 10ms per chunk

// Session and memory constants
static constexpr int64_t kSessionMemoryBytesBase = 5'000'000;       // 5MB per session
static constexpr int64_t kStreamBufferBytesBase = 10'000'000;       // 10MB per stream
static constexpr int64_t kChunksPerSecondTarget = 1000;             // 1000 chunks/sec target

// SLA Gate Thresholds (nanoseconds for consistency)
namespace gates {
    // STT latencies (Task 5.1)
    static constexpr int64_t kGateSTTLatencyP95_100ms = 2'000'000'000;      // 2000ms
    static constexpr int64_t kGateSTTLatencyP99_100ms = 5'000'000'000;      // 5000ms
    static constexpr int64_t kGateSTTLatencyP95_5s = 10'000'000'000;        // 10000ms
    static constexpr int64_t kGateSTTLatencyP99_5s = 15'000'000'000;        // 15000ms
    static constexpr int64_t kGateSTTLatencyP95_60s = 60'000'000'000;       // 60000ms
    static constexpr int64_t kGateSTTLatencyP99_60s = 120'000'000'000;      // 120000ms

    // TTS latencies (Task 5.1)
    static constexpr int64_t kGateTTSLatencyP95_ShortText = 1'000'000'000;  // 1000ms
    static constexpr int64_t kGateTTSLatencyP99_ShortText = 2'000'000'000;  // 2000ms
    static constexpr int64_t kGateTTSLatencyP95_MediumText = 3'000'000'000; // 3000ms
    static constexpr int64_t kGateTTSLatencyP99_MediumText = 5'000'000'000; // 5000ms

    // Streaming (Task 5.2)
    static constexpr int64_t kGateStreamThroughput = 1'000;                  // chunks/sec
    static constexpr int64_t kGateStreamLatencyP95 = 50'000'000;             // 50ms per chunk
    static constexpr int64_t kGateStreamMemory = 10'000'000;                 // 10MB per session
    static constexpr int64_t kGateStreamRecovery = 500'000'000;              // 500ms recovery

    // Session lifecycle (Task 5.3)
    static constexpr int64_t kGateSessionCreate = 100'000'000;               // 100ms
    static constexpr int64_t kGateSessionDelete = 50'000'000;                // 50ms
    static constexpr int64_t kGateSessionMemory100 = 500'000'000;            // 500MB
    static constexpr int64_t kGateSessionMemory1000 = 2'000'000'000;         // 2GB
    static constexpr int64_t kGateSessionCleanupLeak = 10'000'000;           // 10MB

    // Audio preprocessing (Task 5.4)
    static constexpr int64_t kGateAudioValidateSmall = 10'000'000;           // 10ms
    static constexpr int64_t kGateAudioValidateLarge = 50'000'000;           // 50ms
    static constexpr int64_t kGateAudioPreprocess = 500'000'000;             // 500ms per 5s
    static constexpr int64_t kGateWakeWordLatency = 1'000'000'000;           // 1000ms
    static constexpr int64_t kGateIntentLatency = 500'000'000;               // 500ms

    // Assistant (Task 5.5)
    static constexpr int64_t kGateLLMLatencyP95 = 3'000'000'000;             // 3000ms
    static constexpr int64_t kGateLLMLatencyP99 = 10'000'000'000;            // 10000ms
    static constexpr int64_t kGateTTSSynthesis = 2'000'000'000;              // 2000ms
    static constexpr int64_t kGateE2ELatencyP95 = 6'000'000'000;             // 6000ms
    static constexpr int64_t kGateCommandParse = 100'000'000;                // 100ms
}

// =============================================================================
// Mock Implementations
// =============================================================================

/**
 * Mock STT processor with deterministic latency based on audio size.
 */
class MockSTTProcessor {
public:
    explicit MockSTTProcessor(int64_t base_latency_us = kMockSTTLatencyMicroseconds)
        : base_latency_us_(base_latency_us) {}

    // Simulate STT processing with deterministic latency
    std::string transcribe(const std::vector<uint8_t>& audio) {
        // Simulate processing time proportional to audio size
        int64_t duration_us = base_latency_us_ + (audio.size() / 1000);  // add 1us per KB
        std::this_thread::sleep_for(std::chrono::microseconds(duration_us));
        return "mock transcription";
    }

    // Simulate streaming chunk processing
    std::string processChunk(const std::vector<uint8_t>& chunk) {
        std::this_thread::sleep_for(std::chrono::microseconds(kMockStreamChunkLatencyMicroseconds));
        return "chunk";
    }

private:
    int64_t base_latency_us_;
};

/**
 * Mock TTS processor with deterministic latency based on text length.
 */
class MockTTSProcessor {
public:
    explicit MockTTSProcessor(int64_t base_latency_us = kMockTTSLatencyMicroseconds)
        : base_latency_us_(base_latency_us) {}

    // Simulate TTS synthesis with deterministic latency
    std::vector<uint8_t> synthesize(const std::string& text) {
        // Simulate processing time proportional to text length
        int64_t duration_us = base_latency_us_ + (text.length() * 100);  // add 100us per character
        std::this_thread::sleep_for(std::chrono::microseconds(duration_us));
        
        // Return mock audio buffer
        return std::vector<uint8_t>(text.length() * 1000, 0);
    }

private:
    int64_t base_latency_us_;
};

/**
 * Mock LLM processor with deterministic latency.
 */
class MockLLMProcessor {
public:
    explicit MockLLMProcessor(int64_t base_latency_us = kMockLLMLatencyMicroseconds)
        : base_latency_us_(base_latency_us) {}

    // Simulate LLM response generation
    std::string generate(const std::string& prompt) {
        // Simulate processing time (constant + prompt-dependent)
        int64_t duration_us = base_latency_us_ + (prompt.length() * 10);
        std::this_thread::sleep_for(std::chrono::microseconds(duration_us));
        return "mock response";
    }

private:
    int64_t base_latency_us_;
};

/**
 * Mock streaming buffer for testing throughput and latency.
 */
class MockStreamBuffer {
public:
    MockStreamBuffer(size_t max_size_bytes = kStreamBufferBytesBase)
        : max_size_(max_size_bytes), current_size_(0) {}

    // Add chunk to buffer (simulates network arrival)
    bool addChunk(const std::vector<uint8_t>& chunk) {
        std::unique_lock<std::mutex> lock(mutex_);
        if (current_size_ + chunk.size() > max_size_) {
            return false;  // buffer full
        }
        current_size_ += chunk.size();
        queue_.push(chunk);
        cv_.notify_one();
        return true;
    }

    // Get chunk from buffer (simulates processing)
    bool getChunk(std::vector<uint8_t>& chunk) {
        std::unique_lock<std::mutex> lock(mutex_);
        if (queue_.empty()) {
            return false;
        }
        chunk = queue_.front();
        queue_.pop();
        current_size_ = (current_size_ >= chunk.size()) ? current_size_ - chunk.size() : 0;
        return true;
    }

    size_t getMemoryUsage() const {
        std::unique_lock<std::mutex> lock(mutex_);
        return current_size_;
    }

    void clear() {
        std::unique_lock<std::mutex> lock(mutex_);
        while (!queue_.empty()) queue_.pop();
        current_size_ = 0;
    }

private:
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::queue<std::vector<uint8_t>> queue_;
    size_t max_size_;
    size_t current_size_;
};

// =============================================================================
// Fixture Classes
// =============================================================================

/**
 * Base fixture for all voice benchmarks.
 * Provides common setup/teardown and utilities.
 */
class VoiceBenchmarkFixture : public benchmark::Fixture {
public:
    void SetUp(const benchmark::State& /*unused*/) override {
        // Initialize deterministic RNG
        rng_.seed(kCanonicalRngSeed);
        
        // Create mock processors
        stt_processor_ = std::make_unique<MockSTTProcessor>();
        tts_processor_ = std::make_unique<MockTTSProcessor>();
        llm_processor_ = std::make_unique<MockLLMProcessor>();
        stream_buffer_ = std::make_unique<MockStreamBuffer>();
    }

    void TearDown(const benchmark::State& /*unused*/) override {
        stt_processor_.reset();
        tts_processor_.reset();
        llm_processor_.reset();
        stream_buffer_.reset();
    }

protected:
    std::mt19937 rng_;
    std::unique_ptr<MockSTTProcessor> stt_processor_;
    std::unique_ptr<MockTTSProcessor> tts_processor_;
    std::unique_ptr<MockLLMProcessor> llm_processor_;
    std::unique_ptr<MockStreamBuffer> stream_buffer_;

    // Utility: generate random audio buffer
    std::vector<uint8_t> generateAudio(size_t duration_ms) {
        size_t num_samples = (duration_ms * kAudioSampleRate) / 1000;
        size_t num_bytes = num_samples * kAudioChannels * (kAudioBitsPerSample / 8);
        
        std::vector<uint8_t> audio(num_bytes);
        std::uniform_int_distribution<> dis(0, 255);
        for (auto& byte : audio) {
            byte = static_cast<uint8_t>(dis(rng_));
        }
        return audio;
    }

    // Utility: generate session ID
    std::string generateSessionId() {
        static std::atomic<int> counter(0);
        return "session-" + std::to_string(counter++);
    }

    // Utility: sleep for specified microseconds
    void sleepMicroseconds(int64_t us) {
        std::this_thread::sleep_for(std::chrono::microseconds(us));
    }
};

/**
 * Fixture for STT/TTS latency benchmarks (Task 5.1).
 */
class STTTTSLatencyFixture : public VoiceBenchmarkFixture {
public:
    void SetUp(const benchmark::State& state) override {
        VoiceBenchmarkFixture::SetUp(state);
        // Additional STT/TTS setup if needed
    }
};

/**
 * Fixture for streaming throughput benchmarks (Task 5.2).
 */
class StreamingThroughputFixture : public VoiceBenchmarkFixture {
public:
    void SetUp(const benchmark::State& state) override {
        VoiceBenchmarkFixture::SetUp(state);
        stream_buffer_->clear();
    }

    void TearDown(const benchmark::State& state) override {
        stream_buffer_->clear();
        VoiceBenchmarkFixture::TearDown(state);
    }
};

/**
 * Fixture for session lifecycle benchmarks (Task 5.3).
 */
class SessionLifecycleFixture : public VoiceBenchmarkFixture {
public:
    void SetUp(const benchmark::State& state) override {
        VoiceBenchmarkFixture::SetUp(state);
        sessions_.clear();
    }

    void TearDown(const benchmark::State& state) override {
        sessions_.clear();
        VoiceBenchmarkFixture::TearDown(state);
    }

protected:
    std::vector<std::string> sessions_;
};

/**
 * Fixture for audio preprocessing benchmarks (Task 5.4).
 */
class AudioPreprocessingFixture : public VoiceBenchmarkFixture {
public:
    void SetUp(const benchmark::State& state) override {
        VoiceBenchmarkFixture::SetUp(state);
    }
};

/**
 * Fixture for assistant latency benchmarks (Task 5.5).
 */
class AssistantLatencyFixture : public VoiceBenchmarkFixture {
public:
    void SetUp(const benchmark::State& state) override {
        VoiceBenchmarkFixture::SetUp(state);
    }
};

// =============================================================================
// Utility Functions
// =============================================================================

namespace utils {

    /**
     * Calculate p95 from a sorted vector of latencies (in nanoseconds).
     */
    inline int64_t calculateP95(const std::vector<int64_t>& latencies) {
        if (latencies.empty()) return 0;
        size_t idx = static_cast<size_t>(latencies.size() * 0.95);
        if (idx >= latencies.size()) idx = latencies.size() - 1;
        return latencies[idx];
    }

    /**
     * Calculate p99 from a sorted vector of latencies (in nanoseconds).
     */
    inline int64_t calculateP99(const std::vector<int64_t>& latencies) {
        if (latencies.empty()) return 0;
        size_t idx = static_cast<size_t>(latencies.size() * 0.99);
        if (idx >= latencies.size()) idx = latencies.size() - 1;
        return latencies[idx];
    }

    /**
     * Check if a latency gate passes.
     * @return true if current <= threshold, false otherwise
     */
    inline bool checkGate(int64_t current_ns, int64_t threshold_ns, const char* gate_name) {
        bool passes = current_ns <= threshold_ns;
        if (!passes) {
            std::fprintf(stderr, "GATE FAILED: %s (%.2fms > %.2fms)\n",
                gate_name, current_ns / 1'000'000.0, threshold_ns / 1'000'000.0);
        }
        return passes;
    }

}  // namespace utils

}  // namespace themis::voice::benchmark
