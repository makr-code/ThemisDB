# `semantic_cache.cpp` — Implementierungsreferenz

**Modul:** `src/cache/`  
**Header:** `include/cache/semantic_cache.h`  
**Version:** 0.0.32  
**Status:** ✅ Production Ready  

---

## Übersicht

`SemanticCache` stellt ein exaktes Prompt-Hash-basiertes Cache für LLM-Antworten bereit,
das in einer dedizierten RocksDB-Column-Family persistiert wird. Jede Anfrage wird per
SHA-256 über `prompt + JSON.stringify(params)` gekennzeichnet; die Antwort wird als
strukturiertes JSON-Objekt mit TTL-Metadaten gespeichert.

**Storage-Layout:**

| Feld | Typ | Beschreibung |
|------|-----|--------------|
| Key | `string` | `SHA256(prompt + params.dump())` als Hex-String (64 Zeichen) |
| Value | JSON | `{response, metadata, timestamp_ms, ttl_seconds}` |

---

## Klasseninterface

```cpp
namespace themis {

class SemanticCache {
public:
    struct CacheEntry {
        std::string     response;       // LLM-Antwort
        nlohmann::json  metadata;       // Modell, Tokens, etc.
        int64_t         timestamp_ms;   // Zeitpunkt der Speicherung (ms seit Epoch)
        int             ttl_seconds;    // TTL in Sekunden (0 = kein Ablauf)
    };

    struct Stats {
        uint64_t hit_count;             // Cache-Treffer gesamt
        uint64_t miss_count;            // Cache-Fehler gesamt
        uint64_t total_entries;         // Einträge im Cache
        uint64_t total_size_bytes;      // Gesamtgröße (Schätzwert)
        double   hit_rate;              // hit_count / (hit_count + miss_count)
        double   avg_latency_ms;        // Mittlere Latenz pro query()-Aufruf
    };

    // Konstruktor – RocksDB-Instanz und Column-Family-Handle müssen extern verwaltet werden
    SemanticCache(rocksdb::TransactionDB* db,
                  rocksdb::ColumnFamilyHandle* cf_handle,
                  int default_ttl_seconds = 3600);

    bool put(const std::string& prompt,
             const nlohmann::json& params,
             const std::string& response,
             const nlohmann::json& metadata = {},
             int ttl_seconds = 0);

    std::optional<CacheEntry> query(const std::string& prompt,
                                    const nlohmann::json& params);

    Stats    getStats() const;
    uint64_t clearExpired();
    bool     clear();
};

} // namespace themis
```

---

## Methoden

### `put()`

Speichert eine LLM-Antwort im Cache.

| Parameter | Typ | Beschreibung |
|-----------|-----|--------------|
| `prompt` | `string` | Prompt-Text |
| `params` | `json` | Modellparameter (z. B. `model`, `temperature`) |
| `response` | `string` | LLM-Antwort |
| `metadata` | `json` | Optionale Zusatzinformationen (Token-Anzahl, Modellversion) |
| `ttl_seconds` | `int` | `0` = Standard-TTL; `-1` = kein Ablauf |

**Rückgabe:** `true` bei Erfolg, `false` bei RocksDB-Schreibfehler.

### `query()`

Sucht nach einem gültigen Cache-Eintrag für das angegebene Prompt/Params-Paar.

| Parameter | Typ | Beschreibung |
|-----------|-----|--------------|
| `prompt` | `string` | Prompt-Text |
| `params` | `json` | Modellparameter |

**Rückgabe:** `std::optional<CacheEntry>` — vorhanden, wenn Treffer und nicht abgelaufen.

Abgelaufene Einträge werden bei einer `query()`-Anfrage **nicht** automatisch gelöscht;
sie müssen explizit via `clearExpired()` bereinigt werden.

### `getStats()`

Gibt aktuelle Metriken als `Stats`-Struct zurück. Thread-sicher (atomic counters).

### `clearExpired()`

Iteriert über alle Einträge der Column Family und entfernt abgelaufene Einträge.
Gibt die Anzahl gelöschter Einträge zurück. Geeignet als Hintergrund-Kompaktierungsauslöser.

### `clear()`

Löscht alle Einträge der Column Family. Gibt `true` bei Erfolg zurück.

---

## Interne Hilfsmethoden

| Methode | Beschreibung |
|---------|--------------|
| `computeKey(prompt, params)` | SHA-256-Hex von `prompt + params.dump()` |
| `isExpired(entry)` | Prüft `timestamp_ms + ttl_seconds * 1000 < now_ms` |
| `getCurrentTimestampMs()` | Gibt `std::chrono::system_clock` in Millisekunden zurück |

---

## Speicher- und Serialisierungsformat

```json
{
  "response":     "<LLM-Antwort als String>",
  "metadata":     { "model": "llama3", "tokens": 128 },
  "timestamp_ms": 1706524800000,
  "ttl_seconds":  3600
}
```

`CacheEntry::toJson()` / `CacheEntry::fromJson()` sind für die Serialisierung verantwortlich;
`fromJson()` gibt `std::nullopt` zurück, wenn das JSON-Objekt unvollständig oder ungültig ist.

---

## Threading-Modell

`SemanticCache` ist **nicht** intern thread-sicher. Die Metriken (`hit_count_`, `miss_count_`,
`total_query_latency_ms_`) sind als `mutable`-Felder ohne Mutex deklariert. Beim Einsatz in
Multithreading-Umgebungen muss der Aufrufer externe Synchronisierung sicherstellen oder
separate Instanzen pro Thread verwenden.

---

## Abhängigkeiten

| Abhängigkeit | Verwendung |
|---|---|
| `rocksdb::TransactionDB` | L3-Persistenz (Column Family `semantic_cache`) |
| `rocksdb::ColumnFamilyHandle` | Namespacing innerhalb von RocksDB |
| `nlohmann::json` | Serialisierung von CacheEntry und Parametern |
| `openssl/sha.h` | SHA-256-Berechnung für Cache-Schlüssel |

---

## Konfiguration

`SemanticCache` wird typischerweise aus `AdaptiveQueryCache` heraus instanziiert.
Die relevanten Konfigurationsparameter in `AdaptiveQueryCache::Config`:

```cpp
config.semantic_similarity_threshold = 0.95;  // Cosine-Ähnlichkeitsschwelle
config.default_ttl_seconds           = 3600;  // Standard-TTL (1 Stunde)
```

---

## Bekannte Einschränkungen

- Kein Mutex-Schutz für interne Metriken (`hit_count_`, etc.); bei parallelem Zugriff
  können Zählerwerte inkonsistent sein (kein funktionaler Fehler, nur Messfehler).
- Kein automatisches Ablaufen abgelaufener Einträge; `clearExpired()` muss vom Aufrufer
  periodisch aufgerufen werden.
- Keine Kompression der gespeicherten Werte (im Gegensatz zu L2 in `AdaptiveQueryCache`).

---

## Weiterführende Dokumentation

- [Semantic Cache Feature Documentation](../../features/features_semantic_cache.md) — Nutzerdokumentation
- [Cache Module README](../../../../src/cache/README.md) — Modulübersicht
- [Cache Invalidation Architecture](../../architecture/architecture_cache_invalidation.md) — Invalidierungsmuster
