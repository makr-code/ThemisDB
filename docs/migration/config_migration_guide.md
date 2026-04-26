# Config Path Migration Guide

## Overview

The ThemisDB configuration subsystem has been reorganized into a hierarchical structure for better maintainability and clarity. This guide explains the migration process from legacy flat paths to the new hierarchical structure.

## Hierarchical Structure

The new configuration structure organizes files into logical categories:

```
config/
├── ai_ml/              # AI/ML configurations (LoRA, LLM, Vision, RAG)
├── security/           # Security configs (PII, RBAC, Auth, Graph Protection)
├── compliance/         # Compliance & Ethics (Audit, Governance)
├── data_management/    # Data configs (MIME types, Storage, Retention)
├── performance/        # Performance tuning (Scaling, Cache, Acceleration)
├── core/               # Core system configs
├── platform/           # Platform-specific configs (RPi, QNAP)
├── networking/         # Network configs (Connection pools)
├── content/            # Content processing configs
├── monitoring/         # Monitoring & observability (Prometheus)
├── features/           # Feature flags and capabilities
├── assistants/         # AI assistants configs
├── processing/         # CEP and processing rules
├── licensing/          # License configurations
└── deprecated/         # Deprecated/backup files
```

## Migration Timeline

### Phase 1: Dual Support (Current - June 30, 2026)
- Both legacy and new paths are supported
- Legacy paths trigger warnings in logs
- No breaking changes for existing deployments

### Phase 2: Deprecation Warnings (July 1, 2026 - December 31, 2026)
- Legacy paths trigger ERROR-level warnings
- Migration guide prominently displayed in logs
- New deployments should use new paths only

### Phase 3: Legacy Removal (January 1, 2027+)
- Legacy paths no longer supported
- Applications must use new hierarchical paths
- ConfigNotFoundException thrown for legacy paths

## Path Mapping Reference

### AI/ML Configurations

| Legacy Path | New Path | Category |
|------------|----------|----------|
| `config/lora_training_config.yaml` | `config/ai_ml/lora_training_config.yaml` | ai_ml |
| `config/vision_config.yaml` | `config/ai_ml/vision/config.yaml` | ai_ml |
| `config/llm_system_prompts.yaml` | `config/ai_ml/llm/system_prompts.yaml` | ai_ml |
| `config/rag_judge.yaml` | `config/ai_ml/rag_judge.yaml` | ai_ml |
| `config/voice_assistant.yaml` | `config/ai_ml/voice_assistant.yaml` | ai_ml |

### Security Configurations

| Legacy Path | New Path | Category |
|------------|----------|----------|
| `config/pii_patterns.yaml` | `config/security/pii_patterns.yaml` | security |
| `config/rbac_roles.json` | `config/security/rbac_roles.json` | security |
| `config/user_roles.json` | `config/security/user_roles.json` | security |
| `config/graph_protection.yaml` | `config/security/graph_protection.yaml` | security |
| `config/auth_kerberos.example.yaml` | `config/security/auth_kerberos.example.yaml` | security |

### Compliance & Ethics

| Legacy Path | New Path | Category |
|------------|----------|----------|
| `config/ethical_guidelines.yaml` | `config/compliance/ethical_guidelines.yaml` | compliance |
| `config/governance.yaml` | `config/compliance/governance.yaml` | compliance |
| `config/audit.yaml` | `config/compliance/audit/audit.yaml` | compliance |
| `config/ai_audit_config.yaml` | `config/compliance/audit/ai_audit_config.yaml` | compliance |

### Performance Configurations

| Legacy Path | New Path | Category |
|------------|----------|----------|
| `config/scaling_optimizations.yaml` | `config/performance/scaling_optimizations.yaml` | performance |
| `config/acceleration.yaml` | `config/performance/acceleration.yaml` | performance |
| `config/query_cache_mixed.yaml` | `config/performance/query_cache/mixed.yaml` | performance |
| `config/query_cache_olap.yaml` | `config/performance/query_cache/olap.yaml` | performance |
| `config/query_cache_oltp.yaml` | `config/performance/query_cache/oltp.yaml` | performance |

### Core Configurations

| Legacy Path | New Path | Category |
|------------|----------|----------|
| `config/config.yaml` | `config/core/config.yaml` | core |
| `config/config-minimal.yaml` | `config/core/config-minimal.yaml` | core |
| `config/security.yaml` | `config/core/security.yaml` | core |
| `config/updates.yaml` | `config/core/updates.yaml` | core |

## Migration Steps

### 1. Identify Legacy Paths

Use the `config_migration_scanner` tool to find all files referencing legacy config paths:

```bash
# Scan a deployment directory and print a text report
config_migration_scanner --root /srv/themis

# JSON output (suitable for CI/CD integration)
config_migration_scanner --root /srv/themis --output json

# CSV output
config_migration_scanner --root /srv/themis --output csv
```

The tool scans `.yaml`, `.yml`, `.json`, `.toml`, `.ini`, and `.env` files recursively and prints each occurrence with:
- `file` and `line` number
- `legacy_path` → `new_path` mapping
- `deprecated_date`, `removal_date`, migration guide URL
- `[OVERDUE]` flag for paths whose removal date has passed

You can also use `grep` for a quick manual check:

```bash
grep -r "config/[^/]*\.yaml" your-code-directory
```

### 2. Automatically Fix Legacy Path References

Use `config_migration_scanner --fix` to automatically rewrite legacy path strings in config files:

```bash
# Dry-run: see what would be changed without modifying files
config_migration_scanner --root /srv/themis --dry-run --fix

# Apply fixes in-place (creates .bak backups before modifying)
config_migration_scanner --root /srv/themis --fix
```

Each modified file gets a `.bak` backup (e.g. `deploy.yaml.bak`) before changes are written.

**Exit codes:**
- `0` – No overdue legacy paths found
- `1` – At least one path past its `removal_date` (usable as a CI gate)
- `2` – Argument / usage error

### 3. Update Configuration References Manually

If you prefer to update references by hand, replace legacy paths with their new hierarchical locations:

```cpp
// Before
std::string path = "config/lora_training_config.yaml";

// After
std::string path = "config/ai_ml/lora_training_config.yaml";
```

### 4. Use ConfigPathResolver (Transitional)

During migration, you can use `ConfigPathResolver` which automatically handles fallback:

```cpp
#include "config/config_path_resolver.h"

// Automatically tries new path first, falls back to legacy
std::string resolved = themis::config::ConfigPathResolver::resolve(
    "config/lora_training_config.yaml"
);

// Returns: "config/ai_ml/lora_training_config.yaml" if it exists
```

### 5. Verify Migration

Check logs for deprecation warnings:

```bash
grep "Using legacy config path" your-logs.txt
```

### 6. Update Deployment Scripts

Update your deployment scripts to copy/create files in new locations:

```bash
# Before
cp config/lora_training_config.yaml /opt/themisdb/config/

# After
mkdir -p /opt/themisdb/config/ai_ml
cp config/ai_ml/lora_training_config.yaml /opt/themisdb/config/ai_ml/
```

## Monitoring Migration Progress

### Check Metrics

The ConfigPathResolver tracks metrics for monitoring migration progress:

```cpp
const auto& metrics = ConfigPathResolver::metrics();

std::cout << "Legacy fallbacks: " << metrics.legacy_fallbacks << std::endl;
std::cout << "New path hits: " << metrics.new_path_hits << std::endl;
std::cout << "Unmapped requests: " << metrics.unmapped_requests << std::endl;
```

### Log Analysis

Search for deprecation warnings in logs:

```bash
# Find all legacy path usage
grep "Using legacy config path" logs/*.log | wc -l

# List unique legacy paths still in use
grep "Using legacy config path" logs/*.log | \
    sed -n 's/.*Using legacy config path: \([^.]*\)/\1/p' | \
    sort | uniq
```

## Troubleshooting

### Issue: Config file not found

**Error:**
```
ConfigNotFoundException: Config file not found: config/my_config.yaml
Attempted paths:
  - config/category/my_config.yaml
  - config/my_config.yaml
```

**Solution:**
1. Check if the file exists in either location
2. Verify the path mapping in `config_path_resolver.cpp`
3. Ensure file permissions are correct

### Issue: Path traversal rejected

**Error:**
```
InvalidPathException: Invalid config path: config/../../etc/passwd (path traversal not allowed)
```

**Solution:**
This is a security feature. Do not use `..` in config paths. Use absolute paths within the config directory.

### Issue: Unmapped path warning

**Warning:**
```
ConfigPathResolver: Unmapped path requested: config/custom_config.yaml
```

**Solution:**
If this is a new config file that doesn't need migration:
- Place it directly in the appropriate category directory
- No mapping entry needed for new files

If this is a legacy file that should be mapped:
- Add an entry to `PATH_MAPPING` in `config_path_resolver.cpp`
- Follow contribution guidelines for submitting the change

## Contributing

### Adding New Mappings

1. Edit `src/config/config_path_resolver.cpp`
2. Add entry to `PATH_MAPPING` table:

```cpp
{"config/old_path.yaml", "config/category/new_path.yaml"},
```

3. Run validation:

```bash
python3 scripts/validate_config_mapping.py
```

4. Update this guide with the new mapping
5. Submit a pull request

### Validation Script

The CI pipeline automatically validates:
- No duplicate legacy paths
- Hierarchical structure compliance
- Valid category assignments
- Schema compliance

Run locally before committing:

```bash
python3 scripts/validate_config_mapping.py
```

## Additional Resources

- [Config Subsystem Roadmap](../de/roadmap/config_roadmap.md)
- [Config Path Resolver API](../include/config/config_path_resolver.h)
- [Architecture Overview](de/architecture/ARCHITECTURE_OVERVIEW.md)
- [Security Framework](../SECURITY.md)

## Support

For questions or issues with migration:
- Open an issue on GitHub
- Check existing issues for similar problems
- Contact the development team
