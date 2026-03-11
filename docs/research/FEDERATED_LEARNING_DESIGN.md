# Federated Learning Design

**Module:** `include/importers/federated_learning.h` / `src/importers/federated_learning.cpp`
**Version:** 3.0.0
**Status:** ✅ Implemented

---

## Overview

The `FederatedImportCoordinator` enables ThemisDB to learn aggregate statistics from
multiple distributed PostgreSQL instances **without transferring raw data**.  This is
critical for privacy-sensitive environments (healthcare, finance, legal) where raw data
cannot leave its origin system.

The design is split into two complementary components:

1. **`FederatedAggregator`** – aggregates schema contributions and numeric statistics
   from multiple participants using FedAvg or coordinate-wise median.
2. **`DifferentialPrivacyManager`** – adds calibrated Gaussian noise to outgoing
   statistics so that individual records cannot be reverse-engineered from the published
   aggregate (ε-δ differential privacy).

---

## Scientific Foundations

| Paper | Venue | Relevance |
|-------|-------|-----------|
| McMahan et al. (2017) *Communication-Efficient Learning of Deep Networks from Decentralized Data* (FedAvg) | AISTATS | FedAvg aggregation algorithm |
| Kairouz et al. (2021) *Federated Learning: Challenges, Methods, and Future Directions* | JMLR | FL taxonomy and best practices |
| Dwork et al. (2006) *Calibrating Noise to Sensitivity in Private Data Analysis* | TCC | Differential privacy foundation |
| McMahan et al. (2018) *Learning Differentially Private Recurrent Language Models* | ICLR | Gaussian mechanism for DP-SGD |

---

## FedAvg Aggregation Algorithm

### Problem
Each participant holds local statistics (e.g., row counts, null rates, distinct counts).
We want a global aggregate without requiring participants to share raw records.

### Method

```
global_stat[k] = (1/N) · Σᵢ local_stat_i[k]   for each numeric key k
schema_union   = ∪ᵢ schema_contribution_i        (field-name union)
```

**Coordinate-wise median** (more robust to outliers):
```
global_stat[k] = median({ local_stat_i[k] })
```

### Implementation

```cpp
FederatedAggregator::ParticipantUpdate u;
u.participant_id = "pg-node-1";
u.statistics     = json{{"row_count", 500000.0}, {"null_rate", 0.02}};
u.schema_contribution = json{{"col_name", "integer"}};

auto global = agg.aggregateUpdates({u1, u2, u3}, "FedAvg");
// global["row_count"] == mean(500000, 480000, 520000) == 500000
```

---

## ε-δ Differential Privacy (Gaussian Mechanism)

### Problem
Even aggregate statistics can leak information about individual records.  ε-δ differential
privacy provides a mathematical guarantee that the probability of identifying any single
record from the published statistics is bounded.

### Method

**Gaussian mechanism** with L2 sensitivity = 1:

```
σ = sensitivity · √(2 · ln(1.25 / δ)) / ε

noisy_stat = true_stat + N(0, σ²)
```

**Privacy guarantee:** For any pair of neighbouring datasets D, D' differing in exactly
one record, and any output set S:

```
Pr[M(D) ∈ S] ≤ e^ε · Pr[M(D') ∈ S] + δ
```

### Privacy Budget Tracking

The total privacy budget is managed by `DifferentialPrivacyManager::spendBudget()`.
Basic composition: spending ε₁ and ε₂ on two independent queries costs ε₁ + ε₂ total.
ThemisDB enforces a "strong privacy" threshold of ε_total ≤ 1.0.

```cpp
DifferentialPrivacyManager dp;
dp.spendBudget(0.3);          // query 1
dp.spendBudget(0.2);          // query 2
assert(dp.totalEpsilonSpent() == 0.5);
assert(dp.verifyPrivacyBudget(0.5, 1e-5) == true);
```

### Parameter Guidelines

| Use Case | ε | δ | Notes |
|----------|---|---|-------|
| Research / non-sensitive | 1.0 | 1e-5 | Weaker privacy; more accurate stats |
| Healthcare (HIPAA) | 0.1 | 1e-6 | Recommended for PHI |
| Financial (GDPR) | 0.5 | 1e-5 | Balance between utility and privacy |

---

## Architecture

```
Participant A           Participant B           Participant C
  pg-node-1               pg-node-2               pg-node-3
      │                       │                       │
   Compute                 Compute                 Compute
  local stats             local stats             local stats
      │                       │                       │
   Add DP noise            Add DP noise            Add DP noise
      │                       │                       │
      └───────────────────────┼───────────────────────┘
                              │
                  FederatedAggregator (coordinator)
                              │
                    Aggregate statistics
                    Union schema contributions
                              │
                     Global model / schema
                    (NO raw data ever leaves origin)
```

---

## Security Considerations

- **No raw data is transmitted**: only `statistics` (numeric aggregates) and
  `schema_contribution` (column name→type mappings) leave each participant.
- **DP noise is additive**: each participant adds noise before sharing; even a
  compromised coordinator cannot reconstruct individual records.
- **Budget enforcement**: the coordinator maintains a cumulative ε budget; once
  exhausted, further queries are rejected.

---

## Limitations

- FedAvg assumes i.i.d. data across participants; non-i.i.d. distributions degrade accuracy.
- The Gaussian mechanism adds noise proportional to sensitivity; high-cardinality distinct
  counts require larger noise, reducing utility.
- Schema union may produce spurious fields if participants use different column naming conventions.

---

## References

- McMahan, H. B. et al. (2017). Communication-Efficient Learning of Deep Networks from Decentralized Data. *AISTATS*.
- Kairouz, P. et al. (2021). Federated Learning: Challenges, Methods, and Future Directions. *JMLR*, 22(1), 1–208.
- Dwork, C. et al. (2006). Calibrating Noise to Sensitivity in Private Data Analysis. *TCC 2006*, LNCS 3876, pp. 265–284.
- McMahan, H. B. et al. (2018). Learning Differentially Private Recurrent Language Models. *ICLR 2018*.
