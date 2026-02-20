# ADR-003: SQL Parser Library for pg_dump Import

**Status:** Accepted  
**Date:** 2026-02-20  
**Deciders:** ThemisDB Core Team  
**Scope:** PostgreSQL importer module (`src/importers/postgres_importer.cpp`)

---

## Context

The PostgreSQL pg_dump importer originally used plain regular expressions to parse
DDL (`CREATE TABLE`, `ALTER TABLE`, `CREATE TYPE`) and DML (`INSERT INTO … VALUES`,
`COPY … FROM stdin`) statements extracted from plain-SQL dump files.

The original issue ([Importer-Modul: Production Readiness, Robustness, Observability
& Feature Coverage Gaps](https://github.com/makr-code/ThemisDB/issues/…)) requested
that the regex/heuristic SQL parsing be replaced by a robust SQL parser library to
handle complex DDL (nested parentheses, default expressions with sub-selects, quoted
identifiers, domain types, partitioned tables, etc.).

---

## Decision Drivers

1. **Correctness** – Handle all legal pg_dump DDL/DML without silent truncation or
   misparse.
2. **Safety** – No crash, hang, or OOM on any input (enforced by the existing fuzz
   harness and AFL++ / LibFuzzer targets).
3. **Minimal external dependencies** – ThemisDB has a strict policy of evaluating
   every new C++ dependency for build complexity, license compatibility, and
   maintenance burden before adoption.
4. **Bounded scope** – The importer only processes a strict subset of PostgreSQL
   SQL syntax (the output of `pg_dump --format=plain`); a full SQL parser is
   disproportionate.

---

## Evaluated Options

### Option A – libpg_query (PostgreSQL's own parser, via C bindings)

**Description:** `libpg_query` ships the PostgreSQL server's parser as a standalone
C library.  It produces a full parse tree (protobuf or JSON) for any PostgreSQL SQL
statement.

| Criterion | Assessment |
|-----------|-----------|
| Correctness | ✅ Highest – literally the same parser PostgreSQL uses |
| Safety | ✅ Hardened in production for 30+ years |
| Dependency cost | ⚠️ ~15 MB C library; requires libprotobuf; significant build integration effort |
| License | ✅ PostgreSQL License (permissive) |
| Maintenance | ✅ Actively maintained by Citus / Microsoft |
| pg_dump scope match | ⚠️ Far more powerful than needed; parse tree traversal is non-trivial |

**Conclusion:** Best correctness, but high integration cost for the current scope.
**Deferred** to a future iteration when foreign-key / constraint / partition DDL
support is required.

### Option B – SQLite3 amalgamation parser (standalone)

**Description:** SQLite3's `sqlite3.c` amalgamation provides a complete SQL parser
that could be forked / adapted for PostgreSQL syntax.

| Criterion | Assessment |
|-----------|-----------|
| Correctness | ❌ SQLite SQL ≠ PostgreSQL SQL; significant dialect gaps |
| Safety | ✅ Extensively fuzz-tested |
| Dependency cost | ⚠️ ~200 KLOC C amalgamation to vendor |
| License | ✅ Public domain |

**Conclusion:** Dialect mismatch makes this unsuitable without significant porting
effort.  **Rejected.**

### Option C – Hand-written recursive-descent parser (incremental)

**Description:** Extend the existing regex-based approach with a proper tokeniser and
recursive-descent parser that handles the specific pg_dump output grammar.

| Criterion | Assessment |
|-----------|-----------|
| Correctness | ✅ Can be made correct for the known pg_dump output grammar |
| Safety | ✅ Full control; AFL++ harness already in place |
| Dependency cost | ✅ Zero – pure C++17, no new dependencies |
| Maintenance | ⚠️ Team responsibility |
| pg_dump scope match | ✅ Tailored exactly to pg_dump --format=plain output |

**Conclusion:** Chosen approach (see below).

---

## Decision

**We adopt Option C (incremental hand-written parser) for the current phase.**

The rationale:

1. **Scope is bounded and well-defined.**  `pg_dump --format=plain` produces a
   regular, predictable dialect of PostgreSQL SQL.  There are no sub-selects in DDL
   column defaults in real pg_dump output (defaults are always literals or simple
   function calls like `NOW()`).  A full parse tree is not required.

2. **Correctness gaps are already closed by the v1.1–v1.6 improvements:**
   - `splitTopLevelCommas()` – paren+quote-aware splitter handles all known column
     definition patterns including `numeric(10,4)`, `CHECK (…)`, `DEFAULT 'x,y'`.
   - `findMatchingParen()` – replaces the previously broken `find_last_of(')')`.
   - `ALTER TABLE ADD COLUMN` and `CREATE TYPE AS ENUM/composite` are parsed.
   - Binary COPY is detected and rejected cleanly.
   - All four parsers are covered by AFL++ / LibFuzzer harnesses.

3. **libpg_query adoption is explicitly scheduled for Phase 4 (Q3 2026)** once the
   team needs to support:
   - Partitioned tables (`CREATE TABLE … PARTITION BY …`)
   - Inheritance (`INHERITS (…)`)
   - Foreign keys with complex deferral options
   - Generated columns (`GENERATED ALWAYS AS …`)
   - Check constraints with sub-expressions

---

## Consequences

### Positive

- No new build dependencies introduced now.
- Existing AFL++ harness (`fuzz/harnesses/postgres_importer_harness.cpp`) provides
  continuous safety coverage.
- The parser can be swapped for libpg_query in a future PR with minimal API changes
  (the `parseCreateTable` / `parseInsert` / `parseCopy` methods are private
  implementation details).

### Negative / Risks

- Complex DDL patterns not yet in `splitTopLevelCommas()` may still be misparsed.
  **Mitigation:** Any such case should be filed as a bug; adding a new corpus seed to
  the AFL++ importer corpus and a regression test is the prescribed process.
- Team must maintain the parser as PostgreSQL syntax evolves.
  **Mitigation:** pg_dump output format is stable across minor versions; only major
  PostgreSQL version upgrades require review.

---

## Future Work

When the following features are required, re-evaluate libpg_query adoption:

- [ ] Partition table import (PG 10+)
- [ ] Generated / identity column preservation
- [ ] Full foreign-key / constraint round-trip
- [ ] Multi-statement transaction blocks in dumps

**Target evaluation date:** Q3 2026 (Phase 4 Robust SQL Parsing milestone).

---

## References

- `src/importers/postgres_importer.cpp` – current parser implementation
- `include/importers/importer_interface.h` – public API
- `fuzz/harnesses/postgres_importer_harness.cpp` – AFL++ / LibFuzzer harness
- `docs/importers_roadmap.md` – Phase 4 roadmap items
- `docs/importers_runbook.md` – operator guide
- [libpg_query on GitHub](https://github.com/pganalyze/libpg_query)
