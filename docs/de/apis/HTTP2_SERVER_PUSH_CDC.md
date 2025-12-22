# HTTP/2 Server Push fuer CDC/Changefeed

> **Kategorie:** Enterprise Feature  
> **Seit Version:** 1.3.0  
> **Status:** Beta  
> **Aktualisiert:** 22. Dezember 2025

---

## Inhaltsverzeichnis

- [Ueberblick](#ueberblick)
- [Architektur](#architektur)
- [Erste Schritte](#erste-schritte)
- [Detaillierte Dokumentation](#detaillierte-dokumentation)
- [Best Practices](#best-practices)
- [Troubleshooting](#troubleshooting)
- [Siehe auch](#siehe-auch)

---

## Ueberblick

HTTP/2 Server Push ermoglicht die proaktive Lieferung von CDC (Change Data Capture) Events zu Clients ohne Polling. Dies reduziert erheblich die Latenz und Bandbreite im Vergleich zu traditionellen Polling-basierten Ansaetzen.

## Architecture

```
┌─────────────────────────────────────────────┐
│         HTTP/2 Client (Browser/App)         │
│                                             │
│  1. Subscribe: GET /cdc/subscribe           │
└──────────────────┬──────────────────────────┘
                   │
                   │ HTTP/2 Connection
                   │ (TLS with ALPN "h2")
                   ▼
┌─────────────────────────────────────────────┐
│        ThemisDB HTTP/2 Server               │
│                                             │
│  2. Track subscribed streams                │
│  3. On DB change: Broadcast Server Push     │
└──────────────────┬──────────────────────────┘
                   │
                   │ PUSH_PROMISE + Pushed Response
                   │ Path: /cdc/event/{sequence}
                   ▼
┌─────────────────────────────────────────────┐
│         HTTP/2 Client Receives Event        │
│                                             │
│  Response Body: CDC Event JSON              │
│  {                                          │
│    "type": "cdc_event",                     │
│    "sequence": 123,                         │
│    "key": "user:1001",                      │
│    "value": {...},                          │
│    "operation": "PUT"                       │
│  }                                          │
└─────────────────────────────────────────────┘
```

## Usage

### 1. Subscribe to CDC via HTTP/2

**Request:**
```http
GET /cdc/subscribe HTTP/2
Host: localhost:8443
```

**Response:**
```json
{
  "status": "subscribed",
  "message": "HTTP/2 Server Push enabled for CDC events"
}
```

### 2. Receive CDC Events via Server Push

When a database change occurs, the server automatically pushes events:

**Push Promise:**
```
PUSH_PROMISE Frame
Stream ID: 1 (client stream)
Promised Stream ID: 2 (server push)
:method: GET
:path: /cdc/event/123
:scheme: https
:authority: localhost
```

**Pushed Response:**
```http
HTTP/2 200
content-type: application/json
x-cdc-sequence: 123

{
  "type": "cdc_event",
  "sequence": 123,
  "key": "user:1001",
  "value": {
    "name": "Alice",
    "age": 30
  },
  "operation": "PUT",
  "timestamp": "2025-12-18T18:00:00Z"
}
```

## Configuration

Enable HTTP/2 Server Push in `config.json`:

```json
{
  "enable_tls": true,
  "enable_http2": true,
  "http2_max_concurrent_streams": 100,
  "http2_initial_window_size": 65535,
  "cdc_server_push_enabled": true
}
```

## Client Examples

### JavaScript (Browser)

```javascript
// Modern browsers support HTTP/2 Server Push automatically
async function subscribeToCDC() {
  const response = await fetch('/cdc/subscribe');
  const data = await response.json();
  console.log('Subscribed:', data.message);
  
  // Pushed resources are automatically cached by browser
  // Access them via Resource Timing API
  performance.getEntriesByType('resource').forEach(entry => {
    if (entry.name.includes('/cdc/event/')) {
      console.log('CDC Event pushed:', entry.name);
    }
  });
}
```

### Node.js (http2 module)

```javascript
const http2 = require('http2');

const client = http2.connect('https://localhost:8443');

// Subscribe to CDC
const req = client.request({
  ':path': '/cdc/subscribe',
  ':method': 'GET'
});

req.on('response', (headers) => {
  console.log('Subscribed to CDC');
});

// Handle Server Push
client.on('stream', (pushedStream, headers) => {
  const path = headers[':path'];
  
  if (path.startsWith('/cdc/event/')) {
    let data = '';
    pushedStream.on('data', (chunk) => {
      data += chunk;
    });
    
    pushedStream.on('end', () => {
      const event = JSON.parse(data);
      console.log('CDC Event received:', event);
    });
  }
});

req.end();
```

### Python (httpx with HTTP/2)

```python
import httpx
import json

# HTTP/2 client
client = httpx.Client(http2=True)

# Subscribe to CDC
response = client.get('https://localhost:8443/cdc/subscribe')
print(f"Subscribed: {response.json()}")

# Note: Python httpx doesn't expose push promises directly
# Use WebSocket for Python CDC streaming instead
```

### curl (HTTP/2 with Server Push)

```bash
# Subscribe and receive pushed events
curl --http2 https://localhost:8443/cdc/subscribe \
  --verbose \
  --output /dev/null \
  --include

# Note: curl 7.57.0+ supports HTTP/2 Server Push
# Pushed resources are saved to disk automatically
```

## HTTP/2 Server Push vs WebSocket for CDC

| Feature | HTTP/2 Server Push | WebSocket |
|---------|-------------------|-----------|
| **Protocol** | HTTP/2 over TLS | WebSocket over HTTP/1.1 or HTTP/2 |
| **Connection** | Reuses existing HTTP/2 | Separate WebSocket upgrade |
| **Directionality** | Server → Client only | Bidirectional |
| **Browser Support** | Automatic (transparent) | Requires WebSocket API |
| **Latency** | Low (push on change) | Very low (continuous) |
| **Bandwidth** | Efficient (no polling) | Very efficient (streaming) |
| **Use Case** | Occasional updates | Continuous streaming |
| **Setup Complexity** | Simple (standard HTTP/2) | Moderate (upgrade required) |

**Recommendation:**
- Use **HTTP/2 Server Push** for occasional CDC notifications with minimal client-side code
- Use **WebSocket CDC** for high-frequency updates or bidirectional communication

## Performance Benefits

### Without Server Push (Polling)
```
Client ----[Poll Request]----> Server (no changes)
Client <---[Empty Response]--- Server
Client ----[Poll Request]----> Server (no changes)
Client <---[Empty Response]--- Server
Client ----[Poll Request]----> Server (has changes!)
Client <---[CDC Event]-------- Server

Cost: 3 roundtrips, 2 wasted requests
Latency: Up to poll_interval
```

### With Server Push
```
Client ----[Subscribe]-------> Server
Client <---[Subscribed]------- Server
... wait for actual changes ...
Client <====[PUSH: CDC Event]= Server (proactive!)

Cost: 1 roundtrip setup, 0 wasted requests
Latency: ~0ms (immediate on change)
```

**Latency Improvement:** 95%+ reduction (no polling delay)  
**Bandwidth Savings:** 90%+ reduction (no empty poll responses)

## Technical Details

### nghttp2 Implementation

ThemisDB uses `nghttp2` library for HTTP/2 Server Push:

```cpp
// Create push promise
int32_t promised_stream_id;
nghttp2_submit_push_promise(
    session, 
    NGHTTP2_FLAG_NONE,
    stream_id,                    // Client stream
    promise_headers,
    num_headers,
    &promised_stream_id          // Output: server push stream ID
);

// Send pushed response on promised stream
nghttp2_submit_response(
    session,
    promised_stream_id,          // Push stream (even number)
    response_headers,
    num_headers,
    &data_provider
);
```

### Stream ID Assignment

- **Client streams:** Odd numbers (1, 3, 5, 7, ...)
- **Server push streams:** Even numbers (2, 4, 6, 8, ...)

Each push creates a new server-initiated stream ID.

### Security Considerations

1. **TLS Required:** Server Push only works over HTTPS (HTTP/2 requires TLS in browsers)
2. **ALPN Negotiation:** "h2" must be negotiated during TLS handshake
3. **Push Limits:** Respect `http2_max_concurrent_streams` setting
4. **Authorization:** CDC subscription requires same auth as regular requests

## Monitoring

### Metrics

Track Server Push performance:

```json
{
  "http2_server_push": {
    "total_push_promises": 12345,
    "total_pushed_responses": 12345,
    "active_cdc_subscriptions": 42,
    "push_errors": 0,
    "average_push_latency_ms": 5.2
  }
}
```

### Logging

```
[INFO] HTTP/2 stream 1 subscribed to CDC with Server Push
[DEBUG] HTTP/2 Server Push promise created for stream 1, promised stream 2
[DEBUG] HTTP/2 Server Push sent CDC event to stream 1, sequence 123
```

## Troubleshooting

### Issue: Server Push not working

**Check:**
1. HTTPS enabled (`enable_tls: true`)
2. HTTP/2 enabled (`THEMIS_ENABLE_HTTP2=ON`)
3. ALPN negotiated "h2" (check logs)
4. Client supports HTTP/2 Server Push (modern browsers do)

### Issue: Push promise rejected

**Cause:** Client sent `SETTINGS_ENABLE_PUSH: 0`

**Solution:** Client must enable push in SETTINGS frame

### Issue: Too many concurrent streams

**Cause:** Exceeded `http2_max_concurrent_streams`

**Solution:** Increase limit or close inactive streams

## References

- [RFC 9113: HTTP/2](https://www.rfc-editor.org/rfc/rfc9113.html) - Section 8.4 Server Push
- [nghttp2 Documentation](https://nghttp2.org/documentation/nghttp2_submit_push_promise.html)
- [MDN: HTTP/2 Server Push](https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Link)

## Next Steps

- [ ] Add push cache control headers
- [ ] Implement push promise validation
- [ ] Add push rate limiting per client
- [ ] Create dashboard for monitoring push metrics
- [ ] Add support for filter-based push (only specific keys)
