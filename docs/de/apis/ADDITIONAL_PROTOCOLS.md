# Weitere Netzwerk-Protokolle fuer ThemisDB

> **Kategorie:** Core API  
> **Seit Version:** 1.3.0  
> **Status:** Reference  
> **Aktualisiert:** 22. Dezember 2025

---

## Inhaltsverzeichnis

- [Ueberblick](#ueberblick)
- [Protokoll-Uebersicht](#protokoll-uebersicht)
- [Erste Schritte](#erste-schritte)
- [Detaillierte Analyse](#detaillierte-analyse)
- [Best Practices](#best-practices)
- [Troubleshooting](#troubleshooting)
- [Siehe auch](#siehe-auch)

---

## Übersicht

Dieses Dokument analysiert weitere Netzwerk-Protokolle über HTTP hinaus, die für ThemisDB-Deployments relevant sein könnten. Die Analyse berücksichtigt Use Cases, Implementierungsaufwand und Business Value.

---

## Protokoll-Übersicht nach Use Case

| Protokoll | Use Case | Prio | Aufwand | ROI |
|-----------|----------|------|---------|-----|
| **gRPC** | Microservices, Service-to-Service | **HOCH** | Mittel | **Sehr hoch** |
| **WebSocket** | Real-time Dashboards, Live-Updates | **HOCH** | Niedrig | **Hoch** |
| **GraphQL über WebSocket** | Subscriptions, Real-time Queries | Mittel | Mittel | Mittel |
| **Server-Sent Events (SSE)** | One-way Live-Updates | Niedrig | Sehr niedrig | Mittel |
| **MQTT** | IoT, Sensor-Datenströme | Mittel | Mittel | Hoch (IoT) |
| **Apache Kafka Protocol** | Event Streaming, CDC | Niedrig | Hoch | Mittel |
| **Redis Protocol (RESP)** | Cache-Kompatibilität | Niedrig | Mittel | Niedrig |
| **PostgreSQL Wire Protocol** | SQL-Tool-Kompatibilität | Mittel | Hoch | Hoch (Enterprise) |
| **Native UDP** | Low-Latency Bulk-Transfer | Sehr niedrig | Hoch | Sehr niedrig |
| **SCTP** | Multi-homing, Mobilität | Sehr niedrig | Sehr hoch | Sehr niedrig |

---

## 1. gRPC (Google Remote Procedure Call)

### Überblick
- **Transport:** HTTP/2 (binary frames)
- **Serialisierung:** Protocol Buffers (Protobuf)
- **Use Case:** High-performance service-to-service communication

### Vorteile

#### Performance
1. **Binäres Protokoll**
   - Protobuf ist 3-10x kompakter als JSON
   - Parsing ist 5-20x schneller als JSON
   - **Nutzen für ThemisDB:** Ideal für Microservice-Architekturen mit vielen API-Calls

2. **HTTP/2 Multiplexing**
   - Alle gRPC-Vorteile von HTTP/2 (siehe vorheriges Dokument)
   - **Nutzen:** Reduzierte Latenz bei parallelen Requests

3. **Streaming**
   - Unidirektional (Server/Client)
   - Bidirektional
   - **Nutzen für ThemisDB:** 
     - Bulk Insert Streams
     - Large Query Result Streaming
     - Real-time CDC Feeds

#### Developer Experience

4. **Code-Generierung**
   - Automatische Client-Generierung für 10+ Sprachen
   - Type-Safety durch Protobuf-Schemas
   - **Nutzen:** Reduzierter SDK-Entwicklungsaufwand

5. **Tooling**
   - grpcurl (CLI-Testing wie curl)
   - gRPC UI (Postman-Ähnliches GUI)
   - Built-in Health Checking
   - **Nutzen:** Bessere Developer Experience

### Nachteile

1. **Browser-Support problematisch**
   - Native gRPC funktioniert nicht im Browser
   - Benötigt gRPC-Web (Proxy-Schicht)
   - **Problem:** Web-Dashboards benötigen separate REST-API

2. **Debugging schwieriger**
   - Binäres Protokoll nicht human-readable
   - **Mitigation:** grpcurl, Wireshark-Plugin

3. **Firewall/Proxy-Probleme**
   - Manche alte HTTP-Proxies haben Probleme mit gRPC
   - **Risiko:** Niedrig, da HTTP/2 inzwischen weit verbreitet

### Implementierungsaufwand

**Zeitrahmen:** 2-3 Wochen

- **Woche 1:** Protobuf-Schema-Definition (alle ThemisDB-Operationen)
- **Woche 2:** gRPC-Server-Implementation (C++ mit grpc++)
- **Woche 3:** Client-SDK-Generierung + Testing

**Dependencies:**
```json
{ "name": "grpc", "features": ["cpp-plugin"] }
{ "name": "protobuf" }
```

### Use Cases für ThemisDB

1. **Microservice-Kommunikation** ✅
   - ThemisDB als Backend für Microservices
   - Niedrige Latenz, hoher Throughput

2. **Bulk-Operationen** ✅
   - Streaming Insert (CSV-Import)
   - Large Query Results (Pagination über Streams)

3. **CDC/Changefeed** ✅
   - Server-Side Streaming für Echtzeit-Events
   - Alternative zu SSE mit besserer Performance

### Empfehlung

**✅ STARK EMPFOHLEN** - Prio 1 nach HTTP/2

**Begründung:**
- Sehr hoher Nutzen für Service-to-Service Kommunikation
- Geringer Aufwand (grpc++ ist mature)
- Koexistenz mit REST-API möglich (beide parallel betreiben)

**Strategie:** 
- REST-API für Web-Clients und Public API
- gRPC für interne Microservices und SDKs

---

## 2. WebSocket (RFC 6455)

### Überblick
- **Transport:** TCP mit HTTP-Upgrade
- **Use Case:** Full-duplex bidirektionale Kommunikation
- **Status:** ThemisDB hat bereits SSE (unidirektional) - WebSocket ist der nächste Schritt

### Vorteile

1. **Bidirektionale Kommunikation**
   - Client und Server können jederzeit senden
   - **Nutzen:** Interaktive Queries mit Live-Updates

2. **Niedriger Overhead**
   - Kein HTTP-Header bei jedem Frame
   - **Nutzen:** Effizienter als Polling oder SSE

3. **Breite Browser-Unterstützung**
   - Alle modernen Browser
   - **Nutzen:** Einfache Integration in Web-Dashboards

### Nachteile

1. **Stateful**
   - Server muss Verbindungen offen halten
   - **Impact:** Memory-Overhead (~40 KB pro Verbindung)

2. **Load Balancing komplex**
   - Sticky Sessions erforderlich
   - **Problem:** Horizontale Skalierung schwieriger

3. **Firewall-Probleme**
   - Manche Corporate-Proxies blockieren WebSockets
   - **Mitigation:** Fallback zu SSE oder Long-Polling

### Implementierungsaufwand

**Zeitrahmen:** 1 Woche

Boost.Beast unterstützt WebSocket bereits!

```cpp
#include <boost/beast/websocket.hpp>
```

**Tasks:**
- WebSocket-Handler in HttpServer integrieren
- Subscription-Management für CDC/Changefeed
- Ping/Pong Keep-Alive

### Use Cases für ThemisDB

1. **Live Query Dashboards** ✅
   - Real-time Metrics
   - Live Graph-Visualisierung

2. **Interactive CDC** ✅
   - Client kann Filter ändern (z.B. "nur Tabelle X")
   - Server pushed nur relevante Events

3. **Collaborative Editing** ✅
   - Mehrere Clients bearbeiten gleiche Daten
   - Server broadcasted Änderungen

### Empfehlung

**✅ EMPFOHLEN** - Prio 2

**Begründung:**
- Geringer Aufwand (Boost.Beast unterstützt es bereits)
- Bessere Alternative zu SSE für interaktive Use Cases
- Standard für moderne Web-Apps

**Strategie:**
- SSE behalten für einfache One-Way-Updates
- WebSocket hinzufügen für bidirektionale Kommunikation

---

## 3. GraphQL Subscriptions (über WebSocket)

### Überblick
- **Transport:** WebSocket (graphql-ws Protokoll)
- **Use Case:** Real-time GraphQL Queries

### Vorteile

1. **Deklarative Subscriptions**
   ```graphql
   subscription {
     entityChanged(type: "User") {
       id
       name
       timestamp
     }
   }
   ```

2. **Client-definierte Payload**
   - Client wählt Felder
   - **Nutzen:** Reduziert Bandbreite

### Nachteile

1. **GraphQL-Server erforderlich**
   - ThemisDB hat bereits GraphQL (siehe `src/api/graphql.cpp`)
   - Aber: WebSocket-Integration fehlt noch

2. **Komplexität**
   - Resolver für Subscriptions
   - Pub/Sub-System erforderlich

### Implementierungsaufwand

**Zeitrahmen:** 2 Wochen

- GraphQL-Subscriptions-Resolver
- Integration mit Changefeed
- WebSocket-Transport

### Empfehlung

**⚠️ OPTIONAL** - Prio 4

**Begründung:**
- Nutzen ist marginal gegenüber WebSocket + REST
- Höherer Aufwand
- Nur sinnvoll wenn GraphQL stark genutzt wird

---

## 4. MQTT (Message Queuing Telemetry Transport)

### Überblick
- **Transport:** TCP (oder WebSocket)
- **Use Case:** IoT, Sensor-Daten, Pub/Sub
- **Pattern:** Publisher/Subscriber mit Topics

### Vorteile

1. **IoT-Standard**
   - Extrem weit verbreitet in IoT-Welt
   - Kleine Embedded Devices (ESP32, Arduino)
   - **Nutzen:** ThemisDB könnte IoT-Sensor-Daten direkt aufnehmen

2. **Niedriger Overhead**
   - ~2 Byte Header
   - **Nutzen:** Ideal für hochfrequente Sensor-Daten

3. **QoS-Levels**
   - 0: At most once
   - 1: At least once
   - 2: Exactly once
   - **Nutzen:** Garantien für kritische Daten

4. **Last Will & Testament**
   - Device kann "Abschiedsnachricht" konfigurieren
   - **Nutzen:** Erkennung von Device-Ausfällen

### Nachteile

1. **Broker erforderlich**
   - ThemisDB müsste MQTT-Broker sein
   - **Aufwand:** Komplettes MQTT-Protokoll implementieren (~4-6 Wochen)

2. **Nicht HTTP-kompatibel**
   - Separater Port (Standard: 1883)
   - **Problem:** Firewall-Konfiguration

### Implementierungsaufwand

**Zeitrahmen:** 4-6 Wochen

**Dependencies:**
- mosquitto (als Library) ODER
- Eigene Implementation mit Paho MQTT C++

### Use Cases für ThemisDB

1. **IoT-Sensor-Ingestion** ✅
   ```
   Sensor → MQTT → ThemisDB → Time-Series Store
   ```

2. **Edge Computing** ✅
   - Lokale MQTT-Broker auf Edge-Devices
   - ThemisDB aggregiert Daten

3. **Event-Driven Architecture** ✅
   - ThemisDB published CDC-Events via MQTT
   - Andere Services subscribed

### Empfehlung

**⚠️ NUR FÜR IoT-USE CASES** - Prio 3

**Begründung:**
- Hoher Aufwand
- Nur sinnvoll wenn ThemisDB IoT-Backend sein soll
- Alternative: Externe MQTT-Broker (Mosquitto) + ThemisDB als Storage-Backend

**Bessere Strategie:**
- MQTT-Bridge implementieren (2-3 Tage)
- Mosquitto → HTTP → ThemisDB
- ThemisDB muss kein MQTT-Broker sein

---

## 5. Apache Kafka Protocol

### Überblick
- **Use Case:** Event Streaming, Log Aggregation
- **Pattern:** Publish/Subscribe mit Persistence

### Vorteile

1. **Kafka-Kompatibilität**
   - ThemisDB könnte Kafka-Topics emulieren
   - **Nutzen:** Kafka-Clients könnten direkt ThemisDB lesen

2. **CDC mit Replay**
   - Kafka-Style Event-Log
   - **Nutzen:** Debezium-kompatibel

### Nachteile

1. **Sehr hohe Komplexität**
   - Kafka-Protokoll ist extrem komplex
   - **Aufwand:** 3-6 Monate für vollständige Implementation

2. **ThemisDB ist keine Message-Queue**
   - Unterschiedliche Architektur
   - **Problem:** Impedance Mismatch

### Empfehlung

**❌ NICHT EMPFOHLEN**

**Begründung:**
- Zu hoher Aufwand
- ThemisDB ist Database, nicht Message-Queue
- Bessere Alternative: Kafka Connect Sink für ThemisDB entwickeln

---

## 6. Redis Protocol (RESP - Redis Serialization Protocol)

### Überblick
- **Use Case:** Cache-Kompatibilität
- **Pattern:** Key-Value Commands

### Vorteile

1. **Redis-Tool-Kompatibilität**
   - redis-cli könnte ThemisDB bedienen
   - **Nutzen:** Einfaches Testing/Debugging

2. **Einfaches Protokoll**
   - Text-basiert
   - **Aufwand:** 1-2 Wochen

### Use Cases

1. **Semantic Cache als Redis-Ersatz**
   ```
   GET embedding:user123
   SET embedding:user123 [0.1, 0.2, ...]
   ```

2. **Session-Store**
   - ThemisDB als Redis-Alternative für Sessions

### Empfehlung

**⚠️ NISCHEN-USE CASE** - Prio 5

**Begründung:**
- Geringer Aufwand, aber auch geringer Nutzen
- ThemisDB ist zu mächtig für einfache Key-Value-Ops
- Redis ist dafür besser geeignet

---

## 7. PostgreSQL Wire Protocol

### Überblick
- **Use Case:** PostgreSQL-Tool-Kompatibilität
- **Pattern:** SQL über binäres Protokoll

### Vorteile

1. **Tool-Ökosystem** ⭐
   - pgAdmin, DBeaver, DataGrip funktionieren out-of-the-box
   - **Nutzen:** Massive Developer-Experience-Verbesserung

2. **ORM-Kompatibilität**
   - Django, Rails, TypeORM könnten ThemisDB als Postgres nutzen
   - **Nutzen:** Keine speziellen Adapter erforderlich

3. **SQL-Standard**
   - ThemisDB hat bereits SQL-ähnliche AQL
   - **Synergy:** Könnte vollständiges SQL werden

### Nachteile

1. **Sehr hohe Komplexität**
   - Vollständiges SQL-Implementation erforderlich
   - **Aufwand:** 6-12 Monate

2. **Semantik-Unterschiede**
   - ThemisDB ist Multi-Model, nicht relational
   - **Problem:** Viele PostgreSQL-Features würden fehlen

### Implementierungsaufwand

**Zeitrahmen:** 6-12 Monate (vollständige PostgreSQL-Kompatibilität)

**Alternative: Subset-Implementation (3 Monate):**
- Basic SELECT/INSERT/UPDATE/DELETE
- PostgreSQL Wire Protocol
- Mapping zu AQL

### Empfehlung

**✅ LANGFRISTIG SEHR WERTVOLL** - Prio 2 (Enterprise)

**Begründung:**
- Sehr hoher Business-Value (Tool-Kompatibilität)
- Aber: Sehr hoher Aufwand
- Erst nach Core-Features etabliert

**Strategie:**
- V1: PostgreSQL-kompatible Subset (SELECT, INSERT, UPDATE, DELETE)
- V2: Transactions (BEGIN, COMMIT, ROLLBACK)
- V3: Advanced Features (JOINs, Subqueries)

---

## 8. Native UDP für Bulk-Transfer

### Überblick
- **Use Case:** Low-Latency Bulk-Daten ohne Reliability-Overhead
- **Pattern:** Fire-and-forget

### Vorteile

1. **Maximale Performance**
   - Kein TCP-Overhead
   - **Nutzen:** ~50% weniger Latenz als TCP

2. **Multicast-fähig**
   - Ein Sender, viele Empfänger
   - **Use Case:** Metrics-Broadcasting

### Nachteile

1. **Keine Reliability**
   - Pakete können verloren gehen
   - **Problem:** Ungeeignet für kritische Daten

2. **Sehr spezieller Use Case**
   - Fast immer besser: HTTP/3 (QUIC) nutzen
   - **Fazit:** Redundant mit HTTP/3

### Empfehlung

**❌ NICHT EMPFOHLEN**

**Begründung:**
- HTTP/3 bietet bessere Alternative (UDP + Reliability)
- Zu spezieller Use Case
- Wartungsaufwand nicht gerechtfertigt

---

## 9. SCTP (Stream Control Transmission Protocol)

### Überblick
- **Use Case:** Multi-Homing, Message-orientiert
- **Pattern:** TCP-Alternative mit erweiterten Features

### Vorteile

1. **Multi-Homing**
   - Verbindung über mehrere IPs gleichzeitig
   - **Use Case:** Hochverfügbarkeit

2. **Message-Boundaries**
   - Im Gegensatz zu TCP byte-stream
   - **Nutzen:** Einfacheres Framing

### Nachteile

1. **Kaum verbreitet**
   - NAT-Probleme
   - Firewall-Blockierung
   - **Problem:** Deployment unmöglich

2. **Keine Browser-Support**
   - **Problem:** Webbasierte Clients ausgeschlossen

### Empfehlung

**❌ NICHT EMPFOHLEN**

**Begründung:**
- Zu wenig verbreitet
- HTTP/3 (QUIC) bietet ähnliche Features mit besserer Adoption

---

## Zusammenfassung & Empfehlungen

### Kurzfristig (Q1 2025) - Prio 1

1. **HTTP/2 Support** ✅
   - **Aufwand:** 2-3 Wochen
   - **Nutzen:** Sehr hoch (30-50% Performance-Gewinn)
   - **Status:** Bereits in Arbeit

2. **gRPC Support** ✅
   - **Aufwand:** 2-3 Wochen
   - **Nutzen:** Sehr hoch für Microservices
   - **Delivery:** Q1 2025

3. **WebSocket Support** ✅
   - **Aufwand:** 1 Woche
   - **Nutzen:** Hoch für Real-time Dashboards
   - **Delivery:** Q1 2025

### Mittelfristig (Q2-Q3 2025) - Prio 2

4. **PostgreSQL Wire Protocol (Subset)** ⭐
   - **Aufwand:** 3 Monate
   - **Nutzen:** Sehr hoch (Tool-Kompatibilität)
   - **Delivery:** Q3 2025

5. **MQTT-Bridge** (falls IoT-Use Case vorhanden)
   - **Aufwand:** 2-3 Tage (Bridge), 4-6 Wochen (vollständiger Broker)
   - **Nutzen:** Hoch für IoT-Deployments
   - **Delivery:** On-Demand

### Langfristig (Q4 2025+) - Prio 3

6. **HTTP/3 Support** (nur für spezielle Use Cases)
   - **Aufwand:** 6-8 Wochen
   - **Nutzen:** Variabel (hoch bei Mobile/Edge)
   - **Delivery:** Wenn Business Case validiert

7. **GraphQL Subscriptions** (optional)
   - **Aufwand:** 2 Wochen
   - **Nutzen:** Mittel
   - **Delivery:** On-Demand

### Nicht empfohlen

- ❌ Kafka Protocol (zu komplex, falscher Use Case)
- ❌ Redis Protocol (geringer Nutzen)
- ❌ Native UDP (redundant mit HTTP/3)
- ❌ SCTP (zu wenig verbreitet)

---

## Protokoll-Stack-Architektur (Vorschlag)

```
┌─────────────────────────────────────────────────────────┐
│                   ThemisDB API Gateway                  │
├─────────────────────────────────────────────────────────┤
│                                                           │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌─────────┐ │
│  │   REST   │  │   gRPC   │  │WebSocket │  │PostgreSQL│ │
│  │ HTTP/1.1 │  │  (HTTP/2)│  │          │  │   Wire   │ │
│  │ HTTP/2   │  │          │  │          │  │  Protocol│ │
│  │ HTTP/3*  │  │          │  │          │  │          │ │
│  └────┬─────┘  └────┬─────┘  └────┬─────┘  └────┬────┘ │
│       │             │             │             │       │
│       └─────────────┴─────────────┴─────────────┘       │
│                         │                                │
│                  ┌──────▼──────┐                         │
│                  │   Unified   │                         │
│                  │   Request   │                         │
│                  │   Handler   │                         │
│                  └──────┬──────┘                         │
│                         │                                │
├─────────────────────────┼────────────────────────────────┤
│                         ▼                                │
│              ┌────────────────────┐                      │
│              │  ThemisDB Core     │                      │
│              │  (Storage Engine)  │                      │
│              └────────────────────┘                      │
└─────────────────────────────────────────────────────────┘

Optional Extensions:
┌──────────┐
│  MQTT    │  (IoT Use Case)
│  Bridge  │
└────┬─────┘
     │
     └─→ HTTP → ThemisDB
```

---

## Implementierungs-Roadmap

### Sprint 1-2 (Wochen 1-3): HTTP/2
- [x] vcpkg.json + CMakeLists.txt (bereits erledigt)
- [ ] nghttp2 Integration
- [ ] ALPN Negotiation
- [ ] Testing

### Sprint 3-4 (Wochen 4-6): gRPC
- [ ] Protobuf Schema Definition
- [ ] gRPC Server Implementation
- [ ] Client SDK Generation (C++, Python, Go, JavaScript)
- [ ] Performance Benchmarks

### Sprint 5 (Woche 7): WebSocket
- [ ] Boost.Beast WebSocket Handler
- [ ] CDC/Changefeed Integration
- [ ] Subscription Management
- [ ] Testing

### Sprint 6-8 (Wochen 8-14): PostgreSQL Wire Protocol (MVP)
- [ ] Protocol Parser (Frontend/Backend Messages)
- [ ] SQL → AQL Translator (Subset)
- [ ] Basic CRUD Operations (SELECT, INSERT, UPDATE, DELETE)
- [ ] Connection Pooling
- [ ] Tool Testing (pgAdmin, DBeaver)

---

## Kosten-Nutzen-Übersicht

| Protokoll | Aufwand | Nutzen | ROI | Empfehlung |
|-----------|---------|--------|-----|------------|
| HTTP/2 | 2-3 Wochen | Sehr hoch | ⭐⭐⭐⭐⭐ | ✅ Sofort |
| gRPC | 2-3 Wochen | Sehr hoch | ⭐⭐⭐⭐⭐ | ✅ Q1 2025 |
| WebSocket | 1 Woche | Hoch | ⭐⭐⭐⭐ | ✅ Q1 2025 |
| PostgreSQL Wire | 3 Monate | Sehr hoch | ⭐⭐⭐⭐ | ✅ Q3 2025 |
| MQTT | 4-6 Wochen | Mittel-Hoch* | ⭐⭐⭐ | ⚠️ On-Demand |
| HTTP/3 | 6-8 Wochen | Variabel | ⭐⭐ | ⚠️ Q4 2025 |
| GraphQL Subscriptions | 2 Wochen | Mittel | ⭐⭐ | ⚠️ Optional |
| Kafka Protocol | 3-6 Monate | Niedrig | ⭐ | ❌ Nein |
| Redis Protocol | 1-2 Wochen | Niedrig | ⭐ | ❌ Nein |
| Native UDP | 2-4 Wochen | Sehr niedrig | ⭐ | ❌ Nein |
| SCTP | 4-6 Wochen | Sehr niedrig | ⭐ | ❌ Nein |

*MQTT-Nutzen ist hoch NUR für IoT-Deployments

---

## Ressourcen-Anforderungen (Gesamt)

### Entwickler-Wochen (1 Jahr)

| Phase | Protokolle | Wochen | Kosten* |
|-------|-----------|--------|---------|
| Q1 2025 | HTTP/2 + gRPC + WebSocket | 6-7 | €45K-52K |
| Q2-Q3 2025 | PostgreSQL Wire (MVP) | 12 | €90K |
| Q4 2025 | HTTP/3 (optional) + Polish | 8 | €60K |
| **Total** | **Alle empfohlenen Protokolle** | **26-27** | **€195K-202K** |

*Annahme: €7.5K pro Developer-Woche (inkl. Overhead)

### Infrastruktur

| Resource | Zusätzlicher Bedarf |
|----------|---------------------|
| RAM | +500 MB @ 10K connections (alle Protokolle) |
| CPU | +15-20% (gRPC + HTTP/2 + WebSocket) |
| Disk | +50 MB (Binaries) |
| Ports | +3 (gRPC: 9090, PostgreSQL: 5432, MQTT*: 1883) |

---

## Security-Überlegungen

### Alle Protokolle

1. **TLS Mandatory**
   - Alle Protokolle MÜSSEN TLS 1.3 unterstützen
   - **Exception:** Lokale Development (localhost)

2. **Rate Limiting**
   - Pro Protokoll separate Limits
   - **Reason:** gRPC kann höhere Last erzeugen als REST

3. **Authentication**
   - Unified Auth-Middleware für alle Protokolle
   - **Implementation:** JWT-Tokens funktionieren überall

### Protokoll-spezifisch

| Protokoll | Security-Risiko | Mitigation |
|-----------|-----------------|------------|
| gRPC | Reflection-API kann Schema leaken | Nur in Dev aktivieren |
| WebSocket | Connection Exhaustion | Max 1000 Connections/IP |
| PostgreSQL | SQL Injection | Prepared Statements + Validation |
| MQTT | Topic Flooding | ACL + Topic Limits |

---

## Monitoring & Observability

### Metrics erforderlich (pro Protokoll)

```
themis_protocol_requests_total{protocol="grpc|rest|websocket|postgres"}
themis_protocol_latency_seconds{protocol="..."}
themis_protocol_connections_active{protocol="..."}
themis_protocol_errors_total{protocol="...", error_type="..."}
```

### Tracing

- OpenTelemetry-Integration für alle Protokolle
- Distributed Tracing über Protokoll-Grenzen hinweg
- **Example:** REST → gRPC → ThemisDB Core (single Trace)

---

## Fazit

### Empfohlene Protokolle (nach Priorität)

1. **HTTP/2** - Sofort (bereits in Arbeit) ✅
2. **gRPC** - Q1 2025 ✅
3. **WebSocket** - Q1 2025 ✅
4. **PostgreSQL Wire Protocol** - Q3 2025 ⭐
5. **HTTP/3** - Q4 2025 (nur bei Bedarf) ⚠️

### Nicht empfohlene Protokolle

- Kafka Protocol (zu komplex, falscher Use Case)
- Redis Protocol (geringer Nutzen)
- Native UDP (redundant)
- SCTP (zu wenig verbreitet)

### Conditional (abhängig von Use Case)

- **MQTT** - Nur für IoT-Deployments
- **GraphQL Subscriptions** - Nur wenn GraphQL stark genutzt wird

---

## Referenzen

- [gRPC Official](https://grpc.io/)
- [WebSocket RFC 6455](https://datatracker.ietf.org/doc/html/rfc6455)
- [MQTT Specification](https://mqtt.org/mqtt-specification/)
- [PostgreSQL Frontend/Backend Protocol](https://www.postgresql.org/docs/current/protocol.html)
- [GraphQL over WebSocket Protocol](https://github.com/enisdenjo/graphql-ws/blob/master/PROTOCOL.md)
