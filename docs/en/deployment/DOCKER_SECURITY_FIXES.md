# Docker Security Fixes

## Übersicht

Dieses Dokument beschreibt die Sicherheitsverbesserungen, die an den ThemisDB Docker-Images vorgenommen wurden, um Schwachstellen zu reduzieren.

## Durchgeführte Änderungen

### 1. Base Image Upgrade

**Vorher:** Ubuntu 22.04 LTS (Jammy)  
**Nachher:** Ubuntu 24.04 LTS (Noble)

**Grund:** Ubuntu 24.04 enthält:
- Aktuellere Kernel-Versionen mit Sicherheitspatches
- Neuere Bibliotheken (OpenSSL 3.x, glibc, etc.)
- Weniger bekannte CVEs
- Verlängerter Sicherheitssupport bis 2029

### 2. Sicherheits-Updates während Build

Alle RUN-Befehle wurden erweitert um:
```dockerfile
RUN apt-get update && apt-get upgrade -y && apt-get install -y --no-install-recommends \
    [packages] \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*
```

**Vorteile:**
- `apt-get upgrade -y` installiert alle Sicherheitspatches
- `--no-install-recommends` minimiert Angriffsfläche
- `apt-get clean` reduziert Image-Größe
- Vollständige Bereinigung von temporären Dateien

### 3. Betroffene Dateien

Die folgenden Dockerfiles wurden aktualisiert:

1. **Dockerfile.themis-server** - Haupt-Server-Image
2. **Dockerfile.docker-deploy** - Deployment-Image für Docker Desktop
3. **docker/Dockerfile** - Multi-Stage Build mit Build & Runtime

### 4. Weitere Empfehlungen

#### Für Production-Deployments:

**Option A: Distroless Images (Empfohlen für maximale Sicherheit)**
```dockerfile
FROM gcr.io/distroless/cc-debian12:latest
# Nur die Binary, keine Shell, keine Package Manager
COPY --from=builder /app/themis_server /
CMD ["/themis_server"]
```

**Option B: Alpine Linux**
```dockerfile
FROM alpine:3.19
RUN apk add --no-cache libstdc++ ca-certificates
```

**Vorteile:**
- Minimale Angriffsfläche (10-20 MB statt 100+ MB)
- Keine Shell/Package Manager → keine Shell-Injection-Angriffe
- Weniger Pakete → weniger potenzielle CVEs

#### Sicherheits-Scanning einrichten

1. **Docker Scout aktivieren:**
```bash
docker scout quickview
docker scout cves themis:latest
```

2. **Trivy Integration:**
```bash
trivy image themisdb/themisdb:latest
```

3. **GitHub Actions Security Scan:**
```yaml
- name: Run Trivy vulnerability scanner
  uses: aquasecurity/trivy-action@master
  with:
    image-ref: 'themisdb/themisdb:latest'
    format: 'sarif'
    output: 'trivy-results.sarif'
```

### 5. Best Practices für laufende Container

1. **Regelmäßige Updates:** Images monatlich neu bauen
2. **Read-Only Filesystem:** `--read-only` Flag verwenden
3. **Non-Root User:** Bereits implementiert (User: themisdb, UID: 999)
4. **Resource Limits:** CPU/Memory Limits setzen
5. **Network Policies:** Nur erforderliche Ports exponieren

### 6. Verbleibende Schwachstellen prüfen

Nach dem Rebuild:

```bash
# Image neu bauen
docker build -t themis:latest -f Dockerfile.docker-deploy .

# Schwachstellen scannen
docker scout cves themis:latest

# Detaillierte Analyse
docker scout quickview themis:latest
```

### 7. CI/CD Integration

Fügen Sie zum Build-Pipeline hinzu:

```yaml
# .github/workflows/docker-security.yml
name: Docker Security Scan
on:
  push:
    branches: [ main ]
  pull_request:
    branches: [ main ]

jobs:
  security-scan:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - name: Build image
        run: docker build -t themis:test .
      - name: Run Trivy
        uses: aquasecurity/trivy-action@master
        with:
          image-ref: themis:test
          exit-code: 1
          severity: 'CRITICAL,HIGH'
```

## Zusammenfassung

- ✅ Base Image auf Ubuntu 24.04 aktualisiert
- ✅ Sicherheits-Updates während Build hinzugefügt
- ✅ Image-Größe durch Cleanup optimiert
- ✅ Non-Root User beibehalten (themis:999)
- 📋 Optional: Migration zu Distroless/Alpine für Production

## Nächste Schritte

1. Docker Images neu bauen
2. Vulnerability Scan durchführen
3. Verbleibende CVEs evaluieren
4. Bei Bedarf auf Distroless migrieren
