# Federated Distillation — Threat Model

**Status:** Active  
**Version:** v1.0.0  
**Date:** 2026-04-21  
**Issue:** #4743  
**Owner:** Security + ML Platform  

---

## 1. Scope

This document covers the threat model for the **Federated Distillation Framework** in ThemisDB — specifically the `FederatedDistillationCoordinator` (teacher→student soft-label transfer) and its interaction with `LoRAFederationCoordinator` (gradient aggregation).

**Out of scope:** General ThemisDB authentication, SQL injection, storage encryption.  
See [`security_threat_model.md`](../../../docs/de/security/security_threat_model.md) for the platform-wide threat model.

---

## 2. Assets

| Asset | Description | Sensitivity |
|---|---|---|
| **Soft labels** | Temperature-scaled teacher logit distributions | Medium — reveals model decision boundaries |
| **LoRA gradients** | Weight updates from local training | High — can encode training data structure |
| **Privacy budget (ε)** | Total DP budget spent per round / lifetime | High — exhaustion enables reconstruction |
| **Student adapter weights** | Post-distillation LoRA parameters | High — learned from private data |
| **Query identifiers** | Opaque hashes linking soft labels to queries | Medium — statistical linkage risk |
| **Audit log** | Broadcast history with ε accounting | Medium — reveals federation topology |

---

## 3. Trust Boundaries

```
┌─────────────────────────────────────────────────────────────┐
│  Institution A (Teacher Node)                               │
│  ┌──────────────┐                                           │
│  │ Raw data     │ ← stays here, never leaves               │
│  │ Teacher LLM  │                                           │
│  └──────┬───────┘                                           │
│         │ temperature_scale()                               │
│         ▼                                                   │
│  ┌──────────────┐                                           │
│  │ SoftLabel    │  query_id (hash), probabilities, T        │
│  └──────┬───────┘                                           │
└─────────┼───────────────────────────────────────────────────┘
          │  submitSoftLabels()           TRUST BOUNDARY A
══════════╪═══════════════════════════════════════════════════
          ▼
┌─────────────────────────────────────────────────────────────┐
│  FederatedDistillationCoordinator (Coordinator)             │
│  ┌──────────────────────────────────────────────────────┐   │
│  │ applyDPNoise()  ← Gaussian mechanism σ               │   │
│  │ checkPolicyGate()                                    │   │
│  │ broadcastToStudents()                                │   │
│  └──────────────────────────────────────────────────────┘   │
└─────────┬───────────────────────────────────────────────────┘
          │  DistillationRound (DP-protected)  TRUST BOUNDARY B
══════════╪═══════════════════════════════════════════════════
          ▼
┌─────────────────────────────────────────────────────────────┐
│  Institution B, C, … (Student Nodes)                        │
│  ┌──────────────┐                                           │
│  │ Student LLM  │ receiveSoftLabels() → distillation loss   │
│  └──────────────┘                                           │
└─────────────────────────────────────────────────────────────┘
```

---

## 4. Threat Actors

| Actor | Capability | Motivation |
|---|---|---|
| **Honest-but-curious coordinator** | Observes all broadcasts | Reconstruct training data distribution |
| **Malicious teacher (Byzantine)** | Controls soft-label content | Poison student adapters via crafted distributions |
| **Malicious student (Byzantine)** | Controls utility reports | Trigger false rollbacks, DoS the rollback mechanism |
| **External eavesdropper** | Intercepts network traffic | Reconstruct private data from soft labels |
| **Compromised student** | Reads received DistillationRounds | Membership inference on teacher training data |

---

## 5. Threat Scenarios

### T-1 — Membership Inference via Soft Labels

**Description:** An adversary who receives soft labels `p(y|x)` from teacher for query hash `q` can compare the teacher's distribution against random baseline distributions. Queries where the teacher is highly confident (near one-hot) may reveal that those queries exist in the training set.

**Attack vector:** Repeated rounds, statistical analysis of label sharpness vs. temperature.

**Mitigations:**
- Gaussian DP noise (`require_dp=true`) adds calibrated noise σ before broadcast (see §6.1)
- Temperature T≥4.0 softens distributions, reducing confidence leakage
- `query_id` is an opaque hash — the coordinator cannot link it to cleartext

**Residual risk:** Low with ε≤1.0; increases with higher ε per round.

---

### T-2 — Model Inversion via Accumulated Soft Labels

**Description:** Over many rounds, an adversary accumulates teacher soft-label distributions for many query IDs. By fitting an inversion model, they attempt to reconstruct the teacher's training data.

**Attack vector:** Many broadcast rounds before budget exhaustion; statistical ensemble of labels.

**Mitigations:**
- Privacy budget (`max_rounds * dp_epsilon`) caps total information leakage
- `verifyPrivacyBudget()` enforces hard stop at `max_rounds`
- Each round adds independent Gaussian noise, limiting signal accumulation
- Total ε budget must be set by operator to match GDPR/HIPAA requirements

**Residual risk:** Medium if `max_rounds=0` (unlimited). **Operators should set `max_rounds > 0`** for any production deployment with privacy requirements.

---

### T-3 — Byzantine Teacher (Gradient/Label Poisoning)

**Description:** A malicious teacher submits crafted soft-label distributions that, when applied by students, push student adapters toward adversarial predictions (targeted or untargeted poisoning).

**Attack vector:** Adversarially optimised probability vectors submitted via `submitSoftLabels()`.

**Mitigations:**
- `PolicyGate` callback can validate teacher identity and distribution plausibility before broadcast
- `GradientOutlierFilter` in `LoRAFederationCoordinator` detects outlier gradients (L2-norm test, FPD tests)
- Student utility reporting (`reportStudentUtility()`) detects unexpected quality degradation
- `setRollbackTrigger()` fires automatic rollback when utility drops below `min_utility_threshold`

**Residual risk:** Medium without policy gate; Low with active gate + utility monitoring.

---

### T-4 — Byzantine Student (Rollback DoS)

**Description:** A malicious student calls `reportStudentUtility()` with a low value to trigger the rollback trigger, causing repeated unnecessary rollbacks (DoS on the rollback path).

**Attack vector:** Spamming `reportStudentUtility("student-attacker", 0.0)` continuously.

**Mitigations:**
- `reportStudentUtility()` only calls the rollback trigger if `utility < min_utility_threshold`; the trigger's implementation must be rate-limited
- Rate limiting and authentication of student report submissions should be implemented at the network/API layer
- Rollback count is tracked in `DistillationModelCard` for anomaly detection

**Residual risk:** Medium at the coordinator level if no rate limiting; operator must add rate limiting at the API layer.

---

### T-5 — Privacy Budget Exhaustion Attack

**Description:** An adversary floods the coordinator with `submitSoftLabels()` calls to exhaust the privacy budget (`max_rounds * dp_epsilon`), preventing legitimate training rounds.

**Attack vector:** Rapid round submission until `verifyPrivacyBudget()` returns false.

**Mitigations:**
- `submitSoftLabels()` checks `verifyPrivacyBudget()` before accepting a submission
- Budget exhaustion is an auditable event (audit callback fires on every broadcast)
- Authentication of teacher submissions must be enforced at the API layer

**Residual risk:** Low when authentication is enforced; Medium without it.

---

### T-6 — Information Leakage via Audit Log

**Description:** The audit callback receives `{teacher_id, round, label_count, epsilon_spent}`. A compromised audit sink can infer federation topology and timing patterns.

**Attack vector:** Monitor audit log to identify when specific institutions submit.

**Mitigations:**
- Audit log should be write-only (append-only sink) with no read access for teacher/student nodes
- `teacher_id` in the audit log should use opaque identifiers, not institution names
- Audit log access is governed by `docs/governance/DOCS_PR_POLICY.md`

**Residual risk:** Low with proper access controls on the audit sink.

---

## 6. Controls Summary

### 6.1 Differential Privacy (Gaussian Mechanism)

```
σ = sensitivity · √(2 · ln(1.25 / δ)) / ε
p_noisy_i = clamp(p_i + N(0, σ²), 0, 1)  → renormalise to Σ=1
```

Recommended parameters for production:

| Scenario | ε per round | δ | max_rounds | Total ε |
|---|---|---|---|---|
| High privacy (GDPR sensitive) | 0.1 | 1e-5 | 50 | 5.0 |
| Balanced | 0.5 | 1e-5 | 100 | 50.0 |
| Research/internal | 1.0 | 1e-4 | 200 | 200.0 |

### 6.2 Policy Gate

The `PolicyGate` callback is called before every broadcast.  
**Recommended checks:**

- Verify `teacher_id` is in an allowlist
- Validate distribution entropy (uniform distributions with T≥4 are expected)
- Check cross-border transfer rules (aligned with `LoRAFederationCoordinator::setCrossBorderPolicy()`)

### 6.3 Rollback Trigger

The `setRollbackTrigger()` callback fires when `reportStudentUtility() < min_utility_threshold`.  
**Recommended threshold:** `min_utility_threshold = 0.90` (≥90% of teacher utility).

### 6.4 Outlier Detection (Gradient Path)

For the companion gradient path (`LoRAFederationCoordinator`), the `GradientOutlierFilter` (L2-norm based) filters Byzantine gradient submissions before FedAvg aggregation. This is the primary defense for T-3 on the gradient path.

---

## 7. Security Requirements

| ID | Requirement | Control |
|---|---|---|
| SEC-FDF-01 | Raw query text must never be present in `SoftLabel` | Structural: `SoftLabel` has no `query_text` field; `query_id` is opaque |
| SEC-FDF-02 | Gaussian DP noise must be applied before broadcast | `require_dp=true` (default) in `DistillationConfig` |
| SEC-FDF-03 | Privacy budget must be bounded in production | `max_rounds > 0` required for sensitive deployments |
| SEC-FDF-04 | Teacher identity must be authenticated | API-layer authentication (outside coordinator scope) |
| SEC-FDF-05 | Rollback trigger must be idempotent and rate-limited | Caller responsibility (documented in `setRollbackTrigger` doc) |
| SEC-FDF-06 | Audit log must be append-only | Audit sink implementation responsibility |
| SEC-FDF-07 | Cross-border transfers must respect policy | Wire `PolicyGate` with `CrossBorderTransferPolicy` from `LoRAFederationCoordinator` |

---

## 8. Test Coverage

| Threat | Test ID | File |
|---|---|---|
| T-1 (Membership inference — DP invariant) | FDF-05, FDF-06, FDF-11 | `tests/test_federated_distillation_coordinator.cpp` |
| T-1 (No cleartext in SoftLabel) | FDF-11 | ibid. |
| T-3 (Byzantine teacher — policy gate) | FDF-07 | ibid. |
| T-4 (Rollback DoS — utility threshold) | FDF-09, FDF-09b | ibid. |
| T-5 (Budget exhaustion) | FDF-10 | ibid. |
| T-3 (Gradient path — outlier filter) | FPD-01..10 | `tests/test_federated_poisoning_detection.cpp` |

---

## 9. Open Questions

- [ ] Rényi DP (RDP) accounting for tighter composition bounds (Enhancement F in `FUTURE_ENHANCEMENTS.md`) — planned Q4 2026
- [ ] mTLS between coordinator and student nodes — network layer, outside C++ scope
- [ ] Formal verification of DP composition across mixed gradient + distillation rounds — research track

---

## 10. References

- Hinton, Vinyals, Dean (2015). *Distilling the Knowledge in a Neural Network*. NIPS Workshop.
- Dwork, Roth (2014). *The Algorithmic Foundations of Differential Privacy*. Foundations and Trends.
- Mironov (2017). *Rényi Differential Privacy*. IEEE CSF.
- Bonawitz et al. (2017). *Practical Secure Aggregation for Privacy-Preserving Machine Learning*. ACM CCS.
- Bagdasaryan et al. (2020). *How To Backdoor Federated Learning*. AISTATS.
