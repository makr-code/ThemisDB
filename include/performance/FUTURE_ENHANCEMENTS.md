---
Author: "ThemisDB Team"
Created: "2026-06-01"
Last Updated: "2026-10-31"
Status: "active"
Source_Level: "level1"
SOT_Domain: "api-contract"
Canonical_Sources:
  - "include/performance/*.h"
  - "src/performance/FUTURE_ENHANCEMENTS.md"
Related_Issue: "#7366"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

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
| cycle/expected metrics APIs | `cycle_metrics.h`, `expected_cycles.h` | Benchmarks and diagnostics | Present; compatibility status is not inferred here |
| adaptive optimizer APIs | `workload_adaptive_optimizer.h`, `adaptive_query_compiler.h` | Query and server layers | Present; verify each caller contract in the header |
| NUMA/cache control APIs | `numa_memory_manager.h`, `advanced_cache_manager.h` | Runtime tuning | Present; host support is implementation-dependent |
| accelerator capability APIs | `hardware_accelerator.h`, `phase4/pmu_counters.h` | Hardware-aware execution | Present; compile/runtime/host support must be checked |

---

## Planned Enhancements

### Near-Term (Q4 2026)

- Document unsupported-hardware, degraded-feature, and fallback semantics consistently across accelerator-facing headers.
- Standardize naming for performance incident, capability, and optimizer-result DTOs.
- Clarify which phase-3/phase-4 headers are experimental versus broadly consumable.

### Medium-Term (Q4 2026+)

- Introduce `performance_capability_profile.h` and `optimizer_incident.h` for shared diagnostics/capability exchange.
- Document benchmark-reference expectations for adaptive optimization, measurement, and fallback hot paths.
- Align prefetch, cache, and workload-adaptation headers around a shared runtime-tuning vocabulary.

### Long-Term

- Add deployer extension hooks for custom cost models and accelerator-selection policies.
- Unify experimental phase component capability negotiation under a single performance profile contract.

The proposed `performance_capability_profile.h` and `optimizer_incident.h`
listed in the public roadmap are not present in the current header tree. These
items are proposals, not available APIs. See
[`ROADMAP.md`](ROADMAP.md) for the canonical public-header plan.

Exact model backend not verifiable from final document text alone.
