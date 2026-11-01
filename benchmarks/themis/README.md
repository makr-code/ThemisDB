---
Author: "ThemisDB Contributors"
Created: "2026-06-01"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Model_Attribution: "Exact model backend not verifiable from final document text alone."
AI_Contributor: "themisdb-doc-orchestrator"
Human_Review: false
Human_Author: "unknown"
---

# benchmarks/themis

Mirrored benchmark folder for `src/themis`.

The current CMake registration is conditional on `THEMIS_BUILD_BENCHMARKS` and
adds `bench_themis_core`, `bench_themis_release_gates`, and
`bench_themis_dedicated_gates`. Registration and source definitions do not
indicate that benchmarks have been run; see
[`../../src/themis/PERFORMANCE_EXPECTATIONS.md`](../../src/themis/PERFORMANCE_EXPECTATIONS.md)
for the documented cases and validation limits.
