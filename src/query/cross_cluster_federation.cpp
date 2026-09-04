/**
 * @file cross_cluster_federation.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=4, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#include "query/cross_cluster_federation.h"

#include <algorithm>
#include <future>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

#include <spdlog/spdlog.h>

#ifdef THEMIS_HAS_CURL
#include <curl/curl.h>
#endif

namespace themis::query {

// ============================================================================
// libcurl write callback (file-private)
// ============================================================================

namespace {

#ifdef THEMIS_HAS_CURL
// Maximum number of bytes accepted from a single cluster HTTP response.
// Prevents a rogue or compromised cluster from exhausting server memory.
static constexpr size_t kMaxResponseBytes = 64u * 1024u * 1024u; // 64 MiB

struct ResponseAccumulator {
    std::string* buffer;
    size_t       max_bytes;
    size_t       received{0};
};

static size_t curlWriteCallback(char* ptr, size_t size, size_t nmemb,
                                void* userdata) {
    if (size != 0 && nmemb > std::numeric_limits<size_t>::max() / size) {
        spdlog::error("CrossClusterFederator: nmemb*size would overflow; aborting");
        return 0;
    }
    const size_t total = size * nmemb;
    auto* acc = static_cast<ResponseAccumulator*>(userdata);
    if (acc->received + total > acc->max_bytes) {
        spdlog::error(
            "CrossClusterFederator: response exceeds {} byte limit; aborting",
            acc->max_bytes);
        return 0; // returning != total causes libcurl to abort with CURLE_WRITE_ERROR
    }
    acc->buffer->append(ptr, total);
    acc->received += total;
    return total;
}
#endif

} // anonymous namespace

// ============================================================================
// Construction
// ============================================================================

CrossClusterFederator::CrossClusterFederator()
    : CrossClusterFederator(Config{}) {}

CrossClusterFederator::CrossClusterFederator(const Config& config)
    : config_(config) {
    spdlog::info(
        "CrossClusterFederator: initialized (parallel={}, max_clusters={}, "
        "timeout={}ms, pruning_factor={})",
        config_.enable_parallel_execution,
        config_.max_parallel_clusters,
        config_.default_timeout_ms,
        config_.cost_pruning_factor);
}

// ============================================================================
// Cluster registry
// ============================================================================

void CrossClusterFederator::registerCluster(const ClusterEndpoint& endpoint) {
    if (endpoint.cluster_id.empty()) {
        throw std::invalid_argument(
            "CrossClusterFederator: cluster_id must not be empty");
    }
    if (endpoint.base_url.empty()) {
        throw std::invalid_argument(
            "CrossClusterFederator: base_url must not be empty for cluster '" +
            endpoint.cluster_id + "'");
    }
    const bool is_http  = endpoint.base_url.compare(0, 7,  "http://")  == 0;
    const bool is_https = endpoint.base_url.compare(0, 8,  "https://") == 0;
    if (!is_http && !is_https) {
        throw std::invalid_argument(
            "CrossClusterFederator: base_url must start with 'http://' or 'https://' "
            "for cluster '" + endpoint.cluster_id + "'");
    }
    if (endpoint.auth_token.find('\r') != std::string::npos ||
        endpoint.auth_token.find('\n') != std::string::npos) {
        throw std::invalid_argument(
            "CrossClusterFederator: auth_token must not contain CR/LF characters "
            "for cluster '" + endpoint.cluster_id + "'");
    }

    std::lock_guard<std::mutex> lock(registry_mutex_);
    clusters_[endpoint.cluster_id] = endpoint;
    spdlog::info("CrossClusterFederator: registered cluster '{}' at {}",
                 endpoint.cluster_id, endpoint.base_url);
}

void CrossClusterFederator::unregisterCluster(const std::string& cluster_id) {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    const auto it = clusters_.find(cluster_id);
    if (it != clusters_.end()) {
        clusters_.erase(it);
        latency_cache_.erase(cluster_id);
        spdlog::info("CrossClusterFederator: unregistered cluster '{}'",
                     cluster_id);
    }
}

std::vector<ClusterEndpoint> CrossClusterFederator::listClusters() const {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    std::vector<ClusterEndpoint> result = {};

    result.reserve(clusters_.size());
    for (const auto& [id, ep] : clusters_) {
        result.push_back(ep);
    }
    return result;
}

// ============================================================================
// Cost estimation
// ============================================================================

std::vector<ClusterCostEstimate>
CrossClusterFederator::estimateCosts(const std::string& /*query*/) const {
    std::lock_guard<std::mutex> lock(registry_mutex_);

    std::vector<ClusterCostEstimate> estimates = {};

    estimates.reserve(clusters_.size());

    for (const auto& [id, ep] : clusters_) {
        ClusterCostEstimate est;
        est.cluster_id = id;

        // Row-count estimate: prefer explicit hint, fall back to DEFAULT_ROWS
        est.estimated_rows = (ep.estimated_rows_hint > 0)
                                 ? ep.estimated_rows_hint
                                 : DEFAULT_ROWS;

        // Network latency: prefer measured cache, then hint, then DEFAULT
        auto lat_it = latency_cache_.find(id);
        if (lat_it != latency_cache_.end()) {
            est.network_latency_ms = lat_it->second;
        } else if (ep.network_latency_hint_ms > 0.0) {
            est.network_latency_ms = ep.network_latency_hint_ms;
        } else {
            est.network_latency_ms = DEFAULT_LATENCY_MS;
        }

        est.total_cost = (static_cast<double>(est.estimated_rows) * ROW_COST_WEIGHT) +
                         (est.network_latency_ms * LATENCY_COST_WEIGHT);
        est.should_include = true;

        estimates.push_back(est);
    }

    // Sort cheapest first
    std::sort(estimates.begin(), estimates.end(),
              [](const ClusterCostEstimate& a, const ClusterCostEstimate& b) {
                  return a.total_cost < b.total_cost;
              });

    return estimates;
}

CrossClusterFederator::ExecutionPlan
CrossClusterFederator::createExecutionPlan(const std::string& query) const {
    ExecutionPlan plan;
    plan.merge_strategy = "union";

    auto estimates = estimateCosts(query);
    plan.cost_estimates = estimates;

    // Apply cost-based pruning when enabled
    if (config_.cost_pruning_factor > 0.0 && !estimates.empty()) {
        const double cheapest = estimates.front().total_cost;
        const double threshold = cheapest * config_.cost_pruning_factor;
        for (auto& est : plan.cost_estimates) {
            if (est.total_cost > threshold) {
                est.should_include = false;
                spdlog::debug(
                    "CrossClusterFederator: pruning cluster '{}' "
                    "(cost={:.2f} > threshold={:.2f})",
                    est.cluster_id, est.total_cost, threshold);
            }
        }
    }

    for (const auto& est : plan.cost_estimates) {
        if (est.should_include) {
            plan.selected_clusters.push_back(est.cluster_id);
            plan.total_estimated_cost += est.total_cost;
        }
    }

    spdlog::info(
        "CrossClusterFederator: execution plan: {} of {} clusters selected, "
        "total_cost={:.2f}",
        plan.selected_clusters.size(), plan.cost_estimates.size(),
        plan.total_estimated_cost);

    return plan;
}

// ============================================================================
// Query execution
// ============================================================================

nlohmann::json CrossClusterFederator::execute(const std::string& query) {
    total_queries_++;

    // Snapshot of registered endpoints (to avoid holding lock during I/O)
    std::vector<ClusterEndpoint> endpoints;
    {
        std::lock_guard<std::mutex> lock(registry_mutex_);
        endpoints.reserve(clusters_.size());
        for (const auto& [id, ep] : clusters_) {
            endpoints.push_back(ep);
        }
    }

    if (endpoints.empty()) {
        spdlog::warn("CrossClusterFederator: no clusters registered");
        return nlohmann::json::array();
    }

    // Apply execution plan (respect cost-based pruning)
    auto plan = createExecutionPlan(query);
    const auto& selected = plan.selected_clusters;

    // Filter endpoints to only selected clusters
    std::vector<ClusterEndpoint> active = {};

    active.reserve(selected.size());
    for (const auto& ep : endpoints) {
        if (std::find(selected.begin(), selected.end(), ep.cluster_id) !=
            selected.end()) {
            active.push_back(ep);
        }
    }

    std::vector<nlohmann::json> shard_results;

    if (config_.enable_parallel_execution && active.size() > 1) {
        // Parallel execution via std::async
        const size_t n = std::min(active.size(),
                                  static_cast<size_t>(config_.max_parallel_clusters));
        std::vector<std::future<std::pair<bool, nlohmann::json>>> futures;
        futures.reserve(n);

        for (size_t i = 0; i < n; ++i) {
            futures.push_back(std::async(
                std::launch::async,
                [this, &active, &query, i]() -> std::pair<bool, nlohmann::json> {
                    bool ok = false;
                    auto result = queryCluster(active[i], query, ok);
                    return {ok, std::move(result)};
                }));
        }

        size_t failed = 0;
        for (auto& f : futures) {
            auto [ok, result] = f.get();
            if (ok) {
                shard_results.push_back(std::move(result));
            } else {
                failed++;
                failed_cluster_requests_++;
            }
        }

        if (failed == futures.size()) {
            if (!config_.skip_unreachable_clusters) {
                throw std::runtime_error(
                    "CrossClusterFederator: all cluster queries failed");
            }
            spdlog::error(
                "CrossClusterFederator: all {} cluster(s) failed", failed);
        }

    } else {
        // Sequential execution
        size_t failed = 0;
        for (const auto& ep : active) {
            bool ok = false;
            auto result = queryCluster(ep, query, ok);
            if (ok) {
                shard_results.push_back(std::move(result));
            } else {
                failed++;
                failed_cluster_requests_++;
            }
        }

        if (failed == active.size() && !active.empty()) {
            if (!config_.skip_unreachable_clusters) {
                throw std::runtime_error(
                    "CrossClusterFederator: all cluster queries failed");
            }
        }
    }

    auto merged = mergeResults(shard_results);
    successful_queries_++;

    spdlog::info(
        "CrossClusterFederator: query complete, {} results from {} clusters",
        merged.size(), shard_results.size());

    return merged;
}

// ============================================================================
// Statistics
// ============================================================================

nlohmann::json CrossClusterFederator::getStatistics() const {
    std::lock_guard<std::mutex> lock(registry_mutex_);

    nlohmann::json stats;
    stats["total_queries"]          = total_queries_.load();
    stats["successful_queries"]     = successful_queries_.load();
    stats["failed_cluster_requests"]= failed_cluster_requests_.load();
    stats["registered_clusters"]    = clusters_.size();

    nlohmann::json cluster_list = nlohmann::json::array();
    for (const auto& [id, ep] : clusters_) {
        nlohmann::json c;
        c["cluster_id"]              = id;
        c["base_url"]                = ep.base_url;
        c["estimated_rows_hint"]     = ep.estimated_rows_hint;
        c["network_latency_hint_ms"] = ep.network_latency_hint_ms;
        auto lat_it = latency_cache_.find(id);
        if (lat_it != latency_cache_.end()) {
            c["measured_latency_ms"] = lat_it->second;
        }
        cluster_list.push_back(c);
    }
    stats["clusters"] = cluster_list;

    stats["config"] = {
        {"enable_parallel_execution", config_.enable_parallel_execution},
        {"max_parallel_clusters",     config_.max_parallel_clusters},
        {"default_timeout_ms",        config_.default_timeout_ms},
        {"cost_pruning_factor",       config_.cost_pruning_factor}};

    return stats;
}

// ============================================================================
// Test seam
// ============================================================================

void CrossClusterFederator::setHttpPostForTesting(HttpPostFn fn) {
    http_post_fn_ = std::move(fn);
}

// ============================================================================
// Internal helpers
// ============================================================================

nlohmann::json CrossClusterFederator::queryCluster(
    const ClusterEndpoint& endpoint,
    const std::string&     query,
    bool&                  ok) {
    ok = false;

    const std::string url = endpoint.base_url + "/query/aql";
    const std::string auth_header =
        endpoint.auth_token.empty()
            ? ""
            : ("Authorization: Bearer " + endpoint.auth_token);

    const nlohmann::json request_body = {{"query", query}};
    const std::string    body_str     = request_body.dump();

    const uint32_t timeout =
        (endpoint.timeout_ms > 0) ? endpoint.timeout_ms
                                  : config_.default_timeout_ms;

    std::string response_body;
    int         status_code = 0;

    spdlog::debug("CrossClusterFederator: querying cluster '{}' at {}",
                  endpoint.cluster_id, url);

    if (http_post_fn_) {
        status_code = http_post_fn_(url, body_str, auth_header, timeout,
                                    response_body);
    } else {
        status_code = curlHttpPost(url, body_str, auth_header, timeout,
                                   response_body);
    }

    if (status_code == 0) {
        spdlog::error(
            "CrossClusterFederator: transport error for cluster '{}'",
            endpoint.cluster_id);
        return nlohmann::json::array();
    }
    if (status_code < 200 || status_code >= 300) {
        spdlog::error(
            "CrossClusterFederator: cluster '{}' returned HTTP {}",
            endpoint.cluster_id, status_code);
        return nlohmann::json::array();
    }

    try {
        auto parsed = nlohmann::json::parse(response_body);

        // Extract result array from the standard ThemisDB response envelope
        nlohmann::json results = nlohmann::json::array();
        if (parsed.contains("results") && parsed["results"].is_array()) {
            results = parsed["results"];
        } else if (parsed.is_array()) {
            results = parsed;
        } else {
            results.push_back(parsed);
        }

        ok = true;
        spdlog::debug(
            "CrossClusterFederator: cluster '{}' returned {} result(s)",
            endpoint.cluster_id, results.size());
        return results;

    } catch (const nlohmann::json::parse_error& e) {
        spdlog::error(
            "CrossClusterFederator: JSON parse error from cluster '{}': {}",
            endpoint.cluster_id, e.what());
        return nlohmann::json::array();
    }
}

nlohmann::json CrossClusterFederator::mergeResults(
    const std::vector<nlohmann::json>& shard_results) {
    nlohmann::json merged = nlohmann::json::array();
    for (const auto& sr : shard_results) {
        if (sr.is_array()) {
            for (const auto& item : sr) {
                merged.push_back(item);
            }
        } else if (!sr.is_null()) {
            merged.push_back(sr);
        }
    }
    return merged;
}

int CrossClusterFederator::curlHttpPost(const std::string& url,
                                        const std::string& body,
                                        const std::string& auth_header,
                                        uint32_t           timeout_ms,
                                        std::string&       response) {
    response.clear();

#ifndef THEMIS_HAS_CURL
    spdlog::warn(
        "CrossClusterFederator: libcurl not available; cannot reach {}",
        url);
    return 0;
#else
    CURL* curl = curl_easy_init();
    if (!curl) {
        spdlog::error(
            "CrossClusterFederator: failed to initialize libcurl handle");
        return 0;
    }

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "Accept: application/json");
    if (!auth_header.empty()) {
        headers = curl_slist_append(headers, auth_header.c_str());
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE,
                     static_cast<long>(body.size()));
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 3L);
    curl_easy_setopt(curl, CURLOPT_PROTOCOLS, CURLPROTO_HTTP | CURLPROTO_HTTPS);
    curl_easy_setopt(curl, CURLOPT_REDIR_PROTOCOLS, CURLPROTO_HTTP | CURLPROTO_HTTPS);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS,
                     static_cast<long>(timeout_ms));
    ResponseAccumulator acc{&response, kMaxResponseBytes};
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &acc);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

    const CURLcode res = curl_easy_perform(curl);

    int status_code = 0;
    if (res != CURLE_OK) {
        spdlog::error(
            "CrossClusterFederator: libcurl error for {}: {}",
            url, curl_easy_strerror(res));
    } else {
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

        // Update latency cache using the time info from curl (not exposed
        // here without a cluster_id reference; callers handle latency tracking)
        status_code = static_cast<int>(http_code);
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    return status_code;
#endif
}

} // namespace themis::query
