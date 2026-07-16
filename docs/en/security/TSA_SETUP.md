# Timestamp Authority (TSA) Setup Guide - RFC 3161

**Version:** 1.5.0  
**Status:** ✅ PRODUCTION READY  
**Compliance:** eIDAS (EU) No 910/2014, ETSI EN 319 422

---

## Overview

ThemisDB includes a complete RFC 3161 Timestamp Authority (TSA) client implementation for obtaining cryptographically secure timestamps. This feature is essential for:

- **eIDAS Compliance**: Qualified electronic timestamps (Art. 42)
- **Long-term Signature Validation**: Document timestamping with 30-year retention
- **Audit Compliance**: Tamper-proof audit logs (DSGVO Art. 30)
- **Non-repudiation**: Cryptographic proof that data existed at a specific time

---

## Architecture

### Components

1. **TimestampAuthority Client** (`src/security/timestamp_authority_openssl.cpp`)
   - RFC 3161 Time-Stamp Protocol implementation
   - OpenSSL-based cryptographic operations
   - CURL-based HTTPS communication with TSA servers
   - Support for SHA-256, SHA-384, SHA-512 hash algorithms

2. **eIDAS Timestamp Validator** (`include/security/timestamp_authority.h`)
   - Long-term validation (LTV) for archived timestamps
   - Qualified TSP (Trust Service Provider) validation
   - Age validation (default: 30 years / 10,950 days)

3. **Configuration** (`config/timestamp_authority.yaml`)
   - TSA server endpoints
   - Authentication credentials
   - Certificate validation settings

### Build Modes

ThemisDB supports two TSA implementation modes:

| Mode | Compile Flag | Use Case | Security Level |
|------|--------------|----------|----------------|
| **Production (OpenSSL)** | `THEMIS_USE_OPENSSL_TSA=ON` | Production, eIDAS compliance | ✅ Full RFC 3161 |
| **Development (Stub)** | `THEMIS_USE_OPENSSL_TSA=OFF` | Development, testing | ⚠️ Mock timestamps only |

**Default:** Production mode is **enabled by default** in all builds.

---

## Quick Start

### 1. Basic Usage

```cpp
#include "security/timestamp_authority.h"

using namespace themis::security;

// Configure TSA
TSAConfig config;
config.url = "https://freetsa.org/tsr";
config.hash_algorithm = "SHA256";
config.timeout_seconds = 30;

// Create TSA client
TimestampAuthority tsa(config);

// Get timestamp for data
std::vector<uint8_t> data = {'H', 'e', 'l', 'l', 'o'};
TimestampToken token = tsa.getTimestamp(data);

if (token.success) {
    std::cout << "Timestamp: " << token.timestamp_utc << std::endl;
    std::cout << "Serial: " << token.serial_number << std::endl;
    std::cout << "Policy OID: " << token.policy_oid << std::endl;
}

// Verify timestamp
bool valid = tsa.verifyTimestamp(data, token);
```

### 2. Using Pre-computed Hash

```cpp
// Compute hash externally
std::vector<uint8_t> hash = /* SHA-256 hash */;

// Get timestamp for hash
TimestampToken token = tsa.getTimestampForHash(hash);

// Verify with hash
bool valid = tsa.verifyTimestampForHash(hash, token);
```

### 3. Parse Existing Token

```cpp
// From Base64
std::string token_b64 = "MIIBCgYJKoZIhvcNAQcCoIH...";
TimestampToken token = tsa.parseToken(token_b64);

// From DER bytes
std::vector<uint8_t> token_der = /* ... */;
TimestampToken token = tsa.parseToken(token_der);
```

---

## Supported TSA Providers

ThemisDB has been tested with multiple TSA providers:

### 1. Free Public TSAs

#### FreeTSA (Recommended for Development)
- **URL:** `https://freetsa.org/tsr`
- **Cost:** Free
- **Registration:** Not required
- **Certificate:** Self-signed (set `verify_tsa_cert: false`)
- **Limitations:** Best effort, no SLA

**Configuration:**
```yaml
timestamp_authority:
  url: "https://freetsa.org/tsr"
  hash_algorithm: "SHA256"
  verify_tsa_cert: false
```

#### DigiCert Timestamp Server
- **URL:** `https://timestamp.digicert.com`
- **Cost:** Free
- **Registration:** Not required
- **Certificate:** Trusted CA
- **Limitations:** Rate limited

**Configuration:**
```yaml
timestamp_authority:
  url: "https://timestamp.digicert.com"
  hash_algorithm: "SHA256"
  verify_tsa_cert: true
```

#### Sectigo Timestamp Server
- **URL:** `http://timestamp.sectigo.com`
- **Cost:** Free
- **Registration:** Not required
- **Note:** HTTP only (not HTTPS)

**Configuration:**
```yaml
timestamp_authority:
  url: "http://timestamp.sectigo.com"
  hash_algorithm: "SHA256"
```

### 2. European Qualified TSAs (eIDAS Compliant)

For production use in regulated industries (finance, healthcare, government), use **qualified TSAs** from the EU Trusted List:

#### DFN-PKI (Germany)
- **URL:** `http://zeitstempel.dfn.de`
- **Provider:** DFN-Verein (German Research Network)
- **Qualification:** eIDAS qualified
- **Cost:** Free for research/education, commercial license available

#### D-TRUST (Germany)
- **URL:** Contact D-TRUST for endpoint
- **Provider:** Bundesdruckerei GmbH
- **Qualification:** eIDAS qualified
- **Cost:** Commercial (€500-2000/month)

#### Deutsche Telekom Security
- **URL:** Contact Deutsche Telekom for endpoint
- **Provider:** Deutsche Telekom AG
- **Qualification:** eIDAS qualified
- **Cost:** Commercial

**Finding Qualified TSPs:**
Visit the EU Trusted List Browser:  
https://eidas.ec.europa.eu/efda/tl-browser/

### 3. Enterprise TSA Services

#### GlobalSign TSA
- **URL:** Contact GlobalSign for endpoint
- **Cost:** Commercial (part of SSL certificate package)
- **Authentication:** Client certificate (mTLS) required
- **SLA:** 99.9% uptime

**Configuration:**
```yaml
timestamp_authority:
  url: "https://tsa.globalsign.com/tsr"
  client_cert_path: "/path/to/client.crt"
  client_key_path: "/path/to/client.key"
  verify_tsa_cert: true
```

#### SwissSign TSA
- **URL:** Contact SwissSign for endpoint
- **Cost:** Commercial
- **Authentication:** HTTP Basic Auth
- **SLA:** 99.9% uptime

**Configuration:**
```yaml
timestamp_authority:
  url: "https://tsa.swisssign.com/tsr"
  username: "your-username"
  password: "your-password"
  verify_tsa_cert: true
```

---

## Configuration Reference

### Complete Configuration File

File: `config/timestamp_authority.yaml`

```yaml
timestamp_authority:
  # Enable/disable TSA client
  enabled: true
  
  # TSA server URL (RFC 3161 endpoint)
  url: "https://freetsa.org/tsr"
  
  # Hash algorithm: SHA256 (recommended), SHA384, SHA512
  hash_algorithm: "SHA256"
  
  # Request certificate in response (recommended)
  cert_req: true
  
  # HTTP timeout in seconds
  timeout_seconds: 30
  
  # Retry configuration
  retry_attempts: 3
  retry_delay_ms: 1000
  
  # Optional: TSA authentication
  # username: "your-username"
  # password: "your-password"
  
  # Optional: Client certificate for mTLS
  # client_cert_path: "/path/to/client.crt"
  # client_key_path: "/path/to/client.key"
  
  # Optional: CA certificate for TSA validation
  # ca_cert_path: "/path/to/ca.crt"
  
  # Verify TSA certificate (recommended for production)
  verify_tsa_cert: true
  
  # Optional: Policy OID (TSA-specific)
  # policy_oid: "1.2.3.4.5.6.7.8.9"

# eIDAS compliance settings
eidas:
  enabled: true
  max_age_days: 10950  # 30 years
  qualified_tsps:
    - "D-TRUST"
    - "DFN-PKI"
    - "Bundesdruckerei"
    - "Deutsche Telekom"

# Logging and monitoring
logging:
  level: info
  log_requests: true
  log_responses: true

monitoring:
  enable_metrics: true
  max_latency_ms: 2000
  max_error_rate: 0.1
```

---

## Build Configuration

### Enable/Disable OpenSSL TSA

By default, the OpenSSL-based TSA implementation is **enabled**. To disable it (use stub mode):

**CMake:**
```bash
cmake -DTHEMIS_USE_OPENSSL_TSA=OFF ..
```

**Via cmake/CMakeLists.txt:**
```cmake
# Enable OpenSSL TSA (default)
target_compile_definitions(themis_core PUBLIC THEMIS_USE_OPENSSL_TSA)

# Disable OpenSSL TSA (stub mode)
# Comment out or remove the above line
```

### Dependencies

The OpenSSL TSA implementation requires:

- **OpenSSL 1.1.1+** (for RFC 3161 support)
- **libcurl 7.x+** (for HTTPS communication)

**Install on Ubuntu/Debian:**
```bash
sudo apt-get install libssl-dev libcurl4-openssl-dev
```

**Install on macOS:**
```bash
brew install openssl curl
```

**Install on Windows:**
```powershell
vcpkg install openssl curl
```

---

## eIDAS Compliance

### Long-term Validation (LTV)

For eIDAS compliance, timestamps must be valid for **30 years**:

```cpp
#include "security/timestamp_authority.h"

using namespace themis::security;

eIDASTimestampValidator validator;

// Validate timestamp age (30 years = 10,950 days)
bool valid = validator.validateAge(token, 10950);

if (!valid) {
    auto errors = validator.getValidationErrors();
    for (const auto& error : errors) {
        std::cerr << "Validation error: " << error << std::endl;
    }
}
```

### Qualified TSA Validation

```cpp
// Check if TSA is qualified (eIDAS QTSP)
std::vector<std::string> qtsp_list = {
    "D-TRUST",
    "DFN-PKI",
    "Bundesdruckerei"
};

std::string tsa_cert = token.getTSACertificate();
bool qualified = validator.isQualifiedTSA(tsa_cert, qtsp_list);
```

### Complete eIDAS Validation

```cpp
// Full eIDAS validation
std::vector<std::string> trust_anchors = {
    /* PEM-encoded trusted root certificates */
};

bool eidas_valid = validator.validateeIDASTimestamp(token, trust_anchors);
```

---

## Advanced Usage

### Multiple TSA Providers (Failover)

```cpp
std::vector<std::string> tsa_urls = {
    "https://freetsa.org/tsr",
    "https://timestamp.digicert.com",
    "http://timestamp.sectigo.com"
};

TimestampToken token;
for (const auto& url : tsa_urls) {
    TSAConfig config;
    config.url = url;
    config.hash_algorithm = "SHA256";
    config.timeout_seconds = 10;
    
    TimestampAuthority tsa(config);
    token = tsa.getTimestamp(data);
    
    if (token.success) {
        std::cout << "Timestamp obtained from: " << url << std::endl;
        break;
    } else {
        std::cerr << "TSA failed (" << url << "): " 
                  << token.error_message << std::endl;
    }
}
```

### Hash Algorithm Selection

```cpp
// SHA-256 (recommended, widely supported)
config.hash_algorithm = "SHA256";

// SHA-384 (stronger, less common)
config.hash_algorithm = "SHA384";

// SHA-512 (strongest, rare)
config.hash_algorithm = "SHA512";
```

### Certificate Validation

```cpp
TSAConfig config;
config.url = "https://timestamp.digicert.com";
config.verify_tsa_cert = true;  // Verify against system trust store
config.ca_cert_path = "/etc/ssl/certs/ca-bundle.crt";  // Custom CA

TimestampAuthority tsa(config);
```

### Client Certificate Authentication (mTLS)

```cpp
TSAConfig config;
config.url = "https://enterprise-tsa.example.com/tsr";
config.client_cert_path = "/path/to/client.crt";
config.client_key_path = "/path/to/client.key";
config.verify_tsa_cert = true;

TimestampAuthority tsa(config);
```

---

## Testing

### Unit Tests

Run the timestamp authority test suite:

```bash
# Run all TSA tests
./build/tests/themis_tests --gtest_filter=TimestampAuthorityTest.*

# Skip network tests (if TSA is unreachable)
THEMIS_TEST_SKIP_TSA_NETWORK_TESTS=1 ./build/tests/themis_tests
```

### Manual Testing

Test TSA connectivity:

```bash
# Test FreeTSA
curl -H "Content-Type: application/timestamp-query" \
     --data-binary @request.tsq \
     https://freetsa.org/tsr \
     -o response.tsr

# Check response
openssl ts -reply -in response.tsr -text
```

---

## Troubleshooting

### Common Issues

#### 1. "TSA request failed: curl error: Couldn't resolve host"

**Cause:** TSA URL is unreachable or DNS resolution failed.

**Solution:**
- Check internet connectivity
- Verify TSA URL is correct
- Try alternative TSA provider

#### 2. "HTTP status 400: Bad Request"

**Cause:** Invalid TSP request format.

**Solution:**
- Verify hash algorithm is supported by TSA
- Check that OpenSSL TSA mode is enabled (`THEMIS_USE_OPENSSL_TSA`)
- Ensure hash size matches algorithm (SHA-256 = 32 bytes)

#### 3. "Token marked as unsuccessful"

**Cause:** TSA rejected the timestamp request.

**Solution:**
- Check `token.pki_status` for error code
- Review `token.error_message` for details
- Verify policy OID is correct (if required by TSA)

#### 4. "Failed to parse PKCS7 token"

**Cause:** Invalid or corrupted timestamp token.

**Solution:**
- Verify token data is complete
- Check for transmission errors
- Try re-requesting timestamp

#### 5. "Using TimestampAuthority STUB - NOT SECURE for production!"

**Cause:** OpenSSL TSA mode is disabled (stub mode active).

**Solution:**
- Rebuild with `THEMIS_USE_OPENSSL_TSA=ON`
- Install OpenSSL and libcurl development packages
- Verify CMake configuration

---

## Performance Considerations

### Latency

Typical TSA request latency:

| Provider | Latency | SLA |
|----------|---------|-----|
| FreeTSA | 500-2000ms | None |
| DigiCert | 200-800ms | None |
| Enterprise TSA | 100-500ms | 99.9% |

**Recommendation:** Use async/batch processing for high-volume timestamping.

### Rate Limiting

Most free TSAs have rate limits:

- **FreeTSA:** ~10 requests/second
- **DigiCert:** ~5 requests/second

For high-volume use cases, consider:
- Enterprise TSA with higher limits
- Local caching of timestamps
- Batch timestamping

### Connection Pooling

Enable connection pooling for better performance:

```yaml
performance:
  enable_connection_pooling: true
  max_connections: 10
```

---

## Security Considerations

### Best Practices

1. **Always use HTTPS** for TSA communication (except where unavailable)
2. **Verify TSA certificates** in production (`verify_tsa_cert: true`)
3. **Use qualified TSAs** for eIDAS compliance
4. **Store timestamps securely** (encrypted at rest)
5. **Implement timestamp rotation** (re-timestamp before expiry)
6. **Monitor TSA availability** (implement failover)

### Certificate Validation

```cpp
// Strict validation (recommended for production)
config.verify_tsa_cert = true;
config.ca_cert_path = "/path/to/trusted-roots.pem";

// Relaxed validation (development only)
config.verify_tsa_cert = false;  // ⚠️ Not for production!
```

### Timestamp Storage

Store timestamps securely:

```cpp
// Save timestamp token
std::ofstream file("timestamp.tsr", std::ios::binary);
file.write(reinterpret_cast<const char*>(token.token_der.data()), 
           token.token_der.size());
file.close();

// Load timestamp token
std::ifstream file("timestamp.tsr", std::ios::binary);
std::vector<uint8_t> token_der(
    (std::istreambuf_iterator<char>(file)),
    std::istreambuf_iterator<char>()
);
TimestampToken token = tsa.parseToken(token_der);
```

---

## Compliance References

### Standards

- **RFC 3161:** Internet X.509 Public Key Infrastructure Time-Stamp Protocol (TSP)
- **eIDAS Regulation (EU) No 910/2014:** Electronic identification and trust services
- **ETSI EN 319 422:** Electronic Signatures and Infrastructures (ESI); Time-stamping protocol and time-stamp token profiles

### eIDAS Requirements

For eIDAS compliance, timestamps must:

1. ✅ Use qualified TSAs from EU Trusted List
2. ✅ Support 30-year long-term validation
3. ✅ Include accuracy metadata (optional per RFC 3161)
4. ✅ Support certificate chain validation
5. ✅ Maintain audit logs of timestamp operations

### Compliance Checklist

- [ ] TSA integrated with qualified provider
- [ ] 30-year timestamp validation implemented
- [ ] Certificate chain validation enabled
- [ ] Audit logging configured
- [ ] Legal review completed
- [ ] Documentation updated

---

## Migration from Stub Mode

If upgrading from ThemisDB < 1.5.0 with stub mode:

### Step 1: Verify Build Configuration

```bash
# Check if OpenSSL TSA is enabled
grep "THEMIS_USE_OPENSSL_TSA" build/compile_commands.json
```

### Step 2: Update Configuration

```yaml
# Before (stub mode)
timestamp_authority:
  enabled: false

# After (production mode)
timestamp_authority:
  enabled: true
  url: "https://freetsa.org/tsr"
  hash_algorithm: "SHA256"
```

### Step 3: Test Migration

```bash
# Run migration tests
./build/tests/themis_tests --gtest_filter=TimestampAuthorityTest.GetTimestampFromFreeTSA
```

### Step 4: Update Application Code

No code changes required! The API remains the same.

---

## Support

### Documentation

- **API Reference:** `include/security/timestamp_authority.h`
- **Test Examples:** `tests/test_timestamp_authority.cpp`
- **Configuration:** `config/timestamp_authority.yaml`

### Issues

Report issues at: https://github.com/makr-code/ThemisDB/issues

### License

ThemisDB Timestamp Authority implementation is licensed under the same license as ThemisDB core.

---

**Last Updated:** 2026-04-06  
**Version:** 1.5.0  
**Status:** ✅ Production Ready
