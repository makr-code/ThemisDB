# whisper Module - Developer Gap Note

> Auto-generated from ai_working\gap_scan_results.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: whisper
- Generated: 2026-06-04 08:50:22
- Status: Critical Findings Present
- Total Findings: 43
- Actionable Findings (Critical + High): 22
- Affected Files: 6

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 6 |
| High | 16 |
| Medium | 21 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| primitive_no_volatile | 13 |
| db_connection_leak | 5 |
| thread_join_no_timeout | 5 |
| uncaught_exception | 5 |
| string_concat_loop | 3 |
| delete_no_nullptr | 1 |
| delete_without_nullptr | 1 |
| explicit_delete | 1 |
| generic_catch | 1 |
| manual_cleanup | 1 |
| manual_cleanup_in_destructor | 1 |
| missing_module_doc | 1 |
| no_retry_logic | 1 |
| resource_leaked_in_exception | 1 |
| smart_ptr_misuse | 1 |
| stale_doc_section_reference | 1 |
| uninitialized_access | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| whisper/tests/test_whisper_plugin.cpp | 19 | 5 | 1 | 13 | 0 |
| whisper/whisper_plugin.cpp | 14 | 1 | 9 | 4 | 0 |
| whisper/audio_chunk_reader.cpp | 7 | 0 | 4 | 3 | 0 |
| whisper | 1 | 0 | 0 | 1 | 0 |
| whisper/whisper_plugin_registrar.cpp | 1 | 0 | 1 | 0 | 0 |
| whisper/whisper_transcriber.cpp | 1 | 0 | 1 | 0 | 0 |

## Full Scanner Findings

### whisper/tests/test_whisper_plugin.cpp
Total findings: 19

- Line 446: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: for (auto& th : threads) th.join();
- Line 470: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: for (auto& th : threads) th.join();
- Line 494: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: for (auto& th : threads) th.join();
- Line 915: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: setter.join();
- Line 916: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: caller.join();
- Line 941: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 434: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: constexpr int kThreads = 8;
- Line 435: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: constexpr int kCalls   = 20;
- Line 438: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: for (int i = 0; i < kThreads; ++i) {
- Line 441: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: for (int j = 0; j < kCalls; ++j) {
- Line 458: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: constexpr int kThreads = 4;
- Line 459: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: constexpr int kCalls   = 25;
- Line 462: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: for (int i = 0; i < kThreads; ++i) {
- Line 464: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: for (int j = 0; j < kCalls; ++j) {
- Line 483: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: constexpr int kThreads = 6;
- Line 486: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: for (int i = 0; i < kThreads; ++i) {
- Line 488: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: for (int j = 0; j < 10; ++j) {
- Line 895: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: for (int i = 0; i < 50; ++i) {
- Line 905: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: for (int i = 0; i < 50; ++i) {

### whisper/whisper_plugin.cpp
Total findings: 14

- Line 273: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: return new themis::whisper::WhisperPlugin();
- Line 25: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: void WhisperPlugin::setStubTranscriberFactoryFn(StubTranscriberFactoryFn fn) {

    std::lock_guard<std::mutex> lk(s_stub_transcriber_factory_mutex);

    s_stub_transcriber_factory_fn = std::move(fn);

}



// ── constructors ─────────────────────────────────────────────────────────────
- Line 90: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (!initialized_.load(std::memory_order_acquire)) {
- Line 129: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (!initialized_.load(std::memory_order_acquire)) {
- Line 157: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (!initialized_.load(std::memory_order_acquire)) return {};
- Line 177: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (!initialized_.load(std::memory_order_acquire)) {
- Line 259: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: {"initialized",        initialized_.load(std::memory_order_acquire)},
- Line 278: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: delete p;  // delete nullptr is well-defined; ownership transferred to this function
- Line 278: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: extern "C" THEMIS_PLUGIN_EXPORT

void themis_audio_destroy(themis::audio::IAudioBackend* p) {

    delete p;  // delete nullptr is well-defined; ownership transferred to this function

}

#endif
- Line 278: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: delete p;  // delete nullptr is well-defined; ownership transferred to this function
- Line 40: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Stub/Simulation Lifecycle' that was not found in 'src/whisper/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/whisper/FUTURE_ENHANCEMENTS.md § "Stub/Simulation Lifecycle"
- Line 53: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: transcriber_.reset();

        } catch (const char*) {

            transcriber_.reset();

        } catch (...) {

            transcriber_.reset();

        }

    }
- Line 53: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 278: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: delete p;  // delete nullptr is well-defined; ownership transferred to this function

### whisper/audio_chunk_reader.cpp
Total findings: 7

- Line 160: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }

                return out;

            } else {

                throw std::runtime_error(

                    "WavAudioChunkReader: unsupported format (audio_format=" +

                    std::to_string(audio_format) + ", bits=" +

                    std::to_string(bits_per_sample) + ")");
- Line 170: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: if (chunk_size & 1) ++pos;  // RIFF alignment padding

    }



    throw std::runtime_error("WavAudioChunkReader: 'data' chunk not found");

}



// ── FfmpegAudioChunkReader ───────────────────────────────────────────────────
- Line 219: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: {

        FILE* probe = THEMIS_POPEN("ffmpeg -version 2>/dev/null", "r");

        if (!probe) {

            throw std::runtime_error("FfmpegAudioChunkReader: ffmpeg not available");

        }

        // Read a byte to confirm it actually opened

        char buf[4];
- Line 226: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: bool ok = (std::fread(buf, 1, 1, probe) == 1);

        THEMIS_PCLOSE(probe);

        if (!ok) {

            throw std::runtime_error("FfmpegAudioChunkReader: ffmpeg not available");

        }

    }
- Line 198: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: result += '\'';
- Line 201: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: result += "'\\''";
- Line 202: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: result += "'\\''";

### whisper
Total findings: 1

- Line 1: severity=MEDIUM; category=missing_module_doc
  Description: Module 'whisper' missing required governance doc 'PRODUCTION_REQUIREMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_module_governance_rules
  Context: Expected file: src/whisper/PRODUCTION_REQUIREMENTS.md

### whisper/whisper_plugin_registrar.cpp
Total findings: 1

- Line 90: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: return [](WhisperPlugin& plugin, const json& config) -> bool {

### whisper/whisper_transcriber.cpp
Total findings: 1

- Line 28: severity=HIGH; category=manual_cleanup_in_destructor
  Description: Manual resource cleanup in destructor (should use RAII wrapper)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: WhisperCppTranscriber::~WhisperCppTranscriber() {

## Update Workflow

- Refresh scanner artifacts with: python tools/gs3_orchestrator.py ./src --output ai_working/gap_scan_results.json
- Regenerate docs with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- Add --no-mirror when you only want archive docs in ai_working/module_gaps.

Format: THEMIS_MODULE_GAPS_V4
