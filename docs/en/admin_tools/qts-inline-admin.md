# QTS Inline Admin UI — Setup & Operations on QNAP NAS

**Version:** 1.0.0  
**Date:** 2026-04-16  
**Category:** Admin Tools  
**Status:** ✅ Production-ready (Phase 1 MVP)

---

[docs](../../README.md) > [en](../INDEX.md) > [admin_tools](./README.md) > feature

---

**Datum:** 2026-04-16  
**Status:** stable  
**Primary (Quelle der Wahrheit):** docker/admin-ui/app/app.js  
**Bezug / Reference:** docker-compose.qnap.yml, docker/admin-ui/Dockerfile, docker/admin-ui/nginx.conf

---

## Table of Contents

- [Overview](#overview)
- [Architecture (Phase 1 MVP)](#architecture-phase-1-mvp)
- [Prerequisites](#prerequisites)
- [Deployment on QNAP](#deployment-on-qnap)
- [Admin UI — Features](#admin-ui--features)
- [Configuration](#configuration)
- [Security Notes (Phase 2)](#security-notes-phase-2)
- [Troubleshooting](#troubleshooting)
- [Roadmap (Phase 3 — QPKG)](#roadmap-phase-3--qpkg)

---

## Overview

The **QTS Inline Admin UI** is a lightweight web interface for ThemisDB,
running as a sidecar container alongside the ThemisDB database container
in QNAP Container Station.

**Access points after deployment:**

| Service             | URL                              |
|---------------------|----------------------------------|
| Admin UI            | `http://<QNAP-IP>:18766`         |
| ThemisDB HTTP API   | `http://<QNAP-IP>:18765`         |
| ThemisDB gRPC       | `<QNAP-IP>:18081`                |
| Prometheus Metrics  | `http://<QNAP-IP>:19090/metrics` |

---

## Architecture (Phase 1 MVP)

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
       Browser (QTS Desktop / external)
```

The nginx container has two responsibilities:

1. **Static file serving** — HTML/CSS/JS of the single-page app.
2. **Reverse proxy** — `/api/*` → `http://themis:8080/` (prefix is stripped).
   This avoids any CORS configuration on the ThemisDB server itself.

---

## Prerequisites

- QNAP NAS running Container Station (Docker Engine ≥ 20.10)
- Internet access for initial image pull from Docker Hub
- Available ports: 18765, 18766, 18081, 19090

---

## Deployment on QNAP

### Option A — Container Station GUI

1. Open **Container Station** in QTS.
2. Click **Create Application** → **Upload Compose File**.
3. Upload `docker-compose.qnap.yml` from the ThemisDB repository.
4. Click **Create**.
5. After startup: open the Admin UI at `http://<QNAP-IP>:18766`.

### Option B — SSH / Shell

```bash
# Clone the repository or transfer files to QNAP
cd /share/Container/themisdb

# Build the sidecar Admin UI (once)
docker build -t themisdb-admin-ui:1.0.0 docker/admin-ui/

# Start the stack
docker compose -f docker-compose.qnap.yml up -d

# Check status
docker compose -f docker-compose.qnap.yml ps
```

### Logs & Diagnostics

```bash
# ThemisDB logs
docker logs themisdb

# Admin UI logs (nginx access/error)
docker logs themisdb-admin-ui
```

---

## Admin UI — Features

| Section         | Description                                                  |
|-----------------|--------------------------------------------------------------|
| **Dashboard**   | Server status, version, uptime, total requests, DB size      |
| **Collections** | List of all collections with document count and size         |
| **AQL Query**   | Interactive AQL query editor (Ctrl+Enter = Execute)          |
| **Backup**      | Trigger a full backup to a server-side path                  |
| **Restore**     | Restore the database from an existing backup                 |
| **Monitoring**  | Raw Prometheus metrics (text/plain)                          |

### Quick Access

- **OpenAPI Spec:** link in the navigation bar → `/openapi.json`
- **Metrics:** directly via the Monitoring page or `/api/metrics`

---

## Configuration

### ThemisDB Environment Variables

| Variable                  | Default        | Description                        |
|---------------------------|----------------|------------------------------------|
| `THEMIS_EDITION`          | `COMMUNITY`    | Database edition                   |
| `THEMIS_LOG_LEVEL`        | `info`         | Log verbosity                      |
| `THEMIS_DATA_DIR`         | (internal)     | Data directory inside container    |
| `THEMIS_METRICS_ENABLED`  | `true`         | Enable Prometheus endpoint         |
| `THEMIS_AUTH_ENABLED`     | `false`        | Enable authentication              |
| `THEMIS_ADMIN_USER`       | —              | Admin username (when auth enabled) |
| `THEMIS_ADMIN_PASSWORD`   | —              | Admin password (when auth enabled) |

These variables can be set directly in `docker-compose.qnap.yml` under `environment:`.

### Adjusting Resource Limits

The defaults in `docker-compose.qnap.yml` are tuned for NAS hardware:

```yaml
deploy:
  resources:
    limits:
      cpus: '4'
      memory: 4G
```

Adjust the values to match the available RAM on your QNAP device.

---

## Security Notes (Phase 2)

> Phase 1 is an MVP without TLS termination. For production environments the
> following Phase 2 measures should be implemented.

1. **TLS Termination:** Use QNAP's built-in reverse proxy or an additional
   nginx proxy with Let's Encrypt in front of ports 18766/18765.
2. **Authentication:** Set `THEMIS_AUTH_ENABLED=true` and configure RBAC
   roles (`admin:all`) for administration access.
3. **CORS / CSRF:** nginx already sends `X-Frame-Options: SAMEORIGIN`.
   For external access, add CORS headers to `nginx.conf`.
4. **Firewall:** Restrict port 18766 to trusted subnets via QNAP firewall rules.
5. **Audit Logging:** ThemisDB writes audit events to `/var/log/themis`.
   Mount this path as a named volume for persistence.

---

## Troubleshooting

### Admin UI shows "Connecting…"

- Is the ThemisDB container running? `docker ps | grep themisdb`
- Perform a health check: `curl http://<QNAP-IP>:18765/health`
- Check nginx logs: `docker logs themisdb-admin-ui`

### Port 18766 not reachable

- Check firewall settings in QTS → **Control Panel → Security → Firewall**.
- Container status: `docker compose -f docker-compose.qnap.yml ps`

### Backup fails

- ThemisDB must support the Admin API: `POST /admin/backup`.
- Check that the path is writable inside the container.
- Community Edition: backup API may be restricted — check logs.

---

## Roadmap (Phase 3 — QPKG)

For full native QTS integration, ThemisDB can be packaged as a **QPKG**:

- QPKG installs and manages ThemisDB + Admin UI via the QTS App Center.
- Native QTS menu integration and automatic updates.
- Higher development effort (QPKG lifecycle, QTS version compatibility).

Reference: [QNAP QPKG Developer Guide](https://github.com/qnap-dev/QNAP-QPKG-GUIDELINES)

---

## Further Documentation

- [Admin Guide (DE)](../../de/admin_tools/admin_guide.md)
- [OpenAPI Spec](../../openapi.yaml)
- [docker-compose.qnap.yml](../../../docker-compose.qnap.yml)
- [Admin UI Dockerfile](../../../docker/admin-ui/Dockerfile)
