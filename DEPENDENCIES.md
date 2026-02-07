# ThemisDB External Dependencies

This document describes how external dependencies are managed in ThemisDB, with a focus on reproducible builds and security.

## Dependency Management Strategy

ThemisDB uses a hybrid approach for dependency management:
- **vcpkg** for most C++ libraries (via submodule)
- **Git Submodules** for critical dependencies requiring version pinning
- **FetchContent** for dependencies that need specific commit-level control

## Critical Dependencies

### llama.cpp (LLM Inference Engine)

**Purpose**: Core LLM inference capabilities for ThemisDB's AI features

**Management Strategy**: 
- FetchContent with pinned commit hash
- Git submodule for reproducible builds
- Explicit version control for stability

**Current Version**: 
- **Pinned Commit**: `b4313` (January 2024)
- **Repository**: https://github.com/ggerganov/llama.cpp.git

**Configuration**:
```cmake
# Located in: cmake/Dependencies.cmake
set(LLAMA_CPP_GIT_TAG "b4313" CACHE STRING "llama.cpp commit hash")
```

**Performance Optimizations Enabled**:
- ✅ **Flash Attention** (Release builds only): +15-25% performance
- ✅ **Continuous Batching** (All builds): +8x throughput for parallel requests

**Why Pinned?**
- llama.cpp has frequent breaking changes
- Ensures reproducible builds across environments
- Allows controlled updates with thorough testing
- Prevents unexpected behavior in production

### Updating llama.cpp

To update llama.cpp to a newer version:

1. **Test locally first**:
   ```bash
   # Set new commit hash
   cmake -DLLAMA_CPP_GIT_TAG=<new-commit-hash> ..
   
   # Build and run tests
   cmake --build . --target themis_tests
   ctest -R llm
   ```

2. **Update in cmake/Dependencies.cmake**:
   ```cmake
   set(LLAMA_CPP_GIT_TAG "<new-commit-hash>" CACHE STRING "...")
   ```

3. **Update submodule**:
   ```bash
   cd llama.cpp
   git fetch origin
   git checkout <new-commit-hash>
   cd ..
   git add llama.cpp
   git commit -m "Update llama.cpp to <commit-hash>"
   ```

4. **Run compatibility tests**:
   - CI/CD will automatically run llama.cpp integration tests
   - Manual testing: `./scripts/test-llama-integration.sh`
   - Performance benchmarks: `./benchmarks/llm_performance`

5. **Update this documentation** with new commit hash and release notes

### Verification

After updating llama.cpp, verify:

- [ ] All LLM tests pass (`ctest -R llm`)
- [ ] Performance benchmarks show expected results
- [ ] No security vulnerabilities introduced
- [ ] Flash Attention and Continuous Batching still work
- [ ] Multi-GPU support functions correctly
- [ ] LoRA adapters load and apply correctly

## vcpkg (Package Manager)

**Repository**: https://github.com/microsoft/vcpkg.git  
**Management**: Git submodule  
**Path**: `vcpkg/`

Provides most C++ dependencies:
- RocksDB (storage engine)
- gRPC (RPC framework)
- Protobuf (serialization)
- OpenSSL (cryptography)
- spdlog, fmt (logging)
- And many more...

### Updating vcpkg

```bash
cd vcpkg
git pull origin master
./bootstrap-vcpkg.sh  # or .bat on Windows
cd ..
git add vcpkg
git commit -m "Update vcpkg to latest"
```

## OpenSSL (Cryptography)

**Repository**: https://github.com/openssl/openssl.git  
**Management**: Git submodule (for Docker builds)  
**Path**: `docker/tmp/openssl`

Used for:
- TLS/SSL connections
- Cryptographic operations
- Certificate management

## Dependency Security

### Security Scanning

All dependencies are scanned for vulnerabilities:
- **GitHub Dependabot**: Automatic security alerts
- **OWASP Dependency Check**: Weekly scans
- **CodeQL**: Static analysis on all PRs

### Pinning Strategy

**Critical Dependencies** (pinned to specific commits):
- llama.cpp - Core functionality, frequent changes
- vcpkg - Package manager stability

**Flexible Dependencies** (version ranges):
- Most vcpkg packages - benefit from bug fixes
- System libraries - OS package manager handles

### Update Frequency

- **Critical Security Patches**: Immediate
- **Minor Updates**: Monthly review
- **Major Updates**: Quarterly with testing phase

## CI/CD Integration

### Automated Testing

GitHub Actions workflows test dependency compatibility:

1. **llama.cpp Compatibility** (`.github/workflows/llama-cpp-integration.yml`):
   - Runs on every PR modifying llama.cpp configuration
   - Tests inference, training, and multi-GPU features
   - Performance regression checks

2. **vcpkg Build** (`.github/workflows/build-and-test.yml`):
   - Full dependency resolution
   - Cross-platform builds (Linux, Windows, macOS)

3. **Dependency Scanning** (`.github/workflows/security-scan.yml`):
   - Weekly vulnerability scans
   - SBOM (Software Bill of Materials) generation

### Build Cache

- **vcpkg binary cache**: Speeds up CI builds
- **llama.cpp artifacts**: Cached per commit hash
- **CCCache**: Compiler cache for faster rebuilds

## Troubleshooting

### "llama.cpp target 'llama' not created"

**Solution**:
```bash
# Clean and reconfigure
rm -rf build llama.cpp
git submodule update --init --recursive
cmake -B build -S .
```

### "FetchContent failed to fetch llama.cpp"

**Causes**:
- Network issues
- Invalid commit hash
- Git configuration problems

**Solution**:
```bash
# Manual fetch
git clone https://github.com/ggerganov/llama.cpp.git
cd llama.cpp
git checkout b4313  # or desired commit
cd ..
# CMake will use existing directory
```

### Performance not improving after enabling Flash Attention

**Check**:
1. Build type is Release: `CMAKE_BUILD_TYPE=Release`
2. CUDA/Metal/Vulkan backend is enabled
3. GPU has sufficient memory
4. Model supports Flash Attention (check logs)

## References

- [llama.cpp GitHub](https://github.com/ggerganov/llama.cpp)
- [vcpkg Documentation](https://vcpkg.io/)
- [CMake FetchContent](https://cmake.org/cmake/help/latest/module/FetchContent.html)
- [ThemisDB Build Guide](BUILD.md)

## Cloud Storage Dependencies (GAP-008)

### AWS SDK for C++

**Purpose**: AWS S3 integration for cloud backup automation

**Management Strategy**: 
- vcpkg package manager
- Optional feature: `cloud-storage`
- Components: s3, transfer

**Installation**:
```bash
vcpkg install aws-sdk-cpp[s3,transfer]
```

**CMake Configuration**:
```bash
cmake -DTHEMIS_ENABLE_CLOUD_STORAGE=ON ..
```

**Planned ThemisDB Features (using AWS SDK)**:
- S3 bucket operations (create, list, delete)
- Object upload/download with multipart support
- Transfer manager for efficient large file uploads
- IAM role-based authentication
- Server-side encryption (SSE-S3, SSE-KMS)

> **Note**: ThemisDB's cloud backup APIs are currently stubs ("not yet implemented").  
> The features above describe capabilities of the AWS SDK that will be integrated  
> in future releases, not functionality that is fully available today.

**Why Optional?**
- Large dependency (~200MB compiled)
- Not required for core database functionality
- Only needed for cloud backup features
- Alternative: local filesystem backups

### Azure Storage SDK for C++

**Purpose**: Azure Blob Storage integration for cloud backup automation

**Management Strategy**:
- vcpkg package manager
- Optional feature: `cloud-storage`

**Installation**:
```bash
vcpkg install azure-storage-cpp
```

**CMake Configuration**:
```bash
cmake -DTHEMIS_ENABLE_CLOUD_STORAGE=ON ..
```

**Planned ThemisDB Features (using Azure SDK)**:
- Azure Blob container operations
- Block blob upload/download
- Blob lifecycle management
- Shared Access Signature (SAS) authentication
- Azure Active Directory authentication

> **Note**: ThemisDB's cloud backup APIs are currently stubs ("not yet implemented").  
> The features above describe capabilities of the Azure Storage SDK that will be  
> integrated in future releases, not functionality that is fully available today.

**Why Optional?**
- Only needed for Azure cloud deployments
- Alternative cloud providers available (AWS, GCS)
- Local filesystem backups work without it

### Google Cloud C++ SDK

**Purpose**: Google Cloud Storage integration for cloud backup automation

**Management Strategy**:
- vcpkg package manager
- Optional feature: `cloud-storage`
- Component: storage

**Installation**:
```bash
vcpkg install google-cloud-cpp[storage]
```

**CMake Configuration**:
```bash
cmake -DTHEMIS_ENABLE_CLOUD_STORAGE=ON ..
```

**Planned ThemisDB Features (using Google Cloud SDK)**:
- GCS bucket operations
- Object upload/download
- Resumable uploads for large files
- Service account authentication
- Customer-managed encryption keys (CMEK)

> **Note**: ThemisDB's cloud backup APIs are currently stubs ("not yet implemented").  
> The features above describe capabilities of the Google Cloud Storage SDK that will  
> be integrated in future releases, not functionality that is fully available today.

**Why Optional?**
- Only needed for Google Cloud deployments
- Alternative cloud providers available (AWS, Azure)
- Local filesystem backups work without it

### Enabling Cloud Storage Support

To enable all cloud storage backends:

```bash
# Install dependencies via vcpkg
vcpkg install aws-sdk-cpp[s3,transfer] azure-storage-cpp google-cloud-cpp[storage]

# Configure CMake with cloud storage enabled
cmake -B build -S . -DTHEMIS_ENABLE_CLOUD_STORAGE=ON

# Build
cmake --build build
```

### Enabling Individual Cloud Providers

You can install only the cloud provider(s) you need:

**AWS S3 only**:
```bash
vcpkg install aws-sdk-cpp[s3,transfer]
cmake -B build -S . -DTHEMIS_ENABLE_CLOUD_STORAGE=ON
```

**Azure Blob only**:
```bash
vcpkg install azure-storage-cpp
cmake -B build -S . -DTHEMIS_ENABLE_CLOUD_STORAGE=ON
```

**Google Cloud Storage only**:
```bash
vcpkg install google-cloud-cpp[storage]
cmake -B build -S . -DTHEMIS_ENABLE_CLOUD_STORAGE=ON
```

The build system will automatically detect which SDKs are available and enable the corresponding features.

### Cloud Storage Architecture

```
┌─────────────────────────────────────────┐
│         BackupManager                   │
│  (include/storage/backup_manager.h)    │
└─────────────┬───────────────────────────┘
              │
              ├──► Local Filesystem
              │
              ├──► AWS S3 (if THEMIS_HAS_AWS_SDK)
              │    └─ aws-sdk-cpp
              │
              ├──► Azure Blob (if THEMIS_HAS_AZURE_STORAGE)
              │    └─ azure-storage-cpp
              │
              └──► Google Cloud Storage (if THEMIS_HAS_GCS_SDK)
                   └─ google-cloud-cpp
```

### Security Considerations

1. **Credentials**: Never hardcode cloud credentials. Use:
   - AWS: IAM roles, AWS_PROFILE, or AWS_ACCESS_KEY_ID/AWS_SECRET_ACCESS_KEY env vars
   - Azure: Managed Identity, AZURE_STORAGE_ACCOUNT/AZURE_STORAGE_KEY env vars
   - GCS: Service account JSON, GOOGLE_APPLICATION_CREDENTIALS env var

2. **Encryption**: All cloud backups should use:
   - Encryption in transit (TLS/HTTPS)
   - Encryption at rest (provider-managed or customer-managed keys)

3. **Access Control**: Use least-privilege principle:
   - AWS: IAM policies with s3:PutObject, s3:GetObject only
   - Azure: Storage Account keys or SAS tokens with limited scope
   - GCS: Service account with storage.objects.create/get permissions only

### Troubleshooting

**"AWS SDK not found"**:
```bash
# Verify installation
vcpkg list | grep aws-sdk-cpp

# If not installed:
vcpkg install aws-sdk-cpp[s3,transfer]

# Rebuild
cmake -B build -S . -DTHEMIS_ENABLE_CLOUD_STORAGE=ON
```

**"Azure Storage SDK not found"**:
```bash
# Verify installation
vcpkg list | grep azure-storage-cpp

# If not installed:
vcpkg install azure-storage-cpp

# Rebuild
cmake -B build -S . -DTHEMIS_ENABLE_CLOUD_STORAGE=ON
```

**"Google Cloud SDK not found"**:
```bash
# Verify installation
vcpkg list | grep google-cloud-cpp

# If not installed:
vcpkg install google-cloud-cpp[storage]

# Rebuild
cmake -B build -S . -DTHEMIS_ENABLE_CLOUD_STORAGE=ON
```

### Performance Considerations

- **Upload Speed**: All SDKs support multipart/resumable uploads for large files
- **Network Overhead**: Cloud uploads add ~10-100ms latency depending on region
- **Memory Usage**: Each SDK adds ~50-200MB to memory footprint
- **Recommendation**: Enable only the cloud provider(s) you need

### Compatibility

- **AWS SDK C++**: Requires C++11 or later, OpenSSL
- **Azure Storage C++**: Requires C++11 or later, libcurl, OpenSSL
- **Google Cloud C++**: Requires C++14 or later, gRPC, Protobuf

All SDKs are cross-platform (Linux, Windows, macOS).

## Changelog

### 2026-02-07
- **GAP-008 Implementation**: Added cloud storage dependencies
- Added AWS SDK C++, Azure Storage C++, Google Cloud C++ to vcpkg.json
- Created cloud-storage feature for optional installation
- Updated cmake/Dependencies.cmake with cloud storage detection
- Added comprehensive documentation for cloud storage setup

### 2024-02-03
- **CRITICAL FIX (PR #1022)**: Pinned llama.cpp to commit `b4313`
- Enabled Flash Attention for Release builds (+15-25% performance)
- Enabled Continuous Batching for all builds (+8x throughput)
- Added FetchContent-based dependency management
- Created this documentation

### Previous
- llama.cpp managed via manual git clone
- No version pinning (risk of breaking changes)
- Flash Attention and Continuous Batching disabled
