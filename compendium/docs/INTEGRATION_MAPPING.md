# Integration Mapping (Compendium Docs)

Dieses Dokument ist die Arbeitsgrundlage für die inhaltliche Konsolidierung von `compendium/docs/`.

## Zielbild

- Kapitelstruktur und Reihenfolge sind mit `index.md` synchron.
- Kapitel mit identischer Nummer sind klar abgegrenzt (Suffix `a/b` im Index).
- Appendix, Glossar und Referenzen sind über konsistente Links verbunden.

## Konsolidierte Kapitelzuordnung

### Teil I — Grundlagen
- `chapter_00_genesis.md`
- `chapter_01_introduction.md`
- `chapter_02_architecture.md`
- `chapter_03_multimodel.md`
- `chapter_04_installation.md`

### Teil II — Datenmodelle
- `chapter_05_relational.md`
- `chapter_06_graph.md`
- `chapter_07_document.md`
- `chapter_08_vector.md` (8a: Vektor-Modell)
- `chapter_08_storage_layer.md` (8b: Storage-Layer-Vertiefung)
- `chapter_09_timeseries.md`

### Teil III — Fach- und Spezialdomänen
- `chapter_10_enterprise.md`
- `chapter_11_realtime.md`
- `chapter_12_computervision.md`
- `chapter_13_fulltext.md`
- `chapter_14_geospatial.md`
- `chapter_15_analytics.md`

### Teil IV — KI, ML & Skalierung
- `chapter_16_ml.md` (16a: ML-Grundlagen)
- `chapter_16_sharding.md` (16b: Sharding)
- `chapter_17_llm_integration.md` (17a: LLM/RAG)
- `chapter_17_scaling.md` (17b: Skalierungs-Patterns)
- `chapter_18_ml.md` (18a: ML-Erweiterungen)
- `chapter_18_ha.md` (18b: Hochverfügbarkeit)

### Teil V — Betrieb, Performance & Zuverlässigkeit
- `chapter_19_monitoring.md` (19a)
- `chapter_19_monitoring_observability.md` (19b)
- `chapter_20_backup.md` (20a)
- `chapter_20_performance.md` (20b)
- `chapter_21_auth.md` (21a)
- `chapter_21_performance.md` (21b)
- `chapter_mvcc_hlc.md` (transversales Vertiefungskapitel)

### Teil VI — Security, Governance & Compliance
- `chapter_22_clients.md` (22a)
- `chapter_22_encryption.md` (22b)
- `chapter_36_security_hardening.md`
- `chapter_40_data_governance_compliance.md`

### Teil VII — Engineering, APIs & Best Practices
- `chapter_23_testing_qa.md`
- `chapter_24_ai_ethics.md`
- `chapter_25_devops_infrastructure.md`
- `chapter_26_migration_legacy.md`
- `chapter_27_troubleshooting.md`
- `chapter_28_aql_reference.md`
- `chapter_29_analytics_process_mining.md`
- `chapter_30_deployment_operations.md`
- `chapter_31_api_protocols.md`
- `chapter_32_api_design_rest_principles.md` (32a)
- `chapter_32_aql_oop_implementation.md` (32b)
- `chapter_33_best_practices.md`
- `chapter_34_query_optimization.md`
- `chapter_35_data_modeling_patterns.md`
- `chapter_37_ecosystem_integration.md`
- `chapter_38_observability_sre.md`
- `chapter_39_performance_tuning_cookbook.md`
- `chapter_41_hands_on_labs.md`
- `chapter_42_docs_assistant_usage.md`

## Appendix / Glossar / Referenzen (synchronisiert)

- `appendix_literatur.md` (Anhang A, Referenzen)
- `appendix_d_feature_status.md`
- `appendix_e_incident_runbooks.md`
- `appendix_f_aql_cheatsheet.md`
- `appendix_g_configuration.md`
- `appendix_h_glossary.md`
- `appendix_i_troubleshooting.md`

## Abgrenzung überlappender Kapitel

- Kapitelnummern mit Doppelnutzung (`08`, `16`, `17`, `18`, `19`, `20`, `21`, `22`, `32`) werden im Index mit `a/b` präzisiert.
- `chapter_mvcc_hlc.md` bleibt eigenständig als querliegendes Vertiefungskapitel für Konsistenz/Transaktionen.

## Verbindliche Review-/Audit-Referenzen

Die Konsolidierung wurde gegen folgende Referenzdokumente abgeglichen:
- `docs/DOCUMENTATION_REVIEW_GUIDELINES.md`
- `docs/SYSTEMATISCHER_REVIEWPLAN.md`
- `docs/PR_DOCUMENTATION_CHECKLIST.md`
- `docs/de/development/SOURCE_CODE_AUDIT.md`
- `docs/audit-framework/AUDIT_RUNBOOK.md`

## Review-/Audit-Nachweis (Issue-Abschluss)

- Fachreview (Dokumentationschecklisten) wurde durchgeführt und in den aktualisierten Dateien berücksichtigt: `index.md`, `preface.md`, `cover.md`, `cover_book.md`, `INTEGRATION_MAPPING.md`, `test_links_example.md`.
- Dokumentationsaudit wurde für Kapitelreihenfolge, Doppelkapitel-Abgrenzung, Appendix-/Glossar-Synchronisation und interne Linkkonsistenz durchgeführt.
- Relevante Bereiche sind in diesem Mapping explizit gelistet (Teile I–VII, Anhänge A/D/E/F/G/H/I).
