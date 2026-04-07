<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ../../src/governance/ROADMAP.md -->

# Roadmap — Governance Module (Public Headers)

> Implementation roadmap: `../../src/governance/ROADMAP.md`

## Current Status

v1.8.0 — Production-ready. 21 public headers covering policy management, CCPA/PCI-DSS/SOC-2 compliance, data masking, lineage, model governance, OPA integration, and cross-tenant policy inheritance.

## Completed ✅

- [x] Policy engine core (`policy_engine.h`)
- [x] Policy CRUD and versioning (`policy_manager.h`, `policy_manager_versioned.h`, `policy_version_history.h`)
- [x] CCPA, PCI-DSS, SOC 2 rule sets
- [x] Compliance reporting (`compliance_reporter.h`, `compliance_reporting.h`)
- [x] Data masking, lineage, model governance
- [x] OPA adapter (`opa_adapter.h`)
- [x] Cross-tenant policy inheritance with cycle detection (v1.7.0)
- [x] 19 governance test suites in CI (v1.8.0)

## Planned

- [ ] ISO 27001 rule set header (Target: v1.9.0)
- [ ] HIPAA compliance rules (Target: v2.0.0)
- [ ] Policy diff and rollback interface (Target: v1.9.0)

## Production Readiness Checklist

- [x] All headers compile cleanly
- [x] 3 compliance frameworks (CCPA, PCI-DSS, SOC 2)
- [x] Tenant policy hierarchy with cycle detection
- [ ] ISO 27001 header
- [ ] HIPAA header
