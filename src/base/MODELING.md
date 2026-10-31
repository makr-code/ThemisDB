---
Author: "ThemisDB Contributors"
Created: "2026-10-21"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot coding agent"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Base Module Modeling Notes

## Model summary

The base area is modeled as a lifecycle-control layer around modular execution.
The loader tracks more than file existence; validation depends on configured
checks, isolation is optional/platform-dependent, and module state is reported
through the lifecycle API.

The concrete modeling primitives are:

- `LoadStage` for lifecycle progression in `module_loader.h`
- `ModuleMetadata` and `ModuleDependency` for compatibility and dependency declarations
- `ModuleFailureHistory` for failure accounting and backoff/quarantine behavior
- `PluginDependencyGraph` for dependency topology and ordering
- `ModuleSandbox` and `WasmPluginSandbox` for execution boundaries
- `HotReloadManager` for candidate loading, reload results, and rollback state
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

The hot reload model is an operational state machine rather than a simple file replacement. Its current implementation creates an availability gap while replacing a module; emergency restoration after a failed final load is best-effort. Its key steps include:

1. validate the registered module and ensure the old version is still the current one,
2. save state where configured,
3. notify `BEFORE_UNLOAD`,
4. load the candidate artifact under a temporary key,
5. unload the old module and temporary candidate,
6. load the candidate again under the canonical name,
7. preserve rollback information according to configuration and operation outcome.

The implementation has explicit transition/error handling, but it does not guarantee uninterrupted availability or successful emergency restoration.

## Invariants

The source implies the following invariants:

- configured verification/health checks must pass before the loader advances through the corresponding activation stages,
- incompatible ABI versions must not be activated,
- optional dependencies do not create hard-load ordering requirements,
- callers must inspect reload results; rollback is configurable and only available when a backup exists,
- configured runtime constraints apply only where the relevant backend/platform supports them.

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

Any change that modifies module loading, dependency ordering, WASM runtime injection, or reload semantics must preserve source-backed checks and report unresolved behavior rather than claiming stronger guarantees. A patch that weakens verification or allows unsafe activation without an explicit contract change alters the trust model of the module.
