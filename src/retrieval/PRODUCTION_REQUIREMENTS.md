---
Author: "ThemisDB Contributors"
Created: "2026-07-18"
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
  - "SECURITY.md"
  - "CMakeLists.txt"
  - "include/lora_package.h"
  - "src/lora_package.cc"
---

Exact model backend not verifiable from final document text alone.

# Retrieval production requirements: implementation boundary

This page is not evidence that the retrieval module is production-ready, and
the requirements and approval gates previously described here were not
implemented by the checked-in retrieval sources. No production deployment
policy or runtime configuration contract can be established from this module.

## What the code provides

- `themis_retrieval` is a C++17 static library built from `src/lora_package.cc`,
  controlled by the default-ON build option `THEMIS_BUILD_RETRIEVAL`.
- `LoRAManifestStore` offers mutex-protected, process-local package/product
  maps, CRUD/list operations, and JSON import/export. It has no persistent
  backend, bounded cache, audit log, or lifecycle/deployment-policy engine.
- Manifest verification checks that a stored manifest hash is non-empty and
  matches the canonicalized data. An optional signature callback is called
  only if both a callback and a non-empty signature are present. No callback
  or signature is required by the implementation.
- `DualReadValidator::ExecuteQuery()` returns an empty vector. The validator
  does not connect to indexes or provide production dual-read execution.
- No module-specific runtime configuration exists beyond
  `THEMIS_BUILD_RETRIEVAL`; flags such as `THEMIS_PRODUCTION_MODE` and
  `THEMIS_ENVIRONMENT` are not configuration surfaces of this target.

## Not established here

This module does not supply or verify persistent storage, signer/key
management, access control, lifecycle transitions, deployment authorization,
operator audit events, resource limits, shard coordination, ANN routing,
GPU execution, or production performance acceptance. Do not treat the
historical Phase A-D claims or the synthetic benchmark as implementation or
approval evidence.

## Follow-up

Before a production requirements contract can be written, the responsible
owners must identify the intended runtime integration and authoritative
security/deployment policies. Those requirements are unresolved by this
source review; this page does not guess them. See [module architecture](ARCHITECTURE.md),
[security boundaries](SECURITY.md), and [roadmap status](ROADMAP.md).
