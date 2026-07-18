#!/bin/bash
# Generate test certificates for signature verification tests

set -e

echo "=== Generating Test Certificates ==="

# 1. Generate CA (Certificate Authority) key and certificate
echo "1. Generating CA certificate..."
openssl genrsa -out ca_key.pem 2048
openssl req -new -x509 -days 3650 -key ca_key.pem -out ca_cert.pem \
    -subj "/C=US/ST=Test/L=Test/O=ThemisDB Test CA/CN=Test CA"

# 2. Generate test signing key (2048 bits)
echo "2. Generating 2048-bit RSA test key..."
openssl genrsa -out test_key_2048.pem 2048

# 3. Create certificate signing request
echo "3. Creating CSR..."
openssl req -new -key test_key_2048.pem -out test_csr.pem \
    -subj "/C=US/ST=Test/L=Test/O=ThemisDB/CN=test.themisdb.com"

# 4. Sign the certificate with CA
echo "4. Signing certificate with CA..."
openssl x509 -req -in test_csr.pem -CA ca_cert.pem -CAkey ca_key.pem \
    -CAcreateserial -out test_cert.pem -days 365 -sha256

# 5. Generate 3072-bit key for additional tests
echo "5. Generating 3072-bit RSA test key..."
openssl genrsa -out test_key_3072.pem 3072
openssl req -new -key test_key_3072.pem -out test_csr_3072.pem \
    -subj "/C=US/ST=Test/L=Test/O=ThemisDB/CN=test-3072.themisdb.com"
openssl x509 -req -in test_csr_3072.pem -CA ca_cert.pem -CAkey ca_key.pem \
    -CAcreateserial -out test_cert_3072.pem -days 365 -sha256

# 6. Generate 4096-bit key for additional tests
echo "6. Generating 4096-bit RSA test key..."
openssl genrsa -out test_key_4096.pem 4096
openssl req -new -key test_key_4096.pem -out test_csr_4096.pem \
    -subj "/C=US/ST=Test/L=Test/O=ThemisDB/CN=test-4096.themisdb.com"
openssl x509 -req -in test_csr_4096.pem -CA ca_cert.pem -CAkey ca_key.pem \
    -CAcreateserial -out test_cert_4096.pem -days 365 -sha256

# 7. Generate self-signed certificate (should fail chain validation)
echo "7. Generating self-signed certificate..."
openssl genrsa -out self_signed_key.pem 2048
openssl req -new -x509 -days 365 -key self_signed_key.pem -out self_signed_cert.pem \
    -subj "/C=US/ST=Test/L=Test/O=ThemisDB/CN=selfsigned.themisdb.com"

# 8. Generate expired certificate
echo "8. Generating expired certificate..."
openssl genrsa -out expired_key.pem 2048
openssl req -new -x509 -days 1 -key expired_key.pem -out expired_cert.pem \
    -subj "/C=US/ST=Test/L=Test/O=ThemisDB/CN=expired.themisdb.com"
# Back-date it to make it expired
touch -t 202001010000 expired_cert.pem

# 9. Generate weak key (1024 bits - should fail)
echo "9. Generating weak 1024-bit key..."
openssl genrsa -out weak_key_1024.pem 1024
openssl req -new -key weak_key_1024.pem -out weak_csr.pem \
    -subj "/C=US/ST=Test/L=Test/O=ThemisDB/CN=weak.themisdb.com"
openssl x509 -req -in weak_csr.pem -CA ca_cert.pem -CAkey ca_key.pem \
    -CAcreateserial -out weak_cert_1024.pem -days 365 -sha256

# 10. Create test data file
echo "10. Creating test data file..."
echo "Hello, ThemisDB! This is test data for signature verification." > test_data.txt

# 11. Sign test data with different keys
echo "11. Signing test data..."
openssl dgst -sha256 -sign test_key_2048.pem -out test_data_signature.bin test_data.txt
openssl dgst -sha256 -sign test_key_3072.pem -out test_data_signature_3072.bin test_data.txt
openssl dgst -sha256 -sign test_key_4096.pem -out test_data_signature_4096.bin test_data.txt
openssl dgst -sha256 -sign self_signed_key.pem -out test_data_signature_self.bin test_data.txt

# 12. Create invalid signature (tampered)
echo "12. Creating tampered signature..."
cp test_data_signature.bin test_data_signature_tampered.bin
echo "tampered" >> test_data_signature_tampered.bin

# 13. Generate ECDSA test certificates (Phase 2 Block B)
echo "13. Generating P-256 ECDSA key..."
openssl ecparam -name prime256v1 -genkey -noout -out test_key_p256.pem

echo "14. Creating P-256 CSR..."
openssl req -new -key test_key_p256.pem -out test_csr_p256.pem \
    -subj "/C=US/ST=Test/L=Test/O=ThemisDB/CN=test-p256.themisdb.com"

echo "15. Signing P-256 certificate with CA..."
openssl x509 -req -in test_csr_p256.pem -CA ca_cert.pem -CAkey ca_key.pem \
    -CAcreateserial -out test_cert_p256.pem -days 365 -sha256

echo "16. Generating P-384 ECDSA key..."
openssl ecparam -name secp384r1 -genkey -noout -out test_key_p384.pem

echo "17. Creating P-384 CSR..."
openssl req -new -key test_key_p384.pem -out test_csr_p384.pem \
    -subj "/C=US/ST=Test/L=Test/O=ThemisDB/CN=test-p384.themisdb.com"

echo "18. Signing P-384 certificate with CA..."
openssl x509 -req -in test_csr_p384.pem -CA ca_cert.pem -CAkey ca_key.pem \
    -CAcreateserial -out test_cert_p384.pem -days 365 -sha256

# 14. Generate ECDSA-SHA256 signatures
echo "19. Creating ECDSA-SHA256 signature with P-256..."
openssl dgst -sha256 -sign test_key_p256.pem -out test_data_signature_ecdsa_p256_sha256.bin test_data.txt

echo "20. Creating ECDSA-SHA256 signature with P-384..."
openssl dgst -sha256 -sign test_key_p384.pem -out test_data_signature_ecdsa_p384_sha256.bin test_data.txt

# 15. Generate ECDSA-SHA384 signatures
echo "21. Creating ECDSA-SHA384 signature with P-256..."
openssl dgst -sha384 -sign test_key_p256.pem -out test_data_signature_ecdsa_p256_sha384.bin test_data.txt

echo "22. Creating ECDSA-SHA384 signature with P-384..."
openssl dgst -sha384 -sign test_key_p384.pem -out test_data_signature_ecdsa_p384_sha384.bin test_data.txt

# 16. Generate tampered ECDSA signatures
echo "23. Creating tampered ECDSA signatures..."
cp test_data_signature_ecdsa_p256_sha256.bin test_data_signature_ecdsa_p256_sha256_tampered.bin
echo "tampered" >> test_data_signature_ecdsa_p256_sha256_tampered.bin

cp test_data_signature_ecdsa_p384_sha256.bin test_data_signature_ecdsa_p384_sha256_tampered.bin
echo "tampered" >> test_data_signature_ecdsa_p384_sha256_tampered.bin

cp test_data_signature_ecdsa_p256_sha384.bin test_data_signature_ecdsa_p256_sha384_tampered.bin
echo "tampered" >> test_data_signature_ecdsa_p256_sha384_tampered.bin

cp test_data_signature_ecdsa_p384_sha384.bin test_data_signature_ecdsa_p384_sha384_tampered.bin
echo "tampered" >> test_data_signature_ecdsa_p384_sha384_tampered.bin

# 17. Generate unsupported curve (P-521) for negative tests
echo "24. Generating P-521 ECDSA key (unsupported curve for testing)..."
openssl ecparam -name secp521r1 -genkey -noout -out test_key_p521.pem

echo "25. Creating P-521 CSR..."
openssl req -new -key test_key_p521.pem -out test_csr_p521.pem \
    -subj "/C=US/ST=Test/L=Test/O=ThemisDB/CN=test-p521.themisdb.com"

echo "26. Signing P-521 certificate with CA..."
openssl x509 -req -in test_csr_p521.pem -CA ca_cert.pem -CAkey ca_key.pem \
    -CAcreateserial -out test_cert_p521.pem -days 365 -sha256

echo "27. Creating signature with P-521 (unsupported curve)..."
openssl dgst -sha256 -sign test_key_p521.pem -out test_data_signature_ecdsa_p521.bin test_data.txt

# Clean up intermediate files
rm -f test_csr.pem test_csr_3072.pem test_csr_4096.pem test_csr_p256.pem test_csr_p384.pem test_csr_p521.pem weak_csr.pem ca_cert.srl

echo ""
echo "=== Test Certificates Generated Successfully ==="
echo "CA Certificate: ca_cert.pem"
echo "RSA Test Certificates: test_cert.pem (2048), test_cert_3072.pem, test_cert_4096.pem"
echo "ECDSA Test Certificates: test_cert_p256.pem, test_cert_p384.pem, test_cert_p521.pem"
echo "Self-signed: self_signed_cert.pem"
echo "Expired: expired_cert.pem"
echo "Weak: weak_cert_1024.pem"
echo "Test Data: test_data.txt"
echo "RSA Signatures: test_data_signature*.bin"
echo "ECDSA-SHA256 Signatures: test_data_signature_ecdsa_{p256,p384,p521}_sha256.bin"
echo "ECDSA-SHA384 Signatures: test_data_signature_ecdsa_{p256,p384}_sha384.bin"
echo ""
echo "To verify a signature manually:"
echo "  openssl dgst -sha256 -verify <(openssl x509 -in test_cert.pem -pubkey -noout) -signature test_data_signature.bin test_data.txt"
echo "  openssl dgst -sha256 -verify <(openssl x509 -in test_cert_p256.pem -pubkey -noout) -signature test_data_signature_ecdsa_p256_sha256.bin test_data.txt"
