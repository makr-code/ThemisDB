# plugin_signer - Plugin Digital Signature Tool

> ⚠️ **[PRIVATE] Governance Update (Hyperscaler):**
> Signing/Signatur-Generierung ist owner-kontrolliert und wurde in
> `plugins/private/themisdb_plugin_signer` ausgelagert.
> Das öffentliche Repository stellt nur Verifikationspfade bereit.

## Overview

The historical `plugin_signer` tool was used to sign hardware acceleration plugins (DLLs/SOs).
For Hyperscaler security hardening, signing is now private-owner-only; this public documentation is retained for verification context and migration.

## Use Cases

- **Plugin Distribution:** Sign official plugins before distribution
- **Security Verification:** Ensure plugins haven't been tampered with
- **Trust Establishment:** Verify plugin authenticity with digital certificates
- **Compliance:** Meet security requirements for plugin loading
- **Development:** Sign development plugins for testing

## Requirements

- Python 3.8 or later
- OpenSSL (for certificate generation)
- Plugin file to sign (DLL, SO, or DYLIB)
- Private key and certificate
- Optional: Hardware Security Module (HSM) for production

## Installation

```bash
cd /path/to/ThemisDB/plugins/private/themisdb_plugin_signer

# Install dependencies (if needed)
pip install cryptography
```

## Basic Usage

### Sign a Plugin

```bash
python3 sign_plugin.py \
  <plugin_file> \
  <private_key> \
  <certificate>
```

**Example:**
```bash
python3 sign_plugin.py \
  ../../plugins/themis_accel_cuda.dll \
  certs/themis_plugin_key.pem \
  certs/themis_plugin_cert.pem
```

**Output:**
```
✓ Plugin signed successfully
  SHA-256: a1b2c3d4e5f6...
  Signature: themis_accel_cuda.dll.json
```

### Verify Plugin Signature

```bash
python3 verify_plugin.py \
  ../../plugins/themis_accel_cuda.dll \
  certs/themis_plugin_cert.pem
```

## Certificate Generation

### Development Certificates

For development and testing, generate self-signed certificates:

```bash
cd plugins/private/themisdb_plugin_signer

# 1. Generate private key (4096-bit RSA)
openssl genrsa -out themis_plugin_key.pem 4096

# 2. Generate self-signed certificate (valid 1 year)
openssl req -new -x509 \
  -key themis_plugin_key.pem \
  -out themis_plugin_cert.pem \
  -days 365 \
  -subj "/CN=ThemisDB Official Plugins/O=ThemisDB/C=DE"

# 3. View certificate details
openssl x509 -in themis_plugin_cert.pem -text -noout
```

### Production Certificates

For production, obtain a code signing certificate from a trusted CA:

1. **Purchase Certificate:** From DigiCert, Sectigo, or similar CA
2. **Generate CSR:** Create certificate signing request
3. **Submit CSR:** Send to CA for signing
4. **Receive Certificate:** Install signed certificate
5. **Store Securely:** Use HSM or secure key management system

```bash
# Generate CSR for CA
openssl req -new \
  -key themis_plugin_key.pem \
  -out themis_plugin.csr \
  -subj "/CN=ThemisDB Plugins/O=Your Company/C=US"

# Submit themis_plugin.csr to CA
# Receive signed certificate: themis_plugin_cert.pem
```

## Signature Metadata Format

The tool generates a JSON signature file:

**File:** `themis_accel_cuda.dll.json`

```json
{
  "plugin": {
    "name": "themis_accel_cuda.dll",
    "size": 2458624,
    "timestamp": "2026-01-12T14:00:00Z"
  },
  "hash": {
    "algorithm": "SHA-256",
    "value": "a1b2c3d4e5f6789012345678901234567890abcdef1234567890abcdef123456"
  },
  "signature": {
    "algorithm": "RSA-SHA256",
    "value": "MIIGHwYJKoZIhvcNAQcCoIIGED...",
    "key_size": 4096
  },
  "certificate": {
    "subject": "CN=ThemisDB Official Plugins,O=ThemisDB,C=DE",
    "issuer": "CN=ThemisDB Official Plugins,O=ThemisDB,C=DE",
    "serial": "1234567890",
    "valid_from": "2025-01-12T00:00:00Z",
    "valid_to": "2027-01-12T00:00:00Z",
    "fingerprint": "SHA256:ab:cd:ef:12:34:56:78:90..."
  },
  "metadata": {
    "signer": "plugin_signer v1.0",
    "version": "1.0.0"
  }
}
```

## Batch Signing

Sign multiple plugins at once:

```bash
#!/bin/bash
# batch_sign_plugins.sh

KEY="certs/themis_plugin_key.pem"
CERT="certs/themis_plugin_cert.pem"
PLUGIN_DIR="../../plugins"

for plugin in $PLUGIN_DIR/*.{dll,so,dylib}; do
  if [ -f "$plugin" ]; then
    echo "Signing: $plugin"
    python3 sign_plugin.py "$plugin" "$KEY" "$CERT"
  fi
done

echo "All plugins signed"
```

## Advanced Usage

### Sign with Timestamp

Include RFC 3161 timestamp for long-term validity:

```bash
python3 sign_plugin.py \
  plugin.dll \
  key.pem \
  cert.pem \
  --timestamp-server http://timestamp.digicert.com
```

### ECDSA Signatures

Use ECDSA instead of RSA for smaller signatures:

```bash
# Generate ECDSA key
openssl ecparam -name secp384r1 -genkey -out ecdsa_key.pem

# Generate certificate
openssl req -new -x509 \
  -key ecdsa_key.pem \
  -out ecdsa_cert.pem \
  -days 365 \
  -subj "/CN=ThemisDB ECDSA/O=ThemisDB/C=DE"

# Sign plugin
python3 sign_plugin.py \
  plugin.dll \
  ecdsa_key.pem \
  ecdsa_cert.pem \
  --algorithm ecdsa
```

### Hardware Security Module (HSM)

For production, use HSM for key storage:

```bash
# Configure HSM
export PKCS11_MODULE=/usr/lib/libsofthsm2.so
export HSM_PIN=your-secure-pin
export HSM_KEY_ID=plugin-signing-key

# Sign using HSM
python3 sign_plugin.py \
  plugin.dll \
  --hsm \
  --hsm-key-id $HSM_KEY_ID \
  cert.pem
```

## Security Best Practices

### Private Key Protection

⚠️ **Critical:** Private keys must be protected!

- **Never commit to Git:** Add `*.pem` to `.gitignore`
- **Restrict permissions:** `chmod 600 themis_plugin_key.pem`
- **Use secure storage:** HSM, Azure Key Vault, AWS KMS
- **Rotate regularly:** Replace keys every 12-24 months
- **Separate dev/prod:** Different keys for development and production

### Certificate Management

- **Use CA-signed certificates** for production
- **Self-signed only for development**
- **Monitor expiration dates**
- **Revoke compromised certificates**
- **Maintain certificate chain**

### Verification Process

ThemisDB plugin system verifies:

1. **Signature validity:** Cryptographic signature matches
2. **Certificate trust:** Certificate is from trusted CA
3. **Certificate validity:** Not expired or revoked
4. **Hash integrity:** Plugin file hash matches signed hash

## Integration

### Build Pipeline

```yaml
# .github/workflows/build-plugins.yml
- name: Build Plugin
  run: |
    cd plugins/accel_cuda
    cmake --build build --config Release

- name: Sign Plugin
  run: |
    python3 plugins/private/themisdb_plugin_signer/sign_plugin.py \
      plugins/accel_cuda/build/themis_accel_cuda.dll \
      ${{ secrets.PLUGIN_SIGNING_KEY }} \
      ${{ secrets.PLUGIN_SIGNING_CERT }}

- name: Package Plugin
  run: |
    tar -czf themis_accel_cuda.tar.gz \
      plugins/accel_cuda/build/themis_accel_cuda.dll \
      plugins/accel_cuda/build/themis_accel_cuda.dll.json
```

### Distribution

```bash
# Upload signed plugin to distribution server
scp themis_accel_cuda.dll \
    themis_accel_cuda.dll.json \
    deploy@cdn.example.com:/var/www/plugins/

# Users download both files
wget https://cdn.example.com/plugins/themis_accel_cuda.dll
wget https://cdn.example.com/plugins/themis_accel_cuda.dll.json
```

## Troubleshooting

### "Could not load private key"

**Cause:** Invalid key file or wrong format

**Solution:**
```bash
# Verify key format
openssl rsa -in themis_plugin_key.pem -check -noout

# Convert from PKCS#8 to traditional format if needed
openssl rsa -in key_pkcs8.pem -out key_traditional.pem
```

### "Certificate expired"

**Cause:** Certificate past validity period

**Solution:**
```bash
# Check expiration
openssl x509 -in cert.pem -noout -enddate

# Generate new certificate
openssl req -new -x509 -key key.pem -out new_cert.pem -days 365
```

### "Signature verification failed"

**Cause:** Plugin modified after signing, or wrong certificate

**Solution:**
- Re-sign plugin if legitimately modified
- Verify correct certificate file
- Check for file corruption

### "Permission denied" when signing

**Cause:** Insufficient file permissions

**Solution:**
```bash
# Make key readable (owner only)
chmod 600 themis_plugin_key.pem

# Make script executable
chmod +x sign_plugin.py
```

## Plugin Loading

ThemisDB verifies plugins during loading:

```cpp
// In plugin loader
if (!verify_plugin_signature("themis_accel_cuda.dll", 
                            "themis_accel_cuda.dll.json")) {
    throw std::runtime_error("Plugin signature invalid");
}
```

Configuration:
```yaml
# themis_config.yaml
plugins:
  require_signature: true  # Reject unsigned plugins
  trusted_certificates:
    - /etc/themis/certs/official_plugins.pem
    - /etc/themis/certs/partner_plugins.pem
```

## See Also

- [Plugin Security Documentation](../../security/PLUGIN_SECURITY.md)
- [Plugin Development Guide](../../../plugins/README.md)
- [sign_plugin_manifest.py](sign-plugin-manifest.md) - Sign manifest files
- [Security Best Practices](../../security/best_practices.md)

## License

Part of ThemisDB, licensed under the project's main license.
