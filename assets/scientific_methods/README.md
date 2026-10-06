# Scientific Methods Assets

This directory defines first-class scientific-method configurations for ThemisDB Dream-Mode.

Files:
- `default_method.yaml` — primary 8-phase method for autonomous problem classification, BPMN strategy mapping, validation, and controlled evolution.
- `default_method_phase1_conservative_backup_20251012_204557.yaml` — conservative fallback profile with restricted decision policy.
- `scientific_method.schema.json` — schema contract for validating scientific-method configuration files.

Governance constraints:
- run mode must remain `dream_research`,
- fail-closed behavior is mandatory,
- no promotion without gate closure and human approval.
