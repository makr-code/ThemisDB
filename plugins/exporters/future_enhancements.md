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

- M. Zaharia et al., "Delta lake: High-performance ACID table storage over cloud object stores," *Proc. VLDB Endow.*, vol. 13, no. 12, pp. 3411–3424, Aug. 2020. DOI: [10.14778/3415478.3415560](https://doi.org/10.14778/3415478.3415560)
- S. Iyer et al., "Apache Iceberg: An open table format for huge analytic datasets," in *Proc. 2022 ACM SIGMOD International Conf. Management of Data*, 2022, pp. 2771–2773. DOI: [10.1145/3514221.3526138](https://doi.org/10.1145/3514221.3526138)
- P. Pedreira et al., "Velox: Meta's unified execution engine," *Proc. VLDB Endow.*, vol. 15, no. 12, pp. 3372–3384, 2022. DOI: [10.14778/3554821.3554831](https://doi.org/10.14778/3554821.3554831)
- C. Li et al., "Apache Arrow Flight: A framework for fast data transport," in *Proc. 2020 IEEE International Conf. Big Data (Big Data)*, 2020, pp. 4551–4557. DOI: [10.1109/BigData50022.2020.9378283](https://doi.org/10.1109/BigData50022.2020.9378283)
- W. McKinney, "Data structures for statistical computing in Python," in *Proc. 9th Python in Science Conf.*, 2010, pp. 56–61. DOI: [10.25080/Majora-92bf1922-00a](https://doi.org/10.25080/Majora-92bf1922-00a)
