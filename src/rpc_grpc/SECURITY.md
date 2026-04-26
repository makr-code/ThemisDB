> **Sicherheitshinweis:** Security-Angaben gegen aktuelle Build-Flags, Codepfade und Tests validieren.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Security — gRPC Plugin

## Threat Model

### 1. Unauthenticated gRPC Connections
- **Risk:** Clients connect to port 50051 without valid credentials, accessing RPC
  methods without authorisation.
- **Mitigation:** When `tls_enabled=true` and `auth_required=true`, the server requires
  and verifies client certificates (mTLS). gRPC-level authentication (e.g., JWT bearer
  tokens in metadata) is the responsibility of the service implementation layer.
- **Status:** ✅ mTLS enforced when configured

### 2. TLS Downgrade / Insecure Fallback
- **Risk:** A misconfiguration or file I/O error causes the server to start on an
  insecure transport, silently exposing all RPC traffic.
- **Mitigation:** `configureCredentials()` throws `std::runtime_error` on any certificate
  load failure when `tls_enabled=true`. `GRPCServer::start()` propagates the exception;
  the server never starts. The insecure path is only reachable when `tls_enabled=false`
  (explicit opt-out, documented as development-only).
- **Status:** ✅ Fail-closed by design; no silent fallback

### 3. Man-in-the-Middle (MitM)
- **Risk:** Plaintext interception of gRPC traffic between client and server.
- **Mitigation:** TLS 1.2+ is enforced by gRPC's OpenSSL/BoringSSL TLS context when
  `SslServerCredentials()` is used. Protocol downgrade to TLS 1.0/1.1 is blocked by
  the gRPC TLS layer default configuration.
- **Status:** ✅ TLS 1.2+ via gRPC/BoringSSL

### 4. Oversized gRPC Messages (Memory Exhaustion)
- **Risk:** A client sends an oversized request message to exhaust server heap memory.
- **Mitigation:** `SetMaxReceiveMessageSize(100 * 1024 * 1024)` (100 MB) caps inbound
  message size. gRPC rejects messages exceeding this limit with `RESOURCE_EXHAUSTED`.
- **Status:** ✅ Hardcoded 100 MB cap; configurable via future config key

### 5. Certificate Private Key Exposure
- **Risk:** The server private key file (`tls_key_path`) is readable by unprivileged
  processes if file permissions are too broad.
- **Mitigation:** The plugin reads certificate files at startup but does not cache the
  raw key string beyond credential construction. Operators must ensure certificate
  files have mode 0600 and are owned by the ThemisDB process user.
- **Status:** ⚠️ File permission enforcement is operator responsibility

### 6. gRPC Reflection API
- **Risk:** gRPC server reflection exposes all registered service/method names to
  unauthenticated clients, aiding reconnaissance.
- **Mitigation:** The gRPC Reflection service is not registered by `GRPCServer`;
  callers that explicitly add it are responsible for restricting access.
- **Status:** ✅ Reflection not auto-registered

---

## Security Controls Summary

| Control | Implementation | Status |
|---------|---------------|--------|
| Fail-closed TLS | Throws on cert load failure; never falls back to insecure | ✅ |
| mTLS client cert verification | `GRPC_SSL_REQUEST_AND_REQUIRE_CLIENT_CERTIFICATE_AND_VERIFY` | ✅ |
| TLS 1.2+ enforcement | gRPC/BoringSSL default TLS context | ✅ |
| Message size cap | 100 MB receive + 100 MB send | ✅ |
| Reflection service | Not auto-registered | ✅ |
| Key file permissions | Operator responsibility | ⚠️ |

---

## Known Limitations

| ID | Description | Severity | Status |
|----|-------------|----------|--------|
| GRPC-SEC-01 | `reloadTls()` updates credentials for new connections only; existing TLS sessions are not renegotiated | Low | By design |
| GRPC-SEC-02 | No per-method authorisation at transport layer; relies on service implementation | Medium | By design |
| GRPC-SEC-03 | Max message size (100 MB) not yet configurable via `RPCServerConfig` | Low | Open |
