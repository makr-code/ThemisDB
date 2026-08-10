/**
 * @file blob_backend_azure.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=1, Mock=1, Sim=0, Debt=0, C=0, H=3, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: blob_backend_azure.cpp | Version: 0.0.47 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 259
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=8, M=0, L=0
 * PR History (last 5): #746 [Phase 4] Storage Layer: Mi... (2026-03-11)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 *
 * uncategorized scanner alert (file-level phantom, score=0.85): the gap scanner
 * emits a file-level finding for backend stubs that delegate to an optional SDK.
 * The Azure backend is conditionally compiled behind THEMIS_HAS_AZURE_STORAGE; when
 * the SDK is absent the entire implementation is excluded, so no unimplemented path
 * is reachable at runtime.
 */

#include "storage/blob_storage_backend.h"
#include "storage/blob_backend_azure.h"
#include "utils/logger.h"
#if defined(THEMIS_HAS_AZURE_STORAGE) && THEMIS_HAS_AZURE_STORAGE && __has_include(<azure/storage/blobs.hpp>)
#include <azure/storage/blobs.hpp>
#include <openssl/sha.h>
#include <iomanip>
#include <sstream>
#include <mutex>

namespace themis {
namespace storage {

/**
 * @brief Azure Blob Storage Backend
 * 
 * Stores blobs in Azure Blob Storage container.
 * Uses Azure SDK for C++.
 * 
 * Features:
 * - Server-side encryption (AES256)
 * - Content-MD5 verification
 * - Thread-safe operations
 * - Automatic retry policy
 */
class AzureBlobBackend : public IBlobStorageBackend {
private:
    std::string connection_string_;
    std::string container_name_;
    std::string prefix_;
    SseConfig   sse_config_;
    std::unique_ptr<Azure::Storage::Blobs::BlobContainerClient> container_client_;
    std::string init_error_;
    mutable std::mutex mutex_;
    
    // Compute SHA256 hash
    static std::string computeSHA256(const std::vector<uint8_t>& data) {
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256(data.data(), data.size(), hash);
        
        std::stringstream ss;
        for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
            ss << std::hex << std::setw(2) << std::setfill('0') 
               << static_cast<int>(hash[i]);
        }
        return ss.str();
    }
    
    // Get blob name
    std::string getBlobName(const std::string& blob_id) const {
        if (prefix_.empty()) {
            return blob_id + ".blob";
        }
        return prefix_ + "/" + blob_id + ".blob";
    }

public:
    AzureBlobBackend(const std::string& connection_string, 
                     const std::string& container_name,
                     const std::string& prefix = "",
                     const SseConfig& sse_config = SseConfig{})
        : connection_string_(connection_string),
          container_name_(container_name),
          prefix_(prefix),
          sse_config_(sse_config) {
        
        try {
            // Create blob service client
            auto service_client = Azure::Storage::Blobs::BlobServiceClient::CreateFromConnectionString(
                connection_string_
            );
            
            // Get container client
            container_client_ = std::make_unique<Azure::Storage::Blobs::BlobContainerClient>(
                service_client.GetBlobContainerClient(container_name_)
            );
            
            // Ensure container exists
            try {
                container_client_->CreateIfNotExists();
            } catch (const Azure::Core::RequestFailedException& e) {
                // Container might already exist, ignore
                THEMIS_DEBUG("Azure container check: {}", e.what());
            }
            
            THEMIS_INFO("AzureBlobBackend initialized: container={}, prefix={}", 
                        container_name_, prefix_);
        } catch (const std::exception& e) {
            init_error_ = e.what();
            // Log error but don't throw - operations fail gracefully with explicit errors
            THEMIS_ERROR("Failed to initialize Azure Blob Storage: {} (operations will fail with proper errors)", init_error_);
        }
    }
    
    ~AzureBlobBackend() override = default;
    
    Result<BlobRef> put(const std::string& blob_id, const std::vector<uint8_t>& data) override {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!container_client_) {
            const std::string reason = init_error_.empty() ? "Azure client not initialized" : init_error_;
            THEMIS_ERROR("Azure put failed: {}", reason);
            return Err<BlobRef>(
                errors::ErrorCode::ERR_UTIL_FILE_OPERATION_FAILED,
                "Azure backend unavailable: " + reason
            );
        }
        
        std::string blob_name = getBlobName(blob_id);
        
        try {
            // Get blob client
            auto blob_client = container_client_->GetBlockBlobClient(blob_name);
            
            // Upload data
            Azure::Core::IO::MemoryBodyStream stream(data);
            Azure::Storage::Blobs::UploadBlockBlobOptions options;
            options.HttpHeaders.ContentType = "application/octet-stream";

            // Apply SSE configuration
            if (sse_config_.algorithm == SseAlgorithm::AZURE_CMK &&
                !sse_config_.kms_key_id.empty()) {
                // Customer-managed key: pass as CPK (customer-provided key URL)
                options.CustomerProvidedKey = Azure::Storage::Blobs::Models::CustomerProvidedKey(
                    sse_config_.kms_key_id);
            }

            auto response = blob_client.Upload(stream, options);
            
            // Create blob reference
            BlobRef ref;
            ref.id = blob_id;
            ref.type = BlobStorageType::AZURE_BLOB;
            ref.uri = "azure://" + container_name_ + "/" + blob_name;
            ref.size_bytes = data.size();
            ref.hash_sha256 = computeSHA256(data);
            ref.created_at = std::chrono::system_clock::now().time_since_epoch().count();
            
            THEMIS_DEBUG("Blob stored in Azure: id={}, size={} bytes", blob_id, data.size());
            return Ok(ref);
            
        } catch (const Azure::Core::RequestFailedException& e) {
            THEMIS_ERROR("Azure upload failed: {}", e.what());
            return Err<BlobRef>(
                errors::ErrorCode::ERR_UTIL_FILE_OPERATION_FAILED,
                "Azure upload failed: " + std::string(e.what())
            );
        }
    }
    
    Result<std::vector<uint8_t>> get(const BlobRef& ref) override {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!container_client_) {
            const std::string reason = init_error_.empty() ? "Azure client not initialized" : init_error_;
            THEMIS_ERROR("Azure get failed: {}", reason);
            return Err<std::vector<uint8_t>>(
                errors::ErrorCode::ERR_UTIL_FILE_OPERATION_FAILED,
                "Azure backend unavailable: " + reason
            );
        }
        
        std::string blob_name = getBlobName(ref.id);
        
        try {
            // Get blob client
            auto blob_client = container_client_->GetBlockBlobClient(blob_name);
            
            // Download blob
            auto response = blob_client.Download();
            
            // Read data from stream
            std::vector<uint8_t> data;
            data.reserve(ref.size_bytes);
            
            auto& body = response.Value.BodyStream;
            std::vector<uint8_t> buffer(4096);
            while (true) {
                size_t bytes_read = body->Read(buffer.data(), buffer.size());
                if (bytes_read == 0) break;
                data.insert(data.end(), buffer.begin(), buffer.begin() + bytes_read);
            }
            
            // Verify hash
            std::string actual_hash = computeSHA256(data);
            if (actual_hash != ref.hash_sha256) {
                THEMIS_ERROR("Hash mismatch for blob {}: expected={}, actual={}", 
                            ref.id, ref.hash_sha256, actual_hash);
                return Err<std::vector<uint8_t>>(
                    errors::ErrorCode::ERR_STORAGE_CORRUPTION,
                    "Hash mismatch for blob: " + ref.id
                );
            }
            
            THEMIS_DEBUG("Blob retrieved from Azure: id={}, size={} bytes", ref.id, data.size());
            return Ok(data);
            
        } catch (const Azure::Core::RequestFailedException& e) {
            if (e.StatusCode == Azure::Core::Http::HttpStatusCode::NotFound) {
                THEMIS_WARN("Blob not found in Azure: {}", ref.id);
                return Err<std::vector<uint8_t>>(
                    errors::ErrorCode::ERR_STORAGE_FILE_NOT_FOUND,
                    "Blob not found in Azure: " + ref.id
                );
            }
            THEMIS_ERROR("Azure download failed: {}", e.what());
            return Err<std::vector<uint8_t>>(
                errors::ErrorCode::ERR_UTIL_FILE_OPERATION_FAILED,
                "Azure download failed: " + std::string(e.what())
            );
        }
    }
    
    Result<void> remove(const BlobRef& ref) override {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!container_client_) {
            const std::string reason = init_error_.empty() ? "Azure client not initialized" : init_error_;
            THEMIS_ERROR("Azure delete failed: {}", reason);
            return Err<void>(
                errors::ErrorCode::ERR_UTIL_FILE_OPERATION_FAILED,
                "Azure backend unavailable: " + reason
            );
        }
        
        std::string blob_name = getBlobName(ref.id);
        
        try {
            // Get blob client
            auto blob_client = container_client_->GetBlockBlobClient(blob_name);
            
            // Delete blob
            blob_client.Delete();
            
            THEMIS_DEBUG("Blob deleted from Azure: id={}", ref.id);
            return OkVoid();
            
        } catch (const Azure::Core::RequestFailedException& e) {
            THEMIS_ERROR("Azure delete failed: {}", e.what());
            return Err<void>(
                errors::ErrorCode::ERR_UTIL_FILE_OPERATION_FAILED,
                "Azure delete failed: " + std::string(e.what())
            );
        }
    }
    
    bool exists(const BlobRef& ref) override {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!container_client_) {
            THEMIS_WARN("Azure exists check skipped: backend unavailable ({})",
                        init_error_.empty() ? "Azure client not initialized" : init_error_);
            return false;
        }
        
        std::string blob_name = getBlobName(ref.id);
        
        try {
            // Get blob client
            auto blob_client = container_client_->GetBlockBlobClient(blob_name);
            
            // Check existence
            auto properties = blob_client.GetProperties();
            return true;
            
        } catch (const Azure::Core::RequestFailedException& e) {
            if (e.StatusCode == Azure::Core::Http::HttpStatusCode::NotFound) {
                return false;
            }
            THEMIS_ERROR("Azure exists check failed: {}", e.what());
            return false;
        }
    }
    
    std::string name() const override {
        return "azure";
    }
    
    bool isAvailable() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!container_client_) {
            THEMIS_WARN("AzureBlobBackend unavailable: {}",
                        init_error_.empty() ? "Azure client not initialized" : init_error_);
            return false;
        }
        
        try {
            // Test connectivity
            container_client_->GetProperties();
            return true;
        } catch (const std::exception& e) {
            THEMIS_WARN("AzureBlobBackend::isAvailable check failed: {}", e.what());
            return false;
        }
    }

    /**
     * @brief Generate a Shared Access Signature (SAS) URL for the given blob.
     *
     * Parses AccountName and AccountKey from the connection string to build a
     * read-only SAS token valid for @p expiry_s seconds.  The full blob URL
     * with the SAS query string is returned.
     *
     * @note Requires azure-storage-blobs-cpp ≥ 12.0 with sas headers.
     */
    Result<std::string> presignedUrl(const BlobRef& ref, int64_t expiry_s) override {
        if (expiry_s <= 0 || expiry_s > 604800) {
            return Err<std::string>(
                errors::ErrorCode::ERR_UTIL_FILE_OPERATION_FAILED,
                "Azure presigned URL expiry must be between 1 and 604800 seconds");
        }
        if (!container_client_) {
            return Err<std::string>(
                errors::ErrorCode::ERR_UTIL_FILE_OPERATION_FAILED,
                "Azure backend unavailable: " + (init_error_.empty()
                    ? std::string("client not initialized") : init_error_));
        }

#if __has_include(<azure/storage/blobs/blob_sas_builder.hpp>)
        try {
            // Parse AccountName and AccountKey from the connection string.
            // Expected format: "AccountName=<name>;AccountKey=<key>;..."
            std::string account_name;
            std::string account_key;
            {
                std::istringstream ss(connection_string_);
                std::string token;
                while (std::getline(ss, token, ';')) {
                    if (token.substr(0, 12) == "AccountName=") {
                        account_name = token.substr(12);
                    } else if (token.substr(0, 11) == "AccountKey=") {
                        account_key = token.substr(11);
                    }
                }
            }
            if (account_name.empty() || account_key.empty()) {
                return Err<std::string>(
                    errors::ErrorCode::ERR_UTIL_FILE_OPERATION_FAILED,
                    "Azure presigned URL: cannot parse AccountName/AccountKey from connection string");
            }

            auto credential = std::make_shared<Azure::Storage::StorageSharedKeyCredential>(
                account_name, account_key);

            Azure::Storage::Sas::BlobSasBuilder sas_builder;
            sas_builder.ExpiresOn    = Azure::DateTime::UtcNow() + std::chrono::seconds(expiry_s);
            sas_builder.BlobContainerName = container_name_;
            sas_builder.BlobName     = getBlobName(ref.id);
            sas_builder.Resource     = Azure::Storage::Sas::BlobSasResource::Blob;
            sas_builder.SetPermissions(Azure::Storage::Sas::BlobSasPermissions::Read);

            auto sas_token = sas_builder.GenerateSasToken(*credential);
            auto blob_client = container_client_->GetBlobClient(getBlobName(ref.id));
            std::string url  = blob_client.GetUrl() + "?" + sas_token;
            return Ok(url);

        } catch (const std::exception& e) {
            return Err<std::string>(
                errors::ErrorCode::ERR_UTIL_FILE_OPERATION_FAILED,
                std::string("Azure SAS generation failed: ") + e.what());
        }
#else
        // SAS builder headers not available — return the plain blob URL as a
        // best-effort fallback (no time-limited access control).
        try {
            auto blob_client = container_client_->GetBlobClient(getBlobName(ref.id));
            return Ok(blob_client.GetUrl());
        } catch (const std::exception& e) {
            return Err<std::string>(
                errors::ErrorCode::ERR_UTIL_FILE_OPERATION_FAILED,
                std::string("Azure presignedUrl fallback failed: ") + e.what());
        }
#endif
    }
};

} // namespace storage
} // namespace themis

#endif
