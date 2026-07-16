# voice Module - Developer Gap Note

> Auto-generated from ai_working\gap_scan_results.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: voice
- Generated: 2026-06-04 08:50:22
- Status: Critical Findings Present
- Total Findings: 100
- Actionable Findings (Critical + High): 45
- Affected Files: 20

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 13 |
| High | 32 |
| Medium | 51 |
| Low | 4 |

## Category Summary

| Category | Count |
|---|---:|
| map_vs_unordered_map | 10 |
| o_n_squared | 6 |
| pointer_arithmetic_unbounded | 6 |
| string_concat_loop | 6 |
| uninitialized_access | 6 |
| copy_overhead | 5 |
| generic_catch | 5 |
| hardcoded_path | 5 |
| resource_leaked_in_exception | 5 |
| stale_doc_section_reference | 5 |
| uncaught_exception | 5 |
| missing_audit_log | 4 |
| missing_resource_limits | 4 |
| primitive_no_volatile | 4 |
| data_race | 3 |
| exception_in_destructor | 3 |
| unstructured_log | 3 |
| unvalidated_llm_output | 3 |
| iterator_invalidation | 2 |
| nested_loop_find | 2 |
| range_temporary | 2 |
| arithmetic_overflow | 1 |
| missing_vector_reserve | 1 |
| module_doc_linkset_drift | 1 |
| repeated_search | 1 |
| smart_ptr_misuse | 1 |
| unnecessary_copy | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| voice/voice_assistant.cpp | 11 | 3 | 7 | 1 | 0 |
| voice/voice_macro_manager.cpp | 11 | 0 | 3 | 8 | 0 |
| voice/voice_tts_customizer.cpp | 10 | 1 | 0 | 9 | 0 |
| voice/voice_authenticator.cpp | 9 | 1 | 4 | 2 | 2 |
| voice/voice_telephony.cpp | 8 | 2 | 2 | 4 | 0 |
| voice/emotion_analyzer.cpp | 7 | 0 | 1 | 5 | 1 |
| voice/voice_accessibility.cpp | 7 | 0 | 0 | 7 | 0 |
| voice/voice_assistant_llm.cpp | 7 | 0 | 3 | 4 | 0 |
| voice/voice_batch_processor.cpp | 6 | 0 | 1 | 5 | 0 |
| voice/voice_browser_streaming.cpp | 5 | 2 | 0 | 3 | 0 |
| voice/voice_intent_detector.cpp | 4 | 2 | 2 | 0 | 0 |
| voice/voice_meeting_support.cpp | 4 | 0 | 3 | 1 | 0 |
| voice/voice_audio_storage.cpp | 3 | 0 | 3 | 0 | 0 |
| voice/voice_error_handler.cpp | 2 | 0 | 2 | 0 | 0 |
| voice/PRODUCTION_REQUIREMENTS.md | 1 | 0 | 0 | 0 | 1 |
| voice/audio_preprocessing.cpp | 1 | 0 | 0 | 1 | 0 |
| voice/voice_model_cache.cpp | 1 | 1 | 0 | 0 | 0 |
| voice/voice_security.cpp | 1 | 0 | 0 | 1 | 0 |
| voice/voice_session_manager.cpp | 1 | 1 | 0 | 0 | 0 |
| voice/wake_word_detector.cpp | 1 | 0 | 1 | 0 | 0 |

## Full Scanner Findings

### voice/voice_assistant.cpp
Total findings: 11

- Line 144: severity=CRITICAL; category=missing_audit_log
  Description: Security function "authenticate" without audit log
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: auto auth_result = voice_authenticator_.authenticate(uid, audio_data);
- Line 264: severity=CRITICAL; category=missing_audit_log
  Description: Security function "authenticate" without audit log
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: auto auth_result = voice_authenticator_.authenticate(uid, audio_data);
- Line 659: severity=CRITICAL; category=missing_audit_log
  Description: Security function "authenticate" without audit log
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: auto result = voice_authenticator_.authenticate(user_id, audio_sample);
- Line 395: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Add metadata

    json call_metadata;

    call_metadata["caller"] = metadata.caller_number;

    call_metadata["callee"] = metadata.callee_number;

    call_metadata["start_time"] = metadata.start_time;

    call_metadata["end_time"] = metadata.end_time;
- Line 396: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Add metadata

    json call_metadata;

    call_metadata["caller"] = metadata.caller_number;

    call_metadata["callee"] = metadata.callee_number;

    call_metadata["start_time"] = metadata.start_time;

    call_metadata["end_time"] = metadata.end_time;

    call_metadata["call_type"] = metadata.call_type;
- Line 397: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: json call_metadata;

    call_metadata["caller"] = metadata.caller_number;

    call_metadata["callee"] = metadata.callee_number;

    call_metadata["start_time"] = metadata.start_time;

    call_metadata["end_time"] = metadata.end_time;

    call_metadata["call_type"] = metadata.call_type;

    call_metadata["custom_fields"] = metadata.custom_fields;
- Line 398: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: call_metadata["caller"] = metadata.caller_number;

    call_metadata["callee"] = metadata.callee_number;

    call_metadata["start_time"] = metadata.start_time;

    call_metadata["end_time"] = metadata.end_time;

    call_metadata["call_type"] = metadata.call_type;

    call_metadata["custom_fields"] = metadata.custom_fields;

    result["metadata"] = call_metadata;
- Line 399: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: call_metadata["callee"] = metadata.callee_number;

    call_metadata["start_time"] = metadata.start_time;

    call_metadata["end_time"] = metadata.end_time;

    call_metadata["call_type"] = metadata.call_type;

    call_metadata["custom_fields"] = metadata.custom_fields;

    result["metadata"] = call_metadata;
- Line 400: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: call_metadata["start_time"] = metadata.start_time;

    call_metadata["end_time"] = metadata.end_time;

    call_metadata["call_type"] = metadata.call_type;

    call_metadata["custom_fields"] = metadata.custom_fields;

    result["metadata"] = call_metadata;

    

    // Add segments
- Line 548: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 522: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Voice Audio Format Conversion' that was not found in 'src/voice/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/voice/FUTURE_ENHANCEMENTS.md §Voice Audio Format Conversion.

### voice/voice_macro_manager.cpp
Total findings: 11

- Line 226: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
- Line 324: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (std::find(mt.begin(), mt.end(), tag) != mt.end()) {
- Line 453: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = impl_->macros.find(id);
- Line 94: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (auto it = j["parameters"].begin(); it != j["parameters"].end(); ++it) {
- Line 175: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const std::map<std::string, std::string>& runtime_params)
- Line 188: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto apply = [&](const std::map<std::string, std::string>& params) {
- Line 222: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: delay_ms = 0;

            } catch (const char*) {

                delay_ms = 0;

            } catch (...) {

                delay_ms = 0;

            }

            if (delay_ms > 0 && delay_ms <= 60000) {
- Line 222: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 371: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const std::map<std::string, std::string>& parameters)
- Line 399: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (!combined_output.empty()) combined_output += '\n';
- Line 400: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (!combined_output.empty()) combined_output += '\n';

### voice/voice_tts_customizer.cpp
Total findings: 10

- Line 295: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: const std::set<std::string>* allowed = (it != allowed_map.end()) ? &it->second : nullptr;
- Line 194: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: } catch (const std::out_of_range&) {

            } catch (const std::string&) {

            } catch (const char*) {

            } catch (...) {

            }

        }

        if (!pitch_str.empty()) {
- Line 194: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 204: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: } catch (const std::out_of_range&) {

            } catch (const std::string&) {

            } catch (const char*) {

            } catch (...) {

            }

        }

        seg = validateProsody(seg);
- Line 204: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 226: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (!last_space) { collapsed += ' '; last_space = true; }
- Line 227: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (!last_space) { collapsed += ' '; last_space = true; }
- Line 256: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const std::map<std::string, std::set<std::string>>& ssmlAllowedAttrs() {
- Line 257: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: static const std::map<std::string, std::set<std::string>> kAttrs = {
- Line 473: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: energies.push_back(static_cast<float>(std::sqrt(e / static_cast<double>(window))));

### voice/voice_authenticator.cpp
Total findings: 9

- Line 335: severity=CRITICAL; category=missing_audit_log
  Description: Security function "authenticate" without audit log
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: VoiceAuthResult VoiceBiometricAuthenticator::authenticate(
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #2605 [voice] Implement speaker v... (2026-03-12)
- Line 510: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: *   Index  16:     Spectral centroid (normalised to [0,1])
- Line 515: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: *   Index  21:     Crest factor (normalised to [0,1])
- Line 561: severity=HIGH; category=arithmetic_overflow
  Description: Arithmetic operation result assigned to variable (potential overflow)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['        size_t start = static_cast<size_t>(b) * band_size;', '        size_t end   = (b == kBands - 1) ? n : start + band_size;', '        size_t len   = end - start;', '', '        float rms = 0.0f;']
- Line 494: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: if (callback) {

        try {

            callback(claimed_user_id, result);

        } catch (...) {

            // Audit callbacks must never affect authentication results.

        }

    }
- Line 494: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 285: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: log_sum   += std::log(mag);
- Line 635: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: log_geo  += std::log(e);

### voice/voice_telephony.cpp
Total findings: 8

- Line 215: severity=CRITICAL; category=exception_in_destructor
  Description: Destructors must be noexcept; exceptions here cause std::terminate()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 454: severity=CRITICAL; category=exception_in_destructor
  Description: Destructors must be noexcept; exceptions here cause std::terminate()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #3663 feat(voice): register focus... (2026-03-12) | #3605 feat(voice): teleph
- Line 129: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (lower_sdp.find(codec) != std::string::npos)
- Line 157: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: << "m=audio 0 RTP/AVP " << payload_type << "\r\n"
- Line 159: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: << "/" << clock << "\r\n"
- Line 376: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'SIP TTS G' that was not found in 'src/voice/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/voice/FUTURE_ENHANCEMENTS.md §SIP TTS G.711 Encoder.
- Line 592: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'WebRTC TTS Opus Encoder' that was not found in 'src/voice/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/voice/FUTURE_ENHANCEMENTS.md §WebRTC TTS Opus Encoder.

### voice/emotion_analyzer.cpp
Total findings: 7

- Line 16: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * 1. Convert raw 16-bit LE PCM bytes to normalised float samples [-1, +1].
- Line 400: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: /*static*/ std::map<Emotion, float> EmotionAnalyzer::softmax(
- Line 401: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const std::map<Emotion, float>& raw)
- Line 409: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<Emotion, float> out;
- Line 429: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const std::map<Emotion, float>& probs) const
- Line 432: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: static const std::map<Emotion, float> kPolarity = {
- Line 298: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: log_geo += std::log(e);

### voice/voice_accessibility.cpp
Total findings: 7

- Line 225: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: ss << "</head>\n<body>\n";
- Line 238: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: ss << cue.text << "</p>\n";
- Line 241: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: ss << "</article>\n</body>\n</html>\n";
- Line 361: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: result.push_back(sorted[0]);
- Line 367: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: last.text += " " + sorted[i].text;
- Line 368: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: last.text += " " + sorted[i].text;
- Line 371: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: result.push_back(sorted[i]);

### voice/voice_assistant_llm.cpp
Total findings: 7

- Line 145: severity=HIGH; category=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: std::string response = THEMIS_LLM_GENERATE(prompt.str());
- Line 209: severity=HIGH; category=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: std::string response = THEMIS_LLM_GENERATE(prompt.str());
- Line 254: severity=HIGH; category=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: std::string response = THEMIS_LLM_GENERATE(prompt.str());
- Line 145: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: std::string response = THEMIS_LLM_GENERATE(prompt.str());
- Line 177: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: std::string summary = THEMIS_LLM_GENERATE(prompt.str());
- Line 209: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: std::string response = THEMIS_LLM_GENERATE(prompt.str());
- Line 254: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: std::string response = THEMIS_LLM_GENERATE(prompt.str());

### voice/voice_batch_processor.cpp
Total findings: 6

- Line 313: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = job_summaries_.find(job_id);
- Line 71: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: float total_wer = 0.0f;
- Line 72: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: float total_pesq = 0.0f;
- Line 73: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: int wer_count = 0;
- Line 74: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: int pesq_count = 0;
- Line 386: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: frame_rms.push_back(std::sqrt(sum / static_cast<float>(window_size)));

### voice/voice_browser_streaming.cpp
Total findings: 5

- Line 144: severity=CRITICAL; category=exception_in_destructor
  Description: Destructors must be noexcept; exceptions here cause std::terminate()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 160: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: new VoiceStreamingSession(std::move(config)));
- Line 95: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Browser STT Backend.' that was not found in 'src/voice/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/voice/FUTURE_ENHANCEMENTS.md §Browser STT Backend.
- Line 150: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: end();

        } catch (const std::string&) {

        } catch (const char*) {

        } catch (...) {

        }

    }

}
- Line 150: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {

### voice/voice_intent_detector.cpp
Total findings: 4

- Line 199: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: ent.start_offset = static_cast<int>(it->position());
- Line 200: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: ent.end_offset   = static_cast<int>(it->position() + it->length());
- Line 112: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (lower.find(kw) != std::string::npos) return true;
- Line 149: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (lower.find(kw) != std::string::npos) ++hits;

### voice/voice_meeting_support.cpp
Total findings: 4

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #3423 [WIP] Add real-time meeting... (2026-03-12)
- Line 49: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (lower.find(toLower(t)) != std::string::npos) return true;
- Line 146: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: size_t pos = lower.find(pat);
- Line 68: severity=MEDIUM; category=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (!current.empty()) sentences.push_back(current);

### voice/voice_audio_storage.cpp
Total findings: 3

- Line 230: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (lower_transcript.find(lower_query) != std::string::npos) {
- Line 252: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 253: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety

### voice/voice_error_handler.cpp
Total findings: 2

- Line 164: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(ms));
- Line 248: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety

### voice/PRODUCTION_REQUIREMENTS.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'PRODUCTION_REQUIREMENTS.md' is missing expected cross-links: FUTURE_ENHANCEMENTS.md, README.md, ROADMAP.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### voice/audio_preprocessing.cpp
Total findings: 1

- Line 65: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Phase 2: RNNoise integration' that was not found in 'src/voice/ROADMAP.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/voice/ROADMAP.md § "Phase 2: RNNoise integration"

### voice/voice_model_cache.cpp
Total findings: 1

- Line 267: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = lru_map_.find(model_id);

### voice/voice_security.cpp
Total findings: 1

- Line 213: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: result.push_back(*it);

### voice/voice_session_manager.cpp
Total findings: 1

- Line 207: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = active_cache_.find(session_id);

### voice/wake_word_detector.cpp
Total findings: 1

- Line 151: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety

## Update Workflow

- Refresh scanner artifacts with: python tools/gs3_orchestrator.py ./src --output ai_working/gap_scan_results.json
- Regenerate docs with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- Add --no-mirror when you only want archive docs in ai_working/module_gaps.

Format: THEMIS_MODULE_GAPS_V4
