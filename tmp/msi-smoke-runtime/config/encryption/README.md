# ThemisDB Encryption Configuration Examples

This directory contains configuration examples and setup scripts for various encryption-at-rest scenarios.

## 📁 Files

| File | Description | Platform |
|------|-------------|----------|
| `dm-crypt-setup.sh` | Automated setup script for LUKS/dm-crypt encrypted volumes | Linux (On-Premise) |
| `aws-ebs-encryption-example.yaml` | Terraform/CloudFormation examples for AWS EBS encryption | AWS Cloud |

## 🚀 Quick Start

### On-Premise Deployment (Linux)

Use the `dm-crypt-setup.sh` script to set up encrypted storage:

```bash
# Make script executable
chmod +x dm-crypt-setup.sh

# Run setup (requires root)
sudo ./dm-crypt-setup.sh /dev/sdb /var/lib/themisdb
```

**Features:**
- LUKS2 encryption with AES-256-XTS
- Automatic key generation
- Auto-mount configuration
- Performance benchmarking
- AES-NI detection

### AWS Cloud Deployment

Use the Terraform or CloudFormation templates in `aws-ebs-encryption-example.yaml`:

```bash
# Terraform deployment
cd /path/to/themisdb/infrastructure
terraform init
terraform apply -var="instance_type=r6i.2xlarge"

# Verify encryption
aws ec2 describe-volumes \
  --filters "Name=attachment.instance-id,Values=<instance-id>" \
  --query 'Volumes[*].[VolumeId,Encrypted,KmsKeyId]'
```

**Features:**
- AWS KMS integration
- Encrypted EBS volumes (root + data)
- Automated snapshots
- IAM roles and policies
- S3 backup encryption

## 📚 Related Documentation

- [At-Rest Encryption Research](../../docs/en/security/at_rest_encryption_research.md) - Comprehensive analysis
- [At-Rest Verschlüsselung (DE)](../../docs/de/security/security_at_rest_encryption_research.md) - Deutsche Version
- [Encryption Strategy](../../docs/security/encryption_strategy.md) - Overall encryption strategy

## 🔐 Security Best Practices

### Key Management

**On-Premise:**
- Backup LUKS keys to secure offline storage
- Use hardware security modules (HSM) for production
- Implement key rotation schedule (annually)
- Document key recovery procedures

**Cloud:**
- Enable automatic key rotation in KMS
- Use separate keys for different environments
- Implement least-privilege IAM policies
- Enable audit logging (CloudTrail)

### Performance Optimization

**On-Premise:**
- Ensure AES-NI is enabled: `grep aes /proc/cpuinfo`
- Use SSDs with TRIM support
- Mount with `noatime,nodiratime,discard` flags
- Monitor performance: `iostat -x 1`

**Cloud:**
- Use io2 volumes for production databases
- Enable EBS optimization on instances
- Monitor IOPS and throughput metrics
- Use provisioned IOPS for consistent performance

## 🎯 Deployment Scenarios

### Scenario 1: Development/Testing

```yaml
encryption:
  type: dm-crypt
  cipher: aes-xts-plain64
  key_size: 256
  filesystem: ext4
  performance_priority: low
```

### Scenario 2: Production On-Premise

```yaml
encryption:
  layer1:  # Hardware
    type: self-encrypting-drive
    standard: TCG Opal 2.0
  layer2:  # OS
    type: dm-crypt
    cipher: aes-xts-plain64
    key_size: 512
    key_management: hashicorp-vault
  layer3:  # Application
    type: field-level
    algorithm: aes-256-gcm
```

### Scenario 3: Cloud Production

```yaml
encryption:
  provider: aws
  ebs:
    enabled: true
    kms_key: arn:aws:kms:eu-central-1:xxx:key/xxx
    volume_type: io2
    iops: 64000
  s3:
    encryption: sse-kms
    bucket_key: enabled
  rds:  # If using RDS for metadata
    storage_encrypted: true
```

## 📊 Performance Comparison

| Method | CPU Overhead | Latency | Throughput Loss | Setup Complexity |
|--------|--------------|---------|-----------------|------------------|
| No Encryption | 0% | +0 µs | 0% | ⭐ |
| SED (TCG Opal) | 0% | +0 µs | 0% | ⭐⭐ |
| dm-crypt + AES-NI | 5-10% | +50-100 µs | 5-10% | ⭐⭐⭐ |
| AWS EBS | 0% | +0 µs | 0% | ⭐⭐ |
| GCP Default | 0% | +0 µs | 0% | ⭐ |

## 🧪 Testing

### Verify Encryption

**On-Premise (dm-crypt):**
```bash
# Check LUKS header
sudo cryptsetup luksDump /dev/sdb

# Verify encryption is active
sudo dmsetup table
```

**AWS:**
```bash
# Check EBS encryption status
aws ec2 describe-volumes \
  --volume-ids vol-xxxxx \
  --query 'Volumes[0].Encrypted'
```

### Performance Testing

```bash
# On-Premise: Test encrypted volume
cd /var/lib/themisdb
dd if=/dev/zero of=testfile bs=1M count=1024 conv=fsync
dd if=testfile of=/dev/null bs=1M

# Expected results with AES-NI:
# Write: ~1700 MB/s (unencrypted: ~1900 MB/s) = 10% overhead
# Read:  ~1800 MB/s (unencrypted: ~1900 MB/s) = 5% overhead
```

## 🆘 Troubleshooting

### Common Issues

**Issue:** "Device is busy" during setup
```bash
# Solution: Unmount and close existing mappings
sudo umount /dev/sdb1
sudo cryptsetup luksClose themisdb_encrypted
```

**Issue:** "No AES-NI detected"
```bash
# Solution: Enable AES-NI in BIOS/UEFI
# Verify: grep aes /proc/cpuinfo
```

**Issue:** AWS KMS throttling
```bash
# Solution: Use bucket keys for S3
# Enable in Terraform:
resource "aws_s3_bucket_server_side_encryption_configuration" "example" {
  rule {
    bucket_key_enabled = true
  }
}
```

## 📞 Support

For issues or questions:
- [GitHub Issues](https://github.com/makr-code/ThemisDB/issues)
- [Security Policy](../../SECURITY.md)
- [Documentation](../../docs/en/security/)

---

**Version:** 1.0.0  
**Last Updated:** 2026-02-17  
**Maintained By:** ThemisDB Security Team
