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

# LLM Wiki Production Requirements

## Purpose and source of truth

These are release-readiness requirements for integrators and maintainers. They describe controls that must be demonstrated before production use; they are not claims that the current implementation already satisfies them. Project-level gates and release posture are governed by [`../../ROADMAP.md`](../../ROADMAP.md).

## Required integration controls

- **Edition and licensing:** Verify the shipped binary's compile-time edition setting. The module gate does not independently validate runtime licenses or caller entitlements.
- **Authentication and workspace access:** Authenticate callers, authorize tenant/workspace selection, and secure filesystem roots at the service boundary. The plugin interface carries no user identity or document ACL enforcement.
- **Input screening:** Treat guardrail matching as a heuristic only. The plugin matcher is a small local list; it is not comprehensive injection protection or authorization. Do not enable `fail_open` where flagged input must be rejected.
- **Secrets:** Do not store provider credentials or deployment secrets in wiki pages, workspace state/log files, policy YAML, or diagnostic artifacts. Use the deployment secret-management facility.
- **Workspace integrity:** Do not treat the `sha256:`-prefixed FNV-1a checksum as cryptographic integrity or tamper evidence. Restrict write access and maintain protected backups.
- **Policy configuration:** If a process-policy path is configured, validate the file and schema during deployment. Hot reload is opt-in. Do not assume the YAML's full scheduling, ML rollout, telemetry, or governance behavior is active in plugin requests.
- **Resource limits:** Apply input-size, concurrency, timeout, disk, and memory limits in the integrating service; the module does not establish a complete enforceable limit set.
- **Persistence:** Confirm the configured workspace root is writable and backed up. Confirm RocksDB build/runtime availability separately; the optional store wrapper alone does not establish end-to-end persistence.

## Required behavior before release

- Ingest must process source content and report success/failure consistently.
- Query must use the intended retrieval path and honor its public options.
- Wikipedia dump ingestion must process the supplied input or be disabled from the supported API surface.
- Edition, policy, workspace, and error behavior must have focused tests against the actual shipped build.
- Production-path performance must meet the project target with representative hardware and reproducible evidence; see [`PERFORMANCE_EXPECTATIONS.md`](PERFORMANCE_EXPECTATIONS.md).
- Security review must cover service authorization, workspace isolation, policy integrity, content screening, persistence, and recovery. See [`SECURITY.md`](SECURITY.md).

## Release checklist

- [ ] Validate supported build flags and edition/license integration.
- [ ] Verify caller authentication, tenant routing, and filesystem ownership.
- [ ] Verify content-based ingestion, option-aware retrieval, and Wikipedia import behavior.
- [ ] Verify malformed policy, reload failure, corrupt workspace, and unavailable optional storage behavior.
- [ ] Run focused tests in a configured build and attach test results.
- [ ] Collect production-path performance and resource measurements.
- [ ] Complete maintainer security and operational review.

No item should be marked complete based only on an interface declaration, synthetic benchmark, or documentation statement.

> Exact model backend not verifiable from final document text alone.
