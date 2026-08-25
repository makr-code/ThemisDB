<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · ../../src/cache/FUTURE_ENHANCEMENTS.md -->

# Cache Module — Public Header Future Enhancements

**Module Path:** `include/cache/`
**Canonical implementation enhancements:** [`../../src/cache/FUTURE_ENHANCEMENTS.md`](../../src/cache/FUTURE_ENHANCEMENTS.md)

---

## Scope

Planned enhancements to the **public header contract** in `include/cache/`. Runtime hardening and benchmark work remain tracked in:

→ [`../../src/cache/FUTURE_ENHANCEMENTS.md`](../../src/cache/FUTURE_ENHANCEMENTS.md)

---

## Design Constraints

- `[x]` Cache get/put/invalidate behavior must remain deterministic and auditable.
- `[x]` Tenant isolation and quota boundaries must remain explicit where tenant mode is enabled.
- `[x]` Distributed coordination degradation must remain observable through public result/diagnostic types.
- `[x]` Backend-specific configs must not leak transport internals into generic cache interfaces.

---

## Required Interfaces (Header Contract)

| Interface | Declared In | Consumer | Status |
|-----------|------------|----------|--------|
| adaptive query/result cache APIs | `adaptive_query_cache.h`, `result_cache.h` | Query and server layers | ✅ Stable |
| semantic/embedding cache APIs | `semantic_cache.h`, `embedding_cache.h` | Retrieval and ML layers | ✅ Stable |
| distributed coordinator APIs | `distributed_cache_coordinator.h`, `cache_replication_coordinator.h` | Multi-node cache control | ✅ Stable |
| SLO / metrics APIs | `cache_hit_rate_slo_monitor.h`, `cache_metrics.h` | Operations and monitoring | ✅ Stable |

---

## Planned Enhancements

### Short-Term (Q3 2026)

- Document degraded-backend, partial-replication, and invalidation-failure behavior consistently across public coordinator headers.
- Standardize naming for cache incident, capability, and invalidation-result DTOs.
- Clarify tenant-isolation requirements for cache provider and partition interfaces.

### Medium-Term (Q4 2026)

- Introduce `cache_incident.h` and `cache_capability_profile.h` for shared diagnostics/capability exchange.
- Document benchmark-reference expectations for cache get/put/invalidate, warmup, and tenant-aware hot paths.
- Align distributed cache headers around a shared backend-neutral coordination vocabulary.

### Long-Term

- Add embeddable hooks for custom invalidation buses and cache peer transports.
- Unify local and distributed cache result envelopes under a single cache-operation status schema.
