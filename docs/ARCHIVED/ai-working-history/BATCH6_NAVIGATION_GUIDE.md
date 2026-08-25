# Batch 6: Cross-Module Navigation Guide

**Status:** Batch 6 Phase 6.1 — Cross-Module Navigation  
**Date:** 2026-08-14  
**Scope:** 35 documented modules across Batches 1-5

---

## Overview

This document provides comprehensive cross-module navigation for ThemisDB's 35 documented modules. It maps integration points, dependency hierarchies, and Wave A/B/C/D gate relationships to help developers understand system architecture and module interactions.

---

## Quick Navigation by Use Case

### 🔍 I Want to Understand a Feature

| Feature | Key Modules | Start Here |
|---------|------------|-----------|
| **Full-text Search** | search, index, rag | `src/search/README.md` |
| **Vector Search & RAG** | rag, retrieval, llm_wiki, distributed_tensor | `src/rag/README.md` |
| **LLM Integration** | llm, llm_wiki, prompt_engineering | `src/llm/README.md` |
| **Data Storage** | storage, cache, content, metadata | `src/storage/README.md` |
| **Distributed Transactions** | process, failover, updates, maintenance | `src/process/README.md` |
| **Query Execution** | query, analytics, aql | `src/query/README.md` |
| **GPU Acceleration** | acceleration, gpu, onnx_clip | `src/acceleration/README.md` |
| **Replication & HA** | replication, network, failover | `src/replication/README.md` |
| **Security & Access Control** | security, auth, governance | `src/security/README.md` |
| **Voice & Speech** | voice, voice_ai | `src/voice/README.md` |
| **Time Series Data** | timeseries, storage, index | `src/timeseries/README.md` |
| **Knowledge Graphs** | graph, distributed_knowledge, content | `src/graph/README.md` |
| **Data Import/Export** | importers, ingestion, exporters, cdc | `src/importers/README.md` |

### 🏗️ I Want to Understand Architecture

| Architecture Layer | Primary Modules | Secondary Modules | Start Here |
|---|---|---|---|
| **Core Runtime** | base, core, execution | config, performance | `include/base/README.md` |
| **Storage Layer** | storage, cache | content, metadata | `src/storage/README.md` |
| **Distributed** | failover, replication, distributed_knowledge | network, sharding | `src/failover/README.md` |
| **Query & Retrieval** | query, aql, search, retrieval | analytics, optimization | `src/query/README.md` |
| **AI/ML Layer** | llm, llm_wiki, rag, retrieval | ethics_ai, evaluation | `src/llm/README.md` |
| **Acceleration** | acceleration, gpu, onnx_clip | performance, benchmark | `src/acceleration/README.md` |
| **Ingestion** | ingestion, importers, cdc | content, metadata | `src/ingestion/README.md` |
| **Operations** | observability, governance, auth | security, audit | `src/observability/README.md` |

### 🧪 I Want to Write Tests

Start with the test suite navigation and benchmark documentation:
- **Test Organization:** `TEST_SUITE_NAVIGATION.md` (Phase 6.3)
- **Benchmark Gates:** `BENCHMARK_GATES_REFERENCE.md` (Phase 6.3)
- **Module Tests:** `src/<module>/tests/` (see mapping in Phase 6.3)
- **Wave A/B/C/D Gates:** `WAVE_GATE_DASHBOARD.md` (Phase 6.2)

### 🚀 I Want to Deploy or Operate

Start with the production readiness matrix and feature capability matrix:
- **Readiness Status:** `PRODUCTION_READINESS_MATRIX.md` (Phase 6.2)
- **Feature Matrix:** `FEATURE_CAPABILITY_MATRIX.md` (Phase 6.2)
- **Wave Gates:** `WAVE_GATE_DASHBOARD.md` (Phase 6.2)
- **Module Runbooks:** See `docs/` and `src/<module>/ROADMAP.md`

---

## Module Dependency Hierarchy

### Tier 0: Foundation (No Dependencies on Other Modules)
- **base** — Core data structures, utilities, type system
- **core** — Runtime primitives, lifecycle, config
- **performance** — Performance monitoring and tuning

### Tier 1: Infrastructure (Depend on Tier 0)
- **cache** — In-memory caching layer
- **storage** — Persistent storage engine, WAL, recovery
- **network** — RPC/gRPC, protocol handling
- **metadata** — Schema and metadata management
- **config** — Configuration system

### Tier 2: Distributed & Coordination (Depend on Tier 0-1)
- **failover** — Automatic failover and health detection
- **replication** — Multi-region replication, WAL shipping
- **sharding** — Data distribution across nodes
- **maintenance** — Maintenance operations, compaction
- **cdc** — Change data capture

### Tier 3: Data Access (Depend on Tier 1-2)
- **query** — Query planning and execution
- **aql** — Query language and parsing
- **index** — Indexing strategies (B-tree, LSM, FTS)
- **content** — Content storage, versioning
- **access_model** — Access control and RBAC

### Tier 4: Business Logic (Depend on Tier 2-3)
- **analytics** — Analysis and reporting
- **ingestion** — Data ingestion pipelines
- **importers** — Format-specific importers
- **exporters** — Format-specific exporters
- **updates** — Update coordination and MVCC

### Tier 5: Search & Retrieval (Depend on Tier 3-4)
- **search** — Full-text search orchestration
- **retrieval** — Dense and sparse retrieval
- **rag** — RAG pipeline and orchestration

### Tier 6: AI/ML (Depend on Tier 4-5)
- **llm** — LLM inference and prompting
- **llm_wiki** — Retrieval-augmented LLM generation
- **onnx_clip** — CLIP model inference
- **image_analysis** — Image understanding
- **ethics_ai** — AI ethics and governance
- **evaluation** — Model and system evaluation

### Tier 7: Acceleration (Cross-cutting, Depend on Target Layer)
- **acceleration** — CUDA/GPU acceleration framework
- **gpu** — GPU-specific operations
- **distributed_tensor** — Distributed tensor operations

### Tier 8: Observability & Governance (Cross-cutting)
- **observability** — Metrics, traces, logging
- **security** — Security controls, encryption
- **governance** — Policy and compliance
- **auth** — Authentication and authorization
- **voice** — Voice interface and processing

### Tier 9: Advanced Integration (Depend on Tier 5-8)
- **distributed_knowledge** — Graph synchronization
- **graph** — Knowledge graph operations
- **timeseries** — Time series data handling
- **llm_streaming** — Streaming LLM operations
- **process** — Process execution and orchestration
- **prompt_engineering** — Prompt management and tuning

---

## Integration Points Map

### Search → Retrieval Chain
```
search (full-text index)
  ↓ queries rag
rag (orchestration)
  ↓ uses retrieval, llm_wiki
retrieval (dense/sparse)
  ↓ accesses index, distributed_tensor
index (indexing engine)
  ↓ stores in storage, cache
storage (persistent)
```

### Query Execution Chain
```
aql (query language)
  ↓ parsed by query
query (planner/executor)
  ↓ accesses storage, index
storage (persistent data)
  ↓ coordinated by process
process (transactions)
  ↓ replicated by replication
replication (multi-region)
  ↓ distributed by failover, sharding
failover/sharding (topology)
```

### Distributed Write Coordination
```
ingestion (ingest data)
  ↓ coordinates updates
updates (MVCC, conflict resolution)
  ↓ writes via storage
storage (WAL, persistence)
  ↓ replicated by replication
replication (async WAL shipping)
  ↓ distributed via sharding
sharding (multi-shard consistency)
  ↓ coordinated by process
process (transactions, atomicity)
```

### AI/LLM Stack
```
llm (inference engine)
  ↓ uses llm_wiki
llm_wiki (cached retrieval augmentation)
  ↓ accesses rag
rag (RAG orchestration)
  ↓ uses search, retrieval
search + retrieval (hybrid search)
  ↓ accesses index, distributed_tensor
index + distributed_tensor (indexing)
  ↓ stored in storage
storage (persistence)
```

### Acceleration Stack
```
acceleration (framework dispatch)
  ├─→ gpu (GPU operations)
  ├─→ distributed_tensor (tensor ops)
  └─→ onnx_clip (model inference)
      ↓ used by image_analysis
```

---

## Wave A/B/C/D Gate Alignment

### Wave A: Runtime Reliability (Q3–Q4 2026)

**Documented Modules in Wave A:**
- **process** — Deterministic transaction execution
- **failover** — Failover determinism + topology validation
- **updates** — Concurrent update handling
- **maintenance** — Crash recovery isolation
- **replication** — Geographic placement + lag controls
- **network** — Protocol reliability
- **storage** — WAL and recovery determinism

**Gate Exit Criteria:**
- Deterministic chaos evidence for recovery/failover paths
- Fail-closed behavior verified for all distributed paths
- `release_critical` CI green on `develop`
- Representative-hardware p95/p99 baselines refreshed

### Wave B: Performance Consolidation (Q3–Q4 2026)

**Documented Modules in Wave B:**
- **search** — 4-layer retrieval chain integration
- **retrieval** — Lock p95/p99 + memory gates
- **rag** — End-to-end optimization
- **importers** — Performance under sustained load
- **ingestion** — Throughput and latency gates
- **distributed_knowledge** — Cross-shard consistency validation
- **content** — Large-content performance gates
- **analytics** — Analysis/reporting gates

**Gate Exit Criteria:**
- Full 4-layer retrieval has stable p95/p99 on representative hardware
- Access Model benchmark gates closed with reproducible evidence
- Release decisions based on representative hardware, not scaffolding

### Wave C: Security Production Validation (Q4 2026)

**Documented Modules in Wave C:**
- **security** — Vault/HSM/PKI integration validation
- **governance** — Policy/compliance edge cases
- **auth** — Federation, token, authz workloads

**Gate Exit Criteria:**
- Production-style security integration evidence complete
- Audit evidence trustworthy under sustained load
- Policy gates consistently block boundary/license/hash regressions

### Wave D: Operability Hardening (Q1 2027)

**Covered by Cross-Module Linking:**
- All documented modules: distributed tracing, runbooks, SLA evidence

---

## Batch Documentation Status

### ✅ Batch 1: Include Module READMEs (7 modules)
- access_model, base, distributed_tensor, evaluation, gpu, llm_wiki, retrieval
- **Status:** Complete
- **Location:** `include/<module>/README.md`

### ✅ Batch 2: Core Src Modules (6 modules)
- Early-adopter modules with moderate documentation needs
- **Status:** Complete
- **Location:** `src/<module>/README.md`

### ✅ Batch 3: Tier 1+2 Core Modules (7 modules)
- **High Priority:** llm, server, sharding
- **Secondary:** analytics, rag, query, index
- **Status:** Complete
- **Location:** `src/<module>/README.md`, `ROADMAP.md`, related governance docs

### ✅ Batch 4: Tier 3 Modules (7 modules)
- **Wave A:** storage, replication, network, acceleration
- **Wave C:** security, governance, auth
- **Status:** Complete
- **Location:** `src/<module>/MODULE_GAPS_BATCH4.md` + README/ROADMAP
- **Test Gates:** 216 named gates with definitions
- **Benchmark Gates:** 30+ gates with SLO targets

### ✅ Batch 5: Wave A/B Core Modules (8 modules)
- **Wave A:** process, failover, updates, maintenance
- **Wave B:** importers, ingestion, distributed_knowledge, content
- **Status:** Complete
- **Location:** `src/<module>/MODULE_GAPS_BATCH5.md` + README/ROADMAP
- **Test Gates:** 48 named gates
- **Critical Gaps:** 65 identified

### 📋 Batch 6: Navigation & Consolidation (This Document)

**Phases:**
- **Phase 6.1:** Cross-Module Navigation (this guide)
- **Phase 6.2:** Documentation Index & Dashboard
- **Phase 6.3:** Test & Benchmark Navigation
- **Phase 6.4:** Final Validation & Consolidation

---

## How to Use This Guide

### For Feature Implementation
1. Find your feature in the "Quick Navigation by Use Case" table
2. Navigate to the primary module's README.md
3. Review its ROADMAP.md for phased implementation
4. Check MODULE_GAPS_BATCH<N>.md for known gaps
5. Reference Wave A/B/C/D gates for acceptance criteria
6. See TEST_SUITE_NAVIGATION.md for test organization

### For Architecture Understanding
1. Start with the "Architecture Layer" table
2. Review module dependency hierarchy
3. Study integration points for your layer
4. Check related governance documents in `docs/governance/`
5. See PRODUCTION_READINESS_MATRIX.md for maturity levels

### For Testing & Validation
1. Consult TEST_SUITE_NAVIGATION.md (Phase 6.3)
2. Reference BENCHMARK_GATES_REFERENCE.md (Phase 6.3)
3. Check module-specific test gate definitions
4. Map tests to source modules using Phase 6.3 tables
5. Verify Wave gate compliance with WAVE_GATE_DASHBOARD.md

### For Production Deployment
1. Review PRODUCTION_READINESS_MATRIX.md (Phase 6.2)
2. Check FEATURE_CAPABILITY_MATRIX.md (Phase 6.2)
3. Consult module ROADMAP.md for Phase 6 completion
4. Verify all Wave A/B/C/D gates in WAVE_GATE_DASHBOARD.md
5. Reference module-specific runbooks in `docs/`

---

## Related Documents (Phases 6.2–6.4)

| Phase | Document | Purpose |
|---|---|---|
| 6.2 | `MODULE_INDEX.md` | Alphabetical + categorical index of 35+ modules |
| 6.2 | `WAVE_GATE_DASHBOARD.md` | Wave A/B/C/D gate fulfillment status |
| 6.2 | `PRODUCTION_READINESS_MATRIX.md` | Maturity levels for all 73 modules |
| 6.2 | `FEATURE_CAPABILITY_MATRIX.md` | Feature-to-module capability mapping |
| 6.3 | `TEST_SUITE_NAVIGATION.md` | Test organization and suite purpose |
| 6.3 | `BENCHMARK_GATES_REFERENCE.md` | Benchmark gate definitions and SLOs |
| 6.4 | `BATCH6_CONSOLIDATION_REPORT.md` | Cross-reference validation results |
| 6.4 | `BATCH6_DELIVERY_SUMMARY.md` | Complete Batch 6 delivery summary |

---

## Key Contacts & Escalations

For questions about specific modules, refer to their README.md and ROADMAP.md files for owner/contact information.

For cross-module architecture questions, see `ARCHITECTURE.md` and `docs/governance/`.

For Wave A/B/C/D gate compliance, see `WAVE_GATE_DASHBOARD.md` (Phase 6.2).

---

**Batch 6 Status:** Phase 6.1 complete. Moving to Phase 6.2 (Documentation Index & Dashboard).
