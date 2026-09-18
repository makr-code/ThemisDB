/**
 * @file cloud_sdk_integration_stubs.cpp
 * @brief Stub implementations for cloud-SDK sharding integrations.
 *
 * Provides no-op or simulation stubs for cloud-provider SDKs
 * (AWS, GCP, Azure) so that the sharding module compiles and runs
 * without a live cloud SDK in CI and offline environments.
 */

// Lightweight stubs for cloud SDK integration functions.
// These provide linkable symbols when real cloud SDKs are not available
// in the build environment (e.g., unit test / CI with minimal deps).

#include "sharding/cloud_sdk_integration.h"

namespace themis {
namespace sharding {

/**
 * @brief Initialize S3 Provider.
 * @param[in] param Input parameter.
 * @param[in] param Input parameter.
 * @param[in] param Input parameter.
 * @return True when the operation succeeds.
 * @details Implements initializeS3Provider without additional internal calls.
 */
bool initializeS3Provider(const std::string & /*region*/, const std::string & /*bucket*/, const std::string & /*endpoint*/) {
    // No AWS SDK linked in this build; return false to indicate not available.
    return false;
}

/**
 * @brief Initialize Azure Provider.
 * @param[in] param Input parameter.
 * @param[in] param Input parameter.
 * @param[in] param Input parameter.
 * @return True when the operation succeeds.
 * @details Implements initializeAzureProvider without additional internal calls.
 */
bool initializeAzureProvider(const std::string & /*account_name*/, const std::string & /*container*/, const std::string & /*connection_string*/) {
    // Azure SDK not present; return false.
    return false;
}

/**
 * @brief Initialize GCSProvider.
 * @param[in] param Input parameter.
 * @param[in] param Input parameter.
 * @param[in] param Input parameter.
 * @return True when the operation succeeds.
 * @details Implements initializeGCSProvider without additional internal calls.
 */
bool initializeGCSProvider(const std::string & /*project_id*/, const std::string & /*bucket*/, const std::string & /*credentials_file*/) {
    // GCS SDK not present; return false.
    return false;
}

/**
 * @brief Is S3 Provider Available.
 * @return True when the operation succeeds.
 * @details Implements isS3ProviderAvailable without additional internal calls.
 */
bool isS3ProviderAvailable() {
    return false;
}

/**
 * @brief Is Azure Provider Available.
 * @return True when the operation succeeds.
 * @details Implements isAzureProviderAvailable without additional internal calls.
 */
bool isAzureProviderAvailable() {
    return false;
}

/**
 * @brief Is GCSProvider Available.
 * @return True when the operation succeeds.
 * @details Implements isGCSProviderAvailable without additional internal calls.
 */
bool isGCSProviderAvailable() {
    return false;
}

} // namespace sharding
} // namespace themis
