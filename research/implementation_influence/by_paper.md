# Implementation Influence — By Paper / Source

This file lists all research sources alphabetically, showing which ThemisDB modules each one influences.  
*Generated from the master matrix in [README.md](README.md).*

---

## ADR-001 — HNSW over FAISS for ANN Vector Index

**Type:** Architecture Decision  
**File:** [architecture_decisions/adr_001_hnsw_over_faiss_vector_index.md](../architecture_decisions/adr_001_hnsw_over_faiss_vector_index.md)  
**ThemisDB Version:** v1.0.0+

| Module | Status |
|--------|--------|
| `src/index/` | ✅ Accepted |

---

## ADR-002 — RocksDB as Primary Storage Backend

**Type:** Architecture Decision  
**File:** [architecture_decisions/adr_002_rocksdb_storage_backend.md](../architecture_decisions/adr_002_rocksdb_storage_backend.md)  
**ThemisDB Version:** v1.0.0+

| Module | Status |
|--------|--------|
| `src/cache/` | ✅ Accepted |
| `src/index/` | ✅ Accepted |
| `src/rag/` | ✅ Accepted |

---

## ADR-003 — Boost.Beast + Asio for HTTP Server

**Type:** Architecture Decision  
**File:** [architecture_decisions/adr_003_boost_beast_asio_http_server.md](../architecture_decisions/adr_003_boost_beast_asio_http_server.md)  
**ThemisDB Version:** v1.0.0+

| Module | Status |
|--------|--------|
| `src/server/` | ✅ Accepted |

---

## ADR-004 — Native Multi-Model Data Model

**Type:** Architecture Decision  
**File:** [architecture_decisions/adr_004_multi_model_data_model.md](../architecture_decisions/adr_004_multi_model_data_model.md)  
**ThemisDB Version:** v1.0.0+

| Module | Status |
|--------|--------|
| `src/aql/` | ✅ Accepted |
| `src/graph/` | ✅ Accepted |
| `src/index/` | ✅ Accepted |
| `src/query/` | ✅ Accepted |

---

## ADR-005 — Argon2id over scrypt/bcrypt

**Type:** Architecture Decision  
**File:** [architecture_decisions/adr_005_argon2id_over_scrypt_bcrypt.md](../architecture_decisions/adr_005_argon2id_over_scrypt_bcrypt.md)  
**ThemisDB Version:** v0.1.0

| Module | Status |
|--------|--------|
| `plugins/user_storage_encrypted/` | ✅ Accepted |

---

## ADR-006 — Plugin Adapter Architecture (Chimera)

**Type:** Architecture Decision  
**File:** [architecture_decisions/adr_006_plugin_chimera_adapter_architecture.md](../architecture_decisions/adr_006_plugin_chimera_adapter_architecture.md)  
**ThemisDB Version:** v1.0.0+

| Module | Status |
|--------|--------|
| `src/chimera/` | ✅ Accepted |

---

## ADR-007 — gRPC + Protobuf for Internal RPC

**Type:** Architecture Decision  
**File:** [architecture_decisions/adr_007_grpc_for_internal_rpc.md](../architecture_decisions/adr_007_grpc_for_internal_rpc.md)  
**ThemisDB Version:** v1.0.0+

| Module | Status |
|--------|--------|
| `src/rpc_grpc/` | ✅ Accepted |
| `src/server/` | ✅ Accepted |

---

## ADR-008 — JWT + OAuth2 PKCE for API Auth

**Type:** Architecture Decision  
**File:** [architecture_decisions/adr_008_jwt_oauth2_for_api_auth.md](../architecture_decisions/adr_008_jwt_oauth2_for_api_auth.md)  
**ThemisDB Version:** v1.6.0+

| Module | Status |
|--------|--------|
| `src/server/` | ✅ Accepted |
| `src/auth/` | ✅ Accepted |

---

## Appleby, Austin — MurmurHash3 Sharding

**Type:** Best Practice  
**File:** [best_practices/murmur_hash_deterministic_sharding.md](../best_practices/murmur_hash_deterministic_sharding.md)  
**ThemisDB Version:** v1.9.0

| Module | Status |
|--------|--------|
| `src/prompt_engineering/` | ✅ Adopted |
| `src/sharding/` | ✅ Adopted |

---

## AWS Builder's Library — Exponential Backoff + Circuit Breaker

**Type:** Best Practice  
**File:** [best_practices/exponential_backoff_retry.md](../best_practices/exponential_backoff_retry.md)  
**ThemisDB Version:** v1.8.0

| Module | Status |
|--------|--------|
| `src/chimera/` | ✅ Adopted |

---

## Baader et al. (2003/2007) / W3C (2012) — OWL 2 / Description Logic Handbook

**Type:** Paper + Standard  
**File:** [papers/owl2_description_logics_2012.md](../papers/owl2_description_logics_2012.md)  
**ThemisDB Version:** v2.1.0+ planned

| Module | Status |
|--------|--------|
| `src/graph/` — OntologyManager, PathConstraints, KnowledgeGraphReasoner | ⏳ Planned Q3 2026 |
| `src/rag/` — OntologyAwareRetriever, KnowledgeGraphRetriever | ⏳ Planned Q4 2026 |
| `src/analytics/` — KnowledgeBase (RDF Triple Format) | ⏳ Planned Q2 2027 |

---

## Bordes et al. (2013) — TransE / Knowledge Graph Completion

**Type:** Paper  
**File:** [papers/bordes_transe_2013.md](../papers/bordes_transe_2013.md)  
**ThemisDB Version:** v2.1.0+ planned

| Module | Status |
|--------|--------|
| `src/graph/` — KnowledgeGraphReasoner::applyLoRAScore() (KGE-Baseline) | ⏳ Planned Q4 2026 |
| `src/rag/` — OntologyAwareRetriever (Entity-Embedding-Suche) | ⏳ Planned Q4 2026 |
| `src/analytics/` — LoRAPatternClassifier (neuronales KGE-Äquivalent) | ⏳ Planned Q3 2027 |

---



**Type:** Paper  
**File:** [papers/lmql_beurer_kellner_2023.md](../papers/lmql_beurer_kellner_2023.md)  
**ThemisDB Version:** planned v2.x

| Module | Status |
|--------|--------|
| `src/prompt_engineering/` | ⏳ Planned |
| `src/llm/` | ⏳ Planned |

---

## Boost.Asio Docs — Proactor Async I/O

**Type:** Best Practice  
**File:** [best_practices/boost_asio_async_io.md](../best_practices/boost_asio_async_io.md)  
**ThemisDB Version:** v1.0.0+

| Module | Status |
|--------|--------|
| `src/server/` | ✅ Adopted |

---

## Brown et al. (2020) — GPT-3 Few-Shot Learning

**Type:** Paper  
**File:** [LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md](../LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md)  
**ThemisDB Version:** v1.2.0+

| Module | Status |
|--------|--------|
| `src/prompt_engineering/` | ✅ Implemented |

---

## Bukhsh et al. (2021) — ProcessTransformer

**Type:** Paper  
**File:** [papers/processtransformer_bukhsh_2021.md](../papers/processtransformer_bukhsh_2021.md)  
**ThemisDB Version:** planned Q1 2027

| Module | Status |
|--------|--------|
| `src/process/` | ⏳ Planned |
| `src/training/` | ⏳ Planned |

---

## Busch et al. (2023) — ProcessGPT

**Type:** Paper  
**File:** [papers/processgpt_busch_2023.md](../papers/processgpt_busch_2023.md)  
**ThemisDB Version:** planned Q2 2026

| Module | Status |
|--------|--------|
| `src/process/` | ⏳ Planned |
| `src/llm/` | ⏳ Planned |

---

## C++17 §30.6.5 — std::shared_mutex R/W Locks

**Type:** Best Practice  
**File:** [best_practices/shared_mutex_read_write_locks.md](../best_practices/shared_mutex_read_write_locks.md)  
**ThemisDB Version:** v1.8.0+

| Module | Status |
|--------|--------|
| `src/cache/` | ✅ Adopted |
| `src/config/` | ✅ Adopted |

---

## CERT C MSC06-C — Secure Key Zeroing

**Type:** Best Practice  
**File:** [best_practices/secure_key_zeroing.md](../best_practices/secure_key_zeroing.md)  
**ThemisDB Version:** v0.1.0

| Module | Status |
|--------|--------|
| `plugins/user_storage_encrypted/` | ✅ Adopted |

---

## Chen et al. (2023) — Speculative Decoding

**Type:** Paper  
**File:** [LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md](../LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md)  
**ThemisDB Version:** v1.4.0-alpha+

| Module | Status |
|--------|--------|
| `src/llm/` | ✅ Implemented |

---

## CNCF OpenTelemetry Spec — Span Instrumentation

**Type:** Best Practice  
**File:** [best_practices/opentelemetry_tracing.md](../best_practices/opentelemetry_tracing.md)  
**ThemisDB Version:** v1.9.0

| Module | Status |
|--------|--------|
| `src/server/` (all 64 handlers) | ✅ Adopted |

---

## Dao et al. (2022) — FlashAttention

**Type:** Paper  
**File:** [LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md](../LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md)  
**ThemisDB Version:** v1.4.0-alpha+

| Module | Status |
|--------|--------|
| `src/llm/` (CUDA kernels) | ✅ Implemented |

---

## Dettmers et al. (2023) — QLoRA

**Type:** Paper  
**File:** [LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md](../LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md)  
**ThemisDB Version:** v1.3.0+

| Module | Status |
|--------|--------|
| `src/llm/lora/` | ✅ Implemented |

---

## Devlin et al. (2019) — BERT

**Type:** Paper  
**File:** [LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md](../LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md)  
**ThemisDB Version:** v1.0.0+

| Module | Status |
|--------|--------|
| `src/vector/` | ✅ Implemented |
| `src/rag/` | ✅ Implemented |

---

## Edge et al. (2024) — GraphRAG

**Type:** Paper  
**File:** [papers/graphrag_edge_2024.md](../papers/graphrag_edge_2024.md)  
**ThemisDB Version:** planned Q3 2026

| Module | Status |
|--------|--------|
| `src/process/` | ⏳ Planned |
| `src/rag/` | ⏳ Planned |
| `src/graph/` | ⏳ Planned |

---

## Es et al. (2023) — RAGAS

**Type:** Paper  
**File:** [LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md](../LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md)  
**ThemisDB Version:** v1.3.0+

| Module | Status |
|--------|--------|
| `src/llm/monitoring/` | ✅ Implemented |

---

## FITKO (2024) / BMI (2017) — OZG, FIM, XÖV Standards

**Type:** Standard  
**File:** [papers/verwaltungs_it_ozg_sources.md](../papers/verwaltungs_it_ozg_sources.md)  
**ThemisDB Version:** v1.9.0+

| Module | Status |
|--------|--------|
| `src/process/` | 🔄 Partial |
| `src/importers/` | 🔄 Partial |
| `src/auth/` | 🔄 Partial |

---

## Forgy (1982) — RETE Algorithm

**Type:** Paper  
**File:** [papers/forgy_rete_algorithm_1982.md](../papers/forgy_rete_algorithm_1982.md)  
**ThemisDB Version:** v2.1.0+ planned

| Module | Status |
|--------|--------|
| `src/analytics/` — ExpertSystemEngine, KnowledgeBase | ⏳ Planned Q2 2027 |
| `src/analytics/cep_engine.cpp` — NFA als Alpha-Netz-Äquivalent (Partial) | 🔄 Partial |
| `src/graph/` — KnowledgeGraphReasoner (Horn-Clause Forward-Chaining) | ⏳ Planned Q4 2026 |

---

## Forgy / CLIPS / Drools — RETE Forward-Chaining Rule Engine

**Type:** Best Practice  
**File:** [best_practices/rete_forward_chaining_rule_engine.md](../best_practices/rete_forward_chaining_rule_engine.md)  
**ThemisDB Version:** v2.1.0+ planned

| Module | Status |
|--------|--------|
| `src/analytics/` — ExpertSystemEngine + KnowledgeBase | ⏳ Planned Q2 2027 |
| `src/analytics/cep_engine.cpp` | 🔄 Partial (NFA-Basis) |
| `src/graph/` — KnowledgeGraphReasoner | ⏳ Planned Q4 2026 |

---



**Type:** Best Practice  
**File:** [best_practices/fnv1a_checksums.md](../best_practices/fnv1a_checksums.md)  
**ThemisDB Version:** v2.0.0

| Module | Status |
|--------|--------|
| `src/prompt_engineering/` | ✅ Adopted |

---

## Gutierrez et al. (2024) — HippoRAG

**Type:** Paper  
**File:** [papers/hipporag_gutierrez_2024.md](../papers/hipporag_gutierrez_2024.md)  
**ThemisDB Version:** planned Q2 2026

| Module | Status |
|--------|--------|
| `src/process/` | ⏳ Planned |
| `src/rag/` | ⏳ Planned |

---

## Hu et al. (2022) — LoRA

**Type:** Paper  
**File:** [LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md](../LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md) · [papers/lora_low_rank_adaptation_2022.md](../papers/lora_low_rank_adaptation_2022.md)  
**ThemisDB Version:** v1.3.0+

| Module | Status |
|--------|--------|
| `src/llm/lora/` | ✅ Implemented |
| `src/training/` | ✅ Implemented |
| `src/analytics/` — LoRAPatternClassifier | ⏳ Planned Q3 2027 |
| `src/graph/` — KnowledgeGraphReasoner::applyLoRAScore() | ⏳ Planned Q2 2027 |
| `src/rag/` — LoRAEnhancedRetriever | ⏳ Planned Q2 2027 |

---

## Hu et al. / vLLM / Chronopoulou et al. — Multi-LoRA Adapter Routing

**Type:** Best Practice  
**File:** [best_practices/multi_lora_adapter_routing.md](../best_practices/multi_lora_adapter_routing.md)  
**ThemisDB Version:** v2.1.0+ planned

| Module | Status |
|--------|--------|
| `src/llm/` — MultiLoRAManager | 🔄 Partial (single-adapter) |
| `src/analytics/` — LoRAPatternClassifier | ⏳ Planned Q3 2027 |
| `src/graph/` — KnowledgeGraphReasoner::applyLoRAScore() | ⏳ Planned Q2 2027 |
| `src/rag/` — LoRAEnhancedRetriever | ⏳ Planned Q2 2027 |

---

## Karger et al. (1997) — Consistent Hash Ring

**Type:** Best Practice  
**File:** [best_practices/consistent_hash_ring.md](../best_practices/consistent_hash_ring.md)  
**ThemisDB Version:** v2.1.0

| Module | Status |
|--------|--------|
| `src/server/` | ✅ Adopted |
| `src/sharding/` | ✅ Adopted |

---

## Kulkarni & Michels (2012) — SQL:2011 Temporal Features

**Type:** Paper  
**File:** [papers/temporal_sql2011_2012.md](../papers/temporal_sql2011_2012.md)  
**ThemisDB Version:** v1.x+

| Module | Status |
|--------|--------|
| `src/temporal/` | 🔄 Partial |
| `src/query/` | 🔄 Partial |

---

## Kusupati et al. (2022) — Matryoshka Representation Learning

**Type:** Paper  
**File:** [LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md](../LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md)  
**ThemisDB Version:** v1.4.1+

| Module | Status |
|--------|--------|
| `src/vector/` | ⏳ Planned |

---

## Kwon et al. (2023) — PagedAttention

**Type:** Paper  
**File:** [LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md](../LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md)  
**ThemisDB Version:** v1.4.0-alpha+

| Module | Status |
|--------|--------|
| `src/llm/` | ✅ Implemented |

---

## Lewis et al. (2020) — RAG

**Type:** Paper  
**File:** [LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md](../LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md)  
**ThemisDB Version:** v1.2.0+

| Module | Status |
|--------|--------|
| `src/rag/` | ✅ Implemented |

---

## Malkov & Yashunin (2020) — HNSW

**Type:** Paper  
**File:** [papers/hnsw_efficient_ann_2020.md](../papers/hnsw_efficient_ann_2020.md)  
**ThemisDB Version:** v1.0.0+

| Module | Status |
|--------|--------|
| `src/index/` | ✅ Implemented |
| `src/vector/` | ✅ Implemented |
| `src/rag/` | ✅ Implemented |

---

## Martin Thompson — Lock-Free Cache Reads (Mechanical Sympathy)

**Type:** Best Practice  
**File:** [best_practices/lock_free_cache_reads.md](../best_practices/lock_free_cache_reads.md)  
**ThemisDB Version:** v1.9.0

| Module | Status |
|--------|--------|
| `src/cache/` | ✅ Adopted |

---

## NIST SP 800-52 Rev 2 — TLS 1.3 Cipher Hardening

**Type:** Best Practice  
**File:** [best_practices/tls13_cipher_hardening.md](../best_practices/tls13_cipher_hardening.md)  
**ThemisDB Version:** v1.0.0+

| Module | Status |
|--------|--------|
| `src/server/` | ✅ Adopted |

---

## Marcus et al. (2021) — Bao: Learned Query Optimization

**Type:** Paper  
**File:** [papers/marcus_bao_learned_query_opt_2021.md](../papers/marcus_bao_learned_query_opt_2021.md)  
**ThemisDB Version:** planned v2.0.0

| Module | Status |
|--------|--------|
| `src/query/adaptive_optimizer.cpp` | 🔄 In Progress (TreeConv embedding planned Q3 2026) |
| `src/query/runtime_reoptimizer.cpp` | 🔄 In Progress (Online-Feedback-Loop planned Q4 2026) |
| `src/query/query_optimizer.cpp` | 🔄 In Progress (Plan-Feature-Extraktion) |
| `src/training/` | ⏳ Planned (Modell-Persistierung) |

---

## Marcus et al. (2021) + Zhou et al. (2022) — AI-Driven Query Optimization

**Type:** Best Practice  
**File:** [best_practices/ai_driven_query_optimization.md](../best_practices/ai_driven_query_optimization.md)  
**ThemisDB Version:** v2.0.0+

| Module | Status |
|--------|--------|
| `src/query/adaptive_optimizer.cpp` | 🔄 Partially Adopted |
| `src/query/runtime_reoptimizer.cpp` | 🔄 Partially Adopted |
| `src/storage/index_analyzer.cpp` | ✅ IIndexAnalysisAdvisor-Hook |

---

## Raasveldt & Mühleisen (2019) — DuckDB

**Type:** Paper  
**File:** [papers/duckdb_olap_2019.md](../papers/duckdb_olap_2019.md)  
**ThemisDB Version:** planned v2.x

| Module | Status |
|--------|--------|
| `src/query/` | ⏳ Planned |
| `src/exporters/` | ⏳ Planned |

---

## Reimers & Gurevych (2019) — Sentence-BERT

**Type:** Paper  
**File:** [LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md](../LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md)  
**ThemisDB Version:** v1.0.0+

| Module | Status |
|--------|--------|
| `src/vector/` | ✅ Implemented |
| `src/rag/` | ✅ Implemented |

---

## RFC 6585 — Token Bucket Rate Limiting

**Type:** Best Practice  
**File:** [best_practices/token_bucket_rate_limiting.md](../best_practices/token_bucket_rate_limiting.md)  
**ThemisDB Version:** v1.6.0

| Module | Status |
|--------|--------|
| `src/server/` | ✅ Adopted |

---

## RFC 7519 + RFC 6749 — JWT Short-Lived Tokens

**Type:** Best Practice  
**File:** [best_practices/jwt_short_lived_tokens.md](../best_practices/jwt_short_lived_tokens.md)  
**ThemisDB Version:** v1.6.0

| Module | Status |
|--------|--------|
| `src/server/` | ✅ Adopted |
| `src/auth/` | ✅ Adopted |

---

## RFC 9106 — Argon2id Key Derivation

**Type:** Best Practice  
**File:** [best_practices/argon2id_kdf.md](../best_practices/argon2id_kdf.md)  
**ThemisDB Version:** v0.1.0

| Module | Status |
|--------|--------|
| `plugins/user_storage_encrypted/` | ✅ Adopted |

---

## Robinson, Webber & Eifrem (2015) — Graph Databases

**Type:** Book  
**File:** [papers/graph_databases_oreilly_2015.md](../papers/graph_databases_oreilly_2015.md)  
**ThemisDB Version:** v1.0.0+

| Module | Status |
|--------|--------|
| `src/graph/` | 🔄 Partial |
| `src/aql/` | 🔄 Partial |

---

## RocksDB Docs — WriteBatch Atomicity

**Type:** Best Practice  
**File:** [best_practices/rocksdb_write_batch_atomicity.md](../best_practices/rocksdb_write_batch_atomicity.md)  
**ThemisDB Version:** v1.0.0+

| Module | Status |
|--------|--------|
| `src/index/` | ✅ Adopted |
| `src/cache/` | ✅ Adopted |

---

## Sutter, Herb (GotW #24) — PIMPL Idiom

**Type:** Best Practice  
**File:** [best_practices/pimpl_abi_stability.md](../best_practices/pimpl_abi_stability.md)  
**ThemisDB Version:** v1.9.0

| Module | Status |
|--------|--------|
| `src/server/` | ✅ Adopted |

---

## Sheng et al. (2023) — S-LoRA: Concurrent LoRA Adapter Serving

**Type:** Paper  
**File:** [papers/sheng_slora_concurrent_adapters_2023.md](../papers/sheng_slora_concurrent_adapters_2023.md)  
**ThemisDB Version:** planned Q2/Q3 2026

| Module | Status |
|--------|--------|
| `src/llm/lora/` | 🔄 In Progress (Adapter-Paging planned Q2 2026) |
| `src/rag/streaming_retriever.cpp` | 🔄 In Progress (Hetero-Batching planned Q3 2026) |
| `src/llm/llm_deployment_plugin.cpp` | 🔄 In Progress (Adapter-Prefetching) |
| `src/gpu/` | ⏳ Planned (CUDA sgmv kernel Q3 2026) |

---

## Sheng et al. (2023) + Wang et al. (2024) — S-LoRA Near-Realtime RAG Serving

**Type:** Best Practice  
**File:** [best_practices/slora_realtime_rag_serving.md](../best_practices/slora_realtime_rag_serving.md)  
**ThemisDB Version:** planned Q2 2026

| Module | Status |
|--------|--------|
| `src/llm/lora/` | 🔄 Partially Adopted (Hot-Swap implemented; Paging planned) |
| `src/rag/streaming_retriever.cpp` | ⏳ Planned (Draft-Verify pipeline Q1 2027) |

---

## van der Aalst (2016) — Process Mining

**Type:** Book  
**File:** [papers/process_mining_van_der_aalst_2012.md](../papers/process_mining_van_der_aalst_2012.md)  
**ThemisDB Version:** v1.9.0+

| Module | Status |
|--------|--------|
| `src/process/` | 🔄 Partial |
| `src/analytics/` | 🔄 Partial |

---

## Vaswani et al. (2017) — Attention Is All You Need

**Type:** Paper  
**File:** [LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md](../LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md)  
**ThemisDB Version:** v1.3.0+

| Module | Status |
|--------|--------|
| `src/llm/` | ✅ Implemented |

---

## Wang et al. (2024) — Speculative RAG

**Type:** Paper  
**File:** [papers/wang_speculative_rag_2024.md](../papers/wang_speculative_rag_2024.md)  
**ThemisDB Version:** planned Q1 2027

| Module | Status |
|--------|--------|
| `src/rag/streaming_retriever.cpp` | ⏳ Planned (Draft-Verify pipeline) |
| `src/rag/agentic_rag.cpp` | ⏳ Planned (Speculative iteration strategy) |
| `src/llm/lora/` | ⏳ Planned (Domain-LoRA as Draft-Specialist) |

---

## Wei et al. (2022) — Chain-of-Thought Prompting

**Type:** Paper  
**File:** [LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md](../LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md)  
**ThemisDB Version:** v1.2.0+

| Module | Status |
|--------|--------|
| `src/prompt_engineering/` | ✅ Implemented |

---

## W3C OWL 2 RL / Apache Jena / Stardog — OWL-lite Ontology Constraints

**Type:** Best Practice  
**File:** [best_practices/owl_lite_ontology_constraints.md](../best_practices/owl_lite_ontology_constraints.md)  
**ThemisDB Version:** v2.1.0+ planned

| Module | Status |
|--------|--------|
| `src/graph/` — OntologyManager, PathConstraints | ⏳ Planned Q3 2026 |
| `src/graph/` — KnowledgeGraphReasoner | ⏳ Planned Q4 2026 |
| `src/rag/` — OntologyAwareRetriever | ⏳ Planned Q4 2026 |
| `src/analytics/` — KnowledgeBase (RDF-Triple-Format) | ⏳ Planned Q2 2027 |

---

## White et al. (2023) — Prompt Pattern Catalog

**Type:** Paper  
**File:** [LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md](../LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md) · [papers/prompt_patterns_catalog_2023.md](../papers/prompt_patterns_catalog_2023.md)  
**ThemisDB Version:** v1.2.0+

| Module | Status |
|--------|--------|
| `config/prompts/` | ✅ Implemented |
| `src/prompt_engineering/` | ✅ Implemented |

---

## Zhou et al. (2022) — AI Meets Database (AI4DB Survey)

**Type:** Paper  
**File:** [papers/zhou_ai4db_survey_2022.md](../papers/zhou_ai4db_survey_2022.md)  
**ThemisDB Version:** v2.0.0+ framework

| Module | Status |
|--------|--------|
| `src/query/adaptive_optimizer.cpp` | 🔄 In Progress (Level-2 ML-Feedback-Loop) |
| `src/query/query_optimizer.cpp` | 🔄 In Progress (Level-1 Cardinality + Statistics) |
| `src/storage/index_analyzer.cpp` | ✅ Implemented (IIndexAnalysisAdvisor AI4DB-Hook) |
| `src/cache/adaptive_query_cache.cpp` | ✅ Implemented (Workload-Adaptive Caching) |
| `src/rag/`, `src/llm/` | ✅ Implemented (DB4AI: AQL LLM INFER directive) |

---

## Zhou et al. (2022) — APE (Automatic Prompt Engineer)

**Type:** Paper  
**File:** [LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md](../LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md)  
**ThemisDB Version:** v1.4.0-alpha+

| Module | Status |
|--------|--------|
| `src/prompt_engineering/` | ✅ Implemented |

---




## Khattab et al. (2023/2024) — DSPy: Compiling Declarative Language Model Calls into Self-Improving Pipelines

**Type:** Paper
**File:** [papers/khattab_dspy_2023.md](../papers/khattab_dspy_2023.md)
**ThemisDB Version:** v2.0.0+ (declaration layer); v2.2.0 (compiler)

| Module | Status |
|--------|--------|
| `src/prompt_engineering/dspy_module.cpp` | ✅ Implemented (DspySignature, DspyPredict, DspyChainOfThought) |
| `src/prompt_engineering/prompt_template_compiler.cpp` | ✅ Implemented (typed slot handling) |
| `src/prompt_engineering/dspy_optimizer.cpp` | ⏳ Planned v2.2.0 (DspyOptimize compiler) |

---

## Madaan et al. (2023) + Shinn et al. (2023) — Self-Refine + Reflexion

**Type:** Paper
**File:** [papers/madaan_self_refine_2023.md](../papers/madaan_self_refine_2023.md)
**ThemisDB Version:** v1.5.0+

| Module | Status |
|--------|--------|
| `src/prompt_engineering/reflection_tuner.cpp` | ✅ Implemented (4 strategies, SelfAwareContext, HallucinationGuard) |
| `src/prompt_engineering/llm_reflection_adapter.cpp` | ✅ Implemented |
| `src/prompt_engineering/prompt_engineering_integration.cpp` | ✅ Implemented (afterExecution hook) |
| `src/prompt_engineering/prompt_engineering_metrics.cpp` | ✅ Implemented (reflection counters) |

---

## Pryzant et al. (2023) — ProTeGi: Automatic Prompt Optimization with Textual Gradients

**Type:** Paper
**File:** [papers/pryzant_protegi_prompt_optimization_2023.md](../papers/pryzant_protegi_prompt_optimization_2023.md)
**ThemisDB Version:** v2.0.0+

| Module | Status |
|--------|--------|
| `src/prompt_engineering/protegi_optimizer.cpp` | ✅ Implemented (ProTeGiOptimizer, HeuristicProTeGiProvider) |
| `src/prompt_engineering/self_improvement_orchestrator.cpp` | ✅ Implemented (ProTeGi as optimization back-end) |
| `src/prompt_engineering/meta_prompt_generator.cpp` | ✅ Implemented (candidate prompt supply) |
| `src/prompt_engineering/feedback_collector.cpp` | ✅ Implemented (mini-batch failure sampling) |

---

## ThemisDB Engineering (2026) — LLM-Driven Index Advisor

**Type:** Paper (integration)
**File:** [papers/llm_index_advisor_integrated_2024.md](../papers/llm_index_advisor_integrated_2024.md)
**ThemisDB Version:** v1.9.0+

| Module | Status |
|--------|--------|
| `src/storage/index_analyzer.cpp` | ✅ Implemented (IIndexAnalysisAdvisor hook, applyAdvisor, TierThresholds) |
| `include/storage/index_analyzer.h` | ✅ Implemented (full public API) |
| `src/storage/storage_layout_advisor.cpp` | ✅ Implemented (emitDecisionRecord → DecisionRecordYamlProcessor) |
| `config/index_analyze.yaml` | ✅ Implemented (YAML-driven AI advisor config) |

---

## Yao et al. (2023) — Tree of Thoughts: Deliberate Problem Solving with LLMs

**Type:** Paper
**File:** [papers/yao_tree_of_thoughts_2023.md](../papers/yao_tree_of_thoughts_2023.md)
**ThemisDB Version:** v2.0.0+

| Module | Status |
|--------|--------|
| `src/prompt_engineering/tree_of_thoughts.cpp` | ✅ Implemented (BFS/DFS/BEAM, HeuristicThoughtGenerator) |
| `src/prompt_engineering/chain_of_thought.cpp` | ✅ Implemented (CoT as degenerate ToT case) |
| `src/prompt_engineering/prompt_engineering_integration.cpp` | ✅ Implemented (opt-in ToT via IntegrationConfig) |

---

## AI4DB + ISUM — LLM-Driven Index Optimization (Best Practice)

**Type:** Best Practice
**File:** [best_practices/llm_driven_index_optimization.md](../best_practices/llm_driven_index_optimization.md)
**ThemisDB Version:** v1.9.0+

| Module | Status |
|--------|--------|
| `src/storage/index_analyzer.cpp` | ✅ Adopted |
| `include/storage/index_analyzer.h` | ✅ Adopted |
| `config/index_analyze.yaml` | ✅ Adopted |

---

## ProTeGi + Self-Refine + ToT + DSPy — LLM Prompt Enhancement Pipeline (Best Practice)

**Type:** Best Practice
**File:** [best_practices/llm_prompt_enhancement_pipeline.md](../best_practices/llm_prompt_enhancement_pipeline.md)
**ThemisDB Version:** v1.5.0+ / v2.0.0+

| Module | Status |
|--------|--------|
| `src/prompt_engineering/protegi_optimizer.cpp` | ✅ Adopted |
| `src/prompt_engineering/reflection_tuner.cpp` | ✅ Adopted |
| `src/prompt_engineering/tree_of_thoughts.cpp` | ✅ Adopted |
| `src/prompt_engineering/dspy_module.cpp` | 🔄 Partially Adopted |
| `src/prompt_engineering/self_improvement_orchestrator.cpp` | ✅ Adopted |
| `src/prompt_engineering/prompt_engineering_metrics.cpp` | ✅ Adopted |

---



## Bai et al. (2022) + Lee et al. (2023) — Constitutional AI + RLAIF

**Type:** Paper
**File:** [papers/bai_constitutional_ai_rlaif_2022.md](../papers/bai_constitutional_ai_rlaif_2022.md)
**ThemisDB Version:** v1.6.0+

| Module | Status |
|--------|--------|
| `src/rag/rlaif_trainer.cpp` | ✅ Implemented (IAIJudge, RewardModel, PreferenceDataset, RLAIFGuardrailPlugin) |
| `src/rag/continuous_learning_orchestrator.cpp` | ✅ Implemented (Loop 4 = RLAIF; ILoRAFederationCoordinator) |
| `src/prompt_engineering/reflection_tuner.cpp` | ✅ Implemented (CONSTITUTIONAL strategy = supervised CAI phase) |
| `src/rag/bias_detector.cpp` | ✅ Implemented (constitutional compliance filter) |

---

## Liu et al. (2023) — G-Eval: NLG Evaluation using GPT-4 (EMNLP 2023)

**Type:** Paper
**File:** [papers/liu_geval_2023.md](../papers/liu_geval_2023.md)
**ThemisDB Version:** v1.6.0+

| Module | Status |
|--------|--------|
| `src/rag/geval_evaluator.cpp` | ✅ Implemented (token-probability expected-value scoring; llama_get_logits_ith) |
| `src/rag/rag_judge.cpp` | ✅ Implemented (dispatches to GEvalEvaluator in BALANCED/THOROUGH mode) |
| `src/rag/cot_evaluator.cpp` | ✅ Implemented (CoT criteria generation — first pass of G-Eval two-pass approach) |
| `src/rag/calibration_manager.cpp` | ✅ Implemented (temperature/Platt/isotonic calibration of G-Eval scores) |
| `src/rag/evaluation_cache.cpp` | ✅ Implemented (caches G-Eval results to avoid redundant LLM calls) |

---

## Yao et al. (2022) — ReAct: Synergizing Reasoning and Acting (ICLR 2023)

**Type:** Paper
**File:** [papers/yao_react_2022.md](../papers/yao_react_2022.md)
**ThemisDB Version:** v1.8.0+

| Module | Status |
|--------|--------|
| `src/rag/agentic_rag.cpp` | ✅ Implemented (full TAO loop; AgentTrace; tool registry; deduplication) |
| `src/rag/multi_step_rag.cpp` | ✅ Implemented (multi-step iterative retrieval strategy) |
| `src/rag/knowledge_gap_detector.cpp` | ✅ Implemented (three-level gap detection drives ReAct loop termination) |
| `src/rag/knowledge_graph_retriever.cpp` | ✅ Implemented (graph-traversal tool action in ReAct loop) |

---

## Zheng et al. (2023) — Judging LLM-as-a-Judge / MT-Bench (NeurIPS 2023)

**Type:** Paper
**File:** [papers/zheng_llm_judge_2023.md](../papers/zheng_llm_judge_2023.md)
**ThemisDB Version:** v1.6.0+

| Module | Status |
|--------|--------|
| `src/rag/llm_judge_integration.cpp` | ✅ Implemented (ILLMInferenceEngine* injection; allow_mock guard; position-bias prompt) |
| `src/rag/pairwise_comparator.cpp` | ✅ Implemented (head-to-head; randomised order; consistency check → TIE) |
| `src/rag/distributed_rag_evaluator.cpp` | ✅ Implemented (N-judge ensemble; MEAN/WEIGHTED_MEAN/MAJORITY_VOTING/BEST_OF_N) |
| `src/rag/calibration_manager.cpp` | ✅ Implemented (calibrates judge scores against human annotations) |
| `src/rag/prompt_templates.cpp` | ✅ Implemented (verbosity-bias rubric injection in judge prompts) |

---

## G-Eval + LLM-as-Judge Ensemble + Calibration (Best Practice)

**Type:** Best Practice
**File:** [best_practices/llm_as_judge_rag_evaluation.md](../best_practices/llm_as_judge_rag_evaluation.md)
**ThemisDB Version:** v1.6.0+

| Module | Status |
|--------|--------|
| `src/rag/geval_evaluator.cpp` | ✅ Adopted |
| `src/rag/pairwise_comparator.cpp` | ✅ Adopted |
| `src/rag/distributed_rag_evaluator.cpp` | ✅ Adopted |
| `src/rag/calibration_manager.cpp` | ✅ Adopted |
| `src/rag/evaluation_cache.cpp` | ✅ Adopted |

---

## Constitutional AI / RLAIF Training Pipeline (Best Practice)

**Type:** Best Practice
**File:** [best_practices/constitutional_ai_rlaif_training.md](../best_practices/constitutional_ai_rlaif_training.md)
**ThemisDB Version:** v1.6.0+

| Module | Status |
|--------|--------|
| `src/rag/rlaif_trainer.cpp` | ✅ Adopted |
| `src/rag/continuous_learning_orchestrator.cpp` | ✅ Adopted |
| `src/prompt_engineering/reflection_tuner.cpp` | ✅ Adopted |
| `config/prompts/constitutional_principles.yaml` | ✅ Adopted |

---

## Tarjan (1972) — Depth-First Search and Linear Graph Algorithms

**Type:** Scientific Paper  
**File:** [papers/tarjan_scc_1972.md](../papers/tarjan_scc_1972.md)  
**ThemisDB Version:** v2.2.0+

| Module | Status |
|--------|--------|
| `src/sharding/cross_shard_transaction.cpp` | ✅ Implemented |

---

## ADR-010 — Distributed Deadlock Detection via Wait-For Graph

**Type:** Architecture Decision  
**File:** [architecture_decisions/adr_010_distributed_deadlock_detection.md](../architecture_decisions/adr_010_distributed_deadlock_detection.md)  
**ThemisDB Version:** v2.2.0+

| Module | Status |
|--------|--------|
| `src/sharding/cross_shard_transaction.cpp` | ✅ Accepted |
| `src/sharding/shard_rpc_client.cpp` | ✅ Accepted |
| `src/sharding/shard_rpc_server.cpp` | ✅ Accepted |

---

*Last generated: see git log*
