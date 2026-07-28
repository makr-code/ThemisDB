> **Roadmap-Hinweis:** Vage Bullets ohne Akzeptanzkriterien in Checkbox-Tasks überführen. Format: `- [ ] <Task> (Target: <Q/Jahr>)`.

# Core Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-07-28 -->
<!-- Issue: #5638 (Development Status 2026-07-18) -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · MODULE_EVIDENCE.md -->

## Current Status

Core runtime foundations are delivered: `ConcernsContext`, observability/caching/security concern interfaces, runtime concern replacement APIs, and production adapters (spdlog, OpenTelemetry-family tracers, Prometheus, Redis cache path).  
The module remains **open** for plugin runtime loading hardening and signed adapter governance.

## In Progress

- [~] Development-status evidence refresh and roadmap/future alignment for Issue #5638 (Target: 2026-Q3)

## Planned Features

- [I] Plugin-based adapter loading (no recompile needed) (Issue: #1706)
- [ ] Adapter plugin hardening and signing workflow (Target: Q4 2026)

## Implementation Phases

### Phase 1: Design / API Contract
- [x] Stabilize current concern interfaces (`ILogger`, `ITracer`, `IMetrics`, `ICache`, `ISecrets`, `IFeatureFlags`, `IAuditLog`) as the active baseline
- [ ] Define signed plugin adapter contract and validation envelope (Target: Q4 2026)

### Phase 2: Core Implementation
- [x] Deliver `ConcernsContext` creation and runtime replacement surfaces
- [ ] Add runtime plugin loading path that avoids core-module recompilation (Target: Q4 2026)

### Phase 3: Error Handling and Edge Cases
- [x] Fail-fast null replacement protection in runtime `replace*` APIs
- [ ] Harden plugin load/unload failure semantics (signature mismatch, ABI mismatch, bootstrap errors) (Target: Q4 2026)

### Phase 4: Tests
- [x] Focused module test registration in `tests/core/CMakeLists.txt` (`module_core_<test_stem>_focused`)
- [~] Refresh focused build/test execution evidence for current cycle (Target: 2026-Q3)

### Phase 5: Performance and Hardening
- [ ] Enforce adapter hot-swap SLO and no-drop in-flight semantics from `FUTURE_ENHANCEMENTS.md` (Target: Q4 2026)
- [ ] Add adapter signing and trust policy validation to release hardening flow (Target: Q4 2026)

### Phase 6: Documentation and Acceptance
- [x] Keep roadmap/future/architecture synchronization explicit and source-traceable
- [~] Keep module evidence and issue closure checklist current for the active status issue (Target: 2026-Q3)

## Production Readiness Checklist

- [x] Core concerns architecture documented and source-verifiable (`README.md`, `ARCHITECTURE.md`)
- [x] Runtime replacement APIs and production-mode constraints documented (`ARCHITECTURE.md`, `SECURITY.md`)
- [x] Focused core tests are configured (`tests/core/CMakeLists.txt`)
- [~] Focused build/test execution evidence refreshed for this cycle (see `MODULE_EVIDENCE.md`)
- [ ] Plugin runtime loading and signing hardening completed (Issue #1706 + Q4 2026 hardening item)

## Known Issues & Limitations

- Plugin-based adapter loading without rebuild remains open (Issue #1706)
- Adapter plugin hardening/signing workflow remains open (Target: Q4 2026)
- Canonical Windows evidence snapshot (2026-07-18) documented a focused-binary evidence gap for `module_core_test_*_focused` targets
- Context propagation across async/thread boundaries still requires caller-managed header propagation (`startSpanFromHeaders` / `injectContext`)

## Breaking Changes

- No breaking core-module contract planned in this cycle
- Future plugin-signing enforcement may require adapter packaging/signature metadata for external adapters (migration notes required before rollout)
