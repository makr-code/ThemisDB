# ThemisDB Docker Environment Variables Reference

Diese Dokumentation beschreibt alle verfügbaren Umgebungsvariablen für den ThemisDB Docker Container, angelehnt an PostgreSQL Best Practices.

## Core Configuration

### `THEMIS_CONFIG_PATH`
- **Default:** `/etc/themis/config.json`
- **Beschreibung:** Pfad zur Konfigurationsdatei
- **Beispiel:** `THEMIS_CONFIG_PATH=/custom/config.json`

### `THEMIS_DATA_DIR`
- **Default:** `/var/lib/themisdb`
- **Beschreibung:** Hauptverzeichnis für alle Daten (ähnlich wie `PGDATA` bei PostgreSQL)
- **Beispiel:** `THEMIS_DATA_DIR=/var/lib/themisdb`

### `THEMIS_PORT`
- **Default:** `18765`
- **Beschreibung:** Port für Binary Wire Protocol und gRPC
- **Beispiel:** `THEMIS_PORT=8765`

### `THEMIS_HOST`
- **Default:** `0.0.0.0`
- **Beschreibung:** Host-Adresse für den Server
- **Beispiel:** `THEMIS_HOST=127.0.0.1`

## Storage Configuration

### `THEMIS_ROCKSDB_PATH`
- **Default:** `${THEMIS_DATA_DIR}/data`
- **Beschreibung:** Pfad für RocksDB Datenbankdateien
- **Beispiel:** `THEMIS_ROCKSDB_PATH=/var/lib/themisdb/data`

### `THEMIS_VECTOR_INDEX_PATH`
- **Default:** `${THEMIS_DATA_DIR}/vector_indexes`
- **Beschreibung:** Pfad für Vektorindex-Dateien
- **Beispiel:** `THEMIS_VECTOR_INDEX_PATH=/var/lib/themisdb/vector_indexes`

### `THEMIS_MEMTABLE_SIZE_MB`
- **Default:** `256`
- **Beschreibung:** Größe des RocksDB Memtable in MB
- **Beispiel:** `THEMIS_MEMTABLE_SIZE_MB=512`

### `THEMIS_BLOCK_CACHE_SIZE_MB`
- **Default:** `1024`
- **Beschreibung:** Größe des RocksDB Block Cache in MB
- **Beispiel:** `THEMIS_BLOCK_CACHE_SIZE_MB=2048`

## Server Configuration

### `THEMIS_WORKER_THREADS`
- **Default:** `8`
- **Beschreibung:** Anzahl der Worker-Threads
- **Beispiel:** `THEMIS_WORKER_THREADS=16`

## Logging Configuration

### `THEMIS_LOG_LEVEL`
- **Default:** `info`
- **Beschreibung:** Log-Level (trace, debug, info, warn, error)
- **Beispiel:** `THEMIS_LOG_LEVEL=debug`

### `THEMIS_LOG_DIR`
- **Default:** `/var/log/themis`
- **Beschreibung:** Verzeichnis für Log-Dateien
- **Beispiel:** `THEMIS_LOG_DIR=/var/log/themis`

## Feature Flags

### `THEMIS_ENABLE_TRACING`
- **Default:** `false`
- **Beschreibung:** OpenTelemetry Tracing aktivieren
- **Beispiel:** `THEMIS_ENABLE_TRACING=true`

### `THEMIS_ENABLE_SEMANTIC_CACHE`
- **Default:** `true`
- **Beschreibung:** Semantic Cache aktivieren
- **Beispiel:** `THEMIS_ENABLE_SEMANTIC_CACHE=false`

### `THEMIS_ENABLE_LLM_STORE`
- **Default:** `true`
- **Beschreibung:** LLM Store Feature aktivieren
- **Beispiel:** `THEMIS_ENABLE_LLM_STORE=true`

### `THEMIS_ENABLE_CDC`
- **Default:** `true`
- **Beschreibung:** Change Data Capture aktivieren
- **Beispiel:** `THEMIS_ENABLE_CDC=true`

### `THEMIS_ENABLE_TIMESERIES`
- **Default:** `true`
- **Beschreibung:** Timeseries-Support aktivieren
- **Beispiel:** `THEMIS_ENABLE_TIMESERIES=true`

## Telemetry Configuration

### `THEMIS_OTLP_ENDPOINT`
- **Default:** `http://localhost:4318`
- **Beschreibung:** OpenTelemetry Protocol Endpoint
- **Beispiel:** `THEMIS_OTLP_ENDPOINT=http://jaeger:4318`

### `THEMIS_SERVICE_NAME`
- **Default:** `themis-server`
- **Beschreibung:** Service-Name für Telemetrie
- **Beispiel:** `THEMIS_SERVICE_NAME=my-themis-instance`

## Advanced Configuration

### `THEMIS_FORCE_CONFIG_GENERATION`
- **Default:** `false`
- **Beschreibung:** Erzwingt die Neugenerierung der Konfigurationsdatei aus ENV-Variablen
- **Beispiel:** `THEMIS_FORCE_CONFIG_GENERATION=true`

## Verwendungsbeispiele

### Minimale Konfiguration
```bash
docker run -d \
  --name themis \
  -p 8080:8080 \
  -p 18765:18765 \
  -v themisdb_data:/var/lib/themisdb \
  themisdb/themisdb:1.3.0
```

### Mit benutzerdefinierten ENV-Variablen
```bash
docker run -d \
  --name themis \
  -p 8080:8080 \
  -p 18765:18765 \
  -e THEMIS_PORT=8765 \
  -e THEMIS_WORKER_THREADS=16 \
  -e THEMIS_MEMTABLE_SIZE_MB=512 \
  -e THEMIS_BLOCK_CACHE_SIZE_MB=2048 \
  -e THEMIS_ENABLE_TRACING=true \
  -e THEMIS_OTLP_ENDPOINT=http://jaeger:4318 \
  -v themisdb_data:/var/lib/themisdb \
  themisdb/themisdb:1.3.0
```

### Docker Compose Beispiel
```yaml
version: "3.8"
services:
  themisdb:
    image: themisdb:1.3.0
    container_name: themisdb-server
    ports:
      - "8080:8080"
      - "18765:18765"
      - "4318:4318"
    volumes:
      - themisdb_data:/var/lib/themisdb
    environment:
      THEMIS_PORT: "18765"
      THEMIS_WORKER_THREADS: "16"
      THEMIS_MEMTABLE_SIZE_MB: "512"
      THEMIS_BLOCK_CACHE_SIZE_MB: "2048"
      THEMIS_ENABLE_TRACING: "true"
      THEMIS_OTLP_ENDPOINT: "http://jaeger:4318"
      THEMIS_SERVICE_NAME: "themis-prod"
    healthcheck:
      test: ["CMD-SHELL", "curl -fsS http://localhost:8080/health || exit 1"]
      interval: 10s
      timeout: 3s
      retries: 10
      start_period: 10s

volumes:
  themisdb_data:
    driver: local
```

## Best Practices

1. **Named Volumes verwenden**: Verwenden Sie Named Volumes statt Bind Mounts für Produktionsumgebungen
2. **Nicht als Root laufen**: Der Container läuft standardmäßig als `themis` User (uid 999)
3. **Ressourcen beschränken**: Setzen Sie Memory und CPU Limits in Docker/Kubernetes
4. **Monitoring aktivieren**: Nutzen Sie `THEMIS_ENABLE_TRACING=true` für Produktionsumgebungen
5. **Persistente Daten**: Stellen Sie sicher, dass `/var/lib/themisdb` persistent gemountet ist

## Vergleich mit PostgreSQL

| PostgreSQL | ThemisDB | Beschreibung |
|------------|----------|--------------|
| `PGDATA` | `THEMIS_DATA_DIR` | Hauptdatenverzeichnis |
| `POSTGRES_USER` | `themis` (fest) | Non-root User |
| `POSTGRES_PASSWORD` | - | ThemisDB nutzt keine Passwörter im Basis-Setup |
| `POSTGRES_DB` | - | ThemisDB ist schema-frei |
| `/var/lib/postgresql/data` | `/var/lib/themisdb` | Standard-Datenverzeichnis |
| `postgres:postgres` | `themis:themis` (uid:gid 999:999) | User:Group |

## Siehe auch

- [Docker Best Practices](../docker/README.md)
- [Deployment Guide](./DEPLOYMENT.md)
- [Configuration Reference](./CONFIGURATION.md)
