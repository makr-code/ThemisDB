<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Governance Module (Public Headers)

All notable changes to the Governance module public headers are documented here.  
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).  
For implementation-level changes see `../../src/governance/CHANGELOG.md`.

## [Unreleased]
*(All planned features implemented — see `../../src/governance/FUTURE_ENHANCEMENTS.md` for long-horizon items.)*

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
