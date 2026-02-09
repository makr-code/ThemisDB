#!/bin/bash
# Generate test certificates for wire protocol TLS testing
# WARNING: These are for testing only - DO NOT use in production!

set -e

echo "Generating test certificates for wire protocol..."

# 1. Generate CA certificate
openssl req -x509 -newkey rsa:2048 -nodes \
  -keyout ca-key.pem -out ca-cert.pem \
  -subj "/CN=ThemisDB Test CA/O=ThemisDB Test/C=US" \
  -days 365

# 2. Generate server certificate
openssl req -newkey rsa:2048 -nodes \
  -keyout server-key.pem -out server-req.pem \
  -subj "/CN=localhost/O=ThemisDB Test/C=US"

openssl x509 -req -in server-req.pem \
  -CA ca-cert.pem -CAkey ca-key.pem -CAcreateserial \
  -out server-cert.pem -days 365 -sha256

# 3. Generate client certificate (for mTLS)
openssl req -newkey rsa:2048 -nodes \
  -keyout client-key.pem -out client-req.pem \
  -subj "/CN=test-client/O=ThemisDB Test/C=US"

openssl x509 -req -in client-req.pem \
  -CA ca-cert.pem -CAkey ca-key.pem -CAcreateserial \
  -out client-cert.pem -days 365 -sha256

# Clean up CSR files
rm -f server-req.pem client-req.pem

echo "Test certificates generated successfully!"
echo ""
echo "Files created:"
echo "  - ca-cert.pem / ca-key.pem (CA certificate)"
echo "  - server-cert.pem / server-key.pem (Server certificate)"
echo "  - client-cert.pem / client-key.pem (Client certificate for mTLS)"
echo ""
echo "⚠️  WARNING: These certificates are for testing only!"
