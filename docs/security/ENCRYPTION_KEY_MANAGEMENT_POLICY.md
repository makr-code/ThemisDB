# Encryption Key Management Policy

**Document Version**: 1.0  
**Last Updated**: 2026-04-06  
**Status**: Active  
**Classification**: Confidential  
**Owner**: Chief Information Security Officer (CISO)

---

## 📋 Purpose

This policy establishes standards and procedures for the lifecycle management of cryptographic keys used in ThemisDB systems, ensuring secure generation, storage, distribution, rotation, and destruction of encryption keys.

## 🎯 Scope

This policy covers:
- All cryptographic keys used in ThemisDB systems
- Data encryption keys (DEKs), Key encryption keys (KEKs), Master keys
- Key generation, storage, backup, rotation, and destruction
- All environments: development, staging, and production

---

## 🔑 Key Hierarchy

### Overview

ThemisDB uses a hierarchical key management system:

```
┌─────────────────────────────────────┐
│   Master Key (Root of Trust)       │
│   Storage: HSM / Cloud KMS          │
│   Rotation: Every 3 years           │
└──────────────┬──────────────────────┘
               │
       ┌───────┴───────┐
       │               │
┌──────▼──────┐  ┌────▼──────────┐
│ Key         │  │ Data          │
│ Encryption  │  │ Encryption    │
│ Keys (KEK)  │  │ Keys (DEK)    │
│             │  │               │
│ Rotation:   │  │ Rotation:     │
│ Every 2     │  │ Annually      │
│ years       │  │               │
└─────────────┘  └───┬───────────┘
                     │
        ┌────────────┼────────────┐
        │            │            │
   ┌────▼─────┐ ┌───▼────┐ ┌────▼──────┐
   │ Field    │ │ Blob   │ │ Audit Log │
   │ Encrypt  │ │ Encrypt│ │ Keys      │
   │ Keys     │ │ Keys   │ │           │
   └──────────┘ └────────┘ └───────────┘
```

### Key Types

#### 1. Master Key (MK)

- **Purpose**: Root of trust, encrypts KEKs
- **Storage**: Hardware Security Module (HSM) or Cloud KMS
- **Lifetime**: 3 years
- **Backup**: Encrypted split across multiple secure locations
- **Access**: Extremely restricted (CISO, designated key custodians only)

#### 2. Key Encryption Keys (KEK)

- **Purpose**: Encrypt data encryption keys for storage
- **Storage**: Encrypted by master key, stored in secure key store
- **Lifetime**: 2 years
- **Backup**: Encrypted with master key
- **Access**: Automated systems with service accounts

#### 3. Data Encryption Keys (DEK)

- **Purpose**: Encrypt actual data (fields, blobs, logs)
- **Storage**: Encrypted by KEK, stored with encrypted data or in key store
- **Lifetime**: 1 year (production), unlimited (development)
- **Rotation**: Annual or event-triggered
- **Access**: Application-level, retrieved via key provider interface

---

## 🔐 Key Generation

### Requirements

**Random Number Generation**:
- Use cryptographically secure random number generators (CSRNG)
- Linux: `/dev/urandom` or `getrandom()`
- OpenSSL: `RAND_bytes()`
- Never use `rand()`, `random()`, or time-based seeds

**Key Strength**:
| Key Type | Minimum Strength | Recommended |
|----------|------------------|-------------|
| AES Symmetric | 128 bits | 256 bits |
| RSA Asymmetric | 2048 bits | 3072 bits |
| ECC | P-256 (256 bits) | P-384 (384 bits) |

**Key Generation Location**:
- **Production**: HSM or KMS with FIPS 140-2 Level 2+ certification
- **Staging**: KMS or secure key generation service
- **Development**: Mock key provider (never use in production)

### Generation Process

1. **Request Authorization**: Key generation must be authorized by security team
2. **Generate Key**: Use approved CSRNG with sufficient entropy
3. **Encrypt Key**: Immediately encrypt with KEK (for DEKs) or store in HSM (for Master Keys)
4. **Record Metadata**: Key ID, creation date, purpose, algorithm, creator
5. **Audit Log**: Log key generation event

**Implementation**: `src/security/key_provider.h`

---

## 💾 Key Storage

### Storage Requirements

#### Production Environment

**Master Keys**:
- ✅ Stored in Hardware Security Module (HSM)
- ✅ FIPS 140-2 Level 2 or higher certified
- ✅ Multi-party authorization for access
- ✅ Tamper-evident and tamper-resistant

**Key Encryption Keys**:
- ✅ Encrypted by master key
- ✅ Stored in secure, encrypted key store
- ✅ Access controlled via service accounts
- ✅ Replicated across availability zones

**Data Encryption Keys**:
- ✅ Encrypted by KEK
- ✅ Stored in metadata database or with encrypted data
- ✅ Cached in memory (with protection)
- ✅ Never stored in plaintext

#### Non-Production Environments

**Development/Testing**:
- File-based key storage acceptable (encrypted)
- Mock key provider for unit tests
- Separate keys from production (never copy production keys)
- Keys can be regenerated as needed

### Storage Security

**Access Controls**:
- Principle of least privilege
- Service accounts with minimal permissions
- No human access to plaintext keys in production
- MFA required for HSM access

**Encryption at Rest**:
- All stored keys encrypted (except master key in HSM)
- AES-256-GCM for key wrapping
- Key metadata can be stored in plaintext (but not keys themselves)

---

## 🔄 Key Distribution

### Secure Distribution Methods

1. **HSM/KMS Integration**: Keys retrieved via secure API
2. **Service Accounts**: Application retrieves keys via authenticated API
3. **Secrets Management**: Keys distributed via Vault, AWS Secrets Manager, etc.
4. **Manual Distribution**: Encrypted channel with out-of-band verification (development only)

### Distribution Restrictions

❌ **Never Distribute Keys Via**:
- Email
- Instant messaging
- Source code repositories
- Configuration files in version control
- Unencrypted network protocols
- Removable media without encryption

✅ **Approved Methods**:
- HSM/KMS API calls with mutual TLS
- Secrets management systems
- Encrypted and authenticated protocols
- Split-knowledge key sharing (for recovery)

---

## 🔁 Key Rotation

### Rotation Schedule

| Key Type | Rotation Frequency | Event-Triggered |
|----------|-------------------|-----------------|
| Master Key | Every 3 years | Security incident, HSM compromise |
| KEK | Every 2 years | Security incident |
| Data Encryption Keys | Annually | Security incident, suspected compromise |
| API Keys | Every 90 days | Suspected compromise, user termination |
| TLS Certificates | Annually | Compromise, algorithm deprecation |

### Rotation Process

#### Lazy Re-Encryption (Preferred)

**Implementation**: `src/security/field_encryption.cpp::decryptAndReEncrypt()`

**Advantages**:
- Zero downtime
- No large-scale data migration required
- Gradual transition

**Process**:
1. **Generate New Key**: New key version created
2. **Deploy Key**: New key deployed to all nodes
3. **Update Configuration**: New key marked as active
4. **Lazy Re-encryption**: 
   - Reads decrypt with old key
   - Writes re-encrypt with new key
   - Data migrated over time
5. **Background Migration**: Optional batch update for remaining data
6. **Retire Old Key**: After migration complete, old key retired (but retained for emergency)

**Status Check**: `needsReEncryption()` method detects data encrypted with old keys

#### Immediate Re-Encryption (When Required)

Used when old key is compromised:
1. **Revoke Old Key**: Immediately mark old key as compromised
2. **Generate New Key**: Create replacement key
3. **Batch Re-encryption**: Re-encrypt all data with new key (scheduled job)
4. **Verification**: Verify all data re-encrypted successfully
5. **Destroy Old Key**: Securely destroy compromised key

### Key Version Management

**Key Versioning**:
- Each key has version identifier (e.g., `data-encryption-key-v1`, `data-encryption-key-v2`)
- Encrypted data tagged with key version used
- Multiple versions active simultaneously during rotation
- Old versions retained read-only after rotation

**Backward Compatibility**:
- System can decrypt with any historical key version
- Old keys retained for configured retention period (default: 1 year post-rotation)
- Emergency recovery procedures documented

---

## 💾 Key Backup and Recovery

### Backup Requirements

**Master Keys**:
- Split into shares using Shamir's Secret Sharing (minimum 3-of-5)
- Each share stored in separate secure location
- Geographic distribution of shares
- Annual verification of recovery process

**Key Encryption Keys**:
- Encrypted backup stored in secure offsite location
- Multiple copies across availability zones
- Encrypted with master key or separate backup key

**Data Encryption Keys**:
- Backed up with encrypted data
- Included in database backups
- Versioned with data snapshots

### Recovery Procedures

#### Master Key Recovery

**Trigger**: HSM failure, data center disaster, corruption

**Process**:
1. **Assemble Key Custodians**: Minimum 3 of 5 share holders
2. **Retrieve Shares**: Each custodian retrieves their share
3. **Reconstruct Key**: Combine shares to reconstruct master key
4. **Load into HSM**: Load recovered key into new HSM
5. **Verify**: Test decryption with recovered key
6. **Audit**: Log recovery event and participants

**Authorization**: Requires CISO approval and dual control

#### KEK/DEK Recovery

**Trigger**: Key loss, corruption, accidental deletion

**Process**:
1. **Identify Lost Key**: Determine key ID and version
2. **Restore from Backup**: Retrieve encrypted key from backup
3. **Decrypt with Master Key**: Unwrap key using master key
4. **Verify**: Test key with encrypted data
5. **Re-deploy**: Deploy recovered key to systems
6. **Audit**: Log recovery event

---

## 🗑️ Key Destruction

### When to Destroy Keys

- Key rotation complete and retention period expired
- Key compromised (after incident resolution)
- System decommissioned
- End of data retention period
- Regulatory requirement

### Destruction Methods

#### Cryptographic Erasure (Preferred)

- Destroy master key or KEK
- Encrypted data becomes permanently unrecoverable
- Fast and effective
- Suitable for large datasets

#### Physical Destruction

For keys stored on physical media:
- HSM: Follow manufacturer's key zeroization procedures
- Hard drives: Degaussing followed by physical destruction
- Certificates: Secure shredding
- Backup tapes: Degaussing or incineration

#### Secure Deletion

For keys in software:
- Overwrite memory with random data (minimum 3 passes)
- Use secure deletion APIs (e.g., `sodium_memzero()`)
- Verify deletion
- Clear all copies (cache, logs, backups)

### Destruction Verification

- **Certificate of Destruction**: Required for physical media
- **Audit Log**: All key destructions logged
- **Verification**: Attempt decryption fails after key destruction
- **Compliance**: Meet regulatory destruction requirements (GDPR "right to erasure")

---

## 📊 Key Management Metrics

### Monitoring and Reporting

**Prometheus Metrics** (`include/security/field_encryption.h`):
- `encryption_keys_active_total` - Number of active keys
- `encryption_keys_rotated_total` - Key rotation events
- `encryption_key_age_days` - Age of each active key
- `encryption_reencryption_operations_total` - Lazy re-encryption operations

**Reports**:
- Monthly key lifecycle report
- Key age analysis
- Rotation compliance status
- Key usage patterns

### Alerting

**Alerts Configured**:
- Key approaching expiration (30 days before)
- Key expired but still in use
- Failed key rotation
- Unusual key access patterns
- Key generation failures

---

## 🔐 Access Control

### Key Access Roles

| Role | Permissions | MFA Required |
|------|-------------|--------------|
| **CISO** | Full key lifecycle management | Yes |
| **Security Admin** | Key generation, rotation, monitoring | Yes |
| **Key Custodian** | Master key shard holder | Yes |
| **Application Service** | DEK retrieval for encryption/decryption | No (service authentication) |
| **DBA** | View key metadata (not keys) | Yes |
| **Developer** | Development keys only | No |
| **Auditor** | Read-only access to audit logs | Yes |

### Authorization Matrix

| Action | CISO | Security Admin | Key Custodian | App Service | DBA | Developer | Auditor |
|--------|------|----------------|---------------|-------------|-----|-----------|---------|
| Generate Master Key | ✅ | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ |
| Generate DEK | ✅ | ✅ | ❌ | ✅ | ❌ | ✅* | ❌ |
| Rotate Keys | ✅ | ✅ | ❌ | ❌ | ❌ | ❌ | ❌ |
| Access Master Key | ✅ | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ |
| Retrieve DEK | ✅ | ✅ | ❌ | ✅ | ❌ | ✅* | ❌ |
| View Audit Logs | ✅ | ✅ | ❌ | ❌ | ✅ | ❌ | ✅ |
| Destroy Keys | ✅ | ✅** | ❌ | ❌ | ❌ | ❌ | ❌ |

*Development environment only  
**With CISO approval

---

## 📋 Compliance

### Regulatory Requirements

**GDPR Article 32**: Security of Processing
- ✅ Encryption of personal data (keys managed per this policy)
- ✅ Ongoing confidentiality, integrity, availability
- ✅ Regular testing and evaluation

**PCI DSS** (if applicable):
- ✅ Requirement 3.5: Protect keys against disclosure and misuse
- ✅ Requirement 3.6: Fully document and implement key management processes

**ISO 27001**:
- ✅ Control A.10.1.2: Key management

**NIST SP 800-57**: Recommendations for Key Management
- Followed for key strength, lifecycle, and procedures

---

## 🔍 Audit and Compliance

### Audit Events

All key management operations logged:
- Key generation (with requester, timestamp, purpose)
- Key access (application, timestamp, key ID)
- Key rotation (old version, new version, timestamp)
- Key destruction (key ID, method, authorizer)
- Failed access attempts
- Configuration changes

**Implementation**: Audit logs encrypted and signed (Saga Logger)

### Periodic Reviews

**Quarterly**:
- [ ] Review key access logs
- [ ] Verify key rotation schedule compliance
- [ ] Check for expired keys still in use
- [ ] Review key access permissions

**Annual**:
- [ ] Comprehensive key management audit
- [ ] Test master key recovery procedures
- [ ] Review and update policy
- [ ] Security assessment of key storage systems

---

## 📚 Training and Awareness

### Required Training

**Security Team**:
- Cryptography fundamentals
- HSM/KMS operation
- Key recovery procedures
- Incident response for key compromise

**Developers**:
- Secure key usage in applications
- Never hardcode keys
- Use key provider interface
- Proper key lifecycle management

**Operations**:
- Key rotation procedures
- Monitoring and alerting
- Backup and recovery
- Incident escalation

---

## 🚨 Incident Response

### Key Compromise

**Indicators**:
- Unauthorized key access
- Key exported from HSM
- Suspicious decryption operations
- Key found in logs or repositories

**Response Procedure**:
1. **Contain**: Revoke compromised key immediately
2. **Assess**: Determine scope of compromise
3. **Notify**: Alert CISO, security team, affected parties
4. **Rotate**: Emergency key rotation
5. **Investigate**: Forensic analysis
6. **Report**: Regulatory notification if required
7. **Remediate**: Address root cause
8. **Review**: Post-incident analysis

**Timeline**:
- Detection to containment: < 15 minutes
- Complete key rotation: < 4 hours
- Incident report: Within 24 hours

---

## 📚 Related Documentation

- [encryption_strategy.md](encryption_strategy.md) - Encryption standards and algorithms
- [INFORMATION_SECURITY_POLICY.md](INFORMATION_SECURITY_POLICY.md) - Overall security framework
- `docs/encryption_metrics.md` - Monitoring and metrics
- `include/security/key_provider.h` - Key provider interface
- `src/security/field_encryption.cpp` - Encryption implementation

---

## 📝 Change History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0 | 2026-01-11 | CISO | Initial Encryption Key Management Policy |

---

## ✅ Approval

**Drafted by**: Security Team  
**Reviewed by**: CISO, Legal, Compliance  
**Approved by**: CTO, CEO  
**Effective Date**: 2026-01-11  
**Next Review Date**: 2027-01-11

---

## 📧 Contact

**Security Questions**: security@themisdb.org  
**Key Management Issues**: keymanagement@themisdb.org  
**Emergency Key Compromise**: security-incidents@themisdb.org (24/7)
