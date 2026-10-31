# Auth Module Operator Runbook

- **AI_Used:** true
- **AI_Models:** unknown
- **AI_Contributor:** GitHub Copilot
- **Human_Review:** false
- **Human_Author:** unknown
Exact model backend not verifiable from final document text alone.

> **Status:** 2026-09-16 — initial runbook; the source-backed notes below
> replace previously listed commands and metrics that are not established by
> the auth implementation.
>
> **Canonical split:** `src/auth/PRODUCTION_REQUIREMENTS.md` states operational
> requirements; this document records verified diagnostic surfaces and limits.

This runbook is for maintainers triaging authentication failures. The auth
headers and implementations do not define a general `themisdb-admin` command
set, HTTP metrics endpoint, alert-name schema, or environment-variable
interface. Do not execute undocumented commands or assume specific metric names,
ports, fallback modes, or default deployment configuration.

## 1. JWT/JWKS validation failures

1. Check the `JWTValidatorConfig` supplied by the embedding application,
   including `jwks_url`, `expected_issuer`, and `expected_audience`.
2. The URL-only `JWTValidator` constructor disables issuer and audience
   requirements. The config constructor defaults both requirements to true and
   rejects missing expected values.
3. The default JWKS cache TTL is 600 seconds. Account for it when investigating
   key-set changes; the actual refresh and provider behavior depends on the
   configured endpoint.
4. `parseAndValidate()` returns `JWTClaims` or throws `std::runtime_error`.
   Preserve the rejection; do not treat an unverified claim or refresh failure
   as successful authentication.

## 2. Distributed token-blacklist synchronization

`DistributedTokenBlacklist` is configured with a RocksDB path, a local node,
peer nodes, `enable_cluster_sync`, and sync/timeout intervals. The header's RPC
port default is 9090; deployments may configure a different port.

- Use `getReplicationStats()` to inspect the component's in-process counters
  when the embedding service exposes the object.
- `syncWithCluster()` returns a future with a boolean result.
- `waitForClusterConvergence(timeout)` returns true after observing at least
  one successful background sync; it does **not** prove that every peer has
  received all entries.
- Leader selection is local by lexicographically lowest node ID. The code does
  not implement quorum consensus.
- Entry application overwrites stored values without comparing update times.
  Conventional last-write-wins conflict resolution or a bounded convergence
  guarantee is therefore not established.

If synchronization fails, keep the deployment's security decision policy
explicit and investigate peer connectivity and RocksDB errors through the
embedding application's available logs. Auth sources do not define a supported
cluster-admin command or a universal recovery procedure.

## 3. Session state

`SessionManager` stores sessions in an in-memory map and exposes `size()`,
`listSessions(user_id)`, `validateSession()`, `pruneExpired()`, and termination
methods. It does not expose a persistent session backend or an admin endpoint.
Use the embedding service's own supported interface to inspect or terminate
sessions. Do not assume a process restart preserves session state.

## 4. Provider and rate-limiter failures

- LDAP configuration exposes connection/search timeouts, pooling controls, and
  an asynchronous `authenticateAsync()` API. Check the configured endpoint and
  timeout values in the embedding application.
- Provider errors and result types vary by adapter. Use the specific header
  contract rather than assuming every adapter reports `AuthError`.
- Rate-limiter backends are selected/configured by the embedding application.
  The auth source does not establish a universal Redis-failure fallback or
  cross-node recovery procedure.

## 5. Unsupported operational assumptions

The following are not verified by the auth source/build configuration and must
not be treated as supported runbook steps unless a separate deployment contract
documents them:

- `themisdb-admin` commands for blacklist, cluster, session, or rate-limiter
  management;
- fixed metrics names or a built-in `:9090/metrics` endpoint;
- election ports, quorum behavior, or a single-node environment-variable
  override;
- automatic cross-node rate-limiter fallback or guaranteed session persistence.

Escalate through the service's documented operational process when the required
deployment-level interface is unclear.

## References

- `include/auth/jwt_validator.h` and `src/auth/jwt_validator.cpp`
- `include/auth/distributed_token_blacklist.h` and `src/auth/distributed_token_blacklist.cpp`
- `include/auth/session_manager.h` and `src/auth/session_manager.cpp`
- `include/auth/ldap_authenticator.h` and `src/auth/ldap_authenticator.cpp`
- `include/auth/rate_limiter_backend.h` and `src/auth/rate_limiter_backend.cpp`
- `src/auth/PRODUCTION_REQUIREMENTS.md`
