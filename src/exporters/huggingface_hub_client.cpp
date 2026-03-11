/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            huggingface_hub_client.cpp                         ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-03-10                                         ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     310                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "exporters/huggingface_hub_client.h"
#include "governance/policy_engine.h"
#include "governance/model_governance.h"
#include "security/key_provider.h"
#include "utils/audit_logger.h"
#include "utils/logger.h"

#include <nlohmann/json.hpp>

#ifdef CURL_ENABLED
#  include <curl/curl.h>
#endif

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <vector>

using json = nlohmann::json;
namespace fs = std::filesystem;

namespace themis::exporters {

// ── libcurl helpers ──────────────────────────────────────────────────────────

#ifdef CURL_ENABLED

/// RAII guard for curl_global_init/cleanup (call once per process).
namespace {
struct CurlGlobal {
    CurlGlobal()  { curl_global_init(CURL_GLOBAL_DEFAULT); }
    ~CurlGlobal() { curl_global_cleanup(); }
};

static CurlGlobal g_curl_global;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

static size_t writeStringCb(const char* data, size_t sz, size_t nmemb, void* userp) {
    auto* s = static_cast<std::string*>(userp);
    s->append(data, sz * nmemb);
    return sz * nmemb;
}

struct ProgressData {
    std::function<void(double)> cb;
    double file_fraction_start = 0.0;
    double file_fraction_range = 1.0;
};

static int progressCb(void* clientp, curl_off_t dltotal, curl_off_t dlnow,
                      curl_off_t ultotal, curl_off_t ulnow) {
    auto* pd = static_cast<ProgressData*>(clientp);
    if (pd && pd->cb && ultotal > 0) {
        const double frac = static_cast<double>(ulnow) / static_cast<double>(ultotal);
        pd->cb(pd->file_fraction_start + frac * pd->file_fraction_range);
    }
    return 0;  // non-zero cancels
}

} // anonymous namespace

#endif // CURL_ENABLED

// ── HuggingFaceHubClient ────────────────────────────────────────────────────

HuggingFaceHubClient::HuggingFaceHubClient(HubUploadConfig config)
    : config_(std::move(config)) {}

HuggingFaceHubClient::~HuggingFaceHubClient() = default;

std::string HuggingFaceHubClient::resolveToken() const {
    // Priority 1: explicit hf_token field.
    if (!config_.hf_token.empty()) {
        return config_.hf_token;
    }

    // Priority 2: KEK/KMS-protected token lookup via key_provider.
    if (!config_.hf_token_kek_id.empty()) {
        if (!config_.key_provider) {
            throw std::invalid_argument(
                "HubUploadConfig::hf_token_kek_id is set but key_provider is null");
        }
        // getKey() may throw KeyNotFoundException or KeyOperationException.
        // Raw token bytes are intentionally never logged.
        auto token_bytes = config_.key_provider->getKey(config_.hf_token_kek_id);
        if (token_bytes.empty()) {
            throw std::runtime_error(
                "HubUploadConfig::hf_token_kek_id '" + config_.hf_token_kek_id +
                "' resolved to empty token bytes");
        }
        return std::string(token_bytes.begin(), token_bytes.end());
    }

    // Priority 3: HF_TOKEN environment variable.
    const char* env = std::getenv("HF_TOKEN");
    return env ? std::string(env) : std::string{};
}

// ── HTTP helpers (libcurl path) ──────────────────────────────────────────────

std::pair<int, std::string> HuggingFaceHubClient::httpPost(
    const std::string& url,
    const std::string& json_body,
    const std::string& bearer_token) const
{
#ifndef CURL_ENABLED
    (void)url; (void)json_body; (void)bearer_token;
    return {0, "CURL_ENABLED is not defined; Hub upload requires libcurl"};
#else
    CURL* curl = curl_easy_init();
    if (!curl) return {0, "curl_easy_init() failed"};

    std::string response;
    struct curl_slist* headers = nullptr;
    const std::string auth_hdr = "Authorization: Bearer " + bearer_token;
    headers = curl_slist_append(headers, auth_hdr.c_str());
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL,           url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER,    headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS,    json_body.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(json_body.size()));
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeStringCb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,     &response);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT,       config_.timeout_seconds);

    CURLcode res  = curl_easy_perform(curl);
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

int HuggingFaceHubClient::httpPutFile(
    const std::string& url,
    const std::string& file_path,
    const std::string& bearer_token,
    std::function<void(double)> progress_cb) const
{
#ifndef CURL_ENABLED
    (void)url; (void)file_path; (void)bearer_token; (void)progress_cb;
    return 0;
#else
    std::ifstream f(file_path, std::ios::binary | std::ios::ate);
    if (!f) return 0;
    const auto file_size = f.tellg();
    f.seekg(0);

    std::vector<char> buf(static_cast<size_t>(file_size));
    f.read(buf.data(), file_size);
    f.close();

    CURL* curl = curl_easy_init();
    if (!curl) return 0;

    std::string response;
    struct curl_slist* headers = nullptr;
    const std::string auth_hdr = "Authorization: Bearer " + bearer_token;
    headers = curl_slist_append(headers, auth_hdr.c_str());

    ProgressData pd;
    pd.cb = progress_cb;

    curl_easy_setopt(curl, CURLOPT_URL,             url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER,      headers);
    curl_easy_setopt(curl, CURLOPT_UPLOAD,          1L);
    curl_easy_setopt(curl, CURLOPT_READDATA,        buf.data());
    curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, static_cast<curl_off_t>(file_size));
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,   writeStringCb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,       &response);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION,  1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT,         config_.timeout_seconds);

    if (progress_cb) {
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progressCb);
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA,     &pd);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS,       0L);
    }

    CURLcode res  = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        THEMIS_WARN("HuggingFaceHubClient: PUT {} failed: {}", url, curl_easy_strerror(res));
        return 0;
    }
    return static_cast<int>(http_code);
#endif
}

// ── Repository management ────────────────────────────────────────────────────

HubUploadResult HuggingFaceHubClient::ensureRepo(const std::string& bearer_token) const {
    // Check if repo exists via the Hub API
    const std::string api_url = config_.hub_base_url + "/api/datasets/" + config_.repo_id;
    auto [status, body] = httpPost(api_url, "{}", bearer_token);

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
            auto [cs, cb] = httpPost(create_url, create_req.dump(), bearer_token);
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
static void writeHubUploadAuditEntry(
    themis::utils::AuditLogger& audit_log,
    const HubUploadConfig& config,
    const std::string& dataset_dir,
    const HubUploadResult& result,
    const std::string& outcome)
{
    using nlohmann::json;
    json entry = {
        {"event_type",       "hub_upload"},
        {"repo_id",          config.repo_id},
        {"requesting_user",  config.requesting_user},
        {"dataset_dir",      dataset_dir},
        {"outcome",          outcome},
        {"success",          result.success},
        {"http_status",      result.http_status},
        {"dataset_url",      result.dataset_url},
        {"error_message",    result.error_message},
        {"timestamp",        std::chrono::duration_cast<std::chrono::milliseconds>(
                                 std::chrono::system_clock::now().time_since_epoch()).count()}
    };
    audit_log.logEvent(entry);
}

HubUploadResult HuggingFaceHubClient::uploadDataset(
    const std::string& dataset_dir,
    std::function<void(double)> progress_cb) const
{
    // ── 0. PolicyEngine authorization check ─────────────────────────────────
    if (config_.policy_engine) {
        themis::governance::ModelTrainingExportRequest req;
        req.export_job_id   = "hub-upload-" + config_.repo_id;
        req.collection_ids  = {config_.repo_id};
        req.requesting_user = config_.requesting_user;
        req.purpose         = "HUB_UPLOAD";

        const auto decision = config_.policy_engine->checkExportPermission(req);
        if (!decision.is_permitted) {
            const HubUploadResult denied{
                false, {}, "Hub upload denied by PolicyEngine: " + decision.denial_reason, 0};
            if (config_.audit_log) {
                writeHubUploadAuditEntry(*config_.audit_log, config_, dataset_dir,
                                         denied, "denied");
            }
            THEMIS_WARN("HuggingFaceHubClient: upload to '{}' denied: {}",
                        config_.repo_id, decision.denial_reason);
            return denied;
        }
    }

    std::string token;
    try {
        token = resolveToken();
    } catch (const std::exception& e) {
        const HubUploadResult kek_err{
            false, {}, std::string("hf_token_kek_id resolution failed: ") + e.what(), 0};
        if (config_.audit_log) {
            writeHubUploadAuditEntry(*config_.audit_log, config_, dataset_dir,
                                     kek_err, "error");
        }
        return kek_err;
    }
    if (token.empty()) {
        const HubUploadResult no_token{
            false, {}, "No HF_TOKEN set and HubUploadConfig::hf_token is empty", 0};
        if (config_.audit_log) {
            writeHubUploadAuditEntry(*config_.audit_log, config_, dataset_dir,
                                     no_token, "error");
        }
        return no_token;
    }
    if (config_.repo_id.empty()) {
        const HubUploadResult no_repo{
            false, {}, "HubUploadConfig::repo_id must not be empty", 0};
        if (config_.audit_log) {
            writeHubUploadAuditEntry(*config_.audit_log, config_, dataset_dir,
                                     no_repo, "error");
        }
        return no_repo;
    }

    // 1. Ensure repo exists
    auto repo_res = ensureRepo(token);
    if (!repo_res.success) {
        if (config_.audit_log) {
            writeHubUploadAuditEntry(*config_.audit_log, config_, dataset_dir,
                                     repo_res, "error");
        }
        return repo_res;
    }

    // 2. Collect all files to upload
    std::vector<std::string> files;
    for (const auto& entry : fs::recursive_directory_iterator(dataset_dir)) {
        if (entry.is_regular_file()) {
            files.push_back(entry.path().string());
        }
    }
    if (files.empty()) {
        const HubUploadResult empty_dir{
            false, {}, "Dataset directory '" + dataset_dir + "' contains no files", 0};
        if (config_.audit_log) {
            writeHubUploadAuditEntry(*config_.audit_log, config_, dataset_dir,
                                     empty_dir, "error");
        }
        return empty_dir;
    }
    std::sort(files.begin(), files.end());

    THEMIS_INFO("HuggingFaceHubClient: uploading {} files to {}", files.size(), config_.repo_id);

    // 3. Upload each file with retry logic
    const size_t total_files = files.size();
    size_t uploaded = 0;

    for (const auto& file_path : files) {
        const std::string rel = fs::relative(file_path, dataset_dir).string();
        const std::string upload_url = config_.hub_base_url
            + "/api/datasets/" + config_.repo_id
            + "/upload/main"
            + "/" + rel;

        bool file_ok = false;
        for (int attempt = 0; attempt <= config_.max_retries; ++attempt) {
            if (attempt > 0) {
                const int delay_ms = config_.retry_delay_ms * (1 << (attempt - 1));
                std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
                THEMIS_WARN("HuggingFaceHubClient: retry {} for file {}", attempt, rel);
            }

            const double frac_start = static_cast<double>(uploaded) / static_cast<double>(total_files);
            const double frac_range = 1.0 / static_cast<double>(total_files);

            std::function<void(double)> file_progress;
            if (progress_cb) {
                file_progress = [&progress_cb, frac_start, frac_range](double f) {
                    progress_cb(frac_start + f * frac_range);
                };
            }

            const int http_status = httpPutFile(upload_url, file_path, token, file_progress);

            if (http_status == 200 || http_status == 201) {
                file_ok = true;
                break;
            }
            if (http_status == 401) {
                const HubUploadResult auth_fail{
                    false, {}, "Hub upload authentication failed (HTTP 401)", 401};
                if (config_.audit_log) {
                    writeHubUploadAuditEntry(*config_.audit_log, config_, dataset_dir,
                                             auth_fail, "error");
                }
                return auth_fail;
            }
            if (http_status == 413) {
                THEMIS_WARN("HuggingFaceHubClient: HTTP 413 for {}; file too large for a single PUT", rel);
                const HubUploadResult too_large{
                    false, {},
                    "File '" + rel + "' too large for Hub API (HTTP 413); split shard and retry",
                    413};
                if (config_.audit_log) {
                    writeHubUploadAuditEntry(*config_.audit_log, config_, dataset_dir,
                                             too_large, "error");
                }
                return too_large;
            }
            // Transient error → retry
        }

        if (!file_ok) {
            const HubUploadResult retry_fail{
                false, {},
                "Failed to upload file '" + rel + "' after " + std::to_string(config_.max_retries) + " retries",
                0};
            if (config_.audit_log) {
                writeHubUploadAuditEntry(*config_.audit_log, config_, dataset_dir,
                                         retry_fail, "error");
            }
            return retry_fail;
        }

        ++uploaded;
        if (progress_cb) {
            progress_cb(static_cast<double>(uploaded) / static_cast<double>(total_files));
        }
    }

    THEMIS_INFO("HuggingFaceHubClient: all {} files uploaded successfully to {}",
                total_files, repo_res.dataset_url);
    const HubUploadResult success{true, repo_res.dataset_url, {}, 200};
    if (config_.audit_log) {
        writeHubUploadAuditEntry(*config_.audit_log, config_, dataset_dir, success, "success");
    }
    return success;
}

} // namespace themis::exporters
