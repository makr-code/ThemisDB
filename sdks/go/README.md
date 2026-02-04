# ThemisDB Go SDK

## Overview

Official Go client library for ThemisDB. This SDK provides an idiomatic, efficient Go interface for interacting with ThemisDB's REST API.

## Status

🚧 **Under Development** - This SDK is currently in active development. Basic structure and placeholder functionality are in place.

## Features (Planned)

- ✅ Bearer Token (JWT) authentication
- ✅ Type-safe CRUD operations
- ✅ AQL (Advanced Query Language) query execution
- ✅ LLM inference and RAG operations
- ✅ Context-aware operations
- ✅ Real-time token streaming
- ✅ Model and LoRA management
- ✅ Comprehensive error handling
- ✅ Connection pooling
- ✅ Go 1.20+ support

## Installation

Once published, install the SDK using `go get`:

```bash
go get github.com/makr-code/ThemisDB/sdks/go/themisclient
```

## Quick Start

```go
package main

import (
    "context"
    "fmt"
    "log"
    
    themisclient "github.com/makr-code/ThemisDB/sdks/go/pkg/themisclient"
)

func main() {
    // Initialize client with authentication
    client, err := themisclient.NewClient(
        "http://localhost:8080",
        themisclient.WithBearerToken("your-jwt-token"),
    )
    if err != nil {
        log.Fatal(err)
    }
    defer client.Close()

    ctx := context.Background()

    // Execute AQL query
    result, err := client.Query(ctx, "FOR doc IN myCollection RETURN doc")
    if err != nil {
        log.Fatal(err)
    }
    fmt.Println(result)

    // LLM inference
    response, err := client.LLM().Infer(ctx, &themisclient.InferRequest{
        Prompt: "What is ThemisDB?",
        Model:  "mistral-7b",
    })
    if err != nil {
        log.Fatal(err)
    }
    fmt.Printf("Response: %s\n", response.Text)
}
```

## API Reference

### Client Initialization

```go
import themisclient "github.com/makr-code/ThemisDB/sdks/go/pkg/themisclient"

// Basic initialization
client, err := themisclient.NewClient(
    "http://localhost:8080",
    themisclient.WithBearerToken("token"),
    themisclient.WithTimeout(30 * time.Second),
)

// With custom HTTP client
httpClient := &http.Client{
    Timeout: 30 * time.Second,
}
client, err := themisclient.NewClient(
    "http://localhost:8080",
    themisclient.WithHTTPClient(httpClient),
)
```

### Data Operations

```go
ctx := context.Background()

// Query execution
result, err := client.Query(ctx, 
    "FOR doc IN myCollection FILTER doc.age > @age RETURN doc",
    themisclient.BindVars{"age": 25},
)

// Collection operations
collections, err := client.Collections().List(ctx)
err = client.Collections().Create(ctx, "newCollection", nil)
err = client.Collections().Drop(ctx, "oldCollection")

// Document operations
doc, err := client.Collection("users").Document("user123").Get(ctx)
err = client.Collection("users").Document("user123").Update(ctx, updatedDoc)
err = client.Collection("users").Document("user123").Delete(ctx)
```

### LLM Operations

```go
ctx := context.Background()

// Inference
response, err := client.LLM().Infer(ctx, &themisclient.InferRequest{
    Prompt:    "Explain quantum computing",
    Model:     "mistral-7b",
    MaxTokens: 100,
})

// Streaming inference
stream, err := client.LLM().InferStream(ctx, &themisclient.InferRequest{
    Prompt: "Tell me a story",
})
if err != nil {
    log.Fatal(err)
}

for chunk := range stream {
    if chunk.Err != nil {
        log.Fatal(chunk.Err)
    }
    fmt.Print(chunk.Token)
}

// RAG (Retrieval-Augmented Generation)
ragResp, err := client.LLM().RAG(ctx, &themisclient.RAGRequest{
    Query:      "What are the key features?",
    Collection: "documentation",
})

// Embeddings
embeddings, err := client.LLM().Embed(ctx, "Sample text for embedding", "")

// Model management
models, err := client.LLM().ListModels(ctx)
err = client.LLM().LoadModel(ctx, "mistral-7b")
err = client.LLM().UnloadModel(ctx, "old-model")
```

### Administrative Functions

```go
ctx := context.Background()

// Health check
health, err := client.Admin().Health(ctx)
fmt.Printf("Status: %s\n", health.Status)

// Statistics
stats, err := client.Admin().Stats(ctx)
fmt.Printf("Documents: %d\n", stats.Documents)

// Cache management
err = client.Admin().ClearCache(ctx)
cacheStats, err := client.Admin().CacheStats(ctx)
```

### Error Handling

```go
import "errors"

result, err := client.Query(ctx, "INVALID AQL")
if err != nil {
    var apiErr *themisclient.APIError
    if errors.As(err, &apiErr) {
        fmt.Printf("API error %d: %s\n", apiErr.Code, apiErr.Message)
    }
    
    var netErr *themisclient.NetworkError
    if errors.As(err, &netErr) {
        fmt.Printf("Network error: %s\n", netErr.Error())
    }
}
```

### Context and Cancellation

```go
// With timeout
ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
defer cancel()

result, err := client.Query(ctx, "FOR doc IN myCollection RETURN doc")

// With cancellation
ctx, cancel := context.WithCancel(context.Background())
go func() {
    time.Sleep(2 * time.Second)
    cancel()
}()

result, err := client.Query(ctx, "LONG RUNNING QUERY")
```

## Development

### Prerequisites

- Go 1.20 or later

### Setup

```bash
# Clone the repository
git clone https://github.com/makr-code/ThemisDB.git
cd ThemisDB/sdks/go

# Download dependencies
go mod download

# Run tests
go test ./...

# Run with coverage
go test -cover ./...

# Run examples
go run examples/basic.go
```

### Project Structure

```
go/
├── pkg/
│   └── themisclient/
│       ├── client.go          # Main client implementation
│       ├── query.go           # Query operations
│       ├── llm.go             # LLM operations
│       ├── admin.go           # Admin operations
│       ├── models.go          # Data models
│       ├── errors.go          # Error types
│       └── client_test.go     # Tests
├── cmd/
│   └── example/               # CLI examples
├── examples/
│   └── basic.go               # Basic usage example
├── go.mod                     # Module definition
├── go.sum                     # Dependencies checksums
└── README.md                  # This file
```

## Testing

Run the test suite:

```bash
go test ./...
```

Run with coverage:

```bash
go test -cover ./...
```

Run with race detector:

```bash
go test -race ./...
```

Run benchmarks:

```bash
go test -bench=. ./...
```

## Examples

See the [examples](./examples) directory for usage examples:

- [basic.go](./examples/basic.go) - Basic operations
- [streaming.go](./examples/streaming.go) - Token streaming
- [rag.go](./examples/rag.go) - RAG operations

## Contributing

Contributions are welcome! Please see the [CONTRIBUTING.md](../../CONTRIBUTING.md) guide for details.

## License

Apache 2.0 - See [LICENSE](../../LICENSE) for details.

## Support

- Documentation: [docs.themisdb.org](https://docs.themisdb.org)
- GitHub Issues: [github.com/makr-code/ThemisDB/issues](https://github.com/makr-code/ThemisDB/issues)
- Community: [ThemisDB Discussions](https://github.com/makr-code/ThemisDB/discussions)
