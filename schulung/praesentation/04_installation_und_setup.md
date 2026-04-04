---
marp: true
theme: default
paginate: true
backgroundColor: '#ffffff'
header: 'ThemisDB Schulung'
footer: '© ThemisDB – Modul 4: Installation & Setup'
style: |
  section {
    font-family: 'Segoe UI', Arial, sans-serif;
  }
  h1 { color: #1a73e8; }
  h2 { color: #333; border-bottom: 2px solid #1a73e8; padding-bottom: 8px; }
  code { background: #f5f5f5; padding: 2px 6px; border-radius: 4px; }
  pre { background: #1e1e1e; color: #d4d4d4; }
---

# ThemisDB
## Modul 4: Installation & Setup

**Schulungsversion 1.0 · Niveau: Einsteiger**

---

## Agenda

1. Systemvoraussetzungen
2. Docker — Schnellstart
3. Docker Compose — Produktionssetup
4. Konfiguration
5. Python-Client einrichten
6. Erste Verbindung testen
7. ThemisDB Shell (AQL REPL)
8. Monitoring & Health Checks
9. Häufige Installationsprobleme

---

## Systemvoraussetzungen

### Minimal (Entwicklung)
| Ressource | Minimum | Empfohlen |
|---|---|---|
| CPU | 2 Kerne | 4+ Kerne |
| RAM | 2 GB | 8+ GB |
| Disk | 5 GB | 50+ GB (SSD) |
| OS | Linux, macOS, Windows | Linux (Ubuntu 22.04+) |

### Abhängigkeiten
- **Docker** 24.0+ oder **Docker Desktop**
- **Python** 3.8+ (für Client-Bibliothek)
- **curl** (für Health Checks)

---

## Docker — Schnellstart

```bash
# 1. Image herunterladen und starten
docker run -d \
  --name themisdb \
  -p 8080:8080 \   # HTTP API
  -p 18765:18765 \ # AQL Shell / Wire Protocol
  -v themisdb-data:/var/lib/themisdb \
  themisdb/themisdb:latest

# 2. Logs prüfen
docker logs -f themisdb

# 3. Health Check
curl http://localhost:8080/health
# Antwort: {"status":"ok","version":"1.8.0"}

# 4. Stoppen und starten
docker stop themisdb
docker start themisdb
```

---

## Docker Compose — Produktionssetup

```yaml
# docker-compose.yml
version: '3.9'

services:
  themisdb:
    image: themisdb/themisdb:latest
    restart: unless-stopped
    ports:
      - "8080:8080"
      - "18765:18765"
    volumes:
      - themisdb-data:/var/lib/themisdb
      - ./config/themisdb.json:/etc/themisdb/config.json:ro
    environment:
      THEMIS_LOG_LEVEL: info
      THEMIS_MAX_MEMORY: 4096
    healthcheck:
      test: ["CMD", "curl", "-f", "http://localhost:8080/health"]
      interval: 30s
      timeout: 10s
      retries: 3

volumes:
  themisdb-data:
```

```bash
docker compose up -d
```

---

## Konfigurationsdatei

```json
{
  "server": {
    "host": "0.0.0.0",
    "port": 8080,
    "tls": {
      "enabled": false,
      "cert_path": "/etc/ssl/certs/themisdb.crt",
      "key_path":  "/etc/ssl/private/themisdb.key"
    }
  },
  "storage": {
    "data_dir": "/var/lib/themisdb/data",
    "wal_dir":  "/var/lib/themisdb/wal",
    "cache_mb":  512
  },
  "auth": {
    "enabled":      false,
    "jwt_secret":   "CHANGE_ME_IN_PRODUCTION",
    "token_expiry": "24h"
  },
  "log": {
    "level": "info",
    "format": "json"
  }
}
```

---

## Python-Client installieren

```bash
# Installation via pip
pip install themis-client

# Oder aus dem Quellcode (für Entwicklung)
git clone https://github.com/makr-code/ThemisDB.git
cd ThemisDB/examples/01_hello_world
pip install -r requirements.txt
```

```python
# Verbindung herstellen
from themis_client import ThemisClient

client = ThemisClient(
    base_url="http://localhost:8080",
    timeout=30,
    # auth_token="your-jwt-token"  # Wenn Auth aktiviert
)

# Verbindung testen
status = client.health()
print(status)  # {'status': 'ok', 'version': '1.8.0'}
```

---

## Erste AQL-Abfrage

```python
from themis_client import ThemisClient

client = ThemisClient("http://localhost:8080")

# Collection erstellen
client.query("CREATE COLLECTION IF NOT EXISTS demo")

# Daten einfügen
client.query("""
  FOR i IN 1..5
    INSERT {
      number: i,
      label:  CONCAT("Item ", i),
      value:  RAND() * 100
    } INTO demo
""")

# Daten abfragen
result = client.query("""
  FOR d IN demo
    SORT d.number ASC
    RETURN d
""")

for doc in result:
    print(doc)
```

---

## ThemisDB Shell (AQL REPL)

```bash
# Shell im Container starten
docker exec -it themisdb themis-shell

# Direkt mit Verbindungsstring
themis-shell --host localhost --port 18765
```

```
ThemisDB Shell v1.8.0
Type 'help' for available commands, 'exit' to quit.

themisdb> FOR d IN demo RETURN d
[
  { "_key": "1", "number": 1, "label": "Item 1", "value": 42.3 },
  ...
]
3 documents (12ms)

themisdb> db stats
Collections: 1
Documents:   5
Memory:      32 MB

themisdb> exit
```

---

## HTTP API — direkte Abfragen

```bash
# AQL Query via HTTP POST
curl -X POST http://localhost:8080/api/v1/query \
  -H "Content-Type: application/json" \
  -d '{
    "query": "FOR d IN demo RETURN d",
    "bindVars": {}
  }'

# Mit Bind-Variablen
curl -X POST http://localhost:8080/api/v1/query \
  -H "Content-Type: application/json" \
  -d '{
    "query": "FOR u IN users FILTER u.age > @min_age RETURN u",
    "bindVars": { "min_age": 18 }
  }'

# Collection-Info
curl http://localhost:8080/api/v1/collections/demo
```

---

## Monitoring & Health Checks

```bash
# Health Check Endpoint
curl http://localhost:8080/health
# {"status":"ok","version":"1.8.0","uptime":3600}

# Metrics (Prometheus-Format)
curl http://localhost:8080/metrics
# themisdb_queries_total{status="success"} 1234
# themisdb_query_duration_seconds_p99 0.003
# themisdb_memory_used_bytes 33554432

# Stats Endpoint
curl http://localhost:8080/api/v1/stats
```

**Prometheus + Grafana** Integration:
```yaml
# prometheus.yml Scrape-Config
- job_name: themisdb
  static_configs:
    - targets: ['themisdb:8080']
  metrics_path: /metrics
```

---

## Häufige Probleme & Lösungen

| Problem | Ursache | Lösung |
|---|---|---|
| `Connection refused` | Server läuft nicht | `docker start themisdb` |
| `Port already in use` | Port belegt | `docker run -p 8181:8080 ...` |
| `Out of memory` | RAM zu klein | `THEMIS_MAX_MEMORY` erhöhen |
| `Permission denied` | Volume-Rechte | `chmod 755 /data/dir` |
| `TLS handshake failed` | Zertifikat abgelaufen | Zertifikat erneuern |

```bash
# Diagnosebefehle
docker logs themisdb --tail=50
docker inspect themisdb | grep -A5 '"State"'
docker stats themisdb
```

---

## Nächste Schritte nach der Installation

1. **Erstes Projekt**: `examples/01_grundlegende_operationen/` durcharbeiten
2. **Datenmodell** planen (welches Modell für welche Daten?)
3. **Auth aktivieren** für Produktionsumgebungen
4. **Backup einrichten**: Regelmäßige Snapshots konfigurieren
5. **Monitoring** mit Prometheus + Grafana aufsetzen

```bash
# Backup erstellen
curl -X POST http://localhost:8080/api/v1/backup \
  -d '{"path": "/backups/themisdb-2025-01-01"}'

# Backup wiederherstellen
curl -X POST http://localhost:8080/api/v1/restore \
  -d '{"path": "/backups/themisdb-2025-01-01"}'
```

---

## Zusammenfassung Modul 4

✅ ThemisDB läuft in **Docker** mit einem einzigen Befehl

✅ **docker-compose** für Produktionsdeployments

✅ **JSON-Konfiguration** für alle Server-Einstellungen

✅ **Python-Client** für Anwendungsintegration

✅ **HTTP API** für direkte Abfragen

✅ **Health Checks & Metrics** für Monitoring

---

## 📚 Weiterführend

- Dokument: `dokumente/01_quickstart_guide.md`
- Beispiele: `examples/01_grundlegende_operationen/`
- **Modul 5**: Anwendungsbeispiele & Best Practices
- [Offizielles Setup-Handbuch](../SETUP.md)
