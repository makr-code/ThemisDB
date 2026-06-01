> **Status:** 2026-06-01 – mit aktuellem RAG-Code (`prompt_injection_detector.cpp`, `rag_judge.cpp`, `quality_control_pipeline.cpp`, `rag_context_assembler.cpp`) abgeglichen.

# ThemisDB RAG Module - Production Requirements

## Zweck und Geltungsbereich

Dieses Dokument ist der **kanonische Referenzpunkt für produktive Mindestanforderungen** des RAG-Moduls.
Es definiert verbindliche Anforderungen für Prompt-Injection-Schutz, Context-Assembly, Quality-Gates, Retrieval-Autorisierung und Bias-Kontrolle.

## Dokumentabgrenzung (Canonical Split)

- **`src/rag/PRODUCTION_REQUIREMENTS.md` (dieses Dokument):** verpflichtende Produktionsanforderungen (MUST/MUST NOT), Sicherheitsannahmen, Betriebsgrenzen.
- **`src/rag/README.md`:** Funktionsübersicht, Architekturkontext, API- und Nutzungsbeispiele.
- **`src/rag/ROADMAP.md`:** Lieferphasen, offene/abgeschlossene Features, Readiness-Planung.
- **`src/rag/FUTURE_ENHANCEMENTS.md`:** mittelfristige/langfristige Erweiterungen und Forschungsfelder.

## Verbindliche RAG-Sicherheitsanforderungen

### 1) Prompt-Injection-Schutz

- **MUST:** `prompt_injection_detector.cpp` aktiv für alle retrieval-gestützten Pfade; injizierte Payloads in retrieveten Inhalten werden erkannt und geblockt.
- **MUST:** Sanitizer-Pfad für retrievete Inhalte aktiv vor Context-Assembly.
- **MUST NOT:** Retrievete Inhalte ohne Injection-Check in Generation- oder Judge-Stages weitergeben.

### 2) Context Assembly und Quality Gates

- **MUST:** `rag_context_assembler.cpp` mit bounded Context-Größe konfiguriert; unbegrenztes Context-Wachstum ist nicht zulässig.
- **MUST:** `rag_judge.cpp` und `quality_control_pipeline.cpp` als verpflichtende Gates vor Response-Auslieferung; unsupported/low-trust Outputs werden zurückgehalten.
- **MUST:** Judge-Ergebnis wird als Metadatum mit Response geliefert; kein Silent-Pass bei unklarem Judge-Ergebnis.

### 3) Retrieval-Autorisierung

- **MUST:** Upstream-Autorisierung (`upstream_auth`) integriert; kein unauthoenticated Retrieval-Scope.
- **MUST NOT:** Retrieval-Ergebnisse aus nicht-autorisierten Quellen in Context-Assembly einfließen lassen.

## Betriebsgrenzen (aktuelles RAG-Verhalten)

- `bias_detector.cpp` erfordert periodische Kalibrierung; heuristische Detection-Pfade können unter neuen Angriffsmuster-Varianten nachjustiert werden.
- `ab_testing_framework.cpp` und `batch_evaluator.cpp` sind Evaluations-Tools und nicht für Standard-Produktionspfade vorgesehen.
- `continuous_learning_orchestrator.cpp` (`wireLiveSignalProviders()`) benötigt konfigurierte Signal-Provider; ohne Provider laufen Loop-Iterationen ohne Live-Feedback.

## Minimaler Produktions-Check (Audit-fähig)

- [ ] Prompt-Injection-Detektor aktiv auf allen RAG-Pfaden
- [ ] Sanitizer für retrievete Inhalte vor Context-Assembly aktiv
- [ ] Context-Assembler mit bounded Context-Größe konfiguriert
- [ ] RAG-Judge und Quality-Control-Pipeline aktiv
- [ ] Upstream-Autorisierung für Retrieval-Scope aktiv
- [ ] Judge-Ergebnis wird mit Response geliefert
- [ ] Bias-Detektor kalibriert
- [ ] Produktionsmodus via `THEMIS_PRODUCTION_MODE` oder `THEMIS_ENVIRONMENT` gesetzt

## Review / Sourcecode-Audit-Nachweis

### Betroffene Dateien im Review

- `src/rag/PRODUCTION_REQUIREMENTS.md`
- `src/rag/prompt_injection_detector.cpp`
- `src/rag/rag_judge.cpp`
- `src/rag/quality_control_pipeline.cpp`
- `src/rag/rag_context_assembler.cpp`
- `src/rag/rag_ingestion_bridge.cpp`
- `src/rag/bias_detector.cpp`
- `src/rag/continuous_learning_orchestrator.cpp`
