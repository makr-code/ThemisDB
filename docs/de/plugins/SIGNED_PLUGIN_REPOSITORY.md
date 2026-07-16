# Signed Plugin Repository with Key Pinning

**Stand:** 6. April 2026  
**Version:** 1.0.0  
**Kategorie:** Plugins / Security  
**Implementierung:** `include/plugins/signed_plugin_repository.h`, `src/plugins/signed_plugin_repository.cpp`

---

## Übersicht

`SignedPluginRepository` ist ein thread-sicherer, in-process Katalog für Plugin-Einträge, bei dem jeder Eintrag mit einer gültigen **Ed25519-Signatur** versehen sein muss. Die Signatur wird gegen eine explizit verwaltete Menge von **gepinnten öffentlichen Schlüsseln** geprüft (*key pinning*), bevor ein Eintrag akzeptiert wird.

Key Pinning verhindert:
- **Man-in-the-Middle-Angriffe** auf den Plugin-Distribution-Kanal
- **Registry-Substitution-Angriffe** (Austausch des Plugin-Index durch einen Angreifer)
- Laden von Plugins, die von einem inzwischen widerrufenen Schlüssel signiert wurden

Das Konzept ist analog zu HTTP Public Key Pinning (HPKP) für TLS oder GPG-Keyring-Verwaltung in Paketmanagern (apt/dnf).

---

## Kernkonzepte

### PinnedKey

```cpp
struct PinnedKey {
    std::string fingerprint;       // Hex-SHA-256 des rohen 32-Byte Ed25519-Schlüssels
    std::vector<uint8_t> public_key; // 32 Bytes raw Ed25519 public key
    std::string label;             // Lesbare Bezeichnung, z. B. "ThemisDB Official"
    bool active = true;            // false = Schlüssel deaktiviert (Key Rotation)
};
```

Durch das `active`-Flag können Schlüssel bei einer Key-Rotation deaktiviert werden, ohne sie zu löschen. Deaktivierte Schlüssel werden für Audit-Zwecke beibehalten.

### RepositoryEntry

```cpp
struct RepositoryEntry {
    MarketplaceManifest manifest;  // Vollständiges Plugin-Manifest
    std::string signature_b64;    // Base64-Ed25519-Signatur über das kanonische JSON
    std::string key_fingerprint;  // Fingerprint des Signier-Schlüssels
};
```

Die Signatur wird über das **kanonische JSON** des Manifests erstellt (alphabetisch sortierte Felder, kein Whitespace) – nicht über die serialisierte `MarketplaceManifest`-Struktur direkt.

### Kanonisches JSON

Die Nutzlast für Signatur/Verifikation enthält folgende Felder in alphabetischer Reihenfolge:

```json
{
  "author":             "...",
  "binary_linux":       "plugin.so",
  "binary_macos":       "plugin.dylib",
  "binary_windows":     "plugin.dll",
  "description":        "...",
  "expected_hash":      "<sha256-hex>",
  "license":            "MIT",
  "min_themis_version": "1.0.0",
  "name":               "my_plugin",
  "type":               "custom",
  "verified_publisher": false,
  "version":            "1.0.0"
}
```

> Nur diese Felder sind Teil des signierten Payloads. Display-Felder wie `homepage`, `repository`, `tags` und `category` sind explizit ausgeschlossen.

---

## Verwendung

### 1. Schlüssel pinnen

```cpp
#include "plugins/signed_plugin_repository.h"

themis::plugins::SignedPluginRepository repo;

// Öffentlichen Schlüssel aus Konfiguration / Zertifikat laden
std::vector<uint8_t> raw_pub_key = load_file_bytes("/etc/themis/certs/repo_pub.bin");

themis::plugins::PinnedKey official;
official.fingerprint = themis::plugins::SignedPluginRepository::computeKeyFingerprint(raw_pub_key);
official.public_key  = raw_pub_key;
official.label       = "ThemisDB Official Repository";
official.active      = true;

repo.addPinnedKey(official);
```

### 2. Signierten Index laden

```cpp
// Einträge aus dem Registry-Endpunkt empfangen
for (const auto& entry : fetch_registry_index()) {
    if (!repo.addEntry(entry)) {
        // Signatur ungültig oder Schlüssel nicht gepinnt
        spdlog::error("Rejected repository entry: {}", entry.manifest.name);
    }
}
```

### 3. Plugin zur Ladezeit nachschlagen

```cpp
// Neueste Version suchen
auto entry = repo.findEntry("s3_blob_storage");
if (!entry) {
    throw std::runtime_error("Plugin not found in signed repository");
}

// Spezifische Version suchen
auto entry_v2 = repo.findEntry("s3_blob_storage", "2.0.0");
```

---

## Sicherheitsmodell

### Verifikationsablauf

```
addEntry(RepositoryEntry)
    │
    ▼
[Lock mutex_]
    │
    ├── findPinnedKeyLocked(entry.key_fingerprint)
    │       └── key not found / inactive → return false
    │
    ├── base64Decode(entry.signature_b64)
    │       └── empty/invalid → return false
    │
    ├── canonicalManifestJson(entry.manifest) → payload
    │
    ├── verifyEd25519Signature(key.public_key, payload, sig)
    │       └── sig invalid → return false
    │
    └── Insert/replace entry in catalog
[Unlock mutex_]
```

> **Wichtig:** Verifikation und Einfügung werden unter demselben Mutex durchgeführt.
> Dies verhindert TOCTOU-Angriffe (Time-of-Check/Time-of-Use), bei denen ein Schlüssel
> zwischen Verifikation und Einfügung deaktiviert werden könnte.

### Key Rotation

```cpp
// Neuen Schlüssel hinzufügen
PinnedKey new_key;
new_key.fingerprint = computeKeyFingerprint(new_pub_key);
new_key.public_key  = new_pub_key;
new_key.active      = true;
repo.addPinnedKey(new_key);

// Alten Schlüssel deaktivieren (behält Schlüssel für Audit)
PinnedKey old_deactivated = old_key;
old_deactivated.active = false;
repo.addPinnedKey(old_deactivated);  // ersetzt bestehenden Eintrag

// Vollständig entfernen (wenn nicht mehr benötigt)
repo.removePinnedKey(old_fingerprint);
```

---

## Performance

| Operation                        | Ziel       | Messung         |
|----------------------------------|------------|-----------------|
| Ed25519-Signaturverifikation     | < 1 ms     | ~0.1–0.3 ms     |
| SHA-256-Fingerprint-Berechnung   | < 0.1 ms   | ~0.01 ms        |
| Kanonisches JSON (100-Zeichen)   | < 0.1 ms   | < 0.01 ms       |

Die Performance wird durch einen automatisierten Test in `tests/test_signed_plugin_repository.cpp` validiert (`VerifyEntryMeetsPerformanceTarget`).

---

## Thread-Sicherheit

Alle öffentlichen Methoden sind thread-sicher und durch einen einzelnen `std::mutex` geschützt. Der Mutex wird für die gesamte Dauer von `addEntry` (inkl. Signaturverifikation) gehalten, um TOCTOU-Fenster zu verhindern.

---

## Signing-Workflow

### Plugin-Eintrag signieren (Python-Beispiel)

```python
from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PrivateKey
from cryptography.hazmat.primitives.serialization import Encoding, PublicFormat
import json, base64, hashlib

def canonical_json(manifest: dict) -> str:
    fields = ["author", "binary_linux", "binary_macos", "binary_windows",
              "description", "expected_hash", "license", "min_themis_version",
              "name", "type", "verified_publisher", "version"]
    ordered = {k: manifest[k] for k in fields if k in manifest}
    return json.dumps(ordered, separators=(',', ':'), sort_keys=True)

# Signieren
private_key = Ed25519PrivateKey.generate()
payload = canonical_json(manifest).encode()
signature = private_key.sign(payload)
signature_b64 = base64.b64encode(signature).decode()

# Fingerprint des öffentlichen Schlüssels berechnen
pub_raw = private_key.public_key().public_bytes(Encoding.Raw, PublicFormat.Raw)
fingerprint = hashlib.sha256(pub_raw).hexdigest()
```

---

## Zugehörige Dateien

| Datei                                              | Beschreibung                            |
|----------------------------------------------------|-----------------------------------------|
| `include/plugins/signed_plugin_repository.h`      | API-Definition                          |
| `src/plugins/signed_plugin_repository.cpp`        | Implementierung                         |
| `tests/test_signed_plugin_repository.cpp`         | Unit-Tests (36 Tests)                   |
| `include/plugins/plugin_interface.h`              | `MarketplaceManifest`, `PluginSignatureInfo` |
| `include/plugins/manifest_schema_v2.json`         | JSON-Schema für Manifeste               |
| `docs/de/plugins/MANIFEST_SIGNATURES.md`          | Signierung von plugin.json-Manifesten   |

---

## Verwandte Issues und PRs

- Issue #1571: Signed plugin repository with key pinning (diese Implementierung)
- Issue #1554: Hot-reload support (Q2 2026)
- Issue #1572: WASM-based plugin isolation (geplant)
