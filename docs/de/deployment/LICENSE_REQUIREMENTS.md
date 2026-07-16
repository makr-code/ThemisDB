# License Requirements & Enforcement

**Stand:** 23. April 2026  
**Version:** v1.4.0  
**Kategorie:** 🔐 Licensing  
**Status:** Production-Ready

---

## 📑 Inhaltsverzeichnis

- [Quick Reference](#-quick-reference)
- [MINIMAL & COMMUNITY: No License Required](#-minimal--community-no-license-required)
- [ENTERPRISE: License Required for Release](#-enterprise-license-required-for-release)
- [HYPERSCALER: License Mandatory](#-hyperscaler-license-mandatory)
- [How to Get a License](#-how-to-get-a-license)
- [How to Embed a License](#-how-to-embed-a-license)
- [Runtime Validation](#-runtime-validation)
- [Troubleshooting](#-troubleshooting)

---

## 📊 Quick Reference

### License Requirements Matrix

| Edition | Debug Build | Release Build | epServer Required |
|---------|------------|---------------|-------------------|
| **MINIMAL** | ✅ Optional | ✅ Optional | ❌ No |
| **COMMUNITY** | ✅ Optional | ✅ Optional | ❌ No |
| **ENTERPRISE** | ✅ Optional | ❌ **Required** ⚠️ | ✅ Yes |
| **HYPERSCALER** | ❌ **Mandatory** | ❌ **Mandatory** | ✅ Yes |

### Decision Tree

```
                  ┌─────────────────┐
                  │  Which Edition? │
                  └────────┬────────┘
                           │
        ┌──────────────────┼──────────────────┐
        │                  │                  │
   ┌────▼────┐      ┌──────▼──────┐    ┌─────▼──────┐
   │ MINIMAL │      │  COMMUNITY  │    │ ENTERPRISE │
   │COMMUNITY│      │             │    │HYPERSCALER │
   └────┬────┘      └──────┬──────┘    └─────┬──────┘
        │                  │                  │
        │                  │                  │
   ✅ No License     ✅ No License      Is it Release?
      Required          Required              │
                                     ┌────────┴────────┐
                                     │                 │
                                ┌────▼────┐      ┌─────▼──────┐
                                │  Debug  │      │  Release   │
                                └────┬────┘      └─────┬──────┘
                                     │                 │
                              ✅ Optional         ❌ Required
                                                  (ENTERPRISE)
                                                  ❌ Mandatory
                                                  (HYPERSCALER)
```

---

## ✅ MINIMAL & COMMUNITY: No License Required

### Übersicht

MINIMAL und COMMUNITY Editions sind **vollständig Open Source (MIT-Lizenz)** und benötigen **keine kommerzielle Lizenz**:

```
┌─────────────────────────────────────────────┐
│   MINIMAL & COMMUNITY: No License Required  │
├─────────────────────────────────────────────┤
│                                             │
│  ✅ Debug Build:    No License              │
│  ✅ Release Build:  No License              │
│  ✅ Distribution:   No License              │
│  ✅ Commercial Use: No License              │
│                                             │
│  📝 Lizenz:  MIT (Open Source)              │
│  💰 Kosten:  Kostenlos                      │
│  📧 Kontakt: Nicht erforderlich             │
│                                             │
└─────────────────────────────────────────────┘
```

### Build Examples

```bash
# MINIMAL - Debug
cmake -B build -S . \
  -DTHEMIS_EDITION=MINIMAL \
  -DCMAKE_BUILD_TYPE=Debug
cmake --build build
# ✅ Success - Kein Problem

# MINIMAL - Release
cmake -B build -S . \
  -DTHEMIS_EDITION=MINIMAL \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build
# ✅ Success - Kein Problem

# COMMUNITY - Debug
cmake -B build -S . \
  -DTHEMIS_EDITION=COMMUNITY \
  -DCMAKE_BUILD_TYPE=Debug
cmake --build build
# ✅ Success - Kein Problem

# COMMUNITY - Release
cmake -B build -S . \
  -DTHEMIS_EDITION=COMMUNITY \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build
# ✅ Success - Kein Problem
```

### Optional License Embedding

Auch bei MINIMAL/COMMUNITY können Sie **optional** eine Lizenz einbetten (z.B. für Custom Deployments):

```bash
# Optional: Custom License für OEM
cmake -B build -S . \
  -DTHEMIS_EDITION=COMMUNITY \
  -DCMAKE_BUILD_TYPE=Release \
  -DTHEMIS_LICENSE_FILE=/path/to/custom-license.json
# ✅ Success - Optional, aber unterstützt
```

**Use Cases für optionale Lizenzen:**
- OEM Branding
- Custom Support Agreements
- Tracking & Analytics
- White-Label Deployments

---

## ⚠️ ENTERPRISE: License Required for Release

### Übersicht

ENTERPRISE Edition benötigt eine **Lizenz für Release Builds**, aber **nicht für Debug Builds**:

```
┌─────────────────────────────────────────────┐
│   ENTERPRISE: License Required for Release  │
├─────────────────────────────────────────────┤
│                                             │
│  Debug Builds:                              │
│  ✅ No License Required (Development)       │
│  ✅ Full Flexibility für Entwickler         │
│  ✅ Lokale Tests ohne Lizenz möglich        │
│                                             │
│  Release Builds:                            │
│  ❌ License REQUIRED (Production Safety)    │
│  🔒 Build fails without license             │
│  📧 service@themisdb.org                  │
│                                             │
└─────────────────────────────────────────────┘
```

### Rationale: Why Debug is Optional?

**Development Flexibility:**
- ✅ Entwickler können lokal ohne Lizenz entwickeln
- ✅ CI/CD Debug Builds benötigen keine Lizenz
- ✅ Testing & Prototyping einfacher
- ✅ Keine Lizenz-Verwaltung während der Entwicklung

**Production Safety:**
- 🔒 Release Builds benötigen Lizenz (Production-Gating)
- 🔒 Verhindert versehentliche Production-Deployments ohne Lizenz
- 🔒 Compliance & Audit Trail

### Debug Build (No License Required)

```bash
# ✅ SUCCESS: ENTERPRISE Debug ohne Lizenz
cmake -B build -S . \
  -DTHEMIS_EDITION=ENTERPRISE \
  -DCMAKE_BUILD_TYPE=Debug

cmake --build build
# ✅ Build erfolgreich - keine Lizenz erforderlich

# Tests laufen ohne Lizenz
ctest --test-dir build
# ✅ Tests erfolgreich
```

**Output:**
```
-- ThemisDB ENTERPRISE Edition (Debug)
-- License: Optional (Debug mode - development flexibility)
-- Configuring done
-- Generating done
-- Build files have been written to: /path/to/build
```

### Release Build (License Required)

```bash
# ❌ ERROR: ENTERPRISE Release ohne Lizenz
cmake -B build -S . \
  -DTHEMIS_EDITION=ENTERPRISE \
  -DCMAKE_BUILD_TYPE=Release

# Build fails with helpful error
```

**Error Output:**
```
CMake Error at CMakeLists.txt:123 (message):
  ╔════════════════════════════════════════════════════════════════╗
  ║  ENTERPRISE Edition: License Required for Release Builds      ║
  ╠════════════════════════════════════════════════════════════════╣
  ║                                                                ║
  ║  Release builds require a valid ENTERPRISE license.            ║
  ║                                                                ║
  ║  Options:                                                      ║
  ║  1. Add license: -DTHEMIS_LICENSE_FILE=/path/to/license.json  ║
  ║  2. Use Debug mode: -DCMAKE_BUILD_TYPE=Debug                  ║
  ║                                                                ║
  ║  To obtain a license:                                          ║
  ║  - Email: service@themisdb.org                               ║
  ║  - Web: https://themisdb.org/enterprise                         ║
  ║  - Trial: 30 days free                                         ║
  ║                                                                ║
  ╚════════════════════════════════════════════════════════════════╝

-- Configuring incomplete, errors occurred!
```

### Release Build with License (Success)

```bash
# ✅ SUCCESS: ENTERPRISE Release mit Lizenz
cmake -B build -S . \
  -DTHEMIS_EDITION=ENTERPRISE \
  -DCMAKE_BUILD_TYPE=Release \
  -DTHEMIS_LICENSE_FILE=/path/to/enterprise-license.json

cmake --build build
# ✅ Build erfolgreich
```

**Output:**
```
-- ThemisDB ENTERPRISE Edition (Release)
-- License file: /path/to/enterprise-license.json
-- License validation: OK
-- License holder: ACME Corporation
-- License expires: 2027-12-31
-- Max nodes: 100
-- Configuring done
-- Generating done
-- Build files have been written to: /path/to/build
```

### RelWithDebInfo & MinSizeRel

```bash
# ❌ RelWithDebInfo benötigt Lizenz (gilt als Release)
cmake -B build -S . \
  -DTHEMIS_EDITION=ENTERPRISE \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DTHEMIS_LICENSE_FILE=/path/to/license.json
# ✅ Success mit Lizenz

# ❌ MinSizeRel benötigt Lizenz (gilt als Release)
cmake -B build -S . \
  -DTHEMIS_EDITION=ENTERPRISE \
  -DCMAKE_BUILD_TYPE=MinSizeRel \
  -DTHEMIS_LICENSE_FILE=/path/to/license.json
# ✅ Success mit Lizenz
```

**Regel:** Nur `CMAKE_BUILD_TYPE=Debug` ist lizenzfrei. Alle anderen Modi benötigen Lizenz.

---

## ❌ HYPERSCALER: License Mandatory

### Übersicht

HYPERSCALER Edition benötigt **Lizenz für ALLE Builds** (Debug & Release):

```
┌─────────────────────────────────────────────┐
│     HYPERSCALER: License Mandatory          │
├─────────────────────────────────────────────┤
│                                             │
│  ❌ Debug Builds:   License MANDATORY       │
│  ❌ Release Builds: License MANDATORY       │
│  🔒 No exceptions                           │
│  📧 service@themisdb.org                         │
│                                             │
│  💼 OEM Deal erforderlich                   │
│  👥 Dedicated Engineering Team              │
│  🎯 Custom Features                         │
│                                             │
└─────────────────────────────────────────────┘
```

### Rationale: Why No Debug Exception?

**HYPERSCALER ist für OEM/Cloud Provider:**
- 🔒 Unbegrenzte Nodes/GPU (höchster Wert)
- 🔒 Custom Engineering & Features
- 🔒 White-Label & OEM Branding
- 🔒 Source Code Access (optional)

**Keine Debug-Ausnahme:**
- Kein "Development Mode" - nur Production Use
- OEM Partner haben bereits License Agreement
- Dedicated Support Team für alle Builds

### Debug Build (License Required)

```bash
# ❌ ERROR: HYPERSCALER Debug ohne Lizenz
cmake -B build -S . \
  -DTHEMIS_EDITION=HYPERSCALER \
  -DCMAKE_BUILD_TYPE=Debug

# Build fails
```

**Error Output:**
```
CMake Error at CMakeLists.txt:145 (message):
  ╔════════════════════════════════════════════════════════════════╗
  ║  HYPERSCALER Edition: License Mandatory for All Builds        ║
  ╠════════════════════════════════════════════════════════════════╣
  ║                                                                ║
  ║  HYPERSCALER Edition requires a valid license for all builds, ║
  ║  including Debug builds.                                       ║
  ║                                                                ║
  ║  Usage: -DTHEMIS_LICENSE_FILE=/path/to/license.json           ║
  ║                                                                ║
  ║  To obtain a HYPERSCALER license:                              ║
  ║  - Email: service@themisdb.org                                      ║
  ║  - Web: https://themisdb.org/hyperscaler                        ║
  ║  - OEM Deal required                                           ║
  ║                                                                ║
  ║  Note: There is no Debug build exception for HYPERSCALER.     ║
  ║                                                                ║
  ╚════════════════════════════════════════════════════════════════╝

-- Configuring incomplete, errors occurred!
```

### Debug Build with License (Success)

```bash
# ✅ SUCCESS: HYPERSCALER Debug mit Lizenz
cmake -B build -S . \
  -DTHEMIS_EDITION=HYPERSCALER \
  -DCMAKE_BUILD_TYPE=Debug \
  -DTHEMIS_LICENSE_FILE=/path/to/hyperscaler-license.json

cmake --build build
# ✅ Build erfolgreich
```

### Release Build with License (Success)

```bash
# ✅ SUCCESS: HYPERSCALER Release mit Lizenz
cmake -B build -S . \
  -DTHEMIS_EDITION=HYPERSCALER \
  -DCMAKE_BUILD_TYPE=Release \
  -DTHEMIS_LICENSE_FILE=/path/to/hyperscaler-license.json

cmake --build build
# ✅ Build erfolgreich
```

**Output:**
```
-- ThemisDB HYPERSCALER Edition (Release)
-- License file: /path/to/hyperscaler-license.json
-- License validation: OK
-- License holder: AWS Cloud Provider
-- License type: OEM Unlimited
-- Max nodes: UNLIMITED
-- Max GPU VRAM: UNLIMITED
-- Configuring done
-- Generating done
-- Build files have been written to: /path/to/build
```

---

## 📧 How to Get a License

### ENTERPRISE License

**Kontakt:**
- 📧 Email: service@themisdb.org
- 🌐 Web: https://themisdb.org/enterprise
- 📞 Phone: +49 (0)123 456789 (optional)

**Prozess:**
1. **Trial anfragen:** 30 Tage kostenlos
2. **Evaluation:** Testen mit voller Enterprise-Funktionalität
3. **Purchase:** Subscription abschließen (€5k+/Monat)
4. **License File:** Erhalten via Email (.json)

**Was Sie erhalten:**
- ✅ `enterprise-license.json` Datei
- ✅ 24/7 Commercial Support
- ✅ SLA 99.9%
- ✅ Updates & Patches
- ✅ Max 100 Nodes

**Trial License Example:**

Request:
```bash
curl -X POST https://api.themisdb.org/v1/licenses/trial \
  -H "Content-Type: application/json" \
  -d '{
    "company": "ACME Corp",
    "email": "admin@acme.com",
    "edition": "ENTERPRISE",
    "nodes": 10
  }'
```

Response:
```json
{
  "license_id": "ent-trial-abc123",
  "valid_until": "2026-02-23",
  "download_url": "https://licenses.themisdb.org/trial/abc123.json"
}
```

### HYPERSCALER License

**Kontakt:**
- 📧 Email: service@themisdb.org
- 🌐 Web: https://themisdb.org/hyperscaler

**Prozess:**
1. **OEM Inquiry:** Kontaktaufnahme mit Ihrer Use Case
2. **Discussion:** Custom Requirements, Scale, Features
3. **Proposal:** Custom OEM Deal
4. **Contract:** OEM Agreement unterzeichnen
5. **License File:** Erhalten via secure channel

**Was Sie erhalten:**
- ✅ `hyperscaler-license.json` Datei
- ✅ Unlimited Nodes
- ✅ Unlimited GPU VRAM
- ✅ Dedicated Engineering Team
- ✅ SLA 99.99%
- ✅ Custom Features (optional)
- ✅ Source Access (optional)
- ✅ White-Label (optional)

---

## 🔧 How to Embed a License

### CMake Integration

**Method 1: CMake Command Line**

```bash
cmake -B build -S . \
  -DTHEMIS_EDITION=ENTERPRISE \
  -DCMAKE_BUILD_TYPE=Release \
  -DTHEMIS_LICENSE_FILE=/path/to/enterprise-license.json

cmake --build build
```

**Method 2: Environment Variable**

```bash
export THEMIS_LICENSE_FILE=/path/to/license.json

cmake -B build -S . \
  -DTHEMIS_EDITION=ENTERPRISE \
  -DCMAKE_BUILD_TYPE=Release

cmake --build build
```

**Method 3: CMakePresets.json**

```json
{
  "version": 3,
  "configurePresets": [
    {
      "name": "enterprise-release",
      "binaryDir": "build-enterprise",
      "cacheVariables": {
        "THEMIS_EDITION": "ENTERPRISE",
        "CMAKE_BUILD_TYPE": "Release",
        "THEMIS_LICENSE_FILE": "/path/to/enterprise-license.json"
      }
    }
  ]
}
```

```bash
cmake --preset enterprise-release
cmake --build --preset enterprise-release
```

### License File Format

**enterprise-license.json:**

```json
{
  "license_version": "1.0",
  "license_id": "ent-prod-xyz789",
  "edition": "ENTERPRISE",
  "holder": {
    "company": "ACME Corporation",
    "email": "admin@acme.com",
    "country": "Germany"
  },
  "limits": {
    "max_nodes": 100,
    "max_gpu_vram_gb": 256
  },
  "validity": {
    "issued_at": "2026-01-01T00:00:00Z",
    "expires_at": "2027-01-01T00:00:00Z"
  },
  "features": [
    "multi_master_replication",
    "rbac",
    "field_encryption",
    "hsm_integration",
    "enterprise_plugins"
  ],
  "signature": "base64_encoded_signature_here..."
}
```

**hyperscaler-license.json:**

```json
{
  "license_version": "1.0",
  "license_id": "hyp-oem-aws001",
  "edition": "HYPERSCALER",
  "holder": {
    "company": "AWS Cloud Provider",
    "email": "oem@aws.amazon.com",
    "country": "USA"
  },
  "limits": {
    "max_nodes": 0,  // Unlimited
    "max_gpu_vram_gb": 0  // Unlimited
  },
  "validity": {
    "issued_at": "2026-01-01T00:00:00Z",
    "expires_at": "2030-01-01T00:00:00Z"  // Long-term OEM
  },
  "features": [
    "unlimited_scale",
    "custom_features",
    "oem_branding",
    "source_access",
    "dedicated_support"
  ],
  "oem": {
    "brand_name": "AWS ThemisDB Service",
    "white_label": true,
    "custom_features": ["aws_s3_integration", "aws_iam_auth"]
  },
  "signature": "base64_encoded_signature_here..."
}
```

### Build-Time Validation

CMake validiert die Lizenz zur Build-Zeit:

```cmake
# cmake/LicenseValidation.cmake

function(validate_license)
    if(NOT EXISTS ${THEMIS_LICENSE_FILE})
        message(FATAL_ERROR "License file not found: ${THEMIS_LICENSE_FILE}")
    endif()
    
    # Parse JSON
    file(READ ${THEMIS_LICENSE_FILE} LICENSE_JSON)
    
    # Validate signature
    validate_signature(${LICENSE_JSON})
    
    # Check expiry
    check_license_expiry(${LICENSE_JSON})
    
    # Check edition match
    check_edition_match(${LICENSE_JSON} ${THEMIS_EDITION})
    
    # Embed license into binary
    configure_file(
        ${THEMIS_LICENSE_FILE}
        ${CMAKE_BINARY_DIR}/embedded_license.json
        COPYONLY
    )
endfunction()
```

---

## 🔍 Runtime Validation

### Server Startup Check

```cpp
// src/main_server.cpp

#ifdef THEMIS_EDITION_ENTERPRISE
void validate_runtime_license() {
    // Read embedded license
    auto license = read_embedded_license();
    
    // Check expiry
    if (license.expired()) {
        throw std::runtime_error(
            "ENTERPRISE license expired: " + license.expires_at().to_string() +
            "\nRenew at: service@themisdb.org"
        );
    }
    
    // Check signature
    if (!license.verify_signature()) {
        throw std::runtime_error(
            "Invalid license signature! License may be tampered."
        );
    }
    
    // Log license info
    THEMIS_INFO("License holder: {}", license.holder().company);
    THEMIS_INFO("License expires: {}", license.expires_at());
    THEMIS_INFO("Max nodes: {}", license.limits().max_nodes);
    
    // Start license expiry monitor
    start_license_expiry_monitor(license);
}
#endif
```

### Runtime Enforcement

```cpp
// src/clustering/cluster_manager.cpp

void ClusterManager::add_node(const NodeInfo& node) {
    #ifdef THEMIS_EDITION_ENTERPRISE
        auto license = get_license();
        
        if (active_nodes.size() >= license.limits().max_nodes) {
            throw std::runtime_error(
                fmt::format("Max nodes exceeded: {} (license limit: {})\n"
                           "Upgrade to HYPERSCALER for unlimited nodes.",
                           active_nodes.size() + 1,
                           license.limits().max_nodes)
            );
        }
    #endif
    
    active_nodes.push_back(node);
}
```

### License Expiry Warning

```cpp
// src/license/license_monitor.cpp

void LicenseMonitor::check_expiry() {
    auto license = get_license();
    auto days_remaining = license.days_until_expiry();
    
    if (days_remaining <= 30) {
        THEMIS_WARN("License expires in {} days! Renew at: service@themisdb.org",
                    days_remaining);
    }
    
    if (days_remaining <= 7) {
        THEMIS_ERROR("License expires in {} days! URGENT renewal required!",
                     days_remaining);
    }
    
    if (days_remaining <= 0) {
        THEMIS_CRITICAL("License EXPIRED! Server will shutdown in 24 hours.");
        // Graceful shutdown window
    }
}
```

---

## 🔧 Troubleshooting

### Error: License File Not Found

**Problem:**
```
CMake Error: License file not found: /path/to/license.json
```

**Lösungen:**
1. **Pfad prüfen:** Existiert die Datei?
   ```bash
   ls -la /path/to/license.json
   ```

2. **Absoluter Pfad:** Verwenden Sie absoluten Pfad
   ```bash
   cmake -B build -S . \
     -DTHEMIS_LICENSE_FILE=/absolute/path/to/license.json
   ```

3. **Environment Variable:** Setzen Sie THEMIS_LICENSE_FILE
   ```bash
   export THEMIS_LICENSE_FILE=/path/to/license.json
   cmake -B build -S .
   ```

### Error: License Expired

**Problem:**
```
ERROR: ENTERPRISE license expired: 2026-01-01
Renew at: service@themisdb.org
```

**Lösungen:**
1. **Lizenz erneuern:** Kontaktiere service@themisdb.org
2. **Neue Lizenz erhalten:** Download neue `license.json`
3. **Rebuild:** Mit neuer Lizenz
   ```bash
   cmake -B build -S . \
     -DTHEMIS_LICENSE_FILE=/path/to/new-license.json
   cmake --build build
   ```

### Error: Invalid Signature

**Problem:**
```
ERROR: Invalid license signature! License may be tampered.
```

**Lösungen:**
1. **Lizenz neu herunterladen:** Möglicherweise korrupiert
2. **Keine manuelle Bearbeitung:** Lizenz nicht editieren!
3. **Support kontaktieren:** service@themisdb.org

### Error: Edition Mismatch

**Problem:**
```
ERROR: License edition mismatch!
License: ENTERPRISE
Build: HYPERSCALER
```

**Lösungen:**
1. **Richtige Lizenz verwenden:**
   ```bash
   # Für ENTERPRISE
   cmake -B build -S . \
     -DTHEMIS_EDITION=ENTERPRISE \
     -DTHEMIS_LICENSE_FILE=/path/to/enterprise-license.json
   
   # Für HYPERSCALER
   cmake -B build -S . \
     -DTHEMIS_EDITION=HYPERSCALER \
     -DTHEMIS_LICENSE_FILE=/path/to/hyperscaler-license.json
   ```

### Error: Max Nodes Exceeded (Runtime)

**Problem:**
```
ERROR: Max nodes exceeded: 101 (license limit: 100)
Upgrade to HYPERSCALER for unlimited nodes.
```

**Lösungen:**
1. **Weniger Nodes:** Reduzieren auf ≤100
2. **Upgrade zu HYPERSCALER:** Kontaktiere service@themisdb.org

### Warning: License Expires Soon

**Problem:**
```
WARNING: License expires in 7 days! URGENT renewal required!
```

**Lösungen:**
1. **Lizenz erneuern:** Kontaktiere service@themisdb.org
2. **Trial verlängern:** Falls Trial-Lizenz
3. **Subscription checken:** Auto-Renewal aktiviert?

---

## 📊 Zusammenfassung

### License Matrix

| Edition | Debug | Release | Contact |
|---------|-------|---------|---------|
| **MINIMAL** | ✅ Optional | ✅ Optional | N/A (Open Source) |
| **COMMUNITY** | ✅ Optional | ✅ Optional | N/A (Open Source) |
| **ENTERPRISE** | ✅ Optional | ❌ **Required** | service@themisdb.org |
| **HYPERSCALER** | ❌ **Mandatory** | ❌ **Mandatory** | service@themisdb.org |

### Key Takeaways

✅ **MINIMAL/COMMUNITY:** Keine Lizenz erforderlich (MIT Open Source)  
⚠️ **ENTERPRISE:** Lizenz nur für Release Builds erforderlich  
❌ **HYPERSCALER:** Lizenz für ALLE Builds erforderlich

**Development Flexibility:**
- ENTERPRISE Debug Builds sind lizenzfrei (Development)
- ENTERPRISE Release Builds benötigen Lizenz (Production)
- HYPERSCALER hat keine Debug-Ausnahme (OEM only)

**Siehe auch:**
- [Edition Limits Matrix](EDITION_LIMITS_MATRIX.md) - Vollständiger Vergleich
- [CMake Build System Overview](CMAKE_BUILD_SYSTEM_OVERVIEW.md) - Architektur
- [License Embedding Guide](../guides/LICENSE_EMBEDDING_EPSERVER.md) - epServer Integration

---

**Letzte Aktualisierung:** 23. April 2026  
**Version:** v1.4.0  
**Kontakt:** service@themisdb.org | service@themisdb.org
