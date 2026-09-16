# Auth Module Operator Runbook

> **Status:** 2026-09-16 — Initial runbook for operator-critical auth scenarios.
> **Canonical split:** `src/auth/PRODUCTION_REQUIREMENTS.md` for requirements;
> this document for operational remediation.

This runbook covers the five operator-critical failure scenarios for the ThemisDB
auth module.  Each scenario includes: **Symptom**, **Diagnosis**, and
**Remediation** steps.

---

## Scenario 1 — Token-Blacklist Backend Failure

### Symptom
- Distributed token blacklist (TBLK/v1) is not rejecting revoked tokens.
- `PROVIDER_DEGRADED` errors appear in logs for blacklist operations.
- Alert: `auth_blacklist_errors_total` counter is rising.
- Revoked JTIs are passing validation (confirmed by audit log gap).

### Diagnosis
1. Check `DistributedTokenBlacklist` log output for RocksDB error messages:
   ```
   grep "DistributedTokenBlacklist" /var/log/themisdb/auth.log | grep -i error
   ```
2. Verify RocksDB data directory health:
   ```
   ls -la /var/lib/themisdb/blacklist/
   df -h /var/lib/themisdb/
   ```
3. Check cluster sync status (if multi-node):
   ```
   curl http://localhost:9090/metrics | grep auth_blacklist
   ```
4. Confirm the fallback in-memory blacklist is active (check log for
   `"DistributedTokenBlacklist: falling back to in-memory"`).

### Remediation
1. **Short-term (< 5 min):** Restart the auth service to trigger RocksDB recovery.
2. **Medium-term:** If the RocksDB directory is corrupt, restore from the most
   recent snapshot:
   ```
   systemctl stop themisdb-auth
   cp -a /backup/blacklist/latest/ /var/lib/themisdb/blacklist/
   systemctl start themisdb-auth
   ```
3. **Disk full:** Expand the volume or purge expired entries:
   ```
   themisdb-admin blacklist purge-expired
   ```
4. **Cluster sync failure:** Verify TBLK/v1 TCP port (default 7171) is reachable
   between nodes.  Check firewall rules.
5. **Escalation:** If in-memory fallback capacity is reached (> 10 000 JTIs),
   enable emergency rate-limiting on all authenticated endpoints.

---

## Scenario 2 — Leader-Election Stall

### Symptom
- Auth workers report `AUTH_INTERNAL_ERROR` for operations that require
  cluster-leader coordination (e.g., key rotation, distributed session cleanup).
- Log entries: `"leader election timeout"`, `"no leader elected"`.
- Alert: `auth_leader_election_stall_total` counter is non-zero.

### Diagnosis
1. Check leader election state:
   ```
   grep "leader" /var/log/themisdb/auth.log | tail -50
   ```
2. Verify cluster member connectivity (all nodes must reach each other on the
   election port, default 7172):
   ```
   curl http://localhost:9090/metrics | grep auth_cluster
   ```
3. Check for split-brain: if more than one node claims leadership, investigate
   network partition.

### Remediation
1. **Timeout (transient):** Wait for the election timeout to expire (default 30 s).
   Most transient stalls self-resolve.
2. **Network partition:** Restore connectivity between nodes.  The election will
   resume automatically once quorum is restored.
3. **Permanent stall:** Force a leadership transfer:
   ```
   themisdb-admin cluster force-leader --node <node_id>
   ```
4. **Single-node fallback:** If cluster is unreachable, set
   `THEMIS_AUTH_SINGLE_NODE_MODE=1` to allow the local node to act as leader.
   **WARNING:** This disables distributed consistency guarantees.

---

## Scenario 3 — Federation-Provider Timeout

### Symptom
- `PROVIDER_DEGRADED` errors for federated token validation requests.
- Log entries: `"FederatedIdentityManager: provider error for realm"`.
- Alert: `auth_federation_provider_errors_total` rising.
- Tokens from a specific IdP realm are being rejected.

### Diagnosis
1. Identify the affected realm:
   ```
   grep "PROVIDER_DEGRADED" /var/log/themisdb/auth.log | grep "realm" | tail -20
   ```
2. Test JWKS endpoint reachability from each auth node:
   ```
   curl -m 5 https://<realm-issuer>/.well-known/jwks.json
   ```
3. Check OIDC discovery document:
   ```
   curl -m 5 https://<realm-issuer>/.well-known/openid-configuration
   ```
4. Inspect TLS certificate validity:
   ```
   openssl s_client -connect <realm-host>:443 -showcerts 2>/dev/null | openssl x509 -noout -dates
   ```

### Remediation
1. **Network timeout:** If the IdP is reachable but slow, increase
   `oidc_provider_timeout_ms` in the auth configuration (default: 5000 ms).
2. **TLS failure:** Verify the CA bundle includes the IdP's issuing CA.  Update
   `tls_ca_bundle_path` in the config if necessary.
3. **IdP outage:** Remove the affected realm temporarily to allow other realms
   to continue serving:
   ```
   themisdb-admin federation remove-realm --issuer <realm-url>
   ```
   Re-add the realm once the IdP is recovered.
4. **Cached tokens:** For up to JWT-exp seconds, cached validated tokens remain
   valid in the local in-memory cache.  No immediate action required for
   already-authenticated sessions.

---

## Scenario 4 — Session-Overflow

### Symptom
- New login attempts fail with `SESSION_LIMIT_EXCEEDED` or `AUTH_INTERNAL_ERROR`.
- Log entries: `"SessionManager: max sessions per user reached"`.
- Alert: `auth_session_create_errors_total` rising.
- Memory usage of auth service is growing.

### Diagnosis
1. Check active session count per user:
   ```
   themisdb-admin sessions list --user <user_id>
   ```
2. Inspect SessionManager metrics:
   ```
   curl http://localhost:9090/metrics | grep auth_session
   ```
3. Determine if sessions are leaking (not being invalidated on logout):
   ```
   grep "SessionManager.*invalidate" /var/log/themisdb/auth.log | wc -l
   grep "SessionManager.*create" /var/log/themisdb/auth.log | wc -l
   ```

### Remediation
1. **Immediate:** Force-expire idle sessions for affected users:
   ```
   themisdb-admin sessions expire-idle --older-than 3600s
   ```
2. **Bulk cleanup:** If the overflow is global, trigger a full session eviction
   sweep:
   ```
   themisdb-admin sessions sweep
   ```
3. **Configuration:** Reduce `max_sessions_per_user` (default: 10) or
   `session_absolute_timeout_s` in the auth config.
4. **Root cause:** If clients are not calling `/auth/logout`, investigate the
   client-side session lifecycle.  Ensure logout endpoints invalidate sessions.
5. **Memory pressure:** If memory usage is above 80%, restart the auth service
   after the sweep completes (sessions are re-created on next login).

---

## Scenario 5 — Rate-Limiter Redis Failure

### Symptom
- Rate-limiter backend (Redis) is unreachable or returning errors.
- Log entries: `"rate_limiter_backend: Redis connection failed"`,
  `"AUTH_INTERNAL_ERROR"` on rate-check operations.
- Alert: `auth_rate_limiter_backend_errors_total` rising.
- Brute-force protection may be degraded (in-memory fallback only).

### Diagnosis
1. Check Redis connectivity from the auth node:
   ```
   redis-cli -h <redis-host> -p 6379 PING
   ```
2. Inspect the auth rate-limiter log:
   ```
   grep "rate_limiter\|redis" /var/log/themisdb/auth.log | tail -30
   ```
3. Confirm whether the local in-memory rate limiter fallback is active:
   ```
   curl http://localhost:9090/metrics | grep auth_rate_limiter_mode
   ```

### Remediation
1. **Short-term (< 5 min):** The auth service automatically falls back to the
   in-memory rate limiter.  No immediate action required.  **NOTE:** In-memory
   state is not shared across nodes — per-node limits apply.
2. **Redis recovery:** Restart Redis and verify connectivity.  The rate-limiter
   backend will reconnect automatically within one polling interval (default: 10 s).
3. **Redis sentinel/cluster:** If using Redis Sentinel, verify the sentinel
   configuration is correct and that the primary has been elected.
4. **Persistent Redis failure:** If Redis is down for > 30 minutes, increase
   the in-memory rate-limit thresholds temporarily to compensate for the lack
   of cross-node coordination:
   ```
   themisdb-admin rate-limiter set-local-limit --max-failures 3 --window-s 60
   ```
5. **Escalation:** If brute-force attempts are detected during the Redis outage,
   enable IP-level blocking at the load balancer/WAF layer.

---

## References

- `src/auth/PRODUCTION_REQUIREMENTS.md` — production requirements and security assumptions
- `src/auth/ROADMAP.md` — module feature roadmap and completion status
- `src/auth/ARCHITECTURE.md` — module architecture overview
- `include/auth/auth_principal_contract.h` — §4 Fail-closed, §6 Provider capability contract
- Prometheus metrics: `auth_*` namespace
- TBLK/v1 protocol: `include/auth/distributed_token_blacklist.h`
