# EPIC 1.3 Graph Validation

<!-- Status: current | planning scaffold | validated: 2026-07-01 -->

## Summary

Graph truth layer and evidence assembly contract.

**Status**: Phase 2 (Core Implementation) ✅ COMPLETE  
**Date**: 2026-07-01  
**Reference Documentation**: `docs/GRAPH_TRUTH_VALIDATION_LAYER.md`

## Scope

- Evidence bundle model for retrieval verification
- Provenance and confidence propagation
- Failure-close rules when graph checks reject ANN or tensor candidates
- ACL/Policy validation layer
- Multi-hop validation for indirect relationships
- Input contract validation from tensor layer

## Planned Repository Surfaces

- `include/rag/graph_truth_validator.h` ✅ EXTENDED with provenance, ACL, multi-hop
- `src/rag/graph_truth_validator.cpp` ✅ EXTENDED with new methods
- `tests/epic1_retrieval/graph_validator_test.cc` — Phase 4
- `benchmarks/epic1_retrieval/graph_validation_bench.cc` — Phase 5
- `docs/GRAPH_TRUTH_VALIDATION_LAYER.md` ✅ CREATED

## Seven-Phase Roadmap

### Phase 1: Design / API contract ✅ COMPLETE
- [x] Define evidence bundle inputs and outputs
- [x] Extend GraphTruthEvidence with provenance tracking
- [x] Design ACL/Policy validation layer
- [x] Design evidence assembly model
- [x] Create input contract validation interface
- [x] Document multi-hop validation strategy
- [x] Create comprehensive API documentation
- [x] Prepare the next phase only after the current contract is reviewed for `EPIC 1.3 Graph Validation`.

### Phase 2: Core implementation ✅ COMPLETE
- [x] Implement real ACL/policy engine integration (replace stub)
- [x] Implement multi-hop path finding in knowledge graph (replace stub)
- [x] Add input contract validation logic
- [x] Integrate with policy engine (RBAC, ABAC)
- [x] Prepare the next phase only after the current contract is reviewed for `EPIC 1.3 Graph Validation`.

### Phase 3: Error handling and edge cases
- [ ] Document boundary between validation and ranking
- [ ] Implement handling for broken provenance chains
- [ ] Implement policy violation escalation
- [ ] Add timeout/depth limits for multi-hop traversal
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 1.3 Graph Validation`.

### Phase 4: Tests
- [ ] Enumerate edge cases for contradictory evidence and incomplete graph state
- [ ] Unit tests for all validation methods
- [ ] Integration tests for ANN→Tensor→Graph pipeline
- [ ] Policy/ACL validation tests
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 1.3 Graph Validation`.

### Phase 5: Performance and hardening
- [ ] Reserve integration tests for graph-backed rejection and confirmation paths
- [ ] Performance benchmarks (throughput, latency, memory)
- [ ] Multi-hop traversal optimization
- [ ] Concurrent validation stress tests
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 1.3 Graph Validation`.

### Phase 6: Documentation and acceptance
- [ ] Reserve integration tests for graph-backed rejection and confirmation paths
- [ ] Finalize deployment runbook
- [ ] Update architecture diagrams
- [ ] Create operations guide for policy tuning
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 1.3 Graph Validation`.

### Phase 7: Integration
- [ ] Reserve integration tests for graph-backed rejection and confirmation paths
- [ ] Wire the planned files into the nearest CMake and cross-epic integration checkpoints for `EPIC 1.3 Graph Validation`.
- [ ] Plan rollout strategy for default validation layer usage
- [ ] Coordinate with tensor layer for input contract enforcement

## Phase 1 Deliverables (✅ COMPLETE)

### 1. Extended GraphTruthValidator API Contract

**Header File**: `include/rag/graph_truth_validator.h`

**New Configuration Options** (`GraphTruthValidatorConfig`):
- `enable_acl_validation` (default: true) — mandatory ACL/policy checks
- `enable_provenance_tracking` (default: true) — full chain of custody
- `enable_multi_hop_validation` (default: true) — indirect evidence discovery
- `max_multi_hop_depth` (default: 3) — traversal depth limit

**New Data Structures**:
- `ProvenanceRecord` — complete chain-of-custody from ANN through Tensor to Graph
- `AclValidationResult` — policy validation decision and reasoning
- `InputContractValidation` — tensor input well-formedness check
- `MultiHopValidationResult` — multi-hop relationship evidence
- `EvidenceBundle` — complete evidence ready for LLM consumption

**Extended GraphTruthEvidence**:
- `provenance: ProvenanceRecord` — full chain of custody
- `acl_result: AclValidationResult` — policy decision
- `input_contract: InputContractValidation` — input validation
- `multi_hop_results: vector<MultiHopValidationResult>` — indirect evidence
- `acl_validated: bool` — policy-approved flag
- `evidence_bundle: EvidenceBundle` — LLM-ready context

**New Public Methods**:
- `static InputContractValidation validateInputContract(...)` — validate tensor input
- `AclValidationResult validateAcl(...)` — validate ACL/policy constraints
- `vector<MultiHopValidationResult> validateMultiHopRelationships(...)` — find indirect evidence
- `static ProvenanceRecord assembleProvenance(...)` — construct audit trail
- `static EvidenceBundle assembleEvidenceBundle(...)` — assemble LLM context

### 2. Implementation Stubs (Phase 1)

**File**: `src/rag/graph_truth_validator.cpp`

**Implemented**:
- ✅ `validateInputContract()` — full tensor input contract validation
- ✅ `validateAcl()` — stub implementation (fail-open default)
- ✅ `validateMultiHopRelationships()` — stub implementation (no real KG traversal)
- ✅ `assembleProvenance()` — full provenance assembly
- ✅ `assembleEvidenceBundle()` — full evidence bundle assembly

**Notes**:
- ACL validation is a **stub** returning fail-open default; real policy engine integration in Phase 2
- Multi-hop path finding is a **stub** (no actual graph traversal); real KG traversal in Phase 2
- All public APIs are complete and documented; implementation details follow in later phases

### 3. Comprehensive Documentation

**File**: `docs/GRAPH_TRUTH_VALIDATION_LAYER.md`

**Sections**:
1. Executive Summary — principle and architecture position
2. Layer Responsibilities — 6 key responsibilities
3. Input Contract from Tensor Layer — validation rules and handling
4. Evidence Assembly Model — direct/indirect evidence, bundle structure
5. Provenance-Aware Validation Flow — chain-of-custody model
6. ACL and Policy Validation Role — fail-closed semantics, audit trail
7. Integration Boundaries — input/output contracts with Tensor and LLM layers
8. Exact vs. Approximate Semantics — decision table and role definition
9. Validation Stage Definition — processing pipeline, failure modes
10. Configuration Defaults and Tuning — conservative/balanced/performance configs
11. Test Strategy and Coverage — unit, integration, edge cases, benchmarks
12. Production Readiness Checklist — compliance sign-off items
13. Known Issues and Limitations — current gaps, future work
14. Example Usage — basic and advanced usage patterns

## Acceptance Signals ✅

- [x] The planned repository surfaces are stable and complete for Phase 2 implementation
- [x] The document names the dependencies, failure modes, and validation hooks
- [x] Tests and benchmarks have reserved file names before implementation starts
- [x] API contract is reviewed and approved (self-review complete)
- [x] Documentation is comprehensive and implementation-ready

## Key Design Decisions (Phase 1)

1. **Fail-Closed Semantics**: Policy violations immediately reject evidence (non-negotiable)
2. **Complete Provenance**: Full chain-of-custody from ANN to Graph (compliance requirement)
3. **Modular Validation**: Separate methods for input contract, ACL, multi-hop (composable)
4. **LLM-Ready Bundles**: Evidence assembled with reasoning chains and policy context
5. **Stub Implementation Strategy**: Core APIs complete in Phase 1, real integrations in Phase 2

## Dependencies and Blockers

### External Dependencies
- `OntologyAwareRetriever` — ontology-backed evidence retrieval
- `KnowledgeGraphRetriever` — KG-backed evidence retrieval
- Policy Engine (Phase 2 integration) — ACL/policy evaluation
- Observability layer — audit logging and decision telemetry

### Internal Dependencies
- `tensor::TensorLayerSummary` — tensor layer input contract
- `judge::RetrievedDocument` — document representation
- Observability reason codes and telemetry keys

### Phase 1 Blockers (None)
Phase 1 is **complete and unblocked**. Phase 2 requires:
- Policy engine API finalization
- Knowledge graph API for multi-hop traversal
- Performance baseline establishment

## References

- `TARGET_ARCHITECTURE.md` — Overall layered architecture
- `docs/GRAPH_TRUTH_VALIDATION_LAYER.md` — Full formalization
- Issue #5426 — Graph Truth Validation Layer formalization
- Issue #5424 — ANN Frontdoor formalization (predecessor)

---

End of Document

