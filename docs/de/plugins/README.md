# Plugins Documentation

**Stand:** 22. Dezember 2025  
**Version:** v1.3.0  
**Kategorie:** 🔌 Plugins

---

## 📑 Inhaltsverzeichnis

- [Übersicht](#übersicht)
- [Features](#features)
- [Dokumentation](#dokumentation)

---

## Übersicht

ThemisDB unterstützt ein Plugin-System für Erweiterungen.

## Features

| Feature | Status | Beschreibung | Release |
|---------|--------|--------------|---------|
| Manifest Signatures | ✅ Production | Plugin Verification | v1.0.0 |
| Plugin Migration | ✅ Production | Migration Support | v1.0.0 |
| RPC Framework | ✅ Production | RPC Plugin-Architektur | v1.3.0 |
| Image Analysis Plugins | ✅ Production | Multi-backend AI für Bildanalyse | v1.3.0 |

## Image Analysis Plugin Architecture (v1.3.0)

ThemisDB unterstützt jetzt eine erweiterbare Plugin-Architektur für Bildanalyse-AI:

### Unterstützte Backends

- **llama.cpp Vision** (Primary) - Integriert mit LLM-Engine
- **ONNX Runtime** - CLIP und andere ONNX-Modelle
- **OpenCV DNN** - Leichtgewichtige Inferenz
- **OpenVINO** - Intel-Hardware-Optimierung
- **ncnn** - Mobile und Edge-Deployment

### Dokumentation

- [Image Analysis Plugin Guide](../llm/IMAGE_ANALYSIS_PLUGINS.md) - Vollständige Plugin-Dokumentation
- [Multi-Backend Support](../llm/IMAGE_ANALYSIS_BACKENDS.md) - Backend-Vergleiche und Setup
- [Plugin Development](../llm/README_PLUGINS.md) - Eigene Plugins entwickeln

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
