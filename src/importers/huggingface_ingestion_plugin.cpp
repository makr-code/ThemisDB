/**
 * @file huggingface_ingestion_plugin.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 96/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=3, H=1, M=8, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "plugins/huggingface_ingestion_plugin.h"
#include "content/content_manager.h"
#include "observability/metrics_collector.h"
#include "utils/logger.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <thread>
#include <future>
#include <random>
#include <stdexcept>
#include <cstdlib>
#include <numeric>
#include <openssl/sha.h>

namespace themis {
namespace plugins {

namespace {

// HuggingFace API base URL
constexpr const char* HF_API_BASE = "https://datasets-server.huggingface.co";

// CURL write callback
size_t curlWriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t total_size = size * nmemb;
    auto* str = static_cast<std::string*>(userp);
    str->append(static_cast<char*>(contents), total_size);
    return total_size;
}

} // anonymous namespace

// ============================================================================
// Config Implementation
// ============================================================================

json HuggingFaceIngestionPlugin::Config::toJson() const {
    json j;
    j["dataset_name"] = dataset_name;
    j["split"] = split;
    j["streaming"] = streaming;
    j["chunk_size"] = chunk_size;
    // auth_token intentionally excluded from serialisation to prevent token leaks
    j["text_field"] = text_field;
    j["label_field"] = label_field;
    j["custom_fields"] = custom_fields;
    j["cache_dir"] = cache_dir;
    j["use_cache"] = use_cache;
    j["checkpoint_file"] = checkpoint_file;
    j["max_requests_per_second"] = max_requests_per_second;
    j["max_retries"] = max_retries;
    j["retry_delay_ms"] = retry_delay_ms;
    j["model_download_dir"] = model_download_dir;
    j["enable_metrics"] = enable_metrics;
    return j;
}

HuggingFaceIngestionPlugin::Config HuggingFaceIngestionPlugin::Config::fromJson(const json& j) {
    Config config;
    if (j.contains("dataset_name")) config.dataset_name = j["dataset_name"];
    if (j.contains("split")) config.split = j["split"];
    if (j.contains("streaming")) config.streaming = j["streaming"];
    if (j.contains("chunk_size")) config.chunk_size = j["chunk_size"];
    // auth_token is never stored in JSON; callers set it explicitly or via env var
    if (j.contains("text_field")) config.text_field = j["text_field"];
    if (j.contains("label_field")) config.label_field = j["label_field"];
    if (j.contains("custom_fields")) {
        config.custom_fields = j["custom_fields"].get<std::map<std::string, std::string>>();
    }
    if (j.contains("cache_dir")) config.cache_dir = j["cache_dir"];
    if (j.contains("use_cache")) config.use_cache = j["use_cache"];
    if (j.contains("checkpoint_file")) config.checkpoint_file = j["checkpoint_file"];
    if (j.contains("max_requests_per_second")) {
        config.max_requests_per_second = j["max_requests_per_second"];
    }
    if (j.contains("max_retries")) config.max_retries = j["max_retries"];
    if (j.contains("retry_delay_ms")) config.retry_delay_ms = j["retry_delay_ms"];
    if (j.contains("model_download_dir")) config.model_download_dir = j["model_download_dir"];
    if (j.contains("enable_metrics")) config.enable_metrics = j["enable_metrics"];
    return config;
}

// ============================================================================
// DatasetMetadata Implementation
// ============================================================================

json HuggingFaceIngestionPlugin::DatasetMetadata::toJson() const {
    json j;
    j["dataset_id"] = dataset_id;
    j["description"] = description;
    j["total_rows"] = total_rows;
    j["splits"] = splits;
    j["columns"] = columns;
    return j;
}

// ============================================================================
// HuggingFaceIngestionPlugin Implementation
// ============================================================================

HuggingFaceIngestionPlugin::HuggingFaceIngestionPlugin(
    const Config& config,
    std::shared_ptr<content::ContentManager> content_manager
)
    : config_(config)
    , content_manager_(content_manager)
    , curl_handle_(nullptr)
    , last_request_time_(std::chrono::steady_clock::now())
{
    if (!content_manager_) {
        throw std::invalid_argument("ContentManager cannot be null");
    }
    
    // Feature 1 – Resolve API token: config takes precedence; fallback to env var.
    // Tokens are never logged to prevent credential leaks.
    resolved_token_ = config_.auth_token;
    if (resolved_token_.empty()) {
        const char* env_token = std::getenv("HUGGINGFACE_TOKEN");
        if (env_token && env_token[0] != '\0') {
            resolved_token_ = env_token;
            THEMIS_INFO("HuggingFaceIngestionPlugin: using token from HUGGINGFACE_TOKEN env var");
        }
    }
    if (!resolved_token_.empty()) {
        THEMIS_INFO("HuggingFaceIngestionPlugin: authenticated requests enabled");
    }
    
    // Initialize CURL
    curl_handle_ = curl_easy_init();
    if (!curl_handle_) {
        throw std::runtime_error("Failed to initialize CURL");
    }
    
    // Create cache directory if needed
    if (config_.use_cache && !config_.cache_dir.empty()) {
        std::filesystem::create_directories(config_.cache_dir);
    }
    
    THEMIS_INFO("HuggingFaceIngestionPlugin initialized for dataset: {}", 
        config_.dataset_name.empty() ? "<not set>" : config_.dataset_name);
}

HuggingFaceIngestionPlugin::~HuggingFaceIngestionPlugin() {
    if (curl_handle_) {
        curl_easy_cleanup(curl_handle_);
    }
}

void HuggingFaceIngestionPlugin::registerWithWorker(content::AsyncIngestionWorker& worker) {
    worker_ = &worker;
    
    // Register handler for HUGGINGFACE job type
    worker.registerJobHandler(
        content::IngestionJobType::HUGGINGFACE,
        [this](content::IngestionJob& job) {
            processHuggingFaceJob(job, this);
        }
    );
    
    THEMIS_INFO("HuggingFaceIngestionPlugin registered with AsyncIngestionWorker");
}

std::string HuggingFaceIngestionPlugin::submitDatasetJob(
    const std::string& dataset_name,
    const std::string& split,
    const json& config
) {
    if (!worker_) {
        throw std::runtime_error("Plugin not registered with worker");
    }
    
    // Create job
    content::IngestionJob job;
    
    // Generate job ID with better randomness
    std::random_device rd;
    std::mt19937_64 rng(rd());
    auto u64 = rng();
    std::ostringstream oss;
    oss << "hf_job_" << std::hex << std::setw(16) << std::setfill('0') << u64;
    job.job_id = oss.str();
    
    job.type = content::IngestionJobType::HUGGINGFACE;
    job.status = content::IngestionJobStatus::QUEUED;
    job.filename = dataset_name + (split.empty() ? "" : ("/" + split));
    job.created_at = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    job.started_at = 0;
    job.completed_at = 0;
    job.total_items = -1;  // Unknown until fetched
    job.processed_items = 0;
    job.progress = 0.0f;
    
    // Store configuration
    job.config = config;
    job.config["dataset_name"] = dataset_name;
    job.config["split"] = split.empty() ? config_.split : split;
    job.config["plugin_config"] = config_.toJson();
    
    // Submit to worker (we need to manually add to queue)
    // Since AsyncIngestionWorker doesn't expose a generic submit method,
    // we'll use submitFile with empty blob and detect in our handler
    THEMIS_WARN("Cannot submit HF job directly - worker lacks generic submit API. "
        "To submit this job: "
        "1. Extend AsyncIngestionWorker with submitCustomJob(type, config) method, or "
        "2. Use worker.submitFile() as a temporary workaround and handle HUGGINGFACE type in handler");
    
    return job.job_id;
}

HuggingFaceIngestionPlugin::DatasetMetadata 
HuggingFaceIngestionPlugin::getDatasetMetadata(const std::string& dataset_name) {
    // Construct API URL for dataset info
    std::string url = std::string(HF_API_BASE) + "/info?dataset=" + dataset_name;
    
    DatasetMetadata metadata;
    
    try {
        json response = httpGetJson(url);
        
        metadata.dataset_id = dataset_name;
        
        if (response.contains("dataset_info")) {
            auto info = response["dataset_info"];
            
            if (info.contains("description")) {
                metadata.description = info["description"].get<std::string>();
            }
            
            if (info.contains("splits")) {
                for (const auto& split : info["splits"].items()) {
                    metadata.splits.push_back(split.key());
                    
                    if (split.value().contains("num_examples")) {
                        metadata.total_rows += split.value()["num_examples"].get<size_t>();
                    }
                }
            }
            
            if (info.contains("features")) {
                for (const auto& feature : info["features"].items()) {
                    std::string type = "unknown";
                    if (feature.value().contains("dtype")) {
                        type = feature.value()["dtype"].get<std::string>();
                    }
                    metadata.columns[feature.key()] = type;
                }
            }
        }
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Failed to fetch metadata for {}: {}", dataset_name, e.what());
        throw;
    }
    
    return metadata;
}

size_t HuggingFaceIngestionPlugin::estimateDatasetSize(const std::string& dataset_name) {
    try {
        auto metadata = getDatasetMetadata(dataset_name);
        return metadata.total_rows;
    } catch (...) {
        return 0;  // Unknown
    }
}

// ============================================================================
// HTTP Helpers
// ============================================================================

std::string HuggingFaceIngestionPlugin::httpGet(const std::string& url) {
    waitForRateLimit();
    
    std::string response_body;
    
    for (size_t attempt = 0; attempt < config_.max_retries; ++attempt) {
        response_body.clear();
        
        curl_easy_setopt(curl_handle_, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl_handle_, CURLOPT_WRITEFUNCTION, curlWriteCallback);
        curl_easy_setopt(curl_handle_, CURLOPT_WRITEDATA, &response_body);
        curl_easy_setopt(curl_handle_, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl_handle_, CURLOPT_TIMEOUT, 30L);
        
        // Apply auth header (Feature 1) – uses resolved_token_ which may come from env var
        struct curl_slist* headers = nullptr;
        headers = applyAuthHeader(headers);
        if (headers) {
            curl_easy_setopt(curl_handle_, CURLOPT_HTTPHEADER, headers);
        }
        
        CURLcode res = curl_easy_perform(curl_handle_);
        
        if (headers) {
            curl_slist_free_all(headers);
        }
        
        if (res == CURLE_OK) {
            long http_code = 0;
            curl_easy_getinfo(curl_handle_, CURLINFO_RESPONSE_CODE, &http_code);
            
            if (http_code == 200) {
                return response_body;
            } else if (http_code == 429) {
                // Rate limited - wait and retry
                THEMIS_WARN("Rate limited on HF API (attempt {}/{}), waiting...", 
                    attempt + 1, config_.max_retries);
                std::this_thread::sleep_for(
                    std::chrono::milliseconds(config_.retry_delay_ms * (1ULL << attempt))
                );
                continue;
            } else {
                throw std::runtime_error(
                    "HTTP error " + std::to_string(http_code) + ": " + response_body
                );
            }
        } else {
            THEMIS_WARN("CURL error on attempt {}/{}: {}", 
                attempt + 1, config_.max_retries, curl_easy_strerror(res));
            
            if (attempt < config_.max_retries - 1) {
                std::this_thread::sleep_for(
                    std::chrono::milliseconds(config_.retry_delay_ms * (1ULL << attempt))
                );
            }
        }
    }
    
    throw std::runtime_error("HTTP request failed after " + 
        std::to_string(config_.max_retries) + " attempts");
}

json HuggingFaceIngestionPlugin::httpGetJson(const std::string& url) {
    std::string response = httpGet(url);
    return json::parse(response);
}

// ============================================================================
// Dataset Fetching
// ============================================================================

HuggingFaceIngestionPlugin::FetchResult HuggingFaceIngestionPlugin::fetchBatch(
    const std::string& dataset_name,
    const std::string& split,
    size_t offset,
    size_t limit
) {
    // Construct API URL for rows endpoint
    std::ostringstream url_stream;
    url_stream << HF_API_BASE << "/rows"
               << "?dataset=" << dataset_name
               << "&config=default"
               << "&split=" << split
               << "&offset=" << offset
               << "&length=" << limit;
    
    std::string url = url_stream.str();
    
    FetchResult result;
    result.offset = offset;
    result.has_more = false;
    
    try {
        json response = httpGetJson(url);
        
        if (response.contains("rows")) {
            for (const auto& row : response["rows"]) {
                if (row.contains("row")) {
                    result.documents.push_back(row["row"]);
                }
            }
        }
        
        // Check if there are more rows
        if (response.contains("features") && response.contains("num_rows_total")) {
            size_t total = response["num_rows_total"].get<size_t>();
            result.has_more = (offset + result.documents.size()) < total;
        } else {
            // Assume more if we got a full batch
            result.has_more = (result.documents.size() >= limit);
        }
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Failed to fetch batch (offset={}, limit={}): {}", 
            offset, limit, e.what());
        throw;
    }
    
    return result;
}

// ============================================================================
// Caching
// ============================================================================

std::string HuggingFaceIngestionPlugin::getCachePath(
    const std::string& dataset_name,
    const std::string& split
) const {
    std::filesystem::path cache_path = config_.cache_dir;
    
    // Replace slashes in dataset name with underscores
    std::string safe_name = dataset_name;
    std::replace(safe_name.begin(), safe_name.end(), '/', '_');
    
    cache_path /= safe_name + "_" + split + ".json";
    return cache_path.string();
}

bool HuggingFaceIngestionPlugin::loadFromCache(
    const std::string& dataset_name,
    const std::string& split,
    std::vector<json>& docs
) {
    if (!config_.use_cache) {
        return false;
    }
    
    std::string cache_file = getCachePath(dataset_name, split);
    
    if (!std::filesystem::exists(cache_file)) {
        return false;
    }
    
    try {
        std::ifstream file(cache_file);
        json cache_data;
        file >> cache_data;
        
        if (cache_data.is_array()) {
            for (const auto& doc : cache_data) {
                docs.push_back(doc);
            }
            
            THEMIS_INFO("Loaded {} documents from cache: {}", docs.size(), cache_file);
            return true;
        }
        
    } catch (const std::exception& e) {
        THEMIS_WARN("Failed to load cache {}: {}", cache_file, e.what());
    }
    
    return false;
}

void HuggingFaceIngestionPlugin::saveToCache(
    const std::string& dataset_name,
    const std::string& split,
    const std::vector<json>& docs
) {
    if (!config_.use_cache) {
        return;
    }
    
    std::string cache_file = getCachePath(dataset_name, split);
    
    try {
        json cache_data = json::array();
        for (const auto& doc : docs) {
            cache_data.push_back(doc);
        }
        
        std::ofstream file(cache_file);
        file << cache_data.dump();  // Compact format to save space
        
        THEMIS_INFO("Saved {} documents to cache: {}", docs.size(), cache_file);
        
    } catch (const std::exception& e) {
        THEMIS_WARN("Failed to save cache {}: {}", cache_file, e.what());
    }
}

// ============================================================================
// Rate Limiting
// ============================================================================

void HuggingFaceIngestionPlugin::waitForRateLimit() {
    if (config_.max_requests_per_second == 0) {
        return;  // No rate limiting
    }
    
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - last_request_time_
    ).count();
    
    int64_t min_interval_ms = 1000 / config_.max_requests_per_second;
    
    if (elapsed < min_interval_ms) {
        std::this_thread::sleep_for(
            std::chrono::milliseconds(min_interval_ms - elapsed)
        );
    }
    
    last_request_time_ = std::chrono::steady_clock::now();
}

// ============================================================================
// Job Processing
// ============================================================================

void HuggingFaceIngestionPlugin::processHuggingFaceJob(
    content::IngestionJob& job,
    HuggingFaceIngestionPlugin* plugin
) {
    THEMIS_INFO("Processing HuggingFace job: {}", job.job_id);
    
    // Extract configuration
    std::string dataset_name = job.config.value("dataset_name", plugin->config_.dataset_name);
    std::string split = job.config.value("split", plugin->config_.split);
    
    if (dataset_name.empty()) {
        throw std::runtime_error("dataset_name not specified");
    }

    BatchMetrics metrics;
    
    // Try to load from cache first
    std::vector<json> documents;
    bool from_cache = plugin->loadFromCache(dataset_name, split, documents);
    if (from_cache) {
        ++metrics.cache_hits;
    } else {
        ++metrics.cache_misses;
    }
    
    if (!from_cache) {
        // Feature 2 – Resume from checkpoint if available
        auto ckpt = plugin->loadCheckpoint(dataset_name, split);
        size_t offset = ckpt.next_offset;
        size_t batch_size = plugin->config_.chunk_size;

        auto fetch_t0 = std::chrono::steady_clock::now();
        while (true) {
            auto result = plugin->fetchBatch(dataset_name, split, offset, batch_size);
            ++metrics.batches_fetched;
            documents.insert(documents.end(), result.documents.begin(), result.documents.end());
            
            // Update progress
            job.processed_items = static_cast<int>(documents.size());
            if (job.total_items > 0) {
                job.progress = static_cast<float>(job.processed_items) / job.total_items;
            }
            
            THEMIS_INFO("Fetched {} documents so far from {}/{}", 
                documents.size(), dataset_name, split);

            // Feature 2 – Persist checkpoint after each batch
            CheckpointState ckpt_state;
            ckpt_state.job_id       = job.job_id;
            ckpt_state.dataset_name = dataset_name;
            ckpt_state.split        = split;
            ckpt_state.next_offset  = offset + result.documents.size();
            ckpt_state.total_rows   = (job.total_items > 0)
                                        ? static_cast<size_t>(job.total_items) : 0;
            ckpt_state.updated_at   = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            plugin->saveCheckpoint(ckpt_state);
            
            if (!result.has_more) {
                break;
            }
            
            offset += result.documents.size();
        }
        auto fetch_t1 = std::chrono::steady_clock::now();
        metrics.total_fetch_ms = static_cast<double>(
            std::chrono::duration_cast<std::chrono::milliseconds>(fetch_t1 - fetch_t0).count());
        
        // Save to cache
        plugin->saveToCache(dataset_name, split, documents);
    }
    
    job.total_items = static_cast<int>(documents.size());
    
    // Ingest documents into ContentManager
    for (size_t i = 0; i < documents.size(); ++i) {
        try {
            json content_spec = plugin->documentToContentSpec(documents[i], dataset_name, i);
            
            auto status = plugin->content_manager_->importContent(content_spec, std::nullopt);
            
            if (!status.ok) {
                THEMIS_WARN("Failed to import document {}: {}", i, status.message);
            } else {
                ++metrics.rows_ingested;
                // Extract content ID from status message
                if (content_spec.contains("content") && 
                    content_spec["content"].contains("id")) {
                    job.content_ids.push_back(content_spec["content"]["id"]);
                }
            }
            
        } catch (const std::exception& e) {
            THEMIS_WARN("Error importing document {}: {}", i, e.what());
        }
        
        job.processed_items = static_cast<int>(i + 1);
        job.progress = static_cast<float>(i + 1) / documents.size();
    }

    // Feature 4 – Emit Prometheus metrics
    plugin->emitBatchMetrics(metrics, dataset_name, split);
    
    job.result_metadata["total_documents"] = documents.size();
    job.result_metadata["dataset_name"] = dataset_name;
    job.result_metadata["split"] = split;
    job.result_metadata["from_cache"] = from_cache;

    // Feature 2 – Clear checkpoint on successful completion
    plugin->clearCheckpoint();
    
    THEMIS_INFO("HuggingFace job {} completed: {} documents ingested", 
        job.job_id, job.content_ids.size());
}

json HuggingFaceIngestionPlugin::documentToContentSpec(
    const json& doc,
    const std::string& dataset_name,
    size_t index
) {
    json spec;
    
    // Generate unique ID
    std::ostringstream id_stream;
    id_stream << "hf_" << dataset_name << "_" << index;
    std::string content_id = id_stream.str();
    
    // Replace invalid characters
    std::replace(content_id.begin(), content_id.end(), '/', '_');
    std::replace(content_id.begin(), content_id.end(), ':', '_');
    
    // Create content metadata
    spec["content"] = {
        {"id", content_id},
        {"mime_type", "text/plain"},
        {"category", "text"},
        {"original_filename", dataset_name + "_" + std::to_string(index) + ".txt"},
        {"size_bytes", 0},  // Will be calculated
        {"created_at", std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()},
        {"text_extracted", true},
        {"chunked", true},
        {"indexed", false},
        {"user_metadata", {
            {"source", "huggingface"},
            {"dataset", dataset_name},
            {"index", index}
        }}
    };
    
    // Extract text content
    std::string text_content;
    if (doc.contains(config_.text_field)) {
        text_content = doc[config_.text_field].get<std::string>();
    } else if (doc.contains("text")) {
        text_content = doc["text"].get<std::string>();
    } else {
        // Fall back to JSON dump
        text_content = doc.dump();
    }
    
    spec["content"]["size_bytes"] = text_content.size();
    
    // Create a single chunk with the text
    spec["chunks"] = json::array();
    spec["chunks"].push_back({
        {"id", content_id + "_chunk_0"},
        {"content_id", content_id},
        {"seq_num", 0},
        {"chunk_type", "text"},
        {"text", text_content},
        {"start_offset", 0},
        {"end_offset", static_cast<int>(text_content.size())},
        {"created_at", std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()}
    });
    
    return spec;
}

// ============================================================================
// Feature 1 – Token Auth: applyAuthHeader
// ============================================================================

curl_slist* HuggingFaceIngestionPlugin::applyAuthHeader(curl_slist* headers) const {
    if (resolved_token_.empty()) {
        return headers;
    }
    // Build "Authorization: ******" without logging the token value.
    std::string header = "Authorization: Bearer " + resolved_token_;
    return curl_slist_append(headers, header.c_str());
}

// ============================================================================
// Feature 2 – Checkpoint: CheckpointState serialisation + save/load/clear
// ============================================================================

json HuggingFaceIngestionPlugin::CheckpointState::toJson() const {
    return {
        {"job_id",       job_id},
        {"dataset_name", dataset_name},
        {"split",        split},
        {"next_offset",  next_offset},
        {"total_rows",   total_rows},
        {"updated_at",   updated_at}
    };
}

HuggingFaceIngestionPlugin::CheckpointState
HuggingFaceIngestionPlugin::CheckpointState::fromJson(const json& j) {
    CheckpointState s;
    if (j.contains("job_id"))       s.job_id       = j["job_id"].get<std::string>();
    if (j.contains("dataset_name")) s.dataset_name = j["dataset_name"].get<std::string>();
    if (j.contains("split"))        s.split        = j["split"].get<std::string>();
    if (j.contains("next_offset"))  s.next_offset  = j["next_offset"].get<size_t>();
    if (j.contains("total_rows"))   s.total_rows   = j["total_rows"].get<size_t>();
    if (j.contains("updated_at"))   s.updated_at   = j["updated_at"].get<int64_t>();
    return s;
}

void HuggingFaceIngestionPlugin::saveCheckpoint(const CheckpointState& state) {
    if (config_.checkpoint_file.empty()) {
        return;
    }
    try {
        std::ofstream out(config_.checkpoint_file, std::ios::trunc);
        if (!out) {
            THEMIS_WARN("Cannot write checkpoint file: {}", config_.checkpoint_file);
            return;
        }
        out << state.toJson().dump(2);
        THEMIS_DEBUG("Checkpoint saved: offset={} for {}/{}", 
            state.next_offset, state.dataset_name, state.split);
    } catch (const std::exception& e) {
        THEMIS_WARN("Failed to save checkpoint: {}", e.what());
    }
}

HuggingFaceIngestionPlugin::CheckpointState
HuggingFaceIngestionPlugin::loadCheckpoint(const std::string& dataset_name,
                                            const std::string& split) const {
    CheckpointState empty;
    if (config_.checkpoint_file.empty()) {
        return empty;
    }
    try {
        std::ifstream in(config_.checkpoint_file);
        if (!in) {
            return empty;  // No checkpoint file yet – start from beginning
        }
        json j = json::parse(in);
        auto state = CheckpointState::fromJson(j);
        // Only reuse if it matches the requested dataset/split
        if (state.dataset_name == dataset_name && state.split == split) {
            THEMIS_INFO("Resuming ingestion from offset {} for {}/{}", 
                state.next_offset, dataset_name, split);
            return state;
        }
    } catch (const std::exception& e) {
        THEMIS_WARN("Cannot load checkpoint (starting fresh): {}", e.what());
    }
    return empty;
}

void HuggingFaceIngestionPlugin::clearCheckpoint() {
    if (config_.checkpoint_file.empty()) {
        return;
    }
    std::error_code ec;
    std::filesystem::remove(config_.checkpoint_file, ec);
    if (!ec) {
        THEMIS_DEBUG("Checkpoint file cleared: {}", config_.checkpoint_file);
    }
}

// ============================================================================
// Feature 3 – Model Hub: ModelDownloadResult serialisation + downloadModelWeights
// ============================================================================

json HuggingFaceIngestionPlugin::ModelDownloadResult::toJson() const {
    return {
        {"repo_id",    repo_id},
        {"filename",   filename},
        {"local_path", local_path},
        {"bytes",      bytes},
        {"sha256",     sha256},
        {"from_cache", from_cache}
    };
}

std::string HuggingFaceIngestionPlugin::computeFileSha256(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) {
        throw std::runtime_error("Cannot open file for SHA-256: " + path);
    }
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    char buf[65536];
    while (f.read(buf, sizeof(buf)) || f.gcount() > 0) {
        SHA256_Update(&ctx, buf, static_cast<size_t>(f.gcount()));
    }
    unsigned char digest[SHA256_DIGEST_LENGTH];
    SHA256_Final(digest, &ctx);

    std::ostringstream hex;
    hex << std::hex << std::setfill('0');
    for (unsigned char b : digest) {
        hex << std::setw(2) << static_cast<int>(b);
    }
    return hex.str();
}

std::string HuggingFaceIngestionPlugin::fetchModelFileSha256(
    const std::string& repo_id,
    const std::string& filename)
{
    // HuggingFace resolve endpoint returns file metadata including oid (SHA-256)
    std::string url = "https://huggingface.co/" + repo_id + "/resolve/main/" + filename
                    + "?download=false";
    try {
        // A HEAD request returns the X-Linked-Etag which is the SHA-256.
        // We fall back to a metadata API call for simplicity.
        std::string meta_url = "https://huggingface.co/api/models/" + repo_id
                             + "/resolve/main/" + filename;
        std::string resp = httpGet(meta_url);
        auto j = json::parse(resp);
        if (j.contains("oid")) {
            return j["oid"].get<std::string>();
        }
    } catch (...) {
        // SHA-256 verification is best-effort when the API doesn't provide it
    }
    return {};
}

HuggingFaceIngestionPlugin::ModelDownloadResult
HuggingFaceIngestionPlugin::downloadModelWeights(
    const std::string& repo_id,
    const std::string& filename,
    const std::string& output_dir)
{
    if (repo_id.empty() || filename.empty()) {
        throw std::invalid_argument("repo_id and filename must not be empty");
    }

    std::filesystem::create_directories(output_dir);
    std::string local_path = (std::filesystem::path(output_dir) / filename).string();

    // Fetch expected SHA-256 from Hub metadata (best-effort)
    std::string expected_sha256 = fetchModelFileSha256(repo_id, filename);

    // Check local cache
    if (std::filesystem::exists(local_path)) {
        std::string actual = computeFileSha256(local_path);
        if (expected_sha256.empty() || actual == expected_sha256) {
            THEMIS_INFO("Model artifact served from cache: {}", local_path);
            return ModelDownloadResult{
                repo_id, filename, local_path,
                static_cast<size_t>(std::filesystem::file_size(local_path)),
                actual, /*from_cache=*/true
            };
        }
        THEMIS_WARN("Cached model SHA-256 mismatch – re-downloading");
    }

    // Download the file
    std::string download_url = "https://huggingface.co/" + repo_id
                             + "/resolve/main/" + filename;
    THEMIS_INFO("Downloading model artifact from: {}", download_url);

    std::ofstream out_file(local_path, std::ios::binary | std::ios::trunc);
    if (!out_file) {
        throw std::runtime_error("Cannot create output file: " + local_path);
    }

    // Use a dedicated CURL handle for the potentially large download
    CURL* dl_curl = curl_easy_init();
    if (!dl_curl) {
        throw std::runtime_error("Failed to initialise CURL for model download");
    }
    struct CurlGuard { CURL* h; ~CurlGuard() { curl_easy_cleanup(h); } } guard{dl_curl};

    // Stream directly to file via write callback
    auto file_write_cb = [](void* data, size_t size, size_t nmemb, void* userp) -> size_t {
        auto* f = static_cast<std::ofstream*>(userp);
        size_t total = size * nmemb;
        f->write(static_cast<char*>(data), static_cast<std::streamsize>(total));
        return f->good() ? total : 0;
    };

    curl_easy_setopt(dl_curl, CURLOPT_URL, download_url.c_str());
    curl_easy_setopt(dl_curl, CURLOPT_WRITEFUNCTION,
        static_cast<size_t(*)(void*, size_t, size_t, void*)>(file_write_cb));
    curl_easy_setopt(dl_curl, CURLOPT_WRITEDATA, &out_file);
    curl_easy_setopt(dl_curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(dl_curl, CURLOPT_TIMEOUT, 3600L);  // Allow up to 1 hour for large models

    struct curl_slist* headers = nullptr;
    headers = applyAuthHeader(headers);
    if (headers) {
        curl_easy_setopt(dl_curl, CURLOPT_HTTPHEADER, headers);
    }

    CURLcode res = curl_easy_perform(dl_curl);
    if (headers) { curl_slist_free_all(headers); }
    out_file.close();

    if (res != CURLE_OK) {
        std::filesystem::remove(local_path);
        throw std::runtime_error(std::string("Model download failed: ") + curl_easy_strerror(res));
    }

    long http_code = 0;
    curl_easy_getinfo(dl_curl, CURLINFO_RESPONSE_CODE, &http_code);
    if (http_code != 200) {
        std::filesystem::remove(local_path);
        throw std::runtime_error("Model download HTTP error: " + std::to_string(http_code));
    }

    // Verify SHA-256 after download
    std::string actual_sha256 = computeFileSha256(local_path);
    if (!expected_sha256.empty() && actual_sha256 != expected_sha256) {
        std::filesystem::remove(local_path);
        throw std::runtime_error(
            "SHA-256 mismatch after download – expected " + expected_sha256
            + ", got " + actual_sha256);
    }

    size_t file_size = static_cast<size_t>(std::filesystem::file_size(local_path));
    THEMIS_INFO("Model artifact downloaded ({} bytes, sha256={}): {}", 
        file_size, actual_sha256, local_path);

    return ModelDownloadResult{repo_id, filename, local_path, file_size, actual_sha256, false};
}

// ============================================================================
// Feature 4 – Prometheus per-batch metrics
// ============================================================================

void HuggingFaceIngestionPlugin::emitBatchMetrics(
    const BatchMetrics& m,
    const std::string& dataset_name,
    const std::string& split) const
{
    if (!config_.enable_metrics) {
        return;
    }

    auto& collector = observability::MetricsCollector::getInstance();
    using Labels = std::map<std::string, std::string>;
    Labels labels{{"dataset", dataset_name}, {"split", split}};

    // Counters
    collector.addCounter("hf_ingestion_rows_total",
        static_cast<int64_t>(m.rows_ingested), labels);
    collector.addCounter("hf_ingestion_batches_total",
        static_cast<int64_t>(m.batches_fetched), labels);
    collector.addCounter("hf_ingestion_cache_hits_total",
        static_cast<int64_t>(m.cache_hits), labels);
    collector.addCounter("hf_ingestion_cache_misses_total",
        static_cast<int64_t>(m.cache_misses), labels);

    // Rows/sec gauge (avoid division by zero)
    if (m.total_fetch_ms > 0.0) {
        double rows_per_sec = m.rows_ingested / (m.total_fetch_ms / 1000.0);
        collector.setGauge("hf_ingestion_rows_per_second", rows_per_sec, labels);
    }

    // Cache hit rate gauge
    size_t total_cache = m.cache_hits + m.cache_misses;
    if (total_cache > 0) {
        double hit_rate = static_cast<double>(m.cache_hits) / total_cache;
        collector.setGauge("hf_ingestion_cache_hit_rate", hit_rate, labels);
    }

    THEMIS_DEBUG("Metrics emitted: {} rows, {:.1f} ms, {}/{} cache hits",
        m.rows_ingested, m.total_fetch_ms, m.cache_hits, total_cache);
}

// ============================================================================
// Feature 5 – Multi-dataset parallel ingestion
// ============================================================================

std::vector<std::string> HuggingFaceIngestionPlugin::submitParallelDatasetJobs(
    const std::vector<DatasetSpec>& datasets,
    size_t concurrency)
{
    if (datasets.empty()) {
        throw std::invalid_argument("datasets must not be empty");
    }
    if (!content_manager_) {
        throw std::runtime_error("ContentManager not set");
    }

    // Clamp concurrency
    const size_t hw = std::max(1u, std::thread::hardware_concurrency());
    concurrency = std::max(size_t{1}, std::min(concurrency, hw));

    // Sort by priority descending (higher priority first)
    std::vector<size_t> order(datasets.size());
    std::iota(order.begin(), order.end(), 0);
    std::stable_sort(order.begin(), order.end(), [&](size_t a, size_t b) {
        return datasets[a].priority > datasets[b].priority;
    });

    // Generate job IDs in original order
    std::vector<std::string> job_ids(datasets.size());
    for (size_t i = 0; i < datasets.size(); ++i) {
        std::random_device rd;
        std::mt19937_64 rng(rd());
        std::ostringstream oss;
        oss << "hf_par_" << std::hex << std::setw(16) << std::setfill('0') << rng();
        job_ids[i] = oss.str();
    }

    // Launch thread pool
    std::vector<std::future<void>> futures;
    std::atomic<size_t> slot{0};

    auto worker = [&]() {
        while (true) {
            size_t idx = slot.fetch_add(1, std::memory_order_relaxed);
            if (idx >= order.size()) {
                break;
            }
            size_t original_idx = order[idx];
            const auto& spec = datasets[original_idx];
            const auto& job_id = job_ids[original_idx];

            THEMIS_INFO("Parallel ingestion starting: {} (job {})", 
                spec.dataset_name, job_id);
            try {
                // Build merged config
                Config merged = config_;
                if (!spec.split.empty()) {
                    merged.split = spec.split;
                }

                // Fetch all batches for this dataset
                std::vector<json> documents;
                size_t offset = 0;
                BatchMetrics metrics;
                bool from_cache = loadFromCache(spec.dataset_name, merged.split, documents);
                if (from_cache) {
                    ++metrics.cache_hits;
                } else {
                    ++metrics.cache_misses;
                    auto t0 = std::chrono::steady_clock::now();
                    while (true) {
                        auto result = fetchBatch(
                            spec.dataset_name, merged.split, offset, merged.chunk_size);
                        ++metrics.batches_fetched;
                        documents.insert(documents.end(),
                            result.documents.begin(), result.documents.end());
                        if (!result.has_more) { break; }
                        offset += result.documents.size();
                    }
                    auto t1 = std::chrono::steady_clock::now();
                    metrics.total_fetch_ms = static_cast<double>(
                        std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count());
                    saveToCache(spec.dataset_name, merged.split, documents);
                }

                // Ingest documents
                for (size_t i = 0; i < documents.size(); ++i) {
                    json content_spec = documentToContentSpec(
                        documents[i], spec.dataset_name, i);
                    auto status = content_manager_->importContent(content_spec, std::nullopt);
                    if (status.ok) { ++metrics.rows_ingested; }
                }

                emitBatchMetrics(metrics, spec.dataset_name, merged.split);
                THEMIS_INFO("Parallel ingestion done: {} ({} rows, job {})",
                    spec.dataset_name, metrics.rows_ingested, job_id);
            } catch (const std::exception& e) {
                THEMIS_ERROR("Parallel ingestion failed for {} (job {}): {}",
                    spec.dataset_name, job_id, e.what());
            }
        }
    };

    futures.reserve(concurrency);
    for (size_t t = 0; t < concurrency; ++t) {
        futures.push_back(std::async(std::launch::async, worker));
    }
    for (auto& f : futures) {
        f.get();  // propagate exceptions from workers
    }

    return job_ids;
}

} // namespace plugins
} // namespace themis


