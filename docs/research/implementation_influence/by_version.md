# Implementation Influence — By Version

This file groups research influences by the ThemisDB version in which they were first applied.  
*Generated from the master matrix in [README.md](README.md).*

---

## v0.1.0

| Source | Type | Module(s) | Status |
|--------|------|-----------|--------|
| RFC 9106 — Argon2id Key Derivation | Best Practice | `plugins/user_storage_encrypted/` | ✅ Adopted |
| CERT C MSC06-C — Secure Key Zeroing | Best Practice | `plugins/user_storage_encrypted/` | ✅ Adopted |
| ADR-005 — Argon2id over scrypt/bcrypt | Architecture Decision | `plugins/user_storage_encrypted/` | ✅ Accepted |

---

## v1.0.0+

| Source | Type | Module(s) | Status |
|--------|------|-----------|--------|
| Devlin et al. (2019) — BERT | Paper | `src/vector/`, `src/rag/` | ✅ Implemented |
| Reimers & Gurevych (2019) — Sentence-BERT | Paper | `src/vector/`, `src/rag/` | ✅ Implemented |
| Malkov & Yashunin (2020) — HNSW | Paper | `src/index/`, `src/vector/`, `src/rag/` | ✅ Implemented |
| Robinson, Webber & Eifrem (2015) — Graph Databases | Book | `src/graph/`, `src/aql/` | 🔄 Partial |
| RocksDB Docs — WriteBatch Atomicity | Best Practice | `src/index/`, `src/cache/` | ✅ Adopted |
| NIST SP 800-52 Rev 2 — TLS 1.3 Cipher Hardening | Best Practice | `src/server/` | ✅ Adopted |
| Boost.Asio Docs — Proactor Async I/O | Best Practice | `src/server/` | ✅ Adopted |
| ADR-001 — HNSW over FAISS for ANN Vector Index | Architecture Decision | `src/index/` | ✅ Accepted |
| ADR-002 — RocksDB as Primary Storage Backend | Architecture Decision | `src/cache/`, `src/index/`, `src/rag/` | ✅ Accepted |
| ADR-003 — Boost.Beast + Asio for HTTP Server | Architecture Decision | `src/server/` | ✅ Accepted |
| ADR-004 — Native Multi-Model Data Model | Architecture Decision | `src/aql/`, `src/graph/`, `src/index/`, `src/query/` | ✅ Accepted |
| ADR-006 — Plugin Adapter Architecture (Chimera) | Architecture Decision | `src/chimera/` | ✅ Accepted |
| ADR-007 — gRPC + Protobuf for Internal RPC | Architecture Decision | `src/rpc_grpc/`, `src/server/` | ✅ Accepted |

---

## v1.x+

| Source | Type | Module(s) | Status |
|--------|------|-----------|--------|
| Kulkarni & Michels (2012) — SQL:2011 Temporal Features | Paper | `src/temporal/`, `src/query/` | 🔄 Partial |

---

## v1.2.0+

| Source | Type | Module(s) | Status |
|--------|------|-----------|--------|
| Brown et al. (2020) — GPT-3 Few-Shot Learning | Paper | `src/prompt_engineering/` | ✅ Implemented |
| Wei et al. (2022) — Chain-of-Thought Prompting | Paper | `src/prompt_engineering/` | ✅ Implemented |
| White et al. (2023) — Prompt Pattern Catalog | Paper | `config/prompts/`, `src/prompt_engineering/` | ✅ Implemented |
| Lewis et al. (2020) — RAG | Paper | `src/rag/` | ✅ Implemented |

---

## v1.3.0+

| Source | Type | Module(s) | Status |
|--------|------|-----------|--------|
| Vaswani et al. (2017) — Attention Is All You Need | Paper | `src/llm/` | ✅ Implemented |
| Hu et al. (2022) — LoRA | Paper | `src/llm/lora/`, `src/training/` | ✅ Implemented |
| Dettmers et al. (2023) — QLoRA | Paper | `src/llm/lora/` | ✅ Implemented |
| Es et al. (2023) — RAGAS | Paper | `src/llm/monitoring/` | ✅ Implemented |

---

## v1.4.0-alpha+

| Source | Type | Module(s) | Status |
|--------|------|-----------|--------|
| Zhou et al. (2022) — APE (Automatic Prompt Engineer) | Paper | `src/prompt_engineering/` | ✅ Implemented |
| Dao et al. (2022) — FlashAttention | Paper | `src/llm/` (CUDA kernels) | ✅ Implemented |
| Chen et al. (2023) — Speculative Decoding | Paper | `src/llm/` | ✅ Implemented |
| Kwon et al. (2023) — PagedAttention | Paper | `src/llm/` | ✅ Implemented |

---

## v1.4.1+

| Source | Type | Module(s) | Status |
|--------|------|-----------|--------|
| Kusupati et al. (2022) — Matryoshka Representation Learning | Paper | `src/vector/` | ⏳ Planned |

---

## v1.6.0

| Source | Type | Module(s) | Status |
|--------|------|-----------|--------|
| RFC 6585 — Token Bucket Rate Limiting | Best Practice | `src/server/` | ✅ Adopted |
| RFC 7519 + RFC 6749 — JWT Short-Lived Tokens | Best Practice | `src/server/`, `src/auth/` | ✅ Adopted |
| ADR-008 — JWT + OAuth2 PKCE for API Auth | Architecture Decision | `src/server/`, `src/auth/` | ✅ Accepted |

---

## v1.8.0

| Source | Type | Module(s) | Status |
|--------|------|-----------|--------|
| AWS Builder's Library — Exponential Backoff + Circuit Breaker | Best Practice | `src/chimera/` | ✅ Adopted |
| C++17 §30.6.5 — std::shared_mutex R/W Locks | Best Practice | `src/cache/`, `src/config/` | ✅ Adopted |

---

## v1.9.0

| Source | Type | Module(s) | Status |
|--------|------|-----------|--------|
| Martin Thompson — Lock-Free Cache Reads (Mechanical Sympathy) | Best Practice | `src/cache/` | ✅ Adopted |
| Herb Sutter (GotW #24) — PIMPL Idiom | Best Practice | `src/server/` | ✅ Adopted |
| CNCF OpenTelemetry Spec — Span Instrumentation | Best Practice | `src/server/` (all 64 handlers) | ✅ Adopted |
| Austin Appleby — MurmurHash3 Sharding | Best Practice | `src/prompt_engineering/`, `src/sharding/` | ✅ Adopted |
| van der Aalst (2016) — Process Mining | Book | `src/process/`, `src/analytics/` | 🔄 Partial |
| FITKO (2024) / BMI (2017) — OZG, FIM, XÖV Standards | Standard | `src/process/`, `src/importers/`, `src/auth/` | 🔄 Partial |

---

## v2.0.0

| Source | Type | Module(s) | Status |
|--------|------|-----------|--------|
| FNV — FNV-1a 64-bit Checksums | Best Practice | `src/prompt_engineering/` | ✅ Adopted |

---

## v2.1.0

| Source | Type | Module(s) | Status |
|--------|------|-----------|--------|
| Karger et al. (1997) — Consistent Hash Ring | Best Practice | `src/server/`, `src/sharding/` | ✅ Adopted |

---

## Planned (v2.x)

| Source | Type | Module(s) | Status |
|--------|------|-----------|--------|
| Raasveldt & Mühleisen (2019) — DuckDB | Paper | `src/query/`, `src/exporters/` | ⏳ Planned |
| Beurer-Kellner et al. (2023) — LMQL | Paper | `src/prompt_engineering/`, `src/llm/` | ⏳ Planned |

---

## Planned (Q2 2026)

| Source | Type | Module(s) | Status |
|--------|------|-----------|--------|
| Busch et al. (2023) — ProcessGPT | Paper | `src/process/`, `src/llm/` | ⏳ Planned |
| Gutierrez et al. (2024) — HippoRAG | Paper | `src/process/`, `src/rag/` | ⏳ Planned |

---

## Planned (Q3 2026)

| Source | Type | Module(s) | Status |
|--------|------|-----------|--------|
| Edge et al. (2024) — GraphRAG | Paper | `src/process/`, `src/rag/`, `src/graph/` | ⏳ Planned |

---

## Planned (Q1 2027)

| Source | Type | Module(s) | Status |
|--------|------|-----------|--------|
| Bukhsh et al. (2021) — ProcessTransformer | Paper | `src/process/`, `src/training/` | ⏳ Planned |

---

*Last generated: see git log*

