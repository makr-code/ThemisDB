# Build Binding System - Design Document

## Konzept: Lizenz-gebundene Builds

### Übersicht
Jeder Enterprise/Hyperscaler Build wird kryptographisch an einen spezifischen Lizenzschlüssel gebunden. Der SHA256-Hash des Builds wird im Pricing Server gespeichert und bei der Lizenzvalidierung geprüft.

---

## Architektur

### 1. Build-Prozess (CI/CD)

```
Kunde kauft Lizenz → License Key generiert (THEMIS-ENT-ABC-XYZ)
        ↓
Build-System erstellt Custom Build mit eingebettetem License Key
        ↓
SHA256 Hash von themis_server.exe + kritische Module berechnet
        ↓
Hash wird im Pricing Server für diese Lizenz registriert
        ↓
Build wird dem Kunden ausgeliefert
```

### 2. Runtime-Validierung

```
ThemisDB Server startet
        ↓
Liest eingebetteten License Key aus Binary
        ↓
Berechnet eigenen SHA256 Hash (themis_server.exe + Module)
        ↓
Sendet License Key + SHA256 Hash an Pricing Server
        ↓
Server prüft: Hash stimmt mit registriertem Hash überein?
        ↓
JA → Lizenz gültig + Hash validiert ✅
NEIN → Lizenz ungültig, Binary manipuliert ❌
```

---

## Implementierung

### 1. Database Schema Erweiterung

```sql
-- Erweitere subscriptions Tabelle
ALTER TABLE subscriptions 
ADD COLUMN build_hash VARCHAR(64),  -- SHA256 hash of themis_server.exe
ADD COLUMN build_timestamp TIMESTAMP WITH TIME ZONE,
ADD COLUMN build_version VARCHAR(50),
ADD COLUMN build_platform VARCHAR(50),  -- windows-x64, linux-x64, etc.
ADD COLUMN additional_hashes JSONB;  -- Hashes von kritischen Modulen

-- Index für schnelle Hash-Suche
CREATE INDEX idx_subscriptions_build_hash ON subscriptions(build_hash);

-- Beispiel für additional_hashes:
-- {
--   "themis_core.dll": "abc123...",
--   "themis_enterprise.dll": "def456...",
--   "themis_gpu.dll": "ghi789..."
-- }
```

### 2. Build-Script (Beispiel für Windows)

```powershell
# scripts/build-licensed-server.ps1

param(
    [Parameter(Mandatory=$true)]
    [string]$LicenseKey,
    
    [Parameter(Mandatory=$true)]
    [string]$CustomerId,
    
    [string]$Platform = "windows-x64",
    [string]$OutputDir = "builds/licensed"
)

Write-Host "Building licensed ThemisDB for license: $LicenseKey"

# 1. Erstelle Build mit eingebettetem License Key
$env:THEMIS_EMBEDDED_LICENSE = $LicenseKey

# CMake Build mit License Key
cmake -B build-licensed -S . `
    -G "Visual Studio 17 2022" -A x64 `
    -DTHEMIS_BUILD_ENTERPRISE=ON `
    -DTHEMIS_EMBEDDED_LICENSE="$LicenseKey" `
    -DCMAKE_BUILD_TYPE=Release

cmake --build build-licensed --config Release --target themis_server

# 2. Berechne SHA256 Hashes
$exePath = "build-licensed/bin/themis_server.exe"
$exeHash = (Get-FileHash -Path $exePath -Algorithm SHA256).Hash

Write-Host "Build Hash: $exeHash"

# 3. Berechne Hashes für kritische Module
$moduleHashes = @{}
$criticalModules = @(
    "build-licensed/bin/themis_core.dll",
    "build-licensed/bin/themis_enterprise.dll",
    "build-licensed/bin/themis_gpu.dll"
)

foreach ($module in $criticalModules) {
    if (Test-Path $module) {
        $moduleName = Split-Path $module -Leaf
        $moduleHash = (Get-FileHash -Path $module -Algorithm SHA256).Hash
        $moduleHashes[$moduleName] = $moduleHash
    }
}

$additionalHashes = $moduleHashes | ConvertTo-Json -Compress

# 4. Registriere Build im Pricing Server
$buildInfo = @{
    license_key = $LicenseKey
    build_hash = $exeHash
    build_timestamp = (Get-Date -Format "yyyy-MM-ddTHH:mm:ssZ")
    build_version = "1.3.0"
    build_platform = $Platform
    additional_hashes = $additionalHashes
} | ConvertTo-Json

$headers = @{
    "Content-Type" = "application/json"
    "X-API-Key" = $env:PRICING_SERVER_API_KEY
}

Invoke-RestMethod -Uri "https://service.themisdb.org:6734/admin/register-build" `
    -Method POST `
    -Headers $headers `
    -Body $buildInfo

Write-Host "Build registered successfully"

# 5. Packe Build für Auslieferung
$outputPath = "$OutputDir/$LicenseKey-$Platform-$(Get-Date -Format 'yyyyMMdd').zip"
Compress-Archive -Path "build-licensed/bin/*" -DestinationPath $outputPath

Write-Host "Licensed build created: $outputPath"
```

### 3. C++ Implementation - License Key Embedding

```cpp
// include/themis/license/embedded_license.hpp
#pragma once

#include <string>

namespace themis::license {

// Diese Konstante wird beim Build-Prozess ersetzt
// Durch CMake-Definition: -DTHEMIS_EMBEDDED_LICENSE="THEMIS-ENT-..."
#ifndef THEMIS_EMBEDDED_LICENSE
#define THEMIS_EMBEDDED_LICENSE "COMMUNITY"
#endif

// Konstante im Binary eingebettet (nicht im .data, sondern .rodata)
constexpr const char* EMBEDDED_LICENSE_KEY = THEMIS_EMBEDDED_LICENSE;

// Build-Timestamp einbetten
constexpr const char* BUILD_TIMESTAMP = __DATE__ " " __TIME__;

} // namespace themis::license
```

### 4. C++ Implementation - Self-Hash Calculation

```cpp
// include/themis/license/self_hash.hpp
#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <openssl/sha.h>
#include <fstream>

namespace themis::license {

class SelfHashCalculator {
public:
    // Berechnet SHA256 Hash der eigenen Executable
    static std::string calculateSelfHash() {
        std::filesystem::path exePath = getExecutablePath();
        return calculateFileHash(exePath);
    }
    
    // Berechnet Hashes aller kritischen Module
    static std::map<std::string, std::string> calculateModuleHashes() {
        std::map<std::string, std::string> hashes;
        
        std::filesystem::path exeDir = getExecutablePath().parent_path();
        
        // Kritische Module
        std::vector<std::string> criticalModules = {
            "themis_core" + getDllExtension(),
            "themis_enterprise" + getDllExtension(),
            "themis_gpu" + getDllExtension()
        };
        
        for (const auto& module : criticalModules) {
            auto modulePath = exeDir / module;
            if (std::filesystem::exists(modulePath)) {
                hashes[module] = calculateFileHash(modulePath);
            }
        }
        
        return hashes;
    }
    
private:
    static std::filesystem::path getExecutablePath() {
#ifdef _WIN32
        char buffer[MAX_PATH];
        GetModuleFileNameA(NULL, buffer, MAX_PATH);
        return std::filesystem::path(buffer);
#else
        char buffer[PATH_MAX];
        ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
        if (len != -1) {
            buffer[len] = '\0';
            return std::filesystem::path(buffer);
        }
        return "";
#endif
    }
    
    static std::string calculateFileHash(const std::filesystem::path& filePath) {
        std::ifstream file(filePath, std::ios::binary);
        if (!file) {
            throw std::runtime_error("Cannot open file for hashing: " + filePath.string());
        }
        
        SHA256_CTX sha256;
        SHA256_Init(&sha256);
        
        constexpr size_t BUFFER_SIZE = 8192;
        char buffer[BUFFER_SIZE];
        
        while (file.read(buffer, BUFFER_SIZE) || file.gcount() > 0) {
            SHA256_Update(&sha256, buffer, file.gcount());
        }
        
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256_Final(hash, &sha256);
        
        // Convert to hex string
        std::stringstream ss;
        for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
            ss << std::hex << std::setw(2) << std::setfill('0') 
               << static_cast<int>(hash[i]);
        }
        
        return ss.str();
    }
    
    static std::string getDllExtension() {
#ifdef _WIN32
        return ".dll";
#elif __APPLE__
        return ".dylib";
#else
        return ".so";
#endif
    }
};

} // namespace themis::license
```

### 5. Erweiterte License Validation (mit Hash-Prüfung)

```cpp
// src/license/license_validator.cpp (erweitert)

LicenseValidationResult LicenseValidator::validateWithHash(
    const std::string& license_key
) {
    try {
        // Berechne eigenen Hash
        std::string selfHash = SelfHashCalculator::calculateSelfHash();
        auto moduleHashes = SelfHashCalculator::calculateModuleHashes();
        
        spdlog::info("Self-hash calculated: {}", selfHash);
        
        // Erstelle JSON mit Hashes
        nlohmann::json additionalHashes;
        for (const auto& [module, hash] : moduleHashes) {
            additionalHashes[module] = hash;
        }
        
        // Sende an Server
        httplib::Client client(validation_url_);
        
        nlohmann::json request_body = {
            {"license_key", license_key},
            {"build_hash", selfHash},
            {"additional_hashes", additionalHashes},
            {"server_hostname", getHostname()},
            {"server_version", THEMIS_VERSION}
        };
        
        auto res = client.Post("/license/validate-build",
                              request_body.dump(),
                              "application/json");
        
        if (!res) {
            spdlog::error("Cannot connect to license server");
            return {false, "connection_error", "Cannot validate build", "", std::nullopt};
        }
        
        auto response = nlohmann::json::parse(res->body);
        
        if (!response["valid"]) {
            std::string status = response["status"];
            std::string message = response["message"];
            
            if (status == "hash_mismatch") {
                spdlog::error("BUILD HASH MISMATCH!");
                spdlog::error("This binary has been modified or is not licensed for this key!");
                spdlog::error("Expected hash: {}", response.value("expected_hash", "unknown"));
                spdlog::error("Actual hash: {}", selfHash);
            }
            
            return {false, status, message, "", std::nullopt};
        }
        
        // Erfolgreich validiert
        LicenseValidationResult result;
        result.valid = true;
        result.status = "active";
        result.message = response["message"];
        result.tier = response["tier"];
        
        // ... populate limits ...
        
        return result;
        
    } catch (const std::exception& e) {
        spdlog::error("License validation error: {}", e.what());
        return {false, "error", e.what(), "", std::nullopt};
    }
}
```

---

## Vorteile

✅ **Unbefugte Nutzung verhindert**: Binary funktioniert nur mit dem spezifischen License Key  
✅ **Manipulationsschutz**: Änderungen am Binary werden erkannt (Hash ändert sich)  
✅ **Audit Trail**: Jeder Build ist nachvollziehbar (wann, für wen, welche Version)  
✅ **Keine Umgehung möglich**: License Key ist im Binary, aber Binary ist an Key gebunden  
✅ **Support vereinfacht**: Eindeutige Zuordnung Build ↔ Kunde ↔ Lizenz  

## Nachteile / Considerations

⚠️ **Build-Overhead**: Für jeden Kunden muss ein eigener Build erstellt werden  
⚠️ **Updates komplexer**: Bei Updates muss neuer Hash registriert werden  
⚠️ **Storage**: Mehr Binaries zum Speichern (pro Kunde ein Build)  
⚠️ **Debug-Builds**: Entwicklungs-Builds benötigen spezielle Handling  

## Mitigation Strategies

**Problem: Viele Custom Builds**  
**Lösung**: Automatisiertes Build-System (CI/CD), Builds on-demand

**Problem: Updates**  
**Lösung**: Auto-Update Mechanismus mit Hash-Registrierung

**Problem: Debug/Testing**  
**Lösung**: Spezielle Test-License Keys mit Hash-Whitelist

---

## Deployment Flow

```
1. Kunde kauft Lizenz über Portal
        ↓
2. Pricing Server generiert License Key
        ↓
3. Build-System wird getriggert (GitHub Actions / Jenkins)
        ↓
4. Custom Build mit eingebettetem Key wird erstellt
        ↓
5. Hash wird automatisch beim Pricing Server registriert
        ↓
6. Build wird per E-Mail/Download bereitgestellt
        ↓
7. Kunde installiert → Server validiert beim Start
        ↓
8. Bei Updates: Neuer Build + neue Hash-Registrierung
```

---

## Fazit

**Realistisch möglich?** ✅ **JA**

**Aufwand:**
- **Initial**: Mittel (2-3 Entwicklertage für vollständige Integration)
- **Laufend**: Gering (automatisiert via CI/CD)

**Empfehlung**: 
Für Enterprise/Hyperscaler-Editions definitiv sinnvoll und machbar. Der Sicherheitsgewinn rechtfertigt den Aufwand.
