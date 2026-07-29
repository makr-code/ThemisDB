# CHIMERA Docker Runtime Test Report

Datum: 2026-04-17

## Scope

- Lokale Installation des veroeffentlichten DockerHub-Images in Docker Desktop.
- Start im Release-Modus via Compose.
- Laufzeit-Smoke-Test mit Container-Health, Heartbeat und Ergebnisartefakten.

## Verwendete Artefakte

- Image: `themisdb/chimera:1.0.0`
- Compose: `docker-compose.release.yml`
- Env: `.env.release.local`

## Ergebnis (PASS)

- Image erfolgreich lokal verfuegbar und gepullt.
- Release-Container erfolgreich gestartet und dauerhaft `healthy`.
- Netzwerkanbindung aktiv: `themis_themis-net` + `chimera_default`.
- Runtime-Logs bestaetigen erfolgreiche DB-Verbindung und gestarteten Warm-up.
- Heartbeat und Ergebnisartefakte vorhanden.

## Evidenz

### Container-Status

- `chimera_endurance`: `Up (... healthy)`
- Image im Container: `themisdb/chimera:1.0.0`

### Runtime-Dateien im Container (`/results`)

- `.heartbeat` vorhanden
- `dataset_provisioning.json`
- `result_schema.json`
- `standard_golden_baselines.json`
- `test_topology.json`

### Relevante Log-Signale

- `Verbindungstest zu ThemisDB ...`
- `Verbindung OK.`
- `DB-Info erkannt (/health): ... status=healthy`
- `Warm-up (200 Iterationen pro Workload) ...`

## Hinweise

- Ein separater One-Shot-`docker run` kann ohne Compose-Mapping abweichende Env-Werte nutzen.
- Fuer den Zielbetrieb ist der Compose-Pfad (`docker-compose.release.yml` + `.env.release.local`) der massgebliche Erfolgsnachweis.
