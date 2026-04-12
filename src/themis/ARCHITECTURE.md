# Themis Module — Architecture Guide

**Version:** 1.0  
**Last Updated:** 2026-04-06  
**Module Path:** `src/themis/`

---

## 1. Overview

The Themis module is ThemisDB's main orchestration and core framework layer. It contains
the top-level `ThemisDB` class, query router, module coordinator, lifecycle manager,
edition manager, module hash verifier, and dependency resolver. It serves as the
integration point where all other modules are wired together at startup.

In the current monolithic build, much of the planned functionality for this module lives
in `src/core/`, `src/security/`, and `src/server/`. Formal modularization of these
components into `src/themis/` is planned for v1.7.0+.

---

## 2. Design Principles

- **Single Startup Orchestrator** – `ThemisDB` (main class) owns the startup sequence:
  load edition config → verify modules → initialize storage → start server.
- **Dependency-Ordered Init** – `module_coordinator.cpp` resolves module dependency
  graph and initializes modules in topological order.
- **Edition Gating** – `edition_manager.cpp` gates features by license tier (Community,
  Professional, Enterprise, Hyperscaler); all feature checks route through here.
- **Module Hash Verification** – `module_hash_verifier.cpp` verifies the SHA-256 hash
  of each module shared library before loading to prevent tampering.
- **Graceful Shutdown** – `lifecycle_manager.cpp` manages ordered shutdown: drain
  in-flight requests → flush WAL → stop modules in reverse initialization order.

---

## 3. Component Architecture

### 3.1 Key Components

| File | Role |
|---|---|
| `edition_manager.cpp` | License tier management and feature gates |
| `module_dependency_resolver.cpp` | Module dependency graph resolution |
| `module_hash_verifier.cpp` | SHA-256 verification of module binaries |
| `wire_protocol_server.cpp` | Binary wire protocol server entry point |

*(ThemisDB main class, query_router, module_coordinator, lifecycle_manager are planned for v1.7.0+)*

### 3.2 Component Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                  ThemisDB Process (main)                        │
└──────────────────────────┬──────────────────────────────────────┘
                           │
┌──────────────────────────▼──────────────────────────────────────┐
│                   LifecycleManager (startup)                     │
│                                                                  │
│  1. edition_manager.validateLicense()                           │
│  2. module_hash_verifier.verifyAll()                            │
│  3. module_dependency_resolver.resolve() → sorted init order    │
│  4. initialize modules: storage → index → query → server → ...  │
│  5. start server → accept connections                           │
└──────────────────────────────────────────────────────────────────┘

shutdown:
┌──────────────────────────────────────────────────────────────────┐
│  1. stop accepting new connections                               │
│  2. drain in-flight requests (graceful timeout)                  │
│  3. flush WAL                                                    │
│  4. shutdown modules in reverse order                           │
└──────────────────────────────────────────────────────────────────┘
```

---

## 4. Data Flow

### 4.1 Feature Gate Check

```
vector_index.search(query)
    │
    ├─ edition_manager.isFeatureEnabled("vector_search") ?
    │       Community: false (CPU-only) → reject with upgrade prompt
    │       Professional+: true → proceed
    │
    └─ search executes
```

### 4.2 Module Initialization Order

```
module_dependency_resolver: build dependency graph
    │
    ├─ core (no deps) → init first
    ├─ storage (depends on: core) → init after core
    ├─ index (depends on: storage, core) → init after storage
    ├─ query (depends on: index, storage, metadata) → init after index
    ├─ server (depends on: query, auth, governance) → init last
    │
    └─ start server when all modules ready
```

---

## 5. Integration Points

| Direction | Module | Interface |
|---|---|---|
| **Orchestrates** | All modules | Startup, shutdown, edition gating |
| **Uses** | `src/security/` | License signature verification |
| **Provides** | All consumers | `edition_manager.isFeatureEnabled()` |
| **Provides** | CLI tools | Build info, edition info |

---

## 6. Threading & Concurrency Model

- Startup and shutdown are single-threaded (sequential by design).
- `edition_manager.isFeatureEnabled()` is lock-free read after initialization.
- `module_hash_verifier` runs synchronously at startup; fails fast on tampered modules.

---

## 7. Performance Architecture

| Technique | Detail |
|---|---|
| Edition feature cache | Feature flags cached in-memory after license validation |
| Startup profiling | Module init times logged for startup performance analysis |

---

## 8. Security Considerations

- Module hash verification prevents loading of tampered or replaced shared libraries.
- License signature uses Ed25519; license key is embedded at build time.
- Edition gating is enforced server-side; client cannot bypass by sending different requests.

---

## 9. Configuration

| Parameter | Default | Description |
|---|---|---|
| `themis.edition` | "community" | Edition: community/professional/enterprise/hyperscaler |
| `themis.license_key` | "" | License key string |
| `themis.module_verify` | true | Verify module hashes at startup |
| `themis.startup_timeout_s` | 30 | Max startup time before abort |
| `themis.shutdown_timeout_s` | 30 | Max graceful shutdown time |

---

## 10. Error Handling

| Error Type | Strategy |
|---|---|
| License validation failure | Log error; run in Community mode (or abort, configurable) |
| Module hash mismatch | Abort startup; log security alert |
| Circular module dependency | Abort startup; log error with cycle path |
| Module initialization failure | Abort startup; log failing module |

---

## 11. Known Limitations & Future Work

- Full modularization (`ThemisDB` main class, `QueryRouter`, `LifecycleManager`) is
  planned for v1.7.0+.
- Currently much logic lives in `src/server/http_server.cpp` and `src/core/`.
- Hot-reload of individual modules (without restart) is planned.

---

## 12. References

- `src/themis/README.md` — module overview
- `docs/EDITION_FEATURES.md` — edition feature matrix
- `docs/architecture/MODULE_ARCHITECTURE.md` — module dependency diagram
- `ARCHITECTURE.md` (root) — full system architecture
