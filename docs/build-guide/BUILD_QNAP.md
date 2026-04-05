# QNAP NAS Build Guide

## QNAP Modelle & Unterstützung

| Serie | CPU | Architektur | RAM | Docker | Support |
|-------|-----|-------------|-----|--------|---------|
| **TS-464** | Celeron N5105 | x86_64 | 4-8GB | ✅ | ✅ Best |
| **TS-264** | Celeron N5105 | x86_64 | 4-8GB | ✅ | ✅ Best |
| **TS-932PX** | Ryzen | x86_64 | 8-64GB | ✅ | ✅ Best |
| **TS-253E** | Celeron J3455 | x86_64 | 4-8GB | ⚠️ | ✅ Gut |
| **TS-453E** | Celeron N5105 | x86_64 | 4-8GB | ⚠️ | ✅ Gut |
| **TS-432P** | Celeron N3050 | x86_64 | 2-4GB | ❌ | ⚠️ Möglich |
| **TS-231** | ARMv7 | ARMv7 | 1-2GB | ❌ | ❌ Schwierig |

**Empfehlung**: x86_64 QNAP mit Docker Support (TS-264, TS-464)

## QNAP mit Docker (Empfohlen)

### Voraussetzungen

1. **Docker ist installiert**
   - QNAP Admin Panel → App Center → Suche "Docker"
   - "Container Station" oder "Docker" installieren

2. **SSH aktiviert** (für Commands)
   - Settings → Advanced → Terminal & SNMP → Enable SSH

3. **Genug Storage** (min. 20GB für ThemisDB)

### Image bauen

#### Option A: Vorkompiliertes Image verwenden

```bash
# Von GitHub (falls veröffentlicht)
ssh admin@qnap-ip
docker pull themisdb/themisdb:qnap
docker run -d \
  --name themis \
  -p 18765:18765 \
  -p 8080:8080 \
  -v /share/themisdb:/var/lib/themisdb \
  themisdb/themisdb:qnap
```

#### Option B: Auf QNAP selbst bauen

```bash
# SSH zu QNAP
ssh admin@qnap-ip

# Source klonen
cd /share
git clone https://github.com/makr-code/themisdb.git
cd themisdb

# Dockerfile für QNAP verwenden
docker build -f docker/Dockerfile.qnap \
  -t themis-server:qnap-local \
  .

# Starten
docker-compose -f docker/docker-compose.qnap.yml up -d
```

#### Option C: Cross-Compile von x86_64

```bash
# Auf Linux/WSL (nicht auf QNAP - zu langsam)
docker buildx build \
  --platform linux/amd64 \
  -f docker/Dockerfile.qnap \
  -t themis-server:qnap \
  .

# Zu QNAP pushen
docker save themis-server:qnap | \
  ssh admin@qnap-ip 'docker load'
```

### Docker-Compose auf QNAP

```bash
ssh admin@qnap-ip
cd /share/themisdb

# docker-compose-qnap.yml nutzen
docker-compose -f docker/docker-compose.qnap.yml up -d

# Logs anschauen
docker-compose -f docker/docker-compose.qnap.yml logs -f themis
```

**docker-compose.qnap.yml Inhalt:**
```yaml
version: '3.8'
services:
  themis:
    image: themis-server:qnap-latest
    container_name: themis-db
    ports:
      - "18765:18765"
      - "8080:8080"
    volumes:
      - /share/themisdb/data:/var/lib/themisdb
      - /share/themisdb/config:/etc/themis
    environment:
      THEMIS_PORT: 18765
      THEMIS_HTTP_PORT: 8080
      THEMIS_DATA_DIR: /var/lib/themisdb
    restart: always
    healthcheck:
      test: ["CMD", "curl", "-f", "http://localhost:8080/health"]
      interval: 30s
      timeout: 5s
      retries: 3
```

## QNAP ohne Docker (Native Build)

Nur für x86_64 Modelle!

### Schritt 1: SSH Zugang aktivieren

```
QNAP Admin Panel → Settings → Advanced → Terminal & SNMP
→ Enable SSH ✅
```

### Schritt 2: Build Tools installieren

```bash
ssh admin@qnap-ip

# Qts Linux Verzeichnis (QNAP's Linux-Kernel)
cd /share

# Build tools auf QNAP sind begrenzt
# Besser: Chroot-Umgebung mit vollständigen Tools

# Oder: QNAP Optware/IPKG nutzen
ipkg install gcc g++ make cmake

# Falls das nicht funktioniert, dann Docker verwenden!
```

### Schritt 3: ThemisDB kompilieren

```bash
cd /share
git clone https://github.com/makr-code/themisdb.git
cd themisdb

# QNAP Firmware hat oft alte Tools
# Daher minimal bauen
cmake -S . -B build-qnap \
  -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DTHEMIS_ENABLE_LLM=OFF \
  -DTHEMIS_ENABLE_GPU=OFF \
  -DTHEMIS_BUILD_TESTS=OFF \
  -DTHEMIS_BUILD_BENCHMARKS=OFF

cmake --build build-qnap --parallel 2

# Binary: build-qnap/themis_server
```

## ThemisDB starten

### Über Web UI (falls QNAP App existiert)

```
QNAP Control Panel → Applications
→ Themis DB → Starten
```

### Über SSH (Manual)

```bash
ssh admin@qnap-ip

# Binary direkt
/share/themisdb/build-qnap/themis_server \
  --port 18765 \
  --http-port 8080 \
  --data-dir /share/themisdb/data

# Im Hintergrund
nohup /share/themisdb/build-qnap/themis_server \
  --port 18765 \
  --data-dir /share/themisdb/data &
```

### Als QNAP Service (init.d)

```bash
# Service-Datei erstellen
ssh admin@qnap-ip

cat > /etc/init.d/themis.sh <<'EOF'
#!/bin/bash
case "$1" in
  start)
    /share/themisdb/build-qnap/themis_server \
      --port 18765 \
      --data-dir /share/themisdb/data &
    ;;
  stop)
    pkill -f themis_server
    ;;
esac
EOF

chmod +x /etc/init.d/themis.sh

# Starten
/etc/init.d/themis.sh start
```

## RAID auf QNAP nutzen

ThemisDB hat spezielle RAID-Unterstützung!

```bash
# QNAP RAID-Punkte ermitteln
df -h

# z.B. /mnt/cachedev1_0 → ThemisDB Raid-Config:
/share/themisdb/build-qnap/themis_server \
  --port 18765 \
  --data-dir /mnt/cachedev1_0/themisdb \
  --raid-config /mnt/cachedev1_0/themisdb/raid.json
```

## Backup & Restore

### Backup vom QNAP

```bash
# Komplette DB sichern
tar -czf /share/themisdb-backup-$(date +%Y%m%d).tar.gz \
  /share/themisdb/data/

# Oder mit QNAP Backup-Tool
QNAP Control Panel → Backup/Restore
→ Select /share/themisdb/data
```

### Restore

```bash
ssh admin@qnap-ip

# Backup extrahieren
tar -xzf themisdb-backup-20240101.tar.gz \
  -C /share/themisdb/
```

## Monitoring

### Status überprüfen

```bash
ssh admin@qnap-ip

# Process Status
ps aux | grep themis_server

# Port Binding überprüfen
netstat -tuln | grep 18765
netstat -tuln | grep 8080

# Health Check
curl http://qnap-ip:8080/health
```

### Logs auf QNAP

Docker-Logs:
```bash
docker logs themis-db
```

Standalone-Logs:
```bash
tail -f nohup.out  # Falls mit nohup gestartet
```

## Spezialfall: QNAP mit ARM CPU (TS-231 etc)

Falls Sie ein ARM QNAP haben:

1. **Native Build schwierig** - QNAP Tools sehr begrenzt
2. **Docker unmöglich** - Kein ARM64 Container Support
3. **Alternative**: Cross-Compilation auf x86_64 Host

```bash
# Auf x86_64 Linux Machine:
cmake -S . -B build-qnap-arm \
  -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-arm64.cmake \
  -DCMAKE_BUILD_TYPE=Release \
  -DTHEMIS_ENABLE_LLM=OFF

cmake --build build-qnap-arm --parallel 8

# Binary zu QNAP via SCP übertragen
scp build-qnap-arm/themis_server admin@qnap-ip:/share/themisdb/
```

## Troubleshooting

### Problem: "Docker Permission Denied"
**Lösung**: User zur docker group hinzufügen
```bash
ssh admin@qnap-ip
usermod -aG docker admin
```

### Problem: "No space left on device"
**Lösung**: RAID/Storage-Kapazität prüfen
```bash
ssh admin@qnap-ip
df -h  # Überblick
du -sh /share/themisdb  # DB-Größe
```

### Problem: "Build failed - unknown compiler"
**Lösung**: Docker verwenden statt Native Build auf QNAP

### Problem: Container startet nicht
**Lösung**:
```bash
docker logs themis-db
docker inspect themis-db
```

## Performance-Tipps für QNAP

1. **SSD-Cache für Datenbank**
   - QNAP Settings: Enable SSD Tiering
   - /share/themisdb/data → SSD Cache

2. **Network Optimization**
   - Wired Ethernet bevorzugt
   - GbE oder höher

3. **Thermal Management**
   - ThemisDB kann CPU-intensiv sein
   - QNAP Lüfter-Management überprüfen

4. **Disk-I/O Optimization**
   - Separate RAID Volume für themisdb
   - Dediziert für Datenbank

## Nächste Schritte

Nach erfolgreichem QNAP-Build lesen Sie:
- **QNAP Deployment**: [docs/de/deployment/deployment_qnap.md](../../de/deployment/deployment_qnap.md) - QNAP-spezifisches Deployment
- **ARM Deployment**: [docs/de/deployment/deployment_arm_build.md](../../de/deployment/deployment_arm_build.md) - ARM-Architektur
- **Releases**: [docs/de/releases/updates_distribution_strategy.md](../../de/releases/updates_distribution_strategy.md)

## Weitere Infos

- [BUILD_ARM.md](BUILD_ARM.md) - Generischer ARM-Guide
- [docker/Dockerfile.qnap](../../docker/Dockerfile.qnap) - QNAP Dockerfile
- [docker/docker-compose.qnap.yml](../../docker/docker-compose.qnap.yml) - Compose Config
- [QNAP Docs](https://www.qnap.com/de-de) - QNAP offizielle Dokumentation
