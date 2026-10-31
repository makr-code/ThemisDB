---
Author: "ThemisDB Contributors"
Created: "2026-09-16"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot coding agent"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Base Module — Operator Runbook

<!-- Status: Wave D delivery | created: 2026-09-16 -->
<!-- Links: README.md · ROADMAP.md · ARCHITECTURE.md · include/themis/base/base_error_taxonomy.h -->

This runbook summarizes source-defined error-code groups in
`include/themis/base/base_error_taxonomy.h` and offers diagnostic suggestions.
The presence of a taxonomy entry does not prove that every integration emits
that code or exposes the same operator surface.
Native modules run in the ThemisDB process; `ModuleSandbox` resource limits
apply to the current process rather than isolating each native module. Linux
cgroup/`RLIMIT` and Windows Job Object controls are platform/setup-dependent.
The inspected network, filesystem, and syscall restriction methods warn and
skip. A successful sandbox launch is not proof that every configured control
took effect. The normal `ModuleLoader` verifier path does not call the
implemented CRL/OCSP methods, so this runbook does not assert revocation checks.
The canonical loader also supports an explicitly loaded hash manifest and a
separate `PluginBundleLoader` Ed25519-signature path when a key is configured;
it defines revoked-certificate error/reporting values, which do not prove that
the normal path checks revocation. The bundle signature covers the exact
manifest bytes, not the referenced binary bytes directly. Unsigned bundles
require explicit opt-in; without a public key and opt-in the loader rejects the
bundle.
The repository sources inspected do not define a `themisdb-admin` executable,
the example Prometheus metric names below, or the referenced deployment
configuration keys. Treat those commands, metric names, and settings as
illustrative placeholders that must be mapped to the operator's actual
deployment. Registry clients accept HTTP as well as HTTPS; HTTP emits a warning
and does not provide transport encryption.

Use API field names only where declared in the headers. For example,
`RegistryConfig` includes `auth_token`, `api_key`, `download_dir`, `timeout_ms`,
`max_retries`, `max_total_retry_time_ms`, `verify_ssl`, `ca_bundle_path`, and
`pinned_public_key`; it does not define an application deployment mapping.
Sample keys such as `sandbox.operation_timeout_s` and `sandbox.allow_degraded`
are not `ModuleSandbox::Config` fields. Treat sample paths and deployment
settings as placeholders unless the owning application's mapping confirms them.

---

## Table of Contents

1. [BASE\_LOADER\_\* (1100–1149) — Module Load Failures](#base_loader_-1100-1149--module-load-failures)
2. [BASE\_SANDBOX\_\* (1150–1199) — Sandbox Lifecycle Failures](#base_sandbox_-1150-1199--sandbox-lifecycle-failures)
3. [BASE\_RELOAD\_\* (1200–1249) — Hot-Reload and Rollback Failures](#base_reload_-1200-1249--hot-reload-and-rollback-failures)
4. [BASE\_DEP\_\* (1250–1299) — Dependency Graph Failures](#base_dep_-1250-1299--dependency-graph-failures)
5. [BASE\_REGISTRY\_\* (1300–1349) — Remote Registry Failures](#base_registry_-1300-1349--remote-registry-failures)
6. [Prometheus Alert Rules (Reference)](#prometheus-alert-rules-reference)

---

## BASE\_LOADER\_\* (1100–1149) — Module Load Failures

### Error codes

| Code | Name | Description |
|------|------|-------------|
| 1100 | `BASE_LOADER_PATH_NOT_FOUND` | Module binary not found at the supplied path |
| 1101 | `BASE_LOADER_SIGNATURE_REJECTED` | Module signature verification failed |
| 1102 | `BASE_LOADER_ABI_MISMATCH` | Module ABI version incompatible with host |
| 1103 | `BASE_LOADER_LOAD_FAILED` | OS-level library load failed (dlopen/LoadLibrary) |
| 1104 | `BASE_LOADER_INIT_FAILED` | Module initialization function returned failure |
| 1105 | `BASE_LOADER_HEALTH_CHECK_FAILED` | Staged-loading health check failed |

### Symptoms

- ThemisDB logs contain `[BASE_LOADER_PATH_NOT_FOUND:1100]` or similar codes.
- Module registry shows module in `UNLOADED` or `FAILED` state.
- Dependent queries or plugins return `MODULE_UNAVAILABLE` errors.

### Diagnostic commands

```bash
# Example only: adapt the service name/log backend to the deployment.
journalctl -u themisdb --since "10 minutes ago" | grep BASE_LOADER

# Generic filesystem checks; replace the example path.
ls -lh /path/to/plugin.so
file /path/to/plugin.so

# Platform-specific example (Linux).
ldd /path/to/plugin.so

# Platform-specific examples; these tools may not be installed.
ausearch -m AVC -ts recent | grep themisdb
journalctl -k | grep apparmor | grep themisdb

# A file hash alone does not establish signature trust.
openssl dgst -sha256 /path/to/plugin.so
```

### Remediation

**1100 — PATH\_NOT\_FOUND**
1. Confirm the deployment script copied the module to the configured path.
2. Confirm the configured path through the owning application's supported configuration.
3. Verify the process user has `r-x` permission on the file and parent directory.

**1101 — SIGNATURE\_REJECTED**
1. Identify the verifier used by the caller and compare the artifact metadata,
   supplied signature/hash, and expected issuer against that verifier's policy.
2. Where the active verifier exposes a signing certificate, check its validity
   dates; this is not evidence that revocation is checked.
3. Compare the artifact's SHA-256 with the value from the trusted release
   process. Do not assume a generic OS keyring is used by `ModuleLoader`.
4. If `setHashManifest()` is configured, confirm the module name has an entry;
   modules absent from the manifest are not rejected by that manifest check.

**1102 — ABI\_MISMATCH**
1. Rebuild the module against the current ThemisDB SDK headers.
2. Compare the module's declared ABI metadata against the host contract.
3. Deploy the correct module version for the running ThemisDB release.

**1103 — LOAD\_FAILED**
1. Run `ldd /path/to/plugin.so` to find missing shared libraries.
2. Install missing packages or set `LD_LIBRARY_PATH` if needed.
3. Check `dlerror()` output logged at the `[BASE_LOADER_LOAD_FAILED:1103]` line.

**1104 — INIT\_FAILED**
1. Check module-specific init logs (usually emitted just before the 1104 entry).
2. Verify all required environment variables and config keys are set.
3. Confirm that external services the module connects to are reachable.

**1105 — HEALTH\_CHECK\_FAILED**
1. Identify the failing health check from the diagnostic message.
2. Confirm all staged-loading prerequisites (ports, databases, endpoints) are up.
3. If the owning application provides a retry setting, review its configured budget.

---

## BASE\_SANDBOX\_\* (1150–1199) — Sandbox Lifecycle Failures

### Error codes

| Code | Name | Description |
|------|------|-------------|
| 1150 | `BASE_SANDBOX_LAUNCH_FAILED` | ModuleSandbox::launch() returned false |
| 1151 | `BASE_SANDBOX_RESOURCE_LIMIT` | Sandbox exceeded memory or CPU limit |
| 1152 | `BASE_SANDBOX_TIMEOUT` | Sandbox operation timed out |
| 1153 | `BASE_SANDBOX_DEGRADED` | Sandbox running in degraded state (partial constraints only) |
| 1154 | `BASE_SANDBOX_INACTIVE_STATS` | stats() called on inactive (unstarted) sandbox |

### Symptoms

- Module loaded but sandboxing warnings in logs.
- `[BASE_SANDBOX_DEGRADED:1153]` with a list of unsupported constraints.
- Module terminated with `[BASE_SANDBOX_RESOURCE_LIMIT:1151]`.
- Operations against the module hang until `[BASE_SANDBOX_TIMEOUT:1152]` fires.

### Diagnostic commands

```bash
# Check kernel support for required sandbox primitives
uname -r
grep CONFIG_SECCOMP /boot/config-$(uname -r)
grep CONFIG_CGROUPS /boot/config-$(uname -r)

# List cgroup v2 controllers available to the process
cat /sys/fs/cgroup/cgroup.controllers

# Check ulimits for the ThemisDB process
cat /proc/$(pidof themisdb)/limits

# Inspect sandbox warnings in logs
journalctl -u themisdb --since "1 hour ago" | grep BASE_SANDBOX
```

### Remediation

**1150 — LAUNCH\_FAILED**
1. Check the `lastError()` detail in the 1150 log line for the root cause.
2. Inspect `ModuleSandbox::launchWarnings()` for controls that were skipped.
3. Do not assume network namespaces, filesystem restrictions, or seccomp were
   applied; the inspected methods warn and skip those controls.

**1151 — RESOURCE\_LIMIT**
1. Review the actual `ModuleSandbox::Config` fields and deployment mapping before changing limits.
2. Confirm whether the configured cgroup, `RLIMIT`, or Job Object limit took
   effect on the current process; this is not per-plugin process isolation.
3. Otherwise investigate memory usage using deployment-appropriate tooling.
4. Collect a heap profile before terminating the host process if recurrence is suspected.

**1152 — TIMEOUT**
1. Review the actual sandbox/operation timeout contract; no `sandbox.operation_timeout_s` API field was found.
2. Check the module for blocking I/O or deadlocks (attach `gdb` or use `strace -p`).
3. If the timeout recurs, consider rolling back to the previous module version.

**1153 — DEGRADED**
1. Read the unsupported constraint list from the diagnostic.
2. Upgrade the host kernel if the missing features are security-critical.
3. If degraded state is acceptable under policy, record the acceptance in the change record; no `sandbox.allow_degraded` API field was found.

---

## BASE\_RELOAD\_\* (1200–1249) — Hot-Reload and Rollback Failures

### Error codes

| Code | Name | Description |
|------|------|-------------|
| 1200 | `BASE_RELOAD_NO_BACKUP` | rollback() called but no backup slot is available |
| 1201 | `BASE_RELOAD_ROLLBACK_FAILED` | Reload of the backup binary failed during rollback |
| 1202 | `BASE_RELOAD_CANDIDATE_LOAD_FAILED` | New module binary failed to load during hot-reload |
| 1203 | `BASE_RELOAD_STATE_RESTORE_FAILED` | State restore callback returned failure |
| 1204 | `BASE_RELOAD_NOT_REGISTERED` | Module is not registered with `HotReloadManager` |

### Symptoms

- `[BASE_RELOAD_NO_BACKUP:1200]` after attempting a rollback before any successful reload.
- `[BASE_RELOAD_CANDIDATE_LOAD_FAILED:1202]` when deploying a new module version.
- Module reverts to a previous version unexpectedly.

### Diagnostic commands

```bash
# Placeholder: no built-in admin CLI contract was found in the inspected sources.
# Obtain module state through the deployment's supported management surface.

# List registered modules and backup availability
# Inspect the HotReloadManager state through the owning application.

# Placeholder: use the deployment's supported management API.
# Invoke rollback through the owning application only when its API allows it.

# Inspect reload stats
# Inspect HotReloadManager::getStats() through the owning application.
```

### Remediation

**1200 — NO\_BACKUP**
- A rollback can only happen after at least one successful `reloadModule()`.
- Restore the module by redeploying the known-good binary and calling `reloadModule()`.

**1201 — ROLLBACK\_FAILED**
1. The backup binary is no longer accessible (deleted or unmounted).
2. Redeploy the known-good binary to the backup path and retry.
3. If the path is unknown, obtain the registered module history through the
   owning application's supported management surface (no built-in
   `themisdb-admin` command was found in the inspected sources).

**1202 — CANDIDATE\_LOAD\_FAILED**
1. The new binary has a loader error — follow BASE\_LOADER\_\* remediation above.
2. In the initial candidate-load failure branch, the old module remains loaded;
   this does not cover a later canonical-name load failure after the old module
   has been unloaded.
3. Fix the new binary's issue and retry the deployment.

The `HotReloadManager` loads a candidate under a temporary key, unloads the old
and candidate modules, then loads under the canonical name. This creates an
availability gap; after a failed final load, restoration from the prior path is
best-effort. `Config::sandboxConfig` is optional and empty by default, and
`Config::verifySignature` is currently unused; verification is controlled by
the associated `ModuleLoader` policy.

**1203 — STATE\_RESTORE\_FAILED**
1. Examine the state-restore callback logs for the specific failure.
2. The module loaded successfully; only the state is missing (degraded operation).
3. Reinitialise the module's runtime state via its admin API if available.

### Manual rollback procedure

```bash
# 1. Verify rollback is available
# Check HotReloadManager::isRollbackAvailable() through the owning application.

# 2. Trigger rollback
# Request rollback through the owning application's supported management API.

# 3. Confirm the version reverted
# Confirm version through ModuleLoader/HotReloadManager in the owning application.

# 4. Monitor for 5 minutes
journalctl -u themisdb -f | grep <module_name>
```

---

## BASE\_DEP\_\* (1250–1299) — Dependency Graph Failures

### Error codes

| Code | Name | Description |
|------|------|-------------|
| 1250 | `BASE_DEPENDENCY_CONFLICT` | Two loaded modules declare incompatible requirements for a shared dependency |
| 1251 | `BASE_DEPENDENCY_CYCLE` | Dependency graph contains a cycle |
| 1252 | `BASE_DEPENDENCY_MISSING_REQUIRED` | A required dependency is not registered |
| 1253 | `BASE_DEPENDENCY_VERSION_RANGE_MISMATCH` | Version range requirement cannot be satisfied |

### Symptoms

- Module activation fails with `[BASE_DEPENDENCY_CONFLICT:1250]`.
- Startup log shows `[BASE_DEPENDENCY_CYCLE:1251]` with a cycle path.
- Hot-reload of module A fails because it would break module B's dependency.

### Diagnostic commands

```bash
# The public PluginDependencyGraph API can export DOT/JSON/ASCII and report
# cycles. No admin CLI commands for this API were found in the inspected sources.
```

### Remediation

**1250 — CONFLICT**
1. Identify the conflicting modules from the diagnostic (they declare incompatible version ranges for the same shared library).
2. Update one or both modules to align on a compatible version range.
3. If alignment is not possible, consider loading them in separate sandboxes.

**1251 — CYCLE**
1. Use the DOT export to visualise the cycle.
2. Break the cycle by extracting the shared behaviour into a separate module that both depend on.

**1252 — MISSING\_REQUIRED**
1. Identify the missing dependency from the diagnostic.
2. Load/register the missing module before the dependent one.
3. Update the deployment script to enforce load order.

**1253 — VERSION\_RANGE\_MISMATCH**
1. Check which version of the dependency is currently loaded.
2. Update the dependent module to widen its accepted version range, or update the dependency to a version in the required range.

---

## BASE\_REGISTRY\_\* (1300–1349) — Remote Registry Failures

### Error codes

| Code | Name | Description |
|------|------|-------------|
| 1300 | `BASE_REGISTRY_NETWORK_ERROR` | HTTP request to the registry failed (network-level) |
| 1301 | `BASE_REGISTRY_AUTH_FAILURE` | Authentication or authorisation rejected by registry |
| 1302 | `BASE_REGISTRY_CHECKSUM_MISMATCH` | Downloaded plugin binary SHA-256 does not match registry metadata |
| 1303 | `BASE_REGISTRY_DOWNLOAD_FAILED` | Plugin binary download failed after all retries |

### Symptoms

- `[BASE_REGISTRY_NETWORK_ERROR:1300]` with `CURL error:` detail.
- `[BASE_REGISTRY_AUTH_FAILURE:1301]` with `HTTP 401` or `HTTP 403`.
- `retry_exhausted_count` counter increasing in observability dashboards.
- Plugin installations fail silently or with timeout errors.

### Diagnostic commands

```bash
# Example only; replace the endpoint and credentials with deployment values.
curl -v https://registry.example.com/api/v1/health

# Verify auth token validity (replace with actual endpoint)
curl -H "Authorization: ******" \
     https://registry.example.com/api/v1/plugins | head -c 200

# Check retry-exhaust metrics (Prometheus example)
# themisdb_registry_retry_exhausted_total

# Inspect RemoteRegistryClient::lastRequestStats() through the owning application.

# Verify TLS configuration
openssl s_client -connect registry.example.com:443 -showcerts </dev/null 2>&1 | grep -E "Verify|CN="
```

### Remediation

**1300 — NETWORK\_ERROR**
1. Confirm network connectivity to the registry URL from the ThemisDB host.
2. Check firewall rules and proxy configuration.
3. If transient, the retry logic (max\_retries, exponential back-off) will resolve it automatically.

**1301 — AUTH\_FAILURE**
1. Rotate `RegistryConfig::auth_token` or `api_key` using the owning
   application's secret-management process; this client does not store or
   rotate credentials.
2. Confirm token validity/authorization with the registry provider; expiry,
   scopes, and audience rules are provider-specific.
3. Use HTTPS with `verify_ssl=true` unless an explicitly reviewed test setup
   requires otherwise; HTTP is accepted by the client with a warning.

**1302 — CHECKSUM\_MISMATCH**
1. Compare the registry entry's expected SHA-256 with the value from the
   independent release artifact; the client cannot determine why they differ.
2. The client removes the downloaded file on a hash mismatch. If no expected
   `sha256` was supplied, the client skips this integrity check instead.
3. Ask the registry owner to validate the metadata and artifact before retrying.

**1303 — DOWNLOAD\_FAILED**
1. Check `max_retries` and `max_total_retry_time_ms` in `RegistryConfig` — they may be too low for the network latency.
2. Verify the `download_url` in the registry entry is reachable.
3. Check `download_dir` has sufficient disk space and write permissions.

### Observability counters

`RemoteRegistryClient` accumulates the following lifetime counters in `RequestStats`:

| Counter | Description |
|---------|-------------|
| `retry_exhausted_count` | Number of request sequences that exhausted all retry budget |
| `timeout_count` | Number of individual attempts that timed out (CURLE\_OPERATION\_TIMEDOUT) |

These counters can be observed through the `ObservabilityHook` callback. The
metric names in this example are user-chosen; the base API does not register
Prometheus metrics itself:

```cpp
client->setObservabilityHook([](const RequestStats& stats) {
    // Export to Prometheus, StatsD, OpenTelemetry, etc.
    metrics::record("registry.retry_exhausted", stats.retry_exhausted_count);
    metrics::record("registry.timeout",         stats.timeout_count);
});
```

---

## Prometheus Alert Rules (Reference)

The following alert definitions and metric names are examples only; they are
not emitted or registered by this module. Map them to metrics exported by the
application and adjust thresholds to its SLA and baseline behaviour.

```yaml
# Alert: registry retry-exhaust rate is elevated
- alert: ThemisBaseRegistryRetryExhausted
  expr: rate(themisdb_registry_retry_exhausted_total[5m]) > 0.1
  for: 2m
  labels:
    severity: warning
    module: base
  annotations:
    summary: "ThemisDB registry client is exhausting retries"
    description: "More than 0.1 retry-exhausted events/s in the last 5 minutes. Check registry connectivity and auth."
    runbook: "src/base/RUNBOOK.md#base_registry_-1300-1349--remote-registry-failures"

# Alert: module load failure rate is elevated
- alert: ThemisBaseModuleLoadFailures
  expr: rate(themisdb_module_load_failed_total[5m]) > 0
  for: 1m
  labels:
    severity: critical
    module: base
  annotations:
    summary: "ThemisDB base module load failures detected"
    description: "Module load failures in the last 5 minutes. Immediate investigation required."
    runbook: "src/base/RUNBOOK.md#base_loader_-1100-1149--module-load-failures"

# Alert: hot-reload failure rate is elevated
- alert: ThemisBaseHotReloadFailures
  expr: rate(themisdb_hot_reload_failed_total[5m]) > 0.05
  for: 5m
  labels:
    severity: warning
    module: base
  annotations:
    summary: "ThemisDB hot-reload failures elevated"
    description: "More than 0.05 reload failures/s in the last 5 minutes."
    runbook: "src/base/RUNBOOK.md#base_reload_-1200-1249--hot-reload-and-rollback-failures"

# Alert: sandbox degraded state detected
- alert: ThemisBaseSandboxDegraded
  expr: themisdb_sandbox_degraded_total > 0
  for: 0m
  labels:
    severity: warning
    module: base
  annotations:
    summary: "ThemisDB module sandbox is running in degraded state"
    description: "One or more modules are sandboxed with partial constraints. Review unsupported capabilities."
    runbook: "src/base/RUNBOOK.md#base_sandbox_-1150-1199--sandbox-lifecycle-failures"
```

---

*Last updated: 2026-10-31 — source/build-definition reconciliation*
*Maintainer: ThemisDB base module team*
*Source of Truth: `include/themis/base/base_error_taxonomy.h`*
