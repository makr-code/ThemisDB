<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Security — Index Module (Public Headers)

> For reporting security vulnerabilities, see the project-level [SECURITY.md](../../../SECURITY.md).

## Security Scope

The Index module public headers expose vector, full-text, spatial, and graph index interfaces including GPU and distributed operations. Security concerns focus on multi-tenant isolation, GPU memory safety, injection via query parameters, and resource exhaustion.

## Threat Model

| Threat | Mitigation |
|--------|------------|
| Cross-tenant vector index leakage | Each index operation scoped to tenant context; multi-tenancy hardening in progress (Issue #1872) |
| GPU memory overflow in vector operations | `GPUMemoryOversubscriptionManager` tracks VRAM usage; operations aborted on budget exhaustion |
| Adversarial index poisoning via learned indexes | `LearnedIndex` re-training guarded by `admin:index:write` scope |
| Resource exhaustion via unlimited radius search | `ApproximateRadiusSearch` enforces configurable max-result-count limit |
| Index compression decompression bomb | `IndexCompressionCodec` validates decompressed size against expected bounds |
| Distributed index result tampering | `DistributedVectorIndex` checksums shard results before merge |
| HNSW parameter injection | `HnswParameterTuner` validates ef/M ranges before applying to live index |

## Security Controls

- Tenant-scoped index access enforced at `VectorIndexManager` level.
- GPU memory budget enforced via `GPUMemoryOversubscriptionManager`.
- Learned index re-training requires elevated scope.
- Compression decompression bounded by expected output size.

## Known Limitations

- Multi-tenancy isolation hardening is in progress (Issue #1872); full audit pending (Issue #1885).
- GPU-side bounds checking depends on CUDA driver version; keep CUDA toolkit updated.
- Implementation-level security details: `../../src/index/SECURITY.md`.
