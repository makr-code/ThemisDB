# Security - Tensor Module

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

Report vulnerabilities via project-level SECURITY.md.

## Security Scope

Security in the tensor module focuses on deterministic index/bridge behavior, explicit fingerprint graph and replay-adjacent fault signaling, and bounded tensor structural runtime paths.

## Threat Model

| Threat | Current Mitigation Surface |
|---|---|
| silent tensor index routing inconsistency | explicit tensor manager/index outcomes |
| hidden graph-export or neighbour-path failures | observable fingerprint graph result surfaces |
| opaque bridge ingestion failure | explicit bridge result signaling |
| unobserved tensor replay/integrity regressions | dedicated dedup benchmark and test coverage surfaces |

## Implemented Security Controls

- tensor index and bridge operations expose explicit outcomes.
- fingerprint graph operations remain observable and diagnosable.
- replay/snapshot-adjacent flows are represented by dedicated benchmark/test paths.
- structural helper failures remain explicit and non-silent.

## Security Follow-ups

- broaden fault-injection coverage for bridge and fingerprint export edge cases.
- deepen stress coverage for concurrent tensor graph access patterns.
- tighten diagnostics taxonomy across tensor index and graph incident classes.

## Sourcecode Verification (Module: tensor/security)

- Verified files:
  - src/tensor/tensor_index_manager.cpp
  - src/tensor/tensor_core_bridge.cpp
  - src/tensor/tensor_ingestion_bridge.cpp
  - src/tensor/tensor_fingerprint_graph.cpp
  - src/tensor/hnsw_tt_bridge.cpp
  - src/tensor/tnsr_task.cpp
- Verified controls:
  - explicit index/bridge fault signaling
  - observable fingerprint graph behavior
  - diagnosable tensor structural/runtime failure surfaces