---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Acceleration trust boundaries

## Purpose and scope

This document identifies what the acceleration layer trusts and what controls it applies. It is for backend/plugin integrators and incident responders. [SECURITY.md](./SECURITY.md) remains the module security summary; [PRODUCTION_REQUIREMENTS.md](./PRODUCTION_REQUIREMENTS.md) states production requirements. Higher-level authentication and authorization belong to the calling service and are not provided by backend selection.

## Trust boundaries and controls

| Boundary | Risk | Control and residual limit |
|---|---|---|
| Host driver, runtime, and vendor SDK | A compromised or incompatible driver may misreport devices or corrupt execution | Discovery and backend health/capability checks gate selection, but do not sandbox or attest a vendor runtime |
| Dynamically loaded plugin | Path traversal, tampering, unsigned or untrusted code | Loader validates the path, verifies the plugin before loading, logs security events, and in non-Windows builds rejects group/world-writable files and files over 128 MiB |
| Plugin provenance | Unauthorized binary or metadata | `PluginSecurityPolicy` supports signatures, trusted issuers, hashes, revocation, whitelists, and blacklists; the loader requires signatures in `NDEBUG` builds and permits unsigned plugins in development builds |
| Shader bytes | Substituted or corrupted SPIR-V/GLSL artifacts | `ShaderIntegrityVerifier` hashes bytes against registered SHA-256 values; strict mode rejects missing entries. Strict mode is initialized `false`, so deployments must explicitly configure and populate it if this enforcement is required |
| Model path and tensor input | Untrusted path, malformed tensor, or resource exhaustion | Callers must authorize model paths and validate input ownership/shape. ONNX inference reports load/run errors, but the dispatcher does not provide a general model sandbox or application authorization policy |
| Fallback dispatch | Unsafe execution after a failed or unavailable backend | Invalid capability/request or integrity failures remain errors; documented fallback applies only to paths that explicitly implement it and must be checked by the caller |

Plugin security events contain plugin paths, hashes, messages, timestamps, and severity. Protect exported audit data according to deployment logging policy; avoid sending sensitive paths or artifacts to unauthorized recipients.

## Security review and incident response

1. On a plugin denial, preserve the event type and reason, inspect the binary hash and metadata through the approved software supply-chain process, and keep the plugin unloaded until the signer/policy is verified.
2. On shader hash failure or missing strict-mode registration, stop that execution path. Rebuild or restore the expected artifact from the trusted source and validate the manifest before retrying.
3. On driver/runtime anomalies, stop routing the affected workload to that backend using the deployment's approved rollback procedure. The module does not claim to isolate a faulty driver from the process.
4. Escalate suspected compromise to the security owner and acceleration maintainer. Preserve build identity, host/runtime versions, relevant audit events, and a minimal reproducer; redact secrets and unrelated user data.

## Deployment requirements and limits

- Use production build policy for production plugin loading; do not use development-mode unsigned-plugin behavior in a production deployment.
- Manage trusted issuers, signatures, manifests, and model files through the organization's approved supply-chain controls.
- Keep authorization at the service boundary. A valid accelerator or plugin signature is not authorization for a user request.
- Validate the actual build's enabled enforcement settings. A registered control or header default does not establish that it is enabled in a particular deployment.

## Source references

- [Plugin loader](./plugin_loader.cpp)
- [Plugin policy and auditor interfaces](../../include/acceleration/plugin_security.h)
- [Plugin verifier](./plugin_security.cpp)
- [Shader verification API](../../include/acceleration/shader_integrity.h)
- [Shader verification behavior](./shader_integrity.cpp)
- [Production requirements](./PRODUCTION_REQUIREMENTS.md)

**Exact model backend not verifiable from final document text alone.**
