# Themis.KeyRotationDashboard - Encryption Key Management

## Overview

Desktop application for managing encryption keys and automating key rotation procedures in ThemisDB. Ensures security best practices and compliance with key management standards.

## Status

🚧 **Under Development** - Structure and specification complete, implementation in progress.

## Use Cases

- Automated encryption key rotation
- Certificate lifecycle management
- Emergency key rotation after security incidents
- Compliance with key management regulations (PCI-DSS, HIPAA, NIST)
- Audit trail for all key operations
- Integration with Hardware Security Modules (HSM)

## Requirements

- .NET 8.0+
- Windows, Linux, or macOS
- Access to ThemisDB server with admin privileges
- Optional: HSM or cloud KMS integration

## Features

### Key Management

- **Multiple Key Types:**
  - Data-at-rest encryption (AES-256)
  - Transport encryption (TLS certificates)
  - Field-level encryption
  - Vector embedding encryption
  - JWT signing keys

- **Key Storage Options:**
  - Local secure storage
  - Cloud KMS (AWS KMS, Azure Key Vault, Google Cloud KMS)
  - HashiCorp Vault
  - Hardware Security Modules (HSM/TPM)

### Rotation Strategies

- **Scheduled Rotation:**
  - Daily, weekly, monthly, or custom intervals
  - Automatic rotation based on usage metrics
  - Configurable rotation windows

- **On-Demand Rotation:**
  - Manual key rotation
  - Emergency rotation for compromised keys
  - Testing and validation before production

- **Zero-Downtime Rotation:**
  - Graceful key transition
  - Dual-key operation during rotation
  - Automatic fallback and rollback

### Compliance & Auditing

- **Standards Compliance:**
  - NIST SP 800-57 (Key Management)
  - FIPS 140-2/140-3
  - PCI-DSS Requirement 3.6
  - HIPAA encryption requirements

- **Audit Trail:**
  - All key operations logged
  - Timestamped and digitally signed
  - Immutable audit log storage
  - Exportable compliance reports

## Installation

```bash
cd tools/Themis.KeyRotationDashboard
dotnet restore
dotnet build
dotnet run
```

## Configuration

Edit `appsettings.json`:

```json
{
  "ThemisDB": {
    "BaseUrl": "http://localhost:8080",
    "BearerToken": "your-admin-token"
  },
  "KeyManagement": {
    "Provider": "local|aws|azure|hashicorp|hsm",
    "RotationInterval": "30d",
    "RetentionPeriod": "90d",
    "BackupEnabled": true
  },
  "AWS": {
    "KMSRegion": "us-east-1",
    "AccessKeyId": "your-access-key",
    "SecretAccessKey": "your-secret-key"
  },
  "Azure": {
    "KeyVaultUrl": "https://your-vault.vault.azure.net/",
    "TenantId": "your-tenant-id",
    "ClientId": "your-client-id"
  },
  "HashiCorp": {
    "VaultAddress": "https://vault.example.com",
    "Token": "vault-token",
    "MountPath": "themisdb"
  }
}
```

## Basic Usage

### Dashboard Overview

The main dashboard displays:
- Active encryption keys
- Key expiration status
- Upcoming rotation schedule
- Recent rotation history
- Compliance status

### Key Rotation Workflow

1. **Review Current Keys**
   - View all active encryption keys
   - Check expiration dates and usage
   - Identify keys due for rotation

2. **Plan Rotation**
   - Select keys to rotate
   - Choose rotation strategy
   - Schedule rotation window
   - Configure notification settings

3. **Execute Rotation**
   - Initiate rotation process
   - Monitor rotation progress
   - Verify new key deployment
   - Validate data accessibility

4. **Audit & Verify**
   - Review rotation logs
   - Generate compliance report
   - Archive old keys securely
   - Update documentation

### Key Operations

#### Create New Key

```csharp
// Programmatic example (API will be implemented)
var keyRequest = new KeyCreationRequest
{
    KeyType = KeyType.DataEncryption,
    Algorithm = "AES-256-GCM",
    Purpose = "Document encryption",
    AutoRotate = true,
    RotationPeriod = TimeSpan.FromDays(90)
};

var keyId = await keyManager.CreateKeyAsync(keyRequest);
```

#### Rotate Existing Key

```csharp
var rotationRequest = new KeyRotationRequest
{
    KeyId = "key-12345",
    RotationStrategy = RotationStrategy.GracefulTransition,
    NotifyAdmins = true
};

await keyManager.RotateKeyAsync(rotationRequest);
```

#### Emergency Key Revocation

```csharp
var revocationRequest = new KeyRevocationRequest
{
    KeyId = "compromised-key-67890",
    Reason = "Security incident - potential compromise",
    ImmediateRevocation = true
};

await keyManager.RevokeKeyAsync(revocationRequest);
```

## Key Rotation Strategies

### Strategy 1: Graceful Transition (Default)

1. Generate new key
2. Deploy new key to all nodes
3. Begin dual-key operation (read with both keys)
4. Gradually re-encrypt data with new key
5. Monitor for any issues
6. Deprecate old key after transition period
7. Archive old key securely

**Use Case:** Production systems requiring zero downtime

### Strategy 2: Immediate Rotation

1. Generate new key
2. Deploy new key to all nodes
3. Immediately switch to new key
4. Re-encrypt all data in background
5. Archive old key

**Use Case:** Low-traffic periods, testing environments

### Strategy 3: Emergency Rotation

1. Immediately revoke compromised key
2. Generate and deploy new key
3. Re-encrypt all affected data
4. Notify stakeholders
5. Document incident

**Use Case:** Security incidents, key compromise

## Integration Examples

### AWS KMS Integration

```json
{
  "KeyManagement": {
    "Provider": "aws",
    "AWS": {
      "Region": "us-east-1",
      "KMSKeyId": "arn:aws:kms:us-east-1:123456789:key/abcd-1234",
      "UseIAMRole": true
    }
  }
}
```

### Azure Key Vault Integration

```json
{
  "KeyManagement": {
    "Provider": "azure",
    "Azure": {
      "KeyVaultUrl": "https://themisdb-vault.vault.azure.net/",
      "UseManagedIdentity": true,
      "KeyName": "themisdb-master-key"
    }
  }
}
```

### HashiCorp Vault Integration

```json
{
  "KeyManagement": {
    "Provider": "hashicorp",
    "HashiCorp": {
      "VaultAddress": "https://vault.company.com",
      "AuthMethod": "kubernetes",
      "MountPath": "themisdb/encryption",
      "KeyPath": "data-encryption"
    }
  }
}
```

## Monitoring & Alerts

### Alert Conditions

- Key expiration warnings (30, 14, 7 days before)
- Rotation failures
- Key access anomalies
- Compliance violations
- HSM connectivity issues

### Notification Channels

- Email notifications
- Slack/Teams integration
- PagerDuty integration
- Webhook callbacks
- SIEM integration

## Compliance Reports

Generate reports for:
- PCI-DSS Requirement 3.6 (Key Management)
- HIPAA Security Rule (Encryption)
- NIST SP 800-57 (Key Management)
- SOC 2 Type II (Encryption Controls)
- ISO 27001 (Cryptographic Controls)

## Troubleshooting

### Common Issues

**Issue**: Rotation fails with "key in use" error
```
Solution: Ensure graceful transition period is configured.
Check for long-running queries or transactions.
```

**Issue**: HSM connectivity timeout
```
Solution: Verify network connectivity to HSM.
Check HSM partition credentials.
Review HSM logs for detailed errors.
```

**Issue**: Permission denied when accessing cloud KMS
```
Solution: Verify IAM roles/policies.
Check service principal permissions.
Ensure correct authentication method.
```

## Security Considerations

1. **Access Control**
   - Restrict dashboard access to security admins only
   - Use multi-factor authentication
   - Implement IP whitelisting

2. **Key Storage**
   - Never store keys in plain text
   - Use HSM when available
   - Encrypt key backups

3. **Audit Logging**
   - Log all key operations
   - Store logs in immutable storage
   - Regular log review and analysis

4. **Incident Response**
   - Document emergency procedures
   - Test emergency rotation regularly
   - Maintain offline key backups

## Performance Considerations

- Key rotation impact: ~1-5% CPU overhead during re-encryption
- Typical rotation time: 10-60 minutes depending on data size
- Recommended rotation window: During low-traffic periods
- Concurrent operations: Limited to prevent resource exhaustion

## API Reference

*(To be implemented)*

```csharp
// Key rotation API endpoints
POST /api/v1/admin/keys/rotate
GET  /api/v1/admin/keys/{keyId}/status
GET  /api/v1/admin/keys/rotation-schedule
POST /api/v1/admin/keys/{keyId}/revoke
GET  /api/v1/admin/keys/audit-log
```

## See Also

- [PII Manager](pii-manager.md) - Data privacy management
- [Compliance Reports](compliance-reports.md) - Compliance reporting
- [Admin Tools Overview](ADMIN_TOOLS_OVERVIEW.md) - Complete admin tools guide
- [TOOLS_INDEX.md](../../TOOLS_INDEX.md) - All ThemisDB tools

## Contributing

Contributions welcome! Please ensure:
- Follow .NET coding standards
- Include unit tests
- Document security considerations
- Test with multiple KMS providers

## License

Apache 2.0 - See [LICENSE](../../../LICENSE) for details.
