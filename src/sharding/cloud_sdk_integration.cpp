/**
 * @file cloud_sdk_integration.cpp
 * @brief Real cloud SDK implementations for S3, Azure Storage, and Google Cloud Storage
 * @version 0.0.15
 *
 * This file provides real SDK implementations that can be registered as callbacks
 * with ThemisDB's cloud backup system. Each provider is conditionally compiled
 * based on availability of the respective cloud SDK.
 *
 * Build flags:
 * - THEMIS_WITH_S3_SDK: Enable AWS SDK for C++ integration
 * - THEMIS_WITH_AZURE_SDK: Enable Azure Storage Blobs SDK integration
 * - THEMIS_WITH_GCS_SDK: Enable Google Cloud C++ Client Libraries integration
 */

#include "sharding/cloud_sdk_integration.h"

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string_view>
#include <vector>

#include "utils/logger.h"

namespace fs = std::filesystem;

namespace themis {
namespace sharding {

namespace {
bool validateFixedProviderArgument(std::string_view provider, std::string_view field_name,
                                   const std::string &expected, const std::string &actual) {
    if (expected != actual) {
        THEMIS_ERROR("{} callback rejected mismatched {}: expected='{}' actual='{}'",
                     provider, field_name, expected, actual);
        return false;
    }
    return true;
}
}

// ============================================================================
// AWS S3 Implementation
// ============================================================================

#ifdef THEMIS_WITH_S3_SDK

#include <aws/core/auth/AWSCredentialsProvider.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/DeleteObjectRequest.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/s3/model/HeadObjectRequest.h>
#include <aws/s3/model/ListObjectsV2Request.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <fstream>

namespace {
std::shared_ptr<Aws::S3::S3Client> g_s3_client;
}

bool initializeS3Provider(const std::string &region, const std::string &bucket, const std::string &endpoint) {
    if (region.empty() || bucket.empty()) {
        THEMIS_ERROR("S3 provider initialization requires non-empty region and bucket");
        return false;
    }
    try {
        // Initialize AWS SDK
        Aws::Client::ClientConfiguration client_config;
        client_config.region = region;

        if (!endpoint.empty()) {
            client_config.endpointOverride = endpoint;
        }

        g_s3_client = std::make_shared<Aws::S3::S3Client>(client_config);

        // Register S3 upload callback
        setS3UploadFn([bucket](const std::string &callback_bucket, const std::string &local_path,
                               const std::string &remote_path,
                               const std::map<std::string, std::string> &metadata) -> bool {
            if (!validateFixedProviderArgument("S3", "bucket", bucket, callback_bucket)) {
                return false;
            }
            if (!g_s3_client) {
                THEMIS_ERROR("S3 upload: client not initialized");
                return false;
            }

            if (local_path.empty() || remote_path.empty()) {
                THEMIS_ERROR("S3 upload: empty local or remote path");
                return false;
            }

            if (!fs::exists(local_path)) {
                THEMIS_ERROR("S3 upload: local file not found: {}", local_path);
                return false;
            }

            try {
                Aws::S3::Model::PutObjectRequest request;
                request.SetBucket(bucket);
                request.SetKey(remote_path);

                // Add metadata as object headers
                for (const auto &[key, value] : metadata) {
                    request.AddMetadata(key, value);
                }

                auto input_data = Aws::MakeShared<Aws::FStream>("S3Upload", local_path.c_str(),
                                                                std::ios_base::in | std::ios_base::binary);

                if (!input_data->good()) {
                    THEMIS_ERROR("S3 upload: failed to open file: {}", local_path);
                    return false;
                }

                request.SetBody(input_data);

                auto response = g_s3_client->PutObject(request);

                if (response.IsSuccess()) {
                    THEMIS_INFO("S3 upload completed: s3://{}/{}", bucket, remote_path);
                    return true;
                } else {
                    THEMIS_ERROR("S3 upload failed: {} (Error: {})", remote_path, response.GetError().GetMessage());
                    return false;
                }
            } catch (const std::exception &e) {
                THEMIS_ERROR("S3 upload exception: {}", e.what());
                return false;
            }
        });

        // Register S3 download callback
        setS3DownloadFn([bucket](const std::string &callback_bucket, const std::string &remote_path,
                                 const std::string &local_path) -> bool {
            if (!validateFixedProviderArgument("S3", "bucket", bucket, callback_bucket)) {
                return false;
            }
            if (!g_s3_client) {
                THEMIS_ERROR("S3 download: client not initialized");
                return false;
            }

            if (remote_path.empty() || local_path.empty()) {
                THEMIS_ERROR("S3 download: empty remote or local path");
                return false;
            }

            try {
                Aws::S3::Model::GetObjectRequest request;
                request.SetBucket(bucket);
                request.SetKey(remote_path);

                auto response = g_s3_client->GetObject(request);

                if (!response.IsSuccess()) {
                    THEMIS_ERROR("S3 download failed: s3://{}/{} (Error: {})", bucket, remote_path,
                                 response.GetError().GetMessage());
                    return false;
                }

                // Write to local file
                std::ofstream output_file(local_path, std::ios::binary);
                if (!output_file) {
                    THEMIS_ERROR("S3 download: failed to open output file: {}", local_path);
                    return false;
                }

                output_file << response.GetResult().GetBody().rdbuf();
                output_file.close();

                THEMIS_INFO("S3 download completed: s3://{}/{} -> {}", bucket, remote_path, local_path);
                return true;
            } catch (const std::exception &e) {
                THEMIS_ERROR("S3 download exception: {}", e.what());
                return false;
            }
        });

        // Register S3 delete callback
        setS3DeleteFn([bucket](const std::string &callback_bucket, const std::string &remote_path) -> bool {
            if (!validateFixedProviderArgument("S3", "bucket", bucket, callback_bucket)) {
                return false;
            }
            if (!g_s3_client) {
                THEMIS_ERROR("S3 delete: client not initialized");
                return false;
            }

            if (remote_path.empty()) {
                THEMIS_ERROR("S3 delete: empty remote path");
                return false;
            }

            try {
                Aws::S3::Model::DeleteObjectRequest request;
                request.SetBucket(bucket);
                request.SetKey(remote_path);

                auto response = g_s3_client->DeleteObject(request);

                if (response.IsSuccess()) {
                    THEMIS_INFO("S3 delete completed: s3://{}/{}", bucket, remote_path);
                    return true;
                } else {
                    THEMIS_ERROR("S3 delete failed: {} (Error: {})", remote_path, response.GetError().GetMessage());
                    return false;
                }
            } catch (const std::exception &e) {
                THEMIS_ERROR("S3 delete exception: {}", e.what());
                return false;
            }
        });

        // Register S3 list callback
        setS3ListFn([bucket](const std::string &callback_bucket, const std::string &prefix)
                    -> std::vector<std::string> {
            std::vector<std::string> results = {};

            if (!validateFixedProviderArgument("S3", "bucket", bucket, callback_bucket)) {
                return results;
            }

            if (!g_s3_client) {
                THEMIS_ERROR("S3 list: client not initialized");
                return results;
            }

            try {
                Aws::S3::Model::ListObjectsV2Request request;
                request.SetBucket(bucket);
                if (!prefix.empty()) {
                    request.SetPrefix(prefix);
                }

                auto response = g_s3_client->ListObjectsV2(request);

                if (!response.IsSuccess()) {
                    THEMIS_ERROR("S3 list failed: s3://{}/{} (Error: {})", bucket, prefix,
                                 response.GetError().GetMessage());
                    return results;
                }

                for (const auto &object : response.GetResult().GetContents()) {
                    results.push_back(object.GetKey());
                }

                THEMIS_INFO("S3 list completed: s3://{}/{} (found {} objects)", bucket, prefix, results.size());
                return results;
            } catch (const std::exception &e) {
                THEMIS_ERROR("S3 list exception: {}", e.what());
                return results;
            }
        });

        // Register S3 exists callback
        setS3ExistsFn([bucket](const std::string &callback_bucket, const std::string &remote_path) -> bool {
            if (!validateFixedProviderArgument("S3", "bucket", bucket, callback_bucket)) {
                return false;
            }
            if (!g_s3_client) {
                THEMIS_ERROR("S3 exists: client not initialized");
                return false;
            }

            if (remote_path.empty()) {
                THEMIS_ERROR("S3 exists: empty remote path");
                return false;
            }

            try {
                Aws::S3::Model::HeadObjectRequest request;
                request.SetBucket(bucket);
                request.SetKey(remote_path);

                auto response = g_s3_client->HeadObject(request);

                bool exists = response.IsSuccess();
                if (!exists && response.GetError().GetErrorType() != Aws::S3::S3Errors::NO_SUCH_KEY) {
                    THEMIS_WARN("S3 exists check failed: {} (Error: {})", remote_path,
                                response.GetError().GetMessage());
                }

                THEMIS_INFO("S3 exists check: s3://{}/{} = {}", bucket, remote_path, exists ? "yes" : "no");
                return exists;
            } catch (const std::exception &e) {
                THEMIS_ERROR("S3 exists exception: {}", e.what());
                return false;
            }
        });

        THEMIS_INFO("S3 provider initialized: region={}, bucket={}, endpoint={}", region, bucket,
                    endpoint.empty() ? "default" : endpoint);
        return true;
    } catch (const std::exception &e) {
        THEMIS_ERROR("S3 provider initialization failed: {}", e.what());
        return false;
    }
}

bool isS3ProviderAvailable() {
    return true; // AWS SDK is available
}

#else

bool initializeS3Provider(const std::string &region, const std::string &bucket, const std::string &endpoint) {
    THEMIS_WARN("S3 provider not available: AWS SDK for C++ not linked (THEMIS_WITH_S3_SDK not set)");
    return false;
}

bool isS3ProviderAvailable() {
    return false;
}

#endif // THEMIS_WITH_S3_SDK

// ============================================================================
// Azure Blob Storage Implementation
// ============================================================================

#ifdef THEMIS_WITH_AZURE_SDK

#include <azure/identity.hpp>
#include <azure/storage/blobs.hpp>
#include <fstream>

namespace {
std::shared_ptr<Azure::Storage::Blobs::BlobContainerClient> g_azure_container_client;
}

bool initializeAzureProvider(const std::string &account_name, const std::string &container,
                             const std::string &connection_string) {
    if (account_name.empty() || container.empty()) {
        THEMIS_ERROR("Azure provider initialization requires non-empty account_name and container");
        return false;
    }
    try {
        // Initialize Azure Blob Storage client
        if (!connection_string.empty()) {
            g_azure_container_client = std::make_shared<Azure::Storage::Blobs::BlobContainerClient>(
                Azure::Storage::Blobs::BlobContainerClient::CreateFromConnectionString(connection_string, container));
        } else {
            // Use default credentials (environment variables, managed identity, etc.)
            std::string blob_uri = "https://" + account_name + ".blob.core.windows.net";
            auto credential      = std::make_shared<Azure::Identity::DefaultAzureCredential>();
            g_azure_container_client
                = std::make_shared<Azure::Storage::Blobs::BlobContainerClient>(blob_uri, container, credential);
        }

        // Register Azure upload callback
        setAzureUploadFn([account_name, container](const std::string &callback_account,
                                                   const std::string &callback_container,
                                                   const std::string &local_path, const std::string &remote_path,
                                                   const std::map<std::string, std::string> &metadata) -> bool {
            if (!validateFixedProviderArgument("Azure", "account", account_name, callback_account) ||
                !validateFixedProviderArgument("Azure", "container", container, callback_container)) {
                return false;
            }
            if (!g_azure_container_client) {
                THEMIS_ERROR("Azure upload: client not initialized");
                return false;
            }

            if (local_path.empty() || remote_path.empty()) {
                THEMIS_ERROR("Azure upload: empty local or remote path");
                return false;
            }

            if (!fs::exists(local_path)) {
                THEMIS_ERROR("Azure upload: local file not found: {}", local_path);
                return false;
            }

            try {
                auto blob_client = g_azure_container_client->GetBlobClient(remote_path);
                std::ifstream input_file(local_path, std::ios::binary);
                if (!input_file) {
                    THEMIS_ERROR("Azure upload: failed to open file: {}", local_path);
                    return false;
                }

                // Upload blob
                blob_client.Upload(input_file, true);

                // Set metadata if provided
                if (!metadata.empty()) {
                    Azure::Storage::Blobs::SetBlobMetadataOptions options;
                    for (const auto &[key, value] : metadata) {
                        options.Metadata[key] = value;
                    }
                    blob_client.SetMetadata(options.Metadata);
                }

                THEMIS_INFO("Azure upload completed: {}/{}/{}", account_name, container, remote_path);
                return true;
            } catch (const Azure::Core::RequestFailedException &e) {
                THEMIS_ERROR("Azure upload failed: {} (HTTP {})", remote_path, e.StatusCode);
                return false;
            } catch (const std::exception &e) {
                THEMIS_ERROR("Azure upload exception: {}", e.what());
                return false;
            }
        });

        // Register Azure download callback
        setAzureDownloadFn(
            [account_name, container](const std::string &callback_account, const std::string &callback_container,
                                      const std::string &remote_path, const std::string &local_path) -> bool {
                if (!validateFixedProviderArgument("Azure", "account", account_name, callback_account) ||
                    !validateFixedProviderArgument("Azure", "container", container, callback_container)) {
                    return false;
                }
                if (!g_azure_container_client) {
                    THEMIS_ERROR("Azure download: client not initialized");
                    return false;
                }

                if (remote_path.empty() || local_path.empty()) {
                    THEMIS_ERROR("Azure download: empty remote or local path");
                    return false;
                }

                try {
                    auto blob_client = g_azure_container_client->GetBlobClient(remote_path);

                    // Download blob
                    auto download = blob_client.Download();

                    std::ofstream output_file(local_path, std::ios::binary);
                    if (!output_file) {
                        THEMIS_ERROR("Azure download: failed to open output file: {}", local_path);
                        return false;
                    }

                    std::vector<uint8_t> buffer(8192);
                    size_t bytes_read = 0;
                    while ((bytes_read
                            = download.BodyStream.read(reinterpret_cast<char *>(buffer.data()), buffer.size()).gcount())
                           > 0) {
                        output_file.write(reinterpret_cast<const char *>(buffer.data()), bytes_read);
                    }
                    output_file.close();

                    THEMIS_INFO("Azure download completed: {}/{}/{} -> {}", account_name, container, remote_path,
                                local_path);
                    return true;
                } catch (const Azure::Core::RequestFailedException &e) {
                    THEMIS_ERROR("Azure download failed: {} (HTTP {})", remote_path, e.StatusCode);
                    return false;
                } catch (const std::exception &e) {
                    THEMIS_ERROR("Azure download exception: {}", e.what());
                    return false;
                }
            });

        // Register Azure delete callback
        setAzureDeleteFn([account_name, container](const std::string &callback_account,
                                                   const std::string &callback_container,
                                                   const std::string &remote_path) -> bool {
            if (!validateFixedProviderArgument("Azure", "account", account_name, callback_account) ||
                !validateFixedProviderArgument("Azure", "container", container, callback_container)) {
                return false;
            }
            if (!g_azure_container_client) {
                THEMIS_ERROR("Azure delete: client not initialized");
                return false;
            }

            if (remote_path.empty()) {
                THEMIS_ERROR("Azure delete: empty remote path");
                return false;
            }

            try {
                auto blob_client = g_azure_container_client->GetBlobClient(remote_path);
                blob_client.Delete();

                THEMIS_INFO("Azure delete completed: {}/{}/{}", account_name, container, remote_path);
                return true;
            } catch (const Azure::Core::RequestFailedException &e) {
                THEMIS_ERROR("Azure delete failed: {} (HTTP {})", remote_path, e.StatusCode);
                return false;
            } catch (const std::exception &e) {
                THEMIS_ERROR("Azure delete exception: {}", e.what());
                return false;
            }
        });

        // Register Azure list callback
        setAzureListFn([account_name, container](const std::string &callback_account,
                                                 const std::string &callback_container,
                                                 const std::string &prefix) -> std::vector<std::string> {
            std::vector<std::string> results = {};

            if (!validateFixedProviderArgument("Azure", "account", account_name, callback_account) ||
                !validateFixedProviderArgument("Azure", "container", container, callback_container)) {
                return results;
            }

            if (!g_azure_container_client) {
                THEMIS_ERROR("Azure list: client not initialized");
                return results;
            }

            try {
                Azure::Storage::Blobs::ListBlobsOptions options;
                if (!prefix.empty()) {
                    options.Prefix = prefix;
                }

                auto pageable = g_azure_container_client->ListBlobs(options);
                for (auto page = pageable.begin(); page != pageable.end(); ++page) {
                    for (const auto &blob : page->Blobs) {
                        results.push_back(blob.Name);
                    }
                }

                THEMIS_INFO("Azure list completed: {}/{}/{} (found {} blobs)", account_name, container, prefix,
                            results.size());
                return results;
            } catch (const Azure::Core::RequestFailedException &e) {
                THEMIS_ERROR("Azure list failed: {}/{}/{} (HTTP {})", account_name, container, prefix, e.StatusCode);
                return results;
            } catch (const std::exception &e) {
                THEMIS_ERROR("Azure list exception: {}", e.what());
                return results;
            }
        });

        // Register Azure exists callback
        setAzureExistsFn([account_name, container](const std::string &callback_account,
                                                   const std::string &callback_container,
                                                   const std::string &remote_path) -> bool {
            if (!validateFixedProviderArgument("Azure", "account", account_name, callback_account) ||
                !validateFixedProviderArgument("Azure", "container", container, callback_container)) {
                return false;
            }
            if (!g_azure_container_client) {
                THEMIS_ERROR("Azure exists: client not initialized");
                return false;
            }

            if (remote_path.empty()) {
                THEMIS_ERROR("Azure exists: empty remote path");
                return false;
            }

            try {
                auto blob_client = g_azure_container_client->GetBlobClient(remote_path);
                bool exists      = blob_client.Exists().Value;

                THEMIS_INFO("Azure exists check: {}/{}/{} = {}", account_name, container, remote_path,
                            exists ? "yes" : "no");
                return exists;
            } catch (const Azure::Core::RequestFailedException &e) {
                THEMIS_WARN("Azure exists check failed: {}/{}/{} (HTTP {})", account_name, container, remote_path,
                            e.StatusCode);
                return false;
            } catch (const std::exception &e) {
                THEMIS_ERROR("Azure exists exception: {}", e.what());
                return false;
            }
        });

        THEMIS_INFO("Azure provider initialized: account={}, container={}", account_name, container);
        return true;
    } catch (const std::exception &e) {
        THEMIS_ERROR("Azure provider initialization failed: {}", e.what());
        return false;
    }
}

bool isAzureProviderAvailable() {
    return true; // Azure SDK is available
}

#else

bool initializeAzureProvider(const std::string &account_name, const std::string &container,
                             const std::string &connection_string) {
    THEMIS_WARN("Azure provider not available: Azure Storage SDK not linked (THEMIS_WITH_AZURE_SDK not set)");
    return false;
}

bool isAzureProviderAvailable() {
    return false;
}

#endif // THEMIS_WITH_AZURE_SDK

// ============================================================================
// Google Cloud Storage Implementation
// ============================================================================

#ifdef THEMIS_WITH_GCS_SDK

#include <fstream>
#include <google/cloud/storage/client.h>
#include <google/cloud/storage/object_metadata.h>

namespace {
std::shared_ptr<google::cloud::storage::Client> g_gcs_client;
}

bool initializeGCSProvider(const std::string &project_id, const std::string &bucket,
                           const std::string &credentials_file) {
    if (project_id.empty() || bucket.empty()) {
        THEMIS_ERROR("GCS provider initialization requires non-empty project_id and bucket");
        return false;
    }
    try {
        // Initialize GCS client
        if (!credentials_file.empty()) {
            auto credentials
                = google::cloud::storage::oauth2::CreateServiceAccountCredentialsFromFile(credentials_file);
            if (!credentials) {
                THEMIS_ERROR("GCS provider: failed to load credentials from {}", credentials_file);
                return false;
            }
            g_gcs_client = std::make_shared<google::cloud::storage::Client>(
                google::cloud::storage::CreateDefaultClient(*std::move(credentials)));
        } else {
            // Use Application Default Credentials
            g_gcs_client
                = std::make_shared<google::cloud::storage::Client>(google::cloud::storage::CreateDefaultClient());
        }

        // Register GCS upload callback
        setGCSUploadFn([bucket](const std::string &callback_bucket, const std::string &local_path,
                                const std::string &remote_path,
                                const std::map<std::string, std::string> &metadata) -> bool {
            if (!validateFixedProviderArgument("GCS", "bucket", bucket, callback_bucket)) {
                return false;
            }
            if (!g_gcs_client) {
                THEMIS_ERROR("GCS upload: client not initialized");
                return false;
            }

            if (local_path.empty() || remote_path.empty()) {
                THEMIS_ERROR("GCS upload: empty local or remote path");
                return false;
            }

            if (!fs::exists(local_path)) {
                THEMIS_ERROR("GCS upload: local file not found: {}", local_path);
                return false;
            }

            try {
                std::ifstream input_file(local_path, std::ios::binary);
                if (!input_file) {
                    THEMIS_ERROR("GCS upload: failed to open file: {}", local_path);
                    return false;
                }

                // Build object metadata
                google::cloud::storage::ObjectMetadata object;
                for (const auto &[key, value] : metadata) {
                    object.mutable_metadata()[key] = value;
                }

                // Upload object
                auto result = g_gcs_client->InsertObject(
                    bucket, remote_path, input_file,
                    google::cloud::storage::UploadFromStreamOptions{}.set_user_project(bucket));

                if (!result) {
                    THEMIS_ERROR("GCS upload failed: gs://{}/{} (Error: {})", bucket, remote_path,
                                 result.status().message());
                    return false;
                }

                THEMIS_INFO("GCS upload completed: gs://{}/{}", bucket, remote_path);
                return true;
            } catch (const std::exception &e) {
                THEMIS_ERROR("GCS upload exception: {}", e.what());
                return false;
            }
        });

        // Register GCS download callback
        setGCSDownloadFn([bucket](const std::string &callback_bucket, const std::string &remote_path,
                                  const std::string &local_path) -> bool {
            if (!validateFixedProviderArgument("GCS", "bucket", bucket, callback_bucket)) {
                return false;
            }
            if (!g_gcs_client) {
                THEMIS_ERROR("GCS download: client not initialized");
                return false;
            }

            if (remote_path.empty() || local_path.empty()) {
                THEMIS_ERROR("GCS download: empty remote or local path");
                return false;
            }

            try {
                std::ofstream output_file(local_path, std::ios::binary);
                if (!output_file) {
                    THEMIS_ERROR("GCS download: failed to open output file: {}", local_path);
                    return false;
                }

                auto result = g_gcs_client->ReadObject(bucket, remote_path);

                if (!result) {
                    THEMIS_ERROR("GCS download failed: gs://{}/{} (Error: {})", bucket, remote_path,
                                 result.status().message());
                    return false;
                }

                output_file << result->rdbuf();
                output_file.close();

                THEMIS_INFO("GCS download completed: gs://{}/{} -> {}", bucket, remote_path, local_path);
                return true;
            } catch (const std::exception &e) {
                THEMIS_ERROR("GCS download exception: {}", e.what());
                return false;
            }
        });

        // Register GCS delete callback
        setGCSDeleteFn([bucket](const std::string &callback_bucket, const std::string &remote_path) -> bool {
            if (!validateFixedProviderArgument("GCS", "bucket", bucket, callback_bucket)) {
                return false;
            }
            if (!g_gcs_client) {
                THEMIS_ERROR("GCS delete: client not initialized");
                return false;
            }

            if (remote_path.empty()) {
                THEMIS_ERROR("GCS delete: empty remote path");
                return false;
            }

            try {
                auto result = g_gcs_client->DeleteObject(bucket, remote_path);

                if (!result.ok()) {
                    THEMIS_ERROR("GCS delete failed: gs://{}/{} (Error: {})", bucket, remote_path, result.message());
                    return false;
                }

                THEMIS_INFO("GCS delete completed: gs://{}/{}", bucket, remote_path);
                return true;
            } catch (const std::exception &e) {
                THEMIS_ERROR("GCS delete exception: {}", e.what());
                return false;
            }
        });

        // Register GCS list callback
        setGCSListFn([bucket](const std::string &callback_bucket, const std::string &prefix)
                     -> std::vector<std::string> {
            std::vector<std::string> results = {};

            if (!validateFixedProviderArgument("GCS", "bucket", bucket, callback_bucket)) {
                return results;
            }

            if (!g_gcs_client) {
                THEMIS_ERROR("GCS list: client not initialized");
                return results;
            }

            try {
                auto list = g_gcs_client->ListObjects(bucket, google::cloud::storage::Prefix(prefix));

                for (auto &object : list) {
                    if (!object) {
                        THEMIS_ERROR("GCS list failed: gs://{}/{} (Error: {})", bucket, prefix,
                                     object.status().message());
                        return results;
                    }
                    results.push_back(object->name());
                }

                THEMIS_INFO("GCS list completed: gs://{}/{} (found {} objects)", bucket, prefix, results.size());
                return results;
            } catch (const std::exception &e) {
                THEMIS_ERROR("GCS list exception: {}", e.what());
                return results;
            }
        });

        // Register GCS exists callback
        setGCSExistsFn([bucket](const std::string &callback_bucket, const std::string &remote_path) -> bool {
            if (!validateFixedProviderArgument("GCS", "bucket", bucket, callback_bucket)) {
                return false;
            }
            if (!g_gcs_client) {
                THEMIS_ERROR("GCS exists: client not initialized");
                return false;
            }

            if (remote_path.empty()) {
                THEMIS_ERROR("GCS exists: empty remote path");
                return false;
            }

            try {
                auto metadata = g_gcs_client->GetObjectMetadata(bucket, remote_path);

                if (!metadata) {
                    if (metadata.status().code() == google::cloud::StatusCode::kNotFound) {
                        THEMIS_INFO("GCS exists check: gs://{}/{} = no", bucket, remote_path);
                        return false;
                    }
                    THEMIS_WARN("GCS exists check failed: gs://{}/{} (Error: {})", bucket, remote_path,
                                metadata.status().message());
                    return false;
                }

                THEMIS_INFO("GCS exists check: gs://{}/{} = yes", bucket, remote_path);
                return true;
            } catch (const std::exception &e) {
                THEMIS_ERROR("GCS exists exception: {}", e.what());
                return false;
            }
        });

        THEMIS_INFO("GCS provider initialized: project={}, bucket={}", project_id, bucket);
        return true;
    } catch (const std::exception &e) {
        THEMIS_ERROR("GCS provider initialization failed: {}", e.what());
        return false;
    }
}

bool isGCSProviderAvailable() {
    return true; // GCS SDK is available
}

#else

bool initializeGCSProvider(const std::string &project_id, const std::string &bucket,
                           const std::string &credentials_file) {
    THEMIS_WARN("GCS provider not available: Google Cloud C++ SDK not linked (THEMIS_WITH_GCS_SDK not set)");
    return false;
}

bool isGCSProviderAvailable() {
    return false;
}

#endif // THEMIS_WITH_GCS_SDK

} // namespace sharding
} // namespace themis
