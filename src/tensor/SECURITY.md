> **Sicherheitshinweis:** Security-Angaben gegen aktuelle Build-Flags, Codepfade und Tests validieren.

<!-- Status: current | validated: 2026-05-13 -->
<!-- Links: README.md · ARCHITECTURE.md · AUDIT.md · ROADMAP.md -->

# Security — Tensor Module

> Report vulnerabilities via the project-level [SECURITY.md](../../SECURITY.md).

## Threat Model

| Threat | Mitigation |
|--------|-----------|
| Tenant data leakage via shared in-memory index | Tenant key isolation enforced in-memory via `IndexHandle::key()` prefix scheme (`__ttmgr__:<tenant>:<collection>:<field>`); RocksDB-level isolation deferred to Phase 2 — see `AUDIT.md` finding TEN-01 |
| Malformed input vectors (NaN / Inf / zero-dim) | `FlatTensorIndex::addFlat()` rejects `dim == 0`; TT-SVD decomposition is called with validated shape; NaN/Inf propagation from upstream is the caller's responsibility (no internal sanitisation in Phase 1) |
| GGML zero-copy memory bridge exposing DB internals | `TensorMmapBridge` returns raw float pointers pinned to DB address space; callers MUST NOT write to these pointers; the bridge is documented as read-only; Phase-3 hardening will introduce `const float*` interface |
| Sensitive tenant/collection names in keys and logs | Key segments are validated via `makeKey()` (rejects empty or slash-containing components); debug logs use key strings, not vector data |
| Mmap pages accessible across address space | `TensorMmapBridge` uses `MAP_ANONYMOUS` + memcpy in Phase 3 stub (no shared memory with other processes); real `MAP_SHARED` on SST files is deferred to Q1 2027 |
| Stub paths silently returning incorrect results | All stubs emit `THEMIS_WARN` macros at runtime; Phase-1 stubs return `false` or `nullptr`, never silently succeed with wrong data |
| Deserialization attacks via `load()` | `FlatTensorIndex::load()` and `HnswTTBridge::load()` are Phase-1 stubs returning `false`; no deserialization occurs in Phase 1 |

## Security Controls

- **Tenant key isolation (in-memory):** The `__ttmgr__:<tenant>:<collection>:<field>`
  key scheme isolates tenant data in the in-memory hash map.
  `makeKey()` throws `std::invalid_argument` on empty or slash-containing arguments,
  preventing key-separator injection.
- **Input validation:** `addFlat()` and `add()` reject zero-dimension vectors and
  duplicate IDs, returning `false` without modifying index state.
- **Thread safety:** All `TensorIndexManager` public methods are protected by a
  `shared_mutex`.  Read operations run concurrently; write operations take an
  exclusive lock.
- **No global mutable state:** All state is encapsulated in index instances and the
  `TensorIndexManager`; no process-global variables.
- **Stub warning at runtime:** Every stub emits a `THEMIS_WARN` log entry on
  invocation, ensuring that stub paths are visible in production log analysis.
- **No raw new/delete:** Smart pointers (`std::unique_ptr`, `std::shared_ptr`)
  are used throughout; RAII ensures deterministic cleanup of index instances.

## Phase-2 Security Requirements (RocksDB Persistence)

When RocksDB persistence is added (Phase 2, Target Q4 2026), the following
controls MUST be applied:

- **RocksDB key isolation:** Apply the same `isValidTenantComponent()` guard
  used by `src/index/index_manager.cpp` (#1872) to prevent cross-tenant key
  enumeration via prefix scans.
- **RocksDB key separator injection protection:** Validate that tenant, collection,
  and field name segments do not contain the key separator character before writing.
- **Compaction filter access control:** `TensorCompactionFilter` must not expose
  core data across tenant boundaries during background compaction.

## Phase-3 Security Requirements (GGML Zero-Copy Bridge)

- **Read-only pointer contract:** Replace `float*` return type from
  `TensorIndexManager::ggmlCorePtrs()` with `const float*` to enforce read-only
  access at compile time.
- **Page alignment validation:** `TensorNetworkStorageEngine` must validate that
  core buffers are page-aligned (`sysconf(_SC_PAGESIZE)`) before exposing them
  via `TensorMmapBridge`.
- **GGUF v3 provenance metadata:** All code paths writing TT-cores to storage
  MUST attach `ProvenanceRecord` metadata.  This is a hard requirement for
  regulated-industry deployments.

## Known Limitations

- Tenant key isolation is enforced in-memory only in Phase 1.  There is no
  RocksDB-level access control preventing a process with DB access from
  reading another tenant's keys.  See `AUDIT.md` finding TEN-01.
- NaN/Inf values in input vectors are not explicitly sanitised before
  TT-SVD decomposition; callers are responsible for input validation.
- `TensorMmapBridge` is not thread-safe; it must be used from a single
  inference thread.

## Dependency Security

- `src/storage/tensor_train_decomposer` — internal; no third-party parsing.
- `src/storage/tensor_network_storage_engine` (Phase 2) — RocksDB; apply
  standard RocksDB hardening (encryption at rest, ColumnFamily ACLs).
- `src/index` (HYBRID mode) — HNSW navigation; review hnswlib CVE tracker
  before enabling Phase-2 HYBRID mode.
