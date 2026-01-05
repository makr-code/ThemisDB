# ThemisDB MINIMAL Edition

**Version:** 1.3.5+  
**Status:** Produktionsreif  
**Lizenz:** MIT (Open Source)

---

## 📋 Übersicht

ThemisDB MINIMAL Edition ist die leichtgewichtigste Variante von ThemisDB, entwickelt für:
- **Eingebettete Systeme** mit begrenzten Ressourcen
- **Edge-Deployments**, wo Größe wichtig ist
- **Entwicklungsumgebungen**, die schnelle Builds benötigen
- **Lernen und Experimentieren** ohne Komplexität
- **Microservices**, die nur Kern-Datenbankfunktionen benötigen

### Was ist enthalten

✅ **Kern-Datenbankfunktionen:**
- ACID-Transaktionen mit MVCC (Multi-Version Concurrency Control)
- Multi-Model-Speicher (relational, Graph, Vektor, Dokument)
- Sekundärindizes mit automatischer Wartung
- Basis-Query-Engine mit AQL (Advanced Query Language)
- Zeitreihen-Unterstützung
- REST API (HTTP/1.1)
- GraphQL API (Basis)
- Basis-Backup und -Wiederherstellung

### Was ist NICHT enthalten

❌ **Erweiterte Funktionen (NICHT in MINIMAL):**
- LLM-Integration (llama.cpp)
- GPU-Beschleunigung (CUDA, Vulkan, HIP, etc.)
- Horizontales Sharding
- Replikation (Leader-Follower, Multi-Master)
- Erweiterte Protokolle (HTTP/2, WebSocket, gRPC, MQTT, PostgreSQL Wire, MCP)
- Sprachassistent (STT/TTS)
- Content-Prozessoren (Audio, Bild, Video, CAD)
- OpenTelemetry Distributed Tracing
- RBAC (Role-Based Access Control)
- Feldverschlüsselung
- HSM (Hardware Security Module) Unterstützung

---

## 🎯 Anwendungsfälle

### Perfekt für:

1. **Eingebettete Systeme**
   - Raspberry Pi, QNAP NAS, Synology NAS
   - IoT-Gateways mit begrenztem RAM
   - Edge-Geräte in industriellen Umgebungen

2. **Entwicklung & Testing**
   - Schnelle lokale Entwicklungsumgebung
   - CI/CD-Pipeline-Tests
   - Schnelles Prototyping

3. **Microservices**
   - Single-Purpose-Services, die nur Datenbank benötigen
   - Sidecar-Datenbank für zustandsbehaftete Microservices
   - Containerisierte Anwendungen

4. **Lernen & Bildung**
   - Erkundung der Datenbank-Interna
   - Multi-Model-Datenbankkonzepte
   - MVCC und Transaktionsisolation

### Nicht geeignet für:

- AI/ML-Workloads mit LLM-Integration → Verwenden Sie Community Edition
- GPU-beschleunigte Vektorsuche → Verwenden Sie Community Edition
- Verteilte Deployments → Verwenden Sie Enterprise Edition
- Echtzeit-Analytik mit CEP → Verwenden Sie Enterprise Edition
- Anwendungen mit erweiterten Protokollen → Verwenden Sie Community Edition

---

## 📦 Installation

### Docker (Empfohlen)

```bash
# Von Docker Hub pullen (wenn verfügbar)
docker pull themisdb/themisdb:minimal

# Oder aus Quellcode bauen
docker build -f Dockerfile.minimal -t themisdb:minimal .

# Ausführen
docker run -d \
  --name themis-minimal \
  -p 8080:8080 \
  -v themis_data:/data \
  themisdb:minimal

# Verifizieren
curl http://localhost:8080/health
```

### Aus Quellcode (Linux/WSL)

```bash
# Repository klonen
git clone https://github.com/makr-code/ThemisDB.git
cd ThemisDB

# MINIMAL Edition bauen (schnell: ~5-10 Minuten)
./scripts/build-minimal.sh

# Server starten
./build-minimal/themis_server --config config/config-minimal.yaml
```

### Binärgröße im Vergleich

| Edition | Binärgröße | Build-Zeit | RAM-Nutzung (Idle) |
|---------|------------|------------|--------------------|
| **MINIMAL** | ~30-50 MB | ~5-10 min | ~100-200 MB |
| **COMMUNITY** | ~80-150 MB | ~20-30 min | ~300-500 MB |
| **ENTERPRISE** | ~150-250 MB | ~30-40 min | ~500-800 MB |

*(Größen sind ungefähre Werte und variieren je nach Plattform)*

---

## 🚀 Schnellstart

### 1. Server starten

```bash
# Mit Docker
docker run -d -p 8080:8080 -v themis_data:/data themisdb:minimal

# Oder aus Quellcode
./build-minimal/themis_server --config config/config-minimal.yaml
```

### 2. Health-Check durchführen

```bash
curl http://localhost:8080/health
```

Erwartete Antwort:
```json
{
  "status": "healthy",
  "edition": "MINIMAL",
  "version": "1.3.5",
  "uptime_seconds": 10
}
```

### 3. Erste Entity erstellen

```bash
curl -X PUT http://localhost:8080/entities/users:alice \
  -H "Content-Type: application/json" \
  -d '{"blob":"{\"name\":\"Alice\",\"age\":30,\"city\":\"Berlin\"}"}'
```

### 4. Index erstellen

```bash
curl -X POST http://localhost:8080/index/create \
  -H "Content-Type: application/json" \
  -d '{"table":"users","column":"city"}'
```

### 5. Daten abfragen

```bash
curl -X POST http://localhost:8080/query \
  -H "Content-Type: application/json" \
  -d '{
    "table": "users",
    "predicates": [{"column": "city", "value": "Berlin"}],
    "return": "entities"
  }'
```

---

## 🔧 Konfiguration

Die MINIMAL Edition verwendet eine vereinfachte Konfigurationsdatei: `config/config-minimal.yaml`

Wichtige Einstellungen:

```yaml
storage:
  rocksdb_path: "./data/rocksdb"
  memtable_size_mb: 64      # Reduziert für minimalen Footprint
  block_cache_size_mb: 256  # Reduziert für minimalen Footprint

network:
  http_port: 8080           # Nur HTTP/1.1

performance:
  worker_threads: 4         # Reduziert für minimalen Footprint
```

Siehe [`config/config-minimal.yaml`](../config/config-minimal.yaml) für vollständige Referenz.

---

## 📊 Performance-Charakteristika

### Benchmarks (Minimal Edition auf Standard-Hardware)

| Operation | Durchsatz | Latenz | Hinweise |
|-----------|-----------|--------|----------|
| Entity PUT | ~20.000 ops/s | 0,05 ms | Schreibdurchsatz |
| Entity GET | ~60.000 ops/s | 0,015 ms | Lesedurchsatz |
| Indizierte Abfrage | ~500k Abfragen/s | 2 μs | AQL WHERE-Klausel |
| Graph-Traversierung | ~2M ops/s | 0,5 μs | BFS (Tiefe=3) |
| Vektorsuche (CPU) | ~100k Abfragen/s | 10 μs | 384D-Embeddings |

*Hardware: 4-Kern-CPU, 8 GB RAM, SSD-Speicher*

### Ressourcennutzung

- **RAM (Idle):** ~100-200 MB
- **RAM (Aktiv):** ~300-500 MB unter Last
- **Disk (Installation):** ~50-80 MB
- **CPU (Idle):** <1%
- **CPU (Aktiv):** Skaliert mit Workload

---

## 🔄 Upgrade-Pfad

Benötigen Sie mehr Funktionen? Upgrade auf eine höhere Edition:

### Auf Community Edition
```bash
# Mit Community Edition neu bauen
cmake -DTHEMIS_EDITION=COMMUNITY ...
```

**Fügt hinzu:**
- LLM-Integration (optional)
- GPU-Beschleunigung (optional)
- Erweiterte Protokolle (HTTP/2, WebSocket, gRPC)
- Content-Prozessoren
- OpenTelemetry Tracing

### Auf Enterprise Edition
```bash
# Kontaktieren Sie sales@themisdb.com für Lizenz
cmake -DTHEMIS_EDITION=ENTERPRISE ...
```

**Fügt hinzu (zusätzlich zu Community):**
- Horizontales Sharding
- Replikation (Leader-Follower, Multi-Master)
- RBAC und erweiterte Sicherheit
- OLAP und CEP
- 24/7-Support

---

## 🛠️ Build-Optionen

### CMake-Konfiguration

```bash
cmake -S . -B build-minimal \
  -DTHEMIS_EDITION=MINIMAL \
  -DTHEMIS_ENABLE_LLM=OFF \
  -DTHEMIS_ENABLE_GPU=OFF \
  -DTHEMIS_ENABLE_GRPC=OFF \
  -DTHEMIS_ENABLE_HTTP2=OFF \
  -DTHEMIS_ENABLE_WEBSOCKET=OFF \
  -DTHEMIS_ENABLE_CONTENT_PROCESSORS=OFF \
  -DTHEMIS_ENABLE_TRACING=OFF \
  -DTHEMIS_BUILD_TESTS=OFF \
  -DTHEMIS_BUILD_BENCHMARKS=OFF
```

Alle MINIMAL-spezifischen Optionen werden automatisch gesetzt, wenn `THEMIS_EDITION=MINIMAL` verwendet wird.

### Benutzerdefinierter Minimal-Build

Sie können weiter anpassen:

```bash
# Tests für Entwicklung aktivieren
cmake -DTHEMIS_EDITION=MINIMAL -DTHEMIS_BUILD_TESTS=ON ...

# Tracing für Debugging aktivieren
cmake -DTHEMIS_EDITION=MINIMAL -DTHEMIS_ENABLE_TRACING=ON ...
```

---

## 📚 API-Referenz

MINIMAL Edition unterstützt eine Teilmenge der vollständigen ThemisDB-API:

### Unterstützte Endpoints

- ✅ `GET /health` - Health-Check
- ✅ `GET /metrics` - Basis-Prometheus-Metriken
- ✅ `PUT /entities/{key}` - Entity erstellen/aktualisieren
- ✅ `GET /entities/{key}` - Entity abrufen
- ✅ `DELETE /entities/{key}` - Entity löschen
- ✅ `POST /query` - Abfrage mit AQL
- ✅ `POST /index/create` - Index erstellen
- ✅ `POST /index/drop` - Index löschen
- ✅ `POST /transaction/begin` - Transaktion beginnen
- ✅ `POST /transaction/commit` - Transaktion committen
- ✅ `POST /transaction/rollback` - Transaktion zurückrollen
- ✅ `POST /graphql` - GraphQL-Endpoint (Basis)

### NICHT unterstützt in MINIMAL

- ❌ LLM-Endpoints (`/llm/*`)
- ❌ Sharding-Endpoints (`/shard/*`)
- ❌ Replikations-Endpoints (`/replication/*`)
- ❌ Erweiterte Protokoll-Endpoints
- ❌ Content-Processing-Endpoints

Siehe [API-Dokumentation](../docs/api/api_reference.md) für Details.

---

## 🔒 Sicherheit

MINIMAL Edition enthält grundlegende Sicherheitsfunktionen:

- ✅ TLS/SSL-Unterstützung (optional)
- ✅ Basis-Authentifizierung (optional)
- ✅ Audit-Logging
- ❌ RBAC (verwenden Sie Community+ für RBAC)
- ❌ Feldverschlüsselung (verwenden Sie Enterprise)
- ❌ HSM-Integration (verwenden Sie Enterprise)

Für Produktions-Deployments, aktivieren Sie TLS:

```yaml
security:
  enable_tls: true
  tls_cert: "/path/to/cert.pem"
  tls_key: "/path/to/key.pem"
```

---

## 🐛 Fehlerbehebung

### Build-Probleme

**Problem:** CMake findet vcpkg nicht  
**Lösung:**
```bash
export VCPKG_ROOT=$HOME/vcpkg
./scripts/build-minimal.sh
```

**Problem:** Out of Memory während des Builds  
**Lösung:**
```bash
NUM_JOBS=2 ./scripts/build-minimal.sh  # Parallele Jobs reduzieren
```

### Laufzeit-Probleme

**Problem:** Server startet nicht  
**Lösung:**
```bash
# Konfiguration prüfen
./build-minimal/themis_server --config config/config-minimal.yaml --validate

# Logs prüfen
./build-minimal/themis_server --config config/config-minimal.yaml --log-level debug
```

**Problem:** Port 8080 bereits in Verwendung  
**Lösung:**
```yaml
# config/config-minimal.yaml bearbeiten
network:
  http_port: 8081  # Anderen Port verwenden
```

---

## 📖 Weitere Ressourcen

- [ThemisDB-Dokumentation](https://makr-code.github.io/ThemisDB/)
- [Architektur-Übersicht](../docs/architecture/ARCHITECTURE_OVERVIEW.md)
- [API-Referenz](../docs/api/api_reference.md)
- [GitHub Issues](https://github.com/makr-code/ThemisDB/issues)
- [Beitragsleitfaden](../CONTRIBUTING.md)

---

## 📄 Lizenz

ThemisDB MINIMAL Edition wird unter der **MIT-Lizenz** veröffentlicht.

- ✅ Kostenlos zu verwenden, zu modifizieren und zu verteilen
- ✅ Kommerzielle Nutzung erlaubt
- ✅ Keine Namensnennung erforderlich (aber geschätzt!)

Siehe [LICENSE](../LICENSE) für Details.

---

## 🙏 Danksagungen

MINIMAL Edition basiert auf denselben Kerntechnologien wie das vollständige ThemisDB:

- **RocksDB** - LSM-Tree-Speicher-Engine
- **nlohmann/json** - JSON-Parsing
- **OpenSSL** - TLS/SSL-Unterstützung
- **vcpkg** - Paketverwaltung

Siehe [ATTRIBUTIONS.md](../ATTRIBUTIONS.md) für vollständige Liste.

---

**Mit ❤️ für eingebettete und Edge-Computing entwickelt**

[⭐ Star auf GitHub](https://github.com/makr-code/ThemisDB) · [🐛 Probleme melden](https://github.com/makr-code/ThemisDB/issues) · [📖 Docs lesen](https://makr-code.github.io/ThemisDB/)
