---
Author: "ThemisDB Contributors"
Created: "2026-10-21"
Last Updated: "2026-10-21"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "unknown"
Human_Review: true
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Tensor Trust and Failure Boundaries

## Trust boundary

The tensor module is an internal trust boundary between persisted TT-trained artifacts, adapter state, and retrieval-time execution. It is not a public auth or secret-management surface on its own, but it does own runtime decisions that affect correctness and operational trust.

The relevant trust assumptions in the current implementation are:

- adapter keys and tenant scopes are meaningful boundaries for access and lifecycle
- TT-core data is assumed to be valid and well-formed before it contributes to retrieval
- similarity outputs are meaningful only when the input graph and candidate train state remain valid
- persisted data cannot be treated as trusted if the in-memory graph and index map diverge

## Risk model

The main risks in this module are not traditional auth bypasses; they are correctness risks:

- silent invalid ranking due to malformed data
- stale indexes or adapters after a drop or restore path
- non-finite similarity values reaching result sets
- cross-tenant or cross-domain key collisions
- stale mmap bridge lifetimes causing invalid pointer reads

The code explicitly guards against several of these by rejecting malformed data, emitting diagnostics, and clearing stale state when an index or adapter is dropped.

## Controls that matter in practice

### explicit validation

The module validates TT-core shape and size before registration. `TensorFingerprintGraph::addAdapter()` ensures that the core metadata and data sizes are consistent before accepting an entry.

### fail-safe query behavior

The graph code returns empty results instead of continuing with invalid math when norms or inner products are not finite or are non-positive. That is an intentional trust control: no silent result contamination.

### cleanup discipline

`TensorIndexManager::dropIndex()` and `AdapterRepository::remove()` actively remove state from both the runtime registry and the persisted store when applicable. This reduces stale trust state and limits the chance that a deleted adapter remains implicitly valid in a downstream path.

### diagnostics as evidence

The module emits diagnostics for invalid similarity scores and malformed data paths. Those signals are the primary incident evidence for tensor correctness problems and should be treated as operational trust signals.

## Incident handling

When a tensor incident occurs, the immediate trust question is whether the runtime state is coherent. The investigation should answer:

1. Was the adapter or index created under the expected key and tenant scope?
2. Did the graph store the expected TT-core and fingerprint metadata?
3. Did any malformed or non-finite values appear during scoring or persistence?
4. Was the stale state properly cleaned up after removal or restore?

If any answer is no, the affected workload should be rebuilt from a trusted source before returning to normal operations.

## Trust posture summary

The tensor module is designed around explicit state validity, deterministic route decisions, and straightforward cleanup operations. Its trust posture is conservative: invalid or ambiguous tensor state is treated as operationally untrustworthy rather than silently tolerated.
