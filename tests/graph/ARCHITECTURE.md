---
Author: ThemisDB Contributors
Created: 2026-10-31
Last Updated: 2026-10-31
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: ThemisDB documentation orchestration specialist
Human_Review: false
Human_Author: ThemisDB Contributors
Source level: level1
SOT domain: architecture-governance
Canonical source references:
  - CMakeLists.txt
  - ../../include/graph/
  - ../../src/graph/
Related milestone: DOC-WEEKLY-2026-44
AI_Model_Note: Exact model backend not verifiable from final document text alone.
---

# ARCHITECTURE

## Kontext
- Modul/Ordner: `tests/graph`
- C++ test sources for graph headers and graph-index integrations; registration is in
  `tests/graph/CMakeLists.txt`.

## Verantwortlichkeiten
- Focused test executables are built from `test_*.cpp` files.
- The registration helper supplies `module:graph`, `tier:unit`, and `kind:focused` labels.
- Selected tests attach graph implementation sources explicitly; CMake is authoritative.

## Schnittstellen
- Test code exercises the declarations in `include/graph/` and implementations under `src/graph/`.
- The tests are not a production service/API or a substitute for owning-module integration tests.

## Nicht-Ziele
- Keine Duplizierung von Logik, die in übergeordneten Core-Modulen verankert ist
- Keine Umgehung zentraler Sicherheits- und Compliance-Vorgaben

Both local source lists use plain `file(GLOB)` without `CONFIGURE_DEPENDS`; rerun configure after
adding/removing matching test files.
