# Shard RPC Client - Multi-Node Support

## Overview

The `ShardRPCClient` now supports both in-process simulation (for single-node deployments) and real gRPC connections (for multi-node cluster deployments).

## Features

- **Automatic Mode Selection**: Uses in-process simulation for `localhost` endpoints, gRPC for remote endpoints
- **Connection Management**: gRPC channels with keepalive and automatic reconnection
- **Retry Logic**: Exponential backoff with configurable retry attempts
- **Error Handling**: Categorizes errors as retryable (transient) vs non-retryable
- **Timeout Support**: Per-request timeout configuration
- **Health Checks**: Built-in healthcheck endpoint for monitoring

## Usage Examples

### Basic Client Setup

```cpp
#include "sharding/shard_rpc_client.h"

using namespace themis::sharding;

// For single-node (in-process simulation)
ShardRPCClient::Config local_config{
    .endpoint = "localhost:8080",
    .timeout_ms = 5000,
    .max_retries = 3,
    .retry_delay_ms = 100
};
ShardRPCClient local_client(local_config);

// For multi-node (gRPC)
ShardRPCClient::Config remote_config{
    .endpoint = "shard2.example.com:50051",
    .timeout_ms = 5000,
    .max_retries = 3,
    .retry_delay_ms = 100
};
ShardRPCClient remote_client(remote_config);
```

### Distributed Transaction (2PC)

```cpp
// Phase 1: Prepare
nlohmann::json operations = nlohmann::json::array();
operations.push_back({
    {"type", "insert"},
    {"collection", "users"},
    {"key", "user-123"},
    {"data", {{"name", "John Doe"}, {"email", "john@example.com"}}}
});

bool vote = client.prepare("txn-001", operations);
if (vote) {
    // Phase 2: Commit
    bool committed = client.commit("txn-001", 1234567890);
    if (committed) {
        std::cout << "Transaction committed successfully" << std::endl;
    }
} else {
    // Abort
    client.abort("txn-001");
}
```

### Health Check

```cpp
bool healthy = client.ping();
if (healthy) {
    std::cout << "Shard is healthy" << std::endl;
} else {
    std::cout << "Shard is unavailable" << std::endl;
}
```

## Configuration

### Client Configuration

| Parameter | Description | Default |
|-----------|-------------|---------|
| `endpoint` | Shard endpoint address | Required |
| `timeout_ms` | Request timeout in milliseconds | 5000 |
| `max_retries` | Maximum retry attempts | 3 |
| `retry_delay_ms` | Initial delay between retries (exponential backoff) | 100 |

### gRPC Channel Settings

The client automatically configures gRPC channels with:
- **Keepalive**: 30 seconds (ping every 30s)
- **Keepalive Timeout**: 10 seconds
- **Max Reconnect Backoff**: 10 seconds
- **Initial Reconnect Backoff**: 1 second

## Error Handling

### Retryable Errors
- `UNAVAILABLE`: Service temporarily unavailable
- `DEADLINE_EXCEEDED`: Request timeout
- `RESOURCE_EXHAUSTED`: Server overloaded
- `ABORTED`: Operation aborted
- `INTERNAL`: Internal server error

### Non-Retryable Errors
- `INVALID_ARGUMENT`: Invalid request
- `NOT_FOUND`: Resource not found
- `ALREADY_EXISTS`: Duplicate resource
- `PERMISSION_DENIED`: Access denied
- `UNAUTHENTICATED`: Authentication failed
- `FAILED_PRECONDITION`: Operation not allowed
- `UNIMPLEMENTED`: Operation not supported

## Performance

### Latency Targets
- **In-process**: < 1ms
- **gRPC (multi-node)**: < 50ms (target for acceptance criteria)

## References

- [gRPC Documentation](https://grpc.io/docs/)
- [Protocol Buffers](https://developers.google.com/protocol-buffers)
