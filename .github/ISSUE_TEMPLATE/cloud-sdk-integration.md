---
name: Integrate AWS/Azure/GCS SDKs for Cloud Backup
about: Replace placeholder cloud storage providers with real SDK integrations
title: 'Integrate AWS/Azure/GCS SDKs for Cloud Backup'
labels: type:enhancement, area:sharding, area:storage, priority:P1, effort:medium, v1.4.0
assignees: ''
---

## 📋 Summary

Integrate real cloud storage SDKs (AWS S3, Azure Blob Storage, Google Cloud Storage) into ThemisDB's cloud backup infrastructure. The current v1.3.0 implementation provides complete coordinator logic with placeholder providers that explicitly return failure without SDKs and support mock mode for testing.

**Parent Feature:** Issue #[PR-NUMBER] - Cloud Backup Infrastructure (v1.3.0)

## 🔍 Problem Statement

### Current State (v1.3.0)
- ✅ `CloudBackupCoordinator`: Complete API and logic
- ✅ Provider interfaces: S3, Azure, GCS (placeholder implementations)
- ✅ Backup operations: Create, restore, delete, list
- ✅ Multi-datacenter replication configuration
- ✅ Mock mode for testing (`THEMIS_CLOUD_BACKUP_MOCK=1`)
- ✅ Explicit failure without SDKs (returns `false`)
- ❌ No real AWS S3 SDK integration
- ❌ No real Azure SDK integration
- ❌ No real Google Cloud SDK integration

### Customer Need
Enterprise customers require:
1. **Cloud backup** for disaster recovery
2. **Multi-cloud support** for vendor independence
3. **Geographic redundancy** across regions
4. **Automated backup lifecycle** management
5. **Cost-effective long-term storage**

### Business Impact
**Without Cloud SDKs:**
- Local backups only (single point of failure)
- Manual cloud synchronization required
- No geographic redundancy
- Limited disaster recovery capabilities
- Not production-ready for cloud-native deployments

**With Cloud SDKs:**
- ✅ Automated cloud backup and restore
- ✅ Geographic redundancy across regions
- ✅ Multi-datacenter replication
- ✅ Cost-effective cloud storage (S3 Glacier, etc.)
- ✅ Production-ready disaster recovery

## 🎯 Requirements

### Functional Requirements

#### FR-1: AWS S3 Integration
- [ ] Integrate aws-sdk-cpp library
- [ ] Implement `S3StorageProvider::upload` with real SDK
- [ ] Implement `S3StorageProvider::download` with real SDK
- [ ] Implement `S3StorageProvider::deleteObject`
- [ ] Implement `S3StorageProvider::listObjects`
- [ ] Implement `S3StorageProvider::exists`
- [ ] Support S3-compatible storage (MinIO, etc.)
- [ ] Implement multipart upload for large files (>5MB)
- [ ] Add retry logic with exponential backoff

#### FR-2: Azure Blob Storage Integration
- [ ] Integrate azure-storage-cpp library
- [ ] Implement `AzureStorageProvider::upload` with real SDK
- [ ] Implement `AzureStorageProvider::download` with real SDK
- [ ] Implement `AzureStorageProvider::deleteObject`
- [ ] Implement `AzureStorageProvider::listObjects`
- [ ] Implement `AzureStorageProvider::exists`
- [ ] Support block blob operations
- [ ] Add retry logic with exponential backoff

#### FR-3: Google Cloud Storage Integration
- [ ] Integrate google-cloud-cpp[storage] library
- [ ] Implement `GCSStorageProvider::upload` with real SDK
- [ ] Implement `GCSStorageProvider::download` with real SDK
- [ ] Implement `GCSStorageProvider::deleteObject`
- [ ] Implement `GCSStorageProvider::listObjects`
- [ ] Implement `GCSStorageProvider::exists`
- [ ] Support resumable uploads
- [ ] Add retry logic with exponential backoff

#### FR-4: Credential Management
- [ ] Support environment variable credentials
- [ ] Support credential file configuration
- [ ] Support IAM role credentials (EC2, GCE, Azure VM)
- [ ] Document credential setup for each provider
- [ ] Validate credentials on initialization

#### FR-5: Error Handling
- [ ] Handle authentication failures
- [ ] Handle network errors with retry
- [ ] Handle insufficient permissions
- [ ] Handle storage quota exceeded
- [ ] Maintain fallback behavior (log error, return false)

### Non-Functional Requirements

#### NFR-1: Performance
- [ ] Parallel uploads for multiple shards
- [ ] Compression before upload (configurable)
- [ ] Connection pooling for HTTP requests
- [ ] Progress tracking for large files
- [ ] Async upload/download support

#### NFR-2: Reliability
- [ ] Retry transient failures (3 attempts default)
- [ ] Exponential backoff between retries
- [ ] Data integrity verification (checksums)
- [ ] Atomic operations where possible

#### NFR-3: Cost Optimization
- [ ] Compression reduces storage costs by 30-50%
- [ ] Support storage classes (S3 Glacier, Azure Cool, GCS Nearline)
- [ ] Lifecycle policies for automatic archival
- [ ] Regional selection for cost optimization

## 🏗️ Implementation Guide

**Detailed guide available**: `docs/CLOUD_SDK_INTEGRATION_GUIDE.md`

### Key Components

1. **S3 Provider** (`src/sharding/cloud_backup.cpp`)
   - Initialize AWS SDK (call once at startup)
   - Create S3 client with credentials
   - Implement upload with multipart support
   - Implement download with streaming
   - Add comprehensive error handling

2. **Azure Provider** (`src/sharding/cloud_backup.cpp`)
   - Initialize Azure SDK
   - Parse connection string
   - Create container if not exists
   - Implement upload/download operations
   - Handle Azure-specific errors

3. **GCS Provider** (`src/sharding/cloud_backup.cpp`)
   - Initialize GCS client
   - Use application default credentials
   - Implement upload with resumable support
   - Implement download with streaming
   - Handle GCS-specific errors

### Testing Requirements

#### Unit Tests (`tests/test_cloud_backup.cpp`)
- [x] Mock mode tests (already implemented in v1.3.0)
- [ ] SDK initialization tests
- [ ] Credential validation tests
- [ ] Upload/download tests (requires real accounts)
- [ ] Error handling tests
- [ ] Retry logic tests

#### Integration Tests (require real cloud accounts)
- [ ] End-to-end backup and restore
- [ ] Large file handling (multipart upload)
- [ ] Concurrent operations (thread safety)
- [ ] Cross-region replication
- [ ] Error recovery scenarios

#### CI/CD Strategy
- Use mock mode in CI pipeline
- Real integration tests in staging environment
- Automated testing with test cloud accounts

## 📦 Dependencies

### Build Dependencies
```cmake
# AWS SDK
find_package(AWSSDK REQUIRED COMPONENTS s3 core)
target_link_libraries(themis_core PRIVATE AWS::aws-sdk-cpp-s3 AWS::aws-sdk-cpp-core)

# Azure SDK
find_package(azure-storage-cpp CONFIG REQUIRED)
target_link_libraries(themis_core PRIVATE azure-storage-cpp::azure-storage-cpp)

# Google Cloud SDK
find_package(google_cloud_cpp_storage REQUIRED)
target_link_libraries(themis_core PRIVATE google-cloud-cpp::storage)
```

### Package Installation (vcpkg)
```bash
vcpkg install aws-sdk-cpp[s3]
vcpkg install azure-storage-cpp
vcpkg install google-cloud-cpp[storage]
```

### Runtime Dependencies
- AWS credentials (env vars, credentials file, or IAM role)
- Azure connection string (env var or config file)
- GCS service account key (GOOGLE_APPLICATION_CREDENTIALS)

## 🔐 Credential Configuration

### AWS S3
```bash
export AWS_ACCESS_KEY_ID="your_access_key"
export AWS_SECRET_ACCESS_KEY="your_secret_key"
export AWS_DEFAULT_REGION="us-east-1"

# Or use credentials file: ~/.aws/credentials
# Or use IAM role (recommended for EC2)
```

### Azure Blob Storage
```bash
export AZURE_STORAGE_CONNECTION_STRING="DefaultEndpointsProtocol=https;AccountName=...;AccountKey=...;"

# Or use Azure managed identity (recommended for Azure VMs)
```

### Google Cloud Storage
```bash
export GOOGLE_APPLICATION_CREDENTIALS="/path/to/service-account-key.json"

# Or use application default credentials (recommended for GCE)
```

## 🎯 Acceptance Criteria

### Must Have (P0)
- [ ] AWS S3 upload and download working
- [ ] Azure Blob Storage upload and download working
- [ ] GCS upload and download working
- [ ] Credential validation on initialization
- [ ] Retry logic with exponential backoff
- [ ] Error handling maintains explicit failure behavior
- [ ] Integration tests passing with real cloud accounts

### Should Have (P1)
- [ ] Multipart upload for large files (S3)
- [ ] Resumable uploads (GCS)
- [ ] Progress tracking for large transfers
- [ ] Parallel uploads for multiple shards
- [ ] Compression integration

### Nice to Have (P2)
- [ ] Storage class selection (Glacier, Cool, Nearline)
- [ ] Transfer acceleration (S3)
- [ ] CDN integration for downloads
- [ ] Bandwidth limiting

## 📊 Performance Targets

| Operation | Target | Notes |
|-----------|--------|-------|
| Upload (1GB file) | < 30s | With compression, varies by bandwidth |
| Download (1GB file) | < 30s | Varies by bandwidth |
| List objects (1K items) | < 2s | Pagination supported |
| Delete object | < 1s | Single object deletion |

## 🔗 Related Issues

- #[PR-NUMBER]: Cloud Backup Infrastructure (v1.3.0) - **Prerequisite**
- #[ISSUE-NUMBER]: GPU Kernel Implementation - **Parallel work**
- #[ISSUE-NUMBER]: Incremental Backup Support - **Follow-up**

## 📅 Timeline

**Target Release**: v1.4.0 (Q2 2026)
**Estimated Effort**: 2-3 weeks
- Week 1: AWS S3 integration and testing
- Week 2: Azure and GCS integration
- Week 3: Integration testing, documentation, optimization

## 💡 Implementation Notes

- Refer to `docs/CLOUD_SDK_INTEGRATION_GUIDE.md` for detailed implementation
- Current mock mode tests serve as template for real SDK tests
- Maintain backward compatibility with mock mode
- Document credential setup clearly
- Add examples for each cloud provider
- Consider vcpkg for SDK dependency management

## 🧪 Testing Strategy

### Development Testing
```bash
# Use mock mode during development
export THEMIS_CLOUD_BACKUP_MOCK=1
./tests/test_cloud_backup
```

### Integration Testing
```bash
# Real AWS S3 testing
export AWS_ACCESS_KEY_ID="..."
export AWS_SECRET_ACCESS_KEY="..."
unset THEMIS_CLOUD_BACKUP_MOCK
./tests/test_cloud_backup --filter=*S3*

# Real Azure testing
export AZURE_STORAGE_CONNECTION_STRING="..."
./tests/test_cloud_backup --filter=*Azure*

# Real GCS testing
export GOOGLE_APPLICATION_CREDENTIALS="..."
./tests/test_cloud_backup --filter=*GCS*
```

## 📚 Documentation Requirements

- [ ] Update `PRODUCTION_DEPLOYMENT_GUIDE.md` with credential setup
- [ ] Create credential configuration examples
- [ ] Document SDK installation instructions
- [ ] Add troubleshooting guide for common issues
- [ ] Document cost optimization strategies
- [ ] Update migration guide from mock mode

## ✅ Definition of Done

- [ ] AWS S3 SDK integrated and working
- [ ] Azure SDK integrated and working
- [ ] GCS SDK integrated and working
- [ ] All unit tests passing (mock mode)
- [ ] Integration tests passing (real accounts)
- [ ] Retry logic implemented and tested
- [ ] Error handling comprehensive
- [ ] Credentials documented
- [ ] Code reviewed and approved
- [ ] Documentation updated
- [ ] Merged to main branch

---

**Created**: February 7, 2026
**For**: ThemisDB v1.4.0
**Priority**: High
**Effort**: Medium (2-3 weeks)
