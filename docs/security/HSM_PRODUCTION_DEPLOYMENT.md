# HSM Provider Production Deployment Guide

This guide covers production deployment of ThemisDB with Hardware Security Module (HSM) integration.

## Table of Contents
- [Prerequisites](#prerequisites)
- [Architecture Planning](#architecture-planning)
- [Installation](#installation)
- [Configuration](#configuration)
- [Testing](#testing)
- [Monitoring](#monitoring)
- [Disaster Recovery](#disaster-recovery)
- [Security Hardening](#security-hardening)
- [Performance Tuning](#performance-tuning)

---

## Prerequisites

### Hardware Requirements

**Minimum:**
- 4 CPU cores
- 16 GB RAM
- 100 GB SSD storage
- 1 Gbps network (for network HSMs)

**Recommended (Production):**
- 16+ CPU cores
- 64 GB RAM
- 500 GB NVMe SSD storage
- 10 Gbps network
- Redundant power supplies
- RAID storage

### Software Requirements

- **Operating System**: Ubuntu 20.04/22.04 LTS, RHEL 8+, or Amazon Linux 2
- **Compiler**: GCC 11+ or Clang 13+
- **CMake**: 3.20 or higher
- **OpenSSL**: 3.0 or higher
- **HSM Client Software**: Vendor-specific (see [HSM_VENDOR_CONFIGURATIONS.md](HSM_VENDOR_CONFIGURATIONS.md))

### Network Requirements

- Dedicated HSM network (recommended)
- Firewall rules for HSM traffic
- NTP synchronization
- DNS resolution for HSM hostnames

### Compliance Requirements

Consider your compliance needs:
- **PCI-DSS**: HSM must be FIPS 140-2 Level 2+
- **HIPAA**: FIPS 140-2 Level 2+ with audit logging
- **SOC 2**: Full audit trail of key operations
- **GDPR**: Encryption key management and access controls

---

## Architecture Planning

### Deployment Topologies

#### 1. Single HSM (Development/Testing)

```
┌─────────────────┐
│  ThemisDB       │
│  Application    │
└────────┬────────┘
         │ PKCS#11
         ▼
┌─────────────────┐
│   SoftHSM2      │
│  (Test Only)    │
└─────────────────┘
```

**Use Case**: Development, testing, CI/CD
**Pros**: Simple, cost-effective
**Cons**: No redundancy, not production-ready

#### 2. Single Production HSM

```
┌─────────────────┐
│  ThemisDB       │
│  Server         │
└────────┬────────┘
         │ PKCS#11
         ▼
┌─────────────────┐
│  SafeNet Luna   │
│  Network HSM    │
└─────────────────┘
```

**Use Case**: Small production deployments
**Pros**: Enterprise-grade security
**Cons**: Single point of failure

#### 3. High-Availability HSM Cluster

```
┌─────────────┐      ┌─────────────┐
│ ThemisDB    │      │ ThemisDB    │
│ Primary     │      │ Secondary   │
└──────┬──────┘      └──────┬──────┘
       │                    │
       │   ┌────────────────┘
       │   │
       ▼   ▼
┌──────────────────┐   ┌──────────────────┐
│ HSM Primary      │───│ HSM Secondary    │
│ (Active)         │   │ (Standby)        │
└──────────────────┘   └──────────────────┘
```

**Use Case**: Production with HA requirements
**Pros**: Automatic failover, no single point of failure
**Cons**: Higher cost, more complex

#### 4. Multi-Region Cloud HSM

```
Region 1 (Primary)              Region 2 (DR)
┌─────────────────┐            ┌─────────────────┐
│  ThemisDB       │            │  ThemisDB       │
│  Cluster        │            │  Standby        │
└────────┬────────┘            └────────┬────────┘
         │                              │
         ▼                              ▼
┌─────────────────┐            ┌─────────────────┐
│  AWS CloudHSM   │───Sync───▶ │  AWS CloudHSM   │
│  Cluster        │            │  Cluster        │
└─────────────────┘            └─────────────────┘
```

**Use Case**: Global deployments, disaster recovery
**Pros**: Geographic redundancy, low latency
**Cons**: Complex key synchronization, highest cost

### Capacity Planning

Estimate your HSM requirements:

**Signing Operations:**
- Average: 100-500 operations/sec per HSM
- Peak: Consider 3x average for capacity planning
- Example: 1000 ops/sec peak = 6-10 sessions recommended

**Session Pool Size:**
| Workload | Sessions | Notes |
|----------|----------|-------|
| Low (< 10 ops/sec) | 2-4 | Development, light prod |
| Medium (10-100 ops/sec) | 8-16 | Typical production |
| High (100-1000 ops/sec) | 16-32 | High-throughput |
| Very High (> 1000 ops/sec) | 32+ or multiple HSMs | Consider load balancing |

---

## Installation

### 1. Install ThemisDB

```bash
# Clone repository
git clone https://github.com/makr-code/ThemisDB.git
cd ThemisDB

# Build with HSM support
cmake -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DTHEMIS_ENABLE_HSM_REAL=ON \
  -DTHEMIS_BUILD_TESTS=ON

cmake --build build --parallel $(nproc)
```

### 2. Install HSM Client Software

See [HSM_VENDOR_CONFIGURATIONS.md](HSM_VENDOR_CONFIGURATIONS.md) for vendor-specific instructions.

### 3. Verify Installation

```bash
# Test PKCS#11 library
pkcs11-tool --module /path/to/pkcs11.so --list-slots

# Test ThemisDB HSM provider
./build/tests/test_hsm_provider
```

---

## Configuration

### 1. Create HSM Configuration File

Create `/etc/themisdb/hsm_config.json`:

```json
{
  "hsm": {
    "enabled": true,
    "library_path": "/usr/safenet/lunaclient/lib/libCryptoki2_64.so",
    "slot_id": 0,
    "token_label": "themis-prod",
    "key_label": "themis-master-key",
    "signature_algorithm": "RSA-SHA256",
    "session_pool_size": 16,
    "verbose": false
  },
  "failover": {
    "enabled": true,
    "secondary_hsm": {
      "library_path": "/usr/safenet/lunaclient/lib/libCryptoki2_64.so",
      "slot_id": 1,
      "token_label": "themis-prod-dr"
    },
    "retry_attempts": 3,
    "retry_delay_ms": 1000
  },
  "audit": {
    "enabled": true,
    "log_all_operations": true,
    "log_path": "/var/log/themisdb/hsm_audit.log",
    "syslog": true
  }
}
```

### 2. Set Environment Variables

```bash
# Add to /etc/systemd/system/themisdb.service.d/hsm.conf
[Service]
Environment="THEMIS_HSM_PIN=your-secure-pin"
Environment="THEMIS_HSM_SESSION_POOL=16"
Environment="THEMIS_HSM_LIBRARY=/usr/safenet/lunaclient/lib/libCryptoki2_64.so"
```

**Security Note**: Use systemd credentials or HashiCorp Vault for PIN management in production!

### 3. Generate Production Keys

```bash
# Using HSMProvider API
themisdb-admin hsm generate-keypair \
  --label themis-master-key \
  --key-size 4096 \
  --non-extractable

# Or using PKCS#11 tools
pkcs11-tool --module /path/to/pkcs11.so \
  --login --pin $THEMIS_HSM_PIN \
  --keypairgen --key-type RSA:4096 \
  --label themis-master-key \
  --usage-sign
```

### 4. Import Certificates (if applicable)

```bash
themisdb-admin hsm import-cert \
  --label themis-master-key \
  --cert-file /path/to/certificate.pem
```

---

## Testing

### Pre-Production Testing Checklist

#### 1. Functional Tests

```bash
# Run HSM provider tests
ctest -R hsm --output-on-failure

# Test signing operations
themisdb-test hsm-sign --iterations 1000

# Test verification
themisdb-test hsm-verify --iterations 1000
```

#### 2. Performance Testing

```bash
# Benchmark signing operations
./build/benchmarks/bench_hsm_provider --benchmark_repetitions=5

# Expected results (varies by HSM):
# - SoftHSM2: 500-2000 ops/sec
# - SafeNet Luna: 100-500 ops/sec
# - AWS CloudHSM: 200-1000 ops/sec
```

#### 3. Failover Testing

```bash
# Test primary HSM failure
systemctl stop hsm-primary
# Verify ThemisDB fails over to secondary
tail -f /var/log/themisdb/hsm_audit.log

# Test recovery
systemctl start hsm-primary
# Verify ThemisDB reconnects to primary
```

#### 4. Load Testing

```bash
# Sustained load test
themisdb-load-test \
  --duration 3600 \
  --operations sign,verify \
  --rate 100 \
  --hsm-enabled

# Monitor:
# - HSM CPU usage
# - Session utilization
# - Error rates
# - Latency percentiles
```

---

## Monitoring

### Key Metrics to Monitor

#### HSM Metrics

```cpp
// Get performance statistics
HSMPerformanceStats stats = hsm->getStats();

// Monitor these metrics:
// - stats.sign_count: Total sign operations
// - stats.verify_count: Total verify operations  
// - stats.sign_errors: Failed sign operations
// - stats.verify_errors: Failed verify operations
// - stats.total_sign_time_us: Cumulative sign latency
// - stats.total_verify_time_us: Cumulative verify latency
// - stats.pool_size: Configured session pool size
// - stats.pool_round_robin_hits: Session pool effectiveness
```

#### Recommended Alerts

1. **High Error Rate**
   ```
   Metric: (hsm_sign_errors + hsm_verify_errors) / (hsm_sign_count + hsm_verify_count)
   Threshold: > 1%
   Action: Check HSM connection, logs
   ```

2. **High Latency**
   ```
   Metric: p99(hsm_operation_latency_ms)
   Threshold: > 100ms
   Action: Check HSM load, network latency
   ```

3. **Session Pool Exhaustion**
   ```
   Metric: hsm_session_pool_utilization
   Threshold: > 90%
   Action: Increase session_pool_size
   ```

4. **HSM Availability**
   ```
   Metric: hsm_health_check
   Threshold: failing
   Action: Check HSM connectivity, failover
   ```

### Monitoring Integration

#### Prometheus

```cpp
// Export HSM metrics to Prometheus
#include <prometheus/counter.h>
#include <prometheus/histogram.h>

prometheus::Counter& hsm_signs = prometheus::BuildCounter()
    .Name("themisdb_hsm_sign_operations_total")
    .Register(*registry);

prometheus::Histogram& hsm_sign_latency = prometheus::BuildHistogram()
    .Name("themisdb_hsm_sign_latency_seconds")
    .Register(*registry);
```

#### CloudWatch (AWS)

```bash
aws cloudwatch put-metric-data \
  --namespace ThemisDB/HSM \
  --metric-name SignOperations \
  --value ${sign_count} \
  --timestamp $(date -u +%Y-%m-%dT%H:%M:%S)
```

#### Grafana Dashboard

Import the ThemisDB HSM dashboard:
- Dashboard ID: `themisdb-hsm-ops`
- Panels: Sign/Verify ops, Latency, Error rate, Session utilization

---

## Disaster Recovery

### Backup Strategy

#### 1. Key Backup

**Method 1: HSM-to-HSM Replication**
```bash
# Configure HSM replication (vendor-specific)
# SafeNet Luna example:
lunacm> partition synchronize -partition themis-prod
```

**Method 2: Secure Key Export**
```bash
# Export wrapped key (encrypted with KEK)
themisdb-admin hsm export-key \
  --label themis-master-key \
  --output /secure/backup/master-key.wrapped \
  --wrap-key backup-kek

# Store in secure location (offline, encrypted)
```

#### 2. Configuration Backup

```bash
# Backup HSM configuration
tar czf /backup/themisdb-hsm-config-$(date +%Y%m%d).tar.gz \
  /etc/themisdb/hsm_config.json \
  /etc/systemd/system/themisdb.service.d/

# Encrypt backup
gpg --encrypt --recipient admin@company.com \
  /backup/themisdb-hsm-config-*.tar.gz
```

#### 3. Certificate Backup

```bash
# Export certificates from HSM
themisdb-admin hsm export-cert \
  --label themis-master-key \
  --output /backup/certs/themis-cert-$(date +%Y%m%d).pem
```

### Recovery Procedures

#### Scenario 1: Primary HSM Failure

```bash
# 1. Verify secondary HSM is active
themisdb-admin hsm status --slot 1

# 2. Update configuration to use secondary
sed -i 's/slot_id": 0/slot_id": 1/' /etc/themisdb/hsm_config.json

# 3. Restart ThemisDB
systemctl restart themisdb

# 4. Verify operations
themisdb-test hsm-sign --iterations 10
```

#### Scenario 2: Complete HSM Loss

```bash
# 1. Provision new HSM
# 2. Initialize HSM with secure parameters
# 3. Import wrapped key
themisdb-admin hsm import-key \
  --input /secure/backup/master-key.wrapped \
  --unwrap-key backup-kek \
  --label themis-master-key

# 4. Import certificate
themisdb-admin hsm import-cert \
  --label themis-master-key \
  --cert-file /backup/certs/themis-cert-latest.pem

# 5. Test and validate
themisdb-test hsm-full-suite

# 6. Resume operations
systemctl start themisdb
```

#### Scenario 3: Key Compromise

```bash
# 1. Generate new key pair
themisdb-admin hsm generate-keypair \
  --label themis-master-key-v2 \
  --key-size 4096 \
  --non-extractable

# 2. Request new certificate
# 3. Update application configuration
# 4. Rotate keys across all services
# 5. Revoke compromised certificate
# 6. Destroy old key
themisdb-admin hsm delete-key --label themis-master-key
```

### Recovery Time Objectives (RTO)

| Scenario | Target RTO | Recovery Steps |
|----------|-----------|----------------|
| Primary HSM down | < 5 min | Automatic failover to secondary |
| Primary HSM failed | < 30 min | Manual failover to DR HSM |
| Complete HSM loss | < 4 hours | Provision new HSM, restore from backup |
| Key compromise | < 24 hours | Generate new keys, certificate rotation |

---

## Security Hardening

### 1. Access Control

```bash
# Restrict HSM configuration file permissions
chmod 600 /etc/themisdb/hsm_config.json
chown themisdb:themisdb /etc/themisdb/hsm_config.json

# Restrict service account privileges
usermod -s /usr/sbin/nologin themisdb
```

### 2. Network Security

```bash
# Configure firewall for HSM traffic only
iptables -A OUTPUT -p tcp -d <hsm-ip> --dport 2223:2225 -j ACCEPT
iptables -A OUTPUT -p tcp -d <hsm-ip> -j DROP

# Enable SELinux/AppArmor for ThemisDB process
```

### 3. Audit Logging

```cpp
// Enable comprehensive audit logging
HSMConfig config;
config.audit_enabled = true;
config.audit_log_path = "/var/log/themisdb/hsm_audit.log";
config.audit_syslog = true;  // Also log to syslog

// All operations are logged:
// - HSM initialization
// - Key generation
// - Sign/verify operations
// - Key access
// - Errors and failures
```

### 4. PIN/Password Management

**Best Practices:**
1. **Never hardcode PINs** in configuration files or code
2. **Use environment variables** or secure key stores (HashiCorp Vault, AWS Secrets Manager)
3. **Rotate PINs** every 90 days
4. **Use strong PINs**: 12+ characters, alphanumeric + symbols
5. **Separate PINs** for dev/staging/production

**Example with HashiCorp Vault:**
```bash
# Store HSM PIN in Vault
vault kv put secret/themisdb/hsm pin="$(openssl rand -base64 24)"

# Retrieve in application
export THEMIS_HSM_PIN=$(vault kv get -field=pin secret/themisdb/hsm)
```

### 5. Key Lifecycle Policies

```json
{
  "key_policies": {
    "master_keys": {
      "algorithm": "RSA-4096",
      "rotation_period_days": 365,
      "extractable": false,
      "backup_required": true
    },
    "operational_keys": {
      "algorithm": "RSA-2048", 
      "rotation_period_days": 90,
      "extractable": false,
      "backup_required": false
    }
  }
}
```

---

## Performance Tuning

### 1. Session Pool Optimization

```cpp
// Tune session pool based on workload
HSMConfig config;
config.session_pool_size = 16;  // Start here

// Monitor pool utilization
auto stats = hsm->getStats();
double utilization = stats.pool_round_robin_hits / (double)stats.sign_count;

// If utilization > 90%, increase pool size
// If utilization < 50%, decrease pool size
```

### 2. Connection Optimization

```cpp
// For network HSMs, tune TCP parameters
// /etc/sysctl.conf
net.ipv4.tcp_keepalive_time = 60
net.ipv4.tcp_keepalive_intvl = 10
net.ipv4.tcp_keepalive_probes = 6
```

### 3. Caching Strategy

```cpp
// Cache frequently-used public keys/certificates
std::unordered_map<std::string, std::string> cert_cache;

std::string get_certificate(const std::string& key_label) {
    if (cert_cache.count(key_label)) {
        return cert_cache[key_label];
    }
    auto cert = hsm->getCertificate(key_label);
    if (cert) {
        cert_cache[key_label] = *cert;
        return *cert;
    }
    throw std::runtime_error("Certificate not found");
}
```

### 4. Batch Operations

```cpp
// Batch multiple sign operations when possible
std::vector<std::vector<uint8_t>> data_batch = {...};
std::vector<HSMSignatureResult> results;

for (const auto& data : data_batch) {
    results.push_back(hsm->sign(data));
}

// More efficient than individual operations with session reuse
```

### Expected Performance

| Metric | SoftHSM2 | SafeNet Luna | AWS CloudHSM |
|--------|----------|--------------|--------------|
| Sign ops/sec (RSA-2048) | 500-2000 | 100-500 | 200-1000 |
| Sign ops/sec (RSA-4096) | 200-800 | 50-200 | 100-400 |
| Verify ops/sec | 2000-8000 | 500-2000 | 1000-4000 |
| Latency p50 (ms) | 1-5 | 10-20 | 5-15 |
| Latency p99 (ms) | 10-20 | 50-100 | 20-50 |
| Max sessions | Unlimited | 2048 | 2048 |

---

## Troubleshooting

See [HSM_VENDOR_CONFIGURATIONS.md](HSM_VENDOR_CONFIGURATIONS.md#troubleshooting) for common issues and solutions.

---

## Appendix

### A. Pre-Deployment Checklist

- [ ] HSM hardware/service provisioned
- [ ] HSM client software installed and configured
- [ ] ThemisDB built with HSM support
- [ ] Keys generated in HSM
- [ ] Certificates imported
- [ ] Configuration files created and secured
- [ ] PINs/passwords stored securely
- [ ] Audit logging configured
- [ ] Monitoring and alerting set up
- [ ] Backup procedures documented
- [ ] Disaster recovery plan tested
- [ ] Security review completed
- [ ] Performance testing passed
- [ ] Documentation updated
- [ ] Operations team trained

### B. Production Launch Day

1. **T-1 hour**: Final verification of all systems
2. **T-30 min**: Start monitoring dashboards
3. **T-15 min**: Verify HSM connectivity
4. **T-0**: Start ThemisDB with HSM enabled
5. **T+15 min**: Verify first operations successful
6. **T+1 hour**: Review metrics and logs
7. **T+24 hours**: Full operational review

### C. Support Resources

- **Documentation**: https://github.com/makr-code/ThemisDB/docs
- **Community**: ThemisDB Slack/Discord
- **Professional Support**: support@themisdb.com
- **Security Issues**: security@themisdb.com

---

## Revision History

- **v1.0** (2026-01-22): Initial production deployment guide
- HSM Provider PKCS#11 integration - v1.3.1

