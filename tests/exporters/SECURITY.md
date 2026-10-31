---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: unknown
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
Source_Level: "level1"
SOT_Domain: "security"
Canonical_References: "tests/exporters/test_export_encryption.cpp; tests/exporters/test_huggingface_hub_client.cpp; tests/exporters/test_exporters_phase23_hardening_focused.cpp; SECURITY.md"
Related: "Issue #7346; DOC-WEEKLY-2026-44"
---

# Exporter security-test scope

Exact model backend not verifiable from final document text alone.

The focused suite includes encryption behavior tests, Hub-client policy and
credential-result cases, and exporter policy/error-contract regressions. These
tests exercise code paths; they do not prove that a deployment has configured
authorization, encryption, key management, repository visibility, or incident
controls correctly.

Use synthetic test records and test-only credentials. Do not copy production
tokens, key material, or exported sensitive records into fixtures or logs.
Report security vulnerabilities through the project-level
[`SECURITY.md`](../../SECURITY.md).
