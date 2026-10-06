# Scientific Methods Assets

This directory defines first-class scientific-method configurations for ThemisDB Dream-Mode.

Core files:
- `scientific_foundation.json` — scientific reasoning foundation (core principles, 6-step method, source hierarchy, confidence/error standards, improvement loop).
- `scientific_method.schema.json` — schema contract for validating scientific-method configuration files.
- `../model_catalog.yaml` — catalog/index for grouped scientific-method model families and intended use-cases (stored in `assets/` root).

Grouped process domains (stored directly in `assets/`, each with YAML + same-name Markdown):
- `../scientific/`
  - `default_method.yaml`, `default_method.md`
  - `default_method_phase1_conservative_backup_20251012_204557.yaml`, `default_method_phase1_conservative_backup_20251012_204557.md`
  - `agentic_bpmn_coding_loop.yaml`, `agentic_bpmn_coding_loop.md`
  - `agentic_bpmn_deep_research_loop.yaml`, `agentic_bpmn_deep_research_loop.md`
  - `default_scientific_process.yaml`, `default_scientific_process.md`
  - `default_scientific_submodel.yaml`, `default_scientific_submodel.md`
  - `default_scientific_reusable_model.yaml`, `default_scientific_reusable_model.md`
- `../administration/`
  - `default_administration_process.yaml`, `default_administration_process.md`
  - `default_administration_submodel.yaml`, `default_administration_submodel.md`
  - `default_administration_reusable_model.yaml`, `default_administration_reusable_model.md`
  - `vwvfg_administration_method.yaml`, `vwvfg_administration_method.md`
- `../governance/`
  - `default_governance_process.yaml`, `default_governance_process.md`
  - `default_governance_submodel.yaml`, `default_governance_submodel.md`
  - `default_governance_reusable_model.yaml`, `default_governance_reusable_model.md`
- `../legal/`
  - `default_legal_process.yaml`, `default_legal_process.md`
  - `default_legal_submodel.yaml`, `default_legal_submodel.md`
  - `default_legal_reusable_model.yaml`, `default_legal_reusable_model.md`
- `../judicial/`
  - `default_judicial_process.yaml`, `default_judicial_process.md`
  - `default_judicial_submodel.yaml`, `default_judicial_submodel.md`
  - `default_judicial_reusable_model.yaml`, `default_judicial_reusable_model.md`
- `../legislative/`
  - `default_legislative_process.yaml`, `default_legislative_process.md`
  - `default_legislative_submodel.yaml`, `default_legislative_submodel.md`
  - `default_legislative_reusable_model.yaml`, `default_legislative_reusable_model.md`
- `../executive/`
  - `default_executive_process.yaml`, `default_executive_process.md`
  - `default_executive_submodel.yaml`, `default_executive_submodel.md`
  - `default_executive_reusable_model.yaml`, `default_executive_reusable_model.md`
- `../finance/`
  - `default_finance_process.yaml`, `default_finance_process.md`
  - `default_finance_submodel.yaml`, `default_finance_submodel.md`
  - `default_finance_reusable_model.yaml`, `default_finance_reusable_model.md`
- `../military/`
  - `default_military_process.yaml`, `default_military_process.md`
  - `default_military_submodel.yaml`, `default_military_submodel.md`
  - `default_military_reusable_model.yaml`, `default_military_reusable_model.md`
- `../corporate/`
  - `default_corporate_process.yaml`, `default_corporate_process.md`
  - `default_corporate_submodel.yaml`, `default_corporate_submodel.md`
  - `default_corporate_reusable_model.yaml`, `default_corporate_reusable_model.md`
- `../accounting/`
  - `default_accounting_process.yaml`, `default_accounting_process.md`
  - `default_accounting_submodel.yaml`, `default_accounting_submodel.md`
  - `default_accounting_reusable_model.yaml`, `default_accounting_reusable_model.md`
- `../intelligence/`
  - `default_intelligence_process.yaml`, `default_intelligence_process.md`
  - `default_intelligence_submodel.yaml`, `default_intelligence_submodel.md`
  - `default_intelligence_reusable_model.yaml`, `default_intelligence_reusable_model.md`
  - `people_assessment_compliant.yaml`, `people_assessment_compliant.md`
- `../social/`
  - `default_social_process.yaml`, `default_social_process.md`
  - `default_social_submodel.yaml`, `default_social_submodel.md`
  - `default_social_reusable_model.yaml`, `default_social_reusable_model.md`
- `../networks/`
  - `default_networks_process.yaml`, `default_networks_process.md`
  - `default_networks_submodel.yaml`, `default_networks_submodel.md`
  - `default_networks_reusable_model.yaml`, `default_networks_reusable_model.md`
- `../critical_infrastructure/`
  - `default_critical_infrastructure_process.yaml`, `default_critical_infrastructure_process.md`
  - `default_critical_infrastructure_submodel.yaml`, `default_critical_infrastructure_submodel.md`
  - `default_critical_infrastructure_reusable_model.yaml`, `default_critical_infrastructure_reusable_model.md`

Process documentation rule:
- every process YAML in this directory should provide a same-name Markdown companion (`<process>.md`) with detailed operational explanations.
- process Markdown companions should include a process image (Mermaid flow) for quick operational understanding.

Governance constraints:
- run mode must remain `dream_research`,
- fail-closed behavior is mandatory,
- no promotion without gate closure and human approval.
- all method families should reference `scientific_foundation.json` as shared reasoning baseline.

Modeling note:
- The additional model families are pattern-inspired by established agentic coding and deep-research workflows, but remain ThemisDB-local abstractions under repository governance.
