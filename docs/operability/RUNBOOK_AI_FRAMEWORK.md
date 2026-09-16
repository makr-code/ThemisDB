# Runbook: AI Framework Module

<!-- Status: current | validated: 2026-09-16 | Wave D operability deliverable -->

## Purpose

Operator remediation guide for the ThemisDB `ai` framework module. Covers the
five most critical incident classes, their log patterns, triage steps, and
recommended remediation actions.

---

## Scenario 1 — Plugin Failed

**Log pattern:** `[AI:PluginFailed]`

**Symptoms**
- An AI plugin fails to load, initialise, or execute a dispatch request.
- Log lines contain `[AI:PluginFailed]` with plugin ID and failure reason.
- Dependent AI pipelines stall or fall back to degraded mode.

**Triage**
1. Identify the failing plugin ID and the failure stage (load, init, dispatch).
2. Check plugin health: `themis_admin ai plugin status <id>`.
3. Review the plugin's resource requirements (GPU, memory) against available capacity.
4. Check for recent plugin version deployments that may have introduced a regression.

**Remediation**
1. Restart the failing plugin: `themis_admin ai plugin restart <id>`.
2. If the failure persists, roll back to the previous plugin version:
   `themis_admin ai plugin rollback <id>`.
3. If the plugin requires unavailable hardware, switch to a CPU-capable fallback variant.
4. Disable the plugin if it is not required for current workloads:
   `themis_admin ai plugin disable <id>`.

**Escalation**
Plugin failures that persist after restart and rollback require escalation to
the AI platform team with plugin ID, failure log, resource availability, and
the plugin version history.

---

## Scenario 2 — KG Reasoner Stall

**Log pattern:** `[AI:KGReasonerStall]`

**Symptoms**
- The KG (Knowledge Graph) reasoner stops making progress on inference queries.
- Log lines contain `[AI:KGReasonerStall]` with reasoner ID and last progress timestamp.
- KG-backed answers are unavailable; the AI framework falls back to non-KG paths.

**Triage**
1. Identify the stalled reasoner ID from the log.
2. Check reasoner health: `themis_admin ai kg-reasoner status`.
3. Inspect the KG query queue for a backlog: `themis_admin ai kg-reasoner queue status`.
4. Check whether the KG store is responsive: `themis_admin dist_kg graph status`.

**Remediation**
1. Reset the stalled reasoner: `themis_admin ai kg-reasoner reset <id>`.
2. Drain the query queue if it is backed up: `themis_admin ai kg-reasoner queue drain`.
3. Restart the KG reasoner service if reset does not help:
   `themis_admin ai kg-reasoner restart`.
4. Verify KG store connectivity and health before re-enabling the reasoner.

**Escalation**
KG reasoner stalls that recur after restart indicate a KG store availability
or consistency issue — escalate to the knowledge graph team with reasoner ID,
queue size, and KG store status.

---

## Scenario 3 — Inference Timeout

**Log pattern:** `[AI:InferenceTimeout]`

**Symptoms**
- AI inference requests time out before producing a result.
- Log lines contain `[AI:InferenceTimeout]` with request ID and elapsed time.
- Inference latency p99 exceeds SLO; clients receive timeout errors.

**Triage**
1. Identify the timed-out request IDs and the inference backend in use.
2. Check inference backend utilisation: `themis_admin ai inference status`.
3. Review concurrent request count and queue depth.
4. Check for GPU memory pressure or thermal throttling on inference nodes.

**Remediation**
1. Increase the inference timeout threshold: set `ai.inference.timeout_ms`.
2. Scale inference workers to handle the current load:
   `themis_admin ai inference scale-up`.
3. Enable request shedding to protect the inference backend from overload:
   set `ai.inference.max_queue_depth`.
4. Reduce batch sizes if individual requests are taking too long.

**Escalation**
Persistent inference timeouts after scaling indicate a hardware capacity or
model complexity issue — escalate to the ML infrastructure team with inference
latency histograms, GPU utilisation traces, and the model version in use.

---

## Scenario 4 — Stats Overflow

**Log pattern:** `[AI:StatsOverflow]`

**Symptoms**
- The AI framework statistics collector exceeds its buffer capacity.
- Log lines contain `[AI:StatsOverflow]` with buffer size and drop count.
- Metrics dashboards show gaps; SLO tracking may be incomplete.

**Triage**
1. Check current stats buffer depth: `themis_admin ai stats status`.
2. Identify which stats dimensions are producing the highest cardinality.
3. Verify that the stats exporter is running and consuming from the buffer.
4. Check for a recent workload spike that increased metrics volume.

**Remediation**
1. Increase the stats buffer capacity: set `ai.stats.buffer_capacity`.
2. Increase the stats flush frequency: set `ai.stats.flush_interval_ms`.
3. Reduce stats cardinality by aggregating high-cardinality labels.
4. Restart the stats exporter if it has stalled: `themis_admin ai stats exporter restart`.

**Escalation**
Persistent stats overflow affects SLO observability — escalate to the
observability team with buffer size, flush interval, and the high-cardinality
dimension breakdown.

---

## Scenario 5 — Plugin Dispatch Cascade

**Log pattern:** `[AI:PluginFailed]` (multiple plugins, concurrent)

**Symptoms**
- Multiple AI plugins fail simultaneously during a high-load period.
- A cascade of `[AI:PluginFailed]` entries appears across different plugin IDs.
- The AI framework enters a degraded state where only core inference paths remain.

**Triage**
1. Identify how many plugins have failed and whether the failures share a common
   resource (GPU, shared library, network endpoint).
2. Check system resource exhaustion: memory, GPU memory, CPU.
3. Review the AI framework circuit breaker state:
   `themis_admin ai circuit-breaker status`.
4. Determine whether a recent deployment introduced a shared dependency regression.

**Remediation**
1. Trigger the AI framework fallback mode to protect the core inference path:
   `themis_admin ai fallback enable`.
2. Restart plugins one at a time starting with the highest priority:
   `themis_admin ai plugin restart <id>`.
3. Roll back any recent plugin deployments that share the failing dependency.
4. Restore the circuit breaker to closed state once plugins are stable:
   `themis_admin ai circuit-breaker reset`.

**Escalation**
A multi-plugin cascade failure requires immediate escalation to the AI platform
on-call — provide a list of all failing plugin IDs, shared resource usage, and
the deployment history from the past 24 hours.

---

## Reference

| Log Pattern               | Severity | SLO Impact | Owner |
|---------------------------|----------|------------|-------|
| `[AI:PluginFailed]`       | High     | Yes        | ai    |
| `[AI:KGReasonerStall]`    | Medium   | Partial    | ai    |
| `[AI:InferenceTimeout]`   | High     | Yes        | ai    |
| `[AI:StatsOverflow]`      | Low      | No         | ai    |

---

*Wave D operability deliverable — see `src/ai/ROADMAP.md`.*
