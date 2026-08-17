# LLM Streaming Module — Architecture

<!-- Status: PRODUCTION_CANDIDATE | validated: 2026-08-10 -->

## Overview

The LLM streaming module provides real-time streaming infrastructure for large language model responses within ThemisDB, enabling efficient token-level streaming to clients with flow-control and error recovery capabilities.

## Design Principles

1. **Token-Level Streaming:** Tokens sent to client as soon as available (no buffering)
2. **Backpressure Awareness:** Respects client receive window; buffers on congestion
3. **Connection Resilience:** Graceful handling of client disconnections and network failures
4. **Cancellation Support:** Clients can cancel in-progress streams cleanly
5. **Observable:** All streaming events logged with correlation IDs

## Architecture Diagram

```
┌─────────────────────────────────────────────────────────────┐
│  LLM Inference Engine                                       │
│  • Produces tokens as they are generated                    │
└──────────────────────┬──────────────────────────────────────┘
                       │
                       ▼
┌─────────────────────────────────────────────────────────────┐
│  StreamDispatcher (Request Routing)                         │
│  • Route LLM requests to streaming implementation           │
│  • Manage stream lifecycle (open, active, close)            │
│  • Track active streams and concurrent connections         │
└──────────────────────┬──────────────────────────────────────┘
                       │
                       ▼
┌─────────────────────────────────────────────────────────────┐
│  TokenBuffer (Aggregation & Batching)                       │
│  • Buffer tokens for network efficiency                     │
│  • Batching based on size/time threshold                    │
│  • Preserve token order and metadata                        │
└──────────────────────┬──────────────────────────────────────┘
                       │
                       ▼
┌─────────────────────────────────────────────────────────────┐
│  BackpressureController (Flow Control)                      │
│  • Monitor client receive window                            │
│  • Apply backpressure when buffer full                      │
│  • Implement exponential backoff on congestion              │
└──────────────────────┬──────────────────────────────────────┘
                       │
                       ▼
┌─────────────────────────────────────────────────────────────┐
│  StreamingServer (Protocol Handler)                         │
│  • gRPC streaming endpoint                                  │
│  • HTTP Server-Sent Events (SSE)                            │
│  • Connection management & lifecycle                        │
│  • Timeout enforcement                                      │
└──────────────────────┬──────────────────────────────────────┘
                       │
                       ▼
                   Network
                       │
                       ▼
                  Streaming Client
```

## Core Components

### StreamingServer

**Purpose:** Protocol handler for streaming responses over gRPC or HTTP.

**Responsibilities:**
- Accept streaming requests (gRPC or HTTP SSE)
- Manage connection lifecycle (open, active, close)
- Enforce per-stream timeouts
- Send tokens to client with metadata
- Handle client disconnection gracefully

**Public API:**
```cpp
class StreamingServer {
  Result<> startStream(const LLMRequest& req, StreamWriter* writer);
  Result<> sendToken(const Token& token);
  void cancelStream(const StreamId& id);
};
```

### StreamDispatcher

**Purpose:** Route LLM requests to streaming implementation.

**Responsibilities:**
- Create new stream for each LLM request
- Coordinate with LLM inference engine
- Manage stream state transitions
- Track active streams
- Clean up closed streams

**Key Contracts:**
- `dispatch(request) → StreamId` — Create new stream
- `getStream(id) → Stream*` — Lookup active stream
- `closeStream(id)` — Terminate stream

### TokenBuffer

**Purpose:** Aggregate tokens for efficient network transmission.

**Approach:**
- Buffer tokens until size threshold or time deadline reached
- Batch multiple tokens into single network message
- Preserve token order and metadata
- Configurable batching heuristics

**Configuration:**
- Batch size threshold (default: 10 tokens)
- Max latency threshold (default: 100 ms)
- Buffer capacity (default: 1000 tokens)

**Performance:**
- Reduces network roundtrips by 10-100x
- Maintains latency < 100 ms for small batches

### BackpressureController

**Purpose:** Implement flow control to respect client receive window.

**Approach:**
- Monitor client acknowledgments and window size
- Pause token sending when buffer full
- Implement exponential backoff during congestion
- Resume when client acknowledges

**Flow Control Model:**
```
Token Available
  │
  ├─► Check client receive window
  │
  ├─► If space available:
  │   └─► Send token immediately
  │
  └─► If buffer full:
      ├─► Add to backpressure queue
      ├─► Notify LLM (slow producer)
      └─► Wait for client acknowledgment
```

## Data Flow

### Token Emission Pipeline

```
LLM Inference Engine
  │ produces token
  ▼
StreamDispatcher.onToken(token)
  │ get active stream
  ▼
TokenBuffer.addToken(token)
  │ check batching criteria
  ├─► If size threshold reached:
  │   └─► flush batch
  ├─► If time threshold reached:
  │   └─► flush batch
  └─► If capacity exceeded:
      └─► apply backpressure
          │
          ▼
      BackpressureController
        │ wait for client window
        ▼
      StreamingServer.sendBatch(tokens)
        │ send to client
        ▼
      Network → Client
```

## Concurrency Model

### Thread Safety

1. **Per-Stream State:** Protected by stream-specific mutex
   - Token buffer state
   - Backpressure state
   - Stream lifecycle flags

2. **Global Stream Registry:** Protected by read-write lock
   - Enables fast lookup of active streams
   - Minimal contention for stream creation/deletion

3. **Token Emission:** Lock-free where possible
   - Atomic token counter
   - Compare-and-swap for stream state transitions

### Synchronization Primitives

- `std::mutex` for stream-specific critical sections
- `std::shared_mutex` for stream registry
- `std::condition_variable` for backpressure signaling
- `std::atomic<>` for stream counters

## Performance Characteristics

### Target Latencies (P99)

- **Token Enqueue:** < 1 ms
- **Batch Formation:** < 100 ms (batching deadline)
- **Network Send:** < 50 ms
- **End-to-End (token → client):** < 200 ms
- **Cancellation Propagation:** < 100 ms

### Throughput

- **Token Throughput:** > 100 tokens/sec per stream
- **Concurrent Streams:** ≥ 100 active streams
- **Aggregate Throughput:** 10k+ tokens/sec

### Resource Consumption

- **Per-Stream Memory:** ~10 MB (including buffers)
- **Token Buffer Overhead:** ~100 bytes per token
- **Total Memory (100 streams):** ~1 GB

## Error Handling

### Graceful Degradation

1. **Client Disconnection** → Detect via write failure; clean up stream
2. **Network Timeout** → Retry with exponential backoff
3. **Backpressure Timeout** → Close stream with error
4. **Buffer Overflow** → Return backpressure error to LLM producer

### Error Codes (E7300–E7399)

- E7300: Stream not found
- E7301: Cancellation requested
- E7302: Backpressure buffer exceeded
- E7303: Token send timeout
- E7304: Invalid token sequence

## Integration Points

### LLM Inference Engine

Streaming receives tokens from LLM inference as they are produced:
- Token callback: `onToken(token, finish_reason)`
- LLM can check backpressure: `isBackpressured() → bool`

### Client Protocols

- gRPC: ServerWriter<Token> streaming
- HTTP: Server-Sent Events (SSE)

## See Also

- [`ROADMAP.md`](ROADMAP.md) — Implementation phases and deliverables
- [`FUTURE_ENHANCEMENTS.md`](FUTURE_ENHANCEMENTS.md) — Planned features
- [`../../include/llm_streaming/streaming_server.h`](../../include/llm_streaming/streaming_server.h) — Public API
