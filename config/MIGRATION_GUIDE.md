# Config Architecture Migration Guide

**Version:** 1.1.0  
**Last Updated:** 2026-04-20

This guide helps you migrate from the legacy flat config structure to the new hierarchical organization.

## 🎯 Overview

The config reorganization introduces a hierarchical directory structure that:
- **Improves discoverability** by grouping related configs
- **Maintains backward compatibility** through automatic path resolution
- **Enables future scalability** with clear categorization

Canonical runtime paths are now the directory-based targets (for example `config/core/config.yaml`, `config/security/pii_patterns.yaml`, `config/performance/...`).
Legacy files in the `config/` root are retained as compatibility inputs and should no longer be edited directly.

## 📋 Quick Migration Checklist

- [ ] Review this migration guide
- [ ] Identify configs your code loads
- [ ] Update hardcoded paths to use new locations (optional, but recommended)
- [ ] Test your application with new paths
- [ ] Monitor logs for legacy path warnings
- [ ] Update documentation and deployment scripts

## 🔄 Complete Path Mapping Table

| Legacy Path | New Path | Category |
|------------|----------|----------|
| `config/config.yaml` | `config/core/config.yaml` | Core |
| `config/config-minimal.yaml` | `config/core/config-minimal.yaml` | Core |
| `config/security.yaml` | `config/core/security.yaml` | Core |
| `config/updates.yaml` | `config/core/updates.yaml` | Core |
| `config/config.rpi3.json` | `config/platform/rpi3.json` | Platform |
| `config/config.rpi4.json` | `config/platform/rpi4.json` | Platform |
| `config/config.rpi5.json` | `config/platform/rpi5.json` | Platform |
| `config/config.qnap.json` | `config/platform/qnap.json` | Platform |
| `config/lora_training_config.yaml` | `config/ai_ml/lora_training_config.yaml` | AI/ML |
| `config/vision_config.yaml` | `config/ai_ml/vision/config.yaml` | AI/ML |
| `config/vision_licenses.yaml` | `config/ai_ml/vision/licenses.yaml` | AI/ML |
| `config/llm_system_prompts.yaml` | `config/ai_ml/llm/system_prompts.yaml` | AI/ML |
| `config/llm-models.yaml` | `config/ai_ml/llm/models.yaml` | AI/ML |
| `config/llm_config.example.yaml` | `config/ai_ml/llm/config.example.yaml` | AI/ML |
| `config/llm_config.production.yaml` | `config/ai_ml/llm/config.production.yaml` | AI/ML |
| `config/llm_extended_context.yaml` | `config/ai_ml/llm/extended_context.yaml` | AI/ML |
| `config/llm_remote_models.yaml` | `config/ai_ml/llm/remote_models.yaml` | AI/ML |
| `config/rag_judge.yaml` | `config/ai_ml/rag_judge.yaml` | AI/ML |
| `config/voice_assistant.yaml` | `config/ai_ml/voice_assistant.yaml` | AI/ML |
| `config/pii_patterns.yaml` | `config/security/pii_patterns.yaml` | Security |
| `config/rbac_roles.json` | `config/security/rbac_roles.json` | Security |
| `config/user_roles.json` | `config/security/user_roles.json` | Security |
| `config/graph_protection.yaml` | `config/security/graph_protection.yaml` | Security |
| `config/auth_kerberos.example.yaml` | `config/security/auth_kerberos.example.yaml` | Security |
| `config/ethical_guidelines.yaml` | `config/compliance/ethical_guidelines.yaml` | Compliance |
| `config/governance.yaml` | `config/compliance/governance.yaml` | Compliance |
| `config/audit.yaml` | `config/compliance/audit/audit.yaml` | Compliance |
| `config/ai_audit_config.yaml` | `config/compliance/audit/ai_audit_config.yaml` | Compliance |
| `config/mime_types.yaml` | `config/data_management/mime_types.yaml` | Data Mgmt |
| `config/storage_redundancy.yaml` | `config/data_management/storage_redundancy.yaml` | Data Mgmt |
| `config/retention_policies.yaml` | `config/data_management/retention_policies.yaml` | Data Mgmt |
| `config/scaling_optimizations.yaml` | `config/performance/scaling_optimizations.yaml` | Performance |
| `config/acceleration.yaml` | `config/performance/acceleration.yaml` | Performance |
| `config/config_2ssd_performance.yaml` | `config/performance/config_2ssd_performance.yaml` | Performance |
| `config/config_multi_ssd.yaml` | `config/performance/config_multi_ssd.yaml` | Performance |
| `config/query_cache_mixed.yaml` | `config/performance/query_cache/mixed.yaml` | Performance |
| `config/query_cache_olap.yaml` | `config/performance/query_cache/olap.yaml` | Performance |
| `config/query_cache_oltp.yaml` | `config/performance/query_cache/oltp.yaml` | Performance |
| `config/phase2_optimizations.json` | `config/deprecated/phase2_optimizations.json` | Deprecated |
| `config/phase3_optimizations.json` | `config/deprecated/phase3_optimizations.json` | Deprecated |
| `config/connection_pool_config.yaml` | `config/networking/connection_pool_config.yaml` | Networking |
| `config/content_processors.yaml` | `config/content/processors.yaml` | Content |
| `config/fem_edge_type_defaults.yaml` | `config/content/fem_edge_type_defaults.yaml` | Content |
| `config/prometheus-arm.yml` | `config/monitoring/prometheus/arm.yml` | Monitoring |
| `config/prometheus_ethics.yml` | `config/monitoring/prometheus/ethics.yml` | Monitoring |
| `config/features.yaml.example` | `config/features/features.example.yaml` | Features |
| `config/capability_auto_generation.yaml` | `config/features/capability_auto_generation.yaml` | Features |
| `config/docs_assistant.yaml` | `config/assistants/docs_assistant.yaml` | Assistants |
| `config/feedback_config.yaml` | `config/assistants/feedback_config.yaml` | Assistants |
| `config/cep_rules.yaml` | `config/processing/cep_rules.yaml` | Processing |
| `config/replication.example.yaml` | `config/distributed/replication/basic.example.yaml` | Distributed |
| `config/replication-ha.example.yaml` | `config/distributed/replication/ha.example.yaml` | Distributed |
| `config/sharding-with-metrics.yaml` | `config/distributed/sharding/with-metrics.yaml` | Distributed |
| `config/license_community_default.json` | `config/licensing/community/default.json` | Licensing |
| `config/license_community_example.json` | `config/licensing/community/example.json` | Licensing |
| `config/enterprise_license.example.json` | `config/licensing/enterprise/example.json` | Licensing |
| `config/license_enterprise_test.json` | `config/licensing/enterprise/test.json` | Licensing |

## 🛠️ Migration Strategies

### Strategy 1: Automatic Migration (Recommended)

**No code changes required!** The `ConfigPathResolver` automatically handles path translation.

```cpp
// Your existing code continues to work:
auto config = loadConfig("config/pii_patterns.yaml");
// Automatically resolved to: config/security/pii_patterns.yaml
```

**Pros:**
- Zero code changes
- Immediate compatibility
- Risk-free migration

**Cons:**
- Legacy paths generate warnings in logs
- Doesn't take full advantage of new structure

### Strategy 2: Gradual Migration (Best Practice)

Update code gradually while maintaining compatibility.

```cpp
// Before:
auto config = loadConfig("config/pii_patterns.yaml");

// After:
#include "config/config_path_resolver.h"
auto config = loadConfig(
    themis::config::ConfigPathResolver::resolve("config/pii_patterns.yaml")
);
```

This approach:
1. Uses the resolver for automatic fallback
2. Logs usage for tracking
3. Prepares for future where old paths may be removed

### Strategy 3: Direct Migration (For New Code)

Use new paths directly for new code:

```cpp
// New code should use new paths directly
auto config = loadConfig("config/security/pii_patterns.yaml");
```

**When to use:**
- New features
- Major refactors
- When you want to eliminate legacy code

## 📝 Step-by-Step Migration

### Step 1: Audit Your Config Usage

Find all config file references in your codebase:

```bash
# Search for config file references
grep -r "config/" src/ include/ --include="*.cpp" --include="*.h"

# Search for specific config patterns
grep -r "\.yaml\|\.json\|\.yml" src/ --include="*.cpp" | grep config
```

### Step 2: Update Hardcoded Paths

Replace hardcoded paths with ConfigPathResolver:

**Before:**
```cpp
const std::string CONFIG_PATH = "config/pii_patterns.yaml";
```

**After:**
```cpp
#include "config/config_path_resolver.h"

const std::string CONFIG_PATH = 
    themis::config::ConfigPathResolver::mapLegacyToNew("config/pii_patterns.yaml");
```

### Step 3: Update Config File References

Update references within config files:

**Before (config.yaml):**
```yaml
features:
  retention:
    policies_path: "config/retention_policies.yaml"
```

**After (canonical path in `config/core/config.yaml`):**
```yaml
features:
  retention:
    policies_path: "config/data_management/retention_policies.yaml"
```

### Step 4: Update Tests

Update test fixtures and mocks:

```cpp
// Before
TEST(ConfigTest, LoadPIIPatterns) {
    auto config = loadConfig("config/pii_patterns.yaml");
    ASSERT_TRUE(config.has_value());
}

// After - using new path with resolver
TEST(ConfigTest, LoadPIIPatterns) {
    auto config = loadConfig(
        themis::config::ConfigPathResolver::resolve("config/pii_patterns.yaml")
    );
    ASSERT_TRUE(config.has_value());
}

// Or test both paths for compatibility
TEST(ConfigTest, LoadPIIPatternsLegacyPath) {
    auto config = loadConfig("config/pii_patterns.yaml");
    ASSERT_TRUE(config.has_value());
}

TEST(ConfigTest, LoadPIIPatternsNewPath) {
    auto config = loadConfig("config/security/pii_patterns.yaml");
    ASSERT_TRUE(config.has_value());
}
```

### Step 5: Update Deployment Scripts

Update deployment scripts and infrastructure configs:

**Docker volumes:**
```yaml
# Before
volumes:
  - ./config:/app/config

# After (no change needed, but can be explicit)
volumes:
  - ./config:/app/config
```

**Environment variables:**
```bash
# Before
export PII_CONFIG="config/pii_patterns.yaml"

# After
export PII_CONFIG="config/security/pii_patterns.yaml"
```

### Step 6: Update Documentation

Update all documentation referencing config paths:
- README files
- API documentation
- Deployment guides
- Configuration examples

## 🧪 Testing Your Migration

### 1. Compile and Link

Ensure your code compiles with the new includes:

```bash
cmake --build build --target themis_server
```

### 2. Run Tests

Run your test suite to verify config loading:

```bash
ctest --test-dir build --output-on-failure
```

### 3. Manual Testing

Test config loading manually:

```bash
# Start server with new config path
./themis_server --config config/core/config.yaml

# Or with legacy path (should work via resolver)
./themis_server --config config/config.yaml
```

### 4. Check Logs

Monitor logs for migration warnings:

```bash
# Look for legacy path warnings
tail -f logs/themis.log | grep "legacy config path"
```

## ⚠️ Common Issues and Solutions

### Issue 1: Config File Not Found

**Symptom:**
```
ERROR: Failed to load config: config/pii_patterns.yaml
```

**Solution:**
1. Check if file was moved: `ls config/security/pii_patterns.yaml`
2. Verify file permissions: `ls -la config/security/`
3. Use ConfigPathResolver for automatic fallback

### Issue 2: Relative Path Issues

**Symptom:**
Config loads in development but fails in production

**Solution:**
Use absolute paths or properly set working directory:

```cpp
// Get absolute path
std::filesystem::path abs_path = std::filesystem::absolute("config/security/pii_patterns.yaml");
```

### Issue 3: Circular Dependencies

**Symptom:**
Config A references Config B, which references Config A

**Solution:**
Review config references and restructure if needed:

```yaml
# Use relative paths from config root
included_configs:
  - "security/rbac_roles.json"
  - "compliance/ethical_guidelines.yaml"
```

## 📊 Migration Timeline

### Immediate (Post-Deployment)

- ✅ New hierarchical structure is live
- ✅ ConfigPathResolver provides backward compatibility
- ✅ All existing code continues to work

### Short-Term (1-2 weeks)

- Update new features to use new paths
- Monitor logs for legacy path usage
- Update internal documentation

### Medium-Term (1-2 months)

- Gradually migrate existing code
- Update all tests
- Update deployment scripts

### Long-Term (3-6 months)

- Consider deprecating legacy path support
- Remove ConfigPathResolver fallback (optional)
- Clean up old config files

## ✅ Current Cleanup Status (April 2026)

- Canonical hierarchy is present and usable.
- Root-level legacy files still exist for compatibility and migration safety.
- Existing and new code should prefer canonical paths in subdirectories.
- Root-level edits should be avoided unless explicitly required for legacy support.
- Removed root legacy duplicates on 2026-04-20:
  - `config/phase2_optimizations.json` -> `config/deprecated/phase2_optimizations.json`
  - `config/phase3_optimizations.json` -> `config/deprecated/phase3_optimizations.json`
  - `config/policies.json.backup` -> `config/deprecated/policies.json.backup`
  - `config/content_processors.yaml` -> `config/content/processors.yaml`
  - `config/fem_edge_type_defaults.yaml` -> `config/content/fem_edge_type_defaults.yaml`
  - `config/capability_auto_generation.yaml` -> `config/features/capability_auto_generation.yaml`
  - `config/replication.example.yaml` -> `config/distributed/replication/basic.example.yaml`
  - `config/sharding-with-metrics.yaml` -> `config/distributed/sharding/with-metrics.yaml`

## 🎓 Best Practices

### DO ✅

- **Use ConfigPathResolver** for all config loading
- **Test both paths** during migration period
- **Update documentation** as you migrate code
- **Monitor logs** for legacy path warnings
- **Use new paths** for all new code

### DON'T ❌

- **Don't remove legacy files** immediately
- **Don't hardcode new paths** without resolver fallback
- **Don't ignore migration warnings** in logs
- **Don't skip testing** after migration
- **Don't forget deployment scripts** and Docker configs

## 📞 Support

If you encounter issues during migration:

1. **Check Documentation**: Review this guide and README.md
2. **Check Logs**: Look for specific error messages
3. **Ask the Team**: Reach out on the development channel
4. **File an Issue**: Create a GitHub issue with details

## 📚 Additional Resources

- [Config Architecture README](./README.md)
- [ConfigPathResolver API Documentation](../docs/api/config-path-resolver.md)
- [Architecture Decision Record](../docs/architecture/ADR-CONFIG-REORGANIZATION.md)

---

**Last Updated**: 2026-04-20  
**Version**: 1.1.0
