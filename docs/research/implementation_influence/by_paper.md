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

## Zhou et al. (2022) — APE (Automatic Prompt Engineer)

**Type:** Paper  
**File:** [LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md](../LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md)  
**ThemisDB Version:** v1.4.0-alpha+

| Module | Status |
|--------|--------|
| `src/prompt_engineering/` | ✅ Implemented |

---

*Last generated: see git log*

