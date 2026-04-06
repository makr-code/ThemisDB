<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Security — Query Module

## Scope

Covers all public headers in `include/query/`. Implementation hardening in `../../src/query/`.

## Threat Model

| Threat | Impact | Mitigation |
|---|---|---|
| AQL/SQL injection | High — unauthorized data access | Parameterized queries enforced; literals escaped in `AqlParser` / `SqlParser` |
| SPARQL injection via user input | High — triple-store exfiltration | `SparqlParser` validates and parameterizes all user-supplied IRIs and literals |
| Resource exhaustion via unbounded queries | High — DoS | `QueryResourceLimits` enforces per-query CPU, memory, and wall-clock limits |
| Cross-cluster federation SSRF | High — internal network probe | `CrossClusterFederation` validates shard addresses against allowlist; mTLS required |
| Semantic cache poisoning | Medium — stale/wrong results | `SemanticCache` entries are keyed by embedding + schema version; invalidated on schema change |
| Plan cache collision | Medium — wrong query plan | `PlanCache` keys include schema fingerprint; collisions rejected by structural equivalence check |
| Query cancellation race | Low — partial result leak | `QueryCanceller` cooperative tokens flush result buffers before termination |
| Window function memory amplification | Medium — OOM | `WindowEvaluator` enforces partition size limits and spills to disk beyond threshold |

## Security Controls

1. **Parameterized queries** — `AqlParser`, `SqlParser`, and `SparqlParser` require parameterized inputs for all user-supplied values.
2. **Resource limits** — `QueryResourceLimits` hard-caps CPU time, memory, and wall-clock per query.
3. **mTLS on federation** — `CrossClusterFederation` requires mutual TLS for all remote shard connections.
4. **Schema-versioned caches** — `PlanCache`, `SemanticCache`, and `QueryCache` include schema fingerprint in keys.
5. **Cooperative cancellation** — `QueryCanceller` ensures consistent state on cancellation; no partial writes.
6. **Shard allowlist** — `CrossClusterFederation` rejects connections to non-configured shard endpoints.

## Known Limitations

- GraphQL dialect (planned Q3 2026) will introduce new injection surface requiring dedicated security review.
- `SemanticCache` embedding model is not adversarially hardened — crafted inputs may collide with cached results.
