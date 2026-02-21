# API Module

HTTP API server implementation for ThemisDB.

## Module Purpose

Implements the HTTP API server layer for ThemisDB, exposing RESTful endpoints for document CRUD operations, AQL query execution, graph operations, and authentication using Crow/Beast.

## Subsystem Scope

**In scope:** REST API endpoints, AQL execution endpoint, authentication middleware, TLS/SSL termination, request/response serialization.

**Out of scope:** Query parsing and optimization (handled by query module), authentication logic (handled by auth module).

## Relevant Interfaces

- `http_server.cpp` — main HTTP server
- `api_handler.cpp` — request routing and dispatch
- Middleware stack (auth, rate limiting, TLS)

## Current Delivery Status

**Maturity:** 🟡 Beta — Core REST API operational; GraphQL and WebSocket endpoints planned.

## Components

- HTTP server using Crow/beast
- RESTful API endpoints
- Request/response handling
- API middleware

## Features

- Document CRUD operations
- AQL query execution
- Graph operations
- Authentication and authorization
- TLS/SSL support

## Documentation

For API documentation, see:
- [HTTP Server Documentation](../../docs/src/api/http_server.cpp.md)
- [OpenAPI Specification](../../docs/apis/openapi.md)
- [API Documentation](../../docs/api/)
