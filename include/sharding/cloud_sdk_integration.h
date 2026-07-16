/**
 * @file cloud_sdk_integration.h
 * @brief Real cloud SDK implementations for S3, Azure Storage, and Google Cloud Storage
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 *
 * This header provides helper functions to integrate real cloud SDKs with ThemisDB's
 * callback-based cloud backup system. Applications can use these functions to
 * initialize cloud provider callbacks with real SDK implementations.
 *
 * Usage:
 * @code
 * // For AWS S3
 * #include "sharding/cloud_sdk_integration.h"
 * themis::sharding::initializeS3Provider("us-east-1", "my-bucket");
 *
 * // For Azure Blob Storage
 * themis::sharding::initializeAzureProvider("my-account", "my-container", "connection-string");
 *
 * // For Google Cloud Storage
 * themis::sharding::initializeGCSProvider("my-project", "my-bucket", "/path/to/credentials.json");
 *
 * // Now create coordinator - callbacks are set and ready
 * auto coordinator = std::make_unique<CloudBackupCoordinator>(agent, manager, config);
 * @endcode
 *
 * @see cloud_backup.h for callback setter functions
 */

#pragma once

#include <map>
#include <memory>
#include <string>

namespace themis {
namespace sharding {

/**
 * Initialize S3 cloud provider with real AWS SDK implementation
 *
 * Sets all 5 callbacks (upload, download, delete, list, exists) for S3 operations.
 * Requires AWS SDK for C++ to be available and linked.
 *
 * @param region AWS region (e.g., "us-east-1")
 * @param bucket S3 bucket name
 * @param endpoint Optional S3-compatible endpoint (e.g., MinIO URL). If empty, uses AWS S3.
 * @return true if initialization successful, false if AWS SDK not available
 *
 * @note Thread-safe: can be called multiple times; later calls override earlier ones
 * @note Must be called before creating CloudBackupCoordinator instances
 *
 * @throws std::runtime_error if AWS SDK initialization fails
 */
bool initializeS3Provider(const std::string &region, const std::string &bucket, const std::string &endpoint = "");

/**
 * Initialize Azure Blob Storage provider with real Azure SDK implementation
 *
 * Sets all 5 callbacks (upload, download, delete, list, exists) for Azure operations.
 * Requires Azure Storage Blobs SDK for C++ to be available and linked.
 *
 * @param account_name Azure storage account name
 * @param container Container name
 * @param connection_string Azure connection string. If empty, uses Application Default Credentials.
 * @return true if initialization successful, false if Azure SDK not available
 *
 * @note Thread-safe: can be called multiple times; later calls override earlier ones
 * @note Must be called before creating CloudBackupCoordinator instances
 * @note Connection string format:
 * DefaultEndpointsProtocol=https;AccountName=...;AccountKey=...;EndpointSuffix=core.windows.net
 *
 * @throws std::runtime_error if Azure SDK initialization fails
 */
bool initializeAzureProvider(const std::string &account_name, const std::string &container,
                             const std::string &connection_string = "");

/**
 * Initialize Google Cloud Storage provider with real GCS SDK implementation
 *
 * Sets all 5 callbacks (upload, download, delete, list, exists) for GCS operations.
 * Requires Google Cloud C++ Client Libraries to be available and linked.
 *
 * @param project_id GCS project ID
 * @param bucket GCS bucket name
 * @param credentials_file Path to service account JSON credentials file. If empty, uses Application Default
 * Credentials.
 * @return true if initialization successful, false if GCS SDK not available
 *
 * @note Thread-safe: can be called multiple times; later calls override earlier ones
 * @note Must be called before creating CloudBackupCoordinator instances
 * @note Credentials file should contain service account JSON downloaded from GCP Console
 *
 * @throws std::runtime_error if GCS SDK initialization fails
 */
bool initializeGCSProvider(const std::string &project_id, const std::string &bucket,
                           const std::string &credentials_file = "");

/**
 * Check if S3 provider is available (AWS SDK linked)
 * @return true if AWS SDK for C++ is compiled in
 */
bool isS3ProviderAvailable();

/**
 * Check if Azure provider is available (Azure SDK linked)
 * @return true if Azure Storage Blobs SDK for C++ is compiled in
 */
bool isAzureProviderAvailable();

/**
 * Check if GCS provider is available (GCS SDK linked)
 * @return true if Google Cloud C++ Client Libraries are compiled in
 */
bool isGCSProviderAvailable();

} // namespace sharding
} // namespace themis
