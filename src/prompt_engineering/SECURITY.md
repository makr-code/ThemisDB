---
Author: "ThemisDB Team"
Created: "2026-05-31"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# Security — Prompt Engineering Module

## Security boundary

The module contains template validators, injection-detection heuristics, structured-output helpers, and rewrite-rule interfaces. These are library-level controls and do not provide authorization, model isolation, a complete prompt firewall, or guaranteed output safety. The integrating service owns access control, model transport, tool policy, and downstream parsing/validation.

## Source-backed limits

- `PromptManager::injectContext()` performs literal substitution; it does not escape inserted values or decide whether they are authorized.
- `PromptInjectionDetector` uses heuristics and can miss novel attacks or flag benign text. Its result is a signal for caller policy, not an access decision.
- `PromptTemplateValidator` and `PromptManager::validateTemplate()` have distinct contracts; manager field validation does not run the JSON validator's injection heuristics.
- Feedback may retain query/response content, and provider calls may receive text supplied for evaluation. The host owns retention, access, and redaction policy.
- Rewrite rules are separate from the integration hooks; code presence does not mean they are applied to requests.

## Integration requirements

- Authorize and constrain context before substitution; validate generated output before using it as a query, tool input, or command.
- Apply least privilege to template/rule writers, database access, provider credentials, and readers of feedback or metrics.
- Minimize or redact prompt and response content in logs and incident records.
- Require security review for policy-rule changes, data propagation, provider/model changes, and detector threshold changes.

See [`TRUST.md`](./TRUST.md) for detailed trust boundaries, [`RUNBOOK.md`](./RUNBOOK.md) for incident response, and the repository [security policy](../../SECURITY.md) for reporting.
