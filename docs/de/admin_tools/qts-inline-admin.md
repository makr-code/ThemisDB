# QTS Inline Admin UI — Setup & Betrieb auf QNAP NAS

**Version:** 1.0.0  
**Stand:** 2026-04-16  
**Kategorie:** Admin Tools  
**Status:** ✅ Produktionsreif (Phase 1 MVP)

---

[docs](../../README.md) > [de](../INDEX.md) > [admin_tools](./README.md) > feature

---

**Datum:** 2026-04-16  
**Status:** stable  
**Primary (Quelle der Wahrheit):** docker/admin-ui/app/app.js  
**Bezug / Reference:** docker-compose.qnap.yml, docker/admin-ui/Dockerfile, docker/admin-ui/nginx.conf

---

## 📑 Inhaltsverzeichnis

- [Überblick](#überblick)
- [Architektur (Phase 1 MVP)](#architektur-phase-1-mvp)
- [Voraussetzungen](#voraussetzungen)
- [Deployment auf QNAP](#deployment-auf-qnap)
- [Admin UI — Funktionen](#admin-ui--funktionen)
- [Konfiguration](#konfiguration)
- [Sicherheitshinweise (Phase 2)](#sicherheitshinweise-phase-2)
- [Troubleshooting](#troubleshooting)
- [Ausbaupfad (Phase 3 — QPKG)](#ausbaupfad-phase-3--qpkg)

---

## Überblick

Das **QTS Inline Admin UI** ist eine schlanke Web-Oberfläche für ThemisDB,
die als Sidecar-Container neben dem ThemisDB-Datenbankcontainer in
QNAP Container Station betrieben wird.

**Zugangspunkte nach dem Deployment:**

| Dienst              | URL                              |
|---------------------|----------------------------------|
| Admin UI            | `http://<QNAP-IP>:18766`         |
| ThemisDB HTTP API   | `http://<QNAP-IP>:18765`         |
| ThemisDB gRPC       | `<QNAP-IP>:18081`                |
| Prometheus Metrics  | `http://<QNAP-IP>:19090/metrics` |

---

## Architektur (Phase 1 MVP)

```
  QNAP Container Station
  ┌─────────────────────────────────────────────────────┐
  │                                                     │
  │  ┌─────────────────┐       ┌───────────────────┐   │
  │  │  Admin UI        │       │  ThemisDB         │   │
  │  │  (nginx:alpine)  │──────▶│  (makrcode/       │   │
  │  │  Port: 18766     │ /api/ │   themisdb:latest)│   │
  │  │  Static SPA      │       │  Port: 18765      │   │
  │  └────────┬─────────┘       └───────────────────┘   │
  │           │ themis-net (bridge)                      │
  └───────────┼─────────────────────────────────────────┘
              │
       Browser (QTS Desktop / extern)
```

Der nginx-Container übernimmt zwei Aufgaben:

1. **Statische Dateiauslieferung** — HTML/CSS/JS der Single-Page-App.
2. **Reverse-Proxy** — `/api/*` → `http://themis:8080/` (Prefix wird entfernt).
   Dies vermeidet CORS-Konfiguration auf dem ThemisDB-Server.

---

## Voraussetzungen

- QNAP NAS mit Container Station (Docker Engine ≥ 20.10)
- Internetzugang für den initialen Image-Pull von Docker Hub
- Freie Ports: 18765, 18766, 18081, 19090

---

## Deployment auf QNAP

### Option A — Container Station GUI

1. Öffnen Sie **Container Station** in QTS.
2. Klicken Sie **Anwendungen erstellen** → **Compose-Datei hochladen**.
3. Laden Sie `docker-compose.qnap.yml` aus dem ThemisDB-Repository hoch.
4. Klicken Sie **Erstellen**.
5. Nach dem Start: Admin UI unter `http://<QNAP-IP>:18766` öffnen.

### Option B — SSH / Shell

```bash
# Repository auf QNAP klonen oder Dateien übertragen
cd /share/Container/themisdb

# Sidecar Admin UI bauen (einmalig)
docker build -t themisdb-admin-ui:1.0.0 docker/admin-ui/

# Stack starten
docker compose -f docker-compose.qnap.yml up -d

# Status prüfen
docker compose -f docker-compose.qnap.yml ps
```

### Logs & Diagnose

```bash
# ThemisDB-Logs
docker logs themisdb

# Admin-UI-Logs (nginx access/error)
docker logs themisdb-admin-ui
```

---

## Admin UI — Funktionen

| Bereich         | Beschreibung                                                |
|-----------------|-------------------------------------------------------------|
| **Dashboard**   | Serverstatus, Version, Uptime, Gesamtanfragen, DB-Größe     |
| **Collections** | Liste aller Collections mit Dokumentanzahl und Größe        |
| **AQL Query**   | Interaktiver AQL-Abfrageeditor (Strg+Enter = Ausführen)     |
| **Backup**      | Vollsicherung auf serverseitigem Pfad auslösen              |
| **Restore**     | Wiederherstellung aus einem vorhandenen Backup              |
| **Monitoring**  | Rohe Prometheus-Metriken (text/plain)                       |

### Schnellzugriff

- **OpenAPI-Spec:** Link in der Navigationsleiste → `/openapi.json`
- **Metriken:** direkt über die Monitoring-Seite oder `/api/metrics`

---

## Konfiguration

### Umgebungsvariablen ThemisDB

| Variable                  | Standard       | Beschreibung                      |
|---------------------------|----------------|-----------------------------------|
| `THEMIS_EDITION`          | `COMMUNITY`    | Datenbankversion                  |
| `THEMIS_LOG_LEVEL`        | `info`         | Log-Verbosität                    |
| `THEMIS_DATA_DIR`         | (intern)       | Datenpfad im Container            |
| `THEMIS_METRICS_ENABLED`  | `true`         | Prometheus-Endpunkt aktivieren    |
| `THEMIS_AUTH_ENABLED`     | `false`        | Authentifizierung aktivieren      |
| `THEMIS_ADMIN_USER`       | —              | Admin-Benutzername (wenn Auth an) |
| `THEMIS_ADMIN_PASSWORD`   | —              | Admin-Passwort (wenn Auth an)     |

Diese Variablen können direkt in `docker-compose.qnap.yml` unter `environment:` gesetzt werden.

### Resource-Limits anpassen

Die Standardwerte in `docker-compose.qnap.yml` sind für NAS-Hardware ausgelegt:

```yaml
deploy:
  resources:
    limits:
      cpus: '4'
      memory: 4G
```

Passen Sie die Werte entsprechend dem verfügbaren RAM Ihres QNAP an.

---

## Sicherheitshinweise (Phase 2)

> Phase 1 ist ein MVP ohne TLS-Terminierung. Für produktive Umgebungen sollten
> die folgenden Phase-2-Maßnahmen umgesetzt werden.

1. **TLS-Terminierung:** Nutzen Sie QNAPs eingebautes Reverse-Proxy oder einen
   zusätzlichen nginx-Proxy mit Let's Encrypt-Zertifikat vor Port 18766/18765.
2. **Authentifizierung:** Setzen Sie `THEMIS_AUTH_ENABLED=true` und konfigurieren
   Sie RBAC-Rollen (`admin:all`) für den Adminstrationszugriff.
3. **CORS / CSRF:** nginx liefert bereits `X-Frame-Options: SAMEORIGIN`.
   Für externe Zugriffe CORS-Header in nginx.conf ergänzen.
4. **Firewall:** Beschränken Sie Port 18766 auf vertrauenswürdige Subnetze
   über die QNAP-Firewall-Regeln.
5. **Audit-Logging:** ThemisDB schreibt Audit-Events nach `/var/log/themis`.
   Binden Sie diesen Pfad als Named Volume ein.

---

## Troubleshooting

### Admin UI zeigt "Connecting…"

- Ist der ThemisDB-Container gestartet? `docker ps | grep themisdb`
- Health-Check durchführen: `curl http://<QNAP-IP>:18765/health`
- nginx-Logs prüfen: `docker logs themisdb-admin-ui`

### Port 18766 nicht erreichbar

- Firewall-Einstellungen in QTS → **Systemsteuerung → Sicherheit → Firewall** prüfen.
- Container-Status: `docker compose -f docker-compose.qnap.yml ps`

### Backup schlägt fehl

- ThemisDB muss Admin-API unterstützen: `POST /admin/backup`.
- Prüfen, ob der Pfad im Container beschreibbar ist.
- Community Edition: Backup-API eventuell eingeschränkt — Logs prüfen.

---

## Ausbaupfad (Phase 3 — QPKG)

Für eine vollständige native QTS-Integration kann ThemisDB als **QPKG-Paket**
verpackt werden:

- QPKG installiert und verwaltet ThemisDB + Admin UI über das QTS App Center.
- Native QTS-Menüintegration und automatische Updates.
- Höherer Entwicklungsaufwand (QPKG-Lifecycle, QTS-Versionskompatibilität).

Referenz: [QNAP QPKG Developer Guide](https://github.com/qnap-dev/QNAP-QPKG-GUIDELINES)

---

## Weiterführende Dokumentation

- [Admin Guide](./admin_guide.md)
- [OpenAPI Spec](../../openapi.yaml)
- [docker-compose.qnap.yml](../../../docker-compose.qnap.yml)
- [Admin UI Dockerfile](../../../docker/admin-ui/Dockerfile)
