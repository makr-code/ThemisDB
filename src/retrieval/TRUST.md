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

This document describes only source-level behavior. The module enforces a signed-artifact verification boundary and gates its deployment entry point, but it does not establish the owning service's authorization policy.

## Trust boundaries

### 1. Artifact boundary

`LoRAPackage` and `PortableAdapterProduct` are data structures with provenance, policy, integrity, and status fields. The presence of these fields does not enforce a trust policy.

The boundary breaks down as follows:

- records are persisted to RocksDB configured by `config/ai_ml/retrieval.yaml`;
- every production write/import and reopen requires a matching content hash,
  active pinned signer fingerprint, and valid Ed25519 signature;
- ordinary product writes/imports cannot set DEPLOYED;
- quality evaluation and external deployment authorization are both required to deploy.

### 2. Retrieval evaluation boundary

The second surface is metric computation in `DualReadValidator`. Its query executor is a placeholder, so the current API does not compare actual index outputs.

Ground truth is provided as a map by `SetGroundTruth()`; missing entries return empty data. `ValidateQuery()` computes current-result metrics, logs previous-result recall, and does not return a comparison delta. No live retrieval trust gate is implemented.

## Risk model

The main risks are:

- deployment authorization remains an external service boundary;
- confusion between manifest-hash verification and signature authenticity;
- stale or absent caller-supplied ground truth;
- interpreting empty placeholder results or synthetic tests as retrieval evidence;
- assuming the quality gate is authorization or that status metadata alone establishes trust.

## Controls in source

The current source-level controls include:

- canonical JSON serialization and SHA-256 content-hash verification;
- ConfigPathResolver/YAML-loaded Ed25519 pins and the shared
  `SignedPluginRepository` cryptographic verification primitive;
- mutex-protected package/product caches backed by persistent RocksDB;
- quality-gated deployment with an independent, fail-closed external authorization callback;
- metric helper functions, with empty metrics for empty results/ground truth.

These are limited code-level mechanisms, not a complete trust or security
system. The module does not own private signing keys, define owner/RBAC policy,
or wire authorization to a deployment service. Absent authorization wiring,
deployment fails closed.

## Incident handling

Incident handling, quarantine, rollback, and evidence retention are not
implemented by this module. The consuming system must define those procedures;
the RocksDB registry provides durable artifact records, not an audit-event
system.

## Trust posture

The source provides narrow hash and metric helpers, but does not by itself
establish a production trust posture. Do not infer safety or readiness from
the field names, test-local trust registry, or placeholder validator.

## Conclusion

See [security boundaries](SECURITY.md), [API behavior](API.md), and
[architecture](ARCHITECTURE.md) for the verified implementation limits.
