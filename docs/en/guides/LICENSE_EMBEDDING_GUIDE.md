# ThemisDB License Data Embedding Guide

## Übersicht / Overview

ThemisDB unterstützt das Einbetten von Lizenz- und Unternehmensdaten direkt in die kompilierte Binärdatei. Diese Funktion ermöglicht:

- **Offline-Deployments**: Keine Notwendigkeit für Online-Lizenzvalidierung
- **CI/CD-Integration**: Automatisches Einbetten von Lizenzdaten während des Build-Prozesses
- **Kundenspezifische Builds**: Jede Binärdatei kann mit spezifischen Lizenzdaten kompiliert werden
- **Transparenz**: Lizenzinformationen werden beim Start angezeigt

---

ThemisDB supports embedding license and company data directly into the compiled binary. This feature enables:

- **Offline Deployments**: No need for online license validation
- **CI/CD Integration**: Automatic embedding of license data during the build process
- **Customer-Specific Builds**: Each binary can be compiled with specific license data
- **Transparency**: License information is displayed at startup

---

## Lizenz-Datei Format / License File Format

Die Lizenzdaten werden als JSON-Datei bereitgestellt:

```json
{
  "organization_name": "Example Corporation GmbH",
  "organization_id": "DE123456789",
  "contact_email": "licensing@example-corp.com",
  "license_key": "THEMIS-ENT-2026-ABCD1234-EXAMPLE",
  "edition": "ENTERPRISE",
  "issued_date": "2026-01-01",
  "expiry_date": "2027-12-31",
  "max_nodes": 100,
  "max_cores": -1,
  "max_storage_tb": -1,
  "build_id": "build-2026-01-example",
  "signature": "SHA256-RSA-SIGNATURE-PLACEHOLDER"
}
```

### Feldbeschreibungen / Field Descriptions

| Feld / Field | Typ / Type | Beschreibung / Description |
|--------------|------------|----------------------------|
| `organization_name` | String | Name des Unternehmens / Company name |
| `organization_id` | String | Optional: Unternehmens-ID / Optional: Company ID |
| `contact_email` | String | Kontakt-E-Mail für Lizenzsupport / Contact email for license support |
| `license_key` | String | **Erforderlich**: Eindeutiger Lizenzschlüssel / **Required**: Unique license key |
| `edition` | String | Edition: MINIMAL, COMMUNITY, ENTERPRISE, HYPERSCALER |
| `issued_date` | String | Ausstellungsdatum (ISO 8601: YYYY-MM-DD) / Issue date (ISO 8601: YYYY-MM-DD) |
| `expiry_date` | String | Ablaufdatum (ISO 8601: YYYY-MM-DD) / Expiry date (ISO 8601: YYYY-MM-DD) |
| `max_nodes` | Integer | Max. Anzahl Nodes (-1 = unbegrenzt) / Max nodes (-1 = unlimited) |
| `max_cores` | Integer | Max. CPU-Cores (-1 = unbegrenzt) / Max CPU cores (-1 = unlimited) |
| `max_storage_tb` | Integer | Max. Storage in TB (-1 = unbegrenzt) / Max storage in TB (-1 = unlimited) |
| `build_id` | String | Optional: Build-Identifikator / Optional: Build identifier |
| `signature` | String | Optional: RSA-Signatur zur Verifikation / Optional: RSA signature for verification |

---

## Build-Prozess / Build Process

### Methode 1: CMake-Konfiguration

```bash
# Mit Lizenzdatei kompilieren / Compile with license file
cmake -B build -S . \
  -DTHEMIS_EDITION=ENTERPRISE \
  -DTHEMIS_LICENSE_FILE=/path/to/license.json

cmake --build build --config Release
```

### Methode 2: Umgebungsvariable

```bash
# Lizenzdatei als Umgebungsvariable setzen / Set license file as environment variable
export THEMIS_LICENSE_FILE=/path/to/license.json

cmake -B build -S . -DTHEMIS_EDITION=ENTERPRISE
cmake --build build --config Release
```

### Methode 3: CI/CD-Pipeline

```yaml
# GitHub Actions Beispiel / GitHub Actions Example
- name: Build ThemisDB with License
  run: |
    # Lizenzdatei aus Secret erstellen / Create license file from secret
    echo '${{ secrets.THEMIS_LICENSE_DATA }}' > license.json
    
    # Build mit eingebetteter Lizenz / Build with embedded license
    cmake -B build -S . \
      -DTHEMIS_EDITION=ENTERPRISE \
      -DTHEMIS_LICENSE_FILE=license.json
    
    cmake --build build --config Release
    
    # Lizenzdatei löschen (Sicherheit) / Delete license file (security)
    rm license.json
```

---

## Validierung / Verification

### Beim Start / At Startup

ThemisDB zeigt die eingebetteten Lizenzdaten beim Start an:

```
===============================================================================
                      THEMIS DATABASE LICENSE INFORMATION                       
===============================================================================

ORGANIZATION:
  Name:               Example Corporation GmbH
  Organization ID:    DE123456789
  Contact Email:      licensing@example-corp.com

LICENSE:
  License Key:        THEMIS-ENT-2026-ABCD1234-EXAMPLE
  Edition:            ENTERPRISE
  Issued Date:        2026-01-01
  Expiry Date:        2027-12-31
  Days Until Expiry:  365 days

LICENSE LIMITS:
  Max Nodes:          100
  Max Cores:          Unlimited
  Max Storage:        Unlimited

BUILD INFORMATION:
  Build ID:           build-2026-01-example
  Build Timestamp:    2026-01-09 10:30:00 UTC

===============================================================================
```

### Über HTTP-API

```bash
# Lizenzinformationen über /health Endpoint abrufen
curl http://localhost:8765/health

# Lizenzinformationen über /version Endpoint abrufen
curl http://localhost:8765/version
```

---

## Offline-Deployment

Für Offline-Deployments ohne Internetzugang:

1. **Lizenzdatei erstellen**: Erstellen Sie die license.json mit den Unternehmensdaten
2. **Build durchführen**: Kompilieren Sie ThemisDB mit der Lizenzdatei
3. **Binärdatei verteilen**: Die kompilierte Binärdatei enthält alle Lizenzdaten
4. **Keine Online-Validierung**: ThemisDB prüft nur die eingebetteten Daten

```bash
# Offline-Build für Kunde
cmake -B build -S . \
  -DTHEMIS_EDITION=ENTERPRISE \
  -DTHEMIS_LICENSE_FILE=customer_license.json \
  -DTHEMIS_STATIC_BUILD=ON

cmake --build build --config Release

# Resultierende Binärdatei verteilen
# distribute: build/themis_server (oder themis_server.exe)
```

---

## CI/CD-Integration (egserver)

### Docker Build

```dockerfile
# Dockerfile mit Lizenz-Embedding
FROM ubuntu:24.04 AS builder

# Build-Tools installieren
RUN apt-get update && apt-get install -y cmake g++ git

# Source kopieren
COPY . /src
WORKDIR /src

# Lizenzdatei als Build-Argument
ARG LICENSE_DATA
RUN echo "$LICENSE_DATA" > /tmp/license.json

# Build mit eingebetteter Lizenz
RUN cmake -B build -S . \
    -DTHEMIS_EDITION=ENTERPRISE \
    -DTHEMIS_LICENSE_FILE=/tmp/license.json && \
    cmake --build build --config Release && \
    rm /tmp/license.json

FROM ubuntu:24.04
COPY --from=builder /src/build/themis_server /usr/local/bin/
ENTRYPOINT ["/usr/local/bin/themis_server"]
```

Build-Befehl:

```bash
docker build \
  --build-arg LICENSE_DATA="$(cat license.json)" \
  -t themisdb-enterprise:latest .
```

### Jenkins Pipeline

```groovy
pipeline {
    agent any
    
    environment {
        LICENSE_FILE = credentials('themis-enterprise-license')
    }
    
    stages {
        stage('Build') {
            steps {
                sh '''
                    cmake -B build -S . \
                      -DTHEMIS_EDITION=ENTERPRISE \
                      -DTHEMIS_LICENSE_FILE=${LICENSE_FILE}
                    
                    cmake --build build --config Release
                '''
            }
        }
        
        stage('Package') {
            steps {
                sh '''
                    cd build
                    cpack -G TGZ
                '''
                archiveArtifacts artifacts: 'build/*.tar.gz'
            }
        }
    }
}
```

### GitLab CI

```yaml
build-enterprise:
  stage: build
  script:
    # Lizenzdaten aus Secret-Variable
    - echo "$THEMIS_LICENSE_JSON" > license.json
    
    # Build
    - cmake -B build -S . 
        -DTHEMIS_EDITION=ENTERPRISE 
        -DTHEMIS_LICENSE_FILE=license.json
    - cmake --build build --config Release
    
    # Cleanup
    - rm license.json
    
  artifacts:
    paths:
      - build/themis_server
    expire_in: 1 week
```

---

## Sicherheitshinweise / Security Notes

1. **Lizenzdatei-Schutz**: Speichern Sie Lizenzdateien sicher (Git Secrets, Vault, etc.)
2. **Build-Artefakte**: Löschen Sie temporäre Lizenzdateien nach dem Build
3. **Signatur-Validierung**: Verwenden Sie RSA-Signaturen für zusätzliche Sicherheit
4. **Zugriffskontrolle**: Beschränken Sie Zugriff auf Lizenz-Builds

---

## Beispiele / Examples

### Community Edition (ohne Limits)

```json
{
  "organization_name": "Open Source Community",
  "license_key": "THEMIS-COMMUNITY-OPENSOURCE",
  "edition": "COMMUNITY",
  "issued_date": "2026-01-01",
  "expiry_date": "9999-12-31",
  "max_nodes": 1,
  "max_cores": -1,
  "max_storage_tb": -1
}
```

### Enterprise Edition (mit Limits)

```json
{
  "organization_name": "Enterprise Corp Ltd",
  "organization_id": "UK987654321",
  "contact_email": "support@enterprise-corp.com",
  "license_key": "THEMIS-ENT-2026-UK-987654321",
  "edition": "ENTERPRISE",
  "issued_date": "2026-01-09",
  "expiry_date": "2027-01-09",
  "max_nodes": 50,
  "max_cores": 512,
  "max_storage_tb": 100,
  "build_id": "enterprise-prod-v1.4.0"
}
```

### Hyperscaler Edition (unbegrenzt)

```json
{
  "organization_name": "Cloud Hyperscaler Inc",
  "organization_id": "US-CLOUD-2026",
  "contact_email": "enterprise@hyperscaler.com",
  "license_key": "THEMIS-HYPER-2026-CLOUD-UNLIMITED",
  "edition": "HYPERSCALER",
  "issued_date": "2026-01-01",
  "expiry_date": "2029-12-31",
  "max_nodes": -1,
  "max_cores": -1,
  "max_storage_tb": -1,
  "build_id": "hyperscaler-custom-build"
}
```

---

## Fehlerbehebung / Troubleshooting

### Lizenz wird nicht angezeigt / License not displayed

```bash
# Prüfen, ob Lizenz eingebettet wurde / Check if license was embedded
strings build/themis_server | grep "THEMIS-"

# Build-Log prüfen / Check build log
cmake -B build -S . -DTHEMIS_LICENSE_FILE=license.json 2>&1 | grep -i license
```

### Edition-Mismatch Warnung / Edition mismatch warning

```
WARNING: License edition (ENTERPRISE) does not match build edition (COMMUNITY)
```

**Lösung / Solution**: Stellen Sie sicher, dass die Edition in der Lizenzdatei mit der CMake-Konfiguration übereinstimmt:

```bash
cmake -B build -S . \
  -DTHEMIS_EDITION=ENTERPRISE \
  -DTHEMIS_LICENSE_FILE=license.json
```

### Abgelaufene Lizenz / Expired license

```
ERROR: WARNING: License has expired!
Please contact licensing@example.com to renew your license.
```

**Lösung / Solution**: 
- Aktualisieren Sie `expiry_date` in der Lizenzdatei
- Erstellen Sie einen neuen Build
- Für Perpetual Licenses: Verwenden Sie `"expiry_date": "9999-12-31"`

---

## Support

Bei Fragen zur Lizenz-Einbettung / For questions about license embedding:

- **Email**: licensing@themisdb.com
- **Enterprise Support**: enterprise@themisdb.com
- **Documentation**: https://docs.themisdb.org/license-embedding

---

## Changelog

- **v1.4.0 (2026-01-09)**: Initial implementation of license embedding feature
  - Support for JSON license files
  - CMake integration for build-time embedding
  - Startup validation and display
  - CI/CD examples and documentation
