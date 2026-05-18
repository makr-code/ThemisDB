# Git-ähnliche Features für ThemisDB MVCC

**Projekt:** ThemisDB
**Kategorie:** Research Documentation
**Status:** Review-ready (überarbeitet)
**Datum:** 2026-05-18
**Version:** 1.1

---

## Abstract / Zusammenfassung

Dieses Review prüft die Aussage, welche „Git-ähnlichen“ Funktionen im aktuellen ThemisDB-MVCC-Stack bereits vorhanden sind und welche weiterhin fehlen. Die zentrale Korrektur gegenüber dem vorherigen Dokument lautet: **Named Snapshots, strukturierte Diffs, persistente Branches und Drei-Wege-Merge sind im Open-Source-Repository bereits konkret implementiert** – inklusive Tests, HTTP-API-Handlern und eigenen Benchmarks.

Gleichzeitig zeigt der Faktencheck klare Grenzen der Git-Analogie:

- ThemisDB nutzt MVCC/ACID-Transaktionen und Changefeeds, nicht ein content-addressed Commit-DAG wie Git.
- Zeitreisen und Restore-Funktionen sind **teilweise** vorhanden, aber nicht als globales `git checkout` über alle Datenmodelle nachweisbar.
- Explizite APIs für **Cherry-Pick**, **Interactive Rebase** oder generelles Umschreiben der Datenhistorie wurden im überprüften OSS-Baum nicht gefunden.
- Für Snapshot-, Branch- und Diff-Komponenten existieren Benchmarks, aber dieses Dokument belegt **keine gemessenen Laufzeitresultate**, sondern nur die vorhandene Benchmark-Abdeckung.

Das Ergebnis ist daher kein Produktmarketing, sondern eine evidenzbasierte Einordnung des aktuellen Stands: ThemisDB besitzt mehrere Git-inspirierte Bedienkonzepte für MVCC, aber nicht jede Git-Metapher ist technisch oder betrieblich sinnvoll 1:1 auf eine Datenbank übertragbar.

---

## Introduction / Einleitung

### Problemstellung

Git und MVCC teilen eine gemeinsame Grundidee: mehrere konsistente Zustände eines Systems müssen parallel existieren können, ohne dass Leser und Schreiber sich unnötig blockieren. Daraus entsteht regelmäßig die Frage, welche Git-Konzepte – Snapshots, Tags, Branches, Diffs, Merges oder Rollback-Punkte – für ThemisDB bereits nutzbar sind und welche Funktionen noch fehlen.

### Ziel dieses Reviews

1. Die Aussagen des bisherigen Dokuments gegen den **aktuellen** Repository-Stand prüfen.
2. Begriffe vereinheitlichen (AQL, Multi-Model, MVCC/Snapshot-Isolation, Branch/Snapshot/Diff/Merge).
3. Nur solche Claims stehen lassen, die durch **Code, Tests, Benchmarks oder API-Artefakte** nachvollziehbar belegt sind.
4. Eine review-fähige Argumentationskette liefern: Problem → Ansatz → Evidenz → Grenzen → Fazit.

### Terminologie (vereinheitlicht)

- **AQL** = *Advanced Query Language* laut `README.md` und Architektur-Dokumentation.
- **Multi-Model** = relational + graph + vector + document; im Repository zusätzlich mit geospatial- und time-series-Bezügen dokumentiert (`README.md`, `ARCHITECTURE.md`).
- **Konsistenzmodell** = ACID-Transaktionen mit MVCC/Snapshot-Isolation; im `transaction`-Modul zusätzlich SSI-/Deadlock-/Timeout-Mechanismen (`README.md`, `include/transaction/transaction_manager.h`, `src/transaction/transaction_manager.cpp`).
- **Git-ähnlich** bedeutet in diesem Review **funktionsähnlich**, nicht implementierungsgleich: ThemisDB modelliert Zustände primär über Sequenznummern, Tags, Changefeed-Ereignisse und Branch-Metadaten – nicht über einen Git-Objektspeicher mit SHA-basiertem Commit-DAG.

---

## Methodik / Ansatz

### 1) Artefaktbasierter Faktencheck

Primäre Evidenzquellen für dieses Review:

- Implementierungen im `transaction`-, `cdc`-, `analytics`-, `server`- und `index`-/`temporal`-Umfeld
- Fokus-Tests für Snapshot-, Branch-, Merge-, Diff-, Changefeed-, Audit- und Temporal-Funktionalität
- Benchmark-Dateien für MVCC-, Snapshot-, Branch-, Diff- und Transaktionspfade
- Architektur- und Modul-Dokumentation (`README.md`, `ARCHITECTURE.md`, `src/transaction/README.md`)

### 2) Claim-Klassifizierung

Jeder zentrale Claim wurde in eine der folgenden Klassen eingeordnet:

- **Bestätigt:** direkt durch Code + Tests +/oder API/Benchmark-Artefakte belegbar
- **Teilweise bestätigt:** Kernbausteine vorhanden, aber Reichweite oder Betriebswirkung enger als im bisherigen Text behauptet
- **Nicht bestätigt:** im überprüften OSS-Baum nicht belastbar nachweisbar

### 3) Redaktionsprinzip

- Überholte Aussagen aus dem Alttext werden entfernt oder korrigiert.
- Prozentwerte ohne nachvollziehbare Messbasis werden nicht mehr verwendet.
- Leistungsversprechen werden nur dann genannt, wenn konkrete Benchmark-Artefakte existieren; **ohne gemessene Resultate** werden keine Laufzeitzahlen als Tatsache behauptet.

---

## Evaluation / Experimente

## A) Verifizierte Implementierungsbefunde (Problem → Ansatz → Evidenz)

| Problem / Git-Metapher | Ansatz in ThemisDB | Status | Evidenz |
|---|---|---|---|
| Konsistente Transaktions-Snapshots | MVCC-Transaktionen mit `beginTransaction()`, `commitTransaction()`, `rollbackTransaction()`, Lock-/Timeout-/SSI-Hooks | **Bestätigt** | `include/transaction/transaction_manager.h`, `src/transaction/transaction_manager.cpp`, `src/transaction/README.md`, `benchmarks/bench_transaction_throughput.cpp`, `benchmarks/bench_mvcc.cpp` |
| Änderungsverlauf ähnlich `git log` | Changefeed mit monotonen Sequenznummern, Filtern, Long-Polling, Subscriptions, Retention und Redaction | **Bestätigt** | `include/cdc/changefeed.h`, `src/cdc/changefeed.cpp`, `tests/test_cdc_changefeed_core.cpp`, `tests/test_http_changefeed_sse.cpp` |
| Benannte Tags / Snapshots | `SnapshotManager` für persistente Tags inkl. CRUD, Statistik, Retention und Restore-Metadaten | **Bestätigt** | `include/transaction/snapshot_manager.h`, `src/transaction/snapshot_manager.cpp`, `tests/test_snapshot_manager.cpp`, `src/server/snapshot_api_handler.cpp`, `benchmarks/bench_snapshot_manager.cpp` |
| Strukturierter Diff zwischen Zuständen | `analytics::DiffEngine` für Sequenz-, Zeitstempel- und Tag-basierte Diffs mit Filtern, Pagination und Cache | **Bestätigt** | `include/analytics/diff_engine.h`, `src/analytics/diff_engine.cpp`, `tests/test_diff_engine.cpp`, `src/server/diff_api_handler.cpp`, `benchmarks/bench_diff_engine.cpp` |
| Persistente Branches | `BranchManager` mit Branch-Erzeugung aus Tag/Sequenz/Zeitstempel, aktivem Branch-Kontext, Persistenz und Historie | **Bestätigt** | `include/transaction/branch_manager.h`, `src/transaction/branch_manager.cpp`, `tests/test_branch_manager.cpp`, `src/server/branch_api_handler.cpp`, `benchmarks/bench_branch_manager.cpp` |
| Drei-Wege-Merge | `MergeEngine` mit Konflikttypen, Strategien (`OURS`, `THEIRS`, `MANUAL`, `FAST_FORWARD`) und Dry-Run | **Bestätigt** | `include/transaction/merge_engine.h`, `src/transaction/merge_engine.cpp`, `tests/test_merge_engine.cpp`, `src/server/merge_api_handler.cpp` |
| Konfliktvorschau und manuelle Auflösung | Branch-Merge-Preview und manuelle Konfliktauflösung über Branch-/Merge-Tests | **Bestätigt** | `tests/test_branch_conflict_resolution.cpp`, `src/server/branch_api_handler.cpp` |
| Time-Travel / historische Sicht | Zeitabhängige Graph-Abfragen über `TemporalFilter` und `bfsAtTime(...)`/temporal graph operations | **Teilweise bestätigt** | `tests/test_temporal_graph.cpp`, `include/temporal/README.md` |
| Audit Trail ähnlich „wer hat was geändert?“ | Audit-Logging mit strukturierten Security-/Data-Events, Hash-Chain und API-Unterstützung | **Bestätigt** | `include/utils/audit_logger.h`, `src/utils/audit_logger.cpp`, `tests/test_audit_logger.cpp`, `src/server/audit_api_handler.cpp` |
| Point-in-time restore | `restoreToTag()` liefert Zielsequenz und erzeugt Audit-Restore-Tag | **Teilweise bestätigt** | `include/transaction/snapshot_manager.h`, `tests/test_snapshot_manager.cpp` |
| Cherry-Pick einzelner Änderungen | Keine explizite Cherry-Pick-API im überprüften OSS-Baum gefunden | **Nicht bestätigt** | Negativsuche in `include/`, `src/`, `tests/`; keine dedizierte Komponente oder API gefunden |
| Interactive Rebase / Umschreiben der Historie | Keine explizite Rebase-/History-Rewrite-Funktion im überprüften OSS-Baum gefunden | **Nicht bestätigt** | Negativsuche in `include/`, `src/`, `tests/`; kein dedizierter MVCC-Rewrite-Pfad belegt |

## B) Gegenprüfung zentraler Alt-Claims

| Alt-Claim aus der bisherigen Fassung | Ergebnis | Begründung |
|---|---|---|
| „Named Snapshots / Semantic Tagging fehlen“ | **Falsch** | `SnapshotManager` ist vorhanden, persistent und getestet; zusätzlich existieren Snapshot-HTTP-Endpunkte und Benchmarks. |
| „Diff API fehlt“ | **Falsch** | `DiffEngine` und `DiffApiHandler` implementieren Sequenz-, Zeitstempel- und Tag-basierte Diffs. |
| „Persistent Branches fehlen“ | **Falsch** | `BranchManager` implementiert persistente Branch-Metadaten, Branch-Wechsel, Branch-Statistiken und Merge-Einstiegspunkte. |
| „Three-Way Merge fehlt“ | **Falsch** | `MergeEngine` und Branch-Conflict-Tests belegen Drei-Wege-Merge, Dry-Run und Konfliktstrategien. |
| „ThemisDB hat nur numerische Versionen, aber keine benannten Marker“ | **Falsch** | Snapshot-Tags werden mit Namen, Beschreibung, Benutzer und Sequenz persistiert. |
| „Git-Zeitreise entspricht vollständig dem Datenbankzustand in ThemisDB“ | **Zu stark formuliert** | Nachweisbar sind zeitbezogene Graph-/Temporal-APIs und tag-/sequenzbezogene Restore-Hilfen, aber kein globales `checkout`-Semantikmodell über alle Datenmodelle. |
| „Cherry-Pick/Rebase sind die nächsten offensichtlichen Features“ | **Nicht belegt** | Im Repository gibt es dafür keine konkrete Implementierung; der Mehrwert gegenüber Savepoints, Diffs und gezielten Merges bleibt im OSS-Baum unbelegt. |

## C) Bewertete Reichweite der Git-Analogie

### 1. Stark belegte Analogien

- **Snapshot / Tag:** Named tags auf Sequenzständen sind im Repository explizit als „Git-like named snapshots“ dokumentiert.
- **Diff:** Änderungsmengen zwischen Zuständen können strukturiert berechnet und per HTTP abgefragt werden.
- **Branch:** Branches sind persistente Metadatenobjekte und nicht nur lose Ideen im Roadmap-Text.
- **Merge:** Drei-Wege-Merge samt Konfliktstrategien ist im Code- und Testbestand direkt belegt.

### 2. Nur teilweise tragfähige Analogien

- **Time travel:** Vorhanden, aber domänenspezifisch; die nachgewiesenen APIs betreffen insbesondere temporale Graph-/Zeitfilter-Szenarien.
- **Restore:** Vorhanden als Wiederherstellung auf eine Zielsequenz samt Audit-Tag, nicht als vollständige physische Rücksetzung aller Datenstrukturen im Stil eines Git-Checkout.
- **Audit ≈ blame:** Sinnvoll als Herkunfts-/Compliance-Metapher, aber nicht gleichbedeutend mit Git-Blame auf Zeilenebene.

### 3. Schwache oder unbelegte Analogien

- **Cherry-Pick:** keine explizite Transplantation einzelner Commits/Änderungssets gefunden
- **Interactive Rebase / Force Push:** keine History-Rewrite-Mechanik für MVCC nachgewiesen
- **Content-addressed storage:** Das Transaktions-/MVCC-Modell basiert auf RocksDB + Sequenzen; ein Git-artiger Objektgraph mit inhaltsadressierten Commits ist nicht belegt

## D) Benchmark- und Testevidenz

### Tests

Die Git-ähnlichen MVCC-Bausteine sind durch fokussierte Tests abgedeckt, u. a.:

- `tests/test_snapshot_manager.cpp`
- `tests/test_branch_manager.cpp`
- `tests/test_branch_conflict_resolution.cpp`
- `tests/test_merge_engine.cpp`
- `tests/test_diff_engine.cpp`
- `tests/test_cdc_changefeed_core.cpp`
- `tests/test_audit_logger.cpp`
- `tests/test_temporal_graph.cpp`

### Benchmarks

Für zentrale Bausteine existieren eigene Microbenchmarks:

- `benchmarks/bench_snapshot_manager.cpp`
- `benchmarks/bench_branch_manager.cpp`
- `benchmarks/bench_diff_engine.cpp`
- `benchmarks/bench_mvcc.cpp`
- `benchmarks/bench_transaction_throughput.cpp`

Wichtig: Diese Review-Fassung belegt **die Existenz** dieser Messartefakte, aber nicht deren konkrete Resultate. Der Artikel macht daher bewusst **keine numerischen Performance-Claims** für Snapshot-, Branch-, Diff- oder Merge-Pfade.

### Reproduzierbare Dokumentenprüfung

Für diese Datei wurden die vorhandenen Dokumenten-Checks verwendet:

```bash
python3 scripts/docs-lint.py research/GIT_LIKE_FEATURES_FOR_MVCC.md
python3 scripts/link-check.py research/GIT_LIKE_FEATURES_FOR_MVCC.md
```

---

## Limitations / Known Issues

1. **Die Git-Analogie ist funktional, nicht strukturell**
   ThemisDB arbeitet mit MVCC, RocksDB, Changefeed-Sequenzen und branch-/snapshot-bezogenen Metadaten. Das ist nicht identisch zu Git-Objekten, Commit-DAG und SHA-basiertem Objektmodell.

2. **Time-Travel ist nicht als globales Datenbank-Checkout belegt**
   Nachgewiesen sind temporale Abfragen und graphbezogene Zeitfilter. Ein universelles „setze die gesamte Datenbank auf Tag X“ im Git-Sinn ist im überprüften OSS-Baum nicht als End-to-End-Funktion belegt.

3. **`restoreToTag()` ist eher ein koordinierter Wiederanlaufpunkt als ein vollständiges Physical Restore**
   Der aktuelle Pfad validiert einen Tag, liefert dessen Sequenz und erzeugt einen auditierbaren Restore-Tag. Das ist nützlich, aber enger als ein vollständiges Recovery-/Rollback-System für alle Speicherstrukturen.

4. **Merge-Konfliktqualität hängt von der rekonstruierten Historie ab**
   Die Branch-Conflict-Tests dokumentieren selbst, dass bestimmte Umgebungen bei unvollständiger historischer Rekonstruktion in vereinfachte Fast-Forward-/No-Conflict-Pfade degradieren können.

5. **Cherry-Pick und History-Rewrite bleiben offen**
   Für Cherry-Pick, Rebase oder Force-Push-ähnliche Semantik gibt es in den geprüften Artefakten keine belastbare Implementierung. Ob diese Funktionen überhaupt wünschenswert sind, ist daher eine Architekturfrage – kein belegter Lieferstand.

6. **Benchmark-Abdeckung ist vorhanden, aber Resultatlage in diesem Dokument bewusst zurückhaltend**
   Ohne eingefrorene Resultate oder reproduzierte Läufe in diesem Review sollten keine Laufzeitversprechen aus Kommentarzeilen oder Benchmark-Namen abgeleitet werden.

---

## Schlussfolgerung / Fazit

**Kurzfazit:** ThemisDB besitzt im aktuellen Repository deutlich mehr Git-ähnliche MVCC-Funktionalität, als die bisherige Fassung des Artikels behauptet hat. Tags/Snapshots, Diffs, persistente Branches und Drei-Wege-Merge sind nicht nur Ideen, sondern in Code, Tests, API-Handlern und Benchmarks konkret nachweisbar.

**Gleichzeitig bleibt die sauberste Formulierung:**

- ThemisDB ist **keine Git-Datenbank**.
- Es stellt aber mehrere **Git-inspirierte Bedien- und Vergleichsmodelle** für MVCC bereit.
- Die stärksten Parallelen liegen bei Snapshots, Diffs, Branches und Merge.
- Die schwächsten oder fehlenden Parallelen liegen bei globalem Checkout, Cherry-Pick und History-Rewrite.

**Empfohlene nächste Schritte für Folgearbeit:**

1. Falls gewünscht, Cherry-Pick/Rebase nicht als Marketing-Claim, sondern als eigenständige Architekturentscheidung mit klaren Sicherheits- und Betriebsgrenzen evaluieren.
2. Für Snapshot-/Branch-/Diff-/Merge-Komponenten reproduzierbare Benchmark-Resultate an Research- oder Baseline-Artefakte koppeln.
3. Die Produkt- und API-Dokumentation stärker darauf ausrichten, dass Branches und Tags sequenz-/changefeed-basiert sind und nicht als vollständige Git-Repos missverstanden werden.

---

## References / Quellen

## Interne ThemisDB-Artefakte

1. Produkt- und Architekturkontext: `README.md`, `ARCHITECTURE.md`
2. Transaktionsmodul: `src/transaction/README.md`, `include/transaction/transaction_manager.h`, `src/transaction/transaction_manager.cpp`
3. Changefeed / CDC: `include/cdc/changefeed.h`, `src/cdc/changefeed.cpp`
4. Named snapshots / restore: `include/transaction/snapshot_manager.h`, `src/transaction/snapshot_manager.cpp`, `src/server/snapshot_api_handler.cpp`
5. Structured diff: `include/analytics/diff_engine.h`, `src/analytics/diff_engine.cpp`, `src/server/diff_api_handler.cpp`
6. Branching: `include/transaction/branch_manager.h`, `src/transaction/branch_manager.cpp`, `src/server/branch_api_handler.cpp`
7. Merge: `include/transaction/merge_engine.h`, `src/transaction/merge_engine.cpp`, `src/server/merge_api_handler.cpp`
8. Temporal/time-travel evidence: `tests/test_temporal_graph.cpp`, `include/temporal/README.md`
9. Audit logging: `include/utils/audit_logger.h`, `src/utils/audit_logger.cpp`, `src/server/audit_api_handler.cpp`
10. Test evidence: `tests/test_snapshot_manager.cpp`, `tests/test_branch_manager.cpp`, `tests/test_branch_conflict_resolution.cpp`, `tests/test_merge_engine.cpp`, `tests/test_diff_engine.cpp`, `tests/test_cdc_changefeed_core.cpp`, `tests/test_audit_logger.cpp`
11. Benchmark evidence: `benchmarks/bench_snapshot_manager.cpp`, `benchmarks/bench_branch_manager.cpp`, `benchmarks/bench_diff_engine.cpp`, `benchmarks/bench_mvcc.cpp`, `benchmarks/bench_transaction_throughput.cpp`

## Externe Literatur und Systeme (validierbare URL/DOI)

1. Chacon, S. & Straub, B. *Pro Git* — Git branching basics.
   URL: [https://git-scm.com/book/en/v2/Git-Branching-Branches-in-a-Nutshell](https://git-scm.com/book/en/v2/Git-Branching-Branches-in-a-Nutshell)
2. Chacon, S. & Straub, B. *Pro Git* — Git tagging.
   URL: [https://git-scm.com/book/en/v2/Git-Basics-Tagging](https://git-scm.com/book/en/v2/Git-Basics-Tagging)
3. Chacon, S. & Straub, B. *Pro Git* — Advanced merging.
   URL: [https://git-scm.com/book/en/v2/Git-Tools-Advanced-Merging](https://git-scm.com/book/en/v2/Git-Tools-Advanced-Merging)
4. RocksDB Wiki — Transactions / TransactionDB.
   URL: [https://github.com/facebook/rocksdb/wiki/Transactions](https://github.com/facebook/rocksdb/wiki/Transactions)
5. Ports, D. R. K. & Grittner, K. (2012). *Serializable Snapshot Isolation in PostgreSQL.*
   URL: [https://drkp.net/papers/ssi-vldb12.pdf](https://drkp.net/papers/ssi-vldb12.pdf)
6. Snodgrass, R. T. (1999). *Developing Time-Oriented Database Applications in SQL.*
   URL: [https://www2.cs.arizona.edu/people/rts/tdbbook.pdf](https://www2.cs.arizona.edu/people/rts/tdbbook.pdf)

---

## Changelog

| Datum | Version | Änderungen |
|---|---|---|
| 2026-05-18 | 1.1 | Vollständige Research-Review-Überarbeitung: Pflichtstruktur ergänzt, Alt-Claims gegen aktuellen Code/Test/Benchmark-Stand neu bewertet, Terminologie vereinheitlicht, kaputte interne Links entfernt, Quellenliste neu aufgebaut |
| 2026-01-11 | 1.0 | Initiale Fassung |
