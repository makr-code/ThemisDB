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

> Exact model backend not verifiable from final document text alone.

# Security boundaries — plugins module

Report vulnerabilities through the project-level
[`../../SECURITY.md`](../../SECURITY.md). This page summarizes source-visible
controls; it is not a security certification or a complete deployment policy.
See [`TRUST.md`](./TRUST.md) for precise trust semantics and
[`PRODUCTION_REQUIREMENTS.md`](./PRODUCTION_REQUIREMENTS.md) for unresolved
policy questions.

## Load-path distinctions

| Path/control | Source-visible behavior | Limit |
|---|---|---|
| `loadPlugin(name)` | Uses a discovered manifest, dependency declarations, edition/license checks, manifest checks, and binary verification before activation. | Manifest `.sig` verification is a SHA-256 comparison in `NDEBUG`, not a public-key signature. |
| `loadPluginFromPath(path, config)` | Applies binary verification and native library/export/initialization checks. | Does not parse `plugin.json`; manifest edition/license rules are not applied. |
| `loadPluginFromOci(...)` | Retrieves the plugin binary layer, checks its digest, then delegates to path loading. | Layer digest is not publisher authentication; manifest policy is not applied. |
| `SignedPluginRepository` | Verifies catalog entries using active pinned Ed25519 keys and canonical manifest serialization. | Separate catalog API; its checks are not automatically used for every manager load. |
| `loadWasmPlugin(...)` | Checks an expected hash and runtime license gate on the built-in path; a registered loader callback is consulted first. | Runtime availability depends on build flags. The module does not establish a general sandbox guarantee. |

Native shared libraries execute in the host process. A defect or malicious
plugin can affect the server process; unloading is not a containment boundary.
Capabilities are snapshotted after initialization, but escalation detection is
an explicit manager operation, not an implicit check on every call.

## Deployment responsibilities and open policy

Protect plugin and cache directory writes, scope registry credentials, preserve
artifact provenance, and choose durable audit retention at the deployment
boundary. The current module API allows anonymous OCI retrieval when no
credentials are configured and does not provide durable audit retention.

**TODO / blocking policy gap:** the source does not identify a single approved
production policy for publisher identity across every load path, native-code
isolation, mandatory registry authentication, or audit retention. Do not
describe these as enforced module controls until the policy owner and evidence
are identified.

Primary evidence: `plugin_manager.cpp`, `oci_registry_client.cpp`,
`signed_plugin_repository.cpp`, `wasm_plugin_loader.cpp`, and the corresponding
headers under [`../../include/plugins/`](../../include/plugins/).
