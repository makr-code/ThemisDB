# RUNBOOK: RPC/gRPC — Triage & Recovery

**Author:** ThemisDB Contributors
**Created:** 2026-09-16
**Last Updated:** 2026-09-16
**Status:** active

**Audience:** Database Operators, SREs, Transport Platform Team Lead
**Purpose:** Triage and recover from gRPC service unavailability, stream adapter failures, reload timeouts, and transport errors
**Severity:** High (gRPC failures degrade WAL-apply throughput and distributed coordination paths)
**Estimated Duration:** 5 min – 2 hours (depending on failure class)

---

## Overview

This runbook guides operators through diagnosing and recovering from failures in the ThemisDB rpc_grpc module (`src/rpc_grpc/`). The module has five primary surfaces:

1. **Server lifecycle** — start/stop/restart with precondition checks (`GrpcPlugin::start()`, `stop()`)
2. **TLS/mTLS credentials** — credential validation, atomic reload (`GrpcPlugin::reloadTls()`)
3. **Service registration** — idempotent service register/deregister with bounds (`registerService()`)
4. **Stream adapters** — bidirectional streaming with frame ordering (`RpcStreamAdapter`)
5. **Method observability** — per-method latency and error code tracking

**Key Principles:**
- All credential reloads are atomic; a failed reload leaves the previous credentials active (fail-closed)
- Service registration is idempotent; re-registering an existing service name is a no-op
- Transport errors (status != OK) are logged with `[GRPC:TransportError]` and structured error codes `[RPC-Exxx]`
- Stream adapters maintain frame ordering; out-of-order delivery is a transport-level error, not a silent skip

---

## Prerequisites Checklist

Before beginning any intervention, verify:

- [ ] Access to server logs with `[GRPC:*]` tag filtering
- [ ] Prometheus/Grafana dashboard showing `rpc_grpc_*` metrics
- [ ] TLS certificate paths and validity (check `GrpcPlugin.tls_cert_path`, `tls_key_path`)
- [ ] Service registry state accessible via admin API or log scan

---

## Failure Scenarios

---

### Scenario 1: Service Unavailable

**Symptoms:**
- Log pattern: `[GRPC:ServiceUnavailable] service=<name> reason=<reason>`
- gRPC clients receive `UNAVAILABLE` status
- `rpc_grpc_service_unavailable_total` counter rising

**Log patterns:**
```
[GRPC:ServiceUnavailable] service=<name> reason=server_not_started
[GRPC:ServiceUnavailable] service=<name> reason=port_in_use port=<port>
[GRPC:ServiceUnavailable] service=<name> reason=max_services_exceeded limit=<N>
```

#### Step 1: Check Server State
```bash
grep '\[GRPC:ServiceUnavailable\]' /var/log/themisdb/themisdb.log | tail -20
themis-admin grpc server-status
```

#### Step 2: Restart gRPC Server
```bash
themis-admin grpc restart --grace-period 10s
# Verify: log should show [GRPC:ServerStarted] within 15 s
```

#### Step 3: Check Port Conflicts
```bash
ss -tlnp | grep <grpc_port>
# Release conflicting process if safe
```

---

### Scenario 2: Stream Adapter Failure

**Symptoms:**
- Log pattern: `[GRPC:StreamAdapterFailed] stream_id=<id> reason=<reason>`
- Bidirectional stream clients disconnect or receive `INTERNAL` status
- `rpc_grpc_stream_adapter_failures_total` counter rising

**Log patterns:**
```
[GRPC:StreamAdapterFailed] stream_id=<id> reason=frame_ordering_violation
[GRPC:StreamAdapterFailed] stream_id=<id> reason=send_buffer_overflow
[GRPC:StreamAdapterFailed] stream_id=<id> reason=peer_closed_unexpectedly
```

#### Step 1: Identify Affected Streams
```bash
grep '\[GRPC:StreamAdapterFailed\]' /var/log/themisdb/themisdb.log | \
  grep -oP 'stream_id=\S+' | sort | uniq -c | sort -rn | head -10
```

#### Step 2: Reset Stream Pool
```bash
themis-admin grpc reset-stream-pool --drain-timeout 30s
```

#### Step 3: Reduce Stream Concurrency
If `send_buffer_overflow` is the cause:
```bash
themis-admin config set grpc.stream_adapter.max_concurrent_streams 128
themis-admin grpc reload-config
```

---

### Scenario 3: Reload Timeout

**Symptoms:**
- Log pattern: `[GRPC:ReloadTimeout] credential_path=<path> elapsed_ms=<ms>`
- TLS credential reload blocked; new certificates not yet active
- `rpc_grpc_reload_timeout_total` counter rising

**Log patterns:**
```
[GRPC:ReloadTimeout] credential_path=/tls/server.crt elapsed_ms=5200 threshold_ms=3000
[GRPC:ReloadTimeout] reason=lock_contention waiters=<N>
```

#### Step 1: Check Reload State
```bash
grep '\[GRPC:ReloadTimeout\]' /var/log/themisdb/themisdb.log | tail -20

# Check current certificate validity
openssl x509 -in /tls/server.crt -noout -dates
```

#### Step 2: Force Credential Reload
```bash
themis-admin grpc reload-tls --force
# Verify: log should show [GRPC:CredentialReloaded] within 5 s
```

#### Step 3: Extend Reload Timeout (if environment is slow)
```bash
themis-admin config set grpc.tls_reload_timeout_ms 10000
themis-admin grpc reload-config
```

---

### Scenario 4: Transport Error Storm

**Symptoms:**
- Log pattern: `[GRPC:TransportError] code=<code> method=<method>`
- High rate of non-OK gRPC status codes from server
- `rpc_grpc_transport_errors_total` counter with label `status_code` non-zero

**Log patterns:**
```
[GRPC:TransportError] code=UNAVAILABLE method=/themis.WalService/Apply
[GRPC:TransportError] code=RESOURCE_EXHAUSTED method=/themis.QueryService/Execute
[GRPC:TransportError] code=DEADLINE_EXCEEDED method=/themis.WireService/Stream
```

#### Step 1: Identify Top Error Codes
```bash
grep '\[GRPC:TransportError\]' /var/log/themisdb/themisdb.log | \
  grep -oP 'code=\S+' | sort | uniq -c | sort -rn | head -10
```

#### Step 2: Address by Status Code

- **UNAVAILABLE:** Restart server (see Scenario 1)
- **RESOURCE_EXHAUSTED:** Increase server thread pool:
  ```bash
  themis-admin config set grpc.server.min_pollers 4
  themis-admin config set grpc.server.max_pollers 16
  themis-admin grpc reload-config
  ```
- **DEADLINE_EXCEEDED:** Increase client deadline or reduce request payload size

---

### Scenario 5: Service Registration Bounds Exceeded

**Symptoms:**
- Log pattern: `[GRPC:RegistrationBoundsExceeded] service=<name> registered=<N> limit=<L>`
- New services fail to register; existing services unaffected
- `rpc_grpc_registration_rejected_total` counter rising

**Log patterns:**
```
[GRPC:RegistrationBoundsExceeded] service=<name> registered=256 limit=256
```

#### Step 1: Check Registration Count
```bash
themis-admin grpc service-list | wc -l
grep '\[GRPC:RegistrationBoundsExceeded\]' /var/log/themisdb/themisdb.log | tail -10
```

#### Step 2: Deregister Unused Services
```bash
# List services not called in last 1 hour
themis-admin grpc service-list --idle-since 1h

# Deregister stale services
themis-admin grpc deregister-service --service-name <stale_service>
```

#### Step 3: Increase Limit (if justified)
```bash
themis-admin config set grpc.service_registry.max_services 512
themis-admin grpc reload-config
```

---

## Alert → Runbook Mapping

| Alert Name | Log Pattern | Runbook Scenario |
|------------|-------------|------------------|
| `grpc_service_unavailable` | `[GRPC:ServiceUnavailable]` | Scenario 1 |
| `grpc_stream_adapter_failed` | `[GRPC:StreamAdapterFailed]` | Scenario 2 |
| `grpc_reload_timeout` | `[GRPC:ReloadTimeout]` | Scenario 3 |
| `grpc_transport_error_rate_high` | `[GRPC:TransportError]` | Scenario 4 |
| `grpc_registration_bounds_exceeded` | `[GRPC:RegistrationBoundsExceeded]` | Scenario 5 |

---

## Escalation Path

1. **L1 (Operator):** Apply runbook steps; resolve within 30 min
2. **L2 (SRE):** Escalate if transport error rate remains > 1% after 15 min
3. **L3 (Transport Platform):** Engage for TLS credential or stream-adapter protocol issues

---

## Related Documentation

- `src/rpc_grpc/ROADMAP.md` — Wave D operability items
- `include/rpc_grpc/rpc_grpc_api_contract.h` — Frozen v1.x contract
- `src/rpc_grpc/OPERATOR_RUNBOOK.md` — Deployment and configuration guide
- `tests/integration/test_rpc_grpc_soak.cpp` — Wave D soak tests
- `tests/rpc_grpc/test_rpc_grpc_highcardinality_stress.cpp` — Wave D stress tests
- `benchmarks/rpc_grpc/bench_rpc_grpc_dedicated_gates.cpp` — Dedicated performance gates
