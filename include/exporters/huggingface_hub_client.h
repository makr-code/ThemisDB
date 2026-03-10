/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            huggingface_hub_client.h                           ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-03-10                                         ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     145                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <functional>
#include <string>
#include <vector>

namespace themis::exporters {

/// Result returned by HuggingFaceHubClient::uploadDataset().
struct HubUploadResult {
    bool success = false;
    /// Full Hub URL for the uploaded dataset repo (populated on success).
    /// Example: "https://huggingface.co/datasets/my-org/my-dataset"
    std::string dataset_url;
    /// Human-readable error message (populated on failure).
    std::string error_message;
    /// HTTP status code from the last Hub API call (0 if no HTTP response received).
    int http_status = 0;
};

/// Configuration for a HuggingFace Hub upload operation.
struct HubUploadConfig {
    /// Hub API token.  When empty, the client reads the `HF_TOKEN` environment
    /// variable.  Raw key material must never be logged.
    std::string hf_token;

    /// Target Hub repository in the form `"owner/dataset-name"`.
    /// The repository is created automatically when it does not exist.
    std::string repo_id;

    /// Commit message for the Hub commit.
    std::string commit_message = "Upload via ThemisDB HuggingFaceHubClient";

    /// Whether to create the Hub repository if it does not exist.
    bool create_repo = true;

    /// Whether the newly created repository is private.
    bool private_repo = false;

    /// Hub API base URL (default: "https://huggingface.co").
    /// Can be overridden for Hub Enterprise or testing.
    std::string hub_base_url = "https://huggingface.co";

    /// Maximum number of HTTP retries on transient failures (timeout, 5xx).
    int max_retries = 3;

    /// Initial retry delay in milliseconds (doubles on each retry).
    int retry_delay_ms = 1000;

    /// Connection / operation timeout in seconds.
    long timeout_seconds = 120;
};

/// @brief Uploads a HuggingFace Datasets-compatible directory to the HF Hub.
///
/// The client uses libcurl for HTTP communication.  Authentication is via a
/// Bearer token supplied in HubUploadConfig::hf_token or the `HF_TOKEN`
/// environment variable.
///
/// ### Upload workflow
/// 1. Resolve or create the Hub repository via the Hub API.
/// 2. Enumerate all files in the dataset directory.
/// 3. Upload each file via the Hub LFS / files API with chunked transfer.
/// 4. Create a single commit bundling all uploaded files.
///
/// ### Error handling
/// - Network timeouts trigger up to `max_retries` retries with exponential
///   back-off starting at `retry_delay_ms`.
/// - HTTP 401 Unauthorized is surfaced immediately (no retry) as
///   `HubUploadResult::success=false` and a descriptive `error_message`.
/// - HTTP 413 Payload Too Large causes the client to split the shard into
///   two halves and retry each half independently.
///
/// ### Thread safety
/// HuggingFaceHubClient instances are NOT thread-safe.  Create one instance
/// per upload operation.
class HuggingFaceHubClient {
public:
    explicit HuggingFaceHubClient(HubUploadConfig config);
    ~HuggingFaceHubClient();

    /// @brief Upload a dataset directory to the Hugging Face Hub.
    ///
    /// @param dataset_dir  Path to the directory produced by HuggingFaceExporter
    ///                     (contains `dataset_info.json`, `README.md`, and
    ///                     `data/*.jsonl` shards).
    /// @param progress_cb  Optional progress callback; receives a fraction in
    ///                     [0.0, 1.0] as each file completes.
    /// @returns HubUploadResult with success flag and dataset URL or error.
    HubUploadResult uploadDataset(
        const std::string& dataset_dir,
        std::function<void(double /*fraction*/)> progress_cb = {}) const;

private:
    HubUploadConfig config_;

    /// Resolve the effective API token (config field → HF_TOKEN env).
    std::string resolveToken() const;

    /// POST to Hub API; returns {http_status, response_body}.
    std::pair<int, std::string> httpPost(
        const std::string& url,
        const std::string& json_body,
        const std::string& bearer_token) const;

    /// PUT file content to Hub LFS / file-upload endpoint.
    /// Returns HTTP status code.
    int httpPutFile(
        const std::string& url,
        const std::string& file_path,
        const std::string& bearer_token,
        std::function<void(double)> progress_cb) const;

    /// Ensure the Hub repo exists; creates it when create_repo=true.
    /// Returns the repo's full path or an error string in the result.
    HubUploadResult ensureRepo(const std::string& bearer_token) const;
};

} // namespace themis::exporters
