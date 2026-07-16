# PR-Issue-Linkage (Stand 2026-05-25)

## Ergebnis

Die 8 spezifischen Umsetzungs-Issues aus `ISSUE_SET_MISSING_IMPLEMENTIERUNGEN_2026-05-25.md` wurden als
GitHub-Issues angelegt (#5340..#5347), mit Commit-Referenzen dokumentiert und anschliessend geschlossen,
weil die Umsetzung bereits auf `develop` enthalten ist.

Status:
- #5340..#5347: CLOSED (umgesetzt, Commit-Referenz hinterlegt)
- #5332..#5339: CLOSED als Duplikate (verweisen auf #5340..#5347)

Hinweis:
- Fuer die drei relevanten Commits wurde keine dedizierte PR gefunden (direkter Commit-Flow auf `develop`).

## Gepruefte Kandidaten

### Offene Issues (als `Refs` geeignet)
- #5172 `[PHASE 1-5] Gap Scanner Analysis` (canonical planning tracker)
- #5231 `[PHASE 1-5] Gap Scanner Analysis` (historical alternate wave)
- #5245 `[P0-CRITICAL] LLM Module`
- #5246 `[P0-CRITICAL] SERVER Module`
- #5248 `[P0-CRITICAL] SHARDING Module`
- #5250 `[P0-CRITICAL] STORAGE Module`
- #5234 `[RELIABILITY] ...`
- #5235 `[CONCURRENCY] ...`

### Geschlossene Historien-Issues (nicht fuer `Closes` nutzen)
- #3954 `Distributed Transaction Coordinator (2PC)`
- #1244 `Implement 2-Phase Commit (2PC) Coordinator ...`
- #393 `[RPC-P3] Implement Transaction Support ...`
- #2326 `[transaction] Distributed SAGA orchestration ...`

## Zuordnung pro Commit/PR

### Commit `bb613c9e78`
Abdeckung:
- ISSUE-01, ISSUE-02, ISSUE-03, ISSUE-04, ISSUE-05, ISSUE-06, ISSUE-07, ISSUE-08

Empfohlene PR-Verknuepfung:
- `Closes #5340`
- `Closes #5341`
- `Closes #5342`
- `Closes #5343`
- `Closes #5344`
- `Closes #5345`
- `Closes #5346`
- `Closes #5347`
- `Refs #5172`
- `Refs #5245`
- `Refs #5246`
- `Refs #5248`
- `Refs #5250`

### Commit `58cd5e598d`
Abdeckung:
- ISSUE-01 (2PC Batch/Concurrency Hardening)

Empfohlene PR-Verknuepfung:
- `Closes #5340` (nur falls noch offen)
- `Refs #5234`
- `Refs #5235`
- `Refs #5246`

### Commit `add4781d0f`
Abdeckung:
- ISSUE-08 (Wire-Startup-Fail-Closed Testangleichung)

Empfohlene PR-Verknuepfung:
- `Closes #5347` (nur falls noch offen)
- `Refs #5246`

## Vorschlag fuer neue 8 Issues

Quelle:
- `ai_working/ISSUE_SET_MISSING_IMPLEMENTIERUNGEN_2026-05-25.md`
- `ai_working/ISSUE_SET_MISSING_IMPLEMENTIERUNGEN_2026-05-25.json`

Neue Issues sollten 1:1 mit ISSUE-01..ISSUE-08 angelegt werden, damit die PRs sauber und ohne Mehrdeutigkeit verknuepft werden koennen.

## Copy/Paste-Block fuer PR-Body (Hauptpaket)

```text
## Linked Issues
Closes #5340
Closes #5341
Closes #5342
Closes #5343
Closes #5344
Closes #5345
Closes #5346
Closes #5347

Refs #5172
Refs #5245
Refs #5246
Refs #5248
Refs #5250
```

## Copy/Paste-Block fuer PR-Body (2PC Follow-up)

```text
## Linked Issues
Closes #5340

Refs #5234
Refs #5235
Refs #5246
```

## Copy/Paste-Block fuer PR-Body (Wire Tests)

```text
## Linked Issues
Closes #5347

Refs #5246
```

## Schneller Anlegepfad (gh CLI)

Die folgenden Kommandos sind Vorlagen. Titel/Body aus
`ISSUE_SET_MISSING_IMPLEMENTIERUNGEN_2026-05-25.md` uebernehmen.
Falls ein Label lokal nicht existiert, Label-Argument entfernen oder Label vorher anlegen.

```powershell
gh issue create --title "[DTX] 2PC Phase-2 fuer Remote-Teilnehmer fail-closed verdrahten" --label "area:transaction" --label "priority:P0"
gh issue create --title "[Wire/Protobuf] AQL/Cursor/Geo/TS/Graph produktiv anbinden statt 501/503" --label "area:themis" --label "area:network" --label "priority:P0"
gh issue create --title "[Wire/JSON] Graph/AQL/Geo auf Startup-validierte Pflichtabhaengigkeiten umstellen" --label "area:network" --label "priority:P0"
gh issue create --title "[Backup/Cloud] S3/Azure/GCS Provider von Placeholder auf Produktionspfade umstellen" --label "area:sharding" --label "area:storage" --label "priority:P1"
gh issue create --title "[PITR] WAL-Replay als verpflichtenden Standardpfad integrieren" --label "area:storage" --label "priority:P1"
gh issue create --title "[LLM/Distributed] AllReduce/Broadcast bei world_size>1 verpflichtend verdrahten" --label "area:llm" --label "priority:P1"
gh issue create --title "[Sharding Transport] LZ4-Kompressionspfad vollstaendig implementieren" --label "area:sharding" --label "priority:P2"
gh issue create --title "[Wire Bootstrapping] Zentrale Verdrahtung der Bridge-Setter mit Startvalidierung" --label "area:network" --label "area:themis" --label "priority:P2"
```
