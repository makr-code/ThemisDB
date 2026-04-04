# HotReloadManager — Plugin Hot-Reload without Database Restart

## Overview

The `HotReloadManager` enables ThemisDB plugins and modules to be replaced at
runtime without stopping the database process.

Key capabilities:

- **Atomic swap** – the new binary is fully loaded and initialised *before* the
  old one is unloaded, so a failed update leaves the running module untouched.
- **Rollback** – one backup slot is kept per module; a single `rollback()` call
  re-activates the previous version.
- **State preservation** – optional save/restore callbacks allow stateful
  plugins to survive a reload cycle.
- **Phase notifications** – registered callbacks are invoked at every lifecycle
  phase (`BEFORE_UNLOAD`, `AFTER_UNLOAD`, `AFTER_LOAD`, `ROLLBACK`).
- **Thread-safe** – read-only queries use a shared reader/writer lock
  (`std::shared_mutex`) allowing multiple concurrent readers; write operations
  (`reloadModule`, `rollback`, registration) hold an exclusive lock.

## Header

```cpp
#include "themis/base/hot_reload_manager.h"
```

## Quick Start

```cpp
using namespace themis::modules;

// Create a manager (default config: signature verification on, rollback on)
HotReloadManager mgr;

// Register the module with the loader that currently owns it.
// The loader must outlive the manager.
ModuleLoader loader;
mgr.registerModule("themis_storage", loader);

// When a new build lands on disk:
auto result = mgr.reloadModule("themis_storage", "/opt/themis/themis_storage.so");
if (result.success) {
    // Module is live on the new version.
    spdlog::info("Reloaded: {} -> {}", result.previousVersion, result.newVersion);
} else {
    spdlog::error("Reload failed: {}", result.errorMessage);
    // Old module is still running.  Optionally roll back explicitly:
    mgr.rollback("themis_storage");
}
```

## Configuration

```cpp
HotReloadManager::Config cfg;
cfg.verifySignature = true;   // Verify digital signature of new binary (default: true)
cfg.preserveState   = true;   // Invoke state save/restore callbacks (default: true)
cfg.enableRollback  = true;   // Keep backup slot for rollback (default: true)

HotReloadManager mgr(cfg);
```

## API Reference

### Lifecycle

| Method | Description |
|--------|-------------|
| `registerModule(name, loader)` | Register a module so it can be hot-reloaded |
| `unregisterModule(name)` | Remove the module and free its backup slot |
| `reloadModule(name, new_path)` | Atomically replace a module from a new binary path |
| `rollback(name)` | Reactivate the previous version (if a backup is available) |

### Queries

| Method | Returns |
|--------|---------|
| `getCurrentVersion(name)` | `std::optional<ModuleVersion>` |
| `isRollbackAvailable(name)` | `bool` |
| `registeredModules()` | `std::vector<std::string>` |

### Callbacks

```cpp
// Notified at each reload phase.
mgr.addReloadCallback(
    [](const std::string& name, HotReloadManager::ReloadPhase phase) {
        if (phase == HotReloadManager::ReloadPhase::AFTER_LOAD)
            spdlog::info("Module '{}' reloaded successfully", name);
    });

// Save plugin state before the old binary is unloaded.
mgr.setStateSaveCallback(
    [](const std::string& name) -> std::string {
        return my_plugin->serializeState();
    });

// Restore plugin state after the new binary is loaded.
mgr.setStateRestoreCallback(
    [](const std::string& name, const std::string& state) -> bool {
        return my_plugin->deserializeState(state);
    });
```

### Reload phases

| Phase | When emitted |
|-------|-------------|
| `BEFORE_UNLOAD` | Before unloading the old binary |
| `AFTER_UNLOAD`  | After the old binary is unloaded |
| `AFTER_LOAD`    | After the new binary is live (success only) |
| `ROLLBACK`      | After a rollback completes successfully |

### Statistics

```cpp
auto s = mgr.getStats();
// s.totalReloads, s.successfulReloads, s.failedReloads
// s.rollbacks, s.statesSaved, s.statesRestored
mgr.resetStats();
```

## Reload Sequence

```
reloadModule("mod", "/new/mod.so")
│
├─► StateSaveCallback (if preserveState=true)
├─► Emit BEFORE_UNLOAD
├─► Load new binary under temp key (fails → return error, old still live)
├─► Unload old binary
├─► Emit AFTER_UNLOAD
├─► Final load under canonical name (fails → emergency restore attempt)
├─► Update backup slot (if enableRollback=true)
├─► StateRestoreCallback (if preserveState=true and state was saved)
└─► Emit AFTER_LOAD → return HotReloadResult{success=true}
```

## Thread Safety

All public methods are thread-safe.  Read-only queries (`getCurrentVersion`,
`isRollbackAvailable`, `registeredModules`, `getSandboxStats`, `getStats`)
acquire a shared reader lock (`std::shared_lock`) so multiple threads can query
concurrently without blocking each other.  Write operations (`reloadModule`,
`rollback`, `registerModule`, `unregisterModule`, and callback/stats mutators)
acquire an exclusive writer lock (`std::unique_lock`) on the same
`std::shared_mutex`.

Callbacks are invoked *outside* the lock to prevent re-entrancy deadlocks.
Exceptions thrown by callbacks are caught and logged; they do not propagate.

## Integration with ModuleLoader

`HotReloadManager` delegates all actual load/unload operations to the
`ModuleLoader` instance provided at `registerModule()` time.  The loader is
responsible for signature verification, ABI checks, and the module registry.
`HotReloadManager` only orchestrates the swap sequence.

## See Also

- [Module Loader](../../src/base/module_loader.cpp) — secure shared library loading
- [Module Sandbox](../../src/base/module_sandbox.cpp) — ABI checker and resource limits
- [Plugin Hot-Plug Monitoring](HOT_PLUG_MONITORING.md) — filesystem-based auto-reload
- [Base Module Roadmap](../../src/base/ROADMAP.md)
