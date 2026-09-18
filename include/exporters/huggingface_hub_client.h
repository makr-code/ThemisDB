/**
 * @file huggingface_hub_client.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 82/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

// Forward declarations – avoids pulling heavy headers into every translation
// unit that only needs the config type.
namespace themis {
class KeyProvider;
namespace governance {
class PolicyEngine;
} // namespace governance
namespace utils {
class AuditLogger;
} // namespace utils
} // namespace themis

namespace themis::exporters {

// Forward declare ExporterMetrics to avoid heavy header inclusion.
class ExporterMetrics;

struct HubUploadResult {
    bool success = false;
    std::string dataset_url;
    std::string error_message;
    int http_status = 0;
};

struct HubUploadConfig {
    std::string hf_token = {};

    std::string hf_token_kek_id = {};

    std::shared_ptr<themis::KeyProvider> key_provider;

    std::string repo_id;

    std::string commit_message = "Upload via ThemisDB HuggingFaceHubClient";

    bool create_repo = true;

    bool private_repo = false;

    std::string hub_base_url = "https://huggingface.co";

    int max_retries = 3;

    int retry_delay_ms = 1000;

    long timeout_seconds = 120;

    themis::governance::PolicyEngine* policy_engine = nullptr;

    std::shared_ptr<themis::utils::AuditLogger> audit_log;

    std::string requesting_user;

    std::shared_ptr<ExporterMetrics> metrics;
};

struct MemoryShardSpec {
    std::string relative_path;
    std::vector<char> content;
};

class HuggingFaceHubClient {
public:
    /**
     * @brief Hugging Face Hub Client.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit HuggingFaceHubClient(HubUploadConfig config);
    ~HuggingFaceHubClient();

    HubUploadResult uploadDataset(
        const std::string& dataset_dir,
        std::function<void(double /*fraction*/)> progress_cb = {}) const;

    HubUploadResult uploadShards(
        const std::vector<MemoryShardSpec>& shards,
        std::function<void(double /*fraction*/)> progress_cb = {}) const;

private:
    HubUploadConfig config_;

    mutable std::mutex config_access_mutex_;

    /**
     * @brief Resolve Token.
     * @return Return value.
     */
    std::string resolveToken() const;

    std::pair<int, std::string> httpPost(
        const std::string& url,
        const std::string& json_body,
        const std::string& bearer_token) const;

    int httpPutBytes(
        const std::string& url,
        const char* data,
        std::size_t size,
        const std::string& bearer_token,
        std::function<void(double)> progress_cb,
        std::string* retry_after_out = nullptr) const;

    int httpPutFile(
        const std::string& url,
        const std::string& file_path,
        const std::string& bearer_token,
        std::function<void(double)> progress_cb,
        std::string* retry_after_out = nullptr) const;

    /**
     * @brief Ensure Repo.
     * @param[in] bearer_token Input parameter.
     * @return Return value.
     */
    HubUploadResult ensureRepo(const std::string& bearer_token) const;
};

} // namespace themis::exporters
