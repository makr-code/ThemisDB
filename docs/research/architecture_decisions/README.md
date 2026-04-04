# Architecture Decisions — Index

This directory documents significant architecture decisions using an ADR-like format (Architecture Decision Records).

## Purpose

Whenever a non-trivial design choice is made — especially when multiple options were evaluated — an entry here captures:

- The context and constraints
- The options considered
- The decision made and its rationale
- The trade-offs accepted

## Decision Log

See [decision_log.md](decision_log.md) for a chronological list of all decisions.

## Index by Status

### ✅ Accepted
| ID | Decision | Date | Modules |
|----|----------|------|---------|
| [ADR-001](adr_001_hnsw_over_faiss_vector_index.md) | HNSW over FAISS for ANN Vector Index | 2023-06-01 | `src/index/` |
| [ADR-002](adr_002_rocksdb_storage_backend.md) | RocksDB as Primary Persistent Storage Backend | 2022-11-15 | `src/cache/`, `src/index/`, `src/rag/` |
| [ADR-003](adr_003_boost_beast_asio_http_server.md) | Boost.Beast + Asio for HTTP/WebSocket/MQTT Server | 2022-09-01 | `src/server/` |
| [ADR-004](adr_004_multi_model_data_model.md) | Native Multi-Model Data Model (Relational + Vector + Graph + Document) | 2022-07-01 | `src/aql/`, `src/graph/`, `src/index/`, `src/query/` |
| [ADR-005](adr_005_argon2id_over_scrypt_bcrypt.md) | Argon2id over scrypt / bcrypt for Key Derivation | 2023-04-01 | `plugins/user_storage_encrypted/` |
| [ADR-006](adr_006_plugin_chimera_adapter_architecture.md) | Plugin-Based Adapter Architecture for Multi-Database Benchmarking (Chimera) | 2023-08-01 | `src/chimera/` |
| [ADR-007](adr_007_grpc_for_internal_rpc.md) | gRPC + Protobuf for Internal Service RPC | 2022-10-01 | `src/rpc_grpc/`, `src/server/` |
| [ADR-008](adr_008_jwt_oauth2_for_api_auth.md) | JWT + OAuth2 PKCE as Primary API Authentication | 2023-01-01 | `src/server/`, `src/auth/` |

### 🔄 Proposed / Under Review
| ID | Decision | Date | Modules |
|----|----------|------|---------|
| *(none yet)* | | | |

### ⛔ Superseded
| ID | Decision | Superseded By | Date |
|----|----------|---------------|------|
| *(none yet)* | | | |

## Adding a New Decision

1. Copy [_template_decision.md](_template_decision.md) to `adr_<NNN>_<short_title>.md`  
   Example: `adr_001_vector_index_choice.md`
2. Assign the next sequential ID from [decision_log.md](decision_log.md)
3. Fill in all required sections
4. Link it in the relevant module README and in [implementation_influence/README.md](../implementation_influence/README.md)

## Naming Convention

```
adr_<NNN>_<short_title>.md
```

Examples:
- `adr_001_vector_index_hnsw_vs_faiss.md`
- `adr_002_storage_engine_rocksdb.md`
- `adr_003_consensus_raft.md`

## See Also

- [decision_log.md](decision_log.md) — chronological log
- [_template_decision.md](_template_decision.md) — copy-paste template
- [../RESEARCH_GUIDE.md](../RESEARCH_GUIDE.md) — contributor workflow
