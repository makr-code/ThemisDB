/**
 * @file bench_cross_functional_end_to_end.cpp
 * @brief Cross-functional end-to-end performance benchmarks
 * 
 * Benchmarks complete workflows across multiple ThemisDB components:
 * - Voice + Observability + Storage
 * - Plugin + Query + Metrics
 * - Multi-component realistic scenarios
 * 
 * @author ThemisDB Team
 * @date January 2025
 */

#include <benchmark/benchmark.h>
#if defined(THEMIS_ENABLE_VOICE_ASSISTANT)
#include "voice/voice_assistant.h"
#endif
#include "plugins/plugin_manager.h"
#include "observability/metrics_collector.h"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <random>

#if defined(THEMIS_ENABLE_VOICE_ASSISTANT)
using namespace themis::voice;
#endif
using namespace themis::plugins;
using namespace themis::observability;
using json = nlohmann::json;
namespace fs = std::filesystem;

// ============================================================================
// Setup Utilities
// ============================================================================

#if defined(THEMIS_ENABLE_VOICE_ASSISTANT)
VoiceAssistant::Config createVoiceConfig() {
    VoiceAssistant::Config config;
    config.stt_model_path = "/tmp/bench_stt";
    config.tts_model_path = "/tmp/bench_tts";
    config.llm_model_path = "/tmp/bench_llm";
    config.storage_path = "/tmp/bench_voice_storage";
    config.enable_revision_control = true;
    return config;
}
#endif

std::vector<uint8_t> generateMockAudio(size_t size) {
    std::vector<uint8_t> audio(size);
    std::random_device rd = {};
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    std::generate(audio.begin(), audio.end(), [&]() { return static_cast<uint8_t>(dis(gen)); });
    return audio;
}

void createPluginManifest(const std::string& dir, const std::string& name, PluginType type) {
    json manifest;
    manifest["name"] = name;
    manifest["version"] = "1.0.0";
    manifest["type"] = static_cast<int>(type);
    manifest["author"] = "Benchmark";
    manifest["library"] = name + ".so";
    
    std::string path = dir + "/" + name + ".json";
    std::ofstream file(path);
    file << manifest.dump(2);
    file.close();
}

// ============================================================================
// Voice + Observability + Storage Benchmarks
// ============================================================================

#if defined(THEMIS_ENABLE_VOICE_ASSISTANT)
static void BM_VoiceCommandWithMetrics(benchmark::State& state) {
    auto config = createVoiceConfig();
    VoiceAssistant assistant(config);
    auto& metrics = MetricsCollector::getInstance();
    
    for (auto _ : state) {
        state.PauseTiming();
        std::string session_id = "bench-session-" + std::to_string(state.iterations());
        std::string command = "Process this command with metrics tracking";
        metrics.reset();
        state.ResumeTiming();
        
        auto start = std::chrono::steady_clock::now();
        std::string response = assistant.processTextCommand(command, session_id);
        auto end = std::chrono::steady_clock::now();
        
        auto duration_ms = std::chrono::duration<double, std::milli>(end - start).count();
        metrics.recordQuery("voice_command", duration_ms, 1);
        metrics.recordContentImport("text/plain", command.size());
        
        std::string export_metrics = metrics.getPrometheusMetrics();
        benchmark::DoNotOptimize(response);
        benchmark::DoNotOptimize(export_metrics);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_VoiceCommandWithMetrics);

static void BM_PhoneCallProcessingWithFullMetrics(benchmark::State& state) {
    auto config = createVoiceConfig();
    VoiceAssistant assistant(config);
    auto& metrics = MetricsCollector::getInstance();
    
    size_t audio_size = state.range(0) * 1024; // KB to bytes
    auto audio_data = generateMockAudio(audio_size);
    
    for (auto _ : state) {
        state.PauseTiming();
        PhoneCallMetadata metadata;
        metadata.call_id = "bench-call-" + std::to_string(state.iterations());
        metadata.caller_number = "+49123456789";
        metadata.callee_number = "+49987654321";
        metadata.start_time = std::chrono::system_clock::now().time_since_epoch().count();
        metadata.duration_ms = 60000;
        metadata.call_type = "inbound";
        metrics.reset();
        state.ResumeTiming();
        
        auto start = std::chrono::steady_clock::now();
        json result = assistant.recordPhoneCall(audio_data, metadata);
        auto end = std::chrono::steady_clock::now();
        
        auto duration_ms = std::chrono::duration<double, std::milli>(end - start).count();
        metrics.recordContentImport("audio/call", audio_size);
        metrics.recordQuery("phone_call_processing", duration_ms, 1); // NOPII: metric operation type literal, not a phone number
        metrics.recordMemoryUsage(audio_size);
        
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * audio_size);
}
BENCHMARK(BM_PhoneCallProcessingWithFullMetrics)
    ->Arg(10)    // 10 KB
    ->Arg(100)   // 100 KB
    ->Arg(1024); // 1 MB

static void BM_MeetingProtocolWithMetrics(benchmark::State& state) {
    auto config = createVoiceConfig();
    VoiceAssistant assistant(config);
    auto& metrics = MetricsCollector::getInstance();
    
    size_t audio_size = state.range(0) * 1024;
    auto audio_data = generateMockAudio(audio_size);
    
    for (auto _ : state) {
        state.PauseTiming();
        MeetingMetadata meeting;
        meeting.meeting_id = "bench-meeting-" + std::to_string(state.iterations());
        meeting.title = "Performance Benchmark Meeting";
        meeting.start_time = std::chrono::system_clock::now().time_since_epoch().count();
        meeting.participants = {"user1@test.com", "user2@test.com", "user3@test.com"};
        meeting.organizer = "user1@test.com";
        metrics.reset();
        state.ResumeTiming();
        
        auto start = std::chrono::steady_clock::now();
        json protocol = assistant.generateMeetingProtocol(audio_data, meeting);
        auto end = std::chrono::steady_clock::now();
        
        auto duration_ms = std::chrono::duration<double, std::milli>(end - start).count();
        metrics.recordContentImport("audio/meeting", audio_size);
        metrics.recordQuery("meeting_protocol", duration_ms, 1);
        metrics.recordChunkCreation(meeting.participants.size());
        
        benchmark::DoNotOptimize(protocol);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * audio_size);
}
BENCHMARK(BM_MeetingProtocolWithMetrics)
    ->Arg(100)   // 100 KB - 5 min meeting
    ->Arg(500)   // 500 KB - 25 min meeting
    ->Arg(2048); // 2 MB - 100 min meeting

static void BM_AudioConversionWithMetrics(benchmark::State& state) {
    auto config = createVoiceConfig();
    VoiceAssistant assistant(config);
    auto& metrics = MetricsCollector::getInstance();
    
    size_t audio_size = state.range(0) * 1024;
    auto audio_data = generateMockAudio(audio_size);
    
    for (auto _ : state) {
        state.PauseTiming();
        metrics.reset();
        state.ResumeTiming();
        
        auto start = std::chrono::steady_clock::now();
        auto converted = assistant.convertAudioFormat(audio_data, "mp3");
        auto end = std::chrono::steady_clock::now();
        
        auto duration_ms = std::chrono::duration<double, std::milli>(end - start).count();
        metrics.recordQuery("audio_conversion", duration_ms, converted.size());
        
        if (converted.size() > 0) {
            double ratio = static_cast<double>(converted.size()) / audio_size;
            metrics.recordTSStoreCompression("audio", ratio);
        }
        
        benchmark::DoNotOptimize(converted);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * audio_size);
}
BENCHMARK(BM_AudioConversionWithMetrics)
    ->Arg(10)
    ->Arg(100)
    ->Arg(1024);
#endif

// ============================================================================
// Plugin + Query + Metrics Benchmarks
// ============================================================================

static void BM_PluginDiscoveryWithMetrics(benchmark::State& state) {
    const int num_plugins = state.range(0);
    std::string plugin_dir = "/tmp/bench_plugins_" + std::to_string(state.iterations());
    fs::create_directories(plugin_dir);
    
    // Create plugin manifests
    for (int i = 0; i < num_plugins; i++) {
        createPluginManifest(plugin_dir, "plugin" + std::to_string(i), PluginType::COMPUTE_BACKEND);
    }
    
    auto& metrics = MetricsCollector::getInstance();
    
    for (auto _ : state) {
        state.PauseTiming();
        PluginManager manager;
        metrics.reset();
        state.ResumeTiming();
        
        auto start = std::chrono::steady_clock::now();
        auto scan_result = manager.scanPluginDirectory(plugin_dir);
        auto end = std::chrono::steady_clock::now();

        if (!scan_result) {
            state.SkipWithError("scanPluginDirectory failed");
            break;
        }
        size_t count = *scan_result;
        
        auto duration_ms = std::chrono::duration<double, std::milli>(end - start).count();
        metrics.recordQuery("plugin_discovery", duration_ms, count);
        metrics.recordIndexScan("plugin_index", count);
        
        benchmark::DoNotOptimize(count);
    }
    
    fs::remove_all(plugin_dir);
    state.SetItemsProcessed(state.iterations() * num_plugins);
}
BENCHMARK(BM_PluginDiscoveryWithMetrics)
    ->Arg(10)
    ->Arg(50)
    ->Arg(100);

static void BM_PluginQueryWithMetrics(benchmark::State& state) {
    std::string plugin_dir = "/tmp/bench_plugin_query";
    fs::create_directories(plugin_dir);
    
    // Create mixed plugin types
    for (int i = 0; i < 50; i++) {
        PluginType type = static_cast<PluginType>(i % 4);
        createPluginManifest(plugin_dir, "plugin" + std::to_string(i), type);
    }
    
    PluginManager manager;
    auto scan_result = manager.scanPluginDirectory(plugin_dir);
    if (!scan_result) {
        state.SkipWithError("scanPluginDirectory failed");
        fs::remove_all(plugin_dir);
        return;
    }
    
    auto& metrics = MetricsCollector::getInstance();
    
    for (auto _ : state) {
        state.PauseTiming();
        metrics.reset();
        state.ResumeTiming();
        
        // Query all plugins
        auto start = std::chrono::steady_clock::now();
        auto all_plugins = manager.listPlugins();
        auto end = std::chrono::steady_clock::now();
        
        auto duration_ms = std::chrono::duration<double, std::milli>(end - start).count();
        metrics.recordQuery("get_all_plugins", duration_ms, all_plugins.size());
        metrics.recordFullScan("plugins", all_plugins.size());
        
        // Query by type
        for (int i = 0; i < 4; i++) {
            start = std::chrono::steady_clock::now();
            std::vector<PluginManifest> typed = {};

            typed.reserve(all_plugins.size());
            for (const auto& plugin : all_plugins) {
                if (plugin.type == static_cast<PluginType>(i)) {
                    typed.push_back(plugin);
                }
            }
            end = std::chrono::steady_clock::now();
            
            duration_ms = std::chrono::duration<double, std::milli>(end - start).count();
            metrics.recordQuery("get_by_type", duration_ms, typed.size());
            metrics.recordIndexScan("type_index", typed.size());
        }
        
        benchmark::DoNotOptimize(all_plugins);
    }
    
    fs::remove_all(plugin_dir);
    state.SetItemsProcessed(state.iterations() * 5); // 1 full scan + 4 type queries
}
BENCHMARK(BM_PluginQueryWithMetrics);

static void BM_PluginInfoCacheWithMetrics(benchmark::State& state) {
    std::string plugin_dir = "/tmp/bench_plugin_cache";
    fs::create_directories(plugin_dir);
    
    const int num_plugins = 20;
    for (int i = 0; i < num_plugins; i++) {
        createPluginManifest(plugin_dir, "cache_plugin" + std::to_string(i), PluginType::COMPUTE_BACKEND);
    }
    
    PluginManager manager;
    auto scan_result = manager.scanPluginDirectory(plugin_dir);
    if (!scan_result) {
        state.SkipWithError("scanPluginDirectory failed");
        fs::remove_all(plugin_dir);
        return;
    }
    auto manifests = manager.listPlugins();
    
    auto& metrics = MetricsCollector::getInstance();
    
    for (auto _ : state) {
        state.PauseTiming();
        metrics.reset();
        state.ResumeTiming();
        
        // Simulate cache behavior - repeated queries
        for (int round = 0; round < 3; round++) {
            for (int i = 0; i < num_plugins; i++) {
                std::string name = "cache_plugin" + std::to_string(i);
                
                auto start = std::chrono::steady_clock::now();
                auto info = std::find_if(manifests.begin(), manifests.end(), [&name](const PluginManifest& manifest) {
                    return manifest.name == name;
                });
                auto end = std::chrono::steady_clock::now();
                
                auto duration_ms = std::chrono::duration<double, std::milli>(end - start).count();
                metrics.recordQuery("plugin_info", duration_ms, info != manifests.end() ? 1 : 0);
                
                if (round > 0) {
                    metrics.recordCacheHit("plugin_cache");
                } else {
                    metrics.recordCacheMiss("plugin_cache");
                }
            }
        }
    }
    
    fs::remove_all(plugin_dir);
    state.SetItemsProcessed(state.iterations() * num_plugins * 3);
}
BENCHMARK(BM_PluginInfoCacheWithMetrics);

// ============================================================================
// Multi-Component Realistic Workflows
// ============================================================================

#if defined(THEMIS_ENABLE_VOICE_ASSISTANT)
static void BM_CompleteVoiceWorkflowWithMetrics(benchmark::State& state) {
    auto config = createVoiceConfig();
    VoiceAssistant assistant(config);
    auto& metrics = MetricsCollector::getInstance();
    
    for (auto _ : state) {
        state.PauseTiming();
        std::string session_id = "workflow-" + std::to_string(state.iterations());
        metrics.reset();
        state.ResumeTiming();
        
        // Step 1: Process voice command
        auto start = std::chrono::steady_clock::now();
        assistant.processTextCommand("Start recording", session_id);
        auto t1 = std::chrono::steady_clock::now();
        metrics.recordQuery("command1", std::chrono::duration<double, std::milli>(t1 - start).count(), 1);
        
        // Step 2: Record call
        auto audio = generateMockAudio(50 * 1024);
        PhoneCallMetadata metadata;
        metadata.call_id = "workflow-call";
        metadata.duration_ms = 30000;
        metadata.call_type = "outbound";
        
        assistant.recordPhoneCall(audio, metadata);
        auto t2 = std::chrono::steady_clock::now();
        metrics.recordQuery("record_call", std::chrono::duration<double, std::milli>(t2 - t1).count(), 1);
        metrics.recordContentImport("audio/call", audio.size());
        
        // Step 3: Convert format
        assistant.convertAudioFormat(audio, "mp3");
        auto t3 = std::chrono::steady_clock::now();
        metrics.recordQuery("convert", std::chrono::duration<double, std::milli>(t3 - t2).count(), 1);
        
        // Step 4: Store recording
        assistant.storeRecording(audio, "Workflow test", json());
        auto t4 = std::chrono::steady_clock::now();
        metrics.recordQuery("store", std::chrono::duration<double, std::milli>(t4 - t3).count(), 1);
        
        // Step 5: Get statistics
        assistant.getStatistics();
        metrics.getPrometheusMetrics();
        auto end = std::chrono::steady_clock::now();
        
        auto total_ms = std::chrono::duration<double, std::milli>(end - start).count();
        benchmark::DoNotOptimize(total_ms);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_CompleteVoiceWorkflowWithMetrics);
#endif

static void BM_CompletePluginWorkflowWithMetrics(benchmark::State& state) {
    std::string plugin_dir = "/tmp/bench_plugin_workflow";
    fs::create_directories(plugin_dir);
    
    for (int i = 0; i < 30; i++) {
        PluginType type = static_cast<PluginType>(i % 4);
        createPluginManifest(plugin_dir, "workflow_plugin" + std::to_string(i), type);
    }
    
    auto& metrics = MetricsCollector::getInstance();
    
    for (auto _ : state) {
        state.PauseTiming();
        PluginManager manager;
        metrics.reset();
        state.ResumeTiming();
        
        auto start = std::chrono::steady_clock::now();
        
        // Step 1: Discovery
        auto scan_result = manager.scanPluginDirectory(plugin_dir);
        auto t1 = std::chrono::steady_clock::now();

        if (!scan_result) {
            state.SkipWithError("scanPluginDirectory failed");
            break;
        }
        size_t count = *scan_result;
        metrics.recordQuery("discovery", std::chrono::duration<double, std::milli>(t1 - start).count(), count);
        
        // Step 2: Query all
        auto all = manager.listPlugins();
        auto t2 = std::chrono::steady_clock::now();
        metrics.recordQuery("query_all", std::chrono::duration<double, std::milli>(t2 - t1).count(), all.size());
        
        // Step 3: Query by type
        for (int i = 0; i < 4; i++) {
            size_t typed_count = 0;
            for (const auto& plugin : all) {
                if (plugin.type == static_cast<PluginType>(i)) {
                    ++typed_count;
                }
            }
            metrics.recordIndexScan("type_index", typed_count);
        }
        auto t3 = std::chrono::steady_clock::now();
        metrics.recordQuery("query_by_type", std::chrono::duration<double, std::milli>(t3 - t2).count(), 4);
        
        // Step 4: Get individual info
        for (int i = 0; i < 10; i++) {
            std::string name = "workflow_plugin" + std::to_string(i);
            auto info = std::find_if(all.begin(), all.end(), [&name](const PluginManifest& manifest) {
                return manifest.name == name;
            });
            benchmark::DoNotOptimize(info);
        }
        auto t4 = std::chrono::steady_clock::now();
        metrics.recordQuery("info_queries", std::chrono::duration<double, std::milli>(t4 - t3).count(), 10);
        
        // Step 5: Statistics
        auto loaded = manager.listLoadedPlugins();
        benchmark::DoNotOptimize(loaded);
        metrics.getPrometheusMetrics();
        auto end = std::chrono::steady_clock::now();
        
        auto total_ms = std::chrono::duration<double, std::milli>(end - start).count();
        benchmark::DoNotOptimize(total_ms);
    }
    
    fs::remove_all(plugin_dir);
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_CompletePluginWorkflowWithMetrics);

static void BM_ConcurrentMultiComponentWorkload(benchmark::State& state) {
#if defined(THEMIS_ENABLE_VOICE_ASSISTANT)
    auto voice_config = createVoiceConfig();
    VoiceAssistant assistant(voice_config);
#endif
    
    std::string plugin_dir = "/tmp/bench_concurrent";
    fs::create_directories(plugin_dir);
    for (int i = 0; i < 20; i++) {
        createPluginManifest(plugin_dir, "concurrent" + std::to_string(i), PluginType::COMPUTE_BACKEND);
    }
    
    PluginManager plugin_manager;
    auto scan_result = plugin_manager.scanPluginDirectory(plugin_dir);
    if (!scan_result) {
        state.SkipWithError("scanPluginDirectory failed");
        fs::remove_all(plugin_dir);
        return;
    }
    
    auto& metrics = MetricsCollector::getInstance();
    
    for (auto _ : state) {
        state.PauseTiming();
        metrics.reset();
        state.ResumeTiming();
        
        // Simulate concurrent operations across components
        auto start = std::chrono::steady_clock::now();
        
#if defined(THEMIS_ENABLE_VOICE_ASSISTANT)
        // Voice operations
        assistant.processTextCommand("Concurrent test", "session-1");
        metrics.recordQuery("voice", 1.0, 1);
#endif
        
        // Plugin operations
        auto all_plugins = plugin_manager.listPlugins();
        metrics.recordQuery("plugins", 1.0, all_plugins.size());
        benchmark::DoNotOptimize(all_plugins);
        
        // Multiple metric recordings
        for (int i = 0; i < 10; i++) {
            metrics.recordCacheHit("test_cache");
            metrics.recordIndexScan("test_index", 5);
        }
        
        // Export metrics
        std::string export_data = metrics.getPrometheusMetrics();
        
        auto end = std::chrono::steady_clock::now();
        auto duration_ms = std::chrono::duration<double, std::milli>(end - start).count();
        
        benchmark::DoNotOptimize(duration_ms);
        benchmark::DoNotOptimize(export_data);
    }
    
    fs::remove_all(plugin_dir);
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ConcurrentMultiComponentWorkload);

// ============================================================================
// Main
// ============================================================================

BENCHMARK_MAIN();
