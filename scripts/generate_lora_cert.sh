#!/bin/bash
# Generate self-signed certificate for LoRA adapter encryption (development/testing)

set -e

# Default values
CERT_DIR="/tmp/themis_lora_certs"
CERT_NAME="lora-encryption"
KEY_SIZE=4096
DAYS_VALID=365
COUNTRY="DE"
ORG="ThemisDB"
CN="ThemisDB-LoRA-Encryption"

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --dir)
            CERT_DIR="$2"
            shift 2
            ;;
        --name)
            CERT_NAME="$2"
            shift 2
            ;;
        --days)
            DAYS_VALID="$2"
            shift 2
            ;;
        --cn)
            CN="$2"
            shift 2
            ;;
        --org)
            ORG="$2"
            shift 2
            ;;
        --country)
            COUNTRY="$2"
            shift 2
            ;;
        --help)
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Generate self-signed certificate for LoRA adapter encryption"
            echo ""
            echo "Options:"
            echo "  --dir DIR        Output directory (default: $CERT_DIR)"
            echo "  --name NAME      Certificate name (default: $CERT_NAME)"
            echo "  --days DAYS      Validity period in days (default: $DAYS_VALID)"
            echo "  --cn CN          Common Name (default: $CN)"
            echo "  --org ORG        Organization (default: $ORG)"
            echo "  --country CODE   Country code (default: $COUNTRY)"
            echo "  --help           Show this help message"
            echo ""
            echo "Example:"
            echo "  $0 --dir /etc/themis/certs --days 730"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

# Create output directory
mkdir -p "$CERT_DIR"

KEY_FILE="$CERT_DIR/${CERT_NAME}.key"
CERT_FILE="$CERT_DIR/${CERT_NAME}.crt"

echo "======================================"
echo "LoRA Encryption Certificate Generator"
echo "======================================"
echo ""
echo "Configuration:"
echo "  Output directory: $CERT_DIR"
echo "  Certificate name: $CERT_NAME"
echo "  Key size: $KEY_SIZE bits"
echo "  Valid for: $DAYS_VALID days"
echo "  Subject: /CN=$CN/O=$ORG/C=$COUNTRY"
echo ""

# Check if files already exist
if [ -f "$KEY_FILE" ] || [ -f "$CERT_FILE" ]; then
    echo "Warning: Certificate or key file already exists!"
    read -p "Overwrite existing files? (y/N) " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        echo "Aborted."
        exit 0
    fi
fi

# Generate private key
echo "Step 1: Generating private key..."
openssl genrsa -out "$KEY_FILE" $KEY_SIZE 2>/dev/null
if [ $? -eq 0 ]; then
    echo "  ✓ Private key generated: $KEY_FILE"
else
    echo "  ✗ Failed to generate private key"
    exit 1
fi

# Set secure permissions on private key
chmod 600 "$KEY_FILE"
echo "  ✓ Private key permissions set to 600"

# Generate self-signed certificate
echo "Step 2: Generating self-signed certificate..."
openssl req -new -x509 -key "$KEY_FILE" \
    -out "$CERT_FILE" -days $DAYS_VALID \
    -subj "/CN=$CN/O=$ORG/C=$COUNTRY" 2>/dev/null

if [ $? -eq 0 ]; then
    echo "  ✓ Certificate generated: $CERT_FILE"
else
    echo "  ✗ Failed to generate certificate"
    exit 1
fi

# Set permissions on certificate
chmod 644 "$CERT_FILE"
echo "  ✓ Certificate permissions set to 644"

# Display certificate information
echo ""
echo "Step 3: Certificate verification..."
openssl x509 -in "$CERT_FILE" -noout -subject -dates -fingerprint

echo ""
echo "======================================"
echo "Certificate generation complete!"
echo "======================================"
echo ""
echo "Certificate files:"
echo "  Private key:  $KEY_FILE"
echo "  Certificate:  $CERT_FILE"
echo ""
echo "Configuration example (C++):"
echo "  config.use_pki_for_encryption = true;"
echo "  config.pki_cert_path = \"$CERT_FILE\";"
echo "  config.pki_private_key_path = \"$KEY_FILE\";"
echo ""
echo "Configuration example (YAML):"
echo "  use_pki_for_encryption: true"
echo "  pki_cert_path: $CERT_FILE"
echo "  pki_private_key_path: $KEY_FILE"
echo ""
echo "IMPORTANT SECURITY NOTES:"
echo "  • This is a SELF-SIGNED certificate for development/testing only"
echo "  • For production, obtain a certificate from a trusted CA"
echo "  • Keep the private key secure and never commit it to version control"
echo "  • Set up monitoring for certificate expiration"
echo "  • Certificate expires in $DAYS_VALID days ($(date -d "+$DAYS_VALID days" +%Y-%m-%d))"
echo ""
