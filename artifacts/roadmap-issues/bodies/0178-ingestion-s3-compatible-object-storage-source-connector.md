### Context

This issue implements the roadmap item 'S3-Compatible Object Storage Source Connector' for the ingestion domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.7.0.

Primary detail section: S3-Compatible Object Storage Source Connector

### Goal

Deliver the scoped changes for S3-Compatible Object Storage Source Connector in src/ingestion/ and complete the linked detail section in a release-ready state for v1.7.0.

### Detailed Scope

### S3-Compatible Object Storage Source Connector
**Priority:** Medium
**Target Version:** v1.7.0

Add an `S3Connector` that lists and downloads objects from an S3-compatible bucket (AWS S3, MinIO, GCS via S3 interop) and ingests them as documents. This supports batch ingestion of data lake files without requiring a local copy.

**Implementation Notes:**
- Add `s3_connector.cpp` implementing `ISourceConnector`; use the AWS C++ SDK (`aws-sdk-cpp`) or, for lighter-weight builds, a minimal S3 client built on the `HttpClient` class introduced above.
- Incremental mode: checkpoint the last processed `LastModified` timestamp or lexicographic key prefix; on restart, use `ListObjectsV2` with a `StartAfter` marker.
- Support all flat-file formats handled by the Importers module flat-file path (`.jsonl`, `.csv`, `.parquet`) by delegating parsing to `FileSystemIngester`'s format readers.
- Configurable `max_keys_per_list` (default 1 000) and `max_concurrent_downloads` (default 4) to balance throughput against memory pressure.

**Performance Targets:**
- S3 object listing overhead ≤ 100 ms per 1 000 objects (single `ListObjectsV2` call).
- Concurrent download throughput ≥ 200 MB/s aggregate with 4 parallel downloads on a 10 Gbps network.

---

### Acceptance Criteria

- [ ] Add `s3_connector.cpp` implementing `ISourceConnector`; use the AWS C++ SDK (`aws-sdk-cpp`) or, for lighter-weight builds, a minimal S3 client built on the `HttpClient` class introduced above.
- [ ] Incremental mode: checkpoint the last processed `LastModified` timestamp or lexicographic key prefix; on restart, use `ListObjectsV2` with a `StartAfter` marker.
- [ ] Support all flat-file formats handled by the Importers module flat-file path (`.jsonl`, `.csv`, `.parquet`) by delegating parsing to `FileSystemIngester`'s format readers.
- [ ] Configurable `max_keys_per_list` (default 1 000) and `max_concurrent_downloads` (default 4) to balance throughput against memory pressure.
- [ ] S3 object listing overhead ≤ 100 ms per 1 000 objects (single `ListObjectsV2` call).
- [ ] Concurrent download throughput ≥ 200 MB/s aggregate with 4 parallel downloads on a 10 Gbps network.

### Relationships

- Roadmap row: #178 (🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/ingestion/FUTURE_ENHANCEMENTS.md#s3-compatible-object-storage-source-connector
- Source key: roadmap:178:ingestion:v1.7.0:s3-compatible-object-storage-source-connector

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:178:ingestion:v1.7.0:s3-compatible-object-storage-source-connector -->
<!-- roadmap-ref: row=178;module=ingestion;target=v1.7.0 -->
<!-- roadmap-detail: src/ingestion/FUTURE_ENHANCEMENTS.md#s3-compatible-object-storage-source-connector -->
