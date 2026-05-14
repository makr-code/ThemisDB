# Algorithm Validation Process — ThemisDB

**Status:** reviewed, repository-aligned process document
**Last updated:** 2026-05-14
**Scope:** algorithmic changes with measurable performance, latency, memory, or quality impact in ThemisDB modules

## Abstract

Dieses Dokument beschreibt den verbindlichen Validierungsprozess für algorithmische Änderungen in ThemisDB. Der Prozess ist erst dann abgeschlossen, wenn folgende Punkte erfüllt sind:

- ein messbares Ziel aus den Performance-Erwartungen oder einem gleichwertigen Modul-Artefakt ist festgelegt
- eine reproduzierbare Baseline ist eingefroren
- Alternativen sind strukturiert verglichen
- Ergebnisse sind nachvollziehbar ausgewertet
- Regression-Gates sind an die bestehende Benchmark-Infrastruktur angebunden
- die Entscheidung ist als Research- und ADR-Artefakt dokumentiert

Der Text wurde gegen den aktuellen Repository-Stand verifiziert.

- **Bestätigt:**
  - kanonische SLO- und KPI-Quellen
  - Ziel-ID-zu-Benchmark-Mapping und Mapping-Validierer
  - Performance-Regression-Detektor und zentraler PR-Regression-Check
  - Nightly-Benchmark-Sweep
  - Research-Verzeichnisse für Experimente, ADRs und den Implementation-Influence-Index
- **Entfernt oder eingegrenzt:** Aussagen über nicht im Repository nachweisbare Hilfsskripte oder bereits vollständig dokumentierte End-to-End-Fallstudien.

## 1. Einleitung

ThemisDB wird im Repository konsistent als **Multi-Model-Datenbank** mit modulbezogenen Performance-Zielen, Benchmark-Harnesses und Research-Artefakten geführt. Für algorithmische Änderungen genügt daher weder ein isolierter Mikrobenchmark noch eine unbelegte Literaturbehauptung. Erforderlich ist eine nachvollziehbare Kette aus Problem, Methodik, Evaluation, Grenzen und Entscheidung.

Der folgende Prozess ist auf Änderungen zugeschnitten, die mindestens eines der folgenden Ziele betreffen:

- P95- oder P99-Latenz
- Throughput bzw. `items_per_second`
- Speicherverbrauch bzw. Peak RSS
- Ergebnisqualität bei algorithmisch relevanten Ranking-, Retrieval- oder Optimierungsverfahren
- Regression-Resistenz in CI

Nicht jede kleine Refaktorierung braucht einen vollständigen Research-Zyklus. Sobald aber ein Change als algorithmische Verbesserung kommuniziert, in Roadmaps priorisiert oder über SLOs gerechtfertigt wird, ist dieser Prozess verpflichtend.

## 2. Methodik und Verifikationsbasis

### 2.1 Verifikationsprinzip

Dieses Dokument trennt bewusst zwischen zwei Evidenzarten:

1. **Repository-Evidenz:** reale Dateien, Workflows, Skripte und Research-Artefakte in ThemisDB.
2. **Methodische Evidenz:** allgemein anerkannte Benchmarking- und Statistikquellen für Versuchsdesign, Signifikanztests und Ausreißerbehandlung.

Repository-spezifische Aussagen in diesem Dokument wurden gegen die aktuelle Codebasis geprüft. Methodische Empfehlungen werden als Leitplanken formuliert; sie ersetzen keine Modul-spezifische Fachprüfung.

### 2.2 Gegen den aktuellen Stand bestätigte Prozessbausteine

| Prozessbaustein | Repository-Artefakt | Verifikationsstatus |
|---|---|---|
| Kanonische KPI-/SLO-Definitionen | [`../PERFORMANCE_EXPECTATIONS.md`](../PERFORMANCE_EXPECTATIONS.md) | Bestätigt |
| Ziel-ID → Benchmark-Mapping | [`../benchmarks/benchmark_target_mapping.json`](../benchmarks/benchmark_target_mapping.json) | Bestätigt |
| Mapping-Validierung | [`../tools/verify_benchmark_mapping.py`](../tools/verify_benchmark_mapping.py) | Bestätigt |
| Regression-Detektor | [`../benchmarks/performance_regression_detector.py`](../benchmarks/performance_regression_detector.py) | Bestätigt |
| PR-Gate für Regressionen | [`../.github/workflows/performance-regression-check.yml`](../.github/workflows/performance-regression-check.yml) | Bestätigt |
| Nightly-Benchmark-Sweep | [`../.github/workflows/07-quality_nightly-benchmark-sweep.yml`](../.github/workflows/07-quality_nightly-benchmark-sweep.yml) | Bestätigt |
| Experiment-Protokolle | [`experiments/README.md`](experiments/README.md) | Bestätigt |
| Entscheidungslogik | [`architecture_decisions/adr_009_algorithm_validation_framework.md`](architecture_decisions/adr_009_algorithm_validation_framework.md) | Bestätigt |
| Research-Traceability | [`implementation_influence/README.md`](implementation_influence/README.md) | Bestätigt |

### 2.3 Terminologie

Im restlichen Dokument werden die folgenden Begriffe einheitlich verwendet:

- **Ziel-ID:** kanonischer Identifier für ein messbares Ziel
- **SLO:** quantifizierter Zielwert für Latenz, Throughput, Speicher oder Qualität
- **Baseline:** eingefrorener Ausgangszustand mit reproduzierbaren Messdaten
- **Kandidat:** alternative Implementierung, Bibliothek oder Konfiguration
- **Regression Gate:** automatisierte Prüfung in CI gegen Baseline oder SLO
- **ADR:** Architecture Decision Record für Adopt/Reject-Entscheidungen
- **AQL:** Query-Sprache von ThemisDB; keine alternativen Schreibweisen verwenden
- **Multi-Model-Datenbank:** bevorzugter Systembegriff für ThemisDB

## 3. Validierungsprozess

### 3.1 Schritt 1 — Ziel-ID und Akzeptanzkriterium festlegen

Jede algorithmische Arbeit beginnt mit einer expliziten Ziel-ID oder einem äquivalenten, bereits dokumentierten Modulziel. Für modulübergreifende Performance-Ziele ist die kanonische Root-Quelle [`../PERFORMANCE_EXPECTATIONS.md`](../PERFORMANCE_EXPECTATIONS.md); zusätzlich können modulinterne `PERFORMANCE_EXPECTATIONS.md`-Dateien oder klar definierte Zieltabellen herangezogen werden.

**Pflichtartefakte:**

- referenzierte Ziel-ID oder ein explizit dokumentiertes Ersatzkriterium
- konkreter SLO-Wert
- betroffener Hot Path oder betroffene API/Operation
- geplanter Roadmap-/FUTURE_ENHANCEMENTS-Eintrag im Modul

**Mindestfragen:**

- Welches Verhalten ist heute zu langsam, zu speicherintensiv oder qualitativ unzureichend?
- Welche Metrik entscheidet über Erfolg oder Misserfolg?
- Welche reale Query, welches Dataset oder welcher Benchmark-Fall repräsentiert das Problem?

### 3.2 Schritt 2 — Baseline einfrieren

Vor jeder Änderung muss die bestehende Implementierung als reproduzierbare Baseline erfasst werden. Maßgeblich sind dabei dieselben Binärartefakte, Build-Flags und Eingabedaten, die später auch für Kandidatenmessungen verwendet werden.

**Erforderliche Baseline-Bestandteile:**

| Artefakt | Erwarteter Inhalt |
|---|---|
| Benchmark-JSON | maschinenlesbare Messwerte des Ausgangszustands |
| Hardware-/Build-Profil | CPU/GPU, Speicher, OS, Compiler, relevante Flags |
| Problemkontext | Modul, Ziel-ID, Hot Path, bekannte Engpässe |
| Commit-Bezug | exakte Version der Baseline |

**Repository-Bezug:**

- Mappings zwischen Ziel-ID und Benchmarks stehen in [`../benchmarks/benchmark_target_mapping.json`](../benchmarks/benchmark_target_mapping.json).
- Das Repository validiert diese Zuordnung mit [`../tools/verify_benchmark_mapping.py`](../tools/verify_benchmark_mapping.py).
- Experimentprotokolle sind unter [`experiments/README.md`](experiments/README.md) strukturiert beschrieben.

### 3.3 Schritt 3 — Kandidaten systematisch sammeln

Eine Optimierung wird nicht gegen eine einzige Lieblingsidee validiert. Stattdessen werden mehrere realistische Kandidaten gesammelt und nach denselben Kriterien beschrieben. Geeignete Kandidaten sind:

- alternative Datenstrukturen oder Algorithmen
- bewährte Bibliotheken oder Runtime-Konfigurationen
- parameterisierte Varianten derselben Implementierung
- best-practice-getriebene Änderungen mit klarer Messhypothese

**Pflichtinhalt pro Kandidat:**

- Quelle mit URL oder DOI
- Kernidee in einem Satz
- erwarteter Effekt auf Latenz, Throughput, Speicher oder Qualität
- Integrationsaufwand und technische Risiken
- Annahmen und Abhängigkeiten

Dafür nutzt ThemisDB die bestehenden Vorlagen unter [`papers/_template_paper.md`](papers/_template_paper.md) und [`best_practices/_template_best_practice.md`](best_practices/_template_best_practice.md).

### 3.4 Schritt 4 — Experimente standardisiert ausführen

Alle Kandidaten werden unter möglichst identischen Bedingungen gemessen. Abweichungen von Hardware, Compiler, Datensätzen oder Lastprofilen müssen im Protokoll explizit erklärt werden.

**Empfohlene Mindestmethodik:**

- mehrere unabhängige Runs statt Einzelmessung
- dokumentierter Warmup und Messdauer
- Auswertung mindestens von P50, P95, P99, Throughput und Peak RSS, sofern die Metriken für das Modul sinnvoll sind
- Signifikanztest nur dann berichten, wenn Eingangsgrößen, Stichprobengröße und Testannahmen dokumentiert sind
- Ausreißerbehandlung nur mit offengelegter Regel

**Wichtig:** Statistische Vergleiche müssen im Experimentprotokoll immer mit einem explizit dokumentierten Auswertungspfad hinterlegt werden. Wenn Welch-Test, Mann-Whitney-U-Test oder Effektstärken berichtet werden, muss das verwendete Skript, Notebook oder der Auswertungspfad im Experimentprotokoll mit angegeben werden. Für den Pfad `tools/benchmark_compare.py` gibt es im aktuellen Repository kein Standard-Skript.

**Empfohlener Ersatzpfad:** Die Auswertung kann mit einem versionierten SciPy-/Python-Skript, einem R-Skript oder einem eingecheckten Notebook erfolgen. Entscheidend ist nicht das Tool selbst, sondern dass Eingabedaten, Testparameter, Signifikanzniveau und erzeugte Kennzahlen zusammen mit dem Experimentprotokoll reproduzierbar abgelegt werden. Die erwartete Struktur für dieses Protokoll ist in [`experiments/README.md`](experiments/README.md) beschrieben.

**Praktischer Mindest-Output pro Experiment:**

- Rohdaten (JSON)
- tabellarischer Vergleich Baseline vs. Kandidat
- Interpretation der gemessenen Deltas
- klare Empfehlung: Adopt, Reject oder weiterer Test

### 3.5 Schritt 5 — Regression Gates an die bestehende CI anbinden

Ein Kandidat gilt nicht als übernommen, solange die Verbesserung nicht gegen spätere Regressionen abgesichert ist. ThemisDB besitzt dafür bereits eine funktionierende Infrastruktur:

- [`../.github/workflows/performance-regression-check.yml`](../.github/workflows/performance-regression-check.yml) führt einen zentralen PR-Regressionstest aus.
- [`../.github/workflows/07-quality_nightly-benchmark-sweep.yml`](../.github/workflows/07-quality_nightly-benchmark-sweep.yml) führt Nightly-Benchmarks aus und prüft die Abdeckung.
- [`../benchmarks/performance_regression_detector.py`](../benchmarks/performance_regression_detector.py) unterstützt konfigurierbare Schwellenwerte; die Defaults liegen bei 5 % / 10 % / 20 % für minor / major / critical.

**Verpflichtend vor Adoption:**

- Ziel-ID-Mapping ist aktualisiert und validiert.
- Der relevante Benchmark läuft in CI.
- Der Failure-Modus ist dokumentiert (z. B. Block bei major regression).
- Die Interpretation des Gates ist identisch mit dem im Experimentbericht ausgewiesenen SLO.

### 3.6 Schritt 6 — Entscheidung dokumentieren und rückverfolgbar machen

Am Ende steht keine lose Notiz, sondern eine belastbare Entscheidung. Dafür müssen mindestens diese Artefakte aktualisiert werden:

- ADR unter [`architecture_decisions/`](architecture_decisions/README.md)
- Experimentprotokoll unter [`experiments/`](experiments/README.md)
- Research- oder Best-Practice-Eintrag für die Quellenlage
- Eintrag im [`implementation_influence/README.md`](implementation_influence/README.md)
- Modul-Roadmap und ggf. `FUTURE_ENHANCEMENTS.md`

Adopt- und Reject-Entscheidungen sind gleichermaßen dokumentationspflichtig. Auch ein verworfener Kandidat spart künftige Doppelarbeit, wenn die Begründung nachvollziehbar archiviert ist.

## 4. Evaluation des aktuellen ThemisDB-Prozessstands

### 4.1 Was der aktuelle Repository-Stand bereits gut abdeckt

Der Prozess ist im Repository nicht rein theoretisch, sondern institutionell verankert:

- Root-Performance-Erwartungen und modulare Zieldefinitionen existieren.
- Ziel-ID-Mapping und Mapping-Validierung sind produktiv vorhanden.
- Es gibt sowohl ein zentrales PR-Gate als auch einen Nightly-Sweep für Benchmarks.
- Experimente, ADRs und der Influence-Index besitzen eigene, dokumentierte Verzeichnisse.
- ADR-009 beschreibt das 6-Schritte-Framework als akzeptierte Architekturentscheidung.

Damit ist die Infrastruktur für reproduzierbare algorithmische Entscheidungen grundsätzlich vorhanden.

### 4.2 Wo frühere Formulierungen zu stark waren

Bei der Review dieses Dokuments wurden mehrere Aussagen entschärft oder korrigiert:

1. **Nicht vorhandenes Hilfsskript entfernt:** Ein Verweis auf `tools/benchmark_compare.py` war nicht belegbar und wurde gestrichen; stattdessen fordert das Dokument nun einen versionierten, im Experimentprotokoll referenzierten Auswertungspfad.
2. **Relative Pfade korrigiert:** Verweise auf Root- und `src/`-Artefakte nutzen nun gültige Pfade aus `research/` heraus.
3. **Beispielcharakter des mimalloc-Falls eingegrenzt:** ADR-009 nennt mimalloc als internes Vorbild. Dieses Prozessdokument behauptet jedoch nicht mehr, dass `research/ALGORITHM_VALIDATION_PROCESS.md` selbst bereits ein vollständig nachprüfbares, in sich abgeschlossenes End-to-End-Fallbeispiel enthält.
4. **CI-Aussagen auf belegte Artefakte reduziert:** Es wird nur auf Workflows und Skripte verwiesen, die aktuell im Repository existieren.
5. **Statistikempfehlungen von Repository-Automation getrennt:** Methodische Empfehlungen bleiben erlaubt, werden aber nicht als bereits implementierte Standardtoolchain dargestellt.

## 5. Limitations / Known Issues

- Dieses Dokument definiert einen verbindlichen Prozess, ersetzt aber keine modul-spezifische Fachprüfung der Metriken oder Datensätze.
- Nicht jedes Modul besitzt denselben Reifegrad an Benchmark-Harnesses; der Prozess darf deshalb nicht behaupten, dass jede Ziel-ID bereits vollständig automatisiert messbar ist.
- Statistische Tests sind nur belastbar, wenn Stichprobengröße, Datenverteilung und Auswertungsweg dokumentiert werden.
- Manche Qualitätsziele sind proxy-basiert oder nur indirekt messbar; in solchen Fällen muss das Protokoll die Proxy-Begründung explizit festhalten.
- Externe Literatur verbessert die Kandidatensuche, ist aber nie Ersatz für eine reproduzierbare ThemisDB-interne Messung.

## 6. Abschlusskriterium

Eine algorithmische Änderung gilt in ThemisDB erst dann als **gewonnen**, wenn alle folgenden Fragen mit `ja` beantwortet werden können:

- Ist das Problem an eine Ziel-ID oder ein gleichwertiges Akzeptanzkriterium gebunden?
- Existiert eine reproduzierbare Baseline?
- Wurden mehrere Kandidaten mit dokumentierter Quellenlage verglichen?
- Liegen Messdaten und eine nachvollziehbare Interpretation vor?
- Blockiert CI spätere Regressionen für den relevanten Benchmarkpfad?
- Ist die Entscheidung in ADR, Experimentprotokoll, Research-Index und Modulplanung rückverfolgbar dokumentiert?

Wenn eine dieser Fragen offen bleibt, befindet sich die Änderung weiterhin in der Experiment- oder Explorationsphase.

## References

### Interne ThemisDB-Quellen

1. ThemisDB: [`../PERFORMANCE_EXPECTATIONS.md`](../PERFORMANCE_EXPECTATIONS.md)
2. ThemisDB: [`../benchmarks/benchmark_target_mapping.json`](../benchmarks/benchmark_target_mapping.json)
3. ThemisDB: [`../tools/verify_benchmark_mapping.py`](../tools/verify_benchmark_mapping.py)
4. ThemisDB: [`../benchmarks/performance_regression_detector.py`](../benchmarks/performance_regression_detector.py)
5. ThemisDB: [`../.github/workflows/performance-regression-check.yml`](../.github/workflows/performance-regression-check.yml)
6. ThemisDB: [`../.github/workflows/07-quality_nightly-benchmark-sweep.yml`](../.github/workflows/07-quality_nightly-benchmark-sweep.yml)
7. ThemisDB: [`architecture_decisions/adr_009_algorithm_validation_framework.md`](architecture_decisions/adr_009_algorithm_validation_framework.md)
8. ThemisDB: [`experiments/README.md`](experiments/README.md)
9. ThemisDB: [`implementation_influence/README.md`](implementation_influence/README.md)
10. ThemisDB: [`papers/_template_paper.md`](papers/_template_paper.md)
11. ThemisDB: [`best_practices/_template_best_practice.md`](best_practices/_template_best_practice.md)

### Externe Methoden- und Benchmark-Referenzen

12. Google Benchmark project. <https://github.com/google/benchmark>
13. B. L. Welch. *The Generalization of “Student's” Problem when Several Different Population Variances are Involved.* Biometrika, 34(1/2), 1947. DOI: <https://doi.org/10.2307/2332510>
14. H. B. Mann, D. R. Whitney. *On a Test of Whether one of Two Random Variables is Stochastically Larger than the Other.* Annals of Mathematical Statistics, 18(1), 1947. DOI: <https://doi.org/10.1214/aoms/1177730491>
15. J. W. Tukey. *Exploratory Data Analysis.* Addison-Wesley, 1977. URL: <https://books.google.com/books?id=R-1QAAAAMAAJ>
16. TPC Benchmark C. Transaction Processing Performance Council. <https://www.tpc.org/tpcc/>
17. TPC Benchmark H. Transaction Processing Performance Council. <https://www.tpc.org/tpch/>
18. ANN-Benchmarks project. <https://github.com/erikbern/ann-benchmarks>

---

*Review-Hinweis:* Dieses Dokument beschreibt den Prozess. Konkrete Messergebnisse gehören in modul- oder zielbezogene Experimentprotokolle und ADRs, nicht in diese Übersichtsseite.
