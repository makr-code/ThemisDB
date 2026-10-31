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

# Plugins Incident Runbook

## Purpose and safety

Use this runbook to triage plugin discovery, load, reload, health, and retrieval
incidents. Plugin operations can execute native code inside the server process.
For suspected compromise or process instability, prioritize service isolation
and the deployment's incident procedure over live experimentation.

## Initial triage

1. Record plugin name, version, edition, manifest and binary paths, build mode,
   recent deployment change, and the exact manager error.
2. Query `PluginManager::getDiagnosticsForPlugin(name)` and inspect
   `PluginManager::getMetrics()` for lifecycle/error counts. If a health monitor
   is attached, inspect `getPluginStats(name)` and the latest monitoring event.
3. Preserve the manifest, adjacent `.sig` file, binary hash, relevant server
   logs, and OCI reference/digest before replacing files.
4. Determine whether the incident is in manifest discovery, artifact retrieval,
   validation, library construction, initialization, runtime health, or unload.

The core module provides APIs and in-memory metrics, not a universal operator
CLI or persistent plugin incident log.

Record which entry point performed the load. Manifest-backed `loadPlugin(name)`
applies manifest edition/license checks; explicit-path and OCI loading do not
read `plugin.json` and do not enforce those restrictions.

## Symptoms and response

| Symptom | Diagnose | Safe response |
|---|---|---|
| Plugin is not discovered | Confirm `plugin.json` exists, parses, has `name`, `version`, and a `binary` object; check name characters and directory layout. | Correct the manifest/artifact through normal release review, rescan, and inspect the returned result. Do not weaken name validation. |
| Load fails validation | For `loadPlugin(name)`, separate edition/license rejection, manifest-side hash mismatch, binary-policy rejection, dependency failure, and missing library/symbol errors. For path/OCI loading, investigate binary-policy, library/symbol, initialization, registry, and digest failures; those routes do not apply manifest edition/license restrictions. | Verify the approved manifest and artifact as a pair. Prefer manifest-backed loading when edition/license policy is required; do not copy development-mode behavior into production. |
| OCI pull fails | Check reference syntax, registry reachability, configured credentials, timeout, manifest response, expected media type, and layer digest. | Restore registry/network/auth configuration or use an approved cached artifact. A matching digest does not establish publisher identity. |
| WASM load fails | Determine whether the application registered a `WasmPluginLoadFn` and review that callback's own validation, runtime, and error contract. Otherwise check `THEMIS_WASM_SUPPORT`, the selected Wasmtime/WasmEdge flags, `enterprise_plugins` eligibility, and the expected SHA-256. | Correct the callback/build/runtime/license or restore the artifact matching the reviewed hash. Without an injected callback or enabled built-in path, the loader returns `nullptr`. Do not assume the callback applies the built-in hash/license checks. |
| Hot reload fails | Check plugin is loaded, dependent plugins, replacement ABI/symbols, initialization, and state restoration. | The manager's reload contract keeps the old instance active when replacement fails. Leave it in service only if its risk is acceptable; otherwise follow the service isolation procedure. |
| Plugin is unhealthy | Inspect diagnostics, consecutive failures, recovery attempts, and configured backoff/auto-disable policy. | Stop repeated recovery if it worsens the incident; disable monitoring only as a deliberate operational decision, then unload or isolate the plugin using the normal lifecycle path. |
| Repeated hot-plug actions | Check watched path, event type, `auto_load`/`auto_reload`/`auto_unload`, and whether a deployment tool is replacing files. | Disable hot-plug through `PluginManager::disableHotPlug()` during controlled artifact repair; perform one explicit scan/load after files are complete. |

## Recovery and rollback

- For an invalid replacement, keep the old active instance if it is healthy and
  security review permits; correct the replacement and retry through
  `reloadPlugin()`.
- For a bad deployed artifact, stop further automatic discovery, restore the
  last approved manifest and binary as a matched pair, then perform a controlled
  load or reload and verify health.
- If native plugin behavior threatens the host process, do not rely on unloading
  to contain it. Drain or stop the server using its service-level procedure and
  escalate to the plugin owner and security responders.
- Preserve key and registry evidence before removing a trusted key or clearing
  the catalog. For suspected publisher compromise, revoke/rotate trust material
  through the deployment's approved key-management process.

## Escalation

Escalate immediately when validation accepts an artifact that cannot be tied to
approved provenance, a capability escalation is reported, signature/hash
behavior differs by build mode, a native plugin destabilizes the server, or
recovery leaves registry state inconsistent. See [`TRUST.md`](./TRUST.md) for
the trust model and [`OPERATIONS.md`](./OPERATIONS.md) for routine controls.
