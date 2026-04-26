#!/bin/bash
# Generate ThemisDB.org Plugin Signing CA Certificate
# This script creates a self-signed CA certificate for signing ThemisDB plugins

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CERTS_DIR="$(dirname "$SCRIPT_DIR")"

echo "========================================="
echo "ThemisDB Plugin Signing CA Generator"
echo "========================================="
echo ""

# Configuration
CA_KEY="$CERTS_DIR/manufacturer/themisdb-ca.key"
CA_CERT="$CERTS_DIR/manufacturer/themisdb-ca.crt"
CA_SUBJECT="/CN=ThemisDB Official Plugins CA/O=ThemisDB.org/C=DE"
CA_VALIDITY_DAYS=3650  # 10 years

echo "Generating CA certificate:"
echo "  Key: $CA_KEY"
echo "  Cert: $CA_CERT"
echo "  Subject: $CA_SUBJECT"
echo "  Validity: $CA_VALIDITY_DAYS days"
echo ""

# Create manufacturer directory if it doesn't exist
mkdir -p "$CERTS_DIR/manufacturer"

# Generate CA private key and self-signed certificate
openssl req -x509 -newkey rsa:4096 -sha256 -days "$CA_VALIDITY_DAYS" \
    -keyout "$CA_KEY" \
    -out "$CA_CERT" \
    -nodes \
    -subj "$CA_SUBJECT" \
    -addext "basicConstraints=critical,CA:TRUE" \
    -addext "keyUsage=critical,keyCertSign,cRLSign" \
    -addext "subjectKeyIdentifier=hash"

echo ""
echo "✅ CA Certificate generated successfully!"
echo ""
echo "Certificate details:"
openssl x509 -in "$CA_CERT" -text -noout | grep -A 3 "Subject:"

echo ""
echo "⚠️  IMPORTANT: Keep the CA private key ($CA_KEY) secure!"
echo "   - Never commit it to Git"
echo "   - Store in HSM or secure key vault for production"
echo "   - Add to .gitignore"
echo ""
