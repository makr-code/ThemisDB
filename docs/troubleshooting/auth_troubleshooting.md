# Auth Troubleshooting Guide

The `auth` module handles all authentication mechanisms in ThemisDB, including JWT/JWKS validation, OAuth2 (PKCE and device flow), OIDC, SAML, Kerberos/GSSAPI, TOTP MFA, API keys, and token rotation with replay protection.

## Quick Diagnostics

| Symptom | Likely Cause | Quick Fix |
|---------|-------------|-----------|
| `401 Unauthorized` on all requests | JWT secret misconfigured or JWKS unreachable | Check `auth.jwt.jwks_url` or `auth.jwt.secret` |
| `Token expired` immediately after issue | Server clock skew vs issuer | Sync NTP; set `auth.jwt.clock_skew_seconds: 30` |
| Kerberos auth fails with `KRB5KDC_ERR_S_PRINCIPAL_UNKNOWN` | Wrong service principal name | Verify `auth.kerberos.service_principal` |
| MFA TOTP always invalid | Shared secret mismatch or time drift | Re-provision TOTP secret; check server clock |
| OAuth2 PKCE flow returns `invalid_grant` | Code verifier mismatch or code expired | Reduce `auth.oauth2.auth_code_lifetime_seconds` |
| API key returns `403 Forbidden` | Key not assigned required scope | Add missing scope via admin API |
| Rate limiter blocks legitimate users | Threshold too low | Increase `auth.rate_limiter.max_requests_per_minute` |
| GSSAPI handshake fails | Missing keytab or wrong realm | Verify keytab file and `auth.kerberos.realm` |
| OIDC discovery endpoint unreachable | Wrong issuer URL or firewall | Check `auth.oidc.issuer_url` connectivity |
| Password policy rejects valid passwords | Policy too strict | Review `auth.password_policy` settings |

## Common Issues

### Issue 1: JWKS Endpoint Unreachable

**Description:** ThemisDB cannot fetch the JSON Web Key Set to validate JWT signatures.

**Symptoms:**
- Log: `JwksValidator: failed to fetch JWKS from https://idp.example.com/.well-known/jwks.json: connection refused`
- All JWT-authenticated requests return `401`

**Cause:** Identity provider is down, firewall blocks outbound HTTPS, or `jwks_url` is wrong.

**Solution:**
```bash
# Test JWKS endpoint reachability from the ThemisDB host
curl -v https://idp.example.com/.well-known/jwks.json

# Check firewall
iptables -L OUTPUT -n | grep 443
```
```yaml
auth:
  jwt:
    jwks_url: https://idp.example.com/.well-known/jwks.json
    jwks_cache_ttl_seconds: 300      # cache keys to survive brief IdP outages
    jwks_refresh_interval_seconds: 60
    fallback_on_jwks_error: false    # set true only in dev
```

---

### Issue 2: JWT Clock Skew Causes Immediate Expiry

**Description:** Tokens issued by the IdP are immediately rejected as expired.

**Symptoms:**
- Log: `JwtValidator: token not yet valid or already expired (nbf/exp check)`
- Users are logged out seconds after login

**Cause:** Server clock is ahead of the token issuer's clock by more than the `exp` grace window.

**Solution:**
```bash
# Check server time vs NTP
timedatectl status
chronyc tracking

# Sync immediately
chronyc makestep
```
```yaml
auth:
  jwt:
    clock_skew_seconds: 30    # allow up to 30s clock difference
```

---

### Issue 3: Kerberos `KRB5KDC_ERR_S_PRINCIPAL_UNKNOWN`

**Description:** Kerberos authentication fails because the KDC does not recognise the ThemisDB service principal.

**Symptoms:**
- Log: `KerberosSecurity: kinit failed: KRB5KDC_ERR_S_PRINCIPAL_UNKNOWN`
- Clients using Kerberos SSO cannot connect

**Cause:** The SPN `themisdb/hostname@REALM` has not been created in Active Directory / MIT KDC, or the keytab is stale.

**Solution:**
```bash
# Create the SPN (Active Directory, run as Domain Admin)
setspn -A themisdb/db.corp.example.com@CORP.EXAMPLE.COM svc-themisdb

# Export keytab
ktpass -princ themisdb/db.corp.example.com@CORP.EXAMPLE.COM \
       -mapuser svc-themisdb -crypto AES256-SHA1 \
       -ptype KRB5_NT_PRINCIPAL -pass * \
       -out /etc/themisdb/themisdb.keytab

# Verify keytab
klist -kt /etc/themisdb/themisdb.keytab
```
```yaml
auth:
  kerberos:
    enabled: true
    service_principal: themisdb/db.corp.example.com@CORP.EXAMPLE.COM
    keytab_file: /etc/themisdb/themisdb.keytab
    realm: CORP.EXAMPLE.COM
    kdc: kdc.corp.example.com
```

---

### Issue 4: TOTP MFA Always Returns Invalid Code

**Description:** Time-based one-time passwords are always rejected even when they appear correct.

**Symptoms:**
- Log: `MfaAuthenticator: TOTP validation failed for user=alice`
- Users cannot complete MFA step despite entering correct code

**Cause:** Server clock is more than 30 seconds out of sync, or the TOTP shared secret was re-provisioned without the user re-scanning the QR code.

**Solution:**
```bash
# Check time drift
timedatectl show --property=NTPSynchronized,TimeUSec
chronyc makestep

# Re-provision TOTP for a user via admin API
curl -X POST http://localhost:9090/admin/auth/mfa/reprovision \
     -H "Authorization: Bearer $ADMIN_TOKEN" \
     -d '{"username": "alice"}'
```
```yaml
auth:
  mfa:
    totp:
      window: 1          # allow 1 step before and after (default)
      issuer: ThemisDB
      digits: 6
      algorithm: SHA1
```

---

### Issue 5: OAuth2 PKCE Flow Returns `invalid_grant`

**Description:** After the user authorises the application, the token exchange fails.

**Symptoms:**
- OAuth2 callback returns `{"error": "invalid_grant"}`
- Log: `OAuthPkceFlow: code_verifier does not match code_challenge`

**Cause:** The client sent a different `code_verifier` than was hashed to produce the `code_challenge`, or the authorization code has expired.

**Solution:**
```yaml
auth:
  oauth2:
    pkce:
      enabled: true
      code_challenge_method: S256     # must be S256, not plain
    auth_code_lifetime_seconds: 60    # increase if network is slow
```
```bash
# Debug PKCE flow
themisdb-admin auth oauth2-debug --client-id myapp --trace
```

---

### Issue 6: API Key Missing Required Scope

**Description:** A valid API key is rejected with `403` when accessing a specific endpoint.

**Symptoms:**
- Log: `ApiKeyAuthenticator: key=ak_xxx lacks required scope 'query:write'`
- HTTP 403 response body: `{"error": "insufficient_scope"}`

**Cause:** API key was created without the necessary scope.

**Solution:**
```bash
# List scopes for a key
themisdb-admin auth api-key info --key ak_xxx

# Add scope to existing key
themisdb-admin auth api-key update --key ak_xxx --add-scope query:write

# Or create a new key with full scopes
themisdb-admin auth api-key create \
  --name "ci-pipeline" \
  --scopes "query:read,query:write,admin:metrics" \
  --expiry 2027-01-01
```

---

### Issue 7: Rate Limiter Blocks Legitimate Traffic

**Description:** The `AuthRateLimiter` throttles valid login attempts during business hours.

**Symptoms:**
- Log: `AuthRateLimiter: IP 10.0.0.5 exceeded limit of 60 req/min`
- Users report intermittent `429 Too Many Requests`

**Cause:** Default rate limit is too aggressive for high-traffic deployments; corporate NAT makes many users share one IP.

**Solution:**
```yaml
auth:
  rate_limiter:
    enabled: true
    max_requests_per_minute: 300      # increase from default 60
    burst: 50
    strategy: sliding_window
    whitelist_cidrs:
      - 10.0.0.0/8                    # internal network
      - 192.168.0.0/16
    per_user_limit: true              # rate-limit per user, not per IP
```

---

### Issue 8: OIDC Discovery Document Missing Claims

**Description:** OIDC login succeeds but the user's email/groups are not populated.

**Symptoms:**
- Users logged in via OIDC have no `email` attribute
- Log: `OidcProvider: claim 'email' not found in id_token`

**Cause:** The IdP's ID token does not include the `email` claim by default; additional scopes must be requested.

**Solution:**
```yaml
auth:
  oidc:
    issuer_url: https://accounts.google.com
    client_id: ${OIDC_CLIENT_ID}
    client_secret: ${OIDC_CLIENT_SECRET}
    scopes:
      - openid
      - email           # required for email claim
      - profile
      - groups          # IdP-specific for group membership
    claim_mappings:
      username: preferred_username
      email: email
      groups: groups
```

---

### Issue 9: JWT Key Rotation Causes Brief 401 Errors

**Description:** After rotating the JWT signing key, in-flight tokens are rejected for a short window.

**Symptoms:**
- Log: `JwtKeyRotationManager: old key expired; new key active`
- Some clients get `401` for ~60 seconds after rotation

**Cause:** JWKS cache on ThemisDB still references the old key; new tokens signed with new key fail until cache refreshes.

**Solution:**
```yaml
auth:
  jwt:
    jwks_cache_ttl_seconds: 60        # reduce during rotation windows
    key_rotation:
      overlap_seconds: 300            # keep old key valid for 5 min after rotation
      auto_rotate: true
      rotation_interval_days: 30
```
```bash
# Manually trigger key rotation with overlap
themisdb-admin auth jwt rotate-key --overlap-seconds 300
```

---

### Issue 10: Password Policy Rejects All New Passwords

**Description:** User self-service password change always fails validation.

**Symptoms:**
- API returns `{"error": "password_policy_violation", "detail": "..."}`
- Log: `PasswordPolicy: password fails entropy check`

**Cause:** Entropy requirement is too high, or the character class requirements conflict.

**Solution:**
```yaml
auth:
  password_policy:
    min_length: 12
    require_uppercase: true
    require_lowercase: true
    require_digit: true
    require_special: true
    min_entropy_bits: 40      # lower from 60 if users struggle
    max_age_days: 90
    history_count: 5          # prevent reuse of last 5 passwords
    breach_check: true        # check against HaveIBeenPwned
```

## Diagnostic Commands

```bash
# Test JWT validation with a sample token
themisdb-admin auth validate-token --token <jwt>

# Check JWKS cache state
themisdb-admin auth jwks-status

# List all active API keys
themisdb-admin auth api-key list

# Show rate limiter counters
themisdb-admin auth rate-limiter stats --ip 10.0.0.5

# Tail auth log lines
journalctl -u themisdb -f | grep -E "auth|jwt|kerberos|oauth|oidc|mfa"

# Live auth metrics
curl -s http://localhost:9100/metrics | grep themisdb_auth

# Test Kerberos ticket
kinit -k -t /etc/themisdb/themisdb.keytab \
      themisdb/db.corp.example.com@CORP.EXAMPLE.COM
klist
```

## Configuration Reference

```yaml
auth:
  jwt:
    jwks_url: https://idp.example.com/.well-known/jwks.json
    secret: ""                     # used only when jwks_url is empty
    algorithm: RS256
    clock_skew_seconds: 30
    jwks_cache_ttl_seconds: 300
  oauth2:
    pkce:
      enabled: true
      code_challenge_method: S256
    auth_code_lifetime_seconds: 60
  oidc:
    issuer_url: https://idp.example.com
    client_id: ${OIDC_CLIENT_ID}
    client_secret: ${OIDC_CLIENT_SECRET}
    scopes: [openid, email, profile]
  kerberos:
    enabled: false
    keytab_file: /etc/themisdb/themisdb.keytab
    service_principal: ""
    realm: ""
  mfa:
    totp:
      enabled: true
      window: 1
  api_keys:
    enabled: true
    default_expiry_days: 365
  rate_limiter:
    enabled: true
    max_requests_per_minute: 120
    burst: 30
  password_policy:
    min_length: 12
    min_entropy_bits: 40
```

**Common misconfigurations:**

| Key | Wrong | Correct |
|-----|-------|---------|
| `jwt.algorithm` | `HS256` with public IdP | `RS256` |
| `jwks_cache_ttl_seconds` | `0` | `300` |
| `kerberos.enabled` | `true` without keytab | Set keytab first |
| `rate_limiter.per_user_limit` | `false` behind NAT | `true` |

## Known Limitations

- SAML SP-initiated SSO requires an external SAML library; ensure `libsaml2` is installed.
- GSSAPI requires MIT Kerberos or Heimdal libraries linked at build time (`THEMIS_ENABLE_GSSAPI=ON`).
- API key secrets are shown only once at creation; ThemisDB stores only the PBKDF2 hash.
- The JWKS cache does not support mutual TLS between ThemisDB and the IdP's JWKS endpoint.
- Token replay protection requires a Redis or in-process nonce store; configure `auth.replay_protection.backend`.

## Related Documentation

- [Auth Module ROADMAP](../../src/auth/ROADMAP.md)
- [Auth Implementation Summary](../AUTH_IMPLEMENTATION_SUMMARY.md)
- [Kerberos Implementation Summary](../ARCHIVED/implementation-summaries/KERBEROS_IMPLEMENTATION_SUMMARY.md)
- [API Authentication & Authorization](../security/api_authentication_authorization.md)
- [Security Executive Summary](../de/security/SECURITY_EXECUTIVE_SUMMARY.md)
