# At-Rest Encryption: Hyperscaler Research & Best Practices

**Date:** February 17, 2026  
**Version:** 1.0.0  
**Status:** Research & Recommendation  
**Category:** Security

---

## 📋 Executive Summary

This document investigates mechanisms for granular at-rest encryption at hardware and filesystem levels, without requiring ThemisDB to implement custom encryption mechanisms. It analyzes approaches used by major hyperscalers (AWS, Google Cloud, Microsoft Azure) and provides recommendations for ThemisDB deployments.

**Current Situation:**
- ✅ ThemisDB already has field-level encryption (AES-256-GCM)
- ✅ Data in transit is encrypted (TLS 1.3)
- ⚠️ No comprehensive encryption down to hardware/filesystem level
- ⚠️ RocksDB data resides in plaintext on filesystem (except encrypted fields)

---

## 🎯 Problem Statement

ThemisDB currently does not establish end-to-end encryption down to the hardware/filesystem level. This gap means:

1. **Backup Security**: RocksDB SST file backups contain unencrypted metadata and index structures
2. **Disk Theft**: Physical theft of disks allows attackers to read metadata, keys, and non-encrypted fields
3. **Cold Storage**: Archived data is not protected at the filesystem level
4. **Compliance**: Some compliance frameworks require encryption-at-rest at the storage layer

---

## 🏢 Hyperscaler Approaches: AWS, Google Cloud, Microsoft Azure

### 1. Amazon Web Services (AWS)

#### A. Server-Side Encryption (SSE)

**SSE-S3 (S3-Managed Keys)**
```
┌─────────────┐
│  S3 Bucket  │
│             │
│  ┌───────┐  │
│  │ Object│  │ ← Automatic AES-256 encryption
│  └───────┘  │
│             │
└─────────────┘
     │
     └──> AWS manages keys transparently
```

**Properties:**
- Automatic encryption on write
- Automatic decryption on read
- No key management logic in application code
- AES-256-GCM algorithm
- Envelope encryption (Data Key + Master Key)

**SSE-KMS (AWS Key Management Service)**
```
┌─────────────┐         ┌──────────────┐
│  S3 Bucket  │         │   AWS KMS    │
│             │◄────────┤              │
│  ┌───────┐  │  KEK    │  Master Keys │
│  │ Object│  │◄────────┤  (Hardware)  │
│  └───────┘  │  DEK    │              │
│             │         └──────────────┘
└─────────────┘
```

**Properties:**
- Centralized key management
- Audit trail (CloudTrail)
- FIPS 140-2 Level 2 validated HSMs
- Automatic key rotation
- Fine-grained access control (IAM policies)
- Envelope encryption: Master key protects data encryption keys

**SSE-C (Customer-Provided Keys)**
```
Client                 S3
  │                    │
  ├─ PUT Object ──────>│
  │  + AES-256 Key     │
  │  + Key MD5         │
  │                    │ Encrypt with customer key
  │                    │ Store (encrypted)
  │                    │ Discard key
  │                    │
  ├─ GET Object ──────>│
  │  + AES-256 Key     │
  │                    │ Decrypt with customer key
  │<─ Plaintext ───────┤
```

**Properties:**
- Customer retains full control of keys
- AWS does NOT store keys
- HMAC-SHA256 hash for key verification
- Key must be sent with every request

#### B. Amazon EBS Encryption

**Transparent Volume Encryption:**
```
┌─────────────────────────────────────────┐
│           EC2 Instance                  │
│                                         │
│  ┌────────────────────────────────┐    │
│  │  Operating System / App        │    │
│  └────────────┬───────────────────┘    │
│               │ I/O Operations          │
│  ┌────────────▼───────────────────┐    │
│  │  EBS Volume (encrypted)        │    │
│  │  - AES-256-XTS                 │    │
│  │  - Hardware-accelerated        │    │
│  │  - Transparent to OS           │    │
│  └────────────┬───────────────────┘    │
│               │                         │
└───────────────┼─────────────────────────┘
                │ Encrypted I/O
┌───────────────▼─────────────────────────┐
│      AWS Nitro System                   │
│      (Dedicated Hardware Encryption)    │
└─────────────────────────────────────────┘
```

**Properties:**
- AES-256-XTS algorithm (optimized for block storage)
- Hardware-accelerated encryption (AWS Nitro System)
- Transparent encryption at hypervisor level
- Zero performance overhead (hardware offload)
- AWS KMS integration
- Boot volume encryption supported
- Snapshots automatically encrypted

**Important for ThemisDB:**
- RocksDB files reside on encrypted EBS volume
- No code change in ThemisDB required
- OS sees only unencrypted data (transparency)

#### C. Amazon RDS Encryption

**MySQL/PostgreSQL At-Rest Encryption:**
- Based on EBS encryption
- Additional encryption for logs and backups
- Replica encryption
- Transparent Data Encryption (TDE) for Oracle/SQL Server

---

### 2. Google Cloud Platform (GCP)

#### A. Default Encryption at Rest

**Automatic Encryption for ALL Services:**
```
┌────────────────────────────────────────────────┐
│         Google Cloud Storage                   │
│                                                │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐    │
│  │  Chunk 1 │  │  Chunk 2 │  │  Chunk 3 │    │
│  └────┬─────┘  └────┬─────┘  └────┬─────┘    │
│       │             │             │           │
│  ┌────▼─────┬───────▼─────┬───────▼─────┐    │
│  │   DEK 1  │    DEK 2    │    DEK 3    │    │
│  │ AES-256  │  AES-256    │  AES-256    │    │
│  └────┬─────┴─────┬───────┴─────┬───────┘    │
│       │           │             │             │
│  ┌────▼───────────▼─────────────▼─────────┐  │
│  │     Master Key (KEK)                    │  │
│  │     - 90-day automatic rotation        │  │
│  │     - FIPS 140-2 Level 3 HSMs         │  │
│  └─────────────────────────────────────────┘  │
│                                                │
└────────────────────────────────────────────────┘
```

**Properties:**
- **Default**: All data automatically encrypted (opt-out NOT possible)
- **Envelope Encryption**: Data Encryption Keys (DEK) + Key Encryption Keys (KEK)
- **Chunk-Level**: Each chunk (several MB) has its own DEK
- **Automatic Key Rotation**: KEKs rotated every 90 days
- **FIPS 140-2 Level 3**: HSMs in Google data centers
- **Zero-Copy**: Hardware encryption (Titan chip)

**Google Titan Security Chip:**
```
┌──────────────────────────────────────┐
│         Server Hardware              │
│                                      │
│  ┌────────────┐    ┌──────────────┐ │
│  │    CPU     │    │   Storage    │ │
│  └─────┬──────┘    └──────┬───────┘ │
│        │                  │         │
│  ┌─────▼──────────────────▼───────┐ │
│  │    Titan Security Chip         │ │
│  │    - Root of Trust             │ │
│  │    - Key Management            │ │
│  │    - AES-256 Engine            │ │
│  └────────────────────────────────┘ │
└──────────────────────────────────────┘
```

- Hardware Root of Trust
- Secure Boot verification
- Hardware-based encryption
- Protection against physical attacks

#### B. Customer-Managed Encryption Keys (CMEK)

**Integration with Cloud KMS:**
```python
# GCS Bucket with CMEK
from google.cloud import storage

client = storage.Client()
bucket = client.bucket('themisdb-backups')
bucket.default_kms_key_name = (
    'projects/my-project/'
    'locations/eu-central1/'
    'keyRings/themis-ring/'
    'cryptoKeys/backup-key'
)
bucket.patch()
```

**Properties:**
- Customer controls key lifecycle
- Audit logs for key usage
- Automatic rotation or manual rotation
- Access control via IAM
- Multi-region keys supported

#### C. Cloud External Key Manager (Cloud EKM)

**External HSM Integration:**
```
┌─────────────────┐         ┌──────────────────┐
│  Google Cloud   │         │  Customer HSM    │
│                 │         │  (On-Premise)    │
│  ┌───────────┐  │         │                  │
│  │   Data    │  │  KEK    │  ┌────────────┐  │
│  │(encrypted)│  │◄────────┼──┤Master Keys │  │
│  └───────────┘  │ Request │  └────────────┘  │
│                 │         │                  │
└─────────────────┘         └──────────────────┘
```

**Properties:**
- Keys NEVER leave Google Cloud premises
- Supports Thales, Fortanix, HashiCorp Vault
- FIPS 140-2 Level 3 HSMs
- Compliance for highly regulated industries

---

### 3. Microsoft Azure

#### A. Azure Storage Service Encryption (SSE)

**Default Encryption:**
```
┌────────────────────────────────────────────┐
│       Azure Storage Account                │
│                                            │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐ │
│  │  Blob 1  │  │  Blob 2  │  │  Blob 3  │ │
│  └────┬─────┘  └────┬─────┘  └────┬─────┘ │
│       │             │             │        │
│  ┌────▼─────────────▼─────────────▼─────┐  │
│  │     Platform-Managed Keys (PMK)     │  │
│  │     AES-256                          │  │
│  │     Automatic Rotation               │  │
│  └──────────────────────────────────────┘  │
└────────────────────────────────────────────┘
```

**Properties:**
- Enabled by default (cannot be disabled)
- AES-256 encryption
- Automatic key rotation
- No additional cost
- Supports: Blobs, Files, Tables, Queues, Disks

#### B. Azure Disk Encryption (ADE)

**BitLocker (Windows) / dm-crypt (Linux):**
```
┌──────────────────────────────────────────────┐
│           Azure VM                           │
│                                              │
│  ┌────────────────────────────────────────┐ │
│  │  Operating System                      │ │
│  │  - BitLocker (Windows)                 │ │
│  │  - dm-crypt/LUKS (Linux)              │ │
│  └────────────┬───────────────────────────┘ │
│               │                              │
│  ┌────────────▼───────────────────────────┐ │
│  │  Encrypted OS Disk                     │ │
│  │  - AES-256-XTS                         │ │
│  │  - KEK in Azure Key Vault              │ │
│  └────────────────────────────────────────┘ │
│                                              │
└──────────────────────────────────────────────┘
```

**Linux Implementation (dm-crypt):**
```bash
# Azure Disk Encryption uses dm-crypt under the hood
# DEK stored in Azure Key Vault
# Automatic unlock on VM start

# Manual equivalent:
cryptsetup luksFormat /dev/sdb
cryptsetup luksOpen /dev/sdb themisdb_data
mkfs.ext4 /dev/mapper/themisdb_data
mount /dev/mapper/themisdb_data /var/lib/themisdb
```

**Properties:**
- OS-native encryption (BitLocker/dm-crypt)
- Azure Key Vault integration
- Boot volume encryption
- Managed and unmanaged disks
- Snapshot encryption

#### C. Customer-Managed Keys (CMK) in Azure Key Vault

**Custom Keys in Azure Key Vault:**
```csharp
// .NET Example
var keyVaultClient = new KeyVaultClient(...);
var encryptionKey = await keyVaultClient.GetKeyAsync(
    "https://themisdb-vault.vault.azure.net/",
    "storage-encryption-key"
);

var storageAccount = new StorageAccountCreateParameters
{
    Encryption = new Encryption
    {
        Services = new EncryptionServices
        {
            Blob = new EncryptionService { Enabled = true },
            File = new EncryptionService { Enabled = true }
        },
        KeySource = KeySource.MicrosoftKeyvault,
        KeyVaultProperties = new KeyVaultProperties
        {
            KeyName = "storage-encryption-key",
            KeyVaultUri = "https://themisdb-vault.vault.azure.net/"
        }
    }
};
```

**Properties:**
- Full control over key lifecycle
- HSM-backed keys (Premium tier)
- Soft delete and purge protection
- Access control via RBAC
- Audit logs

#### D. Confidential Computing

**Azure Confidential VMs:**
```
┌──────────────────────────────────────────────┐
│         Confidential VM (AMD SEV-SNP)        │
│                                              │
│  ┌────────────────────────────────────────┐ │
│  │  Encrypted Memory (RAM)                │ │
│  │  - AMD Secure Encrypted Virtualization│ │
│  │  - CPU-based AES-128 Encryption       │ │
│  └────────────────────────────────────────┘ │
│                                              │
│  ┌────────────────────────────────────────┐ │
│  │  Encrypted Storage                     │ │
│  │  - ADE (dm-crypt)                      │ │
│  └────────────────────────────────────────┘ │
│                                              │
└──────────────────────────────────────────────┘
```

**Properties:**
- Hardware-based memory encryption
- Protection against hypervisor access
- Attestation for trustworthiness
- Supports AMD SEV-SNP, Intel TDX, Intel SGX

---

## 💾 Filesystem-Level Encryption (Linux)

### 1. dm-crypt / LUKS

**Linux Unified Key Setup (LUKS):**
```bash
# Setup
cryptsetup luksFormat /dev/sdb
cryptsetup luksOpen /dev/sdb themisdb_encrypted
mkfs.ext4 /dev/mapper/themisdb_encrypted
mount /dev/mapper/themisdb_encrypted /var/lib/themisdb

# RocksDB uses normal filesystem path
# Encryption is transparent
```

**Architecture:**
```
┌────────────────────────────────────────────────┐
│           Application (ThemisDB)               │
│                     │                          │
│                     ▼                          │
│  ┌──────────────────────────────────────────┐ │
│  │  Filesystem (ext4, xfs, btrfs)           │ │
│  └──────────────┬───────────────────────────┘ │
│                 │                              │
│  ┌──────────────▼───────────────────────────┐ │
│  │  dm-crypt (Device Mapper)                │ │
│  │  - AES-256-XTS                           │ │
│  │  - Hardware AES-NI acceleration         │ │
│  └──────────────┬───────────────────────────┘ │
│                 │                              │
│  ┌──────────────▼───────────────────────────┐ │
│  │  Block Device (/dev/sdb)                 │ │
│  └──────────────────────────────────────────┘ │
└────────────────────────────────────────────────┘
```

**Properties:**
- ✅ **Linux Standard**: Kernel module dm-crypt
- ✅ **Transparent**: Applications see unencrypted data
- ✅ **Hardware-Accelerated**: Uses Intel AES-NI
- ✅ **Multiple Keys**: 8 key slots for different passwords
- ✅ **Performance**: ~5-10% overhead with AES-NI
- ⚠️ **Manual Key Management**: Keys must be provided at boot

**Performance (with AES-NI):**
```
Sequential Read:  1800 MB/s (unencrypted: 1900 MB/s)  → 5% Overhead
Sequential Write: 1700 MB/s (unencrypted: 1800 MB/s)  → 6% Overhead
Random Read:      450k IOPS (unencrypted: 470k IOPS)  → 4% Overhead
Random Write:     380k IOPS (unencrypted: 400k IOPS)  → 5% Overhead
```

### 2. fscrypt

**Filesystem-Native Encryption (ext4, f2fs):**
```bash
# Enable fscrypt on ext4
tune2fs -O encrypt /dev/sdb1

# Create encrypted directory
mkdir /var/lib/themisdb/encrypted
fscrypt setup
fscrypt encrypt /var/lib/themisdb/encrypted
```

**Architecture:**
```
┌────────────────────────────────────────┐
│      Application (ThemisDB)            │
│               │                        │
│               ▼                        │
│  ┌──────────────────────────────────┐ │
│  │  ext4 Filesystem                 │ │
│  │  ┌─────────────────────────────┐ │ │
│  │  │ /var/lib/themisdb           │ │ │
│  │  │   ├─ data/ (encrypted)      │ │ │
│  │  │   │   ├─ file1.sst (AES)    │ │ │
│  │  │   │   └─ file2.sst (AES)    │ │ │
│  │  │   └─ logs/ (unencrypted)    │ │ │
│  │  └─────────────────────────────┘ │ │
│  └──────────────────────────────────┘ │
└────────────────────────────────────────┘
```

**Properties:**
- ✅ **Granular**: Per-directory encryption
- ✅ **Selective**: Only encrypt sensitive directories
- ✅ **Performance**: Better than dm-crypt (~2% overhead)
- ✅ **Flexible Keys**: Different keys for different directories
- ⚠️ **Filesystem-Specific**: Only ext4, f2fs (not XFS, Btrfs)
- ⚠️ **Limited Adoption**: Not as widely used as LUKS

**Use Case for ThemisDB:**
```bash
# Encrypt RocksDB SST files, not WAL
fscrypt encrypt /var/lib/themisdb/data
# WAL remains unencrypted for better performance
```

---

## 🔧 Hardware-Level Encryption

### 1. Self-Encrypting Drives (SED)

**TCG Opal Standard:**
```
┌────────────────────────────────────────────────┐
│             SSD/HDD Hardware                   │
│                                                │
│  ┌──────────────────────────────────────────┐ │
│  │  Controller                              │ │
│  │  - AES-256-XTS Engine                    │ │
│  │  - Key Management                        │ │
│  │  - Authentication                        │ │
│  └──────────────┬───────────────────────────┘ │
│                 │                              │
│  ┌──────────────▼───────────────────────────┐ │
│  │  NAND Flash / Platters (encrypted)       │ │
│  │  - Data always encrypted                 │ │
│  │  - DEK in hardware                       │ │
│  └──────────────────────────────────────────┘ │
└────────────────────────────────────────────────┘
```

**Properties:**
- ✅ **Zero CPU Overhead**: Hardware encryption
- ✅ **Always-On**: Data always encrypted on write
- ✅ **Secure Erase**: Cryptographic deletion in <1 second (key destruction)
- ✅ **Pre-Boot Authentication**: BIOS/UEFI integration
- ⚠️ **Vendor Lock-In**: Different implementations
- ⚠️ **Key Recovery Complex**: Difficult if drive fails

**TCG Opal 2.0 Features:**
- Hardware-based keys
- Multi-user authentication
- Locking ranges (different regions, different keys)
- Secure boot integration

**Known SEDs:**
- Samsung 850/860/870 EVO/PRO
- Crucial MX500
- WD Black SN850
- Intel Optane SSD DC P4800X

**Linux Integration (sedutil):**
```bash
# Activate SED
sedutil-cli --initialSetup <password> /dev/sdb

# Pre-boot authentication
sedutil-cli --enableLockingRange 0 <password> /dev/sdb

# Unlock in running system
sedutil-cli --setLockingRange 0 RW <password> /dev/sdb
```

### 2. Intel AES-NI

**CPU-based Acceleration:**
```
┌────────────────────────────────────────┐
│              CPU                       │
│                                        │
│  ┌──────────────────────────────────┐ │
│  │  AES-NI Instructions             │ │
│  │  - AESENC, AESDEC               │ │
│  │  - AESIMC, AESKEYGENASSIST      │ │
│  └──────────────────────────────────┘ │
│                                        │
│  Performance: ~4-6x faster than       │
│               software AES            │
└────────────────────────────────────────┘
```

**Properties:**
- ✅ **Hardware-Acceleration**: 4-6x faster than software AES
- ✅ **Standard**: In all modern Intel/AMD CPUs
- ✅ **Transparent**: Automatically used by dm-crypt/fscrypt
- ✅ **Constant-Time**: Protection against timing attacks

**Verification:**
```bash
# Check for AES-NI support
grep -m1 -o aes /proc/cpuinfo
# Output: aes

# OpenSSL speed test
openssl speed -evp aes-256-cbc
# With AES-NI:    ~3000 MB/s
# Without AES-NI: ~500 MB/s
```

---

## 📊 RocksDB-Specific Encryption

### RocksDB Encryption-at-Rest

**RocksDB offers native encryption:**
```cpp
// RocksDB with encryption-at-rest
#include <rocksdb/db.h>
#include <rocksdb/encryption.h>

// Custom EncryptionProvider
class AES256EncryptionProvider : public rocksdb::EncryptionProvider {
  // Implementation see RocksDB docs
};

rocksdb::Options options;
auto encryption = std::make_shared<AES256EncryptionProvider>();
options.env = rocksdb::NewEncryptedEnv(rocksdb::Env::Default(), encryption);

rocksdb::DB* db;
rocksdb::DB::Open(options, "/var/lib/themisdb/data", &db);
```

**Properties:**
- ✅ **Granular**: Encryption at SST file level
- ✅ **Transparent**: No changes to query logic
- ✅ **Key-Rotation**: Supports key rotation
- ⚠️ **Performance**: 10-15% overhead (without hardware acceleration)
- ⚠️ **Custom Implementation**: Requires custom EncryptionProvider implementation

---

## 🎯 Recommendations for ThemisDB

### Deployment Scenarios

#### 1. Cloud Deployment (AWS/Azure/GCP)

**Recommendation: Managed Encryption Services**

```yaml
# AWS Deployment
deployment:
  cloud: aws
  encryption:
    ebs:
      enabled: true
      kms_key: "arn:aws:kms:eu-central-1:123456789012:key/..."
      algorithm: AES-256-XTS
    s3:
      encryption: SSE-KMS
      kms_key: "arn:aws:kms:eu-central-1:123456789012:key/..."
```

**Advantages:**
- ✅ No code changes in ThemisDB
- ✅ Zero performance overhead (hardware encryption)
- ✅ Automatic key rotation
- ✅ Compliance-ready (FIPS 140-2)
- ✅ Audit trail (CloudTrail/Azure Monitor/Cloud Audit Logs)

**Disadvantages:**
- ⚠️ Vendor lock-in
- ⚠️ Additional costs (KMS requests)

#### 2. On-Premise Deployment

**Recommendation: dm-crypt/LUKS with hardware SED**

```bash
# Layer 1: Self-Encrypting Drive (SED)
# - Hardware-based encryption
# - Zero CPU overhead
# - Protection against physical theft

# Layer 2: dm-crypt/LUKS
cryptsetup luksFormat --type luks2 \
  --cipher aes-xts-plain64 \
  --key-size 512 \
  --hash sha256 \
  /dev/sdb

cryptsetup luksOpen /dev/sdb themisdb_data
mkfs.ext4 /dev/mapper/themisdb_data
mount /dev/mapper/themisdb_data /var/lib/themisdb

# Layer 3: ThemisDB field-level encryption
# - Already present
# - AES-256-GCM
# - Granular PII encryption
```

**Multi-Layer Security:**
```
┌───────────────────────────────────────────────┐
│ Layer 3: Field-Level (ThemisDB)               │
│ - PII, PHI, Financial Data                    │
│ - AES-256-GCM                                 │
└───────────────┬───────────────────────────────┘
                │
┌───────────────▼───────────────────────────────┐
│ Layer 2: Filesystem (dm-crypt/LUKS)           │
│ - All RocksDB Files                           │
│ - AES-256-XTS                                 │
└───────────────┬───────────────────────────────┘
                │
┌───────────────▼───────────────────────────────┐
│ Layer 1: Hardware (SED)                       │
│ - Physical Disk Encryption                    │
│ - AES-256-XTS                                 │
└───────────────────────────────────────────────┘
```

**Advantages:**
- ✅ Defense-in-depth
- ✅ No cloud dependency
- ✅ Full control over keys
- ✅ Compliance flexibility

**Disadvantages:**
- ⚠️ More complex key management
- ⚠️ Manual key rotation
- ⚠️ Performance overhead (5-10% with AES-NI)

#### 3. Hybrid Deployment

**Recommendation: dm-crypt + HashiCorp Vault**

```hcl
# Vault Transit Secrets Engine for DEK
resource "vault_mount" "transit" {
  path = "transit"
  type = "transit"
}

resource "vault_transit_secret_backend_key" "themisdb_dek" {
  backend = vault_mount.transit.path
  name    = "themisdb-encryption-key"
  type    = "aes256-gcm96"
}
```

**Advantages:**
- ✅ Centralized key management (Vault)
- ✅ Automatic key rotation
- ✅ Audit logging
- ✅ Multi-cloud ready

---

## 📊 Performance Comparison

| Method | CPU Overhead | Latency Overhead | Throughput Impact | Complexity |
|---------|--------------|------------------|-------------------|-------------|
| **No Encryption** | 0% | +0 µs | 0% | ⭐ |
| **Self-Encrypting Drive** | 0% | +0 µs | 0% | ⭐⭐ |
| **dm-crypt (AES-NI)** | 5-10% | +50-100 µs | 5-10% | ⭐⭐⭐ |
| **fscrypt (AES-NI)** | 2-5% | +20-50 µs | 2-5% | ⭐⭐⭐ |
| **RocksDB Encryption** | 10-15% | +100-200 µs | 10-15% | ⭐⭐⭐⭐ |
| **AWS EBS Encryption** | 0% | +0 µs | 0% | ⭐⭐ |
| **Azure Disk Encryption** | 0% | +0 µs | 0% | ⭐⭐ |
| **GCP Default Encryption** | 0% | +0 µs | 0% | ⭐ |

---

## 🎯 Summary & Recommendation

### For ThemisDB Production Deployments

**Cloud (AWS/Azure/GCP):**
```yaml
Recommendation: Managed Encryption
  - EBS/Azure Disk/Persistent Disk Encryption (enabled)
  - KMS Integration (AWS KMS/Azure Key Vault/Cloud KMS)
  - Field-Level Encryption (already present, keep)
  
Performance: ✅ No overhead
Security: ✅ FIPS 140-2 Level 2+
Complexity: ✅ Minimal (no code change)
Costs: ⚠️ KMS Requests (~$0.03 per 10k requests)
```

**On-Premise:**
```yaml
Recommendation: dm-crypt/LUKS + Optional SED
  - Hardware: Self-Encrypting Drives (if available)
  - OS: dm-crypt/LUKS (always enabled)
  - App: Field-Level Encryption (already present, keep)
  
Performance: ✅ 5-10% Overhead (with AES-NI)
Security: ✅ Defense-in-Depth
Complexity: ⚠️ Moderate (Key Management)
Costs: ✅ No ongoing costs
```

### What ThemisDB Does NOT Need to Implement

- ❌ Custom disk encryption (use OS/hardware)
- ❌ Custom KMS (use cloud KMS or Vault)
- ❌ RocksDB encryption provider (unless special requirements)

### What ThemisDB Should Keep

- ✅ Field-level encryption (AES-256-GCM)
- ✅ TLS for data-in-transit
- ✅ Audit logging with encrypt-then-sign

**Conclusion:** ThemisDB already has a solid encryption strategy at the application level. For at-rest encryption, standardized OS and cloud mechanisms should be used instead of developing custom implementations.

---

**Author:** ThemisDB Security Team  
**Review:** Pending  
**Next Steps:** Create configuration guides (see Phase 2)

## 📚 References

### Hyperscaler Documentation
- [AWS EBS Encryption](https://docs.aws.amazon.com/AWSEC2/latest/UserGuide/EBSEncryption.html)
- [Google Cloud Encryption at Rest](https://cloud.google.com/docs/security/encryption/default-encryption)
- [Azure Storage Service Encryption](https://docs.microsoft.com/en-us/azure/storage/common/storage-service-encryption)

### Linux Kernel Documentation
- [dm-crypt Documentation](https://www.kernel.org/doc/html/latest/admin-guide/device-mapper/dm-crypt.html)
- [fscrypt Design Document](https://www.kernel.org/doc/html/latest/filesystems/fscrypt.html)

### Standards
- **TCG Opal 2.0**: Self-Encrypting Drive Specification
- **FIPS 140-2**: Security Requirements for Cryptographic Modules
- **NIST SP 800-111**: Guide to Storage Encryption Technologies
