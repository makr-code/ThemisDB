---
name: Module Task
about: Create a focused module implementation or documentation task with the repository's governance and evidence contract.
title: "[module][task] {{MODULE_NAME}}: {{TASK_SUMMARY}}"
labels: {{ISSUE_LABELS_JSON}}
assignees: []
---

## Metadaten
- Labels: {{ISSUE_LABELS}}
- Milestone: {{ISSUE_MILESTONE}}
- Status: {{ISSUE_STATUS}}
- Link zur Gesamtstatus-Tabelle: [ROADMAP.md](../../ROADMAP.md)
- Issue-Key: {{ISSUE_KEY}}

## Gesamtstatus
{{OVERALL_STATUS}}

## Kurzbefund
{{SUMMARY_FINDINGS}}

## Scope
- Module: {{MODULE_NAME}}
- Files: {{SCOPE_FILES}}
- Status: {{STATUS}}
- Documentation: {{DOCS_STATUS}}

## Source Evidence
- Module docs: {{README_PATH}}
- Source graph / symbol evidence: {{GRAPH_PATH}}
- Doxygen artifact: {{DOXYGEN_PATH}}
- Compliance report: {{COMPLIANCE_REPORT_PATH}}

## What needs to be done
{{IMPLEMENTATION_TASKS}}

## Validation
- Repro command: {{VALIDATION_REPRO}}
- Gate check: {{VALIDATION_GATE}}
- Success condition: {{VALIDATION_SUCCESS}}

{{ISSUE_BODY}}

## Merge Gate
- Merge erst nach vollstaendig erfuellen Akzeptanzkriterien und Maintainer-Freigabe.

## Acceptance criteria
{{ACCEPTANCE_CRITERIA}}

## Notes
- Risk/constraint: {{RISK_CONSTRAINT}}
- Ownership/path rule: {{OWNERSHIP_PATH}}
- Follow the module quick rules in [.github/copilot/module-quick-rules.md](../copilot/module-quick-rules.md) and the detailed guidance in [.github/MODULE_DEVELOPER_DOCUMENTATION_GUIDELINES.md](../MODULE_DEVELOPER_DOCUMENTATION_GUIDELINES.md).
- Add exactly one `area:*` label matching the module and keep `type:*`, `priority:*`, and `status:*` aligned with [.github/GOVERNANCE.md](../GOVERNANCE.md).
- Archive or delete markdown files that are no longer relevant, except the governance documentation.
