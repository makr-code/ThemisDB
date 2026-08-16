# Feature Capability Matrix

**Status:** Batch 6 Phase 6.2 — Feature Capability Matrix  
**Date:** 2026-08-14  
**Scope:** 35 documented modules across Batches 1–5

---

## Overview

This matrix maps capabilities and features to their responsible modules, helping developers find implementation details and understand module responsibilities. It serves as a reverse index from features to modules.

---

## Core Database Features

### Data Storage & Persistence
| Feature | Primary Module | Supporting Modules | Status | Readiness |
|---------|----------------|--------------------|--------|-----------|
| **Persistent Key-Value Storage** | storage | base, cache, metadata | ✅ | 🟢 RC |
| **Write-Ahead Log (WAL)** | storage | failover, replication, maintenance | ✅ | 🟢 RC |
| **Multi-Version Concurrency Control (MVCC)** | storage, updates | failover, content | ✅ | 🟢 RC |
| **Crash Recovery** | storage, maintenance | failover, replication, process | ✅ | 🟠 Beta |
| **Compaction & Cleanup** | storage, maintenance | index, cdc | ✅ | 🟠 Beta |
| **Content Storage & Versioning** | content | storage, updates, metadata | ✅ | 🟢 RC |
| **Blob/LOB Management** | importers, content | storage, ingestion | ✅ | 🟠 Beta |

### Query & Analytics
| Feature | Primary Module | Supporting Modules | Status | Readiness |
|---------|----------------|--------------------|--------|-----------|
| **Query Planning** | query | aql, index, analytics | ✅ | 🟢 RC |
| **Query Execution** | query | storage, cache, index, execution | ✅ | 🟢 RC |
| **AQL Query Language** | aql | query, index | ✅ | 🟠 Beta |
| **Index-Based Optimization** | index, query | analytics, storage | ✅ | 🟠 Beta |
| **Aggregation & Grouping** | analytics, query | storage, distributed_knowledge | ✅ | 🟠 Beta |
| **Reporting** | analytics, query | storage, index, metadata | ✅ | 🟠 Beta |

### Search & Retrieval
| Feature | Primary Module | Supporting Modules | Status | Readiness |
|---------|----------------|--------------------|--------|-----------|
| **Full-Text Search** | search, index | retrieval, content, storage | ✅ | 🟠 Beta |
| **Dense Vector Search** | retrieval, distributed_tensor | index, storage, acceleration | ✅ | 🟢 RC |
| **Sparse Search** | retrieval, search | index, storage | ✅ | 🟠 Beta |
| **Hybrid Search (Dense + Sparse)** | search, retrieval, rag | index, storage | ✅ | 🟠 Beta |
| **Full Retrieval Chain (4-layer)** | search, retrieval, rag | index, llm_wiki, distributed_tensor | 🟡 | 🟠 Beta |

### Indexing
| Feature | Primary Module | Supporting Modules | Status | Readiness |
|---------|----------------|--------------------|--------|-----------|
| **B-Tree Indexing** | index, query | storage | ✅ | 🟢 RC |
| **LSM Indexing** | index, storage | query, content | ✅ | 🟢 RC |
| **Full-Text Index** | index, search | storage, cache | ✅ | 🟠 Beta |
| **Vector Index (ANN)** | index, retrieval, distributed_tensor | acceleration, storage | ✅ | 🟠 Beta |
| **Graph Index** | graph, index | distributed_knowledge, storage | ✅ | 🟠 Beta |

---

## Distributed & High-Availability Features

### Replication & Multi-Region
| Feature | Primary Module | Supporting Modules | Status | Readiness |
|---------|----------------|--------------------|--------|-----------|
| **Synchronous Replication** | replication, failover | storage, network | ✅ | 🟠 Beta |
| **Asynchronous Replication** | replication, cdc | failover, storage | ✅ | 🟠 Beta |
| **Multi-Region Deployment** | replication, network, failover | storage, sharding | ✅ | 🟠 Beta |
| **Geo-Aware Routing** | replication, network | failover, sharding | 🟡 | 🟡 Alpha |
| **Cross-Region Failover** | failover, replication | network, process | ✅ | 🟠 Beta |
| **Lag Monitoring & Alerts** | replication, observability | network, failover | ✅ | 🟠 Beta |

### Failover & HA
| Feature | Primary Module | Supporting Modules | Status | Readiness |
|---------|----------------|--------------------|--------|-----------|
| **Automatic Failover Detection** | failover, observability | network, storage | ✅ | 🟢 RC |
| **Leader Election** | failover, process | storage, network | ✅ | 🟢 RC |
| **Split-Brain Prevention** | failover, process | network, replication | ✅ | 🟢 RC |
| **Health Checks** | failover, observability | network | ✅ | 🟠 Beta |
| **Topology Management** | failover, sharding | network, storage, process | ✅ | 🟠 Beta |
| **Graceful Degradation** | failover, acceleration, gpu | network, observability | ✅ | 🟠 Beta |

### Sharding & Distribution
| Feature | Primary Module | Supporting Modules | Status | Readiness |
|---------|----------------|--------------------|--------|-----------|
| **Data Sharding** | sharding, storage | network, failover, process | ✅ | 🟠 Beta |
| **Shard Rebalancing** | sharding, maintenance | failover, process, observability | ✅ | 🟠 Beta |
| **Consistent Hashing** | sharding, network | storage, failover | ✅ | 🟠 Beta |
| **Shard-Aware Routing** | sharding, network, query | failover, process | ✅ | 🟠 Beta |
| **Multi-Shard Transactions** | process, sharding, updates | failover, storage | ✅ | 🟠 Beta |

---

## Transaction & Consistency Features

### Transactions
| Feature | Primary Module | Supporting Modules | Status | Readiness |
|---------|----------------|--------------------|--------|-----------|
| **ACID Transactions** | process, storage, updates | failover, replication | ✅ | 🟢 RC |
| **Distributed Transactions** | process, sharding, updates | failover, network | ✅ | 🟠 Beta |
| **SAGA Transactions** | process, updates | failover, storage | ✅ | 🟠 Beta |
| **Nested Transactions** | process, updates | storage, failover | ✅ | 🟠 Beta |
| **Transaction Rollback** | updates, process, storage | maintenance, failover | ✅ | 🟢 RC |
| **Optimistic Locking** | updates, storage | process | ✅ | 🟠 Beta |

### Consistency Models
| Feature | Primary Module | Supporting Modules | Status | Readiness |
|---------|----------------|--------------------|--------|-----------|
| **Strong Consistency** | process, failover, replication | storage, updates, network | ✅ | 🟠 Beta |
| **Eventual Consistency** | replication, cdc | storage, updates | ✅ | 🟠 Beta |
| **Read-Your-Writes** | storage, updates, cache | process, replication | ✅ | 🟠 Beta |
| **Causal Consistency** | updates, process, distributed_knowledge | storage, replication | ✅ | 🟠 Beta |

### Updates & Conflict Resolution
| Feature | Primary Module | Supporting Modules | Status | Readiness |
|---------|----------------|--------------------|--------|-----------|
| **Conflict-Free Replicated Data Types (CRDTs)** | updates, distributed_knowledge | storage, replication | ✅ | 🟠 Beta |
| **Last-Write-Wins (LWW)** | updates, replication | storage | ✅ | 🟠 Beta |
| **Custom Merge Policies** | updates, ingestion | distributed_knowledge | ✅ | 🟡 Alpha |

---

## AI/ML & LLM Features

### LLM Integration
| Feature | Primary Module | Supporting Modules | Status | Readiness |
|---------|----------------|--------------------|--------|-----------|
| **LLM Inference** | llm, acceleration | gpu, onnx_clip | ✅ | 🟠 Beta |
| **Batch Inference** | llm, acceleration | gpu, distributed_tensor | ✅ | 🟠 Beta |
| **Streaming Inference** | llm_streaming, llm | acceleration, observability | ✅ | 🟡 Alpha |
| **Model Loading & Caching** | llm_wiki, cache, execution | storage | ✅ | 🟠 Beta |
| **Prompt Management** | prompt_engineering, llm | evaluation, ethics_ai | ✅ | 🟡 Alpha |
| **Token Counting** | llm, prompt_engineering | evaluation | ✅ | 🟠 Beta |

### RAG & Retrieval Augmentation
| Feature | Primary Module | Supporting Modules | Status | Readiness |
|---------|----------------|--------------------|--------|-----------|
| **Retrieval-Augmented Generation (RAG)** | rag, retrieval, llm_wiki | search, index, llm | ✅ | 🟠 Beta |
| **Hybrid Search (BM25 + Vector)** | rag, retrieval, search | index, distributed_tensor | ✅ | 🟠 Beta |
| **Context Windowing** | rag, llm, llm_wiki | retrieval, content | ✅ | 🟠 Beta |
| **Re-ranking** | retrieval, rag | index, analytics | 🟡 | 🟡 Alpha |
| **Question Reformulation** | rag, prompt_engineering | llm | 🟡 | 🟡 Alpha |

### AI Ethics & Governance
| Feature | Primary Module | Supporting Modules | Status | Readiness |
|---------|----------------|--------------------|--------|-----------|
| **Bias Detection** | ethics_ai, evaluation | llm, rag | ✅ | 🟠 Beta |
| **Fairness Monitoring** | ethics_ai, observability | evaluation | 🟡 | 🟡 Alpha |
| **Explainability** | evaluation, ethics_ai | rag, retrieval | 🟡 | 🟡 Alpha |
| **Model Audit Trail** | governance, ethics_ai, observability | storage, cdc | ✅ | 🟡 Alpha |

### Model Evaluation
| Feature | Primary Module | Supporting Modules | Status | Readiness |
|---------|----------------|--------------------|--------|-----------|
| **Embedding Evaluation** | evaluation, distributed_tensor | llm, rag | ✅ | 🟠 Beta |
| **Ranking Evaluation** | evaluation, rag | retrieval, analytics | ✅ | 🟠 Beta |
| **LLM Output Evaluation** | evaluation, llm | prompt_engineering | ✅ | 🟠 Beta |
| **Benchmark Tracking** | evaluation, performance | observability | ✅ | 🟠 Beta |

---

## Data Integration & Ingestion

### Ingestion
| Feature | Primary Module | Supporting Modules | Status | Readiness |
|---------|----------------|--------------------|--------|-----------|
| **Batch Ingestion** | ingestion, importers | storage, updates, process | ✅ | 🟢 RC |
| **Streaming Ingestion** | ingestion, importers | storage, cache, updates | ✅ | 🟢 RC |
| **Backpressure & Flow Control** | ingestion, network | storage, updates | ✅ | 🟠 Beta |
| **Schema Enforcement** | ingestion, metadata | storage, content | ✅ | 🟠 Beta |
| **Error Recovery** | ingestion, importers, updates | storage, failover | ✅ | 🟠 Beta |

### Import Formats
| Feature | Primary Module | Supporting Modules | Status | Readiness |
|---------|----------------|--------------------|--------|-----------|
| **CSV/TSV Import** | importers, ingestion | storage, metadata | ✅ | 🟠 Beta |
| **JSON/JSONL Import** | importers, ingestion | storage, content | ✅ | 🟠 Beta |
| **Parquet Import** | importers, ingestion | storage, distributed_tensor | ✅ | 🟠 Beta |
| **Binary Format Import** | importers, ingestion, content | storage | ✅ | 🟡 Alpha |
| **Custom Format Support** | importers | ingestion, metadata | ✅ | 🟡 Alpha |

### Export
| Feature | Primary Module | Supporting Modules | Status | Readiness |
|---------|----------------|--------------------|--------|-----------|
| **CSV/TSV Export** | exporters, query | storage, analytics | ✅ | 🟠 Beta |
| **JSON Export** | exporters, query | storage, content | ✅ | 🟠 Beta |
| **Parquet Export** | exporters, query | storage, distributed_tensor | ✅ | 🟠 Beta |
| **Streaming Export** | exporters, cdc | replication, storage | ✅ | 🟠 Beta |

### Change Data Capture
| Feature | Primary Module | Supporting Modules | Status | Readiness |
|---------|----------------|--------------------|--------|-----------|
| **CDC (Change Data Capture)** | cdc, replication | storage, ingestion | ✅ | 🟠 Beta |
| **Event Ordering Guarantees** | cdc, replication | storage, process | ✅ | 🟠 Beta |
| **CDC Streaming** | cdc, exporters | replication, storage | ✅ | 🟠 Beta |

---

## Security & Access Control

### Authentication
| Feature | Primary Module | Supporting Modules | Status | Readiness |
|---------|----------------|--------------------|--------|-----------|
| **User Authentication** | auth, security | governance, observability | ✅ | 🟢 RC |
| **Token-Based Auth** | auth, security | governance | ✅ | 🟢 RC |
| **Federation (OAuth/SAML)** | auth, security | governance, network | ✅ | 🟠 Beta |
| **Multi-Factor Auth (MFA)** | auth, security | governance | 🟡 | 🟡 Alpha |

### Authorization & Access Control
| Feature | Primary Module | Supporting Modules | Status | Readiness |
|---------|----------------|--------------------|--------|-----------|
| **Role-Based Access Control (RBAC)** | access_model, security, governance | auth, storage | ✅ | 🟠 Beta |
| **Row-Level Security (RLS)** | access_model, query, security | storage, updates | ✅ | 🟠 Beta |
| **Attribute-Based Access Control (ABAC)** | access_model, governance | auth, security | 🟡 | 🟡 Alpha |
| **Policy Enforcement** | governance, security | access_model, auth | ✅ | 🟠 Beta |

### Encryption
| Feature | Primary Module | Supporting Modules | Status | Readiness |
|---------|----------------|--------------------|--------|-----------|
| **Encryption at Rest** | security, storage | content, cache | ✅ | 🟢 RC |
| **Encryption in Transit** | security, network | auth | ✅ | 🟠 Beta |
| **Key Management (HSM)** | security, governance | auth | ✅ | 🟠 Beta |
| **Transparent Data Encryption (TDE)** | security, storage | encryption, metadata | ✅ | 🟠 Beta |

---

## Observability & Operations

### Monitoring & Observability
| Feature | Primary Module | Supporting Modules | Status | Readiness |
|---------|----------------|--------------------|--------|-----------|
| **Metrics Collection** | observability, performance | network, storage | ✅ | 🟠 Beta |
| **Distributed Tracing** | observability, network | process, failover | ✅ | 🟠 Beta |
| **Logging** | observability, storage | all modules | ✅ | 🟠 Beta |
| **Alerting** | observability, governance | network, failover | ✅ | 🟡 Alpha |
| **Diagnostics** | observability, failover | all modules | ✅ | 🟠 Beta |

### Performance Optimization
| Feature | Primary Module | Supporting Modules | Status | Readiness |
|---------|----------------|--------------------|--------|-----------|
| **Caching** | cache, storage | performance, query | ✅ | 🟠 Beta |
| **Query Optimization** | query, analytics, index | performance | ✅ | 🟠 Beta |
| **Resource Pooling** | execution, network, storage | performance | ✅ | 🟠 Beta |
| **Benchmarking** | performance, evaluation | all modules | ✅ | 🟠 Beta |

### Acceleration
| Feature | Primary Module | Supporting Modules | Status | Readiness |
|---------|----------------|--------------------|--------|-----------|
| **GPU Acceleration** | acceleration, gpu | distributed_tensor, onnx_clip | ✅ | 🟠 Beta |
| **CUDA Kernels** | gpu, acceleration | distributed_tensor, performance | 🟡 | 🟡 Alpha |
| **CPU Fallback** | acceleration, gpu | performance, failover | ✅ | 🟠 Beta |
| **Distributed Tensor Operations** | distributed_tensor, acceleration | gpu, network | ✅ | 🟢 RC |

---

## Advanced Features

### Knowledge Graphs
| Feature | Primary Module | Supporting Modules | Status | Readiness |
|---------|----------------|--------------------|--------|-----------|
| **Graph Storage** | graph, content | storage, index | ✅ | 🟠 Beta |
| **Graph Queries** | graph, query | retrieval, analytics | ✅ | 🟠 Beta |
| **Knowledge Graph Sync** | distributed_knowledge, graph | replication, updates | ✅ | 🟠 Beta |

### Time Series
| Feature | Primary Module | Supporting Modules | Status | Readiness |
|---------|----------------|--------------------|--------|-----------|
| **Time Series Storage** | timeseries, storage | index, cache | ✅ | 🟠 Beta |
| **Time Series Aggregation** | timeseries, analytics | storage, query | ✅ | 🟠 Beta |
| **Downsampling** | timeseries, maintenance | storage | ✅ | 🟠 Beta |

### Voice & Audio
| Feature | Primary Module | Supporting Modules | Status | Readiness |
|---------|----------------|--------------------|--------|-----------|
| **Audio Streaming** | voice, network | ingestion, observability | ✅ | 🟠 Beta |
| **Speech-to-Text** | voice, llm | onnx_clip, acceleration | ✅ | 🟠 Beta |
| **Audio Processing** | voice, acceleration | gpu, performance | ✅ | 🟠 Beta |

---

## Related Documents

| Document | Purpose |
|----------|---------|
| MODULE_INDEX.md | Module directory by alphabetical/category/tier |
| BATCH6_NAVIGATION_GUIDE.md | Cross-module navigation and integration patterns |
| WAVE_GATE_DASHBOARD.md | Wave A/B/C/D gate fulfillment tracking |
| PRODUCTION_READINESS_MATRIX.md | Module readiness levels and maturity |

---

**Batch 6 Status:** Phase 6.2 complete. Moving to Phase 6.3 (Test Suite Navigation & Benchmark Gates Reference).
