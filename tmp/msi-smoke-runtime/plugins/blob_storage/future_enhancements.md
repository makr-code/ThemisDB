# Blob Storage Plugin – Future Enhancements

> **Note:** Items here are *not* on the active roadmap. They are ideas and backlog entries for later consideration. See [`ROADMAP.md`](ROADMAP.md) for committed near-term work.

---

## Scope

- Enhancements to the blob storage plugin covering new backend providers (GCS, MinIO, Backblaze B2), multipart upload optimisation, client-side encryption, and tiered storage integration.
- Targets `plugins/blob_storage/azure/` and `plugins/blob_storage/s3/` entry-points; new backends follow the same `IBlobStorageBackend` interface.
- Out of scope: changes to core ThemisDB query engine or vector index; blob plugin only handles object read/write/delete lifecycle.
- Covers observability instrumentation (OpenTelemetry spans per blob operation) and SDK ergonomics improvements.

## Design Constraints

- [ ] Every backend MUST implement `IBlobStorageBackend` (upload, download, delete, list, exists).
- [ ] No backend may cache credentials in process memory beyond the lifetime of a single request handler.
- [ ] Multipart thresholds MUST be configurable per backend (default: 64 MB part size, 8 parallel parts).
- [ ] Client-side encryption MUST use AES-256-GCM with a per-object random 96-bit nonce; plaintext never written to disk.
- [ ] Tiered storage transitions (hot → cool → archive) MUST be idempotent and produce audit log entries.
- [ ] All backend constructors MUST validate required configuration keys at startup and fail fast with a descriptive error.

## Required Interfaces

| Interface | Consumer | Notes |
|---|---|---|
| `IBlobStorageBackend` | `BlobStoragePlugin`, ThemisDB core | `upload`, `download`, `delete`, `list`, `exists` methods |
| `IEncryptionProvider` | `BlobStoragePlugin` | Pluggable AES-256-GCM wrapper; default impl provided |
| `IMultipartUploader` | `IBlobStorageBackend` impls | Handles part splitting, parallel upload, manifest commit |
| `IBlobMetadataStore` | `BlobStoragePlugin` | Persists object key → tier, checksum, encryption nonce |
| OpenTelemetry `Tracer` | All backends | One span per blob operation with status + byte-count attribute |

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

## Test Strategy

- Unit tests for each backend using a mock HTTP server (localstack for S3, Azurite for Azure); ≥ 90 % line coverage.
- Integration tests against real GCS/S3/Azure buckets run in CI nightly; gated behind `BLOB_INTEGRATION_TESTS=1`.
- Encryption round-trip tests: encrypt → upload → download → decrypt must reproduce identical bytes for 1 KB, 1 MB, 100 MB objects.
- Fault-injection tests: simulate network timeout mid-upload; verify multipart abort and no orphan parts.
- Tiered storage tests: trigger hot → cool transition and assert metadata store reflects new tier within 5 s.

## Performance Targets

- S3 multipart upload throughput ≥ 500 MB/s (8 × 64 MB parts in parallel on 10 Gbit NIC).
- GCS resumable upload latency ≤ 100 ms for first-chunk acknowledgement on objects ≤ 1 MB.
- Blob read p99 latency ≤ 50 ms for objects ≤ 1 MB from same-region endpoint.
- AES-256-GCM client-side encryption overhead ≤ 3 % of total upload wall-clock time.
- Backend `exists()` check ≤ 5 ms p99 (HEAD request, no data transfer).

## Security / Reliability

- All credentials (API keys, SAS tokens, service-account JSON) MUST be sourced from environment variables or a configured secret manager; never from config files committed to source control.
- No credential material, plaintext object content, or encryption keys may appear in log output at any log level.
- Client-side AES-256-GCM encryption MUST be enabled by default when `blob.client_encryption=true`; the per-object nonce is stored in `IBlobMetadataStore`, not in object metadata headers.
- Upload operations MUST verify SHA-256 checksum post-upload; mismatch triggers automatic retry up to 3 times before returning error.
- Backend connection pools MUST enforce a configurable maximum idle lifetime (default 60 s) to prevent stale credential use after rotation.

## Research / References

- S. Ghemawat, H. Gobioff, and S.-T. Leung, "The Google file system," in *Proc. 19th ACM Symp. Operating Systems Principles (SOSP)*, 2003, pp. 29–43. DOI: [10.1145/945445.945450](https://doi.org/10.1145/945445.945450)
- G. DeCandia et al., "Dynamo: Amazon's highly available key-value store," in *Proc. 21st ACM Symp. Operating Systems Principles (SOSP)*, 2007, pp. 205–220. DOI: [10.1145/1294261.1294281](https://doi.org/10.1145/1294261.1294281)
- B. Zhu, K. Li, and R. H. Patterson, "Avoiding the disk bottleneck in the data domain deduplication file system," in *Proc. 6th USENIX Conf. File and Storage Technologies (FAST)*, 2008, pp. 269–282.
- C. Huang et al., "Erasure coding in Windows Azure storage," in *Proc. 2012 USENIX Annual Technical Conf. (ATC)*, 2012, pp. 15–26.
- A. Muthitacharoen, B. Chen, and D. Mazières, "A low-bandwidth network file system," in *Proc. 18th ACM Symp. Operating Systems Principles (SOSP)*, 2001, pp. 174–187. DOI: [10.1145/502034.502052](https://doi.org/10.1145/502034.502052)
