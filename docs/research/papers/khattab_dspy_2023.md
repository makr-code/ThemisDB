# DSPy: Compiling Declarative Language Model Calls into Self-Improving Pipelines

**Metadaten:**
- Author(en): Omar Khattab, Arnav Singhvi, Paridhi Maheshwari, Zhiyuan Zhang, Keshav Santhanam, Sri Vardhamanan A, Saiful Haq, Ashutosh Sharma, Thomas T. Joshi, Hanna Moazam, Heather Miller, Matei Zaharia, Christopher Potts
- Konferenz/Journal: ICLR 2024 (Oral)
- Jahr: 2023/2024
- Link: [arXiv:2310.03714](https://arxiv.org/abs/2310.03714) · [GitHub: stanfordnlp/dspy](https://github.com/stanfordnlp/dspy)
- Zitierweise: `khattab2023dspy`
- Tags: `dspy`, `declarative-llm`, `prompt-optimization`, `pipeline-compilation`, `typed-signature`, `predict`, `chain-of-thought`, `module-composition`
- ThemisDB-Versionen: v2.0.0+ (`src/prompt_engineering/dspy_module.cpp`)
- Status: [x] Implementiert (v2.0.0, `DspySignature`, `DspyPredict`, `DspyChainOfThought`, 30 Unit-Tests)

## 📋 Executive Summary

DSPy (Declarative Self-Improving Language Programs) definiert ein **Programmierparadigma für LLM-Pipelines**: Statt Prompts manuell zu verfassen, spezifiziert der Entwickler **typisierte Input/Output-Signaturen** (`DspySignature`). Ein **Compiler** (`DspyOptimize`) übersetzt diese Signaturen in optimierte Prompts unter Nutzung von Trainingsdaten und Metric-Funktionen. Module wie `Predict`, `ChainOfThought`, `ReAct`, `Retrieve` werden zu einer Pipeline kombiniert. Dies trennt die **Programmlogik** (was das Modell tun soll) von der **Prompt-Implementierung** (wie es das tun soll).

**ThemisDB-Status:** DSPy-kompatible Deklarationsschicht implementiert in `dspy_module.cpp` mit `DspySignature`, `DspyPredict`, `DspyChainOfThought`, `IDspyLLMProvider`, `EchoDspyLLMProvider` und `DspyMissingFieldError`. Der DSPy-Compiler (`DspyOptimize`) ist für v2.2.0 geplant.

## 🎯 Key Findings

- **Signaturen statt Prompts**: `question: str -> answer: str` ist eine DSPy-Signatur. Das System leitet daraus automatisch einen effektiven Prompt ab.
- **Composable Module**: `Predict(sig)`, `ChainOfThought(sig)`, `Retrieve(k=3)`, `ReAct(tools)` werden wie Software-Komponenten zusammengesteckt.
- **Compilation = Prompt-Optimierung**: `dspy.compile(program, trainset, metric)` führt automatische Prompt-Optimierung (über Bootstrap-Few-Shot oder ProTeGi-ähnliche Gradients) durch.
- **Teleprompter (Teacher-Student)**: Ein leistungsstarkes Modell generiert Demonstrations für ein schwächeres Modell; der Compiler destilliert das Wissen in Few-Shot-Prompts.
- **Metrische Optimierungsziele**: Jede Python-Funktion kann als Metrik dienen (`lambda pred, gold: pred.answer == gold.answer`).
- **ICLR 2024 Oral**: Topbewertung; Community von >18.000 GitHub-Stars (April 2024); produktiver Einsatz bei Stanford, Databricks, JetBlue.

## 🔗 Direct Influence on ThemisDB

### Affected Modules

- [x] Prompt Engineering → `src/prompt_engineering/dspy_module.cpp` (DspySignature, DspyPredict, DspyChainOfThought, IDspyLLMProvider)
- [x] Prompt Engineering → `src/prompt_engineering/prompt_template_compiler.cpp` (DSL-Compiler verarbeitet DspySignature-artige Slots)
- [x] Prompt Engineering → `src/prompt_engineering/chain_of_thought.cpp` (DspyChainOfThought-Adapter auf `ChainOfThoughtBuilder`)
- [ ] Prompt Engineering → `src/prompt_engineering/dspy_optimizer.cpp` (DspyOptimize-Compiler — geplant v2.2.0)

### Was wurde implementiert?

#### `DspySignature` — Typisierte I/O-Beschreibung

```cpp
// include/prompt_engineering/dspy_module.h (vereinfacht)
enum class DspyFieldType { STRING, REASONING, CODE, LIST };

struct DspyField {
    std::string name;
    std::string description;  // Semantische Beschreibung für Prompt-Ableitung
    DspyFieldType type;
    bool required;
    std::string default_value;
};

class DspySignature {
public:
    DspySignature(std::string name, std::string description);

    DspySignature& addInputField(DspyField field);
    DspySignature& addOutputField(DspyField field);

    // Generiert den Basis-Prompt aus der Signatur
    std::string toPromptTemplate() const;

    const std::vector<DspyField>& inputFields() const;
    const std::vector<DspyField>& outputFields() const;
};
```

Beispiel einer ThemisDB-Signatur für AQL-Übersetzung:
```cpp
auto sig = DspySignature("AQLTranslator", "Translates natural language to AQL")
    .addInputField({"query", "Natural language database query", DspyFieldType::STRING, true, ""})
    .addInputField({"schema", "Available collections and fields", DspyFieldType::STRING, false, ""})
    .addOutputField({"aql", "AQL query string", DspyFieldType::STRING, true, ""})
    .addOutputField({"reasoning", "Step-by-step translation logic", DspyFieldType::REASONING, false, ""});
```

#### `DspyPredict` — Basis-Inferenz-Modul

```cpp
class DspyPredict {
public:
    explicit DspyPredict(DspySignature signature);
    void setLLMProvider(std::shared_ptr<IDspyLLMProvider> provider);

    // Führt Inferenz durch: Rendert Signatur → LLM-Aufruf → Parst Output-Felder
    DspyPrediction forward(const DspyContext& input);
};
```

`DspyPredict::forward()` folgt dem DSPy-Pattern:
1. Rendere `toPromptTemplate()` mit `input`-Feldern
2. Rufe `IDspyLLMProvider::generate(prompt)` auf
3. Parse Output-Felder aus der LLM-Antwort (JSON-Extraktion oder Regex-Fallback)
4. Gib `DspyPrediction` mit allen Output-Feldern zurück

#### `DspyChainOfThought` — CoT-Erweiterung

```cpp
class DspyChainOfThought : public DspyPredict {
public:
    // Erweitert Signatur automatisch um REASONING-Output-Feld
    // Rendert "Let's think step by step" Präambel
    explicit DspyChainOfThought(DspySignature base_signature);
};
```

Entspricht `dspy.ChainOfThought(sig)` in der Python-DSPy-API.

#### `EchoDspyLLMProvider` — Test-Provider

```cpp
// Gibt Input-Prompt unverändert zurück (für deterministische Tests)
class EchoDspyLLMProvider : public IDspyLLMProvider {
public:
    std::string generate(const std::string& prompt) override { return prompt; }
};
```

#### `DspyMissingFieldError` — Typsicherer Fehler

Wird geworfen, wenn ein required Output-Feld nicht in der LLM-Antwort gefunden wurde. Ermöglicht strukturierte Fehlerbehandlung in Pipelines.

### Geplante DSPy-Compiler-Integration (v2.2.0)

Der DSPy-Compiler (`DspyOptimize`) soll folgende Teleprompter implementieren:
- `BootstrapFewShot`: Generiert Demonstrations aus `FeedbackCollector::getByType(SUCCESS)` für Few-Shot-Prompts
- `LabeledFewShot`: Nutzt manuell annotierte `RegressionFixture`-Paare aus `PromptRegressionRunner`
- `ProTeGiTeleprompter`: Verbindet `DspySignature` mit `ProTeGiOptimizer` für vollautomatische Prompt-Compilation

### How Was It Adapted?

| DSPy-Konzept | ThemisDB Adaptation | Rationale |
|---|---|---|
| Python `dspy.Signature` | C++ `DspySignature` mit `addInputField/addOutputField` | Native C++-Integration in ThemisDB; keine Python-Bindung erforderlich |
| `dspy.Predict(sig)` | `DspyPredict` mit `IDspyLLMProvider` interface | Provider-Abstraktion wie im Rest des PromptEngineering-Moduls |
| `dspy.ChainOfThought(sig)` | `DspyChainOfThought` als `DspyPredict`-Subklasse | Wiederverwendung von `ChainOfThoughtBuilder`-Infrastruktur |
| Globaler `dspy.settings.lm` | `DspyPredict::setLLMProvider()` pro Instanz | ThemisDB: Multi-Tenant; verschiedene Provider pro Template möglich |
| `dspy.compile()` | `DspyOptimize` — geplant v2.2.0 | Kompilierungs-Phase erfordert trainierte Bootstrap-Daten; Phase 1: Deklarationsschicht |
| Python `dict`-basierte I/O | `DspyContext` (Map<string, string>) + typisierte `DspyField` | Type-Sicherheit; `DspyMissingFieldError` für fehlende Pflichtfelder |

### Performance Impact

| Metric | Paper Claim | ThemisDB Target | Status |
|--------|-------------|-----------------|--------|
| Prompt-Qualität nach Compilation (MultiHop QA) | +11% F1 gegenüber manuellen Prompts | +5–10% auf AQL-Translations-Tasks | ✅ Signatur-Schicht implementiert (Compiler folgt) |
| `DspyPredict::forward()` Latenz (ohne LLM) | n/a | < 0.5 ms P99 | ✅ Implemented |
| Unit-Test-Abdeckung | n/a | 30 Tests (AC-01..AC-30) | ✅ Implemented |
| DSPy-Compiler (`DspyOptimize`) | +11–40% F1 | geplant Q3 2026 | ⏳ Planned |

## ⚠️ Limitations & Open Questions

- Der DSPy-Compiler benötigt Trainingsdaten und eine Metrik-Funktion — nicht immer in Produktions-Deployments verfügbar.
  - ThemisDB-Lösung: `FeedbackCollector` + `PromptRegressionRunner` liefern Trainingsdaten organisch.
- Output-Feld-Parsing aus freiem LLM-Text ist fehleranfällig.
  - ThemisDB-Lösung: `DspyMissingFieldError` + strukturiertes JSON-Output-Forcing via `structured_output.cpp`.
- DSPy-Signaturen können nicht alle Prompt-Kontrollflüsse ausdrücken (z.B. bedingte Logik).
  - ThemisDB-Lösung: Hybridansatz: `DspySignature` für einfache I/O; `PromptTemplateCompiler`-DSL für komplexe Logik.

## 📚 Related Work

- [ProTeGi — Pryzant et al. (2023)](pryzant_protegi_prompt_optimization_2023.md) — DSPy-Compiler nutzt ProTeGi-ähnliche Textual Gradients
- [Tree of Thoughts — Yao et al. (2023)](yao_tree_of_thoughts_2023.md) — ToT als DSPy-Reasoning-Modul integrierbar
- [Zhou et al. (2022) — APE](../LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md#51-automatic-prompt-engineer-ape) — APE: erster Schritt zu automatischer Prompt-Generierung; DSPy erweitert dies zu vollständigen Pipelines
- [Best Practice: LLM Prompt Enhancement Pipeline](../best_practices/llm_prompt_enhancement_pipeline.md)
- [`src/prompt_engineering/dspy_module.cpp`](../../../src/prompt_engineering/dspy_module.cpp)
- [`src/prompt_engineering/prompt_template_compiler.cpp`](../../../src/prompt_engineering/prompt_template_compiler.cpp)

---
**Last Updated:** 2026-04-27  
**Next Review:** 2026-10-31
