<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Security — Chimera Module Public Headers

**Module Path:** `include/chimera/`  
**Implementation Security:** `../../src/chimera/SECURITY.md`

---

## Scope

Security considerations for the Chimera multi-database adapter public header API surface.
Covers credential handling, connection security, query injection, and cross-database
data leakage risks.

---

## Threat Model

| Threat | Vector | Mitigation Header |
|--------|--------|------------------|
| Credential exposure in adapter config | Plaintext passwords in `AdapterConfig` | Config types use `SecureBuffer` for credential fields |
| SQL/NoSQL injection via query parameters | Unsanitised parameters in `IDatabaseAdapter::query()` | Parameterised query API; raw string queries rejected |
| Connection pool exhaustion (DoS) | Malicious tenant opening excessive connections | `connection_pool.hpp` — `max_connections` and per-tenant pool quotas |
| Unencrypted database connections | Missing TLS in adapter config | All `*Config` structs require `tls_config` or `ssl_mode` field |
| Cross-adapter data leakage in federated queries | `ThemisDBAdapter` cross-tenant routing | Adapter requires `tenant_id` context; cross-tenant queries rejected |
| Retry amplification attack | Attacker triggers excessive retries | `retry_policy.hpp` — `max_retries` and `max_elapsed_ms` bounds |
| Vector store data exfiltration | Unrestricted Pinecone/Qdrant/Weaviate namespace access | Adapter config requires namespace scoped to tenant |

---

## Security Controls

### Credential Security
All `*Config` credential fields use `SecureBuffer` from `../../src/auth/include/secure_memory.h`;
credentials are zeroed on destruction.

### Parameterised Queries
`IDatabaseAdapter::query()` accepts a `QueryParams` map; raw string interpolation is not
supported in the public API. Adapters must reject unparameterised queries.

### TLS Enforcement
All adapter configs include a `tls_config` or `ssl_mode` field; connections without TLS
are rejected in production mode (see `production_mode.h`).

### Connection Pool Limits
`PoolConfig::max_connections` and `PoolConfig::max_per_tenant_connections` prevent
pool exhaustion. Idle connections are reaped after `max_idle_time_ms`.

### Retry Bounds
`RetryConfig::max_retries` and `RetryConfig::max_elapsed_ms` bound retry amplification;
infinite retry loops are a contract violation.

---

## Known Limitations

- Cross-adapter join security (`ICrossAdapterJoin`, planned Q4 2026) will require
  additional tenant-scoping enforcement not yet defined in the header contract.
- Vector store namespace scoping (Pinecone, Qdrant, Weaviate) is enforced by adapter
  config but is not validated server-side; operators must also configure namespace ACLs
  in the vector store.
- The Elasticsearch adapter uses HTTP; operators must ensure HTTPS is configured in
  `ElasticsearchConfig`.
