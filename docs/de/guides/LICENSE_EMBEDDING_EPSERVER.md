# ThemisDB License Embedding Integration mit epServer

## Übersicht

Dieses Dokument beschreibt die Integration zwischen:
- **ThemisDB Binary Builds** mit eingebetteten Lizenzdaten
- **epServer (Enterprise Pricing Server)** für Lizenzverwaltung

## Zwei Deployment-Modi

### Modus 1: Embedded License (Offline)

Lizenzdaten werden **zur Build-Zeit** in die Binärdatei eingebettet:

```
┌─────────────────┐
│   epServer      │ Generiert license.json
│  (Pricing API)  │
└────────┬────────┘
         │ license.json
         ↓
┌─────────────────┐
│  Build Process  │ Embeds license data
│   (CMake/CI)    │ at compile time
└────────┬────────┘
         │ themis_server binary
         ↓
┌─────────────────┐
│ Customer Site   │ Runs offline
│  (No Internet)  │ License embedded
└─────────────────┘
```

**Vorteile:**
- ✅ Keine Online-Validierung erforderlich
- ✅ Funktioniert in Air-Gapped Umgebungen
- ✅ Kundenspezifische Builds
- ✅ Keine externe Abhängigkeiten

**Workflow:**

1. **epServer**: Kunde kauft Lizenz → `POST /license/generate`
2. **epServer**: Generiert `license.json` mit Kundendaten
3. **Build System**: `cmake -DTHEMIS_LICENSE_FILE=license.json`
4. **Resultat**: Binary enthält Lizenzdaten
5. **Deployment**: Binary zu Kunde versenden

### Modus 2: Online Validation (mit epServer)

ThemisDB validiert Lizenz **zur Laufzeit** mit epServer:

```
┌─────────────────┐
│   epServer      │◄─── Validates license
│  (Pricing API)  │     at runtime
└────────┬────────┘
         │ HTTPS
         ↓
┌─────────────────┐
│  themis_server  │ Checks license
│  (with license  │ on startup & periodic
│   key in config)│
└─────────────────┘
```

**Vorteile:**
- ✅ Zentrale Lizenzverwaltung
- ✅ Echtzeit-Validierung
- ✅ Lizenz-Widerruf möglich
- ✅ Nutzungs-Tracking

**Workflow:**

1. **Config**: `license_key: "THEMIS-ENT-..."` in config.yaml
2. **Startup**: ThemisDB → `POST /license/validate` an epServer
3. **epServer**: Prüft Key in Datenbank
4. **Response**: Lizenzstatus + Limits zurück
5. **Runtime**: Periodische Validierung (optional)

---

## Integration: Embedded License Build

### 1. epServer: Lizenz generieren

Der epServer generiert eine `license.json` für den Kunden:

```python
# epServer: routers/license.py
@router.post("/license/generate")
async def generate_license(request: LicenseGenerateRequest):
    license_data = {
        "organization_name": request.organization_name,
        "organization_id": request.organization_id,
        "contact_email": request.contact_email,
        "license_key": generate_license_key(),  # THEMIS-ENT-...
        "edition": request.edition,  # ENTERPRISE
        "issued_date": datetime.now().strftime("%Y-%m-%d"),
        "expiry_date": (datetime.now() + timedelta(days=365)).strftime("%Y-%m-%d"),
        "max_nodes": request.max_nodes,
        "max_cores": request.max_cores or -1,
        "max_storage_tb": request.max_storage_tb or -1,
        "build_id": f"build-{request.organization_id}-{datetime.now().strftime('%Y%m%d')}",
        "signature": sign_license_data(license_data)  # RSA signature
    }
    
    # Speichern in Datenbank
    await db.licenses.insert_one(license_data)
    
    return license_data
```

### 2. CI/CD: Build mit Lizenz

Die generierte `license.json` wird im Build-Prozess verwendet:

```yaml
# .github/workflows/build-enterprise.yml
name: Build Enterprise ThemisDB

on:
  workflow_dispatch:
    inputs:
      customer_id:
        description: 'Customer Organization ID'
        required: true

jobs:
  build-licensed:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      
      # Lizenz von epServer abrufen
      - name: Fetch License from epServer
        run: |
          curl -X POST https://service.themisdb.org:6734/license/generate \
            -H "Content-Type: application/json" \
            -H "Authorization: Bearer ${{ secrets.EPSERVER_API_KEY }}" \
            -d '{
              "organization_name": "${{ inputs.customer_id }}",
              "organization_id": "${{ inputs.customer_id }}",
              "edition": "ENTERPRISE",
              "max_nodes": 100
            }' -o license.json
      
      # Build mit eingebetteter Lizenz
      - name: Build ThemisDB with License
        run: |
          cmake -B build -S . \
            -DTHEMIS_EDITION=ENTERPRISE \
            -DTHEMIS_LICENSE_FILE=license.json
          
          cmake --build build --config Release
      
      # Artifakte hochladen
      - name: Upload Binary
        uses: actions/upload-artifact@v4
        with:
          name: themisdb-${{ inputs.customer_id }}
          path: build/themis_server
      
      # Cleanup
      - name: Remove License File
        if: always()
        run: rm -f license.json
```

### 3. Docker Build mit Lizenz

```dockerfile
# Dockerfile.enterprise
FROM ubuntu:24.04 AS builder

# Build tools
RUN apt-get update && \
    apt-get install -y cmake g++ git curl && \
    rm -rf /var/lib/apt/lists/*

COPY . /src
WORKDIR /src

# ARG for license data (passed at build time)
ARG LICENSE_JSON
RUN echo "$LICENSE_JSON" > /tmp/license.json

# Build with embedded license
RUN cmake -B build -S . \
    -DTHEMIS_EDITION=ENTERPRISE \
    -DTHEMIS_LICENSE_FILE=/tmp/license.json && \
    cmake --build build --config Release && \
    rm /tmp/license.json

# Runtime image
FROM ubuntu:24.04
RUN apt-get update && \
    apt-get install -y libssl3 && \
    rm -rf /var/lib/apt/lists/*

COPY --from=builder /src/build/themis_server /usr/local/bin/
ENTRYPOINT ["/usr/local/bin/themis_server"]
```

Build-Befehl:

```bash
# Lizenz von epServer abrufen
LICENSE_DATA=$(curl -X POST https://service.themisdb.org:6734/license/generate \
  -H "Authorization: Bearer ${EPSERVER_API_KEY}" \
  -d '{"organization_name": "Customer Corp", "edition": "ENTERPRISE"}')

# Docker Image bauen
docker build \
  --build-arg LICENSE_JSON="$LICENSE_DATA" \
  -t themisdb-customer:1.4.0 \
  -f Dockerfile.enterprise .
```

---

## Integration: Online License Validation

Für Online-Validierung siehe: [epServer/LICENSE_INTEGRATION.md](../epServer/LICENSE_INTEGRATION.md)

### Kurzübersicht:

1. **Config**: In `config.yaml`:
   ```yaml
   license:
     key: "THEMIS-ENT-2026-CUSTOMER-001"
     validation_url: "https://service.themisdb.org:6734"
     validate_on_startup: true
     validate_interval_hours: 24
   ```

2. **ThemisDB Startup**: Validiert mit epServer
3. **epServer Response**: Lizenzstatus + Limits
4. **Runtime**: Optional periodische Validierung

---

## Hybrid-Ansatz: Embedded + Online

Kombiniert beide Ansätze für maximale Flexibilität:

1. **Build-Zeit**: Lizenz einbetten (für Offline-Betrieb)
2. **Runtime**: Optional Online-Validierung (wenn Internet verfügbar)

```cpp
// src/main_server.cpp
auto embedded_license = themis::license::getEmbeddedLicense();
if (embedded_license) {
    // Zeige eingebettete Lizenz an
    display_license(*embedded_license);
    
    // Versuche Online-Validierung (optional)
    if (config.license.validation_url) {
        try {
            auto online_status = validate_license_online(
                embedded_license->license_key,
                config.license.validation_url
            );
            
            if (online_status.valid) {
                THEMIS_INFO("Online license validation successful");
            } else {
                THEMIS_WARN("Online validation failed, using embedded license");
            }
        } catch (const std::exception& e) {
            THEMIS_WARN("Could not reach license server, using embedded license");
        }
    }
}
```

---

## Sicherheitsaspekte

### 1. Signatur-Validierung

Lizenzen sollten mit RSA-4096 signiert werden:

```python
# epServer: utils/license.py
def sign_license_data(license_data: dict) -> str:
    # Daten für Signatur vorbereiten
    data_to_sign = f"{license_data['license_key']}" \
                   f"{license_data['organization_name']}" \
                   f"{license_data['issued_date']}" \
                   f"{license_data['expiry_date']}"
    
    # RSA-SHA256 Signatur
    private_key = load_private_key()
    signature = private_key.sign(
        data_to_sign.encode(),
        padding.PSS(
            mgf=padding.MGF1(hashes.SHA256()),
            salt_length=padding.PSS.MAX_LENGTH
        ),
        hashes.SHA256()
    )
    
    return base64.b64encode(signature).decode()
```

### 2. Public Key Embedding

Der öffentliche Schlüssel wird in ThemisDB eingebettet:

```cpp
// src/utils/license_info.cpp
bool verifyLicenseSignature(const LicenseData& license) {
    // Embedded public key (PEM format)
    constexpr const char* PUBLIC_KEY = R"(
-----BEGIN PUBLIC KEY-----
MIICIjANBgkqhkiG9w0BAQEFAAOCAg8AMIICCgKCAgEA...
-----END PUBLIC KEY-----
    )";
    
    // Verify signature using OpenSSL
    // Implementation details...
}
```

### 3. Tamper Detection

```cpp
// Prüfe, ob Binary modifiziert wurde
bool verify_binary_integrity() {
    // SHA-256 Hash der Binary
    auto binary_hash = compute_sha256(get_executable_path());
    
    // Vergleich mit erwarteten Hash (in Signatur kodiert)
    return binary_hash == license.expected_binary_hash;
}
```

---

## Best Practices

### 1. Automatisierte Builds

```python
# Jenkins Pipeline
pipeline {
    agent any
    parameters {
        string(name: 'CUSTOMER_ID', description: 'Customer Organization ID')
        choice(name: 'EDITION', choices: ['ENTERPRISE', 'HYPERSCALER'])
    }
    
    stages {
        stage('Fetch License') {
            steps {
                script {
                    // Lizenz von epServer abrufen
                    sh """
                        curl -X POST ${EPSERVER_URL}/license/generate \
                          -H "Authorization: Bearer ${EPSERVER_TOKEN}" \
                          -d '{"organization_id": "${params.CUSTOMER_ID}", "edition": "${params.EDITION}"}' \
                          -o license.json
                    """
                }
            }
        }
        
        stage('Build') {
            steps {
                sh """
                    cmake -B build -S . \
                      -DTHEMIS_EDITION=${params.EDITION} \
                      -DTHEMIS_LICENSE_FILE=license.json
                    cmake --build build --config Release
                """
            }
        }
        
        stage('Package') {
            steps {
                sh """
                    tar czf themisdb-${params.CUSTOMER_ID}-${params.EDITION}.tar.gz \
                      -C build themis_server
                """
                archiveArtifacts "*.tar.gz"
            }
        }
    }
    
    post {
        always {
            sh 'rm -f license.json'
        }
    }
}
```

### 2. Lizenz-Tracking

epServer speichert alle generierten Lizenzen:

```python
# Database Schema
{
    "license_key": "THEMIS-ENT-2026-CUSTOMER-001",
    "organization_id": "CUSTOMER-001",
    "generated_at": "2026-01-09T10:30:00Z",
    "build_id": "build-CUSTOMER-001-20260109",
    "binary_hash": "sha256:abc123...",
    "downloads": [
        {"timestamp": "2026-01-09T11:00:00Z", "ip": "203.0.113.42"}
    ],
    "validations": [
        {"timestamp": "2026-01-09T12:00:00Z", "hostname": "prod-db-01"}
    ]
}
```

### 3. Lizenz-Erneuerung

```python
@router.post("/license/renew")
async def renew_license(license_key: str, new_expiry_date: str):
    # Alte Lizenz laden
    old_license = await db.licenses.find_one({"license_key": license_key})
    
    # Neue Lizenz generieren (gleiche Daten, neue Expiry)
    new_license = {**old_license}
    new_license["expiry_date"] = new_expiry_date
    new_license["signature"] = sign_license_data(new_license)
    
    # Speichern
    await db.licenses.insert_one(new_license)
    
    return new_license
```

---

## Troubleshooting

### Problem: Lizenz nicht sichtbar

```bash
# Prüfen, ob Lizenz eingebettet wurde
strings themis_server | grep "THEMIS-ENT"

# Build-Log prüfen
cmake -B build -S . -DTHEMIS_LICENSE_FILE=license.json 2>&1 | grep -i license
```

### Problem: Signatur-Fehler

```bash
# Public Key prüfen
openssl rsa -pubin -in public_key.pem -text -noout

# Signatur manuell verifizieren
echo "data_to_sign" | openssl dgst -sha256 -verify public_key.pem -signature signature.bin
```

---

## Weitere Dokumentation

- [LICENSE_EMBEDDING_GUIDE.md](../docs/en/guides/LICENSE_EMBEDDING_GUIDE.md) - Umfassende Anleitung
- [epServer/LICENSE_INTEGRATION.md](../epServer/LICENSE_INTEGRATION.md) - Online-Validierung
- [config/license_example.json](../config/license_example.json) - Beispiel-Lizenzdatei

---

## Support

- **Technical Support**: support@themisdb.com
- **License Issues**: licensing@themisdb.com
- **epServer API**: https://service.themisdb.org:6734/docs
