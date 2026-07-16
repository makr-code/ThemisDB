# Prompt Engineering Modul — Fehlende / Unvollständige Implementierungen

<!-- Status: current | validated: 2026-04-06 -->
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

## 3. Chain-of-Thought Execution Tracer — ✅ implementiert (v1.7.0)

| Feld | Wert |
|---|---|
| **Claim-Quelle** | `src/prompt_engineering/ROADMAP.md` §"Phase 3" + `include/prompt_engineering/FUTURE_ENHANCEMENTS.md` §"Chain-of-Thought Step Tracer Interface" |
| **Erwartet** | `IChainOfThoughtTracer` mit `onStepBegin(StepId)`, `onStepEnd(StepId, reasoning, duration)` (beide `noexcept`); `ChainOfThoughtBuilder::attachTracer()` |
| **Beobachtet** | Vollständig implementiert in `include/prompt_engineering/cot_tracer.h` + `src/prompt_engineering/cot_tracer.cpp`. `ChainOfThoughtBuilder::attachTracer()` / `detachTracer()` / `hasTracer()` vorhanden; `build()` feuert Callbacks pro Schritt. |
| **ROADMAP-Status** | `[x]` – implementiert (v1.7.0) |

---

## 4. Prompt A/B Experimentation Framework — ✅ implementiert (v1.9.0)

| Feld | Wert |
|---|---|
| **Claim-Quelle** | `src/prompt_engineering/ROADMAP.md` §"Phase 3" + `include/prompt_engineering/FUTURE_ENHANCEMENTS.md` §"Prompt A/B Experimentation Framework" |
| **Erwartet** | Deterministische Varianten-Zuweisung per `request_id`, konfigurierbarer Traffic-Split, Auto-Promotion des Gewinners |
| **Beobachtet** | Vollständig implementiert: `PromptABExperimentFramework` (`include/prompt_engineering/prompt_ab_experiment.h`); MurmurHash3-32 Variant-Assignment; Welch-t-Test für Signifikanz; Auto-Promotion ab `min_samples`; exception-sicherer `WinnerCallback` |
| **ROADMAP-Status** | `[x]` – implementiert (v1.9.0) |

---

## 5. Automated Quality Regression Interface — ✅ implementiert (v1.8.0)

| Feld | Wert |
|---|---|
| **Claim-Quelle** | `src/prompt_engineering/ROADMAP.md` §"Phase 3" + `include/prompt_engineering/FUTURE_ENHANCEMENTS.md` §"Automated Quality Regression Interface" |
| **Erwartet** | Regression-Harness um `PromptEvaluator`; Vergleich Kandidat vs. Baseline auf goldenen Fixtures; Block-Gate wenn Score >5 % fällt |
| **Beobachtet** | Vollständig implementiert: `PromptRegressionRunner` (`include/prompt_engineering/prompt_regression_runner.h`); `RegressionFixture`, `RegressionConfig`, `RegressionResult`, `FixtureDelta`; `loadFeedbackFixtures()` integriert `FeedbackCollector`; `run()` berechnet `delta_pct`, `is_regression`, `blocked`; Log-Callback für strukturiertes Logging |
| **ROADMAP-Status** | `[x]` – implementiert (v1.8.0) |

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
| 3 | CoT Execution Tracer | ROADMAP Phase 3 | Niedrig | ✅ implementiert (v1.7.0) |
| 4 | Typed A/B Experimentation Framework | ROADMAP Phase 3 | Mittel | ✅ implementiert (v1.9.0) |
| 5 | Quality Regression Interface | ROADMAP Phase 3 | Mittel | ✅ implementiert (v1.8.0) |
| 6 | PII-Detektion & Template-Integritätshash | FUTURE_ENHANCEMENTS | Hoch | `[ ]` offen |
| 7 | Reflection Tuning + LLM-Adapter | ROADMAP Phase 3 | Hoch | ✅ implementiert (v1.5.0 / v1.6.0) |

*Alle Phase-1-ROADMAP-Einträge (`[x]`) sind durch vorhandene Implementierungsdateien
auf `develop` belegt. Phase-2- und Phase-3-Einträge sind korrekt als `[?]` markiert.*
