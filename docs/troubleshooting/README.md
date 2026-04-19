# ThemisDB Troubleshooting Guides

This directory contains detailed troubleshooting guides for all 44 ThemisDB source modules.

Each guide provides:

- **Quick Diagnostics** table — a fast reference for the most common symptoms, causes and fixes
- **Common Issues** — step-by-step solutions to the top 8–10 problems for the module
- **Diagnostic Commands** — CLI and curl commands to investigate the module at runtime
- **Configuration Reference** — the most important configuration keys with recommended values
- **Known Limitations** — documented constraints and planned future improvements
- **Related Documentation** — links to deeper module-specific docs

---

## Module Index

| Module | Description |
|--------|-------------|
| [acceleration](./acceleration_troubleshooting.md) | Hardware-agnostic compute backend (CPU/CUDA/ROCm/FAISS) |
| [analytics](./analytics_troubleshooting.md) | OLAP, CEP, anomaly detection, forecasting, materialized views |
| [api](./api_troubleshooting.md) | HTTP/REST, GraphQL, gRPC, WebSocket API servers |
| [aql](./aql_troubleshooting.md) | AQL query builder, autocomplete, optimizer advisor, migration |
| [auth](./auth_troubleshooting.md) | JWT, OAuth2 PKCE, OIDC, Kerberos, TOTP MFA, API keys |
| [base](./base_troubleshooting.md) | Dynamic module loading, hot reload, sandboxing, ABI checks |
| [cache](./cache_troubleshooting.md) | Multi-tier LRU cache, semantic cache, embedding cache, warmup |
| [cdc](./cdc_troubleshooting.md) | Change Data Capture, changefeeds, tenant buffer management |
| [chimera](./chimera_troubleshooting.md) | MongoDB wire protocol compatibility adapter |
| [config](./config_troubleshooting.md) | YAML/JSON config parsing, live reload, path resolution |
| [content](./content_troubleshooting.md) | Multi-format content processing (image, audio, CAD, archive) |
| [core](./core_troubleshooting.md) | Security initialisation, module bootstrap, startup sequence |
| [exporters](./exporters_troubleshooting.md) | Parquet, JSONL, HuggingFace, streaming export with PII detection |
| [geo](./geo_troubleshooting.md) | R-tree spatial index, GeoJSON, GDAL, radius search |
| [governance](./governance_troubleshooting.md) | Policy engine, compliance reporting, versioned policies |
| [gpu](./gpu_troubleshooting.md) | Device discovery, VRAM management, load balancing, safe-fail |
| [graph](./graph_troubleshooting.md) | Parallel graph traversal, path constraints, graph index |
| [importers](./importers_troubleshooting.md) | MongoDB, MySQL, PostgreSQL source migration |
| [index](./index_troubleshooting.md) | HNSW vector index, GPU index, temporal/graph/adaptive index |
| [ingestion](./ingestion_troubleshooting.md) | HuggingFace, filesystem, API connector ingestion pipelines |
| [llm](./llm_troubleshooting.md) | GGUF model loading, multi-GPU inference, LoRA, continuous batching |
| [metadata](./metadata_troubleshooting.md) | Schema management, constraints, index recommendations, statistics |
| [network](./network_troubleshooting.md) | Wire protocol, connection pooling, QoS, TLS, WebSocket |
| [observability](./observability_troubleshooting.md) | Prometheus metrics, query profiler, continuous profiler, alerting |
| [performance](./performance_troubleshooting.md) | NUMA, hugepages, mimalloc, bloom filter, async metrics export |
| [plugins](./plugins_troubleshooting.md) | Plugin lifecycle, health monitoring, hot-plug, edition gating |
| [prompt\_engineering](./prompt_engineering_troubleshooting.md) | Prompt templates, injection detection, optimiser, feedback |
| [query](./query_troubleshooting.md) | AQL parser, cost-based optimizer, CTE, window functions, federation |
| [rag](./rag_troubleshooting.md) | Retrieval-augmented generation, hallucination detection, A/B testing |
| [replication](./replication_troubleshooting.md) | Raft replication, WAL streaming, hot-spare, conflict resolution |
| [scheduler](./scheduler_troubleshooting.md) | Distributed cron, event triggers, task anomaly detection, audit |
| [search](./search_troubleshooting.md) | BM25, hybrid search, faceted search, LTR, LLM reranking |
| [security](./security_troubleshooting.md) | RBAC/ABAC, field encryption, HSM/PKCS#11, AQL injection detection |
| [server](./server_troubleshooting.md) | Admin API, auth middleware, API gateway, tenant isolation |
| [sharding](./sharding_troubleshooting.md) | Consistent hash routing, Raft, circuit breaker, auto-rebalancer |
| [storage](./storage_troubleshooting.md) | RocksDB, WAL, PITR, compaction, blob backends (S3/Azure/WebDAV) |
| [temporal](./temporal_troubleshooting.md) | Bi-temporal tables, PITR, HLC, conflict resolution, retention |
| [themis](./themis_troubleshooting.md) | Edition management, module hash verification, dependency resolution |
| [timeseries](./timeseries_troubleshooting.md) | Hypertables, Gorilla compression, continuous aggregates, retention |
| [training](./training_troubleshooting.md) | LoRA fine-tuning, auto-labelling, knowledge graph enrichment |
| [transaction](./transaction_troubleshooting.md) | MVCC, 2PC, saga, deadlock detection, crash recovery, branching |
| [updates](./updates_troubleshooting.md) | Schema migration, canary rollout, hot reload, release manifests |
| [utils](./utils_troubleshooting.md) | Audit logger, cron parser, cursor management, compression |
| [voice](./voice_troubleshooting.md) | ASR (Whisper), TTS (Piper), speaker auth, VAD, batch processing |

---

## How to Use These Guides

1. **Identify the module** involved in the problem (look at the error log prefix).
2. **Open the module's guide** and check the Quick Diagnostics table first.
3. **Run the diagnostic commands** to gather evidence.
4. **Follow the step-by-step solution** for your specific issue.
5. **Check the Configuration Reference** to verify your settings.

## Common Cross-Module Issues

| Issue | Modules Involved | See |
|-------|-----------------|-----|
| Startup fails | core, config, security | [core](./core_troubleshooting.md), [config](./config_troubleshooting.md) |
| High latency | query, cache, index, sharding | [query](./query_troubleshooting.md), [cache](./cache_troubleshooting.md) |
| Authentication failures | auth, security, server | [auth](./auth_troubleshooting.md) |
| Data not appearing | ingestion, cdc, replication | [ingestion](./ingestion_troubleshooting.md), [cdc](./cdc_troubleshooting.md) |
| Disk space exhaustion | storage, observability, scheduler | [storage](./storage_troubleshooting.md) |
| GPU/ML errors | llm, gpu, acceleration, training | [llm](./llm_troubleshooting.md), [gpu](./gpu_troubleshooting.md) |

## General Diagnostic Commands

```bash
# Overall server health
curl http://localhost:8080/health

# Prometheus metrics
curl -s http://localhost:9100/metrics | head -50

# Admin API status
curl -H "Authorization: Bearer $ADMIN_TOKEN" \
     http://localhost:9090/admin/status

# Tail all ThemisDB logs
journalctl -u themisdb -f

# Server version and build info
themisdb --version
themisdb-admin utils build-info
```

## See Also

- [Architecture Overview](../../ARCHITECTURE.md)
- [Setup Guide](../../SETUP.md)
- [Operations Guide](../operational_guide.md)
- [Performance Tuning](../performance/PERFORMANCE_ROADMAP.md)
- [Security Executive Summary](../SECURITY_EXECUTIVE_SUMMARY.md)
