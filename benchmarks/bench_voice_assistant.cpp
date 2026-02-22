/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            bench_voice_assistant.cpp                          ║
  Version:         0.0.28                                             ║
  Last Modified:   2026-02-22 11:29:11                                ║
  Author:          copilot-swe-agent[bot]                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     460                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • c97d719  2026-02-22  Add parallel multi-source BFS/DFS implementation (graph/p... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file bench_voice_assistant.cpp
 * @brief Performance benchmarks for Voice Assistant module
 * 
 * Benchmarks voice command processing, audio conversion, transcription,
 * and storage operations.
 * 
 * @author ThemisDB Team
 * @date January 2025
 */

#include <benchmark/benchmark.h>
#include "voice/voice_assistant.h"
#include <nlohmann/json.hpp>
#include <vector>
#include <random>

using namespace themis::voice;
using json = nlohmann::json;

// ============================================================================
// Test Data Generation
// ============================================================================

std::vector<uint8_t> generateMockAudio(size_t size_bytes) {
    std::vector<uint8_t> audio(size_bytes);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    
    // Use std::generate for better performance
    std::generate(audio.begin(), audio.end(), [&]() { return static_cast<uint8_t>(dis(gen)); });
    return audio;
}

VoiceAssistant::Config createTestConfig() {
    VoiceAssistant::Config config;
    config.stt_model_path = "/tmp/test_stt_model";
    config.stt_model_size = "base";
    config.stt_language = "en";
    config.tts_model_path = "/tmp/test_tts_model";
    config.tts_voice = "default";
    config.tts_speed = 1.0f;
    config.llm_model_path = "/tmp/test_llm_model";
    config.llm_n_ctx = 2048;
    config.llm_n_gpu_layers = 0;
    config.storage_path = "/tmp/test_voice_storage";
    config.enable_revision_control = true;
    config.compress_audio = true;
    config.audio_format = "ogg";
    return config;
}

// ============================================================================
// Session Management Benchmarks
// ============================================================================

static void BM_SessionCreation(benchmark::State& state) {
    auto config = createTestConfig();
    VoiceAssistant assistant(config);
    
    int session_counter = 0;
    for (auto _ : state) {
        std::string session_id = "session-" + std::to_string(session_counter++);
        VoiceSession session = assistant.getSession(session_id);
        benchmark::DoNotOptimize(session);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_SessionCreation);

static void BM_SessionContextUpdate(benchmark::State& state) {
    auto config = createTestConfig();
    VoiceAssistant assistant(config);
    
    std::string session_id = "benchmark-session";
    json context;
    context["user_preference"] = "detailed";
    context["language"] = "en";
    context["history_limit"] = 100;
    
    for (auto _ : state) {
        assistant.updateSession(session_id, context);
        benchmark::DoNotOptimize(context);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_SessionContextUpdate);

static void BM_MultipleSessionsParallel(benchmark::State& state) {
    auto config = createTestConfig();
    VoiceAssistant assistant(config);
    
    const int num_sessions = state.range(0);
    std::vector<std::string> session_ids;
    for (int i = 0; i < num_sessions; i++) {
        session_ids.push_back("session-" + std::to_string(i));
    }
    
    int idx = 0;
    for (auto _ : state) {
        VoiceSession session = assistant.getSession(session_ids[idx % num_sessions]);
        benchmark::DoNotOptimize(session);
        idx++;
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_MultipleSessionsParallel)->Arg(10)->Arg(100)->Arg(1000);

// ============================================================================
// Voice Command Processing Benchmarks
// ============================================================================

static void BM_TextCommandProcessing(benchmark::State& state) {
    auto config = createTestConfig();
    VoiceAssistant assistant(config);
    
    std::string session_id = "bench-session";
    std::vector<std::string> commands = {
        "What is the weather today?",
        "Tell me about ThemisDB",
        "List all active sessions",
        "Show me the statistics"
    };
    
    int cmd_idx = 0;
    for (auto _ : state) {
        std::string response = assistant.processTextCommand(
            commands[cmd_idx % commands.size()],
            session_id
        );
        benchmark::DoNotOptimize(response);
        cmd_idx++;
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_TextCommandProcessing);

static void BM_VoiceCommandProcessing_SmallAudio(benchmark::State& state) {
    auto config = createTestConfig();
    VoiceAssistant assistant(config);
    
    std::string session_id = "bench-session";
    auto audio_data = generateMockAudio(10 * 1024); // 10 KB
    
    for (auto _ : state) {
        auto response = assistant.processVoiceCommand(audio_data, session_id);
        benchmark::DoNotOptimize(response);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * audio_data.size());
}
BENCHMARK(BM_VoiceCommandProcessing_SmallAudio);

static void BM_VoiceCommandProcessing_MediumAudio(benchmark::State& state) {
    auto config = createTestConfig();
    VoiceAssistant assistant(config);
    
    std::string session_id = "bench-session";
    auto audio_data = generateMockAudio(100 * 1024); // 100 KB
    
    for (auto _ : state) {
        auto response = assistant.processVoiceCommand(audio_data, session_id);
        benchmark::DoNotOptimize(response);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * audio_data.size());
}
BENCHMARK(BM_VoiceCommandProcessing_MediumAudio);

static void BM_VoiceCommandProcessing_LargeAudio(benchmark::State& state) {
    auto config = createTestConfig();
    VoiceAssistant assistant(config);
    
    std::string session_id = "bench-session";
    auto audio_data = generateMockAudio(1024 * 1024); // 1 MB
    
    for (auto _ : state) {
        auto response = assistant.processVoiceCommand(audio_data, session_id);
        benchmark::DoNotOptimize(response);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * audio_data.size());
}
BENCHMARK(BM_VoiceCommandProcessing_LargeAudio);

// ============================================================================
// Audio Format Conversion Benchmarks
// ============================================================================

static void BM_AudioFormatConversion_OggToMp3(benchmark::State& state) {
    auto config = createTestConfig();
    VoiceAssistant assistant(config);
    
    size_t audio_size = state.range(0) * 1024; // Size in KB
    auto audio_data = generateMockAudio(audio_size);
    
    for (auto _ : state) {
        auto converted = assistant.convertAudioFormat(audio_data, "mp3");
        benchmark::DoNotOptimize(converted);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * audio_size);
}
BENCHMARK(BM_AudioFormatConversion_OggToMp3)
    ->Arg(10)    // 10 KB
    ->Arg(100)   // 100 KB
    ->Arg(1024); // 1 MB

static void BM_AudioFormatConversion_Mp3ToOgg(benchmark::State& state) {
    auto config = createTestConfig();
    VoiceAssistant assistant(config);
    
    size_t audio_size = state.range(0) * 1024;
    auto audio_data = generateMockAudio(audio_size);
    
    for (auto _ : state) {
        auto converted = assistant.convertAudioFormat(audio_data, "ogg");
        benchmark::DoNotOptimize(converted);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * audio_size);
}
BENCHMARK(BM_AudioFormatConversion_Mp3ToOgg)
    ->Arg(10)
    ->Arg(100)
    ->Arg(1024);

// ============================================================================
// Phone Call Recording Benchmarks
// ============================================================================

static void BM_PhoneCallRecording(benchmark::State& state) {
    auto config = createTestConfig();
    VoiceAssistant assistant(config);
    
    PhoneCallMetadata metadata;
    metadata.call_id = "bench-call-001";
    metadata.caller_number = "+49123456789";
    metadata.callee_number = "+49987654321";
    metadata.start_time = 1234567890;
    metadata.end_time = 1234568890;
    metadata.duration_ms = 60000;
    metadata.call_type = "inbound";
    
    size_t audio_size = state.range(0) * 1024;
    auto audio_data = generateMockAudio(audio_size);
    
    for (auto _ : state) {
        json result = assistant.recordPhoneCall(audio_data, metadata);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * audio_size);
}
BENCHMARK(BM_PhoneCallRecording)
    ->Arg(100)   // 100 KB - short call
    ->Arg(500)   // 500 KB - medium call
    ->Arg(2048); // 2 MB - long call

// ============================================================================
// Meeting Protocol Generation Benchmarks
// ============================================================================

static void BM_MeetingProtocolGeneration(benchmark::State& state) {
    auto config = createTestConfig();
    VoiceAssistant assistant(config);
    
    MeetingMetadata metadata;
    metadata.meeting_id = "bench-meeting-001";
    metadata.title = "Performance Benchmark Meeting";
    metadata.start_time = 1234567890;
    metadata.end_time = 1234571490;
    metadata.participants = {"user1@test.com", "user2@test.com", "user3@test.com"};
    metadata.organizer = "user1@test.com";
    
    size_t audio_size = state.range(0) * 1024;
    auto audio_data = generateMockAudio(audio_size);
    
    for (auto _ : state) {
        json protocol = assistant.generateMeetingProtocol(audio_data, metadata);
        benchmark::DoNotOptimize(protocol);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * audio_size);
}
BENCHMARK(BM_MeetingProtocolGeneration)
    ->Arg(500)   // 500 KB - 15 min meeting
    ->Arg(2048)  // 2 MB - 60 min meeting
    ->Arg(8192); // 8 MB - 4 hour meeting

// ============================================================================
// Storage Operations Benchmarks
// ============================================================================

static void BM_StoreRecording(benchmark::State& state) {
    auto config = createTestConfig();
    VoiceAssistant assistant(config);
    
    size_t audio_size = state.range(0) * 1024;
    auto audio_data = generateMockAudio(audio_size);
    std::string transcript = "This is a benchmark recording.";
    json metadata;
    metadata["source"] = "benchmark";
    metadata["type"] = "performance_test";
    
    for (auto _ : state) {
        std::string doc_id = assistant.storeRecording(audio_data, transcript, metadata);
        benchmark::DoNotOptimize(doc_id);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * audio_size);
}
BENCHMARK(BM_StoreRecording)
    ->Arg(10)
    ->Arg(100)
    ->Arg(1024);

static void BM_StoreRecordingWithCompression(benchmark::State& state) {
    auto config = createTestConfig();
    config.compress_audio = true;
    VoiceAssistant assistant(config);
    
    size_t audio_size = state.range(0) * 1024;
    auto audio_data = generateMockAudio(audio_size);
    std::string transcript = "Compressed benchmark recording.";
    json metadata;
    metadata["compression"] = "enabled";
    
    for (auto _ : state) {
        std::string doc_id = assistant.storeRecording(audio_data, transcript, metadata);
        benchmark::DoNotOptimize(doc_id);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * audio_size);
}
BENCHMARK(BM_StoreRecordingWithCompression)
    ->Arg(100)
    ->Arg(1024)
    ->Arg(4096);

// ============================================================================
// Statistics Collection Benchmarks
// ============================================================================

static void BM_GetStatistics(benchmark::State& state) {
    auto config = createTestConfig();
    VoiceAssistant assistant(config);
    
    for (auto _ : state) {
        json stats = assistant.getStatistics();
        benchmark::DoNotOptimize(stats);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GetStatistics);

// ============================================================================
// Concurrent Operations Benchmarks
// ============================================================================

static void BM_ConcurrentTextCommands(benchmark::State& state) {
    auto config = createTestConfig();
    VoiceAssistant assistant(config);
    
    const int num_threads = state.range(0);
    std::vector<std::string> commands;
    for (int i = 0; i < num_threads; i++) {
        commands.push_back("Command from thread " + std::to_string(i));
    }
    
    int cmd_idx = 0;
    for (auto _ : state) {
        std::string response = assistant.processTextCommand(
            commands[cmd_idx % num_threads],
            "session-" + std::to_string(cmd_idx % num_threads)
        );
        benchmark::DoNotOptimize(response);
        cmd_idx++;
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ConcurrentTextCommands)
    ->Arg(1)
    ->Arg(4)
    ->Arg(8)
    ->Arg(16)
    ->ThreadRange(1, 8);

// ============================================================================
// Memory Usage Benchmarks
// ============================================================================

static void BM_MemoryUsagePerSession(benchmark::State& state) {
    auto config = createTestConfig();
    VoiceAssistant assistant(config);
    
    const int num_sessions = state.range(0);
    
    for (auto _ : state) {
        state.PauseTiming();
        std::vector<std::string> session_ids;
        for (int i = 0; i < num_sessions; i++) {
            session_ids.push_back("mem-session-" + std::to_string(i));
        }
        state.ResumeTiming();
        
        for (const auto& session_id : session_ids) {
            VoiceSession session = assistant.getSession(session_id);
            benchmark::DoNotOptimize(session);
        }
    }
    
    state.SetItemsProcessed(state.iterations() * num_sessions);
}
BENCHMARK(BM_MemoryUsagePerSession)
    ->Arg(10)
    ->Arg(100)
    ->Arg(1000)
    ->Arg(10000);

// ============================================================================
// Main
// ============================================================================

BENCHMARK_MAIN();
