---
Author: ThemisDB Contributors
Created: 2026-10-31
Creation_Date_Note: Creation date of this rewritten page; the original file creation date is unknown.
Last Updated: 2026-10-31
Status: review
AI_Used: true
AI_Models: unknown
AI_Contributor: themisdb-doc-orchestrator
Human_Review: false
Human_Author: unknown
Source_Level: level1
SOT_Domain: module-behavior
Canonical_References:
  - benchmarks/security/CMakeLists.txt
  - benchmarks/CMakeLists.txt
  - CMakePresets.json
  - benchmarks/security/bench_security.cpp
  - benchmarks/security/bench_security_release_gates.cpp
Related_Issue: "#7379"
Related_Milestone: DOC-WEEKLY-2026-W44
---

Exact model backend not verifiable from final document text alone.

# Security benchmarks

There are nine top-level `bench_*.cpp` files. When
`THEMIS_BUILD_BENCHMARKS=ON` and `benchmark::benchmark` is available,
`benchmarks/security/CMakeLists.txt` discovers them and skips
`bench_security.cpp` and `bench_compliance_security_governance.cpp` in that
subdirectory, leaving seven candidates there. The root
`benchmarks/CMakeLists.txt` registers `bench_security.cpp` separately; this note
makes no claim about a target for the skipped compliance file.

The `linux-release` preset disables benchmarks. Benchmark target registration
and measured results are separate: source discovery does not establish that a
benchmark was run or that a performance gate passed.
