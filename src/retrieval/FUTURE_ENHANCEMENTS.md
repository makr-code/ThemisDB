---
Author: "ThemisDB Contributors"
Created: "2026-09-24"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "ThemisDB documentation orchestration assistant"
Human_Review: false
Human_Author: "ThemisDB Contributors"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_References:
  - "README.md"
  - "ROADMAP.md"
  - "API.md"
  - "PERFORMANCE_EXPECTATIONS.md"
---

Exact model backend not verifiable from final document text alone.

# Retrieval follow-up questions

This is a non-normative list of open questions, not a delivery commitment or
approved feature plan. The LoRA artifact registry now has configured RocksDB
persistence, fail-closed signature callback checks, and quality-gated
deployment entry behavior. The separate dual-read metrics helper still has a
placeholder query executor.

## Unresolved design and ownership questions

- Which application/deployment-service owner will provide and operate the
  required authorization callback? This module deliberately does not define
  owner/RBAC policy and has no current source-backed composition wiring.
- What are the owning service's public-key provisioning, rotation, revocation,
  and callback audit requirements? The retrieval module checks signer-ID pins
  and requires the callback but does not manage keys or audit decisions.
- Is a real index integration for `DualReadValidator` intended? Identify the
  authoritative query API and test owner before specifying behavior.
- Are ANN, hybrid retrieval, embedding, reranking, shard coordination, or GPU
  paths in scope for this module? No such implementation is established by
  the current source.
- What representative workload and hardware would support future performance
  acceptance? The existing benchmark is synthetic and cannot set a baseline.

No phase dates, acceptance thresholds, or production-readiness commitments are
assigned here. Those need an owner and canonical source before they can be
documented. See the [verified status](ROADMAP.md) and
[performance evidence](PERFORMANCE_EXPECTATIONS.md).
