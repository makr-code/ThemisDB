# Governance Module

Policy engine and compliance governance implementation for ThemisDB.

## Module Purpose

Implements policy-based data governance for ThemisDB, enabling GDPR/HIPAA compliance through policy rule evaluation, data classification, automated retention policies, and audit trail integration.

## Subsystem Scope

**In scope:** Policy engine for data access control, GDPR/HIPAA rule evaluation, data retention automation, data classification and labeling, audit trail integration.

**Out of scope:** Authentication (handled by auth module), encryption (handled by security module), audit log storage (handled by utils module).

## Relevant Interfaces

- `policy_engine.cpp` — core policy evaluation engine
- `policy_manager.cpp` — policy lifecycle management (load, validate, activate)
- `cross_tenant_policy_inheritance.cpp` — cross-tenant governance policy inheritance
- `compliance_reporter.cpp` — GDPR/HIPAA/CCPA compliance reporting
- `compliance_reporting.cpp` — report generation engine (JSON, CSV, HTML)
- `data_lineage.cpp` — data lineage tracking for governed datasets
- `ccpa_rules.cpp` — CCPA/CPRA data subject rights rule evaluators
- `soc2_controls.cpp` — SOC 2 Trust Services Criteria controls and evidence collection
- `policy_template.cpp` — built-in policy templates (GDPR, HIPAA, SOC 2, etc.)

## Current Delivery Status

**Maturity:** 🟡 Beta — Policy engine, GDPR/HIPAA/CCPA rule evaluation, SOC 2 controls, hot-reload, and compliance reporting operational; OPA integration planned.

## Components

- Policy engine
- Compliance rule evaluation
- Data governance enforcement
- Audit trail integration

## Features

- Policy-based data access control
- Compliance rule evaluation (GDPR, HIPAA, CCPA/CPRA, etc.)
- SOC 2 Trust Services Criteria controls and evidence collection (CC6.1, CC7.2, CC8.1, A1.1, C1.1, PI1.2)
- Automated data retention policies
- Data classification and labeling
- CCPA/CPRA data subject rights enforcement
- Policy hot-reload without service restart
- Cross-tenant governance policy inheritance with most-restrictive-wins merge semantics

## Documentation

For governance documentation, see:
- [Policy Engine](../../docs/src/governance/policy_engine.cpp.md)
- [Governance Usage](../../docs/governance_usage.md)
- [Compliance Governance Strategy](../../docs/compliance_governance_strategy.md)
- [Compliance Integration](../../docs/compliance_integration.md)

## Scientific References

1. European Parliament and Council. (2016). **General Data Protection Regulation (GDPR)**. Official Journal of the European Union, L 119. https://eur-lex.europa.eu/eli/reg/2016/679/oj

2. National Institute of Standards and Technology. (2020). **Security and Privacy Controls for Information Systems and Organizations**. NIST Special Publication 800-53 Rev. 5. https://doi.org/10.6028/NIST.SP.800-53r5

3. Abiteboul, S., Hull, R., & Vianu, V. (1995). **Foundations of Databases**. Addison-Wesley. https://webdam.inria.fr/Alice/

4. Sandhu, R. S., Coyne, E. J., Feinstein, H. L., & Youman, C. E. (1996). **Role-Based Access Control Models**. *IEEE Computer*, 29(2), 38–47. https://doi.org/10.1109/2.485845
