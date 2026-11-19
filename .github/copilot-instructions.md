---
Author: "ThemisDB Team"
Created: "2026-10-09"
Last Updated: "2026-11-19"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "unknown"
---

Exact model backend not verifiable from final document text alone.

# Copilot Instructions for Roadmap-Driven Implementation

Diese Regeln steuern, wie Copilot in diesem Repository aus `ROADMAP.md` und `FUTURE_ENHANCEMENTS.md` produktive Implementierungen erzeugt.

## 0) Wiki-Nutzung für alle Coding-AI-Agenten (MUST)

Copilot, Claude und vergleichbare Coding-AI-Agenten müssen die Repository-Wiki als Pflichtkontext verwenden.

Pflichtregeln:

1. Vor Implementierung oder Refactoring zuerst die relevante Wiki-Seite prüfen (insbesondere Architektur-, Betriebs-, Modul- und Governance-Seiten).
2. Bei Konflikten zwischen verstreuten Dokuquellen gilt die in der Wiki verlinkte Source-of-Truth-Reihenfolge.
3. Änderungen, die Architektur, Betrieb, Prozesse oder Governance betreffen, müssen die zugehörigen Wiki-Inhalte synchron halten.
4. Fehlende Wiki-Inhalte sind als Lücke zu melden und in den Arbeitsartefakten/PR-Hinweisen zu markieren statt stillschweigend zu ignorieren.

## 0.1) Developer LLM Wiki fuer GitHub Copilot auf GitHub (MUST)

Wenn GitHub Copilot als Coding Agent auf GitHub implementiert, reviewt oder refactort, muss es das Developer LLM Wiki aktiv als Vibe-Coding-Kontext nutzen.

Pflichtablauf:

1. Zuerst `AI_WIKI_INTEGRATION_PLAYBOOK.md` lesen.
2. Vor Codeaenderungen die relevanten Developer-Wiki-Artefakte aus `ai_context/developer_llm_wiki/` konsultieren:
   - `INDEX.md`
   - `MODULES_AND_APIS.md`
   - `BUILD_TEST_CI_AND_OPERATIONS.md`
   - `GOVERNANCE_AND_ROADMAP.md`
3. `ai_context/developer_llm_wiki/WIKI_STATUS.json` als Frische-/Health-Signal pruefen (`generated_at`, `source_count`, Delta-Status).
4. Wiki-Kontext nach Task-Typ priorisieren:
   - API-/Modularbeit → `MODULES_AND_APIS.md`
   - Build/CI/Test → `BUILD_TEST_CI_AND_OPERATIONS.md`
   - Governance/Roadmap/Release → `GOVERNANCE_AND_ROADMAP.md`
   - C/C++-Implementierung → `MODULES_AND_APIS.md` plus `memory_management_policy.md`, `OOP_AND_SOC_PRINCIPLES.md`, `FUNCTION_CLASSIFICATION.md`
   - allgemeiner Einstieg/Link-Hub → `INDEX.md`
5. Bei Konflikten gilt: Root-SOT und Modulquellen vor kompilierter Wiki-Synthese.
6. Wenn Wiki-Artefakte fehlen, stale wirken oder der Aufgabe widersprechen, diese Luecke explizit nennen statt das Wiki still zu ignorieren.

Task-spezifische Mindestregeln:

- API-/Modularbeit: zuerst `ai_context/developer_llm_wiki/MODULES_AND_APIS.md`, danach relevante `ai_context/api_contracts/*.md` und modulnahe `src/<module>/*.md`.
- Build/CI/Test: zuerst `ai_context/developer_llm_wiki/BUILD_TEST_CI_AND_OPERATIONS.md`; bei Fehlern zusaetzlich immer konkrete GitHub-Actions-Logs und betroffene Workflow-Dateien pruefen.
- Governance/Roadmap/Release: zuerst `ai_context/developer_llm_wiki/GOVERNANCE_AND_ROADMAP.md`, danach Root-SOT-Dateien (`ROADMAP.md`, `FUTURE_ENHANCEMENTS.md`, `RELEASE_STRATEGY.md`, `BRANCHING_STRATEGY.md`, `VERSIONING.md`).
- C/C++-Arbeit: zuerst `ai_context/developer_llm_wiki/MODULES_AND_APIS.md`, danach `ai_context/memory_management_policy.md`, `ai_context/OOP_AND_SOC_PRINCIPLES.md`, `ai_context/FUNCTION_CLASSIFICATION.md` sowie die einschlaegigen `.github/instructions/*cpp*` Regeln.
  - Public API / Header-Vertraege: zusaetzlich zuerst relevante `ai_context/api_contracts/*.md` und Doxygen-/Thread-Safety-/Ownership-Pflichten pruefen.
  - Internal Core / Implementierungslogik: Ownership, RAII, `detail::`-Grenzen und unbeabsichtigte API-Flaechenerweiterungen gegen `memory_management_policy.md` und `OOP_AND_SOC_PRINCIPLES.md` pruefen.
  - Concurrency / Performance: Locking, Atomics, Hot-Path-Klassifikation, Timeout/Cancellation und Benchmark-Bezug gegen `FUNCTION_CLASSIFICATION.md` und C++-Best-Practice-Regeln pruefen.
  - Plugin-Boundaries / Extensibility: Adapter-/Interface-Grenzen sowie Public-vs-Private-Grenzen gegen `OOP_AND_SOC_PRINCIPLES.md` und `GOVERNANCE_AND_ROADMAP.md` pruefen.
  - Einordnungshilfe:
    - `include/**` oder externe Vertragsaenderung → Public API
    - `src/**`, interne Header oder `detail::` → Internal Core
    - Locking/Atomics/Latenz/Benchmarks/Timeouts → Concurrency / Performance
    - Plugin-Interfaces/Adapter/Edition-Grenzen → Plugin-Boundaries / Extensibility
  - Wenn mehrere C++-Subtypen zugleich betroffen sind, gelten alle relevanten Wiki-/Governance-Pflichten kumulativ.

Details und Dateizwecke: [AI_WIKI_CONTEXT.md](copilot/AI_WIKI_CONTEXT.md)

## 1) Ziel

Roadmap-Einträge müssen so konkret sein, dass Copilot **produktiven Sourcecode** statt Stub/Rumpf erzeugen kann.

## 1.1) Branch / Edition Governance (MUST)

Copilot und andere AI-Agenten müssen das kanonische Branch-Modell aus `BRANCHING_STRATEGY.md` verwenden.

Kanonische permanente Branches:

- `develop`
- `minimal`
- `community`
- `enterprise`
- `hyperscaler`
- `military`

Legacy-Namen:

- `main` → ersetzt durch `community`
- `millitary` → ersetzt durch `military`

Regeln:

1. Normale Implementierungsarbeit zielt auf `develop`, sofern der Benutzer nicht ausdrücklich editionsspezifische Arbeit verlangt.
2. Community-Release-Arbeit zielt auf `community`, niemals auf `main`.
3. Military-Release-Arbeit zielt auf `military`, niemals auf `millitary`.
4. Keine neuen PRs, Branches, Automationen oder Doku-Anweisungen gegen Legacy-Branch-Namen vorschlagen.
5. Wenn eine Aufgabe Branch-Governance, Release-Fluss oder Editions-Mapping ändert, müssen diese Dateien zusammen aktualisiert werden:
  - `BRANCHING_STRATEGY.md`
  - `RELEASE_STRATEGY.md`
  - `DOCUMENTATION_GOVERNANCE.md`
  - `.github/copilot-instructions.md`
  - `ai_context/COPILOT_INSTRUCTIONS.md`
  - `VERSIONING.md`
  - `CHANGELOG.md`
  - `ROADMAP.md`
  - `FUTURE_ENHANCEMENTS.md`

## 1.2) Documentation Governance Sync (MUST)

For documentation-related work, treat these as one aligned rule set:

- `DOCUMENTATION_GOVERNANCE.md`
- `.github/copilot-instructions.md`
- `ai_context/COPILOT_INSTRUCTIONS.md`
- `.github/ISSUE_TEMPLATE/docs_audit.md`
- `.github/MODULE_DEVELOPER_DOCUMENTATION_GUIDELINES.md`
- `.github/copilot/module-quick-rules.md`

Module-level documentation and source expectations are defined in this repository contract: [.github/MODULE_DEVELOPER_DOCUMENTATION_GUIDELINES.md](MODULE_DEVELOPER_DOCUMENTATION_GUIDELINES.md). The short operational version is in [.github/copilot/module-quick-rules.md](copilot/module-quick-rules.md). Copilot must treat both as the default structure baseline for module README/ARCHITECTURE/ROADMAP and for source-level Doxygen, path ownership, and issue/action packet compliance.

Mandatory rules:

1. Source precedence and SOT domain mapping follow `DOCUMENTATION_GOVERNANCE.md`.
2. Naming follows repository reality per scope/directory (no abstract global style).
3. ThemisDB default for module-adjacent docs is UPPER_SNAKE naming unless local canon differs.
4. No semantic filename duplicates in one scope (for example `ARCHITECTURE.md` + `architecture.md`).
5. Enforce conformance checks on each docs change: naming, structure, duktus, SOT consistency.
6. Keep instructions compact and non-redundant to control token size; do not remove normative constraints.
7. For beta-to-GA or release-hardening updates, keep the gate model aligned across `ROADMAP.md`, `FUTURE_ENHANCEMENTS.md`, `RELEASE_STRATEGY.md`, `VERSIONING.md`, and `CHANGELOG.md`:
   - Wave 7 baseline PASS
   - `release_critical` on `develop`
   - top-risk module sign-off (`server`, `llm`, `sharding`)
   - resilience/security/operations evidence (Wave 5/6 retention, Wave 8 or equivalent, chaos, sanitizer/recovery, penetration test, SLA, runbooks)
8. Private plugin work must stay inside the intended public/private boundary: public SDK and governance changes may land in this repository, but private implementation work belongs only in the provisioned `plugins/private/*` plugin-named submodules.
9. When naming private plugin repositories or submodule paths, prefer names that mirror the current plugin names so the repository-to-plugin mapping stays obvious.
10. Community guardrails are mandatory: no private credentials, private sources, or private artefacts in Community/Minimal workflows, packaging, or docs.

## 2) Pflichtstruktur für `ROADMAP.md` je Modul

Jede Modul-Roadmap MUSS diese Abschnitte enthalten:

1. `## Current Status`
2. `## In Progress` und/oder `## Planned Features`
3. `## Implementation Phases` mit `### Phase 1 ... ### Phase N`
4. `## Production Readiness Checklist`
5. `## Known Issues & Limitations`
6. `## Breaking Changes` (falls relevant)

### 2.1 Checkbox-Status

- `[ ]` offen
- `[~]` in Bearbeitung
- `[x]` erledigt
- `[I]` Issue vorhanden
- `[P]` Pull Request vorhanden
- `[?]` Human question/blockiert
- `[!]` unklarer/zu prüfender Zustand

### 2.2 Aufgabenformat (Pflicht)

Jede umzusetzende Aufgabe muss als Checkbox vorliegen und nach Möglichkeit einen Target-Hinweis haben:

- `- [ ] <konkrete technische Aufgabe> (Target: <Milestone/Quartal>)`

Beispiel:

- `- [ ] CUDA geospatial distance and containment kernels (Target: Q3 2026)`

## 3) Pflichtstruktur für `FUTURE_ENHANCEMENTS.md`

Wenn vorhanden, MUSS die Datei pro Modul klare, implementierbare Hinweise enthalten:

```markdown
## <module-name>

### Scope
- ...

### Design Constraints
- ...

### Required Interfaces
- ...

### Implementation Notes
- ...

### Test Strategy
- ...

### Performance Targets
- ...

### Security / Reliability
- ...
```

Regel: keine vagen Formulierungen wie „improve“, „optimize“ ohne messbares Ziel.

## 4) Qualitätsanforderungen für Roadmap-Items

Jedes implementierbare Item soll enthalten:

- betroffene Subsysteme/Dateien/Namespaces
- erwartetes Laufzeitverhalten
- Fehlerfälle und Validierung
- Testanforderungen (Unit/Integration)
- messbare Performance-Ziele (wo relevant)
- Kompatibilitäts-/Migrationshinweise

## 5) Phasenmodell (verbindlich)

`## Implementation Phases` muss mindestens folgende Phasen abdecken:

- Phase 1: Design / API-Vertrag
- Phase 2: Core-Implementierung
- Phase 3: Fehlerbehandlung & Edge Cases
- Phase 4: Tests
- Phase 5: Performance/Hardening
- Phase 6: Dokumentation & Abnahme

Jede Phase braucht konkrete Bullet-Tasks, keine Platzhalter.

## 6) Copilot-Ausführungsregeln

Beim Implementieren aus Roadmap/Future-Enhancement gilt:

1. Keine Stub-Methoden ohne Produktionslogik.
2. Kein rein syntaktischer "TODO-Code" als Endergebnis.
3. Tests müssen reale Funktionalität verifizieren.
4. Akzeptanzkriterien aus Roadmap sind bindend.
5. Bei fehlenden Details zuerst Roadmap/Future-Enhancement präzisieren statt raten.
6. Wenn Stubs, Mock-Pfade oder Simulationen im Sourcecode erforderlich sind, müssen sie explizit dokumentiert werden (Zweck, Aktivierungsbedingungen, Unterschiede zur Produktionslogik, geplanter Entfernungspfad).

## 7) Beispiel für guten Roadmap-Eintrag

```markdown
- [ ] CUDA geospatial distance and containment kernels (Target: Q3 2026)
  - Inputs: WGS84 points/polygons, batch-size up to 1e6
  - Outputs: distance matrix + containment bitset
  - Constraints: deterministic FP tolerance <= 1e-6
  - Errors: invalid geometry, NaN coordinates, overflow
  - Tests: unit + property-based + GPU/CPU parity
  - Perf: >= 8x speedup vs CPU baseline on RTX-class GPU
```

Dieser Detaillierungsgrad ist für produktiven Code erforderlich.

## 8) C++ Best Practices (VS Code/Copilot)

Bei C++-Aufgaben gelten folgende Leitlinien fuer Generierung, Review und Refactoring:

```yaml
cpp_best_practices:
  modern_cpp_features:
    - "Use 'auto' for type inference to improve readability."
    - "Prefer 'nullptr' over NULL or 0."
    - "Use 'constexpr' for compile-time computations."
    - "Use range-based for loops for container iteration."
    - "Prefer std::string_view and std::span for read-only/view-style parameter passing to avoid unnecessary copies."
    - "Prefer C++20 ranges (std::ranges) for composable transformations/filtering over ad-hoc raw loops when readability is improved."
    - "Prefer smart pointers (std::unique_ptr, std::shared_ptr) over raw pointers."

  resource_management_raii:
    - "Use RAII to bind resource lifetime to object lifetime."
    - "Use std::lock_guard or std::unique_lock for mutex locking."
    - "Avoid manual new/delete; prefer smart pointers (std::unique_ptr/std::shared_ptr)."
    - "Any intentional new/delete usage is review-exception only and must be declared in the PR under the AI-Generated Code checklist plus explicit justification in the PR body."
    - "Ensure resources are released automatically when objects go out of scope."

  avoid_unnecessary_copies:
    - "Pass large objects by const reference (const &)."
    - "Use move semantics (std::move) for efficient transfer of ownership."
    - "Implement copy and move constructors appropriately."

  template_constraints:
    - "Use C++20 concepts/requires clauses for template constraints; avoid std::enable_if-based SFINAE in new code."

  coroutines:
    - "When introducing coroutines, document promise type responsibilities explicitly (lifecycle, allocation, suspension points, error propagation)."
    - "Coroutine APIs must state cancellation and exception behavior in interface documentation."

  clear_and_safe_interfaces:
    - "Mark member functions that do not modify state as 'const'."
    - "Document side effects and exceptions clearly."
    - "Avoid global variables and mutable shared state."

  threading_and_synchronization:
    - "Use std::mutex with std::lock_guard for critical sections."
    - "Keep critical sections as short as possible."
    - "Use atomic operations (std::atomic) when feasible."
    - "Avoid deadlocks by consistent lock ordering or using std::lock."

  error_handling:
    - "Use exceptions for errors that cannot be handled locally."
    - "Write exception-safe code."
    - "Avoid silent failures and undefined behavior."

  performance:
    - "Profile code before optimizing."
    - "Avoid premature optimization."
    - "Use cache-friendly data structures and algorithms."
    - "Avoid false sharing in multithreaded code."

  style_and_readability:
    - "Use meaningful and descriptive names."
    - "Keep functions short and focused."
    - "Follow a consistent coding style guide."

  copilot_guidance:
    - "When generating C++ code, use modern language features like 'auto', smart pointers, 'constexpr', concepts, and ranges where appropriate."
    - "Avoid manual memory management; prefer RAII with std::unique_ptr/std::shared_ptr; treat raw new/delete as review-exception only."
    - "For RocksDB Open APIs (Open/OpenForReadOnly/open-with-column-families), do not pass std::unique_ptr<rocksdb::DB>* as out-parameter. Use portable ABI-safe pattern: rocksdb::DB* db_raw = nullptr; call Open(..., &db_raw); then transfer ownership via std::unique_ptr<rocksdb::DB>(db_raw)."
    - "Synchronize threads with std::mutex and std::lock_guard; keep critical sections short."
    - "Prevent deadlocks by consistent locking order."
    - "Write clear, const-correct, exception-safe functions; prefer std::string_view/std::span for non-owning parameters."
    - "Optimize only after profiling and consider cache friendliness; prefer std::ranges pipelines over manual loops when they improve clarity."
    - "If stubs, mocks, or simulation paths are introduced in source code, document them explicitly (purpose, activation conditions, production delta, and removal plan)."
```

  Empfohlenes Kommentar-Template direkt ueber Stub/Mock/Simulation-Codepfaden:

  ```cpp
  // STUB/SIMULATION NOTE:
  // Purpose: <why this non-production path exists>
  // Activation: <build flag/runtime condition/test-only gate>
  // Production Delta: <how behavior differs from production>
  // Removal Plan: <when/how this path will be removed>
  ```

## 9) Optional local model router

The VS Code extension in `tools/copilot-ollama-router/` is optional. Its
chat participant and routing commands may be used when explicitly selected
and configured by the developer. Repository tasks do not require a local
Ollama endpoint, model, routing hint, or transfer to a separate service.
Normal Copilot workflows remain supported.


## 10) Mandatory documentation enforcement for AI-assisted C++ work

Documentation is a required quality artifact, not an optional add-on.

### 10.1 Mandatory documentation rules (Doxygen-aware, no CI/CD dependency)

- Every new or changed public C++ API MUST include API-facing documentation.
- Documentation MUST cover purpose, parameter expectations, return behavior (if applicable), and failure/edge-case behavior.
- For in-code C++ API docs, prefer Doxygen-compatible tags (`@brief`, `@param`, `@return`, `@throws`) where applicable.
- For templates/concepts, semantic requirements must be documented in plain language; use `@tparam`/`@requires` when using Doxygen-style comments.
- Comments must explain intent and constraints ("why"), not only paraphrase implementation details ("what").
- Refactoring changes MUST update affected documentation in the same change.

### 10.2 Intent and edge-case documentation

- Document failure and edge-case behavior explicitly (invalid input, timeout, cancellation, empty state).
- For APIs with ownership/lifetime contracts, state those contracts explicitly.

### 10.3 Tooling and semantic context

- Continue using C++ language-service tools for symbol work (`GetSymbolInfo_CppTools`, `GetSymbolReferences_CppTools`, `GetSymbolCallHierarchy_CppTools`).
- VS Code semantic assistance should keep `C_Cpp.enableCppCodeEditingTools: true` and `C_Cpp.codeAnalysis.runAutomatically: true` enabled.

### 10.4 Governance expectation (no CI/CD gate dependency)

- Documentation gaps are review blockers and must be fixed before merge.
- README/API docs must stay synchronized with architecture and behavior changes.
- Enforcement is achieved via prompt rules, review checklists, and manual sign-off.

### 10.5 Mandatory AI provenance declaration for generated documentation

When a document, section, or generated artifact was created or materially edited by an AI system, the responsible AI MUST declare that fact in the document metadata and, when relevant, in a short provenance note. This is a repository-wide documentation convention for Copilot, Claude, Ollama, and other AI agents.

Required metadata keys:

- `AI_Used`: `true` or `false`
- `AI_Models`: comma-separated model names or roles, or `unknown`
- `AI_Contributor`: model/agent name or `unknown`
- `Human_Review`: `true` or `false`
- `Human_Author`: human maintainer or contributor name

Example YAML block:

```yaml
---
Author: "ThemisDB Team"
Created: "2026-10-17"
Last Updated: "2026-10-17"
Status: "active"
AI_Used: true
AI_Models: "GitHub Copilot, Ollama/qwen2.5-coder:14b"
AI_Contributor: "GitHub Copilot"
Human_Review: true
Human_Author: "ThemisDB Team"
---
```

If the exact model backend cannot be determined with certainty, the document must state this explicitly instead of claiming a specific model with false certainty:

- "Exact model backend not verifiable from final document text alone."
- "AI-assisted generation; model attribution is approximate and not guaranteed."

This requirement applies to Markdown docs, architecture notes, roadmap entries, public docs, and any AI-generated narrative content in this repository.

## 11) Legacy-Code Governance (mandatory)

Legacy code paths are forbidden by default in AI-generated changes.

### 11.1 Default rule

- No new legacy/compatibility fallback paths without explicit prior human approval.
- "No approval" means: do not add legacy branches, dual APIs, or hidden migration shims.

### 11.2 Approval requirement

- A human maintainer must explicitly approve legacy introduction in the same task/PR context.
- Approval must include scope and sunset/removal expectation.

### 11.3 Mandatory marking when approved

- Every approved legacy path MUST be clearly marked in code comments with:
  - reason/business need
  - activation conditions
  - behavior delta to the primary path
  - owner and removal target (date/milestone)

Recommended marker template:

```cpp
// LEGACY PATH (requires human approval)
// Reason: <why needed>
// Activation: <when active>
// Primary Delta: <difference to primary path>
// Approved By: <name/role + reference>
// Removal Target: <date/milestone>
```

### 11.4 PR policy

- Unmarked legacy code or legacy code without explicit human approval is a hard review blocker.
- AI agents must prefer migration to the canonical API/path over adding compatibility aliases.

### 11.5 Non-production path ban (Simulation/Stub/Mockup)

- Simulations, stubs, and mockup code paths are forbidden by default in production code changes.
- Such paths are only allowed after explicit human approval and must be marked by a human with purpose, activation, production delta, approver reference, and removal target.
- AI-generated unapproved or unmarked Simulation/Stub/Mockup code is a hard blocker and must be removed before merge.
