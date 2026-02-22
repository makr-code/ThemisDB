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

- Q. Lhoest et al., "Datasets: A community library for natural language processing," in *Proc. 2021 Conf. Empirical Methods Natural Language Processing (EMNLP): System Demonstrations*, 2021, pp. 175–184. DOI: [10.18653/v1/2021.emnlp-demo.21](https://doi.org/10.18653/v1/2021.emnlp-demo.21)
- T. Wolf et al., "Transformers: State-of-the-art natural language processing," in *Proc. 2020 Conf. Empirical Methods Natural Language Processing (EMNLP): System Demonstrations*, 2020, pp. 38–45. DOI: [10.18653/v1/2020.emnlp-demos.6](https://doi.org/10.18653/v1/2020.emnlp-demos.6)
- L. Gao et al., "The Pile: An 800GB dataset of diverse text for language modeling," arXiv:2101.00027, 2020. [https://arxiv.org/abs/2101.00027](https://arxiv.org/abs/2101.00027)
- T. Brown et al., "Language models are few-shot learners," in *Advances in Neural Information Processing Systems (NeurIPS)*, vol. 33, 2020, pp. 1877–1901. arXiv:2005.14165. [https://arxiv.org/abs/2005.14165](https://arxiv.org/abs/2005.14165)
- D. Zha et al., "Data-centric artificial intelligence: A survey," arXiv:2303.10158, 2023. [https://arxiv.org/abs/2303.10158](https://arxiv.org/abs/2303.10158)
