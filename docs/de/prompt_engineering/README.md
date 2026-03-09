# Prompt Engineering Modul

**Stand:** 9. März 2026  
**Version:** 1.x  
**Kategorie:** Prompt Engineering  
**Validated:** 2026-03-09 (09f7c55)  
**Status:** current

---

## Übersicht

Das Prompt-Engineering-Modul implementiert ein vollständiges Lifecycle-Management-System für
LLM-Prompt-Templates in ThemisDB. Es umfasst Template-Erstellung und -Speicherung,
Versionskontrolle (Branching, Diffing, Rollback), iterative Optimierung über Meta-Prompts,
Feedback-Erfassung, Performance-Tracking, A/B-Tests, einen Self-Improvement-Orchestrator
sowie einen Prometheus-Metrik-Export. Multi-modale Prompts (Text + Bild-Beschreibungen) sind
über `PromptManager::ImageDescription` und `buildMultiModalPrompt()` unterstützt.

**Primäre Dokumentation:** [`src/prompt_engineering/README.md`](../../../src/prompt_engineering/README.md)  
**Roadmap:** [`src/prompt_engineering/ROADMAP.md`](../../../src/prompt_engineering/ROADMAP.md)  
**Geplante Erweiterungen:** [`src/prompt_engineering/FUTURE_ENHANCEMENTS.md`](../../../src/prompt_engineering/FUTURE_ENHANCEMENTS.md)  
**Fehlende Implementierungen:** [`missing-implementations.md`](missing-implementations.md)

---

## Source-Code Referenz

| Komponente | Header | Source | Beschreibung |
|------------|--------|--------|--------------|
| PromptManager | `prompt_manager.h` | `prompt_manager.cpp` | Template-CRUD, Kontext-Injektion, Multi-Modal-Prompt-Assembly |
| FeedbackCollector | `feedback_collector.h` | `feedback_collector.cpp` | Feedback-Erfassung, Statistiken, Fehlermuster-Analyse |
| PromptEvaluator | `prompt_evaluator.h` | `prompt_evaluator.cpp` | Qualitätsbewertung, statistische Signifikanztests |
| PromptOptimizer | `prompt_optimizer.h` | `prompt_optimizer.cpp` | Iterative Prompt-Verbesserung |
| MetaPromptGenerator | `meta_prompt_generator.h` | `meta_prompt_generator.cpp` | LLM-gestütztes Meta-Prompt-Rewriting |
| PromptVersionControl | `prompt_version_control.h` | `prompt_version_control.cpp` | Git-artiges Versionsmanagement für Prompts |
| PromptPerformanceTracker | `prompt_performance_tracker.h` | `prompt_performance_tracker.cpp` | Ausführungsmetriken pro Prompt |
| SelfImprovementOrchestrator | `self_improvement_orchestrator.h` | `self_improvement_orchestrator.cpp` | Auto-Optimierungsorchestrierung und A/B-Tests |
| PromptEngineeringMetrics | `prompt_engineering_metrics.h` | `prompt_engineering_metrics.cpp` | Prometheus-Metrik-Export und Persistenz |
| PromptInjectionDetector | `prompt_injection_detector.h` | `prompt_injection_detector.cpp` | Musterbasierte Injection-Erkennung und -Sanitisierung |
| PromptEngineeringIntegration | `prompt_engineering_integration.h` | `prompt_engineering_integration.cpp` | High-Level-Fassade und Background-Worker |

**Gesamt:** 11 Header, 11 Source-Dateien in `src/prompt_engineering/` und `include/prompt_engineering/`

---

## Architektur

```
PromptEngineeringIntegration  (Fassade + Background-Worker)
        │
        ├─ PromptManager          ──► RocksDB (Templates)
        ├─ FeedbackCollector      ──► RocksDB (Feedback-Einträge)
        ├─ PromptVersionControl   ──► RocksDB (Versionen, Branches)
        ├─ PromptPerformanceTracker ─► RocksDB (Metriken)
        │
        ├─ PromptEvaluator        (reine Berechnung, keine Persistenz)
        ├─ MetaPromptGenerator    (reine Berechnung, keine Persistenz)
        ├─ PromptOptimizer        (nutzt Evaluator + MetaPromptGenerator)
        │
        ├─ SelfImprovementOrchestrator
        │       ├─ liest  PromptPerformanceTracker
        │       ├─ ruft   PromptOptimizer
        │       └─ schreibt PromptManager + PromptVersionControl
        │
        ├─ PromptEngineeringMetrics  (Prometheus-Export)
        │
        └─ PromptInjectionDetector   (zustandslose Sicherheitsschicht)
```

---

## Bekannte Lücken und fehlende Implementierungen

Drei Roadmap-Einträge in Phase 1 waren fälschlicherweise als `[x]` erledigt markiert, haben
aber keine Code-Evidenz. Sie wurden auf `[?]` zurückgesetzt:

| Feature | Status | Details |
|---------|--------|---------|
| Chain-of-Thought (CoT) Support | `[?]` kein Code | Kein `chain_of_thought.cpp` / `.h` vorhanden |
| RAG Prompt Construction Helpers | `[?]` kein Code | Kein `rag_prompt_builder.cpp` / `.h` vorhanden |
| System Prompt Management (per-role) | `[?]` kein Code | Kein `system_prompt_manager.cpp` / `.h` vorhanden |

Vollständiger Befundbericht: [`missing-implementations.md`](missing-implementations.md)

---

## Produktionsreife-Checkliste

| Kriterium | Status |
|-----------|--------|
| Template-Validierung mit detaillierter Fehlerberichterstattung | ✅ |
| Feedback-Paging-API für große Archive | ✅ |
| Prüfsumme (FNV-1a) auf Feedback-Einträge | ✅ |
| Pluggable LLM-Interface für MetaPromptGenerator | ✅ |
| Pluggable Embedding-Interface für PromptEvaluator | ✅ |
| Welch t-Test für statistische Signifikanz | ✅ |
| Korrekte Normal-CDF für A/B-Test-z-Test-p-Werte | ✅ |
| Metrik-Snapshot/Restore für Crash-Recovery | ✅ |
| Schwellenwert-Alerting mit pluggablen Callbacks | ✅ |
| Prompt-Injection-Erkennungsschicht | ✅ |
| Multi-Modal-Prompt-Unterstützung (Bild-Beschreibungen) | ✅ |
| Unit-Test-Abdeckung > 80 % | `[?]` |
| Integrationstests (Versions-Roundtrip, A/B-Signifikanz) | `[?]` |
| Performance-Benchmarks | `[?]` |
| API-Stabilität garantiert | `[?]` |

---

## Externe Abhängigkeiten

| Bibliothek | Zweck |
|-----------|-------|
| `nlohmann/json` | JSON-Serialisierung/Deserialisierung aller gespeicherten Objekte |
| `yaml-cpp` | YAML-Bulk-Load von Prompt-Templates (`PromptManager::loadFromYAML`) |
| `openssl/sha.h` | SHA-256-Versions-ID-Generierung in `PromptVersionControl` |
| `tbb/concurrent_hash_map.h` | Lock-freie gleichzeitige Hash-Map in `PromptManager` |
| `spdlog` (via logger utils) | Strukturiertes Logging |
