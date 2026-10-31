---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Prompt Engineering Trust Boundaries

## Purpose

This page describes the module's trust assumptions for developers integrating prompt templates, user context, model responses, feedback, and rewrite rules. It is not a security certification or a substitute for the project security policy.

## Trust boundaries

| Input or component | Trust treatment | Module control and caller responsibility |
|---|---|---|
| Template definitions and YAML files | Configuration input; treat as untrusted until reviewed and validated | Template validation checks required fields and metadata shape. The rewrite loader accepts only regex and dictionary rules and reports parse/validation failures. Restrict write access to the files and rule-registration code. |
| Request context and user prompt text | Untrusted data | `PromptManager::injectContext()` performs literal substitution without escaping or missing-placeholder checks. Authorize and constrain values before rendering; do not treat substituted text as trusted instructions. |
| Model output | Untrusted, including text that may be forwarded into a later model call | `PromptInjectionDetector::detectInResponse()` adds heuristics for fake system-role and instruction markers. The downstream caller still owns output parsing, validation, and authorization. |
| Embedding provider | External or caller-supplied implementation | Provider receives text supplied for scoring. Manage endpoint credentials, network policy, data residency, model version, and timeout behavior outside this module. |
| Feedback storage and synchronization | May contain sensitive user/model data | Feedback entries retain query, response, text, and metadata locally and may be persisted. Cross-shard publishing sends a query embedding rather than raw text when both dependencies are configured; embeddings are not guaranteed anonymous or non-reversible. |
| Rewrite policies | Security-sensitive executable C++ rules | YAML cannot create semantic or policy rules. Review and test compiled policy rules, registration, ordering, terminal behavior, and bounded execution. |

## Detection and security limits

The injection detector combines shared-registry regex patterns, keywords, and syntax heuristics into a risk score. It can miss novel or obfuscated attacks and can flag benign text. Its `sanitize()` method replaces matching patterns and keywords; it does not establish that the remaining text is safe. Treat detections as signals for an explicit caller policy, not as authentication, authorization, isolation, or a complete prompt firewall.

`PromptTemplateValidator` uses bounded string/pattern checks for specific SQL, command, path, and template constructs. These checks are not a general-purpose parser or sandbox. A warning from its serialized-JSON validation makes the result invalid, while `PromptManager::validateTemplate()` is a separate structural check and does not run those injection heuristics.

`FeedbackEntry::computeChecksum()` is FNV-1a 64-bit for lightweight audit correlation; it is not a cryptographic signature, tamper-proof log, or confidentiality control. Avoid logging raw prompt or response content; detector warnings include a risk score, and providers may receive full text.

## Required controls for integrations

- Keep safety and authorization checks enabled in production. The detector's configurable `enabled` switch is not approval to disable controls in a deployment.
- Apply least privilege to template/rule writers, RocksDB access, provider credentials, and metrics/feedback readers.
- Set explicit host-side limits and timeouts for model/provider calls and bound input sizes before storage, embedding, or evaluation.
- Redact or minimize prompt, response, and feedback data in logs and incident artifacts; define retention and deletion at the owning service.
- Require security review for policy-rule changes, cross-shard data propagation, provider/model changes, and changes to detection thresholds.

## Incident handling

On suspected prompt injection or data exposure, preserve the relevant redacted result, matched labels, detector configuration, template/version ID, and service logs. Follow the project [security policy](../../SECURITY.md) and [`RUNBOOK.md`](./RUNBOOK.md). Do not rely on sanitized output as proof that no sensitive content was disclosed; escalate provider, storage, and authorization incidents to their owning teams.

Source contracts: [`prompt_injection_detector.h`](../../include/prompt_engineering/prompt_injection_detector.h), [`prompt_template_validator.cpp`](./prompt_template_validator.cpp), [`feedback_collector.h`](../../include/prompt_engineering/feedback_collector.h), and [`rewrite_rule.h`](../../include/prompt_engineering/rewrite_rule.h).

Exact model backend not verifiable from final document text alone.
