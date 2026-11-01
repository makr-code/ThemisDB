---
Author: "ThemisDB Contributors"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

> Exact model backend not verifiable from final document text alone.

# Tensor Public Headers

This guide indexes the public declarations under `include/tensor/`. The headers
are the source of truth for signatures, ownership, failure behavior, and
thread-safety; inclusion in this directory does not imply that every feature is
production-ready. The module target requires C++17.

## Header inventory

| Area | Headers |
|---|---|
| Indexing and routing | `tensor_index.h`, `tensor_index_manager.h`, `hnsw_tt_bridge.h`, `ht_index.h`, `ht_train.h`, `hyper_index_builder.h`, `tensor_routing_strategy.h` |
| Ingestion and storage | `tensor_ingestion_bridge.h`, `tensor_core_bridge.h`, `tensor_mmap_bridge.h`, `adapter_repository.h`, `persistent_tensor_fingerprint_graph.h` |
| Fingerprints and summaries | `tensor_fingerprint_graph.h`, `tensor_summary_types.h`, `tensor_mid_layer.h` |
| Compression and operations | `compression_strategy.h`, `tensor_compression_routing_accelerator.h`, `tensor_butterfly_operator.h`, `tensor_redundancy_detection.h`, `hiss_structural_search.h` |
| Workflow and types | `dream_orchestrator.h`, `encoder_interface.h`, `tensor_api_contract.h`, `tensor_compat.h`, `tensor_edge_case_handler.h`, `tensor_error_handling.h`, `tensor_workflow_observability.h`, `tnsr_task.h`, `utr_converter.h` |

## Main runtime contracts

### Tensor indexes

`ITensorIndex` defines TT and flat-vector insert/search operations. The
`TensorIndexManager` owns indexes, provides `routeFor()` for a routing estimate,
and offers lifecycle and persistence operations. In the current implementation
`createIndex()` constructs a flat TT index without consuming the route returned
by `routeFor()`; the header's HNSW-route/null-return comment disagrees with that
implementation. See the [API contract note](../../src/tensor/API.md).

`HnswTTBridge` is a separate `ITensorIndex` implementation. Its configuration
and runtime behavior are defined in `hnsw_tt_bridge.h`; do not infer that the
manager currently selects or constructs it.

### Adapter fingerprints

`TensorFingerprintGraph::findSimilar()` ranks registered TT trains using TT
inner products. `findSimilarByFingerprint()` performs approximate cosine
scoring over stored fingerprints, supports an optional tenant filter, treats
missing dimensions as zero, and scans eligible entries; it is not an LSH lookup.
`AdapterRepository` requires a storage backend and can optionally wire a
fingerprint graph.

### Core storage and mapped views

`TensorCoreStorageBridge` implements the ingestion core-bridge contract and
stores serialized records through `ITensorStorageBackend`. Without an injected
backend or factory, it uses an in-memory backend; persistence across restarts is
not provided by that fallback.

`TensorIndexManager::mapCores()` returns an owning `TensorMmapBridge`. The
manager's current path starts from an in-memory TT train and normally copies
core data into anonymous mapped memory. File-backed `MAP_SHARED` mapping is
available through the separate FD or injected SST-mapper path. Keep the bridge
alive while using its slices. The deprecated `ggmlCorePtrs()` should not be used
for new code; use `mapCores()`.

## Installation

This guide covers in-tree development. It does not define a separate package
installation procedure for the tensor headers or library.

## Usage

Include the relevant `tensor/*.h` public header and follow its declaration
contract. The runtime examples and known API caveats are summarized in the
[implementation overview](../../src/tensor/README.md) and [API notes](../../src/tensor/API.md).

## Build and test

With an existing configured build directory:

```sh
cmake --build <build-dir> --target themis_tensor
cmake --build <build-dir> --target module_tensor_test_tensor_contract_hardening_focused
ctest --test-dir <build-dir> -L tensor --output-on-failure
```

The module target is a C++17 static library. Focused test executables are
discovered from `tests/tensor/test_*.cpp`; benchmarks are discovered from
`benchmarks/tensor/bench_*.cpp` only when `THEMIS_BUILD_BENCHMARKS` is enabled.
Registration alone does not establish that a target has built or passed.

## Related documentation

- [Implementation overview](../../src/tensor/README.md)
- [Runtime architecture](../../src/tensor/ARCHITECTURE.md)
- [API contract and source discrepancy](../../src/tensor/API.md)
- [Tensor tests](../../tests/tensor/)
- [Tensor benchmarks](../../benchmarks/tensor/)
