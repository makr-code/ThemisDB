# ThemisDB – Production-Readiness-Roadmap (audit-abgeleitet)

**Author:** ThemisDB Team
**Created:** 2026-10-31
**Last Updated:** 2026-11-01
**Status:** review
**AI_Used:** true
**AI_Models:** unknown
**AI_Contributor:** GitHub Copilot
**Human_Review:** false
**Human_Author:** ThemisDB Team
**Quelle:** [FULL_DEEP_DIVE_AUDIT_2026-10-31.md](FULL_DEEP_DIVE_AUDIT_2026-10-31.md) (Findings-IDs in dieser Roadmap verweisen dorthin)
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

---

## Planned Features

Jedes Item verweist auf die Finding-ID im Audit-Bericht. Notation: `[ID]` → Abschnitt 5 von
[FULL_DEEP_DIVE_AUDIT_2026-10-31.md](FULL_DEEP_DIVE_AUDIT_2026-10-31.md).

### WS1 – Durability & Recovery

- [~] **P0 · STO-01 – WAL-Recovery fail-closed** (Target: M1 · 2026-12-15)
  - [~] 2026-11-01: Segment replay errors now fail `WALStorage::open`; corruption, checksum/type/sequence errors,
    callback aborts, and timeouts are surfaced. A physically incomplete tail is truncated only after scanning for a
    subsequent complete record header. Remaining: structured recovery status/metrics, configurable timeout and
    override/quarantine policy, replication-deserialization errors, and broader corruption/property tests.
  - Dateien: `src/storage/wal_storage.cpp` (`replaySegment`, Z. 490-514), `include/storage/wal_storage.h`,
    Nutzer `src/sharding/wal_manager.cpp:147`, `src/replication/replication_manager.cpp:670-685`.
  - Verhalten: Recovery liefert `RecoveryResult { status ∈ {Complete, TruncatedTail, Corrupted, TimedOut},
    last_valid_lsn, skipped_bytes, segment }`. `TruncatedTail` nur, wenn nach der defekten Stelle **kein** gültiger
    Record-Header mehr im Segment folgt; jede Korruption mit nachfolgenden gültigen Records ist `Corrupted`.
  - Produktionsmodus: `Corrupted`/`TimedOut` → Start verweigert, `readiness=false`, Metrik
    `themis_wal_recovery_status{status}` und Audit-Event. Override nur explizit über
    `wal.recovery_mode=skip_corrupted` (Default `strict`) mit Quarantäne des Segments nach `<wal_dir>/quarantine/`.
  - Timeout: `wal.recovery_timeout_seconds` (Default `0` = unbegrenzt) plus Fortschrittsmetrik; Ablauf = `TimedOut`.
  - Replikation: nicht deserialisierbare Einträge (`replication_manager.cpp:670-674`) werden `Corrupted` statt WARN+Skip.
  - Fehlerfälle: Magic-Mismatch mitten im Log, CRC-Fehler, abgeschnittener Header, Länge > Segmentrest, I/O-Fehler, Timeout.
  - Tests: `CorruptedEntry_StopsReplay` (`tests/test_wal_storage.cpp:290-316`) ersetzen durch
    `MidLogCorruption_FailsClosed`, `TruncatedTail_IsAccepted`, `SkipCorrupted_QuarantinesSegment`,
    `RecoveryTimeout_FailsClosed`; Property-Test mit zufälligen Bit-Flips (≥ 10.000 Fälle, deterministischer Seed).
  - Performance: Replay-Durchsatz-Regression ≤ 5 % gegenüber Baseline (Benchmark vorher messen und einchecken).
  - Kompatibilität: Breaking Change (siehe [Breaking Changes](#breaking-changes)); Storage-README korrigieren (WS9).
- [ ] **P0 · STO-07 – Prozess-Crash-Harness** (Target: M1 · 2027-01-31)
  - Kindprozess schreibt bestätigte Commits; Elternprozess beendet ihn per `SIGKILL`/`TerminateProcess` an zufälligen
    Punkten (inkl. während `fsync`, Segment-Rotation, Checkpoint, Backup). Danach Neustart und Prüfung:
    jeder bestätigte Commit sichtbar, kein unbestätigter Commit sichtbar, Indizes konsistent.
  - Abdeckung: RocksDB-Primärspeicher, `WALStorage` (Sharding/Replikation), Backup/Restore.
  - Umfang: ≥ 50 Iterationen auf PRs mit Storage-Änderungen, ≥ 1.000 Iterationen nightly; Ergebnis als Artefakt.
  - Bestehende In-Process-Tests (`tests/test_crash_recovery_direct.cpp`, `tests/test_wal_chaos.cpp`) bleiben als Unit-Ebene.
- [ ] **P0 · STO-04 – Wartungs-Threads mit Lebenszyklus** (Target: M1 · 2027-01-31) – `std::thread(...).detach()` in
  `src/maintenance/database_maintenance_orchestrator.cpp:608-613` durch `std::jthread` mit `std::stop_token` ersetzen;
  `stop()` joint mit Timeout; TSan-Test „Shutdown während laufender Wartung“.
- [ ] **P1 · STO-10 – Teilbatch-Semantik Streaming-Ingest** (Target: M1 · 2027-02-28) – `src/storage/streaming_ingest_manager.cpp:320-326`:
  entweder atomarer `WriteBatch` (alles oder nichts) oder Per-Record-Status im Ergebnis; Verhalten in API-Doku festlegen; Fehlerinjektions-Test.
- [ ] **P1 · STO-05 – Changefeed-Sequenz fail-closed** (Target: M1 · 2026-12-15) – leere Catches in `src/cdc/changefeed.cpp:220-223`
  durch Fehlerstatus + Metrik ersetzen; korrupte Legacy-Sequenz darf nicht still auf Fallback zurückfallen.
- [ ] **P1 · STO-06 – CDC-Outbox-Tests unter Windows** (Target: M1 · 2027-01-31) – Fixture-Crash (`tests/cdc/test_cdc_outbox.cpp:26`) beheben, Skip entfernen.
- [ ] **P2 · STO-08 – Timeseries-Testtiefe** (Target: M3 · 2027-06-30) – Retention, Compaction, große Aggregationsfenster, nebenläufige Ingestion.

### WS2 – Distributed Consistency & HA

- [ ] **P0 · DIS-05 – Ein kanonischer 2PC-Koordinator** (Target: ADR 2026-12-15 · Umsetzung M2)
  - ADR: `CrossShardTransactionCoordinator` vs. `TwoPhaseCommitCoordinator` – einen als kanonisch festlegen
    (Kriterien: durable Decision-Log, Presumed-Abort, Timeout-/Retry-Semantik, Testabdeckung).
  - Alle Aufrufer migrieren; der zweite Koordinator wird entfernt (kein Kompatibilitäts-Alias, Legacy-Verbot beachten).
  - Fehlerfälle: Koordinator-Crash vor/nach Decision-Write, Participant-Timeout, doppelte Nachrichten.
  - Tests: bestehende Modelltests auf kanonischen Koordinator umstellen; Prozess-Crash-Varianten aus STO-07.
- [ ] **P0 · DIS-06 – Recovery-Vertrag** (Target: M2 · 2027-04-30)
  - Ein Dokument plus Code-Durchsetzung: Decision-Log durable **vor** COMMIT-ACK; In-Doubt-Auflösung beim Start
    (blockierend, mit Metrik); Participant-WAL und Replikations-WAL nutzen den fail-closed Recovery-Status aus STO-01.
  - Test: Matrix {Koordinator, Participant, Replica} × {Crash vor Prepare, nach Prepare, nach Decision, während Commit}.
- [ ] **P0 · DIS-03 – Echter Stream-Transport für Shard-Transfer** (Target: M2 · 2027-03-31)
  - `src/sharding/stream_protocol.cpp:663-710`: Produktions-Caller verdrahtet `setPrepareTransferCallback()` mit dem
    gRPC-Transport; ohne Callback **und** mit realem Endpunkt → Fehler `TransportNotConfigured` statt In-Process-Simulation.
  - Simulation nur hinter dem bestehenden Compile-Gate `THEMIS_TEST_MODE` (nie in Produktions-Presets) und mit `STUB/SIMULATION NOTE`.
  - Test: Shard-Transfer zwischen zwei Prozessen inkl. Abbruch und Wiederaufnahme; Checksummenvergleich Quelle/Ziel.
- [~] **P0 · DIS-08 – Failover/DR-Handler fail-closed** (Target: M2 · 2027-04-30)
  - [~] 2026-11-01: Non-dry-run DR steps now fail when snapshot/restore/catch-up/traffic/verification hooks are missing;
    quorum no longer counts as catch-up proof. Spare failure blocks failover and missing metadata persistence returns
    failure. Remaining: implement persistent metadata and real adapters with ACK plus independent content/LSN
    verification, then add multi-process end-to-end coverage.
  - Handler ohne konfigurierten Storage-/Traffic-/Metadata-Adapter liefern `NotConfigured`; Erfolg nur nach Adapter-ACK
    **und** Verifikation (Checksumme bzw. LSN/Term-Vergleich); Metadata-Update persistent.
  - Tests: negative Tests je fehlendem Adapter; Ende-zu-Ende-Failover in der Multi-Prozess-Suite.
- [ ] **P0 · DIS-10/DIS-11/TST-03 – Multi-Prozess-Chaos-Suite** (Target: M2 · 2027-05-31)
  - 3–5 Knoten (Docker Compose oder kind) in CI; Netzfehler über `tc netem`/Toxiproxy (Latenz, Reordering, Duplikate,
    asymmetrische Partition); `kill -9` von Koordinator/Participants/Leader.
  - History-Checker für Serialisierbarkeit bzw. Linearisierbarkeit (z. B. Elle-kompatibles Historienformat).
  - `tests/test_auto_failover_recovery.cpp` registrieren (derzeit ausgeschlossen, `tests/CMakeLists.txt:315`).
  - Exit: 72 h nightly ohne Invariantenverletzung; Ergebnisse als Release-Evidenz.
- [~] **P0 · DIS-07 – Spare-Aktivierung auswerten** (Target: M2) – Rückgabe von `activateSpareIfNeeded()`
  (`src/failover/auto_failover_manager.cpp:613-614`) wird geprüft; fehlgeschlagene Aktivierung blockiert den Abschluss
  und liefert eine Fehlermeldung. Status `Degraded` und externe Alarmierung bleiben offen.
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

- [ ] **P0 · SEC-14 – Kanonische Produktionsmodus-Erkennung + mTLS fail-closed** (Target: M1 · 2026-12-15)
  - Alle Direktparser von `THEMIS_PRODUCTION_MODE`/`THEMIS_ENVIRONMENT` (`src/sharding/shard_rpc_server.cpp:330`,
    `src/sharding/shard_rpc_client.cpp:224-226`, `src/storage/storage_engine.cpp:39-50`,
    `src/storage/ggml_tensor_bridge.cpp:51`, `src/security/timestamp_authority.cpp:52`,
    `include/security/transport_security_checker.h:37-65`, `include/security/hsm_security_checker.h:38`) durch
    `themis::core::ProductionMode::isEnabled()` ersetzen; Werteliste case-insensitiv und dokumentiert
    (`1|true|yes|on|production` bzw. `THEMIS_ENVIRONMENT=production|prod`); unbekannte Werte → Startfehler.
  - `shard_rpc_server.cpp:320-326`: Zertifikats-/Key-Ladefehler bei aktiviertem mTLS → Exception (kein Insecure-Fallback).
  - Tests: Parametrisierter Test über alle Werteformen; Test „mTLS aktiviert + fehlende Datei → Start schlägt fehl“.
- [ ] **P0 · SEC-01 – Signaturpflicht für Update-Manifeste** (Target: M1 · 2026-12-15)
  - `src/updates/manifest_database.cpp:272`: fehlender Verifier oder leere Signatur → Ablehnung.
  - Override nur außerhalb des Produktionsmodus über `updates.allow_unsigned=true` (Audit-Event).
  - Gepinnte Trust-Anchors; Negativtests: unsigniert, unbekannter Signer, abgelaufenes Zertifikat, manipulierte Payload.
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
- [ ] **P1 · AI-07/AI-09 – Zentrale Admission Control** (Target: M3 · 2027-04-30)
  - Hartes `max_tokens`-Cap an der Request-Grenze (`src/llm/task_decomposer.cpp:982` nur Fallback); globales
    Concurrency-Limit je Modell; GPU-Speicherbuchhaltung; Überlast → HTTP 429 mit `Retry-After`.
  - Tests: Lasttest mit 2× Kapazität ohne OOM; Requests über Cap werden abgelehnt.
- [ ] **P1 · SEC-10 – Stub-Backends im Produktionsmodus sperren** (Target: M1) – `src/whisper/whisper_plugin.cpp:18-29` und
  alle in `src/STUB_INVENTORY.md` gelisteten aktiven Stubs: Start im Produktionsmodus ohne echtes Backend schlägt fehl.
- [ ] **P1 · Preview-Kennzeichnung** (Target: M1) – AI-Endpunkte liefern `X-Themis-Feature-Stage: preview`; Aktivierung
  nur über explizites Feature-Flag; Doku in API-Referenz.
- [ ] **P2 · AI-10 – Vision-/LoRA-Skips beheben** (Target: M3) – `tests/llm/test_llm_vision_encoder.cpp:129`,
  `tests/llm/test_lora_adapters.cpp:319`; LoRA-Lifecycle unter TSan.
- [ ] **P2 · AI-08 – Concurrency-Claim belegen oder entfernen** (Target: M0) – `src/llm/THREADING.md:493`.
- [ ] **P2 · AI-12 – Timeout-Lücke abgleichen** (Target: M1) – `src/llm/TODO_CRITICAL_GAPS.md:107` vs. Produktions-Claims; Timeout-Integrationstest.
- [ ] **P2 · AI-01 – Toten Code entfernen** (Target: M0) – `src/llm/ssm_stub_plugin.cpp` (vollständig `#if 0`).

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
  - Guard-Check: neuer Workflow-Lint, der Toleranz-Konstrukte in Gate-Workflows ablehnt.
- [ ] **P0 · OPS-06 – Sanitizer auf PRs** (Target: M1 · 2026-12-31) – ASan+UBSan-Lane für geänderte Kernmodule
  (`storage`, `transaction`, `query`, `aql`, `server`, `auth`, `network`, `sharding`); Laufzeitbudget ≤ 45 min;
  TSan nightly für `transaction`, `sharding`, `llm`.
- [~] **P0 · TST-03 – Test-Registrierung fail-closed** (Target: M0 · 2026-11-20)
  - [~] 2026-11-01: Der tote WAL-Fokustest-Pfad zeigt jetzt auf `tests/test_wal_storage.cpp` und registriert diesen
    Test wieder. Offen: `tests/test_auto_failover_recovery.cpp` registrieren und alle gelisteten `if(EXISTS …)`-Guards
    fail-closed machen.
  - Stilles `if(EXISTS …)` für gelistete Testquellen durch `message(FATAL_ERROR …)` ersetzen; CI-Check: jede
    `tests/**/test_*.cpp` ist registriert oder steht mit Begründung auf einer Ausschlussliste.
- [ ] **P0 · OPS-07 – Fuzz-Smoke auf PRs** (Target: M1 · 2027-01-31) – 5 min je Parser-Fuzzer (AQL, HTTP, Wire-Protokoll, Importer)
  bei Änderungen in den jeweiligen Pfaden; Korpus-Pflege in `security-fuzzing.yml`.
- [ ] **P1 · TST-01 – Skip-/Assert-Budget** (Target: M1 · 2027-03-31) – Skip-Budget je Modul (Ziel: −50 % der 2.965
  `GTEST_SKIP` bis M1); 1.020 triviale Asserts ersetzen oder Tests löschen; 1.018 Legacy-Testdateien portieren oder entfernen.
- [ ] **P1 · TST-02 – Testdichte Hochrisikomodule** (Target: M1) – `server` (Routenmatrix aus SEC-13), `distributed_tensor`,
  `prompt_engineering`, `timeseries`, `scheduler`, `llama_cpp`: Mindestens Fehlerpfad- und Nebenläufigkeitstests je öffentlicher API.
- [ ] **P1 · Coverage-Messung in CI** (Target: M1) – Zeilen-/Branch-Coverage je Modul nightly; Stufe-1-Module ≥ 70 % Zeilen-Coverage als GA-Gate (Vorschlag, Human-Entscheid).
- [ ] **P2 · CI-04 – `pull_request_target`-Guard** (Target: M1) – Lint-Regel: kein Checkout von PR-Head unter `pull_request_target`.

### WS7 – Kubernetes & Packaging

- [ ] **P0 · OPS-09 – HA-Profil im Helm-Chart** (Target: M2 · 2027-03-31) – `values-production.yaml` mit
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
- [ ] **P0 · Storage-README korrigieren** (Target: M0) – `src/storage/README.md:67`, `:76`, `:122` bis zum Abschluss von STO-01
  als „nicht erfüllt“ markieren und auf diese Roadmap verweisen.
- [ ] **P1 · DOC-08 – Eine reproduzierbare Fortschrittsmetrik** (Target: M0) – `ROADMAP.md:47-53`.
- [x] **P1 · Compliance-Snapshot kennzeichnen** (Target: M0) – `audit/README.md`: Prozentwerte als Checklisten-Selbsteinschätzung, nicht als Zertifizierung (erledigt 2026-10-31).
- [ ] **P1 · DOC-10 – `llm_l_wiki`** (Target: M1) – Pflicht-Docs ergänzen oder Modul entfernen.
- [ ] **P1 · Developer-LLM-Wiki regenerieren** (Target: M0) – `ai_context/developer_llm_wiki/` nach den Doku-Korrekturen neu kompilieren (generiert, nicht manuell editieren).
- [ ] **P2 · DOC-19 – Doku-Sprawl** (Target: M3) – Archiv aus der Hauptnavigation; Betriebsindex für `docs/production/`.
- [ ] **P2 · DOC-21/DOC-16/DOC-17/DOC-18 – Konsistenz** (Target: M1) – Changelog chronologisch, Edition-Schreibweise, Sprache im Onboarding.

### WS10 – Repo-Hygiene

- [ ] **P1 · HYG-01 – Generierte Artefakte aus dem VCS** (Target: M1 · 2027-01-31)
  - Generiertes Doxygen-XML und Dateien > 5 MB (u. a. `docs/ARCHIVED/ai-working-history/scan_tests.json`,
    `archive/moved_files.txt`, Root-Bench-JSONs) nicht mehr tracken; Ablage als Release-/CI-Artefakt; `.gitignore`.
  - PR-Gate: neue Dateien > 5 MB werden abgelehnt (Allowlist für Testfixtures).
  - Keine History-Umschreibung ohne gesonderten Human-Entscheid.

### WS11 – Evidence & GA-Gates

- [ ] **P0 · GA-Gate-Modell für Stufe 1 nachweisen** (Target: M3 · 2027-06-15) – siehe [Production Readiness Checklist](#production-readiness-checklist), Abschnitt „GA-Gate-Modell“.
- [ ] **P0 · Top-Risk-Modul-Sign-off** (Target: M3) – `server`, `llm`, `sharding` gemäß Root-Roadmap (für `llm` als Preview-Sign-off).
- [ ] **P1 · Evidenz-Frische** (Target: M0, dann laufend) – Ablaufdaten aller Gate-Artefakte überwachen; abgelaufene Artefakte blockieren Promotion.
- [ ] **P1 · Audit-Nachprüfung** (Target: M1 · 2027-03-31) – Re-Audit aller Critical/High-Findings mit Nachweis-Links.

---

## Implementation Phases

### Phase 1 – Design / API-Vertrag (2026-11-01 – 2026-12-15)

- [ ] `RecoveryResult`-API und Recovery-Modi für `WALStorage` spezifizieren (STO-01) (Target: 2026-11-20)
- [ ] ADR „Kanonischer 2PC-Koordinator“ inkl. Migrationsplan der Aufrufer (DIS-05) (Target: 2026-12-15)
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
- [ ] Changefeed (STO-05), Streaming-Ingest-Teilbatch (STO-10), Wartungs-Thread-Lebenszyklus (STO-04) (Target: 2027-01-31)
- [ ] Keystore-Ausfall vs. ungültiges Token (SEC-03), Zahlen-Parsing (QRY-02), Search-Deadlines (QRY-07) (Target: 2027-02-28)
- [ ] Stub-Backends im Produktionsmodus sperren (SEC-10) und AI-Admission-Control (AI-07, AI-09) (Target: 2027-02-28)
- [ ] Nicht-blockierender Retry-Backoff (DIS-14), persistenter Manifest-Store (DIS-13) (Target: 2027-02-28)

### Phase 4 – Tests (2027-01-01 – 2027-05-31)

- [ ] Prozess-Crash-Harness Single-Node (STO-07) (Target: 2027-01-31)
- [ ] AuthZ-Routenmatrix (SEC-13), JWT-Sicherheitstests (SEC-12) (Target: 2027-02-28)
- [ ] Sanitizer-PR-Lane (OPS-06) und Fuzz-Smoke (OPS-07) (Target: 2027-01-31)
- [ ] Skip-/Assert-Budget und Testdichte (TST-01, TST-02), Coverage in CI (Target: 2027-03-31)
- [ ] Multi-Prozess-Chaos-Suite mit History-Checker (DIS-10, DIS-11) (Target: 2027-05-31)
- [ ] Real-Model-CI-Lane (AI-03) und AI-Skip-Abbau (AI-10) (Target: 2027-04-30)

### Phase 5 – Performance/Hardening (2027-02-01 – 2027-04-30)

- [ ] Performance-Ziele QRY-01 und STO-01 gegen eingecheckte Baselines nachweisen (Target: 2027-03-15)
- [ ] LLM-Lasttest mit 2× Kapazität ohne OOM (AI-07, AI-09) (Target: 2027-04-30)
- [ ] Helm-HA-Profil, NetworkPolicy, ServiceMonitor/Alerts (OPS-09, OPS-08, OPS-16) (Target: 2027-03-31)
- [ ] Base-Image-Digest-Pins (OPS-11), Plugin-Signer-Pinning (SEC-02), SAML-CSPRNG (SEC-09) (Target: 2027-02-28)
- [ ] Repo-Hygiene und Dateigrößen-Gate (HYG-01) (Target: 2027-01-31)
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
- [ ] Skip-Budget eingehalten; Coverage-Ziel für Stufe-1-Module erreicht
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
- **Shard-Transfer simuliert** ohne verdrahteten Transport ([DIS-03](FULL_DEEP_DIVE_AUDIT_2026-10-31.md#dis-03)).
- **Transport-Security abhängig von exakter Env-Schreibweise:** Shard-mTLS-Zwang nur bei `THEMIS_PRODUCTION_MODE=1`;
  mTLS-Zertifikatsfehler → Plaintext ([SEC-14](FULL_DEEP_DIVE_AUDIT_2026-10-31.md#sec-14)). Bis zur Behebung
  `THEMIS_PRODUCTION_MODE=1` setzen und Shard-Zertifikate vor dem Start prüfen.
- **gRPC-Admin-Listener immer Plaintext** ([DIS-02](FULL_DEEP_DIVE_AUDIT_2026-10-31.md#dis-02)) – nur auf Loopback/isoliertem Netz betreiben.
- **Unsignierte Update-Manifeste werden akzeptiert** ([SEC-01](FULL_DEEP_DIVE_AUDIT_2026-10-31.md#sec-01)) – Auto-Update deaktivieren.
- **`LIMIT` ohne Early Termination** ([QRY-01](FULL_DEEP_DIVE_AUDIT_2026-10-31.md#qry-01)) – große Scans mit kleinem Limit sind speicherintensiv.
- **AI/LLM ist Preview:** Real-Model-Tests überwiegend übersprungen ([AI-03](FULL_DEEP_DIVE_AUDIT_2026-10-31.md#ai-03)).
- **Keine gültige aktuelle Test-Evidenz** ([EVD-01](FULL_DEEP_DIVE_AUDIT_2026-10-31.md#evd-01)).
- **Grenzen dieses Plans:** Er basiert auf statischer Analyse ohne neuen Build/CTest-Lauf; 🔎-markierte Findings sind
  nicht einzeln nachgelesen. Zeitziele sind Vorschläge ohne Kapazitätsplanung und benötigen Human-Priorisierung.

---

## Breaking Changes

| Änderung | Item | Auswirkung | Migration |
|---|---|---|---|
| WAL-Recovery standardmäßig `strict` (Start verweigert bei Korruption/Timeout) | STO-01 | Knoten mit defektem Tx-/Replikations-WAL starten nicht mehr still | Vor Upgrade WAL-Integritätsprüfung; bei Bedarf einmalig `wal.recovery_mode=skip_corrupted` mit Quarantäne |
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
| Zweiter 2PC-Koordinator entfernt | DIS-05 | API-Entfernung für interne Aufrufer | Migration laut ADR |

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

## Pflege dieser Roadmap

- Status-Updates erfolgen per Checkbox-Notation (`[ ]`, `[~]`, `[x]`, `[I]`, `[P]`, `[?]`, `[!]`) mit Issue-/PR-Link.
- Übernommene Items werden in [../ROADMAP.md](../ROADMAP.md) bzw. die betroffenen `src/<modul>/ROADMAP.md` gespiegelt;
  bei Abweichungen gilt die Root-Roadmap.
- Nach jedem Meilenstein: Re-Audit der zugeordneten Findings, Aktualisierung von `Last Updated` und der
  [Current Status](#current-status)-Tabelle.
- Änderungen an Branch-/Release-Governance sind **nicht** Gegenstand dieser Roadmap; sie folgen
  [../BRANCHING_STRATEGY.md](../BRANCHING_STRATEGY.md) und [../RELEASE_STRATEGY.md](../RELEASE_STRATEGY.md).
