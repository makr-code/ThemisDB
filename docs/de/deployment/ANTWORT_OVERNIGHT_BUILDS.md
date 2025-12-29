# Antwort: Wie funktionieren overnight builds für ThemisDB inklusive DockerHub push?

## Kurze Antwort

Die overnight builds für ThemisDB funktionieren automatisch über GitHub Actions:

1. **Jeden Tag um 2:00 Uhr UTC** wird geprüft, ob es Änderungen gab
2. **Nur bei Änderungen** wird automatisch ein Build gestartet (spart Ressourcen)
3. ThemisDB wird **von Grund auf kompiliert** (inkl. aller Dependencies via vcpkg)
4. Ein **Docker-Image wird erstellt** mit dem neuen Binary
5. Das Image wird automatisch **zu DockerHub hochgeladen** mit drei Tags:
   - `themisdb/server:nightly` (immer aktuellste)
   - `themisdb/server:nightly-20231221` (datumsspezifisch)
   - `themisdb/server:1.3.0-nightly` (versionsspezifisch)

## Detaillierte Erklärung

### 1. Automatischer Zeitplan

```yaml
schedule:
  - cron: '0 2 * * *'  # Täglich um 2:00 Uhr UTC
```

**Wichtig**: Der Build läuft vollautomatisch, aber **nur wenn Änderungen** im Repository in den letzten 24 Stunden erkannt wurden. Dies spart Ressourcen und GitHub Actions Minuten.

### 2. Build-Prozess (5 Phasen)

#### Phase 0: Änderungserkennung (~1 Minute)
- Prüft auf Commits in den letzten 24 Stunden
- Bei Änderungen: Build wird gestartet
- Ohne Änderungen: Build wird übersprungen
- Manuelle Ausführung kann Build erzwingen

#### Phase 1: Setup (~2 Minuten)
- Liest Version aus `VERSION` Datei (aktuell: 1.3.0)
- Generiert Build-Datum (Format: YYYYMMDD)
- Bestimmt Push-Strategie

#### Phase 2: Binary erstellen (~30-40 Minuten)
```bash
# Systemvorbereitung
- Festplatte aufräumen (~5 Min)
- vcpkg Cache wiederherstellen (~2 Min)
- System-Dependencies installieren (~2 Min)

# Kompilierung
- CMake konfigurieren (~5 Min)
- Ninja Build ausführen (~20-30 Min)
- Binary als Artifact hochladen (~1 Min)
```

#### Phase 3: Docker-Image (~5-10 Minuten)
```bash
# Docker Build
- Binary herunterladen
- QEMU Setup (Multi-Arch-Support)
- Docker Buildx Setup
- DockerHub Login (mit Secrets)
- Image bauen (Dockerfile.simple)
- Zu DockerHub pushen (3 Tags)
```

#### Phase 4: Benachrichtigung (~1 Minute)
- Build-Status melden
- Pull-Befehle bereitstellen
- Zusammenfassung generieren

### 3. Konfiguration

Erforderliche GitHub Secrets:
```yaml
DOCKER_USERNAME: themisdb  # DockerHub Benutzername
DOCKER_TOKEN: dckr_...     # DockerHub Access Token
```

### 4. Technische Details

**Workflow-Datei**: `.github/workflows/nightly-build.yml`

**Docker-Datei**: `docker/Dockerfile.simple` (nutzt vorkompiliertes Binary)

**Verwendete Tools**:
- GitHub Actions (Build-Orchestrierung)
- vcpkg (Dependency-Management)
- CMake + Ninja (Build-System)
- Docker Buildx (Multi-Arch-Images)
- GitHub Actions Cache (Beschleunigung)

**Optimierungen**:
- vcpkg-Cache spart ~30-40 Minuten
- Docker Layer-Cache beschleunigt Image-Build
- Festplatten-Cleanup schafft ~19 GB Platz
- Binary-Artifact vermeidet Rebuild in Docker-Phase

### 5. Verwendung

**Latest Nightly herunterladen**:
```bash
docker pull themisdb/server:nightly
```

**Nightly Build ausführen**:
```bash
docker run -d \
  -p 18765:18765 \
  -v themisdb-data:/data \
  themisdb/server:nightly
```

**Spezifisches Datum verwenden**:
```bash
docker pull themisdb/server:nightly-20231221
```

### 6. Manuelles Auslösen

Der Build kann auch manuell gestartet werden:

1. GitHub → Actions Tab
2. "Nightly Build & DockerHub Push" auswählen
3. "Run workflow" klicken
4. Optionen wählen:
   - Push to DockerHub: Ja/Nein
   - Build platforms: linux/amd64 oder Multi-Arch
   - Enable LLM support: Ja/Nein

### 7. Monitoring

**Build-Status prüfen**:
- GitHub Actions Tab → Workflow-Ausführungen
- DockerHub → https://hub.docker.com/r/themisdb/server/tags

**Bei Problemen**:
- Workflow-Logs in GitHub Actions überprüfen
- Secrets-Konfiguration verifizieren
- DockerHub-Account-Berechtigungen prüfen

## Zusammenfassung

Die overnight builds funktionieren **vollautomatisch**:

- ✅ **Täglich** um 2:00 Uhr UTC
- ✅ **Automatisch** kompiliert und deployed
- ✅ **Optimiert** durch Caching (Build-Zeit: ~35-50 Min)
- ✅ **Zuverlässig** durch GitHub Actions
- ✅ **Verfügbar** auf DockerHub mit 3 Tags

Die neueste Entwicklungsversion ist somit **immer verfügbar** für Tests und Deployment.

## Dokumentation

Vollständige Dokumentation:

- **Quick Reference**: [NIGHTLY_BUILDS_QUICKREF.md](NIGHTLY_BUILDS_QUICKREF.md)
- **Technische Details**: [deployment_nightly_builds_de.md](deployment_nightly_builds_de.md)
- **Setup-Anleitung**: [SETUP_NIGHTLY_BUILDS.md](SETUP_NIGHTLY_BUILDS.md)
- **Implementation**: [OVERNIGHT_BUILDS_SUMMARY.md](OVERNIGHT_BUILDS_SUMMARY.md)
- **English Version**: [deployment_nightly_builds.md](deployment_nightly_builds.md)

---

**Status**: ✅ Implementiert und einsatzbereit  
**Workflow**: `.github/workflows/nightly-build.yml`  
**Version**: ThemisDB 1.3.0  
**Datum**: Dezember 2024
