# Blob Storage Plugin – Roadmap

## Current Status

| Backend | Status |
|---------|--------|
| Azure Blob Storage | ✅ Production-ready |
| Amazon S3 | ✅ Production-ready |

Entry-points: `plugins/blob_storage/azure/` · `plugins/blob_storage/s3/`

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

## Dependencies

- `libcurl` / Azure SDK / AWS SDK (already in ThemisDB vcpkg)
- ThemisDB `PluginManager` (`include/plugins/plugin_manager.h`)
- Plugin manifest signing infrastructure

## Open Questions

- [ ] Should blob metadata be stored in ThemisDB's AQL layer or externally?
- [ ] Which CDN/edge-caching strategy should be recommended?

---

*See also: [`future_enhancements.md`](future_enhancements.md)*
