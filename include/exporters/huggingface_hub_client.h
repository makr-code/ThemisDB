/**
 * @file huggingface_hub_client.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 82/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: huggingface_hub_client.h | Version: 0.0.13 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 97/100 | Lines: 252
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): #3789 feat(exporters): add hf_tok... (2026-03-12) | #3762 feat(exporters): HuggingFac... (2026-03-12) | #3621 feat(exporters): EXP-001 Po... (2026-03-12)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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
    /// Hub API token.  When empty, the client checks `hf_token_kek_id` (if
    /// set) and then falls back to the `HF_TOKEN` environment variable.
    /// Raw key material must never be logged.
    std::string hf_token;

    /// KEK/KMS key ID for protected HF token lookup.
    /// When non-empty, `resolveToken()` fetches the raw token bytes from
    /// `key_provider->getKey(hf_token_kek_id)` and interprets them as a
    /// UTF-8 bearer token.  This field is evaluated only when `hf_token` is
    /// empty; it takes precedence over the `HF_TOKEN` environment variable.
    /// Requires `key_provider` to be non-null; a null `key_provider` with a
    /// non-empty `hf_token_kek_id` is a misconfiguration and will be surfaced
    /// as an error.  Raw token bytes are never logged.
    std::string hf_token_kek_id;

    /// Key provider used to resolve `hf_token_kek_id`.
    /// Null when KEK-based token lookup is not used (backward-compatible
    /// default).  The provider must outlive all upload calls.
    std::shared_ptr<themis::KeyProvider> key_provider;

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

    /// Optional PolicyEngine for upload authorization checks.
    /// When non-null, `uploadDataset()` calls
    /// `PolicyEngine::checkExportPermission()` before any HTTP activity.
    /// A denied decision causes `uploadDataset()` to return immediately with
    /// `success=false` — no files are uploaded.
    /// Raw non-owning pointer; the caller must ensure it outlives all upload
    /// calls.  Null = no policy check (backward-compatible default).
    themis::governance::PolicyEngine* policy_engine = nullptr;

    /// Optional AuditLogger for recording every upload attempt.
    /// When non-null, each call to `uploadDataset()` appends a JSON entry
    /// containing the repo_id, requesting_user, dataset_dir, outcome
    /// (success / denied / error), and timestamp.
    /// Null = no audit trail (backward-compatible default).
    std::shared_ptr<themis::utils::AuditLogger> audit_log;

    /// User identity forwarded to PolicyEngine and the audit log.
    /// Treated as anonymous when empty.
    std::string requesting_user;

    /// Optional ExporterMetrics instance for recording rate-limit events.
    /// When non-null, a `exporters.huggingface.rate_limit_hit` metric is
    /// incremented each time the Hub returns HTTP 429.
    /// Null = no metrics emitted (backward-compatible default).
    std::shared_ptr<ExporterMetrics> metrics;
};

/// A single in-memory shard for use with HuggingFaceHubClient::uploadShards().
///
/// `relative_path` is the path that will appear in the Hub repository
/// (e.g., `"data/train-00000-of-00001.jsonl"`).  `content` holds the raw
/// bytes of the shard and is uploaded directly without writing to disk.
struct MemoryShardSpec {
    /// Relative path within the Hub repository (forward-slash separated).
    std::string relative_path;
    /// Raw shard bytes (e.g., JSONL content).
    std::vector<char> content;
};

/// @brief Uploads a HuggingFace Datasets-compatible directory to the HF Hub.
///
/// The client uses libcurl for HTTP communication.  Authentication is via a
/// Bearer token supplied in HubUploadConfig::hf_token or the `HF_TOKEN`
/// environment variable.
///
/// ### Upload workflow
/// 1. Resolve or create the Hub repository via the Hub API.
/// 2. Enumerate all files in the dataset directory (disk path) **or** accept
///    pre-built in-memory shards (memory path via `uploadShards()`).
/// 3. Upload each file / shard via the Hub LFS / files API.
/// 4. Create a single commit bundling all uploaded files.
///
/// ### Error handling
/// - Network timeouts trigger up to `max_retries` retries with exponential
///   back-off starting at `retry_delay_ms`.
/// - HTTP 401 Unauthorized is surfaced immediately (no retry) as
///   `HubUploadResult::success=false` and a descriptive `error_message`.
/// - HTTP 413 Payload Too Large causes the client to return an error with a
///   hint to reduce the shard size and retry.
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
    /// If `HubUploadConfig::policy_engine` is non-null, the method calls
    /// `PolicyEngine::checkExportPermission()` before any network activity.
    /// A denied decision returns immediately with `success=false` and a
    /// descriptive `error_message` — no files are uploaded.
    ///
    /// If `HubUploadConfig::audit_log` is non-null, every invocation is
    /// recorded as a JSON audit entry regardless of outcome (success, denied,
    /// or network error).
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

    /// @brief Upload pre-built in-memory shards to the Hugging Face Hub.
    ///
    /// This is the memory-streaming variant of `uploadDataset()`: it accepts
    /// a list of `MemoryShardSpec` values (each carrying a relative path and
    /// raw byte content) and uploads them directly via the libcurl read
    /// callback, without writing any temporary files to disk.  It is suitable
    /// for container / serverless environments with read-only or absent local
    /// storage.
    ///
    /// All retry, progress-callback, PolicyEngine, and AuditLogger behaviour
    /// is identical to `uploadDataset()`.
    ///
    /// @param shards       In-memory shards to upload.  Must not be empty.
    /// @param progress_cb  Optional progress callback; receives a fraction in
    ///                     [0.0, 1.0] as each shard completes.
    /// @returns HubUploadResult with success flag and dataset URL or error.
    HubUploadResult uploadShards(
        const std::vector<MemoryShardSpec>& shards,
        std::function<void(double /*fraction*/)> progress_cb = {}) const;

private:
    HubUploadConfig config_;

    /// Guards concurrent access to config_.policy_engine and
    /// config_.key_provider within resolveToken(), uploadDataset(), and
    /// uploadShards(). Serialises the KEK-fetch and policy-check boundaries;
    /// raw token material is never logged.
    mutable std::mutex config_access_mutex_;

    /// Resolve the effective API token.
    /// Resolution order:
    ///   1. `hf_token` (explicit plaintext field)
    ///   2. `hf_token_kek_id` via `key_provider` (KEK/KMS-protected lookup)
    ///   3. `HF_TOKEN` environment variable
    /// Returns an empty string only when none of the above sources provide a
    /// token.  Throws `std::invalid_argument` when `hf_token_kek_id` is set
    /// but `key_provider` is null, and `std::runtime_error` when key lookup
    /// fails or the resolved bytes are empty.  Raw token material is never
    /// logged at any level.
    std::string resolveToken() const;

    /// POST to Hub API; returns {http_status, response_body}.
    std::pair<int, std::string> httpPost(
        const std::string& url,
        const std::string& json_body,
        const std::string& bearer_token) const;

    /// PUT raw bytes to Hub LFS / file-upload endpoint via libcurl read
    /// callback (no filesystem access).  Returns HTTP status code.
    /// If `retry_after_out` is non-null and the server returns a
    /// `Retry-After` response header, its raw value is written there.
    int httpPutBytes(
        const std::string& url,
        const char* data,
        std::size_t size,
        const std::string& bearer_token,
        std::function<void(double)> progress_cb,
        std::string* retry_after_out = nullptr) const;

    /// PUT file content to Hub LFS / file-upload endpoint.
    /// Reads the file from disk then delegates to httpPutBytes().
    /// Returns HTTP status code.
    int httpPutFile(
        const std::string& url,
        const std::string& file_path,
        const std::string& bearer_token,
        std::function<void(double)> progress_cb,
        std::string* retry_after_out = nullptr) const;

    /// Ensure the Hub repo exists; creates it when create_repo=true.
    /// Returns the repo's full path or an error string in the result.
    HubUploadResult ensureRepo(const std::string& bearer_token) const;
};

} // namespace themis::exporters
