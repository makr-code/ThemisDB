# Graph Truth Validation Layer - Formalized Design

**Status**: Phase 1 - Design / API Contract Complete  
**Date**: 2026-07-01  
**Issue**: #5426  
**Target Completion**: 2026-08-06

---

## 1. Executive Summary

The Graph Truth Validation Layer formalizes graph-based exact semantic validation as a **first-class correctness layer** in ThemisDB's hybrid retrieval architecture.

### Architecture Position

```
ANN Frontdoor (fast retrieval)
        ↓
Tensor Mid-Layer (compression, routing)
        ↓
Graph Truth Layer (exact validation) ← THIS LAYER
        ↓
LLM/LoRA Final Layer (grounded generation)
```

### Key Principle

> **Final correctness belongs to the graph/policy validation layer.**
> 
> Approximate methods (ANN, Tensor) may prioritize and prefilter candidates, but final decisions involving semantics, security, compliance, or provenance must remain exact.

---

## 2. Layer Responsibilities

The Graph Truth Layer is responsible for:

### 2.1 Exact Relation Validation
- Verify semantic relationships between candidates and supporting evidence
- Confirm multi-hop paths in the knowledge graph
- Validate against explicit ontology constraints
- Reject candidates that violate graph cardinality or dependency rules

### 2.2 Provenance Tracking
- Maintain complete chain-of-custody from ANN retrieval through tensor compression to graph validation
- Record decision points at each layer
- Enable audit trails for compliance (e.g., SOX, GDPR)
- Preserve confidence/score evolution across stages

### 2.3 Evidence Chain Assembly
- Extract supporting nodes from the knowledge graph
- Identify direct relationships (1-hop evidence)
- Identify indirect relationships (multi-hop evidence)
- Bundle evidence with reasoning chains for LLM consumption

### 2.4 ACL and Policy Validation
- Enforce access control lists (ACL) on candidates
- Apply policy engine constraints (e.g., data classification, encryption requirements)
- Implement fail-closed semantics: any policy violation **rejects** evidence
- Provide detailed policy decision reasoning for audit

### 2.5 Policy-Aware Constraints
- Enforce data residency requirements
- Validate encryption/masking obligations
- Check temporal validity (e.g., data retention policies)
- Enforce role-based and attribute-based access control (RBAC/ABAC)

### 2.6 Exact Multi-Hop Validation
- Find and validate transitive relationships
- Confirm indirect evidence supports the candidate
- Limit hops to prevent combinatorial explosion (default: 3 hops)
- Track path confidence and intermediate node validity

---

## 3. Input Contract from Tensor Layer

The Tensor Mid-Layer must provide a well-formed `TensorLayerSummary` with:

### 3.1 Required Fields
```cpp
struct TensorLayerSummary {
    std::string scope_key;                          // Non-empty scope identifier
    TensorLayerKind layer_kind;                     // Valid layer kind
    index::AnnScopeKind ann_scope_kind;             // Valid ANN scope
    std::vector<SimilarityResult> similar_adapters; // Non-empty candidate list
    std::size_t candidate_count;                    // Matches adapter count
};
```

### 3.2 Validation Rules (Fail-Safe)
1. **Non-empty candidate list**: At least one candidate provided
2. **Valid scope key**: Non-empty, unique identifier
3. **Valid similarity scores**: All scores in [0.0, 1.0]
4. **Sufficient diversity**: Recommend ≥2 candidates for validation
5. **Consistent metadata**: Scope kind matches actual data type

### 3.3 Contract Violation Handling
- **Critical violations** (empty candidates, invalid scope): escalate to fallback path
- **Warnings** (single candidate, edge-case scores): log and continue
- **Recommendation logic**: "continue", "escalate", or "fallback"

---

## 4. Evidence Assembly Model

### 4.1 Evidence Types

#### Direct Evidence
- Nodes directly linked to the candidate in the knowledge graph
- Provided by OntologyAwareRetriever or KnowledgeGraphRetriever
- High confidence (graph-validated relationships)

#### Indirect Evidence (Multi-Hop)
- Relationships found through traversals of 2+ edges
- Enables transitive inference (e.g., "A supports B, B supports C → A indirectly supports C")
- Configurable depth (default 3 hops)
- Lower confidence than direct evidence

### 4.2 Evidence Bundle Structure

```cpp
struct EvidenceBundle {
    std::string candidate_id;
    
    // Direct evidence nodes from graph
    std::vector<std::string> direct_evidence_nodes;
    
    // Indirect (multi-hop) evidence with paths
    std::vector<MultiHopValidationResult> indirect_evidence_paths;
    
    // Complete provenance trail
    ProvenanceRecord provenance;
    
    // ACL/policy validation results
    AclValidationResult acl_result;
    
    // Readiness for LLM consumption
    bool ready_for_llm;
    double combined_confidence;
    
    // Structured context for LLM
    std::string llm_context;
    std::unordered_map<std::string, std::string> metadata;
};
```

### 4.3 Bundle Assembly Algorithm

1. **Retrieve direct evidence** from KG (ontology or plain retriever)
2. **Extract supporting nodes** and reasoning chains
3. **Validate multi-hop paths** (if enabled) up to `max_multi_hop_depth`
4. **Construct provenance record** tracing ANN→Tensor→Graph decisions
5. **Perform ACL/policy checks** (fail-closed on violations)
6. **Combine confidence scores** from tensor (0.3 weight) + graph (0.7 weight)
7. **Assemble LLM context** with evidence, provenance, and policy metadata
8. **Mark as ready** if: ACL passed AND graph validated AND provenance complete

---

## 5. Provenance-Aware Validation Flow

### 5.1 Provenance Record Components

```cpp
struct ProvenanceRecord {
    std::string evidence_id;                         // Unique identifier
    std::string source_layer;                        // "ANN" (origin)
    std::chrono::system_clock::time_point creation_timestamp;
    std::chrono::system_clock::time_point validation_timestamp;
    
    // Chain: ANN → Tensor → Graph
    std::vector<std::string> layer_decisions;
    
    // Graph nodes supporting this evidence
    std::vector<std::string> supporting_nodes;
    std::vector<std::string> indirect_supporting_nodes;
    
    // Graph edges forming the validation chain
    std::vector<std::pair<std::string, std::string>> validation_edges;
    
    // Confidence at each layer
    std::unordered_map<std::string, double> layer_confidence_scores;
    
    // Human-readable reasoning
    std::string reasoning_chain;
    
    // Policies validated against
    std::vector<std::string> validated_policies;
    
    // Principal performing validation
    std::string validation_principal;
    
    // Completeness indicator
    bool provenance_complete;
};
```

### 5.2 Validation Chain

1. **ANN Retrieval** → records candidate ID, retrieval timestamp, initial confidence
2. **Tensor Compression** → records routing decision, scope normalization
3. **Graph Validation** → records supporting nodes, validation decision
4. **Policy Engine** → records ACL results, policy decisions, principal
5. **Audit Trail** → constructs reasoning chain from all stages

### 5.3 Completeness Criteria

Provenance is **complete** when:
- ✅ All layer decision points recorded
- ✅ Supporting nodes identified
- ✅ Validation timestamp recorded
- ✅ Principal authenticated
- ✅ No missing intermediate steps

Provenance is **broken** when:
- ❌ Supporting nodes empty (no graph evidence found)
- ❌ Missing principal identity
- ❌ Layer decision gaps
- ❌ Timestamp inconsistencies

**Handling broken provenance**:
- Log warning with details
- Mark `provenance_complete = false`
- Still usable if graph validates (lower compliance confidence)
- Escalate to audit system for investigation

---

## 6. ACL and Policy Validation Role

### 6.1 Policy Validation Interface

```cpp
AclValidationResult GraphTruthValidator::validateAcl(
    const std::string& candidate_id,
    const std::string& principal,
    const std::string& action,
    const std::unordered_map<std::string, std::string>& context = {}
) const;
```

### 6.2 Policy Decision Logic (Fail-Closed)

```
For each candidate:
    If policy_engine_.evaluate(principal, action, candidate) == DENY:
        ❌ Evidence rejected
        ❌ Log policy violation
        ❌ Add to policy_violations list
        → candidate NOT added to result
    Else if policy_engine_.evaluate(...) == ALLOW:
        ✅ Evidence accepted
        ✅ Record granting policies
        → candidate added to result
    Else if NO policy engine configured:
        ⚠️  Degraded mode
        ⚠️  Default ALLOW (fail-open)
        → log warning, add with caveat
```

### 6.3 Policy Categories

| Category | Example | Decision |
|----------|---------|----------|
| **Data Classification** | "Public", "Confidential", "Restricted" | Principal clearance must match |
| **Encryption** | "AES-256", "RSA-2048" | Candidate must be encrypted per policy |
| **Data Residency** | "US-only", "EU-only" | Check candidate location |
| **Retention** | "30-day retention" | Check evidence age |
| **RBAC** | "Admin", "Editor", "Viewer" | Principal role required |
| **ABAC** | Department, cost center, project | Attribute matching |

### 6.4 Audit Trail

Every policy decision is logged:
```
timestamp: 2026-07-01T10:45:32Z
principal: alice@example.com
action: use_in_generation
candidate: doc-12345
decision: DENY
policies: [data-classification-policy-v2, encryption-policy-v1]
denied_by: data-classification-policy-v2 (principal lacks SECRET clearance)
reason_code: ACL_CLEARANCE_INSUFFICIENT
```

---

## 7. Integration Boundaries with Tensor Layer

### 7.1 Input Boundary (Tensor → Graph)

**Tensor Layer provides**:
- `TensorLayerSummary` with candidates and confidence scores
- Routing and scope normalization
- Compression metadata (fingerprints, tensor ranks)

**Graph Layer validates**:
- Input contract (well-formedness)
- Candidate authenticity against knowledge graph
- Temporal validity
- Policy compliance

### 7.2 Output Boundary (Graph → LLM)

**Graph Layer produces**:
- Validated evidence bundles
- Provenance trails
- Policy decision metadata
- Confidence aggregation

**LLM Layer consumes**:
- `llm_context` (structured evidence)
- Provenance for citations
- Policy metadata (e.g., data classification)
- Confidence for answer grounding

### 7.3 Failure Handling

| Failure | Tensor Behavior | Graph Behavior |
|---------|-----------------|-----------------|
| **Invalid input** | Escalate to fallback | Reject with reason, log alert |
| **Policy violation** | N/A (tensor doesn't check) | Fail-closed: remove evidence |
| **Graph unavailable** | N/A | Degrade to continue (if configured) |
| **Broken provenance** | N/A | Log warning, still use evidence |

---

## 8. Exact vs. Approximate Semantics

### 8.1 ANN Layer (Approximate)
- Fast semantic search via embeddings
- Vector-based similarity
- No semantic guarantees
- **Role**: Quick candidate shortlist

### 8.2 Tensor Layer (Hybrid)
- Candidate compression and deduplication
- Routing optimization
- Approximate relational signals
- **Role**: Reduce noise, prepare for exact layer

### 8.3 Graph Layer (Exact)
- Explicit semantic relationships
- Entity and relation triples
- Ontology-backed constraints
- Policy and ACL enforcement
- **Role**: Final arbiter of correctness

### 8.4 Decision Table

| Query Type | ANN Output | Tensor Decision | Graph Validation |
|-----------|-----------|-----------------|------------------|
| "What is the capital of France?" | Similar documents | Top K after compression | Validate "Paris" in ontology |
| "Who are authorized users?" | User-related docs | Filter by role | Apply ACL, check permissions |
| "Show confidential data" | Semantic matches | Rank by relevance | Apply encryption policy, deny if not authorized |

**Principle**: Tensor may reorder candidates for efficiency, but Graph makes final semantic and compliance decisions.

---

## 9. Validation Stage Definition

### 9.1 Stage Inputs

```
From Tensor Layer:
├─ query: original user query
├─ tensor_summary: candidates with scores
├─ scope_key: normalized scope (doc, chunk, entity, adapter)
└─ routing_reason: why tensor chose this routing

From Configuration:
├─ enable_acl_validation: require policy checks
├─ enable_provenance_tracking: full chain of custody
├─ enable_multi_hop_validation: find indirect evidence
├─ max_evidence_candidates: limit processing
├─ max_multi_hop_depth: limit traversal
└─ min_graph_truth_score: acceptance threshold
```

### 9.2 Stage Processing

```
1. Input Contract Validation
   ↓
2. Evidence Retrieval (Ontology/KG)
   ↓
3. Direct Evidence Extraction
   ↓
4. [Optional] Multi-Hop Validation
   ↓
5. [Optional] ACL/Policy Checks
   ↓
6. Provenance Assembly
   ↓
7. Evidence Bundle Compilation
   ↓
8. Output Generation
```

### 9.3 Stage Outputs

```
GraphTruthValidationResult:
├─ evidences[]: validated candidates with confidence
├─ input_contract_validation: well-formedness check
├─ acl_validated_count: policy-approved candidates
├─ multi_hop_count: indirect evidence found
├─ complete_provenance_count: full audit trails
├─ policy_violations[]: denied candidates
├─ all_validations_passed: ready for LLM?
└─ audit_trail: compliance log
```

### 9.4 Failure Modes and Recovery

| Failure | Symptom | Recovery |
|---------|---------|----------|
| **No retriever configured** | No evidence found | Log warning, degrade to continue |
| **Empty input candidates** | Input contract fails | Escalate to fallback, log alert |
| **Policy rejection** | Candidate in violations list | Fail-closed, exclude from results |
| **Broken provenance** | Missing supporting nodes | Log warning, use evidence (lower confidence) |
| **Multi-hop timeout** | Traversal depth exceeded | Truncate paths, use direct evidence |

---

## 10. Configuration Defaults and Tuning

### 10.1 Conservative Configuration (High Assurance)

```cpp
GraphTruthValidatorConfig config{
    .max_evidence_candidates = 4,
    .use_ontology_validation = true,
    .min_graph_truth_score = 0.5,          // Higher threshold
    .enable_acl_validation = true,         // Mandatory
    .enable_provenance_tracking = true,    // Full audit
    .enable_multi_hop_validation = true,
    .max_multi_hop_depth = 2               // Limit traversal
};
```
**Use case**: Regulatory, compliance-critical, financial services

### 10.2 Balanced Configuration (Default)

```cpp
GraphTruthValidatorConfig config{
    .max_evidence_candidates = 8,
    .use_ontology_validation = true,
    .min_graph_truth_score = 0.15,
    .enable_acl_validation = true,
    .enable_provenance_tracking = true,
    .enable_multi_hop_validation = true,
    .max_multi_hop_depth = 3
};
```
**Use case**: General-purpose enterprise

### 10.3 Performance Configuration (Lower Latency)

```cpp
GraphTruthValidatorConfig config{
    .max_evidence_candidates = 16,         // More parallel checks
    .use_ontology_validation = true,       // Slightly faster than KG
    .min_graph_truth_score = 0.1,          // Lower threshold
    .enable_acl_validation = true,         // Still mandatory
    .enable_provenance_tracking = false,   // Skip full audit
    .enable_multi_hop_validation = false,  // Skip multi-hop
    .max_multi_hop_depth = 1
};
```
**Use case**: High-throughput RAG systems with basic policy needs

---

## 11. Test Strategy and Coverage

### 11.1 Unit Tests

- ✅ Input contract validation (valid, invalid, edge cases)
- ✅ Evidence extraction (direct, multi-hop, reasoning chains)
- ✅ Provenance assembly (complete, broken, partial trails)
- ✅ ACL validation (pass, deny, policy details)
- ✅ Evidence bundle assembly (completeness, LLM context)

### 11.2 Integration Tests

- ✅ ANN → Tensor → Graph validation pipeline
- ✅ Graph validation with both Ontology and KG retrievers
- ✅ Policy engine integration (RBAC, ABAC, data classification)
- ✅ Multi-shard federated validation
- ✅ Fallback and degradation paths

### 11.3 Edge Cases

- ✅ Empty candidate lists
- ✅ Single candidate (no diversity)
- ✅ Conflicting evidence
- ✅ Broken multi-hop paths
- ✅ Missing ACL policies
- ✅ Policy engine unavailable
- ✅ Knowledge graph disconnection

### 11.4 Performance Benchmarks

- ✅ Throughput: candidates/sec (target: 100+ per sec per validator instance)
- ✅ Latency: ms per candidate (target: p50 <50ms, p99 <200ms)
- ✅ Memory: evidences buffer (target: <50MB for 10K candidates)
- ✅ Graph traversal: hops/sec (target: 1000+ multi-hop checks/sec)

---

## 12. Production Readiness Checklist

- [ ] 100% path coverage for all validation flows
- [ ] Policy and ACL test coverage ≥95%
- [ ] Integration tests with real graph and policy engine
- [ ] Performance benchmarks meet targets
- [ ] Provenance audit trails validated
- [ ] Multi-hop traversal limits enforced
- [ ] Fail-closed semantics verified
- [ ] Degradation modes tested
- [ ] Documentation complete
- [ ] Compliance audit sign-off

---

## 13. Known Issues and Limitations

### 13.1 Current Limitations

1. **Policy extensions still vague**
   - ACL validation is a stub (fail-open default)
   - Real integration with policy engine TBD in Phase 2
   - Attribute-based access control (ABAC) design pending

2. **Multi-hop traversal incomplete**
   - Path finding uses stub (no real graph traversal)
   - Confidence aggregation for multi-hop paths TBD
   - Cycle detection and path deduplication needed

3. **Provenance chain may break**
   - If tensor layer doesn't record source info
   - If graph can't link back to original ANN result
   - Mitigation: log warnings, accept evidence anyway (lower confidence)

4. **Performance untested at scale**
   - Benchmarks run on single validator instance
   - Distributed validation across shards TBD
   - Lock contention in high-concurrency scenarios unknown

### 13.2 Future Work

- Phase 2: Implement real ACL/policy engine integration
- Phase 3: Add full multi-hop path finding with confidence aggregation
- Phase 4: Performance tuning at 1M+ candidate scale
- Phase 5: Distributed/federated validation across shards
- Phase 6: Streaming provenance and audit log output

---

## 14. References

- `TARGET_ARCHITECTURE.md` — Overall layered architecture
- `TENSOR_MIDLAYER_DESIGN.md` — Tensor layer specifics
- `FUTURE_PLAN.md` — Long-term graph evolution
- Issue #5426 — This formalization task
- Issue #5424 — ANN Frontdoor formalization (predecessor)

---

## 15. Example Usage

### 15.1 Basic Validation

```cpp
#include "rag/graph_truth_validator.h"

auto validator = std::make_shared<GraphTruthValidator>();
validator->setOntologyRetriever(ontology_retriever);

tensor::TensorLayerSummary tensor_summary{...};

GraphTruthValidatorConfig config{
    .enable_acl_validation = true,
    .enable_provenance_tracking = true
};

auto result = validator->validate(
    "What is the capital of France?",
    tensor_summary,
    config,
    "correlation-id-12345"
);

// Check result
if (result.all_validations_passed) {
    for (const auto& evidence : result.evidences) {
        if (evidence.validated && evidence.acl_validated) {
            llm_context.evidences.push_back(evidence.evidence_bundle);
        }
    }
}
```

### 15.2 Advanced: Policy-Critical Application

```cpp
GraphTruthValidatorConfig config{
    .max_evidence_candidates = 4,
    .min_graph_truth_score = 0.5,
    .enable_acl_validation = true,           // Mandatory
    .enable_provenance_tracking = true,      // Full audit
    .enable_multi_hop_validation = true,
    .max_multi_hop_depth = 2
};

auto result = validator->validate(query, tensor_summary, config, correlation_id);

// Audit all decisions
for (const auto& evidence : result.evidences) {
    audit_logger->logValidation({
        .candidate_id = evidence.candidate_id,
        .principal = evidence.provenance.validation_principal,
        .decision = evidence.validated ? "ACCEPT" : "REJECT",
        .acl_passed = evidence.acl_validated,
        .provenance_complete = evidence.provenance.provenance_complete,
        .policy_violations = result.policy_violations
    });
}
```

---

## 16. Contacts and Governance

- **Owner**: [@makr-code](https://github.com/makr-code)
- **Status**: Phase 1 (Design) — 2026-07-01 to 2026-07-15
- **Next Review**: 2026-07-15 (Phase 1 → Phase 2 approval)

---

**End of Document**
