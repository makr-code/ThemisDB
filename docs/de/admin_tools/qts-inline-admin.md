# QTS Inline Admin UI — Setup & Betrieb auf QNAP NAS

**Version:** 1.1.0
**Stand:** 2026-04-16
**Kategorie:** Admin Tools
**Status:** ✅ Produktionsreif (Phase 2 — Security Hardening)

---

[docs](../../README.md) > [de](../INDEX.md) > [admin_tools](./README.md) > feature

---

**Datum:** 2026-04-16
**Status:** stable
**Primary (Quelle der Wahrheit):** docker/admin-ui/app/app.js
**Bezug / Reference:** docker-compose.qnap.yml, docker/admin-ui/Dockerfile, docker/admin-ui/nginx.conf, docker/admin-ui/nginx.ssl.conf

---

## 📑 Inhaltsverzeichnis

- [Überblick](#überblick)
- [Architektur (Phase 2)](#architektur-phase-2)
- [Voraussetzungen](#voraussetzungen)
- [Deployment auf QNAP](#deployment-auf-qnap)
- [Admin UI — Funktionen](#admin-ui--funktionen)
- [Authentifizierung & Sicherheit](#authentifizierung--sicherheit)
- [TLS / HTTPS aktivieren](#tls--https-aktivieren)
- [Konfiguration](#konfiguration)
- [Troubleshooting](#troubleshooting)
- [Ausbaupfad (Phase 3 — QPKG)](#ausbaupfad-phase-3--qpkg)

---

## Überblick

Das **QTS Inline Admin UI** ist eine schlanke Web-Oberfläche für ThemisDB,
die als Sidecar-Container neben dem ThemisDB-Datenbankcontainer in
QNAP Container Station betrieben wird.

Ab **v1.1.0 (Phase 2)** enthält das Admin UI vollständige Security-Hardening-Features:
Authentifizierung mit Bearer-Token, CSRF-Schutz, Rate-Limiting,
CORS-Validierung und optionalen TLS-Support.

**Zugangspunkte nach dem Deployment:**

| Dienst              | URL                               |
|---------------------|-----------------------------------|
| Admin UI (HTTP)     | `http://<QNAP-IP>:18766`          |
| Admin UI (HTTPS)    | `https://<QNAP-IP>:18767` ¹       |
| ThemisDB HTTP API   | `http://<QNAP-IP>:18765`          |
| ThemisDB gRPC       | `<QNAP-IP>:18081`                 |
| Prometheus Metrics  | `http://<QNAP-IP>:19090/metrics`  |

¹ Nur aktiv wenn TLS-Zertifikate eingebunden werden (siehe [TLS / HTTPS aktivieren](#tls--https-aktivieren)).

---

## Architektur (Phase 2)

```
  QNAP Container Station
  ┌────────────────────────────────────────────────────────────────┐
  │                                                                │
  │  Browser                                                       │
  │  (QTS Desktop / extern)                                        │
  │         │                                                      │
  │  :18766 (HTTP) / :18767 (HTTPS, optional)                      │
  │         ▼                                                       │
  │  ┌──────────────────────────────┐                              │
  │  │  Admin UI (nginx:1.25-alpine) │                              │
  │  │  ─────────────────────────── │                              │
  │  │  • Rate-Limiting (30r/m API, │                              │
  │  │    5r/m Login)               │                              │
  │  │  • CORS/Origin-Validierung   │                              │
  │  │  • CSRF-Header-Prüfung       │──────────────────────────┐   │
  │  │  • /api/* → Reverse-Proxy    │  /api/                   │   │
  │  │  • Statische SPA (HTML/JS)   │                          ▼   │
  │  └──────────────────────────────┘     ┌────────────────────┐   │
  │                                       │  ThemisDB          │   │
  │  SPA (Vanilla JS, kein Build-Step):   │  (makrcode/        │   │
  │  • Login-Overlay (Bearer-Token)       │   themisdb:latest) │   │
  │  • CSRF-Nonce (crypto.getRandomValues)│  Port: 18765       │   │
  │  • 401-Interception → Login           └────────────────────┘   │
  │  • Logout (DELETE /auth/sessions/{id})                         │
  │                                                                │
  │  Volumes: themis-data, themis-logs (ro auf Admin UI)           │
  └────────────────────────────────────────────────────────────────┘
```

---

## Voraussetzungen

- QNAP NAS mit Container Station (Docker Engine ≥ 20.10)
- Internetzugang für den initialen Image-Pull von Docker Hub
- Freie Ports: 18765, 18766, 18081, 19090
- Optional für HTTPS: TLS-Zertifikat + Schlüssel (PEM-Format)

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

# Sidecar Admin UI bauen (einmalig, oder nach Änderungen)
docker build -t themisdb-admin-ui:1.1.0 docker/admin-ui/

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

## Authentifizierung & Sicherheit

### Authentifizierung aktivieren

Setzen Sie in `docker-compose.qnap.yml` unter `environment:`:

```yaml
- THEMIS_AUTH_ENABLED=true
- THEMIS_ADMIN_USER=admin
- THEMIS_ADMIN_PASSWORD=<sicheres-passwort>
- THEMIS_MFA_REQUIRED_ROLES=admin,operator   # MFA-Pflicht für Admin-Rolle
```

Nach dem Neustart des Stacks zeigt das Admin UI ein **Login-Formular**.
Der Nutzer authentifiziert sich mit Benutzername/Passwort; bei Erfolg
speichert die SPA das Bearer-Token in `sessionStorage` (wird beim Schließen
des Tabs automatisch gelöscht).

### CSRF-Schutz

Alle zustandsändernden Requests (POST/PUT/PATCH/DELETE) senden automatisch
einen `X-CSRF-Token`-Header. nginx prüft diesen Header serverseitig und lehnt
Requests ohne gültiges Token mit HTTP 403 ab.

Ausnahme: der Login-Endpunkt (`POST /api/auth/sessions`) ist ohne CSRF-Token
erreichbar — er ist durch Rate-Limiting geschützt (max. 5 Versuche/Minute).

### Rate-Limiting

nginx begrenzt Anfragen pro Client-IP:

| Zone         | Limit        | Burst | Endpunkte             |
|--------------|--------------|-------|-----------------------|
| `admin_api`  | 30 req/min   | 10    | alle `/api/`-Pfade    |
| `admin_login`| 5 req/min    | 3     | `POST /api/auth/sessions` |

Bei Überschreitung: HTTP **429** mit JSON-Body `{"error":"rate_limit_exceeded"}`.

### CORS / Origin-Validierung

nginx lehnt Anfragen von unbekannten Origins mit HTTP 403 ab.
Erlaubte Origins sind in `docker/admin-ui/nginx.conf` unter
`map $http_origin $cors_allowed` konfiguriert.

Standard-Whitelist:
- Kein Origin-Header (Same-Origin / LAN-Zugriff)
- `http(s)://localhost` (beliebiger Port)
- `http(s)://127.0.0.1` (beliebiger Port)

**Eigene QNAP-Hostname hinzufügen:**

```nginx
# docker/admin-ui/nginx.conf — map $http_origin $cors_allowed
~^https?://qnap\.local(:[0-9]+)?$ "1";
```

### Audit-Log-Mount

Das Audit-Log von ThemisDB (`/var/log/themis`) wird schreibgeschützt in den
Admin-UI-Container eingebunden (`themis-logs:/var/log/themis:ro`).
Dies erlaubt künftige Log-Ansicht über die Admin UI ohne Schreibrechte.

### Logout

Der **Sign-out**-Button oben rechts ruft `DELETE /api/auth/sessions/{id}` auf
und löscht Token und CSRF-Nonce aus dem `sessionStorage`.

---

## TLS / HTTPS aktivieren

### Option A — QNAP Reverse Proxy (empfohlen)

Nutzen Sie QNAPs integrierten Reverse-Proxy in:
**Systemsteuerung → Anwendungsportal → Reverse-Proxy**

- Ziel: `http://localhost:18766`
- HTTPS-Zertifikat über das QNAP-Zertifikatsverwaltung einrichten.
- Kein Eingriff in die Container-Konfiguration notwendig.

### Option B — Direktes TLS auf dem Container

1. Legen Sie Zertifikat und Schlüssel bereit (PEM-Format):
   ```
   /share/certs/admin.crt   ← vollständige Zertifikatskette
   /share/certs/admin.key   ← privater Schlüssel
   ```

2. Kommentieren Sie in `docker-compose.qnap.yml` folgende Zeilen ein:
   ```yaml
   ports:
     - "18767:443"     # HTTPS
   volumes:
     - /share/certs/admin.crt:/etc/nginx/ssl/cert.crt:ro
     - /share/certs/admin.key:/etc/nginx/ssl/cert.key:ro
   ```

3. Ersetzen Sie `docker/admin-ui/nginx.conf` durch `docker/admin-ui/nginx.ssl.conf`:
   ```bash
   cp docker/admin-ui/nginx.ssl.conf docker/admin-ui/nginx.conf
   docker compose -f docker-compose.qnap.yml up -d --build admin-ui
   ```

Die TLS-Konfiguration unterstützt TLS 1.2 und TLS 1.3, moderne
Cipher-Suites (ECDHE/AES-GCM/ChaCha20), deaktivierte Session-Tickets
und optionales OCSP-Stapling.

### Option C — Let's Encrypt via acme.sh

Richten Sie einen acme.sh-Sidecar-Container ein, der Zertifikate ausstellt
und erneuert. Binden Sie die resultierenden `.crt`/`.key`-Dateien wie in
Option B beschrieben ein.

---

## Konfiguration

### Umgebungsvariablen ThemisDB

| Variable                      | Standard       | Beschreibung                           |
|-------------------------------|----------------|----------------------------------------|
| `THEMIS_EDITION`              | `COMMUNITY`    | Datenbankversion                       |
| `THEMIS_LOG_LEVEL`            | `info`         | Log-Verbosität                         |
| `THEMIS_DATA_DIR`             | (intern)       | Datenpfad im Container                 |
| `THEMIS_METRICS_ENABLED`      | `true`         | Prometheus-Endpunkt aktivieren         |
| `THEMIS_AUTH_ENABLED`         | `false`        | Authentifizierung aktivieren           |
| `THEMIS_ADMIN_USER`           | —              | Admin-Benutzername (wenn Auth an)      |
| `THEMIS_ADMIN_PASSWORD`       | —              | Admin-Passwort (wenn Auth an)          |
| `THEMIS_MFA_REQUIRED_ROLES`   | —              | MFA-Pflicht für Rollen (z. B. `admin,operator`) |

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

## Troubleshooting

### Admin UI zeigt "Connecting…"

- Ist der ThemisDB-Container gestartet? `docker ps | grep themisdb`
- Health-Check durchführen: `curl http://<QNAP-IP>:18765/health`
- nginx-Logs prüfen: `docker logs themisdb-admin-ui`

### Admin UI zeigt "Auth required" / Login-Formular erscheint

- Erwartetes Verhalten wenn `THEMIS_AUTH_ENABLED=true` gesetzt ist.
- Zugangsdaten aus `THEMIS_ADMIN_USER` / `THEMIS_ADMIN_PASSWORD` verwenden.
- Nach 5 Fehlversuchen greift das Rate-Limit (429 — 1 Minute warten).

### HTTP 403 — "csrf_required"

- Der Browser sendet keinen `X-CSRF-Token`-Header.
- Stellen Sie sicher, dass JavaScript aktiviert ist und die SPA korrekt geladen wurde.
- Leeren Sie den Browser-Cache und laden Sie die Seite neu.

### HTTP 403 — "cors_forbidden"

- Die Origin Ihres Browsers ist nicht in der CORS-Whitelist von nginx.
- Bearbeiten Sie `docker/admin-ui/nginx.conf` und fügen Sie Ihren QNAP-Hostname hinzu.
- Rebuild: `docker compose -f docker-compose.qnap.yml up -d --build admin-ui`

### HTTP 429 — "rate_limit_exceeded"

- Zu viele Anfragen in kurzer Zeit. 1 Minute warten, dann erneut versuchen.
- Bei wiederholten Problemen: nginx-Konfiguration (`limit_req_zone`) anpassen.

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
- [nginx.ssl.conf (TLS)](../../../docker/admin-ui/nginx.ssl.conf)

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
