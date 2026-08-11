# ThemisDB LLM Streaming Module

<!-- Status: PRODUCTION_CANDIDATE | Phase 1-3 complete | validated: 2026-08-10 -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Module Purpose

The LLM Streaming module provides real-time streaming infrastructure and client-side integration for large language model responses within ThemisDB, including token-level streaming, cancellation support, and flow-control surfaces.

## Relevant Interfaces

| Interface / File | Role |
|---|---|
| streaming_server.cpp | gRPC/HTTP streaming endpoint and protocol handler |
| stream_dispatcher.cpp | request-to-stream dispatch and lifecycle management |
| token_buffer.cpp | buffered token aggregation and batching |
| cancellation_handler.cpp | cancellation signal handling and propagation |
| backpressure_controller.cpp | client backpressure and flow control |
| stream_telemetry.cpp | streaming-specific observability and metrics |
| error_recovery.cpp | graceful error handling and reconnection paths |

## Scope

In scope:
- streaming protocol implementation and client integration paths
- token-level buffering and flow-control surfaces
- cancellation and error recovery behaviors
- streaming-specific observability and SLO monitoring

Out of scope:
- core LLM inference engine internals
- non-streaming protocol adapter logic
- business-domain LLM orchestration outside streaming runtime boundaries

## Runtime Behavior and Limits

- behavior depends on configured backend, buffer sizes, and flow-control policies
- streaming operations return structured failure paths for degraded conditions
- cancellation is idempotent and best-effort under network partitions

## Sourcecode Verification (Module: llm_streaming/readme)

- Verified files:
  - src/llm_streaming/streaming_server.cpp
  - src/llm_streaming/stream_dispatcher.cpp
  - src/llm_streaming/token_buffer.cpp
  - src/llm_streaming/cancellation_handler.cpp
  - src/llm_streaming/backpressure_controller.cpp
  - src/llm_streaming/stream_telemetry.cpp
  - src/llm_streaming/error_recovery.cpp
- Verified behavior surfaces:
  - protocol-level streaming and client integration paths
  - token buffering and flow-control coordination
  - cancellation and error recovery paths
- Note:
  - forward planning is tracked in ROADMAP.md and FUTURE_ENHANCEMENTS.md
  - historical completion remains in CHANGELOG.md
