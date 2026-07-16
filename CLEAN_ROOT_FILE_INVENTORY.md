# CLEAN Root File Inventory

Stand: 2026-06-25

Ziel dieser Datei:
- Vollstaendige Uebersicht aller Dateien im Repository-Root.
- Klare Beschreibung der Aufgabe jeder Datei.
- Schnellere Entscheidungen fuer kuenftige Aufraeumrunden.

Legende Empfehlung:
- KEEP_ROOT: Sollte im Root bleiben (Entry-Point, zentrale Governance, Build/Tooling).
- MOVE_OPTIONAL: Kann in einen Fachordner verschoben werden, wenn Prozesse angepasst sind.
- MOVE_LATER: Sinnvoller Kandidat fuer spaeteres Aufraeumen, aber nicht dringend.

## 1) Repository- und Tooling-Metadaten

| Datei | Aufgabe | Empfehlung |
|---|---|---|
| .agent.md | Agent/Automations-Hinweise fuer KI-Workflows. | KEEP_ROOT |
| .clang-format | C/C++ Formatierungsregeln fuer das gesamte Repo. | KEEP_ROOT |
| .clang-tidy | Statische Analyse-Regeln fuer C/C++. | KEEP_ROOT |
| .copilot-cross-compile-prompt.md | Prompt-Vorlage fuer Cross-Compile-Assistenz. | MOVE_OPTIONAL |
| .copilot-cross-compile-rules.json | Maschinenlesbare Regeln fuer Cross-Compile-Assistenz. | MOVE_OPTIONAL |
| .cppcheck | Konfiguration fuer cppcheck. | KEEP_ROOT |
| .cppcheck-suppressions | Suppression-Liste fuer cppcheck Findings. | KEEP_ROOT |
| .dockerignore | Ausschlussliste fuer Docker-Build-Context. | KEEP_ROOT |
| .docs-validation.example.yml | Beispielkonfiguration fuer Doku-Validierung. | MOVE_LATER |
| .editorconfig | Editor-Formatierungsstandards fuer alle Dateien. | KEEP_ROOT |
| .gitattributes | Git-Attribute (EOL, Linguist, Filter). | KEEP_ROOT |
| .gitignore | Globale Ignore-Regeln fuer das Repo. | KEEP_ROOT |
| .gitleaks.toml | Konfiguration fuer Secret-Scanning (gitleaks). | KEEP_ROOT |
| .gitmodules | Submodule-Definitionen. | KEEP_ROOT |
| .license-policy.json | Lizenz-Policy fuer Dependency/Compliance-Pruefungen. | KEEP_ROOT |
| .markdownlint.json | Lint-Regeln fuer Markdown. | KEEP_ROOT |
| .pre-commit-config.yaml | Pre-Commit Hook-Definitionen. | KEEP_ROOT |
| .secret-scan-allowlist.txt | Allowlist fuer Secret-Scanner-Ausnahmen. | KEEP_ROOT |
| .secrets.baseline | Baseline fuer Secret-Erkennung (z. B. detect-secrets). | KEEP_ROOT |
| .secrets.example | Beispiel-Datei fuer Secret-Konfiguration. | MOVE_LATER |

## 2) Governance, Strategie und zentrale Projektdokumente

| Datei | Aufgabe | Empfehlung |
|---|---|---|
| ARCHITECTURE.md | Gesamtarchitektur des Systems. | KEEP_ROOT |
| AUDIT.md | Audit-Rahmen, Auditstatus oder Auditvorgaben. | KEEP_ROOT |
| BRANCH_MIGRATION_INVENTORY.md | Inventar fuer Branch-Migrationen. | KEEP_ROOT |
| BRANCHING_STRATEGY.md | Verbindliche Branch- und Flow-Strategie. | KEEP_ROOT |
| CHANGELOG.md | Versionshistorie der Aenderungen. | KEEP_ROOT |
| CLAUDE.md | Arbeitsvertrag/Agentenregeln fuer Claude-basierten Workflow. | KEEP_ROOT |
| CODE_OF_CONDUCT.md | Verhaltenskodex des Projekts. | KEEP_ROOT |
| CONTRIBUTING.md | Richtlinien fuer Beitragsprozesse. | KEEP_ROOT |
| CTEST.md | Testausfuehrung und CTest-Konventionen. | KEEP_ROOT |
| DISTRIBUTED_TENSOR_SHARDING.md | Fachdokumentation zu Tensor-Sharding. | MOVE_LATER |
| EVALUATION_FRAMEWORK.md | Rahmen fuer Evaluierung/Qualitaetsbewertung. | MOVE_LATER |
| FEATURE_ENHANCEMENT.md | Sammeldokument fuer Feature-Erweiterungen. | MOVE_LATER |
| FUTURE_ENHANCEMENTS.md | Zukunftsplan mit implementierbaren Erweiterungen. | KEEP_ROOT |
| FUTURE_PLAN_SHORT.md | Kurzfassung der Zukunftsplanung. | MOVE_LATER |
| FUTURE_PLAN.md | Ausfuehrliche Zukunftsplanung. | MOVE_LATER |
| GAP_ANALYSIS.md | Gap-Analyse ueber fehlende/ausstehende Umsetzungen. | KEEP_ROOT |
| GOVERNANCE.md | Projekt-Governance und Entscheidungsregeln. | KEEP_ROOT |
| HARDWARE_REQUIREMENTS.md | Hardwareanforderungen und Betriebsrahmen. | KEEP_ROOT |
| IMPACT_REMEDIATION_ROADMAP.md | Roadmap fuer Impact-/Remediation-Arbeiten. | MOVE_LATER |
| INDEX.md | Dokumentindex/Navigationsseite. | KEEP_ROOT |
| ISSUE_SET.md | Zusammenstellung relevanter Issues. | MOVE_LATER |
| LICENSE | Lizenztext des Repositories. | KEEP_ROOT |
| MAINTAINERS.md | Verantwortliche Maintainer und Rollen. | KEEP_ROOT |
| MIGRATION_RUNBOOK.md | Operatives Runbook fuer Migrationen. | KEEP_ROOT |
| QUICKSTART.md | Schnellstart fuer neue Nutzer/Entwickler. | KEEP_ROOT |
| README.md | Haupteinstieg in das Projekt. | KEEP_ROOT |
| RELEASE_STRATEGY.md | Release-Prozess und Freigabestrategie. | KEEP_ROOT |
| RELEASE_TAG_INVENTORY.md | Inventar historischer/aktueller Release-Tags. | MOVE_LATER |
| RELEASE_TAG_MIGRATION.md | Migrationsvorgehen fuer Release-Tags. | MOVE_LATER |
| RELEASE_TYPE | Kennzeichnung des Release-Typs. | KEEP_ROOT |
| ROADMAP.md | Produkt- und Umsetzungsroadmap. | KEEP_ROOT |
| SECURITY.md | Security-Richtlinien und Meldeprozess. | KEEP_ROOT |
| SETUP.md | Setup- und Installationsbeschreibung. | KEEP_ROOT |
| SOP.md | Standard Operating Procedures. | KEEP_ROOT |
| SUPPORT.md | Support- und Kontaktpfade. | KEEP_ROOT |
| TARGET_ARCHITECTURE.md | Zielbild/Target-Architektur. | KEEP_ROOT |
| VERSION | Roh-Versionstext fuer Build/Release-Prozesse. | KEEP_ROOT |
| VERSIONING.md | Regeln zur Versionierung. | KEEP_ROOT |

## 3) Build-, Test- und Analysekonfiguration

| Datei | Aufgabe | Empfehlung |
|---|---|---|
| build.ps1 | Build-Hilfsskript fuer Windows/PowerShell. | KEEP_ROOT |
| CMAKE_HARDENING_PLAN.md | Plan fuer CMake-Hardening und Build-Qualitaet. | MOVE_LATER |
| CMakeLists.txt | Haupt-CMake-Einstieg des Projekts. | KEEP_ROOT |
| CMakePresets.json | Kanonische CMake-Presets fuer CI/Dev. | KEEP_ROOT |
| CMakeUserPresets.json | Lokale/ergaenzende User-Presets. | KEEP_ROOT |
| CMakeUserPresets.json.example | Beispiel fuer User-Presets. | KEEP_ROOT |
| Doxyfile | Standard-Doxygen-Konfiguration. | KEEP_ROOT |
| Doxyfile.audit | Doxygen-Variante fuer Audit/Qualitaetslauf. | KEEP_ROOT |
| Doxyfile.html.local | Lokale Doxygen-Variante fuer HTML-Ausgabe. | MOVE_OPTIONAL |
| Doxyfile.local | Lokale Doxygen-Konfiguration. | MOVE_OPTIONAL |
| Doxyfile.smoke | Doxygen-Konfiguration fuer Smoke-Durchlaeufe. | KEEP_ROOT |
| Doxyfile.xml.local | Lokale Doxygen-Variante fuer XML-Ausgabe. | MOVE_OPTIONAL |
| mkdocs-nopdf.yml | MkDocs-Konfiguration ohne PDF-Pipeline. | KEEP_ROOT |
| mkdocs.yml | Haupt-MkDocs-Konfiguration. | KEEP_ROOT |
| openapitools.json | OpenAPI Tooling-/Generator-Konfiguration. | KEEP_ROOT |
| Phase0-2-TestValidation.ps1 | Validierungsskript fuer definierte Phase-Tests. | MOVE_OPTIONAL |
| pom.xml | Maven-Konfiguration fuer Java-basierte Teiltools. | KEEP_ROOT |
| requirements-docs.txt | Python-Abhaengigkeiten fuer Doku-Tooling. | KEEP_ROOT |
| requirements.txt | Allgemeine Python-Abhaengigkeiten. | KEEP_ROOT |
| setup-build-env.ps1 | Setup des Build-Umfelds unter PowerShell. | KEEP_ROOT |
| sonar-project.properties | SonarQube/SonarCloud Analysekonfiguration. | KEEP_ROOT |
| themis.code-workspace | VS Code Workspace-Definition. | MOVE_OPTIONAL |
| ThemisDB.sln | Visual-Studio-Solution-Entry fuer Windows. | KEEP_ROOT |
| vcpkg-configuration.json | vcpkg Registry/Baseline-Konfiguration. | KEEP_ROOT |
| vcpkg.docker.json | Docker-spezifische vcpkg-Konfiguration. | MOVE_OPTIONAL |
| vcpkg.json | vcpkg Manifest (Dependencies/Features). | KEEP_ROOT |

## 4) Container- und Packaging-Dateien

| Datei | Aufgabe | Empfehlung |
|---|---|---|
| docker-bake.hcl | Buildx Bake-Definition fuer Multi-Target Docker Builds. | KEEP_ROOT |
| docker-compose.qnap.yml | QNAP-spezifische Compose-Variante. | MOVE_OPTIONAL |
| docker-compose.user-storage.yml | Compose-Overlay fuer User-Storage-Szenarien. | MOVE_OPTIONAL |
| docker-compose.yml | Haupt-Compose-Definition. | KEEP_ROOT |
| Dockerfile | Standard-Image-Build fuer das Projekt. | KEEP_ROOT |
| Dockerfile.community-simple | Vereinfachtes Community-Image. | KEEP_ROOT |
| Dockerfile.prebuilt-helper | Helper-Image fuer Prebuilt-/Build-Hilfsprozesse. | MOVE_OPTIONAL |
| Dockerfile.prebuilt-local | Lokale Prebuilt-Variante. | MOVE_OPTIONAL |

## 5) Root-nahe C++ Test-/Mini-Dateien

| Datei | Aufgabe | Empfehlung |
|---|---|---|
| test_include.cpp | Kleine Include-/Compile-Probe im Root. | MOVE_LATER |
| test_namespace_graph.cpp | Namespace/Graph-Testprobe im Root. | MOVE_LATER |
| test_unity_mini.cpp | Unity-Build-Testprobe im Root. | MOVE_LATER |

## 6) Konkrete Aufraeum-Entscheidung fuer naechste Runde

Direkt verschiebbar (niedriges Risiko, sofern Referenzen geprueft sind):
- .copilot-cross-compile-prompt.md
- .copilot-cross-compile-rules.json
- .docs-validation.example.yml
- .secrets.example
- Doxyfile.html.local
- Doxyfile.local
- Doxyfile.xml.local
- themis.code-workspace
- docker-compose.qnap.yml
- docker-compose.user-storage.yml
- Dockerfile.prebuilt-helper
- Dockerfile.prebuilt-local
- test_include.cpp
- test_namespace_graph.cpp
- test_unity_mini.cpp

Empfohlenes Zielschema:
- tooling/linting/ fuer .copilot- und Validierungs-Assets
- docs/local/ fuer lokale Doxygen-Varianten
- deploy/docker/variants/ fuer optionale Compose-/Dockerfile-Varianten
- tests/scratch/ oder tests/manual/ fuer Root-Testproben

Pruefschritt vor jeder Verschiebung:
- Mit git grep auf Dateinamen in CI/Docs/Skripten suchen.
- Bei Treffer zuerst Referenzen auf neuen Pfad umstellen.
- Danach erst Datei verschieben.
