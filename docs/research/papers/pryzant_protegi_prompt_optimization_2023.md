# ProTeGi: Automatic Prompt Optimization with "Gradient Descent" and Beam Search

**Metadaten:**
- Author(en): Reid Pryzant, Dan Iter, Jerry Li, Yin Tat Lee, Chenguang Zhu, Michael Zeng
- Konferenz/Journal: EMNLP 2023 (Findings)
- Jahr: 2023
- Link: [arXiv:2305.03495](https://arxiv.org/abs/2305.03495) · [ACL Anthology](https://aclanthology.org/2023.findings-emnlp.398/)
- Zitierweise: `pryzant2023protegi`
- Tags: `prompt-optimization`, `textual-gradient`, `beam-search`, `meta-llm`, `automatic-prompt`, `mini-batch`, `protegi`
- ThemisDB-Versionen: v2.0.0+ (`src/prompt_engineering/protegi_optimizer.cpp`)
- Status: [x] Implementiert (v2.0.0, `ProTeGiOptimizer`, `IProTeGiLLMProvider`, 18 Unit-Tests)

## 📋 Executive Summary

ProTeGi (Prompt Optimization with Textual Gradients) überträgt das Konzept des **Gradientenabstiegs** auf natürliche Sprache: Statt numerischer Gradienten berechnet ein "Critic-LLM" einen natürlichsprachlichen **Textual Gradient** — eine Kritik des aktuellen Prompts bezogen auf einen Mini-Batch von Fehlerbeispielen. Auf Basis dieser Kritik generiert das LLM eine Menge verbesserter Prompt-Kandidaten (Beam). Die besten Kandidaten überleben in die nächste Iteration (Beam Search). Das Ergebnis ist ein vollautomatischer, LLM-getriebener Prompt-Optimierungskreislauf, der ohne gelabelte Daten und ohne Modelländerungen auskommt.

**ThemisDB-Status:** Vollständig implementiert in `src/prompt_engineering/protegi_optimizer.cpp` als `ProTeGiOptimizer`. Das System bildet den Kern des automatischen Prompt-Verbesserungskreislaufs im `SelfImprovementOrchestrator` und ist durch `IProTeGiLLMProvider` vollständig abstrahiert.

## 🎯 Key Findings

- **Textual Gradient = LLM-Kritik als Gradienten-Analogon**: Der "Gradient" ist eine vom LLM generierte natürlichsprachliche Fehlerbeschreibung: "Der Prompt ist zu vage in Bezug auf Formatierungsanforderungen; er liefert keine Ausgabe für Edge-Case X." Diese Kritik ersetzt den Backpropagation-Gradienten in differenzierbaren Systemen.
- **Mini-Batch-Evaluierung**: ProTeGi bewertet den aktuellen Prompt auf einem zufällig gezogenen Mini-Batch von Test-Beispielen (typisch 4–16 Beispiele). Fehler im Batch akkumulieren sich zur Gradient-Nachricht.
- **Beam-Search über Prompt-Raum**: Aus der Gradient-Nachricht generiert das LLM B Prompt-Varianten (Beam-Width B, typisch 4–8). Alle Varianten werden auf dem Validierungs-Set bewertet; die besten B/2 überleben.
- **Konvergenz**: ProTeGi konvergiert typischerweise in 5–10 Iterationen auf Benchmarks wie BigBench Hard, Instruction Induction und BBH-CoT.
- **Keine Modelländerungen**: ProTeGi verändert keine Modellgewichte. Jede Iteration besteht aus rein Forward-Pass-Operationen (LLM-Inference), was es kompatibel mit Black-Box-LLMs macht.
- **Überlegenheit gegenüber APE**: ProTeGi übertrifft APE (Zhou et al., 2022) auf 17 von 24 BBH-Tasks; besonders stark bei Aufgaben mit komplexem Reasoning (CoT-Kombination).

## 🔗 Direct Influence on ThemisDB

### Affected Modules

- [x] Prompt Engineering → `src/prompt_engineering/protegi_optimizer.cpp` (vollständige ProTeGi-Implementierung)
- [x] Prompt Engineering → `src/prompt_engineering/self_improvement_orchestrator.cpp` (ProTeGiOptimizer als Verbesserungs-Backend)
- [x] Prompt Engineering → `src/prompt_engineering/prompt_optimizer.cpp` (PromptOptimizer: generischer Optimierer, ProTeGi-Spezialisierung)
- [x] Prompt Engineering → `src/prompt_engineering/meta_prompt_generator.cpp` (MetaPromptGenerator liefert Kandidaten-Prompts für ProTeGi-Beam)
- [x] Prompt Engineering → `src/prompt_engineering/feedback_collector.cpp` (FeedbackCollector liefert Fehler-Beispiele für Mini-Batch)

### Was wurde implementiert?

#### `ProTeGiOptimizer` (vollständig implementiert)

```cpp
// include/prompt_engineering/protegi_optimizer.h (vereinfacht)
class IProTeGiLLMProvider {
public:
    virtual ProTeGiGradient computeGradient(
        const std::string& prompt,
        const std::vector<std::string>& errors) = 0;
    virtual std::vector<std::string> generateCandidates(
        const std::string& prompt,
        const ProTeGiGradient& gradient,
        size_t num_candidates) = 0;
};

class ProTeGiOptimizer {
public:
    // Führt vollständige ProTeGi-Optimierungsschleife aus:
    // Mini-Batch → computeGradient → generateCandidates → evaluate → Beam-Filterung
    ProTeGiResult optimize(
        const std::string& initial_prompt,
        const std::vector<ProTeGiExample>& examples,
        EvalFunction eval_fn,
        const ProTeGiConfig& config);
};
```

**ProTeGiConfig** (in ThemisDB):
- `beam_width` (default: 4): Anzahl paralleler Prompt-Kandidaten in jeder Iteration
- `mini_batch_size` (default: 8): Anzahl Fehlerbeispiele pro Gradient-Berechnung
- `max_iterations` (default: 6): Maximale Optimierungsiterationen
- `target_score` (default: 0.9): Abbruchschwelle bei Ziel-Qualität

**HeuristicProTeGiProvider** (eingebetteter Fallback):
- `computeGradient()`: Regelbasierte Kritik basierend auf Fehlerrate und Prompt-Länge
- `generateCandidates()`: Template-basierte Variationen (Präzisions-, Kontext-, Formatierungsverbesserungen)
- Aktiviert automatisch wenn kein LLM-Provider registriert

#### Mini-Batch-Fehlersammlung (FeedbackCollector-Integration)

`SelfImprovementOrchestrator::runProTeGiCycle()` sammelt Fehler-Beispiele aus `FeedbackCollector::getByType(FAILURE | LOW_SCORE)`, zieht einen zufälligen Mini-Batch der Größe `mini_batch_size` und übergibt sie als `errors`-Liste an `IProTeGiLLMProvider::computeGradient()`.

#### Beam-Search-Implementierung

In jeder Iteration:
1. Aktueller Beam (initial: `{initial_prompt}`) → Mini-Batch-Evaluierung
2. Für jeden Prompt im Beam: `computeGradient(errors)` → `generateCandidates(num=beam_width/|beam|)`
3. Alle neu generierten Kandidaten + aktuelle Beam-Prompts → vollständige Evaluierung auf Validierungsset
4. Die `beam_width` besten Kandidaten (nach Score) bilden den neuen Beam
5. Frühabbruch wenn `best_score >= target_score`

### How Was It Adapted?

| ProTeGi Konzept | ThemisDB Adaptation | Rationale |
|---|---|---|
| GPT-4 als Critic-LLM | `IProTeGiLLMProvider` interface + `HeuristicProTeGiProvider` Fallback | LLM-Provider-Abstraktion ermöglicht beliebige Backend-Modelle; Fallback für offline-Betrieb |
| Labeled Test-Set (BigBench) | `FeedbackCollector` Fehler-Beispiele aus Produktionsdaten | Kein manuell annotiertes Test-Set erforderlich; Produktions-Feedback treibt Optimierung |
| Fester Beam-Width | Konfigurierbares `ProTeGiConfig` | ThemisDB-Deployments variieren stark in GPU-Budget; konfigurierbarer Trade-off |
| Globaler Prompt-Pool | Pro-Template ProTeGi-Instanz via `PromptVersionControl` | ThemisDB verwaltet versionierte Templates; jede Optimierungsrunde erzeugt neuen Commit |
| Keine Sicherheitsprüfung | `PromptInjectionDetector` Gate vor jedem Kandidaten-Commit | Generierte Prompt-Varianten können unbeabsichtigt Injection-Muster enthalten |

### Performance Impact

| Metric | Paper Claim (GPT-3.5) | ThemisDB Target | Status |
|--------|----------------------|-----------------|--------|
| Qualitätsverbesserung vs. initialer Prompt | +8–15 pp EM-Score (BBH) | +5 pp auf domänenspezifischen AQL-Prompts | ✅ Implemented |
| Konvergenz | 5–10 Iterationen | ≤ 8 Iterationen | ✅ Implemented |
| Optimierungslatenz pro Iteration (ohne LLM) | n/a | < 1 ms (HeuristicProvider) | ✅ Implemented |
| Beam-Kandidaten pro Iteration | 4–8 | 4 (default, konfigurierbar) | ✅ Implemented |
| Unit-Test-Abdeckung | n/a | 18 Tests (PG-01..PG-18) | ✅ Implemented |

## ⚠️ Limitations & Open Questions

- ProTeGi benötigt ein leistungsfähiges LLM als Critic (GPT-3.5+/LLaMA-13B+); schwache Modelle generieren uninformative Gradienten.
  - ThemisDB-Lösung: `HeuristicProTeGiProvider` als Fallback; LLM-Provider optionales Opt-in.
- Beam-Search kann lokale Optima konvergieren wenn alle Kandidaten ähnliche Fehler machen.
  - ThemisDB-Lösung: `mini_batch_size`-Diversifizierung durch zufälliges Sampling aus `FeedbackCollector`; verschiedene Fehlertypen im Batch.
- ProTeGi optimiert auf syntaktische Prompt-Verbesserung; semantische Qualität (Faktizität, Halluzination) erfordert separate Evaluierungslogik.
  - ThemisDB-Lösung: `PromptEvaluator::score()` bewertet semantische Ähnlichkeit; `PromptRegressionRunner` prüft Regression auf Golden-Set.
- Produktions-Feedback kann biased sein (selektive Benutzerberichte).
  - ThemisDB-Lösung: `FeedbackCollector::detectOutliers()` Z-Score-Anomalie-Erkennung identifiziert atypische Feedback-Einträge vor Mini-Batch-Sampling.

## 🔬 Validation

- [x] `ProTeGiOptimizer::optimize()` implementiert und unit-getestet (18 Tests, PG-01..PG-18)
- [x] `HeuristicProTeGiProvider` implementiert und getestet
- [x] `ProTeGiConfig` alle Parameter validiert
- [x] Beam-Search-Logik: korrekte Kandidaten-Selektion und Beam-Filterung getestet
- [x] CI: `protegi-optimizer-ci.yml`
- [ ] Integration-Test: ProTeGi-Zyklus via `SelfImprovementOrchestrator` mit echtem LLM-Backend
- [ ] A/B-Test: ProTeGi-optimierter Prompt vs. manuell geschriebener Prompt auf Verwaltungsrecht-QA

## 📚 Related Work

- [Zhou et al. (2022) — APE: Automatic Prompt Engineer](../LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md#51-automatic-prompt-engineer-ape) — Vorgänger-Ansatz; ProTeGi übertrifft APE auf komplexen Tasks
- [Madaan et al. (2023) — Self-Refine](madaan_self_refine_2023.md) — Verwandte Selbstverbesserungsschleife; Self-Refine für Response-Verbesserung, ProTeGi für Prompt-Optimierung
- [Best Practice: LLM Prompt Enhancement Pipeline](../best_practices/llm_prompt_enhancement_pipeline.md)
- [`src/prompt_engineering/protegi_optimizer.cpp`](../../../src/prompt_engineering/protegi_optimizer.cpp)
- [`src/prompt_engineering/self_improvement_orchestrator.cpp`](../../../src/prompt_engineering/self_improvement_orchestrator.cpp)
- [Tree of Thoughts — Yao et al. (2023)](yao_tree_of_thoughts_2023.md)

---
**Last Updated:** 2026-04-27  
**Next Review:** 2026-10-31
