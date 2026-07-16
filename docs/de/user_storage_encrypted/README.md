# User-Storage-Encrypted Plugin

<!-- Status: current | validated: 2026-05-13 -->
<!-- Links: PRIMARY_SOURCES.md · ../../../src/user_storage_encrypted/README.md -->

**Stand:** 13. Mai 2026
**Version:** 0.3.0
**Kategorie:** Storage / Sicherheit
**Status:** 🟢 Production-Ready

---

## Übersicht

Das User-Storage-Encrypted-Plugin bietet mehrstufige, gocryptfs-basierte Verschlüsselung für benutzerspezifische Datenbereiche innerhalb von ThemisDB. Es implementiert transparente FUSE-Mount-Lebenszyklusverwaltung, Argon2id-Schlüsselableitung und automatische Bereinigung verwaister Mounts.

**Primäre Quellen:** [`src/user_storage_encrypted/`](../../../src/user_storage_encrypted/) · [`plugins/user_storage_encrypted/`](../../../plugins/user_storage_encrypted/)

---

## Kernkomponenten

| Komponente | Datei | Beschreibung |
|------------|-------|--------------|
| MultiLevelStorage | `multi_level_storage.hpp` | Hauptfassade: FUSE-Mount, Argon2id-KDF, verwaiste Mount-Bereinigung |
| GocryptfsBackend | `gocryptfs_backend.hpp` | Gocryptfs-FUSE-Prozessverwaltung (fork/exec) |
| reconcileStaleMounts() | `multi_level_storage.cpp` | Scannt /proc/mounts auf verwaiste FUSE-Mounts; fusermount -u / umount-Fallback |

---

## v0.2.0 Änderungen (neu)

- `reconcileStaleMounts()`: Scannt `/proc/mounts` auf verwaiste FUSE-Mounts unter konfigurierten Basispfaden; unmountet via `fusermount -u` / `umount`-Fallback; nicht-fatal
- Aufgerufen von `initialize()` vor `initializeLevel()`
- 5 neue `StaleMountReconciliationTest`-Tests (insgesamt 27 Tests)

---

## v0.3.0 Änderungen

- **Prometheus Metrics**: `MultiLevelEncryptedStorage::getMetricsText()` liefert 4 Metrik-Familien im Prometheus-Textformat:
  - `user_storage_mounts_active` (Gauge)
  - `user_storage_mount_operations_total` (Counter, Label: `operation`)
  - `user_storage_key_rotations_total` (Counter, Label: `level`)
  - `user_storage_container_size_bytes` (Gauge)
- **`executeCommand()` entfernt**: Alle Aufrufstellen auf `executeCommandSafe()` migriert

---

## Installation

Das Modul wird als Teil von ThemisDB gebaut. Build-Voraussetzungen:

```bash
# Laufzeit-Abhängigkeiten
apt-get install gocryptfs fuse libargon2-dev

# Build
cmake --preset linux-release && cmake --build --preset linux-release
```

Include-Pfad für eigene Targets:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```

---

## Usage

### Minimales Beispiel (C++)

```cpp
#include "user_storage_encrypted/gocryptfs_backend.hpp"
#include "user_storage_encrypted/multi_level_storage.hpp"

using namespace themis::plugins::user_storage;

MultiLevelEncryptedStorage storage;
storage.initialize(R"({"base_path": "/var/lib/themisdb/user_storage", "levels": []})");
storage.mountAll();

User u;
u.user_id = "user-42";
u.username = "alice";
u.classification = SecurityLevel::VS_NFD;
storage.createUser(u, SecurityLevel::VS_NFD);

storage.unmountAll();
storage.shutdown();
```

Vollständige API-Referenz: [`include/user_storage_encrypted/README.md`](../../../include/user_storage_encrypted/README.md)

---

## Primäre Dokumentation

| Dokument | Beschreibung |
|----------|--------------|
| [`src/user_storage_encrypted/README.md`](../../../src/user_storage_encrypted/README.md) | Implementierungsübersicht und Quick-Start |
| [`include/user_storage_encrypted/README.md`](../../../include/user_storage_encrypted/README.md) | Öffentliche API-Referenz (alle 8 Header) |
| [`src/user_storage_encrypted/ARCHITECTURE.md`](../../../src/user_storage_encrypted/ARCHITECTURE.md) | Komponentendiagramm, Key-Material-Fluss |
| [`src/user_storage_encrypted/SECURITY.md`](../../../src/user_storage_encrypted/SECURITY.md) | Bedrohungsmodell und Sicherheitskontrollen |
| [`src/user_storage_encrypted/ROADMAP.md`](../../../src/user_storage_encrypted/ROADMAP.md) | Entwicklungs-Roadmap |
| [`src/user_storage_encrypted/FUTURE_ENHANCEMENTS.md`](../../../src/user_storage_encrypted/FUTURE_ENHANCEMENTS.md) | Geplante Erweiterungen (Spezifikationen) |
| [`src/user_storage_encrypted/CHANGELOG.md`](../../../src/user_storage_encrypted/CHANGELOG.md) | Änderungshistorie |
