/**
 * @file tenant_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <string_view>
#include <optional>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <memory>
#include <chrono>
#include <atomic>
#include <cstdint>

namespace themis {

struct TenantConfig {
    std::string tenant_id;                  // Unique tenant identifier
    std::string display_name;               // Human-readable tenant name
    bool enabled = true;                    // Tenant enabled/disabled
    
    // Custom domain routing: domains whose Host header maps to this tenant.
    // Examples: {"acme.example.com", "www.acme.com"}
    // Ports are stripped before matching (e.g., "acme.example.com:8443" -> "acme.example.com").
    std::vector<std::string> custom_domains;
    
    // Resource quotas
    uint64_t max_storage_bytes = 0;         // 0 = unlimited
    uint64_t max_documents = 0;             // 0 = unlimited
    uint64_t max_collections = 0;           // 0 = unlimited
    uint32_t max_concurrent_queries = 100;  // Max concurrent queries
    uint32_t max_connections = 50;          // Max simultaneous connections
    
    // Rate limiting
    uint32_t requests_per_second = 1000;    // Rate limit per second
    uint32_t burst_size = 100;              // Token bucket burst size
    
    // Feature flags
    bool allow_gpu_acceleration = true;
    bool allow_vector_search = true;
    bool allow_graph_queries = true;
    bool allow_timeseries = true;
    bool allow_geo_queries = true;
    bool allow_full_text_search = true;
    
    // Encryption settings
    std::string encryption_key_id;          // Tenant-specific encryption key
    bool require_encryption = false;        // Force encryption for all data
    
    // Custom domain routing
    // When set, requests whose HTTP Host header matches this value are
    // automatically routed to this tenant without requiring X-Tenant-ID
    // or a /tenants/{id}/ path prefix.
    // Example: "acme.example.com"
    std::string custom_domain;

    // Metadata
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point updated_at;
    std::unordered_map<std::string, std::string> metadata;
};

struct TenantContext {
    std::string tenant_id;
    std::string user_id;                    // User within tenant
    std::vector<std::string> roles;         // User roles within tenant
    bool is_admin = false;                  // Tenant admin flag
    
    // Resource tracking for this request
    std::chrono::steady_clock::time_point request_start;
    
    // Derived from TenantConfig
    bool gpu_allowed = true;
    bool vector_search_allowed = true;
    std::string encryption_key_id;
    
    static TenantContext fromConfig(const TenantConfig& config, 
                                    std::string_view user_id,
                                    const std::vector<std::string>& roles = {}) {
        TenantContext ctx;
        ctx.tenant_id = config.tenant_id;
        ctx.user_id = std::string(user_id);
        ctx.roles = roles;
        ctx.is_admin = std::find(roles.begin(), roles.end(), "admin") != roles.end();
        ctx.gpu_allowed = config.allow_gpu_acceleration;
        ctx.vector_search_allowed = config.allow_vector_search;
        ctx.encryption_key_id = config.encryption_key_id;
        ctx.request_start = std::chrono::steady_clock::now();
        return ctx;
    }
};

struct TenantUsage {
    std::string tenant_id;
    
    // Current usage
    std::atomic<uint64_t> storage_bytes_used{0};
    std::atomic<uint64_t> document_count{0};
    std::atomic<uint64_t> collection_count{0};
    std::atomic<uint32_t> active_connections{0};
    std::atomic<uint32_t> active_queries{0};
    
    // Cumulative metrics
    std::atomic<uint64_t> total_requests{0};
    std::atomic<uint64_t> total_queries{0};
    std::atomic<uint64_t> total_bytes_read{0};
    std::atomic<uint64_t> total_bytes_written{0};
    std::atomic<uint64_t> rate_limited_requests{0};
    
    // Last activity
    std::atomic<int64_t> last_activity_epoch{0};  // Unix timestamp
};

class TenantManager {
public:
    /**
     * @brief Instance.
     * @return Return value.
     */
    static TenantManager& instance();
    
    // Configuration
    struct Config {
        // Tenant identification method
        std::string tenant_header = "X-Tenant-ID";     // Header for tenant ID
        std::string tenant_path_prefix = "/tenants/";  // Path prefix for tenant routing

        // Custom domain routing: header whose value is matched against
        // per-tenant custom_domains.  Set to "Host" for standard HTTP/1.1
        // routing; use ":authority" for raw HTTP/2 pseudo-headers if the
        // session layer normalises them before passing headers here.
        std::string custom_domain_host_header = "Host";

        // Default tenant for single-tenant deployments
        std::string default_tenant_id = "default";
        bool allow_default_tenant = true;              // Allow requests without tenant ID
        
        // Resource limits
        uint64_t global_max_tenants = 1000;
        bool enforce_quotas = true;
    };
    
    /**
     * @brief Configure.
     * @param[in] config Input parameter.
     */
    void configure(const Config& config);

    const Config& getConfig() const { return config_; }
    
    // Tenant lifecycle
    enum class CreateResult {
        Success,
        AlreadyExists,
        InvalidConfig,
        QuotaExceeded,
        InternalError
    };
    
    /**
     * @brief Create Tenant.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    CreateResult createTenant(const TenantConfig& config);

    /**
     * @brief Update Tenant.
     * @param[in] config Input parameter.
     * @return True when the operation succeeds.
     */
    bool updateTenant(const TenantConfig& config);

    /**
     * @brief Delete Tenant.
     * @param[in] tenant_id Identifier of the tenant.
     * @return True when the operation succeeds.
     */
    bool deleteTenant(std::string_view tenant_id);

    /**
     * @brief Set Tenant Enabled.
     * @param[in] tenant_id Identifier of the tenant.
     * @param[in] enabled Input parameter.
     * @return True when the operation succeeds.
     */
    bool setTenantEnabled(std::string_view tenant_id, bool enabled);
    
    // Tenant lookup
    /**
     * @brief Get Tenant.
     * @param[in] tenant_id Identifier of the tenant.
     * @return Return value.
     */
    std::optional<TenantConfig> getTenant(std::string_view tenant_id) const;

    /**
     * @brief List Tenants.
     * @return Return value.
     */
    std::vector<TenantConfig> listTenants() const;

    /**
     * @brief Tenant Exists.
     * @param[in] tenant_id Identifier of the tenant.
     * @return True when the operation succeeds.
     */
    bool tenantExists(std::string_view tenant_id) const;

    /**
     * @brief Get Tenant Count.
     * @return Return value.
     */
    size_t getTenantCount() const;
    
    // Request context resolution
    std::optional<TenantContext> resolveContext(
        const std::unordered_map<std::string, std::string>& headers,
        std::string_view path,
        std::string_view user_id = "",
        const std::vector<std::string>& roles = {}
    ) const;
    
    // Extract tenant ID from request
    std::optional<std::string> extractTenantId(
        const std::unordered_map<std::string, std::string>& headers,
        std::string_view path
    ) const;

    /**
     * @brief Register Custom Domain.
     * @param[in] tenant_id Identifier of the tenant.
     * @param[in] domain Input parameter.
     * @return True when the operation succeeds.
     */
    bool registerCustomDomain(std::string_view tenant_id, std::string_view domain);

    /**
     * @brief Unregister Custom Domain.
     * @param[in] domain Input parameter.
     * @return True when the operation succeeds.
     */
    bool unregisterCustomDomain(std::string_view domain);

    /**
     * @brief Lookup Tenant By Domain.
     * @param[in] host Input parameter.
     * @return Return value.
     */
    std::optional<std::string> lookupTenantByDomain(std::string_view host) const;

    /**
     * @brief Strip Tenant Path.
     * @param[in] path Input parameter.
     * @return Return value.
     */
    std::string stripTenantPath(std::string_view path) const;

    // Combined path-rewrite result for namespace routing.
    // Returned by rewriteTenantPath() to convey both the effective path and
    // the tenant ID that was embedded in the original URL.
    struct PathRewriteResult {
        std::string effective_path; // stripped path, or original path if no prefix
        std::string tenant_id;      // tenant ID extracted from path, or empty string
        bool rewritten = false;     // true when the path contained a tenant prefix
    };

    /**
     * @brief Rewrite Tenant Path.
     * @param[in] path Input parameter.
     * @return Return value.
     */
    PathRewriteResult rewriteTenantPath(std::string_view path) const;

    /**
     * @brief Resolve Tenant By Domain.
     * @param[in] host Input parameter.
     * @return Return value.
     */
    std::optional<std::string> resolveTenantByDomain(std::string_view host) const;

    // Resource quota enforcement
    struct QuotaCheckResult {
        bool allowed = true;
        std::string reason = {};
    };
    
    QuotaCheckResult checkQuota(std::string_view tenant_id, 
                                 std::string_view resource_type,
                                 uint64_t requested_amount = 1) const;
    
    // Resource usage tracking
    /**
     * @brief Get Usage.
     * @param[in] tenant_id Identifier of the tenant.
     * @return Pointer to the result.
     */
    TenantUsage* getUsage(std::string_view tenant_id);

    /**
     * @brief Get Usage.
     * @param[in] tenant_id Identifier of the tenant.
     * @return Pointer to the result.
     */
    const TenantUsage* getUsage(std::string_view tenant_id) const;
    
    /**
     * @brief Increment Storage.
     * @param[in] tenant_id Identifier of the tenant.
     * @param[in] bytes Input parameter.
     */
    void incrementStorage(std::string_view tenant_id, int64_t bytes);

    /**
     * @brief Increment Documents.
     * @param[in] tenant_id Identifier of the tenant.
     * @param[in] count Input parameter.
     */
    void incrementDocuments(std::string_view tenant_id, int64_t count);

    /**
     * @brief Increment Collections.
     * @param[in] tenant_id Identifier of the tenant.
     * @param[in] count Input parameter.
     */
    void incrementCollections(std::string_view tenant_id, int64_t count);

    /**
     * @brief Record Request.
     * @param[in] tenant_id Identifier of the tenant.
     */
    void recordRequest(std::string_view tenant_id);

    /**
     * @brief Record Query.
     * @param[in] tenant_id Identifier of the tenant.
     */
    void recordQuery(std::string_view tenant_id);

    /**
     * @brief Record Bytes Read.
     * @param[in] tenant_id Identifier of the tenant.
     * @param[in] bytes Input parameter.
     */
    void recordBytesRead(std::string_view tenant_id, uint64_t bytes);

    /**
     * @brief Record Bytes Written.
     * @param[in] tenant_id Identifier of the tenant.
     * @param[in] bytes Input parameter.
     */
    void recordBytesWritten(std::string_view tenant_id, uint64_t bytes);

    /**
     * @brief Record Rate Limited.
     * @param[in] tenant_id Identifier of the tenant.
     */
    void recordRateLimited(std::string_view tenant_id);
    
    // Connection tracking
    /**
     * @brief Acquire Connection.
     * @param[in] tenant_id Identifier of the tenant.
     * @return True when the operation succeeds.
     */
    bool acquireConnection(std::string_view tenant_id);

    /**
     * @brief Release Connection.
     * @param[in] tenant_id Identifier of the tenant.
     */
    void releaseConnection(std::string_view tenant_id);
    
    // Query tracking
    /**
     * @brief Acquire Query Slot.
     * @param[in] tenant_id Identifier of the tenant.
     * @return True when the operation succeeds.
     */
    bool acquireQuerySlot(std::string_view tenant_id);

    /**
     * @brief Release Query Slot.
     * @param[in] tenant_id Identifier of the tenant.
     */
    void releaseQuerySlot(std::string_view tenant_id);
    
    // Metrics (Prometheus format)
    /**
     * @brief Get Metrics.
     * @return Return value.
     */
    std::string getMetrics() const;
    
    /**
     * @brief Get Tenant Key Id.
     * @param[in] tenant_id Identifier of the tenant.
     * @return Return value.
     */
    std::string getTenantKeyId(std::string_view tenant_id) const;
    
private:
    TenantManager();
    ~TenantManager() = default;
    TenantManager(const TenantManager&) = delete;
    TenantManager& operator=(const TenantManager&) = delete;
    
    mutable std::mutex mutex_;
    Config config_;
    std::unordered_map<std::string, TenantConfig> tenants_;
    std::unordered_map<std::string, std::unique_ptr<TenantUsage>> usage_;
    // Reverse index: lower-case domain -> tenant_id
    // Reverse map: custom_domain -> tenant_id for O(1) Host-header lookups
    std::unordered_map<std::string, std::string> domain_to_tenant_;
    
    /**
     * @brief Ensure Default Tenant.
     */
    void ensureDefaultTenant();

    /**
     * @brief Rebuild Domain Index.
     */
    void rebuildDomainIndex();

    /**
     * @brief Normalise Domain.
     * @param[in] host Input parameter.
     * @return Return value.
     */
    static std::string normaliseDomain(std::string_view host);
};

class TenantContextGuard {
public:
    /**
     * @brief Tenant Context Guard.
     * @param[in] ctx Input parameter.
     * @return Return value.
     */
    explicit TenantContextGuard(const TenantContext& ctx) 
        : ctx_(ctx), 
          connection_acquired_(false),
          query_slot_acquired_(false) {
        auto& tm = TenantManager::instance();
        connection_acquired_ = tm.acquireConnection(ctx_.tenant_id);
        tm.recordRequest(ctx_.tenant_id);
    }
    
    ~TenantContextGuard() {
        auto& tm = TenantManager::instance();
        if (connection_acquired_) {
            tm.releaseConnection(ctx_.tenant_id);
        }
        if (query_slot_acquired_) {
            tm.releaseQuerySlot(ctx_.tenant_id);
        }
    }
    
    /**
     * @brief Acquire Query Slot.
     * @return True when the operation succeeds.
     * @details Calls: TenantManager::instance(), recordQuery().
     */
    bool acquireQuerySlot() {
        if (!query_slot_acquired_) {
            auto& tm = TenantManager::instance();
            query_slot_acquired_ = tm.acquireQuerySlot(ctx_.tenant_id);
            if (query_slot_acquired_) {
                tm.recordQuery(ctx_.tenant_id);
            }
        }
        return query_slot_acquired_;
    }
    
    const TenantContext& context() const { return ctx_; }

    bool hasConnection() const { return connection_acquired_; }

    bool hasQuerySlot() const { return query_slot_acquired_; }
    
private:
    TenantContext ctx_;
    bool connection_acquired_;
    bool query_slot_acquired_;
};

} // namespace themis
