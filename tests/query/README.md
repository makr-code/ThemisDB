---
Author: "ThemisDB Contributors"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "OpenAI API assistant (model unknown)"
Human_Review: false
Human_Author: "unknown"
---

> Original creation date was not recorded; this provenance block was first recorded on 2026-10-31.
> Exact model backend not verifiable from final document text alone.

Source level: 1
SOT domain: module-behavior
Canonical references: `tests/query/CMakeLists.txt`, `tests/CMakeLists.txt`, `src/query/ROADMAP.md`, `include/query/`
Related issue: #7371
Related milestone: `DOC-WEEKLY-2026-44`

# Query module tests

`tests/query/` contains 61 C++ test source files and six Markdown documents. The
test sources cover AQL parsing and validation, query execution and optimization,
federation, caching, full-text and geospatial behavior, continuous queries,
resource limits, and related regression cases. This is an inventory of files,
not a claim that every test has been built or run.

## Build and registration

`tests/query/CMakeLists.txt` defines module-focused targets conditionally when
their source files exist. The repository also has query-related targets in
`tests/CMakeLists.txt`; the presence of a `.cpp` file in this directory alone
does not establish that it is registered or passing. Check the applicable CMake
registration before selecting a target, and consult the generated build/CTest
configuration for the active build.

For benchmark registration, see `benchmarks/query/CMakeLists.txt`. There are
currently no Markdown files in `benchmarks/query/`.

## Source of truth

Behavior and public contracts are defined by the query implementation and
headers, not by test names or this inventory. Use
[`../../src/query/ROADMAP.md`](../../src/query/ROADMAP.md) for module status and
[`../../include/query/`](../../include/query/) for public declarations.
