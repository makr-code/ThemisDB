# voice Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: voice
- Generated: 2026-06-02 11:55:48
- Status: Critical Findings Present
- Total Findings: 186
- Actionable Findings (Critical + High): 66
- Affected Files: 18

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 17 |
| High | 49 |
| Medium | 117 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| performance_patterns | 87 |
| container | 34 |
| memory | 17 |
| llm_ai_safety | 15 |
| exception_safety | 8 |
| platform | 7 |
| performance | 5 |
| audit_logging | 4 |
| concurrency | 3 |
| observability | 3 |
| reliability | 2 |
| raii | 1 |
| security | 1 |
| type_conversion | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/voice/voice_macro_manager.cpp | 31 | 1 | 4 | 26 | 0 |
| src/voice/voice_assistant.cpp | 26 | 3 | 18 | 5 | 0 |
| src/voice/voice_telephony.cpp | 15 | 2 | 2 | 11 | 0 |
| src/voice/voice_accessibility.cpp | 14 | 0 | 0 | 14 | 0 |
| src/voice/voice_assistant_llm.cpp | 13 | 3 | 6 | 4 | 0 |
| src/voice/voice_tts_customizer.cpp | 12 | 1 | 0 | 11 | 0 |
| src/voice/voice_meeting_support.cpp | 11 | 0 | 5 | 6 | 0 |
| src/voice/voice_authenticator.cpp | 10 | 1 | 4 | 3 | 2 |
| src/voice/voice_batch_processor.cpp | 9 | 0 | 1 | 8 | 0 |
| src/voice/emotion_analyzer.cpp | 8 | 0 | 1 | 6 | 1 |
| src/voice/voice_audio_storage.cpp | 7 | 0 | 3 | 4 | 0 |
| src/voice/voice_intent_detector.cpp | 7 | 2 | 2 | 3 | 0 |
| src/voice/voice_security.cpp | 7 | 0 | 0 | 7 | 0 |
| src/voice/wake_word_detector.cpp | 5 | 0 | 1 | 4 | 0 |
| src/voice/voice_session_manager.cpp | 4 | 1 | 0 | 3 | 0 |
| src/voice/voice_error_handler.cpp | 3 | 0 | 2 | 1 | 0 |
| src/voice/voice_browser_streaming.cpp | 2 | 2 | 0 | 0 | 0 |
| src/voice/voice_model_cache.cpp | 2 | 1 | 0 | 1 | 0 |

## Full Scanner Findings

### src/voice/voice_macro_manager.cpp
Total findings: 31

- Line 190: severity=CRITICAL; category=hardcoded_secret
  Description: hardcoded_secret: Hardcoded secret — use environment variable
  Remediation: Hardcoded secret — use environment variable
  Context: std::string token = "@" + kv.first;
- Line 226: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
- Line 324: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: if (std::find(mt.begin(), mt.end(), tag) != mt.end()) {
- Line 324: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: if (std::find(mt.begin(), mt.end(), tag) != mt.end()) {
- Line 453: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = impl_->macros.find(id);
- Line 45: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<char>(std::tolower(c)));
  Confidence: band=high; score=0.74
- Line 82: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: sub.push_back(stepToJson(s));
  Confidence: band=high; score=0.74
- Line 82: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: sub.push_back(stepToJson(s));
  Confidence: band=high; score=0.74
- Line 83: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: sub.push_back(stepToJson(s));
- Line 94: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: for (auto it = j["parameters"].begin(); it != j["parameters"].end(); ++it) {
  Confidence: band=high; score=0.74
- Line 99: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: step.sub_steps.push_back(stepFromJson(s));
  Confidence: band=high; score=0.74
- Line 99: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: step.sub_steps.push_back(stepFromJson(s));
  Confidence: band=high; score=0.74
- Line 100: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: step.sub_steps.push_back(stepFromJson(s));
- Line 119: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: steps.push_back(stepToJson(s));
  Confidence: band=high; score=0.74
- Line 120: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: steps.push_back(stepToJson(s));
- Line 150: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: m.steps.push_back(stepFromJson(s));
  Confidence: band=high; score=0.74
- Line 151: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: m.steps.push_back(stepFromJson(s));
- Line 175: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<std::string, std::string>& runtime_params)
  Confidence: band=high; score=0.74
- Line 188: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: auto apply = [&](const std::map<std::string, std::string>& params) {
  Confidence: band=high; score=0.74
- Line 316: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(kv.second);
  Confidence: band=high; score=0.74
- Line 328: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (matched) result.push_back(kv.second);
  Confidence: band=high; score=0.74
- Line 371: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<std::string, std::string>& parameters)
  Confidence: band=high; score=0.74
- Line 399: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!combined_output.empty()) combined_output += '\n';
  Confidence: band=high; score=0.74
- Line 400: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (!combined_output.empty()) combined_output += '\n';
- Line 404: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.step_results.push_back(std::move(sr));
  Confidence: band=high; score=0.74
- Line 449: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(macroInfoToJson(kv.second));
  Confidence: band=high; score=0.74
- Line 449: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(macroInfoToJson(kv.second));
  Confidence: band=high; score=0.74
- Line 450: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: arr.push_back(macroInfoToJson(kv.second));
- Line 455: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(macroInfoToJson(it->second));
  Confidence: band=high; score=0.74
- Line 456: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: arr.push_back(macroInfoToJson(it->second));
- Line 491: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: imported_ids.push_back(id);
  Confidence: band=high; score=0.74

### src/voice/voice_assistant.cpp
Total findings: 26

- Line 144: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authenticate" without audit log
  Context: auto auth_result = voice_authenticator_.authenticate(uid, audio_data);
  Confidence: band=very_high; score=0.99
- Line 264: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authenticate" without audit log
  Context: auto auth_result = voice_authenticator_.authenticate(uid, audio_data);
  Confidence: band=very_high; score=0.99
- Line 659: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authenticate" without audit log
  Context: auto result = voice_authenticator_.authenticate(user_id, audio_sample);
  Confidence: band=very_high; score=0.99
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 162: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto transcription = stt_processor_->transcribe(audio_data);
- Line 375: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto transcription = stt_processor_->transcribe(audio_data, transcription_options);
- Line 395: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: call_metadata["caller"] = metadata.caller_number;
- Line 396: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: call_metadata["callee"] = metadata.callee_number;
- Line 397: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: call_metadata["start_time"] = metadata.start_time;
- Line 398: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: call_metadata["end_time"] = metadata.end_time;
- Line 399: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: call_metadata["call_type"] = metadata.call_type;
- Line 400: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: call_metadata["custom_fields"] = metadata.custom_fields;
- Line 401: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result["metadata"] = call_metadata;
- Line 454: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto protocol = stt_processor_->generateMeetingProtocol(audio_data, protocol_options);
- Line 457: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: protocol["meeting_id"] = metadata.meeting_id;
- Line 458: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: protocol["title"] = metadata.title;
- Line 459: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: protocol["start_time"] = metadata.start_time;
- Line 460: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: protocol["end_time"] = metadata.end_time;
- Line 461: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: protocol["duration_ms"] = metadata.end_time - metadata.start_time;
- Line 462: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: protocol["organizer"] = metadata.organizer;
- Line 469: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: protocol["custom_fields"] = metadata.custom_fields;
- Line 185: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: session.history.push_back("User: " + transcription.full_text);
- Line 337: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: session.history.push_back("User: " + full_transcript);
- Line 341: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: session.history.push_back("Assistant: " + llm_response);
- Line 413: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: segments.push_back(seg_json);
  Confidence: band=high; score=0.74
- Line 465: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: participants.push_back(participant);
  Confidence: band=high; score=0.74

### src/voice/voice_telephony.cpp
Total findings: 15

- Line 0: severity=CRITICAL; category=uncategorized
  Confidence: band=very_high; score=0.85
- Line 0: severity=CRITICAL; category=uncategorized
  Confidence: band=very_high; score=0.85
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #3663 feat(voice): register focus... (2026-03-12) | #3605 feat(voice): teleph
- Line 129: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (lower_sdp.find(codec) != std::string::npos)
- Line 157: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "m=audio 0 RTP/AVP " << payload_type << "\r\n"
- Line 157: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "m=audio 0 RTP/AVP " << payload_type << "\r\n"
- Line 159: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "/" << clock << "\r\n"
- Line 159: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "/" << clock << "\r\n"
- Line 307: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (uint8_t b : payload) pcm.push_back(alawToPcm(b));
  Confidence: band=high; score=0.74
- Line 315: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: pcm.push_back(s);
  Confidence: band=high; score=0.74
- Line 315: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: pcm.push_back(s);
  Confidence: band=high; score=0.74
- Line 385: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (char c : text) pkt.push_back(static_cast<uint8_t>(c));
- Line 385: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: packets.push_back(std::move(pkt));
  Confidence: band=high; score=0.74
- Line 598: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (char c : text) pkt.push_back(static_cast<uint8_t>(c));
- Line 598: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: packets.push_back(std::move(pkt));
  Confidence: band=high; score=0.74

### src/voice/voice_accessibility.cpp
Total findings: 14

- Line 60: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cues.push_back(std::move(cue));
  Confidence: band=high; score=0.74
- Line 88: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cues.push_back(std::move(cue));
  Confidence: band=high; score=0.74
- Line 225: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: ss << "</head>\n<body>\n";
- Line 238: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: ss << cue.text << "</p>\n";
- Line 241: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: ss << "</article>\n</body>\n</html>\n";
- Line 262: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(obj);
  Confidence: band=high; score=0.74
- Line 314: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(cue);
  Confidence: band=high; score=0.74
- Line 349: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(sub);
  Confidence: band=high; score=0.74
- Line 349: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(sub);
  Confidence: band=high; score=0.74
- Line 361: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(sorted[0]);
- Line 367: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: last.text += " " + sorted[i].text;
  Confidence: band=high; score=0.74
- Line 368: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: last.text += " " + sorted[i].text;
- Line 370: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(sorted[i]);
  Confidence: band=high; score=0.74
- Line 371: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(sorted[i]);

### src/voice/voice_assistant_llm.cpp
Total findings: 13

- Line 85: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: entry.details = "Prompt input blocked by shared prompt policy";
  Confidence: band=very_high; score=0.99
- Line 87: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: {"blocked_rule", user_input_outcome.blocked_rule},
  Confidence: band=very_high; score=0.99
- Line 88: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: {"blocked_reason", user_input_outcome.blocked_reason}
  Confidence: band=very_high; score=0.99
- Line 85: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: entry.details = "Prompt input blocked by shared prompt policy";
  Confidence: band=very_high; score=0.9
- Line 87: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: {"blocked_rule", user_input_outcome.blocked_rule},
  Confidence: band=very_high; score=0.9
- Line 88: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: {"blocked_reason", user_input_outcome.blocked_reason}
  Confidence: band=very_high; score=0.9
- Line 139: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::string response = THEMIS_LLM_GENERATE(prompt.str());
  Confidence: band=very_high; score=0.9
- Line 203: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::string response = THEMIS_LLM_GENERATE(prompt.str());
  Confidence: band=very_high; score=0.9
- Line 248: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::string response = THEMIS_LLM_GENERATE(prompt.str());
  Confidence: band=very_high; score=0.9
- Line 139: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: std::string response = THEMIS_LLM_GENERATE(prompt.str());
  Confidence: band=high; score=0.74
- Line 171: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: std::string summary = THEMIS_LLM_GENERATE(prompt.str());
  Confidence: band=high; score=0.74
- Line 203: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: std::string response = THEMIS_LLM_GENERATE(prompt.str());
  Confidence: band=high; score=0.74
- Line 248: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: std::string response = THEMIS_LLM_GENERATE(prompt.str());
  Confidence: band=high; score=0.74

### src/voice/voice_tts_customizer.cpp
Total findings: 12

- Line 295: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const std::set<std::string>* allowed = (it != allowed_map.end()) ? &it->second : nullptr;
- Line 106: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(p);
  Confidence: band=high; score=0.74
- Line 117: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(p);
  Confidence: band=high; score=0.74
- Line 226: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!last_space) { collapsed += ' '; last_space = true; }
  Confidence: band=high; score=0.74
- Line 226: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!last_space) { collapsed += ' '; last_space = true; }
  Confidence: band=high; score=0.74
- Line 227: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (!last_space) { collapsed += ' '; last_space = true; }
- Line 256: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<std::string, std::set<std::string>>& ssmlAllowedAttrs() {
  Confidence: band=high; score=0.74
- Line 257: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: static const std::map<std::string, std::set<std::string>> kAttrs = {
  Confidence: band=high; score=0.74
- Line 472: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: energies.push_back(static_cast<float>(std::sqrt(e / static_cast<double>(window))));
  Confidence: band=high; score=0.74
- Line 472: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: energies.push_back(static_cast<float>(std::sqrt(e / static_cast<double>(window))));
  Confidence: band=high; score=0.74
- Line 473: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: energies.push_back(static_cast<float>(std::sqrt(e / static_cast<double>(window))));
- Line 545: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(lv);
  Confidence: band=high; score=0.74

### src/voice/voice_meeting_support.cpp
Total findings: 11

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #3423 [WIP] Add real-time meeting... (2026-03-12)
- Line 49: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (lower.find(toLower(t)) != std::string::npos) return true;
- Line 49: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (lower.find(toLower(t)) != std::string::npos) return true;
- Line 146: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: size_t pos = lower.find(pat);
- Line 146: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: size_t pos = lower.find(pat);
- Line 68: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (!current.empty()) sentences.push_back(current);
  Confidence: band=high; score=0.74
- Line 68: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (!current.empty()) sentences.push_back(current);
  Confidence: band=high; score=0.74
- Line 178: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: items.push_back(std::move(ai));
  Confidence: band=high; score=0.74
- Line 191: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: decisions.push_back(sent);
  Confidence: band=high; score=0.74
- Line 208: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: points.push_back(sent);
  Confidence: band=high; score=0.74
- Line 247: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: protocol.segments.push_back(seg);
  Confidence: band=high; score=0.74

### src/voice/voice_authenticator.cpp
Total findings: 10

- Line 335: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authenticate" without audit log
  Context: VoiceAuthResult VoiceBiometricAuthenticator::authenticate(
  Confidence: band=very_high; score=0.99
- Line 0: severity=HIGH; category=uncategorized
  Context: ['        size_t start = static_cast<size_t>(b) * band_size;', '        size_t end   = (b == kBands - 1) ? n : start + band_size;', '        size_t len   = end - start;', '', '        float rms = 0.0f;']
  Confidence: band=high; score=0.81
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #2605 [voice] Implement speaker v... (2026-03-12)
- Line 510: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: *   Index  16:     Spectral centroid (normalised to [0,1])
- Line 515: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: *   Index  21:     Crest factor (normalised to [0,1])
- Line 215: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.matches.push_back(m);
  Confidence: band=high; score=0.74
- Line 430: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ids.push_back(kv.first);
  Confidence: band=high; score=0.74
- Line 494: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 285: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: log_sum   += std::log(mag);
  Confidence: band=medium; score=0.6
- Line 635: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: log_geo  += std::log(e);
  Confidence: band=medium; score=0.6

### src/voice/voice_batch_processor.cpp
Total findings: 9

- Line 313: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = job_summaries_.find(job_id);
- Line 117: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(processItem(items[j]));
  Confidence: band=high; score=0.74
- Line 117: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(processItem(items[j]));
  Confidence: band=high; score=0.74
- Line 302: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: items.push_back(std::move(item));
  Confidence: band=high; score=0.74
- Line 385: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: frame_rms.push_back(std::sqrt(sum / static_cast<float>(window_size)));
  Confidence: band=high; score=0.74
- Line 385: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: frame_rms.push_back(std::sqrt(sum / static_cast<float>(window_size)));
  Confidence: band=high; score=0.74
- Line 385: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: frame_rms.push_back(std::sqrt(sum / static_cast<float>(window_size)));
  Confidence: band=high; score=0.74
- Line 386: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: frame_rms.push_back(std::sqrt(sum / static_cast<float>(window_size)));
- Line 417: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: samples.push_back(static_cast<float>(s) / 32768.0f);
  Confidence: band=high; score=0.74

### src/voice/emotion_analyzer.cpp
Total findings: 8

- Line 16: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * 1. Convert raw 16-bit LE PCM bytes to normalised float samples [-1, +1].
- Line 185: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: timeline.timeline.push_back(te);
  Confidence: band=high; score=0.74
- Line 400: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: /*static*/ std::map<Emotion, float> EmotionAnalyzer::softmax(
  Confidence: band=high; score=0.74
- Line 401: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<Emotion, float>& raw)
  Confidence: band=high; score=0.74
- Line 409: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<Emotion, float> out;
  Confidence: band=high; score=0.74
- Line 429: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<Emotion, float>& probs) const
  Confidence: band=high; score=0.74
- Line 432: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: static const std::map<Emotion, float> kPolarity = {
  Confidence: band=high; score=0.74
- Line 298: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: log_geo += std::log(e);
  Confidence: band=medium; score=0.6

### src/voice/voice_audio_storage.cpp
Total findings: 7

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 230: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (lower_transcript.find(lower_query) != std::string::npos) {
  Confidence: band=very_high; score=0.9
- Line 207: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(rec);
  Confidence: band=high; score=0.74
- Line 230: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(rec);
  Confidence: band=high; score=0.74
- Line 230: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(rec);
  Confidence: band=high; score=0.74
- Line 230: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(rec);
  Confidence: band=high; score=0.74

### src/voice/voice_intent_detector.cpp
Total findings: 7

- Line 199: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ent.start_offset = static_cast<int>(it->position());
- Line 200: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ent.end_offset   = static_cast<int>(it->position() + it->length());
- Line 112: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (lower.find(kw) != std::string::npos) return true;
- Line 149: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (lower.find(kw) != std::string::npos) ++hits;
  Confidence: band=very_high; score=0.9
- Line 181: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entities.push_back(ent);
  Confidence: band=high; score=0.74
- Line 200: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entities.push_back(ent);
  Confidence: band=high; score=0.74
- Line 223: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entities.push_back(ent);
  Confidence: band=high; score=0.74

### src/voice/voice_security.cpp
Total findings: 7

- Line 91: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.found_pii.emplace_back(type, matched);
  Confidence: band=high; score=0.74
- Line 91: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.found_pii.emplace_back(type, matched);
  Confidence: band=high; score=0.74
- Line 119: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cumulative.found_pii.push_back(p);
  Confidence: band=high; score=0.74
- Line 119: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cumulative.found_pii.push_back(p);
  Confidence: band=high; score=0.74
- Line 212: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(*it);
  Confidence: band=high; score=0.74
- Line 213: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(*it);
- Line 276: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: audit_entries.push_back(e);
  Confidence: band=high; score=0.74

### src/voice/wake_word_detector.cpp
Total findings: 5

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 69: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: tokens.push_back(tok);
  Confidence: band=high; score=0.74
- Line 103: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: wake_words_.push_back(std::move(ww));
  Confidence: band=high; score=0.74
- Line 123: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ids.push_back(ww.id);
  Confidence: band=high; score=0.74
- Line 151: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: sample_buffer_.push_back(s);
  Confidence: band=high; score=0.74

### src/voice/voice_session_manager.cpp
Total findings: 4

- Line 207: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = active_cache_.find(session_id);
- Line 61: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ids.push_back(id);
  Confidence: band=high; score=0.74
- Line 280: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (session.user_id == user_id) result.push_back(session);
  Confidence: band=high; score=0.74
- Line 280: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (session.user_id == user_id) result.push_back(session);
  Confidence: band=high; score=0.74

### src/voice/voice_error_handler.cpp
Total findings: 3

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 164: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(ms));
- Line 279: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: json status;

### src/voice/voice_browser_streaming.cpp
Total findings: 2

- Line 0: severity=CRITICAL; category=uncategorized
  Confidence: band=very_high; score=0.85
- Line 160: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: new VoiceStreamingSession(std::move(config)));

### src/voice/voice_model_cache.cpp
Total findings: 2

- Line 267: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = lru_map_.find(model_id);
- Line 257: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: models_arr.push_back(m);
  Confidence: band=high; score=0.74

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
