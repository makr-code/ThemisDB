# PERFORMANCE_EXPECTATIONS - src/voice

## Scope
- Module: src/voice
- This file defines measurable Voice module performance expectations for release gating.
- Phase 5: Performance Benchmarks and SLA Gates (2026-08-08)

## Benchmark Reference
- Relevant benchmark files (Phase 4 baseline):
  - benchmarks/bench_voice_assistant.cpp
  - benchmarks/bench_voice_wake_word_batch.cpp
  - benchmarks/bench_whisper_transcription.cpp
- Phase 5 benchmark suites (NEW):
  - benchmarks/voice/benchmark_fixtures.h (foundational mocks and utilities)
  - benchmarks/voice/bench_voice_stt_tts_latency.cpp (Task 5.1)
  - benchmarks/voice/bench_voice_streaming_throughput.cpp (Task 5.2)
  - benchmarks/voice/bench_voice_session_lifecycle.cpp (Task 5.3)
  - benchmarks/voice/bench_voice_audio_preprocessing.cpp (Task 5.4)
  - benchmarks/voice/bench_voice_assistant_latency.cpp (Task 5.5)
  - tests/integration/voice/test_voice_endurance_stress.cpp (Task 5.6)

## Specific Expectations
| Target ID | Expectation | Benchmark case |
|---|---|---|
| VOI-1 | session lifecycle overhead remains within release baseline budget | BM_SessionCreation, BM_SessionContextUpdate, BM_MultipleSessionsParallel |
| VOI-2 | command processing latency remains within release baseline budget | BM_TextCommandProcessing, BM_VoiceCommandProcessing_SmallAudio, BM_VoiceCommandProcessing_MediumAudio |
| VOI-3 | long-audio processing remains bounded | BM_VoiceCommandProcessing_LargeAudio |
| VOI-4 | wake-word detection overhead remains bounded | BM_WakeWordDetect_Silence, BM_WakeWordDetect_Voiced, BM_VoiceAssistant_DetectWakeWord |
| VOI-5 | wake-word chunk and rolling buffer processing remain bounded | BENCHMARK_F(WakeWordBenchFixture, ProcessChunk_Silent_100ms), BENCHMARK_F(WakeWordBenchFixture, ProcessChunk_Energy_100ms), BENCHMARK_F(WakeWordBenchFixture, RollingBuffer_1500ms), BENCHMARK_F(WakeWordBenchFixture, ChunkThroughput) |
| VOI-6 | batch processing scale behavior remains bounded | BENCHMARK_F(BatchProcessorScalingFixture, SingleItem), BENCHMARK_F(BatchProcessorScalingFixture, BatchThroughput_ThreadScaling), BENCHMARK_F(BatchProcessorScalingFixture, LoadTestHelper) |
| VOI-7 | STT path latency and behavior remain within release baseline budget | BM_STTLatency_Short, BM_STTLatency_ByDuration, BM_STTLatency_WithDiarization, BM_STTLatency_Streaming |
| VOI-8 | TTS generation throughput and latency remain within release baseline budget | BM_TTSGenSpeed_Short, BM_TTSGenSpeed_ByLength, BM_TTSGenSpeed_WithOptions, BM_TTSGenSpeed_Streaming |
| VOI-9 | whisper-transcription plugin paths remain bounded | BENCHMARK_F(WhisperPluginFixture, Transcribe_1s), BENCHMARK_F(WhisperPluginFixture, Transcribe_5s), BENCHMARK_F(WhisperPluginFixture, DetectLanguage), BM_Transcribe_BufferSize |
| VOI-10 | storage and meeting/phone processing paths remain bounded | BM_PhoneCallRecording, BM_MeetingProtocolGeneration, BM_StoreRecording, BM_StoreRecordingWithCompression |

## Module Hard Gates (v1.0 docs baseline)

| Gate ID | Expectation | Measurement |
|---|---|---|
| VG-1 | Regression <= 10 percent vs release baseline | (current - baseline) / baseline |
| VG-2 | STT path p99 <= release threshold | p99 from BM_STTLatency_* and mapped BENCHMARK_F(WhisperPluginFixture, ...) cases |
| VG-3 | voice command path p99 <= release threshold | p99 from BM_VoiceCommandProcessing_* mappings |
| VG-4 | No mapped benchmark case missing in release run | benchmark run manifest completeness |

## Validation
- Expectations are met when mapped benchmarks run reproducibly in release profile and remain inside configured thresholds.
- For proxy-only targets, keep follow-up benchmark hardening explicitly tracked.

## Sourcecode Verification (Module: voice/performance)

- Verified benchmark sources:
  - benchmarks/bench_voice_assistant.cpp
  - benchmarks/bench_voice_wake_word_batch.cpp
  - benchmarks/bench_whisper_transcription.cpp
- Verified mapping surfaces:
  - session and command processing benchmarks
  - wake-word and batch scaling benchmarks
  - STT/TTS and whisper-transcription benchmarks
  - storage/meeting/phone processing benchmarks
- Result:
  - Referenced benchmark cases exist in current benchmark sources.
  - Release gates remain tied to reproducible benchmark runs and baseline comparisons.

---

## Phase 5: Performance Benchmarks and SLA Gates (2026-08-08)

### 5.1 STT/TTS Latency Benchmarks

| Benchmark | Metric | Target | SLA Gate |
|-----------|--------|--------|----------|
| VoiceSTT_SmallAudio100ms | p95 latency | < 2000ms | GATE_STT_LATENCY_P95_100ms |
| VoiceSTT_SmallAudio100ms | p99 latency | < 5000ms | GATE_STT_LATENCY_P99_100ms |
| VoiceSTT_MediumAudio5s | p95 latency | < 10000ms | GATE_STT_LATENCY_P95_5s |
| VoiceSTT_MediumAudio5s | p99 latency | < 15000ms | GATE_STT_LATENCY_P99_5s |
| VoiceSTT_LargeAudio60s | p95 latency | < 60000ms | GATE_STT_LATENCY_P95_60s |
| VoiceSTT_LargeAudio60s | p99 latency | < 120000ms | GATE_STT_LATENCY_P99_60s |
| VoiceSTT_NoiseAudio | p95 latency | < 3000ms | GATE_STT_LATENCY_P95_NOISE |
| VoiceSTT_NoiseAudio | accuracy | >= 95% | GATE_STT_ACCURACY_NOISE |
| VoiceTTS_ShortText | p95 latency (10 words) | < 1000ms | GATE_TTS_LATENCY_P95_SHORT |
| VoiceTTS_ShortText | p99 latency (10 words) | < 2000ms | GATE_TTS_LATENCY_P99_SHORT |
| VoiceTTS_MediumText | p95 latency (100 words) | < 3000ms | GATE_TTS_LATENCY_P95_MEDIUM |
| VoiceTTS_MediumText | p99 latency (100 words) | < 5000ms | GATE_TTS_LATENCY_P99_MEDIUM |
| VoiceTTS_LongText | p95 latency (500 words) | < 10000ms | GATE_TTS_LATENCY_P95_LONG |
| VoiceTTS_LongText | p99 latency (500 words) | < 15000ms | GATE_TTS_LATENCY_P99_LONG |
| VoiceTTS_RapidFire | throughput | >= 10 calls/sec | GATE_TTS_THROUGHPUT |

### 5.2 Streaming Throughput Benchmarks

| Benchmark | Metric | Target | SLA Gate |
|-----------|--------|--------|----------|
| VoiceStreaming_ChunkThroughput | throughput | >= 1000 chunks/sec | GATE_STREAM_THROUGHPUT |
| VoiceStreaming_ChunkLatency | p95 latency | < 50ms/chunk | GATE_STREAM_LATENCY_P95 |
| VoiceStreaming_BufferOverhead | memory per session | < 10MB | GATE_STREAM_MEMORY |
| VoiceStreaming_MultipleStreamsThroughput | 10 concurrent | >= 5000 chunks/sec | GATE_STREAM_MULTI_THROUGHPUT |
| VoiceStreaming_StreamRebalancing | rebalance time | < 100ms | GATE_STREAM_REBALANCE |
| VoiceStreaming_ConnectionLossRecovery | recovery time | < 500ms | GATE_STREAM_RECOVERY |

### 5.3 Session Lifecycle Benchmarks

| Benchmark | Metric | Target | SLA Gate |
|-----------|--------|--------|----------|
| VoiceSession_CreateSession | latency | < 100ms | GATE_SESSION_CREATE |
| VoiceSession_CreateSessionConcurrent10 | 10 creates | < 500ms total | GATE_SESSION_CREATE_CONCURRENT |
| VoiceSession_DeleteSession | latency | < 50ms | GATE_SESSION_DELETE |
| VoiceSession_ScalingTo100Sessions | peak memory | < 500MB | GATE_SESSION_MEMORY_100 |
| VoiceSession_ScalingTo1000Sessions | peak memory | < 2GB | GATE_SESSION_MEMORY_1000 |
| VoiceSession_ResourceCleanupNoLeak | leak / 1000 cycles | < 10MB | GATE_SESSION_CLEANUP_LEAK |

### 5.4 Audio Preprocessing Benchmarks

| Benchmark | Metric | Target | SLA Gate |
|-----------|--------|--------|----------|
| VoiceAudio_ValidateAudioSmall | p95 latency (100ms audio) | < 10ms | GATE_AUDIO_VALIDATE_SMALL |
| VoiceAudio_ValidateAudioLarge | p95 latency (5s audio) | < 50ms | GATE_AUDIO_VALIDATE_LARGE |
| VoiceAudio_PreprocessChain | latency (5s audio) | < 500ms | GATE_AUDIO_PREPROCESS |
| VoiceAudio_WakeWordDetection | p95 latency (2s audio) | < 1000ms | GATE_WAKEWORD_LATENCY |
| VoiceAudio_IntentDetection | p95 latency | < 500ms | GATE_INTENT_LATENCY |
| VoiceAudio_EmotionAnalysis | p95 latency | < 200ms | GATE_EMOTION_LATENCY |

### 5.5 Backend Integration Benchmarks (Assistant Latency)

| Benchmark | Metric | Target | SLA Gate |
|-----------|--------|--------|----------|
| VoiceAssistant_LLMResponseGeneration | p95 latency | < 3000ms | GATE_LLM_LATENCY_P95 |
| VoiceAssistant_LLMResponseGeneration | p99 latency | < 10000ms | GATE_LLM_LATENCY_P99 |
| VoiceAssistant_TTSSynthesis | latency (50 words) | < 2000ms | GATE_TTS_SYNTHESIS |
| VoiceAssistant_EndToEndLatency | p95 latency (audio-in → audio-out) | < 6000ms | GATE_E2E_LATENCY_P95 |
| VoiceAssistant_CommandParsingAndRouting | latency | < 100ms | GATE_COMMAND_PARSE |
| VoiceAssistant_CommandResponseGeneration | latency | < 500ms | GATE_COMMAND_RESPONSE |
| VoiceAssistant_MultiCommandSequence | p95 latency | < 500ms/command | GATE_MULTI_CMD_LATENCY |

### 5.6 Endurance and Stress Testing

| Test Case | Configuration | Requirements | Validation |
|-----------|---------------|--------------|-----------|
| MultiSessionLoad | 50 sessions, 1 cmd/sec each, 1+ hour | No resource leaks, all commands processed | Memory stable, < 1% drop rate |
| ResourceGrowthBounded | Monitor memory, CPU, queues over time | All metrics remain stable | Memory growth < 50MB, no unbounded growth |
| NoMemoryLeaks | 100 create/delete cycles × 10 sessions | No leaks detected | Baseline memory consistent |

---

## Baseline Latencies by Workload Profile

| Operation | p50 | p95 | p99 | Profile | Notes |
|-----------|-----|-----|-----|---------|-------|
| STT (100ms audio) | 400ms | 2s | 5s | small | real-time requirement |
| STT (5s audio) | 3s | 10s | 15s | medium | typical voice command |
| STT (60s audio) | 30s | 60s | 120s | large | long-form audio |
| TTS (10 words) | 200ms | 1s | 2s | small | rapid response |
| TTS (100 words) | 600ms | 3s | 5s | medium | typical assistant response |
| TTS (500 words) | 2s | 10s | 15s | large | extended narrative |
| E2E command | 2.5s | 6s | 10s | typical | audio-in → audio-out |
| Wake-word detect | 200ms | 1s | 2s | realtime | sub-second optimal |
| Command parse | 20ms | 100ms | 200ms | fast | lightweight operation |

---

## Resource Consumption

| Metric | Per Session | 100 Sessions | 1000 Sessions | Notes |
|--------|------------|-------------|---------------|-------|
| Memory | 5MB | 500MB | 2GB | Baseline + context accumulation |
| CPU | <5% | <50% | <80% | Typical workload |
| Stream buffers | <10MB | <500MB | <2GB | Per-session buffering |
| Latency variance | p99/p50 ~10x | ~10x | ~10x | Consistent performance |

---

## SLA Gates and Alert Thresholds

| Gate | Threshold | Alert | Critical |
|------|-----------|-------|----------|
| STT p95 latency | 2s | > 2.5s | > 3s |
| STT p99 latency | 5s | > 6s | > 7s |
| TTS p95 latency | 1s | > 1.2s | > 1.5s |
| Stream throughput | 1000 chunks/s | < 900 | < 700 |
| Session memory | 5MB/session | > 8MB | > 10MB |
| E2E latency p95 | 6s | > 7s | > 8s |
| Resource leak | < 10MB / 1000 cycles | > 15MB | > 20MB |

---

## Baseline Hardware Assumptions

- **CPU**: 8-core, 2.5 GHz (representative modern CPU)
- **Memory**: 16GB available (sufficient for 1000+ concurrent sessions)
- **Storage**: SSD with typical latency <5ms
- **Network**: 10Mbps symmetric (representative cloud link)
- **OS**: Linux 5.10+ or Windows Server 2019+
- **Audio**: 16kHz, 16-bit PCM, mono (standard voice)

---

## Performance Goals

- ✅ STT/TTS latencies meet p95 targets for production workloads
- ✅ Streaming throughput supports 1000+ concurrent sessions
- ✅ Session lifecycle overhead negligible (<1% total latency)
- ✅ No resource leaks detectable over 24-hour operation
- ✅ Endurance testing confirms stability under sustained load
- ✅ All SLA gates defined, verifiable, and reproducible

---

## Validation and Release Gates

### Hard Gates (must pass to release)
- GATE_STT_LATENCY_P95_100ms: <= 2000ms ✓
- GATE_TTS_LATENCY_P95_SHORT: <= 1000ms ✓
- GATE_E2E_LATENCY_P95: <= 6000ms ✓
- GATE_STREAM_THROUGHPUT: >= 1000 chunks/sec ✓
- GATE_SESSION_MEMORY_100: <= 500MB ✓
- GATE_SESSION_CLEANUP_LEAK: <= 10MB ✓

### Endurance Requirements
- Sustained load: 50 sessions × 1 cmd/sec × 1+ hour
- Memory: stable (no growth > 50MB)
- Commands: >= 99% success rate
- No detectable leaks (valgrind/asan clean)

### Reproducibility
- All benchmarks use kCanonicalRngSeed=42
- steady_clock for consistent timing
- deterministic mock latencies
- < 5% variance across runs

---

## Specific Expectations (Legacy - Phase 4)

| Target ID | Expectation | Benchmark case |
|---|---|---|
| VOI-1 | session lifecycle overhead remains within release baseline budget | BM_SessionCreation, BM_SessionContextUpdate, BM_MultipleSessionsParallel |
| VOI-2 | command processing latency remains within release baseline budget | BM_TextCommandProcessing, BM_VoiceCommandProcessing_SmallAudio, BM_VoiceCommandProcessing_MediumAudio |
| VOI-3 | long-audio processing remains bounded | BM_VoiceCommandProcessing_LargeAudio |
| VOI-4 | wake-word detection overhead remains bounded | BM_WakeWordDetect_Silence, BM_WakeWordDetect_Voiced, BM_VoiceAssistant_DetectWakeWord |
| VOI-5 | wake-word chunk and rolling buffer processing remain bounded | BENCHMARK_F(WakeWordBenchFixture, ProcessChunk_Silent_100ms), BENCHMARK_F(WakeWordBenchFixture, ProcessChunk_Energy_100ms), BENCHMARK_F(WakeWordBenchFixture, RollingBuffer_1500ms), BENCHMARK_F(WakeWordBenchFixture, ChunkThroughput) |
| VOI-6 | batch processing scale behavior remains bounded | BENCHMARK_F(BatchProcessorScalingFixture, SingleItem), BENCHMARK_F(BatchProcessorScalingFixture, BatchThroughput_ThreadScaling), BENCHMARK_F(BatchProcessorScalingFixture, LoadTestHelper) |
| VOI-7 | STT path latency and behavior remain within release baseline budget | BM_STTLatency_Short, BM_STTLatency_ByDuration, BM_STTLatency_WithDiarization, BM_STTLatency_Streaming |
| VOI-8 | TTS generation throughput and latency remain within release baseline budget | BM_TTSGenSpeed_Short, BM_TTSGenSpeed_ByLength, BM_TTSGenSpeed_WithOptions, BM_TTSGenSpeed_Streaming |
| VOI-9 | whisper-transcription plugin paths remain bounded | BENCHMARK_F(WhisperPluginFixture, Transcribe_1s), BENCHMARK_F(WhisperPluginFixture, Transcribe_5s), BENCHMARK_F(WhisperPluginFixture, DetectLanguage), BM_Transcribe_BufferSize |
| VOI-10 | storage and meeting/phone processing paths remain bounded | BM_PhoneCallRecording, BM_MeetingProtocolGeneration, BM_StoreRecording, BM_StoreRecordingWithCompression |

### Module Hard Gates (v1.0 docs baseline)

| Gate ID | Expectation | Measurement |
|---|---|---|
| VG-1 | Regression <= 10 percent vs release baseline | (current - baseline) / baseline |
| VG-2 | STT path p99 <= release threshold | p99 from BM_STTLatency_* and mapped BENCHMARK_F(WhisperPluginFixture, ...) cases |
| VG-3 | voice command path p99 <= release threshold | p99 from BM_VoiceCommandProcessing_* mappings |
| VG-4 | No mapped benchmark case missing in release run | benchmark run manifest completeness |

### Validation
- Expectations are met when mapped benchmarks run reproducibly in release profile and remain inside configured thresholds.
- For proxy-only targets, keep follow-up benchmark hardening explicitly tracked.

### Sourcecode Verification (Module: voice/performance)

- Verified benchmark sources:
  - benchmarks/bench_voice_assistant.cpp
  - benchmarks/bench_voice_wake_word_batch.cpp
  - benchmarks/bench_whisper_transcription.cpp
  - benchmarks/voice/bench_voice_stt_tts_latency.cpp (NEW Phase 5)
  - benchmarks/voice/bench_voice_streaming_throughput.cpp (NEW Phase 5)
  - benchmarks/voice/bench_voice_session_lifecycle.cpp (NEW Phase 5)
  - benchmarks/voice/bench_voice_audio_preprocessing.cpp (NEW Phase 5)
  - benchmarks/voice/bench_voice_assistant_latency.cpp (NEW Phase 5)
- Verified mapping surfaces:
  - session and command processing benchmarks
  - wake-word and batch scaling benchmarks
  - STT/TTS and whisper-transcription benchmarks
  - storage/meeting/phone processing benchmarks
  - streaming throughput and latency (NEW)
  - assistant end-to-end latency (NEW)
- Result:
  - Referenced benchmark cases exist in current benchmark sources.
  - Release gates remain tied to reproducible benchmark runs and baseline comparisons.
  - Phase 5 SLA gates are verifiable and reproducible.

