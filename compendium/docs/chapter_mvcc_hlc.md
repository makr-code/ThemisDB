# MVCC und Hybrid Logical Clocks

> **Zusammenfassung:** Multi-Version Concurrency Control (MVCC) kombiniert mit Hybrid Logical Clocks (HLC) ermöglicht ThemisDB konsistente Snapshot-Lesevorgänge und serialisierbare Transaktionen ohne Lesesperren. Dieses Kapitel beschreibt die Architektur, das Schlüssel-Encoding, die API und den Garbage-Collection-Mechanismus.
>
> **Voraussetzungen:** [Kapitel 8: Storage Layer](chapter_08_storage_layer.md), [Kapitel 17: Horizontale Skalierung](chapter_17_scaling.md)
>
> **Lernziele:**
> - MVCC-Grundprinzipien und ihre Anwendung in ThemisDB verstehen
> - Hybrid Logical Clocks für kausale Zeitstempel einsetzen
> - Versionierte Reads (Latest und Snapshot) korrekt verwenden
> - Garbage Collection sicher konfigurieren und ausführen
> - MVCC über die REST-API und Prometheus-Metriken überwachen

---

## 18.1 Überblick

### Motivation

Traditionelle Datenbanken verwenden Lesesperren, um konsistente Lesevorgänge zu garantieren. Dieses Modell hat erhebliche Nachteile:

- Reads blockieren Writes (und umgekehrt)
- Lange Lesevorgänge verzögern Schreibtransaktionen
- In verteilten Systemen führen globale Sperren zu hoher Latenz

MVCC löst dieses Problem, indem jede Schreiboperation eine **neue Version** eines Datensatzes erzeugt, anstatt die alte zu überschreiben. Lesevorgänge lesen immer eine konsistente *Momentaufnahme* (Snapshot) und blockieren dabei keine Schreiboperationen.

### Ziele der MVCC-Implementierung

| Ziel | Umsetzung |
|------|-----------|
| Snapshot-Reads | `getAtTimestamp(key, ts)` |
| Linearisierbare Reads | `getLatest(key)` |
| Versionsverlauf | `scanVersions(key, callback)` |
| Garbage Collection | `gcVersionsBefore(key, min_ts)` |
| Verteilte Zeitstempel | Hybrid Logical Clock (`HybridLogicalClock`) |

---

## 18.2 Hybrid Logical Clocks (HLC)

### Grundprinzip

Ein *Hybrid Logical Clock* (HLC) kombiniert physikalische Echtzeit mit einem logischen Zähler zu einem **streng monoton steigenden** 64-Bit-Wert:

```
┌─────────────────────────────────┬──────────────────┐
│ Physikalische Zeit (44 Bits)    │ Logik (20 Bits)  │
│ Millisekunden seit Unix-Epoch   │ Tie-Breaker      │
└─────────────────────────────────┴──────────────────┘
  Bit 63 ─────────────── Bit 20    Bit 19 ──── Bit 0
```

- **Physikalischer Teil** (Bits 63–20): Systemuhrzeit in Millisekunden. Sorgt dafür, dass Zeitstempel näherungsweise der realen Zeit entsprechen.
- **Logischer Teil** (Bits 19–0): Wird inkrementiert, wenn zwei Ereignisse in derselben Millisekunde auftreten. Garantiert strikte Ordnung unabhängig von der Uhrzeitgenauigkeit.

Der maximale logische Zähler pro Millisekunde beträgt **1.048.575** (2²⁰ − 1), was für alle praktischen Szenarien ausreicht.

### Implementierung

```cpp
#include "storage/hlc.h"
using namespace themis;

HybridLogicalClock clock;

// Neuen Zeitstempel generieren (streng monoton)
HLCTimestamp ts = clock.now();

// Uhr bei Empfang einer Nachricht vorwärts stellen
HLCTimestamp remote_ts = ...; // aus eingehender Anfrage
HLCTimestamp adjusted = clock.update(remote_ts);

// Aktuellen Zeitstempel lesen ohne Vorwärtsstellen
HLCTimestamp current = clock.peek();
```

### Zeitstempel-Encoding

Zeitstempel werden als **8-Byte Big-Endian** in RocksDB-Schlüssel eingebettet. Dieses Format garantiert, dass die lexikografische Sortierreihenfolge der Schlüssel der chronologischen Reihenfolge entspricht – eine Voraussetzung für effiziente Bereichsabfragen.

```
Schlüsselformat: <base_key> '\x00' <8-Byte-Big-Endian-Timestamp>
```

> **Wichtig:** Der Zeitstempel wird immer als die **letzten 8 Bytes** des versionierten Schlüssels dekodiert (feste Breite, nicht via `rfind('\x00')`), da Big-Endian-Zeitstempel selbst Null-Bytes enthalten können.

---

## 18.3 MVCC Store

### Architektur

Der `MVCCStore` ist eine dünne Schicht über `RocksDBWrapper`:

```
┌─────────────────────────────────────────┐
│           Anwendungsschicht             │
│    MvccApiHandler  /  Ihre Logik        │
├─────────────────────────────────────────┤
│              MVCCStore                  │
│  put / getLatest / getAtTimestamp / GC  │
├─────────────────────────────────────────┤
│         HybridLogicalClock              │
│         now() / update() / peek()       │
├─────────────────────────────────────────┤
│           RocksDBWrapper                │
│        (KV-Store, RocksDB)              │
└─────────────────────────────────────────┘
```

### Schreiben

```cpp
#include "storage/mvcc_store.h"
using namespace themis;

auto clock = std::make_shared<HybridLogicalClock>();
auto store = std::make_shared<MVCCStore>(rocksdb_wrapper, clock);

// Automatisch timestampierter Schreibvorgang
std::vector<uint8_t> value = {/* ... */};
HLCTimestamp ts = store->put("user:42", value);

// Explizit timestampierter Schreibvorgang (z.B. für Replikation)
HLCTimestamp explicit_ts = HLCTimestamp::from(1234567890, 0);
store->putWithTimestamp("user:42", value, explicit_ts);
```

### Lesen

```cpp
// Linearisierbarer Read (neueste Version)
auto latest = store->getLatest("user:42");
if (latest.has_value()) {
    // latest.value() ist std::vector<uint8_t>
}

// Snapshot-Read bei einem gegebenen Zeitstempel
HLCTimestamp snapshot_ts = HLCTimestamp::from(1234567890, 0);
auto at_ts = store->getAtTimestamp("user:42", snapshot_ts);
// Gibt die neueste Version zurück, die <= snapshot_ts ist
```

### Versionsverlauf iterieren

```cpp
store->scanVersions("user:42", [](const MVCCStore::VersionEntry& entry) -> bool {
    std::cout << "ts=" << entry.timestamp.value
              << " value_size=" << entry.value.size() << "\n";
    return true; // true = weitermachen, false = abbrechen
});
// Versions werden von alt nach neu geliefert
```

### Garbage Collection

Alte Versionen müssen explizit bereinigt werden:

```cpp
// Alle Versionen eines Schlüssels, die älter als watermark sind, löschen
HLCTimestamp watermark = store->currentTimestamp();
MVCCStore::GCOptions opts;
opts.min_versions_to_keep = 2; // Mindestens 2 neueste Versionen behalten
uint64_t deleted = store->gcVersionsBefore("user:42", watermark, opts);

// Vollständiger GC-Scan über alle Schlüssel
uint64_t total = store->gcAllBefore(watermark, opts);
```

> **Sicherheitsmerkmal:** Auch wenn `min_versions_to_keep = 1` (Standard), wird mindestens die aktuellste Version eines Schlüssels **immer behalten**, selbst wenn ihr Zeitstempel unterhalb des Watermarks liegt.

---

## 18.4 REST-API

### Endpunkte

| Methode | Pfad | Beschreibung |
|---------|------|-------------|
| `GET` | `/api/v1/mvcc/keys/{key}` | Neueste Version lesen |
| `GET` | `/api/v1/mvcc/keys/{key}?timestamp={ts}` | Snapshot-Read |
| `POST` | `/api/v1/mvcc/keys/{key}` | Neue Version schreiben |
| `GET` | `/api/v1/mvcc/keys/{key}/versions` | Alle Versionen auflisten |
| `DELETE` | `/api/v1/mvcc/keys/{key}/versions` | Alte Versionen bereinigen |
| `GET` | `/api/v1/mvcc/clock` | Aktuellen HLC-Zeitstempel abfragen |
| `GET` | `/api/v1/mvcc/stats` | Betriebsstatistiken |

### Beispiele

#### Schreiben einer neuen Version

```http
POST /api/v1/mvcc/keys/user%3A42
Content-Type: application/json

{"value": "eyJuYW1lIjoiQWxpY2UifQ=="}
```

```json
{
  "key": "user:42",
  "timestamp": 1740000000050010
}
```

#### Snapshot-Read

```http
GET /api/v1/mvcc/keys/user%3A42?timestamp=1740000000050000
```

```json
{
  "key": "user:42",
  "value": "eyJuYW1lIjoiQWxpY2UifQ==",
  "timestamp": 1740000000050000
}
```

#### Versionsverlauf

```http
GET /api/v1/mvcc/keys/user%3A42/versions
```

```json
{
  "key": "user:42",
  "versions": [
    {"timestamp": 1740000000050000, "value": "old_value"},
    {"timestamp": 1740000000050010, "value": "new_value"}
  ]
}
```

#### Garbage Collection

```http
DELETE /api/v1/mvcc/keys/user%3A42/versions
Content-Type: application/json

{
  "before_timestamp": 1740000000050005,
  "min_versions_to_keep": 1
}
```

```json
{
  "key": "user:42",
  "versions_deleted": 1
}
```

#### HLC-Uhr

```http
GET /api/v1/mvcc/clock
```

```json
{
  "timestamp": 1740000000050010,
  "physical_ms": 1740000000,
  "logical": 10
}
```

---

## 18.5 Metriken

Der `MVCCStore` und `MvccApiHandler` exportieren folgende Prometheus-Metriken über den `PrometheusMetrics`-Service:

| Metrik | Typ | Beschreibung |
|--------|-----|-------------|
| `themis_mvcc_writes_total` | Counter | Gesamtzahl Schreiboperationen |
| `themis_mvcc_write_latency_seconds` | Histogram | Schreiblatenz |
| `themis_mvcc_reads_total{read_type}` | Counter | Reads nach Typ (`latest`/`snapshot`) |
| `themis_mvcc_read_latency_seconds{read_type}` | Histogram | Leselatenz nach Typ |
| `themis_mvcc_gc_runs_total` | Counter | Anzahl GC-Läufe |
| `themis_mvcc_gc_versions_deleted_total` | Counter | Gelöschte Versionseinträge gesamt |
| `themis_mvcc_gc_batch_size` | Histogram | Gelöschte Einträge pro GC-Lauf |
| `themis_mvcc_version_entries` | Gauge | Aktuelle Anzahl gespeicherter Versionseinträge |
| `themis_hlc_advances_total{type}` | Counter | HLC-Vorwärtsbewegungen nach Typ (`local`/`received`) |

### Beispiel-Dashboard-Abfragen (PromQL)

```promql
# Schreibdurchsatz (Writes/s über die letzten 5 Minuten)
rate(themis_mvcc_writes_total[5m])

# Anteil Snapshot-Reads
rate(themis_mvcc_reads_total{read_type="snapshot"}[5m])
  /
rate(themis_mvcc_reads_total[5m])

# 99. Perzentil Leselatenz
histogram_quantile(0.99, rate(themis_mvcc_read_latency_seconds_bucket[5m]))

# GC-Effizienz: durchschnittliche gelöschte Einträge pro Lauf
rate(themis_mvcc_gc_versions_deleted_total[1h])
  /
rate(themis_mvcc_gc_runs_total[1h])
```

---

## 18.6 Architekturentscheidungen

### Warum HLC statt TrueTime?

Google Spanner verwendet TrueTime (GPS + Atomuhren) für global konsistente Zeitstempel. Dies setzt spezielle Hardware voraus. ThemisDB verwendet stattdessen Hybrid Logical Clocks:

| Eigenschaft | TrueTime (Spanner) | HLC (ThemisDB) |
|-------------|-------------------|----------------|
| Hardware | GPS + Atomuhr | Keine (Softwarelösung) |
| Latenz bei Commits | 2–7 ms (wait-out) | Minimal |
| Kausalitätsgarantie | Ja | Ja |
| Externalitätskonsistenz | Stark (Global) | Gut (Node-Cluster) |
| Deployment-Komplexität | Hoch | Niedrig |

Die `DistributedTimeCoordinator`-Klasse (in `sharding/distributed_time_coordinator.h`) ergänzt HLC um konsensbasierte Zeitstempel (Raft-Log-Index) für raft-übergreifende Konsistenz.

### Schlüssel-Encoding und Null-Bytes

Das Encoding `<base_key>\x00<8-Byte-TS>` wurde bewusst mit einem Null-Byte als Trennzeichen gewählt:

1. Null-Bytes erscheinen in normalen Schlüsseln selten.
2. Die feste Länge des Zeitstempel-Teils (8 Bytes) macht das Encoding deterministisch.
3. Da Big-Endian-Zeitstempel selbst Null-Bytes enthalten können, wird der Zeitstempel immer als die **letzten 8 Bytes** dekodiert (kein `rfind`).

### Separation von MVCCStore und RocksDBWrapper

`MVCCStore` verwendet intern `RocksDBWrapper` über dessen öffentliche Interface (`put`, `get`, `scanPrefix`, `del`). Es greift **nicht** direkt auf RocksDB-Internals zu, wodurch zukünftige Storage-Backend-Änderungen isoliert bleiben.

---

## 18.7 Operativer Leitfaden

### Empfohlene GC-Strategie

```
[Periodisch, z.B. alle 10 Minuten]
Watermark = currentTimestamp() - 5 Minuten
gcAllBefore(watermark, {min_versions_to_keep: 2})

[Nach bulk-writes]
gcVersionsBefore(key, watermark, {min_versions_to_keep: 1})
```

### Überwachung der Versionserosion

Wenn `themis_mvcc_version_entries` kontinuierlich wächst, wurde GC nicht konfiguriert:

1. Prüfen ob `gc_runs_total` sich erhöht.
2. Ggf. GC-Watermark erhöhen oder Intervall verkürzen.
3. `min_versions_to_keep` auf 1 setzen, wenn kein PITR benötigt wird.

### Troubleshooting

| Problem | Symptom | Lösung |
|---------|---------|--------|
| Snapshot-Read gibt 404 | `getAtTimestamp` für alten ts | Versions wurden bereits per GC gelöscht – Watermark erhöhen |
| Hohe Schreiblatenz | `write_latency_seconds` hoch | RocksDB compaction prüfen; ggf. Schreibbatch-Größe erhöhen |
| HLC läuft nicht monoton | Nicht möglich (API-Garantie) | – |
| `version_entries` wächst | GC inaktiv | GC-Job überprüfen |
| Linearizable-Read schlägt fehl | `is_leader == false` | Anfrage an den aktuellen Raft-Leader weiterleiten |

---

## 18.9 Raft-MVCC-Bridge: Konsistente Reads über Shard-Grenzen

### Motivation

Für Snapshots und linearisierbare Reads, die **mehrere Shards** gleichzeitig betreffen, müssen alle beteiligten Shards vom selben Zeitpunkt des Raft-Commit-Index lesen.  Die `RaftMvccBridge`-Klasse (in `include/storage/raft_mvcc_bridge.h`) überbrückt die beiden Zeitstempel-Räume:

| Subsystem | Zeitstempel-Typ | Eigenschaft |
|-----------|----------------|-------------|
| `DistributedTimeCoordinator` | `int64_t` Raft-Log-Index | Kausal geordnet per Raft |
| `MVCCStore` / `HybridLogicalClock` | `HLCTimestamp` (64-Bit) | Streng monoton, wall-clock-gebunden |

### Zeitstempel-Konvertierung

```
TimeInterval.system_time_ns / 1e6  →  HLCTimestamp.physical_ms
TimeInterval.logical_timestamp & 0xFFFFF  →  HLCTimestamp.logical (20 Bit)
```

Dadurch wird der Raft-Log-Index in die niederwertigen 20 Bit des HLC-Zeitstempels eingebettet, sodass höhere Log-Indizes innerhalb derselben Millisekunde immer größere HLC-Werte erzeugen.

### Nutzung

```cpp
#include "storage/raft_mvcc_bridge.h"
using namespace themis;

// Setup (einmalig pro Shard)
auto bridge = std::make_shared<RaftMvccBridge>(mvcc_store, coordinator);

// ──── Linearisierbarer Read (nur auf dem Raft-Leader) ────
auto [is_leader, value] = bridge->linearizableRead("user:42");
if (!is_leader) {
    // An Leader weiterleiten
}

// ──── Snapshot-Zeitstempel ableiten ────
HLCTimestamp snap = bridge->snapshotTimestamp();
// snap ist jetzt ≥ jedem HLC-Timestamp eines Schreibvorgangs,
// der vor diesem Aufruf im Raft-Log committet wurde.

// ──── Konsistenter Snapshot-Read auf mehreren Shards ────
auto val_shard1 = bridge_shard1->snapshotRead("user:42", snap);
auto val_shard2 = bridge_shard2->snapshotRead("order:99", snap);
// Beide Reads sehen einen konsistenten Zustand.

// ──── Raft-bewusstes Schreiben ────
HLCTimestamp ts = bridge->raftAwareWrite("user:42", new_value);
// ts ist in das MVCC-Log eingetragen und sofort für Snapshot-Reads sichtbar.
```

### Schaubild

```
Shard A                              Shard B
┌──────────────────────┐             ┌──────────────────────┐
│  DistributedTime     │             │  DistributedTime     │
│  Coordinator         │             │  Coordinator         │
│  (Raft commit idx)   │             │  (Raft commit idx)   │
└──────────┬───────────┘             └──────────┬───────────┘
           │ snapshotTimestamp()                │ snapshotTimestamp()
           │ → HLCTimestamp T                  │ → HLCTimestamp T
           ▼                                   ▼
┌──────────────────────┐             ┌──────────────────────┐
│  MVCCStore           │             │  MVCCStore           │
│  getAtTimestamp(T)   │             │  getAtTimestamp(T)   │
└──────────────────────┘             └──────────────────────┘
          Beide Shards lesen denselben konsistenten Snapshot
```

---

## 18.10 Referenzen

- Kulkarni, S. et al. (2014). *Logical Physical Clocks and Consistent Snapshots in Globally Distributed Databases.* ([arXiv](https://arxiv.org/abs/1507.05768))
- Corbett, J. et al. (2012). *Spanner: Google's Globally Distributed Database.* OSDI.
- [CockroachDB MVCC](https://www.cockroachlabs.com/docs/stable/architecture/storage-layer.html#mvcc)
- [FoundationDB Record Layer](https://foundationdb.github.io/fdb-record-layer/)
- ThemisDB: `include/storage/hlc.h`, `include/storage/mvcc_store.h`, `include/storage/raft_mvcc_bridge.h`, `include/server/mvcc_api_handler.h`
