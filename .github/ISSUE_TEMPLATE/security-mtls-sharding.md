---
name: 🔐 Security: mTLS for Shard Communication
about: Implement mutual TLS for shard-to-shard communication
title: "[SECURITY] Implement mTLS for Shard RPC Communication"
labels: priority:P1, type:security, area:sharding, area:networking, effort:medium, v1.4.0
assignees: ''
---

## ⚠️ Important for v1.4.0 Production

**Current Status:** TODO marker in code  
**Priority:** P1 (High)  
**Effort:** 1-2 weeks  
**Target Version:** v1.4.0  
**Related Audit:** `NAMESPACE_IMPLEMENTATION_AUDIT_REPORT.md` Section 3.4 (Sharding Module)

---

## 📋 Problem Description

Shard-to-shard RPC communication currently lacks mTLS support:

```cpp
// src/sharding/shard_rpc_client.cpp
// TODO: Add mTLS support for production
```

**Security Risk:** **MEDIUM-HIGH**  
- Shard communication is not encrypted
- No mutual authentication between shards
- Vulnerable to man-in-the-middle attacks
- Cannot verify shard identity

---

## 🎯 Requirements

### Must Have (P1) - v1.4.0

- [ ] **Mutual TLS (mTLS) Support**
  - Enable TLS for gRPC shard communication
  - Require client certificates
  - Verify server certificates
  - Bidirectional authentication
  
- [ ] **Certificate Management**
  - Per-shard certificates
  - Certificate rotation support
  - Certificate expiration handling
  - Root CA configuration
  
- [ ] **Configuration**
  - Enable/disable mTLS per environment
  - Certificate paths configuration
  - TLS version selection (TLS 1.2+)
  - Cipher suite configuration
  
- [ ] **Error Handling**
  - Certificate validation failures
  - TLS handshake errors
  - Certificate expiration warnings
  - Graceful fallback (optional for testing)

### Should Have (P2)

- [ ] **Certificate Auto-Renewal**
  - Integrate with cert-manager (Kubernetes)
  - ACME protocol support
  - Automatic certificate rotation
  
- [ ] **Certificate Pinning**
  - Pin specific certificates for high-security deployments
  - Prevent rogue CA attacks
  
- [ ] **Monitoring**
  - TLS handshake success/failure metrics
  - Certificate expiration alerts
  - Connection encryption status

### Nice to Have (P3)

- [ ] **HSM Integration**
  - Store shard private keys in HSM
  - HSM-based TLS operations
  
- [ ] **Zero-Trust Networking**
  - Service mesh integration (Istio, Linkerd)
  - Automatic mTLS via sidecar proxy

---

## 🔧 Implementation Details

### Files to Modify

- `src/sharding/shard_rpc_client.cpp` - Enable mTLS for client
- `src/sharding/shard_rpc_server.cpp` - Enable mTLS for server
- `include/sharding/shard_rpc_client.h` - Add TLS configuration
- `include/sharding/shard_rpc_server.h` - Add TLS configuration

### gRPC TLS Configuration

```cpp
#include <grpcpp/security/credentials.h>
#include <grpcpp/security/server_credentials.h>

// Server-side mTLS
std::shared_ptr<grpc::ServerCredentials> createServerCredentials() {
    // Read certificates
    std::string root_ca = readFile(config_.tls.root_ca_path);
    std::string server_cert = readFile(config_.tls.server_cert_path);
    std::string server_key = readFile(config_.tls.server_key_path);
    
    grpc::SslServerCredentialsOptions::PemKeyCertPair key_cert_pair;
    key_cert_pair.private_key = server_key;
    key_cert_pair.cert_chain = server_cert;
    
    grpc::SslServerCredentialsOptions ssl_opts;
    ssl_opts.pem_root_certs = root_ca;
    ssl_opts.pem_key_cert_pairs.push_back(key_cert_pair);
    
    // Require client certificates (mutual TLS)
    ssl_opts.client_certificate_request = 
        GRPC_SSL_REQUEST_AND_REQUIRE_CLIENT_CERTIFICATE_AND_VERIFY;
    
    return grpc::SslServerCredentials(ssl_opts);
}

// Client-side mTLS
std::shared_ptr<grpc::ChannelCredentials> createChannelCredentials() {
    std::string root_ca = readFile(config_.tls.root_ca_path);
    std::string client_cert = readFile(config_.tls.client_cert_path);
    std::string client_key = readFile(config_.tls.client_key_path);
    
    grpc::SslCredentialsOptions ssl_opts;
    ssl_opts.pem_root_certs = root_ca;
    ssl_opts.pem_cert_chain = client_cert;
    ssl_opts.pem_private_key = client_key;
    
    return grpc::SslCredentials(ssl_opts);
}
```

### Configuration Example

```yaml
sharding:
  rpc:
    mtls:
      enabled: true
      
      # Server configuration
      server:
        cert_path: "/etc/themisdb/certs/shard-server.pem"
        key_path: "/etc/themisdb/certs/shard-server-key.pem"
        root_ca_path: "/etc/themisdb/certs/ca.pem"
        
      # Client configuration
      client:
        cert_path: "/etc/themisdb/certs/shard-client.pem"
        key_path: "/etc/themisdb/certs/shard-client-key.pem"
        root_ca_path: "/etc/themisdb/certs/ca.pem"
        verify_server: true
        
      # TLS options
      tls_version: "TLS1.3"
      cipher_suites:
        - "TLS_AES_256_GCM_SHA384"
        - "TLS_CHACHA20_POLY1305_SHA256"
        
      # Certificate management
      cert_expiration_warning_days: 30
      auto_reload_certs: true
      reload_interval_seconds: 3600
```

---

## ✅ Acceptance Criteria

- [ ] mTLS enabled for all shard-to-shard RPC calls
- [ ] Both client and server certificates verified
- [ ] **Zero TODO comments** about missing mTLS
- [ ] Unencrypted connections rejected in production mode
- [ ] Certificate expiration monitored
- [ ] All tests pass with mTLS enabled
- [ ] Documentation includes certificate setup guide

---

## 🧪 Testing Requirements

### Unit Tests

- [ ] Test TLS handshake success
- [ ] Test rejection without client certificate
- [ ] Test rejection with invalid certificate
- [ ] Test rejection with expired certificate
- [ ] Test certificate reload
- [ ] Test cipher suite negotiation

### Integration Tests

- [ ] Test shard-to-shard RPC with mTLS
- [ ] Test multi-shard queries with mTLS
- [ ] Test failover with mTLS
- [ ] Test performance impact (< 5% overhead)
- [ ] Test with different TLS versions

### Manual Testing

- [ ] Generate test certificates (self-signed)
- [ ] Test with Let's Encrypt certificates
- [ ] Test certificate rotation
- [ ] Verify with Wireshark (encrypted traffic)
- [ ] Test in Kubernetes with cert-manager

---

## 📚 References

- **gRPC Authentication:** https://grpc.io/docs/guides/auth/
- **gRPC TLS:** https://github.com/grpc/grpc/blob/master/doc/ssl.md
- **TLS 1.3:** https://tools.ietf.org/html/rfc8446
- **Current Implementation:** `src/sharding/shard_rpc_client.cpp` (line with TODO)

---

## 📊 Success Metrics

- ✅ 100% of shard RPC calls use mTLS in production
- ✅ Certificate validation success rate > 99.9%
- ✅ TLS overhead < 5% (latency increase)
- ✅ Zero unencrypted shard connections
- ✅ Ready for v1.4.0 release

---

## 🚨 Important Notes

- **Performance:** mTLS adds ~5-10ms latency to connection establishment
- **Certificate Distribution:** Plan for automated certificate distribution in clusters
- **Key Rotation:** Document procedure for rotating certificates without downtime
- **Development Mode:** Allow disabling mTLS for local development

---

## 📅 Implementation Plan

### Week 1: Core mTLS
- [ ] Day 1-2: Implement server-side mTLS
- [ ] Day 3-4: Implement client-side mTLS
- [ ] Day 5: Unit tests

### Week 2: Advanced Features
- [ ] Day 1: Certificate reload
- [ ] Day 2: Configuration system
- [ ] Day 3-4: Integration tests
- [ ] Day 5: Documentation and setup guide

---

## 🔗 Related Issues

- #XXX - HSM Provider Implementation (for key storage)
- #XXX - Certificate management system

---

**Created:** Based on Namespace Implementation Audit (2026-01-20)  
**Audit Section:** 3.4 Sharding Module - mTLS TODO
