# TODO: KI-Workflow-Verbesserungen für ThemisDB

Dieses Dokument leitet sich direkt aus dem Strategiepapier
`.github/KI-optimiertes C++-Projekt auf GitHub.md` ab und listet konkrete
Umsetzungsaufgaben, geordnet nach Priorität und Aufwand.

Checkbox-Status: `[ ]` offen · `[~]` in Bearbeitung · `[x]` erledigt ·
`[?]` blockiert · `[!]` zu prüfen

---

## Phase 1 — VS Code / IDE-Integration (Sofort, geringer Aufwand)

### 1.1 `.vscode/settings.json` erstellen
- [x] `github.copilot.chat.codeGeneration.useInstructionFiles: true` setzen
  — aktiviert automatisches Laden aller `.github/instructions/*.instructions.md`
- [x] `C_Cpp.enableCppCodeEditingTools: true` setzen
  — schaltet GetSymbolInfo, GetSymbolReferences, GetSymbolCallHierarchy für
  Copilot frei (semantische Analyse statt reiner Textsuche)
- [x] `cmake.configureOnOpen: true` + `cmake.buildDirectory` auf Build-Preset
  zeigen lassen, damit Copilot Build-Konfiguration automatisch erkennt
- [x] `github.copilot.chat.localeOverride: "de"` optional, falls Team DE bevorzugt

Zieldatei: `.vscode/settings.json` (repo-versioniert via `.vscode.example/settings.json`)
Akzeptanzkriterium: Copilot nutzt bei Symbol-Suche die C++-Symboltools
statt `grep`; verifizierbar über Copilot Chat → `/explain` auf einem
overloaded Funktionsnamen.

### 1.2 `.vscode/tasks.json` — CMake-Tasks für Copilot-Verifikation
- [x] Task `cmake: configure` — ruft `cmake --preset linux-debug` auf
- [x] Task `cmake: build`     — ruft `cmake --build --preset linux-debug` auf
- [x] Task `cmake: test`      — ruft `ctest --preset linux-debug` auf
- [x] Tasks als `"group": {"kind": "build"/"test", "isDefault": true}`
  registrieren, damit Copilot via `"Fixe den Kompilierfehler und verifiziere
  mit einem Build"` direkt triggern kann

Zieldatei: `.vscode/tasks.json` (repo-versioniert via `.vscode.example/tasks.json`)
Abhängigkeit: CMakePresets.json (bereits vorhanden)

### 1.3 `.vscode/launch.json` — Debug-Konfigurationen
- [x] GDB/LLDB-Konfiguration für `main_server` anlegen
- [x] `preLaunchTask` auf den cmake-build-Task verknüpfen

Zieldatei: `.vscode/launch.json` (repo-versioniert via `.vscode.example/launch.json`)

---

## Phase 2 — Custom Instructions erweitern (Mittlerer Aufwand)

### 2.1 Neue Instruction-Datei: C++ Language Service Tools
- [x] Datei `.github/instructions/cpp-language-service-tools.instructions.md`
  anlegen (analog zu `cpp-best-practices.instructions.md`)
- [x] Inhalt mandatiert:
  - Immer `GetSymbolInfo_CppTools` statt Textstichwort-Suche
  - Immer `GetSymbolReferences_CppTools` vor Rename/Refactor
  - Immer `GetSymbolCallHierarchy_CppTools` bei Analyse von Aufrufketten
  - Absolute Pfade bei Tool-Parametern verwenden
  - Kein `grep` / `ripgrep` für C++-Symbol-Lookups
- [x] Glob-Pattern `**/*.{cpp,hpp,h,cc,cxx}` in der Datei registrieren
- [x] Vorlage: https://github.com/github/awesome-copilot/blob/main/instructions/cpp-language-service-tools.instructions.md

### 2.2 `copilot-instructions.md` um Modern-C++20/23-Mandate erweitern
- [x] `std::string_view` und `std::span` für Parameter-Übergabe ohne Kopien vorschreiben
- [x] C++20 Concepts für Template-Constraints mandatieren (kein `enable_if`)
- [x] Ranges-Bibliothek gegenüber Raw-Loop-Schleifen bevorzugen
- [x] Coroutinen-Abschnitt: Promise-Typ-Implementierung explizit dokumentieren
- [x] RAII-Abschnitt schärfen: `std::unique_ptr`/`std::shared_ptr` zwingend,
  kein `new`/`delete` ohne explizites Review-Flag

### 2.3 Prompt-Engineering-Leitfaden ergänzen
- [x] Neues Dokument `.github/copilot/PROMPT_ENGINEERING.md` anlegen
- [x] Enthält: Step-by-step Decomposition statt „implement feature X"
- [x] Enthält: Acceptance-Kriterien + Testfälle im Prompt als Best Practice
- [x] Enthält: Checkpoint-Strategie bei komplexen Agent-Läufen
- [x] Enthält: Beispiel-Prompts für Thread-sichere Queue, Socket-Handler,
  Token-Bucket-Algorithmus

---

## Phase 3 — KI-Kontext-Verzeichnisse (Mittlerer Aufwand)

### 3.1 `ai_context/` — Persistente Wissensbasis für KI-Agenten
- [x] Verzeichnis `ai_context/` im Root anlegen
- [x] `ai_context/README.md` mit Zweck und Ablage-Konventionen
- [x] Initiale ADRs aus `docs/research/architecture_decisions/` hierher
  verlinken oder symbolisch referenzieren
- [x] `ai_context/memory_management_policy.md` — RAII + Ownership-Regeln
  als für KI lesbares Dokument
- [x] `ai_context/api_contracts/` — öffentliche Header-API-Kontrakte als
  maschinenlesbare Markdown-Tabellen

### 3.2 `ai_working/` — Iterativer Entwurfsraum
- [x] Verzeichnis `ai_working/` anlegen
- [x] `.gitignore`-Einträge für `ai_working/*.tmp.*` und `ai_working/debug_*`
  hinzufügen (tempor. Agent-Entwürfe nicht committen)
- [x] `ai_working/README.md` — Regeln: kein produktiver Code hier,
  Inhalte vor PR-Merge nach `docs/` oder `src/` migrieren

---

## Phase 4 — CI/CD Statische Analyse (Höherer Aufwand, Governance beachten)

> **Wichtig:** Alle neuen Workflows müssen `.github/WORKFLOW_GUIDELINES.md`
> und `.github/WORKFLOW_REGISTRY.md` erfüllen. Start als
> `workflow_dispatch`-only oder non-blocking, bis Trigger-Qualität verifiziert.

### 4.1 clang-tidy + clang-format CI-Job
- [x] Neuen Job in bestehendem Workflow evaluieren (Präferenz vor neuem File)
  — Ergebnis: eigener Workflow günstiger; cmake-multi-platform.yml hat zu breite PR-Trigger,
    die governance-inkompatibel mit clang-tidy wären.
- [x] Falls eigener Workflow: `workflow_dispatch`-only für ersten Rollout;
  PR-Trigger mit enger `paths:`-Begrenzung erst nach Trigger-Qualitätsprüfung
- [x] SARIF-Output aktivieren und als GitHub Code Scanning Artefakt hochladen
  (nahtlose Integration in GitHub Security tab) — via `clang_tidy_to_sarif.py` +
  `github/codeql-action/upload-sarif@v3`
- [x] Erst als `workflow_dispatch`-only starten; nach Verifikation als
  optional non-blocking PR-Check aktivieren
  — Implementiert in `.github/workflows/08-quality_clang-tidy-analysis.yml`
- [x] Eintrag in `WORKFLOW_REGISTRY.md` + `WORKFLOW_GUIDELINES.md` pflegen
- [ ] Lokal verifizieren: `pwsh -NoProfile -File ./scripts/test-github-actions-local.ps1 -Mode all`

### 4.2 AddressSanitizer (ASan) in Nightly-Sweep integrieren
- [x] ASan-Build-Konfiguration in `CMakePresets.json` ergänzen
  (`-fsanitize=address -fno-omit-frame-pointer`)
- [x] ASan-Job in `07-quality_nightly-benchmark-sweep.yml` als zusätzlichen
  Step einfügen (schedule-only → kein PR-Trigger → Governance-konform)
- [x] Ziel: dynamische Prüfung auf Laufzeitfehler in KI-generiertem C++-Code

### 4.3 CodeQL-Workflow auf `paths:` eingrenzen
- [x] `security-codeql.yml` aktuell triggert auf alle PRs nach `develop` ohne `paths:`
  — prüfen ob dies WORKFLOW_GUIDELINES-konform ist
- [x] `paths:` auf `src/**`, `include/**` begrenzen (Docs/Meta-Änderungen
  lösen keinen CodeQL-Scan aus)
- [x] Concurrency-Block prüfen und ggf. ergänzen

---

## Phase 5 — PR Governance (Geringer bis mittlerer Aufwand)

### 5.1 `ai-generated` Label einführen
- [?] Label `ai-generated` in `.github/LABELS.md` und als GitHub-Label anlegen (Repo-Doku erledigt, GitHub-Label-Objekt noch manuell anzulegen)
- [x] `labeler.yml` Eintrag für automatische Vergabe erweitern:
  - Option A: PR-Body enthält `<!-- ai-generated -->` Marker
  - Option B: Änderungen in `ai_working/` → automatisch `ai-generated`
- [x] `CODEOWNERS` prüfen: KI-generierte PRs sollten Review durch Maintainer
  erfordern (kein Auto-Merge ohne Human-Approval)

### 5.2 `pull_request_template.md` — AI-Review-Checkliste ergänzen
- [x] Neuen Abschnitt `## KI-generierter Code` hinzufügen:
  - `[ ]` Code wurde mit `GetSymbolReferences_CppTools` auf vollständige
    Referenz-Abdeckung geprüft
  - `[ ]` Keine rohen Pointer / kein `new`/`delete` eingeführt
  - `[ ]` RAII und Exception-Safety verifiziert
  - `[ ]` Kein übermäßig cleverer / unnötig abstrakter Code (AI loves cleverness)
  - `[ ]` Performance-Metriken geprüft, falls Hotpath betroffen

### 5.3 Copilot Code Review im PR-Prozess aktivieren
- [ ] GitHub Copilot PR-Summary automatisch für PRs mit `ai-generated`-Label
  aktivieren (GitHub-Settings → Copilot → Code Review)
- [x] Dokumentieren in `.github/WORKFLOW_GUIDELINES.md` unter "PR Governance"

---

## Phase 6 — Agent-Konfiguration dokumentieren (Geringer Aufwand)

### 6.1 @Modernize Agent-Konfiguration
- [x] `.github/copilot/MODERNIZE_AGENT.md` anlegen
- [x] Beschreibt: Assessment-Phase (assessment.md), Plan-Phase (plan.md),
  Autonomous-Execution-Phase + Build-Verifikation
- [x] Listet Trigger-Szenarien: veraltete MSVC-Build-Tools, C++11/14-Konstrukte
  die nach C++20/23 modernisiert werden sollen
- [x] Verlinkt auf `ai_working/` als Staging-Bereich

### 6.2 @BuildPerfCpp Agent-Konfiguration
- [x] `.github/copilot/BUILD_PERF_AGENT.md` anlegen
- [x] Beschreibt: ETL-Trace-basierte Build-Insight-Analyse
- [x] Ziele: teure Header-Inklusionen, langsame Template-Instanziierungen,
  ineffiziente Funktionsgenerierung identifizieren
- [x] Integration mit bestehenden CMake-Targets dokumentieren

---

## Umsetzungsreihenfolge (Empfehlung)

| Prio | Item | Aufwand | Impact |
|------|------|---------|--------|
| 1 | Phase 1: `.vscode/settings.json` + `tasks.json` | Klein | Hoch |
| 2 | Phase 2.1: `cpp-language-service-tools.instructions.md` | Klein | Hoch |
| 3 | Phase 5.1: `ai-generated` Label + labeler | Klein | Mittel |
| 4 | Phase 5.2: PR-Template KI-Checkliste | Klein | Mittel |
| 5 | Phase 2.2: copilot-instructions.md Modern-C++23 | Mittel | Hoch |
| 6 | Phase 3.1+3.2: `ai_context/` + `ai_working/` | Mittel | Mittel |
| 7 | Phase 4.1: clang-tidy/clang-format CI-Job | Groß | Hoch |
| 8 | Phase 4.2: ASan Nightly | Mittel | Hoch |
| 9 | Phase 4.3: CodeQL `paths:` eingrenzen | Klein | Mittel |
| 10 | Phase 6: Agent-Konfigurationsdocs | Klein | Niedrig |
| 11 | Phase 2.3: Prompt-Engineering-Leitfaden | Mittel | Mittel |
| 12 | Phase 1.3: `launch.json` | Mittel | Niedrig |

---

## Referenz-Dokument

Alle Aufgaben basieren auf:
`.github/KI-optimiertes C++-Projekt auf GitHub.md`

Externe Referenzen (aus dem Dokument):
- https://code.visualstudio.com/docs/cpp/cpp-devtools
- https://github.com/github/awesome-copilot/blob/main/instructions/cpp-language-service-tools.instructions.md
- https://devblogs.microsoft.com/cppblog/c-symbol-context-and-cmake-build-configuration-awareness-for-github-copilot-in-vs-code/
- https://learn.microsoft.com/en-us/cpp/porting/copilot-app-modernization-cpp
