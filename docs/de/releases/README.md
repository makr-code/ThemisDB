# Releases & Updates Dokumentation

**Stand:** 26. April 2026
**Aktueller Versionsstand im Repo:** 1.9.0-alpha
**Kategorie:** Releases & Updates

---

## 📦 Release-Übersicht

ThemisDB Release-Dokumentation enthält:
- Release Notes für alle Versionen
- Update-System und Manifest-Management
- die kompakte manuelle Release-Strategie

Grundsatz:
- ein Git-Tag pro freigegebenem Quellstand
- mehrere Artefakte pro Release sind erlaubt
- ZIP und MSI erhalten keinen eigenen separaten Tag
- ein GitHub-Milestone definiert den jeweiligen Release-Scope
- die zugehörigen GitHub-Issues müssen diesem Milestone zugeordnet sein

## 🎉 Aktuelle Releases

### v2.0.x Serie (Q4 2026)

| Version | Datum | Fokus | Dokumentation |
|---------|-------|-------|---------------|
| **v2.0.0** | in Umsetzung | CDC Interface-Ausbau, Query v2.0.0 Port, Storage SIMD/Parquet Port | [Release Notes](./RELEASE_NOTES_v2.0.0.md) |

### v1.10.x Serie (Q3 2026)

| Version | Datum | Fokus | Dokumentation |
|---------|-------|-------|---------------|
| **v1.10.0** | in Umsetzung | MQTT Client TLS im Server-Modul | [Release Notes](./RELEASE_NOTES_v1.10.0.md) |

### v1.9.x Serie (Q3 2026)

| Version | Datum | Fokus | Dokumentation |
|---------|-------|-------|---------------|
| **v1.9.1** | in Umsetzung | Auth-Testtarget-Registrierung / Build-Integration | [Release Notes](./RELEASE_NOTES_v1.9.1.md) |
| **v1.9.0-alpha** | 26.04.2026 | Packaging- und Logging-Hardening fuer Windows Release-Artefakte | [Release Notes](./RELEASE_NOTES_v1.9.0-alpha.md) · [GitHub Release](https://github.com/makr-code/ThemisDB/releases/tag/v1.9.0-alpha) |
| **v1.9.0** | Vorabstand: 1.9.0-alpha | Chimera Adapter-Interface-Update, Governance Compliance-Regeln | [Release Notes](./RELEASE_NOTES_v1.9.0.md) |

### v1.8.x Serie (Q2 2026)

| Version | Datum | Fokus | Dokumentation |
|---------|-------|-------|---------------|
| **v1.8.2** | 19.04.2026 (Aggregation) | Retrospektive Aggregation der Features zwischen v1.8.0 und v1.8.2 inkl. QA-/Blocker-Status | [Release Notes](./RELEASE_NOTES_v1.8.2.md) |
| **v1.8.1-rc1** | 04.04.2026 | Geo/Search/Storage/Sharding-Hardening-Release (RC) | [Release Notes](./RELEASE_NOTES_v1.8.1-rc1.md) |
| **v1.8.0** | TBD | SSI, Versioned API Routing, SAGA Orchestration, Markov Prefetching, ArrowUserRegistration, JWT Scope, CRL/OCSP, HardwareAccelerator, ExporterFactory, Geo Clustering, Wire Protocol V2 | [Release Notes](./RELEASE_NOTES_v1.8.0.md) |

### v1.7.x Serie (Q2 2026)

| Version | Datum | Fokus | Dokumentation |
|---------|-------|-------|---------------|
| **v1.7.0** | TBD | Config Hierarchie, Multi-GPU Scaffolding, Git-Like Features, HybridSearch Hardening, FAISS ADC, Docs Audit | [Release Notes](./RELEASE_NOTES_v1.7.0.md) |

### v1.5.x Serie (Februar 2026)

| Version | Datum | Fokus | Dokumentation |
|---------|-------|-------|---------------|
| **v1.5.0** | 03.02.2026 | GPU Kernel Sandbox, QueryMaskingPolicy, Geo CPU/GPU Benchmarks, Full GeoJSON RFC 7946, FAISS Quantizer, RFC 3161 TSA | [Release Target](./RELEASE_TARGET_v1.5.0.md) · [Release Notes](./RELEASE_NOTES_v1.5.0.md) |

### v1.3.x Serie (Dezember 2025)

| Version | Datum | Fokus | Dokumentation |
|---------|-------|-------|---------------|
| **v1.3.5** | 28.12.2025 | Documentation Update & Build | [Release Notes](./RELEASE_NOTES_v1.3.5.md) |
| **v1.3.3** | 21.12.2025 | Network Protocol Enhancements | [Release Notes](./RELEASE_NOTES_v1.3.3.md) |
| **v1.3.2** | 21.12.2025 | Image Analysis AI Plugin | [Release Notes](./RELEASE_NOTES_v1.3.2.md) |
| **v1.3.1** | 20.12.2025 | Third-Party Attribution | [Release Notes](./RELEASE_NOTES_v1.3.1.md) |
| **v1.3.0** | 17.12.2025 | LLM Integration (Optional) | [Release Notes](./RELEASE_NOTES_v1.3.0.md) |

### Frühere Releases

| Version | Datum | Dokumentation |
|---------|-------|---------------|
| v1.2.0 | 28.11.2025 | [Release Notes](./v1.2.0.md) |
| v1.1.0 | 15.10.2025 | [Release Notes](./v1.1.0.md) |
| v1.0.0 | 01.09.2025 | - |

## 📋 Release Notes

### Version 1.3.5 - Documentation Update & Build
- 📚 MkDocs Integration mit Material Theme
- 📄 PDF-Export und GitHub Wiki Generation
- 🔧 Verbesserte Build-Pipeline für Dokumentation
- 📖 Vollständige deutsche Dokumentation in docs/de/
- 🔄 Versionsverwaltung und Cross-Referenzen

### Version 1.3.3 - Network Protocol Enhancements
- 🌐 HTTP/2 mit Server Push für CDC/Changefeed
- 📡 WebSocket Support mit CDC Streaming
- 📬 MQTT Broker mit WebSocket Transport
- ⚡ HTTP/3 Basisimplementierung mit QUIC
- 🐘 PostgreSQL Wire Protocol für BI-Tool-Kompatibilität
- 🔌 MCP Server (Model Context Protocol)

### Version 1.3.2 - Image Analysis AI Plugin
- 🖼️ Plugin-Architektur für Bildanalyse-KI
- 🔧 Multi-Backend-Unterstützung (llama.cpp Vision, ONNX, OpenCV, OpenVINO, ncnn)
- 🛠️ Plugin-Interfaces: IImageAnalysisBackend, ImageAnalysisManager
- 🧪 Umfassende Unit Tests (15+ Testfälle) und Benchmarks

### Version 1.3.1 - Third-Party Attribution
- 📄 ATTRIBUTIONS.md mit 15+ Kern-Abhängigkeiten
- 🏆 Dokumentation von ThemisDBs 12 einzigartigen Innovationen
- ✅ Klare Attribution für alle Hauptabhängigkeiten

### Version 1.3.0 - LLM Integration
- 🧠 Native LLM-Integration mit llama.cpp (optional)
- ⚡ GPU-Beschleunigung (CUDA, Metal, Vulkan)
- 🔌 Plugin-Architektur für LLM-Backends
- 📊 Monitoring mit Grafana/Prometheus

## 📚 Update-System Dokumentation

| Datei | Beschreibung |
|-------|--------------|
| [updates_checker.md](./updates_checker.md) | Update Checker Funktionalität |
| [updates_konzept.md](./updates_konzept.md) | Konzept Update Checker |
| [updates_security_summary.md](./updates_security_summary.md) | Sicherheitsübersicht |
| [updates_encrypted_manifests.md](./updates_encrypted_manifests.md) | Verschlüsselte Manifeste Konzept |
| [updates_manifest_encryption.md](./updates_manifest_encryption.md) | Manifest-Verschlüsselung Analyse |
| [updates_manifest_security.md](./updates_manifest_security.md) | Manifest-Sicherheitsprinzipien |
| [updates_release_manifest.md](./updates_release_manifest.md) | Release Manifest Service |
| [updates_distribution_strategy.md](./updates_distribution_strategy.md) | Release-Verteilungsstrategie |

## 🔄 Release-Prozess

1. Release-Kandidat auf dem passenden Release-Zweig vorbereiten.
2. Passenden GitHub-Milestone anlegen oder prüfen und Release-Issues zuordnen.
3. Version in `VERSION` und Release Notes prüfen.
4. Artefakte manuell bauen.
5. Einen Release-Tag für den freigegebenen Quellstand erzeugen.
6. Alle zugehörigen Artefakte unter demselben Release veröffentlichen.

Siehe dazu auch [../../RELEASE_STRATEGY.md](../../RELEASE_STRATEGY.md).

## 🔗 Verwandte Dokumentation

- [../../CHANGELOG.md](CHANGELOG.md) - Vollständiger Changelog
- [../deployment/v1.3.5_RELEASE_BUILD_STRATEGY.md](../deployment/v1.3.5_RELEASE_BUILD_STRATEGY.md) - v1.3.5 Build-Strategie
- [../security/security_encryption_strategy.md](../security/security_encryption_strategy.md) - Verschlüsselungsstrategie
