# Overnight Builds für ThemisDB mit DockerHub Push

## Übersicht

ThemisDB verfügt über ein automatisiertes Overnight-Build-System (nächtliche Builds), das täglich den neuesten Code kompiliert und Docker-Images zu DockerHub hochlädt. Dies stellt sicher, dass die neueste Entwicklungsversion immer zum Testen und zur Bereitstellung verfügbar ist.

## Wie funktionieren die Overnight Builds?

### 1. Zeitplan

Die Builds werden **automatisch jeden Tag um 2:00 Uhr UTC** ausgeführt:
- Keine manuelle Intervention erforderlich
- Läuft im Hintergrund über GitHub Actions
- Dauert ca. 30-60 Minuten (abhängig von Cache-Verfügbarkeit)
- **Intelligente Erkennung**: Build wird nur gestartet, wenn Änderungen in den letzten 24 Stunden erkannt wurden
- Spart Ressourcen durch Überspringen unnötiger Builds

### 2. Build-Prozess

Der Overnight-Build durchläuft fünf Hauptphasen:

#### Phase 0: Änderungserkennung
- Prüft auf Commits in den letzten 24 Stunden
- Überspringt Build bei geplanten Ausführungen ohne Änderungen
- Manuelle Ausführungen können Build auch ohne Änderungen erzwingen

#### Phase 1: Setup
- Liest die Version aus der `VERSION`-Datei (aktuell: 1.3.0)
- Generiert Build-Datum (Format: YYYYMMDD)
- Bestimmt, ob zu DockerHub gepusht werden soll

#### Phase 2: Binary-Erstellung
- Räumt Festplattenspeicher auf (entfernt .NET, Android SDKs, etc.)
- Verwendet vcpkg für Dependency-Management
- Kompiliert ThemisDB mit CMake und Ninja
- Erstellt `themis_server` Binary
- Lädt Binary als Artifact hoch (7 Tage Aufbewahrung)

#### Phase 3: Docker-Image-Erstellung
- Lädt das kompilierte Binary herunter
- Baut Docker-Image mit `docker/Dockerfile.simple`
- Pusht zu DockerHub mit mehreren Tags
- Nutzt Layer-Caching für schnellere Builds

#### Phase 4: Benachrichtigung
- Meldet Build-Status
- Generiert Build-Zusammenfassung
- Stellt Pull-Befehle bereit

### 3. Docker-Tags

Jeder nächtliche Build erzeugt drei Docker-Tags:

- **`themisdb/server:nightly`** - Zeigt immer auf den neuesten nächtlichen Build
- **`themisdb/server:nightly-YYYYMMDD`** - Datumsspezifischer Build (z.B. `nightly-20231221`)
- **`themisdb/server:VERSION-nightly`** - Versionsspezifischer nächtlicher Build (z.B. `1.3.0-nightly`)

## Verwendung der Nightly Builds

### Neuesten Nightly Build herunterladen

```bash
docker pull themisdb/server:nightly
```

### Nightly Build ausführen

```bash
docker run -d \
  -p 8080:8080 \
  -p 18765:18765 \
  -v themisdb-data:/data \
  --name themisdb-nightly \
  themisdb/server:nightly
```

### Spezifischen Nightly Build verwenden

```bash
# Spezifisches Datum verwenden
docker pull themisdb/server:nightly-20231221

# Mit eigener Konfiguration ausführen
docker run -d \
  -p 18765:18765 \
  -v $(pwd)/config.json:/etc/themis/config.json \
  -v themisdb-data:/data \
  themisdb/server:nightly-20231221
```

## Manuelles Auslösen

Der Workflow kann auch manuell über GitHub Actions ausgelöst werden:

1. Zur Repository-Seite "Actions" gehen
2. "Nightly Build & DockerHub Push" Workflow auswählen
3. "Run workflow" klicken
4. Optionen konfigurieren:
   - **Push to DockerHub**: Push zu DockerHub aktivieren/deaktivieren
   - **Build platforms**: `linux/amd64` (schneller) oder `linux/amd64,linux/arm64` (Multi-Arch) wählen
   - **Enable LLM support**: Build mit llama.cpp Integration
   - **Force build**: Build erzwingen, auch wenn keine Änderungen in den letzten 24 Stunden erkannt wurden

## Erforderliche Konfiguration

### GitHub Secrets

Die folgenden Secrets müssen in den Repository-Einstellungen konfiguriert sein:

| Secret | Zweck |
|--------|-------|
| `DOCKER_USERNAME` | DockerHub-Benutzername |
| `DOCKER_TOKEN` | DockerHub-Zugriffstoken |

### Secrets einrichten

1. Zu Repository-Einstellungen gehen
2. "Secrets and variables" → "Actions" navigieren
3. Erforderliche Secrets hinzufügen:
   - `DOCKER_USERNAME`: Ihr DockerHub-Benutzername
   - `DOCKER_TOKEN`: Token unter https://hub.docker.com/settings/security generieren

## Build-Optimierung

### Caching-Strategie

Der Workflow verwendet GitHub Actions Cache zur Beschleunigung:
- **vcpkg Cache**: Kompilierte Dependencies (~1-2 GB)
- **Docker Layer Cache**: Docker Build Layer
- **Build Artifacts**: Binary Artifacts zwischen Jobs

### Festplattenspeicher-Management

Der Workflow räumt automatisch unnötige Dateien auf:
- Entfernt .NET SDK (~2 GB)
- Entfernt Android SDK (~8 GB)
- Entfernt GHC (~4 GB)
- Entfernt CodeQL (~5 GB)

## Build-Status überwachen

### GitHub Actions UI

1. Zur Repository-Seite "Actions" gehen
2. "Nightly Build & DockerHub Push" auswählen
3. Aktuelle Workflow-Ausführungen und Status anzeigen

### Build-Artifacts

Jeder Build erzeugt:
- Binary Artifact (`themis_server_nightly`) - 7 Tage aufbewahrt
- Build-Zusammenfassung im Workflow-Run
- Docker Images auf DockerHub

## Wichtige Hinweise

### Unterschied zu Release-Builds

- **Nightly Builds**: Entwicklungs-Snapshots, getaggt mit `nightly`
- **Release Builds**: Stabile Releases, ausgelöst durch Version-Tags (z.B. `v1.3.0`)
- **Release Workflow**: `.github/workflows/release.yml`

### Best Practices

1. **Verwendung von Nightly Builds**
   - Für Tests der neuesten Features verwenden
   - Nicht für Produktions-Deployments empfohlen
   - Kann instabilen oder experimentellen Code enthalten

2. **Aktualisierung von Nightly**
   - Immer vor dem Test aktualisieren: `docker pull themisdb/server:nightly`
   - Build-Datum prüfen für neueste Version
   - CHANGELOG.md für aktuelle Änderungen überprüfen

3. **Probleme melden**
   - Nightly Build Datum/Tag in Bug-Reports angeben
   - Relevante Logs vom Container bereitstellen
   - Prüfen, ob Problem im neuesten Nightly existiert

## Technische Details

### Workflow-Datei

`.github/workflows/nightly-build.yml`

### Verwendete Docker-Datei

`docker/Dockerfile.simple` - Verwendet vorkompiliertes Binary für schnellen Build

### Unterstützte Plattformen

- **linux/amd64**: Vollständig unterstützt (Standard)
- **linux/arm64**: Unterstützt (via manuellen Trigger mit Multi-Arch-Option)

## Weitere Dokumentation

- [Englische Dokumentation](deployment_nightly_builds.md) - Vollständige technische Details
- [Docker Multi-Arch Deployment](deployment_docker_multiarch.md)
- [CI/CD Multi-Arch Strategy](deployment_cicd_multiarch.md)
- [Build Strategy Guide](../guides/guides_build_strategy.md)
- [Deployment Strategy](deployment_strategy.md)

## Workflow-Konfiguration

Die Workflow-Datei ist unter `.github/workflows/nightly-build.yml` zu finden und enthält:

- **Trigger**: Cron-Schedule (`0 2 * * *`) und manueller Workflow-Dispatch
- **Jobs**: setup, build-binary, build-docker, notify
- **Secrets**: DOCKER_USERNAME, DOCKER_TOKEN
- **Caching**: vcpkg-Cache und Docker-Layer-Cache
- **Artifacts**: themis_server_nightly (7 Tage Retention)

## Support

Bei Problemen oder Fragen:
1. GitHub Issues erstellen
2. Workflow-Logs überprüfen (Actions Tab)
3. DockerHub-Status überprüfen (hub.docker.com/r/themisdb/server)
