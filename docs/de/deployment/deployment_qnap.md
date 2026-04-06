# ThemisDB auf QNAP NAS

**Stand:** 6. April 2026  
**Version:** v1.3.1  
**Kategorie:** 🚀 Deployment

---

## 📑 Inhaltsverzeichnis

- [Systemvoraussetzungen](#systemvoraussetzungen)
- [Besonderheiten](#besonderheiten-des-qnap-builds)
- [Installation](#installation)
- [Konfiguration](#konfiguration)

Dieses Dokument beschreibt die Installation und Konfiguration von ThemisDB auf QNAP NAS-Systemen mittels Container Station.

## Systemvoraussetzungen

- **QNAP NAS** mit Container Station installiert
- **Architektur**: x86_64 (Intel/AMD) oder ARM64 (empfohlen: x86_64)
- **RAM**: Mindestens 2GB verfügbar für den Container
- **Storage**: Mindestens 10GB freier Speicher

## Besonderheiten des QNAP-Builds

Das QNAP-Image basiert auf **Ubuntu 20.04** (statt 22.04) für maximale Kompatibilität:

- **GLIBC 2.31** (statt 2.35) - funktioniert auch auf älteren QNAP-Systemen
- **Optimiert für Container Station** - angepasste Healthchecks und Restart-Policy
- **Port 18765** - vermeidet Konflikte mit Standard-QNAP-Diensten

## Installation

### Option 1: Über Docker Compose (empfohlen)

1. **Datei herunterladen:**
   ```bash
   wget https://raw.githubusercontent.com/makr-code/ThemisDB/main/docker-compose.qnap.yml
   ```

2. **Container starten:**
   ```bash
   docker-compose -f docker-compose.qnap.yml up -d
   ```

### Option 2: Über Container Station GUI

1. **Container Station öffnen**
2. **"Container erstellen"** wählen
3. **Image**: `themisdb/themisdb:qnap`
4. **Port-Mapping**: `18765:18765`
5. **Volume**: `/share/Container/themis/data` → `/data`
6. **Container starten**

### Option 3: Docker CLI

```bash
docker run -d \
  --name themis \
  --restart unless-stopped \
  -p 18765:18765 \
  -v /share/Container/themis/data:/data \
  -e TZ=Europe/Berlin \
  -e THEMIS_PORT=18765 \
  themisdb/themisdb:qnap
```

## Konfiguration

### Volumes

Erstelle folgende Verzeichnisse auf deinem QNAP:

```bash
mkdir -p /share/Container/themis/data
mkdir -p /share/Container/themis/config
```

### Eigene Konfiguration

1. **Template kopieren:**
   ```bash
   docker cp themis:/etc/themis/config.json /share/Container/themis/config/
   ```

2. **Konfiguration anpassen** und in `docker-compose.qnap.yml` einkommentieren:
   ```yaml
   volumes:
     - /share/Container/themis/config/config.json:/etc/themis/config.json:ro
   ```

3. **Container neu starten:**
   ```bash
   docker-compose -f docker-compose.qnap.yml restart
   ```

## Zugriff

Nach dem Start ist ThemisDB verfügbar unter:

- **HTTP API**: `http://<QNAP-IP>:18765`
- **Health Check**: `http://<QNAP-IP>:18765/health`
- **Metrics**: `http://<QNAP-IP>:18765/metrics`

## Überwachung

### Container-Logs

```bash
# Über Docker CLI
docker logs -f themis

# Über Container Station
# → Container auswählen → "Log" Tab
```

### Health Status

```bash
docker inspect themis --format='{{.State.Health.Status}}'
```

### Resource-Nutzung

```bash
docker stats themis
```

## Backup

### Datensicherung

Erstelle ein Backup des Data-Volumes:

```bash
# Stoppe Container
docker-compose -f docker-compose.qnap.yml stop

# Erstelle Backup
tar -czf themis-backup-$(date +%Y%m%d).tar.gz /share/Container/themis/data

# Starte Container
docker-compose -f docker-compose.qnap.yml start
```

### Restore

```bash
# Stoppe Container
docker-compose -f docker-compose.qnap.yml stop

# Lösche alte Daten
rm -rf /share/Container/themis/data/*

# Restore Backup
tar -xzf themis-backup-YYYYMMDD.tar.gz -C /

# Starte Container
docker-compose -f docker-compose.qnap.yml start
```

## Updates

### Image aktualisieren

```bash
# Pull neues Image
docker pull themisdb/themisdb:qnap

# Neustart mit neuem Image
docker-compose -f docker-compose.qnap.yml up -d
```

## Troubleshooting

### Container startet nicht

1. **Logs prüfen:**
   ```bash
   docker logs themis
   ```

2. **Ports prüfen:**
   ```bash
   netstat -tulpn | grep 18765
   ```

3. **Permissions prüfen:**
   ```bash
   ls -la /share/Container/themis/
   ```

### Langsame Performance

1. **RAM-Nutzung prüfen:**
   ```bash
   docker stats themis
   ```

2. **Mehr RAM zuweisen** in Container Station GUI

3. **SSD Cache** für `/share/Container/themis/data` aktivieren (falls verfügbar)

### Healthcheck schlägt fehl

```bash
# Manuell prüfen
curl http://localhost:18765/health

# Container neu starten
docker restart themis
```

## ARM-Support

Für ARM-basierte QNAP-Modelle (z.B. TS-x32PXU):

```bash
# Build mit ARM64-Triplet
docker build -f Dockerfile.qnap \
  --build-arg VCPKG_TRIPLET=arm64-linux \
  -t themisdb/themisdb:qnap-arm64 .
```

Dann in `docker-compose.qnap.yml` das Image anpassen:
```yaml
image: themisdb/themisdb:qnap-arm64
```

## Sicherheit

### TLS aktivieren

1. **Zertifikat generieren** (siehe Haupt-Dokumentation)
2. **Mount Zertifikat:**
   ```yaml
   volumes:
     - /share/Container/themis/certs:/etc/themis/certs:ro
   ```
3. **Config anpassen** für HTTPS

### Firewall

Öffne Port **18765** in der QNAP-Firewall (falls aktiviert):

```
System → Sicherheit → Firewall → Regel hinzufügen
```

## Performance-Tuning

### Für große Datenmengen

In `docker-compose.qnap.yml`:

```yaml
environment:
  - THEMIS_MAX_CONNECTIONS=100
  - THEMIS_CACHE_SIZE_MB=512
deploy:
  resources:
    limits:
      memory: 4G
    reservations:
      memory: 2G
```

## Support

Bei Problemen:

1. **Logs sammeln**: `docker logs themis > themis.log`
2. **Issue erstellen**: https://github.com/makr-code/ThemisDB/issues
3. **QNAP-Modell angeben** (z.B. TS-453D, QHora-322)

## Weitere Informationen

- **Projekt**: https://github.com/makr-code/ThemisDB
- **Dokumentation**: https://makr-code.github.io/ThemisDB/
- **Docker Hub**: https://hub.docker.com/r/themisdb/themisdb
