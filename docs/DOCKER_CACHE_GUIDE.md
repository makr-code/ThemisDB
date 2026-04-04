# Docker Build mit vcpkg Cache

Diese Dateien ermöglichen schnellere Docker-Builds durch Nutzung deiner lokalen vcpkg Binary-Cache-Artefakte.

## Schnellstart

```powershell
# Einfacher Build mit Cache
.\docker-build-with-cache.ps1

# Oder mit Parametern
.\docker-build-with-cache.ps1 -Edition ENTERPRISE -Tag themisdb:enterprise
```

## Manueller 2-Schritt Prozess

### Schritt 1: Helper-Image erstellen

```powershell
docker build -f Dockerfile.prebuilt-local -t themis-prebuilt .
```

Dies erstellt ein minimales Image mit deinen vcpkg-Paketen aus `.\vcpkg\packages`.

### Schritt 2: Haupt-Image bauen

```powershell
docker buildx build `
  --build-context prebuilt=docker-image://themis-prebuilt `
  --build-arg ENABLE_VCPKG_CACHE=ON `
  --build-arg THEMIS_EDITION=COMMUNITY `
  -t themisdb:latest .
```

## Ohne Cache bauen

```powershell
# Verwendet die scratch-Dummy-Stage
docker buildx build -t themisdb:latest .
```

## Vorteile

- **Schnellerer Build**: vcpkg nutzt bereits kompilierte Pakete aus deinem Windows-Build
- **Weniger Netzwerk**: Keine wiederholten Downloads der gleichen Abhängigkeiten
- **Konsistenz**: Gleiche Paketversionen wie im Windows-Build

## Wie es funktioniert

1. `Dockerfile.prebuilt-local` packt deine `vcpkg/packages/*_x64-linux` Verzeichnisse in ein scratch-Image
2. Das Haupt-`Dockerfile` mounted dieses Image und kopiert die x64-linux Pakete nach `$VCPKG_ROOT/packages`
3. vcpkg erkennt die Pakete automatisch und überspringt deren Neubau
4. Nur fehlende oder nicht gecachte Pakete werden neu gebaut

## Voraussetzungen

- Docker mit BuildKit-Unterstützung (Docker Desktop 19.03+)
- Vorhandene vcpkg-Pakete in `.\vcpkg\packages\` (mindestens `*_x64-linux`)
- Die Pakete sollten von einem vorherigen lokalen Build stammen

## Dateien

- `Dockerfile.prebuilt-local` - Helper-Image Definition
- `docker-build-with-cache.ps1` - Automatisiertes Build-Skript
- `DOCKER_CACHE_GUIDE.md` - Diese Anleitung
