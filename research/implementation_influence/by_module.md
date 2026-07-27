# Implementation Influence — By Module

This file groups all research influences by ThemisDB module (`src/<module>`).  
*Generated from the master matrix in [README.md](README.md).*  
*Last enhanced: 2026-07-27 — top-risk modules (server, llm, sharding) expanded with full research-source → planned-capability → implementation-evidence mapping per Phase 6 governance rule.*

> **Top-risk modules** (`server`, `llm`, `sharding`) contain expanded tables with five columns: Category, Source, Planned Capability, Implementation Evidence, Version, Status. All other modules use the legacy four-column format until their next roadmap sync.

---

## Module Index

- [src/api/](#srcapi)
- [src/aql/](#srcaql)
- [src/analytics/](#srcanalytics)
- [src/auth/](#srcauth)
- [src/cache/](#srccache)
- [src/cdc/](#srccdc)
- [src/chimera/](#srcchimera)
- [src/config/](#srcconfig)
- [src/exporters/](#srcexporters)
- [src/geo/](#srcgeo)
- [src/governance/](#srcgovernance)
- [src/gpu/](#srcgpu)
- [src/graph/](#srcgraph)
- [src/importers/](#srcimporters)
- [src/index/](#srcindex)
- [src/ingestion/](#srcingestion)
- [src/llm/](#srcllm)
- [src/metadata/](#srcmetadata)
- [src/network/](#srcnetwork)
- [src/observability/](#srcobservability)
- [src/performance/](#srcperformance)
- [src/process/](#srcprocess)
- [src/prompt\_engineering/](#srcprompt_engineering)
- [src/query/](#srcquery)
- [src/rag/](#srcrag)
- [src/replication/](#srcreplication)
- [src/rpc\_grpc/](#srcrpc_grpc)
- [src/search/](#srcsearch)
- [src/security/](#srcsecurity)
- [src/server/](#srcserver)
- [src/sharding/](#srcsharding)
- [src/storage/](#srcstorage)
- [src/temporal/](#srctemporal)
- [src/timeseries/](#srctimeseries)
- [src/training/](#srctraining)
- [src/transaction/](#srctransaction)
- [src/vector/](#srcvector)
- [src/voice/](#srcvoice)
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
| Paper | Forgy (1982) — RETE Algorithm | v2.1.0+ planned | ⏳ Planned |
| Best Practice | Forgy / CLIPS / Drools — RETE Forward-Chaining Rule Engine | v2.1.0+ planned | ⏳ Planned |
| Best Practice | Hu et al. / vLLM — Multi-LoRA Adapter Routing | v2.1.0+ planned | ⏳ Planned |

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
| Paper | W3C / Baader et al. (2012/2003) — OWL 2 / Description Logic Handbook | v2.1.0+ planned | ⏳ Planned |
| Paper | Bordes et al. (2013) — TransE / Knowledge Graph Completion | v2.1.0+ planned | ⏳ Planned |
| Best Practice | W3C OWL 2 RL / Apache Jena — OWL-lite Ontology Constraints | v2.1.0+ planned | ⏳ Planned |
| Best Practice | Hu et al. / vLLM — Multi-LoRA Adapter Routing | v2.1.0+ planned | ⏳ Planned |
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

> **Top-risk module** (3,664 gaps, 1,245 CRITICAL — rescan baseline 2026-05-27). GA hardening Phase 1-6 complete 2026-07-27.  
> Mapping scope: research source → planned capability → implementation evidence.

| Category | Source | Planned Capability | Implementation Evidence | Version | Status |
|----------|--------|--------------------|------------------------|---------|--------|
| Paper | Vaswani et al. (2017) — Attention Is All You Need | Transformer inference backbone; context-window budget enforcement | `include/llm/`, `src/llm/`; `ContextWindowBudgetManager`; 190 files with Doxygen coverage | v1.3.0+ | ✅ Implemented |
| Paper | Hu et al. (2022) — LoRA | Low-rank adapter training and hot-swap serving | `include/training/incremental_lora_trainer.h`; `src/training/`; `exportGradient()` / `applyGlobalDelta()`; ILT-EG-01..03 tests | v1.3.0+ | ✅ Implemented |
| Paper | Dettmers et al. (2023) — QLoRA | 4-bit quantized fine-tuning; reduced GPU memory footprint | `include/training/`, `src/training/`; quantisation paths in LoRA pipeline | v1.3.0+ | ✅ Implemented |
| Paper | Es et al. (2023) — RAGAS | RAG quality evaluation (faithfulness, context relevance, answer relevance) | `include/rag/`; `tests/rag/test_rag_quality.cpp`; WISQ-01..05 in `test_wiki_rag_quality.cpp` | v1.3.0+ | ✅ Implemented |
| Paper | Dao et al. (2022) — FlashAttention (CUDA kernels) | Memory-efficient attention for long-context inference | CUDA kernel paths in `src/llm/`; acceleration integration | v1.4.0-alpha+ | ✅ Implemented |
| Paper | Chen et al. (2023) — Speculative Decoding | Draft-model assisted token generation; latency reduction | `src/llm/` speculative paths; LLM plugin manager hot-plug | v1.4.0-alpha+ | ✅ Implemented |
| Paper | Kwon et al. (2023) — PagedAttention (vLLM) | KV-cache paging; high-throughput concurrent request serving | `include/llm/`; `src/llm/`; cache management in plugin manager | v1.4.0-alpha+ | ✅ Implemented |
| Paper | Sheng et al. (2023) — S-LoRA: Concurrent LoRA Adapter Serving | Multi-LoRA concurrent serving; per-request adapter selection | `include/llm/`; `src/llm/llm_plugin_manager.cpp`; adapter routing | v2.1.0+ | 🔄 In Progress |
| Paper | Wang et al. (2024) — Speculative RAG | Speculation-driven retrieval augmentation; draft retrieval before full generation | `include/rag/`; RAG pipeline speculative paths | Q1 2027 | ⏳ Planned |
| Paper | Busch et al. (2023) — ProcessGPT | Process-aware LLM; database-native process reasoning | `include/process/process_agentic_rag.h`; `src/process/` | Q2 2026+ | 🔄 In Progress |
| Paper | Beurer-Kellner et al. (2023) — LMQL | Constrained LLM decoding; type-safe prompt programming | `include/llm/`; future constrained-decoding integration | v2.x | ⏳ Planned |
| **GA Hardening** | Phase 5-L Delivery (2026-07-20) | Exception safety, RAII lifecycle, memory-leak closure, race-condition fixes | P5-L01 (`tests/llm/`): 51 focused tests; P5-L02 exception-safe paths; `src/llm/ROADMAP.md` Phase 5-L block | v2.4.0-rc1 | ✅ GA-complete |
| **GA Hardening** | WikiIndexStore Phase B (2026-07-27) | Thread-safe KNN embedding; wiki-chunk secondary index; LLM-backed RAG source | `include/llm/wiki_index_store.h`; `src/llm/wiki_index_store.cpp`; WIS-B-01..16 tests | v2.4.0-rc1 | ✅ GA-complete |
| **GA Hardening** | LLM Module Doxygen (2026-07-19) | 100% @file header coverage across 190 files; maturity metadata per file | `src/llm/ROADMAP.md` Module Evidence Summary; 90 .cpp + 100 .h files verified | v2.4.0-rc1 | ✅ GA-complete |
| Architecture Decision | ADR-002 — RocksDB as Primary Storage Backend | Persistent embedding and model-metadata storage | `include/llm/`; RocksDB integration in LLM persistence paths | v1.0.0+ | ✅ Accepted |

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
| Paper | Madaan et al. (2023) — Self-Refine (NeurIPS 2023) | v1.5.0+ | ✅ Implemented |
| Paper | Shinn et al. (2023) — Reflexion (NeurIPS 2023) | v1.5.0+ | ✅ Implemented |
| Paper | Pryzant et al. (2023) — ProTeGi (EMNLP 2023) | v2.0.0+ | ✅ Implemented |
| Paper | Yao et al. (2023) — Tree of Thoughts (NeurIPS 2023 Spotlight) | v2.0.0+ | ✅ Implemented |
| Paper | Khattab et al. (2023/2024) — DSPy (ICLR 2024 Oral) | v2.0.0+ (decl); v2.2.0 (compiler) | 🔄 Partially Implemented |
| Paper | Beurer-Kellner et al. (2023) — LMQL | planned v2.x | ⏳ Planned |
| Best Practice | Austin Appleby — MurmurHash3 Sharding | v1.9.0 | ✅ Adopted |
| Best Practice | FNV — FNV-1a 64-bit Checksums | v2.0.0 | ✅ Adopted |
| Best Practice | ProTeGi + Self-Refine + ToT + DSPy — LLM Prompt Enhancement Pipeline | v1.5.0+/v2.0.0+ | ✅ Adopted |

---

## src/query/

| Category | Source | Version | Status |
|----------|--------|---------|--------|
| Paper | Kulkarni & Michels (2012) — SQL:2011 Temporal Features | v1.x+ | 🔄 Partial |
| Paper | Raasveldt & Mühleisen (2019) — DuckDB | planned v2.x | ⏳ Planned |
| Paper | Marcus et al. (2021) — Bao: Learned Query Optimization | planned v2.0.0 | 🔄 In Progress |
| Paper | Zhou et al. (2022) — AI Meets Database (AI4DB) | v2.0.0+ framework | 🔄 In Progress |
| Best Practice | AI-Driven Query Optimization | v2.0.0+ | 🔄 Partially Adopted |
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
| Paper | Sheng et al. (2023) — S-LoRA: Concurrent LoRA Adapter Serving | planned Q2/Q3 2026 | 🔄 In Progress |
| Paper | Wang et al. (2024) — Speculative RAG | planned Q1 2027 | ⏳ Planned |
| Paper | W3C / Baader et al. (2012/2003) — OWL 2 / Description Logic Handbook | v2.1.0+ planned | ⏳ Planned |
| Paper | Liu et al. (2023) — G-Eval: NLG Evaluation with GPT-4 (EMNLP 2023) | v1.6.0+ | ✅ Implemented |
| Paper | Zheng et al. (2023) — Judging LLM-as-a-Judge / MT-Bench (NeurIPS 2023) | v1.6.0+ | ✅ Implemented |
| Paper | Yao et al. (2022) — ReAct: Synergizing Reasoning and Acting (ICLR 2023) | v1.8.0+ | ✅ Implemented |
| Paper | Bai et al. (2022) + Lee et al. (2023) — Constitutional AI / RLAIF | v1.6.0+ | ✅ Implemented |
| Best Practice | G-Eval + LLM-as-Judge Ensemble + Calibration | v1.6.0+ | ✅ Adopted |
| Best Practice | Constitutional AI / RLAIF Training Pipeline | v1.6.0+ | ✅ Adopted |
| Best Practice | S-LoRA Near-Realtime RAG Serving | planned Q2 2026 | 🔄 Partially Adopted |
| Paper | Bordes et al. (2013) — TransE / Knowledge Graph Completion | v2.1.0+ planned | ⏳ Planned |
| Best Practice | W3C OWL 2 RL — OWL-lite Ontology Constraints | v2.1.0+ planned | ⏳ Planned |
| Best Practice | Hu et al. / vLLM — Multi-LoRA Adapter Routing | v2.1.0+ planned | ⏳ Planned |
| Architecture Decision | ADR-002 — RocksDB as Primary Storage Backend | v1.0.0+ | ✅ Accepted |

---

## src/rpc_grpc/

| Category | Source | Version | Status |
|----------|--------|---------|--------|
| Architecture Decision | ADR-007 — gRPC + Protobuf for Internal RPC | v1.0.0+ | ✅ Accepted |

---

## src/server/

> **Top-risk module** (high gap count, release-critical serving path). GA hardening Phase 5-S complete 2026-07-20.  
> Mapping scope: research source → planned capability → implementation evidence.

| Category | Source | Planned Capability | Implementation Evidence | Version | Status |
|----------|--------|--------------------|------------------------|---------|--------|
| Best Practice | Herb Sutter (GotW #24) — PIMPL Idiom | Stable ABI for plugin/adapter boundaries; reduce recompilation | `include/server/http_server.h`; PIMPL in core server types | v1.9.0 | ✅ Adopted |
| Best Practice | RFC 6585 — Token Bucket Rate Limiting | Per-client request throttling; DoS protection on public endpoints | `include/server/`; rate-limiter in HTTP handler pipeline | v1.6.0 | ✅ Adopted |
| Best Practice | CNCF OpenTelemetry Spec — Span Instrumentation | End-to-end distributed trace per request; latency breakdown by handler | `src/server/`; OTel span injection in request lifecycle | v1.9.0 | ✅ Adopted |
| Best Practice | RFC 7519 + RFC 6749 — JWT Short-Lived Tokens | Stateless auth with short-lived bearer tokens; OAuth2 PKCE flow | `include/auth/`; `src/server/`; JWT middleware in request pipeline | v1.6.0 | ✅ Adopted |
| Best Practice | NIST SP 800-52 Rev 2 — TLS 1.3 Cipher Hardening | TLS 1.3-only enforcement; AES-256-GCM / ChaCha20-Poly1305 cipher selection | `src/server/` TLS config; cipher suite enforcement | v1.0.0+ | ✅ Adopted |
| Best Practice | Karger et al. (1997) — Consistent Hash Ring | Load-balanced request routing to shard replicas | `include/server/`; consistent-hash router in cluster dispatch | v2.1.0 | ✅ Adopted |
| Best Practice | Boost.Asio Docs — Proactor Async I/O | Non-blocking async request handling; high connection concurrency | `src/server/`; Boost.Asio event loop in HTTP/RPC server | v1.0.0+ | ✅ Adopted |
| Best Practice | NET RFC 7230 / HTTP/2 RFC 7540 — Graceful Shutdown | Drain in-flight requests on SIGTERM; configurable drain timeout | `include/server/http_server.h`; graceful-shutdown handler | v2.4.0-rc1 | ✅ Adopted |
| Best Practice | Retry with Exponential Backoff (Google SRE Book §22) | Wire-protocol retry with jitter; bounded retry budget; backoff caps | `include/network/wire_protocol_connection_pool.h`; P5-S01 delivery | v2.4.0-rc1 | ✅ Adopted |
| Architecture Decision | ADR-003 — Boost.Beast + Asio for HTTP Server | Unified async HTTP/WS server; single event-loop model | `src/server/`; Beast HTTP server core | v1.0.0+ | ✅ Accepted |
| Architecture Decision | ADR-007 — gRPC + Protobuf for Internal RPC | Type-safe internal RPC; language-agnostic service contracts | `src/server/`; gRPC handler registration | v1.0.0+ | ✅ Accepted |
| Architecture Decision | ADR-008 — JWT + OAuth2 PKCE for API Auth | Stateless API authentication; refresh-token rotation | `include/auth/`; `src/server/`; auth middleware wiring | v1.6.0+ | ✅ Accepted |
| **GA Hardening** | Phase 5-S Delivery (2026-07-20) | Retry/backoff hardening; HTTP timeout patterns; graceful-shutdown fault closure | P5-S01 / P5-S02: `tests/server/`; 39 new focused tests; `src/server/ROADMAP.md` Phase 5-S block | v2.4.0-rc1 | ✅ GA-complete |
| **Next Phase** | HTTP/3 QUIC enablement (Phase 3.5) | Zero-RTT reconnect; multiplexed streams; reduced head-of-line blocking | `include/network/`; QUIC transport integration (lsquic or quiche) | Q3 2026 | ⏳ Planned |
| **Next Phase** | Zero-copy socket I/O (Phase 3.5) | `sendfile()`/`io_uring` for large payload transfers; reduced CPU overhead | `src/server/`; `src/network/`; io_uring backend | Q4 2026 | ⏳ Planned |

---

## src/sharding/

> **Top-risk module** (2,051 gaps, 696 CRITICAL — rescan baseline). GA hardening Phase 6 complete 2026-07-22.  
> Mapping scope: research source → planned capability → implementation evidence.

| Category | Source | Planned Capability | Implementation Evidence | Version | Status |
|----------|--------|--------------------|------------------------|---------|--------|
| Best Practice | Karger et al. (1997) — Consistent Hash Ring | Minimal-disruption partition rebalancing on node join/leave | `include/sharding/`; consistent-hash ring in shard router | v2.1.0 | ✅ Adopted |
| Best Practice | Austin Appleby — MurmurHash3 Sharding | Uniform key distribution; low-collision shard key hashing | `include/sharding/`; MurmurHash3 key hasher | v1.9.0 | ✅ Adopted |
| Paper | Tarjan (1972) — SCC / Wait-For Graph Cycle Detection | Distributed deadlock detection via wait-for graph SCC scan | `include/sharding/`; `src/sharding/`; WFG cycle-detection algorithm | v2.2.0 | ✅ Implemented |
| Paper | Gray & Lamport (2006) — Consensus on Transaction Commit (2PC) | Two-phase commit for cross-shard atomic transactions | `include/sharding/transaction_wal.h`; `src/sharding/`; P6-01 TXC-01..TXC-32 | v2.4.0-rc1 | ✅ Implemented |
| Paper | Skeen (1981) — Non-Blocking Commit Protocol (3PC) | Three-phase commit; avoids coordinator blocking on crash | `include/sharding/`; `src/sharding/`; 2PC/3PC protocol tagging in WAL | v2.4.0-rc1 | ✅ Implemented |
| Paper | Garcia-Molina & Salem (1987) — SAGAS | Long-running distributed transactions with compensating actions | `include/sharding/`; SAGA protocol implementation; `docs/architecture/transaction_coordinators.md` | v2.4.0-rc1 | ✅ Implemented |
| Paper | Brewer (2000) — CAP Theorem; Gilbert & Lynch (2002) — CAP Proof | Explicit CAP trade-off documentation per consistency mode; partition tolerance design | `src/sharding/ROADMAP.md`; consistency-mode configuration | v1.9.0+ | ✅ Adopted |
| Paper | Lamport (1998) — Paxos Made Simple | Consensus leader election for shard coordinator failover | `include/replication/`; `src/replication/`; Paxos-derived leader election | v2.0.0+ | 🔄 In Progress |
| Paper | Ongaro & Ousterhout (2014) — Raft Consensus | Replicated log; membership changes via joint-consensus | `include/replication/raft_v2.h`; `src/replication/raft_v2.cpp`; JOINT→COMMIT WAL entries | v2.4.0-rc1 | ✅ Implemented |
| Architecture Decision | ADR-010 — Distributed Deadlock Detection via WFG | WFG-based deadlock detection as mandatory conflict resolution step | `include/sharding/`; `src/sharding/`; WFG integration tests | v2.2.0 | ✅ Accepted |
| **GA Hardening** | Phase 6 Delivery (2026-07-22) | 2PC/3PC consistency hardening; fault-injection recovery; cross-shard WAL atomicity | P6-01/TXC-01..TXC-32 + P6-02/FLR-01..FLR-20 + P6-03/FI-01..FI-40; `tests/sharding/test_sharding_phase6_hardening.cpp`; label=`release_critical;sharding_p6` | v2.4.0-rc1 | ✅ GA-complete |
| **Next Phase** | Automatic shard rebalancing (Phase 3.2) | Topology-change-driven partition redistribution; zero-downtime rebalance | `include/sharding/`; rebalancer module (planned) | Q3 2026 | ⏳ Planned |
| **Next Phase** | Cross-datacenter shard placement (Phase 3.2) | Latency-aware replica placement policies; geo-affinity routing | `include/sharding/`; `include/replication/`; placement policy API | Q4 2026 | ⏳ Planned |
| **Next Phase** | Global secondary indexes across shards (Phase 3.2) | Index entries federated across shard boundaries; consistent cross-shard index read | `include/index/`; `include/sharding/`; distributed index coordinator | Q4 2026 | ⏳ Planned |

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

## src/api/

| Category | Source | Version | Status |
|----------|--------|---------|--------|
| Paper | Fielding (2000) — Architectural Styles (REST Dissertation) | v1.0.0+ | ✅ Adopted |
| Standard | Belshe, Peon & Thomson (2015) — RFC 7540 HTTP/2 | v1.0.0+ | ✅ Adopted |
| Standard | Fette & Melnikov (2011) — RFC 6455 WebSocket Protocol | v1.0.0+ | ✅ Adopted |
| Standard | Sporny et al. (2021) — W3C JSON-LD 1.1 | v1.5.0+ | 🔄 Partial |
| Architecture Decision | ADR-003 — Boost.Beast + Asio for HTTP Server | v1.0.0+ | ✅ Accepted |
| Architecture Decision | ADR-007 — gRPC + Protobuf for Internal RPC | v1.0.0+ | ✅ Accepted |
| Architecture Decision | ADR-008 — JWT + OAuth2 PKCE for API Auth | v1.6.0+ | ✅ Accepted |

---

## src/cdc/

| Category | Source | Version | Status |
|----------|--------|---------|--------|
| Paper | Stonebraker, Rowe & Hirohama (1990) — Implementation of Postgres (WAL/CDC) | v1.0.0+ | ✅ Implemented |
| Paper | Mohan et al. (1992) — ARIES Write-Ahead Logging | v1.0.0+ | ✅ Implemented |
| Book | Kleppmann (2017) — Designing Data-Intensive Applications | v1.0.0+ | ✅ Adopted |
| Paper | Flink Community (2015) — Apache Flink Stream Processing | v1.8.0+ | 🔄 Partial |
| Architecture Decision | ADR-002 — RocksDB as Primary Storage Backend | v1.0.0+ | ✅ Accepted |

---

## src/exporters/

| Category | Source | Version | Status |
|----------|--------|---------|--------|
| Paper | Abadi et al. (2013) — Design and Implementation of Modern Column-Oriented Databases | v1.0.0+ | ✅ Implemented |
| Standard | Apache Arrow Community (2016) — Apache Arrow Cross-Language In-Memory Format | v1.0.0+ | ✅ Adopted |
| Book | Vohra (2016) — Apache Parquet | v1.0.0+ | ✅ Adopted |
| Paper | Raasveldt & Mühleisen (2019) — DuckDB | planned v2.x | ⏳ Planned |

---

## src/geo/

| Category | Source | Version | Status |
|----------|--------|---------|--------|
| Paper | Guttman (1984) — R-Trees: A Dynamic Index Structure for Spatial Searching | v1.0.0+ | ✅ Implemented |
| Paper | Haverkort & van Walderveen (2008) — Locality and Bounding-Box Quality of Space-Filling Curves | v1.0.0+ | ✅ Implemented |
| Standard | Open Geospatial Consortium (2010) — OGC Simple Feature Access 1.2.1 | v1.0.0+ | ✅ Adopted |
| Paper | Beckmann et al. (1990) — R*-Tree: An Efficient and Robust Access Method | v1.4.0+ | 🔄 Partial |

---

## src/governance/

| Category | Source | Version | Status |
|----------|--------|---------|--------|
| Standard | European Parliament (2016) — GDPR Regulation (EU) 2016/679 | v1.0.0+ | ✅ Adopted |
| Standard | NIST (2020) — SP 800-53 Rev. 5 Security and Privacy Controls | v1.0.0+ | ✅ Adopted |
| Book | Abiteboul, Hull & Vianu (1995) — Foundations of Databases | v1.0.0+ | 🔄 Partial |
| Standard | ISO/IEC 27001:2022 — Information Security Management Systems | v1.6.0+ | 🔄 Partial |

---

## src/gpu/

| Category | Source | Version | Status |
|----------|--------|---------|--------|
| Paper | Nickolls et al. (2008) — Scalable Parallel Programming with CUDA | v1.4.0-alpha+ | ✅ Implemented |
| Paper | Ryoo et al. (2008) — Optimization Principles and Application Performance on GPU | v1.4.0-alpha+ | ✅ Implemented |
| Paper | Dao et al. (2022) — FlashAttention (CUDA memory-efficient attention) | v1.4.0-alpha+ | ✅ Implemented |
| Standard | NVIDIA Corporation (2023) — CUDA C++ Programming Guide v12.x | v1.4.0-alpha+ | ✅ Adopted |
| Standard | NVIDIA Corporation (2019) — RAPIDS: Open GPU Data Science (cuDF/cuML) | v1.9.0+ | 🔄 Partial |

---

## src/importers/

| Category | Source | Version | Status |
|----------|--------|---------|--------|
| Paper | Vassiliadis, Simitsis & Skiadopoulos (2002) — Conceptual Modeling for ETL Processes | v1.0.0+ | ✅ Implemented |
| Book | Kimball & Caserta (2004) — The Data Warehouse ETL Toolkit | v1.0.0+ | ✅ Adopted |
| Paper | Stonebraker et al. (2013) — Data Curation at Scale: The Data Tamer System | v1.5.0+ | 🔄 Partial |
| Standard | FITKO (2024) / BMI (2017) — OZG, FIM, XÖV Standards | v1.9.0+ | 🔄 Partial |

---

## src/ingestion/

| Category | Source | Version | Status |
|----------|--------|---------|--------|
| Paper | Zaharia et al. (2013) — Discretized Streams: Fault-Tolerant Streaming Computation | v1.0.0+ | ✅ Implemented |
| Book | Kleppmann (2017) — Designing Data-Intensive Applications | v1.0.0+ | ✅ Adopted |
| Paper | Karp, Shenker & Papadimitriou (2003) — Finding Frequent Elements in Streams | v1.5.0+ | 🔄 Partial |
| Paper | Flink Community (2015) — Apache Flink Stream Processing | v1.8.0+ | 🔄 Partial |

---

## src/metadata/

| Category | Source | Version | Status |
|----------|--------|---------|--------|
| Standard | ISO/IEC 11179-1:2013 — Information Technology Metadata Registries (MDR) | v1.0.0+ | ✅ Adopted |
| Standard | W3C (2013) — PROV-O: The PROV Ontology | v1.5.0+ | 🔄 Partial |
| Paper | Bernstein & Melnik (2007) — Model Management 2.0: Manipulating Richer Mappings | v1.6.0+ | 🔄 Partial |
| Standard | Dublin Core Metadata Initiative (2020) — DCMI Metadata Terms | v1.0.0+ | ✅ Adopted |

---

## src/network/

| Category | Source | Version | Status |
|----------|--------|---------|--------|
| Standard | Postel (1981) — RFC 793 Transmission Control Protocol | v1.0.0+ | ✅ Adopted |
| Standard | Rescorla (2018) — RFC 8446 TLS Protocol Version 1.3 | v1.0.0+ | ✅ Adopted |
| Standard | Belshe, Peon & Thomson (2015) — RFC 7540 HTTP/2 | v1.0.0+ | ✅ Adopted |
| Best Practice | NIST SP 800-52 Rev 2 — TLS 1.3 Cipher Hardening | v1.0.0+ | ✅ Adopted |

---

## src/observability/

| Category | Source | Version | Status |
|----------|--------|---------|--------|
| Standard | OpenTelemetry Authors (2021) — OpenTelemetry Specification (CNCF) | v1.0.0+ | ✅ Adopted |
| Paper | Sigelman et al. (2010) — Dapper, a Large-Scale Distributed Systems Tracing Infrastructure | v1.0.0+ | ✅ Implemented |
| Standard | W3C Distributed Tracing WG (2021) — Trace Context Level 1 | v1.0.0+ | ✅ Adopted |
| Standard | OpenMetrics / Prometheus (2021) — OpenMetrics Specification 1.0.0 | v1.0.0+ | ✅ Adopted |

---

## src/performance/

| Category | Source | Version | Status |
|----------|--------|---------|--------|
| Paper | Leis, Kemper & Neumann (2013) — The Adaptive Radix Tree (ART) | v1.0.0+ | ✅ Implemented |
| Paper | Ding et al. (2020) — Tsunami: A Learned Multi-dimensional Index | planned v2.x | ⏳ Planned |
| Paper | Faleiro & Abadi (2017) — Latch-free Synchronization in Database Systems | v1.8.0+ | ✅ Adopted |
| Best Practice | Martin Thompson — Mechanical Sympathy (CPU cache-friendly data layouts) | v1.9.0 | ✅ Adopted |

---

## src/process/

| Category | Source | Version | Status |
|----------|--------|---------|--------|
| Book | van der Aalst (2016) — Process Mining: Data Science in Action | v1.9.0+ | 🔄 Partial |
| Standard | Object Management Group (2013) — BPMN 2.0.2 Specification | v1.9.0+ | ✅ Adopted |
| Paper | Busch et al. (2023) — ProcessGPT | planned Q2 2026 | ⏳ Planned |
| Paper | Bukhsh et al. (2021) — ProcessTransformer | planned Q1 2027 | ⏳ Planned |
| Paper | Edge et al. (2024) — GraphRAG | planned Q3 2026 | ⏳ Planned |
| Standard | FITKO (2024) / BMI (2017) — OZG, FIM, XÖV Standards | v1.9.0+ | 🔄 Partial |

---

## src/replication/

| Category | Source | Version | Status |
|----------|--------|---------|--------|
| Paper | Ongaro & Ousterhout (2014) — In Search of an Understandable Consensus Algorithm (Raft) | v1.0.0+ | ✅ Implemented |
| Paper | Lamport (1998) — The Part-Time Parliament (Paxos) | v1.0.0+ | ✅ Adopted |
| Paper | Gilbert & Lynch (2002) — Brewer's Conjecture and CAP Theorem | v1.0.0+ | ✅ Adopted |
| Paper | DeCandia et al. (2007) — Dynamo: Amazon's Highly Available Key-Value Store | v1.5.0+ | 🔄 Partial |

---

## src/search/

| Category | Source | Version | Status |
|----------|--------|---------|--------|
| Paper | Robertson & Zaragoza (2009) — The Probabilistic Relevance Framework: BM25 and Beyond | v1.0.0+ | ✅ Implemented |
| Paper | Vaswani et al. (2017) — Attention Is All You Need | v1.3.0+ | ✅ Implemented |
| Paper | Karpukhin et al. (2020) — Dense Passage Retrieval (DPR) | v1.2.0+ | ✅ Implemented |
| Paper | Cormack, Clarke & Buettcher (2009) — Reciprocal Rank Fusion (RRF) | v1.5.0+ | ✅ Implemented |

---

## src/security/

| Category | Source | Version | Status |
|----------|--------|---------|--------|
| Paper | Diffie & Hellman (1976) — New Directions in Cryptography | v1.0.0+ | ✅ Adopted |
| Standard | NIST (2001) — FIPS 197 Advanced Encryption Standard (AES) | v1.0.0+ | ✅ Adopted |
| Paper | Rivest, Shamir & Adleman (1978) — RSA Public-Key Cryptosystem | v1.0.0+ | ✅ Adopted |
| Standard | NIST SP 800-52 Rev 2 — TLS 1.3 Cipher Hardening | v1.0.0+ | ✅ Adopted |
| Best Practice | CERT C MSC06-C — Secure Key Zeroing | v0.1.0 | ✅ Adopted |
| Architecture Decision | ADR-005 — Argon2id over scrypt/bcrypt | v0.1.0 | ✅ Accepted |

---

## src/storage/

| Category | Source | Version | Status |
|----------|--------|---------|--------|
| Paper | O'Neil et al. (1996) — The Log-Structured Merge-Tree (LSM-tree) | v1.0.0+ | ✅ Implemented |
| Paper | Rosenblum & Ousterhout (1992) — Design and Implementation of a Log-Structured File System | v1.0.0+ | ✅ Adopted |
| Paper | Dong et al. (2017) — Optimizing Space Amplification in RocksDB | v1.0.0+ | ✅ Adopted |
| Paper | Zhou et al. (2022) — AI Meets Database (AI4DB) — index advisor framework | v1.9.0+ | ✅ Implemented |
| Paper | ThemisDB Engineering (2026) — LLM-Driven Index Advisor (IIndexAnalysisAdvisor + StorageLayoutAdvisor) | v1.9.0+ | ✅ Implemented |
| Best Practice | RocksDB Docs — WriteBatch Atomicity | v1.0.0+ | ✅ Adopted |
| Best Practice | AI4DB + ISUM — LLM-Driven Index Optimization | v1.9.0+ | ✅ Adopted |
| Architecture Decision | ADR-002 — RocksDB as Primary Storage Backend | v1.0.0+ | ✅ Accepted |

---

## src/timeseries/

| Category | Source | Version | Status |
|----------|--------|---------|--------|
| Paper | Pelkonen et al. (2015) — Gorilla: A Fast, Scalable, In-Memory Time Series Database | v1.0.0+ | ✅ Implemented |
| Paper | Elias (1975) — Universal Codeword Sets and Representations of the Integers | v1.0.0+ | ✅ Implemented |
| Paper | Ding et al. (2015) — YADING: Fast Clustering of Large-Scale Time Series Data | v1.5.0+ | 🔄 Partial |
| Paper | Keogh & Ratanamahatana (2005) — Exact Indexing of Dynamic Time Warping | planned v2.x | ⏳ Planned |

---

## src/training/

| Category | Source | Version | Status |
|----------|--------|---------|--------|
| Paper | Hu et al. (2022) — LoRA: Low-Rank Adaptation of Large Language Models | v1.3.0+ | ✅ Implemented |
| Paper | Howard & Ruder (2018) — Universal Language Model Fine-Tuning (ULMFiT) | v1.3.0+ | ✅ Implemented |
| Paper | Radford et al. (2018) — GPT-1: Improving Language Understanding by Generative Pre-Training | v1.3.0+ | ✅ Adopted |
| Paper | Dettmers et al. (2023) — QLoRA: Efficient Finetuning of Quantized LLMs | v1.3.0+ | ✅ Implemented |

---

## src/transaction/

| Category | Source | Version | Status |
|----------|--------|---------|--------|
| Book | Gray & Reuter (1992) — Transaction Processing: Concepts and Techniques | v1.0.0+ | ✅ Implemented |
| Book | Bernstein, Hadzilacos & Goodman (1987) — Concurrency Control and Recovery | v1.0.0+ | ✅ Adopted |
| Paper | Kung & Robinson (1981) — On Optimistic Methods for Concurrency Control (OCC) | v1.0.0+ | ✅ Implemented |
| Paper | Garcia-Molina & Salem (1987) — Sagas (distributed long-running transactions) | v1.5.0+ | ✅ Implemented |
| Paper | Herlihy & Wing (1990) — Linearizability: A Correctness Condition for Concurrent Objects | v1.0.0+ | ✅ Adopted |
| Paper | Mohan et al. (1992) — ARIES Write-Ahead Logging | v1.0.0+ | ✅ Implemented |

---

## src/voice/

| Category | Source | Version | Status |
|----------|--------|---------|--------|
| Paper | Radford et al. (2023) — Robust Speech Recognition via Large-Scale Weak Supervision (Whisper) | v1.9.0+ | 🔄 Partial |
| Paper | Graves, Mohamed & Hinton (2013) — Speech Recognition with Deep Recurrent Neural Networks | v1.9.0+ | 🔄 Partial |
| Paper | Bahdanau, Cho & Bengio (2015) — Neural Machine Translation by Jointly Learning to Align | v1.9.0+ | 🔄 Partial |
| Paper | Devlin et al. (2019) — BERT: Pre-training of Deep Bidirectional Transformers | v1.9.0+ | 🔄 Partial |

---

*Last generated: see git log*

