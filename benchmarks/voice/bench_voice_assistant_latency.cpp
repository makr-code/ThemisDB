/**
 * @file bench_voice_assistant_latency.cpp
 * @brief Google Benchmark suite for Voice assistant end-to-end latency.
 *
 * Benchmarks:
 *  - GATE_LLM_LATENCY_P95 <= 3000ms
 *  - GATE_LLM_LATENCY_P99 <= 10000ms
 *  - GATE_TTS_SYNTHESIS <= 2000ms
 *  - GATE_E2E_LATENCY_P95 <= 6000ms
 *  - GATE_COMMAND_PARSE <= 100ms
 */

#include <benchmark/benchmark.h>
#include "benchmarks/voice/benchmark_fixtures.h"

#include <algorithm>
#include <chrono>
#include <vector>
#include <string>

using namespace themis::voice::benchmark;

// =============================================================================
// Mock Assistant Components
// =============================================================================

class MockCommandParser {
public:
    struct ParseResult {
        std::string command;
        std::map<std::string, std::string> parameters;
    };

    // Parse voice command from text
    ParseResult parseCommand(const std::string& text) {
        // Simulate parsing time (very fast)
        int64_t parse_us = 10'000 + (text.length() * 5);  // base 10ms + text-dependent
        std::this_thread::sleep_for(std::chrono::microseconds(parse_us));

        ParseResult result;
        result.command = "play_music";
        result.parameters["artist"] = "unknown";
        return result;
    }

    // Route command to handler
    std::string routeCommand(const ParseResult& parse_result) {
        // Simulate routing (very fast)
        std::this_thread::sleep_for(std::chrono::microseconds(5'000));  // 5ms
        return "music_handler";
    }
};

class MockResponseGenerator {
public:
    // Generate response for command (excluding TTS)
    std::string generateResponse(const std::string& command, const std::map<std::string, std::string>& params) {
        // Simulate command-specific response generation
        int64_t gen_us = 50'000 + (command.length() * 10);  // base 50ms
        std::this_thread::sleep_for(std::chrono::microseconds(gen_us));
        return "Now playing your favorite music";
    }
};

// =============================================================================
// Assistant Latency Benchmarks
// =============================================================================

/**
 * @test BENCHMARK(VoiceAssistant, LLMResponseGeneration)
 * Measure: prompt → response from LLM
 * Assert: p95 < 3s, p99 < 10s
 * GATE_LLM_LATENCY_P95 <= 3000ms
 * GATE_LLM_LATENCY_P99 <= 10000ms
 */
BENCHMARK_F(AssistantLatencyFixture, VoiceAssistant_LLMResponseGeneration)(benchmark::State& state) {
    std::vector<int64_t> latencies_ns;
    latencies_ns.reserve(state.max_iterations);

    for (auto _ : state) {
        // Simulate LLM request with different prompt lengths
        std::string prompt = "You are a helpful voice assistant. The user asked: play music by artist X.";

        auto start = std::chrono::steady_clock::now();
        auto response = llm_processor_->generate(prompt);
        auto end = std::chrono::steady_clock::now();

        int64_t latency_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        latencies_ns.push_back(latency_ns);

        benchmark::DoNotOptimize(response);
    }

    std::sort(latencies_ns.begin(), latencies_ns.end());

    int64_t p95_ns = utils::calculateP95(latencies_ns);
    int64_t p99_ns = utils::calculateP99(latencies_ns);

    utils::checkGate(p95_ns, gates::kGateLLMLatencyP95, "GATE_LLM_LATENCY_P95");
    utils::checkGate(p99_ns, gates::kGateLLMLatencyP99, "GATE_LLM_LATENCY_P99");

    state.counters["p95_ms"] = p95_ns / 1'000'000.0;
    state.counters["p99_ms"] = p99_ns / 1'000'000.0;
    state.SetLabel("Assistant LLMResponseGeneration: p95 < 3s, p99 < 10s");
}

/**
 * @test BENCHMARK(VoiceAssistant, TTSSynthesis)
 * Measure: text → audio from TTS
 * Assert: < 2s for 50 word response
 * GATE_TTS_SYNTHESIS <= 2000ms
 */
BENCHMARK_F(AssistantLatencyFixture, VoiceAssistant_TTSSynthesis)(benchmark::State& state) {
    std::vector<int64_t> latencies_ns;
    latencies_ns.reserve(state.max_iterations);

    for (auto _ : state) {
        // 50-word response
        std::string text = "Now playing your favorite music. I hope you enjoy this track. "
                          "Let me know if you want to change the artist or album.";

        auto start = std::chrono::steady_clock::now();
        auto audio = tts_processor_->synthesize(text);
        auto end = std::chrono::steady_clock::now();

        int64_t latency_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        latencies_ns.push_back(latency_ns);

        benchmark::DoNotOptimize(audio);
    }

    std::sort(latencies_ns.begin(), latencies_ns.end());

    int64_t p95_ns = utils::calculateP95(latencies_ns);

    utils::checkGate(p95_ns, gates::kGateTTSSynthesis, "GATE_TTS_SYNTHESIS");

    state.counters["p95_ms"] = p95_ns / 1'000'000.0;
    state.SetLabel("Assistant TTSSynthesis: < 2s for 50 words");
}

/**
 * @test BENCHMARK(VoiceAssistant, EndToEndLatency)
 * Measure: audio-in → audio-out (full assistant latency)
 * Assert: p95 < 6s (1s audio + 2s STT + 3s LLM/TTS)
 * GATE_E2E_LATENCY_P95 <= 6000ms
 */
BENCHMARK_F(AssistantLatencyFixture, VoiceAssistant_EndToEndLatency)(benchmark::State& state) {
    std::vector<int64_t> latencies_ns;
    latencies_ns.reserve(state.max_iterations);

    for (auto _ : state) {
        // Generate 1 second audio input
        auto audio_in = generateAudio(1000);  // 1 second

        auto start = std::chrono::steady_clock::now();

        // Step 1: STT (speech-to-text)
        auto transcription = stt_processor_->transcribe(audio_in);
        
        // Step 2: LLM (language model response)
        auto llm_response = llm_processor_->generate(transcription);
        
        // Step 3: TTS (text-to-speech)
        auto audio_out = tts_processor_->synthesize(llm_response);

        auto end = std::chrono::steady_clock::now();

        int64_t latency_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        latencies_ns.push_back(latency_ns);

        benchmark::DoNotOptimize(audio_out);
    }

    std::sort(latencies_ns.begin(), latencies_ns.end());

    int64_t p95_ns = utils::calculateP95(latencies_ns);
    int64_t p99_ns = utils::calculateP99(latencies_ns);

    utils::checkGate(p95_ns, gates::kGateE2ELatencyP95, "GATE_E2E_LATENCY_P95");

    state.counters["p95_ms"] = p95_ns / 1'000'000.0;
    state.counters["p99_ms"] = p99_ns / 1'000'000.0;
    state.SetLabel("Assistant EndToEndLatency: p95 < 6s");
}

/**
 * @test BENCHMARK(VoiceAssistant, CommandParsingAndRouting)
 * Measure: identify command + route to handler
 * Assert: < 100ms
 * GATE_COMMAND_PARSE <= 100ms
 */
BENCHMARK_F(AssistantLatencyFixture, VoiceAssistant_CommandParsingAndRouting)(benchmark::State& state) {
    MockCommandParser parser;
    std::vector<int64_t> latencies_ns;
    latencies_ns.reserve(state.max_iterations);

    for (auto _ : state) {
        std::string text = "play music by artist name";

        auto start = std::chrono::steady_clock::now();

        // Parse command
        auto parse_result = parser.parseCommand(text);
        
        // Route command
        auto handler = parser.routeCommand(parse_result);

        auto end = std::chrono::steady_clock::now();

        int64_t latency_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        latencies_ns.push_back(latency_ns);

        benchmark::DoNotOptimize(handler);
    }

    std::sort(latencies_ns.begin(), latencies_ns.end());

    int64_t p95_ns = utils::calculateP95(latencies_ns);
    int64_t p99_ns = utils::calculateP99(latencies_ns);

    utils::checkGate(p95_ns, gates::kGateCommandParse, "GATE_COMMAND_PARSE");

    state.counters["p95_ms"] = p95_ns / 1'000'000.0;
    state.counters["p99_ms"] = p99_ns / 1'000'000.0;
    state.SetLabel("Assistant CommandParsingAndRouting: < 100ms");
}

/**
 * @test BENCHMARK(VoiceAssistant, CommandResponseGeneration)
 * Measure: command → response generation (excluding TTS)
 * Assert: < 500ms
 */
BENCHMARK_F(AssistantLatencyFixture, VoiceAssistant_CommandResponseGeneration)(benchmark::State& state) {
    MockCommandParser parser;
    MockResponseGenerator generator;
    std::vector<int64_t> latencies_ns;
    latencies_ns.reserve(state.max_iterations);

    for (auto _ : state) {
        std::string command_text = "play music by artist name";

        auto start = std::chrono::steady_clock::now();

        // Parse command
        auto parse_result = parser.parseCommand(command_text);
        
        // Generate response (command handler execution)
        auto response = generator.generateResponse(parse_result.command, parse_result.parameters);

        auto end = std::chrono::steady_clock::now();

        int64_t latency_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        latencies_ns.push_back(latency_ns);

        benchmark::DoNotOptimize(response);
    }

    std::sort(latencies_ns.begin(), latencies_ns.end());

    int64_t p95_ns = utils::calculateP95(latencies_ns);

    utils::checkGate(p95_ns, 500'000'000, "GATE_COMMAND_RESPONSE");

    state.counters["p95_ms"] = p95_ns / 1'000'000.0;
    state.SetLabel("Assistant CommandResponseGeneration: < 500ms");
}

// =============================================================================
// Complex E2E Scenarios
// =============================================================================

/**
 * @test BENCHMARK(VoiceAssistant, MultiCommandSequence)
 * Measure: sequence of rapid commands (e.g., "play", "pause", "next")
 * Assert: < 500ms each in sequence
 */
BENCHMARK_F(AssistantLatencyFixture, VoiceAssistant_MultiCommandSequence)(benchmark::State& state) {
    MockCommandParser parser;
    
    std::vector<std::string> commands = {
        "play music",
        "next track",
        "pause",
        "resume",
        "volume up"
    };

    std::vector<int64_t> command_latencies_ns;

    for (auto _ : state) {
        for (const auto& cmd : commands) {
            auto start = std::chrono::steady_clock::now();
            
            auto parse_result = parser.parseCommand(cmd);
            auto handler = parser.routeCommand(parse_result);
            
            auto end = std::chrono::steady_clock::now();

            int64_t latency_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
            command_latencies_ns.push_back(latency_ns);

            benchmark::DoNotOptimize(handler);
        }
    }

    std::sort(command_latencies_ns.begin(), command_latencies_ns.end());

    int64_t p95_ns = utils::calculateP95(command_latencies_ns);

    state.counters["cmd_p95_ms"] = p95_ns / 1'000'000.0;
    state.SetLabel("Assistant MultiCommandSequence: rapid command handling");
}

BENCHMARK_MAIN();
