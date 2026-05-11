# HSM Production Setup Guide

**Version:** 1.4.2  
**Last Updated:** April 2026  
**Classification:** Public  
**Related:** FIND-002, Security Controls Audit

---

## Table of Contents

1. [Overview](#overview)
2. [Security Requirements](#security-requirements)
3. [Supported HSM Providers](#supported-hsm-providers)
4. [Quick Start](#quick-start)
5. [Provider-Specific Setup](#provider-specific-setup)
6. [Production Checklist](#production-checklist)
7. [Troubleshooting](#troubleshooting)
8. [Compliance Verification](#compliance-verification)

---

## Overview

ThemisDB requires Hardware Security Module (HSM) protection for master encryption keys in production environments. The default **stub provider** is intended for development only and **MUST NOT** be used in production.

### Why HSM is Required

| Risk | Impact Without HSM | HSM Protection |
|------|-------------------|----------------|
| **Key Exposure** | Master keys stored in plaintext or weakly encrypted | Keys never leave hardware boundary |
| **Compliance Violation** | Fails NIST, PCI DSS, GDPR requirements | Meets cryptographic key management standards |
| **Unauthorized Access** | Keys can be copied, stolen, or extracted | Hardware-backed access controls |
| **Audit Trail** | Limited or no cryptographic operation logging | Complete audit trail of key usage |

---

## Security Requirements

### Compliance Standards

Production deployments must comply with:

- **NIST SP 800-53 SC-12:** Cryptographic Key Establishment and Management
- **ISO 27001 A.8.24:** Use of Cryptography
- **PCI DSS Requirement 3.6:** Protect cryptographic keys
- **GDPR Article 32:** Security of Processing (encryption key protection)
- **HIPAA § 164.312(a)(2)(iv):** Encryption and Decryption

### Minimum Security Level

- **FIPS 140-2 Level 2** or higher for HSM devices
- **FIPS 140-2 Level 3** recommended for high-security environments
- Cloud KMS services (AWS KMS, Azure Key Vault, GCP KMS) meet these requirements

---

## Supported HSM Providers

### 1. PKCS#11 Hardware HSMs (Recommended for On-Premises)

| Provider | Model | FIPS Level | Use Case |
|----------|-------|------------|----------|
| **Thales** | Luna HSM 7 | Level 3 | Enterprise data centers |
| **Utimaco** | CryptoServer | Level 3 | Financial services |
| **AWS** | CloudHSM | Level 3 | AWS deployments |
| **nCipher** | nShield | Level 3 | Government, defense |
| **SoftHSM2** | Software | N/A | Testing ONLY |

### 2. Cloud Key Management Services

| Provider | Service | Certification | Use Case |
|----------|---------|---------------|----------|
| **AWS** | AWS KMS | FIPS 140-2 Level 2 | AWS cloud deployments |
| **Azure** | Key Vault (Premium) | FIPS 140-2 Level 2 | Azure cloud deployments |
| **GCP** | Cloud KMS (HSM) | FIPS 140-2 Level 3 | GCP cloud deployments |

### 3. Encryption-as-a-Service

| Provider | Service | Use Case |
|----------|---------|----------|
| **HashiCorp** | Vault Transit Engine | Multi-cloud, hybrid deployments |

---

## Quick Start

### Step 1: Choose Your HSM Provider

Select based on your deployment environment:

- **On-premises:** PKCS#11 HSM (Thales Luna, Utimaco)
- **AWS:** AWS KMS or CloudHSM
- **Azure:** Azure Key Vault (Premium tier)
- **GCP:** Cloud KMS with HSM backend
- **Multi-cloud:** HashiCorp Vault

### Step 2: Build with HSM Support

```bash
# Enable real HSM provider during build
cmake -DTHEMIS_ENABLE_HSM_REAL=ON \
      -DTHEMIS_USE_VENDOR_PKCS11=ON \  # Use vendor PKCS#11 headers (recommended)
      -DTHEMIS_ENABLE_SECURITY=ON \
      -DCMAKE_BUILD_TYPE=Release \
      ..
make -j$(nproc)
```

**Build Options:**
- `-DTHEMIS_ENABLE_HSM_REAL=ON` - Enable real HSM provider (vs stub)
- `-DTHEMIS_USE_VENDOR_PKCS11=ON` - Use vendor PKCS#11 headers (production)
- `-DPKCS11_INCLUDE_DIR=/path/to/headers` - Vendor header location (if needed)
- `-DPKCS11_LIBRARY=/path/to/lib.so` - PKCS#11 library path (if needed)

See: [PKCS#11 Integration Guide](PKCS11_INTEGRATION.md) for details on vendor headers.

### Step 3: Configure HSM Provider

Edit `config/security.yaml`:

```yaml
hsm:
  provider: pkcs11  # Or aws_kms, azure_keyvault, gcp_kms, vault
  
  pkcs11:
    library_path: "/usr/lib/libCryptoki2_64.so"  # Path to your HSM library
    slot_id: 0
    pin: ""  # Use THEMIS_HSM_PIN environment variable
    key_label: "themis-master-key"
```

### Step 4: Set HSM PIN Securely

**NEVER** commit PINs to configuration files. Use environment variables:

```bash
export THEMIS_HSM_PIN="your-secure-pin"
```

Or use secrets management:
- AWS Secrets Manager
- Azure Key Vault Secrets
- HashiCorp Vault
- Kubernetes Secrets

### Step 5: Verify Configuration

```bash
# Without HSM config and without explicit stub opt-in, startup now fails closed
# and prints an HSM startup policy error.

# Test mode (development) - requires explicit opt-in for stub
export THEMIS_ALLOW_HSM_STUB=1
./themis_server --config config/development.yaml

# Production mode (enforces real HSM, blocks stub)
export THEMIS_PRODUCTION_MODE=1
./themis_server --config config/production.yaml
```

**🛡️ NEW Security Gating (v1.4.2+):**

Production environments are now protected by security gating:
- ✅ Real HSM: Works normally
- ❌ Stub HSM without opt-in: **FAILS** with error
- ❌ Missing HSM config without opt-in: **FAILS** with error
- ⚠️ Stub HSM with opt-in: Works but logs ERROR warnings

**Environment Variables:**
- `THEMIS_PRODUCTION_MODE=1` - Forces production mode (stub always fails)
- `THEMIS_ALLOW_HSM_STUB=1` - Explicitly allows stub (dev only)
- `ENVIRONMENT=production` - Auto-detected as production (requires opt-in)

Expected output:
```
# With Real HSM:
[INFO] HSMProvider init (real_ready=true)
[INFO] HSM connected: Thales Luna SA-7
```

---

## Provider-Specific Setup

### PKCS#11 (Thales Luna HSM)

#### Prerequisites

1. Install Luna Client software:
```bash
# Download from Thales support portal
sudo dpkg -i LunaClient-10.x.x-xxx.x86_64.deb
```

2. Configure Luna client:
```bash
# Add HSM appliance
/usr/safenet/lunaclient/bin/vtl addServer -n luna-hsm.example.com -c luna-hsm.pem

# Verify connectivity
/usr/safenet/lunaclient/bin/vtl verify
```

3. Create partition and initialize:
```bash
# On Luna HSM appliance (via SSH)
lunash:> partition create -partition themis-prod
lunash:> partition init -partition themis-prod
```

4. Generate master key:
```bash
# Using pkcs11-tool
pkcs11-tool --module /usr/lib/libCryptoki2_64.so \
  --login --pin YOUR_PIN \
  --keypairgen --key-type RSA:4096 \
  --label "themis-master-key" \
  --id 01
```

#### Configuration

```yaml
hsm:
  provider: pkcs11
  pkcs11:
    library_path: "/usr/lib/libCryptoki2_64.so"
    slot_id: 0
    key_label: "themis-master-key"
    session_pool_size: 8  # For high throughput
```

---

### AWS KMS

#### Prerequisites

1. Create KMS key:
```bash
aws kms create-key \
  --description "ThemisDB master key" \
  --key-usage ENCRYPT_DECRYPT \
  --origin AWS_KMS \
  --multi-region false \
  --tags TagKey=Application,TagValue=ThemisDB
```

2. Create alias:
```bash
aws kms create-alias \
  --alias-name alias/themis-master-key \
  --target-key-id <key-id-from-step-1>
```

3. Grant permissions:
```bash
# Attach policy to IAM role
aws iam attach-role-policy \
  --role-name ThemisServerRole \
  --policy-arn arn:aws:iam::aws:policy/AWSKeyManagementServicePowerUser
```

#### Configuration

```yaml
hsm:
  provider: aws_kms
  aws_kms:
    region: "us-east-1"
    key_id: "alias/themis-master-key"
    # Optional: Use IAM role
    # role_arn: "arn:aws:iam::123456789012:role/ThemisKMSRole"
```

#### Authentication

Use one of:
- **EC2 Instance Profile** (recommended)
- **ECS Task Role** (for containers)
- **AWS credentials file** (`~/.aws/credentials`)

---

### Azure Key Vault

#### Prerequisites

1. Create Key Vault (Premium tier for HSM):
```bash
az keyvault create \
  --name themis-vault \
  --resource-group themis-rg \
  --location eastus \
  --sku Premium \
  --enable-rbac-authorization true
```

2. Create HSM-backed key:
```bash
az keyvault key create \
  --vault-name themis-vault \
  --name themis-master-key \
  --protection hsm \
  --kty RSA-HSM \
  --size 4096
```

3. Grant access:
```bash
# For managed identity
az role assignment create \
  --role "Key Vault Crypto Officer" \
  --assignee <managed-identity-principal-id> \
  --scope /subscriptions/<sub-id>/resourceGroups/themis-rg/providers/Microsoft.KeyVault/vaults/themis-vault
```

#### Configuration

```yaml
hsm:
  provider: azure_keyvault
  azure_keyvault:
    vault_url: "https://themis-vault.vault.azure.net/"
    key_name: "themis-master-key"
    auth_method: "managed_identity"
```

---

### Google Cloud KMS

#### Prerequisites

1. Create key ring:
```bash
gcloud kms keyrings create themis-keyring \
  --location us-central1
```

2. Create HSM-protected key:
```bash
gcloud kms keys create themis-master-key \
  --keyring themis-keyring \
  --location us-central1 \
  --purpose encryption \
  --protection-level hsm
```

3. Grant permissions:
```bash
gcloud kms keys add-iam-policy-binding themis-master-key \
  --keyring themis-keyring \
  --location us-central1 \
  --member serviceAccount:themis-sa@project.iam.gserviceaccount.com \
  --role roles/cloudkms.cryptoKeyEncrypterDecrypter
```

#### Configuration

```yaml
hsm:
  provider: gcp_kms
  gcp_kms:
    project_id: "your-gcp-project"
    location: "us-central1"
    key_ring: "themis-keyring"
    key_name: "themis-master-key"
```

---

### HashiCorp Vault

#### Prerequisites

1. Enable Transit engine:
```bash
vault secrets enable transit
```

2. Create encryption key:
```bash
vault write -f transit/keys/themis-master-key \
  type=aes256-gcm96 \
  exportable=false \
  allow_plaintext_backup=false
```

3. Create policy:
```bash
vault policy write themis-policy - <<EOF
path "transit/encrypt/themis-master-key" {
  capabilities = ["update"]
}
path "transit/decrypt/themis-master-key" {
  capabilities = ["update"]
}
EOF
```

4. Create AppRole:
```bash
vault auth enable approle
vault write auth/approle/role/themis \
  token_policies="themis-policy" \
  token_ttl=1h \
  token_max_ttl=4h
```

#### Configuration

```yaml
hsm:
  provider: vault
  vault:
    address: "https://vault.example.com:8200"
    mount_path: "transit"
    key_name: "themis-master-key"
    auth_method: "approle"
    # Set env vars: VAULT_ROLE_ID, VAULT_SECRET_ID
```

---

## Production Checklist

### Pre-Deployment

- [ ] HSM provider selected based on environment
- [ ] ThemisDB built with `-DTHEMIS_ENABLE_HSM_REAL=ON`
- [ ] **🆕 Vendor PKCS#11 headers installed** (for PKCS#11 providers)
- [ ] **🆕 Build with `-DTHEMIS_USE_VENDOR_PKCS11=ON`** (recommended)
- [ ] Master key generated in HSM
- [ ] `config/security.yaml` configured with real provider
- [ ] **🆕 `THEMIS_PRODUCTION_MODE=1` set** (enforces real HSM)
- [ ] **🆕 `THEMIS_ALLOW_HSM_STUB` NOT set** (blocks accidental stub usage)
- [ ] HSM PIN/credentials stored in secrets management
- [ ] HSM connectivity tested from application server
- [ ] Backup and recovery procedures documented
- [ ] Access controls configured (who can use keys)

### Deployment

- [ ] Server starts without stub provider warnings
- [ ] **🆕 No "HSM STUB PROVIDER ACTIVE" warnings** in logs
- [ ] Logs show: `HSMProvider init (real_ready=true)`
- [ ] First encryption operation succeeds
- [ ] Metrics show HSM operations (not stub)
- [ ] **🆕 `isStubProvider()` returns `false`** (real HSM confirmed)
- [ ] Audit logs capture HSM operations

### Post-Deployment

- [ ] Monitor HSM health metrics
- [ ] **🆕 Periodic security checks pass** (no stub warnings)
- [ ] Verify periodic security checks pass
- [ ] Test key rotation procedures
- [ ] Document disaster recovery process
- [ ] Schedule compliance audit

### 🛡️ Security Gating Verification (v1.4.2+)

Test security gating before production:

```bash
# Test 1: Verify production mode blocks stub
export THEMIS_PRODUCTION_MODE=1
# Should FAIL if stub is active
./themis_server --config config/test.yaml

# Test 2: Verify production environment detection
export ENVIRONMENT=production
unset THEMIS_ALLOW_HSM_STUB
# Should FAIL without opt-in
./themis_server --config config/test.yaml

# Test 3: Verify real HSM works in production mode
export THEMIS_PRODUCTION_MODE=1
# Should SUCCEED with real HSM
./themis_server --config config/production.yaml
```


---

## Troubleshooting

### Issue: "HSM stub provider cannot be used in production mode"

**New in v1.4.2** - Security gating prevents accidental stub usage.

**Error Message:**
```
SECURITY ERROR: HSM stub provider cannot be used in production mode. 
Build with -DTHEMIS_ENABLE_HSM_REAL=ON or disable THEMIS_PRODUCTION_MODE.
```

**Cause:** `THEMIS_PRODUCTION_MODE=1` is set but stub provider is active.

**Solutions:**

1. **Build with real HSM (recommended):**
```bash
cmake -DTHEMIS_ENABLE_HSM_REAL=ON \
      -DTHEMIS_USE_VENDOR_PKCS11=ON \
      -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
```

2. **Configure real HSM provider:**
```yaml
hsm:
  provider: pkcs11  # Not 'stub'
  pkcs11:
    library_path: "/usr/lib/libCryptoki2_64.so"
```

3. **Disable production mode (NOT recommended):**
```bash
unset THEMIS_PRODUCTION_MODE
```

### Issue: "HSM stub provider detected production environment"

**New in v1.4.2** - Auto-detection of production environments.

**Error Message:**
```
SECURITY ERROR: HSM stub provider detected production environment but 
THEMIS_ALLOW_HSM_STUB is not set. Set THEMIS_ALLOW_HSM_STUB=1 to explicitly 
allow insecure stub, or use real HSM.
```

**Cause:** `ENVIRONMENT=production` or `NODE_ENV=production` detected without opt-in.

**Solutions:**

1. **Use real HSM (recommended):**
   - Follow pre-deployment checklist above
   - Ensure build has `THEMIS_ENABLE_HSM_REAL=ON`

2. **Explicitly opt-in to stub (dev/test only):**
```bash
export THEMIS_ALLOW_HSM_STUB=1
```

3. **Change environment indicator:**
```bash
export ENVIRONMENT=development  # Or staging, test, etc.
```

### Issue: "HSM fallback stub active"

**Cause:** Real HSM connection failed.

**Solutions:**

1. Check library path:
```bash
ls -la /usr/lib/libCryptoki2_64.so
ldd /usr/lib/libCryptoki2_64.so  # Check dependencies
```

2. Verify HSM connectivity:
```bash
# For PKCS#11
pkcs11-tool --module /usr/lib/libCryptoki2_64.so --list-slots

# For Luna HSM
vtl verify
```

3. Check PIN:
```bash
export THEMIS_HSM_PIN="correct-pin"
echo $THEMIS_HSM_PIN  # Verify set
```

4. Enable verbose logging:
```yaml
hsm:
  pkcs11:
    verbose: true
```

### Issue: "Production mode blocks stub provider"

**Expected behavior** in production mode.

**Resolution:**

1. Fix HSM configuration (recommended)
2. Or use override flag (NOT RECOMMENDED):
```bash
./themis_server --config config/production.yaml --allow-stub-hsm
```

### Issue: AWS KMS permission denied

**Check IAM policy:**

```json
{
  "Version": "2012-10-17",
  "Statement": [{
    "Effect": "Allow",
    "Action": [
      "kms:Encrypt",
      "kms:Decrypt",
      "kms:GenerateDataKey",
      "kms:DescribeKey"
    ],
    "Resource": "arn:aws:kms:us-east-1:123456789012:key/*"
  }]
}
```

### Issue: Azure Key Vault 403 Forbidden

**Grant RBAC role:**

```bash
az role assignment create \
  --role "Key Vault Crypto User" \
  --assignee <principal-id> \
  --scope /subscriptions/<sub>/resourceGroups/<rg>/providers/Microsoft.KeyVault/vaults/<vault>
```

---

## Compliance Verification

### Self-Assessment Checklist

Use this checklist to verify compliance:

#### NIST SP 800-53 SC-12 (Key Management)

- [ ] Keys generated in HSM (never exported)
- [ ] Key access restricted by authentication
- [ ] Key usage logged to audit trail
- [ ] Key backup and recovery procedures in place
- [ ] Periodic key rotation policy defined

#### PCI DSS 3.6 (Key Protection)

- [ ] Cryptographic keys stored in HSM
- [ ] Access to keys requires multiple authentication factors
- [ ] Key storage separate from encrypted data
- [ ] Key custodian separation implemented
- [ ] Annual key review performed

#### GDPR Article 32 (Security of Processing)

- [ ] State-of-the-art encryption (AES-256-GCM)
- [ ] Pseudonymization where applicable
- [ ] Ability to restore data availability
- [ ] Regular testing of security measures
- [ ] Data breach notification procedures

#### ISO 27001 A.8.24 (Cryptography)

- [ ] Cryptographic policy documented
- [ ] Key management procedures defined
- [ ] Approved algorithms used (AES-256, RSA-4096)
- [ ] Key lifecycle management implemented

### External Audit

For formal compliance certification:

1. **Schedule audit** with certified assessor
2. **Provide documentation:**
   - HSM configuration
   - Key management procedures
   - Access control policies
   - Audit logs
3. **Demonstrate controls:**
   - Key generation in HSM
   - Access restrictions
   - Audit trail
   - Backup/recovery

---

## Additional Resources

### Documentation

- [Security Best Practices](./SECURITY_BEST_PRACTICES.md)
- [Compliance Guide](./COMPLIANCE_GUIDE.md)
- [Audit Reports](../audit-reports/v1.4.1/)

### Vendor Documentation

- **Thales Luna HSM:** [docs.thalesesecurity.com](https://docs.thalesesecurity.com)
- **AWS KMS:** [docs.aws.amazon.com/kms](https://docs.aws.amazon.com/kms)
- **Azure Key Vault:** [docs.microsoft.com/azure/key-vault](https://docs.microsoft.com/azure/key-vault)
- **GCP Cloud KMS:** [cloud.google.com/kms/docs](https://cloud.google.com/kms/docs)
- **HashiCorp Vault:** [vaultproject.io/docs](https://vaultproject.io/docs)

### Support

- **Email:** security@themisdb.com
- **Slack:** #security-help
- **Emergency:** +1-555-SECURE-NOW

---

## Changelog

| Version | Date | Changes |
|---------|------|---------|
| 1.4.2 | 2026-02-03 | Initial production setup guide |
| 1.4.1 | 2026-01-15 | Security audit findings |

---

**IMPORTANT:** This document contains security-sensitive information. Distribute only to authorized personnel.
