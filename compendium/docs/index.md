# ThemisDB: Das vollständige Handbuch

**Version 1.8.0** | **April 2026**

Dieses Buch ist die narrative Gesamtdoku für ThemisDB: weniger Überschriften, mehr Fließtext. Es führt Sie von den Grundlagen bis zur Mastery, mit echten Beispielen und klaren Handlungsanweisungen.

Sie finden hier keine fragmentierten 700 Einzeltexte, sondern einen geführten Weg: zuerst verstehen, dann anwenden, dann optimieren. Konzepte, Design-Entscheidungen und Best Practices werden stets mit vollständigen, lauffähigen Beispielen verknüpft.

**Für wen?** Einsteiger erhalten einen sanften Einstieg; Entwickler und Architekten bekommen belastbare Patterns; Admins und SREs finden Betrieb, Skalierung und Sicherheit kompakt zusammengeführt.

**Wie lesen?** Am Stück (empfohlen) für den roten Faden. Oder kapitelweise springen: Multi-Model in Teil II (Kap. 5–9), Production-Ready in Teil V (Kap. 17–21), Security in Teil VI (Kap. 21, 36, 40). Alle Beispielprojekte sind direkt referenziert.

**Struktur auf einen Blick:** Grundlagen, Datenmodelle, Spezialanwendungen, Erweiterte Features, Skalierung, Sicherheit, Entwicklung, Best Practices, Anhänge. Jedes Kapitel folgt dem Muster Überblick → Theorie → Praxis → Patterns → Performance → Takeaways.

**Ressourcen:** [GitHub](https://github.com/makr-code/ThemisDB) · [Issues](https://github.com/makr-code/ThemisDB/issues) · [Discussions](https://github.com/makr-code/ThemisDB/discussions) · [QUICKSTART.md](../../QUICKSTART.md). Feedback ist willkommen – jede Korrektur hilft.

Hinweise und Code sind konsistent formatiert; AQL- und Python-Snippets zeigen realistische Aufrufe. Hervorhebungen bleiben sparsam: fett für Schlüsselbegriffe, Inline-Code für Befehle und Dateinamen.

Los geht's: Direkt ins Vorwort für Kontext, oder in Kapitel 1 für den Einstieg in ThemisDB.

---

## Inhaltsverzeichnis

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
| [Kapitel 5](chapter_05_relational.md) | Relationales Modell & SQL-Kompatibilität |
| [Kapitel 6](chapter_06_graph.md) | Graph-Datenbank (BFS/DFS/A*/Dijkstra, Distributed) |
| [Kapitel 7](chapter_07_document.md) | Dokument-Management (PDF/Office/OCR, 10-Stage Pipeline) |
| [Kapitel 8](chapter_08_storage_layer.md) | Storage Layer (RocksDB, MVCC, WAL, Cache, Scheduler) |
| [Kapitel 8b](chapter_08_vector.md) | Vector-Datenbank (HNSW, ANN, GPU-beschleunigt) |
| [Kapitel 9](chapter_09_timeseries.md) | Time-Series & Bi-Temporal (System-Versioning, Time-Travel) |

### Teil III — Spezialanwendungen

| Kapitel | Titel |
|---------|-------|
| [Kapitel 10](chapter_10_enterprise.md) | Enterprise-Features (Multi-Tenancy, RBAC, Audit) |
| [Kapitel 11](chapter_11_realtime.md) | Real-Time: CDC, Ingestion, Changefeeds |
| [Kapitel 12](chapter_12_computervision.md) | Computer Vision & Bildanalyse |
| [Kapitel 13](chapter_13_fulltext.md) | Full-Text & Hybrid Search (BM25+Vector, RRF) |
| [Kapitel 14](chapter_14_geospatial.md) | Geospatial (WGS-84, R-Tree, GPU Clustering) |
| [Kapitel 15](chapter_15_analytics.md) | Analytics (OLAP, CEP, Anomaly Detection, ONNX) |

### Teil IV — KI / LLM Integration

| Kapitel | Titel |
|---------|-------|
| [Kapitel 16 (ML)](chapter_16_ml.md) | Machine Learning: Training, Exporters, Importers |
| [Kapitel 16 (Sharding)](chapter_16_sharding.md) | Horizontal Scaling & Sharding |
| [Kapitel 17](chapter_17_llm_integration.md) | LLM Integration: llama.cpp, RAG, LoRA, Voice, Prompt Engineering |
| [Kapitel 18 (ML)](chapter_18_ml.md) | ML-Erweiterungen |

### Teil V — Produktion & Skalierung

| Kapitel | Titel |
|---------|-------|
| [Kapitel 17b](chapter_17_scaling.md) | Scaling-Patterns |
| [Kapitel 18 (HA)](chapter_18_ha.md) | High Availability: Replication, WAL-Archival, Chaos, Failover |
| [Kapitel 19](chapter_19_monitoring.md) | Monitoring & Alerting |
| [Kapitel 19b](chapter_19_monitoring_observability.md) | Observability-Deep-Dive |
| [Kapitel 20](chapter_20_backup.md) | Backup & PITR |
| [Kapitel 20b](chapter_20_performance.md) | Performance-Referenz |
| [Kapitel MVCC](chapter_mvcc_hlc.md) | MVCC, HLC, Transaktionen, SAGA, Deadlock-Prediction |

### Teil VI — Sicherheit & Compliance

| Kapitel | Titel |
|---------|-------|
| [Kapitel 21](chapter_21_auth.md) | Authentication (JWT/OAuth2/SAML/WebAuthn/LDAP/MFA/OIDC) |
| [Kapitel 21b](chapter_21_performance.md) | Performance-Optimierung in Security-Pfaden |
| [Kapitel 22](chapter_22_clients.md) | Client-Bibliotheken & SDK |
| [Kapitel 22b](chapter_22_encryption.md) | Encryption (AES-256-GCM, DEK/KEK/MasterKey, HSM) |
| [Kapitel 36](chapter_36_security_hardening.md) | Security Hardening (RLS, Zero Trust, Field Encryption) |
| [Kapitel 40](chapter_40_data_governance_compliance.md) | Data Governance & Compliance (GDPR/HIPAA/CCPA/PCI/SOC2) |

### Teil VII — Entwicklung & Betrieb

| Kapitel | Titel |
|---------|-------|
| [Kapitel 23](chapter_23_testing_qa.md) | Testing & QA |
| [Kapitel 24](chapter_24_ai_ethics.md) | KI-Ethik & Responsible AI |
| [Kapitel 25](chapter_25_devops_infrastructure.md) | DevOps: Updates, Canary, Blue/Green, Schema Migration |
| [Kapitel 26](chapter_26_migration_legacy.md) | Migration von Legacy-Systemen |
| [Kapitel 27](chapter_27_troubleshooting.md) | Troubleshooting |
| [Kapitel 28](chapter_28_aql_reference.md) | AQL-Referenz |
| [Kapitel 29](chapter_29_analytics_process_mining.md) | Analytics & Process Mining |
| [Kapitel 30](chapter_30_deployment_operations.md) | Deployment & Betrieb |
| [Kapitel 31](chapter_31_api_protocols.md) | API & Protokolle (Wire V2, QUIC, gRPC, UDP) |
| [Kapitel 32 (API)](chapter_32_api_design_rest_principles.md) | REST API Design |
| [Kapitel 32 (AQL OOP)](chapter_32_aql_oop_implementation.md) | AQL OOP-Implementierung |
| [Kapitel 33](chapter_33_best_practices.md) | Best Practices |
| [Kapitel 34](chapter_34_query_optimization.md) | Query-Optimierung (Adaptive Optimizer, Index Management) |
| [Kapitel 35](chapter_35_data_modeling_patterns.md) | Datenmodellierungs-Patterns |
| [Kapitel 37](chapter_37_ecosystem_integration.md) | Ecosystem Integration (Chimera, Plugins) |
| [Kapitel 38](chapter_38_observability_sre.md) | Observability & SRE (Prometheus, OpenTelemetry) |
| [Kapitel 39](chapter_39_performance_tuning_cookbook.md) | Performance-Tuning Cookbook |
| [Kapitel 41](chapter_41_hands_on_labs.md) | Hands-On Labs |
| [Kapitel 42](chapter_42_docs_assistant_usage.md) | Docs-Assistent |

### Anhänge

| Anhang | Titel |
|--------|-------|
| [Appendix D](appendix_d_feature_status.md) | Feature Status Matrix (v1.8.0) |
| [Appendix E](appendix_e_incident_runbooks.md) | Incident Runbooks |
| [Appendix F](appendix_f_aql_cheatsheet.md) | AQL Cheatsheet |
| [Appendix G](appendix_g_configuration.md) | Konfigurationsreferenz |
| [Appendix H](appendix_h_glossary.md) | Glossar |
| [Appendix I](appendix_i_troubleshooting.md) | Troubleshooting-Index |
| [Literatur](appendix_literatur.md) | Literatur & Referenzen |
