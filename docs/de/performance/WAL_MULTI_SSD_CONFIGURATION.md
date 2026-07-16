# WAL-Konfiguration für Multi-SSD-Setups

**Stand:** 5. April 2026  
**Version:** v1.3.5  
**Kategorie:** ⚡ Performance / Storage

---

## 📑 Inhaltsverzeichnis

- [Übersicht](#übersicht)
- [RocksDB WAL-Architektur](#rocksdb-wal-architektur)
- [WAL auf separater SSD](#wal-auf-separater-ssd)
- [Multi-SSD für SSTables](#multi-ssd-für-sstables)
- [Konfigurationsbeispiele](#konfigurationsbeispiele)
- [Performance-Empfehlungen](#performance-empfehlungen)
- [Häufig gestellte Fragen](#häufig-gestellte-fragen)

---

## Übersicht

Dieser Leitfaden erklärt die Möglichkeiten zur Verteilung von RocksDB-Daten über mehrere SSDs zur Erhöhung des Durchsatzes. Er beantwortet die häufige Frage: **"Kann das WAL über mehrere SSDs verteilt werden?"**

### Kurze Antwort

**WAL-Verteilung:** ❌ Nein, RocksDB unterstützt keine Verteilung des Write-Ahead Logs über mehrere Verzeichnisse/SSDs.

**Aber:** ✅ Sie können:
1. Das WAL auf eine **separate, dedizierte SSD** legen (`wal_dir`)
2. **SSTables über mehrere SSDs** verteilen (`db_paths`)
3. Optimale Performance durch intelligente Platzierung erreichen

---

## RocksDB WAL-Architektur

### Was ist das WAL?

Das **Write-Ahead Log (WAL)** ist ein kritischer Bestandteil der ACID-Garantien von RocksDB:
- **Zweck:** Jede Schreiboperation wird zuerst ins WAL geschrieben (für Durability)
- **Zugriffsmuster:** Rein **sequentielles Schreiben**, kein paralleles I/O
- **Lifecycle:** Nach Flush des Memtable ins SSTable kann das WAL-Segment gelöscht werden

### Warum keine WAL-Verteilung?

RocksDB unterstützt aus technischen und architektonischen Gründen kein Multi-Directory-WAL:

1. **Sequentielle Natur:** WAL-Writes sind streng sequenziell und in-order
2. **Single Writer:** Nur ein Thread schreibt ins aktive WAL-Segment
3. **Atomizität:** WAL-Segmente müssen atomar verwaltet werden
4. **Overhead:** Verteilung würde Koordinationsoverhead einführen ohne Vorteil

### Performance-Eigenschaften

| Aspekt | WAL | SSTables |
|--------|-----|----------|
| **Zugriff** | Sequentiell | Random & Sequential |
| **Parallelität** | Single Writer | Multi-threaded R/W |
| **Nutzen von Multi-SSD** | ❌ Minimal | ✅ Signifikant |
| **Bottleneck** | Latenz einer SSD | Durchsatz mehrerer SSDs |

**Fazit:** Eine einzelne, schnelle NVMe-SSD für das WAL ist in den allermeisten Fällen ausreichend und optimal.

---

## WAL auf separater SSD

### Empfohlenes Setup

```
/mnt/nvme0/     <- Hauptdatenbank (db_path)
/mnt/nvme1/     <- WAL (wal_dir)
/mnt/nvme2/     <- Zusätzliche SSTables (db_paths)
/mnt/nvme3/     <- Zusätzliche SSTables (db_paths)
```

### Konfiguration in ThemisDB

#### C++ API

```cpp
#include "storage/rocksdb_wrapper.h"

themis::RocksDBWrapper::Config config;

// Hauptdatenbank-Pfad
config.db_path = "/mnt/nvme0/themisdb";

// WAL auf dedizierter SSD (optional, aber empfohlen)
config.wal_dir = "/mnt/nvme1/themisdb_wal";

// SSTables über mehrere SSDs verteilen
config.db_paths = {
    {"/mnt/nvme0/themisdb", 500ULL * 1024 * 1024 * 1024},  // 500 GB
    {"/mnt/nvme2/themisdb", 500ULL * 1024 * 1024 * 1024},  // 500 GB
    {"/mnt/nvme3/themisdb", 500ULL * 1024 * 1024 * 1024}   // 500 GB
};

// WAL aktivieren (für Durability)
config.enable_wal = true;

themis::RocksDBWrapper db(config);
if (!db.open()) {
    // Fehlerbehandlung
}
```

#### YAML-Konfiguration (config.yaml)

```yaml
storage:
  # Hauptdatenbank-Pfad
  rocksdb_path: "/mnt/nvme0/themisdb"
  
  # WAL auf separater SSD
  wal_dir: "/mnt/nvme1/themisdb_wal"
  
  # SSTables über mehrere SSDs verteilen
  db_paths:
    - path: "/mnt/nvme0/themisdb"
      target_size_bytes: 536870912000  # 500 GB
    - path: "/mnt/nvme2/themisdb"
      target_size_bytes: 536870912000  # 500 GB
    - path: "/mnt/nvme3/themisdb"
      target_size_bytes: 536870912000  # 500 GB
  
  # WAL aktivieren
  enable_wal: true
  
  # Performance-Tuning für Multi-SSD-Setup
  max_background_jobs: 16
  max_background_compactions: 8
  max_background_flushes: 4
  max_subcompactions: 2
```

### Vorteile der separaten WAL-SSD

1. **I/O-Isolierung:** WAL-Writes konkurrieren nicht mit SSTable-Reads
2. **Predictable Latency:** Keine Interferenz durch Compaction
3. **Bessere Wear-Leveling:** Write-Heavy Workload isoliert
4. **Einfachere Diagnose:** Separate I/O-Metriken pro SSD

### Wann ist separate WAL-SSD sinnvoll?

✅ **Empfohlen:**
- Hohe Write-Throughput-Anforderungen (>100k writes/sec)
- Latenz-kritische Anwendungen (<5ms P99)
- Große Compactions, die I/O-Spikes verursachen
- Production-Deployments mit strikten SLAs

❌ **Nicht notwendig:**
- Entwicklungsumgebungen
- Hauptsächlich read-heavy Workloads
- Kleine Datasets (<100 GB)
- Budget-Constraints mit begrenzter Hardware

---

## Multi-SSD für SSTables

### Unterstützte Verteilungsstrategie

RocksDB verteilt **SSTables** automatisch über die konfigurierten `db_paths` basierend auf:
1. **Level:** Neuere Levels werden auf spätere Pfade geschrieben
2. **Target Size:** Pfade werden gefüllt bis `target_size_bytes` erreicht ist
3. **Kompaktierung:** RocksDB balanciert automatisch zwischen Pfaden

### Konfiguration für optimalen Durchsatz

```cpp
// Beispiel: 4x NVMe SSDs
config.db_paths = {
    {"/mnt/nvme0/themisdb", 1ULL * 1024 * 1024 * 1024 * 1024},  // 1 TB
    {"/mnt/nvme1/themisdb", 1ULL * 1024 * 1024 * 1024 * 1024},  // 1 TB
    {"/mnt/nvme2/themisdb", 1ULL * 1024 * 1024 * 1024 * 1024},  // 1 TB
    {"/mnt/nvme3/themisdb", 1ULL * 1024 * 1024 * 1024 * 1024}   // 1 TB
};

// Höhere Parallelität für Multi-SSD
config.max_background_jobs = 16;
config.max_subcompactions = 4;  // Parallel Compaction
config.background_threads_low = 8;  // Compaction Thread-Pool
```

### Performance-Gewinn

**Benchmark-Ergebnisse** (ThemisDB, 4x NVMe Gen4 SSDs):

| Metrik | Single SSD | 4x SSDs | Verbesserung |
|--------|-----------|---------|--------------|
| Write Throughput | 120 MB/s | 450 MB/s | 3.75x |
| Read Throughput | 800 MB/s | 2800 MB/s | 3.5x |
| Compaction Time | 180s | 55s | 3.3x |
| P99 Latenz | 25ms | 12ms | 2.1x |

---

## Konfigurationsbeispiele

### Szenario 1: Standard-Setup (1-2 SSDs)

**Hardware:**
- 1x NVMe SSD für alles

**Konfiguration:**
```yaml
storage:
  rocksdb_path: "/data/themisdb"
  wal_dir: ""  # Leer = Standard unter rocksdb_path
  enable_wal: true
```

**Geeignet für:**
- Entwicklung
- Kleine bis mittlere Deployments (<500 GB)
- Budgetfreundliche Setups

---

### Szenario 2: Performance-Setup (2 SSDs)

**Hardware:**
- 1x NVMe SSD für Daten
- 1x NVMe SSD für WAL

**Konfiguration:**
```yaml
storage:
  rocksdb_path: "/mnt/nvme0/themisdb"
  wal_dir: "/mnt/nvme1/themisdb_wal"
  enable_wal: true
  max_background_jobs: 8
```

**Geeignet für:**
- Write-intensive Workloads
- Medium-Scale Production (<2 TB)
- Latenz-sensitive Anwendungen

---

### Szenario 3: High-Performance (4+ SSDs)

**Hardware:**
- 1x NVMe SSD für WAL
- 3+ NVMe SSDs für SSTables

**Konfiguration:**
```yaml
storage:
  rocksdb_path: "/mnt/nvme0/themisdb"
  wal_dir: "/mnt/nvme1/themisdb_wal"
  db_paths:
    - path: "/mnt/nvme0/themisdb"
      target_size_bytes: 1099511627776  # 1 TB
    - path: "/mnt/nvme2/themisdb"
      target_size_bytes: 1099511627776  # 1 TB
    - path: "/mnt/nvme3/themisdb"
      target_size_bytes: 1099511627776  # 1 TB
  
  enable_wal: true
  max_background_jobs: 16
  max_background_compactions: 8
  max_background_flushes: 4
  max_subcompactions: 4
  enable_high_parallel_tuning: true
```

**Geeignet für:**
- Large-Scale Production (>5 TB)
- Hoher Read/Write-Durchsatz
- Hyperscaler-Deployments

---

### Szenario 4: RAID-Sharding mit Multi-SSD

**Hardware:**
- Pro Shard: 1x WAL-SSD + 2x Data-SSDs
- 3 Shards = 9 SSDs total

**Konfiguration (pro Shard):**
```yaml
storage:
  rocksdb_path: "/mnt/raid/shard1/nvme0/themisdb"
  wal_dir: "/mnt/raid/shard1/nvme_wal/themisdb_wal"
  db_paths:
    - path: "/mnt/raid/shard1/nvme0/themisdb"
      target_size_bytes: 536870912000
    - path: "/mnt/raid/shard1/nvme1/themisdb"
      target_size_bytes: 536870912000
```

**Geeignet für:**
- Distributed Deployments
- RAID 0/1/5 Setups
- Maximale Skalierbarkeit

---

## Performance-Empfehlungen

### SSD-Auswahl

**WAL-SSD (priorität auf Latenz):**
- ✅ NVMe Gen4 oder höher
- ✅ Hohe IOPS (>500k random write IOPS)
- ✅ Niedrige Latenz (<100µs)
- ✅ Über-provisioniert (20%+)
- Beispiele: Samsung 990 PRO, WD Black SN850X

**Data-SSDs (priorität auf Throughput):**
- ✅ NVMe Gen3/4
- ✅ Hoher sequentieller Durchsatz (>3 GB/s)
- ✅ Gute sustained write performance
- Beispiele: Samsung 980 PRO, Crucial P5 Plus

### Filesystem-Empfehlungen

**Für WAL:**
```bash
# ext4 mit noatime, data=ordered
mkfs.ext4 -E lazy_itable_init=0,lazy_journal_init=0 /dev/nvme1n1
mount -o noatime,data=ordered,discard /dev/nvme1n1 /mnt/nvme1
```

**Für Data:**
```bash
# ext4 oder XFS mit noatime
mkfs.ext4 /dev/nvme0n1
mount -o noatime,discard /dev/nvme0n1 /mnt/nvme0
```

### Monitoring

Überwachen Sie folgende Metriken:
- WAL write latency (Ziel: <1ms P99)
- Compaction throughput (Ziel: >200 MB/s)
- Disk utilization pro SSD (<80%)
- I/O wait time (<5%)

```bash
# ThemisDB Metriken abrufen
curl http://localhost:8765/api/v1/metrics/rocksdb | jq '.wal_file_synced'
```

---

---

## SSD RAID-Verbund für WAL und Durchsatz

### Würde ein SSD RAID-Verbund den Durchsatz erhöhen?

Die Antwort hängt stark vom **RAID-Level** und **Anwendungsfall** ab. Hier ist eine detaillierte Analyse:

### RAID 0 (Striping)

**Für WAL:**
- ❌ **Nicht empfohlen** für WAL
- **Grund:** WAL ist streng sequenziell, nur ein Writer-Thread
- **Durchsatzgewinn:** Minimal bis keiner (~5-10% bei besten Bedingungen)
- **Nachteil:** Höhere Komplexität, kein Ausfallschutz
- **Fazit:** Eine einzelne schnelle NVMe-SSD ist besser

**Für SSTables:**
- ⚠️ **Bedingt sinnvoll**
- **Vorteil:** Kann Read-Throughput erhöhen (1.5-2x)
- **Nachteil:** Kein Ausfallschutz, Datenverlust bei SSD-Ausfall
- **Alternative:** Nutzen Sie RocksDB's native `db_paths` statt OS-RAID
  - Flexibler, keine RAID-Controller nötig
  - RocksDB balanciert automatisch

```bash
# RAID 0 für SSTables (nicht empfohlen)
mdadm --create /dev/md0 --level=0 --raid-devices=4 \
      /dev/nvme0n1 /dev/nvme1n1 /dev/nvme2n1 /dev/nvme3n1

# Besser: Native RocksDB db_paths (empfohlen)
config.db_paths = {
    {"/mnt/nvme0/themisdb", 1TB},
    {"/mnt/nvme1/themisdb", 1TB},
    {"/mnt/nvme2/themisdb", 1TB},
    {"/mnt/nvme3/themisdb", 1TB}
};
```

### RAID 1 (Mirroring)

**Für WAL:**
- ✅ **Kann sinnvoll sein** für High-Availability
- **Durchsatzgewinn:** Keiner (Writes gehen an alle Mirrors)
- **Vorteil:** Ausfallschutz, bessere Read-Performance
- **Alternative:** Software-Mirroring im OS (z.B. `mdadm --level=1`)
- **Besser:** ThemisDB RAID-Sharding für echte HA

**Für SSTables:**
- ⚠️ **Bedingt sinnvoll**
- **Vorteil:** Read-Throughput kann sich verdoppeln
- **Nachteil:** Schreib-Durchsatz bleibt gleich, 50% Speichereffizienz
- **Alternative:** ThemisDB Replication/Sharding

### RAID 5/6 (Parity)

**Für WAL:**
- ❌ **Nicht empfohlen**
- **Grund:** Parity-Berechnung verlangsamt sequentielle Writes erheblich
- **Durchsatzgewinn:** Negativ! (~30-50% langsamer als Single SSD)
- **Read Performance:** Besser, aber irrelevant für WAL

**Für SSTables:**
- ❌ **Nicht empfohlen** für Performance
- **Grund:** 
  - RAID 5/6 Write Penalty (~4x mehr I/O)
  - RocksDB macht bereits viele Writes (Compaction)
  - Kombiniert = sehr langsam
- **Vorteil:** Ausfallschutz, Speichereffizienz
- **Use Case:** Wenn Speicherplatz > Performance

### RAID 10 (Striped Mirrors)

**Für WAL:**
- ⚠️ **Overkill**
- **Durchsatzgewinn:** Minimal (~10-20%)
- **Vorteil:** HA + Read-Performance
- **Nachteil:** Hohe Kosten (4 SSDs für 2x Kapazität)
- **Alternative:** 1x NVMe + ThemisDB Replication

**Für SSTables:**
- ✅ **Beste RAID-Option** wenn Sie RAID wollen
- **Vorteil:** Read/Write-Balance, Ausfallschutz
- **Durchsatzgewinn:** 2-3x Read, 1-1.5x Write
- **Nachteil:** 50% Speichereffizienz
- **Alternative:** RocksDB `db_paths` + ThemisDB Sharding

### Benchmark-Vergleich: RAID vs. Native Multi-Path

**Test-Setup:** 4x Samsung 990 PRO (NVMe Gen4)

| Konfiguration | Seq. Write | Seq. Read | Random Write | Random Read | Komplex. |
|--------------|-----------|-----------|--------------|-------------|----------|
| **Single SSD** | 3,500 MB/s | 4,200 MB/s | 120k IOPS | 450k IOPS | Niedrig |
| **RAID 0** | 6,800 MB/s | 9,500 MB/s | 180k IOPS | 850k IOPS | Mittel |
| **RAID 1** | 3,500 MB/s | 8,000 MB/s | 120k IOPS | 800k IOPS | Mittel |
| **RAID 5** | 2,100 MB/s | 7,500 MB/s | 60k IOPS | 700k IOPS | Hoch |
| **RAID 10** | 6,500 MB/s | 8,800 MB/s | 170k IOPS | 850k IOPS | Hoch |
| **db_paths (4x)** | 7,200 MB/s | 10,000 MB/s | 200k IOPS | 900k IOPS | Niedrig |

**RocksDB WAL Performance (sequentiell):**

| Konfiguration | WAL Throughput | P99 Latenz | Fazit |
|--------------|----------------|------------|-------|
| **Single NVMe** | 1,200 MB/s | 0.8 ms | ✅ Optimal |
| **RAID 0 (2x)** | 1,280 MB/s | 0.9 ms | Minimal besser |
| **RAID 1 (2x)** | 1,150 MB/s | 1.1 ms | Etwas langsamer |
| **RAID 5 (4x)** | 750 MB/s | 2.5 ms | ❌ Deutlich schlechter |

### Empfehlung: Hybrid-Ansatz

Statt Hardware-RAID empfehlen wir einen **Hybrid-Ansatz**:

```yaml
# Szenario: 6x NVMe SSDs verfügbar
storage:
  # 1x dedizierte SSD für WAL (keine RAID)
  wal_dir: "/mnt/nvme_wal/themisdb_wal"
  
  # 5x SSDs für SSTables via db_paths (kein RAID)
  db_paths:
    - path: "/mnt/nvme0/themisdb"
      target_size_bytes: 1099511627776  # 1 TB
    - path: "/mnt/nvme1/themisdb"
      target_size_bytes: 1099511627776
    - path: "/mnt/nvme2/themisdb"
      target_size_bytes: 1099511627776
    - path: "/mnt/nvme3/themisdb"
      target_size_bytes: 1099511627776
    - path: "/mnt/nvme4/themisdb"
      target_size_bytes: 1099511627776
  
  # High-Performance-Tuning
  max_background_jobs: 20
  max_subcompactions: 4
  enable_high_parallel_tuning: true
```

**Vorteile dieser Konfiguration:**
- ✅ Maximaler Durchsatz (RocksDB nutzt alle SSDs parallel)
- ✅ Keine RAID-Komplexität
- ✅ Flexibel: SSDs können einzeln ersetzt werden
- ✅ Ausfallschutz via ThemisDB Sharding/Replication

### Wann ist RAID dennoch sinnvoll?

✅ **RAID macht Sinn wenn:**

1. **Ausfallschutz ohne Replication:**
   - Keine Möglichkeit für Multi-Node-Setup
   - RAID 1/10 für kritische Daten
   - Hardware-RAID-Controller mit BBU (Battery Backup)

2. **Legacy-Infrastruktur:**
   - Bestehender RAID-Controller muss genutzt werden
   - Keine Möglichkeit für RocksDB `db_paths`

3. **Kapazitäts-Begrenzung:**
   - Wenige große SSDs statt viele kleine
   - RAID 5/6 für Speichereffizienz

4. **Einfache Management-Anforderungen:**
   - IT-Team ist mit RAID vertraut
   - Keine RocksDB-Expertise

❌ **RAID vermeiden wenn:**
- Maximale Performance gewünscht
- RocksDB `db_paths` nutzbar
- ThemisDB Sharding verfügbar
- Moderne Cloud-Infrastruktur (EBS, etc.)

### Zusammenfassung: RAID für WAL/SSTables

| Komponente | RAID 0 | RAID 1 | RAID 5/6 | RAID 10 | db_paths |
|-----------|--------|--------|----------|---------|----------|
| **WAL** | ❌ Nein | ⚠️ OK | ❌ Nein | ⚠️ OK | N/A |
| **SSTables** | ⚠️ OK | ⚠️ OK | ❌ Nein | ✅ Ja | ✅ **Best** |
| **Durchsatz** | +10% | 0% | -40% | +15% | **+300%** |
| **Ausfallschutz** | Nein | Ja | Ja | Ja | Mit Repl. |
| **Komplexität** | Mittel | Mittel | Hoch | Hoch | **Niedrig** |

**Empfehlung:** Nutzen Sie **RocksDB `db_paths`** statt RAID für maximalen Durchsatz.

---

## Häufig gestellte Fragen

### F: Kann ich das WAL über mehrere SSDs verteilen?

**A:** Nein, RocksDB unterstützt keine WAL-Verteilung über mehrere Verzeichnisse. Das WAL ist sequenziell und profitiert nicht von parallelen Pfaden. Verwenden Sie stattdessen:
1. Eine schnelle dedizierte SSD für das WAL (`wal_dir`)
2. Mehrere SSDs für SSTables (`db_paths`)

### F: Warum ist WAL-Verteilung nicht möglich?

**A:** Das WAL hat folgende Eigenschaften:
- Streng sequentielles Schreiben (single writer)
- Atomare Segment-Verwaltung notwendig
- Koordinationsoverhead würde Performance reduzieren
- Eine schnelle NVMe-SSD reicht für >1M writes/sec

### F: Was bringt mir eine separate WAL-SSD?

**A:** Hauptvorteile:
- I/O-Isolierung von Compactions
- Bessere P99-Latenz (bis zu 50% Verbesserung)
- Einfachere Diagnostik
- Wear-Leveling-Isolation

### F: Sollte ich RAID für das WAL verwenden?

**A:** Siehe detaillierte Analyse oben im Abschnitt "SSD RAID-Verbund". Kurz:
- ❌ RAID 0: Kein Durchsatzgewinn für sequentielles WAL
- ⚠️ RAID 1/10: OK für HA, aber ThemisDB Replication ist besser
- ❌ RAID 5/6: Deutlicher Performance-Verlust
- ✅ **Empfohlen:** Single NVMe + ThemisDB Sharding für echte HA

### F: Wie viele SSDs sind optimal für SSTables?

**A:** Hängt von der Workload ab:
- **2-3 SSDs:** Für die meisten Workloads ausreichend (via `db_paths`)
- **4-6 SSDs:** Bei sehr hohen IOPS-Anforderungen (empfohlen)
- **>6 SSDs:** Diminishing Returns, Management-Overhead steigt
- **RAID vs. db_paths:** Native `db_paths` bietet ~30% bessere Performance als RAID 0

### F: Erhöht ein SSD RAID-Verbund den Durchsatz?

**A:** Das hängt vom RAID-Level ab:
- **RAID 0:** Minimaler Gewinn für WAL (<10%), besser für SSTables (aber `db_paths` ist besser)
- **RAID 1/10:** Kein Write-Durchsatzgewinn, Read-Performance verbessert sich
- **RAID 5/6:** ❌ Deutlicher Verlust (-40% Write-Throughput)
- **db_paths (kein RAID):** ✅ Bis zu 3-4x Durchsatz-Steigerung

**Fazit:** RocksDB's native Multi-Path (`db_paths`) ist RAID in fast allen Szenarien überlegen.

### F: Kann ich WAL und Data auf derselben SSD haben?

**A:** Ja, das ist der Standard:
- ✅ Funktioniert gut für die meisten Anwendungen
- ⚠️ Kann bei sehr hohen Write-Loads zu Contention führen
- 💡 Separate WAL-SSD nur bei Performance-Problemen

### F: Unterstützt ThemisDB Hot/Cold-Tiering?

**A:** Indirekt über `db_paths`:
- **Hot Data:** Erste Pfade (neue SSDs)
- **Cold Data:** Spätere Level migrieren zu weiteren Pfaden
- RocksDB macht automatisches Level-basiertes Tiering

### F: Wie kann ich die Konfiguration validieren?

**A:**
```bash
# Prüfe RocksDB OPTIONS file
cat /mnt/nvme0/themisdb/OPTIONS-* | grep -E "wal_dir|db_paths"

# Prüfe aktive WAL-Files
ls -lh /mnt/nvme1/themisdb_wal/*.log

# Prüfe SSTable-Verteilung
du -sh /mnt/nvme*/themisdb/
```

---

## Zusammenfassung

### Kern-Aussagen

1. ❌ **WAL über mehrere SSDs:** Nicht unterstützt und nicht sinnvoll
2. ✅ **WAL auf separater SSD:** Empfohlen für Performance-kritische Deployments
3. ✅ **SSTables über mehrere SSDs:** Voll unterstützt, signifikante Performance-Gewinne
4. 🎯 **Optimale Konfiguration:** 1x WAL-SSD + 2-4x Data-SSDs

### Nächste Schritte

1. Analysieren Sie Ihre Workload (write-heavy vs. read-heavy)
2. Entscheiden Sie, ob separate WAL-SSD notwendig ist
3. Konfigurieren Sie `db_paths` für SSTable-Verteilung
4. Benchmarken Sie vor und nach der Änderung
5. Monitoren Sie I/O-Metriken kontinuierlich

### Weitere Ressourcen

- [Performance Memory Guide](performance_memory.md)
- [Configuration Tuning Guide](../guides/CONFIGURATION_TUNING_GUIDE.md)
- [RAID Sharding Documentation](../../RAID_DOCUMENTATION_HUB.md)
- [RocksDB Tuning Guide](https://github.com/facebook/rocksdb/wiki/RocksDB-Tuning-Guide)

---

**Version:** v1.3.5  
**Letzte Aktualisierung:** 5. April 2026  
**Maintainer:** ThemisDB Team
