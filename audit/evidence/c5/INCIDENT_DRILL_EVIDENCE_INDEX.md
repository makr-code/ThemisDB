---
Author: "ThemisDB Contributors"
Created: "2026-09-14"
Last Updated: "2026-11-09"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# BSI C5 Incident Drill Evidence Index

**Author:** ThemisDB Contributors  
**Created:** 2026-09-14  
**Last Updated:** 2026-11-09
**Status:** active

## Scope

Reference index for incident and recovery drills required by the C5-2026 delta.

## Required entries per drill

- Drill identifier and date
- Scenario type (failover, recovery, data-integrity, security incident)
- Linked CI/test artifacts
- Linked runbook section
- Result (pass/fail) and open remediations

## Current release-candidate evidence (v2.5.0-rc-candidate)

| Control / Drill | Run ID / Commit SHA | Evidence Artifact | Runbook Reference | Result | Open Remediation |
|---|---|---|---|---|---|
| BCM-03 incident/restore drill | No qualifying drill run or commit trace identified; [compliance run 42321944686](https://github.com/makr-code/ThemisDB/actions/runs/42321944686) (`5f8b44bacd78cfb940f5c1bf40456d6617d0284a`) is unrelated and failed overall | None linked for the candidate release | `docs/operations/RUNBOOKS.md` §DR / §Failover | **Partial** — no current dated drill result or run artifact | ThemisDB maintainers; provide a dated incident/restore exercise and artifact in [#7817](https://github.com/makr-code/ThemisDB/issues/7817) |

The [v2.5.0 candidate manifest](v2.5.0-rc-candidate/manifest.json) links the available run and explicitly retains BCM-03 as partial. No historical test path below is a current drill record.

## Historical test/source mapping (2026-09-21; not current runtime evidence)

The following rows were recorded in the 2026-09-21 index without workflow run IDs, commit SHAs, or CI artifact links. Their historical `pass` labels are not independently run-verified and do not satisfy the current release evidence requirement.

| Drill ID | Date | Scenario | Evidence Artifact | Runbook Ref | Result | Open Remediation |
|---|---|---|---|---|---|---|
| DR-2026-01 | 2026-09-21 | Disaster recovery / crash recovery | `tests/test_disaster_recovery_manager.cpp`, `tests/legacy/disaster/test_disaster_recovery_manager.cpp` | `docs/operations/RUNBOOKS.md §DR` | pass | none |
| FAILOVER-2026-01 | 2026-09-21 | Failover Phase 2+3 (consensus + fencing) | `tests/failover/test_failover_phase2_phase3_focused.cpp`, `tests/failover/test_failover_wave_b_consensus.cpp`, `tests/failover/test_failover_wave_c_fencing_security.cpp` | `docs/operations/RUNBOOKS.md §Failover` | pass | none |
| FAILOVER-DR-EDGE-2026-01 | 2026-09-21 | Failover DR edge scenarios | `tests/failover/test_failover_dr_edge_scenarios.cpp` | `docs/operations/RUNBOOKS.md §DR` | pass | none |
| CHAOS-NET-2026-01 | 2026-09-21 | Network chaos / packet loss / partition | `tests/chaos/test_chaos_network.cpp`, `tests/chaos/test_chaos_stress.cpp` | `docs/operations/RUNBOOKS.md §ChaosOps` | pass | none |
| CHAOS-SCHED-2026-01 | 2026-09-21 | Scheduler chaos / timing / concurrency hardening | `tests/chaos/test_chaos_scheduler.cpp`, `tests/chaos/test_chaos_concurrency_hardening.cpp` | `docs/operations/RUNBOOKS.md §ChaosOps` | pass | none |
| CHAOS-AI-2026-01 | 2026-09-21 | AI safety chaos (fail-closed, safety boundaries) | `tests/security/ai_safety/test_ai_safety_chaos.cpp` | `docs/operations/RUNBOOKS.md §AISafety` | pass | none |
| GPU-RECOVER-2026-01 | 2026-09-21 | GPU memory management / exhaust / recovery | `tests/test_gpu_memory_management.cpp`, `tests/gpu/test_gpu_memory_management.cpp` | `src/gpu/ROADMAP.md §WaveA` | pass | none |
