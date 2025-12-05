# CDC Module (Change Data Capture)

**Stand:** 5. Dezember 2025  
**Version:** 1.0.0  
**Kategorie:** CDC

---

## Übersicht

Das CDC-Modul implementiert Change Data Capture für ThemisDB mit Sequence-basiertem Event-Tracking und Long-Polling.

## Source-Code Referenz

| Komponente | Header | Source | Beschreibung |
|------------|--------|--------|--------------|
| Changefeed | `changefeed.h` | `changefeed.cpp` | CDC Implementation |

**Gesamt:** 1 Header, 1 Source-Datei, ~510 LOC

## Implementierte Klassen

### Changefeed

```cpp
class Changefeed {
    // Key format: "changefeed:{sequence_number}"
    
    enum class ChangeEventType {
        EVENT_PUT,
        EVENT_DELETE,
        EVENT_TRANSACTION_COMMIT,
        EVENT_TRANSACTION_ROLLBACK
    };
    
    struct ChangeEvent {
        uint64_t sequence;                // Monotonic sequence number
        ChangeEventType type;             // Event type
        std::string key;                  // Affected key
        std::optional<std::string> value; // Value (nullopt for DELETE)
        int64_t timestamp_ms;             // Event timestamp
        nlohmann::json metadata;          // tx_id, user, etc.
    };
    
    struct ListOptions {
        uint64_t from_sequence = 0;       // Start after sequence
        size_t limit = 100;
        uint32_t long_poll_ms = 0;        // Long-poll timeout
        std::optional<std::string> key_prefix;
        std::optional<ChangeEventType> event_type;
    };
    
    struct Stats {
        uint64_t total_events;
        uint64_t latest_sequence;
        size_t total_size_bytes;
    };
    
    // API
    ChangeEvent publish(ChangeEventType type, key, value, metadata);
    std::vector<ChangeEvent> subscribe(ListOptions);
    Stats getStats();
};
```

## Features

### Event Types

| Type | Beschreibung |
|------|--------------|
| `EVENT_PUT` | Key-Value wurde eingefügt/aktualisiert |
| `EVENT_DELETE` | Key wurde gelöscht |
| `EVENT_TRANSACTION_COMMIT` | Transaktion committed |
| `EVENT_TRANSACTION_ROLLBACK` | Transaktion zurückgerollt |

### Long-Polling

```cpp
// Client wartet max 30 Sekunden auf neue Events
auto events = changefeed.subscribe({
    .from_sequence = last_seen_sequence,
    .limit = 100,
    .long_poll_ms = 30000
});
```

### Key-Prefix Filter

```cpp
// Nur Events für "users:" Keys
auto events = changefeed.subscribe({
    .key_prefix = "users:"
});
```

## HTTP API

### GET /api/cdc/events?from_sequence=100&limit=50

```json
{
  "events": [
    {
      "sequence": 101,
      "type": "PUT",
      "key": "users:123",
      "value": "{\"name\":\"Alice\"}",
      "timestamp_ms": 1733385600000,
      "metadata": {"tx_id": "txn-456"}
    }
  ],
  "next_sequence": 102
}
```

### GET /api/cdc/stats

```json
{
  "total_events": 10500,
  "latest_sequence": 10500,
  "total_size_bytes": 2097152
}
```

## Server-Sent Events (SSE)

```
GET /api/cdc/stream
Accept: text/event-stream

data: {"sequence":101,"type":"PUT","key":"users:123",...}

data: {"sequence":102,"type":"DELETE","key":"users:456",...}
```

## Beispiel

```cpp
Changefeed cdc(db);

// Event publizieren (automatisch bei DB-Operationen)
cdc.publish(ChangeEventType::EVENT_PUT, "users:123", user_json, {
    {"tx_id", "txn-456"},
    {"user", "admin"}
});

// Events abonnieren
auto events = cdc.subscribe({
    .from_sequence = 0,
    .limit = 100
});

for (const auto& event : events) {
    process(event);
}
```

## Verwandte Dokumentation

- [Features: CDC](../features/features_cdc.md) - Feature-Details
- [Features: Change Data Capture](../features/features_change_data_capture.md) - Konzept
