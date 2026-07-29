# CHIMERA DockerHub Release Package

Dieses Paket enthaelt alle Dateien fuer den Build/Push des Docker-Images und den anschliessenden Betrieb per `docker-compose.release.yml`.

Empfohlener Release-Tag fuer dieses Paket: `1.0.0`

## Inhalt

- `Dockerfile`
- `docker-compose.release.yml`
- `.env.release.example`
- `release_dockerhub.ps1`
- `RELEASE_NOTES.md`
- `DOCKER_IMAGE_SCOPE.md`
- `endurance_config.toml`
- `requirements.txt`

## 1) Image bauen und zu DockerHub pushen

Voraussetzungen:

- Docker Desktop / Docker Engine laeuft
- `docker login` auf DockerHub erfolgreich

Beispiel (PowerShell, im Repo-Root):

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\release_dockerhub.ps1 -RequireSemverTag -Tag 1.0.0 -AlsoLatest
```

Optional Multi-Arch:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\release_dockerhub.ps1 -RequireSemverTag -Tag 1.0.0 -MultiArch -AlsoLatest
```

## 2) Deployment mit Release-Compose

1. `.env.release.example` nach `.env` kopieren
2. Werte in `.env` setzen (Repo/Tag, DB-Host, Credentials)
3. Pull + Start:

```powershell
docker compose -f docker-compose.release.yml pull
docker compose -f docker-compose.release.yml up -d
```

## 3) Legacy-Endpoint-Beispiel (ThemisDB Community)

Falls die Zielinstanz Legacy-Routen nutzt, in `.env` setzen:

```dotenv
TEST_DB_HEALTH_ENDPOINT=/health
TEST_DB_QUERY_ENDPOINT=/query
TEST_DB_VECTOR_SEARCH_ENDPOINT=/vector/search
TEST_DB_INFO_ENDPOINTS=/health
TEST_DB_USE_TLS=false
TEST_DB_VERIFY_TLS=false
```

## 4) Scope

Welche Dateien bewusst im Runtime-Image enthalten sind, ist in `DOCKER_IMAGE_SCOPE.md` dokumentiert.
