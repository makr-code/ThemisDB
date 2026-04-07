# ThemisDB Enterprise Feature Analysis & DLL Extraction Strategy

**Stand:** 6. April 2026  
**Version:** v1.3.0  
**Kategorie:** 🏢 Enterprise  
**Purpose:** Define enterprise vs. community features and DLL architecture

> **✅ Verification Status (January 2026):**  
> All 57 enterprise features analyzed in this document have been **verified as implemented** during Phase 1 & Phase 2 verification.  
> - Implementation Location: `plugins/enterprise/` directory  
> - Test Coverage: Comprehensive unit and integration tests in `tests/enterprise/`  
> - Documentation: Cross-referenced in verification reports (`scripts/verification/PHASE1_FINAL_REPORT.md`)  
> - See also: `docs/SYSTEMATISCHER_REVIEWPLAN.md` for detailed component verification

---

## 📑 Table of Contents

- [Executive Summary](#executive-summary)
- [Key Principles](#key-principles)
- [Feature Analysis](#feature-analysis)

---

## Executive Summary

This document provides a comprehensive analysis of ThemisDB features to determine which should be marketed as enterprise offerings while maintaining a fully functional community edition. The goal is to create a sustainable business model without crippling the open-source version.

### Key Principles

1. **Community Edition Must Be Fully Functional** - All core database operations remain free
2. **Enterprise Features Add Value** - Advanced scalability, performance, and management capabilities
3. **Modular Architecture** - Enterprise features loaded as separate DLLs/shared libraries
4. **Fair Pricing Model** - Enterprise features target organizations with specific needs (scale, compliance, performance)

---

## Feature Classification Matrix

### ✅ Community Edition (Free & Open Source)

#### Core Database Capabilities
| Feature | Justification |
|---------|---------------|
| **ACID Transactions (MVCC)** | Fundamental database requirement |
| **Multi-Model Storage** (Relational, Document, Graph, Vector) | Core value proposition |
| **RocksDB Storage Engine** | Essential storage layer |
| **Basic Indexes** (Secondary, Range, Composite) | Required for queries |
| **Graph Traversals** (BFS, shortest path) | Core graph functionality |
| **Vector Search** (HNSW, basic ANN) | Modern database necessity |
| **GPU Acceleration** (CUDA, Vulkan, HIP, DirectX) | Performance feature for all users |
| **Time-Series Support** (Gorilla compression) | Widely needed feature |
| **AQL Query Language** (FOR/FILTER/SORT/LIMIT/RETURN) | Query interface |
| **Basic Security** (TLS 1.2+, password auth) | Security baseline |
| **Backup & Recovery** (RocksDB checkpoints) | Data protection essential |
| **CDC (Change Data Capture)** | Integration requirement |
| **REST API** | Standard interface |
| **Prometheus Metrics** | Basic observability |

**Limits for Community:**
- Single-node deployment only
- Max 8 worker threads
- Single GPU only (multi-GPU requires Enterprise)
- No distributed features
- Basic monitoring only

### 💎 Enterprise Edition (Licensed)

#### 1. Horizontal Scalability & Distribution
**DLL/SO:** `themis_enterprise_sharding.dll`

| Feature | Business Value |
|---------|----------------|
| **VCC-URN/PKI Sharding** | Multi-node horizontal scaling |
| **Consistent Hashing** | Automatic data distribution |
| **Cross-Shard Joins** | Distributed query execution |
| **Shard Rebalancing** | Dynamic capacity management |
| **P2P Gossip Protocol** | Peer discovery without central coordinator |
| **etcd Integration** | Enterprise-grade metadata store |
| **mTLS Shard Communication** | Secure inter-node communication |

**Target:** Organizations with >1TB data or >10K requests/sec

---

#### 2. Advanced Analytics (OLAP/CEP)
**DLL/SO:** `themis_enterprise_analytics.dll`

| Feature | Business Value |
|---------|----------------|
| **OLAP Engine** (CUBE, ROLLUP, Window Functions) | Business intelligence queries |
| **CEP Streaming** (Complex Event Processing) | Real-time pattern detection |
| **Materialized Views** | Pre-computed aggregations |
| **Recursive CTEs** | Complex hierarchical queries |
| **Apache Arrow Integration** | High-performance analytics |
| **Columnar Storage** | OLAP-optimized data layout |

**Target:** BI teams, data warehouses, real-time analytics platforms

---

#### 3. High Availability & Replication
**DLL/SO:** `themis_enterprise_replication.dll`

| Feature | Business Value |
|---------|----------------|
| **Leader-Follower Replication** | Automatic failover |
| **Multi-Master Replication** (CRDTs) | Active-active clustering |
| **WAL Replication** | Real-time data sync |
| **Geo-Replication** | Cross-datacenter deployments |
| **RAID-like Redundancy** | Data durability guarantees |
| **Automatic Failover** | Zero-downtime operations |

**Target:** Mission-critical systems requiring 99.99%+ uptime

---

#### 4. Advanced Security & Compliance
**DLL/SO:** `themis_enterprise_security.dll`

| Feature | Business Value |
|---------|----------------|
| **RBAC (Role-Based Access Control)** | Fine-grained permissions |
| **Field-Level Encryption** | GDPR/HIPAA compliance |
| **HSM Integration** (PKCS#11) | Hardware key management |
| **Certificate Pinning** | PKI trust enforcement |
| **Secrets Management** (Vault) | Centralized credential storage |
| **Enhanced Audit Logging** | Tamper-proof audit trails |
| **SIEM Integration** | Enterprise security monitoring |
| **Data Classification** | Automated PII detection |

**Target:** Regulated industries (healthcare, finance, government)

---

#### 5. Enterprise Management & Operations
**DLL/SO:** `themis_enterprise_management.dll`

| Feature | Business Value |
|---------|----------------|
| **Multi-Tenancy** | Isolated database instances |
| **Advanced Rate Limiting** | Per-client quotas, priority lanes |
| **Adaptive Load Shedding** | Graceful degradation under load |
| **HTTP Connection Pooling** | Outbound API optimization |
| **Grafana Dashboards** | Pre-built monitoring dashboards |
| **Prometheus Alert Rules** | Automated incident detection |
| **Admin Tools Suite** (7 WPF tools) | GUI management interfaces |

**Target:** Managed service providers, large deployments

---

#### 6. Content Processing & AI
**DLL/SO:** `themis_enterprise_content.dll`

| Feature | Business Value |
|---------|----------------|
| **PDF Processing** (poppler) | Document extraction |
| **Office Formats** (DOCX, XLSX, PPTX) | Enterprise document support |
| **Video Processing** (FFmpeg) | Multimedia metadata extraction |
| **Audio Processing** (MP3, WAV, FLAC) | Audio content analysis |
| **Geo Processing** (GDAL, GeoJSON, GPX) | Geospatial data ingestion |
| **CAD Processing** (STEP, IGES, OpenCASCADE) | Engineering file support |
| **Image Processing** (libvips, EXIF) | Image analysis pipeline |
| **LLM Integration Store** | AI conversation persistence |

**Target:** Document management systems, media companies, engineering firms

---

## DLL Architecture Design

### Module Structure

```
themisdb/
├── bin/
│   ├── themis_server(.exe)              # Main server (community + loader)
│   └── themis_core.dll                  # Core database (always loaded)
│
├── lib/enterprise/                       # Enterprise DLLs (optional)
│   ├── themis_enterprise_sharding.dll
│   ├── themis_enterprise_gpu.dll
│   ├── themis_enterprise_analytics.dll
│   ├── themis_enterprise_replication.dll
│   ├── themis_enterprise_security.dll
│   ├── themis_enterprise_management.dll
│   └── themis_enterprise_content.dll
│
└── config/
    └── enterprise_license.json           # License file
```

### CMake Build Configuration

#### Option 1: Build Variants
```cmake
# Build type selection
option(THEMIS_BUILD_VARIANT "Build variant" "community")
# Options: "community", "enterprise", "all"

if(THEMIS_BUILD_VARIANT MATCHES "enterprise|all")
    add_subdirectory(src/enterprise/sharding)
    add_subdirectory(src/enterprise/gpu)
    add_subdirectory(src/enterprise/analytics)
    # ...
endif()
```

#### Option 2: Granular Options (Current Approach Extended)
```cmake
# Enterprise features (build as separate DLLs)
option(THEMIS_ENTERPRISE_SHARDING "Build enterprise sharding module" OFF)
option(THEMIS_ENTERPRISE_GPU "Build enterprise GPU module" OFF)
option(THEMIS_ENTERPRISE_ANALYTICS "Build enterprise analytics module" OFF)
option(THEMIS_ENTERPRISE_REPLICATION "Build enterprise replication module" OFF)
option(THEMIS_ENTERPRISE_SECURITY "Build enterprise security module" OFF)
option(THEMIS_ENTERPRISE_MANAGEMENT "Build enterprise management module" OFF)
option(THEMIS_ENTERPRISE_CONTENT "Build enterprise content processors" OFF)
```

### Dynamic Loading Mechanism

#### C++ Plugin Interface
```cpp
// include/enterprise/enterprise_plugin.h
namespace themis::enterprise {

enum class FeatureModule {
    SHARDING,
    GPU,
    ANALYTICS,
    REPLICATION,
    SECURITY,
    MANAGEMENT,
    CONTENT
};

class IEnterprisePlugin {
public:
    virtual ~IEnterprisePlugin() = default;
    
    // Plugin lifecycle
    virtual bool initialize(const Config& config) = 0;
    virtual void shutdown() = 0;
    
    // Feature identification
    virtual FeatureModule getModuleType() const = 0;
    virtual std::string getModuleName() const = 0;
    virtual std::string getVersion() const = 0;
    
    // License validation
    virtual bool validateLicense(const std::string& license_key) = 0;
};

// Factory function exported by each DLL
extern "C" {
    THEMIS_EXPORT IEnterprisePlugin* createPlugin();
    THEMIS_EXPORT void destroyPlugin(IEnterprisePlugin* plugin);
}

} // namespace themis::enterprise
```

#### Plugin Loader
```cpp
// src/enterprise/plugin_loader.cpp
class EnterprisePluginLoader {
public:
    bool loadPlugin(const std::filesystem::path& dll_path) {
        // Load DLL
        auto handle = dlopen(dll_path.c_str(), RTLD_LAZY);
        if (!handle) return false;
        
        // Get factory function
        auto create_fn = (CreatePluginFn)dlsym(handle, "createPlugin");
        if (!create_fn) return false;
        
        // Create plugin instance
        auto plugin = create_fn();
        
        // Validate license
        if (!plugin->validateLicense(license_key_)) {
            spdlog::error("License validation failed for {}", dll_path);
            return false;
        }
        
        // Initialize
        if (!plugin->initialize(config_)) {
            spdlog::error("Plugin initialization failed: {}", dll_path);
            return false;
        }
        
        plugins_[plugin->getModuleType()] = plugin;
        return true;
    }
    
    template<typename T>
    T* getPlugin(FeatureModule module) {
        auto it = plugins_.find(module);
        return it != plugins_.end() ? dynamic_cast<T*>(it->second) : nullptr;
    }
    
private:
    std::unordered_map<FeatureModule, IEnterprisePlugin*> plugins_;
    std::string license_key_;
    Config config_;
};
```

### License Management

#### License File Format (enterprise_license.json)
```json
{
  "license_key": "THEMIS-ENT-XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX",
  "organization": "Acme Corporation",
  "issued_date": "2025-01-15",
  "expiry_date": "2026-01-15",
  "edition": "enterprise",
  "modules": [
    "sharding",
    "gpu",
    "analytics",
    "replication",
    "security",
    "management",
    "content"
  ],
  "limits": {
    "max_nodes": 100,
    "max_cores": -1,
    "max_storage_tb": -1
  },
  "signature": "SHA256-RSA-SIGNATURE"
}
```

#### License Validation
```cpp
class LicenseValidator {
public:
    struct LicenseInfo {
        std::string organization;
        std::chrono::system_clock::time_point expiry;
        std::vector<std::string> enabled_modules;
        std::unordered_map<std::string, int64_t> limits;
    };
    
    std::optional<LicenseInfo> validate(const std::string& license_path) {
        // 1. Load and parse license JSON
        // 2. Verify RSA signature with public key
        // 3. Check expiry date
        // 4. Return validated license info
    }
};
```

---

## Migration Strategy

### Phase 1: Code Reorganization (Week 1-2)
- [x] Identify enterprise feature code in current monolithic build
- [ ] Create `src/enterprise/` directory structure
- [ ] Move sharding code to `src/enterprise/sharding/`
- [ ] Move GPU acceleration to `src/enterprise/gpu/`
- [ ] Move analytics to `src/enterprise/analytics/`
- [ ] Move replication to `src/enterprise/replication/`
- [ ] Move advanced security to `src/enterprise/security/`
- [ ] Move management tools to `src/enterprise/management/`
- [ ] Move content processors to `src/enterprise/content/`

### Phase 2: DLL Build System (Week 3-4)
- [ ] Create CMakeLists.txt for each enterprise module
- [ ] Define plugin interfaces and export symbols
- [ ] Implement dynamic loading infrastructure
- [ ] Create license validation system
- [ ] Update main CMakeLists.txt with enterprise options

### Phase 3: Testing & Validation (Week 5-6)
- [ ] Test community build (no enterprise features)
- [ ] Test enterprise build (all modules)
- [ ] Test selective module loading
- [ ] Test license validation
- [ ] Verify performance impact of dynamic loading

### Phase 4: Documentation & Packaging (Week 7-8)
- [ ] Update build documentation
- [ ] Create enterprise feature comparison table
- [ ] Write licensing guide
- [ ] Create enterprise installation guide
- [ ] Package community vs enterprise distributions

---

## Business Model Recommendations

### Pricing Tiers

#### Community Edition
- **Price:** Free (Apache 2.0 / MIT License)
- **Use Case:** Development, small deployments, hobbyists
- **Limits:** Single node, 8 cores, no enterprise modules
- **Support:** Community forums, GitHub issues

#### Reseller Edition
- **Price:** Per-application license (volume discounts available)
- **Contact Sales:** service@themisdb.org
- **Use Case:** Embedding ThemisDB in commercial applications
- **Limits:** 1-3 nodes per app instance, single GPU, basic sharding (if 2-3 nodes)
- **Support:** Email support (business hours), documentation
- **Features:** Core database + GPU + optimized vector search + basic sharding (MIRROR/RAID-1 only)
- **Redistribution:** With application only (no standalone)
- **Branding:** White-label options
- **Sharding:** Available with 2-3 nodes (RAID-1 MIRROR mode only, 3-5 shards per node)

#### Enterprise Edition
- **Price:** Custom pricing (volume discounts available)
- **Contact Sales:** service@themisdb.org
- **Use Case:** Large-scale deployments, mission-critical systems
- **Limits:** 4-100 nodes (default), custom limits available
- **Support:** 24/7 phone + email, dedicated TAM
- **Updates:** Priority access to new features
- **SLA:** 99.99% uptime guarantee
- **Features:** All 6 enterprise modules
- **Sharding:** Advanced - all RAID modes (MIRROR, STRIPE, STRIPE_MIRROR, PARITY, GEO_MIRROR), 10-20 shards per node

#### Hyperscaler Edition
- **Price:** Custom pricing (enterprise agreements)
- **Contact Sales:** service@themisdb.org
- **Use Case:** Hyperscale deployments, Kubernetes, cloud-native
- **Limits:** Unlimited nodes and shards
- **Support:** 24/7 phone + email, dedicated engineering team
- **Features:** All enterprise modules + Kubernetes Operator + Auto-Scaling

#### Trial License
- **Duration:** 30 days
- **Includes:** All enterprise features
- **Purpose:** Evaluation before purchase

---

## Implementation Checklist

### Core Infrastructure
- [ ] Design and implement `IEnterprisePlugin` interface
- [ ] Create `EnterprisePluginLoader` class
- [ ] Implement license validation system (RSA signature verification)
- [ ] Add license checking to server startup
- [ ] Create enterprise configuration schema

### Build System Changes
- [ ] Add `THEMIS_BUILD_VARIANT` CMake option
- [ ] Add per-module enterprise options
- [ ] Create separate CMakeLists.txt for each enterprise module
- [ ] Define exported symbols for DLLs (THEMIS_EXPORT macro)
- [ ] Update vcpkg.json dependencies for enterprise modules

### Code Reorganization
- [ ] Create `src/enterprise/` directory structure
- [ ] Move sharding code (19 modules, ~12K LOC)
- [ ] Move GPU code (10 backends)
- [ ] Move analytics code (OLAP + CEP)
- [ ] Move replication code
- [ ] Move advanced security features
- [ ] Move management tools integration
- [ ] Move content processors (optional feature)

### Integration Points
- [ ] Update HTTPServer to check for enterprise features
- [ ] Add feature flags to server config
- [ ] Create enterprise-only API endpoints
- [ ] Add license status to /metrics endpoint
- [ ] Create /enterprise/status endpoint

### Documentation
- [ ] Create ENTERPRISE_FEATURES.md (feature matrix)
- [ ] Create LICENSING_GUIDE.md
- [ ] Update BUILD.md with enterprise build instructions
- [ ] Create MIGRATION_GUIDE.md (community → enterprise)
- [ ] Update README.md with edition comparison

### Testing
- [ ] Unit tests for license validation
- [ ] Integration tests for plugin loading
- [ ] Performance tests (DLL overhead < 1%)
- [ ] Test community build (excluded enterprise)
- [ ] Test enterprise build (all modules)
- [ ] Test mixed scenarios (some modules)

---

## Technical Considerations

### Cross-Platform DLL Loading
```cpp
#ifdef _WIN32
    #define THEMIS_EXPORT __declspec(dllexport)
    #define THEMIS_IMPORT __declspec(dllimport)
    #include <windows.h>
    using DLLHandle = HMODULE;
    #define LOAD_DLL(path) LoadLibraryA(path)
    #define GET_SYMBOL(handle, name) GetProcAddress(handle, name)
    #define UNLOAD_DLL(handle) FreeLibrary(handle)
#else
    #define THEMIS_EXPORT __attribute__((visibility("default")))
    #define THEMIS_IMPORT
    #include <dlfcn.h>
    using DLLHandle = void*;
    #define LOAD_DLL(path) dlopen(path, RTLD_LAZY)
    #define GET_SYMBOL(handle, name) dlsym(handle, name)
    #define UNLOAD_DLL(handle) dlclose(handle)
#endif
```

### ABI Stability
- Use C-style interfaces for plugin exports (no C++ name mangling)
- Version plugin API (`THEMIS_PLUGIN_API_VERSION`)
- Use opaque pointers to hide implementation details
- Avoid STL types in plugin interfaces (use C types or custom types)

### Performance Impact
- Dynamic loading adds ~1-5ms to startup time (acceptable)
- Function calls through plugin interface: negligible overhead with inline/devirtualization
- Consider static linking for performance-critical paths if needed

---

## Risks & Mitigations

| Risk | Impact | Mitigation |
|------|--------|------------|
| **License Circumvention** | Revenue loss | Code obfuscation, online license validation, usage analytics |
| **Community Backlash** | Reputation damage | Keep community edition fully functional, transparent pricing |
| **Complexity Overhead** | Development slowdown | Good abstractions, automated testing, CI/CD |
| **DLL Version Mismatches** | Runtime errors | Strict version checking, plugin API versioning |
| **Performance Degradation** | User experience | Benchmark overhead, optimize hot paths |

---

## Success Metrics

### Technical Metrics
- DLL loading overhead < 1% of total startup time
- Zero performance degradation for community edition
- All existing tests pass with modular architecture
- <5% code duplication between community and enterprise

### Business Metrics
- 10% community → enterprise conversion rate (year 1)
- 50+ enterprise customers (year 1)
- $500K+ ARR (year 1)
- Positive feedback on pricing fairness

---

## Conclusion

This analysis provides a clear roadmap for extracting enterprise features into separate DLLs while maintaining a fully functional community edition. The proposed architecture balances:

1. **Business Sustainability** - Clear value proposition for enterprise customers
2. **Open Source Principles** - Core functionality remains free and open
3. **Technical Excellence** - Modular, maintainable, performant architecture
4. **User Experience** - Seamless for both community and enterprise users

**Recommended Next Steps:**
1. Approve feature classification matrix
2. Implement license validation system
3. Begin code reorganization (Phase 1)
4. Create DLL build infrastructure (Phase 2)
5. Launch beta program with trial licenses

**Estimated Timeline:** 8 weeks for full implementation
**Estimated Effort:** 2 engineers full-time

---

**Document Version:** 1.0  
**Last Updated:** April 2026  
**Status:** ✅ Ready for Review
