# Prompt Engineering Modul
<!-- Status: current | validated: 2026-04-06 -->
<!-- Primärdokumentation: ../../../src/prompt_engineering/ -->

**Stand:** 6. April 2026  
**Version:** 1.1  
**Kategorie:** LLM Prompt-Management  
**Validated:** 2026-03-09 (Reality-Check gegen Sourcecode; siehe [MISSING_IMPLEMENTATIONS.md](MISSING_IMPLEMENTATIONS.md))

---

## Übersicht

Das Prompt Engineering Modul stellt ein vollständiges **Lifecycle-Management-System** für LLM-Prompt-Templates in ThemisDB bereit. Es umfasst:

- **Template-Verwaltung** – CRUD mit RocksDB-Persistenz und YAML-Bulk-Load
- **Kontext-Injektion** – `{placeholder}`-Variablensubstitution
- **Versionskontrolle** – Git-ähnliche Branching-, Commit-, Diff- und Rollback-Operationen
- **Iterative Optimierung** – Pluggable Evaluierungs- und Verbesserungsfunktionen mit MetaPromptGenerator
- **Feedback-Erfassung** – 10 Feedback-Typen, Aggregatstatistiken, Anomalieerkennung
- **A/B-Testing** – statistische Signifikanztestung (Welch t-Test, `std::erfc` z-Test)
- **Self-Improvement** – Hintergrund-Worker für automatische Prompt-Optimierung
- **Prometheus-Metriken** – Snapshot/Restore für absturzsichere Persistenz
- **Prompt-Injection-Erkennung** – `PromptInjectionDetector` mit 10 eingebauten Mustern
- **Chain-of-Thought** – `ChainOfThoughtBuilder` mit Schritt-Trennzeichen, Zero-Shot/Few-Shot-Helfer
- **RAG-Prompt-Builder** – `RAGPromptBuilder` mit budgetbewusster Chunk-Selektion und Quellenangaben
- **System-Prompt-Manager** – `SystemPromptManager` mit rollenbasierten Overrides
- **Context-Window-Budget** – `ContextWindowBudgetManager` mit tokenbasierter Limitierung und `PromptBudgetExceededError`
- **Reflection Tuning** – `ReflectionTuner` mit vier Strategien (SELF_REFINE, REFLEXION, CONSTITUTIONAL, SOCRATIC), dynamisch selbst-bewusstem Prompting (`SelfAwareContext`), `ReflectionHallucinationGuard` gegen Halluzinations-Verstärkung
- **LLM-Reflection-Adapter** – `ILLMProviderReflectionAdapter` verbindet jeden bestehenden `ILLMProvider` direkt mit dem `ReflectionTuner` (Adapter-Muster)

---

## Source-Code Referenz

### Implementierung (`src/prompt_engineering/`)

| Datei / Komponente | Rolle |
|---|---|
| `prompt_manager.cpp` | Template-CRUD mit RocksDB-Persistenz, Kontext-Injektion, YAML-Load |
| `feedback_collector.cpp` | Feedback-Erfassung (10 Typen), Aggregatstatistiken, Paging-API |
| `prompt_evaluator.cpp` | Qualitätsbewertung: semantische Ähnlichkeit, exakter/partieller Match, Relevanz |
| `prompt_optimizer.cpp` | Iterative Verbesserungsschleife mit Evaluierungs- und Verbesserungsfunktionen |
| `meta_prompt_generator.cpp` | LLM-gestützte Meta-Prompt-Generierung (`ILLMProvider`) |
| `prompt_version_control.cpp` | Git-ähnliche Versionskontrolle: Commits, Branches, Diff, Rollback |
| `prompt_performance_tracker.cpp` | Laufzeitmetriken: Erfolgsrate, Latenz, Nutzerzufriedenheit |
| `self_improvement_orchestrator.cpp` | Automatische Optimierungs-Pipeline inkl. A/B-Testing |
| `prompt_engineering_metrics.cpp` | Prometheus-kompatible Metriken + Snapshot/Restore |
| `prompt_engineering_integration.cpp` | Hochrangige Facade + Hintergrund-Worker-Thread |
| `prompt_injection_detector.cpp` | Musterbasierte Prompt-Injection-Erkennung und Bereinigung |
| `chain_of_thought.cpp` | CoT-Prompt-Konstruktion: Builder, Zero-Shot, Few-Shot, Wrap-Helfer; Tracer-Wiring |
| `cot_tracer.cpp` | Per-Schritt-Tracing: `IChainOfThoughtTracer`, `RecordingCoTTracer`, `CoTTraceCollector` |
| `prompt_regression_runner.cpp` | Qualitäts-Regressionserkennung: `PromptRegressionRunner`, `RegressionFixture`, `RegressionResult` |
| `prompt_ab_experiment.cpp` | A/B-Experiment-Framework: `PromptABExperimentFramework`, `PromptExperiment`, `ExperimentVariant`, MurmurHash3-32 |
| `prompt_library_io.cpp` | Import/Export-Bibliothek: `PromptLibraryIO`, `PromptLibraryBundle`, JSON+YAML, FNV-1a-Prüfsumme |
| `rag_prompt_builder.cpp` | RAG-Prompt-Zusammenstellung: budgetbewusste Chunk-Selektion |
| `system_prompt_manager.cpp` | System-Prompt-Registry mit rollenbasierter Override-Unterstützung |
| `context_window_manager.cpp` | Token-Budget-Enforcement vor LLM-Dispatch (`ContextWindowBudgetManager`, `ITokenCounter`) |
| `reflection_tuner.cpp` | Iterativer Selbstkritik-Revisions-Zyklus (`ReflectionTuner`, `DynamicReflectionPromptBuilder`, `SelfAwareContext`, `ReflectionHallucinationGuard`) |
| `llm_reflection_adapter.cpp` | Adapter `ILLMProvider` → `IReflectionProvider` (`ILLMProviderReflectionAdapter`, `IReflectionScorer`) |

### Öffentliche Header (`include/prompt_engineering/`)

| Header | Rolle |
|---|---|
| `prompt_manager.h` | `PromptManager`, `PromptTemplate`, `ImageDescription`, `ValidationResult` |
| `feedback_collector.h` | `FeedbackCollector`, `FeedbackType`, `FeedbackEntry`, `FeedbackStats` |
| `prompt_evaluator.h` | `PromptEvaluator`, `IEmbeddingProvider`, `EvaluationMetrics`, `AggregatedMetrics` |
| `prompt_optimizer.h` | `PromptOptimizer`, `OptimizationConfig`, `OptimizationResult` |
| `meta_prompt_generator.h` | `MetaPromptGenerator`, `ILLMProvider`, `MetaPromptConfig`, `MetaPromptResult` |
| `prompt_version_control.h` | `PromptVersionControl`, `VersionCommit`, `Branch`, `DiffResult` |
| `prompt_performance_tracker.h` | `PromptPerformanceTracker`, `PerformanceRecord`, `PerformanceStats` |
| `self_improvement_orchestrator.h` | `SelfImprovementOrchestrator`, `ImprovementConfig`, `ABTestResult` |
| `prompt_engineering_metrics.h` | `PromptEngineeringMetrics`, `AlertConfig` |
| `prompt_engineering_integration.h` | `PromptEngineeringIntegration`, `IntegrationConfig`, `ExecutionContext` |
| `prompt_injection_detector.h` | `PromptInjectionDetector`, `DetectionResult` |
| `chain_of_thought.h` | `ChainOfThoughtBuilder`, `CoTStep`, `CoTConfig` |
| `cot_tracer.h` | `IChainOfThoughtTracer`, `CoTSpanRecord`, `RecordingCoTTracer`, `CoTTraceCollector`, `StepId` |
| `prompt_regression_runner.h` | `PromptRegressionRunner`, `RegressionFixture`, `RegressionConfig`, `RegressionResult`, `FixtureDelta` |
| `prompt_ab_experiment.h` | `PromptABExperimentFramework`, `PromptExperiment`, `ExperimentVariant`, `ExperimentContext`, `ExperimentStatus`, `ExperimentOutcome`, `ExperimentSummary` |
| `prompt_library_io.h` | `PromptLibraryIO`, `PromptLibraryBundle`, `ExportFormat`, `ImportResult`, `ExportResult` |
| `rag_prompt_builder.h` | `RAGPromptBuilder`, `RetrievedChunk`, `RAGPromptConfig` |
| `system_prompt_manager.h` | `SystemPromptManager`, `SystemPrompt`, `Role` |
| `context_window_manager.h` | `ContextWindowBudgetManager`, `ITokenCounter`, `CharDivisionCounter`, `ModelTokenBudget`, `BudgetAllocation`, `PromptBudgetExceededError` |
| `reflection_tuner.h` | `ReflectionTuner`, `IReflectionProvider`, `DynamicReflectionPromptBuilder`, `SelfAwareContext`, `ReflectionHallucinationGuard`, `ReflectionConfig`, `ReflectionResult` |
| `llm_reflection_adapter.h` | `ILLMProviderReflectionAdapter`, `IReflectionScorer` |

---

## Komponentenarchitektur

```
PromptEngineeringIntegration  (Facade + Hintergrund-Worker)
        │
        ├─ PromptManager          ──► RocksDB (Templates)
        ├─ FeedbackCollector      ──► RocksDB (Feedback-Einträge)
        ├─ PromptVersionControl   ──► RocksDB (Versionen, Branches)
        ├─ PromptPerformanceTracker ─► RocksDB (Metriken)
        │
        ├─ PromptEvaluator        (reine Berechnung)
        ├─ MetaPromptGenerator    (reine Berechnung; ILLMProvider-Schnittstelle)
        ├─ PromptOptimizer        (verwendet Evaluator + MetaPromptGenerator)
        │
        ├─ SelfImprovementOrchestrator
        │       ├─ liest  PromptPerformanceTracker
        │       ├─ ruft   PromptOptimizer auf
        │       └─ schreibt PromptManager + PromptVersionControl
        │
        ├─ PromptEngineeringMetrics  (Prometheus-Export)
        │
        ├─ PromptInjectionDetector   (zustandslose Sicherheitsschicht)
        │
        ├─ ChainOfThoughtBuilder     (reine Berechnung; CoT-Prompt-Konstruktion)
        │       └─ IChainOfThoughtTracer  (per-step-Tracing; RecordingCoTTracer / CoTTraceCollector)
        ├─ RAGPromptBuilder          (reine Berechnung; RAG-Kontext-Injektion)
        ├─ SystemPromptManager       (In-Memory-Registry; rollenbasierte System-Prompts)
        ├─ ContextWindowBudgetManager (Token-Limit-Enforcement; ITokenCounter)
        ├─ ReflectionTuner           (iterativer Selbstkritik-Zyklus; 4 Strategien)
        │       ├─ DynamicReflectionPromptBuilder  (strategy-spezifische Prompts)
        │       ├─ SelfAwareContext               (Konfidenz/Unsicherheit aus Antworttext)
        │       └─ ReflectionHallucinationGuard   (Marker-Scan + Divergenz-Erkennung)
        └─ ILLMProviderReflectionAdapter  (ILLMProvider → IReflectionProvider Bridge)
```

---

## Schnellstart

```cpp
#include "prompt_engineering/prompt_engineering_integration.h"
#include "prompt_engineering/prompt_manager.h"
#include "prompt_engineering/chain_of_thought.h"
#include "prompt_engineering/rag_prompt_builder.h"
#include "prompt_engineering/system_prompt_manager.h"

using namespace themis::prompt_engineering;

// --- Template anlegen und verwenden ---
PromptManager mgr;
PromptManager::PromptTemplate t;
t.name    = "Fallzusammenfassung";
t.version = "v1";
t.content = "Fasse den folgenden Rechtsfall zusammen: {fall_text}";
auto created = mgr.createTemplate(t);

auto rendered = mgr.getPromptWithContext(created.id, {{"fall_text", fall_inhalt}});

// --- Chain-of-Thought ---
ChainOfThoughtBuilder cot;
cot.addStep("Alle Parteien identifizieren.")
   .addStep("Streitpunkte ermitteln.")
   .setFinalAnswer("Ergebnis strukturiert ausgeben.");
std::string cot_prompt = cot.build();

// --- RAG-Prompt ---
std::vector<RetrievedChunk> chunks = {
    {"Klausel 4.2: Lieferung bis Q3.", "vertrag_v2.pdf", 0.95}
};
RAGPromptBuilder rag;
std::string rag_prompt = rag.buildFullPrompt(
    "Du bist ein juristischer Assistent.",
    "Welche Lieferpflichten bestehen?",
    chunks);

// --- System-Prompts ---
SystemPromptManager spm;
spm.setPrompt(Role::USER,  "Du hilfst bei {produkt}.", "1.0");
spm.setPrompt(Role::ADMIN, "Admin-Zugang für {produkt}.", "1.0");
std::string sys = spm.renderPrompt(Role::USER, {{"produkt", "ThemisDB"}});
```

---

## Build-Integration

Die Komponenten sind in `cmake/CMakeLists.txt` unter `THEMIS_CORE_SOURCES` registriert:

```cmake
../src/prompt_engineering/prompt_manager.cpp
../src/prompt_engineering/feedback_collector.cpp
../src/prompt_engineering/prompt_evaluator.cpp
../src/prompt_engineering/meta_prompt_generator.cpp
../src/prompt_engineering/prompt_optimizer.cpp
../src/prompt_engineering/prompt_performance_tracker.cpp
../src/prompt_engineering/prompt_version_control.cpp
../src/prompt_engineering/self_improvement_orchestrator.cpp
../src/prompt_engineering/prompt_engineering_integration.cpp
../src/prompt_engineering/prompt_engineering_metrics.cpp
../src/prompt_engineering/prompt_injection_detector.cpp
../src/prompt_engineering/chain_of_thought.cpp
../src/prompt_engineering/rag_prompt_builder.cpp
../src/prompt_engineering/system_prompt_manager.cpp
```

---

## Tests

Testdateien in `tests/`:

| Testdatei | Abgedeckte Komponente |
|---|---|
| `test_prompt_manager.cpp` | `PromptManager` CRUD |
| `test_prompt_manager_validation.cpp` | Template-Validierung |
| `test_prompt_manager_multimodal.cpp` | Multi-modale Prompts |
| `test_prompt_evaluator.cpp` | Evaluierungsmetriken |
| `test_prompt_evaluator_ttest.cpp` | Welch t-Test |
| `test_prompt_evaluator_embedding.cpp` | Embedding-Provider |
| `test_prompt_optimizer.cpp` | Optimierungsschleife |
| `test_meta_prompt_generator.cpp` | Meta-Prompt-Generierung |
| `test_meta_prompt_llm_provider.cpp` | LLM-Provider-Integration |
| `test_prompt_version_control.cpp` | Versionskontrolle |
| `test_prompt_version_control_diff.cpp` | Diff-Algorithmus |
| `test_prompt_performance_tracker.cpp` | Laufzeitmetriken |
| `test_prompt_engineering_integration.cpp` | Facade-Integration |
| `test_prompt_engineering_metrics.cpp` | Prometheus-Metriken |
| `test_prompt_engineering_metrics_persistence.cpp` | Metriken-Snapshot/Restore |
| `test_feedback_collector_scaling.cpp` | Feedback-Skalierung |
| `test_self_improvement_auto_optimize.cpp` | Auto-Optimierung |
| `test_prompt_injection_detector.cpp` | Injection-Erkennung |
| `test_prompt_policy.cpp` | Prompt-Richtlinien |
| `test_chain_of_thought.cpp` | CoT-Builder |
| `test_cot_tracer.cpp` | CoT-Tracer (IChainOfThoughtTracer, RecordingCoTTracer, CoTTraceCollector) |
| `test_prompt_regression_runner.cpp` | Prompt-Regressions-Runner (PromptRegressionRunner, RegressionFixture, FeedbackCollector-Integration) |
| `test_prompt_ab_experiment.cpp` | A/B-Experiment-Framework (PromptABExperimentFramework, MurmurHash3-32, Welch-t-Test, WinnerCallback) |
| `test_prompt_library_io.cpp` | Import/Export-Bibliothek (PromptLibraryIO, JSON/YAML-Roundtrip, FNV-1a-Prüfsumme) |
| `test_rag_prompt_builder.cpp` | RAG-Prompt-Builder |
| `test_system_prompt_manager.cpp` | System-Prompt-Manager |

---

## Verwandte Dokumentation

### Primärdokumentation (Quellcode)

- [README (src/prompt_engineering)](../../../src/prompt_engineering/README.md) — Modulübersicht und Entwicklerleitfaden
- [ARCHITECTURE (src/prompt_engineering)](../../../src/prompt_engineering/ARCHITECTURE.md) — Architektur-Leitfaden
- [ROADMAP (src/prompt_engineering)](../../../src/prompt_engineering/ROADMAP.md) — Entwicklungs-Roadmap, verifiziert gegen Sourcecode
- [FUTURE_ENHANCEMENTS (src/prompt_engineering)](../../../src/prompt_engineering/FUTURE_ENHANCEMENTS.md) — Geplante Features mit Performance-Zielen und IEEE-Referenzen
- [FUTURE_ENHANCEMENTS (include/prompt_engineering)](../../../include/prompt_engineering/FUTURE_ENHANCEMENTS.md) — Header-API-Enhancements

### Verwandte Module

- [LLM-Modul](../llm/README.md) — LLM-Inferenz-Engine, die Prompts verarbeitet
- [RAG-Modul](../rag/KONTINUIERLICHES_LERNEN.md) — Retrieval-Augmented Generation
- [AQL-Modul](../aql/aql_prompt_engineering.md) — AQL-Prompt-Engineering-Integration

### Reality-Check & Offene Implementierungen

- [MISSING_IMPLEMENTATIONS.md](MISSING_IMPLEMENTATIONS.md) — Reality-Check: fehlende/unvollständige Implementierungen mit Evidence und Issue-Vorschlägen (Stand 2026-03-09)
- [missing-implementations.json](missing-implementations.json) — Maschinenlesbares Format
