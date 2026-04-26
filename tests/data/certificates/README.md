> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# Test Certificates for Signature Verification

This directory contains test certificates and keys for testing the RSA-SHA256 signature verification system.

## Generated Files

### Certificate Authority (CA)
- `ca_cert.pem` - Test CA certificate (valid for 10 years)
- `ca_key.pem` - Test CA private key (**DO NOT USE IN PRODUCTION**)

### Valid Test Certificates
- `test_cert.pem` - 2048-bit RSA certificate (signed by CA)
- `test_key_2048.pem` - 2048-bit RSA private key
- `test_cert_3072.pem` - 3072-bit RSA certificate (signed by CA)
- `test_key_3072.pem` - 3072-bit RSA private key
- `test_cert_4096.pem` - 4096-bit RSA certificate (signed by CA)
- `test_key_4096.pem` - 4096-bit RSA private key

### Invalid/Special Test Certificates
- `self_signed_cert.pem` - Self-signed certificate (should fail chain validation)
- `self_signed_key.pem` - Self-signed private key
- `expired_cert.pem` - Expired certificate (dated 2020-01-01)
- `expired_key.pem` - Expired certificate private key
- `weak_cert_1024.pem` - 1024-bit RSA certificate (should fail minimum key size check)
- `weak_key_1024.pem` - 1024-bit RSA private key

### Test Data and Signatures
- `test_data.txt` - Sample data to be signed
- `test_data_signature.bin` - Valid signature of test_data.txt with 2048-bit key
- `test_data_signature_3072.bin` - Valid signature with 3072-bit key
- `test_data_signature_4096.bin` - Valid signature with 4096-bit key
- `test_data_signature_self.bin` - Signature with self-signed key
- `test_data_signature_tampered.bin` - Tampered signature (should fail verification)

## Regenerating Certificates

To regenerate all test certificates:

```bash
cd tests/data/certificates
./generate_test_certs.sh
```

## Manual Verification

To manually verify a signature with OpenSSL:

```bash
# Extract public key from certificate
openssl x509 -in test_cert.pem -pubkey -noout > test_pubkey.pem

# Verify signature
openssl dgst -sha256 -verify test_pubkey.pem -signature test_data_signature.bin test_data.txt
```

## Security Notes

⚠️ **WARNING**: These certificates and keys are for **TESTING ONLY**.
- All keys are committed to source control
- Do NOT use these certificates in production
- Do NOT use these keys to sign real data
- Regenerate all certificates for production use
- Use proper key management and HSM for production

## Test Scenarios

### Valid Signature Tests
1. Test with 2048-bit key → Should PASS
2. Test with 3072-bit key → Should PASS
3. Test with 4096-bit key → Should PASS

### Invalid Signature Tests
1. Test with tampered signature → Should FAIL
2. Test with tampered data → Should FAIL
3. Test with wrong certificate → Should FAIL

### Certificate Chain Tests
1. Test with CA-signed certificate → Should PASS (when CA is trusted)
2. Test with self-signed certificate → Should FAIL
3. Test with expired certificate → Should FAIL

### Key Size Tests
1. Test with 2048-bit key → Should PASS
2. Test with 3072-bit key → Should PASS
3. Test with 4096-bit key → Should PASS
4. Test with 1024-bit key → Should FAIL (too weak)

## File Format

All certificates are in PEM format (Base64 encoded X.509).
All signatures are in binary DER format.
