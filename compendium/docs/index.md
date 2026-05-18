# ThemisDB: Das vollständige Handbuch

**Version 1.8.0** | **Mai 2026**

Dieses Buch ist die narrative Gesamtdokumentation für ThemisDB. Die Struktur unten ist die verbindliche Navigationsbasis für `compendium/docs/` und grenzt Kapitel mit ähnlicher Nummerierung explizit ab (z. B. `16a/16b`, `21a/21b`).

---

## Inhaltsverzeichnis (konsolidiert)

### Teil I — Grundlagen

| Kapitel | Titel |
|---------|-------|
| [Vorwort](preface.md) | Warum ThemisDB? Vision und Entstehung |
| [Kapitel 0](chapter_00_genesis.md) | Genesis — Entstehungsgeschichte |
| [Kapitel 1](chapter_01_introduction.md) | Einführung in ThemisDB |
| [Kapitel 2](chapter_02_architecture.md) | System-Architektur |
| [Kapitel 3](chapter_03_multimodel.md) | Multi-Model Überblick |
| [Kapitel 4](chapter_04_installation.md) | Installation & Erstkonfiguration |

### Teil II — Datenmodelle

| Kapitel | Titel |
|---------|-------|
| [Kapitel 5](chapter_05_relational.md) | Relationales Modell |
| [Kapitel 6](chapter_06_graph.md) | Graph-Datenmodell |
| [Kapitel 7](chapter_07_document.md) | Dokumentenmodell |
| [Kapitel 8a](chapter_08_vector.md) | Vektor-Modell (ANN/HNSW) |
| [Kapitel 8b](chapter_08_storage_layer.md) | Storage Layer für Multi-Model-Workloads |
| [Kapitel 9](chapter_09_timeseries.md) | Time-Series & Bi-Temporal |

### Teil III — Fach- und Spezialdomänen

| Kapitel | Titel |
|---------|-------|
| [Kapitel 10](chapter_10_enterprise.md) | Enterprise-Features |
| [Kapitel 11](chapter_11_realtime.md) | Realtime & Streaming |
| [Kapitel 12](chapter_12_computervision.md) | Computer Vision |
| [Kapitel 13](chapter_13_fulltext.md) | Full-Text & Hybrid Search |
| [Kapitel 14](chapter_14_geospatial.md) | Geospatial |
| [Kapitel 15](chapter_15_analytics.md) | Analytics |

### Teil IV — KI, ML & Skalierung

| Kapitel | Titel |
|---------|-------|
| [Kapitel 16a](chapter_16_ml.md) | Machine-Learning Grundlagen |
| [Kapitel 16b](chapter_16_sharding.md) | Sharding-Strategien |
| [Kapitel 17a](chapter_17_llm_integration.md) | LLM-Integration & RAG |
| [Kapitel 17b](chapter_17_scaling.md) | Skalierungs-Patterns |
| [Kapitel 18a](chapter_18_ml.md) | ML-Erweiterungen |
| [Kapitel 18b](chapter_18_ha.md) | Hochverfügbarkeit |

### Teil V — Betrieb, Performance & Zuverlässigkeit

| Kapitel | Titel |
|---------|-------|
| [Kapitel 19a](chapter_19_monitoring.md) | Monitoring |
| [Kapitel 19b](chapter_19_monitoring_observability.md) | Observability Deep Dive |
| [Kapitel 20a](chapter_20_backup.md) | Backup & Recovery |
| [Kapitel 20b](chapter_20_performance.md) | Performance-Referenz |
| [Kapitel 21a](chapter_21_auth.md) | Authentifizierung |
| [Kapitel 21b](chapter_21_performance.md) | Performance in Security-Pfaden |
| [Kapitel MVCC](chapter_mvcc_hlc.md) | MVCC, HLC, Transaktionen |

### Teil VI — Security, Governance & Compliance

| Kapitel | Titel |
|---------|-------|
| [Kapitel 22a](chapter_22_clients.md) | Clients & SDKs |
| [Kapitel 22b](chapter_22_encryption.md) | Verschlüsselung |
| [Kapitel 36](chapter_36_security_hardening.md) | Security Hardening |
| [Kapitel 40](chapter_40_data_governance_compliance.md) | Data Governance & Compliance |

### Teil VII — Engineering, APIs & Best Practices

| Kapitel | Titel |
|---------|-------|
| [Kapitel 23](chapter_23_testing_qa.md) | Testing & QA |
| [Kapitel 24](chapter_24_ai_ethics.md) | AI Ethics |
| [Kapitel 25](chapter_25_devops_infrastructure.md) | DevOps & Infrastructure |
| [Kapitel 26](chapter_26_migration_legacy.md) | Migration & Legacy |
| [Kapitel 27](chapter_27_troubleshooting.md) | Troubleshooting |
| [Kapitel 28](chapter_28_aql_reference.md) | AQL-Referenz |
| [Kapitel 29](chapter_29_analytics_process_mining.md) | Analytics & Process Mining |
| [Kapitel 30](chapter_30_deployment_operations.md) | Deployment & Operations |
| [Kapitel 31](chapter_31_api_protocols.md) | API & Protokolle |
| [Kapitel 32a](chapter_32_api_design_rest_principles.md) | REST/API-Design |
| [Kapitel 32b](chapter_32_aql_oop_implementation.md) | AQL-OOP-Implementierung |
| [Kapitel 33](chapter_33_best_practices.md) | Best Practices |
| [Kapitel 34](chapter_34_query_optimization.md) | Query-Optimierung |
| [Kapitel 35](chapter_35_data_modeling_patterns.md) | Data-Modeling Patterns |
| [Kapitel 37](chapter_37_ecosystem_integration.md) | Ecosystem Integration |
| [Kapitel 38](chapter_38_observability_sre.md) | Observability & SRE |
| [Kapitel 39](chapter_39_performance_tuning_cookbook.md) | Performance Tuning Cookbook |
| [Kapitel 41](chapter_41_hands_on_labs.md) | Hands-on Labs |
| [Kapitel 42](chapter_42_docs_assistant_usage.md) | Docs-Assistent Nutzung |

### Anhänge

| Anhang | Titel |
|--------|-------|
| [Anhang A](appendix_literatur.md) | Literatur & Referenzen |
| [Anhang D](appendix_d_feature_status.md) | Feature-Status |
| [Anhang E](appendix_e_incident_runbooks.md) | Incident Runbooks |
| [Anhang F](appendix_f_aql_cheatsheet.md) | AQL Cheatsheet |
| [Anhang G](appendix_g_configuration.md) | Konfigurationsreferenz |
| [Anhang H](appendix_h_glossary.md) | Glossar |
| [Anhang I](appendix_i_troubleshooting.md) | Troubleshooting-Index |

---

## Synchronisationshinweis

Die Kapitelreihenfolge, Abgrenzung überlappender Kapitelnummern, Appendix-Referenzen sowie Integrationsmapping sind synchronisiert mit:
- `compendium/docs/INTEGRATION_MAPPING.md`
- `compendium/mkdocs-nav.yml`
- `compendium/docs/appendix_h_glossary.md`
- `compendium/docs/appendix_literatur.md`

## Kanonische Einstiegspunkte vs. unterstützende Dateien

### Kanonische Einstiegspunkte
- `index.md` (diese Datei): verbindliches Inhaltsverzeichnis und Navigationsquelle.
- `preface.md`: inhaltlicher Einstieg vor den Kapiteln.

### Unterstützende Dateien (nicht-kanonisch für Kapitelnavigation)
- `cover_book.md`: primäres Cover-Template für die PDF/HTML-Generierung.
- `cover.md`: vereinfachte Cover-Variante für alternative Renderpfade.
- `INTEGRATION_MAPPING.md`: redaktionelles Synchronisations- und Audit-Dokument.
- `test_links_example.md`: Link-Validierungsbeispiele, kein Kapitelinhalt.
