// Lightweight stubs for cloud SDK integration functions.
// These provide linkable symbols when real cloud SDKs are not available
// in the build environment (e.g., unit test / CI with minimal deps).

#include "sharding/cloud_sdk_integration.h"

namespace themis {
namespace sharding {

bool initializeS3Provider(const std::string & /*region*/, const std::string & /*bucket*/, const std::string & /*endpoint*/) {
    // No AWS SDK linked in this build; return false to indicate not available.
    return false;
}

bool initializeAzureProvider(const std::string & /*account_name*/, const std::string & /*container*/, const std::string & /*connection_string*/) {
    // Azure SDK not present; return false.
    return false;
}

bool initializeGCSProvider(const std::string & /*project_id*/, const std::string & /*bucket*/, const std::string & /*credentials_file*/) {
    // GCS SDK not present; return false.
    return false;
}

bool isS3ProviderAvailable() {
    return false;
}

bool isAzureProviderAvailable() {
    return false;
}

bool isGCSProviderAvailable() {
    return false;
}

} // namespace sharding
} // namespace themis
