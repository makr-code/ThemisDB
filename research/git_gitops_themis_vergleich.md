# Git, GitHub und GitOps im Vergleich zur Versionskontrolle von ThemisDB

**Stand:** 18. Mai 2026
**Status:** Review Ready
**Kategorie:** 🔍 Research Review

---

## Abstract

Dieser Review vergleicht **Git**, **GitHub** und **GitOps** mit dem heute im Repository belegbaren Versions- und Änderungsmodell von **ThemisDB**. Das zentrale Ergebnis lautet: ThemisDB besitzt eine produktionsreife **MVCC-/Transaktionsschicht** für Datenversionierung, atomare Multi-Index-Commits und Konflikterkennung, ist aber **kein verteiltes Versionskontrollsystem für Daten** im Git-Sinne. GitHub-ähnliche Kollaborationsmechanismen (Pull Requests, Review-Gates, CI-Policies) existieren im Repository-Workflow des Projekts, nicht als generische Datenbankfunktion. GitOps-nahe Muster sind in ThemisDB bereits **teilweise** sichtbar — insbesondere durch breite YAML-Nutzung für Konfiguration, OpenAPI und Deployment-Artefakte —, eine YAML-basierte Schema-Steuerung des Datenbankkerns ist laut aktuellem Repository-Stand jedoch **weiterhin Konzept bzw. Plan**, nicht eingeführte Produktfunktion. Der Vergleich ist daher nur in Teilbereichen direkt tragfähig: **Snapshots, Historie, Rollback und deklarative Artefakte** sind sinnvoll vergleichbar; **Branches, Merges, Rebase, Data Pull Requests und vollautomatische Schema-Synchronisation** sind derzeit keine belegten Kernfeatures von ThemisDB.

---

## 1. Einleitung

Git, GitHub und GitOps werden oft als Referenzmodelle für Nachvollziehbarkeit, deklarative Workflows und reproduzierbare Änderungen verwendet. Für ThemisDB ist diese Perspektive attraktiv, weil das System laut `README.md` als **Multi-Model-Datenbank** mit **ACID-Transaktionen**, **MVCC**, **AQL** und **verteilten Komponenten** positioniert ist. Gerade dadurch entsteht leicht die Versuchung, ThemisDB als „Git für Daten“ oder als bereits GitOps-fähiges Datensystem zu beschreiben.

Für einen review-fähigen Artikel reicht eine solche Analogie aber nicht aus. Entscheidend ist, welche Aussagen sich gegen den aktuellen Repository-Stand tatsächlich belegen lassen:

1. **Welche Git-ähnlichen Eigenschaften sind heute implementiert?**
2. **Welche Eigenschaften existieren nur auf Ebene des Projekt-Workflows (GitHub/CI/CD)?**
3. **Welche GitOps- oder Git-inspirierten Datenfunktionen sind bisher nur als Forschung, Konzept oder Implementierungsplan dokumentiert?**

Dieser Artikel beantwortet diese Fragen anhand von Code, Tests, Benchmarks, Produktdokumentation und einschlägigen Research-Artefakten im Repository.

---

## 2. Methodik / Ansatz

Repository-Snapshot für diesen Review:

- Repository: `makr-code/ThemisDB`
- Prüfsnapshot: Commit `ad28140846783869d11d13a9540bb0523d471f4d`
- Review-Datum: 18.05.2026

Der Faktencheck kombiniert vier Evidenzklassen:

1. **Produktionscode**
   - `include/transaction/transaction_manager.h`
   - `src/transaction/transaction_manager.cpp`
   - `src/config/config_schema_validator.cpp`
   - `src/config/config_path_resolver.cpp`
   - `src/prompt_engineering/prompt_library_io.cpp`
2. **Tests und Benchmarks**
   - `tests/test_transaction_manager.cpp`
   - `benchmarks/bench_mvcc.cpp`
   - `benchmarks/bench_transaction_throughput.cpp`
   - `benchmarks/baselines/distributed/bench_transaction_v190_baseline.json`
3. **Produkt- und Architektur-Dokumentation**
   - `README.md`
   - `docs/de/architecture/architecture_mvcc.md`
   - `docs/de/features/features_transactions.md`
   - `docs/de/aql/aql_query_engine.md`
   - `docs/ci-cd/branching-release-history/BRANCHING_STRATEGY.md`
   - `openapi/openapi.yaml`
4. **Research-/Planungsdokumente für nicht implementierte Erweiterungen**
   - `research/bestehende_yaml_nutzung.md`
   - `research/schema/README.md`
   - `research/IMPLEMENTATION_PLAN_GIT_FEATURES.md`

Bewertungskriterien:

- Aussagen über den **Ist-Zustand** werden nur dann als belastbar behandelt, wenn sie sich direkt auf Code, Tests, Benchmarks oder produktnahe Doku zurückführen lassen.
- Aussagen über **GitOps-, Branching- oder Schema-as-Code-Erweiterungen** werden nur dann als implementiert beschrieben, wenn ein konkreter Codepfad oder eine produktive API belegbar ist.
- Reine Konzept- oder Planungsdokumente werden explizit als **Zukunftsperspektive** und nicht als Produktstatus gewertet.

---

## 3. Systemkontext und vereinheitlichte Terminologie

### 3.1 Git

Git ist ein **verteiltes Versionskontrollsystem für Quellcode und Dateien**. Seine Kernobjekte sind Commits, Trees, Blobs, Branches und Tags. Branching, Merge-Historien und Rebase beziehen sich auf eine commit-basierte DAG-Struktur.

### 3.2 GitHub

GitHub ist im hier relevanten Sinn **nicht Git selbst**, sondern die Kollaborations- und Automatisierungsebene darüber: Pull Requests, Reviews, Branch Protection, Actions/CI und Repository-Governance.

### 3.3 GitOps

GitOps bezeichnet ein Betriebsmodell, in dem **Git den deklarativen Soll-Zustand** für Infrastruktur oder Plattformartefakte hält und ein Operator diesen Zustand kontinuierlich mit einem Zielsystem abgleicht. GitOps setzt also typischerweise auf **deklarative Dateien**, **Pull-Request-Workflows** und **automatisierte Reconciliation**.

### 3.4 ThemisDB

Für dieses Dokument ist ThemisDB präzise so zu fassen:

- **Multi-Model-Datenbank** laut `README.md`
- **AQL (Advanced Query Language)** als zentrale deklarative Query-Sprache laut `docs/de/aql/aql_query_engine.md` und `openapi/openapi.yaml`
- **MVCC-/Transaktionsschicht** mit Snapshot-Isolation, Konflikterkennung und atomaren Multi-Index-Commits laut `docs/de/architecture/architecture_mvcc.md`, `docs/de/features/features_transactions.md` und `include/transaction/transaction_manager.h`
- **YAML-Nutzung** für Konfiguration, Validierung, Import/Export und Deployment-Artefakte laut `research/bestehende_yaml_nutzung.md`, `src/config/config_schema_validator.cpp`, `src/config/config_path_resolver.cpp` und `src/prompt_engineering/prompt_library_io.cpp`

Wichtig für die Terminologie:

- ThemisDB ist **kein Git-Ersatz** für Daten.
- Die fachlich richtige Bezeichnung für das Daten-Konsistenzmodell im hier geprüften Kontext ist **MVCC mit Snapshot-Isolation / ReadCommitted, Write-Write-Konflikterkennung und atomaren Transaktionen**.
- GitHub-Workflow-Regeln des Repositories sind **Projektprozess**, nicht automatisch Datenbank-Feature.
- YAML-basierte Schema-Steuerung ist nach aktuellem Stand **nicht** als allgemeine Kernfunktion belegt.

---

## 4. Vergleich: Wo die Analogie trägt — und wo nicht

### 4.1 Vergleichstabelle

| Aspekt | Git / GitHub / GitOps | ThemisDB (heutiger Stand) | Bewertung |
|---|---|---|---|
| **Atomarer Snapshot** | Commit erzeugt einen nachvollziehbaren Zustandswechsel | Transaktionen erzeugen konsistente Zustandswechsel; Snapshot-Isolation ist dokumentiert und implementiert | **Tragfähige Analogie** |
| **Historie / Nachvollziehbarkeit** | Commit-Historie, Review-Historie, Auditierbarkeit | MVCC-Versionen, Transaktionsstatus und Bench-/Testartefakte belegen Versions- und Commit-Logik | **Tragfähig, aber nicht identisch** |
| **Parallele Arbeit** | Branches + spätere Merges | Gleichzeitige Transaktionen mit Konflikterkennung; keine persistenten Daten-Branches als Kernfeature belegt | **Nur teilweise vergleichbar** |
| **Konflikte** | Merge-Konflikte beim Zusammenführen divergenter Historien | Write-Write-Konflikte bzw. OCC-/SSI-Konflikte zur Commit-Zeit | **Begrifflich ähnlich, technisch anders** |
| **Rollback** | Revert / Reset / Checkout historischer Zustände | Commit/Abort/Rollback innerhalb von Transaktionen vorhanden | **Tragfähig auf Operationsebene** |
| **Pull Requests / Reviews** | GitHub-Kollaborationsmodell | Im Repository-Workflow vorhanden, aber keine generische „Data PR“-Funktion als Produktmerkmal belegt | **Nur Projektprozess** |
| **Deklarativer Soll-Zustand** | GitOps: YAML/Manifest als Source of Truth | YAML ist breit vorhanden; allgemeines DB-Schema-as-Code laut `research/schema/README.md` weiterhin Konzept | **Teilweise vorhanden, nicht voll umgesetzt** |
| **Automatische Reconciliation** | Operator gleicht Git-Zustand mit Laufzeitsystem ab | Für Deployment-/Infra-Artefakte plausibel; für ThemisDB-Core-Schema nicht als implementierte Standardfunktion belegt | **Nicht als Kernfeature belegt** |
| **Branches / Merge-Engine / Rebase** | Kernfunktionen von Git | Für Datenbestand im geprüften Stand nicht als allgemeine Produktfunktion verifiziert | **Keine belastbare Gleichsetzung** |

### 4.2 Kernaussage des Vergleichs

Die beste Kurzform ist daher:

> **ThemisDB besitzt heute Git-ähnliche Eigenschaften bei Versionierung, Atomarität und Nachvollziehbarkeit von Datenänderungen, aber keine Git-äquivalente Branch-/Merge-Plattform für Daten. GitHub- und GitOps-Konzepte sind vor allem als Projektworkflow bzw. als Inspirationsquelle für deklarative Erweiterungen relevant.**

---

## 5. Evaluation / Experimente

Da für diesen Review keine neuen Laufzeitexperimente durchgeführt wurden, besteht die Evaluation aus einer **Repository-basierten Evidenzprüfung**. Sie stützt sich auf vorhandene Tests, Benchmarks, Baselines und implementierte Schnittstellen.

### 5.1 Verifizierte Kernclaims

| Claim | Status | Evidenz |
|---|---|---|
| **ThemisDB besitzt produktionsreife MVCC-/Transaktionsmechanismen** | **Belegt** | `docs/de/architecture/architecture_mvcc.md` beschreibt Snapshot-Isolation, Konflikterkennung und atomare Index-Updates; `include/transaction/transaction_manager.h` und `src/transaction/transaction_manager.cpp` implementieren die API und das Laufzeitverhalten. |
| **Transaktionen sind über mehrere Index-Arten atomar** | **Belegt** | `include/transaction/transaction_manager.h` exponiert Relational-, Graph- und Vector-Operationen; `tests/test_transaction_manager.cpp` prüft atomare Commits und Rollbacks über Primärdaten und Secondary Indexes. |
| **AQL ist die richtige Bezeichnung für die deklarative Query-Sprache** | **Belegt** | `docs/de/aql/aql_query_engine.md` beschreibt AQL als Query-Layer; `openapi/openapi.yaml` dokumentiert `/query/aql` als API-Endpunkt. |
| **YAML ist im Repository bereits breit verankert** | **Belegt** | `research/bestehende_yaml_nutzung.md`, `src/config/config_schema_validator.cpp`, `src/config/config_path_resolver.cpp` und `src/prompt_engineering/prompt_library_io.cpp` belegen produktive YAML-Codepfade. |
| **ThemisDB nutzt bereits YAML als allgemeines Schema-as-Code für den DB-Kern** | **Nicht belegt** | `research/schema/README.md` markiert YAML-basierte Schema-Definition ausdrücklich als „Konzept“ und „Zukünftig“. |
| **Git-inspirierte Erweiterungen wie Named Snapshots, Diff-API oder PITR sind vollständig eingeführte Kernfeatures** | **Nicht als aktueller Produktstatus belegbar** | `research/IMPLEMENTATION_PLAN_GIT_FEATURES.md` ist ein Implementierungsplan; die Datei selbst dokumentiert damit Zukunftsarbeit statt bereits ausgerollten Standardbetriebs. |

### 5.2 Test- und Benchmarklage

Die Repository-Artefakte stützen die obigen Claims zusätzlich:

1. **Transaktionstests**
   - `tests/test_transaction_manager.cpp` prüft u. a. Begin/Commit/Rollback sowie atomare Multi-Entity-Commits.
   - Die deutschsprachige MVCC-Architekturdoku nennt zusätzlich einen konsolidierten Teststatus für Transaktions- und MVCC-Tests.

2. **MVCC-Benchmarks**
   - `benchmarks/bench_mvcc.cpp` enthält Benchmarks für Single-Entity-Commit, Batch-Insert, Snapshot-Overhead und Rollback.
   - Das ist ein belastbarer Hinweis darauf, dass MVCC im Repository nicht nur dokumentiert, sondern auch als Performance-Thema explizit gemessen wird.

3. **Transaktions-Baselines und Lückenbild**
   - `benchmarks/bench_transaction_throughput.cpp` und `benchmarks/baselines/distributed/bench_transaction_v190_baseline.json` zeigen, dass Transaktions-SLOs und Benchmark-Mapping gepflegt werden.
   - Gleichzeitig weist die Baseline mehrere Fälle explizit als **proxy/gap** aus. Das ist wichtig: Nicht jede fortgeschrittene, Git-inspirierte oder verteilte Workflow-Idee ist bereits benchmark-seitig vollständig abgesichert.

### 5.3 Bewertung der GitOps-Frage

Die GitOps-Frage fällt differenziert aus:

- **Ja:** ThemisDB verwendet bereits deklarative YAML-Artefakte in mehreren realen Bereichen.
- **Ja:** Das Repository selbst nutzt GitHub-/Branching-/CI-Regeln, die gut zu GitOps-Denkmustern passen.
- **Nein:** Daraus folgt **nicht**, dass der Datenbankkern schon heute standardmäßig per GitOps-Schema-Datei gesteuert wird.
- **Nein:** Die im alten Artikel gezeigten CLI- und Workflow-Beispiele (`themis schema apply`, Data Branches, Data Pull Requests) sind in dieser Form nicht als aktuell eingeführte Standardoberfläche belegbar.

---

## 6. Limitations / Known Issues

1. **Repository-basierte Bewertung:** Dieser Review prüft Repository-Stand, Doku, Code, Tests und Benchmarks — nicht den vollständigen Betrieb aller Editionen oder Deployment-Topologien.
2. **Analogie ist nicht Gleichheit:** Ein Git-Commit und ein DB-Commit sind beide atomare Zustandswechsel, aber ihre Datenmodelle, Historienstrukturen und Konfliktsemantiken sind verschieden.
3. **GitHub ≠ Datenbankfunktion:** Branch Protection, Pull Requests und CI/CD sind klar als Projektworkflow des Repositories belegbar, nicht als native Data-Collaboration-Schicht von ThemisDB.
4. **GitOps nur teilweise anschlussfähig:** YAML existiert breit, aber „YAML vorhanden“ ist nicht gleich „Core-Schema wird GitOps-gesteuert“. Gerade hier muss zwischen produktivem Ist-Zustand und Konzeptpapieren sauber getrennt werden.
5. **Zukunftsfeatures bleiben Zukunftsfeatures:** Research- und Planungsdokumente zu Named Snapshots, Diff-API, PITR oder Schema-as-Code sind wertvoll, dürfen aber ohne zugehörigen Codepfad nicht als bereits ausgelieferte Kernfunktion beschrieben werden.
6. **Benchmark-Abdeckung ist selektiv:** Die vorhandenen MVCC-/Transaction-Benchmarks belegen ernsthafte Performance-Arbeit, aber die Baseline-Dateien zeigen zugleich, dass einzelne Zielbilder noch als Proxy oder Gap geführt werden.

---

## 7. Fazit

Die überarbeitete Bewertung ist bewusst nüchtern:

- **Git** ist das richtige Referenzmodell für commit-basierte, dateiorientierte Versionskontrolle.
- **GitHub** ist das passende Referenzmodell für Review-, Branching- und CI/CD-Governance des ThemisDB-Repositories.
- **GitOps** ist ein sinnvolles Denkmodell für deklarative Betriebsartefakte und mögliche zukünftige Schema-/Deployment-Workflows.
- **ThemisDB** besitzt heute bereits eine starke, belegte Datenversions- und Transaktionsbasis mit MVCC, AQL und YAML-gestützten Konfigurationspfaden.

Die fachlich saubere Schlussfolgerung lautet daher:

> **ThemisDB ist heute kein „Git für Daten“, aber eine multi-modale Datenbank mit belastbarer MVCC-/Transaktionsarchitektur, die mehrere Git- und GitOps-Ideen sinnvoll aufgreifen kann.**

Für Architektur-, Produkt- und Research-Kommunikation sollte man deshalb konsequent zwischen

1. **heute belegtem Produktverhalten**,
2. **Repository-/GitHub-Prozessregeln** und
3. **geplanten Git-/GitOps-inspirierten Erweiterungen**

unterscheiden.

---

## References / Quellen

### Externe Referenzen

1. Git Documentation.
   URL: https://git-scm.com/doc
2. GitHub Docs: About workflows / GitHub Actions.
   URL: https://docs.github.com/en/actions
3. OpenGitOps: GitOps Principles.
   URL: https://opengitops.dev/
4. Argo CD Documentation.
   URL: https://argo-cd.readthedocs.io/
5. Flux Documentation.
   URL: https://fluxcd.io/

### Repository-Artefakte (commit-gepinnt)

6. ThemisDB README — Positionierung als Multi-Model-Datenbank mit ACID/MVCC/AQL/Distributed-Fokus.
   URL: https://github.com/makr-code/ThemisDB/blob/ad28140846783869d11d13a9540bb0523d471f4d/README.md
7. MVCC-Architekturübersicht (DE).
   URL: https://github.com/makr-code/ThemisDB/blob/ad28140846783869d11d13a9540bb0523d471f4d/docs/de/architecture/architecture_mvcc.md
8. TransactionManager Public API.
   URL: https://github.com/makr-code/ThemisDB/blob/ad28140846783869d11d13a9540bb0523d471f4d/include/transaction/transaction_manager.h
9. TransactionManager Laufzeitimplementierung.
   URL: https://github.com/makr-code/ThemisDB/blob/ad28140846783869d11d13a9540bb0523d471f4d/src/transaction/transaction_manager.cpp
10. Transaction-Tests.
    URL: https://github.com/makr-code/ThemisDB/blob/ad28140846783869d11d13a9540bb0523d471f4d/tests/test_transaction_manager.cpp
11. MVCC-Benchmarks.
    URL: https://github.com/makr-code/ThemisDB/blob/ad28140846783869d11d13a9540bb0523d471f4d/benchmarks/bench_mvcc.cpp
12. Transaction-Benchmark-Baseline (`bench_transaction_v190_baseline.json`).
    URL: https://github.com/makr-code/ThemisDB/blob/ad28140846783869d11d13a9540bb0523d471f4d/benchmarks/baselines/distributed/bench_transaction_v190_baseline.json
13. AQL Query Engine (DE).
    URL: https://github.com/makr-code/ThemisDB/blob/ad28140846783869d11d13a9540bb0523d471f4d/docs/de/aql/aql_query_engine.md
14. OpenAPI-Spezifikation mit `/query/aql`.
    URL: https://github.com/makr-code/ThemisDB/blob/ad28140846783869d11d13a9540bb0523d471f4d/openapi/openapi.yaml
15. Git-Branching-Strategie des Repositories.
    URL: https://github.com/makr-code/ThemisDB/blob/ad28140846783869d11d13a9540bb0523d471f4d/docs/ci-cd/branching-release-history/BRANCHING_STRATEGY.md
16. Review zur bestehenden YAML-Nutzung.
    URL: https://github.com/makr-code/ThemisDB/blob/ad28140846783869d11d13a9540bb0523d471f4d/research/bestehende_yaml_nutzung.md
17. YAML-Schema-Beispiele — explizit als Konzept markiert.
    URL: https://github.com/makr-code/ThemisDB/blob/ad28140846783869d11d13a9540bb0523d471f4d/research/schema/README.md
18. Implementierungsplan für Git-ähnliche Features.
    URL: https://github.com/makr-code/ThemisDB/blob/ad28140846783869d11d13a9540bb0523d471f4d/research/IMPLEMENTATION_PLAN_GIT_FEATURES.md
