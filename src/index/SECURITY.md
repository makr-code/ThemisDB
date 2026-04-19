> **Sicherheitshinweis:** Security-Angaben gegen aktuelle Build-Flags, Codepfade und Tests validieren.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Security — Index Module

> For reporting security vulnerabilities, see the project-level [SECURITY.md](../../../SECURITY.md).

## Security Scope

The Index module manages HNSW vector indexes, GPU-accelerated nearest-neighbour search (Vulkan/CUDA/HIP), quantisation schemes (PQ/BQ/RQ), B-tree/range/spatial secondary indexes (R-tree with Z-order curves), graph indexing, adaptive index advisor, full-text inverted index, DiskANN/ScaNN algorithms, and multi-GPU distributed vector indexing. Security controls apply to multi-tenancy isolation, GPU memory safety, key prefix scoping, and index access control.

## Threat Model

| Threat | Mitigation |
|--------|------------|
| GPU VRAM data leakage between tenants (residual vector data in VRAM after index eviction) | VRAM secure clear executed on all index eviction paths; GPU memory zeroed before deallocation |
| RocksDB key prefix collision enabling one tenant to read another tenant's index data | Tenant key prefix format `tenant:<id>:<index_name>` enforced at IndexManager layer; prefix separator chosen to prevent prefix-prefix ambiguity |
| Side-channel timing attack via index query latency (inferring neighbour existence from query timing) | Query result timing is normalised; batch query paths avoid early-exit leakage where feasible |
| Unbounded ANN query causing GPU OOM or CPU exhaustion (DoS) | Per-query result-set size limit and VRAM budget cap enforced; queries exceeding budget are rejected before GPU dispatch |
| Cross-tenant index advisor exposure (workload replay leaking query patterns of another tenant) | Adaptive index advisor operates within tenant-scoped query log partitions; cross-tenant workload data is never co-mingled |
| Index name injection (malicious tenant ID or index name containing separator characters) | Tenant IDs and index names are validated against an allow-list (alphanumeric + hyphen/underscore only) before key prefix construction |
| Insecure DiskANN/ScaNN on-disk files accessible to other processes | Index files are written to per-tenant directories with 0600 permissions; directory creation enforces umask |

## Security Controls

### GPU Memory Safety

- All GPU index eviction paths call a secure VRAM clear routine before returning memory to the device allocator.
- The VRAM clear is a GPU-side memset (not host-side) to ensure residual data is actually overwritten before the next allocation.
- Multi-GPU distributed index evictions are coordinated: all device copies are cleared before the index is considered evicted.
- GPU memory allocation failure does not leave partial index data accessible; the entire allocation is rolled back atomically.

### RocksDB Key Prefix Isolation

- Every RocksDB key for vector and secondary index data is prefixed with `tenant:<id>:<index_name>:` where `<id>` is the authenticated tenant identifier.
- The colon (`:`) separator is disallowed in tenant IDs and index names to prevent prefix-prefix collision attacks.
- Prefix extractor is configured in RocksDB column family options to enable bloom filter acceleration and to scope iterators to a single tenant prefix.
- The IndexManager layer validates the tenant context on every index open, read, write, and delete operation.
- Cross-tenant key access is impossible at the RocksDB layer because iterators are bounded by the tenant prefix range.

### Index Registry Isolation

- The IndexManager maintains a per-tenant index registry; index handles from one tenant cannot be passed to another tenant's query context.
- Index creation, deletion, and listing operations are gated by the tenant's authenticated identity.
- The global index registry (used for cache management) stores only opaque handles; no raw vector data or key material is held in the registry.

### Query Limits

- Maximum ANN result set (`k`): configurable, default 1000 vectors.
- Maximum VRAM usage per query: configurable, default 2 GB.
- Query timeout: configurable, default 10 s for GPU queries, 30 s for CPU fallback.
- Full-text index query maximum clause count: 1024.

### Adaptive Index Advisor

- Workload replay is scoped to the requesting tenant's query log.
- Index recommendations are generated locally per tenant; no cross-tenant query data is used.
- Replay logs are retained for a configurable window (default 7 days) and then purged.

## Data Handling

- Vector data stored in the HNSW index is tenant-scoped and not accessible across tenant boundaries.
- Quantised vectors (PQ/BQ/RQ) do not reconstruct the original embedding exactly; however, they are still treated as sensitive and subject to the same key prefix isolation.
- Full-text inverted index posting lists are stored under the same tenant prefix scheme as vector indexes.
- DiskANN/ScaNN on-disk index files are stored in per-tenant directories with restrictive filesystem permissions.

## Known Limitations

- **Multi-tenancy isolation (issue #1872):** The current implementation enforces key prefix isolation at the RocksDB and IndexManager layers, but a formal security audit of the multi-tenancy model has not been completed. This is tracked as an open security issue.
- **GPU VRAM isolation on shared-memory architectures:** On integrated GPU/CPU systems where VRAM and system RAM are shared, the secure-clear guarantee is best-effort; dedicated discrete GPU is recommended for strict tenant isolation.
- **Side-channel timing:** Full timing normalisation for ANN queries is not yet implemented; high-precision timing side-channels remain a theoretical risk in co-tenanted deployments.
- **HIP backend VRAM clear:** The HIP (AMD GPU) backend's secure VRAM clear has not been independently validated on all supported ROCm versions; requires hardware-specific testing (see issue #1878).

## Dependency Security

| Dependency | Usage | Review Status |
|------------|-------|---------------|
| RocksDB | Persistent index storage | Reviewed; tenant prefix isolation enforced |
| Vulkan SDK | GPU vector search (compute shaders) | Reviewed; descriptor set lifecycle managed |
| CUDA toolkit | GPU HNSW distance kernels | Reviewed; stream synchronisation validated |
| ROCm/HIP | AMD GPU backend | ⚠️ VRAM clear validation pending (#1878) |
| DiskANN library | Billion-scale ANN | Under review |
| ScaNN library | Asymmetric quantised ANN | Under review |
