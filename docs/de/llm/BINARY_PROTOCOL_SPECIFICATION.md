# Binary Protocol Specification (gRPC)

## Overview

ThemisDB provides a high-performance binary protocol using gRPC and Protocol Buffers for LLM operations. This protocol offers 5-10x better performance than HTTP REST API due to binary encoding, multiplexing, and bidirectional streaming.

**Default Port**: `9090`

**Protocol**: HTTP/2 with gRPC

**Serialization**: Protocol Buffers v3

## Service Definition

### Proto File: `llm_service.proto`

```protobuf
syntax = "proto3";

package themis.llm.v1;

option go_package = "github.com/themis/api/llm/v1;llmv1";
option java_package = "com.themis.llm.v1";
option java_outer_classname = "LLMServiceProto";

// LLM Service for high-performance inference and model management
service LLMService {
  // Inference operations
  rpc Inference(InferenceRequest) returns (InferenceResponse);
  rpc StreamInference(InferenceRequest) returns (stream Token);
  rpc RAGInference(RAGRequest) returns (InferenceResponse);
  rpc Embed(EmbedRequest) returns (EmbedResponse);
  
  // Model management
  rpc LoadModel(ModelLoadRequest) returns (ModelLoadResponse);
  rpc UnloadModel(ModelUnloadRequest) returns (ModelUnloadResponse);
  rpc ListModels(ListModelsRequest) returns (ListModelsResponse);
  rpc GetModelInfo(GetModelInfoRequest) returns (ModelInfo);
  rpc IngestModel(stream ModelChunk) returns (ModelIngestResponse);
  
  // LoRA management
  rpc LoadLoRA(LoRALoadRequest) returns (LoRALoadResponse);
  rpc UnloadLoRA(LoRAUnloadRequest) returns (LoRAUnloadResponse);
  rpc ListLoRAs(ListLoRAsRequest) returns (ListLoRAsResponse);
  rpc GetLoRAInfo(GetLoRAInfoRequest) returns (LoRAInfo);
  
  // Statistics and health
  rpc GetStatistics(Empty) returns (Statistics);
  rpc GetCacheStatistics(Empty) returns (CacheStatistics);
  rpc GetHealth(Empty) returns (HealthResponse);
  rpc ClearCache(ClearCacheRequest) returns (ClearCacheResponse);
}

// Messages

message Empty {}

// Inference Request
message InferenceRequest {
  string prompt = 1;
  string model = 2;
  string lora_adapter = 3;
  int32 max_tokens = 4;
  float temperature = 5;
  float top_p = 6;
  int32 top_k = 7;
  repeated string stop_sequences = 8;
  int32 priority = 9;
  map<string, string> metadata = 10;
}

// Inference Response
message InferenceResponse {
  string text = 1;
  int32 tokens_generated = 2;
  int64 inference_time_ms = 3;
  string model_used = 4;
  string lora_used = 5;
  bool cache_hit = 6;
  string finish_reason = 7;
}

// Token (for streaming)
message Token {
  string text = 1;
  int32 index = 2;
  bool is_final = 3;
  TokenMetadata metadata = 4;
}

message TokenMetadata {
  int64 timestamp_ms = 1;
  float probability = 2;
}

// RAG Request
message RAGRequest {
  string query = 1;
  string collection = 2;
  int32 top_k = 3;
  float similarity_threshold = 4;
  string model = 5;
  string lora_adapter = 6;
  int32 max_tokens = 7;
  float temperature = 8;
  ContextAssembly context_assembly = 9;
}

enum ContextAssembly {
  CONCAT = 0;
  SUMMARIZE = 1;
  RANK = 2;
}

// Embed Request
message EmbedRequest {
  string text = 1;
  string model = 2;
  bool normalize = 3;
}

// Embed Response
message EmbedResponse {
  repeated float embedding = 1;
  int32 dimension = 2;
  string model_used = 3;
  int64 inference_time_ms = 4;
}

// Model Management

message ModelLoadRequest {
  string model_id = 1;
  string path = 2;
  ModelOptions options = 3;
  bool pin = 4;
}

message ModelOptions {
  int32 n_gpu_layers = 1;
  int32 n_ctx = 2;
  int32 n_batch = 3;
  int32 n_threads = 4;
  bool use_mmap = 5;
  bool use_mlock = 6;
  map<string, string> custom_options = 7;
}

message ModelLoadResponse {
  string model_id = 1;
  ModelStatus status = 2;
  int64 load_time_ms = 3;
  int64 memory_used_mb = 4;
}

enum ModelStatus {
  UNKNOWN = 0;
  LOADING = 1;
  LOADED = 2;
  UNLOADING = 3;
  UNLOADED = 4;
  ERROR = 5;
}

message ModelUnloadRequest {
  string model_id = 1;
}

message ModelUnloadResponse {
  string model_id = 1;
  ModelStatus status = 2;
  int64 memory_freed_mb = 3;
}

message ListModelsRequest {
  ModelStatus status_filter = 1;
}

message ListModelsResponse {
  repeated ModelInfo models = 1;
}

message GetModelInfoRequest {
  string model_id = 1;
}

message ModelInfo {
  string model_id = 1;
  string path = 2;
  ModelStatus status = 3;
  int64 size_bytes = 4;
  string format = 5;
  string version = 6;
  string architecture = 7;
  int32 n_layers = 8;
  int32 n_heads = 9;
  int32 n_embd = 10;
  int32 n_vocab = 11;
  int32 context_length = 12;
  int64 loaded_timestamp_ms = 13;
  int64 last_used_timestamp_ms = 14;
  int64 usage_count = 15;
  int64 memory_usage_mb = 16;
  int32 gpu_layers = 17;
  bool pinned = 18;
}

// Model Ingestion (streaming)

message ModelChunk {
  oneof data {
    ModelIngestMetadata metadata = 1;
    bytes chunk = 2;
  }
}

message ModelIngestMetadata {
  string model_id = 1;
  string version = 2;
  string description = 3;
  string shard_affinity = 4;
  bool replicate = 5;
  int64 total_size_bytes = 6;
}

message ModelIngestResponse {
  string model_id = 1;
  string version = 2;
  string urn = 3;
  int64 size_bytes = 4;
  string checksum = 5;
  int64 upload_time_ms = 6;
  ReplicationStatus replication_status = 7;
  int32 shards_replicated = 8;
  int32 total_shards = 9;
}

enum ReplicationStatus {
  REPLICATION_PENDING = 0;
  REPLICATION_IN_PROGRESS = 1;
  REPLICATION_COMPLETE = 2;
  REPLICATION_FAILED = 3;
}

// LoRA Management

message LoRALoadRequest {
  string lora_id = 1;
  string base_model = 2;
  string path = 3;
  float scale = 4;
}

message LoRALoadResponse {
  string lora_id = 1;
  string base_model = 2;
  LoRAStatus status = 3;
  int64 load_time_ms = 4;
  int32 slot = 5;
}

enum LoRAStatus {
  LORA_UNKNOWN = 0;
  LORA_LOADING = 1;
  LORA_LOADED = 2;
  LORA_UNLOADING = 3;
  LORA_UNLOADED = 4;
  LORA_ERROR = 5;
}

message LoRAUnloadRequest {
  string lora_id = 1;
  string base_model = 2;
}

message LoRAUnloadResponse {
  string lora_id = 1;
  LoRAStatus status = 2;
  int32 slot_freed = 3;
}

message ListLoRAsRequest {
  string base_model_filter = 1;
  LoRAStatus status_filter = 2;
}

message ListLoRAsResponse {
  repeated LoRAInfo loras = 1;
}

message GetLoRAInfoRequest {
  string lora_id = 1;
  string base_model = 2;
}

message LoRAInfo {
  string lora_id = 1;
  string base_model = 2;
  string path = 3;
  LoRAStatus status = 4;
  int64 size_bytes = 5;
  int32 rank = 6;
  int32 alpha = 7;
  repeated string target_modules = 8;
  int64 loaded_timestamp_ms = 9;
  int64 last_used_timestamp_ms = 10;
  int64 usage_count = 11;
  int32 slot = 12;
}

// Statistics

message Statistics {
  int64 uptime_seconds = 1;
  int64 total_requests = 2;
  int64 successful_requests = 3;
  int64 failed_requests = 4;
  int32 active_requests = 5;
  int32 queued_requests = 6;
  Throughput throughput = 7;
  Latency latency = 8;
  ModelStats model_stats = 9;
  LoRAStats lora_stats = 10;
  WorkerStats worker_stats = 11;
  GPUStats gpu_stats = 12;
}

message Throughput {
  double requests_per_second = 1;
  double tokens_per_second = 2;
}

message Latency {
  int64 p50_ms = 1;
  int64 p95_ms = 2;
  int64 p99_ms = 3;
  double avg_ms = 4;
}

message ModelStats {
  int32 loaded = 1;
  int32 total_available = 2;
  int64 memory_used_mb = 3;
}

message LoRAStats {
  int32 loaded = 1;
  int32 total_available = 2;
  int64 memory_used_mb = 3;
}

message WorkerStats {
  int32 total = 1;
  int32 busy = 2;
  int32 idle = 3;
  double utilization = 4;
}

message GPUStats {
  double utilization = 1;
  int64 memory_used_mb = 2;
  int64 memory_total_mb = 3;
}

// Cache Statistics

message CacheStatistics {
  CacheStats response_cache = 1;
  CacheStats prefix_cache = 2;
  CacheStats model_metadata_cache = 3;
  CacheStats lora_metadata_cache = 4;
  BufferPoolStats kv_cache_buffer_pool = 5;
}

message CacheStats {
  int64 hits = 1;
  int64 misses = 2;
  double hit_rate = 3;
  int64 total_entries = 4;
  int64 memory_used_mb = 5;
  double avg_lookup_time_ms = 6;
}

message BufferPoolStats {
  int32 total_buffers = 1;
  int32 active_buffers = 2;
  int64 buffer_reuse_count = 3;
}

// Health

message HealthResponse {
  HealthStatus status = 1;
  int64 timestamp_ms = 2;
  HealthChecks checks = 3;
}

enum HealthStatus {
  HEALTHY = 0;
  DEGRADED = 1;
  UNHEALTHY = 2;
}

message HealthChecks {
  bool models_loaded = 1;
  bool workers_active = 2;
  bool gpu_available = 3;
  bool queue_ok = 4;
}

// Cache Management

message ClearCacheRequest {
  CacheType cache_type = 1;
}

enum CacheType {
  ALL = 0;
  RESPONSE = 1;
  PREFIX = 2;
}

message ClearCacheResponse {
  CacheType cleared = 1;
  int64 entries_removed = 2;
  int64 memory_freed_mb = 3;
}
```

## Usage Examples

### Go Client

```go
package main

import (
    "context"
    "fmt"
    "io"
    "log"
    
    "google.golang.org/grpc"
    llmv1 "github.com/themis/api/llm/v1"
)

func main() {
    // Connect to server
    conn, err := grpc.Dial("localhost:9090", grpc.WithInsecure())
    if err != nil {
        log.Fatal(err)
    }
    defer conn.Close()
    
    client := llmv1.NewLLMServiceClient(conn)
    
    // Simple inference
    resp, err := client.Inference(context.Background(), &llmv1.InferenceRequest{
        Prompt: "What is ThemisDB?",
        Model: "mistral-7b",
        LoraAdapter: "general-qa",
        MaxTokens: 100,
    })
    if err != nil {
        log.Fatal(err)
    }
    fmt.Printf("Response: %s\n", resp.Text)
    
    // Streaming inference
    stream, err := client.StreamInference(context.Background(), &llmv1.InferenceRequest{
        Prompt: "Write a story...",
        Model: "mistral-7b",
        MaxTokens: 500,
    })
    if err != nil {
        log.Fatal(err)
    }
    
    for {
        token, err := stream.Recv()
        if err == io.EOF {
            break
        }
        if err != nil {
            log.Fatal(err)
        }
        fmt.Print(token.Text)
    }
    
    // RAG inference
    ragResp, err := client.RAGInference(context.Background(), &llmv1.RAGRequest{
        Query: "Contract provisions",
        Collection: "legal_docs",
        TopK: 5,
        Model: "mistral-7b",
        LoraAdapter: "legal-qa",
    })
    if err != nil {
        log.Fatal(err)
    }
    fmt.Printf("RAG Response: %s\n", ragResp.Text)
}
```

### Python Client

```python
import grpc
from themis.llm.v1 import llm_service_pb2, llm_service_pb2_grpc

# Connect to server
channel = grpc.insecure_channel('localhost:9090')
client = llm_service_pb2_grpc.LLMServiceStub(channel)

# Simple inference
response = client.Inference(llm_service_pb2.InferenceRequest(
    prompt="What is ThemisDB?",
    model="mistral-7b",
    lora_adapter="general-qa",
    max_tokens=100
))
print(f"Response: {response.text}")

# Streaming inference
stream = client.StreamInference(llm_service_pb2.InferenceRequest(
    prompt="Write a story...",
    model="mistral-7b",
    max_tokens=500
))

for token in stream:
    print(token.text, end='', flush=True)

# Model ingestion (streaming upload)
def model_chunks(model_path):
    # First send metadata
    yield llm_service_pb2.ModelChunk(
        metadata=llm_service_pb2.ModelIngestMetadata(
            model_id="llama-3-8b",
            version="v1.0",
            replicate=True
        )
    )
    
    # Then send file chunks
    with open(model_path, 'rb') as f:
        while True:
            chunk = f.read(1024 * 1024)  # 1 MB chunks
            if not chunk:
                break
            yield llm_service_pb2.ModelChunk(chunk=chunk)

response = client.IngestModel(model_chunks("/path/to/llama-3-8b.gguf"))
print(f"Uploaded: {response.urn}")
```

### Rust Client

```rust
use tonic::Request;
use themis_llm::llm_service_client::LlmServiceClient;
use themis_llm::{InferenceRequest, RAGRequest};

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    // Connect to server
    let mut client = LlmServiceClient::connect("http://localhost:9090").await?;
    
    // Simple inference
    let request = Request::new(InferenceRequest {
        prompt: "What is ThemisDB?".to_string(),
        model: "mistral-7b".to_string(),
        lora_adapter: "general-qa".to_string(),
        max_tokens: 100,
        ..Default::default()
    });
    
    let response = client.inference(request).await?;
    println!("Response: {}", response.into_inner().text);
    
    // Streaming inference
    let request = Request::new(InferenceRequest {
        prompt: "Write a story...".to_string(),
        model: "mistral-7b".to_string(),
        max_tokens: 500,
        ..Default::default()
    });
    
    let mut stream = client.stream_inference(request).await?.into_inner();
    
    while let Some(token) = stream.message().await? {
        print!("{}", token.text);
    }
    
    Ok(())
}
```

## Performance Characteristics

### Throughput Comparison

| Protocol | Requests/sec | Latency (p50) | Latency (p99) |
|----------|--------------|---------------|---------------|
| HTTP/REST | 24 | 45ms | 180ms |
| gRPC Binary | 128 | 18ms | 65ms |

**Improvement**: 5.3x throughput, 2.5x lower latency

### Binary Size Comparison

| Format | Request Size | Response Size |
|--------|--------------|---------------|
| JSON (HTTP) | 245 bytes | 1.2 KB |
| Protobuf (gRPC) | 87 bytes | 420 bytes |

**Improvement**: 2.8x smaller requests, 2.9x smaller responses

## Best Practices

1. **Use connection pooling** - Reuse gRPC channels across requests
2. **Enable compression** - Use gzip for large payloads
3. **Set appropriate timeouts** - Prevent hanging connections
4. **Use streaming for large data** - Model ingestion, long responses
5. **Implement retry logic** - Handle transient failures
6. **Monitor metrics** - Track latency, error rates
7. **Use metadata for tracing** - Add request IDs, trace contexts

## Security

### TLS/SSL

```go
// Client with TLS
creds, err := credentials.NewClientTLSFromFile("server.crt", "")
conn, err := grpc.Dial("localhost:9090", grpc.WithTransportCredentials(creds))
```

### Authentication

**All gRPC requests require Bearer Token authentication via metadata.**

**Go Client**:
```go
// Bearer Token authentication
type tokenAuth struct {
    token string
}

func (t tokenAuth) GetRequestMetadata(ctx context.Context, uri ...string) (map[string]string, error) {
    return map[string]string{
        "authorization": "Bearer " + t.token,
    }, nil
}

func (t tokenAuth) RequireTransportSecurity() bool {
    return true
}

// Create connection with token
conn, err := grpc.Dial("localhost:9090",
    grpc.WithTransportCredentials(credentials.NewTLS(&tls.Config{})),
    grpc.WithPerRPCCredentials(tokenAuth{token: "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9..."}))
```

**Python Client**:
```python
import grpc
from grpc import metadata_call_credentials

class BearerTokenAuth(grpc.AuthMetadataPlugin):
    def __init__(self, token):
        self.token = token
    
    def __call__(self, context, callback):
        callback((('authorization', f'Bearer {self.token}'),), None)

# Create credentials
token = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9..."
auth_creds = metadata_call_credentials(BearerTokenAuth(token))
ssl_creds = grpc.ssl_channel_credentials()
composite_creds = grpc.composite_channel_credentials(ssl_creds, auth_creds)

# Connect with authentication
channel = grpc.secure_channel('localhost:9090', composite_creds)
client = llm_service_pb2_grpc.LLMServiceStub(channel)
```

**Rust Client**:
```rust
use tonic::metadata::MetadataValue;
use tonic::Request;

// Add token to request
let token = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...";
let mut request = Request::new(inference_request);
request.metadata_mut().insert(
    "authorization",
    MetadataValue::from_str(&format!("Bearer {}", token)).unwrap()
);
```

**Token Format**: JWT (JSON Web Token)

**Unauthorized Response**: gRPC status code `UNAUTHENTICATED` (16)
```

## Error Handling

gRPC uses status codes (similar to HTTP):

- `OK` (0): Success
- `CANCELLED` (1): Request cancelled
- `UNKNOWN` (2): Unknown error
- `INVALID_ARGUMENT` (3): Invalid parameters
- `NOT_FOUND` (5): Model/LoRA not found
- `RESOURCE_EXHAUSTED` (8): Queue full, insufficient memory
- `UNIMPLEMENTED` (12): Feature not implemented
- `INTERNAL` (13): Internal server error
- `UNAVAILABLE` (14): Service unavailable

```go
// Error handling
resp, err := client.Inference(ctx, req)
if err != nil {
    if st, ok := status.FromError(err); ok {
        switch st.Code() {
        case codes.NotFound:
            fmt.Println("Model not found")
        case codes.ResourceExhausted:
            fmt.Println("Queue full, retry later")
        default:
            fmt.Printf("Error: %v\n", st.Message())
        }
    }
}
```

## Advantages Over HTTP REST

1. **Performance**: 5-10x faster due to binary encoding
2. **Streaming**: Bidirectional, multiplexed streams
3. **Type Safety**: Strong typing with Protocol Buffers
4. **Code Generation**: Auto-generated clients for many languages
5. **HTTP/2**: Multiplexing, header compression, server push
6. **Smaller Payloads**: 3x smaller than JSON
7. **Better Error Handling**: Rich status codes and details
8. **Built-in Load Balancing**: Client-side load balancing support
