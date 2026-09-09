# MODULE_INTEGRATION_CONTRACTS

## Scope

This document captures cross-module integration contracts that are explicitly present in the repository's public interfaces and core wiring.

## Abstract Interface Contracts

| Contract | Purpose | Evidence |
|---|---|---|
| `IStorageEngine` | Canonical storage abstraction (`open/get/put/del/scan*`) used across query/index/transaction paths. | `include/themis/base/interfaces/storage_interface.h:37-166` |
| `IQueryEngine` | Canonical query abstraction (`execute/validate/explain`) for protocol-facing layers. | `include/themis/base/interfaces/query_interface.h:73-136` |
| `IIndexManager` | Unified index manager contract for secondary/vector/graph index lifecycle. | `include/themis/base/interfaces/index_interface.h:259-326` |
| `ILLMPlugin` | Pluggable LLM backend contract (`loadModel`, inference/LoRA management). | `include/llm/llm_plugin_interface.h:333-414` |
| `ILLMWikiPlugin` | Enterprise-gated LLM Wiki plugin contract with ingestion/query lifecycle. | `include/llm_wiki/llm_wiki_plugin_interface.h:233-337` |
| `DistributedTransactionManager` contract | 2PC coordinator abstraction for distributed transaction control. | `include/transaction/distributed_transaction_manager.h:16-33` |
| `TwoPhaseCommitCoordinator` contract | Sharding-side 2PC coordinator participant orchestration. | `include/sharding/two_phase_commit_coordinator.h:16-31` |

## Stability and Versioning Signals

Version/maturity metadata (auto-generated Doxygen headers) provide explicit contract maturity signals:

| Interface/File | Version | Maturity | Score | Evidence |
|---|---:|---|---:|---|
| `storage_interface.h` | `0.0.47` | PRODUCTION-READY | 86/100 | `include/themis/base/interfaces/storage_interface.h:1-10` |
| `query_interface.h` | `0.0.47` | PRODUCTION-READY | 86/100 | `include/themis/base/interfaces/query_interface.h:1-10` |
| `index_interface.h` | `0.0.47` | PRODUCTION-READY | 86/100 | `include/themis/base/interfaces/index_interface.h:1-10` |
| `distributed_transaction_manager.h` | `0.0.12` | PRODUCTION-READY | 86/100 | `include/transaction/distributed_transaction_manager.h:1-10` |
| `two_phase_commit_coordinator.h` | `0.0.34` | PRODUCTION-READY | 86/100 | `include/sharding/two_phase_commit_coordinator.h:1-10` |
| `llm_plugin_interface.h` | `1.9.0-beta` | PRODUCTION-READY | 100/100 | `include/llm/llm_plugin_interface.h:3-11` |
| `http_server.h` | `0.0.47` | PRODUCTION-READY | 86/100 | `include/server/http_server.h:1-9` |

## Edition System Gating (MINIMAL/COMMUNITY/ENTERPRISE/HYPERSCALER/MILITARY)

Repository-wide edition gate contracts are explicit:

- Edition types and resource ceilings are defined in `include/themis/edition.h:21-25`, `include/themis/edition.h:57-61`, `include/themis/edition.h:88-105`.
- Runtime feature checks are implemented via EditionManager (`src/themis/edition_manager.cpp:44-69`).
- LLM Wiki plugin contract explicitly states enterprise/hyperscaler/military gating (`include/llm_wiki/llm_wiki_plugin_interface.h:239-243`).
- Plugin boundary enforces private-in-community failure code (`include/plugins/plugin_interface.h:618`).

## API Maturity Scores (cross-module integration view)

The following maturity snapshot is source-backed and can be used for integration risk triage:

| Domain | Primary contract file(s) | Score signal | Integration risk note |
|---|---|---:|---|
| Storage | `storage_interface.h` | 86 | Stable core abstraction; broad downstream dependency footprint. |
| Query | `query_interface.h` | 86 | Stable query abstraction; coupled to storage/index capabilities. |
| Indexing | `index_interface.h` | 86 | Unified index manager API; cycle pressure with query/storage modules. |
| Distributed Txn | `distributed_transaction_manager.h`, `two_phase_commit_coordinator.h` | 86 / 86 | Stable 2PC contracts; CI/hardware evidence still release-gated. |
| LLM plugin | `llm_plugin_interface.h` | 100 | Mature interface contract; server coupling remains architectural hotspot. |
| Server ingress | `http_server.h` | 86 | Contract mature, but aggregation breadth creates compile/runtime coupling pressure. |
