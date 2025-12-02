# ThemisDB SDK Publishing Checklist

## Pre-Publishing Requirements

### 1. Version Control
- [ ] All changes committed to main branch
- [ ] Version number updated in VERSION file
- [ ] CHANGELOG.md updated with release notes
- [ ] Git tag created (e.g., `v1.0.0`)

### 2. Testing
- [ ] All unit tests passing
- [ ] Integration tests passing
- [ ] SDK examples verified working
- [ ] Cross-platform builds verified

### 3. Documentation
- [ ] API documentation up to date
- [ ] README.md reviewed and updated
- [ ] Migration guide (if breaking changes)
- [ ] Example code tested

## SDK-Specific Checklists

### JavaScript/TypeScript (@themisdb/client → NPM)

**Prerequisites:**
- [ ] Node.js 18+ installed
- [ ] NPM account with publishing rights
- [ ] `NPM_TOKEN` environment variable set

**Files to verify:**
- [ ] `clients/javascript/package.json` - version, dependencies
- [ ] `clients/javascript/tsconfig.json` - compilation settings
- [ ] `clients/javascript/README.md` - npm-specific docs

**Build & Test:**
```bash
cd clients/javascript
npm ci
npm run build
npm test
npm pack --dry-run  # Verify package contents
```

**Publish:**
```bash
npm publish --access public
```

---

### Python (themisdb → PyPI)

**Prerequisites:**
- [ ] Python 3.9+ installed
- [ ] PyPI account with publishing rights
- [ ] `PYPI_TOKEN` environment variable set

**Files to verify:**
- [ ] `clients/python/pyproject.toml` - version, dependencies
- [ ] `clients/python/setup.cfg` - metadata
- [ ] `clients/python/README.md` - PyPI-specific docs

**Build & Test:**
```bash
cd clients/python
python -m venv .venv
source .venv/bin/activate
pip install -e ".[dev]"
pytest
python -m build
twine check dist/*
```

**Publish:**
```bash
twine upload dist/* -u __token__ -p $PYPI_TOKEN
```

---

### C# (ThemisDB.Client → NuGet)

**Prerequisites:**
- [ ] .NET 6.0+ SDK installed
- [ ] NuGet account with publishing rights
- [ ] `NUGET_API_KEY` environment variable set

**Files to verify:**
- [ ] `clients/csharp/ThemisDB.Client/ThemisDB.Client.csproj` - version, metadata
- [ ] `clients/csharp/README.md` - NuGet-specific docs

**Build & Test:**
```bash
cd clients/csharp
dotnet restore
dotnet build -c Release
dotnet test -c Release
dotnet pack -c Release -o ./nupkg
```

**Publish:**
```bash
dotnet nuget push ./nupkg/*.nupkg --api-key $NUGET_API_KEY --source https://api.nuget.org/v3/index.json
```

---

### Java (io.themisdb:client → Maven Central)

**Prerequisites:**
- [ ] JDK 11+ installed
- [ ] Maven 3.8+ installed
- [ ] Sonatype OSSRH account
- [ ] GPG key for signing
- [ ] `MAVEN_USERNAME`, `MAVEN_PASSWORD`, `GPG_PASSPHRASE` set

**Files to verify:**
- [ ] `clients/java/pom.xml` - version, groupId, artifactId
- [ ] `clients/java/README.md` - Maven-specific docs

**Build & Test:**
```bash
cd clients/java
mvn clean verify
mvn source:jar javadoc:jar
```

**Publish:**
```bash
mvn deploy -Dgpg.passphrase=$GPG_PASSPHRASE
```

---

### Rust (themisdb → Crates.io)

**Prerequisites:**
- [ ] Rust 1.70+ installed
- [ ] Crates.io account
- [ ] `CARGO_TOKEN` environment variable set

**Files to verify:**
- [ ] `clients/rust/Cargo.toml` - version, metadata
- [ ] `clients/rust/README.md` - Crates.io-specific docs

**Build & Test:**
```bash
cd clients/rust
cargo fmt --check
cargo clippy -- -D warnings
cargo test
cargo package --list
```

**Publish:**
```bash
cargo login $CARGO_TOKEN
cargo publish
```

---

### Go (github.com/themisdb/go-client)

**Prerequisites:**
- [ ] Go 1.21+ installed
- [ ] GitHub repository access

**Files to verify:**
- [ ] `clients/go/go.mod` - module path, Go version
- [ ] `clients/go/README.md` - Go-specific docs

**Build & Test:**
```bash
cd clients/go
go mod tidy
go build ./...
go test ./...
```

**Publish:**
```bash
git tag clients/go/v1.0.0
git push origin clients/go/v1.0.0
```

---

### Swift (ThemisDB → Swift Package Manager)

**Prerequisites:**
- [ ] Xcode 14+ installed (macOS)
- [ ] GitHub repository access

**Files to verify:**
- [ ] `clients/swift/Package.swift` - version, dependencies
- [ ] `clients/swift/README.md` - Swift-specific docs

**Build & Test:**
```bash
cd clients/swift
swift build
swift test
```

**Publish:**
```bash
git tag clients/swift/1.0.0
git push origin clients/swift/1.0.0
```

---

## Post-Publishing Verification

### For Each SDK:
- [ ] Package visible in registry
- [ ] Version number correct
- [ ] Dependencies resolved correctly
- [ ] README rendered properly
- [ ] License displayed
- [ ] Installation command works

### Integration Test:
```bash
# Create fresh project and test installation
mkdir test-themisdb && cd test-themisdb

# JavaScript
npm init -y && npm install @themisdb/client

# Python
python -m venv venv && source venv/bin/activate && pip install themisdb

# C#
dotnet new console && dotnet add package ThemisDB.Client

# Java
mvn archetype:generate ... && add dependency to pom.xml

# Rust
cargo new test-themisdb && add to Cargo.toml

# Go
go mod init test && go get github.com/themisdb/go-client
```

## Rollback Procedure

If issues are discovered after publishing:

### NPM
```bash
npm unpublish @themisdb/client@1.0.0  # Within 72 hours only
# OR deprecate
npm deprecate @themisdb/client@1.0.0 "Critical bug, use 1.0.1"
```

### PyPI
```bash
# Cannot unpublish - publish new version instead
# Can yank to prevent new installs
pip install twine
twine yank themisdb 1.0.0
```

### NuGet
```bash
# Unlist (doesn't delete)
dotnet nuget delete ThemisDB.Client 1.0.0 --source https://api.nuget.org/v3/index.json
```

### Maven Central
- Cannot delete - publish new version
- Can close/drop staging repository before release

### Crates.io
```bash
cargo yank --vers 1.0.0
```

### Go
```bash
# Retract in go.mod
retract v1.0.0
```

## Security Checklist

- [ ] No secrets in published packages
- [ ] No test credentials in code
- [ ] Dependencies scanned for vulnerabilities
- [ ] SBOM generated and attached to release
- [ ] Signed releases where supported

## Automation

Use the master publish script:
```bash
./scripts/sdk-publish/publish-all.sh --version 1.0.0
```

Or publish individual SDKs:
```bash
./scripts/sdk-publish/publish-npm.sh --version 1.0.0
./scripts/sdk-publish/publish-pypi.sh --version 1.0.0
# etc.
```

## Registry URLs

| SDK | Registry | URL |
|-----|----------|-----|
| JavaScript | NPM | https://www.npmjs.com/package/@themisdb/client |
| Python | PyPI | https://pypi.org/project/themisdb/ |
| C# | NuGet | https://www.nuget.org/packages/ThemisDB.Client |
| Java | Maven | https://search.maven.org/artifact/io.themisdb/client |
| Rust | Crates.io | https://crates.io/crates/themisdb |
| Go | pkg.go.dev | https://pkg.go.dev/github.com/themisdb/go-client |
| Swift | GitHub | https://github.com/themisdb/swift-client |
