# Governance Module Roadmap
<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

## Current Status
**Beta** — Policy-based data access control, GDPR/HIPAA compliance rule evaluation, automated data retention, and data classification are functional. CCPA compliance and OPA integration are planned.

## Completed ✅
- [x] Policy engine for data access control
- [x] Compliance rule evaluation (GDPR, HIPAA, and other regulations)
- [x] Automated data retention policy enforcement
- [x] Data classification and labeling
- [x] Audit trail integration for governance events
- [x] Policy-based governance enforcement at query time

## In Progress 🚧
- [ ] Dynamic policy hot-reload without restart (Target: Q2 2026)
- [ ] Policy conflict detection and resolution reporting (Target: Q2 2026)
- [ ] CCPA compliance rule set (Target: Q3 2026)

## Planned Features 📋

### Short-term (Next 3-6 months)
- [ ] Policy hot-reload on configuration change
- [ ] Conflict detection for overlapping access control policies
- [ ] CCPA / CPRA data subject rights enforcement
- [ ] Data lineage tracking for governed datasets
- [ ] Policy simulation / dry-run mode to preview access decisions
- [ ] Compliance report generation (PDF / JSON summary)

### Long-term (6-12 months)
- [ ] OPA (Open Policy Agent) integration for policy-as-code
- [ ] SOC 2 compliance controls and evidence collection
- [ ] PCI-DSS data isolation rules
- [ ] AI/ML model governance (training data lineage, bias auditing)
- [ ] Cross-tenant governance policy inheritance
- [ ] Automated data masking for sensitive fields in query results

## Implementation Phases

### Phase 1: Policy Engine and Compliance Rules (Status: Completed)
- [x] Implemented policy engine for attribute-based data access control (`governance/policy_engine.cpp`)
- [x] Implemented GDPR and HIPAA compliance rule evaluation at query time
- [x] Implemented automated data retention policy enforcement with configurable TTLs
- [x] Implemented data classification and labeling for PII, PHI, and confidential fields
- [x] Integrated audit trail recording all governance enforcement events

### Phase 2: Policy Versioning and Reporting (Status: In Progress)
- [~] Implement policy versioning with rollback support (`governance/policy_manager_versioned.cpp`)
- [~] Implement compliance report generation summarizing rule evaluations per time window
- [~] Implement policy conflict detection for overlapping access control rules

### Phase 3: Hot-Reload, CCPA, and OPA Integration (Status: Planned)
- [ ] Implement policy hot-reload on config file change without service restart
- [ ] Implement CCPA/CPRA data subject rights enforcement (right-to-delete, right-to-know)
- [ ] Implement automated data masking for configured sensitive fields in query results
- [ ] Integrate Open Policy Agent (OPA) as an alternative policy evaluation engine

## Production Readiness Checklist
- [ ] Unit tests coverage > 80%
- [x] Integration tests (policy evaluation, retention enforcement, audit trail)
- [ ] Performance benchmarks (policy evaluation latency at query time)
- [x] Security audit (policy bypass prevention, audit trail integrity)
- [x] Documentation complete (policy engine, compliance governance, integration guide)
- [x] API stability guaranteed for policy engine and rule evaluation

## Known Issues & Limitations
- Policy hot-reload requires restart currently
- CCPA rule set is not yet implemented
- OPA integration is planned but not started
- Automated data masking in query results is not yet implemented
- Data lineage tracking is not yet available

## Breaking Changes
- OPA integration will introduce a new policy language alongside existing rule format (additive)
- Policy simulation API will be a new endpoint (non-breaking)
