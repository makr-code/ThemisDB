/**
 * @file huggingface_hub_client.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=5, H=2, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "exporters/huggingface_hub_client.h"

#include <nlohmann/json.hpp>

#include "exporters/exporter_metrics.h"
#include "governance/model_governance.h"
#include "governance/policy_engine.h"
#include "security/key_provider.h"
#include "utils/audit_logger.h"
#include "utils/logger.h"
#include "utils/retry_policy.h"

#ifdef CURL_ENABLED
#include <curl/curl.h>
#endif

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <vector>

using json   = nlohmann::json;
namespace fs = std::filesystem;

namespace themis::exporters {

// ── libcurl helpers ──────────────────────────────────────────────────────────

#ifdef CURL_ENABLED

/// RAII guard for curl_global_init/cleanup (call once per process).
namespace {
struct CurlGlobal {
    CurlGlobal() {
        curl_global_init(CURL_GLOBAL_DEFAULT);
    }
    ~CurlGlobal() {
        curl_global_cleanup();
    }
};

static CurlGlobal g_curl_global; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

static size_t writeStringCb(const char *data, size_t sz, size_t nmemb, void *userp) {
    auto *s = static_cast<std::string *>(userp);
    s->append(data, sz * nmemb);
    return sz * nmemb;
}

struct ProgressData {
    std::function<void(double)> cb;
    double file_fraction_start = 0.0;
    double file_fraction_range = 1.0;
};

static int progressCb(void *clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) {
    auto *pd = static_cast<ProgressData *>(clientp);
    if (pd && pd->cb && ultotal > 0) {
        const double frac = static_cast<double>(ulnow) / static_cast<double>(ultotal);
        pd->cb(pd->file_fraction_start + frac * pd->file_fraction_range);
    }
    return 0; // non-zero cancels
}

/// State for the libcurl read callback used by httpPutBytes().
struct CurlMemoryReadState {
    const char *data   = nullptr;
    std::size_t size   = 0;
    std::size_t offset = 0;
};

/// libcurl CURLOPT_READFUNCTION callback that reads from a CurlMemoryReadState.
static size_t memoryReadCb(char *dest, size_t sz, size_t nmemb, void *userp) {
    auto *state                 = static_cast<CurlMemoryReadState *>(userp);
    const std::size_t available = state->size - state->offset;
    const std::size_t to_copy   = std::min(sz * nmemb, available);
    if (to_copy == 0)
        return 0;
    std::memcpy(dest, state->data + state->offset, to_copy);
    state->offset += to_copy;
    return to_copy;
}

/// libcurl CURLOPT_HEADERFUNCTION callback; accumulates raw response headers.
static size_t headerCaptureCb(char *buffer, size_t size, size_t nitems, void *userp) {
    auto *hdrs = static_cast<std::string *>(userp);
    hdrs->append(buffer, size * nitems);
    return size * nitems;
}

} // anonymous namespace

#endif // CURL_ENABLED

namespace {

/// Extract the value of the `Retry-After` response header from a raw
/// header block captured by headerCaptureCb().  Returns an empty string
/// when the header is absent.
[[maybe_unused]] static std::string extractRetryAfterHeader(const std::string &raw_headers) {
    // Walk line by line (headers end with \r\n or \n).
    std::istringstream stream(raw_headers);
    std::string line = {};
    while (std::getline(stream, line)) {
        // Trim trailing \r
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        // Case-insensitive prefix match for "retry-after:"
        const std::string key = "retry-after:";
        if (static_cast<int>(line.size()) > = key.size()) {
            std::string lower_line = line.substr(0,static_cast<int>(key.size()));
            std::transform(lower_line.begin(), lower_line.end(), lower_line.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            if (lower_line == key) {
                std::string value = line.substr(key.size());
                // Trim leading whitespace
                const auto first = value.find_first_not_of(" \t");
                if (first != std::string::npos) {
                    value = value.substr(first);
                }
                return value;
            }
        }
    }
    return {};
}

/// Parse a `Retry-After` header value and return the number of seconds to
/// wait.  Accepts:
///   - Plain integer seconds (e.g., "120")
///   - HTTP-date (e.g., "Fri, 31 Dec 1999 23:59:59 GMT")
/// Returns 0 when the value cannot be parsed.
static long parseRetryAfterSeconds(const std::string &value) {
    if (value.empty()) {
        return 0;
    }

    // Try integer first.
    try {
        std::size_t pos = 0;
        const long secs = std::stol(value, &pos);
        // Ensure the whole token is numeric (skip trailing whitespace).
        const auto tail = value.find_first_not_of(" \t", pos);
        if (tail == std::string::npos && secs >= 0) {
            return secs;
        }
    } catch (...) {
        // Not an integer; fall through to date parsing.
    }

    // Try HTTP-date via strptime (POSIX).
    // Supported format: "Day, DD Mon YYYY HH:MM:SS GMT"
#ifndef _WIN32
    struct tm tm_val{};
    const char *parsed = strptime(value.c_str(), "%a, %d %b %Y %H:%M:%S %Z", &tm_val);
    if (parsed != nullptr) {
        const time_t retry_time = timegm(&tm_val);
        const time_t now        = std::time(nullptr);
        if (retry_time > now) {
            return static_cast<long>(retry_time - now);
        }
        return 0; // Date is in the past; retry immediately.
    }
#endif

    return 0; // Unrecognised format; caller uses default.
}

} // anonymous namespace

// ── HuggingFaceHubClient ────────────────────────────────────────────────────

HuggingFaceHubClient::HuggingFaceHubClient(HubUploadConfig config) : config_(std::move(config)) {}

HuggingFaceHubClient::~HuggingFaceHubClient() = default;

std::string HuggingFaceHubClient::resolveToken() const {
    // FIXED: Protect all config_ reads with mutex
    std::lock_guard<std::mutex> lk(config_access_mutex_);
    
    // Priority 1: explicit hf_token field.
    if (!config_.hf_token.empty()) {
        return config_.hf_token;
    }

    // Priority 2: KEK/KMS-protected token lookup via key_provider.
    if (!config_.hf_token_kek_id.empty()) {
        if (!config_.key_provider) {
            throw std::invalid_argument("HubUploadConfig::hf_token_kek_id is set but key_provider is null");
        }
        // Serialise concurrent callers at the KEK-fetch boundary; raw token
        // bytes are intentionally never logged.
        std::vector<uint8_t> token_bytes = config_.key_provider->getKey(config_.hf_token_kek_id);
        if (token_bytes.empty()) {
            throw std::runtime_error("HubUploadConfig::hf_token_kek_id '" + config_.hf_token_kek_id
                                     + "' resolved to empty token bytes");
        }
        return std::string(token_bytes.begin(), token_bytes.end());
    }

    // Priority 3: HF_TOKEN environment variable (not protected by mutex, safe to access)
    const char *env = std::getenv("HF_TOKEN");
    return env ? std::string(env) : std::string{};
}

// ── HTTP helpers (libcurl path) ──────────────────────────────────────────────

std::pair<int, std::string> HuggingFaceHubClient::httpPost([[maybe_unused]] const std::string &url,
                                                           [[maybe_unused]] const std::string &json_body,
                                                           [[maybe_unused]] const std::string &bearer_token) const {
#ifndef CURL_ENABLED
    return {0, "CURL_ENABLED is not defined; Hub upload requires libcurl"};
#else
    CURL *curl = curl_easy_init();
    if (!curl)
        return {0, "curl_easy_init() failed"};

    std::string response;
    struct curl_slist *headers = nullptr;
    const std::string auth_hdr = "Authorization: Bearer " + bearer_token;
    headers                    = curl_slist_append(headers, auth_hdr.c_str());
    headers                    = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_body.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(json_body.size()));
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeStringCb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, config_.timeout_seconds);

    CURLcode res   = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        return {0, std::string("CURL error: ") + curl_easy_strerror(res)};
    }
    return {static_cast<int>(http_code), response};
#endif
}

int HuggingFaceHubClient::httpPutBytes([[maybe_unused]] const std::string &url, [[maybe_unused]] const char *data,
                                       [[maybe_unused]] std::size_t size,
                                       [[maybe_unused]] const std::string &bearer_token,
                                       [[maybe_unused]] std::function<void(double)> progress_cb,
                                       [[maybe_unused]] std::string *retry_after_out) const {
#ifndef CURL_ENABLED
    return 0;
#else
    CURL *curl = curl_easy_init();
    if (!curl)
        return 0;

    CurlMemoryReadState read_state{data, size, 0};
    std::string response;
    std::string raw_headers;
    struct curl_slist *headers = nullptr;
    const std::string auth_hdr = "Authorization: Bearer " + bearer_token;
    headers                    = curl_slist_append(headers, auth_hdr.c_str());

    ProgressData pd;
    pd.cb = progress_cb;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, memoryReadCb);
    curl_easy_setopt(curl, CURLOPT_READDATA, &read_state);
    curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, static_cast<curl_off_t>(size));
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeStringCb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, headerCaptureCb);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &raw_headers);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, config_.timeout_seconds);

    if (progress_cb) {
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progressCb);
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &pd);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    }

    CURLcode res   = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        THEMIS_WARN("HuggingFaceHubClient: PUT {} failed: {}", url, curl_easy_strerror(res));
        return 0;
    }
    if (retry_after_out) {
        *retry_after_out = extractRetryAfterHeader(raw_headers);
    }
    return static_cast<int>(http_code);
#endif
}

int HuggingFaceHubClient::httpPutFile([[maybe_unused]] const std::string &url,
                                      [[maybe_unused]] const std::string &file_path,
                                      [[maybe_unused]] const std::string &bearer_token,
                                      [[maybe_unused]] std::function<void(double)> progress_cb,
                                      [[maybe_unused]] std::string *retry_after_out) const {
#ifndef CURL_ENABLED
    return 0;
#else
    std::ifstream f(file_path, std::ios::binary | std::ios::ate);
    if (!f)
        return 0;
    const auto file_size = f.tellg();
    f.seekg(0);

    std::vector<char> buf(static_cast<size_t>(file_size));
    f.read(buf.data(), file_size);
    f.close();

    return httpPutBytes(url, buf.data(),static_cast<int>(buf.size()), bearer_token, progress_cb, retry_after_out);
#endif
}

// ── Repository management ────────────────────────────────────────────────────

HubUploadResult HuggingFaceHubClient::ensureRepo(const std::string &bearer_token) const {
    // Check if repo exists via the Hub API
    const std::string api_url = config_.hub_base_url + "/api/datasets/" + config_.repo_id;
    auto [status, body]       = httpPost(api_url, "{}", bearer_token);

    if (status == 0) {
        return {false, {}, "Cannot reach Hub API: " + body, 0};
    }
    if (status == 200 || status == 404) {
        if (status == 404 && config_.create_repo) {
            // Create the repository
            const std::string create_url = config_.hub_base_url + "/api/repos/create";
            json create_req;
            create_req["type"]    = "dataset";
            create_req["name"]    = config_.repo_id;
            create_req["private"] = config_.private_repo;
            auto [cs, cb]         = httpPost(create_url, create_req.dump(), bearer_token);
            if (cs != 200 && cs != 201) {
                return {false, {}, "Failed to create Hub repo (HTTP " + std::to_string(cs) + "): " + cb, cs};
            }
        } else if (status == 404) {
            return {false, {}, "Hub repo '" + config_.repo_id + "' not found and create_repo=false", 404};
        }
        return {true, config_.hub_base_url + "/datasets/" + config_.repo_id, {}, status};
    }
    if (status == 401) {
        return {false, {}, "Hub authentication failed (HTTP 401): invalid or missing HF_TOKEN", 401};
    }
    return {false, {}, "Unexpected Hub API status " + std::to_string(status) + ": " + body, status};
}

// ── Main upload ──────────────────────────────────────────────────────────────

/// Write a structured audit entry for a Hub upload attempt.
/// @note Internal helper; intentionally not exposed in the header since callers
///       access audit logging exclusively via HubUploadConfig::audit_log.
static void writeHubUploadAuditEntry(themis::utils::AuditLogger &audit_log, const HubUploadConfig &config,
                                     const std::string &dataset_dir, const HubUploadResult &result,
                                     const std::string &outcome) {
    using nlohmann::json;
    json entry
        = {{"event_type", "hub_upload"},
           {"repo_id", config.repo_id},
           {"requesting_user", config.requesting_user},
           {"dataset_dir", dataset_dir},
           {"outcome", outcome},
           {"success", result.success},
           {"http_status", result.http_status},
           {"dataset_url", result.dataset_url},
           {"error_message", result.error_message},
           {"timestamp",
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
                .count()}};
    audit_log.logEvent([[maybe_unused]] entry);
}

HubUploadResult HuggingFaceHubClient::uploadDataset(const std::string &dataset_dir,
                                                    std::function<void(double)> progress_cb) const {
    // FIXED: Read all config values once under lock, then release lock before I/O
    std::string repo_id;
    std::string hub_base_url;
    long timeout_seconds;
    int max_retries;
    int retry_delay_ms;
    std::shared_ptr<themis::utils::AuditLogger> audit_log;
    std::shared_ptr<ExporterMetrics> metrics;
    std::string requesting_user;
    themis::governance::PolicyEngine* policy_engine = nullptr;
    
    {
        std::lock_guard<std::mutex> lk(config_access_mutex_);
        repo_id = config_.repo_id;
        hub_base_url = config_.hub_base_url;
        timeout_seconds = config_.timeout_seconds;
        max_retries = config_.max_retries;
        retry_delay_ms = config_.retry_delay_ms;
        audit_log = config_.audit_log;
        metrics = config_.metrics;
        requesting_user = config_.requesting_user;
        policy_engine = config_.policy_engine;
    }
    // Lock released here; all config values have been captured
    
    // ── 0. PolicyEngine authorization check ─────────────────────────────────
    if (policy_engine) {
        themis::governance::ModelTrainingExportRequest req;
        req.export_job_id   = "hub-upload-" + repo_id;
        req.collection_ids  = {repo_id};
        req.requesting_user = requesting_user;
        req.purpose         = "HUB_UPLOAD";

        themis::governance::ModelGovernanceDecision decision = policy_engine->checkExportPermission(req);
        if (!decision.is_permitted) {
            const HubUploadResult denied{false, {}, "Hub upload denied by PolicyEngine: " + decision.denial_reason, 0};
            if (audit_log) {
                writeHubUploadAuditEntry(*audit_log, config_, dataset_dir, denied, "denied");
            }
            if (metrics) {
                metrics->recordPolicyDenial(repo_id, requesting_user);
            }
            THEMIS_WARN("[EXPORT_DENIED] collection={} user={} reason={}", repo_id, requesting_user,
                        decision.denial_reason);
            return denied;
        }
    }

    std::string token = {};
    try {
        token = resolveToken();
    } catch (const std::exception &e) {
        const HubUploadResult kek_err{false, {}, std::string("hf_token_kek_id resolution failed: ") + e.what(), 0};
        if (audit_log) {
            writeHubUploadAuditEntry(*audit_log, config_, dataset_dir, kek_err, "error");
        }
        return kek_err;
    }
    if (token.empty()) {
        const HubUploadResult no_token{false, {}, "No HF_TOKEN set and HubUploadConfig::hf_token is empty", 0};
        if (audit_log) {
            writeHubUploadAuditEntry(*audit_log, config_, dataset_dir, no_token, "error");
        }
        return no_token;
    }
    if (repo_id.empty()) {
        const HubUploadResult no_repo{false, {}, "HubUploadConfig::repo_id must not be empty", 0};
        if (audit_log) {
            writeHubUploadAuditEntry(*audit_log, config_, dataset_dir, no_repo, "error");
        }
        return no_repo;
    }

    // 1. Ensure repo exists
    auto repo_res = ensureRepo(token);
    if (!repo_res.success) {
        if (audit_log) {
            writeHubUploadAuditEntry(*audit_log, config_, dataset_dir, repo_res, "error");
        }
        return repo_res;
    }

    // 2. Collect all files to upload
    std::vector<std::string> files = {};

    for (const auto &entry : fs::recursive_directory_iterator(dataset_dir)) {
        if (entry.is_regular_file()) {
            files.push_back(entry.path().string());
        }
    }
    if (files.empty()) {
        const HubUploadResult empty_dir{false, {}, "Dataset directory '" + dataset_dir + "' contains no files", 0};
        if (audit_log) {
            writeHubUploadAuditEntry(*audit_log, config_, dataset_dir, empty_dir, "error");
        }
        return empty_dir;
    }
    std::sort(files.begin(), files.end());

    THEMIS_INFO("HuggingFaceHubClient: uploading {} files to {}",static_cast<int>(files.size()), repo_id);

    // 3. Upload each file with retry logic
    const size_t total_files = files.size();
    size_t uploaded          = 0;

    for (const auto &file_path : files) {
        const std::string rel = fs::relative(file_path, dataset_dir).string();
        const std::string upload_url = hub_base_url + "/api/datasets/" + repo_id + "/upload/main" + "/" + rel;

        bool file_ok      = false;
        bool rate_limited = false;
        // Exponential backoff for transient errors (HTTP 429 uses its own
        // Retry-After sleep and does NOT advance the backoff state).
        themis::utils::RetryConfig hub_backoff_cfg;
        hub_backoff_cfg.max_attempts       = static_cast<uint32_t>(max_retries) + 1u;
        hub_backoff_cfg.initial_backoff_ms = static_cast<uint32_t>(std::max(0, retry_delay_ms));
        hub_backoff_cfg.max_backoff_ms     = 30'000u;
        hub_backoff_cfg.multiplier         = 2.0;
        hub_backoff_cfg.jitter_fraction    = 0.0;
        themis::utils::ExponentialBackoff file_backoff(hub_backoff_cfg);
        for (int attempt = 0; attempt <= max_retries; ++attempt) {
            if (attempt > 0 && !rate_limited) {
                THEMIS_WARN("HuggingFaceHubClient: retry {} for file {}", attempt, rel);
                // NOLINT(blocking_no_timeout): wait() is bounded by max_backoff_ms=30'000ms
                if (!file_backoff.wait()) {
                    break;
                }
            }
            rate_limited = false;

            const double frac_start = static_cast<double>(uploaded) / static_cast<double>(total_files);
            const double frac_range = 1.0 / static_cast<double>(total_files);

            std::function<void(double)> file_progress = {};

            if (progress_cb) {
                file_progress
                    = [&progress_cb, frac_start, frac_range]([[maybe_unused]] double f) { progress_cb(frac_start + f * frac_range); };
            }

            std::string retry_after_hdr = {};
            const int http_status = httpPutFile(upload_url, file_path, token, file_progress, &retry_after_hdr);

            if (http_status == 200 || http_status == 201) {
                file_ok = true;
                break;
            }
            if (http_status == 401) {
                const HubUploadResult auth_fail{false, {}, "Hub upload authentication failed (HTTP 401)", 401};
                if (audit_log) {
                    writeHubUploadAuditEntry(*audit_log, config_, dataset_dir, auth_fail, "error");
                }
                return auth_fail;
            }
            if (http_status == 413) {
                THEMIS_WARN("HuggingFaceHubClient: HTTP 413 for {}; file too large for a single PUT", rel);
                const HubUploadResult too_large{
                    false, {}, "File '" + rel + "' too large for Hub API (HTTP 413); split shard and retry", 413};
                if (audit_log) {
                    writeHubUploadAuditEntry(*audit_log, config_, dataset_dir, too_large, "error");
                }
                return too_large;
            }
            if (http_status == 429) {
                if (metrics) {
                    metrics->recordRateLimitHit();
                }
                long sleep_secs = parseRetryAfterSeconds(retry_after_hdr);
                if (sleep_secs <= 0) {
                    sleep_secs = (retry_delay_ms > 0) ? (retry_delay_ms / 1000 + 1) : 1;
                }
                sleep_secs = std::min(sleep_secs, timeout_seconds);
                THEMIS_WARN("HuggingFaceHubClient: HTTP 429 for {}; Retry-After='{}';"
                            " sleeping {}s",
                            rel, retry_after_hdr, sleep_secs);
                std::this_thread::sleep_for(std::chrono::seconds(sleep_secs));
                rate_limited = true;
                continue;
            }
            // Transient error → retry
        }

        if (!file_ok) {
            const HubUploadResult retry_fail{false,
                                             {},
                                             "Failed to upload file '" + rel + "' after "
                                                 + std::to_string(max_retries) + " retries",
                                             0};
            if (audit_log) {
                writeHubUploadAuditEntry(*audit_log, config_, dataset_dir, retry_fail, "error");
            }
            if (metrics) {
                metrics->recordHubUploadFailure("retry_exhausted:" + rel);
            }
            THEMIS_WARN("[HUB_UPLOAD_FAILED] repo={} reason={} http_status=0", repo_id,
                        retry_fail.error_message);
            return retry_fail;
        }

        ++uploaded;
        if (progress_cb) {
            progress_cb(static_cast<double>(uploaded) / static_cast<double>(total_files));
        }
    }

    THEMIS_INFO("HuggingFaceHubClient: all {} files uploaded successfully to {}", total_files, repo_res.dataset_url);
    const HubUploadResult success{true, repo_res.dataset_url, {}, 200};
    if (audit_log) {
        writeHubUploadAuditEntry(*audit_log, config_, dataset_dir, success, "success");
    }
    return success;
}

// ── Memory-streaming upload ──────────────────────────────────────────────────

HubUploadResult HuggingFaceHubClient::uploadShards(const std::vector<MemoryShardSpec> &shards,
                                                   std::function<void(double)> progress_cb) const {
    const std::string context = "<memory:" + std::to_string(shards.size()) + " shards>";

    // FIXED: Read all config values once under lock, then release lock before I/O
    std::string repo_id;
    std::string hub_base_url;
    long timeout_seconds;
    int max_retries;
    int retry_delay_ms;
    std::shared_ptr<themis::utils::AuditLogger> audit_log;
    std::shared_ptr<ExporterMetrics> metrics;
    std::string requesting_user;
    themis::governance::PolicyEngine* policy_engine = nullptr;
    
    {
        std::lock_guard<std::mutex> lk(config_access_mutex_);
        repo_id = config_.repo_id;
        hub_base_url = config_.hub_base_url;
        timeout_seconds = config_.timeout_seconds;
        max_retries = config_.max_retries;
        retry_delay_ms = config_.retry_delay_ms;
        audit_log = config_.audit_log;
        metrics = config_.metrics;
        requesting_user = config_.requesting_user;
        policy_engine = config_.policy_engine;
    }
    // Lock released here; all config values have been captured

    // ── 0. PolicyEngine authorization check ─────────────────────────────────
    if (policy_engine) {
        themis::governance::ModelTrainingExportRequest req;
        req.export_job_id   = "hub-upload-" + repo_id;
        req.collection_ids  = {repo_id};
        req.requesting_user = requesting_user;
        req.purpose         = "HUB_UPLOAD";

        themis::governance::ModelGovernanceDecision decision = policy_engine->checkExportPermission(req);
        if (!decision.is_permitted) {
            const HubUploadResult denied{false, {}, "Hub upload denied by PolicyEngine: " + decision.denial_reason, 0};
            if (audit_log) {
                writeHubUploadAuditEntry(*audit_log, config_, context, denied, "denied");
            }
            if (metrics) {
                metrics->recordPolicyDenial(repo_id, requesting_user);
            }
            THEMIS_WARN("[EXPORT_DENIED] collection={} user={} reason={}", repo_id, requesting_user,
                        decision.denial_reason);
            return denied;
        }
    }

    const std::string token = resolveToken();
    if (token.empty()) {
        const HubUploadResult no_token{false, {}, "No HF_TOKEN set and HubUploadConfig::hf_token is empty", 0};
        if (audit_log) {
            writeHubUploadAuditEntry(*audit_log, config_, context, no_token, "error");
        }
        return no_token;
    }
    if (repo_id.empty()) {
        const HubUploadResult no_repo{false, {}, "HubUploadConfig::repo_id must not be empty", 0};
        if (audit_log) {
            writeHubUploadAuditEntry(*audit_log, config_, context, no_repo, "error");
        }
        return no_repo;
    }
    if (shards.empty()) {
        const HubUploadResult empty_shards{false, {}, "No shards provided for memory upload", 0};
        if (audit_log) {
            writeHubUploadAuditEntry(*audit_log, config_, context, empty_shards, "error");
        }
        return empty_shards;
    }

    // 1. Ensure repo exists
    auto repo_res = ensureRepo(token);
    if (!repo_res.success) {
        if (audit_log) {
            writeHubUploadAuditEntry(*audit_log, config_, context, repo_res, "error");
        }
        return repo_res;
    }

    THEMIS_INFO("HuggingFaceHubClient: uploading {} memory shards to {}",static_cast<int>(shards.size()), repo_id);

    // 2. Upload each shard with retry logic
    const size_t total_shards = shards.size();
    size_t uploaded           = 0;

    for (const auto &shard : shards) {
        const std::string &rel = shard.relative_path;
        const std::string upload_url = hub_base_url + "/api/datasets/" + repo_id + "/upload/main" + "/" + rel;

        bool shard_ok     = false;
        bool rate_limited = false;
        // Exponential backoff for transient errors (HTTP 429 uses its own
        // Retry-After sleep and does NOT advance the backoff state).
        themis::utils::RetryConfig shard_backoff_cfg;
        shard_backoff_cfg.max_attempts       = static_cast<uint32_t>(max_retries) + 1u;
        shard_backoff_cfg.initial_backoff_ms = static_cast<uint32_t>(std::max(0, retry_delay_ms));
        shard_backoff_cfg.max_backoff_ms     = 30'000u;
        shard_backoff_cfg.multiplier         = 2.0;
        shard_backoff_cfg.jitter_fraction    = 0.0;
        themis::utils::ExponentialBackoff shard_backoff(shard_backoff_cfg);
        for (int attempt = 0; attempt <= max_retries; ++attempt) {
            if (attempt > 0 && !rate_limited) {
                THEMIS_WARN("HuggingFaceHubClient: retry {} for shard {}", attempt, rel);
                // NOLINT(blocking_no_timeout): wait() is bounded by max_backoff_ms=30'000ms
                if (!shard_backoff.wait()) {
                    break;
                }
            }
            rate_limited = false;

            const double frac_start = static_cast<double>(uploaded) / static_cast<double>(total_shards);
            const double frac_range = 1.0 / static_cast<double>(total_shards);

            std::function<void(double)> shard_progress = {};

            if (progress_cb) {
                shard_progress
                    = [&progress_cb, frac_start, frac_range]([[maybe_unused]] double f) { progress_cb(frac_start + f * frac_range); };
            }

            std::string retry_after_hdr = {};
            const int http_status
                = httpPutBytes(upload_url, shard.content.data(),static_cast<int>(shard.content.size()), token, shard_progress, &retry_after_hdr);

            if (http_status == 200 || http_status == 201) {
                shard_ok = true;
                break;
            }
            if (http_status == 401) {
                const HubUploadResult auth_fail{false, {}, "Hub upload authentication failed (HTTP 401)", 401};
                if (audit_log) {
                    writeHubUploadAuditEntry(*audit_log, config_, context, auth_fail, "error");
                }
                return auth_fail;
            }
            if (http_status == 413) {
                THEMIS_WARN("HuggingFaceHubClient: HTTP 413 for {}; shard too large for a single PUT", rel);
                const HubUploadResult too_large{
                    false, {}, "Shard '" + rel + "' too large for Hub API (HTTP 413); reduce shard size and retry", 413};
                if (audit_log) {
                    writeHubUploadAuditEntry(*audit_log, config_, context, too_large, "error");
                }
                return too_large;
            }
            if (http_status == 429) {
                if (metrics) {
                    metrics->recordRateLimitHit();
                }
                long sleep_secs = parseRetryAfterSeconds(retry_after_hdr);
                if (sleep_secs <= 0) {
                    sleep_secs = (retry_delay_ms > 0) ? (retry_delay_ms / 1000 + 1) : 1;
                }
                sleep_secs = std::min(sleep_secs, timeout_seconds);
                THEMIS_WARN("HuggingFaceHubClient: HTTP 429 for {}; Retry-After='{}';"
                            " sleeping {}s",
                            rel, retry_after_hdr, sleep_secs);
                std::this_thread::sleep_for(std::chrono::seconds(sleep_secs));
                rate_limited = true;
                continue;
            }
            // Transient error → retry
        }

        if (!shard_ok) {
            const HubUploadResult retry_fail{false,
                                             {},
                                             "Failed to upload shard '" + rel + "' after "
                                                 + std::to_string(max_retries) + " retries",
                                             0};
            if (audit_log) {
                writeHubUploadAuditEntry(*audit_log, config_, context, retry_fail, "error");
            }
            if (metrics) {
                metrics->recordHubUploadFailure("retry_exhausted:" + rel);
            }
            THEMIS_WARN("[HUB_UPLOAD_FAILED] repo={} reason={} http_status=0", repo_id,
                        retry_fail.error_message);
            return retry_fail;
        }

        ++uploaded;
        if (progress_cb) {
            progress_cb(static_cast<double>(uploaded) / static_cast<double>(total_shards));
        }
    }

    THEMIS_INFO("HuggingFaceHubClient: all {} shards uploaded successfully to {}", total_shards, repo_res.dataset_url);
    const HubUploadResult success{true, repo_res.dataset_url, {}, 200};
    if (audit_log) {
        writeHubUploadAuditEntry(*audit_log, config_, context, success, "success");
    }
    return success;
}

} // namespace themis::exporters

