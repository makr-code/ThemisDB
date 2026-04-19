# Config Troubleshooting Guide

The `config` module manages all runtime configuration for ThemisDB, including YAML/JSON parsing, live config reloading via inotify/file watchers, path resolution with LRU caching, and migration from legacy configuration formats.

## Quick Diagnostics

| Symptom | Likely Cause | Quick Fix |
|---------|-------------|-----------|
| `ConfigPathResolver: file not found` | Wrong config path or missing file | Check `--config` flag and file existence |
| Config changes not applied after edit | File watcher not running | Enable `config.watch.enabled: true` |
| `LruCache: config entry evicted` | Cache too small | Increase `config.lru_cache.max_entries` |
| YAML parse error on startup | Syntax error in config file | Validate with `yamllint config/themisdb.yaml` |
| Legacy config key not recognised | Old key not mapped to new key | Run `themisdb-admin config migrate-legacy` |
| Environment variable override not working | Wrong env var name format | Use `THEMIS_<SECTION>_<KEY>` format |
| Config reload crashes server | Invalid value in updated config | Validate config before live reload |
| `PathMapping: circular reference` | Config file includes itself | Check `include` directives |

## Common Issues

### Issue 1: Configuration File Not Found

**Description:** ThemisDB cannot find its configuration file at startup.

**Symptoms:**
- Log: `ConfigPathResolver: config file not found: /etc/themisdb/themisdb.yaml`
- Server exits immediately

**Cause:** Config file path is wrong; file permissions deny read access.

**Solution:**
```bash
# Check default config paths
themisdb --help | grep config

# Create config from template
cp /usr/share/themisdb/themisdb.yaml.example /etc/themisdb/themisdb.yaml
chmod 640 /etc/themisdb/themisdb.yaml
chown root:themisdb /etc/themisdb/themisdb.yaml

# Override config path
themisdb --config /opt/themisdb/config/custom.yaml
```

---

### Issue 2: Live Reload Does Not Apply Changes

**Description:** After editing the configuration file, changes are not picked up without a restart.

**Symptoms:**
- No log line `ConfigManager: reloaded config from /etc/themisdb/themisdb.yaml`
- inotify watch not working in container

**Cause:** `config.watch.enabled` is false, or the filesystem does not support inotify (e.g., NFS, some container filesystems).

**Solution:**
```yaml
config:
  watch:
    enabled: true
    poll_interval_ms: 5000          # fallback polling if inotify unavailable
    use_inotify: true
    debounce_ms: 500                # wait 500ms after change before reloading
```
```bash
# Trigger manual config reload
themisdb-admin config reload

# Force reload via signal
kill -SIGHUP $(pidof themisdb)
```

---

### Issue 3: YAML Parse Error on Startup

**Description:** Configuration file contains a syntax error that prevents startup.

**Symptoms:**
- Log: `ConfigManager: YAML parse error at line 42: mapping values are not allowed in this context`
- Server exits with code 78 (EX_CONFIG)

**Cause:** Incorrect indentation, missing quotes, or invalid YAML syntax.

**Solution:**
```bash
# Validate YAML syntax
yamllint /etc/themisdb/themisdb.yaml

# Use JSON schema validation
themisdb-admin config validate --file /etc/themisdb/themisdb.yaml

# Common YAML pitfalls
# Wrong: key: value: nested   (colon in value without quotes)
# Right: key: "value: nested"
```

---

### Issue 4: Legacy Configuration Keys Not Recognised

**Description:** Configuration from an older ThemisDB version is not applied.

**Symptoms:**
- Log: `ConfigManager: unknown key 'database.max_connections' (was this renamed?)`
- Features behave as if unconfigured

**Cause:** Keys were renamed or restructured between versions.

**Solution:**
```bash
# Show mapping of legacy keys to new keys
themisdb-admin config legacy-key-map

# Automatically migrate legacy config
themisdb-admin config migrate-legacy \
  --input /etc/themisdb/themisdb.yaml \
  --output /etc/themisdb/themisdb.yaml.migrated

diff /etc/themisdb/themisdb.yaml /etc/themisdb/themisdb.yaml.migrated
```

---

### Issue 5: Environment Variable Override Not Working

**Description:** Setting an environment variable does not override the config file value.

**Symptoms:**
- `THEMIS_STORAGE_ROCKSDB_MAX_OPEN_FILES=50000` has no effect

**Cause:** Wrong environment variable naming convention.

**Solution:**
```bash
# Format: THEMIS_<SECTION>_<KEY> (all uppercase, dots → underscores)
# config path: storage.rocksdb.max_open_files
export THEMIS_STORAGE_ROCKSDB_MAX_OPEN_FILES=50000

# Verify override is recognised
themisdb-admin config show --key storage.rocksdb.max_open_files
```
```yaml
config:
  env_override:
    enabled: true
    prefix: THEMIS_
    separator: _
```

---

### Issue 6: Config Reload Crashes Server

**Description:** A live config reload causes the server to crash.

**Symptoms:**
- Log: `ConfigManager: reload failed: invalid value for api.http.port: -1`
- Server restarts via systemd

**Cause:** Config file was saved with an invalid value; reload applied it.

**Solution:**
```yaml
config:
  watch:
    validate_before_reload: true    # reject invalid configs before applying
    rollback_on_error: true         # roll back to last good config on error
    backup_on_reload: true          # save copy of previous config
```
```bash
# Validate before manual reload
themisdb-admin config validate && themisdb-admin config reload
```

---

### Issue 7: Include Directive Creates Circular Reference

**Description:** Config file includes another file that includes back the original.

**Symptoms:**
- Log: `ConfigPathResolver: circular include detected: a.yaml → b.yaml → a.yaml`
- Server exits with parse error

**Cause:** Config `include` directives create a cycle.

**Solution:**
```yaml
# Use flat includes only; no circular references allowed
# main config: /etc/themisdb/themisdb.yaml
config:
  include:
    - /etc/themisdb/conf.d/storage.yaml
    - /etc/themisdb/conf.d/auth.yaml
    # Do NOT include themisdb.yaml here
```

## Diagnostic Commands

```bash
# Show effective configuration (merged from all sources)
themisdb-admin config show --effective

# Show a specific key
themisdb-admin config show --key storage.rocksdb.max_open_files

# Validate config file
themisdb-admin config validate --file /etc/themisdb/themisdb.yaml

# List all config sources (file, env, defaults)
themisdb-admin config sources

# Reload config
themisdb-admin config reload

# Tail config logs
journalctl -u themisdb -f | grep -E "config|reload|parse|watch"
```

## Configuration Reference

```yaml
config:
  watch:
    enabled: true
    use_inotify: true
    poll_interval_ms: 10000
    debounce_ms: 500
    validate_before_reload: true
    rollback_on_error: true
  lru_cache:
    max_entries: 1000
    ttl_ms: 60000
  env_override:
    enabled: true
    prefix: THEMIS_
  strict_mode: false               # reject unknown keys if true
```

## Known Limitations

- Live config reload is not supported for all keys; some require a full restart (e.g., `network.wire_protocol.version`).
- Config file includes are limited to one level of nesting; deeply nested includes are not supported.
- Environment variable overrides do not support nested arrays or maps; use config files for complex values.

## Related Documentation

- [Config Module ROADMAP](../../src/config/ROADMAP.md)
- [Config Roadmap](../config_roadmap.md)
- [Config Implementation Summary](../ARCHIVED/implementation-summaries/config_implementation_summary.md)
- [Config Migration Guide](../config_migration_guide.md)
