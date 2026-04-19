# Governance Module — Architecture Guide

**Version:** 1.0
**Last Updated:** 2026-04-06
**Module Path:** `src/governance/`

---

## 1. Overview

The Governance module implements ThemisDB's policy-based data governance layer. It evaluates
access control policies, enforces data retention rules, classifies and labels data for
compliance purposes, and generates compliance reports (GDPR, HIPAA, NIST 800-53).

The module sits between the server/API layer (which authenticates principals) and the
storage layer (which persists data), acting as the policy enforcement point (PEP) in a
Policy-Based Access Control (PBAC) architecture.

---

## 2. Design Principles

- **Policy as Code** – policies are defined in structured YAML/JSON files that are
  version-controlled and reviewable, not hardcoded.
- **Versioned Policies** – `policy_manager_versioned.cpp` tracks policy history with
  timestamps, enabling rollback and compliance audit of policy changes.
- **Hot-Reload** – `policy_file_watcher.cpp` monitors policy files with inotify/FSEvents,
  enabling live updates without server restart.
- **Separation of Enforcement** – the governance module evaluates policies; it does not
  encrypt data (security module) or authenticate users (auth module).
- **Audit Integration** – all policy decisions (PERMIT, DENY, N/A) are logged to the
  audit trail via the utils module.

---

## 3. Component Architecture

### 3.1 Key Components

| File | Role |
|---|---|
| `policy_engine.cpp` | Core policy evaluation: evaluate(principal, action, resource) → PERMIT/DENY |
| `policy_manager.cpp` | Policy lifecycle: load, validate, activate, deactivate |
| `policy_manager_versioned.cpp` | Versioned policy management with history tracking |
| `policy_coordinator.cpp` | Coordinates policy evaluation across distributed nodes |
| `policy_validator.cpp` | Syntactic and semantic policy validation |
| `policy_validation.cpp` | Policy rule conflict detection (contradictory, overlapping, circular) |
| `policy_template.cpp` | Built-in policy templates (GDPR, HIPAA, SOC 2, least-privilege, time-based) |
| `policy_version_history.cpp` | Policy change history and rollback |
| `policy_file_watcher.cpp` | inotify/FSEvents-based hot-reload of policy files |
| `policy_review.cpp` | Policy review workflow: draft → review → approve → activate |
| `review_scheduler.cpp` | Scheduled policy review reminders |
| `compliance_reporter.cpp` | GDPR/HIPAA/CCPA/PCI-DSS/SOC 2 compliance reports |
| `compliance_reporting.cpp` | Report generation engine (PDF, JSON, HTML) |
| `soc2_controls.cpp` | SOC 2 Trust Services Criteria controls and evidence collection (CC6.1, CC7.2, CC8.1, A1.1, C1.1, PI1.2) |
| `ccpa_rules.cpp` | CCPA/CPRA data subject rights rule evaluators (RightToKnow, RightToDelete, OptOutOfSale, DataPortability) |
| `pci_dss_rules.cpp` | PCI-DSS data isolation and compliance rules |
| `data_masker.cpp` | Field-level data masking (REDACT, TOKENIZE, TRUNCATE, HASH strategies) |
| `data_lineage.cpp` | Data lineage tracking for governed datasets |
| `cross_tenant_policy_inheritance.cpp` | Cross-tenant hierarchical policy composition (most-restrictive-wins) |
| `model_governance.cpp` | AI/ML model training governance, bias auditing, training data lineage |
| `opa_adapter.cpp` | Open Policy Agent integration for Rego-based policy evaluation |

### 3.2 Component Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│              Request Handler (src/server/)                       │
│   evaluate(principal, "READ", "collection/users")               │
└──────────────────────────┬──────────────────────────────────────┘
                           │
┌──────────────────────────▼──────────────────────────────────────┐
│                     PolicyEngine                                 │
│                                                                  │
│  1. Load applicable policies (from PolicyManager cache)          │
│  2. Evaluate each rule: subject + action + resource              │
│  3. Combine decisions (deny-overrides / first-applicable)        │
│  4. Return PolicyDecision { PERMIT | DENY | NOT_APPLICABLE }     │
└────────┬──────────────────────────────────────────────┬─────────┘
         │                                              │
┌────────▼────────────────────┐        ┌───────────────▼──────────┐
│   PolicyManager (versioned) │        │   AuditLogger (utils)    │
│   policy cache with TTL     │        │   log: principal,        │
│   hot-reload via watcher    │        │   action, resource,      │
│   version history           │        │   decision, timestamp    │
└─────────────────────────────┘        └──────────────────────────┘
```

---

## 4. Data Flow

### 4.1 Policy Evaluation

```
RequestContext { principal, action, resource, tenant }
    │
    ▼
PolicyEngine::evaluate(ctx)
    │
    ├─ load policies for tenant (from cache or disk)
    ├─ for each applicable policy rule:
    │       evaluate condition (subject match, action match, resource match)
    │       → collect decisions
    ├─ combine: deny-overrides OR first-applicable (configurable)
    └─ return PolicyDecision + matched_rule_ids
    │
    ▼
AuditLogger: log decision + context
```

### 4.2 Policy Hot-Reload

```
PolicyFileWatcher: inotify event on policy directory
    │
    ▼
PolicyValidator: validate new policy file (syntax + semantics)
    ├─ invalid → log error; keep existing policy; alert operator
    └─ valid →
           PolicyManager: load new version → increment version counter
           → cache invalidation → next evaluations use new policy
```

### 4.3 Compliance Reporting

```
ComplianceReporter::generateReport(standard: "GDPR", period: "2026-Q1")
    │
    ▼
Collect: data classification labels, access logs, retention events
    │
    ▼
Map against GDPR articles / HIPAA safeguards
    │
    ▼
Generate report: PDF / JSON / HTML
```

---

## 5. Integration Points

| Direction | Module | Interface |
|---|---|---|
| **Called by** | `src/server/` | Per-request policy evaluation |
| **Called by** | `src/scheduler/` | Automated retention policy execution |
| **Uses** | `src/utils/` | Audit logging for policy decisions |
| **Uses** | `src/auth/` | Receives authenticated principal info |
| **Provides to** | Compliance consumers | Reports and dashboards |

---

## 6. Threading & Concurrency Model

- `PolicyEngine::evaluate()` is stateless and safe for concurrent invocation.
- `PolicyManager` uses a read-write lock: policy evaluation holds a shared lock; policy
  reload holds an exclusive lock.
- `PolicyFileWatcher` runs on a dedicated background thread.
- `ComplianceReporter` runs as a batch job; not performance-critical.

---

## 7. Performance Architecture

| Technique | Detail |
|---|---|
| Policy cache | Compiled policy rules cached in memory with TTL |
| Rule indexing | Policies indexed by (tenant, action) for O(1) lookup of applicable rules |
| Lazy reporting | Compliance reports generated asynchronously, not on the request path |

---

## 8. Security Considerations

- Policy files are loaded from a trusted directory; file integrity is validated at load.
- Policy reviews require explicit approval before activation (`policy_review.cpp`).
- Compliance reports contain sensitive access metadata; access is restricted to admin role.
- The `policy_engine.cpp` defaults to **DENY** when no applicable policy is found (fail closed).

---

## 9. Configuration

| Parameter | Default | Description |
|---|---|---|
| `governance.policy_dir` | "config/policies/" | Policy file directory |
| `governance.default_decision` | "deny" | Decision when no policy matches |
| `governance.combination_algorithm` | "deny-overrides" | Policy combination: deny-overrides / first-applicable |
| `governance.hot_reload.enabled` | true | Enable inotify-based hot-reload |
| `governance.audit.enabled` | true | Log all policy decisions |

---

## 10. Error Handling

| Error Type | Strategy |
|---|---|
| Policy load failure | Keep existing policy; log error; alert operator |
| Policy validation failure | Reject new policy; keep existing; log rejection reason |
| Evaluation error | Default to DENY; log structured error |
| Compliance report failure | Return partial report with error annotations |

---

## 11. Known Limitations & Future Work

- ABAC (Attribute-Based Access Control) expressions in policy rules are partially supported; complex expression trees are not yet evaluated.
- Distributed policy coordination (`policy_coordinator.cpp`) is experimental; consistency guarantees in multi-node deployments are not yet validated.
- Cross-border data transfer control (ISO 3166-based region enforcement) is planned as a future header interface (`IDataTransferPolicy`).
- OPA (Open Policy Agent) integration is implemented (`opa_adapter.cpp`); Wasm-embedded bundle evaluation (offline mode) is not yet supported.
- GDPR data subject request handling (erasure, portability) is implemented for CCPA/CPRA rights via `ccpa_rules.cpp`; full GDPR Article 17/20 automated workflows are planned.

---

## 12. References

- `src/governance/README.md` — module overview
- `docs/TENANT_ISOLATION_GUIDE.md` — tenant isolation guide
- `docs/compliance/` — compliance documentation
- `ARCHITECTURE.md` (root) — full system architecture
