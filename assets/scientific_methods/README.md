# Scientific Methods Assets

This directory defines first-class scientific-method configurations for ThemisDB Dream-Mode.

Files:
- `scientific_foundation.json` — scientific reasoning foundation (core principles, 6-step method, source hierarchy, confidence/error standards, improvement loop).
- `default_method.yaml` — primary 8-phase method for autonomous problem classification, BPMN strategy mapping, validation, and controlled evolution.
- `default_method.md` — detailed process explanation for `default_method.yaml`.
- `default_method_phase1_conservative_backup_20251012_204557.yaml` — conservative fallback profile with restricted decision policy.
- `default_method_phase1_conservative_backup_20251012_204557.md` — detailed process explanation for the conservative backup profile.
- `agentic_bpmn_coding_loop.yaml` — coding-oriented agentic BPMN loop (scope -> plan -> candidate patching -> validation -> controlled promotion).
- `agentic_bpmn_coding_loop.md` — detailed process explanation for the coding loop.
- `agentic_bpmn_deep_research_loop.yaml` — deep-research-oriented agentic BPMN loop (decomposition -> evidence -> contradiction analysis -> synthesis -> controlled promotion).
- `agentic_bpmn_deep_research_loop.md` — detailed process explanation for the deep research loop.
- `vwvfg_administration_method.yaml` — legal-governed administration process profile for VwVfG-style workflows.
- `vwvfg_administration_method.md` — detailed process explanation for the VwVfG administration profile.
- `model_catalog.yaml` — catalog/index for scientific-method model families and intended use-cases.
- `scientific_method.schema.json` — schema contract for validating scientific-method configuration files.

Process documentation rule:
- every process YAML in this directory should provide a same-name Markdown companion (`<process>.md`) with detailed operational explanations.

Governance constraints:
- run mode must remain `dream_research`,
- fail-closed behavior is mandatory,
- no promotion without gate closure and human approval.
- all method families should reference `scientific_foundation.json` as shared reasoning baseline.

Modeling note:
- The additional model families are pattern-inspired by established agentic coding and deep-research workflows, but remain ThemisDB-local abstractions under repository governance.
