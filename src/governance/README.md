# Governance Module

Policy engine and compliance governance implementation for ThemisDB.

## Module Purpose

Implements policy-based data governance for ThemisDB, enabling GDPR/HIPAA compliance through policy rule evaluation, data classification, automated retention policies, and audit trail integration.

## Subsystem Scope

**In scope:** Policy engine for data access control, GDPR/HIPAA rule evaluation, data retention automation, data classification and labeling, audit trail integration.

**Out of scope:** Authentication (handled by auth module), encryption (handled by security module), audit log storage (handled by utils module).

## Relevant Interfaces

- `policy_engine.cpp` — core policy evaluation engine (evaluate, checkQueryPermission, simulateDecision)
- `policy_manager.cpp` — policy lifecycle management (load, validate, activate)
- `policy_manager_versioned.cpp` — versioned policy management with rollback and conflict detection
- `cross_tenant_policy_inheritance.cpp` — cross-tenant governance policy inheritance
- `compliance_reporter.cpp` — GDPR/HIPAA/CCPA/PCI-DSS/SOC 2 compliance reporting
- `compliance_reporting.cpp` — report generation engine (JSON, CSV, HTML, PDF)
- `data_lineage.cpp` — data lineage tracking for governed datasets
- `ccpa_rules.cpp` — CCPA/CPRA data subject rights rule evaluators
- `soc2_controls.cpp` — SOC 2 Trust Services Criteria controls and evidence collection
- `pci_dss_rules.cpp` — PCI-DSS data isolation and compliance rules
- `data_masker.cpp` — field-level data masking (REDACT, TOKENIZE, TRUNCATE, HASH)
- `model_governance.cpp` — AI/ML model training governance and bias auditing
- `opa_adapter.cpp` — Open Policy Agent integration for Rego-based policy evaluation
- `policy_template.cpp` — built-in policy templates (GDPR, HIPAA, SOC 2, least-privilege, etc.)

## Current Delivery Status

**Maturity:** 🟢 Production-Ready — Policy engine, GDPR/HIPAA/CCPA/CPRA/PCI-DSS/SOC 2 rule evaluation, OPA integration, policy simulation, hot-reload, compliance reporting, data masking, data lineage, cross-tenant policy inheritance, and AI/ML model governance are all operational.

## Components

- Policy engine
- Compliance rule evaluation
- Data governance enforcement
- Audit trail integration

## Features

- Policy-based data access control (attribute-based, classification-based, and RBAC)
- Compliance rule evaluation (GDPR, HIPAA, CCPA/CPRA, PCI-DSS, ISO 27001, SOC 2)
- SOC 2 Trust Services Criteria controls and evidence collection (CC6.1, CC7.2, CC8.1, A1.1, C1.1, PI1.2)
- Automated data retention policies
- Data classification and labeling
- CCPA/CPRA data subject rights enforcement (right-to-know, right-to-delete, opt-out-of-sale, data portability)
- PCI-DSS data isolation rules
- Policy hot-reload without service restart
- Cross-tenant governance policy inheritance with most-restrictive-wins merge semantics
- Automated data masking in query results (REDACT, TOKENIZE, TRUNCATE, HASH)
- Data lineage tracking for governed datasets
- AI/ML model governance (training data lineage, bias auditing, export control)
- OPA (Open Policy Agent) integration for Rego-based policy evaluation
- Policy simulation / dry-run mode (`simulateDecision`) without audit trail side effects
- Policy versioning with rollback and conflict detection (contradictory, overlapping, circular)

## Documentation

For governance documentation, see:
- [ARCHITECTURE.md](./ARCHITECTURE.md) — Architecture guide with component diagram and data flow
- [ROADMAP.md](./ROADMAP.md) — Development roadmap and production readiness checklist
- [FUTURE_ENHANCEMENTS.md](./FUTURE_ENHANCEMENTS.md) — Planned features with performance targets and IEEE references
- [include/governance/README.md](../../include/governance/README.md) — Public API header overview
- [docs/de/governance/README.md](../../docs/de/governance/README.md) — German secondary documentation

## Scientific References

1. European Parliament and Council. (2016). **General Data Protection Regulation (GDPR)**. Official Journal of the European Union, L 119. https://eur-lex.europa.eu/eli/reg/2016/679/oj

2. National Institute of Standards and Technology. (2020). **Security and Privacy Controls for Information Systems and Organizations**. NIST Special Publication 800-53 Rev. 5. https://doi.org/10.6028/NIST.SP.800-53r5

3. Abiteboul, S., Hull, R., & Vianu, V. (1995). **Foundations of Databases**. Addison-Wesley. https://webdam.inria.fr/Alice/

4. Sandhu, R. S., Coyne, E. J., Feinstein, H. L., & Youman, C. E. (1996). **Role-Based Access Control Models**. *IEEE Computer*, 29(2), 38–47. https://doi.org/10.1109/2.485845

## Installation

This module is built as part of ThemisDB. See the root `CMakeLists.txt` for build configuration.

## Usage

The implementation files in this module are compiled into the ThemisDB library.
See [`../../include/governance/README.md`](../../include/governance/README.md) for the public API.
