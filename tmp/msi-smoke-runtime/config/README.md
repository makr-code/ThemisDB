# ThemisDB Configuration Architecture

This directory contains all configuration files for ThemisDB, organized in a hierarchical structure for better maintainability and discoverability.

Canonical runtime configuration files live in subdirectories (for example `core/`, `security/`, `performance/`). Root-level files are legacy compatibility inputs handled by `ConfigPathResolver`.

## 📁 Directory Structure

```
config/
├── README.md                          # This file
├── core/                              # Core system configurations
│   ├── config.yaml                   # Main server configuration
│   ├── config-minimal.yaml           # Minimal configuration for basic deployments
│   ├── security.yaml                 # HSM and security settings
│   └── updates.yaml                  # Update checker configuration
│
├── platform/                          # Platform-specific configurations
│   ├── rpi3.json                     # Raspberry Pi 3 optimized config
│   ├── rpi4.json                     # Raspberry Pi 4 optimized config
│   ├── rpi5.json                     # Raspberry Pi 5 optimized config
│   └── qnap.json                     # QNAP NAS optimized config
│
├── performance/                       # Performance optimization configs
│   ├── acceleration.yaml             # Hardware acceleration settings
│   ├── scaling_optimizations.yaml   # Auto-scaling and HNSW tuning
│   ├── config_2ssd_performance.yaml # 2-SSD RAID configuration
│   ├── config_multi_ssd.yaml        # Multi-SSD configuration
│   └── query_cache/
│       ├── mixed.yaml                # Mixed workload cache config
│       ├── olap.yaml                 # OLAP-optimized cache config
│       └── oltp.yaml                 # OLTP-optimized cache config
│
├── ai_ml/                            # AI/ML configurations
│   ├── lora_training_config.yaml    # LoRA fine-tuning configuration
│   ├── rag_judge.yaml               # RAG quality judge settings
│   ├── voice_assistant.yaml         # Voice assistant configuration
│   ├── llm/
│   │   ├── models.yaml              # LLM model definitions
│   │   ├── config.example.yaml      # Example LLM configuration
│   │   ├── config.production.yaml   # Production LLM settings
│   │   ├── extended_context.yaml    # Extended context window config
│   │   ├── remote_models.yaml       # Remote LLM endpoints
│   │   └── system_prompts.yaml      # System prompt templates
│   └── vision/
│       ├── config.yaml              # Vision model configuration
│       └── licenses.yaml            # Model license information
│
├── data_management/                  # Data lifecycle management
│   ├── storage_redundancy.yaml      # Blob storage redundancy policies
│   ├── retention_policies.yaml      # Data retention rules
│   └── mime_types.yaml              # MIME type detection rules
│
├── distributed/                      # Distributed system configs
│   ├── replication/
│   │   ├── basic.example.yaml       # Basic replication setup
│   │   └── ha.example.yaml          # High-availability replication
│   └── sharding/
│       └── with-metrics.yaml        # Sharding with metrics collection
│
├── security/                         # Security & authentication
│   ├── auth_kerberos.example.yaml  # Kerberos authentication example
│   ├── rbac_roles.yaml              # Canonical RBAC roles
│   ├── rbac_roles.json              # Role-based access control roles
│   ├── user_roles.json              # User-to-role mappings
│   ├── graph_protection.yaml        # Knowledge graph protection
│   └── pii_patterns.yaml            # PII detection patterns
│
├── compliance/                       # Compliance & ethics
│   ├── ethical_guidelines.yaml      # Ethical AI guidelines
│   ├── README_ETHICAL_GUIDELINES.md # Ethics documentation
│   ├── governance.yaml              # Data governance policies
│   └── audit/
│       ├── audit.yaml               # Audit logging configuration
│       └── ai_audit_config.yaml     # AI-specific audit settings
│
├── licensing/                        # License configurations
│   ├── community/
│   │   ├── default.json             # Default community license
│   │   └── example.json             # Community license example
│   └── enterprise/
│       ├── example.json             # Enterprise license example
│       └── test.json                # Test enterprise license
│
├── networking/                       # Network configurations
│   └── connection_pool_config.yaml  # Connection pool settings
│
├── content/                         # Content processing
│   ├── processors.yaml              # Content processor definitions
│   └── fem_edge_type_defaults.yaml  # Feature engineering edge types
│
├── monitoring/                      # Monitoring & observability
│   └── prometheus/
│       ├── arm.yml                  # Prometheus metrics for ARM
│       └── ethics.yml               # Ethics-related metrics
│
├── features/                        # Feature flags
│   ├── features.example.yaml       # Feature flag examples
│   └── capability_auto_generation.yaml # Auto-capability generation
│
├── assistants/                      # Assistant configurations
│   ├── docs_assistant.yaml         # Documentation assistant
│   └── feedback_config.yaml        # Feedback collection settings
│
├── processing/                      # Stream/event processing
│   └── cep_rules.yaml              # Complex event processing rules
│
└── deprecated/                      # Deprecated/backup configs
    ├── phase2_optimizations.json   # Legacy Phase 2 optimizations
    ├── phase3_optimizations.json   # Legacy Phase 3 optimizations
    └── policies.json.backup        # Archived policy backup
```

## 🔄 Backward Compatibility

All legacy config paths are automatically mapped to their new locations by the `ConfigPathResolver` utility. This ensures:

- **Zero Breaking Changes**: Existing code continues to work
- **Automatic Migration**: Old paths resolve to new locations
- **Deprecation Warnings**: Legacy path usage is logged for migration tracking

### Path Mapping Examples

| Legacy Path | New Path |
|------------|----------|
| `config/lora_training_config.yaml` | `config/ai_ml/lora_training_config.yaml` |
| `config/pii_patterns.yaml` | `config/security/pii_patterns.yaml` |
| `config/config.yaml` | `config/core/config.yaml` |
| `config/scaling_optimizations.yaml` | `config/performance/scaling_optimizations.yaml` |

## ✅ Recommended Editing Policy

- Edit canonical files under their domain subdirectories.
- Do not introduce new root-level duplicates.
- Keep root-level legacy files only for compatibility windows.
- If both legacy and canonical versions exist, canonical content is authoritative.

## 🚀 Usage

### In C++ Code

```cpp
#include "config/config_path_resolver.h"

// Automatic resolution with backward compatibility
std::string path = themis::config::ConfigPathResolver::resolve("config/pii_patterns.yaml");
// Returns: "config/security/pii_patterns.yaml" (new) or "config/pii_patterns.yaml" (legacy fallback)

// Try resolve (returns optional)
auto maybe_path = themis::config::ConfigPathResolver::tryResolve("config/some_config.yaml");
if (maybe_path) {
    // Use *maybe_path
}

// Check if path is legacy
bool is_old = themis::config::ConfigPathResolver::isLegacyPath("config/rbac_roles.json");
// Returns: true
```

### In Configuration Files

Reference configs using their new paths:

```yaml
# main_config.yaml
retention:
  policies_path: "config/data_management/retention_policies.yaml"
  
security:
  pii_detector: "config/security/pii_patterns.yaml"
  rbac_roles: "config/security/rbac_roles.json"
```

## 📝 Migration Guide

See [MIGRATION_GUIDE.md](./MIGRATION_GUIDE.md) for detailed migration instructions.

## 🏗️ Design Principles

1. **Hierarchical Organization**: Configs grouped by functional domain
2. **Discoverability**: Clear directory names indicate content
3. **Scalability**: Easy to add new configs in appropriate categories
4. **Separation of Concerns**: Each directory has a specific purpose
5. **Backward Compatibility**: Legacy paths continue to work during migration

## 📚 Related Documentation

- [Architecture Decision Record: Config Reorganization](../docs/architecture/ADR-CONFIG-REORGANIZATION.md)
- [Migration Guide](./MIGRATION_GUIDE.md)
- [ConfigPathResolver API Documentation](../docs/api/config-path-resolver.md)

## 🛠️ Troubleshooting

### Config File Not Found

If you encounter "Config file not found" errors:

1. **Check New Path**: Ensure the file exists in its new location
2. **Check Legacy Path**: If migrating, the old path may still exist
3. **Use ConfigPathResolver**: Always use the resolver for automatic fallback
4. **Check Permissions**: Verify read permissions on config files

### Legacy Path Warnings

If you see warnings about legacy paths:

```
[WARN] Using legacy config path: config/pii_patterns.yaml. 
       Please migrate to: config/security/pii_patterns.yaml
```

**Action**: Update your code to use the new path. The old path will continue to work but may be removed in future versions.

## 📖 Examples

### Example 1: Loading Main Config

```cpp
// Try new path first, then legacy
std::vector<std::string> paths = {
    "config/core/config.yaml",
    "config/config.yaml",
    "/etc/vccdb/config.yaml"
};

for (const auto& path : paths) {
    auto maybe_config = loadConfig(path);
    if (maybe_config) {
        return *maybe_config;
    }
}
```

### Example 2: Using ConfigPathResolver

```cpp
// Simplest approach - automatic resolution
auto config = loadConfig(
    themis::config::ConfigPathResolver::resolve("config/pii_patterns.yaml")
);
```

## 🔐 Security Considerations

- **File Permissions**: Config files may contain sensitive data. Ensure proper file permissions (0600 for sensitive configs).
- **PKI Verification**: Some configs support PKI signature verification for enhanced security.
- **Secrets Management**: Never commit secrets to config files. Use environment variables or secret managers.

## 📈 Future Enhancements

- **Dynamic Reloading**: Support for hot-reloading configs without restart
- **Schema Validation**: JSON Schema / YAML Schema validation for configs
- **Config Encryption**: Optional encryption for sensitive configuration files
- **Centralized Config Server**: Support for fetching configs from a central server

---

**Last Updated**: 2026-04-20  
**Version**: 2.0.0 (Post-Reorganization)
