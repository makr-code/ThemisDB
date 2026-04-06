# ThemisDB Client SDKs

This directory contains official client SDKs for ThemisDB in multiple programming languages.

## 🆕 SDK Enhancements (v1.8.0-rc1)

**New Features** (All Backwards Compatible):
- ✅ **Circuit Breaker Pattern**: Prevents cascading failures
- ✅ **Retry with Exponential Backoff**: Handles transient failures
- ✅ **Request/Response Logging**: Comprehensive debugging support
- ✅ **Connection Pooling**: Efficient HTTP connection management

See [SDK_ENHANCEMENTS.md](SDK_ENHANCEMENTS.md) for detailed documentation and usage examples.

## Available SDKs

### C# (.NET)
- **Path:** `csharp/`
- **Documentation:** [C# SDK Documentation](../docs/clients/)
- **Status:** Production-ready

### Go
- **Path:** `go/`
- **Documentation:** [Go SDK Documentation](../docs/clients/)
- **Status:** Production-ready

### Java
- **Path:** `java/`
- **Documentation:** [Java SDK Documentation](../docs/clients/)
- **Status:** Production-ready

### JavaScript/TypeScript
- **Path:** `javascript/` (HTTP/REST client)
- **Documentation:** [JavaScript SDK Quickstart](../docs/clients/javascript_sdk_quickstart.md)
- **Status:** Production-ready
- **Note:** Binary wire protocol implementation available in `typescript/` (experimental)

### PHP
- **Path:** `php/`
- **Documentation:** [PHP SDK README](php/README.md)
- **Status:** Production-ready
- **Package:** `composer require themisdb/themisdb-php`

### Python
- **Path:** `python/`
- **Documentation:** [Python SDK Quickstart](../docs/clients/python_sdk_quickstart.md)
- **Status:** Production-ready

### Ruby
- **Path:** `ruby/`
- **Documentation:** [Ruby SDK README](ruby/README.md)
- **Status:** Production-ready
- **Package:** `gem install themisdb`

### Rust
- **Path:** `rust/`
- **Documentation:** [Rust SDK Quickstart](../docs/clients/rust_sdk_quickstart.md)
- **Status:** Production-ready

### Swift
- **Path:** `swift/`
- **Documentation:** [Swift SDK Documentation](../docs/clients/)
- **Status:** Production-ready

## Getting Started

Each SDK directory contains its own README with language-specific installation and usage instructions.

## CI/CD Status

All client SDKs have automated CI/CD workflows for building, testing, and packaging:

| SDK | CI Workflow | Status |
|-----|-------------|--------|
| Python | `python-sdk-test.yml` | ✅ Available (Dry-Run) |
| Java | `java-sdk-test.yml` | ✅ Available (Dry-Run) |
| C# | `csharp-sdk-test.yml` | ✅ Available (Dry-Run) |
| Go | `go-sdk-test.yml` | ✅ Available (Dry-Run) |
| Rust | `rust-sdk-test.yml` | ✅ Available (Dry-Run) |
| JavaScript/TypeScript | `javascript-sdk-test.yml` | ✅ Available (Dry-Run) |
| PHP | `php-sdk-test.yml` | ✅ Available (Dry-Run) |
| Ruby | `ruby-sdk-test.yml` | ✅ Available (Dry-Run) |
| Swift | `swift-sdk-test.yml` | ✅ Available (Dry-Run) |

**Note:** All workflows are in "Dry-Run" mode - they build and test packages but do not publish to package registries. Publishing steps are commented out with TODO markers for production releases.

## SDK Feature Parity Matrix

All SDKs now support the same feature set (as of v1.8.0-rc1, April 2026):

| Feature | Python | JavaScript | Go | Rust | Java | C# | Swift | PHP | Ruby |
|---------|--------|------------|----|----|------|----|----|-----|------|
| CRUD Operations | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| AQL Queries | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| Transactions | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| **Graph API** | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| - `traverse()` | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| - `shortestPath()` | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| - `neighbors()` | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| **Vector API** | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| - `vectorSearch()` | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| - `vectorUpsert()` | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| - `vectorDelete()` | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| **LLM API (v1.4.0+)** 🆕 | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| - `llmInteraction()` | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| - `getLlmInteraction()` | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| - `listLlmInteractions()` | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| Batch Operations | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| Topology-Aware Routing | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| Async/Await | ✅ | ✅ | N/A | ✅ | N/A | ✅ | ✅ | N/A | N/A |
| TLS/mTLS | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| **🆕 Circuit Breaker** | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ❌ |
| **🆕 Retry + Backoff** | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ❌ |
| **🆕 Request/Response Logging** | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ❌ |
| Connection Pooling | ⚠️  | ⚠️  | ✅ | ✅ | ❌ | ✅ | ❌ | ✅ | ❌ |
| Binary Protocol | ❌ | ✅ | ✅ | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ |

**Legend:**
- ✅ = Implemented and tested
- ⚠️  = Implicit/Built-in (e.g., browser fetch, standard HTTP client)
- ❌ = Not implemented
- 📋 = Planned (template available in Python/JavaScript clients)
- N/A = Not applicable for this language
- 🆕 = New in v1.4.0-alpha

## Documentation

For comprehensive SDK documentation and examples, see the [docs/clients](../docs/clients/) directory.
