# Wire Protocol Test Certificates

⚠️ **FOR TESTING ONLY - DO NOT USE IN PRODUCTION!**

This directory contains test certificates for wire protocol TLS/mTLS testing.

## Generated Certificates

- `ca-cert.pem` / `ca-key.pem` - Test Certificate Authority
- `server-cert.pem` / `server-key.pem` - Server certificate (CN=localhost)
- `client-cert.pem` / `client-key.pem` - Client certificate for mTLS (CN=test-client)

## Regenerating Certificates

Run the generation script:

```bash
./generate_test_certs.sh
```

## Usage in Tests

These certificates are used by:
- `tests/test_wire_protocol_connection_pool.cpp` - TLS connection pool tests
- `tests/test_wire_protocol_integration.cpp` - Wire protocol integration tests

## Certificate Details

- **Algorithm:** RSA 2048-bit
- **Signature:** SHA-256
- **Validity:** 365 days
- **Organization:** ThemisDB Test
- **Country:** US

## Security Notice

These certificates are:
- ✅ Valid for automated testing
- ✅ Valid for local development
- ❌ **NEVER** to be used in production
- ❌ **NEVER** to be committed to secure storage
- ❌ **NEVER** to be trusted by production systems

The private keys are intentionally weak and publicly available for testing purposes only.

---

**Version:** 1.0  
**Last Updated:** 2026-02-09
