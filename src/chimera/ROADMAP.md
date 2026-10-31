---
Author: unknown (original author not verified)
Created: 2026-10-31
Last Updated: 2026-10-31
Status: review
AI_Used: true
AI_Models: unknown
AI_Contributor: AI documentation assistant
Human_Review: false
Human_Author: unknown
---

Exact model backend not verifiable from final document text alone.

# Chimera roadmap notes

**Status basis:** source and CMake inspection for issue #7336 only. This file does not establish release status, maturity, or completed runtime validation.

## Current source posture

- The ThemisDB adapter, retry, batch, and transaction-context components are present in the local source tree.
- MongoDB, Neo4j, and Qdrant adapters are present locally but have conditional driver paths; their availability is configuration-dependent and is not established by this roadmap.
- `THEMIS_BUILD_CHIMERA` defaults `OFF`. With it enabled, `cmake/ChimeraAdapters.cmake` adds external CHIMERA adapter source files and the local ThemisDB adapter. CMake test registrations include additional local/external references, so a supported configuration and successful build still need to be established.
- Local test and benchmark source files exist; their existence is not evidence that targets built or tests/benchmarks passed.

## Candidate next work (uncommitted)

- [ ] Reconcile CMake adapter and focused-test source lists with files actually present; document the external checkout requirements.
- [ ] Specify supported combinations for engine injection and MongoDB/Neo4j/Qdrant driver compilation.
- [ ] Correct retry behavior or narrow its documented guarantees; test concrete retryable and non-retryable errors.
- [ ] Define transaction semantics and avoid implying distributed coordination unless implemented and tested.
- [ ] Tie any performance thresholds to real adapter workloads and an executed, maintained CI gate; identify stub benchmarks as synthetic.

## Readiness checklist and known issues

- [ ] Confirm the enabled CMake configuration and external checkout build.
- [ ] Record passing focused tests for each supported adapter/driver combination.
- [ ] Establish supported engine-dispatch and vendor-driver behavior.
- [ ] Verify retry, transaction, and performance claims with targeted evidence.

Known issue: the current documentation/source inventory does not establish which optional vendor paths are supported in a shipped binary. No planned breaking change is evidenced by this inspection; compatibility policy remains dependent on the selected external contract.

## Readiness boundary

No dated release targets, phase completion, test-pass claim, production designation, or maturity score is asserted. Track those only when supported by current root governance/release evidence and configuration-specific validation.

## References

`CMakeLists.txt`, `cmake/ChimeraAdapters.cmake`, `tests/CMakeLists.txt`, `tests/chimera/CMakeLists.txt`, `benchmarks/CMakeLists.txt`, [`ARCHITECTURE.md`](ARCHITECTURE.md).

Issue/milestone: #7336 / DOC-WEEKLY-2026-44.
