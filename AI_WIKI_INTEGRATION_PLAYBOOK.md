# AI Wiki Integration Playbook fuer ThemisDB

Datum: 2026-07-28
Status: Active
Bezug: Uebertragung des LLM-Wiki-Musters auf ThemisDB Development-Management und AI Vibe Coding
Primary (Quelle der Wahrheit): ROADMAP.md, FUTURE_ENHANCEMENTS.md, DOCUMENTATION_GOVERNANCE.md, BRANCHING_STRATEGY.md, RELEASE_STRATEGY.md, ai_context/COPILOT_INSTRUCTIONS.md

---

## 1. Zielbild

Dieses Dokument definiert, wie ThemisDB ein LLM-Wiki als produktives AI-Arbeitswerkzeug betreibt.

Kernziel:
- Kontextverlust in grossen, komplexen Modulen reduzieren
- Entscheidungen, Invarianten und Testwissen persistent kompilieren
- AI-Agenten mit einem reproduzierbaren, governance-konformen Arbeitsprozess betreiben
- Offline-Ollama gezielt fuer Inference- und Coding-Workloads einsetzen

Das Modell folgt dem LLM-Wiki-Prinzip:
- Nicht nur Retrieval aus Rohdokumenten
- Sondern kontinuierliche Verdichtung in eine persistente, verlinkte Wissensschicht

---

## 2. Uebertragbarkeitsanalyse fuer ThemisDB

### 2.1 Ergebnis

Die Uebertragbarkeit auf ThemisDB ist hoch.

Bereits vorhanden:
- Eigene AI-Kontextschicht in ai_context/
- Arbeits- und Evidenzschicht in ai_working/
- Modulnahe, strukturierte Dokumentation in src/<module>/*.md
- Governance fuer Source-of-Truth, Dokumentationsfluss und Branching

Luecken fuer eine vollstaendige LLM-Wiki-Operationalisierung:
- Einheitlicher Ingest-Query-Lint Loop
- Globaler Knowledge-Index mit stabiler Seitentypisierung
- Chronologisches Knowledge-Log als Audit-Trail
- Verbindliche Drift- und Widerspruchspruefungen
- Tool-Routing-Regeln (offline/online) als operative SOP

### 2.2 Erwarteter Nutzen

- Schnellere Einarbeitung in Module trotz hoher Codebasis-Komplexitaet
- Weniger Wiederholungsarbeit bei Agenten-Sessions
- Reduzierte Halluzinationsrate durch kompilierte Wissensbasis
- Bessere Nachvollziehbarkeit von Architektur- und Testentscheidungen
- Reproduzierbare AI-Lieferprozesse statt ad-hoc Prompting

---

## 3. Zielarchitektur (4 Schichten)

### 3.1 Layer A: Raw Sources (immutable)

Beispiele:
- Code, Header, Tests, Benchmarks
- PRs, Issues, Incident-Logs
- Build- und Test-Reports
- Externe Spezifikationen

Regeln:
- Raw Sources werden nicht von AI umgeschrieben
- Nur append/aktualisieren ueber normalen Dev-Prozess

### 3.2 Layer B: Compiled Wiki (LLM-maintained)

Beispiele:
- Modul-Status-Synthesen
- Entity/Subsystem-Pages
- Decision-Pages
- Incident-Pages
- Contract- und Benchmark-Pages

Regeln:
- AI darf diese Schicht aktiv schreiben/aktualisieren
- Jede relevante Query-Erkenntnis kann als neue Wiki-Seite persistiert werden

### 3.3 Layer C: Schema and Rules (governance)

Beispiele:
- ai_context/COPILOT_INSTRUCTIONS.md
- .github/copilot-instructions.md
- DOCUMENTATION_GOVERNANCE.md
- Dieses Playbook

Regeln:
- Schema-Aenderungen mit hoeherer Review-Huerde
- Regeln muessen branch- und release-konform bleiben

### 3.4 Layer D: Delivery/Publication

Beispiele:
- Root-Reports
- docs/ Publikationsartefakte
- Release-nahe Uebersichten

Regeln:
- Nur downstream aus A bis C
- Keine isolierte Parallel-Wahrheit

---

## 4. Pflichtobjekte des LLM-Wiki-Betriebs

Im Root sind folgende Dateien verpflichtend bzw. empfohlen:

Pflicht:
- Ein globaler Index: INDEX.md
- Ein append-only Log: LOG.md
- Dieses Betriebsdokument: AI_WIKI_INTEGRATION_PLAYBOOK.md

Empfohlen:
- ai_context/KNOWLEDGE_LINT_REPORT.md (periodische Lint-Ergebnisse)
- ai_context/KNOWLEDGE_CONFLICTS.md (offene Widersprueche und Klaerungen)

Formatregeln:
- Jede Wiki-Seite hat klaren Scope
- Jede starke Status-Behauptung verweist auf kanonische Quellen
- Keine semantischen Duplikatseiten

---

## 5. Standard-Workflow (Ingest -> Query -> Lint)

### 5.1 Ingest

Trigger:
- Neue Quelle (Issue, PR, Incident, Benchmark, Design-Entscheidung)

Schritte:
1. Quelle klassifizieren (Modul, SOT-Domain, Kritikalitaet)
2. Zielseiten im Wiki bestimmen (bestehende Seite aktualisieren oder neue Seite)
3. Index aktualisieren
4. Log-Eintrag schreiben
5. Bei Konflikten conflict-Entry erzeugen

### 5.2 Query

Schritte:
1. Zuerst INDEX.md lesen
2. Relevante 3-7 Seiten priorisieren
3. Antwort mit Quellenverweisen erzeugen
4. Bei hoher Wiederverwendungswahrscheinlichkeit Ergebnis als Wiki-Seite persistieren

### 5.3 Lint

Minimale Lint-Pruefungen:
- Orphan-Seiten
- Defekte interne Links
- Stale Claims gegen neuere Quellen
- Widerspruechliche Kern-Claims
- Fehlende Cross-Links bei stark gekoppelten Modulen

Frequenz:
- Nach groesseren Ingest-Batches
- Mindestens nightly
- Zusaetzlich vor Release-Readiness-Gates

---

## 6. Tooling-Integration Offline und Online

## 6.1 Offline-Werkzeuge (Default-Backbone)

### VS Code (offline)

Use Cases:
- Code Navigation, Symbolsuche, lokale Aufgaben
- CMake/CTest-Tasks
- Problems/Diagnostics
- Lokale Markdown-Pflege und Review

Regeln:
- Erst lokale Build/Test-Evidenz, dann Wiki-Status aktualisieren
- Task-Ausgaben als Evidenz in Ingest/Log beruecksichtigen

### Git lokal (offline)

Use Cases:
- Versionierung des Wikis
- Reviewbare Diffs fuer Wissensaenderungen
- Branch-basierte Wissensarbeit

Regeln:
- Knowledge-Aenderungen in nachvollziehbaren Commits
- Kein ungepruefter Massenrewrite

### Ollama lokal/remote-im-LAN als Offline-AI

Rolle:
- Primärer Inference- und Coding-Beschleuniger fuer wiederholbare technische Aufgaben
- Erstellung/Update von Wiki-Seiten aus lokalen Quellen
- Cross-Linking und Strukturpflege im Compiled Layer

Konfiguration:
- Endpoint gemaess Repo-Setup: http://192.168.178.106:11434
- Delegation-Modus vorzugsweise auto mit klaren Routing-Regeln

ThemisDB-Routingregeln (verbindlich):
- C/C++ Dateien: an @ollama /local delegieren
- CMake/Build-System-Themen: an @ollama delegieren
- Security/Audit/Architektur-Entscheidungen: Standard-Copilot (Cloud) priorisieren
- ROADMAP/FUTURE_ENHANCEMENTS Governance-Updates: Standard-Copilot

Empfohlene Ollama-Modelle nach Zweck:
- qwen2.5-coder:14b fuer C/C++-Implementierungs- und Refactor-Aufgaben
- deepseek-coder-v2:16b fuer groessere CMake-/Massenrefactor-Themen (ohne Tool-Use)
- gemma4:latest fuer Agent-Chat/Tooling mit breiter Aufgabenabdeckung
- phi4:latest fuer Reasoning-Textarbeit ohne Tool-Use

Inference-Aufgaben fuer Ollama (priorisiert):
- API-nahe Boilerplate und strukturelle C++-Generierung
- Unit-Test-Generierung und Edge-Case-Listen
- Doxygen-Kommentarentwuerfe
- CMake-Target-Erweiterungen
- Konsolidierung wiederkehrender Pattern in Modulcode

Nicht an Ollama delegieren:
- Sicherheitskritische Endbewertung
- Release-Go/No-Go Entscheidungen
- Governance-Policy-Aenderungen ohne Human-Review

## 6.2 Online-Werkzeuge (Erweiterung)

### GitHub Online

Use Cases:
- Issues/PRs als Quellen und Governance-Trigger
- Review-Threads als Wissensinput
- Milestones/Labels fuer Dokumentationsorchestrierung

Regeln:
- Jede kritische Wissensaenderung auf nachvollziehbare PR/Issue-Evidenz stuetzen
- Keine unverifizierten Behauptungen aus Chat allein

### GitHub Actions / CI

Use Cases:
- Automatische Lint- und Drift-Pruefung
- Link-Validierung
- Optional: Scheduled Knowledge-Lint

Implementierung in diesem Repository:
- `.github/workflows/maintenance-docs.yml`
- `scripts/ai-context-lint.py`
- Auto-Write-Back nur per `workflow_dispatch` mit `apply_ai_updates=true`
- Update-Modus:
	- `direct-commit` (schneller Sync)
	- `pull-request` (review-gate-konformer Sync mit automatischer PR-Erstellung)

Regeln:
- CI-Fehler in Wissensartefakten blockieren bei kritischen Claims
- Rein kosmetische Findings separat behandeln

### Cloud-Copilot / Cloud-LLMs

Use Cases:
- Security-Review
- Komplexe, moduluebergreifende Architekturentscheidungen
- Externe Aktualitaetspruefungen (z. B. CVE-relevante Kontexte)

Regeln:
- Ergebnisse in Compiled Wiki nur mit Quellenbezug persistieren

---

## 7. Rollenmodell (Human-in-the-loop)

### Role: Knowledge Maintainer
- Pflegt Schema-Regeln
- Priorisiert Konfliktaufloesung
- Gibt kritische Claims frei

### Role: AI Compiler (Ollama/Copilot)
- Kompiliert neue Quellen in Wiki-Seiten
- Aktualisiert Cross-Links, Index und Log
- Fuehrt Erstklassifikation von Konflikten durch

### Role: Reviewer
- Prueft Evidenzkette und SOT-Konformitaet
- Kontrolliert Drift-, Duplikat- und Widerspruchsrisiken

---

## 8. Umsetzungsrichtlinie (SOP) fuer ThemisDB

### 8.1 Phase 1 (1-2 Wochen)

1. Root-Dateien INDEX.md und LOG.md anlegen
2. Seitentypen finalisieren (Entity, Decision, Incident, Contract, Benchmark)
3. Pilotmodule bestimmen (empfohlen: graph, scheduler, rag)
4. Minimalen Lint-Job (Links + Orphans) lokal und CI-seitig definieren

### 8.2 Phase 2 (2-4 Wochen)

1. Ingest-Template und Query-Persist-Regel verpflichtend machen
2. Konfliktregister ai_context/KNOWLEDGE_CONFLICTS.md einfuehren
3. Nightly-Lint fuer Stale Claims und Widersprueche etablieren
4. Ollama-Routing fuer Inference-Tasks operationalisieren

### 8.3 Phase 3 (ab Woche 5)

1. KPI-Messung pro Sprint
2. Qualitaetsgates vor Release-Hardening
3. Kontinuierliche Schema-Nachschaerfung aus wiederkehrenden Fehlern

---

## 9. KPI- und Qualitaetsmetriken

Empfohlene Kennzahlen:
- Time-to-Context pro Modul-Onboarding
- Anteil beantwortbarer Fragen ohne Rohquellen-Recherche
- Anzahl erkannter/aufgeloester Wissenskonflikte pro Sprint
- Anteil veralteter Claims ueber Zeit
- Delta Build/Test-Failures durch bessere Wissensfuehrung

Qualitaetsziel:
- Die Wiki ist nicht nur Dokumentablage, sondern aktives AI-Betriebssystem fuer Entwicklungsentscheidungen.

---

## 10. Verbindliche Regeln fuer AI Vibe Coding

1. Vor jeder groesseren Codeaenderung: Query gegen Compiled Wiki + Moduldocs
2. Nach jeder relevanten Erkenntnis: Entscheidung/Ergebnis in Wiki persistieren
3. Bei Widerspruch zwischen Seiten: Konflikt markieren, nicht still ueberbuegeln
4. Bei fehlender Evidenz: Claim als unsicher kennzeichnen
5. Fuer C/C++ und CMake-Heavy-Arbeit: Ollama-Inference zuerst nutzen, danach Human-Review
6. Fuer Security/Governance/Release-Entscheidung: Cloud-Review + Human-Signoff verpflichtend
7. Branch-Governance strikt einhalten (develop/community/military etc. nach Regelwerk)
8. Keine Legacy- oder Stub-Pfade ohne explizite menschliche Freigabe

---

## 11. Risiken und Gegenmassnahmen

Risiko:
- Wiki-Drift durch inkonsistente Agentenupdates

Gegenmassnahme:
- Scheduled Lint + PR-Gate + klare Schema-Regeln

Risiko:
- Tool-Sprawl ohne klare Zustandsgrenzen

Gegenmassnahme:
- Layer-Verantwortung strikt trennen (Raw unveraenderlich, Compiled editierbar)

Risiko:
- Ueberdelegation an lokale Modelle bei sicherheitskritischen Themen

Gegenmassnahme:
- Harte Routingregel: Security/Audit/Go-NoGo niemals rein lokal final entscheiden

---

## 12. Entscheidungsvorlage fuer sofortige Aktivierung

Go-Live-Checkliste:
- INDEX.md vorhanden
- LOG.md vorhanden
- ai_context/KNOWLEDGE_LINT_REPORT.md vorhanden
- ai_context/KNOWLEDGE_CONFLICTS.md vorhanden
- Pilotmodule benannt
- Ollama-Routing-Regeln dokumentiert
- Lint-Minimum aktiv
- Reviewer-Rolle benannt

Wenn alle Punkte erfuellt sind, ist das LLM-Wiki-Modell fuer ThemisDB operativ einsetzbar.
