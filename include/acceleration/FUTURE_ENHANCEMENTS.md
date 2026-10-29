---
Author: "ThemisDB Team"
Created: "2026-10-29"
Last Updated: "2026-10-29"
Status: "active"
AI_Used: true
AI_Models: "GitHub Copilot"
AI_Contributor: "GitHub Copilot"
Human_Review: true
Human_Author: "ThemisDB Team"
---

> Build: `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-10-29 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · ../../src/acceleration/FUTURE_ENHANCEMENTS.md -->

# Acceleration Module — Public Header Future Enhancements

**Module path:** `include/acceleration/`
**Canonical runtime enhancements:** [`../../src/acceleration/FUTURE_ENHANCEMENTS.md`](../../src/acceleration/FUTURE_ENHANCEMENTS.md)

## Scope

This file tracks only the public header-facing roadmap. Runtime behavior, multi-device hardening, plugin validation, and benchmark gates stay in the implementation roadmap and are not inferred to be complete just because the header contract is stable.

## Design constraints

- Public headers must not expose internal implementation-only types or layout assumptions.
- New APIs should remain additive unless the project explicitly approves a breaking change.
- Capability and backend contract changes must stay aligned with `VERSIONING.md` and the runtime implementation in `src/acceleration/`.
- Any new public API that introduces hardware or plugin behavior must document the validation and fallback contract.

## Required interfaces

Current public surfaces cover:
- backend registration and capability probing
- vector/graph/geo backend selection
- AI hardware dispatch
- plugin and shader validation
- multi-GPU and vLLM resource-management contracts

## Planned enhancements

### Short term

- [ ] Add explicit `[[nodiscard]]` / `noexcept` annotations to public query APIs where semantics are clear and safe.
- [ ] Add structured diagnostic types for public backend failure paths where exceptions or statuses are not yet explicit.
- [ ] Document migration notes for any deprecated or superseded public acceleration API entries.

### Medium term

- [ ] Add convenience factory helpers for backend selection patterns used by index/geo/graph consumers.
- [ ] Revisit public ABI markers and compatibility expectations for long-lived acceleration APIs.
- [ ] Expand public examples covering CPU fallback semantics, DEVICE injection, and plugin failure paths.

### Long term

- [ ] Evaluate a C++20 module or wrapper layer for `include/acceleration/` if stable ABI and packaging requirements justify it.
- [ ] Keep header-level abstraction and implementation-level behavior aligned as the accelerator stack grows.

## Test strategy

- Verify public API changes with focused compile-time and runtime tests under `tests/acceleration/`.
- Exercise CPU fallback paths and deterministic capability injection before exposing new selection APIs.
- Keep plugin security, shader validation, and multi-device resource changes covered by the module regression and security tests.

## Performance and security goals

- Do not widen the public API without a corresponding validation path.
- Preserve deterministic CPU fallback behavior under invalid or unavailable hardware states.
- Ensure new public interfaces document error modes and bounded execution semantics.

## TODO / open assumptions

- Detailed long-term ABI or module-wrapper strategy remains a future design decision; no implementation commitment is made yet.
- Hardware-specific throughput targets remain tracked at the runtime and benchmark layer and are not assumed to be generic header-level semantics.
