# Prompt Engineering Modul — Fehlende / Unvollständige Implementierungen

<!-- Status: current | validated: 2026-03-09 -->
<!-- Primärdokumentation: ../../../src/prompt_engineering/ -->

Dieser Report dokumentiert Funktionen, die in `src/prompt_engineering/ROADMAP.md`,
`src/prompt_engineering/FUTURE_ENHANCEMENTS.md` oder dem zugehörigen Header-Verzeichnis als
implementiert oder geplant beschrieben werden, jedoch bei der Reality-Check-Prüfung als
**nicht vollständig umgesetzt** befunden wurden.

Prüfstand: 2026-03-09 | Branch: `develop`

---

## 1. Typed Template DSL — nicht implementiert

| Feld | Wert |
|---|---|
| **Claim-Quelle** | `src/prompt_engineering/ROADMAP.md` §"Phase 2: Typed DSL & Context Budget" (`[?]`) |
| **Erwartet** | `IPromptTemplate`-Schnittstelle mit kompilierter DSL-Unterstützung (Slots, Conditionals, Loops), `TemplateContext`-Typsystem, `validate()` ohne Rendering |
| **Beobachtet** | Keine `IPromptTemplate`-Schnittstelle im `include/`-Verzeichnis; aktuelle Templates sind einfache `std::string`-Felder in `PromptTemplate::content`; kein DSL-Compiler vorhanden |
| **Evidence (geprüfte Pfade)** | `include/prompt_engineering/prompt_manager.h` (kein `IPromptTemplate`); `include/prompt_engineering/FUTURE_ENHANCEMENTS.md` §"Structured Prompt Template DSL API" (alle `[ ]`) |
| **ROADMAP-Status** | `[?]` – nicht begonnen (Target: Q2 2026) |
| **Issue-Titelvorschlag** | `[prompt_engineering] Implement typed DSL for PromptTemplate with slot/conditional/loop support` |
| **Label-Vorschläge** | `type:feature`, `priority:medium`, `prompt_engineering`, `status:planned` |

---

## 2. Context Window Budget Manager — ✅ implementiert (v1.5.0)

| Feld | Wert |
|---|---|
| **Claim-Quelle** | `src/prompt_engineering/ROADMAP.md` §"Phase 2" |
| **Erwartet** | Token-basiertes Budget-Enforcement vor LLM-Dispatch |
| **Beobachtet** | `ContextWindowBudgetManager` in `include/prompt_engineering/context_window_manager.h` + `src/prompt_engineering/context_window_manager.cpp` vollständig implementiert. `ITokenCounter`, `CharDivisionCounter` (BPE-Approximation), `PromptBudgetExceededError`, greedy Chunk-Selektion und Nutzungs-Callback vorhanden. |
| **ROADMAP-Status** | `[x]` – implementiert (v1.5.0) |

---

## 3. Chain-of-Thought Execution Tracer — nicht implementiert

| Feld | Wert |
|---|---|
| **Claim-Quelle** | `src/prompt_engineering/ROADMAP.md` §"Phase 3" + `include/prompt_engineering/FUTURE_ENHANCEMENTS.md` §"Chain-of-Thought Step Tracer Interface" |
| **Erwartet** | `IChainOfThoughtTracer` mit `onStepBegin(StepId)`, `onStepEnd(StepId, reasoning, duration)` (beide `noexcept`); `ChainOfThoughtBuilder::attachTracer()` |
| **Beobachtet** | `ChainOfThoughtBuilder` ist ein reiner String-Assembler; kein Tracer-Interface; kein Callback-Mechanismus; keine Latenz-Attribution pro Schritt |
| **Evidence (geprüfte Pfade)** | `include/prompt_engineering/chain_of_thought.h` (kein Tracer); `include/prompt_engineering/FUTURE_ENHANCEMENTS.md` §"Chain-of-Thought Step Tracer Interface" (alle `[ ]`) |
| **ROADMAP-Status** | `[?]` – geplant (Phase 3) |
| **Issue-Titelvorschlag** | `[prompt_engineering] Implement IChainOfThoughtTracer for per-step reasoning instrumentation` |
| **Label-Vorschläge** | `type:feature`, `priority:low`, `prompt_engineering`, `status:planned` |

---

## 4. Prompt A/B Experimentation Framework (Typed) — nicht implementiert

| Feld | Wert |
|---|---|
| **Claim-Quelle** | `src/prompt_engineering/ROADMAP.md` §"Phase 3" + `include/prompt_engineering/FUTURE_ENHANCEMENTS.md` §"Prompt A/B Experimentation Framework" |
| **Erwartet** | `IPromptABFramework` mit deterministischem `assignVariant(UserId, ExperimentKey)`, `listExperiments()`, konfigurierbaren Traffic-Splits |
| **Beobachtet** | A/B-Testing ist in `self_improvement_orchestrator.cpp` als `ABTest`-Klasse und `startABTest()`/`analyzeABTest()` implementiert; dies ist jedoch ein internes Optimierungs-A/B-Test, kein öffentliches `IPromptABFramework`; deterministische Varianten-Zuweisung nach User-ID fehlt |
| **Evidence (geprüfte Pfade)** | `src/prompt_engineering/self_improvement_orchestrator.cpp` Z.71–332 (ABTest implementiert); `include/prompt_engineering/FUTURE_ENHANCEMENTS.md` §"Prompt A/B Experimentation Framework" (alle `[ ]`) |
| **ROADMAP-Status** | `[?]` – geplant (Phase 3) |
| **Anmerkung** | Das interne A/B-Testing für Optimierungszyklen ist implementiert; das öffentliche Framework für Feature-Flag-artige Traffic-Splits ist nicht vorhanden |
| **Issue-Titelvorschlag** | `[prompt_engineering] Implement IPromptABFramework with deterministic per-user variant assignment` |
| **Label-Vorschläge** | `type:feature`, `priority:medium`, `prompt_engineering`, `status:planned` |

---

## 5. Automated Quality Regression Interface — nicht implementiert

| Feld | Wert |
|---|---|
| **Claim-Quelle** | `src/prompt_engineering/ROADMAP.md` §"Phase 3" + `include/prompt_engineering/FUTURE_ENHANCEMENTS.md` §"Automated Quality Regression Interface" |
| **Erwartet** | `IPromptQualityEvaluator` mit `evaluate(IPromptTemplate, QualityConfig) -> QualityReport`; zustandslose Evaluation ohne Persistierung von Rohdaten |
| **Beobachtet** | `PromptEvaluator` (implementiert) ist ein generischer Qualitäts-Scorer; kein `IPromptQualityEvaluator`-Interface; kein Regressions-Runner; kein `QualityReport`-Typ |
| **Evidence (geprüfte Pfade)** | `include/prompt_engineering/prompt_evaluator.h` (generischer Evaluator vorhanden, kein Regressions-Interface); `include/prompt_engineering/FUTURE_ENHANCEMENTS.md` §"Automated Quality Regression Interface" (alle `[ ]`) |
| **ROADMAP-Status** | `[?]` – geplant (Phase 3) |
| **Issue-Titelvorschlag** | `[prompt_engineering] Implement IPromptQualityEvaluator regression interface` |
| **Label-Vorschläge** | `type:feature`, `priority:medium`, `prompt_engineering`, `status:planned` |

---

## 6. Security-Features aus FUTURE_ENHANCEMENTS — teilweise offen

| Feld | Wert |
|---|---|
| **Claim-Quelle** | `src/prompt_engineering/FUTURE_ENHANCEMENTS.md` §"Security / Reliability" |
| **Erwartet** | PII-Detektion vor LLM-Transmission (`utils/pii_detector.cpp`); Template-Integritätshash; Tenant-Isolation im A/B-Framework |
| **Beobachtet** | `utils/pii_detector.cpp` und `utils/pii_pseudonymizer.cpp` existieren im Projekt (aber nicht in `src/prompt_engineering/`); Prompt-Templates haben keinen PII-Check-Aufruf; kein Integritäts-Hash für externe Templates; Tenant-Scoping nicht in `SelfImprovementOrchestrator` implementiert |
| **Evidence (geprüfte Pfade)** | `src/prompt_engineering/prompt_engineering_integration.cpp` (kein PII-Check-Aufruf); `src/prompt_engineering/FUTURE_ENHANCEMENTS.md` Security-Abschnitt (alle `[ ]`) |
| **ROADMAP-Status** | `[ ]` – offen, kein Target-Datum |
| **Issue-Titelvorschlag** | `[prompt_engineering] Wire PII detection and integrity hash into prompt template lifecycle` |
| **Label-Vorschläge** | `type:security`, `priority:high`, `prompt_engineering`, `status:planned` |

---

## Nicht als Issues betroffene Befunde (Hinweise)

### Issue-Referenzen nicht verifizierbar

Die ROADMAP enthält Issue- und PR-Referenzen (`#2428`, `#2534`, `#2429`), die auf externe
GitHub-Issues/PRs verweisen. Eine Überprüfung dieser Referenzen gegen das Repository war
zum Zeitpunkt des Reality-Checks nicht möglich. Die betreffenden ROADMAP-Einträge wurden
belassen, da die Code-Evidence (Quelldateien) für die markierten Features vorhanden ist.

### Test-Coverage < 80%

`ROADMAP.md` §"Production Readiness Checklist" markiert "Unit tests coverage > 80%"
als `[?]`. Eine Coverage-Messung wurde nicht durchgeführt. Angesichts von 22+ Testdateien
für ~14 Quelldateien ist eine Coverage > 80% wahrscheinlich, aber nicht nachgewiesen.

---

## Zusammenfassung

| # | Feature | Quelle | Kritikalität | Status |
|---|---|---|---|---|
| 1 | Typed Template DSL | ROADMAP Phase 2 | Mittel | `[?]` nicht begonnen |
| 2 | Context Window Budget Manager | ROADMAP Phase 2 | Hoch | ✅ implementiert (v1.5.0) |
| 3 | CoT Execution Tracer | ROADMAP Phase 3 | Niedrig | `[?]` geplant |
| 4 | Typed A/B Experimentation Framework | ROADMAP Phase 3 | Mittel | `[?]` geplant |
| 5 | Quality Regression Interface | ROADMAP Phase 3 | Mittel | `[?]` geplant |
| 6 | PII-Detektion & Template-Integritätshash | FUTURE_ENHANCEMENTS | Hoch | `[ ]` offen |
| 7 | Reflection Tuning + LLM-Adapter | ROADMAP Phase 3 | Hoch | ✅ implementiert (v1.5.0 / v1.6.0) |

*Alle Phase-1-ROADMAP-Einträge (`[x]`) sind durch vorhandene Implementierungsdateien
auf `develop` belegt. Phase-2- und Phase-3-Einträge sind korrekt als `[?]` markiert.*
