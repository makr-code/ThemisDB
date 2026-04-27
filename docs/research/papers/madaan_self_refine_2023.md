# Self-Refine: Iterative Refinement with Self-Feedback + Reflexion

**Metadaten:**
- Author(en): Aman Madaan, Niket Tandon, Prakhar Gupta, Skyler Hallinan, Luyu Gao, Sarah Wiegreffe, Uri Alon, Nouha Dziri, Shrimai Prabhumoye, Yiming Yang, Shashank Gupta, Bodhisattwa Prasad Majumder, Katherine Hermann, Sean Welleck, Amir Yazdanbakhsh, Peter Clark  (Self-Refine); Noah Shinn, Federico Cassano, Ashwin Gopinath, Karthik Narasimhan, Shunyu Yao (Reflexion)
- Konferenz/Journal: NeurIPS 2023 (Self-Refine) · NeurIPS 2023 (Reflexion)
- Jahr: 2023
- Links: [Self-Refine arXiv:2303.17651](https://arxiv.org/abs/2303.17651) · [Reflexion arXiv:2303.11366](https://arxiv.org/abs/2303.11366)
- Zitierweise: `madaan2023selfrefine`, `shinn2023reflexion`
- Tags: `self-refine`, `reflexion`, `reflection`, `constitutional-ai`, `socratic`, `hallucination-guard`, `iterative-refinement`, `verbal-rl`
- ThemisDB-Versionen: v1.5.0+ (`src/prompt_engineering/reflection_tuner.cpp`)
- Status: [x] Implementiert (v1.5.0, `ReflectionTuner`, 4 Strategien, 38 Unit-Tests)

## 📋 Executive Summary

**Self-Refine** (Madaan et al., NeurIPS 2023) und **Reflexion** (Shinn et al., NeurIPS 2023) etablieren das Paradigma der **iterativen LLM-Selbstverbesserung**: Das Modell generiert eine Antwort, kritisiert sie aus eigener Perspektive, und verfeinert die Antwort auf Basis der Kritik — ohne externe Labels oder Modelltraining. Beide Arbeiten demonstrieren substanzielle Qualitätsverbesserungen auf Code-Generierung, QA, Dialogsteuerung und mathematischen Problemen.

**ThemisDB-Status:** Vollständig implementiert als `ReflectionTuner` mit vier Strategien (SELF_REFINE, REFLEXION, CONSTITUTIONAL, SOCRATIC), pluggablem `IReflectionProvider`-Interface, selbstbewusstem Kontext-Extraktor (`SelfAwareContext`) und Halluzinations-Schutzmechanismus (`ReflectionHallucinationGuard`).

## 🎯 Key Findings

### Self-Refine (Madaan et al.)

- **3-Phasen-Schleife**: Generate → Feedback → Refine, iteriert bis Konvergenzschwelle oder max. Iterationen
- **Gleiches Modell für alle Phasen**: Kein separates Critic-Modell nötig; GPT-3.5/GPT-4 übernimmt alle drei Rollen
- **Effektive Qualitätssteigerung**: +14–30% auf Dialogerzeugung, +7% auf Mathe (GSM8K), +15% auf Code-Review-Tasks gegenüber direkter Generierung
- **Selbstmotivierte Kritik**: Das Modell identifiziert eigene Schwächen (Logikfehler, Sprachstil, Vollständigkeit) ohne externe Supervision
- **Konvergenz**: Typisch 2–3 Iterationen für die meisten Tasks; diminishing returns nach 4 Iterationen

### Reflexion (Shinn et al.)

- **Verbales Reinforcement Learning**: Anstatt Gradienten-Updates speichert der Agent Erfahrungen als natürlichsprachliche Reflexionen im Episodic Memory Context
- **Episodic Buffer**: Vergangene Fehler und Korrekturen werden als Text in den nächsten Prompt eingefügt → stärkere Drift-Korrektur als Self-Refine
- **Eval-Signal**: Externe Signal-Quellen (Interpreter-Fehler, Testergebnisse) können die Reflexion steuern; ThemisDB nutzt `PromptEvaluator::score()` als Signal
- **Stärke**: Besonders wirksam bei sequentiellen Entscheidungsaufgaben (AlfWorld: +22%) und Code (HumanEval: +17%)

### Constitutional AI (Bai et al., Anthropic 2022 — integriert in CONSTITUTIONAL-Strategie)

- **Prinzipien-geleitete Selbstkritik**: Das Modell prüft seine Antwort anhand einer expliziten Liste von Prinzipien (z.B. "keine Fehlinformationen", "quellenbasierte Aussagen")
- **Skalierbare Aufsicht**: Ohne Human-Feedback skalierbar; Prinzipien ersetzen RLHF-Labels
- **ThemisDB-Nutzung**: `ReflectionTuner::CONSTITUTIONAL` mit konfigurierbaren `constitutional_principles` in `ReflectionConfig`

## 🔗 Direct Influence on ThemisDB

### Affected Modules

- [x] Prompt Engineering → `src/prompt_engineering/reflection_tuner.cpp` (vollständige Implementierung aller 4 Strategien)
- [x] Prompt Engineering → `src/prompt_engineering/prompt_engineering_integration.cpp` (`afterExecution()` optionaler Reflection-Pass)
- [x] Prompt Engineering → `src/prompt_engineering/llm_reflection_adapter.cpp` (`ILLMProviderReflectionAdapter`: Bridges `ILLMProvider` → `IReflectionProvider`)
- [x] Prompt Engineering → `src/prompt_engineering/prompt_engineering_metrics.cpp` (4 Reflection-Metriken + Prometheus-Export)

### Was wurde implementiert?

#### `ReflectionTuner` — Vier Strategien

```cpp
// include/prompt_engineering/reflection_tuner.h (vereinfacht)
enum class ReflectionStrategy {
    SELF_REFINE,     // Madaan et al. 2023: generate → feedback → refine
    REFLEXION,       // Shinn et al. 2023: generate → critique → revise mit Episodic Memory
    CONSTITUTIONAL,  // Bai et al. 2022: Prinzipien-geprüfte Selbstkritik
    SOCRATIC         // Sokrates-Methode: Hypothesen durch Fragen schrittweise verfeinern
};

class ReflectionTuner {
public:
    // Iterative Verbesserungsschleife mit gewählter Strategie
    ReflectionResult refine(
        const std::string& initial_response,
        const std::string& task_context,
        const ReflectionConfig& config);
};

class IReflectionProvider {
public:
    virtual std::string generate(const std::string& prompt) = 0;
    virtual std::string critique(const std::string& response, const std::string& context) = 0;
    virtual std::string revise(const std::string& response, const std::string& critique) = 0;
    virtual double score(const std::string& response, const std::string& context) = 0;
};
```

**ReflectionConfig** (in ThemisDB):
- `strategy`: SELF_REFINE | REFLEXION | CONSTITUTIONAL | SOCRATIC
- `max_iterations` (default: 3): Maximale Verfeinerungsrunden
- `convergence_threshold` (default: 0.85): Score-Schwelle für Frühabbruck
- `min_delta_improvement` (default: 0.01): Minimale Verbesserung pro Iteration
- `divergence_threshold` (default: 0.15): Maximale Score-Abnahme vor Halluzinations-Stop
- `divergence_window` (default: 3): Fenster für gleitenden Durchschnitt
- `constitutional_principles`: Liste von Prüfprinzipien für CONSTITUTIONAL-Strategie
- `include_self_aware_context` (default: true): `SelfAwareContext` injizieren

#### `SelfAwareContext` — Selbstbewusste Kontext-Extraktion (ThemisDB-Innovation)

ThemisDB erweitert Self-Refine um eine **selbst-bewusste Komponente**: `SelfAwareContext::fromResponse()` analysiert die linguistischen Konfidenzsignale in der LLM-Antwort:

```cpp
// Linguistische Marker für Unsicherheit:
kUncertaintyMarkers = { "i think", "possibly", "perhaps", "maybe", "uncertain",
                        "i'm not sure", "likely", "probably", "might", "could be" };
// Linguistische Marker für Konfidenz:
kConfidenceMarkers = { "definitely", "certainly", "absolutely", "clearly",
                       "without doubt", "i'm certain", "exactly", "i know" };
```

`confidence_ratio = confidence_markers / (confidence + uncertainty_markers)` → steuert die **Intensität der Kritik-Prompts**: Niedrige Konfidenz → schärfere Selbstkritik-Aufforderung, hohe Konfidenz → Faktizitäts-Check.

#### `ReflectionHallucinationGuard` — Halluzinations-Schutz

Zwei Mechanismen verhindern, dass die Refinement-Schleife Halluzinationen amplifikiert:

**Mechanismus 1 — Marker-Scan:**
```cpp
kHallucinationMarkers = {
    "i cannot verify", "i'm not sure about", "this may be incorrect",
    "actually, let me reconsider", "wait, i made an error",
    "my previous response was incorrect", "i hallucinated", ...
};
```
Wenn ≥ 1 Marker in der Kritik oder Revision gefunden → sofortiger Abbruch der Schleife.

**Mechanismus 2 — Rolling-Average Divergenz:**
Wenn der Quality-Score über `divergence_window` Iterationen um mehr als `divergence_threshold` fällt → Abbruch + Rückgabe der besten bisher gesehenen Antwort.

#### Strategie-spezifische Prompt-Konstruktion

`DynamicReflectionPromptBuilder` generiert für jede Strategie angepasste Prompts:

| Strategie | Critique-Prompt-Muster | Revise-Prompt-Muster |
|---|---|---|
| SELF_REFINE | "Identify weaknesses in your response..." | "Improve your response, addressing: {critique}" |
| REFLEXION | "Reflect on your mistakes: {errors}. What would you do differently?" | "Based on your reflection, provide a revised answer" |
| CONSTITUTIONAL | "Does your response violate: {principles}? List all violations." | "Revise your response to comply with all principles" |
| SOCRATIC | "What assumptions does your response make? Are they all valid?" | "Revise to eliminate invalid assumptions" |

#### Integration in `PromptEngineeringIntegration`

```cpp
// src/prompt_engineering/prompt_engineering_integration.cpp
void PromptEngineeringIntegration::afterExecution(
    const std::string& template_id,
    const std::string& response,
    const ExecutionContext& ctx)
{
    if (config_.enable_reflection_tuning && reflection_tuner_) {
        auto result = reflection_tuner_->refine(
            response, ctx.task_context,
            ReflectionConfig{.max_iterations = config_.reflection_max_iterations});
        if (result.final_score > result.initial_score + 0.05) {
            // Verbesserte Antwort an Caller zurückgeben
        }
        metrics_->recordReflectionCycle(result.iterations, result.final_score);
    }
}
```

### How Was It Adapted?

| Paper-Konzept | ThemisDB Adaptation | Rationale |
|---|---|---|
| Gleiches Modell für alle Phasen | `IReflectionProvider` mit 4 Methoden (generate/critique/revise/score) | Flexibilität: verschiedene Modelle pro Phase möglich (z.B. schnelles Modell für critique) |
| GPT-4 as Evaluator | `IReflectionScorer` interface; Fallback zu `PromptEvaluator::score()` | Provider-Agnostizität; kein Cloud-Lock-in |
| Reflexion Episodic Buffer | `ReflectionResult.iteration_trace` + `SelfAwareContext` Injektion | ThemisDB-Kontext: Iterationen explizit im Trace für Observability-Integration |
| Constitutional Principles (Anthropic) | Konfigurierbares `constitutional_principles`-Array in `ReflectionConfig` | Domänen-spezifische Compliance (Datenschutz, Verwaltungsrecht) |
| Keine Halluzinations-Schutze | `ReflectionHallucinationGuard` (ThemisDB-Innovation) | Produktionssicherheit: Schleife darf keine Fehlinformationen verstärken |
| Kein selbstbewusster Kontext | `SelfAwareContext` Konfidenzsignal-Extraktion (ThemisDB-Innovation) | Adaptives Prompting auf Basis LLM-eigener Unsicherheit |

### Performance Impact

| Metric | Paper Claim | ThemisDB Target | Status |
|--------|-------------|-----------------|--------|
| Qualitätsverbesserung (Dialogue) | +14–30% | +5–15% (AQL-Prompt-Erklärungen) | ✅ Implemented |
| Typische Konvergenz | 2–3 Iterationen | 3 Iterationen (max_iterations default) | ✅ Implemented |
| Refinement-Latenz pro Iteration (ohne LLM) | n/a | < 0.5 ms P99 (Prompt-Konstruktion) | ✅ Implemented |
| Vollständiger 3-Iter-Zyklus (ohne LLM) | n/a | < 1 ms P99 | ✅ Implemented |
| `SelfAwareContext::fromResponse()` | n/a | < 0.1 ms (512-Token-Antwort) | ✅ Implemented |
| Unit-Test-Abdeckung | n/a | 38 Tests (AC-01..AC-38) | ✅ Implemented |

## ⚠️ Limitations & Open Questions

- Self-Refine und Reflexion profitieren von leistungsstarken LLMs (≥ 7B Parameter); schwache Modelle produzieren uninformative Kritiken.
  - ThemisDB-Lösung: Template-basierter Fallback in `IReflectionProvider::critique()` ohne LLM.
- Die Reflexionsschleife kann unter adversarialen Bedingungen (feindlicher User-Input) Schadprompts amplifikieren.
  - ThemisDB-Lösung: `PromptInjectionDetector` Gate vor jedem `generate()`-Aufruf; `ReflectionHallucinationGuard` bricht bei Marker-Treffer ab.
- Konstitutionelle Prinzipien müssen manuell gepflegt werden.
  - Auflösung: `config/prompts/constitutional_principles.yaml` — zentrale Verwaltung; per-Tenant-Override möglich.
- Keine empirische Validierung der ThemisDB-spezifischen SOCRATIC-Strategie gegen Paper-Benchmarks.
  - Offen: Benchmark-Suite für ReflectionTuner auf ThemisDB-eigenen QA-Paaren.

## 📚 Related Work

- [ProTeGi — Pryzant et al. (2023)](pryzant_protegi_prompt_optimization_2023.md) — ProTeGi optimiert Prompts; Self-Refine verbessert Antworten — komplementäre Loops
- [Yao et al. (2023) — Tree of Thoughts](yao_tree_of_thoughts_2023.md) — ToT erweitert den Gedankenraum; Self-Refine verfeinert einzelne Reasoning-Pfade
- [Zhou et al. (2022) — APE](../LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md#51-automatic-prompt-engineer-ape)
- [Best Practice: LLM Prompt Enhancement Pipeline](../best_practices/llm_prompt_enhancement_pipeline.md)
- [`src/prompt_engineering/reflection_tuner.cpp`](../../../src/prompt_engineering/reflection_tuner.cpp)
- [`src/prompt_engineering/llm_reflection_adapter.cpp`](../../../src/prompt_engineering/llm_reflection_adapter.cpp)

---
**Last Updated:** 2026-04-27  
**Next Review:** 2026-10-31
