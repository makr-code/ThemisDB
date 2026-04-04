# HSM Vendor Configurations

This document provides vendor-specific configuration guides for Hardware Security Module (HSM) integration with ThemisDB.

## Table of Contents
- [SoftHSM2 (Testing/Development)](#softhsm2-testingdevelopment)
- [SafeNet Luna HSM](#safenet-luna-hsm)
- [Yubico YubiHSM2](#yubico-yubihsm2)
- [AWS CloudHSM](#aws-cloudhsm)
- [Common Configuration](#common-configuration)

---

## SoftHSM2 (Testing/Development)

SoftHSM2 is a software implementation of PKCS#11 for testing and development.

### Installation

#### Ubuntu/Debian
```bash
sudo apt-get update
sudo apt-get install softhsm2
```

#### macOS
```bash
brew install softhsm
```

#### Windows
Download from: https://github.com/opendnssec/SoftHSMv2/releases

### Configuration

1. **Initialize Token**
```bash
# Initialize token on slot 0
softhsm2-util --init-token --slot 0 --label "themis-test" --pin 1234 --so-pin 5678

# Verify token
softhsm2-util --show-slots
```

2. **Generate Test Key**
```bash
# Generate RSA 2048-bit key pair
pkcs11-tool --module /usr/lib/softhsm/libsofthsm2.so \
  --login --pin 1234 \
  --keypairgen --key-type RSA:2048 \
  --label "themis-signing-key"
```

3. **ThemisDB Configuration**
```cpp
HSMConfig config;
config.library_path = "/usr/lib/softhsm/libsofthsm2.so";  // or /usr/local/lib/softhsm/libsofthsm2.so
config.slot_id = 0;
config.pin = "1234";
config.key_label = "themis-signing-key";
config.signature_algorithm = "RSA-SHA256";
config.session_pool_size = 4;  // For development
```

### Common Library Paths
- **Linux (Debian/Ubuntu)**: `/usr/lib/softhsm/libsofthsm2.so` or `/usr/lib/x86_64-linux-gnu/softhsm/libsofthsm2.so`
- **macOS (Intel)**: `/usr/local/lib/softhsm/libsofthsm2.so`
- **macOS (M1/M2)**: `/opt/homebrew/lib/softhsm/libsofthsm2.so`
- **Windows**: `C:\SoftHSM2\lib\softhsm2-x64.dll`

---

## SafeNet Luna HSM

SafeNet Luna HSMs are enterprise-grade hardware security modules from Thales.

### Models Supported
- Luna Network HSM (Luna SA)
- Luna PCIe HSM
- Luna Cloud HSM Service

### Prerequisites
- Luna Client Software installed
- Network connectivity to Luna HSM (for Network HSM)
- Client registered with HSM
- Partition created and assigned

### Installation

1. **Install Luna Client**
   - Download from Thales Customer Portal
   - Follow installation guide for your OS

2. **Register Client**
```bash
# On Luna Client machine
cd /usr/safenet/lunaclient/bin
./vtl addServer -n <hsm-ip-address>
./vtl verify

# Register client with HSM (requires HSM admin)
# On HSM: client register -client <client-name> -ip <client-ip>

# Assign partition to client (requires HSM admin)  
# On HSM: client assignPartition -client <client-name> -partition <partition-name>
```

3. **Verify Connection**
```bash
/usr/safenet/lunaclient/bin/vtl verify
/usr/safenet/lunaclient/bin/lunacm
```

### ThemisDB Configuration

```cpp
HSMConfig config;
config.library_path = "/usr/safenet/lunaclient/lib/libCryptoki2_64.so";  // Linux
// config.library_path = "C:\\Program Files\\SafeNet\\LunaClient\\cryptoki.dll";  // Windows
config.slot_id = 0;  // Usually 0 for first partition
config.pin = getenv("THEMIS_HSM_PIN");  // Never hardcode!
config.key_label = "themis-prod-key";
config.signature_algorithm = "RSA-SHA256";
config.session_pool_size = 16;  // Recommended for production
config.verbose = false;  // Disable verbose logging in production
```

### Best Practices
1. **Use environment variables for PIN**: Never hardcode PINs in source code
2. **Session pooling**: Use 8-16 sessions for high-throughput applications
3. **Monitoring**: Monitor HSM health via Luna Admin tools
4. **Backup**: Use Luna Backup HSM or secure key export
5. **HA setup**: Configure multiple Luna HSMs with replication

### Common Library Paths
- **Linux**: `/usr/safenet/lunaclient/lib/libCryptoki2_64.so`
- **Windows**: `C:\Program Files\SafeNet\LunaClient\cryptoki.dll`
- **macOS**: `/usr/local/safenet/lunaclient/lib/libCryptoki2_64.dylib`

---

## Yubico YubiHSM2

YubiHSM2 is a compact, USB-attached HSM ideal for servers and edge deployments.

### Prerequisites
- YubiHSM2 device
- YubiHSM Shell and SDK installed
- USB access on server

### Installation

1. **Install YubiHSM Software**

Ubuntu/Debian:
```bash
sudo add-apt-repository ppa:yubico/stable
sudo apt-get update
sudo apt-get install yubihsm-shell libykpiv1
```

macOS:
```bash
brew install yubihsm-shell
```

2. **Connect and Test**
```bash
# Test connection
yubihsm-shell

# In yubihsm-shell:
> connect
> list-objects
> quit
```

### ThemisDB Configuration

```cpp
HSMConfig config;
config.library_path = "/usr/lib/x86_64-linux-gnu/pkcs11/yubihsm_pkcs11.so";  // Linux
// config.library_path = "/usr/local/lib/pkcs11/yubihsm_pkcs11.dylib";  // macOS
config.slot_id = 0;
config.pin = getenv("THEMIS_HSM_PIN");  // Default: 0001password (change immediately!)
config.key_label = "themis-key";
config.signature_algorithm = "RSA-SHA256";
config.session_pool_size = 4;  // YubiHSM supports fewer concurrent sessions
```

### Best Practices
1. **Change default PIN**: Default authentication key is `0001password` - change immediately
2. **Object IDs**: Use unique object IDs (0x0001-0xFFFF) for keys
3. **Backup**: Use wrap key to securely export/import keys
4. **Firmware updates**: Keep YubiHSM2 firmware up to date
5. **USB reliability**: Use server-grade USB ports or USB-over-IP for reliability

### Common Library Paths
- **Linux**: `/usr/lib/x86_64-linux-gnu/pkcs11/yubihsm_pkcs11.so`
- **macOS**: `/usr/local/lib/pkcs11/yubihsm_pkcs11.dylib`
- **Windows**: `C:\Program Files\Yubico\YubiHSM2\pkcs11\yubihsm_pkcs11.dll`

---

## AWS CloudHSM

AWS CloudHSM provides FIPS 140-2 Level 3 validated HSMs in the AWS cloud.

### Prerequisites
- AWS CloudHSM cluster created
- Client instance in same VPC
- Security group allowing HSM traffic (2223-2225)
- CloudHSM Client software installed

### Installation

1. **Create CloudHSM Cluster** (AWS Console or CLI)
```bash
aws cloudhsmv2 create-cluster \
  --hsm-type hsm1.medium \
  --subnet-ids subnet-xxxxx
```

2. **Install CloudHSM Client**

Amazon Linux 2:
```bash
wget https://s3.amazonaws.com/cloudhsmv2-software/CloudHsmClient/EL7/cloudhsm-client-latest.el7.x86_64.rpm
sudo yum install -y ./cloudhsm-client-latest.el7.x86_64.rpm
```

Ubuntu:
```bash
wget https://s3.amazonaws.com/cloudhsmv2-software/CloudHsmClient/Bionic/cloudhsm-client_latest_u18.04_amd64.deb
sudo apt install -y ./cloudhsm-client_latest_u18.04_amd64.deb
```

3. **Configure Client**
```bash
sudo /opt/cloudhsm/bin/configure -a <hsm-ip-address>
```

4. **Initialize and Activate HSM**
```bash
# Initialize HSM (first time only)
/opt/cloudhsm/bin/cloudhsm_mgmt_util /opt/cloudhsm/etc/cloudhsm_mgmt_util.cfg

# In cloudhsm_mgmt_util:
> loginHSM PRECO admin password
> changePswd PRECO admin <new-password>
> createUser CU themisdb <password>
> quit
```

### ThemisDB Configuration

```cpp
HSMConfig config;
config.library_path = "/opt/cloudhsm/lib/libcloudhsm_pkcs11.so";
config.slot_id = 0;
config.pin = getenv("THEMIS_CLOUDHSM_PIN");  // CU user password
config.token_label = "cavium";  // CloudHSM token label
config.key_label = "themis-prod-key";
config.signature_algorithm = "RSA-SHA256";
config.session_pool_size = 32;  // CloudHSM can handle many concurrent sessions
config.verbose = false;
```

### Best Practices
1. **High Availability**: Deploy HSMs in multiple AZs
2. **Session pooling**: Use 16-32 sessions for high-throughput workloads
3. **Key backup**: Use key export/import for disaster recovery
4. **Monitoring**: Integrate with CloudWatch for HSM metrics
5. **Rotation**: Implement automated key rotation policies
6. **Quorum Authentication**: Use M-of-N authentication for sensitive operations

### Common Library Paths
- **Linux**: `/opt/cloudhsm/lib/libcloudhsm_pkcs11.so`
- **CloudHSM v2**: Default library path post-client installation

---

## Common Configuration

### Environment Variables

For security, use environment variables for sensitive configuration:

```bash
export THEMIS_HSM_PIN="your-hsm-pin"
export THEMIS_HSM_LIBRARY="/path/to/pkcs11/library.so"
export THEMIS_HSM_SESSION_POOL="16"
```

### Configuration File (config.json)

```json
{
  "hsm": {
    "enabled": true,
    "library_path": "/usr/lib/softhsm/libsofthsm2.so",
    "slot_id": 0,
    "token_label": "themis-hsm",
    "key_label": "themis-signing-key",
    "signature_algorithm": "RSA-SHA256",
    "session_pool_size": 16,
    "verbose": false
  }
}
```

### Build Configuration

To build ThemisDB with real HSM support:

```bash
cmake -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DTHEMIS_ENABLE_HSM_REAL=ON

cmake --build build --target themis_server
```

### Runtime Provider Switching

ThemisDB supports runtime HSM provider switching via configuration:

```cpp
// Load config from file
auto config = HSMConfig::fromFile("hsm_config.json");

// Create provider
auto hsm = std::make_unique<HSMProvider>(config);

// Switch at runtime (reinitialize with new config)
if (environment_changed) {
    hsm->finalize();
    config = HSMConfig::fromFile("hsm_config_dr.json");
    hsm = std::make_unique<HSMProvider>(config);
    hsm->initialize();
}
```

### Failover Configuration

For production high-availability setups:

```cpp
std::vector<HSMConfig> hsm_configs = {
    {.library_path = "/path/to/primary/hsm.so", .slot_id = 0, ...},
    {.library_path = "/path/to/secondary/hsm.so", .slot_id = 0, ...},
};

// Try each HSM in order until one succeeds
std::unique_ptr<HSMProvider> hsm;
for (const auto& config : hsm_configs) {
    hsm = std::make_unique<HSMProvider>(config);
    if (hsm->initialize()) {
        THEMIS_INFO("Connected to HSM: {}", hsm->getTokenInfo());
        break;
    }
    THEMIS_WARN("Failed to connect to HSM, trying next...");
}
```

### Troubleshooting

#### Common Issues

1. **"Library could not be loaded"**
   - Check library path is correct
   - Verify PKCS#11 library is installed
   - Check file permissions
   - Try `ldd /path/to/library.so` to check dependencies

2. **"No slots found"**
   - HSM not connected/initialized
   - Check USB connection (for YubiHSM)
   - Verify network connectivity (for network HSMs)
   - Check HSM is powered on

3. **"PIN incorrect"**
   - Verify PIN is correct
   - Check if HSM is locked (too many failed attempts)
   - Ensure using correct user type (CKU_USER vs CKU_SO)

4. **"No private key found"**
   - Key with specified label doesn't exist
   - Generate key pair first
   - Check key label spelling

5. **Performance issues**
   - Increase session_pool_size
   - Check HSM network latency (for network HSMs)
   - Consider HSM load balancing

#### Debug Logging

Enable verbose logging:

```cpp
config.verbose = true;
```

Check system logs:
```bash
# Linux
sudo journalctl -u themis_server -f

# Check PKCS#11 library logs
export PKCS11SPY=/path/to/pkcs11/library.so
export PKCS11SPY_OUTPUT=/tmp/pkcs11.log
```

### Security Considerations

1. **PIN Management**
   - Never hardcode PINs in source code
   - Use environment variables or secure key stores
   - Rotate PINs regularly
   - Use different PINs for dev/staging/prod

2. **Key Lifecycle**
   - Generate keys inside HSM (never import production keys)
   - Use non-extractable keys for production
   - Implement key rotation policies
   - Maintain key backup strategy

3. **Audit Logging**
   - Enable HSM audit logs
   - Monitor all cryptographic operations
   - Alert on suspicious activity
   - Retain logs per compliance requirements

4. **Network Security** (for network HSMs)
   - Use dedicated HSM network/VLAN
   - Enable mTLS for HSM connections
   - Implement firewall rules
   - Monitor network traffic

5. **Physical Security**
   - Secure HSM hardware in controlled environment
   - Use tamper-evident seals
   - Log physical access
   - Follow HSM vendor security guidelines

---

## Vendor Support Matrix

| Feature | SoftHSM2 | SafeNet Luna | YubiHSM2 | AWS CloudHSM |
|---------|----------|--------------|-----------|--------------|
| FIPS 140-2 Level | N/A | Level 3 | Level 3 | Level 3 |
| Key Generation | ✅ | ✅ | ✅ | ✅ |
| RSA Signing | ✅ | ✅ | ✅ | ✅ |
| ECDSA Signing | ✅ | ✅ | ✅ | ✅ |
| AES Encryption | ✅ | ✅ | ✅ | ✅ |
| Session Pooling | ✅ (unlimited) | ✅ (up to 2048) | ✅ (up to 16) | ✅ (up to 2048) |
| HA/Clustering | N/A | ✅ | N/A | ✅ |
| Price Range | Free | $$$$$ | $$ | $$$ |
| Best For | Dev/Test | Enterprise | Edge/Server | Cloud Native |

---

## Next Steps

1. Choose appropriate HSM for your deployment
2. Follow vendor-specific installation guide
3. Test with development HSM first
4. Configure production HSM with proper security
5. Implement monitoring and alerting
6. Document disaster recovery procedures
7. Train operations team on HSM management

For additional support, consult:
- Vendor documentation
- ThemisDB community forums
- Professional services team
