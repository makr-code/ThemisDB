# ThemisDB Security Posture Guide

**Version:** 1.4.0  
**Last Updated:** April 2026  
**Target Audience:** Security Engineers, Integration Teams, Platform Engineers

> [!IMPORTANT]
> This document is the authoritative reference for ThemisDB security defaults, unsafe-mode indicators, and the configuration steps required for a production-hardened deployment. **Read this before going to production.**

---

## Table of Contents

1. [Security Modes Overview](#security-modes-overview)
2. [Unsafe Defaults — Development Mode](#unsafe-defaults--development-mode)
3. [Production Hardening Checklist](#production-hardening-checklist)
4. [Required Production Settings](#required-production-settings)
5. [HSM Configuration](#hsm-configuration)
6. [Token and Secret Management](#token-and-secret-management)
7. [TLS / mTLS Configuration](#tls--mtls-configuration)
8. [Audit Logging Requirements](#audit-logging-requirements)
9. [PII and Data Classification](#pii-and-data-classification)
10. [WAL Replication Security](#wal-replication-security)
11. [Threat Model Quick Reference](#threat-model-quick-reference)
12. [Actionable Integrator Checklist](#actionable-integrator-checklist)

---

## Security Modes Overview

ThemisDB operates in one of two modes, controlled by environment variables:

| Mode | `THEMIS_PRODUCTION_MODE` | `THEMIS_ENVIRONMENT` | Description |
|------|:------------------------:|:--------------------:|-------------|
| **Development** | `0` (default) | `development` (default) | Stub providers allowed; mock keys; no token enforcement. **Unsafe for production.** |
| **Production** | `1` | `production` | Stub providers blocked; real HSM/Vault required; strict PII; token enforcement. |

> [!CAUTION]
> **Development mode is the default.** If you do not explicitly set `THEMIS_PRODUCTION_MODE=1` and `THEMIS_ENVIRONMENT=production`, ThemisDB starts with **insecure development defaults** that must never be used to handle real data.

---

## Unsafe Defaults — Development Mode

The following table lists every insecure default that is active when `THEMIS_PRODUCTION_MODE` is **not** `1`. The *Risk* column describes the consequence if used in production.

| Default Behaviour | Environment Variable to Change | Risk |
|-------------------|-------------------------------|------|
| HSM stub provider is used (software simulation of HSM) | Set `THEMIS_HSM_ENABLED=1`; do **not** set `THEMIS_ALLOW_HSM_STUB` | Encryption keys are not hardware-protected; key material can be extracted from memory |
| Admin API token is not required | Set `THEMIS_TOKEN_ADMIN=<strong-secret>` | Any client can call administrative endpoints without authentication |
| WAL gRPC replication is plaintext | Set `THEMIS_WAL_GRPC_ENABLE_MTLS=1` | Replication traffic can be intercepted and modified in transit |
| Mock/file-based key provider is used if Vault is not configured | Set `THEMIS_VAULT_ADDR` and `THEMIS_VAULT_TOKEN` | Encryption keys stored alongside data; no key-data separation |
| PII strict mode is off | Set `THEMIS_PII_STRICT=1` | PII fields may leak into logs or query results |
| Auto-download of LLM models is enabled | Set `THEMIS_DISABLE_AUTO_DOWNLOAD=1` | Models pulled from internet in production; supply-chain and SSRF risk |
| Debug encryption dumps are permitted | Ensure `THEMIS_DEBUG_ENC_DIR` is **unset** | Plaintext data written to arbitrary disk paths |
| PKI debug logging is permitted | Ensure `THEMIS_DEBUG_PKI` is **unset** | TLS private key material written to logs |

---

## Production Hardening Checklist

Use this checklist before go-live. Every item must be marked ✅.

### Mode & Secrets

- [ ] `THEMIS_PRODUCTION_MODE=1` is set
- [ ] `THEMIS_ENVIRONMENT=production` is set
- [ ] `THEMIS_TOKEN_ADMIN` is set to a cryptographically random value (≥32 bytes, base64-encoded)
- [ ] `THEMIS_ALLOW_HSM_STUB` is **not** set (or is explicitly absent)
- [ ] `THEMIS_DEBUG_ENC_DIR` is **not** set
- [ ] `THEMIS_DEBUG_PKI` is **not** set
- [ ] `THEMIS_PII_FORCE_INIT_FAIL` is **not** set

### Key Management

- [ ] HashiCorp Vault or hardware HSM is configured (`THEMIS_VAULT_ADDR` or `THEMIS_HSM_ENABLED=1`)
- [ ] Vault token (`THEMIS_VAULT_TOKEN`) is a short-lived token or uses Vault agent / Kubernetes auth
- [ ] HSM PIN (`THEMIS_HSM_PIN`) is injected via secrets manager, not hardcoded
- [ ] Key rotation schedule is configured (recommended: 90-day master key rotation)

### Transport Security

- [ ] TLS 1.3 certificate and key paths are set (`THEMIS_PKI_CERTIFICATE`, `THEMIS_PKI_PRIVATE_KEY`)
- [ ] WAL gRPC mTLS is enabled (`THEMIS_WAL_GRPC_ENABLE_MTLS=1`, `THEMIS_WAL_GRPC_REQUIRE_CLIENT_CERT=1`)
- [ ] All TLS certificate SANs match the service hostname

### Operational

- [ ] `THEMIS_PII_STRICT=1` is set
- [ ] `THEMIS_DISABLE_AUTO_DOWNLOAD=1` is set
- [ ] Audit logging is configured to a persistent external sink (SIEM / S3 / Syslog)
- [ ] Health endpoints are bound to an internal-only address if possible

### Docker Image

- [ ] Community image pulled from `themisdb/themisdb` on Docker Hub (do **not** use `makr-code/themisdb`)
- [ ] Image built **without** `THEMIS_ENABLE_ENCRYPTED_STORAGE=ON` unless gocryptfs at-rest encryption is explicitly required
- [ ] No CRITICAL or HIGH CVEs in the deployed image (verify with `docker scout cves <image>` before go-live)
- [ ] Container runs as non-root user (`themisdb`, UID 999) — confirm with `docker run --rm <image> id`
- [ ] Known accepted CVEs (no upstream fix) tracked in `docs/audit-reports/cve-waivers.md`

---

## Required Production Settings

The minimal set of environment variables for a secure production deployment:

```bash
# --- Mode ---
THEMIS_PRODUCTION_MODE=1
THEMIS_ENVIRONMENT=production

# --- Authentication ---
THEMIS_TOKEN_ADMIN=<base64-encoded-random-32-bytes>

# --- TLS ---
THEMIS_PKI_CERTIFICATE=/etc/themisdb/tls/server.crt
THEMIS_PKI_PRIVATE_KEY=/etc/themisdb/tls/server.key

# --- Key Management (choose one) ---
# Option A: HashiCorp Vault
THEMIS_VAULT_ADDR=https://vault.example.com:8200
THEMIS_VAULT_TOKEN=<vault-token>          # prefer Vault agent instead
THEMIS_VAULT_TRANSIT_MOUNT=transit

# Option B: Hardware HSM (PKCS#11)
THEMIS_HSM_ENABLED=1
THEMIS_HSM_PIN=<pin>                      # inject via secrets manager

# --- Data Safety ---
THEMIS_PII_STRICT=1
THEMIS_DISABLE_AUTO_DOWNLOAD=1

# --- WAL Replication (multi-node only) ---
THEMIS_WAL_GRPC_ENABLE_MTLS=1
THEMIS_WAL_GRPC_REQUIRE_CLIENT_CERT=1
THEMIS_WAL_GRPC_CERT_PATH=/etc/themisdb/tls/wal-client.crt
THEMIS_WAL_GRPC_KEY_PATH=/etc/themisdb/tls/wal-client.key
THEMIS_WAL_GRPC_CA_CERT_PATH=/etc/themisdb/tls/ca.crt
```

---

## HSM Configuration

### Real HSM (recommended for production)

```bash
THEMIS_HSM_ENABLED=1
THEMIS_HSM_PIN=<pkcs11-user-pin>
THEMIS_HSM_SESSION_POOL=8        # tune to HSM capacity
```

**Never** set `THEMIS_ALLOW_HSM_STUB=1` in production. ThemisDB will refuse to start if both `THEMIS_PRODUCTION_MODE=1` and `THEMIS_ALLOW_HSM_STUB=1` are set simultaneously.

### HashiCorp Vault (alternative)

Using Kubernetes auth (recommended over static tokens):

```yaml
# Vault agent injector annotations
vault.hashicorp.com/agent-inject: "true"
vault.hashicorp.com/role: "themisdb-production"
vault.hashicorp.com/agent-inject-secret-vault-token: "auth/kubernetes/login"
```

Then set only:
```bash
THEMIS_VAULT_ADDR=https://vault.example.com:8200
THEMIS_VAULT_TRANSIT_MOUNT=transit
# THEMIS_VAULT_TOKEN is not needed when using Vault agent
```

---

## Token and Secret Management

### Admin Token

The `THEMIS_TOKEN_ADMIN` token is used to authenticate calls to the admin HTTP endpoints. Requirements:
- **Minimum entropy:** 256 bits (32 random bytes, base64-encoded = 44 characters)
- **Storage:** Kubernetes `Secret` with `type: Opaque`, or Vault `kv` secret. Never in ConfigMaps or environment-variable logs.
- **Rotation:** Rotate at least every 90 days, or immediately after any suspected compromise.

**Generate a secure token:**
```bash
openssl rand -base64 32
```

### Protecting Secrets at Rest in Kubernetes

```yaml
apiVersion: v1
kind: Secret
metadata:
  name: themisdb-secrets
  namespace: production
type: Opaque
stringData:
  THEMIS_TOKEN_ADMIN: "<generated-token>"
  THEMIS_VAULT_TOKEN: "<vault-token>"
  THEMIS_HSM_PIN: "<hsm-pin>"
```

Reference in the pod spec:
```yaml
envFrom:
  - secretRef:
      name: themisdb-secrets
```

> [!WARNING]
> Do **not** store secrets in Helm `values.yaml` files committed to source control. Use `helm secrets` (SOPS) or an external secrets operator (ESO) instead.

---

## TLS / mTLS Configuration

### Minimum TLS requirements

| Requirement | Value |
|-------------|-------|
| Minimum TLS version | **TLS 1.2** (TLS 1.3 strongly recommended) |
| Required cipher suites (TLS 1.3) | `TLS_AES_256_GCM_SHA384`, `TLS_CHACHA20_POLY1305_SHA256` |
| Certificate key size | RSA ≥ 3072 bits **or** ECDSA P-256/P-384 |
| Certificate validity | ≤ 397 days (automated renewal recommended) |
| Session tickets | Disabled (breaks forward secrecy) |

### Server TLS

```bash
THEMIS_PKI_CERTIFICATE=/etc/themisdb/tls/server.crt
THEMIS_PKI_PRIVATE_KEY=/etc/themisdb/tls/server.key
THEMIS_PKI_KEY_PASSPHRASE=<passphrase>   # if key is encrypted
```

### WAL gRPC mTLS (required in multi-node clusters)

```bash
THEMIS_WAL_GRPC_ENABLE_MTLS=1
THEMIS_WAL_GRPC_REQUIRE_CLIENT_CERT=1
THEMIS_WAL_GRPC_CERT_PATH=/etc/themisdb/tls/wal-client.crt
THEMIS_WAL_GRPC_KEY_PATH=/etc/themisdb/tls/wal-client.key
THEMIS_WAL_GRPC_CA_CERT_PATH=/etc/themisdb/tls/ca.crt
```

> [!WARNING]
> Deploying a multi-node cluster **without** `THEMIS_WAL_GRPC_ENABLE_MTLS=1` means WAL replication data (including encrypted record batches) traverses the network in plaintext.

---

## Audit Logging Requirements

Production deployments must send audit events to an external, tamper-resistant sink.

```yaml
# config.yaml
logging:
  audit:
    enabled: true
    outputs:
      - type: syslog          # RFC 5424
        server: siem.example.com:514
        protocol: tcp
      - type: file
        path: /var/log/themisdb/audit.log
        rotation:
          max_size: 100MB
          max_age: 90
    signing:
      enabled: true            # ECDSA-signed audit chain
      algorithm: ECDSA-P256
```

Minimum required audit event categories for compliance:
- `authentication` (login success/failure, session timeout)
- `authorization` (permission granted/denied, role change)
- `data_access` (read, write, delete, export)
- `administrative` (config change, user creation/deletion, key rotation)
- `security` (failed TLS handshake, rate limit exceeded, HSM errors)

---

## PII and Data Classification

Enable strict PII mode in production:

```bash
THEMIS_PII_STRICT=1
```

In strict mode, any query or ingest operation that would write an unredacted PII field to an unencrypted column **fails closed** rather than silently succeeding. Configure redaction patterns in `config/security/pii_patterns.yaml`.

> [!NOTE]
> The pattern configuration file is validated by the `pii-redaction-check` CI workflow. Changes to `pii_patterns.yaml` require an accompanying update to `docs/en/security/PII_REDACTION_POLICY.md`.

---

## WAL Replication Security

| Deployment | mTLS Required | Shared Secret Acceptable |
|------------|:------------:|:------------------------:|
| Single node | No | N/A |
| Multi-node, same subnet | ✅ Yes | No |
| Multi-node, cross-region | ✅ Yes | No |

mTLS certificates for WAL peers should be issued by a dedicated internal CA, separate from the public-facing API CA. Short-lived certificates (≤ 30 days) with automated renewal are recommended.

---

## Threat Model Quick Reference

| Threat | Mitigated By | Status Without Hardening |
|--------|-------------|--------------------------|
| Key extraction from memory | Real HSM (`THEMIS_HSM_ENABLED=1`) | ⚠️ Keys exposed in process memory |
| MITM on replication traffic | WAL mTLS | ⚠️ Replication is plaintext |
| Unauthorised admin access | `THEMIS_TOKEN_ADMIN` | ⚠️ Admin endpoints open to anyone |
| Audit log tampering | ECDSA-signed audit chain + WORM sink | ⚠️ Logs can be modified |
| PII leakage via logs/queries | `THEMIS_PII_STRICT=1` | ⚠️ PII may leak into query results/logs |
| Model supply-chain attack | `THEMIS_DISABLE_AUTO_DOWNLOAD=1` | ⚠️ Arbitrary model files fetched at runtime |
| Debug data written to disk | Unset `THEMIS_DEBUG_ENC_DIR` / `THEMIS_DEBUG_PKI` | ⚠️ Plaintext keys/data written to disk |

---

## Actionable Integrator Checklist

For integrators embedding ThemisDB into a larger platform, the following items are the minimum security integration requirements:

1. **Inject secrets via your platform's secret store** (Kubernetes Secrets + ESO, AWS Secrets Manager, GCP Secret Manager, etc.). Never pass secrets as plain environment variables visible in pod specs or CI logs.
2. **Set `THEMIS_PRODUCTION_MODE=1` and `THEMIS_ENVIRONMENT=production`** in all non-development environments (staging included).
3. **Provision a dedicated Vault transit key** (or HSM partition) per ThemisDB cluster — do not share encryption keys between clusters.
4. **Enable mTLS on all inter-node gRPC channels** (`THEMIS_WAL_GRPC_ENABLE_MTLS=1`).
5. **Configure a SIEM sink** for audit events before accepting production traffic.
6. **Run the pre-deployment security checklist** ([CHECKLISTS/pre_deployment.md](../CHECKLISTS/pre_deployment.md)) before every release.
7. **Monitor** the `themisdb_security_events_total` Prometheus counter — a spike indicates potential intrusion.
8. **Test the HSM failover path** in staging: set `THEMIS_HSM_ENABLED=1`, simulate an HSM unreachable event, verify that ThemisDB rejects new writes rather than falling back to a stub.

---

**Document Version:** 1.0  
**Last Updated:** April 2026  
**Next Review:** May 2026  
**Related Documents:**
- [CORE_MODULE_RUNBOOK.md](RUNBOOKS/CORE_MODULE_RUNBOOK.md)
- [PRODUCTION_HARDENING_CHECKLIST.md](../security/PRODUCTION_HARDENING_CHECKLIST.md)
- [systemd unit and production drop-in](../../deploy/systemd/)
- [Kubernetes production Helm values example](examples/k8s_production_values.yaml)
- [Root SECURITY.md](../../SECURITY.md)
