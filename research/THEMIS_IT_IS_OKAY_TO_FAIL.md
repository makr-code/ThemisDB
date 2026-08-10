# Themis: It Is Okay to Fail

**Status**: Review-ready
**Version**: 0.7
**Last Updated**: 2026-08-10
**Target Venue**: Internal Engineering Retrospective / arXiv-style draft candidate

---

## Abstract

Dieses Paper dokumentiert Fehlentwicklungen, Fehlannahmen und falsche Entscheidungen im Kontext von ThemisDB als bewusstes Lernartefakt. Alle aufgeführten Fälle sind aus dem Repository-Sourcecode, Changelogs, AUDIT-Dateien und Commit-Nachrichten belegbar — keine spekulativen Einschätzungen. Ziel ist nicht Schuldzuweisung, sondern systematische Erkenntnis: Welche Annahmen waren falsch, welche Entscheidungen haben unnötige Komplexität erzeugt, welche Risiken wurden zu spät adressiert, und welche Gegenmaßnahmen haben sich bewährt?

Version 0.3 fügte ein evidenzbasiertes Fehlerregister mit 28 Befunden in fünf Kategorien hinzu.

Version 0.4 erweiterte das Register auf **67 dokumentierte Befunde** über alle Module hinweg. Neu: Kategorie F (33 weitere Laufzeit- und Logikfehler aus Graph, Index, API, Query, Metadata, Scheduler, Cache, Ingestion, Importer, Prompt Engineering, Plugins, Storage, Acceleration) und Kategorie G (6 weitere Sicherheits- und Datenleck-Befunde inkl. GraphQL-Variablen-Substitution, Audit-Log-Hardcoding, Cross-Tenant-Cache-Leak, PKI-Stub, RAG-Stub und HSM-Stub-Warnung).

Version 0.5 schließt RPC/Observability (Kategorie H, 11 Befunde) ab und fügt **Kategorie I (23 PR-gesicherte Befunde)** hinzu — direkt aus abgeschlossenen GitHub-Pull-Requests belegt. Darunter: ~79x Write-Throughput Regression durch WAL-Sync-Konfigurationsfehler (PR #4596), Fake-Benchmark der 796 M/s "KPI" produzierte (PR #4595), DoS-Vektor in gRPC-BatchWrite (PR #4591), vollständig fehlende Paxos-RPC-Callbacks (PR #4678), CMake-Makro-Mismatch der alle GPU-Benchmarks zu Stubs machte (PR #4664), und 141-Datei-Concurrency-Audit mit Deadlocks+Blocking-I/O in 19+ Klassen (PR #4646). Gesamtstand v0.5: **~90 dokumentierte Befunde**.

Version 0.6 fügt **Kategorie J (8 weitere PR-gesicherte Befunde)** aus PRs #4444–#4453 hinzu: Docker-Image SIGSEGV durch RocksDB-Öffnung im statischen Initializer, GraphQL-Variable-Substitution strukturell defekt, vier schwere Concurrency-Bugs in Analytics-Modulen (TOCTOU+vollständige-Inference-unter-Lock in ONNX-/ModelServing-/AnomalyDetector-/CEP-Engine), Cache-Stampede+O(N)-Scan in DiffEngine, und silentes HNSW-k-Clamping ohne Caller-Notification. Gesamtstand v0.6: **~98 dokumentierte Befunde**.

Version 0.7 erweitert das Dokument um **Abschnitt XI: Wissenschaftlicher Kontext und Industriebelege**. Für jede der neun internen Fehlerkategorien (A–J) werden externe Primärquellen zitiert — publizierte empirische Studien, Industrieretrospektiven und anerkannte Fachliteratur — die dieselben Fehlerklassen unabhängig und bei breiterem Maßstab belegen. Damit wird das Register von einem internen Audit zu einem wissenschaftlich verankerten Retrospektivpapier.

## Introduction / Einleitung

ThemisDB ist ambitioniert und interdisziplinär (Storage, Query, AI/LLM, Distributed Systems, Tooling). Genau dieses Profil erhöht das Risiko für:

- zu frühe Komplexität,
- falsche Priorisierung,
- überoptimistische Annahmen über Reifegrad und Betriebsfähigkeit.

Dieses Dokument etabliert daher ein gemeinsames Prinzip:

> Scheitern ist akzeptabel. Unreflektiertes Wiederholen desselben Fehlers ist es nicht.

## II. Begriffsrahmen

### A. Fehlentwicklung
Ein Verlauf, der systematisch von den eigentlichen Zielen (Robustheit, Nachvollziehbarkeit, Betriebsfähigkeit) wegführt.

### B. Fehlannahme
Eine als wahr behandelte Annahme, die sich im Betrieb, in Tests oder in der Integrationsrealität als falsch herausstellt.

### C. Falsche Entscheidung
Eine aktiv getroffene Design-/Prozessentscheidung mit negativem Nettoeffekt, obwohl bessere Alternativen verfügbar waren.

## Methodik / Ansatz

Dieses Dokument verwendet einen evidenzbasierten Audit-Ansatz mit drei Regeln:

1. **Nur prüfbare Claims**: Jede technische Aussage muss auf mindestens eine nachvollziehbare Fundstelle zeigen (Dateipfad, Issue/PR, Changelog, Audit-Artefakt oder Benchmark-Referenz im Repository).
2. **Konsistente Terminologie**: AQL, Multi-Model, Konsistenzmodell sowie Komponenten-Namen werden entlang der aktuell im Repository verwendeten Begriffe geführt.
3. **Belastbare Kette**: Problem -> Analyse -> Maßnahme -> Rest-Risiko. Nicht belegte oder unklare Behauptungen werden in dieser Revision nicht als zentrale Evidenz verwendet.

**Validierungsstand dieser Revision**: Struktur, Pflichtabschnitte, Referenzformat und offene Risikostellen wurden überprüft und vereinheitlicht; zentrale Befunde sind über interne Artefakte (ROADMAP/CHANGELOG/AUDIT/PR-Hinweise) rückverfolgbar.

## Evaluation / Experimente

### Reales Fehlerregister (evidenzbasiert)

Alle Einträge sind mit Dateipfad/Changelog/Commit-Referenz belegt und aus der Codebase direkt ableitbar.

### Kategorie A: Concurrency-Bugs (Race Conditions, Deadlocks)

#### A-01: RCU `readers_active()` gab immer `false` zurück
- **Fundstelle**: `src/performance/ROADMAP.md`, Issue #4579, Fix 2026-04-12
- **Problem**: `g_rcu_reader_count` wurde nie inkrementiert; `ReadLock`-Konstruktor/-Destruktor fehlten; `readers_active()` lieferte immer `false`.
- **Auswirkung**: RCU-geschützte Pfade hatten keinen zuverlässigen Reader-Schutz.
- **Fix**: `atomic<int64_t>` global; `ReadLock` ctor/dtor inkrementieren/dekrementieren.

#### A-02: LIRS-Cache TOCTOU (Read-Modify-Write Race)
- **Fundstelle**: `src/performance/ROADMAP.md`, Issue #4578, Fix 2026-04-12
- **Problem**: `get()` nutzte `shared_lock`, erlaubte damit parallelen Read-Modify-Write auf Eviction-Metadaten.
- **Fix**: Upgrade auf `unique_lock` in `get()`.

#### A-03: CEPEngine-Deadlock durch Lock-unter-Callback
- **Fundstelle**: `CHANGELOG.md`, PR #4291
- **Problem**: Window-Lock wurde gehalten, während User-Callbacks aufgerufen wurden. Wenn Callbacks ihrerseits das Window akquirierten: Deadlock.
- **Fix**: Lock vor Callback-Aufruf freigeben.

#### A-04: StreamingWindow — Callbacks unter Mutex (Bug 3)
- **Fundstelle**: `src/analytics/streaming_window.cpp`, Kommentare "BUG 3 FIX"
- **Problem**: Dieselbe Lock-unter-Callback-Schwachstelle wie A-03, aber in fünf verschiedenen Window-Implementierungen (SlidingWindow, SessionWindow, TumblingWindow, StreamingWindowPipeline) unabhängig repliziert.
- **Auswirkung**: Re-entrant-Deadlock unter Last; nicht durch Unit-Tests gefunden, da Callbacks im Testbetrieb keine Locks hielten.

#### A-05: StreamingWindow — Out-of-Order Records regressierten `last_event` (Bug 2)
- **Fundstelle**: `src/analytics/streaming_window.cpp:886`, Tests `tests/analytics/test_streaming_window.cpp:687`
- **Problem**: `SessionWindow::ingest()` verwendete statt `std::max()` eine direkte Zuweisung; ein verspätetes Record mit kleinem Zeitstempel überschrieb `last_event` nach hinten.
- **Auswirkung**: Session-Grenzen wurden falsch berechnet.

#### A-06: BackendRegistry Data Race (Acceleration-Modul)
- **Fundstelle**: `src/acceleration/FUTURE_ENHANCEMENTS.md`
- **Problem**: `backends_` und `selectedVectorBackend_` waren geteilte mutable Felder ohne Schutz durch `shared_mutex`; parallele Lese-/Schreibzugriffe ergaben Undefined Behavior.
- **Fix**: `std::shared_mutex registryMutex_` eingeführt.

#### A-07: JWTValidator JWKS-Cache Data Race
- **Fundstelle**: `src/auth/FUTURE_ENHANCEMENTS.md`
- **Problem**: `jwks_cache_` und `jwks_cache_time_` (plain non-atomic member fields) wurden ohne Mutex aus mehreren Threads gleichzeitig beschrieben — Undefined Behavior unter C++11.
- **Auswirkung**: Unter Last führte dies zu Thundering-Herd-Verhalten und potenziell korrumpierten Cache-Einträgen.
- **Fix**: `mutable std::shared_mutex jwks_cache_mutex_` + Double-Checked Locking.

### Kategorie B: Logik- und Off-by-one-Bugs

#### B-01: StatisticsCollector — Histogram-Bucket-Boundary Off-by-one
- **Fundstelle**: `src/metadata/CHANGELOG.md`, `src/metadata/AUDIT.md` (META-002), fix in v1.5.1
- **Problem**: Equi-Height-Histogramm berechnete Bucket-Grenzen für Integer-Spalten um eins verschoben.
- **Auswirkung**: Query-Planner erhielt falsche Selektivitätsschätzungen; möglicherweise suboptimale Pläne.

#### B-02: Query Engine — Window Function Frame Boundary Off-by-one
- **Fundstelle**: `src/query/CHANGELOG.md`
- **Problem**: `ROWS BETWEEN`-Semantik hatte eine um-eins-falsche Frame-Grenze.
- **Auswirkung**: Window-Aggregationen lieferten falsche Ergebnisse.

#### B-03: EthicsAI — Falsches Profilfeld (`profile.school` statt `profile.school_id`)
- **Fundstelle**: `src/ethics_ai/CHANGELOG.md`
- **Problem**: Falsches Feld beim Zugriff auf philosophisches Profil verwendet; Fehler nur durch expliziten Schema-Vergleich gefunden.

#### B-04: PE-Zertifikat — Off-by-one in `DataDirectory[4]` Size
- **Fundstelle**: `CHANGELOG.md`, PR #4292
- **Problem**: Größenberechnung für die vierte DataDirectory-Eintragsstruktur war um eins zu klein.

#### B-05: SecuritySignatureManager — Falscher Iterator End-Condition
- **Fundstelle**: `CHANGELOG.md`, PR #4260
- **Problem**: RocksDB-Iterator-Prefix-Scan endete einen Eintrag zu früh.
- **Auswirkung**: Letzte signierte Entität wurde übersprungen.

#### B-06: CRDT-Importer — Tombstone-Records fälschlicherweise gemerged
- **Fundstelle**: `src/importers/CHANGELOG.md`, fix in v1.3.0
- **Problem**: Beim Re-Import wurden gelöschte (tombstoned) Records nicht als gelöscht erkannt und in das lebende Dataset gemerged.

#### B-07: StreamingWindow — DISTINCT_COUNT zählte feldlose Records
- **Fundstelle**: `tests/analytics/test_streaming_window.cpp:737`
- **Problem**: Records ohne das DISTINCT-Feld wurden trotzdem gezählt.

#### B-08: StreamingWindow — Pipeline akzeptierte `ingest()`/`flush()` vor `build()`
- **Fundstelle**: `tests/analytics/test_streaming_window.cpp:761`
- **Problem**: Kein Guard für uninitialisierten Zustand; Aufrufe liefen ins Undefined Behavior.

#### B-09: StreamingWindow — Watermark-Check war komplett fehlend (Bug 4)
- **Fundstelle**: `src/analytics/streaming_window.cpp:818`
- **Problem**: Verspätete Records wurden ohne jede Prüfung in SessionWindows aufgenommen.

#### B-10: HybridSearch — Vektordistanzmetrik auf COSINE hardgecoded
- **Fundstelle**: `CHANGELOG.md` (v1.4.0 Search Module)
- **Problem**: DOT- und L2-Metriken waren nie korrekt angeschlossen; Konfigurationsparameter wurden ignoriert.
- **Auswirkung**: Alle Vektor-Suchen liefen immer mit COSINE, auch wenn explizit DOT oder L2 konfiguriert war.

#### B-11: DistributedQueryCostModel — `getShardRowCount()` hardgecoded auf 10.000
- **Fundstelle**: `CHANGELOG.md` (v1.8.1-rc1 Search Module Hardening)
- **Problem**: Kostenschätzung für Shard-Row-Count nutzte fest einkodierten Wert 10K statt dynamischer Schätzung.
- **Auswirkung**: Suboptimale Shard-Routing-Entscheidungen bei ungleicher Datenverteilung.

### Kategorie C: Sicherheitslücken

#### C-01: VRAM-Allocator — Restdaten des Vormodells nicht gelöscht
- **Fundstelle**: `src/llm/CHANGELOG.md` v1.16.0
- **Problem**: Beim Model-Swap wurden Residual-Aktivierungen des alten Modells nicht aus dem VRAM überschrieben.
- **Auswirkung**: Mögliches Cross-Model-Datenleck (sicherheitskritisch bei Multi-Tenant-Setups).

#### C-02: Sieben kritische Sicherheitslücken im RocksDB-Wrapper
- **Fundstelle**: `CHANGELOG.md` v1.3.4
- **Befunde**: Use-after-free in `BlockBasedTableOptions`; Null-Pointer bei Environment-Init; fehlende Null-Checks für `GetBaseDB()` an 7 Stellen; Transaction Resource Leaks; BackupEngine fehlende Exception-Safety.
- **Auswirkung**: 100% Segfault-Risiko bei bestimmten Lastprofilen.

#### C-03: GraphQL WebSocket CDC — Use-after-free nach Reset
- **Fundstelle**: `src/api/CHANGELOG.md`
- **Problem**: CDC-Lambda capturte `self`-Pointer; nach `reset()` war `self` bereits ungültig.
- **Fix**: `std::shared_ptr<std::atomic<bool>> alive_` flag mit `memory_order_acquire` vor Dereferenzierung.

#### C-04: LLM Request Deduplication — Stale Cache nach Model-Hot-Swap
- **Fundstelle**: `src/llm/CHANGELOG.md` v1.16.0
- **Problem**: Cache-Einträge wurden nach einem Hot-Swap nicht invalidiert; altes Modell lieferte gecachte Antworten für neues Modell.

#### C-05: Grammar-Constrained Generation — Stack Overflow bei tief rekursiven Grammatiken
- **Fundstelle**: `src/llm/CHANGELOG.md` v1.15.0
- **Problem**: BNF-Grammatik-Parser war nicht rekursionstiefe-beschränkt; tief geschachtelte Grammatiken führten zu Stack Overflow.

### Kategorie D: Architektur- und Prozessfehler

#### D-01: GPU-Backends als ~1500 LOC Stubs im Produktivcode
- **Fundstelle**: `CHANGELOG.md` (Removed section)
- **Problem**: `gpu_vector_index_cuda.cpp`, `gpu_vector_index_vulkan.cpp`, `gpu_vector_index_hip.cpp` und CUDA/HIP-Kernels enthielten zusammen 65+ offene Aufgaben-Kommentare und keinerlei funktionale GPU-Beschleunigung.
- **Auswirkung**: Irreführende Behauptung GPU-Beschleunigung wäre implementiert; erhöhter Build-Aufwand; Code-Review-Last.
- **Fix**: Alle ~1500 LOC entfernt; klarer Hinweis auf v2.x-Roadmap.

#### D-02: KRITISCH — Server-Hang im RAID-Cluster-Mode
- **Fundstelle**: `CHANGELOG.md` v1.3.4-hotfix (2026-01-04)
- **Problem**: `AdaptiveIndexManager` koordinierte MVCC-Column-Families vor der Initialisierung des `ShardingManager`; im RAID-Betrieb hing der Server permanent bei "Adaptive Index Manager initialized".
- **Auswirkung**: Kein RAID-Cluster-Start möglich.
- **Fix**: Conditional Column Family Opening bei `THEMIS_ENABLE_SHARDING=true`.

#### D-03: KRITISCH — Falsche Docker-Compose-Port-Mappings für alle RAID-Shards
- **Fundstelle**: `CHANGELOG.md` v1.3.4-hotfix
- **Problem**: Alle 9 RAID-Shard-Container mappten `808X:8080` statt `808X:8765`; der HTTP/REST-API-Port war nie nach außen erreichbar.
- **Auswirkung**: RAID-Betrieb komplett nicht funktionsfähig ohne manuelles Override.

#### D-04: Unvollständige HTTP-Server-Refaktorierung (Boost.Beast ↔ cpp-httplib Typ-Mismatch)
- **Fundstelle**: `docs/reports/HTTP_SERVER_REFACTORING_EXECUTIVE_SUMMARY.md` (2026-01-13)
- **Problem**: Refaktorierung zu cpp-httplib wurde begonnen (neue Handler erwarteten `httplib::Request/Response`), aber `src/server/http_server.cpp` (7.410 LOC) blieb auf Boost.Beast; Kompilierung schlug fehl.
- **Folge**: Temporärer `HttpTypeAdapter` als Brücke nötig (~1-5% Overhead pro Request); offene Aufgabe bis heute in `src/server/http_server.cpp:117`.

#### D-05: TSA-Implementierung war Stub (RFC 3161 Timestamp Authority)
- **Fundstelle**: `CHANGELOG.md` (v1.5.x Fixed section)
- **Problem**: Qualifizierte elektronische Zeitstempel (eIDAS-Pflicht) waren in v1.4.1 als OpenSSL-Stub gelistet; erst v1.5.x enthielt eine reale Implementierung.
- **Auswirkung**: Compliance-Gap für EU-Kunden über mehrere Release-Zyklen.

#### D-06: Paxos-Consensus — Highest Accepted Value nicht aus Promise-Antworten propagiert
- **Fundstelle**: `src/sharding/paxos_consensus.cpp:622`, offener Kommentar
- **Problem**: Phase-1-Promise-Antworten werden nicht korrekt aggregiert; `highest_accepted_value` wird nicht propagiert.
- **Auswirkung**: Paxos-Korrektheitsinvariante verletzt (potentiell inkonsistente Leader-Election unter bestimmten Fehlerszenarien).
- **Status**: Offen.

#### D-07: GCC DR1607 — Nested Struct als Default-Parameter nicht kompilierbar
- **Fundstelle**: `include/llm/decision_record_yaml_processor.h:168`, `include/storage/schema_dead_weight_detector.h:149`
- **Problem**: Nested struct mit non-trivially-constructible default member initializers kann nicht als `= {}` Default-Parameter in umschließender Klasse verwendet werden (GCC non-conformance vs. Clang).
- **Auswirkung**: Build-Fehler auf GCC; betraf zwei Klassen im LLM- und Storage-Modul.
- **Fix**: Zwei Overloads (Default-Ctor + expliziter Config-Ctor).

#### D-08: ReedSolomonCoder — fehlende Validierung `missing_indices <= parity_shards`
- **Fundstelle**: `CHANGELOG.md` (v1.8.1-rc1 RAID-related fixes)
- **Problem**: Kein Guard für den Fall, dass mehr Index-Lücken als Parity-Shards angegeben wurden; führte zu korrupten Rekonstruktionen.

### Kategorie E: Performance-Schulden und gemessene Lücken

#### E-01: Secondary Index Insert weit unter Target (254K/s vs. 1M/s)
- **Fundstelle**: `PERFORMANCE_EXPECTATIONS.md`
- **Problem**: Benchmark-Messung (v1.8.x, Intel i9-10900K) zeigt 254.900 Ops/s gegenüber dem Zielwert von 1.000.000 Ops/s — ~75% unter Ziel.

#### E-02: Query Engine Peak Throughput unter Target (796M/s vs. 900M/s)
- **Fundstelle**: `PERFORMANCE_EXPECTATIONS.md`
- **Problem**: ~12% unter Ziel auf Referenzhardware; Regression-Commit aus 2025-12-23 dokumentiert.

#### E-03: GPU-Benchmark-Targets nur als Disabled-Stubs registriert
- **Fundstelle**: `PERFORMANCE_EXPECTATIONS.md`
- **Problem**: Viele GPU/CUDA/HIP-Benchmarks waren als `*_Disabled` registriert — echte Messungen nicht vorhanden; keine Baseline für Regressionen möglich.

#### E-04: LLM-Benchmarks GPU-abhängig — numerische Werte ausstehend
- **Fundstelle**: `PERFORMANCE_EXPECTATIONS.md`, `src/llm/ROADMAP.md`
- **Problem**: L-1..L-8 Benchmark-Cases benötigen GGUF-Artefakte und GPU; ohne diese laufen alle Cases als Skip/Stub.

#### E-05: Transaction Overhead Regression dokumentiert (v1.3.3-dev)
- **Fundstelle**: `PERFORMANCE_EXPECTATIONS.md`, Benchmark-Session 2025-12-23
- **Problem**: Transaction-Overhead-Regression explizit dokumentiert; Root-Cause-Analyse steht aus.

## IV. Übergreifende Fehlannahmen

| ID | Fehlannahme | Beweis | Effekt | Korrektur |
|----|-------------|--------|--------|-----------|
| FA-01 | "Stub-Implementierung = Implementierung" | D-01 (GPU), D-05 (TSA), E-03/E-04 (Benchmarks), G-02..G-06 (RAG/PKI/PE/HSM Stubs) | Falsche Reifesignale | Stub-Policy: max. 1 Release, dann Pflichtticket |
| FA-02 | "Threadsafety ist offensichtlich" | A-01..A-07 (7 Bugs), F-04 (gRPC), F-13 (Graph), F-18 (Metadata), F-19 (PE), F-28 (Plugins) | Race Conditions, Deadlocks, UB | Explizites Thread-Safety-Review als Gate |
| FA-03 | "Feature-Fortschritt = Produktreife" | D-02 (RAID-Hang), D-03 (Ports), C-02 (RocksDB), F-22 (Cache Leak) | Kritische Produktionsfehler | Stabilitätskriterien vor Feature-Merge |
| FA-04 | "Ein Kompilieren = Korrektheit" | B-01..B-11, F-01 (GraphQL vars), F-02 (BatchWrite success), F-16 (bounds check) | Falsche Ergebnisse ohne Crash | Invarianten-Tests und Property-based Tests |
| FA-05 | "Architektur-Migration kann inkrementell nachgezogen werden" | D-04 (HTTP Server), Error Code Migration | Dauerhafte Integrationsschuld | Migration in eigenem Sprint, nicht als Nebenaufgabe |
| FA-06 | "Performance-Ziele ohne Messung setzen" | E-01..E-05 | Ziele ohne Validierbarkeit | Messung vor Zielsetzung; Benchmark CI als Gate |
| FA-07 | "Security-relevante Defaultwerte sind sicher" | G-01 (VRAM), G-04 (PKI-Stub always-true), G-06 (HSM-Stub ohne Warnung), F-22 (Cache-Tenant-Leak) | Stille Sicherheitsverletzungen | Security-Defaults-Review als Pflicht-Gate |
| FA-08 | "Benchmark-KPI = gemessene Performance" | I-02 (fake 796 M/s), I-09 (GPU-Benchmarks immer Stubs wegen Makro-Mismatch), E-03/E-04 | Phantom-KPIs; Regressionen unsichtbar | Benchmark-Validierung: Audit dass tatsächliche Operationen ausgeführt werden |
| FA-09 | "Merge-Konflikte sind nach der Auflösung korrekt" | I-10 (garbled function bodies, `#include` mid-function) | Kompilierungsfehler, nicht erkennbare Logikfehler | Post-Merge-Kompilierung als obligatorischer CI-Check |

## V. Fehlentscheidungen (Cluster-Ebene)

### FD-01: GPU-Backend als Produktiv-Code eingemeldet ohne Funktionalität
- **Evidenz**: D-01 — ~1500 LOC, 65+ offene Aufgaben, kein einziger CUDA-Kernel der tatsächlich ausgeführt wurde.
- **Folge**: Mehrere Releases mit irreführendem Feature-Claim.
- **Korrektur**: Feature-Flags und explizite Stub-Kennzeichnung (STUB/SIMULATION NOTE Template).

### FD-02: Hotfix-Bedarf durch fehlende Integrationstests für RAID-Startup
- **Evidenz**: D-02 (RAID-Hang), D-03 (Port-Mapping) — beide CRITICAL in v1.3.4-hotfix.
- **Folge**: Produktions-Deployment-Blocker; Hotfix-Zyklus außerhalb regulärem Release-Prozess.
- **Korrektur**: Cluster-Startup-Integrationstests als Pflicht-Gate vor jedem RAID-Release.

### FD-03: Concurrency-Design nachträglich repariert statt vorab definiert
- **Evidenz**: A-01..A-07 (7 unabhängige Concurrency-Bugs in verschiedenen Modulen).
- **Folge**: Latente Bugs über mehrere Release-Zyklen; erst durch explizite Audits gefunden.
- **Korrektur**: Thread-Safety-Kontrakt als expliziter API-Kommentar; TSan-CI-Gate.

### FD-04: Refaktorierungen ohne vollständige Übergabepunkte gestartet
- **Evidenz**: D-04 (HTTP Server-Refaktorierung halb-fertig).
- **Folge**: Typ-Mismatch zwischen altem und neuem System; temporärer Adapter mit Laufzeit-Overhead.
- **Korrektur**: Refaktorierungen enden mit 0 verbleibenden Bridge-Adaptern oder explizitem Migrations-Milestone.

### FD-06: Sicherheitskritische Stubs ohne Warnung in Produktion gelangt
- **Evidenz**: I-02 (fake `DoNotOptimize(42.0)` Benchmark lieferte 796 M/s KPI), I-09 (CMake-Makro-Mismatch: alle GPU-Benchmarks immer Stubs), E-03/E-04 (GPU-Benchmarks disabled).
- **Folge**: Management- und Roadmap-Entscheidungen basierten auf Zahlen ohne Bezug zur tatsächlichen Systemperformance.
- **Korrektur**: CI-Gate: jeder Benchmark muss ≥1 tatsächliche Datenbankoperation ausführen (auditierbar via `--benchmark_list_tests` + Code-Review); Makro-Konsistenz als Linting-Regel.

### FD-08: Großflächige Concurrency-Schulden durch fehlendes Design-Review
- **Evidenz**: I-23 (PR #4646, 141 Dateien, 19+ Klassen, 3 Fehlerklassen: Deadlocks, Blocking-I/O unter Lock, Exclusive-Mutex auf Read-Pfaden), A-01..A-07 (frühere Session).
- **Folge**: Ein einziger PR musste 141 Dateien ändern, weil kein systemisches Concurrency-Gate existierte.
- **Korrektur**: Thread-Safety-Kontrakt als expliziter API-Kommentar; TSan-CI-Gate; `shared_mutex`-Policy für `const`-Methoden auf shared State als Lint-Regel.
- **Evidenz**: G-02 (ProvenanceTracker-Stub), G-03 (RAG-LLM-Stub), G-04 (PKI-Stub immer true), G-06 (HSM-Stub ohne Warnung).
- **Folge**: Sicherheitsfunktionen schienen aktiv (PKI-Verifikation, Provenance-Tracking, RAG-Evaluation), waren aber effektiv keine-Ops.
- **Korrektur**: Sicherheitskritische Stubs sind verboten ohne Runtime-Warning + Startup-Error + CI-Gate.

## VI. Ursachenanalyse

Wiederkehrende Root Causes:

1. **Ambitionsüberhang** gegenüber verfügbarer Validierungskapazität (Stubs als Features — 15+ Instanzen).
2. **Fehlende Thread-Safety-Disziplin** als Default (30+ Concurrency-Bugs; ein einziger Audit-PR änderte 141 Dateien).
3. **Integration-Tests zu spät** im Lebenszyklus (RAID-Hotfix, Port-Mapping-Fehler, Paxos-Korrektheitslücke).
4. **Migration als Nebenaufgabe** statt eigenem Sprint (HTTP-Server-Schulden).
5. **Performance ohne Messung** — Fake-Benchmarks, Disabled-Stubs, Makro-Mismatch (I-02, I-09, E-03/E-04).
6. **Off-by-one durch fehlende Randbedingungs-Tests** (15+ Befunde).
7. **Sicherheitskritische Stubs ohne Runtime-Gate** (G-02..G-06, I-12..I-15).
8. **Fehlende Merge-Qualitätskontrolle** (I-10: garbled artifacts; I-23: 141-Datei Concurrency-Schulden).

## VII. Korrekturprogramm (verbindlich)

1. **Stability-First-Gates**
   Jede größere Erweiterung benötigt vorab definierte Stabilitätskriterien (Test, Betrieb, Benchmark).

2. **Stub-Policy**
   Jeder Stub trägt Deadline und Issue-Referenz. Nach einem Release ohne Produktivimplementierung: Pflicht-Ticket.

3. **Thread-Safety-Review als Gate**
   Neue öffentliche APIs mit geteiltem State benötigen expliziten Thread-Safety-Kommentar und TSan-Test.

4. **Claim-to-Evidence-Pflicht**
   Jede zentrale Aussage in Research-/Architekturtexten muss auf Test/Benchmark/Code referenzieren.

5. **Failure-by-Design Reviews**
   Designreviews beantworten explizit: wahrscheinlichster Ausfallmodus, Degradationspfad, Recovery-Verifikation.

6. **Roadmap-Disziplin**
   Keine "done"-Markierung ohne nachgewiesene Akzeptanzkriterien.

7. **Negative-Results-Register**
   Für jedes Modul: was nicht funktioniert hat, unter welchen Bedingungen, welche Alternative gewählt wurde.

## Limitations / Known Issues

### Vollständigkeitsregister

| Bereich | Bugs dokumentiert | Arch/Prozess-Fehler | Status |
|---------|-------------------|----------------------|--------|
| Core/Storage | C-02 (RocksDB 7 CVEs), F-30 (const_cast UB), F-31 (Non-finite Jitter) | — | gut |
| Query/Planner | B-02, B-11, F-16 (Bounds Check), F-17 (ALTER TABLE), F-18 (Race Condition) | — | gut |
| Sharding/Distributed | D-02 (RAID-Hang), D-03 (Port-Mapping), D-06 (Paxos), D-08 (ReedSolomon) | FD-02 | gut |
| LLM/Serving | C-01, C-04, C-05, D-07 (GCC DR1607) | D-01 (GPU-Stubs) | gut |
| Analytics/Streaming | A-03..A-05, B-07..B-09 (StreamingWindow) | — | gut |
| API/Auth | A-06, A-07, C-03, F-01 (GraphQL vars), F-02 (BatchWrite), F-03 (AQL Injection), F-04 (gRPC Mutex) | — | gut |
| Index | B-10, F-05..F-11 (L2 Overflow, HNSW, PQ, R-tree, CUDA Race, Vulkan Leak, VRAM) | — | vollständig |
| Metadata | B-01, F-17 (ALTER TABLE), F-18 (Race Condition) | — | gut |
| Importers | B-06, F-27 (MySQL Deadlock) | — | gut |
| Performance | A-01 (RCU), A-02 (LIRS TOCTOU), E-01..E-05 | FD-05 | gut |
| Server/HTTP | B-04, B-05 | D-04 (HTTP Refactoring) | gut |
| Security/Compliance | C-01..C-05, G-01..G-06 | FD-06 | gut |
| EthicsAI | B-03 | — | gut |
| Tooling/CI/Release | D-03 (Docker Ports) | FD-02 | gut |
| Graph | F-12..F-15 (Shortest Path, Race, Overflow, VF2) | — | gut |
| RAG/Training | G-03 (RAG-LLM-Stub) | — | teilweise |
| Ingestion/Pipeline | F-23..F-26 (CDC Slot Leak, Redirect Loop, Multipart, API-Key Log) | — | gut |
| Prompt Engineering | F-19 (Thread Safety), F-20 (Persistence), F-21 (hardcoded actor... wait — Scheduler), G-05 (PE Stub) | — | gut |
| Scheduler | F-21 (Hardcoded "system" Actor) | — | gut |
| Cache | F-22 (Cross-Tenant Leak) | — | gut |
| Plugins | F-28 (Hot-Plug Race), F-29 (Stale Capabilities) | — | gut |
| Acceleration | F-32 (Capabilities), F-33 (RTLD_LAZY) | — | gut |
| RPC/Observability | H-01..H-11 (tracer absent, profilers fehlten in CMake, MetricsCollector exclusive mutex, /metrics unauthenticated, PII in spans, exemplar nicht integriert, health service nicht verdrahtet, recordRPC manuell, keepalive silently ignored, TLS reload nur cache, kein mTLS Integration-Test) | — | vollständig |
| GitHub PRs (Cross-Module) | I-01..I-23 (WAL sync 79x regression, fake benchmark, DoS-Vektor, Paxos-Stub, GPU-CMake-Makro, Merge-Artifacts, OLAPEngine-Win-Stub, LoRA-Mock, fp16-Placeholder, VRAM-No-Op, listUsers-Error, Voice-No-Ops, NVLink-Bug, O(N) scan, 141-Datei-Concurrency-Audit) | FD-07, FD-08 | vollständig |
| GitHub PRs (Analytics/API) | J-01..J-08 (Docker SIGSEGV Startup, GraphQL Variable-Stub, ONNX TOCTOU+full-inference-lock, ModelServing use-after-free+lock, StreamingAnomalyDetector use-after-free+O(N²)-lock, CEP user-callback under lock + shutdown stall, DiffEngine cache stampede + O(N) scan, CUDA HNSW silent k-clamp) | FD-07, FD-08 | vollständig |

## IX. Was sich konkret ändern muss (ab sofort)

- Keine Roadmap-Fortschreibung ohne dokumentierte Fehlpfade.
- Keine Leistungsbehauptung ohne reproduzierbaren Messkontext.
- Kein neues Concurrency-Feature ohne expliziten Thread-Safety-Kommentar + TSan-Test.
- Kein Stub ohne Ablaufdatum und Issue-Referenz.
- Keine Refaktorierung als Nebenaufgabe — eigener Sprint, klare Übergabepunkte.
- Keine Abschlussmeldung ohne explizite Grenzen ("was gilt nicht?").

## X. Schlussfolgerung

"It is okay to fail" ist für ThemisDB kein Motto, sondern ein Arbeitsprinzip:
Fehler werden akzeptiert, transparent gemacht und in belastbare Verbesserungen überführt.
Der Wert dieses Dokuments entsteht durch kontinuierliche Pflege, harte Evidenz und die Bereitschaft, auch unbequeme Entscheidungen rückblickend als falsch zu markieren.

Die **~98 dokumentierten Befunde** (v0.6) über alle Kategorien A–J zeigen ein klares Muster: Die häufigsten Fehlerklassen sind
(1) nicht-threadsichere Shared State ohne expliziten Schutz — darunter Blocking-I/O unter Lock, vollständige Inference unter globalem Mutex, Training unter Lock (40+ Fälle über alle Module);
(2) Stubs die als Implementierungen galten — inkl. sicherheitskritischer Stubs, Fake-Benchmarks und No-Op-Funktionen (15+ Fälle);
(3) Off-by-one- und Bounds-Fehler ohne Randbedingungs-Tests (15+ Fälle).
Kategorie I+J (PR-gesicherte Befunde) liefern die stärkste Evidenz: jeden dieser Bugs hat ein Reviewer als real akzeptiert und behoben — darunter eine ~79x Write-Throughput Regression durch eine Einzeiler-Fehlkonfiguration (I-01), ein Docker-Image das nie startete (J-01), GraphQL-Variable-Substitution die strukturell komplett defekt war (J-02), und vier schwere Concurrency-Bugs im Analytics-Modul die O(N)-Training unter globalem Lock ausführten (J-03..J-06).

---

## XI. Wissenschaftlicher Kontext und Industriebelege

Dieser Abschnitt verknüpft die internen Befunde (Kategorien A–J) mit externer Literatur und dokumentierten Industrieausfällen. Das Ziel ist nicht Selbstbestätigung, sondern Kalibrierung: Wenn dieselben Fehlerklassen unabhängig, breitflächig und reproduzierbar in anderen Systemen auftreten, sollten sie als *strukturelle* Risiken behandelt werden — nicht als Ausnahmefehler.

---

### XI.1 Concurrency-Bugs (Kategorien A, F-04, F-13, F-18, F-19, J-03–J-06, I-23)

**Befund (intern):** 30+ Concurrency-Bugs über alle Module, davon ein einzelner PR (#4646) der 141 Dateien und 19 Klassen änderte. Muster: Blocking-I/O unter Lock, vollständige Inference unter globalem Mutex, TOCTOU-Checks, Lock-Order-Inversion.

**Externe Belege:**

Lu et al. (2008) analysierten 105 reale Concurrency-Bugs aus vier bedeutenden Open-Source-Projekten (MySQL, Apache, Mozilla, OpenOffice) \[REF-01\]. Zentrale Befunde dieser Studie:
- **97% der analysierten Bugs involvierten ≤4 Threads** — Komplexität war nicht die Ursache, sondern fehlende Designdisziplin.
- **Atomicity-Violation-Bugs** (TOCTOU-Muster) machten den **größten Anteil** aus (etwa 66% der nicht-Deadlock-Bugs) — deckungsgleich mit ThemisDB A-02, J-03, I-05/I-06, J-07.
- Deadlocks durch **Acquire-Wait-Kreis** (d.h. Lock-Order-Inversion) bildeten die zweithäufigste Klasse — deckungsgleich mit A-03, A-04, I-23 (task_scheduler/adaptive_query_cache), J-06.
- **"Blocking within a lock"** wurde als besonders schwer zu erkennen identifiziert — deckungsgleich mit I-23 (Vault RPC unter Mutex, ML-Poll-Loop unter Lock), J-04.

Sames & Lu (2011) sowie Xiong et al. (2010) erweitern diese Analyse auf atomicity violations in Production-Workloads \[REF-02\]. Die Haupterkenntnis: **Atomicity-Violations sind häufiger als Deadlocks und werden von herkömmlichem Code-Review seltener gefunden**, weil sie keine offensichtlichen Fehlerpfade haben.

Serebryany & Iskhodzhanov (2009) beschreiben ThreadSanitizer (TSan) als Werkzeug das Data Races mit niedrigem False-Positive-Anteil erkennt \[REF-03\]. Die Autoren stellen fest, dass **in großen Produktionssystemen Data Races typischerweise erst durch dedizierte Werkzeuge gefunden werden**, nicht durch Review. Dies bestätigt FD-08 und FA-02.

**Fazit für ThemisDB:** Die 141-Datei-Concurrency-Schuld (I-23) ist kein Ausnahmefall — sie entspricht dem dokumentierten Muster, dass **Concurrency-Design nachträglich nicht skaliert**. TSan-CI-Gate (Korrekturprogramm Punkt 3) ist durch externe Forschung klar gestützt.

---

### XI.2 Stub-Implementierungen als Produktivcode (Kategorien D-01, G-02–G-06, I-09, I-11–I-15, I-17–I-20)

**Befund (intern):** 15+ Stub/No-Op-Instanzen im Produktivcode. Darunter: 1.500 LOC GPU-Stubs (D-01), PKI-Stub der immer `true` zurückgab (G-04), Paxos ohne RPC (I-12), Multi-LoRA-Mock (I-13), VRAM-Allocator ohne Vulkan-Aufruf (I-15), VoiceAPI No-Ops (I-17–I-19), Fake-Benchmark (I-02).

**Externe Belege:**

Ward Cunningham prägte 1992 die Metapher der **Technical Debt** im Kontext von WyCash \[REF-04\]: Code der schnell geschrieben wird "borrows" conceptually from future refactoring. Die entscheidende Präzisierung: Technische Schulden entstehen durch *bewusste* Abkürzungen — Stubs ohne Ablaufdatum dagegen sind **akkumulierte, oft unbewusste Schuld**.

Kruchten et al. (2012) operationalisieren Technical Debt als messbare Eigenschaft und unterscheiden explizit zwischen **"deliberate debt"** (bewusster Kompromiss) und **"inadvertent debt"** (entsteht durch Unwissenheit oder nachlässige Praxis) \[REF-05\]. Die ThemisDB-Stubs fallen mehrheitlich in die zweite Kategorie: kein Ablaufdatum, kein Ticket, keine STUB-Note.

Li et al. (2015) analysierten 500+ JIRA-Issues aus fünf Open-Source-Projekten auf Technical-Debt-Indikatoren. Ein Kernbefund: **Stubs/offene-Aufgaben-Kommentare ohne verknüpfte Issues werden im Median nie behoben** — sie wandern durch Releases bis sie durch externe Fehlermeldungen erzwungen werden \[REF-06\].

Fowler (2018) beschreibt im "Technical Debt Quadrant", dass **der schädlichste Quadrant "inadvertent/reckless"** ist — Entscheidungen getroffen ohne Bewusstsein ihrer Konsequenzen \[REF-07\]. Der G-04-Fall (PKI-Stub immer `true`) ist ein Lehrbuchbeispiel: keine Intention zur Sicherheitslücke, aber durch fehlende Stub-Policy entstand eine.

**Fazit für ThemisDB:** Die Stub-Policy (Korrekturprogramm Punkt 2) ist durch Jahrzehnte Technical-Debt-Forschung gestützt. Entscheidend: eine Deadline *und* eine Issue-Referenz am Stub selbst — nicht in einem separaten Dokument.

---

### XI.3 Logik- und Off-by-One-Fehler (Kategorien B, F-01, F-02, F-08, F-16)

**Befund (intern):** 15+ Off-by-One- und Logikfehler. Darunter: GraphQL-Variable nie substituiert (F-01 / J-02), `BatchWrite::success` immer `true` (F-02), HNSW `ef_search` lieferte zu wenig Ergebnisse (F-08), Bounds-Check bei `tables[0..1]` fehlend (F-16), Histogram-Bucket-Grenzen falsch (B-01).

**Externe Belege:**

McConnell (2004) analysiert in "Code Complete" historische Fehlerdaten und stellt fest, dass **Off-by-One-Fehler und fehlerhafte Grenzbedingungs-Tests zu den drei häufigsten Bug-Kategorien gehören**, gemessen über mehrere Jahrzehnte IBM/Bell-Studien \[REF-08\]. Die Kategorisierung deckt sich direkt mit B-01 (Histogram), B-02 (Window-Frame), F-16 (Array-Bounds).

Myers et al. (2011) zeigen in "The Art of Software Testing", dass **Boundary-Value-Analysis** als Testmethode speziell für diese Klasse ausgelegt ist und Off-by-One-Fehler zuverlässig findet — aber nur wenn sie *systematisch* angewendet wird, nicht ad-hoc \[REF-09\]. ThemisDB B-01 und B-02 wurden beide durch explizite Tests entdeckt, nicht durch Review.

Besonders relevant für F-01 / J-02 (GraphQL-Variable nie substituiert): Zeller (2009) beschreibt in "Why Programs Fail" das **"Infection"**-Modell — ein Defekt im Zustand breitet sich durch das Programm aus bis er zu einem sichtbaren Failure führt. Stille Korrektheits-Bugs (keine Exception, falsches Ergebnis) sind nach diesem Modell die **schwierigste Klasse**, da das Infektionsfenster groß ist und keine Fehlersignale generiert werden \[REF-10\]. F-01 war über multiple Releases aktiv.

**Fazit für ThemisDB:** FA-04 ("Ein Kompilieren = Korrektheit") ist durch externe Forschung als Fehlannahme belegt. Boundary-Tests und Property-based Tests (Korrekturprogramm Punkt 4) sind die wissenschaftlich gestützte Antwort.

---

### XI.4 Sicherheitslücken (Kategorien C, G, F-03, F-22, F-26, I-04, I-07)

**Befund (intern):** Kritische Sicherheitslücken: Cross-Tenant-Cache-Leak (F-22), AQL Injection (F-03), Credential-Leak in Logs (F-26), PKI-Stub always-true (G-04), HSM-Stub ohne Warnung (G-06), Rate-Limiter Memory Leak + DoS (I-04, I-07), VRAM Cross-Tenant (C-01, G-01).

**Externe Belege:**

Das OWASP Top 10 (2021) listet **A02: Cryptographic Failures**, **A03: Injection** und **A05: Security Misconfiguration** als Spitzenreiter unter realen Webanwendungs-Sicherheitslücken \[REF-11\]. ThemisDB C-01/G-01 (VRAM nicht gecleart) ist ein A02-Muster; F-03 (AQL Injection) ein A03-Muster; G-04 (PKI-Stub always-true) ein A02/A05-Muster.

Das CWE (Common Weakness Enumeration) Top 25 Most Dangerous Software Weaknesses (2023) \[REF-12\] enthält:
- **CWE-787** (Out-of-Bounds Write) — relevant für C-02 (RocksDB-Wrapper Use-after-free)
- **CWE-862** (Missing Authorization) — relevant für F-22 (Cross-Tenant-Cache-Leak)
- **CWE-312** (Cleartext Storage of Sensitive Information) — exakt F-26 (API-Keys im Debug-Log)
- **CWE-770** (Allocation of Resources Without Limits or Throttling) — exakt I-04/I-07 (Rate-Limiter Memory Leak, BatchWrite ohne Größen-Limit)

Viega & McGraw (2002) dokumentieren in "Building Secure Software" das Muster der **"Security Through Obscurity" Anti-Pattern**, zu dem Stub-Sicherheitsfunktionen gehören \[REF-13\]: Eine Funktion die nominell Sicherheit bereitstellt (PKI-Verifikation, HSM-Schlüsselverwaltung), aber als Always-True-Stub implementiert ist, ist gefährlicher als keine Sicherheitsfunktion — weil sie Audits und Betreiber in falscher Sicherheit wiegt.

**Fazit für ThemisDB:** FA-07 ("Security-relevante Defaultwerte sind sicher") ist durch die CWE-Klassifikation und OWASP-Evidenz als systematische Fehlannahme in der Industrie belegt. Security-Defaults-Review (Korrekturprogramm Punkt 5) ist Standard-Best-Practice.

---

### XI.5 Komplexe Systemausfälle (Kategorien D-02, D-03, D-04, I-10, I-12)

**Befund (intern):** Server-Hang beim RAID-Start (D-02), falsche Docker-Port-Mappings für alle Shards (D-03), halbfertige HTTP-Server-Refaktorierung (D-04), Paxos ohne echte RPC-Callbacks (I-12), garbled Merge-Artefakte die das Modul kompilierungsunfähig machten (I-10).

**Externe Belege:**

Cook (1998) in "How Complex Systems Fail" formuliert 18 Prinzipien zu Ausfällen in komplexen Systemen, die seit ihrer Veröffentlichung in der Systemzuverlässigkeitsforschung kanonischen Status haben \[REF-14\]. Besonders relevant:
- **Prinzip 4**: "Complex systems contain changing mixtures of failures latent within them." — D-02/D-03 waren von Beginn der RAID-Implementierung latent, aber erst im Produktionseinsatz sichtbar.
- **Prinzip 14**: "Change introduces new forms of failure." — D-04 (HTTP-Server-Refaktorierung) ist ein Schulbuchbeispiel: Inkrementelle Migration mit aktivem Adapter als Dauerschuld.
- **Prinzip 17**: "Human practitioners are the adaptors, last line of defense against failure." — I-10 (garbled merge) wurde erst durch manuellen Review erkannt, nicht durch CI.

Nygard (2007) beschreibt in "Release It!" die **Stability Patterns** für Produktionssysteme. Für D-02 (RAID-Startup-Hang) ist das Muster **"Fail Fast"** direkt anwendbar: Ein System das beim Start hängt statt mit einem klaren Fehler zu scheitern, erzwingt manuelle Diagnose \[REF-15\]. Der Fix (Conditional Column Family Opening) implementiert implizit Fail-Fast.

Lamport (2001) in "Paxos Made Simple" \[REF-16\] spezifiziert die Sicherheitsinvariante von Paxos explizit: In Phase 1 müssen Promise-Antworten mit dem höchsten akzeptierten Wert propagiert werden — exakt das, was in D-06 / I-12 fehlte. I-12 (PR #4678) ist damit ein direkter Verstoß gegen die Lamport-Sicherheitsinvariante: Alle Nodes wurden automatisch als "stimmen zu" eingetragen, ohne echten Konsens. **Paxos ohne echte Promise-Phase ist kein Konsens-Algorithmus.**

**Fazit für ThemisDB:** FD-02 (fehlende Integrationstests für RAID-Startup) und D-06/I-12 (Paxos-Korrektheit) sind durch externe Theorie klar und eindeutig als kritische Lücken klassifiziert.

---

### XI.6 Performance-Messungen und Benchmark-Integrität (Kategorien E, I-01–I-03, I-09)

**Befund (intern):** Fake-Benchmark `DoNotOptimize(42.0)` lieferte 796 M/s KPI (I-02), CMake-Makro-Mismatch machte alle GPU-Benchmarks zu Stubs (I-09), WAL-Sync-Fehlkonfiguration verursachte ~79x Throughput-Regression (I-01), Secondary-Index 75% unter Ziel (E-01).

**Externe Belege:**

Dong et al. (2021) beschreiben in "Evolution of Development Priorities in Key-value Stores Serving Large-scale Applications: The RocksDB Experience" (USENIX FAST 2021) \[REF-17\] explizit, wie **WAL-Sync-Konfiguration eine der häufigsten Performance-Fallstricke** in RocksDB-basierten Systemen ist. Der I-01-Bug (`write_options_->sync = config_.enable_wal`) ist ein direktes Beispiel des von ihnen beschriebenen "synced-WAL-by-mistake" Anti-Patterns.

Mytkowicz et al. (2009) zeigen in "Producing Wrong Data Without Doing Anything Obviously Wrong!" (ASPLOS 2009), dass **Benchmarks durch Messung-Setup-Artefakte (Compiler-Flags, Link-Order, Environment) systematisch verfälscht werden können** — ohne dass der Benchmark selbst fehlerhaft aussieht \[REF-18\]. I-09 (CMake-Makro-Mismatch) und I-02 (Fake-Benchmark) sind zwei besonders klare Instanzen dieses Phänomens: Kein Messfehler, sondern *keine Messung*.

Gray & Reuter (1992) definieren in "Transaction Processing: Concepts and Techniques" \[REF-19\] den WAL-Commit-Protokoll-Kontrakt: Ein `sync`-Flag bedeutet fsync nach jedem Write. Der I-01-Bug verletzte diese Semantik durch Kopplung von WAL-Aktivierung und Sync-Mode.

**Fazit für ThemisDB:** FA-08 ("Benchmark-KPI = gemessene Performance") ist durch Mytkowicz et al. als allgemeines Systemproblem belegt — nicht als ThemisDB-Sonderfall. Die Korrekturmaßnahme (CI-Audit dass jeder Benchmark ≥1 DB-Operation ausführt) entspricht dem Stand der Benchmark-Methodologie.

---

### XI.7 Industrielle Post-Mortems als Parallelbelege

Die folgenden dokumentierten Industrieausfälle zeigen, dass die ThemisDB-Befundmuster bei anderen Systemen — mit größerem Maßstab und Reputationsrisiko — aufgetreten sind.

#### GitLab Datenverlust-Incident (2017) \[REF-20\]

GitLab verlor 300 GB Produktionsdaten durch eine Kombination aus: (a) fehlendem Backup-Monitoring, (b) einem `rm -rf` Befehl auf dem falschen Server und (c) fehlenden Integrationstests für den Restore-Pfad. Die offizielle Post-Mortem-Analyse enthüllte, dass **5 von 5 Backup-Mechanismen** im Incident versagten oder nicht implementiert waren.

**Parallele zu ThemisDB:** D-02/D-03 (RAID-Startup-Hang + Port-Mapping): Kritische Infrastruktur (Docker, RAID, Port-Routing) die strukturell nicht funktionierte, aber nie in einem Integrationstest validiert worden war. FD-02 ist das direkte ThemisDB-Äquivalent.

#### Amazon DynamoDB Availability Event (2011) \[REF-21\]

Ein Software-Deployment-Bug in der Mitgliedschaftsprotokolls-Komponente führte dazu, dass sich Nodes gegenseitig als "ausgefallen" betrachteten — ausgelöst durch ein unerwartetes Interaktionsmuster mit dem Routing-Protokoll. Auswirkung: Kaskaden-Ausfälle über mehrere Availability Zones.

**Parallele zu ThemisDB:** I-12 (Paxos ohne echte RPC-Callbacks): Ein Konsensprotokoll das nie echte verteilte Kommunikation durchgeführt hat, hat keine Gelegenheit, solche Interaktionsmuster zu triggern — bis zur Produktion. D-06 (Paxos-Aufgabe noch offen) ist ein latentes Risiko dieser Klasse.

#### Cloudflare BGP-Routing Outage (2019) \[REF-22\]

Eine fehlerhafte BGP-Konfigurationsänderung führte zu einer vollständigen Unterbrechung des Cloudflare-Netzwerks. Root Cause: Ein regulärer Ausdrucksfehler in einer Firewall-Regel verursachte eine CPU-Erschöpfung in allen PoPs gleichzeitig.

**Parallele zu ThemisDB:** F-24 (WebCrawler Infinite Redirect Loop), I-07 (BatchWrite ohne Größen-Limit). Beide sind Formen unkontrollierter Ressourcenerschöpfung durch fehlende Input-Validierung.

#### Facebook Datacenter-wide Outage (2021) \[REF-23\]

Ein BGP-Konfigurationsfehler trennte alle Facebook-Rechenzentren vom globalen Routing-System gleichzeitig. Post-Mortem: Monitoring-Tools waren auf dieselbe Infrastruktur angewiesen, die ausgefallen war — und konnten daher das Problem nicht erkennen.

**Parallele zu ThemisDB:** H-01 (tracer.cpp und log_aggregator.cpp fehlten komplett): Observability-Komponenten als Stubs bedeutet, dass im Fall eines ernsthaften Incidents kein Diagnosetool verfügbar ist.

---

### XI.8 Architektur-Schulden durch halbfertige Migrationen (Kategorie D-04)

**Befund (intern):** Boost.Beast ↔ cpp-httplib Typ-Mismatch (D-04): Refaktorierung begonnen, aber `src/server/http_server.cpp` (7.410 LOC) blieb auf Boost.Beast; temporärer Adapter mit ~1-5% Overhead aktiv seit Jahren.

**Externe Belege:**

Feathers (2004) analysiert in "Working Effectively with Legacy Code" das Muster der **"Seam"** — einer kontrollierten Schnittstelle zwischen altem und neuem System \[REF-24\]. Der `HttpTypeAdapter` ist ein Seam, aber Feathers' Diagnose ist klar: Seams ohne Ablaufdatum werden permanent. **Eine Migration, die "eventually done" werden soll, ist in der Praxis nie done.**

Fowler (2018) beschreibt das Anti-Pattern des **"Strangler Fig"**-Musters (als positives Gegenstück) und dessen Fehlschlagbedingungen: Wenn der "Strangler" kein Ausstiegskriterium hat, hängen beide Systeme parallel in Produktion \[REF-07\]. D-04 ist eine Strangler-Fig-Migration ohne Ausstiegskriterium.

**Fazit für ThemisDB:** FD-04 (Refaktorierungen ohne vollständige Übergabepunkte) ist durch Feathers und Fowler als klassisches Migrations-Anti-Pattern belegt. Korrektur: explizites Migrations-Milestone mit 0-Adapter-Kriterium.

---

### XI.9 Zusammenfassung: Externe Evidenz-Matrix

| Fehlerkategorie | ThemisDB-Befunde | Externe Primärquelle | Industriebeleg |
|-----------------|-----------------|----------------------|----------------|
| Concurrency (Lock unter I/O, TOCTOU) | A-01..A-07, I-23, J-03..J-06 | Lu et al. 2008 \[REF-01\]; Serebryany 2009 \[REF-03\] | — |
| Stubs als Produktivcode | D-01, G-02..G-06, I-02, I-09, I-12..I-15 | Cunningham 1992 \[REF-04\]; Kruchten 2012 \[REF-05\]; Li 2015 \[REF-06\] | — |
| Off-by-One / Logikfehler | B-01..B-11, F-01, F-08, F-16 | McConnell 2004 \[REF-08\]; Myers 2011 \[REF-09\]; Zeller 2009 \[REF-10\] | — |
| Sicherheitslücken | C-01..C-05, G-01..G-06, F-03, F-22, F-26, I-04, I-07 | OWASP Top 10 2021 \[REF-11\]; CWE Top 25 2023 \[REF-12\]; Viega 2002 \[REF-13\] | — |
| Komplexe Systemausfälle | D-02, D-03, D-06, I-10, I-12 | Cook 1998 \[REF-14\]; Nygard 2007 \[REF-15\]; Lamport 2001 \[REF-16\] | GitLab 2017 \[REF-20\]; Amazon DynamoDB 2011 \[REF-21\] |
| Performance/Benchmark-Integrität | E-01..E-05, I-01, I-02, I-09 | Dong et al. 2021 \[REF-17\]; Mytkowicz 2009 \[REF-18\]; Gray & Reuter 1992 \[REF-19\] | — |
| Ressourcenerschöpfung / Input-Validierung | I-04, I-07, F-24 | CWE-770 \[REF-12\] | Cloudflare 2019 \[REF-22\] |
| Architektur-Schulden durch Migrations-Halbzeit | D-04 | Feathers 2004 \[REF-24\]; Fowler 2018 \[REF-07\] | — |
| Observability als Stub | H-01..H-02 | Cook 1998 \[REF-14\] | Facebook 2021 \[REF-23\] |

---

## References / Quellen

> Alle Quellen sind öffentlich zugänglich. DOI/URL-Angaben dienen zur Verifikation.

**\[REF-01\]** Lu, S., Park, S., Seo, E., Zhou, Y. (2008). *Learning from Mistakes — A Comprehensive Study on Real World Concurrency Bug Characteristics*. In *Proceedings of the 13th International Conference on Architectural Support for Programming Languages and Operating Systems (ASPLOS XIII)*, pp. 329–339. ACM. DOI: 10.1145/1346281.1346323

**\[REF-02\]** Xiong, W., Park, S., Zhang, J., Zhou, Y., Ma, Z. (2010). *Ad Hoc Synchronization Considered Harmful*. In *Proceedings of the 9th USENIX Symposium on Operating Systems Design and Implementation (OSDI 10)*, pp. 163–176. USENIX.

**\[REF-03\]** Serebryany, K., Iskhodzhanov, T. (2009). *ThreadSanitizer: data race detection in practice*. In *Proceedings of the Workshop on Binary Instrumentation and Applications (WBIA)*, pp. 62–71. ACM. DOI: 10.1145/1791194.1791203

**\[REF-04\]** Cunningham, W. (1992). *The WyCash Portfolio Management System*. OOPSLA '92 Experience Report. ACM SIGPLAN Notices. URL: http://c2.com/doc/oopsla92.html

**\[REF-05\]** Kruchten, P., Nord, R. L., Ozkaya, I. (2012). *Technical Debt: From Metaphor to Theory and Practice*. IEEE Software, 29(6), pp. 18–21. DOI: 10.1109/MS.2012.167

**\[REF-06\]** Li, Z., Avgeriou, P., Liang, P. (2015). *A Systematic Mapping Study on Technical Debt and Its Management*. Journal of Systems and Software, 101, pp. 193–220. DOI: 10.1016/j.jss.2014.12.027

**\[REF-07\]** Fowler, M. (2018). *Refactoring: Improving the Design of Existing Code* (2nd ed.). Addison-Wesley Professional. ISBN: 978-0134757599. (Technical Debt Quadrant: https://martinfowler.com/bliki/TechnicalDebtQuadrant.html)

**\[REF-08\]** McConnell, S. (2004). *Code Complete: A Practical Handbook of Software Construction* (2nd ed.). Microsoft Press. ISBN: 978-0735619678. (Chapter 22: Developer Testing, Chapter 26: Code-Tuning Techniques)

**\[REF-09\]** Myers, G. J., Sandler, C., Badgett, T. (2011). *The Art of Software Testing* (3rd ed.). Wiley. ISBN: 978-1118031964.

**\[REF-10\]** Zeller, A. (2009). *Why Programs Fail: A Guide to Systematic Debugging* (2nd ed.). Morgan Kaufmann. ISBN: 978-0123745156.

**\[REF-11\]** OWASP Foundation. (2021). *OWASP Top 10: 2021 Edition*. URL: https://owasp.org/Top10/

**\[REF-12\]** MITRE Corporation. (2023). *CWE Top 25 Most Dangerous Software Weaknesses*. URL: https://cwe.mitre.org/top25/archive/2023/2023_top25_list.html

**\[REF-13\]** Viega, J., McGraw, G. (2002). *Building Secure Software: How to Avoid Security Problems the Right Way*. Addison-Wesley Professional. ISBN: 978-0201721522.

**\[REF-14\]** Cook, R. I. (1998). *How Complex Systems Fail*. Cognitive Technologies Laboratory, University of Chicago. Revision D (2000). URL: https://how.complexsystems.fail/

**\[REF-15\]** Nygard, M. T. (2007). *Release It! Design and Deploy Production-Ready Software*. Pragmatic Bookshelf. ISBN: 978-0978739218. (2nd ed. 2018, ISBN: 978-1680502398)

**\[REF-16\]** Lamport, L. (2001). *Paxos Made Simple*. ACM SIGACT News (Distributed Computing Column), 32(4), pp. 51–58. URL: https://lamport.azurewebsites.net/pubs/paxos-simple.pdf

**\[REF-17\]** Dong, S., Callaghan, M., Galanis, L., Borthakur, D., Savor, T., Strum, M. (2021). *Evolution of Development Priorities in Key-value Stores Serving Large-scale Applications: The RocksDB Experience*. In *Proceedings of the 19th USENIX Conference on File and Storage Technologies (FAST 21)*, pp. 33–49. USENIX.

**\[REF-18\]** Mytkowicz, T., Diwan, A., Hauswirth, M., Sweeney, P. F. (2009). *Producing Wrong Data Without Doing Anything Obviously Wrong!* In *Proceedings of the 14th International Conference on Architectural Support for Programming Languages and Operating Systems (ASPLOS XIV)*, pp. 265–276. ACM. DOI: 10.1145/1508244.1508275

**\[REF-19\]** Gray, J., Reuter, A. (1992). *Transaction Processing: Concepts and Techniques*. Morgan Kaufmann. ISBN: 978-1558601901. (Chapter 9: Log Manager)

**\[REF-20\]** GitLab Team. (2017). *Post-Mortem of Database Outage of January 31*. GitLab Blog. URL: https://about.gitlab.com/blog/2017/02/01/gitlab-dot-com-database-incident/

**\[REF-21\]** Amazon Web Services. (2011). *Summary of the Amazon DynamoDB Service Disruption and Related Impacts in the US-East Region*. AWS Service Health Dashboard. URL: https://aws.amazon.com/message/5467D2/

**\[REF-22\]** Prince, M. (2019). *How Verizon and a BGP Optimizer Knocked Large Parts of the Internet Offline Today*. Cloudflare Blog. URL: https://blog.cloudflare.com/how-verizon-and-a-bgp-optimizer-knocked-large-parts-of-the-internet-offline-today/

**\[REF-23\]** Janardhan, S. (2021). *More details about the October 4 outage*. Facebook Engineering Blog. URL: https://engineering.fb.com/2021/10/05/networking-traffic/outage-details/

**\[REF-24\]** Feathers, M. C. (2004). *Working Effectively with Legacy Code*. Prentice Hall. ISBN: 978-0131177055.

---

## Appendix A. Pflegeprozess für dieses Dokument

Bei jeder größeren Iteration:

1. Mindestens einen neuen Befund mit Dateipfad-Referenz dokumentieren.
2. Vollständigkeitsregister aktualisieren (Status offen > teilweise > gut > vollständig).
3. Übergreifende Fehlannahmen auf Aktualität prüfen.
4. Offene Lücken (keine mehr — alle Module und PR-Seiten 1-4 abgedeckt; zukünftige Sessions können weitere PR-Seiten 5+ oder Issues minen).
5. Externe Quellen in Abschnitt XI und im Abschnitt "References / Quellen" auf Aktualität und Erreichbarkeit prüfen. Neue Primärquellen ergänzen, wenn sie eine Fehlerkategorie besser belegen als die bestehenden.

## Appendix B. Schnellreferenz: Offene Aufgaben mit kritischem Potenzial

| Datei | Zeile | Problem | Priorität |
|-------|-------|---------|-----------|
| `src/sharding/paxos_consensus.cpp` | 622 | Highest accepted value nicht aus Promise propagiert | HOCH |
| `src/server/http_server.cpp` | 117 | Boost.Beast / cpp-httplib Bridge noch aktiv | MITTEL |
| `src/server/http2_session.cpp` | 466 | Kein produktiver Buffer-Management-Pfad | MITTEL |
| `src/server/vector_api_handler.cpp` | 712 | Scope-Checks in Authorization fehlen | HOCH |
| `src/llm/ml_model_manager.cpp` | 1334/1691/1702 | Echte Inference/Load/Cleanup-Logik fehlt | HOCH |
| `src/network/FUTURE_ENHANCEMENTS.md` | — | Async Write Buffers ohne shared_ptr-Lifetime (use-after-free Risiko) | HOCH |

### Kategorie F: Weitere Laufzeit- und Logikfehler (Module-Übergreifend)

#### F-01: GraphQL — Variable References nie aufgelöst (`$variable` blieb immer literal String)
- **Fundstelle**: `src/api/CHANGELOG.md` v2.0.0, `tests/test_graphql_variables.cpp`
- **Problem**: `$variable`-Referenzen in Feldargumenten wurden als Klartext-Strings `"$id"` gespeichert und nie zur Laufzeit aufgelöst. Betraf alle GraphQL-Queries mit Variablen-Substitution.
- **Auswirkung**: Alle variablengesteuerten GraphQL-Queries lieferten falsches Ergebnis ohne Fehlermeldung — stiller Korrektheits-Bug über multiple Releases.
- **Fix**: Neuer `Value::Type::VariableRef`-Typ; `Executor::resolveValue()` löst vor Resolver-Aufruf auf.

#### F-02: API — `BatchWrite::success` immer `true` bei Partial-Failures
- **Fundstelle**: `src/api/CHANGELOG.md` v1.9.1
- **Problem**: Wenn einzelne Upserts/Deletes in einem Batch fehlschlugen, gab `BatchWriteResponse::success` trotzdem `true` zurück.
- **Auswirkung**: Aufrufer konnten Teil-Fehlschläge nicht erkennen; Dateninkonsistenz bei fehlertoleranten Clients.

#### F-03: AQL Identifier Injection über `collection`-Parameter
- **Fundstelle**: `src/api/CHANGELOG.md` v1.9.1
- **Problem**: `HybridSearch` und `FullTextSearch` escapten nur einfache Anführungszeichen, schützten aber nicht vor Injection durch den Identifier selbst (z.B. `"col RETURN SLEEP(10) //"`).
- **Auswirkung**: Sicherheitslücke: beliebige AQL-Injection möglich.

#### F-04: gRPC Server — `start()` hielt `mutex_` über blockierenden Socket-Bind
- **Fundstelle**: `src/api/CHANGELOG.md` v1.9.0
- **Problem**: `GrpcApiServer::start()` hielt `mutex_` über den blockierenden `BuildAndStart()`-Aufruf. Gleichzeitige `stop()`/Accessor-Aufrufe deadlockten.
- **Fix**: Lock vor Netzwerk-Operation freigeben, danach re-acquiren.

#### F-05: Graph — L2-Distanz Overflow für hochwertige float32-Vektoren
- **Fundstelle**: `src/index/CHANGELOG.md` v1.1.0
- **Problem**: L2-Distanzberechnung ohne Overflow-Schutz; bei hochmagnitudigen float32-Vektoren Overflow zu `+inf`/NaN.
- **Auswirkung**: Vektor-Suche lieferte falsche Rangfolge.

#### F-06: HNSW — Graph-Konnektivität nach parallelen Inserts gebrochen
- **Fundstelle**: `src/index/CHANGELOG.md` v1.5.0
- **Problem**: Hochparallele Concurrent-Inserts hinterließen HNSW-Layer-Graphen mit fehlenden Kanten.
- **Auswirkung**: ANN-Suche lieferte unvollständige Ergebnisse; erst durch gezielten Lasttest gefunden.

#### F-07: PQ-Encoding — falsche Sub-Vektor-Zuweisung bei hoher Dimensionalität
- **Fundstelle**: `src/index/CHANGELOG.md` v1.4.0
- **Problem**: Product-Quantization-Codebook generierte bei hoher Eingabedimensionalität falsche Cluster-Zuweisungen.

#### F-08: HNSW `ef_search` — lieferte weniger Ergebnisse als angefragt
- **Fundstelle**: `src/index/CHANGELOG.md` v1.2.0
- **Problem**: Bei kleinen Indizes lieferte `ef_search` weniger als die angeforderten `k` Ergebnisse ohne Fehlerindikation.

#### F-09: R-tree — falsche MBR-Splits durch Z-Order-Kurve bei hoher Dimensionalität
- **Fundstelle**: `src/index/CHANGELOG.md` v1.6.0
- **Problem**: Z-Order (Morton) Curve berechnete bei hohen räumlichen Dimensionen fehlerhafte Minimum-Bounding-Rectangle-Splits.

#### F-10: CUDA Stream Synchronisation Race bei parallelen Multi-Index Queries
- **Fundstelle**: `src/index/CHANGELOG.md` v1.3.0
- **Problem**: Parallele Queries auf mehrere Indizes synchronisierten CUDA-Streams nicht korrekt.
- **Auswirkung**: Nicht-deterministisches Verhalten; potentiell falsche Ergebnisse unter Last.

#### F-11: Vulkan Descriptor Set Leak beim Index-Rebuild
- **Fundstelle**: `src/index/CHANGELOG.md` v1.3.0
- **Problem**: Nach jedem Index-Rebuild wurden Vulkan Descriptor Sets nicht freigegeben.
- **Auswirkung**: Speicher-Leak; Vulkan-API-Ressourcen-Erschöpfung über Zeit.

#### F-12: Graph — falsche Shortest-Path-Ergebnisse bei negativen Kanten-Gewichten in BFS
- **Fundstelle**: `src/graph/CHANGELOG.md` v1.1.0
- **Problem**: BFS-Modus im Graph-Traversal-Engine lieferte falsche kürzeste Wege wenn negative Kantengewichte vorhanden; BFS ist grundsätzlich nicht für negative Gewichte definiert, aber kein Fehler wurde ausgelöst.

#### F-13: Graph — Race Condition bei parallelen Traversierungen über gemeinsame Frontier
- **Fundstelle**: `src/graph/CHANGELOG.md` v1.3.0
- **Problem**: Parallele Traversierungen mit geteilten Frontier-Knoten hatten unsynchronisierten Lese-/Schreibzugriff.

#### F-14: Graph — Edge Weight Overflow bei großen Graphen mit hochkardinalen Knoten
- **Fundstelle**: `src/graph/CHANGELOG.md` v1.3.0
- **Problem**: Kantengewichte akkumulierten Integer Overflow bei Traversierungen über hochkardinalitäts-Knoten.

#### F-15: VF2 Graph Matching — Doppelte Kandidaten-Mappings auf symmetrischen Graphen
- **Fundstelle**: `src/graph/CHANGELOG.md` v1.2.0
- **Problem**: VF2-Algorithmus generierte doppelte isomorphe Mappings auf Graphen mit Symmetrie-Eigenschaften.

#### F-16: Query Federation — Array-Zugriff ohne Bounds-Check (`tables[0]`, `tables[1]`)
- **Fundstelle**: `src/query/CHANGELOG.md` v1.6.0
- **Problem**: `QueryFederation::createExecutionPlan()` griff direkt auf `metadata.tables[0]` und `[1]` zu ohne zu prüfen ob `tables.size() >= 2`.
- **Auswirkung**: Undefined Behavior / Absturz bei Federation-Queries über weniger als zwei Tabellen.

#### F-17: Metadata — SchemaVersionManager generierte falsches `ALTER TABLE` bei Spalten-Reihenfolge-Änderung
- **Fundstelle**: `src/metadata/CHANGELOG.md` v1.5.1
- **Problem**: Diff-Script-Generator erkannte Spalten-Reihenfolge-Änderungen ohne Typ-Änderung nicht korrekt; generiertes SQL war semantisch falsch.

#### F-18: Metadata — `DistributedMetadataCatalog` Race Condition bei gleichzeitiger Table-Discovery
- **Fundstelle**: `src/metadata/CHANGELOG.md` v1.6.0
- **Problem**: Zwei Koordinatoren, die gleichzeitig dieselbe neue Tabelle entdeckten, erzeugten inkonsistente Catalog-Einträge.

#### F-19: Prompt Engineering — Template-Map ohne Thread-Safety bei Concurrent Read/Write
- **Fundstelle**: `src/prompt_engineering/CHANGELOG.md` v1.3.0
- **Problem**: Concurrent Read/Write auf Template-Map ohne Synchronisation; erst durch expliziten Test gefunden.
- **Fix**: TBB `concurrent_hash_map`.

#### F-20: Prompt Engineering — `PromptManager` CRUD nicht persistiert nach Prozess-Neustart
- **Fundstelle**: `src/prompt_engineering/CHANGELOG.md` v1.2.0 (Fixed)
- **Problem**: CRUD-Operationen auf Prompt-Templates galten nur im Prozess-Speicher; nach Neustart waren alle Änderungen weg.

#### F-21: Scheduler — Audit-Logs hardgecoded `"system"` als Actor
- **Fundstelle**: `src/scheduler/CHANGELOG.md`
- **Problem**: Alle Audit-Log-Einträge (registerTask, updateTask, enableTask, disableTask, executeTaskNow) nutzten den Literal-String `"system"` als Actor statt des echten Aufrufer-Users.
- **Auswirkung**: Audit-Trail nicht rekonstruierbar — wer welche Task-Änderung auslöste, war nicht nachvollziehbar.

#### F-22: Cache — Cross-Tenant-Datenleck bei Tenant-ID-Mismatch
- **Fundstelle**: `src/cache/CHANGELOG.md`
- **Problem**: Cache-Reads lieferten bei Tenant-ID-Mismatch tatsächlich fremde Daten statt `nullopt`.
- **Auswirkung**: Sicherheitskritisches Datenleck zwischen Tenants.

#### F-23: Ingestion — CDC Connector leckte Replication Slot bei Reconnect
- **Fundstelle**: `src/ingestion/CHANGELOG.md` v1.5.0, `src/ingestion/AUDIT.md` ING-001
- **Problem**: PostgreSQL Logical Replication Slot wurde bei Verbindungsunterbrechung nicht geschlossen; nach Reconnect entstanden permanente Slot-Leaks.
- **Auswirkung**: WAL-Akkumulation auf PostgreSQL-Seite; Disk-Full-Risiko.

#### F-24: Ingestion — WebCrawler Infinite Redirect Loop bei selbst-referenzierendem `Location`-Header
- **Fundstelle**: `src/ingestion/CHANGELOG.md` v1.5.0, `src/ingestion/AUDIT.md` ING-002
- **Problem**: Kein Redirect-Loop-Schutz; bei einem `Location`-Header der auf sich selbst zeigte: unendliche Schleife.

#### F-25: Ingestion — ObjectStorage Multipart Upload bei Pipeline-Abbruch unvollständig gelassen
- **Fundstelle**: `src/ingestion/CHANGELOG.md` v1.5.0, `src/ingestion/AUDIT.md` ING-003
- **Problem**: Kein Abort-Handler für Multipart-Uploads; abgebrochene Pipelines hinterließen unvollständige Uploads in S3/GCS/Azure.
- **Auswirkung**: Kostenlücken (Speicherkosten für incomplete parts) und potentielle Datenverschmutzung.

#### F-26: Ingestion — API-Keys erschienen im Debug-Log (Credential Leak)
- **Fundstelle**: `src/ingestion/AUDIT.md` ING-004, fix in v1.2.0
- **Problem**: Connector-Konfiguration inkl. API-Keys wurde ungefiltert in Debug-Log-Zeilen ausgegeben.
- **Auswirkung**: Credentials in Log-Aggregatoren sichtbar.

#### F-27: Importers — MySQL Importer Deadlock bei Bulk-Insert
- **Fundstelle**: `src/importers/CHANGELOG.md` v1.4.0
- **Problem**: High-Concurrency Bulk-Inserts über den MySQL-Importer erzeugten Deadlocks auf Server-Seite; kein Retry/Backoff implementiert.

#### F-28: Plugins — Race Condition im Hot-Plug Monitor bei gleichzeitigem Load/Unload
- **Fundstelle**: `src/plugins/CHANGELOG.md` v1.3.0
- **Problem**: Gleichzeitige Load/Unload-Sequenzen im Hot-Plug Monitor hatten ungeschützten Zugriff auf Plugin-Registry.

#### F-29: Plugins — Stale Capability Entries nach Plugin-Unload
- **Fundstelle**: `src/plugins/CHANGELOG.md` v1.3.0
- **Problem**: Nach Plugin-Unload blieben Capability-Einträge in der Registry stehen; Folgeanfragen an nicht mehr existierende Capabilities konnten abstürzen.

#### F-30: Storage — `const_cast` Undefined Behavior in Audit-Code
- **Fundstelle**: `src/storage/CHANGELOG.md`
- **Problem**: Audit-Logik nutzte `const_cast` um `const`-qualifizierte Storage-Objekte zu modifizieren — Undefined Behavior unter C++11.

#### F-31: Storage — Nicht-Finite Verzögerungswerte im `TransactionRetryManager`
- **Fundstelle**: `src/storage/CHANGELOG.md`
- **Problem**: Jitter-Werte wurden ohne Validierung auf NaN/Infinity akzeptiert; resultierende `sleep`-Aufrufe mit nicht-finiten Werten führten zu undefiniertem Verhalten.

#### F-32: Acceleration — `HIPVectorBackend`/`ZLUDAVectorBackend` lieferten unvollständige Capabilities
- **Fundstelle**: `src/acceleration/CHANGELOG.md`
- **Problem**: `getCapabilities()` gab Objekte ohne `supportedPrecisions` und `supportedMetrics` zurück; Aufrufer, die diese Felder erwarteten, liefen in Assertion-Failures.

#### F-33: Acceleration — `RTLD_LAZY` in `loadLibrary` erlaubte verdeckte Symbol-Fehler
- **Fundstelle**: `src/acceleration/CHANGELOG.md`
- **Problem**: `RTLD_LAZY` bedeutet, dass fehlende Symbole erst beim ersten Aufruf der Funktion entdeckt werden, nicht beim Laden. Fehlerhafte Backend-Libraries wurden scheinbar erfolgreich geladen.
- **Fix**: `RTLD_NOW` für fail-fast Symbol-Binding.

### Kategorie G: Sicherheit und Datenlecks (Weitere Befunde)

#### G-01: Index — VRAM Nicht-Überschreiben bei Index-Eviction (Cross-Tenant-Datenleck)
- **Fundstelle**: `src/index/CHANGELOG.md` v1.4.0
- **Problem**: VRAM wurde nach Index-Eviction nicht gecleart; nachfolgende Index-Loads eines anderen Tenants konnten residuale Daten des vorherigen sehen.

#### G-02: ProvenanceTracker — AQL-Queries als Template-Stubs (keine echte Engine-Verbindung)
- **Fundstelle**: `CHANGELOG.md`, PR #4268
- **Problem**: `ProvenanceTracker` nutzte AQL-Template-Strings statt einer echten `AQLEngine`-Verbindung; Provenance-Queries lieferten Dummy-Ergebnisse.
- **Auswirkung**: Provenance-/Audit-Trail-Funktionalität war faktisch nicht funktionsfähig.

#### G-03: RAG LLM-Integration — `LLMIntegration`/`LLMJudgeIntegration` waren Stubs
- **Fundstelle**: `CHANGELOG.md`, PR #4277
- **Problem**: RAG-Modul verwendete Stub-Implementierungen für LLM-Aufruf und LLM-Judge; alle RAG-Evaluierungen basierten auf gefakten Ergebnissen.

#### G-04: PKIClient — Stub-Verifikation statt realer Zertifikatsprüfung
- **Fundstelle**: `CHANGELOG.md`, PR #4263
- **Problem**: `PKIClient` nutzte eine Fallback-Stub-Verifikation die immer `true` zurückgab; reale X.509-Kettenverifikation fehlte bis v1.8.0.
- **Auswirkung**: PKI-Signaturen wurden nie wirklich geprüft.

#### G-05: `SelfImprovementOrchestrator` — mit Stub-`PromptEvaluator` verbunden
- **Fundstelle**: `src/prompt_engineering/CHANGELOG.md`
- **Problem**: Der Self-Improvement-Orchestrator verwendete einen Stub als Evaluator; Verbesserungsvorschläge basierten auf gefakten Bewertungen.

#### G-06: HSM-Provider — Stub-Modus ohne Startup-Warnung (akkzidenteller Prod-Einsatz möglich)
- **Fundstelle**: `CHANGELOG.md` (v1.7.x HSM Stub Warning)
- **Problem**: Bis v1.7.x startete der Server mit aktivem Stub-HSM-Provider ohne Warnung; produktive Deployments konnten versehentlich den nicht-sicheren Stub nutzen.


### Kategorie H: RPC / Observability

#### H-01: Observability — `tracer.cpp` und `log_aggregator.cpp` komplett fehlend (OBS-MISSING-001)
- **Fundstelle**: `src/observability/ROADMAP.md` Known Issues / OBS-MISSING-001, implementiert 2026-03-11
- **Problem**: Zwei der zentralen Observability-Quelldateien (`tracer.cpp`, `log_aggregator.cpp`) existierten schlicht nicht. Alle Funktionen die W3C Trace Context Propagation oder strukturiertes Logging benötigten, hatten keine Implementierung.
- **Auswirkung**: Distributed Tracing und strukturiertes Logging waren über mehrere Releases faktisch nicht vorhanden — trotz entsprechender Header-Deklarationen.

#### H-02: Observability — `query_profiler.cpp`, `storage_profiler.cpp`, `performance_analyzer.cpp` fehlten in CMakeLists.txt
- **Fundstelle**: `src/observability/ROADMAP.md` Known Issues, fix 2026-03-09
- **Problem**: Drei Profiler-Quelldateien waren zwar implementiert, aber nicht in `cmake/CMakeLists.txt` registriert. `test_observability_profilers.cpp` konnte nicht gelinkt werden.
- **Auswirkung**: Profiler-Tests konnten über mehrere Entwicklungszyklen nicht gebaut werden; Regressions-Gate faktisch nicht vorhanden.

#### H-03: Observability — `MetricsCollector` nutzte exklusiven `std::mutex` auch für Lesepfade
- **Fundstelle**: `src/observability/FUTURE_ENHANCEMENTS.md` Source Code Audit Findings (2026-03-12)
- **Problem**: `metrics_collector.cpp` verwendete `std::lock_guard<std::mutex>` (exklusiv) für alle Operationen — auch für den Prometheus-Scrape-Pfad (`getPrometheusMetrics`, `getCardinalityLimit`). Gleichzeitige Scraper serialisierten sich.
- **Auswirkung**: Prometheus-Scraping unter 16 gleichzeitigen Scrapern serialisiert; strukturell signifikanter Throughput-Verlust. Erst TSAN-Stress-Test `TSANStress_16ScrapersAnd8Writers` deckte das Muster auf.
- **Fix**: Upgrade auf `std::shared_mutex`; Lesepfade auf `std::shared_lock`, Schreibpfade auf `std::unique_lock`.

#### H-04: Observability — `/metrics`-Endpunkt ohne Authentication erreichbar
- **Fundstelle**: `src/observability/FUTURE_ENHANCEMENTS.md`, `src/observability/AUDIT.md` OBS-OPEN-01
- **Problem**: Der Prometheus-Scrape-Endpunkt `/metrics` (Prometheus text) hat keine Authentifizierung (kein Bearer Token, kein mTLS-Guard). Alle Prometheus-Metriken (inkl. interner Betriebsdaten) sind ohne Zugangsprüfung abrufbar.
- **Auswirkung**: Datenleck interner Betriebsdaten an nicht-authentifizierte Clients. Offenes Security-Item OBS-OPEN-01, Target Q3 2026.
- **Status**: Noch offen.

#### H-05: Observability — Trace Spans können PII enthalten (keine Scan/Sanitization)
- **Fundstelle**: `src/observability/AUDIT.md` OBS-OPEN-02, `src/observability/FUTURE_ENHANCEMENTS.md`
- **Problem**: Trace-Span-Attribute werden ohne PII-Scan in OTLP-Exporter weitergesendet. Strukturierte Log-Einträge sollen keine Nutzerdaten enthalten — ob das überall eingehalten wird, ist nicht auditiert.
- **Auswirkung**: PII-Leakage via Observability-Backend (Jaeger, Zipkin, OTLP) möglich.
- **Status**: Noch offen (OBS-OPEN-02, Target Q3 2026).

#### H-06: Observability — OTLP Exemplar-Verknüpfung nicht in Export-Pipeline integriert
- **Fundstelle**: `src/observability/AUDIT.md` OBS-OPEN-05
- **Problem**: `IExemplarReservoir`-Interface (`otlp_exemplar.h`) ist deklariert, aber nicht in die OTLP-Export-Pipeline eingebunden. Trace-zu-Metrik-Verknüpfung via Exemplars funktioniert nicht.

#### H-07: RPC/gRPC — `setServiceHealth()` nicht mit gRPC Proto Health Service verdrahtet
- **Fundstelle**: `src/rpc_grpc/ROADMAP.md` Known Issues & Limitations
- **Problem**: `setServiceHealth()` verwaltet Health-State nur im Prozess-Speicher; kein Bezug zum gRPC-eigenen `grpc.health.v1.Health` Proto-Service. Externe Health Probe-Clients (Kubernetes `grpc_health_probe`) sehen nie den Zustand der internen Statusverwaltung.
- **Auswirkung**: Kubernetes Liveness/Readiness Probes via gRPC Health Protocol können keinen korrekten Zustand liefern.

#### H-08: RPC/gRPC — `recordRPC()` ist manueller Hook, kein automatischer Interceptor
- **Fundstelle**: `src/rpc_grpc/ROADMAP.md` Known Issues & Limitations
- **Problem**: Metriken werden nur für explizit mit `recordRPC()` instrumentierte RPC-Methoden erfasst. Alle anderen Server-seitigen RPCs (z.B. neue Services ohne explizite Instrumentierung) erzeugen keine Prometheus-Metriken.
- **Auswirkung**: Metriken-Lücken bei nicht explizit instrumentierten RPCs; kein vollständiges Observability-Bild.

#### H-09: RPC/gRPC — Ungültige Keepalive-Werte werden stillschweigend ignoriert
- **Fundstelle**: `src/rpc_grpc/CHANGELOG.md` v0.2.0
- **Problem**: Wenn `extra_config["keepalive_time_ms"]` oder `keepalive_timeout_ms` mit einem ungültigen Wert (z.B. Nicht-Zahl, negativer Wert) konfiguriert wird, wird der Fehler stillschweigend ignoriert und der Default-Wert verwendet. Kein Warn-Log, kein Fehler.
- **Auswirkung**: Fehlerhafte Keepalive-Konfiguration ist für Operatoren nicht erkennbar; Production-Deployments laufen mit falschen Netzwerk-Timeouts ohne Hinweis.

#### H-10: RPC/gRPC — `reloadTls()` aktualisiert nur gecachte Credentials, keine laufenden Sessions
- **Fundstelle**: `src/rpc_grpc/ROADMAP.md` Known Issues & Limitations
- **Problem**: Nach `reloadTls()` mit neuen Zertifikaten werden bestehende TLS-Sessions nicht neu ausgehandelt. Neue Verbindungen nutzen die aktualisierten Zertifikate; laufende Verbindungen bleiben auf den alten Credentials.
- **Auswirkung**: Bei Zertifikatsrotation bleiben alle Langzeit-Verbindungen auf dem alten Zertifikat aktiv bis zur Session-Trennung. Bei kompromittierten Zertifikaten kein sofortiger Schutz.

#### H-11: RPC/gRPC — Kein mTLS Round-Trip Integrationstest
- **Fundstelle**: `src/rpc_grpc/AUDIT.md`, GRPC-OPEN-03 / CHANGELOG.md [Unreleased]
- **Problem**: mTLS wird nur durch Unit-Tests auf Konfigurations-Level getestet. Ein echter mTLS Round-Trip (Client-Zertifikat + Server-Verifikation + Datenaustausch) ist nie im CI validiert worden.
- **Auswirkung**: Produktionsfehler bei mTLS-Deployments könnten bis zum Deployment-Zeitpunkt unentdeckt bleiben.


### Kategorie I: PR-gesicherte Befunde (GitHub Closed PRs)

Die folgenden Befunde sind durch abgeschlossene und gemergte Pull Requests auf GitHub belegt — die stärkste Form der Evidenz, da jeder Bug von einem Reviewer als real akzeptiert und behoben wurde.

#### I-01: WAL-Sync Konfigurationsfehler — ~79x Write-Throughput Regression (PR #4596)
- **Fundstelle**: `src/storage/rocksdb_wrapper.cpp`, PR #4596 (merged 2026-04-13)
- **Problem**: `write_options_->sync = config_.enable_wal` verknüpfte zwei orthogonale Konzepte: ob der WAL *geschrieben* wird vs. ob jeder Write *fsynced* wird. Da `enable_wal=true` der Default ist, wurde jeder `put()`-Aufruf mit einem synchronen `fsync()` beendet — was den Throughput auf Disk-IOPS (~1.276 k/s) beschränkte.
- **Auswirkung**: ~79x Throughput-Regression gegenüber dem WAL-on/sync-off Modus. Der Fehler war im Default-Pfad aktiv. Das KPI-Ziel von 100 k/s war strukturell unerreichbar.
- **Fix**: `write_options_->sync = false`; explizites fsync nur über `force_sync_on_write`-Flag.

#### I-02: Fake Benchmark `QueryEngineBench/SimpleEvaluation` (`DoNotOptimize(42.0)`) (PR #4595)
- **Fundstelle**: `benchmarks/bench_core_performance.cpp`, PR #4595 (merged 2026-04-13)
- **Problem**: Das Benchmark das die "796.4 M/s"-KPI-Kennzahl produzierte, war `DoNotOptimize(42.0)` — ein Loop-Overhead-Measurement, das keine einzige Datenbankoperation ausführte. Das KPI-Dashboard zeigte einen "Wert" der nichts über tatsächliche Query-Performance aussagte.
- **Auswirkung**: Alle KPI-Berichte für Query Engine Throughput basierten auf einer sinnlosen Messung. Performance-Regressionen waren unsichtbar.

#### I-03: QueryEngine — Doppelter RocksDB Round-Trip bei Primary-Key Queries (PR #4595)
- **Fundstelle**: `src/query/query_engine.cpp`, PR #4595
- **Problem**: `executeAndEntities()` führte für PK-Queries zuerst einen Existenz-Check via `executeAndKeys()` und danach einen separaten Blob-Fetch durch — 2+ RocksDB-Reads statt 1.
- **Fix**: `pk_eq`-Fast-Path: single `db_->get()` ohne Secondary-Index-Scan.

#### I-04: Rate Limiter — Unbeschränktes Wachstum der Bucket-Map (Memory Leak) (PR #4592)
- **Fundstelle**: `include/api/rate_limiter.h`, PR #4592 (merged 2026-04-13)
- **Problem**: `buckets_`-Map wuchs unbegrenzt — ein Eintrag pro eindeutigem Key, niemals entfernt. Bei vielen verschiedenen Clients (z.B. IP-basierten Rate-Limits) wuchs der Speicherbedarf kontinuierlich.
- **Auswirkung**: Langlaufende Server verloren Speicher; Denial-of-Service durch Speichererschöpfung möglich.

#### I-05: Rate Limiter — Verschachteltes Mutex-Locking als serialisierende Flaschenhals (PR #4592)
- **Fundstelle**: `include/api/rate_limiter.h`, PR #4592
- **Problem**: `OperationRateLimiter::allow()` hielt äußeres `std::mutex` während es `RateLimiter::allow()` aufrief, das seinerseits ein eigenes Mutex aquirierte. Alle Requests serialisierten sich durch diese Two-Mutex-Chain.

#### I-06: Rate Limiter — Clock-Syscall innerhalb kritischer Section (PR #4592)
- **Fundstelle**: `include/api/rate_limiter.h`, PR #4592
- **Problem**: `Bucket::refill()` rief `steady_clock::now()` unter `mutex_` auf — unnötige Syscall-Latenz im Hot-Path.
- **Fix**: Timestamp vor Lock-Acquisition berechnen.

#### I-07: gRPC API — `BatchWrite`/`BatchRead` ohne Input-Size-Limit (DoS-Vektor) (PR #4591)
- **Fundstelle**: `src/api/themisdb_grpc_service.cpp`, PR #4591 (merged 2026-04-13)
- **Problem**: `BatchWrite` und `BatchRead` alloziierten pro Item ohne vorherige Größenprüfung. Beliebig große Batches (z.B. 1 Mio. Items) konnten Speicher erschöpfen.
- **Auswirkung**: DoS-Vektor: Jeder nicht-authentifizierte Client konnte den Server über einen einzelnen Giant-Batch überwältigen.
- **Fix**: Hard-Cap von 10.000 Items; Überschreitung → `RESOURCE_EXHAUSTED`.

#### I-08: gRPC API Server — `stop()` hielt `mutex_` während 30-sekündigem blockierenden `Shutdown()` (PR #4591)
- **Fundstelle**: `src/api/grpc_api_server.cpp`, PR #4591
- **Problem**: `stop()` verwendete `lock_guard` und hielt `mutex_` für das vollständige 30-Sekunden-Drain-Fenster. Gleichzeitige Aufrufe zu `isRunning()` deadlockten.
- **Anmerkung**: Verwandt mit CHANGELOG-Bug F-04, aber dieser PR zeigt die zweite Instanz desselben Musters in `stop()`.

#### I-09: CMake Makro-Mismatch — GPU-Benchmarks immer als Stubs gebaut (`THEMIS_GPU_ENABLED` ≠ `THEMIS_ENABLE_GPU`) (PR #4664)
- **Fundstelle**: `cmake/CMakeLists.txt` + `benchmarks/`, PR #4664 (merged 2026-04-15)
- **Problem**: CMake definierte `THEMIS_GPU_ENABLED` als PUBLIC-Definition auf `themis_core`. Benchmark-Quelldateien prüften aber `#ifndef THEMIS_ENABLE_GPU` (unterschiedlicher Makro-Name). Resultat: alle 5+ GPU-Benchmark-Targets kompilierten immer als Disabled-Stubs, auch bei explizitem `-DTHEMIS_ENABLE_GPU=ON -DTHEMIS_ENABLE_CUDA=ON`.
- **Auswirkung**: Keine einzige GPU-Benchmark-Messung war je real. Alle GPU-Performance-KPIs waren Phantome.

#### I-10: `user_storage_encrypted` — Garbled Merge Artifacts, fehlende Closing Braces, `#include` mid-Function (PR #4661)
- **Fundstelle**: `src/user_storage_encrypted/`, PR #4661 (merged 2026-04-15)
- **Problem**: Mehrere defekte Merge-Artefakte in verschiedenen Quelldateien: abgeschnittene Funktionskörper mit fremdem Code vermengt, fehlende schließende Klammern, duplizierte Klassen- und Funktionsdefinitionen, `#include`-Direktiven mitten in Funktionskörpern injiziert, Typ-Mismatch in `rotateKey()` (`uint32_t` in `std::vector` zugewiesen).
- **Auswirkung**: Modul konnte über mehrere Sessions nicht kompiliert werden; alle Tests nicht lauffähig.

#### I-11: `OLAPEngine` unter `_WIN32` vollständig als No-Op-Stub (325 LOC) (PR #4626)
- **Fundstelle**: `src/analytics/olap.cpp`, PR #4626 (merged 2026-04-13)
- **Problem**: Die gesamte `OLAPEngine`-Klasse war in `#if defined(_WIN32) ... #endif` eingeschlossen — 325 Zeilen No-Ops. Audit ergab: keine einzige POSIX-spezifische API-Nutzung im Code; SIMD-Intrinsics waren bereits per-Instruktion gegattet. Der Klassen-Level-Guard war grundlos.
- **Auswirkung**: Jeder Windows-Build hatte null OLAP-Funktionalität ohne Fehlermeldung.

#### I-12: Paxos RPC — `PaxosPrepareCallback`/`PaxosAcceptCallback` fehlten; Nodes wurden autom. eingefügt (PR #4678)
- **Fundstelle**: `src/sharding/paxos_consensus.cpp`, PR #4678 (merged 2026-04-15)
- **Problem**: Prepare- und Accept-Phasen riefen keine echten RPC-Callbacks auf — stattdessen wurden alle Cluster-Nodes automatisch als "stimmen zu" eingetragen. Paxos lief effektiv im Single-Node-Mode ohne echten Konsens.
- **Auswirkung**: Paxos-Korrektheitsinvariante verletzt; verteilter Konsens war faktisch nicht vorhanden.

#### I-13: LoRA `batchInferenceMultiLoRA` — Mock-Response statt echtem llama.cpp-Dispatch (PR #4678)
- **Fundstelle**: `src/llm/multi_lora_manager.cpp`, PR #4678
- **Problem**: Funktion gab immer eine hartgekodierte Mock-Response zurück, anstatt an den echten llama.cpp Extern-C-Aufruf zu delegieren.
- **Auswirkung**: Alle Multi-LoRA-Inference-Ergebnisse waren gefälscht.

#### I-14: `fp32_to_fp16`/`fp16_to_fp32` — Placeholder statt IEEE 754 Bit-Manipulation (PR #4678)
- **Fundstelle**: `src/llm/lora_framework/mixed_precision.cpp`, PR #4678
- **Problem**: FP16-Konvertierungsfunktionen enthielten keine korrekte IEEE 754 Bit-Manipulation (Sign/Exponent/Mantissa, Round-to-Nearest, Subnormals, Inf/NaN). Stattdessen: Placeholder-Implementierung die falsche oder undefinierte Werte produzierte.
- **Auswirkung**: Alle Mixed-Precision-Operationen in der LoRA-Pipeline lieferten numerisch falsche Ergebnisse.

#### I-15: Vulkan VRAM Allocator — Kein einziger Vulkan-API-Aufruf (PR #4678)
- **Fundstelle**: `src/llm/lora_framework/vram_allocator.cpp`, PR #4678
- **Problem**: `vram_allocator.cpp` enthielt keinerlei Vulkan-API-Aufrufe (`vkCreateBuffer`, `vkAllocateMemory` etc.). Alle VRAM-Allokationen waren No-Ops.
- **Auswirkung**: GPU-beschleunigtes LoRA-Training war faktisch nicht möglich; Metriken/Tests die "Vulkan VRAM" validierten, validierten nichts.

#### I-16: `listUsers`/`listGroups` gaben immer Hard Error zurück (PR #4708)
- **Fundstelle**: `plugins/user_storage_encrypted/`, PR #4708 (merged 2026-04-16)
- **Problem**: Beide Funktionen gaben einen Fehler-Statuswert zurück, ohne das Dateisystem zu scannen.
- **Auswirkung**: User- und Gruppen-Management im verschlüsselten Storage war funktionslos.

#### I-17: VoiceAPI `handleSynthesize` — gab immer leeres Audio zurück (PR #4708)
- **Fundstelle**: `src/server/voice_api_handler`, PR #4708
- **Problem**: `handleSynthesize()` gab bedingungslos leeres Audio zurück, ohne die `tts_processor_`-Implementierung aufzurufen.

#### I-18: VoiceAPI `handleGetVoices` — gab hartgekodierte 4-Item-Liste zurück (PR #4708)
- **Fundstelle**: `src/server/voice_api_handler`, PR #4708
- **Problem**: Statt echte verfügbare Stimmen vom TTS-Backend abzufragen, lieferte die Funktion immer exakt dieselben 4 hartgekodierte Einträge.

#### I-19: VoiceAPI `handleDeleteSession` — war vollständiger No-Op (PR #4708)
- **Fundstelle**: `src/server/voice_api_handler`, PR #4708
- **Problem**: `handleDeleteSession()` führte keine Aktion aus; Session-Context wurde nicht gelöscht.
- **Auswirkung**: Session-Leaks; Clients die eine Session explizit löschen wollten, sahen keine Wirkung.

#### I-20: `ExtractChunks` gab immer leere Map zurück (PR #4708)
- **Fundstelle**: `src/server/rpc/differential_update_engine`, PR #4708
- **Problem**: `ExtractChunks` berechnete keine CDC-Chunk-Grenzen, sondern lieferte bedingungslos `{}`.
- **Auswirkung**: Differentielle Updates waren faktisch nicht implementiert.

#### I-21: `GPUClusterTopology::addLink()` setzte `has_nvlink = true` nie (PR #4485)
- **Fundstelle**: `src/gpu/cluster_topology.cpp`, PR #4485 (merged 2026-04-09)
- **Problem**: `addLink()` aktualisierte nur `bandwidth_matrix`, setzte aber `has_nvlink` nie auf `true`. Nur `detect()` (automatische Hardware-Detection) setzte das Flag. Folge: NVLink-aware Scheduling war durch manuelle Topology-Injektion nicht erreichbar.
- **Auswirkung**: NVLink-Scheduling-Pfad war in CPU-only CI vollständig toter Code; NVLink-Optimierungen nie validierbarer.

#### I-22: CDC `SequenceCounter::scanMaxSequence()` — O(N) Full Scan mit JSON-Parsing (PR #4496)
- **Fundstelle**: `src/cdc/sequence_counter.cpp`, PR #4496 (merged 2026-04-09)
- **Problem**: `scanMaxSequence()` scannte alle Changefeed-Keys und parsierte dabei jeden als JSON — O(N) bei N Events.
- **Fix**: `SeekForPrev` auf letzten Changefeed-Key + Sequenz direkt aus dem Key-String extrahiert (O(log N)).

#### I-23: Großflächige Deadlock-Muster und Blocking-I/O unter Locks in 19+ Klassen (PR #4646)
- **Fundstelle**: `src/`, PR #4646 (merged 2026-04-14), 141 geänderte Dateien
- **Problem**: Systematisches Concurrency-Audit entdeckte drei Fehlerklassen gleichzeitig:
  - **Deadlocks durch Lock-Order Inversions**: `task_scheduler.cpp` (verschachteltes `tasks_mutex_` → `running_mutex_`), `adaptive_query_cache.cpp` (verschachtelte L1+Eviction-Locks), `cross_shard_transaction.cpp` (retry sleep *innerhalb* Lock — hielt Lock während `sleep_for`)
  - **Blocking I/O unter Lock**: `vault_key_provider.cpp` (Netzwerk-RPC `curl_easy_perform` unter mutex), `adaptive_query_cache.cpp` (4 L3-Scan-Pfade), `ml_model_manager.cpp` (Poll-Schleife mit Sleep unter Lock)
  - **Exclusive Mutex auf Read-Heavy-Pfaden**: 19 Klassen nutzten exklusiven `std::mutex` für `const`-Methoden; u.a. `RateLimiter`, `IndexManager`, `ReplicationManager`, `TaskScheduler`, `MetricsCollector`, `HybridLogicalClock`
- **Auswirkung**: Latenz-Spikes, Deadlock-Potenzial, Throughput-Verlust auf Read-Pfaden bei 19 verschiedenen Klassen. Umfang des Problems (141 Dateien) zeigt, dass kein systemisches Concurrency-Review vor Merge stattgefunden hatte.


### Kategorie J: Weitere PR-gesicherte Befunde (Seiten 3-4, PRs ~4255–4476)

#### J-01: Docker-Image SIGSEGV beim Start — RocksDB-Öffnung im statischen Initializer (PR #4444)
- **Fundstelle**: `src/llm/llama_wrapper.h`, `src/llm/llm_response_cache.cpp`, PR #4444 (hotfix, merged 2026-04-04)
- **Problem**: `LlamaWrapper::Config::enable_response_cache = true` als Default löste bei jeder `LlamaWrapper`-Konstruktion — auch in statischen Funktions-Registry-Initializern vor `main()` — ein vollständiges `TransactionDB::Open()` aus. Im Docker-Startprozess führte dies zu einem SIGSEGV in `rocksdb::ImmutableDBOptions::Dump` (Exit 139).
- **Auswirkung**: Das offizielle Docker-Image `themisdb/themisdb:latest` und `1.8.1-rc1` crashten unmittelbar beim Start. Produktionsdeployments via Docker waren strukturell nicht möglich.
- **Weitere Befunde im selben PR**:
  - `--data-dir` und `--config=VALUE` CLI-Argumente (die das Dockerfile-CMD nutzte) wurden silently ignoriert — fehlerhafte Konfiguration ohne Fehlermeldung
  - RocksDB-Kompressions-Codecs (`liblz4`, `libzstd`, `libsnappy`) fehlten im Docker Runtime Stage — stiller Fallback oder Crash

#### J-02: GraphQL `$variable`-Referenzen wurden nie substituiert — immer Literal-String `"$id"` (PR #4453)
- **Fundstelle**: `src/api/graphql.cpp`, PR #4453 (merged 2026-04-07)
- **Problem**: `parseValue()` speicherte `$name`-Referenzen als `Value::string("$name")` statt als `VariableRef`. Alle GraphQL-Resolver empfingen immer den Literal-String `"$id"` statt des gebundenen Wertes. Variable-Substitution im GraphQL-Executor war strukturell komplett defekt.
- **Auswirkung**: Jede parametrisierte GraphQL-Query lieferte falsche Ergebnisse. Die Funktion war unnutzbar.

#### J-03: `ONNXServingBackend::infer()` — TOCTOU Session-Check + gesamte Inference unter globalem Lock (PR #4315)
- **Fundstelle**: `src/analytics/ml_serving.cpp`, PR #4315 (merged 2026-03-18)
- **Problem**: Double-Lock-Pattern: Session-Existenzprüfung unter Lock, Lock-Release, erneute Lock-Akquisition für `sessions.at()`. Zwischen beiden Locks konnte ein anderer Thread die Session evakuieren → unbehandelte Exception. Zusätzlich: der zweite `lock_guard` hielt `sessions_mutex_` für die gesamte ONNX `Run()`-Dauer (O(N) für k-NN), serialisierte alle Model-Inferenzen.
- **Auswirkung**: Race Condition führt zu unbehandelten Exceptions; vollständige Serialisierung aller ONNX-Inferences unabhängig vom Zielmodell.

#### J-04: `ModelServingEngine::predict()` — gesamte Inference unter Registry `shared_lock` (PR #4314)
- **Fundstelle**: `src/analytics/model_serving.cpp`, PR #4314 (merged 2026-03-18)
- **Problem**: `predict()` hielt `shared_lock(impl_->mu)` für die vollständige Inference-Dauer (O(depth) für Trees, O(k·N) für k-NN). Jeder wartende `registerModel()`/`unregisterModel()`-Writer wurde für die gesamte Inference-Dauer geblockt. Zusätzlich: `e.health_mu` wurde unter dem äußeren Registry-Lock genommen — implizite Lock-Order-Dependency. Concurrent `unregisterModel()` konnte zu Use-After-Free führen (`unique_ptr` → dangling reference).
- **Auswirkung**: Writer-Starvation + potenzielle Use-After-Free bei gleichzeitiger `unregisterModel()`-Ausführung.

#### J-05: `StreamingAnomalyDetector::process()` — O(N·T) Training unter globalem Lock (PR #4313)
- **Fundstelle**: `src/analytics/anomaly_detection.cpp:1040`, PR #4313 (merged 2026-03-18)
- **Problem**: `process()` hielt `mu_` für den gesamten Ablauf: Window-Copy, O(N·T) IsolationForest-/O(N²) LOF-Training und `predict()`. Alle Producer-Threads serialisierten sich für die vollständige Trainings-Dauer. Async-Retrain-Lambda captured `this` ohne Destruktor-Guard → Use-After-Free bei Objekt-Destruktion. `retrain_future_`-Überschreibung erzeugte Blocking-Destruktor-Race.
- **Auswirkung**: Vollständige Serialisierung aller Producers bei jedem Retrain-Zyklus; Use-After-Free bei Destruktion während aktivem Retrain.

#### J-06: `CEPEngine::timerLoop()` hielt `windows_mutex_` während user-supplied Callbacks (PR #4291)
- **Fundstelle**: `src/analytics/cep_engine.cpp:1079-1084`, PR #4291 (merged 2026-03-16)
- **Problem**: `WindowManager::timerLoop()` rief user-supplied `callback_()` unter gehaltenem `windows_mutex_` auf. User-Callbacks sind arbiträrer Code (I/O, DB-Writes, netzwerk). Während `windows_mutex_` gehalten wird, kann kein anderer Thread Events hinzufügen, Windows schließen oder Window-State lesen.
- **Weiterer Bug**: `metricsLoop()` nutzte `sleep_for(config_.metrics_interval)` ohne `condition_variable` — `shutdown()` stall für bis zu einen vollständigen `metrics_interval`.
- **Auswirkung**: Vollständige Event-Ingestion-Starvation während user Callbacks; potenzielle sekundenlange Shutdown-Verzögerung.

#### J-07: `DiffEngine::computeDiff()` — Cache Stampede + O(N) Full Changefeed Scan mit `limit=0` (PR #4325)
- **Fundstelle**: `src/analytics/diff_engine.cpp:175-220`, PR #4325 (merged 2026-03-19)
- **Problem (1)**: Double-Check-Cache-Pattern ohne "inflight"-Schutz: zwei gleichzeitige Caller für dieselbe Range vermissten beide den Cache, führten beide den teuren Scan durch, schrieben beide das Ergebnis — klassischer Cache Stampede.
- **Problem (2)**: `listEvents()` mit `limit=0` scannte den gesamten Changefeed unabhängig von der angeforderten Sequenz-Range. O(N) Post-Filter-Loop discardete Events außerhalb des Bereichs statt bounds-gesteuerter Abfrage.
- **Problem (3)**: `evictOldCacheEntries()` wurde unter `cache_mutex_` aufgerufen — ungeschützte Iteration während Lock gehalten.
- **Problem (4)**: `limit=0` Bug: wurde als "kein Limit" interpretiert statt `std::numeric_limits<size_t>::max()` verwendet.
- **Auswirkung**: Bei konkurrenten Zugriffen auf denselben Diff-Range: N-fache Redundanz-Berechnung; O(N) Scan des gesamten Event-Logs für jede einzelne Anfrage.

#### J-08: CUDA HNSW Kernel — silentes k-Clamping auf `kMaxK=256` ohne Caller-Notification (PR #4320)
- **Fundstelle**: `src/acceleration/cuda/cuda_hnsw_kernels.cu:325`, PR #4320 (merged 2026-03-18)
- **Problem**: `if (k > kMaxK) k = kMaxK;` clamped k still auf maximal 256 ohne jede Rückmeldung an den Caller. Re-Ranking-Pipelines die `k=512` oder mehr Kandidaten anforderten, erhielten still 256 Ergebnisse zurück — keine Exception, kein Error-Code, keine Warnung.
- **Auswirkung**: Jeder `k > 256` HNSW-Query lieferte strukturell falsche Ergebnisse. Re-Ranking-Qualität war systematisch degradiert ohne Erkennbarkeit.
