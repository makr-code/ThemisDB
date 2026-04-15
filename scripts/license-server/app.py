"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            app.py                                             ║
  Version:         0.0.40                                             ║
  Last Modified:   2026-04-15 18:07:09                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     441                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

from __future__ import annotations

"""
ThemisDB License Validation Server
===================================
Minimal FastAPI service that acts as the authoritative license backend for
ThemisDB C++ server instances.

Endpoints
---------
POST /v1/activate   – Activate a license key (first-time binding to machine)
POST /v1/validate   – Validate an already-activated license
POST /v1/refresh    – Re-check a license (force online re-validation)
POST /v1/revoke     – Revoke a license (admin, requires admin secret header)
GET  /v1/status     – Health check / server status

All license responses include an HMAC-SHA256 signature that the C++ client
(LicenseClient) can verify independently.

Configuration (environment variables)
--------------------------------------
THEMIS_LS_API_KEY        – Required.  Shared secret checked via "Authorization: Bearer <key>"
THEMIS_LS_ADMIN_SECRET   – Required for admin endpoints (X-ThemisDB-Admin-Secret header)
THEMIS_LS_DB_PATH        – Path to the SQLite database file (default: ./license_store.db)
THEMIS_LS_HOST           – Bind host (default: 0.0.0.0)
THEMIS_LS_PORT           – Bind port (default: 8765)

Running
-------
    pip install -r requirements.txt
    uvicorn app:app --host 0.0.0.0 --port 8765

Or via Docker:
    docker build -t themisdb-license-server .
    docker run -e THEMIS_LS_API_KEY=<key> -p 8765:8765 themisdb-license-server
"""

import hashlib
import hmac
import json
import logging
import os
import sqlite3
import time
from contextlib import asynccontextmanager
from datetime import datetime, timezone, timedelta
from typing import Optional

from fastapi import Depends, FastAPI, Header, HTTPException, Request, status
from fastapi.responses import JSONResponse
from pydantic import BaseModel, Field

# ---------------------------------------------------------------------------
# Logging
# ---------------------------------------------------------------------------
logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [%(levelname)s] %(name)s – %(message)s",
)
logger = logging.getLogger("themisdb.license_server")

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------
API_KEY      = os.environ.get("THEMIS_LS_API_KEY", "")
ADMIN_SECRET = os.environ.get("THEMIS_LS_ADMIN_SECRET", "")
DB_PATH      = os.environ.get("THEMIS_LS_DB_PATH", "./license_store.db")

# ---------------------------------------------------------------------------
# Database helpers
# ---------------------------------------------------------------------------

def get_db() -> sqlite3.Connection:
    """Open (and lazily initialise) the SQLite database."""
    conn = sqlite3.connect(DB_PATH, check_same_thread=False)
    conn.row_factory = sqlite3.Row
    _init_db(conn)
    return conn


def _init_db(conn: sqlite3.Connection) -> None:
    conn.executescript("""
        CREATE TABLE IF NOT EXISTS licenses (
            id              INTEGER PRIMARY KEY AUTOINCREMENT,
            license_key     TEXT    NOT NULL UNIQUE,
            edition         TEXT    NOT NULL DEFAULT 'COMMUNITY',
            organization    TEXT,
            email           TEXT,
            issued_date     TEXT,
            expiry_date     TEXT,
            max_nodes       INTEGER DEFAULT -1,
            max_cores       INTEGER DEFAULT -1,
            max_storage_tb  INTEGER DEFAULT -1,
            status          TEXT    NOT NULL DEFAULT 'active',
            created_at      TEXT    NOT NULL,
            updated_at      TEXT    NOT NULL
        );

        CREATE TABLE IF NOT EXISTS activations (
            id                  INTEGER PRIMARY KEY AUTOINCREMENT,
            license_key         TEXT    NOT NULL,
            machine_fingerprint TEXT    NOT NULL,
            activated_at        TEXT    NOT NULL,
            last_seen_at        TEXT    NOT NULL,
            UNIQUE(license_key, machine_fingerprint)
        );

        CREATE TABLE IF NOT EXISTS audit_log (
            id              INTEGER PRIMARY KEY AUTOINCREMENT,
            license_key     TEXT,
            action          TEXT    NOT NULL,
            result          TEXT    NOT NULL,
            ip_address      TEXT,
            fingerprint     TEXT,
            created_at      TEXT    NOT NULL
        );
    """)
    conn.commit()


# ---------------------------------------------------------------------------
# Security helpers
# ---------------------------------------------------------------------------

def _verify_api_key(authorization: str = Header(default="")) -> None:
    """FastAPI dependency: verify Bearer token against THEMIS_LS_API_KEY."""
    if not API_KEY:
        raise HTTPException(status_code=503, detail="Server API key not configured.")
    if not authorization.startswith("Bearer "):
        raise HTTPException(status_code=401, detail="Missing Bearer token.")
    token = authorization[len("Bearer "):]
    if not hmac.compare_digest(API_KEY, token):
        raise HTTPException(status_code=403, detail="Invalid API key.")


def _verify_admin(x_themisdb_admin_secret: str = Header(default="")) -> None:
    """FastAPI dependency: verify X-ThemisDB-Admin-Secret header."""
    if not ADMIN_SECRET:
        raise HTTPException(status_code=503, detail="Admin secret not configured.")
    if not hmac.compare_digest(ADMIN_SECRET, x_themisdb_admin_secret):
        raise HTTPException(status_code=403, detail="Invalid admin secret.")


def _sign_response(body: dict) -> str:
    """
    Compute HMAC-SHA256 over the sorted, JSON-encoded response body using
    THEMIS_LS_API_KEY as the secret.  The C++ LicenseClient can verify this.
    """
    canonical = json.dumps(body, sort_keys=True, ensure_ascii=False, separators=(",", ":"))
    return hmac.new(API_KEY.encode(), canonical.encode(), hashlib.sha256).hexdigest()


def _now_iso() -> str:
    return datetime.now(timezone.utc).isoformat(timespec="seconds")


def _days_until_expiry(expiry_date: Optional[str]) -> int:
    """Return days until expiry; 999999 for perpetual; negative if expired."""
    if not expiry_date or expiry_date == "9999-12-31":
        return 999999
    try:
        exp = datetime.strptime(expiry_date, "%Y-%m-%d").replace(tzinfo=timezone.utc)
        delta = exp - datetime.now(timezone.utc)
        return delta.days
    except ValueError:
        return -999999


# ---------------------------------------------------------------------------
# Pydantic models
# ---------------------------------------------------------------------------

class ActivateRequest(BaseModel):
    license_key: str
    machine_fingerprint: str = ""
    edition: str = ""


class ValidateRequest(BaseModel):
    license_key: str
    machine_fingerprint: str = ""
    edition: str = ""


class RevokeRequest(BaseModel):
    license_key: str
    reason: str = "Revoked via API"


# ---------------------------------------------------------------------------
# FastAPI app
# ---------------------------------------------------------------------------
@asynccontextmanager
async def lifespan(application: FastAPI):  # noqa: ARG001
    if not API_KEY:
        logger.warning("THEMIS_LS_API_KEY is not set – all requests will be rejected.")
    logger.info("ThemisDB License Server started. DB: %s", DB_PATH)
    yield


app = FastAPI(
    title="ThemisDB License Server",
    description="Authoritative license validation service for ThemisDB instances.",
    version="1.0.0",
    docs_url="/docs",
    redoc_url=None,
    lifespan=lifespan,
)


# ---------------------------------------------------------------------------
# Helper: write audit entry
# ---------------------------------------------------------------------------

def _audit(
    conn: sqlite3.Connection,
    license_key: Optional[str],
    action: str,
    result: str,
    ip_address: str = "",
    fingerprint: str = "",
) -> None:
    conn.execute(
        "INSERT INTO audit_log (license_key, action, result, ip_address, fingerprint, created_at) "
        "VALUES (?, ?, ?, ?, ?, ?)",
        (license_key, action, result, ip_address, fingerprint, _now_iso()),
    )
    conn.commit()


# ---------------------------------------------------------------------------
# Endpoints
# ---------------------------------------------------------------------------

@app.get("/v1/status")
def health_status():
    """Health check – no authentication required."""
    return {"status": "ok", "timestamp": _now_iso(), "version": "1.0.0"}


@app.post("/v1/activate", dependencies=[Depends(_verify_api_key)])
def activate_license(body: ActivateRequest, request: Request):
    """
    Activate a license key and bind it to a machine fingerprint.
    Returns the authoritative LicenseData if the key is valid.
    """
    ip = request.client.host if request.client else ""
    conn = get_db()

    try:
        row = conn.execute(
            "SELECT * FROM licenses WHERE license_key = ?", (body.license_key,)
        ).fetchone()

        if not row:
            _audit(conn, body.license_key, "activate", "not_found", ip, body.machine_fingerprint)
            raise HTTPException(status_code=404, detail="License key not found.")

        if row["status"] != "active":
            _audit(conn, body.license_key, "activate", row["status"], ip, body.machine_fingerprint)
            raise HTTPException(status_code=402, detail=f"License is {row['status']}.")

        days = _days_until_expiry(row["expiry_date"])
        if days < 0:
            conn.execute(
                "UPDATE licenses SET status='expired', updated_at=? WHERE license_key=?",
                (_now_iso(), body.license_key),
            )
            conn.commit()
            _audit(conn, body.license_key, "activate", "expired", ip, body.machine_fingerprint)
            raise HTTPException(status_code=402, detail="License has expired.")

        # Register / update activation record
        now = _now_iso()
        conn.execute(
            "INSERT INTO activations (license_key, machine_fingerprint, activated_at, last_seen_at) "
            "VALUES (?, ?, ?, ?) "
            "ON CONFLICT(license_key, machine_fingerprint) DO UPDATE SET last_seen_at=excluded.last_seen_at",
            (body.license_key, body.machine_fingerprint, now, now),
        )
        conn.commit()

        _audit(conn, body.license_key, "activate", "success", ip, body.machine_fingerprint)

        response_body = {
            "success": True,
            "status": "active",
            "license_key": row["license_key"],
            "edition": row["edition"],
            "organization": row["organization"],
            "email": row["email"],
            "issued_date": row["issued_date"],
            "expiry_date": row["expiry_date"],
            "days_remaining": days,
            "max_nodes": row["max_nodes"],
            "max_cores": row["max_cores"],
            "max_storage_tb": row["max_storage_tb"],
            "timestamp": now,
        }
        response_body["signature"] = _sign_response(response_body)
        return response_body

    finally:
        conn.close()


@app.post("/v1/validate", dependencies=[Depends(_verify_api_key)])
def validate_license(body: ValidateRequest, request: Request):
    """
    Validate an already-activated license.
    Returns current status including grace period information.
    """
    ip = request.client.host if request.client else ""
    conn = get_db()

    try:
        row = conn.execute(
            "SELECT * FROM licenses WHERE license_key = ?", (body.license_key,)
        ).fetchone()

        if not row:
            _audit(conn, body.license_key, "validate", "not_found", ip, body.machine_fingerprint)
            response_body = {
                "success": False,
                "status": "invalid",
                "error": "License key not found.",
                "timestamp": _now_iso(),
            }
            response_body["signature"] = _sign_response(response_body)
            return JSONResponse(status_code=404, content=response_body)

        days = _days_until_expiry(row["expiry_date"])
        if row["status"] != "active" or days < 0:
            status_str = row["status"] if row["status"] != "active" else "expired"
            _audit(conn, body.license_key, "validate", status_str, ip, body.machine_fingerprint)
            response_body = {
                "success": False,
                "status": status_str,
                "license_key": body.license_key,
                "error": f"License is {status_str}.",
                "timestamp": _now_iso(),
            }
            response_body["signature"] = _sign_response(response_body)
            return JSONResponse(status_code=402, content=response_body)

        # Update last-seen timestamp for the activation record
        now = _now_iso()
        conn.execute(
            "UPDATE activations SET last_seen_at=? WHERE license_key=? AND machine_fingerprint=?",
            (now, body.license_key, body.machine_fingerprint),
        )
        conn.commit()

        _audit(conn, body.license_key, "validate", "success", ip, body.machine_fingerprint)

        response_body = {
            "success": True,
            "status": "active",
            "license_key": row["license_key"],
            "edition": row["edition"],
            "organization": row["organization"],
            "days_remaining": days,
            "max_nodes": row["max_nodes"],
            "max_cores": row["max_cores"],
            "max_storage_tb": row["max_storage_tb"],
            "timestamp": now,
        }
        response_body["signature"] = _sign_response(response_body)
        return response_body

    finally:
        conn.close()


@app.post("/v1/refresh", dependencies=[Depends(_verify_api_key)])
def refresh_license(body: ValidateRequest, request: Request):
    """Force re-validation – identical to /validate but clears any local cache hint."""
    return validate_license(body, request)


@app.post(
    "/v1/revoke",
    dependencies=[Depends(_verify_api_key), Depends(_verify_admin)],
)
def revoke_license(body: RevokeRequest, request: Request):
    """
    Revoke a license (admin-only endpoint).
    Requires both the API key and the admin secret header.
    """
    ip = request.client.host if request.client else ""
    conn = get_db()

    try:
        row = conn.execute(
            "SELECT id FROM licenses WHERE license_key = ?", (body.license_key,)
        ).fetchone()

        if not row:
            _audit(conn, body.license_key, "revoke", "not_found", ip)
            raise HTTPException(status_code=404, detail="License key not found.")

        conn.execute(
            "UPDATE licenses SET status='suspended', updated_at=? WHERE license_key=?",
            (_now_iso(), body.license_key),
        )
        conn.commit()

        _audit(conn, body.license_key, "revoke", "success", ip)

        return {
            "success": True,
            "license_key": body.license_key,
            "status": "suspended",
            "reason": body.reason,
            "timestamp": _now_iso(),
        }

    finally:
        conn.close()
