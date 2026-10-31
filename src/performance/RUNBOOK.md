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

# Performance Module Runbook

## Scope and safety

Use this runbook to triage performance behavior attributable to the
performance module. It does not replace service, storage, or incident-command
procedures. Do not change global runtime flags or restart a production service
without the owning service operator's approval and a recorded baseline.

## Triage sequence

1. Record the affected operation, time window, build revision, host/CPU/NUMA
   layout, workload shape, and enabled compile-time/runtime flags. Preserve
   representative latency and throughput samples before changing configuration.
2. Determine whether the report is a measurement/export issue or an actual
   workload regression. Check metric sampling, exporter state, dropped metrics,
   and whether counters are available; an empty export alone does not establish
   that the workload path is healthy.
3. Check optional-path evidence: detected hardware capabilities, accelerator
   `used_hw_path` and error fields, io_uring availability/fallback counters,
   NUMA topology, and vector-index cache/disk counters where relevant.
4. Compare against the same release-profile benchmark case and baseline. Check
   the mapped case manifest for missing entries before concluding that a run
   passed.
5. If an optimization is implicated, disable only that specific optional path
   through its supported feature control, with operator approval. Re-run the
   same workload and compare against the preserved sample. Keep the CPU or
   standard-I/O fallback if it is the documented safe alternative.
6. Restore the prior configuration only after the owner has reviewed the
   comparison. Escalate unresolved data loss, incorrect query results,
   unbounded resource growth, repeated exporter loss, or inability to return
   to the safe fallback through the service incident process.

## Common symptoms

| Symptom | Checks | Safe response |
|---|---|---|
| Unexpected latency after an adaptive change | Workload class, observation window, adaptation count, current strategy, and before/after baseline | Disable only the affected auto-adaptation path if the change is causally implicated; preserve observations for review. |
| Requested GPU/NUMA path is not active | Build gates, detected capabilities, `used_hw_path`, topology, and OS permissions | Treat capability fallback as distinct from failure; use the supported CPU/default path until the host/build is verified. |
| No or incomplete performance metrics | Collector running state, sampling/operation filters, `getDroppedMetrics()`, exporter output, and metric retention limits | Restore collection configuration and record gaps; do not interpret absent data as zero activity. |
| io_uring setup or operation failure | `io_uring_available`, kernel/runtime support, permissions, fallback counters, and returned errno | Keep standard send/receive fallback; do not enable SQ polling without the required host permission. |
| Vector search is slow or results differ | Dimension, k, beam width, dataset, index and sidecar files, cache/disk counters, recall against exact search | Stop changing the index until its graph and metadata files are preserved; compare the exact and approximate path with the same query set. |

## Escalation evidence

Attach the configuration and capability snapshot, time-bounded metrics,
benchmark manifest and baseline, affected operation names, error/result fields,
and the exact feature flag changed. Avoid attaching raw query contents or
credentials. Escalate code defects with the focused test and source path; route
security or privacy concerns through the project security process described in
[TRUST.md](TRUST.md).

Exact model backend not verifiable from final document text alone.
