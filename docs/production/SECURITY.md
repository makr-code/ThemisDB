# Security Hardening Guide

**Version:** 1.8.0-rc1  
**Last Updated:** April 2026  
**Target Audience:** Security Engineers, DevOps

## Table of Contents

1. [Overview](#overview)
2. [GPU Access Control](#gpu-access-control)
3. [Encryption at Rest](#encryption-at-rest)
4. [Encryption in Transit](#encryption-in-transit)
5. [Audit Logging](#audit-logging)
6. [HSM Integration](#hsm-integration)
7. [Key Rotation](#key-rotation)
8. [VRAM Security](#vram-security)
9. [Compliance](#compliance)

---

## Overview

This guide covers security hardening for GPU-accelerated ThemisDB deployments. Production systems must implement defense-in-depth with multiple security layers.

### Security Principles

1. **Least Privilege**: Minimal permissions for all users/processes
2. **Defense in Depth**: Multiple security layers
3. **Encryption Everywhere**: Data at rest and in transit
4. **Audit Everything**: Comprehensive logging
5. **Assume Breach**: Design for compromise scenarios

---

## GPU Access Control

### User-Level GPU Restrictions

**Linux cgroups for GPU isolation:**

```bash
# Create GPU cgroup
sudo cgcreate -g devices:/gpu_users

# Allow specific GPUs only
sudo cgset -r devices.allow="c 195:0 rwm" gpu_users  # GPU 0
sudo cgset -r devices.allow="c 195:1 rwm" gpu_users  # GPU 1

# Deny other GPUs
sudo cgset -r devices.deny="c 195:* rwm" gpu_users

# Assign users to cgroup
sudo cgclassify -g devices:gpu_users $(pgrep -u themisdb_user)
```

### Process-Level GPU Isolation

**Configure in** `config.yaml`:

```yaml
security:
  gpu:
    # Enable GPU access control
    access_control:
      enabled: true
      mode: strict  # strict, permissive, audit
      
      # Per-user GPU allocation
      user_gpu_mapping:
        ml_user1: [0, 1]
        ml_user2: [2, 3]
        admin: [0, 1, 2, 3]
      
      # Process limits
      per_process_limit:
        max_vram_mb: 8192
        max_gpu_utilization_percent: 50
      
    # Compute mode restrictions
    compute_mode: exclusive_process  # default, exclusive_thread, prohibited
    
    # GPU operations whitelist
    allowed_operations:
      - training
      - inference
      # - mining  # BLOCKED
```

### GPU Device Permissions

```bash
# Set GPU device permissions
sudo chmod 660 /dev/nvidia*
sudo chown root:gpu-users /dev/nvidia*

# Add users to GPU group
sudo usermod -aG gpu-users themisdb

# Verify permissions
ls -la /dev/nvidia*
```

### Secure MIG Mode (Multi-Instance GPU)

For A100/H100 GPUs:

```bash
# Enable MIG mode
sudo nvidia-smi -mig 1

# Create GPU instances (7 instances on A100)
sudo nvidia-smi mig -cgi 1g.5gb,1g.5gb,1g.5gb,1g.5gb,1g.5gb,1g.5gb,1g.5gb

# Create compute instances
sudo nvidia-smi mig -cci

# Assign to users
cat >> /etc/themisdb/mig.yaml << 'EOF'
mig:
  enabled: true
  instances:
    - uuid: MIG-GPU-12345678
      users: [user1]
      vram_limit: 5gb
    - uuid: MIG-GPU-23456789
      users: [user2]
      vram_limit: 5gb
EOF
```

---

## Encryption at Rest

### Full Disk Encryption

**LUKS encryption for data drives:**

```bash
# Encrypt partition
sudo cryptsetup luksFormat /dev/nvme0n1
sudo cryptsetup luksOpen /dev/nvme0n1 themisdb_data

# Format and mount
sudo mkfs.ext4 /dev/mapper/themisdb_data
sudo mkdir -p /data/themisdb
sudo mount /dev/mapper/themisdb_data /data/themisdb

# Auto-mount with key file (secure location)
echo "themisdb_data UUID=$(blkid -s UUID -o value /dev/nvme0n1) /etc/keys/themisdb.key luks" | \
  sudo tee -a /etc/crypttab

sudo mkdir -p /etc/keys
sudo dd if=/dev/urandom of=/etc/keys/themisdb.key bs=1024 count=4
sudo chmod 600 /etc/keys/themisdb.key
sudo cryptsetup luksAddKey /dev/nvme0n1 /etc/keys/themisdb.key
```

### Application-Level Encryption

**Configure in** `config.yaml`:

```yaml
storage:
  encryption:
    enabled: true
    
    # Encryption algorithm
    algorithm: AES-256-GCM
    
    # Key derivation
    kdf: argon2id
    kdf_params:
      memory_cost: 65536  # 64 MB
      time_cost: 3
      parallelism: 4
    
    # Key provider
    key_provider: vault  # file, env, vault, hsm
    
    # Encryption scope
    encrypt_data: true
    encrypt_indexes: false  # Performance tradeoff
    encrypt_checkpoints: true
    encrypt_logs: true
    
    # Per-field encryption (for sensitive data)
    field_level:
      enabled: true
      sensitive_fields:
        - user_email
        - api_keys
        - model_weights  # Optional: encrypt model weights
```

### Model Encryption

```bash
# Encrypt model files
themisdb-cli model encrypt \
  --input /models/llama-2-7b.gguf \
  --output /models/encrypted/llama-2-7b.enc \
  --key-id production-key-001

# Decrypt at runtime (automatic)
llm:
  model_path: /models/encrypted/llama-2-7b.enc
  encryption:
    enabled: true
    key_provider: vault
    key_id: production-key-001
```

### Checkpoint Encryption

```yaml
training:
  checkpoint:
    encryption:
      enabled: true
      algorithm: AES-256-GCM
      compress_before_encrypt: true  # Smaller size
      
      # Sign checkpoints
      signing:
        enabled: true
        algorithm: RSA-4096
        key_id: checkpoint-signing-key
```

---

## Encryption in Transit

### TLS 1.3 Configuration

**Generate certificates:**

```bash
# Self-signed (development only)
openssl req -x509 -newkey rsa:4096 \
  -keyout /etc/themisdb/certs/server.key \
  -out /etc/themisdb/certs/server.crt \
  -days 365 -nodes \
  -subj "/CN=themisdb.example.com"

# Production: Use Let's Encrypt or internal CA
certbot certonly --standalone -d themisdb.example.com
```

**Configure TLS in** `config.yaml`:

```yaml
network:
  tls:
    enabled: true
    
    # TLS version (only 1.3)
    min_version: "1.3"
    max_version: "1.3"
    
    # Certificate paths
    cert_file: /etc/themisdb/certs/server.crt
    key_file: /etc/themisdb/certs/server.key
    ca_file: /etc/themisdb/certs/ca.crt
    
    # Cipher suites (TLS 1.3)
    cipher_suites:
      - TLS_AES_256_GCM_SHA384
      - TLS_CHACHA20_POLY1305_SHA256
      - TLS_AES_128_GCM_SHA256
    
    # Certificate validation
    verify_client_cert: true
    client_ca_file: /etc/themisdb/certs/client-ca.crt
    
    # OCSP stapling
    ocsp_stapling: true
    
    # Session tickets
    session_tickets: false  # Better forward secrecy
```

### Mutual TLS (mTLS)

**Generate client certificates:**

```bash
# Create client key and CSR
openssl genrsa -out client.key 4096
openssl req -new -key client.key -out client.csr \
  -subj "/CN=client-001/O=ThemisDB/C=US"

# Sign with CA
openssl x509 -req -in client.csr \
  -CA /etc/themisdb/certs/ca.crt \
  -CAkey /etc/themisdb/certs/ca.key \
  -CAcreateserial \
  -out client.crt \
  -days 365 -sha256
```

**Configure mTLS:**

```yaml
network:
  tls:
    # Require client certificates
    client_auth: required  # required, optional, none
    
    # Client certificate validation
    client_cert_verification:
      verify_cn: true
      allowed_cn_patterns:
        - "client-*"
        - "service-*"
      
      verify_organization: true
      allowed_organizations:
        - ThemisDB
        - TrustedPartner
      
      # Certificate revocation
      crl_check: true
      crl_url: http://crl.example.com/themisdb.crl
```

### gRPC Security

```yaml
grpc:
  tls:
    enabled: true
    cert_file: /etc/themisdb/certs/grpc-server.crt
    key_file: /etc/themisdb/certs/grpc-server.key
    
    # ALPN for HTTP/2
    alpn_protocols:
      - h2
      - grpc-exp
    
    # Client authentication
    client_auth: required
```

### Network Segmentation

```bash
# Separate networks for different traffic
network:
  management:
    interface: eth0
    cidr: 10.0.1.0/24
    
  gpu_data:
    interface: ib0  # InfiniBand
    cidr: 10.0.2.0/24
    
  client_api:
    interface: eth1
    cidr: 10.0.3.0/24

# Firewall rules
sudo iptables -A INPUT -i eth0 -s 10.0.1.0/24 -p tcp --dport 22 -j ACCEPT  # SSH
sudo iptables -A INPUT -i eth1 -s 10.0.3.0/24 -p tcp --dport 8080 -j ACCEPT  # API
sudo iptables -A INPUT -j DROP  # Default deny
```

---

## Audit Logging

### Comprehensive Audit Configuration

```yaml
logging:
  audit:
    enabled: true
    level: info  # debug, info, warn, error
    
    # Output destinations
    outputs:
      - type: file
        path: /var/log/themisdb/audit.log
        format: json
        
      - type: syslog
        server: syslog.example.com:514
        protocol: tcp
        facility: local0
        
      - type: elasticsearch
        url: https://es.example.com:9200
        index: themisdb-audit
        
      - type: splunk
        url: https://splunk.example.com:8088
        token: "${SPLUNK_HEC_TOKEN}"
    
    # Rotation
    rotation:
      max_size: 100MB
      max_age: 90  # days
      max_backups: 100
      compress: true
    
    # Events to log
    events:
      authentication:
        - login_success
        - login_failure
        - logout
        - session_timeout
        
      authorization:
        - permission_granted
        - permission_denied
        - role_change
        
      data_access:
        - read
        - write
        - delete
        - export
        
      gpu_operations:
        - gpu_allocation
        - gpu_release
        - model_load
        - training_start
        - training_stop
        
      administrative:
        - config_change
        - user_created
        - user_deleted
        - key_rotation
        
      security:
        - failed_tls_handshake
        - invalid_certificate
        - suspicious_activity
        - rate_limit_exceeded
    
    # Sensitive data handling
    redaction:
      enabled: true
      fields:
        - password
        - api_key
        - private_key
        - credit_card
      
      patterns:
        - regex: '\b\d{16}\b'  # Credit card numbers
          replacement: '[REDACTED-CC]'
        - regex: '\b[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Z|a-z]{2,}\b'
          replacement: '[REDACTED-EMAIL]'
```

### Audit Log Analysis

```bash
# Search audit logs
themisdb-cli audit query \
  --event-type "login_failure" \
  --user "admin" \
  --start "2026-01-17 00:00:00" \
  --end "2026-01-17 23:59:59"

# Detect anomalies
themisdb-cli audit analyze \
  --anomaly-detection \
  --sensitivity high \
  --output /tmp/anomalies.json

# Generate compliance report
themisdb-cli audit report \
  --type compliance \
  --standard SOC2 \
  --period "last-month" \
  --output /reports/soc2-$(date +%Y%m).pdf
```

### Tamper-Proof Logging

```yaml
logging:
  audit:
    # Immutable logging
    immutable:
      enabled: true
      backend: blockchain  # or: append-only-log
      
      # Blockchain configuration
      blockchain:
        type: hyperledger
        endpoint: http://blockchain.example.com:7050
        channel: themisdb-audit
        
      # Digital signatures
      signing:
        enabled: true
        algorithm: ECDSA-P256
        key_id: audit-signing-key
    
    # Forward to write-once storage
    worm_storage:
      enabled: true
      endpoint: s3://audit-worm-bucket/
      retention_period: 2555  # 7 years
```

---

## HSM Integration

### Hardware Security Module Setup

**AWS CloudHSM:**

```yaml
security:
  hsm:
    enabled: true
    provider: aws_cloudhsm
    
    # CloudHSM cluster
    cluster_id: cluster-abcd1234
    
    # PKCS#11 library
    pkcs11_library: /opt/cloudhsm/lib/libcloudhsm_pkcs11.so
    
    # Credentials
    user: crypto_user
    password: "${HSM_PASSWORD}"  # From environment
    
    # Key management
    keys:
      master_key:
        label: themisdb-master-key
        type: AES-256
        extractable: false
        
      signing_key:
        label: themisdb-signing-key
        type: RSA-4096
        extractable: false
```

**Thales Luna HSM:**

```yaml
security:
  hsm:
    enabled: true
    provider: thales_luna
    
    # Luna configuration
    partition: themisdb_partition
    
    # HA configuration
    ha_group: themisdb_ha
    members:
      - hsm1.example.com
      - hsm2.example.com
      - hsm3.example.com
    
    # Key storage
    key_handles:
      master_key: 0x00010001
      signing_key: 0x00010002
```

### Key Operations with HSM

```bash
# Generate key in HSM
themisdb-cli hsm generate-key \
  --label themisdb-master-key \
  --type AES-256 \
  --attributes non-extractable

# Encrypt data with HSM key
themisdb-cli encrypt \
  --input /data/sensitive.dat \
  --output /data/sensitive.enc \
  --key-label themisdb-master-key \
  --hsm

# Sign with HSM
themisdb-cli sign \
  --input /data/checkpoint.bin \
  --output /data/checkpoint.sig \
  --key-label themisdb-signing-key \
  --hsm
```

---

## Key Rotation

### Automated Key Rotation

```yaml
security:
  key_rotation:
    enabled: true
    
    # Rotation schedule
    schedules:
      master_key:
        interval: 90  # days
        advance_notice: 7  # days
        
      tls_certificates:
        interval: 365  # days
        advance_notice: 30  # days
        
      api_keys:
        interval: 30  # days
        advance_notice: 3  # days
    
    # Rotation strategy
    strategy: blue_green  # immediate, blue_green, gradual
    
    # Notification
    notifications:
      email: security@example.com
      slack_webhook: https://hooks.slack.com/services/XXX
      advance_warning: true
```

### Manual Key Rotation

```bash
# Rotate master encryption key
themisdb-cli security rotate-key \
  --key-type master \
  --algorithm AES-256-GCM \
  --backup-old-key

# Re-encrypt data with new key
themisdb-cli security re-encrypt \
  --old-key-id key-001 \
  --new-key-id key-002 \
  --verify

# Update references
themisdb-cli security update-key-refs \
  --old-key-id key-001 \
  --new-key-id key-002

# Retire old key (after grace period)
themisdb-cli security retire-key \
  --key-id key-001 \
  --archive
```

### Key Rotation Procedure

```bash
# 1. Generate new key
NEW_KEY=$(themisdb-cli hsm generate-key --label master-key-v2)

# 2. Dual-key period (both keys active)
themisdb-cli config set security.active_keys="[master-key-v1,master-key-v2]"

# 3. Re-encrypt in background
themisdb-cli security re-encrypt-background \
  --from master-key-v1 \
  --to master-key-v2 \
  --verify

# 4. Monitor progress
themisdb-cli security re-encrypt-status

# 5. Switch to new key
themisdb-cli config set security.active_keys="[master-key-v2]"

# 6. Archive old key
themisdb-cli security archive-key --key-id master-key-v1
```

---

## VRAM Security

### Secure VRAM Clearing

```yaml
security:
  vram:
    # Clear VRAM after use
    secure_clear:
      enabled: true
      method: overwrite  # overwrite, zero, random
      passes: 3
      
    # Scrub on job completion
    scrub_on_exit: true
    
    # Prevent data leakage between jobs
    isolation:
      enabled: true
      mode: strict  # strict, standard, permissive
```

### ECC Memory

```bash
# Enable ECC (on supported GPUs)
sudo nvidia-smi -e 1

# Check ECC status
nvidia-smi --query-gpu=ecc.mode.current --format=csv

# Monitor ECC errors
nvidia-smi --query-gpu=ecc.errors.corrected.aggregate.total,ecc.errors.uncorrected.aggregate.total --format=csv
```

### VRAM Encryption

For GPUs with VRAM encryption support (H100+):

```yaml
security:
  vram_encryption:
    enabled: true
    key_provider: hsm
    key_id: vram-encryption-key
```

---

## Compliance

### SOC 2 Compliance

```yaml
compliance:
  soc2:
    enabled: true
    
    controls:
      # CC6.1: Logical access controls
      - access_control
      - multi_factor_auth
      - session_management
      
      # CC6.6: Encryption
      - data_at_rest_encryption
      - data_in_transit_encryption
      
      # CC7.2: Monitoring
      - audit_logging
      - anomaly_detection
      
    # Automated evidence collection
    evidence_collection:
      enabled: true
      output: /compliance/soc2/evidence/
      schedule: daily
```

### GDPR Compliance

```yaml
compliance:
  gdpr:
    enabled: true
    
    # Data subject rights
    data_subject_rights:
      - right_to_access
      - right_to_rectification
      - right_to_erasure
      - right_to_portability
      
    # Data processing
    data_processing:
      logging: comprehensive
      consent_tracking: enabled
      
    # Data retention
    retention:
      default_period: 365  # days
      auto_delete: true
```

### HIPAA Compliance

```yaml
compliance:
  hipaa:
    enabled: true
    
    # PHI protection
    phi_protection:
      encryption_required: true
      access_logging: comprehensive
      minimum_necessary: enforced
      
    # Audit controls
    audit:
      integrity_controls: enabled
      person_authentication: required
```

---

## Security Checklist

### Pre-Production Security Audit

- [ ] TLS 1.3 enforced
- [ ] mTLS configured
- [ ] Disk encryption enabled
- [ ] HSM integrated
- [ ] Key rotation automated
- [ ] Audit logging enabled
- [ ] GPU access controls configured
- [ ] VRAM secure clearing enabled
- [ ] Firewall rules applied
- [ ] Network segmentation implemented
- [ ] Intrusion detection configured
- [ ] Security monitoring active
- [ ] Incident response plan documented
- [ ] Compliance requirements met

---

## Security Incident Response

See [RUNBOOKS.md](RUNBOOKS.md#security-incident-response) for detailed incident response procedures.

---

## Next Steps

- **Checklists**: Use security checklists ([CHECKLISTS/pre_deployment.md](CHECKLISTS/pre_deployment.md))
- **Monitoring**: Set up security monitoring ([MONITORING.md](MONITORING.md))
- **Compliance**: Review compliance requirements

---

**Document Version:** 1.0  
**Last Updated:** April 2026  
**Next Review:** April 2026
