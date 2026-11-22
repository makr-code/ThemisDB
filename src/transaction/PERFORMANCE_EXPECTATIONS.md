---
Author: "ThemisDB Contributors"
Created: "2026-10-31"
Last Updated: "2026-11-21"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

Source level: 1; SOT domain: module-behavior. Canonical references: `benchmarks/benchmark_target_mapping.json`, `benchmarks/transaction/bench_transaction_throughput.cpp`, `benchmarks/transaction/bench_transaction_phase4.cpp`, `benchmarks/transaction/bench_transaction_dedicated_gates.cpp`, and `benchmarks/maintenance/bench_saga_compensation.cpp`. Related issue: #7390.

# PERFORMANCE_EXPECTATIONS — src/transaction

## Scope
- Modul: `src/transaction`
- Diese Datei dokumentiert die modulspezifischen, messbaren Performance-Erwartungswerte (Ops/s, Latenz, Throughput) für Release-Gates.
- Primärquelle: `benchmarks/benchmark_target_mapping.json` (Ziel-ID ↔ Benchmark-Fall). Die Werte sind Zielvorgaben aus dem Mapping, keine hier bestätigten Messergebnisse.

## Benchmark-Bezug
- Relevante Benchmark-Dateien:
  - `benchmarks/transaction/bench_transaction_throughput.cpp`
  - `benchmarks/maintenance/bench_saga_compensation.cpp` (Mapping source; not registered in `benchmarks/transaction/CMakeLists.txt`)
  - `benchmarks/transaction/bench_transaction_phase4.cpp`
  - `benchmarks/transaction/bench_transaction_dedicated_gates.cpp`

`TransactionBenchmarkFixture/ReadOnlyTransaction` performs ten deterministic
point reads through `Transaction::readEntityJson` per transaction from the
1,000 pre-populated entities and validates each result. This case has no
standalone expectation here; the existing target mapping is unchanged.

`TransactionBenchmarkFixture/MixedTransaction` reads each selected user's
stored JSON, updates its existing numeric balance, and writes the entity back
within the same transaction. Missing entities, invalid balances, failed writes,
and failed commits invalidate the benchmark run rather than being measured as
successful work.

The commit-latency, abort, savepoint, and OCC put/erase cases likewise validate
transaction creation and every measured write, savepoint operation, rollback,
and commit. A failed operation stops the case instead of allowing a partial
transaction to count as a successful iteration; OCC conflict outcomes remain
separately counted by the read-version/update case.

All `TransactionBenchmarkFixture` workloads use Google Benchmark's
`UseRealTime()`, so their reported rates and latencies use wall time rather
than CPU time. This measurement-mode change does not alter thresholds or
target mappings.

The TX-BM-01..04 cases in `bench_transaction_dedicated_gates.cpp` are skipped
via `SkipWithMessage`:
they exercise only in-process no-op stubs and are not commit/abort/2PC latency
or throughput evidence. Their targets remain unverified; the production-API
measurement follow-up and required evidence are tracked in
`src/transaction/ROADMAP.md`.

## Spezifische Erwartungswerte
Parametrisierte Benchmark-Fälle werden mit dem exakten Argument-Suffix angegeben,
wenn ein SLO nur für eine Workload-Größe gilt. So prüft der Comparator nicht
versehentlich Geschwisterfälle mit anderen Größen.

| Ziel-ID | Erwartungswert | Benchmark-Fall |
|---|---|---|
| TX-1 | P50 <= 100 us | `TransactionBenchmarkFixture/CommitLatency/1` |
| TX-2 | P99 <= 5 ms | `TransactionBenchmarkFixture/CommitLatency/100` |
| TX-3 | > 6 k/s (items_per_second = ops/s; one item is one committed transaction) | `TransactionBenchmarkFixture/WriteOnlyTransaction` |
| TX-4 | <= 5 ms | `TwoPhaseCommitFixture/TwoPhaseCommitLatency/5` |
| TX-5 | <= 20 ms | `SagaBenchmarkFixture/DatabaseWriteCompensation/50` |
| TX-6 | <= 1 us direct predictor-call latency | `DeadlockDetectionFixture/DeadlockDetectionOverhead_Predict` |
| TX-7 | < 5 % (benchmark counter: false_conflict_rate_percent) | `TransactionBenchmarkFixture/OccReadVersionAndUpdate` |
| TX-8 | > 90 % (benchmark counter: success_rate_percent) | `TransactionBenchmarkFixture/OccOptimisticPut` |
| TX-9 | > 5 k/s (items_per_second = ops/s; one item is one completed 2PC transaction) | `DistributedPhase4Fixture/DistributedThroughput_2PC_3Participants` |
| TX-10 | P99 <= 50 ms | `DistributedPhase4Fixture/DistributedThroughput_2PC_3Participants` |
| TX-11 | P99 <= 50 ms | `DistributedPhase4Fixture/DistributedThroughput_AbortPath` |

## Validierung
- TX-1/TX-2: `CommitLatency` exportiert `p50_real_time` bzw. `p99_real_time` als nearest-rank-Perzentil einzelner `steady_clock`-Samples ausschließlich um den erfolgreichen `commitTransaction`-Aufruf; Setup und Writes liegen außerhalb dieses Messfensters. Der Comparator wertet diese Felder in `time_unit=us` aus. Lokale Release-Messwerte müssen weiterhin erhoben werden.
- TX-6 verwendet einen absoluten, direkt messbaren Grenzwert von 1 µs; es wird kein relatives Prozentverhältnis zu einem Transaktions-Baseline-Lauf behauptet. TX-7 und TX-8 deklarieren die exportierten Prozentzähler explizit, sodass der Comparator die Verhältnisse statt beliebiger Zeit- oder Durchsatzwerte prüft.
- TX-7 zählt konfliktbedingt abgebrochene OCC-Updates geteilt durch alle abgeschlossenen Versuche. Weil der Benchmark seriell läuft, sind solche OCC-Konflikte in diesem Profil false conflicts; andere Fehler brechen die Messung ab. TX-8 ist ausdrücklich ein nicht konkurrierender Einzelthread-Insert-Baseline-Fall, keine Contention-Messung.
- TX-3 bereitet pro Iteration zehn Entities mit festem Zufalls-Seed während `PauseTiming()` vor. Die gemessene Transaktion behält `beginTransaction`, Entity-Lookup, zehn `putEntity`-Aufrufe und Commit bei; Zufallswert- und Entity-Konstruktion sind nicht Teil der Transaktionsrate.
- TX-4 nutzt `UseRealTime()`, weil der gemessene 2PC-Pfad auf synchrone WAL-I/O wartet. So basieren Latenz und `kIsRate`-Durchsatzcounter auf Wandzeit statt CPU-Zeit; eine auf null gerundete CPU-Zeit darf kein `Infinity`-JSON erzeugen.
- TX-5 prüft Forward-Writes und Compensation-Erases; sein gemeldeter `real_time`-Wert ist nun mittels `UseManualTime()` die reine Kompensations-Wandzeit für den größten registrierten 50-Write-Fall. Das simulierte `SimpleCompensation`-Fallback wurde entfernt, und jedes Saga-Fixture bekommt ein isoliertes DB/WAL-Verzeichnis.
- Lokale TX-3-Messungen am 2026-11-21 sind nicht stabil: Zwei Smoke-Läufe mit `--benchmark_min_time=0.2s` ergaben 5.145 k/s (FAIL) und 6.003 k/s (knapper PASS). Drei Wiederholungen mit `--benchmark_min_time=1s` ergaben 5.185, 5.921 und 6.040 k/s (Mittelwert 5.715 k/s, CV 8.11 %). Das belegt keine reproduzierbare Erfüllung des bestehenden Ziels von >6 k/s und ist keine neue Baseline; Mapping und Schwelle bleiben unverändert.
- Der registrierte CTest-Smoke vom 2026-11-21 maß für TX-3 5.281 k/s (FAIL), für TX-4 90.418 ms gegen 5 ms (FAIL) und für TX-6 1.263 µs gegen 1 µs (FAIL). Diese kurz laufende, lokale Smoke-Evidenz ist keine Regressions-Baseline und ersetzt keine repräsentative Wiederholungsserie.
- Der erneute CTest-Lauf `bench_transaction_throughput_wave_a_smoke` vom 2026-11-21 (lokaler Release/Ninja-Build) maß TX-3 mit 5.596 k/s gegen >6 k/s, TX-4 mit 55.744 ms gegen <=5 ms und TX-6 mit 1.161 µs gegen <=1 µs; alle drei Gates bleiben verfehlt. Für TX-6 wurde die Erzeugung zufälliger Candidate-Strings aus der gemessenen Schleife verlagert; gegenüber dem unmittelbar vorherigen Wert von 1.283 µs ist dies eine Verbesserung, aber kein Gate-Pass. Schwellenwerte und Zielzuordnungen bleiben unverändert. Der lokale Smoke ist diagnostisch, nicht repräsentativ und keine Release-Baseline.
- Der CTest-Lauf `bench_transaction_throughput_wave_a_smoke` vom 2026-11-22 bestand den Benchmark-Smoke vor der anschließenden Wiederverwendung des TX-3-Entity-Vektors; TX-3 maß 2.036 k/s, TX-2 29.316 ms, TX-4 117.005 ms und TX-6 1.547 µs, jeweils außerhalb des bestehenden Gates. Drei direkte TX-3-Wiederholungen mit `--benchmark_min_time=1s` ergaben in diesem Stand im Mittel 2.164 k/s bei 29,14 % CV. Der nachgelagerte globale Postprozessor schlug erwartungsgemäß auch wegen 150 nicht ausgeführter Erwartungswerte fehl. Diese lokalen Werte sind diagnostisch und keine repräsentative Baseline; Grenzwerte und Zuordnungen bleiben unverändert.
- Nach Wiederverwendung des Entity-Vektors in TX-3 ergaben drei direkte Wiederholungen mit `--benchmark_min_time=1s` 5.876, 4.555 und 4.564 k/s (Mittelwert 4.998 k/s, CV 15,20 %). Die Entity-Konstruktion und -Zerstörung liegt damit außerhalb der gemessenen Transaktion; TX-3 verfehlt das unveränderte Ziel weiterhin, und eine CTest-Wiederholung dieses Quellstands steht noch aus.
- Der folgende CTest-Lauf vom 2026-11-22 nach dem TX-4-`UseRealTime()`-Fix bestand `bench_transaction_throughput_wave_a_smoke`; TX-3 maß 5.051 k/s und TX-4 423.301 ms, beide weiterhin außerhalb ihrer Gates. TX-6 bestand das 1-µs-Gate. Der Postprozessor akzeptierte die Benchmark-JSONs und meldete nur TX-3/TX-4 als Leistungsfehler; zusätzlich blieben 150 globale Ziele nicht ausgeführt. Drei direkte TX-6-Wiederholungen nach Wiederverwendung des begrenzten Pair-Key-Scratch-Puffers maßen 0.516, 0.553 und 0.621 µs (Mittelwert 0.563 µs). Der direkte TX-4-Export für 3 und 5 Shards war ebenfalls strict-JSON-valide und enthielt endliche Durchsatzwerte. Die Smoke-Messungen sind nicht repräsentativ und keine Release-Baseline.
- Nach Herabstufung der regulären Commit-/Rollback-Erfolgsmeldungen in `TransactionManager`, `DistributedTransactionManager` und `TwoPhaseCommitCoordinator` von INFO auf DEBUG ergab ein direkter Release-Lauf am 2026-11-22 (je Fall 1 s Mindestzeit, 3 Wiederholungen) für TX-3 5.247 k/s im Mittel (CV 1,25 %) und für TX-4/5 Shards 85,9 ms Median bzw. 88,2 ms Mittel (CV 10,71 %). Beide unveränderten Gates bleiben verfehlt; dies ist lokale Diagnostik, keine repräsentative Baseline. WAL-Synchronisierung und Fencing blieben unverändert.
- `OccReadVersionAndUpdate` liest jetzt den gespeicherten Zähler und schreibt den inkrementierten Wert mit der gelesenen OCC-Version; committed und konfliktbedingt abgebrochene Transaktionen werden getrennt gezählt. Der frühere `BM_TransactionContention`-Fall war wegen `Threads(1)` und fehlendem Read-Modify-Write keine Contention-Messung; er heißt nun `BM_TransactionHotKeyOverwrite` und ist kein TX-3-Fallback.
- `BM_TransactionConcurrentCommitUniform`, `BM_TransactionConcurrentCommitHotKey` und `BM_TransactionConcurrentCommitDisjoint` laufen mit 1, 8 und 32 Google-Benchmark-Workern auf einer gemeinsam genutzten RocksDB-Instanz und einem gemeinsamen `TransactionManager`. Jeder Versuch liest Version und Wert, inkrementiert den gespeicherten Zähler und committet per OCC. Die Hot-Key-/Uniform-Profile zählen OCC-Konflikte als Aborts; im Disjoint-Profil hat jeder Worker einen eigenen vorbereiteten Schlüssel. Es gilt `attempted = committed + aborted`; Retries sind 0, da die Fälle keine Wiederholung implementieren.
- Die Concurrent-Commit-Profile messen die vollständige Wandzeit vom Transaktionsbeginn bis Commit oder Konflikt-Rollback. `mean_worker_p50_us`, `mean_worker_p95_us` und `mean_worker_p99_us` sind nearest-rank-Perzentile je Worker, anschließend von Google Benchmark über die Worker gemittelt; sie sind keine gepoolten Perzentile über alle Samples.
- `BM_TransactionConcurrentAbortDisjoint` führt dasselbe Read-Modify-Write auf einem je Worker eigenen Schlüssel aus und misst bis zum bestätigten `rollbackTransaction`; jeder gemessene Versuch wird als Abort gezählt, Commits und Retries bleiben null. `BM_TransactionConcurrentLockWait` verwendet den echten `LockManager` des `TransactionManager`: Ein separater Holder gibt den gemeinsamen exklusiven Lock erst frei, nachdem alle Benchmark-Worker nachweislich in der Warteschlange stehen. So durchläuft jeder `acquireLock`-Aufruf bei 1, 8 und 32 Workern den blockierenden Wait-Pfad. Lock-Versuche, Erfolge, Timeouts, Denials und Wartelatenzen werden separat exportiert; dies ist keine vollständige Transaktions- oder Commit-Latenz.
- Für Abort- und Lock-Wait-Profile sind `mean_worker_p50_us`, `mean_worker_p95_us` und `mean_worker_p99_us` nearest-rank-Perzentile je Worker, anschließend von Google Benchmark über die Worker gemittelt. Die lokalen Smoke-Ergebnisse validieren nur Ausführung und Counter-Form; sie begründen weder SLO noch Release-Baseline.
- Reproduzierbare Release-Läufe verwenden `benchmarks/run_benchmarks_msvc.ps1`, das Revision/Dirty-Status, Binary-Hashes, CMake-Cache-Hash und ausgewählte CMake-Optionen, CPU/RAM, Betriebssystem, Volume-/Dateisystem-Metadaten, Benchmark-Argumente, Warm-up, Wiederholungen und die Pfade zu unveränderten Google-Benchmark-Roh-JSONs in `run_manifest.json` erfasst. Volume-Metadaten beschreiben weder das physische Laufwerksmodell noch I/O pro Operation.
- Das gemeinsame Transaktionsfixture seedet pro Programmstart 129 Entities: 1 Hot-Key, 64 Uniform-Keys, 32 worker-disjunkte Commit-Keys und 32 worker-disjunkte Abort-Keys. Die RocksDB- und WAL-Pfade liegen unter dem OS-Temp-Verzeichnis als `bench_transaction_concurrency_<run-id>/` und werden nach Prozessende entfernt.
- Reproduzierbarer Windows-Release-Aufruf (fünf Wiederholungen, 0,5 s Warm-up und 1 s Mindestmesszeit je Google-Benchmark-Lauf):
  ```powershell
  # Run from the ThemisDB checkout root.
  .\benchmarks\run_benchmarks_msvc.ps1 `
    -Config Release `
    -BuildRoot .\build-msvc-windows-bench `
    -Filter '^bench_transaction_throughput\.exe$' `
    -BenchmarkFilter '^BM_TransactionConcurrent(Commit|Abort|LockWait)' `
    -MinTime 1s `
    -WarmupTimeSeconds 0.5 `
    -Repetitions 5 `
    -TimeoutSeconds 900 `
    -OutRoot benchmarks\benchmark_results
  ```
  Der Lauf schreibt `bench_transaction_throughput.json` unverändert neben `run_manifest.json` unter `benchmarks/benchmark_results/<timestamp>/`. Das Ergebnis dieses lokalen Aufrufs ist zunächst nur Messmaterial; ein Regression-Budget wird erst aus einem geprüften, repräsentativen und freigegebenen Baseline-Lauf abgeleitet.
- Letzter lokaler Fünf-Wiederholungs-Lauf vom 2026-11-21: Release/Ninja, AVX2 an, ASAN aus; Intel Core i9-10900K, 20 logische Prozessoren, NTFS-Volume unter `C:\Projects\ThemisDB`. Er erfasste 75 Iterationszeilen (15 Fälle × 5 Wiederholungen) und bestand die Outcome-/Lock-Wait-Counter-Prüfungen. Der Manifest-CV (Stichproben-Standardabweichung / Betrag des Mittelwerts) markiert 3 von 15 Durchsatz-Metriken und 9 von 45 Worker-Perzentil-Metriken als >5 %. Auffälligster Wert: `mean_worker_p99_us` beim Lock-Wait/1 Worker mit CV 71,78 %; die 1-Worker-Konfiguration hat keinen konkurrierenden Warte-Worker und ist daher kein repräsentatives Lock-Wait-Profil. Der Worktree war dirty; der Lauf ist weder freigegebene Baseline noch Performance-Gate. Physisches Laufwerksmodell und Patch-Snapshot wurden nicht erfasst.
- Rohdaten: `C:\Projects\ThemisDB\benchmarks\benchmark_results\transaction-wave1-cv\20261121_110337\bench_transaction_throughput.json`; Metadaten und 60 CV-Diagnosen: `C:\Projects\ThemisDB\benchmarks\benchmark_results\transaction-wave1-cv\20261121_110337\run_manifest.json`.
- Längerer Diagnoselauf derselben 5 auffälligen Mehrthread-Fälle (2026-11-21, 2 s Warm-up, 2 s Mindestmesszeit, 5 Wiederholungen): 25 Rohmessungen, 6 von 20 CV-Metriken >5 %. Die Durchsatz-CVs von Disjoint/8, Uniform/32 und Lock-Wait/8/32 sanken auf 1,26 %, 2,33 %, 4,04 % bzw. 1,02 %; Hot-Key/32 blieb bei 5,93 % Durchsatz-CV und 6,36–9,74 % CV der Latenzperzentile. Lock-Wait/8 p99 lag noch bei 9,20 %. Das spricht für stabilere breite Profile bei längerer Messzeit, aber weiterhin für hohe Tail-Schwankung unter extremer Hot-Key-Konkurrenz. Dieser Diagnoselauf hatte ebenfalls einen dirty Worktree und wird nicht als Baseline verwendet.
- Erweiterte Diagnose-Rohdaten: `C:\Projects\ThemisDB\benchmarks\benchmark_results\transaction-wave1-extended\20261121_110741\bench_transaction_throughput.json`; Manifest und CV-Werte: `C:\Projects\ThemisDB\benchmarks\benchmark_results\transaction-wave1-extended\20261121_110741\run_manifest.json`.
- Zehn-Wiederholungs-Nachmessung von Hot-Key/32 und Lock-Wait/1 (je 2 s Warm-up und Mindestzeit): 20 Iterationszeilen, 8 CV-Metriken; sechs blieben >5 %. Hot-Key/32 zeigt 7,84 % Durchsatz-CV und 8,66–11,79 % Perzentil-CV. Lock-Wait/1 Durchsatz-CV beträgt 3,88 %, p50-CV 2,96 %, aber p99-CV bleibt 20,46 % bei einem mittleren p99 von 4,12 µs. Das stützt die Einordnung als sporadischer Scheduling-Tail bei kurzer Ein-Worker-Wartezeit, nicht als throughput-Instabilität.
- Hot-Key-/Lock-Wait-Nachmessungs-Rohdaten: `C:\Projects\ThemisDB\benchmarks\benchmark_results\transaction-wave1-tail\20261121_110912\bench_transaction_throughput.json`; Metadaten: `C:\Projects\ThemisDB\benchmarks\benchmark_results\transaction-wave1-tail\20261121_110912\run_manifest.json`. Zehn Wiederholungen für die zwei verbleibenden Fälle: `C:\Projects\ThemisDB\benchmarks\benchmark_results\transaction-wave1-tail10\20261121_111020\bench_transaction_throughput.json` und `C:\Projects\ThemisDB\benchmarks\benchmark_results\transaction-wave1-tail10\20261121_111020\run_manifest.json`.
- TX-4 ordnet den 5-Teilnehmer-2PC-Fall `TwoPhaseCommitFixture/TwoPhaseCommitLatency/5` zu; Coordinator und Teilnehmer nutzen eindeutige WALs und gültige per-shard Fencing-Tokens. Storage-Anwendung bleibt no-op; der nicht vergleichbare Single-Node-`MixedTransaction`-Fallback wurde entfernt. Erfolgreich gezählte Durchsatz-Items des Phase-4-2PC-Benchmarks sind bestätigte Commits, und der Abort-Pfad prüft nach dem Aufruf den terminalen `ABORTED`-Status.
- `TwoPhaseCommitFixture` verwendet pro Lauf eindeutige Coordinator- und Teilnehmer-WALs sowie einen epoch-1-Test-Fencing-Provider, passend zu den expliziten In-Process-Teilnehmern. Ohne WAL oder gültige Fencing-Tokens schlagen echte Commits fail-closed fehl. Null-Storage-Callbacks bedeuten, dass der Fall Protokoll-/WAL-Arbeit misst, nicht persistente Anwendungswrites.
- Phase-4 latency percentiles now use nearest-rank order statistics over the measured steady-clock samples, rather than the shared fixed-bucket histogram (whose `floor(count * p)` rank becomes zero for small sample counts). The benchmark exports `p95_real_time` and `p99_real_time` in its declared `time_unit` for comparator evaluation.
- TX-9 maps the existing AC-17 internal gate (5,000 transactions/s) to the benchmark's completed-transaction `items_per_second`. Three local runs on 2026-11-21 measured 47.09, 16.07, and 61.83 transactions/s; each reported `gate_pass=0`. This is a mock-participant/coordinator-WAL microbenchmark, not evidence of networked or production 2PC throughput.
- The 2026-11-21 CTest phase-4 smoke measured 0.008 k/s for the three-participant throughput case (TX-9), 366.555 ms P99 for the commit case (TX-10), and 83.758 ms P99 for the abort case (TX-11). These results failed the existing short-run smoke gates and remain non-representative local evidence; they do not establish a release baseline.
- The 2026-11-22 CTest phase-4 smoke passed its benchmark executable but failed postprocessing: TX-9 measured 0.032 k/s, TX-10 97.271 ms P99, and TX-11 97.019 ms P99. The remaining 154 expectation targets were not run in this selective smoke. These values fail the unchanged gates and are local diagnostic evidence only.
- Ein direkter Release-Lauf von `bench_transaction_phase4` am 2026-11-22 (TX-9/TX-11, je 1 s Mindestzeit und 3 Wiederholungen, nach Umstellung routinemäßiger Transaktions-Erfolgsmeldungen auf DEBUG) maß für TX-9 durchschnittlich 24,206 Transaktionen/s und 194,644 ms mittleres P99; für TX-11 38,829 Aborts/s und 661,675 ms mittleres P99. Beide unveränderten AC-17-Gates verfehlten das Ziel deutlich; TX-11 zeigte 70,79 % CV. Die lokale Messung ist stark schwankende Diagnostik, keine Release-Baseline.
- TX-10/TX-11 map the existing AC-17 P99 gates for commit and abort paths. After correcting percentile calculation, a local run measured 51.64 ms and 52.51 ms respectively against the existing 50 ms limit: commit is within the comparator's 5% warning margin, while abort exceeds that margin and fails. The earlier bucket-based values understated these tails and are not valid evidence.
- Durable WAL append paths no longer issue a second `flushDurable()` immediately after `WALManager::append()` has already synchronously flushed under `sync_on_write=true`. Buffered WAL configurations still require and perform an explicit durable flush, and the five TX-4 participants still retain synchronous participant WAL writes and fencing. This removes redundant I/O without changing sync policy or durability guarantees; it does not make the required stable-storage operations free, so the existing TX-4/TX-9/TX-10/TX-11 targets remain independently gated by storage latency and must be remeasured.
- The Phase-4 AC-17 commit and abort cases export `attempted`, `committed`, `aborted`, and `retried` counters. Retries are explicitly zero because neither case implements retries; the commit case counts only successful commits, while the abort case counts transactions verified in the terminal `ABORTED` state.
- The AC-14–AC-16 phase-4 lifecycle cases and local read/write, savepoint, OCC, and 2PC benchmark cases export the same outcome counters for measured transactions. Setup work is excluded; rollback cases count verified rollbacks as aborted, OCC version conflicts count as aborted attempts, and predictor-only TX-6 calls are not transaction attempts. Retries are zero because these benchmarks do not implement retries.
- Erwartungswerte gelten erst als erfüllt, wenn die zugeordneten Benchmarks im Release-Profil reproduzierbar ausgeführt wurden und die Zielwerte erreichen. Ein einzelner lokaler Lauf belegt keine allgemeine oder release-repräsentative Leistung.
- Bei `proxy`/`not_measurable`-Ziel-IDs ist ein dedizierter Messpfad als Folgeaufgabe zu tracken; bis dahin gilt das dokumentierte Proxy-Ziel.

## Sourcecode Verification (Module: transaction/performance)

- Gepruefte Benchmark-Quellen:
  - `benchmarks/transaction/bench_transaction_throughput.cpp`
  - `benchmarks/transaction/bench_transaction_phase4.cpp`
  - `benchmarks/bench_saga_compensation.cpp`
- Gepruefte Ziel-Fall-Zuordnung:
  - Commit, read-only, write-only, mixed, OCC paths in `TransactionBenchmarkFixture`
  - Distributed 2PC throughput and latency paths in `DistributedPhase4Fixture` (TX-9..11; in-process mock participants)
  - Compensation path in `SagaBenchmarkFixture`
- Ergebnis:
  - TX-5 maps to a benchmark outside the transaction benchmark CMake directory; TX-9..11 map the AC-17 mock-participant cases in `bench_transaction_phase4.cpp`.
  - Registered transaction targets are declared in `benchmarks/transaction/CMakeLists.txt`; local measurements are not representative release baselines.
