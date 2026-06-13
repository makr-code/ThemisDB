> **Hinweis:** Produktionsanforderungen gegen aktuelle Build-Flags, Codepfade und Tests validieren.

<!-- Status: current | validated: 2026-06-13 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · SECURITY.md -->

# Production Requirements — Whisper Plugin

> For project-level production requirements, see the global [PRODUCTION_REQUIREMENTS.md](../../PRODUCTION_REQUIREMENTS.md).

## Module Scope

This document defines the production readiness criteria for the Whisper audio transcription plugin module, covering:
- `WhisperPlugin` (plugin entry point and orchestration)
- `WhisperCppTranscriber` (whisper.cpp integration)
- `WavAudioChunkReader` and `FfmpegAudioChunkReader` (audio input processing)
- `WhisperConfig` (runtime configuration)
- `EnergyThresholdVad` (voice activity detection)

---

## Production Readiness Criteria

### 1. Code Quality

| Requirement | Status | Evidence |
|-------------|--------|----------|
| All public interfaces documented with Doxygen comments | ✅ | Doxygen blocks present in all header files |
| No raw `new`/`delete` outside RAII wrappers | ⚠️ | See MODULE_GAPS.md Line 160 (smart_ptr_misuse in whisper_plugin.cpp:273) |
| All exceptions caught or documented as uncaught | ⚠️ | See MODULE_GAPS.md Lines 53, 274, 291, 308, 325 |
| No manual resource cleanup in destructors | ⚠️ | See MODULE_GAPS.md Line 268 (whisper_transcriber.cpp:28) |
| Thread-safety verified for all shared state | ⚠️ | See MODULE_GAPS.md Lines 434-488 (primitive_no_volatile findings) |

### 2. Build & Integration

| Requirement | Status | Evidence |
|-------------|--------|----------|
| Clean compilation with `-Wall -Wextra -Werror` | ✅ | CI pipeline passes |
| No dependencies on external audio libraries for WAV | ✅ | `WavAudioChunkReader` is pure C++ |
| FFmpeg dependency is optional with graceful degradation | ✅ | `FfmpegAudioChunkReader` checks for ffmpeg binary at runtime |
| Plugin interface stable (IAudioBackend) | ✅ | Documented in FUTURE_ENHANCEMENTS.md Lines 17-18 |

### 3. Configuration Management

| Requirement | Status | Evidence |
|-------------|--------|----------|
| All configuration serializable to/from JSON | ✅ | `WhisperConfig::fromJson()` and `WhisperConfig::toJson()` implemented |
| Default values are production-safe | ✅ | All defaults validated in `whisper_config.cpp` Lines 37-41 |
| Configuration changes do not require recompilation | ✅ | Runtime config via JSON |

### 4. Error Handling

| Requirement | Status | Evidence |
|-------------|--------|----------|
| All error paths return valid error codes | ✅ | `TranscriptionResult` carries success flag |
| No exception leakage from plugin boundary | ⚠️ | Generic catch(...) in whisper_plugin.cpp Lines 339, 343 |
| Resource cleanup on exception paths | ⚠️ | See MODULE_GAPS.md Line 88 (resource_leaked_in_exception) |

### 5. Performance

| Metric | Target | Current | Evidence |
|--------|--------|---------|----------|
| Transcription latency (1-min audio, 16kHz, stub) | < 100ms | < 50ms | Performance test results |
| VAD processing (1-sec chunk, CPU) | < 5ms | < 5ms | FUTURE_ENHANCEMENTS.md Line 92 |
| First-token latency (streaming) | ≤ 50ms | Planned Q3 2026 | FUTURE_ENHANCEMENTS.md Line 54 |
| Memory usage per concurrent transcription | < 500MB | TBD | Needs profiling |

### 6. Security

| Requirement | Status | Evidence |
|-------------|--------|----------|
| Input validation for all file paths | ✅ | SECURITY.md Lines 33-35 |
| No transcript text in logs | ✅ | SECURITY.md Line 50 |
| Path traversal prevention | ✅ | SECURITY.md Line 21 |
| Model file integrity verification | ❌ | Planned Q3 2026, SECURITY.md Lines 38-39 |

### 7. Observability

| Requirement | Status | Evidence |
|-------------|--------|----------|
| All operations emit metrics | ⚠️ | Needs implementation |
| Provenance stamps on all results | ✅ | SECURITY.md Lines 42-43 |
| Structured logging for all significant events | ⚠️ | Needs audit |

### 8. Testing

| Requirement | Status | Evidence |
|-------------|--------|----------|
| Unit test coverage > 80% | ⚠️ | Needs measurement |
| Integration tests with audio files | ✅ | `tests/test_whisper_plugin.cpp` exists |
| Exception safety tests | ⚠️ | See MODULE_GAPS.md uncaught_exception findings |
| Thread-safety tests | ⚠️ | See MODULE_GAPS.md thread_join_no_timeout findings |

---

## Blocking Issues for Production

Based on MODULE_GAPS.md analysis, the following issues must be resolved before production deployment:

### Critical (Must Fix)
1. **Thread join without timeout** (5 occurrences in test_whisper_plugin.cpp)
   - Lines 446, 470, 494, 915, 916
   - Remediation: Add timeout parameters to all thread joins

2. **Smart pointer misuse** (whisper_plugin.cpp:273)
   - Raw `new` without immediate wrapping in smart pointer
   - Remediation: Use `std::make_unique` or `std::make_shared`

### High Priority (Should Fix Before Production)
1. **Resource leaks** (whisper_plugin.cpp Lines 184, 189, 194, 199, 204)
   - DB connection leak findings
   - Remediation: Verify all resource acquisitions have matching releases

2. **Uninitialized access** (whisper_plugin_registrar.cpp:90)
   - Container element access before initialization
   - Remediation: Ensure all containers are initialized before access

3. **Manual cleanup in destructor** (whisper_transcriber.cpp:28)
   - Manual resource cleanup should use RAII wrapper
   - Remediation: Replace manual cleanup with smart pointers or RAII wrappers

4. **Missing retry logic** (whisper_plugin.cpp:25)
   - RPC/network call without retry logic
   - Remediation: Add retry logic with exponential backoff

5. **Generic catch blocks** (whisper_plugin.cpp:339, 343)
   - Generic catch(...) ignores specific exception types
   - Remediation: Catch specific exception types and handle appropriately

---

## Deployment Requirements

### Prerequisites
- C++17 or later
- whisper.cpp library and model files
- FFmpeg binary (optional, for MP3/OGG support)
- nlohmann/json header

### Configuration
```json
{
  "model_path": "/path/to/ggml-model.bin",
  "language": "auto",
  "n_threads": 4,
  "translate": false,
  "beam_size": 5,
  "print_progress": false,
  "quality_threshold": 0.0,
  "language_confidence_threshold": 0.0
}
```

### Resource Limits
- Recommended: Limit audio file size to 100MB
- Recommended: Limit concurrent transcriptions per instance
- Recommended: Timeout for transcription operations: 30 seconds

---

## Monitoring Requirements

The following metrics should be monitored in production:
- `whisper_transcription_count` (counter)
- `whisper_transcription_duration_ms` (histogram)
- `whisper_transcription_errors` (counter)
- `whisper_audio_bytes_processed` (counter)
- `whisper_concurrent_transcriptions` (gauge)

---

## Validation Checklist

- [ ] All critical issues from MODULE_GAPS.md resolved
- [ ] All high-priority issues from MODULE_GAPS.md resolved or accepted
- [ ] Thread-safety verified for all shared state
- [ ] Exception safety verified for all code paths
- [ ] Resource leak testing completed
- [ ] Performance benchmarks meet targets
- [ ] Security controls validated
- [ ] Documentation reviewed and updated

---

Format: THEMIS_PRODUCTION_REQUIREMENTS_V1
