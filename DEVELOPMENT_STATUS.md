# DEVELOPMENT_STATUS

## Development Status

As of **2026-04-12 (UTC)**, the current released version of ThemisDB is **v1.8.0**.

### Completed Phases

1. **Phase 1: Initial Setup**
   - Version: 1.0.0
   - Date: 2023-01-10
   - Description: Initial project setup with basic database functionality (RocksDB storage, CRUD, basic indexing).

2. **Phase 2: Core Functionality**
   - Version: 1.1.0
   - Date: 2023-05-15
   - Description: MVCC transactions, snapshot isolation, multi-model data support (document, graph, vector, relational).

3. **Phase 3: User Management & Security**
   - Version: 1.2.0
   - Date: 2023-09-20
   - Description: JWT authentication, RBAC, field-level AES-256-GCM encryption, audit logging.

4. **Phase 4: API Integration**
   - Version: 1.3.0
   - Date: 2024-01-05
   - Description: REST API, AQL query language, 100+ built-in functions, CTE support.

5. **Phase 5: Performance Optimization & Modular Architecture**
   - Version: 1.4.0
   - Date: 2024-10-15
   - Description: HNSW GPU vector search, SIMD analytics (4.5×–6.9× speedup), modular build system,
     FLARE adaptive retrieval, circuit breakers, auto-retry, 99.99% corruption detection.

6. **Phase 6: Distributed Systems & AI Integration**
   - Version: 1.5.0
   - Date: 2026-03-12
   - Description: Raft consensus replication (50K–100K writes/sec), PKCS#11 HSM + RFC 3161 TSA,
     HTTP/3 (QUIC), gRPC server, MQTT broker, PostgreSQL wire protocol, MCP server,
     blue/green deployment, HSM-backed SigningService, 3-stage hybrid retention (99.9% compression),
     real-time meeting transcription (STT/TTS), post-quantum crypto (Kyber/Dilithium).

7. **Phase 7: Observability, Search & Data Integration**
   - Version: 1.7.0
   - Date: 2026-03-09
   - Description: Config architecture reorganization (hierarchical `config/` with `ConfigPathResolver`),
     HybridSearch production hardening (configurable vector metric, `SearchStats`, exception safety),
     distributed query optimizer (dynamic shard row estimates, predicate selectivity),
     FAISS ADC distance tables (~40% faster search), full 44-module documentation audit,
     Root Cause Analyzer for observability.

8. **Phase 8: Security Hardening, Transactions & Protocol Completeness**
   - Version: 1.8.0
   - Date: 2026-03-22
   - Description: JWT scope enforcement (OAuth2), ArrowUserRegistrationPlugin (Arrow-backed user store),
     CRL/OCSP certificate revocation, Serializable Snapshot Isolation (SSI) with predicate locks,
     SAGA Orchestration Engine, versioned API routing (`/v1/`, `/v2/`), Wire Protocol V2,
     Geo clustering (DBSCAN/K-means), UDP ingestion + QoS, MySQL/MariaDB importer,
     GraphQL WebSocket use-after-free fix, async job API.

### Active Work Items (v1.9.0 — Unreleased)

**Selected highlights across modules:**
- **Phase 4 completion:** Epoch fencing (4.1), automatic failover orchestration (4.2), chaos framework (4.3), and disaster recovery orchestration (4.4) are implemented and phase4-focused tests are green.
- **Cache (v1.9.0):** Lock-free L1 read path (`std::shared_mutex`), atomic `L1Entry` fields, lazy expiry via CAS. `RequestCoalescer` real Singleflight implementation shipped ([#4580](https://github.com/makr-code/ThemisDB/issues/4580): promise/shared_future inflight map, 14 tests RC-01…RC-14).
- **Prompt Engineering:** Typed DSL for structured prompt authoring, token budget manager, context-window enforcement.
- **Search:** Additional HybridSearch and multi-modal search hardening.
- **Server (MQTT, v1.9.0):** `MqttClientService` + `MqttCDCTransport`, Boost.Asio async I/O, RPCServiceRegistry integration.
- **Prompt Library IO (v2.0.0):** `PromptLibraryBundle` with FNV-1a checksum, JSON+YAML export, A/B experiment framework.
- **Analytics:** `IStreamingJoin` / `HashJoin` / `IntervalJoin` shipped ([#4576](https://github.com/makr-code/ThemisDB/issues/4576): composite-key hash table, inner/left-outer join, `IntervalJoin` with LRU pruning, 15 tests SJ-01…SJ-15) (2026-04-12).
- **Storage:** `StreamingIngestManager` ([#4571](https://github.com/makr-code/ThemisDB/issues/4571): ring-buffer + flush-thread, ≥1 M events/s) and `ColumnarCache` ([#4572](https://github.com/makr-code/ThemisDB/issues/4572): LRU + PinGuard RAII) shipped (2026-04-12).
- **Timeseries:** `TsStreamCursor` ([#4573](https://github.com/makr-code/ThemisDB/issues/4573): lazy paginated iterator, page_size=4 096) and `TSStore::putBatch` ([#4574](https://github.com/makr-code/ThemisDB/issues/4574): zero-copy batch write via single `WriteBatch`) shipped (2026-04-12).
- **Temporal:** `TemporalCompressor` LZ4 support added ([#4575](https://github.com/makr-code/ThemisDB/issues/4575)) (2026-04-12).
- **Performance:** `LockFreeHistogram<T>` header-only with atomic buckets, P50/P90/P99 ([#4577](https://github.com/makr-code/ThemisDB/issues/4577)); LIRS cache `shared_mutex` race fixed ([#4578](https://github.com/makr-code/ThemisDB/issues/4578)); RCU `readers_active()` fixed ([#4579](https://github.com/makr-code/ThemisDB/issues/4579)) (2026-04-12).
- **Acceleration:** `AiHardwareDispatcher` v1.0 with NPU priority chain shipped ([#4584](https://github.com/makr-code/ThemisDB/issues/4584)); NCCL/RCCL `mergeTopK` complete ([#4585](https://github.com/makr-code/ThemisDB/issues/4585)); all Phase 1–6 items now [x] (2026-04-12).
- **Network:** `IoUringBatchedSender` (single `io_uring_enter()` for N WireProtocolBatcher flushes) shipped ([#4581](https://github.com/makr-code/ThemisDB/issues/4581)) (2026-04-12).
- **Utils:** UUID v7 (`generate_uuid_v7()`, RFC 9562) ([#4582](https://github.com/makr-code/ThemisDB/issues/4582)) and streaming ZSTD (`zstd_compress_stream`/`zstd_decompress_stream`) ([#4583](https://github.com/makr-code/ThemisDB/issues/4583)) shipped (2026-04-12).
- **Maintenance:** MVCC_CLEANUP ([#4586](https://github.com/makr-code/ThemisDB/issues/4586)) and STORAGE_COMPACTION ([#4587](https://github.com/makr-code/ThemisDB/issues/4587)) wired in `http_server.cpp` (2026-04-12).
- **Index:** Concurrent-Unique-Lücke fix (sentinel key locking in `updateIndexesForPut_`) ([#4588](https://github.com/makr-code/ThemisDB/issues/4588)); secondary index performance improvements via `SecondaryIndexMetadataCache` ([#4589](https://github.com/makr-code/ThemisDB/issues/4589)) (2026-04-12).
- **Stable Diffusion plugin v2.2.0** ([#4590](https://github.com/makr-code/ThemisDB/issues/4590)): `SDCppGenerator` wrapping `stable-diffusion.cpp` C API (THEMIS_ENABLE_STABLE_DIFFUSION); real IDAT PNG encoder (`encodeMinimalPng` stored-deflate + CRC32 + Adler32); `SDStubGenerator::generateImg2Img` pass-through; 51 tests A-Q (2026-04-12).
- **WhisperPlugin v2.1.0** ([#4591](https://github.com/makr-code/ThemisDB/issues/4591)): thread-safe via `transcriber_mutex_` + `std::atomic` counters; `FfmpegAudioChunkReader` (popen ffmpeg, shell-escaped path, 500 MB cap); `CompositeAudioChunkReader`; 36 tests A-L (2026-04-12).
- **Sharding: Paxos WAL durability** ([#4592](https://github.com/makr-code/ThemisDB/issues/4592)): `handlePrepare`/`handleAccept` call `wal_->logPromise()`/`logAccept()` before returning; `recoverFromWAL()` restores instances on restart; 10 tests PSR-01…PSR-10 (2026-04-12).
- **Sharding: `ShardRPCClient::writeEntity()`** ([#4593](https://github.com/makr-code/ThemisDB/issues/4593)): gRPC `ReplicateData` RPC for cross-shard writes; `handleWriteEntityGrpc()` wired in `sendRequestGrpc()` (2026-04-12).

**Current focus after Phase 4:**
- Phase 5 operational tooling (admin CLI, repair dashboard, ops runbooks).
- System-wide validation gates: cluster-level fault-injection in CI, 99.99% SLA validation under load+faults, and penetration-test reporting.

For individual module work items, refer to each module's `src/<module>/CHANGELOG.md` `[Unreleased]` section.

### Realistic Timelines

- **v1.9.0 release:** Q2 2026 (target)
- **Phase 5 operational tooling baseline:** Q2/Q3 2026 (target)
- **v2.0 feature goals:** GPU-accelerated vector search production deployment, hyperscale operations, advanced AI hardening — see [roadmap.md](roadmap.md)

For further updates, refer to the official repository, [roadmap.md](roadmap.md), and individual module changelogs in `src/`.