# Docker Production Optimierungen - Niedrige Priorität

Dieses Dokument beschreibt optionale Optimierungen mit niedriger Priorität, die je nach Bedarf implementiert werden können.

## Übersicht

Die folgenden Verbesserungen sind **optional** und sollten nur implementiert werden, wenn spezifische Anforderungen bestehen:

1. **Signal Handling im Entrypoint** - Graceful Shutdown
2. **SBOM Generation** - Software Bill of Materials für Compliance

---

## 1. Signal Handling im Entrypoint (Graceful Shutdown)

### Aktueller Status
- ✅ `set -euo pipefail` für Error Handling vorhanden
- ⚠️ Kein explizites Signal Handling via `trap`

### Warum ist das optional?
- Der entrypoint.sh nutzt `exec` für den themis_server Start
- `exec` ersetzt den Shell-Prozess durch den Server-Prozess
- Signale werden direkt an themis_server weitergeleitet
- Für die meisten Szenarien ist dies ausreichend

### Wann sollte es implementiert werden?
- Wenn Pre-Shutdown Hooks benötigt werden (z.B. Benachrichtigungen)
- Wenn zusätzliche Cleanup-Aktionen vor dem Shutdown nötig sind
- Wenn der Server im Background laufen soll (derzeit nicht der Fall)

### Implementierung (falls benötigt)

```bash
#!/usr/bin/env bash
set -euo pipefail

# ... existierende Setup-Logik ...

# Signal Handler für graceful shutdown
shutdown_handler() {
  echo "[entrypoint] Received shutdown signal, stopping gracefully..."
  
  # Optional: Pre-Shutdown Hooks
  # - Benachrichtigungen senden
  # - Verbindungen drainieren
  # - Temp-Dateien aufräumen
  
  if [ -n "${SERVER_PID:-}" ]; then
    kill -TERM "$SERVER_PID" 2>/dev/null || true
    wait "$SERVER_PID" 2>/dev/null || true
  fi
  
  echo "[entrypoint] Shutdown complete"
  exit 0
}

# Trap Signale (nur wenn Server im Background läuft)
trap shutdown_handler SIGTERM SIGINT SIGQUIT

# Server im Background starten
/usr/local/bin/themis_server --config "$TARGET_CONFIG" "$@" &
SERVER_PID=$!

# Auf Server-Prozess warten
wait "$SERVER_PID"
```

### Nachteile dieser Implementierung
- Komplexerer Code
- Shell-Prozess bleibt als PID 1 (statt themis_server direkt)
- Potenzielle Edge-Cases bei Signal-Weiterleitung

### Empfehlung
**Nicht implementieren**, außer es gibt konkrete Anforderungen für Pre-Shutdown Hooks.

---

## 2. SBOM Generation (Software Bill of Materials)

### Aktueller Status
- ⚠️ Keine automatische SBOM-Generierung

### Warum ist das optional?
- SBOM ist primär für Compliance und Security Audits relevant
- Nicht erforderlich für Funktionalität
- Kann jederzeit nachträglich generiert werden

### Wann sollte es implementiert werden?
- **Compliance-Anforderungen:** NTIA, Executive Order 14028
- **Security Audits:** Transparenz über Dependencies
- **Supply Chain Security:** Vulnerability Tracking
- **Enterprise Deployments:** Governance Requirements

### Implementierung

#### Option 1: Docker SBOM (Docker Desktop 4.7+)

```bash
# SBOM generieren
docker sbom themisdb:1.3.0 > sbom.json

# Format: SPDX 2.2 JSON
```

#### Option 2: Syft (Anchore)

```bash
# Installation
curl -sSfL https://raw.githubusercontent.com/anchore/syft/main/install.sh | sh -s -- -b /usr/local/bin

# SBOM generieren (SPDX)
syft themisdb:1.3.0 -o spdx-json > sbom.spdx.json

# SBOM generieren (CycloneDX)
syft themisdb:1.3.0 -o cyclonedx-json > sbom.cyclonedx.json

# Direkt aus Dockerfile
syft dir:. -o spdx-json > sbom.spdx.json
```

#### Option 3: GitHub Actions Integration

```yaml
name: Generate SBOM

on:
  release:
    types: [published]

jobs:
  sbom:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      
      - name: Build Docker Image
        run: docker build -t themisdb:${{ github.ref_name }} .
      
      - name: Generate SBOM with Syft
        uses: anchore/sbom-action@v0
        with:
          image: themisdb:${{ github.ref_name }}
          format: spdx-json
          output-file: sbom.spdx.json
      
      - name: Upload SBOM
        uses: actions/upload-artifact@v4
        with:
          name: sbom
          path: sbom.spdx.json
      
      - name: Attach SBOM to Release
        uses: softprops/action-gh-release@v1
        with:
          files: sbom.spdx.json
```

### SBOM Formate

| Format | Standard | Use Case |
|--------|----------|----------|
| SPDX | ISO/IEC 5962:2021 | Industry Standard, sehr umfassend |
| CycloneDX | OWASP | Security-fokussiert, leichtgewichtig |
| Syft JSON | Anchore | Tool-spezifisch, detailliert |

### SBOM Validierung

```bash
# SPDX Validierung
npm install -g @spdx/tools
spdx-verify sbom.spdx.json

# CycloneDX Validierung
cyclonedx-cli validate --input-file sbom.cyclonedx.json
```

### Integration mit Vulnerability Scanning

```bash
# Grype (Anchore) - Vulnerability Scanning mit SBOM
grype sbom:sbom.spdx.json

# Trivy - Vulnerability Scanning
trivy sbom sbom.spdx.json
```

### Empfehlung

**Implementieren bei Bedarf:**
- Für **Enterprise/Government:** Ja, SBOM ist oft erforderlich
- Für **Open Source Community:** Optional, aber hilfreich für Transparenz
- Für **Development:** Nicht notwendig

**Empfohlenes Format:** SPDX (ISO Standard)

**Empfohlenes Tool:** Syft (einfach, schnell, gut dokumentiert)

---

## Implementierungsreihenfolge

Falls beide Optimierungen implementiert werden sollen:

1. **SBOM Generation** - Einmalige Aktion, einfach hinzuzufügen
2. **Signal Handling** - Nur wenn konkrete Anforderung besteht

---

## Alternative: BuildKit SBOM (Zukunft)

Docker BuildKit arbeitet an nativer SBOM-Unterstützung:

```dockerfile
# Zukünftige BuildKit Feature
# syntax=docker/dockerfile:1.7-labs
FROM ubuntu:22.04

# BuildKit generiert automatisch SBOM
```

**Status:** Experimental (BuildKit 0.12+)

---

## Zusammenfassung

| Optimierung | Priorität | Aufwand | Empfehlung |
|-------------|-----------|---------|------------|
| Signal Handling | Niedrig | Mittel | Nur bei konkreter Anforderung |
| SBOM Generation | Niedrig | Gering | Bei Compliance-Anforderungen |

Beide Optimierungen können **jederzeit nachträglich** hinzugefügt werden, ohne die bestehende Funktionalität zu beeinträchtigen.

---

## Weitere Informationen

- **SBOM Standards:** https://ntia.gov/sbom
- **Syft Documentation:** https://github.com/anchore/syft
- **Docker SBOM:** https://docs.docker.com/engine/sbom/
- **Signal Handling Best Practices:** https://hynek.me/articles/docker-signals/
