#!/bin/bash
# Generate ThemisDB Plugin Signing Certificate
# This certificate is used to sign plugin DLLs/SOs

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CERTS_DIR="$(dirname "$SCRIPT_DIR")"

echo "========================================="
echo "ThemisDB Plugin Signing Certificate Generator"
echo "========================================="
echo ""

# Configuration
CA_KEY="$CERTS_DIR/manufacturer/themisdb-ca.key"
CA_CERT="$CERTS_DIR/manufacturer/themisdb-ca.crt"
SIGNING_KEY="$CERTS_DIR/manufacturer/themisdb-plugin-signer.key"
SIGNING_CSR="$CERTS_DIR/manufacturer/themisdb-plugin-signer.csr"
SIGNING_CERT="$CERTS_DIR/manufacturer/themisdb-plugin-signer.crt"
SIGNING_SUBJECT="/CN=ThemisDB Plugin Signer/O=ThemisDB.org/C=DE"
CERT_VALIDITY_DAYS=1825  # 5 years

# Check if CA exists
if [ ! -f "$CA_KEY" ] || [ ! -f "$CA_CERT" ]; then
    echo "❌ Error: CA certificate not found. Please run generate_ca.sh first."
    exit 1
fi

echo "Generating signing certificate:"
echo "  Key: $SIGNING_KEY"
echo "  CSR: $SIGNING_CSR"
echo "  Cert: $SIGNING_CERT"
echo "  Subject: $SIGNING_SUBJECT"
echo "  Validity: $CERT_VALIDITY_DAYS days"
echo ""

# Generate private key
openssl genrsa -out "$SIGNING_KEY" 4096

# Generate CSR
openssl req -new -key "$SIGNING_KEY" -out "$SIGNING_CSR" \
    -subj "$SIGNING_SUBJECT"

# Create extension config for code signing
cat > "$CERTS_DIR/manufacturer/signing_cert_ext.cnf" << EOF
basicConstraints=CA:FALSE
keyUsage=critical,digitalSignature
extendedKeyUsage=codeSigning
subjectKeyIdentifier=hash
authorityKeyIdentifier=keyid,issuer
EOF

# Sign CSR with CA
openssl x509 -req -in "$SIGNING_CSR" \
    -CA "$CA_CERT" \
    -CAkey "$CA_KEY" \
    -CAcreateserial \
    -out "$SIGNING_CERT" \
    -days "$CERT_VALIDITY_DAYS" \
    -sha256 \
    -extfile "$CERTS_DIR/manufacturer/signing_cert_ext.cnf"

# Create certificate chain
cat "$SIGNING_CERT" "$CA_CERT" > "$CERTS_DIR/manufacturer/certificate-chain.pem"

# Clean up temporary files
rm "$SIGNING_CSR" "$CERTS_DIR/manufacturer/signing_cert_ext.cnf"

echo ""
echo "✅ Signing certificate generated successfully!"
echo ""
echo "Certificate details:"
openssl x509 -in "$SIGNING_CERT" -text -noout | grep -A 3 "Subject:"

echo ""
echo "Certificate chain created: $CERTS_DIR/manufacturer/certificate-chain.pem"
echo ""
echo "⚠️  IMPORTANT: Keep the signing private key ($SIGNING_KEY) secure!"
echo "   - Never commit it to Git"
echo "   - Store in HSM or secure key vault for production"
echo "   - Add to .gitignore"
echo ""
