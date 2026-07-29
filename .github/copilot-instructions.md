# Copilot Instructions for Roadmap-Driven Implementation

Diese Regeln steuern, wie Copilot in diesem Repository aus `ROADMAP.md` und `FUTURE_ENHANCEMENTS.md` produktive Implementierungen erzeugt.

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
8. Private plugin work must stay inside the intended public/private boundary: public SDK and governance changes may land in this repository, but private implementation work belongs only in the provisioned `plugins/private/*` family submodules.
9. Community guardrails are mandatory: no private credentials, private sources, or private artefacts in Community/Minimal workflows, packaging, or docs.

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

## 9) Ollama Delegation Bridge — AI Agent Instructions

The repository ships a VS Code extension at `tools/copilot-ollama-router/` that
routes code-generation tasks to an **Ollama endpoint (local or remote)** instead of Copilot
cloud for selected workloads. Every AI agent (Copilot, GitHub Copilot in VS Code, this coding agent)
MUST follow these rules when working on ThemisDB:

### 9.1 When to use Ollama (via `@ollama` chat participant)

Use `@ollama` for all of the following:

| Task | Example prompt |
|---|---|
| C++ boilerplate / class scaffolding | `@ollama Generate a RAII wrapper for FILE*` |
| Unit test generation | `@ollama Write GTest cases for KnowledgeGraphRetriever::neighbours()` |
| Refactoring | `@ollama Refactor this function to use std::expected` |
| Documentation / API comments | `@ollama Update API documentation comments for LLMPluginManager::loadModel()` |
| CMakeLists.txt edits | `@ollama Add a new test target for test_foo.cpp` |
| Repetitive code patterns | `@ollama Implement getters/setters for all fields in struct X` |

### 9.2 When to use Copilot cloud (standard chat)

Use the normal Copilot chat (no `@ollama`) for:

- Security review and vulnerability analysis
- Architecture and design decisions
- Complex multi-file debugging
- Code review with quality judgement
- Any task requiring up-to-date knowledge (CVEs, new APIs)

### 9.3 ThemisDB-specific routing rules (always enforced)

- **C++ files** (`.cpp`, `.hpp`, `.h`, `.cc`) → always `@ollama /local`
- **Security / audit** prompts → always standard Copilot (cloud)
- **CMakeLists.txt / build system** → `@ollama`
- **ROADMAP.md / FUTURE_ENHANCEMENTS.md updates** → standard Copilot

Note for this repository setup:
- The currently configured Ollama endpoint is remote: `http://192.168.178.106:11434`.
- `@ollama /local` continues to mean "route to the configured Ollama endpoint" (not necessarily localhost).

### 9.4 First-time setup

Before using `@ollama`, ensure Ollama is running and at least one coding
model is installed:

```bash
# 1. Install Ollama (https://ollama.com)
ollama serve &

# 2. Pull recommended models via VS Code command palette:
#    Command: "Ollama Bridge: Set Up / Download Coding Models"
#    Command ID: ollamaBridge.setupModels
#    → selects from ranked list (DeepSeek-Coder-V2, Qwen2.5-Coder, CodeLlama…)

# 3. Verify connectivity and list installed models:
#    Command: "Ollama Bridge: Check Ollama Connection"
#    Command ID: ollamaBridge.checkOllamaHealth
#    Command: "Ollama Bridge: List Installed Models"
#    Command ID: ollamaBridge.listInstalledModels
```

Or from the terminal:

```bash
# Currently installed on remote (192.168.178.106:11434):
ollama pull qwen2.5-coder:14b          # primary coding + tool-use (edit/apply)
ollama pull deepseek-coder-v2:16b      # heavy refactors + CMake (no tool-use)
ollama pull gemma4:latest              # primary agent/chat + tool-use
ollama pull phi4:latest                # reasoning/text (no tool-use)
ollama pull codestral:latest           # autocomplete only
ollama pull MHKetbi/Unsloth-Phi-4-mini-instruct:latest  # lightweight chat + tool-use
```

**Tool-calling support** (verified against remote endpoint):
- ✅ `qwen2.5-coder:14b`, `gemma4:latest`, `Unsloth-Phi-4-mini` — full tool-use
- ❌ `deepseek-coder-v2:16b`, `phi4:latest`, `codestral:latest` — no tool-use (edit/autocomplete only)

The ranked model catalog is maintained in
`tools/copilot-ollama-router/src/modelSetup.ts` → `RANKED_CODING_MODELS`.

### 9.5 Delegation mode configuration

Set in VS Code `settings.json` (workspace or user):

```jsonc
{
  "copilotOllamaRouter.endpoint": "http://192.168.178.106:11434",
  "copilotOllamaRouter.delegationMode": "auto",
  "copilotOllamaRouter.defaultModel": "deepseek-coder-v2:16b",
  "copilotOllamaRouter.reasoningModel": "phi4:latest",
  "copilotOllamaRouter.copilotReviewEnabled": true,
  "copilotOllamaRouter.themisDbRules": true,
  "copilotOllamaRouter.requestTimeoutMs": 120000,
  "copilotOllamaRouter.contextTokenBudget": 6144
}
```

### 9.6 How agents instruct the bridge

When writing automation scripts, CI steps, or agent prompts that need to
trigger Ollama delegation programmatically:

- Use the VS Code command `ollamaBridge.delegateToOllama` (prompts for input)
- Use the chat participant: `@ollama /local <task description>`
- Use `ollamaBridge.autoRoute` to preview the routing decision without executing

Routing defaults used in this repo:
- C/C++/Python edits: `qwen2.5-coder:14b`
- CMake + heavy generic prompts: `deepseek-coder-v2:16b`
- Markdown/JSON/YAML reasoning: `phi4:latest`

Copilot coding agents running in this repository SHOULD annotate their
task descriptions with the target destination so the delegation router
can be bypassed efficiently:

```
// ROUTING HINT: ollama-local
// Model: deepseek-coder-v2:16b
// Reason: C++ boilerplate generation
```


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
