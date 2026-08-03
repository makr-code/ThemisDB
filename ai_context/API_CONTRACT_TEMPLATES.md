# API Contract Templates & Filling Guidelines

**Datum:** 2026-08-03  
**Status:** Active  
**Primary:** api_contracts/README.md, include/**/*.h  
**Bezug:** Maschinenlesbare API-Verträge pro Modul

## Master Template (fill one row per public function)

```markdown
| API | Namespace | Input Contract | Output Contract | Errors | Thread-Safety | Ownership/Lifetime | Notes |
|---|---|---|---|---|---|---|---|
| `functionName(...)` | `themis::module` | Preconditions (types, ranges, null checks) | Return type, invariants, validity | Possible exceptions/error codes | Concurrent calls allowed? Single-writer? | Ownership transfer? Borrowed refs? | Performance class (P0/P1/P2), C++ version required, stability |
```

## Filling Instructions

### API Column
- Function/method name with signature
- Link to header file if possible: `[name](link)`

**Example:**
```
search(std::string_view key)
```

### Namespace Column
- Full namespace path including `themis::`

**Example:**
```
themis::index
```

### Input Contract
- Parameter types & allowed values
- Preconditions (null-check required? max size? encoding?)
- Format constraints (UTF-8? ASCII-safe?)

**Example:**
```
key: non-empty UTF-8 string, max 1 KB
options: valid IndexSearchOptions or default
```

### Output Contract
- Return type & semantic meaning
- Validity guarantees (is result guaranteed non-null? immutable?)
- Invariants (sorted? deduplicated? ordered by score?)

**Example:**
```
std::optional<Value> or null if not found
Result guaranteed immutable; caller must not modify
```

### Errors Column
- Possible exceptions / error codes
- When/why they occur
- Recovery strategy (can caller retry? should it abort?)

**Example:**
```
IndexCorruptionError: corrupted internal state → abort
NotFoundError: key not found → OK to retry
TimeoutError: inference took > 30s → caller may retry
```

### Thread-Safety Column
- Can multiple threads call concurrently?
- Locking strategy (mutex per operation? RW-lock? lock-free?)
- Side effects on other threads?

**Example:**
```
✅ Thread-safe: multiple concurrent readers; writes serialized
🔒 Internal mutex per operation
⚠️ Not thread-safe: single-threaded only
```

### Ownership/Lifetime Column
- Does caller own returned pointer/reference?
- Transfer of ownership? Borrowed reference?
- Lifetime of output relative to input?

**Example:**
```
Returns std::unique_ptr<> → caller owns lifetime
Borrowed reference; valid until next call to put()
Returns view valid only within this transaction
```

### Notes Column
- Performance class (P0/P1/P2), gate if applicable
- C++ standard required (C++17, C++20?)
- API stability (public/frozen, internal/fluid, deprecated?)
- Any special requirements

**Example:**
```
P0 (Hot-path); GATE-INDEX-01 ≤10µs
Public API v1.x (stable)
Requires C++20 concepts
```

## Minimal Example (Complete Row)

```markdown
| `search(string_view)` | `themis::index` | key: UTF-8 ≤1KB | std::optional<Value>, immutable | IndexCorruptionError, TimeoutError | ✅ Multi-reader, single-writer | Returns optional; no ownership | P0; GATE-INDEX-01 ≤10µs; public v1.x |
```

## Priority Modules for API Contracts

Start with these (used heavily across codebase):

1. **api** — HTTP/gRPC/GraphQL transport interface
2. **llm** — LLM inference, model switching
3. **index** — Index operations (search, insert, delete)
4. **storage** — RocksDB K-V backend
5. **transaction** — Transaction coordinator (2PC/3PC/SAGA)
6. **auth** — Authentication & authorization

## Auto-Generation Tools

Script: `scripts/ai-context-lint.py` (optional future work)
- Scans `include/**/*.h` for public functions
- Extracts Doxygen comments
- Generates skeleton rows (human review required)

## Integration with CI/CD

**Workflow:** `.github/workflows/08-maintenance_ai-context-wiki-sync.yml`

- On PR: validate API contract rows exist for modified public APIs
- On merge: regenerate skeleton rows for new functions (optional)
- On schedule: weekly verification

## Example: api.md (Complete Sample)

See [api_contracts/api.md](./api/api.md) for filled example.

Key sections:
- Transport layer APIs (HTTP, gRPC)
- Version routing
- Error taxonomy
- Rate limiting & load shedding

---

**Zuletzt geprueft (API Contract Templates):** 2026-08-03
