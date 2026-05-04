# HuggingFace Ingestion Plugin – Roadmap

## Current Status

**Status:** ✅ Ready for use

Entry-point: `plugins/huggingface/plugin.json` · implementation: `src/plugins/huggingface_ingestion_plugin.cpp` · public header: `include/plugins/huggingface_ingestion_plugin.h`

| Feature | Status |
|---------|--------|
| REST API integration | ✅ Implemented |
| Streaming support | ✅ Implemented |
| Local file cache | ✅ Implemented |
| Rate limiting | ✅ Implemented |
| Retry / backoff | ✅ Implemented |

---

## In Progress

- [~] CI test fetching a small public HuggingFace dataset in a sandboxed environment
- [~] Documentation of all `plugin.json` configuration fields with types and defaults

## Planned Features

- [ ] HuggingFace token authentication for private/gated datasets (Target: Q3 2026)
- [ ] Resume / checkpoint: restart interrupted ingestions from last saved offset (Target: Q3 2026)
- [ ] Dataset subsets (config parameter) and column selection (Target: Q3 2026)
- [ ] Prometheus per-batch ingestion metrics (rows/sec, cache hit rate) (Target: Q3 2026)
- [ ] HuggingFace Model Hub support – download and register model weights (Target: Q4 2026)
- [ ] Incremental sync – detect new dataset versions and ingest only diffs (Target: Q4 2026)
- [ ] Auto-generate ThemisDB schema from HuggingFace dataset features description (Target: Q4 2026)
- [ ] Multi-dataset parallel ingestion with priority queue (Target: 2027)

---

## Short-term Goals (next 1–2 sprints)

- [ ] Add CI test that fetches a small public HuggingFace dataset (e.g., `imdb` split `test[:10]`) in a sandboxed environment.
- [ ] Document all `plugin.json` configuration fields with types, defaults and valid ranges.
- [ ] Verify rate-limiter behaviour under concurrent ingestion workers.

## Mid-term Goals (1–3 months)

- [ ] Support **HuggingFace token authentication** for private/gated datasets.
- [ ] Add **resume / checkpoint** support: restart interrupted ingestions from last saved offset.
- [ ] Support **dataset subsets** (HuggingFace config parameter) and **column selection**.
- [ ] Emit per-batch ingestion metrics (rows/sec, cache hit rate) to Prometheus.

## Long-term Goals (3–12 months)

- [ ] Support **HuggingFace Model Hub** – download and register model weights (GGUF, safetensors).
- [ ] **Incremental sync** – detect new dataset versions and ingest only diffs.
- [ ] Auto-generate ThemisDB schema from HuggingFace dataset features description.
- [ ] Multi-dataset parallel ingestion with priority queue.

## Milestones

| Milestone | Target | Status |
|-----------|--------|--------|
| Auth support for private datasets | TODO | 🔲 Planned |
| Resume / checkpoint | TODO | 🔲 Planned |
| Model Hub support | TODO | 🔲 Planned |

## Implementation Phases

### Phase 1 – Private Dataset Auth & Resume / Checkpoint
- [ ] Implement HuggingFace token authentication (Bearer header, token stored in ThemisDB secrets)
- [ ] Persist ingestion offset to ThemisDB; resume from last saved shard + row on restart
- [ ] Support `config` (subset) parameter and `columns` filter in `plugin.json`
- [ ] Integration test: private dataset fetch with mock token server

### Phase 2 – Model Hub & Incremental Sync
- [ ] Model Hub: download GGUF / safetensors artifacts and register in ThemisDB model registry
- [ ] Incremental sync: compare local dataset commit hash against HuggingFace API; ingest diffs only
- [ ] Prometheus metrics: rows/sec, cache hit rate, download progress

### Phase 3 – Auto-Schema & Multi-Dataset Parallel
- [ ] Parse HuggingFace dataset `features` JSON and generate a ThemisDB collection schema automatically
- [ ] Multi-dataset parallel ingestion with configurable concurrency and priority queue
- [ ] Rate-limiter stress test under concurrent workers
- [ ] Documentation: all `plugin.json` fields, environment variables, error codes

---

## Dependencies

- `libcurl` (already in ThemisDB)
- `nlohmann/json` (already in ThemisDB)
- ThemisDB `AsyncIngestionWorker` (`include/content/async_ingestion_worker.h`)
- ThemisDB `ContentManager`

## Open Questions

- [ ] Should authentication tokens be stored in ThemisDB secrets store or config file?

---

## Production Readiness Checklist

| Item | Status |
|------|--------|
| REST API integration | ✅ Ready |
| Streaming support | ✅ Ready |
| Local file cache | ✅ Ready |
| Rate limiting | ✅ Ready |
| Retry / backoff | ✅ Ready |
| Private / gated dataset authentication | ❌ Not implemented |
| Resume / checkpoint on interrupted ingestion | ❌ Not implemented |
| Model Hub support | ❌ Not implemented |
| Rate-limiter tested under concurrent workers | ❌ Pending |
| Exact implementation source file confirmed | ✅ Ready |

## Known Issues & Limitations

- Private and gated datasets are not supported; requests return 401 with no recovery path
- Rate-limiter has not been tested under concurrent ingestion workers; correctness unverified
- No resume/checkpoint: a crashed ingestion restarts from the beginning

---

*See also: [`FUTURE_ENHANCEMENTS.md`](FUTURE_ENHANCEMENTS.md) · [Full docs](../../docs/plugins/HUGGINGFACE_INGESTION.md)*
