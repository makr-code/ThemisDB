# Netzwerk-Protokoll Übersicht und Build-Schalter

> **Kategorie:** Core API  
> **Seit Version:** 1.3.0  
> **Status:** Stable  
> **Aktualisiert:** 22. Dezember 2025

---

## Inhaltsverzeichnis

- [Ueberblick](#ueberblick)
- [Protokoll-Status und Build-Schalter](#protokoll-status-und-build-schalter)
- [Erste Schritte](#erste-schritte)
- [Detaillierte Konfiguration](#detaillierte-konfiguration)
- [Best Practices](#best-practices)
- [Troubleshooting](#troubleshooting)
- [Siehe auch](#siehe-auch)

---

## Ueberblick

ThemisDB unterstützt mehrere Netzwerk-Protokolle fuer verschiedene Use Cases. Aus Sicherheitsgruenden kann jedes Protokoll ueber Build-Schalter explizit aktiviert oder deaktiviert werden.

---

## Protokoll-Status und Build-Schalter

| Protokoll | Status | Build-Schalter | Standard | Beschreibung |
|-----------|--------|----------------|----------|--------------|
| **HTTP/1.1** | ✅ Production | - | **ON** | Basis REST API (immer aktiviert) |
| **GraphQL** | ✅ Production | `THEMIS_ENABLE_GRAPHQL` | **ON** | GraphQL API Unterstützung |
| **SSE** | ✅ Production | `THEMIS_ENABLE_SSE` | **ON** | Server-Sent Events für CDC/Live-Updates |
| **gRPC** | ✅ Production | `THEMIS_ENABLE_GRPC` | **ON** | Inter-Shard Kommunikation (v1.3.0) |
| **HTTP/2** | 🚧 Development | `THEMIS_ENABLE_HTTP2` | **OFF** | HTTP/2 REST API (opt-in) |
| **HTTP/3** | 📋 Planned | `THEMIS_ENABLE_HTTP3` | **OFF** | HTTP/3 (QUIC) REST API (opt-in) |
| **WebSocket** | 📋 Planned | `THEMIS_ENABLE_WEBSOCKET` | **OFF** | Bidirektionale Real-time Kommunikation |
| **MQTT** | 📋 Planned | `THEMIS_ENABLE_MQTT` | **OFF** | IoT Message Broker |
| **PostgreSQL Wire** | 📋 Planned | `THEMIS_ENABLE_POSTGRES_WIRE` | **OFF** | PostgreSQL Tool-Kompatibilität |

---

## Standard-Konfiguration (Production)

**Standardmäßig aktiviert (ON):**
- ✅ HTTP/1.1 (immer, nicht deaktivierbar)
- ✅ GraphQL (bereits implementiert)
- ✅ SSE - Server-Sent Events (bereits implementiert)
- ✅ gRPC (bereits implementiert für Inter-Shard Kommunikation)

**Standardmäßig deaktiviert (OFF):**
- ❌ HTTP/2 (opt-in aus Sicherheitsgründen)
- ❌ HTTP/3 (opt-in aus Sicherheitsgründen)
- ❌ WebSocket (noch nicht implementiert)
- ❌ MQTT (noch nicht implementiert)
- ❌ PostgreSQL Wire Protocol (noch nicht implementiert)

---

## Build-Beispiele

### Standard-Build (Production-Ready)

```bash
# Nur die bereits implementierten Protokolle
cmake -B build -S .
cmake --build build -j8
```

**Aktivierte Protokolle:** HTTP/1.1, GraphQL, SSE, gRPC

---

### Minimaler Build (Security-First)

```bash
# Nur HTTP/1.1 und gRPC (minimale Attack-Surface)
cmake -B build -S . \
  -DTHEMIS_ENABLE_GRAPHQL=OFF \
  -DTHEMIS_ENABLE_SSE=OFF
cmake --build build -j8
```

**Aktivierte Protokolle:** HTTP/1.1, gRPC

---

### HTTP/2 aktivieren

```bash
# HTTP/2 zusätzlich zu Standard-Protokollen
cmake -B build -S . \
  -DTHEMIS_ENABLE_HTTP2=ON
cmake --build build -j8
```

**Aktivierte Protokolle:** HTTP/1.1, GraphQL, SSE, gRPC, HTTP/2

---

### Alle Protokolle aktivieren (Development/Testing)

```bash
# Alle verfügbaren Protokolle
cmake -B build -S . \
  -DTHEMIS_ENABLE_HTTP2=ON \
  -DTHEMIS_ENABLE_HTTP3=ON \
  -DTHEMIS_ENABLE_WEBSOCKET=ON \
  -DTHEMIS_ENABLE_MQTT=ON \
  -DTHEMIS_ENABLE_POSTGRES_WIRE=ON
cmake --build build -j8
```

**Hinweis:** Einige Protokolle sind noch nicht vollständig implementiert (siehe Status-Tabelle oben).

---

## Sicherheitsmodell

### Opt-In Design

**Prinzip:** Nur explizit aktivierte Protokolle werden kompiliert und in die Binary eingebunden.

**Vorteile:**
1. ✅ **Minimale Attack-Surface** - Deaktivierte Protokolle werden nicht kompiliert
2. ✅ **Bewusste Entscheidung** - Administrator muss explizit aktivieren
3. ✅ **Granulare Kontrolle** - Jedes Protokoll unabhängig steuerbar
4. ✅ **Keine toten Code-Pfade** - `#ifdef` Guards verhindern ungenutzten Code
5. ✅ **Kleiner Binary** - Nur gewünschte Features eingebunden

### Production vs. Development

**Production (Empfohlen):**
```cmake
# Standard-Build: Nur stabile, implementierte Protokolle
THEMIS_ENABLE_GRAPHQL=ON
THEMIS_ENABLE_SSE=ON
THEMIS_ENABLE_GRPC=ON
THEMIS_ENABLE_HTTP2=OFF  # Opt-in wenn benötigt
THEMIS_ENABLE_HTTP3=OFF
```

**Development/Testing:**
```cmake
# Alle Protokolle für Testing
THEMIS_ENABLE_HTTP2=ON
THEMIS_ENABLE_HTTP3=ON
THEMIS_ENABLE_WEBSOCKET=ON
# etc.
```

---

## Protokoll-Details

### HTTP/1.1 (Immer aktiviert)

- **Status:** ✅ Production
- **Use Case:** Standard REST API
- **Implementierung:** Boost.Beast
- **Deaktivierbar:** Nein (Basis-Protokoll)

### GraphQL (Standard: ON)

- **Status:** ✅ Production
- **Use Case:** Flexible Query API
- **Implementierung:** `src/api/graphql.cpp`
- **Build-Schalter:** `THEMIS_ENABLE_GRAPHQL=ON/OFF`

### SSE - Server-Sent Events (Standard: ON)

- **Status:** ✅ Production
- **Use Case:** CDC/Changefeed, Live-Updates
- **Implementierung:** `src/server/sse_connection_manager.cpp`
- **Build-Schalter:** `THEMIS_ENABLE_SSE=ON/OFF`

### gRPC (Standard: ON)

- **Status:** ✅ Production (v1.3.0)
- **Use Case:** Inter-Shard Kommunikation, LoRA Transfer
- **Implementierung:** Sharding-Module
- **Build-Schalter:** `THEMIS_ENABLE_GRPC=ON/OFF`
- **Dokumentation:** `docs/llm/AI_ECOSYSTEM_SHARDING_ARCHITECTURE.md`

### HTTP/2 (Standard: OFF)

- **Status:** 🚧 Development
- **Use Case:** High-Performance REST API
- **Implementierung:** `src/server/http2_session.cpp` (Stub)
- **Build-Schalter:** `THEMIS_ENABLE_HTTP2=ON/OFF`
- **Dependencies:** nghttp2
- **Dokumentation:** `docs/apis/HTTP2_HTTP3_USAGE_GUIDE.md`

### HTTP/3 (Standard: OFF)

- **Status:** 📋 Planned
- **Use Case:** Mobile/WAN Resilience
- **Implementierung:** `src/server/http3_session.cpp` (Stub)
- **Build-Schalter:** `THEMIS_ENABLE_HTTP3=ON/OFF`
- **Dependencies:** nghttp3, ngtcp2
- **Dokumentation:** `docs/apis/HTTP2_HTTP3_USAGE_GUIDE.md`

### WebSocket (Standard: OFF)

- **Status:** 📋 Planned
- **Use Case:** Bidirektionale Real-time Kommunikation
- **Implementierung:** Noch nicht vorhanden
- **Build-Schalter:** `THEMIS_ENABLE_WEBSOCKET=ON/OFF`
- **Dependencies:** Boost.Beast (bereits vorhanden)
- **Dokumentation:** `docs/apis/ADDITIONAL_PROTOCOLS.md`

### MQTT (Standard: OFF)

- **Status:** 📋 Planned
- **Use Case:** IoT Message Broker
- **Implementierung:** Noch nicht vorhanden
- **Build-Schalter:** `THEMIS_ENABLE_MQTT=ON/OFF`
- **Dependencies:** paho-mqttpp3
- **Dokumentation:** `docs/apis/ADDITIONAL_PROTOCOLS.md`

### PostgreSQL Wire Protocol (Standard: OFF)

- **Status:** 📋 Planned
- **Use Case:** PostgreSQL Tool-Kompatibilität
- **Implementierung:** Noch nicht vorhanden
- **Build-Schalter:** `THEMIS_ENABLE_POSTGRES_WIRE=ON/OFF`
- **Dependencies:** Keine (eigene Implementation)
- **Dokumentation:** `docs/apis/ADDITIONAL_PROTOCOLS.md`

---

## vcpkg Features

Protokolle können auch über vcpkg Features aktiviert werden:

```bash
# HTTP/2
vcpkg install themis-qnap[http2]

# HTTP/3
vcpkg install themis-qnap[http3]

# gRPC
vcpkg install themis-qnap[grpc]

# WebSocket (keine zusätzlichen Dependencies)
vcpkg install themis-qnap[websocket]

# MQTT
vcpkg install themis-qnap[mqtt]

# Mehrere Features
vcpkg install themis-qnap[http2,grpc,websocket]
```

---

## Migration

### Von älteren Versionen (< v1.2.0)

**Ältere Versionen:** Alle Protokolle automatisch aktiviert

**v1.2.0+:** Explizites Opt-In erforderlich

**Migration:**
```bash
# Alt (< v1.2.0): Alles automatisch aktiviert
cmake -B build -S .

# Neu (>= v1.2.0): Explizit aktivieren
cmake -B build -S . \
  -DTHEMIS_ENABLE_HTTP2=ON  # falls HTTP/2 benötigt
```

### Bestehende Deployments

**Check aktivierte Protokolle:**
```bash
# Binary prüfen
strings ./build/themis_server | grep "THEMIS_ENABLE"

# Build-Log prüfen
grep "support enabled" build/CMakeCache.txt
```

---

## Best Practices

### ✅ DO

1. **Production: Nur benötigte Protokolle aktivieren**
   ```bash
   # Beispiel: Nur HTTP/1.1 + gRPC
   cmake -B build -S . \
     -DTHEMIS_ENABLE_GRAPHQL=OFF \
     -DTHEMIS_ENABLE_SSE=OFF
   ```

2. **Testing: Alle Protokolle aktivieren**
   ```bash
   # CI/CD: Teste alle Protokolle
   cmake -B build -S . \
     -DTHEMIS_ENABLE_HTTP2=ON \
     -DTHEMIS_ENABLE_HTTP3=ON
   ```

3. **Dokumentation prüfen**
   - Check Status-Tabelle oben
   - Nur stabile Protokolle in Production

### ❌ DON'T

1. **Nicht alle Protokolle in Production aktivieren**
   - Erhöhte Attack-Surface
   - Größerer Binary
   - Mehr Maintenance

2. **Nicht ohne Testing aktivieren**
   - Neue Protokolle erst testen
   - Performance-Impact messen
   - Security-Audit durchführen

---

## Troubleshooting

### Protokoll nicht verfügbar

**Problem:** Feature aktiviert, aber nicht verfügbar

**Lösung:**
```bash
# 1. Check Build-Log
grep "support enabled" build/CMakeCache.txt

# 2. Check Dependencies
cmake -B build -S . -DTHEMIS_ENABLE_HTTP2=ON 2>&1 | grep nghttp2

# 3. Install Dependencies
vcpkg install nghttp2

# 4. Rebuild
cmake --build build -j8
```

### Binary zu groß

**Problem:** Binary ist zu groß (>200 MB)

**Lösung:**
```bash
# Minimaler Build
cmake -B build -S . \
  -DCMAKE_BUILD_TYPE=Release \
  -DTHEMIS_ENABLE_GRAPHQL=OFF \
  -DTHEMIS_ENABLE_SSE=OFF \
  -DTHEMIS_ENABLE_TRACING=OFF

# Strip Binary
strip ./build/themis_server
```

---

## Referenzen

- [HTTP/2 und HTTP/3 Dokumentation](HTTP2_HTTP3_USAGE_GUIDE.md)
- [Zusätzliche Protokolle](ADDITIONAL_PROTOCOLS.md)
- [HTTP API Referenz](HTTP_API_REFERENCE.md)
- [gRPC Sharding Architektur](../llm/AI_ECOSYSTEM_SHARDING_ARCHITECTURE.md)
