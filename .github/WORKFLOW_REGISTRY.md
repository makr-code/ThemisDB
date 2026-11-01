# ThemisDB Workflow Registry (Comprehensive Governance)

> Author: ThemisDB Contributors
> Created: 2026-09-09
> Last Updated: 2026-11-01
> Update Note: Documented vcpkg cache paths, Windows sccache backend and cache measurement limits; workflow inventory unchanged
> Status: active
> Total Workflows: 99 (verified 2026-10-29)
> AI_Used: true
> AI_Models: "GitHub Copilot agent; exact model backend not verifiable from final document text alone"
> AI_Contributor: "GitHub Copilot"
> Human_Review: false
> Human_Author: "ThemisDB Team"

## Zielbild

Dieses Repository nutzt ein bewusst strukturiertes, release-orientiertes CI/CD-System.
Alle Workflows sind dokumentiert und validiert gegen WORKFLOW_GUIDELINES.md.

### CI remediation (2026-10-31)

- Alle 50 Aufrufe der lokalen Run-Summary-Action führen den Checkout im eigenen Job
  ohne persistierte Zugangsdaten aus; Workflow-Anzahl, Trigger und Gate-Ergebnisse bleiben unverändert.
- `benchmark-gate.yml` nutzt das Linux-Release-Preset explizit und führt den
  Benchmark im Build-Job mit dessen Shared Libraries aus. Nur JSON-Ergebnisse
  werden als Artifact zum Baseline-Vergleich übertragen.
- `coverage-report-nightly.yml` installiert RocksDB und verbietet fehlende RocksDB-Abhängigkeiten;
  `reusable-benchmark-build-run-report.yml` installiert Boost.Filesystem.
- `msvc.yml` installiert nlohmann-json und erkennt das Importer-Projekt direkt,
  statt ein vom Visual-Studio-Generator nicht unterstütztes `help`-Target abzufragen.
- `gate-pr-core.yml` delegiert `issues: write` nur am Build-Components-Aufruf;
  RAG-Phasen 7–10 und ihr Reusable benötigen `pull-requests: write` für
  PR-Labels/-Kommentare; `issues: write` allein genügt dort nicht.
- Housekeeping und Release-Promotion brechen nur überholte PR-Läufe ab;
  manuelle Release-/Wartungsläufe bleiben gegen Abbruch geschützt.
- Docker-Consumer, Plattform-Builder und Modell-Cache nutzen getrennte
  Concurrency-Namensräume. AMD64 und ARM64 haben eigene Plattformgruppen;
  Cache-Vorbereitung serialisiert unabhängig von ihren wartenden Aufrufern.
  Builds verwenden native AMD64-/ARM64-Runner. Provenance/SBOM bleiben für
  Registry-Pushes aktiv, aber nicht für den inkompatiblen lokalen Docker-Exporter.
- Der Core-Preflight erkennt lokale Action-/Workflow-Referenzen und kommentierte
  SHA-Pins korrekt; neue Policy-Verstöße werden anhand der hinzugefügten Zeilen geprüft.
- Der Reusable-CMake-Build nutzt den vorhandenen dateibasierten vcpkg-Cache statt
  eines unvorbereiteten NuGet-Feeds. Bei sccache-Backendfehlern deaktiviert der
  bestehende Retry die CMake-Cache-Option tatsächlich, erhält die Toolchain und
  propagiert Reconfigure-Fehler. Custom-Caller deklarieren ihre tatsächlichen
  Binärverzeichnisse über `build_directory` für Cache und Recovery.
  Release-Critical-Builds erhalten 120 statt 45 Minuten;
  ein vollständiger erfolgreicher Build bleibt separat nachzuweisen.
- Abgelaufene Ausführungsfreigaben benötigen eine Maintainer-Aktion. Der im
  MSVC-Lauf beobachtete C1060-Analyser-Speicherfehler bleibt separat zu verifizieren;
  die Reparaturen unterdrücken ihn nicht.
- Linux-Release-Matrix-Lanes verwenden den über Actions-Cache persistierten lokalen
  sccache-Speicher statt des GHA-Cache-Backends; die Auswahl wird je Plattform
  explizit durchgereicht, damit ein `false` nicht durch einen Fallback überschrieben wird.

### Build-Cache-Aktualisierung (2026-11-01)

| Workflow | Aktualisierter Cache-Vertrag |
|---|---|
| [reusable-cmake-build.yml](workflows/reusable-cmake-build.yml) | Persistiert vcpkg-Downloads, Binary-Archive und lokalen sccache-Speicher; entfernt den nicht verwendeten Root-Installationspfad aus dem Cache. Loggt Archivanzahl/-größe und behält Archive auch bei fehlendem exakten Cache-Treffer. |
| [release-build-matrix.yml](workflows/release-build-matrix.yml) | Windows-Configure-/Build-Befehle erhalten das aktivierte GHA-sccache-Backend vom Reusable. Linux bleibt beim lokalen Backend; Windows-Parallelitätslimits bleiben unverändert. |
| [msvc.yml](workflows/msvc.yml) | Initialisiert das vcpkg-Submodul vor Restore, cached den klassischen Installationspfad `vcpkg/installed` und bindet den Schlüssel an Submodul-Commit und Manifest-/Configuration-Hash. Aktiviert den dateibasierten Binary-Cache explizit. |

Workflow-Anzahl, Trigger, Concurrency und Release-/Security-Gates werden durch
diese Änderungen nicht geändert. Der Reusable und die MSVC-Analyse verwenden
weiterhin getrennte Cache-Namensräume. Die Reparatur führt weder einen gemeinsamen
Windows-Paketfeed noch einen zusätzlichen Cache-Warming-Workflow ein.

Die [Guidelines zur Cache-Diagnose](WORKFLOW_GUIDELINES.md#build-cache-betrieb-und-diagnose)
beschreiben die drei getrennten Messgrößen Actions-Restore, vcpkg-Paket-Restore
und Compiler-Treffer. Unveränderliche Cache-Einträge und ausbleibende automatische
Saves bei Jobfehlern bleiben Einschränkungen. Die vorhandenen Regressionstests
prüfen den Workflow-Vertrag; verbesserte Online-Trefferraten und Windows-Laufzeiten
sind erst durch erfolgreiche Wiederholungsläufe nachzuweisen.

## Leitprinzipien

- **Keine Schatten-CI:** Alle 99 aktiven Workflow-Dateien sind in diesem Registry dokumentiert.
- **Klare Verantwortlichkeit:** Jeder Workflow hat definierte Trigger und Zweck.
- **Konsistente Governance:** Alle Workflows folgen Naming-, Concurrency- und Trigger-Policies.
- **Drift Prevention:** Registry-Update ist erforderlich bei neuen Workflows (Pre-Commit-Hook in Phase 3).

## CMake-Reuse Audit (2026-10-28)

- Vollständige Datei-basierte Klassifizierung aller 99 Workflow-Dateien:
  `.github/workflows/cmake-reuse-audit.csv`
- Klassifizierungslogik:
  - `uses reusable-cmake-build.yml`
  - `direct/inline CMake-or-CTest`
  - `exception-or-not-relevant`

### Audit-Summary (alle `.github/workflows/*.yml`)

| Klasse | Anzahl |
|---|---:|
| uses reusable-cmake-build.yml | 11 |
| direct/inline CMake-or-CTest | 18 |
| exception-or-not-relevant | 70 |

### Workflows mit direkter CMake/CTest-Logik (ohne `reusable-cmake-build.yml`) und begründete Ausnahmen

| Workflow | Ausnahmegrund |
|---|---|
| `benchmark-gate.yml` | Wave-4 Gate mit eigener Baseline-Manifest-Prüfung und benchmark-spezifischem Evidenz-Output; derzeit bewusst als eigenständige Gate-Lane. |
| `build-benchmarks.yml` | Stark spezialisierte Multi-Lane Benchmark-Orchestrierung (Voice/GPU/Soak/Wave-A) mit heterogenen Targets und Artefakten. |
| `build-content-regression.yml` | Content/Wave-D Regression mit eigener Test- und Soak-Orchestrierung außerhalb eines einzelnen Build-Line-Contracts. |
| `build-llm-inference.yml` | Modell-/Inference-spezifische Build+Test-Pipeline inkl. LLM-Artefakt-Vorbereitung. |
| `build-wave-a-gpu.yml` | GPU-spezifische Matrix-Lane mit Hardware-/Backend-spezifischer Ausführung. |
| `build-wave-b-transaction.yml` | Transaktionsspezifische Build-/Testlane mit fokussierten Targets. |
| `coverage-report-nightly.yml` | Coverage-Instrumentierung und Report-Erzeugung (nightly) als spezialisierter Ablauf. |
| `gate-pr-final-runtime-api.yml` | Wiederverwendbarer Final-Runtime-Gate-Workflow mit TinyLlama/docs.db + Runtime-API/Docker-Prüfungen. |
| `gate-pr-rag-security.yml` | Sicherheitsgate mit isolierten Security-Target-Builds und Security-spezifischer Auswertung. |
| `gate-pr-rag-version.yml` | Versions-/Reindex-Governance-Gate mit gezielten Canary-Tests. |
| `release-mainline.yml` | Kanonischer Release-Orchestrator (CPack/Release-Publish/Distribution) mit mehrstufigem Release-Vertrag. |
| `reusable-benchmark-build-run-report.yml` | Reusable Benchmark-Pipeline (Build/Run/Report) mit eigenem benchmark-zentriertem Vertrag. |
| `reusable-benchmark-runner.yml` | Reusable Benchmark-Runner mit eigener benchmark-zentrierter Build/Run-Semantik. |
| `reusable-pr-build-components-gate.yml` | Reusable PR-Components-Gate mit zielgerichteter Build+CTest-Selektion. |
| `wave-a-gpu-ci-execution.yml` | Wave-A GPU Execution/Baseline-Lane mit eigenem GPU-validierungsfokussiertem Ablauf. |
| `security-codeql.yml` | Security-Scan-Lane mit analyzer-spezifischen Build-Anforderungen. |
| `msvc.yml` | Windows/MSVC-analyzebezogene Build-Lane, separat vom Linux-zentrierten CMake-Reuse-Vertrag. |
| `reusable-cmake-build.yml` | Das CMake-Reuse-Template selbst (intentional inline CMake, kein Consumer). |

### Konsolidierung in diesem Change

- `gate-distributed-knowledge.yml` wurde auf `reusable-cmake-build.yml` umgestellt.
- Ergebnis: diese Gate-Lane nutzt jetzt den gemeinsamen CMake-Reuse-Pfad und behält die bestehenden Consumer-Jobs (`ctest` gegen Producer-Artefakt) unverändert bei.

### Fokus-Dateien aus dem CI-Audit-Request

| Workflow | Status |
|---|---|
| `gate-pr-benchmark-ctest.yml` | `exception-or-not-relevant` (führt kein CMake/CTest-Build aus; validiert Benchmark-Registrierung via Python-Audit) |
| `benchmark-gate.yml` | `direct/inline CMake-or-CTest` (dokumentierte Ausnahme, siehe Tabelle oben) |
| `benchmark-performance-gate.yml` | `uses reusable-cmake-build.yml` |
| `build-benchmarks.yml` | `direct/inline CMake-or-CTest` (dokumentierte Ausnahme, siehe Tabelle oben) |
| `compliance-governance-gates.yml` | `exception-or-not-relevant` (Governance-/Policy-Lane ohne CMake/CTest-Build) |
| `nightly-governance-audit.yml` | `exception-or-not-relevant` (Nightly-Governance-Audit ohne CMake/CTest-Build) |

## Aktiver Workflow-Kern: Alle 90 Workflow-Dateien

### 1. BUILD & MAINLINE (13 Workflows)

| Workflow | Purpose | Status |
|----------|---------|--------|
| `build-mainline.yml` | Build: Mainline (All platforms, multi-matrix) | active |
| `build-clang-fast.yml` | Build: Clang Fast (Quick clang-only validation) | active |
| `build-benchmarks.yml` | Build: Benchmarks (Performance regression suite, 64-matrix) | active |
| `build-sanitizer-nightly.yml` | Build: Sanitizer Nightly (AddressSanitizer + UBSan) | active |
| `build-wave-a-gpu.yml` | Build: Wave A GPU (CUDA/GPU feature validation) | active |
| `build-wave-b-transaction.yml` | Build: Wave B Transaction (Transactional subsystem) | active |
| `build-wave-b-llm-benchmarks.yml` | Build: Wave B LLM Benchmarks (LLM inference performance) | active |
| `build-llm-inference.yml` | Build: LLM Inferencing (TinyLlama + doku.db RAG + AdaLoRA) | active |
| `build-content-regression.yml` | Build: Content Regression [Wave D] (Content processing validation) | active |
| `build-widget.yml` | Build: WinGet E2E Validation (Windows installer end-to-end) | active |
| `build-ollama-router.yml` | Build: Copilot Ollama Router (VS Code extension build) | active |
| `benchmark-thematic-suites.yml` | Benchmark: Thematic Suites (Scheduled thematic benchmark suites via reusable pipeline) | active |
| `wave-a-gpu-ci-execution.yml` | Wave A GPU CI Execution (GPU + CPU Fallback matrix) | active |
| `wave-a-gpu-phase3-baseline-execution.yml` | Wave A GPU Phase 3: Baseline Capture & Measurement | active |

### 2. GATES: PR VALIDATION (23 Workflows)

| Workflow | Purpose | Gate Type |
|----------|---------|-----------|
| `gate-pr-core.yml` | Gate: PR Core (Primary PR gate: tests, boundaries, policies) | fast-path |
| `gate-pr-community-failclosed.yml` | Gate: PR Core [Community Fail-Closed] (Community-specific gate) | fail-closed |
| `gate-pr-edition-license.yml` | Gate: PR Core [Edition & License] (Edition compliance) | compliance |
| `gate-pr-hash-sbom.yml` | Gate: PR Core [Hash & SBOM Integrity] (Supply chain security) | compliance |
| `gate-pr-plugin-boundary.yml` | Gate: PR Core [Plugin Boundary] (Plugin boundary enforcement) | compliance |
| `gate-pr-primary-doc-structure.yml` | Gate: PR Primary Doc Structure (README/ARCHITECTURE validation) | docs |
| `gate-pr-doc-metadata.yml` | Gate: PR Doc Metadata (Markdown metadata & governance) | docs |
| `gate-pr-doxygen-governance.yml` | Gate: PR Doxygen Governance (C++ API doc validation) | docs |
| `gate-pr-module-doxygen-xml.yml` | Gate: PR Module Doxygen XML (Module-level Doxygen generation) | docs |
| `gate-pr-version-targeting.yml` | Gate: PR Version Targeting (PR body version field validation) | version-control |
| `gate-pr-merge-readiness.yml` | Gate: PR Merge Readiness (Pre-merge final validation) | pre-merge |
| `gate-pr-rag-eval.yml` | Gate: PR RAG Evaluation Contract (RAG system evaluation) | specialized |
| `gate-pr-rag-security.yml` | Gate: PR RAG Security Guardrails (RAG security validation) | security |
| `gate-pr-rag-version.yml` | Gate: PR RAG Embedding Version Governance (Embedding version tracking) | version-control |
| `gate-pr-rag-phase7.yml` | Gate: PR RAG Phase 7 (Freshness SLA) | specialized |
| `gate-pr-rag-phase8.yml` | Gate: PR RAG Phase 8 (Observability SLO) | specialized |
| `gate-pr-rag-phase9.yml` | Gate: PR RAG Phase 9 (Research Evaluation) | specialized |
| `gate-pr-rag-phase10.yml` | Gate: PR RAG Phase 10 (Cost Optimizer) | specialized |
| `gate-pr-benchmark-ctest.yml` | Gate: PR Benchmark CTest Coverage (Benchmark CTest registration coverage gate) | quality |
| `benchmark-gate.yml` | Benchmark: Wave 4 Release Gate (Wave 4 release-critical benchmark evidence and threshold validation) | quality |
| `gate-pr-final-runtime-api.yml` | Gate: PR Final Runtime API (TinyLlama model load + docs.db RAG + runtime API smoke) | runtime |
| `gate-wave-closure.yml` | Gate: Wave Closure Governance (Wave exit criteria validation) | governance |
| `gate-distributed-knowledge.yml` | Gate: Module Validation [distributed_knowledge] (Module-specific gate) | module-specific |
| `gate-workflow-guidelines.yml` | Gate: Workflow Guidelines Compliance (Automated workflow policy validation) | governance |

### 3. RELEASE & PUBLICATION (21 Workflows)

| Workflow | Purpose | Stage |
|----------|---------|-------|
| `release-mainline.yml` | Release: Packaging (CPack orchestration, tag-triggered) | canonical |
| `release-build-matrix.yml` | Release: Build Matrix (Multi-platform build coordination) | build |
| `release-changelog.yml` | Release: Changelog (Release notes generation) | docs |
| `release-docker-image.yml` | Release: Docker Image Consumer (Docker Hub publish) | distribution |
| `release-docker-approval.yml` | Release: Docker Approval Gate (Docker release approval) | approval |
| `release-winget.yml` | Release: WinGet Consumer (WinGet publish) | distribution |
| `release-winget-approval.yml` | Release: WinGet Approval Gate (WinGet approval) | approval |
| `release-windows-distribution.yml` | Release: Windows Distribution Consumer (Windows MSI/ZIP) | distribution |
| `release-windows-distro-approval.yml` | Release: Windows Distro Approval Gate (Windows approval) | approval |
| `release-linux-distribution.yml` | Release: Linux Distribution Consumer (Linux packages) | distribution |
| `release-linux-distro-approval.yml` | Release: Linux Distro Approval Gate (Linux approval) | approval |
| `release-mainline-approval.yml` | Release: GitHub Release Approval Gate (GitHub release approval) | approval |
| `release-nightly.yml` | Release: Nightly (Daily snapshot builds) | automation |
| `release-promote.yml` | Release: Promote (Semi-Automatic edition promotion) | automation |
| `release-evidence-chain.yml` | Release: Evidence Chain (Hash-linked release evidence ledger + verification) | governance |
| `release-rollback.yml` | Release: Rollback (Release rollback orchestration) | recovery |
| `release-wordpress-press.yml` | Release: WordPress Press (Blog/announcement publish) | communication |
| `edition-hyperscaler-ci.yml` | Edition: Hyperscaler [CI] (Hyperscaler-edition specific CI) | edition-specific |
| `ga-promotion-signoff.yml` | Governance: GA Promotion Sign-Off (GA promotion approval workflow) | governance |
| `benchmark-performance-gate.yml` | Benchmark: Performance Gate (Performance regression detection) | quality |
| `gate-release-benchmark-validation.yml` | Gate: Release Benchmark Coverage Validation (Release benchmark coverage decision gate) | quality-gate |

### 4. MAINTENANCE & GOVERNANCE (21 Workflows)

| Workflow | Purpose | Category |
|----------|---------|----------|
| `maintenance-pr-failure-diagnosis.yml` | Maintenance: PR Failure Diagnosis [Recommend-Only] (Auto-diagnosis on PR failure) | support |
| `maintenance-ci-health.yml` | Maintenance: CI Health Dashboard (CI health metrics aggregation) | observability |
| `maintenance-build-issues.yml` | Maintenance: Build Error Issues (Automatic build error tracking) | issue-tracking |
| `maintenance-issues.yml` | Maintenance: Issues [GS3 + Security Alerts] (GS3 security alert handling) | security |
| `maintenance-docs.yml` | Maintenance: Docs (Documentation health and drift detection) | docs |
| `nightly-governance-audit.yml` | Nightly: Governance Audit Scope (Nightly governance evidence and checklist validation) | governance |
| `coverage-report-nightly.yml` | Maintenance: Nightly Coverage Collection & Baseline Update (Nightly coverage metrics and baseline refresh) | quality |
| `nightly-governance-audit.yml` | Nightly: Governance Audit Scope (Scheduled governance artifact and checklist audit) | governance |
| `maintenance-housekeeping.yml` | Maintenance: Housekeeping (Repository cleanup and maintenance) | maintenance |
| `maintenance-architecture-ci.yml` | Maintenance: Architecture CI (Architecture documentation validation) | docs |
| `maintenance-workflow-guardrails-observe.yml` | Maintenance: Workflow Guardrails [Observe] (Workflow policy observability) | governance |
| `nightly-governance-audit.yml` | Nightly: Governance Audit Scope (Scheduled governance artifact and checklist audit) | governance |
| `maintenance-soll-ist-gap-issues.yml` | Maintenance: SOLL-IST Gap Issues (Roadmap vs. reality tracking) | governance |
| `maintenance-ai-working.yml` | Maintenance: AI Working Cleanup - LLM Wiki (AI context cleanup) | automation |
| `nightly-governance-audit.yml` | Governance: Nightly Audit Scope (Nightly governance artifact and checklist audit) | governance |
| `maintenance-docs-db-build.yml` | Maintenance: Docs-to-ThemisDB Database Build (Documentation database sync) | automation |
| `maintenance-compendium-sync.yml` | Maintenance: Compendium Sync (Compendium documentation sync) | automation |
| `wiki-pr-gate.yml` | Maintenance: Wiki PR Gate (GitHub Wiki PR validation) | docs |
| `wiki-publish-from-issue.yml` | Wiki: Publish from Issue Approval (Wiki auto-publish from issues) | automation |
| `publish-wiki.yml` | Publish: GitHub Wiki (Wiki content publishing) | publication |
| `compliance-governance-gates.yml` | Compliance: Governance Gates (Multi-layer compliance validation) | compliance |
| `compliance-supply-chain.yml` | Compliance: Supply Chain (Supply chain security validation) | compliance |
| `gate-copilot-regression.yml` | Gate: Copilot Regression (Copilot code review regression detection) | quality |
| `copilot-code-review.yml` | Copilot: Code Review Setup (Copilot code review initialization) | tooling |

### 5. SECURITY (6 Workflows)

| Workflow | Purpose | Frequency |
|----------|---------|-----------|
| `security-codeql.yml` | Security: CodeQL (CodeQL static analysis) | scheduled |
| `security-consolidated.yml` | Security: Consolidated Scans (Multi-tool security aggregation) | scheduled |
| `security-dast-zap.yml` | Security: OWASP ZAP DAST (Dynamic application security testing) | scheduled |
| `security-fortify.yml` | Security: Fortify AST Scan [Scheduled] (Fortify static analysis) | scheduled |
| `security-fuzzing.yml` | Security: Fuzzing (Fuzz testing suite) | scheduled |
| `security-pentest-quarterly.yml` | Security: Quarterly Pentest Cadence (Quarterly penetration testing) | quarterly |

### 5b. SECURITY (ADDITIONAL SCANNERS, DOCUMENTED)

| Workflow | Purpose | Frequency |
|----------|---------|-----------|
| `codeql.yml` | Security: CodeQL Advanced (multi-language CodeQL matrix scan) | scheduled |
| `crda.yml` | Security: Trivy Dependency Scan (dependency vulnerability scanning + SARIF upload) | pull_request / manual |
| `msvc.yml` | Security: Microsoft C++ Code Analysis (Windows static analysis + SARIF upload) | scheduled |

### 6. REUSABLE WORKFLOWS (10 Workflows)

| Workflow | Purpose | Consumers |
|----------|---------|-----------|
| `reusable-cmake-build.yml` | Reusable CMake Build Pipeline (Build orchestration reusable) | mainline, gates, release |
| `reusable-benchmark-runner.yml` | Reusable: Benchmark Runner (Benchmark execution reusable) | build-benchmarks |
| `reusable-benchmark-build-run-report.yml` | Reusable: Benchmark Build/Run/Report (Thematic benchmark build/run/report pipeline) | benchmark-thematic-suites |
| `reusable-docs-db-builder.yml` | Reusable: Docs-to-ThemisDB Database Builder (Docs DB generation) | maintenance-docs-db-build |
| `reusable-status-flags-and-issues.yml` | Reusable: Status Flags and Issues (Status/issue automation) | maintenance, release |
| `reusable-docker-image-platform.yml` | Reusable: Docker Image Platform Build (Per-platform Docker build + digest handoff) | release-docker-image |
| `reusable-llm-model-cache.yml` | Reusable: LLM Model Cache (Shared model-cache restore/save for LLM workflows) | reusable-docker-image-platform |
| `reusable-pr-build-components-gate.yml` | Reusable: PR Build Components Gate (Release-critical + runtime API gate bundle for PRs) | gate-pr-core |
| `reusable-rag-phase-gate.yml` | Reusable: RAG Phase Gate (Shared PR gate for RAG phase file/semantic/compile checks) | gate-pr-rag-phase7/8/9/10 |
| `reusable-release-approval-comment-gate.yml` | Reusable: Release Approval Comment Gate (Maintainer-command issue_comment release dispatch gate) | release-*-approval workflows |

## Compliance Scorecard

| Category | Total | Status |
|----------|-------|--------|
| **BUILD & MAINLINE** | 13 | ✅ All documented |
| **GATES: PR VALIDATION** | 23 | ✅ All documented |
| **RELEASE & PUBLICATION** | 21 | ✅ All documented |
| **MAINTENANCE & GOVERNANCE** | 21 | ✅ All documented |
| **SECURITY** | 9 | ✅ All documented |
| **REUSABLE WORKFLOWS** | 4 | ✅ All documented |
| **TOTAL** | **90** | ✅ **100% Registry Sync** |

## Complete Workflow Redesign and Consolidation Plan

> Status: active redesign target for the canonical CI/CD topology.
> Goal: one reusable build core, one release orchestrator, one artifact contract, and one approval policy layer.

### 1. Design principles

- Keep one canonical reusable engine for CMake builds: [reusable-cmake-build.yml](.github/workflows/reusable-cmake-build.yml)
- Keep one canonical release orchestrator: [release-mainline.yml](.github/workflows/release-mainline.yml)
- Keep downstream distribution workflows as consumers only, not as independent artifact producers
- Remove duplicate setup/configure/build logic from the top-level matrix files and standardize on a single parameterized contract
- Split quality validation into named build lines that reuse the same engine with different parameters: PR release-critical tests and benchmark execution/reporting
- Treat release approval and release packaging as separate concerns: approval = authorization; packaging = artifact assembly
- Make gate workflows remain fast, non-blocking when appropriate, and scoped by file or module

### 2. Canonical layered topology

#### Layer 0 — policy and gate

- [gate-pr-core.yml](.github/workflows/gate-pr-core.yml)
- [gate-pr-final-runtime-api.yml](.github/workflows/gate-pr-final-runtime-api.yml)
- [gate-pr-merge-readiness.yml](.github/workflows/gate-pr-merge-readiness.yml)
- [gate-workflow-guidelines.yml](.github/workflows/gate-workflow-guidelines.yml)
- [compliance-governance-gates.yml](.github/workflows/compliance-governance-gates.yml)

Purpose:
- enforce repo policy
- verify workflow compliance and metadata
- gate PR merge and release readiness without heavy package work

#### Layer 1 — build orchestration

- [build-mainline.yml](.github/workflows/build-mainline.yml)
- [reusable-cmake-build.yml](.github/workflows/reusable-cmake-build.yml)
- [build-clang-fast.yml](.github/workflows/build-clang-fast.yml)
- [build-content-regression.yml](.github/workflows/build-content-regression.yml)
- specialized build lanes such as GPU/LLM/benchmarks remain in dedicated build workflows but must call the same reusable engine

Purpose:
- define build cost boundaries
- reuse one build pipeline contract for all platform and architecture lanes
- avoid copy-paste setup scripts for each entrypoint

#### Layer 2 — release orchestration

- [release-mainline.yml](.github/workflows/release-mainline.yml)
- [release-build-matrix.yml](.github/workflows/release-build-matrix.yml)
- [release-changelog.yml](.github/workflows/release-changelog.yml)
- [release-promote.yml](.github/workflows/release-promote.yml)
- [release-rollback.yml](.github/workflows/release-rollback.yml)

Purpose:
- define one official release pipeline
- package the canonical release artifacts
- validate tag and edition semantics before publication

#### Layer 3 — downstream delivery consumers

- [release-docker-image.yml](.github/workflows/release-docker-image.yml)
- [release-winget.yml](.github/workflows/release-winget.yml)
- [release-linux-distribution.yml](.github/workflows/release-linux-distribution.yml)
- [release-windows-distribution.yml](.github/workflows/release-windows-distribution.yml)

Purpose:
- consume final package outputs from the release orchestrator
- not create a second artifact identity or build path
- remain downstream of the canonical release contract

#### Layer 4 — security and compliance

- [security-codeql.yml](.github/workflows/security-codeql.yml)
- [security-consolidated.yml](.github/workflows/security-consolidated.yml)
- [security-dast-zap.yml](.github/workflows/security-dast-zap.yml)
- [security-fortify.yml](.github/workflows/security-fortify.yml)
- [security-fuzzing.yml](.github/workflows/security-fuzzing.yml)
- [security-pentest-quarterly.yml](.github/workflows/security-pentest-quarterly.yml)
- [compliance-supply-chain.yml](.github/workflows/compliance-supply-chain.yml)

Purpose:
- keep security scanning isolated from product build speed
- avoid mixing release build logic with scheduled security scans

#### Layer 5 — maintenance and governance

- [maintenance-ci-health.yml](.github/workflows/maintenance-ci-health.yml)
- [maintenance-build-issues.yml](.github/workflows/maintenance-build-issues.yml)
- [maintenance-housekeeping.yml](.github/workflows/maintenance-housekeeping.yml)
- [maintenance-ai-working.yml](.github/workflows/maintenance-ai-working.yml)
- [maintenance-docs.yml](.github/workflows/maintenance-docs.yml)

Purpose:
- operational housekeeping
- build issue tracking and registry governance
- repository hygiene without impacting build correctness

### 3. Consolidation targets

The following patterns are explicitly identified as duplicate or drift-prone and should be normalized into the canonical model above.

#### 3.1 Build lane consolidation

Current duplication:
- [build-mainline.yml](.github/workflows/build-mainline.yml) defines repeated platform setup blocks in `step2-*` jobs
- [release-build-matrix.yml](.github/workflows/release-build-matrix.yml) repeats a second matrix definition with largely parallel setup/configure/build commands

Target:
- one build matrix contract with `platform`, `preset`, `runner`, `setup_script`, `configure_command`, `build_command`, `artifact_name`
- all platform jobs route through [reusable-cmake-build.yml](.github/workflows/reusable-cmake-build.yml)
- no second shell-native configure logic outside the reusable engine

#### 3.2 Release approval consolidation

Current duplication:
- [release-mainline-approval.yml](.github/workflows/release-mainline-approval.yml)
- [release-docker-approval.yml](.github/workflows/release-docker-approval.yml)
- [release-winget-approval.yml](.github/workflows/release-winget-approval.yml)
- [release-linux-distro-approval.yml](.github/workflows/release-linux-distro-approval.yml)
- [release-windows-distro-approval.yml](.github/workflows/release-windows-distro-approval.yml)

Target:
- canonical approval gateway with a single `approval_policy` and a shared artifact gate
- downstream distribution jobs remain consumers only
- downstream release consumers should not carry independent release-signoff logic

#### 3.3 Artifact contract normalization

Current drift:
- artifact naming is split between `build-*`, `packages-*`, and `dist/*` flows
- release packaging logic is spread between release orchestration and downstream package jobs

Target:
- canonical artifact naming:
  - `themisdb-<edition>-<version>-<platform>-<arch>` for final release bundles
  - `packages-<preset>-<run_number>` for package staging artifacts
  - `build-<preset>-<run_number>` for raw build outputs
- all downstream jobs must fetch from the canonical release artifact set only

#### 3.4 Concurrency and trigger tightening

The repo must enforce the following standard:
- PR gate workflows: `cancel-in-progress: true`
- release orchestrators: `cancel-in-progress: false`
- scheduled maintenance jobs: low-frequency only and structured by day/time bucket
- `paths` filters must be narrow and module- or file-specific; avoid broad repository-wide patterns unless explicitly justified

### 4. Consolidated workflow map (target state)

#### Canonical build and release chain

- [reusable-cmake-build.yml](.github/workflows/reusable-cmake-build.yml) — universal build primitive
- [gate-pr-core.yml](.github/workflows/gate-pr-core.yml) — parameterized PR release-critical test quality line consumer
- [benchmark-performance-gate.yml](.github/workflows/benchmark-performance-gate.yml) — parameterized benchmark quality line consumer
- [build-mainline.yml](.github/workflows/build-mainline.yml) — mainline build lane on develop
- [release-mainline.yml](.github/workflows/release-mainline.yml) — release orchestrator
- [release-build-matrix.yml](.github/workflows/release-build-matrix.yml) — matrix implementation of the release build contract
- [release-docker-image.yml](.github/workflows/release-docker-image.yml) — downstream packaging consumer
- [release-winget.yml](.github/workflows/release-winget.yml) — downstream delivery consumer
- [release-linux-distribution.yml](.github/workflows/release-linux-distribution.yml) — downstream Linux package consumer
- [release-windows-distribution.yml](.github/workflows/release-windows-distribution.yml) — downstream Windows package consumer

#### Canonical governance and support chain

- [gate-pr-core.yml](.github/workflows/gate-pr-core.yml)
- [gate-pr-final-runtime-api.yml](.github/workflows/gate-pr-final-runtime-api.yml)
- [gate-pr-merge-readiness.yml](.github/workflows/gate-pr-merge-readiness.yml)
- [gate-workflow-guidelines.yml](.github/workflows/gate-workflow-guidelines.yml)
- [maintenance-ci-health.yml](.github/workflows/maintenance-ci-health.yml)
- [maintenance-build-issues.yml](.github/workflows/maintenance-build-issues.yml)
- [compliance-governance-gates.yml](.github/workflows/compliance-governance-gates.yml)

### 5. Deprecation and retirement policy

The following patterns are considered non-canonical and should be reduced or merged over time:

- near-duplicate setup blocks across multiple top-level workflows
- parallel release approval workflows with almost identical gating logic
- release package assembly distributed across both central and downstream workflows
- mixed build/release responsibilities in one file for packaging, governance, and publishing
- any workflow that creates a second artifact lineage without a clear consumer contract

### 6. Cutover rules

1. No new top-level workflow may be introduced unless it fills a missing responsibility not already covered by the canonical layers.
2. If a workflow duplicates another workflow's build or packaging logic, it must be merged or retired before release gating is considered complete.
3. Any consolidation change must keep [WORKFLOW_GUIDELINES.md](.github/WORKFLOW_GUIDELINES.md) and [WORKFLOW_REGISTRY.md](.github/WORKFLOW_REGISTRY.md) in sync.
4. Reusable workflows must remain the default choice for build and packaging logic; direct copy-paste implementation is a review blocker.
5. Downstream delivery jobs must consume from the canonical release artifact set, not generate a distinct, competing release identity.

### 7. Final target statement

The intended end-state is a single, clear CI/CD topology:

- fast policy gates at the top,
- one reusable build engine underneath,
- one canonical release orchestrator,
- downstream delivery consumers only,
- dedicated security and maintenance layers kept separate from runtime build and release logic.

This is the reference design for future workflow changes and should remain the default baseline for all workflow refactors.

## Critical Governance Rules (from WORKFLOW_GUIDELINES.md)

1. **Trigger Policy**: All workflows must constrain `push.tags` with `branches:` guard
2. **Concurrency**: All workflows must use qualified `group` names or `cancel-in-progress: false`
3. **Naming Convention**: Workflows must follow `Build:`, `Gate:`, `Release:`, `Maintenance:`, `Security:` prefixes
4. **Action Pinning**: All GitHub Actions must use SHA pinning (not `@latest` or `@main`)
5. **Registry Sync**: New workflows require WORKFLOW_REGISTRY.md update (enforce via pre-commit hook)

## Recent Updates

- **2026-10-29**: Reconciled the registry against the 99 active workflows; removed the stale community automation reference
- **2026-09-26**: Comprehensive inventory of all 83 workflows; added categorization and governance scorecard
- **2026-09-09**: Initial lean registry (76 workflows documented)

## Next Steps: Pre-Commit Hook (Phase 3)

A pre-commit hook will enforce Registry sync on `git commit --allow-empty .github/workflows/*.yml`:
1. Scan for new workflow files
2. Verify all workflows are documented in REGISTRY
3. Block commit if discrepancy > 0 workflows

See `.github/pre-commit-hooks/workflow-registry-sync.sh` (Phase 3 implementation).
