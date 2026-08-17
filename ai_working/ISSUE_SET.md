# ISSUE_SET.md

# ThemisDB Issue Set
## Hybrid Knowledge Retrieval Architecture, Evaluation, and Distributed Tensor Sharding

**Status:** Draft  
**Date:** 2026-06-01

---

## Purpose

This document contains the complete proposed GitHub issue set for implementing the ThemisDB target architecture.

It includes:
- 3 wrapper epics
- 21 sub-issues
- suggested ordering
- repository document references

---

# EPIC 1

## Title
`[EPIC] Implement hybrid knowledge retrieval architecture (ANN -> Tensor -> Graph -> LLM/LoRA)`

## Body

```markdown
## Summary

Implement the target hybrid knowledge retrieval architecture for ThemisDB as a layered system:

1. ANN Frontdoor
2. Tensor Mid-Layer
3. Graph Truth Layer
4. LLM / LoRA Final Layer

This epic covers the core implementation track required to move ThemisDB from a modular hybrid RAG system toward a layered knowledge retrieval architecture.

## Motivation

ThemisDB already contains strong capabilities across graph, vector retrieval, training, provenance, sharding, and LoRA-related subsystems. However, these capabilities are not yet fully integrated into a coherent layered architecture.

The goal of this epic is to make the target architecture operational.

## Scope

This epic includes:

- formalizing the ANN frontdoor,
- defining and implementing tensor mid-layer abstractions,
- formalizing graph truth validation,
- evolving LoRA artifacts toward package/product lifecycle support,
- implementing model-switch workflows,
- integrating federated / cross-shard tensor summaries,
- improving observability and governance boundaries.

## Out of Scope

This epic does not itself define the full benchmark framework or hardware sizing strategy. Those belong to dedicated companion issues and epics.

## Reference Documents

- `FUTURE_PLAN.md`
- `TARGET_ARCHITECTURE.md`
- `GAP_ANALYSIS.md`
- `DISTRIBUTED_TENSOR_SHARDING.md`

## Desired Outcome

A first operational version of the layered retrieval architecture exists in ThemisDB, with clear boundaries between:

- ANN candidate generation,
- tensor-based compression/routing,
- graph-based evidence/provenance validation,
- LLM/LoRA final generation.

## Sub-Issues

- Formalize ANN Frontdoor
- Implement Tensor Mid-Layer abstractions
- Formalize Graph Truth Validation Layer
- Implement LoRAPackage / PortableAdapterProduct
- Implement model-switch workflow
- Implement federated / cross-shard tensor summaries
- Observability / governance / ADRs
```

---

## Sub-Issue 1.1
### Title
`Formalize ANN Frontdoor for layered retrieval`

### Body

```markdown
## Summary

Formalize the ANN Frontdoor as the first retrieval layer in the ThemisDB hybrid knowledge retrieval architecture.

## Motivation

The target architecture requires a clear first-stage semantic retrieval layer that is responsible for fast candidate generation before tensor refinement and graph validation.

ANN should become a first-class architectural concept rather than an implicit retrieval detail.

## Goals

- define ANN frontdoor abstraction
- clarify HNSW vs DiskANN usage
- define hot/cold retrieval behavior
- support ANN retrieval over:
  - documents
  - chunks
  - entities
  - adapter fingerprints
  - package fingerprints
  - shard summaries

## Deliverables

- architectural interface / abstraction for ANN frontdoor
- initial ANN routing rules
- explicit retrieval stage contract
- documentation of supported ANN artifact classes

## Open Questions

- when should HNSW be preferred over DiskANN?
- which artifact classes should be ANN-indexed first?
- what metadata is required for shard-aware ANN routing?

## References

- `TARGET_ARCHITECTURE.md`
- `GAP_ANALYSIS.md`
- `HARDWARE_REQUIREMENTS.md`
```

---

## Sub-Issue 1.2
### Title
`Implement Tensor Mid-Layer abstractions for compression, routing, and summaries`

### Body

```markdown
## Summary

Define and implement the Tensor Mid-Layer as the structural compression and routing layer between ANN retrieval and graph validation.

## Motivation

Tensor-oriented capabilities exist in ThemisDB, but they are not yet formalized as a coherent system-level abstraction for candidate compression, summaries, routing, and similarity fingerprints.

## Goals

- define tensor mid-layer abstraction
- define tensor fingerprints
- define tensor summary types
- define routing and prioritization interfaces
- support candidate compression and redundancy reduction
- align tensor outputs with graph validation input requirements

## Deliverables

- core tensor abstraction design
- tensor summary model
- tensor fingerprint model
- routing/compression API surfaces
- documentation for tensor mid-layer responsibilities

## Open Questions

- which tensor summary types should be implemented first?
- what level of approximation is acceptable before graph validation?
- which outputs are persistent vs derived vs ephemeral?

## References

- `FUTURE_PLAN.md`
- `TARGET_ARCHITECTURE.md`
- `DISTRIBUTED_TENSOR_SHARDING.md`
```

---

## Sub-Issue 1.3
### Title
`Formalize Graph Truth Validation Layer`

### Body

```markdown
## Summary

Formalize the Graph Truth Layer as the exact semantic, evidence, provenance, and policy validation stage after ANN and tensor-based approximation.

## Motivation

The graph layer must remain the exact validation layer in the target architecture. It should not be treated as an optional post-filter, but as a first-class correctness layer.

## Goals

- define graph truth layer responsibilities
- define graph evidence assembly flow
- define provenance-aware validation flow
- define policy and ACL validation role
- define graph input contract from tensor/ANN outputs

## Deliverables

- graph validation stage definition
- evidence assembly model
- provenance validation flow
- integration boundaries with tensor layer
- documentation of exact-vs-approximate semantics

## Open Questions

- how should evidence sets be represented for LLM input?
- which graph validations are always mandatory?
- which graph checks may remain optional or configurable?

## References

- `TARGET_ARCHITECTURE.md`
- `GAP_ANALYSIS.md`
- `EVALUATION_FRAMEWORK.md`
```

---

## Sub-Issue 1.4
### Title
`Implement LoRAPackage and PortableAdapterProduct artifact model`

### Body

```markdown
## Summary

Introduce a package/product-oriented artifact model for LoRA and related adapter lifecycle management.

## Motivation

The current direction requires a distinction between rebuildable source-oriented adapter artifacts and deployable model-bound adapter products.

## Goals

- define LoRAPackage concept
- define PortableAdapterProduct concept
- define package lineage and provenance metadata
- define compatibility metadata
- align with model-switch workflow and rebuild logic

## Deliverables

- artifact schemas
- lifecycle model
- package/product distinction
- integration with registry/provenance systems
- documentation for rebuild vs deploy semantics

## Open Questions

- what metadata is required for robust rebuild?
- which product formats should be supported?
- how should adapter compatibility be expressed across base models?

## References

- `FUTURE_PLAN.md`
- `research/RESEARCH_PAPER.md`
- `DISTRIBUTED_TENSOR_SHARDING.md`
```

---

## Sub-Issue 1.5
### Title
`Implement model-switch workflow with compatibility matrix and rebuild-first logic`

### Body

```markdown
## Summary

Implement a model-switch workflow for adapters/packages based on compatibility metadata and rebuild-first logic.

## Motivation

The target architecture assumes that model changes should not silently invalidate adapter artifacts. Instead, model-switches should be managed via explicit compatibility and rebuild policies.

## Goals

- define compatibility matrix model
- define rebuild-first workflow
- define validation gates after model switch
- define failure handling for incompatible products
- connect model-switch logic to package lineage

## Deliverables

- compatibility metadata structure
- model-switch decision flow
- rebuild policy definition
- validation criteria after rebuild
- documentation of supported switch scenarios

## Open Questions

- which changes are considered compatible?
- when is rebuild mandatory?
- how should failed rebuilds be surfaced to higher layers?

## References

- `FUTURE_PLAN.md`
- `GAP_ANALYSIS.md`
- `DISTRIBUTED_TENSOR_SHARDING.md`
```

---

## Sub-Issue 1.6
### Title
`Implement federated and cross-shard tensor summaries`

### Body

```markdown
## Summary

Implement shard-level tensor summaries and summary-first retrieval behavior for distributed and federated ThemisDB deployments.

## Motivation

Distributed retrieval should not require immediate broad fan-out or full graph/data movement. Tensor summaries should support efficient shard selection and selective exact loading.

## Goals

- define shard summary representation
- support summary-first retrieval
- support selective exact loading
- integrate shard summaries into retrieval planning
- support distributed/federated retrieval flows

## Deliverables

- shard summary artifact definition
- retrieval semantics for summary-first flows
- integration with ANN/tensor/graph pipeline
- documentation for distributed summary usage

## Open Questions

- what are the false-negative risks of summary-first routing?
- how large should shard summaries be?
- which shard summary data must be replicated?

## References

- `FUTURE_PLAN.md`
- `DISTRIBUTED_TENSOR_SHARDING.md`
- `HARDWARE_REQUIREMENTS.md`
```

---

## Sub-Issue 1.7
### Title
`Add observability, governance boundaries, and ADR support for layered retrieval`

### Body

```markdown
## Summary

Add observability, explainability, governance boundaries, and architecture decision support for the layered retrieval architecture.

## Motivation

A multi-layer architecture requires strong tracing and explicit boundaries for where approximation is allowed and where exactness is mandatory.

## Goals

- define approximation boundaries
- improve layered tracing
- improve explainability
- document governance-sensitive boundaries
- produce ADRs for major architectural decisions

## Deliverables

- observability requirements
- explainability and tracing requirements
- governance boundary rules
- ADR list and decision topics

## Open Questions

- how should approximation decisions be surfaced?
- how much retrieval-path telemetry is required?
- which decisions require ADRs immediately?

## References

- `GAP_ANALYSIS.md`
- `EVALUATION_FRAMEWORK.md`
- `HARDWARE_REQUIREMENTS.md`
```

---

# EPIC 2

## Title
`[EPIC] Evaluation, hardware profiles, benchmarking and hybrid query planning`

## Body

```markdown
## Summary

Establish the evaluation, benchmarking, hardware profiling, and hybrid query planning foundations required to support the target ThemisDB architecture.

## Motivation

The layered architecture cannot be validated by intuition alone. It requires benchmark-driven, hardware-aware, and scientifically grounded evaluation.

This epic covers the operational and research-support foundation needed to make architectural decisions measurable.

## Scope

This epic includes:

- hardware profiles and sizing guidance,
- benchmark matrices,
- evaluation metrics and ablations,
- approximation boundary rules,
- hybrid query planner design,
- derived-artifact lifecycle and staleness management,
- quantization / mmap / zero-copy assessments.

## Reference Documents

- `HARDWARE_REQUIREMENTS.md`
- `EVALUATION_FRAMEWORK.md`
- `research/RESEARCH_PAPER.md`

## Desired Outcome

The architecture can be evaluated, benchmarked, and tuned across multiple hardware and deployment profiles, with a clear planner direction and well-defined tradeoff metrics.

## Sub-Issues

- Define hardware profiles and sizing
- Create benchmark matrix for ANN / Tensor / Graph / LLM
- Implement evaluation metrics and ablation framework
- Define approximation boundaries and governance rules
- Design hybrid query planner
- Define derived-artifact lifecycle and staleness management
- Assess quantization / mmap / zero-copy strategy
```

---

## Sub-Issue 2.1
### Title
`Define hardware profiles and sizing for layered ThemisDB deployments`

### Body

```markdown
## Summary

Define hardware profiles and sizing assumptions for development, production, and federated deployments of the layered ThemisDB architecture.

## Motivation

Different layers stress different resources. Hardware assumptions must therefore be made explicit.

## Goals

- define dev profile
- define production profile
- define high-performance / federated profile
- clarify RAM / NVMe / GPU / network expectations
- document hot/warm/cold tier implications

## Deliverables

- hardware profile definitions
- sizing guidance
- infrastructure assumptions by layer
- initial break-even considerations

## Open Questions

- what is the smallest realistic production profile?
- when does DiskANN become preferable?
- how much GPU/VRAM is required for representative workloads?

## References

- `HARDWARE_REQUIREMENTS.md`
```

---

## Sub-Issue 2.2
### Title
`Create benchmark matrix for HNSW, DiskANN, Tensor Mid-Layer, Graph validation, and LLM/LoRA flows`

### Body

```markdown
## Summary

Create a benchmark matrix covering the major architecture paths and break-even comparisons in ThemisDB.

## Motivation

The target architecture introduces multiple competing or complementary strategies that require direct comparison.

## Goals

- define benchmark scenarios
- compare HNSW vs DiskANN
- compare summary-first vs direct retrieval
- compare ANN-only vs ANN+Tensor vs ANN+Tensor+Graph
- compare distributed shard-summary flows
- include prompt-size and faithfulness impacts

## Deliverables

- benchmark matrix
- scenario definitions
- evaluation dimensions
- break-even comparison plan

## Open Questions

- which workloads should be the canonical benchmarks?
- which datasets best represent target usage?
- how should distributed scenarios be simulated?

## References

- `EVALUATION_FRAMEWORK.md`
- `HARDWARE_REQUIREMENTS.md`
```

---

## Sub-Issue 2.3
### Title
`Implement evaluation metrics and ablation framework for layered retrieval`

### Body

```markdown
## Summary

Define and implement the evaluation metrics and ablation framework for the layered ThemisDB architecture.

## Motivation

A layered system must be evaluated along multiple axes beyond recall@k, including evidence quality, provenance fidelity, prompt reduction, and distributed efficiency.

## Goals

- define core metrics
- define evidence metrics
- define provenance metrics
- define compression metrics
- define LLM answer metrics
- define distributed metrics
- define ablation methodology

## Deliverables

- evaluation metric definitions
- ablation framework
- baseline comparison plan
- reporting format

## Open Questions

- how should evidence quality be scored?
- how should approximation loss be estimated?
- which answer-quality metrics are most actionable?

## References

- `EVALUATION_FRAMEWORK.md`
- `research/RESEARCH_PAPER.md`
```

---

## Sub-Issue 2.4
### Title
`Define approximation boundaries and governance rules for layered retrieval`

### Body

```markdown
## Summary

Define where approximation is permitted in the layered architecture and where exact validation is mandatory.

## Motivation

Approximation is central to ANN and tensor usage, but governance-sensitive decisions must remain exact.

## Goals

- identify approximation-permitted stages
- identify exact-required stages
- define governance-sensitive boundaries
- define policy/ACL/provenance exactness requirements
- align with observability and evaluation

## Deliverables

- approximation boundary rules
- governance matrix
- exactness requirements by layer
- documentation for engineering usage

## Open Questions

- what qualifies as governance-sensitive?
- which approximation failures are tolerable?
- how should fallback-to-exact be triggered?

## References

- `EVALUATION_FRAMEWORK.md`
- `TARGET_ARCHITECTURE.md`
- `GAP_ANALYSIS.md`
```

---

## Sub-Issue 2.5
### Title
`Design hybrid query planner for ANN, Tensor, Graph, and distributed retrieval flows`

### Body

```markdown
## Summary

Design the hybrid query planner that decides how retrieval should flow across ANN, tensor, graph, and distributed shard-summary paths.

## Motivation

As the architecture becomes layered, retrieval behavior must be planned rather than hardcoded into ad hoc logic.

## Goals

- define planner responsibilities
- define candidate path selection rules
- define cost-aware and confidence-aware routing
- support summary-first shard planning
- support escalation to exact graph validation
- define planner inputs and outputs

## Deliverables

- planner design document
- planner decision model
- routing criteria
- fallback-to-exact logic
- integration points with retrieval layers

## Open Questions

- what signals should drive planner decisions?
- should planner logic be rule-based, scored, or learned?
- how should planner decisions be surfaced for debugging?

## References

- `TARGET_ARCHITECTURE.md`
- `EVALUATION_FRAMEWORK.md`
- `DISTRIBUTED_TENSOR_SHARDING.md`
```

---

## Sub-Issue 2.6
### Title
`Define derived-artifact lifecycle and staleness management`

### Body

```markdown
## Summary

Define lifecycle and freshness rules for derived artifacts such as ANN indexes, tensor summaries, shard summaries, and routing artifacts.

## Motivation

The architecture relies heavily on derived artifacts. Their staleness and rebuild behavior must be explicit.

## Goals

- define source-of-truth vs derived artifact policy
- define freshness metadata
- define invalidation rules
- define rebuild triggers
- define staleness handling behavior
- define disposable vs reconstructable classes

## Deliverables

- lifecycle rules
- freshness model
- invalidation/rebuild policy
- engineering guidance for artifact handling

## Open Questions

- which artifacts must be rebuilt eagerly?
- which artifacts may be stale within tolerance windows?
- how should stale artifacts affect planner behavior?

## References

- `DISTRIBUTED_TENSOR_SHARDING.md`
- `GAP_ANALYSIS.md`
```

---

## Sub-Issue 2.7
### Title
`Assess quantization, mmap, and zero-copy strategy for tensor and adapter artifacts`

### Body

```markdown
## Summary

Assess storage and runtime strategies for tensor and adapter artifacts, including quantization, mmap-friendly formats, and zero-copy loading.

## Motivation

Performance and deployment cost will depend heavily on artifact layout and loading strategy.

## Goals

- define candidate artifact formats
- assess mmap feasibility
- assess zero-copy opportunities
- assess quantization tradeoffs
- align artifact strategy with hardware tiers

## Deliverables

- strategy document
- candidate format evaluation
- tradeoff analysis
- implementation recommendations

## Open Questions

- which tensor artifacts can be safely quantized?
- which formats are best for immutable mmap-backed artifacts?
- where does zero-copy deliver real benefit?

## References

- `HARDWARE_REQUIREMENTS.md`
- `DISTRIBUTED_TENSOR_SHARDING.md`
- `EVALUATION_FRAMEWORK.md`
```

---

# EPIC 3

## Title
`[EPIC] Distributed tensor artifacts, RAID-style sharding, integrity and recovery`

## Body

```markdown
## Summary

Design and implement distributed tensor artifact handling in the Themis sharding fabric, including RAID-style redundancy concepts, integrity verification, recovery, manifest-driven coordination, and factorization-aware placement.

## Motivation

Tensor artifacts are becoming first-class objects in ThemisDB. They therefore require explicit distributed systems treatment, not only local in-memory handling.

This epic focuses on tensor artifacts as distributed knowledge artifacts.

## Scope

This epic includes:

- tensor artifact classes and lifecycle,
- manifest-driven artifact coordination,
- factorization-aware shard placement,
- integrity and provenance verification,
- recovery and rebuild strategy,
- query planner integration for distributed tensor retrieval,
- hardware and network considerations for the tensor artifact fabric.

## Reference Documents

- `DISTRIBUTED_TENSOR_SHARDING.md`
- `HARDWARE_REQUIREMENTS.md`
- `EVALUATION_FRAMEWORK.md`
- `research/RESEARCH_PAPER.md`

## Desired Outcome

ThemisDB gains a clear design and initial implementation path for distributed tensor artifacts that are:

- verifiable,
- reconstructable,
- selectively loadable,
- provenance-preserving,
- aligned with package lineage and graph-validated retrieval.

## Sub-Issues

- Define tensor artifact classes and lifecycle
- Design manifest schema for distributed tensor artifacts
- Implement factorization-aware shard placement strategy
- Define integrity / Merkle / receipt model for tensor artifacts
- Define recovery / rebuild / erasure strategy
- Integrate distributed tensor retrieval with hybrid query planner
- Assess hardware and network implications of tensor artifact fabric
```

---

## Sub-Issue 3.1
### Title
`Define tensor artifact classes and lifecycle for distributed Themis sharding`

### Body

```markdown
## Summary

Define tensor artifact classes and lifecycle rules for distributed tensor handling in ThemisDB.

## Motivation

Primary, derived, and ephemeral tensor artifacts have different durability, replication, rebuild, and query requirements.

## Goals

- define primary tensor artifacts
- define derived tensor artifacts
- define ephemeral tensor artifacts
- define lifecycle expectations for each
- align artifact classes with recovery and integrity strategies

## Deliverables

- tensor artifact class definitions
- lifecycle model
- artifact handling policy
- documentation of rebuildability and durability expectations

## References

- `DISTRIBUTED_TENSOR_SHARDING.md`
```

---

## Sub-Issue 3.2
### Title
`Design manifest schema for distributed tensor artifacts`

### Body

```markdown
## Summary

Design the manifest schema used to coordinate distributed tensor artifacts in the Themis sharding fabric.

## Motivation

Distributed tensor artifacts should be manifest-driven so that placement, reconstruction, provenance, and integrity remain explicit.

## Goals

- define manifest schema
- include content hash and manifest hash
- include provenance links
- include package lineage
- include placement metadata
- include reconstruction metadata
- include freshness/version metadata

## Deliverables

- manifest schema proposal
- manifest field definitions
- artifact/manifest relationship model

## References

- `DISTRIBUTED_TENSOR_SHARDING.md`
```

---

## Sub-Issue 3.3
### Title
`Implement factorization-aware shard placement strategy for tensor artifacts`

### Body

```markdown
## Summary

Define and prototype factorization-aware placement for selected tensor artifacts such as TT cores, factor matrices, and modular tensor packages.

## Motivation

Generic block placement may ignore tensor structure. For some artifact classes, factorization-aware placement may improve partial loading and reconstruction efficiency.

## Goals

- identify artifact classes suitable for factorization-aware placement
- define placement strategy candidates
- assess placement/recovery tradeoffs
- prototype selected placement behavior

## Deliverables

- placement strategy design
- candidate artifact list
- prototype or design notes
- tradeoff analysis

## References

- `DISTRIBUTED_TENSOR_SHARDING.md`
- `HARDWARE_REQUIREMENTS.md`
```

---

## Sub-Issue 3.4
### Title
`Define integrity, Merkle, and receipt-chain model for distributed tensor artifacts`

### Body

```markdown
## Summary

Define the integrity and verification model for distributed tensor artifacts, including Merkle-compatible fragment verification and receipt-chain-aligned metadata.

## Motivation

Tensor artifacts must be verifiable across shards and across rebuild workflows.

## Goals

- define content-hash model
- define fragment verification model
- define Merkle-compatible structure
- define provenance verification hooks
- align with package lineage and receipt-style verification

## Deliverables

- integrity model
- verification rules
- Merkle/receipt integration notes
- artifact verification workflow

## References

- `DISTRIBUTED_TENSOR_SHARDING.md`
```

---

## Sub-Issue 3.5
### Title
`Define recovery, rebuild, and erasure strategy for distributed tensor artifacts`

### Body

```markdown
## Summary

Define how distributed tensor artifacts should be recovered or rebuilt under shard loss, corruption, mismatch, or incompatibility events.

## Motivation

Tensor artifact failure handling must be explicit if these artifacts are to become first-class distributed objects.

## Goals

- define replication fallback strategy
- define rebuild-from-lineage strategy
- define summary regeneration rules
- define invalidation/rematerialization behavior
- assess erasure/parity support where justified

## Deliverables

- recovery matrix
- rebuild policy
- failure scenario handling rules
- engineering guidance for implementation

## References

- `DISTRIBUTED_TENSOR_SHARDING.md`
- `HARDWARE_REQUIREMENTS.md`
```

---

## Sub-Issue 3.6
### Title
`Integrate distributed tensor retrieval with hybrid query planner`

### Body

```markdown
## Summary

Integrate distributed tensor artifact semantics into the hybrid query planner.

## Motivation

Summary-first routing, exact-on-demand loading, and shard-aware tensor retrieval require planner support.

## Goals

- define planner inputs from tensor summaries
- support summary-first routing
- support exact fragment loading
- support graph-validated finalization
- support distributed recovery-aware routing

## Deliverables

- planner integration design
- retrieval-path semantics
- planner/tensor interaction points

## References

- `DISTRIBUTED_TENSOR_SHARDING.md`
- `EVALUATION_FRAMEWORK.md`
```

---

## Sub-Issue 3.7
### Title
`Assess hardware and network implications of distributed tensor artifact fabric`

### Body

```markdown
## Summary

Assess the hardware, storage, and network implications of distributed tensor artifacts across the Themis sharding fabric.

## Motivation

Tensor artifact placement and recovery are infrastructure-sensitive. Their cost depends on RAM, NVMe locality, network latency, and possibly GPU placement.

## Goals

- assess NVMe locality requirements
- assess RAM pressure for reconstruction
- assess network costs of fragment retrieval
- assess shard-summary traffic
- assess hot/warm/cold placement for tensor artifacts

## Deliverables

- infrastructure assessment
- deployment considerations
- hardware/network tradeoff summary

## References

- `HARDWARE_REQUIREMENTS.md`
- `DISTRIBUTED_TENSOR_SHARDING.md`
```

---

# Suggested Creation Order

## Wrapper Epics First
1. EPIC 1
2. EPIC 2
3. EPIC 3

## Then Sub-Issues
### Architecture Track
- 1.1
- 1.2
- 1.3
- 1.4
- 1.5
- 1.6
- 1.7

### Evaluation / Planner Track
- 2.1
- 2.2
- 2.3
- 2.4
- 2.5
- 2.6
- 2.7

### Distributed Tensor Fabric Track
- 3.1
- 3.2
- 3.3
- 3.4
- 3.5
- 3.6
- 3.7
