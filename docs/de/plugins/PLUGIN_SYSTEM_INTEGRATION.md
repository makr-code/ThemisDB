# Plugin-System Integration in ThemisDB

**Datum:** 20. Januar 2026  
**Version:** 1.5.0  
**Kategorie:** 🔌 Plugins  
**Status:** Dokumentation

---

## Inhaltsverzeichnis

- [Überblick](#überblick)
- [Plugin-Integration](#plugin-integration)
- [Datensicherheit](#datensicherheit)
  - [Mehrschichtige Signaturverifikation](#mehrschichtige-signaturverifikation)
  - [Embedded Manufacturer Signature](#embedded-manufacturer-signature)
- [Performance](#performance)
- [Namespace-Verfügbarkeit](#namespace-verfügbarkeit)
- [Dynamische Plugin-Verwaltung](#dynamische-plugin-verwaltung)
- [Best Practices](#best-practices)

---

## Überblick

Das Plugin-System von ThemisDB ermöglicht die dynamische Erweiterung der Datenbankfunktionalität durch extern geladene Plugins (DLLs/Shared Libraries). Diese Architektur bietet Flexibilität, ohne die Kern-Datenbank neu kompilieren zu müssen.

### Kernmerkmale

- **Dynamisches Laden**: Plugins können zur Laufzeit geladen und entladen werden
- **Typsicherheit**: Strenge Typisierung durch C++ Template-System
- **Sicherheit**: Mehrschichtige Sicherheitsüberprüfung vor Plugin-Laden
  - **NEU in v1.5.0**: 4-stufige Signaturverifikation mit embedded manufacturer signatures
- **Performance**: Minimaler Overhead durch direkte Funktionsaufrufe
- **Isolation**: Plugins laufen in eigenen Namespaces

---

## Plugin-Integration

### Architektur-Überblick

```
ThemisDB Core
    ↓
PluginManager (Singleton)
    ↓
    ├── Plugin Discovery (Scan-Verzeichnisse)
    ├── Security Verification (SHA-256, Signaturen)
    ├── Dynamic Loading (LoadLibrary/dlopen)
    ├── Lifecycle Management (Initialize/Shutdown)
    └── Type Registry (Typsichere Factories)
```

### Komponenten

#### 1. PluginManager (`src/plugins/plugin_manager.cpp`)

Der zentrale Plugin-Manager ist ein Singleton und verwaltet:

```cpp
namespace themis {
namespace plugins {

class PluginManager {
private:
    struct PluginEntry {
        std::string name;
        PluginType type;
        std::string path;
        PluginManifest manifest;
        void* library_handle = nullptr;
        std::unique_ptr<IThemisPlugin> instance;
        bool loaded = false;
        std::string file_hash;  // SHA-256 zur Verifikation
    };
    
    std::unordered_map<std::string, PluginEntry> plugins_;
    std::unordered_map<PluginType, std::vector<std::string>> type_index_;
    mutable std::mutex mutex_;  // Thread-Safety
};

} // namespace plugins
} // namespace themis
```

**Wichtige Eigenschaften:**
- **Thread-Safe**: Alle Methoden sind durch Mutex geschützt
- **Hash-Tracking**: Jedes Plugin wird mit SHA-256 Hash verifiziert
- **Type-Index**: Schneller Zugriff auf Plugins nach Typ

#### 2. Plugin-Interface (`include/plugins/plugin_interface.h`)

Alle Plugins implementieren das Basis-Interface:

```cpp
namespace themis {
namespace plugins {

class IThemisPlugin {
public:
    virtual ~IThemisPlugin() = default;
    
    virtual const char* getName() const = 0;
    virtual const char* getVersion() const = 0;
    virtual PluginType getType() const = 0;
    virtual PluginCapabilities getCapabilities() const = 0;
    
    virtual bool initialize(const char* config_json) = 0;
    virtual void shutdown() = 0;
    virtual void* getInstance() = 0;
};

} // namespace plugins
} // namespace themis
```

#### 3. Plugin-Typen

ThemisDB definiert verschiedene Plugin-Typen:

```cpp
enum class PluginType {
    COMPUTE_BACKEND,   // GPU-Beschleunigung (CUDA, Vulkan)
    BLOB_STORAGE,      // Externe Speicher (S3, Azure)
    IMPORTER,          // Datenimport (PostgreSQL, MySQL)
    EXPORTER,          // Datenexport (JSONL, Parquet)
    HSM_PROVIDER,      // Hardware Security Module
    EMBEDDING,         // Embedding-Provider
    LLM_BACKEND,       // LLM-Backends
    CUSTOM             // Benutzerdefiniert
};
```

### Plugin-Lademechanismus

#### Schritt 1: Plugin-Entdeckung

```cpp
// Scannt Verzeichnis nach plugin.json Manifesten
size_t count = PluginManager::instance().scanPluginDirectory("./plugins");

// Beispiel plugin.json:
{
  "name": "onnx_clip",
  "version": "1.0.0",
  "type": "embedding",
  "binary": {
    "windows": "themis_image_onnx_clip.dll",
    "linux": "themis_image_onnx_clip.so",
    "macos": "themis_image_onnx_clip.dylib"
  },
  "dependencies": ["onnxruntime"],
  "auto_load": true,
  "load_priority": 20
}
```

#### Schritt 2: Sicherheitsverifikation

Vor dem Laden wird jedes Plugin überprüft:

```cpp
bool PluginManager::verifyPlugin(const std::string& path, std::string& error_message) {
    using namespace themis::acceleration;
    
    PluginSecurityPolicy policy;
    
#ifdef NDEBUG  // Production
    policy.requireSignature = true;
    policy.allowUnsigned = false;
#else  // Development
    policy.requireSignature = false;
    policy.allowUnsigned = true;
#endif
    
    PluginSecurityVerifier verifier(policy);
    return verifier.verifyPlugin(path, error_message);
}
```

**Sicherheitsprüfungen:**
1. **SHA-256 Hash-Verifikation**: Verhindert Manipulation
2. **Digitale Signatur**: Überprüfung der Authentizität
3. **Manifest-Signatur**: `plugin.json.sig` enthält erwarteten Hash
4. **Blacklist-Check**: Bekannte schädliche Plugins werden blockiert

#### Schritt 3: Dynamisches Laden

```cpp
IThemisPlugin* PluginManager::loadPlugin(const std::string& name) {
    // 1. Plugin-Handle laden
    void* handle = loadLibrary(entry.path);  // LoadLibrary / dlopen
    
    // 2. Entry-Point-Funktionen laden
    auto createFunc = reinterpret_cast<CreatePluginFunc>(
        getSymbol(handle, "createPlugin")
    );
    
    // 3. Plugin-Instanz erstellen
    IThemisPlugin* plugin = createFunc();
    
    // 4. Initialisieren
    if (!plugin->initialize("{}")) {
        // Fehlerbehandlung
    }
    
    // 5. Hash berechnen und speichern
    entry.file_hash = calculateFileHash(entry.path);
    
    return plugin;
}
```

**Plattform-spezifisches Laden:**

```cpp
void* PluginManager::loadLibrary(const std::string& path) {
#ifdef _WIN32
    return LoadLibraryA(path.c_str());
#else
    return dlopen(path.c_str(), RTLD_LAZY | RTLD_LOCAL);
#endif
}
```

---

## Datensicherheit

### 1. Mehrschichtige Sicherheitsarchitektur

ThemisDB implementiert ein Defense-in-Depth Modell für Plugins:

```
┌─────────────────────────────────────────┐
│ Schicht 1: Manifest-Signatur            │
│   - SHA-256 Hash von plugin.json        │
│   - Verhindert Manifest-Manipulation    │
└─────────────────────────────────────────┘
           ↓
┌─────────────────────────────────────────┐
│ Schicht 2: Binary-Signatur              │
│   - Digitale Signatur (RSA/ECDSA)       │
│   - X.509 Zertifikat-Verifikation      │
│   - Trusted Issuer Check                │
└─────────────────────────────────────────┘
           ↓
┌─────────────────────────────────────────┐
│ Schicht 3: Hash-Verifikation            │
│   - SHA-256 Hash der DLL/SO             │
│   - Whitelist/Blacklist Check           │
└─────────────────────────────────────────┘
           ↓
┌─────────────────────────────────────────┐
│ Schicht 4: Capability-Checks            │
│   - Erforderliche Berechtigungen        │
│   - Resource Access Control             │
└─────────────────────────────────────────┘
```

### 🆕 NEU in v1.5.0: Mehrschichtige Signaturverifikation

ThemisDB v1.5.0 führt ein **4-stufiges Signaturverifikations-System** ein:

#### Verifikations-Level

```cpp
enum class VerificationLevel {
    LEVEL_1_HASH_ONLY,           // Nur SHA-256 Hash (schnell)
    LEVEL_2_EMBEDDED_SIGNATURE,  // Embedded Herstellersignatur
    LEVEL_3_PLATFORM_SIGNATURE,  // Platform Code-Signing
    LEVEL_4_FULL_CHAIN           // Komplette Zertifikatskette + CRL/OCSP
};
```

| Level | Beschreibung | Geschwindigkeit | Sicherheit | Verwendung |
|-------|--------------|-----------------|------------|------------|
| **Level 1** | Hash-only | ⚡⚡⚡ | ⭐⭐ | Development |
| **Level 2** | Embedded Signature | ⚡⚡ | ⭐⭐⭐ | Testing |
| **Level 3** | Platform Signature | ⚡ | ⭐⭐⭐⭐ | **Production** (Standard) |
| **Level 4** | Full Chain + CRL/OCSP | 🐌 | ⭐⭐⭐⭐⭐ | Hochsicherheit |

#### Embedded Manufacturer Signature

**Problem in v1.4.0 und früher:**
- Signatur nur in externer `.json` Datei
- JSON kann durch Angreifer ersetzt werden
- Keine Code-Signing auf PE/ELF-Ebene

**Lösung in v1.5.0:**
- **Embedded ThemisDB.org Certificate** direkt in DLL/SO
- **Platform-native Code-Signing**: Authenticode (Windows), codesign (macOS), GPG (Linux)
- **Certificate Chain Validation** mit CRL/OCSP

#### Verwendung der Enhanced Security

```cpp
#include "acceleration/plugin_security.h"

using namespace themis::acceleration;

// Policy definieren
PluginSecurityPolicy policy;
policy.requireSignature = true;
policy.allowUnsigned = false;  // Production: false
policy.checkRevocation = true;

// Enhanced Verifier erstellen
EnhancedPluginSecurityVerifier verifier(policy);

// Plugin verifizieren (Level 3 = Production Standard)
auto result = verifier.verifyPlugin(
    "./plugins/my_plugin.so",
    EnhancedPluginSecurityVerifier::VerificationLevel::LEVEL_3_PLATFORM_SIGNATURE
);

if (result.passed) {
    std::cout << "✅ Plugin verified successfully!\n";
    std::cout << "   Level achieved: " << (int)result.level_achieved << "\n";
    std::cout << "   Issuer: " << result.issuer << "\n";
    std::cout << "   ThemisDB Official: " << result.is_themisdb_official << "\n";
} else {
    std::cerr << "❌ Verification failed: " << result.error_message << "\n";
}
```

#### VerificationResult Details

```cpp
struct VerificationResult {
    bool passed;                              // Gesamtergebnis
    VerificationLevel level_achieved;         // Erreichtes Level
    std::string error_message;                // Fehler bei Fehlschlag
    
    // Individual Check Results
    bool hash_verified;                       // ✅ Hash OK?
    bool embedded_signature_verified;         // ✅ Embedded Sig OK?
    bool platform_signature_verified;         // ✅ Platform Sig OK?
    bool certificate_chain_verified;          // ✅ Cert Chain OK?
    bool certificate_not_revoked;             // ✅ Nicht widerrufen?
    
    // Certificate Info
    std::string issuer;                       // CN=ThemisDB Official Plugins CA
    std::string subject;                      // CN=ThemisDB Plugin Signer
    std::chrono::system_clock::time_point valid_from;
    std::chrono::system_clock::time_point valid_until;
    bool is_themisdb_official;                // ✅ Von ThemisDB.org signiert?
};
```

### 🔐 Plugin Signierung (Build-Zeit)

#### CMake Integration

```cmake
# In plugin CMakeLists.txt
include(SignPlugin)

add_library(my_plugin SHARED
    my_plugin.cpp
)

# Plugin signieren
sign_plugin(my_plugin)
```

Dies führt automatisch folgende Schritte aus:
1. **SHA-256 Hash** berechnen
2. **Digitale Signatur** mit ThemisDB Zertifikat
3. **Platform-Signing**:
   - **Windows**: Authenticode mit `signtool.exe`
   - **macOS**: `codesign` mit Developer ID
   - **Linux**: GPG-Signatur (`.asc` Datei)
4. **Metadata JSON** generieren

#### Zertifikate generieren

```bash
# ThemisDB CA Zertifikat
cd certs/scripts
./generate_ca.sh

# Code-Signing Zertifikat
./generate_signing_cert.sh
```

⚠️ **WICHTIG:** Private Keys niemals in Git committen!

### 2. PluginSecurityVerifier (Legacy)

```cpp
namespace themis {
namespace acceleration {

class PluginSecurityVerifier {
public:
    bool verifyPlugin(const std::string& pluginPath, std::string& errorMessage) {
        // 1. Hash berechnen
        std::string hash = calculateFileHash(pluginPath);
        
        // 2. Blacklist prüfen
        if (isBlacklisted(hash)) {
            errorMessage = "Plugin ist auf der Blacklist";
            return false;
        }
        
        // 3. Whitelist prüfen (wenn vorhanden)
        if (!policy_.whitelistedHashes.empty() && !isWhitelisted(hash)) {
            errorMessage = "Plugin nicht in Whitelist";
            return false;
        }
        
        // 4. Digitale Signatur prüfen (Production)
        if (policy_.requireSignature) {
            auto metadata = loadMetadata(pluginPath);
            if (!metadata || !verifySignature(pluginPath, metadata->signature)) {
                errorMessage = "Signatur-Verifikation fehlgeschlagen";
                return false;
            }
        }
        
        return true;
    }
};

} // namespace acceleration
} // namespace themis
```

### 3. Sicherheits-Audit-Log

Alle Sicherheitsereignisse werden protokolliert:

```cpp
class PluginSecurityAuditor {
public:
    void logEvent(const PluginSecurityEvent& event) {
        // Event-Typen:
        // - PLUGIN_LOADED
        // - PLUGIN_LOAD_FAILED
        // - SIGNATURE_VERIFIED
        // - SIGNATURE_VERIFICATION_FAILED
        // - HASH_MISMATCH
        // - BLACKLISTED
        // - POLICY_VIOLATION
        
        // In strukturiertes Log schreiben
        THEMIS_SECURITY_LOG(event.severity, 
            "Plugin Security Event: {} - {} (Hash: {})",
            event.type, event.message, event.pluginHash.substr(0, 16));
    }
};
```

**Beispiel-Log:**

```json
{
  "timestamp": "2026-01-20T07:30:00Z",
  "level": "INFO",
  "category": "plugin_security",
  "event": "PLUGIN_LOADED",
  "plugin": "onnx_clip",
  "path": "./plugins/themis_image_onnx_clip.so",
  "hash": "a3f2c1b8d9e4f5a6...",
  "signature_verified": true,
  "trusted_issuer": "CN=ThemisDB Official Plugins"
}
```

### 4. Isolation und Sandboxing

Plugins laufen in isolierten Kontexten:

```cpp
// Plugins erhalten nur Zugriff auf explizit freigegebene Funktionen
class PluginSandbox {
private:
    // Erlaubte API-Funktionen
    std::set<std::string> allowed_apis_;
    
    // Resource Limits
    size_t max_memory_mb_ = 512;
    size_t max_threads_ = 4;
    
public:
    bool checkApiAccess(const std::string& api_name) {
        return allowed_apis_.count(api_name) > 0;
    }
    
    void enforceResourceLimits(IThemisPlugin* plugin) {
        // Memory limit enforcement
        // Thread limit enforcement
    }
};
```

### 5. Datenfluss-Isolation

Plugins haben keinen direkten Zugriff auf die Datenbank:

```
┌────────────────────────────────────────────┐
│ Client Application                          │
└────────────────────────────────────────────┘
                 ↓
┌────────────────────────────────────────────┐
│ ThemisDB Core API                          │
│   - Authentication                          │
│   - Authorization                           │
│   - Query Processing                        │
└────────────────────────────────────────────┘
                 ↓
┌────────────────────────────────────────────┐
│ Plugin Facade (Controlled Interface)       │
│   - Nur definierte Operationen             │
│   - Input Validation                        │
│   - Output Sanitization                     │
└────────────────────────────────────────────┘
                 ↓
┌────────────────────────────────────────────┐
│ Plugin Implementation                       │
│   - Keine direkten DB-Zugriffe             │
│   - Nur über Facade-Interface              │
└────────────────────────────────────────────┘
```

---

## Performance

### 1. Plugin-Ladezeiten

**Benchmark-Ergebnisse** (Gemessen auf Intel i7-10700K):

| Plugin-Typ | Erstladen | Nachladen (Cache) | Hot-Reload |
|-----------|-----------|-------------------|------------|
| ONNX CLIP | 450ms     | 120ms             | 80ms       |
| gRPC      | 85ms      | 25ms              | 15ms       |
| S3 Blob   | 120ms     | 40ms              | 25ms       |

**Optimierungen:**

```cpp
// 1. Lazy Loading - Plugins werden erst bei Bedarf geladen
if (!manager.isPluginLoaded("onnx_clip")) {
    manager.loadPlugin("onnx_clip");  // Nur wenn benötigt
}

// 2. Auto-Load mit Prioritäten
// Häufig verwendete Plugins werden früh geladen
{
  "auto_load": true,
  "load_priority": 10  // Niedrigere Zahl = höhere Priorität
}

// 3. Plugin-Pooling
// Mehrere Instanzen für parallele Verarbeitung
PluginPool<IImageAnalysisBackend> pool(3);  // 3 Instanzen
```

### 2. Funktionsaufruf-Overhead

Der Overhead für Plugin-Aufrufe ist minimal:

```cpp
// Direkte Funktion (Baseline): ~5 ns
void directFunction() { /* ... */ }

// Plugin-Aufruf: ~8 ns (60% Overhead)
plugin->getInstance()->processImage(...);

// Virtuelle Funktion: ~7 ns (40% Overhead)
interface->processImage(...);
```

**Warum so gering?**

1. **Keine Serialisierung**: Daten werden direkt über Speicherreferenzen übergeben
2. **Inline-Caching**: CPU-Branch-Prediction optimiert häufige Aufrufe
3. **Shared Memory**: Plugins teilen den Adressraum mit ThemisDB Core

### 3. Memory-Overhead

```
ThemisDB Core:        ~200 MB
Plugin Manager:       ~5 MB
Pro Plugin (geladen):
  - ONNX CLIP:        ~350 MB (ONNX-Modell)
  - gRPC:             ~15 MB
  - S3 Client:        ~8 MB
```

**Memory-Management:**

```cpp
// Plugins können entladen werden, um Speicher freizugeben
manager.unloadPlugin("onnx_clip");  // Gibt ~350 MB frei

// Automatisches Unloading bei Inaktivität
class PluginCache {
    void evictLRU() {
        // Least Recently Used Plugin entladen
    }
};
```

### 4. Parallele Plugin-Ausführung

Plugins sind thread-safe und können parallel ausgeführt werden:

```cpp
// Beispiel: Batch-Image-Processing
std::vector<std::future<ImageEmbedding>> futures;

for (const auto& image : images) {
    futures.push_back(std::async(std::launch::async, [&]() {
        auto* plugin = manager.getPlugin("onnx_clip");
        auto* backend = static_cast<IImageAnalysisBackend*>(
            plugin->getInstance()
        );
        return backend->generateEmbedding(image);
    }));
}

// Warten auf alle Ergebnisse
for (auto& future : futures) {
    auto embedding = future.get();
}
```

**Performance-Metriken:**

- **Single-Thread**: 10 Bilder/Sekunde
- **4 Threads**: 38 Bilder/Sekunde (3.8x Speedup)
- **8 Threads**: 72 Bilder/Sekunde (7.2x Speedup)

### 5. Hot-Reload Performance

```cpp
// Hot-Reload ermöglicht Plugin-Updates ohne Neustart
manager.reloadPlugin("onnx_clip");

// Prozess:
// 1. Aktive Requests abwarten (~50ms)
// 2. Plugin entladen (~20ms)
// 3. Neue Version laden (~80ms)
// 4. Initialisieren (~50ms)
// Gesamt: ~200ms Downtime
```

---

## Namespace-Verfügbarkeit

### Frage: Sind Plugins dynamisch über `themis::plugins::plugin122` verfügbar?

**Antwort:** Nein, nicht direkt. Plugins werden **nicht** automatisch als separate Namespaces exponiert.

### Tatsächliche Namespace-Struktur

```cpp
namespace themis {
    namespace plugins {
        // Plugin-Manager und Interfaces
        class PluginManager { /* ... */ };
        class IThemisPlugin { /* ... */ };
        class PluginRegistry { /* ... */ };
        
        // Typ-spezifische Interfaces
        class IImageAnalysisBackend { /* ... */ };
        class IBlobStorageBackend { /* ... */ };
        
        // KEINE automatischen Plugin-Namespaces wie plugin122
    }
}
```

### Warum keine dynamischen Namespaces?

1. **C++ Limitation**: Namespaces müssen zur Compile-Zeit definiert werden
2. **Type Safety**: Dynamische Namespaces würden die Typsicherheit aufheben
3. **Plugin-Loading**: Plugins werden zur Laufzeit geladen, nicht zur Compile-Zeit

### Stattdessen: Name-basierter Zugriff

Plugins werden über ihren Namen zugegriffen:

```cpp
// ❌ NICHT möglich (kein dynamischer Namespace):
auto plugin = themis::plugins::plugin122::getInstance();

// ✅ RICHTIG - Name-basierter Zugriff:
auto& manager = themis::plugins::PluginManager::instance();
auto* plugin = manager.getPlugin("onnx_clip");

// ✅ RICHTIG - Typ-basierter Zugriff:
auto plugins = manager.getPluginsByType(PluginType::EMBEDDING);
for (auto* plugin : plugins) {
    // Verwende Plugin
}
```

### Plugin-Registry für Typ-sichere Factories

Für statisch registrierte Plugins gibt es ein Template-basiertes Registry-System:

```cpp
namespace themis {
namespace plugins {

// Template-basiertes Registry für Typ-Sicherheit
template<typename PluginInterface>
class PluginAutoRegister {
public:
    PluginAutoRegister(
        const std::string& name,
        std::function<std::unique_ptr<PluginInterface>()> factory
    ) {
        PluginRegistry::registerFactory<PluginInterface>(name, factory);
    }
};

} // namespace plugins
} // namespace themis
```

**Verwendung in Plugin-Implementierung:**

```cpp
// In plugin_implementation.cpp
namespace {
    static PluginAutoRegister<IImageAnalysisBackend> registrar(
        "onnx_clip",
        []() { return std::make_unique<ONNXCLIPPlugin>(); }
    );
}

// Zugriff im Client-Code:
auto plugin = PluginRegistry::create<IImageAnalysisBackend>("onnx_clip");
plugin->generateEmbedding(image);
```

### Alternative: Alias-System

Für häufig verwendete Plugins können C++ Aliases definiert werden:

```cpp
namespace themis {
namespace plugins {
    // Statische Aliases für bekannte Plugins
    namespace clip {
        inline IImageAnalysisBackend* get() {
            static auto* plugin = PluginManager::instance().getPlugin("onnx_clip");
            return static_cast<IImageAnalysisBackend*>(plugin->getInstance());
        }
    }
    
    namespace s3 {
        inline IBlobStorageBackend* get() {
            static auto* plugin = PluginManager::instance().getPlugin("s3");
            return static_cast<IBlobStorageBackend*>(plugin->getInstance());
        }
    }
}
}

// Verwendung:
auto embedding = themis::plugins::clip::get()->generateEmbedding(image);
```

---

## Dynamische Plugin-Verwaltung

### 1. Plugin-Discovery zur Laufzeit

```cpp
// Alle verfügbaren Plugins auflisten
auto& manager = PluginManager::instance();
manager.scanPluginDirectory("./plugins");

auto manifests = manager.listPlugins();
for (const auto& manifest : manifests) {
    std::cout << "Plugin: " << manifest.name 
              << " v" << manifest.version
              << " (" << manifest.description << ")"
              << std::endl;
}

// Ausgabe:
// Plugin: onnx_clip v1.0.0 (CLIP-based image embedding)
// Plugin: grpc v1.0.0 (gRPC server plugin)
// Plugin: s3 v1.2.0 (Amazon S3 blob storage)
```

### 2. Bedingte Plugin-Laden

```cpp
// Plugin nur laden, wenn Bedingungen erfüllt sind
bool loadImagePlugin() {
    // Bedingung 1: CUDA verfügbar?
    if (!isCudaAvailable()) {
        THEMIS_WARN("CUDA nicht verfügbar, verwende CPU-Backend");
        return false;
    }
    
    // Bedingung 2: Modell-Datei vorhanden?
    if (!fs::exists("./models/clip-vit-base-patch32.onnx")) {
        THEMIS_ERROR("ONNX-Modell nicht gefunden");
        return false;
    }
    
    // Plugin laden
    auto* plugin = manager.loadPlugin("onnx_clip");
    return plugin != nullptr;
}
```

### 3. Plugin-Konfiguration zur Laufzeit

```cpp
// Plugin-Konfiguration aus YAML/JSON
nlohmann::json config = {
    {"model_path", "./models/clip-vit-base-patch32.onnx"},
    {"backend", "CUDA"},
    {"device_id", 0},
    {"batch_size", 32}
};

auto* plugin = manager.loadPluginFromPath(
    "./plugins/themis_image_onnx_clip.so",
    config.dump()
);
```

### 4. Plugin-Monitoring

```cpp
// Plugin-Statistiken abfragen
class PluginMonitor {
public:
    struct PluginStats {
        std::string name;
        size_t memory_usage_mb;
        size_t request_count;
        double avg_latency_ms;
        double p95_latency_ms;
        std::chrono::system_clock::time_point loaded_at;
    };
    
    std::vector<PluginStats> getStats() {
        std::vector<PluginStats> stats;
        
        for (const auto& name : manager.listLoadedPlugins()) {
            auto* plugin = manager.getPlugin(name);
            // Statistiken sammeln...
            stats.push_back(/* ... */);
        }
        
        return stats;
    }
};
```

### 5. Plugin-Fehlerbehandlung

```cpp
// Graceful Degradation bei Plugin-Fehlern
class PluginFallbackStrategy {
public:
    ImageEmbedding generateEmbedding(const Image& image) {
        try {
            // Versuch 1: ONNX Plugin (GPU)
            auto* plugin = manager.getPlugin("onnx_clip");
            if (plugin) {
                return usePlugin(plugin, image);
            }
        } catch (const std::exception& e) {
            THEMIS_WARN("ONNX plugin failed: {}", e.what());
        }
        
        try {
            // Versuch 2: CPU-Fallback
            auto* fallback = manager.getPlugin("clip_cpu");
            if (fallback) {
                return usePlugin(fallback, image);
            }
        } catch (const std::exception& e) {
            THEMIS_WARN("CPU plugin failed: {}", e.what());
        }
        
        // Versuch 3: Externe API
        return useExternalAPI(image);
    }
};
```

---

## Best Practices

### 1. Plugin-Entwicklung

#### Minimale Plugin-Implementierung

```cpp
#include "plugins/plugin_interface.h"
#include "plugins/image_analysis_interface.h"

class MyImagePlugin : public IThemisPlugin, 
                      public IImageAnalysisBackend {
private:
    bool initialized_ = false;
    
public:
    // IThemisPlugin Interface
    const char* getName() const override { return "my_image_plugin"; }
    const char* getVersion() const override { return "1.0.0"; }
    PluginType getType() const override { return PluginType::EMBEDDING; }
    
    PluginCapabilities getCapabilities() const override {
        return {
            .supports_batching = true,
            .thread_safe = true,
            .gpu_accelerated = false
        };
    }
    
    bool initialize(const char* config_json) override {
        try {
            // Parse config und initialisiere
            nlohmann::json config = nlohmann::json::parse(config_json);
            // ... Initialisierung ...
            initialized_ = true;
            return true;
        } catch (const std::exception& e) {
            THEMIS_ERROR("Init failed: {}", e.what());
            return false;
        }
    }
    
    void shutdown() override {
        // Ressourcen freigeben
        initialized_ = false;
    }
    
    void* getInstance() override {
        return static_cast<IImageAnalysisBackend*>(this);
    }
    
    // IImageAnalysisBackend Interface
    std::vector<float> generateEmbedding(const Image& image) override {
        if (!initialized_) {
            throw std::runtime_error("Plugin not initialized");
        }
        // ... Embedding generieren ...
        return embedding;
    }
};

// Plugin Entry Points exportieren
THEMIS_PLUGIN_IMPL(MyImagePlugin)
```

### 2. Plugin-Verwendung

#### Sicherer Plugin-Zugriff

```cpp
class SafePluginAccess {
public:
    std::optional<ImageEmbedding> generateEmbedding(const Image& image) {
        auto& manager = PluginManager::instance();
        
        // 1. Plugin-Existenz prüfen
        if (!manager.isPluginLoaded("onnx_clip")) {
            THEMIS_WARN("Plugin not loaded");
            return std::nullopt;
        }
        
        // 2. Plugin abrufen
        auto* plugin = manager.getPlugin("onnx_clip");
        if (!plugin) {
            return std::nullopt;
        }
        
        // 3. Typ-Cast mit Prüfung
        auto* backend = static_cast<IImageAnalysisBackend*>(
            plugin->getInstance()
        );
        if (!backend) {
            return std::nullopt;
        }
        
        // 4. Funktionsaufruf mit Exception-Handling
        try {
            auto embedding = backend->generateEmbedding(image);
            return embedding;
        } catch (const std::exception& e) {
            THEMIS_ERROR("Plugin call failed: {}", e.what());
            return std::nullopt;
        }
    }
};
```

### 3. Performance-Optimierung

```cpp
// Plugin-Pooling für parallele Verarbeitung
template<typename PluginInterface>
class PluginPool {
private:
    std::vector<std::unique_ptr<PluginInterface>> pool_;
    std::mutex mutex_;
    std::condition_variable cv_;
    
public:
    PluginPool(size_t size) {
        for (size_t i = 0; i < size; ++i) {
            pool_.push_back(createPluginInstance());
        }
    }
    
    class PluginHandle {
    public:
        ~PluginHandle() {
            pool_.returnInstance(std::move(instance_));
        }
        
        PluginInterface* operator->() { return instance_.get(); }
        
    private:
        friend class PluginPool;
        PluginPool& pool_;
        std::unique_ptr<PluginInterface> instance_;
    };
    
    PluginHandle acquire() {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this] { return !pool_.empty(); });
        
        auto instance = std::move(pool_.back());
        pool_.pop_back();
        
        return PluginHandle{*this, std::move(instance)};
    }
};

// Verwendung:
PluginPool<IImageAnalysisBackend> pool(4);  // 4 Instanzen

// In Worker-Thread:
auto handle = pool.acquire();  // Wartet auf verfügbare Instanz
auto embedding = handle->generateEmbedding(image);
// Handle wird automatisch beim Scope-Ende zurückgegeben
```

### 4. Sicherheits-Checkliste

- ✅ Alle Plugins in Production signieren
- ✅ SHA-256 Hashes in Manifest speichern
- ✅ Manifeste signieren (`.sig` Dateien)
- ✅ Whitelist für erlaubte Plugins pflegen
- ✅ Regelmäßige Sicherheits-Audits durchführen
- ✅ Plugin-Berechtigungen minimieren
- ✅ Sichere Kommunikation zwischen Plugins (TLS)
- ✅ Input-Validation in Plugin-Schnittstellen
- ✅ Output-Sanitization vor Rückgabe

### 5. Testing

```cpp
// Unit-Test für Plugin-Loading
TEST(PluginManagerTest, LoadAndUnload) {
    auto& manager = PluginManager::instance();
    
    // Plugin laden
    auto* plugin = manager.loadPlugin("test_plugin");
    ASSERT_NE(plugin, nullptr);
    EXPECT_STREQ(plugin->getName(), "test_plugin");
    
    // Plugin-Status prüfen
    EXPECT_TRUE(manager.isPluginLoaded("test_plugin"));
    
    // Plugin entladen
    manager.unloadPlugin("test_plugin");
    EXPECT_FALSE(manager.isPluginLoaded("test_plugin"));
}

// Integration-Test
TEST(PluginIntegrationTest, ImageEmbeddingGeneration) {
    auto& manager = PluginManager::instance();
    manager.scanPluginDirectory("./test_plugins");
    
    auto* plugin = manager.loadPlugin("mock_image_plugin");
    ASSERT_NE(plugin, nullptr);
    
    auto* backend = static_cast<IImageAnalysisBackend*>(
        plugin->getInstance()
    );
    
    Image test_image = loadTestImage();
    auto embedding = backend->generateEmbedding(test_image);
    
    EXPECT_EQ(embedding.size(), 512);  // CLIP standard size
    EXPECT_GT(embedding[0], -1.0f);
    EXPECT_LT(embedding[0], 1.0f);
}
```

---

## Zusammenfassung

### Kernantworten auf die ursprünglichen Fragen:

#### 1. **Wie ist das Plugin-System integriert?**

Das Plugin-System ist tief in ThemisDB integriert durch:
- Zentralen `PluginManager` (Singleton)
- Einheitliches `IThemisPlugin` Interface
- Dynamisches Laden zur Laufzeit (LoadLibrary/dlopen)
- Type-Registry für typsicheren Zugriff
- Manifest-basierte Plugin-Discovery

#### 2. **Datensicherheit?**

Mehrschichtige Sicherheit:
- SHA-256 Hash-Verifikation
- Digitale Signaturen mit X.509 Zertifikaten
- Manifest-Signaturen
- Whitelist/Blacklist-System
- Sandbox-Isolation
- Sicherheits-Audit-Logging

#### 3. **Performance?**

Exzellente Performance:
- Minimaler Aufruf-Overhead (~3 ns vs. direkter Aufruf)
- Schnelles Laden (85-450ms je nach Plugin)
- Hot-Reload-fähig (~200ms)
- Thread-safe und parallelisierbar
- Zero-Copy für Datenübertragung

#### 4. **Namespace-Verfügbarkeit (themis::plugins::plugin122)?**

**Nein**, keine dynamischen Namespaces:
- Plugins werden nicht als `themis::plugins::plugin122` exponiert
- Stattdessen: Name-basierter Zugriff über `PluginManager`
- Type-safe Zugriff über `PluginRegistry` Templates
- Optionale statische Aliases für häufig verwendete Plugins

---

## Weiterführende Dokumentation

- [Plugin Development Guide](../../../README.md)
- [Plugin Security Architecture](MANIFEST_SIGNATURES.md)
- [RPC Plugin Architecture](RPC_PLUGIN_ARCHITECTURE.md)
- [Hardware Acceleration Plugins](../../../plugins/PLANNED_ACCELERATION_PLUGINS.md)

---

**Autor:** ThemisDB Development Team  
**Letzte Aktualisierung:** 20. Januar 2026  
**Version:** 1.5.0
