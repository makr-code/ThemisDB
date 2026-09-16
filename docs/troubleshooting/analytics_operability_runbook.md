# Analytics Operability Runbook

## Scope

This runbook covers operator-critical analytics incidents for streaming,
distributed coordination, export, and serving fail-closed paths.

## Required Correlation Fields

Use these fields first when triaging incidents:

- `operation_id`
- `correlation_id`
- `failure_class`
- `operator_hints`

## Incident Classes

### 1. Backpressure or high-cardinality saturation

- Symptoms:
  - rising `partition_keys_rejected`
  - rising `windows_evicted`
  - `failure_class=input_validation` or bounded-window rejections in logs
- Actions:
  - reduce cardinality at ingestion boundaries
  - lower tenant fan-in per window
  - verify `max_distinct_partition_keys`, `max_open_windows`, and `max_records_per_window`

### 2. Distributed fail-closed or open circuit breaker

- Symptoms:
  - `failure_class=partial_failure`
  - operator hints mention `Fail-closed` or `circuit breaker is OPEN`
  - reduced `successful_shards / total_shards`
- Actions:
  - inspect shard reachability and shard-local logs
  - confirm whether `allow_partial_results=false` is expected for the environment
  - re-run only after shard health is restored

### 3. Export failure or policy rejection

- Symptoms:
  - `failure_class=io_failure`, `policy_rejected`, or `timeout`
  - operator hints mention destination path or bounded execution limits
- Actions:
  - verify destination path exists and is writable
  - confirm filesystem capacity
  - review export `BoundedExecutionPolicy` limits before retrying

### 4. Serving validation, timeout, or dependency failures

- Symptoms:
  - `failure_class=input_validation`, `timeout`, `dependency_unavailable`, or `security_policy`
  - operator hints mention model availability, TLS, or tensor validation
- Actions:
  - validate feature extraction and tensor shape contracts
  - confirm model artifact integrity and endpoint TLS configuration
  - review serving timeout and concurrency limits

### 5. Representative-hardware baseline regression

- Symptoms:
  - baseline comparison report status `warn` or `fail`
  - p95/p99 drift beyond the module threshold
- Actions:
  - compare against `/home/runner/work/ThemisDB/ThemisDB/benchmarks/baselines/analytics/representative_hardware_manifest.json`
  - reproduce on the same hardware profile before widening thresholds
  - only update baselines after a reviewed, intentional performance change
