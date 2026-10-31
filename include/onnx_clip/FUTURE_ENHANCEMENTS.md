---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "ThemisDB documentation orchestration assistant"
Human_Review: false
Human_Author: "ThemisDB Team"
Source_Level: "level1"
SOT_Domain: "api-contract"
Canonical_Sources:
  - "src/onnx_clip/onnx_clip_plugin.h"
  - "src/onnx_clip/onnx_clip_plugin.cpp"
  - "include/plugins/image_analysis_interface.h"
  - "src/onnx_clip/FUTURE_ENHANCEMENTS.md"
Related_Issue: "#7365"
---

Exact model backend not verifiable from final document text alone.

<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · ../../src/onnx_clip/FUTURE_ENHANCEMENTS.md · ../../src/onnx_clip/PRODUCTION_REQUIREMENTS.md -->

# ONNX CLIP Interface Enhancement Proposals

**Documentation directory:** `include/onnx_clip/`
**Canonical implementation enhancements:** [`../../src/onnx_clip/FUTURE_ENHANCEMENTS.md`](../../src/onnx_clip/FUTURE_ENHANCEMENTS.md)

---

## Scope

This directory currently contains documentation only; there are no C++ headers
under `include/onnx_clip/`. The plugin declaration is the source-local
[`../../src/onnx_clip/onnx_clip_plugin.h`](../../src/onnx_clip/onnx_clip_plugin.h),
in namespace `themis::plugins::image`. Items below are unapproved proposals for
future interface/documentation work. Runtime, algorithm, and benchmark work is
tracked in:

→ [`../../src/onnx_clip/FUTURE_ENHANCEMENTS.md`](../../src/onnx_clip/FUTURE_ENHANCEMENTS.md)

---

## Proposed Design Constraints

- Public headers should not expose internal implementation types or arena/memory layout details to callers.
- Breaking changes to a future public API must follow `VERSIONING.md`.
- New entry points should be additive where compatible with the approved API design.
- `[ ]` Keep future declarations aligned with the existing `themis::plugins::image` interface and repository API/versioning policy.

---

## Possible future header interface (not defined)

There is no public C++ header in `include/onnx_clip/`. The current class
declaration is source-local and implements `IImageAnalysisBackend`; it is not an
ONNX Runtime inference contract.

No public header contract or destination directory is approved here. If
maintainers approve a public header, its namespace, location, and include
contract must be decided then; do not assume `include/onnx_clip/` or
`themis::onnx_clip`.

---

## Unapproved proposals

The Q3/Q4 2026 labels below are legacy planning labels, not an active schedule,
approved header contract, or evidence that public headers exist. The listed
ideas do not commit to placing headers in this documentation directory. Confirm
scope, location, and dates with maintainers before treating them as planned work.

### Short-Term (Q3 2026)

- [ ] Add `[[deprecated]]` annotations to any legacy entry points with migration notes
- [ ] Add explicit `noexcept` specifications to query/read paths where safe
- [ ] Add structured diagnostic types for all thrown exceptions

### Medium-Term (Q4 2026)

- [ ] Expand public surface with convenience factory functions for common usage patterns
- [ ] Add Concepts (C++20) constraints to template parameters in public headers
- [ ] Introduce explicit ABI stability markers (`THEMIS_STABLE_ABI`) to long-term interfaces

### Long-Term (2027+)

- [ ] Consider a C++20 module wrapper if a public interface is approved (location TBD)
- [ ] Evaluate a header-only delivery only if maintainers approve that API shape
- [ ] Add layer-mapping documentation only if a relevant integration is approved

---

## Alignment with Strategic Plan

These proposals do not establish a Tensor/LLM layer association or a delivery
schedule. Reconcile any cross-module integration proposal with the canonical
module roadmap before treating it as planned work.
