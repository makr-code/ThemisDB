# Exporters Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

Production exporter runtime exists across format-specific pipelines (JSONL/Parquet/Arrow/HuggingFace), streaming/incremental/join orchestration, and policy/security controls.

## In Progress

- [~] hardening policy/filter parity and edge behavior across all exporter variants (Target: Q3 2026)
- [~] benchmark stabilization for export throughput and delta/stream hot paths (Target: Q3 2026)
- [~] diagnostics consistency improvements for policy denial and export failure classes (Target: Q3 2026)

## Planned Features

### Short-term (3-6 months)
- [ ] tighten deterministic behavior for mixed-format and mixed-policy export permutations (Target: Q4 2026)
- [ ] expand regressions for join/stream/incremental checkpoint edge scenarios (Target: Q4 2026)
- [ ] improve operator-facing observability for hub upload and redaction incidents (Target: Q4 2026)

### Mid-term (6-12 months)
- [ ] re-baseline p95/p99 and throughput envelopes for major exporter paths (Target: Q1 2027)
- [ ] broaden benchmark depth for join/predicate and template-heavy workflows (Target: Q1 2027)
- [ ] harden long-running reliability under sustained large-export workloads (Target: Q1 2027)

## Implementation Phases

### Phase 1: Design / API Contract
- [ ] freeze exporter interface and option contracts for active major line (Target: Q3 2026)
- [ ] define explicit error taxonomy for policy, filter, and output-failure classes (Target: Q3 2026)

### Phase 2: Core Implementation
- [ ] complete hardening for format pipelines and orchestration internals (Target: Q4 2026)
- [ ] align security and governance behavior to bounded runtime contracts (Target: Q4 2026)

### Phase 3: Error Handling and Edge Cases
- [ ] standardize fail-closed behavior for unauthorized/unsafe export scenarios (Target: Q4 2026)
- [ ] unify diagnostics across stream/incremental/join and hub upload failures (Target: Q4 2026)

### Phase 4: Tests
- [ ] expand focused regressions for format, policy, and checkpoint edge scenarios (Target: Q4 2026)
- [ ] extend deterministic fixture coverage for template and join predicate permutations (Target: Q4 2026)

### Phase 5: Performance and Hardening
- [ ] lock benchmark-backed release gates for exporter throughput and latency hot paths (Target: Q4 2026)
- [ ] validate p95/p99 and throughput behavior against release baselines (Target: Q4 2026)

### Phase 6: Documentation and Acceptance
- [x] core exporters module docs aligned to source-verifiable behavior
- [x] roadmap/future planning separated from historical changelog entries

## Production Readiness Checklist

- [x] core exporters surfaces documented and source-verified
- [x] module-level security and failure behavior documented
- [x] benchmark mapping documented in performance expectations
- [ ] remaining hardening tasks closed for policy/filter/checkpoint edge paths
- [ ] release benchmark stabilization complete

## Known Issues and Limitations

- behavior depends on selected format backend and runtime configuration.
- selected join/stream/incremental edge scenarios need continued hardening.
- benchmark breadth should keep expanding for advanced export workflows.

## Breaking Changes

No breaking exporters contract planned. Any contract-breaking change requires migration notes and changelog entry before merge.