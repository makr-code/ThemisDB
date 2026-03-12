### Context

This issue implements the roadmap item 'Content Deduplication via Perceptual Hashing' for the content domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.8.0.

Primary detail section: Content Deduplication via Perceptual Hashing

### Goal

Deliver the scoped changes for Content Deduplication via Perceptual Hashing in src/content/ and complete the linked detail section in a release-ready state for v1.8.0.

### Detailed Scope

### Content Deduplication via Perceptual Hashing
**Priority:** Medium
**Target Version:** v1.8.0

Exact duplicate detection (SHA-256 of raw bytes) is already performed in `content_manager.cpp`. Add near-duplicate detection using perceptual hashing (pHash for images, MinHash for text documents) to reject semantically identical content before storage.

**Implementation Notes:**
- `[x]` Images: compute pHash (DCT-based 64-bit hash) in `image_processor.cpp` using a pure C++ implementation (no OpenCV dependency); store hash in content metadata as `phash_hex`.
- `[x]` Text documents: compute MinHash signature (128 hash functions, Jaccard threshold 0.85) in `text_processor.cpp`; use a band LSH index stored in `cache::BoundedLRUCache` for fast lookup.
- `[x]` `ContentManager::ingest()` calls `DeduplicationChecker::isDuplicate(content_id, phash_or_minhash)` before committing; returns `DuplicateOf{existing_id}` if a near-duplicate is found.
- `[x]` Deduplication is opt-in per collection via `ContentPolicy` in `content_policy.cpp`; default off.
- `[x]` Expose `content_dedup_hits_total` and `content_dedup_checks_total` Prometheus counters.

**Performance Targets:**
- pHash computation for a 4 MP JPEG in < 5 ms.
- MinHash + LSH lookup for a 10 KB text document in < 1 ms (with warm band index of 100K entries).
- Near-duplicate detection adds < 10% overhead to total ingestion latency when deduplication is enabled.

---

### Acceptance Criteria

- [ ] Images: compute pHash (DCT-based 64-bit hash) in `image_processor.cpp` using a pure C++ implementation (no OpenCV dependency); store hash in content metadata as `phash_hex`.
- [ ] Text documents: compute MinHash signature (128 hash functions, Jaccard threshold 0.85) in `text_processor.cpp`; use a band LSH index stored in `cache::BoundedLRUCache` for fast lookup.
- [ ] `ContentManager::ingest()` calls `DeduplicationChecker::isDuplicate(content_id, phash_or_minhash)` before committing; returns `DuplicateOf{existing_id}` if a near-duplicate is found.
- [ ] Deduplication is opt-in per collection via `ContentPolicy` in `content_policy.cpp`; default off.
- [ ] Expose `content_dedup_hits_total` and `content_dedup_checks_total` Prometheus counters.
- [ ] pHash computation for a 4 MP JPEG in < 5 ms.
- [ ] MinHash + LSH lookup for a 10 KB text document in < 1 ms (with warm band index of 100K entries).
- [ ] Near-duplicate detection adds < 10% overhead to total ingestion latency when deduplication is enabled.

### Relationships

- Roadmap row: #168 (🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/content/FUTURE_ENHANCEMENTS.md#content-deduplication-via-perceptual-hashing
- Source key: roadmap:168:content:v1.8.0:content-deduplication-via-perceptual-hashing

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:168:content:v1.8.0:content-deduplication-via-perceptual-hashing -->
<!-- roadmap-ref: row=168;module=content;target=v1.8.0 -->
<!-- roadmap-detail: src/content/FUTURE_ENHANCEMENTS.md#content-deduplication-via-perceptual-hashing -->
