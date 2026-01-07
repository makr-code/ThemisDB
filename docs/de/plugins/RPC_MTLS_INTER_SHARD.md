# RPC mTLS für Inter-Shard Kommunikation

**Version:** 1.0.0  
**Release:** v1.3.0  
**Datum:** 17. Dezember 2025  
**Status:** Design & Investigation  
**Kategorie:** RPC, Sharding, Security

---

## Executive Summary

Dieses Dokument untersucht die Verwendung des RPC Frameworks für sichere Inter-Shard Kommunikation mit mutual TLS (mTLS). ThemisDB besitzt bereits eine mTLS-Implementierung für Shard-zu-Shard Kommunikation (siehe `mtls_client.h` und `pki_shard_certificate.h`). Diese Untersuchung analysiert, wie das neue RPC Framework diese bestehende mTLS-Infrastruktur nutzen und erweitern kann.

**Hauptziele:**
- 🔒 **Sichere Kommunikation** - mTLS für alle Shard-zu-Shard RPC-Aufrufe
- 🎯 **Shard-Authentifizierung** - Zertifikat-basierte Identifikation von Shards
- 🔑 **PKI-Integration** - Nutzung der bestehenden PKI-Infrastruktur
- ⚡ **Performance** - Effiziente binäre Protokolle (gRPC) statt HTTP/JSON
- 🔄 **Migration** - Schrittweise Migration von HTTP/REST zu RPC

---

## 1. Bestehende mTLS-Infrastruktur

### 1.1 Aktuelle Implementierung

ThemisDB verfügt bereits über:

**mTLS Client (`include/sharding/mtls_client.h`):**
- Mutual TLS für HTTP-basierte Shard-Kommunikation
- Zertifikat-Verifizierung gegen Root CA
- CRL (Certificate Revocation List) Checking
- Connection Pooling
- Retry-Logik mit exponential backoff

**PKI Shard Certificate (`include/sharding/pki_shard_certificate.h`):**
- X.509 Zertifikat-Parsing mit Custom Extensions
- Shard-spezifische Metadaten:
  - `shard_id` - Eindeutige Shard-Identifikation
  - `datacenter` / `rack` - Lokations-Informationen
  - `token_range_start` / `token_range_end` - Hash-Range Assignment
  - `capabilities` - Berechtigungen (read, write, replicate, admin)
  - `role` - Primary vs. Replica
- Subject Alternative Names (SAN) für Hostname-Verifizierung

### 1.2 Aktuelles Kommunikationsprotokoll

**HTTP/REST über mTLS:**
```cpp
// Beispiel: Aktueller Shard-zu-Shard Request
MTLSClient::Config config;
config.cert_path = "/etc/themis/certs/shard-001.crt";
config.key_path = "/etc/themis/certs/shard-001.key";
config.ca_cert_path = "/etc/themis/certs/ca.crt";

MTLSClient client(config);
auto response = client.post(
    "https://shard-002.dc1:8080",
    "/api/v1/replicate",
    json_payload
);
```

**Limitierungen:**
- ❌ HTTP/JSON Overhead (ca. 3-5x größer als binäre Protokolle)
- ❌ Hohe Latenz (HTTP-Header, JSON-Parsing)
- ❌ Keine Streaming-Unterstützung
- ❌ Ineffizient für große Datenmengen (z.B. Bulk-Replication)

---

## 2. RPC Framework für Inter-Shard Kommunikation

### 2.1 Architektur-Überblick

```
┌─────────────────────────────────────────────────────────────┐
│                   ThemisDB Cluster                           │
│                                                               │
│  ┌──────────────┐         mTLS gRPC         ┌──────────────┐│
│  │  Shard 001   │◄─────────────────────────►│  Shard 002   ││
│  │  DC1, Rack01 │    Cert: shard-001.crt    │  DC1, Rack02 ││
│  │              │    Verify: CA + CRL        │              ││
│  └──────────────┘                            └──────────────┘│
│         ▲                                            ▲        │
│         │                                            │        │
│         │          mTLS gRPC (inter-DC)              │        │
│         │                                            │        │
│         ▼                                            ▼        │
│  ┌──────────────┐                            ┌──────────────┐│
│  │  Shard 003   │                            │  Shard 004   ││
│  │  DC2, Rack01 │                            │  DC2, Rack02 ││
│  └──────────────┘                            └──────────────┘│
└─────────────────────────────────────────────────────────────┘
```

### 2.2 gRPC mit mTLS

**Vorteile von gRPC für Inter-Shard Kommunikation:**

1. **Performance:**
   - Binäres Protokoll (Protocol Buffers) - 5-10x schneller als JSON
   - HTTP/2 Multiplexing - Mehrere Streams pro Connection
   - Header Compression (HPACK)

2. **Streaming:**
   - Bidirectional Streaming - Ideal für Bulk-Replication
   - Server-side Streaming - Effizient für große Result Sets
   - Client-side Streaming - Batch-Uploads

3. **Native mTLS Support:**
   - gRPC hat eingebaute TLS/mTLS Unterstützung
   - Zertifikat-basierte Authentifizierung
   - Channel Credentials API

4. **Code-Generation:**
   - Automatische Client/Server Stub-Generierung
   - Type-Safe APIs
   - Sprachübergreifend (C++, Python, Go, Java, etc.)

---

## 3. Design: RPC mTLS für Sharding

### 3.1 gRPC Service Definition

**Protobuf Service für Inter-Shard Operations:**

```protobuf
// shard_rpc.proto

syntax = "proto3";

package themis.sharding;

// Inter-Shard Communication Service
service ShardService {
    // Replication
    rpc ReplicateData(ReplicateRequest) returns (ReplicateResponse);
    rpc ReplicateDataStream(stream ReplicateChunk) returns (ReplicateResponse);
    
    // Distributed Transactions
    rpc PrepareTransaction(PrepareRequest) returns (PrepareResponse);
    rpc CommitTransaction(CommitRequest) returns (CommitResponse);
    rpc AbortTransaction(AbortRequest) returns (AbortResponse);
    
    // Data Migration (for rebalancing)
    rpc MigrateData(MigrateRequest) returns (stream MigrateChunk);
    
    // Shard Status & Health
    rpc GetShardStatus(StatusRequest) returns (StatusResponse);
    rpc HealthCheck(HealthRequest) returns (HealthResponse);
    
    // Gossip Protocol Integration
    rpc GossipExchange(GossipMessage) returns (GossipMessage);
    
    // Raft Consensus (if using Raft)
    rpc RequestVote(VoteRequest) returns (VoteResponse);
    rpc AppendEntries(AppendEntriesRequest) returns (AppendEntriesResponse);
}

message ReplicateRequest {
    string shard_id = 1;              // Source shard ID
    repeated Entity entities = 2;     // Entities to replicate
    uint64 timestamp_ns = 3;          // Replication timestamp
    string transaction_id = 4;        // Optional: transaction ID
}

message ReplicateChunk {
    bytes data = 1;                   // Serialized entity data
    uint32 chunk_index = 2;           // Chunk sequence number
    bool is_last = 3;                 // Last chunk in stream
}

message ReplicateResponse {
    bool success = 1;
    uint64 replicated_count = 2;
    string error = 3;
}

message PrepareRequest {
    string transaction_id = 1;
    string coordinator_shard_id = 2;
    repeated string participant_shards = 3;
    bytes transaction_data = 4;
}

message PrepareResponse {
    bool vote_commit = 1;             // true = prepared to commit
    string error = 2;
}

message CommitRequest {
    string transaction_id = 1;
}

message CommitResponse {
    bool success = 1;
    string error = 2;
}

message StatusRequest {
    bool include_metrics = 1;
}

message StatusResponse {
    string shard_id = 1;
    string state = 2;                 // "active", "readonly", "draining"
    uint64 token_range_start = 3;
    uint64 token_range_end = 4;
    ShardMetrics metrics = 5;
}

message ShardMetrics {
    uint64 entity_count = 1;
    uint64 storage_size_bytes = 2;
    double cpu_usage_percent = 3;
    double memory_usage_bytes = 4;
    uint64 rpc_requests_total = 5;
    double avg_latency_ms = 6;
}

message Entity {
    string uuid = 1;
    string collection = 2;
    bytes data = 3;
    uint64 version = 4;
    uint64 timestamp_ns = 5;
}
```

### 3.2 mTLS Integration mit gRPC

**Server-Side (Shard):**

```cpp
// src/sharding/rpc_shard_server.cpp

#include <grpcpp/grpcpp.h>
#include <grpcpp/security/server_credentials.h>
#include "shard_rpc.grpc.pb.h"
#include "sharding/pki_shard_certificate.h"

namespace themis {
namespace sharding {

class ShardServiceImpl : public themis::sharding::ShardService::Service {
public:
    grpc::Status ReplicateData(
        grpc::ServerContext* context,
        const ReplicateRequest* request,
        ReplicateResponse* response
    ) override {
        // Extract client certificate info from context
        auto auth_context = context->auth_context();
        auto peer_cert = getPeerCertificateInfo(auth_context);
        
        if (!peer_cert) {
            return grpc::Status(grpc::StatusCode::UNAUTHENTICATED, 
                "Client certificate required");
        }
        
        // Verify shard has replication capability
        if (!peer_cert->hasCapability("replicate")) {
            return grpc::Status(grpc::StatusCode::PERMISSION_DENIED,
                "Shard does not have replication capability");
        }
        
        // Verify token range (optional: ensure shard is authorized for this data)
        // ...
        
        // Perform replication
        try {
            // TODO: Implement actual replication logic
            response->set_success(true);
            response->set_replicated_count(request->entities_size());
            return grpc::Status::OK;
        } catch (const std::exception& e) {
            response->set_success(false);
            response->set_error(e.what());
            return grpc::Status(grpc::StatusCode::INTERNAL, e.what());
        }
    }
    
    // ... other method implementations
    
private:
    std::optional<ShardCertificateInfo> getPeerCertificateInfo(
        const std::shared_ptr<const grpc::AuthContext>& auth_context
    ) {
        // Extract peer certificate from auth context
        // Parse and return shard certificate info
        // TODO: Implement certificate extraction and parsing
        return std::nullopt;
    }
};

class RPCShardServer {
public:
    RPCShardServer(const std::string& cert_path,
                   const std::string& key_path,
                   const std::string& ca_cert_path) 
        : cert_path_(cert_path)
        , key_path_(key_path)
        , ca_cert_path_(ca_cert_path)
    {}
    
    bool start(const std::string& listen_address) {
        grpc::ServerBuilder builder;
        
        // Configure mTLS
        grpc::SslServerCredentialsOptions ssl_opts;
        ssl_opts.client_certificate_request = 
            GRPC_SSL_REQUEST_AND_REQUIRE_CLIENT_CERTIFICATE_AND_VERIFY;
        
        // Load server certificate and key
        grpc::SslServerCredentialsOptions::PemKeyCertPair key_cert_pair;
        key_cert_pair.private_key = readFile(key_path_);
        key_cert_pair.cert_chain = readFile(cert_path_);
        ssl_opts.pem_key_cert_pairs.push_back(key_cert_pair);
        
        // Load CA certificate for client verification
        ssl_opts.pem_root_certs = readFile(ca_cert_path_);
        
        auto creds = grpc::SslServerCredentials(ssl_opts);
        
        // Register service
        builder.AddListeningPort(listen_address, creds);
        builder.RegisterService(&service_);
        
        // Build and start
        server_ = builder.BuildAndStart();
        
        if (server_) {
            THEMIS_INFO("RPC Shard Server listening on {} with mTLS", listen_address);
            return true;
        }
        
        return false;
    }
    
    void stop() {
        if (server_) {
            server_->Shutdown();
        }
    }
    
    void wait() {
        if (server_) {
            server_->Wait();
        }
    }
    
private:
    std::string cert_path_;
    std::string key_path_;
    std::string ca_cert_path_;
    ShardServiceImpl service_;
    std::unique_ptr<grpc::Server> server_;
    
    std::string readFile(const std::string& path) {
        std::ifstream file(path);
        if (!file) {
            throw std::runtime_error("Failed to read file: " + path);
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }
};

} // namespace sharding
} // namespace themis
```

**Client-Side (Calling Shard):**

```cpp
// src/sharding/rpc_shard_client.cpp

#include <grpcpp/grpcpp.h>
#include <grpcpp/security/credentials.h>
#include "shard_rpc.grpc.pb.h"

namespace themis {
namespace sharding {

class RPCShardClient {
public:
    struct Config {
        std::string cert_path;      // Client certificate
        std::string key_path;       // Private key
        std::string ca_cert_path;   // CA certificate
        std::string target_address; // Target shard address (e.g., "shard-002.dc1:50051")
    };
    
    explicit RPCShardClient(const Config& config) 
        : config_(config)
    {
        initializeChannel();
    }
    
    bool replicateData(const std::vector<Entity>& entities, 
                      const std::string& transaction_id = "") {
        ReplicateRequest request;
        request.set_shard_id(getLocalShardId());
        request.set_timestamp_ns(getCurrentTimestampNs());
        if (!transaction_id.empty()) {
            request.set_transaction_id(transaction_id);
        }
        
        for (const auto& entity : entities) {
            auto* e = request.add_entities();
            e->set_uuid(entity.uuid);
            e->set_collection(entity.collection);
            e->set_data(entity.data);
            e->set_version(entity.version);
            e->set_timestamp_ns(entity.timestamp_ns);
        }
        
        ReplicateResponse response;
        grpc::ClientContext context;
        
        // Set timeout
        auto deadline = std::chrono::system_clock::now() + 
                       std::chrono::seconds(30);
        context.set_deadline(deadline);
        
        // Make RPC call
        grpc::Status status = stub_->ReplicateData(&context, request, &response);
        
        if (status.ok()) {
            return response.success();
        } else {
            THEMIS_ERROR("ReplicateData RPC failed: {} ({})", 
                status.error_message(), status.error_code());
            return false;
        }
    }
    
    bool replicateDataStream(const std::vector<Entity>& entities) {
        grpc::ClientContext context;
        ReplicateResponse response;
        
        auto stream = stub_->ReplicateDataStream(&context, &response);
        
        // Stream entities in chunks
        const size_t chunk_size = 100; // entities per chunk
        for (size_t i = 0; i < entities.size(); i += chunk_size) {
            ReplicateChunk chunk;
            
            // Serialize chunk
            // TODO: Implement serialization
            
            chunk.set_chunk_index(i / chunk_size);
            chunk.set_is_last(i + chunk_size >= entities.size());
            
            if (!stream->Write(chunk)) {
                THEMIS_ERROR("Failed to write chunk {}", i / chunk_size);
                break;
            }
        }
        
        stream->WritesDone();
        grpc::Status status = stream->Finish();
        
        if (status.ok()) {
            return response.success();
        } else {
            THEMIS_ERROR("ReplicateDataStream RPC failed: {}", 
                status.error_message());
            return false;
        }
    }
    
    std::optional<StatusResponse> getShardStatus(bool include_metrics = false) {
        StatusRequest request;
        request.set_include_metrics(include_metrics);
        
        StatusResponse response;
        grpc::ClientContext context;
        
        grpc::Status status = stub_->GetShardStatus(&context, request, &response);
        
        if (status.ok()) {
            return response;
        } else {
            THEMIS_ERROR("GetShardStatus RPC failed: {}", status.error_message());
            return std::nullopt;
        }
    }
    
private:
    Config config_;
    std::shared_ptr<grpc::Channel> channel_;
    std::unique_ptr<themis::sharding::ShardService::Stub> stub_;
    
    void initializeChannel() {
        // Configure mTLS credentials
        grpc::SslCredentialsOptions ssl_opts;
        ssl_opts.pem_root_certs = readFile(config_.ca_cert_path);
        ssl_opts.pem_private_key = readFile(config_.key_path);
        ssl_opts.pem_cert_chain = readFile(config_.cert_path);
        
        auto creds = grpc::SslCredentials(ssl_opts);
        
        // Create channel
        grpc::ChannelArguments args;
        args.SetSslTargetNameOverride(extractHostname(config_.target_address));
        
        channel_ = grpc::CreateCustomChannel(
            config_.target_address,
            creds,
            args
        );
        
        // Create stub
        stub_ = themis::sharding::ShardService::NewStub(channel_);
    }
    
    std::string readFile(const std::string& path) {
        std::ifstream file(path);
        if (!file) {
            throw std::runtime_error("Failed to read file: " + path);
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }
    
    std::string extractHostname(const std::string& address) {
        // Extract hostname from "hostname:port"
        size_t colon_pos = address.find(':');
        if (colon_pos != std::string::npos) {
            return address.substr(0, colon_pos);
        }
        return address;
    }
    
    std::string getLocalShardId() {
        // TODO: Get from configuration
        return "shard_001";
    }
    
    uint64_t getCurrentTimestampNs() {
        auto now = std::chrono::system_clock::now();
        auto duration = now.time_since_epoch();
        return std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
    }
};

} // namespace sharding
} // namespace themis
```

---

## 4. Zertifikat-Management

### 4.1 Shard-Zertifikat Struktur

**Certificate Extensions für Shards:**

```
X.509 Certificate
├── Subject: CN=shard-001.dc1.themis.local
├── Issuer: CN=themis-cluster-ca
├── Validity: 2025-01-01 to 2026-01-01
├── Subject Alternative Names:
│   ├── DNS: shard-001.dc1.themis.local
│   ├── DNS: shard-001.dc1.internal
│   ├── IP: 10.0.1.10
│   └── URI: urn:themis:shard:prod-cluster:001
└── Custom Extensions:
    ├── shardId: "shard_001"
    ├── datacenter: "dc1"
    ├── rack: "rack01"
    ├── tokenRangeStart: 0
    ├── tokenRangeEnd: 4294967295
    ├── capabilities: ["read", "write", "replicate", "admin"]
    └── role: "primary"
```

### 4.2 Zertifikat-Generierung

**Skript für Shard-Zertifikat Erstellung:**

```bash
#!/bin/bash
# scripts/generate_shard_cert.sh

SHARD_ID=$1
DATACENTER=$2
RACK=$3
TOKEN_START=$4
TOKEN_END=$5

# Generate private key
openssl genrsa -out shard-${SHARD_ID}.key 4096

# Create certificate request with custom extensions
cat > shard-${SHARD_ID}.conf <<EOF
[ req ]
default_bits = 4096
prompt = no
default_md = sha256
distinguished_name = dn
req_extensions = v3_req

[ dn ]
CN = shard-${SHARD_ID}.${DATACENTER}.themis.local
O = ThemisDB Cluster
OU = Sharding

[ v3_req ]
subjectAltName = @alt_names
1.2.3.4.5.6.7.8.1 = ASN1:UTF8String:${SHARD_ID}
1.2.3.4.5.6.7.8.2 = ASN1:UTF8String:${DATACENTER}
1.2.3.4.5.6.7.8.3 = ASN1:UTF8String:${RACK}
1.2.3.4.5.6.7.8.4 = ASN1:INTEGER:${TOKEN_START}
1.2.3.4.5.6.7.8.5 = ASN1:INTEGER:${TOKEN_END}
1.2.3.4.5.6.7.8.6 = ASN1:UTF8String:read,write,replicate,admin
1.2.3.4.5.6.7.8.7 = ASN1:UTF8String:primary

[ alt_names ]
DNS.1 = shard-${SHARD_ID}.${DATACENTER}.themis.local
DNS.2 = shard-${SHARD_ID}.${DATACENTER}.internal
URI.1 = urn:themis:shard:prod-cluster:${SHARD_ID}
EOF

# Generate CSR
openssl req -new -key shard-${SHARD_ID}.key \
    -out shard-${SHARD_ID}.csr \
    -config shard-${SHARD_ID}.conf

# Sign with CA
openssl x509 -req -in shard-${SHARD_ID}.csr \
    -CA ca.crt -CAkey ca.key -CAcreateserial \
    -out shard-${SHARD_ID}.crt \
    -days 365 -sha256 \
    -extfile shard-${SHARD_ID}.conf \
    -extensions v3_req

echo "Generated certificate: shard-${SHARD_ID}.crt"
```

---

## 5. Migration Strategy

### 5.1 Phasen-Plan

**Phase 1: Parallel Deployment (Woche 1-2)**
- ✅ Bestehende HTTP/mTLS bleibt aktiv
- ✅ RPC/gRPC mit mTLS wird zusätzlich deployed
- ✅ Feature-Flag für RPC-Nutzung

**Phase 2: Gradual Migration (Woche 3-4)**
- 🔄 Schrittweise Migration von Features zu RPC:
  - Week 3: Replication über RPC
  - Week 4: Distributed Transactions über RPC

**Phase 3: Performance Testing (Woche 5)**
- 📊 Vergleichs-Benchmarks HTTP vs. RPC
- 📊 Latenz-Messungen (intra-DC, inter-DC)
- 📊 Throughput-Tests (Bulk Replication)

**Phase 4: Full Migration (Woche 6)**
- ✅ Alle Shard-Kommunikation über RPC
- ✅ HTTP/mTLS nur noch für backwards compatibility

### 5.2 Feature Flags

```yaml
# config/sharding.yaml

sharding:
  communication:
    # Protocol selection
    rpc_enabled: true
    rpc_fallback_to_http: true  # Fallback if RPC fails
    
    # RPC configuration
    rpc:
      protocol: "grpc"  # grpc, thrift
      listen_address: "0.0.0.0:50051"
      
      # mTLS configuration
      mtls:
        enabled: true
        cert_path: "/etc/themis/certs/shard.crt"
        key_path: "/etc/themis/certs/shard.key"
        ca_cert_path: "/etc/themis/certs/ca.crt"
        crl_path: "/etc/themis/certs/crl.pem"
        verify_peer: true
        verify_hostname: true
      
      # Connection settings
      connection:
        max_connections_per_shard: 10
        connection_timeout_ms: 5000
        request_timeout_ms: 30000
        keepalive_time_ms: 60000
    
    # Legacy HTTP/mTLS (for compatibility)
    http:
      enabled: true
      port: 8080
```

---

## 6. Performance Expectations

### 6.1 Benchmarks (Projected)

| Metric | HTTP/REST + mTLS | gRPC + mTLS | Improvement |
|--------|------------------|-------------|-------------|
| **Latency (p50)** | 5.0 ms | 0.8 ms | 6.25x faster |
| **Latency (p99)** | 15.0 ms | 2.5 ms | 6x faster |
| **Throughput** | 2,000 ops/s | 15,000 ops/s | 7.5x faster |
| **Bandwidth** | 10 MB/s | 60 MB/s | 6x faster |
| **CPU Usage** | 45% | 15% | 3x lower |

**Bulk Replication (10,000 entities):**
- HTTP/REST: ~15 seconds
- gRPC Streaming: ~2 seconds
- **Improvement: 7.5x faster**

### 6.2 Latenz-Breakdown

**Inter-Shard Request (intra-datacenter):**

```
HTTP/REST + mTLS:
├── TLS Handshake: 2.0 ms
├── HTTP Header Parsing: 0.5 ms
├── JSON Deserialization: 1.5 ms
├── Processing: 0.5 ms
├── JSON Serialization: 1.0 ms
└── Response: 0.5 ms
Total: ~6.0 ms

gRPC + mTLS:
├── TLS Handshake (cached): 0.1 ms
├── Protobuf Deserialization: 0.2 ms
├── Processing: 0.5 ms
├── Protobuf Serialization: 0.1 ms
└── Response: 0.1 ms
Total: ~1.0 ms
```

---

## 7. Security Considerations

### 7.1 Threat Model

**Threats:**
1. **Man-in-the-Middle (MITM)** - Abfangen/Manipulation von Shard-Kommunikation
2. **Rogue Shard** - Unautorisierter Shard versucht Cluster-Zugriff
3. **Certificate Theft** - Gestohlenes Zertifikat wird missbraucht
4. **Replay Attacks** - Alte Requests werden wiederholt

**Mitigations:**
1. **mTLS** - Verhindert MITM durch verschlüsselte Kommunikation
2. **Certificate Verification** - Nur signierte Zertifikate von Root CA akzeptiert
3. **CRL Checking** - Gestohlene Zertifikate werden revoked
4. **Timestamp Validation** - Requests mit veralteten Timestamps ablehnen

### 7.2 Capability-Based Access Control

```cpp
// Verify shard has required capability
bool verifyShardCapability(const ShardCertificateInfo& cert,
                          const std::string& operation) {
    static const std::map<std::string, std::vector<std::string>> required_caps = {
        {"replicate", {"replicate", "write"}},
        {"migrate", {"admin"}},
        {"read", {"read"}},
        {"write", {"write"}}
    };
    
    auto it = required_caps.find(operation);
    if (it == required_caps.end()) {
        return false;  // Unknown operation
    }
    
    for (const auto& required_cap : it->second) {
        if (cert.hasCapability(required_cap)) {
            return true;
        }
    }
    
    return false;
}
```

---

## 8. Monitoring & Observability

### 8.1 Metrics

**RPC-spezifische Metriken:**

```cpp
// OpenTelemetry Metrics
- rpc_shard_requests_total{protocol="grpc", operation="replicate", status="success"}
- rpc_shard_request_duration_seconds{protocol="grpc", operation="replicate"}
- rpc_shard_active_connections{protocol="grpc", target_shard="shard_002"}
- rpc_shard_bytes_sent_total{protocol="grpc"}
- rpc_shard_bytes_received_total{protocol="grpc"}
- rpc_shard_certificate_verification_failures_total
- rpc_shard_tls_handshake_duration_seconds
```

### 8.2 Distributed Tracing

```cpp
// OpenTelemetry Trace Propagation
auto span = tracer->StartSpan("shard.replicate");
span->SetAttribute("target_shard", "shard_002");
span->SetAttribute("entity_count", entities.size());
span->SetAttribute("transaction_id", tx_id);

// Propagate trace context via gRPC metadata
grpc::ClientContext context;
auto trace_context = span->GetContext();
context.AddMetadata("traceparent", serializeTraceContext(trace_context));

// Make RPC call
stub->ReplicateData(&context, request, &response);

span->End();
```

---

## 9. Next Steps & Open Questions

### 9.1 Implementation Tasks

- [ ] **Woche 1-2: Protobuf Definitions**
  - [ ] Erstelle `shard_rpc.proto` mit allen Inter-Shard Operations
  - [ ] Generiere C++ Code mit protoc/grpc_cpp_plugin
  - [ ] Dokumentiere Protobuf Messages

- [ ] **Woche 3-4: gRPC Server/Client**
  - [ ] Implementiere `RPCShardServer` mit mTLS
  - [ ] Implementiere `RPCShardClient` mit Connection Pooling
  - [ ] Integriere `PKIShardCertificate` für Cert-Parsing

- [ ] **Woche 5: Testing**
  - [ ] Unit Tests für RPC Service Methods
  - [ ] Integration Tests (Multi-Shard Setup)
  - [ ] mTLS Certificate Verification Tests
  - [ ] Performance Benchmarks

- [ ] **Woche 6: Migration**
  - [ ] Feature Flag Implementation
  - [ ] Gradual Rollout zu Production Cluster
  - [ ] Monitoring & Alerting Setup

### 9.2 Open Questions

1. **Certificate Rotation:**
   - Wie handhaben wir Zertifikat-Rotation ohne Downtime?
   - Automatische Renewal-Prozess?

2. **CRL Distribution:**
   - Wie wird CRL aktualisiert und verteilt?
   - OCSP (Online Certificate Status Protocol) als Alternative?

3. **Performance Tuning:**
   - Optimale gRPC Channel/Connection Pool Größe?
   - Keep-Alive Settings für lange Idle-Zeiten?

4. **Backward Compatibility:**
   - Wie lange HTTP/REST parallel laufen lassen?
   - Migration Path für alte Clients?

5. **Cross-Datacenter:**
   - Spezielle Optimierungen für WAN-Latenz?
   - Compression für Inter-DC Traffic?

---

## 10. Fazit

Die Nutzung des RPC Frameworks mit mTLS für Inter-Shard Kommunikation bietet signifikante Vorteile:

✅ **Performance:** 6-8x schneller als HTTP/REST  
✅ **Security:** Mutual TLS mit Zertifikat-basierter Authentifizierung  
✅ **Capabilities:** Feinkörnige Zugriffskontrolle über Cert-Extensions  
✅ **Streaming:** Effiziente Bulk-Operationen (Replication, Migration)  
✅ **Integration:** Nutzt bestehende PKI-Infrastruktur  

**Empfehlung:** Schrittweise Migration zu gRPC mit mTLS für alle Inter-Shard Kommunikation.

---

## 11. Verwandte Dokumentation

- [RPC Plugin Architecture](RPC_PLUGIN_ARCHITECTURE.md)
- [Inter-Shard Data Pipeline Analysis](INTER_SHARD_DATA_PIPELINE_ANALYSIS.md) ⭐ NEU
- [mTLS Client Implementation](../../include/sharding/mtls_client.h)
- [PKI Shard Certificate](../../include/sharding/pki_shard_certificate.h)
- [Sharding Architecture](../architecture/sharding_architecture.md)
- [Security Best Practices](../architecture/SECURITY_ARCHITECTURE.md)

---

## 12. Updates in v1.3.0

### Neue Features für Inter-Shard Communication

**RocksDB Snapshot Transfer:**
- Snapshot-basierter Bulk-Transfer für große Shards
- 10-20x schneller als Record-by-Record
- Compression mit Zstd (Level 9)
- Chunking mit CRC32 Checksums

**Blob Transfer für LoRA Adapters:**
- Dedicated Blob Transfer Service
- Optimiert für große Binärdateien (100 MB - 10 GB)
- High Compression (3-6x mit Zstd)
- Resume Support bei Unterbrechungen

**Enhanced Compression & Chunking:**
- Konfigurierbare Chunk-Größen (1-100 MB)
- Multiple Compression Algorithms (LZ4, Zstd, Snappy)
- Per-Chunk Checksums (CRC32, SHA256, XXH64)
- End-to-End Integrity Verification

Siehe [Inter-Shard Data Pipeline Analysis](INTER_SHARD_DATA_PIPELINE_ANALYSIS.md) für Details.
