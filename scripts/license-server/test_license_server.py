"""
Tests for the ThemisDB License Server (app.py).

Run with:
    pip install pytest httpx fastapi
    pytest test_license_server.py -v
"""

import os
import tempfile
import sqlite3
import pytest
from fastapi.testclient import TestClient

# Use a temporary file so all connections share the same database
_db_file = tempfile.NamedTemporaryFile(suffix=".db", delete=False)
_db_file.close()

# Configure test secrets before importing app
os.environ["THEMIS_LS_API_KEY"]      = "test-api-key-12345"
os.environ["THEMIS_LS_ADMIN_SECRET"] = "test-admin-secret"
os.environ["THEMIS_LS_DB_PATH"]      = _db_file.name

# Re-import after env is set so constants are picked up
import importlib
import app as server_app
importlib.reload(server_app)

client = TestClient(server_app.app)

HEADERS_USER  = {"Authorization": "Bearer test-api-key-12345"}
HEADERS_ADMIN = {
    **HEADERS_USER,
    "X-ThemisDB-Admin-Secret": "test-admin-secret",
}


# ---------------------------------------------------------------------------
# Test teardown – remove temp DB
# ---------------------------------------------------------------------------

def pytest_sessionfinish(session, exitstatus):
    try:
        os.unlink(_db_file.name)
    except OSError:
        pass


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def seed_license(
    key: str = "THEMIS-ENT-AABBCCDD-11223344",
    edition: str = "ENTERPRISE",
    expiry: str = "2099-12-31",
    status: str = "active",
) -> None:
    """Insert a test license directly into the shared test DB."""
    conn = sqlite3.connect(_db_file.name)
    # Ensure tables exist
    server_app._init_db(conn)
    conn.execute(
        "INSERT OR REPLACE INTO licenses "
        "(license_key, edition, organization, email, issued_date, expiry_date, "
        "max_nodes, max_cores, max_storage_tb, status, created_at, updated_at) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
        (key, edition, "Test Org", "admin@test.com", "2025-01-01", expiry,
         100, -1, -1, status, "2025-01-01T00:00:00+00:00", "2025-01-01T00:00:00+00:00"),
    )
    conn.commit()
    conn.close()


# ---------------------------------------------------------------------------
# Health check
# ---------------------------------------------------------------------------

def test_status_endpoint_no_auth():
    resp = client.get("/v1/status")
    assert resp.status_code == 200
    data = resp.json()
    assert data["status"] == "ok"


# ---------------------------------------------------------------------------
# Authentication
# ---------------------------------------------------------------------------

def test_activate_requires_auth():
    resp = client.post("/v1/activate", json={"license_key": "THEMIS-COM-XXXXXXXX-YYYYYYYY"})
    assert resp.status_code in (401, 403)


def test_validate_requires_auth():
    resp = client.post("/v1/validate", json={"license_key": "THEMIS-COM-XXXXXXXX-YYYYYYYY"})
    assert resp.status_code in (401, 403)


def test_revoke_requires_admin_secret():
    # API key only – should fail
    resp = client.post(
        "/v1/revoke",
        json={"license_key": "THEMIS-ENT-AABBCCDD-11223344"},
        headers=HEADERS_USER,  # missing admin secret
    )
    assert resp.status_code in (401, 403)


# ---------------------------------------------------------------------------
# Activate
# ---------------------------------------------------------------------------

def test_activate_not_found():
    resp = client.post(
        "/v1/activate",
        json={"license_key": "THEMIS-ENT-NOTFOUND-00000000", "machine_fingerprint": "abc123"},
        headers=HEADERS_USER,
    )
    assert resp.status_code == 404


def test_activate_valid_license():
    seed_license()
    resp = client.post(
        "/v1/activate",
        json={"license_key": "THEMIS-ENT-AABBCCDD-11223344", "machine_fingerprint": "fp-001"},
        headers=HEADERS_USER,
    )
    assert resp.status_code == 200
    data = resp.json()
    assert data["success"] is True
    assert data["status"] == "active"
    assert data["edition"] == "ENTERPRISE"
    assert "signature" in data


def test_activate_expired_license():
    seed_license(
        key="THEMIS-ENT-EXPIRED0-00000000",
        expiry="2000-01-01",
    )
    resp = client.post(
        "/v1/activate",
        json={"license_key": "THEMIS-ENT-EXPIRED0-00000000", "machine_fingerprint": "fp-002"},
        headers=HEADERS_USER,
    )
    assert resp.status_code == 402


def test_activate_suspended_license():
    seed_license(
        key="THEMIS-ENT-SUSPEND0-00000000",
        status="suspended",
    )
    resp = client.post(
        "/v1/activate",
        json={"license_key": "THEMIS-ENT-SUSPEND0-00000000", "machine_fingerprint": "fp-003"},
        headers=HEADERS_USER,
    )
    assert resp.status_code == 402


# ---------------------------------------------------------------------------
# Validate
# ---------------------------------------------------------------------------

def test_validate_valid_license():
    seed_license(key="THEMIS-ENT-VALID001-11111111")
    resp = client.post(
        "/v1/validate",
        json={"license_key": "THEMIS-ENT-VALID001-11111111", "machine_fingerprint": "fp-004"},
        headers=HEADERS_USER,
    )
    assert resp.status_code == 200
    data = resp.json()
    assert data["success"] is True
    assert "signature" in data


def test_validate_not_found():
    resp = client.post(
        "/v1/validate",
        json={"license_key": "THEMIS-ENT-MISSING0-00000000"},
        headers=HEADERS_USER,
    )
    assert resp.status_code == 404


# ---------------------------------------------------------------------------
# Revoke
# ---------------------------------------------------------------------------

def test_revoke_license():
    seed_license(key="THEMIS-ENT-REVOKE01-00000000")
    resp = client.post(
        "/v1/revoke",
        json={"license_key": "THEMIS-ENT-REVOKE01-00000000", "reason": "Test revoke"},
        headers=HEADERS_ADMIN,
    )
    assert resp.status_code == 200
    data = resp.json()
    assert data["success"] is True
    assert data["status"] == "suspended"


def test_revoke_then_validate_fails():
    seed_license(key="THEMIS-ENT-REVOKE02-00000000")
    # Revoke
    client.post(
        "/v1/revoke",
        json={"license_key": "THEMIS-ENT-REVOKE02-00000000"},
        headers=HEADERS_ADMIN,
    )
    # Validate should now fail
    resp = client.post(
        "/v1/validate",
        json={"license_key": "THEMIS-ENT-REVOKE02-00000000"},
        headers=HEADERS_USER,
    )
    assert resp.status_code == 402


# ---------------------------------------------------------------------------
# Refresh (alias for validate)
# ---------------------------------------------------------------------------

def test_refresh_valid_license():
    seed_license(key="THEMIS-ENT-REFRESH1-00000000")
    resp = client.post(
        "/v1/refresh",
        json={"license_key": "THEMIS-ENT-REFRESH1-00000000", "machine_fingerprint": "fp-005"},
        headers=HEADERS_USER,
    )
    assert resp.status_code == 200
    assert resp.json()["success"] is True
