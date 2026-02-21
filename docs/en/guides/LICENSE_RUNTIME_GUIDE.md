# ThemisDB Runtime License System – Admin & Customer Guide

**Applies to**: v1.5.0+  
**Audience**: Administrators, DevOps, End Customers

---

## Overview

Starting with v1.5.0 ThemisDB adds a **runtime license validation** layer on top of the existing
compile-time feature gates.  The system has three components:

| Component | Location | Purpose |
|-----------|----------|---------|
| **C++ License Client** | `ThemisDB` server binary | Calls the license server at startup and periodically |
| **WordPress REST API** | `themisdb-order-request` plugin | Issues, validates, and revokes licenses |
| **License Validation Server** | `scripts/license-server/` | Stand-alone FastAPI service (optional) |

The compile-time embedding guide (`LICENSE_EMBEDDING_GUIDE.md`) still applies for **offline /
air-gapped deployments**.  The runtime system is the recommended path for internet-connected
production environments.

---

## Quick-Start for Administrators

### 1 — Configure the WordPress Plugin

1. Log in to WordPress Admin → **ThemisDB Orders → Settings**.
2. Scroll to the **License API Settings** section:

   | Setting | Description |
   |---------|-------------|
   | **License API Key** | Shared secret sent by ThemisDB servers as `Authorization: Bearer <key>`. Generate with `openssl rand -hex 32`. |
   | **Admin Secret** | Additional header (`X-ThemisDB-Admin-Secret`) required for renew/revoke endpoints. |
   | **Renewal Reminder (days)** | How many days before expiry an automated reminder e-mail is sent (default: 30). |

3. Click **Save Changes**.

### 2 — Point the ThemisDB Server at the License API

Set these values in your ThemisDB configuration (YAML or environment variables):

```yaml
# themis.yaml
license:
  server_url: "https://your-wordpress-site.com/wp-json/themisdb/v1"
  api_key: "<same value as License API Key above>"
  allow_offline: true      # fall back to embedded license if server unreachable
  grace_period_days: 7     # extra days before refusing to start after offline
```

Or as environment variables:

```bash
export THEMIS_LICENSE_SERVER_URL="https://your-wordpress-site.com/wp-json/themisdb/v1"
export THEMIS_LICENSE_API_KEY="<api-key>"
```

### 3 — (Optional) Deploy the Stand-Alone License Server

If you prefer a lightweight validation service instead of calling WordPress directly:

```bash
cd scripts/license-server
pip install -r requirements.txt

export THEMIS_LS_API_KEY="<strong-random-key>"
export THEMIS_LS_ADMIN_SECRET="<strong-admin-secret>"
export THEMIS_LS_DB_PATH="/var/lib/themisdb-ls/licenses.db"

uvicorn app:app --host 0.0.0.0 --port 8765
```

Or with Docker:

```bash
docker build -t themisdb-license-server scripts/license-server/

docker run -d \
  -e THEMIS_LS_API_KEY="<key>" \
  -e THEMIS_LS_ADMIN_SECRET="<secret>" \
  -e THEMIS_LS_DB_PATH="/data/licenses.db" \
  -v /var/lib/themisdb-ls:/data \
  -p 8765:8765 \
  themisdb-license-server
```

---

## REST API Reference (WordPress Plugin)

Base URL: `https://your-site.com/wp-json/themisdb/v1`

All endpoints require:
```
Authorization: Bearer <themisdb_license_api_key>
```

### POST `/license/validate`

Called by ThemisDB servers to check whether a license is active.

**Request body**
```json
{
  "license_key": "THEMIS-ENT-AABBCCDD-11223344",
  "machine_fingerprint": "<sha256-of-mac-address>",
  "edition": "ENTERPRISE"
}
```

**Success response (HTTP 200)**
```json
{
  "valid": true,
  "status": "active",
  "license_key": "THEMIS-ENT-AABBCCDD-11223344",
  "tier": "enterprise",
  "organization": "Example Corp",
  "limits": { "max_nodes": 100, "max_cores": -1, "max_storage_tb": -1 },
  "start_date": "2026-01-01T00:00:00+00:00",
  "end_date": "2027-01-01T00:00:00+00:00",
  "days_remaining": 344,
  "timestamp": "2026-02-21T09:00:00+00:00",
  "signature": "<hmac-sha256>"
}
```

**Failure response (HTTP 402)**
```json
{
  "valid": false,
  "status": "expired",
  "error": "License expired on 2025-12-31",
  "timestamp": "2026-02-21T09:00:00+00:00",
  "signature": "<hmac-sha256>"
}
```

Possible `status` values: `active`, `expired`, `suspended`, `cancelled`, `pending_payment`, `invalid`, `not_found`.

### GET `/license/download/{license_key}`

Returns the license JSON file for automated provisioning.

```bash
curl -H "Authorization: Bearer <key>" \
  "https://your-site.com/wp-json/themisdb/v1/license/download/THEMIS-ENT-AABBCCDD-11223344"
```

The response body is the license JSON file with a `Content-Disposition: attachment` header.

### POST `/license/renew` *(admin only)*

Extends the expiry date of a license.

Additional required header: `X-ThemisDB-Admin-Secret: <admin-secret>`

```json
{
  "license_key": "THEMIS-ENT-AABBCCDD-11223344",
  "extend_days": 365
}
```

### POST `/license/revoke` *(admin only)*

Suspends a license immediately.

```json
{
  "license_key": "THEMIS-ENT-AABBCCDD-11223344",
  "reason": "Subscription cancelled"
}
```

---

## Response Signature Verification (C++ Server)

Every API response includes a `signature` field: an HMAC-SHA256 over the sorted, JSON-encoded
response body, keyed with `THEMIS_LS_API_KEY` (or `themisdb_license_api_key`).

The C++ `LicenseClient` verifies this signature automatically.  For manual verification:

```python
import hmac, hashlib, json

def verify(response_body: dict, api_key: str) -> bool:
    sig = response_body.pop("signature", "")
    canonical = json.dumps(response_body, sort_keys=True, separators=(",", ":"))
    expected  = hmac.new(api_key.encode(), canonical.encode(), hashlib.sha256).hexdigest()
    return hmac.compare_digest(expected, sig)
```

---

## Customer Self-Service Portal

Embed the portal on any WordPress page:

```
[themisdb_license_portal]
```

Features available to logged-in customers:

- **License overview** – list of all licenses with status badges and expiry dates
- **Download** – download the `themis-license.json` file for installation
- **Renew** – link to the renewal contact form (URL configurable via the `themisdb_renewal_url` filter)
- **Start trial** – one-click 30-day Community Edition trial (one per customer)

### Install a Downloaded License File

Place the downloaded `themis-license.json` at one of these locations
(ThemisDB checks them in order):

1. Path specified via `--license-file /path/to/themis-license.json` CLI flag
2. `$THEMIS_HOME/.themis-license`
3. `~/.themisdb/license.json`
4. `/etc/themisdb/license.json`

After placing the file, restart the ThemisDB server.  The startup log will confirm:

```
[INFO] ThemisDB License: ENTERPRISE – Example Corp (expires 2027-01-01, 344 days)
```

---

## Error Scenarios & Troubleshooting

### License Expired

**Server log:**
```
[ERROR] WARNING: License has expired!
[ERROR] Please contact admin@example.com to renew your license.
```

**Resolution:**
1. Log in to your ThemisDB customer portal.
2. Click **Renew** next to the expired license.
3. After payment is processed, download the updated `themis-license.json`.
4. Replace the old license file and restart the server.

For ENTERPRISE Release and HYPERSCALER builds the server will **refuse to start** with an
expired license.  Community and debug builds log a warning but continue.

### Invalid or Tampered License

**Server log:**
```
[ERROR] License signature verification FAILED!
[WARN]  License signature is invalid. This may indicate a tampered license.
```

**Resolution:**
- Re-download the license file from the customer portal.
- Ensure the file has not been modified after download.
- Contact `licensing@themisdb.com` if the problem persists.

### Server Unreachable (Grace Period)

If the online license server is temporarily unavailable, the C++ client uses the
**embedded / cached license** for `grace_period_days` (default 7).

**Server log:**
```
[WARN] License server unreachable. Using cached license (grace period: 5 days remaining).
```

After the grace period expires, ENTERPRISE/HYPERSCALER builds will refuse to start.

### License Not Found

```json
{ "valid": false, "status": "not_found", "error": "License key not found." }
```

**Resolution:** Verify the `THEMIS_LICENSE_KEY` build variable or the `license_key` field in your
license file matches a key issued by the ThemisDB license portal.

### API Key Not Configured (WordPress)

```json
{ "code": "rest_forbidden", "message": "License API key not configured." }
```

**Resolution:** Go to WordPress Admin → ThemisDB Orders → Settings → License API Settings and
enter a License API Key.

---

## Renewal Reminder Emails

The plugin sends automated renewal reminder e-mails via WP-Cron once per day for each active
license whose expiry date is within the configured reminder window.

- Configure the window in **Settings → License API Settings → Renewal Reminder (days)**.
- E-mails are sent from the address configured in **Settings → E-Mail Settings**.
- Each license receives at most one reminder e-mail per day.
- The reminder stops once the license is renewed or expired.

---

## Audit Log

Every license validation, download, renewal and revocation request is logged.

**View in WordPress Admin:** ThemisDB Orders → License Audit Log

The log table (`{prefix}themisdb_license_audit_log`) captures:

| Column | Description |
|--------|-------------|
| `created_at` | UTC timestamp of the request |
| `license_key` | License key involved |
| `action` | `validate`, `download`, `renew`, `revoke`, `auth_failed` |
| `result` | `success`, `expired`, `not_found`, `invalid`, `db_error`, etc. |
| `ip_address` | Client IP address |
| `user_agent` | HTTP User-Agent header |

---

## Security Notes

1. **API Key Rotation** – rotate `themisdb_license_api_key` and `themisdb_license_admin_secret`
   regularly.  Update `THEMIS_LICENSE_API_KEY` on all ThemisDB instances after rotation.
2. **HTTPS Only** – always serve the WordPress REST API over TLS.  The ThemisDB C++ client
   enforces `CURLOPT_SSL_VERIFYPEER=1`.
3. **Response Signature** – the C++ client verifies the HMAC-SHA256 response signature so a
   man-in-the-middle cannot inject a fake "valid" response.
4. **Rate Limiting** – consider adding a WordPress rate limiter (e.g. Wordfence) to the
   `/wp-json/themisdb/v1/license/*` endpoints to prevent brute-force key probing.

---

## Support

| Channel | Contact |
|---------|---------|
| Licensing issues | licensing@themisdb.com |
| Enterprise support | enterprise@themisdb.com |
| Documentation | https://docs.themisdb.org |
| GitHub | https://github.com/makr-code/ThemisDB |
