> **Sicherheitshinweis:** Security-Angaben gegen aktuelle Build-Flags, Codepfade und Tests validieren.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Security — Graph Module

> For reporting security vulnerabilities, see the project-level [SECURITY.md](../../../SECURITY.md).

## Security Scope

The Graph module handles property graph traversal, path constraint evaluation, AQL-based graph pattern matching, and distributed cross-shard edge queries. Security controls apply to all traversal entry points, query planning, and shard communication.

## Threat Model

| Threat | Mitigation |
|--------|------------|
| Path traversal injection via malicious node/edge filter expressions in path constraints | Path constraint validator rejects expressions containing unsanitized user input; allow-listed operators only |
| DoS via unbounded graph queries (infinite loops, exponential fan-out) | Per-query timeout limits enforced in `parallel_traversal.cpp`; max-depth and max-hop limits required |
| AQL injection via graph pattern matching | AQL pattern inputs are validated and parameterised before execution; raw string interpolation is prohibited |
| Cross-shard edge traversal exposing shard topology to untrusted callers | Shard-isolated execution model; cross-shard requests use internal service credentials; shard identifiers are never returned to client |
| Information leakage via EXPLAIN endpoint | EXPLAIN output is access-controlled; unauthenticated requests receive HTTP 401 |
| Denial of service via subgraph isomorphism (VF2) on adversarial graphs | VF2 candidate-set size capped; backtracking depth limited; execution budget enforced |

## Security Controls

### Input Validation
- Path constraint expressions are parsed by a dedicated validator in `path_constraints.cpp` before reaching the traversal engine.
- Node and edge filter predicates are type-checked against the schema; unknown field references are rejected.
- AQL graph patterns undergo injection detection (keyword and operator allow-listing) before compilation.

### Query Execution Limits
- Maximum traversal depth: configurable, default 100 hops.
- Maximum result set size: configurable, default 10 000 paths.
- Wall-clock query timeout: configurable, default 30 s; enforced via cooperative cancellation tokens.

### Distributed Execution
- Cross-shard edge traversal uses mTLS between shard agents.
- Shard topology (host/port mapping) is kept server-side only; clients receive only result data.
- Shard-isolated execution ensures one tenant's traversal cannot read another tenant's subgraph.

### EXPLAIN Endpoint
- Accessible only to authenticated, authorised users (role: `graph_admin` or `query_explain`).
- Returns logical query plan only; physical shard addresses are redacted.

## Data Handling

- Edge properties may contain sensitive user data; access is governed by the calling user's graph-level ACL.
- Traversal result sets are streamed and not persisted in query logs.
- Vector similarity scores used in scheduled edge refresh are stored in dedicated index partitions, not in raw edge records.

## Known Limitations

- VF2 subgraph isomorphism does not yet implement full adversarial input hardening for graphs exceeding 10 000 nodes (see issue #1832).
- AQL injection detection covers standard AQL operators; custom UDF function names require manual review.
- GPU-accelerated BFS/DFS (planned, issue #1829) will require additional VRAM isolation review before production enablement.

## Dependency Security

| Dependency | Usage | Review Status |
|------------|-------|---------------|
| AQL engine | Graph pattern compilation | Reviewed; parameterised API used |
| RocksDB | Adjacency/property storage | Reviewed; key prefix isolation enforced |
| gRPC/mTLS | Cross-shard communication | Reviewed; certificate pinning enabled |
| Thread pool (work-stealing) | Parallel traversal | Reviewed; no shared mutable state across tenants |
