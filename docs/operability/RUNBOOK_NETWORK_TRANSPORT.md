# RUNBOOK: Network Transport — Triage & Recovery

**Author:** ThemisDB Contributors
**Created:** 2026-09-16
**Last Updated:** 2026-09-16
**Status:** active

**Audience:** Database Operators, SREs, Network/Transport Team Lead
**Purpose:** Triage and recover from network transport failures, protocol frame errors, rate-limit breaches, circuit-breaker trips, WebSocket session expiry, and gRPC fallback activations
**Severity:** High (affects all client connectivity — data path and control plane)
**Estimated Duration:** 5 min – 2 hours (depending on failure class)

---

## Overview

This runbook covers the ThemisDB network transport layer (`src/network/`). The transport layer has six primary surfaces:

1. **TCP wire protocol** — Binary framed connections (`WireProtocolServer`, `wire_protocol_server.cpp`)
2. **WebSocket transport** — HTTP upgrade + WS frame dispatch (`THEMIS_ENABLE_WEBSOCKET`)
3. **UDP path** — Low-latency best-effort datagrams (`udp_server.cpp`)
4. **QUIC/HTTP3** — Production-default for external API endpoints (`THEMIS_ENABLE_HTTP3`)
5. **gRPC transport** — RPC fallback path (`EnvoyXDSClient`, gRPC stubs)
6. **Adaptive circuit breaker** — Per-error-class protection (`AdaptiveCircuitBreaker`)

**Key Principles:**
- All transport paths are fail-closed: invalid frames and unauthenticated requests are rejected before opcode dispatch
- Circuit breakers protect all outbound paths; a tripped breaker is expected under persistent partition — do not bypass
- Connection draining (`DRAINING` state) must be respected; in-flight requests complete, new requests are rejected with `SERVER_DRAINING`
- Rate-limiting and backpressure failures are transient; connection-closing errors (`FRAME_INVALID`, `AUTH_TIMEOUT`, `TRANSPORT_CLOSED`) are not

---

## Prerequisites Checklist

Before beginning any intervention, verify:

- [ ] At least one transport path is reachable (check `themisdb-admin network status`)
- [ ] Access to server logs with `[NETWORK:*]`, `[FRAME:*]`, `[AUTH:*]`, `[RATELIMIT:*]` tag filtering
- [ ] Prometheus/Grafana dashboard showing network metrics (`network_frame_*`, `network_connection_*`, `network_circuit_breaker_*`)
- [ ] Knowledge of which transport paths are active (`THEMIS_ENABLE_WEBSOCKET`, `THEMIS_ENABLE_HTTP3`, gRPC endpoint config)
- [ ] Circuit breaker state accessible via admin API or log scan

---

## Failure Scenarios

---

### Scenario 1: Transport Connection Failure (TCP/WS/QUIC — Connection Refused or TLS Failure)

**Symptoms:**
- Clients receive `ECONNREFUSED` or TLS handshake errors
- Server logs contain `[NETWORK:AcceptError]` or `[NETWORK:TLSHandshakeFailed]` tags
- Connection accept rate drops to zero
- `network_connection_accept_total_rate` metric flat-lines

**Log patterns to search for:**
```
[NETWORK:AcceptError] accept() failed: connection refused (port=<port>)
[NETWORK:TLSHandshakeFailed] TLS handshake error: <reason> peer=<ip>
[NETWORK:BindFailed] Failed to bind TCP listener on <addr>:<port>
```

#### Step 1: Confirm Which Transport is Failing

```bash
# Check accept errors across all transports
grep '\[NETWORK:AcceptError\]\|\[NETWORK:TLSHandshakeFailed\]\|\[NETWORK:BindFailed\]' \
  /var/log/themisdb/themisdb.log | tail -20

# Check which port is involved
grep 'AcceptError\|BindFailed' /var/log/themisdb/themisdb.log | \
  grep -oP 'port=\K[0-9]+' | sort | uniq -c
```

#### Step 2: Diagnose by Transport Type

| Transport | Port (default) | TLS Config Location | Common Cause |
|-----------|---------------|--------------------|----|
| TCP wire | 7474 | `config.tls_cert_path`, `config.tls_key_path` | Certificate expiry, port conflict |
| WebSocket | 7475 | Same TLS cert as TCP | HTTP upgrade failure, wrong `Host` header |
| QUIC/HTTP3 | 443 (UDP) | `THEMIS_QUIC_CERT`, `THEMIS_QUIC_KEY` | UDP blocked by firewall, cert rotation lag |
| gRPC | 50051 | gRPC TLS creds | Envoy proxy failure, mTLS cert mismatch |

```bash
# Check TLS certificate expiry
openssl x509 -in /path/to/tls.crt -noout -dates

# Check if port is bound
ss -tlnp | grep -E '7474|7475|50051'

# Check for firewall blocks on UDP (QUIC)
iptables -L INPUT -n | grep -E '443|QUIC'
```

#### Step 3: Restart the Affected Transport Listener

```bash
# Soft restart of the network listener (does not disconnect active sessions)
themisdb-admin network restart-listener --transport tcp

# For WebSocket transport
themisdb-admin network restart-listener --transport websocket

# For QUIC — requires the QUIC listener to be re-initialized
themisdb-admin network restart-listener --transport quic
```

#### Step 4: Validate Recovery

```bash
# Confirm accept rate recovers
query-metrics --metric network_connection_accept_total_rate --range 5m --window 30s

# Confirm no new AcceptError logs
grep '\[NETWORK:AcceptError\]' /var/log/themisdb/themisdb.log | \
  tail -5 | awk '{print $1, $2}'
```

**Decision Point:**
- ✅ **Recovery confirmed:** Accept rate restored, no new AcceptError logs → incident closed
- ❌ **TLS cert expired:** Rotate certificate, reload listener (see cert rotation runbook)
- ❌ **Port conflict:** Identify conflicting process, reassign port in config, restart

---

### Scenario 2: Protocol Frame Validation Error Spike (FRAME_INVALID, Magic Mismatch)

**Symptoms:**
- `network_frame_invalid_total_rate` metric rising steeply
- Connections being closed immediately after connect (no auth handshake)
- Logs contain `[FRAME:Invalid]` or `[FRAME:MagicMismatch]` tags
- Possible signs: port scanner, misconfigured client, protocol version mismatch

**Log patterns:**
```
[FRAME:Invalid] session=<id> Invalid frame magic: expected=0x544D4442 received=<hex>
[FRAME:Oversized] session=<id> Payload size <N> exceeds maximum <M> bytes
[FRAME:MagicMismatch] session=<id> peer=<ip> connection closed immediately
```

#### Step 1: Identify the Source of Invalid Frames

```bash
# Recent FRAME_INVALID events with peer IP
grep '\[FRAME:Invalid\]\|\[FRAME:MagicMismatch\]' /var/log/themisdb/themisdb.log | \
  grep -oP 'peer=\K[\d.]+' | sort | uniq -c | sort -rn | head -10

# Check for FRAME_OVERSIZED (potential DoS probe)
grep '\[FRAME:Oversized\]' /var/log/themisdb/themisdb.log | tail -20
```

#### Step 2: Diagnose by Pattern

| Pattern | Likely Cause | Action |
|---------|-------------|--------|
| Single peer, many FRAME_INVALID | Port scanner or misconfigured client | Block at firewall, notify client team |
| Many peers, FRAME_INVALID spike | Protocol version mismatch post-deploy | Roll back or publish new client SDK |
| FRAME_OVERSIZED from single peer | DoS / fuzzing probe | Rate-limit or block peer IP |
| FRAME_OVERSIZED from many peers | Client bug (payload size calculation) | Notify client SDK team |

#### Step 3: Block Abusive Peers (If Confirmed Attack)

```bash
# Block a specific IP at the network layer
themisdb-admin network block-peer --ip <peer-ip> --duration 1h

# Verify block is active
themisdb-admin network list-blocked-peers
```

#### Step 4: Verify Frame Error Rate Returns to Baseline

```bash
# FRAME_INVALID rate (gate: < 0.1% of total frames)
query-metrics --metric network_frame_invalid_total_rate,network_frame_total_rate \
  --range 30m --window 1m
```

**Decision Point:**
- ✅ **Rate returns to < 0.1%:** False alarm or fixed client → incident closed
- ⚠ **Sustained from single peer:** Rate-limit / block peer; alert security
- ❌ **Broad spike post-deploy:** Protocol version regression → rollback deployment

---

### Scenario 3: Rate-Limit / Backpressure Breach Under DDoS-like Conditions

**Symptoms:**
- `network_rate_limit_rejections_total_rate` metric rising sharply
- `network_backpressure_queue_depth` approaching `kMaxQueueDepth`
- Clients receiving `RATE_LIMITED` or `BACKPRESSURE_EXCEEDED` error codes
- CPU usage elevated; I/O thread saturation

**Log patterns:**
```
[RATELIMIT:Exceeded] session=<id> peer=<ip> rate_limit=<N>/s current_rate=<M>/s
[BACKPRESSURE:QueueFull] worker_pool_queue_depth=<N> threshold=<M> — shedding request
[RATELIMIT:GlobalThrottle] Global rate limit active: requests_per_sec=<N> limit=<M>
```

#### Step 1: Confirm Rate-Limit vs. Backpressure

```bash
# Rate-limit rejections (per-connection)
grep '\[RATELIMIT:Exceeded\]' /var/log/themisdb/themisdb.log | \
  grep -oP 'peer=\K[\d.]+' | sort | uniq -c | sort -rn | head -10

# Backpressure queue depth
grep '\[BACKPRESSURE:' /var/log/themisdb/themisdb.log | tail -20

# Global rate limit activation (server-wide)
grep '\[RATELIMIT:GlobalThrottle\]' /var/log/themisdb/themisdb.log | tail -10
```

#### Step 2: Assess Scope

```bash
# Total request rate vs. limit
query-metrics \
  --metric network_requests_per_sec,network_rate_limit_threshold \
  --range 30m --window 1m

# Worker pool saturation
query-metrics --metric network_worker_pool_queue_depth --range 30m --window 30s
```

#### Step 3: Mitigations

**Per-connection rate limit breach (single client):**
```bash
# Reduce per-connection limit for the offending peer (temporary)
themisdb-admin network set-connection-rate-limit --session-id <id> --limit 100

# Or drop the connection entirely
themisdb-admin network drop-session --session-id <id>
```

**Global backpressure (server under load):**
```bash
# Scale up worker pool (live — takes effect immediately)
themisdb-admin config set network.worker_pool_threads 32 --apply-live

# Increase queue depth limit (use with caution — increases memory pressure)
themisdb-admin config set network.max_worker_queue_depth 2048 --apply-live
```

**DDoS-level ingress:**
```bash
# Enable global rate limiter (hard cap on new connections/sec)
themisdb-admin network enable-global-rate-limit --connections-per-sec 500

# Or activate connection drain to shed load gracefully
themisdb-admin network start-drain --transport tcp --grace-period 30s
```

#### Step 4: Verify Recovery

```bash
# Rate-limit rejection rate back to baseline
query-metrics --metric network_rate_limit_rejections_total_rate --range 10m --window 30s

# Worker pool queue draining
query-metrics --metric network_worker_pool_queue_depth --range 10m --window 30s
```

**Decision Point:**
- ✅ **Rejections < 1%, queue depth normal:** Recovery confirmed → incident closed
- ⚠ **Single peer causing load:** Block or rate-limit peer; escalate to security if adversarial
- ❌ **Global load sustained:** Invoke DDoS mitigation; coordinate with infra team for capacity

---

### Scenario 4: Circuit Breaker Open (Network Partition / Cascading Failure)

**Symptoms:**
- `network_circuit_breaker_state` metric shows `OPEN` for one or more error classes
- Outbound connections failing immediately (no retry delay)
- Logs contain `[NETWORK:CircuitBreakerOpen]` tags
- `network_circuit_breaker_trips_total` counter incrementing

**Log patterns:**
```
[NETWORK:CircuitBreakerOpen] error_class=TRANSPORT_UNAVAILABLE trips=<N> — rejecting call
[NETWORK:CircuitBreakerHalfOpen] error_class=<class> — probe allowed
[NETWORK:CircuitBreakerClosed] error_class=<class> — probe succeeded, breaker closed
```

#### Step 1: Identify Which Error Class Tripped the Breaker

```bash
# All circuit breaker open events (grouped by error class)
grep '\[NETWORK:CircuitBreakerOpen\]' /var/log/themisdb/themisdb.log | \
  grep -oP 'error_class=\K[A-Z_]+' | sort | uniq -c | sort -rn | head -10

# Trip history
grep 'CircuitBreakerOpen\|CircuitBreakerHalfOpen\|CircuitBreakerClosed' \
  /var/log/themisdb/themisdb.log | tail -30
```

#### Step 2: Diagnose by Error Class

| Error Class | Meaning | Likely Root Cause |
|-------------|---------|-------------------|
| `TRANSPORT_UNAVAILABLE` | Downstream unreachable | Network partition, down service |
| `TRANSPORT_CLOSED` | Connections closing unexpectedly | Remote host crash, FIN flood |
| `AUTH_TIMEOUT` | Auth handshake timed out | Auth backend slow/down |
| `QUORUM_DEGRADED` | Not enough replicas reachable | Storage layer partition |
| `INTERNAL_ERROR` | Server-side panic or assertion | Bug — escalate immediately |

```bash
# Check for downstream connectivity
ping -c 4 <downstream-host>
curl -v https://<downstream-host>:<port>/health

# Check if the issue is specific to a single availability zone
query-metrics --metric network_transport_errors_by_zone --range 30m --window 1m
```

#### Step 3: Force Circuit Breaker State (If Needed)

```bash
# View current breaker states
themisdb-admin network circuit-breaker status --all

# Force half-open (probe) — use only when downstream is confirmed healthy
themisdb-admin network circuit-breaker reset --error-class TRANSPORT_UNAVAILABLE --force-half-open

# Force close (ONLY if you are certain downstream is healthy and the trip was spurious)
themisdb-admin network circuit-breaker close --error-class TRANSPORT_UNAVAILABLE
```

#### Step 4: Validate Recovery

```bash
# Confirm circuit breaker returns to CLOSED
grep '\[NETWORK:CircuitBreakerClosed\]' /var/log/themisdb/themisdb.log | tail -5

# Confirm error rate returns to baseline
query-metrics --metric network_transport_error_rate --range 10m --window 30s
```

**Decision Point:**
- ✅ **Breaker closes, error rate baseline:** Recovery confirmed → incident closed
- ⚠ **Breaker repeatedly trips:** Downstream still unhealthy — investigate downstream service
- ❌ **INTERNAL_ERROR class:** Potential bug — capture stack traces, escalate to core team

---

### Scenario 5: WebSocket Session Expiry / Auth Failure Spike

**Symptoms:**
- `network_ws_session_expired_total_rate` metric rising
- Clients receiving `SESSION_EXPIRED` or `AUTH_REQUIRED` errors on reconnect
- WebSocket connections being closed shortly after upgrade
- Logs contain `[AUTH:SessionExpired]`, `[AUTH:Required]`, or `[AUTH:Revoked]` tags

**Log patterns:**
```
[AUTH:Required] session=<id> opcode=0x20 — AUTH_REQUIRED: no session token
[AUTH:SessionExpired] session=<id> token=<prefix>... — session expired at <ts>
[AUTH:Revoked] session=<id> — session revoked (fail-closed)
[AUTH:Timeout] session=<id> — auth handshake timed out after <N>ms
```

#### Step 1: Classify the Auth Failure Pattern

```bash
# Auth failure breakdown by error type
grep '\[AUTH:' /var/log/themisdb/themisdb.log | \
  grep -oP '\[AUTH:\K[^\]]+' | sort | uniq -c | sort -rn | head -10

# Session expiry rate (by time window)
grep '\[AUTH:SessionExpired\]' /var/log/themisdb/themisdb.log | \
  awk '{print $1, $2}' | cut -d: -f1,2 | sort | uniq -c | tail -20
```

#### Step 2: Action by Error Type

| Error Code | Log Tag | Likely Cause | Action |
|-----------|---------|-------------|--------|
| `AUTH_REQUIRED` | `[AUTH:Required]` | Client not sending token / reconnect without re-auth | Notify client; check SDK auth flow |
| `SESSION_EXPIRED` | `[AUTH:SessionExpired]` | Session TTL too short or clock skew | Extend session TTL or sync clocks |
| `SESSION_REVOKED` | `[AUTH:Revoked]` | Token revoked (security response) | Expected; do not revert if intentional |
| `AUTH_TIMEOUT` | `[AUTH:Timeout]` | Auth backend latency spike | Check auth backend health |
| `SESSION_MALFORMED` | `[AUTH:Malformed]` | Corrupted or forged token | Security event — block client |

```bash
# For AUTH_TIMEOUT: check auth backend latency
query-metrics --metric auth_backend_latency_p99_ms --range 30m --window 1m

# For SESSION_EXPIRED: check current session TTL config
themisdb-admin config get network.session_ttl_seconds

# Extend session TTL (if TTL is too short relative to expected session duration)
themisdb-admin config set network.session_ttl_seconds 3600 --apply-live
```

#### Step 3: Verify Recovery

```bash
# Confirm AUTH_REQUIRED/SessionExpired rate returns to baseline
grep '\[AUTH:SessionExpired\]\|\[AUTH:Required\]' /var/log/themisdb/themisdb.log | \
  awk '{print $1, $2}' | cut -d: -f1,2 | sort | uniq -c | tail -10

# Confirm WS connections stabilise
query-metrics --metric network_ws_active_sessions --range 10m --window 30s
```

**Decision Point:**
- ✅ **Failure rate at baseline, active sessions stable:** Recovery confirmed → incident closed
- ⚠ **SESSION_EXPIRED sustained:** Review TTL config; check NTP/clock skew between nodes
- ❌ **SESSION_MALFORMED or mass AUTH_REQUIRED:** Possible credential leak — initiate security review

---

### Scenario 6: gRPC Transport Fallback Activation

**Symptoms:**
- Logs contain `[GRPC:FallbackActivated]` or `[GRPC:PrimaryFailed]` tags
- `network_grpc_fallback_active` metric becomes 1
- Latency increase on the gRPC path (fallback may use a slower route)
- Envoy xDS subscription interruption

**Log patterns:**
```
[GRPC:PrimaryFailed] primary gRPC endpoint <host>:<port> unreachable — activating fallback
[GRPC:FallbackActivated] fallback_endpoint=<host>:<port> reason=<error>
[GRPC:XDSInterrupted] EnvoyXDSClient subscription interrupted: <error>
[GRPC:ReconnectAttempt] attempt=<N>/5 to primary endpoint <host>:<port>
```

#### Step 1: Confirm Fallback is Active

```bash
# Check fallback activation events
grep '\[GRPC:FallbackActivated\]\|\[GRPC:PrimaryFailed\]' \
  /var/log/themisdb/themisdb.log | tail -10

# Check xDS subscription health
grep '\[GRPC:XDSInterrupted\]\|\[GRPC:XDS\]' /var/log/themisdb/themisdb.log | tail -10

# Confirm metric state
query-metrics --metric network_grpc_fallback_active --range 30m --window 1m
```

#### Step 2: Diagnose gRPC Primary Failure

```bash
# Check gRPC primary endpoint connectivity
grpc-health-probe -addr=<primary-host>:<port> -tls

# Check Envoy proxy health
curl -s http://<envoy-admin>:9901/ready
curl -s http://<envoy-admin>:9901/clusters | grep -A5 '<cluster-name>'

# Check for mTLS certificate issues on the gRPC path
openssl s_client -connect <primary-host>:<port> -servername <sni>
```

#### Step 3: Restore Primary gRPC Endpoint

```bash
# If primary is healthy but ThemisDB thinks it is not (stale failure state):
# Force retry to primary
themisdb-admin network grpc reset-fallback --force-primary-probe

# If Envoy is healthy, restart the xDS subscription
themisdb-admin network grpc restart-xds-subscription

# Verify reconnect attempts in logs
grep '\[GRPC:ReconnectAttempt\]' /var/log/themisdb/themisdb.log | tail -10
```

#### Step 4: Validate Primary Restored

```bash
# Confirm fallback_active drops to 0
query-metrics --metric network_grpc_fallback_active --range 10m --window 30s

# Confirm PrimaryFailed events stop
grep '\[GRPC:PrimaryFailed\]' /var/log/themisdb/themisdb.log | \
  awk '{print $1, $2}' | tail -5
```

**Decision Point:**
- ✅ **Primary restored, fallback_active=0:** Recovery confirmed → incident closed
- ⚠ **Fallback functional, primary still down:** Accept degraded state; escalate to Envoy/infra team
- ❌ **Both primary and fallback fail:** Major outage — invoke incident escalation procedure

---

## Troubleshooting Quick Reference

| Symptom | Log Tag | `[NETWORK:*]` / `[FRAME:*]` Pattern | First Action |
|---------|---------|--------------------------------------|-------------|
| Connection refused | `[NETWORK:AcceptError]` | `AcceptError.*port=` | Check listener binding (Scenario 1) |
| TLS handshake failure | `[NETWORK:TLSHandshakeFailed]` | `TLSHandshakeFailed.*peer=` | Check cert expiry (Scenario 1) |
| Frame magic mismatch | `[FRAME:Invalid]` | `FRAME_INVALID.*magic=` | Identify source IP, check client SDK (Scenario 2) |
| Oversized frame | `[FRAME:Oversized]` | `FRAME_OVERSIZED.*size=` | Block if DoS probe (Scenario 2) |
| Rate limit active | `[RATELIMIT:Exceeded]` | `rate_limit=.*current_rate=` | Check per-connection rate, scale if needed (Scenario 3) |
| Backpressure queue full | `[BACKPRESSURE:QueueFull]` | `worker_pool_queue_depth=` | Scale worker pool (Scenario 3) |
| Circuit breaker open | `[NETWORK:CircuitBreakerOpen]` | `error_class=.*trips=` | Diagnose downstream (Scenario 4) |
| Session expired | `[AUTH:SessionExpired]` | `SessionExpired.*token=` | Check TTL config (Scenario 5) |
| Auth required | `[AUTH:Required]` | `AUTH_REQUIRED.*opcode=` | Check client auth flow (Scenario 5) |
| gRPC fallback active | `[GRPC:FallbackActivated]` | `fallback_endpoint=` | Diagnose primary endpoint (Scenario 6) |
| xDS subscription down | `[GRPC:XDSInterrupted]` | `XDSInterrupted.*error=` | Restart xDS subscription (Scenario 6) |

---

## Evidence & Logging Checklist

After incident resolution, collect:

- [ ] `network_frame_error_rates.json` — FRAME_INVALID / FRAME_OVERSIZED rates pre- and post-incident
- [ ] `network_circuit_breaker_events.log` — Filtered `CircuitBreakerOpen/HalfOpen/Closed` events
- [ ] `network_connection_lifecycle.csv` — Accept, drain, close counts over incident window
- [ ] `network_auth_failure_distribution.json` — `AUTH_REQUIRED` / `SESSION_EXPIRED` / `AUTH_TIMEOUT` counts
- [ ] `network_ratelimit_events.log` — Per-session rate-limit events with peer IPs
- [ ] `network_grpc_fallback_timeline.csv` — Fallback activation and primary restore timestamps

Archive in: `evidence/network-transport-incidents/<date>-<issue-id>/`

---

## Quick Reference: Diagnostic Commands

```bash
# ── Frame validation errors ─────────────────────────────────────────────────
grep '\[FRAME:Invalid\]\|\[FRAME:Oversized\]\|\[FRAME:MagicMismatch\]' \
  /var/log/themisdb/themisdb.log | tail -30

# ── Auth / session failures ─────────────────────────────────────────────────
grep '\[AUTH:' /var/log/themisdb/themisdb.log | tail -30

# ── Rate limit and backpressure ─────────────────────────────────────────────
grep '\[RATELIMIT:\]\|\[BACKPRESSURE:\]' /var/log/themisdb/themisdb.log | tail -20

# ── Circuit breaker events ──────────────────────────────────────────────────
grep 'CircuitBreaker' /var/log/themisdb/themisdb.log | tail -20

# ── gRPC and xDS events ─────────────────────────────────────────────────────
grep '\[GRPC:' /var/log/themisdb/themisdb.log | tail -20

# ── Combined network health check ───────────────────────────────────────────
themisdb-admin network health-check --verbose

# ── Live metric snapshot ─────────────────────────────────────────────────────
query-metrics \
  --metric network_frame_invalid_total_rate,\
network_connection_accept_total_rate,\
network_circuit_breaker_trips_total,\
network_rate_limit_rejections_total_rate,\
network_grpc_fallback_active \
  --range 30m --window 1m
```

---

## Wave D — D1 Distributed Trace Span Cross-Links

> **Wave D Phase 2A dependency:** The trace span annotations below reference the `DistributedTraceSpan`
> framework planned in `docs/operability/WAVE_D_ROADMAP.md` §2A. Until Phase 2A implementation
> completes (Target: Q1 2027), the listed span names are reference identifiers for future
> instrumentation.

### Network Transport D1 Trace Spans

When the Phase 2A tracing SDK is available, the following operator actions map to trace spans:

| Transport Surface | D1 Span Name | Baggage Keys | Notes |
|---|---|---|---|
| TCP frame dispatch | `network.tcp.frame_dispatch` | `opcode`, `payload_size`, `session_id` | Includes worker-pool handoff child span |
| WebSocket frame round-trip | `network.ws.frame_roundtrip` | `ws_opcode`, `frame_size`, `session_id` | Includes upgrade span on first frame |
| Auth / session check | `network.auth.session_validate` | `session_token_prefix`, `error_code`, `opcode` | Error status set on `AUTH_REQUIRED` / `SESSION_EXPIRED` |
| Circuit breaker trip | `network.circuit_breaker.trip` | `error_class`, `consecutive_errors`, `state` | Status ERROR |
| gRPC transport | `network.grpc.call` | `method`, `endpoint`, `fallback_active` | Includes retry child spans |
| Connection lifecycle | `network.connection.lifecycle` | `transport`, `peer_ip`, `state_transitions` | Spans ACCEPTING → SERVING → DRAINING → CLOSED |

### Querying Trace Spans (Phase 2A onwards)

```bash
# Find all frame dispatch traces with high latency
otel-query --service themisdb_network --operation network.tcp.frame_dispatch \
  --min-duration 200ms --range 1h

# Find all auth failures by error code
otel-query --service themisdb_network --operation network.auth.session_validate \
  --status ERROR --range 24h --include-baggage

# Find circuit breaker trip events
otel-query --service themisdb_network --operation network.circuit_breaker.trip \
  --status ERROR --range 7d

# Cross-reference gRPC fallback with frame error rate
otel-metrics-join \
  --trace-operation network.grpc.call \
  --metric network_grpc_fallback_active \
  --window 5m
```

### Phase 2A Instrumentation Targets

Once Phase 2A is implemented, add trace points in:

- `src/network/wire_protocol_server.cpp`: Wrap `dispatchToWorkerPool()` in `DistributedTraceSpan`
  with baggage `opcode`, `payload_size`, `session_id`
- `src/network/wire_protocol_server.cpp`: Wrap `handleAuthRequest()` in span with baggage
  `session_token_prefix`, `error_code`
- `src/network/adaptive_circuit_breaker.cpp`: Emit span events on OPEN, HALF_OPEN, CLOSED
  state transitions with baggage `error_class`, `consecutive_errors`
- `src/network/grpc_transport.cpp`: Add span for each gRPC call with fallback detection baggage

**Related Wave D documents:**
- `docs/operability/WAVE_D_ROADMAP.md` §2A — DistributedTraceSpan implementation plan
- `src/network/ROADMAP.md` §Wave D — Contribution items and closure status
- `include/network/network_api_contract.h` — `NetworkErrorCode` taxonomy referenced above

---

**Runbook Version:** 1.0
**Last Updated:** 2026-09-16
**Owner:** Network/Transport Team, Operations Team
**Next Review:** 2027-03-01 (post-Wave D Phase 2A delivery)
