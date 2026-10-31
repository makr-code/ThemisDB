---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# LLM Wiki Security

## Purpose and security posture

This page documents source-visible trust boundaries and residual risks for `llm_wiki`. It is not a security certification. The integrating service must provide authentication, authorization, tenant routing, and deployment controls that are outside the plugin contract.

## Trust boundaries and controls

| Boundary | Source-visible behavior | Residual risk |
|---|---|---|
| Query and chunk text | The plugin has a small case-folded fixed-string matcher. `guardrail_patterns.h` is a separate, broader utility. | Matching is heuristic and incomplete; bypass/false-positive risk remains. With `fail_open=true`, a flagged query continues. |
| Edition availability | `edition_gate.cpp` maps `THEMISDB_LLM_WIKI_ENTERPRISE_ENABLED` to Enterprise, otherwise Community. | No independent runtime license verification; the gate does not establish caller authorization. |
| Workspace selection | Workspace files are written below the configured root. | The plugin does not authenticate users or enforce tenant-to-root mapping or document ACLs. Filesystem ownership and service authorization are integration responsibilities. |
| Workspace state | JSON state is checked against a 64-bit FNV-1a checksum formatted with a `sha256:` prefix; recovery attempts use stored log data. | This is not cryptographic integrity or tamper evidence. The state manager requires caller-side synchronization. |
| Process policy | A configured YAML file is schema-validated and checked for semantic invariants; optional hot reload retains the prior in-memory policy on candidate failure. | File permissions and provenance are operator responsibilities. The declared policy stages/rollout are not all implemented in current plugin request paths. |
| Persisted content and logs | Workspace pages and logs include ingested/query-derived content and source references. | Treat workspaces, backups, and diagnostics as sensitive; the module does not establish actor-attributed audit logging. |

## Security requirements for integrators

- Authenticate callers and authorize each tenant/workspace operation before calling the plugin.
- Restrict the workspace and policy files to the service identity; use separate roots where tenant isolation is required.
- Treat user queries, source documents, provider outputs, workspace files, and policy files as untrusted input.
- Do not use a successful pattern check or checksum as proof of safe content, authorization, or tamper resistance.
- Keep secrets in the deployment secret manager; do not store them in pages, state, policy files, or logs.
- Preserve workspace data and sanitized logs before incident recovery. Escalate suspected cross-tenant exposure, policy tampering, or state corruption.
- Apply request-size, concurrency, timeout, memory, and disk limits outside the module until enforceable module-level limits are implemented.

## Known security gaps

- Incomplete plugin-local prompt-injection matching and `fail_open` behavior.
- Compile-time edition selection without independent runtime license validation.
- No plugin-level user identity, authorization service, or actor-attributed audit trail.
- Non-cryptographic workspace checksum and limited recovery semantics.
- No established end-to-end runtime evidence-allowlist or provenance trust enforcement on the plugin query path.
- No source-backed module-wide resource limits.

## Security test evidence

Relevant test sources include `tests/llm/test_llm_wiki_edition_gates.cpp`, `test_llm_wiki_phase3_edge_cases_focused.cpp`, `test_llm_wiki_process_policy_manager.cpp`, and `test_llm_wiki_block4_backend_gate.cpp`. Their existence is not evidence that a specific build lane has executed or passed them; use configured test results and security review for release decisions.

See [`TRUST.md`](TRUST.md) for the source-grounded trust model and [`RUNBOOK.md`](RUNBOOK.md) for incident response guidance.

> Exact model backend not verifiable from final document text alone.
