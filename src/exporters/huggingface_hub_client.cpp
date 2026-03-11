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
    if (!config_.hf_token.empty()) {
        return config_.hf_token;
    }
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

HubUploadResult HuggingFaceHubClient::uploadDataset(
    const std::string& dataset_dir,
    std::function<void(double)> progress_cb) const
{
    const std::string token = resolveToken();
    if (token.empty()) {
        return {false, {}, "No HF_TOKEN set and HubUploadConfig::hf_token is empty", 0};
    }
    if (config_.repo_id.empty()) {
        return {false, {}, "HubUploadConfig::repo_id must not be empty", 0};
    }

    // 1. Ensure repo exists
    auto repo_res = ensureRepo(token);
    if (!repo_res.success) return repo_res;

    // 2. Collect all files to upload
    std::vector<std::string> files;
    for (const auto& entry : fs::recursive_directory_iterator(dataset_dir)) {
        if (entry.is_regular_file()) {
            files.push_back(entry.path().string());
        }
    }
    if (files.empty()) {
        return {false, {}, "Dataset directory '" + dataset_dir + "' contains no files", 0};
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
                return {false, {}, "Hub upload authentication failed (HTTP 401)", 401};
            }
            if (http_status == 413) {
                THEMIS_WARN("HuggingFaceHubClient: HTTP 413 for {}; file too large for a single PUT", rel);
                return {false, {}, "File '" + rel + "' too large for Hub API (HTTP 413); split shard and retry", 413};
            }
            // Transient error → retry
        }

        if (!file_ok) {
            return {false, {}, "Failed to upload file '" + rel + "' after " + std::to_string(config_.max_retries) + " retries", 0};
        }

        ++uploaded;
        if (progress_cb) {
            progress_cb(static_cast<double>(uploaded) / static_cast<double>(total_files));
        }
    }

    THEMIS_INFO("HuggingFaceHubClient: all {} files uploaded successfully to {}", total_files, repo_res.dataset_url);
    return {true, repo_res.dataset_url, {}, 200};
}

} // namespace themis::exporters
