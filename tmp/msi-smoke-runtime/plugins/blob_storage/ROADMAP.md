# Blob Storage Plugin – Roadmap

## Current Status

| Backend | Status |
|---------|--------|
| Azure Blob Storage | ✅ Production-ready |
| Amazon S3 | ✅ Production-ready |

Entry-points: `plugins/blob_storage/azure/` · `plugins/blob_storage/s3/`

---

## In Progress

- [~] Integration tests for Azure and S3 backends running against local emulators (Azurite / MinIO)
- [~] Per-backend health-check endpoint via ThemisDB metrics API

## Planned Features

- [ ] **Google Cloud Storage (GCS)** backend (Target: Q3 2026)
- [ ] Server-side encryption configuration per backend (Target: Q3 2026)
- [ ] Presigned URL generation for direct client uploads/downloads (Target: Q3 2026)
- [ ] Retry policy configuration (max retries, backoff strategy) (Target: Q3 2026)
- [ ] Multi-region replication strategy across backends (Target: Q4 2026)
- [ ] Lifecycle / tiering policies for cold data (Target: Q4 2026)
- [ ] Unified blob-storage abstraction layer (Target: Q4 2026)

---

## Short-term Goals (next 1–2 sprints)

- [ ] Add integration tests for Azure and S3 backends running against local emulators (Azurite / MinIO).
- [ ] Expose per-backend health-check endpoint via ThemisDB metrics API.
- [ ] Verify and document `plugin.json.sig` signature workflow for both backends.

## Mid-term Goals (1–3 months)

- [ ] Add **Google Cloud Storage (GCS)** backend.
- [ ] Implement server-side encryption configuration per backend.
- [ ] Support presigned URL generation for direct client uploads/downloads.
- [ ] Introduce retry policy configuration (max retries, backoff strategy).

## Long-term Goals (3–12 months)

- [ ] Multi-region replication strategy across backends.
- [ ] Lifecycle / tiering policies (move cold data to cheaper storage classes).
- [ ] Unified blob-storage abstraction layer usable by all plugin types.
- [ ] Performance benchmarks and throughput targets documented.

## Milestones

| Milestone | Target | Status |
|-----------|--------|--------|
| GCS backend MVP | TODO | 🔲 Planned |
| Full encryption support | TODO | 🔲 Planned |
| Benchmark suite | TODO | 🔲 Planned |

## Implementation Phases

### Phase 1 – GCS Backend MVP
- [ ] Implement `GCSBlobBackend` using Google Cloud Storage C++ client library
- [ ] Add `plugin.json` and `plugin.json.sig` for the GCS backend
- [ ] Unit tests: upload, download, delete, list objects

### Phase 2 – Encryption & Presigned URLs
- [ ] Server-side encryption configuration (SSE-S3, SSE-KMS, customer-managed keys)
- [ ] Presigned URL generation with configurable expiry per backend
- [ ] Integration tests with local emulators (Azurite, MinIO, fake-gcs-server)

### Phase 3 – Multi-Region & Tiering
- [ ] Multi-region replication configuration and failover logic
- [ ] Lifecycle / tiering policy API (hot → cool → archive)
- [ ] Retry policy: max retries, exponential backoff, jitter

### Phase 4 – Unified Abstraction & Benchmarks
- [ ] Unified `IBlobStorageBackend` interface consumed by all plugin types
- [ ] Performance benchmark suite: throughput (MB/s) and latency (p50/p99) documented
- [ ] End-to-end CI pipeline with all three backends

---

## Dependencies

- `libcurl` / Azure SDK / AWS SDK (already in ThemisDB vcpkg)
- ThemisDB `PluginManager` (`include/plugins/plugin_manager.h`)
- Plugin manifest signing infrastructure

## Open Questions

- [ ] Should blob metadata be stored in ThemisDB's AQL layer or externally?
- [ ] Which CDN/edge-caching strategy should be recommended?

---

## Production Readiness Checklist

| Item | Status |
|------|--------|
| Azure Blob Storage backend | ✅ Ready |
| Amazon S3 backend | ✅ Ready |
| GCS backend | ❌ Not implemented |
| Server-side encryption configuration | ❌ Not implemented |
| Presigned URL generation | ❌ Not implemented |
| Retry policy documented and configurable | ❌ Not implemented |
| Integration tests running in CI | ❌ Pending |
| `plugin.json.sig` signing workflow documented | ❌ Pending |

## Known Issues & Limitations

- No integration tests running in CI yet (Azurite / MinIO emulators not wired up)
- Presigned URL generation is not implemented for any backend
- GCS backend is completely absent; any GCS writes currently unsupported
- Retry/backoff parameters are not user-configurable at runtime

---

*See also: [`FUTURE_ENHANCEMENTS.md`](FUTURE_ENHANCEMENTS.md)*
