## Ziel
Rekursive Kondensierung und Aktualisierung aller Entwickler-Dokumente in `src/include/examples/tools/benchmarks/tests` auf Basis des echten Sourcecodes. Anschließend Update aller Root-Einstiege/Indexe auf denselben Stand.

## Scope (Ist-Stand im Repo)
- src: 422 MD-Dateien
- include: 392 MD-Dateien
- examples: 320 MD-Dateien
- tools: 171 MD-Dateien
- benchmarks: 165 MD-Dateien
- tests: 145 MD-Dateien
- Gesamt: 1615 MD-Dateien

## Pflicht: Rekursiv kondensieren bis Root-Ebene
- Innerhalb jedes Moduls Inhalte verdichten (Dubletten reduzieren, veraltete Abschnitte entfernen/markieren, Querverweise zusammenziehen).
- Ergebnis stufenweise nach oben propagieren: Modulordner -> Bereichs-README/INDEX -> Root-Doku.
- Root-Doku darf nach Abschluss keine alten Aussagen/Pfade zu den bearbeiteten Modulen enthalten.

## Exakte Dateinamen und Arbeitsauftrag je Dateiname
Die folgenden Dateinamen kommen im Scope vor und sind jeweils mit der definierten Arbeitsaufgabe zu bearbeiten:

| Dateiname | Erwarteter Inhalt | Arbeitsaufgabe |
|---|---|---|
| ADMIN_GUIDE.md | Schritt-fuer-Schritt-Kommandos, Voraussetzungen, erwartete Ergebnisse. | Kommandos dry-run-pruefen und auf aktuellen Build/Test-Flow korrigieren. |
| ADVANCED_BENCHMARK_RESEARCH.md | Reproduzierbare Methodik, Parameter, Metriken, Limits. | Methodik reproduzierbar machen; alte Zahlen als historisch markieren. |
| ADVANCED_BENCHMARKS_ANALYSIS.md | Reproduzierbare Methodik, Parameter, Metriken, Limits. | Methodik reproduzierbar machen; alte Zahlen als historisch markieren. |
| ADVANCED_BENCHMARKS_GUIDE.md | Schritt-fuer-Schritt-Kommandos, Voraussetzungen, erwartete Ergebnisse. | Kommandos dry-run-pruefen und auf aktuellen Build/Test-Flow korrigieren. |
| ADVANCED_FEATURES_README.md | Zweck, Modulgrenzen, Abhaengigkeiten, Build/Run/Test, Einstieg. | Mit aktuellem Modulcode synchronisieren; falsche Pfade/Kommandos korrigieren. |
| ALGORITHM.md | Dateizweck klar; Aussagen nur mit Codebezug. | Inhalt rekursiv validieren, verdichten und klare ToDos hinterlassen. |
| ANALYTICS_PIPELINE.md | Dateizweck klar; Aussagen nur mit Codebezug. | Inhalt rekursiv validieren, verdichten und klare ToDos hinterlassen. |
| API_INTEGRATION.md | Signaturen/Parameter/Fehlerfaelle gegen reale Endpunkte/Typen. | Signaturen und Fehlerfaelle gegen Implementierung pruefen und korrigieren. |
| API_REFERENCE.md | Signaturen/Parameter/Fehlerfaelle gegen reale Endpunkte/Typen. | Signaturen und Fehlerfaelle gegen Implementierung pruefen und korrigieren. |
| API_USAGE.md | Signaturen/Parameter/Fehlerfaelle gegen reale Endpunkte/Typen. | Signaturen und Fehlerfaelle gegen Implementierung pruefen und korrigieren. |
| AQL_QUERY_PATTERNS.md | Dateizweck klar; Aussagen nur mit Codebezug. | Inhalt rekursiv validieren, verdichten und klare ToDos hinterlassen. |
| ARCHITECTURE_COMPARISON.md | Komponenten, Datenfluss, zentrale Typen/Funktionen, Schnittstellen. | Architekturtext auf reale Klassen/Namespaces/Funktionen anpassen. |
| ARCHITECTURE.md | Komponenten, Datenfluss, zentrale Typen/Funktionen, Schnittstellen. | Architekturtext auf reale Klassen/Namespaces/Funktionen anpassen. |
| AUDIT.md | Soll/Ist gegen Code mit Evidenzen, Risiken und Abweichungen. | Jeden Befund mit Codebeleg versehen; veraltete Befunde entfernen. |
| baugenehmigung.md | Dateizweck klar; Aussagen nur mit Codebezug. | Inhalt rekursiv validieren, verdichten und klare ToDos hinterlassen. |
| BENCH_LORA_AUTO_BINDING_README.md | Zweck, Modulgrenzen, Abhaengigkeiten, Build/Run/Test, Einstieg. | Mit aktuellem Modulcode synchronisieren; falsche Pfade/Kommandos korrigieren. |
| BENCH_RAG_ETHICS_README.md | Zweck, Modulgrenzen, Abhaengigkeiten, Build/Run/Test, Einstieg. | Mit aktuellem Modulcode synchronisieren; falsche Pfade/Kommandos korrigieren. |
| BENCHMARK_ANALYSIS_20251210.md | Reproduzierbare Methodik, Parameter, Metriken, Limits. | Methodik reproduzierbar machen; alte Zahlen als historisch markieren. |
| BENCHMARK_ANALYSIS_20251228.md | Reproduzierbare Methodik, Parameter, Metriken, Limits. | Methodik reproduzierbar machen; alte Zahlen als historisch markieren. |
| BENCHMARK_ANALYSIS_AI_IMAGERY.md | Reproduzierbare Methodik, Parameter, Metriken, Limits. | Methodik reproduzierbar machen; alte Zahlen als historisch markieren. |
| BENCHMARK_CORRECTION.md | Reproduzierbare Methodik, Parameter, Metriken, Limits. | Methodik reproduzierbar machen; alte Zahlen als historisch markieren. |
| BENCHMARK_DETAILED_RESULTS.md | Reproduzierbare Methodik, Parameter, Metriken, Limits. | Methodik reproduzierbar machen; alte Zahlen als historisch markieren. |
| BENCHMARK_INDEX.md | Reproduzierbare Methodik, Parameter, Metriken, Limits. | Methodik reproduzierbar machen; alte Zahlen als historisch markieren. |
| benchmark_protocol_20251204_122427.md | Reproduzierbare Methodik, Parameter, Metriken, Limits. | Methodik reproduzierbar machen; alte Zahlen als historisch markieren. |
| BENCHMARK_PROTOCOLS_FINAL_REPORT.md | Reproduzierbare Methodik, Parameter, Metriken, Limits. | Methodik reproduzierbar machen; alte Zahlen als historisch markieren. |
| BENCHMARK_PROTOCOLS_SUMMARY.md | Reproduzierbare Methodik, Parameter, Metriken, Limits. | Methodik reproduzierbar machen; alte Zahlen als historisch markieren. |
| BENCHMARK_QUICK_START.md | Reproduzierbare Methodik, Parameter, Metriken, Limits. | Methodik reproduzierbar machen; alte Zahlen als historisch markieren. |
| BENCHMARK_RESULTS.md | Reproduzierbare Methodik, Parameter, Metriken, Limits. | Methodik reproduzierbar machen; alte Zahlen als historisch markieren. |
| BENCHMARK_STATUS.md | Reproduzierbare Methodik, Parameter, Metriken, Limits. | Methodik reproduzierbar machen; alte Zahlen als historisch markieren. |
| BENCHMARK_SUITE_EXECUTIVE_SUMMARY.md | Reproduzierbare Methodik, Parameter, Metriken, Limits. | Methodik reproduzierbar machen; alte Zahlen als historisch markieren. |
| BENCHMARK_SYSTEM_CERTIFICATION.md | Reproduzierbare Methodik, Parameter, Metriken, Limits. | Methodik reproduzierbar machen; alte Zahlen als historisch markieren. |
| BENCHMARK_VISUALIZATION.md | Reproduzierbare Methodik, Parameter, Metriken, Limits. | Methodik reproduzierbar machen; alte Zahlen als historisch markieren. |
| BENCHMARKS_DE.md | Reproduzierbare Methodik, Parameter, Metriken, Limits. | Methodik reproduzierbar machen; alte Zahlen als historisch markieren. |
| BENCHMARKS_DOCUMENTATION_INDEX.md | Reproduzierbare Methodik, Parameter, Metriken, Limits. | Methodik reproduzierbar machen; alte Zahlen als historisch markieren. |
| BENCHMARKS_EXECUTIVE_SUMMARY.md | Reproduzierbare Methodik, Parameter, Metriken, Limits. | Methodik reproduzierbar machen; alte Zahlen als historisch markieren. |
| BENCHMARKS_MASTER_INDEX.md | Reproduzierbare Methodik, Parameter, Metriken, Limits. | Methodik reproduzierbar machen; alte Zahlen als historisch markieren. |
| BEST_PRACTICES_REVIEW.md | Soll/Ist gegen Code mit Evidenzen, Risiken und Abweichungen. | Jeden Befund mit Codebeleg versehen; veraltete Befunde entfernen. |
| bimschg.md | Dateizweck klar; Aussagen nur mit Codebezug. | Inhalt rekursiv validieren, verdichten und klare ToDos hinterlassen. |
| BUDGET_GUIDE.md | Schritt-fuer-Schritt-Kommandos, Voraussetzungen, erwartete Ergebnisse. | Kommandos dry-run-pruefen und auf aktuellen Build/Test-Flow korrigieren. |
| BUILD_INSTRUCTIONS.md | Dateizweck klar; Aussagen nur mit Codebezug. | Inhalt rekursiv validieren, verdichten und klare ToDos hinterlassen. |
| CACHE_STRATEGIES_README.md | Zweck, Modulgrenzen, Abhaengigkeiten, Build/Run/Test, Einstieg. | Mit aktuellem Modulcode synchronisieren; falsche Pfade/Kommandos korrigieren. |
| CEP_PATTERNS.md | Dateizweck klar; Aussagen nur mit Codebezug. | Inhalt rekursiv validieren, verdichten und klare ToDos hinterlassen. |
| CHANGELOG.md | Reale Aenderungen mit Datum, Wirkung und Migrationshinweis. | Nur verifizierte Eintraege behalten; Dubletten bereinigen. |
| CHIMERA_README.md | Zweck, Modulgrenzen, Abhaengigkeiten, Build/Run/Test, Einstieg. | Mit aktuellem Modulcode synchronisieren; falsche Pfade/Kommandos korrigieren. |
| COMPLETE_BENCHMARK_FRAMEWORK.md | Reproduzierbare Methodik, Parameter, Metriken, Limits. | Methodik reproduzierbar machen; alte Zahlen als historisch markieren. |
| COMPLETION_REPORT.md | Verifizierter Ist-Stand, Delta, offene Punkte, Quellen. | Ist-Stand verifizieren; veraltete Reports in Historie markieren/verschieben. |
| COMPREHENSIVE_BENCHMARK_GUIDE.md | Schritt-fuer-Schritt-Kommandos, Voraussetzungen, erwartete Ergebnisse. | Kommandos dry-run-pruefen und auf aktuellen Build/Test-Flow korrigieren. |
| CONFIG_README.md | Zweck, Modulgrenzen, Abhaengigkeiten, Build/Run/Test, Einstieg. | Mit aktuellem Modulcode synchronisieren; falsche Pfade/Kommandos korrigieren. |
| CONFIGURATION.md | Dateizweck klar; Aussagen nur mit Codebezug. | Inhalt rekursiv validieren, verdichten und klare ToDos hinterlassen. |
| CONSOLIDATION_COMPLETE.md | Verifizierter Ist-Stand, Delta, offene Punkte, Quellen. | Ist-Stand verifizieren; veraltete Reports in Historie markieren/verschieben. |
| CPP_INTEGRATION_GUIDE.md | Schritt-fuer-Schritt-Kommandos, Voraussetzungen, erwartete Ergebnisse. | Kommandos dry-run-pruefen und auf aktuellen Build/Test-Flow korrigieren. |
| CRUD_BENCHMARK_ANALYSIS_20251210.md | Reproduzierbare Methodik, Parameter, Metriken, Limits. | Methodik reproduzierbar machen; alte Zahlen als historisch markieren. |
| DATA_MODEL.md | Felder/Typen/Constraints/Indizes und Migrationen. | Schemafelder und Constraints gegen reale Struktur verifizieren. |
| DATABASE_MATRIX.md | Dateizweck klar; Aussagen nur mit Codebezug. | Inhalt rekursiv validieren, verdichten und klare ToDos hinterlassen. |
| DELIVERY_SUMMARY.md | Verifizierter Ist-Stand, Delta, offene Punkte, Quellen. | Ist-Stand verifizieren; veraltete Reports in Historie markieren/verschieben. |
| DEPLOYMENT_README.md | Zweck, Modulgrenzen, Abhaengigkeiten, Build/Run/Test, Einstieg. | Mit aktuellem Modulcode synchronisieren; falsche Pfade/Kommandos korrigieren. |
| DEPLOYMENT_STATUS_REPORT.md | Verifizierter Ist-Stand, Delta, offene Punkte, Quellen. | Ist-Stand verifizieren; veraltete Reports in Historie markieren/verschieben. |
| DEPLOYMENT.md | Deploy/Rollback, Konfiguration, Betriebschecks. | Deploy-/Rollback-Anleitung an reale Skripte/Targets angleichen. |
| DESIGN.md | Dateizweck klar; Aussagen nur mit Codebezug. | Inhalt rekursiv validieren, verdichten und klare ToDos hinterlassen. |
| DISABLED_BENCHMARK_POLICY.md | Reproduzierbare Methodik, Parameter, Metriken, Limits. | Methodik reproduzierbar machen; alte Zahlen als historisch markieren. |
| DOCKER_BENCHMARKS_STATUS_REPORT.md | Reproduzierbare Methodik, Parameter, Metriken, Limits. | Methodik reproduzierbar machen; alte Zahlen als historisch markieren. |
| docker_benchmarks_suite_index.md | Reproduzierbare Methodik, Parameter, Metriken, Limits. | Methodik reproduzierbar machen; alte Zahlen als historisch markieren. |
| DOCKER_BENCHMARKS_UNIFIED_INTEGRATION.md | Reproduzierbare Methodik, Parameter, Metriken, Limits. | Methodik reproduzierbar machen; alte Zahlen als historisch markieren. |
| DOCKER_COMPARATIVE_BENCHMARKS_README.md | Zweck, Modulgrenzen, Abhaengigkeiten, Build/Run/Test, Einstieg. | Mit aktuellem Modulcode synchronisieren; falsche Pfade/Kommandos korrigieren. |
| DOCKER_QUICKSTART.md | Schritt-fuer-Schritt-Kommandos, Voraussetzungen, erwartete Ergebnisse. | Kommandos dry-run-pruefen und auf aktuellen Build/Test-Flow korrigieren. |
| DOCKER_RAID_BENCHMARK_SUITE_README.md | Zweck, Modulgrenzen, Abhaengigkeiten, Build/Run/Test, Einstieg. | Mit aktuellem Modulcode synchronisieren; falsche Pfade/Kommandos korrigieren. |
| DOCKER_RAID_IMPLEMENTATION_SUMMARY.md | Verifizierter Ist-Stand, Delta, offene Punkte, Quellen. | Ist-Stand verifizieren; veraltete Reports in Historie markieren/verschieben. |
| DOCKER_RAID_PERFORMANCE_REPORT.md | Reproduzierbare Methodik, Parameter, Metriken, Limits. | Methodik reproduzierbar machen; alte Zahlen als historisch markieren. |
| DOCUMENTATION_STATUS.md | Verifizierter Ist-Stand, Delta, offene Punkte, Quellen. | Ist-Stand verifizieren; veraltete Reports in Historie markieren/verschieben. |
| embed_certificate_README.md | Zweck, Modulgrenzen, Abhaengigkeiten, Build/Run/Test, Einstieg. | Mit aktuellem Modulcode synchronisieren; falsche Pfade/Kommandos korrigieren. |
| EMBEDDINGS_GUIDE.md | Schritt-fuer-Schritt-Kommandos, Voraussetzungen, erwartete Ergebnisse. | Kommandos dry-run-pruefen und auf aktuellen Build/Test-Flow korrigieren. |
| ENTERPRISE_INTEGRATION_GUIDE.md | Schritt-fuer-Schritt-Kommandos, Voraussetzungen, erwartete Ergebnisse. | Kommandos dry-run-pruefen und auf aktuellen Build/Test-Flow korrigieren. |
| ENTERPRISE_SUITE_README.md | Zweck, Modulgrenzen, Abhaengigkeiten, Build/Run/Test, Einstieg. | Mit aktuellem Modulcode synchronisieren; falsche Pfade/Kommandos korrigieren. |
| ENTERPRISE_SUMMARY.md | Verifizierter Ist-Stand, Delta, offene Punkte, Quellen. | Ist-Stand verifizieren; veraltete Reports in Historie markieren/verschieben. |
| ETHICAL_AI_FRAMEWORK.md | Dateizweck klar; Aussagen nur mit Codebezug. | Inhalt rekursiv validieren, verdichten und klare ToDos hinterlassen. |
| ETHICS_AI_BEST_PRACTICES.md | Dateizweck klar; Aussagen nur mit Codebezug. | Inhalt rekursiv validieren, verdichten und klare ToDos hinterlassen. |
| ETHICS_AI_LITERATURE_REVIEW.md | Soll/Ist gegen Code mit Evidenzen, Risiken und Abweichungen. | Jeden Befund mit Codebeleg versehen; veraltete Befunde entfernen. |
| ETHICS_AI_ROADMAP.md | Konkrete umsetzbare Tasks mit Phasen und Akzeptanzkriterien. | Vage Punkte in umsetzbare Checkbox-Tasks ueberfuehren (Design->Core->Edge->Tests->Hardening->Doku). |
| ETHICS_EVALUATION_METRICS_README.md | Zweck, Modulgrenzen, Abhaengigkeiten, Build/Run/Test, Einstieg. | Mit aktuellem Modulcode synchronisieren; falsche Pfade/Kommandos korrigieren. |
| ETHICS_MONITORING_DASHBOARD_README.md | Zweck, Modulgrenzen, Abhaengigkeiten, Build/Run/Test, Einstieg. | Mit aktuellem Modulcode synchronisieren; falsche Pfade/Kommandos korrigieren. |
| EXECUTIVE_SUMMARY_REAL_VERIFICATION.md | Verifizierter Ist-Stand, Delta, offene Punkte, Quellen. | Ist-Stand verifizieren; veraltete Reports in Historie markieren/verschieben. |
| EXPANSION_PROPOSAL.md | Dateizweck klar; Aussagen nur mit Codebezug. | Inhalt rekursiv validieren, verdichten und klare ToDos hinterlassen. |
| EXTENDED_BENCHMARK_ANALYSIS.md | Reproduzierbare Methodik, Parameter, Metriken, Limits. | Methodik reproduzierbar machen; alte Zahlen als historisch markieren. |
| FILE_INVENTORY.md | Dateizweck klar; Aussagen nur mit Codebezug. | Inhalt rekursiv validieren, verdichten und klare ToDos hinterlassen. |
| FINAL_BENCHMARK_REPORT.md | Reproduzierbare Methodik, Parameter, Metriken, Limits. | Methodik reproduzierbar machen; alte Zahlen als historisch markieren. |
| FINAL_SUMMARY.md | Verifizierter Ist-Stand, Delta, offene Punkte, Quellen. | Ist-Stand verifizieren; veraltete Reports in Historie markieren/verschieben. |
| FUTURE_ENHANCEMENTS.md | Messbare Erweiterungen inkl. Constraints, Interfaces, Teststrategie. | Nur messbare Enhancements mit klaren Interfaces/Tests/Perf-Zielen. |
| GGUF_LOADER_README.md | Zweck, Modulgrenzen, Abhaengigkeiten, Build/Run/Test, Einstieg. | Mit aktuellem Modulcode synchronisieren; falsche Pfade/Kommandos korrigieren. |
| GOOGLE_TEST_WIRKSAMKEIT_ZUSAMMENFASSUNG.md | Teststrategie, Ausfuehrung, Fixtures, Flake-Hinweise, Coverage-Regeln. | Testbefehle/Fixtures/Coverage-Hinweise mit aktuellem Testlayout synchronisieren. |
| GRAPH_THEORY.md | Dateizweck klar; Aussagen nur mit Codebezug. | Inhalt rekursiv validieren, verdichten und klare ToDos hinterlassen. |
| GRPC_IMPLEMENTATION_STATUS.md | Verifizierter Ist-Stand, Delta, offene Punkte, Quellen. | Ist-Stand verifizieren; veraltete Reports in Historie markieren/verschieben. |
| GRPC_INTEGRATION.md | Integrationsvertrag, Kompatibilitaet, End-to-End-Pfade. | Integrationsannahmen gegen reale Module und Schnittstellen pruefen. |
| GUI_IMPLEMENTATION.md | Dateizweck klar; Aussagen nur mit Codebezug. | Inhalt rekursiv validieren, verdichten und klare ToDos hinterlassen. |
| GUI_SCREENSHOT.md | Dateizweck klar; Aussagen nur mit Codebezug. | Inhalt rekursiv validieren, verdichten und klare ToDos hinterlassen. |
| HARDWARE_CONFIGURATION_BENCHMARK_SUITE.md | Reproduzierbare Methodik, Parameter, Metriken, Limits. | Methodik reproduzierbar machen; alte Zahlen als historisch markieren. |
| HARDWARE_CONSTRAINTS_IMPLEMENTATION_SUMMARY.md | Verifizierter Ist-Stand, Delta, offene Punkte, Quellen. | Ist-Stand verifizieren; veraltete Reports in Historie markieren/verschieben. |
| HARDWARE_CONSTRAINTS_INTEGRATION_GUIDE.md | Schritt-fuer-Schritt-Kommandos, Voraussetzungen, erwartete Ergebnisse. | Kommandos dry-run-pruefen und auf aktuellen Build/Test-Flow korrigieren. |
| HARDWARE_CONSTRAINTS_README.md | Zweck, Modulgrenzen, Abhaengigkeiten, Build/Run/Test, Einstieg. | Mit aktuellem Modulcode synchronisieren; falsche Pfade/Kommandos korrigieren. |
| HARDWARE_OPTIMIZED_STRATEGY.md | Dateizweck klar; Aussagen nur mit Codebezug. | Inhalt rekursiv validieren, verdichten und klare ToDos hinterlassen. |
| historical_gaps.md | Dateizweck klar; Aussagen nur mit Codebezug. | Inhalt rekursiv validieren, verdichten und klare ToDos hinterlassen. |
| HOW_TO.md | Schritt-fuer-Schritt-Kommandos, Voraussetzungen, erwartete Ergebnisse. | Kommandos dry-run-pruefen und auf aktuellen Build/Test-Flow korrigieren. |
| IMAGE_PROCESSING.md | Dateizweck klar; Aussagen nur mit Codebezug. | Inhalt rekursiv validieren, verdichten und klare ToDos hinterlassen. |
| IMPLEMENTATION_AUDIT.md | Soll/Ist gegen Code mit Evidenzen, Risiken und Abweichungen. | Jeden Befund mit Codebeleg versehen; veraltete Befunde entfernen. |
| IMPLEMENTATION_COMPLETE_SUMMARY.md | Verifizierter Ist-Stand, Delta, offene Punkte, Quellen. | Ist-Stand verifizieren; veraltete Reports in Historie markieren/verschieben. |
| IMPLEMENTATION_COMPLETE.md | Verifizierter Ist-Stand, Delta, offene Punkte, Quellen. | Ist-Stand verifizieren; veraltete Reports in Historie markieren/verschieben. |
| IMPLEMENTATION_GUIDE.md | Schritt-fuer-Schritt-Kommandos, Voraussetzungen, erwartete Ergebnisse. | Kommandos dry-run-pruefen und auf aktuellen Build/Test-Flow korrigieren. |
| IMPLEMENTATION_REPORT_11_20.md | Verifizierter Ist-Stand, Delta, offene Punkte, Quellen. | Ist-Stand verifizieren; veraltete Reports in Historie markieren/verschieben. |
| IMPLEMENTATION_REPORT.md | Verifizierter Ist-Stand, Delta, offene Punkte, Quellen. | Ist-Stand verifizieren; veraltete Reports in Historie markieren/verschieben. |
| IMPLEMENTATION_STATUS.md | Verifizierter Ist-Stand, Delta, offene Punkte, Quellen. | Ist-Stand verifizieren; veraltete Reports in Historie markieren/verschieben. |
| IMPLEMENTATION_SUMMARY.md | Verifizierter Ist-Stand, Delta, offene Punkte, Quellen. | Ist-Stand verifizieren; veraltete Reports in Historie markieren/verschieben. |
| IMPROVEMENTS.md | Dateizweck klar; Aussagen nur mit Codebezug. | Inhalt rekursiv validieren, verdichten und klare ToDos hinterlassen. |
| INDEX_AND_NAVIGATION.md | Dateizweck klar; Aussagen nur mit Codebezug. | Inhalt rekursiv validieren, verdichten und klare ToDos hinterlassen. |
| INDEX.md | Dateizweck klar; Aussagen nur mit Codebezug. | Inhalt rekursiv validieren, verdichten und klare ToDos hinterlassen. |
| INFRASTRUCTURE_STATUS.md | Verifizierter Ist-Stand, Delta, offene Punkte, Quellen. | Ist-Stand verifizieren; veraltete Reports in Historie markieren/verschieben. |
| INGESTION_TOOL_COMPARISON.md | Dateizweck klar; Aussagen nur mit Codebezug. | Inhalt rekursiv validieren, verdichten und klare ToDos hinterlassen. |
| INTEGRATION_CHAOS_TESTING.md | Teststrategie, Ausfuehrung, Fixtures, Flake-Hinweise, Coverage-Regeln. | Testbefehle/Fixtures/Coverage-Hinweise mit aktuellem Testlayout synchronisieren. |
| INTEGRATION_GUIDE.md | Schritt-fuer-Schritt-Kommandos, Voraussetzungen, erwartete Ergebnisse. | Kommandos dry-run-pruefen und auf aktuellen Build/Test-Flow korrigieren. |
| INTEGRATION_TEST_GUIDELINES.md | Schritt-fuer-Schritt-Kommandos, Voraussetzungen, erwartete Ergebnisse. | Kommandos dry-run-pruefen und auf aktuellen Build/Test-Flow korrigieren. |
| KEYWORDS_AND_DETAILS_FEATURE.md | Dateizweck klar; Aussagen nur mit Codebezug. | Inhalt rekursiv validieren, verdichten und klare ToDos hinterlassen. |
| KNOWLEDGE_GAP_DETECTOR_PHASE2_BENCHMARKS.md | Reproduzierbare Methodik, Parameter, Metriken, Limits. | Methodik reproduzierbar machen; alte Zahlen als historisch markieren. |
| KNOWLEDGE_SOURCES.md | Dateizweck klar; Aussagen nur mit Codebezug. | Inhalt rekursiv validieren, verdichten und klare ToDos hinterlassen. |
| LARGE_SCALE_BENCHMARK_STRATEGY.md | Reproduzierbare Methodik, Parameter, Metriken, Limits. | Methodik reproduzierbar machen; alte Zahlen als historisch markieren. |
| legal-ingestion-example.md | Dateizweck klar; Aussagen nur mit Codebezug. | Inhalt rekursiv validieren, verdichten und klare ToDos hinterlassen. |
| LIBRARY_INTEGRATION_TEST_ANALYSIS.md | Teststrategie, Ausfuehrung, Fixtures, Flake-Hinweise, Coverage-Regeln. | Testbefehle/Fixtures/Coverage-Hinweise mit aktuellem Testlayout synchronisieren. |
| LLAMA_LORA_ADAPTER_README.md | Zweck, Modulgrenzen, Abhaengigkeiten, Build/Run/Test, Einstieg. | Mit aktuellem Modulcode synchronisieren; falsche Pfade/Kommandos korrigieren. |
| LLM_ETHICS_PROJECTS.md | Dateizweck klar; Aussagen nur mit Codebezug. | Inhalt rekursiv validieren, verdichten und klare ToDos hinterlassen. |
| LLM_INTEGRATION.md | Integrationsvertrag, Kompatibilitaet, End-to-End-Pfade. | Integrationsannahmen gegen reale Module und Schnittstellen pruefen. |
| LLM_NLP_INTEGRATION_FRAMEWORK.md | Integrationsvertrag, Kompatibilitaet, End-to-End-Pfade. | Integrationsannahmen gegen reale Module und Schnittstellen pruefen. |
| LLM_NLP_INTEGRATION_QUICKSTART.md | Schritt-fuer-Schritt-Kommandos, Voraussetzungen, erwartete Ergebnisse. | Kommandos dry-run-pruefen und auf aktuellen Build/Test-Flow korrigieren. |
| LLM_NLP_INTEGRATION_TEST_PLAN.md | Teststrategie, Ausfuehrung, Fixtures, Flake-Hinweise, Coverage-Regeln. | Testbefehle/Fixtures/Coverage-Hinweise mit aktuellem Testlayout synchronisieren. |
| LLM_STATUS_IMPLEMENTATION.md | Verifizierter Ist-Stand, Delta, offene Punkte, Quellen. | Ist-Stand verifizieren; veraltete Reports in Historie markieren/verschieben. |
| LLM_STATUS_USERS_GUIDE.md | Schritt-fuer-Schritt-Kommandos, Voraussetzungen, erwartete Ergebnisse. | Kommandos dry-run-pruefen und auf aktuellen Build/Test-Flow korrigieren. |
| LOAD_TESTING.md | Teststrategie, Ausfuehrung, Fixtures, Flake-Hinweise, Coverage-Regeln. | Testbefehle/Fixtures/Coverage-Hinweise mit aktuellem Testlayout synchronisieren. |
| LORA_ETHICAL_ALIGNMENT_BEST_PRACTICES.md | Dateizweck klar; Aussagen nur mit Codebezug. | Inhalt rekursiv validieren, verdichten und klare ToDos hinterlassen. |
| LORA_FRAMEWORK_TEST_DOCUMENTATION.md | Teststrategie, Ausfuehrung, Fixtures, Flake-Hinweise, Coverage-Regeln. | Testbefehle/Fixtures/Coverage-Hinweise mit aktuellem Testlayout synchronisieren. |
| LORA_TESTS_SUMMARY.md | Teststrategie, Ausfuehrung, Fixtures, Flake-Hinweise, Coverage-Regeln. | Testbefehle/Fixtures/Coverage-Hinweise mit aktuellem Testlayout synchronisieren. |
| LOSSLESS_COMPRESSION_README.md | Zweck, Modulgrenzen, Abhaengigkeiten, Build/Run/Test, Einstieg. | Mit aktuellem Modulcode synchronisieren; falsche Pfade/Kommandos korrigieren. |
| MASTER_OPTIMIZATION_INDEX.md | Dateizweck klar; Aussagen nur mit Codebezug. | Inhalt rekursiv validieren, verdichten und klare ToDos hinterlassen. |
| ML_MODELS.md | Dateizweck klar; Aussagen nur mit Codebezug. | Inhalt rekursiv validieren, verdichten und klare ToDos hinterlassen. |
| MMDB_E_BENCHMARK_DESIGN.md | Reproduzierbare Methodik, Parameter, Metriken, Limits. | Methodik reproduzierbar machen; alte Zahlen als historisch markieren. |
| MOCK_MODE.md | Dateizweck klar; Aussagen nur mit Codebezug. | Inhalt rekursiv validieren, verdichten und klare ToDos hinterlassen. |
| modern_dms_workflow_scenarios.md | Dateizweck klar; Aussagen nur mit Codebezug. | Inhalt rekursiv validieren, verdichten und klare ToDos hinterlassen. |
| MONITORING_GUIDE.md | Schritt-fuer-Schritt-Kommandos, Voraussetzungen, erwartete Ergebnisse. | Kommandos dry-run-pruefen und auf aktuellen Build/Test-Flow korrigieren. |
| MULTI_AI_BACKENDS.md | Dateizweck klar; Aussagen nur mit Codebezug. | Inhalt rekursiv validieren, verdichten und klare ToDos hinterlassen. |
| MULTI_SHARD_RAID_BENCHMARK_PLAN.md | Reproduzierbare Methodik, Parameter, Metriken, Limits. | Methodik reproduzierbar machen; alte Zahlen als historisch markieren. |
| MULTI_SHARD_RAID_QUICKSTART.md | Schritt-fuer-Schritt-Kommandos, Voraussetzungen, erwartete Ergebnisse. | Kommandos dry-run-pruefen und auf aktuellen Build/Test-Flow korrigieren. |
| NAMESPACE_ANALYSIS_RESULTS.md | Dateizweck klar; Aussagen nur mit Codebezug. | Inhalt rekursiv validieren, verdichten und klare ToDos hinterlassen. |
| NAMESPACE_ANALYZER_README.md | Zweck, Modulgrenzen, Abhaengigkeiten, Build/Run/Test, Einstieg. | Mit aktuellem Modulcode synchronisieren; falsche Pfade/Kommandos korrigieren. |
| NEWS_SOURCES.md | Dateizweck klar; Aussagen nur mit Codebezug. | Inhalt rekursiv validieren, verdichten und klare ToDos hinterlassen. |
| OPTIMIZATION_ROADMAP.md | Konkrete umsetzbare Tasks mit Phasen und Akzeptanzkriterien. | Vage Punkte in umsetzbare Checkbox-Tasks ueberfuehren (Design->Core->Edge->Tests->Hardening->Doku). |
| OPTIMIZATION_SUMMARY_ALL_PHASES.md | Verifizierter Ist-Stand, Delta, offene Punkte, Quellen. | Ist-Stand verifizieren; veraltete Reports in Historie markieren/verschieben. |
| OVERHEAD_ANALYSIS_RAW_VS_THEMIS.md | Dateizweck klar; Aussagen nur mit Codebezug. | Inhalt rekursiv validieren, verdichten und klare ToDos hinterlassen. |
| PARALLEL_BOTTLENECK_DIAGNOSIS.md | Dateizweck klar; Aussagen nur mit Codebezug. | Inhalt rekursiv validieren, verdichten und klare ToDos hinterlassen. |
| PARALLEL_FIX_EXECUTIVE_SUMMARY.md | Verifizierter Ist-Stand, Delta, offene Punkte, Quellen. | Ist-Stand verifizieren; veraltete Reports in Historie markieren/verschieben. |
| PARALLEL_FIX_PRIORITY1_BARRIER_REMOVAL.md | Dateizweck klar; Aussagen nur mit Codebezug. | Inhalt rekursiv validieren, verdichten und klare ToDos hinterlassen. |
| PARALLEL_FIX_V1_COUNTER_ELIMINATION.md | Dateizweck klar; Aussagen nur mit Codebezug. | Inhalt rekursiv validieren, verdichten und klare ToDos hinterlassen. |
| PARALLEL_FIX_V2_POSTMORTEM.md | Dateizweck klar; Aussagen nur mit Codebezug. | Inhalt rekursiv validieren, verdichten und klare ToDos hinterlassen. |
| PARALLEL_FIX_V2_SHARDING_STRATEGY.md | Dateizweck klar; Aussagen nur mit Codebezug. | Inhalt rekursiv validieren, verdichten und klare ToDos hinterlassen. |
| PARALLEL_OPTIMIZATION_INDEX.md | Dateizweck klar; Aussagen nur mit Codebezug. | Inhalt rekursiv validieren, verdichten und klare ToDos hinterlassen. |
| PERFORMANCE_BENCHMARKS.md | Reproduzierbare Methodik, Parameter, Metriken, Limits. | Methodik reproduzierbar machen; alte Zahlen als historisch markieren. |
| PERFORMANCE_COMPARISON_V1.3.0_VS_V1.3.3.md | Reproduzierbare Methodik, Parameter, Metriken, Limits. | Methodik reproduzierbar machen; alte Zahlen als historisch markieren. |
| PERFORMANCE_EVALUATION_V1.3.0_DE.md | Reproduzierbare Methodik, Parameter, Metriken, Limits. | Methodik reproduzierbar machen; alte Zahlen als historisch markieren. |
| PERFORMANCE_IMPROVEMENT_OPTIONS_V1.3.0.md | Reproduzierbare Methodik, Parameter, Metriken, Limits. | Methodik reproduzierbar machen; alte Zahlen als historisch markieren. |
| PERFORMANCE_REPORT_V1.3.0.md | Reproduzierbare Methodik, Parameter, Metriken, Limits. | Methodik reproduzierbar machen; alte Zahlen als historisch markieren. |
| PERFORMANCE_TUNING.md | Reproduzierbare Methodik, Parameter, Metriken, Limits. | Methodik reproduzierbar machen; alte Zahlen als historisch markieren. |
| PERFORMANCE.md | Reproduzierbare Methodik, Parameter, Metriken, Limits. | Methodik reproduzierbar machen; alte Zahlen als historisch markieren. |
| PHASE1_COMPLETE.md | Verifizierter Ist-Stand, Delta, offene Punkte, Quellen. | Ist-Stand verifizieren; veraltete Reports in Historie markieren/verschieben. |
| PHASE1_DEPLOYMENT_GUIDE.md | Schritt-fuer-Schritt-Kommandos, Voraussetzungen, erwartete Ergebnisse. | Kommandos dry-run-pruefen und auf aktuellen Build/Test-Flow korrigieren. |
| PHASE1.5_COMPLETE.md | Verifizierter Ist-Stand, Delta, offene Punkte, Quellen. | Ist-Stand verifizieren; veraltete Reports in Historie markieren/verschieben. |
| PHASE2_COMPLETE.md | Verifizierter Ist-Stand, Delta, offene Punkte, Quellen. | Ist-Stand verifizieren; veraltete Reports in Historie markieren/verschieben. |
| PHASE2_IMPLEMENTATION_PLAN.md | Dateizweck klar; Aussagen nur mit Codebezug. | Inhalt rekursiv validieren, verdichten und klare ToDos hinterlassen. |
| PHASE2_INITIAL_ANALYSIS.md | Dateizweck klar; Aussagen nur mit Codebezug. | Inhalt rekursiv validieren, verdichten und klare ToDos hinterlassen. |
| PHASE2_REPORT.md | Verifizierter Ist-Stand, Delta, offene Punkte, Quellen. | Ist-Stand verifizieren; veraltete Reports in Historie markieren/verschieben. |
| PHASE2_ROOT_CAUSE_ANALYSIS.md | Dateizweck klar; Aussagen nur mit Codebezug. | Inhalt rekursiv validieren, verdichten und klare ToDos hinterlassen. |
| PHASE2F_ROOT_CAUSE_ANALYSIS.md | Dateizweck klar; Aussagen nur mit Codebezug. | Inhalt rekursiv validieren, verdichten und klare ToDos hinterlassen. |
| PHASE2H_FINAL_RESULTS.md | Dateizweck klar; Aussagen nur mit Codebezug. | Inhalt rekursiv validieren, verdichten und klare ToDos hinterlassen. |
| PHASE2H_FURTHER_OPTIMIZATIONS.md | Dateizweck klar; Aussagen nur mit Codebezug. | Inhalt rekursiv validieren, verdichten und klare ToDos hinterlassen. |
| PHASE2H_OUTLOOK.md | Dateizweck klar; Aussagen nur mit Codebezug. | Inhalt rekursiv validieren, verdichten und klare ToDos hinterlassen. |
| PHASE3_COMPLETE.md | Verifizierter Ist-Stand, Delta, offene Punkte, Quellen. | Ist-Stand verifizieren; veraltete Reports in Historie markieren/verschieben. |
| PHASE3_IMPLEMENTATION_NOTE.md | Dateizweck klar; Aussagen nur mit Codebezug. | Inhalt rekursiv validieren, verdichten und klare ToDos hinterlassen. |
| PHASE4_COMPLETE.md | Verifizierter Ist-Stand, Delta, offene Punkte, Quellen. | Ist-Stand verifizieren; veraltete Reports in Historie markieren/verschieben. |
| PHASE5_ROADMAP.md | Konkrete umsetzbare Tasks mit Phasen und Akzeptanzkriterien. | Vage Punkte in umsetzbare Checkbox-Tasks ueberfuehren (Design->Core->Edge->Tests->Hardening->Doku). |
| POLYGLOT_BENCHMARK_STATUS.md | Reproduzierbare Methodik, Parameter, Metriken, Limits. | Methodik reproduzierbar machen; alte Zahlen als historisch markieren. |
| PRODUCTION_DEPLOYMENT_GUIDE.md | Schritt-fuer-Schritt-Kommandos, Voraussetzungen, erwartete Ergebnisse. | Kommandos dry-run-pruefen und auf aktuellen Build/Test-Flow korrigieren. |
| PRODUCTION_REQUIREMENTS.md | Dateizweck klar; Aussagen nur mit Codebezug. | Inhalt rekursiv validieren, verdichten und klare ToDos hinterlassen. |
| PROJECT_COMPLETE.md | Verifizierter Ist-Stand, Delta, offene Punkte, Quellen. | Ist-Stand verifizieren; veraltete Reports in Historie markieren/verschieben. |
| PROJECT_COMPLETION_REPORT.md | Verifizierter Ist-Stand, Delta, offene Punkte, Quellen. | Ist-Stand verifizieren; veraltete Reports in Historie markieren/verschieben. |
| PROMETHEUS_INTEGRATION_QUICKSTART.md | Schritt-fuer-Schritt-Kommandos, Voraussetzungen, erwartete Ergebnisse. | Kommandos dry-run-pruefen und auf aktuellen Build/Test-Flow korrigieren. |
| PROTOCOL_TESTS_README.md | Zweck, Modulgrenzen, Abhaengigkeiten, Build/Run/Test, Einstieg. | Mit aktuellem Modulcode synchronisieren; falsche Pfade/Kommandos korrigieren. |
| QUALITY_CONTROL_README.md | Zweck, Modulgrenzen, Abhaengigkeiten, Build/Run/Test, Einstieg. | Mit aktuellem Modulcode synchronisieren; falsche Pfade/Kommandos korrigieren. |
| QUICK_START.md | Dateizweck klar; Aussagen nur mit Codebezug. | Inhalt rekursiv validieren, verdichten und klare ToDos hinterlassen. |
| QUICK_WINS_REVISED.md | Dateizweck klar; Aussagen nur mit Codebezug. | Inhalt rekursiv validieren, verdichten und klare ToDos hinterlassen. |
| QUICK_WINS.md | Dateizweck klar; Aussagen nur mit Codebezug. | Inhalt rekursiv validieren, verdichten und klare ToDos hinterlassen. |
| QUICKSTART_5GB_BENCHMARK.md | Schritt-fuer-Schritt-Kommandos, Voraussetzungen, erwartete Ergebnisse. | Kommandos dry-run-pruefen und auf aktuellen Build/Test-Flow korrigieren. |
| QUICKSTART.md | Schritt-fuer-Schritt-Kommandos, Voraussetzungen, erwartete Ergebnisse. | Kommandos dry-run-pruefen und auf aktuellen Build/Test-Flow korrigieren. |
| RAID_SHARDING_QUICKSTART.md | Schritt-fuer-Schritt-Kommandos, Voraussetzungen, erwartete Ergebnisse. | Kommandos dry-run-pruefen und auf aktuellen Build/Test-Flow korrigieren. |
| RAID_SHARDING_TEST_PLAN.md | Teststrategie, Ausfuehrung, Fixtures, Flake-Hinweise, Coverage-Regeln. | Testbefehle/Fixtures/Coverage-Hinweise mit aktuellem Testlayout synchronisieren. |
| README_BENCHMARK_SUMMARY.md | Zweck, Modulgrenzen, Abhaengigkeiten, Build/Run/Test, Einstieg. | Mit aktuellem Modulcode synchronisieren; falsche Pfade/Kommandos korrigieren. |
| README_HYBRID_BENCH.md | Zweck, Modulgrenzen, Abhaengigkeiten, Build/Run/Test, Einstieg. | Mit aktuellem Modulcode synchronisieren; falsche Pfade/Kommandos korrigieren. |
| README_LORA_TESTS.md | Zweck, Modulgrenzen, Abhaengigkeiten, Build/Run/Test, Einstieg. | Mit aktuellem Modulcode synchronisieren; falsche Pfade/Kommandos korrigieren. |
| README_MODEL_LOADING_TESTS.md | Zweck, Modulgrenzen, Abhaengigkeiten, Build/Run/Test, Einstieg. | Mit aktuellem Modulcode synchronisieren; falsche Pfade/Kommandos korrigieren. |
| README_PLUGIN_HOT_PLUG.md | Zweck, Modulgrenzen, Abhaengigkeiten, Build/Run/Test, Einstieg. | Mit aktuellem Modulcode synchronisieren; falsche Pfade/Kommandos korrigieren. |
| README_STANDALONE.md | Zweck, Modulgrenzen, Abhaengigkeiten, Build/Run/Test, Einstieg. | Mit aktuellem Modulcode synchronisieren; falsche Pfade/Kommandos korrigieren. |
| README.md | Zweck, Modulgrenzen, Abhaengigkeiten, Build/Run/Test, Einstieg. | Mit aktuellem Modulcode synchronisieren; falsche Pfade/Kommandos korrigieren. |
| REAL_IMPLEMENTATION_SUMMARY.md | Verifizierter Ist-Stand, Delta, offene Punkte, Quellen. | Ist-Stand verifizieren; veraltete Reports in Historie markieren/verschieben. |
| REAL_INGESTION_GUIDE.md | Schritt-fuer-Schritt-Kommandos, Voraussetzungen, erwartete Ergebnisse. | Kommandos dry-run-pruefen und auf aktuellen Build/Test-Flow korrigieren. |
| REAL_INGESTION_STATUS.md | Verifizierter Ist-Stand, Delta, offene Punkte, Quellen. | Ist-Stand verifizieren; veraltete Reports in Historie markieren/verschieben. |
| REALITY_CHECK_VISUAL.md | Dateizweck klar; Aussagen nur mit Codebezug. | Inhalt rekursiv validieren, verdichten und klare ToDos hinterlassen. |
| ROADMAP.md | Konkrete umsetzbare Tasks mit Phasen und Akzeptanzkriterien. | Vage Punkte in umsetzbare Checkbox-Tasks ueberfuehren (Design->Core->Edge->Tests->Hardening->Doku). |
| ROCKSDB_BENCHMARK_BEST_PRACTICES.md | Reproduzierbare Methodik, Parameter, Metriken, Limits. | Methodik reproduzierbar machen; alte Zahlen als historisch markieren. |
| RPC_FRAMEWORK_FINAL_SUMMARY.md | Verifizierter Ist-Stand, Delta, offene Punkte, Quellen. | Ist-Stand verifizieren; veraltete Reports in Historie markieren/verschieben. |
| RPC_FRAMEWORK_REVIEW.md | Soll/Ist gegen Code mit Evidenzen, Risiken und Abweichungen. | Jeden Befund mit Codebeleg versehen; veraltete Befunde entfernen. |
| RPC_IMPLEMENTATION_SUMMARY.md | Verifizierter Ist-Stand, Delta, offene Punkte, Quellen. | Ist-Stand verifizieren; veraltete Reports in Historie markieren/verschieben. |
| S4_BENCHMARK_RUNNING.md | Reproduzierbare Methodik, Parameter, Metriken, Limits. | Methodik reproduzierbar machen; alte Zahlen als historisch markieren. |
| SCALING_GUIDE.md | Schritt-fuer-Schritt-Kommandos, Voraussetzungen, erwartete Ergebnisse. | Kommandos dry-run-pruefen und auf aktuellen Build/Test-Flow korrigieren. |
| SCIENTIFIC_BENCHMARK_PROTOCOL_TEMPLATE.md | Reproduzierbare Methodik, Parameter, Metriken, Limits. | Methodik reproduzierbar machen; alte Zahlen als historisch markieren. |
| SCIENTIFIC_BENCHMARKS_COMPLETION_SUMMARY.md | Reproduzierbare Methodik, Parameter, Metriken, Limits. | Methodik reproduzierbar machen; alte Zahlen als historisch markieren. |
| SCIENTIFIC_PROTOCOL_IMPLEMENTATION.md | Reproduzierbare Methodik, Parameter, Metriken, Limits. | Methodik reproduzierbar machen; alte Zahlen als historisch markieren. |
| SCIENTIFIC_STANDARDS_README.md | Zweck, Modulgrenzen, Abhaengigkeiten, Build/Run/Test, Einstieg. | Mit aktuellem Modulcode synchronisieren; falsche Pfade/Kommandos korrigieren. |
| SCIENTIFIC_VALIDATION_REPORT_v1.0.1.md | Verifizierter Ist-Stand, Delta, offene Punkte, Quellen. | Ist-Stand verifizieren; veraltete Reports in Historie markieren/verschieben. |
| SECURITY.md | Threat-Model, Schutzmassnahmen, Security-Tests und Grenzen. | Security-Angaben gegen aktuelle Flags/Codepfade/Tests validieren. |
| SENSOR_SIMULATION.md | Dateizweck klar; Aussagen nur mit Codebezug. | Inhalt rekursiv validieren, verdichten und klare ToDos hinterlassen. |
| SESSION3_SUMMARY.md | Verifizierter Ist-Stand, Delta, offene Punkte, Quellen. | Ist-Stand verifizieren; veraltete Reports in Historie markieren/verschieben. |
| SHARDING_BENCHMARKS_GUIDE.md | Schritt-fuer-Schritt-Kommandos, Voraussetzungen, erwartete Ergebnisse. | Kommandos dry-run-pruefen und auf aktuellen Build/Test-Flow korrigieren. |
| STANDARD_BENCHMARKS_README.md | Zweck, Modulgrenzen, Abhaengigkeiten, Build/Run/Test, Einstieg. | Mit aktuellem Modulcode synchronisieren; falsche Pfade/Kommandos korrigieren. |
| STATUS.md | Verifizierter Ist-Stand, Delta, offene Punkte, Quellen. | Ist-Stand verifizieren; veraltete Reports in Historie markieren/verschieben. |
| SUMMARY_EXTENDED.md | Verifizierter Ist-Stand, Delta, offene Punkte, Quellen. | Ist-Stand verifizieren; veraltete Reports in Historie markieren/verschieben. |
| SUMMARY.md | Verifizierter Ist-Stand, Delta, offene Punkte, Quellen. | Ist-Stand verifizieren; veraltete Reports in Historie markieren/verschieben. |
| TASK_COMPLETION_REPORT.md | Verifizierter Ist-Stand, Delta, offene Punkte, Quellen. | Ist-Stand verifizieren; veraltete Reports in Historie markieren/verschieben. |
| TEST_AND_BENCHMARK_COVERAGE_ENHANCEMENT.md | Reproduzierbare Methodik, Parameter, Metriken, Limits. | Methodik reproduzierbar machen; alte Zahlen als historisch markieren. |
| TEST_CONFIG_GUIDE.md | Schritt-fuer-Schritt-Kommandos, Voraussetzungen, erwartete Ergebnisse. | Kommandos dry-run-pruefen und auf aktuellen Build/Test-Flow korrigieren. |
| TEST_COVERAGE_REPORT.md | Teststrategie, Ausfuehrung, Fixtures, Flake-Hinweise, Coverage-Regeln. | Testbefehle/Fixtures/Coverage-Hinweise mit aktuellem Testlayout synchronisieren. |
| TEST_ENHANCEMENT_SUMMARY.md | Teststrategie, Ausfuehrung, Fixtures, Flake-Hinweise, Coverage-Regeln. | Testbefehle/Fixtures/Coverage-Hinweise mit aktuellem Testlayout synchronisieren. |
| TEST_STATISTICS.md | Teststrategie, Ausfuehrung, Fixtures, Flake-Hinweise, Coverage-Regeln. | Testbefehle/Fixtures/Coverage-Hinweise mit aktuellem Testlayout synchronisieren. |
| TEST_VERIFICATION_SUMMARY.md | Teststrategie, Ausfuehrung, Fixtures, Flake-Hinweise, Coverage-Regeln. | Testbefehle/Fixtures/Coverage-Hinweise mit aktuellem Testlayout synchronisieren. |
| TESTING_REPORT.md | Teststrategie, Ausfuehrung, Fixtures, Flake-Hinweise, Coverage-Regeln. | Testbefehle/Fixtures/Coverage-Hinweise mit aktuellem Testlayout synchronisieren. |
| THEMIS_HELP_LORA_README.md | Zweck, Modulgrenzen, Abhaengigkeiten, Build/Run/Test, Einstieg. | Mit aktuellem Modulcode synchronisieren; falsche Pfade/Kommandos korrigieren. |
| THEMISDB_INTEGRATION.md | Integrationsvertrag, Kompatibilitaet, End-to-End-Pfade. | Integrationsannahmen gegen reale Module und Schnittstellen pruefen. |
| THEMISDB_SPECIFIC_IMPROVEMENTS_V1.3.0.md | Dateizweck klar; Aussagen nur mit Codebezug. | Inhalt rekursiv validieren, verdichten und klare ToDos hinterlassen. |
| tpc_h_README.md | Zweck, Modulgrenzen, Abhaengigkeiten, Build/Run/Test, Einstieg. | Mit aktuellem Modulcode synchronisieren; falsche Pfade/Kommandos korrigieren. |
| TRANSACTIONDB_OPTIMIZATION_STRATEGY.md | Dateizweck klar; Aussagen nur mit Codebezug. | Inhalt rekursiv validieren, verdichten und klare ToDos hinterlassen. |
| TRANSACTIONDB_VS_DB_ANALYSIS.md | Dateizweck klar; Aussagen nur mit Codebezug. | Inhalt rekursiv validieren, verdichten und klare ToDos hinterlassen. |
| TROUBLESHOOTING.md | Dateizweck klar; Aussagen nur mit Codebezug. | Inhalt rekursiv validieren, verdichten und klare ToDos hinterlassen. |
| TUTORIAL.md | Schritt-fuer-Schritt-Kommandos, Voraussetzungen, erwartete Ergebnisse. | Kommandos dry-run-pruefen und auf aktuellen Build/Test-Flow korrigieren. |
| USER_GUIDE.md | Schritt-fuer-Schritt-Kommandos, Voraussetzungen, erwartete Ergebnisse. | Kommandos dry-run-pruefen und auf aktuellen Build/Test-Flow korrigieren. |
| VCCDB Design.md | Dateizweck klar; Aussagen nur mit Codebezug. | Inhalt rekursiv validieren, verdichten und klare ToDos hinterlassen. |
| VECTOR_ADVANCED_FEATURES_README.md | Zweck, Modulgrenzen, Abhaengigkeiten, Build/Run/Test, Einstieg. | Mit aktuellem Modulcode synchronisieren; falsche Pfade/Kommandos korrigieren. |
| VECTOR_COMPRESSION_BENCHMARK_README.md | Zweck, Modulgrenzen, Abhaengigkeiten, Build/Run/Test, Einstieg. | Mit aktuellem Modulcode synchronisieren; falsche Pfade/Kommandos korrigieren. |
| VECTOR_SEARCH.md | Dateizweck klar; Aussagen nur mit Codebezug. | Inhalt rekursiv validieren, verdichten und klare ToDos hinterlassen. |
| VSCODE_INTEGRATION.md | Integrationsvertrag, Kompatibilitaet, End-to-End-Pfade. | Integrationsannahmen gegen reale Module und Schnittstellen pruefen. |
| WEB_SCRAPING.md | Signaturen/Parameter/Fehlerfaelle gegen reale Endpunkte/Typen. | Signaturen und Fehlerfaelle gegen Implementierung pruefen und korrigieren. |
| WIKIPEDIA_STRESS_TEST_SETUP.md | Teststrategie, Ausfuehrung, Fixtures, Flake-Hinweise, Coverage-Regeln. | Testbefehle/Fixtures/Coverage-Hinweise mit aktuellem Testlayout synchronisieren. |
| WORKFLOW_DESIGN.md | Dateizweck klar; Aussagen nur mit Codebezug. | Inhalt rekursiv validieren, verdichten und klare ToDos hinterlassen. |
| workflows-inventory.md | Dateizweck klar; Aussagen nur mit Codebezug. | Inhalt rekursiv validieren, verdichten und klare ToDos hinterlassen. |
| YAML_CONFIG_IMPLEMENTATION_SUMMARY.md | Verifizierter Ist-Stand, Delta, offene Punkte, Quellen. | Ist-Stand verifizieren; veraltete Reports in Historie markieren/verschieben. |

## Durchfuehrung (autonom)
1. Vollinventar je Bereich und Statuslabel pro Datei: valid / update-needed / obsolete / history-candidate.
2. Dateiweise Abgleich mit Sourcecode (Symbole, Commands, Pfade, Konfigurationen).
3. Rekursive Kondensierung je Modul (Dubletten raus, Kernwissen in stabile Dateien).
4. Bereichs- und Root-Doku synchronisieren.
5. Linkcheck + Stichproben (mind. 5 Symbol/Command-Checks je Bereich).

## Abnahmekriterien
- Keine Broken Links in geänderten MD-Dateien.
- Dokumentierte Evidenz fuer Symbol-/Command-Validierungen je Bereich.
- Root-Ebene ist mit den rekursiv kondensierten Moduldokus konsistent.

## PR-Report Pflicht
1. Geänderte Dateien je Bereich.
2. Pro Datei: alt -> neu inhaltlich (kurz).
3. Korrigierte Links/Pfade.
4. Offene Restpunkte mit Begründung.
