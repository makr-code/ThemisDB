# Blob Storage Plugin – Future Enhancements

> **Note:** Items here are *not* on the active roadmap. They are ideas and backlog entries for later consideration. See [`roadmap.md`](roadmap.md) for committed near-term work.

---

## Idea Backlog

### Additional Storage Backends

- [ ] **MinIO** – self-hosted S3-compatible backend for air-gapped deployments.
- [ ] **Backblaze B2** – cost-efficient alternative cloud storage.
- [ ] **FTP / SFTP** – legacy enterprise integration.
- [ ] **IPFS / Filecoin** – decentralised / content-addressed storage.

### Advanced Features

- [ ] **Deduplication** – content-addressed storage to avoid storing duplicate blobs.
- [ ] **Delta compression** – store incremental changes rather than full objects.
- [ ] **Streaming upload/download** – chunked transfer for very large objects (> 1 GB).
- [ ] **Encryption at rest (client-side)** – encrypt before upload, decrypt after download.
- [ ] **Blob tagging and search** – store and query custom metadata tags via AQL.

### Observability

- [ ] Per-backend latency histograms exported to Prometheus.
- [ ] Cost-estimation module (estimated cloud spend per collection).

### Developer Experience

- [ ] CLI command (`themisdb blob ls`, `themisdb blob put`) for manual inspection.
- [ ] Mock backend for unit tests (in-memory, zero dependencies).

---

## Research / References

- [ ] TODO: Add reference – *Erasure Coding in Distributed Storage* (DOI / arXiv placeholder)
- [ ] TODO: Add reference – *Data Deduplication Techniques Survey* (DOI / arXiv placeholder)
- [ ] TODO: Add reference – *Content-Addressed Storage* (URL placeholder)
