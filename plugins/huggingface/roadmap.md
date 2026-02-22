# HuggingFace Ingestion Plugin – Roadmap

## Current Status

**Status:** ✅ Ready for use

Entry-point: `plugins/huggingface/plugin.json` · implementation header: `include/plugins/huggingface_ingestion_plugin.h`

> TODO: Confirm exact implementation path (search `src/` for `huggingface_ingestion_plugin.cpp`).

| Feature | Status |
|---------|--------|
| REST API integration | ✅ Implemented |
| Streaming support | ✅ Implemented |
| Local file cache | ✅ Implemented |
| Rate limiting | ✅ Implemented |
| Retry / backoff | ✅ Implemented |

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

## Dependencies

- `libcurl` (already in ThemisDB)
- `nlohmann/json` (already in ThemisDB)
- ThemisDB `AsyncIngestionWorker` (`include/content/async_ingestion_worker.h`)
- ThemisDB `ContentManager`

## Open Questions

- [ ] Where exactly is the implementation source file (`*.cpp`)? – TODO: locate in `src/`
- [ ] Should authentication tokens be stored in ThemisDB secrets store or config file?

---

*See also: [`future_enhancements.md`](future_enhancements.md) · [Full docs](../../docs/plugins/HUGGINGFACE_INGESTION.md)*
