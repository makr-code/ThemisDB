# RUNBOOK: Core Module — Operator Remediation Guide

**Module:** `core`
**Wave:** D (Q1 2027)
**Maintainers:** ThemisDB Platform Team
**Last Updated:** 2026-09-16

---

## Overview

This runbook covers operator-critical incident scenarios for the ThemisDB
`core` module. Each scenario includes diagnostic log patterns, triage steps,
and remediation actions.

Log patterns use structured prefixes of the form `[CORE:<EVENT>]` and
appear in the application log stream (spdlog, JSON lines format).

---

## Scenario 1 — Bootstrap Failure (`[CORE:BootstrapFailed]`)

### Symptoms
- Log line: `[CORE:BootstrapFailed] module=<name> reason=<reason>`
- Service fails to start or enters a degraded state on startup
- Health check returns `503` immediately after deployment

### Triage
1. Check the specific `module` field in the log to identify the failing subsystem.
2. Verify configuration files are present and valid JSON/YAML.
3. Check for missing required environment variables (e.g., `THEMIS_CONFIG_PATH`).
4. Inspect plugin loading logs for adapter ABI mismatches or missing `.sig` files.
5. Review `AdapterRegistry` logs for registration failures.

### Remediation
- If config missing: restore configuration from backup and restart.
- If plugin ABI mismatch: rebuild the plugin against the current API version
  (`kCurrentApiVersion`).
- If adapter signature missing: re-sign the adapter or disable
  `AdapterTrustPolicy::kRequireSignature` for the failing adapter (dev only).
- If environment variable missing: set it and restart.

---

## Scenario 2 — Config Reload Failure (`[CORE:ConfigReloadFailed]`)

### Symptoms
- Log line: `[CORE:ConfigReloadFailed] key=<key> reason=<reason>`
- Live config reload via `POST /admin/config/reload` returns an error
- Running config diverges from the stored config after a reload attempt

### Triage
1. Validate the new config file syntax: `themis-validate-config <path>`.
2. Check file permissions on the config path.
3. Identify which `key` failed from the log.
4. Look for concurrent reload requests (race condition in reload path).

### Remediation
- Fix config syntax errors and re-trigger reload.
- If permissions issue: `chmod 644 <config_path>` and retry.
- If concurrent reload race: implement reload lock at the config service layer.
- Roll back to the previous config if the new one is invalid:
  `POST /admin/config/rollback`

---

## Scenario 3 — Tracing Exporter Lag (`[CORE:TracingExporterLag]`)

### Symptoms
- Log line: `[CORE:TracingExporterLag] exporter=<name> lag_ms=<ms> backlog=<n>`
- Distributed traces missing or delayed in Jaeger/Tempo
- `themis_core_exporter_backlog` metric rising above threshold

### Triage
1. Check the tracing backend (Jaeger/Tempo) connectivity:
   `curl -s http://jaeger:14268/api/traces`.
2. Verify network between ThemisDB and tracing backend.
3. Check exporter buffer size: `core_tracing_exporter_buffer_size` config key.
4. Inspect `ConcernsContext` propagation: ensure spans are being closed properly
   (`startSpanFromHeaders` / `injectContext` balanced calls).

### Remediation
- If backend unreachable: check network/firewall and restore connectivity.
- If buffer overflow: increase `core_tracing_exporter_buffer_size`.
- If spans not closed: fix the caller to always close spans (RAII guard recommended).
- If lag is transient: the exporter will drain automatically once backend recovers.
- Restart exporter if it is stuck: `POST /admin/core/restart_exporter`

---

## Scenario 4 — Module Init Failure (`[CORE:ModuleInitFailed]`)

### Symptoms
- Log line: `[CORE:ModuleInitFailed] module=<name> api_version=<v> reason=<reason>`
- A plugin or adapter fails to initialize during hot-plug or startup
- `AdapterRegistry::register()` returns non-OK status

### Triage
1. Check `apiVersion` in adapter metadata vs `kCurrentApiVersion`.
2. Verify the adapter's `.sig` file is present if `kRequireSignature` is active.
3. Check if `hot_swap_timeout_ms` (default: `kHotSwapTimeoutMs = 100`) was exceeded.
4. Inspect adapter initialization return code (non-zero = failure).

### Remediation
- If API version mismatch: rebuild adapter against current `adapter_metadata.h`.
- If signature missing: re-sign adapter with `SignedAdapterValidator`.
- If hot-swap timeout: investigate adapter init performance or increase timeout.
- If init returns non-zero: check adapter-specific logs for the root cause.
- Disable the failing adapter and restart to restore service without it.

---

## Scenario 5 — General Module Degradation / Circuit Breaker Open

### Symptoms
- Multiple log patterns firing simultaneously
- Circuit breaker open on core module: `CircuitBreaker state=OPEN`
- `AdapterRegistry` rejecting all registrations

### Triage
1. Identify the triggering event from logs (BootstrapFailed, ModuleInitFailed, etc.).
2. Check `CircuitBreaker` state via admin API: `GET /admin/core/circuit_breaker`.
3. Verify no cascading failures from dependent modules.

### Remediation
1. Resolve root cause (see relevant scenario above).
2. Reset circuit breaker: `POST /admin/core/circuit_breaker/reset`.
3. Re-initialize failed adapters via hot-plug: `POST /admin/core/adapters/reload`.
4. Validate all adapters are registered: `GET /admin/core/adapters`.
5. File post-incident report and update this runbook.

---

## Alert Reference

| Alert Name                    | Log Pattern                      | Severity |
|-------------------------------|----------------------------------|----------|
| `CoreBootstrapFailed`         | `[CORE:BootstrapFailed]`         | Critical |
| `CoreConfigReloadFailed`      | `[CORE:ConfigReloadFailed]`      | High     |
| `CoreTracingExporterLag`      | `[CORE:TracingExporterLag]`      | Medium   |
| `CoreModuleInitFailed`        | `[CORE:ModuleInitFailed]`        | High     |

---

## Related Documents

- `src/core/ROADMAP.md` — Wave D contribution
- `tests/integration/test_core_soak.cpp` — soak test coverage
- `tests/core/test_core_highcardinality_stress.cpp` — stress coverage
- `docs/operability/WAVE_D_ACCEPTANCE_CHECKLIST.md` — sign-off checklist
