> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · ../../src/performance/FUTURE_ENHANCEMENTS.md -->

# Performance Module — Public Header Future Enhancements

**Module Path:** `include/performance/`
**Canonical implementation enhancements:** [`../../src/performance/FUTURE_ENHANCEMENTS.md`](../../src/performance/FUTURE_ENHANCEMENTS.md)

---

## Scope

Planned enhancements to the **public header contract** in `include/performance/`. Runtime hardening and benchmark work remain tracked in:

→ [`../../src/performance/FUTURE_ENHANCEMENTS.md`](../../src/performance/FUTURE_ENHANCEMENTS.md)

---

## Design Constraints

- `[x]` Optimization and fallback behavior must remain explicit and deterministic.
- `[x]` Experimental phase-specific headers must not blur stability expectations of core contracts.
- `[x]` Hardware-dependent capability loss must remain observable through public types.
- `[x]` Runtime config and feature-flag headers must remain deployer-controlled.

---

## Required Interfaces (Header Contract)

| Interface | Declared In | Consumer | Status |
|-----------|------------|----------|--------|
| cycle/expected metrics APIs | `cycle_metrics.h`, `expected_cycles.h` | Benchmarks and diagnostics | ✅ Stable |
| adaptive optimizer APIs | `workload_adaptive_optimizer.h`, `adaptive_query_compiler.h` | Query and server layers | ✅ Stable |
| NUMA/cache control APIs | `numa_memory_manager.h`, `advanced_cache_manager.h` | Runtime tuning | ✅ Stable |
| accelerator capability APIs | `hardware_accelerator.h`, `phase4/pmu_counters.h` | Hardware-aware execution | ✅ Stable |

---

## Planned Enhancements

### Short-Term (Q3 2026)

- Document unsupported-hardware, degraded-feature, and fallback semantics consistently across accelerator-facing headers.
- Standardize naming for performance incident, capability, and optimizer-result DTOs.
- Clarify which phase-3/phase-4 headers are experimental versus broadly consumable.

### Medium-Term (Q4 2026)

- Introduce `performance_capability_profile.h` and `optimizer_incident.h` for shared diagnostics/capability exchange.
- Document benchmark-reference expectations for adaptive optimization, measurement, and fallback hot paths.
- Align prefetch, cache, and workload-adaptation headers around a shared runtime-tuning vocabulary.

### Long-Term

- Add deployer extension hooks for custom cost models and accelerator-selection policies.
- Unify experimental phase component capability negotiation under a single performance profile contract.
