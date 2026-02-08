# WAL gRPC mTLS Configuration Guide

## Overview

The WAL gRPC service supports mutual TLS (mTLS) for secure production deployments. This guide explains how to configure and deploy the service with proper encryption and authentication.

## Security Modes

### 1. Development Mode (Insecure)
**Use only in development/testing environments**

```bash
# No configuration needed - this is the default
# The service will use InsecureServerCredentials
```

⚠️ **Warning**: Development mode transmits data in plaintext and should never be used in production.

### 2. Server-side TLS (Optional client authentication)
Server presents a certificate to clients. Clients can optionally present certificates that will be verified if provided.

```bash
export THEMIS_WAL_GRPC_ENABLE_MTLS=true
export THEMIS_WAL_GRPC_CERT_PATH=/etc/themis/certs/server.crt
export THEMIS_WAL_GRPC_KEY_PATH=/etc/themis/certs/server.key
export THEMIS_WAL_GRPC_CA_CERT_PATH=/etc/themis/certs/ca.crt  # Optional but recommended
export THEMIS_WAL_GRPC_REQUIRE_CLIENT_CERT=false
```

### 3. Mutual TLS (Two-way authentication) - **Recommended for Production**
Both server and clients authenticate each other using certificates. CA certificate is required for client verification.

```bash
export THEMIS_WAL_GRPC_ENABLE_MTLS=true
export THEMIS_WAL_GRPC_CERT_PATH=/etc/themis/certs/server.crt
export THEMIS_WAL_GRPC_KEY_PATH=/etc/themis/certs/server.key
export THEMIS_WAL_GRPC_CA_CERT_PATH=/etc/themis/certs/ca.crt  # Required for mTLS
export THEMIS_WAL_GRPC_REQUIRE_CLIENT_CERT=true
```

## Environment Variables

| Variable | Required | Default | Description |
|----------|----------|---------|-------------|
| `THEMIS_WAL_GRPC_ENABLE_MTLS` | No | `false` | Enable mTLS/TLS for the WAL gRPC service |
| `THEMIS_WAL_GRPC_CERT_PATH` | Yes* | - | Path to server certificate (PEM format) |
| `THEMIS_WAL_GRPC_KEY_PATH` | Yes* | - | Path to server private key (PEM format) |
| `THEMIS_WAL_GRPC_CA_CERT_PATH` | Yes** | - | Path to CA certificate for client verification |
| `THEMIS_WAL_GRPC_REQUIRE_CLIENT_CERT` | No | `true` | Require and verify client certificates |
| `THEMIS_WAL_GRPC_HOST` | No | `0.0.0.0` | Network interface to bind to |
| `THEMIS_WAL_GRPC_PORT` | No | `50051` | Port to listen on |

\* Required when `THEMIS_WAL_GRPC_ENABLE_MTLS=true`  
\*\* Required when `THEMIS_WAL_GRPC_REQUIRE_CLIENT_CERT=true` (mutual TLS mode); optional when `REQUIRE_CLIENT_CERT=false` (server-side TLS mode)

## Certificate Generation

### Using OpenSSL (Self-Signed for Testing)

#### 1. Generate CA Certificate
```bash
# Generate CA private key
openssl genrsa -out ca.key 4096

# Generate CA certificate (valid for 10 years)
openssl req -new -x509 -key ca.key -out ca.crt -days 3650 \
  -subj "/C=US/ST=State/L=City/O=Organization/CN=ThemisDB CA"
```

#### 2. Generate Server Certificate
```bash
# Generate server private key
openssl genrsa -out server.key 4096

# Generate certificate signing request
openssl req -new -key server.key -out server.csr \
  -subj "/C=US/ST=State/L=City/O=Organization/CN=themis-server"

# Sign server certificate with CA
openssl x509 -req -in server.csr -CA ca.crt -CAkey ca.key \
  -CAcreateserial -out server.crt -days 365
```

#### 3. Generate Client Certificate (for mTLS)
```bash
# Generate client private key
openssl genrsa -out client.key 4096

# Generate certificate signing request
openssl req -new -key client.key -out client.csr \
  -subj "/C=US/ST=State/L=City/O=Organization/CN=themis-client"

# Sign client certificate with CA
openssl x509 -req -in client.csr -CA ca.crt -CAkey ca.key \
  -CAcreateserial -out client.crt -days 365
```

### Using cert-manager (Kubernetes)

```yaml
apiVersion: cert-manager.io/v1
kind: Certificate
metadata:
  name: themis-wal-grpc-server
  namespace: themis
spec:
  secretName: themis-wal-grpc-tls
  duration: 2160h # 90 days
  renewBefore: 360h # 15 days
  subject:
    organizations:
      - themis
  commonName: themis-wal-grpc
  isCA: false
  privateKey:
    algorithm: RSA
    encoding: PKCS1
    size: 4096
  usages:
    - server auth
    - client auth
  dnsNames:
    - themis-wal-grpc.themis.svc.cluster.local
  issuerRef:
    name: themis-ca-issuer
    kind: ClusterIssuer
```

## File Permissions

Secure your private keys with restricted permissions:

```bash
# Server private key (read-only by owner)
chmod 600 /etc/themis/certs/server.key
chown themis:themis /etc/themis/certs/server.key

# Certificates can be world-readable
chmod 644 /etc/themis/certs/server.crt
chmod 644 /etc/themis/certs/ca.crt
```

## Docker Configuration

### Using Docker Secrets
```yaml
version: '3.8'
services:
  themis:
    image: themis:latest
    environment:
      THEMIS_WAL_GRPC_ENABLE_MTLS: "true"
      THEMIS_WAL_GRPC_CERT_PATH: /run/secrets/server_cert
      THEMIS_WAL_GRPC_KEY_PATH: /run/secrets/server_key
      THEMIS_WAL_GRPC_CA_CERT_PATH: /run/secrets/ca_cert
      THEMIS_WAL_GRPC_REQUIRE_CLIENT_CERT: "true"
    secrets:
      - server_cert
      - server_key
      - ca_cert

secrets:
  server_cert:
    file: ./certs/server.crt
  server_key:
    file: ./certs/server.key
  ca_cert:
    file: ./certs/ca.crt
```

### Using Volume Mounts
```bash
docker run -d \
  -v /etc/themis/certs:/etc/themis/certs:ro \
  -e THEMIS_WAL_GRPC_ENABLE_MTLS=true \
  -e THEMIS_WAL_GRPC_CERT_PATH=/etc/themis/certs/server.crt \
  -e THEMIS_WAL_GRPC_KEY_PATH=/etc/themis/certs/server.key \
  -e THEMIS_WAL_GRPC_CA_CERT_PATH=/etc/themis/certs/ca.crt \
  -e THEMIS_WAL_GRPC_REQUIRE_CLIENT_CERT=true \
  -p 50051:50051 \
  themis:latest
```

## Kubernetes Configuration

### Using ConfigMap and Secret
```yaml
apiVersion: v1
kind: Secret
metadata:
  name: themis-wal-grpc-tls
  namespace: themis
type: Opaque
data:
  server.key: <base64-encoded-key>
  server.crt: <base64-encoded-cert>
  ca.crt: <base64-encoded-ca>
---
apiVersion: v1
kind: ConfigMap
metadata:
  name: themis-wal-grpc-config
  namespace: themis
data:
  THEMIS_WAL_GRPC_ENABLE_MTLS: "true"
  THEMIS_WAL_GRPC_CERT_PATH: "/etc/themis/certs/server.crt"
  THEMIS_WAL_GRPC_KEY_PATH: "/etc/themis/certs/server.key"
  THEMIS_WAL_GRPC_CA_CERT_PATH: "/etc/themis/certs/ca.crt"
  THEMIS_WAL_GRPC_REQUIRE_CLIENT_CERT: "true"
---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: themis
  namespace: themis
spec:
  replicas: 1
  selector:
    matchLabels:
      app: themis
  template:
    metadata:
      labels:
        app: themis
    spec:
      containers:
      - name: themis
        image: themis:latest
        envFrom:
        - configMapRef:
            name: themis-wal-grpc-config
        volumeMounts:
        - name: tls-certs
          mountPath: /etc/themis/certs
          readOnly: true
      volumes:
      - name: tls-certs
        secret:
          secretName: themis-wal-grpc-tls
          defaultMode: 0400
```

## Verification

### Check Server Logs
Look for these log messages on startup:

```
[INFO] WAL gRPC: Loaded CA certificate from: /etc/themis/certs/ca.crt
[INFO] WAL gRPC: Loaded server certificate from: /etc/themis/certs/server.crt
[INFO] WAL gRPC: Client certificate verification enabled (mutual TLS)
[INFO] WAL gRPC: mTLS/TLS enabled for production deployment
[INFO] WAL gRPC Apply service listening on 0.0.0.0:50051 (mTLS: enabled)
```

### Test with grpcurl
```bash
# Test mTLS connection
grpcurl -cacert ca.crt \
  -cert client.crt \
  -key client.key \
  -d '{"entries":[]}' \
  localhost:50051 \
  themis.sharding.ShardService/ApplyWalBatch
```

## Troubleshooting

### TLS Configuration Error - Server Won't Start
```
[ERROR] WAL gRPC: Failed to configure mTLS: ... Server will NOT start to avoid insecure fallback in production.
[ERROR] WAL gRPC: Fix the TLS configuration or set THEMIS_WAL_GRPC_ENABLE_MTLS=false for development.
[ERROR] WAL gRPC Apply service NOT started due to TLS configuration errors
```

**Cause**: When `THEMIS_WAL_GRPC_ENABLE_MTLS=true`, the server will refuse to start if TLS configuration is invalid. This prevents accidental downgrade to insecure mode in production.

**Solution**: 
- Verify all required environment variables are set correctly
- Check that certificate files exist and are readable by the ThemisDB process
- For mutual TLS (`REQUIRE_CLIENT_CERT=true`), ensure `CA_CERT_PATH` is configured
- To run in development mode without TLS, set `THEMIS_WAL_GRPC_ENABLE_MTLS=false`

### Certificate Not Found
```
[ERROR] WAL gRPC: Failed to configure mTLS: Failed to read file: ...
```

**Solution**: Verify file paths and permissions. Ensure the files exist and are readable by the ThemisDB process.

### Missing CA Certificate for Mutual TLS
```
[ERROR] WAL gRPC: Failed to configure mTLS: THEMIS_WAL_GRPC_CA_CERT_PATH is required when THEMIS_WAL_GRPC_REQUIRE_CLIENT_CERT is true
```

**Solution**: When `THEMIS_WAL_GRPC_REQUIRE_CLIENT_CERT=true`, the `THEMIS_WAL_GRPC_CA_CERT_PATH` must be set to verify client certificates.

### Certificate Validation Failed
```
[ERROR] Failed to start WAL gRPC Apply service
```

**Solution**: 
- Verify certificate and key match: `openssl x509 -noout -modulus -in server.crt | openssl md5`
- Verify key modulus: `openssl rsa -noout -modulus -in server.key | openssl md5`
- The MD5 hashes should match

### Client Connection Refused
**Solution**: 
- Ensure client is using the correct CA certificate
- Verify client certificate is signed by the same CA
- Check that certificate hasn't expired: `openssl x509 -noout -dates -in server.crt`

## Security Best Practices

1. **Use Strong Key Sizes**: Minimum 2048-bit RSA, recommend 4096-bit
2. **Regular Rotation**: Rotate certificates every 90 days
3. **Secure Storage**: Store private keys in hardware security modules (HSM) when possible
4. **Monitor Expiration**: Set up alerts for certificate expiration (30 days before)
5. **Principle of Least Privilege**: Restrict file access to private keys
6. **Use Production CAs**: Avoid self-signed certificates in production
7. **Enable mTLS**: Always require client certificates in production
8. **Audit Logs**: Monitor certificate validation failures
9. **Fail-Fast Configuration**: The server refuses to start when TLS is misconfigured, preventing accidental insecure fallback
10. **Required CA Certificate**: Always configure CA certificate when using mutual TLS to avoid trusting public CAs

## References

- [gRPC Authentication Guide](https://grpc.io/docs/guides/auth/)
- [OpenSSL Documentation](https://www.openssl.org/docs/)
- [cert-manager Documentation](https://cert-manager.io/docs/)
- ThemisDB Security Architecture: See `docs/SECURITY.md`
