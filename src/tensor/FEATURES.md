---
Author: "ThemisDB Team"
Created: "2026-10-18"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Tensor in ThemisDB

## Overview

The tensor module provides TT-based indexing interfaces and implementations,
ingestion/storage bridges, adapter persistence, fingerprint similarity, and
tensor-specific helper workflows. Its interfaces are implemented across
`include/tensor/` and `src/tensor/`; storage backends and routing contracts are
owned by their respective modules.

## Implemented capabilities

- `ITensorIndex` supports TT-train and flat-vector insert/search operations.
- `TensorIndexManager` owns index lifecycle, file persistence, route estimation,
  and mapped-core access. Its current `createIndex()` implementation does not
  apply the separate `routeFor()` result; see [API.md](./API.md).
- `TensorCoreStorageBridge` stores serialized ingestion records through an
  injected backend or registered factory; without either, storage is in-memory.
- `AdapterRepository` stores TT trains and can register them with
  `TensorFingerprintGraph`. Key-based similarity uses TT inner products;
  fingerprint search is an approximate candidate scan, not an LSH index.
- `TensorWorkflowObservability` exposes metric recording, Prometheus text export,
  and SLO evaluation APIs; deployments must connect these APIs to their own
  collection/export path.

## Limits

- `routeFor()` returns an estimate; the current manager creation path does not
  use it to choose an index implementation.
- `TensorIndexManager::mapCores()` normally uses anonymous mapped memory and
  copies core data. File-backed `MAP_SHARED` requires the separate FD or
  injected-mapper path.
- Module benchmark registration and stated target thresholds are not evidence
  of a successful run. See the [benchmark notes](../../benchmarks/tensor/).

## References

- [Module README](./README.md)
- [Architecture](./ARCHITECTURE.md)
- [API contract](./API.md)
- [Implementation map](./IMPLEMENTATION.md)
- [Production requirements and evidence status](./PRODUCTION_REQUIREMENTS.md)
