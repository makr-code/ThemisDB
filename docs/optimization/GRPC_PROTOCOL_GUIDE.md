# gRPC Protocol Implementation for ThemisDB

**Date:** December 25, 2024  
**Version:** 1.0  
**Target:** ThemisDB v1.4.0+  
**Source:** THEMISDB_OPTIMIZATION_IMPACT_ANALYSIS_SUMMARY.md

---

## 📋 Executive Summary

ThemisDB now supports gRPC as an alternative protocol to HTTP/REST, providing **+30% performance improvement** and **-70% serialization overhead**. This guide covers setup, usage, and migration strategies.

**Key Benefits:**
- ✅ +30% overall performance vs HTTP/JSON
- ✅ -70% serialization overhead (Protocol Buffers vs JSON)
- ✅ Native streaming support
- ✅ Strong typing with .proto contracts
- ✅ HTTP/2 multiplexing and flow control
- ✅ Language-agnostic client generation

---

## 🎯 Performance Comparison

### HTTP/REST vs gRPC

| Metric | HTTP/REST | gRPC | Improvement |
|--------|-----------|------|-------------|
| **Serialization Overhead** | 100% (JSON) | 30% (Protobuf) | **-70%** |
| **Message Size** | 1.0× (baseline) | 0.3-0.5× | **50-70% smaller** |
| **Latency (P99)** | 5.2ms | 3.6ms | **-31%** |
| **Throughput (QPS)** | 8,500 | 11,000 | **+29%** |
| **CPU Usage** | 100% | 75% | **-25%** |

*Benchmarked on: 8-core CPU, 1KB entities, mixed read/write workload*

---

## 🚀 Quick Start

### Server Configuration

**1. Enable gRPC in config.json:**

```json
{
  "server": {
    "host": "0.0.0.0",
    "http_port": 8765,
    "grpc_port": 50051,
    "enable_grpc": true,
    "enable_http": true
  }
}
```

**2. Start ThemisDB server:**

```bash
./themisdb-server --config config.json

# Output:
# [INFO] HTTP server listening on 0.0.0.0:8765
# [INFO] gRPC server listening on 0.0.0.0:50051
```

---

### Client Usage

#### Python Client

```python
import grpc
from themis_wire_v1_pb2 import *
from themis_wire_v1_pb2_grpc import ThemisServiceStub

# Connect
channel = grpc.insecure_channel('localhost:50051')
stub = ThemisServiceStub(channel)

# Hello
hello_req = HelloRequest(
    protocol_version=1,
    client_name="python-client",
    client_version="1.0.0"
)
hello_ack = stub.Hello(hello_req)
print(f"Server version: {hello_ack.server_version}")

# PUT operation
put_req = PutRequest(
    model="documents",
    collection="articles",
    uuid="doc_001",
    entity=b'{"title": "Article 1", "content": "..."}'
)
put_resp = stub.Put(put_req)
print(f"Success: {put_resp.success}, Version: {put_resp.version}")

# GET operation
get_req = GetRequest(
    model="documents",
    collection="articles",
    uuid="doc_001"
)
get_resp = stub.Get(get_req)
if get_resp.found:
    print(f"Entity: {get_resp.entity}")
```

#### C++ Client

```cpp
#include <grpcpp/grpcpp.h>
#include "themis_wire_v1.grpc.pb.h"

auto channel = grpc::CreateChannel("localhost:50051", 
                                   grpc::InsecureChannelCredentials());
auto stub = themis::wire::v1::ThemisService::NewStub(channel);

// Hello
themis::wire::v1::HelloRequest hello_req;
hello_req.set_protocol_version(1);
hello_req.set_client_name("cpp-client");
hello_req.set_client_version("1.0.0");

themis::wire::v1::HelloAck hello_ack;
grpc::ClientContext context;
grpc::Status status = stub->Hello(&context, hello_req, &hello_ack);

if (status.ok()) {
    std::cout << "Server: " << hello_ack.server_version() << "\n";
}

// PUT operation
themis::wire::v1::PutRequest put_req;
put_req.set_model("documents");
put_req.set_collection("articles");
put_req.set_uuid("doc_001");
put_req.set_entity(R"({"title": "Article 1"})");

themis::wire::v1::PutResponse put_resp;
grpc::ClientContext put_context;
status = stub->Put(&put_context, put_req, &put_resp);

if (status.ok() && put_resp.success()) {
    std::cout << "PUT successful, version: " << put_resp.version() << "\n";
}
```

#### Go Client

```go
package main

import (
    "context"
    "google.golang.org/grpc"
    pb "path/to/themis_wire_v1"
)

func main() {
    conn, _ := grpc.Dial("localhost:50051", grpc.WithInsecure())
    defer conn.Close()
    
    client := pb.NewThemisServiceClient(conn)
    
    // Hello
    helloReq := &pb.HelloRequest{
        ProtocolVersion: 1,
        ClientName:      "go-client",
        ClientVersion:   "1.0.0",
    }
    helloAck, _ := client.Hello(context.Background(), helloReq)
    println("Server:", helloAck.ServerVersion)
    
    // PUT
    putReq := &pb.PutRequest{
        Model:      "documents",
        Collection: "articles",
        Uuid:       "doc_001",
        Entity:     []byte(`{"title": "Article 1"}`),
    }
    putResp, _ := client.Put(context.Background(), putReq)
    println("Success:", putResp.Success, "Version:", putResp.Version)
}
```

---

## 🤖 Multi-Agent LLM Integration

### Agent Task Submission

```python
# Multi-agent task via gRPC
from themis_wire_v1_pb2 import *

# Batch PUT for agent results (WriteBatch integration)
batch_req = BatchPutRequest(
    model="agent_results",
    collection="tasks",
    items=[
        PutItem(
            uuid=f"result_{agent_id}",
            entity=result.SerializeToString()
        )
        for agent_id, result in agent_results.items()
    ]
)

batch_resp = stub.BatchPut(batch_req)
print(f"Committed {batch_resp.success_count} agent results atomically")
```

### Vector Search for Agent Context

```python
# Vector search for document retrieval
vector_req = VectorSearchRequest(
    collection="documents",
    vector=[0.1, 0.2, ...],  # 384-dim embedding
    k=10,
    distance_metric=DistanceMetric.COSINE,
    ef_search=96  # Production preset
)

vector_resp = stub.VectorSearch(vector_req)
for result in vector_resp.results:
    print(f"Document: {result.uuid}, Distance: {result.distance}")
```

---

## 📊 Protocol Buffers Schema

### Core Messages

**Connection:**
- `HelloRequest` / `HelloAck` - Connection handshake
- `AuthRequest` / `AuthResponse` / `AuthSuccess` - Authentication

**CRUD:**
- `GetRequest` / `GetResponse` - Single entity retrieval
- `PutRequest` / `PutResponse` - Single entity write
- `DeleteRequest` / `DeleteResponse` - Single entity deletion
- `BatchGetRequest` / `BatchGetResponse` - Bulk retrieval
- `BatchPutRequest` / `BatchPutResponse` - Atomic bulk write (WriteBatch)

**Query:**
- `QueryRequest` / `QueryResult` - AQL query execution
- `VectorSearchRequest` / `VectorSearchResponse` - HNSW vector search

**Transaction:**
- `TransactionBeginRequest` / `TransactionBeginResponse`
- `TransactionCommitRequest` / `TransactionCommitResponse`
- `TransactionAbortRequest` / `TransactionAbortResponse`

**Full schema:** `src/network/themis_wire_v1.proto`

---

## 🔧 Advanced Features

### 1. Streaming RPCs

**Server-side streaming for large result sets:**

```proto
// Future enhancement
rpc StreamQuery(QueryRequest) returns (stream QueryResult);
```

```python
# Client code (when implemented)
for result_batch in stub.StreamQuery(query_req):
    process_batch(result_batch.results)
```

### 2. Compression

**Enable gzip compression:**

```python
# Python
channel_options = [
    ('grpc.default_compression_algorithm', grpc.Compression.Gzip),
    ('grpc.default_compression_level', grpc.CompressionLevel.Medium)
]
channel = grpc.insecure_channel('localhost:50051', options=channel_options)
```

```cpp
// C++
grpc::ChannelArguments args;
args.SetCompressionAlgorithm(GRPC_COMPRESS_GZIP);
auto channel = grpc::CreateCustomChannel("localhost:50051", creds, args);
```

**Impact:** -50% network bandwidth for large payloads

### 3. Deadlines and Timeouts

```python
# Set timeout for operation
response = stub.Get(request, timeout=5.0)  # 5 second timeout
```

```cpp
// C++ deadline
grpc::ClientContext context;
auto deadline = std::chrono::system_clock::now() + std::chrono::seconds(5);
context.set_deadline(deadline);
stub->Get(&context, request, &response);
```

### 4. Metadata (Headers)

```python
# Add custom metadata
metadata = [
    ('session-id', session_id),
    ('trace-id', trace_id),
]
response = stub.Get(request, metadata=metadata)
```

---

## 🎓 Migration Strategies

### Strategy 1: Gradual Migration (Recommended)

**Phase 1:** Run both protocols in parallel
```json
{
  "server": {
    "enable_grpc": true,
    "enable_http": true  // Keep both enabled
  }
}
```

**Phase 2:** Migrate high-volume clients to gRPC
```bash
# Performance-critical clients use gRPC
client.connect("grpc://localhost:50051")

# Legacy/admin clients still use HTTP
curl http://localhost:8765/api/...
```

**Phase 3:** Deprecate HTTP (optional)
```json
{
  "server": {
    "enable_grpc": true,
    "enable_http": false  // Disable HTTP after full migration
  }
}
```

### Strategy 2: Protocol-Per-Service

**Use gRPC for:**
- High-throughput data ingestion
- Real-time vector search
- Multi-agent orchestration (batch operations)

**Use HTTP for:**
- Admin/management APIs
- Human-readable debugging
- Web browser integration

---

## 📈 Performance Tuning

### Server-Side

**1. Thread Pool Size:**

```cpp
::grpc::ServerBuilder builder;
builder.SetSyncServerOption(
    ::grpc::ServerBuilder::SyncServerOption::NUM_CQS, 8);  // 8 completion queues
builder.SetSyncServerOption(
    ::grpc::ServerBuilder::SyncServerOption::MIN_POLLERS, 4);
builder.SetSyncServerOption(
    ::grpc::ServerBuilder::SyncServerOption::MAX_POLLERS, 16);
```

**2. Message Size Limits:**

```cpp
builder.SetMaxReceiveMessageSize(100 * 1024 * 1024);  // 100MB
builder.SetMaxSendMessageSize(100 * 1024 * 1024);
```

**3. Keepalive Settings:**

```cpp
builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_TIME_MS, 30000);
builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_TIMEOUT_MS, 10000);
```

### Client-Side

**1. Connection Pooling:**

```python
# Reuse channels
class GRPCClientPool:
    def __init__(self, address, pool_size=10):
        self.channels = [
            grpc.insecure_channel(address)
            for _ in range(pool_size)
        ]
        self.stubs = [
            ThemisServiceStub(ch) for ch in self.channels
        ]
        self.index = 0
    
    def get_stub(self):
        stub = self.stubs[self.index]
        self.index = (self.index + 1) % len(self.stubs)
        return stub

pool = GRPCClientPool('localhost:50051')
stub = pool.get_stub()
```

**2. Retry Policy:**

```python
service_config = {
    "methodConfig": [{
        "name": [{"service": "themis.wire.v1.ThemisService"}],
        "retryPolicy": {
            "maxAttempts": 3,
            "initialBackoff": "0.1s",
            "maxBackoff": "1s",
            "backoffMultiplier": 2,
            "retryableStatusCodes": ["UNAVAILABLE", "DEADLINE_EXCEEDED"]
        }
    }]
}

channel = grpc.insecure_channel(
    'localhost:50051',
    options=[('grpc.service_config', json.dumps(service_config))]
)
```

---

## 🔍 Monitoring & Debugging

### Server Metrics

```bash
# gRPC server metrics (Prometheus)
curl http://localhost:8765/metrics | grep grpc

# Example metrics:
grpc_server_started_total{grpc_method="Get",grpc_service="ThemisService"} 1250
grpc_server_handled_total{grpc_code="OK",grpc_method="Get"} 1240
grpc_server_handled_total{grpc_code="INTERNAL",grpc_method="Get"} 10
grpc_server_handling_seconds{quantile="0.99",grpc_method="Get"} 0.0036
```

### Client-Side Logging

```python
# Enable gRPC debug logging
import logging
logging.basicConfig()
logging.getLogger('grpc').setLevel(logging.DEBUG)
```

### Network Tracing

```bash
# Wireshark filter for gRPC (HTTP/2)
tcp.port == 50051 and http2

# grpcurl for manual testing
grpcurl -plaintext localhost:50051 list
grpcurl -plaintext localhost:50051 themis.wire.v1.ThemisService.Ping
```

---

## 🐛 Troubleshooting

### Problem: Connection Refused

**Solution:**
```bash
# Check if gRPC is enabled
grep enable_grpc config.json

# Check if port is open
netstat -an | grep 50051

# Check firewall
sudo ufw allow 50051/tcp
```

### Problem: Large Message Errors

**Error:** `RESOURCE_EXHAUSTED: Received message larger than max`

**Solution:**
```python
# Increase client limit
channel_options = [
    ('grpc.max_receive_message_length', 100 * 1024 * 1024)
]
channel = grpc.insecure_channel('localhost:50051', options=channel_options)
```

### Problem: Deadline Exceeded

**Error:** `DEADLINE_EXCEEDED: Deadline Exceeded`

**Solution:**
1. Increase client timeout
2. Optimize server-side query
3. Check network latency

---

## 📚 References

1. **THEMISDB_OPTIMIZATION_IMPACT_ANALYSIS_SUMMARY.md**
2. **gRPC Official Docs:** https://grpc.io/docs/
3. **Protocol Buffers:** https://developers.google.com/protocol-buffers
4. **gRPC Performance Best Practices:** https://grpc.io/docs/guides/performance/
5. **themis_wire_v1.proto:** `src/network/themis_wire_v1.proto`

---

## ✅ Production Checklist

- [ ] gRPC enabled in config.json
- [ ] TLS/SSL configured for production
- [ ] Authentication implemented
- [ ] Connection limits configured
- [ ] Monitoring/metrics enabled
- [ ] Load balancing setup (if multi-server)
- [ ] Client retry policies configured
- [ ] Performance benchmarks run

---

**Remember:** gRPC provides +30% performance over HTTP/REST but requires Protocol Buffers compilation and language-specific client generation. Use HTTP for quick prototyping and admin interfaces, gRPC for production workloads.
