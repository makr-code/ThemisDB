# Exporter Plugins – Future Enhancements

> **Note:** Items here are *not* on the active roadmap. They are ideas and backlog entries for later consideration. See [`roadmap.md`](roadmap.md) for committed near-term work.

---

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

## Research / References

- [ ] TODO: Add reference – *Apache Arrow: A Cross-Language Development Platform for In-Memory Analytics* (URL placeholder)
- [ ] TODO: Add reference – *Delta Lake: High-Performance ACID Table Storage* – arXiv:2004.13007
- [ ] TODO: Add reference – *Apache Iceberg Table Spec* (URL placeholder)
