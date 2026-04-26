## Ziel
Der docs-Root ist weiterhin deutlich überladen und muss weiter systematisch entlastet werden, ohne Navigation, wichtige Einstiege oder aktive Querverweise zu brechen.

Dieses Issue ist so formuliert, dass ein GitHub-AI-Agent (Copilot/Coding Agent) die Arbeiten autonom in einer Night-Run-Session ausführen kann.

## Ausgangslage
- Root-Entlastung wurde bereits in mehreren Wellen gestartet.
- Historien-Bereiche existieren bereits (z. B. implementation-history, archive, governance/documentation-history, ci-cd/branching-release-history).
- Trotzdem liegen weiterhin sehr viele thematische Spezialdokumente im docs-Root.

## Definition of Done
- Der docs-Root enthält nur noch:
  - Einstiegs-/Navigationsdokumente
  - Governance-/Policy-Dokumente
  - wenige, klar begründete Top-Level-Hubs mit aktiver Querverlinkung
- Thematische Spezialdokumente sind in passende Unterordner verschoben.
- Alle geänderten lokalen Links sind valide.
- Keine regressiven Broken Links in den zentralen Hubs.
- Root-Dateianzahl wurde messbar reduziert.
- Änderungen sind in kleinen, nachvollziehbaren Commits gruppiert (pro Themenwelle).

## Nicht-Ziele
- Keine inhaltliche Neuschreibung großer Fachdokumente.
- Keine großflächigen redaktionellen Umschreibungen.
- Keine Änderung von Architektur-/Produktentscheidungen.
- Keine Löschung fachlich relevanter Inhalte (nur Umzug + Linkanpassung).

## Guardrails (verbindlich)
1. Keine Destruktivbefehle (`git reset --hard`, `git checkout --`, etc.).
2. Keine Reverts von fremden/unrelated Änderungen.
3. Moves nur dann, wenn Zielordner semantisch passt.
4. Vor jedem Move: Referenzanalyse im docs-Baum.
5. Nach jedem Move-Block: Link-Validierung.
6. Wenn Dateisystem-Zeitstempel unbrauchbar sind: Git-Historie für Datums-Triage (`git log -1 --format=%cs -- <file>`).

## Priorisierte Arbeitswellen

### Welle 1: Branching/Merge/Release-Dokumentfamilie abschließen
- Prüfen, ob alle umgezogenen Dateien konsistent unter `docs/ci-cd/branching-release-history/` referenziert sind.
- Offene Alt-Links außerhalb Zielordner beseitigen.
- `BRANCHING_DOCS_INDEX.md` vollständig auf neue Pfade normalisieren.
- Intra-folder Links im Zielordner validieren.

Akzeptanzkriterien:
- Keine Alt-Links auf ehem. Root-Pfade dieser Familie außerhalb Zielordners.
- Linkcheck für:
  - docs/BRANCHING_DOCS_INDEX.md
  - docs/ci-cd/branching-release-history/README.md
  - docs/ci-cd/branching-release-history/*.md (sofern geändert)

### Welle 2: Performance/Optimization-Familie aus dem Root auslagern
Kandidaten (Beispiele, final per Referenz-Triage):
- BUILD_/PERFORMANCE_/CACHE_/MMAP_/VECTOR_/QUANTIZATION_/WORKLOAD_/WRITE_AMPLIFICATION_...

Vorgehen:
- Dokumente nach `docs/performance/` oder `docs/build-guide/` verschieben (je nach Inhalt).
- Root-Links in Hubs/Indexen anpassen.

Akzeptanzkriterien:
- Für verschobene Dateien keine alten Root-Links mehr im docs-Baum.
- `CATEGORY_INDEX.md` enthält valide Einstiege für Performance/Build.

### Welle 3: Prompt/LLM-Strategie- und Experimentdokumente bündeln
Kandidaten:
- PROMPT_*, RAG_*, ggf. LLM-Analyse-/Framework-Dokus aus Root

Vorgehen:
- Nach `docs/llm/`, `docs/llm_orchestration/` oder `docs/research/` verschieben (inhaltsbasiert).

Akzeptanzkriterien:
- Root enthält keine isolierten Prompt-/Experiment-Snapshots ohne Hub-Bezug.
- Einstiegspfade in README/HUB/CATEGORY sind konsistent.

### Welle 4: Dokumentationsprozess-Snapshots final bereinigen
- Bereits gestartete Verschiebungen nach `docs/governance/documentation-history/` vervollständigen.
- Verbleibende `DOCUMENTATION_*` Root-Dateien mit 0-Referenzen umziehen.

Akzeptanzkriterien:
- Root enthält nur aktive Dokumentationsprozessseiten, keine abgeschlossenen Snapshots ohne Referenzen.

## Navigationsdateien (müssen nach jeder Welle intakt bleiben)
- docs/README.md
- docs/00_DOCUMENTATION_INDEX.md
- docs/DOCUMENTATION_HUB.md
- docs/CATEGORY_INDEX.md
- docs/DOCS_ORGANIZATION_PLAN.md
- docs/governance/DOCS_PR_POLICY.md

## Triage-Regel pro Datei
Eine Root-Datei ist Move-Kandidat, wenn mindestens 2/3 zutreffen:
1. Kein dauerhafter Einstieg (kein Hub/Index/Policy)
2. 0 oder sehr geringe docs-interne Referenzen
3. Historischer Snapshot-Charakter (Complete/Status/Summary/Report) ODER altes Git-Datum

Ausnahme:
- Aktive operative Dokumente dürfen trotz weniger Referenzen im Root bleiben, wenn sie klar als Top-Level-Hub dienen.

## Technische Ausführung (empfohlen)

### 1) Baseline erfassen
- Anzahl Root-Markdown-Dateien
- Kandidatenliste je Themenfamilie
- Referenzanzahl je Kandidat

### 2) Move in kleinen Batches
- Max. 8-20 Dateien pro Batch
- Nach Batch sofort Linkfixes

### 3) Validierung je Batch
- Alt-Link-Scan für verschobene Dateinamen
- Zielgerichteter Linkcheck auf geänderte Dateien
- Kurzer docs-Status-Report

## Qualitätsgates
- Gate A: Keine Broken Links in den geänderten Dateien
- Gate B: Keine Alt-Links auf verschobene Root-Dateien
- Gate C: Navigationsdateien bleiben funktional
- Gate D: Root-Dateianzahl sinkt gegenüber Baseline

## Reporting im PR
Der Agent-PR muss enthalten:
1. Vorher/Nachher Root-Dateianzahl
2. Liste aller verschobenen Dateien mit Zielordner
3. Liste aller angepassten Linkdateien
4. Validierungsausgaben (Alt-Link-Scan + Linkchecks)
5. Offene Restkandidaten (falls bewusst verschoben auf Folgewelle)

## Optional (wenn Zeit)
- Kleine „Root Hygiene“ CI-Prüfung ergänzen, die warnt, wenn neue Snapshot-Dateien im Root landen.
- Klarer Hinweis in der PR-Template-Dokumentation auf Root-vs-History-Regel.

## Risikohinweise
- Hohe Patch-Volumina in langen Markdown-Dateien können fehlschlagen -> lieber kleinere Patches.
- Bei wiederholten Linkmustern sorgfältig auf relative Pfade achten.
- Nicht in `wiki_out/` oder externen Spiegeln gegenprüfen; nur im primären `docs/`-Baum validieren.

## Labels (Vorschlag)
- documentation
- cleanup
- refactor
- automation

## Priorität
Hoch (Strukturqualität der Projektdokumentation + bessere AI/Contributor-Navigation)
