# gRPC API Specification - ThemisDB v1.4

**Version:** 1.4.0  
**Status:** ✅ Produktionsreif  
**Aktualisiert:** Januar 2026

---

## 📑 Inhaltsverzeichnis

- [Übersicht](#übersicht)
- [Protocol Buffers Definition](#protocol-buffers-definition)
- [Service Endpoints](#service-endpoints)
- [Authentifizierung](#authentifizierung)
- [Core Services](#core-services)
- [LLM Services](#llm-services)
- [Sharding Services](#sharding-services)
- [Fehlerbehandlung](#fehlerbehandlung)
- [Streaming](#streaming)
- [Beispiele](#beispiele)

---

## Übersicht

ThemisDB bietet eine vollständige gRPC-API für high-performance Kommunikation. gRPC verwendet Protocol Buffers für effiziente Serialisierung und unterstützt bidirektionales Streaming.

### gRPC Vorteile

- ✅ **Performance**: Binäres Protokoll mit geringer Latenz
- ✅ **Type Safety**: Starke Typisierung durch Protocol Buffers
- ✅ **Bidirektionales Streaming**: Server- und Client-seitiges Streaming
- ✅ **Load Balancing**: Native Unterstützung für Load Balancing
- ✅ **Multi-Language Support**: Code-Generierung für viele Sprachen

---

## Protocol Buffers Definition

### Basis-URL

```
grpc://your-themis-instance.com:50051
```

### Proto-Dateien

ThemisDB verwendet folgende Proto-Definitionen:

- `proto/themis_core.proto` - Core Database Services
- `proto/llm_service.proto` - LLM Integration Services
- `proto/sharding/shard_rpc.proto` - Distributed Sharding Services

---

## Core Services

### ThemisCoreService

Haupt-Service für Database-Operationen.

#### Proto Definition

```protobuf
syntax = "proto3";

package themis;

service ThemisCoreService {
  // Document Operations
  rpc InsertDocument(InsertDocumentRequest) returns (InsertDocumentResponse);
  rpc GetDocument(GetDocumentRequest) returns (GetDocumentResponse);
  rpc UpdateDocument(UpdateDocumentRequest) returns (UpdateDocumentResponse);
  rpc DeleteDocument(DeleteDocumentRequest) returns (DeleteDocumentResponse);
  
  // Query Operations
  rpc ExecuteQuery(ExecuteQueryRequest) returns (ExecuteQueryResponse);
  rpc StreamQuery(ExecuteQueryRequest) returns (stream QueryResultChunk);
  
  // Batch Operations
  rpc BatchInsert(BatchInsertRequest) returns (BatchInsertResponse);
  rpc BatchUpdate(BatchUpdateRequest) returns (BatchUpdateResponse);
  
  // Transaction Operations
  rpc BeginTransaction(BeginTransactionRequest) returns (TransactionResponse);
  rpc CommitTransaction(CommitTransactionRequest) returns (TransactionResponse);
  rpc RollbackTransaction(RollbackTransactionRequest) returns (TransactionResponse);
}

message InsertDocumentRequest {
  string database = 1;
  string collection = 2;
  bytes document = 3;  // JSON-encoded document
  map<string, string> metadata = 4;
}

message InsertDocumentResponse {
  string document_id = 1;
  string revision = 2;
  int64 timestamp = 3;
  bool success = 4;
  string error_message = 5;
}

message ExecuteQueryRequest {
  string database = 1;
  string query = 2;  // AQL query string
  map<string, bytes> bind_vars = 3;
  QueryOptions options = 4;
}

message QueryOptions {
  int32 batch_size = 1;
  int32 ttl = 2;
  bool count = 3;
  bool full_count = 4;
  int32 max_plans = 5;
}

message ExecuteQueryResponse {
  repeated bytes results = 1;  // JSON-encoded results
  int64 count = 2;
  bool has_more = 3;
  QueryStats stats = 4;
}

message QueryStats {
  int64 execution_time_ms = 1;
  int64 scanned = 2;
  int64 filtered = 3;
  int64 index_hits = 4;
}
```

---

## Beispiele

### Beispiel 1: Dokument einfügen (Go)

```go
package main

import (
    "context"
    "log"
    "google.golang.org/grpc"
    pb "themisdb/proto"
)

func main() {
    conn, err := grpc.Dial("localhost:50051", grpc.WithInsecure())
    if err != nil {
        log.Fatalf("Failed to connect: %v", err)
    }
    defer conn.Close()
    
    client := pb.NewThemisCoreServiceClient(conn)
    
    // Insert document
    req := &pb.InsertDocumentRequest{
        Database:   "mydb",
        Collection: "users",
        Document:   []byte(`{"name":"John Doe","age":30}`),
        Metadata: map[string]string{
            "source": "api",
        },
    }
    
    resp, err := client.InsertDocument(context.Background(), req)
    if err != nil {
        log.Fatalf("InsertDocument failed: %v", err)
    }
    
    log.Printf("Document inserted: ID=%s, Rev=%s", resp.DocumentId, resp.Revision)
}
```

### Beispiel 2: AQL Query ausführen (Python)

```python
import grpc
import themis_pb2
import themis_pb2_grpc
import json

def execute_query():
    channel = grpc.insecure_channel('localhost:50051')
    stub = themis_pb2_grpc.ThemisCoreServiceStub(channel)
    
    # Prepare query
    request = themis_pb2.ExecuteQueryRequest(
        database='mydb',
        query='FOR doc IN users FILTER doc.age > @minAge RETURN doc',
        bind_vars={
            'minAge': json.dumps(25).encode('utf-8')
        },
        options=themis_pb2.QueryOptions(
            batch_size=100,
            count=True
        )
    )
    
    # Execute query
    response = stub.ExecuteQuery(request)
    
    # Process results
    for result_bytes in response.results:
        doc = json.loads(result_bytes)
        print(f"Name: {doc['name']}, Age: {doc['age']}")
    
    print(f"Total: {response.count}, Time: {response.stats.execution_time_ms}ms")

if __name__ == '__main__':
    execute_query()
```

### Beispiel 3: Streaming Query (C++)

```cpp
#include <grpc++/grpc++.h>
#include "themis_core.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using themis::ThemisCoreService;
using themis::ExecuteQueryRequest;
using themis::QueryResultChunk;

void streamQuery() {
    auto channel = grpc::CreateChannel("localhost:50051", 
                                       grpc::InsecureChannelCredentials());
    auto stub = ThemisCoreService::NewStub(channel);
    
    // Prepare query
    ExecuteQueryRequest request;
    request.set_database("mydb");
    request.set_query("FOR doc IN large_collection RETURN doc");
    
    ClientContext context;
    QueryResultChunk chunk;
    
    // Stream results
    auto reader = stub->StreamQuery(&context, request);
    while (reader->Read(&chunk)) {
        // Process chunk
        std::cout << "Received chunk with " << chunk.results_size() 
                  << " documents" << std::endl;
        
        for (const auto& result : chunk.results()) {
            // Process each document
            std::cout << result << std::endl;
        }
    }
    
    Status status = reader->Finish();
    if (status.ok()) {
        std::cout << "Query completed successfully" << std::endl;
    } else {
        std::cerr << "Query failed: " << status.error_message() << std::endl;
    }
}
```

### Beispiel 4: Batch Insert (Java)

```java
import io.grpc.ManagedChannel;
import io.grpc.ManagedChannelBuilder;
import themis.ThemisCoreServiceGrpc;
import themis.ThemisCore.*;

import java.util.ArrayList;
import java.util.List;

public class ThemisGrpcExample {
    public static void main(String[] args) {
        ManagedChannel channel = ManagedChannelBuilder
            .forAddress("localhost", 50051)
            .usePlaintext()
            .build();
        
        ThemisCoreServiceGrpc.ThemisCoreServiceBlockingStub stub = 
            ThemisCoreServiceGrpc.newBlockingStub(channel);
        
        // Prepare documents
        List<ByteString> documents = new ArrayList<>();
        documents.add(ByteString.copyFromUtf8("{\"name\":\"Alice\",\"age\":25}"));
        documents.add(ByteString.copyFromUtf8("{\"name\":\"Bob\",\"age\":30}"));
        documents.add(ByteString.copyFromUtf8("{\"name\":\"Charlie\",\"age\":35}"));
        
        // Batch insert
        BatchInsertRequest request = BatchInsertRequest.newBuilder()
            .setDatabase("mydb")
            .setCollection("users")
            .addAllDocuments(documents)
            .build();
        
        BatchInsertResponse response = stub.batchInsert(request);
        
        System.out.println("Inserted: " + response.getInsertedCount());
        System.out.println("Failed: " + response.getFailedCount());
        
        channel.shutdown();
    }
}
```

---

## LLM Services

### LLMService für AI/ML-Integration

```protobuf
service LLMService {
  rpc LoadModel(LoadModelRequest) returns (LoadModelResponse);
  rpc UnloadModel(UnloadModelRequest) returns (UnloadModelResponse);
  rpc Generate(GenerateRequest) returns (GenerateResponse);
  rpc StreamGenerate(GenerateRequest) returns (stream GenerateStreamChunk);
  rpc ApplyLoRA(ApplyLoRARequest) returns (ApplyLoRAResponse);
  rpc GetModelInfo(GetModelInfoRequest) returns (ModelInfo);
}

message GenerateRequest {
  string model_id = 1;
  string prompt = 2;
  GenerateOptions options = 3;
  repeated string lora_adapters = 4;
}

message GenerateOptions {
  int32 max_tokens = 1;
  float temperature = 2;
  float top_p = 3;
  int32 top_k = 4;
  repeated string stop_sequences = 5;
}

message GenerateResponse {
  string text = 1;
  int32 tokens_generated = 2;
  float generation_time_ms = 3;
  TokenStats token_stats = 4;
}
```

#### Beispiel: LLM Inference (Python)

```python
import grpc
import llm_service_pb2
import llm_service_pb2_grpc

def generate_text():
    channel = grpc.insecure_channel('localhost:50052')
    stub = llm_service_pb2_grpc.LLMServiceStub(channel)
    
    request = llm_service_pb2.GenerateRequest(
        model_id='llama-2-7b',
        prompt='Explain quantum computing in simple terms:',
        options=llm_service_pb2.GenerateOptions(
            max_tokens=200,
            temperature=0.7,
            top_p=0.95
        ),
        lora_adapters=['physics-expert']
    )
    
    response = stub.Generate(request)
    print(f"Generated text: {response.text}")
    print(f"Tokens: {response.tokens_generated}, Time: {response.generation_time_ms}ms")
```

---

## Sharding Services

### ShardRpcService für verteilte Operationen

```protobuf
service ShardRpcService {
  rpc RouteQuery(RouteQueryRequest) returns (RouteQueryResponse);
  rpc ExecuteOnShard(ExecuteOnShardRequest) returns (ExecuteOnShardResponse);
  rpc CrossShardJoin(CrossShardJoinRequest) returns (CrossShardJoinResponse);
  rpc RebalanceShards(RebalanceRequest) returns (RebalanceResponse);
  rpc GetShardStatus(GetShardStatusRequest) returns (ShardStatusResponse);
}

message RouteQueryRequest {
  string query = 1;
  map<string, bytes> bind_vars = 2;
  RoutingStrategy strategy = 3;
}

enum RoutingStrategy {
  HASH_BASED = 0;
  RANGE_BASED = 1;
  BROADCAST = 2;
  SMART_ROUTING = 3;
}
```

---

## Authentifizierung

### Metadata-basierte Authentifizierung

gRPC verwendet Metadata für Authentifizierung:

```python
import grpc

# Add authentication metadata
metadata = [
    ('authorization', 'Bearer YOUR_JWT_TOKEN'),
    ('x-api-key', 'your-api-key')
]

response = stub.ExecuteQuery(request, metadata=metadata)
```

### mTLS (Mutual TLS)

```python
import grpc

# Load certificates
with open('client.crt', 'rb') as f:
    client_cert = f.read()
with open('client.key', 'rb') as f:
    client_key = f.read()
with open('ca.crt', 'rb') as f:
    ca_cert = f.read()

# Create credentials
credentials = grpc.ssl_channel_credentials(
    root_certificates=ca_cert,
    private_key=client_key,
    certificate_chain=client_cert
)

# Create secure channel
channel = grpc.secure_channel('localhost:50051', credentials)
```

---

## Fehlerbehandlung

### gRPC Status Codes

| Code | Name | Bedeutung |
|------|------|-----------|
| 0 | OK | Erfolg |
| 1 | CANCELLED | Operation abgebrochen |
| 2 | UNKNOWN | Unbekannter Fehler |
| 3 | INVALID_ARGUMENT | Ungültige Parameter |
| 4 | DEADLINE_EXCEEDED | Timeout |
| 5 | NOT_FOUND | Ressource nicht gefunden |
| 6 | ALREADY_EXISTS | Ressource existiert bereits |
| 7 | PERMISSION_DENIED | Keine Berechtigung |
| 8 | RESOURCE_EXHAUSTED | Rate Limit erreicht |
| 13 | INTERNAL | Interner Server-Fehler |
| 14 | UNAVAILABLE | Service nicht verfügbar |

### Fehlerbehandlung Beispiel

```python
import grpc

try:
    response = stub.InsertDocument(request)
except grpc.RpcError as e:
    if e.code() == grpc.StatusCode.ALREADY_EXISTS:
        print("Document already exists")
    elif e.code() == grpc.StatusCode.PERMISSION_DENIED:
        print("No permission to insert document")
    else:
        print(f"Error: {e.details()}")
```

---

## Streaming

### Server-Side Streaming

```python
def stream_query_results():
    request = themis_pb2.ExecuteQueryRequest(
        database='mydb',
        query='FOR doc IN large_collection RETURN doc'
    )
    
    for chunk in stub.StreamQuery(request):
        for result in chunk.results:
            process_document(json.loads(result))
```

### Client-Side Streaming

```python
def batch_insert_stream():
    def generate_requests():
        for i in range(1000):
            yield themis_pb2.InsertDocumentRequest(
                database='mydb',
                collection='users',
                document=json.dumps({'id': i, 'value': f'doc_{i}'}).encode()
            )
    
    response = stub.BatchInsertStream(generate_requests())
    print(f"Inserted {response.inserted_count} documents")
```

### Bidirektionales Streaming

```python
def bidirectional_stream():
    def generate_queries():
        queries = [
            'FOR doc IN users FILTER doc.age > 20 RETURN doc',
            'FOR doc IN users FILTER doc.age > 30 RETURN doc',
            'FOR doc IN users FILTER doc.age > 40 RETURN doc'
        ]
        for query in queries:
            yield themis_pb2.ExecuteQueryRequest(
                database='mydb',
                query=query
            )
    
    responses = stub.BidirectionalQuery(generate_queries())
    for response in responses:
        print(f"Received {len(response.results)} results")
```

---

## Performance Best Practices

### Connection Pooling

```python
from grpc import insecure_channel
from concurrent.futures import ThreadPoolExecutor

# Create channel pool
channels = [
    insecure_channel('localhost:50051')
    for _ in range(10)
]

# Round-robin usage
def get_client():
    import random
    channel = random.choice(channels)
    return themis_pb2_grpc.ThemisCoreServiceStub(channel)
```

### Timeout Configuration

```python
# Set timeout for individual call
response = stub.ExecuteQuery(
    request,
    timeout=10.0  # 10 seconds
)

# Set default timeout for stub
stub = themis_pb2_grpc.ThemisCoreServiceStub(
    channel,
    timeout=5.0
)
```

### Compression

```python
# Enable compression
response = stub.ExecuteQuery(
    request,
    compression=grpc.Compression.Gzip
)
```

---

## Code-Generierung

### Proto-Dateien kompilieren

**Python:**
```bash
python -m grpc_tools.protoc -I. --python_out=. --grpc_python_out=. proto/themis_core.proto
```

**Go:**
```bash
protoc --go_out=. --go-grpc_out=. proto/themis_core.proto
```

**C++:**
```bash
protoc --cpp_out=. --grpc_out=. --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` proto/themis_core.proto
```

**Java:**
```bash
protoc --java_out=. --grpc-java_out=. proto/themis_core.proto
```

---

## Siehe auch

- [REST API Specification](REST_API_SPECIFICATION.md)
- [GraphQL API](GRAPHQL_API.md)
- [Protocol Buffer Definitions](../../proto/)
- [Performance Tuning Guide](../performance/tuning.md)
