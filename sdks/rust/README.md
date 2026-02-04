# ThemisDB Rust SDK

## Overview

Official Rust client library for ThemisDB. This SDK provides a safe, performant, and idiomatic Rust interface for interacting with ThemisDB's REST API.

## Status

🚧 **Under Development** - This SDK is currently in active development. Basic structure and placeholder functionality are in place.

## Features (Planned)

- ✅ Bearer Token (JWT) authentication
- ✅ Type-safe CRUD operations
- ✅ AQL (Advanced Query Language) query execution
- ✅ LLM inference and RAG operations
- ✅ Async/await support with tokio
- ✅ Real-time token streaming
- ✅ Model and LoRA management
- ✅ Comprehensive error handling
- ✅ Zero-copy deserialization with serde

## Installation

Once published, add the SDK to your `Cargo.toml`:

```toml
[dependencies]
themisdb-client = "0.1"
tokio = { version = "1", features = ["full"] }
```

## Quick Start

```rust
use themisdb_client::{Client, Error};

#[tokio::main]
async fn main() -> Result<(), Error> {
    // Initialize client with authentication
    let client = Client::builder()
        .base_url("http://localhost:8080")
        .bearer_token("your-jwt-token")
        .build()?;

    // Execute AQL query
    let result = client
        .query("FOR doc IN myCollection RETURN doc")
        .execute()
        .await?;
    
    println!("{:?}", result);

    // LLM inference
    let response = client
        .llm()
        .infer()
        .prompt("What is ThemisDB?")
        .model("mistral-7b")
        .send()
        .await?;
    
    println!("Response: {}", response.text);

    Ok(())
}
```

## API Reference

### Client Initialization

```rust
use themisdb_client::Client;

// Builder pattern
let client = Client::builder()
    .base_url("http://localhost:8080")
    .bearer_token("token")
    .timeout(std::time::Duration::from_secs(30))
    .build()?;
```

### Data Operations

```rust
// Query execution
let result = client
    .query("FOR doc IN myCollection FILTER doc.age > @age RETURN doc")
    .bind_var("age", 25)
    .execute()
    .await?;

// Collection operations
let collections = client.collections().list().await?;
client.collections().create("newCollection").await?;
client.collections().drop("oldCollection").await?;

// Document operations
let doc = client
    .collection("users")
    .document("user123")
    .get()
    .await?;

client
    .collection("users")
    .document("user123")
    .update(/* document */)
    .await?;
```

### LLM Operations

```rust
// Inference
let response = client
    .llm()
    .infer()
    .prompt("Explain quantum computing")
    .model("mistral-7b")
    .max_tokens(100)
    .send()
    .await?;

// Streaming inference
let mut stream = client
    .llm()
    .infer()
    .prompt("Tell me a story")
    .stream()
    .await?;

while let Some(chunk) = stream.next().await {
    print!("{}", chunk?.token);
}

// RAG (Retrieval-Augmented Generation)
let response = client
    .llm()
    .rag()
    .query("What are the key features?")
    .collection("documentation")
    .send()
    .await?;

// Embeddings
let embeddings = client
    .llm()
    .embed("Sample text for embedding")
    .await?;

// Model management
let models = client.llm().list_models().await?;
client.llm().load_model("mistral-7b").await?;
client.llm().unload_model("old-model").await?;
```

### Administrative Functions

```rust
// Health check
let health = client.admin().health().await?;
println!("Status: {}", health.status);

// Statistics
let stats = client.admin().stats().await?;
println!("Documents: {}", stats.document_count);

// Cache management
client.admin().clear_cache().await?;
let cache_stats = client.admin().cache_stats().await?;
```

### Error Handling

```rust
use themisdb_client::{Error, ErrorKind};

match client.query("INVALID AQL").execute().await {
    Ok(result) => println!("{:?}", result),
    Err(Error::Api(err)) => {
        eprintln!("API error {}: {}", err.code, err.message);
    }
    Err(Error::Network(err)) => {
        eprintln!("Network error: {}", err);
    }
    Err(err) => {
        eprintln!("Other error: {}", err);
    }
}
```

## Development

### Prerequisites

- Rust 1.70+ (MSRV)
- Cargo

### Setup

```bash
# Clone the repository
git clone https://github.com/makr-code/ThemisDB.git
cd ThemisDB/sdks/rust

# Build the SDK
cargo build

# Run tests
cargo test

# Run examples
cargo run --example basic
```

### Project Structure

```
rust/
├── src/
│   ├── lib.rs              # Library entry point
│   ├── client.rs           # Main client implementation
│   ├── api/                # API modules
│   │   ├── mod.rs
│   │   ├── query.rs        # Query operations
│   │   ├── llm.rs          # LLM operations
│   │   └── admin.rs        # Admin operations
│   ├── models/             # Data models
│   │   ├── mod.rs
│   │   └── ...
│   ├── error.rs            # Error types
│   └── utils/              # Utility functions
├── tests/
│   └── integration.rs      # Integration tests
├── examples/
│   └── basic.rs            # Basic usage example
├── Cargo.toml              # Package manifest
└── README.md               # This file
```

## Testing

Run the test suite:

```bash
cargo test
```

Run with code coverage:

```bash
cargo tarpaulin
```

Run clippy lints:

```bash
cargo clippy -- -D warnings
```

## Examples

See the [examples](./examples) directory for usage examples:

- [basic.rs](./examples/basic.rs) - Basic operations
- [streaming.rs](./examples/streaming.rs) - Token streaming
- [rag.rs](./examples/rag.rs) - RAG operations

## Contributing

Contributions are welcome! Please see the [CONTRIBUTING.md](../../CONTRIBUTING.md) guide for details.

## License

Apache 2.0 - See [LICENSE](../../LICENSE) for details.

## Support

- Documentation: [docs.themisdb.org](https://docs.themisdb.org)
- GitHub Issues: [github.com/makr-code/ThemisDB/issues](https://github.com/makr-code/ThemisDB/issues)
- Community: [ThemisDB Discussions](https://github.com/makr-code/ThemisDB/discussions)
