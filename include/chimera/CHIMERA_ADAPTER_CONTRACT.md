# Chimera Adapter Module — Frozen Contract Semantics

**Version:** 1.0.0 (Frozen)  
**Status:** FROZEN — Q3 2026 gate delivery  
**Last Validated:** 2026-08-09  
**Frozen By:** Copilot agent (roadmap gate: Phase 1 Design / API Contract)

---

## 1. Scope

This document defines the **frozen adapter contract semantics** for the
Chimera database adapter module, and the **explicit error taxonomy** for
connection, capability, and dispatch failure classes.

Chimera provides a uniform `IDatabaseAdapter` interface over heterogeneous
backends (MongoDB, Neo4j, Qdrant, ThemisDB).  This contract governs how
adapters must behave under error conditions across all backend implementations.

---

## 2. Adapter Contract Semantics

### 2.1 Lifecycle Contract

| Method | Contract |
|---|---|
| `connect()` | Establishes one or more connections.  Returns `Result<void>` with `CONNECTION_ERROR` if unreachable. |
| `disconnect()` | Tears down all connections.  Must be idempotent — safe to call multiple times. |
| `isConnected()` | Returns `true` only when a valid, usable connection exists. |
| `ping()` | Round-trips a minimal request to verify the connection.  Returns `false` on timeout or error. |

**Lifecycle invariants:**
- All query/command methods MUST check connection state and return
  `Result::err(CONNECTION_ERROR, ...)` when not connected, rather than
  throwing or segfaulting.
- After `disconnect()`, calling `isConnected()` MUST return `false`.
- Re-calling `connect()` after a previous successful connect MUST either
  succeed (idempotent) or return `ALREADY_EXISTS`.

### 2.2 Query / Command Dispatch Contract

All methods that dispatch to the backend return `Result<T>`:

- `query(sql) → Result<RelationalTable>`
- `execute(sql) → Result<void>`
- `vectorSearch(...) → Result<...>`

**Dispatch guarantees:**
- A `Result::ok(...)` return means the backend accepted and processed the request.
- A `Result::err(...)` return is always accompanied by a non-empty `error_message`.
- Adapters MUST NOT throw exceptions from public methods — all errors are
  propagated via `Result::err(...)`.

---

## 3. Error Taxonomy

### 3.1 Frozen Error Code Table

The following error codes are frozen in `chimera::ErrorCode` (see `database_adapter.hpp`):

| Code | Value | Class | Retryable | Semantics |
|---|---|---|---|---|
| `SUCCESS` | 0 | — | — | Operation succeeded |
| `NOT_IMPLEMENTED` | 1 | Contract | No | Method exists in interface but not in this adapter implementation |
| `INVALID_ARGUMENT` | 2 | Input | No | Caller provided a malformed query or invalid parameter |
| `NOT_FOUND` | 3 | Data | No | Referenced document/node/vector does not exist |
| `ALREADY_EXISTS` | 4 | Data | No | Insert rejected: entity already present |
| `PERMISSION_DENIED` | 5 | Auth | No | Caller lacks permission for the requested operation |
| `CONNECTION_ERROR` | 6 | Connection | **Yes** (transient) | Backend unreachable, refused connection, or protocol error |
| `TIMEOUT` | 7 | Connection | **Yes** (with backoff) | Request exceeded time budget |
| `RESOURCE_EXHAUSTED` | 8 | Resource | **Yes** (with delay) | Backend reports too many connections, memory, or I/O saturation |
| `INTERNAL_ERROR` | 9 | Internal | No | Unclassified backend error |
| `UNSUPPORTED` | 10 | Contract | No | Backend does not support a requested feature at the protocol level |
| `TRANSACTION_ABORTED` | 11 | Transaction | **Yes** (application retry) | Backend aborted the transaction (serialization failure, etc.) |
| `CONSTRAINT_VIOLATION` | 12 | Data | No | Write rejected by a backend constraint |
| `DEADLOCK` | 13 | Transaction | **Yes** (application retry) | Deadlock detected; caller should retry with backoff |
| `DISPATCH_FAILED` | 14 | Dispatch | Conditional | Dispatch could not be forwarded to backend; retryable only if cause is transient |
| `CAPABILITY_MISMATCH` | 15 | Contract | No | The adapter does not support the requested capability |

### 3.2 Connection Failure Contract (`CONNECTION_ERROR` = 6)

**Triggering conditions:**
- TCP/TLS connection refused or reset
- DNS resolution failure
- Handshake or authentication failure
- Keep-alive probe timeout

**Caller contract:**
- `CONNECTION_ERROR` is **retryable** with exponential backoff
- Maximum retry budget is caller-controlled; the adapter itself does not retry
- After `max_retries` exhausted, callers SHOULD surface a connection error to
  the operator and stop retrying

### 3.3 Dispatch Failure Contract (`DISPATCH_FAILED` = 14)

**Triggering conditions:**
- Connection exists but the request serialization or wire send failed
- Backend accepted the connection but rejected the specific command format
- Partial write before a connection drop

**Caller contract:**
- `DISPATCH_FAILED` is **conditionally retryable**: retryable only when the
  specific `error_message` indicates a transient transport error; non-retryable
  when the message indicates a protocol or format rejection
- Adapters MUST include a machine-readable prefix in `error_message`:
  - `"[TRANSIENT]"` → safe to retry
  - `"[FORMAT_REJECTED]"` → non-retryable, fix the command

### 3.4 Capability Mismatch Contract (`CAPABILITY_MISMATCH` = 15)

**Triggering conditions:**
- Vector search (`vectorSearch()`) called on a relational-only adapter
  (MongoDB text-only, plain SQL)
- Graph traversal called on a non-graph adapter (Qdrant, plain SQL)
- Any feature requiring a backend extension the adapter was not configured with

**Caller contract:**
- `CAPABILITY_MISMATCH` is **non-retryable**
- Callers MUST NOT retry with the same adapter configuration
- Callers SHOULD select a different adapter or disable the feature flag

---

## 4. Backward Compatibility

- Error codes 0–13 are **immutably frozen** at their current values.
- Codes 14 (`DISPATCH_FAILED`) and 15 (`CAPABILITY_MISMATCH`) are frozen
  as of this document.
- New error codes may be added (additive) without a major version bump.
- Removal or renumbering of any existing code requires a **major version bump**.

---

## 5. Contract Validation

Tests enforcing this contract are in:

- `tests/chimera/` — adapter lifecycle and dispatch tests
- See also `include/chimera/database_adapter.hpp` for the frozen `ErrorCode` enum.
