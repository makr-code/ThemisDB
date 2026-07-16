# ThemisDB Security Certification (TSC)

## Certification Overview

The **ThemisDB Security Certification (TSC)** is an expert-level certification that validates your mastery of database security, compliance, and incident response. This certification demonstrates that you can implement comprehensive security controls, ensure regulatory compliance, conduct security audits, and respond effectively to security incidents.

### Certification Details

- **Certification Code**: TSC
- **Level**: Expert
- **Duration**: 150 minutes (exam) + security audit project
- **Question Count**: 30-35 questions + practical assessment
- **Question Types**: Multiple choice, scenario-based, and hands-on security labs
- **Passing Score**: 80% (24/30 minimum on exam + passing security audit)
- **Validity**: 2 years
- **Prerequisites**: TDF + (TQE or TOC recommended)
- **Exam Fee**: $350 USD
- **Retake Fee**: $175 USD
- **Language**: English

---

## Target Audience

This certification is ideal for:

- **Security Engineers** securing database systems
- **Security Architects** designing secure architectures
- **Compliance Officers** ensuring regulatory compliance
- **Database Administrators** with security responsibilities
- **DevSecOps Engineers** implementing security automation
- **Risk Managers** assessing database security risks
- **Penetration Testers** evaluating database security
- **Chief Information Security Officers (CISOs)**

---

## Prerequisites

### Required Certification
- **ThemisDB Fundamentals Certification (TDF)** - Must be current

### Strongly Recommended
- **TQE or TOC certification** (demonstrates operational knowledge)
- Security+ or equivalent security certification
- Experience with compliance frameworks (GDPR, HIPAA, SOC 2)

### Technical Prerequisites
- 1+ year database security experience
- Understanding of cryptography fundamentals
- Knowledge of network security
- Familiarity with identity and access management
- Experience with security tools (SIEM, vulnerability scanners)
- Understanding of compliance requirements

---

## Learning Objectives

Upon completing this certification, you will be able to:

### 1. Authentication and Authorization (20%)
- Implement multi-factor authentication
- Configure SSO and federation
- Design role-based access control (RBAC)
- Implement attribute-based access control (ABAC)
- Manage service accounts and API keys
- Audit authentication events

### 2. Encryption (20%)
- Configure TLS/SSL for data in transit
- Implement encryption at rest
- Manage encryption keys
- Use hardware security modules (HSM)
- Implement column-level encryption
- Handle key rotation

### 3. RBAC and Access Control (15%)
- Design least-privilege access models
- Implement separation of duties
- Configure database-level permissions
- Manage object-level access control
- Implement row-level security
- Audit access patterns

### 4. Audit Logging and Compliance (15%)
- Configure comprehensive audit logging
- Implement log analysis and monitoring
- Ensure compliance with regulations
- Generate compliance reports
- Manage log retention
- Protect audit logs from tampering

### 5. Security Hardening (15%)
- Harden operating system
- Secure network configuration
- Minimize attack surface
- Implement defense in depth
- Configure security baselines
- Perform vulnerability assessments

### 6. Incident Response (10%)
- Detect security incidents
- Conduct forensic analysis
- Implement incident response procedures
- Perform breach notification
- Conduct post-incident review
- Implement preventive measures

### 7. Compliance Frameworks (5%)
- Understand GDPR requirements
- Implement HIPAA controls
- Achieve SOC 2 compliance
- Meet PCI DSS requirements
- Demonstrate ISO 27001 alignment

---

## Authentication and Authorization

### Multi-Factor Authentication

#### TOTP Configuration
```ini
# themisdb.conf
[authentication]
method = multi-factor
mfa-required = true
mfa-types = totp,webauthn

[totp]
issuer = ThemisDB
digits = 6
period = 30
algorithm = SHA256
```

#### User MFA Setup
```sql
-- Enable MFA for user
ALTER USER alice REQUIRE MFA;

-- Generate TOTP secret
SELECT GENERATE_TOTP_SECRET('alice') AS secret, 
       GENERATE_TOTP_QR_CODE('alice') AS qr_code;

-- Verify TOTP token
SELECT VERIFY_TOTP('alice', '123456') AS valid;
```

### Single Sign-On (SSO)

#### SAML 2.0 Configuration
```xml
<!-- saml-config.xml -->
<EntityDescriptor xmlns="urn:oasis:names:tc:SAML:2.0:metadata"
                  entityID="https://themisdb.company.com">
  
  <SPSSODescriptor protocolSupportEnumeration="urn:oasis:names:tc:SAML:2.0:protocol">
    <KeyDescriptor use="signing">
      <KeyInfo xmlns="http://www.w3.org/2000/09/xmldsig#">
        <X509Data>
          <X509Certificate>MIIDXTCCAkWgAwIBAgI...</X509Certificate>
        </X509Data>
      </KeyInfo>
    </KeyDescriptor>
    
    <AssertionConsumerService
        Binding="urn:oasis:names:tc:SAML:2.0:bindings:HTTP-POST"
        Location="https://themisdb.company.com/saml/acs"
        index="0" isDefault="true"/>
  </SPSSODescriptor>
</EntityDescriptor>
```

#### OAuth 2.0 / OpenID Connect
```ini
# themisdb.conf
[oauth]
enabled = true
provider = okta
client-id = your-client-id
client-secret-file = /etc/themisdb/oauth-secret
authorization-endpoint = https://company.okta.com/oauth2/v1/authorize
token-endpoint = https://company.okta.com/oauth2/v1/token
jwks-uri = https://company.okta.com/oauth2/v1/keys
scopes = openid,profile,email,groups

[role-mapping]
# Map OAuth groups to ThemisDB roles
okta-group.DBAdmins = db_admin
okta-group.Developers = db_developer
okta-group.Analysts = db_readonly
```

### Role-Based Access Control (RBAC)

#### Role Hierarchy
```sql
-- Create roles
CREATE ROLE db_admin;
CREATE ROLE db_developer;
CREATE ROLE db_readonly;
CREATE ROLE app_service;

-- Grant privileges to roles
GRANT ALL PRIVILEGES ON DATABASE production TO db_admin;
GRANT READ, WRITE ON DATABASE production TO db_developer;
GRANT READ ON DATABASE production TO db_readonly;
GRANT READ, WRITE ON production.orders TO app_service;
GRANT READ, WRITE ON production.customers TO app_service;

-- Create role hierarchy
GRANT db_readonly TO db_developer;
GRANT db_developer TO db_admin;

-- Assign roles to users
GRANT db_developer TO alice;
GRANT db_readonly TO bob;
GRANT app_service TO service_account_api;
```

#### Attribute-Based Access Control (ABAC)
```sql
-- Define security labels
CREATE SECURITY LABEL confidential;
CREATE SECURITY LABEL secret;
CREATE SECURITY LABEL top_secret;

-- Assign labels to data
ALTER TABLE employees 
ADD COLUMN security_label VARCHAR 
DEFAULT 'confidential';

UPDATE employees 
SET security_label = 'secret' 
WHERE department = 'Executive';

-- Create policy
CREATE SECURITY POLICY employee_access
ON employees
FOR SELECT
USING (
    security_label = 'confidential' 
    OR (security_label = 'secret' AND CURRENT_USER_HAS_CLEARANCE('secret'))
    OR (security_label = 'top_secret' AND CURRENT_USER_HAS_CLEARANCE('top_secret'))
);

-- Grant clearance to users
GRANT CLEARANCE 'secret' TO alice;
GRANT CLEARANCE 'confidential' TO bob;
```

### Row-Level Security (RLS)

```sql
-- Create policy for multi-tenant application
CREATE SECURITY POLICY tenant_isolation
ON orders
FOR ALL
USING (tenant_id = CURRENT_TENANT_ID());

-- Enable policy
ALTER TABLE orders ENABLE SECURITY POLICY tenant_isolation;

-- Users can only see their tenant's data
-- Set session context
SET SESSION tenant_id = 'tenant-123';

-- This query automatically filters by tenant
SELECT * FROM orders;  -- Only returns orders for tenant-123
```

---

## Encryption

### TLS/SSL Configuration

#### Generate Certificates
```bash
#!/bin/bash
# generate-certs.sh

# CA certificate
openssl req -new -x509 -days 3650 -nodes \
  -keyout ca-key.pem \
  -out ca-cert.pem \
  -subj "/C=US/ST=CA/L=SF/O=Company/CN=ThemisDB-CA"

# Server certificate
openssl req -new -nodes \
  -keyout server-key.pem \
  -out server-req.pem \
  -subj "/C=US/ST=CA/L=SF/O=Company/CN=themisdb.company.com"

# Sign server certificate
openssl x509 -req -in server-req.pem \
  -days 365 \
  -CA ca-cert.pem \
  -CAkey ca-key.pem \
  -set_serial 01 \
  -out server-cert.pem

# Client certificate
openssl req -new -nodes \
  -keyout client-key.pem \
  -out client-req.pem \
  -subj "/C=US/ST=CA/L=SF/O=Company/CN=alice"

# Sign client certificate
openssl x509 -req -in client-req.pem \
  -days 365 \
  -CA ca-cert.pem \
  -CAkey ca-key.pem \
  -set_serial 02 \
  -out client-cert.pem

# Set permissions
chmod 400 *-key.pem
chmod 444 *-cert.pem ca-cert.pem
```

#### Server TLS Configuration
```ini
# themisdb.conf
[ssl]
enabled = true
require-ssl = true
certificate = /etc/themisdb/ssl/server-cert.pem
private-key = /etc/themisdb/ssl/server-key.pem
ca-certificate = /etc/themisdb/ssl/ca-cert.pem
cipher-suites = TLS_AES_256_GCM_SHA384,TLS_CHACHA20_POLY1305_SHA256
min-protocol-version = TLSv1.3
verify-client-certificate = true
crl-file = /etc/themisdb/ssl/crl.pem
```

#### Client Connection
```bash
# Connect with TLS
themisdb-client \
  --ssl \
  --ssl-cert /path/to/client-cert.pem \
  --ssl-key /path/to/client-key.pem \
  --ssl-ca /path/to/ca-cert.pem \
  --server themisdb.company.com:8529
```

### Encryption at Rest

#### Configuration
```ini
# themisdb.conf
[encryption]
enabled = true
algorithm = AES-256-GCM
key-provider = vault  # or hsm, kms, file

[vault]
address = https://vault.company.com:8200
token-file = /etc/themisdb/vault-token
mount-path = secret/themisdb
key-name = database-encryption-key
```

#### Key Management with HashiCorp Vault
```bash
# Initialize Vault
vault secrets enable transit
vault write -f transit/keys/themisdb-encryption-key

# Grant ThemisDB access
vault policy write themisdb-encryption - <<EOF
path "transit/encrypt/themisdb-encryption-key" {
  capabilities = ["update"]
}
path "transit/decrypt/themisdb-encryption-key" {
  capabilities = ["update"]
}
path "transit/datakey/plaintext/themisdb-encryption-key" {
  capabilities = ["update"]
}
EOF

vault token create -policy=themisdb-encryption -period=768h
```

#### Column-Level Encryption
```sql
-- Create encrypted column
CREATE TABLE sensitive_data (
    id INT PRIMARY KEY,
    name VARCHAR(100),
    ssn VARCHAR(11) ENCRYPTED,
    credit_card VARCHAR(16) ENCRYPTED,
    created_at TIMESTAMP
);

-- Insert encrypted data
INSERT INTO sensitive_data (id, name, ssn, credit_card)
VALUES (1, 'John Doe', ENCRYPT('123-45-6789'), ENCRYPT('4111-1111-1111-1111'));

-- Query with decryption (requires privilege)
SELECT id, name, 
       DECRYPT(ssn) as ssn,
       DECRYPT(credit_card) as credit_card
FROM sensitive_data
WHERE id = 1;

-- Grant decryption privilege
GRANT DECRYPT ON sensitive_data.ssn TO alice;
GRANT DECRYPT ON sensitive_data.credit_card TO payment_processor;
```

### Key Rotation

```bash
#!/bin/bash
# rotate-encryption-keys.sh

# Generate new key
NEW_KEY=$(vault write -field=key transit/datakey/plaintext/themisdb-encryption-key-v2)

# Configure ThemisDB to use both keys
cat > /etc/themisdb/encryption-keys.conf <<EOF
[encryption-keys]
current-key-id = key-v2
keys = [
    {id: "key-v1", version: 1, retire-date: "2025-03-01"},
    {id: "key-v2", version: 2, active: true}
]
EOF

# Re-encrypt data with new key
themisdb-admin reencrypt \
  --database production \
  --old-key-id key-v1 \
  --new-key-id key-v2 \
  --parallel 8

# Verify re-encryption
themisdb-admin verify-encryption \
  --database production \
  --key-id key-v2

# Retire old key
vault write transit/keys/themisdb-encryption-key-v1/config \
  deletion_allowed=true
vault delete transit/keys/themisdb-encryption-key-v1
```

---

## Audit Logging and Compliance

### Comprehensive Audit Configuration

```ini
# themisdb.conf
[audit]
enabled = true
log-file = /var/log/themisdb/audit.log
log-format = json
rotation-size = 100MB
rotation-count = 100
compression = gzip

# What to audit
audit-connections = true
audit-authentication = true
audit-authorization-failures = true
audit-ddl = true
audit-dml = true
audit-privilege-changes = true
audit-configuration-changes = true
audit-backup-restore = true

# Sensitive data access
audit-sensitive-tables = [
    "production.customers",
    "production.credit_cards",
    "production.employees"
]

# Filter noise
exclude-users = [monitoring_user, backup_user]
exclude-queries = SELECT 1  # Health checks

[audit-encryption]
enabled = true
public-key-file = /etc/themisdb/audit-encryption-key.pub
```

### Audit Log Format

```json
{
  "timestamp": "2025-01-24T10:30:45.123Z",
  "event_type": "authentication",
  "event_id": "auth-12345",
  "user": "alice",
  "source_ip": "192.168.1.100",
  "success": true,
  "mfa_used": true,
  "session_id": "sess-67890"
}

{
  "timestamp": "2025-01-24T10:31:12.456Z",
  "event_type": "query",
  "event_id": "query-23456",
  "user": "alice",
  "database": "production",
  "table": "customers",
  "operation": "SELECT",
  "query_hash": "abc123...",
  "rows_returned": 1,
  "duration_ms": 15,
  "sensitive_data": true
}

{
  "timestamp": "2025-01-24T10:32:05.789Z",
  "event_type": "authorization_failure",
  "event_id": "authz-34567",
  "user": "bob",
  "requested_privilege": "DELETE",
  "database": "production",
  "table": "customers",
  "reason": "insufficient_privileges"
}
```

### GDPR Compliance

#### Data Subject Rights Implementation

```sql
-- Right to Access
CREATE PROCEDURE gdpr_subject_access_request(subject_email VARCHAR)
BEGIN
    -- Return all personal data for subject
    SELECT 'customers' as table_name, * 
    FROM customers 
    WHERE email = subject_email
    
    UNION ALL
    
    SELECT 'orders' as table_name, *
    FROM orders
    WHERE customer_email = subject_email
    
    UNION ALL
    
    SELECT 'profiles' as table_name, *
    FROM profiles
    WHERE email = subject_email;
END;

-- Right to Erasure (Right to be Forgotten)
CREATE PROCEDURE gdpr_erase_subject_data(subject_email VARCHAR, legal_basis VARCHAR)
BEGIN
    -- Log erasure request
    INSERT INTO gdpr_erasure_log (email, legal_basis, requested_at)
    VALUES (subject_email, legal_basis, NOW());
    
    -- Anonymize or delete data
    BEGIN TRANSACTION;
        UPDATE customers 
        SET name = 'REDACTED',
            email = CONCAT('redacted-', id, '@example.com'),
            phone = NULL,
            address = NULL
        WHERE email = subject_email;
        
        DELETE FROM marketing_consents WHERE email = subject_email;
        DELETE FROM user_sessions WHERE email = subject_email;
        
        -- Retain order history for legal/financial reasons but anonymize
        UPDATE orders 
        SET customer_email = CONCAT('redacted-', customer_id, '@example.com')
        WHERE customer_email = subject_email;
    COMMIT;
END;

-- Right to Data Portability
CREATE PROCEDURE gdpr_export_subject_data(subject_email VARCHAR)
BEGIN
    -- Export in machine-readable format (JSON)
    SELECT JSON_OBJECT(
        'personal_data', (
            SELECT JSON_OBJECT(
                'name', name,
                'email', email,
                'phone', phone,
                'address', address,
                'created_at', created_at
            )
            FROM customers WHERE email = subject_email
        ),
        'orders', (
            SELECT JSON_ARRAYAGG(
                JSON_OBJECT(
                    'order_id', order_id,
                    'order_date', order_date,
                    'total', total
                )
            )
            FROM orders WHERE customer_email = subject_email
        ),
        'consents', (
            SELECT JSON_ARRAYAGG(
                JSON_OBJECT(
                    'type', consent_type,
                    'granted', granted,
                    'granted_at', granted_at
                )
            )
            FROM marketing_consents WHERE email = subject_email
        )
    ) as subject_data;
END;
```

#### Data Retention Policies

```sql
-- Define retention policy
CREATE RETENTION POLICY customer_data
ON customers
RETENTION PERIOD 7 YEARS
AFTER deletion_requested_at
CASCADE TO orders, transactions;

-- Automated cleanup job
CREATE SCHEDULED JOB gdpr_cleanup
SCHEDULE 'EVERY 1 DAY'
EXECUTE PROCEDURE
BEGIN
    -- Delete data past retention period
    DELETE FROM customers
    WHERE deletion_requested_at IS NOT NULL
      AND deletion_requested_at < DATE_SUB(NOW(), INTERVAL 7 YEAR);
    
    -- Log compliance
    INSERT INTO compliance_log (action, records_deleted, executed_at)
    VALUES ('gdpr_retention_cleanup', ROW_COUNT(), NOW());
END;
```

### HIPAA Compliance

#### Access Controls
```sql
-- Minimum necessary principle
CREATE VIEW patient_summary FOR ROLE nurse AS
    SELECT patient_id, name, date_of_birth, current_medications
    FROM patients;  -- Limited fields

CREATE VIEW patient_full_record FOR ROLE doctor AS
    SELECT * FROM patients;  -- Full access

-- Break-glass access for emergencies
CREATE PROCEDURE emergency_access(patient_id INT, reason VARCHAR)
BEGIN
    -- Log emergency access
    INSERT INTO emergency_access_log (
        user_id, patient_id, reason, timestamp
    ) VALUES (
        CURRENT_USER_ID(), patient_id, reason, NOW()
    );
    
    -- Notify compliance team
    NOTIFY 'hipaa-alerts' 
    PAYLOAD JSON_OBJECT(
        'user', CURRENT_USER(),
        'patient', patient_id,
        'reason', reason
    );
    
    -- Grant temporary access
    GRANT SELECT ON patients TO CURRENT_USER()
    WITH DURATION INTERVAL 1 HOUR;
END;
```

#### Audit Requirements
```sql
-- HIPAA-compliant audit trail
CREATE TABLE hipaa_audit_log (
    id BIGINT PRIMARY KEY AUTO_INCREMENT,
    timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    user_id INT NOT NULL,
    user_name VARCHAR(100),
    action VARCHAR(50),
    resource_type VARCHAR(50),
    resource_id VARCHAR(100),
    patient_id INT,
    phi_accessed BOOLEAN,
    ip_address VARCHAR(45),
    workstation VARCHAR(100),
    success BOOLEAN,
    failure_reason VARCHAR(500),
    INDEX idx_patient (patient_id),
    INDEX idx_timestamp (timestamp),
    INDEX idx_user (user_id)
) RETENTION PERIOD 6 YEARS;
```

### SOC 2 Compliance

#### Monitoring and Alerting
```python
#!/usr/bin/env python3
# soc2-monitoring.py

import logging
from datetime import datetime, timedelta

def monitor_soc2_controls():
    """Monitor SOC 2 security controls"""
    
    # Control: Privileged access monitoring
    check_privileged_access_changes()
    
    # Control: Authentication failures
    check_authentication_anomalies()
    
    # Control: Data access patterns
    check_unusual_data_access()
    
    # Control: Backup verification
    check_backup_completion()
    
    # Control: Encryption status
    check_encryption_compliance()

def check_privileged_access_changes():
    """Detect changes to privileged accounts"""
    query = """
        SELECT user, action, timestamp
        FROM audit_log
        WHERE action IN ('GRANT', 'REVOKE', 'CREATE USER', 'DROP USER')
          AND timestamp > NOW() - INTERVAL 1 HOUR
    """
    changes = execute_query(query)
    
    if changes:
        alert_soc2_violation(
            control="AC-2.1",
            description=f"{len(changes)} privileged access changes detected",
            severity="HIGH",
            changes=changes
        )

def check_authentication_anomalies():
    """Detect unusual authentication patterns"""
    query = """
        SELECT user, COUNT(*) as failed_attempts, MIN(timestamp) as first_attempt
        FROM audit_log
        WHERE event_type = 'authentication'
          AND success = false
          AND timestamp > NOW() - INTERVAL 15 MINUTE
        GROUP BY user
        HAVING failed_attempts >= 5
    """
    anomalies = execute_query(query)
    
    for anomaly in anomalies:
        alert_soc2_violation(
            control="AC-7",
            description=f"Multiple failed auth attempts for user {anomaly['user']}",
            severity="MEDIUM",
            details=anomaly
        )

def check_unusual_data_access():
    """Detect abnormal data access patterns"""
    query = """
        WITH user_baseline AS (
            SELECT user, 
                   AVG(rows_returned) as avg_rows,
                   STDDEV(rows_returned) as stddev_rows
            FROM audit_log
            WHERE event_type = 'query'
              AND sensitive_data = true
              AND timestamp > NOW() - INTERVAL 30 DAY
            GROUP BY user
        )
        SELECT a.user, a.rows_returned, b.avg_rows, b.stddev_rows
        FROM audit_log a
        JOIN user_baseline b ON a.user = b.user
        WHERE a.rows_returned > (b.avg_rows + 3 * b.stddev_rows)
          AND a.timestamp > NOW() - INTERVAL 1 HOUR
    """
    anomalies = execute_query(query)
    
    if anomalies:
        alert_soc2_violation(
            control="CC6.6",
            description="Unusual data access volume detected",
            severity="HIGH",
            anomalies=anomalies
        )
```

---

## Security Hardening

### Operating System Hardening

```bash
#!/bin/bash
# os-hardening.sh

# Disable unnecessary services
systemctl disable bluetooth
systemctl disable cups
systemctl disable avahi-daemon

# Configure firewall
ufw default deny incoming
ufw default allow outgoing
ufw allow from 10.0.0.0/8 to any port 8529  # ThemisDB
ufw allow from 10.0.0.0/8 to any port 22    # SSH from internal
ufw enable

# Harden SSH
cat >> /etc/ssh/sshd_config <<EOF
PermitRootLogin no
PasswordAuthentication no
PubkeyAuthentication yes
MaxAuthTries 3
ClientAliveInterval 300
ClientAliveCountMax 2
AllowUsers themisdb-admin
EOF

systemctl restart sshd

# Configure SELinux / AppArmor
setenforce 1
cat > /etc/selinux/config <<EOF
SELINUX=enforcing
SELINUXTYPE=targeted
EOF

# Kernel hardening
cat >> /etc/sysctl.d/99-themisdb-hardening.conf <<EOF
# IP forwarding
net.ipv4.ip_forward = 0
net.ipv6.conf.all.forwarding = 0

# SYN flood protection
net.ipv4.tcp_syncookies = 1
net.ipv4.tcp_syn_retries = 2
net.ipv4.tcp_synack_retries = 2
net.ipv4.tcp_max_syn_backlog = 4096

# IP spoofing protection
net.ipv4.conf.all.rp_filter = 1
net.ipv4.conf.default.rp_filter = 1

# Ignore ICMP redirects
net.ipv4.conf.all.accept_redirects = 0
net.ipv6.conf.all.accept_redirects = 0

# Disable source packet routing
net.ipv4.conf.all.accept_source_route = 0
net.ipv6.conf.all.accept_source_route = 0

# Log suspicious packets
net.ipv4.conf.all.log_martians = 1
EOF

sysctl -p /etc/sysctl.d/99-themisdb-hardening.conf

# File system hardening
chmod 700 /var/lib/themisdb
chmod 700 /etc/themisdb
chmod 600 /etc/themisdb/*.conf
chown -R themisdb:themisdb /var/lib/themisdb
chown -R root:themisdb /etc/themisdb

# Remove unnecessary packages
apt-get remove --purge telnet ftp rsh-client

# Enable automatic security updates
apt-get install unattended-upgrades
dpkg-reconfigure -plow unattended-upgrades
```

### Network Segmentation

```
┌─────────────────────────────────────────────────┐
│                  Internet                       │
└─────────────────┬───────────────────────────────┘
                  │
┌─────────────────▼───────────────────────────────┐
│           DMZ (Firewall)                        │
│  - Web Application Firewall                     │
│  - Load Balancer                                │
└─────────────────┬───────────────────────────────┘
                  │
┌─────────────────▼───────────────────────────────┐
│        Application Tier (Private Subnet)        │
│  - Application Servers                          │
│  - API Gateway                                  │
│  - No direct internet access                    │
└─────────────────┬───────────────────────────────┘
                  │
┌─────────────────▼───────────────────────────────┐
│        Database Tier (Isolated Subnet)          │
│  - ThemisDB Cluster                             │
│  - Only accessible from application tier        │
│  - No outbound internet access                  │
│  - Monitoring/backup via bastion host           │
└─────────────────────────────────────────────────┘
```

### Vulnerability Assessment

```bash
#!/bin/bash
# vulnerability-scan.sh

# Scan for known vulnerabilities
lynis audit system

# Check for outdated packages
apt list --upgradable | grep -i security

# Scan open ports
nmap -sV -sC localhost

# Check SSL/TLS configuration
nmap --script ssl-enum-ciphers -p 8529 localhost

# Database-specific checks
themisdb-security-audit \
  --check-permissions \
  --check-encryption \
  --check-audit-config \
  --check-weak-passwords \
  --output-format json \
  --output-file /var/log/themisdb/security-audit.json

# Parse results and alert
python3 << EOF
import json
with open('/var/log/themisdb/security-audit.json') as f:
    results = json.load(f)
    
high_risks = [r for r in results['findings'] if r['severity'] == 'HIGH']
if high_risks:
    print(f"⚠️  {len(high_risks)} HIGH severity issues found!")
    for risk in high_risks:
        print(f"  - {risk['description']}")
EOF
```

---

## Incident Response

### Detection and Alerting

```python
#!/usr/bin/env python3
# incident-detection.py

import re
from datetime import datetime, timedelta

class IncidentDetector:
    def __init__(self):
        self.alerts = []
    
    def detect_sql_injection(self, query_log):
        """Detect potential SQL injection attempts"""
        sql_injection_patterns = [
            r"'\s*(OR|AND)\s*'1'\s*=\s*'1",
            r"UNION\s+SELECT",
            r";\s*DROP\s+TABLE",
            r"EXEC\s*\(",
            r"<script>",
        ]
        
        for entry in query_log:
            query = entry['query']
            for pattern in sql_injection_patterns:
                if re.search(pattern, query, re.IGNORECASE):
                    self.raise_alert({
                        'type': 'SQL_INJECTION_ATTEMPT',
                        'severity': 'CRITICAL',
                        'user': entry['user'],
                        'query': query,
                        'ip': entry['source_ip'],
                        'timestamp': entry['timestamp']
                    })
    
    def detect_privilege_escalation(self, audit_log):
        """Detect privilege escalation attempts"""
        escalation_events = [
            'GRANT', 'CREATE USER', 'ALTER USER', 'DROP USER'
        ]
        
        # Check for unauthorized privilege changes
        for entry in audit_log:
            if entry['action'] in escalation_events:
                if not self.is_authorized_admin(entry['user']):
                    self.raise_alert({
                        'type': 'PRIVILEGE_ESCALATION',
                        'severity': 'CRITICAL',
                        'user': entry['user'],
                        'action': entry['action'],
                        'target': entry.get('target_user'),
                        'timestamp': entry['timestamp']
                    })
    
    def detect_data_exfiltration(self, query_log):
        """Detect potential data exfiltration"""
        threshold_rows = 10000
        threshold_queries = 100
        time_window = timedelta(minutes=15)
        
        # Group queries by user and time window
        user_activity = {}
        for entry in query_log:
            user = entry['user']
            if user not in user_activity:
                user_activity[user] = {
                    'queries': 0,
                    'rows_returned': 0,
                    'first_query': entry['timestamp']
                }
            
            user_activity[user]['queries'] += 1
            user_activity[user]['rows_returned'] += entry['rows_returned']
        
        # Check for anomalies
        for user, activity in user_activity.items():
            if (activity['rows_returned'] > threshold_rows or
                activity['queries'] > threshold_queries):
                self.raise_alert({
                    'type': 'POTENTIAL_DATA_EXFILTRATION',
                    'severity': 'HIGH',
                    'user': user,
                    'queries': activity['queries'],
                    'rows_returned': activity['rows_returned'],
                    'time_window': str(time_window)
                })
    
    def detect_brute_force(self, auth_log):
        """Detect brute force attacks"""
        failed_attempts = {}
        threshold = 10
        time_window = timedelta(minutes=5)
        
        for entry in auth_log:
            if not entry['success']:
                ip = entry['source_ip']
                if ip not in failed_attempts:
                    failed_attempts[ip] = []
                failed_attempts[ip].append(entry['timestamp'])
        
        # Check for rapid failed attempts
        for ip, attempts in failed_attempts.items():
            recent_attempts = [
                t for t in attempts 
                if datetime.now() - t < time_window
            ]
            
            if len(recent_attempts) >= threshold:
                self.raise_alert({
                    'type': 'BRUTE_FORCE_ATTACK',
                    'severity': 'HIGH',
                    'source_ip': ip,
                    'attempts': len(recent_attempts),
                    'time_window': str(time_window)
                })
                
                # Automatically block IP
                self.block_ip(ip)
    
    def raise_alert(self, alert):
        """Raise security alert"""
        self.alerts.append(alert)
        
        # Send to SIEM
        self.send_to_siem(alert)
        
        # Notify security team
        if alert['severity'] in ['CRITICAL', 'HIGH']:
            self.notify_security_team(alert)
        
        # Log to audit trail
        self.log_security_event(alert)
    
    def send_to_siem(self, alert):
        """Send alert to SIEM system"""
        # Implementation depends on SIEM system
        pass
    
    def notify_security_team(self, alert):
        """Notify security team via PagerDuty/Slack"""
        # Implementation
        pass
    
    def log_security_event(self, alert):
        """Log to tamper-proof audit log"""
        # Implementation
        pass
    
    def block_ip(self, ip):
        """Block IP address in firewall"""
        import subprocess
        subprocess.run(['ufw', 'deny', 'from', ip])
```

### Incident Response Playbook

```markdown
# Security Incident Response Playbook

## Severity Classification

### Critical (P0)
- Data breach
- Ransomware
- Complete system compromise
- Privilege escalation by external attacker

**Response Time**: Immediate (< 15 minutes)

### High (P1)
- SQL injection attempts
- Unauthorized access
- DDoS attack
- Suspicious data exfiltration

**Response Time**: < 1 hour

### Medium (P2)
- Brute force attempts
- Configuration vulnerabilities
- Audit log anomalies

**Response Time**: < 4 hours

### Low (P3)
- Failed login attempts (below threshold)
- Minor policy violations

**Response Time**: < 24 hours

## Response Procedures

### Phase 1: Detection and Analysis (0-30 minutes)

1. **Receive Alert**
   - SIEM alert
   - Monitoring system
   - User report

2. **Initial Assessment**
   - Verify incident is real (not false positive)
   - Determine severity level
   - Identify affected systems

3. **Activate Response Team**
   - Notify incident commander
   - Assemble response team
   - Establish communication channel

### Phase 2: Containment (30-60 minutes)

**Short-term Containment**
1. Isolate affected systems
2. Block malicious IPs
3. Disable compromised accounts
4. Preserve evidence

**Long-term Containment**
1. Apply security patches
2. Change compromised credentials
3. Implement additional monitoring
4. Update firewall rules

### Phase 3: Eradication (1-4 hours)

1. Remove malware/backdoors
2. Close security vulnerabilities
3. Patch systems
4. Reset passwords
5. Revoke compromised certificates

### Phase 4: Recovery (4-24 hours)

1. Restore systems from clean backups
2. Verify system integrity
3. Monitor for re-infection
4. Gradually restore services
5. Update security controls

### Phase 5: Post-Incident (1-7 days)

1. Conduct post-mortem
2. Document lessons learned
3. Update security policies
4. Improve detection rules
5. Train staff on findings
6. Report to management/regulators if required

## Communication Templates

### Internal Notification
```
SECURITY INCIDENT ALERT

Severity: [CRITICAL/HIGH/MEDIUM/LOW]
Incident ID: INC-2025-001
Time Detected: 2025-01-24 10:30 UTC
Affected Systems: [List]
Current Status: [Under Investigation/Contained/Resolved]

Description:
[Brief description of incident]

Actions Taken:
- [Action 1]
- [Action 2]

Next Steps:
- [Next step 1]
- [Next step 2]

Incident Commander: [Name]
Contact: [Email/Phone]
```

### Customer Notification (Data Breach)
```
Security Incident Notification

Dear [Customer],

We are writing to inform you of a security incident that may have affected your data.

What Happened:
[Clear, concise description]

What Information Was Involved:
[Specific data types]

What We're Doing:
[Steps taken to address]

What You Should Do:
[Recommended actions]

How to Get More Information:
[Contact details]

We sincerely apologize for any inconvenience this may cause.

[Company Name]
[Date]
```
```

---

## Sample Exam Questions

### Section 1: Authentication

**Question 1**: Which authentication method provides the strongest security?
- A) Username and password
- B) Multi-factor authentication
- C) API keys
- D) IP whitelisting

**Answer**: B

---

**Question 2**: What is the purpose of SAML?
- A) Database encryption
- B) Single sign-on federation
- C) Password hashing
- D) Network security

**Answer**: B

---

### Section 2: Encryption

**Question 3**: What is the minimum recommended TLS version?
- A) TLS 1.0
- B) TLS 1.1
- C) TLS 1.2
- D) TLS 1.3

**Answer**: C (minimum), D (recommended)

---

**Question 4**: Where should encryption keys be stored?
- A) In database configuration files
- B) In application code
- C) In a dedicated key management system
- D) In environment variables

**Answer**: C

---

### Section 3: Compliance

**Question 5**: How long must HIPAA audit logs be retained?
- A) 1 year
- B) 3 years
- C) 6 years
- D) 10 years

**Answer**: C

---

**Question 6**: What is a data subject access request (DSAR) under GDPR?
- A) Database query optimization
- B) Individual's right to access their personal data
- C) Administrator access control
- D) Data backup procedure

**Answer**: B

---

### Section 4: Incident Response

**Question 7**: What is the first step in incident response?
- A) Eradication
- B) Recovery
- C) Detection and analysis
- D) Post-incident review

**Answer**: C

---

**Question 8**: How long should evidence be preserved after a security incident?
- A) Until incident is resolved
- B) 30 days
- C) As required by legal/regulatory requirements
- D) Evidence is not necessary

**Answer**: C

---

## Security Audit Project

### Overview
Conduct a comprehensive security audit of a ThemisDB deployment and create a remediation plan.

### Requirements

**Part 1: Security Assessment (30%)**
- Authentication and authorization review
- Encryption configuration audit
- Access control evaluation
- Network security assessment
- Vulnerability scan results

**Part 2: Compliance Mapping (25%)**
- Map controls to GDPR requirements
- Identify HIPAA compliance gaps
- Assess SOC 2 readiness
- Document compliance status

**Part 3: Remediation Plan (25%)**
- Prioritize security findings
- Develop remediation roadmap
- Cost-benefit analysis
- Implementation timeline

**Part 4: Incident Response (20%)**
- Develop incident response procedures
- Create runbooks for common scenarios
- Design detection rules
- Test incident response plan

### Deliverables
1. Security audit report (20-30 pages)
2. Risk register
3. Remediation roadmap
4. Incident response playbook
5. Executive summary presentation

### Evaluation Criteria
- Thoroughness of assessment
- Quality of analysis
- Feasibility of recommendations
- Compliance knowledge
- Presentation clarity

---

## Certification Benefits

- Expert-level security designation
- Average 30% salary increase
- CISO/Security architect opportunities
- Industry recognition
- Conference speaking opportunities
- Consulting opportunities

---

## Support

**Security Support**: security-cert@themisdb.com  
**Compliance Questions**: compliance@themisdb.com  
**Project Help**: security-audit@themisdb.com

---

[Register for TSC Certification →](https://certify.themisdb.com/register/tsc)

---

*Last Updated: April 2026*  
*Version: 1.0*  
*© 2025 ThemisDB. All rights reserved.*
