# content Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: content
- Generated: 2026-06-02 12:40:50
- Status: Critical Findings Present
- Total Findings: 505
- Actionable Findings (Critical + High): 200
- Affected Files: 36

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 36 |
| High | 164 |
| Medium | 300 |
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
| src/content/content_manager.cpp | 86 | 11 | 11 | 64 | 0 |
| src/content/office_processor.cpp | 42 | 3 | 22 | 17 | 0 |
| src/content/video_processor.cpp | 42 | 7 | 16 | 19 | 0 |
| src/content/geo_processor.cpp | 29 | 1 | 17 | 11 | 0 |
| src/content/content_logger.cpp | 28 | 0 | 22 | 1 | 5 |
| src/content/mime_detector.cpp | 28 | 1 | 0 | 27 | 0 |
| src/content/stt_processor.cpp | 24 | 0 | 9 | 15 | 0 |
| src/content/async_ingestion_worker.cpp | 22 | 5 | 5 | 12 | 0 |
| src/content/html_processor.cpp | 21 | 0 | 8 | 13 | 0 |
| src/content/pdf_processor.cpp | 21 | 2 | 5 | 14 | 0 |
| src/content/content_manager_llm.cpp | 16 | 0 | 5 | 11 | 0 |
| src/content/archive_processor.cpp | 14 | 0 | 3 | 11 | 0 |
| src/content/audio_processor.cpp | 14 | 0 | 9 | 5 | 0 |
| src/content/cad_processor.cpp | 11 | 0 | 5 | 6 | 0 |
| src/content/tts_processor.cpp | 11 | 1 | 2 | 8 | 0 |
| src/content/content_fs.cpp | 9 | 2 | 2 | 5 | 0 |
| src/content/image_processor.cpp | 9 | 1 | 2 | 6 | 0 |
| src/content/markdown_processor.cpp | 9 | 0 | 3 | 6 | 0 |
| src/content/pipeline/async_bulk_uploader.cpp | 9 | 0 | 1 | 8 | 0 |
| src/content/pipeline/multimodal_chunker.cpp | 9 | 1 | 1 | 7 | 0 |
| src/content/content_type.cpp | 8 | 0 | 3 | 5 | 0 |
| src/content/content_security.cpp | 5 | 0 | 0 | 5 | 0 |
| src/content/text_processor.cpp | 5 | 0 | 2 | 3 | 0 |
| src/content/version_manager.cpp | 5 | 0 | 4 | 1 | 0 |
| src/content/content_metrics.cpp | 4 | 0 | 1 | 3 | 0 |
| src/content/language_detector.cpp | 4 | 0 | 2 | 2 | 0 |
| src/content/ocr_processor.cpp | 4 | 0 | 0 | 4 | 0 |
| src/content/abuse_detector.cpp | 3 | 0 | 1 | 2 | 0 |
| src/content/content_validator.cpp | 3 | 0 | 0 | 3 | 0 |
| src/content/pipeline/bulk_upload_interface.cpp | 3 | 0 | 0 | 3 | 0 |
| src/content/embedding_pipeline.cpp | 2 | 0 | 1 | 1 | 0 |
| src/content/adapters/format_extractor_factory.cpp | 1 | 0 | 0 | 1 | 0 |
| src/content/deduplication_checker.cpp | 1 | 1 | 0 | 0 | 0 |
| src/content/mock_clip_processor.cpp | 1 | 0 | 1 | 0 | 0 |
| src/content/pipeline/content_chunker.cpp | 1 | 0 | 0 | 1 | 0 |
| src/content/pipeline/zstd_compression.cpp | 1 | 0 | 1 | 0 | 0 |

## Full Scanner Findings

### src/content/content_manager.cpp
Total findings: 86

- Line 862: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto emb = embedding_pipeline_->generateEmbedding(c.text);
- Line 900: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (vector_index_->getDimension() == static_cast<int>(c.embedding.size())) {
- Line 1372: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto [st, results] = vector_index_->searchKnn(q, static_cast<size_t>(k), wptr);
- Line 2175: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: dup = dedup_checker_->isDuplicateImage(cached_phash);
- Line 2180: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: dup = dedup_checker_->isDuplicateText(cached_minhash);
- Line 2189: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: result.primary_content_id = dup->existing_id;
- Line 2636: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const bool stream_embedding_active = [&]() -> bool {
- Line 2706: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto emb = embedding_pipeline_->generateEmbedding(text);
- Line 2730: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (vector_index_->getDimension() == static_cast<int>(cm.embedding.size())) {
- Line 2865: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: s.total_storage_bytes = static_cast<int64_t>(storage_->getApproximateSize());
- Line 2871: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (vector_index_) s.total_embeddings = static_cast<int>(vector_index_->getVectorCount());
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 63: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(retry_delay_ms));
- Line 342: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: match = (vptr->dump() == cond.dump());
- Line 342: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: match = (vptr->dump() == cond.dump());
- Line 952: severity=HIGH; category=allocation_loop
  Description: Dynamic allocation in loop — high overhead
  Remediation: Pre-allocate outside loop or use object pool pattern
  Context: // the previous raw-new / manual-delete pattern (CWE-401 / RAII).
- Line 1576: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (beta != 0.0) {
  Confidence: band=very_high; score=0.9
- Line 1600: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: out.erase(std::remove_if(out.begin(), out.end(), [&](const auto& p){ return allowed.find(p.first) ==
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
- Line 194: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 202: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::string> fieldMap;
  Confidence: band=medium; score=0.66
- Line 209: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: for (auto it = sc["field_map"].begin(); it != sc["field_map"].end(); ++it) {
  Confidence: band=high; score=0.74
- Line 216: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 227: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 261: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 346: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 360: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (cid.is_string()) whitelist.push_back(std::string("chunks:") + cid.get<std::string>());
  Confidence: band=high; score=0.74
- Line 361: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (cid.is_string()) whitelist.push_back(std::string("chunks:") + cid.get<std::string>());
- Line 366: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 372: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 580: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 631: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: threat_info += r.threat_name + " (" + r.scanner_name + "); ";
  Confidence: band=high; score=0.74
- Line 663: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& mv : cj["skip_compressed_mimes"]) if (mv.is_string()) skip_mimes.push_back(mv.get<s
- Line 667: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 764: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 813: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto ftcfg = cj["fulltext_config"];
  Confidence: band=high; score=0.74
- Line 819: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (sw.is_string()) fulltext_config.stopwords.push_back(sw.get<std::string>());
  Confidence: band=high; score=0.74
- Line 820: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (sw.is_string()) fulltext_config.stopwords.push_back(sw.get<std::string>());
- Line 936: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& f : mcfg["fields"]) if (f.is_string()) meta_fields.push_back(f.get<std::string>());
- Line 1072: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& f : mcfg["fields"]) if (f.is_string()) meta_fields.push_back(f.get<std::string>());
- Line 1077: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1081: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto enc_section = j["_encrypted_meta"];
  Confidence: band=high; score=0.74
- Line 1107: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1144: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1209: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1217: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(ChunkMeta::fromJson(j));
  Confidence: band=high; score=0.74
- Line 1218: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(ChunkMeta::fromJson(j));
- Line 1221: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1239: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
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
- Line 1539: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
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
- Line 1603: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1668: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1701: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1725: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 2239: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks_json.push_back(cm.toJson());
  Confidence: band=high; score=0.74
- Line 2240: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: chunks_json.push_back(cm.toJson());
- Line 2318: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks_json.push_back(cm.toJson());
  Confidence: band=high; score=0.74
- Line 2319: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: chunks_json.push_back(cm.toJson());
- Line 2399: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks_json.push_back(cm.toJson());
  Confidence: band=high; score=0.74
- Line 2400: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: chunks_json.push_back(cm.toJson());
- Line 2672: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto ftcfg = cj["fulltext_config"];
  Confidence: band=high; score=0.74
- Line 2678: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (sw.is_string()) fulltext_config.stopwords.push_back(sw.get<std::string>());
  Confidence: band=high; score=0.74
- Line 2679: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (sw.is_string()) fulltext_config.stopwords.push_back(sw.get<std::string>());
- Line 2684: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/content/office_processor.cpp
Total findings: 42

- Line 844: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: ssize_t written = write(in_fd, bdata, remaining);
- Line 1004: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: int out_fd = open(out_path.c_str(), O_RDONLY);
- Line 1015: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: while ((n = read(out_fd, buf, sizeof(buf))) > 0) {
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #3780 fix(content/security): CON-... (2026-03-12) | #3738 feat(content): Libr
- Line 240: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["document_type"] = "docx";
- Line 241: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["mime_type"]     = DOCX_CONTENT_TYPE;
- Line 312: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["extraction_method"] = "basic_regex";
- Line 313: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["token_count"]       = countTokens(result.text);
- Line 349: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["title"]         = metadata.title;
- Line 350: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["author"]        = metadata.author;
- Line 351: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["created_date"]  = metadata.created_date;
- Line 429: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["token_count"] = countTokens(result.text);
- Line 436: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["note"]              = "Full XLSX extraction requires building with -DTHEMIS_ENABLE_
- Line 437: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["extraction_method"] = "not_available";
- Line 465: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (entry.find("ppt/slides/slide") != std::string::npos && entry.find(".xml") != std::string::npos)
- Line 526: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["token_count"] = countTokens(result.text);
- Line 533: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["note"]              = "Full PPTX extraction requires building with -DTHEMIS_ENABLE_
- Line 534: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["extraction_method"] = "not_available";
- Line 562: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["document_type"] = type_str;
- Line 589: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: } else if (std::string(child.name()).find("text:") == 0) {
  Confidence: band=very_high; score=0.9
- Line 606: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["note"] = "Full ODF extraction requires building with -DTHEMIS_ENABLE_OFFICE=ON";
- Line 777: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["document_type"]     = type_str;
- Line 778: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["extraction_method"] = "libreoffice_headless";
- Line 815: severity=HIGH; category=posix_only_api
  Description: POSIX-only API unlink( without platform guard
  Remediation: Wrap in #ifndef _WIN32 ... #endif or provide Windows alternative
  Context: unlink(in_file.c_str());
- Line 817: severity=HIGH; category=posix_only_api
  Description: POSIX-only API unlink( without platform guard
  Remediation: Wrap in #ifndef _WIN32 ... #endif or provide Windows alternative
  Context: unlink(out_file.c_str());
- Line 0: severity=MEDIUM; category=uncategorized
  Context: ['            const size_t combined = token_hash ^ (i * 31u) ^ (static_cast<size_t>(seed) * 97u);', '            for (int d = 0; d < 10; ++d) {', '                const int dim = static_cast<int>((combined + static_cast<size_t>(d) * 73u) % static_cast<size_t>(kDim));', '                const float weight = 1.0f / (1.0f + static_cast<float>(i) * 0.1f);', '                const float phase']
  Confidence: band=medium; score=0.62
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
- Line 641: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: zip_close(archive);
- Line 726: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entries.push_back(name);
  Confidence: band=high; score=0.74
- Line 731: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: zip_close(archive);
- Line 848: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close(in_fd);
- Line 855: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close(in_fd);
- Line 1019: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close(out_fd);
- Line 1023: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close(out_fd);
- Line 1070: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks.push_back(chunk);
  Confidence: band=high; score=0.74
- Line 1077: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: current_chunk += "\n";
  Confidence: band=high; score=0.74

### src/content/video_processor.cpp
Total findings: 42

- Line 539: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: data.framerate = static_cast<double>(stream->avg_frame_rate.num) / stream->avg_frame_rate.den;
- Line 539: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: data.framerate = static_cast<double>(stream->avg_frame_rate.num) / stream->avg_frame_rate.den;
- Line 703: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: double aspect = static_cast<double>(frame->width) / frame->height;
- Line 716: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: = sws_getContext(frame->width, frame->height, static_cast<AVPixelFormat>(frame->format), thumb_width
- Line 746: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (rgb_frame->linesize[0] == static_cast<int>(row_size)) {
- Line 945: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const uint8_t *ra = a->data[0] + static_cast<ptrdiff_t>(y) * a->linesize[0];
- Line 946: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const uint8_t *rb = b->data[0] + static_cast<ptrdiff_t>(y) * b->linesize[0];
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #3120 [content] Implement video f... (2026-03-12) | #2996 feat(content): Vide
- Line 507: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (fmt_ctx->iformat) {
- Line 508: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: data.container_format = fmt_ctx->iformat->name;
- Line 512: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (fmt_ctx->duration != AV_NOPTS_VALUE) {
- Line 513: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: data.duration_ms = fmt_ctx->duration / 1000;
- Line 517: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (fmt_ctx->bit_rate > 0) {
- Line 518: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: data.bitrate_kbps = fmt_ctx->bit_rate / 1000;
- Line 522: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: for (unsigned int i = 0; i < fmt_ctx->nb_streams; i++) {
- Line 523: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: AVStream *stream            = fmt_ctx->streams[i];
- Line 629: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: for (unsigned int i = 0; i < fmt_ctx->nb_streams; i++) {
- Line 630: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
- Line 642: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: AVStream *video_stream      = fmt_ctx->streams[video_stream_index];
- Line 676: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: int64_t seek_target = fmt_ctx->duration / 10;
- Line 740: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: const uint8_t* src = rgb_frame->data[0];
- Line 828: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: AVRational time_base = fmt_ctx->streams[video_stream_index]->time_base;
- Line 902: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: AVStream *video_stream      = fmt_ctx->streams[video_stream_index];
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
- Line 306: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: current_text += " ";
- Line 451: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: keyframes.push_back(i * interval_ms);
  Confidence: band=high; score=0.74
- Line 452: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: keyframes.push_back(i * interval_ms);
- Line 566: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 759: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: av_frame_free(&rgb_frame);
- Line 765: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: av_frame_free(&frame);
- Line 766: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: av_packet_free(&packet);
- Line 771: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 847: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
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
- Line 988: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: av_frame_free(&frame);
- Line 989: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: av_frame_free(&prev_frame);
- Line 990: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: av_packet_free(&packet);
- Line 994: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/content/geo_processor.cpp
Total findings: 29

- Line 531: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: OGRPoint* point = static_cast<OGRPoint*>(multipoint->getGeometryRef(g));
- Line 0: severity=HIGH; category=uncategorized
  Context: Array declared without initialization
  Confidence: band=high; score=0.81
- Line 203: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["crs"] = geo.crs;
- Line 567: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: data.properties["layer_name"] = layer->GetName();
- Line 734: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: data.properties["height"] = std::to_string(height);
- Line 735: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: data.properties["bands"] = std::to_string(band_count);
- Line 736: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: data.properties["size_pixels"] = std::to_string(width * height);
- Line 754: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: data.bounds[0] = minX;
- Line 755: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: data.bounds[1] = minY;
- Line 756: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: data.bounds[2] = maxX;
- Line 757: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: data.bounds[3] = maxY;
- Line 759: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: data.properties["bounds_minX"] = std::to_string(minX);
- Line 760: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: data.properties["bounds_minY"] = std::to_string(minY);
- Line 761: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: data.properties["bounds_maxX"] = std::to_string(maxX);
- Line 762: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: data.properties["bounds_maxY"] = std::to_string(maxY);
- Line 766: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: const char* projection = dataset->GetProjectionRef();
- Line 801: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: data.properties[band_prefix + "_color_interpretation"] =
- Line 806: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: double nodata = band->GetNoDataValue(&has_nodata);
- Line 135: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: bool GeoProcessor::canProcess(const std::string& mime_type) const {
  Confidence: band=high; score=0.74
- Line 181: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: (mime_type == "application/json" && content.find("\"type\"") != std::string::npos)) {
- Line 181: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: (mime_type == "application/json" && content.find("\"type\"") != std::string::npos)) {
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
- Line 585: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 679: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 830: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/content/content_logger.cpp
Total findings: 28

- Line 41: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["content_id"] = content_id;
- Line 42: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["mime_type"] = mime_type;
- Line 43: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["size_bytes"] = size_bytes;
- Line 61: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["content_id"] = content_id;
- Line 62: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["mime_type"] = mime_type;
- Line 63: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["size_bytes"] = size_bytes;
- Line 64: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["success"] = success;
- Line 65: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["duration_ms"] = duration_ms;
- Line 86: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["content_id"] = content_id;
- Line 87: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["operation"] = operation;
- Line 88: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["duration_ms"] = duration_ms;
- Line 89: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["success"] = success;
- Line 113: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["content_id"] = content_id;
- Line 114: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["operation"] = operation;
- Line 115: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["error_code"] = error_code;
- Line 116: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["error_message"] = pii_sanitization_ ? sanitizeMessage(error_message) : error_message;
- Line 132: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["content_id"] = content_id;
- Line 133: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["operation"] = operation;
- Line 134: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["timeout_seconds"] = timeout_seconds;
- Line 135: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["elapsed_seconds"] = elapsed_seconds;
- Line 145: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["content_id"] = content_id;
- Line 146: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["hit"] = hit;
- Line 214: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: size_t pos = filename.find_last_of("/\\");
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

### src/content/mime_detector.cpp
Total findings: 28

- Line 74: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: bool verified = sig_mgr_->verifyFile(config_path, resource_id);
- Line 114: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: sig.signature.push_back(static_cast<uint8_t>(byte.as<int>()));
  Confidence: band=high; score=0.74
- Line 114: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: sig.signature.push_back(static_cast<uint8_t>(byte.as<int>()));
  Confidence: band=high; score=0.74
- Line 115: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: sig.signature.push_back(static_cast<uint8_t>(byte.as<int>()));
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
- Line 219: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 310: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: buffer += "[extensions]\n";
  Confidence: band=high; score=0.74
- Line 310: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: buffer += "[extensions]\n";
  Confidence: band=high; score=0.74
- Line 322: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: snprintf(tmp, sizeof(tmp), "%02x", static_cast<unsigned int>(b));
- Line 322: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: snprintf(tmp, sizeof(tmp), "%02x", static_cast<unsigned int>(b));
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
- Line 327: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: wildcards += ":";
- Line 329: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!first) wildcards += ",";
  Confidence: band=high; score=0.74
- Line 330: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (!first) wildcards += ",";
- Line 330: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (!first) wildcards += ",";
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
- Line 349: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (i) joined += ",";
- Line 349: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (i) joined += ",";
- Line 351: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: category_lines.push_back(cat.first + "=" + joined);
  Confidence: band=high; score=0.74
- Line 351: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: category_lines.push_back(cat.first + "=" + joined);
  Confidence: band=high; score=0.74
- Line 363: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: snprintf(&hex_out[i * 2], 3, "%02x", static_cast<unsigned int>(digest[i]));

### src/content/stt_processor.cpp
Total findings: 24

- Line 0: severity=HIGH; category=uncategorized
  Context: ['            size_t bs  = static_cast<size_t>(b) * band_size;', '            size_t be  = (b == kBands - 1) ? n : bs + band_size;', '            size_t len = be - bs;', '', '            float rms = 0.0f;']
  Confidence: band=high; score=0.81
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #3619 fix(content): build system ... (2026-03-12) | #3574 fix: clear all rema
- Line 166: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["transcription"] = {{"language", transcription.detected_language},
- Line 576: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: + " (only PCM [1] and IEEE float [3] are supported)");
- Line 990: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // Assumes both inputs are L2-normalised; result is in [-1, 1].
  Confidence: band=very_high; score=0.9
- Line 1152: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: char buffer[32];
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
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
- Line 345: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 457: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: whisper_free(static_cast<struct whisper_context *>(whisper_ctx_));
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
Total findings: 22

- Line 0: severity=CRITICAL; category=uncategorized
  Confidence: band=very_high; score=0.85
- Line 264: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: backpressure_cv_.wait(lock, [this] {
- Line 325: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: backpressure_cv_.wait(lock, [this] {
- Line 591: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: queue_cv_.wait(lock, [this] { return !job_queue_.empty() || shutdown_requested_.load(); });
- Line 913: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: job.config.merge_patch(additional_config);
  Confidence: band=very_high; score=0.99
- Line 139: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: cleanup_thread_ = std::thread(&AsyncIngestionWorker::cleanupLoop, this);
  Confidence: band=very_high; score=0.9
- Line 154: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(queue_mutex_);
- Line 608: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(history_mutex_);
- Line 913: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: job.config.merge_patch(additional_config);
  Confidence: band=very_high; score=0.9
- Line 1065: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::minutes(5));
- Line 115: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
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
- Line 414: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: file_list.push_back({{"filename", filename}, {"size", blob.size()}});
- Line 420: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: job.config["_blobs"].push_back({{"filename", filename}, {"blob", blob}});
  Confidence: band=high; score=0.74
- Line 421: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: job.config["_blobs"].push_back({{"filename", filename}, {"blob", blob}});
- Line 483: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(job);
  Confidence: band=high; score=0.74
- Line 805: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: job.content_ids.push_back(result.primary_content_id);
  Confidence: band=high; score=0.74
- Line 806: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: job.content_ids.push_back(result.primary_content_id);
- Line 860: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: names.push_back(name);
  Confidence: band=high; score=0.74

### src/content/html_processor.cpp
Total findings: 21

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #3619 fix(content): build system ... (2026-03-12) | #3012 [content] HTML cont
- Line 0: severity=MEDIUM; category=uncategorized
  Context: ['            size_t combined = token_hash ^ (i * 31) ^ (static_cast<size_t>(seed) * 97);', '            for (int d = 0; d < 10; ++d) {', '                int dim = static_cast<int>((combined + static_cast<size_t>(d) * 73) % DIM);', '                float weight = 1.0f / (1.0f + static_cast<float>(i) * 0.1f);', '                float phase  = static_cast<float>((combined + static_cast<size_t>(dim)) % 360)']
  Confidence: band=medium; score=0.62
- Line 193: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: static const std::regex heading_close(R"(<\s*/\s*h[1-6][^>]*>)",
- Line 193: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: static const std::regex heading_close(R"(<\s*/\s*h[1-6][^>]*>)",
- Line 205: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: replaced += '\n';
  Confidence: band=high; score=0.74
- Line 206: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: replaced += '\n';
- Line 208: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: replaced += ' ';
- Line 219: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: R"(<\s*/?\s*(p|div|article|section|main|h[1-6]|li|ul|ol|blockquote|pre|br|tr|td|th|dt|dd)[^>]*>)",
- Line 324: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: R"(<title[^>]*>([\s\S]*?)<\/title>)",
- Line 407: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: result += '\n';
- Line 414: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: result += ' ';
- Line 515: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: paragraphs.push_back(para);
  Confidence: band=high; score=0.74
- Line 567: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: while (iss >> tok) tokens.push_back(tok);
  Confidence: band=high; score=0.74
- Line 572: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (!overlap_text.empty()) overlap_text += ' ';

### src/content/pdf_processor.cpp
Total findings: 21

- Line 162: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: int max_pages = config_.max_pages > 0 ? std::min(config_.max_pages, doc->pages()) : doc->pages();
- Line 329: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: int max_pages = config_.max_pages > 0 ? std::min(config_.max_pages, doc->pages()) : doc->pages();
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 236: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["extraction_method"] = "basic_regex";
- Line 237: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["note"]              = "Full PDF extraction requires building with -DTHEMIS_ENABLE_P
- Line 238: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["layout_preserved"]  = false;
- Line 239: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["token_count"]       = countTokens(result.text);
- Line 0: severity=MEDIUM; category=uncategorized
  Context: ['            page_obj["page"]     = i + 1;', '            page_obj["text"]     = page_text;', '            page_obj["width"]    = static_cast<int>(rect.width());', '            page_obj["height"]   = static_cast<int>(rect.height());', '            page_obj["rotation"] = page->orientation() * 90;']
  Confidence: band=medium; score=0.62
- Line 0: severity=MEDIUM; category=uncategorized
  Context: ['            page_obj["text"]     = page_text;', '            page_obj["width"]    = static_cast<int>(rect.width());', '            page_obj["height"]   = static_cast<int>(rect.height());', '            page_obj["rotation"] = page->orientation() * 90;', '            pages_array.push_back(std::move(page_obj));']
  Confidence: band=medium; score=0.62
- Line 0: severity=MEDIUM; category=uncategorized
  Context: ['        // Get dimensions', '        poppler::rectf rect = page->page_rect();', '        info.width          = static_cast<int>(rect.width());', '        info.height         = static_cast<int>(rect.height());', '        info.rotation       = page->orientation() * 90;']
  Confidence: band=medium; score=0.62
- Line 0: severity=MEDIUM; category=uncategorized
  Context: ['        poppler::rectf rect = page->page_rect();', '        info.width          = static_cast<int>(rect.width());', '        info.height         = static_cast<int>(rect.height());', '        info.rotation       = page->orientation() * 90;', '']
  Confidence: band=medium; score=0.62
- Line 0: severity=MEDIUM; category=uncategorized
  Context: ['            const size_t combined = token_hash ^ (i * 31u) ^ (static_cast<size_t>(seed) * 97u);', '            for (int d = 0; d < 10; ++d) {', '                const int dim = static_cast<int>((combined + static_cast<size_t>(d) * 73u) % static_cast<size_t>(kDim));', '                const float weight = 1.0f / (1.0f + static_cast<float>(i) * 0.1f);', '                const float phase']
  Confidence: band=medium; score=0.62
- Line 0: severity=MEDIUM; category=uncategorized
  Context: Struct with uninitialized fields
  Confidence: band=medium; score=0.65
- Line 219: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: std::regex page_regex("/Type\\s*/Page[^s]");
- Line 300: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: std::regex pattern("/" + key + "\\s*\\(([^)]+)\\)");
- Line 300: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: std::regex pattern("/" + key + "\\s*\\(([^)]+)\\)");
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

### src/content/content_manager_llm.cpp
Total findings: 16

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 383: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result["people"]        = json::array();
- Line 384: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result["places"]        = json::array();
- Line 385: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result["organizations"] = json::array();
- Line 75: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: prompt << "3. Sentiment (positive/negative/neutral)\n";
- Line 76: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: prompt << "4. Content category (article/technical/business/personal/other)\n\n";
- Line 81: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: std::string analysis = THEMIS_LLM_GENERATE(prompt.str());
  Confidence: band=high; score=0.74
- Line 128: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: std::string tags_text = THEMIS_LLM_GENERATE(prompt.str());
  Confidence: band=high; score=0.74
- Line 140: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 172: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: std::string summary = THEMIS_LLM_GENERATE(prompt.str());
  Confidence: band=high; score=0.74
- Line 178: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 210: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: std::string category = THEMIS_LLM_GENERATE(prompt.str());
  Confidence: band=high; score=0.74
- Line 221: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 261: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: std::string entities_text = THEMIS_LLM_GENERATE(prompt.str());
  Confidence: band=high; score=0.74
- Line 268: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/content/archive_processor.cpp
Total findings: 14

- Line 0: severity=HIGH; category=uncategorized
  Context: ['            // Determine entry type from typeflag (byte 156):', "            //   '0'/NUL = regular file, '5' = directory, '2' = symlink, etc.", '            const char typeflag = block[kTypeOffset];', "            const bool is_dir   = (typeflag == '5');", '']
  Confidence: band=high; score=0.78
- Line 366: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: return metadata.has_value() && metadata->is_encrypted;
- Line 893: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["members"] = members;
- Line 102: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 115: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: bool ArchiveProcessor::canHandle(const std::string &mime_type) const {
  Confidence: band=high; score=0.74
- Line 252: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: zip_close(za);
- Line 392: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: result += "/";
  Confidence: band=high; score=0.74
- Line 393: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: result += "/";
- Line 610: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: zip_close(za);
- Line 751: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 758: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 889: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: members.push_back(
  Confidence: band=high; score=0.74
- Line 890: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: members.push_back(
- Line 915: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: extracted_files.push_back(file_path);
  Confidence: band=high; score=0.74

### src/content/audio_processor.cpp
Total findings: 14

- Line 0: severity=HIGH; category=uncategorized
  Context: ['        if (bitrate > 0) {', '            // Approximate: subtract ID3 header size from content length', '            size_t audio_bytes = (blob.size() > search_start) ? blob.size() - search_start : blob.size();', '            data.duration_ms   = static_cast<int64_t>(audio_bytes) * 8 * 1000 / (bitrate * 1000);', '        }']
  Confidence: band=high; score=0.78
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #3619 fix(content): build system ... (2026-03-12) | #3109 feat(content): Audi
- Line 146: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["duration_ms"]      = media.duration_ms;
- Line 147: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["audio_codec"]      = media.audio_codec;
- Line 148: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["bitrate_kbps"]     = media.bitrate_kbps;
- Line 149: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["sample_rate"]      = media.sample_rate;
- Line 150: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["channels"]         = media.channels;
- Line 151: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["container_format"] = media.container_format;
- Line 238: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["transcription"] = trans_meta;
- Line 184: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: waveform_json.push_back(sample);
  Confidence: band=high; score=0.74
- Line 214: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: segments_json.push_back(seg_json);
  Confidence: band=high; score=0.74
- Line 222: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 281: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks.push_back(chunk);
  Confidence: band=high; score=0.74
- Line 288: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: current_chunk += " ";
  Confidence: band=high; score=0.74

### src/content/cad_processor.cpp
Total findings: 11

- Line 146: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["units"]      = default_units_;
- Line 149: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["bounding_box"]
- Line 157: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["dimensions"] = {dx, dy, dz};
- Line 250: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: chunk.metadata["part_id"] = cad.part_ids[i];
- Line 381: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (minX != std::numeric_limits<double>::max()) {
  Confidence: band=very_high; score=0.9
- Line 92: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: bool CADProcessor::canProcess(const std::string &mime_type) const {
  Confidence: band=high; score=0.74
- Line 134: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: } else if (mime_type == "image/vnd.dxf" || header.find("0\nSECTION") != std::string::npos) {
- Line 134: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: } else if (mime_type == "image/vnd.dxf" || header.find("0\nSECTION") != std::string::npos) {
- Line 251: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks.push_back(chunk);
  Confidence: band=high; score=0.74
- Line 303: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: data.part_ids.push_back("PART_" + std::to_string(i + 1));
  Confidence: band=high; score=0.74
- Line 304: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: data.part_ids.push_back("PART_" + std::to_string(i + 1));

### src/content/tts_processor.cpp
Total findings: 11

- Line 252: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: auto *voice = new piper::PiperVoice();
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4833 Continue Phase-6 tensorgrap... (2026-05-07) | #3619 fix(content): build
- Line 275: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: static_cast = nullptr;
  Context: delete static_cast<piper::PiperVoice *>(tts_ctx_);
- Line 212: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: voices.push_back(voice1);
  Confidence: band=high; score=0.74
- Line 437: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: wav_data.push_back(byte_rate & 0xFF);
- Line 438: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: wav_data.push_back((byte_rate >> 8) & 0xFF);
- Line 439: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: wav_data.push_back((byte_rate >> 16) & 0xFF);
- Line 440: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: wav_data.push_back((byte_rate >> 24) & 0xFF);
- Line 447: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: wav_data.push_back(data_size & 0xFF);
- Line 448: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: wav_data.push_back((data_size >> 8) & 0xFF);
- Line 449: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: wav_data.push_back((data_size >> 16) & 0xFF);

### src/content/content_fs.cpp
Total findings: 9

- Line 232: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: uint64_t part_end   = (i == end_idx) ? (end - chunk_off) : static_cast<uint64_t>(part->size());
- Line 234: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: part_end = static_cast<uint64_t>(part->size());
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 37: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: oss << std::setw(2) << static_cast<int>(data[i]);
- Line 114: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 172: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 245: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 271: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 285: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/content/image_processor.cpp
Total findings: 9

- Line 546: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 64 > array 31
  Remediation: Fix loop condition or increase array size
  Context: double median = (sorted_freq[31] + sorted_freq[32]) / 2.0;
- Line 0: severity=HIGH; category=uncategorized
  Context: ['        for (size_t i = 2; i < blob.size() - 9; ++i) {', '            if (blob[i] == 0xFF && (blob[i + 1] == 0xC0 || blob[i + 1] == 0xC2)) {', '                height = (blob[i + 5] << 8) | blob[i + 6];', '                width = (blob[i + 7] << 8) | blob[i + 8];', '                return;']
  Confidence: band=high; score=0.78
- Line 0: severity=HIGH; category=uncategorized
  Context: ['            if (blob[i] == 0xFF && (blob[i + 1] == 0xC0 || blob[i + 1] == 0xC2)) {', '                height = (blob[i + 5] << 8) | blob[i + 6];', '                width = (blob[i + 7] << 8) | blob[i + 8];', '                return;', '            }']
  Confidence: band=high; score=0.78
- Line 118: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: bool ImageProcessor::canProcess(const std::string& mime_type) const {
  Confidence: band=high; score=0.74
- Line 186: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: color_array.push_back({
  Confidence: band=high; score=0.74
- Line 187: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: color_array.push_back({
- Line 268: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks.push_back(chunk);
  Confidence: band=high; score=0.74
- Line 273: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: current_chunk += " ";
  Confidence: band=high; score=0.74
- Line 274: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: current_chunk += " ";

### src/content/markdown_processor.cpp
Total findings: 9

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=MEDIUM; category=uncategorized
  Context: ['            size_t combined = token_hash ^ (i * 31) ^ (static_cast<size_t>(seed) * 97);', '            for (int d = 0; d < 10; ++d) {', '                int dim = static_cast<int>((combined + static_cast<size_t>(d) * 73) % DIM);', '                float weight = 1.0f / (1.0f + static_cast<float>(i) * 0.1f);', '                float phase  = static_cast<float>((combined + static_cast<size_t>(dim)) % 360)']
  Confidence: band=medium; score=0.62
- Line 247: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: for (char c : tl) row += (c == '|') ? ' ' : c;
  Confidence: band=high; score=0.74
- Line 441: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (consecutive_nl <= 2) result += '\n';
- Line 448: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (!last_was_space && !last_was_newline) result += ' ';
- Line 585: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: while (iss >> tok) tokens.push_back(tok);
  Confidence: band=high; score=0.74
- Line 590: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (!overlap_text.empty()) overlap_text += ' ';

### src/content/pipeline/async_bulk_uploader.cpp
Total findings: 9

- Line 64: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: content_to_job_map_[metadata.content_id] = job_id;
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
- Line 118: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: files.push_back({metadata_list[i].content_id, blob});
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
- Line 226: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: using themis::content::IngestionJobStatus;

### src/content/pipeline/multimodal_chunker.cpp
Total findings: 9

- Line 0: severity=CRITICAL; category=uncategorized
  Context: ['    }', '', '    const size_t expected_size = width * height * bytes_per_pixel;', '    if (data.size() != expected_size) {', '        // Size mismatch - fall back to generic chunking']
  Confidence: band=very_high; score=0.93
- Line 100: severity=HIGH; category=allocation_loop
  Description: Dynamic allocation in loop — high overhead
  Remediation: Pre-allocate outside loop or use object pool pattern
  Context: // Start new chunk with overlap
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
- Line 230: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: boundaries.push_back(text.size());  // End
- Line 249: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: boundaries.push_back(boundary);
  Confidence: band=high; score=0.74
- Line 270: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: boundaries.push_back(text.size());  // End

### src/content/content_type.cpp
Total findings: 8

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #3012 [content] HTML content extr... (2026-03-12)
- Line 90: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: type.supports_metadata_extraction = j["supports_metadata_extraction"];
- Line 218: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (size_t i = 0; i < std::min(blob.size(), size_t(1000)); i++) {
- Line 234: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(&type);
  Confidence: band=high; score=0.74
- Line 234: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(&type);
  Confidence: band=high; score=0.74
- Line 235: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(&type);
- Line 243: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(&type);
  Confidence: band=high; score=0.74
- Line 244: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(&type);

### src/content/content_security.cpp
Total findings: 5

- Line 404: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> seen_types;
  Confidence: band=medium; score=0.66
- Line 407: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.pii_types.push_back(type_str);
  Confidence: band=high; score=0.74
- Line 529: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: static const std::regex unix_path_regex(R"((/(?:[a-zA-Z_][a-zA-Z0-9_\-]*)(?:/[a-zA-Z0-9_\-.]+)+))");
- Line 530: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: static const std::regex windows_path_regex(R"([A-Z]:\\([a-zA-Z0-9_\-]+\\)+[a-zA-Z0-9_\-./]*)");
- Line 531: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: static const std::regex home_path_regex(R"(~/[a-zA-Z0-9_\-./]+)");

### src/content/text_processor.cpp
Total findings: 5

- Line 0: severity=HIGH; category=uncategorized
  Context: ['            // Distribute token influence across dimensions', '            for (int dim_offset = 0; dim_offset < 10; dim_offset++) {', '                int dim = (combined_hash + dim_offset * 73) % EMBEDDING_DIM;', '', '                // Add influence with varying weights based on position']
  Confidence: band=high; score=0.78
- Line 63: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["token_count"] = token_count;
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

### src/content/version_manager.cpp
Total findings: 5

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 48: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: lines.push_back(line);
  Confidence: band=high; score=0.74

### src/content/content_metrics.cpp
Total findings: 4

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #3556 docs(content): reality-chec... (2026-03-12) | #2592 [content] PDF text
- Line 118: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, double> ContentMetrics::getLatencyPercentiles(const std::string& operation) const {
  Confidence: band=high; score=0.74
- Line 120: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, double> result;
  Confidence: band=high; score=0.74
- Line 386: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: oss << "# HELP content_extract_errors_total Total PDF/document extraction errors\n";

### src/content/language_detector.cpp
Total findings: 4

- Line 202: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: for (const char *const *ind = kProfiles[p].indicators; *ind != nullptr; ++ind) {
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
Total findings: 4

- Line 0: severity=MEDIUM; category=uncategorized
  Context: ['            size_t combined = token_hash ^ (i * 31) ^ (static_cast<size_t>(seed) * 97);', '            for (int d = 0; d < 10; ++d) {', '                int dim      = static_cast<int>((combined + static_cast<size_t>(d) * 73) % DIM);', '                float weight = 1.0f / (1.0f + static_cast<float>(i) * 0.1f);', '                float phase = static_cast<float>((combined + static_cast<size_t>(dim)) % 360) * 3.14159265359f / 180.0f;']
  Confidence: band=medium; score=0.62
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

### src/content/content_validator.cpp
Total findings: 3

- Line 301: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: normalised.push_back(c == '\\' ? '/' : c);
  Confidence: band=high; score=0.74
- Line 302: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: normalised.push_back(c == '\\' ? '/' : c);
- Line 302: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: normalised.push_back(c == '\\' ? '/' : c);

### src/content/pipeline/bulk_upload_interface.cpp
Total findings: 3

- Line 69: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(result);
  Confidence: band=high; score=0.74
- Line 76: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(upload(contents[i], metadata_list[i]));
  Confidence: band=high; score=0.74
- Line 77: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(upload(contents[i], metadata_list[i]));

### src/content/embedding_pipeline.cpp
Total findings: 2

- Line 83: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: auto status = future.wait_for(std::chrono::milliseconds(config_.timeout_ms));
- Line 106: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/content/adapters/format_extractor_factory.cpp
Total findings: 1

- Line 60: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(mime);
  Confidence: band=high; score=0.74

### src/content/deduplication_checker.cpp
Total findings: 1

- Line 160: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto val = band_cache_->get(makeBandKey(b, bh));

### src/content/mock_clip_processor.cpp
Total findings: 1

- Line 24: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: res.metadata["mime_type"]           = content_type.mime_type;

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

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
