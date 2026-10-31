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

- The artifact library computes manifest SHA-256 values and compares a stored
  manifest hash with a recomputed canonical representation.
- `LoRAManifestStore` protects its in-memory package/product maps and verifier
  callback with a mutex.
- A signature-verifier callback is optional. It is called only when configured
  and when the artifact has a non-empty signature. A matching manifest hash
  can pass verification when either is absent; the code does not establish a
  mandatory-signature policy.
- `from_json()` checks selected required fields but treats many other fields
  as optional. `storePackage()` and `storeProduct()` check for a non-empty ID,
  not a deployment authorization or lifecycle transition.
- The dual-read query executor returns no results. Its metrics therefore do
  not validate a live index or provide a production security gate.

## Not provided by this module

The checked-in target provides no persistent or access-controlled store, key
management, signer trust registry, deployment authorization, lifecycle-policy
enforcement, audit event system, runtime configuration, or production
retrieval path. Tests that use a local signer registry or hybrid retrieval
mock do not implement these controls in the library.

Do not treat this page as a production security certification. The consuming
application must determine and document its own security policy and integration
controls; those requirements are not defined by the present module code.

## References

- [API reference](API.md)
- [Architecture](ARCHITECTURE.md)
- [Trust boundary notes](TRUST.md)
- [Focused tests and their limits](../../tests/retrieval/README.md)
