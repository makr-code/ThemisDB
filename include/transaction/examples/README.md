---
Author: "ThemisDB Contributors"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

Source level: 1; SOT domain: api-contract. Canonical references: `../transaction_semantic_advisor.h`, `../deadlock_predictor.h`, `transaction_semantic_advisor_example.cpp`, `../../../cmake/ModularBuild.cmake`, and `../../../cmake/CMakeLists.txt`. Related issue: #7390.

# Transaction Module — Examples

This directory contains a `TransactionSemanticAdvisor` example sketch. The current header declares `TransactionSemanticAdvisor::analyzeBatch()` and `suggestDeferral()` over `TransactionContext`; the example's `TransactionBatch`, `BatchAffinityHint`, predictor scoring, GDPR guard, `DecisionRecord`, and `TransactionBatcher::applyAffinityHint()` calls are not current APIs.

## Contents

| File | Paper | Issue | Status |
|------|-------|-------|--------|
| `transaction_semantic_advisor_example.cpp` | Paper 2 §Layer 5 | IMPL-B5 | Non-buildable design sketch; calls are commented or simulated |

## transaction_semantic_advisor_example.cpp

Demonstrates:

1. **Conflict detection** — identifying overlapping write-sets across a `TransactionBatch`
2. **DeadlockPredictor** integration — existing predictor score used as a prior for the advisor
3. **BatchAffinityHint** (IMPL-B5) — ordering hint with `retry_reduction_estimate ≥ 15 %`
4. **GDPR guard** — `evidence_snippet` must not contain tagged field values
5. **DecisionRecord** written to `AIDecisionAuditor` for every non-trivial hint
6. **Latency invariant** — hint computation ≤ 2 ms p99

The source file itself states that the shown advisor calls are planned; this status is stale for the current `TransactionSemanticAdvisor` type. The current class does not declare the methods/types or performance/privacy guarantees described in the sketch. Treat the sketch's estimates and safeguards as **TODO/assumptions**, not implementation claims.

## Related Documentation

- Issue spec: `docs/issues/optimization_layers/IMPL-B5-transaction-semantics.md`
- Research paper: `docs/en/research/LLM_OPTIMIZATION_LAYERS_MATRIX.md` §Layer 5
- Module ROADMAP: `include/transaction/ROADMAP.md` §Phase 7

## Installation

This module is included as part of ThemisDB. The inspected build definitions do not establish an exported `THEMISDB_INCLUDE_DIR` variable or installed transaction package target. **TODO:** document installed-consumer instructions from the canonical packaging/export configuration.

## Usage

Include the relevant headers from this module:

```cpp
#include "transaction/transaction_semantic_advisor.h"
```

See [`ARCHITECTURE.md`](../ARCHITECTURE.md) and [`ROADMAP.md`](../ROADMAP.md) for details.
