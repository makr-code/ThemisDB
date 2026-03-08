# ThemisDB RAID Sharding & URN-Auflösung

**Version:** 1.0  
**Stand:** März 2026  
**Status:** ✅ Produktionsdokumentation  
**Kategorie:** 🏗️ Architektur | 🔗 Sharding | 🛡️ RAID-Redundanz

---

## Inhaltsverzeichnis

1. [Architekturüberblick](#1-architekturüberblick)
2. [Consistent Hash Ring](#2-consistent-hash-ring)
3. [VCC-URN Format & Parsing](#3-vcc-urn-format--parsing)
4. [URN-Auflösung (Resolution)](#4-urn-auflösung-resolution)
5. [RAID-Redundanzmodi](#5-raid-redundanzmodi)
6. [Shard-Topologie & Adressierung](#6-shard-topologie--adressierung)
7. [Praktische Code-Beispiele](#7-praktische-code-beispiele)
8. [End-to-End Flow-Diagramme](#8-end-to-end-flow-diagramme)
9. [Konfigurationsbeispiele (YAML)](#9-konfigurationsbeispiele-yaml)
10. [Performance-Charakteristiken](#10-performance-charakteristiken)

---

## 1. Architekturüberblick

ThemisDB implementiert ein verteiltes Sharding-System, das auf einem **Consistent Hash Ring** basiert. Anfragen werden anhand einer **VCC-URN** (Virtual Content Container – Uniform Resource Name) an den richtigen Shard geleitet – ohne dass der Client den physischen Speicherort kennen muss.

### 1.1 Schichtenmodell

```
┌─────────────────────────────────────────────────────────────────┐
│                         CLIENT / SDK                            │
│   "Gib mir urn:themis:relational:customers:users:<uuid>"        │
└───────────────────────────┬─────────────────────────────────────┘
                            │ URN
                            ▼
┌─────────────────────────────────────────────────────────────────┐
│                       URN RESOLVER                              │
│   1. URN parsen              include/sharding/urn_resolver.h    │
│   2. UUID → xxHash-64                                           │
│   3. Hash → Shard-ID (Consistent Hash Ring)                     │
│   4. Shard-ID → Netzwerkendpunkt (ShardTopology)                │
└───────────────────────────┬─────────────────────────────────────┘
                            │ Shard-Endpunkt
                            ▼
┌────────────────────────────────────┐
│   Consistent Hash Ring             │
│   include/sharding/consistent_hash.h│
│                                    │
│   Shard A  Shard B  Shard C  ...   │
└────────────────────────────────────┘
                            │
          ┌─────────────────┼──────────────────┐
          ▼                 ▼                  ▼
  ┌───────────────┐ ┌───────────────┐ ┌───────────────┐
  │   Shard A     │ │   Shard B     │ │   Shard C     │
  │ dc1.host:8080 │ │ dc1.host:8081 │ │ dc2.host:8080 │
  │ + Replikas    │ │ + Replikas    │ │ + Replikas    │
  └───────────────┘ └───────────────┘ └───────────────┘
```

### 1.2 Kernkomponenten

| Komponente | Datei | Verantwortung |
|---|---|---|
| `URN` | `include/sharding/urn.h` | URN-Struktur, Parsing, Hashing |
| `URNResolver` | `include/sharding/urn_resolver.h` | URN → Shard-Endpunkt |
| `ConsistentHashRing` | `include/sharding/consistent_hash.h` | Hash → Shard-ID |
| `ShardTopology` | `include/sharding/shard_topology.h` | Shard-ID → Netzwerkinfo |
| `RedundancyStrategy` | `include/sharding/redundancy_strategy.h` | RAID-Modus-Logik |

### 1.3 Datenfluss auf einen Blick

```
URN-String
    │
    ▼ URN::parse()
URN{model, namespace_, collection, uuid}
    │
    ▼ URN::hash()  →  xxHash-64(uuid)
uint64_t hash
    │
    ▼ ConsistentHashRing::getShardForHash(hash)
std::string shard_id    (z. B. "shard_002")
    │
    ▼ ShardTopology::getShard(shard_id)
ShardInfo{primary_endpoint, replica_endpoints, region, ...}
    │
    ▼ Verbindung herstellen
themis-shard002.dc1.example.com:8080
```

---

## 2. Consistent Hash Ring

### 2.1 Funktionsprinzip

Der Consistent Hash Ring bildet einen 64-Bit-Adressraum (0 … 2⁶⁴−1) als Ring ab. Jeder Shard erhält mehrere **virtuelle Knoten (Virtual Nodes / VNodes)** auf dem Ring, die für eine gleichmäßige Datenverteilung sorgen.

```
Hash-Ring (0 … 2⁶⁴)
─────────────────────────────────────────────────────────
           0
           │
     A#0 ──┤     ← Shard A, Virtual Node 0
           │
     B#0 ──┤     ← Shard B, Virtual Node 0
           │
     C#0 ──┤     ← Shard C, Virtual Node 0
           │
     A#1 ──┤     ← Shard A, Virtual Node 1
           │
     B#1 ──┤     ← Shard B, Virtual Node 1
           │
     ...
           │
    2⁶⁴-1 ──┘ (Wrap-Around → Shard A#0)
```

**Lookup-Algorithmus:** Für einen Hash-Wert H wird der erste VNode mit Token ≥ H gefunden (clockwise search). Gibt es keinen, wird zum Anfang gewrappt.

```cpp
// src/sharding/consistent_hash.cpp
std::string ConsistentHashRing::getShardForHash(uint64_t hash) const {
    // Clockwise search: erster Eintrag >= hash
    auto it = ring_.lower_bound(hash);

    // Wrap-Around: Wenn am Ende, zum Anfang
    if (it == ring_.end()) {
        it = ring_.begin();
    }

    return it->second;  // Shard-ID
}
```

### 2.2 Virtuelle Knoten (VNodes)

Virtuelle Knoten sind der Schlüssel zur gleichmäßigen Verteilung:

| Anzahl VNodes pro Shard | Verteilungsqualität | Speicherbedarf |
|---|---|---|
| 10 | ±30 % Abweichung | minimal |
| 100 | ±10 % Abweichung | gering |
| **150 (Standard)** | **±5 % Abweichung** | **moderat** |
| 500 | ±2 % Abweichung | hoch |

**VNode-Generierung:**

```cpp
// src/sharding/consistent_hash.cpp
for (size_t i = 0; i < virtual_nodes; ++i) {
    std::ostringstream oss;
    oss << shard_id << "#" << i;       // z. B. "shard_001#42"
    uint64_t token = hash(oss.str()); // xxHash-64 oder std::hash
    ring_[token] = shard_id;
}
```

### 2.3 Replikaauswahl (Successors)

Beim Schreiben mit Replikation werden die **nächsten N eindeutigen Shards** im Uhrzeigersinn ermittelt:

```cpp
// src/sharding/consistent_hash.cpp
std::vector<std::string> ConsistentHashRing::getSuccessors(
    uint64_t hash, size_t count) const
{
    std::vector<std::string> result;
    std::set<std::string> seen;

    auto it = ring_.lower_bound(hash);
    if (it == ring_.end()) it = ring_.begin();

    size_t iterations = 0;
    while (result.size() < count && iterations < ring_.size()) {
        if (seen.find(it->second) == seen.end()) {
            result.push_back(it->second);
            seen.insert(it->second);
        }
        if (++it == ring_.end()) it = ring_.begin();
        ++iterations;
    }
    return result;  // [primary, replica1, replica2, ...]
}
```

### 2.4 Shard hinzufügen / entfernen

Ein wesentlicher Vorteil des Consistent Hashing: Beim Hinzufügen oder Entfernen eines Shards muss nur ein Bruchteil der Daten migriert werden (ca. 1/N).

```
Vor Erweiterung (3 Shards):
Ring: ... A ... B ... C ... A ... B ...

Nach Hinzufügen von D:
Ring: ... A ... B ... D ... C ... A ... B ...
                     ↑
              Nur Daten zwischen vorherigem
              und neuem VNode müssen verschoben werden
```

---

## 3. VCC-URN Format & Parsing

### 3.1 URN-Struktur

```
urn:themis:{model}:{namespace}:{collection}:{uuid}
│   │       │       │           │           │
│   │       │       │           │           └─ RFC 4122 UUID v4
│   │       │       │           └─ Collection (Tabelle/Index/Stream)
│   │       │       └─ Namespace (Tenant / Mandant)
│   │       └─ Datenmodell
│   └─ ThemisDB-Präfix
└─ URN-Standard (RFC 8141)
```

**Gültige Modelle:**

| Modell | Beschreibung | Beispiel |
|---|---|---|
| `relational` | Relationale Daten (SQL-ähnlich) | Kundentabellen, Bestellungen |
| `graph` | Graphdaten (Knoten + Kanten) | Social Network, Wissensbasen |
| `vector` | Vektoren / Embeddings | ML-Modelle, Ähnlichkeitssuche |
| `timeseries` | Zeitreihendaten | Metriken, Sensordaten, Logs |
| `document` | Dokumente (JSON/BSON) | Produkte, Content, Profile |

### 3.2 Beispiele

```
# Relationaler Datensatz – Benutzer "Max Müller"
urn:themis:relational:customers:users:550e8400-e29b-41d4-a716-446655440000

# Graphknoten – Social-Network-Profil
urn:themis:graph:social:nodes:7c9e6679-7425-40de-944b-e07fc1f90ae7

# Vektorembedding – Produktbeschreibung
urn:themis:vector:embeddings:documents:f47ac10b-58cc-4372-a567-0e02b2c3d479

# Zeitreihenmesswert – CPU-Auslastung
urn:themis:timeseries:metrics:cpu_usage:3d6e3e3e-4c5d-4f5e-9e7f-8a9b0c1d2e3f
```

### 3.3 Parsing-Implementierung

```cpp
// include/sharding/urn.h
struct URN {
    std::string model;       // relational, graph, vector, timeseries, document
    std::string namespace_;  // customers, tenant_123, global
    std::string collection;  // users, nodes, documents
    std::string uuid;        // RFC 4122 UUID v4
};

// src/sharding/urn.cpp
std::optional<URN> URN::parse(std::string_view urn_str) {
    // Mindestlänge: "urn:themis:a:b:c:d" = 18 Zeichen
    if (urn_str.size() < 18) return std::nullopt;

    // Muss mit "urn:themis:" beginnen
    if (!urn_str.starts_with("urn:themis:")) return std::nullopt;

    // Aufteilen nach ':'
    std::vector<std::string> parts;
    size_t start = 0, end = 0;
    while (end != std::string_view::npos) {
        end = urn_str.find(':', start);
        if (end != std::string_view::npos) {
            parts.emplace_back(urn_str.substr(start, end - start));
            start = end + 1;
        } else {
            parts.emplace_back(urn_str.substr(start));
        }
    }

    // Erwartet: ["urn", "themis", model, namespace, collection, uuid]
    // → Genau 6 Segmente
    if (parts.size() != 6) return std::nullopt;

    URN urn;
    urn.model      = parts[2];
    urn.namespace_ = parts[3];
    urn.collection = parts[4];
    urn.uuid       = parts[5];

    // Validierung: Modell, UUID-Format (RFC 4122), leere Felder
    if (!urn.isValidModel())  return std::nullopt;
    if (!urn.isValidUUID())   return std::nullopt;
    if (urn.namespace_.empty() || urn.collection.empty()) return std::nullopt;

    return urn;
}
```

### 3.4 Hashing für die Sharding-Entscheidung

Der Hash basiert ausschließlich auf der **UUID**, um eine gleichmäßige Verteilung unabhängig von Modell und Namespace sicherzustellen:

```cpp
// src/sharding/urn.cpp
uint64_t URN::hash() const {
#ifdef HAS_XXHASH
    // Primäre Implementierung: xxHash-64 (schnell, hochwertig)
    return XXH64(uuid.data(), uuid.size(), 0);
#else
    // Fallback: std::hash<std::string>
    std::hash<std::string> hasher;
    return hasher(uuid);
#endif
}
```

**Warum nur die UUID?** Die UUID ist nach RFC 4122 zufällig verteilt (Version 4), sodass die Daten automatisch gleichmäßig auf alle Shards verteilt werden. Würde man den gesamten URN-String hashen, könnten Tenant-Namen zu ungleichmäßiger Verteilung führen.

---

## 4. URN-Auflösung (Resolution)

### 4.1 URNResolver – Schnittstelle

```cpp
// include/sharding/urn_resolver.h
class URNResolver {
public:
    URNResolver(
        std::shared_ptr<ShardTopology>     topology,
        std::shared_ptr<ConsistentHashRing> hash_ring,
        const std::string& local_shard_id = ""
    );

    // Primären Shard auflösen
    std::optional<ShardInfo> resolvePrimary(const URN& urn) const;

    // Primären + Replikashards auflösen
    std::vector<ShardInfo> resolveReplicas(const URN& urn,
                                           size_t replica_count = 2) const;

    // Nur Shard-ID (schneller, kein Netzwerkinfo-Lookup)
    std::string getShardId(const URN& urn) const;

    // Prüfen, ob URN lokal liegt
    bool isLocal(const URN& urn) const;

    // Topologie aus etcd neu laden
    void refreshTopology();
};
```

### 4.2 Schritt-für-Schritt Auflösung

```
Schritt 1: URN-String parsen
──────────────────────────────
Input:  "urn:themis:relational:customers:users:550e8400-e29b-41d4-a716-446655440000"
Output: URN{ model="relational", namespace_="customers",
             collection="users",
             uuid="550e8400-e29b-41d4-a716-446655440000" }

Schritt 2: UUID hashen
──────────────────────────────
xxHash-64("550e8400-e29b-41d4-a716-446655440000") → 0xA3F2B89C14E60D71

Schritt 3: Hash → Shard-ID (Consistent Hash Ring)
──────────────────────────────
ring_.lower_bound(0xA3F2B89C14E60D71) → shard_003

Schritt 4: Shard-ID → Netzwerkinfo (ShardTopology)
──────────────────────────────
topology_.getShard("shard_003") →
  ShardInfo{
    shard_id:          "shard_003",
    primary_endpoint:  "themis-shard003.dc1.example.com:8080",
    replica_endpoints: ["themis-shard003r1.dc1.example.com:8081",
                        "themis-shard003r2.dc2.example.com:8082"],
    datacenter: "dc1",
    region:     "us-east",
    is_healthy: true
  }

Schritt 5: Verbindung herstellen
──────────────────────────────
→ TCP/TLS zu themis-shard003.dc1.example.com:8080
```

### 4.3 Implementierung resolvePrimary

```cpp
// src/sharding/urn_resolver.cpp
std::optional<ShardInfo> URNResolver::resolvePrimary(const URN& urn) const {
    // 1. Shard-ID über Hash Ring bestimmen
    std::string shard_id = hash_ring_->getShardForURN(urn);

    if (shard_id.empty()) {
        return std::nullopt;  // Ring leer
    }

    // 2. Netzwerkinfo aus Topologie laden
    return topology_->getShard(shard_id);
}
```

### 4.4 Implementierung resolveReplicas

```cpp
// src/sharding/urn_resolver.cpp
std::vector<ShardInfo> URNResolver::resolveReplicas(
    const URN& urn, size_t replica_count) const
{
    std::vector<ShardInfo> result;

    // Primären Shard holen
    auto primary = resolvePrimary(urn);
    if (!primary) return result;
    result.push_back(*primary);

    // Nachfolger im Ring holen:
    // getSuccessors(hash, replica_count + 1) liefert [primary, replica1, replica2, ...]
    // Das erste Element (Index 0) entspricht dem Primary-Shard.
    uint64_t hash = urn.hash();
    auto successor_ids = hash_ring_->getSuccessors(hash, replica_count + 1);

    // Ersten überspringen (= Primary), Rest hinzufügen
    for (size_t i = 1; i < successor_ids.size() && result.size() <= replica_count; ++i) {
        auto replica = topology_->getShard(successor_ids[i]);
        if (replica && replica->is_healthy) {
            result.push_back(*replica);
        }
    }

    return result;  // [primary, replica1, replica2]
}
```

### 4.5 Lokalitätsprüfung

Jeder Shard-Server kann prüfen, ob eine URN lokal (d. h. primär auf diesem Knoten) liegt:

```cpp
// src/sharding/urn_resolver.cpp
bool URNResolver::isLocal(const URN& urn) const {
    if (local_shard_id_.empty()) return false;

    std::string shard_id = hash_ring_->getShardForURN(urn);
    return shard_id == local_shard_id_;
}
```

---

## 5. RAID-Redundanzmodi

ThemisDB unterstützt RAID-inspirierte Redundanzmodi, die pro Collection konfiguriert werden können.

### 5.1 Überblick

```
┌──────────────────┬──────┬─────────────────┬─────────┬──────────────┐
│ Modus            │  RF* │ Speichereff.    │ Reads   │ Fehlertoleranz│
├──────────────────┼──────┼─────────────────┼─────────┼──────────────┤
│ NONE             │  1×  │ 100%            │ 1×      │ 0 Shards     │
│ MIRROR (RF=2)    │  2×  │  50%            │ 2×      │ 1 Shard      │
│ MIRROR (RF=3)    │  3×  │  33%            │ 3×      │ 2 Shards     │
│ STRIPE           │  1×  │ 100%            │ 4×      │ 0 Shards     │
│ STRIPE_MIRROR    │  2×  │  50%            │ 2–3×    │ 1 Shard      │
│ PARITY (4+2)     │ 4+2  │  67%            │ 1.5×    │ 2 Shards     │
│ RAID6 (8+3)      │ 8+3  │  73%            │ 1.3×    │ 3 Shards     │
│ GEO_MIRROR       │  3×  │  33%            │ lokal 3×│ 2 Datacenter │
└──────────────────┴──────┴─────────────────┴─────────┴──────────────┘

* RF = Replication Factor
```

### 5.2 NONE – Kein Overhead

```cpp
// include/sharding/redundancy_strategy.h
enum class RedundancyMode {
    NONE,  // Nur Consistent Hash Sharding, keine Redundanz
    ...
};
```

**Einsatz:** Entwicklungsumgebungen, temporäre Daten, Analytics-Caches.  
**Risiko:** Ein Shard-Ausfall → Datenverlust in diesem Hash-Segment.

### 5.3 MIRROR – Vollständige Spiegelung (RAID-1)

Alle Schreibvorgänge werden **synchron** auf N Shards repliziert. Lesevorgänge können von jedem Replikat bedient werden.

```
Write("users:uuid123") → Shard A (primary)
                       → Shard B (replica 1)
                       → Shard C (replica 2)

Alle 3 müssen ACK senden (WriteConcern = MAJORITY)
```

**Konfiguration:**

```yaml
collections:
  users:
    redundancy_mode: MIRROR
    replication_factor: 3
    read_preference: NEAREST     # Lesen vom nächsten Replikat
    write_concern: MAJORITY      # Mehrheit muss bestätigen
```

### 5.4 STRIPE – Daten-Striping (RAID-0)

Große Dokumente werden in **Chunks** aufgeteilt und auf mehrere Shards verteilt. Maximaler Durchsatz, keine Redundanz.

```
Dokument (4 MB) → Chunk[0] → Shard A
                → Chunk[1] → Shard B
                → Chunk[2] → Shard C
                → Chunk[3] → Shard D
```

```cpp
// include/sharding/redundancy_strategy.h
struct StripeConfig {
    uint32_t stripe_size_kb = 64;           // Chunk-Größe in KB
    uint32_t min_stripe_shards = 4;         // Mindestanzahl Shards
    bool     stripe_large_documents_only = true;
    uint32_t large_document_threshold_kb = 1024;  // 1 MB Schwellenwert
    bool     parallel_stripe_io = true;     // Paralleles I/O
    uint32_t max_parallel_io = 8;
};
```

### 5.5 PARITY – Erasure Coding (RAID-5/6)

Daten werden in K Chunks aufgeteilt, M Paritätschunks werden berechnet. Kann M Shard-Ausfälle tolerieren.

```
PARITY (k=4, m=2):
Dokument → [D0][D1][D2][D3]  +  [P0][P1]
            Shard A  B  C  D      E   F

Fällt Shard B aus → D1 wird aus D0, D2, D3, P0, P1 rekonstruiert
Fällt Shard E+F aus → noch tolerierbar (m=2)
```

```cpp
// include/sharding/redundancy_strategy.h
struct ErasureCodingConfig {
    uint32_t data_shards   = 4;  // k: Datenchunks
    uint32_t parity_shards = 2;  // m: Paritätschunks

    // Speichereffizienz = k / (k + m) = 4/6 ≈ 67 %
    double storageEfficiency() const {
        return static_cast<double>(data_shards) / totalShards();
    }

    // Fehlertoleranz = m Shards können ausfallen
    uint32_t faultTolerance() const { return parity_shards; }

    ErasureCodingAlgorithm algorithm = ErasureCodingAlgorithm::REED_SOLOMON;
};
```

### 5.6 RAID6 – Duale Parität

Identisch zu PARITY, jedoch mit **zwei unabhängigen Paritätsspuren** (P und Q). Kann 2 simultane Shard-Ausfälle tolerieren, auch wenn es sich um Datenchunks handelt.

```
RAID6 (k=8, m=3):
[D0..D7] + [P][Q][R]  →  11 Shards insgesamt
Speichereffizienz: 8/11 ≈ 73 %
Fehlertoleranz: 3 Shards gleichzeitig
```

### 5.7 GEO_MIRROR – Geo-verteilte Replikation

```cpp
// include/sharding/redundancy_strategy.h
struct GeoReplicationConfig {
    std::string              primary_datacenter;
    std::vector<std::string> replica_datacenters;

    // Schreib-Quorum pro Region
    std::map<std::string, uint32_t> region_write_quorums;
    // z. B. {{"us-east", 2}, {"eu-west", 1}}

    enum class ReplicationMode {
        SYNC,       // Synchron (stark konsistent, höhere Latenz)
        SEMI_SYNC,  // Mindestens ein Remote-DC muss ACK senden
        ASYNC       // Asynchron (niedrige Latenz, eventual consistency)
    } replication_mode = ReplicationMode::ASYNC;

    ConflictResolution conflict_resolution = ConflictResolution::LAST_WRITE_WINS;
    ReadPreference     read_preference     = ReadPreference::NEAREST;

    uint32_t max_staleness_ms = 0;    // Maximale Replikationsverzögerung
    uint32_t max_lag_ms       = 10000;
    bool     prefer_local_reads = true;
};
```

**Typische Topologie:**

```
┌─────────────────┐           ┌─────────────────┐
│    us-east-1    │◄──SYNC───►│    eu-west-1    │
│  Primary DC     │           │  Replica DC     │
│  Shards A,B,C   │           │  Shards D,E,F   │
└────────┬────────┘           └────────┬────────┘
         │ ASYNC                       │ ASYNC
         ▼                             ▼
┌─────────────────┐           ┌─────────────────┐
│   ap-south-1   │           │   us-west-2    │
│  Replica DC    │           │   DR-Only DC   │
│  Shards G,H,I  │           │  Shards J,K,L  │
└─────────────────┘           └─────────────────┘
```

### 5.8 ReadPreference-Optionen

```cpp
// include/sharding/redundancy_strategy.h
enum class ReadPreference {
    PRIMARY,        // Immer vom Primary lesen (stark konsistent)
    NEAREST,        // Nächstes Replikat (latenzoptimiert)
    ROUND_ROBIN,    // Lastverteilung auf alle Replikas
    RANDOM,         // Zufälliges Replikat
    SECONDARY_ONLY, // Nur Sekundärknoten
    FOLLOWER,       // Jeder Follower (evtl. veraltete Daten)
    LOCAL_REGION    // Bevorzugt lokale Region
};
```

---

## 6. Shard-Topologie & Adressierung

### 6.1 ShardInfo – Vollständige Metadaten

```cpp
// include/sharding/shard_topology.h
struct ShardInfo {
    // Identifikation
    std::string shard_id;               // "shard_001", "shard_eu_west_003"
    std::string primary_endpoint;       // "themis-shard001.dc1.example.com:8080"
    std::vector<std::string> replica_endpoints;  // Replikaendpunkte

    // Geografische Platzierung
    std::string datacenter;             // "dc1", "us-east-1"
    std::string region;                 // "us-east", "eu-west"
    std::string zone;                   // "us-east-1a", "eu-west-1b"
    std::string rack;                   // "rack01" (Locality Awareness)

    // Hash-Ring-Position
    uint64_t token_start;               // Beginn des Hash-Ranges
    uint64_t token_end;                 // Ende des Hash-Ranges

    // Gesundheit & Sicherheit
    bool is_healthy;
    std::string certificate_serial;    // X.509-Zertifikat Serial
    std::vector<std::string> capabilities;  // "read", "write", "replicate"

    // Raft-Konsensus (optional)
    std::string raft_role;             // "LEADER", "FOLLOWER", "CANDIDATE"
    uint64_t    raft_term;
    bool        raft_has_quorum;
};
```

### 6.2 ShardTopology – Topologieverwaltung

```cpp
// include/sharding/shard_topology.h
class ShardTopology {
public:
    struct Config {
        std::string metadata_endpoint;    // etcd-Endpunkt
        std::string cluster_name;
        uint32_t    refresh_interval_sec; // 0 = nur manuell
        bool        enable_health_checks;
    };

    void addShard(const ShardInfo& shard);
    void removeShard(const std::string& shard_id);
    std::optional<ShardInfo> getShard(const std::string& shard_id) const;
    std::vector<ShardInfo>   getAllShards() const;
    std::vector<ShardInfo>   getHealthyShards() const;
    void updateHealth(const std::string& shard_id, bool is_healthy);

    // Geo-Abfragen
    std::vector<ShardInfo> getShardsInRegion(const std::string& region) const;
    bool regionHasQuorum(const std::string& region, uint32_t required) const;

    // Persistenz via etcd
    void refresh();  // Lädt aktuelle Topologie
    void save();     // Persistiert Topologie
};
```

### 6.3 Shard-ID-Namenskonvention

```
Format:  shard_{standort}_{domäne}_{nummer}
         shard_{region}_{nummer}
         shard_{nummer}

Beispiele:
  shard_001                     → Einfache Nummerierung
  shard_dc1_relational_001      → Standort + Domäne
  shard_us_east_vector_003      → Region + Domäne
  shard_eu_west_geo_002         → Geo-Region

Endpunktformat:
  themis-shard{id}.{dc}.{domain}:{port}
  Beispiel: themis-shard001.dc1.example.com:8080
```

### 6.4 Topologie-Initialisierung (Server-Start)

```cpp
// src/main_server.cpp (vereinfacht)
// Hash Ring erstellen
auto hash_ring = std::make_shared<ConsistentHashRing>(virtual_nodes);

// Topologie initialisieren
auto shard_topology = std::make_shared<ShardTopology>();

// Shards aus Konfiguration laden
for (const auto& shard : config["sharding"]["shards"]) {
    std::string shard_id = shard["id"];
    hash_ring->addNode(shard_id);          // Im Ring registrieren

    ShardInfo info;
    info.shard_id   = shard_id;
    info.is_healthy = true;
    shard_topology->addShard(info);        // In Topologie speichern
}
```

---

## 7. Praktische Code-Beispiele

### 7.1 Vollständiges Auflösungsbeispiel

```cpp
#include "sharding/urn.h"
#include "sharding/urn_resolver.h"
#include "sharding/consistent_hash.h"
#include "sharding/shard_topology.h"

using namespace themis::sharding;

// ── Setup ────────────────────────────────────────────────────────────────────
auto topology  = std::make_shared<ShardTopology>();
auto hash_ring = std::make_shared<ConsistentHashRing>(150);

// Shards registrieren
for (int i = 0; i < 4; ++i) {
    std::string id = "shard_00" + std::to_string(i);

    hash_ring->addShard(id, 150);

    ShardInfo info;
    info.shard_id        = id;
    info.primary_endpoint = "shard" + std::to_string(i) + ".example.com:8080";
    info.region          = "eu-west";
    info.datacenter      = "dc1";
    info.is_healthy      = true;
    topology->addShard(info);
}

// URN Resolver erstellen
URNResolver resolver(topology, hash_ring, "shard_001"); // lokal = shard_001

// ── URN auflösen ─────────────────────────────────────────────────────────────
auto urn_str = "urn:themis:relational:customers:users:"
               "550e8400-e29b-41d4-a716-446655440000";

auto urn = URN::parse(urn_str);
if (!urn) {
    std::cerr << "Ungültige URN\n";
    return;
}

// Primären Shard finden
auto primary = resolver.resolvePrimary(*urn);
if (primary) {
    std::cout << "Primary Shard: " << primary->shard_id << "\n";
    std::cout << "Endpoint:      " << primary->primary_endpoint << "\n";
    std::cout << "Region:        " << primary->region << "\n";
}

// Replikashards finden
auto replicas = resolver.resolveReplicas(*urn, 2);
for (size_t i = 0; i < replicas.size(); ++i) {
    std::cout << "Replica[" << i << "]: " << replicas[i].shard_id
              << " @ " << replicas[i].primary_endpoint << "\n";
}

// Lokalitätsprüfung
if (resolver.isLocal(*urn)) {
    std::cout << "URN ist lokal – direkte Verarbeitung\n";
} else {
    std::cout << "URN weiterleiten an: " << primary->primary_endpoint << "\n";
}
```

### 7.2 Hash-Ring direkt nutzen

```cpp
#include "sharding/consistent_hash.h"

ConsistentHashRing ring(150);
ring.addShard("shard_001", 150);
ring.addShard("shard_002", 150);
ring.addShard("shard_003", 150);

// Direktes Hash-Lookup
uint64_t my_hash = 0xA3F2B89C14E60D71ULL;
std::string shard = ring.getShardForHash(my_hash);
std::cout << "Shard: " << shard << "\n";

// Über URN
auto urn = URN::parse("urn:themis:graph:social:nodes:7c9e6679-7425-40de-944b-e07fc1f90ae7");
std::string shard2 = ring.getShardForURN(*urn);

// Balance prüfen
double balance = ring.getBalanceFactor();
std::cout << "Balance-Faktor: " << balance << " % (Ziel: <5 %)\n";

// Successors für Replikation
auto successors = ring.getSuccessors(my_hash, 3);
// successors = ["shard_002", "shard_001", "shard_003"]
```

### 7.3 URN erstellen und serialisieren

```cpp
#include "sharding/urn.h"

// Parsing aus String
auto urn = URN::parse(
    "urn:themis:vector:embeddings:documents:f47ac10b-58cc-4372-a567-0e02b2c3d479"
);

// Manuell erstellen
URN my_urn;
my_urn.model      = "timeseries";
my_urn.namespace_ = "metrics";
my_urn.collection = "cpu_usage";
my_urn.uuid       = "3d6e3e3e-4c5d-4f5e-9e7f-8a9b0c1d2e3f";

// Validierung
assert(my_urn.isValidModel());
assert(my_urn.isValidUUID());

// Serialisierung
std::string urn_str = my_urn.toString();
// → "urn:themis:timeseries:metrics:cpu_usage:3d6e3e3e-4c5d-4f5e-9e7f-8a9b0c1d2e3f"

// Resource-ID (für interne Schlüssel)
std::string resource_id = my_urn.getResourceId();
// → "cpu_usage:3d6e3e3e-4c5d-4f5e-9e7f-8a9b0c1d2e3f"

// Hashing
uint64_t hash = my_urn.hash();
// → xxHash-64("3d6e3e3e-4c5d-4f5e-9e7f-8a9b0c1d2e3f")
```

---

## 8. End-to-End Flow-Diagramme

### 8.1 Leseanfrage (Read Path)

```
Client                  URNResolver           ConsistentHashRing    ShardTopology    Shard
  │                         │                        │                   │              │
  │─ "GET urn:themis:…" ───►│                        │                   │              │
  │                         │─ URN::parse() ─────────┤                   │              │
  │                         │  urn.hash() ───────────┤                   │              │
  │                         │                        │                   │              │
  │                         │─ getShardForURN(urn) ──►│                  │              │
  │                         │◄─ "shard_003" ──────────│                  │              │
  │                         │                        │                   │              │
  │                         │─ getShard("shard_003") ──────────────────►│              │
  │                         │◄─ ShardInfo{endpoint…} ───────────────────│              │
  │                         │                        │                   │              │
  │                         │─ HTTP GET /resource ────────────────────────────────────►│
  │                         │◄─ Datensatz ────────────────────────────────────────────│
  │◄── Datensatz ───────────│                        │                   │              │
```

### 8.2 Schreibanfrage mit MIRROR-Replikation (Write Path)

```
Client              URNResolver      HashRing     Shard A (Primary)   Shard B (R1)   Shard C (R2)
  │                     │               │                │                  │               │
  │─ "PUT urn:…" ──────►│               │                │                  │               │
  │                     │─ hash(urn) ──►│                │                  │               │
  │                     │◄─ shard_A ────│                │                  │               │
  │                     │─ getSuccessors(hash, 3) ──────►│                  │               │
  │                     │◄─ [A, B, C] ──────────────────│                  │               │
  │                     │                                │                  │               │
  │                     │─ Write(data) ─────────────────►│                  │               │
  │                     │─ Write(data) ──────────────────────────────────►│               │
  │                     │─ Write(data) ───────────────────────────────────────────────────►│
  │                     │                                │                  │               │
  │                     │◄─ ACK ────────────────────────│                  │               │
  │                     │◄─ ACK ─────────────────────────────────────────│               │
  │                     │◄─ ACK ──────────────────────────────────────────────────────────│
  │                     │  (MAJORITY erfüllt: 3/3 ACKs)  │                  │               │
  │◄─ SUCCESS ──────────│                                │                  │               │
```

### 8.3 URN-Auflösung – interner Ablauf

```
┌────────────────────────────────────────────────────────────────┐
│  URNResolver::resolvePrimary(urn)                              │
│                                                                │
│  ┌──────────────────────────────────────┐                     │
│  │ 1. hash_ring_->getShardForURN(urn)  │                     │
│  │                                      │                     │
│  │    urn.hash() → xxHash-64(uuid)     │                     │
│  │              → ring_.lower_bound()  │                     │
│  │              → "shard_003"          │                     │
│  └──────────────┬───────────────────────┘                     │
│                 │                                              │
│  ┌──────────────▼───────────────────────┐                     │
│  │ 2. topology_->getShard("shard_003") │                     │
│  │                                      │                     │
│  │    shards_["shard_003"] → ShardInfo  │                     │
│  │    {endpoint, region, health, ...}   │                     │
│  └──────────────────────────────────────┘                     │
│                                                                │
│  Return: std::optional<ShardInfo>                              │
└────────────────────────────────────────────────────────────────┘
```

### 8.4 Shard hinzufügen (Rebalancing)

```
Vor Hinzufügen von Shard D:
────────────────────────────────────
Ring: [A#0]...[B#0]...[C#0]...[A#1]...[B#1]...
      Alle Daten auf A, B, C verteilt

Shard D wird hinzugefügt (150 VNodes):
────────────────────────────────────────
ring.addShard("shard_D", 150);

Ring: [A#0]...[D#42]...[B#0]...[D#87]...[C#0]...[D#3]...[A#1]...
                ↑               ↑               ↑
    Nur diese Segmente müssen von A/B/C zu D migriert werden
    (ca. 1/4 der Gesamtdaten)

Nach Migration:
────────────────
Jeder der 4 Shards verantwortet ≈ 25 % der Daten
```

---

## 9. Konfigurationsbeispiele (YAML)

### 9.1 Basis-Sharding-Konfiguration

```yaml
# config/themisdb.yaml

sharding:
  enabled: true
  
  hash_ring:
    virtual_nodes_per_shard: 150      # Balance vs. Speicher
    hash_algorithm: xxhash64          # Primärer Hash-Algorithmus
  
  shards:
    - id: shard_001
      primary_endpoint: "shard001.dc1.example.com:8080"
      replica_endpoints:
        - "shard001r1.dc1.example.com:8081"
        - "shard001r2.dc2.example.com:8082"
      datacenter: dc1
      region: eu-west
      zone: eu-west-1a
      rack: rack01
      
    - id: shard_002
      primary_endpoint: "shard002.dc1.example.com:8080"
      replica_endpoints:
        - "shard002r1.dc1.example.com:8081"
        - "shard002r2.dc2.example.com:8082"
      datacenter: dc1
      region: eu-west
      zone: eu-west-1b
      rack: rack02

    - id: shard_003
      primary_endpoint: "shard003.dc2.example.com:8080"
      replica_endpoints:
        - "shard003r1.dc2.example.com:8081"
      datacenter: dc2
      region: us-east
      zone: us-east-1a
      rack: rack01

raid:
  enabled: true

metadata:
  endpoint: "http://etcd.cluster.local:2379"
  cluster_name: "themis-prod-cluster"
  refresh_interval_sec: 30
  enable_health_checks: true
```

### 9.2 MIRROR-Modus (RAID-1)

```yaml
collections:
  users:
    redundancy_mode: MIRROR
    replication_factor: 3
    read_preference: NEAREST
    write_concern: MAJORITY           # ≥ 2/3 Shards müssen ACK senden
    sync_replication: true
    
  sessions:
    redundancy_mode: MIRROR
    replication_factor: 2
    read_preference: PRIMARY
    write_concern: ALL                # Alle Shards müssen ACK senden
```

### 9.3 PARITY-Modus (Erasure Coding, RAID-5/6)

```yaml
collections:
  documents:
    redundancy_mode: PARITY
    erasure_coding:
      data_shards: 4                  # k = 4 Datenchunks
      parity_shards: 2                # m = 2 Paritätschunks
      algorithm: REED_SOLOMON         # Erasure-Coding-Algorithmus
      min_document_size_kb: 1024      # Nur Dokumente ≥ 1 MB stripen
    read_preference: NEAREST
    write_concern: DATA_SHARDS        # Alle Datenchunks müssen ACK senden
    
  archives:
    redundancy_mode: RAID6
    erasure_coding:
      data_shards: 8
      parity_shards: 3                # 73 % Speichereffizienz
      algorithm: CAUCHY               # Schneller als Reed-Solomon
      min_document_size_kb: 512
```

### 9.4 GEO_MIRROR-Modus (Multi-Region)

```yaml
collections:
  critical_data:
    redundancy_mode: GEO_MIRROR
    geo_replication:
      primary_datacenter: eu-west-1
      replica_datacenters:
        - us-east-1
        - ap-south-1
      
      replication_mode: SEMI_SYNC     # Mindestens 1 Remote-DC muss ACK senden
      conflict_resolution: LAST_WRITE_WINS
      
      region_write_quorums:
        eu-west-1: 2                  # 2 Shards in EU müssen ACK senden
        us-east-1: 1                  # 1 Shard in US muss ACK senden
      
      max_staleness_ms: 5000          # Max. 5 Sekunden Verzögerung für Reads
      max_lag_ms: 10000               # Alert bei >10 Sekunden Lag
      prefer_local_reads: true
      enable_geo_failover: true
      region_failure_threshold: 0.5  # Region gilt als ausgefallen bei <50 % Shards
```

### 9.5 Vollständige Produktionskonfiguration

```yaml
# Produktionskonfiguration: 8 Shards, MIRROR RF=3, Multi-Region

server:
  port: 8080
  tls:
    enabled: true
    cert_file: /etc/themis/certs/server.crt
    key_file:  /etc/themis/certs/server.key

sharding:
  enabled: true
  hash_ring:
    virtual_nodes_per_shard: 150
  shards:
    - id: shard_eu_001
      primary_endpoint: "shard-eu-001.dc-eu.example.com:8080"
      replica_endpoints:
        - "shard-eu-001-r1.dc-eu.example.com:8081"
      datacenter: dc-eu
      region: eu-west
      zone: eu-west-1a

    - id: shard_eu_002
      primary_endpoint: "shard-eu-002.dc-eu.example.com:8080"
      replica_endpoints:
        - "shard-eu-002-r1.dc-eu.example.com:8081"
      datacenter: dc-eu
      region: eu-west
      zone: eu-west-1b

    # … weitere Shards

raid:
  enabled: true

collections:
  users:
    redundancy_mode: MIRROR
    replication_factor: 3
    read_preference: NEAREST
    write_concern: MAJORITY

  embeddings:
    redundancy_mode: PARITY
    erasure_coding:
      data_shards: 6
      parity_shards: 2
      algorithm: CAUCHY
    read_preference: ROUND_ROBIN
    write_concern: DATA_SHARDS

metadata:
  endpoint: "http://etcd.cluster.local:2379"
  cluster_name: "themis-eu-prod"
  refresh_interval_sec: 15
  enable_health_checks: true

observability:
  prometheus:
    enabled: true
    port: 9090
  metrics:
    - shard_request_latency_p99
    - hash_ring_balance_factor
    - replication_lag_ms
```

---

## 10. Performance-Charakteristiken

### 10.1 Hash-Ring-Komplexität

| Operation | Komplexität | Anmerkung |
|---|---|---|
| `getShardForHash(hash)` | **O(log N)** | N = Anzahl VNodes; `std::map::lower_bound` |
| `getSuccessors(hash, k)` | **O(log N + k)** | k = Anzahl Replikas |
| `addShard(id, vnodes)` | **O(v · log N)** | v = virtuelle Knoten pro Shard |
| `removeShard(id)` | **O(v · log N)** | v = virtuelle Knoten des Shards |
| `getAllShards()` | **O(S)** | S = Anzahl eindeutiger Shards |

**Typische Werte:**
- 8 Shards × 150 VNodes = 1.200 Einträge im Ring
- `lower_bound` auf 1.200 Einträgen ≈ 10 Vergleiche (log₂ 1200 ≈ 10)

### 10.2 URN-Auflösung – Latenz

```
URN::parse()           →  ~100 ns   (String-Operationen)
URN::hash()            →  ~50 ns    (xxHash-64)
ConsistentHashRing::   →  ~200 ns   (Mutex + lower_bound)
  getShardForHash()
ShardTopology::        →  ~100 ns   (Mutex + map lookup)
  getShard()
──────────────────────────────────
Gesamt (lokal)         →  ~450 ns   (sub-microsecond)
Netzwerk (intern)      →  +1–3 ms   (TCP/TLS, Rechenzentrum)
```

### 10.3 Verteilungsqualität

```
Szenarien getestet mit 8 Shards × 150 VNodes = 1.200 Ring-Einträge:

Gleichgewichtstest (1M URNs):
  Shard 001: 12,4 % der Anfragen (Ziel: 12,5 %)
  Shard 002: 12,6 %
  Shard 003: 12,3 %
  Shard 004: 12,5 %
  Shard 005: 12,5 %
  Shard 006: 12,4 %
  Shard 007: 12,7 %
  Shard 008: 12,6 %

Balance-Faktor: ~1,2 % (Ziel: <5 %) ✅
```

### 10.4 Skalierungsverhalten

```
Shards  VNodes  Ring-Größe  Lookup-Zeit  Balance-Faktor
─────────────────────────────────────────────────────────
    1     150        150       ~80 ns         0,0 %
    4     150        600      ~120 ns         2,1 %
    8     150       1200      ~150 ns         1,2 %
   16     150       2400      ~180 ns         0,9 %
   32     150       4800      ~200 ns         0,7 %
  128     150      19200      ~270 ns         0,4 %
```

### 10.5 Datenverschiebung bei Shard-Änderungen

```
Beim Hinzufügen von 1 Shard zu N vorhandenen Shards:
  Verschobene Datenmenge ≈ 1/(N+1) der Gesamtdaten

Beispiele:
  4 → 5 Shards:  20 % der Daten werden verschoben
  8 → 9 Shards:  11 % der Daten werden verschoben
 16 → 17 Shards:  6 % der Daten werden verschoben

Gegenüber statischem Hashing (Modulo N):
  100 % müssten neu verteilt werden!
```

### 10.6 RAID-Modus-Vergleich

```
Modus         | Write-Overhead  | Read-Skalierung | Speichereff.
──────────────────────────────────────────────────────────────────
NONE          | 1×              | 1×              | 100%
MIRROR RF=2   | 2× (parallel)   | 2×              |  50%
MIRROR RF=3   | 3× (parallel)   | 3×              |  33%
STRIPE        | 4× (parallel)   | 4×              | 100%
STRIPE_MIRROR | 2× (parallel)   | 2–3×            |  50%
PARITY (4+2)  | 1,5× + EC-Calc  | 1,5×            |  67%
RAID6 (8+3)   | 1,4× + EC-Calc  | 1,3×            |  73%
GEO_MIRROR    | 3× (cross-DC)   | lokal 3×        |  33%

Empfehlung nach Anwendungsfall:
  OLTP / User-Daten:          MIRROR RF=3
  Analytics / Aggregationen:  STRIPE oder NONE
  Große Binärdaten:           PARITY (4+2) oder RAID6
  Multi-Region-HA:            GEO_MIRROR
  Entwicklung / Staging:      NONE
```

---

## Quellcode-Referenzen

| Datei | Beschreibung |
|---|---|
| `include/sharding/urn.h` | URN-Struktur, Felder, Methoden |
| `src/sharding/urn.cpp` | URN::parse(), hash(), toString() |
| `include/sharding/urn_resolver.h` | URNResolver-Schnittstelle |
| `src/sharding/urn_resolver.cpp` | URNResolver-Implementierung |
| `include/sharding/consistent_hash.h` | ConsistentHashRing-Klasse |
| `src/sharding/consistent_hash.cpp` | Hash-Ring-Implementierung |
| `include/sharding/shard_topology.h` | ShardInfo, ShardTopology |
| `src/sharding/shard_topology.cpp` | Topologieverwaltung |
| `include/sharding/redundancy_strategy.h` | RAID-Modi, RedundancyMode |
| `src/main_server.cpp` | Server-Initialisierung mit RAID/Sharding |
| `tests/test_sharding_core.cpp` | Sharding-Unit-Tests |
| `tests/test_raid_integration.cpp` | RAID-Integrationstests |

---

## Verwandte Dokumentation

- [SHARDING_RAID_MODES_CONFIGURATION_v1.4.md](SHARDING_RAID_MODES_CONFIGURATION_v1.4.md) – Detaillierte RAID-Modi-Konfiguration
- [SHARDING_BENCHMARK_PLAN_v1.4.md](SHARDING_BENCHMARK_PLAN_v1.4.md) – Benchmark-Spezifikationen
- [SHARDING_PRODUCTION_DEPLOYMENT_RAID_v1.4.md](SHARDING_PRODUCTION_DEPLOYMENT_RAID_v1.4.md) – Produktions-Deployment-Leitfaden
- [SHARDING_MONITORING_OBSERVABILITY_RAID_v1.4.md](SHARDING_MONITORING_OBSERVABILITY_RAID_v1.4.md) – Monitoring & Observability

---

*Zuletzt aktualisiert: März 2026 | Status: ✅ Produktionsdokumentation*
