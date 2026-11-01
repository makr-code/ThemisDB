---
Author: "ThemisDB Contributors"
Created: "2026-07-18"
Last Updated: "2026-11-01"
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

This page is not evidence that the retrieval module is production-ready. The
artifact registry now has an explicit runtime config and RocksDB persistence,
but application authorization wiring, key lifecycle, and production acceptance
remain external or unestablished.

## What the code provides

- `themis_retrieval` is a C++20 static library built from the artifact store,
  configuration loader, and quality gate source,
  controlled by the default-ON build option `THEMIS_BUILD_RETRIEVAL`.
- `LoRAManifestStore::openFromThemisConfig()` loads `config/ai_ml/retrieval.yaml` via `ConfigPathResolver`/yaml-cpp and requires a database path plus at least one valid pinned key before opening RocksDB.
- Every production artifact write/import and persisted record reload requires a matching manifest hash and a valid Ed25519 signature over that hash using the shared pinned-key verifier. The no-argument compatibility constructor rejects all writes.
- `deployProduct()` applies quality-only gating and requires separate evidence,
  authorization, and persisted-lifecycle verification callbacks. These are
  application integration boundaries, not local access-control policy.
- `DualReadValidator::ExecuteQuery()` returns an empty vector. The validator
  does not connect to indexes or provide production dual-read execution.
- The runtime config at `config/ai_ml/retrieval.yaml` supplies the RocksDB path
  and Ed25519 public-key pins. It does not configure application callbacks;
  flags such as `THEMIS_PRODUCTION_MODE` and `THEMIS_ENVIRONMENT` are not
  configuration surfaces of this target.

## Not established here

This module does not supply owner/RBAC policy, public-key provisioning or
rotation, source-backed authorization callback wiring, operator audit events,
resource limits, shard coordination, ANN routing, GPU execution, or production
performance acceptance. Do not treat the
historical Phase A-D claims or the synthetic benchmark as implementation or
approval evidence.

## Follow-up

The owning deployment service must be identified and wired to supply the
authorization callback and key-verification composition. This page does not
guess the service's policy. See [module architecture](ARCHITECTURE.md),
[security boundaries](SECURITY.md), and [roadmap status](ROADMAP.md).
