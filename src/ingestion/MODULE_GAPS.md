# ingestion Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: ingestion
- Generated: 2026-06-02 11:09:13
- Status: Critical Findings Present
- Total Findings: 178
- Actionable Findings (Critical + High): 16
- Affected Files: 34

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 1 |
| High | 15 |
| Medium | 162 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| performance_patterns | 133 |
| reliability | 90 |
| container | 74 |
| performance | 46 |
| exception_safety | 30 |
| observability | 21 |
| platform | 18 |
| determinism | 13 |
| memory | 11 |
| raii | 10 |
| audit_logging | 6 |
| llm_ai_safety | 6 |
| concurrency | 4 |
| legacy_duplication | 4 |
| input_validation | 2 |
| security | 2 |
| uninitialized | 2 |
| oop_design | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/ingestion/legal_domain.cpp | 22 | 0 | 0 | 22 | 0 |
| src/ingestion/entity_assembler.cpp | 17 | 0 | 0 | 17 | 0 |
| src/ingestion/ingestion_manager.cpp | 16 | 0 | 1 | 15 | 0 |
| src/ingestion/workflow_engine.cpp | 15 | 0 | 1 | 14 | 0 |
| src/ingestion/ingestion_coordinator.cpp | 12 | 0 | 1 | 11 | 0 |
| src/ingestion/steps/ner_step.cpp | 10 | 0 | 1 | 9 | 0 |
| src/ingestion/agentic_reference_validator.cpp | 9 | 0 | 0 | 9 | 0 |
| src/ingestion/ingestion_sinks.cpp | 8 | 0 | 0 | 8 | 0 |
| src/ingestion/database_connector.cpp | 6 | 1 | 0 | 5 | 0 |
| src/ingestion/steps/llm_extract_step.cpp | 6 | 0 | 2 | 4 | 0 |
| src/ingestion/deontic_extractor.cpp | 5 | 0 | 0 | 5 | 0 |
| src/ingestion/filesystem_ingester.cpp | 5 | 0 | 0 | 5 | 0 |
| src/ingestion/llm_adapter.cpp | 5 | 0 | 2 | 3 | 0 |
| src/ingestion/steps/legal_reference_step.cpp | 5 | 0 | 1 | 4 | 0 |
| src/ingestion/api_connector.cpp | 4 | 0 | 0 | 4 | 0 |
| src/ingestion/steps/decompress_step.cpp | 4 | 0 | 1 | 3 | 0 |
| src/ingestion/cdc_connector.cpp | 3 | 0 | 1 | 2 | 0 |
| src/ingestion/s3_connector.cpp | 3 | 0 | 0 | 3 | 0 |
| src/ingestion/semantic_validator.cpp | 3 | 0 | 0 | 3 | 0 |
| src/ingestion/steps/chunk_embed_step.cpp | 3 | 0 | 1 | 2 | 0 |
| src/ingestion/steps/chunk_tt_decompose_step.cpp | 3 | 0 | 1 | 2 | 0 |
| src/ingestion/web_crawler_connector.cpp | 3 | 0 | 0 | 3 | 0 |
| src/ingestion/huggingface_connector.cpp | 2 | 0 | 1 | 1 | 0 |
| src/ingestion/ingestion_quality_judge.cpp | 2 | 0 | 0 | 2 | 0 |
| src/ingestion/steps/base_entity_assembler_step.cpp | 2 | 0 | 0 | 2 | 0 |
| src/ingestion/steps/tensor_core_bridge_step.cpp | 2 | 0 | 1 | 1 | 0 |
| src/ingestion/steps/chunk_text_step.cpp | 1 | 0 | 0 | 1 | 0 |
| src/ingestion/steps/format_parse_step.cpp | 1 | 0 | 0 | 1 | 0 |
| src/ingestion/steps/legal_metadata_step.cpp | 1 | 0 | 0 | 1 | 0 |
| src/ingestion/kafka_connector.cpp | 0 | 0 | 0 | 0 | 0 |
| src/ingestion/oauth_token_manager.cpp | 0 | 0 | 0 | 0 | 0 |
| src/ingestion/object_storage_connector.cpp | 0 | 0 | 0 | 0 | 0 |
| src/ingestion/steps/deontic_step.cpp | 0 | 0 | 0 | 0 | 0 |
| src/ingestion/steps/parse_text_step.cpp | 0 | 0 | 0 | 0 | 0 |

## Full Scanner Findings

### src/ingestion/legal_domain.cpp
Total findings: 22

- Line 187: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: teile.emplace_back(it->position(), std::move(tn));
  Confidence: band=high; score=0.74
- Line 194: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: hier.root.children.push_back(std::move(para));
  Confidence: band=high; score=0.74
- Line 211: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: target_teil->children.push_back(std::move(para));
  Confidence: band=high; score=0.74
- Line 211: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: target_teil->children.push_back(std::move(para));
  Confidence: band=high; score=0.74
- Line 219: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: hier.root.children.push_back(std::move(tn));
  Confidence: band=high; score=0.74
- Line 243: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: match_starts.push_back(static_cast<std::size_t>((*it).position()));
  Confidence: band=high; score=0.74
- Line 250: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto body_start = headers[i].second;
  Confidence: band=high; score=0.74
- Line 287: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(node));
  Confidence: band=high; score=0.74
- Line 338: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entities.push_back(std::move(ae));
  Confidence: band=high; score=0.74
- Line 559: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (!item.empty()) be.auflagen.push_back(item);
  Confidence: band=high; score=0.74
- Line 567: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (!nb.empty()) be.nebenbestimmungen.push_back(nb);
  Confidence: band=high; score=0.74
- Line 594: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) auflagen_str += "; ";
  Confidence: band=high; score=0.74
- Line 594: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) auflagen_str += "; ";
  Confidence: band=high; score=0.74
- Line 630: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, const BaseEntity*> id_map;
  Confidence: band=medium; score=0.66
- Line 650: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: edges.push_back(std::move(rel));
  Confidence: band=high; score=0.74
- Line 650: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: edges.push_back(std::move(rel));
  Confidence: band=high; score=0.74
- Line 669: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: edges.push_back(std::move(rel));
  Confidence: band=high; score=0.74
- Line 719: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: case '"':  out += "\\\""; break;
  Confidence: band=high; score=0.74
- Line 719: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: case '"':  out += "\\\""; break;
  Confidence: band=high; score=0.74
- Line 767: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: graph.push_back(std::move(node));
  Confidence: band=high; score=0.74
- Line 767: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: graph.push_back(std::move(node));
  Confidence: band=high; score=0.74
- Line 787: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: graph.push_back(std::move(edge));
  Confidence: band=high; score=0.74

### src/ingestion/entity_assembler.cpp
Total findings: 17

- Line 33: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<char>(std::tolower(c)));
  Confidence: band=high; score=0.74
- Line 59: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::string>
  Confidence: band=medium; score=0.66
- Line 68: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::string> parts;
  Confidence: band=medium; score=0.66
- Line 198: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::size_t> id_to_idx;
  Confidence: band=medium; score=0.66
- Line 205: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: deduped.push_back(std::move(ent));
  Confidence: band=high; score=0.74
- Line 266: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ctx.relations.push_back(std::move(r));
  Confidence: band=high; score=0.74
- Line 297: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::size_t> id_map;
  Confidence: band=medium; score=0.66
- Line 311: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ctx.relations.push_back(std::move(r));
  Confidence: band=high; score=0.74
- Line 311: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ctx.relations.push_back(std::move(r));
  Confidence: band=high; score=0.74
- Line 320: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::vector<std::string>> by_section;
  Confidence: band=medium; score=0.66
- Line 323: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: by_section[ref].push_back(ent.id);
  Confidence: band=high; score=0.74
- Line 336: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ctx.relations.push_back(std::move(r));
  Confidence: band=high; score=0.74
- Line 336: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ctx.relations.push_back(std::move(r));
  Confidence: band=high; score=0.74
- Line 336: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ctx.relations.push_back(std::move(r));
  Confidence: band=high; score=0.74
- Line 347: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::string> authority_by_text;
  Confidence: band=medium; score=0.66
- Line 366: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ctx.relations.push_back(std::move(r));
  Confidence: band=high; score=0.74
- Line 366: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ctx.relations.push_back(std::move(r));
  Confidence: band=high; score=0.74

### src/ingestion/ingestion_manager.cpp
Total findings: 16

- Line 95: severity=HIGH; category=performance; pattern=regex_in_loop
  Description: std::regex compiled in loop (compile once, reuse)
  Context: for (char c : s) {
  Confidence: band=very_high; score=0.9
- Line 95: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (kMeta.find(c) != std::string::npos) out += '\\';
  Confidence: band=high; score=0.74
- Line 210: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: compiled_fields.emplace_back(kv.first, kv.second);
  Confidence: band=high; score=0.74
- Line 322: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += '_';
  Confidence: band=high; score=0.74
- Line 932: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: all_sources.push_back(pair.second);
  Confidence: band=high; score=0.74
- Line 967: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: futures.push_back(
  Confidence: band=high; score=0.74
- Line 1015: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(pair.second);
  Confidence: band=high; score=0.74
- Line 1015: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(pair.second);
  Confidence: band=high; score=0.74
- Line 1369: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: quarantine_.push_back(std::move(entry));
  Confidence: band=high; score=0.74
- Line 1441: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: names.push_back(kv.first);
  Confidence: band=high; score=0.74
- Line 1663: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.warnings.push_back(w);
  Confidence: band=high; score=0.74
- Line 1725: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (c == '\\') out += "\\\\";
  Confidence: band=high; score=0.74
- Line 1879: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<int, size_t> code_counts;
  Confidence: band=medium; score=0.66
- Line 1884: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: labels.push_back({"error_code", std::to_string(code_int)});
  Confidence: band=high; score=0.74
- Line 1884: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: labels.push_back({"error_code", std::to_string(code_int)});
  Confidence: band=high; score=0.74
- Line 2169: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(s));
  Confidence: band=high; score=0.74

### src/ingestion/workflow_engine.cpp
Total findings: 15

- Line 708: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: Result<BaseEntitySet> WorkflowEngine::execute(ExtractionContext& ctx) {
  Confidence: band=very_high; score=0.9
- Line 148: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, Entry> steps_;
  Confidence: band=medium; score=0.66
- Line 350: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (m.is_string()) p.file_patterns.mime_types.push_back(m);
  Confidence: band=high; score=0.74
- Line 355: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (fn.is_string()) p.file_patterns.filename_patterns.push_back(fn);
  Confidence: band=high; score=0.74
- Line 371: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: p.steps.push_back(std::move(sc));
  Confidence: band=high; score=0.74
- Line 415: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (item.IsScalar()) arr.push_back(item.as<std::string>(""));
  Confidence: band=high; score=0.74
- Line 415: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (item.IsScalar()) arr.push_back(item.as<std::string>(""));
  Confidence: band=high; score=0.74
- Line 447: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (m.IsScalar()) p.file_patterns.mime_types.push_back(m.as<std::string>(""));
  Confidence: band=high; score=0.74
- Line 452: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (f.IsScalar()) p.file_patterns.filename_patterns.push_back(f.as<std::string>(""));
  Confidence: band=high; score=0.74
- Line 468: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: p.steps.push_back(std::move(sc));
  Confidence: band=high; score=0.74
- Line 514: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ctx.warnings.push_back("Step '" + step_cfg.name
  Confidence: band=high; score=0.74
- Line 514: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ctx.warnings.push_back("Step '" + step_cfg.name
  Confidence: band=high; score=0.74
- Line 573: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ctx.warnings.push_back("Quality gate: only "
  Confidence: band=high; score=0.74
- Line 659: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: impl_->profiles_.push_back(std::move(profile));
  Confidence: band=high; score=0.74
- Line 708: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: Result<BaseEntitySet> WorkflowEngine::execute(ExtractionContext& ctx) {
  Confidence: band=high; score=0.74

### src/ingestion/ingestion_coordinator.cpp
Total findings: 12

- Line 276: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t i = 1; i < n; ++i) {
  Confidence: band=very_high; score=0.9
- Line 169: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: ConsistentHashRing::removeNode(const std::string& node_id)
  Context: void ConsistentHashRing::removeNode(const std::string& node_id) {
  Confidence: band=medium; score=0.56
- Line 181: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: ConsistentHashRing::getNode(const std::string& key)
  Context: std::string ConsistentHashRing::getNode(const std::string& key) const {
  Confidence: band=medium; score=0.56
- Line 336: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: threads.emplace_back(&WorkStealingPool::workerFn, this, i, cb);
  Confidence: band=high; score=0.74
- Line 403: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: nodes_.push_back(node);
  Confidence: band=high; score=0.74
- Line 462: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(info));
  Confidence: band=high; score=0.74
- Line 477: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::vector<SourceConfig>>
  Confidence: band=medium; score=0.66
- Line 483: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::vector<SourceConfig>> partitions;
  Confidence: band=medium; score=0.66
- Line 493: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: partitions[node_id].push_back(src);
  Confidence: band=high; score=0.74
- Line 722: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.quarantine.push_back(q);
  Confidence: band=high; score=0.74
- Line 722: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.quarantine.push_back(q);
  Confidence: band=high; score=0.74
- Line 722: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.quarantine.push_back(q);
  Confidence: band=high; score=0.74

### src/ingestion/steps/ner_step.cpp
Total findings: 10

- Line 177: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: Result<void> execute(ExtractionContext& ctx, const StepConfig& cfg) override {
  Confidence: band=very_high; score=0.9
- Line 66: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back({it->str(), "LAW",
  Confidence: band=high; score=0.74
- Line 78: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back({it->str(), "DATE",
  Confidence: band=high; score=0.74
- Line 90: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back({it->str(), "LAW",
  Confidence: band=high; score=0.74
- Line 110: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (!m.text.empty()) out.push_back(std::move(m));
  Confidence: band=high; score=0.74
- Line 121: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: for (const auto& t : types) type_list += t + ", ";
  Confidence: band=high; score=0.74
- Line 177: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: Result<void> execute(ExtractionContext& ctx, const StepConfig& cfg) override {
  Confidence: band=high; score=0.74
- Line 185: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: requested_types.push_back(t.get<std::string>());
  Confidence: band=high; score=0.74
- Line 195: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: texts_with_refs.emplace_back(c.text, c.section_ref);
  Confidence: band=high; score=0.74
- Line 232: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ctx.warnings.push_back(
  Confidence: band=high; score=0.74

### src/ingestion/agentic_reference_validator.cpp
Total findings: 9

- Line 95: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: report.validated.push_back(vr);
  Confidence: band=high; score=0.74
- Line 128: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (!dup) refs.push_back(std::move(ref));
  Confidence: band=high; score=0.74
- Line 128: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (!dup) refs.push_back(std::move(ref));
  Confidence: band=high; score=0.74
- Line 146: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: refs.push_back(std::move(ref));
  Confidence: band=high; score=0.74
- Line 146: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: refs.push_back(std::move(ref));
  Confidence: band=high; score=0.74
- Line 165: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (!dup) refs.push_back(std::move(ref));
  Confidence: band=high; score=0.74
- Line 165: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (!dup) refs.push_back(std::move(ref));
  Confidence: band=high; score=0.74
- Line 182: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (!dup) refs.push_back(std::move(ref));
  Confidence: band=high; score=0.74
- Line 182: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (!dup) refs.push_back(std::move(ref));
  Confidence: band=high; score=0.74

### src/ingestion/ingestion_sinks.cpp
Total findings: 8

- Line 126: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: edges_.push_back(e);
  Confidence: band=high; score=0.74
- Line 201: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: nodes_arr.push_back(std::move(nj));
  Confidence: band=high; score=0.74
- Line 212: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: edges_arr.push_back(std::move(ej));
  Confidence: band=high; score=0.74
- Line 306: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: nodes.push_back(std::move(ej));
  Confidence: band=high; score=0.74
- Line 306: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: nodes.push_back(std::move(ej));
  Confidence: band=high; score=0.74
- Line 323: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: edges.push_back(std::move(rj));
  Confidence: band=high; score=0.74
- Line 323: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: edges.push_back(std::move(rj));
  Confidence: band=high; score=0.74
- Line 339: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks.push_back(std::move(cj));
  Confidence: band=high; score=0.74

### src/ingestion/database_connector.cpp
Total findings: 6

- Line 173: severity=CRITICAL; category=audit_logging; pattern=sensitive_data_logging
  Description: Potential PII/credential logging: password
  Context: if (!password.empty()) cs << "PWD=" << password << ";";
  Confidence: band=very_high; score=0.92
- Line 215: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (c == '"')  { out += "\\\""; }
  Confidence: band=high; score=0.74
- Line 215: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (c == '"')  { out += "\\\""; }
  Confidence: band=high; score=0.74
- Line 259: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(token.substr(b, e - b + 1));
  Confidence: band=high; score=0.74
- Line 569: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: "SQLAllocHandle(STMT) failed",
  Confidence: band=high; score=0.74
- Line 606: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: col_names.push_back(std::string(reinterpret_cast<char*>(name),
  Confidence: band=high; score=0.74

### src/ingestion/steps/llm_extract_step.cpp
Total findings: 6

- Line 12: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: #include "ingestion/inference_backend.h"
  Confidence: band=very_high; score=0.9
- Line 90: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: Result<void> execute(ExtractionContext& ctx, const StepConfig& cfg) override {
  Confidence: band=very_high; score=0.9
- Line 81: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: bool canHandle(const ExtractionContext& ctx) const override {
  Confidence: band=high; score=0.74
- Line 90: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: Result<void> execute(ExtractionContext& ctx, const StepConfig& cfg) override {
  Confidence: band=high; score=0.74
- Line 127: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: backend_->generate(prompt, max_tokens, temperature, lora);
  Confidence: band=high; score=0.74
- Line 200: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ctx.entities.push_back(std::move(ent));
  Confidence: band=high; score=0.74

### src/ingestion/deontic_extractor.cpp
Total findings: 5

- Line 183: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.deontic_categories.push_back(dp.category);
  Confidence: band=high; score=0.74
- Line 233: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.obligations.emplace_back(actor, action, "", 0.75);
  Confidence: band=high; score=0.74
- Line 260: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entities.emplace_back(ep.type, raw, raw, 0.85);
  Confidence: band=high; score=0.74
- Line 260: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entities.emplace_back(ep.type, raw, raw, 0.85);
  Confidence: band=high; score=0.74
- Line 260: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entities.emplace_back(ep.type, raw, raw, 0.85);
  Confidence: band=high; score=0.74

### src/ingestion/filesystem_ingester.cpp
Total findings: 5

- Line 193: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: result += ' ';
  Confidence: band=high; score=0.74
- Line 241: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (c == '"') escaped += "\\\"";
  Confidence: band=high; score=0.74
- Line 252: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (c == '\'') escaped += "'\"'\"'";
  Confidence: band=high; score=0.74
- Line 451: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: files_to_process.push_back(entry.path());
  Confidence: band=high; score=0.74
- Line 459: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: files_to_process.push_back(entry.path());
  Confidence: band=high; score=0.74

### src/ingestion/llm_adapter.cpp
Total findings: 5

- Line 17: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // See include/ingestion/inference_backend.h and llm/llm_ingestion_bridge.h.
  Confidence: band=very_high; score=0.9
- Line 61: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: const std::string response = captured_backend->generate(
  Confidence: band=very_high; score=0.9
- Line 61: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: const std::string response = captured_backend->generate(
  Confidence: band=high; score=0.74
- Line 70: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: std::string("ITextGenerationBackend::generate() threw: ") + e.what());
  Confidence: band=high; score=0.74
- Line 161: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.entities.emplace_back(type_str, value_str, value_str, 0.85);
  Confidence: band=high; score=0.74

### src/ingestion/steps/legal_reference_step.cpp
Total findings: 5

- Line 52: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: Result<void> execute(ExtractionContext& ctx, const StepConfig& cfg) override {
  Confidence: band=very_high; score=0.9
- Line 52: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: Result<void> execute(ExtractionContext& ctx, const StepConfig& cfg) override {
  Confidence: band=high; score=0.74
- Line 84: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: warnings_arr.push_back(w);
  Confidence: band=high; score=0.74
- Line 84: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: warnings_arr.push_back(w);
  Confidence: band=high; score=0.74
- Line 91: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ctx.warnings.push_back(
  Confidence: band=high; score=0.74

### src/ingestion/api_connector.cpp
Total findings: 4

- Line 180: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stats.errors.push_back(err);
  Confidence: band=high; score=0.74
- Line 234: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (!value.empty()) results.push_back(std::move(value));
  Confidence: band=high; score=0.74
- Line 550: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: body += "&client_id=" + urlEncode(oauth_config_.client_id);
  Confidence: band=high; score=0.74
- Line 596: severity=MEDIUM; category=observability; pattern=missing_health_check
  Description: Service initialization without nearby health/status handling
  Context: bool GenericApiConnector::initialize(const SourceConfig& config) {
  Confidence: band=medium; score=0.66

### src/ingestion/steps/decompress_step.cpp
Total findings: 4

- Line 130: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: Result<void> execute(ExtractionContext& ctx, const StepConfig& cfg) override {
  Confidence: band=very_high; score=0.9
- Line 61: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: static bool runProcess(const std::vector<const char*>& argv_vec) {
  Confidence: band=high; score=0.74
- Line 98: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(entry.path().string());
  Confidence: band=high; score=0.74
- Line 130: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: Result<void> execute(ExtractionContext& ctx, const StepConfig& cfg) override {
  Confidence: band=high; score=0.74

### src/ingestion/cdc_connector.cpp
Total findings: 3

- Line 180: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: /// PostgreSQL itself outputs LSN values in uppercase hexadecimal (e.g. `0/16E0478`);
  Confidence: band=very_high; score=0.9
- Line 74: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_map<std::string, std::string>& m) {
  Confidence: band=medium; score=0.66
- Line 135: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(token.substr(b, e - b + 1));
  Confidence: band=high; score=0.74

### src/ingestion/s3_connector.cpp
Total findings: 3

- Line 425: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: futs.push_back(std::async(std::launch::async, [&fetcher, key]() {
  Confidence: band=high; score=0.74
- Line 508: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: safe_keys.push_back(k);
  Confidence: band=high; score=0.74
- Line 619: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: keys.push_back(obj.GetKey());
  Confidence: band=high; score=0.74

### src/ingestion/semantic_validator.cpp
Total findings: 3

- Line 173: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.warnings.push_back(w);
  Confidence: band=high; score=0.74
- Line 279: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: doc_result.provisions.push_back(std::move(prov));
  Confidence: band=high; score=0.74
- Line 284: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: doc_result.warnings.push_back(w);
  Confidence: band=high; score=0.74

### src/ingestion/steps/chunk_embed_step.cpp
Total findings: 3

- Line 56: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: Result<void> execute(ExtractionContext& ctx, const StepConfig& cfg) override {
  Confidence: band=very_high; score=0.9
- Line 56: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: Result<void> execute(ExtractionContext& ctx, const StepConfig& cfg) override {
  Confidence: band=high; score=0.74
- Line 115: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ctx.embeddings.push_back(std::move(rec));
  Confidence: band=high; score=0.74

### src/ingestion/steps/chunk_tt_decompose_step.cpp
Total findings: 3

- Line 69: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: Result<void> execute(ExtractionContext& ctx, const StepConfig& cfg) override {
  Confidence: band=very_high; score=0.9
- Line 69: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: Result<void> execute(ExtractionContext& ctx, const StepConfig& cfg) override {
  Confidence: band=high; score=0.74
- Line 132: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ctx.warnings.push_back(
  Confidence: band=high; score=0.74

### src/ingestion/web_crawler_connector.cpp
Total findings: 3

- Line 145: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!text.empty() && text.back() != ' ') text += ' ';
  Confidence: band=high; score=0.74
- Line 145: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!text.empty() && text.back() != ' ') text += ' ';
  Confidence: band=high; score=0.74
- Line 169: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!prev_space) { result += ' '; prev_space = true; }
  Confidence: band=high; score=0.74

### src/ingestion/huggingface_connector.cpp
Total findings: 2

- Line 675: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // (degraded mode) to preserve backward compatibility with existing connectors.
  Confidence: band=high; score=0.8
- Line 439: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: body += "&client_id=" + urlEncode(oauth_config_.client_id);
  Confidence: band=high; score=0.74

### src/ingestion/ingestion_quality_judge.cpp
Total findings: 2

- Line 528: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: unique_steps.push_back(s);
  Confidence: band=high; score=0.74
- Line 635: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.history.push_back(report);
  Confidence: band=high; score=0.74

### src/ingestion/steps/base_entity_assembler_step.cpp
Total findings: 2

- Line 68: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::size_t> id_to_idx;
  Confidence: band=medium; score=0.66
- Line 75: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: deduped.push_back(std::move(ent));
  Confidence: band=high; score=0.74

### src/ingestion/steps/tensor_core_bridge_step.cpp
Total findings: 2

- Line 70: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: Result<void> execute(ExtractionContext& ctx, const StepConfig& cfg) override {
  Confidence: band=very_high; score=0.9
- Line 70: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: Result<void> execute(ExtractionContext& ctx, const StepConfig& cfg) override {
  Confidence: band=high; score=0.74

### src/ingestion/steps/chunk_text_step.cpp
Total findings: 1

- Line 174: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ctx.chunks.push_back(std::move(c));
  Confidence: band=high; score=0.74

### src/ingestion/steps/format_parse_step.cpp
Total findings: 1

- Line 72: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: bool canHandle(const ExtractionContext& ctx) const override {
  Confidence: band=high; score=0.74

### src/ingestion/steps/legal_metadata_step.cpp
Total findings: 1

- Line 95: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ctx.entities.push_back(std::move(ent));
  Confidence: band=high; score=0.74

### src/ingestion/kafka_connector.cpp
Total findings: 0


### src/ingestion/oauth_token_manager.cpp
Total findings: 0


### src/ingestion/object_storage_connector.cpp
Total findings: 0


### src/ingestion/steps/deontic_step.cpp
Total findings: 0


### src/ingestion/steps/parse_text_step.cpp
Total findings: 0

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
