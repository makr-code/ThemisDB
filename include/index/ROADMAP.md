<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ../../src/index/ROADMAP.md -->

# Roadmap — Index Module (Public Headers)

> Implementation roadmap: `../../src/index/ROADMAP.md`

## Current Status

v1.7.0 — Production-ready. 39 public headers. Full vector, full-text, spatial, graph, and learned index support. GPU and multi-GPU backends available. Index compression added in v1.7.0.

## Completed ✅

- [x] `IAnnIndex` universal ANN interface with ScaNN and DiskANN backends
- [x] GPU single and multi-GPU vector indexes
- [x] Distributed vector index
- [x] CUDA HNSW traversal
- [x] Learned index and quantiser
- [x] Tiered index manager with cost-based routing
- [x] 5-codec index compression (v1.7.0)
- [x] Full-text inverted index
- [x] Spatial, property graph, temporal graph, process graph indexes
- [x] Binary, product, residual quantisers
- [x] RoPE variants (base, GPU, LoRA, learnable)
- [x] GNN embeddings, approximate radius search, multi-vector search

## Planned

- [ ] Multi-GPU distributed vector index improvements (Issue #1878) (Target: v1.8.0)
- [ ] Multi-tenancy isolation hardening (Issue #1872) (Target: v1.8.0)
- [ ] Integration test suite (Issue #1883) (Target: v1.8.0)
- [ ] Performance benchmark suite (Issue #1884) (Target: v1.8.0)
- [ ] Full security audit (Issue #1885) (Target: v1.8.0)
- [ ] API stability guarantees documentation (Issue #1887) (Target: v1.8.0)

## Production Readiness Checklist

- [x] 39 public headers compile cleanly
- [x] 4 ANN backends (ScaNN, DiskANN, GPU, Multi-GPU)
- [x] Index compression (5 codecs)
- [ ] Multi-tenancy isolation hardened
- [ ] Full integration test suite
- [ ] Performance benchmark suite
- [ ] Security audit complete
