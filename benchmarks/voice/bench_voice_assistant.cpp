/**
 * @file bench_voice_assistant.cpp
 * @brief Performance benchmarks for Voice Assistant module
 * 
 * Benchmarks voice command processing, audio conversion, transcription,
 * storage operations, STT latency, and TTS generation speed.
 * 
 * @author ThemisDB Team
 * @date January 2025
 */

#include <benchmark/benchmark.h>
#include "voice/voice_assistant.h"
#include "voice/wake_word_detector.h"
#include "content/stt_processor.h"
#include "content/tts_processor.h"
#include "content/content_plugin_interface.h"
#include <nlohmann/json.hpp>
#include <cmath>
#include <numbers>
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
    std::vector<std::string> session_ids = {};

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
    std::vector<std::string> commands = {};

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
        std::vector<std::string> session_ids = {};

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
// Wake-Word Detection Benchmarks
// ============================================================================

// Build 16-bit PCM audio at 16 kHz with a sine wave of given amplitude.
static std::vector<uint8_t> generateSinePcm(int duration_ms, float amplitude,
                                             int sample_rate = 16000) {
    if (duration_ms <= 0) return {};
    constexpr float kFrequencyHz = 440.0f;
    const int num_samples = (sample_rate * duration_ms) / 1000;
    std::vector<uint8_t> pcm;
    pcm.reserve(static_cast<size_t>(num_samples) * 2);
    for (int i = 0; i < num_samples; ++i) {
        float val = amplitude *
            std::sin(2.0f * std::numbers::pi_v<float> * kFrequencyHz * i / sample_rate);
        auto s = static_cast<int16_t>(val * 32767.0f);
        pcm.push_back(static_cast<uint8_t>(s & 0xFF));
        pcm.push_back(static_cast<uint8_t>((s >> 8) & 0xFF));
    }
    return pcm;
}

// Benchmark: processAudioChunk with silence (VAD gate path).
static void BM_WakeWordDetect_Silence(benchmark::State& state) {
    WakeWordConfig cfg;
    cfg.vad_min_energy = 0.01f;
    cfg.sensitivity    = 0.5f;
    WakeWordDetector detector(cfg);
    detector.addWakeWord("hey-themis", "hey themis");
    detector.addWakeWord("themis",     "themis");

    int duration_ms = state.range(0);
    auto audio = generateSinePcm(duration_ms, 0.0001f);  // near-silence

    for (auto _ : state) {
        auto result = detector.processAudioChunk(audio);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(audio.size()));
}
BENCHMARK(BM_WakeWordDetect_Silence)
    ->Arg(100)   // 100 ms chunk
    ->Arg(500)   // 500 ms chunk
    ->Arg(1500); // 1.5 s chunk (full buffer)

// Benchmark: processAudioChunk with voiced audio (full scoring path).
static void BM_WakeWordDetect_Voiced(benchmark::State& state) {
    WakeWordConfig cfg;
    cfg.vad_min_energy = 0.0f;   // Always pass VAD gate
    cfg.sensitivity    = 0.99f;  // Very high threshold so we score but rarely fire
    cfg.cooldown_ms    = 0;
    WakeWordDetector detector(cfg);
    detector.addWakeWord("hey-themis", "hey themis");
    detector.addWakeWord("themis",     "themis");
    detector.addWakeWord("database",   "database");

    int duration_ms = state.range(0);
    auto audio = generateSinePcm(duration_ms, 0.8f);

    for (auto _ : state) {
        detector.reset();  // clear cooldown so each chunk is evaluated
        auto result = detector.processAudioChunk(audio);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(audio.size()));
}
BENCHMARK(BM_WakeWordDetect_Voiced)
    ->Arg(100)
    ->Arg(500)
    ->Arg(1500);

// Benchmark: Wake-word detection via VoiceAssistant::detectWakeWord.
static void BM_VoiceAssistant_DetectWakeWord(benchmark::State& state) {
    VoiceAssistant::Config cfg = createTestConfig();
    cfg.enable_wake_word                  = true;
    cfg.wake_word_config.vad_min_energy   = 0.0f;
    cfg.wake_word_config.sensitivity      = 0.99f;
    cfg.wake_word_config.cooldown_ms      = 0;
    VoiceAssistant assistant(cfg);

    auto audio = generateSinePcm(state.range(0), 0.8f);

    for (auto _ : state) {
        auto result = assistant.detectWakeWord(audio);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(audio.size()));
}
BENCHMARK(BM_VoiceAssistant_DetectWakeWord)
    ->Arg(100)
    ->Arg(500)
    ->Arg(1500);

// Benchmark: getStatistics after detection processing.
static void BM_WakeWordDetector_GetStatistics(benchmark::State& state) {
    WakeWordDetector detector;
    detector.addWakeWord("hey-themis", "hey themis");
    auto audio = generateSinePcm(500, 0.5f);
    detector.processAudioChunk(audio);  // prime the stats

    for (auto _ : state) {
        auto stats = detector.getStatistics();
        benchmark::DoNotOptimize(stats);
    }

    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_WakeWordDetector_GetStatistics);

// ============================================================================
// STT Latency Benchmarks
// ============================================================================

// Build a minimal valid WAV blob (PCM 16-bit LE, 16 kHz mono) of the given
// duration. The audio content is random noise – sufficient to exercise the
// full STTProcessor::transcribe() code path without a real Whisper model.
static std::vector<uint8_t> buildWavBlob(int duration_ms, int sample_rate = 16000) {
    const int num_samples = (sample_rate * duration_ms) / 1000;
    const int data_bytes  = num_samples * 2; // 16-bit samples

    std::vector<uint8_t> wav;
    wav.reserve(44 + static_cast<size_t>(data_bytes));

    // RIFF header
    auto write32le = [&](uint32_t v) {
        wav.push_back(static_cast<uint8_t>(v));
        wav.push_back(static_cast<uint8_t>(v >> 8));
        wav.push_back(static_cast<uint8_t>(v >> 16));
        wav.push_back(static_cast<uint8_t>(v >> 24));
    };
    auto write16le = [&](uint16_t v) {
        wav.push_back(static_cast<uint8_t>(v));
        wav.push_back(static_cast<uint8_t>(v >> 8));
    };

    wav.insert(wav.end(), {'R','I','F','F'});
    write32le(static_cast<uint32_t>(36 + data_bytes)); // ChunkSize
    wav.insert(wav.end(), {'W','A','V','E'});
    wav.insert(wav.end(), {'f','m','t',' '});
    write32le(16);                       // Subchunk1Size (PCM)
    write16le(1);                        // AudioFormat   (PCM = 1)
    write16le(1);                        // NumChannels   (mono)
    write32le(static_cast<uint32_t>(sample_rate));
    write32le(static_cast<uint32_t>(sample_rate * 2)); // ByteRate
    write16le(2);                        // BlockAlign
    write16le(16);                       // BitsPerSample
    wav.insert(wav.end(), {'d','a','t','a'});
    write32le(static_cast<uint32_t>(data_bytes));

    // PCM samples (random noise)
    std::mt19937 gen(42);
    std::uniform_int_distribution<int16_t> dis(
        std::numeric_limits<int16_t>::min(),
        std::numeric_limits<int16_t>::max());
    for (int i = 0; i < num_samples; ++i) {
        auto s = dis(gen);
        wav.push_back(static_cast<uint8_t>(s & 0xFF));
        wav.push_back(static_cast<uint8_t>((s >> 8) & 0xFF));
    }
    return wav;
}

static themis::content::STTProcessor& getSTTProcessor() {
    static themis::content::STTProcessor processor;
    static bool initialized = false;
    if (!initialized) {
        nlohmann::json settings;
        settings["model_path"]              = "/tmp/bench_stt_model";
        settings["model_size"]              = "base";
        settings["language"]                = "en";
        settings["enable_timestamps"]       = true;
        settings["enable_speaker_diarization"] = false;
        themis::content::PluginConfig config(settings);
        processor.initialize(config);
        initialized = true;
    }
    return processor;
}

// Benchmark: STT transcribe() latency for short audio (1 s).
static void BM_STTLatency_Short(benchmark::State& state) {
    auto& processor = getSTTProcessor();
    auto audio = buildWavBlob(1000); // 1 s

    for (auto _ : state) {
        auto result = processor.transcribe(audio);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(audio.size()));
    state.counters["audio_duration_ms"] = 1000;
}
BENCHMARK(BM_STTLatency_Short);

// Benchmark: STT transcribe() latency across multiple audio durations.
static void BM_STTLatency_ByDuration(benchmark::State& state) {
    auto& processor = getSTTProcessor();
    int duration_ms = state.range(0);
    auto audio = buildWavBlob(duration_ms);

    for (auto _ : state) {
        auto result = processor.transcribe(audio);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(audio.size()));
    state.counters["audio_duration_ms"] = static_cast<double>(duration_ms);
}
BENCHMARK(BM_STTLatency_ByDuration)
    ->Arg(500)    //  0.5 s (short utterance)
    ->Arg(1000)   //  1.0 s
    ->Arg(5000)   //  5.0 s (sentence-length)
    ->Arg(30000)  // 30.0 s (paragraph)
    ->Arg(60000); // 60.0 s (one minute of speech)

// Benchmark: STT transcribe() with speaker diarization enabled.
static void BM_STTLatency_WithDiarization(benchmark::State& state) {
    themis::content::STTProcessor processor;
    nlohmann::json settings;
    settings["model_path"]                 = "/tmp/bench_stt_model";
    settings["model_size"]                 = "base";
    settings["language"]                   = "en";
    settings["enable_speaker_diarization"] = true;
    settings["max_speakers"]               = 4;
    themis::content::PluginConfig config(settings);
    processor.initialize(config);

    int duration_ms = state.range(0);
    auto audio = buildWavBlob(duration_ms);

    for (auto _ : state) {
        nlohmann::json opts;
        opts["speaker_diarization"] = true;
        auto result = processor.transcribe(audio, opts);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(audio.size()));
    state.counters["audio_duration_ms"] = static_cast<double>(duration_ms);
}
BENCHMARK(BM_STTLatency_WithDiarization)
    ->Arg(1000)   // 1 s
    ->Arg(5000)   // 5 s
    ->Arg(30000); // 30 s

// Benchmark: STT streamTranscribe() latency (real-time streaming path).
static void BM_STTLatency_Streaming(benchmark::State& state) {
    auto& processor = getSTTProcessor();
    int duration_ms = state.range(0);
    auto audio = buildWavBlob(duration_ms);

    for (auto _ : state) {
        int segment_count = 0;
        bool ok = processor.streamTranscribe(audio,
            [&segment_count](const themis::content::TranscriptionSegment& /*seg*/) {
                ++segment_count;
            });
        benchmark::DoNotOptimize(ok);
        benchmark::DoNotOptimize(segment_count);
    }

    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(audio.size()));
    state.counters["audio_duration_ms"] = static_cast<double>(duration_ms);
}
BENCHMARK(BM_STTLatency_Streaming)
    ->Arg(1000)   // 1 s
    ->Arg(5000)   // 5 s
    ->Arg(30000); // 30 s

// Benchmark: STT statistics collection overhead.
static void BM_STTStatistics(benchmark::State& state) {
    auto& processor = getSTTProcessor();
    for (auto _ : state) {
        auto stats = processor.getStatistics();
        benchmark::DoNotOptimize(stats);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_STTStatistics);

// ============================================================================
// TTS Generation Speed Benchmarks
// ============================================================================

static themis::content::TTSProcessor& getTTSProcessor() {
    static themis::content::TTSProcessor processor;
    static bool initialized = false;
    if (!initialized) {
        nlohmann::json settings;
        settings["model_path"]    = "/tmp/bench_tts_model";
        settings["default_voice"] = "default";
        settings["language"]      = "en";
        settings["sample_rate"]   = 22050;
        themis::content::PluginConfig config(settings);
        processor.initialize(config);
        initialized = true;
    }
    return processor;
}

// Benchmark: TTS synthesize() generation speed for a short phrase.
static void BM_TTSGenSpeed_Short(benchmark::State& state) {
    auto& processor = getTTSProcessor();
    const std::string text = "Hello, how can I help you today?";

    for (auto _ : state) {
        auto result = processor.synthesize(text);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(state.iterations());
    state.counters["text_chars"] = static_cast<double>(text.size());
}
BENCHMARK(BM_TTSGenSpeed_Short);

// Benchmark: TTS synthesize() generation speed across different text lengths.
static void BM_TTSGenSpeed_ByLength(benchmark::State& state) {
    auto& processor = getTTSProcessor();

    // Generate text of approximately the requested character count.
    const std::string word   = "ThemisDB ";
    const int target_chars   = state.range(0);
    std::string text;
    text.reserve(static_cast<size_t>(target_chars));
    while (static_cast<int>(text.size()) < target_chars) {
        text += word;
    }
    text.resize(static_cast<size_t>(target_chars));

    for (auto _ : state) {
        auto result = processor.synthesize(text);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(state.iterations());
    state.counters["text_chars"] = static_cast<double>(text.size());
}
BENCHMARK(BM_TTSGenSpeed_ByLength)
    ->Arg(50)    // ~10 words
    ->Arg(200)   // ~40 words (typical utterance)
    ->Arg(500)   // ~100 words (paragraph)
    ->Arg(2000); // ~400 words (long passage)

// Benchmark: TTS synthesize() with non-default speed/pitch options.
static void BM_TTSGenSpeed_WithOptions(benchmark::State& state) {
    auto& processor = getTTSProcessor();
    const std::string text = "This is a benchmark for TTS generation speed with custom options.";

    themis::content::TTSOptions opts;
    opts.speed  = static_cast<float>(state.range(0)) / 10.0f; // 0.5 – 2.0
    opts.pitch  = 1.0f;
    opts.format = "wav";

    for (auto _ : state) {
        auto result = processor.synthesize(text, opts);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(state.iterations());
    state.counters["speed_x10"] = static_cast<double>(state.range(0));
}
BENCHMARK(BM_TTSGenSpeed_WithOptions)
    ->Arg(5)   // 0.5x (slow)
    ->Arg(10)  // 1.0x (normal)
    ->Arg(15)  // 1.5x (fast)
    ->Arg(20); // 2.0x (very fast)

// Benchmark: TTS streamSynthesize() throughput (streaming audio chunks).
static void BM_TTSGenSpeed_Streaming(benchmark::State& state) {
    auto& processor = getTTSProcessor();
    const std::string text =
        "Streaming speech synthesis benchmark for ThemisDB voice assistant module.";

    for (auto _ : state) {
        size_t total_bytes = 0;
        bool ok = processor.streamSynthesize(text,
            [&total_bytes](const std::vector<uint8_t>& chunk) {
                total_bytes += chunk.size();
            });
        benchmark::DoNotOptimize(ok);
        benchmark::DoNotOptimize(total_bytes);
    }

    state.SetItemsProcessed(state.iterations());
    state.counters["text_chars"] = static_cast<double>(text.size());
}
BENCHMARK(BM_TTSGenSpeed_Streaming);

// Benchmark: TTS getAvailableVoices() overhead.
static void BM_TTSAvailableVoices(benchmark::State& state) {
    auto& processor = getTTSProcessor();
    for (auto _ : state) {
        auto voices = processor.getAvailableVoices();
        benchmark::DoNotOptimize(voices);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_TTSAvailableVoices);

// Benchmark: TTS statistics collection overhead.
static void BM_TTSStatistics(benchmark::State& state) {
    auto& processor = getTTSProcessor();
    for (auto _ : state) {
        auto stats = processor.getStatistics();
        benchmark::DoNotOptimize(stats);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_TTSStatistics);

// ============================================================================
// Main
// ============================================================================

BENCHMARK_MAIN();
