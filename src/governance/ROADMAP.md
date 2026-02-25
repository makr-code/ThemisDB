# Governance Module Roadmap
<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

## Current Status
**Beta** — Policy-based data access control, GDPR/HIPAA compliance rule evaluation, automated data retention, data classification, and CCPA/CPRA data subject rights are functional. OPA integration is planned.

## Completed ✅
- [x] Policy engine for data access control
- [x] Compliance rule evaluation (GDPR, HIPAA, and other regulations)
- [x] Automated data retention policy enforcement
- [x] Data classification and labeling
- [x] Audit trail integration for governance events
- [x] Policy-based governance enforcement at query time
- [x] CCPA/CPRA data subject rights enforcement (right-to-know, right-to-delete, opt-out-of-sale, data portability)

## In Progress 🚧
- [I] Dynamic policy hot-reload without restart (Target: Q2 2026) (Issue: #1759)
- [I] Policy conflict detection and resolution reporting (Target: Q2 2026) (Issue: #1760)
- [~] CCPA compliance rule set (Target: Q3 2026) (Issue: #1761)

## Planned Features 📋

### Short-term (Next 3-6 months)
- [x] Policy hot-reload on configuration change (Issue: #1762)
- [x] Conflict detection for overlapping access control policies (Issue: #1763)
- [P] CCPA / CPRA data subject rights enforcement (Issue: #1764)
- [x] Data lineage tracking for governed datasets (Issue: #1765)
- [P] Policy simulation / dry-run mode to preview access decisions (Issue: #1766)
- [x] Compliance report generation (PDF / JSON summary) (Issue: #1767)

### Long-term (6-12 months)
- [I] OPA (Open Policy Agent) integration for policy-as-code (Issue: #1768)
- [I] SOC 2 compliance controls and evidence collection (Issue: #1769)
- [I] PCI-DSS data isolation rules (Issue: #1770)
- [x] AI/ML model governance (training data lineage, bias auditing) (Issue: #1771)
- [I] Cross-tenant governance policy inheritance (Issue: #1772)
- [I] Automated data masking for sensitive fields in query results (Issue: #1773)

## Implementation Phases

### Phase 1: Policy Engine and Compliance Rules (Status: Completed)
- [x] Implemented policy engine for attribute-based data access control (`governance/policy_engine.cpp`)
- [x] Implemented GDPR and HIPAA compliance rule evaluation at query time
- [x] Implemented automated data retention policy enforcement with configurable TTLs
- [x] Implemented data classification and labeling for PII, PHI, and confidential fields
- [x] Integrated audit trail recording all governance enforcement events

### Phase 2: Policy Versioning and Reporting (Status: In Progress)
- [I] Implement policy versioning with rollback support (`governance/policy_manager_versioned.cpp`) (Issue: #1780)
- [x] Implement compliance report generation summarizing rule evaluations per time window (Issue: #1781)
- [x] Implement policy conflict detection for overlapping access control rules (Issue: #1782)

### Phase 3: Hot-Reload, CCPA, and OPA Integration (Status: In Progress)
- [x] Implement policy hot-reload on config file change without service restart (Issue: #1774)
- [P] Implement CCPA/CPRA data subject rights enforcement (right-to-delete, right-to-know) (Issue: #1775)
- [I] Implement automated data masking for configured sensitive fields in query results (Issue: #1776)
- [I] Integrate Open Policy Agent (OPA) as an alternative policy evaluation engine (Issue: #1777)

## Production Readiness Checklist
- [I] Unit tests coverage > 80% (Issue: #1778)
- [x] Integration tests (policy evaluation, retention enforcement, audit trail)
- [I] Performance benchmarks (policy evaluation latency at query time) (Issue: #1779)
- [x] Security audit (policy bypass prevention, audit trail integrity)
- [x] Documentation complete (policy engine, compliance governance, integration guide)
- [x] API stability guaranteed for policy engine and rule evaluation

## Known Issues & Limitations
- CCPA rule set is implemented in `src/governance/ccpa_rules.cpp` (CcpaRuleSet)
- OPA integration is planned but not started
- Automated data masking in query results is not yet implemented
- Data lineage tracking is implemented (`governance/data_lineage.cpp`; `DataLineageTracker`)
- AI/ML model governance is implemented (`governance/model_governance.cpp`; `ModelGovernancePolicy`)

## Breaking Changes
- OPA integration will introduce a new policy language alongside existing rule format (additive)
- Policy simulation API will be a new endpoint (non-breaking)
