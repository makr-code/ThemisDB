# QTS Inline Admin UI — Setup & Operations on QNAP NAS

**Version:** 1.1.0
**Date:** 2026-04-16
**Category:** Admin Tools
**Status:** ✅ Production-ready (Phase 2 — Security Hardening)

---

[docs](../../README.md) > [en](../INDEX.md) > [admin_tools](./README.md) > feature

---

**Datum:** 2026-04-16
**Status:** stable
**Primary (Quelle der Wahrheit):** docker/admin-ui/app/app.js
**Bezug / Reference:** docker-compose.qnap.yml, docker/admin-ui/Dockerfile, docker/admin-ui/nginx.conf, docker/admin-ui/nginx.ssl.conf

---

## Table of Contents

- [Overview](#overview)
- [Architecture (Phase 2)](#architecture-phase-2)
- [Prerequisites](#prerequisites)
- [Deployment on QNAP](#deployment-on-qnap)
- [Admin UI — Features](#admin-ui--features)
- [Authentication & Security](#authentication--security)
- [TLS / HTTPS Setup](#tls--https-setup)
- [Configuration](#configuration)
- [Troubleshooting](#troubleshooting)
- [Roadmap (Phase 3 — QPKG)](#roadmap-phase-3--qpkg)

---

## Overview

The **QTS Inline Admin UI** is a lightweight web interface for ThemisDB,
running as a sidecar container alongside the ThemisDB database container
in QNAP Container Station.

Starting with **v1.1.0 (Phase 2)**, the Admin UI includes full security
hardening: Bearer-token authentication, CSRF protection, rate limiting,
CORS validation, and optional TLS support.

**Access points after deployment:**

| Service             | URL                               |
|---------------------|-----------------------------------|
| Admin UI (HTTP)     | `http://<QNAP-IP>:18766`          |
| Admin UI (HTTPS)    | `https://<QNAP-IP>:18767` ¹       |
| ThemisDB HTTP API   | `http://<QNAP-IP>:18765`          |
| ThemisDB gRPC       | `<QNAP-IP>:18081`                 |
| Prometheus Metrics  | `http://<QNAP-IP>:19090/metrics`  |

¹ Only active when TLS certificates are mounted (see [TLS / HTTPS Setup](#tls--https-setup)).

---

## Architecture (Phase 2)

```
  QNAP Container Station
  ┌────────────────────────────────────────────────────────────────┐
  │                                                                │
  │  Browser                                                       │
  │  (QTS Desktop / external)                                      │
  │         │                                                      │
  │  :18766 (HTTP) / :18767 (HTTPS, optional)                      │
  │         ▼                                                       │
  │  ┌──────────────────────────────┐                              │
  │  │  Admin UI (nginx:1.25-alpine) │                              │
  │  │  ─────────────────────────── │                              │
  │  │  • Rate limiting (30r/m API, │                              │
  │  │    5r/m Login)               │                              │
  │  │  • CORS/Origin validation    │                              │
  │  │  • CSRF header enforcement   │──────────────────────────┐   │
  │  │  • /api/* → Reverse proxy    │  /api/                   │   │
  │  │  • Static SPA (HTML/JS)      │                          ▼   │
  │  └──────────────────────────────┘     ┌────────────────────┐   │
  │                                       │  ThemisDB          │   │
  │  SPA (Vanilla JS, no build step):     │  (makrcode/        │   │
  │  • Login overlay (Bearer token)       │   themisdb:latest) │   │
  │  • CSRF nonce (crypto.getRandomValues)│  Port: 18765       │   │
  │  • 401 interception → login           └────────────────────┘   │
  │  • Logout (DELETE /auth/sessions/{id})                         │
  │                                                                │
  │  Volumes: themis-data, themis-logs (ro on Admin UI)            │
  └────────────────────────────────────────────────────────────────┘
```

---

## Prerequisites

- QNAP NAS running Container Station (Docker Engine ≥ 20.10)
- Internet access for initial image pull from Docker Hub
- Available ports: 18765, 18766, 18081, 19090
- Optional for HTTPS: TLS certificate + key (PEM format)

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

# Build the sidecar Admin UI (once, or after changes)
docker build -t themisdb-admin-ui:1.1.0 docker/admin-ui/

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

## Authentication & Security

### Enabling Authentication

In `docker-compose.qnap.yml` under `environment:`, uncomment and set:

```yaml
- THEMIS_AUTH_ENABLED=true
- THEMIS_ADMIN_USER=admin
- THEMIS_ADMIN_PASSWORD=<secure-password>
- THEMIS_MFA_REQUIRED_ROLES=admin,operator   # enforce MFA for admin role
```

After restarting the stack, the Admin UI displays a **login form**.
The user authenticates with username/password; on success the SPA stores
the Bearer token in `sessionStorage` (automatically cleared when the tab
is closed).

### CSRF Protection

All state-changing requests (POST/PUT/PATCH/DELETE) automatically send an
`X-CSRF-Token` header. nginx validates this header server-side and rejects
requests without a valid token with HTTP 403.

Exception: the login endpoint (`POST /api/auth/sessions`) is accessible
without a CSRF token — it is protected by rate limiting instead
(max. 5 attempts/minute).

### Rate Limiting

nginx limits requests per client IP:

| Zone          | Limit       | Burst | Endpoints                     |
|---------------|-------------|-------|-------------------------------|
| `admin_api`   | 30 req/min  | 10    | all `/api/` paths             |
| `admin_login` | 5 req/min   | 3     | `POST /api/auth/sessions`     |

On limit exceeded: HTTP **429** with JSON body `{"error":"rate_limit_exceeded"}`.

### CORS / Origin Validation

nginx rejects requests from unknown origins with HTTP 403.
Allowed origins are configured in `docker/admin-ui/nginx.conf` under
`map $http_origin $cors_allowed`.

Default whitelist:
- No Origin header (same-origin / LAN access)
- `http(s)://localhost` (any port)
- `http(s)://127.0.0.1` (any port)

**Adding your QNAP hostname:**

```nginx
# docker/admin-ui/nginx.conf — map $http_origin $cors_allowed
~^https?://qnap\.local(:[0-9]+)?$ "1";
```

### Audit Log Mount

The ThemisDB audit log (`/var/log/themis`) is mounted read-only into the
Admin UI container (`themis-logs:/var/log/themis:ro`). This enables future
log viewing via the Admin UI without write access.

### Logout

The **Sign out** button in the top-right corner calls
`DELETE /api/auth/sessions/{id}` and clears the token and CSRF nonce from
`sessionStorage`.

---

## TLS / HTTPS Setup

### Option A — QNAP Reverse Proxy (recommended)

Use QNAP's built-in reverse proxy:
**Control Panel → Application Portal → Reverse Proxy**

- Target: `http://localhost:18766`
- Configure an HTTPS certificate via the QNAP certificate manager.
- No changes to the container configuration required.

### Option B — Direct TLS on the Container

1. Prepare your certificate and key (PEM format):
   ```
   /share/certs/admin.crt   ← full certificate chain
   /share/certs/admin.key   ← private key
   ```

2. In `docker-compose.qnap.yml`, uncomment the following lines:
   ```yaml
   ports:
     - "18767:443"     # HTTPS
   volumes:
     - /share/certs/admin.crt:/etc/nginx/ssl/cert.crt:ro
     - /share/certs/admin.key:/etc/nginx/ssl/cert.key:ro
   ```

3. Replace `docker/admin-ui/nginx.conf` with `docker/admin-ui/nginx.ssl.conf`:
   ```bash
   cp docker/admin-ui/nginx.ssl.conf docker/admin-ui/nginx.conf
   docker compose -f docker-compose.qnap.yml up -d --build admin-ui
   ```

The TLS configuration supports TLS 1.2 and TLS 1.3, modern cipher suites
(ECDHE/AES-GCM/ChaCha20), disabled session tickets, and optional
OCSP stapling.

### Option C — Let's Encrypt via acme.sh

Set up an acme.sh sidecar container to issue and renew certificates.
Mount the resulting `.crt`/`.key` files as described in Option B.

---

## Configuration

### ThemisDB Environment Variables

| Variable                      | Default        | Description                              |
|-------------------------------|----------------|------------------------------------------|
| `THEMIS_EDITION`              | `COMMUNITY`    | Database edition                         |
| `THEMIS_LOG_LEVEL`            | `info`         | Log verbosity                            |
| `THEMIS_DATA_DIR`             | (internal)     | Data directory inside container          |
| `THEMIS_METRICS_ENABLED`      | `true`         | Enable Prometheus endpoint               |
| `THEMIS_AUTH_ENABLED`         | `false`        | Enable authentication                    |
| `THEMIS_ADMIN_USER`           | —              | Admin username (when auth enabled)       |
| `THEMIS_ADMIN_PASSWORD`       | —              | Admin password (when auth enabled)       |
| `THEMIS_MFA_REQUIRED_ROLES`   | —              | Roles requiring MFA (e.g. `admin,operator`) |

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

## Troubleshooting

### Admin UI shows "Connecting…"

- Is the ThemisDB container running? `docker ps | grep themisdb`
- Perform a health check: `curl http://<QNAP-IP>:18765/health`
- Check nginx logs: `docker logs themisdb-admin-ui`

### Admin UI shows "Auth required" / login form appears

- Expected behaviour when `THEMIS_AUTH_ENABLED=true` is set.
- Use the credentials from `THEMIS_ADMIN_USER` / `THEMIS_ADMIN_PASSWORD`.
- After 5 failed attempts the rate limit kicks in (429 — wait 1 minute).

### HTTP 403 — "csrf_required"

- The browser is not sending an `X-CSRF-Token` header.
- Ensure JavaScript is enabled and the SPA loaded correctly.
- Clear the browser cache and reload the page.

### HTTP 403 — "cors_forbidden"

- Your browser's origin is not on the CORS whitelist in nginx.
- Edit `docker/admin-ui/nginx.conf` and add your QNAP hostname.
- Rebuild: `docker compose -f docker-compose.qnap.yml up -d --build admin-ui`

### HTTP 429 — "rate_limit_exceeded"

- Too many requests in a short period. Wait 1 minute and retry.
- If the problem persists, adjust `limit_req_zone` in the nginx configuration.

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
- [nginx.ssl.conf (TLS)](../../../docker/admin-ui/nginx.ssl.conf)

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
