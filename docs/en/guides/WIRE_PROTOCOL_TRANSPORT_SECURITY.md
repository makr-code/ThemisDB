# Wire Protocol Transport Security Guide

> **Version:** 1.5.0  
> **Last Updated:** April 2026  
> **Status:** ✅ Production Ready

## Overview

ThemisDB's Wire Protocol provides a high-performance binary protocol for client-server communication. This guide covers transport security for the Wire Protocol, including TLS/mTLS configuration, best practices, and production deployment guidelines.

## Table of Contents

- [Security Features](#security-features)
- [Quick Start](#quick-start)
- [Configuration](#configuration)
- [Production Deployment](#production-deployment)
- [Client Libraries](#client-libraries)
- [Connection Pooling](#connection-pooling)
- [Troubleshooting](#troubleshooting)
- [Best Practices](#best-practices)

---

## Security Features

### Transport Layer Security (TLS)

The Wire Protocol supports industry-standard TLS for encrypted communication:

- **TLS 1.2 & 1.3** - Modern, secure protocol versions
- **Strong Cipher Suites** - ECDHE-RSA-AES256-GCM-SHA384, ChaCha20-Poly1305
- **Certificate Verification** - Full chain verification with configurable CA trust
- **SNI Support** - Server Name Indication for multi-host deployments

### Mutual TLS (mTLS)

For enhanced security, mTLS provides bidirectional authentication:

- **Client Certificate Authentication** - Clients must present valid certificates
- **Certificate-Based Identity** - No passwords transmitted over the wire
- **Zero-Trust Architecture** - Every connection is authenticated and encrypted

### Production Safety

Built-in safeguards ensure production deployments are secure:

- **Production Mode Enforcement** - Fail-closed: reject connections when TLS is disabled in production
- **Configuration Validation** - Pre-flight checks catch misconfigurations
- **Security Logging** - TLS handshake details logged for audit trails
- **Deprecation Warnings** - Clear warnings for insecure configurations

---

## Quick Start

### Basic TLS Connection (Go Client)

```go
package main

import (
    "log"
    themisdb "github.com/makr-code/ThemisDB/clients/go"
)

func main() {
    // Create TLS configuration
    tlsConfig := themisdb.NewTLSConfig()
    tlsConfig.CACertPath = "/path/to/ca.crt"
    tlsConfig.ServerName = "themisdb.example.com"
    
    // Create client with TLS
    client, err := themisdb.NewWireClientWithTLS(
        "themisdb.example.com",
        18765,
        "username",
        "password",
        tlsConfig,
    )
    if err != nil {
        log.Fatalf("Failed to create client: %v", err)
    }
    
    // Connect and authenticate
    if err := client.Connect(); err != nil {
        log.Fatalf("Connection failed: %v", err)
    }
    defer client.Disconnect()
    
    log.Println("Connected securely with TLS!")
}
```

### Mutual TLS (mTLS) Connection

```go
// Create production-ready mTLS configuration
tlsConfig := themisdb.NewProductionTLSConfig("/path/to/ca.crt")
tlsConfig.ClientCertPath = "/path/to/client.crt"
tlsConfig.ClientKeyPath = "/path/to/client.key"

client, err := themisdb.NewWireClientWithTLS(
    "themisdb.example.com",
    18765,
    "username",
    "password",
    tlsConfig,
)
```

### Environment-Based Configuration

```bash
# Set environment variables
export THEMIS_WIRE_HOST=themisdb.example.com
export THEMIS_WIRE_PORT=18765
export THEMIS_WIRE_USERNAME=myuser
export THEMIS_WIRE_PASSWORD=mypassword
export THEMIS_WIRE_TLS_ENABLED=true
export THEMIS_WIRE_TLS_CA_CERT=/etc/themisdb/certs/ca.crt
export THEMIS_WIRE_TLS_CLIENT_CERT=/etc/themisdb/certs/client.crt
export THEMIS_WIRE_TLS_CLIENT_KEY=/etc/themisdb/certs/client-key.pem
export THEMIS_WIRE_PRODUCTION_MODE=true
```

```go
// Create client from environment
client, err := themisdb.NewWireClientFromEnv()
if err != nil {
    log.Fatalf("Failed to create client: %v", err)
}
```

---

## Configuration

### Server Configuration (C++)

The Wire Protocol server is configured via the `WireProtocolServer::Config` structure:

```cpp
#include "network/wire_protocol_server.h"

themis::network::WireProtocolServer::Config config;

// Basic settings
config.host = "0.0.0.0";
config.port = 18765;
config.num_io_threads = 4;
config.num_worker_threads = 8;

// Security settings
config.max_connections = 1000;
config.max_connections_per_ip = 10;
config.connection_timeout_sec = 300;
config.require_auth = true;

// TLS Configuration
config.enable_tls = true;
config.tls_cert_path = "/etc/themisdb/certs/server.crt";
config.tls_key_path = "/etc/themisdb/certs/server-key.pem";
config.tls_ca_cert_path = "/etc/themisdb/certs/ca.crt";

// mTLS (optional)
config.tls_require_client_cert = true;

auto server = std::make_unique<themis::network::WireProtocolServer>(
    config, storage, secondary_index, graph_index, 
    vector_index, tx_manager
);
```

### Client Configuration Options

#### TLSConfig Fields

| Field | Type | Description | Default |
|-------|------|-------------|---------|
| `Enabled` | `bool` | Enable TLS encryption | `true` |
| `CACertPath` | `string` | CA certificate for server verification | `""` |
| `ClientCertPath` | `string` | Client certificate for mTLS | `""` |
| `ClientKeyPath` | `string` | Client private key for mTLS | `""` |
| `MinVersion` | `uint16` | Minimum TLS version | `TLS 1.2` |
| `InsecureSkipVerify` | `bool` | Skip certificate verification (INSECURE) | `false` |
| `ServerName` | `string` | Server name for SNI | `host` |
| `ProductionMode` | `bool` | Enforce production security | `false` |

#### Environment Variables

| Variable | Description | Default |
|----------|-------------|---------|
| `THEMIS_WIRE_HOST` | Server hostname | `localhost` |
| `THEMIS_WIRE_PORT` | Server port | `18765` |
| `THEMIS_WIRE_USERNAME` | Authentication username | `""` |
| `THEMIS_WIRE_PASSWORD` | Authentication password | `""` |
| `THEMIS_WIRE_TLS_ENABLED` | Enable TLS | `false` |
| `THEMIS_WIRE_TLS_CA_CERT` | CA certificate path | `""` |
| `THEMIS_WIRE_TLS_CLIENT_CERT` | Client certificate (mTLS) | `""` |
| `THEMIS_WIRE_TLS_CLIENT_KEY` | Client key (mTLS) | `""` |
| `THEMIS_WIRE_TLS_INSECURE_SKIP_VERIFY` | Skip verification (INSECURE) | `false` |
| `THEMIS_WIRE_TLS_SERVER_NAME` | Server name for SNI | `host` |
| `THEMIS_WIRE_PRODUCTION_MODE` | Enforce production mode | `false` |

---

## Production Deployment

### Production Mode Enforcement

When `ProductionMode` is enabled, the client enforces strict security requirements:

```go
tlsConfig := &themisdb.TLSConfig{
    Enabled:        false,  // ❌ This will cause an error
    ProductionMode: true,
}

client, err := themisdb.NewWireClientWithTLS(host, port, user, pass, tlsConfig)
// ERROR: "TLS is REQUIRED in production mode but is disabled"
```

**Production mode requirements:**
- ✅ TLS must be enabled
- ✅ TLS 1.3 is strongly recommended (TLS 1.2 allowed with warning)
- ❌ `InsecureSkipVerify` is forbidden
- ✅ Certificate verification is mandatory

### Certificate Management

#### Development Environment

For development, use self-signed certificates:

```bash
# Generate self-signed certificates (development only)
./scripts/generate_test_certs.sh /path/to/certs
```

#### Production Environment

For production, use certificates from a trusted CA:

**Option 1: Let's Encrypt (Free, Automated)**
```bash
certbot certonly --standalone -d themisdb.example.com
```

**Option 2: Internal PKI**
- Use your organization's CA (e.g., Active Directory Certificate Services)
- Integrate with HashiCorp Vault for automated certificate rotation

**Option 3: Commercial CA**
- DigiCert, GlobalSign, etc.

#### Certificate Rotation

Implement automated certificate rotation to prevent expiration. Note that accessing the underlying TLS connection state requires extending the client API:

```go
// Conceptual example - would require adding a public method to WireClient
// to expose the TLS connection state when TLS is enabled.

// For now, monitor certificate expiration by:
// 1. Track certificate expiration dates in your deployment system
// 2. Set up alerts 30 days before expiry
// 3. Use automated certificate renewal (Let's Encrypt, Vault)
// 4. Reload client connections after certificate renewal
```

**Recommended approach for certificate monitoring:**
- Use external monitoring tools (cert-manager, Vault, monitoring systems)
- Set up alerts 30 days before certificate expiry
- Implement automated certificate renewal with Let's Encrypt or HashiCorp Vault
- Plan certificate rotation windows during maintenance periods

### Network Architecture

#### Recommended Ports

| Port | Protocol | Purpose | TLS |
|------|----------|---------|-----|
| `18765` | Wire Protocol | Binary API (Production) | ✅ Required |
| `8080` | HTTP/REST | REST API | ⚠️ Optional |
| `50051` | gRPC | RPC API | ✅ Recommended |
| `9100` | HTTP | Metrics/Prometheus | ⚠️ Internal only |

#### Firewall Rules

**Production deployment:**
```bash
# Allow Wire Protocol with TLS
iptables -A INPUT -p tcp --dport 18765 -j ACCEPT

# Block insecure ports from external access
iptables -A INPUT -p tcp --dport 8080 -s 10.0.0.0/8 -j ACCEPT
iptables -A INPUT -p tcp --dport 8080 -j DROP
```

---

## Connection Pooling

The Wire Protocol supports connection pooling on both client and server sides for optimal performance.

### Client-Side Pooling (C++)

```cpp
#include "network/wire_protocol_connection_pool.h"

themis::network::WireProtocolConnectionPool::Config poolConfig;
poolConfig.min_connections_per_target = 2;
poolConfig.max_connections_per_target = 20;
poolConfig.idle_timeout = std::chrono::seconds(60);
poolConfig.enable_ssl = true;
poolConfig.enable_mtls = true;
poolConfig.ssl_cert_path = "/etc/themisdb/certs/client.crt";
poolConfig.ssl_key_path = "/etc/themisdb/certs/client-key.pem";
poolConfig.ssl_ca_cert_path = "/etc/themisdb/certs/ca.crt";

auto pool = std::make_unique<themis::network::WireProtocolConnectionPool>(poolConfig);
```

### Server-Side Limits

Configure connection limits on the server to prevent resource exhaustion:

```cpp
config.max_connections = 1000;               // Global limit
config.max_connections_per_ip = 10;          // Per-IP limit
config.max_requests_per_second = 1000;       // Rate limiting
config.connection_timeout_sec = 300;         // Idle timeout
```

---

## Troubleshooting

### Common Errors

#### 1. "TLS is REQUIRED in production mode but is disabled"

**Cause:** Production mode is enabled but TLS is disabled.

**Solution:**
```go
tlsConfig.Enabled = true
// OR
tlsConfig.ProductionMode = false  // Only for development
```

#### 2. "Failed to establish TLS connection: x509: certificate signed by unknown authority"

**Cause:** Server certificate is not trusted (no CA certificate provided).

**Solution:**
```go
tlsConfig.CACertPath = "/path/to/ca.crt"
// OR (for testing only)
tlsConfig.InsecureSkipVerify = true
```

#### 3. "TLS handshake failed: remote error: tls: bad certificate"

**Cause:** Server requires mTLS but client did not provide certificate.

**Solution:**
```go
tlsConfig.ClientCertPath = "/path/to/client.crt"
tlsConfig.ClientKeyPath = "/path/to/client-key.pem"
```

#### 4. "CA certificate file not found"

**Cause:** Certificate path is incorrect or file doesn't exist.

**Solution:**
```bash
# Verify file exists
ls -l /path/to/ca.crt

# Check permissions
chmod 644 /path/to/ca.crt
```

### Debug Logging

Enable verbose logging to diagnose TLS issues:

```go
import "log"

log.SetFlags(log.LstdFlags | log.Lshortfile)

// The client will now log:
// - "INFO: Establishing TLS connection..."
// - "INFO: TLS connection established (Version: TLS 1.3, ...)"
// - "WARNING: TLS is disabled. Connection is NOT encrypted..."
```

### Network Analysis

Use `openssl s_client` to test TLS connectivity:

```bash
# Test TLS connection
openssl s_client -connect themisdb.example.com:18765 \
    -CAfile /path/to/ca.crt

# Test mTLS connection
openssl s_client -connect themisdb.example.com:18765 \
    -CAfile /path/to/ca.crt \
    -cert /path/to/client.crt \
    -key /path/to/client-key.pem
```

---

## Best Practices

### Security

1. **Always use TLS in production** - Never deploy without encryption
2. **Use TLS 1.3 when possible** - Better security and performance
3. **Implement mTLS for service-to-service** - Eliminate password-based auth
4. **Rotate certificates regularly** - Automate with Let's Encrypt or Vault
5. **Monitor certificate expiration** - Set up alerts 30 days before expiry
6. **Use strong cipher suites** - Disable weak ciphers (3DES, RC4, MD5)
7. **Enable production mode** - Fail-closed approach prevents misconfigurations

### Performance

1. **Enable connection pooling** - Reduce handshake overhead
2. **Use keepalive** - Prevent connection timeouts
3. **Tune connection limits** - Balance between resource usage and throughput
4. **Monitor connection metrics** - Track pool utilization and timeouts
5. **Use persistent connections** - Avoid reconnecting for every request

### Operations

1. **Document your PKI** - Maintain certificate inventory and procedures
2. **Implement monitoring** - Track TLS handshake failures and certificate expiry
3. **Test certificate rotation** - Verify zero-downtime certificate updates
4. **Use secrets management** - Store private keys securely (Vault, AWS Secrets Manager)
5. **Implement gradual rollout** - Test TLS changes in staging before production

---

## References

- [Wire Protocol Server Implementation](../../../include/network/wire_protocol_server.h)
- [Wire Protocol Connection Pool](../../../include/network/wire_protocol_connection_pool.h)
- [TLS Setup Guide](../../de/guides/guides_tls_setup.md)
- [Port Reference](../../de/deployment/PORT_REFERENCE.md)
- [mTLS Configuration Example](../../../config/sharding/shard-router-mtls-example.yaml)

---

## Support

For security issues or questions:
- **Security Issues**: Report to service@themisdb.org (do not open public issues)
- **General Help**: [GitHub Discussions](https://github.com/makr-code/ThemisDB/discussions)
- **Documentation**: [https://makr-code.github.io/ThemisDB/](https://makr-code.github.io/ThemisDB/)
