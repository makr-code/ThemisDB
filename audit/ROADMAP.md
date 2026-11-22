# ThemisDB – Production-Readiness-Roadmap (audit-abgeleitet)

**Author:** ThemisDB Team
**Created:** 2026-10-31
**Last Updated:** 2026-11-21
**Status:** review
**AI_Used:** true
**AI_Models:** unknown
**AI_Contributor:** GitHub Copilot
**Human_Review:** false
**Human_Author:** ThemisDB Team
**Quelle:** [FULL_DEEP_DIVE_AUDIT_2026-10-31.md](FULL_DEEP_DIVE_AUDIT_2026-10-31.md) (Findings-IDs in dieser Roadmap verweisen dorthin)
**Related Milestone:** DOC-WEEKLY-2026-W46 (source follow-up; evidence and module acceptance gates remain open)
**Zielbranch:** `develop` (Editions-Branches `community`, `enterprise`, `hyperscaler`, `military`, `minimal` erhalten Änderungen über den regulären Promotion-Flow gemäß [../BRANCHING_STRATEGY.md](../BRANCHING_STRATEGY.md))

> **AI-Provenienz:** AI-assisted generation; model attribution is approximate and not guaranteed.
> Exact model backend not verifiable from final document text alone. Menschliche Prüfung und Priorisierung stehen aus.
>
> **Normativer Rang:** Diese Roadmap ist ein **audit-abgeleiteter Umsetzungsplan**. Verbindlich für Release-/GA-Status
> bleiben [../ROADMAP.md](../ROADMAP.md) („Canonical Status Authority“), [../RELEASE_STRATEGY.md](../RELEASE_STRATEGY.md),
> [../VERSIONING.md](../VERSIONING.md) und der Human-Sign-off nach
> [../docs/governance/GA_PROMOTION_SIGN_OFF.md](../docs/governance/GA_PROMOTION_SIGN_OFF.md). Übernommene Items sind
> in die Root-Roadmap bzw. die Modul-`ROADMAP.md` zu spiegeln; diese Datei verfolgt nur die Audit-Traceability.

---

## Inhalt

- [Current Status](#current-status)
- [Definition „Production-Ready“](#definition-production-ready)
- [Meilensteine](#meilensteine)
- [In Progress](#in-progress)
- [Planned Features](#planned-features) (Workstreams WS1–WS11)
- [Implementation Phases](#implementation-phases)
- [Production Readiness Checklist](#production-readiness-checklist)
- [Known Issues & Limitations](#known-issues--limitations)
- [Breaking Changes](#breaking-changes)
- [Traceability-Matrix](#traceability-matrix)
- [Pflege dieser Roadmap](#pflege-dieser-roadmap)

---

## Current Status

| Aspekt | Stand 2026-10-31 |
|---|---|
| Version / Release-Typ | `2.4.0` / `alpha` (`VERSION`, `RELEASE_TYPE`) |
| Audit-Gesamturteil | **Nicht produktionsreif**, ungewichteter Mittelwert ≈ 2,4 / 5 über 8 Dimensionen |
| Findings (kalibriert) | 🔴 2 Critical · 🟠 22 High · 🟡 40 Medium · ⚪ 15 Low/Info |
| Modulreife (73 Module) | Reife 4: 4 (`auth`, `security`, `governance`, `plugins`) · Reife 3: 33 · Reife 2: 32 (u. a. `sharding`, `failover`, `rpc_grpc`, `updates`, `cdc`) · Reife ≤ 1: 4 · Reife 5: 0 |
| Test-Evidenz | Kein gültiger aktueller Volllauf (`ctest_full.log` 2026-10-24: 1.777/1.781 Fehlschläge durch Infrastrukturfehler) |
| Root-Gate-Lage | Wave C offen (C3 Sustained Load ausstehend, C4 in Arbeit; Validierungsartefakt am 2026-10-30 abgelaufen) |

**Kernaussage:** Der **Single-Node-Kern** (RocksDB-Storage, lokale Transaktionen, AQL, Index, Auth/Security/Governance,
Server/API) ist nach Abschluss der P0-Items dieser Roadmap realistisch **Release-Candidate-fähig**. **Multi-Node/HA**
benötigt zusätzlich einen kanonischen Transaktions-/Recovery-Vertrag und echte Multi-Prozess-Chaos-Evidenz.
**AI/LLM** bleibt bis zu belastbarer Real-Model-CI ein Preview-Feature.

---

## Definition „Production-Ready“

Ein Modul bzw. Deployment-Profil gilt im Sinne dieser Roadmap als produktionsreif, wenn **alle** Punkte erfüllt sind:

1. **Fail-closed:** Kein Codepfad meldet Erfolg ohne Wirkung; Fehlkonfiguration (fehlende Abhängigkeit, ungültige
   Zertifikate, fehlende Adapter) führt zu Startfehler oder `readiness=false` – nie zu stillem Degradieren.
2. **Durability-Vertrag:** Bestätigte Writes überleben `kill -9` an beliebiger Stelle; Korruption wird erkannt,
   gemeldet und blockiert den Start, sofern kein expliziter, auditierter Override gesetzt ist.
3. **Sichere Defaults:** Produktionsmodus (kanonisch über `themis::core::ProductionMode`) erzwingt TLS/mTLS,
   Zertifikatsprüfung, signierte Updates und schließt Stub-/Simulationspfade aus.
4. **Nachweisbare Tests:** Gültiger CTest-Volllauf auf `develop` (Linux + Windows) ≤ 7 Tage alt; Sanitizer- und
   Fuzz-Smoke auf PRs; Skip-Budget je Modul eingehalten; Prozess-Crash- und (für Multi-Node) Netzwerk-Chaos-Suiten grün.
5. **Ressourcengrenzen:** Jede externe Anfrage ist durch Timeout, Speicherbudget und Admission Control begrenzt.
6. **Betreibbarkeit:** HA-fähige Helm-Werte, PDB, NetworkPolicy, Monitoring/Alerting, aktuelle und **geübte**
   Runbooks (Restore, Upgrade, Failover) mit gemessenen RPO/RTO.
7. **Wahrhaftige Dokumentation:** Keine Status-/Feature-Behauptung ohne Code- oder Evidenzbeleg; ein kanonischer
   Release-Status über alle Root-Dokumente.
8. **Governance:** GA-Gate-Modell der Root-Roadmap erfüllt und Human-Sign-off erteilt.

---

## Meilensteine

| ID | Name | Zieltermin | Umfang | Exit-Kriterium |
|---|---|---|---|---|
| **M0** | Truth Reset | 2026-11-20 | Doku-Status konsistent, gültige Test-Baseline | DOC-02 geschlossen; gültiger CTest-Volllauf (Linux + Windows) als Artefakt; Gate-Workflows ohne stille Toleranz |
| **M1** | Single-Node RC | 2027-03-31 (Q1 2027) | Stufe 1: `storage`, `transaction` (lokal), `query`, `aql`, `index`, `auth`, `security`, `governance`, `server`, `api`, `config`, `observability` | Alle P0-Items WS1, WS3, WS4, WS6, WS8, WS9 erledigt; Prozess-Crash-Suite grün; Restore-Übung mit gemessener RPO/RTO |
| **M2** | Multi-Node Beta | 2027-06-30 (Q2 2027) | Stufe 2: `sharding`, `replication`, `failover`, `network`, `rpc_grpc`, `cdc` | Alle P0-Items WS2 + WS7; kanonischer Koordinator; Multi-Prozess-Chaos-Suite (≥ 3 Knoten) grün |
| **M3** | GA Single-Node | 2027-06-30 (Q2 2027), **vorbehaltlich Human-Sign-off** | Stufe 1 GA | Komplette [Production Readiness Checklist](#production-readiness-checklist) für Stufe 1; Pentest-Retest ohne offene High |
| **M4** | GA Multi-Node (indikativ) | Q4 2027 | Stufe 2 GA | 90 Tage Beta-Betrieb ohne Datenverlust-Incident; Checklist für Stufe 2 |
| — | AI/LLM | kein GA-Termin | Stufe 3 bleibt „Preview“ hinter Feature-Flags | Re-Evaluierung nach WS5 (Real-Model-CI ohne Skips) |

**Prioritäten:** **P0** = Blocker für den genannten Meilenstein · **P1** = erforderlich für GA der Stufe · **P2** = nach GA
bzw. Qualitätsverbesserung ohne Gate-Wirkung. **Regel:** Jedes Critical-/High-Finding ist P0 für den zugeordneten
Meilenstein; AI/LLM-Findings sind P0 nur für den Preview-Exit (kein GA-Blocker der Stufen 1/2).

---

## In Progress

Bereits in der Root-Roadmap laufende Arbeiten, die diese Roadmap voraussetzt (Status dort maßgeblich):

- [~] Wave C – C3 Sustained-Load-Evidenz (Target: M0 · 2026-11-20) – Quelle [../ROADMAP.md](../ROADMAP.md)
- [~] Wave C – C4 (Target: M0 · 2026-11-20) – Quelle [../ROADMAP.md](../ROADMAP.md)
- [ ] Wave-C-Validierungsartefakt erneuern (abgelaufen 2026-10-30) (Target: M0 · 2026-11-20) – Quelle [WAVE_C_AUDIT_EVIDENCE.md](WAVE_C_AUDIT_EVIDENCE.md)
- [~] Issue #5373 – Koordinator-Konsolidierung; laut Audit **nicht** abgeschlossen, Fortführung in WS2 ([DIS-05](FULL_DEEP_DIVE_AUDIT_2026-10-31.md#dis-05)) (Target: M2)

### Cross-cutting rollout evidence ownership (2026-11-09)

The work-package issue state is administrative tracking, not a gate result. The release-status authority is [`../ROADMAP.md`](../ROADMAP.md); promote a gate only after its current artifact, run, and required review satisfy the listed exit criteria.

| Target | Gate / owner | Current evidence and status |
|---|---|---|
| M0 · 2026-11-20 | Wave A representative hardware and release-critical proof: [#7810](https://github.com/makr-code/ThemisDB/issues/7810) | [`wave_a_exit_closure_manifest.json`](evidence/waves/manifests/wave_a_exit_closure_manifest.json), generated 2026-11-09, remains `open`: release-critical and representative-hardware gates are `pending`; transaction/GPU artifacts are `in_progress`. The issue is closed, but no gate is promoted by that closure. |
| M0 · 2026-11-20 | Wave AB gap inventory and G2/G3/G4: [#7814](https://github.com/makr-code/ThemisDB/issues/7814) | [`wave_ab_gap_closure_manifest.json`](evidence/waves/manifests/wave_ab_gap_closure_manifest.json) remains `open`, with G1–G4 `in_progress`. The 2026-11-09 [marker inventory](MARKER_LOCATIONS_2026-11-09.md) and [classification](MARKER_GAP_CLASSIFICATION_2026-11-09.md) are dated snapshots; candidate review and validator evidence remain necessary. |
| M0 · 2026-11-20 | Full-run evidence, fail-closed exceptions, and CI-04: [#7819](https://github.com/makr-code/ThemisDB/issues/7819) | EVD-01, OPS-01/CI-03, and CI-04 remain open in WS6 until Linux and Windows full-run JUnit artifacts, the owned/dated exception inventory, and positive/negative `pull_request_target` guard evidence are linked. The required artifact scope and retention are specified in the WS6 items below. |
| M0 · 2026-11-20 | Wave C C3/C4 evidence: [#7812](https://github.com/makr-code/ThemisDB/issues/7812), with [#7813](https://github.com/makr-code/ThemisDB/issues/7813), [#7816](https://github.com/makr-code/ThemisDB/issues/7816), and [#7817](https://github.com/makr-code/ThemisDB/issues/7817) | [`security_wave_c_closure_manifest.json`](evidence/waves/manifests/security_wave_c_closure_manifest.json) is dated 2026-10-01 and remains `open` (C3 `pending`, C4 `in_progress`). The Q3 bundle still lists PENTEST-Q3-001/002/003/004 as open; #7813's implementation reference is not executed remediation evidence, #7816's triage closure does not dispose of Q3-003/004, and #7817 remains open for BSI C5 traceability. Keep C4 open pending current evidence and authorized sign-off. |
| v2.5.0 M1 · 2027-03-31 | PR sanitizer and parser fuzz gates: [#7821](https://github.com/makr-code/ThemisDB/issues/7821) | OPS-06/07 remain acceptance gates in WS6. Verify bounded runs, failure propagation, tested commit, and retained run evidence; the older sanitizer bundle is not current Wave C proof. |
| v2.5.0 M1 · 2027-03-31; v3.0.0 M3 coverage sign-off | Test-quality inventory and aggregate coverage: [#7820](https://github.com/makr-code/ThemisDB/issues/7820) | [`TEST_QUALITY_INVENTORY.md`](TEST_QUALITY_INVENTORY.md) and [`test_quality_baseline_v1.json`](test_quality_baseline_v1.json) are a 2026-11-09 static baseline from commit `c21f7bba0cb44d58e5ad49f64781a76703b3ee56`; coverage is unavailable. Module behavior stays with the linked module owners, and M3 coverage gates remain open. |
| v2.5.0 M1 · 2027-01-31 | Generated/oversized artifact inventory and file-size gate: [#7822](https://github.com/makr-code/ThemisDB/issues/7822) | HYG-01 remains open in WS10. Refresh the tracked-file inventory and attach before/after and positive/negative gate evidence; historical audit counts do not establish current cleanup. |
| v2.5.0 M1 NetworkPolicy; v3.0.0 M2 HA · 2027-06-30 | Production Helm profile: [#7823](https://github.com/makr-code/ThemisDB/issues/7823); monitoring remains [#7532](https://github.com/makr-code/ThemisDB/issues/7532) | OPS-08/09 require rendered-profile validation, including NetworkPolicy allowlists for M1 and ≥3 replicas, PDB, anti-affinity, and topology spread for M2. OPS-16 ServiceMonitor/alert evidence remains owned by #7532; verify rendered manifests and runtime checks before release promotion. |
| v2.5.0 M1 · 2027-03-31 | Benchmark density and release-claim traceability: [#7828](https://github.com/makr-code/ThemisDB/issues/7828) | [`BENCHMARK_MODULE_COVERAGE.json`](BENCHMARK_MODULE_COVERAGE.json), generated 2026-11-09 from develop commit `c21f7bba0cb44d58e5ad49f64781a76703b3ee56`, inventories 73 modules but reports 0 covered, 3 gaps, and 70 blocked; it is not an executed benchmark baseline. Run [42548800031](https://github.com/makr-code/ThemisDB/actions/runs/42548800031) failed at CMake configure because RocksDB was unavailable; benchmark build, execution, and result upload were skipped. Refresh the inventory and attach reproducible workloads/results; do not impose a blanket per-module benchmark count. Specific workloads remain with #7488, #7534, #7810, #7492, and #7490. |
| v2.5.0 M1 · 2027-03-31 | Wave D observability, soak, and operator readiness: [#7811](https://github.com/makr-code/ThemisDB/issues/7811) | [`operability_wave_d_closure_manifest.json`](evidence/waves/manifests/operability_wave_d_closure_manifest.json), generated 2026-11-09, remains `open`; D2–D4 are pending. Require all four 24–48-hour runs, measured thresholds, current run/artifact links, and operations/architecture acceptance before promotion. |

Pentest ownership is intentionally distinct by finding: PENTEST-Q3-001 is tracked by #7813 (implementation evidence alone does not close it); PENTEST-Q3-002 is the RAG upload path owned by [#7491](https://github.com/makr-code/ThemisDB/issues/7491), not the separate historical #1912 path; PENTEST-Q3-003/004 remain open with summary-only evidence despite #7816 triage closure. [#7817](https://github.com/makr-code/ThemisDB/issues/7817) is the separate BSI C5 evidence-trace work package. Do not infer finding disposition or C4 sign-off from issue closure.

---

## Planned Features

Jedes Item verweist auf die Finding-ID im Audit-Bericht. Notation: `[ID]` → Abschnitt 5 von
[FULL_DEEP_DIVE_AUDIT_2026-10-31.md](FULL_DEEP_DIVE_AUDIT_2026-10-31.md).

### WS1 – Durability & Recovery

- [~] **P0 · STO-01 – WAL-Recovery fail-closed** (Target: M1 · 2026-12-15)
  - [~] 2026-11-02 source follow-up: `WALStorage::open` exposes an optional `RecoveryResult` with
    `Complete`, `TruncatedTail`, `Corrupted`, `TimedOut`, and `Failed` status values. Strict is the default;
    `Config::recovery_timeout` is configurable, zero means unlimited, and negative values are rejected. An incomplete
    final header is truncatable only when its available bytes match the WAL magic prefix; corruption in the middle of
    a segment fails open. Explicit `SkipCorrupted` quarantines a corrupt segment under `<wal_dir>/quarantine` and can
    succeed with status `Corrupted`. Callback aborts and I/O errors fail open. Replication returns false from its
    recovery callback when `WALEntry::deserialize` fails, so startup fails.
  - **Partial implementation only.** Remaining: process-kill and broader corruption/property-test evidence,
    actual test execution, and replay-performance evidence. The ≤5% replay-regression benchmark against a recorded
    baseline is still open; status reporting does not establish metrics or readiness wiring.
  - Dateien: `src/storage/wal_storage.cpp` (`replaySegment`, Z. 490-514), `include/storage/wal_storage.h`,
    Nutzer `src/sharding/wal_manager.cpp:147`, `src/replication/replication_manager.cpp:670-685`.
  - Verhalten: Recovery liefert `RecoveryResult { status ∈ {Complete, TruncatedTail, Corrupted, TimedOut, Failed},
    last_valid_lsn, skipped_bytes, segment }`. `TruncatedTail` nur, wenn nach der defekten Stelle **kein** gültiger
    Record-Header mehr im Segment folgt; jede Korruption mit nachfolgenden gültigen Records ist `Corrupted`.
  - Produktionsmodus/Observability offen: Die Quelle implementiert keine hier zugesagte `readiness`- oder
    `themis_wal_recovery_status`-Metrik. `RecoveryMode::SkipCorrupted` und `Config::recovery_timeout` sind
    `WALStorage`-API-Konfigurationen; es ist keine externe `wal.recovery_mode`- bzw. Sekunden-Config hier belegt.
  - Replikation: nicht deserialisierbare Einträge (`replication_manager.cpp:670-674`) lassen den Recovery-Callback
    jetzt `false` zurückgeben und schlagen beim Start fehl.
  - Fehlerfälle: Magic-Mismatch mitten im Log, CRC-Fehler, abgeschnittener Header, Länge > Segmentrest, I/O-Fehler, Timeout.
  - Tests: `CorruptedEntry_StopsReplay` (`tests/test_wal_storage.cpp:290-316`) ersetzen durch
    `MidLogCorruption_FailsClosed`, `TruncatedTail_IsAccepted`, `SkipCorrupted_QuarantinesSegment`,
    `RecoveryTimeout_FailsClosed`; Property-Test mit zufälligen Bit-Flips (≥ 10.000 Fälle, deterministischer Seed).
  - Performance-Gate offen: Replay-Durchsatz-Regression ≤ 5 % gegenüber einer gemessenen und eingecheckten Baseline
    ist noch nicht nachgewiesen.
  - Kompatibilität: Breaking Change (siehe [Breaking Changes](#breaking-changes)); Storage-README/API synchronisiert
    2026-11-02, ohne die offenen Test- oder Performance-Gates als erledigt auszuweisen.
- [ ] **P0 · STO-07 – Prozess-Crash-Harness** (Target: M1 · 2027-01-31)
  - [~] 2026-11-02: The registered subprocess harness now covers coordinator decision boundaries, participant crashes before PREPARE/after durable PREPARE/during apply/after ACK, replica-WAL crashes across prepare/decision/commit boundaries, and a combined coordinator/participant/replica restart scenario. Build/test execution and CI artifact publication are pending; these WAL-process tests do not establish network replication, RocksDB, backup/restore, or randomized storage-fault coverage.
  - Kindprozess schreibt bestätigte Commits; Elternprozess beendet ihn per `SIGKILL`/`TerminateProcess` an zufälligen
    Punkten (inkl. während `fsync`, Segment-Rotation, Checkpoint, Backup). Danach Neustart und Prüfung:
    jeder bestätigte Commit sichtbar, keine uncommitteten Transaktionen sichtbar, Indizes konsistent.
    Ein fehlendes/beim Abbruch noch nicht beobachtetes Client-ACK beweist keinen Rollback:
    ein bereits vollständig persistierter Commit darf nach Neustart sichtbar sein.
  - Abdeckung: RocksDB-Primärspeicher, `WALStorage` (Sharding/Replikation), Backup/Restore.
  - Umfang: ≥ 50 Iterationen auf PRs mit Storage-Änderungen, ≥ 1.000 Iterationen nightly; Ergebnis als Artefakt.
  - **Teilweise implementiert, nicht ausgeführt:** `tests/storage/test_wal_process_crash.cpp` startet einen separaten
    WAL-Worker mit `fsync_on_write=true`, hält ihn zwischen Operationen an und sendet `SIGKILL` nach bestätigten
    Appends an Rotations- und Checkpoint-Grenzen. Nach dem Neustart prüft der Test, dass genau die bestätigten PUTs
    wiederhergestellt werden. Er beweist keine Kills innerhalb eines laufenden write/fsync/rotate/checkpoint-Aufrufs
    und deckt weder Backup/Restore noch RocksDB-Indizes ab.
  - **Offen:** Testausführung und Artefakte; in-syscall-Kill-Punkte, Backup/Restore, Indexkonsistenz sowie ≥ 50
    PR- und ≥ 1.000 Nightly-Iterationen. Die geforderten STO-07-Gates bleiben bis zu diesen Nachweisen offen.
  - Bestehende In-Process-Tests (`tests/test_crash_recovery_direct.cpp`, `tests/test_wal_chaos.cpp`) bleiben als Unit-Ebene.
  - [x] Wiederholte Recovery je Crash ergänzt: Nach dem ersten validierten Replay muss ein zweiter separater Prozess exakt denselben Präfix prüfen, ohne weitere Zusatz-PUT-Toleranz. Segmentnamen, Größen und SHA-256-Inhalte dürfen sich dabei nicht mehr ändern. JSON erfasst `verified_reopens` und WAL-Bytes vor/nach der ersten validierten Recovery. Beide 50er-CTest-Szenarien bestanden (100 Abbrüche, 200 validierte Reopens); 28 fokussierte Tests plus acht Subtests bestanden. Dies belegt WAL-Replay-Stabilität, nicht idempotente Anwendung auf RocksDB/Indizes.
    - Berichtskontrolle: Im ACK-Grenzen-Lauf keine erste Größenänderung; im Unobserved-Append-Lauf zwei erste Größenänderungen bei erfolgreicher Recovery. Alle zweiten Replays blieben byte-/segmentstabil; ein gezielter Syscall-Abbruch ist dadurch weiterhin nicht bewiesen.
  - [x] Erste-Recovery-Grenze verschärft (2026-11-04): Byte-Snapshots ersetzen den Hashvergleich und prüfen zusätzlich den ersten Replay. Keine Segmente dürfen hinzukommen/verschwinden; ältere Segmente bleiben exakt unverändert. Nur im Unobserved-Append-Szenario darf das numerisch neueste Segment zu einem strikt kürzeren identischen Präfix werden. ACK-Grenzen erlauben keine Byte-Änderung. Beide 50er-CTest-Szenarien bestanden; die neue Regression reproduzierte zuvor unbemerkt akzeptierte Änderungen beim ersten Replay.
    - Validierung: 30 Report-/Workflow-/Metadaten-Tests plus 14 Subtests bestanden; `git diff --check` sauber. Die gespeicherten Berichte enthalten 100 Abbrüche und 200 validierte Reopens; acht erlaubte erste Tail-Kürzungen im Unobserved-Append-Lauf, keine im ACK-Grenzen-Lauf. Keine Produktions-C++-Änderung und kein erneuter Build erforderlich.
  - [x] Fehler-Retention ist mit `--report` umgesetzt. Eindeutige berichtsnahe `*-failures/iteration-*`-Ordner erhalten WAL vor Recovery und beim Fehler sowie Reproduktionsparameter und Fehlermeldung; `failure_artifact` verlinkt den Ordner im JSON. Erfolgslaeufe bereinigen weiterhin ihre temporaeren Daten. Beide CI-Caller deklarieren diese Ordner fuer sekundaere Uploads; tatsaechliche GitHub-Publikation bleibt offen. Beide 50er-CTest-Szenarien sowie 33 fokussierte Tests und 14 Subtests bestanden lokal.
    - Retention-I/O abgesichert: Der Workspace entsteht direkt im Evidenzordner; keine fehleranfaellige nachtraegliche Kopie des Originals. Scheitert `failure.txt`, bleiben beide WAL-Zustaende erhalten; JSON nennt den urspruenglichen Fehler und `retention_error`. 37 fokussierte Tests plus 14 Subtests und beide 50er-Crash-CTest-Szenarien bestanden. Ein echter Worker mit anschliessendem Recovery-Fehler und realem Diagnose-Dateisystemfehler bestaetigte den Erhalt beider WAL-Zustaende.
  - [~] Sanitizer-Gate fail-closed konfiguriert: `-fno-sanitize-recover=all` fuer C/C++ in beiden Nightly-Modi verhindert recoverable UBSan-Fortsetzung. 38 fokussierte Tests plus 14 Subtests, actionlint und ausgefuehrte Bash-Flag-Auswahl bestanden. Kein Sanitizer-Laufzeit-PASS: der gepruefte develop-Lauf [41760710588](https://github.com/makr-code/ThemisDB/actions/runs/41760710588) scheiterte vor Validierung an `spdlog`-Compilerfehlern in `src/sharding/raft_state.cpp` (Zeilen 130, 150, 358, 408).
    - Versionsabgleich zum Sanitizer-Compilerblocker (2026-11-04): Lauf 41760710588 baute develop-Revision `d66b7a7f4f50e08c3bebeb8b5ef57d28745b2da5`. Deren neue Raft-Fehlerpfade verwenden `spdlog::error` ohne direkten Include; diese Pfade fehlen im aktuellen Workspace. Der konfigurierte lokale `raft_state.cpp`-Objekt-Build besteht, belegt aber weder eine Reparatur dieser develop-Revision noch Sharding-Link-/Test-/Sanitizer-Erfolg. Kein ungenutzter Include und keine ungepruefte Uebernahme der abweichenden Raft-Implementierung.
  - [x] Writer-Diagnostik erhalten: `writer.log` sichert kombinierte stdout/stderr nach Prozessende und Reader-Join, einschliesslich konsumierter ACKs und verbleibender Diagnosezeilen. Log-I/O-Fehler scheitern explizit und erhalten den urspruenglichen Writer-Fehler im Bericht. Beide 50er-Crash-Suiten, 40 fokussierte Tests und 14 Subtests bestanden; ein echter Fehlerlauf erhielt alle zwoelf geordneten ACKs. Nicht ausgegebene Kindprozess-Puffer bleiben naturgemaess unbeobachtet.
  - [x] Reader-Fehler fail-closed: Pipe-I/O- und Dekodierungsfehler werden dem Elternprozess uebergeben statt als Thread-Exception/ACK-Timeout verloren zu gehen. Nach Reader-Join werden auch Fehler hinter dem letzten ACK vor Recovery abgewiesen; partieller Writer-Log bleibt erhalten. Beide 50er-Crash-Suiten und 42 fokussierte Tests plus 16 Subtests bestanden.
  - [x] Aktuellen gehaerteten Harness erneut skaliert (2026-11-04): je 1.000/1.000 Abbrueche pro Szenario bestanden, insgesamt 4.000 validierte Reopens mit erster/wiederholter Byte-Pruefung. ACK-Grenzen: 783 Rotationen, keine Byte-Aenderung, 378,079 s. Unobserved-Append: 758 Rotationen, 635 Zusatz-PUTs, 365 Praefix-Recoveries, 174 erlaubte erste Tail-Kuerzungen, 560,185 s. Separate `wal-process-crash-hardened-1000.json` und `wal-unobserved-append-hardened-1000.json` im Windows-Build erhalten diesen neuen Pruefstand; Erfolgs-Evidenzordner leer. 42 fokussierte Tests plus 16 Subtests bestanden. Kein Linux-/Sanitizer-/GitHub-PASS und weiterhin kein gezielter Syscall-/Primaerspeicher-Nachweis.
  - [x] Pruefstand-Zuordnung neuer Berichte: `worker_sha256` und `harness_sha256` identifizieren Binary und Harness-Datei vor den Iterationen; Hash-I/O-Fehler und Abweichungen am Laufende verhindern `passed`. Beide 50er-Crash-Suiten und 46 fokussierte Tests plus 16 Subtests bestanden. Keine DLL-/Build-Attestierung und keine Erkennung zwischenzeitlich rueckgaengig gemachter Aenderungen; vorhandene 1.000er-Berichte werden nicht nachtraeglich mit Hashes versehen.
  - [x] Sanitizer-Diagnosen im Harness fail-closed: Erkannte ASan/LSan/UBSan/TSan/MSan-Meldungen verhindern Erfolg unabhaengig vom Exitcode. Writer-Pruefung vor ACK und nach Reader-Join, Verifier-Pruefung auf stdout/stderr inklusive N+1-Retry. Beide 50er-Crash-Suiten und 53 fokussierte Tests plus 30 Subtests bestanden. Diagnose-Textregressionen sind keine instrumentierten Sanitizer-Laufzeittests; Signaturerkennung und Beobachtung ungeflushter Ausgabe sind begrenzt.
  - [x] Verifier-Timeout-Diagnostik erhalten: Das bestehende 30-Sekunden-Limit pro Prozess meldet erwartete Replay-Anzahl und beide Ausgabekanaele via byte-erhaltendem `repr`, auch beim N+1-Retry. Timeout erlaubt keine weitere Toleranz; JSON/`failure.txt` erhalten den Fehler und der WAL bleibt erhalten. Beide 50er-Crash-Suiten und 57 fokussierte Tests plus 33 Subtests bestanden; echter Subprozess-Timeout mit geflushter stdout/stderr geprueft. Kein Produktions-Recovery-Latenznachweis.
  - [x] Laufenden Fortschritt atomar gesichert: Nach jeder validierten Iteration werden Ergebnis-Praefix und Laufzeit geschrieben; Status bleibt bis zum Abschluss `running`. Harter Elternprozess-Abbruch erhaelt den letzten Checkpoint, nicht den laufenden Versuch oder einen PASS. Schreibfehler stoppen weitere Iterationen. Beide 50er-Crash-Suiten und 60 fokussierte Tests plus 33 Subtests bestanden. Elternprozess-Abbruchregression verwendet ausschliesslich Test-Iterationsergebnisse fuer den Berichtsnachweis; kein neuer WAL-/Stromausfall-Nachweis.
  - [x] Alle finalen Byte-Grenzen lokal geprueft (2026-11-04): 40 Praefixe des letzten 39-Byte-Frames nach echtem Writer-Kill bestanden; 38 nichtleere Partial-Tails wurden exakt gekuerzt und beim zweiten Replay nicht mehr veraendert. 18 Payload-/CRC-Bitfehler und 36 unvollstaendige Enden eines aelteren Segments scheiterten zweimal geschlossen ohne Dateiaenderung. Bericht `build-msvc-windows-debug/test-results/wal-tail-boundaries-20261104.json`: 94 Faelle, 80 erfolgreiche Reopens, 108 fehlgeschlagene Opens, 189 Verifier-Prozesse inklusive Count-Probe, 31,203 s. Absichtliche Dateifehler nach 12 ACKs pruefen den Erhalt von Eintraegen 1..11; kein ACK-Verlust durch Prozessabbruch allein behauptet. Ad-hoc-Kampagne, nicht CI-registriert und kein gezielter Syscall-/Primaerspeicher-Nachweis.
  - [x] 94-Faelle-Kampagne reproduzierbar eingecheckt: `tests/storage/test_wal_tail_boundaries.py` mit per-Fall-Bericht, Fehler-WAL-Retention und drei Dateifingerprints. PR/Nightly rufen sie auf und deklarieren `wal-tail-boundaries.json` plus Fehlerordner fuer Upload. Lokal 94/94 bestanden (80 erfolgreiche Reopens, 108 geschlossene Open-Fehler, 189 Verifier-Prozesse, 21,146 s); 64 fokussierte Tests plus 35 Subtests, actionlint und Bash-Syntax bestanden. Tatsaechliche GitHub-/Sanitizer-Ausfuehrung bleibt offen.
  - [x] Dateifehlerkampagne auf alle drei aelteren Segmente erweitert (2026-11-04): je 36 unvollstaendige Frame-Tails scheitern zweimal geschlossen ohne Byte-Aenderung. Ergebnis und Reproduktionsdaten nennen `affected_segment`. Separater lokaler Bericht `build-msvc-windows-debug/test-results/wal-tail-all-segments.json`: 166 Faelle, 80 erfolgreiche Reopens, 252 geschlossene Fehler-Opens, 333 Verifier-Prozesse, 43,566 s; alle 38 nichtleeren Partial-Tails im neuesten Segment exakt repariert. Exakte Segment-/Offset-Mengen und bestehende Vertraege: 64 Tests plus 35 Subtests bestanden. Historische 94er-Berichte unveraendert; bestehende CI-Aufrufe nutzen das erweiterte Skript, erfolgreiche GitHub-/Sanitizer-Ausfuehrung sowie Syscall-/Primaerspeicher-Nachweise bleiben offen.
  - [x] Forensische Retention der Dateifehlerkampagne abgesichert (2026-11-04): `before-recovery/` sichert die injizierten WAL-Bytes vor jedem Verifier; Kopierfehler stoppen vor Recovery und erhalten das Original. Regressionen reproduzierten vorher fehlende Eingangsdaten auf beiden Recovery-Pfaden. 166 echte Worker-Faelle erneut bestanden (`wal-tail-forensic-retention.json`, 80 erfolgreiche Reopens, 252 geschlossene Fehler-Opens, 333 Prozesse, 40,822 s); 66 Tests plus 37 Subtests bestanden. Ein diagnostisch absichtlich falscher Count nach echter Tail-Reparatur erhielt 79-Byte-Eingang und 78-Byte-Ausgang sowie einen expliziten Fehlerbericht. Kein Produktions-Recovery-Defekt oder Backup-Durabilitaetsnachweis behauptet; die offenen STO-07-Nachweise bleiben unveraendert.
  - [x] Payload-/CRC-Korruption aller zwoelf Fixture-Frames geprueft (2026-11-04): je ein Low-Bit-Flip an 198 Bytepositionen scheitert zweimal geschlossen ohne WAL-Aenderung. Absolute `segment_byte_offset`-Angaben in Ergebnis/Reproduktion und explizite Laengenpruefung aelterer Fixture-Segmente. Lokaler Bericht `build-msvc-windows-debug/test-results/wal-all-frame-corruption.json`: 346 Faelle, 80 erfolgreiche Reopens, 612 geschlossene Fehler-Opens, 693 Verifier-Prozesse, 83,410 s. Exakte Bytepositionen je Frame, Fingerprints und Bereinigung unabhaengig geprueft; 67 Tests plus 39 Subtests und Python-Syntax bestanden. Kein Alle-Bits-/Header-Fuzzing oder Syscall-/Primaerspeicher-/Sanitizer-Nachweis; historische Berichte unveraendert.
  - [x] Magic-/Sequenz-/Typ-Headerkorruption aller zwoelf Frames geprueft (2026-11-04): je ein Low-Bit-Flip an 156 Header-Bytepositionen (Frame-Offsets 0..12) scheitert zweimal geschlossen ohne Dateiaenderung. Absolute Byteoffsets in Ergebnis/Reproduktion. `build-msvc-windows-debug/test-results/wal-all-frame-header-corruption.json`: 502 Faelle, 80 erfolgreiche Reopens, 924 geschlossene Fehler-Opens, 1.005 Verifier-Prozesse, 120,286 s. Exakte Header-/Payload-/CRC-Mengen, Fingerprints und Bereinigung unabhaengig geprueft; 67 Tests plus 40 Subtests bestanden. Laengenfelder (13..20) bleiben wegen der Abgrenzung zu reparierbaren unvollstaendigen neuesten Tails separat; kein pauschaler Ablehnungs-, Alle-Bits-, Syscall-/Primaerspeicher-/Sanitizer-Nachweis behauptet. Historische Berichte unveraendert.
- [ ] **P0 · STO-04 – Wartungs-Threads mit Lebenszyklus** (Target: M1 · 2027-01-31) – `std::thread(...).detach()` in
  `src/maintenance/database_maintenance_orchestrator.cpp:608-613` durch `std::jthread` mit `std::stop_token` ersetzen;
  `stop()` joint mit Timeout; TSan-Test „Shutdown während laufender Wartung“.
- [~] **P1 · STO-10 – Teilbatch-Semantik Streaming-Ingest** (Target: M1 · 2027-02-28) – `ingestBatch()` admits all
  events or none: BLOCK waits for whole-batch capacity and times out with none enqueued; DROP returns zero and drops
  all if capacity is insufficient. Each RocksDB WriteBatch chunk is atomic, not the entire input batch. A failed
  chunk and remaining suffix are retained ahead of newer events; errors report `failed_offset`/`total`; prior
  successful chunks remain committed and ordered retry is PUT-idempotent. Contract documentation is synchronized.
  `tests/storage/test_streaming_ingest_retry_queue.cpp` covers the requeue helper with a later-chunk offset, ensuring
  committed-prefix entries are excluded and the suffix precedes newer queued events. The new
  `tests/storage/test_streaming_ingest_rocksdb_failure.cpp` wraps a real RocksDB `Env` and fails a later WAL-file
  `Append`; it verifies `failed_offset`, that the committed prefix stays visible, that the failed suffix is retried
  before a concurrently queued newer PUT, and that final values survive close/reopen. This is source coverage only
  until the focused target is built and run; execution evidence remains open.
  - Validation note (2026-11-02): no tests or benchmarks were run for this documentation follow-up. Local diagnostic
    configure was blocked by missing system `fmt`. Observed unrelated Actions Build Wave B Transaction run
    `41389381448` failed CMake configure because `yaml-cpp` was missing; its test gate failed because jobs were
    skipped. That run is not storage execution evidence.
- [ ] **P1 · STO-05 – Changefeed-Sequenz fail-closed** (Target: M1 · 2026-12-15) – leere Catches in `src/cdc/changefeed.cpp:220-223`
  durch Fehlerstatus + Metrik ersetzen; korrupte Legacy-Sequenz darf nicht still auf Fallback zurückfallen.
- [ ] **P1 · STO-06 – CDC-Outbox-Tests unter Windows** (Target: M1 · 2027-01-31) – Fixture-Crash (`tests/cdc/test_cdc_outbox.cpp:26`) beheben, Skip entfernen.
- [ ] **P2 · STO-08 – Timeseries-Testtiefe** (Target: M3 · 2027-06-30) – Retention, Compaction, große Aggregationsfenster, nebenläufige Ingestion.

### WS2 – Distributed Consistency & HA

- [~] **P0 · DIS-05 – Ein kanonischer 2PC-Koordinator** (Target: ADR 2026-12-15 · Umsetzung M2)
  - [x] Entscheidung: `TwoPhaseCommitCoordinator` ist kanonisch für direktes 2PC;
    `CrossShardTransactionCoordinator` bleibt die Multi-Protokoll-Fassade. Siehe
    [`ADR_DIS-05_CANONICAL_2PC_COORDINATOR.md`](ADR_DIS-05_CANONICAL_2PC_COORDINATOR.md).
  - [~] CrossShard-2PC delegiert Prepare/Decision/Phase 2 an den kanonischen Koordinator;
    dessen WAL speichert Endpoint-Metadaten und ist die einzige 2PC-Recovery-Autorität.
    Unaufgelöste Legacy-CrossShard-2PC-Einträge ohne kanonischen WAL bleiben fail-closed.
    Diese Konsolidierung belegt weder die Replikation der Koordinatorentscheidung per
    Shard-Raft noch die RAID-1-Integration des separaten `DistributedTransactionCoordinator`,
    eine per Leaderwechsel wiederaufnehmbare 3PC-Recovery oder dauerhaften Blocktransport mit
    Client-ACK. Diese End-to-End-Nachweise bleiben außerhalb von DIS-05 offen; siehe
    [Replikations-Roadmap](../src/replication/ROADMAP.md) und DIS-06.
  - [ ] Fehlerfälle mit Tests belegen: Koordinator-Crash vor/nach Decision-Write,
    Participant-Timeout, doppelte Nachrichten und Wiederholung der ausgewählten Entscheidung.
  - [ ] Beide öffentlichen Aufrufpfade mit Erfolg, Abbruch, Decision-Write-Fehler,
    Phase-2-Fehler und Grenzfällen testen; Prozess-Crash-Varianten mit STO-07 abstimmen.
- [ ] **P0 · DIS-06 – Recovery-Vertrag** (Target: M2 · 2027-04-30)
  - Ein Dokument plus Code-Durchsetzung: Decision-Log durable **vor** COMMIT-ACK; In-Doubt-Auflösung beim Start
    (blockierend, mit Metrik); Participant-WAL und Replikations-WAL nutzen den fail-closed Recovery-Status aus STO-01.
  - Test: Matrix {Koordinator, Participant, Replica} × {Crash vor Prepare, nach Prepare, nach Decision, während Commit}.
  - [~] 2026-11-02: `DistributedTransactionManager` and `GlobalTransactionManager` check durable decisions before
    Phase-2 delivery; participant PREPARE votes and COMMIT acknowledgments require durable WAL records, participant
    recovery restores prepared locks and propagates WAL errors, and incomplete delivery remains unresolved.
    DTM journals participant node IDs and reconstructs decisions through a configured endpoint resolver. The HTTP
    `/dtxn/*` runtime now enables its coordinator WAL under the configured storage WAL/database directory, propagates
    WAL replay errors as startup failures, resolves node addresses and IDs from `ShardingManager`, periodically retries
    only known recovery blockers from readiness checks and topology changes, and makes `/health/ready` fail while durable recovery blockers remain (not
    for ordinary in-flight requests).
    Recovery also preserves in-doubt state when participant IDs/endpoints are missing instead of completing an empty
    replay; malformed participant descriptors are also retained as unresolved.
  - [~] 2026-11-19 / #8200: `TwoPhaseCommitCoordinator` ist die kanonische Entscheidungsautorität
    für das benannte Koordinator-Paar. `CrossShardTransactionCoordinator` delegiert ausschließlich seinen
    2PC-Pfad; andere Protokolle und öffentliche Einstiegspunkte bleiben erhalten. BEGIN, Entscheidung
    und Abschluss werden synchron persistiert; Abschluss erst nach allen Participant-ACKs.
    Participant-Timeouts melden In-Doubt, schreiben niemals selbst ABORT und geben keine vorbereiteten
    Sperren frei. Nicht auflösbare Wiederanläufe blockieren Readiness, nicht die Entscheidungs-RPCs.
    WAL-Domänen bleiben getrennt; ungelöste alte CrossShard-Records werden nicht still migriert.
    Neue Prozess-Regressionen verwenden echte Koordinator-/Participant-Objekte über Test-IPC;
    dies ist kein Nachweis für produktiven gRPC/mTLS- oder Replikat-Transport.
    Ausführungsevidenz siehe `../src/sharding/ROADMAP.md`; vollständige Matrix, Release-/Plattform-Gates
    und Maintainer-Freigabe bleiben offen. Keine API-Entfernung oder WAL-Formatmigration in diesem Schritt.
- [~] **P0 · DIS-03 – Echter Stream-Transport für Shard-Transfer** (Target: M2 · 2027-03-31; [#8197](https://github.com/makr-code/ThemisDB/issues/8197))
  - `AutoRebalancer` verlangt einen expliziten Executor; der bisherige `StreamSession`-Pfad bleibt ohne Transport fail-closed.
  - Offen: Deployment-Verdrahtung des Storage-Exports, Schreibquieszenz, autoritativer gefenceter Cutover, Recovery bei verlorener Cutover-Bestätigung und Release-Evidenz.
  - Dedizierter mTLS-gRPC-Transfer mit dauerhaftem Staging, Checkpoints, Abbruch
    und Inhaltsprüfung; `AutoRebalancer` verlangt einen expliziten Executor.
    Der bisherige `StreamSession`-Pfad bleibt ohne Transport fail-closed.
  - Offen: Deployment-Verdrahtung des Storage-Exports, Schreibquieszenz, autoritativer
    gefenceter Cutover, Recovery bei verlorener Cutover-Bestätigung und Release-Evidenz.
    Ein typed Routing-Token-/Ownership-Snapshot sowie ein prozesslokaler,
    begrenzter MVCC-Export mit echter RocksDB-Snapshotsequenz, HLC-Grenze und
    expliziten Tombstones sind implementiert; Testquellen sind hinzugefügt,
    Ausführungsevidenz steht aus. Keine Hash- oder Ownership-Änderung:
    [Architekturstatus](../src/sharding/ARCHITECTURE.md#dis-03-routing--und-snapshot-grundlagen)
    und [Sharding API](../src/sharding/API.md#shard-transfer-dis-03).
- [~] **P0 · DIS-08 – Failover/DR-Handler fail-closed** (Target: M2 · 2027-04-30)
  - [~] 2026-11-01: Non-dry-run DR steps now fail when snapshot/restore/catch-up/traffic/verification hooks are missing;
    quorum no longer counts as catch-up proof. Spare failure blocks failover and missing metadata persistence returns
    failure. 2026-11-21: independent verification retries mismatches up to the configured additional-attempt count;
    proofs returned after `verification_timeout` fail closed. DRE-27..29 cover retry and deadline behavior.
    2026-11-21 contract follow-up: typed storage, traffic, and shared-metadata adapter boundaries are now explicit;
    DRE-30..33 cover adapter success, negative append/load, and shared replay behavior. Remaining: implement real
    storage/traffic adapters and a consensus-backed metadata authority, then add networked multi-process
    end-to-end coverage.
    failure. Remaining: implement persistent metadata and real adapters with ACK plus independent content/LSN
    verification, then add multi-process end-to-end coverage.
  - [x] 2026-11-04: When spare activation is enabled but no spare manager is configured, failover now emits
    `SPARE_ACTIVATION_FAILED` and stops before metadata update; a focused regression covers the missing-manager case.
    The item remains in progress because the sharding runtime adapter, persistent metadata, independent verification,
    and multi-process coverage are still absent.
  - Handler ohne konfigurierten Storage-/Traffic-/Metadata-Adapter liefern `NotConfigured`; Erfolg nur nach Adapter-ACK
    **und** Verifikation (Checksumme bzw. LSN/Term-Vergleich); Metadata-Update persistent.
  - Tests: negative Tests je fehlendem Adapter; Ende-zu-Ende-Failover in der Multi-Prozess-Suite.
- [~] **P0 · DIS-10/DIS-11/TST-03 – Multi-Prozess-Chaos-Suite** (Target: M2 · 2027-05-31)
  - DIS-10 source follow-up 2026-11-21: `TransactionProcessCrashRecoveryTests` ist registriert und deckt
    echte POSIX-Prozess-Crashes, Coordinator-/Participant-Restart, WAL-Replay, Locks und verlorene ACKs ab.
    Das bestehende Test-IPC über Pipes ist keine gRPC/mTLS-Netzwerk- oder Partition-Heilungs-Evidenz;
    Stub-2PC-/WAL-Fälle in `test_sharding_contract_hardening_focused.cpp` ersetzen diesen Nachweis nicht.
  - Direkter Linux-Evidenzlauf am 2026-11-21: 3/3 reale gRPC/mTLS-Prozessfälle bestanden
    (Commit-Erfolg, Coordinator-SIGKILL-Recovery, Participant-Crash/Restart-Heilung). Zu diesem
    Zeitpunkt gab es noch keinen registrierten CMake/CTest-Nachweis; der damalige lokale Build-
    Graph-Blocker wurde durch den fokussierten registrierten Lauf am 2026-11-22 unten überholt.
  - Wiederholungsnachweis am 2026-11-22: 3 Iterationen × 3 Fälle = 9/9 erfolgreiche
    Prozess-Testausführungen im direkten Diagnostik-Binary; kein Ersatz für CTest-/CI-Evidenz.
  - Registrierter Linux-Nachweis am 2026-11-22: Die ursprüngliche 3-Fall-Version von
    `DIS10RealTransportProcessRecoveryTests` wurde als CMake/Ninja-Target gebaut und einmal
    plus 3/3 Wiederholungen per CTest bestanden. Dies belegt reale gRPC/mTLS-Prozesse und
    WAL-Heilung unter Linux/Diagnostik; kein Release-/CI-/Plattformgate.
  - Partition-Heilungsnachweis am 2026-11-22: ein transparenter TCP-Testproxy blockiert
    bidirektional nach persistierter COMMIT-Entscheidung; PREPARE bleibt im Participant-WAL,
    die Phase-2-Abschlussmarke fehlt bis zur Heilung und anschließenden WAL-Recovery. Die
    4-Test-CMake/CTest-Suite bestand einmal plus 3 Wiederholungen. Weitere asymmetrische Fehler,
    Latenz, Reordering, Duplikate und 3–5-Knoten-Chaos sind weiterhin offen.
  - Asymmetrischer ACK-Verlust am 2026-11-22: ein weiterer Testfall leitet Coordinator-Requests
    weiter und verwirft Participant-Response-Bytes. Der Participant-WAL enthält COMMIT, während
    die Coordinator-Abschlussmarke fehlt; nach Heilung und Recovery aus dem Coordinator-WAL
    bleibt der Participant-COMMIT idempotent bei genau einem WAL-Eintrag. Der 5-Test-CMake/CTest-
    Lauf bestand einmal und in 3 Wiederholungen (Linux Debug, strikte Core-Abhängigkeiten).
  - Sechster Asymmetrie-Fall am 2026-11-22: nach der dauerhaften Coordinator-COMMIT-Entscheidung
    verwirft ein transparenter Proxy Coordinator-to-Participant-TLS-Request-Bytes. Der Lauf endet
    mit `FAILURE`, ohne Coordinator-Complete-Record, und Participant B bleibt PREPARED ohne
    COMMIT-Record. Nach Heilung und frischer Recovery erhält Participant B genau einen COMMIT;
    der Coordinator-Complete-Record ist genau einer. Der 6-Test-CMake/CTest-Lauf ist Linux-
    real-gRPC/mTLS-Evidenz und schließt DIS-10 nicht. Weitere asymmetrische Fehler,
    Reordering/Duplikate, 3–5 Knoten und Plattform-/CI-Gates bleiben offen.
  - Siebter Fall ergänzt deterministische Coordinator-to-Participant-Latenz nach persistierter
    COMMIT-Entscheidung und validiert Recovery; achter Fall crasht den dritten Teilnehmer in
    einer Transaktion mit drei Teilnehmern, startet ihn neu und prüft die konsistente terminale
    WAL-Historie (eine COMMIT-Entscheidung/Completion, ein COMMIT je Teilnehmer, kein ABORT).
    Der einfache WAL-Konsistenzcheck ist kein Serialisierbarkeits-/Linearizierbarkeits-Checker;
    der 8-Test-Lauf bestand in Ubuntu 24.04/Linux Debug einmal und in drei CTest-Wiederholungen.
  - Der Wave-B-Workflow baut das Linux-Real-Transport-Target und archiviert drei CTest-
    Wiederholungen; ein erfolgreicher Workflow-Lauf samt Artefakt ist noch nicht nachgewiesen.
  - TCP erhält die Byte-Reihenfolge; rohe Reordering-/Duplikat-Injektion verschlüsselter TLS-Bytes
    würde den Stream beschädigen und bildet keine anwendungsseitig beobachtbare RPC-Semantik ab.
    Semantische COMMIT-Wiederholung nach ACK-Verlust ist bereits durch Recovery-Fälle mit
    dedupliziertem Participant-WAL abgedeckt.
  - Offen: zusätzliche asymmetrische Fehlerzeitpunkte, Coordinator-/Participant-/Leader-Crashmatrix
    über das begrenzte 3-Teilnehmer-Szenario hinaus, echter konkurrierender History-Checker,
    72-Stunden-Nightly, Windows/macOS-Prozess-Transportabdeckung und Release-/CI-Artefakte.
  - `tests/test_auto_failover_recovery.cpp` ist separat registriert, deckt jedoch nur In-Memory-Mocks ab; die
    geforderte 3–5-Knoten-Multi-Prozess-Chaos-Suite bleibt offen.
  - Exit: 72 h nightly ohne Invariantenverletzung; Ergebnisse als Release-Evidenz.
- [x] **P0 · DIS-07 – Spare-Aktivierung auswerten** (Target: M2) – Fehlgeschlagene Aktivierung blockiert den Failover-Abschluss,
  setzt `DEGRADED`, liefert einen Fehlerstatus, emittiert `FAILOVER_DEGRADED` über registrierte Callbacks und schreibt
  das Diagnose-Log `SPARE_ACTIVATION_FAILED`; `degraded_failovers` erfasst den Ausgang. Kein Prometheus-Export wird
  behauptet; externe Sichtbarkeit hängt von der Deployment-Integration ab. Evidenz: `tests/failover/test_failover_wave_a_fencing.cpp`
  (`FO_Promote_05` bis `_08`), registriert als `test_failover_wave_a_fencing_failover_FocusedTests`.
- [ ] **P1 · DIS-15 – Quorum/Fencing im Modul erzwingen** (Target: M2) – Epoch-/Term-Fencing-Token bei jeder
  schreibenden Failover-Aktion; Ablehnung ohne Quorum; nicht nur Deployment-Empfehlung.
- [ ] **P1 · DIS-04 – Release-Gate Shard-gRPC** (Target: M1) – Produktionspakete müssen mit `THEMIS_HAS_SHARD_GRPC=1`
  gebaut sein; CI-Check im Release-Workflow.
- [ ] **P1 · DIS-13 – Persistenter Manifest-Store** (Target: M2) – `src/distributed_tensor/src/manifest_store.cpp` auf
  RocksDB-Column-Family; Neustart-Test.
- [ ] **P1 · DIS-14 – Nicht-blockierender Retry-Backoff** (Target: M2) – `sleep_for` in `src/sharding/stream_protocol.cpp:135`
  durch Timer/Scheduler ersetzen; Worker-Pool begrenzen; Test mit 1.000 parallelen fehlschlagenden Transfers ohne Thread-Erschöpfung.
- [ ] **P0 · STO-03 – Redis-Cache-Koordinator fail-closed** (Target: M2) – konfiguriert, aber ohne hiredis gebaut →
  Startfehler bzw. `readiness=false` (`src/cache/redis_cache_coordinator.cpp:105-111`).
- [ ] **P1 · STO-02 – Kafka-CDC-Readiness** (Target: M2) – konfigurierter Kafka-Producer ohne librdkafka →
  `readiness=false` statt nur `false`-Rückgaben (`src/cdc/kafka_cdc_producer.cpp:15-29`).

### WS3 – Security Defaults & Supply Chain

- [~] **P0 · SEC-14 – Kanonische Produktionsmodus-Erkennung + mTLS fail-closed** (Target: M1 · 2026-12-15)
  - [x] 2026-11-01: Direktparser zentralisiert, unbekannte Produktionsmodus-Werte fail-closed behandelt und
    Shard-RPC-Client/Server im Produktionsmodus gegen mTLS- oder Zertifikatsprüfungs-Bypass abgesichert.
  - [x] 2026-11-01: Windows-Debug-Build mit aktivem Shard-gRPC; 52 Produktionsmodus-/mTLS-Tests bestanden.
  - [ ] Linux-/Release-Build und Release-Gate-Evidenz ergänzen (Target: M1).
  - Alle Direktparser von `THEMIS_PRODUCTION_MODE`/`THEMIS_ENVIRONMENT` (`src/sharding/shard_rpc_server.cpp:330`,
    `src/sharding/shard_rpc_client.cpp:224-226`, `src/storage/storage_engine.cpp:39-50`,
    `src/storage/ggml_tensor_bridge.cpp:51`, `src/security/timestamp_authority.cpp:52`,
    `include/security/transport_security_checker.h:37-65`, `include/security/hsm_security_checker.h:38`) durch
    `themis::core::ProductionMode::isEnabled()` ersetzen; Werteliste case-insensitiv und dokumentiert
    (`1|true|yes|on|production` bzw. `THEMIS_ENVIRONMENT=production|prod`); unbekannte Werte → Startfehler.
  - `shard_rpc_server.cpp:320-326`: Zertifikats-/Key-Ladefehler bei aktiviertem mTLS → Exception (kein Insecure-Fallback).
  - Tests: Parametrisierter Test über alle Werteformen; Test „mTLS aktiviert + fehlende Datei → Start schlägt fehl“.
- [~] **P0 · SEC-01 – Signaturpflicht für Update-Manifeste** (Target: M1 · 2026-12-15)
  - [x] 2026-11-01: Manifestprüfung lehnt fehlenden Verifier, Signatur oder Zertifikat, Hash-Abweichung sowie untrusted/abgelaufene Zertifikate ab; Zertifikatskette muss im System-Trust-Store verankert und der Issuer in der Verifier-Policy erlaubt sein.
  - [x] 2026-11-01: Signatur-Cache und `verify_signatures=false` können die Prüfung nicht umgehen; unsignierte Overrides werden bewusst nicht unterstützt.
  - [x] 2026-11-01: Windows-Debug-Regressionstests 7/7 bestanden (fehlender Verifier, unsignierte Manifeste trotz gecachtem Erfolg, fehlendes Zertifikat, gültige Selbstsignatur/unbekannter Trust-Anchor, abgelaufenes Zertifikat, manipulierte Payload, `verify_signatures=false`).
  - [!] Gesamtes Production-Updates-Target: 124/125 bestanden; der unabhängige `DeltaUpdateEngineTest.GenerateApplyPatch_ZstdDict_SmallDiff` erwartet ZSTD-Kompression, während der Runtime-Fallback 4.112 Bytes statt <4.096 Bytes erzeugt.
  - [ ] Linux-/Release-Build und Release-Gate-Evidenz ergänzen (Target: M1).
- [ ] **P0 · DIS-02 – gRPC-Admin-Listener absichern** (Target: M1 · 2026-12-15) – `src/rpc_grpc/grpc_plugin.cpp:216-228`, `:743-747`:
  Admin-Listener nutzt dieselben TLS/mTLS-Credentials wie der Hauptlistener; Plaintext nur bei Bind auf Loopback;
  Produktionsmodus verweigert Plaintext auf Nicht-Loopback. Test je Kombination.
- [ ] **P0 · DIS-01 – QUIC `verify_tls=false` im Produktionsmodus verbieten** (Target: M1 · 2026-12-15) –
  `src/network/quic_server.cpp:809-814` über `ProductionMode::enforce(...)`; Test.
- [ ] **P0 · SEC-13 – AuthZ-Routenmatrix** (Target: M1 · 2027-02-28)
  - Test enumeriert alle registrierten HTTP-, gRPC-, WebSocket- und GraphQL-Routen aus den Router-Registries und prüft je
    Route: unauthentifiziert → 401, fremder Mandant → 403, fehlender Scope → 403, berechtigter Aufruf → 2xx.
  - Neue Route ohne Policy-Eintrag lässt den Test fehlschlagen (Allowlist nur für explizit öffentliche Endpunkte).
- [ ] **P1 · SEC-05/06/07 – Sichere Beispielkonfigurationen** (Target: M0 · 2026-11-20) – `config/timestamp_authority.yaml:64`
  `verify_tsa_cert: true`; `config/raid_entity_config.example.yaml:172`, `:185` und `config/replication.example.yaml:6`
  auf `127.0.0.1` bzw. TLS an; Config-Lint in CI, der `0.0.0.0` ohne TLS in `config/**` ablehnt.
- [ ] **P1 · SEC-09 – CSPRNG für SAML-Request-IDs** (Target: M1) – `src/auth/saml_authenticator.cpp:122`: `RAND_bytes` mit ≥ 128 Bit.
- [ ] **P1 · SEC-12 – JWT-Sicherheitstests reaktivieren** (Target: M1) – `tests/security/test_jwt_security.cpp.skip` lauffähig machen und registrieren.
- [ ] **P1 · SEC-03 – Keystore-Ausfall unterscheidbar** (Target: M1) – `src/server/auth_middleware.cpp:505-518`: Validator-/Keystore-Fehler → 503 +
  Metrik, ungültiges Token → 401; weiterhin fail-closed.
- [ ] **P1 · SEC-02 – Signer-Pinning für Plugins** (Target: M1) – `src/plugins/plugin_manager.cpp:1545-1560`; Negativtest unbekannter Signer.
- [ ] **P1 · OPS-11 – Base-Image per Digest pinnen** (Target: M1) – `Dockerfile:18`, `:181`; automatisierte Digest-Updates.
- [ ] **P1 · Pentest-Retest** (Target: M3 · 2027-05-31) – nach Abschluss aller WS3-P0-Items; Exit: 0 Critical, 0 High.
- [ ] **P2 · SEC-04 – Pfad-Redaction in Audit-Logger-Fehlern** (Target: M3) – `src/utils/audit_logger.cpp:231-238`.
- [ ] **P2 · SEC-08 – Jitter-Helper ohne `std::rand()`** (Target: M3) – `src/auth/ldap_connection_pool.cpp:213`.

### WS4 – Query Resource Limits

- [ ] **P0 · QRY-01 – LIMIT-Pushdown und Speicherbudget** (Target: M1 · 2027-02-28)
  - `src/query/vectorized_execution.cpp:274-277`: `LIMIT`/`TOP-K` beendet die Pipeline frühzeitig (Early Termination);
    Speicherbudget je Query (`query.max_memory_bytes`, Default konfigurierbar) mit Abbruchfehler `QueryMemoryLimitExceeded`.
  - Fehlerfälle: Budget überschritten, `LIMIT` mit `SORT` (Top-K-Heap statt Vollsortierung), Abbruch durch Client.
  - Tests: `LIMIT 10` über 10 Mio. Zeilen; Budget-Überschreitung liefert definierten Fehler, kein OOM.
  - Performance: Peak-Speicher und Latenz von `LIMIT 10` unabhängig von Tabellengröße (10 Mio. Zeilen ≤ 2× des
    Werts bei 100.000 Zeilen); Baseline vorher messen.
- [ ] **P1 · QRY-02 – Zahlen-Parsing ohne Exceptions** (Target: M1) – `src/aql/aql_query_validator.cpp:261-262`:
  `std::from_chars` mit Bereichsprüfung → Validierungsfehler (HTTP 400); Fuzz-Seed mit überlangen Zahlen.
- [ ] **P1 · QRY-07 – Search-Timeouts erzwingen** (Target: M1) – Deadline/Cancellation durch alle Search-Pfade
  (`src/search/ROADMAP.md:266-284`); Test mit künstlich langsamer Teilsuche.
- [ ] **P0 · QRY-03 – Semantischer Fallback sichtbar** (Target: M1) – `src/tensor/utr_converter.cpp:294-306`: Ergebnis trägt
  `degraded=lexical_fallback`; Konfiguration `semantic.require_embedding_provider=true` lässt die Anfrage fehlschlagen.
- [ ] **P1 · QRY-04 – GPU-Fallback im Ergebnisstatus** (Target: M1) – `src/gpu/gpu_module.cpp:137-145`: Ausführungsmodus
  (`gpu|cpu_fallback`) im Ergebnis und in Metriken.
- [ ] **P2 · QRY-05/06 – AQL-Skips abbauen** (Target: M3) – `tests/aql/test_aql_with_clause.cpp:272`, `tests/aql/test_aql_shortest_path.cpp:30`.
- [ ] **P2 · QRY-08 – Optimizer-Statistiken** (Target: M3) – fehlende Statistiken in `src/query/query_optimizer.cpp:490-552`
  im Explain-Plan kennzeichnen; Statistik-Sammlung beim Start/Analyze.

### WS5 – AI/LLM Preview-Hardening

Ziel: AI/LLM-Module bleiben „Preview“, werden aber sicher betreibbar (keine stillen Stubs, harte Ressourcengrenzen).

- [ ] **P0 (Preview-Exit) · AI-03 – Real-Model-CI-Lane** (Target: M3 · 2027-04-30)
  - Gepinnte Mini-Modelle (Hash im Repo, Download über `.github/workflows/reusable-llm-model-cache.yml`) für
    Embedding, Generierung, LoRA, Vision, Whisper.
  - Exit: `GTEST_SKIP` in `tests/llm` von 258 auf ≤ 20, jeder verbleibende Skip mit Begründung und Ticket.
  - Code-Fortschritt (2026-11-19, noch kein Ausführungsbeleg): TinyLlama wird in der wiederverwendbaren Cache-Lane
    gegen den gepinnten SHA256 geprüft; die geplante LLM-Lane führt Embedding-, Generierungs-, LoRA-Fehlerpfad-,
    Wiki-Phase-B-, RAG- und AdaLoRA-Tests mit Skip-Budget 0 aus und muss bei übersprungenen Jobs fehlschlagen.
  - Weiter offen: erfolgreicher CI-Lauf samt Artefakt, echte Vision-/Whisper-Modelltests und der Modul-Gesamtwert
    `GTEST_SKIP` ≤ 20. Diese Änderung allein schließt AI-03 nicht.
- [ ] **P1 · AI-07/AI-09 – Zentrale Admission Control** (Target: M3 · 2027-04-30)
  - Managed inference now has explicit model routing, a process-wide non-queueing cap of 8 active calls,
    a 4096 generated-token limit, a combined 1 MiB input byte limit, and overload HTTP 429 with `Retry-After: 1`.
    Per-model/GPU-memory-aware admission is not implemented; focused validation remains pending.
  - Remaining evidence: validate overload and input-boundary behavior; do not infer real-model, TSan, or hardware results.
- [ ] **P1 · SEC-10 – Stub-Backends im Produktionsmodus sperren** (Target: M1) – `src/whisper/whisper_plugin.cpp:18-29` und
  alle in `src/STUB_INVENTORY.md` gelisteten aktiven Stubs: Start im Produktionsmodus ohne echtes Backend schlägt fehl.
- [ ] **P1 · Preview-Kennzeichnung** (Target: M1) – AI-Endpunkte liefern `X-Themis-Feature-Stage: preview`; Aktivierung
  nur über explizites Feature-Flag; Doku in API-Referenz.
- [ ] **P2 · AI-10 – Vision-/LoRA-Skips beheben** (Target: M3) – `tests/llm/test_llm_vision_encoder.cpp:129`,
  `tests/llm/test_lora_adapters.cpp:319`; LoRA-Lifecycle unter TSan.
- [x] **P2 · AI-08 – Concurrency-Claim belegen oder entfernen** (Target: M0) – Unsupported 1024-request/hardware claim removed from `src/llm/THREADING.md`; process-wide cap documented without TSan or hardware evidence.
- [ ] **P2 · AI-12 – Timeout-Lücke abgleichen** (Target: M1) – `src/llm/TODO_CRITICAL_GAPS.md:107`; docs now state that synchronous `ILLMPlugin::generate()` cannot guarantee a deadline for a non-cooperative backend; timeout integration evidence remains pending.
- [x] **P2 · AI-01 – Toten Code entfernen** (Target: M0) – `SyntheticSSMStub` header/source and its benchmark removed; no SSM inference backend is provided.

### WS6 – CI Fail-Closed & Test-Evidenz

- [ ] **P0 · EVD-01 – Gültige Volllauf-Baseline** (Target: M0 · 2026-11-20)
  - Ursache: `tests/cmake/RunRegisteredTest.cmake:135-150` baut fehlende Test-Executables **während** des CTest-Laufs
    nach; parallele Tests starten konkurrierende `cmake --build`-Aufrufe im selben Build-Verzeichnis
    (`ninja: … Permission denied`).
  - Maßnahme: Alle Testziele vor `ctest` bauen; On-Demand-Build in CI deaktivieren (fehlendes Executable =
    sofortiger, eindeutiger Fehler) oder per Lock serialisieren.
  - Exit: CTest-Volllauf auf `develop` für Linux und Windows mit JUnit-Artefakt, Pass-Rate je Modul, Retention ≥ 90 Tage,
    eingebunden in `.github/workflows/release-evidence-chain.yml`.
- [ ] **P0 · OPS-01/CI-03 – Gate-Workflows ohne stille Toleranz** (Target: M0 · 2026-11-20)
  - `continue-on-error: true` und `|| true` aus allen `gate-*`-Workflows, `compliance-governance-gates.yml:157` und
    `wiki-pr-gate.yml:83` entfernen; verbleibende Ausnahmen nur über Allowlist mit Begründung und Ablaufdatum.
  - Guard-Check: fail-open lint tracks gate workflows and their local reusable dependencies; unclassified or stale tolerances fail.
  - [x] 2026-11-19: Der Guard verfolgt lokale reusable-Aufrufe transitiv und klassifiziert die dort vorhandenen Toleranzen exakt; aktivierte `*_continue_on_error`-Inputs in Gate-Aufrufen werden erfasst.
  - [ ] Gesamtstatus bleibt offen bis die betroffenen Workflow-Gates nachweislich laufen und Maintainer-Freigabe vorliegt; fehlende Plattform-Artefakte gelten nicht als PASS.
- [ ] **P0 · OPS-06 – Sanitizer auf PRs** (Target: M1 · 2026-12-31) – ASan+UBSan-Lane für geänderte Kernmodule
  (`storage`, `transaction`, `query`, `aql`, `server`, `auth`, `network`, `sharding`); Laufzeitbudget ≤ 45 min;
  TSan nightly für `transaction`, `sharding`, `llm`.
- [~] **P0 · TST-03 – Test-Registrierung fail-closed** (Target: M0 · 2026-11-20)
  - [x] 2026-11-01: Der WAL-Fokustest-Pfad zeigt auf `tests/test_wal_storage.cpp`; `tests/test_auto_failover_recovery.cpp`
    ist separat als Legacy-/Mock-Szenario registriert. Der Windows-Debug-Build und alle 9 Tests bestanden; der Test
    ist ausdrücklich nicht release-kritisch und belegt kein reales Multi-Node-Failover.
  - [x] Der WAL-Test wird ohne `if(EXISTS ...)` konfiguriert; fehlende Quelle bricht die CMake-Konfiguration statt
    die Registrierung stillschweigend auszulassen.
  - [x] Das fokussierte Produktionsmodus-/Shard-mTLS-Testtarget wird ebenfalls ohne dateibasierte
    `if(EXISTS ...)`-Auslassung konfiguriert; CMake meldet fehlende Quelldateien als Konfigurationsfehler.
  - [x] Inkrementeller PR-Gate: geänderte `tests/**/test_*.cpp` müssen in der CMake-Compilation-Database des
    Release-Critical-Builds vorkommen; ein fehlender Eintrag lässt den Gate fehlschlagen. Positive/negative
    Unit-Tests, ein lokaler Compilation-Database-Lauf und Workflow-Lint bestehen; der erste Ubuntu-PR-Lauf steht aus.
  - [x] PR-Gate lehnt neu hinzugefügte explizite `if(EXISTS … test_*.cpp)`- und
    `elseif(... EXISTS ... test_*.cpp)`-CMake-Guards ab, die eine Testregistrierung still auslassen könnten.
  - [x] PR-Gate erkennt entfernte CMake-Quellreferenzen, wenn die Testdatei weiter existiert, aber im
    Release-Critical-Build nicht mehr kompiliert wird; ersetzte Registrierungen bleiben erlaubt, sofern die
    Quelle weiterhin in der Compilation-Database erscheint.
  - [x] Für geänderte Testquellen werden mehrere eigenständige CMake-Testtargets abgewiesen. Das notwendige
    Sammelziel `themis_tests` ist davon ausgenommen; ein Fokus-Testtarget zusätzlich zum Sammelziel ist erlaubt.
  - [x] PR-CI veröffentlicht ein JSON-Inventar aller `test_*.cpp`-Quellen für das konfigurierte Build-Profil,
    einschließlich Status `compiled`/`not_in_profile` und der erkannten CMake-Targets.
  - [x] 2026-11-01: Redundante Geo-Autofocused-Registrierung entfernt. Das Windows-Debug-Profil enthält danach
    41 kompilierte Geo-Testquellen ohne doppelte fokussierte Geo-Targets; `module_geo_test_geo_3d_functions_focused`
    baut erfolgreich und `test_geo_3d_functions_GeoFocusedTests` besteht.
  - [x] 2026-11-01: Sechs Timeseries-Quellen verwenden nur noch ihr kanonisches `_focused`-Buildtarget; die
    `AutoFocusedTests`-CTest-Namen und das `autogen`-Label bleiben auf demselben Binary registriert. Der Windows-
    Debug-Build und beide Retention-CTest-Registrierungen bestehen.
  - [x] 2026-11-01: 19 Process-Quellen verwenden nur noch ihr kanonisches `_focused`-Buildtarget; die
    release-critical `AutoFocusedTests`-CTest-Namen und das `autogen`-Label bleiben erhalten.
    `module_process_test_process_aris_xml_focused` baut; beide Process-CTest-Einträge bestehen. Die Windows-
    Runtime-DLL-Synchronisierung wird am gemeinsam genutzten Target nur einmal angehängt.
  - [x] 2026-11-03: Neun bisherige Process-Fokustargets verwenden jetzt die kanonischen Modul-Executables;
    zehn mehrfach kompilierte Testquellen entfallen. Legacy-Buildtarget- und CTest-Namen, Labels, Timeouts sowie
    der Process-Graph-Filter bleiben erhalten. Beide Windows-Debug-Profile konfigurieren; alle neun Legacy-CTest-
    Namen sind registriert, und die Compilation Database enthält keine mehrfach registrierte Process-Quelle.
    Der nicht-modulare Zähler sinkt von 97 auf 87 (86 Testquellen plus der absichtlich gemeinsam genutzte GTest-
    Runner). Das Inventar umfasst 4.229 Quellen; 2.818/2.908 sind im modularen/nicht-modularen Profil kompiliert,
    und die Profilunion lässt 1.320 Quellen außerhalb beider Profile. Laufzeit-Build und -Tests sind noch nicht
    bestätigt.
  - [x] 2026-11-03: Sieben Replication-Quellen verwenden ihre kanonischen Modul-Executables statt neun separater
    Root-Buildtargets. Legacy-CTest-Namen, Labels, Timeouts, `RUN_SERIAL`, der Multi-Tier-Filter sowie die
    `async_wal_shipper.cpp`-/Topology-Handler-Quellen, Zstd-Linkage und HTTP-Server-Compile-Definition bleiben
    erhalten. Beide Windows-Debug-Profile konfigurieren; alle neun Legacy-CTest-Namen sind registriert, und
    keine `tests/replication/`-Quelle ist mehrfach fokussiert registriert. Der nicht-modulare Zähler sinkt von
    87 auf 80 (79 Testquellen plus der absichtlich gemeinsam genutzte GTest-Runner). Laufzeit-Build und -Tests
    sind noch nicht bestätigt.
  - [x] 2026-11-03: Sieben Transaction-Testquellen (Manager Comprehensive, OCC, Bulk, Retry, Timeout, SSI und
    Semantic Advisor) verwenden ihre kanonischen Modul-Executables. Die sechs Legacy-CTest-Namen sowie
    Buildtarget-Aliase bleiben erhalten; die Semantic-Advisor-Falltests werden weiter nach dem Build entdeckt.
    Beide Windows-Debug-Profile konfigurieren, und keine `tests/transaction/`-Quelle ist mehrfach fokussiert
    registriert. Der nicht-modulare Zähler sinkt von 80 auf 73 (72 Testquellen plus der absichtlich gemeinsam
    genutzte GTest-Runner). Laufzeit-Build und -Tests sind noch nicht bestätigt.
  - [x] 2026-11-03: Sieben RAG-Testquellen verwenden ihre kanonischen Modul-Executables. Legacy-Buildtarget-
    Aliase, sechs bisherige explizite CTest-Namen, Labels, Timeouts, der Ingestion-Bridge-Filter und Self-RAG-
    Falltest-Discovery bleiben erhalten; die notwendigen Produktionsquellen, Linkabhängigkeiten und
    Compile-Definitionen sind auf die Modultargets übertragen. Beide Windows-Debug-Profile konfigurieren, und
    keine `tests/rag/`-Quelle ist mehrfach fokussiert registriert. Der nicht-modulare Zähler sinkt von 73 auf 66
    (65 Testquellen plus der absichtlich gemeinsam genutzte GTest-Runner). Laufzeit-Build und -Tests sind noch
    nicht bestätigt.
  - [x] 2026-11-03: Sieben Training-Testquellen verwenden ihre kanonischen Modul-Executables. Legacy-Buildtarget-
    Aliase, beide ModalityParser-/TrainingConvergence-CTest-Namen, alle weiteren fokussierten CTest-Namen und
    Multi-Task-LoRA-Falltest-Discovery bleiben erhalten; notwendige Trainingsquellen und RocksDB/OpenSSL/JSON-
    Linkabhängigkeiten wurden übertragen. Beide Windows-Debug-Profile konfigurieren, und keine
    `tests/training/`-Quelle ist mehrfach fokussiert registriert. Der nicht-modulare Zähler sinkt von 66 auf 59
    (58 Testquellen plus der absichtlich gemeinsam genutzte GTest-Runner). Laufzeit-Build und -Tests sind noch
    nicht bestätigt.
  - [x] 2026-11-03: Sechs Storage-Testquellen verwenden ihre kanonischen Modul-Executables. Legacy-Buildtarget-
    Aliase, fünf explizite CTest-Namen und StorageLayoutAdvisor-Falltest-Discovery bleiben erhalten; MVCC-
    Temporalquellen, Parquet-Exporter/Columnarquellen sowie RocksDB-, LZ4- und Snappy-Linkbedingungen wurden
    übertragen. Beide Windows-Debug-Profile konfigurieren, und keine `tests/storage/`-Quelle ist mehrfach
    fokussiert registriert. Der nicht-modulare Zähler sinkt von 59 auf 53 (52 Testquellen plus der absichtlich
    gemeinsam genutzte GTest-Runner). Laufzeit-Build und -Tests sind noch nicht bestätigt.
  - [x] 2026-11-03: Vier Chaos-Testquellen verwenden ihre kanonischen Modul-Executables. Die vier Legacy-CTest-
    Namen, Labels, Timeouts und der HTTP-Server-Feature-Gate für Network/Scheduler bleiben erhalten; die
    Chaos-Framework-Quelle und `themis_network`-Linkabhängigkeit sind auf die Modultargets übertragen. Beide
    Windows-Debug-Profile konfigurieren, und keine `tests/chaos/`-Quelle ist mehrfach fokussiert registriert.
    Der nicht-modulare Zähler sinkt von 53 auf 49 (48 Testquellen plus der absichtlich gemeinsam genutzte
    GTest-Runner). Laufzeit-Build und -Tests sind noch nicht bestätigt.
  - [x] 2026-11-03: Drei PostgreSQL-Importer-Testquellen (FK, Security Audit und Streaming) verwenden nur noch
    ihre kanonischen Modul-Executables. Der Legacy-Buildtarget- und Aggregat-CTest-Name bleiben erhalten; der
    Aggregattest führt genau diese drei Modul-CTest-Einträge aus, während die übrigen Importer-Suiten
    unberührt bleiben. Beide Windows-Debug-Profile konfigurieren, und keine `tests/postgres/`-Quelle ist
    mehrfach fokussiert registriert. Der nicht-modulare Zähler sinkt von 49 auf 46 (45 Testquellen plus der
    absichtlich gemeinsam genutzte GTest-Runner). Laufzeit-Build und -Tests sind noch nicht bestätigt.
  - [x] 2026-11-03: Zwei AQL-Testquellen (GraphQL-Integration und Multi-Statement-Transaction) verwenden ihre
    kanonischen Modul-Executables. Legacy-Buildtarget- und CTest-Namen, Labels, Timeouts sowie die expliziten
    RocksDB-/nlohmann-json-Abhängigkeiten bleiben erhalten. Beide Windows-Debug-Profile konfigurieren, und
    keine `tests/aql/`-Quelle ist mehrfach fokussiert registriert. Der nicht-modulare Zähler sinkt von 46 auf
    44 (43 Testquellen plus der absichtlich gemeinsam genutzte GTest-Runner). Laufzeit-Build und -Tests sind
    noch nicht bestätigt.
  - [x] 2026-11-01: 15 Temporal-Quellen verwenden nur noch ihr kanonisches `_focused`-Buildtarget; die
    `AutoFocusedTests`-CTest-Namen und `temporal`-/`autogen`-Labels bleiben erhalten.
    `module_temporal_test_temporal_aggregation_focused` baut und beide Temporal-CTest-Einträge bestehen.
  - [x] 2026-11-01: 13 Storage-Quellen verwenden nur noch ihr kanonisches `_focused`-Buildtarget; der
    `AutoFocusedTests`-Alias und seine Labels bleiben erhalten. Target-spezifische Produktionsquellen und
    release-critical Labels verbleiben am gemeinsamen Executable; gezielter Build und beide CTest-Einträge
    bestehen.
  - [x] 2026-11-01: 12 Index-Quellen verwenden nur noch ihr kanonisches `_focused`-Buildtarget; die
    `AutoFocusedTests`-Namen, `index`-/`autogen`-Labels und strikten Build-Flags bleiben erhalten.
    `module_index_test_index_manager_di_focused` baut und beide CTest-Einträge bestehen.
  - [x] 2026-11-01: 12 Transaction-Quellen verwenden nur noch ihr jeweiliges kanonisches Testtarget;
    `AutoFocusedTests`-Namen sowie `transaction`-/`autogen`-Labels bleiben auf demselben Binary registriert.
    Target-spezifische Quellen und Link-Konfiguration bleiben erhalten; `test_transaction_auditor_focused`
    baut und beide Auditor-CTest-Einträge bestehen.
  - [x] 2026-11-01: 10 Query-Quellen verwenden nur noch ihr kanonisches fokussiertes Testtarget; die
    `AutoFocusedTests`-Namen und `query`-/`autogen`-Labels bleiben erhalten. `test_query_engine_focused`
    baut und beide Query-Engine-CTest-Einträge bestehen.
  - [x] 2026-11-01: 10 Security-Quellen und 9 Metadata-Quellen verwenden nur noch ihr jeweiliges
    kanonisches fokussiertes Testtarget; die `AutoFocusedTests`-Namen und Modul-/`autogen`-Labels bleiben
    erhalten. Zusatzquellen am Metadata-Snapshot-Target bleiben am gemeinsamen Binary.
  - [x] 2026-11-01: Nach der Neu-Konfiguration enthält das Windows-Debug-Profil in Security und Metadata
    keine mehrfach fokussiert kompilierten Testquellen mehr; der Gesamtbestand sank von 141 auf 122.
  - [x] 2026-11-01: 7 Sharding-Quellen und 5 Performance-Quellen verwenden nur noch ihr kanonisches Testtarget;
    ihre `AutoFocusedTests`-Namen und Modul-/`autogen`-Labels bleiben erhalten. Target-spezifische Zusatzquellen
    bleiben am jeweiligen gemeinsamen Binary.
  - [x] 2026-11-01: Nach der Neu-Konfiguration enthält das Windows-Debug-Profil in Sharding und Performance
    keine mehrfach fokussiert kompilierten Testquellen mehr; der Gesamtbestand sank von 122 auf 110.
  - [x] 2026-11-01: 39 RAG-Quellen verwenden nur noch ihr kanonisches fokussiertes Testtarget; für alle
    verglichenen Paare waren die kompilierten Quelldateien identisch. `AutoFocusedTests`-Namen und
    `rag`-/`autogen`-Labels bleiben erhalten; beide Adaptive-Retrieval-CTest-Einträge bestehen.
  - [x] 2026-11-01: 66 LLM-Quellen und die drei zuvor getrennten LLM-Providerfälle verwenden für den
    zusätzlichen `AutoFocusedTests`-Namen ihr kanonisches fokussiertes Target; Testnamen und Modul-Labels
    bleiben erhalten. Die drei Provider-Sets waren zusätzliche Build-Abhängigkeiten, nicht unterschiedliche
    Testquellen oder Testfälle.
  - [x] 2026-11-01: Der Tensor-Shard-Summary-Autofocused-Name verwendet das vorhandene Phase-C-Testtarget;
    beide CTest-Namen bleiben bestehen. Der dateibasierte `if(EXISTS ...)`-Guard für die Testquelle wurde
    entfernt, sodass eine fehlende Datei die CMake-Konfiguration fehlschlagen lässt. Der gezielte Windows-
    Debug-Build aller vier kanonischen Targets und alle fünf betroffenen CTest-Namen bestehen.
  - [x] 2026-11-01: Das modulare Windows-Debug-Profil inventarisiert 4.228 Testquellen: 2.817 kompiliert,
    1.411 außerhalb des Profils, 0 ohne auflösbares Target. Die einzige Mehrfachregistrierung ist der
    gemeinsame Runner `test_gtest_main.cpp`, der absichtlich in separaten Test-Executables kompiliert wird.
  - [x] 2026-11-01: Der Profilvergleich bestätigt 2.908 kompilierte Quellen im nicht-modularen
    Windows-Debug-Profil, 1.320 außerhalb des Profils und 0 ohne auflösbares Target. Es ergänzt 91 Quellen
    gegenüber dem modularen Profil; die modulare Abdeckung ist enthalten.
  - [x] 2026-11-01: Die adaptiven Quellen `test_adaptive_flush_controller.cpp` und
    `test_adaptive_shard_router.cpp` verwenden im nicht-modularen Profil jeweils nur ihr kanonisches Modul-
    Target. Bestehende CTest-Aliase, Domain-Filter und GTest-Einzeltest-Discovery bleiben erhalten;
    das Inventar sinkt von 394 auf 392 Mehrfachregistrierungen.
  - [x] 2026-11-01: 22 GPU-Quellen verwenden im nicht-modularen Profil nur noch ihr kanonisches Modul-
    Target. Die bestehenden GTest-Einzeltest-Discovery-Einträge werden dort am Target im Modulverzeichnis
    erzeugt; das Inventar sinkt von 392 auf 370 Mehrfachregistrierungen.
  - [x] 2026-11-01: 13 Exporter-Testquellen verwenden im nicht-modularen Profil nur noch ihr kanonisches
    Modul-Target. Die bisherigen Root-Targets kompilieren dieselben 16 Exporter-Dateien und `logger.cpp` redundant,
    obwohl sie bereits von `themis_core` gebaut werden; alte CTest-Namen, Labels und Build-Target-Namen
    bleiben als Aliase erhalten. Das Inventar sinkt von 370 auf 357 Mehrfachregistrierungen.
  - [x] 2026-11-01: `tests/gpu/test_gpu_graph_traversal.cpp` verwendet das kanonische GPU-Modulziel;
    die vier zusätzlich vom Root-Target kompilierten Graph-Produktionsdateien liegen bereits in `themis_core`.
    `GPUGraphTraversalTests` und `test_gpu_graph_traversal` bleiben als CTest- bzw. Build-Aliase erhalten.
    Der CPU-Fallback-Test wird auch bei `THEMIS_ENABLE_GPU=OFF` registriert; beide GPU-Profile konfigurieren,
    der Off-Modus wurde im vorhandenen nicht-modularen Build-Tree verifiziert. Das Inventar sinkt auf 356.
  - [x] 2026-11-01: `tests/graph/test_gpu_traversal.cpp` wird nur noch durch das kanonische Graph-Modul-Target
    kompiliert. GTest-Einzelfallnamen und das bisherige Build-Target `test_gpu_traversal_focused` bleiben erhalten;
    das Inventar sinkt von 356 auf 355 Mehrfachregistrierungen.
  - [x] 2026-11-01: `tests/analytics/test_process_discovery_conformance.cpp` wird nur noch durch das kanonische
    Analytics-Modul-Target kompiliert. Der bisherige CTest-Name samt Labels und Timeout sowie das Build-Target
    `test_process_discovery_conformance_focused` bleiben als Aliase erhalten; das Inventar sinkt von 355 auf 354.
  - [x] 2026-11-01: `tests/advanced/test_advanced_cache_manager.cpp` wird nur noch durch das kanonische Advanced-
    Modul-Target kompiliert. Der aggregierte CTest-Name samt Labels/Timeout, die bisherigen Einzeltestnamen und beide
    Build-Targets (`test_advanced_cache_manager`, `test_advanced_cache_manager_focused`) bleiben erhalten;
    `src/performance/advanced_cache_manager.cpp` stammt bereits aus `themis_core`. Das Inventar sinkt von 354 auf 353.
  - [x] 2026-11-01: `tests/analytics/test_process_mining_llm.cpp` wird nur noch durch das kanonische Analytics-Modul-
    Target kompiliert. `ProcessMiningPatternFocusedTests`, seine Labels/Timeout und das bisherige Build-Target
    `test_process_mining_pattern_focused` bleiben als Aliase erhalten; die Quelldatei gehört laut CMake-Vertrag zum
    Process-Mining-Pattern-Test und benötigt keine LLM-spezifischen Abhängigkeiten. Das Inventar sinkt von 353 auf 352.
  - [x] 2026-11-01: `tests/analytics/test_anomaly_detection.cpp` und `test_cep_engine.cpp` verwenden jeweils nur noch
    ihr kanonisches Analytics-Modul-Target. Die bisherigen Root-Build-Targets und CTest-Namen samt Labels/Timeouts
    bleiben als Aliase erhalten; beide Quellpaare einschließlich ihrer Produktionsdatei waren identisch. Das
    Inventar sinkt von 352 auf 350 Mehrfachregistrierungen.
  - [x] 2026-11-01: `tests/analytics/test_columnar_execution.cpp` wird nur noch durch das kanonische Analytics-
    Modul-Target kompiliert. Im nicht-modularen Profil bleiben `ColumnarExecutionFocusedTests` samt bisherigen
    Labels/Timeout sowie `test_columnar_execution_focused` als Aliase erhalten; beide Targets hatten dieselbe Test-
    und Produktionsquelle. Das Inventar sinkt von 350 auf 349 Mehrfachregistrierungen.
  - [x] 2026-11-01: Die Analytics-Quellen für Expert-System-Engine, Forecasting, LLM-Process-Analyzer,
    Streaming-Join und Streaming-Window verwenden nur noch ihr jeweiliges Modul-Target. Die bisherigen Root-
    CTest-Namen samt Labels/Timeouts und Build-Target-Namen bleiben im nicht-modularen Profil erhalten; für alle
    fünf Paare waren Test- und Produktionsquellen identisch. Das Inventar sinkt von 349 auf 344 Mehrfachregistrierungen.
  - [x] 2026-11-01: `tests/api/test_api_interfaces.cpp` und `test_api_key_authenticator.cpp` verwenden je ein
    kanonisches API-Modul-Target. Root-CTest-Namen samt Labels/Timeouts sowie Build-Target-Namen bleiben als Aliase
    erhalten; die Quellmengen waren identisch. Die OpenSSL-Linkabhängigkeiten des API-Key-Tests bleiben explizit am
    Modul-Target. Das nicht-modulare Inventar sinkt von 344 auf 342 Mehrfachregistrierungen.
  - [x] 2026-11-01: `tests/base/test_base_entity_focused.cpp` und
    `test_base_reload_regression.cpp` verwenden je ein kanonisches Base-Modul-Target. Die bisherigen Root-Build-
    Targets sowie CTest-Namen, Labels und Timeouts bleiben als Aliase erhalten; beide Quellmengen waren identisch.
    Das nicht-modulare Inventar sinkt von 342 auf 340 Mehrfachregistrierungen.
  - [x] 2026-11-01: 25 Ingestion-Testquellen unter `tests/ingestion/` verwenden nur noch ihr kanonisches Modul-
    Target. Root-Build-Target-Namen, CTest-Namen, Labels und Timeouts bleiben erhalten; Quellen außerhalb des
    Ingestion-Moduls behalten ihre bisherigen eigenständigen Targets. Das Builtin-Steps-Target behält seine
    explizite `CXX_STANDARD 17`-Einstellung und nlohmann-json-Abhängigkeit. Das nicht-modulare Inventar sinkt
    von 340 auf 315 Mehrfachregistrierungen.
  - [x] 2026-11-02: 16 Network-Testquellen mit identischen Test-/Produktionsquellen verwenden nur noch ihr
    kanonisches Network-Modul-Target. Root-Build-Target-Namen, CTest-Namen, Labels und Timeouts bleiben erhalten;
    `tests/network/test_websocket_cdc.cpp` bleibt wegen zusätzlicher Root-Produktionsquellen separat registriert.
    Das nicht-modulare Inventar sinkt von 315 auf 299 Mehrfachregistrierungen.
  - [x] 2026-11-02: Sechs CDC-Testquellen (`changefeed_core`, `pause_control`, `backpressure_signal`, `fan_in`,
    `event_schema`, `delivery_guarantee_config`) verwenden nur noch ihr kanonisches Modul-Target. Root-Build-
    Target-Namen, CTest-Namen, Labels, Timeouts sowie explizite nlohmann-json-/RocksDB-Abhängigkeiten bleiben
    erhalten. Das nicht-modulare Inventar sinkt von 299 auf 293 Mehrfachregistrierungen.
  - [x] 2026-11-02: Die Plugin-Hot-Plug- und Hot-Reload-Tests verwenden ihr kanonisches Plugin-Modul-Target;
    die Legacy-Buildtarget-Namen, CTest-Namen, Labels und Timeouts bleiben erhalten. Die explizite
    `CMAKE_DL_LIBS`-Abhängigkeit bleibt auf dem Modul-Target; das nicht-modulare Inventar sinkt von 293 auf 291.
  - [x] 2026-11-02: Sechs weitere CDC-Tests (`admin`, `retention`, `changefeed_sequence_counter`,
    `consumer_group`, `kafka_producer`, `ws_handler`) verwenden ihr kanonisches CDC-Modul-Target. Die
    bedingten Produktionsquellen für HTTP-Server- und Kafka-Konfigurationen sowie nlohmann-json-/RocksDB-
    Abhängigkeiten und Windows-Runtime-DLL-Synchronisierung wurden auf das Modulziel übertragen. Legacy-
    Buildtarget-Namen und CTest-Namen, Labels und Timeouts bleiben erhalten. Beide Windows-Debug-Profile
    konfigurieren; im nicht-modularen Profil haben alle sechs Quellen genau ein fokussiertes Target und
    das Inventar sinkt von 291 auf 285. Der gezielte Build bleibt durch einen unabhängigen MSVC-Fehler im
    generierten gRPC-Header vor Erreichen der CDC-Targets blockiert.
  - [x] 2026-11-02: Sieben Auth-Tests (`anomaly_detection`, `audit_logger`, `error`, `input_validation`,
    `metrics`, `middleware`, `rate_limiter`) verwenden ihr kanonisches Auth-Modul-Target. Die expliziten
    OpenSSL-, optionale CURL- und nlohmann-json-Abhängigkeiten wurden auf die Modulziele übertragen;
    Legacy-Buildtarget-Namen und CTest-Namen, Labels und Timeouts bleiben erhalten. Compilation-Databases
    beider Windows-Debug-Profile bestätigen jeweils genau ein fokussiertes Target je geänderter Quelle;
    das nicht-modulare Inventar sinkt von 285 auf 278. Der verteilte Rate-Limiter bleibt separat, weil sein
    Legacy-Target zusätzlich `src/auth/rate_limiter_backend.cpp` kompiliert.
  - [x] 2026-11-02: Zehn Content-Tests (Metrics, Security, LanguageDetector, AudioProcessor,
    ProcessorChain, Deduplication, Policy, CON-007/CON-012, Embedding Pipeline und Toolbox Bridge) verwenden
    ihre kanonischen Modul-Executables. Die benötigten `themis_content`, nlohmann-json und OpenSSL-
    Abhängigkeiten wurden auf die Modulziele übertragen; der Toolbox-Bridge-GTest-Filter und dessen
    Arbeitsverzeichnis sowie Legacy-Buildtarget-/CTest-Namen, Labels und Timeouts bleiben erhalten.
    Zusätzlich verwendet Toolbox Primitives sein kanonisches Target für den identischen 11-Quelldatei-
    Source-Satz. Beide Windows-Debug-Profile konfigurieren; im nicht-modularen Profil haben alle elf
    Testdateien genau ein fokussiertes Target, das Inventar sinkt von 278 auf 267.
  - [x] 2026-11-02: Die RAG-Tests ReplugRetriever, MultiHopReasoner, AdaptiveRetrieval und PromptBuilder
    verwenden ihre kanonischen Modul-Executables; CTest-Namen, Labels, Timeouts und Legacy-Buildtarget-Namen
    bleiben erhalten. Die C++17-Compile-Feature-Anforderungen der zwei Phase-7-Targets sowie die direkte
    nlohmann-json-Abhängigkeit des PromptBuilder-Tests sind am Modulziel gesetzt. Beide Windows-Debug-Profile
    konfigurieren; das nicht-modulare Inventar sinkt von 267 auf 263 Mehrfachregistrierungen.
  - [x] 2026-11-02: `DatabaseDomainAutoLabeler` verwendet sein kanonisches Database-Modul-Executable;
    die Testquelle wird nun nur einmal kompiliert. Legacy-Buildtarget und CTest-Name bleiben erhalten,
    die GTest-Einzelfall-Discovery liegt im CMake-Verzeichnis des Targets. Beide Windows-Debug-Profile
    konfigurieren; Compilation-Databases zeigen genau ein fokussiertes Target für die Testquelle und
    das nicht-modulare Inventar sinkt von 263 auf 262 Mehrfachregistrierungen. CTest-Registrierung ist
    konfiguriert; Laufzeit-Build und -Tests sind wegen des separaten generierten-gRPC/MSVC-Blockers nicht bestätigt.
  - [x] 2026-11-02: `AIPluginGenerator` verwendet sein kanonisches AI-Modul-Executable. Die drei
    Observability-Quelldateien und die direkte nlohmann-json-Abhängigkeit des Legacy-Targets wurden
    übertragen; der Legacy-Buildtarget-Name sowie CTest-Name, Labels und Timeout bleiben erhalten.
    Beide Windows-Debug-Profile konfigurieren; die Compilation-Databases zeigen nur das Modulziel und
    das nicht-modulare Inventar sinkt von 262 auf 261 Mehrfachregistrierungen.
  - [x] 2026-11-02: `GraphAnalytics`, Category-B BFS und Category-B Dijkstra verwenden ihre kanonischen
    Graph-Modul-Executables. GraphAnalytics behält `fmt`, RocksDB, `THEMIS_GRAPH_EXPORTS` und die
    Windows-Runtime-DLL-Synchronisierung; die beiden Parity-Tests bewahren ihre Legacy-CTest-Namen,
    Labels und 180-s-Timeouts. Beide Windows-Debug-Profile konfigurieren; die Compilation-Databases
    zeigen je Testquelle nur das Modulziel, und das nicht-modulare Inventar sinkt von 261 auf 258.
  - [x] 2026-11-02: GraphTypeFiltering und Graph Edge Empty Fields QW-45 verwenden ihre kanonischen
    Graph-Modul-Executables. `fmt`, RocksDB, `THEMIS_GRAPH_EXPORTS` und Windows-Runtime-DLL-Synchronisierung
    bleiben bei GraphTypeFiltering erhalten; Legacy-Buildtarget, CTest-Name, Labels und Timeouts bleiben
    für beide Tests erhalten. Beide Windows-Debug-Profile konfigurieren; je Testquelle bleibt nur das
    Modulziel, und das nicht-modulare Inventar sinkt von 258 auf 256 Mehrfachregistrierungen.
  - [x] 2026-11-02: Governance Policy Hot-Reload, OPA Adapter, Policy Simulation, Compliance Time-Window
    und Review Scheduler verwenden ihre kanonischen Governance-Modul-Executables. OpenSSL-, optionale
    yaml-cpp- und direkte nlohmann-json-Abhängigkeiten wurden übertragen; Legacy-Buildtarget-Namen,
    CTest-Namen, Labels und 60-s-Timeouts bleiben erhalten. Beide Windows-Debug-Profile konfigurieren,
    und das nicht-modulare Inventar sinkt von 256 auf 251 Mehrfachregistrierungen.
  - [x] 2026-11-02: Graph Edge Encryption und Graph Index Comprehensive verwenden ihre kanonischen
    Graph-Modul-Executables. OpenSSL, fmt, RocksDB, `THEMIS_GRAPH_EXPORTS` und Windows-Runtime-DLL-
    Synchronisierung für Edge Encryption sowie die RocksDB-Abhängigkeit, Legacy-Buildtargets, CTest-
    Namen, Labels und Timeouts bleiben erhalten. Beide Windows-Debug-Profile konfigurieren; das
    nicht-modulare Inventar sinkt von 251 auf 249 Mehrfachregistrierungen.
  - [x] 2026-11-02: Cache Admin API Handler, Cache Interfaces und Cache Phase 6 Interfaces verwenden ihre
    kanonischen Cache-Modul-Executables. Die direkte RocksDB-Abhängigkeit des Admin-Tests sowie alle
    Legacy-Buildtarget-Namen, CTest-Namen, Labels und 60-s-Timeouts bleiben erhalten. Beide Windows-Debug-
    Profile konfigurieren; das nicht-modulare Inventar sinkt von 249 auf 246 Mehrfachregistrierungen.
  - [x] 2026-11-02: Cache Warmup verwendet sein kanonisches Cache-Modul-Executable mit direkter RocksDB-
    Abhängigkeit und unverändertem Legacy-Buildtarget, CTest-Namen, Labels und Timeout. TensorFingerprintGraph
    verwendet sein Graph-Modul-Executable mit denselben Produktionsquellen; die GTest-Einzelfall-Discovery
    bleibt im Target-Verzeichnis und behält das frühere Arbeitsverzeichnis. Beide Windows-Debug-Profile
    konfigurieren; das nicht-modulare Inventar sinkt von 246 auf 244 Mehrfachregistrierungen.
  - [x] 2026-11-02: Index Maintenance, Learned Index und IndexAnalyzer verwenden ihre kanonischen Index-
    Modul-Executables. Die direkte RocksDB-Abhängigkeit bleibt für Maintenance und Analyzer erhalten;
    Legacy-Buildtarget-Namen, CTest-Namen, Labels und Timeouts bleiben erhalten. Beide Windows-Debug-Profile
    konfigurieren, und das nicht-modulare Inventar sinkt von 244 auf 241 Mehrfachregistrierungen.
  - [x] 2026-11-02: Index Compression und Matryoshka Truncation verwenden ihre kanonischen Index-Modul-
    Executables statt separater fokussierter Executables. Legacy-Buildtarget-Namen, CTest-Namen, Labels und
    120-s-Timeouts bleiben erhalten; die Quellmengen stimmen überein und die bedingte `themis_storage`-
    Abhängigkeit der bisherigen fokussierten Targets bleibt explizit. Beide Windows-Debug-Profile konfigurieren,
    und das nicht-modulare Inventar sinkt von 241 auf 239 Mehrfachregistrierungen.
  - [x] 2026-11-02: Distributed Vector Index und HNSW Recall verwenden ihre kanonischen Index-Modul-
    Executables statt separater Root-Executables. Die bedingte `themis_storage`-Abhängigkeit bleibt für
    Distributed Vector Index erhalten; HNSW behält die bedingte `hnswlib::hnswlib`-Abhängigkeit. Legacy-
    Buildtarget-Namen sowie CTest-Namen, Labels und Timeouts bleiben erhalten. Beide Windows-Debug-Profile
    konfigurieren, und das nicht-modulare Inventar sinkt von 239 auf 237 Mehrfachregistrierungen.
  - [x] 2026-11-02: IndexManagerDI, Tiered Migration, Spatial Correctness, ANN Index und GPU Memory
    Oversubscription verwenden ihre kanonischen Index-Modul-Executables. Target-spezifische Produktionsquellen,
    nlohmann-json/RocksDB- und `themis_storage`-Abhängigkeiten sowie der bedingte DiskANN-Schalter wurden
    übertragen; die ANN-GTest-Einzelfall-Discovery bleibt erhalten. Legacy-Buildtarget-Namen, CTest-Namen,
    Labels und Timeouts bleiben verfügbar. Beide Windows-Debug-Profile konfigurieren, und das nicht-modulare
    Inventar sinkt von 237 auf 232 Mehrfachregistrierungen.
  - [x] 2026-11-03: 13 Graph-Testquellen (Graph Query Optimizer/Rewriter, Advanced Features, Distributed
    Graph einschließlich Shared-Mutex-Filter, Parallel/Exact Traversal, Query Explain, Scheduled Edge Refresh,
    Ontology Manager, semantische Path Constraints, KnowledgeGraphReasoner und beide RotatE-Suiten) verwenden
    ihre kanonischen Graph-Modul-Executables. Target-spezifische Produktionsquellen, Link-Abhängigkeiten,
    Export-Definitionen, Windows-DLL-Synchronisierung und RotatE-Einzelfall-Discovery wurden übertragen;
    Legacy-Buildtargets, CTest-Namen, Filter, Labels, Timeouts und PATH-Modifikation bleiben erhalten. Beide
    Windows-Debug-Profile konfigurieren; das nicht-modulare Inventar sinkt von 232 auf 218 Mehrfachregistrierungen.
    CTest listet alle Legacy-Registrierungen; Laufzeit-Build und -Tests bleiben wegen des separaten generierten
    gRPC/MSVC-Fehlers unbestätigt.
  - [x] 2026-11-03: 37 LLM-Testquellen verwenden ihre kanonischen Modul-Executables. Zusätzliche Produktionsquellen,
    Link-Abhängigkeiten und Compile-Definitionen für LLM-, RAID- und Inference-Tests wurden übertragen; Legacy-
    Buildtargets, CTest-Namen, Labels und Timeouts bleiben verfügbar. Der verifizierte nicht-modulare Zähler sinkt
    von 218 auf 181; die LLM-Duplikate sind null.
  - [x] 2026-11-03: 27 Geo-Testquellen verwenden ihre kanonischen Modul-Executables. `themis_geo`-Abhängigkeiten,
    CUDA/HIP-Definitionen und die vorhandenen Geo-Sonderregistrierungen bleiben erhalten; CTest-Verträge und
    Legacy-Buildtargets bleiben verfügbar. Der verifizierte nicht-modulare Zähler sinkt von 181 auf 154; Geo-
    Duplikate sind null.
  - [x] 2026-11-03: 13 Analytics-Testquellen verwenden ihre kanonischen Modul-Executables. Zusätzliche Arrow/
    Parquet-/Arrow-Flight- und ONNX-Runtime-Produktionsquellen und Linkbedingungen wurden übertragen; CTest-
    Namen, Labels, Timeouts und Legacy-Buildtargets bleiben erhalten. Beide Windows-Debug-Profile konfigurieren,
    der nicht-modulare Zähler sinkt von 154 auf 141 und Analytics-Duplikate sind null.
  - [x] 2026-11-03: 13 Temporal-Testquellen verwenden ihre kanonischen Modul-Executables. Target-spezifische
    Produktionsquellen, die LZ4-Abhängigkeit, Legacy-Buildtargets und CTest-Verträge bleiben erhalten; GTest-
    Einzelfall-Discovery liegt im Target-owning CMake-Verzeichnis. Beide Windows-Debug-Profile konfigurieren,
    der nicht-modulare Zähler sinkt von 141 auf 128 und Temporal-Duplikate sind null. Der Laufzeit-Build dieser
    Targets ist nicht bestätigt.
  - [x] 2026-11-03: 12 Voice-Testquellen verwenden ihre kanonischen Modul-Executables. Die erforderlichen
    Produktionsquellen, JSON-/`themis_voice`-Linkabhängigkeiten und Legacy-Buildtarget-/CTest-Verträge wurden
    übertragen. Beide Windows-Debug-Profile konfigurieren; der nicht-modulare Zähler sinkt von 128 auf 116,
    Voice-Duplikate sind null.
  - [x] 2026-11-03: 10 Security-Testquellen verwenden ihre kanonischen Modul-Executables. AI-Safety-Produktions-
    quellen, C++17-Einstellungen und Arrow-/OpenSSL-Linkbedingungen wurden übertragen; Legacy-Buildtargets,
    CTest-Namen, Labels und Timeouts bleiben erhalten. Beide Windows-Debug-Profile konfigurieren; der nicht-
    modulare Zähler sinkt von 116 auf 106, Security-Duplikate sind null.
  - [x] 2026-11-03: Neun Performance-Testquellen verwenden ihre kanonischen Modul-Executables. Der
    Hardware-Accelerator benötigt weiterhin seine direkte Produktionsquelle; bestehende GTest-Einzelfall-
    Discovery wurde in das Target-owning Modulverzeichnis verschoben, einschließlich C++17- und LLM-
    Compile-Einstellungen für den TTFT-Benchmark. Legacy-Buildtargets und der Hardware-Accelerator-CTest-
    Vertrag bleiben erhalten. Beide Windows-Debug-Profile konfigurieren; der nicht-modulare Zähler sinkt von
    106 auf 97, Performance-Duplikate sind null.
  - [x] 2026-11-03: Sieben RAG-, sieben Training- und sechs Storage-Testquellen verwenden ihre kanonischen
    Modul-Executables. Target-spezifische Produktionsquellen, RocksDB/OpenSSL/JSON-/LZ4-/Snappy-Abhängigkeiten,
    Compile-Standards, GTest-Fall-Discovery sowie bestehende Buildtarget- und CTest-Verträge bleiben erhalten.
    Beide Windows-Debug-Profile konfigurieren; der nicht-modulare Zähler sinkt von 97 auf 53.
  - [x] 2026-11-03: Vier Chaos-Quellen, drei PostgreSQL-Importer-Quellen und zwei AQL-Quellen verwenden ihre
    kanonischen Modul-Executables. Chaos-Produktionsquellen und `themis_network`-Abhängigkeit, PostgreSQL-
    Aggregat-CTest/Buildtarget sowie AQL-RocksDB/JSON-Abhängigkeiten bleiben erhalten. Beide Windows-Debug-
    Profile konfigurieren; der nicht-modulare Zähler sinkt von 53 auf 44.
  - [x] 2026-11-03: Cache Replication und Cache Replication Coordinator verwenden kanonische Cache-Modul-
    Executables. Die benötigten Coordinator-/gRPC-Peer-/Logger-Quellen, RocksDB- und optionale gRPC-
    Abhängigkeiten sowie Legacy-Buildtarget- und CTest-Verträge bleiben erhalten. Beide Windows-Debug-Profile
    konfigurieren; der nicht-modulare Zähler sinkt von 44 auf 42 (41 actionable Quellen plus der gemeinsam
    genutzte GTest-Runner).
  - [x] 2026-11-03: Distributed Knowledge und Distributed Hybrid Search verwenden kanonische Distributed-
    Modul-Executables. Produktionsquellen, JSON-/C++17-Anforderungen sowie die GTest-Einzelfall-Discovery mit
    ursprünglichem Arbeitsverzeichnis/Argumenten bleiben erhalten; Legacy-Buildtarget-Namen bleiben Aliase.
    Beide Windows-Debug-Profile konfigurieren; der nicht-modulare Zähler sinkt von 42 auf 40
    (39 actionable Quellen plus der gemeinsam genutzte GTest-Runner).
  - [x] 2026-11-03: Multi-Region Active-Active und Multi-GPU Vector Index verwenden kanonische Multi-Modul-
    Executables. GPU-Implementierungsquellen und bedingte GTest-Einzelfall-Discovery sowie das Multi-Region-
    CTest-Label-/Timeout-Verhalten bleiben erhalten; Legacy-Buildtarget-Namen bleiben Aliase. Beide Windows-
    Debug-Profile konfigurieren; der nicht-modulare Zähler sinkt von 40 auf 38
    (37 actionable Quellen plus der gemeinsam genutzte GTest-Runner).
  - [x] 2026-11-03: WorkloadAdaptiveOptimizer und WorkloadFingerprintEngine verwenden ihre kanonischen
    Workload-Modul-Executables. Die spezifischen Produktionsquellen, beide Legacy-Buildtarget-Aliase und die
    nach dem Build generierten GTest-Einzelfalltests bleiben erhalten; WorkloadAdaptiveOptimizer behält seinen
    benannten CTest-Vertrag samt Labels/Timeout. Beide Windows-Debug-Profile konfigurieren; der nicht-modulare
    Zähler sinkt von 38 auf 36 (35 actionable Quellen plus der gemeinsam genutzte GTest-Runner).
  - [x] 2026-11-03: Flash Attention und KV-Cache Reuse verwenden kanonische Phase-1-Modul-Executables.
    Die LLM-Feature-Gate-Bedingung, Windows-Runtime-DLL-Synchronisierung und GTest-Einzelfall-Discovery für
    Release/Debug bleiben erhalten; Legacy-Buildtarget-Namen bleiben Aliase. Beide Windows-Debug-Profile
    konfigurieren; der nicht-modulare Zähler sinkt von 36 auf 34
    (33 actionable Quellen plus der gemeinsam genutzte GTest-Runner).
  - [x] 2026-11-03: Gossip Custom Handler und Module Sandbox WASM Injection verwenden kanonische Modul-
    Executables. JSON/C++17- und `CMAKE_DL_LIBS`-/`themis_base`-Abhängigkeiten, bestehende GTest-Discovery-
    Einstellungen, benannte CTest-Verträge sowie Legacy-Buildtarget-Aliase bleiben erhalten. Beide Windows-
    Debug-Profile konfigurieren; der nicht-modulare Zähler sinkt von 34 auf 32
    (31 actionable Quellen plus der gemeinsam genutzte GTest-Runner).
  - [x] 2026-11-03: Erasure Coding Backend und NUMA Memory Manager verwenden kanonische Modul-Executables.
    Die NUMA-Implementierungsquelle, GTest-Fall-Discovery, beide fokussierten CTest-Verträge und Legacy-
    Buildtarget-Aliase bleiben erhalten. Beide Windows-Debug-Profile konfigurieren; jeder benannte CTest-
    Vertrag ist einmal registriert, und der nicht-modulare Zähler sinkt von 32 auf 30
    (29 actionable Quellen plus der gemeinsam genutzte GTest-Runner).
  - [x] 2026-11-03: IncrementalLoRATrainer verwendet ein kanonisches Incremental-Modul-Executable.
    Die LLM-bedingten Trainings-/Federation-Produktionsquellen und JSON-/optionale YAML-Abhängigkeiten,
    `THEMIS_ENABLE_LLM`, beide Legacy-Buildtarget-Aliase, die benannte Federation-CTest-Suite und GTest-
    Einzelfall-Discovery bleiben erhalten. Beide Windows-Debug-Profile konfigurieren; der nicht-modulare
    Zähler sinkt von 30 auf 29 (28 actionable Quellen plus der gemeinsam genutzte GTest-Runner).
  - [x] 2026-11-03: Intent Classifier, Knowledge Gap Retrieval Callback und Explainability Reason Builder
    verwenden kanonische Modul-Executables. Die beiden benötigten Intent-/Knowledge-Produktionsquellen und
    die Release/Debug-GTest-Discovery für Knowledge bleiben erhalten; Explainability nutzt weiterhin seine
    modulare Produktionsquelle. Beide Windows-Debug-Profile konfigurieren; der nicht-modulare Zähler sinkt
    von 29 auf 26 (25 actionable Quellen plus der gemeinsam genutzte GTest-Runner).
  - [x] 2026-11-03: Die letzte actionable Mehrfachregistrierung, `tests/chunk/test_chunk_level_encryption.cpp`,
    verwendet jetzt nur noch das kanonische Chunk-Modul-Executable. Der Legacy-Buildtarget-Name bleibt als
    abhängigkeitsbasierter Custom-Target-Alias erhalten; GTest-Einzelfall-Discovery, Arbeitsverzeichnis,
    Argumente, Labels und Timeout liegen weiterhin beim target-owning Modulverzeichnis. Die Case-Namen nutzen
    wie zuvor das unveränderte `Suite.Test`-Format ohne zusätzlichen Target-Präfix. Beide Windows-Debug-Profile
    konfigurieren; das Inventar meldet in beiden Profilen null actionable Mehrfachregistrierungen und nur den
    absichtlich gemeinsam genutzten `tests/test_gtest_main.cpp`-Runner. Das Inventar umfasst 4.229
    Testquellen (2.818 modular / 2.908 nicht-modular kompiliert); 1.320 Quellen liegen außerhalb beider Profile
    und bleiben für Coverage-/Ausschlussprüfung offen. CTest-Registrierung ist sichtbar, die Test-Binaries
    wurden nicht gebaut und Laufzeitverhalten ist daher nicht bestätigt.
  - Der gezielte Build der Ingestion-Targets, BaseEntity-/Base-Reload-Aliase sowie der GPU-Graph-Traversal-,
    Network-, CDC-, Auth- und Plugin-Aliase ist nicht bestätigt: Der aktuelle MSVC-Versuch stoppt beim Kompilieren der
    generierten gRPC-Quellen mit C2065/C3878 in `vcpkg_installed/x64-windows/include/grpcpp/impl/call_op_set.h:979`.
    Ein früherer fokussierter Build erreichte `src/prompt_engineering/rewrite_engine.cpp`, `rewrite_rule_base.cpp`
    und `rewrite_rule_loader.cpp` und stoppte dort mit C2059. Keiner dieser Fehler liegt in den Testregistrierungs-
    änderungen; Compilation-Database und CTest-Registrierung allein belegen keinen erfolgreichen Build oder Testlauf.
  - Offen: Baseline bereinigen und begründete Ausschlussliste erstellen. Die 1.322 Quellen außerhalb beider
    Profile bestehen aus 1.018 C++-Quellen unter `tests/legacy/` (einschließlich Helper-/Support-Dateien wie
    `tests/legacy/binary/mock_signing_service.cpp` und `tests/legacy/plugin/runtime_test_plugin.cpp`), 228 Root-Testdateien und 76 Quellen in sonstigen
    Testverzeichnissen. Keine dieser Gruppen gilt pauschal als freigegebener Ausschluss; Profil-Ausnahmen und
    historische Guards (teils mit fehlenden Quellen) sind einzeln zu bewerten.
  - Dokumentationslücke: Die Developer-LLM-Wiki enthält keinen kuratierten TST-03-/Testregistrierungs-Kontext.
    Bis zur Aufnahme über den Wiki-Sync bleiben `tools/ci/check_test_registration.py` und dieser Audit die
    maßgeblichen Implementierungs- und Evidenzquellen.
- [ ] **P0 · OPS-07 – Fuzz-Smoke auf PRs** (Target: M1 · 2027-01-31) – 5 min je Parser-Fuzzer (AQL, HTTP, Wire-Protokoll, Importer)
  bei Änderungen in den jeweiligen Pfaden; Korpus-Pflege in `security-fuzzing.yml`.
- [ ] **P1 · TST-01 – Skip-/Assert-Budget** (Target: v2.5.0 M1 · 2027-03-31) – Baseline v1: 2.871 `GTEST_SKIP`, 976
  ohne same-statement Literal-Grund, 346
  konservative lexikalische Assertions-Kandidaten (manuelle Prüfung), 1.018 C++-Quellen unter `tests/legacy/` ohne Einzel-Disposition
  (einschließlich Helper-/Support-Dateien unabhängig vom `test_`-Namenspräfix).
  M1-Cap je Modul: floor(aktuelle Modul-Skips / 2); bei einem Baseline-Skip ist das Ziel 0, außer mit maintainer-genehmigter Ausnahme.
  Summe der Modul-Caps höchstens 1.418 (aggregierte Obergrenze: floor(2.871 / 2) = 1.435).
  Grundlose Inline-Skips bleiben offen.
  Metrikdefinitionen und Baseline: [TEST_QUALITY_INVENTORY.md](TEST_QUALITY_INVENTORY.md) und
  [test_quality_baseline_v1.json](test_quality_baseline_v1.json).
- [ ] **P1 · TST-02 – Testdichte Hochrisikomodule** (Target: v2.5.0 M1 · 2027-03-31) – frische LOC-/Datei-/Fallzahlen,
  modulbezogene Eigentümer und vorhandene fokussierte CTest-Registrierungen stehen in
  [TEST_QUALITY_INVENTORY.md](TEST_QUALITY_INVENTORY.md). Fehlerpfad- und Nebenläufigkeitstests je öffentlicher API
  bleiben Follow-up; Registrierungen belegen keine erfolgreiche Ausführung.
- [ ] **P1 · Coverage-Messung in CI** (Target: v2.5.0 M1 · 2027-03-31) – Coverage in Baseline v1 nicht verfügbar.
  ≥ 70 % Zeilen- und ≥ 60 % Branch-Coverage sind nur vorgeschlagene Schwellen, pending maintainer approval; derzeit
  nicht enforced. Die Modul-Coverage-Gates bleiben für v3.0.0 M3 sign-off offen. Siehe
  [TEST_QUALITY_INVENTORY.md](TEST_QUALITY_INVENTORY.md).
- [ ] **P2 · CI-04 – `pull_request_target`-Guard** (Target: M1) – Lint-Regel: kein Checkout von PR-Head unter `pull_request_target`.

### WS7 – Kubernetes & Packaging

- [ ] **P0 · OPS-09 – HA-Profil im Helm-Chart** (Target: M2 · 2027-06-30) – `values-production.yaml` mit
  `replicaCount ≥ 3` (Cluster-Modus), PodDisruptionBudget-Template (`minAvailable`), Pod-Anti-Affinity,
  Topology-Spread; `helm lint` + `kubeconform` + kind-Smoke-Test in CI.
- [ ] **P1 · OPS-08 – NetworkPolicy im Produktionsprofil aktiv** (Target: M1) – `helm/themisdb/values.yaml:241`.
- [ ] **P1 · OPS-16 – ServiceMonitor und PrometheusRule** (Target: M1) – Produktionsprofil mit Alerts für
  WAL-Recovery-Status, Replikationsverzug, In-Doubt-Transaktionen, Readiness, Speicherbudget-Abbrüche.
- [ ] **P0 · OPS-12 (+ P1 OPS-17/OPS-18) – Versions-Gate** (Target: M0 · 2026-11-20) – `helm/themisdb/Chart.yaml` (`appVersion`),
  `debian/changelog` (fehlt), `vcpkg.json`, `VERSION` konsistent; CI-Check schlägt bei Abweichung fehl.
- [ ] **P1 · Upgrade-/Rollback-Test** (Target: M1) – N-1 → N mit Bestandsdaten (Single-Node) und Rolling Upgrade (Multi-Node, M2).

### WS8 – Backup/DR & Runbooks

- [ ] **P0 · RUN-01 – Geübte Runbooks mit gemessener RPO/RTO** (Target: M1 · 2027-02-28)
  - Runbooks unter `docs/production/RUNBOOKS/` auf 2.4/2.5 aktualisieren und aus `QUICKSTART.md` bzw. dem Betriebsindex verlinken.
  - Zielwerte (Vorschlag, Human-Entscheid): Single-Node RPO ≤ 5 min, RTO ≤ 30 min bei 100 GB; Multi-Node RPO 0 für
    bestätigte Writes, RTO ≤ 5 min.
  - Quartalsweise Restore-Übung mit Protokoll unter `audit/evidence/` (erste Übung vor M1).
- [ ] **P1 · Automatisierte Restore-Verifikation** (Target: M1) – nightly Backup → Restore in frischer Instanz → Checksummen-/Zeilenvergleich.
- [ ] **P1 · DRP-Review** (Target: M0) – `docs/production/DISASTER_RECOVERY_PLAN.md` überfälliges Quartalsreview nachholen.

### WS9 – Documentation Truth

- [ ] **P0 · DOC-02 – Ein kanonischer Release-Status** (Target: M0 · 2026-11-20)
  - `CHANGELOG.md:66-72`, `:382-386`, `SECURITY.md:26-27`, `RELEASE_STRATEGY.md:79` an `RELEASE_TYPE=alpha` angleichen
    („GA“/„production-ready“ nur als Ziel kennzeichnen).
  - CI-Check: Begriffe „GA Release“/„production-ready“ in Root-Dokumenten nur, wenn `RELEASE_TYPE=ga`.
- [x] **P0 · Storage-README source-synchronisieren** (erledigt 2026-11-02) – `src/storage/README.md` beschreibt die
  aktuellen WAL-Recovery- und Streaming-Ingest-Verträge; STO-01 bleibt partiell und die Evidence-Gates offen.
- [ ] **P1 · DOC-08 – Eine reproduzierbare Fortschrittsmetrik** (Target: M0) – `ROADMAP.md:47-53`.
- [x] **P1 · Compliance-Snapshot kennzeichnen** (Target: M0) – `audit/README.md`: Prozentwerte als Checklisten-Selbsteinschätzung, nicht als Zertifizierung (erledigt 2026-10-31).
- [ ] **P1 · DOC-10 – `llm_l_wiki`** (Target: M1) – Pflicht-Docs ergänzen oder Modul entfernen.
- [ ] **P1 · Developer-LLM-Wiki regenerieren** (Target: M0) – `ai_context/developer_llm_wiki/` nach den Doku-Korrekturen neu kompilieren (generiert, nicht manuell editieren).
- [ ] **P2 · DOC-19 – Doku-Sprawl** (Target: M3) – Archiv aus der Hauptnavigation; Betriebsindex für `docs/production/`.
- [ ] **P2 · DOC-21/DOC-16/DOC-17/DOC-18 – Konsistenz** (Target: M1) – Changelog chronologisch, Edition-Schreibweise, Sprache im Onboarding.

### WS10 – Repo-Hygiene

- [x] **P1 · HYG-01 – Generierte Artefakte aus dem VCS** (Target: M1 · 2027-01-31)
  - Current inventory, classifications, owners, dispositions, and before/after evidence:
    [HYG-01 inventory](evidence/repo-hygiene/HYG-01-inventory-2026-11-09.md).
  - Generated Doxygen and confirmed transient outputs were removed without rewriting history;
    new raw benchmark and scan outputs are ignored and published through retained CI artifacts.
  - The PR-wide 5 MiB gate and exact-path fixture allowlist are implemented and covered by focused tests.
  - Keine History-Umschreibung ohne gesonderten Human-Entscheid.

### WS11 – Evidence & GA-Gates

- [ ] **P0 · GA-Gate-Modell für Stufe 1 nachweisen** (Target: M3 · 2027-06-15) – siehe [Production Readiness Checklist](#production-readiness-checklist), Abschnitt „GA-Gate-Modell“.
- [ ] **P0 · Top-Risk-Modul-Sign-off** (Target: M3) – `server`, `llm`, `sharding` gemäß Root-Roadmap (für `llm` als Preview-Sign-off).
- [ ] **P1 · Evidenz-Frische** (Target: M0, dann laufend) – Ablaufdaten aller Gate-Artefakte überwachen; abgelaufene Artefakte blockieren Promotion.
- [ ] **P1 · Audit-Nachprüfung** (Target: M1 · 2027-03-31) – Re-Audit aller Critical/High-Findings mit Nachweis-Links.

---

## Implementation Phases

### Phase 1 – Design / API-Vertrag (2026-11-01 – 2026-12-15)

- [x] `RecoveryResult`-API und Recovery-Modi für `WALStorage` spezifiziert und mit der Quelle abgeglichen (STO-01, 2026-11-02); Implementierungs- und Evidenz-Gates bleiben teilweise offen.
- [~] ADR „Kanonischer 2PC-Koordinator“ und Adapterentscheidung umgesetzt;
  fokussierte Validierung sowie Decision-Write-/Recovery-Edge-Tests bleiben offen (DIS-05) —
  `audit/ADR_DIS-05_CANONICAL_2PC_COORDINATOR.md` (2026-11-19)
- [ ] Recovery-Vertrag Koordinator/Participant/Replikation als ein Dokument (DIS-06) (Target: 2026-12-15)
- [ ] Produktionsmodus-Vertrag: kanonische Werteliste und Katalog aller Guards (SEC-14, SEC-01, DIS-01, DIS-02, SEC-10) (Target: 2026-11-20)
- [ ] Fehlercodes `QueryMemoryLimitExceeded`, `TransportNotConfigured`, `NotConfigured` und Degraded-Statusschema für Ergebnisse (QRY-01, QRY-03, QRY-04, DIS-03, DIS-08) (Target: 2026-12-15)
- [ ] Admission-Control-Schnittstelle für LLM-Requests (AI-07, AI-09) (Target: 2026-12-15)
- [ ] RPO/RTO-Zielwerte durch Human-Entscheid festlegen (RUN-01) (Target: 2026-11-30)

### Phase 2 – Core-Implementierung (2026-11-15 – 2027-01-31)

- [ ] Gültige CTest-Baseline (EVD-01) und Test-Registrierung fail-closed (TST-03) (Target: M0 · 2026-11-20)
- [ ] Status-Drift bereinigen (DOC-02, Storage-README, DOC-08) und Versions-Gate (OPS-12) (Target: M0 · 2026-11-20)
- [ ] Gate-Workflows ohne Toleranz (OPS-01, CI-03) (Target: M0 · 2026-11-20)
- [ ] WAL-Recovery fail-closed (STO-01) (Target: 2026-12-15)
- [ ] Kanonischer Produktionsmodus + mTLS fail-closed (SEC-14), Signaturpflicht (SEC-01), gRPC-Admin (DIS-02), QUIC-Guard (DIS-01) (Target: 2026-12-15)
- [ ] LIMIT-Pushdown und Speicherbudget (QRY-01) (Target: 2027-01-31)
- [ ] Sichere Beispielkonfigurationen + Config-Lint (SEC-05/06/07) (Target: M0 · 2026-11-20)
- [ ] Stream-Transport-Verdrahtung für Shard-Transfer (DIS-03) (Target: 2027-01-31)

### Phase 3 – Fehlerbehandlung & Edge Cases (2026-12-01 – 2027-02-28)

- [ ] Failover/DR-Handler fail-closed + echte Adapter (DIS-08), Spare-Auswertung (DIS-07), Fencing (DIS-15) (Target: 2027-02-28)
- [ ] Readiness-Fehler für fehlende optionale Abhängigkeiten (STO-02, STO-03) (Target: 2027-01-31)
- [ ] Changefeed (STO-05), Wartungs-Thread-Lebenszyklus (STO-04) (Target: 2027-01-31)
- [~] Streaming-Ingest-Teilbatch (STO-10): admission-/Retry-Vertrag source-synchronisiert; Test- und Ausführungsevidenz offen.
- [ ] Keystore-Ausfall vs. ungültiges Token (SEC-03), Zahlen-Parsing (QRY-02), Search-Deadlines (QRY-07) (Target: 2027-02-28)
- [ ] Stub-Backends im Produktionsmodus sperren (SEC-10) und AI-Admission-Control (AI-07, AI-09) (Target: 2027-02-28)
- [ ] Nicht-blockierender Retry-Backoff (DIS-14), persistenter Manifest-Store (DIS-13) (Target: 2027-02-28)

### Phase 4 – Tests (2027-01-01 – 2027-05-31)

- [~] POSIX WAL-Prozess-Crash-Test an ACK-/Rotations-/Checkpoint-Grenzen hinzugefügt; Ausführung und voller STO-07-Umfang offen (Target: 2027-01-31)
- [ ] AuthZ-Routenmatrix (SEC-13), JWT-Sicherheitstests (SEC-12) (Target: 2027-02-28)
- [ ] Sanitizer-PR-Lane (OPS-06) und Fuzz-Smoke (OPS-07) (Target: 2027-01-31)
- [ ] Skip-/Assert-Budget und Testdichte (TST-01, TST-02), Coverage in CI (Target: 2027-03-31; Baseline:
  [TEST_QUALITY_INVENTORY.md](TEST_QUALITY_INVENTORY.md))
- [ ] Multi-Prozess-Chaos-Suite mit History-Checker (DIS-10, DIS-11) (Target: 2027-05-31)
- [ ] Real-Model-CI-Lane (AI-03) und AI-Skip-Abbau (AI-10) (Target: 2027-04-30)

### Phase 5 – Performance/Hardening (2027-02-01 – 2027-04-30)

- [ ] Performance-Ziele QRY-01 und STO-01 gegen eingecheckte Baselines nachweisen (Target: 2027-03-15)
- [ ] LLM-Lasttest mit 2× Kapazität ohne OOM (AI-07, AI-09) (Target: 2027-04-30)
- [ ] NetworkPolicy (OPS-08) and ServiceMonitor/Alerts (OPS-16, owner #7532) (Target: M1 · 2027-03-31); production Helm HA profile (OPS-09) (Target: M2 · 2027-06-30)
- [ ] Base-Image-Digest-Pins (OPS-11), Plugin-Signer-Pinning (SEC-02), SAML-CSPRNG (SEC-09) (Target: 2027-02-28)
- [x] Repo-Hygiene und Dateigrößen-Gate (HYG-01) (Target: 2027-01-31)
- [ ] Externer Pentest-Retest (Target: 2027-05-31)

### Phase 6 – Dokumentation & Abnahme (2027-03-01 – 2027-06-30)

- [ ] Runbooks aktualisiert, verlinkt und geübt; RPO/RTO gemessen (RUN-01) (Target: 2027-02-28 für Stufe 1, 2027-05-31 für Stufe 2)
- [ ] Modul-READMEs/ROADMAPs aller geänderten Module synchronisiert; Developer-LLM-Wiki regeneriert (Target: laufend, final 2027-06-15)
- [ ] Re-Audit aller Critical/High-Findings mit Nachweis-Links (Target: M1 · 2027-03-31; M2 · 2027-06-30)
- [ ] GA-Gate-Modell für Stufe 1 vollständig belegt; Human-Sign-off nach [../docs/governance/GA_PROMOTION_SIGN_OFF.md](../docs/governance/GA_PROMOTION_SIGN_OFF.md) (Target: M3 · 2027-06-30)

---

## Production Readiness Checklist

### Stufe 1 – Single-Node Core (M1 → M3)

- [ ] Alle P0-Items aus WS1, WS3, WS4, WS6, WS8, WS9 erledigt und re-auditiert
- [ ] WAL-/Storage-Recovery fail-closed; Prozess-Crash-Suite ≥ 1.000 Iterationen nightly ohne Verlust bestätigter Writes
- [ ] Produktionsmodus erzwingt TLS, Zertifikatsprüfung, signierte Updates, keine Stubs; Startup-Log zeigt aufgelösten Modus
- [ ] AuthZ-Routenmatrix grün für alle öffentlichen Schnittstellen
- [ ] Jede externe Anfrage mit Timeout, Speicherbudget und Admission Control
- [ ] Gültiger CTest-Volllauf (Linux + Windows) ≤ 7 Tage alt; Sanitizer- und Fuzz-Smoke auf PRs aktiv
- [ ] Skip-Budget eingehalten; Coverage-Ziel für Stufe-1-Module erreicht (Baseline/Status:
  [TEST_QUALITY_INVENTORY.md](TEST_QUALITY_INVENTORY.md); M3 coverage gates remain open)
- [ ] Restore-Übung mit gemessener RPO/RTO innerhalb der Zielwerte; Upgrade N-1 → N getestet
- [ ] Root-Dokumente konsistent mit `RELEASE_TYPE`; keine unbelegten Produktions-Claims in Modul-Docs
- [ ] Pentest-Retest: 0 Critical, 0 High offen

### Stufe 2 – Multi-Node (M2 → M4)

- [ ] Kanonischer 2PC-Koordinator; zweiter Koordinator entfernt
- [ ] Recovery-Vertrag implementiert und durch Crash-Matrix belegt
- [ ] Shard-Transfer über echten Transport; keine erreichbare Simulation in Produktions-Builds
- [ ] Failover/DR ausschließlich mit verifizierten Adapter-Aktionen; Quorum/Fencing im Modul erzwungen
- [ ] Multi-Prozess-Chaos-Suite (≥ 3 Knoten, Netzfehler, `kill -9`) 72 h ohne Invariantenverletzung
- [ ] Helm-HA-Profil mit PDB, Anti-Affinity, NetworkPolicy, Monitoring/Alerts produktiv getestet
- [ ] 90 Tage Beta-Betrieb ohne Datenverlust-Incident

### GA-Gate-Modell (Root-Roadmap, unverändert verbindlich)

- [ ] Wave 7 Baseline PASS
- [ ] `release_critical` auf `develop` grün
- [ ] Top-Risk-Modul-Sign-off: `server`, `llm`, `sharding`
- [ ] Wave 5/6 Evidenz-Retention eingehalten
- [ ] Wave 8 oder äquivalente Resilience-Evidenz
- [ ] Chaos-Evidenz (Stufe 1: Prozess-Crash; Stufe 2: Multi-Prozess/Netzwerk)
- [ ] Sanitizer- und Recovery-Evidenz
- [ ] Penetrationstest (Retest) ohne offene Critical/High
- [ ] SLA definiert und gemessen
- [ ] Runbooks aktuell und geübt
- [ ] Human-Sign-off gemäß [../docs/governance/GA_PROMOTION_SIGN_OFF.md](../docs/governance/GA_PROMOTION_SIGN_OFF.md)

---

## Known Issues & Limitations

Bis zur Umsetzung der jeweiligen Items gilt für Betrieb und Evaluierung:

- **Tx-/Replikations-WAL fail-open:** Korruption mitten im Log oder Recovery-Timeout führt zu stillem Datenverlust in
  `WALStorage`-Nutzern ([STO-01](FULL_DEEP_DIVE_AUDIT_2026-10-31.md#sto-01)). Der RocksDB-Primärspeicher ist nicht betroffen.
- **Failover/DR nicht vertrauenswürdig:** Handler melden Erfolg ohne Aktion ([DIS-08](FULL_DEEP_DIVE_AUDIT_2026-10-31.md#dis-08)); Multi-Node nur für Tests.
- **Shard-Transfer:** mTLS-Staging ändert keine Ownership; produktiver Export und gefenceter
  Cutover bleiben offen ([DIS-03](FULL_DEEP_DIVE_AUDIT_2026-10-31.md#dis-03)).
- **SEC-14 lokal umgesetzt, Release-Evidenz offen:** Produktionsmodus-Erkennung ist zentralisiert und Shard-RPC
  fällt bei fehlenden/ungültigen mTLS-Credentials nicht mehr auf Plaintext zurück. Der Windows-Debug-Fokustest lief
  mit aktivem gRPC und bestand (52/52); Linux-/Release-Verifikation bleibt bis zum M1-Gate ausstehend.
- **gRPC-Admin-Listener immer Plaintext** ([DIS-02](FULL_DEEP_DIVE_AUDIT_2026-10-31.md#dis-02)) – nur auf Loopback/isoliertem Netz betreiben.
- **SEC-01 lokal fail-closed umgesetzt, Release-Evidenz offen:** Update-Manifeste benötigen einen konfigurierten Verifier, eine gültige Signatur und ein system-verankertes Zertifikat mit erlaubtem Issuer. Ohne passende Trust-Store-Konfiguration schlagen Updates fehl; Linux-/Release-Verifikation steht aus ([SEC-01](FULL_DEEP_DIVE_AUDIT_2026-10-31.md#sec-01)).
- **`LIMIT` ohne Early Termination** ([QRY-01](FULL_DEEP_DIVE_AUDIT_2026-10-31.md#qry-01)) – große Scans mit kleinem Limit sind speicherintensiv.
- **AI/LLM ist Preview:** Real-Model-Tests überwiegend übersprungen ([AI-03](FULL_DEEP_DIVE_AUDIT_2026-10-31.md#ai-03)).
- **Keine gültige aktuelle Test-Evidenz** ([EVD-01](FULL_DEEP_DIVE_AUDIT_2026-10-31.md#evd-01)).
- **Grenzen dieses Plans:** Er basiert auf statischer Analyse ohne neuen Build/CTest-Lauf; 🔎-markierte Findings sind
  nicht einzeln nachgelesen. Zeitziele sind Vorschläge ohne Kapazitätsplanung und benötigen Human-Priorisierung.

---

## Breaking Changes

| Änderung | Item | Auswirkung | Migration |
|---|---|---|---|
| WAL-Recovery standardmäßig `Strict` (Start verweigert bei Korruption/Timeout) | STO-01 | Knoten mit defektem Tx-/Replikations-WAL starten nicht mehr still | Vor Upgrade WAL-Integritätsprüfung; falls ausdrücklich freigegeben, `WALStorage::Config::recovery_mode=SkipCorrupted` quarantänisiert das betroffene Segment |
| Test `CorruptedEntry_StopsReplay` wird ersetzt | STO-01 | Erwartetes Verhalten kehrt sich um | Testsuite-Update im selben PR |
| Kanonische Produktionsmodus-Erkennung | SEC-14 | Werte wie `true`/`production` aktivieren künftig **alle** Guards; unbekannte Werte → Startfehler | Env-Variablen in Deployments prüfen |
| mTLS-Zertifikatsfehler → Startfehler | SEC-14 | Kein Plaintext-Fallback mehr | Zertifikatspfade vor Rollout validieren |
| Unsignierte Update-Manifeste abgelehnt | SEC-01 | Updates ohne Signatur schlagen fehl | Signier-Pipeline vor Rollout; Override nur außerhalb Produktion |
| gRPC-Admin-Listener TLS/Loopback-Pflicht | DIS-02 | Plaintext-Admin auf Nicht-Loopback verweigert | Admin-Clients auf TLS umstellen oder Loopback-Bind |
| QUIC `verify_tls=false` im Produktionsmodus verboten | DIS-01 | Start schlägt fehl | Gültige CA konfigurieren |
| Beispielkonfigurationen binden `127.0.0.1`, TLS an | SEC-05/06/07 | Kopierte Beispiele exponieren nicht mehr | Bind-Adresse bewusst setzen |
| Ergebnisse tragen Degraded-Status (`lexical_fallback`, `cpu_fallback`) | QRY-03, QRY-04 | Zusätzliches Feld im API-Ergebnis | Clients ignorieren unbekannte Felder bzw. werten sie aus |
| Query-Speicherbudget mit Abbruchfehler | QRY-01 | Sehr große Queries können abbrechen | Budget je Workload konfigurieren |
| Stub-Backends im Produktionsmodus gesperrt | SEC-10 | Start ohne echtes Backend schlägt fehl | Modell/Backend bereitstellen oder Feature deaktivieren |
| Fehlende optionale Abhängigkeiten (Kafka, Redis) → `readiness=false` | STO-02, STO-03 | Pods werden nicht ready | Abhängigkeiten bauen/konfigurieren oder Feature deaktivieren |
| Helm-Produktionsprofil (PDB, NetworkPolicy, ServiceMonitor) | OPS-08, OPS-09, OPS-16 | Neue Ressourcen, Netzwerk restriktiver | NetworkPolicy-Regeln für Clients ergänzen |
| CrossShard-2PC-Adapter auf kanonischen Koordinator | DIS-05 | Keine API-Entfernung; Multi-Protokoll-Fassade bleibt | Canonical WAL/Recovery; Legacy-only 2PC entries fail closed; validate before release |

---

## Traceability-Matrix

Alle 79 kalibrierten Findings aus [FULL_DEEP_DIVE_AUDIT_2026-10-31.md](FULL_DEEP_DIVE_AUDIT_2026-10-31.md#5-findings-register):

| Schweregrad | Finding | Workstream | Prio | Meilenstein |
|---|---|---|---|---|
| 🔴 Critical | STO-01 | WS1 | P0 | M1 |
| 🔴 Critical | DIS-08 | WS2 | P0 | M2 |
| 🟠 High | DIS-03, DIS-05, DIS-06, DIS-07, STO-03 | WS2 | P0 | M2 |
| 🟠 High | SEC-01, SEC-14, DIS-01, DIS-02, SEC-13 | WS3 | P0 | M1 |
| 🟠 High | STO-04 | WS1 | P0 | M1 |
| 🟠 High | QRY-01, QRY-03 | WS4 | P0 | M1 |
| 🟠 High | OPS-01, EVD-01 | WS6 | P0 | M0 |
| 🟠 High | OPS-06, OPS-07 | WS6 | P0 | M1 |
| 🟠 High | OPS-09 | WS7 | P0 | M2 |
| 🟠 High | OPS-12 | WS7 | P0 | M0 |
| 🟠 High | DOC-02 | WS9 | P0 | M0 |
| 🟠 High | RUN-01 | WS8 | P0 | M1 |
| 🟠 High | AI-03 | WS5 | P0 (Preview-Exit) | M3 |
| 🟡 Medium | STO-07 | WS1 | P0 | M1 |
| 🟡 Medium | DIS-10, DIS-11 | WS2 | P0 | M2 |
| 🟡 Medium | TST-03, CI-03 | WS6 | P0 | M0 |
| 🟡 Medium | STO-05, STO-06, STO-10 | WS1 | P1 | M1 |
| 🟡 Medium | STO-08 | WS1 | P2 | M3 |
| 🟡 Medium | DIS-04 | WS2 | P1 | M1 |
| 🟡 Medium | DIS-13, DIS-14, DIS-15, STO-02 | WS2 | P1 | M2 |
| 🟡 Medium | SEC-03, SEC-09, SEC-12, OPS-11 | WS3 | P1 | M1 |
| 🟡 Medium | SEC-05, SEC-06, SEC-07 | WS3 | P1 | M0 |
| 🟡 Medium | SEC-04 | WS3 | P2 | M3 |
| 🟡 Medium | QRY-02, QRY-04, QRY-07 | WS4 | P1 | M1 |
| 🟡 Medium | QRY-05, QRY-06, QRY-08 | WS4 | P2 | M3 |
| 🟡 Medium | AI-07, AI-09 | WS5 | P1 | M3 |
| 🟡 Medium | AI-08 | WS5 | P2 | M0 |
| 🟡 Medium | AI-10 | WS5 | P2 | M3 |
| 🟡 Medium | TST-01, TST-02 | WS6 | P1 | M1 |
| 🟡 Medium | OPS-08 | WS7 | P1 | M1 |
| 🟡 Medium | OPS-18 | WS7 | P1 | M0 |
| 🟡 Medium | DOC-08 | WS9 | P1 | M0 |
| 🟡 Medium | DOC-10 | WS9 | P1 | M1 |
| 🟡 Medium | DOC-19 | WS9 | P2 | M3 |
| 🟡 Medium | HYG-01 | WS10 | P1 | M1 |
| ⚪ Low | SEC-10 | WS5 | P1 | M1 |
| ⚪ Low | SEC-02 | WS3 | P1 | M1 |
| ⚪ Low | SEC-08 | WS3 | P2 | M3 |
| ⚪ Low | STO-09 | WS3 (über SEC-13) | P0 | M1 |
| ⚪ Low | AI-01 | WS5 | P2 | M0 |
| ⚪ Low | AI-12 | WS5 | P2 | M1 |
| ⚪ Low | OPS-16 | WS7 | P1 | M1 |
| ⚪ Low | OPS-17 | WS7 | P1 | M0 |
| ⚪ Low | OPS-04 | – (policy-konform, optional SHA-Pins im Release-Pfad) | P2 | – |
| ⚪ Low | CI-04 | WS6 | P2 | M1 |
| ⚪ Low | DOC-07 | WS9 (mit diesem Audit erledigt) | – | erledigt |
| ⚪ Low | DOC-16, DOC-17, DOC-18, DOC-21 | WS9 | P2 | M1 |

---

## GitHub-Issue-Zuordnung

Diese Zuordnung erfasst die 55 Findings, die bei der Erstprüfung kein offenes Issue hatten. Fünf Tickets zu weiterhin offenen Findings wurden nach aktuellem Roadmap-Abgleich wieder geöffnet. DIS-07 ist inzwischen erledigt; das zugehörige Ticket wurde geschlossen. Die übrigen offenen Findings bleiben bei ihren bereits bestehenden Issues.

| Finding | GitHub-Issue (Status) |
|---|---|
| [AI-01](FULL_DEEP_DIVE_AUDIT_2026-10-31.md#ai-01) | [#8188](https://github.com/makr-code/ThemisDB/issues/8188) |
| [AI-03](FULL_DEEP_DIVE_AUDIT_2026-10-31.md#ai-03) | [#8189](https://github.com/makr-code/ThemisDB/issues/8189) |
| [AI-07](FULL_DEEP_DIVE_AUDIT_2026-10-31.md#ai-07) | [#8190](https://github.com/makr-code/ThemisDB/issues/8190) |
| [AI-08](FULL_DEEP_DIVE_AUDIT_2026-10-31.md#ai-08) | [#8191](https://github.com/makr-code/ThemisDB/issues/8191) |
| [AI-09](FULL_DEEP_DIVE_AUDIT_2026-10-31.md#ai-09) | [#8192](https://github.com/makr-code/ThemisDB/issues/8192) |
| [AI-10](FULL_DEEP_DIVE_AUDIT_2026-10-31.md#ai-10) | [#8193](https://github.com/makr-code/ThemisDB/issues/8193) |
| [AI-12](FULL_DEEP_DIVE_AUDIT_2026-10-31.md#ai-12) | [#8194](https://github.com/makr-code/ThemisDB/issues/8194) |
| [CI-03](FULL_DEEP_DIVE_AUDIT_2026-10-31.md#ci-03) | [#8195](https://github.com/makr-code/ThemisDB/issues/8195) |
| [CI-04](FULL_DEEP_DIVE_AUDIT_2026-10-31.md#ci-04) | [#8196](https://github.com/makr-code/ThemisDB/issues/8196) |
| [DIS-03](FULL_DEEP_DIVE_AUDIT_2026-10-31.md#dis-03) | [#8197](https://github.com/makr-code/ThemisDB/issues/8197) |
| [DIS-04](FULL_DEEP_DIVE_AUDIT_2026-10-31.md#dis-04) | [#8198](https://github.com/makr-code/ThemisDB/issues/8198) |
| [DIS-05](FULL_DEEP_DIVE_AUDIT_2026-10-31.md#dis-05) | [#8199](https://github.com/makr-code/ThemisDB/issues/8199) |
| [DIS-06](FULL_DEEP_DIVE_AUDIT_2026-10-31.md#dis-06) | [#8200](https://github.com/makr-code/ThemisDB/issues/8200) |
| [DIS-07](FULL_DEEP_DIVE_AUDIT_2026-10-31.md#dis-07) | [#8201 — erledigt](https://github.com/makr-code/ThemisDB/issues/8201) |
| [DIS-08](FULL_DEEP_DIVE_AUDIT_2026-10-31.md#dis-08) | [#8202](https://github.com/makr-code/ThemisDB/issues/8202) |
| [DIS-10](FULL_DEEP_DIVE_AUDIT_2026-10-31.md#dis-10) | [#8203](https://github.com/makr-code/ThemisDB/issues/8203) |
| [DIS-11](FULL_DEEP_DIVE_AUDIT_2026-10-31.md#dis-11) | [#8204](https://github.com/makr-code/ThemisDB/issues/8204) |
| [DIS-14](FULL_DEEP_DIVE_AUDIT_2026-10-31.md#dis-14) | [#8205](https://github.com/makr-code/ThemisDB/issues/8205) |
| [DIS-15](FULL_DEEP_DIVE_AUDIT_2026-10-31.md#dis-15) | [#8206](https://github.com/makr-code/ThemisDB/issues/8206) |
| [DOC-02](FULL_DEEP_DIVE_AUDIT_2026-10-31.md#doc-02) | [#8207](https://github.com/makr-code/ThemisDB/issues/8207) |
| [DOC-08](FULL_DEEP_DIVE_AUDIT_2026-10-31.md#doc-08) | [#8208](https://github.com/makr-code/ThemisDB/issues/8208) |
| [DOC-10](FULL_DEEP_DIVE_AUDIT_2026-10-31.md#doc-10) | [#8209](https://github.com/makr-code/ThemisDB/issues/8209) |
| [DOC-16](FULL_DEEP_DIVE_AUDIT_2026-10-31.md#doc-16) | [#8210](https://github.com/makr-code/ThemisDB/issues/8210) |
| [DOC-17](FULL_DEEP_DIVE_AUDIT_2026-10-31.md#doc-17) | [#8211](https://github.com/makr-code/ThemisDB/issues/8211) |
| [DOC-18](FULL_DEEP_DIVE_AUDIT_2026-10-31.md#doc-18) | [#8212](https://github.com/makr-code/ThemisDB/issues/8212) |
| [DOC-19](FULL_DEEP_DIVE_AUDIT_2026-10-31.md#doc-19) | [#8213](https://github.com/makr-code/ThemisDB/issues/8213) |
| [DOC-21](FULL_DEEP_DIVE_AUDIT_2026-10-31.md#doc-21) | [#8214](https://github.com/makr-code/ThemisDB/issues/8214) |
| [EVD-01](FULL_DEEP_DIVE_AUDIT_2026-10-31.md#evd-01) | [#8215](https://github.com/makr-code/ThemisDB/issues/8215) |
| [HYG-01](FULL_DEEP_DIVE_AUDIT_2026-10-31.md#hyg-01) | [#8216](https://github.com/makr-code/ThemisDB/issues/8216) |
| [OPS-01](FULL_DEEP_DIVE_AUDIT_2026-10-31.md#ops-01) | [#8217](https://github.com/makr-code/ThemisDB/issues/8217) |
| [OPS-06](FULL_DEEP_DIVE_AUDIT_2026-10-31.md#ops-06) | [#8218](https://github.com/makr-code/ThemisDB/issues/8218) |
| [OPS-07](FULL_DEEP_DIVE_AUDIT_2026-10-31.md#ops-07) | [#8219](https://github.com/makr-code/ThemisDB/issues/8219) |
| [OPS-08](FULL_DEEP_DIVE_AUDIT_2026-10-31.md#ops-08) | [#8220](https://github.com/makr-code/ThemisDB/issues/8220) |
| [OPS-09](FULL_DEEP_DIVE_AUDIT_2026-10-31.md#ops-09) | [#8221](https://github.com/makr-code/ThemisDB/issues/8221) |
| [OPS-11](FULL_DEEP_DIVE_AUDIT_2026-10-31.md#ops-11) | [#8222](https://github.com/makr-code/ThemisDB/issues/8222) |
| [QRY-01](FULL_DEEP_DIVE_AUDIT_2026-10-31.md#qry-01) | [#8223](https://github.com/makr-code/ThemisDB/issues/8223) |
| [QRY-02](FULL_DEEP_DIVE_AUDIT_2026-10-31.md#qry-02) | [#8224](https://github.com/makr-code/ThemisDB/issues/8224) |
| [QRY-03](FULL_DEEP_DIVE_AUDIT_2026-10-31.md#qry-03) | [#8225](https://github.com/makr-code/ThemisDB/issues/8225) |
| [QRY-04](FULL_DEEP_DIVE_AUDIT_2026-10-31.md#qry-04) | [#8226](https://github.com/makr-code/ThemisDB/issues/8226) |
| [QRY-05](FULL_DEEP_DIVE_AUDIT_2026-10-31.md#qry-05) | [#8227](https://github.com/makr-code/ThemisDB/issues/8227) |
| [QRY-06](FULL_DEEP_DIVE_AUDIT_2026-10-31.md#qry-06) | [#8228](https://github.com/makr-code/ThemisDB/issues/8228) |
| [QRY-07](FULL_DEEP_DIVE_AUDIT_2026-10-31.md#qry-07) | [#8229](https://github.com/makr-code/ThemisDB/issues/8229) |
| [QRY-08](FULL_DEEP_DIVE_AUDIT_2026-10-31.md#qry-08) | [#8230](https://github.com/makr-code/ThemisDB/issues/8230) |
| [RUN-01](FULL_DEEP_DIVE_AUDIT_2026-10-31.md#run-01) | [#8231](https://github.com/makr-code/ThemisDB/issues/8231) |
| [SEC-02](FULL_DEEP_DIVE_AUDIT_2026-10-31.md#sec-02) | [#8232](https://github.com/makr-code/ThemisDB/issues/8232) |
| [SEC-04](FULL_DEEP_DIVE_AUDIT_2026-10-31.md#sec-04) | [#8233](https://github.com/makr-code/ThemisDB/issues/8233) |
| [SEC-08](FULL_DEEP_DIVE_AUDIT_2026-10-31.md#sec-08) | [#8234](https://github.com/makr-code/ThemisDB/issues/8234) |
| [SEC-09](FULL_DEEP_DIVE_AUDIT_2026-10-31.md#sec-09) | [#8235](https://github.com/makr-code/ThemisDB/issues/8235) |
| [SEC-12](FULL_DEEP_DIVE_AUDIT_2026-10-31.md#sec-12) | [#8236](https://github.com/makr-code/ThemisDB/issues/8236) |
| [SEC-14](FULL_DEEP_DIVE_AUDIT_2026-10-31.md#sec-14) | [#8237](https://github.com/makr-code/ThemisDB/issues/8237) |
| [STO-01](FULL_DEEP_DIVE_AUDIT_2026-10-31.md#sto-01) | [#8238](https://github.com/makr-code/ThemisDB/issues/8238) |
| [STO-07](FULL_DEEP_DIVE_AUDIT_2026-10-31.md#sto-07) | [#8239](https://github.com/makr-code/ThemisDB/issues/8239) |
| [STO-10](FULL_DEEP_DIVE_AUDIT_2026-10-31.md#sto-10) | [#8240](https://github.com/makr-code/ThemisDB/issues/8240) |
| [TST-01](FULL_DEEP_DIVE_AUDIT_2026-10-31.md#tst-01) | [#8241](https://github.com/makr-code/ThemisDB/issues/8241) |
| [TST-03](FULL_DEEP_DIVE_AUDIT_2026-10-31.md#tst-03) | [#8242](https://github.com/makr-code/ThemisDB/issues/8242) |

## Pflege dieser Roadmap

- Status-Updates erfolgen per Checkbox-Notation (`[ ]`, `[~]`, `[x]`, `[I]`, `[P]`, `[?]`, `[!]`) mit Issue-/PR-Link.
- Übernommene Items werden in [../ROADMAP.md](../ROADMAP.md) bzw. die betroffenen `src/<modul>/ROADMAP.md` gespiegelt;
  bei Abweichungen gilt die Root-Roadmap.
- Nach jedem Meilenstein: Re-Audit der zugeordneten Findings, Aktualisierung von `Last Updated` und der
  [Current Status](#current-status)-Tabelle.
- Änderungen an Branch-/Release-Governance sind **nicht** Gegenstand dieser Roadmap; sie folgen
  [../BRANCHING_STRATEGY.md](../BRANCHING_STRATEGY.md) und [../RELEASE_STRATEGY.md](../RELEASE_STRATEGY.md).
