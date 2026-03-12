<!-- Status: current | validated: 2026-03-12 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Server Module

All notable changes to the Server module are documented here.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

## [1.5.0] — 2026-03-12
### Added
- HTTP/3 (QUIC) transport support
- MCP (Model Context Protocol) server for AI tool integration
- WasmHandlerRegistry for WebAssembly request handlers (v2.1.0)
- Redis-backed TokenBucketRateLimiter (`Backend::REDIS` via EVALSHA Lua script)
- Per-client rate limiting with `PerClientRateLimiter`
- PostgreSQL wire protocol server for SQL compatibility
- gRPC server with Protobuf serialization
- MQTT broker endpoint for IoT device connectivity
- GraphQL WebSocket handler (graphql-transport-ws protocol) with subscription management
- Force-run endpoint window override for maintenance scheduling

### Changed
- Rate limiter v2 replaces v1 (local fallback preserved)
- Admin PII eviction endpoint wired to cache module (`AdminCachePiiEvictDelete`)

### Fixed
- HMAC validation in distributed cache coordinator

## [1.0.0] — 2024-01-01
### Added
- HTTP/1.1 and HTTP/2 server built on Boost.Beast/Asio
- RESTful routing with 40+ specialized endpoints
- TLS/SSL termination with certificate management
- JWT authentication middleware
- Request/response pipeline with middleware chain
