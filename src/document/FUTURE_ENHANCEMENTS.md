# Document Module — Future Enhancements

<!-- Status: current | validated: 2026-10-31 -->
<!-- Links: README.md · ROADMAP.md · PERFORMANCE_EXPECTATIONS.md · SECURITY.md -->

## Scope and status

This file is for candidate work beyond the active module plan. The active Q1 2027 reliability work and its evidence requirements are tracked in [ROADMAP.md](ROADMAP.md); completed schema/merge hardening, diagnostics, and in-memory benchmark suites are not repeated here as open work.

No separate, approved enhancement issue or milestone for the document module was identified in the sources reviewed for issue #7342. The items below are therefore explicit follow-up questions, not commitments or claims that implementation is underway.

## Unresolved planning questions

- TODO: determine whether a durable `IDocumentStore` adapter belongs to this module or to a storage integration module.
- TODO: define the required production XML/XSD validation boundary for XDOMEA connectors; the current in-memory connector is not an XSD validator.
- TODO: define whether encrypted-entity key rotation must re-cipher persisted bytes; the in-memory entity currently records only a key ID.
- TODO: identify a canonical owner and issue/milestone before adding dates or release commitments.

## Design Constraints

- Preserve the current explicit `Result`/optional/error behavior unless a reviewed API change says otherwise.
- Do not infer persistence, authorization, XSD validation, or encryption guarantees from the in-memory implementations.
- Keep performance claims tied to the measured implementation; see [PERFORMANCE_EXPECTATIONS.md](PERFORMANCE_EXPECTATIONS.md).

## Required Interfaces

Current contracts are listed in [README.md](README.md) and [ARCHITECTURE.md](ARCHITECTURE.md). No additional public interface is approved by the sources reviewed for issue #7342.

## Implementation Notes

- Candidate changes above are unresolved design questions, not implementation tasks with an assigned owner.
- TODO: link each approved follow-up to its owning issue and record the interface/implementation change there.

## Test Strategy

- Existing direct focused sources cover store/manager, schema, diff/merge diagnostics, round-trip persistence, XDOMEA, and high-cardinality stress paths.
- TODO: attach test execution evidence before treating test-source presence as test completion.

## Performance Targets

- No representative-hardware p95/p99 baseline is established here.
- The `DOC-BM` thresholds are synthetic counters, not measurements of document runtime paths; see [PERFORMANCE_EXPECTATIONS.md](PERFORMANCE_EXPECTATIONS.md).
- TODO: define approved workload, percentile method, and baseline evidence with the benchmark owner.

## Security / Reliability

- Current in-memory security limits and unverified deployment responsibilities are summarized in [SECURITY.md](SECURITY.md).
- Long-duration reliability work remains in [ROADMAP.md](ROADMAP.md); test source presence is not a completed soak run.
