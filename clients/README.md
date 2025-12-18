# ThemisDB Client SDKs

This directory contains official client SDKs for ThemisDB in multiple programming languages.

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
- **Path:** `javascript/`
- **Documentation:** [JavaScript SDK Quickstart](../docs/clients/javascript_sdk_quickstart.md)
- **Status:** Production-ready

### PHP
- **Path:** `php/`
- **Documentation:** [PHP SDK README](php/README.md)
- **Status:** Production-ready
- **Package:** `composer require themisdb/themisdb-php`

### Python
- **Path:** `python/`
- **Documentation:** [Python SDK Quickstart](../docs/clients/python_sdk_quickstart.md)
- **Status:** Production-ready

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

## SDK Feature Parity Matrix

All SDKs now support the same feature set (as of December 2025):

| Feature | Python | JavaScript | Go | Rust | Java | C# | Swift | PHP |
|---------|--------|------------|----|----|------|----|----|-----|
| CRUD Operations | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| AQL Queries | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| Transactions | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| **Graph API** | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| - `traverse()` | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| - `shortestPath()` | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| - `neighbors()` | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| **Vector API** | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| - `vectorSearch()` | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| - `vectorUpsert()` | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| - `vectorDelete()` | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| Batch Operations | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| Topology-Aware Routing | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| Async/Await | ✅ | ✅ | N/A | ✅ | N/A | ✅ | ✅ | N/A |
| TLS/mTLS | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |

## Documentation

For comprehensive SDK documentation and examples, see the [docs/clients](../docs/clients/) directory.
