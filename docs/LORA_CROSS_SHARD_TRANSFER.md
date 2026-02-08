# LoRA Cross-Shard Transfer Implementation

## Overview

This implementation enables secure, efficient transfer of LoRA adapters between shards in ThemisDB, reusing the existing WAL shipper transport infrastructure.

## Architecture

### Components

1. **SecureTransportClient** (`include/sharding/secure_transport_client.h`)
   - Shared transport abstraction for secure data transfer
   - Reuses mTLS, compression, and retry logic
   - Can be used by both WALShipper and AdapterSyncManager

2. **AdapterSyncManager** (`src/llm/lora_framework/adapter_sync_manager.cpp`)
   - Orchestrates adapter synchronization across shards
   - Uses SecureTransportClient for network transfer
   - Implements retry logic and replication factor
   - Tracks sync status and metrics

3. **LoRAApiHandler** (`src/server/lora_api_handler.cpp`)
   - Receives adapter transfers via REST endpoint
   - Validates integrity checks (checksums, signatures)
   - Stores adapters via LoRAStorageService

### Data Flow

```
Source Shard                                     Target Shard
│                                                     │
├─ AdapterSyncManager.syncToPeer()                  │
│  ├─ Load adapter from storage                      │
│  ├─ Verify local consistency                       │
│  ├─ Get peer endpoint from topology                │
│  └─ Transfer via SecureTransportClient             │
│     ├─ Serialize metadata + weights                │
│     ├─ Add integrity checks                        │
│     ├─ Compress payload (Zstd)                     │
│     ├─ Transfer via mTLS POST                      │
│     └─ Retry with exponential backoff              │
│                                                     │
│                                    ┌───────────────┤
│                                    │ POST /api/v1/lora/receive
│                                    ├─ Validate checksums/signatures
│                                    ├─ Decompress if needed
│                                    ├─ Store via LoRAStorageService
│                                    └─ Return receipt
```

## Features

### Transport Layer (SecureTransportClient)

- **mTLS Authentication**: Mutual TLS for secure shard-to-shard communication
- **Compression**: Zstd compression with configurable threshold (default: 1KB)
- **Retry Logic**: Exponential backoff with configurable max retries
- **Integrity Checks**: Optional checksums (SHA-256) and digital signatures

### Sync Manager Features

- **Automatic Replication**: Configurable replication factor
- **Peer Discovery**: Automatic detection of healthy shards via ShardTopology
- **Consistency Checking**: Pre and post-transfer validation
- **Metrics**: Prometheus integration for monitoring
- **Retry Logic**: Exponential backoff for failed transfers

### Security Features

- **Checksum Validation**: SHA-256 checksums for data integrity
- **Digital Signatures**: Optional signatures for authenticity
- **mTLS**: Mutual TLS for encrypted, authenticated communication
- **Certificate Validation**: Peer verification and hostname validation

## Configuration

### AdapterSyncManager Configuration

```cpp
AdapterSyncManager::Config config;
config.sync_interval = std::chrono::seconds(300);  // 5 minutes
config.replication_factor = 3;                      // 3 replicas
config.enable_auto_sync = true;                     // Auto-sync enabled
config.max_retries = 5;                             // Max retry attempts
config.cert_path = "/path/to/cert.pem";            // mTLS certificate
config.key_path = "/path/to/key.pem";              // mTLS key
config.ca_cert_path = "/path/to/ca.pem";           // CA certificate
config.enable_compression = true;                   // Enable compression
config.compression_level = 3;                       // Zstd level (1-22)
```

### SecureTransportClient Configuration

```cpp
SecureTransportClient::Config config;
config.compression = SecureTransportClient::Config::CompressionType::Zstd;
config.compression_level = 3;
config.compression_threshold = 1024;               // Compress if > 1KB
config.max_retries = 5;
config.retry_delay_ms = 1000;                     // Initial delay: 1s
config.max_retry_delay_ms = 60000;                // Max delay: 60s
config.cert_path = "/path/to/cert.pem";
config.key_path = "/path/to/key.pem";
config.ca_cert_path = "/path/to/ca.pem";
```

## API Endpoints

### POST /api/v1/lora/receive

Receives LoRA adapter transfers from peer shards.

**Request Body (JSON)**:
```json
{
  "metadata": {
    "adapter_id": "sentiment-analyzer-v2",
    "version": "2.0",
    "base_model": "llama-2-7b",
    "description": "Fine-tuned for sentiment analysis",
    "training_samples": 10000,
    "validation_accuracy": 0.95,
    "checksum": "sha256:abc123...",
    "signature": "ed25519:def456...",
    "format": "safetensors",
    "hyperparameters": { ... }
  },
  "data": <binary>,
  "compression": "zstd",
  "original_size": 104857600,
  "checksum": "sha256:abc123...",
  "signature": "ed25519:def456..."
}
```

**Response (201 Created)**:
```json
{
  "adapter_id": "sentiment-analyzer-v2",
  "version": "2.0",
  "status": "received",
  "bytes_received": 104857600,
  "compressed": true,
  "timestamp": 1234567890123
}
```

## Testing

### Unit Tests

Located in `tests/test_secure_transport_client.cpp`:

```bash
# Run transport client tests
./build/tests/test_secure_transport_client
```

### Integration Tests

Integration tests require:
- Test certificates (set via environment variables)
- Test endpoint running

```bash
# Set up test environment
export TEST_CERT_PATH=/path/to/test/cert.pem
export TEST_KEY_PATH=/path/to/test/key.pem
export TEST_CA_PATH=/path/to/test/ca.pem
export TEST_ENDPOINT=https://test-shard:8080

# Run integration tests
./build/tests/test_secure_transport_client --gtest_filter=*Integration*
```

### Adapter Sync Tests

Located in `tests/test_adapter_sync.cpp`:

```bash
# Run adapter sync tests
./build/tests/test_adapter_sync
```

## Monitoring

### Prometheus Metrics

AdapterSyncManager exports the following metrics:

- `themis_lora_sync_syncs_total` - Total sync operations
- `themis_lora_sync_syncs_success_total` - Successful syncs
- `themis_lora_sync_syncs_failures_total` - Failed syncs
- `themis_lora_sync_bytes_transferred_total` - Bytes transferred
- `themis_lora_sync_sync_duration_seconds` - Sync duration histogram
- `themis_lora_sync_adapters_tracked` - Number of tracked adapters
- `themis_lora_sync_replication_lag_seconds` - Replication lag

### Logging

Logs are written via spdlog:

```cpp
spdlog::info("Successfully synced adapter {} to peer {} "
           "(sent {} bytes, compressed to {}, ratio: {:.2f}x, retries: {})",
           adapter_id, peer_shard_id,
           result.bytes_sent, result.bytes_compressed,
           result.compression_ratio, result.retry_count);
```

## Performance Considerations

### Compression

- **Zstd** provides 3-10x compression for adapter weights
- Compression is only applied for payloads > 1KB (configurable)
- Compression level 3 provides good balance of speed and ratio

### Network Transfer

- Uses connection pooling for efficiency
- Batch transfers when possible
- Exponential backoff prevents network congestion

### Memory Usage

- Adapters are loaded on-demand
- Compression reduces network bandwidth
- Streaming transfer for large adapters (future enhancement)

## Future Enhancements

1. **Streaming Transfer**: Support for large adapters without loading entire payload
2. **LZ4 Compression**: Fast compression option for CPU-constrained scenarios
3. **Delta Sync**: Transfer only changed weights for adapter updates
4. **Bandwidth Throttling**: Rate limiting for network transfer
5. **Multi-target Transfer**: Parallel transfer to multiple shards
6. **Resume Capability**: Resume interrupted transfers

## Related Files

- `include/sharding/secure_transport_client.h`
- `src/sharding/secure_transport_client.cpp`
- `include/llm/lora_framework/adapter_sync_manager.h`
- `src/llm/lora_framework/adapter_sync_manager.cpp`
- `include/server/lora_api_handler.h`
- `src/server/lora_api_handler.cpp`
- `tests/test_secure_transport_client.cpp`
- `tests/test_adapter_sync.cpp`
