# HuggingFace Ingestion Plugin – Future Enhancements

> **Note:** Items here are *not* on the active roadmap. They are ideas and backlog entries for later consideration. See [`ROADMAP.md`](ROADMAP.md) for committed near-term work.

---

## Scope

- HuggingFace Hub integration enhancements: new dataset formats (Parquet, Arrow), transformation pipelines (tokenisation, embedding, language filtering), scheduled sync, and quality filtering.
- Entry-points: `plugin.json`; implementation in `src/`; REST API + streaming + local disk cache + rate limiting already in place.
- Out of scope: changes to ThemisDB storage engine; this plugin only handles ingest from HuggingFace to ThemisDB collections.
- Covers additional dataset sources (Kaggle, OpenML, Zenodo) as secondary priority after HuggingFace parity.

## Design Constraints

- [ ] HuggingFace API tokens MUST be read exclusively from environment variables; never from config files or request parameters.
- [ ] Streaming ingestion MUST NOT buffer more than 64 MB per dataset shard in process memory.
- [ ] Rate limiter MUST honour HuggingFace Hub API limits (default 300 req/min); configurable per deployment.
- [ ] Local disk cache MUST use content-addressable storage (SHA-256 of shard content); cache entries expire after configurable TTL (default 7 days).
- [ ] Dataset schema changes between sync runs MUST be detected and reported as `SCHEMA_DRIFT` events before continuing ingestion.
- [ ] All transformation steps (tokenise, embed, filter) MUST be individually toggleable via plugin configuration.

## Required Interfaces

| Interface | Consumer | Notes |
|---|---|---|
| `IDatasetIngestor` | `HuggingFacePlugin`, ThemisDB core | `stream_shard()`, `get_schema()`, `estimate_size()` |
| `ITransformationPipeline` | `HuggingFacePlugin` | Ordered chain of `ITransformStep`; tokenise, embed, filter |
| `ILocalShardCache` | `HuggingFacePlugin` | Content-addressed shard cache; `get(sha256)`, `put(sha256, data)` |
| `ISyncScheduler` | `HuggingFacePlugin` | Cron-style scheduled ingestion; emits `SyncStarted`/`SyncCompleted` events |
| `IQualityFilter` | `ITransformationPipeline` | Returns pass/fail + quality score per sample |

## Idea Backlog

### Dataset Sources

- [ ] **Kaggle Datasets** – ingestion plugin for the Kaggle data-science platform.
- [ ] **OpenML** – ingest tabular datasets from OpenML.
- [ ] **Zenodo / OSF** – academic dataset repositories.
- [ ] **Common Crawl** – streaming ingestion from Common Crawl S3 buckets.

### Preprocessing / Transformation

- [ ] **On-the-fly tokenisation** – tokenise text fields during ingestion (using llama.cpp tokeniser).
- [ ] **Embedding generation** – compute vector embeddings inline during ingestion via image-analysis or LLM plugin.
- [ ] **Language detection and filtering** – skip records in unwanted languages.
- [ ] **PII scrubbing** – redact personally identifiable information before storage.

### Scheduling & Orchestration

- [ ] **Cron-based sync** – automatically re-ingest datasets on a schedule.
- [ ] **Dataset version pinning** – lock a specific commit hash from HuggingFace Hub.
- [ ] **Webhook trigger** – start ingestion when HuggingFace dataset is updated.

### Quality

- [ ] **Schema drift detection** – alert when upstream dataset schema changes.
- [ ] **Data quality metrics** – null rates, outlier detection, class imbalance checks.

---

## Test Strategy

- Unit tests for `IDatasetIngestor` using a mock HuggingFace REST server (WireMock or equivalent); assert correct shard parsing for JSONL, Parquet, Arrow formats.
- Cache hit/miss tests: ingest same shard twice; assert second call completes without network request and within 10 ms.
- Rate limiter tests: send 350 requests in 60 s; assert no more than 300 are forwarded to the upstream API.
- Schema drift tests: change a field type between two sync runs; assert `SCHEMA_DRIFT` event is emitted and ingestion pauses.
- Quality filter tests: inject 1,000 samples with 15 % below threshold; assert exactly 150 samples are excluded from the ThemisDB collection.
- Token leak tests: trigger an authentication failure; assert the raw API token does not appear in logs or error messages.

## Performance Targets

- Dataset streaming throughput ≥ 10,000 samples/s for text samples ≤ 2 KB each (single worker thread).
- Local cache hit rate ≥ 90 % for repeated loads of the same dataset within the TTL window.
- Shard download ≥ 50 MB/s on a 1 Gbit connection with a single worker.
- Tokenisation transform overhead ≤ 0.5 ms/sample (llama.cpp tokeniser, ≤ 512 tokens).
- Scheduled sync startup latency (first shard received) ≤ 2 s after trigger.

## Security / Reliability

- HuggingFace API tokens MUST be stored in environment variables only; no tokens in logs, error messages, or HTTP response bodies returned by the plugin.
- Local cache entries MUST be validated against their stored SHA-256 on read; corrupted entries are deleted and re-fetched.
- Sync jobs MUST be idempotent: re-running a completed sync must not create duplicate records in the ThemisDB collection.
- Rate limiter MUST apply exponential back-off on 429 responses; max back-off 60 s.
- Plugin MUST fail-safe on schema drift: pause ingestion and emit `SCHEMA_DRIFT` alert rather than silently truncating or coercing fields.

## Research / References

- Q. Lhoest et al., "Datasets: A community library for natural language processing," in *Proc. 2021 Conf. Empirical Methods Natural Language Processing (EMNLP): System Demonstrations*, 2021, pp. 175–184. DOI: [10.18653/v1/2021.emnlp-demo.21](https://doi.org/10.18653/v1/2021.emnlp-demo.21)
- T. Wolf et al., "Transformers: State-of-the-art natural language processing," in *Proc. 2020 Conf. Empirical Methods Natural Language Processing (EMNLP): System Demonstrations*, 2020, pp. 38–45. DOI: [10.18653/v1/2020.emnlp-demos.6](https://doi.org/10.18653/v1/2020.emnlp-demos.6)
- L. Gao et al., "The Pile: An 800GB dataset of diverse text for language modeling," arXiv:2101.00027, 2020. [https://arxiv.org/abs/2101.00027](https://arxiv.org/abs/2101.00027)
- T. Brown et al., "Language models are few-shot learners," in *Advances in Neural Information Processing Systems (NeurIPS)*, vol. 33, 2020, pp. 1877–1901. arXiv:2005.14165. [https://arxiv.org/abs/2005.14165](https://arxiv.org/abs/2005.14165)
- D. Zha et al., "Data-centric artificial intelligence: A survey," arXiv:2303.10158, 2023. [https://arxiv.org/abs/2303.10158](https://arxiv.org/abs/2303.10158)
