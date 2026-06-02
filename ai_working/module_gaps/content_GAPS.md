# content Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: content
- Generated: 2026-06-02 11:09:12
- Status: Critical Findings Present
- Total Findings: 172
- Actionable Findings (Critical + High): 10
- Affected Files: 36

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 1 |
| High | 9 |
| Medium | 157 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| performance_patterns | 134 |
| memory | 87 |
| reliability | 53 |
| container | 46 |
| exception_safety | 28 |
| concurrency | 27 |
| performance | 21 |
| platform | 21 |
| raii | 17 |
| determinism | 15 |
| security | 15 |
| type_conversion | 15 |
| observability | 11 |
| audit_logging | 7 |
| legacy_duplication | 6 |
| distributed_consistency | 5 |
| llm_ai_safety | 5 |
| uninitialized | 2 |
| input_validation | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/content/content_manager.cpp | 33 | 0 | 1 | 32 | 0 |
| src/content/mime_detector.cpp | 17 | 0 | 0 | 17 | 0 |
| src/content/stt_processor.cpp | 13 | 0 | 1 | 12 | 0 |
| src/content/async_ingestion_worker.cpp | 11 | 1 | 2 | 8 | 0 |
| src/content/office_processor.cpp | 11 | 0 | 1 | 10 | 0 |
| src/content/video_processor.cpp | 7 | 0 | 0 | 7 | 0 |
| src/content/geo_processor.cpp | 6 | 0 | 0 | 6 | 0 |
| src/content/pipeline/async_bulk_uploader.cpp | 6 | 0 | 0 | 6 | 0 |
| src/content/content_logger.cpp | 5 | 0 | 0 | 0 | 5 |
| src/content/content_manager_llm.cpp | 5 | 0 | 0 | 5 | 0 |
| src/content/pdf_processor.cpp | 5 | 0 | 0 | 5 | 0 |
| src/content/pipeline/multimodal_chunker.cpp | 5 | 0 | 0 | 5 | 0 |
| src/content/archive_processor.cpp | 4 | 0 | 0 | 4 | 0 |
| src/content/audio_processor.cpp | 4 | 0 | 0 | 4 | 0 |
| src/content/cad_processor.cpp | 4 | 0 | 1 | 3 | 0 |
| src/content/image_processor.cpp | 4 | 0 | 0 | 4 | 0 |
| src/content/abuse_detector.cpp | 3 | 0 | 1 | 2 | 0 |
| src/content/content_type.cpp | 3 | 0 | 0 | 3 | 0 |
| src/content/html_processor.cpp | 3 | 0 | 0 | 3 | 0 |
| src/content/language_detector.cpp | 3 | 0 | 1 | 2 | 0 |
| src/content/ocr_processor.cpp | 3 | 0 | 0 | 3 | 0 |
| src/content/text_processor.cpp | 3 | 0 | 0 | 3 | 0 |
| src/content/content_metrics.cpp | 2 | 0 | 0 | 2 | 0 |
| src/content/content_security.cpp | 2 | 0 | 0 | 2 | 0 |
| src/content/markdown_processor.cpp | 2 | 0 | 0 | 2 | 0 |
| src/content/pipeline/bulk_upload_interface.cpp | 2 | 0 | 0 | 2 | 0 |
| src/content/adapters/format_extractor_factory.cpp | 1 | 0 | 0 | 1 | 0 |
| src/content/content_validator.cpp | 1 | 0 | 0 | 1 | 0 |
| src/content/pipeline/content_chunker.cpp | 1 | 0 | 0 | 1 | 0 |
| src/content/pipeline/zstd_compression.cpp | 1 | 0 | 1 | 0 | 0 |
| src/content/tts_processor.cpp | 1 | 0 | 0 | 1 | 0 |
| src/content/version_manager.cpp | 1 | 0 | 0 | 1 | 0 |
| src/content/content_fs.cpp | 0 | 0 | 0 | 0 | 0 |
| src/content/deduplication_checker.cpp | 0 | 0 | 0 | 0 | 0 |
| src/content/embedding_pipeline.cpp | 0 | 0 | 0 | 0 | 0 |
| src/content/mock_clip_processor.cpp | 0 | 0 | 0 | 0 | 0 |

## Full Scanner Findings

### src/content/content_manager.cpp
Total findings: 33

- Line 1576: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (beta != 0.0) {
  Confidence: band=very_high; score=0.9
- Line 78: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(hex[c >> 4]);
  Confidence: band=high; score=0.74
- Line 150: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> allowedMimes;
  Confidence: band=medium; score=0.66
- Line 151: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, json> wantedMeta;
  Confidence: band=medium; score=0.66
- Line 152: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> wantedTags;
  Confidence: band=medium; score=0.66
- Line 180: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: for (auto it = filters["metadata"].begin(); it != filters["metadata"].end(); ++it) {
  Confidence: band=high; score=0.74
- Line 202: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::string> fieldMap;
  Confidence: band=medium; score=0.66
- Line 209: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: for (auto it = sc["field_map"].begin(); it != sc["field_map"].end(); ++it) {
  Confidence: band=high; score=0.74
- Line 360: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (cid.is_string()) whitelist.push_back(std::string("chunks:") + cid.get<std::string>());
  Confidence: band=high; score=0.74
- Line 631: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: threat_info += r.threat_name + " (" + r.scanner_name + "); ";
  Confidence: band=high; score=0.74
- Line 813: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto ftcfg = cj["fulltext_config"];
  Confidence: band=high; score=0.74
- Line 819: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (sw.is_string()) fulltext_config.stopwords.push_back(sw.get<std::string>());
  Confidence: band=high; score=0.74
- Line 1081: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto enc_section = j["_encrypted_meta"];
  Confidence: band=high; score=0.74
- Line 1217: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(ChunkMeta::fromJson(j));
  Confidence: band=high; score=0.74
- Line 1350: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(chunk);
  Confidence: band=high; score=0.74
- Line 1374: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: res.emplace_back(r.pk, r.distance);
  Confidence: band=high; score=0.74
- Line 1391: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, float> vector_scores;
  Confidence: band=medium; score=0.66
- Line 1392: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, size_t> vector_ranks;
  Confidence: band=medium; score=0.66
- Line 1427: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, float> fulltext_scores;
  Confidence: band=medium; score=0.66
- Line 1428: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, size_t> fulltext_ranks;
  Confidence: band=medium; score=0.66
- Line 1491: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, float> rrf_scores;
  Confidence: band=medium; score=0.66
- Line 1506: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.emplace_back(chunk_id, score);
  Confidence: band=high; score=0.74
- Line 1506: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.emplace_back(chunk_id, score);
  Confidence: band=high; score=0.74
- Line 1506: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.emplace_back(chunk_id, score);
  Confidence: band=high; score=0.74
- Line 1543: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, double> bestScore; bestScore.reserve(base.size()*2);
  Confidence: band=medium; score=0.66
- Line 1565: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> seen;
  Confidence: band=medium; score=0.66
- Line 1593: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.emplace_back(kv.first, static_cast<float>(kv.second));
  Confidence: band=high; score=0.74
- Line 1600: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> allowed(allow.begin(), allow.end());
  Confidence: band=medium; score=0.66
- Line 2239: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks_json.push_back(cm.toJson());
  Confidence: band=high; score=0.74
- Line 2318: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks_json.push_back(cm.toJson());
  Confidence: band=high; score=0.74
- Line 2399: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks_json.push_back(cm.toJson());
  Confidence: band=high; score=0.74
- Line 2672: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto ftcfg = cj["fulltext_config"];
  Confidence: band=high; score=0.74
- Line 2678: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (sw.is_string()) fulltext_config.stopwords.push_back(sw.get<std::string>());
  Confidence: band=high; score=0.74

### src/content/mime_detector.cpp
Total findings: 17

- Line 114: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: sig.signature.push_back(static_cast<uint8_t>(byte.as<int>()));
  Confidence: band=high; score=0.74
- Line 114: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: sig.signature.push_back(static_cast<uint8_t>(byte.as<int>()));
  Confidence: band=high; score=0.74
- Line 130: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: magic_signatures_.push_back(std::move(sig));
  Confidence: band=high; score=0.74
- Line 172: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: policy_.allowed.push_back(std::move(mp));
  Confidence: band=high; score=0.74
- Line 184: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: policy_.denied.push_back(std::move(mp));
  Confidence: band=high; score=0.74
- Line 310: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: buffer += "[extensions]\n";
  Confidence: band=high; score=0.74
- Line 310: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: buffer += "[extensions]\n";
  Confidence: band=high; score=0.74
- Line 326: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: wildcards += ":";
  Confidence: band=high; score=0.74
- Line 326: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: wildcards += ":";
  Confidence: band=high; score=0.74
- Line 326: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: wildcards += ":";
  Confidence: band=high; score=0.74
- Line 329: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!first) wildcards += ",";
  Confidence: band=high; score=0.74
- Line 334: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: magic_lines.push_back(sig.mime_type + "@" + std::to_string(sig.offset) + "=" + hex + wildcards);
  Confidence: band=high; score=0.74
- Line 348: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i) joined += ",";
  Confidence: band=high; score=0.74
- Line 348: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i) joined += ",";
  Confidence: band=high; score=0.74
- Line 348: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i) joined += ",";
  Confidence: band=high; score=0.74
- Line 351: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: category_lines.push_back(cat.first + "=" + joined);
  Confidence: band=high; score=0.74
- Line 351: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: category_lines.push_back(cat.first + "=" + joined);
  Confidence: band=high; score=0.74

### src/content/stt_processor.cpp
Total findings: 13

- Line 990: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // Assumes both inputs are L2-normalised; result is in [-1, 1].
  Confidence: band=very_high; score=0.9
- Line 109: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: bool STTProcessor::canProcess(const std::string &mime_type) const {
  Confidence: band=high; score=0.74
- Line 181: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: segments_json.push_back(seg_json);
  Confidence: band=high; score=0.74
- Line 233: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks.push_back(chunk);
  Confidence: band=high; score=0.74
- Line 244: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: current_chunk += " ";
  Confidence: band=high; score=0.74
- Line 276: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks.push_back(chunk);
  Confidence: band=high; score=0.74
- Line 282: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: current_chunk += " ";
  Confidence: band=high; score=0.74
- Line 808: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.segments.push_back(segment);
  Confidence: band=high; score=0.74
- Line 1005: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: features.push_back(std::move(fv));
  Confidence: band=high; score=0.74
- Line 1048: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: centroids.push_back(features[best_idx]);
  Confidence: band=high; score=0.74
- Line 1048: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: centroids.push_back(features[best_idx]);
  Confidence: band=high; score=0.74
- Line 1048: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: centroids.push_back(features[best_idx]);
  Confidence: band=high; score=0.74
- Line 1132: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: segments.push_back(seg_json);
  Confidence: band=high; score=0.74

### src/content/async_ingestion_worker.cpp
Total findings: 11

- Line 913: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: job.config.merge_patch(additional_config);
  Confidence: band=very_high; score=0.99
- Line 139: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: cleanup_thread_ = std::thread(&AsyncIngestionWorker::cleanupLoop, this);
  Confidence: band=very_high; score=0.9
- Line 913: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: job.config.merge_patch(additional_config);
  Confidence: band=very_high; score=0.9
- Line 133: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: workers_.emplace_back(&AsyncIngestionWorker::workerLoop, this, static_cast<int>(i));
  Confidence: band=high; score=0.74
- Line 276: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: && !queue_depth_high_watermark_.compare_exchange_weak(old_hwm, depth, std::memory_order_relaxed)) {
  Confidence: band=high; score=0.74
- Line 338: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: && !queue_depth_high_watermark_.compare_exchange_weak(old_hwm, depth, std::memory_order_relaxed)) {
  Confidence: band=high; score=0.74
- Line 413: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: file_list.push_back({{"filename", filename}, {"size", blob.size()}});
  Confidence: band=high; score=0.74
- Line 420: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: job.config["_blobs"].push_back({{"filename", filename}, {"blob", blob}});
  Confidence: band=high; score=0.74
- Line 483: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(job);
  Confidence: band=high; score=0.74
- Line 805: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: job.content_ids.push_back(result.primary_content_id);
  Confidence: band=high; score=0.74
- Line 860: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: names.push_back(name);
  Confidence: band=high; score=0.74

### src/content/office_processor.cpp
Total findings: 11

- Line 589: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: } else if (std::string(child.name()).find("text:") == 0) {
  Confidence: band=very_high; score=0.9
- Line 209: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: text += content + " ";
  Confidence: band=high; score=0.74
- Line 282: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: paragraphs.push_back(para_text);
  Confidence: band=high; score=0.74
- Line 282: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: paragraphs.push_back(para_text);
  Confidence: band=high; score=0.74
- Line 341: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: shared_strings.push_back(text);
  Confidence: band=high; score=0.74
- Line 341: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: shared_strings.push_back(text);
  Confidence: band=high; score=0.74
- Line 362: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: sheet_names.push_back(name);
  Confidence: band=high; score=0.74
- Line 466: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: slides.push_back(entry);
  Confidence: band=high; score=0.74
- Line 726: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entries.push_back(name);
  Confidence: band=high; score=0.74
- Line 1070: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks.push_back(chunk);
  Confidence: band=high; score=0.74
- Line 1077: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: current_chunk += "\n";
  Confidence: band=high; score=0.74

### src/content/video_processor.cpp
Total findings: 7

- Line 143: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: bool VideoProcessor::canProcess(const std::string &mime_type) const {
  Confidence: band=high; score=0.74
- Line 234: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: kf_times.push_back(time);
  Confidence: band=high; score=0.74
- Line 255: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: scene_times.push_back(time);
  Confidence: band=high; score=0.74
- Line 451: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: keyframes.push_back(i * interval_ms);
  Confidence: band=high; score=0.74
- Line 963: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: scenes.push_back(pts_ms);
  Confidence: band=high; score=0.74
- Line 963: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: scenes.push_back(pts_ms);
  Confidence: band=high; score=0.74
- Line 963: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: scenes.push_back(pts_ms);
  Confidence: band=high; score=0.74

### src/content/geo_processor.cpp
Total findings: 6

- Line 135: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: bool GeoProcessor::canProcess(const std::string& mime_type) const {
  Confidence: band=high; score=0.74
- Line 292: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks.push_back(chunk);
  Confidence: band=high; score=0.74
- Line 292: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks.push_back(chunk);
  Confidence: band=high; score=0.74
- Line 507: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: data.coordinates.emplace_back(
  Confidence: band=high; score=0.74
- Line 519: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: data.coordinates.emplace_back(
  Confidence: band=high; score=0.74
- Line 531: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: data.coordinates.emplace_back(point->getY(), point->getX());
  Confidence: band=high; score=0.74

### src/content/pipeline/async_bulk_uploader.cpp
Total findings: 6

- Line 97: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(result);
  Confidence: band=high; score=0.74
- Line 108: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(result);
  Confidence: band=high; score=0.74
- Line 117: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: files.push_back({metadata_list[i].content_id, blob});
  Confidence: band=high; score=0.74
- Line 138: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(result);
  Confidence: band=high; score=0.74
- Line 138: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(result);
  Confidence: band=high; score=0.74
- Line 156: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(result);
  Confidence: band=high; score=0.74

### src/content/content_logger.cpp
Total findings: 5

- Line 157: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: void ContentLogger::log(
  Confidence: band=medium; score=0.6
- Line 188: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: log(utils::Logger::Level::INFO, event, message, metadata);
  Confidence: band=medium; score=0.6
- Line 192: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: log(utils::Logger::Level::WARN, event, message, metadata);
  Confidence: band=medium; score=0.6
- Line 196: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: log(utils::Logger::Level::ERROR, event, message, metadata);
  Confidence: band=medium; score=0.6
- Line 200: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: log(utils::Logger::Level::DEBUG, event, message, metadata);
  Confidence: band=medium; score=0.6

### src/content/content_manager_llm.cpp
Total findings: 5

- Line 81: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: std::string analysis = THEMIS_LLM_GENERATE(prompt.str());
  Confidence: band=high; score=0.74
- Line 128: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: std::string tags_text = THEMIS_LLM_GENERATE(prompt.str());
  Confidence: band=high; score=0.74
- Line 172: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: std::string summary = THEMIS_LLM_GENERATE(prompt.str());
  Confidence: band=high; score=0.74
- Line 210: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: std::string category = THEMIS_LLM_GENERATE(prompt.str());
  Confidence: band=high; score=0.74
- Line 261: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: std::string entities_text = THEMIS_LLM_GENERATE(prompt.str());
  Confidence: band=high; score=0.74

### src/content/pdf_processor.cpp
Total findings: 5

- Line 386: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: items.push_back({static_cast<float>(r.x()), static_cast<float>(r.y()), static_cast<float>(r.width()),
  Confidence: band=high; score=0.74
- Line 408: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: positions_out.push_back({item.x, item.y});
  Confidence: band=high; score=0.74
- Line 477: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: sentences.push_back(sentence);
  Confidence: band=high; score=0.74
- Line 493: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks.push_back(chunk);
  Confidence: band=high; score=0.74
- Line 507: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: current_chunk += " ";
  Confidence: band=high; score=0.74

### src/content/pipeline/multimodal_chunker.cpp
Total findings: 5

- Line 97: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks.push_back(std::move(chunk));
  Confidence: band=high; score=0.74
- Line 179: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks.push_back(std::move(chunk));
  Confidence: band=high; score=0.74
- Line 179: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks.push_back(std::move(chunk));
  Confidence: band=high; score=0.74
- Line 223: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: boundaries.push_back(boundary);
  Confidence: band=high; score=0.74
- Line 249: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: boundaries.push_back(boundary);
  Confidence: band=high; score=0.74

### src/content/archive_processor.cpp
Total findings: 4

- Line 115: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: bool ArchiveProcessor::canHandle(const std::string &mime_type) const {
  Confidence: band=high; score=0.74
- Line 392: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: result += "/";
  Confidence: band=high; score=0.74
- Line 889: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: members.push_back(
  Confidence: band=high; score=0.74
- Line 915: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: extracted_files.push_back(file_path);
  Confidence: band=high; score=0.74

### src/content/audio_processor.cpp
Total findings: 4

- Line 184: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: waveform_json.push_back(sample);
  Confidence: band=high; score=0.74
- Line 214: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: segments_json.push_back(seg_json);
  Confidence: band=high; score=0.74
- Line 281: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks.push_back(chunk);
  Confidence: band=high; score=0.74
- Line 288: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: current_chunk += " ";
  Confidence: band=high; score=0.74

### src/content/cad_processor.cpp
Total findings: 4

- Line 381: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (minX != std::numeric_limits<double>::max()) {
  Confidence: band=very_high; score=0.9
- Line 92: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: bool CADProcessor::canProcess(const std::string &mime_type) const {
  Confidence: band=high; score=0.74
- Line 251: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks.push_back(chunk);
  Confidence: band=high; score=0.74
- Line 303: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: data.part_ids.push_back("PART_" + std::to_string(i + 1));
  Confidence: band=high; score=0.74

### src/content/image_processor.cpp
Total findings: 4

- Line 118: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: bool ImageProcessor::canProcess(const std::string& mime_type) const {
  Confidence: band=high; score=0.74
- Line 186: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: color_array.push_back({
  Confidence: band=high; score=0.74
- Line 268: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks.push_back(chunk);
  Confidence: band=high; score=0.74
- Line 273: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: current_chunk += " ";
  Confidence: band=high; score=0.74

### src/content/abuse_detector.cpp
Total findings: 3

- Line 187: severity=HIGH; category=performance; pattern=regex_in_loop
  Description: std::regex compiled in loop (compile once, reuse)
  Context: for (const auto& f : node["flags"]) {
  Confidence: band=very_high; score=0.9
- Line 199: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: patterns.push_back(std::move(p));
  Confidence: band=high; score=0.74
- Line 201: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: error += "Skipping pattern '" + name + "': " + re.what() + "; ";
  Confidence: band=high; score=0.74

### src/content/content_type.cpp
Total findings: 3

- Line 234: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(&type);
  Confidence: band=high; score=0.74
- Line 234: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(&type);
  Confidence: band=high; score=0.74
- Line 243: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(&type);
  Confidence: band=high; score=0.74

### src/content/html_processor.cpp
Total findings: 3

- Line 205: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: replaced += '\n';
  Confidence: band=high; score=0.74
- Line 515: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: paragraphs.push_back(para);
  Confidence: band=high; score=0.74
- Line 567: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: while (iss >> tok) tokens.push_back(tok);
  Confidence: band=high; score=0.74

### src/content/language_detector.cpp
Total findings: 3

- Line 206: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: while ((pos = lower.find(pattern, pos)) != std::string::npos) {
  Confidence: band=very_high; score=0.9
- Line 114: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: lower += ' '; // sentinel: ensure first word gets a leading space
  Confidence: band=high; score=0.74
- Line 118: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: lower += ' '; // sentinel: ensure last word gets a trailing space
  Confidence: band=high; score=0.74

### src/content/ocr_processor.cpp
Total findings: 3

- Line 372: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: sentences.push_back(current);
  Confidence: band=high; score=0.74
- Line 394: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks.push_back(std::move(ch));
  Confidence: band=high; score=0.74
- Line 401: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: current_chunk += " ";
  Confidence: band=high; score=0.74

### src/content/text_processor.cpp
Total findings: 3

- Line 152: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks.push_back(chunk);
  Confidence: band=high; score=0.74
- Line 331: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: sentences.push_back(sentence);
  Confidence: band=high; score=0.74
- Line 376: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: shingles.push_back(words[i] + " " + words[i + 1] + " " + words[i + 2]);
  Confidence: band=high; score=0.74

### src/content/content_metrics.cpp
Total findings: 2

- Line 118: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, double> ContentMetrics::getLatencyPercentiles(const std::string& operation) const {
  Confidence: band=high; score=0.74
- Line 120: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, double> result;
  Confidence: band=high; score=0.74

### src/content/content_security.cpp
Total findings: 2

- Line 404: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> seen_types;
  Confidence: band=medium; score=0.66
- Line 407: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.pii_types.push_back(type_str);
  Confidence: band=high; score=0.74

### src/content/markdown_processor.cpp
Total findings: 2

- Line 247: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: for (char c : tl) row += (c == '|') ? ' ' : c;
  Confidence: band=high; score=0.74
- Line 585: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: while (iss >> tok) tokens.push_back(tok);
  Confidence: band=high; score=0.74

### src/content/pipeline/bulk_upload_interface.cpp
Total findings: 2

- Line 69: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(result);
  Confidence: band=high; score=0.74
- Line 76: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(upload(contents[i], metadata_list[i]));
  Confidence: band=high; score=0.74

### src/content/adapters/format_extractor_factory.cpp
Total findings: 1

- Line 60: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(mime);
  Confidence: band=high; score=0.74

### src/content/content_validator.cpp
Total findings: 1

- Line 301: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: normalised.push_back(c == '\\' ? '/' : c);
  Confidence: band=high; score=0.74

### src/content/pipeline/content_chunker.cpp
Total findings: 1

- Line 58: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks.push_back(std::move(chunk));
  Confidence: band=high; score=0.74

### src/content/pipeline/zstd_compression.cpp
Total findings: 1

- Line 53: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // This maintains compatibility with standard decompress()
  Confidence: band=high; score=0.8

### src/content/tts_processor.cpp
Total findings: 1

- Line 212: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: voices.push_back(voice1);
  Confidence: band=high; score=0.74

### src/content/version_manager.cpp
Total findings: 1

- Line 48: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: lines.push_back(line);
  Confidence: band=high; score=0.74

### src/content/content_fs.cpp
Total findings: 0


### src/content/deduplication_checker.cpp
Total findings: 0


### src/content/embedding_pipeline.cpp
Total findings: 0


### src/content/mock_clip_processor.cpp
Total findings: 0

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
