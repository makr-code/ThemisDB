# importers Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: importers
- Generated: 2026-06-02 11:09:13
- Status: Critical Findings Present
- Total Findings: 198
- Actionable Findings (Critical + High): 55
- Affected Files: 28

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 3 |
| High | 52 |
| Medium | 142 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| performance_patterns | 156 |
| container | 84 |
| security | 65 |
| reliability | 47 |
| performance | 39 |
| llm_ai_safety | 36 |
| concurrency | 25 |
| exception_safety | 13 |
| raii | 13 |
| platform | 11 |
| memory | 10 |
| determinism | 5 |
| audit_logging | 2 |
| input_validation | 1 |
| observability | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/importers/schema_inference.cpp | 41 | 0 | 32 | 9 | 0 |
| src/importers/postgres_importer.cpp | 28 | 0 | 10 | 18 | 0 |
| src/importers/deterministic_matcher.cpp | 20 | 0 | 2 | 18 | 0 |
| src/importers/mdm_engine.cpp | 16 | 0 | 2 | 14 | 0 |
| src/importers/flatfile_importer.cpp | 14 | 0 | 0 | 14 | 0 |
| src/importers/federated_learning.cpp | 9 | 0 | 0 | 8 | 1 |
| src/importers/blockchain_integrity.cpp | 7 | 3 | 3 | 1 | 0 |
| src/importers/data_quality.cpp | 7 | 0 | 0 | 7 | 0 |
| src/importers/huggingface_ingestion_plugin.cpp | 7 | 0 | 0 | 7 | 0 |
| src/importers/adaptive_import.cpp | 6 | 0 | 1 | 5 | 0 |
| src/importers/s3_importer.cpp | 6 | 0 | 0 | 6 | 0 |
| src/importers/polyglot_mapper.cpp | 5 | 0 | 0 | 5 | 0 |
| src/importers/column_importance.cpp | 4 | 0 | 1 | 3 | 0 |
| src/importers/mongo_importer.cpp | 4 | 0 | 0 | 4 | 0 |
| src/importers/gui_import_wizard.cpp | 3 | 0 | 0 | 3 | 0 |
| src/importers/mysql_importer.cpp | 3 | 0 | 0 | 3 | 0 |
| src/importers/oracle_importer.cpp | 3 | 0 | 0 | 3 | 0 |
| src/importers/sqlite_importer.cpp | 3 | 0 | 0 | 3 | 0 |
| src/importers/canonical_resolver.cpp | 2 | 0 | 1 | 1 | 0 |
| src/importers/entity_linker.cpp | 2 | 0 | 0 | 2 | 0 |
| src/importers/graphql_federation.cpp | 2 | 0 | 0 | 2 | 0 |
| src/importers/mdm_audit_trail.cpp | 2 | 0 | 0 | 2 | 0 |
| src/importers/schema_validator.cpp | 2 | 0 | 0 | 2 | 0 |
| src/importers/audit_trail.cpp | 1 | 0 | 0 | 1 | 0 |
| src/importers/postgres_importer_mdm.cpp | 1 | 0 | 0 | 1 | 0 |
| src/importers/conflict_resolver.cpp | 0 | 0 | 0 | 0 | 0 |
| src/importers/kafka_importer.cpp | 0 | 0 | 0 | 0 | 0 |
| src/importers/postgres_cdc.cpp | 0 | 0 | 0 | 0 | 0 |

## Full Scanner Findings

### src/importers/schema_inference.cpp
Total findings: 41

- Line 2: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: * ThemisDB | File: schema_inference.cpp | Version: 0.0.13 | Last Modified: 2026-05-31 12:17:24
  Confidence: band=very_high; score=0.9
- Line 10: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: #include "importers/schema_inference.h"
  Confidence: band=very_high; score=0.9
- Line 24: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: SchemaInferenceEngine::SchemaInferenceEngine(Config cfg)
  Confidence: band=very_high; score=0.9
- Line 31: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: bool SchemaInferenceEngine::columnNameSimilar(const std::string& a,
  Confidence: band=very_high; score=0.9
- Line 46: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: double SchemaInferenceEngine::jaccardSimilarity(const std::vector<std::string>& a,
  Confidence: band=very_high; score=0.9
- Line 62: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::vector<SchemaInferenceEngine::InferredSchema>
  Confidence: band=very_high; score=0.9
- Line 63: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: SchemaInferenceEngine::inferImplicitRelationships(
  Confidence: band=very_high; score=0.9
- Line 64: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: const std::vector<InferenceTableSchema>& schemas,
  Confidence: band=very_high; score=0.9
- Line 67: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::vector<InferredSchema> results;
  Confidence: band=very_high; score=0.9
- Line 72: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: InferredSchema inferred;
  Confidence: band=very_high; score=0.9
- Line 73: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: inferred.table_name = schema.name;
  Confidence: band=very_high; score=0.9
- Line 74: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: inferred.recommendations = json::object();
  Confidence: band=very_high; score=0.9
- Line 101: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: inferred.likely_relationships.push_back(
  Confidence: band=very_high; score=0.9
- Line 112: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: inferred.cardinality_distribution[col] =
  Confidence: band=very_high; score=0.9
- Line 120: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: for (const auto& [col, ratio] : inferred.cardinality_distribution) {
  Confidence: band=very_high; score=0.9
- Line 122: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: inferred.denormalization_candidates.push_back(col);
  Confidence: band=very_high; score=0.9
- Line 126: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: inferred.recommendations["relationship_count"] =
  Confidence: band=very_high; score=0.9
- Line 127: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: inferred.likely_relationships.size();
  Confidence: band=very_high; score=0.9
- Line 128: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: inferred.recommendations["denorm_candidates"] =
  Confidence: band=very_high; score=0.9
- Line 129: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: inferred.denormalization_candidates.size();
  Confidence: band=very_high; score=0.9
- Line 131: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: results.push_back(std::move(inferred));
  Confidence: band=very_high; score=0.9
- Line 141: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: SchemaInferenceEngine::SemanticType
  Confidence: band=very_high; score=0.9
- Line 142: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: SchemaInferenceEngine::detectSingleColumn(
  Confidence: band=very_high; score=0.9
- Line 183: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::map<std::string, SchemaInferenceEngine::SemanticType>
  Confidence: band=very_high; score=0.9
- Line 184: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: SchemaInferenceEngine::detectSemanticTypes(
  Confidence: band=very_high; score=0.9
- Line 185: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: const std::vector<InferenceTableSchema>& schemas,
  Confidence: band=very_high; score=0.9
- Line 200: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it = sample_index.find(key);
  Confidence: band=very_high; score=0.9
- Line 209: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::string SchemaInferenceEngine::semanticTypeToString(SemanticType t) {
  Confidence: band=very_high; score=0.9
- Line 228: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::vector<SchemaInferenceEngine::CardinalityEstimate>
  Confidence: band=very_high; score=0.9
- Line 229: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: SchemaInferenceEngine::estimateCardinalities(
  Confidence: band=very_high; score=0.9
- Line 230: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: const std::vector<InferenceTableSchema>& schemas,
  Confidence: band=very_high; score=0.9
- Line 245: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it = stats.find(local_key);
  Confidence: band=very_high; score=0.9
- Line 51: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> setA(a.begin(), a.end());
  Confidence: band=medium; score=0.66
- Line 52: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> setB(b.begin(), b.end());
  Confidence: band=medium; score=0.66
- Line 100: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: inferred.likely_relationships.push_back(
  Confidence: band=high; score=0.74
- Line 121: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: inferred.denormalization_candidates.push_back(col);
  Confidence: band=high; score=0.74
- Line 159: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<SemanticType, size_t> votes;
  Confidence: band=high; score=0.74
- Line 183: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, SchemaInferenceEngine::SemanticType>
  Confidence: band=high; score=0.74
- Line 188: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, SemanticType> result;
  Confidence: band=high; score=0.74
- Line 192: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::vector<std::string>> sample_index;
  Confidence: band=high; score=0.74
- Line 231: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<std::string, ColumnStatistics>& stats)
  Confidence: band=high; score=0.74

### src/importers/postgres_importer.cpp
Total findings: 28

- Line 615: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (hdr_line.find("-- PostgreSQL database dump") != std::string::npos ||
  Confidence: band=very_high; score=0.9
- Line 616: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: hdr_line.find("pg_dump") != std::string::npos) {
  Confidence: band=very_high; score=0.9
- Line 620: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (hdr_line.find("schema only") != std::string::npos ||
  Confidence: band=very_high; score=0.9
- Line 621: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: hdr_line.find("schema-only") != std::string::npos ||
  Confidence: band=very_high; score=0.9
- Line 622: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: hdr_line.find("--schema-only") != std::string::npos) {
  Confidence: band=very_high; score=0.9
- Line 625: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (hdr_line.find("data only") != std::string::npos ||
  Confidence: band=very_high; score=0.9
- Line 626: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: hdr_line.find("data-only") != std::string::npos ||
  Confidence: band=very_high; score=0.9
- Line 627: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: hdr_line.find("--data-only") != std::string::npos) {
  Confidence: band=very_high; score=0.9
- Line 951: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (upper_def.find("FOREIGN KEY") != std::string::npos) {
  Confidence: band=very_high; score=0.9
- Line 1436: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (std::find(target.columns.begin(), target.columns.end(), col)
  Confidence: band=very_high; score=0.9
- Line 559: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (const auto& ck : schema.check_constraints) ck_arr.push_back(ck.toJson());
  Confidence: band=high; score=0.74
- Line 564: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (const auto& g : schema.generated_columns) gen_arr.push_back(g.toJson());
  Confidence: band=high; score=0.74
- Line 569: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (const auto& ex : schema.exclude_constraints) excl_arr.push_back(ex.toJson());
  Confidence: band=high; score=0.74
- Line 572: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(table_json);
  Confidence: band=high; score=0.74
- Line 578: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (const auto& m : inverse_mappings) relationships_arr.push_back(m.toJson());
  Confidence: band=high; score=0.74
- Line 581: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (const auto& c : cycles) cycles_arr.push_back(c);
  Confidence: band=high; score=0.74
- Line 1074: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: schema.generated_columns.push_back(gen);
  Confidence: band=high; score=0.74
- Line 1131: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) out += ",";
  Confidence: band=high; score=0.74
- Line 1375: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (!col.empty()) index.columns.push_back(col);
  Confidence: band=high; score=0.74
- Line 1428: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stats.structured_errors.push_back(err);
  Confidence: band=high; score=0.74
- Line 1428: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stats.structured_errors.push_back(err);
  Confidence: band=high; score=0.74
- Line 1445: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stats.structured_errors.push_back(err);
  Confidence: band=high; score=0.74
- Line 1573: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: excl.elements.push_back(std::move(el));
  Confidence: band=high; score=0.74
- Line 1573: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: excl.elements.push_back(std::move(el));
  Confidence: band=high; score=0.74
- Line 2023: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: case 'N':  /* \N already handled above as entire field */ out += '\\'; out += 'N'; break;
  Confidence: band=high; score=0.74
- Line 2023: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: case 'N':  /* \N already handled above as entire field */ out += '\\'; out += 'N'; break;
  Confidence: band=high; score=0.74
- Line 2177: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: fk_arr.push_back(fk.toJson());
  Confidence: band=high; score=0.74
- Line 2177: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: fk_arr.push_back(fk.toJson());
  Confidence: band=high; score=0.74

### src/importers/deterministic_matcher.cpp
Total findings: 20

- Line 99: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (r.existing_entity_id == m.existing_entity_id) {
  Confidence: band=very_high; score=0.9
- Line 418: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (norm1 == 0.0 || norm2 == 0.0) {
  Confidence: band=very_high; score=0.9
- Line 40: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matched_keys.push_back(field);
  Confidence: band=high; score=0.74
- Line 104: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(std::move(m));
  Confidence: band=high; score=0.74
- Line 104: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(std::move(m));
  Confidence: band=high; score=0.74
- Line 104: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(std::move(m));
  Confidence: band=high; score=0.74
- Line 122: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: source_fields.push_back(it.key());
  Confidence: band=high; score=0.74
- Line 302: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: code += '0';
  Confidence: band=high; score=0.74
- Line 521: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(std::move(score));
  Confidence: band=high; score=0.74
- Line 581: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: det_results.push_back(d);
  Confidence: band=high; score=0.74
- Line 581: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: det_results.push_back(d);
  Confidence: band=high; score=0.74
- Line 606: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(std::move(hmr));
  Confidence: band=high; score=0.74
- Line 616: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(std::move(hmr));
  Confidence: band=high; score=0.74
- Line 635: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(std::move(hmr));
  Confidence: band=high; score=0.74
- Line 635: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(std::move(hmr));
  Confidence: band=high; score=0.74
- Line 644: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, double> sem_map;
  Confidence: band=high; score=0.74
- Line 658: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(std::move(hmr));
  Confidence: band=high; score=0.74
- Line 658: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(std::move(hmr));
  Confidence: band=high; score=0.74
- Line 676: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(std::move(hmr));
  Confidence: band=high; score=0.74
- Line 676: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(std::move(hmr));
  Confidence: band=high; score=0.74

### src/importers/mdm_engine.cpp
Total findings: 16

- Line 125: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (std::find(key_fields.begin(), key_fields.end(), uf) == key_fields.end()) {
  Confidence: band=very_high; score=0.9
- Line 241: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it = existing_map.find(tid);
  Confidence: band=very_high; score=0.9
- Line 41: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (const auto& g : golden_records) gr_arr.push_back(g.toJson());
  Confidence: band=high; score=0.74
- Line 125: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: key_fields.push_back(uf);
  Confidence: band=high; score=0.74
- Line 125: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: key_fields.push_back(uf);
  Confidence: band=high; score=0.74
- Line 186: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: created.push_back(link);
  Confidence: band=high; score=0.74
- Line 218: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, const json*> incoming_map;
  Confidence: band=high; score=0.74
- Line 223: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, const json*> existing_map;
  Confidence: band=high; score=0.74
- Line 230: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::vector<std::string>> groups;
  Confidence: band=high; score=0.74
- Line 232: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: groups[link.source_id].push_back(link.target_id);
  Confidence: band=high; score=0.74
- Line 232: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: groups[link.source_id].push_back(link.target_id);
  Confidence: band=high; score=0.74
- Line 232: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: groups[link.source_id].push_back(link.target_id);
  Confidence: band=high; score=0.74
- Line 242: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: contributors.emplace_back(tid, *(it->second));
  Confidence: band=high; score=0.74
- Line 242: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: contributors.emplace_back(tid, *(it->second));
  Confidence: band=high; score=0.74
- Line 317: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.review_queue.push_back(e);
  Confidence: band=high; score=0.74
- Line 317: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.review_queue.push_back(e);
  Confidence: band=high; score=0.74

### src/importers/flatfile_importer.cpp
Total findings: 14

- Line 419: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cols_vec.push_back("col_" + std::to_string(i));
  Confidence: band=high; score=0.74
- Line 451: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(SchemaAutoDetector::schemaToJson(schema));
  Confidence: band=high; score=0.74
- Line 517: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: detected.columns.push_back(field->name());
  Confidence: band=high; score=0.74
- Line 615: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: fields.push_back(field);
  Confidence: band=high; score=0.74
- Line 785: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: columns.push_back("col_" + std::to_string(i));
  Confidence: band=high; score=0.74
- Line 818: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stats.warnings.push_back(ve.message);
  Confidence: band=high; score=0.74
- Line 922: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cols.push_back(key);
  Confidence: band=high; score=0.74
- Line 1034: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cols.push_back(key);
  Confidence: band=high; score=0.74
- Line 1034: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cols.push_back(key);
  Confidence: band=high; score=0.74
- Line 1048: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stats.warnings.push_back(ve.message);
  Confidence: band=high; score=0.74
- Line 1149: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: columns.push_back(std::move(col));
  Confidence: band=high; score=0.74
- Line 1212: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: row_values.emplace_back();
  Confidence: band=high; score=0.74
- Line 1285: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stats.warnings.push_back(ve.message);
  Confidence: band=high; score=0.74
- Line 1373: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stats.structured_errors.push_back(err);
  Confidence: band=high; score=0.74

### src/importers/federated_learning.cpp
Total findings: 9

- Line 48: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: values.push_back(upd.gradient[d]);
  Confidence: band=high; score=0.74
- Line 48: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: values.push_back(upd.gradient[d]);
  Confidence: band=high; score=0.74
- Line 81: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: values.push_back(upd.gradient[d]);
  Confidence: band=high; score=0.74
- Line 81: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: values.push_back(upd.gradient[d]);
  Confidence: band=high; score=0.74
- Line 122: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: for (auto it = updates[0].statistics.begin(); it != updates[0].statistics.end(); ++it) {
  Confidence: band=high; score=0.74
- Line 135: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: values.push_back(v);
  Confidence: band=high; score=0.74
- Line 135: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: values.push_back(v);
  Confidence: band=high; score=0.74
- Line 237: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(gradient[i] + mask[i]);
  Confidence: band=high; score=0.74
- Line 196: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: double sigma       = sensitivity * std::sqrt(2.0 * std::log(1.25 / delta)) / epsilon;
  Confidence: band=medium; score=0.6

### src/importers/blockchain_integrity.cpp
Total findings: 7

- Line 25: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: std::string BlockchainIntegrityVerifier::MerkleTreeBuilder::sha256Hex(const std::string &input) {
  Confidence: band=very_high; score=0.99
- Line 28: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: std::size_t h1 = std::hash<std::string>{}(input);
  Confidence: band=very_high; score=0.99
- Line 29: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: std::size_t h2 = std::hash<std::string>{}(input + "_salt");
  Confidence: band=very_high; score=0.99
- Line 25: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: std::string BlockchainIntegrityVerifier::MerkleTreeBuilder::sha256Hex(const std::string &input) {
  Confidence: band=very_high; score=0.9
- Line 28: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: std::size_t h1 = std::hash<std::string>{}(input);
  Confidence: band=very_high; score=0.9
- Line 29: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: std::size_t h2 = std::hash<std::string>{}(input + "_salt");
  Confidence: band=very_high; score=0.9
- Line 69: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: next.push_back(combineHashes(layer[i], layer[i + 1]));
  Confidence: band=high; score=0.74

### src/importers/data_quality.cpp
Total findings: 7

- Line 134: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: validity_sum += computeValidity(sample_data, col, "string"); // default type
  Confidence: band=high; score=0.74
- Line 134: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: validity_sum += computeValidity(sample_data, col, "string"); // default type
  Confidence: band=high; score=0.74
- Line 134: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: validity_sum += computeValidity(sample_data, col, "string"); // default type
  Confidence: band=high; score=0.74
- Line 200: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: table_samples[s.table_name].push_back(json{{s.column_name, v}});
  Confidence: band=high; score=0.74
- Line 200: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: table_samples[s.table_name].push_back(json{{s.column_name, v}});
  Confidence: band=high; score=0.74
- Line 218: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: report.issues.push_back("Table '" + schema.name + "' has low quality score: "
  Confidence: band=high; score=0.74
- Line 218: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: report.issues.push_back("Table '" + schema.name + "' has low quality score: "
  Confidence: band=high; score=0.74

### src/importers/huggingface_ingestion_plugin.cpp
Total findings: 7

- Line 219: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto info = response["dataset_info"];
  Confidence: band=high; score=0.74
- Line 226: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: metadata.splits.push_back(split.key());
  Confidence: band=high; score=0.74
- Line 229: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: metadata.total_rows += split.value()["num_examples"].get<size_t>();
  Confidence: band=high; score=0.74
- Line 365: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.documents.push_back(row["row"]);
  Confidence: band=high; score=0.74
- Line 428: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: docs.push_back(doc);
  Confidence: band=high; score=0.74
- Line 456: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cache_data.push_back(doc);
  Confidence: band=high; score=0.74
- Line 568: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: job.content_ids.push_back(content_spec["content"]["id"]);
  Confidence: band=high; score=0.74

### src/importers/adaptive_import.cpp
Total findings: 6

- Line 33: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: std::string parent = fk.second.substr(0, fk.second.find('.'));
  Confidence: band=very_high; score=0.9
- Line 26: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::set<std::string>> deps; // table → tables it depends on
  Confidence: band=high; score=0.74
- Line 27: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, size_t> in_degree;
  Confidence: band=high; score=0.74
- Line 55: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: order.push_back(t);
  Confidence: band=high; score=0.74
- Line 74: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: order.push_back(s.name);
  Confidence: band=high; score=0.74
- Line 74: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: order.push_back(s.name);
  Confidence: band=high; score=0.74

### src/importers/s3_importer.cpp
Total findings: 6

- Line 167: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: errors.push_back("Invalid S3 URL (expected s3://bucket/key): " +
  Confidence: band=high; score=0.74
- Line 526: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (const auto& e : obj_stats.errors)     stats.errors.push_back(e);
  Confidence: band=high; score=0.74
- Line 528: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stats.structured_errors.push_back(e);
  Confidence: band=high; score=0.74
- Line 528: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stats.structured_errors.push_back(e);
  Confidence: band=high; score=0.74
- Line 574: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: keys.push_back(k);
  Confidence: band=high; score=0.74
- Line 608: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stats.structured_errors.push_back(e);
  Confidence: band=high; score=0.74

### src/importers/polyglot_mapper.cpp
Total findings: 5

- Line 105: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: mapping.rationale.push_back(
  Confidence: band=high; score=0.74
- Line 179: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: nodes.push_back(std::move(node));
  Confidence: band=high; score=0.74
- Line 179: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: nodes.push_back(std::move(node));
  Confidence: band=high; score=0.74
- Line 179: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: nodes.push_back(std::move(node));
  Confidence: band=high; score=0.74
- Line 188: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: edges.push_back(std::move(edge));
  Confidence: band=high; score=0.74

### src/importers/column_importance.cpp
Total findings: 4

- Line 102: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it         = idx.find(key);
  Confidence: band=very_high; score=0.9
- Line 84: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::vector<std::string>> idx;
  Confidence: band=high; score=0.74
- Line 169: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: redundant.emplace_back(a.table_name + "." + a.column_name, b.table_name + "." + b.column_name);
  Confidence: band=high; score=0.74
- Line 169: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: redundant.emplace_back(a.table_name + "." + a.column_name, b.table_name + "." + b.column_name);
  Confidence: band=high; score=0.74

### src/importers/mongo_importer.cpp
Total findings: 4

- Line 288: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::string> field_types;
  Confidence: band=high; score=0.74
- Line 300: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: field_order.push_back(key);
  Confidence: band=high; score=0.74
- Line 351: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: col_arr.push_back({{"name", fname}, {"type", field_types.at(fname)}});
  Confidence: band=high; score=0.74
- Line 719: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(unwrapDocument(elem));
  Confidence: band=high; score=0.74

### src/importers/gui_import_wizard.cpp
Total findings: 3

- Line 72: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: mappings_j.push_back({
  Confidence: band=high; score=0.74
- Line 126: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: s.column_mappings.push_back(std::move(cm));
  Confidence: band=high; score=0.74
- Line 217: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: s.preview_schema.push_back({
  Confidence: band=high; score=0.74

### src/importers/mysql_importer.cpp
Total findings: 3

- Line 337: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(table_json);
  Confidence: band=high; score=0.74
- Line 590: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: col_defs.push_back(cur);
  Confidence: band=high; score=0.74
- Line 1051: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(token);
  Confidence: band=high; score=0.74

### src/importers/oracle_importer.cpp
Total findings: 3

- Line 319: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(table_json);
  Confidence: band=high; score=0.74
- Line 545: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: col_defs.push_back(cur);
  Confidence: band=high; score=0.74
- Line 948: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(token);
  Confidence: band=high; score=0.74

### src/importers/sqlite_importer.cpp
Total findings: 3

- Line 270: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(table_json);
  Confidence: band=high; score=0.74
- Line 509: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: col_defs.push_back(cur);
  Confidence: band=high; score=0.74
- Line 943: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(token);
  Confidence: band=high; score=0.74

### src/importers/canonical_resolver.cpp
Total findings: 2

- Line 351: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (best == v2) {
  Confidence: band=very_high; score=0.9
- Line 243: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: gr.contributing_ids.push_back(eid);
  Confidence: band=high; score=0.74

### src/importers/entity_linker.cpp
Total findings: 2

- Line 126: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entries.push_back(std::move(entry));
  Confidence: band=high; score=0.74
- Line 157: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: nodes.push_back(json{{"id", link.source_id}});
  Confidence: band=high; score=0.74

### src/importers/graphql_federation.cpp
Total findings: 2

- Line 110: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: key_fields += " ";
  Confidence: band=high; score=0.74
- Line 110: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: key_fields += " ";
  Confidence: band=high; score=0.74

### src/importers/mdm_audit_trail.cpp
Total findings: 2

- Line 147: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(e);
  Confidence: band=high; score=0.74
- Line 190: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: events_arr.push_back(e.toJson());
  Confidence: band=high; score=0.74

### src/importers/schema_validator.cpp
Total findings: 2

- Line 107: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cols.push_back(col);
  Confidence: band=high; score=0.74
- Line 147: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: errors.push_back(std::move(err));
  Confidence: band=high; score=0.74

### src/importers/audit_trail.cpp
Total findings: 1

- Line 84: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: events_.push_back(event);
  Confidence: band=high; score=0.74

### src/importers/postgres_importer_mdm.cpp
Total findings: 1

- Line 94: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: imported_entities.push_back(e);
  Confidence: band=high; score=0.74

### src/importers/conflict_resolver.cpp
Total findings: 0


### src/importers/kafka_importer.cpp
Total findings: 0


### src/importers/postgres_cdc.cpp
Total findings: 0

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
