# RUNBOOK: Toolbox — Triage & Recovery

**Author:** ThemisDB Contributors
**Created:** 2026-09-16
**Last Updated:** 2026-09-16
**Status:** active

**Audience:** Database Operators, SREs, Ingestion Orchestration Team Lead
**Purpose:** Triage and recover from toolbox routing failures, stream-bridge stalls, extraction timeouts, and content-bridge errors
**Severity:** High (toolbox failures halt extraction orchestration and content bridging pipelines)
**Estimated Duration:** 5 min – 2 hours (depending on failure class)

---

## Overview

This runbook guides operators through diagnosing and recovering from failures in the ThemisDB toolbox module (`src/toolbox/`). The module has four primary surfaces:

1. **Ingestion orchestration** — tool dispatch, composite routing, extraction pipeline (`ingestion_toolbox.cpp`)
2. **Content bridge** — graph/vector write, bridge failure metrics, null-check guards (`content_toolbox_bridge.cpp`)
3. **Registry / bootstrap** — tool registry initialization, reset semantics, dependency injection (`toolbox_registry.cpp`)
4. **Text helpers** — text chunking, normalisation, quality scoring, language detection, fingerprinting

**Key Principles:**
- Routing is fail-closed; unknown tools return an explicit `RoutingFailed` error, not a silent no-op
- Bridge sinks validate null inputs; null content returns a descriptive error message, not a panic
- The extraction pipeline tracks empty results explicitly via `extract_empty_results` counter
- Prometheus metrics are instrumented across all four execution planes (EX-*, BR-*, REG-*, HLP-*)

---

## Prerequisites Checklist

Before beginning any intervention, verify:

- [ ] Access to server logs with `[TOOLBOX:*]` tag filtering
- [ ] Prometheus/Grafana dashboard showing `toolbox_*` metrics
- [ ] Knowledge of active tool registry configuration and bridge sink endpoints
- [ ] Content fingerprinter and language detector state

---

## Failure Scenarios

---

### Scenario 1: Routing Failure

**Symptoms:**
- Log pattern: `[TOOLBOX:RoutingFailed] tool=<name> reason=<reason>`
- Extraction requests returning error; tool registry lookup failing
- `toolbox_extraction_failures_total{plane="EX"}` counter rising

**Log patterns:**
```
[TOOLBOX:RoutingFailed] tool=<name> reason=unknown_tool
[TOOLBOX:RoutingFailed] tool=<name> reason=tool_dependency_unavailable dep=<dep>
[TOOLBOX:RoutingFailed] tool=<name> reason=registry_not_initialized
```

#### Step 1: Identify Failing Tools
```bash
grep '\[TOOLBOX:RoutingFailed\]' /var/log/themisdb/themisdb.log | \
  grep -oP 'tool=\S+' | sort | uniq -c | sort -rn | head -10
```

#### Step 2: Check Tool Registry
```bash
themis-admin toolbox registry-status
themis-admin toolbox tool-list
```

#### Step 3: Re-initialize Registry
```bash
themis-admin toolbox registry-reset
themis-admin toolbox registry-init
# Verify: log should show [TOOLBOX:RegistryInitialized] within 5 s
```

#### Step 4: Register Missing Tool
```bash
themis-admin toolbox register-tool --tool-name <name> --config /etc/themisdb/tools/<name>.json
```

---

### Scenario 2: Stream Bridge Stall

**Symptoms:**
- Log pattern: `[TOOLBOX:StreamBridgeStall] bridge_id=<id> blocked_ms=<ms>`
- Content not flowing from extraction output to graph/vector sinks
- `toolbox_bridge_failures_total` counter rising; bridge latency `toolbox_bridge_latency_us` high

**Log patterns:**
```
[TOOLBOX:StreamBridgeStall] bridge_id=<id> blocked_ms=5200 threshold_ms=3000
[TOOLBOX:StreamBridgeStall] bridge_id=<id> reason=graph_sink_backpressure queue_depth=<N>
[TOOLBOX:StreamBridgeStall] bridge_id=<id> reason=vector_sink_unavailable
```

#### Step 1: Identify Stalled Bridges
```bash
grep '\[TOOLBOX:StreamBridgeStall\]' /var/log/themisdb/themisdb.log | tail -20
query-metrics --metric toolbox_bridge_latency_us --label bridge_id --range 5m
```

#### Step 2: Flush Bridge Output Queue
```bash
themis-admin toolbox bridge-flush --bridge-id <bridge_id>
```

#### Step 3: Restart Downstream Sinks
```bash
# Restart graph sink
themis-admin graph restart-write-sink
# Restart vector sink
themis-admin vector restart-write-sink
```

#### Step 4: Reduce Bridge Concurrency (Temporary)
```bash
themis-admin config set toolbox.bridge.max_concurrent_writes 4
themis-admin toolbox reload-config
```

---

### Scenario 3: Extraction Timeout

**Symptoms:**
- Log pattern: `[TOOLBOX:ExtractionTimeout] doc_id=<id> elapsed_ms=<ms>`
- Document extraction returning empty results or timing out
- `toolbox_extract_empty_results_total` counter rising; p99 latency above SLO

**Log patterns:**
```
[TOOLBOX:ExtractionTimeout] doc_id=<id> elapsed_ms=8500 threshold_ms=5000
[TOOLBOX:ExtractionTimeout] doc_id=<id> reason=chunker_timeout content_size_kb=<N>
[TOOLBOX:ExtractionTimeout] doc_id=<id> reason=language_detector_stall
```

#### Step 1: Identify Timeout Pattern
```bash
grep '\[TOOLBOX:ExtractionTimeout\]' /var/log/themisdb/themisdb.log | tail -20
```

#### Step 2: Check Content Size
Large documents can cause chunker timeouts:
```bash
# Check if timeouts correlate with large docs
grep '\[TOOLBOX:ExtractionTimeout\]' /var/log/themisdb/themisdb.log | \
  grep 'content_size_kb' | sort -t= -k5 -rn | head -10
```

#### Step 3: Adjust Chunker Configuration
```bash
themis-admin config set toolbox.chunker.max_chunk_size_tokens 512
themis-admin config set toolbox.chunker.timeout_ms 3000
themis-admin toolbox reload-config
```

#### Step 4: Skip Language Detection for Timeout Documents
```bash
themis-admin config set toolbox.language_detector.timeout_ms 500
themis-admin config set toolbox.language_detector.fallback_language "en"
themis-admin toolbox reload-config
```

---

### Scenario 4: Content Bridge Error

**Symptoms:**
- Log pattern: `[TOOLBOX:ContentBridgeError] doc_id=<id> sink=<sink> reason=<reason>`
- Graph or vector writes failing; bridge error metrics rising
- `toolbox_bridge_failures_total{sink="graph"}` or `{sink="vector"}` non-zero

**Log patterns:**
```
[TOOLBOX:ContentBridgeError] doc_id=<id> sink=graph reason=null_content_payload
[TOOLBOX:ContentBridgeError] doc_id=<id> sink=vector reason=embedding_dimension_mismatch
[TOOLBOX:ContentBridgeError] doc_id=<id> sink=graph reason=write_timeout
```

#### Step 1: Identify Error Type
```bash
grep '\[TOOLBOX:ContentBridgeError\]' /var/log/themisdb/themisdb.log | tail -20
query-metrics --metric toolbox_bridge_failures_total --label reason --range 5m
```

#### Step 2: Address by Reason

**null_content_payload:** Upstream extraction returned empty content. Check extraction pipeline:
```bash
themis-admin toolbox extraction-audit --doc-id <id>
```

**embedding_dimension_mismatch:** Vector store schema change:
```bash
themis-admin vector schema-status
# If dimension changed, re-index:
themis-admin toolbox reindex-doc --doc-id <id>
```

**write_timeout:** Sink overloaded:
```bash
themis-admin toolbox set-bridge-timeout --sink <sink> --timeout-ms 10000
```

---

### Scenario 5: Registry Misuse / Initialization Error

**Symptoms:**
- Log pattern: `[TOOLBOX:RegistryMisuse] context=<ctx> reason=<reason>`
- Module start-up fails; tools return `not_registered` errors
- `toolbox_registry_misuse_total` counter non-zero at startup

**Log patterns:**
```
[TOOLBOX:RegistryMisuse] context=bootstrap reason=double_init
[TOOLBOX:RegistryMisuse] context=tool_dispatch reason=registry_not_ready
```

#### Step 1: Check Registry State
```bash
grep '\[TOOLBOX:RegistryMisuse\]' /var/log/themisdb/themisdb.log | tail -20
themis-admin toolbox registry-status
```

#### Step 2: Reset and Re-initialize Registry
```bash
themis-admin toolbox registry-reset
themis-admin service restart toolbox
```

---

## Alert → Runbook Mapping

| Alert Name | Log Pattern | Runbook Scenario |
|------------|-------------|------------------|
| `toolbox_routing_failed` | `[TOOLBOX:RoutingFailed]` | Scenario 1 |
| `toolbox_stream_bridge_stall` | `[TOOLBOX:StreamBridgeStall]` | Scenario 2 |
| `toolbox_extraction_timeout` | `[TOOLBOX:ExtractionTimeout]` | Scenario 3 |
| `toolbox_content_bridge_error` | `[TOOLBOX:ContentBridgeError]` | Scenario 4 |
| `toolbox_registry_misuse` | `[TOOLBOX:RegistryMisuse]` | Scenario 5 |

---

## Escalation Path

1. **L1 (Operator):** Apply runbook steps; resolve within 30 min
2. **L2 (SRE):** Escalate if extraction timeout rate > 5% or bridge stall > 15 min
3. **L3 (Ingestion Platform):** Engage for systematic bridge schema or language-detector issues

---

## Related Documentation

- `src/toolbox/ROADMAP.md` — Wave D operability items
- `src/toolbox/PRODUCTION_REQUIREMENTS.md` — Deployment and operational runbooks
- `include/toolbox/ingestion_toolbox.h` — Public API
- `tests/integration/test_toolbox_soak.cpp` — Wave D soak tests
- `tests/toolbox/test_toolbox_highcardinality_stress.cpp` — Wave D stress tests
- `benchmarks/toolbox/bench_toolbox_dedicated_gates.cpp` — Dedicated performance gates
