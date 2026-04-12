# include stable_diffusion roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

## Current Status
- [x] Header API for Stable Diffusion plugin and helpers is available — v2.2.0 (2026-04-12)
  - `SDCppGenerator` wrapping `stable-diffusion.cpp` C API (Issue: #4590)
  - Real PNG IDAT encoder in `encodeMinimalPng()` (Issue: #4590)
  - `SDStubGenerator::generateImg2Img` input-image pass-through (Issue: #4590)

## In Progress
- [ ] Extend config and policy hooks for advanced deployment profiles (Target: Q3 2026)

## Implementation Phases
### Phase 1: Design / API Contract
- [x] Define plugin/generator/sanitizer contracts (Target: Q2 2026)
### Phase 2: Core Implementation
- [x] Source implementation exists in `src/stable_diffusion` (Target: Q2 2026)
### Phase 3: Error Handling & Edge Cases
- [x] Stub and in-memory generators support non-model test environments (Target: Q2 2026)
### Phase 4: Tests
- [ ] Add include-level compatibility tests for config and prompt-sanitizer APIs (Target: Q3 2026)
### Phase 5: Performance/Hardening
- [ ] Define strict limits for payload and output dimensions in API contract tests (Target: Q3 2026)
### Phase 6: Documentation & Acceptance
- [x] Baseline include module docs created (Target: Q2 2026)

## Production Readiness Checklist
- [x] Public headers map to implementation
- [ ] Expanded API contract tests complete

## Known Issues & Limitations
- Feature completeness depends on linked runtime backend and model availability.

## Breaking Changes
- None currently planned.