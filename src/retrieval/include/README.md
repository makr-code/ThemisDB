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
---

Exact model backend not verifiable from final document text alone.

# `src/retrieval/include/` headers

This directory contains `lora_package.h`, which declares `LoRAPackage`,
`PortableAdapterProduct`, `LoRAManifestStore`, and `IntegrityHelper`. Its
implementation is `src/lora_package.cc`, compiled into the `themis_retrieval`
static library. The library exposes this directory as a public include path.

The dual-read validator is declared separately in
[`../../../include/retrieval/dual_read_validator.h`](../../../include/retrieval/dual_read_validator.h)
and implemented by `../dual_read_validator.cpp`; it is not part of the
`themis_retrieval` source list. The public
[`retrieval.h`](../../../include/retrieval/retrieval.h) is currently an empty
umbrella placeholder, not a declaration of a retrieval engine.

No ANN front door, tensor mid-layer, graph validator, model-switch, federated
summary, or retrieval-observability headers are present in this directory.
`LoRAManifestStore(RuntimeConfig)` persists artifacts through RocksDB. Its
source-compatible default constructor is test-only and rejects artifact writes;
production uses the YAML/config-resolver path and requires signatures. Deployment
remains subject to separate quality and authorization checks. See the
[module API reference](../API.md) and
[architecture](../ARCHITECTURE.md) for current behavior.
