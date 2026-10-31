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

# LLM Wiki Operations

## Operating boundary

This page covers routine deployment verification and maintenance for the
LLM Wiki module. Deployment-specific service commands, alert thresholds, and
storage locations are owned by the service operator and are not defined by
this module's source. Use [`RUNBOOK.md`](RUNBOOK.md) when a check fails or an
incident is active.

## Before enabling the plugin

- Confirm the deployed build is intended to include LLM Wiki. The current
  edition gate is compile-time controlled; `edition_gate.cpp` maps the
  enabled macro to Enterprise and its absence to Community.
- Confirm the plugin binary exports the canonical factory and destroy
  symbols declared in `include/llm_wiki/llm_wiki_plugin_interface.h`.
- If a process policy path is configured, verify the file exists, is
  readable by the service, and passes the module policy schema and semantic
  invariants before rollout.
- Confirm the workspace root is owned and writable by the service identity,
  has sufficient disk capacity, and is not shared with an untrusted tenant.
- Confirm whether the build uses RocksDB. It is optional at CMake configure
  time; the feature's presence must not be inferred from a deployment config
  alone.
- Confirm provider/model configuration and secret delivery through the
  deployment secret manager. Do not put credentials in policy YAML, page
  content, logs, or workspace state.

## Routine checks

1. Review plugin initialization results and policy-load failures in the
   service logs.
2. Track operation results exposed by the integration: ingest `errors` and
   `failed_files`, query guardrail flags and filtered count, workspace stats,
   and policy reload errors.
3. Check workspace disk usage and state/log file readability. Preserve a
   backup before maintenance or manual intervention.
4. Review policy changes before rollout; retain the prior reviewed file for
   rollback. Hot reload validates the candidate and keeps the previous
   in-memory policy when validation fails.
5. Run `wikiLint` for configured workspaces only where the implementation
   supports it, then review findings instead of automatically deleting or
   rewriting pages.

The module source does not establish a uniform metrics exporter, alert
threshold, health endpoint, or service-level objective. Do not assume that
the fields in `WikiWorkspaceStats` are continuously populated or exported.
Configure production observability at the integrating service and validate
the actual signals available in that build.

## Maintenance and change control

- Keep a consistent backup of `wiki/state.json`, `wiki/state.log`, and
  workspace pages before migrations or incident recovery.
- Do not hand-edit `state.json` or truncate append-only state logs.
- Validate schema or process-policy changes in a non-production environment
  before rollout. Schema changes require an explicit compatibility and
  rollback plan; a general migration runner is not present in the module
  source.
- Test restore and rollback using a copy of representative workspace data.
- For performance changes, capture the code revision, hardware, corpus, and
  query set; targets in `PERFORMANCE_EXPECTATIONS.md` are not proof that a
  target has been met.

## Related documents

- [`RUNBOOK.md`](RUNBOOK.md) — incident diagnosis and recovery
- [`PRODUCTION_REQUIREMENTS.md`](PRODUCTION_REQUIREMENTS.md) — module-level
  operating constraints and declared readiness requirements
- [`API.md`](API.md) — result fields available to integrations

> Exact model backend not verifiable from final document text alone.
