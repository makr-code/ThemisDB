# Vault Setup for LoRA Adapter Encryption

This guide explains how to set up HashiCorp Vault for encrypting LoRA adapter weights in ThemisDB.

## Overview

ThemisDB supports production-ready encryption for LoRA adapter weights using HashiCorp Vault for key management. This replaces the `MockKeyProvider` used in development with a secure, enterprise-grade solution.

## Prerequisites

- HashiCorp Vault 1.15+ (running and accessible)
- Network connectivity from ThemisDB to Vault server
- Valid Vault authentication token or credentials

## Quick Start

### 1. Enable KV v2 Secrets Engine

```bash
# Enable KV v2 secrets engine at "themis" mount path
vault secrets enable -version=2 -path=themis kv
```

### 2. Store Encryption Key

```bash
# Generate and store a 256-bit encryption key for LoRA adapters
vault kv put themis/keys/lora_adapters \
  key=$(openssl rand -base64 32) \
  algorithm="AES-256-GCM" \
  version=1
```

### 3. Create Vault Policy

Create a policy file `themis-lora-policy.hcl`:

```hcl
# Read access to LoRA adapter encryption keys
path "themis/data/keys/lora_adapters" {
  capabilities = ["read"]
}

# Read metadata for key versioning
path "themis/metadata/keys/lora_adapters" {
  capabilities = ["read"]
}
```

Apply the policy:

```bash
vault policy write themis-lora themis-lora-policy.hcl
```

### 4. Generate Service Token

```bash
# Generate a token with the themis-lora policy
vault token create -policy=themis-lora -period=24h

# Output will include:
# Key                  Value
# ---                  -----
# token                hvs.CAESI...
# token_accessor       abc123...
# token_duration       24h
```

**Important:** Store the token securely. This token will be used by ThemisDB to authenticate with Vault.

### 5. Configure ThemisDB

#### Option A: Configuration File

Update your ThemisDB configuration:

```cpp
LoRAStorageService::Config config;
config.enable_encryption = true;
config.use_vault_for_encryption = true;
config.vault_addr = "http://localhost:8200";  // Your Vault server
config.vault_token = "hvs.CAESI...";          // Token from step 4
config.vault_kv_mount = "themis";             // KV mount path
config.encryption_key_id = "lora_adapters";   // Key ID in Vault
```

#### Option B: Environment Variables

Set environment variables:

```bash
export VAULT_ADDR="http://localhost:8200"
export VAULT_TOKEN="hvs.CAESI..."
```

Then configure ThemisDB:

```cpp
LoRAStorageService::Config config;
config.enable_encryption = true;
config.use_vault_for_encryption = true;
config.vault_kv_mount = "themis";
config.encryption_key_id = "lora_adapters";
// vault_addr and vault_token will be read from environment
```

## Key Rotation

### Why Rotate Keys?

- Comply with security policies (e.g., rotate every 90 days)
- Mitigate risk if a key is compromised
- Meet compliance requirements (PCI-DSS, HIPAA, etc.)

### How Key Rotation Works

ThemisDB uses **envelope encryption** with automatic key versioning:

1. Each encrypted adapter stores the key version used
2. Old adapters can still be decrypted with their original key version
3. New adapters use the latest active key version
4. No data migration required!

### Perform Key Rotation

```bash
# Create a new version of the key
vault kv put themis/keys/lora_adapters \
  key=$(openssl rand -base64 32) \
  algorithm="AES-256-GCM" \
  version=2

# Vault automatically marks the new version as active
# Old adapters will still decrypt with version 1
# New adapters will encrypt with version 2
```

### Verify Key Versions

```bash
# List all versions of the key
vault kv metadata get themis/keys/lora_adapters

# Get specific version
vault kv get -version=1 themis/keys/lora_adapters
```

## Security Best Practices

### 1. Use TLS in Production

Enable TLS for Vault connections:

```cpp
config.vault_addr = "https://vault.example.com:8200";  // Use HTTPS
```

Configure Vault with a valid TLS certificate:

```bash
vault server \
  -config=/etc/vault/config.hcl \
  -tls-cert-file=/path/to/cert.pem \
  -tls-key-file=/path/to/key.pem
```

### 2. Least Privilege Access

- Use separate tokens for each service
- Grant only necessary permissions (read-only for ThemisDB)
- Avoid using root tokens in production

### 3. Token Management

**Token Rotation:**

```bash
# Generate a new token before the old one expires
vault token create -policy=themis-lora -period=24h

# Update ThemisDB configuration with new token
# Restart ThemisDB or use dynamic reloading if available
```

**Token Lifecycle:**

- Use periodic tokens with automatic renewal
- Set appropriate TTL (e.g., 24 hours)
- Monitor token expiration

### 4. Audit Logging

Enable Vault audit logging:

```bash
vault audit enable file file_path=/var/log/vault/audit.log
```

Monitor audit logs for:
- Failed authentication attempts
- Unauthorized key access
- Unusual access patterns

## Monitoring

### Key Retrieval Metrics

ThemisDB logs key retrieval statistics:

```
[INFO] Using VaultKeyProvider for encryption (Production-Ready)
[DEBUG] Vault Address: http://localhost:8200
[DEBUG] KV Mount: themis
[DEBUG] Key ID: lora_adapters
[DEBUG] Encrypted adapter data with key version 2
```

### Performance Expectations

- **Cold fetch** (first request): ~50-100ms
- **Cached fetch** (subsequent): <0.1ms
- **Cache TTL**: 1 hour (configurable)
- **Cache hit rate**: >95% expected

### Health Checks

Monitor Vault connectivity:

```bash
# Check Vault status
vault status

# Test key retrieval
vault kv get themis/keys/lora_adapters
```

## Troubleshooting

### Connection Failures

**Error:** "Vault connection failed (transient)"

**Solutions:**
1. Check Vault server is running: `vault status`
2. Verify network connectivity: `curl http://localhost:8200/v1/sys/health`
3. Check firewall rules

### Authentication Failures

**Error:** "Vault initialization failed: 403 Forbidden"

**Solutions:**
1. Verify token is valid: `vault token lookup`
2. Check token has required policy: `vault token lookup -format=json | jq .data.policies`
3. Regenerate token if expired

### Key Not Found

**Error:** "Key not found: lora_adapters"

**Solutions:**
1. Verify key exists: `vault kv get themis/keys/lora_adapters`
2. Check KV mount path matches configuration
3. Ensure key_id matches Vault path

### Decryption Failures

**Error:** "Decryption failed: Key version 1 not found"

**Solutions:**
1. Verify all key versions are available: `vault kv metadata get themis/keys/lora_adapters`
2. Don't delete old key versions while adapters still use them
3. Check Vault audit logs for access denials

## Production Deployment Checklist

- [ ] Vault server deployed with TLS
- [ ] KV v2 secrets engine enabled
- [ ] Encryption keys generated and stored
- [ ] Vault policies created with least privilege
- [ ] Service tokens generated and distributed securely
- [ ] ThemisDB configured with Vault credentials
- [ ] Connection tested and verified
- [ ] Audit logging enabled
- [ ] Monitoring and alerting configured
- [ ] Key rotation schedule defined
- [ ] Backup and disaster recovery plan documented

## Example Configuration

### Development (Docker)

```bash
# Start Vault in dev mode
docker run --rm --cap-add=IPC_LOCK \
  -e 'VAULT_DEV_ROOT_TOKEN_ID=myroot' \
  -p 8200:8200 \
  vault

# Configure ThemisDB
export VAULT_ADDR="http://localhost:8200"
export VAULT_TOKEN="myroot"
```

### Production (Kubernetes)

```yaml
apiVersion: v1
kind: Secret
metadata:
  name: themisdb-vault-token
type: Opaque
stringData:
  token: "hvs.CAESI..."

---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: themisdb
spec:
  template:
    spec:
      containers:
      - name: themisdb
        env:
        - name: VAULT_ADDR
          value: "https://vault.example.com:8200"
        - name: VAULT_TOKEN
          valueFrom:
            secretKeyRef:
              name: themisdb-vault-token
              key: token
```

## References

- [HashiCorp Vault KV v2](https://www.vaultproject.io/docs/secrets/kv/kv-v2)
- [Vault Authentication](https://www.vaultproject.io/docs/auth)
- [Vault Policies](https://www.vaultproject.io/docs/concepts/policies)
- [ThemisDB Security Documentation](../SECURITY.md)

## Support

For issues or questions:
- GitHub Issues: https://github.com/makr-code/ThemisDB/issues
- Security issues: See [SECURITY.md](../../../SECURITY.md)
