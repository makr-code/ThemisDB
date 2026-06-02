# voice Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: voice
- Generated: 2026-06-02 11:09:13
- Status: Critical Findings Present
- Total Findings: 107
- Actionable Findings (Critical + High): 15
- Affected Files: 18

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 7 |
| High | 8 |
| Medium | 89 |
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
| src/voice/voice_macro_manager.cpp | 19 | 0 | 0 | 19 | 0 |
| src/voice/voice_assistant_llm.cpp | 13 | 3 | 6 | 4 | 0 |
| src/voice/voice_tts_customizer.cpp | 9 | 0 | 0 | 9 | 0 |
| src/voice/voice_accessibility.cpp | 8 | 0 | 0 | 8 | 0 |
| src/voice/emotion_analyzer.cpp | 7 | 0 | 0 | 6 | 1 |
| src/voice/voice_batch_processor.cpp | 7 | 0 | 0 | 7 | 0 |
| src/voice/voice_meeting_support.cpp | 6 | 0 | 0 | 6 | 0 |
| src/voice/voice_security.cpp | 6 | 0 | 0 | 6 | 0 |
| src/voice/voice_assistant.cpp | 5 | 3 | 0 | 2 | 0 |
| src/voice/voice_audio_storage.cpp | 5 | 0 | 1 | 4 | 0 |
| src/voice/voice_authenticator.cpp | 5 | 1 | 0 | 2 | 2 |
| src/voice/voice_telephony.cpp | 5 | 0 | 0 | 5 | 0 |
| src/voice/voice_intent_detector.cpp | 4 | 0 | 1 | 3 | 0 |
| src/voice/wake_word_detector.cpp | 4 | 0 | 0 | 4 | 0 |
| src/voice/voice_session_manager.cpp | 3 | 0 | 0 | 3 | 0 |
| src/voice/voice_model_cache.cpp | 1 | 0 | 0 | 1 | 0 |
| src/voice/voice_browser_streaming.cpp | 0 | 0 | 0 | 0 | 0 |
| src/voice/voice_error_handler.cpp | 0 | 0 | 0 | 0 | 0 |

## Full Scanner Findings

### src/voice/voice_macro_manager.cpp
Total findings: 19

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
- Line 119: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: steps.push_back(stepToJson(s));
  Confidence: band=high; score=0.74
- Line 150: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: m.steps.push_back(stepFromJson(s));
  Confidence: band=high; score=0.74
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
- Line 455: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(macroInfoToJson(it->second));
  Confidence: band=high; score=0.74
- Line 491: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: imported_ids.push_back(id);
  Confidence: band=high; score=0.74

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
Total findings: 9

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
- Line 545: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(lv);
  Confidence: band=high; score=0.74

### src/voice/voice_accessibility.cpp
Total findings: 8

- Line 60: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cues.push_back(std::move(cue));
  Confidence: band=high; score=0.74
- Line 88: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cues.push_back(std::move(cue));
  Confidence: band=high; score=0.74
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
- Line 367: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: last.text += " " + sorted[i].text;
  Confidence: band=high; score=0.74
- Line 370: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(sorted[i]);
  Confidence: band=high; score=0.74

### src/voice/emotion_analyzer.cpp
Total findings: 7

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

### src/voice/voice_batch_processor.cpp
Total findings: 7

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
- Line 417: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: samples.push_back(static_cast<float>(s) / 32768.0f);
  Confidence: band=high; score=0.74

### src/voice/voice_meeting_support.cpp
Total findings: 6

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

### src/voice/voice_security.cpp
Total findings: 6

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
- Line 276: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: audit_entries.push_back(e);
  Confidence: band=high; score=0.74

### src/voice/voice_assistant.cpp
Total findings: 5

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
- Line 413: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: segments.push_back(seg_json);
  Confidence: band=high; score=0.74
- Line 465: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: participants.push_back(participant);
  Confidence: band=high; score=0.74

### src/voice/voice_audio_storage.cpp
Total findings: 5

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

### src/voice/voice_authenticator.cpp
Total findings: 5

- Line 335: severity=CRITICAL; category=audit_logging; pattern=missing_audit_log
  Description: Security function "authenticate" without audit log
  Context: VoiceAuthResult VoiceBiometricAuthenticator::authenticate(
  Confidence: band=very_high; score=0.99
- Line 215: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.matches.push_back(m);
  Confidence: band=high; score=0.74
- Line 430: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ids.push_back(kv.first);
  Confidence: band=high; score=0.74
- Line 285: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: log_sum   += std::log(mag);
  Confidence: band=medium; score=0.6
- Line 635: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: log_geo  += std::log(e);
  Confidence: band=medium; score=0.6

### src/voice/voice_telephony.cpp
Total findings: 5

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
- Line 385: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: packets.push_back(std::move(pkt));
  Confidence: band=high; score=0.74
- Line 598: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: packets.push_back(std::move(pkt));
  Confidence: band=high; score=0.74

### src/voice/voice_intent_detector.cpp
Total findings: 4

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

### src/voice/wake_word_detector.cpp
Total findings: 4

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
Total findings: 3

- Line 61: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ids.push_back(id);
  Confidence: band=high; score=0.74
- Line 274: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (session.user_id == user_id) result.push_back(session);
  Confidence: band=high; score=0.74
- Line 274: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (session.user_id == user_id) result.push_back(session);
  Confidence: band=high; score=0.74

### src/voice/voice_model_cache.cpp
Total findings: 1

- Line 257: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: models_arr.push_back(m);
  Confidence: band=high; score=0.74

### src/voice/voice_browser_streaming.cpp
Total findings: 0


### src/voice/voice_error_handler.cpp
Total findings: 0

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
