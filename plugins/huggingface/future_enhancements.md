# HuggingFace Ingestion Plugin – Future Enhancements

> **Note:** Items here are *not* on the active roadmap. They are ideas and backlog entries for later consideration. See [`roadmap.md`](roadmap.md) for committed near-term work.

---

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

## Research / References

- [ ] TODO: Add reference – *HuggingFace Datasets: A Community Library for NLP* – arXiv:2109.02846
- [ ] TODO: Add reference – *The Pile: An 800GB Dataset of Diverse Text* – arXiv:2101.00027
- [ ] TODO: Add reference – *Data-Centric AI* (URL placeholder)
