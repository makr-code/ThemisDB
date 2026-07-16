# Binary Protocol Extensions for AutoBuffer Operations

**Stand:** 6. April 2026  
**Version:** v1.3.0  
**Kategorie:** 🧩 Architecture

---

## 📑 Table of Contents

- [Overview](#overview)
- [Protocol Specification](#protocol-specification)
- [Operation Details](#operation-details)

## Overview

This document describes the binary protocol extensions for ThemisDB's AutoBuffer operations. These extensions enable efficient, low-overhead buffered operations for time series data, vector indices, and property graphs using the MessagePack serialization format.

## Protocol Specification

### Message Format

All messages follow this format:

```
[1 byte: opcode] [4 bytes: payload length (big-endian)] [N bytes: payload (MessagePack)]
```

### Response Format

All responses follow this format:

```
[1 byte: status] [4 bytes: payload length (big-endian)] [N bytes: payload (MessagePack)]
```

### Opcodes

| Opcode | Name | Description |
|--------|------|-------------|
| `0x70` | TS_PUT_BUFFERED | Buffered time series data point insert |
| `0x71` | TS_PUT_BUFFERED_BATCH | Buffered batch of time series points |
| `0x72` | VECTOR_ADD_BUFFERED | Buffered vector add operation |
| `0x73` | VECTOR_UPDATE_BUFFERED | Buffered vector update operation |
| `0x74` | VECTOR_REMOVE_BUFFERED | Buffered vector remove operation |
| `0x75` | GRAPH_NODE_BUFFERED | Buffered graph node add |
| `0x76` | GRAPH_EDGE_BUFFERED | Buffered graph edge add |
| `0x77` | BUFFER_STATS | Get buffer statistics |
| `0x78` | BUFFER_FLUSH | Manual buffer flush |

### Status Codes

| Code | Name | Description |
|------|------|-------------|
| `0x00` | STATUS_SUCCESS | Operation succeeded |
| `0x01` | STATUS_INVALID_OPCODE | Unknown opcode |
| `0x02` | STATUS_MALFORMED_PAYLOAD | Invalid MessagePack payload |
| `0x03` | STATUS_PROCESSING_ERROR | Error processing request |
| `0x04` | STATUS_BUFFER_OVERFLOW | Buffer capacity exceeded |

## Operation Details

### 0x70: TS_PUT_BUFFERED

Buffer a single time series data point.

**Request Payload (MessagePack map):**
```
{
    "metric": string,      // Metric name (e.g., "cpu.usage")
    "entity": string,      // Entity identifier (e.g., "server01")
    "timestamp": int64,    // Unix timestamp in seconds
    "value": float64       // Data point value
}
```

**Response Payload:**
Empty on success.

**Example (Python):**
```python
import struct
import msgpack

payload = msgpack.packb({
    'metric': 'cpu.usage',
    'entity': 'server01',
    'timestamp': 1700000000,
    'value': 75.5
})

message = struct.pack('!BI', 0x70, len(payload)) + payload
socket.send(message)

# Read response
response = socket.recv(1024)
status = struct.unpack('!B', response[0:1])[0]
```

### 0x77: BUFFER_STATS

Get buffer statistics for all AutoBuffer components.

**Request Payload:**
Empty.

**Response Payload (MessagePack map):**
```
{
    "ts_buffer": {
        "enabled": bool,
        "points_buffered": int,      // Total points buffered
        "points_flushed": int,       // Total points flushed
        "current_buffer_size": int   // Current number of buffered points
    },
    "vector_buffer": {
        "enabled": bool,
        "vectors_buffered": int,
        "vectors_flushed": int,
        "current_buffer_size": int
    },
    "graph_buffer": {
        "enabled": bool,
        "operations_buffered": int,
        "operations_flushed": int,
        "current_buffer_size": int
    }
}
```

## Performance Characteristics

### Protocol Efficiency

Compared to JSON over HTTP:
- **Serialization size:** ~40% smaller (MessagePack vs JSON)
- **CPU overhead:** ~30% lower (no text parsing)
- **Latency:** ~20% lower (less data transfer)

### Throughput

With binary protocol + AutoBuffer:
- **Time Series:** 50,000+ points/sec per connection
- **Vector Operations:** 10,000+ vectors/sec per connection
- **Graph Operations:** 5,000+ operations/sec per connection

## Client Implementation Guide

### Python Client Example

```python
import socket
import struct
import msgpack

class ThemisDBBufferedClient:
    def __init__(self, host='localhost', port=9090):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.connect((host, port))
    
    def _send_message(self, opcode, payload_dict):
        """Send a message and return the response."""
        payload = msgpack.packb(payload_dict) if payload_dict else b''
        message = struct.pack('!BI', opcode, len(payload)) + payload
        self.sock.send(message)
        
        # Read response
        response = self.sock.recv(4096)
        status = response[0]
        payload_len = struct.unpack('!I', response[1:5])[0]
        
        if payload_len > 0:
            response_payload = msgpack.unpackb(response[5:5+payload_len])
        else:
            response_payload = None
        
        return status, response_payload
    
    def put_ts_buffered(self, metric, entity, timestamp, value):
        """Buffer a time series data point."""
        status, _ = self._send_message(0x70, {
            'metric': metric,
            'entity': entity,
            'timestamp': timestamp,
            'value': value
        })
        return status == 0x00
    
    def get_buffer_stats(self):
        """Get buffer statistics."""
        status, stats = self._send_message(0x77, None)
        return stats if status == 0x00 else None
    
    def flush_buffers(self, buffer='all'):
        """Flush buffers manually."""
        status, response = self._send_message(0x78, {'buffer': buffer})
        return response.get('flushed_count', 0) if status == 0x00 else 0
    
    def close(self):
        """Close the connection."""
        self.sock.close()

# Usage example
client = ThemisDBBufferedClient()
client.put_ts_buffered('cpu.usage', 'server01', 1700000000, 75.5)
stats = client.get_buffer_stats()
print(f"TS buffer size: {stats['ts_buffer']['current_buffer_size']}")
client.close()
```

## See Also

- [TSAutoBuffer Documentation](../timeseries/AUTO_BUFFER.md)
- [VectorAutoBuffer Documentation](../search/VECTOR_AUTO_BUFFER.md)
- [HTTP REST API Documentation](../apis/BUFFER_API.md)
- [Client Implementation Roadmap](../roadmap/CLIENT_IMPLEMENTATION_ROADMAP.md)
