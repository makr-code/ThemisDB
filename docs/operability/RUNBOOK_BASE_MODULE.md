# Runbook: Base Module — Wave D Operability

<!-- Runbook: base | Wave D | validated: 2026-09-16 -->
<!-- Links: src/base/ROADMAP.md · docs/operability/README.md -->

## Overview

This runbook covers operator-critical incident scenarios for the `base`
module. Use it to diagnose and remediate tracing exporter failures, core
initialization failures, helper overload events, and exporter lag incidents.

---

## Scenario 1 — Tracing Exporter Failed

**Log pattern:** `[BASE:TracingExporterFailed]`

### Symptoms
- Base tracing exporter fails to export spans.
- `[BASE:TracingExporterFailed]` emitted with `exporter_id`, `endpoint`, `error_code`, and `dropped_spans` fields.
- Distributed traces may have gaps; trace completeness may be below SLO.

### Diagnosis
1. Confirm tracing exporter failures:
   ```
   grep '\[BASE:TracingExporterFailed\]' /var/log/themisdb/base.log
   ```
2. Identify `exporter_id` and `endpoint`.
3. Test exporter endpoint connectivity:
   ```
   curl -v <endpoint>/v1/traces
   ```
4. Review `base_tracing_exporter_failure_total` metric.

### Remediation
1. Restore exporter endpoint connectivity.
2. Increase retry count: `base.tracing.exporter_retry_max`.
3. If endpoint is permanently unavailable: update endpoint config and restart exporter.
4. Confirm `[BASE:TracingExporterFailed]` stops after recovery.

### Escalation
Escalate to the infrastructure team if tracing endpoint is unavailable for more than
5 minutes.

---

## Scenario 2 — Core Init Failed

**Log pattern:** `[BASE:CoreInitFailed]`

### Symptoms
- Core module initialization fails at startup or during hot reload.
- `[BASE:CoreInitFailed]` emitted with `module_name`, `init_stage`, and `error_code` fields.
- Module may be in degraded state; dependent services may fail.

### Diagnosis
1. Confirm core init failures:
   ```
   grep '\[BASE:CoreInitFailed\]' /var/log/themisdb/base.log
   ```
2. Identify `module_name`, `init_stage` (dependency_resolution, sandbox_init, registry_load).
3. Check dependency graph for missing or circular dependencies:
   ```
   themisdb-admin base show-dependencies --module <module_name>
   ```
4. Review `base_core_init_failure_total` metric.

### Remediation
1. For dependency resolution failures: fix missing dependency or add to registry.
2. For sandbox init failures: check wasm runtime availability and fuel-budget config.
3. For registry load failures: validate module manifest and ABI version.
4. Restart module: `themisdb-admin base reload --module <module_name>`.
5. Confirm module transitions to INITIALIZED state.

### Escalation
Escalate to the platform team if core init fails repeatedly for a critical module.

---

## Scenario 3 — Helper Overload

**Log pattern:** `[BASE:HelperOverload]`

### Symptoms
- Base helper dispatch queue exceeds capacity.
- `[BASE:HelperOverload]` emitted with `helper_name`, `queue_depth`, and `latency_ms` fields.
- Downstream consumers of base helpers may experience elevated latency.

### Diagnosis
1. Confirm helper overload:
   ```
   grep '\[BASE:HelperOverload\]' /var/log/themisdb/base.log
   ```
2. Identify `helper_name` and `queue_depth`.
3. Check helper thread pool utilization.
4. Review `base_helper_dispatch_latency_p99_ms` metric.

### Remediation
1. Increase helper thread pool capacity: `base.helper.max_threads`.
2. Apply dispatch rate limiting to prevent thundering herd.
3. Identify and optimize the highest-latency helper call path.
4. Confirm overload condition clears.

### Escalation
Escalate to the platform team if helper queue depth remains above high-water mark for
more than 2 minutes.

---

## Scenario 4 — Exporter Lag

**Log pattern:** `[BASE:ExporterLag]`

### Symptoms
- Tracing exporter experiences persistent lag; spans are not exported in near real time.
- `[BASE:ExporterLag]` emitted with `exporter_id`, `lag_ms`, `buffer_depth`, and `export_rate_per_sec` fields.
- Trace data may be available with significant delay; SLO for trace freshness may be breached.

### Diagnosis
1. Confirm exporter lag:
   ```
   grep '\[BASE:ExporterLag\]' /var/log/themisdb/base.log
   ```
2. Identify `lag_ms` and `buffer_depth`.
3. Compare `export_rate_per_sec` against span production rate.
4. Review `base_exporter_lag_ms` metric.

### Remediation
1. Increase export batch size: `base.tracing.exporter_batch_size`.
2. Reduce export interval: `base.tracing.exporter_interval_ms`.
3. Scale out exporter instances if single-instance throughput is insufficient.
4. Confirm lag returns below SLO threshold.

### Escalation
Escalate to the infrastructure team if lag exceeds 10 seconds for more than 1 minute.

---

## Scenario 5 — Wasm Sandbox Degradation During Core Init Under Load

**Log pattern:** `[BASE:CoreInitFailed]` with `init_stage=sandbox_init`

### Symptoms
- Wasm sandbox fails to initialize during high-concurrency core init.
- Sandbox fuel-budget may be exhausted or host-function allowlist may reject calls.
- `base_sandbox_init_failure_total` metric elevated.

### Diagnosis
1. Confirm sandbox init failures:
   ```
   grep 'sandbox_init' /var/log/themisdb/base.log | head -20
   ```
2. Check fuel-budget configuration:
   ```
   themisdb-admin base show-sandbox-config
   ```
3. Review sandbox host-function allowlist violations.

### Remediation
1. Increase fuel budget: `base.sandbox.fuel_budget`.
2. Add required host functions to allowlist: `base.sandbox.host_function_allowlist`.
3. Use validation-only mode for non-critical modules: `base.sandbox.mode=validation_only`.
4. Confirm sandbox initializes successfully.

### Escalation
Escalate to the platform team if sandbox fuel budget changes require security review
(any changes to host-function allowlist require security team sign-off).
