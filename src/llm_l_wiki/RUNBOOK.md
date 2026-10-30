---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# LLM-L-Wiki runbook

## Applicability

This runbook covers mistaken or unsupported claims that `src/llm_l_wiki/` is a
running component. The checked-out path has no registered target or direct
tests, so it has no module-specific process, endpoint, log stream, health check,
or service recovery command.

## Triage: module reported as unavailable

1. Confirm the deployed artifact and edition/configuration with the service
   owner; this directory alone does not establish what is deployed.
2. In the exact source revision, inspect
   `src/llm_l_wiki/wikipedia/wiki_workspace_orchestrator.cpp` and search the
   build files for `src/llm_l_wiki`. A zero-byte source and absent registration
   mean there is no evidence this module was compiled.
3. Check whether the operator or release note intended the separate
   `llm_wiki` module or `themisdb_llm_wiki` plugin. Do not silently redirect
   traffic; verify the owning component's contract first.
4. If `llm_l_wiki` is claimed as a release capability, pause that claim and
   escalate to the module/release owner for source, target, and test evidence.

## Recovery and rollback

There is no module-specific state store or recovery procedure to run. Do not
delete or rewrite data based on this documentation. If an actual deployed
component is affected, use that component's approved service runbook and
release rollback procedure; identify its owning module before acting.

## Escalation evidence

Record the deployed artifact/version, exact source revision, reported
operation, build-target evidence, and any logs from the actual owning service.
No `llm_l_wiki`-specific telemetry or incident severity threshold is defined.

## References

- [Operations gates](OPERATIONS.md)
- [Architecture and failure boundary](ARCHITECTURE.md)
- [Separate `llm_wiki` docs](../llm_wiki/README.md)

> AI-assisted documentation. Exact model backend not verifiable from final document text alone.
