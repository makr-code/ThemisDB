<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Governance Module (Public Headers)

All notable changes to the Governance module public headers are documented here.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).
For implementation-level changes see `../../src/governance/CHANGELOG.md`.

## [Unreleased]
*(All planned features implemented — see `../../src/governance/FUTURE_ENHANCEMENTS.md` for long-horizon items.)*

## [1.9.0] — 2026-04-09
### Added
- `iso27001_rules.h`: 6 Annex A control evaluators (`Iso27001A912Control`, `Iso27001A1011Control`, `Iso27001A1241Control`, `Iso27001A1242Control`, `Iso27001A1323Control`, `Iso27001A1813Control`), `Iso27001ControlSet`, and supporting types (`Iso27001EvidenceItem`, `Iso27001ControlResult`, `Iso27001AuditReport`) covering A.9.1.2, A.10.1.1, A.12.4.1, A.12.4.2, A.13.2.3, A.18.1.3
- `hipaa_rules.h`: 6 Security Rule requirement evaluators (`HipaaAccessControl`, `HipaaEncryption`, `HipaaAuditControls`, `HipaaIntegrityControls`, `HipaaTransmissionSecurity`, `HipaaRetention`), `HipaaRuleSet`, and `HipaaRuleEvalResult` covering §164.312(a)(1), §164.312(a)(2)(iv), §164.312(b), §164.312(c)(1), §164.312(e)(2)(ii), §164.530(j)

## [1.8.0] — 2026-03-21
### Added
- CI workflow `governance-module-ci.yml` covering 19 governance test suites

## [1.7.0] — 2026-03-09
### Added
- `cross_tenant_policy_inheritance.h`: tenant policy hierarchy with cycle detection and most-restrictive-wins merge semantics; `evaluateEffectivePolicy()` and `resolveEffectiveRules()` (Issue #1772)

## [1.6.0] — 2026-02-01
### Added
- `model_governance.h`: ML model governance and audit interface
- `opa_adapter.h`: Open Policy Agent integration adapter
- `data_lineage.h`: Data lineage graph tracking
- `review_scheduler.h`: Scheduled policy review reminders
- `policy_file_watcher.h`: Hot-reload on policy file changes

## [1.0.0] — 2024-01-01
### Added
- `policy_engine.h`, `policy_manager.h`, `policy_validator.h`
- `ccpa_rules.h`, `pci_dss_rules.h`, `soc2_controls.h`
- `compliance_reporter.h`, `compliance_reporting.h`
- `data_masker.h`, `policy_template.h`, `policy_coordinator.h`
