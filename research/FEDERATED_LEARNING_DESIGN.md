# Federated Learning Design (ThemisDB Importer Module)

**Scope:** `include/importers/federated_learning.h`, `src/importers/federated_learning.cpp`
**Status:** Implemented (code and tests present)
**Review type:** Repository-grounded technical verification

---

## Abstract

This document reviews the current federated-learning design implemented in ThemisDB's importer stack. The verified implementation provides two core capabilities: (1) aggregation of participant-level numeric statistics plus schema fragments via `FederatedAggregator::aggregateUpdates()` and (2) privacy-aware publication support via `DifferentialPrivacyManager` with Gaussian-noise injection and budget accounting. The implementation is aligned with ThemisDB's multi-model and AQL-centered architecture, but this module itself remains an importer/federation utility rather than an end-to-end training orchestrator. Evaluation in this review is evidence-based (source and test verification), not a new benchmark campaign.

---

## Introduction

### Problem statement

When importing from multiple distributed source systems, ThemisDB needs consolidated metadata/statistics without centralizing raw records. The design goal is therefore federated aggregation of participant summaries with explicit privacy controls.

### Terminology (normalized)

- **AQL:** Advanced Query Language in ThemisDB query layer (`themis::query` context in `ARCHITECTURE.md`).
- **Multi-model:** ThemisDB combines relational, graph, vector, and document capabilities in one architecture.
- **Federated aggregation (this module):** Statistical reduction over participant updates (`FedAvg` or coordinate-wise `median`) plus schema union.
- **Privacy budget:** Cumulative epsilon spending tracked by `DifferentialPrivacyManager`.

### Repository-grounded claim boundaries

This review only claims behavior that is directly visible in source code, tests, or roadmap artifacts. It does not claim full-system empirical superiority, regulatory certification, or complete distributed-learning lifecycle coverage.

---

## Methodology / Approach

### M1. Source-of-truth artifacts

- API contract: `include/importers/federated_learning.h`
- Implementation: `src/importers/federated_learning.cpp`
- Test evidence: `tests/test_postgres_importer_v2.cpp` (FederatedLearning test block)
- Planning status: `src/importers/ROADMAP.md`
- Terminology alignment: `ARCHITECTURE.md` and `src/distributed_knowledge/ARCHITECTURE.md`

### M2. Verification criteria

A statement is kept only if at least one criterion is satisfied:

1. Explicitly documented in public API comments or signatures.
2. Executable behavior in implementation code.
3. Covered by concrete test assertions.
4. Traceable to project roadmap/architecture documentation.

### M3. Verified implementation facts

1. **Aggregation algorithms:** `aggregateUpdates()` supports `"FedAvg"` and `"median"`; unknown algorithms fall back to `"FedAvg"`.
2. **Aggregated output contract:** Numeric fields are reduced; schema contributions are merged into `_schema`; participant count is emitted as `_participants`.
3. **Gaussian mechanism:** `addDifferentialPrivacy()` computes
   `sigma = sqrt(2 * ln(1.25 / delta)) / epsilon` with sensitivity fixed to 1.0, then adds normal noise to numeric JSON fields. In current code, the fixed sensitivity is an implementation simplification for normalized count/average-style statistics, not a universal bound for every possible statistic. The `1.25 / delta` term comes from the Gaussian tail-bound used to satisfy `(epsilon, delta)`-DP in the standard mechanism derivation (Ref. 4).
4. **Input validation:** `addDifferentialPrivacy()` rejects invalid `(epsilon, delta)` via `std::invalid_argument`.
5. **Budget policy:** `verifyPrivacyBudget(epsilon_total, delta)` accepts only `epsilon_total <= 1.0` and `0 < delta <= 1e-5`; `spendBudget()` accumulates epsilon and rejects negative epsilon increments (throws `std::invalid_argument`).
6. **Test coverage exists:** The `FederatedLearning` tests validate empty aggregation, FedAvg averaging, noise application, invalid epsilon rejection, budget check, and budget accumulation.

---

## Evaluation / Experiments

### E1. Code-level evaluation

This review uses code inspection rather than runtime model-quality experiments.

- **Aggregation correctness (unit level):** `FedAvgAveragesNumericFields` checks mean aggregation and participant counter behavior.
- **Privacy behavior (unit level):** `DifferentialPrivacyAddsNoise` verifies non-deterministic numeric perturbation over repeated runs.
- **Safety guards (unit level):** Invalid epsilon and negative budget tests confirm defensive exceptions.

### E2. Architecture consistency

- The importer federated-learning utilities are consistent with broader ThemisDB concepts:
  - multi-model system architecture (`ARCHITECTURE.md`)
  - AQL terminology at query layer (`ARCHITECTURE.md`)
  - distributed-knowledge documentation reusing `FederatedAggregator` and `DifferentialPrivacyManager` interfaces (`src/distributed_knowledge/ARCHITECTURE.md`)

### E3. Evidence limitations

- No fresh benchmark numbers are generated in this review.
- No external claim is made about production deployment topology beyond repository artifacts.
- Differential-privacy guarantees are mathematical/model assumptions; real privacy risk also depends on upstream query policy and operational controls.

---

## Limitations / Known Issues

1. **No robust aggregation beyond median/FedAvg:** Outlier resistance is limited to coordinate-wise median; no trimmed mean/Krum/Bulyan implementation in this module.
2. **Schema union semantics are permissive:** Conflicting field names/types across participants are merged using first-contributor-wins semantics for duplicate keys, which may require downstream normalization.
3. **Budget model is simple composition:** Current budget enforcement is a threshold check based on simple composition. Advanced accounting (for example RDP accounting as in Ref. 6) is not implemented here and would provide tighter privacy-loss bounds under composition. In practice, simple composition is more conservative and can force fewer rounds or stronger noise for the same privacy target.
4. **Importer-level scope:** This component is not a full federated-training platform by itself; it is a reusable aggregation/privacy utility used by higher-level modules.

---

## Conclusion

The current ThemisDB implementation provides a concrete and test-backed federated aggregation utility with differential-privacy mechanics in the importer domain. The strongest claims supported by repository evidence are: deterministic reduction behavior for participant statistics, explicit privacy-parameter validation, and enforced budget checks. The main practical constraints are simple budget composition and permissive schema union semantics, which should be considered when integrating this module into larger distributed-learning pipelines.

---

## References

### Scientific literature

1. McMahan, H. B., et al. (2017). *Communication-Efficient Learning of Deep Networks from Decentralized Data* (AISTATS).
   DOI: https://doi.org/10.5555/3305890.3306006
2. Kairouz, P., et al. (2021). *Advances and Open Problems in Federated Learning* (Foundations and Trends in Machine Learning).
   DOI: https://doi.org/10.1561/2200000083
3. Dwork, C., McSherry, F., Nissim, K., Smith, A. (2006). *Calibrating Noise to Sensitivity in Private Data Analysis* (TCC).
   DOI: https://doi.org/10.1007/11681878_14
4. Dwork, C., Roth, A. (2014). *The Algorithmic Foundations of Differential Privacy*.
   URL: https://www.cis.upenn.edu/~aaroth/privacybook.html
5. McMahan, H. B., et al. (2018). *Learning Differentially Private Recurrent Language Models* (ICLR Workshop).
   URL: https://arxiv.org/abs/1710.06963v2
6. Mironov, I. (2017). *Rényi Differential Privacy* (IEEE CSF).
   DOI: https://doi.org/10.1109/CSF.2017.11

### ThemisDB repository artifacts (source-of-truth, local paths by design)

7. `include/importers/federated_learning.h`
8. `src/importers/federated_learning.cpp`
9. `tests/test_postgres_importer_v2.cpp`
10. `src/importers/ROADMAP.md`
11. `ARCHITECTURE.md`
12. `src/distributed_knowledge/ARCHITECTURE.md`
