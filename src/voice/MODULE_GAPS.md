# voice Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: voice
- Generated: 2026-05-31 08:50:11
- Status: Critical Findings Present
- Total Findings: 326
- Actionable Findings (Critical + High): 151
- Affected Files: 19

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 38 |
| High | 113 |
| Medium | 175 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| performance_patterns | 115 |
| container | 107 |
| memory | 33 |
| llm_ai_safety | 13 |
| security | 13 |
| concurrency | 11 |
| exception_safety | 8 |
| platform | 7 |
| performance | 5 |
| reliability | 5 |
| audit_logging | 4 |
| observability | 3 |
| raii | 1 |
| type_conversion | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/voice/voice_macro_manager.cpp | 39 | 1 | 7 | 31 | 0 |
| src/voice/voice_assistant.cpp | 33 | 4 | 22 | 7 | 0 |
| src/voice/voice_telephony.cpp | 29 | 4 | 7 | 18 | 0 |
| src/voice/voice_accessibility.cpp | 21 | 0 | 0 | 21 | 0 |
| src/voice/voice_audio_storage.cpp | 20 | 3 | 11 | 6 | 0 |
| src/voice/voice_tts_customizer.cpp | 19 | 2 | 3 | 14 | 0 |
| src/voice/voice_authenticator.cpp | 18 | 3 | 9 | 4 | 2 |
| src/voice/voice_model_cache.cpp | 18 | 8 | 8 | 2 | 0 |
| src/voice/voice_batch_processor.cpp | 17 | 0 | 4 | 13 | 0 |
| src/voice/voice_meeting_support.cpp | 17 | 0 | 5 | 12 | 0 |
| src/voice/voice_assistant_llm.cpp | 15 | 2 | 7 | 6 | 0 |
| src/voice/voice_session_manager.cpp | 15 | 4 | 6 | 5 | 0 |
| src/voice/voice_security.cpp | 14 | 1 | 4 | 9 | 0 |
| src/voice/voice_intent_detector.cpp | 13 | 3 | 4 | 6 | 0 |
| src/voice/wake_word_detector.cpp | 12 | 0 | 4 | 8 | 0 |
| src/voice/emotion_analyzer.cpp | 11 | 0 | 1 | 9 | 1 |
| src/voice/voice_browser_streaming.cpp | 8 | 3 | 5 | 0 | 0 |
| src/voice/audio_preprocessing.cpp | 4 | 0 | 4 | 0 | 0 |
| src/voice/voice_error_handler.cpp | 3 | 0 | 2 | 1 | 0 |

## Full Scanner Findings

### src/voice/voice_macro_manager.cpp
Total findings: 39

- Line 192: severity=CRITICAL; category=hardcoded_secret
  Description: hardcoded_secret: Hardcoded secret — use environment variable
  Remediation: Hardcoded secret — use environment variable
  Context: std::string token = "@" + kv.first;
- Line 228: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
- Line 325: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& tag : tags) {
  Confidence: band=very_high; score=0.9
- Line 326: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: if (std::find(mt.begin(), mt.end(), tag) != mt.end()) {
- Line 326: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: if (std::find(mt.begin(), mt.end(), tag) != mt.end()) {
- Line 400: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (int i = 0; i < static_cast<int>(info.steps.size()); ++i) {
  Confidence: band=very_high; score=0.9
- Line 436: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& kv : impl_->macros) {
  Confidence: band=very_high; score=0.9
- Line 455: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = impl_->macros.find(id);
- Line 47: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<char>(std::tolower(c)));
  Confidence: band=high; score=0.74
- Line 48: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(static_cast<char>(std::tolower(c)));
- Line 84: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: sub.push_back(stepToJson(s));
  Confidence: band=high; score=0.74
- Line 84: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: sub.push_back(stepToJson(s));
  Confidence: band=high; score=0.74
- Line 85: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: sub.push_back(stepToJson(s));
- Line 96: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: for (auto it = j["parameters"].begin(); it != j["parameters"].end(); ++it) {
  Confidence: band=high; score=0.74
- Line 101: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: step.sub_steps.push_back(stepFromJson(s));
  Confidence: band=high; score=0.74
- Line 101: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: step.sub_steps.push_back(stepFromJson(s));
  Confidence: band=high; score=0.74
- Line 102: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: step.sub_steps.push_back(stepFromJson(s));
- Line 121: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: steps.push_back(stepToJson(s));
  Confidence: band=high; score=0.74
- Line 122: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: steps.push_back(stepToJson(s));
- Line 152: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: m.steps.push_back(stepFromJson(s));
  Confidence: band=high; score=0.74
- Line 153: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: m.steps.push_back(stepFromJson(s));
- Line 177: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<std::string, std::string>& runtime_params)
  Confidence: band=high; score=0.74
- Line 190: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: auto apply = [&](const std::map<std::string, std::string>& params) {
  Confidence: band=high; score=0.74
- Line 318: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(kv.second);
  Confidence: band=high; score=0.74
- Line 319: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(kv.second);
- Line 330: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (matched) result.push_back(kv.second);
  Confidence: band=high; score=0.74
- Line 331: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (matched) result.push_back(kv.second);
- Line 373: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<std::string, std::string>& parameters)
  Confidence: band=high; score=0.74
- Line 401: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!combined_output.empty()) combined_output += '\n';
  Confidence: band=high; score=0.74
- Line 402: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (!combined_output.empty()) combined_output += '\n';
- Line 406: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.step_results.push_back(std::move(sr));
  Confidence: band=high; score=0.74
- Line 407: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.step_results.push_back(std::move(sr));
- Line 451: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(macroInfoToJson(kv.second));
  Confidence: band=high; score=0.74
- Line 451: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(macroInfoToJson(kv.second));
  Confidence: band=high; score=0.74
- Line 452: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: arr.push_back(macroInfoToJson(kv.second));
- Line 457: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(macroInfoToJson(it->second));
  Confidence: band=high; score=0.74
- Line 458: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: arr.push_back(macroInfoToJson(it->second));
- Line 493: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: imported_ids.push_back(id);
  Confidence: band=high; score=0.74
- Line 494: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: imported_ids.push_back(id);

### src/voice/voice_assistant.cpp
Total findings: 33

- Line 144: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authenticate" without audit log
  Context: auto auth_result = voice_authenticator_.authenticate(uid, audio_data);
  Confidence: band=very_high; score=0.99
- Line 263: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authenticate" without audit log
  Context: auto auth_result = voice_authenticator_.authenticate(uid, audio_data);
  Confidence: band=very_high; score=0.99
- Line 560: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = sessions_.find(session_id);
- Line 656: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authenticate" without audit log
  Context: return voice_authenticator_.authenticate(user_id, audio_sample);
  Confidence: band=very_high; score=0.99
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 161: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto transcription = stt_processor_->transcribe(audio_data);
- Line 296: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: bool ok = stt_processor_->streamTranscribe(audio_data, on_segment);
- Line 313: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto transcription = stt_processor_->transcribe(audio_data);
- Line 373: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto transcription = stt_processor_->transcribe(audio_data, transcription_options);
- Line 385: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result["call_id"] = metadata.call_id;
- Line 393: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: call_metadata["caller"] = metadata.caller_number;
- Line 394: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: call_metadata["callee"] = metadata.callee_number;
- Line 395: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: call_metadata["start_time"] = metadata.start_time;
- Line 396: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: call_metadata["end_time"] = metadata.end_time;
- Line 397: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: call_metadata["call_type"] = metadata.call_type;
- Line 398: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: call_metadata["custom_fields"] = metadata.custom_fields;
- Line 399: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result["metadata"] = call_metadata;
- Line 452: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto protocol = stt_processor_->generateMeetingProtocol(audio_data, protocol_options);
- Line 455: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: protocol["meeting_id"] = metadata.meeting_id;
- Line 456: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: protocol["title"] = metadata.title;
- Line 457: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: protocol["start_time"] = metadata.start_time;
- Line 458: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: protocol["end_time"] = metadata.end_time;
- Line 459: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: protocol["duration_ms"] = metadata.end_time - metadata.start_time;
- Line 460: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: protocol["organizer"] = metadata.organizer;
- Line 467: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: protocol["custom_fields"] = metadata.custom_fields;
- Line 618: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const uint8_t byte : data) {
  Confidence: band=very_high; score=0.9
- Line 184: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: session.history.push_back("User: " + transcription.full_text);
- Line 335: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: session.history.push_back("User: " + full_transcript);
- Line 339: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: session.history.push_back("Assistant: " + llm_response);
- Line 411: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: segments.push_back(seg_json);
  Confidence: band=high; score=0.74
- Line 412: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: segments.push_back(seg_json);
- Line 463: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: participants.push_back(participant);
  Confidence: band=high; score=0.74
- Line 464: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: participants.push_back(participant);

### src/voice/voice_telephony.cpp
Total findings: 29

- Line 0: severity=CRITICAL; category=uncategorized
  Confidence: band=very_high; score=0.85
- Line 0: severity=CRITICAL; category=uncategorized
  Confidence: band=very_high; score=0.85
- Line 747: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = sip_calls_.find(call_id);
- Line 811: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = webrtc_calls_.find(call_id);
- Line 131: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (lower_sdp.find(codec) != std::string::npos)
- Line 212: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 333: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto ct = runCallStt(impl_->call_id, impl_->pcm_buffer, /*is_final=*/false);
- Line 451: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 546: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: impl_->pcm_buffer.insert(impl_->pcm_buffer.end(),
- Line 825: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock_sip(sip_mutex_);
- Line 826: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock_rtc(webrtc_mutex_);
- Line 159: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "m=audio 0 RTP/AVP " << payload_type << "\r\n"
- Line 159: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "m=audio 0 RTP/AVP " << payload_type << "\r\n"
- Line 161: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "/" << clock << "\r\n"
- Line 161: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "/" << clock << "\r\n"
- Line 307: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (uint8_t b : payload) pcm.push_back(ulawToPcm(b));
- Line 309: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (uint8_t b : payload) pcm.push_back(alawToPcm(b));
  Confidence: band=high; score=0.74
- Line 310: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (uint8_t b : payload) pcm.push_back(alawToPcm(b));
- Line 317: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: pcm.push_back(s);
  Confidence: band=high; score=0.74
- Line 317: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: pcm.push_back(s);
  Confidence: band=high; score=0.74
- Line 318: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: pcm.push_back(s);
- Line 364: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: packets.push_back(std::move(pkt));
- Line 387: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (char c : text) pkt.push_back(static_cast<uint8_t>(c));
- Line 387: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: packets.push_back(std::move(pkt));
  Confidence: band=high; score=0.74
- Line 388: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: packets.push_back(std::move(pkt));
- Line 580: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: packets.push_back(std::move(pkt));
- Line 600: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (char c : text) pkt.push_back(static_cast<uint8_t>(c));
- Line 600: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: packets.push_back(std::move(pkt));
  Confidence: band=high; score=0.74
- Line 601: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: packets.push_back(std::move(pkt));

### src/voice/voice_accessibility.cpp
Total findings: 21

- Line 62: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cues.push_back(std::move(cue));
  Confidence: band=high; score=0.74
- Line 63: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: cues.push_back(std::move(cue));
- Line 90: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cues.push_back(std::move(cue));
  Confidence: band=high; score=0.74
- Line 91: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: cues.push_back(std::move(cue));
- Line 227: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: ss << "</head>\n<body>\n";
- Line 240: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: ss << cue.text << "</p>\n";
- Line 243: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: ss << "</article>\n</body>\n</html>\n";
- Line 264: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(obj);
  Confidence: band=high; score=0.74
- Line 265: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: arr.push_back(obj);
- Line 316: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(cue);
  Confidence: band=high; score=0.74
- Line 317: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(cue);
- Line 325: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: while (iss >> word) words.push_back(word);
- Line 328: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(cue);
- Line 351: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(sub);
  Confidence: band=high; score=0.74
- Line 351: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(sub);
  Confidence: band=high; score=0.74
- Line 352: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(sub);
- Line 363: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(sorted[0]);
- Line 369: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: last.text += " " + sorted[i].text;
  Confidence: band=high; score=0.74
- Line 370: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: last.text += " " + sorted[i].text;
- Line 372: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(sorted[i]);
  Confidence: band=high; score=0.74
- Line 373: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(sorted[i]);

### src/voice/voice_audio_storage.cpp
Total findings: 20

- Line 72: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: result.existing_record_id = it->second;
- Line 135: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const std::string& orig_id = dup_it->second;
- Line 192: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator rit may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto rit = records_.find(record_id);
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 54: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (uint8_t b : data) {
  Confidence: band=very_high; score=0.9
- Line 97: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: data[OGG_OPUS_SIGNATURE_OFFSET+4]=='H' &&
- Line 98: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: data[OGG_OPUS_SIGNATURE_OFFSET+5]=='e' &&
- Line 99: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: data[OGG_OPUS_SIGNATURE_OFFSET+6]=='a' &&
- Line 100: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: data[OGG_OPUS_SIGNATURE_OFFSET+7]=='d');
- Line 207: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [id, rec] : records_) {
  Confidence: band=very_high; score=0.9
- Line 231: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& c : lower_transcript) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  Confidence: band=very_high; score=0.9
- Line 232: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (lower_transcript.find(lower_query) != std::string::npos) {
  Confidence: band=very_high; score=0.9
- Line 252: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& [id, rec] : records_) {
  Confidence: band=very_high; score=0.9
- Line 209: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(rec);
  Confidence: band=high; score=0.74
- Line 210: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(rec);
- Line 232: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(rec);
  Confidence: band=high; score=0.74
- Line 232: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(rec);
  Confidence: band=high; score=0.74
- Line 232: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(rec);
  Confidence: band=high; score=0.74
- Line 233: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(rec);

### src/voice/voice_tts_customizer.cpp
Total findings: 19

- Line 100: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = profiles_.find(voice_id);
- Line 297: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const std::set<std::string>* allowed = (it != allowed_map.end()) ? &it->second : nullptr;
- Line 297: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: const std::set<std::string>* allowed = (it != allowed_map.end()) ? &it->second : nullptr;
- Line 297: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: const std::set<std::string>* allowed = (it != allowed_map.end()) ? &it->second : nullptr;
- Line 353: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: bool attr_allowed = (allowed != nullptr) && (allowed->count(attr_name_lower) > 0);
- Line 108: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(p);
  Confidence: band=high; score=0.74
- Line 109: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(p);
- Line 119: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(p);
  Confidence: band=high; score=0.74
- Line 120: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(p);
- Line 228: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!last_space) { collapsed += ' '; last_space = true; }
  Confidence: band=high; score=0.74
- Line 228: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!last_space) { collapsed += ' '; last_space = true; }
  Confidence: band=high; score=0.74
- Line 229: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (!last_space) { collapsed += ' '; last_space = true; }
- Line 258: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<std::string, std::set<std::string>>& ssmlAllowedAttrs() {
  Confidence: band=high; score=0.74
- Line 259: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: static const std::map<std::string, std::set<std::string>> kAttrs = {
  Confidence: band=high; score=0.74
- Line 474: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: energies.push_back(static_cast<float>(std::sqrt(e / static_cast<double>(window))));
  Confidence: band=high; score=0.74
- Line 474: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: energies.push_back(static_cast<float>(std::sqrt(e / static_cast<double>(window))));
  Confidence: band=high; score=0.74
- Line 475: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: energies.push_back(static_cast<float>(std::sqrt(e / static_cast<double>(window))));
- Line 547: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(lv);
  Confidence: band=high; score=0.74
- Line 548: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(lv);

### src/voice/voice_authenticator.cpp
Total findings: 18

- Line 208: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = profiles_.find(pid);
- Line 337: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authenticate" without audit log
  Context: VoiceAuthResult VoiceBiometricAuthenticator::authenticate(
  Confidence: band=very_high; score=0.99
- Line 401: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = profiles_.find(profile_id);
- Line 0: severity=HIGH; category=uncategorized
  Context: ['        size_t start = static_cast<size_t>(b) * band_size;', '        size_t end   = (b == kBands - 1) ? n : start + band_size;', '        size_t len   = end - start;', '', '        float rms = 0.0f;']
  Confidence: band=high; score=0.81
- Line 7: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR: #2605 [voice] Implement speaker verification for voice biometrics + audit... (2026-03-12T05:52
- Line 118: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& fv : feature_vectors) {
  Confidence: band=very_high; score=0.9
- Line 119: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t i = 0; i < dim; ++i) {
  Confidence: band=very_high; score=0.9
- Line 124: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (float& v : mean_fv) {
  Confidence: band=very_high; score=0.9
- Line 421: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& kv : profiles_) {
  Confidence: band=very_high; score=0.9
- Line 443: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 473: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: *   Index  16:     Spectral centroid (normalised to [0,1])
- Line 478: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: *   Index  21:     Crest factor (normalised to [0,1])
- Line 217: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.matches.push_back(m);
  Confidence: band=high; score=0.74
- Line 218: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.matches.push_back(m);
- Line 421: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ids.push_back(kv.first);
  Confidence: band=high; score=0.74
- Line 422: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ids.push_back(kv.first);
- Line 287: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: log_sum   += std::log(mag);
  Confidence: band=medium; score=0.6
- Line 598: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: log_geo  += std::log(e);
  Confidence: band=medium; score=0.6

### src/voice/voice_model_cache.cpp
Total findings: 18

- Line 56: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (config_.pin_frequently_used && it->second.use_count >= config_.pin_threshold) {
- Line 151: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = models_.find(model_id);
- Line 155: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator unloader_it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto unloader_it = unloaders_.find(it->second.model_type);
- Line 163: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator lru_it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto lru_it = lru_map_.find(model_id);
- Line 205: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator unloader_it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto unloader_it = unloaders_.find(model.model_type);
- Line 269: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = lru_map_.find(model_id);
- Line 281: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = models_.find(id);
- Line 284: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator unloader_it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto unloader_it = unloaders_.find(it->second.model_type);
- Line 79: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: void* handle = loader_it->second(model_path, config);
- Line 156: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (unloader_it != unloaders_.end() && it->second.handle) {
- Line 157: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: unloader_it->second(it->second.handle);
- Line 204: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& [id, model] : models_) {
  Confidence: band=very_high; score=0.9
- Line 207: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: unloader_it->second(model.handle);
- Line 232: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [id, model] : models_) {
  Confidence: band=very_high; score=0.9
- Line 285: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (unloader_it != unloaders_.end() && it->second.handle) {
- Line 286: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: unloader_it->second(it->second.handle);
- Line 259: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: models_arr.push_back(m);
  Confidence: band=high; score=0.74
- Line 260: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: models_arr.push_back(m);

### src/voice/voice_batch_processor.cpp
Total findings: 17

- Line 117: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t i = 0; i < items.size(); i += batch_size) {
  Confidence: band=very_high; score=0.9
- Line 119: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t j = i; j < end; ++j) {
  Confidence: band=very_high; score=0.9
- Line 302: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t i = 0; i < num_concurrent; ++i) {
  Confidence: band=very_high; score=0.9
- Line 315: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = job_summaries_.find(job_id);
- Line 119: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(processItem(items[j]));
  Confidence: band=high; score=0.74
- Line 119: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(processItem(items[j]));
  Confidence: band=high; score=0.74
- Line 120: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(processItem(items[j]));
- Line 304: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: items.push_back(std::move(item));
  Confidence: band=high; score=0.74
- Line 305: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: items.push_back(std::move(item));
- Line 362: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (!word.empty()) tokens.push_back(word);
- Line 387: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: frame_rms.push_back(std::sqrt(sum / static_cast<float>(window_size)));
  Confidence: band=high; score=0.74
- Line 387: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: frame_rms.push_back(std::sqrt(sum / static_cast<float>(window_size)));
  Confidence: band=high; score=0.74
- Line 387: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: frame_rms.push_back(std::sqrt(sum / static_cast<float>(window_size)));
  Confidence: band=high; score=0.74
- Line 388: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: frame_rms.push_back(std::sqrt(sum / static_cast<float>(window_size)));
- Line 409: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: samples.push_back((static_cast<float>(b) - 128.0f) / 128.0f);
- Line 419: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: samples.push_back(static_cast<float>(s) / 32768.0f);
  Confidence: band=high; score=0.74
- Line 420: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: samples.push_back(static_cast<float>(s) / 32768.0f);

### src/voice/voice_meeting_support.cpp
Total findings: 17

- Line 7: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR: #3423 [WIP] Add real-time meeting transcription with action item extraction (2026-03-12T07:12:
- Line 51: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (lower.find(toLower(t)) != std::string::npos) return true;
- Line 51: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (lower.find(toLower(t)) != std::string::npos) return true;
- Line 148: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: size_t pos = lower.find(pat);
- Line 148: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: size_t pos = lower.find(pat);
- Line 70: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (!current.empty()) sentences.push_back(current);
  Confidence: band=high; score=0.74
- Line 70: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (!current.empty()) sentences.push_back(current);
  Confidence: band=high; score=0.74
- Line 71: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (!current.empty()) sentences.push_back(current);
- Line 79: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (!current.empty()) sentences.push_back(current);
- Line 180: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: items.push_back(std::move(ai));
  Confidence: band=high; score=0.74
- Line 181: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: items.push_back(std::move(ai));
- Line 193: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: decisions.push_back(sent);
  Confidence: band=high; score=0.74
- Line 194: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: decisions.push_back(sent);
- Line 210: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: points.push_back(sent);
  Confidence: band=high; score=0.74
- Line 211: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: points.push_back(sent);
- Line 249: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: protocol.segments.push_back(seg);
  Confidence: band=high; score=0.74
- Line 250: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: protocol.segments.push_back(seg);

### src/voice/voice_assistant_llm.cpp
Total findings: 15

- Line 32: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const std::string& user_input,
  Confidence: band=very_high; score=0.99
- Line 46: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: prompt << "User: " << user_input << "\n";
  Confidence: band=very_high; score=0.99
- Line 30: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Replace generateLLMResponse to use EmbeddedLLM instead of inference engine
  Confidence: band=very_high; score=0.9
- Line 32: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const std::string& user_input,
  Confidence: band=very_high; score=0.9
- Line 46: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: prompt << "User: " << user_input << "\n";
  Confidence: band=very_high; score=0.9
- Line 49: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Use EmbeddedLLM instead of inference engine
  Confidence: band=very_high; score=0.9
- Line 51: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::string response = THEMIS_LLM_GENERATE(prompt.str());
  Confidence: band=very_high; score=0.9
- Line 103: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::string response = THEMIS_LLM_GENERATE(prompt.str());
  Confidence: band=very_high; score=0.9
- Line 142: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::string response = THEMIS_LLM_GENERATE(prompt.str());
  Confidence: band=very_high; score=0.9
- Line 51: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: std::string response = THEMIS_LLM_GENERATE(prompt.str());
  Confidence: band=high; score=0.74
- Line 77: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: std::string summary = THEMIS_LLM_GENERATE(prompt.str());
  Confidence: band=high; score=0.74
- Line 103: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: std::string response = THEMIS_LLM_GENERATE(prompt.str());
  Confidence: band=high; score=0.74
- Line 116: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: key_points.push_back(line);
- Line 142: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: std::string response = THEMIS_LLM_GENERATE(prompt.str());
  Confidence: band=high; score=0.74
- Line 158: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: action_items.push_back(item);

### src/voice/voice_session_manager.cpp
Total findings: 15

- Line 48: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = store_.find(session_id);
- Line 190: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = active_cache_.find(session_id);
- Line 227: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = active_cache_.find(session_id);
- Line 249: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: for (auto it = active_cache_.begin(); it != active_cache_.end(); ) {
- Line 62: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [id, _] : store_) {
  Confidence: band=very_high; score=0.9
- Line 171: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (it->second.context.is_object() && context_update.is_object()) {
- Line 172: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [k, v] : context_update.items()) {
  Confidence: band=very_high; score=0.9
- Line 218: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: it->second.preferred_language = language_code;
- Line 249: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto it = active_cache_.begin(); it != active_cache_.end(); ) {
  Confidence: band=very_high; score=0.9
- Line 263: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [id, session] : active_cache_) {
  Confidence: band=very_high; score=0.9
- Line 62: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ids.push_back(id);
  Confidence: band=high; score=0.74
- Line 63: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ids.push_back(id);
- Line 263: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (session.user_id == user_id) result.push_back(session);
  Confidence: band=high; score=0.74
- Line 263: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (session.user_id == user_id) result.push_back(session);
  Confidence: band=high; score=0.74
- Line 264: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (session.user_id == user_id) result.push_back(session);

### src/voice/voice_security.cpp
Total findings: 14

- Line 266: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: data["consent"]["recording"]    = it->second.recording_consent;
- Line 213: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto it = audit_log_.rbegin(); it != audit_log_.rend(); ++it) {
  Confidence: band=very_high; score=0.9
- Line 262: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: data["user_id"] = user_id;
- Line 272: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& entry : audit_log_) {
  Confidence: band=very_high; score=0.9
- Line 281: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: data["audit_log"] = audit_entries;
- Line 93: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.found_pii.emplace_back(type, matched);
  Confidence: band=high; score=0.74
- Line 93: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.found_pii.emplace_back(type, matched);
  Confidence: band=high; score=0.74
- Line 121: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cumulative.found_pii.push_back(p);
  Confidence: band=high; score=0.74
- Line 121: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cumulative.found_pii.push_back(p);
  Confidence: band=high; score=0.74
- Line 122: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: cumulative.found_pii.push_back(p);
- Line 214: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(*it);
  Confidence: band=high; score=0.74
- Line 215: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(*it);
- Line 278: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: audit_entries.push_back(e);
  Confidence: band=high; score=0.74
- Line 279: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: audit_entries.push_back(e);

### src/voice/voice_intent_detector.cpp
Total findings: 13

- Line 62: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = entities_.find(key);
- Line 201: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ent.start_offset = static_cast<int>(it->position());
- Line 202: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ent.end_offset   = static_cast<int>(it->position() + it->length());
- Line 114: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (lower.find(kw) != std::string::npos) return true;
- Line 151: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (lower.find(kw) != std::string::npos) ++hits;
  Confidence: band=very_high; score=0.9
- Line 259: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto last_entity = context->getEntity("last_object");
- Line 284: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: result.requires_context = (context != nullptr && !context->getHistory().empty());
- Line 183: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entities.push_back(ent);
  Confidence: band=high; score=0.74
- Line 184: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: entities.push_back(ent);
- Line 202: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entities.push_back(ent);
  Confidence: band=high; score=0.74
- Line 203: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: entities.push_back(ent);
- Line 225: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entities.push_back(ent);
  Confidence: band=high; score=0.74
- Line 226: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: entities.push_back(ent);

### src/voice/wake_word_detector.cpp
Total findings: 12

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 97: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& ww : wake_words_) {
  Confidence: band=very_high; score=0.9
- Line 125: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& ww : wake_words_) {
  Confidence: band=very_high; score=0.9
- Line 230: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 71: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: tokens.push_back(tok);
  Confidence: band=high; score=0.74
- Line 72: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back(tok);
- Line 105: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: wake_words_.push_back(std::move(ww));
  Confidence: band=high; score=0.74
- Line 106: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: wake_words_.push_back(std::move(ww));
- Line 125: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ids.push_back(ww.id);
  Confidence: band=high; score=0.74
- Line 126: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ids.push_back(ww.id);
- Line 153: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: sample_buffer_.push_back(s);
  Confidence: band=high; score=0.74
- Line 154: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: sample_buffer_.push_back(s);

### src/voice/emotion_analyzer.cpp
Total findings: 11

- Line 18: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * 1. Convert raw 16-bit LE PCM bytes to normalised float samples [-1, +1].
- Line 187: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: timeline.timeline.push_back(te);
  Confidence: band=high; score=0.74
- Line 188: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: timeline.timeline.push_back(te);
- Line 189: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: stress_levels.push_back(result->stress_level);
- Line 190: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: engagement_scores.push_back(result->engagement_score);
- Line 402: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: /*static*/ std::map<Emotion, float> EmotionAnalyzer::softmax(
  Confidence: band=high; score=0.74
- Line 403: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<Emotion, float>& raw)
  Confidence: band=high; score=0.74
- Line 411: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<Emotion, float> out;
  Confidence: band=high; score=0.74
- Line 431: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<Emotion, float>& probs) const
  Confidence: band=high; score=0.74
- Line 434: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: static const std::map<Emotion, float> kPolarity = {
  Confidence: band=high; score=0.74
- Line 300: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: log_geo += std::log(e);
  Confidence: band=medium; score=0.6

### src/voice/voice_browser_streaming.cpp
Total findings: 8

- Line 0: severity=CRITICAL; category=uncategorized
  Confidence: band=very_high; score=0.85
- Line 162: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: new VoiceStreamingSession(std::move(config)));
- Line 340: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = sessions_.find(stream_id);
- Line 137: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 141: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 219: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: impl_->audio_buffer.insert(impl_->audio_buffer.end(),
- Line 251: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: impl_->audio_buffer,
- Line 253: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: impl_->audio_buffer.clear();

### src/voice/audio_preprocessing.cpp
Total findings: 4

- Line 126: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t i = 0; i < avail; ++i)
  Confidence: band=very_high; score=0.9
- Line 128: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t i = avail; i < static_cast<size_t>(kRNNoiseFrameSamples); ++i)
  Confidence: band=very_high; score=0.9
- Line 131: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: float vad_prob = rnnoise_process_frame(impl_->state, buf.data(), buf.data());
- Line 137: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t i = 0; i < avail; ++i)
  Confidence: band=very_high; score=0.9

### src/voice/voice_error_handler.cpp
Total findings: 3

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 166: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(ms));
- Line 281: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: json status;

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
