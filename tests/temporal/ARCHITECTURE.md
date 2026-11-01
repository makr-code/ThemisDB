---
Author: ThemisDB Contributors
Created: 2026-10-31
Last Updated: 2026-10-31
Status: review
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Contributors
---

Exact model backend not verifiable from final document text alone.

# Temporal test architecture

The directory is a flat set of focused C++ test sources for the APIs in
`include/temporal/` and implementations in `src/temporal/`. The source files do
not define a separate runtime API, configuration system, CLI, or service
integration.

[`CMakeLists.txt`](CMakeLists.txt) has two glob-based registrations:

1. All `test_*.cpp` files become `_focused` targets with a 120-second CTest
   timeout.
2. The `test_temporal_*.cpp` subset is also registered as `_autofocused`
   targets, with `temporal` and `autogen` labels.

The second loop creates additional test registrations from existing source
files; it does not represent a second independent implementation or suite of
source files. Optional linking to `themis_temporal` and the fallback
`src/index/temporal_graph.cpp` are conditional in CMake.

Tests that rely on `themisdb::temporal` runtime classes should be distinguished
from contract-focused tests that use test-local helper models. A test's presence
does not establish that it was executed or passed. See the directory
[`README.md`](README.md) for the source inventory and test groupings.
