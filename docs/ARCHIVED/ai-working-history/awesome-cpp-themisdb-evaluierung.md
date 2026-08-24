# ThemisDB Evaluierung: Sinnvolle Projekte aus awesome-cpp

Quelle: https://github.com/fffaraz/awesome-cpp
Stand: 2026-06-18

## Ziel

Aus der awesome-cpp Sammlung wurden Kandidaten gefiltert, die fuer ThemisDB realistisch als

1. direkt integrierbare Bibliothek (kurz- bis mittelfristig), oder
2. Entwicklungsvorlage (Architektur-/Implementierungsreferenz)

Mehrwert bringen koennen.

## Kurzfazit

- Sehr hoher Mehrwert mit geringer Integrationshuerde: CRoaring, xxHash, RE2, simdutf, cpptrace/backward-cpp.
- Hoher Mehrwert mit mittlerer Integrationshuerde: Cap'n Proto oder FlatBuffers, Taskflow, concurrentqueue.
- Als Architekturvorlage sinnvoll, aber nicht als direkte Abhaengigkeit: DuckDB, Kuzu, Seastar, Redpanda.
- Bereits gut abgedeckte Bereiche in ThemisDB: RocksDB, gRPC, Boost.Asio/Beast, simdjson, FAISS, ONNX Runtime, zstd/lz4, spdlog.

## Bereits in ThemisDB vorhanden (aus awesome-cpp relevant)

Bereits im Manifest vorhanden (vcpkg):

- rocksdb
- simdjson
- faiss
- onnxruntime
- boost-asio, boost-beast, boost-url
- grpc
- spdlog
- nlohmann-json
- yaml-cpp
- zstd, lz4
- mimalloc, jemalloc
- cpp-httplib
- msgpack

Konsequenz: In diesen Bereichen sollte der Fokus eher auf Harterung, Tuning und API-Konsolidierung liegen statt auf neuen Bibliotheken.

## Top-Kandidaten fuer direkte Integration

| Prioritaet | Projekt | Typ | Nutzen fuer ThemisDB | Aufwand | Lizenz |
|---|---|---|---|---|---|
| P1 | CRoaring | Lib | Kompakte, schnelle Bitmap-Indizes (Filter, Postings, Segment/Shard-Selektion) | M | Apache-2.0 |
| P1 | xxHash | Lib | Sehr schneller non-crypto Hash fuer Partitionierung, Bloom/Cache Keys, WAL Checks | S | BSD-2-Clause |
| P1 | RE2 | Lib | Deterministische Regex-Laufzeit ohne Catastrophic Backtracking (API/Query-Sicherheit) | M | BSD-3-Clause |
| P1 | simdutf | Lib | Schnelle UTF-8/16/32 Validierung/Transkodierung in API- und Ingest-Pfaden | S-M | Apache-2.0/MIT |
| P1 | cpptrace oder backward-cpp | Lib | Bessere Crash-Diagnose in Produktivumgebungen, schnellere RCA | S-M | MIT |
| P2 | concurrentqueue | Lib | Lock-free MPMC Queues fuer Ingest, CDC, Hintergrundjobs | M | BSD/Boost |
| P2 | Taskflow | Lib | Klare, robuste Task-Graphen fuer Build/Query/Ingest-Pipelines und GPU-Orchestrierung | M | MIT |
| P2 | Cap'n Proto | Lib+RPC | Sehr schnelle schema-basierte Binairserialisierung + RPC Option | M-L | MIT |
| P2 | FlatBuffers | Lib | Zero-copy Datenaustausch fuer interne Protokolle und Snapshots | M-L | Apache-2.0 |
| P2 | libsodium | Lib | Einfachere, sichere Crypto-Bausteine fuer Token, Key Derivation, AEAD-Pfade | M | ISC |

Hinweis zu Cap'n Proto vs FlatBuffers:

- Wenn ThemisDB mittelfristig RPC + Schema Evolution aus einem Guss will: Cap'n Proto zuerst pruefen.
- Wenn Fokus auf Read-heavy zero-copy Datenpfade liegt: FlatBuffers zuerst pruefen.
- Nicht beide gleichzeitig als Primarformat einfuehren.

## Entwicklungsvorlagen (Blueprints, nicht zwingend direkte Dependencies)

### Management-Quickview

| Projekt | Reife-Signale (06/2026) | Blueprint-Wert fuer ThemisDB | Integrationsrisiko als direkte Dependency | Urteil |
|---|---|---|---|---|
| DuckDB | ~38.8k Stars, 62 Releases, sehr aktive Commits | Sehr hoch (vectorized execution, pipeline/operator design, in-process OLAP) | Mittel (Funktionsueberlappung mit ThemisDB-Kern) | Top-Blueprint |
| Kuzu | ~4.0k Stars, 36 Releases, Repository archiviert (read-only) | Mittel-Hoch (Graph-Layout + factorized/vectorized query engine) | Hoch (kein aktives Upstream im gezeigten Repo) | Architekturstudie, keine neue Abhaengigkeit |
| Seastar | ~9.3k Stars, 220 Contributors, hochaktive Entwicklung | Sehr hoch (shard-per-core, futures, reactor, userspace I/O patterns) | Hoch (Architektur-Paradigmenwechsel) | Muster selektiv uebernehmen |
| Redpanda | ~12.2k Stars, 326 Releases, hohe Release-Frequenz | Hoch (Streaming/WAL/Backpressure/SRE-Hardening, Betriebsmodelle) | Hoch (System ist kompletter eigener Datenstack) | Betriebs- und Architektur-Blueprint |
| uWebSockets | ~18.9k Stars, 215 Releases, battle-tested in WS-Lastszenarien | Hoch (WS/HTTP event loops, pub/sub, low-latency connection handling) | Mittel (API-/Semantik-Wechsel in Netzwerkschicht) | Sehr gute Referenz fuer CDC/Websocket-Pfade |

### DuckDB: Was genau als Vorlage taugt

**Warum relevant**

- In-process Datenbankdesign mit klaren Operator-Pipelines und starkem Fokus auf vektorisierten Datenpfaden.
- Sehr ausgereiftes Extension-Denken und pragmatische Build-/Test-Praxis.

**Konkrete Muster fuer ThemisDB**

1. Operator-Grenzen strikt als Datenflussgrenzen definieren (Pipeline Breaker bewusst, nicht zufaellig).
2. Einheitliche, reproduzierbare Micro-/Macro-Benchmark-Runner fuer Query-Subsysteme.
3. Erweiterungsfaehigkeit per sauberem API-Schnitt statt ad-hoc Hooks.

**Nicht 1:1 uebernehmen**

- Kein komplettes SQL/Planner-Modell duplizieren.
- Keine direkte Engine-Einbettung, solange Kernfunktionalitaet in ThemisDB bereits vorhanden ist.

### Kuzu: Was trotz Archiv-Status lehrreich bleibt

**Warum relevant**

- Property-Graph + Cypher + analytischer Fokus in C++.
- Interessant sind vor allem die internen Ideen zu Graph-Storage, Join-Indizes und factorized/vectorized Ausfuehrung.

**Konkrete Muster fuer ThemisDB**

1. CSR-/Adjazenz-nahe Datenstrukturen fuer Graph-Traversal-Hotpaths.
2. Trennung von Graph-spezifischem Planner/Executor und allgemeinem Storage-Layer.
3. Fokus auf join-zentrierte Graph-Abfragen statt reine Traversal-APIs.

**Einschraenkung**

- Repository ist archiviert. Daher nur als statische Architektur-Referenz nutzen, nicht als aktive strategische Abhaengigkeit.

### Seastar: Performance-Muster fuer den Server-Kern

**Warum relevant**

- Event-driven, futures-basiertes Modell mit shard-per-core und reactor-Architektur.
- Entwickelt fuer niedrige Latenz bei hoher Last; starkes NUMA-/Core-Lokalitaetsdenken.

**Konkrete Muster fuer ThemisDB**

1. Core-Lokale Queues und Datenhaltung fuer kritische Pfade (weniger Cross-Core Traffic).
2. Explizites Backpressure im gesamten Request- und Streaming-Pfad.
3. Asynchrone Pipelines mit klarer Ownership und Cancellation-Semantik.

**Nicht 1:1 uebernehmen**

- Kein Full-Rewrite auf Seastar-Paradigma; stattdessen selektive Uebernahme von Scheduling-, Queue- und Backpressure-Mustern.

### Redpanda: Betriebs- und Streaming-Hardening als Blaupause

**Warum relevant**

- Kafka-kompatible Streaming-Plattform in C++, mit starker operativer Reife und hoher Release-Disziplin.
- Historisch eng an Seastar-Ideen angelehnt, aber produktionsseitig stark in Wartbarkeit/Operations ausgepraegt.

**Konkrete Muster fuer ThemisDB**

1. Saubere Trennung von Data Plane und Control Plane (Admin, Health, Reconfig).
2. WAL/Log-zentrierte Robustheit: Recovery-First, nicht Feature-First.
3. SLO-orientierte Metriklandschaft inkl. Backpressure- und Queue-Lag-Signalen.

**Nicht 1:1 uebernehmen**

- Kein Aufbau eines zweiten, vollwertigen Streaming-Stacks parallel zum ThemisDB-Kern.

### uWebSockets: Netzwerk-Hotpath und CDC-Websocket

**Warum relevant**

- Sehr hohe Performance fuer HTTP/WS, erprobte event-loop-basierte Architektur.
- Fokus auf pragmatische, latenzarme I/O-Pfade und hohe Verbindungsdichte.

**Konkrete Muster fuer ThemisDB**

1. Minimale Allokationen im WS-Message-Hotpath.
2. Pub/Sub-Semantik mit klarer Flow-Control (slow consumer behandeln, nicht ignorieren).
3. Trennung von Protokoll-Parsing, Routing und Business-Handlern fuer bessere Profilierbarkeit.

**Achtungspunkt**

- Lizenzhinweise sauber pruefen und fuer Distribution klar dokumentieren; bei Unsicherheit juristisch klaeren.

### Blueprint-Backlog fuer ThemisDB (direkt umsetzbar)

1. DuckDB-inspirierter Operator-Hotpath-Review: Materialization-Grenzen, Batchgroessen, SIMD-Hebel.
2. Seastar/Redpanda-inspirierter Backpressure-Standard fuer CDC/Streaming und interne Queues.
3. uWebSockets-inspirierte WS-Latenzoptimierung: weniger Copies, strengere Slow-Consumer-Policy.
4. Kuzu-inspirierter Graph-Storage-Spike: CSR-/Adjazenzlayout fuer reale Traversal-Workloads.
5. Einheitliche Benchmark-Matrix (p50/p95/p99, Throughput, CPU, RAM) fuer alle vier Blueprint-Linien.

### Konkrete Implementierungsvorlagen je Blueprint-Objekt (ThemisDB)

Hinweis: Die Vorlagen sind absichtlich so formuliert, dass sie als ADR-/Spike-Input oder direkt als Startpunkt fuer Production-Code dienen.

---

### 1) DuckDB -> Vorlage fuer vectorized Execution + Operator-Pipelines

**Ziel-Design fuer ThemisDB**

- Query-Ausfuehrung auf festen Batch-Objekten (z. B. 1K-16K Tuples), statt tuple-at-a-time.
- Operatoren als Pipeline-Stufen mit expliziten Pipeline-Breakern (Sort, HashAgg, Join build).
- Materialisierung nur an klaren Grenzen; dazwischen spaltenorientierte Vektorpfaede.

**ThemisDB-Modul-Skizze**

- `ExecutionBatch`: spaltenorientierter Batch mit optionaler Selection-Vector.
- `PhysicalOperator`: `open()`, `nextBatch()`, `close()`.
- `PipelineScheduler`: baut aus Planfragmenten lauffaehige Pipelines.

**Code-Vorlage (C++)**

```cpp
// Simplified vectorized operator contract inspired by in-process OLAP engines.
struct ExecutionBatch {
	std::size_t row_count{0};
	std::vector<ColumnBuffer> columns;
	std::span<const uint32_t> selection; // optional filtered row ids
};

class PhysicalOperator {
public:
	virtual ~PhysicalOperator() = default;
	virtual Result<void> open(ExecutionContext& ctx) = 0;
	virtual Result<bool> nextBatch(ExecutionBatch& out) = 0; // false => EOS
	virtual Result<void> close() = 0;
};

class FilterProjectOperator final : public PhysicalOperator {
public:
	Result<void> open(ExecutionContext& ctx) override;
	Result<bool> nextBatch(ExecutionBatch& out) override;
	Result<void> close() override;
private:
	std::unique_ptr<PhysicalOperator> child_;
	ExprPredicate predicate_;
	std::vector<Expr> projections_;
};
```

**Konkrete Referenzen**

- DuckDB Source: `src/execution/`, `src/include/duckdb/execution/` (Operator-/Pipeline-Struktur)
- DuckDB Docs: https://duckdb.org/docs/current/internals/overview

**Akzeptanzkriterien**

1. p95-Latenz fuer 3 representative Analytical-Queries verbessert sich >= 15%.
2. CPU-Zeit pro Query sinkt messbar in Perf-Traces.
3. Keine funktionale Regression in Query-Regression-Tests.

---

### 2) Kuzu -> Vorlage fuer Graph-Storage + join-zentrierte Graph-Ausfuehrung

**Ziel-Design fuer ThemisDB**

- Graph-Storage mit CSR-/Adjazenzlayout fuer out-neighbors/in-neighbors.
- Planner trennt graph-spezifische Pattern-Expansion von relationalen Join-Schritten.
- Traversal wird als Kombination aus Expand + Join + Filter ausgefuehrt.

**ThemisDB-Modul-Skizze**

- `GraphAdjacencyStore`: CSR-artige Kantenablage.
- `GraphExpandOp`: liefert Nachbarn als Batch.
- `GraphJoinBridge`: ueberfuehrt Expand-Ausgabe in Join-Key-Batches.

**Code-Vorlage (C++)**

```cpp
struct CsrAdjacency {
	std::vector<uint64_t> offsets; // size = num_vertices + 1
	std::vector<uint64_t> dst;     // neighbor vertex ids
};

class GraphAdjacencyStore {
public:
	Result<void> buildFromEdges(std::span<const EdgeRecord> edges);
	std::span<const uint64_t> neighbors(uint64_t vertex_id) const {
		const auto b = csr_.offsets[vertex_id];
		const auto e = csr_.offsets[vertex_id + 1];
		return std::span<const uint64_t>(csr_.dst.data() + b, e - b);
	}
private:
	CsrAdjacency csr_;
};
```

**Konkrete Referenzen**

- Kuzu Source: `src/storage/`, `src/processor/`, `src/planner/`
- Kuzu Docs: https://kuzudb.github.io/docs

**Akzeptanzkriterien**

1. 2-Hop/3-Hop Traversal auf grossen Graphen >= 20% schneller vs. Baseline.
2. Speicherbedarf pro Kante sinkt oder bleibt stabil bei besserer Latenz.
3. Graph+Relational Mischabfragen laufen ueber einheitlichen Execution-Pfad.

---

### 3) Seastar -> Vorlage fuer shard-per-core Scheduling + Backpressure

**Ziel-Design fuer ThemisDB**

- Kernpfade (CDC, Queueing, Hintergrundjobs) shard-lokal auf CPU-Kerne verteilen.
- Keine unkontrollierte Cross-Core-Arbeit; Routing ueber konsistente Shard-Key-Funktion.
- Backpressure als festes Vertragsverhalten (enqueue kann ablehnen oder drosseln).

**ThemisDB-Modul-Skizze**

- `ShardRouter`: mappt Request -> Shard.
- `ShardQueue`: bounded MPSC/MPMC Queue mit Wasserstandsgrenzen.
- `BackpressurePolicy`: reject/retry/token-bucket.

**Code-Vorlage (C++)**

```cpp
class ShardQueue {
public:
	explicit ShardQueue(std::size_t capacity) : capacity_(capacity) {}

	bool tryEnqueue(Task&& t) {
		if (size_.load(std::memory_order_relaxed) >= capacity_) {
			return false; // backpressure signal
		}
		queue_.push(std::move(t));
		size_.fetch_add(1, std::memory_order_release);
		return true;
	}

	std::optional<Task> tryDequeue();

private:
	std::size_t capacity_;
	std::atomic<std::size_t> size_{0};
	ConcurrentQueue<Task> queue_;
};
```

**Konkrete Referenzen**

- Seastar Docs: https://docs.seastar.io/master/
- Seastar Source: `include/seastar/core/` (futures/reactor/sharded patterns)

**Akzeptanzkriterien**

1. Unter Last kein unbounded Queue-Wachstum in CDC-Pfaden.
2. p99-Latenz bleibt innerhalb definierter SLOs bei Burst-Traffic.
3. Drop/Retry-Verhalten ist deterministisch und observierbar.

---

### 4) Redpanda -> Vorlage fuer WAL/Streaming-Hardening + Operations

**Ziel-Design fuer ThemisDB**

- Log-/WAL-zentrierte Robustheit: append, flush, segment roll, recovery replay.
- Strikte Trennung von Data Plane (Read/Write/Replicate) und Control Plane (Admin/Reconfig).
- Betriebsmetriken als API: lag, flush-latency, replay-duration, throttling-events.

**ThemisDB-Modul-Skizze**

- `WalSegmentManager`: segment lifecycle.
- `ReplicationCoordinator`: commit/ack policy.
- `OpsMetricsRegistry`: SLO-orientierte Metriken.

**Code-Vorlage (C++)**

```cpp
struct WalAppendResult {
	uint64_t lsn;
	bool fsync_required;
};

class WalWriter {
public:
	Result<WalAppendResult> append(std::span<const std::byte> record);
	Result<void> flush();
	Result<void> rollSegmentIfNeeded();
};

class RecoveryEngine {
public:
	Result<RecoveryStats> replayFromLastCheckpoint(uint64_t checkpoint_lsn);
};
```

**Konkrete Referenzen**

- Redpanda Docs: https://docs.redpanda.com
- Redpanda Source: `src/v/storage/`, `src/v/cluster/`, `src/v/kafka/`

**Akzeptanzkriterien**

1. Crash-Recovery reproduzierbar mit definiertem Recovery-SLO.
2. WAL-Segment-Rotation ohne Throughput-Einbruch > X% (projektdefiniert).
3. Lag- und Backpressure-Metriken sind im Monitoring vollstaendig sichtbar.

---

### 5) uWebSockets -> Vorlage fuer CDC/WebSocket-Hotpath

**Ziel-Design fuer ThemisDB**

- Event-loop-basierter WS/HTTP Versand mit minimalen Kopien.
- Slow-Consumer-Erkennung und harte Policy (drop, downgrade, disconnect).
- Entkopplung: Transport-Schicht != CDC-Serialisierung != Business-Routing.

**ThemisDB-Modul-Skizze**

- `WsSession`: per-client state inkl. outbound budget.
- `WsBroadcaster`: topic/channel fanout mit flow control.
- `CdcMessageEncoder`: serialisiert Delta-Events in transportneutrales Buffer-Format.

**Code-Vorlage (C++)**

```cpp
class WsSession {
public:
	bool canSend(std::size_t bytes) const {
		return (pending_bytes_ + bytes) <= max_pending_bytes_;
	}

	Result<void> send(std::span<const std::byte> payload) {
		if (!canSend(payload.size())) {
			return Result<void>::error("slow-consumer");
		}
		pending_bytes_ += payload.size();
		// transport_->send(payload, on_complete -> pending_bytes_ -= sent)
		return Result<void>::ok();
	}

private:
	std::size_t pending_bytes_{0};
	std::size_t max_pending_bytes_{1 << 20};
};
```

**Konkrete Referenzen**

- uWebSockets Source: `src/App.h`, `src/WebSocketContext.h`, `src/Loop.h`
- uWebSockets Repo: https://github.com/uNetworking/uWebSockets

**Akzeptanzkriterien**

1. WS-CDC p99 Send-Latenz sinkt bei N gleichzeitigen Verbindungen.
2. Slow-Consumer verursacht keine globale Stauwirkung.
3. Copy-Count pro Nachricht sinkt messbar im Profiling.

---

### Einheitliches Einfuehrungsmuster fuer alle 5 Vorlagen

1. ADR anlegen: Ziel, Nicht-Ziele, Integrationsgrenzen.
2. 1 Spike-Branch pro Vorlage (isoliert, messbar).
3. Vorher/Nachher-Benchmark mit identischer Last.
4. Nur bei SLO-Gewinn + stabiler Testlage in `develop` uebernehmen.
5. Danach Dokumentation in ROADMAP/FUTURE_ENHANCEMENTS synchronisieren.

### Umsetzbares Arbeitspaket fuer ThemisDB (konkrete Dateien + PR-Reihenfolge)

#### PR-00: Architekturrahmen und Akzeptanzkriterien fixieren

**Ziel**

- Gemeinsame Leitplanken fuer alle folgenden Aenderungen (kein Scope-Drift).

**Dateien (konkret)**

- `ROADMAP.md`
- `FUTURE_ENHANCEMENTS.md`
- `CHANGELOG.md`

**Definition of Done**

1. Jede Blueprint-Linie hat klare Nicht-Ziele.
2. Messkriterien sind dokumentiert (p50/p95/p99, Throughput, CPU, RAM, Recovery-Zeit).

---

#### PR-01 (DuckDB-Blueprint): Vectorized Execution Contract haerten

**Ziel**

- Batch-/Operator-Vertraege vereinheitlichen und an Hotpaths konsistent anwenden.

**Dateien (konkret)**

- `include/query/vectorized_execution.h`
- `src/query/vectorized_execution.cpp`
- `include/query/query_engine.h`
- `src/query/query_engine.cpp`
- `tests/vectorized/test_vectorized_execution.cpp`

**Lieferumfang**

1. Einheitlicher Batch-Vertrag (Batchgroesse, Selection-Vector, EOS-Semantik).
2. Explizite Pipeline-Breaker-Markierung an zentralen Operatorgrenzen.
3. Query-Profiler-Hook fuer Batch-Metriken.

**Definition of Done**

1. `test_vectorized_execution` ohne Regression.
2. p95-Latenz auf Referenzabfragen verbessert oder mindestens nicht verschlechtert.

---

#### PR-02 (Kuzu-Blueprint): Graph-Storage Spike auf CSR/Adjazenz

**Ziel**

- Traversal-Hotpaths auf kompakteres Nachbarschaftslayout bringen.

**Dateien (konkret)**

- `include/index/graph_index.h`
- `src/index/graph_index.cpp`
- `include/graph/graph_query_optimizer.h`
- `src/graph/graph_query_optimizer.cpp`
- `tests/graph/test_graph_parallel_traversal.cpp`
- `tests/graph/test_graph_index_comprehensive.cpp`

**Lieferumfang**

1. CSR-nahe Adjazenzreprasentation fuer mindestens einen kritischen Traversalpfad.
2. Join-zentrierte Expand-zu-Filter-Execution fuer Basiscases.

**Definition of Done**

1. 2-Hop/3-Hop Traversal-Benchmark schneller oder gleiche Latenz bei weniger RAM.
2. Graph-Testsuite bleibt gruen.

---

#### PR-03 (Seastar-Blueprint): Shard-Queue + Backpressure im CDC-Pfad

**Ziel**

- Bounded Queues und deterministische Ueberlastreaktion einziehen.

**Dateien (konkret)**

- `include/cdc/icdc_backpressure_signal.h`
- `include/cdc/icdc_fan_in.h`
- `src/cdc/cdc_ws_handler.cpp`
- `src/server/changefeed_api_handler.cpp`
- `tests/cdc/test_cdc_backpressure_signal.cpp`
- `tests/cdc/test_cdc_fan_in.cpp`

**Lieferumfang**

1. Einheitliche Backpressure-Signale (reject/retry/defer).
2. Queue-Wasserstandsgrenzen pro Stream/Shard.
3. Lastabwurfregeln mit Telemetrie.

**Definition of Done**

1. Kein unbounded Queue-Wachstum unter Burst-Last.
2. Reproduzierbares Verhalten fuer Slow-Producer/Slow-Consumer.

---

#### PR-04 (Redpanda-Blueprint): WAL Recovery/Segment-Disziplin haerten

**Ziel**

- WAL-Pfade recovery-first ausrichten und operativ sichtbar machen.

**Dateien (konkret)**

- `include/storage/wal_storage.h`
- `src/storage/wal_storage.cpp`
- `include/sharding/wal_manager.h`
- `src/sharding/wal_manager.cpp`
- `include/server/wal_api_handler.h`
- `src/server/wal_api_handler.cpp`
- `tests/wal/test_wal_manager.cpp`
- `tests/wal/test_wal_replication_integration.cpp`

**Lieferumfang**

1. Segment-Rotation/Flush-Verhalten klar spezifizieren und instrumentieren.
2. Recovery-Statistiken als API- und Metriksignal.
3. Fehlerpfade (Corruption/Replay) explizit testen.

**Definition of Done**

1. WAL-Integrationstests stabil gruen.
2. Recovery-SLO dokumentiert und in Testlauf nachweisbar.

---

#### PR-05 (uWebSockets-Blueprint): WS-Hotpath fuer CDC entkoppeln und absichern

**Ziel**

- WS-Sendeweg copy-arm und slow-consumer-fest machen.

**Dateien (konkret)**

- `include/server/websocket_session.h`
- `src/server/websocket_session.cpp`
- `include/network/wire_protocol_websocket.h`
- `src/cdc/cdc_ws_handler.cpp`
- `tests/network/test_websocket_cdc.cpp`
- `tests/network/test_wire_protocol_websocket.cpp`

**Lieferumfang**

1. Outbound-Budget pro Session und konsequente Slow-Consumer-Policy.
2. Trennung von Transport/Encoding/Dispatch.
3. Latenz- und Drop-Metriken je Kanal.

**Definition of Done**

1. p99-Sendelatenz unter Last verbessert.
2. Ein einzelner langsamer Client blockiert keinen gesamten Kanal.

---

#### PR-06: Benchmark- und Gate-Konsolidierung (querliegend)

**Ziel**

- Objektive Merge-Gates fuer alle vorherigen PRs.

**Dateien (konkret)**

- `tests/cmake/RegisterModuleTests.cmake`
- `ctest_focus_latest.txt`
- `CTEST.md`

**Lieferumfang**

1. Fokus-Suites fuer Query/Graph/CDC/WAL/WS klar benannt.
2. Einheitliche Lastprofile fuer Vorher/Nachher-Vergleich.

**Definition of Done**

1. Reproduzierbare Vergleichsläufe lokal auf `windows-release`.
2. Delta-Report (Latenz/Throughput/CPU/RAM/Recovery) pro PR.

---

#### Empfohlene Abarbeitungsreihenfolge (strict)

1. PR-00
2. PR-01
3. PR-03
4. PR-05
5. PR-04
6. PR-02
7. PR-06

Begruendung: Erst Execution/Backpressure/WS stabilisieren (direkter Runtime-Hebel), dann WAL-Hardening, danach Graph-Storage-Spike, abschliessend gemeinsame Gates.

## Kandidaten, die aktuell eher nachrangig sind

- Vollstaendige Webframeworks (Drogon, oat++, Crow), da ThemisDB bereits starke eigene Server-/Protocol-Pfade hat.
- Zusatz-JSON-Libs ohne klaren Mehrwert gegen simdjson + nlohmann-json.
- Weitere allgemeine Allocators neben mimalloc/jemalloc ohne profilbasierten Nachweis.

## Empfohlener Umsetzungsplan (90 Tage)

### Phase A (2-3 Wochen): Quick Wins

- Spike: xxHash in Hash-hotspots (Sharding, Cache-Key, Bloom-Kontexte).
- Spike: RE2 fuer user-kontrollierte Regex-Pfade.
- Spike: simdutf fuer API-Ingress UTF-Validierung.
- Spike: cpptrace/backward-cpp in Crash- und Fatal-Logging integrieren.

Erfolgskriterien:

- Keine Regression in bestehenden Tests.
- Messbarer Latenz/CPU-Gewinn in mindestens zwei Hotpaths.
- Verbesserte Diagnosezeit bei Crash-/Fail-Faellen.

### Phase B (3-5 Wochen): Datenpfad-Optimierung

- Spike: CRoaring fuer bitset-lastige Filterpfade.
- Spike: concurrentqueue in mindestens einem echten Producer/Consumer Pfad.
- Optional: Taskflow Pilot in einem klar abgegrenzten Pipeline-Modul.

Erfolgskriterien:

- Geringere Heap-Last und CPU in Scan-/Filterpfaden.
- Stabilere Throughput-Werte unter Last.

### Phase C (4-6 Wochen): Wire-/Format-Strategie

- Architectural Decision Record: Cap'n Proto vs FlatBuffers (genau eins auswaehlen).
- POC auf einem internen Protokoll oder Snapshot-Format.

Erfolgskriterien:

- Klare Entscheidung mit Benchmarkdaten (Encode/Decode, Bytes on wire, CPU, Evolvierbarkeit).

## Lizenz- und Governance-Hinweise

- Bevorzugt: MIT, BSD, Apache-2.0, ISC.
- Vorsicht bei AGPL/GPL fuer Kernpfade und distributable Builds.
- Fuer jede neue Lib: Security-Scan, Update-Policy, SBOM-Eintrag, vcpkg-Strategie.

## Konkrete Start-Shortlist

1. CRoaring
2. xxHash
3. RE2
4. simdutf
5. cpptrace (oder backward-cpp)
6. Taskflow
7. concurrentqueue
8. Cap'n Proto oder FlatBuffers (Entscheidungssprint)
9. libsodium

## Anmerkung zur Repo-Qualitaet awesome-cpp

awesome-cpp ist sehr gut als Discovery-Quelle geeignet, aber keine technische Due-Diligence.
Jeder Kandidat muss zusaetzlich auf Wartungsgrad, API-Stabilitaet, CVE-Historie, Windows/Linux-Build-Verhalten und Lizenzdetails geprueft werden.

## Deep-Dive der Zieladressen (compress, graph, relational, vector, DB, AI)

Hinweis: Dieser Abschnitt basiert auf direkter Sichtung der Ziel-Repositories und deren oeffentlich sichtbaren Signalen (Aktivitaet, Releases, Maintainer-Breite, Doku, Build-Hinweise, Lizenz).

### Bewertungsraster

- Strategischer Fit: Wie gut passt das Projekt zu ThemisDB-Kernzielen.
- Reifegrad: Aktivitaet, Release-Praxis, Community-Breite.
- Integrationsrisiko: Build-Komplexitaet, Plattformrisiko, operatives Risiko.
- Empfehlung: Use now, Pilot, Blueprint, oder No-go.

### Compression

| Projekt | Beobachtung | Risiko | Empfehlung |
|---|---|---|---|
| zstd | Sehr hoher Reifegrad, sehr grosse Nutzerbasis, stabile Spezifikation (RFC8878), starke Small-Data-Dictionary-Pfade | Gering, bereits im Stack | Weiterfuehren und tiefer profilieren (Dictionary-Training fuer WAL/Metadata) |
| lz4 | Sehr schnell in Decode, hoher Reifegrad, sehr gute Eignung fuer Throughput-Pfade | Gering, bereits im Stack | Weiterfuehren fuer Hot-Path-Kompression (Cache, Replikation, Streaming) |
| fsst | DB-nahe Textkompression mit Random Access und guter Eignung fuer String-Spalten/Pruefpraedikate | Mittel: kleinere Community, weniger Release-Disziplin | Gezielter Pilot in String-heavy Pfaden (Secondary Index Keys, FTS-Precompute) |

Deep-Dive Urteil Compression:

- Kein Codec-Wechsel noetig.
- Hoechster Hebel: adaptives zstd/lz4-Tiering plus FSST als spezialisierter Text-Codec.

### Graph

| Projekt | Beobachtung | Risiko | Empfehlung |
|---|---|---|---|
| CXXGraph | Gute Algo-Breite und aktive Pflege, moderne C++-Ausrichtung, MPL-2.0 | Mittel: eher Algo-Library, kein vollstaendiger Graph-DB-Kern | Als Algorithmik-Referenz sinnvoll, nicht als Kernabhaengigkeit |
| graaf | Schlank, header-only, MIT, gute Lehr-/Referenzqualitaet | Mittel-Hoch: kleinere Community und geringere Schlagkraft | Nur als Blaupause fuer klar isolierte Algorithmen |
| Kuzu | Fachlich sehr starke Graph-DB-Architektur, aber Repository archiviert (read-only) | Hoch: keine aktive Weiterentwicklung im gezeigten Repo | Nicht als direkte Abhaengigkeit; nur Architekturstudie aus bestehender Codebasis |

Deep-Dive Urteil Graph:

- Fuer produktive Integration keine neue Graph-Library aufnehmen.
- Stattdessen gezielte Uebernahme von Architekturideen (Adjazenz-/Join-Indizes, Query-Ausfuehrung, Layout-Strategien).

### Relational/DB Engine

| Projekt | Beobachtung | Risiko | Empfehlung |
|---|---|---|---|
| DuckDB | Sehr hoher Reifegrad, extrem aktive Entwicklung, klarer In-Process-Fit, MIT | Mittel: direkte Einbettung fuehrt zu Engine-Ueberlappung | Primaere Blueprint-Quelle fuer Operator-, Vectorized- und Storage-Design |
| Velox | Sehr starke Ausfuehrungsengine, hohe Industrieadoption, aktive Governance | Hoch: Integrationsgewicht und Build-/Dependency-Komplexitaet | Blueprint fuer Execution/Vector-Memory, keine kurzfristige Direktintegration |
| Infinity | AI-native DB mit Hybrid Search, aktive Releases, Apache-2.0 | Mittel-Hoch: Produktueberlappung, Plattform-/Betriebsannahmen | Competitor Benchmarking und Feature-Benchmark, nicht als Core-Dependency |

Deep-Dive Urteil Relational/DB:

- DuckDB und Velox als wichtigste Architekturreferenzen.
- Infinity als Markt-/Feature-Messlatte fuer Search-UX und Hybrid-Retrieval.

### Vector/Hybrid Search

| Projekt | Beobachtung | Risiko | Empfehlung |
|---|---|---|---|
| FAISS | State-of-the-art ANN, sehr reif, starke GPU-Pfade, MIT | Mittel: Tuning- und Betriebskomplexitaet bei vielen Indexmodi | Bestehende Nutzung vertiefen (Adaptive Index Selection, Auto-Tuning) |
| hnswlib | Header-only, sehr verbreitet, robust fuer dynamische Updates, Apache-2.0 | Mittel: eigenstaendige Tuning- und Persistenzlogik noetig | Sinnvoll fuer fokussierte In-Memory-Szenarien und leichte Embeddings |
| zvec | Sehr starkes Momentum, in-process vector DB, Hybrid/FTS, Apache-2.0 | Mittel-Hoch: junges Projekt, schnell wandelnde APIs | Als Referenz fuer in-process UX und On-Disk ANN-Design (z. B. DiskANN-Strategien) |

Deep-Dive Urteil Vector:

- ThemisDB ist mit FAISS/hnswlib gut positioniert.
- Prioritaet liegt auf interner Produktisierung (Auto-Tuner, Quality-Latency-SLO-Controller) statt Bibliothekswechsel.

### AI Inference

| Projekt | Beobachtung | Risiko | Empfehlung |
|---|---|---|---|
| ONNX Runtime | Sehr hoher Reifegrad, breite Hardware-Backends, starke Release-Praxis, MIT | Mittel: Build-Optionen und EP-Matrix koennen komplex werden | Als Primar-Inference-Layer beibehalten und gezielt auf EP-Strategie standardisieren |
| llama.cpp | Sehr hohe Entwicklungsdynamik, starke Edge/Local-Inference-Eignung, MIT | Mittel-Hoch: schnelle API-Aenderungen, operativ viel Bewegung | Optionaler Spezialpfad fuer lokale LLM-Features, klar vom Kern entkoppeln |

Deep-Dive Urteil AI:

- ONNX Runtime bleibt der stabile Standard.
- llama.cpp nur als klar abgegrenztes Modul fuer Local/Edge-LLM Use-Cases.

## Deep-Dive Priorisierung fuer ThemisDB

### T1 (sofort, 0-6 Wochen)

1. Compression-Tiering finalisieren: zstd (ratio) plus lz4 (latency) mit datengetriebenen Schwellwerten.
2. FAISS/hnswlib Auto-Tuning-Rahmen fuer Recall/Latency/Speicher.
3. ORT Execution-Provider-Standardisierung pro Plattform (CPU, CUDA, optional weitere EPs).

### T2 (6-12 Wochen)

1. FSST-Pilot fuer stringlastige Spalten und Predicate-Pushdown-Szenarien.
2. DuckDB-/Velox-inspirierte Operator-Hotpath-Optimierungen (vectorized kernels, materialization boundaries).
3. Benchmark-Paritaet gegen Infinity/zvec fuer Hybrid-Search Workloads.

### T3 (12+ Wochen)

1. Optionales llama.cpp-Modul fuer lokale RAG/Agent-Workflows.
2. Graph-Engine-Verbesserungen aus CXXGraph/Kuzu-Studie in eigene Kernkomponenten uebernehmen.

## Harte No-go-/Vorsichtspunkte aus dem Deep-Dive

- Kuzu in der gezeigten Form nicht als aktive Abhaengigkeit einplanen (archiviertes Repository).
- Velox nicht als Schnellintegration behandeln; nur mit dediziertem Architekturprojekt.
- Keine redundanten Engine-Stacks parallel produktiv ziehen (z. B. mehrere konkurrierende Kernpfade fuer denselben Zweck), solange kein klarer SLO-Nachweis vorliegt.

## Empfohlene Zielarchitektur-These

- Kern bleibt: ThemisDB-eigene Engine plus bestehende, bereits integrierte, reife Libraries.
- Externe Projekte dienen primaer als
	- Leistungs-Benchmark,
	- Architekturreferenz,
	- und selektive Spezialbaustein-Quelle.

Das minimiert Integrationsrisiko und maximiert gleichzeitig die Geschwindigkeit bei messbarem Produktfortschritt.

