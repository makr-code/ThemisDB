> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../src/tensor/ARCHITECTURE.md -->

# Tensor Mid-Layer Module — Public Header Architecture

**Module Path:** `include/tensor/`  
**Implementation:** `../../src/tensor/`  
**Canonical architecture doc:** [`../../src/tensor/ARCHITECTURE.md`](../../src/tensor/ARCHITECTURE.md)

---

## 1. Overview

`include/tensor/` defines the **public tensor index management, encoder interfaces, HNSW tensor bridges, hyper-index construction, butterfly operators, tensor fingerprints, mmap bridges, and TT decomposition API contract** for ThemisDB.

For runtime composition and implementation internals see:
→ [`../../src/tensor/ARCHITECTURE.md`](../../src/tensor/ARCHITECTURE.md)

---

## 2. Header Groups

### 2.1 Tensor Index and Management

| Header | Public Type | Purpose |
|--------|------------|---------|
| `tensor_index.h` | `TensorIndex` | Primary tensor index entry point |
| `tensor_index_manager.h` | `TensorIndexManager` | Multi-tensor index lifecycle manager |
| `ht_index.h` | `HTIndex` | Hierarchical tensor index |
| `ht_train.h` | `HTTrain` | Hierarchical tensor index training |
| `hyper_index_builder.h` | `HyperIndexBuilder` | Hyper-index construction pipeline |
### 2.2 Encoder and Bridges

| Header | Public Type | Purpose |
|--------|------------|---------|
| `encoder_interface.h` | `IEncoderInterface` | Pluggable encoder contract |
| `hnsw_tt_bridge.h` | `HNSWTTBridge` | HNSW↔tensor-train bridge |
| `tensor_core_bridge.h` | `TensorCoreBridge` | Tensor-core operation bridge |
| `tensor_mmap_bridge.h` | `TensorMmapBridge` | Memory-mapped tensor access bridge |
| `tensor_ingestion_bridge.h` | `TensorIngestionBridge` | Ingestion→tensor pipeline bridge |
### 2.3 Operations and Fingerprinting

| Header | Public Type | Purpose |
|--------|------------|---------|
| `tensor_butterfly_operator.h` | `TensorButterflyOperator` | Butterfly-transform tensor operator |
| `tensor_fingerprint_graph.h` | `TensorFingerprintGraph` | Tensor fingerprint for graph dedup |
| `hiss_structural_search.h` | `HISSStructuralSearch` | HISS structural similarity search |
| `utr_converter.h` | `UTRConverter` | Universal tensor representation converter |
### 2.4 Adapter and Tasks

| Header | Public Type | Purpose |
|--------|------------|---------|
| `adapter_repository.h` | `AdapterRepository` | Tensor adapter registry |
| `tnsr_task.h` | `TnsrTask` | Async tensor computation task |

---

## 3. Namespace Layout

All public types reside in the `themis::tensor` namespace (or a sub-namespace).

---

## 4. Contract Notes

- Headers in `include/tensor/` expose the **stable public API**; internal types live in `src/tensor/`.
- Clients depend only on types declared here; implementation details in `src/` may change without notice.
- For breaking-change policy see [`../../VERSIONING.md`](../../VERSIONING.md).
- Layer association: **Tensor Mid-Layer**.
