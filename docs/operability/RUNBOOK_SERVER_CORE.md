# Runbook: Server Core — Wave D Operability

<!-- ThemisDB | docs/operability/RUNBOOK_SERVER_CORE.md -->
<!-- Module: server | Wave: D | Status: delivered Q1 2027 -->

## Overview

This runbook covers operator-critical incident scenarios for the ThemisDB
server module.  Each section describes the trigger condition, diagnostic steps,
log patterns to search, and remediation actions.

---

## Scenario 1 — Plugin Load Failure

### Trigger
A server plugin DSO fails to load at startup or during hot-reload.  Affected
requests to plugin-backed endpoints return 503.

### Log pattern
```
[SERVER:PluginLoadFailed] plugin=<name> reason=<dlopen_error|sig_mismatch|missing_symbol>
```

### Diagnostic steps
1. Search recent logs for `[SERVER:PluginLoadFailed]`.
2. Note `plugin=` field to identify the failing DSO.
3. Check `reason=`:
   - `dlopen_error` — DSO not found or ABI mismatch; check path and build target.
   - `sig_mismatch` — signature validation rejected the DSO; re-sign or roll back.
   - `missing_symbol` — plugin was built against an older API; rebuild against current headers.
4. Run `ldd <plugin_path>` to detect missing shared library dependencies.
5. Check plugin rollback state: look for `plugin_rollback_active=true` in the same log entry.

### Remediation
| Cause | Action |
|-------|--------|
| Path not found | Update `plugin.search_path` config and restart. |
| ABI mismatch | Rebuild plugin against current `include/server/plugin_api.h`. |
| Signature invalid | Re-sign DSO with `themis-plugin-sign` and re-deploy. |
| Missing dep | Install missing shared library on all nodes. |

### Escalation
If plugin load failure persists after remediation, disable the plugin by
setting `plugin.<name>.enabled=false` and escalate to the platform team.

---

## Scenario 2 — Rate-Limit Bypass

### Trigger
Metrics or log analysis indicates a client is bypassing configured per-client
rate limits.  Observed as unexpected traffic spikes or `rate_limited_total`
counter not incrementing despite over-quota traffic.

### Log pattern
```
[SERVER:RateLimitBypass] client=<id> window=<ms> observed=<n> limit=<n> reason=<clock_skew|config_drift|state_replication_lag>
```

### Diagnostic steps
1. Search for `[SERVER:RateLimitBypass]` in the last 10 minutes.
2. Identify `client=` and `reason=`:
   - `clock_skew` — nodes disagree on window boundary; check NTP sync.
   - `config_drift` — node received an old config; trigger a config push.
   - `state_replication_lag` — distributed rate-limit state not replicated fast enough.
3. Correlate with `rate_limit_state_replication_lag_ms` metric in Prometheus.
4. Cross-check `rate_limited_total{client=<id>}` counter across all nodes.

### Remediation
| Cause | Action |
|-------|--------|
| Clock skew | Fix NTP on the affected node; restart the rate-limit worker. |
| Config drift | Push updated config via `themis-admin config push --target <node>`. |
| Replication lag | Scale the distributed state backend or reduce state TTL. |

### Escalation
If bypass events exceed 100 in a 5-minute window, trigger a security incident
and temporarily drop all traffic from the offending client CIDR at the load
balancer.

---

## Scenario 3 — WASM Sandbox OOM

### Trigger
A WASM workload exceeds its memory limit.  The sandbox is forcefully terminated
and the request returns 500 or 507.

### Log pattern
```
[SERVER:WASMOom] sandbox_id=<id> limit_bytes=<n> peak_bytes=<n> module=<name>
```

### Diagnostic steps
1. Search for `[SERVER:WASMOom]` in the last 30 minutes.
2. Identify `module=` and `peak_bytes=` vs `limit_bytes=`.
3. Check `wasm_sandbox_memory_peak_bytes` metric histogram for trends.
4. Determine if the OOM is from a specific user workload or a platform module.
5. Review WASM module's memory growth pattern — look for allocator loops.

### Remediation
| Cause | Action |
|-------|--------|
| Limit too low for legitimate workload | Increase `wasm.memory_limit_bytes` in module config. |
| Runaway allocator in module | Roll back or disable the WASM module. |
| Sustained OOM storm | Enable `wasm.oom_circuit_breaker=true` to auto-disable on repeated OOM. |

### Escalation
Repeated OOM for the same WASM module within 1 hour indicates a module defect.
File an incident and disable the module until a fixed build is available.

---

## Scenario 4 — Auth Service Unavailability

### Trigger
The upstream auth/OIDC provider is unreachable or returning errors.  All
auth-gated requests fail closed with 401/503.

### Log pattern
```
[SERVER:AuthUnavailable] provider=<url> status=<http_status|timeout|connection_refused> fail_closed=true
```

### Diagnostic steps
1. Search for `[SERVER:AuthUnavailable]` in the last 5 minutes.
2. Note `provider=` URL and `status=`.
3. Test connectivity: `curl -sk <provider_url>/.well-known/openid-configuration`.
4. Check JWKS cache age: `themis-admin auth cache-status` — if expired, the
   server will attempt a refresh on each request.
5. Review `auth_provider_request_duration_p99_ms` metric for latency trends.

### Remediation
| Cause | Action |
|-------|--------|
| Provider timeout | Increase `auth.jwks_request_timeout_ms`; check network path. |
| Certificate expired | Update TLS cert on provider; update CA bundle on ThemisDB nodes. |
| Provider overload | Enable JWKS cache with `auth.jwks_cache_ttl_s=300`. |
| Permanent outage | Activate emergency mode: `themis-admin auth bypass-mode --duration 15m` (admin only; audited). |

### Escalation
Auth unavailability affecting >5% of requests for >2 minutes is a P1 incident.
Page the auth-service on-call team and activate the incident bridge.

---

## Scenario 5 — GraphQL Schema Conflict

### Trigger
A schema push or federated service registration results in conflicting type
definitions.  GraphQL requests return 400 with schema validation errors.

### Log pattern
```
[SERVER:GraphQLSchemaConflict] type=<TypeName> conflict=<duplicate_field|type_mismatch|missing_required> source_a=<service> source_b=<service>
```

### Diagnostic steps
1. Search for `[SERVER:GraphQLSchemaConflict]` at the time of the failed deploy.
2. Identify `type=` and `conflict=` fields.
3. Run `themis-admin graphql validate-schema --federation` to reproduce the error.
4. Diff the schema from `source_a` vs `source_b` to locate the conflicting definition.
5. Check recent federated service deployments for schema changes.

### Remediation
| Cause | Action |
|-------|--------|
| Duplicate field with different type | Coordinate between service owners; use schema versioning. |
| Missing `@external` directive | Add `@external` to the field in the non-owning service. |
| Incompatible interface | Roll back the offending service to the last compatible schema version. |

### Escalation
If schema conflict blocks production traffic for >15 minutes, roll back the
most recently deployed federated service and open a schema governance review.

---

## Related Resources

- `docs/operability/WAVE_D_ROADMAP.md` — Wave D acceptance checklist
- `src/server/ROADMAP.md` — Wave D contribution items
- `include/server/server_api_contract.h` — auth/error contract constants
- Prometheus alerts: `server_plugin_load_failure_total`, `server_rate_limit_bypass_total`, `server_wasm_oom_total`, `server_auth_unavailable_total`
