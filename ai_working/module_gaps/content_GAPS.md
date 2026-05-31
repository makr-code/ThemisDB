# content Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: content
- Generated: 2026-05-31 08:50:11
- Status: Critical Findings Present
- Total Findings: 859
- Actionable Findings (Critical + High): 432
- Affected Files: 36

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 47 |
| High | 385 |
| Medium | 427 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| memory | 222 |
| container | 150 |
| performance_patterns | 143 |
| reliability | 110 |
| raii | 35 |
| concurrency | 33 |
| security | 29 |
| exception_safety | 28 |
| performance | 21 |
| platform | 21 |
| determinism | 15 |
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
| src/content/content_manager.cpp | 114 | 12 | 30 | 72 | 0 |
| src/content/office_processor.cpp | 82 | 3 | 48 | 31 | 0 |
| src/content/geo_processor.cpp | 70 | 1 | 57 | 12 | 0 |
| src/content/stt_processor.cpp | 68 | 0 | 44 | 24 | 0 |
| src/content/video_processor.cpp | 58 | 7 | 22 | 29 | 0 |
| src/content/async_ingestion_worker.cpp | 52 | 9 | 29 | 14 | 0 |
| src/content/pdf_processor.cpp | 51 | 2 | 30 | 19 | 0 |
| src/content/mime_detector.cpp | 40 | 1 | 6 | 33 | 0 |
| src/content/content_logger.cpp | 28 | 0 | 22 | 1 | 5 |
| src/content/html_processor.cpp | 27 | 2 | 7 | 18 | 0 |
| src/content/archive_processor.cpp | 26 | 0 | 7 | 19 | 0 |
| src/content/audio_processor.cpp | 21 | 0 | 13 | 8 | 0 |
| src/content/text_processor.cpp | 21 | 2 | 9 | 10 | 0 |
| src/content/content_manager_llm.cpp | 20 | 0 | 8 | 12 | 0 |
| src/content/tts_processor.cpp | 20 | 1 | 1 | 18 | 0 |
| src/content/pipeline/multimodal_chunker.cpp | 17 | 1 | 1 | 15 | 0 |
| src/content/image_processor.cpp | 16 | 1 | 8 | 7 | 0 |
| src/content/pipeline/async_bulk_uploader.cpp | 16 | 0 | 4 | 12 | 0 |
| src/content/markdown_processor.cpp | 15 | 0 | 3 | 12 | 0 |
| src/content/content_fs.cpp | 14 | 2 | 7 | 5 | 0 |
| src/content/cad_processor.cpp | 13 | 0 | 6 | 7 | 0 |
| src/content/content_metrics.cpp | 10 | 0 | 7 | 3 | 0 |
| src/content/ocr_processor.cpp | 10 | 0 | 1 | 9 | 0 |
| src/content/version_manager.cpp | 8 | 1 | 4 | 3 | 0 |
| src/content/content_security.cpp | 7 | 0 | 1 | 6 | 0 |
| src/content/content_type.cpp | 7 | 0 | 2 | 5 | 0 |
| src/content/abuse_detector.cpp | 4 | 0 | 1 | 3 | 0 |
| src/content/adapters/format_extractor_factory.cpp | 4 | 1 | 1 | 2 | 0 |
| src/content/content_validator.cpp | 4 | 0 | 0 | 4 | 0 |
| src/content/language_detector.cpp | 4 | 0 | 2 | 2 | 0 |
| src/content/pipeline/bulk_upload_interface.cpp | 4 | 0 | 0 | 4 | 0 |
| src/content/deduplication_checker.cpp | 2 | 1 | 1 | 0 | 0 |
| src/content/embedding_pipeline.cpp | 2 | 0 | 1 | 1 | 0 |
| src/content/pipeline/content_chunker.cpp | 2 | 0 | 0 | 2 | 0 |
| src/content/mock_clip_processor.cpp | 1 | 0 | 1 | 0 | 0 |
| src/content/pipeline/zstd_compression.cpp | 1 | 0 | 1 | 0 | 0 |

## Full Scanner Findings

### src/content/content_manager.cpp
Total findings: 114

- Line 878: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto emb = embedding_pipeline_->generateEmbedding(c.text);
- Line 916: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (vector_index_->getDimension() == static_cast<int>(c.embedding.size())) {
- Line 1388: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto [st, results] = vector_index_->searchKnn(q, static_cast<size_t>(k), wptr);
- Line 2065: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator filename_pos may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto filename_pos = relative_path.find_last_of('/');
- Line 2191: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: dup = dedup_checker_->isDuplicateImage(cached_phash);
- Line 2196: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: dup = dedup_checker_->isDuplicateText(cached_minhash);
- Line 2205: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: result.primary_content_id = dup->existing_id;
- Line 2652: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const bool stream_embedding_active = [&]() -> bool {
- Line 2722: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto emb = embedding_pipeline_->generateEmbedding(text);
- Line 2746: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (vector_index_->getDimension() == static_cast<int>(cm.embedding.size())) {
- Line 2881: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: s.total_storage_bytes = static_cast<int64_t>(storage_->getApproximateSize());
- Line 2887: severity=CRITICAL; category=data_race
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
- Line 79: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(retry_delay_ms));
- Line 153: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (!cur->is_object() || !cur->contains(key)) return nullptr;
- Line 309: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (vptr->dump() == c.dump()) { match = true; break; }
- Line 314: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (vptr->is_number()) { numeric_val = vptr->get<double>(); ok = true; }
- Line 315: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: else if (vptr->is_string()) {
- Line 317: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: numeric_val = std::stod(vptr->get<std::string>());
- Line 358: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: match = (vptr->dump() == cond.dump());
- Line 358: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: match = (vptr->dump() == cond.dump());
- Line 789: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto meta_info = kp->getKeyMetadata(encryption_key_id, 0);
- Line 968: severity=HIGH; category=allocation_loop
  Description: Dynamic allocation in loop — high overhead
  Remediation: Pre-allocate outside loop or use object pool pattern
  Context: // the previous raw-new / manual-delete pattern (CWE-401 / RAII).
- Line 981: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (target->is_null() || (target->is_object() && target->empty()) || (target->is_array() && target->
- Line 987: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto meta_info = kp->getKeyMetadata("dek", 0);
- Line 1388: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto [st, results] = vector_index_->searchKnn(q, static_cast<size_t>(k), wptr);
- Line 1388: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto [st, results] = vector_index_->searchKnn(q, static_cast<size_t>(k), wptr);
- Line 1419: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto [st, results] = vector_index_->searchKnn(q, fetch_k, wptr);
- Line 1419: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto [st, results] = vector_index_->searchKnn(q, fetch_k, wptr);
- Line 1592: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (beta != 0.0) {
  Confidence: band=very_high; score=0.9
- Line 1616: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: out.erase(std::remove_if(out.begin(), out.end(), [&](const auto& p){ return allowed.find(p.first) ==
- Line 1672: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: storage_->scanPrefix("content:", [&](std::string_view key, std::string_view value) {
- Line 1710: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: storage_->scanPrefix("content:", [&](std::string_view /*key*/, std::string_view value) {
- Line 1726: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: storage_->scanPrefix("content:", [&](std::string_view /*key*/, std::string_view value) {
- Line 2127: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["archive_id"] = archive_id;
- Line 2400: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: ocr_extraction.metadata["content_ocr_text"] = ocr_text;
- Line 2884: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: storage_->scanPrefix("content:", [&](std::string_view, std::string_view){ s.total_content_items++; r
- Line 2885: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: storage_->scanPrefix("chunk:", [&](std::string_view, std::string_view){ s.total_chunks++; return tru
- Line 94: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(hex[c >> 4]);
  Confidence: band=high; score=0.74
- Line 95: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(hex[c >> 4]);
- Line 96: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(hex[c & 0x0F]);
- Line 166: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> allowedMimes;
  Confidence: band=medium; score=0.66
- Line 167: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, json> wantedMeta;
  Confidence: band=medium; score=0.66
- Line 168: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> wantedTags;
  Confidence: band=medium; score=0.66
- Line 196: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: for (auto it = filters["metadata"].begin(); it != filters["metadata"].end(); ++it) {
  Confidence: band=high; score=0.74
- Line 210: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 218: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::string> fieldMap;
  Confidence: band=medium; score=0.66
- Line 225: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: for (auto it = sc["field_map"].begin(); it != sc["field_map"].end(); ++it) {
  Confidence: band=high; score=0.74
- Line 232: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 243: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 277: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 362: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 376: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (cid.is_string()) whitelist.push_back(std::string("chunks:") + cid.get<std::string>());
  Confidence: band=high; score=0.74
- Line 377: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (cid.is_string()) whitelist.push_back(std::string("chunks:") + cid.get<std::string>());
- Line 382: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 388: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 596: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 647: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: threat_info += r.threat_name + " (" + r.scanner_name + "); ";
  Confidence: band=high; score=0.74
- Line 679: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& mv : cj["skip_compressed_mimes"]) if (mv.is_string()) skip_mimes.push_back(mv.get<s
- Line 683: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 780: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 829: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto ftcfg = cj["fulltext_config"];
  Confidence: band=high; score=0.74
- Line 835: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (sw.is_string()) fulltext_config.stopwords.push_back(sw.get<std::string>());
  Confidence: band=high; score=0.74
- Line 836: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (sw.is_string()) fulltext_config.stopwords.push_back(sw.get<std::string>());
- Line 884: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: chunk_ids.push_back(c.id);
- Line 952: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& f : mcfg["fields"]) if (f.is_string()) meta_fields.push_back(f.get<std::string>());
- Line 1088: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& f : mcfg["fields"]) if (f.is_string()) meta_fields.push_back(f.get<std::string>());
- Line 1093: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1097: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto enc_section = j["_encrypted_meta"];
  Confidence: band=high; score=0.74
- Line 1123: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1160: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1225: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1233: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(ChunkMeta::fromJson(j));
  Confidence: band=high; score=0.74
- Line 1234: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(ChunkMeta::fromJson(j));
- Line 1237: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1255: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1366: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(chunk);
  Confidence: band=high; score=0.74
- Line 1367: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(chunk);
- Line 1390: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: res.emplace_back(r.pk, r.distance);
  Confidence: band=high; score=0.74
- Line 1407: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, float> vector_scores;
  Confidence: band=medium; score=0.66
- Line 1408: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, size_t> vector_ranks;
  Confidence: band=medium; score=0.66
- Line 1443: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, float> fulltext_scores;
  Confidence: band=medium; score=0.66
- Line 1444: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, size_t> fulltext_ranks;
  Confidence: band=medium; score=0.66
- Line 1507: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, float> rrf_scores;
  Confidence: band=medium; score=0.66
- Line 1522: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.emplace_back(chunk_id, score);
  Confidence: band=high; score=0.74
- Line 1522: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.emplace_back(chunk_id, score);
  Confidence: band=high; score=0.74
- Line 1522: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.emplace_back(chunk_id, score);
  Confidence: band=high; score=0.74
- Line 1555: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1559: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, double> bestScore; bestScore.reserve(base.size()*2);
  Confidence: band=medium; score=0.66
- Line 1581: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> seen;
  Confidence: band=medium; score=0.66
- Line 1609: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.emplace_back(kv.first, static_cast<float>(kv.second));
  Confidence: band=high; score=0.74
- Line 1616: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> allowed(allow.begin(), allow.end());
  Confidence: band=medium; score=0.66
- Line 1619: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1684: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1717: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1741: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 2255: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks_json.push_back(cm.toJson());
  Confidence: band=high; score=0.74
- Line 2256: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: chunks_json.push_back(cm.toJson());
- Line 2283: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.stage_outcomes.push_back(std::move(extraction_outcome));
- Line 2288: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.stage_outcomes.push_back(std::move(extraction_outcome));
- Line 2334: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks_json.push_back(cm.toJson());
  Confidence: band=high; score=0.74
- Line 2335: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: chunks_json.push_back(cm.toJson());
- Line 2362: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.stage_outcomes.push_back(std::move(extraction_outcome));
- Line 2367: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.stage_outcomes.push_back(std::move(extraction_outcome));
- Line 2415: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks_json.push_back(cm.toJson());
  Confidence: band=high; score=0.74
- Line 2416: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: chunks_json.push_back(cm.toJson());
- Line 2688: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto ftcfg = cj["fulltext_config"];
  Confidence: band=high; score=0.74
- Line 2694: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (sw.is_string()) fulltext_config.stopwords.push_back(sw.get<std::string>());
  Confidence: band=high; score=0.74
- Line 2695: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (sw.is_string()) fulltext_config.stopwords.push_back(sw.get<std::string>());
- Line 2700: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/content/office_processor.cpp
Total findings: 82

- Line 843: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: ssize_t written = write(in_fd, bdata, remaining);
- Line 1003: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: int out_fd = open(out_path.c_str(), O_RDONLY);
- Line 1014: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: while ((n = read(out_fd, buf, sizeof(buf))) > 0) {
- Line 96: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Check for legacy Office formats (OLE Compound Document)
  Confidence: band=high; score=0.8
- Line 102: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Legacy Office format - try to determine type
  Confidence: band=high; score=0.8
- Line 166: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["mime_type"]     = DOCX_CONTENT_TYPE;
- Line 171: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["document_type"] = "xlsx";
- Line 172: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["mime_type"]     = XLSX_CONTENT_TYPE;
- Line 177: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["document_type"] = "pptx";
- Line 178: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["mime_type"]     = PPTX_CONTENT_TYPE;
- Line 185: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["document_type"] = "odf";
- Line 196: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["document_type"] = "rtf";
- Line 213: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["extraction_method"] = "basic_rtf";
- Line 239: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["document_type"] = "docx";
- Line 240: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["mime_type"]     = DOCX_CONTENT_TYPE;
- Line 254: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["title"]         = metadata.title;
- Line 255: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["author"]        = metadata.author;
- Line 256: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["subject"]       = metadata.subject;
- Line 257: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["keywords"]      = metadata.keywords;
- Line 258: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["created_date"]  = metadata.created_date;
- Line 297: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["note"] = "Full DOCX extraction requires building with -DTHEMIS_ENABLE_OFFICE=ON";
- Line 311: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["extraction_method"] = "basic_regex";
- Line 312: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["token_count"]       = countTokens(result.text);
- Line 323: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["document_type"] = "xlsx";
- Line 324: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["mime_type"]     = XLSX_CONTENT_TYPE;
- Line 348: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["title"]         = metadata.title;
- Line 349: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["author"]        = metadata.author;
- Line 350: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["created_date"]  = metadata.created_date;
- Line 426: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["row_count"]   = row_count;
- Line 427: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["cell_count"]  = cell_count;
- Line 428: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["token_count"] = countTokens(result.text);
- Line 435: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["note"]              = "Full XLSX extraction requires building with -DTHEMIS_ENABLE_
- Line 436: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["extraction_method"] = "not_available";
- Line 448: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["document_type"] = "pptx";
- Line 449: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["mime_type"]     = PPTX_CONTENT_TYPE;
- Line 456: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["author"]        = metadata.author;
- Line 457: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["created_date"]  = metadata.created_date;
- Line 464: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (entry.find("ppt/slides/slide") != std::string::npos && entry.find(".xml") != std::string::npos)
- Line 525: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["token_count"] = countTokens(result.text);
- Line 532: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["note"]              = "Full PPTX extraction requires building with -DTHEMIS_ENABLE_
- Line 533: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["extraction_method"] = "not_available";
- Line 561: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["document_type"] = type_str;
- Line 588: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: } else if (std::string(child.name()).find("text:") == 0) {
  Confidence: band=very_high; score=0.9
- Line 598: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["token_count"] = countTokens(result.text);
- Line 605: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["note"] = "Full ODF extraction requires building with -DTHEMIS_ENABLE_OFFICE=ON";
- Line 748: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // LibreOffice headless fallback for legacy OLE formats (DOC/XLS/PPT)
  Confidence: band=high; score=0.8
- Line 776: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["document_type"]     = type_str;
- Line 777: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["extraction_method"] = "libreoffice_headless";
- Line 814: severity=HIGH; category=posix_only_api
  Description: POSIX-only API unlink( without platform guard
  Remediation: Wrap in #ifndef _WIN32 ... #endif or provide Windows alternative
  Context: unlink(in_file.c_str());
- Line 816: severity=HIGH; category=posix_only_api
  Description: POSIX-only API unlink( without platform guard
  Remediation: Wrap in #ifndef _WIN32 ... #endif or provide Windows alternative
  Context: unlink(out_file.c_str());
- Line 1026: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["token_count"] = countTokens(extracted_text);
- Line 0: severity=MEDIUM; category=uncategorized
  Context: ['            const size_t combined = token_hash ^ (i * 31u) ^ (static_cast<size_t>(seed) * 97u);', '            for (int d = 0; d < 10; ++d) {', '                const int dim = static_cast<int>((combined + static_cast<size_t>(d) * 73u) % static_cast<size_t>(kDim));', '                const float weight = 1.0f / (1.0f + static_cast<float>(i) * 0.1f);', '                const float phase']
  Confidence: band=medium; score=0.62
- Line 208: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: text += content + " ";
  Confidence: band=high; score=0.74
- Line 281: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: paragraphs.push_back(para_text);
  Confidence: band=high; score=0.74
- Line 281: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: paragraphs.push_back(para_text);
  Confidence: band=high; score=0.74
- Line 282: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: paragraphs.push_back(para_text);
- Line 340: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: shared_strings.push_back(text);
  Confidence: band=high; score=0.74
- Line 340: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: shared_strings.push_back(text);
  Confidence: band=high; score=0.74
- Line 341: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: shared_strings.push_back(text);
- Line 361: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: sheet_names.push_back(name);
  Confidence: band=high; score=0.74
- Line 362: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: sheet_names.push_back(name);
- Line 465: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: slides.push_back(entry);
  Confidence: band=high; score=0.74
- Line 466: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: slides.push_back(entry);
- Line 627: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: zip_source_free(source);
- Line 634: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: zip_close(archive);
- Line 640: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: zip_close(archive);
- Line 646: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: zip_close(archive);
- Line 653: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: zip_fclose(file);
- Line 654: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: zip_close(archive);
- Line 717: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: zip_source_free(source);
- Line 725: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entries.push_back(name);
  Confidence: band=high; score=0.74
- Line 726: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: entries.push_back(name);
- Line 730: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: zip_close(archive);
- Line 847: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close(in_fd);
- Line 854: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close(in_fd);
- Line 1018: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close(out_fd);
- Line 1022: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close(out_fd);
- Line 1053: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: paragraphs.push_back(line);
- Line 1069: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks.push_back(chunk);
  Confidence: band=high; score=0.74
- Line 1070: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: chunks.push_back(chunk);
- Line 1076: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: current_chunk += "\n";
  Confidence: band=high; score=0.74
- Line 1113: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back(tok);

### src/content/geo_processor.cpp
Total findings: 70

- Line 530: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: OGRPoint* point = static_cast<OGRPoint*>(multipoint->getGeometryRef(g));
- Line 0: severity=HIGH; category=uncategorized
  Context: Array declared without initialization
  Confidence: band=high; score=0.81
- Line 201: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["geometry_type"] = geo.geometry_type;
- Line 202: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["crs"] = geo.crs;
- Line 289: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: chunk.metadata["start_index"] = i;
- Line 290: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: chunk.metadata["end_index"] = end;
- Line 369: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: data.bounds[0] = std::min(data.bounds[0], coord.second);
- Line 370: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: data.bounds[1] = std::min(data.bounds[1], coord.first);
- Line 371: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: data.bounds[2] = std::max(data.bounds[2], coord.second);
- Line 372: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: data.bounds[3] = std::max(data.bounds[3], coord.first);
- Line 377: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(std::string("GeoJSON parse error: ") + e.what());
- Line 437: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Failed to create VSI memory file for shapefile");
- Line 449: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Failed to open shapefile with GDAL");
- Line 454: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: int layer_count = dataset->GetLayerCount();
- Line 456: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: OGRLayer* layer = dataset->GetLayer(i);
- Line 485: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: while ((feature = layer->GetNextFeature()) != nullptr &&
- Line 485: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: while ((feature = layer->GetNextFeature()) != nullptr &&
- Line 531: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: data.coordinates.emplace_back(point->getY(), point->getX());
- Line 565: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: data.properties["feature_count"] = std::to_string(feature_count);
- Line 566: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: data.properties["layer_name"] = layer->GetName();
- Line 577: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: data.bounds[0] = std::min(data.bounds[0], coord.second);
- Line 578: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: data.bounds[1] = std::min(data.bounds[1], coord.first);
- Line 579: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: data.bounds[2] = std::max(data.bounds[2], coord.second);
- Line 580: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: data.bounds[3] = std::max(data.bounds[3], coord.first);
- Line 593: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("GDAL support not enabled. Build with -DTHEMIS_ENABLE_GDAL=ON");
- Line 615: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Failed to create VSI memory file for GeoPackage");
- Line 627: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Failed to open GeoPackage with GDAL");
- Line 632: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: int layer_count = dataset->GetLayerCount();
- Line 634: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: OGRLayer* layer = dataset->GetLayer(i);
- Line 663: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: while ((feature = layer->GetNextFeature()) != nullptr &&
- Line 663: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: while ((feature = layer->GetNextFeature()) != nullptr &&
- Line 675: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: data.properties["feature_count"] = std::to_string(feature_count);
- Line 676: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: data.properties["layer_name"] = layer->GetName();
- Line 723: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Failed to open GeoTIFF with GDAL");
- Line 728: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: int width = dataset->GetRasterXSize();
- Line 729: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: int height = dataset->GetRasterYSize();
- Line 730: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: int band_count = dataset->GetRasterCount();
- Line 732: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: data.properties["width"] = std::to_string(width);
- Line 733: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: data.properties["height"] = std::to_string(height);
- Line 734: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: data.properties["bands"] = std::to_string(band_count);
- Line 735: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: data.properties["size_pixels"] = std::to_string(width * height);
- Line 744: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: data.properties["rotation_x"] = std::to_string(geotransform[2]);
- Line 745: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: data.properties["rotation_y"] = std::to_string(geotransform[4]);
- Line 753: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: data.bounds[0] = minX;
- Line 754: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: data.bounds[1] = minY;
- Line 755: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: data.bounds[2] = maxX;
- Line 756: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: data.bounds[3] = maxY;
- Line 758: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: data.properties["bounds_minX"] = std::to_string(minX);
- Line 759: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: data.properties["bounds_minY"] = std::to_string(minY);
- Line 760: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: data.properties["bounds_maxX"] = std::to_string(maxX);
- Line 761: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: data.properties["bounds_maxY"] = std::to_string(maxY);
- Line 765: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: const char* projection = dataset->GetProjectionRef();
- Line 784: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: GDALRasterBand* band = dataset->GetRasterBand(i);
- Line 790: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: data.properties[band_prefix + "_data_type"] = GDALGetDataTypeName(dtype);
- Line 795: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: data.properties[band_prefix + "_block_size"] =
- Line 800: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: data.properties[band_prefix + "_color_interpretation"] =
- Line 805: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: double nodata = band->GetNoDataValue(&has_nodata);
- Line 838: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("GDAL support not enabled. Build with -DTHEMIS_ENABLE_GDAL=ON");
- Line 134: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: bool GeoProcessor::canProcess(const std::string& mime_type) const {
  Confidence: band=high; score=0.74
- Line 180: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: (mime_type == "application/json" && content.find("\"type\"") != std::string::npos)) {
- Line 180: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: (mime_type == "application/json" && content.find("\"type\"") != std::string::npos)) {
- Line 291: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks.push_back(chunk);
  Confidence: band=high; score=0.74
- Line 291: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks.push_back(chunk);
  Confidence: band=high; score=0.74
- Line 292: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: chunks.push_back(chunk);
- Line 506: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: data.coordinates.emplace_back(
  Confidence: band=high; score=0.74
- Line 518: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: data.coordinates.emplace_back(
  Confidence: band=high; score=0.74
- Line 530: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: data.coordinates.emplace_back(point->getY(), point->getX());
  Confidence: band=high; score=0.74
- Line 584: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 678: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 829: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/content/stt_processor.cpp
Total findings: 68

- Line 0: severity=HIGH; category=uncategorized
  Context: ['            size_t bs  = static_cast<size_t>(b) * band_size;', '            size_t be  = (b == kBands - 1) ? n : bs + band_size;', '            size_t len = be - bs;', '', '            float rms = 0.0f;']
  Confidence: band=high; score=0.81
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 165: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["transcription"] = {{"language", transcription.detected_language},
- Line 183: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["segments"] = segments_json;
- Line 231: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: chunk.metadata["start_ms"] = chunk_start_ms;
- Line 232: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: chunk.metadata["end_ms"]   = chunk_end_ms;
- Line 256: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: chunk.metadata["start_ms"] = chunk_start_ms;
- Line 257: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: chunk.metadata["end_ms"]   = chunk_end_ms;
- Line 477: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("WAV file too small: " + std::to_string(wav_data.size()) + " bytes (minimum
- Line 482: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto readUInt32LE = [&wav_data](size_t offset) -> uint32_t {
- Line 484: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Buffer overflow reading uint32 at offset " + std::to_string(offset));
- Line 488: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: | (static_cast<uint32_t>(wav_data[offset + 3]) << 24);
- Line 492: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto readUInt16LE = [&wav_data](size_t offset) -> uint16_t {
- Line 494: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Buffer overflow reading uint16 at offset " + std::to_string(offset));
- Line 501: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Invalid WAV file: missing RIFF header");
- Line 509: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Invalid WAV file: missing WAVE format identifier");
- Line 532: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Invalid fmt chunk size: " + std::to_string(fmt_size));
- Line 536: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("fmt chunk extends beyond file boundary");
- Line 554: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Chunk size extends beyond file boundary");
- Line 569: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("WAV file missing fmt chunk");
- Line 574: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Unsupported audio format: " + std::to_string(audio_format)
- Line 575: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: + " (only PCM [1] and IEEE float [3] are supported)");
- Line 580: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Invalid number of channels: 0");
- Line 584: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Invalid sample rate: 0");
- Line 588: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Invalid bits per sample: " + std::to_string(bits_per_sample));
- Line 615: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Chunk size extends beyond file boundary");
- Line 630: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("WAV file missing data chunk");
- Line 634: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("data chunk extends beyond file boundary");
- Line 642: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Integer overflow in sample size calculation");
- Line 647: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Invalid frame size: 0");
- Line 667: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Sample offset out of bounds");
- Line 678: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("24-bit sample extends beyond buffer");
- Line 682: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: | (static_cast<uint32_t>(wav_data[sample_offset + 1]) << 8)
- Line 683: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: | (static_cast<uint32_t>(wav_data[sample_offset + 2]) << 16));
- Line 694: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Unsupported PCM bit depth: " + std::to_string(bits_per_sample));
- Line 709: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Unsupported IEEE float bit depth: " + std::to_string(bits_per_sample));
- Line 945: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: rms += data[i] * data[i];
- Line 989: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // Assumes both inputs are L2-normalised; result is in [-1, 1].
  Confidence: band=very_high; score=0.9
- Line 1138: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: protocol["key_points"]   = json::array();
- Line 1139: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: protocol["action_items"] = json::array();
- Line 1151: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: char buffer[32];
- Line 1152: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d.%03d", hours, minutes, seconds, millis);
  Confidence: band=very_high; score=0.9
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 108: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: bool STTProcessor::canProcess(const std::string &mime_type) const {
  Confidence: band=high; score=0.74
- Line 180: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: segments_json.push_back(seg_json);
  Confidence: band=high; score=0.74
- Line 181: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: segments_json.push_back(seg_json);
- Line 232: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks.push_back(chunk);
  Confidence: band=high; score=0.74
- Line 233: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: chunks.push_back(chunk);
- Line 243: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: current_chunk += " ";
  Confidence: band=high; score=0.74
- Line 275: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks.push_back(chunk);
  Confidence: band=high; score=0.74
- Line 276: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: chunks.push_back(chunk);
- Line 281: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: current_chunk += " ";
  Confidence: band=high; score=0.74
- Line 344: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 456: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: whisper_free(static_cast<struct whisper_context *>(whisper_ctx_));
- Line 724: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: pcm_data.push_back(mixed_sample);
- Line 807: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.segments.push_back(segment);
  Confidence: band=high; score=0.74
- Line 808: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.segments.push_back(segment);
- Line 1004: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: features.push_back(std::move(fv));
  Confidence: band=high; score=0.74
- Line 1005: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: features.push_back(std::move(fv));
- Line 1030: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: centroids.push_back(features[0]);
- Line 1047: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: centroids.push_back(features[best_idx]);
  Confidence: band=high; score=0.74
- Line 1047: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: centroids.push_back(features[best_idx]);
  Confidence: band=high; score=0.74
- Line 1047: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: centroids.push_back(features[best_idx]);
  Confidence: band=high; score=0.74
- Line 1048: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: centroids.push_back(features[best_idx]);
- Line 1131: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: segments.push_back(seg_json);
  Confidence: band=high; score=0.74
- Line 1132: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: segments.push_back(seg_json);

### src/content/video_processor.cpp
Total findings: 58

- Line 556: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: data.framerate = static_cast<double>(stream->avg_frame_rate.num) / stream->avg_frame_rate.den;
- Line 556: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: data.framerate = static_cast<double>(stream->avg_frame_rate.num) / stream->avg_frame_rate.den;
- Line 720: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: double aspect = static_cast<double>(frame->width) / frame->height;
- Line 733: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: = sws_getContext(frame->width, frame->height, static_cast<AVPixelFormat>(frame->format), thumb_width
- Line 763: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (rgb_frame->linesize[0] == static_cast<int>(row_size)) {
- Line 962: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const uint8_t *ra = a->data[0] + static_cast<ptrdiff_t>(y) * a->linesize[0];
- Line 963: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const uint8_t *rb = b->data[0] + static_cast<ptrdiff_t>(y) * b->linesize[0];
- Line 211: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["duration_ms"]      = media.duration_ms;
- Line 212: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["width"]            = media.width;
- Line 213: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["height"]           = media.height;
- Line 214: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["video_codec"]      = media.video_codec;
- Line 215: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["audio_codec"]      = media.audio_codec;
- Line 216: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["bitrate_kbps"]     = media.bitrate_kbps;
- Line 217: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["framerate"]        = media.framerate;
- Line 218: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["container_format"] = media.container_format;
- Line 254: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["keyframe_timestamps_ms"] = kf_times;
- Line 275: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["scene_changes_ms"] = scene_times;
- Line 568: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: data.channels = codecpar->channels;
- Line 656: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("No video stream found");
- Line 667: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Decoder not found");
- Line 675: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Failed to allocate codec context");
- Line 682: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Failed to copy codec parameters");
- Line 689: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Failed to open codec");
- Line 728: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Thumbnail dimensions exceed RGB buffer limits");
- Line 745: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: sws_scale(sws_ctx, frame->data, frame->linesize, 0, frame->height, rgb_frame->data,
- Line 757: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: const uint8_t* src = rgb_frame->data[0];
- Line 760: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Invalid RGB frame line size");
- Line 962: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: const uint8_t *ra = a->data[0] + static_cast<ptrdiff_t>(y) * a->linesize[0];
- Line 963: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: const uint8_t *rb = b->data[0] + static_cast<ptrdiff_t>(y) * b->linesize[0];
- Line 160: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: bool VideoProcessor::canProcess(const std::string &mime_type) const {
  Confidence: band=high; score=0.74
- Line 251: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: kf_times.push_back(time);
  Confidence: band=high; score=0.74
- Line 252: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: kf_times.push_back(time);
- Line 272: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: scene_times.push_back(time);
  Confidence: band=high; score=0.74
- Line 273: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: scene_times.push_back(time);
- Line 318: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: chunks.push_back(chunk);
- Line 323: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: current_text += " ";
- Line 468: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: keyframes.push_back(i * interval_ms);
  Confidence: band=high; score=0.74
- Line 469: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: keyframes.push_back(i * interval_ms);
- Line 507: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: temp_file.close();
- Line 583: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 629: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: temp_file.close();
- Line 776: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: av_frame_free(&rgb_frame);
- Line 782: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: av_frame_free(&frame);
- Line 783: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: av_packet_free(&packet);
- Line 788: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 823: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: temp_file.close();
- Line 853: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: keyframes.push_back(pts_ms);
- Line 861: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: av_packet_free(&packet);
- Line 864: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 898: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: temp_file.close();
- Line 980: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: scenes.push_back(pts_ms);
  Confidence: band=high; score=0.74
- Line 980: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: scenes.push_back(pts_ms);
  Confidence: band=high; score=0.74
- Line 980: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: scenes.push_back(pts_ms);
  Confidence: band=high; score=0.74
- Line 981: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: scenes.push_back(pts_ms);
- Line 1005: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: av_frame_free(&frame);
- Line 1006: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: av_frame_free(&prev_frame);
- Line 1007: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: av_packet_free(&packet);
- Line 1011: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/content/async_ingestion_worker.cpp
Total findings: 52

- Line 0: severity=CRITICAL; category=uncategorized
  Confidence: band=very_high; score=0.85
- Line 263: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: backpressure_cv_.wait(lock, [this] {
- Line 324: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: backpressure_cv_.wait(lock, [this] {
- Line 542: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = job_history_.begin();
- Line 590: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: queue_cv_.wait(lock, [this] { return !job_queue_.empty() || shutdown_requested_.load(); });
- Line 845: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = plugins_.find(plugin_name);
- Line 912: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: job.config.merge_patch(additional_config);
  Confidence: band=very_high; score=0.99
- Line 921: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: job.total_items = static_cast<int>(plugin->estimateJobSize(job));
- Line 1053: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: it->second.progress        = total > 0 ? static_cast<float>(processed) / total : 0.0f;
- Line 107: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("ContentManager cannot be null");
- Line 138: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: cleanup_thread_ = std::thread(&AsyncIngestionWorker::cleanupLoop, this);
  Confidence: band=very_high; score=0.9
- Line 197: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Worker not running");
- Line 217: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Job queue full");
- Line 240: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Worker not running");
- Line 268: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Worker shutting down");
- Line 294: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Worker not running");
- Line 329: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: promise->set_exception(std::make_exception_ptr(std::runtime_error("Worker shutting down")));
- Line 355: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Worker not running");
- Line 374: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Job queue full");
- Line 396: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Worker not running");
- Line 412: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto &[filename, blob] : files) {
  Confidence: band=very_high; score=0.9
- Line 419: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto &[filename, blob] : files) {
  Confidence: band=very_high; score=0.9
- Line 427: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Job queue full");
- Line 481: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto &[job_id, job] : job_history_) {
  Confidence: band=very_high; score=0.9
- Line 612: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: job.completion_promise->set_exception(std::make_exception_ptr(std::runtime_error("Job cancelled")));
- Line 723: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Unsupported job type: " + jobTypeToString(job.type));
- Line 733: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(result.error_message);
- Line 748: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Stream job has no stream pointer");
- Line 756: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(result.error_message);
- Line 773: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(result.error_message);
- Line 789: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Batch job missing file blobs");
- Line 828: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("Plugin cannot be null");
- Line 859: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto &[name, plugin] : plugins_) {
  Confidence: band=very_high; score=0.9
- Line 878: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Worker not running");
- Line 897: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Plugin not found: " + source.plugin_name);
- Line 912: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: job.config.merge_patch(additional_config);
  Confidence: band=very_high; score=0.9
- Line 1020: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Plugin not found: " + source.plugin_name);
- Line 1064: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::minutes(5));
- Line 114: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 132: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: workers_.emplace_back(&AsyncIngestionWorker::workerLoop, this, static_cast<int>(i));
  Confidence: band=high; score=0.74
- Line 275: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: && !queue_depth_high_watermark_.compare_exchange_weak(old_hwm, depth, std::memory_order_relaxed)) {
  Confidence: band=high; score=0.74
- Line 337: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: && !queue_depth_high_watermark_.compare_exchange_weak(old_hwm, depth, std::memory_order_relaxed)) {
  Confidence: band=high; score=0.74
- Line 412: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: file_list.push_back({{"filename", filename}, {"size", blob.size()}});
  Confidence: band=high; score=0.74
- Line 413: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: file_list.push_back({{"filename", filename}, {"size", blob.size()}});
- Line 419: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: job.config["_blobs"].push_back({{"filename", filename}, {"blob", blob}});
  Confidence: band=high; score=0.74
- Line 420: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: job.config["_blobs"].push_back({{"filename", filename}, {"blob", blob}});
- Line 482: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(job);
  Confidence: band=high; score=0.74
- Line 483: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(job);
- Line 804: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: job.content_ids.push_back(result.primary_content_id);
  Confidence: band=high; score=0.74
- Line 805: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: job.content_ids.push_back(result.primary_content_id);
- Line 859: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: names.push_back(name);
  Confidence: band=high; score=0.74
- Line 860: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: names.push_back(name);

### src/content/pdf_processor.cpp
Total findings: 51

- Line 161: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: int max_pages = config_.max_pages > 0 ? std::min(config_.max_pages, doc->pages()) : doc->pages();
- Line 328: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: int max_pages = config_.max_pages > 0 ? std::min(config_.max_pages, doc->pages()) : doc->pages();
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 98: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["mime_type"]  = "application/pdf";
- Line 147: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["title"]             = metadata.title;
- Line 148: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["author"]            = metadata.author;
- Line 149: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["subject"]           = metadata.subject;
- Line 150: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["keywords"]          = metadata.keywords;
- Line 151: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["creator"]           = metadata.creator;
- Line 152: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["producer"]          = metadata.producer;
- Line 153: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["creation_date"]     = metadata.creation_date;
- Line 165: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: std::unique_ptr<poppler::page> page(doc->create_page(i));
- Line 165: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: std::unique_ptr<poppler::page> page(doc->create_page(i));
- Line 177: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: poppler::byte_array bytes = page->text().to_utf8();
- Line 195: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: page_obj["rotation"] = page->orientation() * 90;
- Line 200: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["extracted_pages"]  = max_pages;
- Line 201: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["pages"]            = pages_array;
- Line 202: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["layout_preserved"] = config_.maintain_layout;
- Line 203: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["token_count"]      = countTokens(result.text);
- Line 222: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["page_count"] = page_count;
- Line 235: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["extraction_method"] = "basic_regex";
- Line 236: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["note"]              = "Full PDF extraction requires building with -DTHEMIS_ENABLE_P
- Line 237: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["layout_preserved"]  = false;
- Line 238: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["token_count"]       = countTokens(result.text);
- Line 272: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata.subject  = to_string(doc->get_subject());
- Line 273: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata.keywords = to_string(doc->get_keywords());
- Line 274: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata.creator  = to_string(doc->get_creator());
- Line 275: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata.producer = to_string(doc->get_producer());
- Line 293: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata.is_linearized = doc->is_linearized();
- Line 331: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: std::unique_ptr<poppler::page> page(doc->create_page(i));
- Line 331: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: std::unique_ptr<poppler::page> page(doc->create_page(i));
- Line 344: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: poppler::byte_array text_bytes = page->text().to_utf8();
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
- Line 218: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: std::regex page_regex("/Type\\s*/Page[^s]");
- Line 299: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: std::regex pattern("/" + key + "\\s*\\(([^)]+)\\)");
- Line 299: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: std::regex pattern("/" + key + "\\s*\\(([^)]+)\\)");
- Line 385: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: items.push_back({static_cast<float>(r.x()), static_cast<float>(r.y()), static_cast<float>(r.width()),
  Confidence: band=high; score=0.74
- Line 386: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: items.push_back({static_cast<float>(r.x()), static_cast<float>(r.y()), static_cast<float>(r.width())
- Line 407: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: positions_out.push_back({item.x, item.y});
  Confidence: band=high; score=0.74
- Line 408: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: positions_out.push_back({item.x, item.y});
- Line 476: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: sentences.push_back(sentence);
  Confidence: band=high; score=0.74
- Line 477: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: sentences.push_back(sentence);
- Line 492: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks.push_back(chunk);
  Confidence: band=high; score=0.74
- Line 493: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: chunks.push_back(chunk);
- Line 506: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: current_chunk += " ";
  Confidence: band=high; score=0.74
- Line 544: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back(tok);

### src/content/mime_detector.cpp
Total findings: 40

- Line 73: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: bool verified = sig_mgr_->verifyFile(config_path, resource_id);
- Line 37: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Try multiple locations (new hierarchical structure first, then legacy)
  Confidence: band=high; score=0.8
- Line 310: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: buffer += "[extensions]\n";
- Line 321: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: snprintf(tmp, sizeof(tmp), "%02x", static_cast<unsigned int>(b));
  Confidence: band=very_high; score=0.9
- Line 337: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: buffer += "[magic]\n";
- Line 354: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: buffer += "[categories]\n";
- Line 362: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: snprintf(&hex_out[i * 2], 3, "%02x", static_cast<unsigned int>(digest[i]));
  Confidence: band=very_high; score=0.9
- Line 113: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: sig.signature.push_back(static_cast<uint8_t>(byte.as<int>()));
  Confidence: band=high; score=0.74
- Line 113: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: sig.signature.push_back(static_cast<uint8_t>(byte.as<int>()));
  Confidence: band=high; score=0.74
- Line 114: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: sig.signature.push_back(static_cast<uint8_t>(byte.as<int>()));
- Line 129: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: magic_signatures_.push_back(std::move(sig));
  Confidence: band=high; score=0.74
- Line 130: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: magic_signatures_.push_back(std::move(sig));
- Line 171: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: policy_.allowed.push_back(std::move(mp));
  Confidence: band=high; score=0.74
- Line 172: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: policy_.allowed.push_back(std::move(mp));
- Line 183: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: policy_.denied.push_back(std::move(mp));
  Confidence: band=high; score=0.74
- Line 184: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: policy_.denied.push_back(std::move(mp));
- Line 218: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 307: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ext_lines.push_back(kv.first + "=" + kv.second);
- Line 309: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: buffer += "[extensions]\n";
  Confidence: band=high; score=0.74
- Line 309: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: buffer += "[extensions]\n";
  Confidence: band=high; score=0.74
- Line 321: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: snprintf(tmp, sizeof(tmp), "%02x", static_cast<unsigned int>(b));
- Line 321: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: snprintf(tmp, sizeof(tmp), "%02x", static_cast<unsigned int>(b));
- Line 325: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: wildcards += ":";
  Confidence: band=high; score=0.74
- Line 325: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: wildcards += ":";
  Confidence: band=high; score=0.74
- Line 325: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: wildcards += ":";
  Confidence: band=high; score=0.74
- Line 326: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: wildcards += ":";
- Line 328: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!first) wildcards += ",";
  Confidence: band=high; score=0.74
- Line 329: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (!first) wildcards += ",";
- Line 329: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (!first) wildcards += ",";
- Line 333: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: magic_lines.push_back(sig.mime_type + "@" + std::to_string(sig.offset) + "=" + hex + wildcards);
  Confidence: band=high; score=0.74
- Line 334: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: magic_lines.push_back(sig.mime_type + "@" + std::to_string(sig.offset) + "=" + hex + wildcards);
- Line 347: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i) joined += ",";
  Confidence: band=high; score=0.74
- Line 347: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i) joined += ",";
  Confidence: band=high; score=0.74
- Line 347: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i) joined += ",";
  Confidence: band=high; score=0.74
- Line 348: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (i) joined += ",";
- Line 348: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (i) joined += ",";
- Line 350: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: category_lines.push_back(cat.first + "=" + joined);
  Confidence: band=high; score=0.74
- Line 350: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: category_lines.push_back(cat.first + "=" + joined);
  Confidence: band=high; score=0.74
- Line 351: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: category_lines.push_back(cat.first + "=" + joined);
- Line 362: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: snprintf(&hex_out[i * 2], 3, "%02x", static_cast<unsigned int>(digest[i]));

### src/content/content_logger.cpp
Total findings: 28

- Line 40: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["content_id"] = content_id;
- Line 41: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["mime_type"] = mime_type;
- Line 42: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["size_bytes"] = size_bytes;
- Line 60: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["content_id"] = content_id;
- Line 61: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["mime_type"] = mime_type;
- Line 62: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["size_bytes"] = size_bytes;
- Line 63: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["success"] = success;
- Line 64: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["duration_ms"] = duration_ms;
- Line 85: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["content_id"] = content_id;
- Line 86: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["operation"] = operation;
- Line 87: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["duration_ms"] = duration_ms;
- Line 88: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["success"] = success;
- Line 112: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["content_id"] = content_id;
- Line 113: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["operation"] = operation;
- Line 114: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["error_code"] = error_code;
- Line 115: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["error_message"] = pii_sanitization_ ? sanitizeMessage(error_message) : error_message;
- Line 131: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["content_id"] = content_id;
- Line 132: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["operation"] = operation;
- Line 133: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["timeout_seconds"] = timeout_seconds;
- Line 134: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["elapsed_seconds"] = elapsed_seconds;
- Line 144: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["content_id"] = content_id;
- Line 145: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["hit"] = hit;
- Line 213: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: size_t pos = filename.find_last_of("/\\");
- Line 156: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: void ContentLogger::log(
  Confidence: band=medium; score=0.6
- Line 187: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: log(utils::Logger::Level::INFO, event, message, metadata);
  Confidence: band=medium; score=0.6
- Line 191: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: log(utils::Logger::Level::WARN, event, message, metadata);
  Confidence: band=medium; score=0.6
- Line 195: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: log(utils::Logger::Level::ERROR, event, message, metadata);
  Confidence: band=medium; score=0.6
- Line 199: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: log(utils::Logger::Level::DEBUG, event, message, metadata);
  Confidence: band=medium; score=0.6

### src/content/html_processor.cpp
Total findings: 27

- Line 335: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator start may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto start = t.find_first_not_of(" \t\r\n");
- Line 336: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator end may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto end   = t.find_last_not_of(" \t\r\n");
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
- Line 0: severity=MEDIUM; category=uncategorized
  Context: ['            size_t combined = token_hash ^ (i * 31) ^ (static_cast<size_t>(seed) * 97);', '            for (int d = 0; d < 10; ++d) {', '                int dim = static_cast<int>((combined + static_cast<size_t>(d) * 73) % DIM);', '                float weight = 1.0f / (1.0f + static_cast<float>(i) * 0.1f);', '                float phase  = static_cast<float>((combined + static_cast<size_t>(dim)) % 360)']
  Confidence: band=medium; score=0.62
- Line 192: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: static const std::regex heading_close(R"(<\s*/\s*h[1-6][^>]*>)",
- Line 192: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: static const std::regex heading_close(R"(<\s*/\s*h[1-6][^>]*>)",
- Line 204: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: replaced += '\n';
  Confidence: band=high; score=0.74
- Line 205: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: replaced += '\n';
- Line 207: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: replaced += ' ';
- Line 218: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: R"(<\s*/?\s*(p|div|article|section|main|h[1-6]|li|ul|ol|blockquote|pre|br|tr|td|th|dt|dd)[^>]*>)",
- Line 323: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: R"(<title[^>]*>([\s\S]*?)<\/title>)",
- Line 406: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: result += '\n';
- Line 413: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: result += ' ';
- Line 514: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: paragraphs.push_back(para);
  Confidence: band=high; score=0.74
- Line 515: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: paragraphs.push_back(para);
- Line 522: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: paragraphs.push_back(para);
- Line 530: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (!para.empty()) paragraphs.push_back(para);
- Line 566: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: while (iss >> tok) tokens.push_back(tok);
  Confidence: band=high; score=0.74
- Line 567: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: while (iss >> tok) tokens.push_back(tok);
- Line 571: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (!overlap_text.empty()) overlap_text += ' ';
- Line 597: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: while (iss >> token) tokens.push_back(token);

### src/content/archive_processor.cpp
Total findings: 26

- Line 0: severity=HIGH; category=uncategorized
  Context: ['            // Determine entry type from typeflag (byte 156):', "            //   '0'/NUL = regular file, '5' = directory, '2' = symlink, etc.", '            const char typeflag = block[kTypeOffset];', "            const bool is_dir   = (typeflag == '5');", '']
  Confidence: band=high; score=0.78
- Line 365: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: return metadata.has_value() && metadata->is_encrypted;
- Line 570: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: char buffer[8192];
- Line 706: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::snprintf(name, sizeof(name), "%.*s/%.*s", 155, prefix, 100, reinterpret_cast<const char *>(hdr));
  Confidence: band=very_high; score=0.9
- Line 708: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::snprintf(name, sizeof(name), "%.*s", 100, reinterpret_cast<const char *>(hdr));
  Confidence: band=very_high; score=0.9
- Line 892: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["members"] = members;
- Line 917: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["extracted_files"] = extracted_files;
- Line 101: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 114: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: bool ArchiveProcessor::canHandle(const std::string &mime_type) const {
  Confidence: band=high; score=0.74
- Line 251: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: zip_close(za);
- Line 387: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: components.push_back(component);
- Line 391: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: result += "/";
  Confidence: band=high; score=0.74
- Line 392: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: result += "/";
- Line 510: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: zip_close(za);
- Line 565: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: zip_fclose(zf);
- Line 592: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: zip_fclose(zf);
- Line 593: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: out_file.close();
- Line 598: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: zip_close(za);
- Line 609: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: zip_close(za);
- Line 750: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 757: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 768: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ofs.close();
- Line 888: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: members.push_back(
  Confidence: band=high; score=0.74
- Line 889: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: members.push_back(
- Line 914: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: extracted_files.push_back(file_path);
  Confidence: band=high; score=0.74
- Line 915: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: extracted_files.push_back(file_path);

### src/content/audio_processor.cpp
Total findings: 21

- Line 0: severity=HIGH; category=uncategorized
  Context: ['        if (bitrate > 0) {', '            // Approximate: subtract ID3 header size from content length', '            size_t audio_bytes = (blob.size() > search_start) ? blob.size() - search_start : blob.size();', '            data.duration_ms   = static_cast<int64_t>(audio_bytes) * 8 * 1000 / (bitrate * 1000);', '        }']
  Confidence: band=high; score=0.78
- Line 145: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["duration_ms"]      = media.duration_ms;
- Line 146: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["audio_codec"]      = media.audio_codec;
- Line 147: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["bitrate_kbps"]     = media.bitrate_kbps;
- Line 148: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["sample_rate"]      = media.sample_rate;
- Line 149: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["channels"]         = media.channels;
- Line 150: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["container_format"] = media.container_format;
- Line 186: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["waveform"] = waveform_json;
- Line 217: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["transcription"] = trans_meta;
- Line 235: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: trans_meta["segments"]          = json::array(
- Line 237: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["transcription"] = trans_meta;
- Line 614: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: data.channels    = static_cast<int>(blob[base + 11]);
- Line 658: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: uint16_t cp = static_cast<uint16_t>(data[i]) | (static_cast<uint16_t>(data[i + 1]) << 8);
- Line 183: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: waveform_json.push_back(sample);
  Confidence: band=high; score=0.74
- Line 184: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: waveform_json.push_back(sample);
- Line 213: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: segments_json.push_back(seg_json);
  Confidence: band=high; score=0.74
- Line 214: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: segments_json.push_back(seg_json);
- Line 221: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 280: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks.push_back(chunk);
  Confidence: band=high; score=0.74
- Line 281: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: chunks.push_back(chunk);
- Line 287: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: current_chunk += " ";
  Confidence: band=high; score=0.74

### src/content/text_processor.cpp
Total findings: 21

- Line 325: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator start may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto start = sentence.find_first_not_of(" \t\n");
- Line 326: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator end may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto end   = sentence.find_last_not_of(" \t\n");
- Line 0: severity=HIGH; category=uncategorized
  Context: ['            // Distribute token influence across dimensions', '            for (int dim_offset = 0; dim_offset < 10; dim_offset++) {', '                int dim = (combined_hash + dim_offset * 73) % EMBEDDING_DIM;', '', '                // Add influence with varying weights based on position']
  Confidence: band=high; score=0.78
- Line 51: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["is_code"]  = true;
- Line 55: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["line_count"] = line_count;
- Line 57: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["is_code"] = false;
- Line 62: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["token_count"] = token_count;
- Line 72: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["detected_language"]     = lang.code;
- Line 73: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["language_name"]         = lang.name;
- Line 74: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["language_confidence"]   = lang.confidence;
- Line 75: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["language_routing_hint"] = LanguageDetector::routingHint(lang.code);
- Line 99: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: chunks.push_back(chunk);
- Line 151: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks.push_back(chunk);
  Confidence: band=high; score=0.74
- Line 152: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: chunks.push_back(chunk);
- Line 216: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back(token);
- Line 330: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: sentences.push_back(sentence);
  Confidence: band=high; score=0.74
- Line 331: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: sentences.push_back(sentence);
- Line 338: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: sentences.push_back(text);
- Line 362: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: words.push_back(std::move(w));
- Line 375: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: shingles.push_back(words[i] + " " + words[i + 1] + " " + words[i + 2]);
  Confidence: band=high; score=0.74
- Line 376: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: shingles.push_back(words[i] + " " + words[i + 1] + " " + words[i + 2]);

### src/content/content_manager_llm.cpp
Total findings: 20

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 235: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result["people"]        = json::array();
- Line 236: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result["places"]        = json::array();
- Line 237: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result["organizations"] = json::array();
- Line 382: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result["people"]        = json::array();
- Line 383: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result["places"]        = json::array();
- Line 384: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result["organizations"] = json::array();
- Line 74: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: prompt << "3. Sentiment (positive/negative/neutral)\n";
- Line 75: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: prompt << "4. Content category (article/technical/business/personal/other)\n\n";
- Line 80: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: std::string analysis = THEMIS_LLM_GENERATE(prompt.str());
  Confidence: band=high; score=0.74
- Line 127: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: std::string tags_text = THEMIS_LLM_GENERATE(prompt.str());
  Confidence: band=high; score=0.74
- Line 139: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 171: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: std::string summary = THEMIS_LLM_GENERATE(prompt.str());
  Confidence: band=high; score=0.74
- Line 177: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 209: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: std::string category = THEMIS_LLM_GENERATE(prompt.str());
  Confidence: band=high; score=0.74
- Line 220: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 260: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: std::string entities_text = THEMIS_LLM_GENERATE(prompt.str());
  Confidence: band=high; score=0.74
- Line 267: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 370: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tags.push_back(tag);

### src/content/tts_processor.cpp
Total findings: 20

- Line 251: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: auto *voice = new piper::PiperVoice();
- Line 274: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: static_cast = nullptr;
  Context: delete static_cast<piper::PiperVoice *>(tts_ctx_);
- Line 211: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: voices.push_back(voice1);
  Confidence: band=high; score=0.74
- Line 212: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: voices.push_back(voice1);
- Line 274: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: delete static_cast<piper::PiperVoice *>(tts_ctx_);
- Line 420: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: wav_data.push_back(file_size & 0xFF);
- Line 421: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: wav_data.push_back((file_size >> 8) & 0xFF);
- Line 422: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: wav_data.push_back((file_size >> 16) & 0xFF);
- Line 423: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: wav_data.push_back((file_size >> 24) & 0xFF);
- Line 431: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: wav_data.push_back(sample_rate & 0xFF);
- Line 432: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: wav_data.push_back((sample_rate >> 8) & 0xFF);
- Line 433: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: wav_data.push_back((sample_rate >> 16) & 0xFF);
- Line 434: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: wav_data.push_back((sample_rate >> 24) & 0xFF);
- Line 436: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: wav_data.push_back(byte_rate & 0xFF);
- Line 437: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: wav_data.push_back((byte_rate >> 8) & 0xFF);
- Line 438: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: wav_data.push_back((byte_rate >> 16) & 0xFF);
- Line 439: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: wav_data.push_back((byte_rate >> 24) & 0xFF);
- Line 446: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: wav_data.push_back(data_size & 0xFF);
- Line 447: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: wav_data.push_back((data_size >> 8) & 0xFF);
- Line 448: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: wav_data.push_back((data_size >> 16) & 0xFF);

### src/content/pipeline/multimodal_chunker.cpp
Total findings: 17

- Line 0: severity=CRITICAL; category=uncategorized
  Context: ['    }', '', '    const size_t expected_size = width * height * bytes_per_pixel;', '    if (data.size() != expected_size) {', '        // Size mismatch - fall back to generic chunking']
  Confidence: band=very_high; score=0.93
- Line 99: severity=HIGH; category=allocation_loop
  Description: Dynamic allocation in loop — high overhead
  Remediation: Pre-allocate outside loop or use object pool pattern
  Context: // Start new chunk with overlap
- Line 96: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks.push_back(std::move(chunk));
  Confidence: band=high; score=0.74
- Line 97: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: chunks.push_back(std::move(chunk));
- Line 119: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: chunks.push_back(std::move(chunk));
- Line 178: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks.push_back(std::move(chunk));
  Confidence: band=high; score=0.74
- Line 178: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks.push_back(std::move(chunk));
  Confidence: band=high; score=0.74
- Line 179: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: chunks.push_back(std::move(chunk));
- Line 210: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: boundaries.push_back(0);  // Start
- Line 222: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: boundaries.push_back(boundary);
  Confidence: band=high; score=0.74
- Line 223: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: boundaries.push_back(boundary);
- Line 229: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: boundaries.push_back(text.size());  // End
- Line 238: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: boundaries.push_back(0);  // Start
- Line 248: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: boundaries.push_back(boundary);
  Confidence: band=high; score=0.74
- Line 249: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: boundaries.push_back(boundary);
- Line 263: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: boundaries.push_back(boundary);
- Line 269: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: boundaries.push_back(text.size());  // End

### src/content/image_processor.cpp
Total findings: 16

- Line 545: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 64 > array 31
  Remediation: Fix loop condition or increase array size
  Context: double median = (sorted_freq[31] + sorted_freq[32]) / 2.0;
- Line 0: severity=HIGH; category=uncategorized
  Context: ['        for (size_t i = 2; i < blob.size() - 9; ++i) {', '            if (blob[i] == 0xFF && (blob[i + 1] == 0xC0 || blob[i + 1] == 0xC2)) {', '                height = (blob[i + 5] << 8) | blob[i + 6];', '                width = (blob[i + 7] << 8) | blob[i + 8];', '                return;']
  Confidence: band=high; score=0.78
- Line 0: severity=HIGH; category=uncategorized
  Context: ['            if (blob[i] == 0xFF && (blob[i + 1] == 0xC0 || blob[i + 1] == 0xC2)) {', '                height = (blob[i + 5] << 8) | blob[i + 6];', '                width = (blob[i + 7] << 8) | blob[i + 8];', '                return;', '            }']
  Confidence: band=high; score=0.78
- Line 176: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["width"] = width;
- Line 177: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["height"] = height;
- Line 178: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["aspect_ratio"] = height > 0 ? static_cast<double>(width) / height : 0;
- Line 179: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["megapixels"] = (width * height) / 1000000.0;
- Line 193: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["dominant_colors"] = color_array;
- Line 346: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: snprintf(hex, sizeof(hex), "#%02X%02X%02X", r, g, b);
  Confidence: band=very_high; score=0.9
- Line 117: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: bool ImageProcessor::canProcess(const std::string& mime_type) const {
  Confidence: band=high; score=0.74
- Line 185: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: color_array.push_back({
  Confidence: band=high; score=0.74
- Line 186: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: color_array.push_back({
- Line 267: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks.push_back(chunk);
  Confidence: band=high; score=0.74
- Line 268: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: chunks.push_back(chunk);
- Line 272: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: current_chunk += " ";
  Confidence: band=high; score=0.74
- Line 273: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: current_chunk += " ";

### src/content/pipeline/async_bulk_uploader.cpp
Total findings: 16

- Line 63: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: content_to_job_map_[metadata.content_id] = job_id;
- Line 115: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t i = 0; i < contents.size(); ++i) {
  Confidence: band=very_high; score=0.9
- Line 122: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: std::string job_id = worker_->submitBatch(files, "", nlohmann::json::object());
- Line 151: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& meta : metadata_list) {
  Confidence: band=very_high; score=0.9
- Line 96: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(result);
  Confidence: band=high; score=0.74
- Line 97: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(result);
- Line 107: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(result);
  Confidence: band=high; score=0.74
- Line 108: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(result);
- Line 116: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: files.push_back({metadata_list[i].content_id, blob});
  Confidence: band=high; score=0.74
- Line 117: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: files.push_back({metadata_list[i].content_id, blob});
- Line 137: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(result);
  Confidence: band=high; score=0.74
- Line 137: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(result);
  Confidence: band=high; score=0.74
- Line 138: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(result);
- Line 155: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(result);
  Confidence: band=high; score=0.74
- Line 156: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(result);
- Line 225: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: using themis::content::IngestionJobStatus;

### src/content/markdown_processor.cpp
Total findings: 15

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=MEDIUM; category=uncategorized
  Context: ['            size_t combined = token_hash ^ (i * 31) ^ (static_cast<size_t>(seed) * 97);', '            for (int d = 0; d < 10; ++d) {', '                int dim = static_cast<int>((combined + static_cast<size_t>(d) * 73) % DIM);', '                float weight = 1.0f / (1.0f + static_cast<float>(i) * 0.1f);', '                float phase  = static_cast<float>((combined + static_cast<size_t>(dim)) % 360)']
  Confidence: band=medium; score=0.62
- Line 126: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: arr.push_back(ti);
- Line 246: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: for (char c : tl) row += (c == '|') ? ' ' : c;
  Confidence: band=high; score=0.74
- Line 440: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (consecutive_nl <= 2) result += '\n';
- Line 447: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (!last_was_space && !last_was_newline) result += ' ';
- Line 544: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (!para.empty()) { paragraphs.push_back(para); para.clear(); }
- Line 547: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (!para.empty()) { paragraphs.push_back(para); para.clear(); }
- Line 553: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (!para.empty()) paragraphs.push_back(para);
- Line 584: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: while (iss >> tok) tokens.push_back(tok);
  Confidence: band=high; score=0.74
- Line 585: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: while (iss >> tok) tokens.push_back(tok);
- Line 589: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (!overlap_text.empty()) overlap_text += ' ';
- Line 615: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: while (iss >> token) tokens.push_back(token);

### src/content/content_fs.cpp
Total findings: 14

- Line 231: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: uint64_t part_end   = (i == end_idx) ? (end - chunk_off) : static_cast<uint64_t>(part->size());
- Line 233: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: part_end = static_cast<uint64_t>(part->size());
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 36: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: oss << std::setw(2) << static_cast<int>(data[i]);
- Line 97: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Ensure legacy blob key is removed to avoid confusion
  Confidence: band=high; score=0.8
- Line 208: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: out.insert(out.end(), blob->begin() + static_cast<ptrdiff_t>(offset),
- Line 209: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: blob->begin() + static_cast<ptrdiff_t>(end));
- Line 236: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: out.insert(out.end(), part->begin() + static_cast<ptrdiff_t>(part_start),
- Line 237: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: part->begin() + static_cast<ptrdiff_t>(part_end));
- Line 113: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 171: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 244: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 270: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 284: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/content/cad_processor.cpp
Total findings: 13

- Line 144: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["part_count"] = cad.part_count;
- Line 145: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["units"]      = default_units_;
- Line 148: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["bounding_box"]
- Line 156: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["dimensions"] = {dx, dy, dz};
- Line 249: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: chunk.metadata["part_id"] = cad.part_ids[i];
- Line 380: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (minX != std::numeric_limits<double>::max()) {
  Confidence: band=very_high; score=0.9
- Line 91: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: bool CADProcessor::canProcess(const std::string &mime_type) const {
  Confidence: band=high; score=0.74
- Line 133: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: } else if (mime_type == "image/vnd.dxf" || header.find("0\nSECTION") != std::string::npos) {
- Line 133: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: } else if (mime_type == "image/vnd.dxf" || header.find("0\nSECTION") != std::string::npos) {
- Line 250: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks.push_back(chunk);
  Confidence: band=high; score=0.74
- Line 251: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: chunks.push_back(chunk);
- Line 302: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: data.part_ids.push_back("PART_" + std::to_string(i + 1));
  Confidence: band=high; score=0.74
- Line 303: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: data.part_ids.push_back("PART_" + std::to_string(i + 1));

### src/content/content_metrics.cpp
Total findings: 10

- Line 285: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(error_mutex_);
- Line 429: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [mime, count] : mime_type_counts_) {
  Confidence: band=very_high; score=0.9
- Line 442: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [category, count] : error_category_counts_) {
  Confidence: band=very_high; score=0.9
- Line 500: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(mime_mutex_);
- Line 504: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(error_mutex_);
- Line 508: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(error_category_mutex_);
- Line 512: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(violation_mutex_);
- Line 117: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, double> ContentMetrics::getLatencyPercentiles(const std::string& operation) const {
  Confidence: band=high; score=0.74
- Line 119: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, double> result;
  Confidence: band=high; score=0.74
- Line 385: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: oss << "# HELP content_extract_errors_total Total PDF/document extraction errors\n";

### src/content/ocr_processor.cpp
Total findings: 10

- Line 314: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.metadata["ocr_binarized"]    = preprocess_info.binarized;
- Line 0: severity=MEDIUM; category=uncategorized
  Context: ['            size_t combined = token_hash ^ (i * 31) ^ (static_cast<size_t>(seed) * 97);', '            for (int d = 0; d < 10; ++d) {', '                int dim      = static_cast<int>((combined + static_cast<size_t>(d) * 73) % DIM);', '                float weight = 1.0f / (1.0f + static_cast<float>(i) * 0.1f);', '                float phase = static_cast<float>((combined + static_cast<size_t>(dim)) % 360) * 3.14159265359f / 180.0f;']
  Confidence: band=medium; score=0.62
- Line 371: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: sentences.push_back(current);
  Confidence: band=high; score=0.74
- Line 372: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: sentences.push_back(current);
- Line 377: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: sentences.push_back(current);
- Line 393: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks.push_back(std::move(ch));
  Confidence: band=high; score=0.74
- Line 394: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: chunks.push_back(std::move(ch));
- Line 400: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: current_chunk += " ";
  Confidence: band=high; score=0.74
- Line 431: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: chunks.push_back(std::move(ch));
- Line 458: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back(token);

### src/content/version_manager.cpp
Total findings: 8

- Line 147: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = versions_.find(content_id);
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 47: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: lines.push_back(line);
  Confidence: band=high; score=0.74
- Line 48: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: lines.push_back(line);
- Line 122: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: versions_[content_id].push_back(version);

### src/content/content_security.cpp
Total findings: 7

- Line 357: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto scan_result = malware_filter_->scan(data, filename, mime_type, content_id);
- Line 403: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> seen_types;
  Confidence: band=medium; score=0.66
- Line 406: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.pii_types.push_back(type_str);
  Confidence: band=high; score=0.74
- Line 407: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.pii_types.push_back(type_str);
- Line 528: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: static const std::regex unix_path_regex(R"((/(?:[a-zA-Z_][a-zA-Z0-9_\-]*)(?:/[a-zA-Z0-9_\-.]+)+))");
- Line 529: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: static const std::regex windows_path_regex(R"([A-Z]:\\([a-zA-Z0-9_\-]+\\)+[a-zA-Z0-9_\-./]*)");
- Line 530: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: static const std::regex home_path_regex(R"(~/[a-zA-Z0-9_\-./]+)");

### src/content/content_type.cpp
Total findings: 7

- Line 89: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: type.supports_metadata_extraction = j["supports_metadata_extraction"];
- Line 217: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (size_t i = 0; i < std::min(blob.size(), size_t(1000)); i++) {
- Line 233: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(&type);
  Confidence: band=high; score=0.74
- Line 233: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(&type);
  Confidence: band=high; score=0.74
- Line 234: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(&type);
- Line 242: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(&type);
  Confidence: band=high; score=0.74
- Line 243: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(&type);

### src/content/abuse_detector.cpp
Total findings: 4

- Line 186: severity=HIGH; category=performance; pattern=regex_in_loop
  Description: std::regex compiled in loop (compile once, reuse)
  Context: for (const auto& f : node["flags"]) {
  Confidence: band=very_high; score=0.9
- Line 198: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: patterns.push_back(std::move(p));
  Confidence: band=high; score=0.74
- Line 199: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: patterns.push_back(std::move(p));
- Line 200: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: error += "Skipping pattern '" + name + "': " + re.what() + "; ";
  Confidence: band=high; score=0.74

### src/content/adapters/format_extractor_factory.cpp
Total findings: 4

- Line 39: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (it != impl_->registry.end()) {
- Line 50: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto &mime : extractor->supportedMimeTypes()) {
  Confidence: band=very_high; score=0.9
- Line 59: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(mime);
  Confidence: band=high; score=0.74
- Line 60: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(mime);

### src/content/content_validator.cpp
Total findings: 4

- Line 300: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: normalised.push_back(c == '\\' ? '/' : c);
  Confidence: band=high; score=0.74
- Line 301: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: normalised.push_back(c == '\\' ? '/' : c);
- Line 301: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: normalised.push_back(c == '\\' ? '/' : c);
- Line 301: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: normalised.push_back(c == '\\' ? '/' : c);

### src/content/language_detector.cpp
Total findings: 4

- Line 201: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: for (const char *const *ind = kProfiles[p].indicators; *ind != nullptr; ++ind) {
- Line 205: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: while ((pos = lower.find(pattern, pos)) != std::string::npos) {
  Confidence: band=very_high; score=0.9
- Line 113: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: lower += ' '; // sentinel: ensure first word gets a leading space
  Confidence: band=high; score=0.74
- Line 117: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: lower += ' '; // sentinel: ensure last word gets a trailing space
  Confidence: band=high; score=0.74

### src/content/pipeline/bulk_upload_interface.cpp
Total findings: 4

- Line 68: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(result);
  Confidence: band=high; score=0.74
- Line 69: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(result);
- Line 75: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(upload(contents[i], metadata_list[i]));
  Confidence: band=high; score=0.74
- Line 76: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(upload(contents[i], metadata_list[i]));

### src/content/deduplication_checker.cpp
Total findings: 2

- Line 159: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto val = band_cache_->get(makeBandKey(b, bh));
- Line 120: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: storage_->scanPrefix(kPrefix, [&](std::string_view key, std::string_view value) -> bool {

### src/content/embedding_pipeline.cpp
Total findings: 2

- Line 82: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: auto status = future.wait_for(std::chrono::milliseconds(config_.timeout_ms));
- Line 105: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/content/pipeline/content_chunker.cpp
Total findings: 2

- Line 57: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks.push_back(std::move(chunk));
  Confidence: band=high; score=0.74
- Line 58: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: chunks.push_back(std::move(chunk));

### src/content/mock_clip_processor.cpp
Total findings: 1

- Line 23: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: res.metadata["mime_type"]           = content_type.mime_type;

### src/content/pipeline/zstd_compression.cpp
Total findings: 1

- Line 52: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // This maintains compatibility with standard decompress()
  Confidence: band=high; score=0.8

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
