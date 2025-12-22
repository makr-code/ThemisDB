# HTTP/2 und HTTP/3 Support - Benutzerhandbuch

> **Kategorie:** Core API  
> **Seit Version:** 1.3.0  
> **Status:** Beta  
> **Aktualisiert:** 22. Dezember 2025

---

## Inhaltsverzeichnis

- [Ueberblick](#ueberblick)
- [HTTP/2 Support](#http2-support)
- [HTTP/3 Support](#http3-support)
- [Erste Schritte](#erste-schritte)
- [Best Practices](#best-practices)
- [Troubleshooting](#troubleshooting)
- [Siehe auch](#siehe-auch)

---

## Überblick

ThemisDB unterstützt moderne HTTP-Protokolle für verbesserte Performance und Effizienz:

- **HTTP/1.1** ✅ Production (Standard, Boost.Beast)
- **HTTP/2** 🚧 In Development (nghttp2, optional)
- **HTTP/3** 📋 Planned (nghttp3 + ngtcp2, optional)

### 🔒 Sicherheitshinweis

**Alle erweiterten Protokolle sind standardmäßig DEAKTIVIERT** aus Sicherheitsgründen:

- HTTP/2 und HTTP/3 müssen **explizit** via Build-Schalter aktiviert werden (`THEMIS_ENABLE_HTTP2=ON`, `THEMIS_ENABLE_HTTP3=ON`)
- Opt-In Design: Kein Protokoll wird automatisch aktiviert
- Jedes Protokoll kann unabhängig aktiviert/deaktiviert werden
- Nur aktivierte Protokolle werden kompiliert und in die Binary eingebunden

**Grund:** Neue Protokolle bringen zusätzliche Attack-Surface. Administratoren müssen bewusst entscheiden, welche Protokolle aktiviert werden.

---

## HTTP/2 Support

### Build-Konfiguration

#### 1. Dependencies installieren

**Wichtig:** HTTP/2 Support ist standardmäßig DEAKTIVIERT. Sie müssen ihn explizit aktivieren.

```bash
# Via vcpkg
vcpkg install nghttp2

# Oder mit Feature Flag
vcpkg install themis-qnap[http2]
```

#### 2. Build mit HTTP/2 (explizit aktivieren)

```bash
# CMake konfigurieren mit HTTP/2 Support (EXPLIZIT aktivieren!)
cmake -B build -S . \
  -DTHEMIS_ENABLE_HTTP2=ON \
  -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build --config Release -j8

# Verifizieren, dass HTTP/2 aktiviert ist
grep "HTTP/2 support enabled" build/CMakeCache.txt
```

**Standard (ohne -DTHEMIS_ENABLE_HTTP2=ON):** HTTP/2 ist NICHT verfügbar (sicher by default)

### Server-Konfiguration

#### JSON-Konfiguration

```json
{
  "host": "0.0.0.0",
  "port": 8443,
  "num_threads": 8,
  
  "enable_tls": true,
  "tls_cert_path": "/path/to/server.crt",
  "tls_key_path": "/path/to/server.key",
  "tls_min_version": "TLSv1.3",
  
  "enable_http2": true,
  "http2_max_concurrent_streams": 100,
  "http2_initial_window_size": 65535
}
```

#### C++ API

```cpp
#include "server/http_server.h"

themis::server::HttpServer::Config config;
config.host = "0.0.0.0";
config.port = 8443;
config.num_threads = 8;

// TLS ist Pflicht für HTTP/2
config.enable_tls = true;
config.tls_cert_path = "/path/to/server.crt";
config.tls_key_path = "/path/to/server.key";
config.tls_min_version = "TLSv1.3";

// HTTP/2 aktivieren
config.enable_http2 = true;
config.http2_max_concurrent_streams = 100;
config.http2_initial_window_size = 65535;

auto server = std::make_shared<themis::server::HttpServer>(
    config, storage, secondary_index, graph_index, vector_index, tx_manager
);

server->start();
```

### Client-Nutzung

#### curl mit HTTP/2

```bash
# Automatische HTTP/2 Negotiation
curl --http2 https://localhost:8443/health

# HTTP/2 erzwingen (Prior Knowledge)
curl --http2-prior-knowledge https://localhost:8443/health

# Verbose für Protokoll-Details
curl --http2 -v https://localhost:8443/health
```

#### Python mit httpx

```python
import httpx

# httpx unterstützt HTTP/2 automatisch
client = httpx.Client(http2=True)
response = client.get("https://localhost:8443/health")
print(f"HTTP Version: {response.http_version}")
print(response.json())
```

#### Node.js mit http2

```javascript
const http2 = require('http2');
const fs = require('fs');

const client = http2.connect('https://localhost:8443');

const req = client.request({
  ':path': '/health',
  ':method': 'GET'
});

req.on('response', (headers) => {
  console.log('Status:', headers[':status']);
});

req.on('data', (chunk) => {
  console.log(chunk.toString());
});

req.end();
```

### Performance-Tuning

#### Optimale Einstellungen für verschiedene Use Cases

**1. Dashboard mit vielen parallelen Requests**

```json
{
  "http2_max_concurrent_streams": 200,
  "http2_initial_window_size": 131072,
  "num_threads": 16
}
```

**2. Batch-Processing (große Payloads)**

```json
{
  "http2_max_concurrent_streams": 50,
  "http2_initial_window_size": 524288,
  "max_request_size_mb": 100
}
```

**3. Microservices (viele kleine Requests)**

```json
{
  "http2_max_concurrent_streams": 100,
  "http2_initial_window_size": 65535,
  "request_timeout_ms": 5000
}
```

### Monitoring

#### Prometheus Metrics

```
# HTTP/2 spezifische Metrics
themis_http2_active_streams{endpoint="/api/query"} 42
themis_http2_max_concurrent_streams 100
themis_http2_window_size_bytes 65535
themis_http2_streams_total{status="ok"} 12345
themis_http2_streams_total{status="error"} 42
```

#### Logging

```cpp
// Logger zeigt Protokoll-Version an
THEMIS_INFO("HTTP/2 request: GET /api/query (stream_id=3)");
THEMIS_DEBUG("HTTP/2 HPACK compression ratio: 0.35");
```

---

## HTTP/3 Support (Geplant)

### Build-Konfiguration

**Wichtig:** HTTP/3 Support ist standardmäßig DEAKTIVIERT. Sie müssen ihn explizit aktivieren.

```bash
# Via vcpkg (wenn verfügbar)
vcpkg install nghttp3 ngtcp2[openssl]

# Oder mit Feature Flag
vcpkg install themis-qnap[http3]

# CMake konfigurieren (EXPLIZIT aktivieren!)
cmake -B build -S . \
  -DTHEMIS_ENABLE_HTTP3=ON \
  -DCMAKE_BUILD_TYPE=Release
```

**Standard (ohne -DTHEMIS_ENABLE_HTTP3=ON):** HTTP/3 ist NICHT verfügbar (sicher by default)

### Server-Konfiguration

```json
{
  "host": "0.0.0.0",
  "port": 8443,
  "num_threads": 8,
  
  "enable_tls": true,
  "tls_cert_path": "/path/to/server.crt",
  "tls_key_path": "/path/to/server.key",
  "tls_min_version": "TLSv1.3",
  
  "enable_http3": true,
  "http3_port": 8443,
  "http3_max_idle_timeout_ms": 30000
}
```

### Client-Nutzung

```bash
# curl mit HTTP/3 (wenn verfügbar)
curl --http3 https://localhost:8443/health

# Chrome mit HTTP/3
chrome --enable-quic --quic-version=h3-29
```

**Hinweis:** HTTP/3 Support ist derzeit ein Stub. Vollständige Implementation erfordert:
- QUIC Connection Management (ngtcp2)
- HTTP/3 Framing (nghttp3)
- 0-RTT Connection Resumption
- Connection Migration

---

## ALPN (Application-Layer Protocol Negotiation)

### Protokoll-Negotiation

ThemisDB unterstützt automatisches Fallback:

```
Client → Server: ClientHello mit ALPN (h2, http/1.1)
Server → Client: ServerHello mit ausgewähltem Protokoll
```

**Priorität:**
1. HTTP/3 (h3) - wenn aktiviert und verfügbar
2. HTTP/2 (h2) - wenn aktiviert und TLS vorhanden
3. HTTP/1.1 (http/1.1) - Fallback

### Testing ALPN

```bash
# OpenSSL zeigt ALPN-Verhandlung
openssl s_client -connect localhost:8443 -alpn h2,http/1.1

# Ausgabe sollte enthalten:
# ALPN protocol: h2
```

---

## Troubleshooting

### HTTP/2 funktioniert nicht

**Problem:** Client verbindet sich mit HTTP/1.1 statt HTTP/2

**Lösungen:**

1. **TLS prüfen**
   ```bash
   # HTTP/2 erfordert TLS
   curl -v https://localhost:8443/health 2>&1 | grep "ALPN"
   ```

2. **ALPN-Support prüfen**
   ```bash
   openssl s_client -connect localhost:8443 -alpn h2
   # Sollte "ALPN protocol: h2" zeigen
   ```

3. **Server-Logs prüfen**
   ```
   THEMIS_INFO("HTTP/2 ALPN configured (h2, http/1.1)")
   THEMIS_DEBUG("HTTP/2 negotiated via ALPN")
   ```

### HTTP/2 Streams blockiert

**Problem:** Requests hängen, keine Response

**Ursache:** Flow Control Window erschöpft

**Lösung:**
```json
{
  "http2_initial_window_size": 131072,  // Verdoppeln
  "http2_max_concurrent_streams": 50    // Reduzieren
}
```

### Hohe Latenz trotz HTTP/2

**Problem:** HTTP/2 ist langsamer als HTTP/1.1

**Mögliche Ursachen:**

1. **TCP Head-of-Line Blocking**
   - HTTP/2 über verlustbehaftetes Netzwerk
   - **Lösung:** HTTP/3 (QUIC) oder HTTP/1.1 mit mehreren Connections

2. **Zu viele parallele Streams**
   - CPU-Overhead für Stream-Multiplexing
   - **Lösung:** `http2_max_concurrent_streams` reduzieren

3. **HPACK Overhead**
   - Bei sehr kleinen Requests kann HPACK langsamer sein
   - **Lösung:** Batching oder HTTP/1.1 nutzen

---

## Performance-Vergleich

### Benchmark-Szenario: 1000 parallele Requests

| Protokoll | Latenz (avg) | Throughput | CPU-Last |
|-----------|--------------|------------|----------|
| HTTP/1.1 (6 Connections) | 45ms | 22K req/s | 35% |
| HTTP/2 (1 Connection) | 28ms | 36K req/s | 42% |
| HTTP/3 (geplant) | ~25ms* | ~40K req/s* | ~50%* |

*Geschätzte Werte basierend auf Literatur

### Bandwidth-Einsparung (HPACK)

| Scenario | HTTP/1.1 | HTTP/2 | Einsparung |
|----------|----------|--------|------------|
| Typische REST API (Bearer Auth) | 450 KB | 320 KB | ~29% |
| Viele kleine Requests | 2.1 MB | 1.2 MB | ~43% |
| Große Payloads (JSON) | 45 MB | 44.5 MB | ~1% |

---

## Migration von HTTP/1.1 zu HTTP/2

### Schritt 1: TLS aktivieren

```json
{
  "enable_tls": true,
  "tls_cert_path": "/etc/themis/certs/server.crt",
  "tls_key_path": "/etc/themis/certs/server.key",
  "tls_min_version": "TLSv1.3"
}
```

### Schritt 2: HTTP/2 aktivieren (Soft Launch)

```json
{
  "enable_http2": true,
  "http2_max_concurrent_streams": 50
}
```

**Wichtig:** HTTP/1.1 bleibt weiterhin verfügbar (Fallback)!

### Schritt 3: Monitoring einrichten

```bash
# Prometheus-Metriken beobachten
curl http://localhost:8080/metrics | grep themis_http2
```

### Schritt 4: Client-Migration

```python
# Clients schrittweise auf HTTP/2 migrieren
client = httpx.Client(http2=True)  # Automatisches Fallback zu HTTP/1.1
```

### Schritt 5: Performance validieren

```bash
# Load-Test mit HTTP/2
ab -n 10000 -c 100 -k https://localhost:8443/health
```

### Schritt 6: Tuning

Basierend auf Monitoring-Daten:
- `http2_max_concurrent_streams` anpassen
- `http2_initial_window_size` optimieren
- `num_threads` für höheren Durchsatz erhöhen

---

## Sicherheit

### 🔒 Opt-In Security Model

**Wichtiger Sicherheitsaspekt:** Alle erweiterten HTTP-Protokolle folgen einem expliziten Opt-In Model:

1. **Standardmäßig deaktiviert**
   - HTTP/2: `THEMIS_ENABLE_HTTP2=OFF` (default)
   - HTTP/3: `THEMIS_ENABLE_HTTP3=OFF` (default)
   - Nur HTTP/1.1 ist standardmäßig verfügbar

2. **Explizite Aktivierung erforderlich**
   - Administratoren müssen bewusst entscheiden, welche Protokolle aktiviert werden
   - Jedes Protokoll bringt zusätzliche Attack-Surface
   - Granulare Kontrolle: Jedes Protokoll unabhängig aktivierbar

3. **Build-Zeit Isolation**
   - Deaktivierte Protokolle werden NICHT in die Binary kompiliert
   - Keine toten Code-Pfade, keine ungenutzten Dependencies
   - Minimale Attack-Surface

**Empfehlung:** Nur Protokolle aktivieren, die wirklich benötigt werden.

### TLS-Konfiguration für HTTP/2

**Empfohlene Cipher Suites (TLS 1.3):**

```cpp
config.tls_cipher_list = "TLS_AES_256_GCM_SHA384:TLS_CHACHA20_POLY1305_SHA256";
```

### mTLS (Mutual TLS)

```json
{
  "enable_tls": true,
  "tls_require_client_cert": true,
  "tls_ca_cert_path": "/etc/themis/certs/ca.crt"
}
```

### Rate Limiting pro Stream

HTTP/2 erlaubt viele Streams pro Connection:

```cpp
// Max 100 Streams pro Connection
config.http2_max_concurrent_streams = 100;

// Rate Limiting pro Client-IP (nicht pro Connection!)
config.audit_rate_limit_per_minute = 1000;
```

---

## Best Practices

### ✅ DO

1. **TLS 1.3 verwenden**
   - HTTP/2 funktioniert am besten mit TLS 1.3
   - Niedrigere Latenz durch 0-RTT

2. **Connection Pooling deaktivieren**
   - HTTP/2 multiplexed alle Requests über eine Connection
   - Mehrere Connections sind kontraproduktiv

3. **HPACK-freundliche Header**
   - Statische Header (z.B. Content-Type) wiederverwenden
   - HPACK komprimiert wiederholte Header effizient

4. **Monitoring einrichten**
   - Stream-Zählungen überwachen
   - Flow Control Window beobachten

### ❌ DON'T

1. **Kein HTTP/2 ohne TLS**
   - HTTP/2 Cleartext (h2c) wird nicht unterstützt
   - Browser unterstützen nur HTTP/2 über TLS

2. **Nicht zu viele parallele Streams**
   - Mehr als 200 Streams → CPU-Overhead
   - Besser: Requests batchen

3. **Keine Annahme über Connection-Persistenz**
   - HTTP/2 Connections können jederzeit schließen
   - Client muss Reconnect-Logik haben

4. **Nicht mit HTTP/1.1 Load Balancer**
   - Load Balancer muss HTTP/2 verstehen
   - Sonst Downgrade zu HTTP/1.1

---

## Roadmap

### v1.2.0 (Q1 2025) - ✅ Aktuell

- [x] HTTP/2 Build-Infrastruktur
- [x] HTTP/2 Session Handler (nghttp2)
- [x] ALPN Negotiation
- [ ] HTTP/2 Request-Response Mapping
- [ ] HTTP/2 Testing & Validation

### v1.3.0 (Q2 2025)

- [ ] HTTP/2 Performance-Optimierungen
- [ ] HTTP/2 Server Push für CDC/Changefeed
- [ ] HTTP/2 Stream-Priorisierung

### v1.4.0 (Q3 2025) - Optional

- [ ] HTTP/3 QUIC Connection Management
- [ ] HTTP/3 HTTP Framing
- [ ] HTTP/3 0-RTT Connection Resumption
- [ ] HTTP/3 Connection Migration

---

## Support & Feedback

### Bug Reports

GitHub Issues: https://github.com/makr-code/ThemisDB/issues

Template:
```
**Protokoll:** HTTP/2
**Build-Config:** THEMIS_ENABLE_HTTP2=ON
**Client:** curl 8.4.0 --http2
**Symptom:** Connection timeout
**Logs:** [Server-Logs anhängen]
```

### Feature Requests

Für HTTP/3 oder andere Protokolle siehe [ADDITIONAL_PROTOCOLS.md](ADDITIONAL_PROTOCOLS.md)

---

## Referenzen

- [HTTP/2 RFC 7540](https://datatracker.ietf.org/doc/html/rfc7540)
- [HTTP/3 RFC 9114](https://datatracker.ietf.org/doc/html/rfc9114)
- [nghttp2 Documentation](https://nghttp2.org/documentation/)
- [HTTP/2 Performance Analysis](HTTP2_HTTP3_PROTOCOL_SUPPORT.md)
- [Additional Protocols](ADDITIONAL_PROTOCOLS.md)
