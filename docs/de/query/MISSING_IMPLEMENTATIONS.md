# Query Module – Missing Implementations Report

**Generated:** 2026-03-09  
**Validated against:** commit `ab3b22a` (HEAD, branch `copilot/sync-documentation-with-sourcecode`)  
**Primary source:** `src/query/`, `include/query/`

---

## Executive Summary

The query module is **production-ready** as of v1.5.0.  The reality-check found no
missing features that were falsely claimed as complete.  All roadmap `[x]` items have
matching source files and/or test evidence.

Three **ghost-file references** were found in `src/query/README.md` and corrected in
this review cycle.  The secondary docs (`docs/de/query/README.md`) were significantly
out of date (December 2025) and have been updated.

The remaining open items from the ROADMAP are correctly marked as `[P]` (PR / in
progress) and do not constitute documentation drift.

---

## Findings

### FINDING-Q-001: Ghost File References in README

| Field | Value |
|---|---|
| **Severity** | Medium |
| **Status** | ✅ Fixed (commit `ab3b22a`) |
| **Claim source** | `src/query/README.md`, "Relevant Interfaces" table |
| **Expected** | Files `query_planner.cpp`, `execution_engine.cpp`, `join_executor.cpp` exist |
| **Observed** | None of these files exist in `src/query/` or anywhere in the repo |
| **Evidence** | `ls src/query/*.cpp` shows no such files |
| **Fix applied** | Table rewritten to list all real interface files |

---

### FINDING-Q-002: Incorrect Note "No Separate SQL Parser"

| Field | Value |
|---|---|
| **Severity** | High |
| **Status** | ✅ Fixed (commit `ab3b22a`) |
| **Claim source** | `src/query/README.md`, "Key Components" callout box |
| **Claim** | "There is no separate SQL parser; if SQL support is needed in the future, see the roadmap entry" |
| **Observed** | `src/query/sql_parser.cpp` (41 KB) and `include/query/sql_parser.h` exist and implement SELECT/INSERT/UPDATE/DELETE passthrough |
| **Evidence** | `ls src/query/sql_parser.cpp`, `ls tests/test_sql_parser.cpp`, `ls tests/test_sql_runner.cpp` |
| **Fix applied** | Note updated to document the actual SQL and SPARQL parsers |

---

### FINDING-Q-003: Stale "UDFs Not Supported" Limitation

| Field | Value |
|---|---|
| **Severity** | Medium |
| **Status** | ✅ Fixed (commit `ab3b22a`) |
| **Claim source** | `src/query/README.md`, "Known Limitations" §6 |
| **Claim** | "User-defined functions (UDFs) not yet supported (planned)" |
| **Observed** | `include/query/functions/udf_registry.h` and `src/query/functions/udf_registry.cpp` implement a full UDF registration API |
| **Evidence** | `ls include/query/functions/udf_registry.h`, `ls tests/test_udf_api_handler.cpp` |
| **Fix applied** | Limitation updated to describe actual status (C++ UDFs supported; WASM/Python planned) |

---

### FINDING-Q-004: Stale "No Parallel Execution" Limitation

| Field | Value |
|---|---|
| **Severity** | Low |
| **Status** | ✅ Fixed (commit `ab3b22a`) |
| **Claim source** | `src/query/README.md`, "Known Limitations" §3 |
| **Claim** | "No parallel execution within a single query (planned for v1.6.0)" |
| **Observed** | `include/query/parallel_scan.h` implements a parallel collection-scan operator; intra-operator parallelism for arbitrary plans is still planned |
| **Evidence** | `ls include/query/parallel_scan.h`, `ls tests/test_parallel_scan.cpp` |
| **Fix applied** | Limitation narrowed to describe what is and isn't yet parallel |

---

### FINDING-Q-005: Stale "No Hash Join" Limitation

| Field | Value |
|---|---|
| **Severity** | Low |
| **Status** | ✅ Fixed (commit `ab3b22a`) |
| **Claim source** | `src/query/README.md`, "Known Limitations" §3 |
| **Claim** | "Join algorithm is nested loop (no hash join or sort-merge join yet)" |
| **Observed** | `query_engine.cpp` (186 KB) implements multiple join strategies; adaptive join selection is in `FUTURE_ENHANCEMENTS.md` as a named feature |
| **Evidence** | `wc -l src/query/query_engine.cpp` = 6 000+ lines; `grep -n "hash.*join\|merge.*join" src/query/query_engine.cpp` returns hits |
| **Fix applied** | Limitation rewritten to accurately describe adaptive join selection |

---

### FINDING-Q-006: Secondary Docs Severely Stale (December 2025)

| Field | Value |
|---|---|
| **Severity** | High |
| **Status** | ✅ Fixed (commit `ab3b22a`) |
| **Claim source** | `docs/de/query/README.md`, "Stand: 5. Dezember 2025" |
| **Observed** | Component table listed only 9 components; actual module has 30 headers and 26 source files. Missing: `sql_parser`, `sparql_parser`, `query_canceller`, `vectorized_execution`, `runtime_reoptimizer`, `cross_cluster_federation`, `parallel_scan`, `result_type_annotation`, `query_resource_limits`, `udf_registry`, `workload_cache_strategy`, `query_plan_visualizer` |
| **Evidence** | `ls include/query/*.h | wc -l` = 30, `ls src/query/*.cpp | wc -l` = 26 |
| **Fix applied** | Full component table rewritten; "Stand" updated to 2026-03-09; primary doc links added |

---

### FINDING-Q-007: Ghost Source Reference in Secondary Docs

| Field | Value |
|---|---|
| **Severity** | Low |
| **Status** | ✅ Fixed (commit `ab3b22a`) |
| **Claim source** | `docs/de/query/README.md`, component table |
| **Claim** | `SubqueryOptimizer` has source `subquery_optimizer.cpp` |
| **Observed** | `include/query/subquery_optimizer.h` exists but there is no `src/query/subquery_optimizer.cpp`; the logic is incorporated in `query_optimizer.cpp` |
| **Evidence** | `ls src/query/subquery_optimizer.cpp` → "No such file" |
| **Fix applied** | Component table entry updated to note header-only |

---

## Open / Remaining Items

These are **correctly tracked** as in-progress in the ROADMAP and are **not** missing
implementations:

| Item | ROADMAP Status | Evidence |
|---|---|---|
| SQL JOIN / subquery / GROUP BY / DDL support | `[P]` (Issue #2236) | `src/query/sql_parser.cpp` documents unsupported constructs in ROADMAP Known Issues |
| Adaptive query re-optimization (full) | `[P]` (Issue #2232) | `runtime_reoptimizer.cpp` exists; full runtime feedback loop is in progress |
| Multi-statement transaction AQL (BEGIN/COMMIT) | `[x]` | `tests/test_aql_multi_statement_transaction.cpp` exists |
| Intra-operator parallelism within a single plan | Not in ROADMAP v1.5 | Correctly described as "planned for v1.6.0" in limitations |
| WASM / Python UDF runtimes | `FUTURE_ENHANCEMENTS.md` | C++ UDFs work; sandboxed runtimes are future work |
| Spill-to-disk for large intermediate results | `FUTURE_ENHANCEMENTS.md` | CTE cache has disk spill; general spill is planned |

---

## Suggested Issue Titles (for tracking)

> These are suggestions only; no auto-issues were created per DoD §4 rule.

| # | Suggested Title | Labels |
|---|---|---|
| — | `[query] SQL compatibility: add JOIN, subquery, GROUP BY, DDL support` | `enhancement`, `query`, `sql-compat` |
| — | `[query] Full intra-operator parallel execution pipeline (v1.6.0)` | `enhancement`, `query`, `performance` |
| — | `[query] WASM/Python UDF sandbox runtime` | `enhancement`, `query`, `udf` |
| — | `[query] Spill-to-disk for large intermediate result sets` | `enhancement`, `query`, `memory` |

---

*Reviewed by: Copilot agent (2026-03-09)*  
*Next review: v1.6.0 milestone*
