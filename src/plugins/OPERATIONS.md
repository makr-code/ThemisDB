---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# Plugins Operations

## Purpose and scope

This guide describes deployment-time controls and observability for
administrators and service operators. It covers the plugin infrastructure in
`src/plugins`; each plugin's resource requirements and external services need
their own operational plan.

## Deployment controls

- Stage each manifest and binary as a reviewed pair. The runtime reads the
  manifest's platform binary path and applies edition/license checks before
  activation when using manifest-backed `loadPlugin(name)`.
- Do not use `loadPluginFromPath()` or `loadPluginFromOci()` where deployment
  policy depends on manifest edition/license restrictions: those paths do not
  read `plugin.json`. OCI verifies the downloaded layer digest, not manifest
  policy or publisher identity.
- Keep production and development build modes distinct. In `NDEBUG` builds,
  the current manager requires its manifest `.sig` hash check and configures
  the binary verifier to require signatures; development builds permit unsigned
  plugins. `THEMIS_TEST_MODE` skips the manifest check in tests. See
  [`TRUST.md`](./TRUST.md) for the exact limits of these controls.
- Restrict write access to plugin directories and cache directories to the
  deployment mechanism. Hot-plug can react to directory changes without a
  separate operator confirmation.
- Configure OCI credentials per registry according to deployment policy.
  `OciRegistryClient` defaults to a 30-second request timeout; no separate
  connect-timeout default is exposed by its current interface. Registry
  authentication may be anonymous if no credentials are configured. HTTPS peer
  and hostname verification are enabled in the client.
- Treat the `PluginHealthMonitor` pointer registration as a lifetime contract:
  the monitored plugin must outlive its registration. Review recovery and
  auto-disable settings before enabling automatic remediation.

## Monitoring and capacity

`PluginMetrics` tracks load/reload durations, function-call count and sampled
latency percentiles, errors, and memory values in process memory. The latency
sample window is bounded to 1,000 observations per plugin. A
`PluginMetricsCollector` can publish snapshots to an `IMetrics` sink; the caller
chooses the collection interval and backend.

The health monitor defaults are a 30-second check interval, three recovery
attempts, exponential backoff beginning at five seconds and capped at 300
seconds, a 10-second health-check timeout, and a 30-second recovery timeout.
These are defaults, not deployment SLOs. Validate actual plugin health-check
and recovery behavior under the target workload.

Hot-plug monitoring is disabled by default. When enabled, automatic load,
reload, and unload flags default to true; set them explicitly to match the
deployment's change-control policy. The header's polling fallback interval is
100 ms. File events for temporary/incomplete names are ignored, and create or
modify handling waits briefly for the file write to complete before attempting
the operation.

Use the measurable mappings and release-baseline policy in
[`PERFORMANCE_EXPECTATIONS.md`](./PERFORMANCE_EXPECTATIONS.md); the presence of
a benchmark case is not evidence that the current deployment meets it.

## Change, rollback, and maintenance

1. Deploy a new plugin under a unique, valid name/path and validate it in the
   target edition and build mode.
2. Check dependency ordering, expected capabilities, health, metrics, and
   service-specific integration before directing production traffic.
3. For an update, use manager reload semantics when state preservation and
   rollback behavior are appropriate. If replacement validation or
   initialization fails, the old plugin remains active according to the
   manager contract.
4. If the artifact is suspect, pause hot-plug, preserve evidence, restore the
   approved manifest/binary pair, and follow [`RUNBOOK.md`](./RUNBOOK.md).
5. Re-enable automatic file monitoring only after the directory and rollout
   process are stable.

## Operational limits

The plugin metrics store and health statistics are in-memory; these interfaces
do not establish durable audit retention. OCI digest verification checks layer
content, not publisher identity. Native plugins share the host process failure
domain. Plan persistent audit, artifact provenance, isolation, and service
recovery at the deployment/security layer rather than assuming this module
provides them.
