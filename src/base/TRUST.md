---
Author: "ThemisDB Contributors"
Created: "2026-10-21"
Last Updated: "2026-10-21"
Status: "active"
AI_Used: true
AI_Models: "GitHub Copilot, MAI-Code-1.1-Flash"
AI_Contributor: "GitHub Copilot"
Human_Review: true
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Base Module Trust and Security Boundaries

## Purpose

The base module is the runtime trust boundary for dynamically loaded and replaced code. It exists to ensure that extension modules do not become an implicit bypass around ThemisDB’s normal safety and validation controls.

## Trust boundaries

The module separates trust into four zones:

1. Artifact trust
   - binary existence,
   - cryptographic validation,
   - signature or hash acceptance,
   - revocation and policy checks.

2. Compatibility trust
   - ABI major/minor compatibility,
   - required-symbol presence,
   - declared dependency constraints.

3. Execution trust
   - sandbox limits,
   - WASM allowlist and linear-memory boundaries,
   - OS-level process or cgroup limits.

4. Operational trust
   - reload correctness,
   - rollback availability,
   - registry authenticity and checksum verification.

These boundaries are reflected directly in the code and the error taxonomy.

## Key controls

### Module verification

`ModuleSecurityVerifier` uses a policy-backed verification path and is designed to fail closed when verification is required. The design is conservative: if the module signature or hash policy fails, the module is not treated as safely loadable.

### ABI validation

`AbiChecker` enforces required symbol sets and ABI compatibility before a module is allowed to activate. This is an important trust control because a module with wrong symbol contracts can destabilize the host or behave unpredictably under execution.

### Dependency validation

`PluginDependencyGraph` and the dependency declarations in `ModuleMetadata` prevent unsafe ordering or impossible version combinations. A cycle or missing dependency is treated as a correctness issue, not as a tolerated state.

### Sandbox and WASM constraints

`ModuleSandbox` and `WasmPluginSandbox` provide the last runtime fence. They enforce memory, CPU, and capability restrictions and make explicit what the host allows a plugin to do. This is the critical boundary against untrusted or resource-heavy plugin execution.

### Registry integrity

`RemoteRegistryClient` validates remote artifact downloads using credentials, TLS settings, and SHA-256 checksums. This ensures the trusted path is not bypassed by unverified registry content.

## Risks and mitigations

### Risk: untrusted or tampered artifact

Mitigation:

- trust policy enforcement,
- signature verification,
- checksum validation,
- revocation and policy checks.

### Risk: incompatible runtime contract

Mitigation:

- ABI and required symbol checks,
- version mismatch rejection,
- dependency graph and ordering validation.

### Risk: dependency poisoning or cycle

Mitigation:

- cycle detection,
- required/optional dependency distinction,
- deterministic topological ordering.

### Risk: resource exhaustion or runaway plugin behavior

Mitigation:

- sandbox caps,
- cgroup or job-object limits,
- WASM linear-memory and host-function allowlist restrictions.

### Risk: unsafe live upgrade

Mitigation:

- atomic candidate load before replacement,
- rollback state preservation,
- strict validation before activation and after reload.

## Incident handling

A trust incident in the base module should be treated as a runtime safety event:

1. identify which boundary failed — artifact, compatibility, dependency, sandbox, or rollout,
2. map the failure to the taxonomy code,
3. quarantine or disable the module if it cannot be safely revalidated,
4. restore the last known-good version when a live replace failed,
5. inspect registry, trust policy, and dependency configuration before re-enabling the artifact.

## Security posture

The base module is intentionally conservative. The main security posture is “fail closed, validate explicitly, and preserve rollback.” This is the correct posture for a runtime layer that sits below the application and extension stack, because silent acceptance would quickly turn a small plugin issue into a platform-wide trust problem.
