# SDK Publishing Guide

**Status:** Ready for Publishing  
**Version:** 1.0.0  
**Date:** December 2025

## Overview

This guide documents the publishing process for all ThemisDB client SDKs. All SDKs are maintained in the `clients/` directory and share feature parity.

## SDK Feature Matrix

All SDKs support the following features:

| Feature | Description |
|---------|-------------|
| CRUD Operations | Create, Read, Update, Delete entities |
| AQL Queries | Execute queries with parameters |
| Transactions | ACID transactions with isolation levels |
| Graph API | `traverse()`, `shortestPath()`, `neighbors()` |
| Vector API | `vectorSearch()`, `vectorUpsert()`, `vectorDelete()` |
| Batch Operations | Batch get/put/delete |
| Topology-Aware Routing | Automatic shard routing |
| TLS/mTLS | Secure connections |

## Publishing Checklist

### Pre-Release

- [ ] All tests pass
- [ ] Feature parity verified across SDKs
- [ ] Version numbers updated
- [ ] CHANGELOG updated
- [ ] README updated with new features
- [ ] Breaking changes documented

### Release Process

1. Create release branch
2. Update version numbers
3. Run full test suite
4. Build packages
5. Publish to registries
6. Tag release in Git
7. Update documentation

---

## Python SDK

### Package Information

- **Name:** `themisdb-client`
- **Registry:** PyPI
- **Config:** `clients/python/pyproject.toml`

### Build & Publish

```bash
cd clients/python

# Create virtual environment
python -m venv venv
source venv/bin/activate

# Install build tools
pip install build twine

# Build package
python -m build

# Upload to PyPI
twine upload dist/*

# For test PyPI first:
twine upload --repository testpypi dist/*
```

### Installation

```bash
pip install themisdb-client
```

### Version Update

Edit `pyproject.toml`:
```toml
[project]
version = "1.0.0"
```

---

## JavaScript/TypeScript SDK

### Package Information

- **Name:** `@themisdb/client`
- **Registry:** NPM
- **Config:** `clients/javascript/package.json`

### Build & Publish

```bash
cd clients/javascript

# Install dependencies
npm install

# Run tests
npm test

# Build TypeScript
npm run build

# Publish to NPM
npm publish --access public

# For beta releases:
npm publish --tag beta --access public
```

### Installation

```bash
npm install @themisdb/client
# or
yarn add @themisdb/client
```

### Version Update

Edit `package.json`:
```json
{
  "version": "1.0.0"
}
```

---

## Go SDK

### Package Information

- **Name:** `github.com/makr-code/ThemisDB/clients/go`
- **Registry:** Go Modules (GitHub)
- **Config:** `clients/go/go.mod`

### Publish

Go modules are automatically available via GitHub tags:

```bash
cd clients/go

# Ensure go.mod is correct
go mod tidy

# Tag release (from repo root)
git tag clients/go/v1.0.0
git push origin clients/go/v1.0.0
```

### Installation

```bash
go get github.com/makr-code/ThemisDB/clients/go@v1.0.0
```

### Version Update

Edit `go.mod`:
```go
module github.com/makr-code/ThemisDB/clients/go

go 1.21
```

---

## Rust SDK

### Package Information

- **Name:** `themisdb-client`
- **Registry:** crates.io
- **Config:** `clients/rust/Cargo.toml`

### Build & Publish

```bash
cd clients/rust

# Run tests
cargo test

# Build
cargo build --release

# Publish to crates.io
cargo publish

# For dry-run:
cargo publish --dry-run
```

### Installation

```toml
# Cargo.toml
[dependencies]
themisdb-client = "1.0.0"
```

### Version Update

Edit `Cargo.toml`:
```toml
[package]
name = "themisdb-client"
version = "1.0.0"
```

---

## Java SDK

### Package Information

- **Name:** `io.themisdb:themisdb-client`
- **Registry:** Maven Central
- **Config:** `clients/java/pom.xml`

### Build & Publish

```bash
cd clients/java

# Build
mvn clean package

# Run tests
mvn test

# Deploy to Maven Central (requires GPG signing)
mvn deploy -P release

# For Sonatype staging:
mvn deploy -P release -DstagingProfileId=xxx
```

### Installation

```xml
<!-- pom.xml -->
<dependency>
    <groupId>io.themisdb</groupId>
    <artifactId>themisdb-client</artifactId>
    <version>1.0.0</version>
</dependency>
```

### Version Update

Edit `pom.xml`:
```xml
<version>1.0.0</version>
```

---

## C# SDK

### Package Information

- **Name:** `ThemisDB.Client`
- **Registry:** NuGet
- **Config:** `clients/csharp/ThemisDB.Client/ThemisDB.Client.csproj`

### Build & Publish

```bash
cd clients/csharp/ThemisDB.Client

# Build
dotnet build -c Release

# Run tests
dotnet test

# Pack
dotnet pack -c Release

# Publish to NuGet
dotnet nuget push bin/Release/ThemisDB.Client.1.0.0.nupkg --api-key YOUR_API_KEY --source https://api.nuget.org/v3/index.json
```

### Installation

```bash
dotnet add package ThemisDB.Client
```

### Version Update

Edit `.csproj`:
```xml
<Version>1.0.0</Version>
```

---

## Swift SDK

### Package Information

- **Name:** `ThemisDB`
- **Registry:** Swift Package Manager (GitHub)
- **Config:** `clients/swift/Package.swift`

### Publish

Swift packages are available via GitHub tags:

```bash
# Tag release (from repo root)
git tag swift-1.0.0
git push origin swift-1.0.0
```

### Installation

```swift
// Package.swift
dependencies: [
    .package(url: "https://github.com/makr-code/ThemisDB.git", from: "1.0.0")
]
```

### Version Update

Tags define versions for Swift packages.

---

## CI/CD Integration

### GitHub Actions Workflow

```yaml
name: Publish SDKs

on:
  release:
    types: [published]

jobs:
  publish-python:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - uses: actions/setup-python@v5
        with:
          python-version: '3.11'
      - name: Publish to PyPI
        working-directory: clients/python
        env:
          TWINE_USERNAME: __token__
          TWINE_PASSWORD: ${{ secrets.PYPI_API_TOKEN }}
        run: |
          pip install build twine
          python -m build
          twine upload dist/*

  publish-npm:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - uses: actions/setup-node@v4
        with:
          node-version: '20'
          registry-url: 'https://registry.npmjs.org'
      - name: Publish to NPM
        working-directory: clients/javascript
        env:
          NODE_AUTH_TOKEN: ${{ secrets.NPM_TOKEN }}
        run: |
          npm ci
          npm run build
          npm publish --access public

  publish-rust:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - uses: dtolnay/rust-toolchain@stable
      - name: Publish to crates.io
        working-directory: clients/rust
        env:
          CARGO_REGISTRY_TOKEN: ${{ secrets.CRATES_IO_TOKEN }}
        run: cargo publish

  publish-nuget:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - uses: actions/setup-dotnet@v4
        with:
          dotnet-version: '8.0.x'
      - name: Publish to NuGet
        working-directory: clients/csharp/ThemisDB.Client
        run: |
          dotnet pack -c Release
          dotnet nuget push bin/Release/*.nupkg --api-key ${{ secrets.NUGET_API_KEY }} --source https://api.nuget.org/v3/index.json
```

## Version Synchronization

All SDKs should use the same major.minor version:

| SDK | Current Version | Target Version |
|-----|-----------------|----------------|
| Python | 0.1.0-beta.1 | 1.0.0 |
| JavaScript | 0.1.0-beta.1 | 1.0.0 |
| Go | 0.1.0 | 1.0.0 |
| Rust | 0.1.0 | 1.0.0 |
| Java | 0.1.0-SNAPSHOT | 1.0.0 |
| C# | 0.1.0-beta | 1.0.0 |
| Swift | 0.1.0 | 1.0.0 |

## Security Considerations

1. **API Keys:** Store all registry API keys in GitHub Secrets
2. **GPG Signing:** Required for Maven Central
3. **2FA:** Enable 2FA on all registry accounts
4. **Audit:** Review dependencies before each release

## Support

For SDK publishing issues, contact the ThemisDB team or open an issue in the repository.
