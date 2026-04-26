# Architecture Decision Log

Chronological record of all architecture decisions documented in ThemisDB.

## Format

Each row contains:
- **ID**: Sequential identifier (`ADR-NNN`)
- **Title**: Short descriptive title
- **Date**: Date the decision was made
- **Status**: `Proposed` | `Accepted` | `Deprecated` | `Superseded`
- **Modules**: Affected `src/<module>` paths

---

## Log

| ID | Title | Date | Status | Modules | Link |
|----|-------|------|--------|---------|------|
| ADR-001 | HNSW over FAISS for ANN Vector Index | 2023-06-01 | Accepted | `src/index/` | [adr_001](adr_001_hnsw_over_faiss_vector_index.md) |
| ADR-002 | RocksDB as Primary Persistent Storage Backend | 2022-11-15 | Accepted | `src/cache/`, `src/index/`, `src/rag/` | [adr_002](adr_002_rocksdb_storage_backend.md) |
| ADR-003 | Boost.Beast + Asio for HTTP/WebSocket/MQTT Server | 2022-09-01 | Accepted | `src/server/` | [adr_003](adr_003_boost_beast_asio_http_server.md) |
| ADR-004 | Native Multi-Model Data Model (Relational + Vector + Graph + Document) | 2022-07-01 | Accepted | `src/aql/`, `src/graph/`, `src/index/`, `src/query/` | [adr_004](adr_004_multi_model_data_model.md) |
| ADR-005 | Argon2id over scrypt / bcrypt for Key Derivation | 2023-04-01 | Accepted | `plugins/user_storage_encrypted/` | [adr_005](adr_005_argon2id_over_scrypt_bcrypt.md) |
| ADR-006 | Plugin-Based Adapter Architecture for Multi-Database Benchmarking (Chimera) | 2023-08-01 | Accepted | `src/chimera/` | [adr_006](adr_006_plugin_chimera_adapter_architecture.md) |
| ADR-007 | gRPC + Protobuf for Internal Service RPC | 2022-10-01 | Accepted | `src/rpc_grpc/`, `src/server/` | [adr_007](adr_007_grpc_for_internal_rpc.md) |
| ADR-008 | JWT + OAuth2 PKCE as Primary API Authentication | 2023-01-01 | Accepted | `src/server/`, `src/auth/` | [adr_008](adr_008_jwt_oauth2_for_api_auth.md) |
| ADR-009 | Systematisches Algorithm-Validation-Framework (6-Schritte-Prozess) | 2026-04-22 | Accepted | alle `src/<modul>/` (cross-cutting) | [adr_009](adr_009_algorithm_validation_framework.md) |

---

## Status Definitions

| Status | Meaning |
|--------|---------|
| `Proposed` | Decision is under discussion, not yet final |
| `Accepted` | Decision is final and in effect |
| `Deprecated` | Decision is no longer relevant (context has changed) |
| `Superseded` | Replaced by a newer decision (reference the new ADR) |

---

*Last generated: 2026-03-24*
