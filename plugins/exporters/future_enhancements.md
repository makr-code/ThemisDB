# Exporter Plugins – Future Enhancements

> **Note:** Items here are *not* on the active roadmap. They are ideas and backlog entries for later consideration. See [`roadmap.md`](roadmap.md) for committed near-term work.

---

## Scope

- New export format plugins: Parquet (via Apache Arrow), Arrow IPC, HuggingFace `datasets`-compatible JSONL with metadata YAML.
- LLM training data quality scoring: perplexity filter, deduplication, instruction-response length validation.
- Pipeline integration: streaming export to S3/GCS, delta export (records changed since timestamp), and checksum manifests.
- Entry-points: `plugins/exporters/`; core implementation in `src/exporters/`.

## Design Constraints

- [ ] Every exporter MUST implement `IExporter` (`open`, `write_record`, `close`, `abort`).
- [ ] Exporters MUST support streaming mode; no exporter may buffer more than 128 MB in process memory.
- [ ] Export authorization MUST be verified per-collection before the first record is written.
- [ ] Sensitive field redaction rules MUST be applied before any record reaches the serialisation layer.
- [ ] Parquet row-group size MUST be configurable (default: 128 MB) to balance read performance and memory.
- [ ] Checksum manifest (SHA-256 per output file) MUST be written atomically after `close()`.

## Required Interfaces

| Interface | Consumer | Notes |
|---|---|---|
| `IExporter` | `ExporterPlugin`, pipeline scheduler | `open`, `write_record`, `close`, `abort` |
| `IExportAuthorizer` | `IExporter` impls | Per-collection access control check before export |
| `IFieldRedactor` | `IExporter` impls | Strips/masks sensitive fields; configurable per export job |
| `IChecksumManifestWriter` | `ExporterPlugin` | Writes SHA-256 manifest alongside output files |
| `IExportQualityFilter` | `ExporterPlugin` | Pluggable quality scoring; returns pass/fail + score per record |

## Idea Backlog

### New Export Formats

- [ ] **JSON-LD / RDF** – export knowledge-graph data with semantic annotations.
- [ ] **CBOR** – compact binary encoding for IoT and edge deployments.
- [ ] **MessagePack** – fast binary serialisation alternative to JSONL.
- [ ] **SQLite dump** – self-contained portable database export.
- [ ] **Markdown / HTML report** – human-readable export for auditing.

### LLM / AI Tooling

- [ ] **GGUF / safetensors export** – export fine-tuning datasets in formats consumed by llama.cpp / Transformers.
- [ ] **Instruction-tuning template support** – Alpaca, ShareGPT, ChatML format output.
- [ ] **Dataset card generation** – auto-generate HuggingFace `dataset_info.json` on export.

### Pipeline Integration

- [ ] **Apache Kafka sink** – stream exported records to a Kafka topic in real time.
- [ ] **Webhook exporter** – POST records to an HTTP endpoint.
- [ ] **dbt integration** – generate dbt-compatible manifest for downstream transformations.

### Quality / Validation

- [ ] **Schema validation on export** – reject records that don't match expected schema.
- [ ] **Differential export** – export only records changed since a given timestamp.
- [ ] **Checksum manifest** – produce a SHA-256 manifest alongside exports.

---

## Test Strategy

- Unit tests for each exporter with in-memory record streams; verify byte-level output matches reference fixtures for JSONL, Parquet, Arrow IPC.
- Authorization tests: assert that exporting from an unauthorised collection returns `EXPORT_DENIED` before any records are written.
- Redaction tests: export a collection containing PII fields; assert no redacted field value appears in output.
- Quality filter tests: inject 100 records with 20 % below quality threshold; assert exactly 20 records are dropped.
- Checksum manifest tests: corrupt one output byte post-export; assert manifest verification detects the mismatch.
- Performance regression tests: JSONL export of 1 M records must complete within 10 s on a single core.

## Performance Targets

- JSONL export throughput ≥ 100,000 records/s (single-threaded, records ≤ 1 KB each).
- Parquet export throughput ≥ 50 MB/s (uncompressed; ≥ 30 MB/s with zstd level 3).
- Arrow IPC streaming export ≤ 5 ms first-batch latency for batch size 1,000 records.
- Peak memory during export ≤ 128 MB regardless of total dataset size (streaming enforced).
- Checksum manifest generation adds ≤ 1 % overhead to total export wall-clock time.

## Security / Reliability

- Export authorization MUST be checked per-collection at job start; re-checked every 5 minutes for long-running exports.
- Sensitive field redaction MUST be enforced in `IFieldRedactor` before records reach any serialisation layer; redaction bypass is a hard error.
- Export jobs MUST be abortable; `abort()` MUST delete all partially written output files atomically.
- No collection schema, field names, or record counts may appear in log output at INFO level if the collection is marked `sensitive`.
- Output files MUST be written with mode `0600` (owner read/write only) until `close()` is called; then set to configured ACL.

## Research / References

- M. Zaharia et al., "Delta lake: High-performance ACID table storage over cloud object stores," *Proc. VLDB Endow.*, vol. 13, no. 12, pp. 3411–3424, Aug. 2020. DOI: [10.14778/3415478.3415560](https://doi.org/10.14778/3415478.3415560)
- S. Iyer et al., "Apache Iceberg: An open table format for huge analytic datasets," in *Proc. 2022 ACM SIGMOD International Conf. Management of Data*, 2022, pp. 2771–2773. DOI: [10.1145/3514221.3526138](https://doi.org/10.1145/3514221.3526138)
- P. Pedreira et al., "Velox: Meta's unified execution engine," *Proc. VLDB Endow.*, vol. 15, no. 12, pp. 3372–3384, 2022. DOI: [10.14778/3554821.3554831](https://doi.org/10.14778/3554821.3554831)
- C. Li et al., "Apache Arrow Flight: A framework for fast data transport," in *Proc. 2020 IEEE International Conf. Big Data (Big Data)*, 2020, pp. 4551–4557. DOI: [10.1109/BigData50022.2020.9378283](https://doi.org/10.1109/BigData50022.2020.9378283)
- W. McKinney, "Data structures for statistical computing in Python," in *Proc. 9th Python in Science Conf.*, 2010, pp. 56–61. DOI: [10.25080/Majora-92bf1922-00a](https://doi.org/10.25080/Majora-92bf1922-00a)
