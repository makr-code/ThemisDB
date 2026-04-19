> ⚠️ **Historischer Plan** – Optimierungsideen beschreiben den Stand nach Phase 2H.

# Phase 2H: Weitere RocksDB-Optimierungen basierend auf offizieller Dokumentation

## Problem
- pipelined_write zeigt inkonsistentes Verhalten: +82% @ 1T, aber -60% @ 4T+
- Skalierung verschlechtert sich bei höheren Thread-Zahlen (4T+)
- WritePrepared zeigt nur moderate Verbesserungen

## Neue Erkenntnisse aus RocksDB Tuning Guide

### 1. Parallelism Options - Der Schlüssel zur Skalierung!

Die Dokumentation betont:
> "When compaction is lagging behind while still far from saturating the disk, try to increase compaction parallelism."

**Wichtige Parameter für hohe Parallelität:**

```cpp
// Background Thread Pools
options.env->SetBackgroundThreads(num_threads, Env::Priority::HIGH);  // Flush threads
options.env->SetBackgroundThreads(num_threads, Env::Priority::LOW);   // Compaction threads

// Max concurrent operations
options.max_background_compactions  // Default: 1 (!)
options.max_background_flushes      // Default: 1
options.max_subcompactions          // For parallel compaction
```

**Unsere aktuelle Konfiguration:**
- Wir haben diese Parameter vermutlich NICHT optimiert!
- Default max_background_compactions = 1 ist ein massiver Bottleneck bei hoher Parallelität
- Bei 8+ Threads konkurrieren alle um 1-2 Background-Threads

### 2. Write Buffer Konfiguration für hohe Parallelität

```cpp
// Größere Write Buffer = bessere Parallelität
options.write_buffer_size = 128MB;  // Standard: 64MB
options.max_write_buffer_number = 4;  // Standard: 2
options.min_write_buffer_number_to_merge = 1;  // Standard: 1
```

**Auswirkung:**
- Bei mehr Threads werden mehr Write Buffers gleichzeitig benötigt
- Default (2) führt zu Stalls bei hoher Parallelität

### 3. Memtable Concurrent Write - KRITISCH!

Aus der Dokumentation:
> "Concurrent memtable insert is enabled by default via `allow_concurrent_memtable_write`"
> "Only skiplist-based memtable supports concurrent insert"

**ABER:** Es gibt Kompatibilitätsprobleme:
- `allow_concurrent_memtable_write` (Default: true)
- `enable_pipelined_write` (Default: false)
- **Diese beiden Optionen können interferieren!**

### 4. Level0 File Limits - Write Stall Ursache

```cpp
// Trigger für Write Stalls
options.level0_slowdown_writes_trigger = 20;  // Stall begins
options.level0_stop_writes_trigger = 36;      // Full stop
options.level0_file_num_compaction_trigger = 4;
```

**Bei hoher Parallelität:**
- Mehr Threads → mehr L0 Files → Write Stalls
- Write Stalls erklären Performance-Einbruch!

### 5. Lock Contention - TransactionDB spezifisch

Aus der Transactions-Dokumentation:
> "PointLockManager has only 16 locks per column family"

**Das ist ein MASSIVER Bottleneck bei Parallelität!**

```cpp
// In unserem Code vermutlich:
// size_t num_stripes = 16  (nur 16 Locks für alle Threads!)
```

Bei 8+ Threads konkurrieren alle um nur 16 Locks!

### 6. Block Cache Sharding

```cpp
// Aus der Doku:
block_cache = NewLRUCache(cache_capacity, shard_bits);

// Bei hoher Parallelität kritisch:
// shard_bits = 4 → 16 Shards
// shard_bits = 6 → 64 Shards (besser für 8+ Threads)
```

**Problem:**
- Default Sharding ist für geringe Parallelität optimiert
- Bei 8+ Threads: Cache Mutex Contention!

### 7. Write Stall - Die Hauptursache der Verschlechterung

Die Dokumentation beschreibt **drei Haupt-Stall-Ursachen:**

#### a) Zu viele Memtables
```
Stopping writes because we have 5 immutable memtables (waiting for flush)
```

#### b) Zu viele L0 Files
```
Stalling writes because we have 4 level-0 files
```

#### c) Zu viele Pending Compaction Bytes
```
Stalling writes because of estimated pending compaction bytes 500000000
```

**Bei höheren Thread-Zahlen:**
- Mehr parallele Writes → schneller Memtable-Flush
- Mehr L0 Files → Compaction hält nicht mit
- Compaction mit nur 1 Thread (Default!) ist Bottleneck

## Prioritäten für Phase 2H

### Priority 1: Background Thread Konfiguration (KRITISCH)
```cpp
// Mindestens so viele Threads wie CPU-Kerne
options.env->SetBackgroundThreads(8, Env::Priority::HIGH);
options.env->SetBackgroundThreads(8, Env::Priority::LOW);
options.max_background_compactions = 4;  // Statt Default 1!
options.max_background_flushes = 2;      // Statt Default 1!
options.max_subcompactions = 4;
```

**Erwartete Verbesserung:** +50-100% bei 8+ Threads

### Priority 2: Write Buffer Tuning
```cpp
options.write_buffer_size = 128 * 1024 * 1024;  // 128MB statt 64MB
options.max_write_buffer_number = 4;             // 4 statt 2
options.db_write_buffer_size = 512 * 1024 * 1024; // Gesamt-Limit
```

### Priority 3: Level0 Kompaction Aggressiver
```cpp
options.level0_file_num_compaction_trigger = 2;  // Statt 4
options.level0_slowdown_writes_trigger = 10;     // Früher eingreifen
options.max_bytes_for_level_base = 512MB;        // Größeres L1
```

### Priority 4: Lock Stripes für TransactionDB erhöhen
```cpp
// Im TransactionDB Options:
TransactionDBOptions txn_db_opts;
// Leider: num_stripes ist hart auf 16 codiert in PointLockManager
// Alternative: Sharding auf mehrere DB-Instanzen
```

### Priority 5: Block Cache Sharding erhöhen
```cpp
// Für 8+ Threads:
table_options.block_cache = NewLRUCache(
    cache_size,
    6  // shard_bits: 64 Shards statt 16
);
```

### Priority 6: Concurrent vs Pipelined Write - Konflikt auflösen

**Problem:** Diese beiden Optionen können interferieren:
```cpp
// Option A: Concurrent Memtable (besser für Parallelität)
options.allow_concurrent_memtable_write = true;
options.enable_pipelined_write = false;

// Option B: Pipelined (besser für 1T, schlechter für 4T+)
options.allow_concurrent_memtable_write = false;
options.enable_pipelined_write = true;
```

**Empfehlung:** Option A für unseren Use Case (4-16 Threads)

## Analyse: Warum pipelined_write versagt hat

### Bei 1 Thread: +82% Performance
- Pipelining reduziert Latenz
- Kein Lock-Contention
- Optimale Nutzung der I/O-Pipeline

### Bei 4+ Threads: -60% Performance
- Pipelined Write serialisiert Writes
- Concurrent Memtable Write wird deaktiviert
- Lock Contention auf Pipeline-Mutex
- Write Stalls durch L0 File Buildup
- Nur 1 Compaction Thread (Default) als Bottleneck

## Vergleich mit "Total ordered database, flash storage" Beispiel

Die Dokumentation zeigt eine Konfiguration für hohe Parallelität:

```cpp
options.env->SetBackgroundThreads(4);  // Thread Pool
options.write_buffer_size = 67108864;  // 64MB
options.max_write_buffer_number = 3;
options.max_background_compactions = 4;  // !!! 4 statt 1
options.level0_file_num_compaction_trigger = 8;
options.level0_slowdown_writes_trigger = 17;
options.level0_stop_writes_trigger = 24;
options.max_bytes_for_level_base = 536870912;  // 512MB
options.max_bytes_for_level_multiplier = 8;
```

**Key Differences zu unserer Konfiguration:**
- Wir haben vermutlich max_background_compactions = 1 (Default)
- Wir haben keine aggressiven L0 Trigger
- Wir haben kleinere Write Buffer Limits

## Implementierungsplan Phase 2H

### Schritt 1: Background Threads Baseline
Test mit **nur** Background Thread Erhöhung:
- max_background_compactions = 1/2/4/8
- Measure @ 1/4/8/16 Threads
- Erwarte: Größter Einzeleffekt

### Schritt 2: Write Buffer Tuning
Kombiniert mit Background Threads:
- write_buffer_size = 128MB
- max_write_buffer_number = 4
- db_write_buffer_size = 512MB

### Schritt 3: Level0 Tuning
Aggressivere Compaction:
- level0_file_num_compaction_trigger = 2
- level0_slowdown_writes_trigger = 10

### Schritt 4: Cache Sharding
- block_cache shard_bits = 6 (64 Shards)

### Schritt 5: Full Optimized Config
Alle Optimierungen kombiniert

## Zu erwartende Ergebnisse

### Baseline (Phase 2G Best = NoPipe Txn10 @ 4T: 914k ops/s)

**Nach Phase 2H Background Thread Optimierung:**
- 1T: ~1.5M ops/s (kein Unterschied)
- 4T: ~1.5M ops/s (+64% vs. Phase 2G)
- 8T: ~2.0M ops/s (+154% vs. Phase 2G 787k)
- 16T: ~2.5M ops/s (+149% vs. Phase 2G 1.0M)
- 32T: ~2.0M ops/s (+82% vs. Phase 2G 1.1M)

**Begründung:**
- Compaction hält jetzt mit Write-Rate mit
- Keine Write Stalls mehr
- Volle CPU-Auslastung
- Lock Contention reduziert

## Next Steps

1. Aktuelle Config prüfen: Welche Parameter haben wir bereits gesetzt?
2. Background Threads Baseline testen
3. Schrittweise weitere Optimierungen
4. Log-Analyse: Suche nach "Stalling writes" / "Stopping writes"
5. Statistics aktivieren und STALL_MICROS messen

## Quellen

- RocksDB Tuning Guide: Parallelism Options
- Write Stalls Documentation
- Transactions Documentation (Lock Contention)
- MemTable Documentation (Concurrent Insert)
- Example Configurations (Total ordered database)
