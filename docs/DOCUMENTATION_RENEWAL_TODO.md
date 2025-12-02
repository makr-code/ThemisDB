# ThemisDB Dokumentations-Erneuerungs-TODO

**Version:** 1.0  
**Erstellt:** 2. Dezember 2025  
**Zweck:** Systematische Überprüfung und Aktualisierung der gesamten ThemisDB-Dokumentation  
**Ausgelöst durch:** Issue zu STREAMING_ARCHITECTURE.md - Dokumentation entspricht nicht mehr dem aktuellen Stand

---

## 📋 Executive Summary

Die ThemisDB-Dokumentation umfasst derzeit:
- **6 Markdown-Dateien** im Root-Verzeichnis
- **439 Markdown-Dateien** im `/docs`-Verzeichnis
- **59 Unterverzeichnisse** in `/docs`

Diese TODO-Liste dient der systematischen Überprüfung und Aktualisierung aller Dokumentationsdateien, um sicherzustellen, dass sie den aktuellen Implementierungsstand widerspiegeln.

---

## 🎯 Ziele

1. **Aktualität**: Alle Dokumente sollen den aktuellen Code-Stand reflektieren
2. **Konsistenz**: Einheitliche Terminologie und Formatierung
3. **Link-Integrität**: Alle internen und externen Links sollen funktionieren
4. **Vollständigkeit**: Alle implementierten Features sollen dokumentiert sein
5. **Korrektheit**: Keine veralteten oder falschen Informationen

---

## 📁 1. Root-Level Dokumente

### 1.1 README.md ⚠️ PRIORITÄT HOCH
**Pfad:** `/README.md`  
**Größe:** ~100KB (sehr umfangreich)

**Prüfpunkte:**
- [ ] Badges und CI-Status-Links validieren
- [ ] Alle Feature-Beschreibungen mit Code abgleichen
- [ ] API-Beispiele auf Funktionalität testen
- [ ] Build-Anweisungen verifizieren (Windows, Linux, WSL, Docker)
- [ ] Versions- und Datumsangaben aktualisieren
- [ ] HSM/PKCS#11 Dokumentation prüfen
- [ ] Distributed Sharding Sektion aktualisieren
- [ ] RAID-like Redundanz Beschreibung verifizieren
- [ ] CEP (Complex Event Processing) Beispiele testen
- [ ] GPU Acceleration Anweisungen prüfen
- [ ] Installation-Anweisungen für alle Package Manager verifizieren
- [ ] Docker/QNAP Deployment-Anweisungen testen
- [ ] ARM/Raspberry Pi Anweisungen validieren
- [ ] "Recent changes" Sektion auf Aktualität prüfen
- [ ] Performance-Benchmarks aktualisieren
- [ ] Alle internen Links (`docs/...`) validieren

### 1.2 CHANGELOG.md
**Pfad:** `/CHANGELOG.md`

**Prüfpunkte:**
- [ ] Alle Releases korrekt dokumentiert
- [ ] Datum und Versionsnummern prüfen
- [ ] Links zu Issues/PRs validieren

### 1.3 CONTRIBUTING.md
**Pfad:** `/CONTRIBUTING.md`

**Prüfpunkte:**
- [ ] Build-Anweisungen aktuell
- [ ] Code-Style Guidelines aktuell
- [ ] PR-Prozess beschrieben

### 1.4 SECURITY.md
**Pfad:** `/SECURITY.md`

**Prüfpunkte:**
- [ ] Vulnerability Disclosure Prozess aktuell
- [ ] Kontaktinformationen korrekt
- [ ] Unterstützte Versionen aktuell

### 1.5 QNAP_QUICKSTART.md
**Pfad:** `/QNAP_QUICKSTART.md`

**Prüfpunkte:**
- [ ] Docker-Anweisungen funktionieren
- [ ] Ports und Konfiguration korrekt

### 1.6 license.md
**Pfad:** `/license.md`

**Prüfpunkte:**
- [ ] Lizenztext korrekt
- [ ] Jahr und Copyright-Holder aktuell

---

## 📁 2. Dokumentations-Index und Übersichten

### 2.1 DOCUMENTATION_INDEX.md ⚠️ PRIORITÄT HOCH
**Pfad:** `/docs/DOCUMENTATION_INDEX.md`

**Prüfpunkte:**
- [ ] Alle verlinkten Dokumente existieren
- [ ] Kategorisierung aktuell
- [ ] Rollen-basierte Navigation korrekt
- [ ] Pfade überprüfen (viele relative Pfade)
- [ ] Letzte Aktualisierung anpassen

### 2.2 FEATURES.md
**Pfad:** `/docs/FEATURES.md`

**Prüfpunkte:**
- [ ] Feature-Status (✅/🔧/📋) mit Code abgleichen
- [ ] Neue Features hinzufügen
- [ ] Veraltete Features aktualisieren

### 2.3 Roadmap-Dokumente
- [ ] `/docs/roadmap.md` - Roadmap aktualisieren
- [ ] `/docs/NEXT_IMPLEMENTATION_PRIORITIES.md` - Prioritäten prüfen

---

## 📁 3. Architektur-Dokumentation

### 3.1 Kern-Architektur
**Verzeichnis:** `/docs/architecture/`

**Dateien zu prüfen:**
- [ ] `README.md` - Architektur-Übersicht
- [ ] `base_entity.md` - Base Entity Konzept
- [ ] `cache_invalidation_strategy.md` - Cache-Strategie
- [ ] `caching_data_structures.md` - Caching-Strukturen
- [ ] `caching_lookup_patterns.md` - Lookup-Patterns
- [ ] `content_architecture.md` - Content-Architektur
- [ ] `content_pipeline.md` - Content-Pipeline
- [ ] `ecosystem_overview.md` - Ökosystem-Übersicht
- [ ] `mvcc_design.md` - MVCC-Design
- [ ] `strategic_overview.md` - Strategische Übersicht

### 3.2 Root-Level Architektur
- [ ] `/docs/architecture.md` - Hauptarchitektur-Dokument
- [ ] `/docs/MULTI_MODEL_ARCHITECTURE.md` - Multi-Model Konzept

---

## 📁 4. Sharding & Skalierung ⚠️ PRIORITÄT HOCH

### 4.1 Sharding-Dokumentation
**Verzeichnis:** `/docs/sharding/`

**Dateien zu prüfen:**
- [ ] `README.md` - Sharding-Übersicht
- [ ] `STREAMING_ARCHITECTURE.md` ⚠️ **AUSLÖSER DIESES TODOS**
- [ ] `SHARDING_UNIFIED_DOCUMENTATION.md` - Unified Docs
- [ ] `RAID_REDUNDANCY_ARCHITECTURE.md` - RAID-Konzept
- [ ] `horizontal_scaling_strategy.md` - Horizontale Skalierung
- [ ] `implementation_summary.md` - Implementierungsstatus
- [ ] `phase1_report.md` - Phase 1 Bericht
- [ ] `phases_1-3_summary.md` - Phasen-Zusammenfassung

### 4.2 Skalierungs-TODOs
- [ ] `/docs/SCALING_TODO.md` - Skalierungs-TODOs aktualisieren
- [ ] `/docs/ENTERPRISE_SCALABILITY.md` - Enterprise-Skalierung

---

## 📁 5. Security & Compliance

### 5.1 Security-Dokumentation
**Verzeichnis:** `/docs/security/`

**Dateien zu prüfen:**
- [ ] Alle Security-Policies aktuell
- [ ] `INCIDENT_RESPONSE_PLAN.md` - Incident Response
- [ ] `SBOM.md` - Software Bill of Materials
- [ ] `encryption_strategy.md` - Verschlüsselungsstrategie
- [ ] `column_encryption.md` - Spalten-Verschlüsselung
- [ ] `key_rotation_strategy.md` - Key-Rotation
- [ ] `INFORMATION_SECURITY_POLICY.md` - ISP
- [ ] `PASSWORD_POLICY.md` - Passwortrichtlinie
- [ ] `RISK_MANAGEMENT_FRAMEWORK.md` - Risikomanagement
- [ ] `PENETRATION_TEST_GUIDE.md` - Pen-Test Guide

### 5.2 Compliance-Dokumentation
**Verzeichnis:** `/docs/compliance/`

**Dateien zu prüfen:**
- [ ] `DPIA.md` - Datenschutz-Folgenabschätzung
- [ ] `BUSINESS_CONTINUITY_PLAN.md` - BCP
- [ ] `BCP_DRP.md` - Disaster Recovery
- [ ] `RISK_REGISTER.md` - Risiko-Register
- [ ] `VENDOR_ASSESSMENT.md` - Lieferanten-Bewertung

### 5.3 Policies
**Verzeichnis:** `/docs/policies/`

**Dateien zu prüfen:**
- [ ] `ACCESS_CONTROL_POLICY.md`
- [ ] `ENCRYPTION_KEY_MANAGEMENT_POLICY.md`
- [ ] `CHANGE_MANAGEMENT_POLICY.md`
- [ ] `DATA_CLASSIFICATION_POLICY.md`

---

## 📁 6. API & Query Language

### 6.1 AQL-Dokumentation
**Verzeichnis:** `/docs/aql/`

**Dateien zu prüfen:**
- [ ] Alle AQL-Syntax-Dokumente
- [ ] `pattern_matching.md` - Pattern Matching
- [ ] AQL-Beispiele mit Server testen

### 6.2 API-Dokumentation
**Verzeichnis:** `/docs/api/` und `/docs/apis/`

**Dateien zu prüfen:**
- [ ] `VCC_CLARA_EXPORT_API.md`
- [ ] `STREAMING_JSONL_TRAINING.md`
- [ ] `/docs/apis/openapi.md` - OpenAPI Spec
- [ ] `/docs/apis/graphql.md` - GraphQL
- [ ] `/docs/apis/contentfs_api.md` - ContentFS
- [ ] `/docs/apis/hybrid_search_api.md` - Hybrid Search

### 6.3 Query-Dokumentation
**Verzeichnis:** `/docs/query/`

**Dateien zu prüfen:**
- [ ] `README.md`
- [ ] `FILTERED_VECTOR_SEARCH.md`
- [ ] `HYBRID_QUERIES_README.md`
- [ ] `HYBRID_QUERY_BENCHMARKS.md`
- [ ] `VECTOR_HYBRID_SEARCH.md`

---

## 📁 7. Performance & Benchmarks

### 7.1 Performance-Dokumentation
**Verzeichnis:** `/docs/performance/`

**Dateien zu prüfen:**
- [ ] `benchmarks.md` - Benchmark-Ergebnisse
- [ ] `compression_benchmarks.md` - Kompression
- [ ] `compression_strategy.md` - Kompressionsstrategien
- [ ] `memory_tuning.md` - Speicher-Tuning
- [ ] `GPU_ACCELERATION_PLAN.md` - GPU-Beschleunigung
- [ ] `TBB_INTEGRATION.md` - TBB Integration
- [ ] `MULTI_CPU_SUPPORT.md` - Multi-CPU
- [ ] `ENTERPRISE_SCALABILITY_STRATEGY.md` - Enterprise
- [ ] `VULKAN_BACKEND.md` - Vulkan Backend
- [ ] `VULKAN_COMPLETE_IMPLEMENTATION.md` - Vulkan Impl.
- [ ] `HARDWARE_ACCELERATION.md` - Hardware Acceleration
- [ ] `CUDA_BACKEND.md` - CUDA Backend

---

## 📁 8. Analytics & Streaming

### 8.1 Analytics-Dokumentation
**Verzeichnis:** `/docs/analytics/`

**Dateien zu prüfen:**
- [ ] `CEP_STREAMING_ANALYTICS.md` - CEP Engine
- [ ] Weitere Analytics-Dokumente

### 8.2 Enterprise Analytics
- [ ] `/docs/ENTERPRISE_ANALYTICS.md`

---

## 📁 9. Storage & Persistence

### 9.1 Storage-Dokumentation
**Verzeichnis:** `/docs/storage/`

**Dateien zu prüfen:**
- [ ] `rocksdb_layout.md` - RocksDB Layout
- [ ] `external_blob_storage_analysis.md` - Blob Storage
- [ ] `CLOUD_BLOB_BACKENDS.md` - Cloud Backends
- [ ] `GRANULAR_BLOB_REDUNDANCY.md` - Blob Redundanz
- [ ] `geo_relational_schema.md` - Geo Schema

---

## 📁 10. Deployment & Operations

### 10.1 Deployment-Dokumentation
**Verzeichnis:** `/docs/deployment/`

**Dateien zu prüfen:**
- [ ] Alle Deployment-Guides
- [ ] Docker-Anweisungen
- [ ] Kubernetes-Konfiguration

### 10.2 Build-Dokumentation
- [ ] `/docs/BUILD_GUIDE.md` - Build-Anleitung
- [ ] `/docs/BUILD_STRATEGY.md` - Build-Strategie
- [ ] `/docs/ENTERPRISE_BUILD_GUIDE.md` - Enterprise Build
- [ ] `/docs/ARM_RASPBERRY_PI_BUILD.md` - ARM/Pi Build
- [ ] `/docs/ARM_BENCHMARKS.md` - ARM Benchmarks
- [ ] `/docs/ARM_PACKAGES.md` - ARM Packages
- [ ] `/docs/RASPBERRY_PI_TUNING.md` - Pi Tuning

### 10.3 CI/CD
- [ ] `/docs/CI_CD_MULTIARCH.md` - Multi-Arch CI/CD
- [ ] `/docs/DOCKER_MULTI_ARCH_STRATEGY.md` - Docker Multi-Arch
- [ ] `/docs/WORKFLOWS_README.md` - Workflow Doku

### 10.4 QNAP
- [ ] `/docs/QNAP_DEPLOYMENT.md` - QNAP Deployment

---

## 📁 11. Client SDKs

### 11.1 SDK-Dokumentation
**Verzeichnis:** `/docs/clients/`

**Dateien zu prüfen:**
- [ ] `javascript_sdk_quickstart.md` - JavaScript SDK
- [ ] `python_sdk_quickstart.md` - Python SDK
- [ ] `rust_sdk_quickstart.md` - Rust SDK

---

## 📁 12. Content Processing

### 12.1 Content-Dokumentation
**Verzeichnis:** `/docs/content/`

**Dateien zu prüfen:**
- [ ] `search_api.md` - Such-API
- [ ] Weitere Content-Dokumente

### 12.2 Ingestion
**Verzeichnis:** `/docs/ingestion/`

**Dateien zu prüfen:**
- [ ] Ingestion-Pipeline Dokumentation

### 12.3 Plugins
**Verzeichnis:** `/docs/plugins/`

**Dateien zu prüfen:**
- [ ] Plugin-System Dokumentation

---

## 📁 13. Geo & Spatial

### 13.1 Geo-Dokumentation
**Verzeichnis:** `/docs/geo/`

**Dateien zu prüfen:**
- [ ] GeoJSON Support
- [ ] Spatial Queries
- [ ] R-Tree Index

---

## 📁 14. Search & Indexing

### 14.1 Search-Dokumentation
**Verzeichnis:** `/docs/search/`

**Dateien zu prüfen:**
- [ ] `content_search_summary.md`
- [ ] `future_work.md`
- [ ] Fulltext Search Doku

---

## 📁 15. Observability

### 15.1 Observability-Dokumentation
**Verzeichnis:** `/docs/observability/`

**Dateien zu prüfen:**
- [ ] Prometheus Metrics
- [ ] OpenTelemetry Tracing
- [ ] Grafana Dashboards

---

## 📁 16. Development

### 16.1 Development-Dokumentation
**Verzeichnis:** `/docs/development/`

**Dateien zu prüfen:**
- [ ] `developers.md` - Developer Guide
- [ ] `implementation_status.md` - Impl. Status
- [ ] `roadmap.md` - Dev Roadmap
- [ ] `priorities.md` - Prioritäten
- [ ] Changefeed-Doku (`/docs/development/changefeed/`)
- [ ] Security-Doku (`/docs/development/security/`)
- [ ] Overview-Doku (`/docs/development/overviews/`)

---

## 📁 17. Admin Tools

### 17.1 Admin-Tool-Dokumentation
**Verzeichnis:** `/docs/admin_tools/`

**Dateien zu prüfen:**
- [ ] `README.md` - Übersicht
- [ ] `admin_guide.md` - Admin Guide
- [ ] `user_guide.md` - User Guide
- [ ] `demo_script.md` - Demo Script
- [ ] `feature_matrix.md` - Feature Matrix
- [ ] `search_sort_filter.md` - Such/Sortier/Filter

---

## 📁 18. Enterprise

### 18.1 Enterprise-Dokumentation
**Verzeichnis:** `/docs/enterprise/`

**Dateien zu prüfen:**
- [ ] `README.md` - Enterprise Features

### 18.2 Enterprise-Dokumente Root-Level
- [ ] `/docs/ENTERPRISE_BUILD_GUIDE.md`
- [ ] `/docs/ENTERPRISE_FINAL_REPORT.md`
- [ ] `/docs/ENTERPRISE_IMPLEMENTATION_STATUS.md`
- [ ] `/docs/HTTP_CLIENT_POOL_COMPLETE.md`

---

## 📁 19. Reports & Analysen

### 19.1 Reports
**Verzeichnis:** `/docs/reports/`

**Dateien zu prüfen:**
- [ ] `BUILD_SUCCESS_REPORT.md`
- [ ] `TEST_REPORT.md`
- [ ] `DOCUMENTATION_TODO.md` - Mit diesem TODO konsolidieren
- [ ] `DOCUMENTATION_GAP_ANALYSIS.md`
- [ ] `DOCUMENTATION_SUMMARY.md`
- [ ] Weitere Reports

### 19.2 Analysen
**Verzeichnis:** `/docs/analysis/`

**Dateien zu prüfen:**
- [ ] Alle Analyse-Dokumente

---

## 📁 20. Archiv

### 20.1 Archivierte Dokumente
**Verzeichnis:** `/docs/archive/`

**Prüfpunkte:**
- [ ] `README.md` - Archiv-Übersicht
- [ ] `cdc_legacy.md` - Markiert als veraltet
- [ ] `mime_detector_deprecated.md` - Deprecated
- [ ] Weitere archivierte Dokumente als deprecated kennzeichnen

---

## 📁 21. Spezielle Dokumente

### 21.1 Konzepte & Designs
- [ ] `/docs/ENCRYPTED_MANIFESTS_KONZEPT.md`
- [ ] `/docs/KONZEPT_RELEASE_MANIFEST_SERVICE.md`
- [ ] `/docs/KONZEPT_UPDATE_CHECKER.md`
- [ ] `/docs/MANIFEST_ENCRYPTION_ANALYSIS.md`
- [ ] `/docs/MANIFEST_SECURITY_PRINCIPLE.md`

### 21.2 Checklisten & Referenzen
- [ ] `/docs/FULL_AUDIT_CHECKLIST.md`
- [ ] `/docs/COMPREHENSIVE_AUDIT_TODO.md` - Mit diesem TODO abgleichen
- [ ] `/docs/SDK_PUBLISHING_CHECKLIST.md`
- [ ] `/docs/SDK_PUBLISHING_GUIDE.md`
- [ ] `/docs/DOCS_QUICKREF.md`
- [ ] `/docs/PACKAGING-QUICKREF.md`

### 21.3 Projektberichte
- [ ] `/docs/THEMIS_SACHSTANDSBERICHT_2025.md`
- [ ] `/docs/THEMIS_PROJECT_VALUATION.md`

### 21.4 Update Checker
- [ ] `/docs/UPDATE_CHECKER.md`
- [ ] `/docs/SECURITY_SUMMARY_UPDATE_CHECKER.md`

---

## 🔗 22. Link-Validierung

### 22.1 Link-Typen zu prüfen

**Interne Links:**
- [ ] Alle relativen Pfade (`./`, `../`, `docs/...`)
- [ ] Anker-Links (`#section-name`)
- [ ] Cross-Referenzen zwischen Dokumenten

**Externe Links:**
- [ ] GitHub Repository Links
- [ ] GitHub Pages Links
- [ ] GitHub Wiki Links
- [ ] GitHub Actions Badge URLs
- [ ] Externe Ressourcen (falls vorhanden)

### 22.2 Link-Validierungs-Strategie

**Automatisierte Prüfung:**
```bash
# Empfohlenes Tool: markdown-link-check
npm install -g markdown-link-check

# Alle Markdown-Dateien prüfen
find docs -name "*.md" -exec markdown-link-check {} \;

# Alternativ: linkchecker für HTTP-Links
pip install linkchecker
linkchecker https://makr-code.github.io/ThemisDB/
```

**Manuelle Prüfung:**
- [ ] Stichproben-Prüfung kritischer Links
- [ ] Badge-URLs in README.md
- [ ] MkDocs-Navigation (`mkdocs.yml`)

### 22.3 Bekannte Link-Probleme zu beheben ⚠️ SOFORT BEHEBEN

**Identifizierte Broken Links in README.md:**

| Broken Link | Korrekter Pfad | Status |
|-------------|----------------|--------|
| `docs/ROADMAP.md` | `docs/roadmap.md` (case-sensitive) | ❌ Broken |
| `docs/TLS_SETUP.md` | Nicht gefunden - erstellen oder entfernen | ❌ Missing |
| `docs/CERTIFICATE_PINNING.md` | Nicht gefunden - erstellen oder entfernen | ❌ Missing |
| `docs/SECRETS_MANAGEMENT.md` | Nicht gefunden - erstellen oder entfernen | ❌ Missing |
| `docs/AUDIT_LOGGING.md` | Nicht gefunden - erstellen oder entfernen | ❌ Missing |
| `docs/RBAC.md` | Nicht gefunden - erstellen oder entfernen | ❌ Missing |
| `docs/SECURITY_IMPLEMENTATION_SUMMARY.md` | Nicht gefunden - erstellen oder entfernen | ❌ Missing |
| `docs/mvcc_design.md` | `docs/architecture/mvcc_design.md` | ❌ Broken |
| `docs/vector_ops.md` | `docs/features/vector_ops.md` | ❌ Broken |
| `docs/time_series.md` | `docs/features/time_series.md` | ❌ Broken |
| `docs/aql_syntax.md` | Nicht gefunden - suchen oder erstellen | ❌ Missing |
| `docs/deployment.md` | `docs/guides/deployment.md` | ❌ Broken |

**Weitere zu prüfen:**
- [ ] Links zu nicht-existierenden Dateien
- [ ] Veraltete Pfade nach Umstrukturierungen
- [ ] Broken Anchors innerhalb von Dokumenten
- [ ] GitHub Wiki vs. GitHub Pages Links

---

## 📊 23. MkDocs-Konfiguration

### 23.1 mkdocs.yml
**Pfad:** `/mkdocs.yml`

**Prüfpunkte:**
- [ ] Navigation (`nav:`) vollständig
- [ ] Alle referenzierten Dateien existieren
- [ ] Theme-Konfiguration aktuell
- [ ] Plugins funktionieren

---

## 🔄 24. Konsolidierung

### 24.1 Duplikate identifizieren
- [ ] Ähnliche Dokumente zusammenführen
- [ ] Redundante Informationen entfernen
- [ ] Einheitliche Struktur schaffen

### 24.2 Archivierung
- [ ] Veraltete Dokumente in `/docs/archive/` verschieben
- [ ] Deprecation-Hinweise hinzufügen
- [ ] Redirects/Verweise einrichten

---

## 📅 25. Priorisierung & Zeitplan

### Phase 1: Kritisch (Woche 1-2)
1. [ ] README.md vollständig überarbeiten
2. [ ] DOCUMENTATION_INDEX.md aktualisieren
3. [ ] STREAMING_ARCHITECTURE.md ⚠️ prüfen und aktualisieren
4. [ ] Alle Sharding-Dokumente prüfen
5. [ ] Link-Validierung durchführen

### Phase 2: Hoch (Woche 3-4)
6. [ ] Security-Dokumentation prüfen
7. [ ] API-Dokumentation validieren
8. [ ] Build-Anweisungen testen
9. [ ] Performance-Benchmarks aktualisieren

### Phase 3: Mittel (Woche 5-6)
10. [ ] Architecture-Dokumentation überarbeiten
11. [ ] SDK-Dokumentation aktualisieren
12. [ ] Admin-Tools-Dokumentation prüfen
13. [ ] Enterprise-Dokumentation aktualisieren

### Phase 4: Niedrig (Woche 7-8)
14. [ ] Alle übrigen Dokumente prüfen
15. [ ] Konsolidierung durchführen
16. [ ] Archivierung abschließen
17. [ ] Final Review

---

## ✅ 26. Tracking & Status

### Gesamtfortschritt
| Bereich | Dateien | Geprüft | Status |
|---------|---------|---------|--------|
| Root-Level | 6 | 0 | ⏳ Ausstehend |
| /docs | 439 | 0 | ⏳ Ausstehend |
| **GESAMT** | **445** | **0** | **0%** |

### Letzte Änderungen
- **2025-12-02**: TODO erstellt

---

## 📝 27. Hinweise für Bearbeiter

### Allgemeine Richtlinien

1. **Code-Abgleich**: Vor Dokumentations-Änderungen immer den aktuellen Code prüfen
2. **Tests**: API-Beispiele und Build-Anweisungen testen
3. **Konsistenz**: Einheitliche Terminologie aus `/docs/glossary.md` verwenden
4. **Versionierung**: Datum und Version bei Änderungen aktualisieren
5. **Review**: Änderungen von zweiter Person prüfen lassen

### Commit-Konventionen

```
docs: [Bereich] Kurzbeschreibung

Beispiele:
docs: README.md Build-Anweisungen aktualisiert
docs: sharding/STREAMING_ARCHITECTURE.md Phase 2 Features hinzugefügt
docs: security/ Link-Validierung durchgeführt
```

### Tools & Ressourcen

- **MkDocs Preview**: `./build-docs.ps1`
- **Link-Check**: `markdown-link-check`
- **Wiki-Sync**: `./sync-wiki.ps1`

---

**Erstellt:** 2. Dezember 2025  
**Verantwortlich:** ThemisDB Team  
**Nächstes Review:** Nach Abschluss Phase 1
