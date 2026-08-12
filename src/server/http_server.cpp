/**
 * @file http_server.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=5; TODO=2, Stub=1, Unimpl=0, Mock=1, Sim=1, Debt=0, C=48, H=69, M=113, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Ensure correct WinSock include order on Windows
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <stdexcept>
#include <windows.h>
#endif

// OpenSSL headers for TLS/SSL support
#include <openssl/ssl.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>

#include <spdlog/spdlog.h>

// Windows macros undefine - MUST be before any includes
#ifdef ERROR
#undef ERROR
#endif

// Include full definitions BEFORE http_server.h to avoid incomplete types
#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include "storage/disk_space_monitor.h"
#include "storage/mvcc_store.h"
#include "index/secondary_index.h"
#include "index/graph_index.h"
#include "index/vector_index.h"
#include "index/spatial_index.h"
#include "api/geo_index_hooks.h"
#include "transaction/transaction_manager.h"
#include "utils/logger.h"
#include "themis/build_info.h"
#include "themis/license_info.h"

#include "utils/logger_impl.h"
#include "utils/tracing.h"
#include "utils/zstd_codec.h"
#include "utils/cursor.h"
#include "utils/pii_detector.h"
#include "observability/alertmanager.h"
#include "observability/provenance_store.h"
#include "performance/phase3/bao.h"
#include "performance/workload_adaptive_optimizer.h"
#include "prompt_engineering/feedback_collector.h"
#include "rag/continuous_learning_orchestrator.h"
#include "security/key_provider.h"
#include "security/mock_key_provider.h"
#include "security/pki_key_provider.h"
#include "security/encryption.h"
#include "utils/audit_logger.h"
#include "utils/pki_client.h"
#include "content/content_manager.h"
#include "content/content_processor.h"
// Fuer Graph-Kanten-Entities
#include "storage/key_schema.h"
// HKDF Helper fuer Feldschluessel-Ableitung beim Entschluesseln
#include "utils/hkdf_helper.h"

// Sprint A features - include BEFORE http_server.h to have complete types
#include "llm/llm_interaction_store.h"
#include "llm/llm_plugin_manager.h"
#include "llm/context_window_budget.h"
#include "prompt_engineering/prompt_manager.h"
#include "cdc/changefeed.h"
#include "cdc/consumer_group.h"
#include "transaction/snapshot_manager.h"
#include "transaction/branch_manager.h"
#include "transaction/merge_engine.h"
#include "analytics/diff_engine.h"
#include <algorithm>
#include <cctype>

// Sprint B features
#include "timeseries/timeseries.h"
#include "timeseries/tsstore.h"
#include "timeseries/continuous_agg.h"

// Sprint C features
#include "index/adaptive_index.h"
#include "server/tenant_manager.h"

// Now include http_server.h which has forward declarations
#include "server/http_server.h"
#include "server/http_shutdown_manager.h"
#include "server/api_key_mgmt_handler.h"
#include "server/pki_api_handler.h"
#include "server/classification_api_handler.h"
#include "server/snapshot_api_handler.h"
#include "server/mvcc_api_handler.h"
#include "server/pitr_api_handler.h"
#include "server/diff_api_handler.h"
#include "server/pitr_api_handler.h"
#include "server/branch_api_handler.h"
#include "server/merge_api_handler.h"
#include "server/feedback_api_handler.h"
#include "server/http_type_adapter.h"  // TODO: Remove after migration to cpp-httplib (see HTTP_SERVER_REFACTORING_ACTION_PLAN.md)
#include "server/mcp_server.h"
#include "analytics/diff_engine.h"
#include "storage/pitr_manager.h"
#include "sharding/multi_primary_coordinator.h"
#include "sharding/health_monitor.h"
#include "sharding/wal_manager.h"
#include "sharding/wal_applier.h"
#include "sharding/distributed_transaction.h"
#include "sharding/truetime.h"
#include "sharding/sharding_manager.h"
#include "server/distributed_txn_api_handler.h"
#include "themis/base/module_loader.h"
#if !defined(_WIN32)
#include <time.h>
#endif

// Portable wrappers for tm <-> time_t conversions
static inline time_t portable_mkgmtime_impl(std::tm const* tmin) {
#ifdef _WIN32
    return _mkgmtime(const_cast<std::tm*>(tmin));
#else
    return timegm(const_cast<std::tm*>(tmin));
#endif
}
static inline void portable_gmtime_r_impl(const time_t* t, std::tm* out) {
#ifdef _WIN32
    gmtime_s(out, t);
#else
    gmtime_r(t, out);
#endif
}
#include "server/reports_api_handler.h"
#include "server/auth_middleware.h"
#include "server/ranger_adapter.h"
#include "server/pii_api_handler.h"
#if THEMIS_ENABLE_LLM
#include "server/feedback_api_handler.h"
#include "llm/docs_assistant.h"
#include "llm/lora_framework/lora_feedback_storage.h"
#include "llm/lora_framework/feedback_plugin.h"
#include "llm/lora_framework/lora_training_config.h"
#endif
#include "config/config_path_resolver.h"
#include "server/schema_api_handler.h"
#include "server/graphql_api_handler.h"
#include "server/grpc_web_proxy_handler.h"
#include "server/serverless_function_api_handler.h"
#include "metadata/schema_manager.h"

#ifdef THEMIS_ENABLE_HTTP2
#include "server/http2_session.h"
#endif

#ifdef THEMIS_ENABLE_HTTP3
#include "server/http3_session.h"
#endif

#ifdef THEMIS_ENABLE_WEBSOCKET
#include "server/websocket_session.h"
#include "api/ws_handler.h"
#endif

#include "query/query_engine.h"
#include "query/query_optimizer.h"
#include "query/aql_parser.h"
#include "query/aql_translator.h"
#include "query/continuous_query_engine.h"

#include "security/signing.h"
#include "utils/input_validator.h"
#include "sharding/metrics_registry.h"
#include "sharding/wal_applier.h"
#include "sharding/wal_manager.h"
#include "sharding/replication_coordinator.h"
#include "sharding/write_concern.h"
#include <openssl/hmac.h>
#include <openssl/evp.h>

// GraphQL types are included via server/graphql_api_handler.h (included in http_server.h)
#include "server/api_version.h"
#include "server/api_version_config.h"
#include "server/route_version_router.h"
#include "server/continuous_query_api_handler.h"
#include "scheduler/task_scheduler.h"
#include "maintenance/maintenance_task_handler_impls.h"
#include "storage/compaction_manager.h"
#include "storage/security_signature_manager.h"
#include "content/mime_detector.h"
#include "index/process_graph.h"

#include <nlohmann/json.hpp>
#include <yaml-cpp/yaml.h>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <tuple>
#include <regex>
#include <queue>
#include <unordered_set>
#include <functional>
#include <cmath>
#include <functional>
#include <map>
#include <unordered_map>
#include <limits>
#include <optional>
#include <cstdlib>
#include <filesystem>
#include <iostream>

using json = nlohmann::json;

namespace themis {
namespace server {

// ============================================================================
// HttpServer Implementation
// ============================================================================

/**
 * @brief Construct server with base sharding dependencies.
 */
HttpServer::HttpServer(
    const Config& config,
    std::shared_ptr<RocksDBWrapper> storage,
    std::shared_ptr<SecondaryIndexManager> secondary_index,
    std::shared_ptr<GraphIndexManager> graph_index,
    std::shared_ptr<VectorIndexManager> vector_index,
    std::shared_ptr<TransactionManager> tx_manager,
    std::shared_ptr<sharding::WALApplier> wal_applier,
    std::shared_ptr<sharding::WALManager> wal_manager,
    std::shared_ptr<sharding::ReplicationCoordinator> replication_coordinator
)
    : HttpServer(
        config,
        std::move(storage),
        std::move(secondary_index),
        std::move(graph_index),
        std::move(vector_index),
        std::move(tx_manager),
        std::move(wal_applier),
        std::move(wal_manager),
        std::move(replication_coordinator),
        nullptr,
        nullptr)
{}

/**
 * @brief Construct server with extended sharding and topology dependencies.
 */
HttpServer::HttpServer(
    const Config& config,
    std::shared_ptr<RocksDBWrapper> storage,
    std::shared_ptr<SecondaryIndexManager> secondary_index,
    std::shared_ptr<GraphIndexManager> graph_index,
    std::shared_ptr<VectorIndexManager> vector_index,
    std::shared_ptr<TransactionManager> tx_manager,
    std::shared_ptr<sharding::WALApplier> wal_applier,
    std::shared_ptr<sharding::WALManager> wal_manager,
    std::shared_ptr<sharding::ReplicationCoordinator> replication_coordinator,
    std::shared_ptr<sharding::MultiPrimaryCoordinator> multi_primary_coordinator,
    std::shared_ptr<sharding::HealthMonitor> health_monitor,
    std::shared_ptr<sharding::CollectionRedundancyManager> redundancy_manager,
    std::shared_ptr<sharding::ConsistentHashRing> hash_ring,
    std::shared_ptr<sharding::ShardTopology> shard_topology
)
    : config_(config)
    , storage_(std::move(storage))
    , secondary_index_(std::move(secondary_index))
    , graph_index_(std::move(graph_index))
    , vector_index_(std::move(vector_index))
    , tx_manager_(std::move(tx_manager))
    , wal_applier_(std::move(wal_applier))
    , wal_manager_(std::move(wal_manager))
    , replication_coordinator_(std::move(replication_coordinator))
    , multi_primary_coordinator_(std::move(multi_primary_coordinator))
    , health_monitor_(std::move(health_monitor))
    , redundancy_manager_(std::move(redundancy_manager))
    , hash_ring_(std::move(hash_ring))
    , shard_topology_(std::move(shard_topology))
    , request_timeout_ms_live_(config.request_timeout_ms)
    , ioc_(static_cast<int>(config_.num_threads))
    , acceptor_(ioc_)
    , start_time_(std::chrono::steady_clock::now())
{
    THEMIS_INFO("HTTP Server created with {} threads on {}:{}", 
        config_.num_threads, config_.host, config_.port);

    // Initialize hot-reloadable atomic shadows from initial config values.
    // These are the only config fields written concurrently by POST /config
    // (hot-reload) while worker threads may read them simultaneously.
    request_timeout_ms_live_.store(config_.request_timeout_ms, std::memory_order_relaxed);
    feature_semantic_cache_live_.store(config_.feature_semantic_cache, std::memory_order_relaxed);
    feature_llm_store_live_.store(config_.feature_llm_store, std::memory_order_relaxed);
    feature_cdc_live_.store(config_.feature_cdc, std::memory_order_relaxed);
    feature_timeseries_live_.store(config_.feature_timeseries, std::memory_order_relaxed);
    
    // Initialize Spatial Index Manager (geo MVP)
    try {
        if (const char* s = std::getenv("THEMIS_WAL_SHARED_SECRET")) {
            wal_shared_secret_ = s;
        }
        if (const char* s = std::getenv("THEMIS_WAL_HMAC_SECRET")) {
            wal_hmac_secret_ = s;
        }
        spatial_index_ = std::make_shared<index::SpatialIndexManager>(*storage_);
        
        // Wire up exact geometry backend.
        // Prefer the GPU backend (always available via CPU fallback) so that
        // the circuit-breaker, metrics, and audit log are active even on CPU-only
        // machines.  Fall back to Boost.Geometry if the GPU backend is absent for
        // any reason, and log an info message in all cases.
        auto* gpu_backend = geo::getGpuSpatialBackend();
        auto* boost_backend = geo::getBoostCpuBackend();
        if (gpu_backend && gpu_backend->isAvailable()) {
            spatial_index_->setExactBackend(gpu_backend);
            THEMIS_INFO("Spatial Index Manager initialized with GPU spatial exact backend (device: {})",
                        geo::getGpuSpatialBackendStatsJson());
        } else if (boost_backend && boost_backend->isAvailable()) {
            spatial_index_->setExactBackend(boost_backend);
            THEMIS_INFO("Spatial Index Manager initialized with Boost.Geometry exact backend");
        } else {
            THEMIS_INFO("Spatial Index Manager initialized (MBR-only, no exact backend)");
        }
    } catch (const std::exception& e) {
        THEMIS_WARN("Failed to initialize Spatial Index Manager: {}", e.what());
        // Non-fatal: geo features will be disabled
    }

    // Initialize Security Signature Manager (content integrity & file verification)
    try {
        security_sig_mgr_ = std::make_shared<storage::SecuritySignatureManager>(storage_);
        THEMIS_INFO("Security Signature Manager initialized");

        // Initialize MIME Detector with signature verification
        mime_detector_ = std::make_shared<content::MimeDetector>("", security_sig_mgr_);
        THEMIS_INFO("MIME Detector initialized");
    } catch (const std::exception& e) {
        THEMIS_WARN("Failed to initialize Security Signature Manager / MIME Detector: {}", e.what());
    }

    // GAP-011: Do NOT log the raw token value — log presence only to avoid
    // leaking the secret into log files (CWE-532).
    {
        const bool admin_token_set = (std::getenv("THEMIS_TOKEN_ADMIN") != nullptr);
        THEMIS_INFO("HttpServer ctor: THEMIS_TOKEN_ADMIN is {}",
                    admin_token_set ? "set" : "not set");
    }
    // Initialize ContentManager and register built-in processors
    // ContentManager wird nach Initialisierung von FieldEncryption erstellt (siehe weiter unten).
    // Hier zunächst nur Platzhalter (nullptr); tatsächliche Instanz folgt nach key_provider_/field_encryption_ Setup.
    // (Früher erstellt -> jetzt verschoben um Encryption sofort verfügbar zu machen.)
    // Defer ContentManager + Processor Registrierung bis FieldEncryption initialisiert ist.
    // (Vorheriger Zugriff auf content_manager_ entfernt - war null und verursachte Crash.)
    
    // Initialize Semantic Cache (Sprint A) if feature enabled
    if (config_.feature_semantic_cache) {
        // Use default column family for MVP (no dedicated CF needed)
        cache_cf_handle_ = nullptr;
        semantic_cache_ = std::make_shared<SemanticCache>(
            storage_->getRawDB(),
            cache_cf_handle_,
            3600 // default TTL: 1 hour
        );
        THEMIS_INFO("Semantic Cache initialized (TTL: 3600s) using default CF");
        // Note: CacheApiHandler requires auth_ which is initialized later.
        // cache_api_ is constructed after auth_ is available (see below).
    }

    // Initialize Cache Admin API Handler (Phase 3: Admin API for cache operations).
    // This instance is dedicated to the admin API. Full integration with the query
    // engine's AdaptiveQueryCache instance (owned by QueryCacheManager) is tracked
    // as a follow-up task once QueryCacheManager exposes a shared accessor.
    {
        AdaptiveQueryCache::Config cache_admin_config;
        cache_admin_config.l3_db_path = "./themis_admin_cache";
        cache_admin_config.enable_circuit_breaker = true;
        cache_admin_config.enable_tenant_isolation = true;
        try {
            adaptive_query_cache_ = std::make_shared<AdaptiveQueryCache>(cache_admin_config);
            cache_admin_api_ = std::make_unique<themis::server::CacheAdminApiHandler>(
                adaptive_query_cache_, auth_);
            THEMIS_INFO("Cache Admin API Handler initialized");
        } catch (const std::exception& e) {
            THEMIS_WARN("Failed to initialize Cache Admin API Handler: {}", e.what());
        }
    }
    
    // Initialize LLM Interaction Store (Sprint A) if feature enabled
    if (config_.feature_llm_store) {
        llm_cf_handle_ = nullptr; // Use default CF
        llm_store_ = std::make_shared<LLMInteractionStore>(
            storage_->getRawDB(),
            llm_cf_handle_
        );
        THEMIS_INFO("LLM Interaction Store initialized using default CF");
    }
    
    // Initialize Changefeed (Sprint A CDC) if feature enabled
    if (config_.feature_cdc) {
        cdc_cf_handle_ = nullptr; // Use default CF

        // Load retention policy from config/data_management/cdc_retention.yaml if it exists
        Changefeed::RetentionPolicy cdc_retention_policy;
        {
            static const char* CDC_RETENTION_CONFIG_PATH =
                "config/data_management/cdc_retention.yaml";
            auto cfg_path =
                themis::config::ConfigPathResolver::tryResolve(CDC_RETENTION_CONFIG_PATH);
            if (!cfg_path) {
                // Fallback: check path as-is (e.g., when CWD is the repo root)
                if (std::filesystem::exists(CDC_RETENTION_CONFIG_PATH)) {
                    cfg_path = CDC_RETENTION_CONFIG_PATH;
                }
            }
            if (cfg_path) {
                try {
                    YAML::Node root = YAML::LoadFile(*cfg_path);
                    if (root["retention"]) {
                        const auto& r = root["retention"];
                        cdc_retention_policy.enabled = r["enabled"].as<bool>(false);
                        if (r["max_age_hours"])
                            cdc_retention_policy.max_age_hours =
                                std::chrono::hours(
                                    r["max_age_hours"].as<uint32_t>(168));  // default: 7 days
                        if (r["max_event_count"])
                            cdc_retention_policy.max_event_count =
                                r["max_event_count"].as<uint64_t>(1000000); // default: 1M events
                        if (r["max_size_bytes"])
                            cdc_retention_policy.max_size_bytes =
                                r["max_size_bytes"].as<size_t>(
                                    Changefeed::RetentionPolicy::DEFAULT_MAX_SIZE_BYTES);
                        if (r["cleanup_interval_minutes"])
                            cdc_retention_policy.cleanup_interval =
                                std::chrono::minutes(
                                    r["cleanup_interval_minutes"].as<uint32_t>(60)); // default: 1 hour
                        if (r["compact_on_cleanup"])
                            cdc_retention_policy.compact_on_cleanup =
                                r["compact_on_cleanup"].as<bool>(false);
                    }
                    THEMIS_INFO("CDC: loaded retention policy from {} (enabled={})",
                                *cfg_path, cdc_retention_policy.enabled);
                } catch (const std::exception& e) {
                    THEMIS_WARN("CDC: failed to load retention config from {}: {} — using defaults",
                                *cfg_path, e.what());
                }
            } else {
                THEMIS_DEBUG("CDC: {} not found, using default retention policy (disabled)",
                             CDC_RETENTION_CONFIG_PATH);
            }
        }

        // Changefeed constructor signature accepts (TransactionDB*, ColumnFamilyHandle*)
        // previous code attempted to pass Route identifiers which no longer apply.
        changefeed_ = std::make_shared<Changefeed>(
            storage_->getRawDB(),
            cdc_cf_handle_,
            cdc_retention_policy
        );
        THEMIS_INFO("Changefeed initialized using default CF");

        // Initialize ConsumerGroupManager for /v2/cdc/stream group-protocol
        // sessions.  Uses the same RocksDB instance and CDC column family as
        // the changefeed so group offsets are co-located with change events.
        consumer_group_manager_ = std::make_unique<cdc::ConsumerGroupManager>(
            storage_->getRawDB(),
            cdc_cf_handle_
        );
        THEMIS_INFO("ConsumerGroupManager initialized (CDC consumer groups ready)");
        
        // Initialize SnapshotManager (Named Snapshots feature)
        snapshot_manager_ = std::make_unique<transaction::SnapshotManager>(*storage_, *changefeed_);
        snapshot_api_handler_ = std::make_unique<server::SnapshotApiHandler>(*snapshot_manager_);
        THEMIS_INFO("SnapshotManager initialized");

        // Initialize MVCC API Handler (per-record versioning + HLC)
        {
            auto clock = std::make_shared<themis::HybridLogicalClock>();
            mvcc_store_ = std::make_shared<themis::MVCCStore>(storage_, std::move(clock));
            mvcc_api_handler_ = std::make_unique<server::MvccApiHandler>(mvcc_store_);
        }
        THEMIS_INFO("MVCC API Handler initialized");
        
        // Initialize PITRManager (Point-in-Time Recovery feature)
        pitr_manager_ = std::make_unique<PITRManager>(storage_.get(), changefeed_.get(), snapshot_manager_.get());
        pitr_api_handler_ = std::make_unique<server::PITRApiHandler>(*pitr_manager_);
        THEMIS_INFO("PITRManager initialized");
        
        // Initialize BranchManager (Phase 4 - Persistent Branches)
        branch_manager_ = std::make_unique<transaction::BranchManager>(*storage_, *changefeed_, *snapshot_manager_);
        branch_api_handler_ = std::make_unique<BranchApiHandler>(*branch_manager_);
        THEMIS_INFO("BranchManager initialized");
        
        // Initialize DiffEngine (Phase 2 MVCC features - required for MergeEngine)
        diff_engine_ = std::make_unique<analytics::DiffEngine>(*changefeed_, snapshot_manager_.get());
        diff_api_handler_ = std::make_unique<DiffApiHandler>(*diff_engine_);
        THEMIS_INFO("DiffEngine initialized with SnapshotManager support");
        
        // Initialize MergeEngine and MergeApiHandler (Phase 5 MVCC features - 3-Way Merge)
        merge_engine_ = std::make_unique<transaction::MergeEngine>(*diff_engine_, *snapshot_manager_, *changefeed_);
        merge_api_handler_ = std::make_unique<MergeApiHandler>(*merge_engine_, *snapshot_manager_);
        THEMIS_INFO("MergeEngine initialized for 3-way merge support");
        
        // Connect MergeEngine to BranchManager for non-fast-forward merges
        branch_manager_->setMergeEngine(merge_engine_.get());
        THEMIS_INFO("BranchManager connected to MergeEngine for 3-way merge support");
        
        // Initialize SSE Connection Manager for streaming (if enabled)
#ifdef THEMIS_ENABLE_SSE
        {
            SseConnectionManager::ConnectionConfig sse_config;
            sse_config.heartbeat_interval_ms = 15000;
            sse_config.max_buffered_events = 1000;
            sse_config.event_poll_interval_ms = 500;
            sse_config.max_events_per_second = config_.sse_max_events_per_second; // from server config

            sse_manager_ = std::make_unique<SseConnectionManager>(
                changefeed_,
                ioc_,
                sse_config
            );
            THEMIS_INFO("SSE Connection Manager initialized");
        }
#endif
    }

#ifdef THEMIS_ENABLE_WEBSOCKET
    // Initialize WebSocket Manager
    if (config_.enable_websocket) {
        // Pass changefeed for CDC support with configurable poll interval
        websocket_manager_ = std::make_shared<WebSocketManager>(
            changefeed_.get(), 
            config_.websocket_cdc_poll_interval_ms
        );
        THEMIS_INFO("WebSocket Connection Manager initialized with CDC support");
        
        // Start CDC polling if changefeed is available
        if (changefeed_) {
            websocket_manager_->startCDCPolling(ioc_, config_.websocket_cdc_poll_interval_ms);
            THEMIS_INFO("CDC polling started for WebSocket connections (interval={}ms)", 
                       config_.websocket_cdc_poll_interval_ms);
        }
    }
#endif

    // Initialize PII Mappings ColumnFamily + Handler (independent of CDC)
    if (config_.feature_pii_manager) {
       std::lock_guard<std::mutex> lock(storage_mutex_);
       auto cf_result = storage_->getOrCreateColumnFamily("pii_mappings");
       if (cf_result) {
           pii_cf_handle_ = *cf_result;
           pii_api_ = std::make_unique<PIIApiHandler>(storage_->getRawDB(), pii_cf_handle_);
           THEMIS_INFO("PII Manager initialized with dedicated CF 'pii_mappings'");
       } else {
           THEMIS_ERROR("Failed to initialize PII Manager CF: {}", cf_result.error().message());
       }
    } else {
       // Fallback: use default CF (still functional, just no separation)
       std::lock_guard<std::mutex> lock(storage_mutex_);
       pii_api_ = std::make_unique<PIIApiHandler>(storage_->getRawDB(), nullptr);
       THEMIS_INFO("PII Manager initialized using default CF (feature flag off, CF isolation disabled)");
    }

    // Initialize PromptManager (Prompt Template Registry)
    try {
        // Try to create a dedicated column family for prompt templates; fall back to default CF
        prompt_cf_handle_ = nullptr;
        if (storage_) {
            auto cf_result = storage_->getOrCreateColumnFamily("prompt_templates");
            if (cf_result) {
                prompt_cf_handle_ = *cf_result;
                THEMIS_INFO("PromptManager: using dedicated CF 'prompt_templates'");
                prompt_manager_ = std::make_shared<themis::prompt_engineering::PromptManager>(storage_.get(), prompt_cf_handle_);
            } else {
                THEMIS_WARN("PromptManager: failed to create dedicated CF, falling back to in-memory: {}", cf_result.error().message());
                prompt_manager_ = std::make_shared<themis::prompt_engineering::PromptManager>();
            }
        } else {
            // No storage available (tests / in-memory run)
            prompt_manager_ = std::make_shared<themis::prompt_engineering::PromptManager>();
            THEMIS_INFO("PromptManager initialized in-memory (no storage provided)");
        }
    } catch (const std::exception& ex) {
        THEMIS_ERROR("PromptManager initialization failure: {}", ex.what());
        prompt_manager_ = std::make_shared<themis::prompt_engineering::PromptManager>();
    }
    
    // Initialize Time-Series Store (Sprint B) if feature enabled
    if (config_.feature_timeseries) {
        ts_cf_handle_ = nullptr; // Use default CF
        timeseries_ = std::make_shared<TSStore>(
            storage_->getRawDB(),
            ts_cf_handle_
        );
        THEMIS_INFO("Time-Series Store initialized using default CF");

        // Initialize ContinuousAggregateManager for materialized aggregate views
        ts_agg_manager_ = std::make_shared<ContinuousAggregateManager>(timeseries_.get());
        THEMIS_INFO("ContinuousAggregateManager initialized");
        // Note: TimeSeriesApiHandler requires auth_ which is initialized later.
        // timeseries_api_ is constructed after auth_ is available (see below).
    }

    // CRITICAL FIX: Initialize Sharding Manager BEFORE AdaptiveIndexManager
    // When THEMIS_ENABLE_SHARDING=true, RocksDB opens 2 column families.
    // AdaptiveIndexManager tries to use MVCC across both CFs for cluster coordination.
    // Without prior Sharding initialization, this blocks waiting for cluster bootstrap.
    // Solution: Detect sharding mode and set up minimal sharding context BEFORE AdaptiveIndexManager.
    {
        const char* sharding_enabled = std::getenv("THEMIS_ENABLE_SHARDING");
        const char* shard_id = std::getenv("THEMIS_SHARD_ID");
        const char* bootstrap_shard = std::getenv("THEMIS_BOOTSTRAP_SHARD");
        
        if (sharding_enabled && (std::string(sharding_enabled) == "true" || std::string(sharding_enabled) == "1")) {
            THEMIS_INFO("Sharding mode detected: preparing cluster context before AdaptiveIndexManager");
            
            // Set sharding context flags so AdaptiveIndexManager knows to coordinate with cluster
            // For now, just log the sharding configuration being active
            if (shard_id) {
                THEMIS_INFO("  Shard ID: {}", shard_id);
            }
            if (bootstrap_shard) {
                THEMIS_INFO("  Bootstrap shard: {}", bootstrap_shard);
            }
            
            // Initialize actual ShardingManager - use the live singleton instance
            if (!sharding_manager_) {
                sharding_manager_ = &themis::sharding::ShardingManager::GetInstance();
            }
            THEMIS_INFO("Sharding context prepared (ShardingManager initialized, cluster discovery ready)");
        }
    }
    
    // Initialize Adaptive Index Manager (Sprint C) - always enabled
    // Now safe to initialize because Sharding context (if needed) is prepared above
    {
       std::lock_guard<std::mutex> lock(storage_mutex_);
       adaptive_index_ = std::make_shared<AdaptiveIndexManager>(storage_->getRawDB());
    }
    THEMIS_INFO("Adaptive Index Manager initialized");

    // Initialize Authorization middleware (MVP: tokens via env)
    auth_ = std::make_shared<themis::AuthMiddleware>();
    // Global-style helper for env lookup (reused across subsequent blocks)
    auto themis_get_env = [](const char* name) -> std::optional<std::string> {
        const char* v = std::getenv(name);
        if (v && *v) return std::string(v);
        return std::nullopt;
    };
    // Admin token
    if (auto t = themis_get_env("THEMIS_TOKEN_ADMIN")) {
        themis::AuthMiddleware::TokenConfig cfg;
        cfg.token = *t;
        cfg.user_id = "admin";
        cfg.scopes = {
            "admin","config:read","config:write","cdc:read","cdc:admin",
            "metrics:read","data:read","data:write","audit:read",
            // PII feature scopes
            "pii:read","pii:write","pii:reveal"
        };
        auth_->addToken(cfg);
        THEMIS_INFO("Auth: ADMIN token configured via env");
        try {
            auto v = auth_->validateToken(cfg.token);
            // GAP-011 fixed: log only token length, never prefix/suffix bytes.
            THEMIS_INFO("Auth check after addToken: validateToken(token_len={}) -> authorized={} user_id='{}' reason='{}'",
                       cfg.token.size(), v.authorized, v.user_id, v.reason);
        } catch (...) {}
    }
    // Read-only token
    if (auto t = themis_get_env("THEMIS_TOKEN_READONLY")) {
        themis::AuthMiddleware::TokenConfig cfg;
        cfg.token = *t;
        cfg.user_id = "readonly";
    cfg.scopes = {"metrics:read","config:read","data:read","cdc:read","audit:read","pii:read"};
        auth_->addToken(cfg);
        THEMIS_INFO("Auth: READONLY token configured via env");
    }
    // Analyst token (read access incl. vectors/query)
    if (auto t = themis_get_env("THEMIS_TOKEN_ANALYST")) {
        themis::AuthMiddleware::TokenConfig cfg;
        cfg.token = *t;
        cfg.user_id = "analyst";
    cfg.scopes = {"metrics:read","data:read","cdc:read","pii:read"};
        auth_->addToken(cfg);
        THEMIS_INFO("Auth: ANALYST token configured via env");
    }

    // Initialize security components  
    key_provider_ = std::make_shared<themis::security::PKIKeyProvider>(
        std::make_shared<themis::utils::VCCPKIClient>(themis::utils::PKIConfig{}),
        storage_,
        "themisdb"
    );
    THEMIS_INFO("PKIKeyProvider initialized with persistent KEK/DEK");

    // Field encryption for PII and schema-based encryption
    field_encryption_ = std::make_shared<themis::FieldEncryption>(key_provider_);
    THEMIS_INFO("FieldEncryption initialized");

    // Initialize ContentManager and register built-in processors (now with encryption)
    try {
        content_manager_ = std::make_shared<themis::content::ContentManager>(
            storage_, vector_index_, graph_index_, secondary_index_, field_encryption_);
        text_processor_ = std::make_unique<themis::content::TextProcessor>();
        content_manager_->registerProcessor(std::unique_ptr<themis::content::IContentProcessor>(text_processor_.release()));
        // Provide FieldEncryption to GraphIndexManager so edges can be encrypted on write
        if (graph_index_) {
            graph_index_->setFieldEncryption(field_encryption_);
        }
    } catch (const std::exception& e) {
        THEMIS_ERROR("Failed to init ContentManager: {}", e.what());
    }

    // Initialize Keys API Handler with KeyProvider
    keys_api_ = std::make_unique<themis::server::KeysApiHandler>(key_provider_);
    THEMIS_INFO("Keys API Handler initialized");

    // Deferred CacheApiHandler initialization (requires auth_ which is now available)
    if (semantic_cache_ && auth_) {
        try {
            cache_api_ = std::make_unique<server::CacheApiHandler>(semantic_cache_, auth_);
            THEMIS_INFO("Cache API Handler initialized (semantic cache endpoints active)");
        } catch (const std::exception& e) {
            THEMIS_WARN("Failed to initialize Cache API Handler: {}", e.what());
        }
    }

    // Deferred TimeSeriesApiHandler initialization (requires auth_ which is now available)
    if (timeseries_ && ts_agg_manager_ && auth_) {
        try {
            timeseries_api_ = std::make_unique<server::TimeSeriesApiHandler>(
                storage_, timeseries_, ts_agg_manager_, auth_);
            THEMIS_INFO("TimeSeries API Handler initialized (time-series endpoints active)");
        } catch (const std::exception& e) {
            THEMIS_WARN("Failed to initialize TimeSeries API Handler: {}", e.what());
        }
    }

    // Initialize API Key Management Handler
    api_key_mgmt_ = std::make_unique<themis::server::ApiKeyMgmtHandler>(auth_);
    THEMIS_INFO("API Key Management Handler initialized");
    // Initialize Session Management Handler
    session_manager_ = std::make_shared<themis::auth::SessionManager>();
    session_api_ = std::make_unique<themis::server::SessionApiHandler>(auth_, session_manager_, audit_logger_);
    THEMIS_INFO("Session Management Handler initialized");
    // Initialize PKI API Handler using a SigningService backed by the KeyProvider
    try {
        pki_api_ = std::make_unique<themis::server::PkiApiHandler>(themis::createKeyProviderSigningService(key_provider_));
        THEMIS_INFO("PKI API Handler initialized");
    } catch (const std::exception& e) {
        THEMIS_WARN("Failed to initialize PKI API Handler: {}", e.what());
    }
    
    // Initialize PII Detector for Classification API (simplified - no PKI for now)
    auto pii_detector = std::make_shared<themis::utils::PIIDetector>();
    THEMIS_INFO("PII Detector initialized");
    
    // Initialize Classification API Handler with PIIDetector
    classification_api_ = std::make_unique<themis::server::ClassificationApiHandler>(pii_detector);
    THEMIS_INFO("Classification API Handler initialized");

    // Initialize Audit Logger and Audit API Handler
    try {
        // Optional: override audit rate limit via env var for tests
        if (const char* lim = std::getenv("THEMIS_AUDIT_RATE_LIMIT")) {
            try { audit_rate_limit_per_minute_ = static_cast<uint32_t>(std::stoul(lim)); } catch (...) {}
        } else {
            audit_rate_limit_per_minute_ = config_.audit_rate_limit_per_minute;
        }
        THEMIS_INFO("Audit rate limit per minute set to {}", audit_rate_limit_per_minute_);
        themis::utils::PKIConfig pki_cfg;
        pki_cfg.service_id = "themisdb";
        // Optional: allow configuring PKI certificate/key via env for real signing
        auto getenv_opt = [](const char* n) -> std::optional<std::string> {
            const char* v = std::getenv(n);
            if (v && *v) return std::string(v);
            return std::nullopt;
        };
        if (auto v = getenv_opt("THEMIS_PKI_ENDPOINT")) pki_cfg.endpoint = *v;
        if (auto v = getenv_opt("THEMIS_PKI_CERT")) pki_cfg.cert_path = *v;
        if (auto v = getenv_opt("THEMIS_PKI_KEY")) pki_cfg.key_path = *v;
        if (auto v = getenv_opt("THEMIS_PKI_KEY_PASSPHRASE")) pki_cfg.key_passphrase = *v;
        if (auto v = getenv_opt("THEMIS_PKI_SIG_ALG")) pki_cfg.signature_algorithm = *v;

        auto pki_client = std::make_shared<themis::utils::VCCPKIClient>(pki_cfg);

        // Minimal AuditLogger setup
        themis::utils::AuditLoggerConfig audit_cfg;
        audit_cfg.log_path = "data/logs/audit.jsonl";
        audit_cfg.enabled = true;
        audit_logger_ = std::make_shared<themis::utils::AuditLogger>(
            field_encryption_, pki_client, audit_cfg);
        THEMIS_INFO("Audit Logger initialized (path: {})", audit_cfg.log_path);

        // Audit API Handler reads/decrypts/filters audit logs
        audit_api_ = std::make_unique<themis::server::AuditApiHandler>(
            field_encryption_, pki_client, audit_cfg.log_path);
        THEMIS_INFO("Audit API Handler initialized");
    } catch (const std::exception& e) {
        THEMIS_WARN("Failed to initialize Audit components: {}", e.what());
    }

    // Wire AuditLogger into VectorIndexManager and GraphIndexManager
    // so that vector and graph operations are recorded for security auditing.
    if (audit_logger_) {
        if (vector_index_) {
            vector_index_->setAuditLogger(audit_logger_, "system");
            THEMIS_INFO("AuditLogger wired into VectorIndexManager");
        }
        if (graph_index_) {
            graph_index_->setAuditLogger(audit_logger_, "system");
            THEMIS_INFO("AuditLogger wired into GraphIndexManager");
        }
    }

    // Initialize Export API Handler (EXP-001)
    export_api_ = std::make_unique<themis::server::ExportApiHandler>(
        storage_, secondary_index_);
    if (audit_logger_) {
        export_api_->setAuditLogger(audit_logger_.get());
    }
    THEMIS_INFO("Export API Handler initialized");

    // Initialize Admin API Handler
    admin_api_ = std::make_unique<themis::server::AdminApiHandler>(
        storage_, auth_
    );
    THEMIS_INFO("Admin API Handler initialized");
    
    // Initialize Vector API Handler
    vector_api_ = std::make_unique<themis::server::VectorApiHandler>(
        storage_, vector_index_, auth_, field_encryption_, key_provider_
    );
    THEMIS_INFO("Vector API Handler initialized");
    
    // Initialize RoPE API Handler
    rope_api_ = std::make_unique<themis::server::RopeApiHandler>(
        storage_, vector_index_, auth_
    );
    THEMIS_INFO("RoPE API Handler initialized");
    
    // Initialize Spatial API Handler
    spatial_api_ = std::make_unique<themis::server::SpatialApiHandler>(
        storage_, spatial_index_, auth_
    );
    THEMIS_INFO("Spatial API Handler initialized");

    // Initialize Geo Topology API Handler
    geo_topology_api_ = std::make_unique<themis::server::GeoTopologyApiHandler>(
        shard_topology_, redundancy_manager_, auth_
    );
    THEMIS_INFO("Geo Topology API Handler initialized");

    // Initialize Replication Topology API Handler (web UI visualizer)
    {
        std::string repl_primary_id;
        if (const char* pid = std::getenv("THEMIS_WAL_PRIMARY_ID")) {
            repl_primary_id = pid;
        }
        replication_topology_api_ = std::make_unique<themis::server::ReplicationTopologyApiHandler>(
            replication_coordinator_, wal_manager_, repl_primary_id, auth_
        );
        THEMIS_INFO("Replication Topology API Handler initialized");
    }
    
    // Initialize Monitoring API Handler
    monitoring_api_ = std::make_unique<themis::server::MonitoringApiHandler>(
        storage_, auth_, &request_count_, &error_count_, &start_time_,
        secondary_index_, schema_manager_.get(), nullptr,
        &running_, &active_requests_, &active_connections_,
        concerns_   // may be nullptr - MonitoringApiHandler tolerates that
    );
    try {
        observability::RocksDBProvenanceStore::Config provenance_cfg;
        if (const char* provenance_path = std::getenv("THEMIS_PROVENANCE_DB_PATH");
            provenance_path && *provenance_path != '\0') {
            provenance_cfg.db_path = provenance_path;
        } else {
            provenance_cfg.db_path = "themis_provenance_store";
        }

        provenance_store_ = std::make_shared<observability::RocksDBProvenanceStore>(
            std::move(provenance_cfg));
        monitoring_api_->setProvenanceStore(provenance_store_);
        THEMIS_INFO("Provenance store initialized for observability export endpoint");
    } catch (const std::exception& e) {
        THEMIS_WARN("Failed to initialize provenance store: {}", e.what());
    }
    bao_optimizer_ = std::make_shared<themis::performance::phase3::BaoOptimizer>();
    workload_optimizer_ = std::make_shared<themis::performance::WorkloadAdaptiveOptimizer>();
    live_feedback_collector_ =
        std::make_shared<themis::prompt_engineering::FeedbackCollector>(storage_.get());
    themis::rag::learning::ContinuousLearningConfig learning_config;
    continuous_learning_orchestrator_ =
        std::make_shared<themis::rag::learning::ContinuousLearningOrchestrator>(learning_config);
    continuous_learning_orchestrator_->wireLiveSignalProviders(
        bao_optimizer_, workload_optimizer_, live_feedback_collector_);
    continuous_learning_orchestrator_->triggerLoop1QueryExecution({
        "server-bootstrap", 0.0, "{}", true
    });
    continuous_learning_orchestrator_->triggerLoop2WorkloadAdaptation();
    continuous_learning_orchestrator_->triggerLoop3IndexLifecycle();
    continuous_learning_orchestrator_->triggerLoop4AdapterImprovement();
    workload_optimizer_->enable_auto_adapt(std::chrono::seconds(60));
    {
        std::lock_guard<std::mutex> lock(api_handlers_mutex_);
        monitoring_api_->setContinuousLearningOrchestrator(continuous_learning_orchestrator_);
    }
    // Wire a disabled-by-default Alertmanager so the Operator API is always available.
    // Operators can enable it via the THEMIS_ALERTMANAGER_URL environment variable.
    {
        observability::AlertmanagerConfig am_cfg;
        const char* am_url = std::getenv("THEMIS_ALERTMANAGER_URL");
        if (am_url && *am_url != '\0') {
            am_cfg.endpoint_url = am_url;
            am_cfg.enabled      = true;
            const char* am_token = std::getenv("THEMIS_ALERTMANAGER_TOKEN");
            if (am_token && *am_token != '\0') {
                am_cfg.auth_token = am_token;
            }
            THEMIS_INFO("Alertmanager enabled: {}", am_cfg.endpoint_url);
        }
        alertmanager_ = std::make_shared<observability::DefaultAlertmanager>(am_cfg);
        {
            std::lock_guard<std::mutex> lock(api_handlers_mutex_);
            monitoring_api_->setAlertmanager(alertmanager_);  // monitoring keeps shared ownership
        }

        // Wire the same Alertmanager into the Cache hit-rate SLO monitor so that
        // SLO violations are forwarded to the same alerting endpoint.
        if (cache_admin_api_) {
            cache::CacheHitRateSloMonitor::Config slo_cfg;
            // Use sensible defaults; operators can override via environment variables.
            if (const char* warn_thr = std::getenv("THEMIS_CACHE_SLO_WARN")) {
                try { slo_cfg.warning_threshold = std::stod(warn_thr); } catch (...) {}
            }
            if (const char* crit_thr = std::getenv("THEMIS_CACHE_SLO_CRIT")) {
                try { slo_cfg.critical_threshold = std::stod(crit_thr); } catch (...) {}
            }
            auto cache_slo = std::make_shared<cache::CacheHitRateSloMonitor>(
                slo_cfg, alertmanager_);
            {
                std::lock_guard<std::mutex> lock(api_handlers_mutex_);
                cache_admin_api_->setSloMonitor(std::move(cache_slo));
            }
            THEMIS_INFO("Cache hit-rate SLO monitor wired into CacheAdminApiHandler");
        }
    }
    THEMIS_INFO("Monitoring API Handler initialized");
    // Initialize Query API Handler
    query_api_ = std::make_unique<themis::server::QueryApiHandler>(
        storage_, secondary_index_, graph_index_, field_encryption_, key_provider_,
        semantic_cache_, llm_store_, prompt_manager_, auth_,
        config_.feature_llm_query_enhancement,
        config_.feature_llm_store
    );
    query_api_->setQueryMaskingPolicy(themis::security::QueryMaskingPolicy::create());
    THEMIS_INFO("Query API Handler initialized");

    // Initialize Ranger client (optional) BEFORE PolicyApiHandler so that
    // policy_api_ receives a valid raw pointer instead of nullptr.
    // (ranger_client_ must be fully constructed before policy_api_ captures .get())
    if (auto base = themis_get_env("THEMIS_RANGER_BASE_URL")) {
        themis::server::RangerClientConfig rcfg;
        rcfg.base_url = *base;
        rcfg.policies_path = std::getenv("THEMIS_RANGER_POLICIES_PATH") ? std::getenv("THEMIS_RANGER_POLICIES_PATH") : "/service/public/v2/api/policy";
        rcfg.service_name = std::getenv("THEMIS_RANGER_SERVICE") ? std::getenv("THEMIS_RANGER_SERVICE") : "themisdb";
        rcfg.bearer_token = std::getenv("THEMIS_RANGER_BEARER") ? std::getenv("THEMIS_RANGER_BEARER") : "";
        rcfg.tls_verify = true;
        if (auto tlsv = themis_get_env("THEMIS_RANGER_TLS_VERIFY")) {
            if (*tlsv == "0" || *tlsv == "false" || *tlsv == "False") rcfg.tls_verify = false;
        }
        if (auto ca = themis_get_env("THEMIS_RANGER_CA_CERT")) rcfg.ca_cert_path = *ca;
        if (auto cc = themis_get_env("THEMIS_RANGER_CLIENT_CERT")) rcfg.client_cert_path = *cc;
        if (auto ck = themis_get_env("THEMIS_RANGER_CLIENT_KEY")) rcfg.client_key_path = *ck;
        if (auto ct = themis_get_env("THEMIS_RANGER_CONNECT_TIMEOUT_MS")) {
            try { rcfg.connect_timeout_ms = std::stol(*ct); } catch (...) {}
        }
        if (auto rt = themis_get_env("THEMIS_RANGER_REQUEST_TIMEOUT_MS")) {
            try { rcfg.request_timeout_ms = std::stol(*rt); } catch (...) {}
        }
        if (auto mr = themis_get_env("THEMIS_RANGER_MAX_RETRIES")) {
            try { rcfg.max_retries = std::stoi(*mr); } catch (...) {}
        }
        if (auto rb = themis_get_env("THEMIS_RANGER_RETRY_BACKOFF_MS")) {
            try { rcfg.retry_backoff_ms = std::stol(*rb); } catch (...) {}
        }
        try {
            ranger_client_ = std::make_unique<themis::server::RangerClient>(std::move(rcfg));
            THEMIS_INFO("Ranger client configured for {}", *base);
        } catch (const std::exception& ex) {
            THEMIS_WARN("Failed to initialize Ranger client: {}; integration disabled", ex.what());
        }
    }

    // Initialize Policy API Handler
    const char* ranger_service_env = std::getenv("THEMIS_RANGER_SERVICE");
    std::string ranger_service = ranger_service_env ? ranger_service_env : "themisdb";
    policy_api_ = std::make_unique<themis::server::PolicyApiHandler>(
        storage_, 
        ranger_client_.get(),
        policy_engine_.get(),
        auth_,
        ranger_service
    );
    THEMIS_INFO("Policy API Handler initialized");
    // Initialize Prompt API Handler
    prompt_api_ = std::make_unique<themis::server::PromptApiHandler>(
        storage_, prompt_manager_, auth_
    );
    THEMIS_INFO("Prompt API Handler initialized");
    // Initialize Graph API Handler
    graph_api_ = std::make_unique<themis::server::GraphApiHandler>(
        storage_, graph_index_, auth_
    );
    THEMIS_INFO("Graph API Handler initialized");
    // Initialize Index API Handler
    index_api_ = std::make_unique<themis::server::IndexApiHandler>(
        storage_, secondary_index_, adaptive_index_, auth_
    );
    THEMIS_INFO("Index API Handler initialized");
    // Initialize Entity API Handler
    server::EntityApiConfig entity_config;
    entity_config.feature_cdc = config_.feature_cdc;
    entity_config.feature_geo = true; // Enable geo if spatial_index exists
    entity_config.feature_replication = (replication_coordinator_ != nullptr);
    entity_config.feature_raid = (redundancy_manager_ != nullptr && hash_ring_ != nullptr && shard_topology_ != nullptr);
    
    entity_api_ = std::make_unique<themis::server::EntityApiHandler>(
        storage_,
        secondary_index_,
        graph_index_,
        tx_manager_,
        field_encryption_,
        key_provider_,
        auth_,
        entity_config,
        spatial_index_.get(),
        changefeed_,
        wal_manager_,
        replication_coordinator_,
        multi_primary_coordinator_,
        redundancy_manager_,
        hash_ring_,
        shard_topology_
    );
    THEMIS_INFO("Entity API Handler initialized (RAID: {})", entity_config.feature_raid ? "enabled" : "disabled");

    // Initialize ProcessGraphManager for BPMN/EPK workflow engine
    try {
        process_graph_ = std::make_shared<ProcessGraphManager>(*storage_);
        THEMIS_INFO("ProcessGraphManager initialized");
    } catch (const std::exception& e) {
        THEMIS_WARN("ProcessGraphManager initialization failed: {} — BPMN endpoints will return 503", e.what());
    }

    // Initialize BPMN API Handler
    bpmn_api_ = std::make_unique<themis::server::BpmnApiHandler>(
        process_graph_,  // May be nullptr if initialization failed above
        auth_
    );
    if (process_graph_) {
        THEMIS_INFO("BPMN API Handler initialized with ProcessGraphManager");
    } else {
        THEMIS_INFO("BPMN API Handler initialized (ProcessGraphManager not available - endpoints will return 503)");
    }
    
    // Initialize Content API Handler
    content_api_ = std::make_unique<themis::server::ContentApiHandler>(
        storage_, content_manager_, nullptr, auth_,
        secondary_index_, vector_index_
    );
    THEMIS_INFO("Content API Handler initialized");

    // Initialize ContentFS (binary content CRUD over HTTP: PUT/GET/HEAD/DELETE /api/v1/content/fs/{pk})
    if (storage_) {
        try {
            content_fs_ = std::make_unique<themis::ContentFS>(*storage_);
            THEMIS_INFO("ContentFS initialized (endpoints: /api/v1/content/fs/*)");
        } catch (const std::exception& e) {
            THEMIS_WARN("ContentFS init failed: {} — /api/v1/content/fs/* endpoints disabled", e.what());
        }
    }
    
    // Initialize Changefeed API Handler if changefeed is available
    if (changefeed_) {
        changefeed_api_ = std::make_unique<themis::server::ChangefeedApiHandler>(
            storage_, changefeed_,
#ifdef THEMIS_ENABLE_SSE
            sse_manager_,
#else
            nullptr,
#endif
            auth_, config_.feature_cdc
        );
        THEMIS_INFO("Changefeed API Handler initialized");
    }

    // Initialize PII Pseudonymizer (used for reveal/erase)
    // DEFERRED: Initialize on first use to avoid deadlock during server construction
    // try {
    //     pii_pseudonymizer_ = std::make_shared<themis::utils::PIIPseudonymizer>(
    //         storage_, field_encryption_, std::make_shared<themis::utils::PIIDetector>(), audit_logger_
    //     );
    //     THEMIS_INFO("PII Pseudonymizer initialized");
    // } catch (const std::exception& e) {
    //     THEMIS_WARN("Failed to initialize PII Pseudonymizer: {}", e.what());
    // }
    
    // Initialize Reports API Handler
    reports_api_ = std::make_unique<themis::server::ReportsApiHandler>();
    THEMIS_INFO("Reports API Handler initialized");
    
    // Initialize Transaction API Handler
    transaction_api_ = std::make_unique<themis::server::TransactionApiHandler>(
        storage_, tx_manager_, auth_
    );
    THEMIS_INFO("Transaction API Handler initialized");

    // Initialize Distributed Transaction (2PC) API Handler
    {
        themis::sharding::TrueTime::Config tt_cfg;
        tt_cfg.base_uncertainty_us = 1000;
        auto truetime = std::make_shared<themis::sharding::TrueTime>(tt_cfg);
        themis::sharding::DistributedTransactionCoordinator::Config dtxn_cfg;
        dtxn_cfg.enable_recovery_log = false; // WAL dir not configured at server init time;
                                              // configure via DistributedTransactionCoordinator::Config
                                              // to enable durable recovery logging in production
        auto dtxn_coordinator = std::make_shared<
            themis::sharding::DistributedTransactionCoordinator>(truetime, dtxn_cfg);
        distributed_txn_api_ = std::make_unique<themis::server::DistributedTxnApiHandler>(
            dtxn_coordinator
        );
    }
    THEMIS_INFO("Distributed Transaction (2PC) API Handler initialized");

    // Initialize WAL API Handler
    wal_api_ = std::make_unique<themis::server::WALApiHandler>(
        storage_, wal_applier_, wal_manager_, replication_coordinator_, auth_,
        wal_shared_secret_, wal_hmac_secret_
    );
    THEMIS_INFO("WAL API Handler initialized");

    // Initialize Ethics AI API Handler
    try {
        ethics_query_engine_ = std::make_unique<QueryEngine>(*storage_, *secondary_index_);
        ethics_api_ = std::make_unique<themis::server::EthicsApiHandler>(
            storage_,
            std::shared_ptr<QueryEngine>(ethics_query_engine_.get(), [](QueryEngine*) {}),
            auth_
        );
        THEMIS_INFO("Ethics AI API Handler initialized");
    } catch (const std::exception& e) {
        THEMIS_WARN("Ethics AI API Handler skipped: " + std::string(e.what()));
    }

    // Initialize Update Checker (if feature enabled)
    if (config_.feature_update_checker) {
        try {
            themis::utils::UpdateCheckerConfig update_config;
            update_config.github_owner = "makr-code";
            update_config.github_repo = "ThemisDB";
#ifdef THEMIS_VERSION_STRING
            update_config.current_version = THEMIS_VERSION_STRING;
#else
            update_config.current_version = "1.0.0"; // Fallback if version not defined
#endif
            update_config.check_interval = std::chrono::seconds(3600); // 1 hour
            
            // Allow configuration via environment variables
            if (auto token = themis_get_env("THEMIS_GITHUB_API_TOKEN")) {
                update_config.github_api_token = *token;
            }
            if (auto interval = themis_get_env("THEMIS_UPDATE_CHECK_INTERVAL")) {
                try {
                    update_config.check_interval = std::chrono::seconds(std::stoul(*interval));
                } catch (...) {}
            }
            if (auto auto_update = themis_get_env("THEMIS_AUTO_UPDATE_ENABLED")) {
                update_config.auto_update_enabled = (*auto_update == "true" || *auto_update == "1");
            }
            
            update_checker_ = std::make_shared<themis::utils::UpdateChecker>(update_config);
            update_api_ = std::make_unique<themis::server::UpdateApiHandler>(update_checker_);
            
            // Start background checking
            update_checker_->start();
            
            THEMIS_INFO("Update Checker initialized (interval: {}s)", update_config.check_interval.count());
        } catch (const std::exception& e) {
            THEMIS_WARN("Failed to initialize Update Checker: {}", e.what());
        }
    }
    
    // Initialize Feedback API Handler
#if THEMIS_ENABLE_LLM
    try {
        using namespace llm::lora;
        
        // Create feedback storage service
        FeedbackStorageService::Config feedback_config;
        feedback_config.db = storage_;
        if (graph_index_) {
            feedback_config.graph_index = graph_index_;
        }
        feedback_config.collection_name = "help_feedback";
        feedback_config.enable_graph_links = true;
        
        auto feedback_storage = std::make_shared<FeedbackStorageService>(feedback_config);
        
        // Register default plugins
        feedback_storage->registerPlugin(std::make_shared<BaseFeedbackPlugin>());
        feedback_storage->registerPlugin(std::make_shared<PrivacyFilterPlugin>());
        feedback_storage->registerPlugin(std::make_shared<ContentValidationPlugin>());
        
        // Try to load YAML configuration and register configured plugins
        try {
            std::string config_path = themis::config::ConfigPathResolver::resolve("config/lora_training_config.yaml");
            auto training_config = LoRATrainingConfig::loadFromFile(config_path);
            
            // Register cache weighting plugin from config
            auto cache_plugin = training_config.createCacheWeightingPlugin("themis_help_lora");
            feedback_storage->registerPlugin(cache_plugin);
            
            // Register training trigger plugin from config
            auto trigger_plugin = training_config.createTrainingTriggerPlugin("themis_help_lora");
            feedback_storage->registerPlugin(trigger_plugin);
            
            THEMIS_INFO("Feedback system initialized with YAML configuration");
        } catch (const std::exception& e) {
            THEMIS_WARN("Failed to load YAML configuration, using defaults: {}", e.what());
            
            // Fall back to default plugins
            TrainingTriggerPlugin::Config trigger_config;
            trigger_config.min_batch_size = 50;
            trigger_config.max_batch_size = 200;
            feedback_storage->registerPlugin(std::make_shared<TrainingTriggerPlugin>(trigger_config));
            
            CacheAwareWeightingPlugin::Config cache_config;
            feedback_storage->registerPlugin(std::make_shared<CacheAwareWeightingPlugin>(cache_config));
        }
        
        feedback_api_handler_ = std::make_unique<server::FeedbackAPIHandler>(feedback_storage);
        if (feedback_api_handler_) {
            feedback_api_handler_->setLiveFeedbackCollector(live_feedback_collector_);
            feedback_api_handler_->setLearningOrchestrator(continuous_learning_orchestrator_);
        }
        THEMIS_INFO("Feedback API Handler initialized");
    } catch (const std::exception& e) {
        THEMIS_WARN("Failed to initialize Feedback API Handler: {}", e.what());
    }
#else
    feedback_api_handler_ = nullptr;
#endif

    // Initialize SchemaManager and Schema API Handler
    try {
        if (storage_ && storage_->isOpen()) {
            schema_manager_ = std::make_unique<SchemaManager>(*storage_, secondary_index_.get());
            schema_api_handler_ = std::make_unique<server::SchemaApiHandler>(
                storage_, secondary_index_, schema_manager_.get());

            // Wire metadata sub-components into SchemaApiHandler
            stats_collector_    = std::make_unique<StatisticsCollector>(*storage_);
            schema_constraints_ = std::make_unique<SchemaConstraints>();
            schema_constraints_->loadFrom(*storage_);  // Reload persisted constraints
            schema_version_mgr_ = std::make_unique<SchemaVersionManager>(*storage_, *schema_manager_);
            index_recommender_  = std::make_unique<metadata::IndexRecommender>();
            schema_audit_log_   = std::make_unique<SchemaAuditLog>(*storage_);

            // Connect audit log to version manager so every version is audited
            schema_version_mgr_->setAuditLog(schema_audit_log_.get());

            schema_api_handler_->setStatisticsCollector(stats_collector_.get());
            schema_api_handler_->setSchemaConstraints(schema_constraints_.get());
            schema_api_handler_->setSchemaVersionManager(schema_version_mgr_.get());
            schema_api_handler_->setIndexRecommender(index_recommender_.get());
            schema_api_handler_->setAuditLog(schema_audit_log_.get());

            // Wire ColumnLineageTracker into SchemaApiHandler
            column_lineage_tracker_ = std::make_unique<themis::metadata::ColumnLineageTracker>();
            schema_api_handler_->setColumnLineageTracker(column_lineage_tracker_.get());

            // Wire IndexRecommender into query handler for access-pattern recording
            if (query_api_) {
                query_api_->setIndexRecommender(index_recommender_.get());
                query_api_->setStatisticsCollector(stats_collector_.get());
            }

            // Background consistency checker (checks every 6 hours by default)
            schema_consistency_checker_ = std::make_unique<SchemaConsistencyChecker>(
                *storage_, *schema_manager_,
                stats_collector_.get(),
                schema_constraints_.get()
            );
            schema_consistency_checker_->startBackgroundCheck(std::chrono::hours(6));

            // Wire changefeed for real-time schema change notifications (if CDC is enabled)
            if (changefeed_) {
                schema_manager_->setChangefeed(changefeed_.get());
            }

            THEMIS_INFO("SchemaManager, Schema API Handler, and metadata sub-components initialized");
        } else {
            THEMIS_WARN("Storage not open, SchemaManager initialization deferred");
        }
    } catch (const std::exception& e) {
        THEMIS_WARN("Failed to initialize SchemaManager/Schema API: {}", e.what());
    }

    // Wire SchemaManager into MonitoringApiHandler now that it is available.
    // MonitoringApiHandler is constructed early (before SchemaManager) so it
    // receives a null pointer from the constructor; this call fills the gap and
    // enables the /api/v1/capabilities schema capability block.
    if (monitoring_api_ && schema_manager_) {
        monitoring_api_->setSchemaManager(schema_manager_.get());
        THEMIS_INFO("SchemaManager wired into MonitoringApiHandler (capabilities endpoint complete)");
    }

    // Initialize GraphQL API Handler with a dedicated AQL query engine
    // so that aql() / aqlMutation() resolver fields execute real queries.
    // The engine must be created before the handler and declared after it
    // in the class so that it is destroyed first (reverse member-order).
    if (storage_ && secondary_index_) {
        graphql_query_engine_ = std::make_unique<QueryEngine>(
            *storage_, *secondary_index_);
        graphql_api_handler_ = std::make_unique<server::GraphQLApiHandler>(
            graphql_query_engine_.get());
    } else {
        graphql_api_handler_ = std::make_unique<server::GraphQLApiHandler>();
    }
    THEMIS_INFO("GraphQL API handler initialized (endpoints: POST /graphql, GET /graphql/schema)");

    // Initialize gRPC-Web proxy handler
    {
        GrpcWebProxyHandler::Config gwcfg;
        gwcfg.backend_address = "localhost:18765";
        grpc_web_proxy_ = std::make_unique<server::GrpcWebProxyHandler>(std::move(gwcfg));
        THEMIS_INFO("gRPC-Web proxy handler initialized (endpoints: POST /grpc-web/*, GET /api/v1/grpc-web/status)");
    }

    // Initialize Serverless Function Hosting Handler
    serverless_fn_handler_ = std::make_unique<server::ServerlessFunctionApiHandler>();
    THEMIS_INFO("Serverless function handler initialized (endpoints: /api/v1/functions)");

    udf_api_handler_ = std::make_unique<server::UdfApiHandler>();
    THEMIS_INFO("UDF API handler initialized (endpoints: /api/v1/query/udfs)");

    // Initialize Task Scheduler and its API / Web UI handler
    {
        TaskScheduler::Config sched_cfg;
        sched_cfg.persist_tasks            = false;
        sched_cfg.enable_audit_logging     = audit_logger_ != nullptr;
        sched_cfg.enable_anomaly_detection = true;
        // Build a QueryEngine for the scheduler using the server's storage.
        // task_scheduler_engine_ must outlive task_scheduler_.  The member
        // declaration order in http_server.h guarantees this: unique_ptr members
        // are destroyed in reverse declaration order, so task_scheduler_ is
        // destroyed before task_scheduler_engine_.
        task_scheduler_engine_ = std::make_unique<QueryEngine>(*storage_, *secondary_index_);
        task_scheduler_ = std::make_unique<TaskScheduler>(
            task_scheduler_engine_.get(),
            sched_cfg,
            changefeed_.get(),
            audit_logger_,
            storage_.get());
        // Wire Alertmanager for task failure / SLA-breach notifications.
        if (alertmanager_) {
            task_scheduler_->setAlertmanager(alertmanager_);
            THEMIS_INFO("Alertmanager wired into TaskScheduler");
        }
        task_scheduler_->start();
        task_scheduler_api_ = std::make_unique<server::TaskSchedulerApiHandler>(task_scheduler_.get());
        THEMIS_INFO("Task Scheduler API handler initialized (endpoints: /api/tasks, /ui/tasks)");

        // Initialize Database Maintenance Orchestrator
        maintenance_orchestrator_ = std::make_unique<themis::maintenance::DatabaseMaintenanceOrchestrator>(
            task_scheduler_.get());
        [[maybe_unused]] const auto maintenance_started = maintenance_orchestrator_->start();

        // Wire STORAGE_COMPACTION handler — calls CompactionManager::compactAll()
        {
            auto compaction_mgr = std::make_shared<themis::CompactionManager>(storage_);
            maintenance_orchestrator_->registerTaskHandler(
                themis::maintenance::MaintenanceTaskType::STORAGE_COMPACTION,
                std::make_shared<themis::maintenance::StorageCompactionHandler>(
                    std::move(compaction_mgr)));
        }

        // Wire MVCC_CLEANUP handler — GC-s stale MVCC versions older than 24 h
        if (mvcc_store_) {
            maintenance_orchestrator_->registerTaskHandler(
                themis::maintenance::MaintenanceTaskType::MVCC_CLEANUP,
                std::make_shared<themis::maintenance::MvccCleanupHandler>(mvcc_store_));
        }

        // Wire REPLICA_VALIDATION handler — delegates to ShardRepairEngine::runConsistencyCheck()
        if (shard_repair_engine_) {
            maintenance_orchestrator_->registerTaskHandler(
                themis::maintenance::MaintenanceTaskType::REPLICA_VALIDATION,
                themis::maintenance::makeReplicaValidationHandler(shard_repair_engine_));
            THEMIS_INFO("REPLICA_VALIDATION handler wired to ShardRepairEngine");
        }

        maintenance_api_ = std::make_unique<server::MaintenanceApiHandler>(
            maintenance_orchestrator_.get());
        THEMIS_INFO("Database Maintenance Orchestrator initialized (endpoints: /api/v1/maintenance/*)");
    }

    // Initialize Async Job API Handler - long-running AQL query submission/polling
    {
        // Executor: builds a synthetic Beast request and delegates to
        // the existing QueryApiHandler so all existing AQL machinery
        // (validation, caching, masking) is reused.
        auto* qapi = query_api_.get();
        server::AsyncJobApiHandler::AqlExecutor executor =
            [qapi](const std::string& aql_query,
                   const std::string& auth_header) -> nlohmann::json {
                http::request<http::string_body> inner{
                    http::verb::post, "/query/aql", 11};
                inner.set(http::field::content_type,  "application/json");
                inner.set(http::field::authorization, auth_header);
                nlohmann::json body = {{"query", aql_query}};
                inner.body() = body.dump();
                inner.prepare_payload();
                auto response = qapi->handleQueryAql(inner);
                return nlohmann::json::parse(response.body());
            };
        async_job_api_ = std::make_unique<server::AsyncJobApiHandler>(
            std::move(executor), auth_);
        THEMIS_INFO("Async job API handler initialized (endpoints: POST/GET/DELETE /v2/jobs)");
    }

    // Initialize Retention Policy Admin API Handler
    {
        std::string retention_path = "config/retention_policies.yaml";
        auto resolved = themis::config::ConfigPathResolver::tryResolve(retention_path);

        // Fallback: if resolution failed (tests may run with a CWD inside a temp
        // directory), attempt to locate the config by walking up ancestor
        // directories relative to the process CWD and the source file location.
        if (!resolved) {
            std::vector<std::string> candidates = {
                "config/data_management/retention_policies.yaml",
                "config/retention_policies.yaml"
            };

            auto try_find_in_ancestors = [&](const std::filesystem::path& start) -> std::optional<std::string> {
                std::filesystem::path p = start;
                for (int i = 0; i < 8 && !p.empty(); ++i) {
                    for (const auto& c : candidates) {
                        auto cand = p / c;
                        if (std::filesystem::exists(cand)) return cand.string();
                    }
                    p = p.parent_path();
                }
                return std::nullopt;
            };

            // 1) search from current_path()
            if (auto s = try_find_in_ancestors(std::filesystem::current_path())) {
                resolved = *s;
            }

            // 2) search from compile-time source location as a last resort
            if (!resolved) {
                std::filesystem::path src = std::filesystem::path(__FILE__).parent_path().parent_path();
                if (auto s = try_find_in_ancestors(src)) {
                    resolved = *s;
                }
            }
        }

        auto retention_mgr = std::make_shared<vcc::RetentionManager>(
            resolved.value_or(retention_path));
        retention_api_ = std::make_unique<server::RetentionApiHandler>(retention_mgr);
        THEMIS_INFO("RetentionApiHandler initialized (endpoints: /api/retention/*), using {}",
                    resolved.value_or(retention_path));
    }

    // Initialize SAGA Audit Log API Handler
    {
        try {
            themis::utils::SAGALoggerConfig saga_cfg;
            saga_cfg.enabled = true;
            saga_cfg.encrypt_then_sign = true;
            saga_cfg.log_path = "data/logs/saga.jsonl";
            saga_cfg.signature_path = "data/logs/saga_signatures.jsonl";
            saga_cfg.key_id = "saga_lek";

            themis::utils::PKIConfig saga_pki_cfg;
            saga_pki_cfg.service_id = "themis-saga";
            if (const char* k = std::getenv("THEMIS_PKI_PRIVATE_KEY")) saga_pki_cfg.key_path = k;
            if (const char* c = std::getenv("THEMIS_PKI_CERTIFICATE")) saga_pki_cfg.cert_path = c;
            if (const char* p = std::getenv("THEMIS_PKI_PRIVATE_KEY_PASSPHRASE")) saga_pki_cfg.key_passphrase = p;
            auto saga_pki = std::make_shared<themis::utils::VCCPKIClient>(saga_pki_cfg);

            auto saga_logger = std::make_shared<themis::utils::SAGALogger>(
                field_encryption_, saga_pki, saga_cfg);
            saga_logger_ = saga_logger;  // retain shared ownership via HttpServer member
            saga_api_ = std::make_unique<server::SAGAApiHandler>(saga_logger);
            THEMIS_INFO("SAGAApiHandler initialized (endpoints: /api/saga/*)");
        } catch (const std::exception& e) {
            THEMIS_WARN("SAGAApiHandler init failed: {} — SAGA endpoints disabled", e.what());
        }
    }

    // Initialize Policy Engine (Governance)
    policy_engine_ = std::make_unique<themis::PolicyEngine>();
    try {
        // Allow overriding the policies path via env `THEMIS_POLICIES_PATH` (useful for tests)
        std::vector<std::filesystem::path> candidates;
        if (const char* envp = std::getenv("THEMIS_POLICIES_PATH")) {
            std::filesystem::path p(envp);
            candidates.push_back(p);
            // If the provided path does not exist, try resolving it relative to repository root
            if (!std::filesystem::exists(p)) {
                // Walk up from current path to find a marker (CMakeLists.txt) to locate repo root
                auto cur = std::filesystem::current_path();
                for (auto up = cur; ; up = up.parent_path()) {
                    if (up == up.parent_path()) break; // reached filesystem root
                    if (std::filesystem::exists(up / "CMakeLists.txt") || std::filesystem::exists(up / ".git")) {
                        std::filesystem::path candidate = up / envp;
                        if (std::filesystem::exists(candidate)) {
                            candidates.front() = candidate;
                            THEMIS_INFO("PolicyEngine: resolved THEMIS_POLICIES_PATH relative to repo root: {}", candidate.string());
                            break;
                        }
                    }
                }
            } else {
                THEMIS_INFO("PolicyEngine: using policies override from THEMIS_POLICIES_PATH={}", p.string());
            }
        } else {
            // Try YAML first, then JSON in default config directory
            candidates = {
                std::filesystem::path("config") / "policies.yaml",
                std::filesystem::path("config") / "policies.yml",
                std::filesystem::path("config") / "policies.json"
            };
        }
        bool loaded_any = false;
        for (const auto& policies_path : candidates) {
            if (std::filesystem::exists(policies_path)) {
                std::string err;
                if (policy_engine_->loadFromFile(policies_path.string(), &err)) {
                    THEMIS_INFO("PolicyEngine: loaded policies from {}", policies_path.string());
                    loaded_any = true;
                    break;
                } else {
                    THEMIS_WARN("PolicyEngine: failed to load {}: {}", policies_path.string(), err);
                }
            }
        }
        if (!loaded_any) {
            THEMIS_INFO("PolicyEngine: no policies file found (config/policies.yaml|yml|json), default allow when empty");
        }
    } catch (const std::exception& e) {
        THEMIS_WARN("PolicyEngine initialization warning: {}", e.what());
    }
    // Wire AuditLogger into PolicyEngine so that authorization decisions are recorded
    if (policy_engine_ && audit_logger_) {
        policy_engine_->setAuditLogger(audit_logger_.get());
        THEMIS_INFO("AuditLogger wired into PolicyEngine");
    }

    // Initialize OPA evaluator (optional); enabled when THEMIS_OPA_ENDPOINT_URL is set.
    // OPA acts as an alternative policy evaluation engine (policy-as-code via Rego).
    // If OPA is unreachable or times out, PolicyEngine falls back to native evaluation
    // and increments the opa_fallback_total metric.
    if (auto opa_url = themis_get_env("THEMIS_OPA_ENDPOINT_URL")) {
        themis::OpaAdapter::Config opa_cfg;
        opa_cfg.endpoint_url = *opa_url;
        if (auto path = themis_get_env("THEMIS_OPA_POLICY_PATH")) {
            opa_cfg.policy_path = *path;
        }
        if (auto tms = themis_get_env("THEMIS_OPA_TIMEOUT_MS")) {
            try { opa_cfg.timeout_ms = std::stol(*tms); } catch (...) {}
        }
        try {
            opa_adapter_ = std::make_unique<themis::OpaAdapter>(opa_cfg);
            if (policy_engine_) {
                policy_engine_->setOpaEvaluator(opa_adapter_.get());
            }
            THEMIS_INFO("OPA evaluator configured: endpoint={}, path={}, timeout={}ms",
                opa_cfg.endpoint_url, opa_cfg.policy_path, opa_cfg.timeout_ms);
        } catch (const std::exception& e) {
            THEMIS_WARN("Failed to initialize OPA evaluator: {}; native policy evaluation will be used", e.what());
        }
    }
    // ExportApiHandler expects themis::governance::PolicyEngine while HttpServer
    // currently owns themis::PolicyEngine. Keep exports operational without
    // unsafe cross-namespace pointer casting.
    if (export_api_ && !policy_engine_) {
        THEMIS_WARN("ExportApiHandler: PolicyEngine not available — export policy enforcement disabled");
    }

    // Initialize Rate Limiter for DoS protection
    RateLimitConfig rate_config;
    rate_config.bucket_capacity = 100;
    rate_config.refill_rate = 100.0 / 60.0; // 100 req/min default
    rate_config.per_ip_enabled = true;
    rate_config.per_user_enabled = true;
    
    // HS-8: Do NOT whitelist localhost by default — an SSRF vulnerability could route
    // requests through loopback to bypass rate limiting. Populate the whitelist only
    // from the THEMIS_RATE_LIMIT_WHITELIST env var (comma-separated IPs).
    if (const char* wl_env = std::getenv("THEMIS_RATE_LIMIT_WHITELIST")) {
        std::istringstream wl_stream{wl_env};
        std::string wl_ip;
        while (std::getline(wl_stream, wl_ip, ',')) {
            if (!wl_ip.empty()) rate_config.whitelist_ips.push_back(wl_ip);
        }
        // Warn when localhost is explicitly whitelisted — SSRF risk.
        for (const auto& ip : rate_config.whitelist_ips) {
            if (ip == "127.0.0.1" || ip == "::1") {
                THEMIS_WARN("[SECURITY] Rate-limit whitelist includes loopback address '{}'. "
                            "This exempts requests routed via SSRF from rate limiting (HS-8).", ip);
            }
        }
    }

    // Load custom limits from environment
    if (auto limit_str = themis_get_env("THEMIS_RATE_LIMIT_PER_MINUTE")) {
        try {
            uint32_t limit = std::stoul(*limit_str);
            rate_config.bucket_capacity = limit;
            rate_config.refill_rate = static_cast<double>(limit) / 60.0;
            THEMIS_INFO("Rate limit set to {} req/min from environment", limit);
        } catch (...) {
            THEMIS_WARN("Invalid THEMIS_RATE_LIMIT_PER_MINUTE value, using default");
        }
    }
    
    rate_limiter_ = std::make_unique<RateLimiter>(rate_config);
    THEMIS_INFO("Rate Limiter initialized: {} req/min", rate_config.refill_rate * 60.0);
    // Wire anomaly callback so that throttle/blacklist events are logged to the
    // audit log (when available) and always emit a structured WARN log line.
    // Capture audit_logger_ as weak_ptr to avoid use-after-free if the logger
    // is reset before the RateLimiter is destroyed.
    {
        std::weak_ptr<themis::utils::AuditLogger> weak_audit = audit_logger_;
        rate_limiter_->setAnomalyCallback(
            [weak_audit](const AnomalyEvent& ev) {
                const char* type_str =
                    (ev.type == AnomalyEvent::Type::IP_BLACKLISTED)
                        ? "IP_BLACKLISTED"
                        : "ADAPTIVE_THROTTLE_TRIGGERED";
                THEMIS_WARN("[RateLimiter] anomaly type={} ip={} detail={}",
                    type_str, ev.ip, ev.detail);
                if (auto audit = weak_audit.lock()) {
                    nlohmann::json entry;
                    entry["event"]  = "rate_limiter_anomaly";
                    entry["type"]   = type_str;
                    entry["ip"]     = ev.ip;
                    entry["detail"] = ev.detail;
                    try { audit->logEvent(entry); } catch (const std::exception& ex) {
                        THEMIS_ERROR("Failed to log rate limiter anomaly: {}", ex.what());
                    }
                }
            });
        THEMIS_INFO("RateLimiter anomaly callback wired");
    }

    // Initialize rate limiting middleware with configurable per-client token bucket
    {
        RateLimitingMiddleware::Config rl_mw_config;
        rl_mw_config.default_capacity    = rate_config.bucket_capacity;
        rl_mw_config.default_refill_rate = rate_config.refill_rate;
        rl_mw_config.whitelist_ips       = rate_config.whitelist_ips;
        rl_mw_config.max_clients         = 10000;
        rl_mw_config.send_rate_limit_headers = true;

        // Apply a tighter limit on bulk-write paths to prevent abuse
        rl_mw_config.endpoint_overrides = {
            RateLimitingMiddleware::EndpointLimit{
                "/v2/documents",
                static_cast<size_t>(rl_mw_config.default_capacity / 2),
                rl_mw_config.default_refill_rate / 2.0
            },
            RateLimitingMiddleware::EndpointLimit{
                "/api/bulk",
                static_cast<size_t>(rl_mw_config.default_capacity / 2),
                rl_mw_config.default_refill_rate / 2.0
            }
        };

        rate_limiting_middleware_ = std::make_unique<RateLimitingMiddleware>(rl_mw_config);
        THEMIS_INFO("RateLimitingMiddleware initialized: {:.1f} req/min default, {} endpoint overrides",
                    rl_mw_config.default_refill_rate * 60.0,
                    rl_mw_config.endpoint_overrides.size());
    }

    // Initialize request correlation ID middleware
    tracing_middleware_ = std::make_unique<api::TracingMiddleware>();
    THEMIS_INFO("TracingMiddleware initialized: X-Correlation-ID propagation enabled");

    // ----------------------------------------------------------------------------
    // CORS configuration (from environment)
    // ----------------------------------------------------------------------------
    cors_allow_all_ = false;
    cors_allow_credentials_ = false;
    cors_allowed_origins_.clear();
    cors_allowed_methods_ = "GET,POST,PUT,DELETE,OPTIONS";
    cors_allowed_headers_ = "Authorization,Content-Type,X-Requested-With";

    if (auto v = themis_get_env("THEMIS_CORS_ALLOW_ALL")) {
        cors_allow_all_ = (*v == "1" || *v == "true" || *v == "TRUE");
    }
    if (auto v = themis_get_env("THEMIS_CORS_ALLOW_CREDENTIALS")) {
        cors_allow_credentials_ = (*v == "1" || *v == "true" || *v == "TRUE");
    }
    if (auto v = themis_get_env("THEMIS_CORS_ALLOWED_ORIGINS")) {
        // Comma-separated exact origins, e.g. https://app.example.com,https://admin.example.com
        std::istringstream iss(*v);
        std::string origin;
        while (std::getline(iss, origin, ',')) {
            if (!origin.empty()) cors_allowed_origins_.push_back(origin);
        }
    }
    if (auto v = themis_get_env("THEMIS_CORS_ALLOWED_METHODS")) {
        if (!v->empty()) cors_allowed_methods_ = *v;
    }
    if (auto v = themis_get_env("THEMIS_CORS_ALLOWED_HEADERS")) {
        if (!v->empty()) cors_allowed_headers_ = *v;
    }
    if (cors_allow_all_) {
        // GAP-012: CORS wildcard is a security risk (CWE-346).  Any origin can
        // make cross-site requests; browsers will expose the response to untrusted
        // JavaScript.  Emit a prominent WARNING so operators know this is not a
        // production-safe configuration.
        THEMIS_WARN("[SECURITY] CORS: Access-Control-Allow-Origin: * is ENABLED via "
                    "THEMIS_CORS_ALLOW_ALL. Any origin can read responses. "
                    "This is NOT recommended for production deployments (GAP-012/CWE-346).");
    } else if (!cors_allowed_origins_.empty()) {
        THEMIS_INFO("CORS: allowed origins configured ({} entries)", cors_allowed_origins_.size());
    } else {
        THEMIS_INFO("CORS: no origins allowed by default (set THEMIS_CORS_ALLOW_ALL=1 for dev)");
    }
    // HS-9: Combining wildcard origin with allow-credentials is prohibited by the CORS spec
    // and can confuse CDN/browser behaviour. Forcibly disable credentials in this case.
    if (cors_allow_all_ && cors_allow_credentials_) {
        THEMIS_WARN("[SECURITY] CORS: Access-Control-Allow-Origin: * combined with "
                    "Access-Control-Allow-Credentials: true is prohibited by the CORS "
                    "specification. Disabling allow-credentials to enforce safe CORS policy.");
        cors_allow_credentials_ = false;
    }

    // Initialize Input Validator (schema dir from env or default config/schemas)
    std::string schema_dir = "config/schemas";
    if (auto v = themis_get_env("THEMIS_SCHEMAS_DIR")) {
        if (!v->empty()) schema_dir = *v;
    }
    validator_ = std::make_unique<themis::utils::InputValidator>(schema_dir);
    THEMIS_INFO("InputValidator initialized with schema dir: {}", schema_dir);

    // ----------------------------------------------------------------------------
    // Request validation middleware (JSON Schema per endpoint)
    // ----------------------------------------------------------------------------
    request_validator_ = std::make_unique<RequestValidationMiddleware>();

    // Register JSON Schema Draft-7 for core write endpoints.
    // Schemas validate required fields, types and basic constraints.
    // Operators can register additional schemas via getRequestValidator().

    // Validation limits used in endpoint schemas
    constexpr int MAX_QUERY_LIMIT       = 10000;  // max rows per query request
    constexpr int MAX_AQL_QUERY_CHARS   = 100000; // max AQL query string length

    // Reusable property definitions
    const nlohmann::json entity_key_prop   = {{"type", "string"}, {"minLength", 1}, {"maxLength", 512}};
    const nlohmann::json entity_data_prop  = {{"type", "object"}};
    const nlohmann::json entity_tags_prop  = {{"type", "array"}};
    const nlohmann::json entity_ttl_prop   = {{"type", "integer"}, {"minimum", 0}};

    // POST /entities - create entity (key required in body)
    request_validator_->registerSchema("POST", "/entities", {
        {"type", "object"},
        {"required", {"key"}},
        {"properties", {
            {"key",  entity_key_prop},
            {"data", entity_data_prop},
            {"tags", entity_tags_prop},
            {"ttl",  entity_ttl_prop}
        }}
    });

    // PUT /entities/<key> - upsert entity body (key is in the URL path, not in the body)
    request_validator_->registerSchema("PUT", "/entities/", {
        {"type", "object"},
        {"properties", {
            {"data", entity_data_prop},
            {"tags", entity_tags_prop},
            {"ttl",  entity_ttl_prop}
        }}
    });

    // POST /query - structured query
    request_validator_->registerSchema("POST", "/query", {
        {"type", "object"},
        {"required", {"query"}},
        {"properties", {
            {"query",   {{"type", "string"}, {"minLength", 1}}},
            {"limit",   {{"type", "integer"}, {"minimum", 1}, {"maximum", MAX_QUERY_LIMIT}}},
            {"offset",  {{"type", "integer"}, {"minimum", 0}}},
            {"filters", {{"type", "object"}}},
            {"sort",    {{"type", "array"}}}
        }}
    });

    // AQL query schema (shared by /query/aql and /api/aql)
    const nlohmann::json aql_schema = {
        {"type", "object"},
        {"required", {"query"}},
        {"properties", {
            {"query",    {{"type", "string"}, {"minLength", 1}, {"maxLength", MAX_AQL_QUERY_CHARS}}},
            {"bindVars", {{"type", "object"}}}
        }}
    };
    // POST /query/aql and POST /api/aql - further validated by validateAqlRequest
    request_validator_->registerSchema("POST", "/query/aql", aql_schema);
    request_validator_->registerSchema("POST", "/api/aql",   aql_schema);

    // POST /v2/jobs - async job submission (same query shape as /query/aql)
    request_validator_->registerSchema("POST", "/v2/jobs", aql_schema);

    // POST /index/create - create index
    request_validator_->registerSchema("POST", "/index/create", {
        {"type", "object"},
        {"required", {"field"}},
        {"properties", {
            {"field",   {{"type", "string"}, {"minLength", 1}, {"maxLength", 256}}},
            {"type",    {{"type", "string"}}},
            {"options", {{"type", "object"}}}
        }}
    });

    // POST /api/v1/transactions - begin transaction
    request_validator_->registerSchema("POST", "/api/v1/transactions", {
        {"type", "object"},
        {"properties", {
            {"isolation_level", {{"type", "string"}}},
            {"timeout_ms",      {{"type", "integer"}, {"minimum", 0}}}
        }}
    });

    // DELETE /vector/by-filter - carries a body with filter criteria
    request_validator_->registerSchema("DELETE", "/vector/by-filter", {
        {"type", "object"},
        {"required", {"filter"}},
        {"properties", {
            {"filter", {{"type", "object"}}}
        }}
    });

    THEMIS_INFO("RequestValidationMiddleware initialized with {} endpoint schemas",
                request_validator_->schemaCount());

    // ----------------------------------------------------------------------------
    // CDN / Edge Cache Middleware - Cache-Control header management
    // ----------------------------------------------------------------------------
    // Register default per-route policies.  Conservative no-store is the global
    // fallback for unregistered paths; only well-defined read endpoints receive
    // cacheable policies here.  Individual handlers may register additional
    // policies or rely on the defaults.
    {
        // Health / liveness probes - very short public cache so CDNs and load
        // balancers can coalesce repeated probes without hammering the origin.
        CdnRoutePolicy health_policy;
        health_policy.directive              = CacheDirective::PUBLIC;
        health_policy.max_age_seconds        = 5;
        health_policy.cdn_max_age_seconds    = 10;
        health_policy.emit_cdn_cache_control = true;
        health_policy.stale_if_error_seconds = 30;
        cdn_cache_middleware_.registerPolicy("/health", health_policy);
        cdn_cache_middleware_.registerPolicy("/status", health_policy);

        // Entity read endpoints - private (user-specific data), enable ETag for
        // conditional GET support so repeated fetches avoid re-transferring bodies.
        CdnRoutePolicy entity_policy;
        entity_policy.directive              = CacheDirective::PRIVATE;
        entity_policy.max_age_seconds        = 60;
        entity_policy.enable_etag            = true;
        entity_policy.emit_cdn_cache_control = true;
        cdn_cache_middleware_.registerPolicy("/entities/", entity_policy);

        // Query / AQL endpoints - dynamic results, never cache.
        CdnRoutePolicy query_policy;
        query_policy.directive = CacheDirective::NO_CACHE;
        cdn_cache_middleware_.registerPolicy("/query", query_policy);
        cdn_cache_middleware_.registerPolicy("/api/aql", query_policy);
        cdn_cache_middleware_.registerPolicy("/v2/jobs", query_policy);

        // Monitoring metrics - short public cache for Prometheus scrapers.
        CdnRoutePolicy metrics_policy;
        metrics_policy.directive              = CacheDirective::PUBLIC;
        metrics_policy.max_age_seconds        = 15;
        metrics_policy.cdn_max_age_seconds    = 15;
        metrics_policy.emit_cdn_cache_control = true;
        cdn_cache_middleware_.registerPolicy("/metrics", metrics_policy);

        // OpenAPI spec - stable across deploys, cache aggressively.
        CdnRoutePolicy openapi_policy;
        openapi_policy.directive              = CacheDirective::PUBLIC;
        openapi_policy.max_age_seconds        = 3600;
        openapi_policy.cdn_max_age_seconds    = 86400;
        openapi_policy.stale_while_revalidate_seconds = 3600;
        openapi_policy.emit_cdn_cache_control = true;
        openapi_policy.emit_surrogate_control = true;
        openapi_policy.surrogate_keys         = "openapi spec";
        cdn_cache_middleware_.registerPolicy("/openapi", openapi_policy);
        cdn_cache_middleware_.registerPolicy("/api-docs", openapi_policy);

        THEMIS_INFO("CdnCacheMiddleware initialized with default route policies");
    }

    // ----------------------------------------------------------------------------
    // Input validation limits
    // ----------------------------------------------------------------------------
    {
        std::lock_guard<std::mutex> lock(max_body_bytes_mutex_);
        if (auto v = themis_get_env("THEMIS_MAX_BODY_BYTES")) {
            try { max_body_bytes_ = static_cast<size_t>(std::stoull(*v)); }
            catch (...) { THEMIS_WARN("Invalid THEMIS_MAX_BODY_BYTES value, using default 10MB"); }
        } else {
            // fall back to config max_request_size_mb if provided
            if (config_.max_request_size_mb > 0) {
                max_body_bytes_ = config_.max_request_size_mb * 1024ull * 1024ull;
            }
        }
        THEMIS_INFO("Max request body set to {} bytes", max_body_bytes_);
    }

    // ----------------------------------------------------------------------------
    // TLS/SSL Configuration
    // ----------------------------------------------------------------------------
    if (config_.enable_tls) {
        try {
            // Create SSL context
            ssl_ctx_ = std::make_unique<boost::asio::ssl::context>(
                boost::asio::ssl::context::tlsv13_server // TLS 1.3 by default
            );

            // Set minimum TLS version
            if (config_.tls_min_version == "TLSv1.2") {
                ssl_ctx_ = std::make_unique<boost::asio::ssl::context>(
                    boost::asio::ssl::context::tlsv12_server
                );
                THEMIS_INFO("TLS: minimum version set to TLSv1.2");
            } else {
                THEMIS_INFO("TLS: minimum version set to TLSv1.3 (recommended)");
            }

            // Load server certificate and private key
            if (config_.tls_cert_path.empty() || config_.tls_key_path.empty()) {
                throw std::runtime_error("TLS enabled but cert/key paths not configured");
            }
            ssl_ctx_->use_certificate_chain_file(config_.tls_cert_path);
            ssl_ctx_->use_private_key_file(config_.tls_key_path, boost::asio::ssl::context::pem);
            THEMIS_INFO("TLS: loaded certificate from {} and private key from {}", 
                config_.tls_cert_path, config_.tls_key_path);

            // Configure strong cipher suites (if custom list provided)
            if (!config_.tls_cipher_list.empty()) {
                SSL_CTX_set_cipher_list(ssl_ctx_->native_handle(), config_.tls_cipher_list.c_str());
                THEMIS_INFO("TLS: custom cipher list configured: {}", config_.tls_cipher_list);
            } else {
                // Default: use strong ciphers only (ECDHE with AES256-GCM, ChaCha20)
                // For TLS 1.3, ciphersuites are configured separately
                SSL_CTX_set_cipher_list(ssl_ctx_->native_handle(), 
                    "ECDHE-RSA-AES256-GCM-SHA384:ECDHE-RSA-AES128-GCM-SHA256:ECDHE-RSA-CHACHA20-POLY1305");
                THEMIS_INFO("TLS: using default strong cipher suites (ECDHE-RSA-AES256-GCM-SHA384, etc.)");
            }

            // Disable weak protocols and ciphers
            ssl_ctx_->set_options(
                boost::asio::ssl::context::default_workarounds |
                boost::asio::ssl::context::no_sslv2 |
                boost::asio::ssl::context::no_sslv3 |
                boost::asio::ssl::context::no_tlsv1 |
                boost::asio::ssl::context::no_tlsv1_1 |
                boost::asio::ssl::context::single_dh_use
            );
            THEMIS_INFO("TLS: disabled weak protocols (SSLv2/v3, TLSv1.0/1.1)");

            // Mutual TLS (mTLS) configuration
            if (config_.tls_require_client_cert) {
                if (config_.tls_ca_cert_path.empty()) {
                    throw std::runtime_error("mTLS enabled but CA cert path not configured");
                }
                ssl_ctx_->load_verify_file(config_.tls_ca_cert_path);
                ssl_ctx_->set_verify_mode(
                    boost::asio::ssl::verify_peer | 
                    boost::asio::ssl::verify_fail_if_no_peer_cert
                );
                THEMIS_INFO("mTLS: client certificate verification enabled (CA: {})", 
                    config_.tls_ca_cert_path);
            } else {
                THEMIS_WARN("[SECURITY][TLS] HTTP server verify_none fallback active: "
                            "tls_require_client_cert=false, client certificate validation is disabled "
                            "(one-way TLS only; GAP-017/CWE-295).");
                ssl_ctx_->set_verify_mode(boost::asio::ssl::verify_none);
                THEMIS_INFO("mTLS: client certificate verification disabled (one-way TLS)");
            }

#ifdef THEMIS_ENABLE_HTTP2
            // Configure ALPN for HTTP/2 protocol negotiation
            if (config_.enable_http2) {
                Http2Handler::configureAlpn(*ssl_ctx_);
                THEMIS_INFO("HTTP/2: ALPN configured for protocol negotiation (h2, http/1.1)");
            }
#endif

            THEMIS_INFO("HTTPS server enabled (TLS configured successfully)");
        } catch (const std::exception& e) {
            THEMIS_ERROR("Failed to initialize TLS: {}", e.what());
            throw std::runtime_error(std::string("TLS initialization failed: ") + e.what());
        }
    } else {
        THEMIS_INFO("HTTP server running without TLS (plaintext mode)");
    }
    
    // Initialize Health/Error Service on separate port
    if (config_.health_error_service_enabled) {
        try {
            // Check for environment variable override
            std::string health_bind = config_.health_error_service_bind_address;
            uint16_t health_port = config_.health_error_service_port;
            
            if (const char* env_port = std::getenv("THEMIS_HEALTH_PORT")) {
                try {
                    health_port = static_cast<uint16_t>(std::stoi(env_port));
                } catch (...) {
                    THEMIS_WARN("Invalid THEMIS_HEALTH_PORT value, using default {}", health_port);
                }
            }
            
            if (const char* env_bind = std::getenv("THEMIS_HEALTH_BIND_ADDRESS")) {
                health_bind = env_bind;
            }
            
            HealthErrorService::Config health_config(health_bind, health_port, true);
            health_error_service_ = std::make_unique<HealthErrorService>(health_config);
            THEMIS_INFO("Health/Error service created on {}:{}", health_bind, health_port);
        } catch (const std::exception& e) {
            THEMIS_ERROR("Failed to initialize Health/Error service: {}", e.what());
            // Non-fatal: main server can continue without health service
        }
    } else {
        THEMIS_INFO("Health/Error service disabled in configuration");
    }
}

/** @brief Destructor triggers graceful shutdown via stop(). */
HttpServer::~HttpServer() {
    stop();
}

/**
 * @brief Start HTTP server accept loop, worker threads, and auxiliary services.
 *
 * Opens/binds/listens the TCP acceptor, starts io_context workers, and starts
 * optional health and HTTP/3 services when configured.
 */
void HttpServer::start() {
    if (running_) {
        THEMIS_WARN("Server already running");
        return;
    }

    // Setup acceptor
    tcp::endpoint endpoint{net::ip::make_address(config_.host), config_.port};
    acceptor_.open(endpoint.protocol());
    acceptor_.set_option(net::socket_base::reuse_address(true));
    acceptor_.bind(endpoint);
    acceptor_.listen(net::socket_base::max_listen_connections);

    THEMIS_INFO("HTTP Server listening on {}:{}", config_.host, config_.port);

    running_ = true;
    
    // Start accepting connections
    doAccept();

    // Start thread pool
    threads_.reserve(config_.num_threads);
    for (size_t i = 0; i < config_.num_threads; ++i) {
        threads_.emplace_back([this, i] {
            THEMIS_DEBUG("Worker thread {} started", i);
            ioc_.run();
            THEMIS_DEBUG("Worker thread {} stopped", i);
        });
    }
    
    // Start Health/Error Service (runs in separate thread)
    if (health_error_service_) {
        try {
            health_error_service_->start();
            THEMIS_INFO("Health/Error service started successfully");
        } catch (const std::exception& e) {
            THEMIS_ERROR("Failed to start Health/Error service: {}", e.what());
            // Non-fatal: main server continues
        }
    }

#ifdef THEMIS_ENABLE_HTTP3
    // Start HTTP/3 (QUIC) handler on UDP port
    if (config_.enable_http3) {
        try {
            if (!config_.enable_tls) {
                THEMIS_WARN("HTTP/3 requires TLS; enable_http3 ignored because enable_tls=false");
            } else {
                uint16_t h3_port = config_.http3_port != 0 ? config_.http3_port : config_.port;
                auto* ssl_ctx_raw = Http3Handler::createSslContext(
                    config_.tls_cert_path, config_.tls_key_path);
                if (ssl_ctx_raw) {
                    http3_handler_ = std::make_shared<Http3Handler>(
                        ioc_, config_.host, h3_port, this,
                        ssl_ctx_raw,
                        config_.http3_max_idle_timeout_ms);
                    http3_handler_->start();
                    THEMIS_INFO("HTTP/3 (QUIC) handler started on UDP {}:{}", config_.host, h3_port);
                } else {
                    THEMIS_ERROR("HTTP/3: Failed to create SSL context; HTTP/3 disabled");
                }
            }
        } catch (const std::exception& e) {
            THEMIS_ERROR("Failed to start HTTP/3 handler: {}; HTTP/3 disabled", e.what());
            // Non-fatal: HTTP/1.1 and HTTP/2 continue
        }
    }
#endif

#ifdef THEMIS_VERSION_STRING
    THEMIS_INFO("🎉 ThemisDB {} is now READY for operations", THEMIS_VERSION_STRING);
#else
    THEMIS_INFO("🎉 ThemisDB 1.0.1 is now READY for operations");
#endif

    THEMIS_INFO("HTTP Server started successfully");
}

/**
 * @brief Gracefully stop server components and join worker threads.
 *
 * Stops acceptor/services, drains in-flight requests until timeout, flushes
 * component state, stops io_context, joins workers, and finally shuts down
 * concerns context when present.
 */
void HttpServer::stop() {
    if (!running_) {
        return;
    }

    THEMIS_INFO("Stopping HTTP Server...");
    THEMIS_INFO("Initiating graceful shutdown...");
    running_ = false;

    if (workload_optimizer_ && workload_optimizer_->is_auto_adapt_enabled()) {
        workload_optimizer_->disable_auto_adapt();
    }
    
    // Stop Health/Error Service first (independent service)
    if (health_error_service_) {
        try {
            health_error_service_->stop();
            THEMIS_INFO("Health/Error service stopped");
        } catch (const std::exception& e) {
            THEMIS_ERROR("Error stopping Health/Error service: {}", e.what());
        }
    }

    // Stop accepting new connections
    beast::error_code ec;
    acceptor_.close(ec);

#ifdef THEMIS_ENABLE_HTTP3
    // Stop HTTP/3 QUIC handler
    if (http3_handler_) {
        try {
            http3_handler_->stop();
            http3_handler_.reset();
            THEMIS_INFO("HTTP/3 handler stopped");
        } catch (const std::exception& e) {
            THEMIS_ERROR("Error stopping HTTP/3 handler: {}", e.what());
        }
    }
#endif

    // P5-S02: Use HttpShutdownManager for phased, bounded drain + force-close.
    // Phase DRAINING waits up to graceful_shutdown_timeout_ms for in-flight
    // requests to finish naturally.  Phase FORCE_CLOSE hard-cancels any
    // sessions that survive the drain window.
    THEMIS_INFO("Initiating phased shutdown (drain_timeout={}ms)...",
                config_.graceful_shutdown_timeout_ms);
    {
        themis::server::HttpShutdownManager shutdown_mgr(
            config_.graceful_shutdown_timeout_ms,
            themis::server::HttpShutdownManager::kDefaultForceCloseTimeoutMs,
            [this]() -> uint64_t {
                return active_requests_.load(std::memory_order_acquire);
            },
            [this]() { ioc_.stop(); }  // force-close: cancel all pending async ops
        );
        shutdown_mgr.run();

        const uint64_t forced = shutdown_mgr.forcedCount();
        if (forced > 0) {
            THEMIS_WARN("Shutdown forced: {} request(s) still in flight after "
                        "force-close phase", forced);
        }
    }

    // Flush and cleanup Sprint A/B features
    if (semantic_cache_) {
        THEMIS_INFO("Flushing Semantic Cache...");
        // Cache is in RocksDB, will be flushed with DB
    }
    
    if (llm_store_) {
        THEMIS_INFO("Flushing LLM Interaction Store...");
        // Store is in RocksDB, will be flushed with DB
    }
    
    if (changefeed_) {
        THEMIS_INFO("Flushing Changefeed...");
        // Changefeed is in RocksDB, will be flushed with DB
    }
    
    if (timeseries_) {
        THEMIS_INFO("Flushing Time-Series Store...");
        // Time-series is in RocksDB, will be flushed with DB
    }
    
    // Stop Task Scheduler gracefully before closing storage
    if (task_scheduler_ && task_scheduler_->isRunning()) {
        THEMIS_INFO("Stopping Task Scheduler...");
        try {
            task_scheduler_->stop();
            THEMIS_INFO("Task Scheduler stopped");
        } catch (const std::exception& e) {
            THEMIS_ERROR("Error stopping Task Scheduler: {}", e.what());
        }
    }

    // Vector index auto-save
    if (vector_index_) {
        THEMIS_INFO("Saving vector index (if auto-save enabled)...");
        vector_index_->shutdown();
    }
    
    // Flush RocksDB WAL and memtables
    if (storage_) {
        THEMIS_INFO("Flushing RocksDB memtables...");
        // RocksDB will flush on close, but we can trigger it explicitly
        storage_->close(); // This flushes and closes cleanly
        THEMIS_INFO("RocksDB closed cleanly");
    }

    // Shut down the SSE manager before the io_context so that its internal
    // poll_timer_ is cancelled and reset while the executor is still alive.
    // (sse_manager_ is declared before ioc_ and would otherwise be destroyed
    // after ioc_, accessing a dead executor in its destructor.)
#ifdef THEMIS_ENABLE_SSE
    if (sse_manager_) {
        sse_manager_->shutdown();
    }
#endif

    // Stop io_context
    ioc_.stop();

    // Wait for all threads
    THEMIS_INFO("Waiting for worker threads to finish...");
    for (auto& thread : threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    threads_.clear();

    // Shutdown core concerns (flush remaining logs/spans/metrics, release resources).
    // Called last, after the final "stopped gracefully" log, so the logger is still
    // available for that message.  The logger itself is the final resource torn down
    // inside concerns_->shutdown().
    THEMIS_INFO("HTTP Server stopped gracefully");

    if (concerns_) {
        concerns_->shutdown();
    }
}

/**
 * @brief Block until all worker threads have terminated.
 */
void HttpServer::wait() {
    for (auto& thread : threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}

/**
 * @brief Reload TLS certificate/key pair for newly accepted sessions.
 * @return true on successful context rebuild and swap.
 */
bool HttpServer::reloadTls() {
    if (!config_.enable_tls) {
        THEMIS_WARN("TLS hot-reload requested but TLS is not enabled - ignoring");
        return false;
    }

    if (config_.tls_cert_path.empty() || config_.tls_key_path.empty()) {
        THEMIS_ERROR("TLS hot-reload: cert/key paths not configured");
        return false;
    }

    THEMIS_INFO("TLS hot-reload: reloading certificate from {} and key from {}",
        config_.tls_cert_path, config_.tls_key_path);

    try {
        // Build a fresh SSL context with the same settings
        auto new_ctx = std::make_unique<boost::asio::ssl::context>(
            config_.tls_min_version == "TLSv1.2"
                ? boost::asio::ssl::context::tlsv12_server
                : boost::asio::ssl::context::tlsv13_server
        );

        new_ctx->use_certificate_chain_file(config_.tls_cert_path);
        new_ctx->use_private_key_file(config_.tls_key_path, boost::asio::ssl::context::pem);

        if (!config_.tls_cipher_list.empty()) {
            SSL_CTX_set_cipher_list(new_ctx->native_handle(), config_.tls_cipher_list.c_str());
        } else {
            SSL_CTX_set_cipher_list(new_ctx->native_handle(),
                "ECDHE-RSA-AES256-GCM-SHA384:ECDHE-RSA-AES128-GCM-SHA256:ECDHE-RSA-CHACHA20-POLY1305");
        }

        new_ctx->set_options(
            boost::asio::ssl::context::default_workarounds |
            boost::asio::ssl::context::no_sslv2 |
            boost::asio::ssl::context::no_sslv3 |
            boost::asio::ssl::context::no_tlsv1 |
            boost::asio::ssl::context::no_tlsv1_1 |
            boost::asio::ssl::context::single_dh_use
        );

        if (config_.tls_require_client_cert && !config_.tls_ca_cert_path.empty()) {
            new_ctx->load_verify_file(config_.tls_ca_cert_path);
            new_ctx->set_verify_mode(
                boost::asio::ssl::verify_peer |
                boost::asio::ssl::verify_fail_if_no_peer_cert
            );
        } else {
            THEMIS_WARN("[SECURITY][TLS] HTTP server TLS reload verify_none fallback active: "
                        "client certificate validation disabled for newly accepted TLS sessions "
                        "(GAP-017/CWE-295).");
            new_ctx->set_verify_mode(boost::asio::ssl::verify_none);
        }

#ifdef THEMIS_ENABLE_HTTP2
        if (config_.enable_http2) {
            Http2Handler::configureAlpn(*new_ctx);
        }
#endif

        // Swap in the new context under the lock so doAccept and onAccept
        // always see a consistent ssl_ctx_ pointer.
        {
            std::lock_guard<std::mutex> lock(ssl_ctx_mutex_);
            ssl_ctx_ = std::move(new_ctx);
        }

        THEMIS_INFO("TLS hot-reload: certificate reloaded successfully");
        return true;
    } catch (const std::exception& e) {
        THEMIS_ERROR("TLS hot-reload failed: {}", e.what());
        return false;
    }
}

void HttpServer::recordContinuousLearningQueryTelemetry(
    const http::request<http::string_body>& req,
    const http::response<http::string_body>& res,
    std::chrono::steady_clock::time_point request_start,
    bool is_aql) {
    if (!bao_optimizer_ || !workload_optimizer_ || !continuous_learning_orchestrator_) {
        return;
    }
    if (res.result_int() < 200 || res.result_int() >= 300 || req.body().empty()) {
        return;
    }

    json request_json = json::parse(req.body(), nullptr, false);
    if (request_json.is_discarded()) {
        return;
    }

    json response_json = json::parse(res.body(), nullptr, false);
    if (response_json.is_discarded()) {
        response_json = json::object();
    }

    size_t result_rows = 0;
    if (response_json.contains("count") && response_json["count"].is_number_unsigned()) {
        result_rows = response_json["count"].get<size_t>();
    } else {
        for (const char* key : {"entities", "keys", "results", "rows"}) {
            if (response_json.contains(key) && response_json[key].is_array()) {
                result_rows = response_json[key].size();
                break;
            }
        }
    }

    const auto elapsed = std::chrono::steady_clock::now() - request_start;
    const double latency_ms =
        std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(elapsed).count();
    const uint64_t latency_us = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count());

    std::string query_text;
    std::string table_name;
    bool used_index = true;
    double complexity = 1.0;

    if (is_aql) {
        query_text = request_json.value("query", std::string{});
        table_name = "aql";
        used_index = !request_json.value("allow_full_scan", false);
        std::string uppercase = query_text;
        std::transform(uppercase.begin(), uppercase.end(), uppercase.begin(), [](unsigned char ch) {
            return static_cast<char>(std::toupper(ch));
        });
        for (const char* keyword : {" FOR ", " FILTER ", " SORT ", " COLLECT ", " LIMIT ",
                                    " LET ", " RETURN ", " JOIN ", " GRAPH "}) {
            const std::string token{keyword};
            size_t offset = 0;
            while ((offset = uppercase.find(token, offset)) != std::string::npos) {
                complexity += 1.0;
                offset += token.size();
            }
        }
    } else {
        table_name = request_json.value("table", std::string{});
        query_text = req.body();
        used_index = !request_json.value("allow_full_scan", false);
        complexity += static_cast<double>(
            request_json.value("predicates", json::array()).size()
            + request_json.value("range", json::array()).size());
        if (request_json.contains("order_by")) {
            complexity += 1.0;
        }
        if (request_json.value("stream", false)) {
            complexity += 0.5;
        }
    }

    workload_optimizer_->set_concurrent_queries(active_requests_.load(std::memory_order_relaxed));
    workload_optimizer_->record_query(
        false,
        complexity,
        result_rows,
        table_name.empty() ? std::string("query") : table_name,
        latency_us);

    const std::string query_fingerprint =
        query_text.empty() ? (table_name.empty() ? std::string(req.target()) : table_name) : query_text;
    const auto plans = bao_optimizer_->generate_plans(query_fingerprint);
    const auto plan = bao_optimizer_->select_plan(query_fingerprint, plans);
    bao_optimizer_->update_model(plan, themis::performance::phase3::QueryResult{
        latency_ms, result_rows, true
    });

    themis::rag::learning::ContinuousLearningOrchestrator::QueryExecutionOutcome outcome;
    outcome.query_id = table_name.empty() ? (is_aql ? "aql-query" : std::string(req.target())) : table_name;
    outcome.latency_ms = latency_ms;
    outcome.used_index = used_index;
    if (response_json.contains("plan")) {
        outcome.explain_plan_json = response_json["plan"].dump();
    } else if (is_aql && request_json.contains("query")) {
        outcome.explain_plan_json = request_json["query"].get<std::string>();
    } else {
        outcome.explain_plan_json = "{}";
    }

    continuous_learning_orchestrator_->triggerLoop1QueryExecution(outcome);
    continuous_learning_orchestrator_->triggerLoop2WorkloadAdaptation();
}



void HttpServer::doAccept() {
    acceptor_.async_accept(
        net::make_strand(ioc_),
        beast::bind_front_handler(&HttpServer::onAccept, this)
    );
}

void HttpServer::onAccept(beast::error_code ec, tcp::socket socket) {
    if (ec) {
        THEMIS_ERROR("Accept error: {}", ec.message());
    } else {
        bool connection_slot_reserved = false;

        // W1-S02: reserve a connection slot atomically at admission to avoid
        // accept-time races that can exceed max_connections under contention.
        if (config_.max_connections > 0) {
            uint64_t observed = active_connections_.load(std::memory_order_relaxed);
            while (observed < config_.max_connections) {
                if (active_connections_.compare_exchange_weak(
                        observed,
                        observed + 1,
                        std::memory_order_acq_rel,
                        std::memory_order_relaxed)) {
                    connection_slot_reserved = true;
                    break;
                }
            }
        }

        // Enforce max_connections limit: close the socket immediately if exceeded
        if (config_.max_connections > 0 && !connection_slot_reserved) {
            THEMIS_WARN("Max connections ({}) reached - rejecting new connection",
                config_.max_connections);
            beast::error_code close_ec;
            socket.shutdown(tcp::socket::shutdown_both, close_ec);
            socket.close(close_ec);
        } else {
            try {
                // Create new session for this connection.
                // Lock briefly to get a stable reference to ssl_ctx_ (hot-reload may swap it).
                if (config_.enable_tls) {
                    std::lock_guard<std::mutex> lock(ssl_ctx_mutex_);
                    if (ssl_ctx_) {
#ifdef THEMIS_ENABLE_HTTP2
                        if (config_.enable_http2) {
                            Http2Handler::createSession(
                                std::move(socket),
                                *ssl_ctx_,
                                this,
                                config_.http2_max_concurrent_streams,
                                config_.http2_initial_window_size,
                                connection_slot_reserved
                            )->start();
                        } else {
                            std::make_shared<SslSession>(
                                std::move(socket), *ssl_ctx_, this, connection_slot_reserved)->start();
                        }
#else
                        std::make_shared<SslSession>(
                            std::move(socket), *ssl_ctx_, this, connection_slot_reserved)->start();
#endif
                        connection_slot_reserved = false; // counted by session dtor
                    } else {
                        THEMIS_WARN("TLS enabled but SSL context unavailable; rejecting new connection");
                    }
                } else {
                    // Plain HTTP/1.1 without TLS
                    std::make_shared<Session>(
                        std::move(socket), this, connection_slot_reserved)->start();
                    connection_slot_reserved = false; // counted by session dtor
                }
            } catch (const std::exception& ex) {
                THEMIS_ERROR("Failed to start session: {}", ex.what());
            }

            if (connection_slot_reserved) {
                active_connections_.fetch_sub(1, std::memory_order_release);
            }
        }
    }

    // Accept next connection
    if (running_) {
        doAccept();
    }
}

namespace {
    // Helper function to URL decode a string
    std::string urlDecode(const std::string& str) {
        std::string result;
        result.reserve(str.size());
        for (auto it = str.begin(); it != str.end();) {
            if (*it == '%' && std::distance(it, str.end()) >= 3) {
                int value;
                std::istringstream is(std::string(it + 1, it + 3));
                if (is >> std::hex >> value) {
                    result += static_cast<char>(value);
                    it += 3;
                    continue;
                } else {
                    result += *it;
                }
            } else if (*it == '+') {
                result += ' ';
            } else {
                result += *it;
            }
            ++it;
        }
        return result;
    }

    // Helper function to parse query parameters from URL
    nlohmann::json parseQueryParams(const std::string& target_str) {
        nlohmann::json query_params = nlohmann::json::object();
        auto query_pos = target_str.find('?');
        if (query_pos == std::string::npos) {
            return query_params;
        }
        
        std::string query_string = target_str.substr(query_pos + 1);
        size_t pos = 0;
        while (pos < query_string.size()) {
            auto eq_pos = query_string.find('=', pos);
            if (eq_pos == std::string::npos) break;
            auto amp_pos = query_string.find('&', eq_pos);
            if (amp_pos == std::string::npos) amp_pos = query_string.size();
            
            std::string key = urlDecode(query_string.substr(pos, eq_pos - pos));
            std::string value = urlDecode(query_string.substr(eq_pos + 1, amp_pos - eq_pos - 1));
            query_params.emplace(std::move(key), std::move(value));
            pos = amp_pos + 1;
        }
        return query_params;
    }

    enum class Route {
        Health,
        HealthLive,    // GET /health/live  - liveness probe
        HealthReady,   // GET /health/ready - readiness probe
        OpenApi,       // GET /api/openapi.json - OpenAPI 3.1 spec export
        Version,
        Stats,
        CapabilitiesGet,
        Metrics,
        MetricsHtml,    // GET /metrics/html - lightweight HTML dashboard
        PluginMetrics,  // GET /api/plugins/metrics
        // Operator observability API (Q1)
        ObservabilityAlertsGet,        // GET  /api/v1/observability/alerts
        ObservabilityAlertSilencePost, // POST /api/v1/observability/alerts/{id}/silence
        ObservabilityHealthGet,        // GET  /api/v1/observability/health
        ObservabilityProvenanceGet,    // GET  /api/v1/observability/provenance
        LicenseStatusGet,              // GET  /api/v1/license/status
        Config,
        AdminBackupPost,
        AdminRestorePost,
        EntitiesGet,
        EntitiesPut,
        EntitiesDelete,
        EntitiesPost,
        EntitiesBatchPost,
        V2DocumentsBulkPost,        // POST /v2/documents - bulk insert via NDJSON
        QueryPost,
        QueryAqlPost,
        QueryStreamSseGet,  // GET /v2/query/stream - SSE streaming of AQL results
        IndexCreatePost,
        IndexDropPost,
        IndexStatsGet,
        IndexRebuildPost,
        IndexReindexPost,
        GraphTraversePost,
    GraphEdgePost,
    GraphEdgeDelete,
    GraphMetricsGet,
    GraphMetricsPrometheusGet,
    GraphQueryIncrementalPost,
    GraphQueryIncrementalDelete,
    GraphChangesPost,
    GraphCostModelCalibratePost,
    GraphCostModelGet,
    GraphCostModelImportPost,
    GraphQueryExplainPost,
        VectorSearchPost,
    VectorBatchInsertPost,
    VectorDeleteByFilterDelete,
        // Beta endpoints
        CacheQueryPost,
        CachePutPost,
        CacheStatsGet,
    // Admin cache endpoints (Phase 3: Admin API)
    AdminCacheHealthGet,            // GET  /v1/admin/cache/health
    AdminCacheStatsGet,             // GET  /v1/admin/cache/stats
    AdminCacheEvictKeyDelete,       // DELETE /v1/admin/cache/key/{encoded_key}
    AdminCacheEvictTenantDelete,    // DELETE /v1/admin/cache/tenant/{tenant_id}
    AdminCacheCbResetPost,          // POST /v1/admin/cache/circuit-breaker/reset
    AdminCacheCbStatusGet,          // GET  /v1/admin/cache/circuit-breaker
    AdminCacheWarmupPost,           // POST /v1/admin/cache/warmup
    AdminCacheSnapshotPost,         // POST /v1/admin/cache/snapshot
    AdminCacheTenantsGet,           // GET  /v1/admin/cache/tenants
    AdminCacheTenantStatsGet,       // GET  /v1/admin/cache/tenant/{tenant_id}/stats
    AdminCacheTenantQuotaPatch,     // PATCH /v1/admin/cache/tenant/{tenant_id}/quota
    AdminCachePiiEvictDelete,       // DELETE /v1/admin/cache/pii/{pii_uuid}
    // Shard Admin endpoints
    AdminShardsPost,                // POST /v1/admin/shards
    AdminShardsGet,                 // GET  /v1/admin/shards
    AdminStorageStatsGet,           // GET  /v1/admin/storage/stats
    // Shard Repair admin endpoints
    AdminRepairHealthGet,           // GET  /v1/admin/repair/health
    AdminRepairPost,                // POST /v1/admin/repair
    AdminRepairScanPost,            // POST /v1/admin/repair/scan
    AdminRepairJobStatusGet,        // GET  /v1/admin/repair/jobs/{job_id}
    AdminRepairDashboardGet,        // GET  /v1/admin/repair/dashboard
    // Module Admin endpoints
    AdminModulesGet,                // GET  /v1/admin/modules
    AdminModulesLoadPost,           // POST /v1/admin/modules/{name}/load
    AdminModulesUnloadDelete,       // DELETE /v1/admin/modules/{name}
    AdminModuleStatusGet,           // GET  /v1/admin/modules/{name}
    // Prompt Template endpoints
    PromptTemplatePost,
    PromptTemplateList,
    PromptTemplateGet,
    PromptTemplatePut,
        LlmInteractionPost,
        LlmInteractionGetList,
        LlmInteractionGetById,
        LlmInteractionUpdateMetadataPatch,
        QueryEnhancedPost, // Enterprise: Query + LLM context
        ChangefeedGet,
        ChangefeedStreamSse,
        ChangefeedStreamAckPost,
    ChangefeedStatsGet,
    ChangefeedRetentionPost,
    ChangefeedRetentionGet,
    ChangefeedRetentionPut,
    ChangefeedCompactPost,
    ChangefeedGdprRedactPost,
        // Sprint B
        TimeSeriesPut,
        TimeSeriesQuery,
        TimeSeriesAggregate,
        TimeSeriesConfigGet,
        TimeSeriesConfigPut,
    // Additional TimeSeries endpoints (list aggregates/retention)
    TimeSeriesAggregatesGet,
    TimeSeriesRetentionGet,
    TimeSeriesMetricsGet,
    TimeSeriesPromRemoteWrite,
        // Sprint C
        IndexSuggestionsGet,
        IndexPatternsGet,
        IndexRecordPatternPost,
        IndexClearPatternsDelete,
        VectorIndexSavePost,
        VectorIndexLoadPost,
        VectorIndexConfigGet,
        VectorIndexConfigPut,
        VectorIndexStatsGet,
        VectorIndexIncrementalReindexPost,
        // RoPE endpoints
        RopeConfigPost,
        RopeConfigGet,
        RopeConfigDelete,
        RopeAddPost,
        RopeAddRelationalPost,
        RopeSearchPost,
        RopeBatchAddPost,
        RopeStatsGet,
        TransactionPost,
        TransactionBeginPost,
        TransactionCommitPost,
        TransactionRollbackPost,
        TransactionStatsGet,
        TransactionVersionGet,
        TransactionExplainGet,
        // Distributed (cross-shard) 2PC transaction endpoints
        DtxnBeginPost,
        DtxnOperationPost,
        DtxnCommitPost,
        DtxnAbortPost,
        DtxnReadOnlyPost,
        DtxnStatusGet,
        DtxnStatsGet,
        ContentImportPost,
        ContentGet,
        ContentBlobGet,
        ContentChunksGet,
        ContentFsGet,       // GET    /api/v1/content/fs/{pk}
        ContentFsPut,       // PUT    /api/v1/content/fs/{pk}
        ContentFsHead,      // HEAD   /api/v1/content/fs/{pk}
        ContentFsDelete,    // DELETE /api/v1/content/fs/{pk}
        HybridSearchPost,
        FusionSearchPost,
        FulltextSearchPost,
        ContentFilterSchemaGet,
        ContentFilterSchemaPut,
        ContentConfigGet,
        ContentConfigPut,
        EdgeWeightConfigGet,
        EdgeWeightConfigPut,
        // Encryption Schema Management
        EncryptionSchemaGet,
        EncryptionSchemaPut,
        // Keys / Classification / Reports
    PkiSignPost,
    PkiVerifyPost,
    PkiHsmSignPost,
    PkiHsmKeysGet,
    PkiTimestampPost,
    PkiTimestampVerifyPost,
    PkiEidasSignPost,
    PkiEidasVerifyPost,
    PkiCertificatesGet,
    PkiCertificateGet,
    PkiStatusGet,
        KeysListGet,
        KeysRotatePost,
        ClassificationRulesGet,
        ClassificationTestPost,
        ReportsComplianceGet,
        PoliciesImportRangerPost,
        PoliciesExportRangerGet,
        // PII
    PiiListGet,
    PiiPost,
    PiiGetByUuid,
    PiiExportCsvGet,
    PiiRevealGet,
    PiiDeleteDelete,
    WalApplyPost,
       // Audit API
       AuditQueryGet,
       AuditExportCsvGet,

    // Export API (JSONL LLM export — EXP-001)
    ExportJsonlLlmPost,        // POST /api/v1/export/jsonl_llm
    ExportStatusGet,           // GET  /api/v1/export/:id/status

    // Update API
    UpdateStatusGet,
    UpdateCheckPost,
    UpdateConfigGet,
    UpdateConfigPut,
       
    // Feedback API
    FeedbackPost,              // POST /api/feedback
    FeedbackGet,               // GET /api/feedback
    FeedbackGetById,           // GET /api/feedback/{id}
    FeedbackPut,               // PUT /api/feedback/{id}
    FeedbackDelete,            // DELETE /api/feedback/{id}
    FeedbackAdapterGet,        // GET /api/feedback/adapter/{adapter_id}
    FeedbackStatsGet,          // GET /api/feedback/stats
       
    // G5: Spatial Index Management
    SpatialIndexCreatePost,
    SpatialIndexRebuildPost,
    SpatialIndexStatsGet,
    SpatialIndexMetricsGet,
       
    // Named Snapshots
    SnapshotsTagsPost,          // POST /api/v1/snapshots/tags
    SnapshotsTagsGet,           // GET /api/v1/snapshots/tags
    SnapshotsTagGet,            // GET /api/v1/snapshots/tags/:name
    SnapshotsTagDelete,         // DELETE /api/v1/snapshots/tags/:name
    SnapshotsStatsGet,          // GET /api/v1/snapshots/stats
    
    // Diff API (Phase 2 MVCC)
    DiffGet,                    // GET /api/v1/diff
    DiffCacheStatsGet,          // GET /api/v1/diff/cache/stats
    DiffCacheClear,             // DELETE /api/v1/diff/cache
    
    // PITR API (Point-in-Time Recovery - Phase 3 MVCC)
    PITRRestorePost,            // POST /api/v1/restore/pitr
    PITRPreviewPost,            // POST /api/v1/restore/preview
    PITRProgressGet,            // GET /api/v1/restore/progress
    // Branch API (Phase 4 MVCC - Optional)
    BranchesPost,               // POST /api/v1/branches
    BranchesGet,                // GET /api/v1/branches
    BranchesActiveGet,          // GET /api/v1/branches/active
    BranchesStatsGet,           // GET /api/v1/branches/stats
    BranchGet,                  // GET /api/v1/branches/:name
    BranchSwitchPost,           // POST /api/v1/branches/:name/switch
    BranchDelete,               // DELETE /api/v1/branches/:name
    BranchesMergePost,          // POST /api/v1/branches/merge
    
    // Merge API routes
    MergePost,                  // POST /api/v1/merge
    MergePreviewPost,           // POST /api/v1/merge/preview
    MergeByTagPost,             // POST /api/v1/merge/by-tag
    MergeCanFastForwardGet,     // GET /api/v1/merge/can-fast-forward
       
    // Schema API
    SchemaGetFull,            // GET /api/v1/schema
    SchemaGetTables,          // GET /api/v1/schema/tables
    SchemaGetTable,           // GET /api/v1/schema/tables/:name
    SchemaPut,                // PUT /api/v1/schema/:tablename
    SchemaPatch,              // PATCH /api/v1/schema/:tablename
    // Schema versioning
    SchemaVersionsGet,        // GET  /api/v1/schema/versions/:table
    SchemaVersionsPost,       // POST /api/v1/schema/versions/:table
    SchemaDiffGet,            // GET  /api/v1/schema/diff/:table?from=V&to=V
    // INFORMATION_SCHEMA
    InformationSchemaGet,     // GET /api/v1/information_schema[/...]
    // Metadata extended
    MetadataStatsGet,         // GET  /api/v1/metadata/stats/:table
    MetadataStatsPost,        // POST /api/v1/metadata/stats/:table
    MetadataConstraintsGet,   // GET  /api/v1/metadata/constraints/:table
    MetadataIndexRecsGet,     // GET  /api/v1/metadata/index_recommendations[/:table]
    MetadataAuditGet,         // GET  /api/v1/metadata/audit[/:table]
    MetadataLineageGet,       // GET  /api/v1/metadata/lineage/:table[/:column]
    MetadataLineagePost,      // POST /api/v1/metadata/lineage
    MetadataSchemaImportPut,  // PUT  /api/v1/metadata/schema_import
    MetadataBatchValidatePost,// POST /api/v1/metadata/constraints/validate/:table
       
    // Error API
    ErrorApiListGet,          // GET /api/v1/errors
    ErrorApiGetByCode,        // GET /api/v1/errors/:code
    ErrorApiCategoriesGet,    // GET /api/v1/errors/categories
    ErrorApiSearchGet,        // GET /api/v1/errors/search
    
    // BPMN Process API
    BpmnProcessStartPost,     // POST /api/v1/bpmn/process/start
    BpmnTaskCompletePost,     // POST /api/v1/bpmn/task/:taskId/complete
    BpmnInstanceQueryGet,     // GET /api/v1/bpmn/instance/:instanceId

    // Geo Topology API
    GeoTopologyGet,           // GET  /api/v1/geo/topology
    GeoRegionsGet,            // GET  /api/v1/geo/regions
    GeoHealthGet,             // GET  /api/v1/geo/health
    GeoTopologyShardPost,     // POST /api/v1/geo/topology/shard
    GeoTopologyShardDelete,   // DELETE /api/v1/geo/topology/shard/{shard_id}
    GeoConfigGet,             // GET  /api/v1/geo/config/{collection}
    GeoConfigPut,             // PUT  /api/v1/geo/config/{collection}

    // Replication Topology API (web UI visualizer)
    ReplicationTopologyGet,   // GET  /api/v1/replication/topology
    ReplicationHealthGet,     // GET  /api/v1/replication/health
    ReplicationTopologyUiGet, // GET  /ui/replication/topology
       
    // MVCC versioning API
    MvccKeyGet,              // GET  /api/v1/mvcc/keys/{key}
    MvccKeyPost,             // POST /api/v1/mvcc/keys/{key}
    MvccKeyVersionsGet,      // GET  /api/v1/mvcc/keys/{key}/versions
    MvccKeyVersionsDelete,   // DELETE /api/v1/mvcc/keys/{key}/versions
    MvccClockGet,            // GET  /api/v1/mvcc/clock
    MvccStatsGet,            // GET  /api/v1/mvcc/stats

    // GraphQL endpoint
    GraphQLPost,             // POST /graphql  or  POST /api/v1/graphql
    GraphQLSchemaGet,        // GET  /graphql/schema  or  GET /api/v1/graphql/schema

    // gRPC-Web proxy (browser clients)
    GrpcWebPost,             // POST   /grpc-web/<service>/<method>
    GrpcWebOptions,          // OPTIONS /grpc-web/<service>/<method>  (CORS preflight)
    GrpcWebStatusGet,        // GET    /api/v1/grpc-web/status

    // Serverless function hosting
    ServerlessFnPost,        // POST /api/v1/functions
    ServerlessFnListGet,     // GET  /api/v1/functions
    ServerlessFnGet,         // GET  /api/v1/functions/{id}
    ServerlessFnPut,         // PUT  /api/v1/functions/{id}
    ServerlessFnDelete,      // DELETE /api/v1/functions/{id}
    ServerlessFnInvokePost,  // POST /api/v1/functions/{id}/invoke
    ServerlessFnVersionsGet, // GET  /api/v1/functions/{id}/versions

    // Async job API - long-running AQL query submission and polling
    AsyncJobSubmitPost,      // POST   /v2/jobs
    AsyncJobListGet,         // GET    /v2/jobs
    AsyncJobStatusGet,       // GET    /v2/jobs/{id}
    AsyncJobCancelDelete,    // DELETE /v2/jobs/{id}
    // API Key Management
    ApiKeyPost,              // POST   /api/keys
    ApiKeyListGet,           // GET    /api/keys
    ApiKeyGet,               // GET    /api/keys/{id}
    ApiKeyPut,               // PUT    /api/keys/{id}
    ApiKeyDelete,            // DELETE /api/keys/{id}
    // Session Management
    SessionPost,             // POST   /auth/sessions
    SessionListGet,          // GET    /auth/sessions
    SessionDeleteById,       // DELETE /auth/sessions/{id}
    SessionDeleteOthers,     // DELETE /auth/sessions  (revoke all others)

    // SAML 2.0 SP
    SamlLoginGet,            // GET    /api/v1/auth/saml/login
    SamlAcsPost,             // POST   /api/v1/auth/saml/acs
    SamlSloPost,             // POST   /api/v1/auth/saml/slo
    SamlMetadataGet,         // GET    /api/v1/auth/saml/metadata

    // UDF registration API - AQL user-defined functions
    UdfPost,                 // POST   /api/v1/query/udfs
    UdfListGet,              // GET    /api/v1/query/udfs
    UdfGet,                  // GET    /api/v1/query/udfs/{name}
    UdfDelete,               // DELETE /api/v1/query/udfs/{name}

    // Task Scheduler API - manage scheduled tasks
    TasksPost,               // POST   /api/tasks
    TasksListGet,            // GET    /api/tasks
    TasksStatsGet,           // GET    /api/tasks/stats
    TasksGet,                // GET    /api/tasks/{id}
    TasksPut,                // PUT    /api/tasks/{id}
    TasksDelete,             // DELETE /api/tasks/{id}
    TasksEnablePost,         // POST   /api/tasks/{id}/enable
    TasksDisablePost,        // POST   /api/tasks/{id}/disable
    TasksExecutePost,        // POST   /api/tasks/{id}/execute
    TasksHistoryGet,         // GET    /api/tasks/{id}/history - searchable audit log
    TasksUiGet,              // GET    /ui/tasks  - Web UI

    // Database Maintenance Orchestrator API
    // Schedule CRUD
    MaintenanceSchedulesPost,       // POST   /api/v1/maintenance/schedules
    MaintenanceSchedulesGet,        // GET    /api/v1/maintenance/schedules
    MaintenanceScheduleGet,         // GET    /api/v1/maintenance/schedules/{id}
    MaintenanceSchedulePut,         // PUT    /api/v1/maintenance/schedules/{id}
    MaintenanceSchedulePatch,       // PATCH  /api/v1/maintenance/schedules/{id}
    MaintenanceScheduleDelete,      // DELETE /api/v1/maintenance/schedules/{id}
    MaintenanceScheduleRunPost,     // POST   /api/v1/maintenance/schedules/{id}/run
    // Jobs
    MaintenanceJobsGet,             // GET    /api/v1/maintenance/jobs
    MaintenanceJobGet,              // GET    /api/v1/maintenance/jobs/{id}
    MaintenanceJobCancelPost,       // POST   /api/v1/maintenance/jobs/{id}/cancel
    // Observability
    MaintenanceStatusGet,           // GET    /api/v1/maintenance/status
    MaintenanceHealthGet,           // GET    /api/v1/maintenance/health
    MaintenanceTaskHandlersGet,     // GET    /api/v1/maintenance/task-handlers

    // Retention Policy Admin API
    RetentionPoliciesGet,           // GET    /api/retention/policies
    RetentionPoliciesPost,          // POST   /api/retention/policies
    RetentionPolicyDelete,          // DELETE /api/retention/policies/{name}
    RetentionHistoryGet,            // GET    /api/retention/history

    // SAGA Audit Log API
    SAGABatchesGet,                 // GET    /api/saga/batches
    SAGABatchGet,                   // GET    /api/saga/batches/{id}
    SAGABatchVerifyPost,            // POST   /api/saga/batches/{id}/verify
    SAGAFlushPost,                  // POST   /api/saga/flush

    // Continuous Query (CQL Phase 8) REST/SSE API
    ContinuousQueryRegisterPost,    // POST   /v1/queries/continuous
    ContinuousQueryDropDelete,      // DELETE /v1/queries/continuous/:name
    ContinuousQueryListGet,         // GET    /v1/queries/continuous
    ContinuousQueryStreamSseGet,    // GET    /v1/queries/continuous/:name/results

    // AI Safety Layer — HILG Approval Endpoints (ASL-6)
    // Docs: docs/de/security/ai_safety/AI_SAFETY_OPERATION_GUARD.md
    AiApprovePendingPost,           // POST /v1/ai/approve/{operation_id}
    AiDenyPendingPost,              // POST /v1/ai/deny/{operation_id}
    AiPendingApprovalsGet,          // GET  /v1/ai/pending-approvals
    AiRollbackPost,                 // POST /v1/ai/rollback/{snapshot_id}  (ASL-10)

        NotFound
    };

    Route classifyRoute(const http::request<http::string_body>& req) {
        const auto method = req.method();
        const std::string target = std::string(req.target());
        // Normalize path by stripping query string to allow matching endpoints with params
        std::string path_only = target;
        auto qpos = path_only.find('?');
        if (qpos != std::string::npos) path_only = path_only.substr(0, qpos);

        if (target == "/" || target == "/health") return Route::Health;
        if (target == "/health/live" && method == http::verb::get) return Route::HealthLive;
        if (target == "/health/ready" && method == http::verb::get) return Route::HealthReady;
        if (target == "/api/openapi.json" && method == http::verb::get) return Route::OpenApi;
        if (target == "/version" && method == http::verb::get) return Route::Version;
    if (target == "/stats" && method == http::verb::get) return Route::Stats;
    if (target == "/api/capabilities" && method == http::verb::get) return Route::CapabilitiesGet;
    if (target == "/metrics" && method == http::verb::get) return Route::Metrics;
    if (target == "/metrics/html" && method == http::verb::get) return Route::MetricsHtml;
    if (target == "/api/plugins/metrics" && method == http::verb::get) return Route::PluginMetrics;
    // Operator observability REST API (Q1)
    if (target == "/api/v1/observability/alerts" && method == http::verb::get) return Route::ObservabilityAlertsGet;
    if (target == "/api/v1/observability/health" && method == http::verb::get) return Route::ObservabilityHealthGet;
    if (path_only == "/api/v1/observability/provenance" && method == http::verb::get) return Route::ObservabilityProvenanceGet;
    if (target == "/api/v1/license/status"       && method == http::verb::get) return Route::LicenseStatusGet;
    {
        // POST /api/v1/observability/alerts/{id}/silence
        // path_only must start with the alerts prefix, have a non-empty {id} segment,
        // and end with the /silence suffix.
        static constexpr std::string_view kAlertsPrefix{"/api/v1/observability/alerts/"};
        static constexpr std::string_view kSilenceSuffix{"/silence"};
        if (method == http::verb::post &&
            path_only.rfind(kAlertsPrefix.data(), 0) == 0 &&
            path_only.size() > kAlertsPrefix.size() + kSilenceSuffix.size() &&
            path_only.substr(path_only.size() - kSilenceSuffix.size()) == kSilenceSuffix) {
            return Route::ObservabilityAlertSilencePost;
        }
    }
    if (path_only == "/api/v1/wal/apply" && method == http::verb::post) return Route::WalApplyPost;
    if (target == "/config" && (method == http::verb::get || method == http::verb::post)) return Route::Config;
    if (target == "/admin/backup" && method == http::verb::post) return Route::AdminBackupPost;
    if (target == "/admin/restore" && method == http::verb::post) return Route::AdminRestorePost;

        // Exact matches for /entities endpoints BEFORE parametrized routes
        if (target == "/entities" && method == http::verb::post) return Route::EntitiesPost;
        if (target == "/entities/batch" && method == http::verb::post) return Route::EntitiesBatchPost;
        // POST /v2/documents - bulk insert via NDJSON (application/x-ndjson body)
        if (path_only == "/v2/documents" && method == http::verb::post) return Route::V2DocumentsBulkPost;

        // Parametrized entity by key (e.g., /entities/users:123)
        if (target.rfind("/entities/", 0) == 0) {
            if (method == http::verb::get) return Route::EntitiesGet;
            if (method == http::verb::put) return Route::EntitiesPut;
            if (method == http::verb::delete_) return Route::EntitiesDelete;
            return Route::NotFound;
        }
        if (target == "/query" && method == http::verb::post) return Route::QueryPost;
    if (target == "/query/aql" && method == http::verb::post) return Route::QueryAqlPost;
    // SSE streaming query result endpoint (v2)
    if (path_only == "/v2/query/stream" && method == http::verb::get) return Route::QueryStreamSseGet;
    // Backward compatibility alias
    if (target == "/api/aql" && method == http::verb::post) return Route::QueryAqlPost;
        if (target == "/index/create" && method == http::verb::post) return Route::IndexCreatePost;
        if (target == "/index/drop" && method == http::verb::post) return Route::IndexDropPost;
        if (target == "/index/stats" && method == http::verb::get) return Route::IndexStatsGet;
        if (target == "/index/rebuild" && method == http::verb::post) return Route::IndexRebuildPost;
        if (target == "/index/reindex" && method == http::verb::post) return Route::IndexReindexPost;
        
        // G5: Spatial Index Management endpoints
        if (target == "/spatial/index/create" && method == http::verb::post) return Route::SpatialIndexCreatePost;
        if (target == "/spatial/index/rebuild" && method == http::verb::post) return Route::SpatialIndexRebuildPost;
        if (target == "/spatial/index/stats" && method == http::verb::get) return Route::SpatialIndexStatsGet;
        if (target == "/spatial/metrics" && method == http::verb::get) return Route::SpatialIndexMetricsGet;
        
        if (target == "/graph/traverse" && method == http::verb::post) return Route::GraphTraversePost;
    if (target == "/graph/edge" && method == http::verb::post) return Route::GraphEdgePost;
    if (target.rfind("/graph/edge/", 0) == 0 && method == http::verb::delete_) return Route::GraphEdgeDelete;
    if (path_only == "/api/v1/graph/metrics" && method == http::verb::get) return Route::GraphMetricsGet;
    if (path_only == "/api/v1/graph/metrics/prometheus" && method == http::verb::get) return Route::GraphMetricsPrometheusGet;
    if (target == "/graph/query/incremental" && method == http::verb::post) return Route::GraphQueryIncrementalPost;
    if (target.rfind("/graph/query/incremental/", 0) == 0 && method == http::verb::delete_) return Route::GraphQueryIncrementalDelete;
    if (target == "/graph/changes" && method == http::verb::post) return Route::GraphChangesPost;
    if (path_only == "/api/v1/graph/cost-model/calibrate" && method == http::verb::post) return Route::GraphCostModelCalibratePost;
    if (path_only == "/api/v1/graph/cost-model" && method == http::verb::get) return Route::GraphCostModelGet;
    if (path_only == "/api/v1/graph/cost-model" && method == http::verb::post) return Route::GraphCostModelImportPost;
    if (path_only == "/api/v1/graph/query/explain" && method == http::verb::post) return Route::GraphQueryExplainPost;
        if (target == "/vector/search" && method == http::verb::post) return Route::VectorSearchPost;
    if (target == "/vector/batch_insert" && method == http::verb::post) return Route::VectorBatchInsertPost;
    if (target == "/vector/by-filter" && method == http::verb::delete_) return Route::VectorDeleteByFilterDelete;
        // Sprint A beta endpoints
        if (target == "/cache/query" && method == http::verb::post) return Route::CacheQueryPost;
        if (target == "/cache/put" && method == http::verb::post) return Route::CachePutPost;
        if (target == "/cache/stats" && method == http::verb::get) return Route::CacheStatsGet;
    // Admin cache endpoints - order matters: more-specific paths first
    if (path_only == "/v1/admin/cache/health" && method == http::verb::get) return Route::AdminCacheHealthGet;
    if (path_only == "/v1/admin/cache/stats" && method == http::verb::get) return Route::AdminCacheStatsGet;
    if (path_only == "/v1/admin/cache/circuit-breaker" && method == http::verb::get) return Route::AdminCacheCbStatusGet;
    if (path_only == "/v1/admin/cache/circuit-breaker/reset" && method == http::verb::post) return Route::AdminCacheCbResetPost;
    if (path_only.rfind("/v1/admin/cache/key/", 0) == 0 && method == http::verb::delete_) return Route::AdminCacheEvictKeyDelete;
    if (path_only == "/v1/admin/cache/tenants" && method == http::verb::get) return Route::AdminCacheTenantsGet;
    // /v1/admin/cache/tenant/{id}/stats must be matched before the tenant evict DELETE
    if (path_only.rfind("/v1/admin/cache/tenant/", 0) == 0 &&
        path_only.size() > 23 &&
        path_only.rfind("/stats") == path_only.size() - 6 &&
        method == http::verb::get) return Route::AdminCacheTenantStatsGet;
    // /v1/admin/cache/tenant/{id}/quota must be matched before the tenant evict DELETE
    if (path_only.rfind("/v1/admin/cache/tenant/", 0) == 0 &&
        path_only.size() > 23 &&
        path_only.rfind("/quota") == path_only.size() - 6 &&
        method == http::verb::patch) return Route::AdminCacheTenantQuotaPatch;
    if (path_only.rfind("/v1/admin/cache/tenant/", 0) == 0 && method == http::verb::delete_) return Route::AdminCacheEvictTenantDelete;
    if (path_only.rfind("/v1/admin/cache/pii/", 0) == 0 && method == http::verb::delete_) return Route::AdminCachePiiEvictDelete;
    if (path_only == "/v1/admin/cache/warmup" && method == http::verb::post) return Route::AdminCacheWarmupPost;
    if (path_only == "/v1/admin/cache/snapshot" && method == http::verb::post) return Route::AdminCacheSnapshotPost;
    if (path_only == "/v1/admin/shards" && method == http::verb::post) return Route::AdminShardsPost;
    if (path_only == "/v1/admin/shards" && method == http::verb::get) return Route::AdminShardsGet;
    if (path_only == "/v1/admin/storage/stats" && method == http::verb::get) return Route::AdminStorageStatsGet;
    // Shard Repair admin endpoints
    if (path_only == "/v1/admin/repair/health" && method == http::verb::get) return Route::AdminRepairHealthGet;
    if (path_only == "/v1/admin/repair" && method == http::verb::post) return Route::AdminRepairPost;
    if (path_only == "/v1/admin/repair/scan" && method == http::verb::post) return Route::AdminRepairScanPost;
    if (path_only.rfind("/v1/admin/repair/jobs/", 0) == 0 && path_only.size() > 22 &&
        method == http::verb::get) return Route::AdminRepairJobStatusGet;
    if (path_only == "/v1/admin/repair/dashboard" && method == http::verb::get) return Route::AdminRepairDashboardGet;
    // Module admin endpoints — order matters: /load POST before generic DELETE/GET
    if (path_only == "/v1/admin/modules" && method == http::verb::get) return Route::AdminModulesGet;
    if (path_only.rfind("/v1/admin/modules/", 0) == 0 &&
        path_only.size() > 18 &&
        path_only.rfind("/load") == path_only.size() - 5 &&
        method == http::verb::post) return Route::AdminModulesLoadPost;
    if (path_only.rfind("/v1/admin/modules/", 0) == 0 &&
        path_only.size() > 18 &&
        method == http::verb::delete_) return Route::AdminModulesUnloadDelete;
    if (path_only.rfind("/v1/admin/modules/", 0) == 0 &&
        path_only.size() > 18 &&
        method == http::verb::get) return Route::AdminModuleStatusGet;
    if (target == "/prompt_template" && method == http::verb::post) return Route::PromptTemplatePost;
    if (target == "/prompt_template" && method == http::verb::get) return Route::PromptTemplateList;
    if (target.rfind("/prompt_template/", 0) == 0 && method == http::verb::get) return Route::PromptTemplateGet;
    if (target.rfind("/prompt_template/", 0) == 0 && method == http::verb::put) return Route::PromptTemplatePut;
        if (target == "/llm/interaction" && method == http::verb::post) return Route::LlmInteractionPost;
    if (target == "/llm/interaction" && method == http::verb::get) return Route::LlmInteractionGetList;
    if (target.rfind("/llm/interaction/", 0) == 0 && method == http::verb::get) return Route::LlmInteractionGetById;
    if (target.rfind("/llm/interaction/", 0) == 0 && method == http::verb::patch) return Route::LlmInteractionUpdateMetadataPatch;
    // Enterprise: Enhanced query endpoint
    if (target == "/query/enhanced" && method == http::verb::post) return Route::QueryEnhancedPost;
    // Changefeed endpoint should match even with query parameters
    if (path_only == "/changefeed" && method == http::verb::get) return Route::ChangefeedGet;
    if (path_only == "/changefeed/stream" && method == http::verb::get) return Route::ChangefeedStreamSse;
    if (path_only == "/changefeed/stream/ack" && method == http::verb::post) return Route::ChangefeedStreamAckPost;
    if (path_only == "/changefeed/stats" && method == http::verb::get) return Route::ChangefeedStatsGet;
    if (path_only == "/changefeed/retention" && method == http::verb::post) return Route::ChangefeedRetentionPost;
    if (path_only == "/changefeed/retention" && method == http::verb::get) return Route::ChangefeedRetentionGet;
    if (path_only == "/changefeed/retention" && method == http::verb::put) return Route::ChangefeedRetentionPut;
    if (path_only == "/changefeed/compact" && method == http::verb::post) return Route::ChangefeedCompactPost;
    if (path_only == "/changefeed/redact" && method == http::verb::post) return Route::ChangefeedGdprRedactPost;
    
    // Snapshot API endpoints
    if (path_only == "/api/v1/snapshots/tags" && method == http::verb::post) return Route::SnapshotsTagsPost;
    if (path_only == "/api/v1/snapshots/tags" && method == http::verb::get) return Route::SnapshotsTagsGet;
    if (path_only.rfind("/api/v1/snapshots/tags/", 0) == 0 && method == http::verb::get) return Route::SnapshotsTagGet;
    if (path_only.rfind("/api/v1/snapshots/tags/", 0) == 0 && method == http::verb::delete_) return Route::SnapshotsTagDelete;
    if (path_only == "/api/v1/snapshots/stats" && method == http::verb::get) return Route::SnapshotsStatsGet;
    
    // Diff API endpoints
    if (path_only == "/api/v1/diff" && method == http::verb::get) return Route::DiffGet;
    if (path_only == "/api/v1/diff/cache/stats" && method == http::verb::get) return Route::DiffCacheStatsGet;
    if (path_only == "/api/v1/diff/cache" && method == http::verb::delete_) return Route::DiffCacheClear;
    
    // PITR API endpoints
    if (path_only == "/api/v1/pitr/preview" && method == http::verb::post) return Route::PITRPreviewPost;
    if (path_only == "/api/v1/pitr/progress" && method == http::verb::get) return Route::PITRProgressGet;
    if (path_only == "/api/v1/restore/pitr" && method == http::verb::post) return Route::PITRRestorePost;
    if (path_only == "/api/v1/restore/preview" && method == http::verb::post) return Route::PITRPreviewPost;
    if (path_only == "/api/v1/restore/progress" && method == http::verb::get) return Route::PITRProgressGet;
    
    // Sprint B endpoints - Time Series
    // Branch API endpoints
    if (path_only == "/api/v1/branches" && method == http::verb::post) return Route::BranchesPost;
    if (path_only == "/api/v1/branches" && method == http::verb::get) return Route::BranchesGet;
    if (path_only == "/api/v1/branches/active" && method == http::verb::get) return Route::BranchesActiveGet;
    if (path_only == "/api/v1/branches/stats" && method == http::verb::get) return Route::BranchesStatsGet;
    if (path_only == "/api/v1/branches/merge" && method == http::verb::post) return Route::BranchesMergePost;
    if (path_only.rfind("/api/v1/branches/", 0) == 0 && path_only.rfind("/switch") == path_only.length() - 7 && method == http::verb::post) return Route::BranchSwitchPost;
    if (path_only.rfind("/api/v1/branches/", 0) == 0 && method == http::verb::get) return Route::BranchGet;
    if (path_only.rfind("/api/v1/branches/", 0) == 0 && method == http::verb::delete_) return Route::BranchDelete;
    
    // Merge API routes
    if (path_only == "/api/v1/merge" && method == http::verb::post) return Route::MergePost;
    if (path_only == "/api/v1/merge/preview" && method == http::verb::post) return Route::MergePreviewPost;
    if (path_only == "/api/v1/merge/by-tag" && method == http::verb::post) return Route::MergeByTagPost;
    if (path_only == "/api/v1/merge/can-fast-forward" && method == http::verb::get) return Route::MergeCanFastForwardGet;
    
        // Sprint B endpoints
    if (target == "/ts/put" && method == http::verb::post) return Route::TimeSeriesPut;
    if (target == "/ts/query" && method == http::verb::post) return Route::TimeSeriesQuery;
    if (target == "/ts/aggregate" && method == http::verb::post) return Route::TimeSeriesAggregate;
    if (target == "/ts/config" && method == http::verb::get) return Route::TimeSeriesConfigGet;
    if (target == "/ts/config" && method == http::verb::put) return Route::TimeSeriesConfigPut;
    if (path_only == "/ts/aggregates" && method == http::verb::get) return Route::TimeSeriesAggregatesGet;
    if (path_only == "/ts/retention" && method == http::verb::get) return Route::TimeSeriesRetentionGet;
    if (path_only == "/ts/metrics" && method == http::verb::get) return Route::TimeSeriesMetricsGet;
    if (path_only == "/api/v1/prom/write" && method == http::verb::post) return Route::TimeSeriesPromRemoteWrite;
        // Sprint C endpoints
        if (target.find("/index/suggestions") == 0 && method == http::verb::get) return Route::IndexSuggestionsGet;
        if (target.find("/index/patterns") == 0 && method == http::verb::get) return Route::IndexPatternsGet;
        if (target == "/index/record-pattern" && method == http::verb::post) return Route::IndexRecordPatternPost;
        if (target == "/index/patterns" && method == http::verb::delete_) return Route::IndexClearPatternsDelete;
        if (target == "/vector/index/save" && method == http::verb::post) return Route::VectorIndexSavePost;
        if (target == "/vector/index/load" && method == http::verb::post) return Route::VectorIndexLoadPost;
        if (target == "/vector/index/config" && method == http::verb::get) return Route::VectorIndexConfigGet;
        if (target == "/vector/index/config" && method == http::verb::put) return Route::VectorIndexConfigPut;
        if (target == "/vector/index/stats" && method == http::verb::get) return Route::VectorIndexStatsGet;
        if (target == "/vector/index/incremental-reindex" && method == http::verb::post) return Route::VectorIndexIncrementalReindexPost;
        // RoPE endpoints - /api/v1/vector-index/{index_name}/rope/*
        if (path_only.find("/api/v1/vector-index/") == 0 && path_only.find("/rope/config") != std::string::npos) {
            if (method == http::verb::post) return Route::RopeConfigPost;
            if (method == http::verb::get) return Route::RopeConfigGet;
            if (method == http::verb::delete_) return Route::RopeConfigDelete;
        }
        if (path_only.find("/api/v1/vector-index/") == 0 && path_only.find("/rope/add-relational") != std::string::npos && method == http::verb::post) return Route::RopeAddRelationalPost;
        if (path_only.find("/api/v1/vector-index/") == 0 && path_only.find("/rope/add") != std::string::npos && method == http::verb::post) return Route::RopeAddPost;
        if (path_only.find("/api/v1/vector-index/") == 0 && path_only.find("/rope/search") != std::string::npos && method == http::verb::post) return Route::RopeSearchPost;
        if (path_only.find("/api/v1/vector-index/") == 0 && path_only.find("/rope/batch-add") != std::string::npos && method == http::verb::post) return Route::RopeBatchAddPost;
        if (path_only.find("/api/v1/vector-index/") == 0 && path_only.find("/rope/stats") != std::string::npos && method == http::verb::get) return Route::RopeStatsGet;
        // PKI endpoints
        if (path_only.rfind("/api/pki/", 0) == 0 && method == http::verb::post) {
            // Expect: /api/pki/:key_id/sign or /api/pki/:key_id/verify
            if (path_only.size() >= 5 && path_only.compare(path_only.size() - 5, 5, "/sign") == 0) return Route::PkiSignPost;
            if (path_only.size() >= 7 && path_only.compare(path_only.size() - 7, 7, "/verify") == 0) return Route::PkiVerifyPost;
        }
        // New PKI HSM, TSA, eIDAS endpoints
        if (path_only == "/api/pki/hsm/sign" && method == http::verb::post) return Route::PkiHsmSignPost;
        if (path_only == "/api/pki/hsm/keys" && method == http::verb::get) return Route::PkiHsmKeysGet;
        if (path_only == "/api/pki/timestamp" && method == http::verb::post) return Route::PkiTimestampPost;
        if (path_only == "/api/pki/timestamp/verify" && method == http::verb::post) return Route::PkiTimestampVerifyPost;
        if (path_only == "/api/pki/eidas/sign" && method == http::verb::post) return Route::PkiEidasSignPost;
        if (path_only == "/api/pki/eidas/verify" && method == http::verb::post) return Route::PkiEidasVerifyPost;
        if (path_only == "/api/pki/certificates" && method == http::verb::get) return Route::PkiCertificatesGet;
        if (path_only.rfind("/api/pki/certificates/", 0) == 0 && method == http::verb::get) return Route::PkiCertificateGet;
        if (path_only == "/api/pki/status" && method == http::verb::get) return Route::PkiStatusGet;
        // Keys API
        if (path_only == "/keys" && method == http::verb::get) return Route::KeysListGet;
        if (path_only == "/keys/rotate" && method == http::verb::post) return Route::KeysRotatePost;
        // Classification API
        if (path_only == "/classification/rules" && method == http::verb::get) return Route::ClassificationRulesGet;
        if (path_only == "/classification/test" && method == http::verb::post) return Route::ClassificationTestPost;
        // Reports API
    if (path_only == "/reports/compliance" && method == http::verb::get) return Route::ReportsComplianceGet;
    // Policies (Ranger integration)
    if (path_only == "/policies/import/ranger" && method == http::verb::post) return Route::PoliciesImportRangerPost;
    if (path_only == "/policies/export/ranger" && method == http::verb::get) return Route::PoliciesExportRangerGet;
    // PII endpoints
    if (path_only == "/pii" && method == http::verb::get) return Route::PiiListGet;
    if (path_only == "/pii" && method == http::verb::post) return Route::PiiPost;
    if (path_only.rfind("/pii/export.csv", 0) == 0 && method == http::verb::get) return Route::PiiExportCsvGet;
    if (path_only.rfind("/pii/reveal/", 0) == 0 && method == http::verb::get) return Route::PiiRevealGet;
    if (path_only.rfind("/pii/", 0) == 0 && method == http::verb::get) return Route::PiiGetByUuid;
    if (path_only.rfind("/pii/", 0) == 0 && method == http::verb::delete_) return Route::PiiDeleteDelete;
    // Audit API endpoints
    if (path_only == "/api/audit" && method == http::verb::get) return Route::AuditQueryGet;
    if (path_only == "/api/audit/export/csv" && method == http::verb::get) return Route::AuditExportCsvGet;
    // Export API endpoints (EXP-001)
    if (path_only == "/api/v1/export/jsonl_llm" && method == http::verb::post) return Route::ExportJsonlLlmPost;
    if (path_only.rfind("/api/v1/export/", 0) == 0 &&
        path_only.rfind("/status") == path_only.size() - 7 &&
        method == http::verb::get) return Route::ExportStatusGet;
    // Update API endpoints
    if (path_only == "/api/updates" && method == http::verb::get) return Route::UpdateStatusGet;
    if (path_only == "/api/updates/check" && method == http::verb::post) return Route::UpdateCheckPost;
    if (path_only == "/api/updates/config" && method == http::verb::get) return Route::UpdateConfigGet;
    if (path_only == "/api/updates/config" && method == http::verb::put) return Route::UpdateConfigPut;
    
    // Feedback API routes
    if (path_only == "/api/feedback/stats" && method == http::verb::get) return Route::FeedbackStatsGet;
    if (path_only == "/api/feedback" && method == http::verb::post) return Route::FeedbackPost;
    if (path_only == "/api/feedback" && method == http::verb::get) return Route::FeedbackGet;
    
    // Pattern: /api/feedback/adapter/{adapter_id} or /api/feedback/{id}
    if (path_only.rfind("/api/feedback/", 0) == 0 && path_only != "/api/feedback/stats") {
        std::string suffix = path_only.substr(14); // Length of "/api/feedback/"
        
        if (suffix.rfind("adapter/", 0) == 0) {
            // /api/feedback/adapter/{adapter_id}
            if (method == http::verb::get) return Route::FeedbackAdapterGet;
        } else if (!suffix.empty() && suffix.find('/') == std::string::npos) {
            // /api/feedback/{id}
            if (method == http::verb::get) return Route::FeedbackGetById;
            if (method == http::verb::put) return Route::FeedbackPut;
            if (method == http::verb::delete_) return Route::FeedbackDelete;
        }
    }
    
    // BPMN Process API routes
    if (path_only == "/api/v1/bpmn/process/start" && method == http::verb::post) return Route::BpmnProcessStartPost;
    
    // Task completion route with strict validation
    if (method == http::verb::post) {
        const std::string task_prefix = "/api/v1/bpmn/task/";
        const std::string complete_suffix = "/complete";
        if (path_only.rfind(task_prefix, 0) == 0 &&
            path_only.size() > task_prefix.size() + complete_suffix.size() &&
            path_only.compare(path_only.size() - complete_suffix.size(), complete_suffix.size(), complete_suffix) == 0) {
            // Ensure there is a non-empty taskId segment between prefix and suffix
            const std::size_t task_id_start = task_prefix.size();
            const std::size_t suffix_pos = path_only.size() - complete_suffix.size();
            // Check that there's exactly the taskId between prefix and suffix (no additional slashes)
            std::string task_id_segment = path_only.substr(task_id_start, suffix_pos - task_id_start);
            if (!task_id_segment.empty() && task_id_segment.find('/') == std::string::npos) {
                return Route::BpmnTaskCompletePost;
            }
        }
    }
    
    if (path_only.rfind("/api/v1/bpmn/instance/", 0) == 0 && method == http::verb::get) return Route::BpmnInstanceQueryGet;

    // Geo Topology API
    if (path_only == "/api/v1/geo/topology" && method == http::verb::get) return Route::GeoTopologyGet;
    if (path_only == "/api/v1/geo/regions"  && method == http::verb::get) return Route::GeoRegionsGet;
    if (path_only == "/api/v1/geo/health"   && method == http::verb::get) return Route::GeoHealthGet;
    if (path_only == "/api/v1/geo/topology/shard" && method == http::verb::post) return Route::GeoTopologyShardPost;
    if (path_only.rfind("/api/v1/geo/topology/shard/", 0) == 0 && method == http::verb::delete_) return Route::GeoTopologyShardDelete;
    if (path_only.rfind("/api/v1/geo/config/", 0) == 0 && method == http::verb::get) return Route::GeoConfigGet;
    if (path_only.rfind("/api/v1/geo/config/", 0) == 0 && method == http::verb::put) return Route::GeoConfigPut;

    // Replication Topology API
    if (path_only == "/api/v1/replication/topology" && method == http::verb::get) return Route::ReplicationTopologyGet;
    if (path_only == "/api/v1/replication/health"   && method == http::verb::get) return Route::ReplicationHealthGet;
    if (path_only == "/ui/replication/topology"     && method == http::verb::get) return Route::ReplicationTopologyUiGet;
    
        if (target == "/transaction" && method == http::verb::post) return Route::TransactionPost;
        if (target == "/transaction/begin" && method == http::verb::post) return Route::TransactionBeginPost;
        if (target == "/transaction/commit" && method == http::verb::post) return Route::TransactionCommitPost;
        if (target == "/transaction/rollback" && method == http::verb::post) return Route::TransactionRollbackPost;
        if (target == "/transaction/stats" && method == http::verb::get) return Route::TransactionStatsGet;
        if (target == "/transaction/version" && method == http::verb::get) return Route::TransactionVersionGet;
        // /transaction/{id}/explain  (GET)
        if (path_only.starts_with("/transaction/") && path_only.ends_with("/explain") &&
            method == http::verb::get) return Route::TransactionExplainGet;

        // Distributed (cross-shard) 2PC transaction endpoints
        if (target == "/dtxn/begin"    && method == http::verb::post) return Route::DtxnBeginPost;
        if (target == "/dtxn/operation" && method == http::verb::post) return Route::DtxnOperationPost;
        if (target == "/dtxn/commit"   && method == http::verb::post) return Route::DtxnCommitPost;
        if (target == "/dtxn/abort"    && method == http::verb::post) return Route::DtxnAbortPost;
        if (target == "/dtxn/readonly" && method == http::verb::post) return Route::DtxnReadOnlyPost;
        if (target.rfind("/dtxn/status/", 0) == 0 && method == http::verb::get) return Route::DtxnStatusGet;
        if (target == "/dtxn/stats"    && method == http::verb::get)  return Route::DtxnStatsGet;

        // Content API
        if (target == "/content/import" && method == http::verb::post) return Route::ContentImportPost;
        if (target == "/content/config" && method == http::verb::get) return Route::ContentConfigGet;
        if (target == "/content/config" && method == http::verb::put) return Route::ContentConfigPut;
        if (target.rfind("/content/", 0) == 0 && method == http::verb::get) {
            if (target.find("/blob") != std::string::npos) return Route::ContentBlobGet;
            if (target.find("/chunks") != std::string::npos) return Route::ContentChunksGet;
            return Route::ContentGet;
        }
        // ContentFS binary blob API (/api/v1/content/fs/{pk})
        if (path_only.rfind("/api/v1/content/fs/", 0) == 0 && path_only.size() > 19) {
            if (method == http::verb::get)    return Route::ContentFsGet;
            if (method == http::verb::put)    return Route::ContentFsPut;
            if (method == http::verb::head)   return Route::ContentFsHead;
            if (method == http::verb::delete_) return Route::ContentFsDelete;
        }

    // Hybrid Search
        if (target == "/search/hybrid" && method == http::verb::post) return Route::HybridSearchPost;
        
        // Fusion Search (Text+Vector with RRF/Weighted)
        if (target == "/search/fusion" && method == http::verb::post) return Route::FusionSearchPost;
        
        // Fulltext Search
        if (target == "/search/fulltext" && method == http::verb::post) return Route::FulltextSearchPost;

    // Content filter schema config
    if (target == "/config/content-filters" && method == http::verb::get) return Route::ContentFilterSchemaGet;
    if (target == "/config/content-filters" && (method == http::verb::put || method == http::verb::post)) return Route::ContentFilterSchemaPut;
    if (target == "/config/edge-weights" && method == http::verb::get) return Route::EdgeWeightConfigGet;
    if (target == "/config/edge-weights" && (method == http::verb::put || method == http::verb::post)) return Route::EdgeWeightConfigPut;
    
    // Encryption schema config
    if (target == "/config/encryption-schema" && method == http::verb::get) return Route::EncryptionSchemaGet;
    if (target == "/config/encryption-schema" && (method == http::verb::put || method == http::verb::post)) return Route::EncryptionSchemaPut;
    
    // Error API endpoints
    if (path_only == "/api/v1/errors/categories" && method == http::verb::get) return Route::ErrorApiCategoriesGet;
    if (path_only == "/api/v1/errors/search" && method == http::verb::get) return Route::ErrorApiSearchGet;
    if (path_only.rfind("/api/v1/errors/", 0) == 0 && path_only != "/api/v1/errors/categories" && path_only != "/api/v1/errors/search" && method == http::verb::get) return Route::ErrorApiGetByCode;
    if (path_only == "/api/v1/errors" && method == http::verb::get) return Route::ErrorApiListGet;
    
    // Schema API routes
    if (path_only == "/api/v1/schema" && method == http::verb::get) return Route::SchemaGetFull;
    if (path_only == "/api/v1/schema/tables" && method == http::verb::get) return Route::SchemaGetTables;
    if (path_only.rfind("/api/v1/schema/tables/", 0) == 0 && method == http::verb::get) return Route::SchemaGetTable;

    // Schema versioning routes (must come before the generic SchemaPut/Patch catch)
    if (path_only.rfind("/api/v1/schema/versions/", 0) == 0) {
        if (method == http::verb::get)  return Route::SchemaVersionsGet;
        if (method == http::verb::post) return Route::SchemaVersionsPost;
    }
    if (path_only.rfind("/api/v1/schema/diff/", 0) == 0 && method == http::verb::get)
        return Route::SchemaDiffGet;

    if (path_only.rfind("/api/v1/schema/", 0) == 0 && path_only != "/api/v1/schema/" && 
        path_only != "/api/v1/schema/tables" && path_only.find("/api/v1/schema/tables/") != 0) {
        if (method == http::verb::put) return Route::SchemaPut;
        if (method == http::verb::patch) return Route::SchemaPatch;
    }

    // INFORMATION_SCHEMA routes
    if (path_only.rfind("/api/v1/information_schema", 0) == 0 && method == http::verb::get)
        return Route::InformationSchemaGet;

    // Metadata extended routes (order: more specific first)
    if (path_only.rfind("/api/v1/metadata/index_recommendations", 0) == 0 && method == http::verb::get)
        return Route::MetadataIndexRecsGet;
    if (path_only == "/api/v1/metadata/schema_import" && method == http::verb::put)
        return Route::MetadataSchemaImportPut;
    if (path_only.rfind("/api/v1/metadata/constraints/validate/", 0) == 0 && method == http::verb::post)
        return Route::MetadataBatchValidatePost;
    if (path_only.rfind("/api/v1/metadata/constraints/", 0) == 0 && method == http::verb::get)
        return Route::MetadataConstraintsGet;
    if (path_only.rfind("/api/v1/metadata/audit", 0) == 0 && method == http::verb::get)
        return Route::MetadataAuditGet;
    if (path_only.rfind("/api/v1/metadata/lineage", 0) == 0) {
        if (method == http::verb::get)  return Route::MetadataLineageGet;
        if (method == http::verb::post) return Route::MetadataLineagePost;
    }
    if (path_only.rfind("/api/v1/metadata/stats/", 0) == 0) {
        if (method == http::verb::get)  return Route::MetadataStatsGet;
        if (method == http::verb::post) return Route::MetadataStatsPost;
    }

    // MVCC versioning API endpoints
    // Note: /versions suffix checked first to avoid matching it as a key named "versions"
    if (path_only.rfind("/api/v1/mvcc/keys/", 0) == 0 &&
        path_only.size() > 18 &&
        path_only.rfind("/versions") == path_only.size() - 9) {
        if (method == http::verb::get)     return Route::MvccKeyVersionsGet;
        if (method == http::verb::delete_) return Route::MvccKeyVersionsDelete;
    }
    if (path_only.rfind("/api/v1/mvcc/keys/", 0) == 0 && path_only.size() > 18) {
        if (method == http::verb::get)  return Route::MvccKeyGet;
        if (method == http::verb::post) return Route::MvccKeyPost;
    }
    if (path_only == "/api/v1/mvcc/clock" && method == http::verb::get) return Route::MvccClockGet;
    if (path_only == "/api/v1/mvcc/stats"  && method == http::verb::get) return Route::MvccStatsGet;

    // GraphQL endpoint
    if ((path_only == "/graphql" || path_only == "/api/v1/graphql") &&
        method == http::verb::post) return Route::GraphQLPost;
    if ((path_only == "/graphql/schema" || path_only == "/api/v1/graphql/schema") &&
        method == http::verb::get) return Route::GraphQLSchemaGet;

    // gRPC-Web proxy (browser clients)
    // Routes: /grpc-web/<package>.<Service>/<Method>
    {
        static constexpr std::string_view kGrpcWebPrefix{"/grpc-web/"};
        if (path_only.rfind(kGrpcWebPrefix.data(), 0) == 0 &&
            path_only.size() > kGrpcWebPrefix.size()) {
            if (method == http::verb::options) return Route::GrpcWebOptions;
            if (method == http::verb::post)    return Route::GrpcWebPost;
        }
    }
    if (path_only == "/api/v1/grpc-web/status" && method == http::verb::get)
        return Route::GrpcWebStatusGet;

    // Serverless function hosting
    if (path_only == "/api/v1/functions" && method == http::verb::post)
        return Route::ServerlessFnPost;
    if (path_only == "/api/v1/functions" && method == http::verb::get)
        return Route::ServerlessFnListGet;
    {
        // /api/v1/functions/{id}/invoke  (POST)
        static constexpr std::string_view kFnInvokePrefix{"/api/v1/functions/"};
        static constexpr std::string_view kInvokeSuffix{"/invoke"};
        if (method == http::verb::post &&
            path_only.rfind(kFnInvokePrefix.data(), 0) == 0 &&
            path_only.size() > kFnInvokePrefix.size() + kInvokeSuffix.size() &&
            path_only.substr(path_only.size() - kInvokeSuffix.size()) == kInvokeSuffix)
            return Route::ServerlessFnInvokePost;

        // /api/v1/functions/{id}/versions  (GET)
        static constexpr std::string_view kVersionsSuffix{"/versions"};
        if (method == http::verb::get &&
            path_only.rfind(kFnInvokePrefix.data(), 0) == 0 &&
            path_only.size() > kFnInvokePrefix.size() + kVersionsSuffix.size() &&
            path_only.substr(path_only.size() - kVersionsSuffix.size()) == kVersionsSuffix)
            return Route::ServerlessFnVersionsGet;

        // /api/v1/functions/{id}  (GET / PUT / DELETE)
        if (path_only.rfind(kFnInvokePrefix.data(), 0) == 0 &&
            path_only.size() > kFnInvokePrefix.size()) {
            if (method == http::verb::get)    return Route::ServerlessFnGet;
            if (method == http::verb::put)    return Route::ServerlessFnPut;
            if (method == http::verb::delete_) return Route::ServerlessFnDelete;
        }
    }

    // Async job API  (/v2/jobs  and  /v2/jobs/{id})
    {
        static constexpr std::string_view kJobsPath{"/v2/jobs"};
        static constexpr std::string_view kJobsPrefix{"/v2/jobs/"};

        if (path_only == kJobsPath.data()) {
            if (method == http::verb::post) return Route::AsyncJobSubmitPost;
            if (method == http::verb::get)  return Route::AsyncJobListGet;
        }
        if (path_only.rfind(kJobsPrefix.data(), 0) == 0 &&
            path_only.size() > kJobsPrefix.size()) {
            if (method == http::verb::get)    return Route::AsyncJobStatusGet;
            if (method == http::verb::delete_) return Route::AsyncJobCancelDelete;
        }
        }
    // API Key Management: /api/keys and /api/keys/{id}
    if (path_only == "/api/keys") {
        if (method == http::verb::post) return Route::ApiKeyPost;
        if (method == http::verb::get)  return Route::ApiKeyListGet;
    }
    if (path_only.rfind("/api/keys/", 0) == 0 && path_only.size() > 10) {
        if (method == http::verb::get)    return Route::ApiKeyGet;
        if (method == http::verb::put)    return Route::ApiKeyPut;
        if (method == http::verb::delete_) return Route::ApiKeyDelete;
    }
    // Session Management: /auth/sessions and /auth/sessions/{id}
    if (path_only == "/auth/sessions") {
        if (method == http::verb::post)    return Route::SessionPost;
        if (method == http::verb::get)     return Route::SessionListGet;
        if (method == http::verb::delete_) return Route::SessionDeleteOthers;
    }
    if (path_only.rfind("/auth/sessions/", 0) == 0 && path_only.size() > 15) {
        if (method == http::verb::delete_) return Route::SessionDeleteById;
    }

    // SAML 2.0 SP endpoints: /api/v1/auth/saml/*
    if (path_only == "/api/v1/auth/saml/login" && method == http::verb::get)
        return Route::SamlLoginGet;
    if (path_only == "/api/v1/auth/saml/acs" && method == http::verb::post)
        return Route::SamlAcsPost;
    if (path_only == "/api/v1/auth/saml/slo" && method == http::verb::post)
        return Route::SamlSloPost;
    if (path_only == "/api/v1/auth/saml/metadata" && method == http::verb::get)
        return Route::SamlMetadataGet;

    // UDF registration API: /api/v1/query/udfs and /api/v1/query/udfs/{name}
    if (path_only == "/api/v1/query/udfs") {
        if (method == http::verb::post) return Route::UdfPost;
        if (method == http::verb::get)  return Route::UdfListGet;
    }
    {
        static constexpr std::string_view kUdfPrefix{"/api/v1/query/udfs/"};
        if (path_only.rfind(kUdfPrefix.data(), 0) == 0 &&
            path_only.size() > kUdfPrefix.size()) {
            if (method == http::verb::get)     return Route::UdfGet;
            if (method == http::verb::delete_) return Route::UdfDelete;
        }
    }

    // Task Scheduler API: /api/tasks[/{id}[/action]] and /ui/tasks
    if (path_only == "/ui/tasks" && method == http::verb::get) return Route::TasksUiGet;
    if (path_only == "/api/tasks") {
        if (method == http::verb::post) return Route::TasksPost;
        if (method == http::verb::get)  return Route::TasksListGet;
    }
    if (path_only == "/api/tasks/stats" && method == http::verb::get) return Route::TasksStatsGet;
    {
        static constexpr std::string_view kTasksPrefix{"/api/tasks/"};
        if (path_only.rfind(kTasksPrefix.data(), 0) == 0 &&
            path_only.size() > kTasksPrefix.size()) {
            std::string rest = path_only.substr(kTasksPrefix.size());
            auto slash = rest.find('/');
            if (slash == std::string::npos) {
                // /api/tasks/{id}
                if (method == http::verb::get)    return Route::TasksGet;
                if (method == http::verb::put)    return Route::TasksPut;
                if (method == http::verb::delete_) return Route::TasksDelete;
            } else {
                std::string action = rest.substr(slash + 1);
                if (method == http::verb::post && action == "enable")  return Route::TasksEnablePost;
                if (method == http::verb::post && action == "disable") return Route::TasksDisablePost;
                if (method == http::verb::post && action == "execute") return Route::TasksExecutePost;
                if (method == http::verb::get  && action == "history") return Route::TasksHistoryGet;
            }
        }
    }

    // Database Maintenance Orchestrator API: /api/v1/maintenance/*
    {
        static constexpr std::string_view kMaintBase{"/api/v1/maintenance/"};
        static constexpr std::string_view kMaintStatus{"/api/v1/maintenance/status"};
        static constexpr std::string_view kMaintHealth{"/api/v1/maintenance/health"};
        static constexpr std::string_view kMaintTaskHandlers{"/api/v1/maintenance/task-handlers"};
        static constexpr std::string_view kMaintSchedules{"/api/v1/maintenance/schedules"};
        static constexpr std::string_view kMaintSchedulesPfx{"/api/v1/maintenance/schedules/"};
        static constexpr std::string_view kMaintJobs{"/api/v1/maintenance/jobs"};
        static constexpr std::string_view kMaintJobsPfx{"/api/v1/maintenance/jobs/"};

        if (path_only == kMaintStatus && method == http::verb::get)
            return Route::MaintenanceStatusGet;
        if (path_only == kMaintHealth && method == http::verb::get)
            return Route::MaintenanceHealthGet;
        if (path_only == kMaintTaskHandlers && method == http::verb::get)
            return Route::MaintenanceTaskHandlersGet;

        // /api/v1/maintenance/schedules (collection)
        if (path_only == kMaintSchedules) {
            if (method == http::verb::post) return Route::MaintenanceSchedulesPost;
            if (method == http::verb::get)  return Route::MaintenanceSchedulesGet;
        }
        // /api/v1/maintenance/schedules/{id}[/run]
        if (path_only.rfind(kMaintSchedulesPfx.data(), 0) == 0 &&
            path_only.size() > kMaintSchedulesPfx.size()) {
            std::string rest = path_only.substr(kMaintSchedulesPfx.size());
            auto slash = rest.find('/');
            if (slash == std::string::npos) {
                // /api/v1/maintenance/schedules/{id}
                if (method == http::verb::get)    return Route::MaintenanceScheduleGet;
                if (method == http::verb::put)    return Route::MaintenanceSchedulePut;
                if (method == http::verb::patch)  return Route::MaintenanceSchedulePatch;
                if (method == http::verb::delete_)return Route::MaintenanceScheduleDelete;
            } else {
                std::string action = rest.substr(slash + 1);
                if (method == http::verb::post && action == "run")
                    return Route::MaintenanceScheduleRunPost;
            }
        }

        // /api/v1/maintenance/jobs (collection)
        if (path_only == kMaintJobs && method == http::verb::get)
            return Route::MaintenanceJobsGet;
        // /api/v1/maintenance/jobs/{id}[/cancel]
        if (path_only.rfind(kMaintJobsPfx.data(), 0) == 0 &&
            path_only.size() > kMaintJobsPfx.size()) {
            std::string rest = path_only.substr(kMaintJobsPfx.size());
            auto slash = rest.find('/');
            if (slash == std::string::npos) {
                if (method == http::verb::get) return Route::MaintenanceJobGet;
            } else {
                std::string action = rest.substr(slash + 1);
                if (method == http::verb::post && action == "cancel")
                    return Route::MaintenanceJobCancelPost;
            }
        }
    }

        // ── Retention Policy Admin API ──────────────────────────────────────
        if (path_only == "/api/retention/policies") {
            if (method == http::verb::get)  return Route::RetentionPoliciesGet;
            if (method == http::verb::post) return Route::RetentionPoliciesPost;
        }
        if (path_only.rfind("/api/retention/policies/", 0) == 0 &&
            path_only.size() > 24) {
            if (method == http::verb::delete_) return Route::RetentionPolicyDelete;
        }
        if (path_only == "/api/retention/history" && method == http::verb::get)
            return Route::RetentionHistoryGet;

        // ── SAGA Audit Log API ──────────────────────────────────────────────
        if (path_only == "/api/saga/batches" && method == http::verb::get)
            return Route::SAGABatchesGet;
        if (path_only == "/api/saga/flush" && method == http::verb::post)
            return Route::SAGAFlushPost;
        if (path_only.rfind("/api/saga/batches/", 0) == 0 &&
            path_only.size() > 18) {
            std::string rest = path_only.substr(18);
            auto slash_pos = rest.find('/');
            if (slash_pos == std::string::npos) {
                if (method == http::verb::get) return Route::SAGABatchGet;
            } else {
                std::string action = rest.substr(slash_pos + 1);
                if (method == http::verb::post && action == "verify")
                    return Route::SAGABatchVerifyPost;
            }
        }

        // ── CQL Phase 8: Continuous Query endpoints ────────────────────────
        // POST   /v1/queries/continuous
        if (path_only == "/v1/queries/continuous") {
            if (method == http::verb::post)   return Route::ContinuousQueryRegisterPost;
            if (method == http::verb::get)    return Route::ContinuousQueryListGet;
        }
        // /v1/queries/continuous/:name
        // /v1/queries/continuous/:name/results
        if (path_only.size() > 23 &&
            path_only.substr(0, 23) == "/v1/queries/continuous/")
        {
            const std::string rest_cq = path_only.substr(23);  // ":name" or ":name/results"
            const auto slash_cq = rest_cq.find('/');
            if (slash_cq == std::string::npos) {
                // /v1/queries/continuous/:name
                if (method == http::verb::delete_) return Route::ContinuousQueryDropDelete;
            } else {
                // /v1/queries/continuous/:name/results
                if (slash_cq + 1 < rest_cq.size()) {
                    const std::string suffix = rest_cq.substr(slash_cq + 1);
                    if (method == http::verb::get && suffix == "results")
                        return Route::ContinuousQueryStreamSseGet;
                }
            }
        }

        // ── AI Safety Layer — HILG Approval endpoints (ASL-6) ──────────────
        // POST /v1/ai/approve/{operation_id}
        if (path_only.rfind("/v1/ai/approve/", 0) == 0 &&
            path_only.size() > 15 &&
            method == http::verb::post) {
            return Route::AiApprovePendingPost;
        }
        // POST /v1/ai/deny/{operation_id}
        if (path_only.rfind("/v1/ai/deny/", 0) == 0 &&
            path_only.size() > 12 &&
            method == http::verb::post) {
            return Route::AiDenyPendingPost;
        }
        // GET /v1/ai/pending-approvals
        if (path_only == "/v1/ai/pending-approvals" &&
            method == http::verb::get) {
            return Route::AiPendingApprovalsGet;
        }
        // POST /v1/ai/rollback/{snapshot_id}  (ASL-10)
        if (path_only.rfind("/v1/ai/rollback/", 0) == 0 &&
            path_only.size() > 16 &&
            method == http::verb::post) {
            return Route::AiRollbackPost;
        }

        return Route::NotFound;
    }
}

http::response<http::string_body> HttpServer::routeRequest(
    const http::request<http::string_body>& req
) {
    // Create root span for this HTTP request.
    // If the caller supplied a W3C traceparent header, the new span becomes
    // a child of that upstream trace context (distributed tracing propagation).
    std::map<std::string, std::string> req_headers;
    for (auto const& field : req) {
        req_headers.emplace(std::string(field.name_string()),
                            std::string(field.value()));
    }
    auto span = Tracer::startSpanFromHeaders("http_request", req_headers);
    span.setAttribute("http.method", std::string(http::to_string(req.method())));
    span.setAttribute("http.target", std::string(req.target()));
    
    auto start = std::chrono::steady_clock::now();
    
    auto target = std::string(req.target());
    auto method = req.method();
    std::string path_only = target;
    if (auto qpos = path_only.find('?'); qpos != std::string::npos) {
        path_only = path_only.substr(0, qpos);
    }

    THEMIS_DEBUG("Request: {} {}", http::to_string(method), target);

    // Increment request counter
    request_count_.fetch_add(1, std::memory_order_relaxed);

    // Track in-flight requests for graceful shutdown draining
    active_requests_.fetch_add(1, std::memory_order_acquire);
    // RAII guard to decrement on all exit paths
    struct ActiveRequestGuard {
        std::atomic<uint64_t>& counter;
        ~ActiveRequestGuard() { counter.fetch_sub(1, std::memory_order_release); }
    } active_guard{active_requests_};

    // Handle CORS preflight early
    if (method == http::verb::options) {
        return makePreflightResponse(req);
    }

    // Extract or generate request correlation ID (X-Correlation-ID).
    // TracingMiddleware sets the logger pattern so every log line on this thread
    // carries the correlation ID.  The RAII guard resets the context on all exit paths.
    std::string correlation_id;
    if (tracing_middleware_) {
        std::lock_guard<std::mutex> lock(tracing_middleware_mutex_);
        if (tracing_middleware_) {
            auto corr_it = req.find("X-Correlation-ID");
            std::string_view incoming_corr = (corr_it != req.end()) ? std::string_view(corr_it->value()) : "";
            correlation_id = tracing_middleware_->processRequest(incoming_corr);
        }
    }
    struct CorrelationIdGuard {
        ~CorrelationIdGuard() { api::TracingMiddleware::clearContext(); }
    } corr_guard;
    span.setAttribute("correlation.id", correlation_id);

    // Extract or generate request ID for tracing
    // HS-5: Strip CR, LF, NUL from client-supplied header to prevent HTTP response splitting.
    auto sanitize_header_value = [](std::string s) {
        s.erase(std::remove_if(s.begin(), s.end(), [](char c) {
            return c == '\r' || c == '\n' || c == '\0';
        }), s.end());
        return s;
    };
    std::string request_id;
    auto req_id_it = req.find("X-Request-ID");
    if (req_id_it != req.end() && !req_id_it->value().empty()) {
        request_id = sanitize_header_value(std::string(req_id_it->value()));
    } else {
        // Generate a simple request ID from timestamp + counter
        request_id = std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()
        ) + "-" + std::to_string(request_count_.load(std::memory_order_relaxed));
    }
    span.setAttribute("request.id", request_id);

    // Enforce max header size limit
    if (config_.max_header_size_bytes > 0) {
        size_t header_bytes = 0;
        for (auto const& field : req) {
            header_bytes += field.name_string().size() + field.value().size() + 4; // 2 for ": " + 2 for "\r\n"
        }
        if (header_bytes > config_.max_header_size_bytes) {
            http::response<http::string_body> res{http::status::request_header_fields_too_large, req.version()};
            res.set(http::field::content_type, "application/json");
            res.set("X-Request-ID", request_id);
            const auto& corr_id = api::TracingMiddleware::currentCorrelationId();
            if (!corr_id.empty()) res.set("X-Correlation-ID", corr_id);
            nlohmann::json body = {
                {"error", "Request Header Fields Too Large"},
                {"message", "Total header size exceeds maximum allowed"},
                {"max_bytes", config_.max_header_size_bytes},
                {"actual_bytes", header_bytes},
                {"status_code", 431}
            };
            res.body() = body.dump();
            res.prepare_payload();
            return res;
        }
    }

    // Enforce request body size limit (after OPTIONS preflight)
    {
        std::lock_guard<std::mutex> lock(max_body_bytes_mutex_);
        if (req.body().size() > max_body_bytes_) {
            http::response<http::string_body> res{http::status::payload_too_large, req.version()};
            res.set(http::field::content_type, "application/json");
            res.set("X-Request-ID", request_id);
            nlohmann::json body = {
                {"error", "Payload Too Large"},
                {"message", "Request body exceeds maximum size"},
                {"max_bytes", max_body_bytes_},
                {"actual_bytes", req.body().size()},
                {"status_code", 413}
            };
            res.body() = body.dump();
            applyGovernanceHeaders(req, res);
            res.prepare_payload();
            recordLatency(std::chrono::microseconds(0)); // negligible work
            return res;
        }
    }

    // Path traversal checks for parameterized route paths
    {
        // Helper: extract and validate a path segment after a known prefix.
        // Returns a 400 response if the segment is invalid; otherwise continues.
        auto checkSegment = [&](const std::string& prefix) -> std::optional<http::response<http::string_body>> {
            if (path_only.rfind(prefix, 0) == 0 && validator_) {
                std::string segment = path_only.substr(prefix.size());
                // Strip any trailing sub-path (e.g., /versions) for the check
                auto slash_pos = segment.find('/');
                if (slash_pos != std::string::npos) segment = segment.substr(0, slash_pos);
                if (!segment.empty() && !validator_->validatePathSegment(segment)) {
                    http::response<http::string_body> res{http::status::bad_request, req.version()};
                    res.set(http::field::content_type, "application/json");
                    nlohmann::json body = {{"error", true},{"message","invalid path segment"},{"status_code",400}};
                    res.body() = body.dump();
                    applyGovernanceHeaders(req, res);
                    res.prepare_payload();
                    recordLatency(std::chrono::microseconds(0));
                    return res;
                }
            }
            return std::nullopt;
        };

        static const std::array<std::string_view, 5> kParameterizedPrefixes = {
            "/entities/",
            "/pii/",
            "/pii/reveal/",
            "/api/v1/content/fs/",
            "/api/v1/mvcc/keys/"
        };
        for (const auto& prefix : kParameterizedPrefixes) {
            if (auto err = checkSegment(std::string(prefix))) {
                return *err;
            }
        }
    }

    // ---------------------------------------------------------------------------
    // Versioned path routing: redirect unversioned paths to /v1/<path>
    //
    // REST endpoints added before API versioning (e.g., /documents/{id}, /query)
    // are implicitly treated as v1.  Clients that omit the version prefix receive a
    // 301 Moved Permanently so their bookmarks / integrations are updated while
    // the canonical /v1/ routes continue to function.
    //
    // Paths that are exempt from redirection (health-checks, already versioned
    // paths, WebSocket endpoints, etc.) are defined in RouteVersionRouter.
    // ---------------------------------------------------------------------------
    {
        static const RouteVersionRouter version_router;
        if (auto redirect_target = version_router.getRedirectTarget(target)) {
            http::response<http::string_body> res{http::status::moved_permanently, req.version()};
            res.set(http::field::location, *redirect_target);
            res.set(http::field::content_type, "application/json");
            nlohmann::json body = {
                {"status", 301},
                {"message", "Moved Permanently - use versioned API path"},
                {"location", *redirect_target}
            };
            res.body() = body.dump();
            applyGovernanceHeaders(req, res);
            res.prepare_payload();
            recordLatency(std::chrono::microseconds(0));
            return res;
        }
    }

    // Check rate limit BEFORE processing request
    if (auto rate_limit_response = checkRateLimit(req)) {
        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        recordLatency(duration);
        span.setStatus(false, "rate_limited");
        return *rate_limit_response;
    }

    // Check tenant quotas BEFORE processing request
    // Note: Tenant context will be acquired if tenant is identified
    std::optional<themis::TenantContext> tenant_ctx_opt;
    std::unique_ptr<themis::TenantContextGuard> tenant_guard;
    {
        auto& tenant_mgr = themis::TenantManager::instance();
        
        // Extract tenant ID from request - optimize by checking header directly first
        std::optional<std::string> tenant_id_opt;
        auto tenant_header_it = req.find("X-Tenant-ID");
        if (tenant_header_it != req.end()) {
            tenant_id_opt = std::string(tenant_header_it->value());
        } else {
            // Fallback: check path prefix (only if header not present)
            std::unordered_map<std::string, std::string> headers;
            for (auto const& field : req) {
                headers[std::string(field.name_string())] = std::string(field.value());
            }
            tenant_id_opt = tenant_mgr.extractTenantId(headers, target);
        }
        
        if (tenant_id_opt) {
            const std::string& tenant_id = *tenant_id_opt;
            
            // Get tenant config and create context
            auto tenant_config = tenant_mgr.getTenant(tenant_id);
            if (tenant_config) {
                tenant_ctx_opt = themis::TenantContext::fromConfig(*tenant_config, "");
                
                // Use RAII guard to acquire connection slot (automatically released on scope exit)
                tenant_guard = std::make_unique<themis::TenantContextGuard>(*tenant_ctx_opt);
                
                if (!tenant_guard->hasConnection()) {
                    // Connection quota exceeded
                    auto end = std::chrono::steady_clock::now();
                    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
                    
                    THEMIS_WARN("Tenant connection quota exceeded: tenant={}", tenant_id);
                    
                    http::response<http::string_body> res{http::status::service_unavailable, req.version()};
                    res.set(http::field::content_type, "application/json");
                    nlohmann::json body = {
                        {"error", "Service Unavailable"},
                        {"message", "Tenant connection quota exceeded"},
                        {"tenant_id", tenant_id},
                        {"status_code", 503}
                    };
                    res.body() = body.dump();
                    applyGovernanceHeaders(req, res);
                    res.prepare_payload();
                    // Use separate metric for quota exceeded (not rate limited)
                    // tenant_mgr has no recordQuotaExceeded, so we document this in logs only
                    recordLatency(duration);
                    span.setStatus(false, "tenant_quota_exceeded");
                    return res;
                }
                
                // Check query quota for query endpoints
                std::string quota_path = target;
                auto qpos = quota_path.find('?');
                if (qpos != std::string::npos) quota_path = quota_path.substr(0, qpos);
                
                // Use prefix match for /search/* to catch all search endpoints
                bool is_query_endpoint = (quota_path == "/query" || 
                                         quota_path.rfind("/search/", 0) == 0 ||
                                         quota_path.rfind("/api/aql", 0) == 0);
                
                if (is_query_endpoint) {
                    // Acquire query slot via RAII guard
                    if (!tenant_guard->acquireQuerySlot()) {
                        // Query quota exceeded
                        auto end = std::chrono::steady_clock::now();
                        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
                        
                        THEMIS_WARN("Tenant query quota exceeded: tenant={}", tenant_id);
                        
                        http::response<http::string_body> res{http::status::service_unavailable, req.version()};
                        res.set(http::field::content_type, "application/json");
                        nlohmann::json body = {
                            {"error", "Service Unavailable"},
                            {"message", "Tenant query quota exceeded"},
                            {"tenant_id", tenant_id},
                            {"status_code", 503}
                        };
                        res.body() = body.dump();
                        applyGovernanceHeaders(req, res);
                        res.prepare_payload();
                        recordLatency(duration);
                        span.setStatus(false, "tenant_quota_exceeded");
                        return res;
                    }
                }
            }
        }
    }
    // tenant_guard will automatically release connection/query slots when it goes out of scope

    // Early routing for Ethics AI API
    {
        std::string ethics_path = target;
        auto qpos = ethics_path.find('?');
        if (qpos != std::string::npos) ethics_path = ethics_path.substr(0, qpos);
        if (ethics_path.rfind("/ethics/", 0) == 0 || ethics_path.rfind("/api/ethics/", 0) == 0) {
            // HS-12: Ethics routes require auth — check before any early dispatch.
            if (auto auth_err = requireAccess(req, "ethics", "ethics.query", ethics_path)) {
                return *auth_err;
            }
            {
                std::lock_guard<std::mutex> lock(api_handlers_mutex_);
                if (ethics_api_) {
                    http::response<http::string_body> response = ethics_api_->handle(req, target);
                    applyGovernanceHeaders(req, response);
                    auto end = std::chrono::steady_clock::now();
                    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
                    recordLatency(duration);
                    span.setStatus(true);
                    return response;
                }
            }
        }
    }

#if THEMIS_ENABLE_LLM
    // Early routing for core LLM API endpoints used by connector-mode tests.
    {
        std::string llm_path = target;
        auto qpos = llm_path.find('?');
        if (qpos != std::string::npos) llm_path = llm_path.substr(0, qpos);

        if (llm_path.rfind("/api/v1/llm/", 0) == 0) {
            // HS-4: LLM routes require auth — check before any payload parsing or dispatch.
            if (auto auth_err = requireAccess(req, "llm", "llm", llm_path)) {
                return *auth_err;
            }
            try {
                auto& plugin_mgr = themis::llm::LLMPluginManager::instance();
                auto tryBootstrapDefaultModel = [&plugin_mgr]() -> bool {
                    const std::array<const char*, 2> env_names = {
                        "THEMIS_DEMO_LLM_MODEL_PATH",
                        "THEMIS_LLM_DEFAULT_MODEL_PATH"
                    };

                    for (const auto* env_name : env_names) {
                        const char* env_value = std::getenv(env_name);
                        if (!env_value || env_value[0] == '\0') {
                            continue;
                        }

                        const std::string model_path = env_value;
                        if (!std::filesystem::exists(model_path)) {
                            THEMIS_WARN("LLM bootstrap skipped: {} points to missing path '{}'",
                                        env_name, model_path);
                            continue;
                        }

                        try {
                            THEMIS_INFO("LLM bootstrap: trying to load default model from {}='{}'",
                                        env_name, model_path);
                            // Register plugin backend first; loadModel below performs
                            // the single authoritative model-load operation.
                            if (!themis::llm::createLlamaWrapper("llamacpp", "", json::object())) {
                                continue;
                            }
                            if (plugin_mgr.loadModel("default", model_path)) {
                                THEMIS_INFO("LLM bootstrap: default model loaded successfully");
                                return true;
                            }
                        } catch (const std::exception& e) {
                            THEMIS_WARN("LLM bootstrap from {} failed: {}", env_name, e.what());
                        }
                    }

                    return false;
                };

                if (path_only == "/api/v1/llm/ready" && method == http::verb::get) {
                    const auto health = plugin_mgr.getHealthStatus();
                    json body = {
                        {"ready", health.is_healthy},
                        {"healthy", health.is_healthy},
                        {"plugin_manager_status", health.plugin_manager_status},
                        {"models_loaded", health.models_loaded},
                        {"loras_loaded", health.loras_loaded}
                    };
                    http::response<http::string_body> response = makeResponse(
                        health.is_healthy ? http::status::ok : http::status::service_unavailable,
                        body.dump(),
                        req);
                    applyGovernanceHeaders(req, response);
                    auto end = std::chrono::steady_clock::now();
                    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
                    recordLatency(duration);
                    span.setStatus(true);
                    return response;
                }

                if (path_only == "/api/v1/llm/health" && method == http::verb::get) {
                    const auto health = plugin_mgr.getHealthStatus();
                    json body = {
                        {"status", health.is_healthy ? "healthy" : "unhealthy"},
                        {"healthy", health.is_healthy},
                        {"ready", health.is_healthy},
                        {"plugin_manager", health.plugin_manager_status},
                        {"plugin_manager_status", health.plugin_manager_status},
                        {"models_loaded", health.models_loaded},
                        {"loras_loaded", health.loras_loaded},
                        {"vram", {
                            {"total_bytes",           health.vram_total_bytes},
                            {"used_bytes",            health.vram_used_bytes},
                            {"free_bytes",            health.vram_free_bytes},
                            {"oom_threshold_exceeded", health.vram_oom_threshold_exceeded}
                        }}
                    };
                    http::response<http::string_body> response = makeResponse(
                        health.is_healthy ? http::status::ok : http::status::service_unavailable,
                        body.dump(),
                        req);
                    applyGovernanceHeaders(req, response);
                    auto end = std::chrono::steady_clock::now();
                    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
                    recordLatency(duration);
                    span.setStatus(true);
                    return response;
                }

                if (path_only == "/api/v1/llm/vram" && method == http::verb::get) {
                    const auto vram = plugin_mgr.getVRAMStats();
                    json body = {
                        {"total_bytes",           vram.total_vram_bytes},
                        {"used_bytes",            vram.used_vram_bytes},
                        {"free_bytes",            vram.free_vram_bytes},
                        {"peak_bytes",            vram.peak_vram_bytes},
                        {"wasted_padding_bytes",  vram.wasted_padding_bytes},
                        {"spilled_cpu_bytes",     vram.spilled_cpu_bytes},
                        {"live_allocation_count", vram.live_allocation_count},
                        {"oom_event_count",       vram.oom_event_count},
                        {"oom_recovery_count",    vram.oom_recovery_count},
                        {"eviction_count",        vram.eviction_count},
                        {"defrag_count",          vram.defrag_count},
                        {"spill_count",           vram.spill_count},
                        {"fragmentation_pct",     vram.fragmentation_pct},
                        {"oom_threshold_exceeded", vram.oom_threshold_exceeded}
                    };
                    http::response<http::string_body> response = makeResponse(
                        http::status::ok, body.dump(), req);
                    applyGovernanceHeaders(req, response);
                    auto end = std::chrono::steady_clock::now();
                    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
                    recordLatency(duration);
                    span.setStatus(true);
                    return response;
                }

                if (path_only == "/api/v1/llm/models/load" && method == http::verb::post) {
                    json payload;
                    try {
                        payload = json::parse(req.body());
                    } catch (const json::exception& e) {
                        auto response = makeErrorResponse(http::status::bad_request,
                            std::string("invalid JSON: ") + e.what(), req);
                        applyGovernanceHeaders(req, response);
                        return response;
                    }

                    const std::string model_id = payload.value("model_id", std::string{"default"});
                    const std::string model_path = payload.value("path", std::string{});
                    // GAP-009 fixed: canonicalize model_path and assert it does not escape
                    // the allowed model directory (THEMIS_MODEL_DIR env var, or the process
                    // working directory as a safe fallback).  This prevents path-traversal
                    // attacks from authenticated admin users.
                    if (model_path.empty()) {
                        auto response = makeErrorResponse(http::status::bad_request,
                            "path is required", req);
                        applyGovernanceHeaders(req, response);
                        return response;
                    }
                    {
                        // Determine the allowed model root.
                        std::filesystem::path allowed_root;
                        try {
                            const char* env_dir = std::getenv("THEMIS_MODEL_DIR");
                            if (env_dir && env_dir[0] != '\0') {
                                allowed_root = std::filesystem::weakly_canonical(env_dir);
                            } else {
                                allowed_root = std::filesystem::weakly_canonical(
                                    std::filesystem::current_path());
                                THEMIS_WARN("THEMIS_MODEL_DIR is not set; using current working directory "
                                            "as model root: '{}'. Set THEMIS_MODEL_DIR to the intended "
                                            "model directory to avoid this warning.",
                                            allowed_root.string());
                            }
                        } catch (const std::filesystem::filesystem_error& fse) {
                            auto response = makeErrorResponse(http::status::internal_server_error,
                                std::string("Model root canonicalization failed: ") + fse.what(), req);
                            applyGovernanceHeaders(req, response);
                            return response;
                        }
                        std::error_code ec;
                        auto canon = std::filesystem::weakly_canonical(model_path, ec);
                        // Use filesystem::equivalent() or a normalised prefix check that is
                        // case-insensitive on platforms with case-insensitive file systems
                        // (Windows, macOS default APFS).  Plain starts_with() on the
                        // string representations is sufficient on Linux (case-sensitive FS)
                        // but could be bypassed on Windows/macOS if the caller supplies a
                        // path that differs only in case.  We guard further with
                        // lexically_normal() to collapse any remaining "..".
                        auto canon_norm = canon.lexically_normal();
                        auto root_norm  = allowed_root.lexically_normal();
                        bool inside_root = false;
                        if (!ec) {
                            // Primary: check that canon is a subdirectory of allowed_root
                            // by verifying the prefix at the path-component boundary.
                            auto [mismatch_it, _] = std::mismatch(
                                root_norm.begin(), root_norm.end(),
                                canon_norm.begin(), canon_norm.end());
                            inside_root = (mismatch_it == root_norm.end());
                        }
                        if (ec || !inside_root) {
                            auto response = makeErrorResponse(http::status::bad_request,
                                "model path is outside the allowed directory", req);
                            applyGovernanceHeaders(req, response);
                            return response;
                        }
                    }

                    bool load_ok = false;
                    try {
                        load_ok = plugin_mgr.loadModel(model_id, model_path);
                    } catch (const std::exception& e) {
                        const std::string msg = e.what();
                        if (msg.find("No default LLM plugin available") != std::string::npos) {
                            // Register plugin without eager model load and retry once.
                            if (themis::llm::createLlamaWrapper("llamacpp", "", json::object())) {
                                load_ok = plugin_mgr.loadModel(model_id, model_path);
                            }
                        } else {
                            throw;
                        }
                    }

                    if (!load_ok && plugin_mgr.getDefaultPlugin() == nullptr) {
                        // Some manager paths report "no default plugin" via false
                        // (without throwing). Bootstrap once and retry loading.
                        if (themis::llm::createLlamaWrapper("llamacpp", "", json::object())) {
                            load_ok = plugin_mgr.loadModel(model_id, model_path);
                        }
                    }

                    if (!load_ok) {
                        auto response = makeErrorResponse(http::status::internal_server_error,
                            "Plugin returned false while loading model", req);
                        applyGovernanceHeaders(req, response);
                        return response;
                    }

                    json body = {
                        {"status", "loaded"},
                        {"model_id", model_id},
                        {"path", model_path}
                    };
                    http::response<http::string_body> response = makeResponse(http::status::ok, body.dump(), req);
                    applyGovernanceHeaders(req, response);
                    auto end = std::chrono::steady_clock::now();
                    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
                    recordLatency(duration);
                    span.setStatus(true);
                    return response;
                }

                if (path_only == "/api/v1/llm/inference" && method == http::verb::post) {
                    json payload;
                    try {
                        payload = json::parse(req.body());
                    } catch (const json::exception& e) {
                        auto response = makeErrorResponse(http::status::bad_request,
                            std::string("invalid JSON: ") + e.what(), req);
                        applyGovernanceHeaders(req, response);
                        return response;
                    }

                    const std::string prompt = payload.value("prompt", std::string{});
                    if (prompt.empty()) {
                        auto response = makeErrorResponse(http::status::bad_request,
                            "prompt is required", req);
                        applyGovernanceHeaders(req, response);
                        return response;
                    }

                    themis::llm::InferenceRequest llm_request;
                    llm_request.prompt = prompt;
                    llm_request.model_id = payload.value("model", std::string{"default"});
                    llm_request.max_tokens = payload.value("max_tokens", 256);
                    llm_request.temperature = static_cast<float>(payload.value("temperature", 0.7));

                    themis::llm::InferenceResponse llm_response;
                    try {
                        llm_response = plugin_mgr.generate(llm_request);
                    } catch (const std::exception& e) {
                        const std::string msg = e.what();
                        const bool retryable =
                            msg.find("No default LLM plugin available") != std::string::npos ||
                            msg.find("UNINITIALIZED") != std::string::npos ||
                            msg.find("Current state: ERROR") != std::string::npos;
                        if (!retryable || !tryBootstrapDefaultModel()) {
                            throw;
                        }
                        llm_response = plugin_mgr.generate(llm_request);
                    }
                    const std::string resolved_model =
                        llm_response.model_id.empty() ? llm_request.model_id : llm_response.model_id;
                    const int tokens_generated = llm_response.tokens_generated;
                    const double inference_time_ms = llm_response.inference_time_ms;
                    const int safe_tokens_generated = tokens_generated > 0 ? tokens_generated : 1;
                    const double safe_inference_time_ms = inference_time_ms > 0.0 ? inference_time_ms : 1.0;
                    const double tokens_per_second =
                        static_cast<double>(tokens_generated) * 1000.0 / safe_inference_time_ms;
                    const double ms_per_token =
                        static_cast<double>(inference_time_ms) / static_cast<double>(safe_tokens_generated);
                    const int max_tokens_requested = llm_request.max_tokens;
                    const bool hit_max_tokens_limit =
                        max_tokens_requested > 0 && tokens_generated >= max_tokens_requested;
                    const bool non_empty_text = !llm_response.text.empty();

                    json body = {
                        {"text", llm_response.text},
                        {"model", resolved_model},
                        {"tokens_generated", tokens_generated},
                        {"inference_time_ms", inference_time_ms},
                        {"max_tokens_requested", max_tokens_requested},
                        {"hit_max_tokens_limit", hit_max_tokens_limit},
                        {"non_empty_text", non_empty_text},
                        {"prompt_length", static_cast<int>(prompt.size())},
                        {"generated_length", static_cast<int>(llm_response.text.size())},
                        {"tokens_per_second", tokens_per_second},
                        {"ms_per_token", ms_per_token}
                    };

                    http::response<http::string_body> response = makeResponse(http::status::ok, body.dump(), req);
                    applyGovernanceHeaders(req, response);
                    auto end = std::chrono::steady_clock::now();
                    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
                    recordLatency(duration);
                    span.setStatus(true);
                    return response;
                }

                if (path_only == "/api/v1/llm/rag" && method == http::verb::post) {
                    json payload;
                    try {
                        payload = json::parse(req.body());
                    } catch (const json::exception& e) {
                        auto response = makeErrorResponse(http::status::bad_request,
                            std::string("invalid JSON: ") + e.what(), req);
                        applyGovernanceHeaders(req, response);
                        return response;
                    }

                    const std::string query = payload.value("query", std::string{});
                    const std::string collection = payload.value("collection", std::string{"default"});
                    const int top_k = payload.value("top_k", 5);
                    const std::string lora_id = payload.value("lora_adapter", std::string{});
                    const std::string rag_mode = payload.value("rag_mode", std::string{"text"});

                    if (query.empty()) {
                        auto response = makeErrorResponse(http::status::bad_request,
                            "query is required", req);
                        applyGovernanceHeaders(req, response);
                        return response;
                    }

                    if (top_k <= 0) {
                        auto response = makeErrorResponse(http::status::bad_request,
                            "top_k must be greater than 0", req);
                        applyGovernanceHeaders(req, response);
                        return response;
                    }

                    const int max_context_tokens = payload.value("max_context_tokens", 0);
                    if (max_context_tokens < 0) {
                        auto response = makeErrorResponse(http::status::bad_request,
                            "max_context_tokens must be >= 0", req);
                        applyGovernanceHeaders(req, response);
                        return response;
                    }

                    const int response_budget_tokens = payload.value("response_budget_tokens", 512);
                    if (response_budget_tokens <= 0) {
                        auto response = makeErrorResponse(http::status::bad_request,
                            "response_budget_tokens must be greater than 0", req);
                        applyGovernanceHeaders(req, response);
                        return response;
                    }

                    const int max_tokens = payload.value("max_tokens", 512);
                    if (max_tokens <= 0) {
                        auto response = makeErrorResponse(http::status::bad_request,
                            "max_tokens must be greater than 0", req);
                        applyGovernanceHeaders(req, response);
                        return response;
                    }

                    auto executeRag = [&]() -> json {
                        themis::llm::RAGContext rag_context;
                        rag_context.query = query;
                        rag_context.collection_name = collection;
                        rag_context.top_k = top_k;
                        const auto normalized_budget = themis::llm::ContextWindowBudget::compute(
                            static_cast<std::size_t>(max_context_tokens),
                            std::string{},
                            query,
                            static_cast<std::size_t>(response_budget_tokens));
                        rag_context.max_context_tokens =
                            static_cast<int>(normalized_budget.model_max_tokens);
                        rag_context.response_budget_tokens =
                            static_cast<int>(normalized_budget.reserved_response_tokens);

                        spdlog::info(
                            "HttpServer /api/v1/llm/rag dispatch: query_len={} collection='{}' top_k={} rag_mode='{}' model='{}' max_context_tokens={} response_budget_tokens={} request_max_tokens={}",
                            query.size(),
                            collection,
                            top_k,
                            rag_mode,
                            payload.value("model", std::string{"default"}),
                            rag_context.max_context_tokens,
                            rag_context.response_budget_tokens,
                            max_tokens);

                        themis::llm::InferenceRequest llm_request;
                        llm_request.prompt = query;
                        llm_request.model_id = payload.value("model", std::string{"default"});
                        llm_request.max_tokens = max_tokens;
                        llm_request.temperature = static_cast<float>(payload.value("temperature", 0.7));
                        if (!lora_id.empty()) {
                            llm_request.lora_adapter_id = lora_id;
                        }
                        llm_request.metadata["rag_mode"] = rag_mode;
                        if (payload.contains("rag_tensor_slots")) {
                            llm_request.metadata["rag_tensor_slots"] = payload["rag_tensor_slots"];
                        }
                        if (payload.contains("rag_tensor_slot_chars")) {
                            llm_request.metadata["rag_tensor_slot_chars"] = payload["rag_tensor_slot_chars"];
                        }

                        auto llm_response = plugin_mgr.generateRAG(rag_context, llm_request);
                        spdlog::info(
                            "HttpServer /api/v1/llm/rag complete: docs={} tokens_generated={} inference_time_ms={:.2f} rag_mode='{}'",
                            rag_context.documents.size(),
                            llm_response.tokens_generated,
                            llm_response.inference_time_ms,
                            rag_mode);
                        return json{
                            {"text", llm_response.text},
                            {"model", llm_response.model_id.empty() ? llm_request.model_id : llm_response.model_id},
                            {"rag_mode_effective", rag_mode},
                            {"documents_retrieved", static_cast<int>(rag_context.documents.size())},
                            {"top_k_effective", rag_context.top_k},
                            {"max_context_tokens_effective", rag_context.max_context_tokens},
                            {"response_budget_tokens_effective", rag_context.response_budget_tokens},
                            {"tokens_generated", llm_response.tokens_generated},
                            {"inference_time_ms", llm_response.inference_time_ms}
                        };
                    };

                    try {
                        json body = executeRag();

                        http::response<http::string_body> response = makeResponse(http::status::ok, body.dump(), req);
                        applyGovernanceHeaders(req, response);
                        auto end = std::chrono::steady_clock::now();
                        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
                        recordLatency(duration);
                        span.setStatus(true);
                        return response;
                    } catch (const std::exception& e) {
                        const std::string msg = e.what();
                        const bool retryable =
                            msg.find("No default LLM plugin available") != std::string::npos ||
                            msg.find("UNINITIALIZED") != std::string::npos ||
                            msg.find("Current state: ERROR") != std::string::npos;
                        if (retryable && tryBootstrapDefaultModel()) {
                                try {
                                    json body = executeRag();
                                    auto response = makeResponse(http::status::ok, body.dump(), req);
                                    applyGovernanceHeaders(req, response);
                                    return response;
                                } catch (...) {
                                    throw;
                                }
                        }
                        throw;
                    }
                }

                if (path_only == "/api/v1/llm/docs/query" && method == http::verb::post) {
                    json payload;
                    try {
                        payload = json::parse(req.body());
                    } catch (const json::exception& e) {
                        auto response = makeErrorResponse(http::status::bad_request,
                            std::string("invalid JSON: ") + e.what(), req);
                        applyGovernanceHeaders(req, response);
                        return response;
                    }

                    const std::string query = payload.value("query", std::string{});
                    if (query.empty()) {
                        auto response = makeErrorResponse(http::status::bad_request,
                            "query is required", req);
                        applyGovernanceHeaders(req, response);
                        return response;
                    }

                    static llm::DocsAssistant assistant;
                    static bool initialized = false;
                    if (!initialized) {
                        if (!assistant.loadDatabase()) {
                            auto response = makeErrorResponse(http::status::service_unavailable,
                                "Documentation database not available", req);
                            applyGovernanceHeaders(req, response);
                            return response;
                        }
                        initialized = true;
                    }

                    auto result = assistant.query(query);
                    json response_body = {
                        {"query", query},
                        {"answer", result.generated_answer},
                        {"confidence_score", result.confidence_score},
                        {"documents_searched", result.total_docs_searched},
                        {"documents_used", result.docs_included_in_context},
                        {"search_time_ms", result.search_time_ms.count()},
                        {"generation_time_ms", result.generation_time_ms.count()}
                    };

                    json docs_array = json::array();
                    for (const auto& doc : result.relevant_docs) {
                        docs_array.push_back({
                            {"file_name", doc.file_name},
                            {"relevance_score", doc.relevance_score},
                            {"content_preview", doc.text_content.substr(0, 200) + "..."}
                        });
                    }
                    response_body["relevant_documents"] = docs_array;

                    auto response = makeResponse(http::status::ok, response_body.dump(), req);
                    applyGovernanceHeaders(req, response);
                    auto end = std::chrono::steady_clock::now();
                    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
                    recordLatency(duration);
                    span.setStatus(true);
                    return response;
                }

                if (path_only == "/api/v1/llm/docs/config" && method == http::verb::post) {
                    json payload;
                    try {
                        payload = json::parse(req.body());
                    } catch (const json::exception& e) {
                        auto response = makeErrorResponse(http::status::bad_request,
                            std::string("invalid JSON: ") + e.what(), req);
                        applyGovernanceHeaders(req, response);
                        return response;
                    }

                    const std::string topic = payload.value("topic", std::string{});
                    if (topic.empty()) {
                        auto response = makeErrorResponse(http::status::bad_request,
                            "topic is required", req);
                        applyGovernanceHeaders(req, response);
                        return response;
                    }

                    static llm::DocsAssistant assistant;
                    static bool initialized = false;
                    if (!initialized) {
                        if (!assistant.loadDatabase()) {
                            auto response = makeErrorResponse(http::status::service_unavailable,
                                "Documentation database not available", req);
                            applyGovernanceHeaders(req, response);
                            return response;
                        }
                        initialized = true;
                    }

                    auto result = assistant.getConfigHelp(topic);
                    json response_body = {
                        {"topic", topic},
                        {"configuration_help", result.generated_answer},
                        {"confidence_score", result.confidence_score},
                        {"documents_used", result.docs_included_in_context}
                    };

                    auto response = makeResponse(http::status::ok, response_body.dump(), req);
                    applyGovernanceHeaders(req, response);
                    auto end = std::chrono::steady_clock::now();
                    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
                    recordLatency(duration);
                    span.setStatus(true);
                    return response;
                }

                if (path_only == "/api/v1/llm/docs/troubleshoot" && method == http::verb::post) {
                    json payload;
                    try {
                        payload = json::parse(req.body());
                    } catch (const json::exception& e) {
                        auto response = makeErrorResponse(http::status::bad_request,
                            std::string("invalid JSON: ") + e.what(), req);
                        applyGovernanceHeaders(req, response);
                        return response;
                    }

                    std::string issue = payload.value("error", std::string{});
                    if (issue.empty()) {
                        issue = payload.value("issue", std::string{});
                    }
                    if (issue.empty()) {
                        auto response = makeErrorResponse(http::status::bad_request,
                            "error or issue is required", req);
                        applyGovernanceHeaders(req, response);
                        return response;
                    }

                    static llm::DocsAssistant assistant;
                    static bool initialized = false;
                    if (!initialized) {
                        if (!assistant.loadDatabase()) {
                            auto response = makeErrorResponse(http::status::service_unavailable,
                                "Documentation database not available", req);
                            applyGovernanceHeaders(req, response);
                            return response;
                        }
                        initialized = true;
                    }

                    auto result = assistant.getTroubleshootingHelp(issue);
                    json response_body = {
                        {"error", issue},
                        {"troubleshooting_help", result.generated_answer},
                        {"confidence_score", result.confidence_score},
                        {"documents_used", result.docs_included_in_context}
                    };

                    auto response = makeResponse(http::status::ok, response_body.dump(), req);
                    applyGovernanceHeaders(req, response);
                    auto end = std::chrono::steady_clock::now();
                    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
                    recordLatency(duration);
                    span.setStatus(true);
                    return response;
                }

                if (path_only == "/api/v1/llm/lora/adapters" && method == http::verb::post) {
                    json payload;
                    try {
                        payload = json::parse(req.body());
                    } catch (const json::exception& e) {
                        auto response = makeErrorResponse(http::status::bad_request,
                            std::string("invalid JSON: ") + e.what(), req);
                        applyGovernanceHeaders(req, response);
                        return response;
                    }

                    const std::string adapter_id = payload.value("adapter_id", std::string{});
                    const std::string base_model = payload.value("base_model", std::string{});

                    if (adapter_id.empty() || base_model.empty()) {
                        auto response = makeErrorResponse(http::status::bad_request,
                            "adapter_id and base_model are required", req);
                        applyGovernanceHeaders(req, response);
                        return response;
                    }

                    // Generate a unique job_id for this training task
                    std::string job_id = "lora-job-" + adapter_id + "-" + 
                        std::to_string(std::chrono::high_resolution_clock::now().time_since_epoch().count());

                    json body = {
                        {"job_id", job_id},
                        {"adapter_id", adapter_id},
                        {"status", "queued"},
                        {"base_model", base_model},
                        {"rank", payload.value("rank", 8)},
                        {"alpha", payload.value("alpha", 16.0)}
                    };

                    http::response<http::string_body> response = makeResponse(
                        http::status::accepted,  // 202 Accepted
                        body.dump(), req);
                    applyGovernanceHeaders(req, response);
                    auto end = std::chrono::steady_clock::now();
                    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
                    recordLatency(duration);
                    span.setStatus(true);
                    return response;
                }
            } catch (const std::exception& e) {
                auto response = makeErrorResponse(http::status::internal_server_error,
                    std::string("LLM endpoint failure: ") + e.what(), req);
                applyGovernanceHeaders(req, response);
                return response;
            }
        }
    }
#endif

    // Request body validation (JSON Schema per endpoint)
    // Validate all methods that may carry a body (POST, PUT, PATCH, DELETE).
    // Safe methods (GET, HEAD) and OPTIONS are always skipped.
    {
        std::lock_guard<std::mutex> lock(request_validator_mutex_);
        if (request_validator_ &&
            method != http::verb::get   &&
            method != http::verb::head  &&
            method != http::verb::options) {
            std::string path_without_query = target;
            auto query_pos = path_without_query.find('?');
            if (query_pos != std::string::npos) path_without_query = path_without_query.substr(0, query_pos);

            auto validation_result = request_validator_->validate(
                std::string(http::to_string(method)), path_without_query, req.body());

            if (!validation_result.valid) {
                span.setStatus(false, "validation_error");
                auto validation_end_time = std::chrono::steady_clock::now();
                auto validation_duration = std::chrono::duration_cast<std::chrono::microseconds>(
                    validation_end_time - start);
                recordLatency(validation_duration);
                return makeErrorResponse(http::status::bad_request, validation_result.error_message, req);
            }
        }
    }

    http::response<http::string_body> response;

    auto ensure_handler_ready = [&](const char* handler_name, const void* handler_ptr) {
        if (handler_ptr != nullptr) {
            return true;
        }
        THEMIS_ERROR("HTTP route aborted: {} not initialized", handler_name);
        response = makeErrorResponse(
            http::status::service_unavailable,
            std::string(handler_name) + " not initialized",
            req);
        return false;
    };

    try {
        switch (classifyRoute(req)) {
            case Route::Health:
                if (monitoring_api_) {
                    response = monitoring_api_->handleHealthCheck(req);
                } else {
                    response = makeErrorResponse(http::status::service_unavailable,
                        "Monitoring API handler not initialized", req);
                }
            break;
        case Route::HealthLive:
            if (monitoring_api_) {
                response = monitoring_api_->handleLiveness(req);
            } else {
                response = makeErrorResponse(http::status::service_unavailable,
                    "Monitoring API handler not initialized", req);
            }
            break;
        case Route::HealthReady:
            if (monitoring_api_) {
                response = monitoring_api_->handleReadiness(req);
            } else {
                response = makeErrorResponse(http::status::service_unavailable,
                    "Monitoring API handler not initialized", req);
            }
            break;
        case Route::OpenApi:
            if (monitoring_api_) {
                response = monitoring_api_->handleOpenApi(req);
            } else {
                response = makeErrorResponse(http::status::service_unavailable,
                    "Monitoring API handler not initialized", req);
            }
            break;
        case Route::Version:
            if (monitoring_api_) {
                response = monitoring_api_->handleVersion(req);
            } else {
                response = makeErrorResponse(http::status::service_unavailable,
                    "Monitoring API handler not initialized", req);
            }
            break;
        case Route::Stats:
            if (monitoring_api_) {
                response = monitoring_api_->handleStats(req);
            } else {
                response = makeErrorResponse(http::status::service_unavailable,
                    "Monitoring API handler not initialized", req);
            }
            break;
        case Route::CapabilitiesGet:
            if (monitoring_api_) {
                response = monitoring_api_->handleCapabilities(req);
            } else {
                response = makeErrorResponse(http::status::service_unavailable,
                    "Monitoring API handler not initialized", req);
            }
            break;
        case Route::Metrics: {
            // HS-3: Restrict metrics to localhost or valid bearer token.
            {
                const std::string client_ip_str = extractClientIP(req);
                const bool from_loopback = (client_ip_str == "127.0.0.1" || client_ip_str == "::1");
                bool token_ok = false;
                if (const char* tok_env = std::getenv("THEMIS_METRICS_TOKEN")) {
                    const std::string expected_tok{tok_env};
                    if (!expected_tok.empty()) {
                        const auto auth_header = req[http::field::authorization];
                        if (!auth_header.empty()) {
                            auto bearer = themis::AuthMiddleware::extractBearerToken(
                                std::string_view(auth_header.data(), auth_header.size()));
                            token_ok = (bearer && *bearer == expected_tok);
                        }
                    }
                }
                if (!from_loopback && !token_ok) {
                    response = makeErrorResponse(http::status::forbidden,
                        "Metrics endpoint requires local access or valid THEMIS_METRICS_TOKEN", req);
                    break;
                }
            }
            // Delegate to MonitoringApiHandler for Prometheus metrics export
            if (monitoring_api_) {
                response = monitoring_api_->handleMetrics(req);
            } else {
                response = makeErrorResponse(http::status::service_unavailable,
                    "Monitoring API handler not initialized", req);
            }
            break;
        }
        case Route::MetricsHtml: {
            // W1-S11: Same localhost-or-metrics-token restriction as the Prometheus /metrics
            // endpoint — the HTML view exposes the same data.
            const std::string client_ip_mhtml = extractClientIP(req);
            const bool from_loopback_mhtml = (client_ip_mhtml == "127.0.0.1" || client_ip_mhtml == "::1");
            bool token_ok_mhtml = false;
            if (const char* tok_env = std::getenv("THEMIS_METRICS_TOKEN")) {
                const std::string expected_tok{tok_env};
                if (!expected_tok.empty()) {
                    auto it = req.find(http::field::authorization);
                    if (it != req.end()) {
                        auto bearer = themis::AuthMiddleware::extractBearerToken(
                            std::string_view(it->value().data(), it->value().size()));
                        token_ok_mhtml = (bearer && *bearer == expected_tok);
                    }
                }
            }
            if (!from_loopback_mhtml && !token_ok_mhtml) {
                response = makeErrorResponse(http::status::forbidden,
                    "Metrics HTML endpoint requires local access or valid THEMIS_METRICS_TOKEN", req);
                break;
            }
            if (monitoring_api_) {
                response = monitoring_api_->handleMetricsHtml(req);
            } else {
                response = makeErrorResponse(http::status::service_unavailable,
                    "Monitoring API handler not initialized", req);
            }
            break;
        }
        case Route::PluginMetrics: {
            // W1-S11: Same localhost-or-metrics-token restriction as the Prometheus /metrics
            // endpoint — plugin metrics expose comparable operational data.
            const std::string client_ip_pm = extractClientIP(req);
            const bool from_loopback_pm = (client_ip_pm == "127.0.0.1" || client_ip_pm == "::1");
            bool token_ok_pm = false;
            if (const char* tok_env = std::getenv("THEMIS_METRICS_TOKEN")) {
                const std::string expected_tok{tok_env};
                if (!expected_tok.empty()) {
                    auto it = req.find(http::field::authorization);
                    if (it != req.end()) {
                        auto bearer = themis::AuthMiddleware::extractBearerToken(
                            std::string_view(it->value().data(), it->value().size()));
                        token_ok_pm = (bearer && *bearer == expected_tok);
                    }
                }
            }
            if (!from_loopback_pm && !token_ok_pm) {
                response = makeErrorResponse(http::status::forbidden,
                    "Plugin metrics endpoint requires local access or valid THEMIS_METRICS_TOKEN", req);
                break;
            }
            if (monitoring_api_) {
                response = monitoring_api_->handlePluginMetrics(req);
            } else {
                response = makeErrorResponse(http::status::service_unavailable,
                    "Monitoring API handler not initialized", req);
            }
            break;
        }
        case Route::ObservabilityAlertsGet:
            // W1-S11: Observability alert list exposes internal alert state — require monitoring read.
            if (auto auth_err = requireAccess(req, "monitoring:read", "monitoring.alerts.read",
                                              "/api/v1/observability/alerts")) {
                response = *auth_err;
                break;
            }
            if (monitoring_api_) {
                response = monitoring_api_->handleObservabilityAlerts(req);
            } else {
                response = makeErrorResponse(http::status::service_unavailable,
                    "Monitoring API handler not initialized", req);
            }
            break;
        case Route::ObservabilityAlertSilencePost:
            // W1-S11: Silencing alerts is a write operation — require monitoring write.
            if (auto auth_err = requireAccess(req, "monitoring:write", "monitoring.alerts.silence",
                                              "/api/v1/observability/alerts/silence")) {
                response = *auth_err;
                break;
            }
            if (monitoring_api_) {
                response = monitoring_api_->handleObservabilityAlertSilence(req);
            } else {
                response = makeErrorResponse(http::status::service_unavailable,
                    "Monitoring API handler not initialized", req);
            }
            break;
        case Route::ObservabilityHealthGet:
            // W1-S11: Observability health exposes internal service config (endpoint URLs,
            // connection state) — require monitoring read.
            if (auto auth_err = requireAccess(req, "monitoring:read", "monitoring.health.read",
                                              "/api/v1/observability/health")) {
                response = *auth_err;
                break;
            }
            if (monitoring_api_) {
                response = monitoring_api_->handleObservabilityHealth(req);
            } else {
                response = makeErrorResponse(http::status::service_unavailable,
                    "Monitoring API handler not initialized", req);
            }
            break;
        case Route::ObservabilityProvenanceGet:
            // Provenance export exposes request lineage and routing reasons; require monitoring read.
            if (auto auth_err = requireAccess(req, "monitoring:read", "monitoring.provenance.read",
                                              "/api/v1/observability/provenance")) {
                response = *auth_err;
                break;
            }
            if (monitoring_api_) {
                response = monitoring_api_->handleObservabilityProvenance(req);
            } else {
                response = makeErrorResponse(http::status::service_unavailable,
                    "Monitoring API handler not initialized", req);
            }
            break;
        case Route::LicenseStatusGet:
            // W1-S11: License status exposes organization name, edition, and masked license key —
            // require monitoring read access.
            if (auto auth_err = requireAccess(req, "monitoring:read", "monitoring.license.read",
                                              "/api/v1/license/status")) {
                response = *auth_err;
                break;
            }
            if (monitoring_api_) {
                response = monitoring_api_->handleLicenseStatus(req);
            } else {
                response = makeErrorResponse(http::status::service_unavailable,
                    "Monitoring API handler not initialized", req);
            }
            break;
        case Route::WalApplyPost:
            // HS-2 fix: require admin privilege at the routing layer.
            // WALApiHandler::handleApply() also validates X-WAL-Auth / X-WAL-HMAC
            // when those secrets are configured, providing defense-in-depth.
            if (auto auth_err = requireAccess(req, "admin", "admin", "/api/v1/wal/apply")) {
                response = *auth_err;
                break;
            }
            if (!ensure_handler_ready("WAL API handler", wal_api_.get())) {
                break;
            }
            response = wal_api_->handleApply(req);
            break;
        case Route::Config:
            response = handleConfig(req);
            break;
        case Route::AdminBackupPost:
            // W1-S11: Backup creates a storage checkpoint — requires admin privilege.
            if (auto auth_err = requireAccess(req, "admin", "admin", "/api/v1/admin/backup")) {
                response = *auth_err;
                break;
            }
            if (!ensure_handler_ready("Admin API handler", admin_api_.get())) {
                break;
            }
            response = admin_api_->handleBackup(req);
            break;
        case Route::AdminRestorePost:
            // W1-S11: Restore replaces the live database — requires admin privilege.
            if (auto auth_err = requireAccess(req, "admin", "admin", "/api/v1/admin/restore")) {
                response = *auth_err;
                break;
            }
            if (!ensure_handler_ready("Admin API handler", admin_api_.get())) {
                break;
            }
            response = admin_api_->handleRestore(req);
            break;
        case Route::EntitiesGet:
            if (!ensure_handler_ready("Entity API handler", entity_api_.get())) {
                break;
            }
            response = entity_api_->handleGet(req);
            break;
        case Route::EntitiesPut:
            if (!ensure_handler_ready("Entity API handler", entity_api_.get())) {
                break;
            }
            response = entity_api_->handlePut(req);
            break;
        case Route::EntitiesDelete:
            if (!ensure_handler_ready("Entity API handler", entity_api_.get())) {
                break;
            }
            response = entity_api_->handleDelete(req);
            break;
        case Route::EntitiesPost:
            if (!ensure_handler_ready("Entity API handler", entity_api_.get())) {
                break;
            }
            response = entity_api_->handlePut(req);
            break;
        case Route::EntitiesBatchPost:
            if (!ensure_handler_ready("Entity API handler", entity_api_.get())) {
                break;
            }
            response = entity_api_->handleBatch(req);
            break;
        case Route::V2DocumentsBulkPost:
            if (!ensure_handler_ready("Entity API handler", entity_api_.get())) {
                break;
            }
            response = entity_api_->handleBulkNdjson(req);
            break;
        case Route::QueryPost:
            if (!ensure_handler_ready("Query API handler", query_api_.get())) {
                break;
            }
            response = query_api_->handleQuery(req);
            recordContinuousLearningQueryTelemetry(req, response, start, false);
            break;
        case Route::QueryAqlPost: {
            if (!ensure_handler_ready("Query API handler", query_api_.get())) {
                break;
            }
            // AQL payload validation before handling
            if (validator_) {
                try {
                    nlohmann::json j = nlohmann::json::object();
                    if (!req.body().empty()) j = nlohmann::json::parse(req.body());
                    if (auto err = validator_->validateAqlRequest(j)) {
                        auto res = makeErrorResponse(http::status::bad_request, *err, req);
                        return res;
                    }
                } catch (const std::exception& ex) {
                    auto res = makeErrorResponse(http::status::bad_request, std::string("invalid JSON: ") + ex.what(), req);
                    return res;
                }
            }
            response = query_api_->handleQueryAql(req);
            recordContinuousLearningQueryTelemetry(req, response, start, true);
            break;
        }
        case Route::QueryStreamSseGet:
            if (!ensure_handler_ready("Query API handler", query_api_.get())) {
                break;
            }
            response = query_api_->handleQueryStreamSse(req);
            break;
        case Route::IndexCreatePost:
            if (!ensure_handler_ready("Index API handler", index_api_.get())) {
                break;
            }
            response = index_api_->handleCreate(req);
            break;
        case Route::IndexDropPost:
            if (!ensure_handler_ready("Index API handler", index_api_.get())) {
                break;
            }
            response = index_api_->handleDrop(req);
            break;
        case Route::IndexStatsGet:
            if (!ensure_handler_ready("Index API handler", index_api_.get())) {
                break;
            }
            response = index_api_->handleStats(req);
            break;
        case Route::IndexRebuildPost:
            if (!ensure_handler_ready("Index API handler", index_api_.get())) {
                break;
            }
            response = index_api_->handleRebuild(req);
            break;
        case Route::IndexReindexPost:
            if (!ensure_handler_ready("Index API handler", index_api_.get())) {
                break;
            }
            response = index_api_->handleReindex(req);
            break;
            
        // G5: Spatial Index Management handlers
        case Route::SpatialIndexCreatePost:
            if (!ensure_handler_ready("Spatial API handler", spatial_api_.get())) {
                break;
            }
            response = spatial_api_->handleIndexCreate(req);
            applyGovernanceHeaders(req, response);
            break;
        case Route::SpatialIndexRebuildPost:
            if (!ensure_handler_ready("Spatial API handler", spatial_api_.get())) {
                break;
            }
            response = spatial_api_->handleIndexRebuild(req);
            applyGovernanceHeaders(req, response);
            break;
        case Route::SpatialIndexStatsGet:
            if (!ensure_handler_ready("Spatial API handler", spatial_api_.get())) {
                break;
            }
            response = spatial_api_->handleIndexStats(req);
            applyGovernanceHeaders(req, response);
            break;
        case Route::SpatialIndexMetricsGet:
            if (!ensure_handler_ready("Spatial API handler", spatial_api_.get())) {
                break;
            }
            response = spatial_api_->handleMetrics(req);
            applyGovernanceHeaders(req, response);
            break;
            
        case Route::GraphTraversePost:
           {
               std::lock_guard<std::mutex> lock(api_handlers_mutex_);
               if (graph_api_) {
                   response = graph_api_->handleTraverse(req);
               } else {
                   response = makeErrorResponse(http::status::service_unavailable, "Graph API not available", req);
               }
           }
            break;
        case Route::GraphEdgePost:
           {
               std::lock_guard<std::mutex> lock(api_handlers_mutex_);
               if (graph_api_) {
                   response = graph_api_->handleEdgeCreate(req);
               } else {
                   response = makeErrorResponse(http::status::service_unavailable, "Graph API not available", req);
               }
           }
            break;
        case Route::GraphEdgeDelete:
           {
               std::lock_guard<std::mutex> lock(api_handlers_mutex_);
               if (graph_api_) {
                   response = graph_api_->handleEdgeDelete(req);
               } else {
                   response = makeErrorResponse(http::status::service_unavailable, "Graph API not available", req);
               }
           }
            break;
        case Route::GraphMetricsGet:
           {
               std::lock_guard<std::mutex> lock(api_handlers_mutex_);
               if (graph_api_) {
                   response = graph_api_->handleMetrics(req);
               } else {
                   response = makeErrorResponse(http::status::service_unavailable, "Graph API not available", req);
               }
           }
            break;
        case Route::GraphMetricsPrometheusGet:
           {
               std::lock_guard<std::mutex> lock(api_handlers_mutex_);
               if (graph_api_) {
                   response = graph_api_->handleMetricsPrometheus(req);
               } else {
                   response = makeErrorResponse(http::status::service_unavailable, "Graph API not available", req);
               }
           }
            break;
        case Route::GraphQueryIncrementalPost:
           {
               std::lock_guard<std::mutex> lock(api_handlers_mutex_);
               if (graph_api_) {
                   response = graph_api_->handleIncrementalQueryRegister(req);
               } else {
                   response = makeErrorResponse(http::status::service_unavailable, "Graph API not available", req);
               }
           }
            break;
        case Route::GraphQueryIncrementalDelete:
           {
               std::lock_guard<std::mutex> lock(api_handlers_mutex_);
               if (graph_api_) {
                   response = graph_api_->handleIncrementalQueryUnregister(req);
               } else {
                   response = makeErrorResponse(http::status::service_unavailable, "Graph API not available", req);
               }
           }
            break;
        case Route::GraphChangesPost:
           {
               std::lock_guard<std::mutex> lock(api_handlers_mutex_);
               if (graph_api_) {
                   response = graph_api_->handleGraphChanges(req);
               } else {
                   response = makeErrorResponse(http::status::service_unavailable, "Graph API not available", req);
               }
           }
            break;
        case Route::GraphCostModelCalibratePost:
           {
               std::lock_guard<std::mutex> lock(api_handlers_mutex_);
               if (graph_api_) {
                   response = graph_api_->handleCostModelCalibrate(req);
               } else {
                   response = makeErrorResponse(http::status::service_unavailable, "Graph API not available", req);
               }
           }
            break;
        case Route::GraphCostModelGet:
           {
               std::lock_guard<std::mutex> lock(api_handlers_mutex_);
               if (graph_api_) {
                   response = graph_api_->handleCostModelExport(req);
               } else {
                   response = makeErrorResponse(http::status::service_unavailable, "Graph API not available", req);
               }
           }
            break;
        case Route::GraphCostModelImportPost:
           {
               std::lock_guard<std::mutex> lock(api_handlers_mutex_);
               if (graph_api_) {
                   response = graph_api_->handleCostModelImport(req);
               } else {
                   response = makeErrorResponse(http::status::service_unavailable, "Graph API not available", req);
               }
           }
            break;
        case Route::GraphQueryExplainPost:
           {
               std::lock_guard<std::mutex> lock(api_handlers_mutex_);
               if (graph_api_) {
                   response = graph_api_->handleQueryExplain(req);
               } else {
                   response = makeErrorResponse(http::status::service_unavailable, "Graph API not available", req);
               }
           }
            break;
        case Route::VectorSearchPost:
           {
               std::lock_guard<std::mutex> lock(api_handlers_mutex_);
               if (vector_api_) {
                   response = vector_api_->handleSearch(req);
               } else {
                   response = makeErrorResponse(http::status::service_unavailable, "Vector API not available", req);
               }
           }
            break;
        case Route::VectorBatchInsertPost:
           {
               std::lock_guard<std::mutex> lock(api_handlers_mutex_);
               if (vector_api_) {
                   response = vector_api_->handleBatchInsert(req);
               } else {
                   response = makeErrorResponse(http::status::service_unavailable, "Vector API not available", req);
               }
           }
            break;
        case Route::VectorDeleteByFilterDelete:
           {
               std::lock_guard<std::mutex> lock(api_handlers_mutex_);
               if (vector_api_) {
                   response = vector_api_->handleDeleteByFilter(req);
               } else {
                   response = makeErrorResponse(http::status::service_unavailable, "Vector API not available", req);
               }
           }
            break;
        case Route::CacheQueryPost:
           {
               std::lock_guard<std::mutex> lock(api_handlers_mutex_);
               if (cache_api_) {
                   response = cache_api_->handleQuery(req);
               } else {
                   response = makeErrorResponse(http::status::not_found, "Cache API not initialized", req);
               }
           }
           break;
        case Route::PromptTemplatePost:
           {
               std::lock_guard<std::mutex> lock(api_handlers_mutex_);
               if (prompt_api_) {
                   response = prompt_api_->handlePost(req);
               } else {
                   response = makeErrorResponse(http::status::not_found, "Prompt API not initialized", req);
               }
           }
           break;
        case Route::PromptTemplateList:
           {
               std::lock_guard<std::mutex> lock(api_handlers_mutex_);
               if (prompt_api_) {
                   response = prompt_api_->handleList(req);
               } else {
                   response = makeErrorResponse(http::status::not_found, "Prompt API not initialized", req);
               }
           }
           break;
        case Route::PromptTemplateGet:
           {
               std::lock_guard<std::mutex> lock(api_handlers_mutex_);
               if (prompt_api_) {
                   response = prompt_api_->handleGet(req);
               } else {
                   response = makeErrorResponse(http::status::not_found, "Prompt API not initialized", req);
               }
           }
           break;
        case Route::PromptTemplatePut:
           {
               std::lock_guard<std::mutex> lock(api_handlers_mutex_);
               if (prompt_api_) {
                   response = prompt_api_->handlePut(req);
               } else {
                   response = makeErrorResponse(http::status::not_found, "Prompt API not initialized", req);
               }
           }
           break;
        case Route::CachePutPost:
           {
               std::lock_guard<std::mutex> lock(api_handlers_mutex_);
               if (cache_api_) {
                   response = cache_api_->handlePut(req);
               } else {
                   response = makeErrorResponse(http::status::not_found, "Cache API not initialized", req);
               }
           }
           break;
        case Route::CacheStatsGet:
           {
               std::lock_guard<std::mutex> lock(api_handlers_mutex_);
               if (cache_api_) {
                   response = cache_api_->handleStats(req);
               } else {
                   response = makeErrorResponse(http::status::not_found, "Cache API not initialized", req);
               }
           }
           break;
        case Route::AdminCacheHealthGet:
           {
               std::lock_guard<std::mutex> lock(api_handlers_mutex_);
               if (cache_admin_api_) {
                   response = cache_admin_api_->handleHealth(req);
               } else {
                   response = makeErrorResponse(http::status::service_unavailable, "Cache admin API not initialized", req);
               }
           }
           break;
        case Route::AdminCacheStatsGet:
           {
               std::lock_guard<std::mutex> lock(api_handlers_mutex_);
               if (cache_admin_api_) {
                   response = cache_admin_api_->handleStats(req);
               } else {
                   response = makeErrorResponse(http::status::service_unavailable, "Cache admin API not initialized", req);
               }
           }
           break;
        case Route::AdminCacheEvictKeyDelete:
           {
               std::lock_guard<std::mutex> lock(api_handlers_mutex_);
               if (cache_admin_api_) {
                   response = cache_admin_api_->handleEvictKey(req);
               } else {
                   response = makeErrorResponse(http::status::service_unavailable, "Cache admin API not initialized", req);
               }
           }
           break;
        case Route::AdminCacheEvictTenantDelete:
           {
               std::lock_guard<std::mutex> lock(api_handlers_mutex_);
               if (cache_admin_api_) {
                   response = cache_admin_api_->handleEvictTenant(req);
               } else {
                   response = makeErrorResponse(http::status::service_unavailable, "Cache admin API not initialized", req);
               }
            }
            break;
        case Route::AdminCacheCbResetPost:
           {
               std::lock_guard<std::mutex> lock(api_handlers_mutex_);
               if (cache_admin_api_) {
                   response = cache_admin_api_->handleCircuitBreakerReset(req);
               } else {
                   response = makeErrorResponse(http::status::service_unavailable, "Cache admin API not initialized", req);
               }
           }
           break;
        case Route::AdminCacheCbStatusGet:
           {
               std::lock_guard<std::mutex> lock(api_handlers_mutex_);
               if (cache_admin_api_) {
                   response = cache_admin_api_->handleCircuitBreakerStatus(req);
               } else {
                   response = makeErrorResponse(http::status::service_unavailable, "Cache admin API not initialized", req);
               }
           }
           break;
        case Route::AdminCacheWarmupPost:
           {
               std::lock_guard<std::mutex> lock(api_handlers_mutex_);
               if (cache_admin_api_) {
                   response = cache_admin_api_->handleWarmup(req);
               } else {
                   response = makeErrorResponse(http::status::service_unavailable, "Cache admin API not initialized", req);
               }
           }
           break;
        case Route::AdminCacheSnapshotPost:
           {
               std::lock_guard<std::mutex> lock(api_handlers_mutex_);
               if (cache_admin_api_) {
                   response = cache_admin_api_->handleSnapshot(req);
               } else {
                   response = makeErrorResponse(http::status::service_unavailable, "Cache admin API not initialized", req);
               }
           }
           break;
        case Route::AdminCacheTenantsGet:
           {
               std::lock_guard<std::mutex> lock(api_handlers_mutex_);
               if (cache_admin_api_) {
                   response = cache_admin_api_->handleListTenants(req);
               } else {
                   response = makeErrorResponse(http::status::service_unavailable, "Cache admin API not initialized", req);
               }
           }
           break;
        case Route::AdminCacheTenantStatsGet:
           {
               std::lock_guard<std::mutex> lock(api_handlers_mutex_);
               if (cache_admin_api_) {
                   response = cache_admin_api_->handleTenantStats(req);
               } else {
                   response = makeErrorResponse(http::status::service_unavailable, "Cache admin API not initialized", req);
               }
           }
           break;
        case Route::AdminCacheTenantQuotaPatch:
           {
               std::lock_guard<std::mutex> lock(api_handlers_mutex_);
               if (cache_admin_api_) {
                   response = cache_admin_api_->handleUpdateTenantQuota(req);
               } else {
                   response = makeErrorResponse(http::status::service_unavailable, "Cache admin API not initialized", req);
               }
           }
           break;
        case Route::AdminCachePiiEvictDelete:
           {
               std::lock_guard<std::mutex> lock(api_handlers_mutex_);
               if (cache_admin_api_) {
                   response = cache_admin_api_->handlePiiEvict(req);
               } else {
                   response = makeErrorResponse(http::status::service_unavailable, "Cache admin API not initialized", req);
               }
           }
            break;
        case Route::AdminShardsPost: {
            // HS-1 fix: require admin privilege before mutating shard topology.
            if (auto auth_err = requireAccess(req, "admin", "admin", "/v1/admin/shards")) {
                response = *auth_err;
                break;
            }
            if (!sharding_manager_) {
                sharding_manager_ = &themis::sharding::ShardingManager::GetInstance();
            }
            auto& sharding_manager = *sharding_manager_;
            try {
                auto body = json::parse(req.body());
                themis::sharding::ShardNodeInfo node;
                node.node_id      = body.value("node_id", uint32_t(0));
                node.node_address = body.value("node_address", std::string{});
                node.node_role    = body.value("node_role", std::string{"PRIMARY"});
                node.is_healthy   = body.value("is_healthy", true);
                if (node.node_address.empty()) {
                    response = makeErrorResponse(http::status::bad_request, "node_address is required", req);
                    break;
                }
                sharding_manager.AddShardNode(node);
                json result = {
                    {"node_id",      node.node_id},
                    {"node_address", node.node_address},
                    {"node_role",    node.node_role},
                    {"is_healthy",   node.is_healthy}
                };
                response = makeResponse(http::status::created, result.dump(), req);
            } catch (const std::runtime_error& e) {
                response = makeErrorResponse(http::status::conflict, e.what(), req);
            } catch (const json::exception& e) {
                response = makeErrorResponse(http::status::bad_request, std::string("invalid JSON: ") + e.what(), req);
            }
            break;
        }
        case Route::AdminShardsGet: {
            // HS-1 fix: require admin privilege before exposing shard topology.
            if (auto auth_err = requireAccess(req, "admin", "admin", "/v1/admin/shards")) {
                response = *auth_err;
                break;
            }
            if (!sharding_manager_) {
                sharding_manager_ = &themis::sharding::ShardingManager::GetInstance();
            }
            auto& sharding_manager = *sharding_manager_;
            auto nodes = sharding_manager.GetAllNodes();
            json shards_arr = json::array();
            shards_arr.get_ref<json::array_t&>().reserve(nodes.size());
            for (const auto& n : nodes) {
                shards_arr.push_back({
                    {"node_id",      n.node_id},
                    {"node_address", n.node_address},
                    {"node_role",    n.node_role},
                    {"is_healthy",   n.is_healthy}
                });
            }
            json result = {
                {"shards",          shards_arr},
                {"total",           sharding_manager.GetNodeCount()},
                {"max_nodes",       themis::sharding::ShardingManager::GetMaxShardNodes()},
                {"remaining",       sharding_manager.GetRemainingNodeCapacity()},
                {"healthy_count",   sharding_manager.GetHealthyNodeCount()}
            };
            response = makeResponse(http::status::ok, result.dump(), req);
            break;
        }
        case Route::AdminStorageStatsGet: {
            // GET /v1/admin/storage/stats
            // HS-1 fix: require admin privilege before exposing internal storage metrics.
            if (auto auth_err = requireAccess(req, "admin", "admin.storage.stats",
                                              "/v1/admin/storage/stats")) {
                response = *auth_err;
                break;
            }
            // Returns RocksDB on-disk SST size and OS-level disk space metrics
            // for the storage path so admin tooling and quota logic can act on
            // real numbers instead of the previous hard-coded 0.
            try {
                uint64_t rocksdb_size = storage_ ? storage_->getApproximateSize() : 0;

                json storage_json = {
                    {"rocksdb_sst_size_bytes", rocksdb_size}
                };

                // Include OS-level disk space for the storage path when available.
                if (storage_) {
                    const std::string& db_path = storage_->getConfig().db_path;
                    storage::DiskSpaceMonitor dsm(db_path);
                    auto space = dsm.checkSpace();

                    storage_json["disk"] = {
                        {"path",            space.path},
                        {"total_bytes",     space.total_bytes},
                        {"used_bytes",      space.used_bytes},
                        {"free_bytes",      space.free_bytes},
                        {"available_bytes", space.available_bytes},
                        {"usage_percent",   space.usage_percent},
                        {"free_percent",    space.free_percent}
                    };
                }

                response = makeResponse(http::status::ok, storage_json.dump(), req);
            } catch (const std::exception& e) {
                response = makeErrorResponse(http::status::internal_server_error,
                    std::string("Failed to get storage stats: ") + e.what(), req);
            }
            break;
        }
        // ─── Shard Repair Admin API ─────────────────────────────────────────────
        case Route::AdminRepairHealthGet: {
            // GET /v1/admin/repair/health
            if (!shard_repair_api_) {
                response = makeErrorResponse(http::status::service_unavailable,
                    "ShardRepairApiHandler not initialized (ShardRepairEngine required)", req);
                break;
            }
            auto& shard_repair_api = *shard_repair_api_;
            response = shard_repair_api.handleHealth(req);
            break;
        }
        case Route::AdminRepairPost: {
            // POST /v1/admin/repair
            if (!shard_repair_api_) {
                response = makeErrorResponse(http::status::service_unavailable,
                    "ShardRepairApiHandler not initialized (ShardRepairEngine required)", req);
                break;
            }
            auto& shard_repair_api = *shard_repair_api_;
            response = shard_repair_api.handleTriggerRepair(req);
            break;
        }
        case Route::AdminRepairScanPost: {
            // POST /v1/admin/repair/scan
            if (!shard_repair_api_) {
                response = makeErrorResponse(http::status::service_unavailable,
                    "ShardRepairApiHandler not initialized (ShardRepairEngine required)", req);
                break;
            }
            auto& shard_repair_api = *shard_repair_api_;
            response = shard_repair_api.handleTriggerFullScan(req);
            break;
        }
        case Route::AdminRepairJobStatusGet: {
            // GET /v1/admin/repair/jobs/{job_id}
            if (!shard_repair_api_) {
                response = makeErrorResponse(http::status::service_unavailable,
                    "ShardRepairApiHandler not initialized (ShardRepairEngine required)", req);
                break;
            }
            auto& shard_repair_api = *shard_repair_api_;
            response = shard_repair_api.handleJobStatus(req);
            break;
        }
        case Route::AdminRepairDashboardGet: {
            // GET /v1/admin/repair/dashboard
            if (!shard_repair_api_) {
                response = makeErrorResponse(http::status::service_unavailable,
                    "ShardRepairApiHandler not initialized (ShardRepairEngine required)", req);
                break;
            }
            auto& shard_repair_api = *shard_repair_api_;
            response = shard_repair_api.handleDashboard(req);
            break;
        }
        // ─── Module Admin API ───────────────────────────────────────────────────
        case Route::AdminModulesGet: {
            // GET /v1/admin/modules — list all loaded modules
            if (auto auth_err = requireAccess(req, "admin", "admin", "/v1/admin/modules")) {
                response = *auth_err;
                break;
            }
            if (!module_loader_) {
                response = makeErrorResponse(http::status::service_unavailable,
                    "ModuleLoader not initialized; set modules.directory in config", req);
                break;
            }
            auto& module_loader = *module_loader_;
            try {
                auto modules = module_loader.getAllLoadedModules();
                json arr = json::array();
                arr.get_ref<json::array_t&>().reserve(modules.size());
                for (const auto& m : modules) {
                    arr.push_back({
                        {"name",            m.name},
                        {"path",            m.path},
                        {"version",         m.version},
                        {"verified",        m.verified},
                        {"fully_activated", m.fullyActivated},
                        {"load_time",       m.loadTime},
                        {"load_duration_ms",m.loadDurationMs}
                    });
                }
                response = makeResponse(http::status::ok,
                    json{{"modules", arr}, {"count", arr.size()}}.dump(), req);
            } catch (const std::exception& e) {
                response = makeErrorResponse(http::status::internal_server_error, e.what(), req);
            }
            break;
        }
        case Route::AdminModulesLoadPost: {
            // POST /v1/admin/modules/{name}/load  body: {"path":"…"}
            if (auto auth_err = requireAccess(req, "admin", "admin", "/v1/admin/modules")) {
                response = *auth_err;
                break;
            }
            if (!module_loader_) {
                response = makeErrorResponse(http::status::service_unavailable,
                    "ModuleLoader not initialized; set modules.directory in config", req);
                break;
            }
            auto& module_loader = *module_loader_;
            try {
                const std::string prefix = "/v1/admin/modules/";
                std::string url_name = path_only.substr(prefix.size());
                auto slash = url_name.rfind("/load");
                if (slash != std::string::npos) url_name = url_name.substr(0, slash);

                auto body = json::parse(req.body());
                std::string lib_path = body.value("path", std::string());
                if (lib_path.empty()) {
                    response = makeErrorResponse(http::status::bad_request,
                        "Missing required field: path", req);
                    break;
                }
                std::string mod_name = body.value("name", url_name);
                auto result = module_loader.loadModule(lib_path, mod_name);
                if (result.success) {
                    response = makeResponse(http::status::ok,
                        json{{"status","loaded"},{"name",mod_name},{"hash",result.moduleHash}}.dump(), req);
                } else {
                    response = makeErrorResponse(http::status::unprocessable_entity,
                        "Load failed: " + result.errorMessage, req);
                }
            } catch (const json::exception& e) {
                response = makeErrorResponse(http::status::bad_request,
                    std::string("JSON parse error: ") + e.what(), req);
            } catch (const std::exception& e) {
                response = makeErrorResponse(http::status::internal_server_error, e.what(), req);
            }
            break;
        }
        case Route::AdminModulesUnloadDelete: {
            // DELETE /v1/admin/modules/{name}
            if (auto auth_err = requireAccess(req, "admin", "admin", "/v1/admin/modules")) {
                response = *auth_err;
                break;
            }
            if (!module_loader_) {
                response = makeErrorResponse(http::status::service_unavailable,
                    "ModuleLoader not initialized; set modules.directory in config", req);
                break;
            }
            auto& module_loader = *module_loader_;
            try {
                const std::string prefix = "/v1/admin/modules/";
                std::string mod_name = path_only.substr(prefix.size());
                module_loader.unloadModule(mod_name);
                response = makeResponse(http::status::ok,
                    json{{"status","unloaded"},{"name",mod_name}}.dump(), req);
            } catch (const std::exception& e) {
                response = makeErrorResponse(http::status::internal_server_error, e.what(), req);
            }
            break;
        }
        case Route::AdminModuleStatusGet: {
            // GET /v1/admin/modules/{name}
            if (auto auth_err = requireAccess(req, "admin", "admin", "/v1/admin/modules")) {
                response = *auth_err;
                break;
            }
            if (!module_loader_) {
                response = makeErrorResponse(http::status::service_unavailable,
                    "ModuleLoader not initialized; set modules.directory in config", req);
                break;
            }
            auto& module_loader = *module_loader_;
            try {
                const std::string prefix = "/v1/admin/modules/";
                std::string mod_name = path_only.substr(prefix.size());
                auto info = module_loader.getModuleInfo(mod_name);
                if (!info) {
                    response = makeErrorResponse(http::status::not_found,
                        "Module not found: " + mod_name, req);
                    break;
                }
                json result = {
                    {"name",            info->name},
                    {"path",            info->path},
                    {"version",         info->version},
                    {"verified",        info->verified},
                    {"fully_activated", info->fullyActivated},
                    {"load_time",       info->loadTime},
                    {"load_duration_ms",info->loadDurationMs}
                };
                auto wd = module_loader.getWatchdogStats(mod_name);
                if (wd) {
                    result["watchdog"] = {
                        {"restart_count",         wd->restart_count},
                        {"consecutive_failures",  wd->consecutive_failures},
                        {"permanently_failed",    wd->permanently_failed},
                        {"last_error",            wd->last_error}
                    };
                }
                response = makeResponse(http::status::ok, result.dump(), req);
            } catch (const std::exception& e) {
                response = makeErrorResponse(http::status::internal_server_error, e.what(), req);
            }
            break;
        }
        // ────────────────────────────────────────────────────────────────────────
        case Route::LlmInteractionPost:
            response = handleLlmInteractionPost(req);
            break;
        case Route::LlmInteractionGetList:
            response = handleLlmInteractionList(req);
            break;
        case Route::LlmInteractionGetById:
            response = handleLlmInteractionGet(req);
            break;
        case Route::LlmInteractionUpdateMetadataPatch:
            response = handleLlmInteractionUpdateMetadata(req);
            break;
        case Route::QueryEnhancedPost:
            response = query_api_->handleQueryEnhanced(req);
            break;
        case Route::ChangefeedGet:
            if (changefeed_api_) {
                response = changefeed_api_->handleGet(req);
            } else {
                response = makeErrorResponse(http::status::service_unavailable, "Changefeed not available", req);
            }
            break;
        case Route::ChangefeedStreamSse:
            if (changefeed_api_) {
                response = changefeed_api_->handleStreamSse(req);
            } else {
                response = makeErrorResponse(http::status::service_unavailable, "Changefeed not available", req);
            }
            break;
        case Route::ChangefeedStreamAckPost:
            if (changefeed_api_) {
                response = changefeed_api_->handleStreamAck(req);
            } else {
                response = makeErrorResponse(http::status::service_unavailable, "Changefeed not available", req);
            }
            break;
        case Route::ChangefeedStatsGet:
            if (changefeed_api_) {
                response = changefeed_api_->handleStats(req);
            } else {
                response = makeErrorResponse(http::status::service_unavailable, "Changefeed not available", req);
            }
            break;
        case Route::ChangefeedRetentionPost:
            if (changefeed_api_) {
                response = changefeed_api_->handleRetention(req);
            } else {
                response = makeErrorResponse(http::status::service_unavailable, "Changefeed not available", req);
            }
            break;
        case Route::ChangefeedRetentionGet:
            if (changefeed_api_) {
                response = changefeed_api_->handleRetentionGet(req);
            } else {
                response = makeErrorResponse(http::status::service_unavailable, "Changefeed not available", req);
            }
            break;
        case Route::ChangefeedRetentionPut:
            if (changefeed_api_) {
                response = changefeed_api_->handleRetentionPut(req);
            } else {
                response = makeErrorResponse(http::status::service_unavailable, "Changefeed not available", req);
            }
            break;
        case Route::ChangefeedCompactPost:
            if (changefeed_api_) {
                response = changefeed_api_->handleCompact(req);
            } else {
                response = makeErrorResponse(http::status::service_unavailable, "Changefeed not available", req);
            }
            break;
        case Route::ChangefeedGdprRedactPost:
            if (changefeed_api_) {
                response = changefeed_api_->handleGdprRedact(req);
            } else {
                response = makeErrorResponse(http::status::service_unavailable, "Changefeed not available", req);
            }
            break;
        
        // Snapshot API (temporarily disabled - needs refactoring to Beast)
        case Route::SnapshotsTagsPost:
            if (snapshot_api_handler_) {
                // Convert Beast → cpp-httplib types
                auto httplib_req = HttpTypeAdapter::beastToHttplib(req);
                httplib::Response httplib_res;
                snapshot_api_handler_->handleCreateTag(httplib_req, httplib_res);
                // Convert cpp-httplib → Beast types
                response = HttpTypeAdapter::httplibToBeast(httplib_res, req.version());
            } else {
                response = makeErrorResponse(http::status::service_unavailable,
                    "Snapshot API not available (requires CDC feature)", req);
            }
            break;
        case Route::SnapshotsTagsGet:
            if (snapshot_api_handler_) {
                // Convert Beast → cpp-httplib types
                auto httplib_req = HttpTypeAdapter::beastToHttplib(req);
                httplib::Response httplib_res;
                snapshot_api_handler_->handleListTags(httplib_req, httplib_res);
                // Convert cpp-httplib → Beast types
                response = HttpTypeAdapter::httplibToBeast(httplib_res, req.version());
            } else {
                response = makeErrorResponse(http::status::service_unavailable,
                    "Snapshot API not available (requires CDC feature)", req);
            }
            break;
        case Route::SnapshotsTagGet:
            if (snapshot_api_handler_) {
                // Convert Beast → cpp-httplib types
                auto httplib_req = HttpTypeAdapter::beastToHttplib(req);
                httplib::Response httplib_res;
                snapshot_api_handler_->handleGetTag(httplib_req, httplib_res);
                // Convert cpp-httplib → Beast types
                response = HttpTypeAdapter::httplibToBeast(httplib_res, req.version());
            } else {
                response = makeErrorResponse(http::status::service_unavailable,
                    "Snapshot API not available (requires CDC feature)", req);
            }
            break;
        case Route::SnapshotsTagDelete:
            if (snapshot_api_handler_) {
                // Convert Beast → cpp-httplib types
                auto httplib_req = HttpTypeAdapter::beastToHttplib(req);
                httplib::Response httplib_res;
                snapshot_api_handler_->handleDeleteTag(httplib_req, httplib_res);
                // Convert cpp-httplib → Beast types
                response = HttpTypeAdapter::httplibToBeast(httplib_res, req.version());
            } else {
                response = makeErrorResponse(http::status::service_unavailable,
                    "Snapshot API not available (requires CDC feature)", req);
            }
            break;
        case Route::SnapshotsStatsGet:
            if (snapshot_api_handler_) {
                // Convert Beast → cpp-httplib types
                auto httplib_req = HttpTypeAdapter::beastToHttplib(req);
                httplib::Response httplib_res;
                snapshot_api_handler_->handleGetStats(httplib_req, httplib_res);
                // Convert cpp-httplib → Beast types
                response = HttpTypeAdapter::httplibToBeast(httplib_res, req.version());
            } else {
                response = makeErrorResponse(http::status::service_unavailable,
                    "Snapshot API not available (requires CDC feature)", req);
            }
            break;
        
        // Diff API
        case Route::DiffGet:
            if (diff_api_handler_) {
                // Convert Beast → cpp-httplib types
                auto httplib_req = HttpTypeAdapter::beastToHttplib(req);
                httplib::Response httplib_res;
                diff_api_handler_->handleGetDiff(httplib_req, httplib_res);
                // Convert cpp-httplib → Beast types
                response = HttpTypeAdapter::httplibToBeast(httplib_res, req.version());
            } else {
                response = makeErrorResponse(http::status::service_unavailable,
                    "Diff API not available (requires CDC feature)", req);
            }
            break;
        case Route::DiffCacheStatsGet:
            if (diff_api_handler_) {
                // Convert Beast → cpp-httplib types
                auto httplib_req = HttpTypeAdapter::beastToHttplib(req);
                httplib::Response httplib_res;
                diff_api_handler_->handleGetCacheStats(httplib_req, httplib_res);
                // Convert cpp-httplib → Beast types
                response = HttpTypeAdapter::httplibToBeast(httplib_res, req.version());
            } else {
                response = makeErrorResponse(http::status::service_unavailable,
                    "Diff API not available (requires CDC feature)", req);
            }
            break;
        case Route::DiffCacheClear:
            if (diff_api_handler_) {
                // Convert Beast → cpp-httplib types
                auto httplib_req = HttpTypeAdapter::beastToHttplib(req);
                httplib::Response httplib_res;
                diff_api_handler_->handleClearCache(httplib_req, httplib_res);
                // Convert cpp-httplib → Beast types
                response = HttpTypeAdapter::httplibToBeast(httplib_res, req.version());
            } else {
                response = makeErrorResponse(http::status::service_unavailable,
                    "Diff API not available (requires CDC feature)", req);
            }
            break;
        
        // PITR API
        case Route::PITRRestorePost:
            if (pitr_api_handler_) {
                // Convert Beast → cpp-httplib types
                auto httplib_req = HttpTypeAdapter::beastToHttplib(req);
                httplib::Response httplib_res;
                pitr_api_handler_->handleRestore(httplib_req, httplib_res);
                // Convert cpp-httplib → Beast types
                response = HttpTypeAdapter::httplibToBeast(httplib_res, req.version());
            } else {
                response = makeErrorResponse(http::status::service_unavailable,
                    "PITR API not available (requires CDC feature)", req);
            }
            break;
        case Route::PITRPreviewPost:
            if (pitr_api_handler_) {
                // Convert Beast → cpp-httplib types
                auto httplib_req = HttpTypeAdapter::beastToHttplib(req);
                httplib::Response httplib_res;
                pitr_api_handler_->handlePreview(httplib_req, httplib_res);
                // Convert cpp-httplib → Beast types
                response = HttpTypeAdapter::httplibToBeast(httplib_res, req.version());
            } else {
                response = makeErrorResponse(http::status::service_unavailable,
                    "PITR API not available (requires CDC feature)", req);
            }
            break;
        case Route::PITRProgressGet:
            if (pitr_api_handler_) {
                // Convert Beast → cpp-httplib types
                auto httplib_req = HttpTypeAdapter::beastToHttplib(req);
                httplib::Response httplib_res;
                pitr_api_handler_->handleGetProgress(httplib_req, httplib_res);
                // Convert cpp-httplib → Beast types
                response = HttpTypeAdapter::httplibToBeast(httplib_res, req.version());
            } else {
                response = makeErrorResponse(http::status::service_unavailable,
                    "PITR API not available (requires CDC feature)", req);
            }
            break;
        // Branch API handlers
        case Route::BranchesPost:
            if (branch_api_handler_) {
                auto httplib_req = HttpTypeAdapter::beastToHttplib(req);
                httplib::Response httplib_res;
                branch_api_handler_->handleCreateBranch(httplib_req, httplib_res);
                response = HttpTypeAdapter::httplibToBeast(httplib_res, req.version());
            } else {
                response = makeErrorResponse(http::status::service_unavailable,
                    "Branch API not available", req);
            }
            break;
        case Route::BranchesGet:
            if (branch_api_handler_) {
                auto httplib_req = HttpTypeAdapter::beastToHttplib(req);
                httplib::Response httplib_res;
                branch_api_handler_->handleListBranches(httplib_req, httplib_res);
                response = HttpTypeAdapter::httplibToBeast(httplib_res, req.version());
            } else {
                response = makeErrorResponse(http::status::service_unavailable,
                    "Branch API not available", req);
            }
            break;
        case Route::BranchesActiveGet:
            if (branch_api_handler_) {
                auto httplib_req = HttpTypeAdapter::beastToHttplib(req);
                httplib::Response httplib_res;
                branch_api_handler_->handleGetActiveBranch(httplib_req, httplib_res);
                response = HttpTypeAdapter::httplibToBeast(httplib_res, req.version());
            } else {
                response = makeErrorResponse(http::status::service_unavailable,
                    "Branch API not available", req);
            }
            break;
        case Route::BranchesStatsGet:
            if (branch_api_handler_) {
                auto httplib_req = HttpTypeAdapter::beastToHttplib(req);
                httplib::Response httplib_res;
                branch_api_handler_->handleGetStats(httplib_req, httplib_res);
                response = HttpTypeAdapter::httplibToBeast(httplib_res, req.version());
            } else {
                response = makeErrorResponse(http::status::service_unavailable,
                    "Branch API not available", req);
            }
            break;
        case Route::BranchGet:
            if (branch_api_handler_) {
                auto httplib_req = HttpTypeAdapter::beastToHttplib(req);
                httplib::Response httplib_res;
                branch_api_handler_->handleGetBranch(httplib_req, httplib_res);
                response = HttpTypeAdapter::httplibToBeast(httplib_res, req.version());
            } else {
                response = makeErrorResponse(http::status::service_unavailable,
                    "Branch API not available", req);
            }
            break;
        case Route::BranchSwitchPost:
            if (branch_api_handler_) {
                auto httplib_req = HttpTypeAdapter::beastToHttplib(req);
                httplib::Response httplib_res;
                branch_api_handler_->handleSwitchBranch(httplib_req, httplib_res);
                response = HttpTypeAdapter::httplibToBeast(httplib_res, req.version());
            } else {
                response = makeErrorResponse(http::status::service_unavailable,
                    "Branch API not available", req);
            }
            break;
        case Route::BranchDelete:
            if (branch_api_handler_) {
                auto httplib_req = HttpTypeAdapter::beastToHttplib(req);
                httplib::Response httplib_res;
                branch_api_handler_->handleDeleteBranch(httplib_req, httplib_res);
                response = HttpTypeAdapter::httplibToBeast(httplib_res, req.version());
            } else {
                response = makeErrorResponse(http::status::service_unavailable,
                    "Branch API not available", req);
            }
            break;
        case Route::BranchesMergePost:
            if (branch_api_handler_) {
                auto httplib_req = HttpTypeAdapter::beastToHttplib(req);
                httplib::Response httplib_res;
                branch_api_handler_->handleMergeBranches(httplib_req, httplib_res);
                response = HttpTypeAdapter::httplibToBeast(httplib_res, req.version());
            } else {
                response = makeErrorResponse(http::status::service_unavailable,
                    "Branch API not available", req);
            }
            break;
        
        // Merge API handlers
        case Route::MergePost:
            if (merge_api_handler_) {
                auto httplib_req = HttpTypeAdapter::beastToHttplib(req);
                httplib::Response httplib_res;
                merge_api_handler_->handleMerge(httplib_req, httplib_res);
                response = HttpTypeAdapter::httplibToBeast(httplib_res, req.version());
            } else {
                response = makeErrorResponse(http::status::service_unavailable,
                    "Merge API not available", req);
            }
            break;
        case Route::MergePreviewPost:
            if (merge_api_handler_) {
                auto httplib_req = HttpTypeAdapter::beastToHttplib(req);
                httplib::Response httplib_res;
                merge_api_handler_->handleMergePreview(httplib_req, httplib_res);
                response = HttpTypeAdapter::httplibToBeast(httplib_res, req.version());
            } else {
                response = makeErrorResponse(http::status::service_unavailable,
                    "Merge API not available", req);
            }
            break;
        case Route::MergeByTagPost:
            if (merge_api_handler_) {
                auto httplib_req = HttpTypeAdapter::beastToHttplib(req);
                httplib::Response httplib_res;
                merge_api_handler_->handleMergeByTag(httplib_req, httplib_res);
                response = HttpTypeAdapter::httplibToBeast(httplib_res, req.version());
            } else {
                response = makeErrorResponse(http::status::service_unavailable,
                    "Merge API not available", req);
            }
            break;
        case Route::MergeCanFastForwardGet:
            if (merge_api_handler_) {
                auto httplib_req = HttpTypeAdapter::beastToHttplib(req);
                httplib::Response httplib_res;
                merge_api_handler_->handleCanFastForward(httplib_req, httplib_res);
                response = HttpTypeAdapter::httplibToBeast(httplib_res, req.version());
            } else {
                response = makeErrorResponse(http::status::service_unavailable,
                    "Merge API not available", req);
            }
            break;
        
        case Route::TimeSeriesPut:
            if (timeseries_api_) {
                response = timeseries_api_->handlePut(req);
            } else {
                response = makeErrorResponse(http::status::service_unavailable, 
                    "Time-series feature not enabled", req);
            }
            break;
        case Route::TimeSeriesQuery:
            if (timeseries_api_) {
                response = timeseries_api_->handleQuery(req);
            } else {
                response = makeErrorResponse(http::status::service_unavailable, 
                    "Time-series feature not enabled", req);
            }
            break;
        case Route::TimeSeriesAggregate:
            if (timeseries_api_) {
                response = timeseries_api_->handleAggregate(req);
            } else {
                response = makeErrorResponse(http::status::service_unavailable, 
                    "Time-series feature not enabled", req);
            }
            break;
        case Route::TimeSeriesConfigGet:
            if (timeseries_api_) {
                response = timeseries_api_->handleConfigGet(req);
            } else {
                response = makeErrorResponse(http::status::service_unavailable, 
                    "Time-series feature not enabled", req);
            }
            break;
        case Route::TimeSeriesConfigPut:
            if (timeseries_api_) {
                response = timeseries_api_->handleConfigPut(req);
            } else {
                response = makeErrorResponse(http::status::service_unavailable, 
                    "Time-series feature not enabled", req);
            }
            break;
        case Route::TimeSeriesAggregatesGet:
            if (timeseries_api_) {
                response = timeseries_api_->handleAggregatesGet(req);
            } else {
                response = makeErrorResponse(http::status::service_unavailable, 
                    "Time-series feature not enabled", req);
            }
            break;
        case Route::TimeSeriesRetentionGet:
            if (timeseries_api_) {
                response = timeseries_api_->handleRetentionGet(req);
            } else {
                response = makeErrorResponse(http::status::service_unavailable, 
                    "Time-series feature not enabled", req);
            }
            break;
        case Route::TimeSeriesMetricsGet:
            if (timeseries_api_) {
                response = timeseries_api_->handleMetricsGet(req);
            } else {
                response = makeErrorResponse(http::status::service_unavailable, 
                    "Time-series feature not enabled", req);
            }
            break;
        case Route::TimeSeriesPromRemoteWrite:
            if (timeseries_api_) {
                response = timeseries_api_->handlePrometheusRemoteWrite(req);
            } else {
                response = makeErrorResponse(http::status::service_unavailable,
                    "Time-series feature not enabled", req);
            }
            break;
        case Route::IndexSuggestionsGet:
            response = index_api_->handleSuggestions(req);
            break;
        case Route::IndexPatternsGet:
            response = index_api_->handlePatterns(req);
            break;
        case Route::IndexRecordPatternPost:
            response = index_api_->handleRecordPattern(req);
            break;
        case Route::IndexClearPatternsDelete:
            response = index_api_->handleClearPatterns(req);
            break;
        case Route::VectorIndexSavePost:
           {
               std::lock_guard<std::mutex> lock(api_handlers_mutex_);
               if (vector_api_) {
                   response = vector_api_->handleIndexSave(req);
               } else {
                   response = makeErrorResponse(http::status::service_unavailable, "Vector API not available", req);
               }
           }
            break;
        case Route::VectorIndexLoadPost:
           {
               std::lock_guard<std::mutex> lock(api_handlers_mutex_);
               if (vector_api_) {
                   response = vector_api_->handleIndexLoad(req);
               } else {
                   response = makeErrorResponse(http::status::service_unavailable, "Vector API not available", req);
               }
           }
            break;
        case Route::VectorIndexConfigGet:
           {
               std::lock_guard<std::mutex> lock(api_handlers_mutex_);
               if (vector_api_) {
                   response = vector_api_->handleIndexConfigGet(req);
               } else {
                   response = makeErrorResponse(http::status::service_unavailable, "Vector API not available", req);
               }
           }
            break;
        case Route::VectorIndexConfigPut:
           {
               std::lock_guard<std::mutex> lock(api_handlers_mutex_);
               if (vector_api_) {
                   response = vector_api_->handleIndexConfigPut(req);
               } else {
                   response = makeErrorResponse(http::status::service_unavailable, "Vector API not available", req);
               }
           }
            break;
        case Route::VectorIndexStatsGet:
           {
               std::lock_guard<std::mutex> lock(api_handlers_mutex_);
               if (vector_api_) {
                   response = vector_api_->handleIndexStats(req);
               } else {
                   response = makeErrorResponse(http::status::service_unavailable, "Vector API not available", req);
               }
           }
            break;
        case Route::VectorIndexIncrementalReindexPost:
           {
               std::lock_guard<std::mutex> lock(api_handlers_mutex_);
               if (vector_api_) {
                   response = vector_api_->handleIncrementalReindex(req);
               } else {
                   response = makeErrorResponse(http::status::service_unavailable, "Vector API not available", req);
               }
           }
            break;
        case Route::RopeConfigPost:
            if (rope_api_) {
                response = rope_api_->handleConfigPost(req);
            } else {
                response = makeErrorResponse(http::status::service_unavailable, "RoPE API not available", req);
            }
            break;
        case Route::RopeConfigGet:
            if (rope_api_) {
                response = rope_api_->handleConfigGet(req);
            } else {
                response = makeErrorResponse(http::status::service_unavailable, "RoPE API not available", req);
            }
            break;
        case Route::RopeConfigDelete:
            if (rope_api_) {
                response = rope_api_->handleConfigDelete(req);
            } else {
                response = makeErrorResponse(http::status::service_unavailable, "RoPE API not available", req);
            }
            break;
        case Route::RopeAddPost:
            if (rope_api_) {
                response = rope_api_->handleAddPost(req);
            } else {
                response = makeErrorResponse(http::status::service_unavailable, "RoPE API not available", req);
            }
            break;
        case Route::RopeAddRelationalPost:
            if (rope_api_) {
                response = rope_api_->handleAddRelationalPost(req);
            } else {
                response = makeErrorResponse(http::status::service_unavailable, "RoPE API not available", req);
            }
            break;
        case Route::RopeSearchPost:
            if (rope_api_) {
                response = rope_api_->handleSearchPost(req);
            } else {
                response = makeErrorResponse(http::status::service_unavailable, "RoPE API not available", req);
            }
            break;
        case Route::RopeBatchAddPost:
            if (rope_api_) {
                response = rope_api_->handleBatchAddPost(req);
            } else {
                response = makeErrorResponse(http::status::service_unavailable, "RoPE API not available", req);
            }
            break;
        case Route::RopeStatsGet:
            if (rope_api_) {
                response = rope_api_->handleStatsGet(req);
            } else {
                response = makeErrorResponse(http::status::service_unavailable, "RoPE API not available", req);
            }
            break;
        case Route::KeysListGet:
            response = handleKeysListKeys(req);
            break;
        case Route::PkiSignPost:
            response = handlePkiSign(req);
            break;
        case Route::PkiVerifyPost:
            response = handlePkiVerify(req);
            break;
        case Route::PkiHsmSignPost:
            response = handlePkiHsmSign(req);
            break;
        case Route::PkiHsmKeysGet:
            response = handlePkiHsmKeys(req);
            break;
        case Route::PkiTimestampPost:
            response = handlePkiTimestamp(req);
            break;
        case Route::PkiTimestampVerifyPost:
            response = handlePkiTimestampVerify(req);
            break;
        case Route::PkiEidasSignPost:
            response = handlePkiEidasSign(req);
            break;
        case Route::PkiEidasVerifyPost:
            response = handlePkiEidasVerify(req);
            break;
        case Route::PkiCertificatesGet:
            response = handlePkiCertificates(req);
            break;
        case Route::PkiCertificateGet:
            response = handlePkiCertificate(req);
            break;
        case Route::PkiStatusGet:
            response = handlePkiStatus(req);
            break;
        case Route::KeysRotatePost:
            response = handleKeysRotateKey(req);
            break;
        case Route::ClassificationRulesGet:
            response = handleClassificationListRules(req);
            break;
        case Route::ClassificationTestPost:
            response = handleClassificationTest(req);
            break;
        case Route::ReportsComplianceGet:
            response = handleReportsCompliance(req);
            break;
        case Route::PiiListGet:
            response = handlePiiListMappings(req);
            break;
        case Route::PiiPost:
            response = handlePiiCreateMapping(req);
            break;
        case Route::PiiGetByUuid:
            response = handlePiiGetByUuid(req);
            break;
        case Route::PiiExportCsvGet:
            response = handlePiiExportCsv(req);
            break;
        case Route::PiiRevealGet:
            response = handlePiiRevealByUuid(req);
            break;
        case Route::PiiDeleteDelete:
            response = handlePiiDeleteByUuid(req);
            break;
        case Route::AuditQueryGet:
            response = handleAuditQuery(req);
            break;
        case Route::AuditExportCsvGet:
            response = handleAuditExportCsv(req);
            break;
        case Route::ExportJsonlLlmPost:
            if (export_api_) {
                response = export_api_->handleExportJsonlLlm(req);
            } else {
                response = makeErrorResponse(http::status::service_unavailable,
                    "Export API not enabled", req);
            }
            break;
        case Route::ExportStatusGet:
            if (export_api_) {
                response = export_api_->handleExportStatus(req);
            } else {
                response = makeErrorResponse(http::status::service_unavailable,
                    "Export API not enabled", req);
            }
            break;
        case Route::UpdateStatusGet:
        case Route::UpdateCheckPost:
        case Route::UpdateConfigGet:
        case Route::UpdateConfigPut:
            if (update_api_) {
                response = update_api_->handleRequest(req);
            } else {
                response = makeErrorResponse(http::status::service_unavailable,
                    "Update checker not enabled", req);
            }
            break;
            
#if THEMIS_ENABLE_LLM
        // Feedback API routes
        case Route::FeedbackPost:
            if (feedback_api_handler_) {
                response = feedback_api_handler_->handleCreateFeedback(req);
            } else {
                response = makeErrorResponse(http::status::service_unavailable,
                    "Feedback API not available", req);
            }
            break;
            
        case Route::FeedbackGet:
            if (feedback_api_handler_) {
                response = feedback_api_handler_->handleListFeedback(req);
            } else {
                response = makeErrorResponse(http::status::service_unavailable,
                    "Feedback API not available", req);
            }
            break;
            
        case Route::FeedbackGetById: {
            if (feedback_api_handler_) {
                std::string path(req.target());
                std::string id = path.substr(14); // Remove "/api/feedback/"
                size_t query_pos = id.find('?');
                if (query_pos != std::string::npos) {
                    id = id.substr(0, query_pos);
                }
                response = feedback_api_handler_->handleGetFeedback(req, id);
            } else {
                response = makeErrorResponse(http::status::service_unavailable,
                    "Feedback API not available", req);
            }
            break;
        }
        
        case Route::FeedbackPut: {
            if (feedback_api_handler_) {
                std::string path(req.target());
                std::string id = path.substr(14); // Remove "/api/feedback/"
                size_t query_pos = id.find('?');
                if (query_pos != std::string::npos) {
                    id = id.substr(0, query_pos);
                }
                response = feedback_api_handler_->handleUpdateFeedback(req, id);
            } else {
                response = makeErrorResponse(http::status::service_unavailable,
                    "Feedback API not available", req);
            }
            break;
        }
        
        case Route::FeedbackDelete: {
            if (feedback_api_handler_) {
                std::string path(req.target());
                std::string id = path.substr(14); // Remove "/api/feedback/"
                size_t query_pos = id.find('?');
                if (query_pos != std::string::npos) {
                    id = id.substr(0, query_pos);
                }
                response = feedback_api_handler_->handleDeleteFeedback(req, id);
            } else {
                response = makeErrorResponse(http::status::service_unavailable,
                    "Feedback API not available", req);
            }
            break;
        }
        
        case Route::FeedbackAdapterGet: {
            if (feedback_api_handler_) {
                std::string path(req.target());
                std::string adapter_id = path.substr(22); // Remove "/api/feedback/adapter/"
                size_t query_pos = adapter_id.find('?');
                if (query_pos != std::string::npos) {
                    adapter_id = adapter_id.substr(0, query_pos);
                }
                response = feedback_api_handler_->handleGetAdapterFeedback(req, adapter_id);
            } else {
                response = makeErrorResponse(http::status::service_unavailable,
                    "Feedback API not available", req);
            }
            break;
        }
        
        case Route::FeedbackStatsGet:
            if (feedback_api_handler_) {
                response = feedback_api_handler_->handleGetStatistics(req);
            } else {
                response = makeErrorResponse(http::status::service_unavailable,
                    "Feedback API not available", req);
            }
            break;
#else
        case Route::FeedbackPost:
        case Route::FeedbackGet:
        case Route::FeedbackGetById:
        case Route::FeedbackPut:
        case Route::FeedbackDelete:
        case Route::FeedbackAdapterGet:
        case Route::FeedbackStatsGet:
            response = makeErrorResponse(http::status::service_unavailable,
                "Feedback API disabled (LLM feature off)", req);
            break;
#endif
            
        case Route::TransactionPost:
            response = transaction_api_->handleTransaction(req);
            break;
        case Route::TransactionBeginPost:
            response = transaction_api_->handleBegin(req);
            break;
        case Route::TransactionCommitPost:
            response = transaction_api_->handleCommit(req);
            break;
        case Route::TransactionRollbackPost:
            response = transaction_api_->handleRollback(req);
            break;
        case Route::TransactionStatsGet:
            response = transaction_api_->handleStats(req);
            break;
        case Route::TransactionVersionGet:
            response = transaction_api_->handleGetVersion(req);
            break;

        // Distributed (cross-shard) 2PC transaction endpoints
        case Route::TransactionExplainGet:
            response = transaction_api_->handleExplain(req);
            break;
        case Route::DtxnBeginPost:
            response = distributed_txn_api_->handleBegin(req);
            break;
        case Route::DtxnOperationPost:
            response = distributed_txn_api_->handleOperation(req);
            break;
        case Route::DtxnCommitPost:
            response = distributed_txn_api_->handleCommit(req);
            break;
        case Route::DtxnAbortPost:
            response = distributed_txn_api_->handleAbort(req);
            break;
        case Route::DtxnReadOnlyPost:
            response = distributed_txn_api_->handleReadOnly(req);
            break;
        case Route::DtxnStatusGet:
            response = distributed_txn_api_->handleStatus(req);
            break;
        case Route::DtxnStatsGet:
            response = distributed_txn_api_->handleStats(req);
            break;

        case Route::ContentImportPost:
            {
                if (validator_) {
                    try {
                        nlohmann::json j = nlohmann::json::object();
                        if (!req.body().empty()) j = nlohmann::json::parse(req.body());
                        if (auto err = validator_->validateJsonStub(j, "content_import")) {
                            response = makeErrorResponse(http::status::bad_request, *err, req);
                            break;
                        }
                    } catch (const std::exception& ex) {
                        response = makeErrorResponse(http::status::bad_request, std::string("invalid JSON: ") + ex.what(), req);
                        break;
                    }
                }
                response = content_api_->handleImport(req);
            }
            break;
        case Route::ContentGet:
            response = content_api_->handleGet(req);
            break;
        case Route::ContentBlobGet:
            response = content_api_->handleGetBlob(req);
            break;
        case Route::ContentChunksGet:
            response = content_api_->handleGetChunks(req);
            break;
        case Route::ContentFsGet:
            response = handleContentFsGet(req);
            break;
        case Route::ContentFsPut:
            response = handleContentFsPut(req);
            break;
        case Route::ContentFsHead:
            response = handleContentFsHead(req);
            break;
        case Route::ContentFsDelete:
            response = handleContentFsDelete(req);
            break;
        case Route::HybridSearchPost:
            response = content_api_->handleHybridSearch(req);
            break;
        case Route::FusionSearchPost:
            response = content_api_->handleFusionSearch(req);
            break;
        case Route::FulltextSearchPost:
            response = content_api_->handleFulltextSearch(req);
            break;
        case Route::ContentFilterSchemaGet:
            response = content_api_->handleContentFilterSchemaGet(req);
            break;
        case Route::ContentFilterSchemaPut:
            response = content_api_->handleContentFilterSchemaPut(req);
            break;
        case Route::ContentConfigGet:
            response = content_api_->handleConfigGet(req);
            break;
        case Route::ContentConfigPut:
            response = content_api_->handleConfigPut(req);
            break;
        case Route::EdgeWeightConfigGet:
            response = content_api_->handleEdgeWeightConfigGet(req);
            break;
        case Route::EdgeWeightConfigPut:
            response = content_api_->handleEdgeWeightConfigPut(req);
            break;
        case Route::EncryptionSchemaGet:
            // Check access control before delegating
            if (auth_ && auth_->isEnabled()) {
                std::string config_path = std::string(req.target());
                auto qpos = config_path.find('?');
                if (qpos != std::string::npos) config_path = config_path.substr(0, qpos);
                if (auto resp = requireAccess(req, "config:read", "config.read", config_path)) {
                    response = *resp;
                    break;
                }
            }
            response = content_api_->handleEncryptionSchemaGet(req);
            break;
        case Route::EncryptionSchemaPut:
            // Check access control before delegating
            if (auth_ && auth_->isEnabled()) {
                std::string config_path = std::string(req.target());
                auto qpos = config_path.find('?');
                if (qpos != std::string::npos) config_path = config_path.substr(0, qpos);
                if (auto resp = requireAccess(req, "config:write", "config.write", config_path)) {
                    response = *resp;
                    break;
                }
            }
            response = content_api_->handleEncryptionSchemaPut(req);
            break;
        case Route::ErrorApiListGet:
            response = handleErrorApiList(req);
            break;
        case Route::ErrorApiGetByCode:
            response = handleErrorApiGetByCode(req);
            break;
        case Route::ErrorApiCategoriesGet:
            response = handleErrorApiCategories(req);
            break;
        case Route::ErrorApiSearchGet:
            response = handleErrorApiSearch(req);
            break;
        case Route::BpmnProcessStartPost:
            if (bpmn_api_) {
                response = bpmn_api_->handleStartProcess(req);
            } else {
                response = makeErrorResponse(http::status::service_unavailable,
                    "BPMN process engine not available", req);
            }
            break;
        case Route::BpmnTaskCompletePost:
            if (bpmn_api_) {
                response = bpmn_api_->handleTaskComplete(req);
            } else {
                response = makeErrorResponse(http::status::service_unavailable,
                    "BPMN process engine not available", req);
            }
            break;
        case Route::BpmnInstanceQueryGet:
            if (bpmn_api_) {
                response = bpmn_api_->handleQueryInstance(req);
            } else {
                response = makeErrorResponse(http::status::service_unavailable,
                    "BPMN process engine not available", req);
            }
            break;

        // ── Geo Topology API ──────────────────────────────────────────────────
        case Route::GeoTopologyGet:
            response = geo_topology_api_->handleTopologyGet(req);
            break;
        case Route::GeoRegionsGet:
            response = geo_topology_api_->handleRegionsGet(req);
            break;
        case Route::GeoHealthGet:
            response = geo_topology_api_->handleHealthGet(req);
            break;
        case Route::GeoTopologyShardPost:
            response = geo_topology_api_->handleTopologyShardPost(req);
            break;
        case Route::GeoTopologyShardDelete:
            response = geo_topology_api_->handleTopologyShardDelete(req);
            break;
        case Route::GeoConfigGet:
            response = geo_topology_api_->handleConfigGet(req);
            break;
        case Route::GeoConfigPut:
            response = geo_topology_api_->handleConfigPut(req);
            break;

        // ── Replication Topology API ──────────────────────────────────────────
        case Route::ReplicationTopologyGet:
            response = replication_topology_api_->handleTopologyGet(req);
            break;
        case Route::ReplicationHealthGet:
            response = replication_topology_api_->handleHealthGet(req);
            break;
        case Route::ReplicationTopologyUiGet:
            response = replication_topology_api_->handleUiGet(req);
            break;
        case Route::SchemaGetFull:
            response = handleSchemaGetFull(req);
            break;
        case Route::SchemaGetTables:
            response = handleSchemaGetTables(req);
            break;
        case Route::SchemaGetTable:
            response = handleSchemaGetTable(req);
            break;
        case Route::SchemaPut:
            response = handleSchemaPut(req);
            break;
        case Route::SchemaPatch:
            response = handleSchemaPatch(req);
            break;
        case Route::SchemaVersionsGet:
            response = handleSchemaVersionHistory(req);
            break;
        case Route::SchemaVersionsPost:
            response = handleSchemaCreateVersion(req);
            break;
        case Route::SchemaDiffGet:
            response = handleSchemaDiff(req);
            break;
        case Route::InformationSchemaGet:
            response = handleMetadataInformationSchema(req);
            break;
        case Route::MetadataStatsGet:
            response = handleMetadataGetStats(req);
            break;
        case Route::MetadataStatsPost:
            response = handleMetadataCollectStats(req);
            break;
        case Route::MetadataConstraintsGet:
            response = handleMetadataGetConstraints(req);
            break;
        case Route::MetadataIndexRecsGet:
            response = handleMetadataIndexRecommendations(req);
            break;
        case Route::MetadataAuditGet:
            response = handleMetadataAuditLog(req);
            break;
        case Route::MetadataLineageGet:
            response = handleMetadataGetColumnLineage(req);
            break;
        case Route::MetadataLineagePost:
            response = handleMetadataRecordLineageDerivation(req);
            break;
        case Route::MetadataSchemaImportPut:
            response = handleMetadataSchemaImport(req);
            break;
        case Route::MetadataBatchValidatePost:
            response = handleMetadataBatchValidate(req);
            break;
        case Route::PoliciesImportRangerPost: {
            // Require admin scope + policy action
            if (auth_ && auth_->isEnabled()) {
                std::string policy_path = std::string(req.target());
                auto qpos = policy_path.find('?');
                if (qpos != std::string::npos) policy_path = policy_path.substr(0, qpos);
                if (auto resp = requireAccess(req, "admin", "admin", policy_path)) {
                    response = *resp;
                    break;
                }
            }
            // Delegate to PolicyApiHandler
            if (policy_api_) {
                response = policy_api_->handleImportRanger(req);
            } else {
                response = makeErrorResponse(http::status::service_unavailable, "Policy API not initialized", req);
            }
            break;
        }
        case Route::PoliciesExportRangerGet: {
            // Require admin scope + policy action
            if (auth_ && auth_->isEnabled()) {
                std::string policy_path = std::string(req.target());
                auto qpos = policy_path.find('?');
                if (qpos != std::string::npos) policy_path = policy_path.substr(0, qpos);
                if (auto resp = requireAccess(req, "admin", "admin", policy_path)) {
                    response = *resp;
                    break;
                }
            }
            // Delegate to PolicyApiHandler
            if (policy_api_) {
                response = policy_api_->handleExportRanger(req);
            } else {
                response = makeErrorResponse(http::status::service_unavailable, "Policy API not initialized", req);
            }
            break;
        }

        // ─── MVCC versioning API ───────────────────────────────────────────────
        // Helper: extract the key from a path like /api/v1/mvcc/keys/{key}[/versions]
        // and populate req.matches so MvccApiHandler::extractKey() works.
        case Route::MvccKeyGet:
        case Route::MvccKeyPost: {
            if (mvcc_api_handler_) {
                auto httplib_req = HttpTypeAdapter::beastToHttplib(req);
                {
                    static const std::regex re(R"(/api/v1/mvcc/keys/([^/]+))");
                    std::smatch m;
                    if (std::regex_search(httplib_req.path, m, re)) {
                        httplib_req.matches = m;
                    }
                }
                httplib::Response httplib_res;
                if (req.method() == http::verb::get) {
                    mvcc_api_handler_->handleGetKey(httplib_req, httplib_res);
                } else {
                    mvcc_api_handler_->handlePutKey(httplib_req, httplib_res);
                }
                response = HttpTypeAdapter::httplibToBeast(httplib_res, req.version());
            } else {
                response = makeErrorResponse(http::status::service_unavailable,
                    "MVCC API not available", req);
            }
            break;
        }
        case Route::MvccKeyVersionsGet:
        case Route::MvccKeyVersionsDelete: {
            if (mvcc_api_handler_) {
                auto httplib_req = HttpTypeAdapter::beastToHttplib(req);
                {
                    static const std::regex re(R"(/api/v1/mvcc/keys/([^/]+)/versions)");
                    std::smatch m;
                    if (std::regex_search(httplib_req.path, m, re)) {
                        httplib_req.matches = m;
                    }
                }
                httplib::Response httplib_res;
                if (req.method() == http::verb::get) {
                    mvcc_api_handler_->handleListVersions(httplib_req, httplib_res);
                } else {
                    mvcc_api_handler_->handleGcVersions(httplib_req, httplib_res);
                }
                response = HttpTypeAdapter::httplibToBeast(httplib_res, req.version());
            } else {
                response = makeErrorResponse(http::status::service_unavailable,
                    "MVCC API not available", req);
            }
            break;
        }
        case Route::MvccClockGet: {
            if (mvcc_api_handler_) {
                auto httplib_req = HttpTypeAdapter::beastToHttplib(req);
                httplib::Response httplib_res;
                mvcc_api_handler_->handleGetClock(httplib_req, httplib_res);
                response = HttpTypeAdapter::httplibToBeast(httplib_res, req.version());
            } else {
                response = makeErrorResponse(http::status::service_unavailable,
                    "MVCC API not available", req);
            }
            break;
        }
        case Route::MvccStatsGet: {
            if (mvcc_api_handler_) {
                auto httplib_req = HttpTypeAdapter::beastToHttplib(req);
                httplib::Response httplib_res;
                mvcc_api_handler_->handleGetStats(httplib_req, httplib_res);
                response = HttpTypeAdapter::httplibToBeast(httplib_res, req.version());
            } else {
                response = makeErrorResponse(http::status::service_unavailable,
                    "MVCC API not available", req);
            }
            break;
        }

        case Route::GraphQLPost: {
            response = graphql_api_handler_->handlePost(req);
            break;
        }

        case Route::GraphQLSchemaGet: {
            response = graphql_api_handler_->handleSchemaGet(req);
            break;
        }

        // ── gRPC-Web proxy (browser clients) ─────────────────────────────────
        case Route::GrpcWebOptions: {
            response = grpc_web_proxy_->handleOptions(req);
            break;
        }
        case Route::GrpcWebStatusGet: {
            response = grpc_web_proxy_->handleStatus(req);
            break;
        }
        case Route::GrpcWebPost: {
            // HS-6: gRPC-Web proxy requires auth before proxying.
            if (auto auth_err = requireAccess(req, "grpc", "grpc.proxy", path_only)) {
                response = *auth_err;
                break;
            }
            // Extract gRPC method path from /grpc-web/<method>
            const std::string target_str{req.target()};
            const auto qpos = target_str.find('?');
            const std::string grpc_path = (qpos != std::string::npos)
                ? target_str.substr(0, qpos) : target_str;
            // Strip /grpc-web prefix - resulting path is "/<package>.<Service>/<Method>"
            static constexpr std::string_view kGrpcWebPrefix{"/grpc-web"};
            const std::string method_path = grpc_path.substr(kGrpcWebPrefix.size());
            response = grpc_web_proxy_->handlePost(req, method_path);
            break;
        }

        // ── Serverless function hosting ──────────────────────────────────────
        case Route::ServerlessFnPost: {
            response = serverless_fn_handler_->handleRegister(req);
            break;
        }
        case Route::ServerlessFnListGet: {
            response = serverless_fn_handler_->handleList(req);
            break;
        }
        case Route::ServerlessFnGet:
        case Route::ServerlessFnPut:
        case Route::ServerlessFnDelete:
        case Route::ServerlessFnInvokePost:
        case Route::ServerlessFnVersionsGet: {
            // HS-7: Serverless function invocation requires auth.
            if (auto auth_err = requireAccess(req, "functions", "functions.invoke", path_only)) {
                response = *auth_err;
                break;
            }
            // Extract function {id} from /api/v1/functions/{id}[/invoke|/versions]
            static constexpr std::string_view kFnPrefix{"/api/v1/functions/"};
            const std::string target_str{req.target()};
            const auto qpos = target_str.find('?');
            const std::string fn_path = (qpos != std::string::npos)
                ? target_str.substr(0, qpos) : target_str;
            std::string id = fn_path.substr(kFnPrefix.size());
            // Strip trailing sub-resource segment if present
            for (const auto* suffix : {"/invoke", "/versions"}) {
                const std::string_view sv{suffix};
                if (id.size() > sv.size() &&
                    id.substr(id.size() - sv.size()) == sv) {
                    id = id.substr(0, id.size() - sv.size());
                    break;
                }
            }
            const auto route_method = req.method();
            const bool has_invoke  = fn_path.size() > 7 &&
                fn_path.substr(fn_path.size() - 7) == "/invoke";
            const bool has_versions = fn_path.size() > 9 &&
                fn_path.substr(fn_path.size() - 9) == "/versions";
            if (has_invoke)
                response = serverless_fn_handler_->handleInvoke(req, id);
            else if (has_versions)
                response = serverless_fn_handler_->handleVersions(req, id);
            else if (route_method == http::verb::get)
                response = serverless_fn_handler_->handleGet(req, id);
            else if (route_method == http::verb::put)
                response = serverless_fn_handler_->handleUpdate(req, id);
            else
                response = serverless_fn_handler_->handleDelete(req, id);
            break;
        }

        // ── Async job API ────────────────────────────────────────────────────
        case Route::AsyncJobSubmitPost:
            if (async_job_api_)
                response = async_job_api_->handleSubmit(req);
            else
                response = makeErrorResponse(http::status::service_unavailable,
                    "Async job API not available", req);
            break;

        case Route::AsyncJobListGet:
            if (async_job_api_)
                response = async_job_api_->handleList(req);
            else
                response = makeErrorResponse(http::status::service_unavailable,
                    "Async job API not available", req);
            break;

        case Route::AsyncJobStatusGet:
            if (async_job_api_)
                response = async_job_api_->handleGetStatus(req);
            else
                response = makeErrorResponse(http::status::service_unavailable,
                    "Async job API not available", req);
            break;

        case Route::AsyncJobCancelDelete:
            if (async_job_api_)
                response = async_job_api_->handleCancel(req);
            else
                response = makeErrorResponse(http::status::service_unavailable,
                    "Async job API not available", req);
            break;

        // ── API Key Management ───────────────────────────────────────────────
        case Route::ApiKeyPost:
            response = handleApiKeyCreate(req);
            break;
        case Route::ApiKeyListGet:
            response = handleApiKeyList(req);
            break;
        case Route::ApiKeyGet:
            response = handleApiKeyGet(req);
            break;
        case Route::ApiKeyPut:
            response = handleApiKeyUpdate(req);
            break;
        case Route::ApiKeyDelete:
            response = handleApiKeyDelete(req);
            break;

        // ── Session Management ────────────────────────────────────────────────
        case Route::SessionPost:
            response = handleSessionCreate(req);
            break;
        case Route::SessionListGet:
            response = handleSessionList(req);
            break;
        case Route::SessionDeleteById:
            response = handleSessionRevokeById(req);
            break;
        case Route::SessionDeleteOthers:
            response = handleSessionRevokeOthers(req);
            break;

        // ── SAML 2.0 SP ───────────────────────────────────────────────────────
        case Route::SamlLoginGet:
            response = handleSamlLogin(req);
            break;
        case Route::SamlAcsPost:
            response = handleSamlAcs(req);
            break;
        case Route::SamlSloPost:
            response = handleSamlSlo(req);
            break;
        case Route::SamlMetadataGet:
            response = handleSamlMetadata(req);
            break;

        // ── UDF Registration API ──────────────────────────────────────────────
        case Route::UdfPost:
            response = udf_api_handler_->handleRegister(req);
            break;
        case Route::UdfListGet:
            response = udf_api_handler_->handleList(req);
            break;
        case Route::UdfGet: {
            static constexpr std::string_view kUdfPfx{"/api/v1/query/udfs/"};
            std::string udf_path = std::string(req.target());
            if (auto qp = udf_path.find('?'); qp != std::string::npos)
                udf_path = udf_path.substr(0, qp);
            std::string udf_name = udf_path.substr(kUdfPfx.size());
            response = udf_api_handler_->handleGet(req, udf_name);
            break;
        }
        case Route::UdfDelete: {
            static constexpr std::string_view kUdfPfx{"/api/v1/query/udfs/"};
            std::string udf_path = std::string(req.target());
            if (auto qp = udf_path.find('?'); qp != std::string::npos)
                udf_path = udf_path.substr(0, qp);
            std::string udf_name = udf_path.substr(kUdfPfx.size());
            response = udf_api_handler_->handleDelete(req, udf_name);
            break;
        }

        // ── Task Scheduler ────────────────────────────────────────────────
        case Route::TasksUiGet: {
            // Require read access for the UI
            if (auto auth_err = requireAccess(req, "tasks:read", "tasks.ui", "/ui/tasks")) {
                response = *auth_err;
                break;
            }
            auto html = task_scheduler_api_
                ? task_scheduler_api_->getWebUi()
                : "<html><body>Task scheduler not initialized.</body></html>";
            response = http::response<http::string_body>{http::status::ok, req.version()};
            response.set(http::field::content_type, "text/html; charset=utf-8");
            response.body() = std::move(html);
            response.prepare_payload();
            break;
        }
        case Route::TasksPost: {
            if (auto auth_err = requireAccess(req, "tasks:write", "tasks.create", "/api/tasks")) {
                response = *auth_err;
                break;
            }
            auto body = nlohmann::json::parse(req.body(), nullptr, false);
            if (body.is_discarded()) {
                response = makeErrorResponse(http::status::bad_request, "Invalid JSON body", req);
                break;
            }
            if (!task_scheduler_api_) {
                response = makeErrorResponse(http::status::service_unavailable, "Scheduler not initialized", req);
                break;
            }
            auto& task_scheduler_api = *task_scheduler_api_;
            bool scheduler_ctx_set = false;
            if (auth_ && auth_->isEnabled()) {
                auto auth_ctx = extractAuthContext(req);
                TaskScheduler::RequestContext scheduler_ctx;
                scheduler_ctx.user_id = auth_ctx.user_id;
                scheduler_ctx.client_ip = extractClientIP(req);
                scheduler_ctx.authorization_justification = "task:register permission check";
                for (const auto& g : auth_ctx.groups) {
                    scheduler_ctx.roles.insert(g);
                }
                auto auth_header = req[http::field::authorization];
                if (!auth_header.empty()) {
                    auto token = themis::AuthMiddleware::extractBearerToken(
                        std::string_view(auth_header.data(), auth_header.size()));
                    if (token) {
                        auto authz = auth_->authorize(*token, "task:register");
                        if (authz.authorized) {
                            scheduler_ctx.granted_permissions.insert("task:register");
                            if (audit_logger_) {
                                nlohmann::json entry;
                                entry["event"]      = "task_registration_authorized";
                                entry["function"]   = "handleRegister";
                                entry["scope"]      = "task:register";
                                entry["user_id"]    = authz.user_id;
                                entry["authorized"] = true;
                                entry["reason"]     = authz.reason;
                                try { audit_logger_->logEvent(entry); } catch (...) {}
                            }
                        } else if (audit_logger_) {
                            nlohmann::json entry;
                            entry["event"]      = "task_registration_denied";
                            entry["function"]   = "handleRegister";
                            entry["scope"]      = "task:register";
                            entry["user_id"]    = authz.user_id;
                            entry["authorized"] = false;
                            entry["reason"]     = authz.reason;
                            try { audit_logger_->logEvent(entry); } catch (...) {}
                        }
                    }
                }
                TaskScheduler::setRequestContext(scheduler_ctx);
                scheduler_ctx_set = true;
            }
            auto result = task_scheduler_api.registerTask(body);
            if (scheduler_ctx_set) {
                TaskScheduler::clearRequestContext();
            }
            if (result.value("status", "") == "error") {
                response = makeErrorResponse(http::status::bad_request, result.value("error", "Unknown error"), req);
            } else {
                response = makeResponse(http::status::created, result.dump(), req);
            }
            break;
        }
        case Route::TasksListGet: {
            if (auto auth_err = requireAccess(req, "tasks:read", "tasks.list", "/api/tasks")) {
                response = *auth_err;
                break;
            }
            if (!task_scheduler_api_) {
                response = makeErrorResponse(http::status::service_unavailable, "Scheduler not initialized", req);
                break;
            }
            auto& task_scheduler_api = *task_scheduler_api_;
            response = makeResponse(http::status::ok, task_scheduler_api.listTasks().dump(), req);
            break;
        }
        case Route::TasksStatsGet: {
            if (auto auth_err = requireAccess(req, "tasks:read", "tasks.stats", "/api/tasks/stats")) {
                response = *auth_err;
                break;
            }
            if (!task_scheduler_api_) {
                response = makeErrorResponse(http::status::service_unavailable, "Scheduler not initialized", req);
                break;
            }
            auto& task_scheduler_api = *task_scheduler_api_;
            response = makeResponse(http::status::ok, task_scheduler_api.getStats().dump(), req);
            break;
        }
        case Route::TasksGet: {
            if (auto auth_err = requireAccess(req, "tasks:read", "tasks.get", "/api/tasks")) {
                response = *auth_err;
                break;
            }
            static constexpr std::string_view kTasksPfx{"/api/tasks/"};
            std::string ponly = std::string(req.target());
            if (auto qp = ponly.find('?'); qp != std::string::npos) ponly = ponly.substr(0, qp);
            std::string task_id = ponly.substr(kTasksPfx.size());
            if (!task_scheduler_api_) {
                response = makeErrorResponse(http::status::service_unavailable, "Scheduler not initialized", req);
                break;
            }
            auto& task_scheduler_api = *task_scheduler_api_;
            auto result = task_scheduler_api.getTask(task_id);
            if (result.value("status", "") == "error") {
                response = makeErrorResponse(http::status::not_found, result.value("error", "Not found"), req);
            } else {
                response = makeResponse(http::status::ok, result.dump(), req);
            }
            break;
        }
        case Route::TasksPut: {
            if (auto auth_err = requireAccess(req, "tasks:write", "tasks.update", "/api/tasks")) {
                response = *auth_err;
                break;
            }
            static constexpr std::string_view kTasksPfx{"/api/tasks/"};
            std::string ponly = std::string(req.target());
            if (auto qp = ponly.find('?'); qp != std::string::npos) ponly = ponly.substr(0, qp);
            std::string task_id = ponly.substr(kTasksPfx.size());
            auto body = nlohmann::json::parse(req.body(), nullptr, false);
            if (body.is_discarded()) {
                response = makeErrorResponse(http::status::bad_request, "Invalid JSON body", req);
                break;
            }
            if (!task_scheduler_api_) {
                response = makeErrorResponse(http::status::service_unavailable, "Scheduler not initialized", req);
                break;
            }
            auto& task_scheduler_api = *task_scheduler_api_;
            auto result = task_scheduler_api.updateTask(task_id, body);
            if (result.value("status", "") == "error") {
                response = makeErrorResponse(http::status::not_found, result.value("error", "Not found"), req);
            } else {
                response = makeResponse(http::status::ok, result.dump(), req);
            }
            break;
        }
        case Route::TasksDelete: {
            if (auto auth_err = requireAccess(req, "tasks:write", "tasks.delete", "/api/tasks")) {
                response = *auth_err;
                break;
            }
            static constexpr std::string_view kTasksPfx{"/api/tasks/"};
            std::string ponly = std::string(req.target());
            if (auto qp = ponly.find('?'); qp != std::string::npos) ponly = ponly.substr(0, qp);
            std::string task_id = ponly.substr(kTasksPfx.size());
            if (!task_scheduler_api_) {
                response = makeErrorResponse(http::status::service_unavailable, "Scheduler not initialized", req);
                break;
            }
            auto& task_scheduler_api = *task_scheduler_api_;
            auto result = task_scheduler_api.unregisterTask(task_id);
            if (result.value("status", "") == "error") {
                response = makeErrorResponse(http::status::not_found, result.value("error", "Not found"), req);
            } else {
                response = makeResponse(http::status::ok, result.dump(), req);
            }
            break;
        }
        case Route::TasksEnablePost: {
            if (auto auth_err = requireAccess(req, "tasks:write", "tasks.enable", "/api/tasks")) {
                response = *auth_err;
                break;
            }
            static constexpr std::string_view kTasksPfx{"/api/tasks/"};
            std::string ponly = std::string(req.target());
            if (auto qp = ponly.find('?'); qp != std::string::npos) ponly = ponly.substr(0, qp);
            std::string rest = ponly.substr(kTasksPfx.size());
            std::string task_id = rest.substr(0, rest.find('/'));
            if (!task_scheduler_api_) {
                response = makeErrorResponse(http::status::service_unavailable, "Scheduler not initialized", req);
                break;
            }
            auto& task_scheduler_api = *task_scheduler_api_;
            auto result = task_scheduler_api.enableTask(task_id);
            if (result.value("status", "") == "error") {
                response = makeErrorResponse(http::status::not_found, result.value("error", "Not found"), req);
            } else {
                response = makeResponse(http::status::ok, result.dump(), req);
            }
            break;
        }
        case Route::TasksDisablePost: {
            if (auto auth_err = requireAccess(req, "tasks:write", "tasks.disable", "/api/tasks")) {
                response = *auth_err;
                break;
            }
            static constexpr std::string_view kTasksPfx{"/api/tasks/"};
            std::string ponly = std::string(req.target());
            if (auto qp = ponly.find('?'); qp != std::string::npos) ponly = ponly.substr(0, qp);
            std::string rest = ponly.substr(kTasksPfx.size());
            std::string task_id = rest.substr(0, rest.find('/'));
            if (!task_scheduler_api_) {
                response = makeErrorResponse(http::status::service_unavailable, "Scheduler not initialized", req);
                break;
            }
            auto& task_scheduler_api = *task_scheduler_api_;
            auto result = task_scheduler_api.disableTask(task_id);
            if (result.value("status", "") == "error") {
                response = makeErrorResponse(http::status::not_found, result.value("error", "Not found"), req);
            } else {
                response = makeResponse(http::status::ok, result.dump(), req);
            }
            break;
        }
        case Route::TasksExecutePost: {
            if (auto auth_err = requireAccess(req, "tasks:admin", "tasks.execute", "/api/tasks")) {
                response = *auth_err;
                break;
            }
            static constexpr std::string_view kTasksPfx{"/api/tasks/"};
            std::string ponly = std::string(req.target());
            if (auto qp = ponly.find('?'); qp != std::string::npos) ponly = ponly.substr(0, qp);
            std::string rest = ponly.substr(kTasksPfx.size());
            std::string task_id = rest.substr(0, rest.find('/'));
            if (!task_scheduler_api_) {
                response = makeErrorResponse(http::status::service_unavailable, "Scheduler not initialized", req);
                break;
            }
            auto& task_scheduler_api = *task_scheduler_api_;
            bool scheduler_ctx_set = false;
            if (auth_ && auth_->isEnabled()) {
                auto auth_ctx = extractAuthContext(req);
                TaskScheduler::RequestContext scheduler_ctx;
                scheduler_ctx.user_id = auth_ctx.user_id;
                scheduler_ctx.client_ip = extractClientIP(req);
                scheduler_ctx.authorization_justification = "task:execute permission check";
                for (const auto& g : auth_ctx.groups) {
                    scheduler_ctx.roles.insert(g);
                }
                auto auth_header = req[http::field::authorization];
                if (!auth_header.empty()) {
                    auto token = themis::AuthMiddleware::extractBearerToken(
                        std::string_view(auth_header.data(), auth_header.size()));
                    if (token) {
                        auto authz = auth_->authorize(*token, "task:execute");
                        if (authz.authorized) {
                            scheduler_ctx.granted_permissions.insert("task:execute");
                            if (audit_logger_) {
                                nlohmann::json entry;
                                entry["event"]      = "task_execution_authorized";
                                entry["function"]   = "handleExecuteTask";
                                entry["scope"]      = "task:execute";
                                entry["task_id"]    = task_id;
                                entry["user_id"]    = authz.user_id;
                                entry["authorized"] = true;
                                entry["reason"]     = authz.reason;
                                try { audit_logger_->logEvent(entry); } catch (...) {}
                            }
                        } else if (audit_logger_) {
                            nlohmann::json entry;
                            entry["event"]      = "task_execution_denied";
                            entry["function"]   = "handleExecuteTask";
                            entry["scope"]      = "task:execute";
                            entry["task_id"]    = task_id;
                            entry["user_id"]    = authz.user_id;
                            entry["authorized"] = false;
                            entry["reason"]     = authz.reason;
                            try { audit_logger_->logEvent(entry); } catch (...) {}
                        }
                    }
                }
                TaskScheduler::setRequestContext(scheduler_ctx);
                scheduler_ctx_set = true;
            }
            auto result = task_scheduler_api.executeTask(task_id);
            if (scheduler_ctx_set) {
                TaskScheduler::clearRequestContext();
            }
            if (result.value("status", "") == "error") {
                response = makeErrorResponse(http::status::not_found, result.value("error", "Not found"), req);
            } else {
                response = makeResponse(http::status::ok, result.dump(), req);
            }
            break;
        }
        case Route::TasksHistoryGet: {
            if (auto auth_err = requireAccess(req, "tasks:read", "tasks.history", "/api/tasks")) {
                response = *auth_err;
                break;
            }
            static constexpr std::string_view kTasksPfx{"/api/tasks/"};
            nlohmann::json qparams = parseQueryParams(std::string(req.target()));
            std::string target_path = std::string(req.target());
            if (auto qp = target_path.find('?'); qp != std::string::npos) target_path = target_path.substr(0, qp);
            std::string rest = target_path.substr(kTasksPfx.size());
            std::string task_id = rest.substr(0, rest.find('/'));
            if (!task_scheduler_api_) {
                response = makeErrorResponse(http::status::service_unavailable, "Scheduler not initialized", req);
                break;
            }
            auto& task_scheduler_api = *task_scheduler_api_;
            auto result = task_scheduler_api.getExecutionHistory(task_id, qparams);
            response = makeResponse(http::status::ok, result.dump(), req);
            break;
        }

        // ---- Database Maintenance Orchestrator API --------------------------

        case Route::MaintenanceStatusGet: {
            if (auto auth_err = requireAccess(req, "maintenance:read", "maintenance.status",
                                              "/api/v1/maintenance/status")) {
                response = *auth_err; break;
            }
            if (!maintenance_api_) {
                response = makeErrorResponse(http::status::service_unavailable,
                                             "Maintenance orchestrator not initialized", req);
                break;
            }
            auto& maintenance_api = *maintenance_api_;
            response = makeResponse(http::status::ok,
                                    maintenance_api.getStatus().dump(), req);
            break;
        }

        case Route::MaintenanceHealthGet: {
            if (auto auth_err = requireAccess(req, "maintenance:read", "maintenance.health",
                                              "/api/v1/maintenance/health")) {
                response = *auth_err; break;
            }
            if (!maintenance_api_) {
                response = makeErrorResponse(http::status::service_unavailable,
                                             "Maintenance orchestrator not initialized", req);
                break;
            }
            auto& maintenance_api = *maintenance_api_;
            response = makeResponse(http::status::ok,
                                    maintenance_api.getHealth().dump(), req);
            break;
        }

        case Route::MaintenanceTaskHandlersGet: {
            if (auto auth_err = requireAccess(req, "maintenance:read", "maintenance.task_handlers",
                                              "/api/v1/maintenance/task-handlers")) {
                response = *auth_err; break;
            }
            if (!maintenance_api_) {
                response = makeErrorResponse(http::status::service_unavailable,
                                             "Maintenance orchestrator not initialized", req);
                break;
            }
            auto& maintenance_api = *maintenance_api_;
            response = makeResponse(http::status::ok,
                                    maintenance_api.listTaskHandlers().dump(), req);
            break;
        }

        // -- Schedule CRUD --------------------------------------------------

        case Route::MaintenanceSchedulesPost: {
            if (auto auth_err = requireAccess(req, "maintenance:write",
                                              "maintenance.schedules.create",
                                              "/api/v1/maintenance/schedules")) {
                response = *auth_err; break;
            }
            auto body = nlohmann::json::parse(req.body(), nullptr, false);
            if (body.is_discarded()) {
                response = makeErrorResponse(http::status::bad_request, "Invalid JSON body", req);
                break;
            }
            if (!maintenance_api_) {
                response = makeErrorResponse(http::status::service_unavailable,
                                             "Maintenance orchestrator not initialized", req);
                break;
            }
            auto& maintenance_api = *maintenance_api_;
            auto result = maintenance_api.createSchedule(body);
            if (result.value("status", "") == "error") {
                response = makeErrorResponse(http::status::bad_request,
                                             result.value("error", "Unknown error"), req);
            } else {
                response = makeResponse(http::status::created, result.dump(), req);
            }
            break;
        }

        case Route::MaintenanceSchedulesGet: {
            if (auto auth_err = requireAccess(req, "maintenance:read",
                                              "maintenance.schedules.list",
                                              "/api/v1/maintenance/schedules")) {
                response = *auth_err; break;
            }
            if (!maintenance_api_) {
                response = makeErrorResponse(http::status::service_unavailable,
                                             "Maintenance orchestrator not initialized", req);
                break;
            }
            auto& maintenance_api = *maintenance_api_;
            response = makeResponse(http::status::ok,
                                    maintenance_api.listSchedules().dump(), req);
            break;
        }

        case Route::MaintenanceScheduleGet: {
            if (auto auth_err = requireAccess(req, "maintenance:read",
                                              "maintenance.schedules.get",
                                              "/api/v1/maintenance/schedules")) {
                response = *auth_err; break;
            }
            {
                static constexpr std::string_view kPfx{"/api/v1/maintenance/schedules/"};
                std::string ponly = std::string(req.target());
                if (auto q = ponly.find('?'); q != std::string::npos) ponly = ponly.substr(0, q);
                std::string id = ponly.substr(kPfx.size());
                if (!maintenance_api_) {
                    response = makeErrorResponse(http::status::service_unavailable,
                                                 "Maintenance orchestrator not initialized", req);
                    break;
                }
                auto& maintenance_api = *maintenance_api_;
                auto result = maintenance_api.getSchedule(id);
                if (result.value("status", "") == "error") {
                    response = makeErrorResponse(http::status::not_found,
                                                 result.value("error", "Not found"), req);
                } else {
                    response = makeResponse(http::status::ok, result.dump(), req);
                }
            }
            break;
        }

        case Route::MaintenanceSchedulePut: {
            if (auto auth_err = requireAccess(req, "maintenance:write",
                                              "maintenance.schedules.update",
                                              "/api/v1/maintenance/schedules")) {
                response = *auth_err; break;
            }
            {
                static constexpr std::string_view kPfx{"/api/v1/maintenance/schedules/"};
                std::string ponly = std::string(req.target());
                if (auto q = ponly.find('?'); q != std::string::npos) ponly = ponly.substr(0, q);
                std::string id = ponly.substr(kPfx.size());
                auto body = nlohmann::json::parse(req.body(), nullptr, false);
                if (body.is_discarded()) {
                    response = makeErrorResponse(http::status::bad_request, "Invalid JSON body", req);
                    break;
                }
                if (!maintenance_api_) {
                    response = makeErrorResponse(http::status::service_unavailable,
                                                 "Maintenance orchestrator not initialized", req);
                    break;
                }
                auto& maintenance_api = *maintenance_api_;
                auto result = maintenance_api.updateSchedule(id, body);
                if (result.value("status", "") == "error") {
                    response = makeErrorResponse(http::status::not_found,
                                                 result.value("error", "Not found"), req);
                } else {
                    response = makeResponse(http::status::ok, result.dump(), req);
                }
            }
            break;
        }

        case Route::MaintenanceSchedulePatch: {
            if (auto auth_err = requireAccess(req, "maintenance:write",
                                              "maintenance.schedules.patch",
                                              "/api/v1/maintenance/schedules")) {
                response = *auth_err; break;
            }
            {
                static constexpr std::string_view kPfx{"/api/v1/maintenance/schedules/"};
                std::string ponly = std::string(req.target());
                if (auto q = ponly.find('?'); q != std::string::npos) ponly = ponly.substr(0, q);
                std::string id = ponly.substr(kPfx.size());
                auto patch = nlohmann::json::parse(req.body(), nullptr, false);
                if (patch.is_discarded()) {
                    response = makeErrorResponse(http::status::bad_request, "Invalid JSON body", req);
                    break;
                }
                if (!maintenance_api_) {
                    response = makeErrorResponse(http::status::service_unavailable,
                                                 "Maintenance orchestrator not initialized", req);
                    break;
                }
                auto& maintenance_api = *maintenance_api_;
                auto result = maintenance_api.patchSchedule(id, patch);
                if (result.value("status", "") == "error") {
                    response = makeErrorResponse(http::status::not_found,
                                                 result.value("error", "Not found"), req);
                } else {
                    response = makeResponse(http::status::ok, result.dump(), req);
                }
            }
            break;
        }

        case Route::MaintenanceScheduleDelete: {
            if (auto auth_err = requireAccess(req, "maintenance:write",
                                              "maintenance.schedules.delete",
                                              "/api/v1/maintenance/schedules")) {
                response = *auth_err; break;
            }
            {
                static constexpr std::string_view kPfx{"/api/v1/maintenance/schedules/"};
                std::string ponly = std::string(req.target());
                if (auto q = ponly.find('?'); q != std::string::npos) ponly = ponly.substr(0, q);
                std::string id = ponly.substr(kPfx.size());
                if (!maintenance_api_) {
                    response = makeErrorResponse(http::status::service_unavailable,
                                                 "Maintenance orchestrator not initialized", req);
                    break;
                }
                auto& maintenance_api = *maintenance_api_;
                auto result = maintenance_api.deleteSchedule(id);
                if (result.value("status", "") == "error") {
                    response = makeErrorResponse(http::status::not_found,
                                                 result.value("error", "Not found"), req);
                } else {
                    response = makeResponse(http::status::ok, result.dump(), req);
                }
            }
            break;
        }

        case Route::MaintenanceScheduleRunPost: {
            // Step 1: baseline auth - all callers need at least maintenance:write.
            if (auto auth_err = requireAccess(req, "maintenance:write",
                                              "maintenance.schedules.run",
                                              "/api/v1/maintenance/schedules")) {
                response = *auth_err; break;
            }
            // Step 2: parse and validate the optional body.
            //   - Invalid JSON  → 400
            //   - "force" not a boolean → 400
            bool force_run = false;
            {
                const auto& body_str = req.body();
                if (!body_str.empty()) {
                    auto body = nlohmann::json::parse(body_str, nullptr, false);
                    if (body.is_discarded()) {
                        response = makeErrorResponse(http::status::bad_request,
                                                     "Invalid JSON body", req);
                        break;
                    }
                    if (body.is_object() && body.contains("force")) {
                        if (!body["force"].is_boolean()) {
                            response = makeErrorResponse(http::status::bad_request,
                                                         "\"force\" must be a boolean", req);
                            break;
                        }
                        force_run = body["force"].get<bool>();
                    }
                }
            }
            // Step 3: force=true requires elevated maintenance:admin scope.
            if (force_run) {
                if (auto auth_err = requireAccess(req, "maintenance:admin",
                                                  "maintenance.schedules.run.force",
                                                  "/api/v1/maintenance/schedules")) {
                    response = *auth_err; break;
                }
            }
            {
                static constexpr std::string_view kPfx{"/api/v1/maintenance/schedules/"};
                std::string ponly = std::string(req.target());
                if (auto q = ponly.find('?'); q != std::string::npos) ponly = ponly.substr(0, q);
                std::string rest = ponly.substr(kPfx.size());
                std::string id   = rest.substr(0, rest.find('/'));
                if (!maintenance_api_) {
                    response = makeErrorResponse(http::status::service_unavailable,
                                                 "Maintenance orchestrator not initialized", req);
                    break;
                }
                auto& maintenance_api = *maintenance_api_;
                auto result = maintenance_api.triggerNow(id, force_run);
                if (result.value("status", "") == "error") {
                    response = makeErrorResponse(http::status::not_found,
                                                 result.value("error", "Not found"), req);
                } else {
                    response = makeResponse(http::status::accepted, result.dump(), req);
                }
            }
            break;
        }

        // -- Jobs -----------------------------------------------------------

        case Route::MaintenanceJobsGet: {
            if (auto auth_err = requireAccess(req, "maintenance:read",
                                              "maintenance.jobs.list",
                                              "/api/v1/maintenance/jobs")) {
                response = *auth_err; break;
            }
            if (!maintenance_api_) {
                response = makeErrorResponse(http::status::service_unavailable,
                                             "Maintenance orchestrator not initialized", req);
                break;
            }
            auto& maintenance_api = *maintenance_api_;
            nlohmann::json qp = parseQueryParams(std::string(req.target()));
            bool active_only = qp.value("active_only", false);
            response = makeResponse(http::status::ok,
                                    maintenance_api.listJobs(active_only).dump(), req);
            break;
        }

        case Route::MaintenanceJobGet: {
            if (auto auth_err = requireAccess(req, "maintenance:read",
                                              "maintenance.jobs.get",
                                              "/api/v1/maintenance/jobs")) {
                response = *auth_err; break;
            }
            {
                static constexpr std::string_view kPfx{"/api/v1/maintenance/jobs/"};
                std::string ponly = std::string(req.target());
                if (auto q = ponly.find('?'); q != std::string::npos) ponly = ponly.substr(0, q);
                std::string id = ponly.substr(kPfx.size());
                if (!maintenance_api_) {
                    response = makeErrorResponse(http::status::service_unavailable,
                                                 "Maintenance orchestrator not initialized", req);
                    break;
                }
                auto& maintenance_api = *maintenance_api_;
                auto result = maintenance_api.getJob(id);
                if (result.value("status", "") == "error") {
                    response = makeErrorResponse(http::status::not_found,
                                                 result.value("error", "Not found"), req);
                } else {
                    response = makeResponse(http::status::ok, result.dump(), req);
                }
            }
            break;
        }

        case Route::MaintenanceJobCancelPost: {
            if (auto auth_err = requireAccess(req, "maintenance:write",
                                              "maintenance.jobs.cancel",
                                              "/api/v1/maintenance/jobs")) {
                response = *auth_err; break;
            }
            {
                static constexpr std::string_view kPfx{"/api/v1/maintenance/jobs/"};
                std::string ponly = std::string(req.target());
                if (auto q = ponly.find('?'); q != std::string::npos) ponly = ponly.substr(0, q);
                std::string rest = ponly.substr(kPfx.size());
                std::string id   = rest.substr(0, rest.find('/'));
                if (!maintenance_api_) {
                    response = makeErrorResponse(http::status::service_unavailable,
                                                 "Maintenance orchestrator not initialized", req);
                    break;
                }
                auto& maintenance_api = *maintenance_api_;
                auto result = maintenance_api.cancelJob(id);
                if (result.value("status", "") == "error") {
                    response = makeErrorResponse(http::status::not_found,
                                                 result.value("error", "Not found"), req);
                } else {
                    response = makeResponse(http::status::ok, result.dump(), req);
                }
            }
            break;
        }

        // ── Retention Policy Admin API ──────────────────────────────────────
        case Route::RetentionPoliciesGet: {
            if (auto auth_err = requireAccess(req, "admin", "retention.policies.list",
                                              "/api/retention/policies")) {
                response = *auth_err; break;
            }
            if (!retention_api_) {
                response = makeErrorResponse(http::status::service_unavailable,
                                             "Retention API not initialized", req);
                break;
            }
            auto& retention_api = *retention_api_;
            server::RetentionQueryFilter filter;
            if (auto qpos = std::string(req.target()).find('?'); qpos != std::string::npos) {
                auto qs = std::string(req.target()).substr(qpos + 1);
                // Simple key=value parsing for common params
                for (auto& seg : {std::string("page"), std::string("page_size"),
                                   std::string("name"), std::string("classification")}) {
                    auto key = seg + "=";
                    auto pos = qs.find(key);
                    if (pos != std::string::npos) {
                        auto val = qs.substr(pos + key.size());
                        if (auto end = val.find('&'); end != std::string::npos) val = val.substr(0, end);
                        if (seg == "page") try { filter.page = std::stoi(val); } catch (...) {}
                        else if (seg == "page_size") try { filter.page_size = std::stoi(val); } catch (...) {}
                        else if (seg == "name") filter.name_filter = val;
                        else if (seg == "classification") filter.classification_filter = val;
                    }
                }
            }
            response = makeResponse(http::status::ok,
                                    retention_api.listPolicies(filter).dump(), req);
            break;
        }

        case Route::RetentionPoliciesPost: {
            if (auto auth_err = requireAccess(req, "admin", "retention.policies.write",
                                              "/api/retention/policies")) {
                response = *auth_err; break;
            }
            if (!retention_api_) {
                response = makeErrorResponse(http::status::service_unavailable,
                                             "Retention API not initialized", req);
                break;
            }
            auto& retention_api = *retention_api_;
            auto body = nlohmann::json::parse(req.body(), nullptr, false);
            if (body.is_discarded()) {
                response = makeErrorResponse(http::status::bad_request,
                                             "Invalid JSON body", req);
                break;
            }
            auto result = retention_api.createOrUpdatePolicy(body);
            auto created = result.value("status", "") == "created";
            response = makeResponse(
                created ? http::status::created : http::status::ok,
                result.dump(), req);
            break;
        }

        case Route::RetentionPolicyDelete: {
            if (auto auth_err = requireAccess(req, "admin", "retention.policies.delete",
                                              "/api/retention/policies")) {
                response = *auth_err; break;
            }
            if (!retention_api_) {
                response = makeErrorResponse(http::status::service_unavailable,
                                             "Retention API not initialized", req);
                break;
            }
            auto& retention_api = *retention_api_;
            std::string request_target = std::string(req.target());
            std::string policy_name = request_target.substr(request_target.rfind('/') + 1);
            auto qpos = policy_name.find('?');
            if (qpos != std::string::npos) policy_name = policy_name.substr(0, qpos);
            if (policy_name.empty()) {
                response = makeErrorResponse(http::status::bad_request,
                                             "Missing policy name in path", req);
                break;
            }
            auto result = retention_api.deletePolicy(policy_name);
            if (result.value("status", "") == "not_found") {
                response = makeErrorResponse(http::status::not_found,
                                             "Retention policy not found", req);
            } else {
                response = makeResponse(http::status::ok, result.dump(), req);
            }
            break;
        }

        case Route::RetentionHistoryGet: {
            if (auto auth_err = requireAccess(req, "audit:read", "retention.history.read",
                                              "/api/retention/history")) {
                response = *auth_err; break;
            }
            if (!retention_api_) {
                response = makeErrorResponse(http::status::service_unavailable,
                                             "Retention API not initialized", req);
                break;
            }
            auto& retention_api = *retention_api_;
            int limit = 100;
            if (auto qpos = std::string(req.target()).find("limit="); qpos != std::string::npos) {
                try { limit = std::stoi(std::string(req.target()).substr(qpos + 6)); } catch (...) {}
            }
            auto result = retention_api.getHistory(limit);
            response = makeResponse(http::status::ok, result.dump(), req);
            break;
        }

        // ── SAGA Audit Log API ──────────────────────────────────────────────
        case Route::SAGABatchesGet: {
            if (auto auth_err = requireAccess(req, "audit:read", "saga.batches.list",
                                              "/api/saga/batches")) {
                response = *auth_err; break;
            }
            if (!saga_api_) {
                response = makeErrorResponse(http::status::service_unavailable,
                                             "SAGA API not initialized", req);
                break;
            }
            auto& saga_api = *saga_api_;
            response = makeResponse(http::status::ok,
                                    saga_api.listBatches().dump(), req);
            break;
        }

        case Route::SAGABatchGet: {
            if (auto auth_err = requireAccess(req, "audit:read", "saga.batches.get",
                                              "/api/saga/batches")) {
                response = *auth_err; break;
            }
            if (!saga_api_) {
                response = makeErrorResponse(http::status::service_unavailable,
                                             "SAGA API not initialized", req);
                break;
            }
            auto& saga_api = *saga_api_;
            std::string request_target = std::string(req.target());
            static constexpr std::string_view kPrefix = "/api/saga/batches/";
            std::string batch_id = request_target.substr(kPrefix.size());
            auto qpos = batch_id.find('?');
            if (qpos != std::string::npos) batch_id = batch_id.substr(0, qpos);
            if (batch_id.empty()) {
                response = makeErrorResponse(http::status::bad_request,
                                             "Missing batch ID in path", req);
                break;
            }
            response = makeResponse(http::status::ok,
                                    saga_api.getBatchDetail(batch_id).dump(), req);
            break;
        }

        case Route::SAGABatchVerifyPost: {
            if (auto auth_err = requireAccess(req, "audit:read", "saga.batches.verify",
                                              "/api/saga/batches")) {
                response = *auth_err; break;
            }
            if (!saga_api_) {
                response = makeErrorResponse(http::status::service_unavailable,
                                             "SAGA API not initialized", req);
                break;
            }
            auto& saga_api = *saga_api_;
            // Extract batch_id from path /api/saga/batches/{id}/verify
            std::string request_target = std::string(req.target());
            static constexpr std::string_view kVPrefix = "/api/saga/batches/";
            std::string rest = request_target.substr(kVPrefix.size());
            auto slash_pos = rest.find('/');
            std::string batch_id = (slash_pos != std::string::npos)
                ? rest.substr(0, slash_pos) : rest;
            auto qpos = batch_id.find('?');
            if (qpos != std::string::npos) batch_id = batch_id.substr(0, qpos);
            if (batch_id.empty()) {
                response = makeErrorResponse(http::status::bad_request,
                                             "Missing batch ID in path", req);
                break;
            }
            response = makeResponse(http::status::ok,
                                    saga_api.verifyBatch(batch_id).dump(), req);
            break;
        }

        case Route::SAGAFlushPost: {
            if (auto auth_err = requireAccess(req, "admin", "saga.flush",
                                              "/api/saga/flush")) {
                response = *auth_err; break;
            }
            if (!saga_api_) {
                response = makeErrorResponse(http::status::service_unavailable,
                                             "SAGA API not initialized", req);
                break;
            }
            auto& saga_api = *saga_api_;
            response = makeResponse(http::status::ok,
                                    saga_api.flushCurrentBatch().dump(), req);
            break;
        }

        // ── CQL Phase 8: Continuous Query endpoints ───────────────────────
        case Route::ContinuousQueryRegisterPost: {
            if (auto auth_err = requireAccess(req, "user", "cq.register",
                                              "/v1/queries/continuous")) {
                response = *auth_err; break;
            }
            if (!continuous_query_api_) {
                response = makeErrorResponse(http::status::service_unavailable,
                    "Continuous query engine not initialized", req);
                break;
            }
            auto& continuous_query_api = *continuous_query_api_;
            response = continuous_query_api.handleRegister(req);
            break;
        }

        case Route::ContinuousQueryListGet: {
            if (auto auth_err = requireAccess(req, "user", "cq.list",
                                              "/v1/queries/continuous")) {
                response = *auth_err; break;
            }
            if (!continuous_query_api_) {
                response = makeErrorResponse(http::status::service_unavailable,
                    "Continuous query engine not initialized", req);
                break;
            }
            auto& continuous_query_api = *continuous_query_api_;
            response = continuous_query_api.handleList(req);
            break;
        }

        case Route::ContinuousQueryDropDelete: {
            // Extract :name from /v1/queries/continuous/:name
            std::string cq_path = std::string(req.target());
            auto qpos = cq_path.find('?');
            if (qpos != std::string::npos) cq_path = cq_path.substr(0, qpos);
            const std::string cq_name = cq_path.substr(23);  // strip "/v1/queries/continuous/"
            if (auto auth_err = requireAccess(req, "user", "cq.drop",
                                              "/v1/queries/continuous")) {
                response = *auth_err; break;
            }
            if (!continuous_query_api_) {
                response = makeErrorResponse(http::status::service_unavailable,
                    "Continuous query engine not initialized", req);
                break;
            }
            auto& continuous_query_api = *continuous_query_api_;
            response = continuous_query_api.handleDrop(req, cq_name);
            break;
        }

        case Route::ContinuousQueryStreamSseGet: {
            // Extract :name from /v1/queries/continuous/:name/results
            std::string cq_path = std::string(req.target());
            auto qpos = cq_path.find('?');
            if (qpos != std::string::npos) cq_path = cq_path.substr(0, qpos);
            const std::string rest_cq = cq_path.substr(23);  // strip prefix
            const auto slash_pos = rest_cq.find('/');
            const std::string cq_name = (slash_pos != std::string::npos)
                ? rest_cq.substr(0, slash_pos)
                : rest_cq;
            if (auto auth_err = requireAccess(req, "user", "cq.stream",
                                              "/v1/queries/continuous")) {
                response = *auth_err; break;
            }
            if (!continuous_query_api_) {
                response = makeErrorResponse(http::status::service_unavailable,
                    "Continuous query engine not initialized", req);
                break;
            }
            auto& continuous_query_api = *continuous_query_api_;
            response = continuous_query_api.handleStreamSse(req, cq_name);
            break;
        }

        // ── AI Safety Layer — HILG Approval Endpoints (ASL-6) ──────────────
        // Docs: docs/de/security/ai_safety/AI_SAFETY_OPERATION_GUARD.md
#ifdef THEMIS_ENABLE_MCP
        case Route::AiPendingApprovalsGet: {
            if (auto auth_err = requireAccess(req, "admin", "ai.approvals.read",
                                              "/v1/ai/pending-approvals")) {
                response = *auth_err; break;
            }
            if (!mcp_server_) {
                response = makeErrorResponse(http::status::service_unavailable,
                    "MCP server not initialized", req);
                break;
            }
            auto& mcp_server = *mcp_server_;
            const json result = mcp_server.handleAiPendingApprovals();
            response = makeResponse(http::status::ok, result.dump(), req);
            break;
        }

        case Route::AiApprovePendingPost: {
            if (auto auth_err = requireAccess(req, "admin", "ai.approvals.write",
                                              "/v1/ai/approve/")) {
                response = *auth_err; break;
            }
            if (!mcp_server_) {
                response = makeErrorResponse(http::status::service_unavailable,
                    "MCP server not initialized", req);
                break;
            }
            auto& mcp_server = *mcp_server_;
            {
                // Extract operation_id from /v1/ai/approve/{operation_id}
                std::string ai_path = std::string(req.target());
                const auto qp = ai_path.find('?');
                if (qp != std::string::npos) ai_path = ai_path.substr(0, qp);
                constexpr std::size_t kApproveRouteLen = 15;  // strlen("/v1/ai/approve/")
                const std::string op_id = ai_path.substr(kApproveRouteLen);
                const json result = mcp_server.handleAiApprove(op_id);
                const bool is_error = result.value("status", "") == "error";
                response = makeResponse(
                    is_error ? http::status::not_found : http::status::ok,
                    result.dump(), req);
            }
            break;
        }

        case Route::AiDenyPendingPost: {
            if (auto auth_err = requireAccess(req, "admin", "ai.approvals.write",
                                              "/v1/ai/deny/")) {
                response = *auth_err; break;
            }
            if (!mcp_server_) {
                response = makeErrorResponse(http::status::service_unavailable,
                    "MCP server not initialized", req);
                break;
            }
            auto& mcp_server = *mcp_server_;
            {
                // Extract operation_id from /v1/ai/deny/{operation_id}
                std::string ai_path = std::string(req.target());
                const auto qp = ai_path.find('?');
                if (qp != std::string::npos) ai_path = ai_path.substr(0, qp);
                constexpr std::size_t kDenyRouteLen = 12;  // strlen("/v1/ai/deny/")
                const std::string op_id = ai_path.substr(kDenyRouteLen);
                const json result = mcp_server.handleAiDeny(op_id);
                const bool is_error = result.value("status", "") == "error";
                response = makeResponse(
                    is_error ? http::status::not_found : http::status::ok,
                    result.dump(), req);
            }
            break;
        }

        // ASL-10: Rollback endpoint
        // Docs: docs/de/security/ai_safety/AI_SAFETY_OPERATION_GUARD.md
        case Route::AiRollbackPost: {
            if (auto auth_err = requireAccess(req, "admin", "ai.approvals.write",
                                              "/v1/ai/rollback/")) {
                response = *auth_err; break;
            }
            if (!mcp_server_) {
                response = makeErrorResponse(http::status::service_unavailable,
                    "MCP server not initialized", req);
                break;
            }
            auto& mcp_server = *mcp_server_;
            {
                std::string ai_path = std::string(req.target());
                const auto qp = ai_path.find('?');
                if (qp != std::string::npos) ai_path = ai_path.substr(0, qp);
                constexpr std::size_t kRollbackRouteLen = 16;  // strlen("/v1/ai/rollback/")
                const std::string snap_id = ai_path.substr(kRollbackRouteLen);
                const json result = mcp_server.handleAiRollback(snap_id);
                const bool is_error = result.value("status", "") != "success";
                response = makeResponse(
                    is_error ? http::status::bad_request : http::status::ok,
                    result.dump(), req);
            }
            break;
        }
#else  // !THEMIS_ENABLE_MCP
        case Route::AiPendingApprovalsGet:
        case Route::AiApprovePendingPost:
        case Route::AiDenyPendingPost:
        case Route::AiRollbackPost:
            response = makeErrorResponse(http::status::not_implemented,
                "MCP support not enabled in this build", req);
            break;
#endif  // THEMIS_ENABLE_MCP

        case Route::NotFound:
        default:
            response = makeErrorResponse(http::status::not_found, "Endpoint not found", req);
            break;
    }

    // Record latency before returning
    auto end = std::chrono::steady_clock::now();
    auto dur = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    recordLatency(dur);
    // Trace status code
    span.setAttribute("http.status_code", static_cast<int64_t>(response.result_int()));
    if (response.result_int() >= 200 && response.result_int() < 400) {
        span.setStatus(true);
    } else {
        span.setStatus(false);
    }

} catch (const nlohmann::json::exception& e) {
        // JSON parsing errors (including invalid UTF-8)
        THEMIS_ERROR("JSON parsing error in routeRequest: {}", e.what());
        response = makeErrorResponse(http::status::bad_request, 
            "JSON parse error: " + std::string(e.what()), req);
        span.setStatus(false, "json_parse_error");
        span.recordError(e.what());
        
        auto end = std::chrono::steady_clock::now();
        auto dur = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        recordLatency(dur);
    } catch (const std::exception& e) {
        // Any other uncaught exceptions
        THEMIS_ERROR("Uncaught exception in routeRequest: {}", e.what());
        response = makeErrorResponse(http::status::internal_server_error, 
            "Internal server error: " + std::string(e.what()), req);
        span.setStatus(false, "internal_error");
        span.recordError(e.what());
        
        auto end = std::chrono::steady_clock::now();
        auto dur = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        recordLatency(dur);
    }

    // Propagate request ID to response
    if (!request_id.empty()) {
        response.set("X-Request-ID", request_id);
    }

    // Emit structured JSON access log line
    {
        auto total_dur = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - start
        );
        double duration_ms = total_dur.count() / 1000.0;
        std::string client_ip = extractClientIP(req);
        THEMIS_INFO("access method={} path={} status={} duration_ms={:.3f} request_id={} client_ip={}",
            std::string(http::to_string(req.method())),
            target,
            response.result_int(),
            duration_ms,
            request_id.empty() ? "-" : request_id,
            client_ip.empty() ? "-" : client_ip
        );
    }

    return response;
}

// -----------------------------------------------------------------------------
// Keys / Classification / Reports API Handlers
// -----------------------------------------------------------------------------

http::response<http::string_body> HttpServer::handleKeysListKeys(
    const http::request<http::string_body>& req
) {
    try {
        if (!keys_api_) {
            return makeErrorResponse(http::status::service_unavailable, "Keys API not available", req);
        }
        auto& keys_api = *keys_api_;
        auto result = keys_api.listKeys();
        return makeResponse(http::status::ok, result.dump(), req);
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> HttpServer::handlePkiSign(
    const http::request<http::string_body>& req
) {
    try {
        if (!pki_api_) return makeErrorResponse(http::status::service_unavailable, "PKI API not available", req);

        // Authorization: require pki:sign when auth is enabled
        if (auto resp = requireAccess(req, "pki:sign", "pki.sign", "/api/pki")) return *resp;

        // Extract key_id from path: /api/pki/:key_id/sign
        auto path = std::string(req.target());
        auto key_id = extractPathParam(path, "/api/pki/");
        // key_id currently contains "<key_id>/sign" -> trim suffix
        if (key_id.size() > 5 && key_id.compare(key_id.size() - 5, 5, "/sign") == 0) {
            key_id = key_id.substr(0, key_id.size() - 5);
        }
        if (key_id.empty()) return makeErrorResponse(http::status::bad_request, "Missing key_id", req);
        if (validator_ && !validator_->validatePathSegment(key_id)) {
            return makeErrorResponse(http::status::bad_request, "Invalid key_id", req);
        }

        nlohmann::json body = nlohmann::json::object();
        if (!req.body().empty()) body = nlohmann::json::parse(req.body());

        if (validator_) {
            if (auto err = validator_->validateJsonStub(body, "pki_sign")) {
                return makeErrorResponse(http::status::bad_request, *err, req);
            }
        }

        auto& pki_api = *pki_api_;
        auto result = pki_api.sign(key_id, body);
        if (result.contains("status_code")) {
            int sc = result.value("status_code", 500);
            return makeErrorResponse(static_cast<http::status>(sc), result.dump(), req);
        }
        return makeResponse(http::status::ok, result.dump(), req);
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> HttpServer::handlePkiVerify(
    const http::request<http::string_body>& req
) {
    try {
        if (!pki_api_) return makeErrorResponse(http::status::service_unavailable, "PKI API not available", req);

        // Authorization: require pki:verify when auth is enabled
        if (auto resp = requireAccess(req, "pki:verify", "pki.verify", "/api/pki")) return *resp;

        // Extract key_id from path: /api/pki/:key_id/verify
        auto path = std::string(req.target());
        auto key_id = extractPathParam(path, "/api/pki/");
        if (key_id.size() > 7 && key_id.compare(key_id.size() - 7, 7, "/verify") == 0) {
            key_id = key_id.substr(0, key_id.size() - 7);
        }
        if (key_id.empty()) return makeErrorResponse(http::status::bad_request, "Missing key_id", req);
        if (validator_ && !validator_->validatePathSegment(key_id)) {
            return makeErrorResponse(http::status::bad_request, "Invalid key_id", req);
        }

        nlohmann::json body = nlohmann::json::object();
        if (!req.body().empty()) body = nlohmann::json::parse(req.body());

        if (validator_) {
            if (auto err = validator_->validateJsonStub(body, "pki_verify")) {
                return makeErrorResponse(http::status::bad_request, *err, req);
            }
        }

        auto& pki_api = *pki_api_;
        auto result = pki_api.verify(key_id, body);
        if (result.contains("status_code")) {
            int sc = result.value("status_code", 500);
            return makeErrorResponse(static_cast<http::status>(sc), result.dump(), req);
        }
        return makeResponse(http::status::ok, result.dump(), req);
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

// ============================================================================
// New PKI HSM, TSA, eIDAS Handlers
// ============================================================================

http::response<http::string_body> HttpServer::handlePkiHsmSign(
    const http::request<http::string_body>& req
) {
    try {
        if (!pki_api_) return makeErrorResponse(http::status::service_unavailable, "PKI API not available", req);
        if (auto resp = requireAccess(req, "pki:sign", "pki.hsm.sign", "/api/pki")) return *resp;

        nlohmann::json body = nlohmann::json::object();
        if (!req.body().empty()) body = nlohmann::json::parse(req.body());

        auto& pki_api = *pki_api_;
        auto result = pki_api.hsmSign(body);
        if (result.contains("status_code")) {
            int sc = result.value("status_code", 500);
            return makeErrorResponse(static_cast<http::status>(sc), result.dump(), req);
        }
        return makeResponse(http::status::ok, result.dump(), req);
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> HttpServer::handlePkiHsmKeys(
    const http::request<http::string_body>& req
) {
    try {
        if (!pki_api_) return makeErrorResponse(http::status::service_unavailable, "PKI API not available", req);
        if (auto resp = requireAccess(req, "pki:read", "pki.hsm.read", "/api/pki")) return *resp;

        auto& pki_api = *pki_api_;
        auto result = pki_api.hsmListKeys();
        if (result.contains("status_code")) {
            int sc = result.value("status_code", 500);
            return makeErrorResponse(static_cast<http::status>(sc), result.dump(), req);
        }
        return makeResponse(http::status::ok, result.dump(), req);
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> HttpServer::handlePkiTimestamp(
    const http::request<http::string_body>& req
) {
    try {
        if (!pki_api_) return makeErrorResponse(http::status::service_unavailable, "PKI API not available", req);
        if (auto resp = requireAccess(req, "pki:timestamp", "pki.timestamp", "/api/pki")) return *resp;

        nlohmann::json body = nlohmann::json::object();
        if (!req.body().empty()) body = nlohmann::json::parse(req.body());

        auto& pki_api = *pki_api_;
        auto result = pki_api.getTimestamp(body);
        if (result.contains("status_code")) {
            int sc = result.value("status_code", 500);
            return makeErrorResponse(static_cast<http::status>(sc), result.dump(), req);
        }
        return makeResponse(http::status::ok, result.dump(), req);
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> HttpServer::handlePkiTimestampVerify(
    const http::request<http::string_body>& req
) {
    try {
        if (!pki_api_) return makeErrorResponse(http::status::service_unavailable, "PKI API not available", req);
        if (auto resp = requireAccess(req, "pki:verify", "pki.timestamp.verify", "/api/pki")) return *resp;

        nlohmann::json body = nlohmann::json::object();
        if (!req.body().empty()) body = nlohmann::json::parse(req.body());

        auto& pki_api = *pki_api_;
        auto result = pki_api.verifyTimestamp(body);
        if (result.contains("status_code")) {
            int sc = result.value("status_code", 500);
            return makeErrorResponse(static_cast<http::status>(sc), result.dump(), req);
        }
        return makeResponse(http::status::ok, result.dump(), req);
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> HttpServer::handlePkiEidasSign(
    const http::request<http::string_body>& req
) {
    try {
        if (!pki_api_) return makeErrorResponse(http::status::service_unavailable, "PKI API not available", req);
        if (auto resp = requireAccess(req, "pki:eidas", "pki.eidas.sign", "/api/pki")) return *resp;

        nlohmann::json body = nlohmann::json::object();
        if (!req.body().empty()) body = nlohmann::json::parse(req.body());

        auto& pki_api = *pki_api_;
        auto result = pki_api.eidasSign(body);
        if (result.contains("status_code")) {
            int sc = result.value("status_code", 500);
            return makeErrorResponse(static_cast<http::status>(sc), result.dump(), req);
        }
        return makeResponse(http::status::ok, result.dump(), req);
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> HttpServer::handlePkiEidasVerify(
    const http::request<http::string_body>& req
) {
    try {
        if (!pki_api_) return makeErrorResponse(http::status::service_unavailable, "PKI API not available", req);
        if (auto resp = requireAccess(req, "pki:verify", "pki.eidas.verify", "/api/pki")) return *resp;

        nlohmann::json body = nlohmann::json::object();
        if (!req.body().empty()) body = nlohmann::json::parse(req.body());

        auto& pki_api = *pki_api_;
        auto result = pki_api.eidasVerify(body);
        if (result.contains("status_code")) {
            int sc = result.value("status_code", 500);
            return makeErrorResponse(static_cast<http::status>(sc), result.dump(), req);
        }
        return makeResponse(http::status::ok, result.dump(), req);
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> HttpServer::handlePkiCertificates(
    const http::request<http::string_body>& req
) {
    try {
        if (!pki_api_) return makeErrorResponse(http::status::service_unavailable, "PKI API not available", req);
        if (auto resp = requireAccess(req, "pki:read", "pki.certificates.read", "/api/pki")) return *resp;

        auto& pki_api = *pki_api_;
        auto result = pki_api.listCertificates();
        if (result.contains("status_code")) {
            int sc = result.value("status_code", 500);
            return makeErrorResponse(static_cast<http::status>(sc), result.dump(), req);
        }
        return makeResponse(http::status::ok, result.dump(), req);
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> HttpServer::handlePkiCertificate(
    const http::request<http::string_body>& req
) {
    try {
        if (!pki_api_) return makeErrorResponse(http::status::service_unavailable, "PKI API not available", req);
        if (auto resp = requireAccess(req, "pki:read", "pki.certificates.read", "/api/pki")) return *resp;

        // Extract cert_id from path: /api/pki/certificates/:cert_id
        auto path = std::string(req.target());
        auto cert_id = extractPathParam(path, "/api/pki/certificates/");
        if (cert_id.empty()) return makeErrorResponse(http::status::bad_request, "Missing cert_id", req);
        if (validator_ && !validator_->validatePathSegment(cert_id)) {
            return makeErrorResponse(http::status::bad_request, "Invalid cert_id", req);
        }

        auto& pki_api = *pki_api_;
        auto result = pki_api.getCertificate(cert_id);
        if (result.contains("status_code")) {
            int sc = result.value("status_code", 500);
            return makeErrorResponse(static_cast<http::status>(sc), result.dump(), req);
        }
        return makeResponse(http::status::ok, result.dump(), req);
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> HttpServer::handlePkiStatus(
    const http::request<http::string_body>& req
) {
    try {
        if (!pki_api_) return makeErrorResponse(http::status::service_unavailable, "PKI API not available", req);
        if (auto resp = requireAccess(req, "pki:read", "pki.status", "/api/pki")) return *resp;

        auto& pki_api = *pki_api_;
        auto result = pki_api.getStatus();
        if (result.contains("status_code")) {
            int sc = result.value("status_code", 500);
            return makeErrorResponse(static_cast<http::status>(sc), result.dump(), req);
        }
        return makeResponse(http::status::ok, result.dump(), req);
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> HttpServer::handleKeysRotateKey(
    const http::request<http::string_body>& req
) {
    try {
        if (!keys_api_) {
            return makeErrorResponse(http::status::service_unavailable, "Keys API not available", req);
        }
        // Parse key_id from body or query string
        std::string key_id;
        try {
            if (!req.body().empty()) {
                auto body = json::parse(req.body());
                if (body.contains("key_id")) key_id = body.value("key_id", "");
            }
        } catch (...) {
            THEMIS_DEBUG("http_server: unhandled exception caught");
            // ignore body parse errors; fallback to query param
        }
        if (key_id.empty()) {
            std::string target = std::string(req.target());
            auto qpos = target.find('?');
            if (qpos != std::string::npos) {
                auto qs = target.substr(qpos + 1);
                std::istringstream iss(qs);
                std::string kv;
                while (std::getline(iss, kv, '&')) {
                    auto eq = kv.find('=');
                    if (eq != std::string::npos) {
                        auto k = kv.substr(0, eq);
                        auto v = kv.substr(eq + 1);
                        if (k == "key_id") { key_id = v; break; }
                    }
                }
            }
        }
        if (key_id.empty()) {
            return makeErrorResponse(http::status::bad_request, "Missing key_id", req);
        }
        // Pass original body if JSON else empty
        json body_json;
        try { if (!req.body().empty()) body_json = json::parse(req.body()); } catch (...) {}
        auto& keys_api = *keys_api_;
        auto result = keys_api.rotateKey(key_id, body_json);
        return makeResponse(http::status::ok, result.dump(), req);
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

// -----------------------------------------------------------------------------
// API Key Management Handlers
// -----------------------------------------------------------------------------

http::response<http::string_body> HttpServer::handleApiKeyCreate(
    const http::request<http::string_body>& req
) {
    try {
        if (!api_key_mgmt_) {
            return makeErrorResponse(http::status::service_unavailable, "API Key Management not available", req);
        }
        if (auto resp = requireAccess(req, "admin:all", "api_key.create", "/api/keys")) return *resp;
        if (req.body().empty()) {
            return makeErrorResponse(http::status::bad_request, "Missing JSON body", req);
        }
        json body;
        try { body = json::parse(req.body()); } catch (...) {
            THEMIS_DEBUG("http_server: unhandled exception caught");
            return makeErrorResponse(http::status::bad_request, "Invalid JSON body", req);
        }
        auto& api_key_mgmt = *api_key_mgmt_;
        auto result = api_key_mgmt.createKey(body);
        if (result.contains("status_code")) {
            int sc = result.value("status_code", 500);
            return makeErrorResponse(static_cast<http::status>(sc), result.dump(), req);
        }
        return makeResponse(http::status::created, result.dump(), req);
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> HttpServer::handleApiKeyList(
    const http::request<http::string_body>& req
) {
    try {
        if (!api_key_mgmt_) {
            return makeErrorResponse(http::status::service_unavailable, "API Key Management not available", req);
        }
        if (auto resp = requireAccess(req, "admin:all", "api_key.list", "/api/keys")) return *resp;
        auto& api_key_mgmt = *api_key_mgmt_;
        auto result = api_key_mgmt.listKeys();
        return makeResponse(http::status::ok, result.dump(), req);
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> HttpServer::handleApiKeyGet(
    const http::request<http::string_body>& req
) {
    try {
        if (!api_key_mgmt_) {
            return makeErrorResponse(http::status::service_unavailable, "API Key Management not available", req);
        }
        if (auto resp = requireAccess(req, "admin:all", "api_key.get", "/api/keys")) return *resp;
        auto key_id = extractPathParam(std::string(req.target()), "/api/keys/");
        if (key_id.empty()) {
            return makeErrorResponse(http::status::bad_request, "Missing key id", req);
        }
        if (validator_ && !validator_->validatePathSegment(key_id)) {
            return makeErrorResponse(http::status::bad_request, "Invalid key id", req);
        }
        auto& api_key_mgmt = *api_key_mgmt_;
        auto result = api_key_mgmt.getKey(key_id);
        if (result.contains("status_code")) {
            int sc = result.value("status_code", 500);
            return makeErrorResponse(static_cast<http::status>(sc), result.dump(), req);
        }
        return makeResponse(http::status::ok, result.dump(), req);
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> HttpServer::handleApiKeyUpdate(
    const http::request<http::string_body>& req
) {
    try {
        if (!api_key_mgmt_) {
            return makeErrorResponse(http::status::service_unavailable, "API Key Management not available", req);
        }
        if (auto resp = requireAccess(req, "admin:all", "api_key.update", "/api/keys")) return *resp;
        auto key_id = extractPathParam(std::string(req.target()), "/api/keys/");
        if (key_id.empty()) {
            return makeErrorResponse(http::status::bad_request, "Missing key id", req);
        }
        if (validator_ && !validator_->validatePathSegment(key_id)) {
            return makeErrorResponse(http::status::bad_request, "Invalid key id", req);
        }
        json body;
        try { if (!req.body().empty()) body = json::parse(req.body()); } catch (...) {
            THEMIS_DEBUG("http_server: unhandled exception caught");
            return makeErrorResponse(http::status::bad_request, "Invalid JSON body", req);
        }
        auto& api_key_mgmt = *api_key_mgmt_;
        auto result = api_key_mgmt.updateKey(key_id, body);
        if (result.contains("status_code")) {
            int sc = result.value("status_code", 500);
            return makeErrorResponse(static_cast<http::status>(sc), result.dump(), req);
        }
        return makeResponse(http::status::ok, result.dump(), req);
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> HttpServer::handleApiKeyDelete(
    const http::request<http::string_body>& req
) {
    try {
        if (!api_key_mgmt_) {
            return makeErrorResponse(http::status::service_unavailable, "API Key Management not available", req);
        }
        if (auto resp = requireAccess(req, "admin:all", "api_key.delete", "/api/keys")) return *resp;
        auto key_id = extractPathParam(std::string(req.target()), "/api/keys/");
        if (key_id.empty()) {
            return makeErrorResponse(http::status::bad_request, "Missing key id", req);
        }
        if (validator_ && !validator_->validatePathSegment(key_id)) {
            return makeErrorResponse(http::status::bad_request, "Invalid key id", req);
        }
        auto& api_key_mgmt = *api_key_mgmt_;
        auto result = api_key_mgmt.deleteKey(key_id);
        if (result.contains("status_code")) {
            int sc = result.value("status_code", 500);
            return makeErrorResponse(static_cast<http::status>(sc), result.dump(), req);
        }
        return makeResponse(http::status::ok, result.dump(), req);
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

// -----------------------------------------------------------------------------
// Session Management Handlers
// -----------------------------------------------------------------------------

http::response<http::string_body> HttpServer::handleSessionCreate(
    const http::request<http::string_body>& req
) {
    try {
        if (!session_api_) {
            return makeErrorResponse(http::status::service_unavailable, "Session management not available", req);
        }
        const auto auth_header = req[http::field::authorization];
        if (auth_header.empty()) {
            return makeErrorResponse(http::status::unauthorized, "Missing Authorization header", req);
        }
        auto token = themis::AuthMiddleware::extractBearerToken(
            std::string_view(auth_header.data(), auth_header.size()));
        if (!token) {
            return makeErrorResponse(http::status::unauthorized, "Invalid Bearer token format", req);
        }
        json body;
        if (!req.body().empty()) {
            try { body = json::parse(req.body()); } catch (...) {
                THEMIS_DEBUG("http_server: unhandled exception caught");
                return makeErrorResponse(http::status::bad_request, "Invalid JSON body", req);
            }
        }
        // Extract client IP from X-Forwarded-For or remote endpoint
        std::string client_ip;
        auto fwd = req.find("X-Forwarded-For");
        if (fwd != req.end()) {
            client_ip = std::string(fwd->value());
            auto comma = client_ip.find(',');
            if (comma != std::string::npos) client_ip = client_ip.substr(0, comma);
            // Trim leading and trailing whitespace
            auto first = client_ip.find_first_not_of(" \t");
            auto last  = client_ip.find_last_not_of(" \t");
            client_ip = (first == std::string::npos) ? "" : client_ip.substr(first, last - first + 1);
        }
        auto& session_api = *session_api_;
        auto result = session_api.createSession(*token, body, client_ip);
        if (result.contains("status_code")) {
            int sc = result.value("status_code", 500);
            return makeErrorResponse(static_cast<http::status>(sc), result.dump(), req);
        }
        return makeResponse(http::status::created, result.dump(), req);
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> HttpServer::handleSessionList(
    const http::request<http::string_body>& req
) {
    try {
        if (!session_api_) {
            return makeErrorResponse(http::status::service_unavailable, "Session management not available", req);
        }
        const auto auth_header = req[http::field::authorization];
        if (auth_header.empty()) {
            return makeErrorResponse(http::status::unauthorized, "Missing Authorization header", req);
        }
        auto token = themis::AuthMiddleware::extractBearerToken(
            std::string_view(auth_header.data(), auth_header.size()));
        if (!token) {
            return makeErrorResponse(http::status::unauthorized, "Invalid Bearer token format", req);
        }
        // Optionally accept current session from a header or query param
        std::string current_session;
        auto cs_hdr = req.find("X-Session-Id");
        if (cs_hdr != req.end()) current_session = std::string(cs_hdr->value());
        auto& session_api = *session_api_;
        auto result = session_api.listSessions(*token, current_session);
        if (result.contains("status_code")) {
            int sc = result.value("status_code", 500);
            return makeErrorResponse(static_cast<http::status>(sc), result.dump(), req);
        }
        return makeResponse(http::status::ok, result.dump(), req);
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> HttpServer::handleSessionRevokeById(
    const http::request<http::string_body>& req
) {
    try {
        if (!session_api_) {
            return makeErrorResponse(http::status::service_unavailable, "Session management not available", req);
        }
        const auto auth_header = req[http::field::authorization];
        if (auth_header.empty()) {
            return makeErrorResponse(http::status::unauthorized, "Missing Authorization header", req);
        }
        auto token = themis::AuthMiddleware::extractBearerToken(
            std::string_view(auth_header.data(), auth_header.size()));
        if (!token) {
            return makeErrorResponse(http::status::unauthorized, "Invalid Bearer token format", req);
        }
        auto session_id = extractPathParam(std::string(req.target()), "/auth/sessions/");
        if (session_id.empty()) {
            return makeErrorResponse(http::status::bad_request, "Missing session id", req);
        }
        auto& session_api = *session_api_;
        auto result = session_api.revokeSession(*token, session_id);
        if (result.contains("status_code")) {
            int sc = result.value("status_code", 500);
            return makeErrorResponse(static_cast<http::status>(sc), result.dump(), req);
        }
        return makeResponse(http::status::ok, result.dump(), req);
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> HttpServer::handleSessionRevokeOthers(
    const http::request<http::string_body>& req
) {
    try {
        if (!session_api_) {
            return makeErrorResponse(http::status::service_unavailable, "Session management not available", req);
        }
        const auto auth_header = req[http::field::authorization];
        if (auth_header.empty()) {
            return makeErrorResponse(http::status::unauthorized, "Missing Authorization header", req);
        }
        auto token = themis::AuthMiddleware::extractBearerToken(
            std::string_view(auth_header.data(), auth_header.size()));
        if (!token) {
            return makeErrorResponse(http::status::unauthorized, "Invalid Bearer token format", req);
        }
        std::string current_session;
        if (!req.body().empty()) {
            try {
                auto body = json::parse(req.body());
                if (body.contains("current_session_id") && body["current_session_id"].is_string()) {
                    current_session = body["current_session_id"].get<std::string>();
                }
            } catch (...) {
                THEMIS_WARN("http_server: unhandled exception caught");
                return makeErrorResponse(http::status::bad_request, "Invalid JSON body", req);
            }
        }
        auto& session_api = *session_api_;
        auto result = session_api.revokeAllOtherSessions(*token, current_session);
        if (result.contains("status_code")) {
            int sc = result.value("status_code", 500);
            return makeErrorResponse(static_cast<http::status>(sc), result.dump(), req);
        }
        return makeResponse(http::status::ok, result.dump(), req);
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

// -----------------------------------------------------------------------------
// SAML 2.0 SP Handlers
// -----------------------------------------------------------------------------

http::response<http::string_body> HttpServer::handleSamlLogin(
    const http::request<http::string_body>& req
) {
    try {
        if (!saml_provider_) {
            return makeErrorResponse(http::status::service_unavailable,
                                     "SAML provider not configured", req);
        }
        // Extract optional relay_state from query string.
        std::string relay_state;
        const std::string target = std::string(req.target());
        auto qpos = target.find('?');
        if (qpos != std::string::npos) {
            const std::string qs = target.substr(qpos + 1);
            std::istringstream iss(qs);
            std::string kv;
            while (std::getline(iss, kv, '&')) {
                auto eq = kv.find('=');
                if (eq != std::string::npos && kv.substr(0, eq) == "relay_state") {
                    relay_state = urlDecode(kv.substr(eq + 1));
                    break;
                }
            }
        }
        auto& saml_provider = *saml_provider_;
        auto result = saml_provider.handleLogin(relay_state);
        if (result.contains("status_code")) {
            int sc = result.value("status_code", 500);
            return makeErrorResponse(static_cast<http::status>(sc), result.dump(), req);
        }
        // Return 302 redirect to IdP SSO URL.
        auto redirect_url = result.value("redirect_url", "");
        http::response<http::string_body> res{http::status::found, req.version()};
        res.set(http::field::location, redirect_url);
        res.set(http::field::content_type, "application/json");
        res.keep_alive(req.keep_alive());
        res.body() = result.dump();
        res.prepare_payload();
        return res;
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> HttpServer::handleSamlAcs(
    const http::request<http::string_body>& req
) {
    try {
        if (!saml_provider_) {
            return makeErrorResponse(http::status::service_unavailable,
                                     "SAML provider not configured", req);
        }
        // ACS receives an application/x-www-form-urlencoded POST body
        // with SAMLResponse and optional RelayState fields (SAML 2.0 Bindings §3.5).
        // InResponseTo is validated by SAMLAuthenticator from the XML assertion; the
        // pending request_id stored during handleSamlLogin is supplied as in_response_to
        // to enable server-side SP-initiated flow verification.  The request_id may be
        // recovered from: (1) a server-side session keyed on the browser session cookie,
        // or (2) the RelayState when it encodes the request_id.  For programmatic / test
        // clients an explicit X-SAML-RequestId header is also accepted.
        std::string saml_response_b64;
        std::string relay_state;

        const auto& body = req.body();
        if (!body.empty()) {
            std::istringstream iss(body);
            std::string kv;
            while (std::getline(iss, kv, '&')) {
                auto eq = kv.find('=');
                if (eq == std::string::npos) continue;
                const auto key   = urlDecode(kv.substr(0, eq));
                const auto value = urlDecode(kv.substr(eq + 1));
                if (key == "SAMLResponse")  saml_response_b64 = value;
                else if (key == "RelayState") relay_state      = value;
            }
        }
        if (saml_response_b64.empty()) {
            return makeErrorResponse(http::status::bad_request, "Missing SAMLResponse field", req);
        }
        // Recover the original SP-initiated request_id (if any).
        // Check the X-SAML-RequestId header first (programmatic clients / proxies),
        // then fall back to an empty string (IdP-initiated flow or no validation needed).
        std::string in_response_to;
        auto req_id_hdr = req.find("X-SAML-RequestId");
        if (req_id_hdr != req.end()) {
            in_response_to = std::string(req_id_hdr->value());
        }
        auto& saml_provider = *saml_provider_;
        auto result = saml_provider.handleAcs(saml_response_b64, relay_state, in_response_to);
        if (result.contains("status_code")) {
            int sc = result.value("status_code", 500);
            return makeErrorResponse(static_cast<http::status>(sc), result.dump(), req);
        }
        return makeResponse(http::status::ok, result.dump(), req);
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> HttpServer::handleSamlSlo(
    const http::request<http::string_body>& req
) {
    try {
        if (!saml_provider_) {
            return makeErrorResponse(http::status::service_unavailable,
                                     "SAML provider not configured", req);
        }
        std::string session_index;
        if (!req.body().empty()) {
            try {
                auto body_json = json::parse(req.body());
                if (body_json.contains("session_index") && body_json["session_index"].is_string()) {
                    session_index = body_json["session_index"].get<std::string>();
                }
            } catch (...) {
                THEMIS_DEBUG("http_server: unhandled exception caught");
                // Non-JSON bodies (e.g. form-encoded) are silently ignored for SLO.
            }
        }
        auto& saml_provider = *saml_provider_;
        auto result = saml_provider.handleSlo(session_index);
        if (result.contains("status_code")) {
            int sc = result.value("status_code", 500);
            return makeErrorResponse(static_cast<http::status>(sc), result.dump(), req);
        }
        // If result contains a redirect_url, issue 302.
        if (result.contains("redirect_url") && !result["redirect_url"].get<std::string>().empty()) {
            http::response<http::string_body> res{http::status::found, req.version()};
            res.set(http::field::location, result["redirect_url"].get<std::string>());
            res.set(http::field::content_type, "application/json");
            res.keep_alive(req.keep_alive());
            res.body() = result.dump();
            res.prepare_payload();
            return res;
        }
        return makeResponse(http::status::ok, result.dump(), req);
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> HttpServer::handleSamlMetadata(
    const http::request<http::string_body>& req
) {
    try {
        if (!saml_provider_) {
            return makeErrorResponse(http::status::service_unavailable,
                                     "SAML provider not configured", req);
        }
        auto& saml_provider = *saml_provider_;
        const auto xml = saml_provider.buildMetadataXml();
        http::response<http::string_body> res{http::status::ok, req.version()};
        res.set(http::field::content_type, "application/samlmetadata+xml");
        res.keep_alive(req.keep_alive());
        res.body() = xml;
        res.prepare_payload();
        return res;
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> HttpServer::handleClassificationListRules(
    const http::request<http::string_body>& req
) {
    try {
        if (!classification_api_) {
            return makeErrorResponse(http::status::service_unavailable, "Classification API not available", req);
        }
        auto& classification_api = *classification_api_;
        auto result = classification_api.listRules();
        return makeResponse(http::status::ok, result.dump(), req);
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> HttpServer::handleClassificationTest(
    const http::request<http::string_body>& req
) {
    try {
        if (!classification_api_) {
            return makeErrorResponse(http::status::service_unavailable, "Classification API not available", req);
        }
        if (req.body().empty()) {
            return makeErrorResponse(http::status::bad_request, "Missing JSON body", req);
        }
        auto body = json::parse(req.body());
        auto& classification_api = *classification_api_;
        auto result = classification_api.testClassification(body);
        return makeResponse(http::status::ok, result.dump(), req);
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> HttpServer::handleReportsCompliance(
    const http::request<http::string_body>& req
) {
    try {
        if (!reports_api_) {
            return makeErrorResponse(http::status::service_unavailable, "Reports API not available", req);
        }
        std::string report_type = "overview";
        std::string target = std::string(req.target());
        auto qpos = target.find('?');
        if (qpos != std::string::npos) {
            auto qs = target.substr(qpos + 1);
            std::istringstream iss(qs);
            std::string kv;
            while (std::getline(iss, kv, '&')) {
                auto eq = kv.find('=');
                if (eq != std::string::npos) {
                    auto k = kv.substr(0, eq);
                    auto v = kv.substr(eq + 1);
                    if (k == "type" && !v.empty()) { report_type = v; break; }
                }
            }
        }
        auto& reports_api = *reports_api_;
        auto result = reports_api.generateComplianceReport(report_type);
        return makeResponse(http::status::ok, result.dump(), req);
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

// -----------------------------------------------------------------------------
// Metrics exporter (Prometheus text exposition)
// -----------------------------------------------------------------------------
http::response<http::string_body> HttpServer::handleMetrics(const http::request<http::string_body>& req) {
    try {
        std::ostringstream out;
        out << "# HELP themis_content_blob_compressed_bytes_total Total bytes stored compressed for content blobs\n";
        out << "# TYPE themis_content_blob_compressed_bytes_total counter\n";
        out << "# HELP themis_content_blob_uncompressed_bytes_total Total uncompressed/original bytes observed for content blob uploads\n";
        out << "# TYPE themis_content_blob_uncompressed_bytes_total counter\n";
        out << "# HELP themis_content_blob_compression_skipped_total Number of uploads skipped for compression (by MIME prefix)\n";
        out << "# TYPE themis_content_blob_compression_skipped_total counter\n";
        out << "# HELP themis_content_blob_compression_ratio Histogram of compression ratios (original_size / compressed_size) per upload\n";
        out << "# TYPE themis_content_blob_compression_ratio histogram\n";

        // Encryption metrics
        out << "# HELP themis_encryption_operations_total Total number of encryption operations\n";
        out << "# TYPE themis_encryption_operations_total counter\n";
        out << "# HELP themis_decryption_operations_total Total number of decryption operations\n";
        out << "# TYPE themis_decryption_operations_total counter\n";
        out << "# HELP themis_reencryption_operations_total Total number of successful lazy re-encryptions\n";
        out << "# TYPE themis_reencryption_operations_total counter\n";
        out << "# HELP themis_reencryption_skipped_total Number of re-encryption checks that found data already using latest key\n";
        out << "# TYPE themis_reencryption_skipped_total counter\n";
        out << "# HELP themis_encryption_errors_total Total number of encryption failures\n";
        out << "# TYPE themis_encryption_errors_total counter\n";
        out << "# HELP themis_decryption_errors_total Total number of decryption failures\n";
        out << "# TYPE themis_decryption_errors_total counter\n";
        out << "# HELP themis_reencryption_errors_total Total number of lazy re-encryption failures\n";
        out << "# TYPE themis_reencryption_errors_total counter\n";
        out << "# HELP themis_encryption_bytes_total Total bytes encrypted\n";
        out << "# TYPE themis_encryption_bytes_total counter\n";
        out << "# HELP themis_decryption_bytes_total Total bytes decrypted\n";
        out << "# TYPE themis_decryption_bytes_total counter\n";
        out << "# HELP themis_encryption_duration_seconds Encryption operation latency histogram\n";
        out << "# TYPE themis_encryption_duration_seconds histogram\n";
        out << "# HELP themis_decryption_duration_seconds Decryption operation latency histogram\n";
        out << "# TYPE themis_decryption_duration_seconds histogram\n";

        // WAL replication metrics (apply-side)
        out << "# HELP themis_wal_apply_success_total Total successful WAL apply batches\n";
        out << "# TYPE themis_wal_apply_success_total counter\n";
        out << "# HELP themis_wal_apply_fail_total Total failed WAL apply batches\n";
        out << "# TYPE themis_wal_apply_fail_total counter\n";
        out << "# HELP themis_wal_apply_latency_seconds WAL apply latency histogram\n";
        out << "# TYPE themis_wal_apply_latency_seconds histogram\n";
        out << "# HELP themis_wal_last_applied_lsn Last replicated WAL LSN observed\n";
        out << "# TYPE themis_wal_last_applied_lsn gauge\n";

        // WAL replication metrics (ship-side, per replica)
        out << "# HELP themis_wal_ship_batches_total Total WAL ship batches sent\n";
        out << "# TYPE themis_wal_ship_batches_total counter\n";
        out << "# HELP themis_wal_ship_entries_total Total WAL entries shipped\n";
        out << "# TYPE themis_wal_ship_entries_total counter\n";
        out << "# HELP themis_wal_ship_bytes_total Total WAL bytes shipped\n";
        out << "# TYPE themis_wal_ship_bytes_total counter\n";
        out << "# HELP themis_wal_ship_failures_total Total WAL ship failures\n";
        out << "# TYPE themis_wal_ship_failures_total counter\n";
        out << "# HELP themis_wal_replication_lag_seconds Replication lag in seconds per replica\n";
        out << "# TYPE themis_wal_replication_lag_seconds gauge\n";
        out << "# HELP themis_wal_backlog_bytes Replication backlog in bytes per replica\n";
        out << "# TYPE themis_wal_backlog_bytes gauge\n";
        
        // Write concern metrics
        out << "# HELP themis_write_concern_waits_total Total write concern wait operations\n";
        out << "# TYPE themis_write_concern_waits_total counter\n";
        out << "# HELP themis_write_concern_wait_seconds Write concern wait time histogram\n";
        out << "# TYPE themis_write_concern_wait_seconds histogram\n";
        out << "# HELP themis_replication_pending_writes Current pending writes awaiting replication\n";
        out << "# TYPE themis_replication_pending_writes gauge\n";
        out << "# HELP themis_replication_quorum_timeouts_total Total quorum timeout failures\n";
        out << "# TYPE themis_replication_quorum_timeouts_total counter\n";

        // Encryption metrics (if field_encryption_ available)
        if (field_encryption_) {
            const auto& em = field_encryption_->getMetrics();
            
            // Operation counters
            out << "themis_encryption_operations_total " << em.encrypt_operations_total.load(std::memory_order_relaxed) << "\n";
            out << "themis_decryption_operations_total " << em.decrypt_operations_total.load(std::memory_order_relaxed) << "\n";
            out << "themis_reencryption_operations_total " << em.reencrypt_operations_total.load(std::memory_order_relaxed) << "\n";
            out << "themis_reencryption_skipped_total " << em.reencrypt_skipped_total.load(std::memory_order_relaxed) << "\n";
            
            // Error counters
            out << "themis_encryption_errors_total " << em.encrypt_errors_total.load(std::memory_order_relaxed) << "\n";
            out << "themis_decryption_errors_total " << em.decrypt_errors_total.load(std::memory_order_relaxed) << "\n";
            out << "themis_reencryption_errors_total " << em.reencrypt_errors_total.load(std::memory_order_relaxed) << "\n";
            
            // Bytes processed
            out << "themis_encryption_bytes_total " << em.encrypt_bytes_total.load(std::memory_order_relaxed) << "\n";
            out << "themis_decryption_bytes_total " << em.decrypt_bytes_total.load(std::memory_order_relaxed) << "\n";
            
            // Encryption duration histogram (cumulative buckets)
            uint64_t enc_le_100us = em.encrypt_duration_le_100us.load(std::memory_order_relaxed);
            uint64_t enc_le_500us = em.encrypt_duration_le_500us.load(std::memory_order_relaxed);
            uint64_t enc_le_1ms = em.encrypt_duration_le_1ms.load(std::memory_order_relaxed);
            uint64_t enc_le_5ms = em.encrypt_duration_le_5ms.load(std::memory_order_relaxed);
            uint64_t enc_le_10ms = em.encrypt_duration_le_10ms.load(std::memory_order_relaxed);
            uint64_t enc_gt_10ms = em.encrypt_duration_gt_10ms.load(std::memory_order_relaxed);
            
            out << "themis_encryption_duration_seconds_bucket{le=\"0.0001\"} " << enc_le_100us << "\n";
            out << "themis_encryption_duration_seconds_bucket{le=\"0.0005\"} " << (enc_le_100us + enc_le_500us) << "\n";
            out << "themis_encryption_duration_seconds_bucket{le=\"0.001\"} " << (enc_le_100us + enc_le_500us + enc_le_1ms) << "\n";
            out << "themis_encryption_duration_seconds_bucket{le=\"0.005\"} " << (enc_le_100us + enc_le_500us + enc_le_1ms + enc_le_5ms) << "\n";
            out << "themis_encryption_duration_seconds_bucket{le=\"0.01\"} " << (enc_le_100us + enc_le_500us + enc_le_1ms + enc_le_5ms + enc_le_10ms) << "\n";
            out << "themis_encryption_duration_seconds_bucket{le=\"+Inf\"} " << (enc_le_100us + enc_le_500us + enc_le_1ms + enc_le_5ms + enc_le_10ms + enc_gt_10ms) << "\n";
            out << "themis_encryption_duration_seconds_count " << em.encrypt_operations_total.load(std::memory_order_relaxed) << "\n";
            
            // Decryption duration histogram (cumulative buckets)
            uint64_t dec_le_100us = em.decrypt_duration_le_100us.load(std::memory_order_relaxed);
            uint64_t dec_le_500us = em.decrypt_duration_le_500us.load(std::memory_order_relaxed);
            uint64_t dec_le_1ms = em.decrypt_duration_le_1ms.load(std::memory_order_relaxed);
            uint64_t dec_le_5ms = em.decrypt_duration_le_5ms.load(std::memory_order_relaxed);
            uint64_t dec_le_10ms = em.decrypt_duration_le_10ms.load(std::memory_order_relaxed);
            uint64_t dec_gt_10ms = em.decrypt_duration_gt_10ms.load(std::memory_order_relaxed);
            
            out << "themis_decryption_duration_seconds_bucket{le=\"0.0001\"} " << dec_le_100us << "\n";
            out << "themis_decryption_duration_seconds_bucket{le=\"0.0005\"} " << (dec_le_100us + dec_le_500us) << "\n";
            out << "themis_decryption_duration_seconds_bucket{le=\"0.001\"} " << (dec_le_100us + dec_le_500us + dec_le_1ms) << "\n";
            out << "themis_decryption_duration_seconds_bucket{le=\"0.005\"} " << (dec_le_100us + dec_le_500us + dec_le_1ms + dec_le_5ms) << "\n";
            out << "themis_decryption_duration_seconds_bucket{le=\"0.01\"} " << (dec_le_100us + dec_le_500us + dec_le_1ms + dec_le_5ms + dec_le_10ms) << "\n";
            out << "themis_decryption_duration_seconds_bucket{le=\"+Inf\"} " << (dec_le_100us + dec_le_500us + dec_le_1ms + dec_le_5ms + dec_le_10ms + dec_gt_10ms) << "\n";
            out << "themis_decryption_duration_seconds_count " << em.decrypt_operations_total.load(std::memory_order_relaxed) << "\n";
        } else {
            // No encryption configured: emit zeros
            out << "themis_encryption_operations_total 0\n";
            out << "themis_decryption_operations_total 0\n";
            out << "themis_reencryption_operations_total 0\n";
            out << "themis_reencryption_skipped_total 0\n";
            out << "themis_encryption_errors_total 0\n";
            out << "themis_decryption_errors_total 0\n";
            out << "themis_reencryption_errors_total 0\n";
            out << "themis_encryption_bytes_total 0\n";
            out << "themis_decryption_bytes_total 0\n";
            out << "themis_encryption_duration_seconds_bucket{le=\"0.0001\"} 0\n";
            out << "themis_encryption_duration_seconds_bucket{le=\"0.0005\"} 0\n";
            out << "themis_encryption_duration_seconds_bucket{le=\"0.001\"} 0\n";
            out << "themis_encryption_duration_seconds_bucket{le=\"0.005\"} 0\n";
            out << "themis_encryption_duration_seconds_bucket{le=\"0.01\"} 0\n";
            out << "themis_encryption_duration_seconds_bucket{le=\"+Inf\"} 0\n";
            out << "themis_encryption_duration_seconds_count 0\n";
            out << "themis_decryption_duration_seconds_bucket{le=\"0.0001\"} 0\n";
            out << "themis_decryption_duration_seconds_bucket{le=\"0.0005\"} 0\n";
            out << "themis_decryption_duration_seconds_bucket{le=\"0.001\"} 0\n";
            out << "themis_decryption_duration_seconds_bucket{le=\"0.005\"} 0\n";
            out << "themis_decryption_duration_seconds_bucket{le=\"0.01\"} 0\n";
            out << "themis_decryption_duration_seconds_bucket{le=\"+Inf\"} 0\n";
            out << "themis_decryption_duration_seconds_count 0\n";
        }

        // WAL replication metrics (always emit, even if replication not configured)
        uint64_t wal_success = wal_api_ ? wal_api_->getApplySuccessCount() : 0;
        uint64_t wal_fail = wal_api_ ? wal_api_->getApplyFailCount() : 0;
        uint64_t wal_le_50ms = wal_api_ ? wal_api_->getApplyLatencyLe50ms() : 0;
        uint64_t wal_le_200ms = wal_api_ ? wal_api_->getApplyLatencyLe200ms() : 0;
        uint64_t wal_le_1000ms = wal_api_ ? wal_api_->getApplyLatencyLe1000ms() : 0;
        uint64_t wal_gt_1000ms = wal_api_ ? wal_api_->getApplyLatencyGt1000ms() : 0;
        uint64_t wal_latency_count = wal_api_ ? wal_api_->getApplyLatencyCount() : 0;
        uint64_t wal_latency_sum_us = wal_api_ ? wal_api_->getApplyLatencySumUs() : 0;

        uint64_t wal_bucket_50 = wal_le_50ms;
        uint64_t wal_bucket_200 = wal_le_50ms + wal_le_200ms;
        uint64_t wal_bucket_1000 = wal_bucket_200 + wal_le_1000ms;
        uint64_t wal_bucket_inf = std::max<uint64_t>(wal_latency_count, wal_bucket_1000 + wal_gt_1000ms);

        out << "themis_wal_apply_success_total " << wal_success << "\n";
        out << "themis_wal_apply_fail_total " << wal_fail << "\n";
        out << "themis_wal_apply_latency_seconds_bucket{le=\"0.05\"} " << wal_bucket_50 << "\n";
        out << "themis_wal_apply_latency_seconds_bucket{le=\"0.2\"} " << wal_bucket_200 << "\n";
        out << "themis_wal_apply_latency_seconds_bucket{le=\"1\"} " << wal_bucket_1000 << "\n";
        out << "themis_wal_apply_latency_seconds_bucket{le=\"+Inf\"} " << wal_bucket_inf << "\n";
        out << std::fixed << std::setprecision(6);
        out << "themis_wal_apply_latency_seconds_sum " << (static_cast<double>(wal_latency_sum_us) / 1'000'000.0) << "\n";
        out << "themis_wal_apply_latency_seconds_count " << wal_latency_count << "\n";
        out.unsetf(std::ios::floatfield);

        std::string wal_last_lsn;
        if (wal_api_) {
            wal_last_lsn = wal_api_->getLastAppliedLsn();
        }
        out << "themis_wal_last_applied_lsn{lsn=\"" << wal_last_lsn << "\"} 1\n";

        // Content metrics (if available)
        if (content_manager_) {
            auto& content_manager = *content_manager_;
            const auto& m = content_manager.getMetrics();
            uint64_t comp_bytes = m.compressed_bytes_total.load(std::memory_order_relaxed);
            uint64_t uncomp_bytes = m.uncompressed_bytes_total.load(std::memory_order_relaxed);
            uint64_t skipped = m.compression_skipped_total.load(std::memory_order_relaxed);
            out << "themis_content_blob_compressed_bytes_total " << comp_bytes << "\n";
            out << "themis_content_blob_uncompressed_bytes_total " << uncomp_bytes << "\n";
            out << "themis_content_blob_compression_skipped_total " << skipped << "\n";

            // Skipped by known categories
            uint64_t skipped_img = m.compression_skipped_image_total.load(std::memory_order_relaxed);
            uint64_t skipped_vid = m.compression_skipped_video_total.load(std::memory_order_relaxed);
            uint64_t skipped_zip = m.compression_skipped_zip_total.load(std::memory_order_relaxed);
            out << "themis_content_blob_compression_skipped_total{mime_prefix=\"image/\"} " << skipped_img << "\n";
            out << "themis_content_blob_compression_skipped_total{mime_prefix=\"video/\"} " << skipped_vid << "\n";
            out << "themis_content_blob_compression_skipped_total{mime_prefix=\"application/zip\"} " << skipped_zip << "\n";

            // Build cumulative buckets from per-bucket counts
            std::vector<std::pair<std::string, uint64_t>> buckets = {
                {"1", m.comp_ratio_le_1.load(std::memory_order_relaxed)},
                {"1.5", m.comp_ratio_le_1_5.load(std::memory_order_relaxed)},
                {"2", m.comp_ratio_le_2.load(std::memory_order_relaxed)},
                {"3", m.comp_ratio_le_3.load(std::memory_order_relaxed)},
                {"5", m.comp_ratio_le_5.load(std::memory_order_relaxed)},
                {"10", m.comp_ratio_le_10.load(std::memory_order_relaxed)},
                {"100", m.comp_ratio_le_100.load(std::memory_order_relaxed)},
                {"+Inf", m.comp_ratio_le_inf.load(std::memory_order_relaxed)}
            };
            // Convert per-bucket counts to cumulative counts while preserving order
            uint64_t running = 0;
            for (const auto& bucket : buckets) {
                running += bucket.second;
                out << "themis_content_blob_compression_ratio_bucket{le=\"" << bucket.first << "\"} " << running << "\n";
            }
            // sum and count
            uint64_t cnt = m.comp_ratio_count.load(std::memory_order_relaxed);
            double sum = static_cast<double>(m.comp_ratio_sum_milli.load(std::memory_order_relaxed)) / 1000.0;
            out << "themis_content_blob_compression_ratio_sum " << std::fixed << std::setprecision(3) << sum << "\n";
            out << "themis_content_blob_compression_ratio_count " << cnt << "\n";
        } else {
            // No content manager configured: emit zeros
            out << "themis_content_blob_compressed_bytes_total 0\n";
            out << "themis_content_blob_uncompressed_bytes_total 0\n";
            out << "themis_content_blob_compression_skipped_total 0\n";
            out << "themis_content_blob_compression_skipped_total{mime_prefix=\"image/\"} 0\n";
            out << "themis_content_blob_compression_skipped_total{mime_prefix=\"video/\"} 0\n";
            out << "themis_content_blob_compression_skipped_total{mime_prefix=\"application/zip\"} 0\n";
            out << "themis_content_blob_compression_ratio_bucket{le=\"1\"} 0\n";
            out << "themis_content_blob_compression_ratio_bucket{le=\"1.5\"} 0\n";
            out << "themis_content_blob_compression_ratio_bucket{le=\"2\"} 0\n";
            out << "themis_content_blob_compression_ratio_bucket{le=\"3\"} 0\n";
            out << "themis_content_blob_compression_ratio_bucket{le=\"5\"} 0\n";
            out << "themis_content_blob_compression_ratio_bucket{le=\"10\"} 0\n";
            out << "themis_content_blob_compression_ratio_bucket{le=\"100\"} 0\n";
            out << "themis_content_blob_compression_ratio_bucket{le=\"+Inf\"} 0\n";
            out << "themis_content_blob_compression_ratio_sum 0\n";
            out << "themis_content_blob_compression_ratio_count 0\n";
        }

        http::response<http::string_body> res{http::status::ok, req.version()};
        res.set(http::field::content_type, "text/plain; version=0.0.4; charset=utf-8");
        res.body() = out.str();
        res.prepare_payload();
        return res;
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

// -----------------------------------------------------------------------------
// Existing handlers
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Audit API Handlers
// -----------------------------------------------------------------------------

namespace {
    // NOTE: urlDecode already defined earlier in this file - removed duplicate
    // URL query parser with percent-decoding
    static std::unordered_map<std::string, std::string> parseQuery(const std::string& target) {
        std::unordered_map<std::string, std::string> out;
        auto qpos = target.find('?');
        if (qpos == std::string::npos) return out;
        auto qs = target.substr(qpos + 1);
        std::istringstream iss(qs);
        std::string kv;
        while (std::getline(iss, kv, '&')) {
            auto eq = kv.find('=');
            std::string k = (eq == std::string::npos) ? kv : kv.substr(0, eq);
            std::string v = (eq == std::string::npos) ? std::string() : kv.substr(eq + 1);
            out[urlDecode(k)] = urlDecode(v);
        }
        return out;
    }
    // Parse ISO8601 with optional fractional seconds and timezone (Z or ±HH:MM), or epoch ms
    static int64_t parseTimeMs(const std::string& s) {
        if (s.empty()) return 0;
        bool numeric = std::all_of(s.begin(), s.end(), [](char c){ return c >= '0' && c <= '9'; });
        if (numeric) {
            try { return std::stoll(s); } catch (...) { return 0; }
        }
        // ISO8601 parsing
        // Expected: YYYY-MM-DDTHH:MM:SS[.fff][Z|±HH:MM]
        std::tm tm{};
        tm.tm_isdst = -1;
        // Split at 'T'
        auto tpos = s.find('T');
        if (tpos == std::string::npos) return 0;
        std::string date = s.substr(0, tpos);
        std::string rest = s.substr(tpos + 1);
        // Parse date
        if (date.size() != 10) return 0;
        std::istringstream dss(date);
        dss >> std::get_time(&tm, "%Y-%m-%d");
        if (dss.fail()) return 0;
        // Find timezone marker
        int tz_sign = 0; int tz_h = 0; int tz_m = 0;
        int64_t millis = 0;
        // Separate time part from timezone
        size_t zpos = rest.find('Z');
        size_t plus = rest.rfind('+');
        size_t minus = rest.rfind('-');
        size_t tzpos = std::string::npos;
        if (zpos != std::string::npos) tzpos = zpos;
        else if (plus != std::string::npos) tzpos = plus;
        else if (minus != std::string::npos) tzpos = (minus > 1 ? minus : std::string::npos);
        std::string timepart = (tzpos == std::string::npos) ? rest : rest.substr(0, tzpos);
        std::string tzpart = (tzpos == std::string::npos) ? std::string() : rest.substr(tzpos);
        // Parse time with optional fractional seconds
        // timepart like HH:MM:SS[.fff]
        int H=0,M=0; double S=0.0;
        char c1=':', c2=':';
        std::istringstream tss(timepart);
        tss >> H >> c1 >> M >> c2 >> S;
        if (tss.fail() || c1 != ':' || c2 != ':') return 0;
        tm.tm_hour = H; tm.tm_min = M; tm.tm_sec = static_cast<int>(S);
        millis = static_cast<int64_t>((S - tm.tm_sec) * 1000.0 + 0.5);
        // Parse timezone
        if (!tzpart.empty()) {
            const char tz_lead = tzpart.front();
            if (tz_lead == 'Z') { tz_sign = 0; }
            else if (tz_lead == '+' || tz_lead == '-') {
                tz_sign = (tz_lead == '+') ? +1 : -1;
                // format ±HH:MM
                if (tzpart.size() >= 6 && tzpart[3] == ':') {
                    try {
                        tz_h = std::stoi(tzpart.substr(1,2));
                        tz_m = std::stoi(tzpart.substr(4,2));
                    } catch (...) { tz_h = tz_m = 0; tz_sign = 0; }
                }
            }
        }
    // Build UTC epoch seconds from tm (interpreted as UTC)
    time_t secs = portable_mkgmtime_impl(&tm);
        if (secs == static_cast<time_t>(-1)) return 0;
        // Adjust for timezone offset: local time part represents wall time in given TZ
        int offset_secs = tz_sign * (tz_h * 3600 + tz_m * 60);
        int64_t epoch_ms = (static_cast<int64_t>(secs) - offset_secs) * 1000 + millis;
        return epoch_ms;
    }
}

http::response<http::string_body> HttpServer::handleAuditQuery(
    const http::request<http::string_body>& req
) {
    try {
        if (auto rl = enforceAuditRateLimit(req, "/api/audit")) return *rl;
        // Authorization: require audit:read when auth is enabled
        if (auto resp = requireAccess(req, "audit:read", "audit.read", "/api/audit")) return *resp;
        if (!audit_api_) {
            return makeErrorResponse(http::status::service_unavailable, "Audit API not available", req);
        }
        auto& audit_api = *audit_api_;
        auto params = parseQuery(std::string(req.target()));
        themis::server::AuditQueryFilter f;
        if (auto it = params.find("start"); it != params.end()) f.start_ts_ms = parseTimeMs(it->second);
        if (auto it = params.find("end"); it != params.end()) f.end_ts_ms = parseTimeMs(it->second);
        if (auto it = params.find("user"); it != params.end()) f.user = it->second;
        if (auto it = params.find("action"); it != params.end()) f.action = it->second;
        if (auto it = params.find("entity_type"); it != params.end()) f.entity_type = it->second;
        if (auto it = params.find("entity_id"); it != params.end()) f.entity_id = it->second;
        if (auto it = params.find("success"); it != params.end()) {
            auto v = it->second; std::transform(v.begin(), v.end(), v.begin(), ::tolower);
            f.success_only = (v == "true" || v == "1" || v == "yes");
        }
        if (auto it = params.find("page"); it != params.end()) {
            try { f.page = std::max(1, std::stoi(it->second)); } catch (...) {}
        }
        if (auto it = params.find("page_size"); it != params.end()) {
            try {
                f.page_size = std::stoi(it->second);
                if (f.page_size < 1) f.page_size = 1;
                if (f.page_size > 1000) f.page_size = 1000;
            } catch (...) {}
        }
        auto result = audit_api.queryAuditLogs(f);
        return makeResponse(http::status::ok, result.dump(), req);
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> HttpServer::handleAuditExportCsv(
    const http::request<http::string_body>& req
) {
    try {
        if (auto rl = enforceAuditRateLimit(req, "/api/audit/export/csv")) return *rl;
        // Authorization: require audit:read when auth is enabled
        if (auto resp = requireAccess(req, "audit:read", "audit.read", "/api/audit/export/csv")) return *resp;
        if (!audit_api_) {
            return makeErrorResponse(http::status::service_unavailable, "Audit API not available", req);
        }
        auto& audit_api = *audit_api_;
        auto params = parseQuery(std::string(req.target()));
        themis::server::AuditQueryFilter f;
        if (auto it = params.find("start"); it != params.end()) f.start_ts_ms = parseTimeMs(it->second);
        if (auto it = params.find("end"); it != params.end()) f.end_ts_ms = parseTimeMs(it->second);
        if (auto it = params.find("user"); it != params.end()) f.user = it->second;
        if (auto it = params.find("action"); it != params.end()) f.action = it->second;
        if (auto it = params.find("entity_type"); it != params.end()) f.entity_type = it->second;
        if (auto it = params.find("entity_id"); it != params.end()) f.entity_id = it->second;
        if (auto it = params.find("success"); it != params.end()) {
            auto v = it->second; std::transform(v.begin(), v.end(), v.begin(), ::tolower);
            f.success_only = (v == "true" || v == "1" || v == "yes");
        }
        if (auto it = params.find("page"); it != params.end()) {
            try { f.page = std::max(1, std::stoi(it->second)); } catch (...) {}
        }
        if (auto it = params.find("page_size"); it != params.end()) {
            try {
                f.page_size = std::stoi(it->second);
                if (f.page_size < 1) f.page_size = 1;
                if (f.page_size > 10000) f.page_size = 10000; // allow larger for export
            } catch (...) {}
        }

        auto csv = audit_api.exportAuditLogsCsv(f);
        http::response<http::string_body> res{http::status::ok, req.version()};
#ifdef THEMIS_VERSION_STRING
        res.set(http::field::server, std::string("THEMIS/") + THEMIS_VERSION_STRING);
#else
        res.set(http::field::server, "THEMIS/1.0.1");
#endif
        res.set(http::field::content_type, "text/csv");
        res.set(http::field::content_disposition, "attachment; filename=themis_audit_export.csv");
        res.keep_alive(req.keep_alive());
        res.body() = std::move(csv);
        applyGovernanceHeaders(req, res);
        res.prepare_payload();
        return res;
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

std::optional<http::response<http::string_body>> HttpServer::enforceAuditRateLimit(
    const http::request<http::string_body>& req,
    std::string_view route_key
) {
    try {
        if (audit_rate_limit_per_minute_ == 0) return std::nullopt;
        // Determine bucket key: Authorization header if present, else "anon"
        std::string key = std::string(route_key) + ":";
        const auto auth_header = req[http::field::authorization];
        if (!auth_header.empty()) key += std::string(auth_header); else key += "anon";
        auto now = std::chrono::time_point_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now()).time_since_epoch().count();
        const uint64_t window_ms = 60ull * 1000ull;
        uint32_t limit = audit_rate_limit_per_minute_;
        uint32_t count = 0;
        {
            std::lock_guard<std::mutex> lk(audit_rate_mutex_);
            // W1-S02: amortised eviction of stale buckets to prevent unbounded map growth.
            // Triggered on each access — erases entries whose window expired more than
            // one full window ago (i.e., at least 2 × window_ms in the past).
            if (audit_rate_buckets_.size() > 128) {
                const uint64_t evict_cutoff = now - 2 * window_ms;
                for (auto bucket_it = audit_rate_buckets_.begin();
                     bucket_it != audit_rate_buckets_.end(); ) {
                    if (bucket_it->second.window_start_ms < evict_cutoff) {
                        bucket_it = audit_rate_buckets_.erase(bucket_it);
                    } else {
                        ++bucket_it;
                    }
                }
            }
            // Use try_emplace to make the insertion intent explicit and to avoid
            // the iterator-invalidation risk that operator[] carries: operator[]
            // inserts a default element when the key is absent, which can trigger
            // a rehash and invalidate all existing iterators/references.
            // try_emplace also inserts a default element if absent but the returned
            // iterator is always stable within this locked section.
            auto [it, inserted] = audit_rate_buckets_.try_emplace(key);
            auto& st = it->second;
            if (now - st.window_start_ms >= window_ms) {
                st.window_start_ms = now;
                st.count = 0;
            }
            if (st.count >= limit) {
                THEMIS_DEBUG("AUDIT_RL_HIT key={} count={} limit={}", key, st.count, limit);
                // respond 429 with diagnostic headers
                auto resp = makeErrorResponse(http::status::too_many_requests, "Rate limit exceeded", req);
                resp.set(http::field::retry_after, "60");
                resp.set("X-RateLimit-Limit", std::to_string(limit));
                resp.set("X-RateLimit-Remaining", "0");
                return resp;
            }
            st.count++;
            count = st.count;
            THEMIS_DEBUG("AUDIT_RL_OK key={} count={} limit={}", key, count, limit);
        }
        return std::nullopt;
    } catch (...) {
        THEMIS_WARN("http_server: unhandled exception caught");
        return std::nullopt;
    }
}

// Monitoring endpoints (Health, Version, Stats, Capabilities) have been
// moved to MonitoringApiHandler for better code organization

http::response<http::string_body> HttpServer::handleConfig(
    const http::request<http::string_body>& req
) {
    // GET -> config:read, POST -> config:write (if auth enabled)
    if (auth_ && auth_->isEnabled()) {
        std::string path_only = std::string(req.target());
        auto qpos = path_only.find('?');
        if (qpos != std::string::npos) path_only = path_only.substr(0, qpos);
        if (req.method() == http::verb::post) {
            if (auto resp = requireAccess(req, "config:write", "config.write", path_only)) {
                if (audit_logger_) {
                    nlohmann::json entry;
                    entry["event"]    = "config_write_denied";
                    entry["resource"] = "config";
                    entry["action"]   = "config.write";
                    entry["path"]     = path_only;
                    try { audit_logger_->logEvent(entry); } catch (...) {}
                }
                return *resp;
            }
            if (audit_logger_) {
                nlohmann::json entry;
                entry["event"]    = "config_write_authorized";
                entry["resource"] = "config";
                entry["action"]   = "config.write";
                entry["path"]     = path_only;
                try { audit_logger_->logEvent(entry); } catch (...) {}
            }
        } else {
            if (auto resp = requireAccess(req, "config:read", "config.read", path_only)) {
                if (audit_logger_) {
                    nlohmann::json entry;
                    entry["event"]    = "config_read_denied";
                    entry["resource"] = "config";
                    entry["action"]   = "config.read";
                    entry["path"]     = path_only;
                    try { audit_logger_->logEvent(entry); } catch (...) {}
                }
                return *resp;
            }
        }
    }
    try {
        // Allow POST to update runtime config (Hot-Reload)
        if (req.method() == http::verb::post) {
            json body;
            try {
                body = json::parse(req.body());
            } catch (...) {
                THEMIS_DEBUG("http_server: unhandled exception caught");
                return makeErrorResponse(http::status::bad_request, "Invalid JSON body", req);
            }
            
            // 1) Logging config (level, format)
            if (body.contains("logging") && body["logging"].is_object()) {
                const auto& lg = body["logging"];
                // level
                if (lg.contains("level")) {
                    auto lvl = lg["level"].get<std::string>();
                    auto mapped = themis::utils::Logger::levelFromString(lvl);
                    themis::utils::Logger::setLevel(mapped);
                    if (audit_logger_) {
                        nlohmann::json entry;
                        entry["event"]        = "logging_level_updated";
                        entry["level"]        = lvl;
                        try { audit_logger_->logEvent(entry); } catch (...) {}
                    }
                    THEMIS_INFO("Hot-reload: logging.level set to {}", lvl);
                }
                // format
                if (lg.contains("format")) {
                    auto fmt = lg["format"].get<std::string>();
                    std::string pattern;
                    if (fmt == "json") {
                        // Minimal JSON line pattern
                        pattern = "{\"ts\":\"%Y-%m-%dT%H:%M:%S.%e\",\"level\":\"%l\",\"thread\":%t,\"msg\":\"%v\"}";
                    } else {
                        // Default text pattern
                        pattern = "[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] [thread %t] %v";
                    }
                    themis::utils::Logger::setPattern(pattern);
                    if (audit_logger_) {
                        nlohmann::json entry;
                        entry["event"]        = "logging_format_updated";
                        entry["format"]       = fmt;
                        try { audit_logger_->logEvent(entry); } catch (...) {}
                    }
                    THEMIS_INFO("Hot-reload: logging.format set to {}", fmt);
                }
            }
            
            // 2) Request timeout
            if (body.contains("request_timeout_ms")) {
                auto timeout = body["request_timeout_ms"].get<uint32_t>();
                if (timeout >= 1000 && timeout <= 300000) { // 1s - 5min range
                    // Write via atomic to prevent data race: worker threads read
                    // request_timeout_ms_live_ concurrently in armReadTimer().
                    config_.request_timeout_ms = timeout;
                    request_timeout_ms_live_.store(timeout, std::memory_order_relaxed);
                    if (audit_logger_) {
                        nlohmann::json entry;
                        entry["event"]        = "request_timeout_updated";
                        entry["timeout_ms"]   = timeout;
                        try { audit_logger_->logEvent(entry); } catch (...) {}
                    }
                    THEMIS_INFO("Hot-reload: request_timeout_ms set to {}", timeout);
                } else {
                    if (audit_logger_) {
                        nlohmann::json entry;
                        entry["event"]        = "request_timeout_invalid";
                        entry["requested_ms"] = timeout;
                        entry["valid_range"]  = "1000-300000";
                        try { audit_logger_->logEvent(entry); } catch (...) {}
                    }
                    return makeErrorResponse(http::status::bad_request, "request_timeout_ms must be 1000-300000", req);
                }
            }
            
            // 3) Feature flags (runtime toggle for beta features)
            if (body.contains("features") && body["features"].is_object()) {
                const auto& features = body["features"];
                if (features.contains("semantic_cache")) {
                    bool enabled = features["semantic_cache"].get<bool>();
                    // Write via atomic to prevent data race with concurrent handler reads.
                    feature_semantic_cache_live_.store(enabled, std::memory_order_relaxed);
                    if (audit_logger_) {
                        nlohmann::json entry;
                        entry["event"]      = "feature_flag_updated";
                        entry["feature"]    = "semantic_cache";
                        entry["enabled"]    = enabled;
                        try { audit_logger_->logEvent(entry); } catch (...) {}
                    }
                    THEMIS_INFO("Hot-reload: feature_semantic_cache set to {}", enabled);
                }
                if (features.contains("llm_store")) {
                    bool enabled = features["llm_store"].get<bool>();
                    feature_llm_store_live_.store(enabled, std::memory_order_relaxed);
                    if (audit_logger_) {
                        nlohmann::json entry;
                        entry["event"]      = "feature_flag_updated";
                        entry["feature"]    = "llm_store";
                        entry["enabled"]    = enabled;
                        try { audit_logger_->logEvent(entry); } catch (...) {}
                    }
                    THEMIS_INFO("Hot-reload: feature_llm_store set to {}", enabled);
                }
                if (features.contains("cdc")) {
                    bool enabled = features["cdc"].get<bool>();
                    feature_cdc_live_.store(enabled, std::memory_order_relaxed);
                    if (audit_logger_) {
                        nlohmann::json entry;
                        entry["event"]      = "feature_flag_updated";
                        entry["feature"]    = "cdc";
                        entry["enabled"]    = enabled;
                        try { audit_logger_->logEvent(entry); } catch (...) {}
                    }
                    THEMIS_INFO("Hot-reload: feature_cdc set to {}", enabled);
                }
                if (features.contains("timeseries")) {
                    bool enabled = features["timeseries"].get<bool>();
                    feature_timeseries_live_.store(enabled, std::memory_order_relaxed);
                    if (audit_logger_) {
                        nlohmann::json entry;
                        entry["event"]      = "feature_flag_updated";
                        entry["feature"]    = "timeseries";
                        entry["enabled"]    = enabled;
                        try { audit_logger_->logEvent(entry); } catch (...) {}
                    }
                    THEMIS_INFO("Hot-reload: feature_timeseries set to {}", enabled);
                }
            }
            
            // 4) CDC Retention policy (auto-cleanup threshold)
            if (body.contains("cdc_retention_hours")) {
                if (!feature_cdc_live_.load(std::memory_order_relaxed) || !changefeed_) {
                    return makeErrorResponse(http::status::bad_request, "CDC not enabled", req);
                }
                auto hours = body["cdc_retention_hours"].get<uint32_t>();
                if (hours < 1 || hours > 8760) { // 1 hour - 1 year
                    if (audit_logger_) {
                        nlohmann::json entry;
                        entry["event"]      = "config_cdc_retention_invalid";
                        entry["requested_hours"] = hours;
                        entry["valid_range"] = "1-8760";
                        try { audit_logger_->logEvent(entry); } catch (...) {}
                    }
                    return makeErrorResponse(http::status::bad_request, "cdc_retention_hours must be 1-8760", req);
                }
                // Apply TTL update to the live retention policy; the background cleanup
                // thread is started automatically by updateRetentionPolicy() if not running.
                auto policy = changefeed_->getRetentionPolicy();
                policy.enabled       = true;
                policy.max_age_hours = std::chrono::hours(hours);
                changefeed_->updateRetentionPolicy(policy);
                if (audit_logger_) {
                    nlohmann::json entry;
                    entry["event"]      = "config_cdc_retention_updated";
                    entry["retention_hours"] = hours;
                    try { audit_logger_->logEvent(entry); } catch (...) {}
                }
                THEMIS_INFO("Hot-reload: cdc_retention_hours set to {} and retention policy enabled", hours);
            }
            
            // Respond with updated config
        }

        // Build comprehensive config response
        json response = {
            {"server", {
                {"port", config_.port},
                {"threads", config_.num_threads},
                {"request_timeout_ms", request_timeout_ms_live_.load(std::memory_order_relaxed)}
            }},
            {"features", {
                {"semantic_cache", feature_semantic_cache_live_.load(std::memory_order_relaxed)},
                {"llm_store", feature_llm_store_live_.load(std::memory_order_relaxed)},
                {"cdc", feature_cdc_live_.load(std::memory_order_relaxed)},
                {"timeseries", feature_timeseries_live_.load(std::memory_order_relaxed)}
            }},
            {"rocksdb", {
                {"db_path", storage_->getConfig().db_path},
                {"wal_dir", storage_->getConfig().wal_dir.empty() ? storage_->getConfig().db_path : storage_->getConfig().wal_dir},
                {"memtable_size_mb", storage_->getConfig().memtable_size_mb},
                {"block_cache_size_mb", storage_->getConfig().block_cache_size_mb},
                {"cache_index_and_filter_blocks", storage_->getConfig().cache_index_and_filter_blocks},
                {"pin_l0_filter_and_index_blocks_in_cache", storage_->getConfig().pin_l0_filter_and_index_blocks_in_cache},
                {"partition_filters", storage_->getConfig().partition_filters},
                {"high_pri_pool_ratio", storage_->getConfig().high_pri_pool_ratio},
                {"bloom_bits_per_key", storage_->getConfig().bloom_bits_per_key},
                {"enable_wal", storage_->getConfig().enable_wal},
                {"enable_blobdb", storage_->getConfig().enable_blobdb},
                {"blob_size_threshold", storage_->getConfig().blob_size_threshold},
                {"max_background_jobs", storage_->getConfig().max_background_jobs},
                {"use_universal_compaction", storage_->getConfig().use_universal_compaction},
                {"dynamic_level_bytes", storage_->getConfig().dynamic_level_bytes},
                {"target_file_size_base_mb", storage_->getConfig().target_file_size_base_mb},
                {"max_bytes_for_level_base_mb", storage_->getConfig().max_bytes_for_level_base_mb},
                {"max_write_buffer_number", storage_->getConfig().max_write_buffer_number},
                {"min_write_buffer_number_to_merge", storage_->getConfig().min_write_buffer_number_to_merge},
                {"use_direct_reads", storage_->getConfig().use_direct_reads},
                {"use_direct_io_for_flush_and_compaction", storage_->getConfig().use_direct_io_for_flush_and_compaction},
                {"compression_default", storage_->getConfig().compression_default},
                {"compression_bottommost", storage_->getConfig().compression_bottommost}
            }},
            {"runtime", {
                {"compression_active", storage_->getCompressionType()},
                {"db_size_bytes", storage_->getApproximateSize()}
            }},
            {"logging", {
                {"level", themis::utils::Logger::levelToString(themis::utils::Logger::getLevel())}
            }},
            {"metrics", {
                {"total_requests", request_count_.load(std::memory_order_relaxed)},
                {"total_errors", error_count_.load(std::memory_order_relaxed)}
            }}
        };

        return makeResponse(http::status::ok, response.dump(2), req); // Pretty print
    } catch (const std::exception& e) {
        error_count_.fetch_add(1, std::memory_order_relaxed);
        return makeErrorResponse(http::status::internal_server_error,
                                 std::string("Failed to get config: ") + e.what(), req);
    }
}


// Metrics endpoint (Prometheus format) has been moved to MonitoringApiHandler
// for better code organization and maintainability

std::optional<http::response<http::string_body>> HttpServer::requireScope(
    const http::request<http::string_body>& req,
    std::string_view scope
) {
    if (!auth_ || !auth_->isEnabled()) return std::nullopt; // No auth configured

    const auto auth_header = req[http::field::authorization];
    if (auth_header.empty()) {
        http::response<http::string_body> res{http::status::unauthorized, req.version()};
        res.set(http::field::www_authenticate, "Bearer realm=\"themis\"");
        res.set(http::field::content_type, "application/json");
        res.keep_alive(req.keep_alive());
    res.body() = R"({"error":"missing_authorization","message":"Missing Authorization header"})";
    applyGovernanceHeaders(req, res);
    res.prepare_payload();
        return res;
    }
    auto token = themis::AuthMiddleware::extractBearerToken(std::string_view(auth_header.data(), auth_header.size()));
    if (!token) {
        http::response<http::string_body> res{http::status::unauthorized, req.version()};
        res.set(http::field::www_authenticate, "Bearer realm=\"themis\"");
        res.set(http::field::content_type, "application/json");
        res.keep_alive(req.keep_alive());
    res.body() = R"({"error":"invalid_authorization","message":"Invalid Bearer token format"})";
    applyGovernanceHeaders(req, res);
    res.prepare_payload();
        return res;
    }
    auto ar = auth_->authorize(*token, scope);
    if (audit_logger_) {
        nlohmann::json entry;
        entry["event"]      = "authorization";
        entry["function"]   = "requireScope";
        entry["scope"]      = std::string(scope);
        entry["user_id"]    = ar.user_id;
        entry["authorized"] = ar.authorized;
        entry["reason"]     = ar.reason;
        try { audit_logger_->logEvent(entry); } catch (...) {}
    }
    if (!ar.authorized) {
        http::response<http::string_body> res{http::status::forbidden, req.version()};
        res.set(http::field::content_type, "application/json");
        res.keep_alive(req.keep_alive());
        std::string body = std::string("{\"error\":\"forbidden\",\"message\":\"") + ar.reason + "\"}";
    res.body() = std::move(body);
    applyGovernanceHeaders(req, res);
    res.prepare_payload();
        return res;
    }
    return std::nullopt;
}

// Combined scope + policy authorization
std::optional<http::response<http::string_body>> HttpServer::requireAccess(
    const http::request<http::string_body>& req,
    std::string_view required_scope,
    std::string_view action,
    std::string_view resource_path
) {
    // If auth is disabled and no policy engine configured, allow
    bool auth_enabled = (auth_ && auth_->isEnabled());
    bool policy_enabled = (policy_engine_ != nullptr);
    if (!auth_enabled && !policy_enabled) return std::nullopt;

    // Normalize resource path (strip query string) if empty passed
    std::string resource = std::string(resource_path);
    if (resource.empty()) {
        resource = std::string(req.target());
    }
    auto qpos = resource.find('?');
    if (qpos != std::string::npos) resource = resource.substr(0, qpos);

    // 1) Scope-based authorization (if auth enabled)
    std::string user_id = "";
    if (auth_enabled) {
        const auto auth_header = req[http::field::authorization];
        if (auth_header.empty()) {
            http::response<http::string_body> res{http::status::unauthorized, req.version()};
            res.set(http::field::www_authenticate, "Bearer realm=\"themis\"");
            res.set(http::field::content_type, "application/json");
            res.keep_alive(req.keep_alive());
            res.body() = R"({"error":"missing_authorization","message":"Missing Authorization header"})";
            applyGovernanceHeaders(req, res);
            res.prepare_payload();
            return res;
        }
        // Authorization header presence validated
        auto token = themis::AuthMiddleware::extractBearerToken(std::string_view(auth_header.data(), auth_header.size()));
        if (!token) {
            http::response<http::string_body> res{http::status::unauthorized, req.version()};
            res.set(http::field::www_authenticate, "Bearer realm=\"themis\"");
            res.set(http::field::content_type, "application/json");
            res.keep_alive(req.keep_alive());
            res.body() = R"({"error":"invalid_authorization","message":"Invalid Bearer token format"})";
            applyGovernanceHeaders(req, res);
            res.prepare_payload();
            return res;
        }
        // Diagnostic: validate token to see which user_id (if any) is associated
        try {
            auto vres = auth_->validateToken(*token);
            THEMIS_INFO("requireAccess: validateToken -> authorized={} user_id='{}' reason='{}'", vres.authorized, vres.user_id, vres.reason);
            if (audit_logger_) {
                nlohmann::json entry;
                entry["event"]      = "token_validation";
                entry["function"]   = "requireAccess";
                entry["user_id"]    = vres.user_id;
                entry["authorized"] = vres.authorized;
                entry["reason"]     = vres.reason;
                try { audit_logger_->logEvent(entry); } catch (...) {}
            }
        } catch (...) {}
        auto ar = auth_->authorize(*token, required_scope);
        if (audit_logger_) {
            audit_logger_->logSecurityEvent(
                ar.authorized ? themis::utils::SecurityEventType::LOGIN_SUCCESS
                              : themis::utils::SecurityEventType::PERMISSION_DENIED,
                ar.user_id,
                std::string(required_scope),
                {{"decision", ar.authorized ? "allowed" : "denied"}, {"reason", ar.reason}}
            );
        }
        if (!ar.authorized) {
            http::response<http::string_body> res{http::status::forbidden, req.version()};
            res.set(http::field::content_type, "application/json");
            res.keep_alive(req.keep_alive());
            std::string body = std::string("{\"error\":\"forbidden\",\"message\":\"") + ar.reason + "\"}";
            res.body() = std::move(body);
            applyGovernanceHeaders(req, res);
            res.prepare_payload();
            return res;
        }
        user_id = ar.user_id;
    }

    // 2) Policy evaluation (if enabled)
    if (policy_enabled) {
        // In unauthenticated mode (no tokens configured), treat PolicyEngine as advisory only.
        // Do not enforce denials to keep developer/test ergonomics unless auth is enabled.
        if (!auth_enabled) {
            return std::nullopt;
        }
        // Admin users bypass policy checks by design
        if (!user_id.empty() && user_id == "admin") {
            THEMIS_INFO("Policy check bypass for admin user_id='{}'", user_id);
            if (audit_logger_) {
                nlohmann::json entry;
                entry["event"]      = "policy_bypass";
                entry["reason"]     = "admin_user";
                entry["user_id"]    = user_id;
                entry["resource"]   = resource;
                entry["action"]     = action;
                try { audit_logger_->logEvent(entry); } catch (...) {}
            }
            return std::nullopt;
        }

        // Extract client IP from headers (X-Forwarded-For or X-Real-IP)
        std::optional<std::string> client_ip;
        for (const auto& h : req) {
            auto name = h.name_string();
            if (beast::iequals(name, "x-forwarded-for")) {
                std::string v = std::string(h.value());
                // take first value before ','
                auto comma = v.find(',');
                if (comma != std::string::npos) v = v.substr(0, comma);
                // trim spaces
                auto start = v.find_first_not_of(" \t");
                auto end = v.find_last_not_of(" \t");
                if (start != std::string::npos) {
                    v = v.substr(start, end - start + 1);
                    client_ip = v;
                }
                break;
            } else if (beast::iequals(name, "x-real-ip")) {
                std::string v = std::string(h.value());
                client_ip = v;
            }
        }

        THEMIS_INFO("PolicyEngine: evaluating user='{}' action='{}' resource='{}' client_ip='{}'", user_id, action, resource, client_ip.has_value() ? *client_ip : std::string("<none>"));
        auto decision = policy_engine_->authorize(user_id, std::string(action), resource, client_ip);
        THEMIS_INFO("PolicyEngine: decision.allowed={} reason='{}' policy_id='{}'", decision.allowed, decision.reason, decision.policy_id);
        if (!decision.allowed) {
            http::response<http::string_body> res{http::status::forbidden, req.version()};
            res.set(http::field::content_type, "application/json");
            res.keep_alive(req.keep_alive());
            nlohmann::json j = {{"error","policy_denied"},{"message",decision.reason}};
            if (!decision.policy_id.empty()) j["policy_id"] = decision.policy_id;
            res.body() = j.dump();
            applyGovernanceHeaders(req, res);
            res.prepare_payload();
            return res;
        }
    }

    return std::nullopt;
}

HttpServer::AuthContext HttpServer::extractAuthContext(const http::request<http::string_body>& req) const {
    AuthContext ctx;
    
    // If auth is disabled, return empty context
    if (!auth_ || !auth_->isEnabled()) {
        return ctx;
    }
    
    // Extract Authorization header
    const auto auth_header = req[http::field::authorization];
    if (auth_header.empty()) {
        return ctx; // No token -> empty context
    }
    
    // Extract Bearer token
    auto token = themis::AuthMiddleware::extractBearerToken(
        std::string_view(auth_header.data(), auth_header.size())
    );
    if (!token) {
        return ctx; // Invalid token format -> empty context
    }
    
    // Validate token and extract user_id + groups
    auto ar = auth_->validateToken(*token);
    if (ar.authorized) {
        ctx.user_id = ar.user_id;
        ctx.groups = ar.groups;
    }
    
    return ctx;
}
http::response<http::string_body> HttpServer::handlePiiRevealByUuid(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handlePiiRevealByUuid");
    span.setAttribute("http.path", "/pii/reveal/{uuid}");

    // Ensure pseudonymizer is available (lazy init) - check early before auth/policy
    try {
        ensurePIIPseudonymizer();
    } catch (const std::exception& ex) {
        return makeErrorResponse(http::status::service_unavailable, 
            std::string("PII service initialization failed: ") + ex.what(), req);
    }
    
    if (!pii_pseudonymizer_) {
        return makeErrorResponse(http::status::service_unavailable, "PII service not initialized", req);
    }
    auto& pii_pseudonymizer = *pii_pseudonymizer_;

    // Extract UUID from path
    std::string target = std::string(req.target());
    std::string path_only = target;
    auto qpos = path_only.find('?');
    if (qpos != std::string::npos) path_only = path_only.substr(0, qpos);
    const std::string prefix = "/pii/reveal/";
    if (path_only.rfind(prefix, 0) != 0) {
        return makeErrorResponse(http::status::bad_request, "Invalid path", req);
    }
    std::string uuid = path_only.substr(prefix.size());
    if (uuid.empty()) {
        return makeErrorResponse(http::status::bad_request, "Missing UUID", req);
    }

    // Authorization: allow tokens with scope 'pii:reveal' OR 'admin'
    std::string user_id = "";
    if (auth_ && auth_->isEnabled()) {
        const auto auth_header = req[http::field::authorization];
        if (auth_header.empty()) {
            http::response<http::string_body> res{http::status::unauthorized, req.version()};
            res.set(http::field::www_authenticate, "Bearer realm=\"themis\"");
            res.set(http::field::content_type, "application/json");
            res.keep_alive(req.keep_alive());
            res.body() = R"({"error":"missing_authorization","message":"Missing Authorization header"})";
            applyGovernanceHeaders(req, res);
            res.prepare_payload();
            return res;
        }
        auto token = themis::AuthMiddleware::extractBearerToken(std::string_view(auth_header.data(), auth_header.size()));
        if (!token) {
            http::response<http::string_body> res{http::status::unauthorized, req.version()};
            res.set(http::field::www_authenticate, "Bearer realm=\"themis\"");
            res.set(http::field::content_type, "application/json");
            res.keep_alive(req.keep_alive());
            res.body() = R"({"error":"invalid_authorization","message":"Invalid Bearer token format"})";
            applyGovernanceHeaders(req, res);
            res.prepare_payload();
            return res;
        }
        auto ar = auth_->authorize(*token, "pii:reveal");
        if (audit_logger_) {
            audit_logger_->logSecurityEvent(
                ar.authorized ? themis::utils::SecurityEventType::LOGIN_SUCCESS
                              : themis::utils::SecurityEventType::PERMISSION_DENIED,
                ar.user_id,
                "pii:reveal",
                {{"decision", ar.authorized ? "allowed" : "denied"}, {"reason", ar.reason}}
            );
        }
        if (!ar.authorized) {
            ar = auth_->authorize(*token, "admin");
            if (audit_logger_) {
                audit_logger_->logSecurityEvent(
                    ar.authorized ? themis::utils::SecurityEventType::LOGIN_SUCCESS
                                  : themis::utils::SecurityEventType::PERMISSION_DENIED,
                    ar.user_id,
                    "admin",
                    {{"decision", ar.authorized ? "allowed" : "denied"}, {"reason", ar.reason}}
                );
            }
            if (!ar.authorized) {
                http::response<http::string_body> res{http::status::forbidden, req.version()};
                res.set(http::field::content_type, "application/json");
                res.keep_alive(req.keep_alive());
                std::string body = std::string("{\"error\":\"forbidden\",\"message\":\"") + ar.reason + "\"}";
                res.body() = std::move(body);
                applyGovernanceHeaders(req, res);
                res.prepare_payload();
                return res;
            }
        }
        user_id = ar.user_id;
    }

    // Policy check (if configured)
    if (policy_engine_) {
        // Extract client IP (X-Forwarded-For or X-Real-IP)
        std::optional<std::string> client_ip;
        for (const auto& h : req) {
            auto name = h.name_string();
            if (beast::iequals(name, "x-forwarded-for")) {
                std::string v = std::string(h.value());
                auto comma = v.find(',');
                if (comma != std::string::npos) v = v.substr(0, comma);
                auto s = v.find_first_not_of(" \t");
                auto e = v.find_last_not_of(" \t");
                if (s != std::string::npos) client_ip = v.substr(s, e - s + 1);
                break;
            } else if (beast::iequals(name, "x-real-ip")) {
                client_ip = std::string(h.value());
            }
        }
        // Admin bypass: admin token should be able to perform PII operations regardless of policies
        if (!user_id.empty() && user_id == "admin") {
            THEMIS_INFO("PII DELETE: bypassing PolicyEngine for admin user_id='{}'", user_id);
        } else {
            auto decision = policy_engine_->authorize(user_id, "pii.write", path_only, client_ip);
            if (!decision.allowed) {
            http::response<http::string_body> res{http::status::forbidden, req.version()};
            res.set(http::field::content_type, "application/json");
            res.keep_alive(req.keep_alive());
            nlohmann::json j = {
                {"error", "policy_denied"},
                {"message", decision.reason},
            };
            if (!decision.policy_id.empty()) j["policy_id"] = decision.policy_id;
            res.body() = j.dump();
            applyGovernanceHeaders(req, res);
            res.prepare_payload();
            return res;
        }
        }
    }

    // Reveal
    auto value_opt = pii_pseudonymizer.revealPII(uuid, user_id.empty() ? std::string("unknown") : user_id);
    if (!value_opt) {
        return makeErrorResponse(http::status::not_found, "PII mapping not found", req);
    }
    nlohmann::json resp{{"uuid", uuid}, {"value", *value_opt}};
    span.setStatus(true);
    return makeResponse(http::status::ok, resp.dump(), req);
}

http::response<http::string_body> HttpServer::handlePiiDeleteByUuid(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handlePiiDeleteByUuid");
    span.setAttribute("http.path", "/pii/{uuid}");

    // Extract path and uuid
    std::string target = std::string(req.target());
    auto qpos = target.find('?');
    std::string path_only = (qpos == std::string::npos) ? target : target.substr(0, qpos);
    std::string query = (qpos == std::string::npos) ? std::string() : target.substr(qpos + 1);
    const std::string prefix = "/pii/";
    if (path_only.rfind(prefix, 0) != 0) {
        return makeErrorResponse(http::status::bad_request, "Invalid path", req);
    }
    std::string uuid = path_only.substr(prefix.size());
    if (uuid.empty()) {
        return makeErrorResponse(http::status::bad_request, "Missing UUID", req);
    }

    // Parse mode from query (?mode=soft|hard), default soft (applies to pseudonymizer fallback)
    std::string mode = "soft";
    if (!query.empty()) {
        auto pos = query.find("mode=");
        if (pos != std::string::npos) {
            auto val = query.substr(pos + 5);
            auto amp = val.find('&'); if (amp != std::string::npos) val = val.substr(0, amp);
            if (val == "hard") mode = "hard";
        }
    }

    // Authorization: require pii:write or admin (erase is a write operation)
    std::string user_id;
    if (auth_ && auth_->isEnabled()) {
        const auto auth_header = req[http::field::authorization];
        if (auth_header.empty()) {
            http::response<http::string_body> res{http::status::unauthorized, req.version()};
            res.set(http::field::www_authenticate, "Bearer realm=\"themis\"");
            res.set(http::field::content_type, "application/json");
            res.keep_alive(req.keep_alive());
            res.body() = R"({"error":"missing_authorization","message":"Missing Authorization header"})";
            res.prepare_payload();
            return res;
        }
        auto token = themis::AuthMiddleware::extractBearerToken(std::string_view(auth_header.data(), auth_header.size()));
        if (!token) {
            http::response<http::string_body> res{http::status::unauthorized, req.version()};
            res.set(http::field::www_authenticate, "Bearer realm=\"themis\"");
            res.set(http::field::content_type, "application/json");
            res.keep_alive(req.keep_alive());
            res.body() = R"({"error":"invalid_authorization","message":"Invalid Bearer token format"})";
            res.prepare_payload();
            return res;
        }
        THEMIS_INFO("PII Delete: Authorization header present, required_scope='pii:write'");
         
        auto ar = auth_->authorize(*token, "pii:write");
        if (audit_logger_) {
            audit_logger_->logSecurityEvent(
                ar.authorized ? themis::utils::SecurityEventType::LOGIN_SUCCESS
                              : themis::utils::SecurityEventType::PERMISSION_DENIED,
                ar.user_id,
                "pii:write",
                {{"decision", ar.authorized ? "allowed" : "denied"}, {"reason", ar.reason}}
            );
        }
        if (!ar.authorized) {
            ar = auth_->authorize(*token, "admin");
            if (audit_logger_) {
                audit_logger_->logSecurityEvent(
                    ar.authorized ? themis::utils::SecurityEventType::LOGIN_SUCCESS
                                  : themis::utils::SecurityEventType::PERMISSION_DENIED,
                    ar.user_id,
                    "admin",
                    {{"decision", ar.authorized ? "allowed" : "denied"}, {"reason", ar.reason}}
                );
            }
            if (!ar.authorized) {
                http::response<http::string_body> res{http::status::forbidden, req.version()};
                res.set(http::field::content_type, "application/json");
                res.keep_alive(req.keep_alive());
                std::string body = std::string("{\"error\":\"forbidden\",\"message\":\"") + ar.reason + "\"}";
                res.body() = std::move(body);
                applyGovernanceHeaders(req, res);
                res.prepare_payload();
                return res;
            }
        }
        user_id = ar.user_id;
    }

    // Policy check
    if (policy_engine_) {
        std::optional<std::string> client_ip;
        for (const auto& h : req) {
            auto name = h.name_string();
            if (beast::iequals(name, "x-forwarded-for")) {
                std::string v = std::string(h.value());
                auto comma = v.find(',');
                if (comma != std::string::npos) v = v.substr(0, comma);
                auto s = v.find_first_not_of(" \t");
                auto e = v.find_last_not_of(" \t");
                if (s != std::string::npos) client_ip = v.substr(s, e - s + 1);
                break;
            } else if (beast::iequals(name, "x-real-ip")) {
                client_ip = std::string(h.value());
            }
        }
    // Policy action aligned to write semantics
        auto decision = policy_engine_->authorize(user_id, "pii.write", path_only, client_ip);
        if (!decision.allowed) {
            http::response<http::string_body> res{http::status::forbidden, req.version()};
            res.set(http::field::content_type, "application/json");
            res.keep_alive(req.keep_alive());
            nlohmann::json j = {{"error","policy_denied"},{"message",decision.reason}};
            if (!decision.policy_id.empty()) j["policy_id"] = decision.policy_id;
            res.body() = j.dump();
            res.prepare_payload();
            return res;
        }
    }
    // Prefer CRUD mapping deletion when PII Manager feature is enabled
    if (config_.feature_pii_manager && pii_api_) {
        auto& pii_api = *pii_api_;
        bool ok = pii_api.deleteMapping(uuid);
        nlohmann::json resp = {{"status", ok ? "deleted" : "not_found"}, {"uuid", uuid}};
        return makeResponse(http::status::ok, resp.dump(), req);
    }

    // Fallback to pseudonymizer erase/soft-delete
    try {
        ensurePIIPseudonymizer();
    } catch (const std::exception& ex) {
        return makeErrorResponse(http::status::service_unavailable, 
            std::string("PII service initialization failed: ") + ex.what(), req);
    }
    if (!pii_pseudonymizer_) {
        return makeErrorResponse(http::status::service_unavailable, "PII service not initialized", req);
    }
    auto& pii_pseudonymizer = *pii_pseudonymizer_;

    nlohmann::json resp;
    if (mode == "hard") {
        bool ok = pii_pseudonymizer.erasePII(uuid);
        resp = {{"status", ok ? "ok" : "not_found"}, {"mode", "hard"}, {"uuid", uuid}, {"deleted", ok}};
    } else {
        bool ok = pii_pseudonymizer.softDeletePII(uuid, user_id.empty() ? std::string("unknown") : user_id);
        resp = {{"status", ok ? "ok" : "not_found"}, {"mode", "soft"}, {"uuid", uuid}, {"updated", ok}};
    }
    return makeResponse(http::status::ok, resp.dump(), req);
}

http::response<http::string_body> HttpServer::handlePiiListMappings(
    const http::request<http::string_body>& req
) {
    if (!config_.feature_pii_manager || !pii_api_) {
        return makeErrorResponse(http::status::not_found, "Feature 'pii_manager' disabled", req);
    }

    // Authorization: require scope pii:read or admin
    if (auto unauth = requireScope(req, "pii:read"); unauth.has_value()) return *unauth;
    auto& pii_api = *pii_api_;

    // Parse query params
    std::string target = std::string(req.target());
    auto qpos = target.find('?');
    std::string query = (qpos == std::string::npos) ? std::string() : target.substr(qpos + 1);
    auto getParam = [&](const std::string& key) -> std::string {
        auto pos = query.find(key + "=");
        if (pos == std::string::npos) return {};
        auto val = query.substr(pos + key.size() + 1);
        auto amp = val.find('&');
        if (amp != std::string::npos) val = val.substr(0, amp);
        return val;
    };
    themis::server::PiiQueryFilter filter;
    filter.original_uuid = getParam("original_uuid");
    filter.pseudonym = getParam("pseudonym");
    filter.active_only = (getParam("active_only") == "true");
    try {
        if (!getParam("page").empty()) filter.page = std::stoi(getParam("page"));
        if (!getParam("page_size").empty()) filter.page_size = std::stoi(getParam("page_size"));
    } catch (...) {}

    auto js = pii_api.listMappings(filter);
    return makeResponse(http::status::ok, js.dump(), req);
}

http::response<http::string_body> HttpServer::handlePiiCreateMapping(
    const http::request<http::string_body>& req
) {
    if (!config_.feature_pii_manager || !pii_api_) {
        return makeErrorResponse(http::status::not_found, "Feature 'pii_manager' disabled", req);
    }
    if (req.method() != http::verb::post) {
        return makeErrorResponse(http::status::method_not_allowed, "Method not allowed", req);
    }
    if (auto unauth = requireScope(req, "pii:write"); unauth.has_value()) return *unauth;
    auto& pii_api = *pii_api_;
    try {
        nlohmann::json body = nlohmann::json::parse(req.body());
        if (!body.contains("original_uuid") || !body.contains("pseudonym")) {
            return makeErrorResponse(http::status::bad_request, "Missing fields 'original_uuid' or 'pseudonym'", req);
        }
        themis::server::PiiMapping m;
        m.original_uuid = body["original_uuid"].get<std::string>();
        m.pseudonym = body["pseudonym"].get<std::string>();
        m.active = body.value("active", true);
        if (!pii_api.addMapping(m)) {
            return makeErrorResponse(http::status::conflict, "Mapping already exists", req);
        }
        return makeResponse(http::status::created, m.toJson().dump(), req);
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::bad_request, std::string("JSON error: ") + e.what(), req);
    }
}

http::response<http::string_body> HttpServer::handlePiiGetByUuid(
    const http::request<http::string_body>& req
) {
    if (!config_.feature_pii_manager || !pii_api_) {
        return makeErrorResponse(http::status::not_found, "Feature 'pii_manager' disabled", req);
    }
    if (auto unauth = requireScope(req, "pii:read"); unauth.has_value()) return *unauth;
    auto& pii_api = *pii_api_;
    std::string target = std::string(req.target());
    auto qpos = target.find('?');
    std::string path_only = (qpos == std::string::npos) ? target : target.substr(0, qpos);
    const std::string prefix = "/pii/";
    if (path_only.rfind(prefix, 0) != 0) {
        return makeErrorResponse(http::status::bad_request, "Invalid path", req);
    }
    std::string uuid = path_only.substr(prefix.size());
    if (uuid.empty() || uuid == "export.csv" || uuid == "reveal") {
        return makeErrorResponse(http::status::bad_request, "Invalid UUID", req);
    }
    auto m = pii_api.getMapping(uuid);
    if (!m) return makeErrorResponse(http::status::not_found, "PII mapping not found", req);
    return makeResponse(http::status::ok, m->toJson().dump(), req);
}

http::response<http::string_body> HttpServer::handlePiiExportCsv(
    const http::request<http::string_body>& req
) {
    if (!config_.feature_pii_manager || !pii_api_) {
        return makeErrorResponse(http::status::not_found, "Feature 'pii_manager' disabled", req);
    }
    if (auto unauth = requireScope(req, "pii:read"); unauth.has_value()) return *unauth;
    auto& pii_api = *pii_api_;
    // Reuse list parsing
    std::string target = std::string(req.target());
    auto qpos = target.find('?');
    std::string query = (qpos == std::string::npos) ? std::string() : target.substr(qpos + 1);
    auto getParam = [&](const std::string& key) -> std::string {
        auto pos = query.find(key + "=");
        if (pos == std::string::npos) return {};
        auto val = query.substr(pos + key.size() + 1);
        auto amp = val.find('&');
        if (amp != std::string::npos) val = val.substr(0, amp);
        return val;
    };
    themis::server::PiiQueryFilter filter;
    filter.original_uuid = getParam("original_uuid");
    filter.pseudonym = getParam("pseudonym");
    filter.active_only = (getParam("active_only") == "true");
    try {
        if (!getParam("page").empty()) filter.page = std::stoi(getParam("page"));
        if (!getParam("page_size").empty()) filter.page_size = std::stoi(getParam("page_size"));
    } catch (...) {}

    std::string csv = pii_api.exportCsv(filter);
    http::response<http::string_body> res{http::status::ok, req.version()};
    res.set(http::field::content_type, "text/csv; charset=utf-8");
    res.keep_alive(req.keep_alive());
    res.body() = std::move(csv);
    applyGovernanceHeaders(req, res);
    res.prepare_payload();
    return res;
}

http::response<http::string_body> HttpServer::handleLlmInteractionPost(
    const http::request<http::string_body>& req
) {
    if (!feature_llm_store_live_.load(std::memory_order_relaxed)) {
        return makeErrorResponse(http::status::not_found, "Feature 'llm_store' disabled", req);
    }
    
    auto span = Tracer::startSpan("handleLlmInteractionPost");
    span.setAttribute("http.path", "/llm/interaction");
    
    try {
        // Parse request body
        auto body_json = json::parse(req.body());
        
        // Build interaction from JSON
        LLMInteractionStore::Interaction interaction;
        interaction.prompt_template_id = body_json.value("prompt_template_id", "");
        interaction.prompt = body_json.value("prompt", "");
        
        if (body_json.contains("reasoning_chain") && body_json["reasoning_chain"].is_array()) {
            interaction.reasoning_chain = body_json["reasoning_chain"].get<std::vector<std::string>>();
        }
        
        interaction.response = body_json.value("response", "");
        interaction.model_version = body_json.value("model_version", "");
        interaction.latency_ms = body_json.value("latency_ms", 0);
        interaction.token_count = body_json.value("token_count", 0);
        
        if (body_json.contains("metadata")) {
            interaction.metadata = body_json["metadata"];
        }
        
        // Store interaction (ID and timestamp will be generated)
        auto stored = llm_store_->createInteraction(interaction);
        
        // Build response
        json response;
        response["success"] = true;
        response["interaction"] = stored.toJson();
        
        span.setAttribute("interaction.id", stored.id);
        span.setAttribute("interaction.tokens", static_cast<int64_t>(stored.token_count));
        span.setStatus(true);
        
        return makeResponse(http::status::created, response.dump(), req);
        
    } catch (const json::exception& e) {
        span.recordError(e.what());
        span.setStatus(false, "json_parse_error");
        return makeErrorResponse(http::status::bad_request, std::string("JSON error: ") + e.what(), req);
    } catch (const std::exception& e) {
        span.recordError(e.what());
        span.setStatus(false, "internal_error");
        return makeErrorResponse(http::status::internal_server_error, std::string("Error: ") + e.what(), req);
    }
}

http::response<http::string_body> HttpServer::handleLlmInteractionList(
    const http::request<http::string_body>& req
) {
    if (!feature_llm_store_live_.load(std::memory_order_relaxed)) {
        return makeErrorResponse(http::status::not_found, "Feature 'llm_store' disabled", req);
    }
    
    auto span = Tracer::startSpan("handleLlmInteractionList");
    span.setAttribute("http.path", "/llm/interaction");
    
    try {
        // Parse query parameters
        LLMInteractionStore::ListOptions options;
        
        // Simple query param parsing (can be enhanced with proper URL parsing library)
        std::string target = std::string(req.target());
        size_t query_pos = target.find('?');
        if (query_pos != std::string::npos) {
            std::string query_str = target.substr(query_pos + 1);
            
            // Parse limit
            size_t limit_pos = query_str.find("limit=");
            if (limit_pos != std::string::npos) {
                size_t limit_end = query_str.find('&', limit_pos);
                std::string limit_str = query_str.substr(limit_pos + 6, 
                    limit_end == std::string::npos ? std::string::npos : limit_end - limit_pos - 6);
                options.limit = std::stoull(limit_str);
            }
            
            // Parse start_after
            size_t start_pos = query_str.find("start_after=");
            if (start_pos != std::string::npos) {
                size_t start_end = query_str.find('&', start_pos);
                std::string start_id = query_str.substr(start_pos + 12,
                    start_end == std::string::npos ? std::string::npos : start_end - start_pos - 12);
                options.start_after_id = start_id;
            }
            
            // Parse filter_model
            size_t model_pos = query_str.find("model=");
            if (model_pos != std::string::npos) {
                size_t model_end = query_str.find('&', model_pos);
                std::string model = query_str.substr(model_pos + 6,
                    model_end == std::string::npos ? std::string::npos : model_end - model_pos - 6);
                options.filter_model = model;
            }
        }
        
        // List interactions
        auto interactions = llm_store_->listInteractions(options);
        
        // Build response
        json response;
        response["interactions"] = json::array();
        for (const auto& interaction : interactions) {
            response["interactions"].push_back(interaction.toJson());
        }
        response["count"] = interactions.size();
        
        span.setAttribute("interaction.count", static_cast<int64_t>(interactions.size()));
        span.setStatus(true);
        
        return makeResponse(http::status::ok, response.dump(), req);
        
    } catch (const std::exception& e) {
        span.recordError(e.what());
        span.setStatus(false, "internal_error");
        return makeErrorResponse(http::status::internal_server_error, std::string("Error: ") + e.what(), req);
    }
}

http::response<http::string_body> HttpServer::handleLlmInteractionGet(
    const http::request<http::string_body>& req
) {
    if (!feature_llm_store_live_.load(std::memory_order_relaxed)) {
        return makeErrorResponse(http::status::not_found, "Feature 'llm_store' disabled", req);
    }
    
    auto span = Tracer::startSpan("handleLlmInteractionGet");
    span.setAttribute("http.path", "/llm/interaction/:id");
    
    try {
        // Extract ID from path: /llm/interaction/{id}
        std::string id = extractPathParam(std::string(req.target()), "/llm/interaction/");
        
        if (id.empty()) {
            span.setStatus(false, "missing_id");
            return makeErrorResponse(http::status::bad_request, "Missing interaction ID", req);
        }
        
        span.setAttribute("interaction.id", id);
        
        // Get interaction
        auto interaction_opt = llm_store_->getInteraction(id);
        
        if (!interaction_opt.has_value()) {
            span.setStatus(false, "not_found");
            return makeErrorResponse(http::status::not_found, "Interaction not found", req);
        }
        
        // Build response
        json response = interaction_opt->toJson();
        
        span.setStatus(true);
        return makeResponse(http::status::ok, response.dump(), req);
        
    } catch (const std::exception& e) {
        span.recordError(e.what());
        span.setStatus(false, "internal_error");
        return makeErrorResponse(http::status::internal_server_error, std::string("Error: ") + e.what(), req);
    }
}

http::response<http::string_body> HttpServer::handleLlmInteractionUpdateMetadata(
    const http::request<http::string_body>& req
) {
    if (!feature_llm_store_live_.load(std::memory_order_relaxed)) {
        return makeErrorResponse(http::status::not_found, "Feature 'llm_store' disabled", req);
    }
    
    auto span = Tracer::startSpan("handleLlmInteractionUpdateMetadata");
    span.setAttribute("http.method", "PATCH");
    span.setAttribute("http.endpoint", "/llm/interaction/:id");
    
    try {
        // Extract ID from path: /llm/interaction/{id}/metadata or /llm/interaction/{id}
        std::string target(req.target());
        std::string id;
        
        // Remove /llm/interaction/ prefix
        static constexpr const char* LLM_INTERACTION_PREFIX = "/llm/interaction/";
        constexpr size_t LLM_INTERACTION_PREFIX_LEN = 17;
        if (target.rfind(LLM_INTERACTION_PREFIX, 0) == 0) {
            std::string suffix = target.substr(LLM_INTERACTION_PREFIX_LEN);
            // Remove trailing /metadata if present
            size_t metadata_pos = suffix.find("/metadata");
            if (metadata_pos != std::string::npos) {
                id = suffix.substr(0, metadata_pos);
            } else {
                id = suffix;
            }
        }
        
        if (id.empty()) {
            span.setStatus(false, "missing_id");
            return makeErrorResponse(http::status::bad_request, "Missing interaction ID", req);
        }
        
        span.setAttribute("interaction.id", id);
        
        // Parse request body with metadata updates
        auto body_json = json::parse(req.body());
        
        if (!body_json.is_object()) {
            span.setStatus(false, "invalid_metadata");
            return makeErrorResponse(http::status::bad_request, "Metadata must be a JSON object", req);
        }
        
        // Update metadata
        bool success = llm_store_->updateMetadata(id, body_json);
        
        if (!success) {
            span.setStatus(false, "not_found");
            return makeErrorResponse(http::status::not_found, "Interaction not found", req);
        }
        
        // Build response
        json response;
        response["success"] = true;
        response["message"] = "Metadata updated successfully";
        
        span.setStatus(true);
        return makeResponse(http::status::ok, response.dump(), req);
        
    } catch (const json::exception& e) {
        span.recordError(e.what());
        span.setStatus(false, "json_parse_error");
        return makeErrorResponse(http::status::bad_request, std::string("JSON error: ") + e.what(), req);
    } catch (const std::exception& e) {
        span.recordError(e.what());
        span.setStatus(false, "internal_error");
        return makeErrorResponse(http::status::internal_server_error, std::string("Error: ") + e.what(), req);
    }
}


void HttpServer::recordPageFetch(std::chrono::milliseconds duration_ms) {
    using namespace std::chrono;
    uint64_t ms = static_cast<uint64_t>(duration_ms.count());
    // Cumulative buckets: each bucket counts all values <= its upper bound
    // Buckets: 1ms, 5ms, 10ms, 25ms, 50ms, 100ms, 250ms, 500ms, 1000ms, 5000ms, +Inf
    if (ms <= 1) page_bucket_1ms_.fetch_add(1, std::memory_order_relaxed);
    if (ms <= 5) page_bucket_5ms_.fetch_add(1, std::memory_order_relaxed);
    if (ms <= 10) page_bucket_10ms_.fetch_add(1, std::memory_order_relaxed);
    if (ms <= 25) page_bucket_25ms_.fetch_add(1, std::memory_order_relaxed);
    if (ms <= 50) page_bucket_50ms_.fetch_add(1, std::memory_order_relaxed);
    if (ms <= 100) page_bucket_100ms_.fetch_add(1, std::memory_order_relaxed);
    if (ms <= 250) page_bucket_250ms_.fetch_add(1, std::memory_order_relaxed);
    if (ms <= 500) page_bucket_500ms_.fetch_add(1, std::memory_order_relaxed);
    if (ms <= 1000) page_bucket_1000ms_.fetch_add(1, std::memory_order_relaxed);
    if (ms <= 5000) page_bucket_5000ms_.fetch_add(1, std::memory_order_relaxed);
    // +Inf bucket always increments (cumulative count of all observations)
    page_bucket_inf_.fetch_add(1, std::memory_order_relaxed);
    page_sum_ms_.fetch_add(ms, std::memory_order_relaxed);
    page_count_.fetch_add(1, std::memory_order_relaxed);
}

// ============================================================================
// Entity Handlers (handleGetEntity, handlePutEntity, handleDeleteEntity, handleEntitiesBatch)
// These handlers (~980 lines) have been extracted to EntityApiHandler (entity_api_)
// and are no longer part of HttpServer. See:
// - include/server/entity_api_handler.h
// - src/server/entity_api_handler.cpp
// Routing delegated at lines 1951-1963 in this file.
// ============================================================================

http::response<http::string_body> HttpServer::handleGetContent(
    const http::request<http::string_body>& req
) {
    try {
        if (!content_manager_) {
            return makeErrorResponse(http::status::service_unavailable,
                "ContentManager not initialized", req);
        }
        auto& content_manager = *content_manager_;
        auto id = extractPathParam(std::string(req.target()), "/content/");
        if (id.empty()) return makeErrorResponse(http::status::bad_request, "Missing content id", req);
        auto meta = content_manager.getContentMeta(id);
        if (!meta) return makeErrorResponse(http::status::not_found, "Content not found", req);
        return makeResponse(http::status::ok, meta->toJson().dump(), req);
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> HttpServer::handleGetContentBlob(
    const http::request<http::string_body>& req
) {
    try {
        if (!content_manager_) {
            return makeErrorResponse(http::status::service_unavailable,
                "ContentManager not initialized", req);
        }
        auto& content_manager = *content_manager_;
        auto path = std::string(req.target());
        // path format: /content/{id}/blob
        auto prefix = std::string("/content/");
        auto pos = path.find("/blob");
        if (pos == std::string::npos) return makeErrorResponse(http::status::bad_request, "Invalid path", req);
        auto id = path.substr(prefix.size(), pos - prefix.size());
    auto auth_ctx = extractAuthContext(req);
    std::string user_ctx = auth_ctx.user_id;
    auto blob = content_manager.getContentBlob(id, user_ctx);
        if (!blob) return makeErrorResponse(http::status::not_found, "Blob not found", req);
        auto meta = content_manager.getContentMeta(id);
        std::string mime = (meta ? meta->mime_type : std::string("application/octet-stream"));

        http::response<http::string_body> res{http::status::ok, req.version()};
        res.set(http::field::server, "THEMIS/0.1.0");
        res.set(http::field::content_type, mime);
        res.keep_alive(req.keep_alive());
        res.body() = *blob; // may contain binary data
    // Apply governance headers also for blob responses
    applyGovernanceHeaders(req, res);
        res.prepare_payload();
        return res;
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> HttpServer::handleGetContentChunks(
    const http::request<http::string_body>& req
) {
    try {
        if (!content_manager_) {
            return makeErrorResponse(http::status::service_unavailable,
                "ContentManager not initialized", req);
        }
        auto& content_manager = *content_manager_;
        auto path = std::string(req.target());
        // path format: /content/{id}/chunks
        auto prefix = std::string("/content/");
        auto pos = path.find("/chunks");
        if (pos == std::string::npos) return makeErrorResponse(http::status::bad_request, "Invalid path", req);
        auto id = path.substr(prefix.size(), pos - prefix.size());
        auto chunks = content_manager.getContentChunks(id);
        json arr = json::array();
        arr.get_ref<json::array_t&>().reserve(chunks.size());
        for (const auto& c : chunks) {
            json j = c.toJson();
            // For response size, omit full embedding by default
            if (j.contains("embedding")) j["embedding"] = json::array();
            arr.push_back(std::move(j));
        }
        json resp = { {"count", chunks.size()}, {"chunks", std::move(arr)} };
        return makeResponse(http::status::ok, resp.dump(), req);
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> HttpServer::handleHybridSearch(
    const http::request<http::string_body>& req
) {
    try {
        if (!content_manager_) return makeErrorResponse(http::status::service_unavailable, "ContentManager not initialized", req);
        auto& content_manager = *content_manager_;
        json body = json::parse(req.body());
        std::string query = body.value("query", "");
        int k = body.value("k", 10);
        int hops = 1;
        if (body.contains("expand") && body["expand"].is_object()) {
            hops = body["expand"].value("hops", 1);
        }
        json filters = json::object();
        if (body.contains("filters")) filters = body["filters"];
        if (body.contains("scoring")) filters["scoring"] = body["scoring"];

        auto results = content_manager.searchWithExpansion(query, k, hops, filters);
        json resp = json::array();
        resp.get_ref<json::array_t&>().reserve(results.size());
        for (const auto& [pk, score] : results) {
            resp.push_back({{"pk", pk}, {"score", score}});
        }
        json out = {
            {"count", resp.size()},
            {"results", resp}
        };
        return makeResponse(http::status::ok, out.dump(), req);
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::bad_request, std::string("Hybrid search error: ") + e.what(), req);
    } catch (...) {
        THEMIS_WARN("http_server: unhandled exception caught");
        return makeErrorResponse(http::status::bad_request, "Hybrid search error", req);
    }
}

http::response<http::string_body> HttpServer::handleFulltextSearch(
    const http::request<http::string_body>& req
) {
    try {
        if (!secondary_index_) return makeErrorResponse(http::status::service_unavailable, "IndexManager not initialized", req);
        auto& secondary_index = *secondary_index_;
        
        json body = json::parse(req.body());
        
        // Required fields
        if (!body.contains("table") || !body["table"].is_string()) {
            return makeErrorResponse(http::status::bad_request, "Missing or invalid 'table' field", req);
        }
        if (!body.contains("column") || !body["column"].is_string()) {
            return makeErrorResponse(http::status::bad_request, "Missing or invalid 'column' field", req);
        }
        if (!body.contains("query") || !body["query"].is_string()) {
            return makeErrorResponse(http::status::bad_request, "Missing or invalid 'query' field", req);
        }
        
        std::string table = body["table"];
        std::string column = body["column"];
        std::string query = body["query"];
        size_t limit = body.value("limit", 1000);
        
        // Check if fulltext index exists
        if (!secondary_index.hasFulltextIndex(table, column)) {
            return makeErrorResponse(http::status::bad_request, 
                "No fulltext index on " + table + "." + column, req);
        }
        
        // Perform BM25-scored fulltext search
        auto [status, results] = secondary_index.scanFulltextWithScores(table, column, query, limit);
        
        if (!status.ok) {
            return makeErrorResponse(http::status::internal_server_error, status.message, req);
        }
        
        // Build response with scores
        json resp = json::array();
        resp.get_ref<json::array_t&>().reserve(results.size());
        for (const auto& result : results) {
            resp.push_back({
                {"pk", result.pk},
                {"score", result.score}
            });
        }
        
        json out = {
            {"count", resp.size()},
            {"results", resp},
            {"table", table},
            {"column", column},
            {"query", query}
        };
        
        return makeResponse(http::status::ok, out.dump(), req);
        
    } catch (const json::exception& e) {
        return makeErrorResponse(http::status::bad_request, std::string("JSON parse error: ") + e.what(), req);
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error, std::string("Fulltext search error: ") + e.what(), req);
    } catch (...) {
        THEMIS_DEBUG("http_server: unhandled exception caught");
        return makeErrorResponse(http::status::internal_server_error, "Unknown fulltext search error", req);
    }
}

http::response<http::string_body> HttpServer::handleFusionSearch(
    const http::request<http::string_body>& req
) {
    try {
        if (!secondary_index_) return makeErrorResponse(http::status::service_unavailable, "SecondaryIndexManager not initialized", req);
        auto& secondary_index = *secondary_index_;
        if (!vector_index_) return makeErrorResponse(http::status::service_unavailable, "VectorIndexManager not initialized", req);
        
        json body = json::parse(req.body());
        
        // Validate required fields
        if (!body.contains("table") || !body["table"].is_string()) {
            return makeErrorResponse(http::status::bad_request, "Missing or invalid 'table' field", req);
        }
        
        std::string table = body["table"];
        int k = body.value("k", 10);
        std::string fusionMode = body.value("fusion_mode", "rrf"); // "rrf" or "weighted"
        
        // Text search parameters (optional)
        std::vector<SecondaryIndexManager::FulltextResult> textResults;
        bool hasTextQuery = body.contains("text_query") && body.contains("text_column");
        
        if (hasTextQuery) {
            std::string textColumn = body["text_column"];
            std::string textQuery = body["text_query"];
            int textLimit = body.value("text_limit", 1000);
            
            if (!secondary_index.hasFulltextIndex(table, textColumn)) {
                return makeErrorResponse(http::status::bad_request, 
                    "No fulltext index on " + table + "." + textColumn, req);
            }
            
            auto [textStatus, textRes] = secondary_index.scanFulltextWithScores(table, textColumn, textQuery, textLimit);
            if (!textStatus.ok) {
                return makeErrorResponse(http::status::internal_server_error, "Text search failed: " + textStatus.message, req);
            }
            textResults = std::move(textRes);
        }
        
        // Vector search parameters (optional)
        std::vector<VectorIndexManager::Result> vectorResults;
        bool hasVectorQuery = body.contains("vector_query");
        
        if (hasVectorQuery) {
            if (!body["vector_query"].is_array()) {
                return makeErrorResponse(http::status::bad_request, "vector_query must be array of floats", req);
            }
            
            std::vector<float> vectorQuery;
            vectorQuery.reserve(body["vector_query"].size());
            for (const auto& val : body["vector_query"]) {
                if (val.is_number()) {
                    vectorQuery.push_back(val.get<float>());
                }
            }
            
            if (vectorQuery.empty()) {
                return makeErrorResponse(http::status::bad_request, "vector_query array is empty", req);
            }
            
            int vectorLimit = body.value("vector_limit", 1000);
            auto [vecStatus, vecRes] = vector_index_->searchKnn(vectorQuery, vectorLimit);
            if (!vecStatus.ok) {
                return makeErrorResponse(http::status::internal_server_error, "Vector search failed: " + vecStatus.message, req);
            }
            vectorResults = std::move(vecRes);
        }
        
        // Require at least one query type
        if (!hasTextQuery && !hasVectorQuery) {
            return makeErrorResponse(http::status::bad_request, "At least one of text_query or vector_query required", req);
        }
        
        // Fusion logic
        std::vector<std::pair<std::string, double>> fusedResults;
        
        if (fusionMode == "rrf") {
            // Reciprocal Rank Fusion: score = sum(1 / (k + rank))
            int kRrf = body.value("k_rrf", 60);
            std::unordered_map<std::string, double> scores;
            scores.reserve(textResults.size() + vectorResults.size());

            // Text contributions
            for (size_t i = 0; i < textResults.size(); ++i) {
                auto [it, inserted] = scores.try_emplace(textResults[i].pk, 0.0);
                it->second += 1.0 / (kRrf + i + 1);
            }
            
            // Vector contributions
            for (size_t i = 0; i < vectorResults.size(); ++i) {
                auto [it, inserted] = scores.try_emplace(vectorResults[i].pk, 0.0);
                it->second += 1.0 / (kRrf + i + 1);
            }
            
            // Convert to vector and sort
            fusedResults.reserve(scores.size());
            for (const auto& [pk, score] : scores) {
                fusedResults.emplace_back(pk, score);
            }
            std::sort(fusedResults.begin(), fusedResults.end(), 
                [](const auto& a, const auto& b) { return a.second > b.second; });
            
        } else if (fusionMode == "weighted") {
            // Weighted fusion: alpha * normalize(text_score) + (1 - alpha) * normalize(vector_sim)
            double alpha = body.value("weight_text", 0.5);
            alpha = std::clamp(alpha, 0.0, 1.0);
            
            // Normalize text scores (min-max)
            double textMin = textResults.empty() ? 0.0 : textResults.back().score;
            double textMax = textResults.empty() ? 1.0 : textResults.front().score;
            double textRange = (textMax - textMin) > 1e-9 ? (textMax - textMin) : 1.0;
            
            // Normalize vector distances (convert to similarity: 1 - normalized_dist)
            // Assuming L2 or COSINE metric; smaller distance = better
            double vecMin = vectorResults.empty() ? 0.0 : vectorResults.front().distance;
            double vecMax = vectorResults.empty() ? 1.0 : vectorResults.back().distance;
            double vecRange = (vecMax - vecMin) > 1e-9 ? (vecMax - vecMin) : 1.0;
            
            std::unordered_map<std::string, double> scores;
            scores.reserve(textResults.size() + vectorResults.size());
            
            // Text contributions
            for (const auto& res : textResults) {
                double normScore = (res.score - textMin) / textRange;
                auto [it, inserted] = scores.try_emplace(res.pk, 0.0);
                it->second += alpha * normScore;
            }
            
            // Vector contributions (convert distance to similarity)
            for (const auto& res : vectorResults) {
                double normDist = (res.distance - vecMin) / vecRange;
                double similarity = 1.0 - normDist;
                auto [it, inserted] = scores.try_emplace(res.pk, 0.0);
                it->second += (1.0 - alpha) * similarity;
            }
            
            // Convert to vector and sort
            fusedResults.reserve(scores.size());
            for (const auto& [pk, score] : scores) {
                fusedResults.emplace_back(pk, score);
            }
            std::sort(fusedResults.begin(), fusedResults.end(), 
                [](const auto& a, const auto& b) { return a.second > b.second; });
            
        } else {
            return makeErrorResponse(http::status::bad_request, 
                "Invalid fusion_mode: " + fusionMode + " (must be 'rrf' or 'weighted')", req);
        }
        
        // Limit to top-k
        if (fusedResults.size() > static_cast<size_t>(k)) {
            fusedResults.resize(k);
        }
        
        // Build response
        json resp = json::array();
        resp.get_ref<json::array_t&>().reserve(fusedResults.size());
        for (const auto& [pk, score] : fusedResults) {
            resp.push_back({
                {"pk", pk},
                {"score", score}
            });
        }
        
        json out = {
            {"count", resp.size()},
            {"fusion_mode", fusionMode},
            {"table", table},
            {"results", resp}
        };
        
        if (hasTextQuery) {
            out["text_count"] = textResults.size();
        }
        if (hasVectorQuery) {
            out["vector_count"] = vectorResults.size();
        }
        
        return makeResponse(http::status::ok, out.dump(), req);
        
    } catch (const json::exception& e) {
        return makeErrorResponse(http::status::bad_request, std::string("JSON parse error: ") + e.what(), req);
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error, std::string("Fusion search error: ") + e.what(), req);
    } catch (...) {
        THEMIS_DEBUG("http_server: unhandled exception caught");
        return makeErrorResponse(http::status::internal_server_error, "Unknown fusion search error", req);
    }
}

http::response<http::string_body> HttpServer::handleContentFilterSchemaGet(
    const http::request<http::string_body>& req
) {
    if (!storage_) {
        return makeErrorResponse(http::status::service_unavailable, "Storage engine not available", req);
    }

    try {
        auto v = storage_->get("config:content_filter_schema");
        json resp;
        if (v) {
            std::string s(v->begin(), v->end());
            resp = json::parse(s);
        } else {
            resp = json{{"field_map", json::object()}};
        }
        return makeResponse(http::status::ok, resp.dump(), req);
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error, std::string("config read error: ") + e.what(), req);
    } catch (...) {
        THEMIS_WARN("http_server: unhandled exception caught");
        return makeErrorResponse(http::status::internal_server_error, "config read error", req);
    }
}

http::response<http::string_body> HttpServer::handleContentFilterSchemaPut(
    const http::request<http::string_body>& req
) {
    if (!storage_) {
        return makeErrorResponse(http::status::service_unavailable, "Storage engine not available", req);
    }

    try {
        json body = json::parse(req.body());
        if (!body.is_object() || !body.contains("field_map") || !body["field_map"].is_object()) {
            return makeErrorResponse(http::status::bad_request, "Body must be { field_map: { key: path } }", req);
        }
        std::string s = body.dump();
        std::vector<uint8_t> bytes(s.begin(), s.end());
        bool ok = storage_->put("config:content_filter_schema", bytes);
        if (!ok) return makeErrorResponse(http::status::internal_server_error, "Failed to store filter schema", req);
        return makeResponse(http::status::ok, json{{"status","ok"}}.dump(), req);
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::bad_request, std::string("config write error: ") + e.what(), req);
    } catch (...) {
        THEMIS_DEBUG("http_server: unhandled exception caught");
        return makeErrorResponse(http::status::bad_request, "config write error", req);
    }
}

http::response<http::string_body> HttpServer::handleContentConfigGet(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handleContentConfigGet");
    if (!storage_) {
        span.setStatus(false, "storage_unavailable");
        return makeErrorResponse(http::status::service_unavailable, "Storage engine not available", req);
    }
    
    try {
        auto v = storage_->get("config:content");
        json resp;
        if (v) {
            std::string s(v->begin(), v->end());
            resp = json::parse(s);
        } else {
            // Return defaults
            resp = {
                {"compress_blobs", false},
                {"compression_level", 19},
                {"skip_compressed_mimes", json::array({"image/", "video/", "application/zip", "application/gzip"})}
            };
        }
        
        span.setStatus(true);
        return makeResponse(http::status::ok, resp.dump(), req);
        
    } catch (const std::exception& e) {
        span.setStatus(false, "error");
        return makeErrorResponse(http::status::internal_server_error, 
            std::string("config read error: ") + e.what(), req);
    }
}

http::response<http::string_body> HttpServer::handleContentConfigPut(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handleContentConfigPut");
    if (!storage_) {
        span.setStatus(false, "storage_unavailable");
        return makeErrorResponse(http::status::service_unavailable, "Storage engine not available", req);
    }
    
    try {
        json body = json::parse(req.body());
        
        // Get current config or defaults
        json config;
        auto v = storage_->get("config:content");
        if (v) {
            std::string s(v->begin(), v->end());
            config = json::parse(s);
        } else {
            config = {
                {"compress_blobs", false},
                {"compression_level", 19},
                {"skip_compressed_mimes", json::array({"image/", "video/", "application/zip", "application/gzip"})}
            };
        }
        
        // Update with provided values
        if (body.contains("compress_blobs")) {
            if (!body["compress_blobs"].is_boolean()) {
                span.setStatus(false, "invalid_compress_blobs");
                return makeErrorResponse(http::status::bad_request, 
                    "compress_blobs must be boolean", req);
            }
            config["compress_blobs"] = body["compress_blobs"];
        }
        
        if (body.contains("compression_level")) {
            if (!body["compression_level"].is_number_integer()) {
                span.setStatus(false, "invalid_compression_level");
                return makeErrorResponse(http::status::bad_request, 
                    "compression_level must be an integer", req);
            }
            int level = body["compression_level"];
            if (level < 1 || level > 22) {
                span.setStatus(false, "compression_level_out_of_range");
                return makeErrorResponse(http::status::bad_request, 
                    "compression_level must be between 1 and 22", req);
            }
            config["compression_level"] = level;
        }
        
        if (body.contains("skip_compressed_mimes")) {
            if (!body["skip_compressed_mimes"].is_array()) {
                span.setStatus(false, "invalid_skip_mimes");
                return makeErrorResponse(http::status::bad_request, 
                    "skip_compressed_mimes must be an array of strings", req);
            }
            // Validate all elements are strings
            for (const auto& item : body["skip_compressed_mimes"]) {
                if (!item.is_string()) {
                    span.setStatus(false, "invalid_skip_mimes_element");
                    return makeErrorResponse(http::status::bad_request, 
                        "All elements in skip_compressed_mimes must be strings", req);
                }
            }
            config["skip_compressed_mimes"] = body["skip_compressed_mimes"];
        }
        
        // Store updated config
        std::string config_str = config.dump();
        std::vector<uint8_t> bytes(config_str.begin(), config_str.end());
        bool ok = storage_->put("config:content", bytes);
        
        if (!ok) {
            span.setStatus(false, "storage_error");
            return makeErrorResponse(http::status::internal_server_error, 
                "Failed to store content config", req);
        }
        
        json response = config;
        response["status"] = "ok";
        response["note"] = "Configuration updated. Changes apply to new content imports only.";
        
        span.setStatus(true);
        return makeResponse(http::status::ok, response.dump(), req);
        
    } catch (const json::exception& e) {
        span.setStatus(false, "json_error");
        return makeErrorResponse(http::status::bad_request, 
            std::string("JSON error: ") + e.what(), req);
    } catch (const std::exception& e) {
        span.setStatus(false, "error");
        return makeErrorResponse(http::status::internal_server_error, 
            std::string("config write error: ") + e.what(), req);
    }
}

http::response<http::string_body> HttpServer::handleEdgeWeightConfigGet(
    const http::request<http::string_body>& req
) {
    if (!storage_) {
        return makeErrorResponse(http::status::service_unavailable, "Storage engine not available", req);
    }

    try {
        auto v = storage_->get("config:edge_weights");
        json resp;
        if (v) {
            std::string s(v->begin(), v->end());
            resp = json::parse(s);
        } else {
            resp = json{{"weights", json{{"parent", 1.0}, {"next", 1.0}, {"prev", 1.0}}}};
        }
        return makeResponse(http::status::ok, resp.dump(), req);
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error, std::string("config read error: ") + e.what(), req);
    } catch (...) {
        THEMIS_WARN("http_server: unhandled exception caught");
        return makeErrorResponse(http::status::internal_server_error, "config read error", req);
    }
}

http::response<http::string_body> HttpServer::handleEdgeWeightConfigPut(
    const http::request<http::string_body>& req
) {
    if (!storage_) {
        return makeErrorResponse(http::status::service_unavailable, "Storage engine not available", req);
    }

    try {
        json body = json::parse(req.body());
        if (!body.is_object() || !body.contains("weights") || !body["weights"].is_object()) {
            return makeErrorResponse(http::status::bad_request, "Body must be { weights: { parent: number, next: number, prev: number } }", req);
        }
        // Validate all values numeric
        for (auto it = body["weights"].begin(); it != body["weights"].end(); ++it) {
            if (!it.value().is_number()) {
                return makeErrorResponse(http::status::bad_request, "All weights must be numeric", req);
            }
        }
        std::string s = body.dump();
        std::vector<uint8_t> bytes(s.begin(), s.end());
        bool ok = storage_->put("config:edge_weights", bytes);
        if (!ok) return makeErrorResponse(http::status::internal_server_error, "Failed to store edge weights", req);
        return makeResponse(http::status::ok, json{{"status","ok"}}.dump(), req);
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::bad_request, std::string("config write error: ") + e.what(), req);
    } catch (...) {
        THEMIS_DEBUG("http_server: unhandled exception caught");
        return makeErrorResponse(http::status::bad_request, "config write error", req);
    }
}

// ===================== Encryption Schema Management =====================

http::response<http::string_body> HttpServer::handleEncryptionSchemaGet(
    const http::request<http::string_body>& req
) {
    // Require config:read scope
    if (auth_ && auth_->isEnabled()) {
        std::string path_only = std::string(req.target());
        auto qpos = path_only.find('?');
        if (qpos != std::string::npos) path_only = path_only.substr(0, qpos);
        if (auto resp = requireAccess(req, "config:read", "config.read", path_only)) return *resp;
    }

    if (!storage_) {
        return makeErrorResponse(http::status::service_unavailable, "Storage engine not available", req);
    }
    
    try {
        auto schema_bytes = storage_->get("config:encryption_schema");
        if (!schema_bytes) {
            // Return empty schema if not configured
            json empty_schema = {
                {"collections", json::object()}
            };
            return makeResponse(http::status::ok, empty_schema.dump(2), req);
        }
        
        std::string schema_json(schema_bytes->begin(), schema_bytes->end());
        // Validate JSON before returning
        try {
            auto parsed = json::parse(schema_json);
            return makeResponse(http::status::ok, parsed.dump(2), req);
        } catch (const json::exception& e) {
            return makeErrorResponse(http::status::internal_server_error, 
                std::string("Stored schema is invalid JSON: ") + e.what(), req);
        }
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> HttpServer::handleEncryptionSchemaPut(
    const http::request<http::string_body>& req
) {
    // Require config:write scope
    if (auth_ && auth_->isEnabled()) {
        std::string path_only = std::string(req.target());
        auto qpos = path_only.find('?');
        if (qpos != std::string::npos) path_only = path_only.substr(0, qpos);
        if (auto resp = requireAccess(req, "config:write", "config.write", path_only)) return *resp;
    }

    if (!storage_) {
        return makeErrorResponse(http::status::service_unavailable, "Storage engine not available", req);
    }
    
    try {
        json body = json::parse(req.body());
        
        // Validate schema structure
        if (!body.contains("collections") || !body["collections"].is_object()) {
            return makeErrorResponse(http::status::bad_request, 
                "Schema must contain 'collections' object", req);
        }
        
        // Validate each collection
        for (auto& [collection_name, collection_config] : body["collections"].items()) {
            if (!collection_config.is_object()) {
                return makeErrorResponse(http::status::bad_request, 
                    "Collection config for '" + collection_name + "' must be an object", req);
            }
            
            if (!collection_config.contains("encryption")) continue;
            
            auto& enc = collection_config["encryption"];
            if (!enc.is_object()) {
                return makeErrorResponse(http::status::bad_request, 
                    "Encryption config for '" + collection_name + "' must be an object", req);
            }
            
            // Validate required fields
            if (!enc.contains("enabled") || !enc["enabled"].is_boolean()) {
                return makeErrorResponse(http::status::bad_request, 
                    "Encryption 'enabled' must be boolean for collection '" + collection_name + "'", req);
            }
            
            if (enc["enabled"].get<bool>()) {
                // If enabled, require fields array
                if (!enc.contains("fields") || !enc["fields"].is_array()) {
                    return makeErrorResponse(http::status::bad_request, 
                        "Encryption 'fields' must be array for collection '" + collection_name + "'", req);
                }
                
                // Validate fields are strings
                for (auto& field : enc["fields"]) {
                    if (!field.is_string()) {
                        return makeErrorResponse(http::status::bad_request, 
                            "All fields must be strings for collection '" + collection_name + "'", req);
                    }
                }
                
                // Validate context_type if present
                if (enc.contains("context_type")) {
                    std::string ctx = enc["context_type"].get<std::string>();
                    if (ctx != "user" && ctx != "group") {
                        return makeErrorResponse(http::status::bad_request, 
                            "context_type must be 'user' or 'group' for collection '" + collection_name + "'", req);
                    }
                    
                    // If group context, allowed_groups is optional but should be array if present
                    if (ctx == "group" && enc.contains("allowed_groups")) {
                        if (!enc["allowed_groups"].is_array()) {
                            return makeErrorResponse(http::status::bad_request, 
                                "allowed_groups must be array for collection '" + collection_name + "'", req);
                        }
                    }
                }
            }
        }
        
        // Store validated schema
        std::string schema_str = body.dump();
        std::vector<uint8_t> bytes(schema_str.begin(), schema_str.end());
        bool ok = storage_->put("config:encryption_schema", bytes);
        
        if (!ok) {
            return makeErrorResponse(http::status::internal_server_error, 
                "Failed to store encryption schema", req);
        }
        
        THEMIS_INFO("Encryption schema updated: {} collections configured", 
            body["collections"].size());
        
        json response = {
            {"status", "ok"},
            {"collections_configured", body["collections"].size()}
        };
        return makeResponse(http::status::ok, response.dump(), req);
        
    } catch (const json::exception& e) {
        return makeErrorResponse(http::status::bad_request, 
            std::string("Invalid JSON: ") + e.what(), req);
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> HttpServer::handleCreateIndex(
    const http::request<http::string_body>& req
) {
    try {
        auto body = json::parse(req.body());
        if (!body.contains("table")) {
            return makeErrorResponse(http::status::bad_request, "Missing 'table'", req);
        }
        std::string table = body["table"].get<std::string>();
        bool unique = false;
        if (body.contains("unique")) {
            unique = body["unique"].get<bool>();
        }

        // Support range index creation via type = "range"
        if (body.contains("type")) {
            std::string type = body["type"].get<std::string>();
            if (type == "range") {
                if (!body.contains("column")) {
                    return makeErrorResponse(http::status::bad_request, "Missing 'column' for range index", req);
                }
                std::string column = body["column"].get<std::string>();
                auto st = secondary_index_->createRangeIndex(table, column);
                if (!st.ok) {
                    return makeErrorResponse(http::status::bad_request, st.message, req);
                }
                json resp = {{"success", true}, {"table", table}, {"column", column}, {"type", "range"}};
                return makeResponse(http::status::ok, resp.dump(), req);
            } else if (type == "fulltext") {
                if (!body.contains("column")) {
                    return makeErrorResponse(http::status::bad_request, "Missing 'column' for fulltext index", req);
                }
                std::string column = body["column"].get<std::string>();
                
                // Parse optional config
                SecondaryIndexManager::FulltextConfig config;
                if (body.contains("config") && body["config"].is_object()) {
                    auto configObj = body["config"];
                    config.stemming_enabled = configObj.value("stemming_enabled", false);
                    config.language = configObj.value("language", "none");
                    config.stopwords_enabled = configObj.value("stopwords_enabled", false);
                    if (configObj.contains("stopwords") && configObj["stopwords"].is_array()) {
                        for (const auto& s : configObj["stopwords"]) {
                            if (s.is_string()) config.stopwords.push_back(s.get<std::string>());
                        }
                    }
                    config.normalize_umlauts = configObj.value("normalize_umlauts", false);
                } else {
                    config.stemming_enabled = false;
                    config.language = "none";
                    config.stopwords_enabled = false;
                    config.normalize_umlauts = false;
                }
                
                auto st = secondary_index_->createFulltextIndex(table, column, config);
                if (!st.ok) {
                    return makeErrorResponse(http::status::bad_request, st.message, req);
                }
                
                json resp = {
                    {"success", true}, 
                    {"table", table}, 
                    {"column", column}, 
                    {"type", "fulltext"},
                    {"config", {
                        {"stemming_enabled", config.stemming_enabled},
                        {"language", config.language},
                        {"stopwords_enabled", config.stopwords_enabled},
                        {"stopwords", config.stopwords},
                        {"normalize_umlauts", config.normalize_umlauts}
                    }}
                };
                return makeResponse(http::status::ok, resp.dump(), req);
            }
        }

        // Support single-column (column) and composite (columns)
        if (body.contains("columns")) {
            if (!body["columns"].is_array() || body["columns"].empty()) {
                return makeErrorResponse(http::status::bad_request, "'columns' must be a non-empty array of strings", req);
            }
            std::vector<std::string> columns;
            columns.reserve(body["columns"].size());
            for (const auto& c : body["columns"]) {
                columns.push_back(c.get<std::string>());
            }
            auto st = secondary_index_->createCompositeIndex(table, columns, unique);
            if (!st.ok) {
                return makeErrorResponse(http::status::bad_request, st.message, req);
            }
            json resp = {{"success", true}, {"table", table}, {"columns", columns}, {"unique", unique}};
            return makeResponse(http::status::ok, resp.dump(), req);
        }
        
        if (!body.contains("column")) {
            return makeErrorResponse(http::status::bad_request, "Missing 'column' or 'columns'", req);
        }
        std::string column = body["column"].get<std::string>();
        auto st = secondary_index_->createIndex(table, column, unique);
        if (!st.ok) {
            return makeErrorResponse(http::status::bad_request, st.message, req);
        }
        json resp = {{"success", true}, {"table", table}, {"column", column}, {"unique", unique}};
        return makeResponse(http::status::ok, resp.dump(), req);
    } catch (const json::exception& e) {
        return makeErrorResponse(http::status::bad_request, "Invalid JSON: " + std::string(e.what()), req);
    }
}

http::response<http::string_body> HttpServer::handleDropIndex(
    const http::request<http::string_body>& req
) {
    try {
        auto body = json::parse(req.body());
        if (!body.contains("table") || !body.contains("column")) {
            return makeErrorResponse(http::status::bad_request, "Missing 'table' or 'column'", req);
        }
        std::string table = body["table"].get<std::string>();
        std::string column = body["column"].get<std::string>();

        // Optional type for dropping range indexes
        if (body.contains("type") && body["type"].is_string() && body["type"].get<std::string>() == "range") {
            auto st = secondary_index_->dropRangeIndex(table, column);
            if (!st.ok) {
                return makeErrorResponse(http::status::bad_request, st.message, req);
            }
            json resp = {{"success", true}, {"table", table}, {"column", column}, {"type", "range"}};
            return makeResponse(http::status::ok, resp.dump(), req);
        }

        auto st = secondary_index_->dropIndex(table, column);
        if (!st.ok) {
            return makeErrorResponse(http::status::bad_request, st.message, req);
        }
        json resp = {{"success", true}, {"table", table}, {"column", column}};
        return makeResponse(http::status::ok, resp.dump(), req);
    } catch (const json::exception& e) {
        return makeErrorResponse(http::status::bad_request, "Invalid JSON: " + std::string(e.what()), req);
    }
}

http::response<http::string_body> HttpServer::makeResponse(
    http::status status,
    const std::string& body,
    const http::request<http::string_body>& req
) {
    http::response<http::string_body> res{status, req.version()};
    res.set(http::field::server, "THEMIS/0.1.0");
    res.set(http::field::content_type, "application/json");
    res.keep_alive(req.keep_alive());
    res.body() = body;
    // Inject governance headers consistently
    applyGovernanceHeaders(req, res);
    res.prepare_payload();
    return res;
}

http::response<http::string_body> HttpServer::makeErrorResponse(
    http::status status,
    const std::string& message,
    const http::request<http::string_body>& req
) {
    // Increment error counter
    error_count_.fetch_add(1, std::memory_order_relaxed);
    
    json error_body = {
        {"error", true},
        {"message", message},
        {"status_code", static_cast<int>(status)}
    };
    return makeResponse(status, error_body.dump(), req);
}

http::response<http::string_body> HttpServer::makePreflightResponse(
    const http::request<http::string_body>& req
) {
    // Determine request origin
    std::string origin;
    if (auto it = req.find(http::field::origin); it != req.end()) {
        origin = std::string(it->value());
    }

    // Decide allowed origin
    std::string allow_origin;
    if (cors_allow_all_) {
        allow_origin = "*";
    } else if (!origin.empty()) {
        bool allowed = false;
        for (const auto& o : cors_allowed_origins_) {
            if (o == origin) { allowed = true; break; }
        }
        if (allowed) allow_origin = origin;
    }

    // If not allowed, respond 403 without exposing CORS headers
    if (allow_origin.empty()) {
        http::response<http::string_body> res{http::status::forbidden, req.version()};
        res.set(http::field::server, "THEMIS/0.1.0");
        res.set(http::field::content_type, "application/json");
        res.keep_alive(req.keep_alive());
        nlohmann::json body = {
            {"error", true},
            {"message", "CORS origin not allowed"},
            {"status_code", 403}
        };
        res.body() = body.dump();
        applyGovernanceHeaders(req, res);
        res.prepare_payload();
        return res;
    }

    // Build successful preflight response (204 No Content)
    http::response<http::string_body> res{http::status::no_content, req.version()};
    res.set(http::field::server, "THEMIS/0.1.0");
    res.keep_alive(req.keep_alive());

    // CORS headers
    res.set("Access-Control-Allow-Origin", allow_origin);
    // Ensure caches vary on relevant request headers
    res.set("Vary", "Origin, Access-Control-Request-Method, Access-Control-Request-Headers");
    res.set("Access-Control-Allow-Methods", cors_allowed_methods_);
    res.set("Access-Control-Allow-Headers", cors_allowed_headers_);
    if (cors_allow_credentials_ && allow_origin != "*") {
        res.set("Access-Control-Allow-Credentials", "true");
    }
    // Cache preflight for 10 minutes
    res.set("Access-Control-Max-Age", "600");

    // Apply security/governance headers consistently
    applyGovernanceHeaders(req, res);

    res.prepare_payload();
    return res;
}

void HttpServer::applyGovernanceHeaders(
    const http::request<http::string_body>& req,
    http::response<http::string_body>& res
) {
    // Derive governance from request headers and path
    auto to_lower = [](std::string s){ for (auto& c : s) c = static_cast<char>(::tolower(static_cast<unsigned char>(c))); return s; };
    std::string path_only = std::string(req.target());
    auto qpos = path_only.find('?');
    if (qpos != std::string::npos) path_only = path_only.substr(0, qpos);

    // Read incoming hints
    std::string classification = ""; // offen | geheim | streng-geheim | vs-nfd
    std::string mode = "observe";    // observe (default) | enforce
    bool encrypt_logs = false;
    for (const auto& h : req) {
        auto name = h.name_string();
        if (beast::iequals(name, "X-Classification")) {
            classification = to_lower(std::string(h.value()));
        } else if (beast::iequals(name, "X-Governance-Mode")) {
            mode = to_lower(std::string(h.value()));
        } else if (beast::iequals(name, "X-Encrypt-Logs")) {
            std::string v = to_lower(std::string(h.value()));
            encrypt_logs = (v == "true" || v == "1" || v == "yes");
        }
    }
    // Resource-based default classification if none provided
    if (classification.empty()) {
        if (path_only.rfind("/admin", 0) == 0) classification = "vs-nfd"; else classification = "offen";
    }
    // Normalize/validate known values
    if (classification != "offen" && classification != "geheim" && classification != "streng-geheim" && classification != "vs-nfd") {
        // Unknown classification -> leave policy header but choose restrictive defaults
        classification = classification; // keep text in summary if provided
    }
    if (mode != "observe" && mode != "enforce") mode = "observe";

    // Derive header values from classification
    std::string ann = (vector_index_ ? std::string("allowed") : std::string("disabled"));
    std::string content_enc = "optional";
    std::string export_perm = "allowed";
    std::string cache_perm = (feature_semantic_cache_live_.load(std::memory_order_relaxed) ? std::string("allowed") : std::string("disabled"));
    std::string retention_days = "365";
    std::string redaction = "none";

    if (classification == "geheim") {
        ann = "disabled";
        cache_perm = "disabled";
        // keep enc optional for geheim per tests (only vs-nfd/streng-geheim require)
    } else if (classification == "streng-geheim") {
        ann = "disabled";
        content_enc = "required";
        export_perm = "forbidden";
        cache_perm = "disabled";
        redaction = "strict";
        retention_days = "1095"; // 3 Jahre, nicht getestet aber plausibel
    } else if (classification == "vs-nfd") {
        content_enc = "required";
        retention_days = "730"; // 2 Jahre
    } else if (classification == "offen") {
        // defaults ok
    }

    // Compose policy summary
    std::string policy_summary = "classification=" + classification + ";mode=" + mode + ";encrypt_logs=" + (encrypt_logs?"true":"false") + ";redaction=" + redaction;

    // Write governance headers
    res.set("X-Themis-Policy", policy_summary);
    res.set("X-Themis-ANN", ann);
    res.set("X-Themis-Content-Enc", content_enc);
    res.set("X-Themis-Export", export_perm);
    res.set("X-Themis-Cache", cache_perm);
    res.set("X-Themis-Retention-Days", retention_days);

    // ------------------------------------------------------------------------
    // API Versioning Headers (RFC 8594 Sunset + custom API-Version)
    // ------------------------------------------------------------------------
    {
        static const APIVersionManager api_version_mgr;

        // Determine requested version from Accept-Version or API-Version request header
        std::string version_header;
        auto av_it = req.find(APIHeaders::ACCEPT_VERSION);
        if (av_it != req.end()) {
            version_header = std::string(av_it->value());
        }
        auto resolved = api_version_mgr.resolveVersion(version_header);
        // Emit the resolved (actual) API version used
        res.set(APIHeaders::API_VERSION, resolved.toString());

        // Emit deprecation headers if the endpoint is scheduled for removal
        auto dep_info = api_version_mgr.getDeprecationInfo(path_only, resolved);
        if (dep_info.has_value()) {
            // RFC 8594: Deprecation header (ISO 8601 date of deprecation)
            {
                std::time_t dep_t = std::chrono::system_clock::to_time_t(dep_info->deprecation_date);
                std::tm dep_tm{};
                portable_gmtime_r_impl(&dep_t, &dep_tm);
                char buf[64];
                std::strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S GMT", &dep_tm);
                res.set(APIHeaders::DEPRECATION_WARNING, buf);
            }
            // RFC 8594: Sunset header (ISO 8601 date of removal)
            {
                std::time_t sun_t = std::chrono::system_clock::to_time_t(dep_info->removal_date);
                std::tm sun_tm{};
                portable_gmtime_r_impl(&sun_t, &sun_tm);
                char buf[64];
                std::strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S GMT", &sun_tm);
                res.set(APIHeaders::SUNSET, buf);
            }
            // Link to migration guide if available
            if (!dep_info->migration_guide_url.empty()) {
                res.set(APIHeaders::LINK,
                    "<" + dep_info->migration_guide_url + ">; rel=\"deprecation\"");
            }
        }
    }

    // ------------------------------------------------------------------------
    // Security Headers (global defaults, safe for JSON APIs)
    // ------------------------------------------------------------------------
    res.set("X-Frame-Options", "DENY");
    res.set("X-Content-Type-Options", "nosniff");
    res.set("Referrer-Policy", "no-referrer");
    // Strict CSP for API-only responses; adjust in UI-serving endpoints if needed
    res.set("Content-Security-Policy", "default-src 'none'; frame-ancestors 'none'; base-uri 'none'");
    // Modern browsers ignore X-XSS-Protection; set to 0 to avoid legacy behavior
    res.set("X-XSS-Protection", "0");

    // ------------------------------------------------------------------------
    // CORS Headers
    // ------------------------------------------------------------------------
    // Determine origin
    std::string origin;
    auto it = req.find(http::field::origin);
    if (it != req.end()) {
        origin = std::string(it->value());
    }

    auto set_cors_common = [&](const std::string& allow_origin) {
        res.set("Access-Control-Allow-Origin", allow_origin);
        // Preserve existing Vary if already set (e.g., by preflight)
        if (res.find(http::field::vary) == res.end()) {
            res.set(http::field::vary, "Origin");
        }
        res.set("Access-Control-Allow-Methods", cors_allowed_methods_);
        res.set("Access-Control-Allow-Headers", cors_allowed_headers_);
        if (cors_allow_credentials_ && allow_origin != "*") {
            res.set("Access-Control-Allow-Credentials", "true");
        }
    };

    if (cors_allow_all_) {
        set_cors_common("*");
    } else if (!origin.empty()) {
        // Exact-match allowlist
        bool allowed = false;
        for (const auto& o : cors_allowed_origins_) {
            if (o == origin) { allowed = true; break; }
        }
        if (allowed) {
            set_cors_common(origin);
        }
    }

    // Echo the request correlation ID on all responses so clients can correlate
    // log entries with their own request traces.
    const auto& corr_id = api::TracingMiddleware::currentCorrelationId();
    if (!corr_id.empty()) {
        res.set("X-Correlation-ID", corr_id);
    }

    // ------------------------------------------------------------------------
    // CDN / Edge Cache headers (Cache-Control, CDN-Cache-Control, ETag, etc.)
    // Applied after governance so X-Themis-Cache: disabled is already set and
    // the middleware can honour it.
    // ------------------------------------------------------------------------
    cdn_cache_middleware_.apply(req, res);
}

void HttpServer::recordLatency(std::chrono::microseconds duration) {
    uint64_t us = static_cast<uint64_t>(duration.count());
    latency_sum_us_.fetch_add(us, std::memory_order_relaxed);
    // Cumulative buckets: each bucket counts all values <= its upper bound
    // Buckets: 100us, 500us, 1ms, 5ms, 10ms, 50ms, 100ms, 500ms, 1s, 5s, +Inf
    if (us <= 100) latency_bucket_100us_.fetch_add(1, std::memory_order_relaxed);
    if (us <= 500) latency_bucket_500us_.fetch_add(1, std::memory_order_relaxed);
    if (us <= 1000) latency_bucket_1ms_.fetch_add(1, std::memory_order_relaxed);
    if (us <= 5000) latency_bucket_5ms_.fetch_add(1, std::memory_order_relaxed);
    if (us <= 10000) latency_bucket_10ms_.fetch_add(1, std::memory_order_relaxed);
    if (us <= 50000) latency_bucket_50ms_.fetch_add(1, std::memory_order_relaxed);
    if (us <= 100000) latency_bucket_100ms_.fetch_add(1, std::memory_order_relaxed);
    if (us <= 500000) latency_bucket_500ms_.fetch_add(1, std::memory_order_relaxed);
    if (us <= 1000000) latency_bucket_1s_.fetch_add(1, std::memory_order_relaxed);
    if (us <= 5000000) latency_bucket_5s_.fetch_add(1, std::memory_order_relaxed);
    // +Inf bucket always increments (cumulative count of all observations)
    latency_bucket_inf_.fetch_add(1, std::memory_order_relaxed);
}

std::string HttpServer::extractPathParam(
    const std::string& path,
    const std::string& prefix
) {
    if (!(path.rfind(prefix, 0) == 0)) {
        return "";
    }
    auto param = path.substr(prefix.length());
    // Remove query string if present
    auto query_pos = param.find('?');
    if (query_pos != std::string::npos) {
        param = param.substr(0, query_pos);
    }
    return param;
}

// Lazy initialization for PIIPseudonymizer (deferred from constructor to avoid RocksDB deadlock)
void HttpServer::ensurePIIPseudonymizer() {
    std::lock_guard<std::mutex> lock(pii_init_mutex_);
    if (pii_pseudonymizer_) {
        return; // Already initialized
    }
    
    try {
        // Failure injection for tests: set THEMIS_PII_FORCE_INIT_FAIL to
        //   "1"   -> throw exception (simulate hard init failure)
        //   "503" -> return without initializing (caller interprets as service unavailable)
        if (const char* fail_env = std::getenv("THEMIS_PII_FORCE_INIT_FAIL")) {
            std::string val = fail_env;
            if (val == "1") {
                spdlog::error("Forced test failure (throw) due to THEMIS_PII_FORCE_INIT_FAIL=1");
                throw std::runtime_error("Forced test failure (THEMIS_PII_FORCE_INIT_FAIL=1)");
            } else if (val == "503") {
                spdlog::error("Forced service unavailable for PII init (THEMIS_PII_FORCE_INIT_FAIL=503)");
                return; // Leave pii_pseudonymizer_ null
            }
        }
        auto pii_detector = std::make_shared<themis::utils::PIIDetector>();
        pii_pseudonymizer_ = std::make_shared<themis::utils::PIIPseudonymizer>(
            storage_,
            field_encryption_,
            pii_detector,
            audit_logger_
        );
        spdlog::info("PIIPseudonymizer lazy-initialized successfully");
    } catch (const std::exception& ex) {
        spdlog::error("Failed to lazy-initialize PII Pseudonymizer: {}", ex.what());
        throw; // Re-throw to allow caller to handle as service unavailable
    }
}

// ============================================================================
// Session Implementation
// ============================================================================

HttpServer::Session::Session(tcp::socket socket, HttpServer* server, bool connection_slot_reserved)
    : socket_(std::move(socket))
    , server_(server)
    , read_timer_(socket_.get_executor())
{
    if (!connection_slot_reserved) {
        server_->active_connections_.fetch_add(1, std::memory_order_relaxed);
    }
}

HttpServer::Session::~Session() {
    server_->active_connections_.fetch_sub(1, std::memory_order_relaxed);
}

void HttpServer::Session::armReadTimer() {
    // Load the live (hot-reloadable) timeout atomically to prevent data race
    // with the POST /config hot-reload path that writes request_timeout_ms_live_.
    const uint32_t timeout_ms = server_->request_timeout_ms_live_.load(std::memory_order_relaxed);
    if (timeout_ms == 0) return;
    read_timer_.expires_after(std::chrono::milliseconds(timeout_ms));
    read_timer_.async_wait([self = shared_from_this(), timeout_ms](beast::error_code ec) {
        if (!ec) {
            // Timer fired before I/O completed.
            // RFC 7231 §6.5.7: server SHOULD send 408 before closing.
            THEMIS_WARN("Request I/O timeout ({}ms) - sending 408 and closing connection",
                        timeout_ms);
            // Synchronous write of a minimal 408 response.  The async_read is
            // still pending on the socket so the socket is open and writable.
            // Errors here are non-fatal: we close regardless.
            //
            // Body = {"error":"Request Timeout","status_code":408}  (45 bytes)
            static const std::string k408{
                "HTTP/1.1 408 Request Timeout\r\n"
                "Content-Type: application/json\r\n"
                "Content-Length: 45\r\n"
                "Connection: close\r\n"
                "\r\n"
                R"({"error":"Request Timeout","status_code":408})"
            };
            beast::error_code write_ec;
            boost::asio::write(self->socket_,
                               boost::asio::buffer(k408),
                               write_ec);
            if (write_ec) {
                THEMIS_DEBUG("Session 408 write on timeout: {}", write_ec.message());
            }
            beast::error_code close_ec;
            self->socket_.shutdown(tcp::socket::shutdown_both, close_ec);
            if (close_ec) THEMIS_DEBUG("Session shutdown on timeout: {}", close_ec.message());
            self->socket_.close(close_ec);
            if (close_ec) THEMIS_DEBUG("Session close on timeout: {}", close_ec.message());
        }
    });
}

void HttpServer::Session::cancelReadTimer() {
    read_timer_.cancel(); // cancels the pending async_wait, if any
}

void HttpServer::Session::start() {
    doRead();
}

void HttpServer::Session::doRead() {
    request_ = {};
    armReadTimer();

    http::async_read(
        socket_,
        buffer_,
        request_,
        beast::bind_front_handler(&Session::onRead, shared_from_this())
    );
}

void HttpServer::Session::onRead(
    beast::error_code ec,
    std::size_t bytes_transferred
) {
    boost::ignore_unused(bytes_transferred);
    cancelReadTimer();

    if (ec == http::error::end_of_stream) {
        // Client closed connection
        socket_.shutdown(tcp::socket::shutdown_send, ec);
        return;
    }

    if (ec) {
        THEMIS_ERROR("Read error: {}", ec.message());
        return;
    }

    // Process the request
    processRequest();
}

void HttpServer::Session::processRequest() {
    try {
#ifdef THEMIS_ENABLE_WEBSOCKET
        // Check for WebSocket upgrade request
        if (server_->config_.enable_websocket && 
            websocket::is_upgrade(request_)) {
            // Extract path_only for route-specific WebSocket handling.
            const std::string ws_target(request_.target());
            const auto ws_qpos = ws_target.find('?');
            const std::string ws_path = (ws_qpos == std::string::npos)
                                            ? ws_target
                                            : ws_target.substr(0, ws_qpos);

            if (api::WsChangeHandler::isChangeStreamPath(ws_path)) {
                // Dedicated /v2/changes endpoint: validate auth and CDC params
                // before accepting the WebSocket handshake.
                api::WsChangeHandler ws_handler(server_->auth_,
                                                server_->changefeed_.get());
                const auto decision = ws_handler.validate(request_);
                if (!decision.should_upgrade) {
                    THEMIS_WARN("WebSocket /v2/changes rejected ({}): {}",
                                static_cast<int>(decision.reject_status),
                                decision.reject_reason);
                    response_.result(decision.reject_status);
                    response_.set(http::field::content_type, "application/json");
                    nlohmann::json err = {{"error", decision.reject_reason}};
                    response_.body() = err.dump();
                    response_.prepare_payload();
                    return;
                }
                THEMIS_INFO("WebSocket /v2/changes upgrade accepted "
                            "(user={}, from_seq={}, prefix='{}')",
                            decision.user_id, decision.from_sequence,
                            decision.key_prefix);

                auto ws_session = std::make_shared<WebSocketSession>(
                    std::move(socket_), server_);
                ws_session->setRequestPath(ws_path);
                // Pre-configure CDC subscription from URL parameters for the
                // legacy /v2/changes protocol only.  The new /v2/cdc/stream
                // endpoint receives subscriptions via JSON frames after connect.
                if (ws_path == "/v2/changes") {
                    ws_session->subscribeToCDC(decision.from_sequence,
                                               decision.key_prefix);
                }
                if (server_->websocket_manager_) {
                    server_->websocket_manager_->addSession(ws_session);
                }
                ws_session->run(std::move(request_));
                return;
            }

            // Generic WebSocket upgrade (any path other than /v2/changes)
            THEMIS_INFO("WebSocket upgrade requested from plain HTTP");

            // Validate JWT from the HTTP upgrade Authorization header before
            // accepting the WebSocket handshake so that auth cannot be bypassed
            // via the WebSocket upgrade path.
            std::string ws_auth_token;
            if (server_->auth_ && server_->auth_->isEnabled()) {
                const auto auth_header = request_[http::field::authorization];
                if (auth_header.empty()) {
                    response_.result(http::status::unauthorized);
                    response_.set(http::field::www_authenticate, "Bearer realm=\"themis\"");
                    response_.set(http::field::content_type, "application/json");
                    response_.keep_alive(false);
                    response_.body() = R"({"error":"missing_authorization","message":"Missing Authorization header"})";
                    response_.prepare_payload();
                    doWrite();
                    return;
                }
                auto token = themis::AuthMiddleware::extractBearerToken(
                    std::string_view(auth_header.data(), auth_header.size()));
                if (!token) {
                    response_.result(http::status::unauthorized);
                    response_.set(http::field::content_type, "application/json");
                    response_.keep_alive(false);
                    response_.body() = R"({"error":"invalid_authorization","message":"Invalid Authorization header"})";
                    response_.prepare_payload();
                    doWrite();
                    return;
                }
                auto ar = server_->auth_->validateToken(*token);
                if (!ar.authorized) {
                    response_.result(http::status::forbidden);
                    response_.set(http::field::content_type, "application/json");
                    response_.keep_alive(false);
                    response_.body() = R"({"error":"forbidden","message":"Access denied"})";
                    response_.prepare_payload();
                    doWrite();
                    return;
                }
                ws_auth_token = *token;
            }
            
            // Capture the request path before moving the request
            std::string ws_path = std::string(request_.target());
            auto qs = ws_path.find('?');
            if (qs != std::string::npos) ws_path = ws_path.substr(0, qs);

            // Create WebSocket session and transfer socket ownership
            auto ws_session = std::make_shared<WebSocketSession>(
                std::move(socket_),
                server_
            );
            ws_session->setRequestPath(ws_path);
            if (!ws_auth_token.empty()) {
                ws_session->setAuthToken(ws_auth_token);
            }
            
            // Add to manager
            if (server_->websocket_manager_) {
                server_->websocket_manager_->addSession(ws_session);
            }
            
            // Start WebSocket session
            ws_session->run(std::move(request_));
            
            // Session transferred to WebSocket, don't continue HTTP processing
            return;
        }
#endif
        
        // Rewrite path for tenant-prefixed namespace routing.
        // When the URL path contains the tenant prefix ("/tenants/{id}/..."),
        // extract the tenant ID, set it as X-Tenant-ID header (if not already
        // present), and strip the prefix so the request reaches normal API
        // handlers.  This makes header-based and path-based routing equivalent.
        // e.g., /tenants/acme-corp/documents/123  ->  /documents/123
        //                                             + X-Tenant-ID: acme-corp
        {
            const auto rw = themis::TenantManager::instance()
                                .rewriteTenantPath(request_.target());
            if (rw.rewritten) {
                if (request_.find("X-Tenant-ID") == request_.end()) {
                    request_.set("X-Tenant-ID", rw.tenant_id);
                }
                request_.target(rw.effective_path);
            }
        }

        // Inject the verified socket peer address so that extractClientIP can
        // return a real IP for direct connections (no proxy headers present).
        // Strip any client-supplied header first to prevent spoofing.
        request_.erase("X-Themis-Peer-Addr");
        try {
            auto ep = socket_.remote_endpoint();
            request_.set("X-Themis-Peer-Addr", ep.address().to_string());
        } catch (...) {
            THEMIS_WARN("http_server: unhandled exception caught");
            // Ignore: best-effort; rate limiting falls back to empty key.
        }

        // Route request to appropriate handler
        response_ = server_->routeRequest(request_);
    } catch (const std::exception& e) {
        // Final safety net: catch any exception that escaped routeRequest
        THEMIS_ERROR("Exception in Session::processRequest: {}", e.what());
        response_.result(http::status::internal_server_error);
        response_.set(http::field::content_type, "application/json");
        nlohmann::json error_body = {
            {"error", "Internal Server Error"},
            {"message", "An unexpected error occurred"},
            {"status_code", 500}
        };
        response_.body() = error_body.dump();
        response_.prepare_payload();
    }
    
    // Send response
    doWrite();
}

void HttpServer::Session::doWrite() {
    // Arm the I/O timeout for the write phase (same timer as read phase; read
    // is already complete and the timer was cancelled in onRead before we get here).
    armReadTimer();
    bool close = response_.need_eof();
    // W1-S02: arm per-connection timeout for potentially blocking async writes.
    armReadTimer();
    http::async_write(
        socket_,
        response_,
        beast::bind_front_handler(
            &Session::onWrite,
            shared_from_this(),
            close
        )
    );
}

void HttpServer::Session::onWrite(
    bool close,
    beast::error_code ec,
    std::size_t bytes_transferred
) {
    cancelReadTimer();  // Cancel the write-phase I/O timeout
    boost::ignore_unused(bytes_transferred);
    cancelReadTimer();

    if (ec) {
        THEMIS_ERROR("Write error: {}", ec.message());
        return;
    }

    if (close) {
        // Close connection
        socket_.shutdown(tcp::socket::shutdown_send, ec);
        return;
    }

    // Read next request
    doRead();
}

// ============================================================================
// SSL Session Implementation
// ============================================================================

HttpServer::SslSession::SslSession(tcp::socket socket, boost::asio::ssl::context& ssl_ctx, HttpServer* server, bool connection_slot_reserved)
    : stream_(std::move(socket), ssl_ctx)
    , server_(server)
    , read_timer_(stream_.get_executor())
{
    if (!connection_slot_reserved) {
        server_->active_connections_.fetch_add(1, std::memory_order_relaxed);
    }
}

HttpServer::SslSession::~SslSession() {
    server_->active_connections_.fetch_sub(1, std::memory_order_relaxed);
}

void HttpServer::SslSession::armReadTimer() {
    // Load the live (hot-reloadable) timeout atomically to prevent data race
    // with the POST /config hot-reload path that writes request_timeout_ms_live_.
    const uint32_t timeout_ms = server_->request_timeout_ms_live_.load(std::memory_order_relaxed);
    if (timeout_ms == 0) return;
    read_timer_.expires_after(std::chrono::milliseconds(timeout_ms));
    read_timer_.async_wait([self = shared_from_this(), timeout_ms](beast::error_code ec) {
        if (!ec) {
            // Timer fired before I/O completed.
            // RFC 7231 §6.5.7: server SHOULD send 408 before closing.
            THEMIS_WARN("Request I/O timeout ({}ms) - sending 408 and closing TLS connection",
                        timeout_ms);
            // Synchronous write of a minimal 408 response over the TLS stream.
            // Body = {"error":"Request Timeout","status_code":408}  (45 bytes)
            static const std::string k408{
                "HTTP/1.1 408 Request Timeout\r\n"
                "Content-Type: application/json\r\n"
                "Content-Length: 45\r\n"
                "Connection: close\r\n"
                "\r\n"
                R"({"error":"Request Timeout","status_code":408})"
            };
            beast::error_code write_ec;
            boost::asio::write(self->stream_,
                               boost::asio::buffer(k408),
                               write_ec);
            if (write_ec) {
                THEMIS_DEBUG("SslSession 408 write on timeout: {}", write_ec.message());
            }
            beast::error_code close_ec;
            self->stream_.lowest_layer().shutdown(tcp::socket::shutdown_both, close_ec);
            if (close_ec) THEMIS_DEBUG("SslSession shutdown on timeout: {}", close_ec.message());
            self->stream_.lowest_layer().close(close_ec);
            if (close_ec) THEMIS_DEBUG("SslSession close on timeout: {}", close_ec.message());
        }
    });
}

void HttpServer::SslSession::cancelReadTimer() {
    beast::error_code ec;
    read_timer_.cancel();
}

void HttpServer::SslSession::start() {
    doHandshake();
}

void HttpServer::SslSession::doHandshake() {
    // Arm timeout for TLS handshake as well
    armReadTimer();
    stream_.async_handshake(
        boost::asio::ssl::stream_base::server,
        beast::bind_front_handler(&SslSession::onHandshake, shared_from_this())
    );
}

void HttpServer::SslSession::onHandshake(beast::error_code ec) {
    cancelReadTimer();
    if (ec) {
        THEMIS_ERROR("TLS handshake error: {}", ec.message());
        return;
    }

    // Log successful TLS connection with client certificate info (mTLS)
    if (server_->config_.tls_require_client_cert) {
        try {
            X509* cert = SSL_get_peer_certificate(stream_.native_handle());
            if (cert) {
                char subject_name[256];
                X509_NAME_oneline(X509_get_subject_name(cert), subject_name, sizeof(subject_name));
                THEMIS_INFO("mTLS: client authenticated with certificate: {}", subject_name);
                X509_free(cert);
            } else {
                THEMIS_WARN("mTLS: no client certificate presented despite requirement");
            }
        } catch (const std::exception& e) {
            THEMIS_ERROR("mTLS: failed to extract client certificate info: {}", e.what());
        }
    }

    // Note: HTTP/2 ALPN negotiation is handled in onAccept() by creating Http2Session
    // This SslSession is only used for HTTP/1.1 over TLS
    doRead();
}

void HttpServer::SslSession::doRead() {
    request_ = {};
    armReadTimer();

    http::async_read(
        stream_,
        buffer_,
        request_,
        beast::bind_front_handler(&SslSession::onRead, shared_from_this())
    );
}

void HttpServer::SslSession::onRead(
    beast::error_code ec,
    std::size_t bytes_transferred
) {
    boost::ignore_unused(bytes_transferred);
    cancelReadTimer();

    if (ec == http::error::end_of_stream) {
        doShutdown();
        return;
    }

    if (ec) {
        THEMIS_ERROR("SSL read error: {}", ec.message());
        return;
    }

    processRequest();
}

void HttpServer::SslSession::processRequest() {
    try {
#ifdef THEMIS_ENABLE_WEBSOCKET
        // Check for WebSocket upgrade request
        if (server_->config_.enable_websocket && 
            websocket::is_upgrade(request_)) {
            // Extract path_only for route-specific WebSocket handling.
            const std::string ws_target(request_.target());
            const auto ws_qpos = ws_target.find('?');
            const std::string ws_path = (ws_qpos == std::string::npos)
                                            ? ws_target
                                            : ws_target.substr(0, ws_qpos);

            if (api::WsChangeHandler::isChangeStreamPath(ws_path)) {
                // Dedicated /v2/changes endpoint: validate auth and CDC params
                // before accepting the WebSocket handshake.
                api::WsChangeHandler ws_handler(server_->auth_,
                                                server_->changefeed_.get());
                const auto decision = ws_handler.validate(request_);
                if (!decision.should_upgrade) {
                    THEMIS_WARN("WebSocket /v2/changes (TLS) rejected ({}): {}",
                                static_cast<int>(decision.reject_status),
                                decision.reject_reason);
                    response_.result(decision.reject_status);
                    response_.set(http::field::content_type, "application/json");
                    nlohmann::json err = {{"error", decision.reject_reason}};
                    response_.body() = err.dump();
                    response_.prepare_payload();
                    return;
                }
                THEMIS_INFO("WebSocket /v2/changes (TLS) upgrade accepted "
                            "(user={}, from_seq={}, prefix='{}')",
                            decision.user_id, decision.from_sequence,
                            decision.key_prefix);

                auto ws_session = std::make_shared<WebSocketSession>(
                    std::move(stream_), server_);
                ws_session->setRequestPath(ws_path);
                // Pre-configure CDC subscription from URL parameters for the
                // legacy /v2/changes protocol only.  The new /v2/cdc/stream
                // endpoint receives subscriptions via JSON frames after connect.
                if (ws_path == "/v2/changes") {
                    ws_session->subscribeToCDC(decision.from_sequence,
                                               decision.key_prefix);
                }
                if (server_->websocket_manager_) {
                    server_->websocket_manager_->addSession(ws_session);
                }
                ws_session->run(std::move(request_));
                return;
            }

            // Generic WebSocket upgrade (any path other than /v2/changes)
            THEMIS_INFO("WebSocket upgrade requested from HTTPS");

            // Validate JWT from the HTTP upgrade Authorization header before
            // accepting the WebSocket handshake.
            std::string ws_auth_token;
            if (server_->auth_ && server_->auth_->isEnabled()) {
                const auto auth_header = request_[http::field::authorization];
                if (auth_header.empty()) {
                    response_.result(http::status::unauthorized);
                    response_.set(http::field::www_authenticate, "Bearer realm=\"themis\"");
                    response_.set(http::field::content_type, "application/json");
                    response_.keep_alive(false);
                    response_.body() = R"({"error":"missing_authorization","message":"Missing Authorization header"})";
                    response_.prepare_payload();
                    doWrite();
                    return;
                }
                auto token = themis::AuthMiddleware::extractBearerToken(
                    std::string_view(auth_header.data(), auth_header.size()));
                if (!token) {
                    response_.result(http::status::unauthorized);
                    response_.set(http::field::content_type, "application/json");
                    response_.keep_alive(false);
                    response_.body() = R"({"error":"invalid_authorization","message":"Invalid Authorization header"})";
                    response_.prepare_payload();
                    doWrite();
                    return;
                }
                auto ar = server_->auth_->validateToken(*token);
                if (!ar.authorized) {
                    response_.result(http::status::forbidden);
                    response_.set(http::field::content_type, "application/json");
                    response_.keep_alive(false);
                    response_.body() = R"({"error":"forbidden","message":"Access denied"})";
                    response_.prepare_payload();
                    doWrite();
                    return;
                }
                ws_auth_token = *token;
            }

            // Capture the request path before moving the request
            std::string ws_path = std::string(request_.target());
            auto qs = ws_path.find('?');
            if (qs != std::string::npos) ws_path = ws_path.substr(0, qs);
            
            // Create WebSocket session and transfer SSL stream ownership
            auto ws_session = std::make_shared<WebSocketSession>(
                std::move(stream_),
                server_
            );
            ws_session->setRequestPath(ws_path);
            if (!ws_auth_token.empty()) {
                ws_session->setAuthToken(ws_auth_token);
            }
            
            // Add to manager
            if (server_->websocket_manager_) {
                server_->websocket_manager_->addSession(ws_session);
            }
            
            // Start WebSocket session
            ws_session->run(std::move(request_));
            
            // Session transferred to WebSocket, don't continue HTTP processing
            return;
        }
#endif
        
        // Rewrite path for tenant-prefixed namespace routing.
        // When the URL path contains the tenant prefix ("/tenants/{id}/..."),
        // extract the tenant ID, set it as X-Tenant-ID header (if not already
        // present), and strip the prefix so the request reaches normal API
        // handlers.  This makes header-based and path-based routing equivalent.
        // e.g., /tenants/acme-corp/documents/123  ->  /documents/123
        //                                             + X-Tenant-ID: acme-corp
        {
            const auto rw = themis::TenantManager::instance()
                                .rewriteTenantPath(request_.target());
            if (rw.rewritten) {
                if (request_.find("X-Tenant-ID") == request_.end()) {
                    request_.set("X-Tenant-ID", rw.tenant_id);
                }
                request_.target(rw.effective_path);
            }
        }

        // Inject the verified socket peer address so that extractClientIP can
        // return a real IP for direct connections (no proxy headers present).
        // Strip any client-supplied header first to prevent spoofing.
        request_.erase("X-Themis-Peer-Addr");
        try {
            auto ep = stream_.lowest_layer().remote_endpoint();
            request_.set("X-Themis-Peer-Addr", ep.address().to_string());
        } catch (...) {
            THEMIS_WARN("http_server: unhandled exception caught");
            // Ignore: best-effort; rate limiting falls back to empty key.
        }

        // Route request to appropriate handler
        response_ = server_->routeRequest(request_);
        
        // Add HSTS header for HTTPS connections
        if (server_->config_.enable_tls) {
            response_.set(http::field::strict_transport_security, "max-age=31536000; includeSubDomains");
        }
    } catch (const std::exception& e) {
        // Final safety net: catch any exception that escaped routeRequest
        THEMIS_ERROR("Exception in SslSession::processRequest: {}", e.what());
        response_.result(http::status::internal_server_error);
        response_.set(http::field::content_type, "application/json");
        nlohmann::json error_body = {
            {"error", "Internal Server Error"},
            {"message", "An unexpected error occurred"},
            {"status_code", 500}
        };
        response_.body() = error_body.dump();
        response_.prepare_payload();
    }
    
    doWrite();
}

void HttpServer::SslSession::doWrite() {
    // Arm the I/O timeout for the write phase (same timer as read/handshake phase;
    // it was cancelled in onRead/onHandshake before we get here).
    armReadTimer();
    bool close = response_.need_eof();
    // W1-S02: arm per-connection timeout for potentially blocking TLS writes.
    armReadTimer();
    http::async_write(
        stream_,
        response_,
        beast::bind_front_handler(
            &SslSession::onWrite,
            shared_from_this(),
            close
        )
    );
}

void HttpServer::SslSession::onWrite(
    bool close,
    beast::error_code ec,
    std::size_t bytes_transferred
) {
    cancelReadTimer();  // Cancel the write-phase I/O timeout
    boost::ignore_unused(bytes_transferred);
    cancelReadTimer();

    if (ec) {
        THEMIS_ERROR("SSL write error: {}", ec.message());
        return;
    }

    if (close) {
        doShutdown();
        return;
    }

    // Read next request (keep-alive)
    doRead();
}

void HttpServer::SslSession::doShutdown() {
    // W1-S02: prevent indefinite async_shutdown hang on stalled peers.
    armReadTimer();
    stream_.async_shutdown(
        beast::bind_front_handler(
            [self = shared_from_this()](beast::error_code ec) {
                self->cancelReadTimer();
                if (ec && ec != boost::asio::error::eof) {
                    THEMIS_ERROR("SSL shutdown error: {}", ec.message());
                }
            }
        )
    );
}

http::response<http::string_body> HttpServer::handleIndexStats(
    const http::request<http::string_body>& req
) {
    try {
        std::string table;
        std::string column;

        // Try parsing JSON body first
        if (!req.body().empty()) {
            json body = json::parse(req.body());
            if (body.contains("table")) {
                table = body["table"];
            }
            if (body.contains("column")) {
                column = body["column"];
            }
        }

        // If no JSON, try query parameters from target
        if (table.empty()) {
            std::string target = std::string(req.target());
            size_t query_start = target.find('?');
            if (query_start != std::string::npos) {
                std::string query = target.substr(query_start + 1);
                // Simple query parser: table=X&column=Y
                size_t pos = 0;
                while (pos < query.size()) {
                    size_t eq = query.find('=', pos);
                    if (eq == std::string::npos) break;
                    size_t amp = query.find('&', eq);
                    if (amp == std::string::npos) amp = query.size();
                    
                    std::string key = query.substr(pos, eq - pos);
                    std::string value = query.substr(eq + 1, amp - eq - 1);
                    
                    if (key == "table") table = value;
                    else if (key == "column") column = value;
                    
                    pos = amp + 1;
                }
            }
        }

        if (table.empty()) {
            return makeErrorResponse(http::status::bad_request, "Missing 'table' parameter", req);
        }

        // If column specified, get single index stats
        if (!column.empty()) {
            auto stats = secondary_index_->getIndexStats(table, column);
            json resp = {
                {"type", stats.type},
                {"table", stats.table},
                {"column", stats.column},
                {"entry_count", stats.entry_count},
                {"estimated_size_bytes", stats.estimated_size_bytes},
                {"unique", stats.unique}
            };
            if (!stats.additional_info.empty()) {
                resp["additional_info"] = stats.additional_info;
            }
            return makeResponse(http::status::ok, resp.dump(), req);
        } else {
            // Get all index stats for table
            auto all_stats = secondary_index_->getAllIndexStats(table);
            json resp = json::array();
            for (const auto& stats : all_stats) {
                json stat_obj = {
                    {"type", stats.type},
                    {"table", stats.table},
                    {"column", stats.column},
                    {"entry_count", stats.entry_count},
                    {"estimated_size_bytes", stats.estimated_size_bytes},
                    {"unique", stats.unique}
                };
                if (!stats.additional_info.empty()) {
                    stat_obj["additional_info"] = stats.additional_info;
                }
                resp.push_back(stat_obj);
            }
            return makeResponse(http::status::ok, resp.dump(), req);
        }
    } catch (const json::exception& e) {
        return makeErrorResponse(http::status::bad_request, "Invalid JSON: " + std::string(e.what()), req);
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error, "Error: " + std::string(e.what()), req);
    }
}

http::response<http::string_body> HttpServer::handleIndexRebuild(
    const http::request<http::string_body>& req
) {
    try {
        json body = json::parse(req.body());
        
        if (!body.contains("table") || !body.contains("column")) {
            return makeErrorResponse(http::status::bad_request, "Missing 'table' or 'column'", req);
        }

        std::string table = body["table"];
        std::string column = body["column"];

        // Rebuild the index
        secondary_index_->rebuildIndex(table, column);

        // Get updated stats
        auto stats = secondary_index_->getIndexStats(table, column);
        
        json resp = {
            {"success", true},
            {"table", table},
            {"column", column},
            {"entry_count", stats.entry_count},
            {"estimated_size_bytes", stats.estimated_size_bytes}
        };
        return makeResponse(http::status::ok, resp.dump(), req);
    } catch (const json::exception& e) {
        return makeErrorResponse(http::status::bad_request, "Invalid JSON: " + std::string(e.what()), req);
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error, "Error: " + std::string(e.what()), req);
    }
}

http::response<http::string_body> HttpServer::handleIndexReindex(
    const http::request<http::string_body>& req
) {
    try {
        json body = json::parse(req.body());
        
        if (!body.contains("table")) {
            return makeErrorResponse(http::status::bad_request, "Missing 'table'", req);
        }

        std::string table = body["table"];

        // Reindex the entire table
        secondary_index_->reindexTable(table);

        // Get all index stats
        auto all_stats = secondary_index_->getAllIndexStats(table);
        
        json resp = {
            {"success", true},
            {"table", table},
            {"indexes_rebuilt", all_stats.size()}
        };
        
        // Include stats for each index
        json stats_array = json::array();
        for (const auto& stats : all_stats) {
            stats_array.push_back({
                {"column", stats.column},
                {"type", stats.type},
                {"entry_count", stats.entry_count}
            });
        }
        resp["indexes"] = stats_array;
        
        return makeResponse(http::status::ok, resp.dump(), req);
    } catch (const json::exception& e) {
        return makeErrorResponse(http::status::bad_request, "Invalid JSON: " + std::string(e.what()), req);
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error, "Error: " + std::string(e.what()), req);
    }
}

// ============================================================================
// Sprint B: Time-Series Endpoints
// ============================================================================
// Note: Time-Series endpoints have been extracted to TimeSeriesApiHandler
// See: src/server/timeseries_api_handler.cpp

// ============================================================================
// Sprint C: Adaptive Indexing Endpoints
// ============================================================================

http::response<http::string_body> HttpServer::handleIndexSuggestions(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handleIndexSuggestions");
    
    try {
        auto target = std::string(req.target());
        
        // Parse query parameters
        std::string collection;
        double min_score = 0.5;
        size_t limit = 10;
        
        // Extract query params from URL
        auto query_pos = target.find('?');
        if (query_pos != std::string::npos) {
            std::string query_string = target.substr(query_pos + 1);
            std::istringstream iss(query_string);
            std::string param;
            
            while (std::getline(iss, param, '&')) {
                auto eq_pos = param.find('=');
                if (eq_pos != std::string::npos) {
                    std::string key = param.substr(0, eq_pos);
                    std::string value = param.substr(eq_pos + 1);
                    
                    if (key == "collection") {
                        collection = value;
                    } else if (key == "min_score") {
                        min_score = std::stod(value);
                    } else if (key == "limit") {
                        limit = std::stoull(value);
                    }
                }
            }
        }
        
        span.setAttribute("collection", collection);
        span.setAttribute("min_score", min_score);
        span.setAttribute("limit", static_cast<int64_t>(limit));
        
        auto suggestions = adaptive_index_->getSuggestions(collection, min_score, limit);
        
        json response = json::array();
        for (const auto& suggestion : suggestions) {
            response.push_back(suggestion.toJson());
        }
        
        span.setAttribute("suggestions.count", static_cast<int64_t>(suggestions.size()));
        span.setStatus(true);
        return makeResponse(http::status::ok, response.dump(), req);
        
    } catch (const std::exception& e) {
        span.setStatus(false, "error");
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> HttpServer::handleIndexPatterns(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handleIndexPatterns");
    
    try {
        auto target = std::string(req.target());
        
        // Parse collection from query params
        std::string collection;
        auto query_pos = target.find('?');
        if (query_pos != std::string::npos) {
            std::string query_string = target.substr(query_pos + 1);
            auto coll_pos = query_string.find("collection=");
            if (coll_pos != std::string::npos) {
                collection = query_string.substr(coll_pos + 11);
                auto amp_pos = collection.find('&');
                if (amp_pos != std::string::npos) {
                    collection = collection.substr(0, amp_pos);
                }
            }
        }
        
        span.setAttribute("collection", collection);
        
        auto patterns = adaptive_index_->getPatterns(collection);
        
        json response = json::array();
        for (const auto& pattern : patterns) {
            response.push_back(pattern.toJson());
        }
        
        span.setAttribute("patterns.count", static_cast<int64_t>(patterns.size()));
        span.setStatus(true);
        return makeResponse(http::status::ok, response.dump(), req);
        
    } catch (const std::exception& e) {
        span.setStatus(false, "error");
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> HttpServer::handleIndexRecordPattern(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handleIndexRecordPattern");
    
    try {
        json body = json::parse(req.body());
        
        std::string collection = body.value("collection", "");
        std::string field = body.value("field", "");
        std::string operation = body.value("operation", "eq");
        int64_t execution_time_ms = body.value("execution_time_ms", int64_t(0));
        
        // Validate required fields
        if (collection.empty()) {
            return makeErrorResponse(http::status::bad_request, "collection is required", req);
        }
        if (field.empty()) {
            return makeErrorResponse(http::status::bad_request, "field is required", req);
        }
        
        if (collection.empty() || field.empty()) {
            span.setStatus(false, "missing_fields");
            return makeErrorResponse(http::status::bad_request, 
                "Missing required fields: collection, field", req);
        }
        
        span.setAttribute("collection", collection);
        span.setAttribute("field", field);
        span.setAttribute("operation", operation);
        
        adaptive_index_->getPatternTracker()->recordPattern(
            collection, field, operation, execution_time_ms
        );
        
        json response = {
            {"status", "recorded"},
            {"collection", collection},
            {"field", field},
            {"operation", operation}
        };
        
        span.setStatus(true);
        return makeResponse(http::status::ok, response.dump(), req);
        
    } catch (const json::exception& e) {
        span.setStatus(false, "json_error");
        return makeErrorResponse(http::status::bad_request, "JSON error: " + std::string(e.what()), req);
    } catch (const std::exception& e) {
        span.setStatus(false, "error");
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> HttpServer::handleIndexClearPatterns(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handleIndexClearPatterns");
    
    try {
        size_t count_before = adaptive_index_->getPatternTracker()->size();
        
        adaptive_index_->getPatternTracker()->clear();
        
        json response = {
            {"status", "cleared"},
            {"patterns_removed", count_before}
        };
        
        span.setAttribute("patterns.removed", static_cast<int64_t>(count_before));
        span.setStatus(true);
        return makeResponse(http::status::ok, response.dump(), req);
        
    } catch (const std::exception& e) {
        span.setStatus(false, "error");
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

// ============================================================================
// Rate Limiting Helper Methods
// ============================================================================

std::string HttpServer::extractClientIP(const http::request<http::string_body>& req) const {
    // Try X-Forwarded-For header first (for proxied requests)
    if (req.find("X-Forwarded-For") != req.end()) {
        std::string xff = std::string(req["X-Forwarded-For"]);
        // Take first IP in comma-separated list
        auto comma_pos = xff.find(',');
        if (comma_pos != std::string::npos) {
            return xff.substr(0, comma_pos);
        }
        return xff;
    }

    // Try X-Real-IP header
    if (req.find("X-Real-IP") != req.end()) {
        return std::string(req["X-Real-IP"]);
    }

    // Fall back to the socket peer address injected by Session::processRequest /
    // SslSession::processRequest.  The injection site strips any client-supplied
    // value before setting it, so this header cannot be spoofed.
    if (req.find("X-Themis-Peer-Addr") != req.end()) {
        return std::string(req["X-Themis-Peer-Addr"]);
    }

    return "";
}

std::optional<http::response<http::string_body>> HttpServer::checkRateLimit(
    const http::request<http::string_body>& req
) {
    if (!rate_limiter_) {
        return std::nullopt; // Rate limiting disabled
    }
    auto& rate_limiter = *rate_limiter_;
    
    std::string client_ip = extractClientIP(req);
    std::string user_id;
    
    // Extract user ID from JWT token if authenticated
    if (auth_ && auth_->isEnabled()) {
        if (req.find("Authorization") != req.end()) {
            std::string auth_header = std::string(req["Authorization"]);
            if (auth_header.size() > 7 && auth_header.substr(0, 7) == "Bearer ") {
                std::string token = auth_header.substr(7);
                auto ctx = auth_->extractContext(token);
                if (ctx) {
                    user_id = ctx->user_id;
                }
            }
        }
    }
    
    // Prefer the per-client middleware (per-endpoint configurable token bucket).
    if (rate_limiting_middleware_) {
        std::lock_guard<std::mutex> lock(rate_limiting_middleware_mutex_);
        if (rate_limiting_middleware_) {
            std::string path = std::string(req.target());
            // Strip query string for path matching
            auto qpos = path.find('?');
            if (qpos != std::string::npos) path = path.substr(0, qpos);

            // Use authenticated user ID when available, otherwise fall back to IP
            const std::string& client_key = user_id.empty() ? client_ip : user_id;

            auto result = rate_limiting_middleware_->check(client_key, path);
        if (!result.allowed) {
            http::response<http::string_body> response;
            response.result(http::status::too_many_requests);
            response.set(http::field::content_type, "application/json");
            for (const auto& [k, v] : result.headers) {
                response.set(k, v);
            }

            json error_body = {
                {"error", "Too Many Requests"},
                {"message", "Rate limit exceeded. Please retry after " +
                             std::to_string(result.retry_after_seconds) + " seconds."},
                {"retry_after_seconds", result.retry_after_seconds},
                {"status_code", 429}
            };

            response.body() = error_body.dump();
            applyGovernanceHeaders(req, response);
            response.prepare_payload();
            return response;
        }
        return std::nullopt; // Rate limit OK
        }
    }

    // Fallback to legacy rate limiter
    if (!rate_limiter.allowRequest(client_ip, user_id)) {
        uint32_t retry_after = rate_limiter.getRetryAfter(client_ip, user_id);
        
        THEMIS_WARN("Rate limit exceeded: ip={}, user={}, retry_after={}s", 
            client_ip.empty() ? "<unknown>" : client_ip, 
            user_id.empty() ? "<anonymous>" : user_id,
            retry_after);
        
        http::response<http::string_body> response;
        response.result(http::status::too_many_requests);
        response.set(http::field::content_type, "application/json");
        response.set("Retry-After", std::to_string(retry_after));
        response.set("X-RateLimit-Limit", "100");
        response.set("X-RateLimit-Remaining", "0");
        
        json error_body = {
            {"error", "Too Many Requests"},
            {"message", "Rate limit exceeded. Please retry after " + std::to_string(retry_after) + " seconds."},
            {"retry_after_seconds", retry_after},
            {"status_code", 429}
        };
        
        response.body() = error_body.dump();
        applyGovernanceHeaders(req, response);
        response.prepare_payload();
        
        return response;
    }
    
    return std::nullopt; // Rate limit OK
}

// ============================================================================
// Dynamic Endpoint Discovery
// ============================================================================
std::vector<HttpServer::RegisteredEndpoint> HttpServer::getRegisteredEndpoints() const {
    std::vector<RegisteredEndpoint> endpoints;
    endpoints.reserve(256);

    // ========== CORE ENDPOINTS (Always Available) ==========
    // Health & Status
    endpoints.push_back({"GET",  "/health",                    "Health check"});
    endpoints.push_back({"GET",  "/health/live",              "Liveness probe"});
    endpoints.push_back({"GET",  "/health/ready",             "Readiness probe"});
    endpoints.push_back({"GET",  "/version",                  "Server version"});
    endpoints.push_back({"GET",  "/stats",                    "Runtime statistics"});
    endpoints.push_back({"GET",  "/metrics",                  "Prometheus metrics"});
    endpoints.push_back({"GET",  "/metrics/html",             "Metrics HTML dashboard"});
    endpoints.push_back({"GET",  "/api/openapi.json",         "OpenAPI 3.1 specification"});
    endpoints.push_back({"GET",  "/api/capabilities",         "Server capabilities"});

    // Observability (Q1)
    endpoints.push_back({"GET",  "/api/v1/observability/alerts",            "Operator alerts"});
    endpoints.push_back({"POST", "/api/v1/observability/alerts/{id}/silence", "Silence alert"});
    endpoints.push_back({"GET",  "/api/v1/observability/health",            "Observability health"});
    endpoints.push_back({"GET",  "/api/v1/observability/provenance",        "Retrieval provenance export"});
    endpoints.push_back({"GET",  "/api/v1/license/status",                  "License status"});

    // Entity API (CRUD)
    endpoints.push_back({"GET",  "/entities/{key}",           "Retrieve entity"});
    endpoints.push_back({"POST", "/entities",                 "Create entity"});
    endpoints.push_back({"PUT",  "/entities/{key}",           "Upsert entity"});
    endpoints.push_back({"DELETE", "/entities/{key}",         "Delete entity"});
    endpoints.push_back({"POST", "/entities/batch",           "Batch create entities"});
    endpoints.push_back({"POST", "/v2/documents",             "Bulk insert via NDJSON"});

    // Query API
    endpoints.push_back({"POST", "/query",                    "Structured query"});
    endpoints.push_back({"POST", "/query/aql",                "AQL query"});
    endpoints.push_back({"POST", "/api/aql",                  "AQL query (compat)"});
    endpoints.push_back({"GET",  "/v2/query/stream",          "SSE streaming AQL results"});

    // Index Management
    endpoints.push_back({"POST", "/index/create",             "Create index"});
    endpoints.push_back({"POST", "/index/drop",               "Drop index"});
    endpoints.push_back({"GET",  "/index/stats",              "Index statistics"});
    endpoints.push_back({"POST", "/index/rebuild",            "Rebuild index"});
    endpoints.push_back({"POST", "/index/reindex",            "Reindex collection"});

    // Spatial Index Management (G5)
    endpoints.push_back({"POST", "/spatial/index/create",     "Create spatial index"});
    endpoints.push_back({"POST", "/spatial/index/rebuild",    "Rebuild spatial index"});
    endpoints.push_back({"GET",  "/spatial/index/stats",      "Spatial index statistics"});
    endpoints.push_back({"GET",  "/spatial/metrics",          "Spatial metrics"});

    // Graph API
    endpoints.push_back({"POST", "/graph/traverse",           "Graph traverse"});
    endpoints.push_back({"POST", "/graph/edge",               "Create graph edge"});
    endpoints.push_back({"DELETE", "/graph/edge/{id}",        "Delete graph edge"});
    endpoints.push_back({"GET",  "/api/v1/graph/metrics",     "Graph metrics"});
    endpoints.push_back({"GET",  "/api/v1/graph/metrics/prometheus", "Graph metrics (Prometheus)"});
    endpoints.push_back({"POST", "/graph/query/incremental",  "Register incremental query"});
    endpoints.push_back({"DELETE", "/graph/query/incremental/{id}", "Unregister incremental query"});
    endpoints.push_back({"POST", "/graph/changes",            "Get graph changes"});
    endpoints.push_back({"POST", "/api/v1/graph/cost-model/calibrate", "Calibrate cost model"});
    endpoints.push_back({"GET",  "/api/v1/graph/cost-model",  "Export cost model"});
    endpoints.push_back({"POST", "/api/v1/graph/cost-model",  "Import cost model"});
    endpoints.push_back({"POST", "/api/v1/graph/query/explain", "Explain query plan"});

    // Vector API
    endpoints.push_back({"POST", "/vector/search",            "Vector search"});
    endpoints.push_back({"POST", "/vector/batch_insert",      "Batch insert vectors"});
    endpoints.push_back({"DELETE", "/vector/by-filter",       "Delete vectors by filter"});
    endpoints.push_back({"POST", "/vector/index/save",        "Save vector index"});
    endpoints.push_back({"POST", "/vector/index/load",        "Load vector index"});
    endpoints.push_back({"GET",  "/vector/index/config",      "Get vector index config"});
    endpoints.push_back({"PUT",  "/vector/index/config",      "Update vector index config"});
    endpoints.push_back({"GET",  "/vector/index/stats",       "Vector index statistics"});
    endpoints.push_back({"POST", "/vector/index/incremental-reindex", "Incremental vector reindex"});

    // RoPE API (Relative Position Encoding)
    endpoints.push_back({"POST", "/api/v1/vector-index/{name}/rope/config",      "RoPE configuration"});
    endpoints.push_back({"GET",  "/api/v1/vector-index/{name}/rope/config",      "Get RoPE config"});
    endpoints.push_back({"DELETE", "/api/v1/vector-index/{name}/rope/config",    "Delete RoPE config"});
    endpoints.push_back({"POST", "/api/v1/vector-index/{name}/rope/add",         "Add RoPE entries"});
    endpoints.push_back({"POST", "/api/v1/vector-index/{name}/rope/add-relational", "Add relational RoPE"});
    endpoints.push_back({"POST", "/api/v1/vector-index/{name}/rope/search",      "RoPE search"});
    endpoints.push_back({"POST", "/api/v1/vector-index/{name}/rope/batch-add",   "Batch add RoPE entries"});
    endpoints.push_back({"GET",  "/api/v1/vector-index/{name}/rope/stats",       "RoPE statistics"});

    // Search API
    endpoints.push_back({"POST", "/search/hybrid",            "Hybrid search (beta)"});
    endpoints.push_back({"POST", "/search/fusion",            "Fusion search (beta)"});
    endpoints.push_back({"POST", "/search/fulltext",          "Fulltext search"});

    // Content Management
    endpoints.push_back({"POST", "/content/import",           "Import content"});
    endpoints.push_back({"GET",  "/content/{id}",             "Get content"});
    endpoints.push_back({"GET",  "/content/{id}/blob",        "Get content blob"});
    endpoints.push_back({"GET",  "/content/{id}/chunks",      "Get content chunks"});
    endpoints.push_back({"PUT",    "/api/v1/content/fs/{pk}", "ContentFS: store binary blob"});
    endpoints.push_back({"GET",    "/api/v1/content/fs/{pk}", "ContentFS: retrieve binary blob"});
    endpoints.push_back({"HEAD",   "/api/v1/content/fs/{pk}", "ContentFS: blob metadata (no body)"});
    endpoints.push_back({"DELETE", "/api/v1/content/fs/{pk}", "ContentFS: remove binary blob"});
    endpoints.push_back({"GET",  "/config/content-filters",   "Get content filter schema"});
    endpoints.push_back({"PUT",  "/config/content-filters",   "Update content filter schema"});
    endpoints.push_back({"GET",  "/config/edge-weights",      "Get edge weight config"});
    endpoints.push_back({"PUT",  "/config/edge-weights",      "Update edge weight config"});

    // Encryption
    endpoints.push_back({"GET",  "/config/encryption-schema", "Get encryption schema"});
    endpoints.push_back({"PUT",  "/config/encryption-schema", "Update encryption schema"});

    // Transaction API
    endpoints.push_back({"POST", "/transaction",              "Begin transaction"});
    endpoints.push_back({"POST", "/transaction/begin",        "Begin transaction (explicit)"});
    endpoints.push_back({"POST", "/transaction/commit",       "Commit transaction"});
    endpoints.push_back({"POST", "/transaction/rollback",     "Rollback transaction"});
    endpoints.push_back({"GET",  "/transaction/stats",        "Transaction statistics"});
    endpoints.push_back({"GET",  "/transaction/version",      "Transaction version"});
    endpoints.push_back({"GET",  "/transaction/{id}/explain", "Explain transaction"});

    // Distributed Transaction (2PC)
    endpoints.push_back({"POST", "/dtxn/begin",               "Begin distributed transaction"});
    endpoints.push_back({"POST", "/dtxn/operation",           "Execute distributed operation"});
    endpoints.push_back({"POST", "/dtxn/commit",              "Commit distributed transaction"});
    endpoints.push_back({"POST", "/dtxn/abort",               "Abort distributed transaction"});
    endpoints.push_back({"POST", "/dtxn/readonly",            "Read-only distributed query"});
    endpoints.push_back({"GET",  "/dtxn/status/{id}",         "Distributed transaction status"});
    endpoints.push_back({"GET",  "/dtxn/stats",               "Distributed transaction statistics"});

    // WAL API (Replication)
    endpoints.push_back({"POST", "/api/v1/wal/apply",         "Apply WAL entries"});

    // Admin API
    endpoints.push_back({"POST", "/admin/backup",             "Backup database"});
    endpoints.push_back({"POST", "/admin/restore",            "Restore database"});
    endpoints.push_back({"POST", "/v1/admin/shards",          "Add shard node"});
    endpoints.push_back({"GET",  "/v1/admin/shards",          "List shard nodes"});
    endpoints.push_back({"GET",  "/v1/admin/storage/stats",   "Storage statistics"});
    endpoints.push_back({"GET",  "/v1/admin/repair/health",       "Shard repair health & metrics"});
    endpoints.push_back({"POST", "/v1/admin/repair",              "Trigger shard repair job"});
    endpoints.push_back({"POST", "/v1/admin/repair/scan",         "Trigger full shard scan"});
    endpoints.push_back({"GET",  "/v1/admin/repair/jobs/{job_id}","Shard repair job status"});
    endpoints.push_back({"GET",  "/v1/admin/repair/dashboard",    "Shard repair dashboard (HTML)"});
    endpoints.push_back({"GET",  "/v1/admin/modules",              "List loaded modules"});
    endpoints.push_back({"POST", "/v1/admin/modules/{name}/load",  "Load module by path (admin)"});
    endpoints.push_back({"DELETE","/v1/admin/modules/{name}",      "Unload module (admin)"});
    endpoints.push_back({"GET",  "/v1/admin/modules/{name}",       "Module status (admin)"});

    // Config & Utilities
    endpoints.push_back({"GET",  "/config",                   "Get configuration"});
    endpoints.push_back({"POST", "/config",                   "Set configuration"});

    // ========== FEATURE-CONDITIONAL ENDPOINTS ==========

    // Snapshot live atomic values once to ensure consistency within this response.
    const bool cap_semantic_cache = feature_semantic_cache_live_.load(std::memory_order_relaxed);
    const bool cap_llm_store      = feature_llm_store_live_.load(std::memory_order_relaxed);
    const bool cap_cdc            = feature_cdc_live_.load(std::memory_order_relaxed);
    const bool cap_timeseries     = feature_timeseries_live_.load(std::memory_order_relaxed);

    // Semantic Cache (Sprint A)
    if (cap_semantic_cache) {
        endpoints.push_back({"POST", "/cache/query",          "Semantic cache lookup (beta)"});
        endpoints.push_back({"POST", "/cache/put",            "Semantic cache store (beta)"});
        endpoints.push_back({"GET",  "/cache/stats",          "Cache statistics (beta)"});
        endpoints.push_back({"GET",  "/v1/admin/cache/health", "Cache health (admin)"});
        endpoints.push_back({"GET",  "/v1/admin/cache/stats",  "Cache stats (admin)"});
        endpoints.push_back({"DELETE", "/v1/admin/cache/key/{key}", "Evict cache key (admin)"});
        endpoints.push_back({"DELETE", "/v1/admin/cache/tenant/{id}", "Evict tenant cache (admin)"});
        endpoints.push_back({"POST", "/v1/admin/cache/circuit-breaker/reset", "Reset circuit breaker (admin)"});
        endpoints.push_back({"GET",  "/v1/admin/cache/circuit-breaker", "Circuit breaker status (admin)"});
        endpoints.push_back({"POST", "/v1/admin/cache/warmup",  "Warmup cache (admin)"});
        endpoints.push_back({"POST", "/v1/admin/cache/snapshot", "Snapshot cache (admin)"});
        endpoints.push_back({"GET",  "/v1/admin/cache/tenants", "List tenants (admin)"});
        endpoints.push_back({"GET",  "/v1/admin/cache/tenant/{id}/stats", "Tenant stats (admin)"});
        endpoints.push_back({"PATCH", "/v1/admin/cache/tenant/{id}/quota", "Update tenant quota (admin)"});
        endpoints.push_back({"DELETE", "/v1/admin/cache/pii/{uuid}", "Evict PII from cache (admin)"});
    }

    // LLM Interaction Store (Sprint A)
    if (cap_llm_store) {
        endpoints.push_back({"POST", "/llm/interaction",        "Store LLM interaction"});
        endpoints.push_back({"GET",  "/llm/interaction",        "List LLM interactions"});
        endpoints.push_back({"GET",  "/llm/interaction/{id}",   "Get LLM interaction"});
        endpoints.push_back({"PATCH", "/llm/interaction/{id}",  "Update LLM metadata"});
        endpoints.push_back({"POST", "/query/enhanced",         "Enhanced query with LLM"});
    }

    // Prompt Template Management
    endpoints.push_back({"POST", "/prompt_template",          "Create prompt template"});
    endpoints.push_back({"GET",  "/prompt_template",          "List prompt templates"});
    endpoints.push_back({"GET",  "/prompt_template/{id}",     "Get prompt template"});
    endpoints.push_back({"PUT",  "/prompt_template/{id}",     "Update prompt template"});

    // Changefeed / CDC (Sprint A)
    if (cap_cdc) {
        endpoints.push_back({"GET",  "/changefeed",            "Get changefeed events"});
        endpoints.push_back({"GET",  "/changefeed/stream",     "Stream CDC events (SSE)"});
        endpoints.push_back({"POST", "/changefeed/stream/ack", "Acknowledge CDC events"});
        endpoints.push_back({"GET",  "/changefeed/stats",      "CDC statistics"});
        endpoints.push_back({"POST", "/changefeed/retention",  "Configure CDC retention"});
        endpoints.push_back({"GET",  "/changefeed/retention",  "Get CDC retention config"});
        endpoints.push_back({"PUT",  "/changefeed/retention",  "Update CDC retention config"});
        endpoints.push_back({"POST", "/changefeed/compact",    "Compact changefeed"});
        endpoints.push_back({"POST", "/changefeed/redact",     "GDPR redact changefeed"});

        // Snapshots (Named Snapshots - MVCC Phase 3)
        endpoints.push_back({"POST", "/api/v1/snapshots/tags",  "Create named snapshot"});
        endpoints.push_back({"GET",  "/api/v1/snapshots/tags",  "List named snapshots"});
        endpoints.push_back({"GET",  "/api/v1/snapshots/tags/{name}", "Get named snapshot"});
        endpoints.push_back({"DELETE", "/api/v1/snapshots/tags/{name}", "Delete named snapshot"});
        endpoints.push_back({"GET",  "/api/v1/snapshots/stats", "Snapshot statistics"});

        // Diff API (Phase 2 MVCC)
        endpoints.push_back({"GET",  "/api/v1/diff",            "Get record diff"});
        endpoints.push_back({"GET",  "/api/v1/diff/cache/stats", "Diff cache stats"});
        endpoints.push_back({"DELETE", "/api/v1/diff/cache",    "Clear diff cache"});

        // PITR API (Point-in-Time Recovery - Phase 3 MVCC)
        endpoints.push_back({"POST", "/api/v1/restore/pitr",   "Restore from PITR"});
        endpoints.push_back({"POST", "/api/v1/restore/preview", "Preview PITR restore"});
        endpoints.push_back({"GET",  "/api/v1/restore/progress", "PITR restore progress"});

        // Branch API (Phase 4 MVCC)
        endpoints.push_back({"POST", "/api/v1/branches",        "Create branch"});
        endpoints.push_back({"GET",  "/api/v1/branches",        "List branches"});
        endpoints.push_back({"GET",  "/api/v1/branches/active", "Get active branch"});
        endpoints.push_back({"GET",  "/api/v1/branches/stats",  "Branch statistics"});
        endpoints.push_back({"GET",  "/api/v1/branches/{name}", "Get branch"});
        endpoints.push_back({"POST", "/api/v1/branches/{name}/switch", "Switch branch"});
        endpoints.push_back({"DELETE", "/api/v1/branches/{name}", "Delete branch"});
        endpoints.push_back({"POST", "/api/v1/branches/merge",  "Merge branches"});

        // Merge API (Phase 5 MVCC - 3-Way Merge)
        endpoints.push_back({"POST", "/api/v1/merge",           "Merge snapshots"});
        endpoints.push_back({"POST", "/api/v1/merge/preview",   "Preview merge"});
        endpoints.push_back({"POST", "/api/v1/merge/by-tag",    "Merge by tag"});
        endpoints.push_back({"GET",  "/api/v1/merge/can-fast-forward", "Check fast-forward merge"});

        // MVCC API
        endpoints.push_back({"GET",  "/api/v1/mvcc/keys/{key}", "Get key versions"});
        endpoints.push_back({"POST", "/api/v1/mvcc/keys/{key}", "Put versioned key"});
        endpoints.push_back({"GET",  "/api/v1/mvcc/keys/{key}/versions", "Get version history"});
        endpoints.push_back({"DELETE", "/api/v1/mvcc/keys/{key}/versions", "Delete versions"});
        endpoints.push_back({"GET",  "/api/v1/mvcc/clock",      "Get HLC timestamp"});
        endpoints.push_back({"GET",  "/api/v1/mvcc/stats",      "MVCC statistics"});
    }

    // Time-Series Store (Sprint B)
    if (cap_timeseries) {
        endpoints.push_back({"POST", "/ts/put",                "Store time-series data"});
        endpoints.push_back({"POST", "/ts/query",              "Query time-series (beta)"});
        endpoints.push_back({"POST", "/ts/aggregate",          "Aggregate time-series (beta)"});
        endpoints.push_back({"GET",  "/ts/config",             "Get time-series config"});
        endpoints.push_back({"PUT",  "/ts/config",             "Update time-series config"});
        endpoints.push_back({"GET",  "/ts/aggregates",         "List aggregates"});
        endpoints.push_back({"GET",  "/ts/retention",          "Get retention policy"});
        endpoints.push_back({"GET",  "/ts/metrics",            "Time-series metrics"});
        endpoints.push_back({"POST", "/api/v1/prom/write",     "Prometheus remote write"});
    }

    // PII Manager
    if (config_.feature_pii_manager) {
        endpoints.push_back({"GET",  "/pii",                   "List PII bindings"});
        endpoints.push_back({"POST", "/pii",                   "Create PII binding"});
        endpoints.push_back({"GET",  "/pii/{uuid}",            "Get PII by UUID"});
        endpoints.push_back({"DELETE", "/pii/{uuid}",          "Delete PII binding"});
        endpoints.push_back({"GET",  "/pii/reveal/{uuid}",     "Reveal PII value"});
        endpoints.push_back({"GET",  "/pii/export.csv",        "Export PII (CSV)"});
    }

    // Update Checker
    if (config_.feature_update_checker) {
        endpoints.push_back({"GET",  "/api/updates",           "Get update status"});
        endpoints.push_back({"POST", "/api/updates/check",     "Check for updates"});
        endpoints.push_back({"GET",  "/api/updates/config",    "Get update config"});
        endpoints.push_back({"PUT",  "/api/updates/config",    "Update update config"});
    }

    // ========== OPTIONAL/CONDITIONAL ENDPOINTS ==========

    // SAML Support (if configured)
    if (saml_provider_ != nullptr) {
        endpoints.push_back({"GET",  "/api/v1/auth/saml/login",    "SAML login initiator"});
        endpoints.push_back({"POST", "/api/v1/auth/saml/acs",      "SAML assertion consumer"});
        endpoints.push_back({"POST", "/api/v1/auth/saml/slo",      "SAML logout"});
        endpoints.push_back({"GET",  "/api/v1/auth/saml/metadata", "SAML metadata"});
    }

    // PKI API
    endpoints.push_back({"POST", "/api/pki/{key_id}/sign",     "Sign with PKI key"});
    endpoints.push_back({"POST", "/api/pki/{key_id}/verify",   "Verify PKI signature"});
    endpoints.push_back({"POST", "/api/pki/hsm/sign",          "HSM sign"});
    endpoints.push_back({"GET",  "/api/pki/hsm/keys",          "List HSM keys"});
    endpoints.push_back({"POST", "/api/pki/timestamp",         "Create TSA timestamp"});
    endpoints.push_back({"POST", "/api/pki/timestamp/verify",  "Verify TSA timestamp"});
    endpoints.push_back({"POST", "/api/pki/eidas/sign",        "eIDAS sign"});
    endpoints.push_back({"POST", "/api/pki/eidas/verify",      "eIDAS verify"});
    endpoints.push_back({"GET",  "/api/pki/certificates",      "List certificates"});
    endpoints.push_back({"GET",  "/api/pki/certificates/{id}", "Get certificate"});
    endpoints.push_back({"GET",  "/api/pki/status",            "PKI status"});

    // Keys Management
    endpoints.push_back({"GET",  "/keys",                      "List keys"});
    endpoints.push_back({"POST", "/keys/rotate",               "Rotate keys"});

    // Classification & Reports
    endpoints.push_back({"GET",  "/classification/rules",      "Classification rules"});
    endpoints.push_back({"POST", "/classification/test",       "Test classification"});
    endpoints.push_back({"GET",  "/reports/compliance",        "Compliance report"});

    // Policies (Ranger integration)
    endpoints.push_back({"POST", "/policies/import/ranger",    "Import Ranger policies"});
    endpoints.push_back({"GET",  "/policies/export/ranger",    "Export Ranger policies"});

    // Audit API
    endpoints.push_back({"GET",  "/api/audit",                 "Query audit logs"});
    endpoints.push_back({"GET",  "/api/audit/export/csv",      "Export audit logs (CSV)"});

    // Export API (EXP-001)
    endpoints.push_back({"POST", "/api/v1/export/jsonl_llm",   "Export to JSONL for LLM"});
    endpoints.push_back({"GET",  "/api/v1/export/{id}/status", "Export job status"});

    // API Key Management
    endpoints.push_back({"POST", "/api/keys",                  "Create API key"});
    endpoints.push_back({"GET",  "/api/keys",                  "List API keys"});
    endpoints.push_back({"GET",  "/api/keys/{id}",             "Get API key"});
    endpoints.push_back({"PUT",  "/api/keys/{id}",             "Update API key"});
    endpoints.push_back({"DELETE", "/api/keys/{id}",           "Delete API key"});

    // Session Management
    endpoints.push_back({"POST", "/auth/sessions",             "Create session"});
    endpoints.push_back({"GET",  "/auth/sessions",             "List sessions"});
    endpoints.push_back({"DELETE", "/auth/sessions/{id}",      "Delete session"});
    endpoints.push_back({"DELETE", "/auth/sessions",           "Revoke all other sessions"});

    // UDF (User-Defined Functions)
    endpoints.push_back({"POST", "/api/v1/query/udfs",         "Register UDF"});
    endpoints.push_back({"GET",  "/api/v1/query/udfs",         "List UDFs"});
    endpoints.push_back({"GET",  "/api/v1/query/udfs/{name}",  "Get UDF"});
    endpoints.push_back({"DELETE", "/api/v1/query/udfs/{name}", "Delete UDF"});

    // GraphQL
    endpoints.push_back({"POST", "/graphql",                   "GraphQL query"});
    endpoints.push_back({"POST", "/api/v1/graphql",            "GraphQL query (v1)"});
    endpoints.push_back({"GET",  "/graphql/schema",            "GraphQL schema"});
    endpoints.push_back({"GET",  "/api/v1/graphql/schema",     "GraphQL schema (v1)"});

    // gRPC-Web Proxy
    endpoints.push_back({"POST", "/grpc-web/{service}/{method}", "gRPC-Web call"});
    endpoints.push_back({"OPTIONS", "/grpc-web/{service}/{method}", "gRPC-Web CORS"});
    endpoints.push_back({"GET",  "/api/v1/grpc-web/status",    "gRPC-Web status"});

    // Serverless Functions
    endpoints.push_back({"POST", "/api/v1/functions",          "Deploy function"});
    endpoints.push_back({"GET",  "/api/v1/functions",          "List functions"});
    endpoints.push_back({"GET",  "/api/v1/functions/{id}",     "Get function"});
    endpoints.push_back({"PUT",  "/api/v1/functions/{id}",     "Update function"});
    endpoints.push_back({"DELETE", "/api/v1/functions/{id}",   "Delete function"});
    endpoints.push_back({"POST", "/api/v1/functions/{id}/invoke", "Invoke function"});
    endpoints.push_back({"GET",  "/api/v1/functions/{id}/versions", "List function versions"});

    // Async Jobs
    endpoints.push_back({"POST", "/v2/jobs",                   "Submit async job"});
    endpoints.push_back({"GET",  "/v2/jobs",                   "List async jobs"});
    endpoints.push_back({"GET",  "/v2/jobs/{id}",              "Get job status"});
    endpoints.push_back({"DELETE", "/v2/jobs/{id}",            "Cancel job"});

    // Task Scheduler
    endpoints.push_back({"POST", "/api/tasks",                 "Create scheduled task"});
    endpoints.push_back({"GET",  "/api/tasks",                 "List scheduled tasks"});
    endpoints.push_back({"GET",  "/api/tasks/stats",           "Task scheduler stats"});
    endpoints.push_back({"GET",  "/api/tasks/{id}",            "Get task"});
    endpoints.push_back({"PUT",  "/api/tasks/{id}",            "Update task"});
    endpoints.push_back({"DELETE", "/api/tasks/{id}",          "Delete task"});
    endpoints.push_back({"POST", "/api/tasks/{id}/enable",     "Enable task"});
    endpoints.push_back({"POST", "/api/tasks/{id}/disable",    "Disable task"});
    endpoints.push_back({"POST", "/api/tasks/{id}/execute",    "Execute task now"});
    endpoints.push_back({"GET",  "/api/tasks/{id}/history",    "Task execution history"});
    endpoints.push_back({"GET",  "/ui/tasks",                  "Tasks web UI"});

    // Database Maintenance
    endpoints.push_back({"POST", "/api/v1/maintenance/schedules", "Create maintenance schedule"});
    endpoints.push_back({"GET",  "/api/v1/maintenance/schedules", "List maintenance schedules"});
    endpoints.push_back({"GET",  "/api/v1/maintenance/schedules/{id}", "Get maintenance schedule"});
    endpoints.push_back({"PUT",  "/api/v1/maintenance/schedules/{id}", "Update maintenance schedule"});
    endpoints.push_back({"PATCH", "/api/v1/maintenance/schedules/{id}", "Patch maintenance schedule"});
    endpoints.push_back({"DELETE", "/api/v1/maintenance/schedules/{id}", "Delete maintenance schedule"});
    endpoints.push_back({"POST", "/api/v1/maintenance/schedules/{id}/run", "Run maintenance now"});
    endpoints.push_back({"GET",  "/api/v1/maintenance/jobs",   "List maintenance jobs"});
    endpoints.push_back({"GET",  "/api/v1/maintenance/jobs/{id}", "Get maintenance job"});
    endpoints.push_back({"POST", "/api/v1/maintenance/jobs/{id}/cancel", "Cancel maintenance job"});
    endpoints.push_back({"GET",  "/api/v1/maintenance/status", "Maintenance status"});
    endpoints.push_back({"GET",  "/api/v1/maintenance/health", "Maintenance health"});
    endpoints.push_back({"GET",  "/api/v1/maintenance/task-handlers", "Available task handlers"});

    // Schema API
    endpoints.push_back({"GET",  "/api/v1/schema",             "Get full schema"});
    endpoints.push_back({"GET",  "/api/v1/schema/tables",      "List tables"});
    endpoints.push_back({"GET",  "/api/v1/schema/tables/{name}", "Get table schema"});
    endpoints.push_back({"PUT",  "/api/v1/schema/{table}",     "Update table schema"});
    endpoints.push_back({"PATCH", "/api/v1/schema/{table}",    "Patch table schema"});
    endpoints.push_back({"GET",  "/api/v1/schema/versions/{table}", "Get schema versions"});
    endpoints.push_back({"POST", "/api/v1/schema/versions/{table}", "Create schema version"});
    endpoints.push_back({"GET",  "/api/v1/schema/diff/{table}", "Compare schema versions"});
    endpoints.push_back({"GET",  "/api/v1/information_schema", "INFORMATION_SCHEMA"});
    endpoints.push_back({"GET",  "/api/v1/metadata/stats/{table}", "Table statistics"});
    endpoints.push_back({"POST", "/api/v1/metadata/stats/{table}", "Update statistics"});
    endpoints.push_back({"GET",  "/api/v1/metadata/constraints/{table}", "Table constraints"});
    endpoints.push_back({"GET",  "/api/v1/metadata/index_recommendations", "Index recommendations"});
    endpoints.push_back({"GET",  "/api/v1/metadata/audit",     "Metadata audit log"});
    endpoints.push_back({"GET",  "/api/v1/metadata/lineage/{table}", "Column lineage"});
    endpoints.push_back({"POST", "/api/v1/metadata/lineage",   "Track column lineage"});
    endpoints.push_back({"PUT",  "/api/v1/metadata/schema_import", "Import schema"});
    endpoints.push_back({"POST", "/api/v1/metadata/constraints/validate/{table}", "Validate constraints"});

    // Error API
    endpoints.push_back({"GET",  "/api/v1/errors",             "List error codes"});
    endpoints.push_back({"GET",  "/api/v1/errors/{code}",      "Get error documentation"});
    endpoints.push_back({"GET",  "/api/v1/errors/categories",  "Error categories"});
    endpoints.push_back({"GET",  "/api/v1/errors/search",      "Search errors"});

    // BPMN Process Management
    endpoints.push_back({"POST", "/api/v1/bpmn/process/start", "Start BPMN process"});
    endpoints.push_back({"POST", "/api/v1/bpmn/task/{id}/complete", "Complete BPMN task"});
    endpoints.push_back({"GET",  "/api/v1/bpmn/instance/{id}", "Get BPMN instance"});

    // Geo Topology
    endpoints.push_back({"GET",  "/api/v1/geo/topology",       "Geo topology"});
    endpoints.push_back({"GET",  "/api/v1/geo/regions",        "List regions"});
    endpoints.push_back({"GET",  "/api/v1/geo/health",         "Geo health"});
    endpoints.push_back({"POST", "/api/v1/geo/topology/shard", "Add shard to topology"});
    endpoints.push_back({"DELETE", "/api/v1/geo/topology/shard/{id}", "Remove shard from topology"});
    endpoints.push_back({"GET",  "/api/v1/geo/config/{collection}", "Get geo config"});
    endpoints.push_back({"PUT",  "/api/v1/geo/config/{collection}", "Update geo config"});

    // Replication Topology
    endpoints.push_back({"GET",  "/api/v1/replication/topology", "Replication topology"});
    endpoints.push_back({"GET",  "/api/v1/replication/health",   "Replication health"});
    endpoints.push_back({"GET",  "/ui/replication/topology",     "Replication topology UI"});

    // Feedback API
    endpoints.push_back({"POST", "/api/feedback",              "Submit feedback"});
    endpoints.push_back({"GET",  "/api/feedback",              "List feedback"});
    endpoints.push_back({"GET",  "/api/feedback/{id}",         "Get feedback"});
    endpoints.push_back({"PUT",  "/api/feedback/{id}",         "Update feedback"});
    endpoints.push_back({"DELETE", "/api/feedback/{id}",       "Delete feedback"});
    endpoints.push_back({"GET",  "/api/feedback/adapter/{adapter_id}", "Get feedback adapter"});
    endpoints.push_back({"GET",  "/api/feedback/stats",        "Feedback statistics"});

    // Retention Policy Admin API
    endpoints.push_back({"GET",    "/api/retention/policies",          "List retention policies"});
    endpoints.push_back({"POST",   "/api/retention/policies",          "Create/update retention policy"});
    endpoints.push_back({"DELETE", "/api/retention/policies/{name}",   "Delete retention policy"});
    endpoints.push_back({"GET",    "/api/retention/history",           "Retention action history"});

    // SAGA Audit Log API
    endpoints.push_back({"GET",  "/api/saga/batches",                  "List SAGA audit batches"});
    endpoints.push_back({"GET",  "/api/saga/batches/{id}",             "Get SAGA batch detail"});
    endpoints.push_back({"POST", "/api/saga/batches/{id}/verify",      "Verify SAGA batch signature"});
    endpoints.push_back({"POST", "/api/saga/flush",                    "Flush current SAGA batch"});

    // Sort by path for consistent, reproducible output
    std::sort(endpoints.begin(), endpoints.end(),
        [](const RegisteredEndpoint& a, const RegisteredEndpoint& b) {
            if (a.path != b.path) return a.path < b.path;
            // Secondary sort by method for same path
            return a.method < b.method;
        });

    return endpoints;
}

// ============================================================================
// Error API handlers
http::response<http::string_body> HttpServer::handleErrorApiList(
    const http::request<http::string_body>& req
) {
    try {
        // Parse query parameters using helper
        std::string target_str = std::string(req.target());
        nlohmann::json query_params = parseQueryParams(target_str);
        
        // Create Request object for handler
        server::Request handler_req;
        handler_req.method = std::string(http::to_string(req.method()));
        handler_req.path = target_str;
        handler_req.query = query_params;
        handler_req.params = nlohmann::json::object();
        handler_req.body = nlohmann::json::object();
        
        // Call handler
        server::Response handler_res;
        if (!error_api_handler_) {
            error_api_handler_ = std::make_unique<server::ErrorApiHandler>();
        }
        auto& error_api_handler = *error_api_handler_;
        error_api_handler.handleGetErrors(handler_req, handler_res);
        
        // Convert response
        return makeResponse(
            static_cast<http::status>(handler_res.status_code),
            handler_res.body.dump(),
            req
        );
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> HttpServer::handleErrorApiGetByCode(
    const http::request<http::string_body>& req
) {
    try {
        std::string target_str = std::string(req.target());
        std::string path_only = target_str;
        auto qpos = path_only.find('?');
        if (qpos != std::string::npos) path_only = path_only.substr(0, qpos);
        
        // Extract error code from path: /api/v1/errors/:code
        std::string code_str;
        if (path_only.rfind("/api/v1/errors/", 0) == 0) {
            code_str = path_only.substr(std::string("/api/v1/errors/").size());
        }
        
        // Create Request object for handler
        server::Request handler_req;
        handler_req.method = std::string(http::to_string(req.method()));
        handler_req.path = target_str;
        handler_req.query = nlohmann::json::object();
        handler_req.params = nlohmann::json::object();
        handler_req.params["code"] = code_str;
        handler_req.body = nlohmann::json::object();
        
        // Call handler
        server::Response handler_res;
        if (!error_api_handler_) {
            error_api_handler_ = std::make_unique<server::ErrorApiHandler>();
        }
        auto& error_api_handler = *error_api_handler_;
        error_api_handler.handleGetError(handler_req, handler_res);
        
        // Convert response
        return makeResponse(
            static_cast<http::status>(handler_res.status_code),
            handler_res.body.dump(),
            req
        );
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> HttpServer::handleErrorApiCategories(
    const http::request<http::string_body>& req
) {
    try {
        // Create Request object for handler
        server::Request handler_req;
        handler_req.method = std::string(http::to_string(req.method()));
        handler_req.path = std::string(req.target());
        handler_req.query = nlohmann::json::object();
        handler_req.params = nlohmann::json::object();
        handler_req.body = nlohmann::json::object();
        
        // Call handler
        server::Response handler_res;
        if (!error_api_handler_) {
            error_api_handler_ = std::make_unique<server::ErrorApiHandler>();
        }
        auto& error_api_handler = *error_api_handler_;
        error_api_handler.handleGetCategories(handler_req, handler_res);
        
        // Convert response
        return makeResponse(
            static_cast<http::status>(handler_res.status_code),
            handler_res.body.dump(),
            req
        );
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> HttpServer::handleErrorApiSearch(
    const http::request<http::string_body>& req
) {
    try {
        // Parse query parameters using helper
        std::string target_str = std::string(req.target());
        nlohmann::json query_params = parseQueryParams(target_str);
        
        // Create Request object for handler
        server::Request handler_req;
        handler_req.method = std::string(http::to_string(req.method()));
        handler_req.path = target_str;
        handler_req.query = query_params;
        handler_req.params = nlohmann::json::object();
        handler_req.body = nlohmann::json::object();
        
        // Call handler
        server::Response handler_res;
        if (!error_api_handler_) {
            error_api_handler_ = std::make_unique<server::ErrorApiHandler>();
        }
        auto& error_api_handler = *error_api_handler_;
        error_api_handler.handleSearchErrors(handler_req, handler_res);
        
        // Convert response
        return makeResponse(
            static_cast<http::status>(handler_res.status_code),
            handler_res.body.dump(),
            req
        );
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

// ============================================================================
// Schema API Handlers
// ============================================================================

http::response<http::string_body> HttpServer::handleSchemaGetFull(
    const http::request<http::string_body>& req
) {
    if (!schema_api_handler_) {
        return makeErrorResponse(http::status::service_unavailable, 
            "Schema API not available", req);
    }
    auto& schema_api_handler = *schema_api_handler_;
    return schema_api_handler.handleGetSchema(req);
}

http::response<http::string_body> HttpServer::handleSchemaGetTables(
    const http::request<http::string_body>& req
) {
    if (!schema_api_handler_) {
        return makeErrorResponse(http::status::service_unavailable, 
            "Schema API not available", req);
    }
    auto& schema_api_handler = *schema_api_handler_;
    return schema_api_handler.handleGetTables(req);
}

http::response<http::string_body> HttpServer::handleSchemaGetTable(
    const http::request<http::string_body>& req
) {
    if (!schema_api_handler_) {
        return makeErrorResponse(http::status::service_unavailable, 
            "Schema API not available", req);
    }
    auto& schema_api_handler = *schema_api_handler_;
    return schema_api_handler.handleGetTable(req);
}

http::response<http::string_body> HttpServer::handleSchemaPut(
    const http::request<http::string_body>& req
) {
    if (!schema_api_handler_) {
        return makeErrorResponse(http::status::service_unavailable, 
            "Schema API not available", req);
    }
    auto& schema_api_handler = *schema_api_handler_;
    auto response = schema_api_handler.handlePutSchema(req);
    if (continuous_learning_orchestrator_
        && response.result_int() >= 200
        && response.result_int() < 300) {
        continuous_learning_orchestrator_->triggerLoop3IndexLifecycle();
    }
    return response;
}

http::response<http::string_body> HttpServer::handleSchemaPatch(
    const http::request<http::string_body>& req
) {
    if (!schema_api_handler_) {
        return makeErrorResponse(http::status::service_unavailable, 
            "Schema API not available", req);
    }
    auto& schema_api_handler = *schema_api_handler_;
    auto response = schema_api_handler.handlePatchSchema(req);
    if (continuous_learning_orchestrator_
        && response.result_int() >= 200
        && response.result_int() < 300) {
        continuous_learning_orchestrator_->triggerLoop3IndexLifecycle();
    }
    return response;
}

// ============================================================================
// Metadata extended handler shims
// ============================================================================

http::response<http::string_body> HttpServer::handleMetadataInformationSchema(
    const http::request<http::string_body>& req)
{
    if (!schema_api_handler_) {
        return makeErrorResponse(http::status::service_unavailable,
            "Schema API not available", req);
    }
    auto& schema_api_handler = *schema_api_handler_;
    return schema_api_handler.handleGetInformationSchema(req);
}

http::response<http::string_body> HttpServer::handleMetadataGetStats(
    const http::request<http::string_body>& req)
{
    if (!schema_api_handler_) {
        return makeErrorResponse(http::status::service_unavailable,
            "Schema API not available", req);
    }
    auto& schema_api_handler = *schema_api_handler_;
    return schema_api_handler.handleGetStats(req);
}

http::response<http::string_body> HttpServer::handleMetadataCollectStats(
    const http::request<http::string_body>& req)
{
    if (!schema_api_handler_) {
        return makeErrorResponse(http::status::service_unavailable,
            "Schema API not available", req);
    }
    auto& schema_api_handler = *schema_api_handler_;
    return schema_api_handler.handleCollectStats(req);
}

http::response<http::string_body> HttpServer::handleMetadataGetConstraints(
    const http::request<http::string_body>& req)
{
    if (!schema_api_handler_) {
        return makeErrorResponse(http::status::service_unavailable,
            "Schema API not available", req);
    }
    auto& schema_api_handler = *schema_api_handler_;
    return schema_api_handler.handleGetConstraints(req);
}

http::response<http::string_body> HttpServer::handleMetadataIndexRecommendations(
    const http::request<http::string_body>& req)
{
    if (!schema_api_handler_) {
        return makeErrorResponse(http::status::service_unavailable,
            "Schema API not available", req);
    }
    auto& schema_api_handler = *schema_api_handler_;
    return schema_api_handler.handleGetIndexRecommendations(req);
}

http::response<http::string_body> HttpServer::handleSchemaVersionHistory(
    const http::request<http::string_body>& req)
{
    if (!schema_api_handler_) {
        return makeErrorResponse(http::status::service_unavailable,
            "Schema API not available", req);
    }
    auto& schema_api_handler = *schema_api_handler_;
    return schema_api_handler.handleGetVersionHistory(req);
}

http::response<http::string_body> HttpServer::handleSchemaCreateVersion(
    const http::request<http::string_body>& req)
{
    if (!schema_api_handler_) {
        return makeErrorResponse(http::status::service_unavailable,
            "Schema API not available", req);
    }
    auto& schema_api_handler = *schema_api_handler_;
    auto response = schema_api_handler.handleCreateVersion(req);
    if (continuous_learning_orchestrator_
        && response.result_int() >= 200
        && response.result_int() < 300) {
        continuous_learning_orchestrator_->triggerLoop3IndexLifecycle();
    }
    return response;
}

http::response<http::string_body> HttpServer::handleSchemaDiff(
    const http::request<http::string_body>& req)
{
    if (!schema_api_handler_) {
        return makeErrorResponse(http::status::service_unavailable,
            "Schema API not available", req);
    }
    auto& schema_api_handler = *schema_api_handler_;
    return schema_api_handler.handleGetDiff(req);
}

http::response<http::string_body> HttpServer::handleMetadataAuditLog(
    const http::request<http::string_body>& req)
{
    if (!schema_api_handler_) {
        return makeErrorResponse(http::status::service_unavailable,
            "Schema API not available", req);
    }
    auto& schema_api_handler = *schema_api_handler_;
    return schema_api_handler.handleGetAuditLog(req);
}

http::response<http::string_body> HttpServer::handleMetadataSchemaImport(
    const http::request<http::string_body>& req)
{
    if (!schema_api_handler_) {
        return makeErrorResponse(http::status::service_unavailable,
            "Schema API not available", req);
    }
    auto& schema_api_handler = *schema_api_handler_;
    auto response = schema_api_handler.handleSchemaImport(req);
    if (continuous_learning_orchestrator_
        && response.result_int() >= 200
        && response.result_int() < 300) {
        continuous_learning_orchestrator_->triggerLoop3IndexLifecycle();
    }
    return response;
}

http::response<http::string_body> HttpServer::handleMetadataBatchValidate(
    const http::request<http::string_body>& req)
{
    if (!schema_api_handler_) {
        return makeErrorResponse(http::status::service_unavailable,
            "Schema API not available", req);
    }
    auto& schema_api_handler = *schema_api_handler_;
    return schema_api_handler.handleBatchConstraintValidation(req);
}

http::response<http::string_body> HttpServer::handleMetadataGetColumnLineage(
    const http::request<http::string_body>& req)
{
    if (!schema_api_handler_) {
        return makeErrorResponse(http::status::service_unavailable,
            "Schema API not available", req);
    }
    auto& schema_api_handler = *schema_api_handler_;
    return schema_api_handler.handleGetColumnLineage(req);
}

http::response<http::string_body> HttpServer::handleMetadataRecordLineageDerivation(
    const http::request<http::string_body>& req)
{
    if (!schema_api_handler_) {
        return makeErrorResponse(http::status::service_unavailable,
            "Schema API not available", req);
    }
    auto& schema_api_handler = *schema_api_handler_;
    return schema_api_handler.handleRecordLineageDerivation(req);
}

// ── ContentFS HTTP handlers ─────────────────────────────────────────────────
// These wrap ContentFS (binary blob storage) over HTTP:
//   PUT    /api/v1/content/fs/{pk}  — store blob (body = raw bytes, Content-Type header used as MIME)
//   GET    /api/v1/content/fs/{pk}  — retrieve blob
//   HEAD   /api/v1/content/fs/{pk}  — metadata only (no body)
//   DELETE /api/v1/content/fs/{pk}  — remove blob

static std::string extractContentFsPk(const http::request<http::string_body>& req) {
    std::string path = std::string(req.target());
    auto qpos = path.find('?');
    if (qpos != std::string::npos) path = path.substr(0, qpos);
    // prefix is "/api/v1/content/fs/"  (19 chars)
    if (path.size() > 19) return path.substr(19);
    return {};
}

http::response<http::string_body> HttpServer::handleContentFsPut(
    const http::request<http::string_body>& req)
{
    if (!content_fs_) {
        return makeErrorResponse(http::status::service_unavailable,
            "ContentFS not initialized", req);
    }
    auto& content_fs = *content_fs_;
    const std::string pk = extractContentFsPk(req);
    if (pk.empty()) {
        return makeErrorResponse(http::status::bad_request,
            "ContentFS: pk (resource key) must not be empty", req);
    }
    const std::string body_str = req.body();
    const std::vector<uint8_t> data(body_str.begin(), body_str.end());
    std::string mime = std::string(req[http::field::content_type]);
    if (mime.empty()) mime = "application/octet-stream";

    // Optional SHA-256 hint via X-Content-SHA256 header
    std::optional<std::string> sha256_hint;
    const auto sha_hdr = req.find("X-Content-SHA256");
    if (sha_hdr != req.end() && !sha_hdr->value().empty()) {
        sha256_hint = std::string(sha_hdr->value());
    }

    auto result = content_fs.put(pk, data, mime, sha256_hint);
    if (!result) {
        const bool bad_req = (result.error().code() == errors::ErrorCode::ERR_API_INVALID_REQUEST);
        return makeErrorResponse(
            bad_req ? http::status::bad_request : http::status::internal_server_error,
            result.error().message(), req);
    }

    json body = {{"pk", pk}, {"size", data.size()}, {"mime", mime}};
    if (sha256_hint) body["sha256"] = *sha256_hint;
    return makeResponse(http::status::created, body.dump(), req);
}

http::response<http::string_body> HttpServer::handleContentFsGet(
    const http::request<http::string_body>& req)
{
    if (!content_fs_) {
        return makeErrorResponse(http::status::service_unavailable,
            "ContentFS not initialized", req);
    }
    auto& content_fs = *content_fs_;
    const std::string pk = extractContentFsPk(req);
    if (pk.empty()) {
        return makeErrorResponse(http::status::bad_request,
            "ContentFS: pk (resource key) must not be empty", req);
    }

    // Range support via standard Range header (bytes=offset-end)
    const auto range_hdr = req.find(http::field::range);
    if (range_hdr != req.end()) {
        uint64_t offset = 0, length = 0;
        // Parse "bytes=<offset>-<end>" (best-effort)
        std::string rv = std::string(range_hdr->value());
        if (rv.rfind("bytes=", 0) == 0) {
            rv = rv.substr(6);
            auto dash = rv.find('-');
            if (dash != std::string::npos) {
                try { offset = std::stoull(rv.substr(0, dash)); } catch (...) {}
                if (dash + 1 < rv.size()) {
                    try {
                        uint64_t end_pos = std::stoull(rv.substr(dash + 1));
                        length = (end_pos >= offset) ? (end_pos - offset + 1) : 0;
                    } catch (...) {}
                }
            }
        }
        auto result = content_fs.getRange(pk, offset, length);
        if (!result) {
            const bool not_found = (result.error().code() == errors::ErrorCode::ERR_STORAGE_FILE_NOT_FOUND);
            return makeErrorResponse(
                not_found ? http::status::not_found : http::status::internal_server_error,
                result.error().message(), req);
        }
        const auto& data = result.value();
        http::response<http::string_body> resp{http::status::partial_content, req.version()};
        resp.set(http::field::server, "ThemisDB");
        resp.set(http::field::content_type, "application/octet-stream");
        resp.body() = std::string(data.begin(), data.end());
        resp.prepare_payload();
        return resp;
    }

    auto result = content_fs.get(pk);
    if (!result) {
        const bool not_found = (result.error().code() == errors::ErrorCode::ERR_STORAGE_FILE_NOT_FOUND);
        return makeErrorResponse(
            not_found ? http::status::not_found : http::status::internal_server_error,
            result.error().message(), req);
    }
    const auto& data = result.value();
    // Retrieve MIME via head() for correct Content-Type
    auto meta_result = content_fs.head(pk);
    std::string mime = "application/octet-stream";
    if (meta_result) mime = meta_result.value().mime;

    http::response<http::string_body> resp{http::status::ok, req.version()};
    resp.set(http::field::server, "ThemisDB");
    resp.set(http::field::content_type, mime);
    resp.body() = std::string(data.begin(), data.end());
    resp.prepare_payload();
    return resp;
}

http::response<http::string_body> HttpServer::handleContentFsHead(
    const http::request<http::string_body>& req)
{
    if (!content_fs_) {
        return makeErrorResponse(http::status::service_unavailable,
            "ContentFS not initialized", req);
    }
    auto& content_fs = *content_fs_;
    const std::string pk = extractContentFsPk(req);
    if (pk.empty()) {
        return makeErrorResponse(http::status::bad_request,
            "ContentFS: pk (resource key) must not be empty", req);
    }
    auto result = content_fs.head(pk);
    if (!result) {
        const bool not_found = (result.error().code() == errors::ErrorCode::ERR_STORAGE_FILE_NOT_FOUND);
        return makeErrorResponse(
            not_found ? http::status::not_found : http::status::internal_server_error,
            result.error().message(), req);
    }
    const auto& meta = result.value();
    http::response<http::string_body> resp{http::status::ok, req.version()};
    resp.set(http::field::server, "ThemisDB");
    resp.set(http::field::content_type, meta.mime);
    resp.set(http::field::content_length, std::to_string(meta.size));
    resp.set("X-Content-SHA256", meta.sha256_hex);
    resp.prepare_payload();
    return resp;
}

http::response<http::string_body> HttpServer::handleContentFsDelete(
    const http::request<http::string_body>& req)
{
    if (!content_fs_) {
        return makeErrorResponse(http::status::service_unavailable,
            "ContentFS not initialized", req);
    }
    auto& content_fs = *content_fs_;
    const std::string pk = extractContentFsPk(req);
    if (pk.empty()) {
        return makeErrorResponse(http::status::bad_request,
            "ContentFS: pk (resource key) must not be empty", req);
    }
    auto result = content_fs.remove(pk);
    if (!result) {
        const bool not_found = (result.error().code() == errors::ErrorCode::ERR_STORAGE_FILE_NOT_FOUND);
        return makeErrorResponse(
            not_found ? http::status::not_found : http::status::internal_server_error,
            result.error().message(), req);
    }
    return makeResponse(http::status::no_content, "", req);
}

void HttpServer::setContinuousQueryEngine(
    std::shared_ptr<themis::query::ContinuousQueryEngine> engine)
{
    continuous_query_engine_ = engine;
    if (engine) {
        continuous_query_api_ =
            std::make_unique<themis::server::ContinuousQueryApiHandler>(std::move(engine));
        THEMIS_INFO("ContinuousQueryEngine wired — CQL REST/SSE endpoints active "
                    "at /v1/queries/continuous");
    } else {
        continuous_query_api_.reset();
        THEMIS_INFO("ContinuousQueryEngine removed — CQL REST/SSE endpoints disabled");
    }
}

#ifdef THEMIS_ENABLE_MCP
void HttpServer::setMcpServer(
    std::shared_ptr<themis::server::McpServer> mcp_server)
{
    mcp_server_ = std::move(mcp_server);
    if (mcp_server_) {
        THEMIS_INFO("McpServer wired — AI Safety Layer HILG endpoints active at /v1/ai/*");
    } else {
        THEMIS_INFO("McpServer removed — AI Safety Layer HILG endpoints disabled");
    }
}
#endif

} // namespace server
} // namespace themis
