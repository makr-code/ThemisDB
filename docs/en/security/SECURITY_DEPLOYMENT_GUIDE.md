# ThemisDB Security Deployment Guide

**Version:** 1.4.0  
**Last Updated:** 2026-04-06  
**Target Audience:** Security Engineers, DevOps Engineers, Database Administrators, Compliance Officers

---

## Table of Contents

1. [Pre-deployment Security Checklist](#pre-deployment-security-checklist)
2. [TLS/mTLS Configuration](#tlsmtls-configuration)
3. [Access Control Implementation](#access-control-implementation)
4. [Encryption Configuration](#encryption-configuration)
5. [Audit Logging](#audit-logging)
6. [Network Security](#network-security)
7. [Security Hardening](#security-hardening)
8. [Compliance Considerations](#compliance-considerations)

---

## Pre-deployment Security Checklist

### Critical Security Requirements

**Pre-deployment Verification:**

```bash
#!/bin/bash
# security_pre_deployment_check.sh
# Run this script before deploying ThemisDB to production

echo "=== ThemisDB Security Pre-Deployment Checklist ==="
echo "Version: 1.4.0"
echo "Date: $(date)"
echo ""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

CHECKS_PASSED=0
CHECKS_FAILED=0

check_pass() {
    echo -e "${GREEN}✓${NC} $1"
    ((CHECKS_PASSED++))
}

check_fail() {
    echo -e "${RED}✗${NC} $1"
    ((CHECKS_FAILED++))
}

check_warn() {
    echo -e "${YELLOW}⚠${NC} $1"
}

# 1. TLS Configuration
echo "--- 1. TLS/SSL Configuration ---"
if [ -f "/opt/themisdb/certs/server.crt" ] && [ -f "/opt/themisdb/certs/server.key" ]; then
    check_pass "TLS certificates found"
    
    # Check certificate expiration
    CERT_EXPIRY=$(openssl x509 -enddate -noout -in /opt/themisdb/certs/server.crt | cut -d= -f2)
    EXPIRY_EPOCH=$(date -d "$CERT_EXPIRY" +%s)
    NOW_EPOCH=$(date +%s)
    DAYS_UNTIL_EXPIRY=$(( ($EXPIRY_EPOCH - $NOW_EPOCH) / 86400 ))
    
    if [ $DAYS_UNTIL_EXPIRY -gt 30 ]; then
        check_pass "Certificate valid for $DAYS_UNTIL_EXPIRY days"
    elif [ $DAYS_UNTIL_EXPIRY -gt 7 ]; then
        check_warn "Certificate expires in $DAYS_UNTIL_EXPIRY days - plan renewal"
    else
        check_fail "Certificate expires in $DAYS_UNTIL_EXPIRY days - URGENT RENEWAL REQUIRED"
    fi
    
    # Check key permissions
    KEY_PERMS=$(stat -c "%a" /opt/themisdb/certs/server.key)
    if [ "$KEY_PERMS" = "600" ] || [ "$KEY_PERMS" = "400" ]; then
        check_pass "Private key permissions secure ($KEY_PERMS)"
    else
        check_fail "Private key permissions insecure ($KEY_PERMS) - should be 600 or 400"
    fi
else
    check_fail "TLS certificates not found - TLS is REQUIRED for production"
fi

# 2. Access Control
echo ""
echo "--- 2. Access Control Configuration ---"
if grep -q "authentication_required: true" /opt/themisdb/config/themisdb.yaml 2>/dev/null; then
    check_pass "Authentication enabled"
else
    check_fail "Authentication disabled - MUST be enabled for production"
fi

if grep -q "rbac_enabled: true" /opt/themisdb/config/themisdb.yaml 2>/dev/null; then
    check_pass "RBAC enabled"
else
    check_warn "RBAC not enabled - recommended for production"
fi

# 3. Encryption
echo ""
echo "--- 3. Encryption Configuration ---"
if grep -q "encryption_at_rest: true" /opt/themisdb/config/themisdb.yaml 2>/dev/null; then
    check_pass "Encryption at rest enabled"
else
    check_fail "Encryption at rest disabled - REQUIRED for production"
fi

if [ -f "/opt/themisdb/keys/master.key" ]; then
    check_pass "Master encryption key found"
    
    KEY_PERMS=$(stat -c "%a" /opt/themisdb/keys/master.key)
    if [ "$KEY_PERMS" = "600" ] || [ "$KEY_PERMS" = "400" ]; then
        check_pass "Master key permissions secure ($KEY_PERMS)"
    else
        check_fail "Master key permissions insecure ($KEY_PERMS)"
    fi
else
    check_fail "Master encryption key not found"
fi

# 4. Audit Logging
echo ""
echo "--- 4. Audit Logging Configuration ---"
if grep -q "audit_logging: true" /opt/themisdb/config/themisdb.yaml 2>/dev/null; then
    check_pass "Audit logging enabled"
else
    check_fail "Audit logging disabled - REQUIRED for production"
fi

if [ -d "/var/log/themisdb/audit" ]; then
    check_pass "Audit log directory exists"
else
    check_fail "Audit log directory not found"
fi

# 5. Network Security
echo ""
echo "--- 5. Network Security Configuration ---"
if grep -q "bind_address: 127.0.0.1" /opt/themisdb/config/themisdb.yaml 2>/dev/null || \
   grep -q "bind_address: 0.0.0.0" /opt/themisdb/config/themisdb.yaml 2>/dev/null; then
    
    if grep -q "bind_address: 0.0.0.0" /opt/themisdb/config/themisdb.yaml 2>/dev/null; then
        check_warn "Server bound to 0.0.0.0 - ensure firewall rules are configured"
    else
        check_pass "Server bound to localhost only"
    fi
else
    check_pass "Server bound to specific interface"
fi

if command -v ufw &> /dev/null; then
    if ufw status | grep -q "Status: active"; then
        check_pass "Firewall (ufw) is active"
    else
        check_warn "Firewall (ufw) is inactive"
    fi
fi

# 6. File System Permissions
echo ""
echo "--- 6. File System Security ---"
CONFIG_DIR="/opt/themisdb/config"
if [ -d "$CONFIG_DIR" ]; then
    DIR_PERMS=$(stat -c "%a" $CONFIG_DIR)
    if [ "$DIR_PERMS" = "750" ] || [ "$DIR_PERMS" = "700" ]; then
        check_pass "Config directory permissions secure ($DIR_PERMS)"
    else
        check_warn "Config directory permissions: $DIR_PERMS (recommended: 750)"
    fi
fi

DATA_DIR="/var/lib/themisdb"
if [ -d "$DATA_DIR" ]; then
    DIR_PERMS=$(stat -c "%a" $DATA_DIR)
    if [ "$DIR_PERMS" = "700" ]; then
        check_pass "Data directory permissions secure ($DIR_PERMS)"
    else
        check_warn "Data directory permissions: $DIR_PERMS (recommended: 700)"
    fi
fi

# 7. Default Credentials
echo ""
echo "--- 7. Default Credentials Check ---"
if [ -f "/opt/themisdb/config/users.yaml" ]; then
    if grep -q "username: admin" /opt/themisdb/config/users.yaml && \
       grep -q "password: admin" /opt/themisdb/config/users.yaml; then
        check_fail "Default admin credentials detected - MUST be changed"
    else
        check_pass "No default credentials detected"
    fi
fi

# 8. Backup Security
echo ""
echo "--- 8. Backup Security ---"
if [ -d "/mnt/backup/themisdb" ]; then
    check_pass "Backup directory exists"
    
    if command -v gpg &> /dev/null; then
        check_pass "GPG available for backup encryption"
    else
        check_warn "GPG not installed - backup encryption recommended"
    fi
else
    check_warn "Backup directory not configured"
fi

# 9. SELinux/AppArmor
echo ""
echo "--- 9. Mandatory Access Control ---"
if command -v getenforce &> /dev/null; then
    SELINUX_STATUS=$(getenforce)
    if [ "$SELINUX_STATUS" = "Enforcing" ]; then
        check_pass "SELinux is enforcing"
    elif [ "$SELINUX_STATUS" = "Permissive" ]; then
        check_warn "SELinux is permissive - consider enforcing mode"
    else
        check_warn "SELinux is disabled"
    fi
elif command -v aa-status &> /dev/null; then
    if aa-status --enabled 2>/dev/null; then
        check_pass "AppArmor is enabled"
    else
        check_warn "AppArmor is disabled"
    fi
else
    check_warn "No MAC system (SELinux/AppArmor) detected"
fi

# 10. Security Updates
echo ""
echo "--- 10. System Security ---"
if [ -f "/var/run/reboot-required" ]; then
    check_warn "System reboot required (pending updates)"
fi

# Summary
echo ""
echo "========================================"
echo "Security Check Summary:"
echo "  Passed: $CHECKS_PASSED"
echo "  Failed: $CHECKS_FAILED"
echo "========================================"

if [ $CHECKS_FAILED -eq 0 ]; then
    echo -e "${GREEN}All critical security checks passed${NC}"
    exit 0
else
    echo -e "${RED}Security issues detected - resolve before production deployment${NC}"
    exit 1
fi
```

**Checklist Summary:**

| Category | Requirement | Severity | Status |
|----------|-------------|----------|--------|
| **TLS/SSL** | Valid certificates installed | 🔴 Critical | ⬜ |
| **TLS/SSL** | Private key permissions (600) | 🔴 Critical | ⬜ |
| **Authentication** | Authentication enabled | 🔴 Critical | ⬜ |
| **Authorization** | RBAC configured | 🟡 High | ⬜ |
| **Encryption** | At-rest encryption enabled | 🔴 Critical | ⬜ |
| **Encryption** | Master key secured (600) | 🔴 Critical | ⬜ |
| **Audit** | Audit logging enabled | 🔴 Critical | ⬜ |
| **Audit** | Log retention policy configured | 🟡 High | ⬜ |
| **Network** | Firewall rules configured | 🔴 Critical | ⬜ |
| **Network** | Rate limiting enabled | 🟡 High | ⬜ |
| **Credentials** | Default passwords changed | 🔴 Critical | ⬜ |
| **Backup** | Backup encryption configured | 🟡 High | ⬜ |
| **System** | MAC system configured | 🟢 Medium | ⬜ |
| **System** | Security updates applied | 🟡 High | ⬜ |

---

## TLS/mTLS Configuration

### Certificate Generation

**Step 1: Create Certificate Authority (CA)**

```bash
#!/bin/bash
# generate_ca.sh - Create internal CA for ThemisDB

# Configuration
CA_DIR="/opt/themisdb/ca"
CERTS_DIR="/opt/themisdb/certs"
KEYS_DIR="/opt/themisdb/keys"
CA_VALIDITY_DAYS=3650  # 10 years
CERT_VALIDITY_DAYS=730  # 2 years

# Create directories
mkdir -p $CA_DIR $CERTS_DIR $KEYS_DIR
chmod 700 $CA_DIR $KEYS_DIR
chmod 755 $CERTS_DIR

# Generate CA private key (4096-bit RSA)
openssl genrsa -aes256 -out $CA_DIR/ca-key.pem 4096
chmod 400 $CA_DIR/ca-key.pem

# Generate CA certificate
openssl req -new -x509 -days $CA_VALIDITY_DAYS \
  -key $CA_DIR/ca-key.pem \
  -sha256 \
  -out $CA_DIR/ca-cert.pem \
  -subj "/C=US/ST=California/L=San Francisco/O=YourOrg/OU=Database/CN=ThemisDB CA"

chmod 444 $CA_DIR/ca-cert.pem

echo "CA created successfully"
echo "CA Certificate: $CA_DIR/ca-cert.pem"
echo "CA Key: $CA_DIR/ca-key.pem (keep secure!)"
```

**Step 2: Generate Server Certificate**

```bash
#!/bin/bash
# generate_server_cert.sh - Create server certificate

SERVER_NAME="themisdb-server-01"
SERVER_IP="192.168.1.10"
SERVER_DNS="themisdb.example.com"

# Generate server private key
openssl genrsa -out $KEYS_DIR/server-key.pem 4096
chmod 400 $KEYS_DIR/server-key.pem

# Create certificate signing request (CSR)
openssl req -new \
  -key $KEYS_DIR/server-key.pem \
  -out $CERTS_DIR/server.csr \
  -subj "/C=US/ST=California/L=San Francisco/O=YourOrg/OU=Database/CN=$SERVER_DNS"

# Create SAN (Subject Alternative Names) configuration
cat > $CERTS_DIR/server-san.cnf <<EOL
[req]
distinguished_name = req_distinguished_name
req_extensions = v3_req

[req_distinguished_name]

[v3_req]
basicConstraints = CA:FALSE
keyUsage = nonRepudiation, digitalSignature, keyEncipherment
extendedKeyUsage = serverAuth
subjectAltName = @alt_names

[alt_names]
DNS.1 = $SERVER_DNS
DNS.2 = localhost
IP.1 = $SERVER_IP
IP.2 = 127.0.0.1
EOL

# Sign server certificate with CA
openssl x509 -req \
  -in $CERTS_DIR/server.csr \
  -CA $CA_DIR/ca-cert.pem \
  -CAkey $CA_DIR/ca-key.pem \
  -CAcreateserial \
  -out $CERTS_DIR/server-cert.pem \
  -days $CERT_VALIDITY_DAYS \
  -sha256 \
  -extfile $CERTS_DIR/server-san.cnf \
  -extensions v3_req

chmod 444 $CERTS_DIR/server-cert.pem

# Verify certificate
openssl x509 -in $CERTS_DIR/server-cert.pem -text -noout | grep -A1 "Subject Alternative Name"

echo "Server certificate created successfully"
echo "Certificate: $CERTS_DIR/server-cert.pem"
echo "Key: $KEYS_DIR/server-key.pem"
```

**Step 3: Generate Client Certificates (for mTLS)**

```bash
#!/bin/bash
# generate_client_cert.sh - Create client certificate for mutual TLS

CLIENT_NAME="$1"
if [ -z "$CLIENT_NAME" ]; then
    echo "Usage: $0 <client-name>"
    exit 1
fi

CLIENT_DIR="/opt/themisdb/clients/$CLIENT_NAME"
mkdir -p $CLIENT_DIR
chmod 700 $CLIENT_DIR

# Generate client private key
openssl genrsa -out $CLIENT_DIR/client-key.pem 4096
chmod 400 $CLIENT_DIR/client-key.pem

# Create CSR
openssl req -new \
  -key $CLIENT_DIR/client-key.pem \
  -out $CLIENT_DIR/client.csr \
  -subj "/C=US/ST=California/L=San Francisco/O=YourOrg/OU=Clients/CN=$CLIENT_NAME"

# Create client certificate extension
cat > $CLIENT_DIR/client-ext.cnf <<EOL
basicConstraints = CA:FALSE
keyUsage = nonRepudiation, digitalSignature, keyEncipherment
extendedKeyUsage = clientAuth
EOL

# Sign client certificate
openssl x509 -req \
  -in $CLIENT_DIR/client.csr \
  -CA $CA_DIR/ca-cert.pem \
  -CAkey $CA_DIR/ca-key.pem \
  -CAcreateserial \
  -out $CLIENT_DIR/client-cert.pem \
  -days $CERT_VALIDITY_DAYS \
  -sha256 \
  -extfile $CLIENT_DIR/client-ext.cnf

chmod 444 $CLIENT_DIR/client-cert.pem

# Create client bundle
cat $CLIENT_DIR/client-cert.pem $CA_DIR/ca-cert.pem > $CLIENT_DIR/client-bundle.pem

echo "Client certificate created successfully for: $CLIENT_NAME"
echo "Certificate: $CLIENT_DIR/client-cert.pem"
echo "Key: $CLIENT_DIR/client-key.pem"
echo "Bundle: $CLIENT_DIR/client-bundle.pem"
```

### Server TLS Configuration

**Basic TLS Configuration:**

```yaml
# /opt/themisdb/config/themisdb.yaml
server:
  bind_address: "0.0.0.0"
  port: 7700
  
  # TLS Configuration
  tls:
    enabled: true
    cert_file: "/opt/themisdb/certs/server-cert.pem"
    key_file: "/opt/themisdb/keys/server-key.pem"
    ca_file: "/opt/themisdb/ca/ca-cert.pem"
    
    # TLS Version
    min_version: "TLS1.2"  # Minimum TLS 1.2
    max_version: "TLS1.3"  # Prefer TLS 1.3
    
    # Cipher Suites (TLS 1.2)
    cipher_suites:
      - "TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384"
      - "TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256"
      - "TLS_ECDHE_RSA_WITH_CHACHA20_POLY1305"
    
    # TLS 1.3 Cipher Suites (automatically used when TLS 1.3 is negotiated)
    # TLS_AES_128_GCM_SHA256
    # TLS_AES_256_GCM_SHA384
    # TLS_CHACHA20_POLY1305_SHA256
    
    # Certificate verification
    verify_client: false  # Set to true for mTLS
    
    # OCSP stapling (optional)
    ocsp_stapling: false
```

**Mutual TLS (mTLS) Configuration:**

```yaml
# /opt/themisdb/config/themisdb.yaml - Mutual TLS
server:
  tls:
    enabled: true
    cert_file: "/opt/themisdb/certs/server-cert.pem"
    key_file: "/opt/themisdb/keys/server-key.pem"
    ca_file: "/opt/themisdb/ca/ca-cert.pem"
    
    # Enable client certificate verification
    verify_client: true
    
    # Client certificate validation
    client_auth:
      mode: "require"  # Options: require, request, none
      
      # Trusted client CAs
      trusted_ca_file: "/opt/themisdb/ca/ca-cert.pem"
      
      # Certificate Revocation List (optional)
      crl_file: "/opt/themisdb/ca/crl.pem"
      
      # Client certificate mapping to users
      certificate_mapping:
        enabled: true
        # Extract username from certificate CN or email
        attribute: "CN"  # Options: CN, email, SAN
```

**TLS Testing and Verification:**

```bash
#!/bin/bash
# test_tls.sh - Verify TLS configuration

SERVER_HOST="themisdb.example.com"
SERVER_PORT="7700"

echo "=== Testing ThemisDB TLS Configuration ==="

# 1. Test basic TLS connection
echo ""
echo "1. Testing TLS handshake..."
openssl s_client -connect $SERVER_HOST:$SERVER_PORT \
  -CAfile /opt/themisdb/ca/ca-cert.pem \
  -showcerts </dev/null 2>&1 | grep -E "Verify return code|Protocol|Cipher"

# 2. Check certificate validity
echo ""
echo "2. Checking certificate..."
echo | openssl s_client -connect $SERVER_HOST:$SERVER_PORT 2>/dev/null | \
  openssl x509 -noout -dates -subject -issuer

# 3. Test supported protocols
echo ""
echo "3. Testing TLS versions..."
for version in tls1 tls1_1 tls1_2 tls1_3; do
    echo -n "  $version: "
    if openssl s_client -connect $SERVER_HOST:$SERVER_PORT -$version \
       -CAfile /opt/themisdb/ca/ca-cert.pem </dev/null 2>&1 | grep -q "Cipher is"; then
        echo "✓ Supported"
    else
        echo "✗ Not supported"
    fi
done

# 4. Test mTLS (if enabled)
echo ""
echo "4. Testing mutual TLS..."
openssl s_client -connect $SERVER_HOST:$SERVER_PORT \
  -CAfile /opt/themisdb/ca/ca-cert.pem \
  -cert /opt/themisdb/clients/client1/client-cert.pem \
  -key /opt/themisdb/clients/client1/client-key.pem \
  -showcerts </dev/null 2>&1 | grep "Verify return code"

# 5. Security scan with testssl.sh (if available)
if command -v testssl.sh &> /dev/null; then
    echo ""
    echo "5. Running comprehensive TLS security scan..."
    testssl.sh --fast $SERVER_HOST:$SERVER_PORT
fi
```

### Client TLS Configuration

**C++ Client Configuration:**

```cpp
// themisdb_client_tls.cpp
#include "themisdb/client.hpp"
#include <iostream>

int main() {
    themisdb::ClientConfig config;
    
    // Server connection
    config.host = "themisdb.example.com";
    config.port = 7700;
    
    // Basic TLS configuration
    config.tls_enabled = true;
    config.tls_ca_file = "/opt/themisdb/ca/ca-cert.pem";
    config.tls_verify_server = true;
    
    // Optional: mTLS client certificate
    config.tls_cert_file = "/opt/themisdb/clients/app1/client-cert.pem";
    config.tls_key_file = "/opt/themisdb/clients/app1/client-key.pem";
    
    // TLS options
    config.tls_min_version = themisdb::TLSVersion::TLS12;
    config.tls_server_name = "themisdb.example.com";  // SNI
    
    // Create client
    auto client = themisdb::Client::create(config);
    
    // Test connection
    if (client->connect()) {
        std::cout << "✓ Connected securely via TLS" << std::endl;
        
        // Perform operations
        auto txn = client->begin_transaction();
        txn->put("key", "value");
        txn->commit();
        
    } else {
        std::cerr << "✗ Connection failed" << std::endl;
        return 1;
    }
    
    return 0;
}
```

**Python Client Configuration:**

```python
#!/usr/bin/env python3
# themisdb_client_tls.py

from themisdb import Client, ClientConfig, TLSConfig

# Configure TLS
tls_config = TLSConfig(
    enabled=True,
    ca_file="/opt/themisdb/ca/ca-cert.pem",
    verify_server=True,
    
    # Optional: mTLS
    cert_file="/opt/themisdb/clients/app1/client-cert.pem",
    key_file="/opt/themisdb/clients/app1/client-key.pem",
    
    # TLS version
    min_version="TLS1.2",
    server_name="themisdb.example.com"  # SNI
)

# Client configuration
config = ClientConfig(
    host="themisdb.example.com",
    port=7700,
    tls=tls_config,
    
    # Connection pooling
    pool_size=10,
    connection_timeout=30
)

# Create client
client = Client(config)

try:
    # Connect
    client.connect()
    print("✓ Connected securely via TLS")
    
    # Perform transaction
    with client.transaction() as txn:
        txn.put("user:1", {"name": "Alice", "email": "alice@example.com"})
        txn.commit()
        
except Exception as e:
    print(f"✗ Error: {e}")
finally:
    client.close()
```

### Certificate Rotation

**Automated Certificate Rotation Script:**

```bash
#!/bin/bash
# rotate_certificates.sh - Zero-downtime certificate rotation

set -e

CERTS_DIR="/opt/themisdb/certs"
KEYS_DIR="/opt/themisdb/keys"
BACKUP_DIR="/opt/themisdb/certs/backup"
CA_DIR="/opt/themisdb/ca"
CERT_VALIDITY_DAYS=730

echo "=== ThemisDB Certificate Rotation ==="
echo "Date: $(date)"

# Create backup directory
mkdir -p $BACKUP_DIR
chmod 700 $BACKUP_DIR

# Step 1: Backup current certificates
echo ""
echo "Step 1: Backing up current certificates..."
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
cp $CERTS_DIR/server-cert.pem $BACKUP_DIR/server-cert.pem.$TIMESTAMP
cp $KEYS_DIR/server-key.pem $BACKUP_DIR/server-key.pem.$TIMESTAMP
echo "  ✓ Backup complete: $BACKUP_DIR/*.$TIMESTAMP"

# Step 2: Generate new certificates
echo ""
echo "Step 2: Generating new certificates..."

# Generate new private key
openssl genrsa -out $KEYS_DIR/server-key.pem.new 4096
chmod 400 $KEYS_DIR/server-key.pem.new

# Create CSR with same details
openssl req -new \
  -key $KEYS_DIR/server-key.pem.new \
  -out $CERTS_DIR/server.csr.new \
  -subj "$(openssl x509 -in $CERTS_DIR/server-cert.pem -noout -subject | sed 's/^subject=//')"

# Get SAN from old certificate
openssl x509 -in $CERTS_DIR/server-cert.pem -noout -text | \
  grep -A1 "Subject Alternative Name" | tail -1 | \
  sed 's/^[[:space:]]*//' > $CERTS_DIR/san_list.txt

# Create new SAN config
cat > $CERTS_DIR/server-san.cnf.new <<EOL
[req]
req_extensions = v3_req

[v3_req]
basicConstraints = CA:FALSE
keyUsage = nonRepudiation, digitalSignature, keyEncipherment
extendedKeyUsage = serverAuth
subjectAltName = @alt_names

[alt_names]
$(cat $CERTS_DIR/san_list.txt)
EOL

# Sign new certificate
openssl x509 -req \
  -in $CERTS_DIR/server.csr.new \
  -CA $CA_DIR/ca-cert.pem \
  -CAkey $CA_DIR/ca-key.pem \
  -CAcreateserial \
  -out $CERTS_DIR/server-cert.pem.new \
  -days $CERT_VALIDITY_DAYS \
  -sha256 \
  -extfile $CERTS_DIR/server-san.cnf.new \
  -extensions v3_req

chmod 444 $CERTS_DIR/server-cert.pem.new

echo "  ✓ New certificates generated"

# Step 3: Verify new certificates
echo ""
echo "Step 3: Verifying new certificates..."
openssl verify -CAfile $CA_DIR/ca-cert.pem $CERTS_DIR/server-cert.pem.new
openssl x509 -in $CERTS_DIR/server-cert.pem.new -noout -dates

# Step 4: Install new certificates (atomic operation)
echo ""
echo "Step 4: Installing new certificates..."
mv $KEYS_DIR/server-key.pem.new $KEYS_DIR/server-key.pem
mv $CERTS_DIR/server-cert.pem.new $CERTS_DIR/server-cert.pem
echo "  ✓ New certificates installed"

# Step 5: Reload ThemisDB configuration
echo ""
echo "Step 5: Reloading ThemisDB configuration..."
if systemctl is-active --quiet themisdb; then
    # Send HUP signal to reload TLS configuration without downtime
    systemctl reload themisdb || systemctl restart themisdb
    echo "  ✓ ThemisDB reloaded"
else
    echo "  ⚠ ThemisDB not running"
fi

# Step 6: Test new connection
echo ""
echo "Step 6: Testing TLS connection..."
sleep 5  # Wait for reload
if openssl s_client -connect localhost:7700 \
   -CAfile $CA_DIR/ca-cert.pem </dev/null 2>&1 | grep -q "Verify return code: 0"; then
    echo "  ✓ TLS connection successful with new certificate"
else
    echo "  ✗ TLS connection failed - rolling back"
    cp $BACKUP_DIR/server-cert.pem.$TIMESTAMP $CERTS_DIR/server-cert.pem
    cp $BACKUP_DIR/server-key.pem.$TIMESTAMP $KEYS_DIR/server-key.pem
    systemctl reload themisdb
    exit 1
fi

# Step 7: Cleanup
rm -f $CERTS_DIR/server.csr.new $CERTS_DIR/server-san.cnf.new $CERTS_DIR/san_list.txt

echo ""
echo "=== Certificate rotation complete ==="
echo "Old certificates backed up to: $BACKUP_DIR/*.$TIMESTAMP"
echo "New certificate expires: $(openssl x509 -in $CERTS_DIR/server-cert.pem -noout -enddate)"
```

**Automated Rotation with Cron:**

```bash
# Add to /etc/cron.d/themisdb-cert-rotation
# Check certificate expiry daily and rotate if needed

0 2 * * * root /opt/themisdb/scripts/check_cert_expiry.sh && \
  [ $? -eq 1 ] && /opt/themisdb/scripts/rotate_certificates.sh || true
```

**Certificate Expiry Check Script:**

```bash
#!/bin/bash
# check_cert_expiry.sh - Check if certificate rotation is needed

CERT_FILE="/opt/themisdb/certs/server-cert.pem"
ROTATION_THRESHOLD_DAYS=30  # Rotate when less than 30 days remaining

EXPIRY_DATE=$(openssl x509 -enddate -noout -in $CERT_FILE | cut -d= -f2)
EXPIRY_EPOCH=$(date -d "$EXPIRY_DATE" +%s)
NOW_EPOCH=$(date +%s)
DAYS_REMAINING=$(( ($EXPIRY_EPOCH - $NOW_EPOCH) / 86400 ))

echo "Certificate expires in $DAYS_REMAINING days"

if [ $DAYS_REMAINING -lt $ROTATION_THRESHOLD_DAYS ]; then
    echo "Certificate rotation needed (threshold: $ROTATION_THRESHOLD_DAYS days)"
    exit 1  # Trigger rotation
else
    echo "Certificate valid"
    exit 0
fi
```

---

## Access Control Implementation

### Role-Based Access Control (RBAC)

**RBAC Configuration:**

```yaml
# /opt/themisdb/config/rbac.yaml
rbac:
  enabled: true
  
  # Built-in roles
  roles:
    # Administrative role - full access
    - name: "admin"
      description: "Full administrative access"
      permissions:
        - "system:*"
        - "database:*"
        - "table:*"
        - "user:*"
        - "role:*"
      
    # Database administrator - database operations
    - name: "dba"
      description: "Database administration"
      permissions:
        - "database:create"
        - "database:drop"
        - "database:alter"
        - "table:*"
        - "index:*"
        - "backup:*"
        - "monitoring:read"
      
    # Developer role - read/write data
    - name: "developer"
      description: "Application development access"
      permissions:
        - "database:read"
        - "table:read"
        - "table:write"
        - "transaction:execute"
        - "query:execute"
      restrictions:
        - "!table:drop"
        - "!database:drop"
      
    # Read-only analyst role
    - name: "analyst"
      description: "Read-only access for analytics"
      permissions:
        - "database:read"
        - "table:read"
        - "query:execute"
      restrictions:
        - "!table:write"
        - "!transaction:write"
      
    # Application service role
    - name: "app_service"
      description: "Service account for applications"
      permissions:
        - "table:read"
        - "table:write"
        - "transaction:execute"
      resources:
        databases:
          - "production_db"
          - "staging_db"
        tables:
          - "production_db.users"
          - "production_db.orders"
          - "production_db.sessions"
      rate_limit:
        max_queries_per_second: 1000
        max_concurrent_transactions: 50
  
  # Custom roles
  custom_roles:
    - name: "finance_admin"
      description: "Finance data administrator"
      inherits:
        - "developer"
      permissions:
        - "table:drop"  # Additional permission
      resources:
        databases:
          - "finance_db"
      
    - name: "auditor"
      description: "Audit log reader"
      permissions:
        - "audit:read"
        - "monitoring:read"
      restrictions:
        - "!table:*"
  
  # Permission inheritance
  inheritance_enabled: true
  max_inheritance_depth: 3
```

### User Management

**User Configuration:**

```yaml
# /opt/themisdb/config/users.yaml
users:
  # Administrative users
  - username: "admin"
    password_hash: "$2b$12$LQv3c1yqBWVHxkd0LHAkCOYz6TtxMQJqhN8/LewY5eBk8KVzpB3iK"
    roles:
      - "admin"
    mfa_enabled: true
    ip_whitelist:
      - "10.0.0.0/8"
      - "172.16.0.0/12"
    
  - username: "dba_user"
    password_hash: "$2b$12$XxvtGf0ZMjKLkBpH8nRN2eBVrF9a7q8FzEpGYf2LmVnPqW4xTz8Se"
    roles:
      - "dba"
    mfa_enabled: true
    
  # Application users
  - username: "app_service_prod"
    password_hash: "$2b$12$YzQwNmI1NDU5ZTIwMzU2Y2FkYjM1NzA1NzYzYTQ2MTJiYzU3ODk"
    roles:
      - "app_service"
    certificate_dn: "CN=app_service_prod,OU=Services,O=YourOrg"
    max_connections: 100
    connection_timeout: 300
    
  # Developer users
  - username: "developer1"
    password_hash: "$2b$12$NzQ5MTIwMzQ1Njc4OTAxMmNkZWY0NTY3ODkwMTIzNDU2Nzg5MGFi"
    roles:
      - "developer"
    allowed_databases:
      - "development_db"
      - "staging_db"
    
  # Analyst users
  - username: "analyst1"
    password_hash: "$2b$12$MGExMjM0NTY3ODkwMTIzNDU2Nzg5MDEyMzQ1Njc4OTAxMjM0NTY"
    roles:
      - "analyst"
    query_timeout: 300  # 5 minutes
    max_query_memory_mb: 4096

# Password policy
password_policy:
  min_length: 12
  require_uppercase: true
  require_lowercase: true
  require_numbers: true
  require_special_chars: true
  expiration_days: 90
  prevent_reuse: 5  # Last 5 passwords
  
# Account lockout policy
lockout_policy:
  enabled: true
  failed_attempts: 5
  lockout_duration_minutes: 30
  reset_after_minutes: 15
```

**User Management Commands:**

```bash
#!/bin/bash
# User management commands

# Create new user
themisdb-admin user create \
  --username="newuser" \
  --password="SecureP@ssw0rd" \
  --roles="developer" \
  --email="newuser@example.com"

# Update user roles
themisdb-admin user update \
  --username="newuser" \
  --add-role="analyst"

# Change password
themisdb-admin user change-password \
  --username="newuser" \
  --new-password="NewSecureP@ssw0rd"

# Enable/disable user
themisdb-admin user disable --username="newuser"
themisdb-admin user enable --username="newuser"

# Delete user
themisdb-admin user delete --username="newuser" --confirm

# List users
themisdb-admin user list --format=table

# View user details
themisdb-admin user show --username="newuser"
```

**Password Hashing Script:**

```python
#!/usr/bin/env python3
# generate_password_hash.py

import bcrypt
import sys
import getpass

def generate_hash(password):
    # Generate bcrypt hash with cost factor 12
    salt = bcrypt.gensalt(rounds=12)
    hashed = bcrypt.hashpw(password.encode('utf-8'), salt)
    return hashed.decode('utf-8')

if __name__ == "__main__":
    if len(sys.argv) > 1:
        password = sys.argv[1]
    else:
        password = getpass.getpass("Enter password: ")
        verify = getpass.getpass("Verify password: ")
        
        if password != verify:
            print("Passwords do not match!", file=sys.stderr)
            sys.exit(1)
    
    hashed = generate_hash(password)
    print(f"Password hash: {hashed}")
```

### JWT Authentication

**JWT Configuration:**

```yaml
# /opt/themisdb/config/jwt.yaml
jwt:
  enabled: true
  
  # JWT signing configuration
  signing:
    algorithm: "RS256"  # RSA with SHA-256
    private_key_file: "/opt/themisdb/keys/jwt-private.pem"
    public_key_file: "/opt/themisdb/keys/jwt-public.pem"
    
    # Alternative: HMAC with shared secret
    # algorithm: "HS256"
    # secret_key: "your-256-bit-secret-key-here"
  
  # Token settings
  tokens:
    access_token:
      expiry_seconds: 900  # 15 minutes
      renewable: true
    
    refresh_token:
      expiry_seconds: 604800  # 7 days
      renewable: false
      rotation_enabled: true  # Issue new refresh token on use
    
  # Claims configuration
  claims:
    issuer: "themisdb.example.com"
    audience: "themisdb-clients"
    include_roles: true
    include_permissions: true
    custom_claims:
      - "user_id"
      - "session_id"
  
  # Validation
  validation:
    verify_expiry: true
    verify_issuer: true
    verify_audience: true
    clock_skew_seconds: 60  # Allow 60s clock skew
    
  # Token revocation
  revocation:
    enabled: true
    check_on_validation: true
    # Redis-based revocation list
    redis:
      host: "localhost"
      port: 6379
      db: 0
      key_prefix: "themisdb:jwt:revoked:"
```

**JWT Key Generation:**

```bash
#!/bin/bash
# generate_jwt_keys.sh - Generate RSA key pair for JWT signing

JWT_KEYS_DIR="/opt/themisdb/keys"
mkdir -p $JWT_KEYS_DIR
chmod 700 $JWT_KEYS_DIR

# Generate private key (4096-bit RSA)
openssl genrsa -out $JWT_KEYS_DIR/jwt-private.pem 4096
chmod 400 $JWT_KEYS_DIR/jwt-private.pem

# Generate public key
openssl rsa -in $JWT_KEYS_DIR/jwt-private.pem \
  -pubout -out $JWT_KEYS_DIR/jwt-public.pem
chmod 444 $JWT_KEYS_DIR/jwt-public.pem

echo "JWT keys generated:"
echo "  Private key: $JWT_KEYS_DIR/jwt-private.pem"
echo "  Public key: $JWT_KEYS_DIR/jwt-public.pem"
```

**JWT Client Implementation:**

```cpp
// jwt_client.cpp - JWT authentication example
#include "themisdb/client.hpp"
#include <jwt-cpp/jwt.h>
#include <iostream>

class JWTAuthClient {
private:
    std::string access_token_;
    std::string refresh_token_;
    themisdb::Client* client_;
    
public:
    JWTAuthClient(const std::string& host, int port) {
        themisdb::ClientConfig config;
        config.host = host;
        config.port = port;
        config.tls_enabled = true;
        client_ = themisdb::Client::create(config);
    }
    
    // Authenticate and get JWT tokens
    bool authenticate(const std::string& username, 
                     const std::string& password) {
        auto response = client_->authenticate(username, password);
        
        if (response.success) {
            access_token_ = response.access_token;
            refresh_token_ = response.refresh_token;
            
            // Set token for future requests
            client_->set_auth_token(access_token_);
            
            return true;
        }
        
        return false;
    }
    
    // Refresh access token using refresh token
    bool refresh_access_token() {
        auto response = client_->refresh_token(refresh_token_);
        
        if (response.success) {
            access_token_ = response.access_token;
            
            // Update client token
            client_->set_auth_token(access_token_);
            
            // Update refresh token if rotated
            if (!response.refresh_token.empty()) {
                refresh_token_ = response.refresh_token;
            }
            
            return true;
        }
        
        return false;
    }
    
    // Perform database operation with automatic token refresh
    bool execute_with_retry(std::function<void()> operation) {
        try {
            operation();
            return true;
        } catch (const themisdb::TokenExpiredException& e) {
            // Token expired, try to refresh
            if (refresh_access_token()) {
                // Retry operation with new token
                try {
                    operation();
                    return true;
                } catch (const std::exception& e) {
                    std::cerr << "Operation failed after token refresh: " 
                             << e.what() << std::endl;
                    return false;
                }
            }
            return false;
        } catch (const std::exception& e) {
            std::cerr << "Operation failed: " << e.what() << std::endl;
            return false;
        }
    }
};

int main() {
    JWTAuthClient client("themisdb.example.com", 7700);
    
    // Authenticate
    if (!client.authenticate("app_service", "SecurePassword123!")) {
        std::cerr << "Authentication failed" << std::endl;
        return 1;
    }
    
    std::cout << "✓ Authentication successful" << std::endl;
    
    // Perform operations with automatic token refresh
    client.execute_with_retry([]() {
        auto txn = client_->begin_transaction();
        txn->put("key", "value");
        txn->commit();
    });
    
    return 0;
}
```

**JWT Validation Middleware:**

```python
#!/usr/bin/env python3
# jwt_middleware.py - JWT validation for web applications

import jwt
import time
from functools import wraps
from flask import request, jsonify

class JWTValidator:
    def __init__(self, public_key_path, issuer, audience):
        with open(public_key_path, 'r') as f:
            self.public_key = f.read()
        self.issuer = issuer
        self.audience = audience
    
    def validate_token(self, token):
        try:
            payload = jwt.decode(
                token,
                self.public_key,
                algorithms=['RS256'],
                issuer=self.issuer,
                audience=self.audience,
                options={
                    'verify_exp': True,
                    'verify_iss': True,
                    'verify_aud': True,
                }
            )
            return payload
        except jwt.ExpiredSignatureError:
            raise Exception("Token expired")
        except jwt.InvalidTokenError as e:
            raise Exception(f"Invalid token: {e}")
    
    def require_auth(self, required_roles=None):
        def decorator(f):
            @wraps(f)
            def decorated_function(*args, **kwargs):
                # Extract token from Authorization header
                auth_header = request.headers.get('Authorization')
                if not auth_header or not auth_header.startswith('Bearer '):
                    return jsonify({'error': 'Missing or invalid authorization header'}), 401
                
                token = auth_header.split(' ')[1]
                
                try:
                    # Validate token
                    payload = self.validate_token(token)
                    
                    # Check roles if required
                    if required_roles:
                        user_roles = payload.get('roles', [])
                        if not any(role in user_roles for role in required_roles):
                            return jsonify({'error': 'Insufficient permissions'}), 403
                    
                    # Add payload to request context
                    request.jwt_payload = payload
                    
                    return f(*args, **kwargs)
                    
                except Exception as e:
                    return jsonify({'error': str(e)}), 401
            
            return decorated_function
        return decorator

# Usage example
validator = JWTValidator(
    public_key_path='/opt/themisdb/keys/jwt-public.pem',
    issuer='themisdb.example.com',
    audience='themisdb-clients'
)

@app.route('/api/data', methods=['GET'])
@validator.require_auth(required_roles=['developer', 'analyst'])
def get_data():
    user_id = request.jwt_payload['user_id']
    # Perform operation
    return jsonify({'data': 'value'})
```

---

## Encryption Configuration

### Encryption at Rest

**Master Key Generation:**

```bash
#!/bin/bash
# generate_master_key.sh - Generate encryption master key

KEYS_DIR="/opt/themisdb/keys"
mkdir -p $KEYS_DIR
chmod 700 $KEYS_DIR

# Generate 256-bit AES master key
openssl rand -base64 32 > $KEYS_DIR/master.key
chmod 400 $KEYS_DIR/master.key

echo "Master key generated: $KEYS_DIR/master.key"
echo "⚠️  IMPORTANT: Back up this key securely!"
echo "⚠️  Store backup in secure location (HSM, vault, encrypted backup)"

# Optionally encrypt master key with passphrase
read -sp "Enter passphrase to encrypt master key (or press Enter to skip): " PASSPHRASE
echo

if [ ! -z "$PASSPHRASE" ]; then
    openssl enc -aes-256-cbc -salt \
      -in $KEYS_DIR/master.key \
      -out $KEYS_DIR/master.key.enc \
      -pass pass:"$PASSPHRASE"
    
    chmod 400 $KEYS_DIR/master.key.enc
    echo "Encrypted master key: $KEYS_DIR/master.key.enc"
fi
```

**Encryption at Rest Configuration:**

```yaml
# /opt/themisdb/config/encryption.yaml
encryption:
  at_rest:
    enabled: true
    
    # Encryption algorithm
    algorithm: "AES-256-GCM"  # AES-256 in GCM mode
    
    # Master key configuration
    master_key:
      provider: "file"  # Options: file, env, kms, vault
      file_path: "/opt/themisdb/keys/master.key"
      # For encrypted master key
      encrypted: false
      # passphrase_env_var: "THEMISDB_MASTER_KEY_PASSPHRASE"
    
    # Key derivation
    key_derivation:
      function: "PBKDF2"
      iterations: 100000
      salt_size_bytes: 32
    
    # Data encryption keys (DEK) rotation
    dek_rotation:
      enabled: true
      rotation_period_days: 90
      auto_rotate: true
    
    # What to encrypt
    encrypt_data_blocks: true
    encrypt_wal: true
    encrypt_backups: true
    encrypt_temp_files: true
    
    # Performance settings
    encryption_buffer_size_kb: 64
    parallel_encryption: true
    hardware_acceleration: true  # Use AES-NI if available

# Alternative: AWS KMS integration
encryption_kms:
  at_rest:
    enabled: true
    master_key:
      provider: "aws-kms"
      kms_key_id: "arn:aws:kms:us-east-1:123456789012:key/12345678-1234-1234-1234-123456789012"
      region: "us-east-1"
    # DEK cached in memory for performance
    dek_cache:
      enabled: true
      cache_size: 100
      ttl_seconds: 3600

# Alternative: HashiCorp Vault integration
encryption_vault:
  at_rest:
    enabled: true
    master_key:
      provider: "vault"
      vault_address: "https://vault.example.com:8200"
      vault_token_file: "/opt/themisdb/secrets/vault-token"
      vault_mount: "transit"
      key_name: "themisdb-master-key"
```

**Encryption Performance Test:**

```bash
#!/bin/bash
# test_encryption_performance.sh

echo "=== Encryption Performance Test ==="

# Test with encryption disabled
themisdb-admin config set encryption.at_rest.enabled false
themisdb-admin service restart

echo "Testing without encryption..."
themisdb-bench write --count=1000000 --threads=4

# Test with encryption enabled
themisdb-admin config set encryption.at_rest.enabled true
themisdb-admin service restart

echo "Testing with encryption..."
themisdb-bench write --count=1000000 --threads=4

# Test with hardware acceleration
echo "Testing with AES-NI hardware acceleration..."
themisdb-admin config set encryption.at_rest.hardware_acceleration true
themisdb-admin service restart
themisdb-bench write --count=1000000 --threads=4
```

### Field-Level Encryption

**Field-Level Encryption Configuration:**

```yaml
# /opt/themisdb/config/field_encryption.yaml
field_encryption:
  enabled: true
  
  # Encryption keys per field
  keys:
    - name: "pii_key"
      algorithm: "AES-256-GCM"
      key_id: "pii_key_v1"
      key_file: "/opt/themisdb/keys/pii.key"
      
    - name: "financial_key"
      algorithm: "AES-256-GCM"
      key_id: "financial_key_v1"
      key_file: "/opt/themisdb/keys/financial.key"
  
  # Encrypted fields mapping
  encrypted_fields:
    - database: "users_db"
      table: "users"
      fields:
        - name: "ssn"
          encryption_key: "pii_key"
          searchable: false
        - name: "email"
          encryption_key: "pii_key"
          searchable: true  # Use deterministic encryption
          deterministic: true
        - name: "credit_card"
          encryption_key: "financial_key"
          searchable: false
          masked: true  # Return masked value on read
          mask_format: "XXXX-XXXX-XXXX-1234"
    
    - database: "finance_db"
      table: "transactions"
      fields:
        - name: "amount"
          encryption_key: "financial_key"
          preserve_format: true  # Format-preserving encryption
```

**Field-Level Encryption Client Usage:**

```cpp
// field_encryption_example.cpp
#include "themisdb/client.hpp"
#include "themisdb/encryption.hpp"

// Insert with automatic field encryption
void insert_user(themisdb::Client* client) {
    auto txn = client->begin_transaction();
    
    // Fields marked for encryption are automatically encrypted
    themisdb::Document user{
        {"user_id", "12345"},
        {"name", "John Doe"},
        {"email", "john@example.com"},  // Encrypted with pii_key
        {"ssn", "123-45-6789"},         // Encrypted with pii_key
        {"credit_card", "4111-1111-1111-1111"}  // Encrypted with financial_key
    };
    
    txn->put("users_db.users:12345", user);
    txn->commit();
}

// Query with encrypted field (deterministic encryption)
void search_by_email(themisdb::Client* client, const std::string& email) {
    auto txn = client->begin_transaction();
    
    // Email is deterministically encrypted, so equality search works
    auto query = themisdb::Query()
        .from("users_db.users")
        .where("email", "=", email);  // Automatically encrypted for comparison
    
    auto results = txn->execute(query);
    
    // Results are automatically decrypted
    for (const auto& doc : results) {
        std::cout << "User: " << doc["name"].as_string() 
                  << ", Email: " << doc["email"].as_string() << std::endl;
        // SSN is masked
        std::cout << "SSN: " << doc["ssn"].as_string() << std::endl;  // XXX-XX-6789
    }
}
```

### Key Rotation

**Encryption Key Rotation Script:**

```bash
#!/bin/bash
# rotate_encryption_keys.sh - Rotate encryption keys

set -e

KEYS_DIR="/opt/themisdb/keys"
BACKUP_DIR="/opt/themisdb/keys/backup"

echo "=== ThemisDB Encryption Key Rotation ==="
echo "Date: $(date)"

# Backup current key
mkdir -p $BACKUP_DIR
chmod 700 $BACKUP_DIR
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
cp $KEYS_DIR/master.key $BACKUP_DIR/master.key.$TIMESTAMP
echo "✓ Current key backed up"

# Generate new master key
echo "Generating new master key..."
openssl rand -base64 32 > $KEYS_DIR/master.key.new
chmod 400 $KEYS_DIR/master.key.new

# Re-encrypt data with new key (online rotation)
echo "Starting online key rotation..."
themisdb-admin encryption rotate-key \
  --old-key=$KEYS_DIR/master.key \
  --new-key=$KEYS_DIR/master.key.new \
  --progress

# Atomic key swap
mv $KEYS_DIR/master.key.new $KEYS_DIR/master.key
echo "✓ Key rotation complete"

# Verify
themisdb-admin encryption verify-key
echo "✓ Key verified"

echo ""
echo "Old key backed up to: $BACKUP_DIR/master.key.$TIMESTAMP"
echo "⚠️  Keep old key for 90 days in case of rollback"
```

**Automated Key Rotation Policy:**

```yaml
# /opt/themisdb/config/key_rotation.yaml
key_rotation:
  enabled: true
  
  # Rotation schedule
  schedule:
    master_key:
      period_days: 365  # Rotate annually
      auto_rotate: true
      rotation_time: "02:00"  # 2 AM
    
    data_encryption_keys:
      period_days: 90  # Rotate quarterly
      auto_rotate: true
      batch_size: 100000  # Re-encrypt in batches
    
    field_encryption_keys:
      period_days: 180  # Rotate semi-annually
      auto_rotate: false  # Manual rotation
  
  # Rotation notifications
  notifications:
    - type: "email"
      recipients:
        - "security@example.com"
        - "dba@example.com"
      events:
        - "rotation_started"
        - "rotation_completed"
        - "rotation_failed"
    
    - type: "slack"
      webhook_url: "https://hooks.slack.com/services/XXX"
  
  # Backup policy
  backup:
    keep_old_keys: true
    retention_days: 90
    encrypted_backup: true
```


---

## Audit Logging

### Audit Log Configuration

**Comprehensive Audit Logging Setup:**

```yaml
# /opt/themisdb/config/audit.yaml
audit:
  enabled: true
  
  # Log destination
  output:
    - type: "file"
      path: "/var/log/themisdb/audit/audit.log"
      rotation:
        max_size_mb: 100
        max_age_days: 90
        max_backups: 100
        compress: true
    
    - type: "syslog"
      host: "syslog.example.com"
      port: 514
      protocol: "tcp"
      tls: true
    
    - type: "elasticsearch"
      hosts:
        - "https://es1.example.com:9200"
        - "https://es2.example.com:9200"
      index: "themisdb-audit"
      username: "audit_writer"
      password_file: "/opt/themisdb/secrets/es_password"
  
  # What to audit
  events:
    authentication:
      - "login_success"
      - "login_failure"
      - "logout"
      - "token_issued"
      - "token_refreshed"
      - "token_revoked"
    
    authorization:
      - "access_granted"
      - "access_denied"
      - "role_changed"
      - "permission_changed"
    
    data_access:
      - "read"
      - "write"
      - "delete"
      - "query_executed"
      - "transaction_started"
      - "transaction_committed"
      - "transaction_aborted"
    
    administrative:
      - "user_created"
      - "user_deleted"
      - "user_modified"
      - "role_created"
      - "role_deleted"
      - "config_changed"
      - "encryption_key_rotated"
      - "backup_created"
      - "backup_restored"
      - "system_started"
      - "system_stopped"
    
    security:
      - "certificate_rotated"
      - "tls_error"
      - "authentication_failure"
      - "brute_force_detected"
      - "suspicious_activity"
  
  # Filtering
  filters:
    # Exclude noisy events
    exclude_events:
      - "heartbeat"
      - "health_check"
    
    # Include only specific users
    include_users:
      - "admin"
      - "dba_*"  # Wildcard
    
    # Include only specific databases
    include_databases:
      - "production_db"
      - "finance_db"
  
  # Format
  format: "json"  # Options: json, cef, syslog
  
  # Additional metadata
  metadata:
    include_timestamp: true
    include_hostname: true
    include_ip_address: true
    include_session_id: true
    include_query_text: true
    include_query_plan: false
    include_execution_time: true
    
  # Performance
  async_logging: true
  buffer_size: 10000
  flush_interval_seconds: 5
```

**Audit Log Format:**

```json
{
  "timestamp": "2026-01-18T10:30:45.123Z",
  "event_id": "a7f3c9e1-8d5b-4a3f-9c2e-1b8d4f6a9c3e",
  "event_type": "data_access",
  "event_action": "query_executed",
  "event_result": "success",
  "user": {
    "username": "app_service_prod",
    "user_id": "12345",
    "roles": ["app_service"],
    "ip_address": "192.168.1.100",
    "session_id": "sess_abc123"
  },
  "resource": {
    "database": "production_db",
    "table": "users",
    "operation": "SELECT"
  },
  "query": {
    "text": "SELECT * FROM users WHERE user_id = ?",
    "parameters": ["redacted"],
    "execution_time_ms": 15,
    "rows_returned": 1
  },
  "metadata": {
    "hostname": "themisdb-server-01",
    "server_version": "1.4.0",
    "client_version": "1.4.0",
    "tls_version": "TLS1.3"
  }
}
```

### SIEM Integration

**Splunk Integration:**

```bash
#!/bin/bash
# splunk_forwarder_setup.sh - Configure Splunk Universal Forwarder

# Install Splunk Universal Forwarder
wget -O splunkforwarder.tgz \
  'https://download.splunk.com/products/universalforwarder/releases/9.0.0/linux/splunkforwarder-9.0.0-linux-x86_64.tgz'
  
tar xvzf splunkforwarder.tgz -C /opt
cd /opt/splunkforwarder

# Configure forwarder
./bin/splunk start --accept-license --answer-yes
./bin/splunk enable boot-start

# Add ThemisDB audit logs
./bin/splunk add monitor /var/log/themisdb/audit/ \
  -index themisdb \
  -sourcetype themisdb:audit

# Configure forwarding to Splunk indexer
./bin/splunk add forward-server splunk-indexer.example.com:9997 \
  -auth admin:changeme

# Create props.conf for parsing
cat > /opt/splunkforwarder/etc/system/local/props.conf <<EOF
[themisdb:audit]
SHOULD_LINEMERGE = false
TIME_PREFIX = "timestamp":"
TIME_FORMAT = %Y-%m-%dT%H:%M:%S.%3NZ
MAX_TIMESTAMP_LOOKAHEAD = 32
KV_MODE = json
TRUNCATE = 0
EOF

./bin/splunk restart
```

**ELK Stack Integration:**

```yaml
# /etc/filebeat/filebeat.yml - Filebeat configuration
filebeat.inputs:
  - type: log
    enabled: true
    paths:
      - /var/log/themisdb/audit/audit.log
    fields:
      app: themisdb
      environment: production
      log_type: audit
    json.keys_under_root: true
    json.add_error_key: true
    
  - type: log
    enabled: true
    paths:
      - /var/log/themisdb/themisdb.log
    fields:
      app: themisdb
      environment: production
      log_type: application
    multiline.pattern: '^[0-9]{4}-[0-9]{2}-[0-9]{2}'
    multiline.negate: true
    multiline.match: after

# Logstash output
output.logstash:
  hosts: ["logstash.example.com:5044"]
  ssl.certificate_authorities: ["/etc/pki/tls/certs/logstash.crt"]
  ssl.certificate: "/etc/pki/tls/certs/filebeat.crt"
  ssl.key: "/etc/pki/tls/private/filebeat.key"

# Processors
processors:
  - add_host_metadata:
      when.not.contains.tags: forwarded
  - add_cloud_metadata: ~
  - add_docker_metadata: ~
```

**Logstash Pipeline:**

```ruby
# /etc/logstash/conf.d/themisdb-audit.conf
input {
  beats {
    port => 5044
    ssl => true
    ssl_certificate => "/etc/pki/tls/certs/logstash.crt"
    ssl_key => "/etc/pki/tls/private/logstash.key"
  }
}

filter {
  if [fields][log_type] == "audit" {
    # Parse JSON audit logs
    json {
      source => "message"
    }
    
    # Extract user information
    if [user][username] {
      mutate {
        add_field => { "[@metadata][username]" => "%{[user][username]}" }
      }
    }
    
    # Enrich with GeoIP
    if [user][ip_address] {
      geoip {
        source => "[user][ip_address]"
        target => "[user][geo]"
      }
    }
    
    # Detect suspicious patterns
    if [event_action] == "login_failure" {
      mutate {
        add_tag => ["security_alert"]
      }
    }
    
    # Redact sensitive data from query text
    if [query][text] {
      mutate {
        gsub => [
          "[query][text]", "password\s*=\s*'[^']*'", "password='REDACTED'",
          "[query][text]", "ssn\s*=\s*'[^']*'", "ssn='REDACTED'"
        ]
      }
    }
  }
}

output {
  if [fields][log_type] == "audit" {
    elasticsearch {
      hosts => ["https://elasticsearch.example.com:9200"]
      index => "themisdb-audit-%{+YYYY.MM.dd}"
      user => "logstash_writer"
      password => "${ELASTICSEARCH_PASSWORD}"
      ssl => true
      cacert => "/etc/pki/tls/certs/elasticsearch.crt"
    }
    
    # Send security alerts to separate index
    if "security_alert" in [tags] {
      elasticsearch {
        hosts => ["https://elasticsearch.example.com:9200"]
        index => "themisdb-security-alerts-%{+YYYY.MM.dd}"
        user => "logstash_writer"
        password => "${ELASTICSEARCH_PASSWORD}"
      }
    }
  }
}
```

### Audit Log Retention and Archival

**Retention Policy Configuration:**

```yaml
# /opt/themisdb/config/audit_retention.yaml
audit_retention:
  # Hot storage (fast access)
  hot:
    duration_days: 30
    storage_path: "/var/log/themisdb/audit"
    compression: "none"
  
  # Warm storage (compressed)
  warm:
    duration_days: 90
    storage_path: "/mnt/warm-storage/themisdb/audit"
    compression: "gzip"
    compression_level: 6
  
  # Cold storage (archived)
  cold:
    duration_days: 365
    storage_path: "s3://audit-archive/themisdb"
    compression: "zstd"
    compression_level: 9
    encryption: true
  
  # Long-term archive (compliance)
  archive:
    duration_days: 2555  # 7 years
    storage_path: "glacier://compliance-archive/themisdb"
    encryption: true
    worm: true  # Write-Once-Read-Many for compliance
```

**Audit Log Archival Script:**

```bash
#!/bin/bash
# archive_audit_logs.sh - Archive old audit logs

set -e

AUDIT_DIR="/var/log/themisdb/audit"
WARM_STORAGE="/mnt/warm-storage/themisdb/audit"
COLD_STORAGE="s3://audit-archive/themisdb"
ENCRYPTION_KEY="/opt/themisdb/keys/archive.key"

# Archive logs older than 30 days to warm storage
find $AUDIT_DIR -name "audit.log.*" -type f -mtime +30 | while read -r logfile; do
    filename=$(basename "$logfile")
    
    # Compress and move to warm storage
    gzip -c "$logfile" > "$WARM_STORAGE/$filename.gz"
    
    # Verify
    if gzip -t "$WARM_STORAGE/$filename.gz"; then
        rm "$logfile"
        echo "✓ Archived to warm: $filename"
    fi
done

# Archive logs older than 90 days to cold storage (S3)
find $WARM_STORAGE -name "audit.log.*.gz" -type f -mtime +90 | while read -r logfile; do
    filename=$(basename "$logfile" .gz)
    
    # Re-compress with zstd for better compression
    gunzip -c "$logfile" | zstd -19 > "/tmp/$filename.zst"
    
    # Encrypt
    openssl enc -aes-256-cbc -salt \
      -in "/tmp/$filename.zst" \
      -out "/tmp/$filename.zst.enc" \
      -pass file:"$ENCRYPTION_KEY"
    
    # Upload to S3
    aws s3 cp "/tmp/$filename.zst.enc" \
      "$COLD_STORAGE/$(date +%Y)/$(date +%m)/$filename.zst.enc" \
      --storage-class STANDARD_IA
    
    # Verify upload
    if aws s3 ls "$COLD_STORAGE/$(date +%Y)/$(date +%m)/$filename.zst.enc"; then
        rm "$logfile" "/tmp/$filename.zst" "/tmp/$filename.zst.enc"
        echo "✓ Archived to cold: $filename"
    fi
done

# Archive logs older than 365 days to Glacier
aws s3 ls "$COLD_STORAGE/" --recursive | \
  awk '{print $4}' | \
  grep "audit.log" | while read -r s3_key; do
    
    # Check if older than 365 days
    file_date=$(echo "$s3_key" | grep -oP '\d{8}')
    file_epoch=$(date -d "$file_date" +%s)
    now_epoch=$(date +%s)
    age_days=$(( ($now_epoch - $file_epoch) / 86400 ))
    
    if [ $age_days -gt 365 ]; then
        # Move to Glacier Deep Archive
        aws s3api restore-object \
          --bucket "audit-archive" \
          --key "themisdb/$s3_key" \
          --restore-request '{"Days":1,"GlacierJobParameters":{"Tier":"Bulk"}}'
        
        echo "✓ Archived to Glacier: $s3_key"
    fi
done
```

### Audit Query and Analysis

**Audit Query Examples:**

```bash
#!/bin/bash
# audit_queries.sh - Common audit log queries

# 1. Failed login attempts in last 24 hours
echo "=== Failed Login Attempts ==="
jq -r 'select(.event_action == "login_failure" and 
              (.timestamp | fromdateiso8601) > (now - 86400)) | 
       [.timestamp, .user.username, .user.ip_address] | @tsv' \
  /var/log/themisdb/audit/audit.log

# 2. All actions by specific user
echo "=== Actions by User: admin ==="
jq -r 'select(.user.username == "admin") | 
       [.timestamp, .event_action, .resource.database, .resource.table] | @tsv' \
  /var/log/themisdb/audit/audit.log

# 3. All DELETE operations
echo "=== DELETE Operations ==="
jq -r 'select(.resource.operation == "DELETE") | 
       [.timestamp, .user.username, .resource.database, .resource.table, .query.rows_affected] | @tsv' \
  /var/log/themisdb/audit/audit.log

# 4. Slow queries (> 1 second)
echo "=== Slow Queries ==="
jq -r 'select(.query.execution_time_ms > 1000) | 
       [.timestamp, .user.username, .query.execution_time_ms, .query.text] | @tsv' \
  /var/log/themisdb/audit/audit.log

# 5. Access denied events
echo "=== Access Denied ==="
jq -r 'select(.event_result == "denied") | 
       [.timestamp, .user.username, .event_action, .resource.database] | @tsv' \
  /var/log/themisdb/audit/audit.log

# 6. Administrative actions
echo "=== Administrative Actions ==="
jq -r 'select(.event_type == "administrative") | 
       [.timestamp, .user.username, .event_action] | @tsv' \
  /var/log/themisdb/audit/audit.log
```

**Elasticsearch Query Examples:**

```json
// Failed login attempts from same IP
GET themisdb-audit-*/_search
{
  "query": {
    "bool": {
      "must": [
        {
          "term": {
            "event_action": "login_failure"
          }
        },
        {
          "range": {
            "timestamp": {
              "gte": "now-1h"
            }
          }
        }
      ]
    }
  },
  "aggs": {
    "by_ip": {
      "terms": {
        "field": "user.ip_address",
        "size": 10
      }
    }
  }
}

// Audit timeline for specific transaction
GET themisdb-audit-*/_search
{
  "query": {
    "term": {
      "user.session_id": "sess_abc123"
    }
  },
  "sort": [
    {
      "timestamp": "asc"
    }
  ]
}

// Detect privilege escalation
GET themisdb-audit-*/_search
{
  "query": {
    "bool": {
      "must": [
        {
          "term": {
            "event_action": "role_changed"
          }
        },
        {
          "range": {
            "timestamp": {
              "gte": "now-24h"
            }
          }
        }
      ]
    }
  }
}
```

---

## Network Security

### Firewall Configuration

**UFW (Uncomplicated Firewall) Configuration:**

```bash
#!/bin/bash
# configure_firewall_ufw.sh - Configure UFW for ThemisDB

# Enable UFW
ufw --force enable

# Default policies
ufw default deny incoming
ufw default allow outgoing

# Allow SSH (restrict to management network)
ufw allow from 10.0.0.0/8 to any port 22 proto tcp

# Allow ThemisDB port (restrict to application network)
ufw allow from 192.168.1.0/24 to any port 7700 proto tcp

# Allow monitoring port (restrict to monitoring network)
ufw allow from 10.1.0.0/24 to any port 9091 proto tcp

# Allow replication between cluster nodes
ufw allow from 192.168.1.10 to any port 7701 proto tcp
ufw allow from 192.168.1.11 to any port 7701 proto tcp
ufw allow from 192.168.1.12 to any port 7701 proto tcp

# Rate limiting for SSH (prevent brute force)
ufw limit ssh/tcp

# Logging
ufw logging medium

# Show status
ufw status verbose
```

**iptables Configuration:**

```bash
#!/bin/bash
# configure_firewall_iptables.sh - Configure iptables for ThemisDB

# Flush existing rules
iptables -F
iptables -X
iptables -Z

# Default policies
iptables -P INPUT DROP
iptables -P FORWARD DROP
iptables -P OUTPUT ACCEPT

# Allow loopback
iptables -A INPUT -i lo -j ACCEPT
iptables -A OUTPUT -o lo -j ACCEPT

# Allow established connections
iptables -A INPUT -m conntrack --ctstate ESTABLISHED,RELATED -j ACCEPT

# Allow SSH from management network
iptables -A INPUT -p tcp -s 10.0.0.0/8 --dport 22 -m conntrack --ctstate NEW -j ACCEPT

# Allow ThemisDB from application network
iptables -A INPUT -p tcp -s 192.168.1.0/24 --dport 7700 -m conntrack --ctstate NEW -j ACCEPT

# Rate limit ThemisDB connections (1000/sec per IP)
iptables -A INPUT -p tcp --dport 7700 -m hashlimit \
  --hashlimit-above 1000/sec \
  --hashlimit-mode srcip \
  --hashlimit-name themisdb_rate_limit \
  -j DROP

# Allow monitoring from monitoring network
iptables -A INPUT -p tcp -s 10.1.0.0/24 --dport 9091 -m conntrack --ctstate NEW -j ACCEPT

# Allow replication between cluster nodes
for node_ip in 192.168.1.10 192.168.1.11 192.168.1.12; do
    iptables -A INPUT -p tcp -s $node_ip --dport 7701 -j ACCEPT
done

# Log dropped packets
iptables -A INPUT -m limit --limit 5/min -j LOG --log-prefix "iptables-dropped: " --log-level 7

# Drop everything else
iptables -A INPUT -j DROP

# Save rules
iptables-save > /etc/iptables/rules.v4
```

### VPC and Network Isolation

**AWS VPC Security Group Configuration:**

```yaml
# terraform/security_groups.tf - AWS Security Groups for ThemisDB
resource "aws_security_group" "themisdb_server" {
  name        = "themisdb-server"
  description = "Security group for ThemisDB servers"
  vpc_id      = aws_vpc.main.id

  # Allow ThemisDB from application tier
  ingress {
    description     = "ThemisDB from application tier"
    from_port       = 7700
    to_port         = 7700
    protocol        = "tcp"
    security_groups = [aws_security_group.application.id]
  }

  # Allow replication between cluster nodes
  ingress {
    description = "Replication between nodes"
    from_port   = 7701
    to_port     = 7701
    protocol    = "tcp"
    self        = true
  }

  # Allow monitoring from monitoring tier
  ingress {
    description     = "Prometheus metrics"
    from_port       = 9091
    to_port         = 9091
    protocol        = "tcp"
    security_groups = [aws_security_group.monitoring.id]
  }

  # Allow SSH from bastion
  ingress {
    description     = "SSH from bastion"
    from_port       = 22
    to_port         = 22
    protocol        = "tcp"
    security_groups = [aws_security_group.bastion.id]
  }

  # Allow all outbound
  egress {
    from_port   = 0
    to_port     = 0
    protocol    = "-1"
    cidr_blocks = ["0.0.0.0/0"]
  }

  tags = {
    Name = "themisdb-server"
  }
}
```

### Rate Limiting and DDoS Protection

**Application-Level Rate Limiting:**

```yaml
# /opt/themisdb/config/rate_limiting.yaml
rate_limiting:
  enabled: true
  
  # Global rate limits
  global:
    max_connections: 10000
    max_queries_per_second: 50000
    max_transactions_per_second: 10000
  
  # Per-IP rate limits
  per_ip:
    max_connections: 100
    max_queries_per_second: 1000
    max_transactions_per_second: 100
    
    # Burst allowance
    burst_size: 150
    burst_duration_seconds: 10
  
  # Per-user rate limits
  per_user:
    max_queries_per_second: 500
    max_transactions_per_second: 50
    max_concurrent_transactions: 10
  
  # Per-database rate limits
  per_database:
    production_db:
      max_queries_per_second: 10000
    development_db:
      max_queries_per_second: 1000
  
  # Rate limit actions
  actions:
    on_limit_exceeded:
      action: "reject"  # Options: reject, delay, queue
      delay_ms: 100
      queue_size: 1000
    
    on_repeated_violations:
      threshold: 10  # violations per minute
      action: "temporary_ban"
      ban_duration_minutes: 15
      
  # Exemptions
  exempted_ips:
    - "10.0.0.0/8"  # Internal network
    - "192.168.1.50"  # Monitoring server
  
  exempted_users:
    - "admin"
    - "monitoring"
```

**Cloudflare DDoS Protection Configuration:**

```yaml
# cloudflare_ddos_protection.yaml - Cloudflare Enterprise DDoS settings
cloudflare:
  ddos_protection:
    # Layer 7 DDoS mitigation
    l7_rules:
      - name: "Rate limit database connections"
        expression: '(http.request.uri.path contains "/connect")'
        action: "challenge"
        rate_limit:
          requests_per_minute: 100
          period: 60
      
      - name: "Block suspicious query patterns"
        expression: '(http.request.uri.query contains "DROP TABLE" or http.request.uri.query contains "DROP DATABASE")'
        action: "block"
      
      - name: "Geo-blocking"
        expression: '(ip.geoip.country in {"CN" "RU"})'
        action: "challenge"
    
    # Layer 3/4 DDoS mitigation
    l3_l4_rules:
      enabled: true
      sensitivity: "high"
      
    # Advanced settings
    advanced:
      javascript_challenge: true
      browser_integrity_check: true
      security_level: "high"
```


---

## Security Hardening

### Operating System Hardening

**System Hardening Script:**

```bash
#!/bin/bash
# harden_os.sh - Operating system hardening for ThemisDB

set -e

echo "=== ThemisDB Operating System Hardening ==="

# 1. Disable unnecessary services
echo "1. Disabling unnecessary services..."
systemctl disable avahi-daemon.service 2>/dev/null || true
systemctl disable cups.service 2>/dev/null || true
systemctl disable bluetooth.service 2>/dev/null || true

# 2. Configure kernel parameters
echo "2. Configuring kernel parameters..."
cat >> /etc/sysctl.conf <<EOF

# ThemisDB Security Hardening
# Disable IP forwarding
net.ipv4.ip_forward = 0

# Disable ICMP redirects
net.ipv4.conf.all.accept_redirects = 0
net.ipv4.conf.default.accept_redirects = 0
net.ipv6.conf.all.accept_redirects = 0
net.ipv6.conf.default.accept_redirects = 0

# Disable source routing
net.ipv4.conf.all.accept_source_route = 0
net.ipv4.conf.default.accept_source_route = 0

# Enable SYN cookies (DDoS protection)
net.ipv4.tcp_syncookies = 1

# Log suspicious packets
net.ipv4.conf.all.log_martians = 1
net.ipv4.conf.default.log_martians = 1

# Ignore ICMP ping requests
net.ipv4.icmp_echo_ignore_all = 1

# Increase connection backlog
net.core.somaxconn = 4096
net.ipv4.tcp_max_syn_backlog = 4096

# TCP hardening
net.ipv4.tcp_timestamps = 0
net.ipv4.tcp_rfc1337 = 1

# Kernel pointer hiding
kernel.kptr_restrict = 2
kernel.dmesg_restrict = 1
EOF

sysctl -p

# 3. Configure file system permissions
echo "3. Configuring file system permissions..."
chmod 700 /opt/themisdb/config
chmod 700 /opt/themisdb/keys
chmod 700 /var/lib/themisdb
chmod 750 /opt/themisdb/bin

# 4. Configure umask
echo "4. Setting secure umask..."
echo "umask 027" >> /etc/profile

# 5. Set password aging
echo "5. Configuring password aging..."
sed -i 's/^PASS_MAX_DAYS.*/PASS_MAX_DAYS   90/' /etc/login.defs
sed -i 's/^PASS_MIN_DAYS.*/PASS_MIN_DAYS   1/' /etc/login.defs
sed -i 's/^PASS_MIN_LEN.*/PASS_MIN_LEN    12/' /etc/login.defs

# 6. Configure SSH hardening
echo "6. Hardening SSH configuration..."
cat >> /etc/ssh/sshd_config <<EOF

# ThemisDB SSH Hardening
Protocol 2
PermitRootLogin no
PubkeyAuthentication yes
PasswordAuthentication no
PermitEmptyPasswords no
ChallengeResponseAuthentication no
UsePAM yes
X11Forwarding no
MaxAuthTries 3
ClientAliveInterval 300
ClientAliveCountMax 2
AllowUsers themisdb admin
EOF

systemctl restart sshd

# 7. Configure audit daemon
echo "7. Enabling audit daemon..."
systemctl enable auditd
systemctl start auditd

# Add audit rules
cat >> /etc/audit/rules.d/themisdb.rules <<EOF
# ThemisDB Audit Rules
-w /opt/themisdb/config/ -p wa -k themisdb_config_change
-w /opt/themisdb/keys/ -p rwa -k themisdb_key_access
-w /var/lib/themisdb/ -p wa -k themisdb_data_change
-w /etc/systemd/system/themisdb.service -p wa -k themisdb_service_change
EOF

augenrules --load

# 8. Enable AIDE (intrusion detection)
echo "8. Configuring AIDE..."
apt-get install -y aide
aideinit
mv /var/lib/aide/aide.db.new /var/lib/aide/aide.db

# Add daily AIDE check
cat > /etc/cron.daily/aide-check <<'EOF'
#!/bin/bash
/usr/bin/aide --check | mail -s "AIDE Report for $(hostname)" security@example.com
EOF
chmod 755 /etc/cron.daily/aide-check

# 9. Configure fail2ban
echo "9. Configuring fail2ban..."
apt-get install -y fail2ban

cat > /etc/fail2ban/jail.d/themisdb.conf <<EOF
[themisdb-auth]
enabled = true
port = 7700
logpath = /var/log/themisdb/audit.log
filter = themisdb-auth
maxretry = 5
bantime = 3600
findtime = 600

[themisdb-abuse]
enabled = true
port = 7700
logpath = /var/log/themisdb/themisdb.log
filter = themisdb-abuse
maxretry = 10
bantime = 1800
findtime = 300
EOF

cat > /etc/fail2ban/filter.d/themisdb-auth.conf <<EOF
[Definition]
failregex = ^.*"event_action":"login_failure".*"ip_address":"<HOST>".*$
ignoreregex =
EOF

cat > /etc/fail2ban/filter.d/themisdb-abuse.conf <<EOF
[Definition]
failregex = ^.*ERROR.*from <HOST>.*rate limit exceeded.*$
            ^.*WARN.*suspicious activity from <HOST>.*$
ignoreregex =
EOF

systemctl enable fail2ban
systemctl restart fail2ban

# 10. Install security updates automatically
echo "10. Enabling automatic security updates..."
apt-get install -y unattended-upgrades
dpkg-reconfigure -plow unattended-upgrades

echo ""
echo "=== Operating system hardening complete ==="
```

### Compiler Security Hardening (SEC-CC-4)

ThemisDB automatically enables compiler and linker hardening flags for all **Release builds**.
No additional configuration is required for standard presets.

**Active flags per platform:**

| Platform | Compile flags | Linker flags |
|---|---|---|
| Linux (GCC/Clang) | `-fstack-protector-strong` `-D_FORTIFY_SOURCE=3` `-fPIE` `-fstack-clash-protection`* | `-pie` `-Wl,-z,relro` `-Wl,-z,now` `-Wl,-z,noexecstack` |
| macOS (Clang) | `-fstack-protector-strong` `-D_FORTIFY_SOURCE=3` `-fPIE` `-fstack-clash-protection`* | `-pie` |
| Windows (MSVC) | `/GS` `/sdl` `/guard:cf` | `/GUARD:CF` `/NXCOMPAT` `/DYNAMICBASE` |

\* `-fstack-clash-protection` is applied when supported by the compiler (GCC ≥ 8, Clang ≥ 11).
† RELRO linker flags (`-Wl,-z,relro/-z,now/-z,noexecstack`) are **Linux-only**; macOS `ld64` does not support ELF-style `-z` flags. ASLR and stack protection are provided natively by the macOS OS and `ld64`.

**Build command (hardening on by default):**
```bash
cmake --preset community-release
cmake --build --preset community-release
```

**Verify hardening on Linux:**
```bash
# Requires checksec tool
checksec --file=build-linux-release/themisdb_server
# Expected: Full RELRO, Canary found, NX enabled, PIE enabled
```

**Important:** Disabling hardening (`-DTHEMIS_DISABLE_SECURITY_HARDENING=ON`) is a
security policy violation (SEC-CC-4) and must not be used in production builds.

For the complete flag reference, compiler version requirements, and policy for new
platforms see [`docs/de/security/COMPILER_SECURITY_HARDENING.md`](../../de/security/COMPILER_SECURITY_HARDENING.md).

---

### Application Hardening

**ThemisDB Security Configuration:**

```yaml
# /opt/themisdb/config/security_hardening.yaml
security:
  # Process isolation
  process:
    run_as_user: "themisdb"
    run_as_group: "themisdb"
    chroot: false  # Optional chroot jail
    
  # Resource limits
  limits:
    max_open_files: 65536
    max_processes: 4096
    max_memory_mb: 65536
    max_cpu_percent: 90
    
  # Query safety
  query_safety:
    max_query_length: 65536
    max_result_rows: 1000000
    max_execution_time_seconds: 300
    allow_dangerous_operations: false
    dangerous_operations:
      - "DROP DATABASE"
      - "DROP TABLE"
      - "TRUNCATE TABLE"
    require_confirmation: true
    
  # Input validation
  input_validation:
    sanitize_inputs: true
    max_key_length: 1024
    max_value_length: 10485760  # 10MB
    allowed_key_characters: "alphanumeric_underscore_dash"
    block_null_bytes: true
    
  # SQL injection prevention
  sql_injection_prevention:
    enabled: true
    parameterized_queries_only: false
    detect_sql_keywords: true
    block_suspicious_patterns: true
    
  # Buffer overflow protection
  buffer_protection:
    stack_canaries: true
    aslr: true  # Address Space Layout Randomization
    dep: true   # Data Execution Prevention
    
  # Core dumps
  core_dumps:
    enabled: false  # Disable in production
    sanitize_dumps: true  # Remove sensitive data if enabled
    
  # Security headers (for HTTP API)
  http_headers:
    x_content_type_options: "nosniff"
    x_frame_options: "DENY"
    x_xss_protection: "1; mode=block"
    strict_transport_security: "max-age=31536000; includeSubDomains"
    content_security_policy: "default-src 'self'"
```

### Container Security (Docker)

**Secure Docker Configuration:**

```dockerfile
# Dockerfile.secure - Secure Docker image for ThemisDB
FROM ubuntu:22.04 AS builder

# Build stage (omitted for brevity)
# ...

# Production stage
FROM ubuntu:22.04

# Create non-root user
RUN groupadd -r themisdb -g 999 && \
    useradd -r -g themisdb -u 999 -d /opt/themisdb -s /sbin/nologin themisdb

# Install runtime dependencies only
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        ca-certificates \
        libssl3 \
        libjemalloc2 && \
    rm -rf /var/lib/apt/lists/*

# Copy binary and configuration
COPY --from=builder --chown=themisdb:themisdb /opt/themisdb/bin/themisdb /opt/themisdb/bin/
COPY --chown=themisdb:themisdb config/ /opt/themisdb/config/

# Set secure permissions
RUN chmod 750 /opt/themisdb/bin && \
    chmod 700 /opt/themisdb/config

# Create data directory
RUN mkdir -p /var/lib/themisdb && \
    chown -R themisdb:themisdb /var/lib/themisdb && \
    chmod 700 /var/lib/themisdb

# Set user
USER themisdb

# Health check
HEALTHCHECK --interval=30s --timeout=10s --start-period=60s --retries=3 \
    CMD /opt/themisdb/bin/themisdb-healthcheck || exit 1

# Expose port
EXPOSE 7700

# Run
ENTRYPOINT ["/opt/themisdb/bin/themisdb"]
CMD ["--config", "/opt/themisdb/config/themisdb.yaml"]
```

**Docker Compose Security:**

```yaml
# docker-compose.security.yml
version: '3.8'

services:
  themisdb:
    image: themisdb:1.4.0-secure
    container_name: themisdb-server
    
    # Security options
    security_opt:
      - no-new-privileges:true
      - seccomp:unconfined
      - apparmor=themisdb-profile
    
    # Read-only root filesystem
    read_only: true
    
    # Temporary directories (writable)
    tmpfs:
      - /tmp:noexec,nosuid,size=100m
      - /var/tmp:noexec,nosuid,size=100m
    
    # Resource limits
    deploy:
      resources:
        limits:
          cpus: '4.0'
          memory: 8G
        reservations:
          cpus: '2.0'
          memory: 4G
    
    # User namespace remapping
    user: "999:999"
    
    # Capabilities (drop all, add only necessary)
    cap_drop:
      - ALL
    cap_add:
      - CHOWN
      - SETUID
      - SETGID
    
    # Network mode
    networks:
      - themisdb-net
    
    # Volumes (with specific mount options)
    volumes:
      - type: bind
        source: /opt/themisdb/config
        target: /opt/themisdb/config
        read_only: true
      - type: volume
        source: themisdb-data
        target: /var/lib/themisdb
      - type: volume
        source: themisdb-logs
        target: /var/log/themisdb
    
    # Environment variables (from secrets)
    environment:
      - THEMISDB_MASTER_KEY_FILE=/run/secrets/master_key
    
    secrets:
      - master_key
      - tls_cert
      - tls_key
    
    # Restart policy
    restart: unless-stopped

networks:
  themisdb-net:
    driver: bridge
    ipam:
      config:
        - subnet: 172.28.0.0/16

volumes:
  themisdb-data:
    driver: local
    driver_opts:
      type: none
      o: bind
      device: /var/lib/themisdb
  themisdb-logs:
    driver: local

secrets:
  master_key:
    file: /opt/themisdb/keys/master.key
  tls_cert:
    file: /opt/themisdb/certs/server-cert.pem
  tls_key:
    file: /opt/themisdb/keys/server-key.pem
```

---

## Compliance Considerations

### Regulatory Compliance

**Compliance Framework Mapping:**

| Requirement | Standard | ThemisDB Implementation | Configuration |
|-------------|----------|------------------------|---------------|
| **Data Encryption (at rest)** | GDPR, HIPAA, PCI-DSS | AES-256-GCM encryption | `encryption.at_rest.enabled` |
| **Data Encryption (in transit)** | GDPR, HIPAA, PCI-DSS | TLS 1.2+, mTLS optional | `server.tls.enabled` |
| **Access Control** | GDPR, HIPAA, SOX | RBAC, user authentication | `rbac.enabled` |
| **Audit Logging** | GDPR, HIPAA, SOX, PCI-DSS | Comprehensive audit logs | `audit.enabled` |
| **Data Retention** | GDPR, HIPAA | Configurable retention | `audit_retention` |
| **Right to be Forgotten** | GDPR | Hard delete support | Application-level |
| **Access Logs** | GDPR, SOX | Detailed access logs | `audit.events.data_access` |
| **Breach Notification** | GDPR, HIPAA | Audit log monitoring | SIEM integration |
| **Data Minimization** | GDPR | Field-level encryption | `field_encryption` |
| **Password Complexity** | PCI-DSS, HIPAA | Configurable policy | `password_policy` |
| **Session Timeout** | PCI-DSS | JWT token expiry | `jwt.tokens.access_token.expiry` |
| **Key Rotation** | PCI-DSS | Automated rotation | `key_rotation.enabled` |
| **Separation of Duties** | SOX | Role-based permissions | `rbac.roles` |
| **Change Management** | SOX | Audit logging of config changes | `audit.events.administrative` |
| **Backup Encryption** | HIPAA, PCI-DSS | Encrypted backups | `encryption.at_rest.encrypt_backups` |

### GDPR Compliance Checklist

```yaml
# gdpr_compliance.yaml - GDPR-specific configuration
gdpr:
  # Data protection
  data_protection:
    encryption_at_rest: true
    encryption_in_transit: true
    pseudonymization: true  # Field-level encryption
    
  # Right to access
  data_export:
    enabled: true
    format: "json"  # Machine-readable format
    max_export_size_mb: 1000
    
  # Right to be forgotten
  data_deletion:
    enabled: true
    hard_delete: true
    deletion_verification: true
    deletion_audit: true
    
  # Data retention
  retention:
    default_retention_days: 365
    retention_policies:
      - data_type: "user_data"
        retention_days: 1095  # 3 years
      - data_type: "audit_logs"
        retention_days: 2555  # 7 years
      - data_type: "session_data"
        retention_days: 30
    
    auto_deletion: true
    deletion_schedule: "0 2 * * *"  # Daily at 2 AM
    
  # Breach detection
  breach_detection:
    enabled: true
    alert_threshold: "unauthorized_access"
    notification_delay_hours: 72
    notification_recipients:
      - "dpo@example.com"
      - "security@example.com"
    
  # Consent management
  consent:
    track_consent: true
    audit_consent_changes: true
    
  # Data portability
  data_portability:
    export_formats:
      - "json"
      - "csv"
      - "xml"
```

### Compliance Audit Report

**Automated Compliance Check:**

```bash
#!/bin/bash
# compliance_audit.sh - Generate compliance audit report

REPORT_FILE="/tmp/compliance_audit_$(date +%Y%m%d_%H%M%S).html"

cat > $REPORT_FILE <<'EOF'
<!DOCTYPE html>
<html>
<head>
    <title>ThemisDB Compliance Audit Report</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; }
        h1 { color: #333; }
        table { border-collapse: collapse; width: 100%; margin-top: 20px; }
        th, td { border: 1px solid #ddd; padding: 12px; text-align: left; }
        th { background-color: #4CAF50; color: white; }
        .pass { background-color: #d4edda; color: #155724; }
        .fail { background-color: #f8d7da; color: #721c24; }
        .warn { background-color: #fff3cd; color: #856404; }
    </style>
</head>
<body>
    <h1>ThemisDB Compliance Audit Report</h1>
    <p><strong>Generated:</strong> $(date)</p>
    <p><strong>ThemisDB Version:</strong> 1.4.0</p>
    
    <h2>Compliance Summary</h2>
    <table>
        <tr>
            <th>Control</th>
            <th>Requirement</th>
            <th>Status</th>
            <th>Details</th>
        </tr>
EOF

# Function to add check result
add_check() {
    local control="$1"
    local requirement="$2"
    local status="$3"
    local details="$4"
    
    cat >> $REPORT_FILE <<EOF
        <tr class="$status">
            <td>$control</td>
            <td>$requirement</td>
            <td>$(echo $status | tr '[:lower:]' '[:upper:]')</td>
            <td>$details</td>
        </tr>
EOF
}

# Encryption at rest
if grep -q "encryption_at_rest: true" /opt/themisdb/config/themisdb.yaml; then
    add_check "ENC-001" "Encryption at Rest" "pass" "AES-256-GCM enabled"
else
    add_check "ENC-001" "Encryption at Rest" "fail" "Encryption disabled"
fi

# TLS enabled
if grep -q "tls_enabled: true" /opt/themisdb/config/themisdb.yaml; then
    add_check "ENC-002" "Encryption in Transit" "pass" "TLS 1.2+ enabled"
else
    add_check "ENC-002" "Encryption in Transit" "fail" "TLS disabled"
fi

# Authentication required
if grep -q "authentication_required: true" /opt/themisdb/config/themisdb.yaml; then
    add_check "AUTH-001" "Authentication Required" "pass" "Authentication enforced"
else
    add_check "AUTH-001" "Authentication Required" "fail" "Authentication disabled"
fi

# Audit logging
if grep -q "audit_logging: true" /opt/themisdb/config/themisdb.yaml; then
    add_check "AUDIT-001" "Audit Logging" "pass" "Comprehensive audit logging enabled"
else
    add_check "AUDIT-001" "Audit Logging" "fail" "Audit logging disabled"
fi

# Certificate expiry
CERT_EXPIRY=$(openssl x509 -enddate -noout -in /opt/themisdb/certs/server-cert.pem 2>/dev/null | cut -d= -f2)
if [ ! -z "$CERT_EXPIRY" ]; then
    EXPIRY_EPOCH=$(date -d "$CERT_EXPIRY" +%s)
    NOW_EPOCH=$(date +%s)
    DAYS_UNTIL_EXPIRY=$(( ($EXPIRY_EPOCH - $NOW_EPOCH) / 86400 ))
    
    if [ $DAYS_UNTIL_EXPIRY -gt 30 ]; then
        add_check "CERT-001" "Certificate Validity" "pass" "Valid for $DAYS_UNTIL_EXPIRY days"
    else
        add_check "CERT-001" "Certificate Validity" "warn" "Expires in $DAYS_UNTIL_EXPIRY days"
    fi
fi

# Password policy
if grep -q "min_length: 12" /opt/themisdb/config/users.yaml; then
    add_check "PWD-001" "Password Complexity" "pass" "12+ character requirement"
else
    add_check "PWD-001" "Password Complexity" "fail" "Weak password policy"
fi

# Backup encryption
if grep -q "encrypt_backups: true" /opt/themisdb/config/themisdb.yaml; then
    add_check "BACKUP-001" "Backup Encryption" "pass" "Backups encrypted"
else
    add_check "BACKUP-001" "Backup Encryption" "warn" "Backup encryption not configured"
fi

# Close HTML
cat >> $REPORT_FILE <<'EOF'
    </table>
</body>
</html>
EOF

echo "Compliance audit report generated: $REPORT_FILE"
xdg-open $REPORT_FILE 2>/dev/null || open $REPORT_FILE 2>/dev/null || echo "Open $REPORT_FILE in a browser"
```

---

## Related Documentation

- **[Production Deployment Guide](../deployment/PRODUCTION_DEPLOYMENT_GUIDE.md)** - Complete production deployment procedures
- **[Operational Procedures](../operations/OPERATIONAL_PROCEDURES.md)** - Day-to-day operational tasks
- **[Monitoring Setup Guide](../operations/MONITORING_SETUP_GUIDE.md)** - Monitoring and alerting configuration
- **[Troubleshooting Guide](../operations/TROUBLESHOOTING_GUIDE.md)** - Diagnosing and resolving issues
- **[MVCC Tuning Guide](../features/MVCC_TUNING_GUIDE.md)** - Transaction performance tuning
- **[RocksDB Optimization Guide](../storage/ROCKSDB_OPTIMIZATION_GUIDE.md)** - Storage layer optimization
- **[Transaction Best Practices](../features/TRANSACTION_BEST_PRACTICES.md)** - Secure transaction patterns

---

**Document Version:** 1.0  
**ThemisDB Compatibility:** 1.4.0+  
**Last Reviewed:** 2026-01-18  
**Next Review:** 2026-04-18

---

## Document Change Log

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0 | 2026-01-18 | ThemisDB Team | Initial comprehensive security deployment guide |

