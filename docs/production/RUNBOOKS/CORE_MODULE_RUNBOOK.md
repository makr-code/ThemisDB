# ThemisDB Core Module — Production Runbook

**Version:** 1.4.0  
**Last Updated:** April 2026  
**Target Audience:** SREs, DevOps Engineers, Platform Engineers

---

## Table of Contents

1. [Required Environment Variables](#required-environment-variables)
2. [Optional Environment Variables](#optional-environment-variables)
3. [Startup Sequence](#startup-sequence)
4. [Health Checks](#health-checks)
5. [Failure Modes and Mitigations](#failure-modes-and-mitigations)
6. [Security-Related Failure Modes](#security-related-failure-modes)
7. [Escalation Path](#escalation-path)

---

## Required Environment Variables

The following variables **must** be set before starting ThemisDB in production.
Missing values cause startup failure or insecure defaults (marked ⚠️).

| Variable | Required | Default | Description |
|----------|:--------:|---------|-------------|
| `THEMIS_PRODUCTION_MODE` | ✅ | `0` ⚠️ | Set to `1` to enable production-mode safety checks. When unset, stub providers are permitted. |
| `THEMIS_ENVIRONMENT` | ✅ | `development` ⚠️ | Set to `production`. Controls HSM stub gating, strict PII mode, and key provider selection. |
| `THEMIS_TOKEN_ADMIN` | ✅ | *none* ⚠️ | Admin API bearer token. If unset, admin endpoints may be unauthenticated. |
| `THEMIS_PKI_PRIVATE_KEY` | ✅ | *none* | Path to the server private key file. |
| `THEMIS_PKI_CERTIFICATE` | ✅ | *none* | Path to the server TLS certificate file. |

> [!CAUTION]
> If `THEMIS_PRODUCTION_MODE=1` is **not** set and `THEMIS_ENVIRONMENT` is not `production`, ThemisDB will start with **insecure defaults**: stub HSM, no token enforcement, and mock key providers. **Never deploy to production without these variables.**

---

## Optional Environment Variables

These variables tune behaviour. Defaults are shown; override as needed.

### Security

| Variable | Default | Description |
|----------|---------|-------------|
| `THEMIS_ALLOW_HSM_STUB` | *unset* | Set to `1` to allow the HSM stub provider. **Never set in production.** |
| `THEMIS_HSM_ENABLED` | `0` | Set to `1` to enable the hardware HSM provider. |
| `THEMIS_HSM_PIN` | *unset* | HSM PKCS#11 user PIN. Prefer injecting via secrets manager. |
| `THEMIS_HSM_SESSION_POOL` | `4` | Number of concurrent HSM sessions. |
| `THEMIS_PII_STRICT` | `0` | Set to `1` to enforce strict PII redaction (fails-closed on violation). |
| `THEMIS_AUDIT_RATE_LIMIT` | *unset* | Max audit events per second (protects against flooding). |

### PKI / TLS

| Variable | Default | Description |
|----------|---------|-------------|
| `THEMIS_PKI_KEY_PASSPHRASE` | *unset* | Passphrase for encrypted private key. Inject via secrets manager. |
| `THEMIS_PKI_ENDPOINT` | *unset* | URL of an external PKI/ACME endpoint for certificate issuance. |
| `THEMIS_PKI_SIG_ALG` | `RS256` | Signature algorithm used by the PKI subsystem. |
| `THEMIS_DEBUG_PKI` | *unset* | Set to `1` to enable verbose PKI debug logging. **Do not set in production.** |

### Vault Integration

| Variable | Default | Description |
|----------|---------|-------------|
| `THEMIS_VAULT_ADDR` | *unset* | HashiCorp Vault address (e.g., `https://vault.example.com:8200`). |
| `THEMIS_VAULT_TOKEN` | *unset* | Vault token. Prefer Vault agent / Kubernetes auth method over static tokens. |
| `THEMIS_VAULT_TRANSIT_MOUNT` | `transit` | Vault transit secrets engine mount path. |
| `VAULT_ADDR` | *unset* | Fallback Vault address (standard Vault SDK env var). |
| `VAULT_TOKEN` | *unset* | Fallback Vault token (standard Vault SDK env var). |

### WAL / gRPC Replication

| Variable | Default | Description |
|----------|---------|-------------|
| `THEMIS_WAL_GRPC_HOST` | `127.0.0.1` | gRPC WAL replication host. |
| `THEMIS_WAL_GRPC_PORT` | `9100` | gRPC WAL replication port. |
| `THEMIS_WAL_GRPC_ENABLE_MTLS` | `0` ⚠️ | Set to `1` to enable mTLS for WAL replication. **Required in production clusters.** |
| `THEMIS_WAL_GRPC_CERT_PATH` | *unset* | mTLS client certificate path. |
| `THEMIS_WAL_GRPC_KEY_PATH` | *unset* | mTLS client key path. |
| `THEMIS_WAL_GRPC_CA_CERT_PATH` | *unset* | CA certificate path for peer verification. |
| `THEMIS_WAL_GRPC_REQUIRE_CLIENT_CERT` | `0` ⚠️ | Set to `1` to require client certs. **Required when mTLS is enabled.** |
| `THEMIS_WAL_SHARED_SECRET` | *unset* | Shared secret for WAL authentication. Use mTLS instead where possible. |
| `THEMIS_WAL_HMAC_SECRET` | *unset* | HMAC secret for WAL message integrity. |

### Network / Health

| Variable | Default | Description |
|----------|---------|-------------|
| `THEMIS_HEALTH_PORT` | `8081` | Port for the `/health` liveness/readiness endpoint. |
| `THEMIS_HEALTH_BIND_ADDRESS` | `0.0.0.0` | Bind address for the health endpoint. |

### Sharding

| Variable | Default | Description |
|----------|---------|-------------|
| `THEMIS_ENABLE_SHARDING` | `0` | Set to `1` to enable horizontal sharding. |
| `THEMIS_SHARD_ID` | `0` | Integer shard identifier for this instance. |
| `THEMIS_SHARDS` | `1` | Total number of shards in the cluster. |
| `THEMIS_BOOTSTRAP_SHARD` | `0` | Set to `1` on the first shard to seed the cluster. |
| `SHARD_ID` | *unset* | Alternative shard ID (overrides `THEMIS_SHARD_ID`). |
| `CLUSTER_SIZE` | *unset* | Total cluster size (overrides `THEMIS_SHARDS`). |
| `THEMIS_RAID_GROUP` | *unset* | RAID group identifier for storage redundancy. |

### LLM / Model Serving

| Variable | Default | Description |
|----------|---------|-------------|
| `THEMIS_MODEL_DIR` | `./models` | Directory to load GGUF/GGML model files from. |
| `THEMIS_GPU_LAYERS` | `0` | Number of model layers to offload to GPU. |
| `THEMIS_THREADS` | auto | Number of CPU inference threads. |
| `THEMIS_CONTEXT_SIZE` | `2048` | Default LLM context window size in tokens. |
| `THEMIS_DISABLE_AUTO_DOWNLOAD` | `0` | Set to `1` to prevent automatic model downloads. **Recommended in production.** |
| `THEMIS_OLLAMA_ENDPOINT` | *unset* | Ollama server endpoint for remote model serving. |

### Policy / Ranger

| Variable | Default | Description |
|----------|---------|-------------|
| `THEMIS_POLICIES_PATH` | `./config/policies` | Path to local RBAC policy files. |
| `THEMIS_RANGER_SERVICE` | `themisdb` | Apache Ranger service name. |
| `THEMIS_RANGER_POLICIES_PATH` | `/service/public/v2/api/policy` | Ranger REST API path. |
| `THEMIS_RANGER_BEARER` | *unset* | Bearer token for Ranger authentication. |

### Encryption Internals

| Variable | Default | Description |
|----------|---------|-------------|
| `THEMIS_ENC_PARALLEL` | `1` | Set to `0` to disable parallel encryption. |
| `THEMIS_DEBUG_ENC_DIR` | *unset* | Directory for encryption debug dumps. **Never set in production.** |

---

## Startup Sequence

On startup ThemisDB performs the following checks in order:

1. **Mode detection** — reads `THEMIS_PRODUCTION_MODE` and `THEMIS_ENVIRONMENT`.
2. **HSM provider selection** — fails if `THEMIS_PRODUCTION_MODE=1` and `THEMIS_ALLOW_HSM_STUB` is set.
3. **Admin token check** — logs a warning if `THEMIS_TOKEN_ADMIN` is unset.
4. **Key provider initialization** — Vault, HSM, or file-based key provider is initialised.
5. **WAL gRPC setup** — configures replication channel (mTLS if `THEMIS_WAL_GRPC_ENABLE_MTLS=1`).
6. **Health endpoint binding** — opens health port on `THEMIS_HEALTH_BIND_ADDRESS:THEMIS_HEALTH_PORT`.
7. **HTTP server start** — begins accepting API requests.

---

## Health Checks

### Liveness probe

```
GET /health
```

Returns `200 OK` with `{"status":"ok"}` if the process is alive.

### Readiness probe

```
GET /health/ready
```

Returns `200 OK` only when all subsystems (storage, key provider, WAL replication) are ready.

### Example Kubernetes probes

```yaml
livenessProbe:
  httpGet:
    path: /health
    port: 8081
  initialDelaySeconds: 30
  periodSeconds: 10
  failureThreshold: 3

readinessProbe:
  httpGet:
    path: /health/ready
    port: 8081
  initialDelaySeconds: 5
  periodSeconds: 5
  failureThreshold: 3
```

---

## Failure Modes and Mitigations

### Startup Failures

| Symptom | Likely Cause | Mitigation |
|---------|-------------|------------|
| Process exits immediately with `HSM stub blocked in production mode` | `THEMIS_PRODUCTION_MODE=1` but HSM stub is the only configured provider | Configure a real HSM or Vault; do not set `THEMIS_ALLOW_HSM_STUB` |
| `THEMIS_TOKEN_ADMIN is not set` warning at startup | Admin token missing | Set `THEMIS_TOKEN_ADMIN` to a strong random secret (≥32 bytes, base64-encoded) |
| `Key provider initialization failed` | Vault unreachable or bad token | Verify `THEMIS_VAULT_ADDR`, `THEMIS_VAULT_TOKEN`; check network policy |
| gRPC WAL replication fails to connect | mTLS cert mismatch or wrong host/port | Verify `THEMIS_WAL_GRPC_*` variables; check certificate SANs |
| `Failed to open RocksDB` | Data directory missing or permissions error | Ensure `/data/themisdb` exists and is writable by UID 1000 |

### Runtime Failures

| Symptom | Likely Cause | Mitigation |
|---------|-------------|------------|
| `503 Service Unavailable` on all requests | Readiness probe failing | Check Vault connectivity and WAL replication lag |
| High audit log dropped events | `THEMIS_AUDIT_RATE_LIMIT` too low or log sink overwhelmed | Increase rate limit; scale log sink; switch to async SIEM |
| OOM / container evicted | Memory limits too low for loaded models | Increase pod memory limits; reduce `THEMIS_GPU_LAYERS` |
| `permission denied` on `/dev/nvidia*` | GPU device not mounted into container | Add device plugin toleration; verify `nvidia.com/gpu` resource |
| Shard coordinator unreachable | `SHARD_ID` / `CLUSTER_SIZE` misconfigured | Verify all shards use the same `THEMIS_SHARDS` and sequential shard IDs |
| Stale data / replication lag | WAL gRPC peer down | Alert on `themisdb_wal_replication_lag_seconds > 5`; trigger failover runbook |

### Data Integrity Failures

| Symptom | Likely Cause | Mitigation |
|---------|-------------|------------|
| Checksum mismatch in audit logs | Tampered or corrupted audit log | Immediately set read-only mode; preserve evidence; restore from WORM backup |
| RocksDB corruption detected | Disk failure, ungraceful shutdown | Run `themisdb-cli integrity check --full`; restore from last verified backup |
| Key decryption failure | Key rotation mid-flight or wrong key version | Check key provider rotation state; see key rotation runbook |

---

## Security-Related Failure Modes

| Symptom | Severity | Mitigation |
|---------|:--------:|------------|
| `THEMIS_ALLOW_HSM_STUB=1` set in production | 🔴 Critical | Remove variable immediately; rotate all credentials; audit HSM-protected keys |
| `THEMIS_DEBUG_ENC_DIR` set in production | 🔴 Critical | Remove variable; delete debug dump files; assume plaintext data was written to disk |
| `THEMIS_DEBUG_PKI=1` set in production | 🟡 High | Remove variable; rotate PKI credentials; review logs for credential leakage |
| `THEMIS_TOKEN_ADMIN` exposed in environment listing | 🔴 Critical | Rotate token; use secrets manager (Vault, Kubernetes Secret); restrict pod env inspection |
| WAL replication without mTLS in a multi-node cluster | 🔴 Critical | Set `THEMIS_WAL_GRPC_ENABLE_MTLS=1` and `THEMIS_WAL_GRPC_REQUIRE_CLIENT_CERT=1` |

---

## Escalation Path

1. **Level 1 (On-Call SRE):** Check health endpoint, review recent pod logs, consult [TROUBLESHOOTING.md](../TROUBLESHOOTING.md).
2. **Level 2 (Database Operations):** WAL replication issues, data integrity failures, key provider problems.
3. **Level 3 (Security Team):** Any event in the *Security-Related Failure Modes* table above.

---

**Document Version:** 1.0  
**Last Updated:** April 2026  
**Next Review:** May 2026  
**Related Documents:**
- [SECURITY_POSTURE.md](../SECURITY_POSTURE.md) — security defaults and hardening guide
- [systemd unit](../../../deploy/systemd/themisdb.service) and [production drop-in](../../../deploy/systemd/themisdb.service.d/production.conf)
- [Kubernetes production Helm values](../examples/k8s_production_values.yaml)
- [TROUBLESHOOTING.md](../TROUBLESHOOTING.md)
