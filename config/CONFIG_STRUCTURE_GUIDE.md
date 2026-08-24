# ThemisDB Configuration Structure Guide

**Version:** 1.2  
**Last Updated:** August 24, 2026  

---

## 🎯 Overview

ThemisDB uses a hierarchical configuration structure to improve organization and maintainability. This guide explains the new structure and how to use it.

---

## 📁 Directory Structure

```
config/
│
├── config.yaml                 # Legacy compatibility entry (maps to core/config.yaml)
├── MIGRATION_GUIDE.md          # Path migration documentation
├── CONFIG_STRUCTURE_GUIDE.md   # This file
├── migrate.ps1                 # Config reorganization helper (Windows)
│
├── security/                   # 🔐 Security & Access Control
│   ├── pii_patterns.yaml       # PII detection engines & patterns
│   ├── rbac_roles.yaml         # Canonical role-based access control definitions
│   ├── rbac_roles.json         # Legacy-compatible RBAC format
│   ├── user_roles.json         # User-to-role mapping
│   └── graph_protection.yaml   # Graph-specific access policies
│
├── ai_ml/                      # 🤖 AI/ML & LLM Configuration
│   ├── lora_training_config.yaml       # LoRA adapter training config
│   ├── rag_judge.yaml                  # RAG evaluation configuration
│   ├── voice_assistant.yaml            # Voice features config
│   ├── llm/                            # Large Language Models
│   │   ├── models.yaml                 # Available LLM models
│   │   ├── system_prompts.yaml         # Built-in system prompts
│   │   ├── config.production.yaml      # Production LLM settings
│   │   └── extended_context.yaml       # Extended context config
│   └── vision/                         # Vision/Image Processing
│       ├── config.yaml                 # Vision model settings
│       └── licenses.yaml               # Vision license keys
│
├── compliance/                 # ⚖️ Compliance & Governance
│   ├── governance.yaml         # Governance policies & rules
│   ├── ethical_guidelines.yaml # Ethical AI guidelines
│   └── audit/                  # Audit & Logging
│       ├── audit.yaml          # Audit logging configuration
│       └── ai_audit_config.yaml # AI-specific audit rules
│
├── data_management/            # 📊 Data Lifecycle & Storage
│   ├── retention_policies.yaml # Data retention rules
│   ├── storage_redundancy.yaml # RAID/replication config
│   └── mime_types.yaml         # MIME type handling
│
├── performance/                # ⚡ Performance Tuning
│   ├── scaling_optimizations.yaml      # Scaling & resource config
│   ├── acceleration.yaml               # GPU/SIMD acceleration
│   ├── config_2ssd_performance.yaml    # 2-SSD optimization profile
│   ├── config_multi_ssd.yaml           # Multi-SSD optimization
│   └── query_cache/                    # Query Caching Profiles
│       ├── mixed.yaml                  # Mixed OLTP/OLAP
│       ├── oltp.yaml                   # OLTP optimized
│       └── olap.yaml                   # OLAP optimized
│
├── networking/                 # 🌐 Network & Communication
│   ├── connection_pool_config.yaml    # Connection pooling
│   └── README.md               # Networking config overview
│
├── content/                    # 📄 Content Processing
│   ├── processors.yaml         # Content processor settings
│   └── fem_edge_type_defaults.yaml    # FEM edge type defaults
│
├── monitoring/                 # 📈 Monitoring & Observability
│   └── prometheus/             # Prometheus Integration
│       ├── arm.yml             # ARM-specific Prometheus config
│       └── ethics.yml          # Ethics metrics config
│
├── distributed/                # 🔗 Distributed Systems
│   ├── replication/            # Replication Configuration
│   │   ├── basic.example.yaml  # Basic replication example
│   │   └── ha.example.yaml     # HA replication with failover
│   └── sharding/               # Sharding Configuration
│       └── with-metrics.yaml   # Sharding with metrics
│
├── editions/                   # 🏷️ Per-Edition Runtime Configurations
│   ├── minimal.yaml            # MINIMAL edition — edge, IoT, single-node
│   ├── community.yaml          # COMMUNITY edition — default (no license required)
│   ├── enterprise.yaml         # ENTERPRISE edition — production, HSM, license required
│   ├── hyperscaler.yaml        # HYPERSCALER edition — cloud-scale, license required
│   ├── military.yaml           # MILITARY edition — air-gapped, hardened, license required
│   └── all-editions.yaml       # Consolidated multi-document reference (all editions)
│
├── core/                       # ✅ Canonical core runtime configuration
│   ├── config.yaml             # Main server configuration
│   ├── config-minimal.yaml     # Minimal baseline configuration
│   ├── security.yaml           # Core security/HSM settings
│   └── updates.yaml            # Update channel configuration
│
└── deprecated/                 # 🗂️ Archived Configurations
    └── phase*.json             # Old phase optimization configs
```

---

## 📋 Configuration Loading Order

The `ConfigPathResolver` searches for configuration files in this priority order:

### For Main Configuration
1. `./config/core/config.yaml` ← **Primary** (Recommended)
2. `./config/config.yaml` ⚠️ (Legacy fallback)
3. `./config.yaml` ⚠️ (Compatibility shim)
4. `/etc/themisdb/config.yaml`
5. `/etc/vccdb/config.yaml`

### For Security Configuration
1. `./config/core/security.yaml` ← **Primary** (Recommended)
2. `./config/security.yaml` ⚠️ (Legacy fallback)
3. `/etc/themisdb/security.yaml` ⚠️ (Legacy fallback)

### For PII Patterns
1. `./config/security/pii_patterns.yaml` ← **Primary** (Recommended)
2. `./config/pii_patterns.yaml` ⚠️ (Legacy fallback; migration deadline per resolver metadata)
3. `/etc/themisdb/pii_patterns.yaml` ⚠️ (Legacy fallback)

---

## 🚀 Quick Start

### 1. Verify Configuration is Present

```bash
# Windows PowerShell
Test-Path "config\core\config.yaml"

# Linux/macOS
test -f config/core/config.yaml && echo "Found" || echo "Not found"
```

### 2. Check for Deprecation Warnings

```bash
# Start server and check logs
./themis 2>&1 | grep -i deprecated

# Or check logs after startup
grep deprecated themis.log
```

### 3. Migrate Old Paths (If Needed)

```powershell
# Windows
.\config\migrate.ps1

# Linux/macOS
bash config/migrate.sh
```

### 4. Update Configuration References

Update your code to use new paths:

```diff
# Before
- path: config/pii_patterns.yaml
- path: config/security.yaml

# After
+ path: config/security/pii_patterns.yaml
+ path: config/core/security.yaml
```

---

## 🔄 Migration Timeline

| Phase | Date | Action |
|-------|------|--------|
| **Phase 1: Announcement** | April 7, 2026 | New paths documented |
| **Phase 2: Deprecation Warnings** | April 7 - May 30, 2026 | Old paths generate warnings |
| **Phase 3: Escalation** | June 1, 2026 (v1.8.2) | Warnings become ERROR logs |
| **Phase 4: Removal** | September 30, 2026 (v1.9.0) | **BREAKING** - Old paths removed |

---

## 📚 Configuration Categories

### Editions (`editions/`)
Per-edition runtime configurations — the authoritative starting point for deploying a specific edition.

| File | Edition | License |
|---|---|---|
| `editions/minimal.yaml` | MINIMAL | None |
| `editions/community.yaml` | COMMUNITY (default) | None |
| `editions/enterprise.yaml` | ENTERPRISE | Required |
| `editions/hyperscaler.yaml` | HYPERSCALER | Required |
| `editions/military.yaml` | MILITARY | Required |
| `editions/all-editions.yaml` | All (multi-doc reference) | — |

**Edition-Feature Matrix summary:**

| Feature | MINIMAL | COMMUNITY | ENTERPRISE | HYPERSCALER | MILITARY |
|---|---|---|---|---|---|
| LLM | CPU-fallback | ALLOWED | ALLOWED | **REQUIRED** | **REQUIRED** (local only) |
| GPU / CUDA | FORBIDDEN | ALLOWED (≤16 GB, 1× T4) | ALLOWED (≤320 GB, 4× A100 80G) | **REQUIRED** (unlimited) | ALLOWED (≤80 GB, 2× A100 40G) |
| gRPC | FORBIDDEN | ALLOWED | **REQUIRED** | **REQUIRED** | **REQUIRED** |
| Tracing (OTel) | FORBIDDEN | — | ALLOWED | **REQUIRED** | ALLOWED |
| HTTP/3 | FORBIDDEN | ALLOWED | ALLOWED | ALLOWED | FORBIDDEN |
| MCP | FORBIDDEN | ALLOWED | ALLOWED | ALLOWED | FORBIDDEN |
| Distributed training | FORBIDDEN | — | FORBIDDEN | ALLOWED | FORBIDDEN |
| Real HSM | FORBIDDEN | — | ON | available | **REQUIRED** |
| Max shard nodes | 1 | 5 | 100 | unlimited | 50 |

**Loading:** `ConfigLoader` via `--config config/editions/<edition>.yaml`

---

### Security (`security/`)
Sensitive authentication, encryption, and access control settings.

**Files:**
- `pii_patterns.yaml` - PII detection rules
- `rbac_roles.yaml` - Canonical role definitions
- `rbac_roles.json` - Legacy-compatible RBAC format
- `user_roles.json` - User assignments

**Loading:** `PIIDetector`, `AuthManager`

**Related core security file:** `config/core/security.yaml` (loaded by `HsmProvider` and security bootstrap)

---

### AI/ML (`ai_ml/`)
Machine learning and language model configurations.

**Files:**
- `llm/*.yaml` - LLM model settings
- `lora_training_config.yaml` - LoRA training
- `rag_judge.yaml` - RAG evaluation

**Loading:** `LLMManager`, `LoRATrainer`, `VisionProcessor`

---

### Compliance (`compliance/`)
Governance, audit, and compliance policies.

**Files:**
- `governance.yaml` - Policy engine config
- `audit/*.yaml` - Audit logging setup
- `ethical_guidelines.yaml` - AI ethics rules

**Loading:** `GovernanceEngine`, `AuditLogger`, `EthicsValidator`

---

### Performance (`performance/`)
Caching, optimization, and resource allocation.

**Files:**
- `query_cache/*.yaml` - Cache profiles
- `acceleration.yaml` - GPU/SIMD config
- `scaling_optimizations.yaml` - Load balancing

**Loading:** `QueryCache`, `AccelerationModule`, `LoadBalancer`

---

### Data Management (`data_management/`)
Data lifecycle, retention, and storage policies.

**Files:**
- `retention_policies.yaml` - Data retention rules
- `storage_redundancy.yaml` - Backup/replication
- `mime_types.yaml` - Content type handling

**Loading:** `RetentionManager`, `StorageEngine`, `ContentProcessor`

---

### Distributed (`distributed/`)
Replication, sharding, and multi-node setup.

**Files:**
- `replication/*.yaml` - Replication topology
- `sharding/*.yaml` - Shard distribution

**Loading:** `ReplicationManager`, `ShardingCoordinator`

---

## ⚙️ Configuration Defaults

Each subsystem has sensible defaults. You only need to configure:

1. **Essential** (You MUST set these):
   - Database path
   - Server host/port

2. **Recommended** (Set for production):
   - Security/HSM settings
   - Audit logging
   - Resource limits

3. **Optional** (Use for tuning):
   - Cache profiles
   - Acceleration settings
   - Performance optimizations

---

## 🔍 Configuration Validation

### Syntax Validation

**Using yamllint (recommended):**
```bash
# Install
pip install yamllint

# Validate all YAML files
find config/ -name "*.yaml" -o -name "*.yml" | xargs yamllint
```

**Using Python:**
```python
import yaml

with open('config/security/pii_patterns.yaml') as f:
    config = yaml.safe_load(f)
    print("✅ Valid YAML")
```

### Configuration Load Test

```bash
# Start with debug logging
./themis --log-level debug 2>&1 | grep -i "config\|loading"
```

---

## ❓ F.A.Q.

### Q: Do I need to reorganize my configs?

**A:** No, the system is backwards-compatible. Reorganization is recommended but optional for now. Old paths will be removed in v1.9.0.

### Q: What if I mix old and new paths?

**A:** Works, but generates deprecation warnings. Choose ONE approach and stick with it.

### Q: Can I use environment variables for paths?

**A:** Yes! Set `THEMIS_CONFIG_ROOT` or `THEMIS_SECURITY_CONFIG` environment variables.

### Q: How do I override a specific config section?

**A:** Use environment variables:
```bash
export THEMIS_HSM_PROVIDER=aws_kms
export THEMIS_DB_PATH=/custom/path
```

---

## 🆘 Troubleshooting

### Issue: Config file not found

**Solution:**
1. Check file exists: `test -f config/...`
2. Check permissions: `ls -l config/`
3. Check working directory: `pwd`
4. Try absolute path: `/full/path/to/config.yaml`

### Issue: Invalid YAML

**Solution:**
1. Use yamllint: `yamllint config/file.yaml`
2. Check for tabs (use 2-space indent)
3. Check for invalid characters
4. Validate structure against schema

### Issue: Config loads but settings ignored

**Solution:**
1. Check for typos in config keys
2. Verify subsystem is enabled
3. Check feature flags
4. Look for environment variable overrides

---

## 📞 Support

- **Documentation:** config/MIGRATION_GUIDE.md
- **Issues:** GitHub #config-refactor
- **Community:** Discord #configuration

---

**Update note (2026-04-20):** This guide was aligned with the active `ConfigPathResolver` mapping and current canonical `core/` paths.

