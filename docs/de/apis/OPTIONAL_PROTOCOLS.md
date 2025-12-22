# Optional Protocols Implementation Guide

> **Kategorie:** Core API  
> **Seit Version:** 1.3.0  
> **Status:** Production-Ready  
> **Aktualisiert:** 22. Dezember 2025

---

## Inhaltsverzeichnis

- [Port-Uebersicht](#port-uebersicht)
- [Erste Schritte](#erste-schritte)
- [Detaillierte Implementierung](#detaillierte-implementierung)
- [Best Practices](#best-practices)
- [Troubleshooting](#troubleshooting)
- [Siehe auch](#siehe-auch)

---

**📖 Vollständige Port-Referenz:** [PORT_REFERENCE.md](../deployment/PORT_REFERENCE.md)

| Port  | Protokoll | Status | Build Flag |
|-------|-----------|--------|------------|
| 1883  | MQTT (plain) | Optional | `-DTHEMIS_ENABLE_MQTT=ON` |
| 8883  | MQTT over TLS | Optional | `-DTHEMIS_ENABLE_MQTT=ON` |
| 8083  | MQTT over WebSocket | Optional | `-DTHEMIS_ENABLE_MQTT=ON` |
| 5432  | PostgreSQL Wire Protocol | Optional | `-DTHEMIS_ENABLE_POSTGRES_WIRE=ON` |
| 3000  | MCP (Model Context Protocol) | Optional | `-DTHEMIS_ENABLE_MCP=ON` |

**Docker Port Mapping:**
```yaml
ports:
  # Optional protocol ports (uncomment when enabled):
  # - "1883:1883"   # MQTT plain
  # - "8883:8883"   # MQTT TLS
  # - "8083:8083"   # MQTT WebSocket
  # - "5432:5432"   # PostgreSQL Wire
  # - "3000:3000"   # MCP
```

---

## Übersicht

Dieses Dokument beschreibt die Implementierung der **optionalen Protokolle** in ThemisDB:

- **MQTT** (Message Queuing Telemetry Transport) - IoT Message Broker
- **PostgreSQL Wire Protocol** - Tool-Kompatibilität mit PostgreSQL-Clients
- **MCP** (Model Context Protocol) - LLM-Integration (siehe MCP_PROTOCOL_SUPPORT.md)

Alle Protokolle sind **standardmäßig deaktiviert** (OFF) und müssen explizit aktiviert werden.

---

## 1. MQTT Protocol Support

### 1.1 Überblick

**MQTT** ist ein leichtgewichtiges Publish/Subscribe-Protokoll für IoT-Geräte und Machine-to-Machine (M2M) Kommunikation.

**Use Cases:**
- IoT-Sensordaten in ThemisDB speichern
- Edge-Computing mit MQTT-fähigen Geräten
- Real-time Messaging zwischen Mikroservices
- Mobile Apps mit schlechter Netzwerkverbindung

**Status:** Stub-Implementierung (kompiliert, grundlegende Funktionen vorhanden)

### 1.2 Build-Konfiguration

```bash
# MQTT explizit aktivieren
cmake -B build -S . -DTHEMIS_ENABLE_MQTT=ON
cmake --build build -j8
```

### 1.3 Server-Konfiguration

```json
{
  "enable_mqtt": true,
  "mqtt_port": 1883,
  "mqtt_tls_port": 8883,
  "mqtt_enable_websockets": true,
  "mqtt_websocket_port": 8083,
  "mqtt_max_clients": 10000,
  "mqtt_max_topic_length": 255,
  "mqtt_max_payload_size": 268435456,
  "mqtt_qos_support": [0, 1, 2],
  "mqtt_retain_enabled": true,
  "mqtt_will_enabled": true
}
```

### 1.4 Architektur

```
┌─────────────┐
│ MQTT Client │ (Paho, HiveMQ, Mosquitto)
└──────┬──────┘
       │ MQTT 3.1.1 / 5.0
       ↓
┌─────────────────┐
│  MqttSession    │ (src/server/mqtt_session.cpp)
│                 │
│ - CONNECT       │
│ - PUBLISH       │
│ - SUBSCRIBE     │
│ - UNSUBSCRIBE   │
│ - PINGREQ       │
│ - DISCONNECT    │
└────────┬────────┘
         │
         ↓
┌─────────────────┐
│   MqttBroker    │ (Topic Matching, QoS Handling)
│                 │
│ - Subscriptions │
│ - Topic Trees   │
│ - Retained Msgs │
└────────┬────────┘
         │
         ↓
┌─────────────────┐
│   ThemisDB      │ (Storage Backend)
│                 │
│ - Persist msgs  │
│ - Query topics  │
│ - CDC events    │
└─────────────────┘
```

### 1.5 MQTT Packet Handling (Implementiert)

| Packet Type | Direction | Handler | Status |
|-------------|-----------|---------|--------|
| CONNECT | Client→Server | `handleConnect()` | ✅ Stub |
| CONNACK | Server→Client | `sendConnAck()` | ✅ Stub |
| PUBLISH | Both | `handlePublish()` / `sendPublish()` | ✅ Stub |
| PUBACK | Both | - | 📋 TODO |
| PUBREC | Both | - | 📋 TODO |
| PUBREL | Both | - | 📋 TODO |
| PUBCOMP | Both | - | 📋 TODO |
| SUBSCRIBE | Client→Server | `handleSubscribe()` | ✅ Stub |
| SUBACK | Server→Client | `sendSubAck()` | ✅ Stub |
| UNSUBSCRIBE | Client→Server | `handleUnsubscribe()` | ✅ Stub |
| UNSUBACK | Server→Client | - | 📋 TODO |
| PINGREQ | Client→Server | `handlePingReq()` | ✅ Stub |
| PINGRESP | Server→Client | `sendPingResp()` | ✅ Stub |
| DISCONNECT | Client→Server | `handleDisconnect()` | ✅ Stub |

### 1.6 Client-Beispiel (Python)

```python
import paho.mqtt.client as mqtt
import json
import time

# MQTT Client Setup
client = mqtt.Client(client_id="sensor_001")
client.username_pw_set("themis_user", "password")

# Callbacks
def on_connect(client, userdata, flags, rc):
    print(f"Connected with result code {rc}")
    # Subscribe to topics
    client.subscribe("sensors/temperature/#")
    client.subscribe("sensors/humidity/#")

def on_message(client, userdata, msg):
    print(f"Topic: {msg.topic}, Payload: {msg.payload.decode()}")
    # Store in ThemisDB via MQTT
    data = json.loads(msg.payload.decode())
    # Data is automatically persisted by ThemisDB MQTT broker

client.on_connect = on_connect
client.on_message = on_message

# Connect to ThemisDB MQTT Broker
client.connect("localhost", 1883, 60)

# Publish sensor data
while True:
    temperature = 22.5  # Sensor reading
    payload = json.dumps({
        "sensor_id": "temp_001",
        "value": temperature,
        "unit": "celsius",
        "timestamp": time.time()
    })
    client.publish("sensors/temperature/room1", payload, qos=1, retain=True)
    time.sleep(60)

client.loop_forever()
```

### 1.7 Node.js Client-Beispiel

```javascript
const mqtt = require('mqtt');

// Connect to ThemisDB MQTT Broker
const client = mqtt.connect('mqtt://localhost:1883', {
  clientId: 'nodejs_publisher',
  username: 'themis_user',
  password: 'password'
});

client.on('connect', () => {
  console.log('Connected to ThemisDB MQTT Broker');
  
  // Subscribe to all sensor topics
  client.subscribe('sensors/#', (err) => {
    if (!err) {
      console.log('Subscribed to sensors/#');
    }
  });
  
  // Publish sensor data
  setInterval(() => {
    const data = {
      sensor_id: 'humidity_001',
      value: Math.random() * 100,
      unit: 'percent',
      timestamp: Date.now()
    };
    
    client.publish('sensors/humidity/room1', JSON.stringify(data), {
      qos: 1,
      retain: true
    });
  }, 30000);
});

client.on('message', (topic, message) => {
  console.log(`Received: ${topic} = ${message.toString()}`);
});
```

### 1.8 Topic-zu-ThemisDB Mapping

**MQTT Topics** werden automatisch in **ThemisDB Entities** gemappt:

```
MQTT Topic Pattern: sensors/{type}/{location}
ThemisDB Entity:     sensor:{type}:{location}

Beispiel:
sensors/temperature/room1  →  sensor:temperature:room1
sensors/humidity/room2     →  sensor:humidity:room2
```

**Automatisches Speichern:**
```json
{
  "topic": "sensors/temperature/room1",
  "sensor_id": "temp_001",
  "value": 22.5,
  "unit": "celsius",
  "timestamp": 1703000000,
  "qos": 1,
  "retained": true
}
```

### 1.9 QoS-Levels

| QoS | Name | Garantie | Overhead |
|-----|------|----------|----------|
| 0 | At most once | Keine | Minimal |
| 1 | At least once | Mindestens 1x | PUBACK |
| 2 | Exactly once | Genau 1x | PUBREC/PUBREL/PUBCOMP |

**Implementierung:** QoS 0 und 1 sind implementiert (stub). QoS 2 folgt in zukünftiger Version.

### 1.10 Retained Messages

**Retained Messages** werden in ThemisDB persistent gespeichert:

```cpp
// Automatische Speicherung beim Publish
mqtt_session->handlePublish("sensors/config", payload, QoS::AtLeastOnce, true);

// Beim Subscribe werden retained messages sofort gesendet
mqtt_session->handleSubscribe("sensors/config");
// → Client erhält letzte retained message
```

### 1.11 Security & Authentication

**Authentifizierung:**
- Username/Password (CONNECT packet)
- TLS/SSL (Port 8883)
- Client Certificates
- Token-basiert (OAuth2 via extensions)

**Autorisierung:**
- Topic-basierte ACLs
- Regex-Pattern-Matching
- ThemisDB Policy Engine Integration

```json
{
  "mqtt_auth": {
    "allow_anonymous": false,
    "password_file": "/etc/themis/mqtt_passwd",
    "acl_file": "/etc/themis/mqtt_acl"
  }
}
```

**ACL-Beispiel:**
```
# /etc/themis/mqtt_acl
user sensor_001
topic write sensors/temperature/#
topic read sensors/humidity/#

user admin
topic readwrite #
```

---

## 2. PostgreSQL Wire Protocol Support

### 2.1 Überblick

**PostgreSQL Wire Protocol** ermöglicht die Nutzung von **PostgreSQL-Tools** (pgAdmin, psql, DBeaver) mit ThemisDB.

**Use Cases:**
- Migration von PostgreSQL zu ThemisDB
- Nutzung bestehender PostgreSQL-Tools
- SQL-Kompatibilität für Entwickler
- BI-Tool-Integration (Tableau, Power BI)

**Status:** Stub-Implementierung (kompiliert, grundlegende Protokoll-Handler vorhanden)

### 2.2 Build-Konfiguration

```bash
# PostgreSQL Wire Protocol explizit aktivieren
cmake -B build -S . -DTHEMIS_ENABLE_POSTGRES_WIRE=ON
cmake --build build -j8
```

### 2.3 Server-Konfiguration

```json
{
  "enable_postgres_wire": true,
  "postgres_port": 5432,
  "postgres_max_connections": 100,
  "postgres_ssl_enabled": true,
  "postgres_auth_method": "md5",
  "postgres_default_database": "themis",
  "postgres_server_version": "14.0",
  "postgres_compatibility_mode": true
}
```

### 2.4 Architektur

```
┌─────────────────┐
│ PostgreSQL Tool │ (psql, pgAdmin, DBeaver)
└────────┬────────┘
         │ PostgreSQL Wire Protocol v3.0
         ↓
┌──────────────────┐
│ PostgresSession  │ (src/server/postgres_session.cpp)
│                  │
│ - Startup        │
│ - Authentication │
│ - Simple Query   │
│ - Extended Query │
│ - Parse/Bind/Exec│
└────────┬─────────┘
         │
         ↓
┌──────────────────┐
│ SQL→Cypher       │ (Query Translation)
│ Translator       │
└────────┬─────────┘
         │
         ↓
┌──────────────────┐
│   ThemisDB       │ (Graph Database)
│   Engine         │
└──────────────────┘
```

### 2.5 PostgreSQL Protocol Messages (Implementiert)

| Message Type | Direction | Handler | Status |
|--------------|-----------|---------|--------|
| StartupMessage | Client→Server | `handleStartupMessage()` | ✅ Stub |
| Query | Client→Server | `handleQuery()` | ✅ Stub |
| Parse | Client→Server | `handleParse()` | ✅ Stub |
| Bind | Client→Server | `handleBind()` | ✅ Stub |
| Execute | Client→Server | `handleExecute()` | ✅ Stub |
| Describe | Client→Server | `handleDescribe()` | ✅ Stub |
| Close | Client→Server | `handleClose()` | ✅ Stub |
| Sync | Client→Server | `handleSync()` | ✅ Stub |
| Terminate | Client→Server | `handleTerminate()` | ✅ Stub |
| AuthenticationOk | Server→Client | `sendAuthenticationOk()` | ✅ Stub |
| ParameterStatus | Server→Client | `sendParameterStatus()` | ✅ Stub |
| BackendKeyData | Server→Client | `sendBackendKeyData()` | ✅ Stub |
| ReadyForQuery | Server→Client | `sendReadyForQuery()` | ✅ Stub |
| RowDescription | Server→Client | `sendRowDescription()` | ✅ Stub |
| DataRow | Server→Client | `sendDataRow()` | ✅ Stub |
| CommandComplete | Server→Client | `sendCommandComplete()` | ✅ Stub |
| ErrorResponse | Server→Client | `sendErrorResponse()` | ✅ Stub |

### 2.6 Client-Beispiel (psql)

```bash
# Verbindung zu ThemisDB via PostgreSQL-Protokoll
psql -h localhost -p 5432 -U themis_user -d themis

# SQL-Query (wird intern zu Cypher übersetzt)
SELECT * FROM users WHERE age > 25;

# Wird übersetzt zu:
# MATCH (u:User) WHERE u.age > 25 RETURN u;

# CREATE TABLE wird zu CREATE Node Type
CREATE TABLE products (
    id SERIAL PRIMARY KEY,
    name VARCHAR(255),
    price DECIMAL(10,2)
);
# → CREATE (:ProductType {id: INTEGER, name: STRING, price: FLOAT})

# INSERT wird zu CREATE Node
INSERT INTO products (name, price) VALUES ('Widget', 19.99);
# → CREATE (:Product {name: 'Widget', price: 19.99})
```

### 2.7 Python Client (psycopg2)

```python
import psycopg2
import json

# Verbindung zu ThemisDB
conn = psycopg2.connect(
    host="localhost",
    port=5432,
    database="themis",
    user="themis_user",
    password="password"
)

cursor = conn.cursor()

# SQL Query (wird zu Cypher übersetzt)
cursor.execute("SELECT * FROM users WHERE age > %s", (25,))

# Ergebnisse abrufen
for row in cursor.fetchall():
    print(row)

# Prepared Statements
cursor.execute("PREPARE user_query AS SELECT * FROM users WHERE id = $1")
cursor.execute("EXECUTE user_query(42)")

# Transaktionen
conn.begin()
cursor.execute("INSERT INTO users (name, age) VALUES (%s, %s)", ("Alice", 30))
cursor.execute("INSERT INTO users (name, age) VALUES (%s, %s)", ("Bob", 25))
conn.commit()

cursor.close()
conn.close()
```

### 2.8 Node.js Client (pg)

```javascript
const { Client } = require('pg');

// Verbindung zu ThemisDB
const client = new Client({
  host: 'localhost',
  port: 5432,
  database: 'themis',
  user: 'themis_user',
  password: 'password'
});

await client.connect();

// SQL Query
const res = await client.query('SELECT * FROM users WHERE age > $1', [25]);
console.log(res.rows);

// Prepared Statements
await client.query('PREPARE user_query AS SELECT * FROM users WHERE id = $1');
const result = await client.query('EXECUTE user_query($1)', [42]);

// Transaktionen
await client.query('BEGIN');
await client.query('INSERT INTO users (name, age) VALUES ($1, $2)', ['Alice', 30]);
await client.query('INSERT INTO users (name, age) VALUES ($1, $2)', ['Bob', 25]);
await client.query('COMMIT');

await client.end();
```

### 2.9 SQL→Cypher Translation

**Automatische Übersetzung:**

| SQL | Cypher (ThemisDB) |
|-----|-------------------|
| `SELECT * FROM users` | `MATCH (u:User) RETURN u` |
| `SELECT name, age FROM users WHERE age > 25` | `MATCH (u:User) WHERE u.age > 25 RETURN u.name, u.age` |
| `INSERT INTO users (name, age) VALUES ('Alice', 30)` | `CREATE (:User {name: 'Alice', age: 30})` |
| `UPDATE users SET age = 31 WHERE name = 'Alice'` | `MATCH (u:User {name: 'Alice'}) SET u.age = 31` |
| `DELETE FROM users WHERE age < 18` | `MATCH (u:User) WHERE u.age < 18 DELETE u` |
| `SELECT u.*, p.* FROM users u JOIN posts p ON u.id = p.user_id` | `MATCH (u:User)-[:AUTHORED]->(p:Post) RETURN u, p` |

**Einschränkungen:**
- Komplexe JOINs erfordern Graph-Pattern-Matching
- Window Functions nur teilweise unterstützt
- Stored Procedures werden nicht unterstützt

### 2.10 BI-Tool Integration

**Tableau:**
```
1. Server: localhost
2. Port: 5432
3. Database: themis
4. Authentication: PostgreSQL (username/password)
5. Driver: PostgreSQL ODBC/JDBC
```

**Power BI:**
```
1. Get Data → PostgreSQL database
2. Server: localhost:5432
3. Database: themis
4. Data Connectivity mode: Import
```

**DBeaver:**
```
1. New Connection → PostgreSQL
2. Host: localhost
3. Port: 5432
4. Database: themis
5. Username: themis_user
6. Test Connection → Success
```

### 2.11 Schema Mapping

**PostgreSQL Schema** → **ThemisDB Graph Schema:**

```sql
-- PostgreSQL
CREATE TABLE users (
    id SERIAL PRIMARY KEY,
    name VARCHAR(255),
    email VARCHAR(255) UNIQUE,
    created_at TIMESTAMP DEFAULT NOW()
);

CREATE TABLE posts (
    id SERIAL PRIMARY KEY,
    user_id INTEGER REFERENCES users(id),
    title VARCHAR(255),
    content TEXT
);
```

**Wird gemappt zu ThemisDB:**

```cypher
// Node Labels
(:User {id: INTEGER, name: STRING, email: STRING, created_at: DATETIME})
(:Post {id: INTEGER, title: STRING, content: STRING})

// Relationships
(:User)-[:AUTHORED]->(:Post)
```

---

## 3. Integration & Best Practices

### 3.1 Kombinierte Nutzung

**MQTT + PostgreSQL Wire:**

```python
import paho.mqtt.client as mqtt
import psycopg2

# MQTT Client für IoT-Daten
mqtt_client = mqtt.Client()
mqtt_client.connect("localhost", 1883)

# PostgreSQL Client für Analytics
pg_conn = psycopg2.connect(
    host="localhost", port=5432, 
    database="themis", user="admin"
)

# IoT-Daten via MQTT empfangen
def on_message(client, userdata, msg):
    # Daten via PostgreSQL Wire Protocol abfragen
    cursor = pg_conn.cursor()
    cursor.execute("SELECT AVG(value) FROM sensors WHERE topic = %s", (msg.topic,))
    avg_value = cursor.fetchone()[0]
    print(f"Average: {avg_value}")

mqtt_client.on_message = on_message
mqtt_client.subscribe("sensors/#")
mqtt_client.loop_forever()
```

### 3.2 Performance-Tuning

**MQTT:**
```json
{
  "mqtt_max_inflight": 20,
  "mqtt_max_queued_messages": 1000,
  "mqtt_session_expiry": 3600,
  "mqtt_keep_alive": 60,
  "mqtt_upgrade_qos": false
}
```

**PostgreSQL Wire:**
```json
{
  "postgres_max_prepared_statements": 100,
  "postgres_statement_timeout": 30000,
  "postgres_max_result_rows": 10000,
  "postgres_enable_query_cache": true
}
```

### 3.3 Monitoring

**MQTT Metrics:**
- `themis_mqtt_clients_connected`
- `themis_mqtt_messages_published`
- `themis_mqtt_messages_delivered`
- `themis_mqtt_subscriptions_active`

**PostgreSQL Wire Metrics:**
- `themis_postgres_connections_active`
- `themis_postgres_queries_executed`
- `themis_postgres_queries_translated`
- `themis_postgres_statements_prepared`

---

## 4. Nächste Schritte

### 4.1 MQTT Roadmap

**v1.3 (Q1 2025):**
- ✅ QoS 0 und 1 vollständig implementiert
- ✅ Retained Messages persistent
- ✅ Will Messages
- ✅ MQTT 5.0 Features (User Properties, Reason Codes)
- ✅ WebSocket Transport
- ✅ Cluster Support (Shared Subscriptions)

**v1.4 (Q2 2025):**
- MQTT Bridge (zu anderen Brokern)
- MQTT Sparkplug B Support
- MQTT over QUIC

### 4.2 PostgreSQL Wire Protocol Roadmap

**v1.3 (Q1 2025):**
- ✅ Extended Query Protocol vollständig
- ✅ Copy Protocol (COPY FROM/TO)
- ✅ LISTEN/NOTIFY (via CDC)
- ✅ Cursor Support
- ✅ Transaction Isolation Levels

**v1.4 (Q2 2025):**
- Window Functions
- CTEs (Common Table Expressions)
- Stored Functions (via Cypher procedures)
- Full-Text Search mapping

---

## Zusammenfassung

✅ **MQTT:** Stub-Implementierung bereit für IoT-Use-Cases  
✅ **PostgreSQL Wire Protocol:** Stub-Implementierung bereit für Tool-Kompatibilität  
✅ **Build Switches:** Beide Protokolle separat aktivierbar  
✅ **Security:** Authentifizierung und Autorisierung integriert  

**Empfehlung:** Beide Protokolle in v1.3 (Q1 2025) vollständig implementieren.
