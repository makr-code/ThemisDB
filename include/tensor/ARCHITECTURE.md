---
Author: "ThemisDB Contributors"
Created: "2026-06-01"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

> Exact model backend not verifiable from final document text alone.

<!-- Status: current | validated: 2026-10-31 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../src/tensor/ARCHITECTURE.md -->

# Tensor Mid-Layer Module — Public Header Architecture

**Module Path:** `include/tensor/`  
**Implementation:** `../../src/tensor/`  
**Canonical architecture doc:** [`../../src/tensor/ARCHITECTURE.md`](../../src/tensor/ARCHITECTURE.md)

---

## 1. Overview

`include/tensor/` defines the public declarations for tensor indexing, ingestion/storage bridges, fingerprints, compression and structural helpers, workflow types, and related operations. The [header inventory](./README.md) lists all current headers.

For runtime composition and implementation internals see:
→ [`../../src/tensor/ARCHITECTURE.md`](../../src/tensor/ARCHITECTURE.md)

---

## 2. Header Groups

### 2.1 Tensor Index and Management

| Header | Public Type | Purpose |
|--------|------------|---------|
| `tensor_index.h` | `ITensorIndex`, `TensorSearchResult`, `TensorIndexStats` | TT index contract and result/statistics types |
| `tensor_index_manager.h` | `TensorIndexManager` | Multi-tensor index lifecycle manager |
| `ht_index.h` | `IHierarchicalTuckerIndex`, `FlatHTIndex` | Hierarchical Tucker index contract and flat implementation |
| `ht_train.h` | `HTTrain` | Hierarchical tensor index training |
| `hyper_index_builder.h` | `HyperIndexBuilder` | Hyper-index construction pipeline |
### 2.2 Encoder and Bridges

| Header | Public Type | Purpose |
|--------|------------|---------|
| `encoder_interface.h` | `ITextEncoder`, `IImageEncoder` | Pluggable text/image encoder contracts |
| `hnsw_tt_bridge.h` | `HnswTTBridge`, `HnswTTConfig` | HNSW↔tensor-train index implementation and config |
| `tensor_core_bridge.h` | `TensorCoreStorageBridge` | Ingestion-to-storage bridge |
| `tensor_mmap_bridge.h` | `TensorMmapBridge` | Memory-mapped tensor access bridge |
| `tensor_ingestion_bridge.h` | `TensorIngestionBridge` | Ingestion→tensor pipeline bridge |
### 2.3 Operations and Fingerprinting

| Header | Public Type | Purpose |
|--------|------------|---------|
| `tensor_butterfly_operator.h` | `TensorButterflyOperator` | Butterfly-transform tensor operator |
| `tensor_fingerprint_graph.h` | `TensorFingerprintGraph` | Tensor fingerprint for graph dedup |
| `hiss_structural_search.h` | `HissStructuralSearchEngine` | HISS structural search |
| `utr_converter.h` | `UTRConverter` | Universal tensor representation converter |
### 2.4 Adapter and Tasks

| Header | Public Type | Purpose |
|--------|------------|---------|
| `adapter_repository.h` | `AdapterRepository` | Tensor adapter registry |
| `tnsr_task.h` | `TNSRTask` | Tensor computation task |

---

## 3. Namespace Layout

Public types use `themis::tensor` or a nested namespace. Check the declaration's
header for its exact namespace and signature.

---

## 4. Contract Notes

- Headers in `include/tensor/` are the public declarations; signatures and implementation details must be checked in the corresponding header/source.
- A public header's presence does not establish production maturity for every feature.
- The target propagates C++17 to consumers; build and focused-test commands are in the [public-header README](./README.md).
- For breaking-change policy see [`../../VERSIONING.md`](../../VERSIONING.md).
- Layer association: **Tensor Mid-Layer**.
