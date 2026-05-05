# TLS 1.3 with Restricted Cipher Suites for Transport Security

**Metadaten:**
- Source: NIST SP 800-52 Rev 2 — Guidelines for the Selection, Configuration, and Use of TLS Implementations; Mozilla TLS Configuration Generator ("Modern" profile)
- URL: https://csrc.nist.gov/publications/detail/sp/800-52/rev-2/final | https://ssl-config.mozilla.org/
- Tags: security, networking
- ThemisDB-Versionen: v1.0.0+
- Status: [x] Identified | [x] Partially Adopted | [x] Fully Adopted

## 📋 Summary

TLS configuration mistakes are one of the most common sources of transport-layer vulnerabilities. NIST SP 800-52 Rev 2 provides authoritative guidance on which TLS versions, cipher suites, and key exchange algorithms are acceptable for federal systems; Mozilla's "Modern" TLS profile translates this into a concrete server configuration targeting current browsers and modern API clients. Both sources converge on the same conclusion: require TLS 1.3 as minimum, disable TLS 1.0/1.1/1.2 where the client population permits, and restrict cipher suites to AEAD-only algorithms with Perfect Forward Secrecy.

ThemisDB's server TLS layer has applied this profile since v1.0.0 using OpenSSL 3.x or later, configured via the `ssl_context` setup in `src/server/`.

## 🎯 Core Principles

- **TLS 1.3 minimum**: `SSL_CTX_set_min_proto_version(ctx, TLS1_3_VERSION)`. TLS 1.2 may be permitted only for explicitly identified legacy client integrations, with a documented exception.
- **AEAD-only cipher suites**: TLS 1.3 mandates AEAD; for any TLS 1.2 fallback, only `TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384` and `TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384` are permitted.
- **Perfect Forward Secrecy (PFS)**: All cipher suites must use ephemeral key exchange (ECDHE); static RSA key exchange is explicitly disabled.
- **Certificate requirements**: Server certificates use RSA-2048 (minimum) or ECDSA P-256/P-384; SHA-256 or stronger signature hash; validity ≤ 398 days.
- **HSTS enforcement**: HTTP responses include `Strict-Transport-Security: max-age=63072000; includeSubDomains; preload`.

## 🔗 Adoption in ThemisDB

### Affected Modules

- `src/server/` TLS layer — `ssl_context` configuration applied at server startup for HTTP, WebSocket, and MQTT-over-TLS listeners.
- `src/server/http_server.cpp` — `boost::asio::ssl::context` configured with `set_options(SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1 | SSL_OP_NO_TLSv1_2 | SSL_OP_CIPHER_SERVER_PREFERENCE)` and TLS 1.3 cipher string.
- `src/server/mqtt_client_service.cpp` — Outbound MQTT connections use the same context settings.
- `certs/` — Certificate management scripts; automated renewal via ACME/Let's Encrypt with 90-day certificates.

### What Was Adopted?

- `SSL_CTX_set_min_proto_version(ctx, TLS1_3_VERSION)` enforces TLS 1.3 minimum.
- TLS 1.3 cipher suite string: `TLS_AES_256_GCM_SHA384:TLS_CHACHA20_POLY1305_SHA256:TLS_AES_128_GCM_SHA256` (Mozilla Modern order).
- `SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1 | SSL_OP_NO_TLSv1_2)` for defence-in-depth.
- `SSL_CTX_set_options(ctx, SSL_OP_CIPHER_SERVER_PREFERENCE)` ensures server's cipher order is used.
- Session tickets disabled (`SSL_OP_NO_TICKET`) to prevent forward-secrecy weakening.
- Certificate is loaded from PEM file whose path is configured via `THEMISDB_TLS_CERT_FILE` environment variable; private key from `THEMISDB_TLS_KEY_FILE` (never embedded in code).
- HSTS header injected by `src/server/http_server.cpp` on every HTTPS response.

### Deviations & Rationale

- **TLS 1.2 retained for MQTT legacy integrations**: Some industrial IoT devices that integrate with the MQTT listener do not support TLS 1.3. An opt-in `allow_tls12 = true` configuration flag re-enables TLS 1.2 for the MQTT port only, with restricted cipher suite (ECDHE+AESGCM only). This deviation is documented in `src/server/README.md`.
- **No mutual TLS (mTLS) by default**: mTLS is supported for admin API endpoints via `SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, ...)` but is not the default for public API endpoints. Client certificate validation is planned as default in v2.0.0.
- **OCSP stapling not yet implemented**: Mozilla Modern profile recommends OCSP stapling. ThemisDB's server does not yet staple OCSP responses; this is tracked as a v2.1.0 enhancement.

## ⚠️ Trade-offs & Limitations

- **Client compatibility**: TLS 1.3-only mode breaks clients on Windows 7, Android < 5.0, and OpenSSL < 1.1.1. These platforms are explicitly out of scope for ThemisDB's client support matrix.
- **Session resumption trade-off**: Disabling TLS session tickets increases handshake overhead for short-lived connections. For the connection-pooling model used by ThemisDB's clients, this is negligible.
- **Certificate rotation downtime risk**: Certificate hot-reload (`SIGHUP`-triggered) is implemented to avoid restart-based downtime, but a brief window where old and new certificates coexist requires careful handling of in-flight connections.
- **CHACHA20 performance on older hardware**: `TLS_CHACHA20_POLY1305_SHA256` is included for mobile/IoT clients that lack AES-NI hardware acceleration. On server hardware with AES-NI, `TLS_AES_256_GCM_SHA384` is always preferred via cipher ordering.

## 🔬 Validation

- [x] Code reviewed against NIST SP 800-52 Rev 2 Table 3-3 and Mozilla Modern profile
- [x] TLS configuration tested with `testssl.sh` and `sslyze`; target: grade A+ on SSL Labs
- [x] Unit tests verify TLS 1.2 connections are rejected in default configuration
- [x] Certificate expiry monitoring integrated with Prometheus alert `tls_cert_expiry_days < 30`
- [x] Module README linked (`src/server/README.md`)
- [ ] implementation_influence index updated

## 📚 Related

- [JWT Short-Lived Tokens](jwt_short_lived_tokens.md)
- [Argon2id KDF](argon2id_kdf.md)

---
**Last Updated:** 2026-04-06
