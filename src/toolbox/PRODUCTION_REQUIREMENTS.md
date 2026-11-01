---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Toolbox integration and operational boundaries

**Purpose:** Source-backed information for services embedding `src/toolbox`. This library does not define a standalone deployment configuration schema or production-readiness certification.

## Implemented interfaces and constraints

- Configure dependencies through `ToolboxBuilder` or inject an `IngestionToolbox`. A default toolbox uses a `NullTextGenerationBackend`; profile-load failures are logged and do not by themselves make `build()` fail.
- `ToolboxBuilder` is single-use. The registry must be initialized before global access; `instance()` otherwise throws. Reinitialization replaces the current instance. Do not reset the registry while operations are active.
- `ContentToolboxBridge` requires a toolbox and `ContentManager`; graph/vector writers are optional. A failed writer is logged and counted separately and does not necessarily change `BridgeResult::ok`.
- Toolbox metric methods return text for the owning objects. The toolbox does not implement an HTTP listener, `/metrics` route, exporter lifecycle, scrape interval, alert rules, or dashboard.
- The module does not define worker pools, registry/cache size settings, a profile reload command, queue management, or a rollback CLI. No module-owned YAML keys for those controls are established here.

## Service-owner responsibilities

The host application chooses lifecycle, access control, resource limits, backend credentials, network exposure, persistence/durability policy, retries, alerting, and metric endpoint publication. Verify those controls against the host and dependency contracts; do not infer them from toolbox construction or a successful bridge result.

Before retrying a bridge operation, determine whether content storage or an optional sink may already have accepted data. `enrichExisting()` retrieves previously stored content and can repeat downstream writes; callers own authorization and idempotency decisions.

## Validation and evidence

- The six source files and their generated CTest names are listed in [`../../tests/toolbox/README.md`](../../tests/toolbox/README.md).
- The three benchmark targets and their limitations are described in [`../../benchmarks/toolbox/README.md`](../../benchmarks/toolbox/README.md).
- Historical deployment snippets and numeric performance/certification statements previously present in this page had no implementation-backed configuration contract or run artifacts. They are intentionally not carried forward as active requirements.

> Exact model backend not verifiable from final document text alone.
