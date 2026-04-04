---
category: "🔄 Data Operations"
version: "v1.5.0"
status: "✅"
date: "2026-03-09"
---

# 📄 Change Data Capture (CDC)

Event-Stream für Datenänderungen mit SSE/WebSocket-Streaming, Consumer Groups und Enterprise-Integration.

## 📋 Inhaltsverzeichnis

- [📋 Übersicht](#-übersicht)
- [✨ Features](#-features)
- [🚀 Schnellstart](#-schnellstart)
- [📖 Detaillierte Dokumentation](#-detaillierte-dokumentation)
- [💡 Best Practices](#-best-practices)
- [🔧 Troubleshooting](#-troubleshooting)
- [📚 Siehe auch](#-siehe-auch)
- [📝 Changelog](#-changelog)

## 📋 Übersicht

---


## Overview

ThemisDB's Change Data Capture (CDC) implementation provides a production-ready, append-only
event log that tracks all data mutations (INSERT/UPDATE/DELETE) in the database. This enables
real-time data synchronisation, audit trails, stream processing, and enterprise integration
use cases.

**Implemented Features:**
- Sequence-basiertes Ordering (monoton steigende `sequence`)
- Automatische Erfassung für alle Schreiboperationen (INSERT/UPDATE/DELETE)
- Inkrementeller Abruf mit Checkpointing (`from_seq`)
- Filterung per `key_prefix`, Collection und Operation-Typ
- Ereignisse mit Timestamp, `before_snapshot`/`after_snapshot` und freiem `metadata`
- SSE-Streaming (`GET /changefeed/stream`) und WebSocket-Streaming (`/v2/cdc/stream`)
- Consumer Groups mit dauerhaftem Offset-Tracking (`ConsumerGroupManager`)
- At-least-once Delivery mit Acknowledgement und Redelivery (`DeliveryTracker`)
- Dead-Letter Queue für Events, die alle Retry-Versuche erschöpft haben (`DeadLetterQueue`)
- Kafka-Produzent (Debezium-Envelope-Format, opt-in via `THEMIS_ENABLE_KAFKA`)
- Transactional Outbox Pattern für atomares CDC + Application-Publishing (`OutboxWriter`, `OutboxRelay`)
- Cross-Collection aggregierte Streams mit per-Collection Resume-Cursor
- CDC-gesteuerte inkrementelle Materialized Views (O(1) pro Änderung)
- GDPR-konforme PII-Redaktion; Change Stream Kompression
- Schema Registry Integration (Confluent Wire Format)

> **Primäre Quellen:** [`src/cdc/README.md`](../../../src/cdc/README.md) ·
> [`src/cdc/ARCHITECTURE.md`](../../../src/cdc/ARCHITECTURE.md) ·
> [`src/cdc/ROADMAP.md`](../../../src/cdc/ROADMAP.md)

---

## Architecture

### Data Model

**ChangeEvent Structure:**
```json
{
  "sequence": 42,
  "type": "PUT",
  "key": "user:alice",
  "value": "{\"name\":\"Alice\",\"email\":\"alice@example.com\"}",
  "timestamp_ms": 1730294567123,
  "metadata": {
    "table": "user",
    "pk": "alice"
  },
  "before_snapshot": "{\"name\":\"Alice\",\"email\":\"alice@old.com\"}",
  "after_snapshot":  "{\"name\":\"Alice\",\"email\":\"alice@example.com\"}"
}
```

Hinweise zu Feldern:
- `sequence`: Monoton steigende ID (uint64)
- `type`: `PUT` oder `DELETE` (Typen `TRANSACTION_COMMIT`/`TRANSACTION_ROLLBACK` sind definiert, werden aktuell aber nicht emittiert)
- `key`: Vollständiger Schlüssel, z. B. `table:pk`
- `value`: JSON-String bei PUT, `null` bei DELETE
- `timestamp_ms`: Millisekunden seit Epoch
- `metadata`: Freies JSON (z. B. `table`, `pk`)
- `before_snapshot` *(optional)*: Zustand des Dokuments **vor** der Änderung; fehlt bei INSERT (neues Dokument, kein Vorzustand)
- `after_snapshot` *(optional)*: Zustand des Dokuments **nach** der Änderung; fehlt bei DELETE

### Storage

- **Column Family**: Default CF (or dedicated CF if configured)
- **Key Format**: `changefeed:{sequence_number}` (zero-padded for lexicographic ordering)
- **Sequence Tracking**: Atomic counter stored at key `changefeed_sequence`

---

## HTTP API

### 1. Query Changefeed Events

Retrieve change events with optional filtering and pagination.

**Endpoint:** `GET /changefeed`

**Query Parameters:**
- `from_seq` (optional): Start after this sequence number (default: 0)
- `limit` (optional): Maximum events to return (default: 100)
- `long_poll_ms` (optional): Long-poll timeout in milliseconds (default: 0 = immediate)
- `key_prefix` (optional): Filter events by key prefix (e.g., `user:`)

**Request Example:**
```bash
# Get all events
curl "http://localhost:8765/changefeed?from_seq=0&limit=20"

# Incremental query from checkpoint
curl "http://localhost:8765/changefeed?from_seq=42&limit=10"

# Filter by key prefix
curl "http://localhost:8765/changefeed?from_seq=0&limit=50&key_prefix=user:"

# Long-poll for new events
curl "http://localhost:8765/changefeed?from_seq=100&limit=10&long_poll_ms=5000"
```

**Response:**
```json
{
  "events": [
    {
      "sequence": 1,
      "type": "PUT",
      "key": "user:alice",
      "value": "{\"name\":\"Alice\",\"email\":\"alice@example.com\"}",
      "timestamp_ms": 1730294567123,
      "metadata": {"table": "user", "pk": "alice"},
      "before_snapshot": "{\"name\":\"Alice\",\"email\":\"alice@old.com\"}",
      "after_snapshot":  "{\"name\":\"Alice\",\"email\":\"alice@example.com\"}"
    },
    {
      "sequence": 2,
      "type": "DELETE",
      "key": "user:bob",
      "value": null,
      "timestamp_ms": 1730294568456,
      "metadata": {"table": "user", "pk": "bob"},
      "before_snapshot": "{\"name\":\"Bob\",\"email\":\"bob@example.com\"}"
    }
  ],
  "count": 2,
  "latest_sequence": 42
}
```

Antwortfelder:
- `events`: Liste von ChangeEvent-Objekten
- `count`: Anzahl der zurückgegebenen Events
- `latest_sequence`: Aktuell letzter Sequence-Wert in der DB (für Checkpointing hilfreich)

---

## Validation Tests (30.10.2025)

### Test 1: Automatic CDC Recording
**Status:** ✅ PASSED

- Created 4 entities via PUT → 4 CDC PUT events recorded
- Deleted 1 entity → 1 CDC DELETE event recorded
- **Result:** All mutations automatically tracked without manual intervention

### Test 2: Query API
**Status:** ✅ PASSED

- **Full Query:** Retrieved all events with `from_seq=0&limit=20`
- **Incremental Query:** Retrieved only new events after checkpoint
- **Key Prefix Filter:** Successfully filtered events by `user:` prefix
- **Pagination:** Limit parameter correctly restricts result size
- **Result:** Query API fully functional with all parameters

### Test 3: Event Structure
**Status:** ✅ PASSED

- **Event Types:** PUT and DELETE events correctly distinguished
- **Metadata:** Table and PK correctly embedded in each event
- **Timestamps:** Millisecond precision timestamps present
- **Value Handling:** PUT events include value, DELETE events have `value: null`
- **Result:** Event structure matches specification

### Test 4: Checkpointing Pattern
**Status:** ✅ PASSED

**Scenario:** Consumer reads batch → updates checkpoint → resumes from checkpoint
```
1. Consume events 1-5, checkpoint = 5
2. New events 6-7 arrive
3. Resume from checkpoint 5, receive events 6-7
```
**Result:** Sequential consumption with checkpointing works correctly

---

## Use Cases

### 1. Real-Time Data Synchronization
Stream database changes to external systems (analytics, search, caching):
```javascript
let checkpoint = 0;
setInterval(async () => {
  const res = await fetch(`/changefeed?from_seq=${checkpoint}&limit=100`);
  const data = await res.json();
  
  for (const event of data.events) {
    await externalSystem.sync(event);
    checkpoint = event.sequence;
  }
}, 1000);
```

### 2. Audit Trail & Compliance
Track all data mutations for compliance (GDPR, HIPAA):
```sql
-- Query all user data modifications
SELECT * FROM changefeed 
WHERE key LIKE 'user:%' AND timestamp_ms > '2025-01-01';
```

### 3. Materialized Views
Maintain denormalized views automatically:
```javascript
// Maintain user count by role
for (const event of events) {
  if (event.key.startsWith('user:')) {
    if (event.type === 'PUT') {
      const user = JSON.parse(event.value);
      roleCountMap[user.role] = (roleCountMap[user.role] || 0) + 1;
    } else if (event.type === 'DELETE') {
      // Decrement count
    }
  }
}
```

### 4. Event Sourcing
Rebuild application state from event log:
```javascript
// Rebuild state from beginning
const events = await fetch('/changefeed?from_seq=0&limit=1000');
const state = {};
for (const event of events.events) {
  applyEvent(state, event);
}
```

---

## Performance & Skalierung

Aktueller Stand (Production):
- Lock-freier Ring-Buffer pro Tenant (MPSC Queue) mit konfigurierbarem High-Water-Mark
- Backpressure: bei Erreichen des High-Water-Mark werden älteste Events verdrängt + Gap-Marker eingefügt
- Batch-SSE-Frames: mehrere Events werden unter Last in einem Frame gebündelt
- Change Log Compaction: veraltete Log-Segmente werden kompaktiert und archiviert
- Konfiguration: `cdc.buffer.max_events_per_tenant` (default 100 000), `cdc.buffer.high_watermark_pct` (default 80 %)

Für sehr hohe Last (> 100 000 Events/s) empfehlen sich:
1. Kafka-Backend (`KafkaCDCProducer`) für entkoppeltes Buffering
2. Dedizierte Column Family für das Change Log (konfigurierbar)
3. Mehrere Consumer Groups zur Parallelisierung der Verarbeitung

---

## Retention & Cleanup

### Retention & Cleanup

Aktuell: Admin-Endpoint `POST /changefeed/retention` mit Body `{ "before_sequence": <uint64> }` löscht Events mit kleinerer Sequence. Statistiken über `GET /changefeed/stats`.

Zukünftig möglich: TTL-/Zeit-basierte Retention und automatische Bereinigung.

---

## Dead-Letter Queue (DLQ)

Events, die nach Erschöpfung aller Wiederholungsversuche nicht zugestellt werden konnten, landen automatisch in der **Dead-Letter Queue** (`DeadLetterQueue`).

### Funktionsweise

Die DLQ ist RocksDB-gestützt (Schlüsselpräfix `dlq:`) und teilt dieselbe Datenbankinstanz wie der Changefeed (kein Schlüsselkonflikt: `changefeed:*` vs. `dlq:*`).

Jeder DLQ-Eintrag (`DLQEntry`) enthält:
- `dlq_sequence` — eindeutige Sequenz innerhalb der DLQ
- `event` — das ursprüngliche `ChangeEvent` (vollständig erhalten)
- `failure_reason` — lesbarer Grund (letzter Fehlermeldung)
- `attempt_count` — Anzahl der unternommenen Zustellungsversuche
- `enqueued_at_ms` — Zeitstempel der Einreihung (ms seit Epoch)

### Aktivierung

```cpp
// DLQ an ChangefeedBuffer hängen (nicht owned)
DeadLetterQueue dlq(db->getDB());
buffer.setDeadLetterQueue(&dlq);
```

Ab diesem Moment werden Events, bei denen alle Retry-Versuche scheitern, automatisch in `dlq` eingereiht statt verworfen.

### Inspektion und Wiedergabe

```cpp
// Alle fehlgeschlagenen Events auflisten
for (const auto& entry : dlq.listEntries()) {
    std::cout << "dlq_seq=" << entry.dlq_sequence
              << " key=" << entry.event.key
              << " reason=" << entry.failure_reason
              << " attempts=" << entry.attempt_count << "\n";
}

// Einzelnen Eintrag nach Ursachenbehebung erneut zustellen
Changefeed::ChangeEvent re_recorded = dlq.replay(dlq_seq, changefeed);
// replay() entfernt den Eintrag automatisch nach Erfolg

// Alle Einträge verwerfen
dlq.drain();
```

### Bekannte Einschränkungen
- Events, die aufgrund von Payload-Dekompressionsfehlern verworfen werden, landen **nicht** in der DLQ (die Daten sind in diesem Fall nicht wiederherstellbar).
- Die DLQ teilt den RocksDB-Namespace mit dem Changefeed; eine separate Column Family ist optional über den `cf`-Parameter konfigurierbar.

---

## Limitations & Trade-offs

### Current Limitations

1. **At-least-once (nicht SSE):** At-least-once Delivery ist via `ConsumerGroupManager` implementiert; für reine SSE-Verbindungen (ohne Consumer Group) gilt kein At-least-once-Guarantee.
2. **Outbox Relay:** In-Flight-State des Outbox-Relay liegt im Memory; FAILED-Records überleben Restarts, PENDING-Records werden nach Restart erneut zugestellt (at-least-once Semantik).
3. **DLQ:** Events, die wegen Payload-Dekompressionsfehlern verworfen werden, landen **nicht** in der DLQ (Daten sind nicht wiederherstellbar).
4. **Kafka:** `KafkaCDCProducer` erfordert das Build-Flag `THEMIS_ENABLE_KAFKA` und eine librdkafka-Installation.
5. **Change log Retention:** Retention-Policies sind zur Laufzeit nicht konfigurierbar (Admin-Endpoint `POST /changefeed/retention` erlaubt manuelles Trimming).

### Trade-offs

| Feature | Benefit | Cost |
|---------|---------|------|
| Append-only log | Simple, reliable, never loses events | Linear storage growth |
| Automatic tracking | Zero code changes, transparent | Cannot disable for specific entities |
| Sequence-based | Guaranteed order, easy checkpointing | Sequential bottleneck |
| Default CF storage | Simple deployment | No isolation from application data |

---

## Integration Examples

### PowerShell Consumer
```powershell
$checkpoint = 0
while ($true) {
    $r = Invoke-RestMethod -Uri "http://localhost:8765/changefeed?from_seq=$checkpoint&limit=100"
    
    foreach ($event in $r.events) {
        Write-Host "[$($event.sequence)] $($event.type) $($event.key)"
        
        # Process event
        if ($event.type -eq "PUT") {
            $data = $event.value | ConvertFrom-Json
            # Sync to external system
        }
        
        $checkpoint = $event.sequence
    }
    
    Start-Sleep -Seconds 1
}
```

### Python Consumer
```python
import requests
import time

checkpoint = 0
while True:
    r = requests.get(f'http://localhost:8765/changefeed?from_seq={checkpoint}&limit=100')
    data = r.json()
    
    for event in data['events']:
        print(f"[{event['sequence']}] {event['type']} {event['key']}")
        
        # Process event
        if event['type'] == 'PUT':
            value = json.loads(event['value'])
            # Sync to external system
        
        checkpoint = event['sequence']
    
    time.sleep(1)
```

### Node.js Consumer with Long-Poll
```javascript
let checkpoint = 0;

async function consumeChangefeed() {
  while (true) {
    const res = await fetch(
      `http://localhost:8765/changefeed?from_seq=${checkpoint}&limit=100&long_poll_ms=5000`
    );
    const data = await res.json();
    
    for (const event of data.events) {
      console.log(`[${event.sequence}] ${event.type} ${event.key}`);
      
      // Process event
      if (event.type === 'PUT') {
        const value = JSON.parse(event.value);
        await processEvent(value);
      }
      
      checkpoint = event.sequence;
    }
    
    // Long-poll handles waiting, no need to sleep
  }
}

consumeChangefeed();
```

---

## Configuration

### Enable CDC Feature

**config/config.json:**
```json
{
  "features": {
    "cdc": true
  },
  "sse": {
    "max_events_per_second": 0
  }
}
```

Bemerkungen:
- `features.cdc`: Aktiviert Changefeed und SSE-Streaming-Endpunkte
- `sse.max_events_per_second`: Server-seitiges Rate-Limit pro SSE-Verbindung (0 = unbegrenzt, empfohlen für Produktion: 0 oder 100–1000 je nach Last)

### Verify CDC Status

Check logs on server startup:
```
[INFO] Changefeed initialized using default CF
[INFO] SSE Connection Manager initialized
```

Query an endpoint to verify feature is enabled:
```bash
curl http://localhost:8765/changefeed?from_seq=0&limit=1
# Should return events, not 404
```

---

## Next Steps

### Offene Punkte

Alle zuvor geplanten CDC-Features sind implementiert. Verbleibende Einschränkungen sind im
Abschnitt *Limitations & Trade-offs* beschrieben. Für den aktuellen Entwicklungsstand siehe
[`src/cdc/ROADMAP.md`](../../../src/cdc/ROADMAP.md).

---

## SSE-Streaming: Backpressure & Reconnect

Das SSE-Streaming über `GET /changefeed/stream` ermöglicht nahezu Echtzeit-Konsum der CDC-Events mittels Server-Sent Events.

Parameter (Query):
- `from_seq` (uint64, optional): Start nach dieser Sequence (exklusiv)
- `key_prefix` (string, optional): Filter nach Schlüsselpräfix (z. B. `user:`)
- `keep_alive` (bool, default `true`): Streaming mit Heartbeats; bei `false` einmalige Batch-Antwort
- `max_seconds` (int, default `30`): Dauer des Streaming-Zyklus (Testzwecke, 1..60)
- `heartbeat_ms` (int, optional): Herzschlag-Intervall (nur Test/Override; min 100ms)
- `retry_ms` (int, default `3000`): SSE-„retry“-Hinweis für Client-Reconnects
- `max_events` (int, default `100`): Maximal pro Poll gelesene Events (Backpressure am Endpunkt)

Header (Client → Server):
- `Last-Event-ID`: Startet das Streaming ab der zuletzt verarbeiteten ID; dank exklusivem `from_seq` werden keine Duplikate erneut gesendet.

Format (Server → Client):
- Vorangestellter Hinweis: `retry: <ms>`
- Pro Event:
  - `id: <sequence>`
  - `data: <json>`
- Heartbeats als Kommentar: `: heartbeat`

Backpressure & Pufferung:
- Jeder Stream hat einen internen Puffer (Standard: 1000 Events). Bei Überlauf gilt Drop-Oldest-Policy (älteste Events werden verworfen).
- Pro-Poll-Kappung via `max_events` begrenzt die Eventabnahme je Schleifendurchlauf.
- Metrik: `vccdb_sse_dropped_events_total` zählt verwarfene Events (Prometheus unter `/metrics`).
- Optional (serverseitig): `max_events_per_second` Rate-Limit pro Verbindung (0 = unbegrenzt).

Client-Reconnect (Beispiel, Browser):
```javascript
let es;
function connect(fromSeq = 0) {
  const headers = fromSeq ? { 'Last-Event-ID': String(fromSeq) } : {};
  es = new EventSource('/changefeed/stream?keep_alive=true', { withCredentials: false });
  let lastId = fromSeq;
  es.onmessage = (ev) => {
    const data = JSON.parse(ev.data);
    lastId = parseInt(ev.lastEventId || lastId, 10);
    handleEvent(data);
  };
  es.onerror = () => {
    es.close();
    setTimeout(() => connect(lastId), 3000); // Backoff siehe retry
  };
}
connect();
```

Proxy-Guidelines (Nginx):
```nginx
location /changefeed/stream {
  proxy_pass http://themis_backend;
  proxy_http_version 1.1;
  proxy_set_header Connection '';
  proxy_buffering off;         # wichtig: keine Pufferung
  proxy_cache off;
  gzip off;                    # keine Transformation
  chunked_transfer_encoding on; # Streaming erlauben
  proxy_read_timeout 1h;       # lange Timeouts für SSE
  add_header Cache-Control "no-cache, no-transform";
}
```

Proxy-Guidelines (HAProxy):
```
frontend fe
  bind *:443
  default_backend be

backend be
  server s1 themis:8080 check
  http-response add-header Cache-Control "no-cache, no-transform"
  option http-keep-alive
  timeout server 1h
  timeout tunnel 1h
```

Empfohlene Defaults:
- `retry_ms=3000`, `heartbeat_ms≈15000`, `max_events=100`
- Proxy: Buffering aus, `no-transform`, lange Read-Timeouts, keine Gzip-Kompression
- Client: Last-Event-ID persistieren (z. B. lokal), Exponential Backoff bei Fehlern

Tests/Verifikation:
- Integrationstests prüfen: Last-Event-ID-Resume (exklusive Fortsetzung), Backpressure (Metrik > 0 unter Last), per-Poll-Kappung bei kurzer Laufzeit.

## Summary

Zusammenfassung:

- Sequence-basiertes append-only Log
- Automatisches Tracking für PUT/DELETE
- GET /changefeed mit Filter/Pagination + Long-Poll
- SSE-Streaming (`/changefeed/stream`) mit Keep-Alive/Heartbeats, Drop-Oldest-Backpressure, Retry/Resume über Last-Event-ID
- **Before/After-Snapshots:** PUT-Events enthalten `before_snapshot` (Dokument vor der Änderung, fehlt bei INSERT) und `after_snapshot` (Dokument nach der Änderung); DELETE-Events enthalten `before_snapshot`

Einsatz: Echtzeit-Sync, Audit-Trails, Event Sourcing – produktionsnah nutzbar; für sehr hohe Last ggf. Erweiterungen einplanen.
