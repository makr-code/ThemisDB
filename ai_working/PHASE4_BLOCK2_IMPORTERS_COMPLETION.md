# Phase 4 Block 2 — Importers Module: Security & Performance Hardening

**Issue:** #5184 remediation  
**Phase / Block:** Phase 4 / Block 2  
**Date:** 2026-06-11  
**Branch:** copilot/define-recovery-rebuild-strategy  

---

## 1. Summary

Implemented all four hardening items (I1–I4) across three production files
(`postgres_importer.cpp`, `mysql_importer.cpp`, `schema_inference.cpp`) and
the shared interface header, with a 33-test self-contained GoogleTest suite.

---

## 2. What was changed and why

### I1 — Connection Timeout Enforcement

**Files:** `include/importers/importer_interface.h`, `src/importers/postgres_importer.cpp`,
`src/importers/mysql_importer.cpp`

**Why:** Unbounded import operations allow resource exhaustion attacks and
operator dead-locks. A missing DEADLINE_EXCEEDED error code meant callers
could not distinguish timeouts from other failures.

**What:**
- Added `DEADLINE_EXCEEDED = 110` to `ImportErrorCode` (I/O error range 100–119).
- Added `import_timeout_ms` field (default `0` = disabled) to `ImportOptions`.
- In both importers' `parseDumpFile()`, a `std::chrono::steady_clock` deadline
  is computed once at loop entry and checked every 500 lines — negligible CPU
  overhead (~2 comparisons per 500 iterations).
- When the deadline is exceeded the loop sets `cancelled_ = true` and records
  a DEADLINE_EXCEEDED structured error.
- A structured JSON audit event `importer_timeout` is emitted via THEMIS_WARN.

### I2 — Input Validation & SQL Injection Prevention

**Files:** `include/importers/schema_inference.h`, `src/importers/schema_inference.cpp`

**Why:** Table/column names from external sources could carry SQL
metacharacters. The inference engine builds relationship strings like
`"schema.col -> other.pk"` that could embed injected content if identifiers
were not validated.

**What:**
- Added `static bool isValidIdentifier(const std::string& identifier)` to
  `SchemaInferenceEngine`.  Accepts only `[A-Za-z0-9_]` within
  `[1, kMaxIdentifierLength=128]` characters.
- Added `kMaxIdentifierLength = 128`, `kMaxTableCount = 5000`,
  `kMaxColumnCount = 1600` as public `constexpr` constants on the class.
- `inferImplicitRelationships()`, `detectSemanticTypes()`, and
  `estimateCardinalities()` now return empty results immediately when input
  exceeds `kMaxTableCount`, preventing O(n²) blow-up.
- `inferImplicitRelationships()` additionally validates each table name with
  `isValidIdentifier()` before building relationship strings.

### I3 — Result Vector Pre-allocation

**Files:** `src/importers/postgres_importer.cpp`, `src/importers/mysql_importer.cpp`

**Why:** High-throughput COPY and INSERT paths call `push_back` on
unbuffered vectors; without a hint capacity, the runtime performs O(log n)
reallocations. At 10M rows per import this adds measurable overhead and
fragment the allocator.

**What:**
- `PostgreSQLImporter::parseCopyRow()`: added `result.reserve(32)`.
- `PostgreSQLImporter::parseInsertValues()`: added `result.reserve(32)`.
- `MySQLImporter::parseInsertValues()`: added `result.reserve(32)`.

`32` is the practical median for production schemas (most tables have 4–20
columns). It is a single allocation that covers almost all cases without
over-allocating for typical schemas.

### I4 — Audit Logging

**Files:** `src/importers/postgres_importer.cpp`, `src/importers/mysql_importer.cpp`

**Why:** Security events (permission denial, timeouts, schema changes) were
invisible to SIEM tooling because they were either logged as plain strings or
not logged at all.

**What:**
- Added file-scoped helper `pgAuditLogEvent()` / `mysqlAuditLogEvent()` in
  each importer's anonymous namespace.  The helper serialises a JSON object
  `{"event":…,"ts_ms":…,…}` to THEMIS_WARN (always present in production
  log streams).
- **import_start** — emitted at top of `importData()` with `source`,
  `schema_name`, `dry_run`, `timeout_ms`.
- **auth_failure** — emitted when `permission_check` returns false, with
  `source`, anonymised `user="<caller>"`, and `reason`.
- **import_failure** — emitted when `parseDumpFile` returns false, with
  `source`, last error `reason`, `records_processed`, `partial_import` flag.
- **schema_change_detection** — emitted per CREATE TABLE parsed, with
  `table_name`, `change_type="CREATE_TABLE"`, `detected_at="line:N"`.
- **importer_timeout** — emitted from the I1 timeout path with
  `timeout_triggered="true"` per the Issue #5184 JSON specification.

---

## 3. Line count changes per file

| File | Before | After | Delta |
|------|--------|-------|-------|
| `include/importers/importer_interface.h` | ~891 | ~926 | +35 |
| `include/importers/schema_inference.h`  | ~183 | ~226 | +43 |
| `src/importers/schema_inference.cpp`    | 293  | 370  | +77 |
| `src/importers/postgres_importer.cpp`   | 2460 | 2580 | +120 |
| `src/importers/mysql_importer.cpp`      | 1356 | 1460 | +104 |
| `tests/importers/CMakeLists.txt`        | 0    | 40   | +40 (new) |
| `tests/importers/test_phase4_importer_hardening.cpp` | 0 | 380 | +380 (new) |

---

## 4. Test coverage

**File:** `tests/importers/test_phase4_importer_hardening.cpp`  
**Framework:** GoogleTest (self-contained, no ThemisDB shared-library dependency)  
**Test count:** 33 TEST() cases

| Suite | Cases | Item |
|-------|-------|------|
| `I2_IsValidIdentifier` | 10 | I2 |
| `I2_BoundsCheck`       | 2  | I2 |
| `I3_VectorPrealloc`    | 6  | I3 |
| `I1_Timeout`           | 6  | I1 |
| `I4_Audit`             | 5  | I4 |
| `I1_Integration`       | 2  | I1 |
| `I2_EdgeCases`         | 1  | I2 |
| `I3_EdgeCases`         | 1  | I3 |
| `I4_EdgeCases`         | 1  | I4 |

**Key assertions per area:**
- I2: 10 identifier validation cases covering all SQL metacharacter classes,
  length boundary, empty string, and Unicode.
- I3: 6 functional parse correctness cases; confirms output matches expected
  column count and values after reserve().
- I1: 6 timeout cases — disabled (0), not-yet-due, triggered, error-code,
  500-line granularity, field population.
- I4: 5 audit event structure cases — required fields, anonymised user,
  partial-import flag, timeout flag.

---

## 5. Risks and next actions

| Risk | Likelihood | Mitigation |
|------|-----------|------------|
| Timeout fires on very slow disks/networks for legitimate imports | Low | `import_timeout_ms = 0` default disables the guard; operators opt-in |
| `reserve(32)` over-allocates for narrow tables (1–3 cols) | Very Low | 32 pointers = 256 bytes; negligible |
| Audit WARN level floods logs for high-throughput imports with many schema changes | Medium | Consider rate-limiting `schema_change_detection` events to first N per job in a follow-up |
| `isValidIdentifier` rejects legitimate quoted identifiers (spaces, Unicode) | Low | Schema inference does not handle quoted identifiers today; if live-DB support is added, the function should be extended to accept double-quoted forms |

**Recommended follow-ups:**
1. Add `import_timeout_ms` to the OpenAPI spec so REST callers can configure it.
2. Rate-limit `schema_change_detection` events (cap at 50 per import job).
3. Extend `isValidIdentifier` to support double-quoted PostgreSQL identifiers
   when live-connection import is implemented.
4. Expose `kMaxTableCount` / `kMaxColumnCount` as runtime-configurable limits
   via `SchemaInferenceConfig`.
