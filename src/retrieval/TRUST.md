---
Author: "ThemisDB Contributors"
Created: "2026-10-21"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "ThemisDB documentation orchestration assistant"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Retrieval Trust and Security Boundary

## Purpose

This document describes only source-level behavior. The module does not establish a deployment trust policy or govern lifecycle transitions.

## Trust boundaries

### 1. Artifact boundary

`LoRAPackage` and `PortableAdapterProduct` are data structures with provenance, policy, integrity, and status fields. The presence of these fields does not enforce a trust policy.

The boundary breaks down as follows:

- records are inserted into an in-memory manifest store;
- manifest SHA-256 can be recomputed and compared by explicit verification calls;
- status values are serialized metadata; the store does not enforce transitions;
- deployment authorization and safe-use decisions are outside this module.

### 2. Retrieval evaluation boundary

The second surface is metric computation in `DualReadValidator`. Its query executor is a placeholder, so the current API does not compare actual index outputs.

Ground truth is provided as a map by `SetGroundTruth()`; missing entries return empty data. `ValidateQuery()` computes current-result metrics, logs previous-result recall, and does not return a comparison delta. No live retrieval trust gate is implemented.

## Risk model

The main risks are:

- records accepted by CRUD operations without semantic validation;
- confusion between manifest-hash verification and signature authenticity;
- stale or absent caller-supplied ground truth;
- interpreting empty placeholder results or synthetic tests as retrieval evidence;
- assuming artifact status fields enforce deployment policy.

## Controls in source

The current source-level controls include:

- canonical JSON serialization and SHA-256 manifest-hash verification;
- an optional verifier callback, invoked only when it is configured and a
  signature is present;
- mutex-protected, process-local package/product maps;
- metric helper functions, with empty metrics for empty results/ground truth.

These are limited code-level mechanisms, not a complete trust or security
system. In particular, unsigned artifacts can pass hash verification, and
neither storage persistence nor lifecycle/deployment enforcement is present.

## Incident handling

Incident handling, quarantine, rollback, and evidence retention are not
implemented by this module. The consuming system must define those procedures;
the module's in-memory store is not durable incident evidence.

## Trust posture

The source provides narrow hash and metric helpers, but does not by itself
establish a production trust posture. Do not infer safety or readiness from
the field names, test-local trust registry, or placeholder validator.

## Conclusion

See [security boundaries](SECURITY.md), [API behavior](API.md), and
[architecture](ARCHITECTURE.md) for the verified implementation limits.
