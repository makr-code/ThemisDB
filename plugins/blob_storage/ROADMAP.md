# Blob Storage Plugin – Roadmap

## Current Status

| Backend | Status |
|---------|--------|
| Azure Blob Storage | ✅ Production-ready |
| Amazon S3 | ✅ Production-ready |
| Google Cloud Storage (GCS) | ✅ Implemented; integration validation pending |

Entry-points: `plugins/blob_storage/azure/` · `plugins/blob_storage/s3/` · `plugins/blob_storage/gcs/`

---

## In Progress

- [~] Integration tests for Azure, S3, and GCS backends running against local emulators (Azurite / MinIO / fake-gcs-server)
- [~] Per-backend health-check endpoint via ThemisDB metrics API

## Planned Features

- [x] **Google Cloud Storage (GCS)** backend (✅ Q3 2026)
- [x] Server-side encryption configuration per backend (✅ Q3 2026)
- [x] Presigned URL generation for direct client uploads/downloads (✅ Q3 2026)
- [ ] Retry policy configuration (max retries, backoff strategy) (Target: Q3 2026)
- [ ] Multi-region replication strategy across backends (Target: Q4 2026)
- [ ] Lifecycle / tiering policies for cold data (Target: Q4 2026)
- [ ] Unified blob-storage abstraction layer (Target: Q4 2026)

---

## Short-term Goals (next 1–2 sprints)

- [~] Add env-gated integration tests for Azure, S3, and GCS backends running against local emulators (Azurite / MinIO / fake-gcs-server).
- [ ] Expose per-backend health-check endpoint via ThemisDB metrics API.
- [ ] Verify and document `plugin.json.sig` signature workflow for Azure, S3, and GCS manifests.

## Mid-term Goals (1–3 months)

- [~] Harden the existing **Google Cloud Storage (GCS)** backend with emulator/nightly validation and build-governance parity.
- [~] Harden server-side encryption coverage per backend (contract tests + emulator validation).
- [~] Harden presigned URL generation for direct client uploads/downloads across all cloud backends.
- [ ] Introduce retry policy configuration (max retries, backoff strategy).

## Long-term Goals (3–12 months)

- [ ] Multi-region replication strategy across backends.
- [ ] Lifecycle / tiering policies (move cold data to cheaper storage classes).
- [ ] Unified blob-storage abstraction layer usable by all plugin types.
- [ ] Performance benchmarks and throughput targets documented.

## Milestones

| Milestone | Target | Status |
|-----------|--------|--------|
| GCS backend MVP | Q3 2026 | ✅ Complete |
| Full encryption support | Q3 2026 | ✅ Complete |
| Benchmark suite | TODO | 🔲 Planned |

## Implementation Phases

### Phase 1 – GCS Backend MVP
- [x] Implement `GCSBlobBackend` using Google Cloud Storage C++ client library
- [x] Add `plugin.json` and `plugin.json.sig` for the GCS backend
- [x] Unit tests: upload, download, delete, list objects (GCS-01..08)

### Phase 2 – Encryption & Presigned URLs
- [x] Server-side encryption configuration (SSE-S3, SSE-KMS, CSEK for GCS)
- [x] Presigned URL generation with configurable expiry per backend (Azure, S3, GCS)
- [~] Integration tests with local emulators (Azurite, MinIO, fake-gcs-server)

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
| GCS backend | ✅ Implemented; emulator/nightly validation pending |
| Server-side encryption configuration | ✅ Implemented; emulator/nightly validation pending |
| Presigned URL generation | ✅ Implemented; backend-specific hardening in progress |
| Retry policy documented and configurable | ❌ Not implemented |
| Integration tests running in CI | ⚠️ Env-gated focused tests added; CI wiring pending |
| `plugin.json.sig` signing workflow documented | ❌ Pending |

## Known Issues & Limitations

- Emulator-backed integration coverage is env-gated today; CI/nightly wiring is still pending
- Multi-region routing/replication is not yet wired through a dedicated blob router
- Retry/backoff parameters are not user-configurable at runtime

---

*See also: [`FUTURE_ENHANCEMENTS.md`](FUTURE_ENHANCEMENTS.md)*
