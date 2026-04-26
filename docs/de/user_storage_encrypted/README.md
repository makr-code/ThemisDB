# User-Storage-Encrypted Plugin

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: PRIMARY_SOURCES.md · ../../../src/user_storage_encrypted/README.md -->

**Stand:** 6. April 2026  
**Version:** 0.2.0  
**Kategorie:** Storage / Sicherheit  
**Validated:** 2026-04-06  
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

## Primäre Dokumentation

| Dokument | Beschreibung |
|----------|--------------|
| [`src/user_storage_encrypted/README.md`](../../../src/user_storage_encrypted/README.md) | Modulübersicht und API |
| [`src/user_storage_encrypted/CHANGELOG.md`](../../../src/user_storage_encrypted/CHANGELOG.md) | Änderungshistorie |
| [`src/user_storage_encrypted/ROADMAP.md`](../../../src/user_storage_encrypted/ROADMAP.md) | Entwicklungs-Roadmap |
