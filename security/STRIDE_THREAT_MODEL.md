# ThemisDB – STRIDE Threat Model

**Version:** 1.0  
**Status:** Production-Ready (Security Module v2)  
**Methodology:** STRIDE (Microsoft Threat Modelling)  
**Letzte Aktualisierung:** 2026-02

---

## Inhaltsverzeichnis

1. [System Overview](#1-system-overview)
2. [Trust Boundaries](#2-trust-boundaries)
3. [STRIDE Threat Analysis](#3-stride-threat-analysis)
4. [Mitigation Summary Table](#4-mitigation-summary-table)
5. [Residual Risks](#5-residual-risks)

---

## 1. System Overview

ThemisDB is a multi-tenant, graph-augmented vector database with an HTTP/gRPC API surface. The security-critical data flows are:

```
Internet clients
      │  HTTPS / gRPC-TLS
      ▼
┌─────────────────────────────────────────┐
│  HTTP Server  (rate limiter, CORS, CSP)  │
│  Auth Middleware  (JWT / mTLS / GSSAPI)  │
└────────────┬────────────────────────────┘
             │
    ┌────────▼────────┐
    │  Policy Engine  │  ← AuditLogger (hash-chain)
    │  (RBAC / ABAC)  │
    └────────┬────────┘
             │
    ┌────────▼────────────────────────────┐
    │  Query Engine  (AQL parser + exec)   │
    │  Storage Engine  (RocksDB / HNSW)    │
    │  Encryption Layer  (AES-256-GCM)     │
    └─────────────────────────────────────┘
             │
    ┌────────▼──────────────────────────────┐
    │  Shard / Raft / WAL / Backup Manager  │
    └───────────────────────────────────────┘
```

**Key assets:**
- Stored data (graph nodes, vectors, documents)
- JWT signing keys / JWK key set
- Application secrets (SecretManager)
- Audit log chain
- Policy definitions

---

## 2. Trust Boundaries

| Boundary | Description |
|----------|-------------|
| **B1** Internet → HTTP Server | Public untrusted network |
| **B2** HTTP Server → Policy Engine | Internal authenticated context |
| **B3** Policy Engine → Storage Engine | Trusted internal call |
| **B4** Storage Engine → Disk / RocksDB | OS-level trust |
| **B5** Admin CLI → Admin API | Elevated trust; mTLS required |
| **B6** Shard ↔ Shard (Raft/gRPC) | Mutual TLS between shards |

---

## 3. STRIDE Threat Analysis

### 3.1 Spoofing (Identitätsfälschung)

| # | Threat | Component | Impact | Mitigation | Status |
|---|--------|-----------|--------|------------|--------|
| S1 | JWT forgery with weak/no signature | Auth Middleware | High | RS256 / ES256 / EdDSA; `alg:none` blocked in JWTValidator | ✅ Mitigated |
| S2 | JWT with revoked kid | JWTValidator | High | JWTKeyRotationManager: revoked kid denylist checked on every validation | ✅ Mitigated |
| S3 | Replayed JWT after user logout | TokenBlacklist | Medium | JTI-based TokenBlacklist with TTL; `isRevoked()` checked before auth | ✅ Mitigated |
| S4 | Stolen service-account credentials | SecretManager | High | Argon2id (OpenSSL ≥ 3.2) / PBKDF2-SHA256 fallback password hashing; secret rotation pipeline; no plain-text storage | ✅ Mitigated |
| S5 | IP address spoofing to bypass IP allowlists | PolicyEngine | Medium | ABAC IP conditions are advisory; primary auth remains JWT/mTLS | ⚠️ Partial (IPv6 spoofing in NAT environments) |
| S6 | mTLS client certificate forgery | gRPC / HTTP2 | High | Client cert validated against trusted CA at TLS layer | ✅ Mitigated |

### 3.2 Tampering (Manipulation)

| # | Threat | Component | Impact | Mitigation | Status |
|---|--------|-----------|--------|------------|--------|
| T1 | Audit log manipulation | AuditLogger | Critical | SHA-256 hash chain; each log entry contains hash of previous entry | ✅ Mitigated |
| T2 | Policy file modification on disk | PolicyEngine | Critical | `reloadIfChanged()` detects mtime changes; file should be permission-restricted (0640, root:themis) | ⚠️ OS hardening required |
| T3 | In-flight data tampering | HTTP Server | High | TLS 1.2+ enforced; HSTS header set | ✅ Mitigated |
| T4 | RocksDB SST file tampering | Storage Engine | High | AES-256-GCM field-level encryption; checksums on WAL | ✅ Mitigated |
| T5 | AQL injection via query parameter | AQL Engine | High | AqlInjectionDetector: AST-based detection; parameterised queries enforced | ✅ Mitigated |
| T6 | JWT payload tampered (kid swap) | JWTValidator | High | kid-to-algorithm binding validated; HS/RS/ES/EdDSA dispatch strict | ✅ Mitigated |

### 3.3 Repudiation (Nichtanerkennung)

| # | Threat | Component | Impact | Mitigation | Status |
|---|--------|-----------|--------|------------|--------|
| R1 | User denies performing action | AuditLogger | High | Structured audit log with user_id, action, resource, timestamp, request_id | ✅ Mitigated |
| R2 | Admin denies policy change | PolicyEngine + AuditLogger | High | `POLICY_UPDATED` events emitted on add/remove/reload with actor | ✅ Mitigated |
| R3 | Key rotation not traceable | JWTKeyRotationManager + AuditLogger | High | `KEY_ROTATED` / `KEY_DELETED` events with kid and rotation counter | ✅ Mitigated |
| R4 | SIEM log forwarding failure not detected | AuditLogger | Medium | SIEM push errors are logged locally; future: SIEM delivery receipts | ⚠️ Enhancement planned |

### 3.4 Information Disclosure (Informationsoffenlegung)

| # | Threat | Component | Impact | Mitigation | Status |
|---|--------|-----------|--------|------------|--------|
| I1 | Secret values leaked in logs | AuditLogger / SecretManager | Critical | `listVersions()` omits secret values; InputValidator sanitises log output | ✅ Mitigated |
| I2 | Stack trace / internal error in HTTP response | HTTP Server | Medium | Error responses return generic messages; detailed errors logged internally only | ✅ Mitigated |
| I3 | PII in query results not masked | Query Engine | High | PII API handler and PII detector; `X-Content-Type-Options: nosniff` | ✅ Mitigated |
| I4 | JWT claims exposure via alg:none | JWTValidator | High | `alg:none` explicitly rejected; no fallback to unsigned tokens | ✅ Mitigated |
| I5 | Key material in memory after rotation | JWTKeyRotationManager | Medium | REVOKED keys removed from validation path; signing key management is external | ⚠️ OS memory scrubbing not enforced in C++ |
| I6 | Verbose CORS error exposing allowed origins | HTTP Server | Low | CORS error returns generic 403; origin list not echoed in error body | ✅ Mitigated |

### 3.5 Denial of Service (Dienstverweigerung)

| # | Threat | Component | Impact | Mitigation | Status |
|---|--------|-----------|--------|------------|--------|
| D1 | HTTP flood / DDoS | RateLimiter | High | Per-IP and per-user token bucket; adaptive throttling with penalty factor | ✅ Mitigated |
| D2 | Policy table exhaustion | PolicyEngine | Medium | `max_policies` resource cap (Config::max_policies); throws `std::length_error` | ✅ Mitigated |
| D3 | JWT blacklist table exhaustion | TokenBlacklist | Medium | `max_entries` cap with LRU pruning; configurable TTL | ✅ Mitigated |
| D4 | Key table exhaustion in rotation manager | JWTKeyRotationManager | Low | `max_keys` cap (Config::max_keys); throws `std::length_error` | ✅ Mitigated |
| D5 | Secret table exhaustion | SecretManager | Low | `max_secrets` and `max_versions_per_secret` caps; throw `std::length_error` | ✅ Mitigated |
| D6 | Expensive AQL queries | AQL Engine | High | Query timeout; rate limiting per user; adaptive throttle penalty | ✅ Mitigated |
| D7 | Memory exhaustion via large request body | HTTP Server | High | Request body size limit enforced; chunked transfer controlled | ✅ Mitigated |

### 3.6 Elevation of Privilege (Rechteausweitung)

| # | Threat | Component | Impact | Mitigation | Status |
|---|--------|-----------|--------|------------|--------|
| E1 | RBAC bypass via wildcard subject | PolicyEngine | Critical | Wildcard `"*"` subject only matches explicit allow; deny rules evaluated first | ✅ Mitigated |
| E2 | JWT with inflated roles/groups | JWTValidator | High | Group membership validated against `allowed_groups` in JWTConfig; no self-issued escalation | ✅ Mitigated |
| E3 | Horizontal privilege escalation (user A accesses user B data) | PolicyEngine | High | Resource path includes user-scoped segment; ABAC conditions enforced | ✅ Mitigated |
| E4 | Time-window ABAC bypass via clock skew | PolicyEngine | Low | UTC hour window uses server clock; NTP sync recommended; window overlap acceptable | ⚠️ NTP required |
| E5 | Plugin loading arbitrary code | Plugin Manager | High | Plugin security sandbox; HSM stub gating; signed plugin manifests | ✅ Mitigated |
| E6 | Admin API accessible without elevated auth | Admin API Handler | Critical | Admin endpoints require `admin` action in PolicyEngine; separate mTLS client cert | ✅ Mitigated |

---

## 4. Mitigation Summary Table

| STRIDE Category | Total Threats | Fully Mitigated | Partial / Enhancement | Notes |
|-----------------|:---:|:---:|:---:|-------|
| Spoofing | 6 | 5 | 1 | IPv6 NAT spoofing advisory only |
| Tampering | 6 | 5 | 1 | Policy file needs OS permissions |
| Repudiation | 4 | 3 | 1 | SIEM delivery receipts planned |
| Information Disclosure | 6 | 5 | 1 | Memory scrubbing out of scope |
| Denial of Service | 7 | 7 | 0 | All resource caps in place |
| Elevation of Privilege | 6 | 5 | 1 | NTP sync required for ABAC |
| **TOTAL** | **35** | **30 (86%)** | **5 (14%)** | |

---

## 5. Residual Risks

| Risk | Likelihood | Impact | Accepted / Planned |
|------|------------|--------|-------------------|
| IPv6 NAT spoofing bypasses IP ABAC allowlists | Low | Medium | Accepted – IP conditions are defence-in-depth only; JWT/mTLS is primary |
| Policy file tampering if OS permissions misconfigured | Low | Critical | Deployment hardening required: file mode 0640, owned by `themis` service account |
| SIEM forwarding failure leaves audit gap | Low | Medium | Planned: SIEM delivery acknowledgement queue with retry |
| Signing key material remnants in process heap | Low | Medium | Accepted – mitigated by OS-level memory protection; future: secure memory allocator |
| Clock skew > 30 min bypasses ABAC time window | Very Low | Low | NTP synchronisation is a deployment prerequisite |

---

*This document should be reviewed whenever the threat landscape changes significantly (new API surface, new authentication mechanism, new data classification).*
