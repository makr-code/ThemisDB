---
Author: "ThemisDB Contributors"
Created: "2026-06-01"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "ThemisDB documentation orchestration assistant"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# `src/retrieval/src/` implementation

The only translation unit in this directory is `lora_package.cc`. The local
[`CMakeLists.txt`](../CMakeLists.txt) builds it into the static
`themis_retrieval` target. It implements JSON serialization, manifest
SHA-256 helpers, in-memory manifest-store operations, and optional signature
callback checks.

`dual_read_validator.cpp` is a sibling of this directory, not a file in it. It
implements `DualReadValidator`, but it is not included in the
`themis_retrieval` static library source list. Its query executor is currently
a placeholder that returns no results.

No separate `retrieval_tests` target or `retrieval_engine.h` exists for this
module. Focused test registration and synthetic benchmark details are
documented in [`../../../tests/retrieval/README.md`](../../../tests/retrieval/README.md)
and [`../../../benchmarks/retrieval/README.md`](../../../benchmarks/retrieval/README.md).
