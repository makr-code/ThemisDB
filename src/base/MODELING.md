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

# Base Module Modeling Notes

## Model summary

The base module is modeled as a lifecycle-control layer around modular execution. The primary state model is not “module exists on disk,” but “module is known, validated, isolated, and either active or recoverable.”

The concrete modeling primitives are:

- `LoadStage` for lifecycle progression in `module_loader.h`
- `ModuleMetadata` and `ModuleDependency` for compatibility and dependency declarations
- `ModuleFailureHistory` for failure accounting and backoff/quarantine behavior
- `PluginDependencyGraph` for dependency topology and ordering
- `ModuleSandbox` and `WasmPluginSandbox` for execution boundaries
- `HotReloadManager` for atomic swap + rollback states
- `ABTestManager` for experiment-controlled rollout decisions

## Structural model

### 1. Module identity and trust

A module is represented by a logical name plus metadata and a loaded artifact path. The loader keeps enough metadata to answer:

- is it loaded?
- what version is it?
- what ABI did it declare?
- what are its declared dependencies?
- is it in a quarantined or retry-backoff state?

This is modeled by `ModuleMetadata`, `ModuleFailureHistory`, and `LoadStage` in `module_loader.h`.

### 2. Dependency graph

The dependency model is intentional and explicit. Module dependency declarations are not optional metadata—they are scheduling constraints. `PluginDependencyGraph` tracks nodes and directed edges, then answers questions such as:

- are there cycles?
- which modules must load before others?
- which dependencies are required versus optional?
- what is the topological ordering that preserves the intended runtime contract?

This is important because each module may declare `minVersion` / `maxVersion` ranges and required dependencies, and the system needs to reject impossible orderings before activation.

### 3. Sandbox and isolation model

Execution boundaries are modeled as a layered concept:

- OS-level constraints: cgroups / Job Objects / resource caps
- WASM linear memory and capability boundaries
- host-function allowlist for runtime imports
- runtime selection via `WasmRuntimeInjector`

`ModuleSandbox::Config` and `WasmPluginSandbox::Config` model the runtime controls needed to contain a plugin without assuming a single platform capability.

### 4. Reload model

The hot reload model is an operational state machine rather than a simple file replacement. The key lifecycle steps are:

1. validate the registered module and ensure the old version is still the current one,
2. save state where configured,
3. notify `BEFORE_UNLOAD`,
4. load the candidate artifact,
5. unload the old module,
6. finalize the new module under the canonical name,
7. preserve a rollback slot until the new version is stable.

The design does not permit a lazy “just point the registry at a new binary” operation. It requires a deterministic transitional path.

## Invariants

The source implies the following invariants:

- a module is not considered active until verification and health checks pass,
- incompatible ABI versions must not be activated,
- optional dependencies do not create hard-load ordering requirements,
- a reload must not mutate the old module identity silently while the new binary is still unproven,
- rollback must remain available while the old module can still be repointed,
- runtime execution must remain constrained under memory and CPU pressure.

## Failure domains

The real failure domains in this module are:

- artifact availability and path integrity,
- trust verification and revoked or invalid signatures,
- ABI compatibility and missing symbol contracts,
- dependency ordering and cycle detection,
- sandbox launch failures and OS capability gaps,
- hot reload failure and state restoration issues,
- remote registry HTTP/auth/download failures.

These are represented in the public taxonomy (`BaseErrorTaxonomy`) and explicitly coded as error ranges and remediation hints.

## Relationship to higher-level modules

The base module sits below the application and extension layers. Its modeling assumptions are intentionally conservative:

- the server or plugin framework decides what to load,
- the base layer decides whether the artifact is valid and safe enough to run,
- the hot reload manager decides whether replacement is safe enough to transition,
- the dependency graph decides whether ordering is legal.

This separation is the foundation of the module’s safety model.

## Consequences for changes

Any change that modifies module loading, dependency ordering, WAASM runtime injection, or reload semantics must preserve the above invariants. A patch that weakens verification or allows unsafe activation without an explicit contract change is not just a low-level bug; it alters the trust model of the entire module.
