# rag Module - Developer Gap Note

> Auto-generated from ai_working\gap_scan_results.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: rag
- Generated: 2026-06-04 08:50:22
- Status: Critical Findings Present
- Total Findings: 557
- Actionable Findings (Critical + High): 248
- Affected Files: 60

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 73 |
| High | 175 |
| Medium | 245 |
| Low | 64 |

## Category Summary

| Category | Count |
|---|---:|
| hardcoded_output | 58 |
| hardcoded_path | 54 |
| unordered_container_iter | 48 |
| data_race | 33 |
| copy_overhead | 32 |
| string_concat_loop | 30 |
| pointer_arithmetic_unbounded | 26 |
| nested_loop_find | 25 |
| uninitialized_access | 22 |
| resource_leaked_in_exception | 19 |
| missing_resource_limits | 17 |
| o_n_squared | 15 |
| command_injection | 13 |
| uncaught_exception | 12 |
| generic_catch | 10 |
| missing_move_constructor_defaulted | 10 |
| model_integrity_gap | 10 |
| range_temporary | 10 |
| null_dereference | 8 |
| unvalidated_llm_output | 8 |
| missing_latency_metric | 7 |
| no_timeout | 7 |
| blocking_no_timeout | 6 |
| fp_exact_comparison | 5 |
| stale_doc_section_reference | 5 |
| unstructured_log | 5 |
| missing_trace_point | 4 |
| missing_vector_reserve | 4 |
| no_retry_logic | 4 |
| smart_ptr_misuse | 4 |
| thread_join_no_timeout | 4 |
| uninitialized_member_field | 4 |
| iterator_invalidation | 3 |
| legacy_or_compat_path | 3 |
| lock_contention | 3 |
| primitive_no_volatile | 3 |
| sensitive_data_logging | 3 |
| unnecessary_copy | 3 |
| expensive_copy | 2 |
| expensive_inner_op | 2 |
| prompt_injection | 2 |
| unspecified_consistency | 2 |
| arithmetic_overflow | 1 |
| db_connection_leak | 1 |
| exception_in_destructor | 1 |
| explicit_lock_unlock | 1 |
| lock_in_loop | 1 |
| manual_cleanup | 1 |
| module_doc_linkset_drift | 1 |
| random_unseeded | 1 |
| regex_in_loop | 1 |
| shift_overflow | 1 |
| size_assumption | 1 |
| timestamp_sorting_unstable | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| rag/evaluation_report_exporter.cpp | 62 | 0 | 3 | 58 | 1 |
| rag/knowledge_gap_detector.cpp | 46 | 2 | 29 | 13 | 2 |
| rag/batch_evaluator.cpp | 33 | 1 | 8 | 3 | 21 |
| rag/continuous_learning_orchestrator.cpp | 25 | 9 | 7 | 9 | 0 |
| rag/rag_judge.cpp | 23 | 0 | 11 | 11 | 1 |
| rag/examples/loop_orchestration_example.cpp | 22 | 0 | 2 | 4 | 16 |
| rag/rag_ingestion_bridge.cpp | 21 | 1 | 20 | 0 | 0 |
| rag/knowledge_graph_retriever.cpp | 17 | 0 | 5 | 11 | 1 |
| rag/reranker.cpp | 17 | 8 | 4 | 3 | 2 |
| rag/agentic_rag.cpp | 16 | 2 | 9 | 5 | 0 |
| rag/continuous_learning_client.cpp | 14 | 4 | 5 | 5 | 0 |
| rag/document_summarizer.cpp | 14 | 1 | 3 | 10 | 0 |
| rag/multi_step_rag.cpp | 14 | 2 | 3 | 9 | 0 |
| rag/dpr_vectorizer.cpp | 13 | 5 | 1 | 3 | 4 |
| rag/distributed_rag_evaluator.cpp | 12 | 0 | 6 | 3 | 3 |
| rag/rlaif_trainer.cpp | 12 | 7 | 3 | 2 | 0 |
| rag/delegate_evaluator.cpp | 11 | 0 | 0 | 7 | 4 |
| rag/fairness_detector.cpp | 10 | 1 | 7 | 2 | 0 |
| rag/response_parser.cpp | 10 | 0 | 3 | 6 | 1 |
| rag/prompt_injection_detector.cpp | 9 | 0 | 2 | 7 | 0 |
| rag/llm_integration.cpp | 8 | 0 | 2 | 5 | 1 |
| rag/llm_judge_client.cpp | 8 | 3 | 3 | 2 | 0 |
| rag/llm_judge_integration.cpp | 8 | 1 | 1 | 5 | 1 |
| rag/multimodal_rag.cpp | 8 | 4 | 1 | 3 | 0 |
| rag/http_metrics_client.cpp | 7 | 6 | 0 | 1 | 0 |
| rag/quality_control_pipeline.cpp | 7 | 3 | 1 | 3 | 0 |
| rag/bayesian_optimizer.cpp | 6 | 0 | 1 | 5 | 0 |
| rag/claim_extractor.cpp | 6 | 0 | 3 | 3 | 0 |
| rag/cot_evaluator.cpp | 6 | 0 | 4 | 2 | 0 |
| rag/replug_retriever.cpp | 6 | 0 | 4 | 2 | 0 |
| rag/coherence_evaluator.cpp | 5 | 0 | 2 | 3 | 0 |
| rag/llm_meta_analyzer.cpp | 5 | 0 | 3 | 1 | 1 |
| rag/lora_enhanced_retriever.cpp | 5 | 2 | 1 | 2 | 0 |
| rag/multi_hop_reasoner.cpp | 5 | 0 | 1 | 4 | 0 |
| rag/relevance_evaluator.cpp | 5 | 0 | 2 | 3 | 0 |
| rag/tensor_rag_pipeline.cpp | 5 | 0 | 1 | 3 | 1 |
| rag/flare_retrieval.cpp | 4 | 2 | 0 | 1 | 1 |
| rag/geval_evaluator.cpp | 4 | 1 | 0 | 3 | 0 |
| rag/ontology_aware_retriever.cpp | 4 | 0 | 2 | 2 | 0 |
| rag/pairwise_comparator.cpp | 4 | 2 | 0 | 2 | 0 |
| rag/adversarial_tester.cpp | 3 | 0 | 0 | 3 | 0 |
| rag/calibration_manager.cpp | 3 | 1 | 0 | 1 | 1 |
| rag/citation_highlighter.cpp | 3 | 0 | 2 | 1 | 0 |
| rag/completeness_evaluator.cpp | 3 | 0 | 1 | 2 | 0 |
| rag/explainability_reason_builder.cpp | 3 | 1 | 1 | 1 | 0 |
| rag/hybrid_retriever.cpp | 3 | 0 | 2 | 1 | 0 |
| rag/onnx_model_loader.cpp | 3 | 1 | 1 | 1 | 0 |
| rag/self_rag.cpp | 3 | 0 | 0 | 3 | 0 |
| rag/faithfulness_evaluator.cpp | 2 | 1 | 0 | 1 | 0 |
| rag/learning_metrics.cpp | 2 | 0 | 0 | 2 | 0 |
| rag/prompt_templates.cpp | 2 | 1 | 0 | 1 | 0 |
| rag/streaming_retriever.cpp | 2 | 0 | 1 | 1 | 0 |
| rag/PRODUCTION_REQUIREMENTS.md | 1 | 0 | 0 | 0 | 1 |
| rag/bias_detector.cpp | 1 | 0 | 0 | 1 | 0 |
| rag/hallucination_dashboard.cpp | 1 | 0 | 1 | 0 | 0 |
| rag/judge_config.cpp | 1 | 0 | 1 | 0 | 0 |
| rag/judge_ensemble.cpp | 1 | 0 | 1 | 0 | 0 |
| rag/nli_faithfulness_verifier.cpp | 1 | 1 | 0 | 0 | 0 |
| rag/rubric_evaluator.cpp | 1 | 0 | 1 | 0 | 0 |
| rag/targ_retrieval.cpp | 1 | 0 | 0 | 0 | 1 |

## Full Scanner Findings

### rag/evaluation_report_exporter.cpp
Total findings: 62

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #3353 [rag] Per-query evaluation ... (2026-03-12)
- Line 46: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: std::snprintf(buf, sizeof(buf), "\\u%04x",
- Line 237: severity=HIGH; category=arithmetic_overflow
  Description: Arithmetic operation result assigned to variable (potential overflow)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['    os << "<!DOCTYPE html>\\n<html lang=\\"en\\">\\n<head>\\n"', '       << "<meta charset=\\"UTF-8\\">\\n"', '       << "<meta name=\\"viewport\\" content=\\"width=device-width,initial-scale=1\\">\\n"', '       << "<title>RAG Evaluation Report";', '    if (!report.report_id.empty())']
- Line 34: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: case '"':  out += "\\\""; break;
- Line 35: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '"':  out += "\\\""; break;
- Line 36: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '\\': out += "\\\\"; break;
- Line 37: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '\b': out += "\\b";  break;
- Line 38: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '\f': out += "\\f";  break;
- Line 39: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '\n': out += "\\n";  break;
- Line 40: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '\r': out += "\\r";  break;
- Line 41: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '\t': out += "\\t";  break;
- Line 46: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::snprintf(buf, sizeof(buf), "\\u%04x",
- Line 62: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: case '&':  out += "&amp;";  break;
- Line 63: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '&':  out += "&amp;";  break;
- Line 64: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '<':  out += "&lt;";   break;
- Line 65: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '>':  out += "&gt;";   break;
- Line 66: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '"':  out += "&quot;"; break;
- Line 67: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '\'': out += "&#39;";  break;
- Line 95: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: << "<span class=\"score-label\">" << escapeHTML(label) << "</span>"
- Line 98: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: << "%;background:" << colour << ";\"></div>"
- Line 102: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: << "</div>\n";
- Line 241: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: os << "</title>\n"
- Line 262: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: << "</style>\n"
- Line 263: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: << "</head>\n<body>\n";
- Line 268: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: os << " <span class=\"meta\">(" << escapeHTML(report.report_id) << ")</span>";
- Line 269: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: os << "</h1>\n";
- Line 275: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: << std::fixed << std::setprecision(3) << res.confidence << "</strong></p>\n";
- Line 279: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: << pass_label << "</span></p>\n";
- Line 283: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: << "<h2>Query</h2>\n"
- Line 284: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: << "<p>" << escapeHTML(inp.query) << "</p>\n"
- Line 285: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: << "<h2>Generated Answer</h2>\n"
- Line 286: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: << "<p>" << escapeHTML(inp.generated_answer) << "</p>\n"
- Line 287: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: << "</div>\n";
- Line 290: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: os << "<div class=\"card\">\n<h2>Dimension Scores</h2>\n";
- Line 298: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: os << "</div>\n";
- Line 301: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: os << "<div class=\"card\">\n<h2>Claims</h2>\n";
- Line 304: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: os << "<p><strong>✓ Verified Claims</strong></p>\n<ul class=\"claims\">\n";
- Line 306: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: os << "<li class=\"verified\">" << escapeHTML(c) << "</li>\n";
- Line 307: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: os << "</ul>\n";
- Line 311: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: os << "<p><strong>✗ Unverified Claims</strong></p>\n<ul class=\"claims\">\n";
- Line 313: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: os << "<li class=\"unverified\">" << escapeHTML(c) << "</li>\n";
- Line 314: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: os << "</ul>\n";
- Line 318: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: os << "<p class=\"meta\">No claims extracted.</p>\n";
- Line 320: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: os << "</div>\n";
- Line 323: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: os << "<div class=\"card\">\n<h2>Ethical Compliance</h2>\n"
- Line 325: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: << (res.respects_human_autonomy ? "Yes" : "No") << "</strong></p>\n"
- Line 327: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: << (res.shows_moral_diversity ? "Yes" : "No") << "</strong></p>\n"
- Line 329: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: << (res.has_ethical_citations ? "Yes" : "No") << "</strong></p>\n";
- Line 332: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: os << "<p><strong>Ethical Violations:</strong></p>\n<ul>\n";
- Line 334: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: os << "<li style=\"color:#c62828\">" << escapeHTML(v) << "</li>\n";
- Line 335: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: os << "</ul>\n";
- Line 337: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: os << "</div>\n";
- Line 341: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: os << "<div class=\"card\">\n<h2>Suggested Improvements</h2>\n<ul>\n";
- Line 343: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: os << "<li>" << escapeHTML(imp) << "</li>\n";
- Line 344: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: os << "</ul>\n</div>\n";
- Line 349: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: os << "<div class=\"card\">\n<h2>Explanation</h2>\n"
- Line 351: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: << "</div>\n</div>\n";
- Line 357: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: << inp.documents.size() << ")</h2>\n";
- Line 365: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: << "</div>\n";
- Line 367: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: os << "</div>\n";
- Line 370: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: os << "</body>\n</html>\n";
- Line 46: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::snprintf(buf, sizeof(buf), "\\u%04x",

### rag/knowledge_gap_detector.cpp
Total findings: 46

- Line 426: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: avg_similarity >= impl_->config.similarity_threshold) {
- Line 477: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: THEMIS_DEBUG("No new documents retrieved, stopping");
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #2576 RAG Phase 2: Wire streaming... (2026-03-12) | #655 [RAG-GAP-P2] Impleme
- Line 208: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Fallback to legacy checks if token_probs not available
- Line 218: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Check perplexity (legacy)
- Line 459: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 462: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 476: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 477: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 855: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (content_lower.find(term) != std::string::npos) {
- Line 1098: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (auto& sent : splitSentences(docs[d].content)) {
- Line 1334: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (content_lower.find(term) != std::string::npos) {
- Line 1357: severity=HIGH; category=command_injection
  Description: sql_injection_concat: SQL injection risk — use prepared statements
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: return original_query + " " + missing_info;
- Line 1504: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (lower_content.find(framework) != std::string::npos) {
- Line 1506: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (framework.find("utilitarian") != std::string::npos ||
- Line 1507: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: framework.find("consequentialist") != std::string::npos ||
- Line 1508: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: framework.find("utility") != std::string::npos) {
- Line 1510: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: } else if (framework.find("deontological") != std::string::npos ||
- Line 1511: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: framework.find("kant") != std::string::npos ||
- Line 1512: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: framework.find("duty") != std::string::npos) {
- Line 1514: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: } else if (framework.find("virtue") != std::string::npos ||
- Line 1515: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: framework.find("aristotle") != std::string::npos ||
- Line 1516: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: framework.find("character") != std::string::npos) {
- Line 1518: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: } else if (framework.find("rights") != std::string::npos) {
- Line 1520: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: } else if (framework.find("care") != std::string::npos ||
- Line 1521: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: framework.find("feminist") != std::string::npos) {
- Line 1523: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: } else if (framework.find("religious") != std::string::npos ||
- Line 1524: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: framework.find("divine") != std::string::npos ||
- Line 1525: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: framework.find("faith") != std::string::npos) {
- Line 1527: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: } else if (framework.find("cultural") != std::string::npos ||
- Line 1528: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: framework.find("relativism") != std::string::npos) {
- Line 31: severity=MEDIUM; category=missing_move_constructor_defaulted
  Description: Missing move ctor prevents efficient rvalue transfers
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 530: severity=MEDIUM; category=missing_move_constructor_defaulted
  Description: Missing move ctor prevents efficient rvalue transfers
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 677: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: all_content += doc.content + " ";
- Line 805: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: return impl_->claim_verification_fn(claim, docs);

        } catch (const std::exception& e) {

            THEMIS_WARN("ClaimVerificationFn threw exception, falling back to heuristic: {}", e.what());

        } catch (...) {

            THEMIS_WARN("ClaimVerificationFn threw unknown exception, falling back to heuristic");

        }

    }
- Line 805: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 1021: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: averages.push_back(sum / window_size);
- Line 1068: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'KnowledgeGapDetector SelfConsistency.' that was not found in 'src/rag/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/rag/FUTURE_ENHANCEMENTS.md §KnowledgeGapDetector SelfConsistency.
- Line 1080: severity=MEDIUM; category=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: sentences.push_back(current.substr(s));
- Line 1081: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: sentences.push_back(current.substr(s));
- Line 1089: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: sentences.push_back(current.substr(s));
- Line 1286: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: sentences.push_back(current_sentence.substr(start));
- Line 1458: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: bool KnowledgeGapDetector::isEthicalQuery(const std::string& query) {
- Line 1496: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> found_frameworks;
- Line 885: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: log_sum += std::log(prob);
- Line 957: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: log_sum += std::log(prob);

### rag/batch_evaluator.cpp
Total findings: 33

- Line 675: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: if (w.joinable()) w.join();
- Line 147: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: return future_.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
- Line 161: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: EvaluationResult AsyncEvaluationHandle::get() {

    if (cancelled_.load()) {

        throw std::runtime_error("AsyncEvaluationHandle: evaluation was cancelled");

    }

    return future_.get();

}
- Line 212: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::unique_lock<std::mutex> lock(queue_mutex_);
- Line 323: severity=HIGH; category=command_injection
  Description: sql_injection_concat: SQL injection risk — use prepared statements
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: safe_input.query = "[BLOCKED_PROMPT]";
- Line 602: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: handle->cancelled_.store(false);
- Line 605: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: handle->future_ = promise.get_future();
- Line 656: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> lock(queue_mutex_);
- Line 663: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(5));
- Line 247: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: if (item.has_promise) {

                try {

                    item.promise.set_exception(std::current_exception());

                } catch (...) {}

            }

        }

    }
- Line 247: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {}
- Line 638: severity=MEDIUM; category=missing_move_constructor_defaulted
  Description: Missing move ctor prevents efficient rvalue transfers
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 263: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: // 1. Size validation (lines 260-287): Reject oversized inputs (DoS prevention)
- Line 354: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::vector<EvaluationInput> inputs;
- Line 355: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: inputs.reserve(test_cases.size());
- Line 361: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: inputs.push_back(std::move(in));
- Line 363: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: return evaluateBatch(inputs);
- Line 367: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: const std::vector<EvaluationInput>& inputs) {
- Line 370: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: if (inputs.size() > 10000) {
- Line 372: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: error_result.progress.total_items = inputs.size();
- Line 373: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: error_result.progress.failed_items = inputs.size();
- Line 374: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: THEMIS_ERROR("BatchEvaluator: Batch size exceeds maximum ({})", inputs.size());
- Line 382: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: results.reserve(inputs.size());
- Line 387: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: for (const auto& input : inputs) {
- Line 394: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: progress.total_items     = inputs.size();
- Line 399: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: static_cast<double>(inputs.size());
- Line 418: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: out.progress.total_items = inputs.size();
- Line 420: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: if (inputs.empty() || results.empty()) {
- Line 438: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: for (size_t i = 0; i < results.size() && i < inputs.size(); ++i) {
- Line 439: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: const auto& input = inputs[i];
- Line 623: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: const std::vector<EvaluationInput>& inputs) {
- Line 625: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: handles.reserve(inputs.size());
- Line 626: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: for (const auto& input : inputs) {

### rag/continuous_learning_orchestrator.cpp
Total findings: 25

- Line 172: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: impl_->learning_loop_active = true;
- Line 173: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: impl_->learning_thread = std::make_unique<std::thread>(&ContinuousLearningOrchestrator::learningLoop
- Line 181: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: impl_->learning_loop_active = false;
- Line 1227: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: #if defined(THEMIS_ENABLE_BAO)

        std::weak_ptr<themis::performance::phase3::BaoOptimizer> bao_weak = bao_optimizer;

        setHnswMissRateProvider([bao_weak]() {

            auto bao = bao_weak.lock();

            if (!bao) {

                throw std::runtime_error("BaoOptimizer unavailable");

            }
- Line 1227: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: auto bao = bao_weak.lock();
- Line 1247: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: std::weak_ptr<themis::performance::WorkloadAdaptiveOptimizer> workload_weak =

            workload_optimizer;

        setWorkloadDriftProvider([workload_weak]() {

            auto workload = workload_weak.lock();

            if (!workload) {

                throw std::runtime_error("WorkloadAdaptiveOptimizer unavailable");

            }
- Line 1247: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: auto workload = workload_weak.lock();
- Line 1262: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: if (feedback_collector) {

        std::weak_ptr<themis::prompt_engineering::FeedbackCollector> feedback_weak = feedback_collector;

        setFeedbackEntryCountProvider([feedback_weak]() {

            auto feedback = feedback_weak.lock();

            if (!feedback) {

                throw std::runtime_error("FeedbackCollector unavailable");

            }
- Line 1262: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: auto feedback = feedback_weak.lock();
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #3355 [rag] Online learning from ... (2026-03-12) | #1270 Implement Continuou
- Line 109: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 273: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 409: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 602: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 708: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::seconds(1));
- Line 1266: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 40: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: static constexpr double kUserFeedbackWeight  = 0.6;
- Line 42: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: static constexpr double kEvalConfidenceWeight = 0.4;
- Line 44: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: static constexpr double kDefaultObjectiveScore = 0.5;
- Line 318: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, size_t> total_per_version;
- Line 319: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, size_t> success_per_version;
- Line 1122: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if      (c == '"')  out += "\\\"";
- Line 1123: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: else if (c == '\\') out += "\\\\";
- Line 1124: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: else if (c == '\n') out += "\\n";
- Line 1130: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: static const std::unordered_map<int, std::string> kPhaseNames{

### rag/rag_judge.cpp
Total findings: 23

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #651 [RAG-ETHICS] Add ethical co... (2026-03-11) | #650 Complete RAG Enhancem
- Line 89: severity=HIGH; category=command_injection
  Description: sql_injection_concat: SQL injection risk — use prepared statements
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: return tenant_id + "\x1F" + query + "|" + answer;
- Line 91: severity=HIGH; category=command_injection
  Description: sql_injection_concat: SQL injection risk — use prepared statements
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: return query + "|" + answer;
- Line 328: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: impl_->injection_detector_initialized.load(std::memory_order_acquire) &&
- Line 428: severity=HIGH; category=missing_trace_point
  Description: Critical function query without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: "(max severity < HIGH) for query (len={})",
- Line 972: severity=HIGH; category=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto result = impl_->completeness_eval->evaluate(
- Line 987: severity=HIGH; category=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto result = impl_->coherence_eval->evaluate(input.generated_answer);
- Line 1220: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: "[",  // Citation markers like [1], [UN Declaration]
- Line 1317: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (trimmed.find(phrase) != std::string::npos) {
- Line 1318: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (trimmed.find(phrase) != std::string::npos) {
- Line 1472: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (doc.content.find(claim) != std::string::npos) {
- Line 45: severity=MEDIUM; category=missing_move_constructor_defaulted
  Description: Missing move ctor prevents efficient rvalue transfers
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 428: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: "(max severity < HIGH) for query (len={})",
- Line 570: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }

                } catch (const std::exception& e) {

                    THEMIS_WARN("RAGJudge claim verification pipeline failed: {}", e.what());

                } catch (...) {

                    THEMIS_WARN("RAGJudge claim verification pipeline failed with unknown exception");

                }

            }
- Line 570: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 638: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: dim_scores.push_back(result.faithfulness_score);
- Line 639: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: dim_scores.push_back(result.completeness_score);
- Line 640: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: dim_scores.push_back(result.coherence_score);
- Line 642: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: dim_scores.push_back(result.ethical_compliance_score);
- Line 644: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: dim_scores.push_back(result.relevance_score);  // always evaluated
- Line 857: severity=MEDIUM; category=missing_move_constructor_defaulted
  Description: Missing move ctor prevents efficient rvalue transfers
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1368: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> set2(terms2.begin(), terms2.end());
- Line 256: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: // 1. Size validation (lines 205-234): Reject oversized inputs (DoS prevention)

### rag/examples/loop_orchestration_example.cpp
Total findings: 22

- Line 53: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::cout << "\n  [ExplainabilityReasonBuilder — PLANNED]\n"
- Line 151: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::cout << "\n  [ExplainabilityReasonBuilder — PLANNED — IMPL-B9]\n"
- Line 44: severity=MEDIUM; category=uninitialized_member_field
  Description: Default constructor does not initialize POD members
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::uninitialized
  Context: Struct with uninitialized fields
- Line 147: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::cout << "    → " << step << "\n";
- Line 153: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: << "  Trigger  : QPS spike +3 200 req/s; p99 latency 85 ms\n"
- Line 169: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: << "\nSee docs/issues/ for implementation specs.\n";
- Line 53: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "\n  [ExplainabilityReasonBuilder — PLANNED]\n"
- Line 67: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "=== Loop 1–4 Orchestration Example (IMPL-A2 + IMPL-A3 + IMPL-B9) ===\n\n";
- Line 77: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "Step 1: Loop 1 — per-query BaoOptimizer feedback\n";
- Line 89: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "  triggerLoop1QueryExecution() called for query: " << outcome.query_id << "\n"
- Line 98: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "\nStep 2: Loop 2 — WorkloadAdaptiveOptimizer\n";
- Line 103: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "  triggerLoop2WorkloadAdaptation() called.\n"
- Line 109: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "\nStep 3: Loop 4 — IncrementalLoRATrainer\n";
- Line 113: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "  Training complete — adapter v" << orchestrator.activeAdapterVersion() << "\n";
- Line 115: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "  triggerLoop4AdapterImprovement() called.\n"
- Line 121: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "\nStep 4: FEDERATED_ROUND_START event\n";
- Line 131: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "  [PLANNED] FEDERATED_ROUND_START fires after Loop 4 completes\n"
- Line 145: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "  Summary: " << chain.summary << "\n";
- Line 147: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "    → " << step << "\n";
- Line 149: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "  Written to AIDecisionAuditor.\n";
- Line 151: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "\n  [ExplainabilityReasonBuilder — PLANNED — IMPL-B9]\n"
- Line 163: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "\n=== Summary ===\n"

### rag/rag_ingestion_bridge.cpp
Total findings: 21

- Line 56: severity=CRITICAL; category=exception_in_destructor
  Description: Destructors must be noexcept; exceptions here cause std::terminate()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 137: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: fallback_chunk.chunk_id = doc_id + "#0";

        fallback_chunk.source_file_id = doc_id;

        fallback_chunk.text_snippet = text;

        fallback_chunk.metadata["collection"] = collection;

        fallback_chunk.metadata["source"] = doc_id;

        entity_set.chunks.push_back(std::move(fallback_chunk));

    }
- Line 138: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: fallback_chunk.source_file_id = doc_id;

        fallback_chunk.text_snippet = text;

        fallback_chunk.metadata["collection"] = collection;

        fallback_chunk.metadata["source"] = doc_id;

        entity_set.chunks.push_back(std::move(fallback_chunk));

    }
- Line 153: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: chunk.source_file_id = doc_id;

            }

            // Inject canonical retrieval metadata for downstream RAG consumers.

            chunk.metadata["collection"] = collection;



            const std::string canonical_source =

                trimCopy(chunk.metadata["source"]).empty()
- Line 156: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: chunk.metadata["collection"] = collection;



            const std::string canonical_source =

                trimCopy(chunk.metadata["source"]).empty()

                    ? chunk.source_file_id

                    : trimCopy(chunk.metadata["source"]);

            const std::string canonical_content =
- Line 158: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: const std::string canonical_source =

                trimCopy(chunk.metadata["source"]).empty()

                    ? chunk.source_file_id

                    : trimCopy(chunk.metadata["source"]);

            const std::string canonical_content =

                trimCopy(chunk.metadata["content"]).empty()

                    ? trimCopy(chunk.text_snippet)
- Line 160: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: ? chunk.source_file_id

                    : trimCopy(chunk.metadata["source"]);

            const std::string canonical_content =

                trimCopy(chunk.metadata["content"]).empty()

                    ? trimCopy(chunk.text_snippet)

                    : trimCopy(chunk.metadata["content"]);
- Line 162: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: const std::string canonical_content =

                trimCopy(chunk.metadata["content"]).empty()

                    ? trimCopy(chunk.text_snippet)

                    : trimCopy(chunk.metadata["content"]);



            if (!canonical_source.empty()) {

                chunk.metadata["source"] = canonical_source;
- Line 165: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: : trimCopy(chunk.metadata["content"]);



            if (!canonical_source.empty()) {

                chunk.metadata["source"] = canonical_source;

            }

            if (!canonical_content.empty()) {

                chunk.metadata["content"] = canonical_content;
- Line 168: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: chunk.metadata["source"] = canonical_source;

            }

            if (!canonical_content.empty()) {

                chunk.metadata["content"] = canonical_content;

                chunk.metadata["text"] = canonical_content;

                chunk.metadata["body"] = canonical_content;

            }
- Line 169: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: }

            if (!canonical_content.empty()) {

                chunk.metadata["content"] = canonical_content;

                chunk.metadata["text"] = canonical_content;

                chunk.metadata["body"] = canonical_content;

            }

        }
- Line 170: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: if (!canonical_content.empty()) {

                chunk.metadata["content"] = canonical_content;

                chunk.metadata["text"] = canonical_content;

                chunk.metadata["body"] = canonical_content;

            }

        }
- Line 230: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: continue;

        }



        const std::string metadata_content = trimCopy(doc.metadata["content"]);

        const std::string metadata_source = trimCopy(doc.metadata["source"]);



        if (metadata_content.empty()) {
- Line 231: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: }



        const std::string metadata_content = trimCopy(doc.metadata["content"]);

        const std::string metadata_source = trimCopy(doc.metadata["source"]);



        if (metadata_content.empty()) {

            doc.metadata["content"] = canonical_content;
- Line 234: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: const std::string metadata_source = trimCopy(doc.metadata["source"]);



        if (metadata_content.empty()) {

            doc.metadata["content"] = canonical_content;

        } else {

            doc.metadata["content"] = metadata_content;

        }
- Line 236: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: if (metadata_content.empty()) {

            doc.metadata["content"] = canonical_content;

        } else {

            doc.metadata["content"] = metadata_content;

        }



        if (metadata_source.empty()) {
- Line 240: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: }



        if (metadata_source.empty()) {

            doc.metadata["source"] = canonical_id;

        } else {

            doc.metadata["source"] = metadata_source;

        }
- Line 242: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: if (metadata_source.empty()) {

            doc.metadata["source"] = canonical_id;

        } else {

            doc.metadata["source"] = metadata_source;

        }



        if (doc.metadata["content"].empty() || doc.metadata["source"].empty()) {
- Line 245: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: doc.metadata["source"] = metadata_source;

        }



        if (doc.metadata["content"].empty() || doc.metadata["source"].empty()) {

            continue;

        }
- Line 249: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: continue;

        }



        auto entities = toolbox_->extractEntities(doc.metadata["content"]);

        if (entities.empty()) {

            continue;

        }
- Line 255: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: }

        const std::string context = buildEntityContext(entities);

        if (!context.empty()) {

            doc.metadata["_entities"] = context;

            ++enriched;

        }

    }

### rag/knowledge_graph_retriever.cpp
Total findings: 17

- Line 373: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: normalise(candidate_node->canonical_name));
- Line 375: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: for (const auto& alias : candidate_node->aliases) {
- Line 387: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: match.node_id       = match.is_linked ? best_node->id : "";
- Line 438: severity=HIGH; category=command_injection
  Description: sql_injection_concat: SQL injection risk — use prepared statements
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: THEMIS_INFO("KnowledgeGraphRetriever::retrieve query='{}', candidates={}",
- Line 584: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: }

            }

            if (!chain_text.empty()) {

                aug_doc.document.metadata["reasoning_chain"] = chain_text;

            }

        }
- Line 42: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: out += ' ';
- Line 43: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: out += ' ';
- Line 76: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: double jaccardSets(const std::unordered_set<std::string>& A,
- Line 77: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: const std::unordered_set<std::string>& B) {
- Line 314: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: entities.push_back({trimmed, EntityType::OTHER, 0.7,
- Line 343: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (!span_buf.empty()) span_buf += ' ';
- Line 463: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> query_neighbourhood;
- Line 502: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto chain = impl_->reasoner->infer(match.node_id, cfg.max_inference_hops);
- Line 536: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> doc_node_ids;
- Line 575: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (!chain_text.empty()) chain_text += "; ";
- Line 576: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (!chain_text.empty()) chain_text += "; ";
- Line 59: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: /// length ratio; otherwise 0.0.  Both inputs should already be normalised.

### rag/reranker.cpp
Total findings: 17

- Line 14: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: * When a real ONNX cross-encoder model is loaded via loadModel() its
- Line 181: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: // loadModel() with a non-empty path.
- Line 309: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: const std::string key = impl_->cacheKey(query, doc.id);
- Line 312: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: if (auto cached = impl_->getCached(key)) {
- Line 393: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: THEMIS_ERROR("CrossEncoderReranker::loadModel: model file not found at '{}' ({})",
- Line 399: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: THEMIS_ERROR("CrossEncoderReranker::loadModel: model path is not a regular file ({})",
- Line 463: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: reranker->loadModel(model_path);
- Line 480: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: reranker->loadModel(model_path);
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #3574 fix: clear all remaining st... (2026-03-12) | #2576 RAG Phase 2: Wire s
- Line 21: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: *   4. Maps the raw score through a sigmoid to obtain a [0, 1] value.
- Line 92: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * document, then maps it through a sigmoid so the result lies in [0,1].
- Line 188: severity=HIGH; category=command_injection
  Description: sql_injection_concat: SQL injection risk — use prepared statements
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: return query + '\0' + doc_id;
- Line 67: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, size_t> termFreq(
- Line 70: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, size_t> tf;
- Line 296: severity=MEDIUM; category=uninitialized_member_field
  Description: Default constructor does not initialize POD members
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::uninitialized
  Context: Struct with uninitialized fields
- Line 219: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: //   auto inputs  = tokenise_pair(query, doc_text, config.max_length);
- Line 220: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: //   auto outputs = session_.Run(...);

### rag/agentic_rag.cpp
Total findings: 16

- Line 355: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: THEMIS_DEBUG("AgenticRAG iter {}: retrieved {} new docs for query='{}'",
- Line 359: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: THEMIS_INFO("AgenticRAG no new documents at iteration {}; stopping.", iter);
- Line 67: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 155: severity=HIGH; category=command_injection
  Description: sql_injection_concat: SQL injection risk — use prepared statements
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: return original_query + " specifically about " + gap.missing_aspects.front();
- Line 194: severity=HIGH; category=command_injection
  Description: sql_injection_concat: SQL injection risk — use prepared statements
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: THEMIS_INFO("AgenticRAG::run started: query='{}', initial_docs={}, max_iter={}, "
- Line 271: severity=HIGH; category=command_injection
  Description: sql_injection_concat: SQL injection risk — use prepared statements
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: THEMIS_DEBUG("AgenticRAG iteration {}: query='{}', docs={}",
- Line 355: severity=HIGH; category=command_injection
  Description: sql_injection_concat: SQL injection risk — use prepared statements
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: THEMIS_DEBUG("AgenticRAG iter {}: retrieved {} new docs for query='{}'",
- Line 355: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 358: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 359: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 365: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 51: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: const std::unordered_set<std::string>& ids)
- Line 63: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string>& seen_ids,
- Line 225: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Gap 4).' that was not found in 'src/rag/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: FUTURE_ENHANCEMENTS.md §Gap 4).
- Line 416: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (!seed.empty()) seed += '\n';
- Line 417: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (!seed.empty()) seed += '\n';

### rag/continuous_learning_client.cpp
Total findings: 14

- Line 55: severity=CRITICAL; category=no_timeout
  Description: thread_join without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: batch_thread.join();
- Line 55: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: batch_thread.join();
- Line 214: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // Send without holding lock

            impl_->batch_mutex.unlock();

            impl_->sendMetricsInternal(to_send);

            impl_->batch_mutex.lock();

        }

    } else {

        // Send immediately
- Line 214: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: impl_->batch_mutex.lock();
- Line 214: severity=HIGH; category=explicit_lock_unlock
  Description: Explicit lock/unlock without RAII guard
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: impl_->batch_mutex.lock();
- Line 383: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: faithfulness.type = MetricType::FAITHFULNESS;

    faithfulness.value = result.faithfulness_score;

    faithfulness.timestamp = now;

    faithfulness.metadata["mode"] = static_cast<int>(result.mode);

    metrics.push_back(faithfulness);

    

    // Relevance metric
- Line 412: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: overall.type = MetricType::OVERALL_QUALITY;

    overall.value = result.overall_score;

    overall.timestamp = now;

    overall.metadata["decision"] = static_cast<int>(result.decision);

    overall.metadata["passed_threshold"] = result.passed_threshold;

    metrics.push_back(overall);
- Line 413: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: overall.value = result.overall_score;

    overall.timestamp = now;

    overall.metadata["decision"] = static_cast<int>(result.decision);

    overall.metadata["passed_threshold"] = result.passed_threshold;

    metrics.push_back(overall);

    

    // Latency metric
- Line 421: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: latency.type = MetricType::LATENCY;

    latency.value = static_cast<double>(result.latency.count());

    latency.timestamp = now;

    latency.metadata["mode"] = static_cast<int>(result.mode);

    metrics.push_back(latency);

    

    return metrics;
- Line 47: severity=MEDIUM; category=missing_move_constructor_defaulted
  Description: Missing move ctor prevents efficient rvalue transfers
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 139: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: values.push_back(metric.value);
- Line 322: severity=MEDIUM; category=missing_move_constructor_defaulted
  Description: Missing move ctor prevents efficient rvalue transfers
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 453: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: combined += "; ";
- Line 454: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: combined += "; ";

### rag/document_summarizer.cpp
Total findings: 14

- Line 106: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator start may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: const auto start = current.find_first_not_of(" \t\n\r");
- Line 174: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: << "Document [" << document_id << "]:\n"
- Line 224: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (auto& w : tokeniseWords(query)) {
- Line 329: severity=HIGH; category=command_injection
  Description: sql_injection_concat: SQL injection risk — use prepared statements
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: THEMIS_INFO("DocumentSummarizer::summarizeMultiple docs={} query='{}'",
- Line 18: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: * LLMIntegration::generate() so that the LLM produces a fluent, compressed
- Line 64: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: const std::unordered_set<std::string>& query_terms)
- Line 73: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> seen;
- Line 108: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: sentences.push_back(current.substr(start));
- Line 154: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (!result.empty()) result += ' ';
- Line 155: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (!result.empty()) result += ' ';
- Line 222: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> queryTerms(const std::string& query) const {
- Line 223: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> terms;
- Line 255: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: ds.summary  = LLMIntegration::generate(prompt, opts);
- Line 359: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: result.combined_summary = LLMIntegration::generate(prompt, opts);

### rag/multi_step_rag.cpp
Total findings: 14

- Line 123: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator first may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: const auto first = line.find_first_not_of(" \t\r");
- Line 124: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator last may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: const auto last  = line.find_last_not_of(" \t\r");
- Line 118: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 419: severity=HIGH; category=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: const std::string gap_response = infer(gap_prompt, gap_max_tokens);
- Line 436: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 126: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: aspects.push_back(line.substr(first, last - first + 1));
- Line 298: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: // EXCEPTIONS: if infer() throws, the exception is stored in the future.
- Line 306: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: return infer(buildMapPrompt(batches[bi], query), map_max_tok);
- Line 315: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: try {

                result.steps.push_back(f.get());

                ++result.steps_executed;

            } catch (...) {

                if (!first_exc) first_exc = std::current_exception();

            }

        }
- Line 315: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 324: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: std::string partial          = infer(map_prompt, map_max_tok);
- Line 348: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: result.final_answer  = infer(reduce_prompt, bounded_max_tokens);
- Line 408: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: result.final_answer = infer(prompt, bounded_max_tokens);
- Line 419: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: const std::string gap_response = infer(gap_prompt, gap_max_tokens);

### rag/dpr_vectorizer.cpp
Total findings: 13

- Line 172: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: const size_t input_count = static_cast<size_t>(session->GetInputCount());
- Line 292: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto query_model = impl_->model_loader->loadModel(config_.query_model_path);
- Line 304: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto passage_model = impl_->model_loader->loadModel(config_.passage_model_path);
- Line 316: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: impl_->query_tokenizer = std::make_unique<themis::llm::lora::LlamaTokenizer>(config_.query_model_pat
- Line 317: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: impl_->passage_tokenizer = std::make_unique<themis::llm::lora::LlamaTokenizer>(config_.passage_model
- Line 409: severity=HIGH; category=missing_trace_point
  Description: Critical function query without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: THEMIS_DEBUG("Encoded query (validated, {} bytes) -> {} dimensions",
- Line 201: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const auto type_info = outputs[0].GetTensorTypeAndShapeInfo();
- Line 372: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: std::vector<float> DPRVectorizer::encodeQuery(const std::string& query) {
- Line 409: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: THEMIS_DEBUG("Encoded query (validated, {} bytes) -> {} dimensions",
- Line 189: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: auto outputs = session->Run(
- Line 197: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: if (outputs.empty() || !outputs[0].IsTensor()) {
- Line 201: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: const auto type_info = outputs[0].GetTensorTypeAndShapeInfo();
- Line 203: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: const auto* out_data = outputs[0].GetTensorData<float>();

### rag/distributed_rag_evaluator.cpp
Total findings: 12

- Line 114: severity=HIGH; category=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (size_t i = 0; i < n; ++i) {
- Line 117: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::unique_lock<std::mutex> lk(sem->mtx);
- Line 128: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: auto result = judge_ptr->evaluate(*shared_input);
- Line 153: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: res = futures[i].get();
- Line 160: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: res = futures[i].get();
- Line 315: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: if (total_w == 0.0) { total_w = 1.0; }
- Line 124: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto judge_ptr = judges[i];  // shared ownership — safe across timeouts
- Line 151: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto status = futures[i].wait_for(timeout);
- Line 170: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: successful_weights.push_back(impl_->workers[i].weight);
- Line 220: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: const std::vector<judge::EvaluationInput>& inputs)
- Line 223: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: results.reserve(inputs.size());
- Line 225: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: for (const auto& input : inputs) {

### rag/rlaif_trainer.cpp
Total findings: 12

- Line 177: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: impl_->judge  = std::make_shared<HeuristicAIJudge>();
- Line 450: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: const double n       = static_cast<double>(impl_->stats.successful_steps);
- Line 451: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: const double old_avg = impl_->stats.avg_preference_score;
- Line 452: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: impl_->stats.avg_preference_score =
- Line 458: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: const double n      = static_cast<double>(impl_->stats.total_steps);
- Line 459: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: const auto old_ms   = impl_->stats.avg_step_ms.count();
- Line 461: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: impl_->stats.avg_step_ms =
- Line 85: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (lower.find(p) != std::string::npos) {
- Line 199: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: "RLAIFTrainer: min_quality_threshold must be in [0, 1]");
- Line 204: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: "RLAIFTrainer: min_preference_score must be in [0, 1]");
- Line 523: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, size_t> pv_map;
- Line 538: severity=MEDIUM; category=missing_move_constructor_defaulted
  Description: Missing move ctor prevents efficient rvalue transfers
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety

### rag/delegate_evaluator.cpp
Total findings: 11

- Line 140: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> seen;
- Line 223: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: static const std::regex TAG_RE(R"(<([A-Za-z_][A-Za-z0-9_:.-]*)[\s/>])");
- Line 255: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> seen;
- Line 451: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: ++result.persistence_write_failures;

                }

            }

        } catch (...) {

            // Forward edit failed: record 0.0 and terminate

            const ReconstructionScore rs = 0.0;

            result.scores.rs_per_interaction.push_back(rs);
- Line 451: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 480: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: ++result.persistence_write_failures;

                }

            }

        } catch (...) {

            // Backward edit failed: record 0.0 and terminate

            const ReconstructionScore rs = 0.0;

            result.scores.rs_per_interaction.push_back(rs);
- Line 480: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 130: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: // This can happen for whitespace/punctuation-only inputs after
- Line 158: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: * @note For inputs above 10 000 characters, this function switches to an
- Line 165: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: * is used instead, keeping RS computation under 5 ms for 100 KB inputs.
- Line 415: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: // ── Validate inputs ──────────────────────────────────────────────────────

### rag/fairness_detector.cpp
Total findings: 10

- Line 338: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: bool loaded = impl_->loadEmbeddings(config_.embedding_model_path);
- Line 147: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto male_it = embeddings.find(male_word);
- Line 148: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto female_it = embeddings.find(female_word);
- Line 197: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto low_it = embeddings.find(low_status);
- Line 198: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto high_it = embeddings.find(high_status);
- Line 244: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto left_it = embeddings.find(left_word);
- Line 245: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto right_it = embeddings.find(right_word);
- Line 499: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: } catch (const std::exception& e) {

        THEMIS_ERROR("Bias detection failed: {}", e.what());

        throw std::runtime_error(std::string("Bias detection failed: ") + e.what());

    }

}
- Line 397: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: words.push_back(Impl::toLower(word));
- Line 403: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: words.push_back(Impl::toLower(word));

### rag/response_parser.cpp
Total findings: 10

- Line 203: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: THEMIS_WARN("Score {} out of valid range [0, 5]", score);
- Line 208: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: THEMIS_WARN("Confidence {} out of valid range [0, 1]", *parsed.confidence);
- Line 217: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: if (max_range == min_range) {
- Line 32: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: const char* ResponseParser::SCORE_PATTERN_1 = R"((?:score|rating)[\s:]+([0-9.]+)(?:/5|%|\s|$))";
- Line 33: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: const char* ResponseParser::SCORE_PATTERN_2 = R"(([0-9.]+)\s*(?:out of|/)\s*([0-9.]+))";
- Line 246: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }

            

            return score;

        } catch (...) {

            THEMIS_DEBUG("Failed to parse score from pattern 1");

        }

    }
- Line 246: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 262: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: double score = (numerator / denominator) * 5.0;

                return score;

            }

        } catch (...) {

            THEMIS_DEBUG("Failed to parse score from pattern 2");

        }

    }
- Line 262: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 12: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: * @brief Implementation of response parsing for LLM judge outputs

### rag/prompt_injection_detector.cpp
Total findings: 9

- Line 104: severity=HIGH; category=regex_in_loop
  Description: std::regex compiled in loop (compile once, reuse)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (const auto& e : shared.patterns()) {
- Line 254: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& rule : getRules()) {
- Line 106: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: r.push_back({
- Line 121: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: r.push_back({
- Line 131: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: r.push_back({
- Line 138: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: r.push_back({
- Line 159: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: r.push_back({
- Line 163: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: R"(\bhttps?://[^\s]+\?(?:[^\s]*=\{[^}]*\}|\[PROMPT\]|\[CONTEXT\]|\[OUTPUT\]))",
- Line 390: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: to_replace.push_back(&f);

### rag/llm_integration.cpp
Total findings: 8

- Line 134: severity=HIGH; category=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto response = llm::LLMPluginManager::instance().generate(req);
- Line 151: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // Create an enhanced inference request

        llm::InferenceEngineEnhanced::EnhancedInferenceRequest request;

        request.base_request.prompt = prompt;

        request.base_request.max_tokens = static_cast<int>(std::min(

            options.max_tokens,

            static_cast<size_t>(std::numeric_limits<int>::max())));

        request.base_request.temperature = static_cast<float>(options.temperature);
- Line 60: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: const std::unordered_map<std::string, std::string>& variables
- Line 134: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto response = llm::LLMPluginManager::instance().generate(req);
- Line 240: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: samples.push_back(generate(prompt, sample_options));
- Line 342: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> set1(tokens1.begin(), tokens1.end());
- Line 343: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> set2(tokens2.begin(), tokens2.end());
- Line 300: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: log_sum += std::log(prob);

### rag/llm_judge_client.cpp
Total findings: 8

- Line 178: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: if (!plugin->loadModel(model_path.string(), plugin_config)) {
- Line 238: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: request.base_request.temperature = static_cast<float>(impl_->config.temperature);
- Line 292: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: request.base_request.temperature = static_cast<float>(impl_->config.temperature);
- Line 132: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& entry : fs::directory_iterator(dir)) {
- Line 238: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: llm::InferenceEngineEnhanced::EnhancedInferenceRequest request;

        request.base_request.prompt = prompt;

        request.base_request.max_tokens = impl_->config.max_tokens;

        request.base_request.temperature = static_cast<float>(impl_->config.temperature);

        request.base_request.top_p = 0.95f;

        request.base_request.stop_sequences = impl_->config.stop_sequences;
- Line 292: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: llm::InferenceEngineEnhanced::EnhancedInferenceRequest request;

            request.base_request.prompt = prompt;

            request.base_request.max_tokens = impl_->config.max_tokens;

            request.base_request.temperature = static_cast<float>(impl_->config.temperature);

            request.base_request.top_p = 0.95f;

            request.base_request.stop_sequences = impl_->config.stop_sequences;
- Line 470: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: try {

                    parsed.score = std::stod(score_str);

                } catch (...) {

                    parsed.score = 0.5; // Default

                }

            }
- Line 470: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {

### rag/llm_judge_integration.cpp
Total findings: 8

- Line 84: severity=CRITICAL; category=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: std::string prompt = template_mgr.generatePrompt(dimension, input);
- Line 209: severity=HIGH; category=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: std::string response = inference_fn_(prompt);
- Line 39: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: // Wire the engine's generate() into the inference function slot
- Line 41: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: return engine->generate(prompt);
- Line 231: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Phase 9: AI Reliability & Safety Evaluation Program' that was not found in 'src/rag/ROADMAP.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/rag/ROADMAP.md § "Phase 9: AI Reliability & Safety Evaluation Program"
- Line 234: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Gap 7.' that was not found in 'src/rag/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: FUTURE_ENHANCEMENTS.md §Gap 7.
- Line 235: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'LLMIntegration and LLMJudgeIntegration: Replace Stub/Mock Mode' that was not found in 'src/rag/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/rag/FUTURE_ENHANCEMENTS.md § "LLMIntegration and LLMJudgeIntegration: Replace Stub/Mock Mode"
- Line 226: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: //                   still non-model mock outputs. As of 2026-04-21 the caller

### rag/multimodal_rag.cpp
Total findings: 8

- Line 238: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: src.image_path      = img_it->second.image_path;
- Line 239: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: src.metadata        = img_it->second.metadata;
- Line 243: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: src.caption = img_it->second.caption;
- Line 245: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: src.caption = impl_->image_captioner(img_it->second);
- Line 136: severity=HIGH; category=missing_trace_point
  Description: Critical function query without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: MultiModalRAGResult MultiModalRAG::query(const MultiModalQuery& mq) const {
- Line 136: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: MultiModalRAGResult MultiModalRAG::query(const MultiModalQuery& mq) const {
- Line 174: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, ImageDocument> image_doc_map;
- Line 224: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> used_ids;

### rag/http_metrics_client.cpp
Total findings: 7

- Line 194: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: std::unique_lock<std::shared_mutex> lock(stats_mutex_);

                    stats_.retries_attempted++;

                }

                backoff.wait();

            } else {

                response.error_message = "HTTP " + std::to_string(result->status);

                THEMIS_ERROR("HTTP {} {} failed with status {}",
- Line 194: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: backoff.wait();
- Line 194: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: backoff.wait();
- Line 213: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: std::unique_lock<std::shared_mutex> lock(stats_mutex_);

                    stats_.retries_attempted++;

                }

                backoff.wait();

            } else {

                THEMIS_ERROR("HTTP {} {} failed: {}",

                               method == HTTPMethod::POST ? "POST" : "GET", path, response.error_message);
- Line 213: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: backoff.wait();
- Line 213: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: backoff.wait();
- Line 124: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: const std::unordered_map<std::string, std::string>& headers) {

### rag/quality_control_pipeline.cpp
Total findings: 7

- Line 124: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: const auto passed_fast_count = static_cast<double>(impl_->stats.passed_fast);
- Line 126: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: impl_->stats.avg_fast_time_ms =
- Line 522: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: impl_->learning_callback = callback;
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #3310 [rag] Wire CitationHighligh... (2026-03-12) | #1273 Analysis: Duplicate
- Line 46: severity=MEDIUM; category=missing_move_constructor_defaulted
  Description: Missing move ctor prevents efficient rvalue transfers
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 456: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, double> weights = {
- Line 514: severity=MEDIUM; category=missing_move_constructor_defaulted
  Description: Missing move ctor prevents efficient rvalue transfers
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety

### rag/bayesian_optimizer.cpp
Total findings: 6

- Line 109: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 73: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, double> BayesianOptimizer::getBestParams() const {
- Line 85: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, double> BayesianOptimizer::sampleRandom() {
- Line 86: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, double> params;
- Line 96: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, double> BayesianOptimizer::sampleAroundBest() {
- Line 97: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, double> params;

### rag/claim_extractor.cpp
Total findings: 6

- Line 54: severity=HIGH; category=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: std::string response = LLMIntegration::generate(prompt);
- Line 113: severity=HIGH; category=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: std::string response = LLMIntegration::generate(prompt);
- Line 246: severity=HIGH; category=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: std::string llm_response = LLMIntegration::generate(prompt);
- Line 54: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: std::string response = LLMIntegration::generate(prompt);
- Line 113: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: std::string response = LLMIntegration::generate(prompt);
- Line 246: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: std::string llm_response = LLMIntegration::generate(prompt);

### rag/cot_evaluator.cpp
Total findings: 6

- Line 50: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (size_t i = 0; i < std::min(documents.size(), size_t(3)); ++i) {
- Line 194: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (conclusion_i.find(neg) != std::string::npos) i_has_negation = true;
- Line 195: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (conclusion_i.find(neg) != std::string::npos) i_has_negation = true;
- Line 196: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (conclusion_j.find(neg) != std::string::npos) j_has_negation = true;
- Line 195: severity=MEDIUM; category=expensive_copy
  Description: Unnecessary expensive copy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (conclusion_i.find(neg) != std::string::npos) i_has_negation = true;
- Line 223: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: inconsistencies.push_back(inconsistency.str());

### rag/replug_retriever.cpp
Total findings: 6

- Line 111: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: "ReplugRetriever: llm_weight must be in [0, 1]");
- Line 178: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: if (range == 0.0) {
- Line 254: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = weights_.find(doc.id);
- Line 343: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: if (w == 0.0) {
- Line 60: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: double jaccardSimilarity(const std::unordered_set<std::string>& a,
- Line 61: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: const std::unordered_set<std::string>& b) {

### rag/coherence_evaluator.cpp
Total findings: 5

- Line 317: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (sent_i.find(neg) != std::string::npos) i_has_negation = true;
- Line 318: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (sent_j.find(neg) != std::string::npos) j_has_negation = true;
- Line 292: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: sentences.push_back(it->str());
- Line 317: severity=MEDIUM; category=expensive_copy
  Description: Unnecessary expensive copy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (sent_i.find(neg) != std::string::npos) i_has_negation = true;
- Line 342: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: contradictions.push_back(sentences[i] + " <-> " + sentences[j]);

### rag/llm_meta_analyzer.cpp
Total findings: 5

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #651 [RAG-ETHICS] Add ethical co... (2026-03-11) | #1297 RAG module: replace
- Line 261: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 288: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: request.priority      = 0;



            static std::atomic<uint64_t> req_counter{0};

            request.request_id = "meta_" + std::to_string(req_counter.fetch_add(1));



            auto response = engine->submit(request).get();

            THEMIS_DEBUG("LLMMetaAnalyzer::callLLM response length: {}", response.text.size());
- Line 210: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: std::regex("([0-9]*\\.?[0-9]+)\\s*/\\s*1\\.?0?"),
- Line 133: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: // Validate all inputs to prevent memory exhaustion and injection attacks

### rag/lora_enhanced_retriever.cpp
Total findings: 5

- Line 128: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: const double lora_s = scorer_->score(query, doc.content, config_.domain);
- Line 152: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: return std::stod(it->second) >= config_.min_lora_score;
- Line 134: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: THEMIS_DEBUG("LoRAEnhancedRetriever: doc='{}' orig={:.3f} lora={:.3f} fused={:.3f}",

                     doc.id, doc.similarity_score, lora_s, fused);



        doc.metadata["lora_score"]  = std::to_string(lora_s);

        doc.similarity_score        = fused;

    }
- Line 46: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: double jaccardTokenSets(const std::unordered_set<std::string>& A,
- Line 47: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: const std::unordered_set<std::string>& B)

### rag/multi_hop_reasoner.cpp
Total findings: 5

- Line 229: severity=HIGH; category=command_injection
  Description: sql_injection_concat: SQL injection risk — use prepared statements
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: " (" + hr.sub_query + "): " + hr.intermediate_answer);
- Line 78: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> seen;
- Line 134: severity=MEDIUM; category=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (!t.empty()) sentences.push_back(t);
- Line 227: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: partial_answers.push_back(
- Line 324: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: previous_answers.push_back(hop.intermediate_answer);

### rag/relevance_evaluator.cpp
Total findings: 5

- Line 62: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = freq.find(vocab[i]);
- Line 215: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (query_lower.find(kw) != std::string::npos) {
- Line 57: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, double> freq;
- Line 157: severity=MEDIUM; category=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: questions.push_back(question.get<std::string>());
- Line 158: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: questions.push_back(question.get<std::string>());

### rag/tensor_rag_pipeline.cpp
Total findings: 5

- Line 129: severity=HIGH; category=missing_trace_point
  Description: Critical function query without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: "threw for FLARE query (len=%zu); embedding left empty "
- Line 123: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: if (efn) {

                    try {

                        decision.flare_query_embedding = efn(decision.flare_query);

                    } catch (...) {

                        // Fail-closed: embedding fn threw; leave embedding empty.

                        // Distinct from "no fn wired" — the backend is registered but

                        // failed at runtime. Operators should diagnose the root cause.
- Line 123: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 129: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: "threw for FLARE query (len=%zu); embedding left empty "
- Line 127: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::fprintf(stderr,

### rag/flare_retrieval.cpp
Total findings: 4

- Line 17: severity=CRITICAL; category=sensitive_data_logging
  Description: Potential PII/credential logging: token
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: *  1. Emit token t with log-probability log(p(t)).
- Line 18: severity=CRITICAL; category=sensitive_data_logging
  Description: Potential PII/credential logging: token
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: *  2. If log(p(t)) < confidence_threshold → mark token as uncertain.
- Line 180: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: std::string FlareRetrieval::buildQuery() const {
- Line 225: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::fprintf(stderr,

### rag/geval_evaluator.cpp
Total findings: 4

- Line 354: severity=CRITICAL; category=sensitive_data_logging
  Description: Potential PII/credential logging: token
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: reasoning << "Token probability distribution:\n";
- Line 138: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: score_tokens.push_back(-1);  // Token not in vocabulary; skip during probability extraction
- Line 249: severity=MEDIUM; category=shift_overflow
  Description: Shift operation detected (verify shift count < bitwidth)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['                std::string tok;', '                size_t idx = 0;', '                while (iss >> tok && idx < response.logprobs.size()) {', '                    // kNumScoreLevels ≤ 9 so single-digit check is safe', "                    char max_digit = static_cast<char>('0' + kNumScoreLevels);"]
- Line 467: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<int, int> counts;

### rag/ontology_aware_retriever.cpp
Total findings: 4

- Line 187: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: const std::string src_type = entityTypeName(src_node->type);
- Line 188: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: const std::string tgt_type = entityTypeName(tgt_node->type);
- Line 88: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> expanded;
- Line 97: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> visited = {concept_id};

### rag/pairwise_comparator.cpp
Total findings: 4

- Line 294: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: result.overall_confidence = static_cast<double>(a_votes) / impl_->config.num_samples;
- Line 297: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: result.overall_confidence = static_cast<double>(b_votes) / impl_->config.num_samples;
- Line 32: severity=MEDIUM; category=random_unseeded
  Description: RNG engine appears default-constructed without explicit seeding
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::mt19937 rng;
- Line 278: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: results.push_back(compareWithLLM(query, documents, answer_a, answer_b, a_first));

### rag/adversarial_tester.cpp
Total findings: 3

- Line 120: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> set_a(a.begin(), a.end());
- Line 121: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> set_b(b.begin(), b.end());
- Line 182: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: result += " (variant " + std::to_string(variant_index + 1) + ")";

### rag/calibration_manager.cpp
Total findings: 3

- Line 448: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: bool CalibrationManager::loadModel(const std::string& filepath) {
- Line 130: severity=MEDIUM; category=uninitialized_member_field
  Description: Default constructor does not initialize POD members
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::uninitialized
  Context: Struct with uninitialized fields
- Line 104: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: double logit = std::log(s / (1.0 - s));

### rag/citation_highlighter.cpp
Total findings: 3

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #3321 [rag] Fix CitationHighlight... (2026-03-12) | #2749 feat(rag): citation
- Line 112: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: bool isDelim = (cfg.sentence_delimiters.find(ch) != std::string::npos);
- Line 218: severity=MEDIUM; category=uninitialized_member_field
  Description: Default constructor does not initialize POD members
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::uninitialized
  Context: Struct with uninitialized fields

### rag/completeness_evaluator.cpp
Total findings: 3

- Line 369: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (size_t i = 0; i < std::min(result.missing_information.size(), size_t(3)); ++i) {
- Line 157: severity=MEDIUM; category=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: aspects.push_back(aspect);
- Line 271: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: missing_info.push_back(aspect.aspect_text);

### rag/explainability_reason_builder.cpp
Total findings: 3

- Line 159: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: "conditions (miss rate, profile drift, or new entry count).",
- Line 159: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 364: severity=MEDIUM; category=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: // Sort by timestamp (ascending — oldest first)

### rag/hybrid_retriever.cpp
Total findings: 3

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #2747 [rag] Hybrid retrieval (BM2... (2026-03-12)
- Line 43: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: if (range == 0.0) {
- Line 275: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, DocData> doc_map;

### rag/onnx_model_loader.cpp
Total findings: 3

- Line 125: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: return loadModel(dest_path);
- Line 186: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = impl_->cache.find(model_name);
- Line 262: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: fclose(fp);

### rag/self_rag.cpp
Total findings: 3

- Line 71: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> doc_terms(d_tokens.begin(), d_tokens.end());
- Line 127: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: static const std::unordered_set<std::string> evidence_terms = {
- Line 198: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> seen(seen_ids_.begin(), seen_ids_.end());

### rag/faithfulness_evaluator.cpp
Total findings: 2

- Line 114: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: impl_->nli_verifier = std::make_shared<NLIFaithfulnessVerifier>(nli_config);
- Line 330: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: explanation << "Claims: " << result.supported_claims_count << "/" << result.total_claims_count << "

### rag/learning_metrics.cpp
Total findings: 2

- Line 128: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: << " +/- " << snap.std_accuracy << "\n";
- Line 130: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: << " +/- " << snap.std_faithfulness << "\n";

### rag/prompt_templates.cpp
Total findings: 2

- Line 99: severity=CRITICAL; category=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: prompt = replacePlaceholders(prompt, input);
- Line 376: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: stream << "Score: " << ex.score << "/5\n";

### rag/streaming_retriever.cpp
Total findings: 2

- Line 210: severity=HIGH; category=command_injection
  Description: sql_injection_concat: SQL injection risk — use prepared statements
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: THEMIS_INFO("StreamingRetriever::stream started: query='{}', candidates={}",
- Line 38: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> tokens;

### rag/PRODUCTION_REQUIREMENTS.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'PRODUCTION_REQUIREMENTS.md' is missing expected cross-links: FUTURE_ENHANCEMENTS.md, README.md, ROADMAP.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### rag/bias_detector.cpp
Total findings: 1

- Line 133: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: lengths.push_back(static_cast<double>(length));

### rag/hallucination_dashboard.cpp
Total findings: 1

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #2753 [rag] Hallucination rate tr... (2026-03-12) | #2613 feat(rag): hallucin

### rag/judge_config.cpp
Total findings: 1

- Line 166: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: THEMIS_ERROR("quality_threshold must be in [0, 1], got {}", quality_threshold);

### rag/judge_ensemble.cpp
Total findings: 1

- Line 12: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Deprecated shim: functionality now lives in rag_rag_judge.cpp (JudgeEnsemble implementation).

### rag/nli_faithfulness_verifier.cpp
Total findings: 1

- Line 358: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: void NLIFaithfulnessVerifier::loadModel(const std::string& model_path) {

### rag/rubric_evaluator.cpp
Total findings: 1

- Line 63: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (size_t i = 0; i < std::min(documents.size(), size_t(3)); ++i) {

### rag/targ_retrieval.cpp
Total findings: 1

- Line 141: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: if (p > 0.0f) entropy -= p * std::log(p);

## Update Workflow

- Refresh scanner artifacts with: python tools/gs3_orchestrator.py ./src --output ai_working/gap_scan_results.json
- Regenerate docs with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- Add --no-mirror when you only want archive docs in ai_working/module_gaps.

Format: THEMIS_MODULE_GAPS_V4
