# Implementation Influence — By Module

This file groups all research influences by ThemisDB module (`src/<module>`).  
*Generated from the master matrix in [README.md](README.md).*

---

## Module Index

- [src/aql/](#srcaql)
- [src/analytics/](#srcanalytics)
- [src/auth/](#srcauth)
- [src/cache/](#srccache)
- [src/chimera/](#srcchimera)
- [src/config/](#srcconfig)
- [src/graph/](#srcgraph)
- [src/index/](#srcindex)
- [src/llm/](#srcllm)
- [src/process/](#srcprocess)
- [src/prompt\_engineering/](#srcprompt_engineering)
- [src/query/](#srcquery)
- [src/rag/](#srcrag)
- [src/rpc\_grpc/](#srcrpc_grpc)
- [src/server/](#srcserver)
- [src/sharding/](#srcsharding)
- [src/temporal/](#srctemporal)
- [src/vector/](#srcvector)
- [plugins/user\_storage\_encrypted/](#pluginsuser_storage_encrypted)

---

## src/aql/

| Category | Source | Version | Status |
|----------|--------|---------|--------|
| Book | Robinson, Webber & Eifrem (2015) — Graph Databases | v1.0.0+ | 🔄 Partial |
| Architecture Decision | ADR-004 — Native Multi-Model Data Model | v1.0.0+ | ✅ Accepted |

---

## src/analytics/

| Category | Source | Version | Status |
|----------|--------|---------|--------|
| Book | van der Aalst (2016) — Process Mining | v1.9.0+ | 🔄 Partial |

---

## src/auth/

| Category | Source | Version | Status |
|----------|--------|---------|--------|
| Standard | FITKO (2024) / BMI (2017) — OZG, FIM, XÖV Standards | v1.9.0+ | 🔄 Partial |
| Best Practice | RFC 7519 + RFC 6749 — JWT Short-Lived Tokens | v1.6.0 | ✅ Adopted |
| Architecture Decision | ADR-008 — JWT + OAuth2 PKCE for API Auth | v1.6.0+ | ✅ Accepted |

---

## src/cache/

| Category | Source | Version | Status |
|----------|--------|---------|--------|
| Best Practice | Martin Thompson — Lock-Free Cache Reads (Mechanical Sympathy) | v1.9.0 | ✅ Adopted |
| Best Practice | RocksDB Docs — WriteBatch Atomicity | v1.0.0+ | ✅ Adopted |
| Best Practice | C++17 §30.6.5 — std::shared_mutex R/W Locks | v1.8.0+ | ✅ Adopted |
| Architecture Decision | ADR-002 — RocksDB as Primary Storage Backend | v1.0.0+ | ✅ Accepted |

---

## src/chimera/

| Category | Source | Version | Status |
|----------|--------|---------|--------|
| Best Practice | AWS Builder's Library — Exponential Backoff + Circuit Breaker | v1.8.0 | ✅ Adopted |
| Architecture Decision | ADR-006 — Plugin Adapter Architecture (Chimera) | v1.0.0+ | ✅ Accepted |

---

## src/config/

| Category | Source | Version | Status |
|----------|--------|---------|--------|
| Best Practice | C++17 §30.6.5 — std::shared_mutex R/W Locks | v1.8.0+ | ✅ Adopted |

---

## src/graph/

| Category | Source | Version | Status |
|----------|--------|---------|--------|
| Book | Robinson, Webber & Eifrem (2015) — Graph Databases | v1.0.0+ | 🔄 Partial |
| Paper | Edge et al. (2024) — GraphRAG | planned Q3 2026 | ⏳ Planned |
| Architecture Decision | ADR-004 — Native Multi-Model Data Model | v1.0.0+ | ✅ Accepted |

---

## src/index/

| Category | Source | Version | Status |
|----------|--------|---------|--------|
| Paper | Malkov & Yashunin (2020) — HNSW | v1.0.0+ | ✅ Implemented |
| Best Practice | RocksDB Docs — WriteBatch Atomicity | v1.0.0+ | ✅ Adopted |
| Architecture Decision | ADR-001 — HNSW over FAISS for ANN Vector Index | v1.0.0+ | ✅ Accepted |
| Architecture Decision | ADR-002 — RocksDB as Primary Storage Backend | v1.0.0+ | ✅ Accepted |
| Architecture Decision | ADR-004 — Native Multi-Model Data Model | v1.0.0+ | ✅ Accepted |

---

## src/llm/

| Category | Source | Version | Status |
|----------|--------|---------|--------|
| Paper | Vaswani et al. (2017) — Attention Is All You Need | v1.3.0+ | ✅ Implemented |
| Paper | Hu et al. (2022) — LoRA | v1.3.0+ | ✅ Implemented |
| Paper | Dettmers et al. (2023) — QLoRA | v1.3.0+ | ✅ Implemented |
| Paper | Es et al. (2023) — RAGAS | v1.3.0+ | ✅ Implemented |
| Paper | Dao et al. (2022) — FlashAttention (CUDA kernels) | v1.4.0-alpha+ | ✅ Implemented |
| Paper | Chen et al. (2023) — Speculative Decoding | v1.4.0-alpha+ | ✅ Implemented |
| Paper | Kwon et al. (2023) — PagedAttention | v1.4.0-alpha+ | ✅ Implemented |
| Paper | Busch et al. (2023) — ProcessGPT | planned Q2 2026 | ⏳ Planned |
| Paper | Beurer-Kellner et al. (2023) — LMQL | planned v2.x | ⏳ Planned |

---

## src/process/

| Category | Source | Version | Status |
|----------|--------|---------|--------|
| Book | van der Aalst (2016) — Process Mining | v1.9.0+ | 🔄 Partial |
| Paper | Busch et al. (2023) — ProcessGPT | planned Q2 2026 | ⏳ Planned |
| Paper | Bukhsh et al. (2021) — ProcessTransformer | planned Q1 2027 | ⏳ Planned |
| Paper | Edge et al. (2024) — GraphRAG | planned Q3 2026 | ⏳ Planned |
| Paper | Gutierrez et al. (2024) — HippoRAG | planned Q2 2026 | ⏳ Planned |
| Standard | FITKO (2024) / BMI (2017) — OZG, FIM, XÖV Standards | v1.9.0+ | 🔄 Partial |

---

## src/prompt_engineering/

| Category | Source | Version | Status |
|----------|--------|---------|--------|
| Paper | Brown et al. (2020) — GPT-3 Few-Shot Learning | v1.2.0+ | ✅ Implemented |
| Paper | Wei et al. (2022) — Chain-of-Thought Prompting | v1.2.0+ | ✅ Implemented |
| Paper | White et al. (2023) — Prompt Pattern Catalog | v1.2.0+ | ✅ Implemented |
| Paper | Zhou et al. (2022) — APE (Automatic Prompt Engineer) | v1.4.0-alpha+ | ✅ Implemented |
| Paper | Beurer-Kellner et al. (2023) — LMQL | planned v2.x | ⏳ Planned |
| Best Practice | Austin Appleby — MurmurHash3 Sharding | v1.9.0 | ✅ Adopted |
| Best Practice | FNV — FNV-1a 64-bit Checksums | v2.0.0 | ✅ Adopted |

---

## src/query/

| Category | Source | Version | Status |
|----------|--------|---------|--------|
| Paper | Kulkarni & Michels (2012) — SQL:2011 Temporal Features | v1.x+ | 🔄 Partial |
| Paper | Raasveldt & Mühleisen (2019) — DuckDB | planned v2.x | ⏳ Planned |
| Architecture Decision | ADR-004 — Native Multi-Model Data Model | v1.0.0+ | ✅ Accepted |

---

## src/rag/

| Category | Source | Version | Status |
|----------|--------|---------|--------|
| Paper | Devlin et al. (2019) — BERT | v1.0.0+ | ✅ Implemented |
| Paper | Reimers & Gurevych (2019) — Sentence-BERT | v1.0.0+ | ✅ Implemented |
| Paper | Lewis et al. (2020) — RAG | v1.2.0+ | ✅ Implemented |
| Paper | Malkov & Yashunin (2020) — HNSW | v1.0.0+ | ✅ Implemented |
| Paper | Edge et al. (2024) — GraphRAG | planned Q3 2026 | ⏳ Planned |
| Paper | Gutierrez et al. (2024) — HippoRAG | planned Q2 2026 | ⏳ Planned |
| Architecture Decision | ADR-002 — RocksDB as Primary Storage Backend | v1.0.0+ | ✅ Accepted |

---

## src/rpc_grpc/

| Category | Source | Version | Status |
|----------|--------|---------|--------|
| Architecture Decision | ADR-007 — gRPC + Protobuf for Internal RPC | v1.0.0+ | ✅ Accepted |

---

## src/server/

| Category | Source | Version | Status |
|----------|--------|---------|--------|
| Best Practice | Herb Sutter (GotW #24) — PIMPL Idiom | v1.9.0 | ✅ Adopted |
| Best Practice | RFC 6585 — Token Bucket Rate Limiting | v1.6.0 | ✅ Adopted |
| Best Practice | CNCF OpenTelemetry Spec — Span Instrumentation | v1.9.0 | ✅ Adopted |
| Best Practice | RFC 7519 + RFC 6749 — JWT Short-Lived Tokens | v1.6.0 | ✅ Adopted |
| Best Practice | NIST SP 800-52 Rev 2 — TLS 1.3 Cipher Hardening | v1.0.0+ | ✅ Adopted |
| Best Practice | Karger et al. (1997) — Consistent Hash Ring | v2.1.0 | ✅ Adopted |
| Best Practice | Boost.Asio Docs — Proactor Async I/O | v1.0.0+ | ✅ Adopted |
| Architecture Decision | ADR-003 — Boost.Beast + Asio for HTTP Server | v1.0.0+ | ✅ Accepted |
| Architecture Decision | ADR-007 — gRPC + Protobuf for Internal RPC | v1.0.0+ | ✅ Accepted |
| Architecture Decision | ADR-008 — JWT + OAuth2 PKCE for API Auth | v1.6.0+ | ✅ Accepted |

---

## src/sharding/

| Category | Source | Version | Status |
|----------|--------|---------|--------|
| Best Practice | Karger et al. (1997) — Consistent Hash Ring | v2.1.0 | ✅ Adopted |
| Best Practice | Austin Appleby — MurmurHash3 Sharding | v1.9.0 | ✅ Adopted |

---

## src/temporal/

| Category | Source | Version | Status |
|----------|--------|---------|--------|
| Paper | Kulkarni & Michels (2012) — SQL:2011 Temporal Features | v1.x+ | 🔄 Partial |

---

## src/vector/

| Category | Source | Version | Status |
|----------|--------|---------|--------|
| Paper | Devlin et al. (2019) — BERT | v1.0.0+ | ✅ Implemented |
| Paper | Reimers & Gurevych (2019) — Sentence-BERT | v1.0.0+ | ✅ Implemented |
| Paper | Malkov & Yashunin (2020) — HNSW | v1.0.0+ | ✅ Implemented |
| Paper | Kusupati et al. (2022) — Matryoshka Representation Learning | v1.4.1+ | ⏳ Planned |

---

## plugins/user_storage_encrypted/

| Category | Source | Version | Status |
|----------|--------|---------|--------|
| Best Practice | RFC 9106 — Argon2id Key Derivation | v0.1.0 | ✅ Adopted |
| Best Practice | CERT C MSC06-C — Secure Key Zeroing | v0.1.0 | ✅ Adopted |
| Architecture Decision | ADR-005 — Argon2id over scrypt/bcrypt | v0.1.0 | ✅ Accepted |

---

*Last generated: see git log*

