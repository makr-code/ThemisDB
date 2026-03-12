### Context

This issue implements the roadmap item 'gRPC API Surface — Wire Stub Implementations' for the api domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v2.0.0.

Primary detail section: gRPC API Surface — Wire Stub Implementations

### Goal

Deliver the scoped changes for gRPC API Surface — Wire Stub Implementations in src/api/ and complete the linked detail section in a release-ready state for v2.0.0.

### Detailed Scope

### gRPC API Surface — Wire Stub Implementations
**Priority:** High
**Target Version:** v2.0.0

`themisdb_grpc_service.cpp` reports "Open Issues: Stubs: 4" in its header. All five non-CRUD RPC methods return `grpc::StatusCode::UNIMPLEMENTED`. The gRPC surface cannot be used for its primary value (AQL execution, vector search) until these stubs are replaced with real engine delegation via `ThemisDBGrpcServiceFactory`.

**Implementation Notes:**
- `[x]` Create `src/api/grpc_server.cpp`; gRPC C++ server using `grpc::ServerBuilder` (synchronous dispatch model, consistent with the rest of the codebase).
- `[x]` Reuse existing service-layer infrastructure via `GrpcApiServer::registerService()`; no business logic duplication — service implementations are registered externally.
- `[x]` TLS: `grpc::SslServerCredentials` using the same PEM cert/key pair as the Beast HTTP listener; fail-closed on cert load failure.
- `[x]` Expose gRPC reflection service in debug builds only to prevent schema leakage in production.
- `[ ]` **`ExecuteAQL` stub** (`themisdb_grpc_service.cpp:~line 302`): returns `UNIMPLEMENTED` with message "AQL execution requires an AQLEngine; wire one in via ThemisDBGrpcServiceFactory". Implement `ThemisDBGrpcServiceFactory` that accepts an `AQLEngine*` and injects it into `ServiceImpl` so `ExecuteAQL` can delegate to `engine_->execute(req->query(), ...)`.
- `[ ]` **`StreamAQL` stub** (`themisdb_grpc_service.cpp:~line 337`): the comment block already shows the exact streaming loop implementation needed (inject `AQLEngine`, call `executeStreaming`, write rows via `writer->Write(row)`). The code exists as a comment — uncomment and wire after `ThemisDBGrpcServiceFactory` provides the engine. Implement server-side streaming RPC `StreamAQL(AQLQueryRequest) returns (stream AQLRow)`.
- `[ ]` **`VectorSearch` stub** (`themisdb_grpc_service.cpp:~line 354`): returns `UNIMPLEMENTED`. Add a `VectorIndex*` injection point to `ServiceImpl` (parallel to the `AQLEngine*` injection) and delegate to `vector_index_->search(req->collection(), req->vector(), req->k())`.
- `[ ]` **`FilteredVectorSearch` stub** (`themisdb_grpc_service.cpp:~line 367`): returns `UNIMPLEMENTED`, message "filtered vector search not yet wired". Wire alongside `VectorSearch` in the same injection pass.
- `[ ]` **`HybridSearch` stub** (`themisdb_grpc_service.cpp:~line 380`): returns `UNIMPLEMENTED`. Implement after `VectorSearch` and full-text index injection are complete.
- `[ ]` **`FullTextSearch` stub** (`themisdb_grpc_service.cpp:~line 393`): returns `UNIMPLEMENTED`, message "full-text search not yet wired". Add `FullTextIndex*` injection point alongside `VectorIndex*`.
- `[ ]` **Hard-coded document version** in `CreateDocument` and `UpdateDocument` (`themisdb_grpc_service.cpp`): both handlers unconditionally set `resp->set_version(1)`, regardless of whether the document already existed. Add a real version counter sourced from the storage layer (e.g., a RocksDB sequence number or a dedicated version key) so optimistic-concurrency clients can detect conflicting updates.
- `[ ]` **`BatchWrite` silent partial failures** (`themisdb_grpc_service.cpp`): the loop over `req->upserts()` increments `upserted` only when `db_->put(key, body)` returns true, but the final response always sets `resp->set_success(true)`. If some puts fail (e.g., storage full), the caller receives a success response with a `upserted_count` less than the number of requested writes, with no error code. Change to: if `upserted_count != req->upserts_size()`, set `success = false` and include error details.
- `[ ]` **`BatchWrite`/`BatchRead` lack input bounds checks**: no validation of the number of documents in `req->upserts()` or keys in `req->keys()`. A single request can contain arbitrarily many items, leading to unbounded memory allocation. Add a hard upper limit (e.g., 10,000 items) with a `RESOURCE_EXHAUSTED` gRPC status code on violation.
- `[ ]` **`GrpcApiServer::start()` holds `mutex_` across `BuildAndStart()`** (`grpc_server.cpp:start()`): `builder.BuildAndStart()` performs a blocking socket bind and TLS handshake inside a `std::lock_guard<std::mutex> lock(mutex_)`. If the port is unavailable or TLS cert loading is slow, `isRunning()` and `stop()` are both blocked for the entire duration. Extract the `ServerBuilder` setup before acquiring the lock; acquire the lock only to store `server_` and set `running_ = true`.
- `[ ]` **`GrpcApiServer::stop()` holds `mutex_` during `server_->Shutdown()`** (`grpc_server.cpp:stop()`): `Shutdown()` without a deadline can block indefinitely waiting for in-flight RPCs. Use `server_->Shutdown(std::chrono::system_clock::now() + std::chrono::seconds(30))` and release the mutex before calling `Shutdown()` to avoid deadlocking callers of `isRunning()` during shutdown.
- `[ ]` **`GrpcServerConfig::max_message_size_bytes` hard-coded** (`grpc_server.h`): default value of `100 * 1024 * 1024` (100 MB) is set in the struct definition rather than loaded from `config/networking/`. Expose as a config key (e.g., `grpc.max_message_size_mb`) loaded at `GrpcApiServer::initialize()` time so operators can tune it without recompiling.

**Performance Targets:**
- gRPC unary `GetDocument` < 1 ms added latency vs equivalent REST call (same process).
- gRPC streaming `ExecuteQuery` sustains ≥ 100,000 rows/sec on localhost.

---

### Acceptance Criteria

- [ ] Create `src/api/grpc_server.cpp`; gRPC C++ server using `grpc::ServerBuilder` (synchronous dispatch model, consistent with the rest of the codebase).
- [ ] Reuse existing service-layer infrastructure via `GrpcApiServer::registerService()`; no business logic duplication — service implementations are registered externally.
- [ ] TLS: `grpc::SslServerCredentials` using the same PEM cert/key pair as the Beast HTTP listener; fail-closed on cert load failure.
- [ ] Expose gRPC reflection service in debug builds only to prevent schema leakage in production.
- [ ] **`ExecuteAQL` stub** (`themisdb_grpc_service.cpp:~line 302`): returns `UNIMPLEMENTED` with message "AQL execution requires an AQLEngine; wire one in via ThemisDBGrpcServiceFactory". Implement `ThemisDBGrpcServiceFactory` that accepts an `AQLEngine*` and injects it into `ServiceImpl` so `ExecuteAQL` can delegate to `engine_->execute(req->query(), ...)`.
- [ ] **`StreamAQL` stub** (`themisdb_grpc_service.cpp:~line 337`): the comment block already shows the exact streaming loop implementation needed (inject `AQLEngine`, call `executeStreaming`, write rows via `writer->Write(row)`). The code exists as a comment — uncomment and wire after `ThemisDBGrpcServiceFactory` provides the engine. Implement server-side streaming RPC `StreamAQL(AQLQueryRequest) returns (stream AQLRow)`.
- [ ] **`VectorSearch` stub** (`themisdb_grpc_service.cpp:~line 354`): returns `UNIMPLEMENTED`. Add a `VectorIndex*` injection point to `ServiceImpl` (parallel to the `AQLEngine*` injection) and delegate to `vector_index_->search(req->collection(), req->vector(), req->k())`.
- [ ] **`FilteredVectorSearch` stub** (`themisdb_grpc_service.cpp:~line 367`): returns `UNIMPLEMENTED`, message "filtered vector search not yet wired". Wire alongside `VectorSearch` in the same injection pass.
- [ ] **`HybridSearch` stub** (`themisdb_grpc_service.cpp:~line 380`): returns `UNIMPLEMENTED`. Implement after `VectorSearch` and full-text index injection are complete.
- [ ] **`FullTextSearch` stub** (`themisdb_grpc_service.cpp:~line 393`): returns `UNIMPLEMENTED`, message "full-text search not yet wired". Add `FullTextIndex*` injection point alongside `VectorIndex*`.
- [ ] **Hard-coded document version** in `CreateDocument` and `UpdateDocument` (`themisdb_grpc_service.cpp`): both handlers unconditionally set `resp->set_version(1)`, regardless of whether the document already existed. Add a real version counter sourced from the storage layer (e.g., a RocksDB sequence number or a dedicated version key) so optimistic-concurrency clients can detect conflicting updates.
- [ ] **`BatchWrite` silent partial failures** (`themisdb_grpc_service.cpp`): the loop over `req->upserts()` increments `upserted` only when `db_->put(key, body)` returns true, but the final response always sets `resp->set_success(true)`. If some puts fail (e.g., storage full), the caller receives a success response with a `upserted_count` less than the number of requested writes, with no error code. Change to: if `upserted_count != req->upserts_size()`, set `success = false` and include error details.
- [ ] **`BatchWrite`/`BatchRead` lack input bounds checks**: no validation of the number of documents in `req->upserts()` or keys in `req->keys()`. A single request can contain arbitrarily many items, leading to unbounded memory allocation. Add a hard upper limit (e.g., 10,000 items) with a `RESOURCE_EXHAUSTED` gRPC status code on violation.
- [ ] **`GrpcApiServer::start()` holds `mutex_` across `BuildAndStart()`** (`grpc_server.cpp:start()`): `builder.BuildAndStart()` performs a blocking socket bind and TLS handshake inside a `std::lock_guard<std::mutex> lock(mutex_)`. If the port is unavailable or TLS cert loading is slow, `isRunning()` and `stop()` are both blocked for the entire duration. Extract the `ServerBuilder` setup before acquiring the lock; acquire the lock only to store `server_` and set `running_ = true`.
- [ ] **`GrpcApiServer::stop()` holds `mutex_` during `server_->Shutdown()`** (`grpc_server.cpp:stop()`): `Shutdown()` without a deadline can block indefinitely waiting for in-flight RPCs. Use `server_->Shutdown(std::chrono::system_clock::now() + std::chrono::seconds(30))` and release the mutex before calling `Shutdown()` to avoid deadlocking callers of `isRunning()` during shutdown.
- [ ] **`GrpcServerConfig::max_message_size_bytes` hard-coded** (`grpc_server.h`): default value of `100 * 1024 * 1024` (100 MB) is set in the struct definition rather than loaded from `config/networking/`. Expose as a config key (e.g., `grpc.max_message_size_mb`) loaded at `GrpcApiServer::initialize()` time so operators can tune it without recompiling.
- [ ] gRPC unary `GetDocument` < 1 ms added latency vs equivalent REST call (same process).
- [ ] gRPC streaming `ExecuteQuery` sustains ≥ 100,000 rows/sec on localhost.

### Relationships

- Roadmap row: #51 (🟠 High Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/api/FUTURE_ENHANCEMENTS.md#grpc-api-surface--wire-stub-implementations
- Source key: roadmap:51:api:v2.0.0:grpc-api-surface-wire-stub-implementations

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:51:api:v2.0.0:grpc-api-surface-wire-stub-implementations -->
<!-- roadmap-ref: row=51;module=api;target=v2.0.0 -->
<!-- roadmap-detail: src/api/FUTURE_ENHANCEMENTS.md#grpc-api-surface--wire-stub-implementations -->
