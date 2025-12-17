# Plugins Documentation

**Stand:** 5. Dezember 2025  
**Version:** 1.0.0  
**Kategorie:** Plugins

---

## Übersicht

ThemisDB unterstützt ein Plugin-System für Erweiterungen.

## Features

| Feature | Status | Beschreibung | Release |
|---------|--------|--------------|---------|
| Manifest Signatures | ✅ Production | Plugin Verification | v1.0.0 |
| Plugin Migration | ✅ Production | Migration Support | v1.0.0 |
| RPC Framework | 📋 Design | RPC Plugin-Architektur | v1.3.0 |

## Source-Code Referenz

- `include/enterprise/analytics_plugins.h` - Plugin Interface
- Plugin-System mit Lizenzierung (CORE, Professional, Enterprise)

## Dokumentation in diesem Ordner

| Datei | Beschreibung |
|-------|--------------|
| [MANIFEST_SIGNATURES.md](MANIFEST_SIGNATURES.md) | Plugin Manifest Signing |
| [PLUGIN_MIGRATION.md](PLUGIN_MIGRATION.md) | Plugin Migration Guide |
| [RPC_PLUGIN_ARCHITECTURE.md](RPC_PLUGIN_ARCHITECTURE.md) | RPC Framework & Plugin-Integration |

## Verwandte Dokumentation

- [Enterprise Features](../enterprise/README.md) - Enterprise Plugin System
- [Security Module](../security/README.md) - Signature Verification
