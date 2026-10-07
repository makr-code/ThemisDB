# Assets Domain Groups

This directory stores grouped process-method assets.

## Process groups

- `administration/`
- `scientific/`
- `governance/`
- `legal/`
- `judicial/`
- `legislative/`
- `executive/`
- `finance/`
- `military/`
- `corporate/`
- `accounting/`
- `intelligence/`
- `social/`
- `networks/`
- `critical_infrastructure/`

Per group, the baseline asset set is:
- `default_<group>_process.yaml` (+ `.md`)
- `default_<group>_submodel.yaml` (+ `.md`)
- `default_<group>_reusable_model.yaml` (+ `.md`)

Additional domain-specialized models can be added beside these defaults
(e.g. `administration/vwvfg_administration_method.yaml`, `intelligence/people_assessment_compliant.yaml`).

Each process YAML has a same-name Markdown companion with detailed explanation and Mermaid process image.

Central catalog:
- `model_catalog.yaml`
