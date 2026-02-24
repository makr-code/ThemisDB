# Plugins Troubleshooting Guide

The `plugins` module provides ThemisDB's plugin system, including plugin registration, lifecycle management, health monitoring, hot-plug support, edition-based feature gating, RPC service registry, and plugin metrics collection.

## Quick Diagnostics

| Symptom | Likely Cause | Quick Fix |
|---------|-------------|-----------|
| `PluginManager: plugin not found` | Plugin not installed | Install plugin package |
| `PluginRegistry: duplicate plugin name` | Plugin registered twice | Check startup sequence; remove duplicate |
| Plugin fails health check | Plugin initialisation error | Check plugin logs; verify config |
| `PluginSystemEdition: feature gated` | Edition does not include plugin | Upgrade edition |
| Hot-plug watcher triggers on temp files | Debounce too short | Increase `plugins.watch.debounce_ms` |
| `RpcServiceRegistry: port conflict` | Plugin RPC port in use | Change `plugins.rpc.port` |
| Plugin memory leak | Plugin not properly unloaded | Enable `plugins.unload.force: true` |
| `PluginHealthMonitor: timeout` | Plugin unresponsive | Increase `plugins.health.timeout_ms` |
| Plugin metrics not visible | Plugin not publishing metrics | Check `plugins.metrics.enabled` |
| Plugin dependency missing | Required plugin not loaded | Check plugin dependencies |

## Common Issues

### Issue 1: Plugin Not Found

**Description:** ThemisDB cannot find a registered plugin at startup or when loading dynamically.

**Symptoms:**
- Log: `PluginManager: plugin 'geo_enhancement_v2' not found in /usr/lib/themisdb/plugins/`
- Feature provided by plugin is unavailable

**Cause:** Plugin file not installed or in wrong directory.

**Solution:**
```bash
# Check plugin directory
ls /usr/lib/themisdb/plugins/

# Install missing plugin
apt install themisdb-plugin-geo-enhancement

# Or copy plugin manually
cp /tmp/geo_enhancement_v2.so /usr/lib/themisdb/plugins/
chmod 755 /usr/lib/themisdb/plugins/geo_enhancement_v2.so
```
```yaml
plugins:
  plugin_dir: /usr/lib/themisdb/plugins
  auto_discover: true
  required_plugins:
    - name: geo_enhancement_v2
      optional: false
```

---

### Issue 2: Plugin Health Check Fails

**Description:** A plugin repeatedly fails its health check and is marked unhealthy.

**Symptoms:**
- Log: `PluginHealthMonitor: plugin 'ml_enricher' failed health check 3 consecutive times`
- Plugin auto-disabled by health monitor

**Cause:** Plugin is unresponsive to health pings; initialisation error or dependency missing.

**Solution:**
```bash
# Check plugin status
themisdb-admin plugins status --plugin ml_enricher

# View plugin-specific logs
themisdb-admin plugins logs --plugin ml_enricher --last 100

# Manually restart plugin
themisdb-admin plugins restart --plugin ml_enricher
```
```yaml
plugins:
  health:
    enabled: true
    check_interval_ms: 30000
    timeout_ms: 5000              # increase if plugin is slow to respond
    failure_threshold: 5          # more failures before marking unhealthy
    auto_disable_unhealthy: false # disable to prevent cascade failures
```

---

### Issue 3: RPC Service Registry Port Conflict

**Description:** Plugin RPC service cannot bind its port because it is already in use.

**Symptoms:**
- Log: `RpcServiceRegistry: failed to bind port 9200 for plugin 'search_enhancer': EADDRINUSE`
- Plugin loads but RPC calls fail

**Cause:** Another service or plugin already using the configured port.

**Solution:**
```yaml
plugins:
  rpc:
    base_port: 9300               # change base port to avoid conflict
    auto_assign_ports: true       # let registry assign ports automatically
    port_range: [9300, 9400]
```
```bash
# Check port usage
ss -tlnp | grep 9200

# List RPC services
themisdb-admin plugins rpc-services list
```

---

### Issue 4: Edition Feature Gate Blocks Plugin

**Description:** A plugin's feature is blocked by the edition feature gate.

**Symptoms:**
- Log: `PluginSystemEdition: feature 'advanced_ml_plugin' requires edition=enterprise`
- Plugin loads but its feature cannot be used

**Cause:** Plugin feature requires Enterprise Edition.

**Solution:**
```bash
# Check current edition
themisdb-admin core edition-info

# Check plugin edition requirements
themisdb-admin plugins edition-requirements --plugin advanced_ml_plugin

# Disable gated feature gracefully (use free alternative)
```
```yaml
plugins:
  edition_gate:
    action: warn                  # "block" | "warn" | "disable"
    fallback_plugin: basic_ml_plugin
```

---

### Issue 5: Hot-Plug Watcher Triggers Too Frequently

**Description:** The hot-plug watcher fires for every file change including build artifacts.

**Symptoms:**
- Log: `PluginHotPlugMonitor: plugin reload triggered by /usr/lib/themisdb/plugins/my_plugin.so.tmp`
- Continuous reloads from temporary files

**Cause:** Debounce too short; build system creates temp files in plugin directory.

**Solution:**
```yaml
plugins:
  watch:
    enabled: true
    debounce_ms: 3000             # wait 3s after last change
    poll_interval_ms: 5000
    ignore_patterns:
      - "*.tmp"
      - "*.swp"
      - "*.so.new"
```

---

### Issue 6: Plugin Metrics Not in Prometheus

**Description:** Plugin performance metrics are not visible in Prometheus.

**Symptoms:**
- No `themisdb_plugin_*` metrics in `/metrics`
- Plugin works but cannot be monitored

**Cause:** Plugin metrics collection disabled.

**Solution:**
```yaml
plugins:
  metrics:
    enabled: true
    prefix: themisdb_plugin        # metric label prefix
    include_per_plugin_labels: true
    collection_interval_ms: 15000
```

---

### Issue 7: Plugin Dependency Not Loaded

**Description:** A plugin fails to load because a required plugin is not yet loaded.

**Symptoms:**
- Log: `PluginManager: plugin 'text_analytics' requires 'nlp_base' which is not loaded`
- Plugin load order issue

**Cause:** Plugin load order does not respect dependencies.

**Solution:**
```yaml
plugins:
  load_order:
    - nlp_base
    - text_analytics              # load after nlp_base
    - ml_enricher
  dependency_resolution: auto     # auto-sort by dependencies
```
```bash
# Show plugin dependency graph
themisdb-admin plugins dependency-graph
```

---

### Issue 8: Plugin Not Fully Unloaded After Hot Reload

**Description:** After hot reloading a plugin, old version's code is still running.

**Symptoms:**
- Log: `PluginManager: dlclose returned non-zero; old version still mapped`
- Memory usage increases after each reload

**Cause:** Plugin has active calls or static state preventing unload.

**Solution:**
```yaml
plugins:
  unload:
    force: true                   # force dlclose even with active references
    drain_timeout_ms: 5000        # wait 5s for active calls to complete
    verify_unload: true           # log warning if old version still mapped
```

## Diagnostic Commands

```bash
# List all plugins
themisdb-admin plugins list

# Plugin health status
themisdb-admin plugins health

# Plugin details
themisdb-admin plugins info --plugin ml_enricher

# RPC service registry
themisdb-admin plugins rpc-services list

# Dependency graph
themisdb-admin plugins dependency-graph

# Hot reload a plugin
themisdb-admin plugins hot-reload --plugin ml_enricher

# Live plugin metrics
curl -s http://localhost:9100/metrics | grep themisdb_plugin

# Tail plugin logs
journalctl -u themisdb -f | grep -E "plugin|hot.plug|rpc.service|health.monitor"
```

## Configuration Reference

```yaml
plugins:
  enabled: true
  plugin_dir: /usr/lib/themisdb/plugins
  auto_discover: true
  health:
    enabled: true
    check_interval_ms: 30000
    failure_threshold: 3
  watch:
    enabled: true
    debounce_ms: 2000
  rpc:
    base_port: 9200
    auto_assign_ports: true
  metrics:
    enabled: true
  unload:
    force: false
    drain_timeout_ms: 5000
```

## Known Limitations

- Plugins compiled for one ThemisDB version may not load in a different version due to ABI changes.
- RPC service registry does not support service discovery across cluster nodes; each node has its own registry.
- Hot-plug monitor does not support plugin removal; removed plugins remain registered until restart.
- Edition feature gates are checked at plugin load time only; changing edition at runtime does not re-gate.

## Related Documentation

- [Plugins Module ROADMAP](../../src/plugins/ROADMAP.md)
- [Base Troubleshooting](./base_troubleshooting.md)
- [Plugin Development Guide](../PLUGIN_DEVELOPMENT_GUIDE.md)
- [Edition Manager](../EDITION_MANAGER.md)
