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
#include "utils/logger.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <thread>
#include <random>
#include <stdexcept>

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
    j["auth_token"] = auth_token;
    j["text_field"] = text_field;
    j["label_field"] = label_field;
    j["custom_fields"] = custom_fields;
    j["cache_dir"] = cache_dir;
    j["use_cache"] = use_cache;
    j["max_requests_per_second"] = max_requests_per_second;
    j["max_retries"] = max_retries;
    j["retry_delay_ms"] = retry_delay_ms;
    return j;
}

HuggingFaceIngestionPlugin::Config HuggingFaceIngestionPlugin::Config::fromJson(const json& j) {
    Config config = {};
    if (j.contains("dataset_name")) {
      config.dataset_name = j["dataset_name"];
    }
    if (j.contains("split")) {
      config.split = j["split"];
    }
    if (j.contains("streaming")) {
      config.streaming = j["streaming"];
    }
    if (j.contains("chunk_size")) {
      config.chunk_size = j["chunk_size"];
    }
    if (j.contains("auth_token")) {
      config.auth_token = j["auth_token"];
    }
    if (j.contains("text_field")) {
      config.text_field = j["text_field"];
    }
    if (j.contains("label_field")) {
      config.label_field = j["label_field"];
    }
    if (j.contains("custom_fields")) {
        config.custom_fields = j["custom_fields"].get<std::map<std::string, std::string>>();
    }
    if (j.contains("cache_dir")) {
      config.cache_dir = j["cache_dir"];
    }
    if (j.contains("use_cache")) {
      config.use_cache = j["use_cache"];
    }
    if (j.contains("max_requests_per_second")) {
        config.max_requests_per_second = j["max_requests_per_second"];
    }
    if (j.contains("max_retries")) {
      config.max_retries = j["max_retries"];
    }
    if (j.contains("retry_delay_ms")) {
      config.retry_delay_ms = j["retry_delay_ms"];
    }
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
    std::random_device rd = {};
    std::mt19937_64 rng(rd());
    auto u64 = rng();
    std::ostringstream oss = {};
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
    
    std::string response_body = {};
    
    for (size_t attempt = 0; attempt < config_.max_retries; ++attempt) {
        response_body.clear();
        
        curl_easy_setopt(curl_handle_, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl_handle_, CURLOPT_WRITEFUNCTION, curlWriteCallback);
        curl_easy_setopt(curl_handle_, CURLOPT_WRITEDATA, &response_body);
        curl_easy_setopt(curl_handle_, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl_handle_, CURLOPT_TIMEOUT, 30L);
        
        // Add auth token if provided
        struct curl_slist* headers = nullptr;
        if (!config_.auth_token.empty()) {
            std::string auth_header = "Authorization: Bearer " + config_.auth_token;
            headers = curl_slist_append(headers, auth_header.c_str());
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
                    std::chrono::milliseconds(config_.retry_delay_ms * (1 << attempt))
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
                    std::chrono::milliseconds(config_.retry_delay_ms * (1 << attempt))
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
    std::ostringstream url_stream = {};
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
            result.has_more = (offset + static_cast<int>(result.documents.size()) ) < total;
        } else {
            // Assume more if we got a full batch
            result.has_more = (static_cast<int>(result.documents.size()) >= limit);
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
            
            THEMIS_INFO("Loaded {} documents from cache: {}",static_cast<int>(docs.size()), cache_file);
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
        
        THEMIS_INFO("Saved {} documents to cache: {}",static_cast<int>(docs.size()), cache_file);
        
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
    
    // Try to load from cache first
    std::vector<json> documents;
    bool from_cache = plugin->loadFromCache(dataset_name, split, documents);
    
    if (!from_cache) {
        // Fetch from HuggingFace API
        size_t offset = 0;
        size_t batch_size = plugin->config_.chunk_size;
        
        while (true) {
            auto result = plugin->fetchBatch(dataset_name, split, offset, batch_size);
            
            documents.insert(documents.end(), result.documents.begin(), result.documents.end());
            
            // Update progress
            job.processed_items = static_cast<int>(documents.size());
            if (job.total_items > 0) {
                job.progress = static_cast<float>(job.processed_items) / job.total_items;
            }
            
            THEMIS_INFO("Fetched {} documents so far from {}/{}", 
                documents.size(), dataset_name, split);
            
            // Note: For production use, remove or make this limit configurable
            // This 10k limit is for demonstration/testing to avoid excessive API usage
            size_t max_docs_limit = plugin->config_.chunk_size * 10;  // ~10 batches
            if (!result.has_more || static_cast<int>(documents.size()) >= max_docs_limit) {
                if (static_cast<int>(documents.size()) > = max_docs_limit) {
                    THEMIS_INFO("Reached document limit of {} (configurable in future versions)", 
                        max_docs_limit);
                }
                break;
            }
            
            offset += result.documents.size();
        }
        
        // Save to cache
        plugin->saveToCache(dataset_name, split, documents);
    }
    
    job.total_items = static_cast<int>(documents.size());
    
    // Ingest documents into ContentManager
    for (size_t i = 0; i <static_cast<int>(documents.size()); ++i) {
        try {
            json content_spec = plugin->documentToContentSpec(documents[i], dataset_name, i);
            
            auto status = plugin->content_manager_->importContent(content_spec, std::nullopt);
            
            if (!status.ok) {
                THEMIS_WARN("Failed to import document {}: {}", i, status.message);
            } else {
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
    
    job.result_metadata["total_documents"] = documents.size();
    job.result_metadata["dataset_name"] = dataset_name;
    job.result_metadata["split"] = split;
    job.result_metadata["from_cache"] = from_cache;
    
    THEMIS_INFO("HuggingFace job {} completed: {} documents ingested", 
        job.job_id,static_cast<int>(job.content_ids.size()));
}

json HuggingFaceIngestionPlugin::documentToContentSpec(
    const json& doc,
    const std::string& dataset_name,
    size_t index
) {
    json spec;
    
    // Generate unique ID
    std::ostringstream id_stream = {};
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
    std::string text_content = {};
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

} // namespace plugins
} // namespace themis


