[docs](../../README.md) > [en](../README.md) > [chimera](./index.md) > [README](./README.md)
**Datum:** 2026-04-16
**Status:** review
**Primary (Quelle der Wahrheit):**
- `src/chimera/README.md`
- `src/chimera/ARCHITECTURE.md`
- `src/chimera/ROADMAP.md`
- `src/chimera/FUTURE_ENHANCEMENTS.md`
- `src/chimera/CHANGELOG.md`
- `src/chimera/themisdb_adapter.cpp`
- `include/chimera/README.md`
- `include/chimera/themisdb_adapter.hpp`
- `include/chimera/database_adapter.hpp`
- `tests/chimera/test_chimera_streaming.cpp`
- `tests/chimera/test_chimera_prepared_statements.cpp`

**Bezug / Reference:**
- Issue: `[MODULE] chimera`
- Context: Secondary documentation for module-level reality check and documentation migration of `chimera`.

---

# chimera Module — Overview and Verification Status

## TL;DR

The `chimera` module currently provides the **ThemisDB reference adapter** with in-process simulation paths and optional engine-backed dispatch.
Streaming and prepared statements are implemented and covered by dedicated tests.

## Scope (Task 1: reality check)

- **Implementation:** `src/chimera/themisdb_adapter.cpp`
- **Public API:** `include/chimera/themisdb_adapter.hpp`, `include/chimera/database_adapter.hpp`
- **Tests:** `tests/chimera/test_chimera_streaming.cpp`, `tests/chimera/test_chimera_prepared_statements.cpp`
- **Primary docs:** `src/chimera/{README,ARCHITECTURE,ROADMAP,FUTURE_ENHANCEMENTS,CHANGELOG}.md`

## Verified statements (Tasks 1–3)

1. **Primary docs were checked against code**
   - Module implementation is centered on `ThemisDBAdapter`; additional vendor adapters/factory files are not present in `src/chimera/`.
   - Simulation mode plus optional engine injection are now explicitly documented in primary docs.
2. **ROADMAP/FUTURE_ENHANCEMENTS were verified**
   - `src/chimera/ROADMAP.md` now reflects phased implementation status with open items and targets.
   - `src/chimera/FUTURE_ENHANCEMENTS.md` follows the required actionable structure (`Scope`, `Design Constraints`, `Required Interfaces`, `Implementation Notes`, `Test Strategy`, `Performance Targets`, `Security / Reliability`).
3. **Research constraints are anchored**
   - Build-flag dependent behavior (`THEMISDB_ENGINE_AVAILABLE`) and capability mismatches are tracked in roadmap/future docs and the missing-implementations report.

## Installation

The chimera module headers are included automatically when linking against the `themis_chimera` CMake target.
No separate installation is required beyond the standard ThemisDB build:

```bash
cmake --preset linux-ninja-release
cmake --build --preset linux-ninja-release
```

See [`include/chimera/README.md`](../../../include/chimera/README.md) for header-level details.

## Usage

### Simulation mode (unit tests / CI)

```cpp
#include "chimera/themisdb_adapter.hpp"

chimera::ThemisDBAdapter adapter;           // no live server required
adapter.connect("themisdb://localhost/test");

// Relational
adapter.insert_row("users", {{"id", "u1"}, {"name", "Alice"}});
auto rows = adapter.execute_query("SELECT * FROM users");

// Streaming result set
auto stream = adapter.execute_query_stream("SELECT * FROM large_table");
while (stream->has_more()) {
    auto batch = stream->next_batch(256);
    // process batch …
}

// Prepared statement
auto stmt = adapter.prepare("SELECT * FROM orders WHERE id = @id");
stmt->bind("id", chimera::Scalar{"o123"});
auto result = stmt->execute();
```

### Engine-wired mode (production)

```cpp
themis::QueryEngine        engine;
themis::VectorIndexManager vim;
themis::GraphIndexManager  gim;

chimera::ThemisDBAdapter adapter(&engine, &vim, &gim);
adapter.connect("themisdb://prod-host:7070/mydb");
// All operations are delegated to the real ThemisDB back-end.
```

See [`include/chimera/README.md`](../../../include/chimera/README.md) for the full API reference and more examples.

## Open items / known risks

- Include-level documentation (`include/chimera/README.md`) has been added — API reference for both header files is now available.
- Capability claims (e.g. connection pooling) are not fully mirrored by concrete adapter APIs.
- Engine-backed paths still include `NOT_IMPLEMENTED` outcomes when build flags are unavailable.

Detailed gap report (DE):
- `docs/de/chimera/MISSING_IMPLEMENTATIONS.md`
