# rpc_grpc Module — Operator Runbook

<!-- Status: v2.0.0 production-ready | 2026-08-06 -->
<!-- Links: ROADMAP.md · FUTURE_ENHANCEMENTS.md · PHASE_6_ACCEPTANCE_CHECKLIST.md -->

## Overview

This runbook documents operational procedures for deploying, configuring, monitoring, and troubleshooting the ThemisDB gRPC plugin and server in production environments.

---

## § 1 — Deployment Prerequisites

### 1.1 System Requirements

| Requirement | Minimum | Recommended |
|-------------|---------|-------------|
| CPU Cores | 2 | 4+ |
| Memory | 256 MB | 1 GB+ |
| gRPC Version | 1.50.0 | 1.60.0+ |
| OpenSSL | 1.1.1 | 3.0+ |
| Linux Kernel | 4.4+ | 5.10+ |

### 1.2 Certificate Files

TLS is **mandatory** in production. Prepare certificate files:

```bash
# Server certificate (PEM format)
/etc/themis/certs/server.crt

# Server private key (PEM format, mode 0600)
/etc/themis/certs/server.key

# CA certificate (for mTLS validation)
/etc/themis/certs/ca.crt
```

**Security note:** Protect private key file with restrictive permissions:
```bash
chmod 600 /etc/themis/certs/server.key
chown themis:themis /etc/themis/certs/server.key
```

### 1.3 Service Account

Create unprivileged service account:
```bash
useradd -m -s /usr/sbin/nologin -c "ThemisDB gRPC" themis
```

---

## § 2 — Configuration

### 2.1 Configuration File Format

Configuration is passed via JSON to the gRPC plugin:

```json
{
  "host": "0.0.0.0",
  "port": 50051,
  "tls_enabled": true,
  "tls_cert_path": "/etc/themis/certs/server.crt",
  "tls_key_path": "/etc/themis/certs/server.key",
  "tls_ca_cert_path": "/etc/themis/certs/ca.crt",
  "auth_required": true,
  "extra_config": {
    "keepalive_time_ms": "30000",
    "keepalive_timeout_ms": "10000",
    "admin_port": "50052"
  }
}
```

### 2.2 Key Parameters

| Parameter | Type | Default | Notes |
|-----------|------|---------|-------|
| `host` | string | `0.0.0.0` | Listening address (all interfaces recommended for k8s) |
| `port` | integer | 50051 | Main gRPC service port |
| `tls_enabled` | boolean | true | **MUST be true in production** |
| `tls_cert_path` | string | (required) | Path to server certificate (PEM) |
| `tls_key_path` | string | (required) | Path to server private key (PEM) |
| `tls_ca_cert_path` | string | (required) | Path to CA certificate (PEM) |
| `auth_required` | boolean | true | Require client certificate (mTLS) |
| `keepalive_time_ms` | string | "30000" | Idle keepalive ping interval (ms) |
| `keepalive_timeout_ms` | string | "10000" | Keepalive timeout (ms) |
| `admin_port` | string | (optional) | Admin/metrics port (insecure; see §1.3) |

### 2.3 Unsafe Configuration Warning (GAP-016)

⚠️ **SECURITY ALERT**

When `admin_port` is specified, the plugin binds an **insecure** metrics/health endpoint on that port (CWE-295). This is suitable for internal-only networks (e.g., Kubernetes service mesh) but **MUST NOT** be exposed to untrusted networks.

Log entry on server start with admin port:
```
[RPC-W/GAP-016] GRPCServer: admin port 50052 bound with insecure credentials (CWE-295).
```

**Mitigation:**
- Only specify `admin_port` in internal, non-routable networks
- Firewall rule: Restrict admin port access to cluster/VPC boundaries
- Use reverse proxy (Envoy, Nginx) to gate access

---

## § 3 — Startup Procedures

### 3.1 Manual Startup

```bash
# Set configuration
export RPC_GRPC_CONFIG='{...}'  # from §2.1

# Start the service
systemctl start themis-grpc

# Verify startup
systemctl status themis-grpc
journalctl -u themis-grpc -f
```

### 3.2 Expected Startup Log Sequence

```
[RPC-I] gRPC server configured for mutual TLS (mTLS)
[RPC-I] gRPC server listening on 0.0.0.0:50051
[RPC-I] gRPC admin port bound on 0.0.0.0:50052
[RPC-I] mark global health as SERVING
```

### 3.3 Startup Failure Diagnosis

**Symptom: "CRITICAL: Failed to configure TLS"**
```
Action: Verify certificate paths and file permissions (§2.2)
        chmod 600 /etc/themis/certs/server.key
        chown themis:themis /etc/themis/certs/*
```

**Symptom: "Server start rejected: already running"**
```
Action: Check for stale process
        ps aux | grep themis
        Kill stale process: kill -9 <PID>
        Restart service: systemctl restart themis-grpc
```

**Symptom: "Address already in use (port 50051)"**
```
Action: Check bound ports
        netstat -tlnp | grep 50051
        Kill blocking process or change port
```

---

## § 4 — TLS Certificate Rotation

### 4.1 Zero-Downtime Reload

The gRPC server supports hot-reload of TLS certificates without shutdown:

```cpp
// Call reloadTls() while server is running
bool success = server->reloadTls(
    "/etc/themis/certs/new_server.crt",
    "/etc/themis/certs/new_server.key",
    "/etc/themis/certs/new_ca.crt"
);
if (!success) {
    // Old credentials remain active; reload failed
    // Log contains reason: [RPC-E8302] TLS reload failed: ...
}
```

### 4.2 Automated Rotation Workflow

```bash
#!/bin/bash
# Refresh certificates (e.g., from Let's Encrypt)
certbot renew --quiet

# Validate new certificates
openssl x509 -in /etc/themis/certs/server.crt -noout -dates

# Trigger reload via ThemisDB API or CLI
# (Implementation depends on your deployment)
themis-ctl grpc reload-tls

# Monitor logs for success
journalctl -u themis-grpc -f | grep "TLS certificates reloaded"
```

### 4.3 Reload Failure Recovery

If `reloadTls()` returns false:

1. **Check error log** for reason: [RPC-E8302] contains details
2. **Validate certificate format:**
   ```bash
   openssl x509 -in /etc/themis/certs/server.crt -text
   openssl pkey -in /etc/themis/certs/server.key -text
   ```
3. **Verify key-certificate pair match:**
   ```bash
   openssl x509 -noout -modulus -in /etc/themis/certs/server.crt | openssl md5
   openssl rsa -noout -modulus -in /etc/themis/certs/server.key | openssl md5
   # Should produce identical hashes
   ```
4. **Retry after 60 seconds** (old credentials remain active during delay)
5. **If persistent failure:** Restart service (will re-load at startup)

---

## § 5 — Health Checks & Monitoring

### 5.1 Health Check Endpoint

If `admin_port` is configured, query `/health`:

```bash
curl -s http://localhost:50052/health | jq .
```

Expected response:
```json
{
  "status": "SERVING",
  "services": {
    "themis.QueryService": "SERVING",
    "themis.WriteService": "SERVING"
  }
}
```

### 5.2 Metrics Export (Prometheus)

Query metrics endpoint:

```bash
curl -s http://localhost:50052/metrics | grep grpc_
```

Key metrics:
- `grpc_server_requests_total{method}` — cumulative request count per method
- `grpc_server_errors_total{method}` — cumulative error count per method
- `grpc_server_latency_ms_total{method}` — cumulative latency (divide by request count for avg)
- `grpc_server_active_connections` — gauge of current active connections

### 5.3 Structured Access Logs

If access logging is enabled, logs are JSON formatted:

```json
{
  "timestamp_ms": 1722963400000,
  "method": "/themis.QueryService/Execute",
  "status_code": 0,
  "duration_ms": 42,
  "client_cn": "client.internal.example.com"
}
```

**Log parsing for monitoring:**
```bash
# Extract error methods (status_code != 0)
journalctl -u themis-grpc -f | jq 'select(.status_code != 0) | .method'

# Calculate p95 latency (requires aggregation)
# Use Prometheus scrape + alerting rules (see dashboards/)
```

---

## § 6 — Troubleshooting

### 6.1 Server State Inquiry

Check if server is running and accepting requests:

```bash
# Via systemd
systemctl is-active themis-grpc

# Via process
ps aux | grep themis

# Via network port
netstat -tlnp | grep 50051

# Via gRPC health check
grpcurl -plaintext localhost:50051 grpc.health.v1.Health/Check \
  -d '{"service":"themis.QueryService"}'
```

### 6.2 Common Issues & Resolutions

| Symptom | Root Cause | Resolution |
|---------|-----------|------------|
| "connection refused" | Server not running | Start service: `systemctl start themis-grpc` |
| "TLS handshake failed" | Certificate mismatch or expired | Rotate certificates (§4); check expiry: `openssl x509 -enddate -in server.crt` |
| "Server shutdown timeout" | Long-running RPC operations | Increase shutdown grace period; check for stuck clients |
| "High error rate on method X" | Client misconfiguration or service outage | Query metrics (§5.2); check service dependencies |
| "Memory usage increasing" | Resource leak or unbound connection growth | Monitor `grpc_server_active_connections`; restart if necessary |

### 6.3 Debug Mode

Enable verbose logging (if built with debug flags):

```bash
# Set debug environment variables
export GRPC_VERBOSITY=DEBUG
export GRPC_TRACE=transport_security

# Restart
systemctl restart themis-grpc
journalctl -u themis-grpc -f
```

---

## § 7 — Maintenance & Upgrades

### 7.1 Graceful Shutdown

```bash
# Drain in-flight requests (30s timeout default)
systemctl stop themis-grpc

# Force shutdown if needed
systemctl kill -s SIGKILL themis-grpc
```

### 7.2 Rolling Updates

In Kubernetes (or similar orchestrators):

```yaml
# Rolling update strategy
spec:
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxSurge: 1
      maxUnavailable: 0  # No unavailability during update
  template:
    spec:
      containers:
      - name: themis-grpc
        image: themis:v2.0.0-rc1
        readinessProbe:
          exec:
            command:
            - grpcurl
            - "-plaintext"
            - "localhost:50051"
            - "grpc.health.v1.Health/Check"
          initialDelaySeconds: 5
          periodSeconds: 5
        terminationGracePeriodSeconds: 30
```

### 7.3 Backup & Recovery

**Backup TLS certificates:**
```bash
tar czf /backup/themis-certs-$(date +%Y%m%d).tar.gz \
  /etc/themis/certs/
```

**Recovery:**
```bash
tar xzf /backup/themis-certs-20260806.tar.gz -C /
chmod 600 /etc/themis/certs/server.key
systemctl restart themis-grpc
```

---

## § 8 — Performance Tuning

### 8.1 Keepalive Configuration

Adjust for high-latency or flaky networks:

```json
{
  "extra_config": {
    "keepalive_time_ms": "60000",        // Ping interval (default 30s)
    "keepalive_timeout_ms": "20000"      // Timeout on ping (default 10s)
  }
}
```

**Guidelines:**
- High latency (> 100ms): Increase both values
- Flaky network: Decrease `keepalive_time_ms` to detect failures faster
- Mobile clients: Use 60s keepalive to reduce overhead

### 8.2 Connection Limits

Configure OS-level limits:

```bash
# Increase file descriptor limit
ulimit -n 65536

# Permanent (add to /etc/security/limits.conf)
themis soft nofile 65536
themis hard nofile 65536
```

### 8.3 TLS Session Caching

gRPC uses OS-level TLS session caching. Ensure system OpenSSL version is current (≥ 1.1.1).

---

## § 9 — Security Hardening Checklist

- [x] TLS enabled (tls_enabled=true)
- [x] Client certificate validation enabled (auth_required=true)
- [x] Private key file permissions: 0600
- [x] Admin port firewalled (if used)
- [x] Certificate expiry monitoring in place
- [x] Access logs monitored for anomalies
- [x] Rate limiting applied upstream (Envoy, API gateway)
- [x] Network policies restrict ingress to trusted sources

---

## § 10 — Support & Escalation

### 10.1 Log Levels

All log entries use structured prefixes:
- `[RPC-I]` — Informational (normal operation)
- `[RPC-W]` — Warning (e.g., insecure config)
- `[RPC-E8xxx]` — Error (specific error code from taxonomy)

### 10.2 Contact & Escalation

- **Level 1 (Operator):** Review logs, check §6.2 troubleshooting
- **Level 2 (SRE):** Restart service, collect metrics, check system resources
- **Level 3 (Engineering):** Debug builds, profiling, code review

### 10.3 Incident Report Template

```
Subject: gRPC Service Incident Report

Time: [timestamp]
Duration: [duration]
Affected Services: [services]

Symptoms:
- [observed behavior]
- [metrics deviation]

Root Cause: [suspected cause]

Mitigation: [action taken]

Resolution: [fix applied]

Preventive Actions:
- [future improvements]
```

---

## § 11 — Change Log & Version History

| Version | Date | Changes |
|---------|------|---------|
| 2.0.0 | 2026-08-06 | Phase 2-6 hardening complete; fail-safe TLS reload; diagnostics unification |
| 1.0.0 | 2026-05-31 | Initial production release |

---

**Last Updated:** 2026-08-06

**Document Owner:** ThemisDB Release Engineering

**Review Frequency:** Quarterly or upon major gRPC version upgrade

