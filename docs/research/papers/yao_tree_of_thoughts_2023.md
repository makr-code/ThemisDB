# Tree of Thoughts: Deliberate Problem Solving with Large Language Models

**Metadaten:**
- Author(en): Shunyu Yao, Dian Yu, Jeffrey Zhao, Izhak Shafran, Tom Griffiths, Yuan Cao, Karthik Narasimhan
- Konferenz/Journal: NeurIPS 2023 (Spotlight)
- Jahr: 2023
- Link: [arXiv:2305.10601](https://arxiv.org/abs/2305.10601) · [GitHub](https://github.com/princeton-nlp/tree-of-thought-llm)
- Zitierweise: `yao2023tot`
- Tags: `tree-of-thoughts`, `deliberate-reasoning`, `beam-search`, `BFS`, `DFS`, `thought-evaluation`, `multi-path-reasoning`, `ToT`
- ThemisDB-Versionen: v2.0.0+ (`src/prompt_engineering/tree_of_thoughts.cpp`)
- Status: [x] Implementiert (v2.0.0, `TreeOfThoughtsBuilder`, BFS/DFS/BEAM, 30 Unit-Tests)

## 📋 Executive Summary

Tree of Thoughts (ToT) generalisiert Chain-of-Thought (CoT) Prompting zu einer **Baumsuche über Denk-Zwischenschritte**: Statt eines linearen Reasoning-Pfads erkundet ToT einen **Baum von Gedanken** (intermediate reasoning steps). Ein Evaluator bewertet jeden Zwischenschritt auf Vielversprechend-keit; Suchstrategien (BFS, DFS, Beam Search) navigieren den Baum. Das Ergebnis ist eine fundamentale Verbesserung bei Aufgaben, die explorative, vorausschauende und korrigierende Überlegungen erfordern.

**ThemisDB-Status:** Vollständig implementiert als `TreeOfThoughtsBuilder` mit BFS-, DFS- und BEAM-Suchstrategien, pluggablem `IToTThoughtGenerator`- und `IToTEvaluator`-Interface, Tiefen-beschränktem Pruning und Antwort-Synthese.

## 🎯 Key Findings

- **Gedanken als Baumknoten**: Jeder Knoten repräsentiert einen partiellen Reasoning-Pfad. "Gedanken" sind typisch 1–3 Sätze; ThemisDB-Implementierung lässt Länge offen.
- **Dreiteilige Suche**:
  - **BFS**: Schichtweises Expandieren aller Knoten; findet optimalen Pfad bei schmalen Bäumen
  - **DFS** mit Backtracking: Folgt dem vielversprechendsten Ast tief; eignet sich für sequentielle Entscheidungen
  - **Beam Search**: Behält die B besten Knoten pro Ebene; Kompromiss zwischen Qualität und Rechenaufwand
- **Evaluator-Integration**: Ein separater `Evaluator`-Prompt bewertet, ob ein Gedanke "sicher/unmöglich/möglicherweise korrekt" ist. Dies ermöglicht Pruning vor der Expansion.
- **Dramatische Verbesserungen**: Game of 24 (Mathematikrätsel): GPT-4 direkt = 4%, CoT = 11%, ToT = **74%**. Creative Writing (Kohärenz): +27% gegenüber CoT.
- **Kombinierbar mit Self-Refine**: ToT für Exploration, Self-Refine/Reflexion für Verfeinerung des besten Pfads.

## 🔗 Direct Influence on ThemisDB

### Affected Modules

- [x] Prompt Engineering → `src/prompt_engineering/tree_of_thoughts.cpp` (vollständige ToT-Implementierung)
- [x] Prompt Engineering → `src/prompt_engineering/chain_of_thought.cpp` (CoT als Spezialfall: linearer ToT-Pfad)
- [x] Prompt Engineering → `src/prompt_engineering/prompt_engineering_integration.cpp` (optionale ToT-Aktivierung per `IntegrationConfig`)

### Was wurde implementiert?

#### `TreeOfThoughtsBuilder` (vollständig implementiert)

```cpp
// include/prompt_engineering/tree_of_thoughts.h (vereinfacht)
enum class ToTSearchStrategy {
    BFS,   // Breadth-First Search: schichtweise Expansion
    DFS,   // Depth-First Search mit Backtracking
    BEAM   // Beam Search: Top-B Knoten pro Ebene
};

class IToTThoughtGenerator {
public:
    // Generiert k neue Gedanken aus aktuellem Pfad und Problem
    virtual std::vector<std::string> generate(
        const std::string& problem,
        const std::vector<std::string>& path,
        size_t k) = 0;
};

class IToTEvaluator {
public:
    // Bewertet einen Gedanken: Score 0.0–1.0; optional Pruning (< threshold = verwerfen)
    virtual double evaluate(
        const std::string& thought,
        const std::string& problem,
        const std::vector<std::string>& path) = 0;
};

class TreeOfThoughtsBuilder {
public:
    // Führt Baumsuche durch und gibt beste Antwort zurück
    ToTResult search(
        const std::string& problem,
        const ToTConfig& config);

    // Liefert den vollständigen Such-Baum für Observability
    ToTSearchTree getLastSearchTree() const;
};
```

**ToTConfig** (in ThemisDB):
- `strategy`: BFS | DFS | BEAM
- `max_depth` (default: 5): Maximale Baumtiefe
- `branching_factor` (default: 3): Anzahl Gedanken pro Knoten
- `beam_width` (default: 4): Für BEAM-Strategie: Anzahl behaltener Knoten pro Ebene
- `pruning_threshold` (default: 0.3): Evaluator-Score unter dem Knoten verworfen wird
- `answer_synthesis`: Aggregation-Methode für finale Antwort (BEST_LEAF / MAJORITY_VOTE / WEIGHTED)

#### `HeuristicThoughtGenerator` (eingebetteter Fallback)

```cpp
// Generiert Gedanken ohne LLM: Variationen auf Basis von Tiefe und Problem-Auszug
// Format: "Consider approach {depth}.{i}: {problem_excerpt} (variant {i})"
class HeuristicThoughtGenerator : public IToTThoughtGenerator;
```

Aktiviert automatisch, wenn kein `IToTThoughtGenerator` registriert. Geeignet für Tests und Demonstrationen.

#### BFS-Implementierung

```
Ebene 0: {problem}
Ebene 1: [G1, G2, G3] (branching_factor=3 Gedanken)
Ebene 2: [G1.1, G1.2, G1.3, G2.1, G2.2, G2.3, ...]
→ Pruning per Evaluator nach jeder Ebene
→ Abbruch bei max_depth oder wenn kein Knoten > pruning_threshold
```

#### DFS-Implementierung

```
Pfad: problem → G1 → G1.1 → G1.1.1 → ...
→ Bei Sackgasse (Evaluator < pruning_threshold): Backtrack
→ Expliziter Stack; Tiefenabbruch bei max_depth
→ Bester Blattknoten per Score als Antwort
```

#### Beam Search (BEAM-Implementierung)

```
Ebene 0: {problem}
Ebene 1: alle branching_factor Kandidaten → Evaluator-Score → Top-beam_width behalten
Ebene 2: für jeden Beam-Knoten branching_factor neue Gedanken → Top-beam_width behalten
...
→ Bester Pfad am Ende als Antwort
```

#### Antwort-Synthese

`ToTResult::synthesizeAnswer()` aggregiert die Blattknoten-Antworten:
- `BEST_LEAF`: Antwort des am höchsten bewerteten Blatts
- `MAJORITY_VOTE`: Häufigste Antwort unter Top-N Blättern (für geschlossene Antwortmengen)
- `WEIGHTED`: Gewichtetes Ensemble aller Blätter über `ToTEvaluator`-Score (für offene Antworten)

### How Was It Adapted?

| ToT-Konzept | ThemisDB Adaptation | Rationale |
|---|---|---|
| GPT-4 als Thought-Generator | `IToTThoughtGenerator` interface + `HeuristicThoughtGenerator` Fallback | LLM-Agnostizität; Fallback für edge-offline-Deployment |
| GPT-4 als Evaluator | `IToTEvaluator` interface + Score-basiertes Pruning | Flexibel: PromptEvaluator, Embedding-Cosine-Similarity oder LLM-basierter Evaluator |
| Game of 24 / Creative Writing | AQL-Abfrage-Dekomposition, juristische Reasoning-Ketten | ThemisDB-Domänen: komplexe Verwaltungsrechtsfragen, Mehrschritt-Datenbankabfragen |
| Flacher Evaluations-Prompt | `IToTEvaluator::evaluate()` mit Pfad-Kontext | Vollständiger Reasoning-Pfad als Kontext verbessert Bewertungsqualität |
| Globaler Problem-Kontext | `ToTConfig` + `PromptManager`-Integration | Prompt-Templates für Thought-Generation aus `PromptManager` geladen |

### Performance Impact

| Metric | Paper Claim (GPT-4) | ThemisDB Target | Status |
|--------|---------------------|-----------------|--------|
| Game of 24 Erfolgsrate | CoT: 11% → ToT: 74% | +20–50 pp auf komplexen AQL-Reasoning-Tasks | ✅ Implemented |
| Creative Writing Kohärenz | +27% über CoT | +10% auf Verwaltungsrecht-Erklärungen | ✅ Implemented |
| Suchlatenz (ohne LLM, BEAM B=4, depth=3) | n/a | < 2 ms P99 (reines Traversal) | ✅ Implemented |
| Unit-Test-Abdeckung | n/a | 30 Tests (AC-01..AC-30) | ✅ Implemented |

## ⚠️ Limitations & Open Questions

- ToT erhöht den LLM-Aufruf-Overhead drastisch: `branching_factor × max_depth` Generierungs- plus `n_nodes` Evaluierungs-Calls.
  - ThemisDB-Lösung: ToT ist opt-in per `IntegrationConfig::enable_tot`; Standard-Inference-Pfad nutzt CoT.
  - Mikro-Caching: `PromptManager` cached Thought-Generator-Outputs für identische `(problem, path)` Paare.
- Evaluator-Qualität ist entscheidend: Schwache Evaluatoren (Fallback = Heuristik) führen zu suboptimalem Pruning.
  - Offen: Produktions-Evaluator auf Basis `PromptEvaluator::score()` + LoRA-fein-justiert für ThemisDB-Domänen.
- Kontext-Fensterlänge begrenzt die maximale Pfadtiefe, da der vollständige Pfad im Kontext eingebettet ist.
  - ThemisDB-Lösung: `ContextWindowBudgetManager` begrenzt Pfad-Einbettung auf Token-Budget; ältere Pfad-Knoten werden komprimiert.

## 📚 Related Work

- [Self-Refine / Reflexion — Madaan & Shinn (2023)](madaan_self_refine_2023.md) — ToT für Exploration, Self-Refine für Verfeinerung
- [ProTeGi — Pryzant et al. (2023)](pryzant_protegi_prompt_optimization_2023.md) — ProTeGi optimiert Prompts die in ToT verwendet werden
- [DSPy — Khattab et al. (2023)](khattab_dspy_2023.md) — DSPy-Layer kann ToT als Reasoning-Modul integrieren
- [Chain-of-Thought — Wei et al. (2022)](../LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md#42-chain-of-thought-cot-prompting) — CoT ist linearer Spezialfall von ToT
- [Best Practice: LLM Prompt Enhancement Pipeline](../best_practices/llm_prompt_enhancement_pipeline.md)
- [`src/prompt_engineering/tree_of_thoughts.cpp`](../../../src/prompt_engineering/tree_of_thoughts.cpp)

---
**Last Updated:** 2026-04-27  
**Next Review:** 2026-10-31
