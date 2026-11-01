---
Author: "ThemisDB Contributors"
Created: "2026-06-01"
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
  - "API.md"
  - "include/lora_package.h"
  - "src/lora_package.cc"
  - "../../include/retrieval/dual_read_validator.h"
  - "dual_read_validator.cpp"
---

Exact model backend not verifiable from final document text alone.

# Retrieval module security boundaries

## Implemented code-level checks

- Production settings load through `ConfigPathResolver` from
  `config/ai_ml/retrieval.yaml`. Startup requires a non-empty RocksDB path,
  at least one valid public-key pin. The store binds its required verifier
  internally to the shared `SignedPluginRepository` Ed25519 verification
  primitive; YAML cannot provide or replace a verifier.
- Every production package/product write and import requires a matching
  content hash, non-empty Base64 Ed25519 signature, and signer ID matching an
  active key fingerprint; the signature is checked over the `manifest_hash`.
  Invalid signed records already in RocksDB fail store construction on reopen.
- The source-compatible no-argument constructor is isolated non-production
  mode. With no persistent database, keys, or verifier, it rejects every
  artifact write/import and deployment.
- Lifecycle status and update timestamps are mutable registry metadata and are
  excluded from the artifact content hash. Other manifest content remains
  covered. Do not treat status metadata as authorization.
- `deployProduct()` requires signed package/product artifacts, a READY product,
  a VALIDATED source package, valid non-empty quality metrics, and an explicit
  `DeploymentGateController` decision of `kAllow`. It then separately requires
  the configured owning-service authorization callback to approve and persists
  the DEPLOYED status. Quality is not authorization.
- Missing quality controller, absent/denying/throwing authorization callback,
  missing signatures, and write errors fail closed. Ordinary product storage
  and import reject DEPLOYED status.
- Store records are protected by a mutex; runtime callbacks are immutable after
  construction. Verification/authorization callbacks run without holding the
  store mutex, and artifact snapshots are rechecked before deployment persists.
- The dual-read query executor returns no results. Its metrics therefore do
  not validate a live index or provide a production security gate.

## Not provided by this module

The module does not implement the deployment service's owner/RBAC policy, key
provisioning/rotation, public-key storage, audit-event pipeline, artifact file
content hashing, runtime index retrieval, or live dual-read query execution.
The YAML config supplies only the database path and pinned key material. The
quality controller and separate deployment authorization callback are injected
by application composition. No source-backed application wiring currently
supplies those callbacks, so deployment remains disabled unless the owning
service explicitly provides them.

The quality controller evaluates regression only. It must never be treated as
an authorization decision. This module is not a production security
certification, and standalone artifact storage does not prove deployment
service integration.

## References

- [API reference](API.md)
- [Architecture](ARCHITECTURE.md)
- [Trust boundary notes](TRUST.md)
- [Focused tests and their limits](../../tests/retrieval/README.md)
