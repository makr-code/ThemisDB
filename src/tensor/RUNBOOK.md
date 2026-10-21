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

# Tensor Module Runbook

## Purpose

This runbook provides the operational playbook for the most likely tensor-side production issues: route mismatch, malformed TT-core state, invalid similarity computations, index persistence issues, and adapter graph inconsistencies.

## Symptom: invalid similarity or empty results

### Likely cause

- query or candidate vector has near-zero norm
- invalid self inner product or cross-product value
- non-finite score from the exact similarity callback
- candidate train missing from the graph

### Immediate checks

1. Inspect the emitted diagnostic and code path in `tensor_fingerprint_graph.cpp`
2. Confirm the query key is present in `trains_`
3. Confirm the candidate entry is present and has a valid `fingerprint`
4. Confirm the similarity callback returns a finite value in `[−1, 1]`

### Recovery

- clear the invalid key or reinsert the adapter
- rebuild the graph entry from a valid TT train
- re-run the query after confirming the data set is consistent

## Symptom: route mismatch or unexpected index type

### Likely cause

- route thresholds no longer align with input shape assumptions
- a workload was configured with incompatible dimension and `kappa_estimate`
- a persisted index was restored from an incompatible state

### Immediate checks

- compare the input dimension and `num_vectors` against `TensorRouter::decide()` thresholds
- inspect the route returned by `TensorIndexManager::routeFor()`
- verify the persisted data directory is not serving stale data under the wrong tenant/field

### Recovery

- drop the index and recreate it with a deterministic route for the known workload profile
- update the configuration or input shape so the route matches the intended path
- verify persisted artifacts are removed when a route change is required

## Symptom: adapter store/load mismatch

### Likely cause

- malformed TT core data
- adapter was stored under a different key or domain than expected
- repository graph and backend state diverged

### Immediate checks

- confirm the adapter key matches the expected `(tenant, domain, base_model_id)` tuple
- verify the TT core dimensions are valid before insertion
- confirm `AdapterRepository::store()` succeeded before lookup

### Recovery

- remove the stale adapter and re-store the valid one
- rebuild the fingerprint graph registration from the same valid TT train
- verify the caller is not using a legacy adapter key format

## Symptom: Mmap or pinned-state issues

### Likely cause

- bridge lifetime outlives the mapped memory usage
- data directory or mapping does not match the live tensor pages
- cleanup path left work in an inconsistent state

### Immediate checks

- check the lifetime of the owning `TensorMmapBridge`
- verify mapped slice usage is not outliving the bridge object
- inspect the bridge cleanup and munmap path for the relevant memory region

### Recovery

- retain the bridge until all mapped slices are released
- re-attach or rebuild the tensor pages before the next request cycle
- prefer a clean reinitialization when the pinned mapping is ambiguous

## Rollback and escalation

- Use per-index or per-adapter reinitialization before broader rollback for transient route or metadata issues
- Escalate when the issue is repeated across multiple tenants or collections
- If a route change causes cross-tenant or persisted state corruption, stop reads/writes for the affected collection and rebuild from a trusted source

Runbook ownership and incident escalation should stay with the module maintainer or the owner of the affected index-backed workload.
