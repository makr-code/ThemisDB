---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "review"
Source_Level: "L1"
SOT_Domain: "module-behavior"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Geo module developer guide

## Purpose and audience

Use this guide to navigate the geo module when implementing, reviewing, or
testing a change. The module entry page is [README.md](./README.md); this page
focuses on the developer workflow rather than repeating the component
overview.

## Read by task

| Work | Start here | Then inspect |
|---|---|---|
| Change an API contract | [API.md](./API.md) | [`geo_api_contract.h`](../../include/geo/geo_api_contract.h), the interface header, and its focused tests |
| Change geometry or CRS semantics | [MODELING.md](./MODELING.md) | [`ewkb.h`](../../include/utils/geo/ewkb.h), [`geo_json_geometry.h`](../../include/geo/geo_json_geometry.h), and parser/validator tests |
| Change GPU/CPU selection | [IMPLEMENTATION.md](./IMPLEMENTATION.md) | [`src/geo/CMakeLists.txt`](./CMakeLists.txt), backend source, device detector, and backend tests |
| Change k-NN projection | [EMBEDDING.md](./EMBEDDING.md) | [`geo_faiss_knn.cpp`](./geo_faiss_knn.cpp) and any available k-NN coverage |
| Change latency or algorithm trade-offs | [RESEARCH.md](./RESEARCH.md) | [`PERFORMANCE_EXPECTATIONS.md`](./PERFORMANCE_EXPECTATIONS.md), the benchmark source, and current run evidence |
| Diagnose a production symptom | [RUNBOOK.md](./RUNBOOK.md) | [OPERATIONS.md](./OPERATIONS.md), geo diagnostic interfaces, and the owning server integration |
| Review access and data handling | [TRUST.md](./TRUST.md) | [SECURITY.md](./SECURITY.md), `geo_policy.h`, and the server-side authorization boundary |

For subsystem ownership and inter-module flows, read
[ARCHITECTURE.md](./ARCHITECTURE.md). Use [ROADMAP.md](./ROADMAP.md) for open
work and [FUTURE_ENHANCEMENTS.md](./FUTURE_ENHANCEMENTS.md) for proposed
direction; neither replaces evidence from current source.

## Change workflow

1. Identify the public declaration under `include/geo` (or
   `include/utils/geo`) and the implementation under `src/geo`. Keep public
   contracts separate from internal algorithms.
2. Record coordinate order, CRS, units, validation behavior, and whether the
   result is exact or approximate. Different geo entry points use different
   argument ordering and distance units.
3. Follow the call path through the query/server owner when authorization,
   resource limits, or temporal storage is involved. A geo policy object is
   not an authorization boundary.
4. Add or select a concrete test under [`tests/geo`](../../tests/geo/).
   Contract-hardening mocks are useful for contract invariants but do not
   substitute for a production-path test.
5. Run the focused suite for the changed path and, for GPU changes, validate
   in the matching CUDA or HIP build/device lane. Record when a dependency or
   device is unavailable instead of treating a CPU fallback as GPU evidence.
6. Update the specific developer document that owns the claim and keep the
   [module README index](./README.md) current when adding a new module guide.

## Build and test entry points

Geo implementation files are added to the parent build through
[`src/geo/CMakeLists.txt`](./CMakeLists.txt). Focused test targets and CTest
names are generated from each `test_*.cpp` in
[`tests/geo/CMakeLists.txt`](../../tests/geo/CMakeLists.txt). For a concrete
contract-test example and the limits of its mock fixtures, see
[IMPLEMENTATION.md](./IMPLEMENTATION.md).

## Sourcecode verification

- Module paths and scope: [README.md](./README.md) and
  [ARCHITECTURE.md](./ARCHITECTURE.md).
- Public/internal ownership and evidence expectations:
  [module documentation guidelines](../../.github/MODULE_DEVELOPER_DOCUMENTATION_GUIDELINES.md).
- Build and focused-test registration:
  [`src/geo/CMakeLists.txt`](./CMakeLists.txt) and
  [`tests/geo/CMakeLists.txt`](../../tests/geo/CMakeLists.txt).

---

**AI provenance:** Exact model backend not verifiable from final document text alone.
