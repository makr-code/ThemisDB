> **Roadmap-Hinweis:** Vage Bullets ohne Akzeptanzkriterien in Checkbox-Tasks ueberfuehren. Format: `- [ ] <Task> (Target: <Q/Jahr>)`.

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

# Query Module Roadmap

## Current Status
Production-ready multi-model query stack with parser, optimizer, execution, federation, caching, and compatibility layers in active use.

## In Progress
- [~] Query hardening wave for safety, resilience, and predictable performance (Target: Q3 2026)
  - [ ] Complete remaining performance/regression benchmark gates for vectorized and federated paths (Target: Q3 2026)
  - [ ] Continue reliability hardening for cancellation, limits, and distributed query failure behavior (Target: Q3 2026)
- [~] **AQL Mutations Language Extension** — Phase 1-5 Implementation (Target: v2.0.0, Q3/Q4 2026)
  - Implement INSERT, UPDATE, REPLACE, REMOVE, UPSERT statements for data manipulation
  - Integrate mutations with transaction blocks (BEGIN...COMMIT) for atomic batching
  - Full detailed roadmap: [AQL_MUTATIONS_ROADMAP.md](./AQL_MUTATIONS_ROADMAP.md)
  - [ ] Phase 1: Parser & Tokenizer Enhancement (Target: Q3 Week 1-2)
  - [ ] Phase 2: Safety & Semantic Validation (Target: Q3 Week 2-3)
  - [ ] Phase 3: Translation & Execution Plan (Target: Q3 Week 3-4)
  - [ ] Phase 4: Transaction Support & Atomicity (Target: Q3 Week 5-6)
  - [ ] Phase 5: Testing, Performance & Documentation (Target: Q3 Week 7-8)

## Planned Features

### Short-term (3-6 months)
- [ ] **AQL Mutations** — INSERT/UPDATE/REPLACE/REMOVE/UPSERT for data manipulation (Target: v2.0.0-beta Q3 2026)
- [ ] Harden optimizer decision quality under skewed statistics and changing workloads (Target: Q4 2026)
- [ ] Expand federated query failure handling with deterministic partial-result policies (Target: Q4 2026)
- [ ] Strengthen query resource-limit enforcement diagnostics and operator-facing observability (Target: Q4 2026)

### Mid-term (6-12 months)
- [ ] Advance approximate query processing from baseline implementations to production-suitable coverage (Target: Q1 2027)
- [ ] Extend ML-assisted optimization with strict fallback guarantees and reproducibility checks (Target: Q1 2027)
- [ ] Expand continuous-query backpressure and persistence hardening for long-lived subscriptions (Target: Q1 2027)

## Implementation Phases

### Phase 1: Safety and Access Hardening
- [ ] Keep parser/translation safety checks complete for edge-case query inputs (Target: Q3 2026)
- [ ] Ensure collection/access validation paths are enforced consistently across all execute entry points (Target: Q3 2026)

### Phase 2: Optimizer and Planning Hardening
- [ ] Improve plan-selection robustness under stale or partial statistics (Target: Q4 2026)
- [ ] Add deterministic regression packs for rewrite, cost, and adaptive plan switches (Target: Q4 2026)

### Phase 3: Federation and Distributed Query Hardening
- [ ] Expand cross-cluster/federated timeout and retry envelopes with bounded memory behavior (Target: Q4 2026)
- [ ] Validate shard routing and partial-failure semantics under fault-injection (Target: Q4 2026)

### Phase 4: Runtime and Performance Hardening
- [ ] Re-baseline vectorized execution performance and memory envelopes on representative datasets (Target: Q1 2027)
- [ ] Tighten JIT fallback and equivalence checks for hot-query compilation paths (Target: Q1 2027)

### Phase 5: Documentation and Release Readiness
- [ ] Keep query docs source-aligned with explicit sourcecode verification evidence per update cycle (Target: ongoing)
- [ ] Keep completed roadmap items exclusively in changelog (Target: ongoing)

## Production Readiness Checklist
- Status: Tracking in progress
- Nachweise: query focused tests, federation tests, optimizer tests, performance suites
- Hinweis: Abgeschlossene Arbeit wird ausschliesslich in CHANGELOG dokumentiert.

## Known Issues and Limitations
- Some long-horizon performance guarantees for federation/vectorized paths still require broader benchmark evidence.
- Advanced approximate/ML optimizer paths need additional production hardening and fallback verification.
- Continuous-query scale and backpressure behavior needs further validation under sustained load.

## Breaking Changes
- Query public APIs and language compatibility paths remain additive-first in active major lines.
- Any future behavioral change requiring client migration must be versioned and documented in changelog/migration notes.
