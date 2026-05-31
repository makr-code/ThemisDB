# PERFORMANCE_EXPECTATIONS - src/voice

## Scope
- Module: src/voice
- This file defines measurable Voice module performance expectations for release gating.

## Benchmark Reference
- Relevant benchmark files:
  - benchmarks/bench_voice_assistant.cpp
  - benchmarks/bench_voice_wake_word_batch.cpp
  - benchmarks/bench_whisper_transcription.cpp

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
