# CHIMERA DockerHub Release Notes

Version: 1.0.0  
Datum: 2026-04-17  
Release-Typ: DockerHub Package

## Enthaltene Verbesserungen

- Runtime-Image auf explizite Whitelist gehaertet (kein pauschales Copy des gesamten Repos).
- Endurance-Client um konfigurierbare Endpoints erweitert:
  - Health Endpoint
  - Query Endpoint
  - Vector Search Endpoint
  - Fallback-Info-Endpoints
- Endpoint-Konfiguration aus Environment und TOML integriert.
- API-Kompatibilitaet fuer v1- und Legacy-Routen mit Fallbacklogik verbessert.
- Import-Robustheit fuer Flat-Script- und Package-Modus stabilisiert.
- Docker-Compose-Release-Konfiguration fuer DockerHub-Deployments ergaenzt.

## Konfigurationshinweise

Typische Legacy-Zielsysteme nutzen folgende Endpoints:

- `/health`
- `/query`
- `/vector/search`

Diese koennen ueber Umgebungsvariablen gesetzt werden:

- `TEST_DB_HEALTH_ENDPOINT`
- `TEST_DB_QUERY_ENDPOINT`
- `TEST_DB_VECTOR_SEARCH_ENDPOINT`
- `TEST_DB_INFO_ENDPOINTS`

## Validierungsstatus

- Docker Build erfolgreich.
- Release-DryRun erfolgreich.
- Compose-Release-Konfiguration validiert.
- Konnektivitaet im Container-Netz mit Legacy-Endpoints erfolgreich verifiziert.

## Bekannte Grenzen

- Dieses Paket erstellt Release-Artefakte, fuehrt aber keinen automatischen Push ohne expliziten Aufruf des Release-Skripts aus.
- Multi-Arch Push ist optional und muss beim Release-Lauf bewusst aktiviert werden.
