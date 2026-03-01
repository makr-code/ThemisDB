# Cloud SDK Integration Guide

**Target Release**: v1.4.0 (Q2 2026)  
**Priority**: High  
**Effort**: 2-3 weeks  
**Expertise Required**: Cloud platforms (AWS/Azure/GCS), C++ SDK integration

## Overview

This guide provides a detailed blueprint for integrating real cloud storage SDKs (AWS S3, Azure Blob Storage, Google Cloud Storage) into ThemisDB's cloud backup infrastructure. The current implementation (`src/sharding/cloud_backup.cpp`) provides placeholder interfaces that return explicit failures without SDK integration and support mock mode for testing.

## Current State

### What Exists (v1.3.0)
- ✅ `CloudBackupCoordinator`: Complete API and coordinator logic
- ✅ Provider interfaces: S3, Azure, GCS (placeholder implementations)
- ✅ Backup operations: Create, restore, delete, list
- ✅ Multi-datacenter replication configuration
- ✅ Mock mode for testing (`THEMIS_CLOUD_BACKUP_MOCK=1`)
- ✅ Explicit failure without SDKs (no silent false positives)

### What Needs Implementation
- ❌ Real AWS S3 SDK integration (aws-sdk-cpp)
- ❌ Real Azure Blob Storage SDK integration (azure-storage-cpp)
- ❌ Real Google Cloud Storage SDK integration (google-cloud-cpp)
- ❌ Credential management and authentication
- ❌ Retry logic and error handling
- ❌ Progress tracking for large uploads/downloads

## Architecture

### Data Flow
```
1. Local backup creation (BackupManager)
   ↓
2. Compression (optional, configurable)
   ↓
3. Encryption (optional, with KEK/DEK)
   ↓
4. Cloud upload via provider SDK
   ↓
5. Metadata catalog update
   ↓
6. Optional: Multi-datacenter replication
```

## AWS S3 Integration

### Step 1: Dependencies

**CMakeLists.txt**:
```cmake
# Find AWS SDK
find_package(AWSSDK REQUIRED COMPONENTS s3 core)

# Link AWS SDK
target_link_libraries(themis_core PRIVATE
    AWS::aws-sdk-cpp-s3
    AWS::aws-sdk-cpp-core
)
```

**vcpkg.json** (or install via vcpkg):
```bash
vcpkg install aws-sdk-cpp[s3]
```

### Step 2: SDK Initialization

**File**: `src/sharding/cloud_backup.cpp`

```cpp
#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/s3/model/DeleteObjectRequest.h>
#include <aws/s3/model/ListObjectsV2Request.h>
#include <aws/s3/model/HeadObjectRequest.h>
#include <aws/core/auth/AWSCredentialsProvider.h>
#include <fstream>

// Global SDK initialization (call once at startup)
static std::once_flag aws_init_flag;
static void initializeAwsSdk() {
    std::call_once(aws_init_flag, []() {
        Aws::SDKOptions options;
        Aws::InitAPI(options);
        std::atexit([]() {
            Aws::SDKOptions options;
            Aws::ShutdownAPI(options);
        });
    });
}

class S3StorageProvider : public ICloudStorageProvider {
public:
    S3StorageProvider(const std::string& bucket, 
                     const std::string& region,
                     const std::string& endpoint = "")
        : bucket_(bucket), region_(region), endpoint_(endpoint) {
        
        // Initialize AWS SDK
        initializeAwsSdk();
        
        // Create S3 client configuration
        Aws::Client::ClientConfiguration config;
        config.region = region_;
        
        if (!endpoint_.empty()) {
            // For S3-compatible storage (e.g., MinIO)
            config.endpointOverride = endpoint_;
            config.scheme = Aws::Http::Scheme::HTTP;  // or HTTPS
            config.verifySSL = false;  // For self-signed certs
        }
        
        // Create S3 client
        // Uses default credential provider chain:
        // 1. Environment variables (AWS_ACCESS_KEY_ID, AWS_SECRET_ACCESS_KEY)
        // 2. AWS credentials file (~/.aws/credentials)
        // 3. IAM role (for EC2 instances)
        s3_client_ = std::make_shared<Aws::S3::S3Client>(config);
        
        THEMIS_INFO("S3StorageProvider initialized: bucket={}, region={}, endpoint={}", 
                   bucket_, region_, endpoint_.empty() ? "default" : endpoint_);
    }
    
    bool upload(const std::string& local_path, 
               const std::string& remote_path,
               const std::map<std::string, std::string>& metadata) override {
        
        if (!fs::exists(local_path)) {
            THEMIS_ERROR("Local file does not exist: {}", local_path);
            return false;
        }
        
        try {
            // Create upload request
            Aws::S3::Model::PutObjectRequest request;
            request.SetBucket(bucket_);
            request.SetKey(remote_path);
            
            // Add metadata
            for (const auto& [key, value] : metadata) {
                request.AddMetadata(key, value);
            }
            
            // Open file
            auto file_stream = Aws::MakeShared<Aws::FStream>(
                "S3Upload",
                local_path.c_str(),
                std::ios_base::in | std::ios_base::binary
            );
            
            if (!file_stream->is_open()) {
                THEMIS_ERROR("Failed to open file: {}", local_path);
                return false;
            }
            
            request.SetBody(file_stream);
            
            // Get file size for progress tracking
            file_stream->seekg(0, std::ios::end);
            auto file_size = file_stream->tellg();
            file_stream->seekg(0, std::ios::beg);
            request.SetContentLength(file_size);
            
            // Upload with retry logic
            int max_retries = 3;
            for (int attempt = 0; attempt < max_retries; ++attempt) {
                auto outcome = s3_client_->PutObject(request);
                
                if (outcome.IsSuccess()) {
                    THEMIS_INFO("S3 upload successful: {} -> s3://{}/{}", 
                               local_path, bucket_, remote_path);
                    return true;
                }
                
                auto error = outcome.GetError();
                THEMIS_WARN("S3 upload failed (attempt {}/{}): {} - {}", 
                           attempt + 1, max_retries,
                           error.GetExceptionName(), error.GetMessage());
                
                if (attempt < max_retries - 1) {
                    // Exponential backoff
                    std::this_thread::sleep_for(std::chrono::seconds(1 << attempt));
                }
            }
            
            THEMIS_ERROR("S3 upload failed after {} retries", max_retries);
            return false;
            
        } catch (const std::exception& e) {
            THEMIS_ERROR("S3 upload exception: {}", e.what());
            return false;
        }
    }
    
    bool download(const std::string& remote_path,
                 const std::string& local_path) override {
        
        try {
            // Create download request
            Aws::S3::Model::GetObjectRequest request;
            request.SetBucket(bucket_);
            request.SetKey(remote_path);
            
            // Download with retry logic
            int max_retries = 3;
            for (int attempt = 0; attempt < max_retries; ++attempt) {
                auto outcome = s3_client_->GetObject(request);
                
                if (outcome.IsSuccess()) {
                    // Save to local file
                    auto& body = outcome.GetResultWithOwnership().GetBody();
                    std::ofstream output_file(local_path, std::ios::binary);
                    
                    if (!output_file.is_open()) {
                        THEMIS_ERROR("Failed to create local file: {}", local_path);
                        return false;
                    }
                    
                    output_file << body.rdbuf();
                    output_file.close();
                    
                    THEMIS_INFO("S3 download successful: s3://{}/{} -> {}", 
                               bucket_, remote_path, local_path);
                    return true;
                }
                
                auto error = outcome.GetError();
                THEMIS_WARN("S3 download failed (attempt {}/{}): {} - {}", 
                           attempt + 1, max_retries,
                           error.GetExceptionName(), error.GetMessage());
                
                if (attempt < max_retries - 1) {
                    std::this_thread::sleep_for(std::chrono::seconds(1 << attempt));
                }
            }
            
            THEMIS_ERROR("S3 download failed after {} retries", max_retries);
            return false;
            
        } catch (const std::exception& e) {
            THEMIS_ERROR("S3 download exception: {}", e.what());
            return false;
        }
    }
    
    bool deleteObject(const std::string& remote_path) override {
        try {
            Aws::S3::Model::DeleteObjectRequest request;
            request.SetBucket(bucket_);
            request.SetKey(remote_path);
            
            auto outcome = s3_client_->DeleteObject(request);
            
            if (outcome.IsSuccess()) {
                THEMIS_INFO("S3 delete successful: s3://{}/{}", bucket_, remote_path);
                return true;
            }
            
            auto error = outcome.GetError();
            THEMIS_ERROR("S3 delete failed: {} - {}", 
                        error.GetExceptionName(), error.GetMessage());
            return false;
            
        } catch (const std::exception& e) {
            THEMIS_ERROR("S3 delete exception: {}", e.what());
            return false;
        }
    }
    
    std::vector<std::string> listObjects(const std::string& prefix) override {
        std::vector<std::string> objects;
        
        try {
            Aws::S3::Model::ListObjectsV2Request request;
            request.SetBucket(bucket_);
            request.SetPrefix(prefix);
            
            auto outcome = s3_client_->ListObjectsV2(request);
            
            if (outcome.IsSuccess()) {
                const auto& result = outcome.GetResult();
                for (const auto& object : result.GetContents()) {
                    objects.push_back(object.GetKey());
                }
                
                THEMIS_INFO("S3 list successful: found {} objects with prefix {}", 
                           objects.size(), prefix);
            } else {
                auto error = outcome.GetError();
                THEMIS_ERROR("S3 list failed: {} - {}", 
                            error.GetExceptionName(), error.GetMessage());
            }
            
        } catch (const std::exception& e) {
            THEMIS_ERROR("S3 list exception: {}", e.what());
        }
        
        return objects;
    }
    
    bool exists(const std::string& remote_path) override {
        try {
            Aws::S3::Model::HeadObjectRequest request;
            request.SetBucket(bucket_);
            request.SetKey(remote_path);
            
            auto outcome = s3_client_->HeadObject(request);
            return outcome.IsSuccess();
            
        } catch (const std::exception& e) {
            THEMIS_ERROR("S3 exists check exception: {}", e.what());
            return false;
        }
    }
    
    std::string name() const override {
        return "s3";
    }
    
private:
    std::string bucket_;
    std::string region_;
    std::string endpoint_;
    std::shared_ptr<Aws::S3::S3Client> s3_client_;
};
```

### Step 3: Multipart Upload (for large files)

```cpp
bool S3StorageProvider::uploadLargeFile(
    const std::string& local_path,
    const std::string& remote_path,
    const std::map<std::string, std::string>& metadata,
    size_t part_size = 5 * 1024 * 1024  // 5 MB default
) {
    try {
        // 1. Initiate multipart upload
        Aws::S3::Model::CreateMultipartUploadRequest init_request;
        init_request.SetBucket(bucket_);
        init_request.SetKey(remote_path);
        
        for (const auto& [key, value] : metadata) {
            init_request.AddMetadata(key, value);
        }
        
        auto init_outcome = s3_client_->CreateMultipartUpload(init_request);
        if (!init_outcome.IsSuccess()) {
            THEMIS_ERROR("Failed to initiate multipart upload");
            return false;
        }
        
        auto upload_id = init_outcome.GetResult().GetUploadId();
        
        // 2. Upload parts
        std::ifstream file(local_path, std::ios::binary);
        std::vector<Aws::S3::Model::CompletedPart> completed_parts;
        
        int part_number = 1;
        while (!file.eof()) {
            std::vector<char> buffer(part_size);
            file.read(buffer.data(), part_size);
            auto bytes_read = file.gcount();
            
            if (bytes_read == 0) break;
            
            // Upload part
            Aws::S3::Model::UploadPartRequest part_request;
            part_request.SetBucket(bucket_);
            part_request.SetKey(remote_path);
            part_request.SetUploadId(upload_id);
            part_request.SetPartNumber(part_number);
            part_request.SetContentLength(bytes_read);
            
            auto part_stream = Aws::MakeShared<Aws::StringStream>("PartStream");
            part_stream->write(buffer.data(), bytes_read);
            part_request.SetBody(part_stream);
            
            auto part_outcome = s3_client_->UploadPart(part_request);
            if (!part_outcome.IsSuccess()) {
                // Abort multipart upload
                Aws::S3::Model::AbortMultipartUploadRequest abort_request;
                abort_request.SetBucket(bucket_);
                abort_request.SetKey(remote_path);
                abort_request.SetUploadId(upload_id);
                s3_client_->AbortMultipartUpload(abort_request);
                
                THEMIS_ERROR("Failed to upload part {}", part_number);
                return false;
            }
            
            Aws::S3::Model::CompletedPart completed_part;
            completed_part.SetPartNumber(part_number);
            completed_part.SetETag(part_outcome.GetResult().GetETag());
            completed_parts.push_back(completed_part);
            
            part_number++;
        }
        
        // 3. Complete multipart upload
        Aws::S3::Model::CompletedMultipartUpload completed_upload;
        completed_upload.SetParts(completed_parts);
        
        Aws::S3::Model::CompleteMultipartUploadRequest complete_request;
        complete_request.SetBucket(bucket_);
        complete_request.SetKey(remote_path);
        complete_request.SetUploadId(upload_id);
        complete_request.SetMultipartUpload(completed_upload);
        
        auto complete_outcome = s3_client_->CompleteMultipartUpload(complete_request);
        
        if (complete_outcome.IsSuccess()) {
            THEMIS_INFO("Multipart upload completed: {} parts", completed_parts.size());
            return true;
        } else {
            THEMIS_ERROR("Failed to complete multipart upload");
            return false;
        }
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Multipart upload exception: {}", e.what());
        return false;
    }
}
```

## Azure Blob Storage Integration

### Step 1: Dependencies

**CMakeLists.txt**:
```cmake
find_package(azure-storage-cpp CONFIG REQUIRED)
target_link_libraries(themis_core PRIVATE azure-storage-cpp::azure-storage-cpp)
```

**vcpkg.json**:
```bash
vcpkg install azure-storage-cpp
```

### Step 2: SDK Implementation

**File**: `src/sharding/cloud_backup.cpp`

```cpp
#include <was/storage_account.h>
#include <was/blob.h>
#include <fstream>

class AzureStorageProvider : public ICloudStorageProvider {
public:
    AzureStorageProvider(const std::string& account_name,
                        const std::string& container)
        : account_name_(account_name), container_(container) {
        
        // Get connection string from environment
        // Format: DefaultEndpointsProtocol=https;AccountName=myaccount;AccountKey=mykey;EndpointSuffix=core.windows.net
        const char* conn_str_env = std::getenv("AZURE_STORAGE_CONNECTION_STRING");
        if (!conn_str_env) {
            THEMIS_ERROR("AZURE_STORAGE_CONNECTION_STRING not set");
            throw std::runtime_error("Azure credentials not configured");
        }
        
        // Parse connection string
        azure::storage::cloud_storage_account account = 
            azure::storage::cloud_storage_account::parse(conn_str_env);
        
        // Create blob client
        azure::storage::cloud_blob_client blob_client = account.create_cloud_blob_client();
        
        // Get container reference
        container_ = blob_client.get_container_reference(container);
        
        // Create container if it doesn't exist
        container_.create_if_not_exists();
        
        THEMIS_INFO("AzureStorageProvider initialized: account={}, container={}", 
                   account_name_, container);
    }
    
    bool upload(const std::string& local_path, 
               const std::string& remote_path,
               const std::map<std::string, std::string>& metadata) override {
        
        try {
            // Get blob reference
            azure::storage::cloud_block_blob blob = 
                container_.get_block_blob_reference(remote_path);
            
            // Set metadata
            for (const auto& [key, value] : metadata) {
                blob.metadata()[key] = value;
            }
            
            // Upload from file
            blob.upload_from_file(local_path);
            
            THEMIS_INFO("Azure upload successful: {} -> {}/{}", 
                       local_path, account_name_, remote_path);
            return true;
            
        } catch (const azure::storage::storage_exception& e) {
            THEMIS_ERROR("Azure upload failed: {}", e.what());
            return false;
        }
    }
    
    bool download(const std::string& remote_path,
                 const std::string& local_path) override {
        
        try {
            azure::storage::cloud_block_blob blob = 
                container_.get_block_blob_reference(remote_path);
            
            blob.download_to_file(local_path);
            
            THEMIS_INFO("Azure download successful: {}/{} -> {}", 
                       account_name_, remote_path, local_path);
            return true;
            
        } catch (const azure::storage::storage_exception& e) {
            THEMIS_ERROR("Azure download failed: {}", e.what());
            return false;
        }
    }
    
    bool deleteObject(const std::string& remote_path) override {
        try {
            azure::storage::cloud_block_blob blob = 
                container_.get_block_blob_reference(remote_path);
            
            blob.delete_blob();
            
            THEMIS_INFO("Azure delete successful: {}/{}", account_name_, remote_path);
            return true;
            
        } catch (const azure::storage::storage_exception& e) {
            THEMIS_ERROR("Azure delete failed: {}", e.what());
            return false;
        }
    }
    
    std::vector<std::string> listObjects(const std::string& prefix) override {
        std::vector<std::string> objects;
        
        try {
            azure::storage::list_blob_item_iterator end_iterator;
            for (auto it = container_.list_blobs(prefix); it != end_iterator; ++it) {
                if (it->is_blob()) {
                    objects.push_back(it->as_blob().name());
                }
            }
            
            THEMIS_INFO("Azure list successful: found {} objects", objects.size());
            
        } catch (const azure::storage::storage_exception& e) {
            THEMIS_ERROR("Azure list failed: {}", e.what());
        }
        
        return objects;
    }
    
    bool exists(const std::string& remote_path) override {
        try {
            azure::storage::cloud_block_blob blob = 
                container_.get_block_blob_reference(remote_path);
            
            return blob.exists();
            
        } catch (const azure::storage::storage_exception& e) {
            THEMIS_ERROR("Azure exists check failed: {}", e.what());
            return false;
        }
    }
    
    std::string name() const override {
        return "azure";
    }
    
private:
    std::string account_name_;
    azure::storage::cloud_blob_container container_;
};
```

## Google Cloud Storage Integration

### Step 1: Dependencies

**CMakeLists.txt**:
```cmake
find_package(google_cloud_cpp_storage REQUIRED)
target_link_libraries(themis_core PRIVATE google-cloud-cpp::storage)
```

**vcpkg.json**:
```bash
vcpkg install google-cloud-cpp[storage]
```

### Step 2: SDK Implementation

**File**: `src/sharding/cloud_backup.cpp`

```cpp
#include <google/cloud/storage/client.h>

namespace gcs = google::cloud::storage;

class GCSStorageProvider : public ICloudStorageProvider {
public:
    GCSStorageProvider(const std::string& project_id,
                      const std::string& bucket)
        : project_id_(project_id), bucket_(bucket) {
        
        // Create GCS client
        // Uses default credentials (GOOGLE_APPLICATION_CREDENTIALS environment variable)
        client_ = std::make_shared<gcs::Client>();
        
        THEMIS_INFO("GCSStorageProvider initialized: project={}, bucket={}", 
                   project_id_, bucket_);
    }
    
    bool upload(const std::string& local_path, 
               const std::string& remote_path,
               const std::map<std::string, std::string>& metadata) override {
        
        try {
            // Read file contents
            std::ifstream file(local_path, std::ios::binary);
            if (!file.is_open()) {
                THEMIS_ERROR("Failed to open file: {}", local_path);
                return false;
            }
            
            std::ostringstream contents;
            contents << file.rdbuf();
            
            // Upload to GCS
            auto writer = client_->WriteObject(bucket_, remote_path);
            
            // Add metadata
            for (const auto& [key, value] : metadata) {
                writer << gcs::WithObjectMetadata(
                    gcs::ObjectMetadata().upsert_metadata(key, value)
                );
            }
            
            writer << contents.str();
            writer.Close();
            
            if (writer.metadata().ok()) {
                THEMIS_INFO("GCS upload successful: {} -> gs://{}/{}", 
                           local_path, bucket_, remote_path);
                return true;
            } else {
                THEMIS_ERROR("GCS upload failed: {}", writer.metadata().status().message());
                return false;
            }
            
        } catch (const std::exception& e) {
            THEMIS_ERROR("GCS upload exception: {}", e.what());
            return false;
        }
    }
    
    bool download(const std::string& remote_path,
                 const std::string& local_path) override {
        
        try {
            auto reader = client_->ReadObject(bucket_, remote_path);
            
            if (!reader.status().ok()) {
                THEMIS_ERROR("GCS download failed: {}", reader.status().message());
                return false;
            }
            
            std::ofstream output_file(local_path, std::ios::binary);
            if (!output_file.is_open()) {
                THEMIS_ERROR("Failed to create local file: {}", local_path);
                return false;
            }
            
            output_file << reader.rdbuf();
            output_file.close();
            
            THEMIS_INFO("GCS download successful: gs://{}/{} -> {}", 
                       bucket_, remote_path, local_path);
            return true;
            
        } catch (const std::exception& e) {
            THEMIS_ERROR("GCS download exception: {}", e.what());
            return false;
        }
    }
    
    bool deleteObject(const std::string& remote_path) override {
        try {
            auto status = client_->DeleteObject(bucket_, remote_path);
            
            if (status.ok()) {
                THEMIS_INFO("GCS delete successful: gs://{}/{}", bucket_, remote_path);
                return true;
            } else {
                THEMIS_ERROR("GCS delete failed: {}", status.message());
                return false;
            }
            
        } catch (const std::exception& e) {
            THEMIS_ERROR("GCS delete exception: {}", e.what());
            return false;
        }
    }
    
    std::vector<std::string> listObjects(const std::string& prefix) override {
        std::vector<std::string> objects;
        
        try {
            for (auto&& metadata : client_->ListObjects(bucket_, gcs::Prefix(prefix))) {
                if (metadata.ok()) {
                    objects.push_back(metadata->name());
                }
            }
            
            THEMIS_INFO("GCS list successful: found {} objects", objects.size());
            
        } catch (const std::exception& e) {
            THEMIS_ERROR("GCS list exception: {}", e.what());
        }
        
        return objects;
    }
    
    bool exists(const std::string& remote_path) override {
        try {
            auto metadata = client_->GetObjectMetadata(bucket_, remote_path);
            return metadata.ok();
            
        } catch (const std::exception& e) {
            THEMIS_ERROR("GCS exists check exception: {}", e.what());
            return false;
        }
    }
    
    std::string name() const override {
        return "gcs";
    }
    
private:
    std::string project_id_;
    std::string bucket_;
    std::shared_ptr<gcs::Client> client_;
};
```

## Credential Management

### Environment Variables

**AWS S3**:
```bash
export AWS_ACCESS_KEY_ID="your_access_key"
export AWS_SECRET_ACCESS_KEY="your_secret_key"
export AWS_DEFAULT_REGION="us-east-1"
```

**Azure**:
```bash
export AZURE_STORAGE_CONNECTION_STRING="DefaultEndpointsProtocol=https;AccountName=...;AccountKey=...;"
```

**GCS**:
```bash
export GOOGLE_APPLICATION_CREDENTIALS="/path/to/service-account-key.json"
```

### Configuration File

**config/cloud_backup.yaml**:
```yaml
cloud_backup:
  provider: "s3"  # or "azure" or "gcs"
  
  s3:
    bucket: "themisdb-backups"
    region: "us-east-1"
    endpoint: ""  # For S3-compatible (MinIO, etc.)
  
  azure:
    account: "themisdbstorage"
    container: "backups"
  
  gcs:
    project_id: "themisdb-project"
    bucket: "themisdb-backups"
  
  local_backup_dir: "/var/lib/themisdb/backups"
  enable_compression: true
  enable_encryption: true
  max_backups: 30
  retention_days: 90
```

## Testing Strategy

### Unit Tests
1. **Mock provider tests**: Test without real cloud SDKs
2. **Credential validation**: Test auth failures
3. **Retry logic**: Test transient failures
4. **Error handling**: Test various error conditions

### Integration Tests (require real cloud accounts)
1. **End-to-end upload/download**: Full backup cycle
2. **Large file handling**: Multipart uploads
3. **Concurrent operations**: Thread safety
4. **Cross-region replication**: Multi-datacenter

### CI/CD Integration
Use mock mode in CI, real integration tests in staging environment.

## Migration from Mock Mode

```cpp
// Before (mock mode)
setenv("THEMIS_CLOUD_BACKUP_MOCK", "1", 1);

// After (real SDK)
// Remove mock mode, ensure credentials configured
// unsetenv("THEMIS_CLOUD_BACKUP_MOCK");
```

## Performance Considerations

1. **Parallel uploads**: Use thread pool for concurrent uploads
2. **Compression**: Reduce transfer size and costs
3. **Connection pooling**: Reuse HTTP connections
4. **Retry with exponential backoff**: Handle transient failures
5. **Progress callbacks**: Track large file transfers

## Cost Optimization

1. **Compression**: Save 30-50% on storage costs
2. **Lifecycle policies**: Auto-delete old backups
3. **Storage classes**: Use cheaper cold storage for old backups
4. **Regional selection**: Choose cost-effective regions

## Implementation Checklist

- [ ] Add AWS SDK dependency to CMakeLists.txt
- [ ] Add Azure SDK dependency to CMakeLists.txt
- [ ] Add GCS SDK dependency to CMakeLists.txt
- [ ] Implement real S3StorageProvider::upload
- [ ] Implement real S3StorageProvider::download
- [ ] Implement S3 multipart upload for large files
- [ ] Implement real AzureStorageProvider::upload
- [ ] Implement real AzureStorageProvider::download
- [ ] Implement real GCSStorageProvider::upload
- [ ] Implement real GCSStorageProvider::download
- [ ] Add retry logic for all providers
- [ ] Add error handling for all providers
- [ ] Create integration tests with real cloud accounts
- [ ] Add progress tracking for large transfers
- [ ] Document credential configuration
- [ ] Add monitoring and metrics
- [ ] Create migration guide from mock mode

## Estimated Timeline

- **Week 1**: AWS S3 integration and testing
- **Week 2**: Azure and GCS integration
- **Week 3**: Integration testing, documentation, and optimization

---

**Created**: February 7, 2026  
**For**: ThemisDB v1.4.0  
**Status**: Implementation Guide
