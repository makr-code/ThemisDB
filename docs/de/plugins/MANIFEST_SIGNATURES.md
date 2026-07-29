# Plugin Manifest Signature Verification

> ⚠️ **[PRIVATE] Governance Update (Hyperscaler):**
> Manifest-Signatur-Generierung ist owner-kontrolliert und wurde in
> `plugins/themisdb_plugin_signer` ausgelagert.
> Die öffentliche Toolchain bietet Verifikation über
> `/home/runner/work/ThemisDB/ThemisDB/tools/verify_plugin_manifest.py`.

**Stand:** 6. April 2026  
**Version:** 1.0.0  
**Kategorie:** Plugins

---


**Datum:** 21. November 2025  
**Feature:** Signatur-Verifikation für plugin.json Manifeste  
**Zweck:** Sicherstellung der Integrität von Plugin-Manifesten

---

## 🔒 Übersicht

ThemisDB verwendet eine Signatur-Verifikationsstrategie für Plugin-Manifeste (`plugin.json`), ähnlich wie für YAML-Konfigurationsdateien. Dies verhindert Manipulation von Plugin-Metadaten.

### Sicherheitsmodell

**Entwicklungsmodus (Debug Build):**
- Signaturen optional
- Warnung bei fehlenden Signaturen
- Warnung bei Hash-Mismatch, aber Plugin wird trotzdem geladen
- Erleichtert Entwicklung und Tests

**Produktionsmodus (Release Build):**
- Signaturen **erforderlich**
- Plugin wird **nicht** geladen ohne gültige Signatur
- Verhindert Manipulation und Supply-Chain-Angriffe
- Strikte Validierung

---

## 📝 Signatur-Format

### Signatur-Datei: `plugin.json.sig`

Die Signatur-Datei enthält eine **detached Ed25519-Signatur** (Base64) des Manifests.

```
5S3g+8a1iW3VZ0d6Y9b1D2sN4h7QwXyK3mPj4w9j+1Tj2x5v1k2GqSgXJz7N7w==
```

**Eigenschaften:**
- Einzeilige Datei
- 64-Byte Ed25519-Signatur, Base64-kodiert
- Signatur wird mit owner-kontrolliertem privatem Schlüssel erzeugt
- Verifikation mit gepinntem öffentlichem Schlüssel

---

## 🛠️ Signatur Generieren

### Verifikation mit öffentlichem Python-Tool

```bash
# Einzelnes Manifest verifizieren
python /home/runner/work/ThemisDB/ThemisDB/tools/verify_plugin_manifest.py \
  plugins/blob/filesystem/plugin.json \
  plugins/blob/filesystem/plugin.json.sig
```

### Manuell mit OpenSSL

```bash
# SHA256-Hash berechnen
openssl dgst -sha256 -hex plugin.json | awk '{print $2}' > plugin.json.sig
```

### Manuell mit sha256sum (Linux)

```bash
sha256sum plugin.json | awk '{print $1}' > plugin.json.sig
```

---

## 🔍 Verifikationsprozess

### Automatische Verifikation

Der PluginManager verifiziert Manifeste automatisch beim Laden:

```cpp
// Beim Scannen von Plugins
auto& pm = PluginManager::instance();
pm.scanPluginDirectory("./plugins");  // Verifiziert alle plugin.json automatisch
```

**Verifikationsschritte:**
1. Prüfe ob `plugin.json.sig` existiert
2. Lese erwarteten Hash aus `.sig` Datei
3. Berechne tatsächlichen Hash von `plugin.json`
4. Vergleiche Hashes
5. Bei Mismatch: Fehler (Produktion) oder Warnung (Entwicklung)

### Logs

**Erfolgreiche Verifikation:**
```
[INFO] Manifest signature verified: ./plugins/blob/filesystem/plugin.json
```

**Fehlende Signatur (Produktion):**
```
[ERROR] Manifest signature file not found: ./plugins/blob/filesystem/plugin.json.sig
```

**Hash-Mismatch (Produktion):**
```
[ERROR] Manifest signature verification failed: hash mismatch
  Expected: a1b2c3d4...
  Actual:   f9e8d7c6...
```

**Warnung (Entwicklung):**
```
[WARN] Manifest signature file not found (development mode): ./plugins/blob/filesystem/plugin.json.sig
```

---

## 📁 Plugin-Struktur mit Signatur

```
plugins/
├── blob/
│   ├── filesystem/
│   │   ├── plugin.json          # Manifest
│   │   ├── plugin.json.sig      # ✅ Signatur (SHA256)
│   │   ├── themis_blob_fs.dll   # Binary (Windows)
│   │   ├── themis_blob_fs.so    # Binary (Linux)
│   │   └── themis_blob_fs.dylib # Binary (macOS)
│   └── webdav/
│       ├── plugin.json
│       ├── plugin.json.sig      # ✅ Signatur
│       └── themis_blob_webdav.dll
└── importers/
    └── postgres/
        ├── plugin.json
        ├── plugin.json.sig      # ✅ Signatur
        └── themis_import_pg.dll
```

---

## 🔐 Erweiterte Signatur-Strategien

### Aktuell: Detached Ed25519-Signaturen (Owner-only Signing)

**Vorteile:**
- ✅ Starke Authentifizierung mit gepinntem Public Key
- ✅ Schnelle Verifikation
- ✅ Schutz gegen Manipulation

**Nachteile:**
- ⚠️ Signatur-Erzeugung ist bewusst auf private Owner-Tooling begrenzt

### Signatur-Erzeugung

Die Signatur-Erzeugung erfolgt ausschließlich über das private Repository
`plugins/themisdb_plugin_signer`.

**Integration mit bestehendem System:**
- Nutzt `storage/security_signature.h` (SecuritySignature struct)
- Nutzt `storage/security_signature_manager.h` (RocksDB-basiert)
- Nutzt `acceleration/plugin_security.h` (PluginSignature)

---

## 🚀 Build-Integration

### CMake Build-Schritt

```cmake
# plugins/blob/filesystem/CMakeLists.txt

# Erstelle Binary
add_library(themis_blob_fs SHARED
    filesystem_plugin.cpp
)

# Verifiziere Manifest nach Build (öffentlicher Verify-Pfad)
add_custom_command(TARGET themis_blob_fs POST_BUILD
    COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/verify_plugin_manifest.py
            ${CMAKE_CURRENT_SOURCE_DIR}/plugin.json
            ${CMAKE_CURRENT_SOURCE_DIR}/plugin.json.sig
    COMMENT "Signing plugin manifest: filesystem"
)
```

### CI/CD Pipeline

```yaml
# .github/workflows/build-plugins.yml
- name: Build Plugins
  run: cmake --build build --target all

- name: Verify Plugin Manifests
  run: |
    python tools/verify_plugin_manifest.py plugins/blob/filesystem/plugin.json plugins/blob/filesystem/plugin.json.sig
    python tools/verify_plugin_manifest.py plugins/blob/webdav/plugin.json plugins/blob/webdav/plugin.json.sig
    python tools/verify_plugin_manifest.py plugins/importers/postgres/plugin.json plugins/importers/postgres/plugin.json.sig

- name: Verify Signatures
  run: |
    # Test in production mode
    cmake -DCMAKE_BUILD_TYPE=Release ...
    ./bin/themis_server --verify-plugins
```

---

## 🧪 Testing

### Unit Tests

```cpp
// tests/test_plugin_manifest_signature.cpp
TEST(PluginManagerTest, ManifestSignatureVerification) {
    // Create test manifest
    std::string manifest_path = "./test_plugin.json";
    createTestManifest(manifest_path);
    
    // Generate signature
    std::string hash = PluginManager::calculateFileHash(manifest_path);
    std::ofstream sig(manifest_path + ".sig");
    sig << hash << std::endl;
    sig.close();
    
    // Verify
    std::string error;
    PluginManager pm;
    EXPECT_TRUE(pm.verifyManifestSignature(manifest_path, error));
}

TEST(PluginManagerTest, ManifestSignatureMismatch) {
    std::string manifest_path = "./test_plugin.json";
    createTestManifest(manifest_path);
    
    // Wrong signature
    std::ofstream sig(manifest_path + ".sig");
    sig << "0000000000000000000000000000000000000000000000000000000000000000" << std::endl;
    sig.close();
    
    std::string error;
    PluginManager pm;
    
#ifdef NDEBUG
    EXPECT_FALSE(pm.verifyManifestSignature(manifest_path, error));
#else
    EXPECT_TRUE(pm.verifyManifestSignature(manifest_path, error));  // Allowed in dev
#endif
}
```

---

## 📊 Vergleich: YAML vs. JSON Signatur

| Aspekt | YAML Config | JSON Manifest |
|--------|-------------|---------------|
| **Datei** | config.yaml | plugin.json |
| **Signatur-Datei** | RocksDB (SecuritySignature) | plugin.json.sig (Datei) |
| **Hash/Signatur** | SHA256 (intern) | Ed25519 detached signature |
| **Speicherort** | RocksDB (security_sig:*) | Filesystem (neben Manifest) |
| **Verifikation** | SecuritySignatureManager | PluginManager |
| **Entwicklung** | Optional | Optional |
| **Produktion** | Optional | **Erforderlich** |

**Warum unterschiedlich?**
- YAML: Zentrale Konfiguration → RocksDB-basiert
- JSON: Verteilte Plugins → Filesystem-basiert (portabel)

---

## 🔧 Fehlerbehebung

### Problem: "Manifest signature file not found"

**Lösung:**
```bash
# Signatur über private Owner-Pipeline neu erzeugen und danach verifizieren
python tools/verify_plugin_manifest.py plugin.json plugin.json.sig
```

### Problem: "Hash mismatch"

**Ursache:** Manifest wurde nach Signierung geändert

**Lösung:**
```bash
# Neu signieren über private Owner-Pipeline, dann verifizieren
python tools/verify_plugin_manifest.py plugin.json plugin.json.sig
```

### Problem: "Plugin wird nicht geladen (Produktion)"

**Prüfung:**
```bash
# Prüfe ob Signatur existiert
ls -la plugin.json.sig

# Prüfe Signatur gegen gepinnten Public Key
python tools/verify_plugin_manifest.py plugin.json plugin.json.sig
```

---

## 🎯 Best Practices

1. **Signatur immer vor Deployment verifizieren**
   ```bash
   python tools/verify_plugin_manifest.py plugin.json plugin.json.sig
   ```

2. **Signaturen in Version Control**
   - Committe `plugin.json.sig` zusammen mit `plugin.json`
   - Nie nur Manifest ohne Signatur commiten

3. **CI/CD Automatisierung**
   - Signieren nur im privaten Owner-Workflow
   - Verifikation vor Package-Erstellung

4. **Versionierung**
   - Bei Manifest-Änderung: Neu signieren
   - Bei Version-Bump: Neu signieren

5. **Security Audits**
   - Regelmäßige Prüfung aller Signaturen
   - Rotation von Signatur-Schlüsseln (bei RSA/ECDSA)

---

## 📚 Weiterführende Dokumentation

- `include/plugins/plugin_manager.h` - PluginManager Interface
- `src/plugins/plugin_manager.cpp` - Verifikations-Implementation
- `tools/verify_plugin_manifest.py` - Signatur-Verifikation (öffentlich)
- `include/storage/security_signature.h` - Bestehende Signatur-Infrastruktur
- `docs/plugins/PLUGIN_MIGRATION.md` - Plugin-System Architektur

---

**Status:** ✅ Implementiert  
**Version:** 1.0.0  
**Nächste Schritte:** CRL/OCSP-Strategie in privater Hyperscaler-Signing-Infrastruktur
