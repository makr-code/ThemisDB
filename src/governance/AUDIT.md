---
Author: "ThemisDB Contributors"
Created: "2026-05-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
Related: "Issue #7349"
---

Exact model backend not verifiable from final document text alone.

# Governance Documentation/Source Audit Snapshot

This is a repository inspection snapshot, not a build, security, compliance, or
release audit. No build, test, linter, or benchmark was run for Issue #7349.

## Current inventory and registration evidence

| Item | Observation |
|---|---|
| Implementation files | 35 `.cpp` files directly under `src/governance/` at review time |
| Public headers | 36 `.h` files directly under `include/governance/` at review time |
| Focused test sources | 16 `.cpp` files directly under `tests/governance/` at review time |
| Module-focused test registration | CMake quarantines `test_operational_audit_evidence` and `test_policy_versioning_and_approval` pending canonical project configure/build/run validation; operational audit passed a standalone local run (26/26), while project configuration is blocked by missing RocksDB and versioning/approval has not been linked or run |
| Benchmarks | Four `.cpp` sources under `benchmarks/governance/`; this directory's CMake registers policy-latency and release-gate targets, while top-level CMake also handles other benchmark sources |

These are source-tree/CMake observations, not proof of successful compilation or
passing execution. Test registration must be distinguished from execution, and
benchmark presence from benchmark execution.

## Documentation findings corrected for this snapshot

- Public header list and exact type spellings aligned to the current headers.
- Stale downstream architecture claims and build/test/benchmark instructions
  removed or qualified.
- Fail-closed WASM evaluator selection, RSA key requirements, policy
  configuration, and bounded in-memory audit/lineage limits made explicit.
- `RSA_SHA256` now uses OpenSSL EVP with an unencrypted PEM RSA private key
  (minimum 2048 bits) for signing; verification accepts the private PEM or its
  public-key PEM. Focused source tests cover signature verification, tampering,
  and malformed-key failure; this does not establish key custody or lifecycle.
- Legacy compliance, audit-integrity, conflict, and versioning guarantees are
  withdrawn where they exceed current implementation evidence.

## Remaining verification limits

- No full dependency/build graph or runtime integration was executed.
- No benchmark result or release baseline was inspected for this review.
- Compliance and production readiness require deployment-level evidence beyond
  source declarations and focused tests.

## References

- [Governance test CMake](../../tests/governance/CMakeLists.txt)
- [Governance benchmark CMake](../../benchmarks/governance/CMakeLists.txt)
- [Public-header inventory](../../include/governance/README.md)
- [Module roadmap](ROADMAP.md)
