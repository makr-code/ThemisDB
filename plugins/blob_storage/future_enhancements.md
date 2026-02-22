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

- S. Ghemawat, H. Gobioff, and S.-T. Leung, "The Google file system," in *Proc. 19th ACM Symp. Operating Systems Principles (SOSP)*, 2003, pp. 29–43. DOI: [10.1145/945445.945450](https://doi.org/10.1145/945445.945450)
- G. DeCandia et al., "Dynamo: Amazon's highly available key-value store," in *Proc. 21st ACM Symp. Operating Systems Principles (SOSP)*, 2007, pp. 205–220. DOI: [10.1145/1294261.1294281](https://doi.org/10.1145/1294261.1294281)
- B. Zhu, K. Li, and R. H. Patterson, "Avoiding the disk bottleneck in the data domain deduplication file system," in *Proc. 6th USENIX Conf. File and Storage Technologies (FAST)*, 2008, pp. 269–282.
- C. Huang et al., "Erasure coding in Windows Azure storage," in *Proc. 2012 USENIX Annual Technical Conf. (ATC)*, 2012, pp. 15–26.
- A. Muthitacharoen, B. Chen, and D. Mazières, "A low-bandwidth network file system," in *Proc. 18th ACM Symp. Operating Systems Principles (SOSP)*, 2001, pp. 174–187. DOI: [10.1145/502034.502052](https://doi.org/10.1145/502034.502052)
