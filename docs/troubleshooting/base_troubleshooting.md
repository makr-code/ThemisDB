# Base Troubleshooting Guide

The `base` module provides ThemisDB's dynamic module loading framework, including hot reload of plugins/modules, ABI compatibility checking, runtime module sandboxing, and dependency resolution.

## Quick Diagnostics

| Symptom | Likely Cause | Quick Fix |
|---------|-------------|-----------|
| `ModuleLoader: symbol not found` | ABI incompatibility between module and core | Recompile module with matching ThemisDB headers |
| Hot reload fails silently | Module uses static state that cannot be reloaded | Move state to module context; avoid static globals |
| `ModuleSandbox: syscall denied` | Sandbox policy too restrictive | Add required syscall to allowlist |
| Module loads but crashes immediately | Missing runtime dependency | Check `ldd` on the module `.so` file |
| `HotReloadManager: reload loop detected` | File watcher triggers on own writes | Set `base.watch.debounce_ms` |
| Dependency cycle prevents load | Circular module dependencies | Resolve cycle; use lazy initialisation |
| `AbiChecker: version mismatch` | Module compiled against wrong ThemisDB version | Recompile against current SDK |
| Module memory leak after hot reload | Old module not fully unloaded | Enable `base.unload.force_dlclose: true` |

## Common Issues

### Issue 1: Module Symbol Not Found at Load

**Description:** A dynamically loaded module fails because a required symbol cannot be resolved.

**Symptoms:**
- Log: `ModuleLoader: dlopen failed: undefined symbol: themisdb_storage_write`
- Module does not load

**Cause:** Module was compiled against a different version of the ThemisDB API or is missing a dependency.

**Solution:**
```bash
# Check unresolved symbols
ldd -r /usr/lib/themisdb/modules/my_module.so | grep "not found"

# Check symbol versions
nm -D /usr/lib/themisdb/modules/my_module.so | grep "themisdb_"

# Recompile module
cmake -DTHEMISDB_SDK=/opt/themisdb/sdk ..
make -j4 && make install
```

---

### Issue 2: Hot Reload Causes Crash

**Description:** Hot reloading a module causes the server to crash.

**Symptoms:**
- Log: `HotReloadManager: preparing hot reload of analytics.so`
- Server crashes immediately after reload

**Cause:** Module uses static state or global variables that cannot survive unload/reload.

**Solution:**
```yaml
base:
  hot_reload:
    enabled: true
    validate_before_reload: true    # test module in isolation before swapping
    state_migration: true           # call module's on_reload() hook
    rollback_on_crash: true         # automatically rollback to previous version
    backup_dir: /var/lib/themisdb/module-backups
```

---

### Issue 3: Sandbox Blocks Required Syscall

**Description:** A module is blocked from making a required system call.

**Symptoms:**
- Log: `ModuleSandbox: syscall 'openat' denied for module=geo_plugin`
- Module works outside sandbox but fails inside

**Cause:** Seccomp sandbox policy does not allow the required syscall.

**Solution:**
```yaml
base:
  sandbox:
    enabled: true
    policy: strict                  # "strict" | "permissive" | "disabled"
    additional_syscalls:
      - openat                      # add allowed syscalls
      - read
      - write
    denied_syscalls:
      - execve
      - ptrace
```

---

### Issue 4: Reload Loop from File Watcher

**Description:** The module hot reload keeps triggering because the file watcher sees its own writes.

**Symptoms:**
- Log: `HotReloadManager: reload triggered (file changed)` repeating every second
- CPU spike from constant reloads

**Cause:** Build system writes to the watched directory; debounce too short.

**Solution:**
```yaml
base:
  watch:
    debounce_ms: 2000              # wait 2s after last change before reloading
    poll_interval_ms: 1000
    ignore_patterns:
      - "*.tmp"
      - "*.swp"
      - "*~"
```

---

### Issue 5: Module Dependency Cycle

**Description:** Two modules depend on each other, preventing either from loading.

**Symptoms:**
- Log: `ModuleLoader: circular dependency: auth → security → auth`
- Both modules remain in `PENDING` state

**Cause:** Circular dependency in module manifest.

**Solution:**
```yaml
# In module manifest, extract shared logic into a third module
# auth/module.yaml
dependencies:
  - security_core               # shared security primitives (not security module)

# security/module.yaml
dependencies:
  - security_core               # same shared primitive

# security_core has no dependencies on auth or security
```

---

### Issue 6: ABI Version Mismatch

**Description:** Module was compiled against an older ThemisDB SDK version.

**Symptoms:**
- Log: `AbiChecker: module analytics.so ABI version=3, core ABI version=4`
- Module refused to load

**Cause:** Module not recompiled after ThemisDB upgrade.

**Solution:**
```bash
# Check module ABI version
themisdb-admin base abi-check --module /usr/lib/themisdb/modules/analytics.so

# Rebuild all modules
themisdb-admin base rebuild-modules --all

# Check current core ABI version
themisdb --version
```

---

### Issue 7: Memory Leak After Hot Reload

**Description:** After repeatedly hot-reloading a module, ThemisDB RSS grows continuously.

**Symptoms:**
- RSS increases by ~50MB per hot reload
- Log: `HotReloadManager: dlclose returned non-zero; module may still be mapped`

**Cause:** Module has registered cleanup callbacks that are not being called; `dlclose` is not fully unloading.

**Solution:**
```yaml
base:
  unload:
    force_dlclose: true            # call dlclose even if refcount > 0
    wait_for_pending_calls_ms: 5000
    leak_check: true               # log address mapping after unload
```

## Diagnostic Commands

```bash
# List loaded modules
themisdb-admin base module-list

# Module load status
themisdb-admin base module-status --module analytics

# ABI compatibility check
themisdb-admin base abi-check --module /usr/lib/themisdb/modules/analytics.so

# Sandbox policy
themisdb-admin base sandbox-policy --module analytics

# Trigger hot reload
themisdb-admin base hot-reload --module analytics

# Tail base logs
journalctl -u themisdb -f | grep -E "base|module.load|hot.reload|sandbox|abi"
```

## Configuration Reference

```yaml
base:
  module_dir: /usr/lib/themisdb/modules
  hot_reload:
    enabled: true
    validate_before_reload: true
    rollback_on_crash: true
  sandbox:
    enabled: true
    policy: strict
  watch:
    debounce_ms: 1000
    poll_interval_ms: 5000
  unload:
    force_dlclose: false
    wait_for_pending_calls_ms: 3000
```

## Known Limitations

- Hot reload does not support modules that maintain open file handles or network connections across reloads.
- Sandbox seccomp policies are Linux-only; macOS and Windows use permissive sandboxing.
- ABI compatibility is checked at load time only; runtime ABI drift is not detected.
- Module dependency cycles cannot be broken at runtime; they must be resolved at development time.

## Related Documentation

- [Base Module ROADMAP](../../src/base/ROADMAP.md)
- [Base Roadmap](../de/roadmap/base_roadmap.md)
- [Plugin System](./plugins_troubleshooting.md)
- [Architecture Overview](../../ARCHITECTURE.md)
