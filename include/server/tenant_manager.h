/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            tenant_manager.h                                   ║
  Version:         0.0.38                                             ║
  Last Modified:   2026-04-13 04:20:18                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     394                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 20db61010a  2026-03-01  feat(server): add per-tenant custom domain routing ║
    • aa48b6bd38  2026-03-01  feat(server): add per-tenant custom domain routing via Ho... ║
    • 9b809d400a  2026-02-24  audit: fix stale banner metadata and update documentation... ║
    • 5375c249ad  2026-02-23  refactor(api): eliminate duplicated tenant path rewriting... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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

/**
 * @brief Tenant configuration and resource quotas
 * 
 * Defines limits and settings for a specific tenant in multi-tenant deployments.
 */
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

/**
 * @brief Current tenant context for request processing
 * 
 * Thread-local context containing tenant information for the current request.
 */
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

/**
 * @brief Tenant usage statistics and metrics
 */
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

/**
 * @brief Multi-tenant management for ThemisDB
 * 
 * Provides tenant isolation, resource quotas, and tenant-aware request routing.
 * Supports both header-based and path-based tenant identification.
 * 
 * Usage:
 * @code
 *   auto& tm = TenantManager::instance();
 *   
 *   // Create tenant
 *   TenantConfig cfg;
 *   cfg.tenant_id = "acme-corp";
 *   cfg.display_name = "ACME Corporation";
 *   cfg.max_storage_bytes = 10 * 1024 * 1024 * 1024ULL; // 10 GB
 *   tm.createTenant(cfg);
 *   
 *   // Resolve tenant from request
 *   auto ctx = tm.resolveContext(headers, path);
 *   if (ctx) {
 *       // Process request in tenant context
 *   }
 * @endcode
 */
class TenantManager {
public:
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
    
    CreateResult createTenant(const TenantConfig& config);
    bool updateTenant(const TenantConfig& config);
    bool deleteTenant(std::string_view tenant_id);
    bool setTenantEnabled(std::string_view tenant_id, bool enabled);
    
    // Tenant lookup
    std::optional<TenantConfig> getTenant(std::string_view tenant_id) const;
    std::vector<TenantConfig> listTenants() const;
    bool tenantExists(std::string_view tenant_id) const;
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

    // Custom domain routing management.
    // Registers a domain -> tenant mapping so that incoming requests whose
    // Host (or configured custom_domain_host_header) value matches `domain`
    // are routed to `tenant_id`.  The domain is stored in lower-case.
    // Returns false when the domain is already registered to a different tenant
    // or when the tenant does not exist.
    bool registerCustomDomain(std::string_view tenant_id, std::string_view domain);

    // Removes a previously registered custom domain mapping.
    // Returns true when the mapping existed and was removed.
    bool unregisterCustomDomain(std::string_view domain);

    // Looks up the tenant ID for a custom domain (case-insensitive).
    // Strips a trailing port (":NNN") from `host` before matching.
    // Returns an empty optional when no mapping exists.
    std::optional<std::string> lookupTenantByDomain(std::string_view host) const;

    // Strip tenant path prefix from a URL path for namespace routing.
    // For path-based tenant routing, removes the "/tenants/{id}/" prefix so
    // the remaining path can be matched against normal API routes.
    // Example: "/tenants/acme-corp/documents/123" -> "/documents/123"
    // Returns the path unchanged when it does not start with the tenant prefix.
    std::string stripTenantPath(std::string_view path) const;

    // Combined path-rewrite result for namespace routing.
    // Returned by rewriteTenantPath() to convey both the effective path and
    // the tenant ID that was embedded in the original URL.
    struct PathRewriteResult {
        std::string effective_path; // stripped path, or original path if no prefix
        std::string tenant_id;      // tenant ID extracted from path, or empty string
        bool rewritten = false;     // true when the path contained a tenant prefix
    };

    // Strips the tenant path prefix (if present) and extracts the tenant ID.
    // Combines extractTenantId-from-path with stripTenantPath in a single pass
    // to avoid repeated prefix scans in session-layer code.
    // Example: "/tenants/acme-corp/documents/123"
    //   -> { effective_path="/documents/123", tenant_id="acme-corp", rewritten=true }
    PathRewriteResult rewriteTenantPath(std::string_view path) const;

    // Resolve tenant ID from an HTTP Host header value.
    // Returns the tenant ID if a tenant with a matching custom_domain is found,
    // or std::nullopt when no domain mapping exists for the given host.
    // The host value may include a port suffix (e.g. "acme.example.com:8443");
    // the port is stripped before lookup.
    std::optional<std::string> resolveTenantByDomain(std::string_view host) const;

    // Resource quota enforcement
    struct QuotaCheckResult {
        bool allowed = true;
        std::string reason;
    };
    
    QuotaCheckResult checkQuota(std::string_view tenant_id, 
                                 std::string_view resource_type,
                                 uint64_t requested_amount = 1) const;
    
    // Resource usage tracking
    TenantUsage* getUsage(std::string_view tenant_id);
    const TenantUsage* getUsage(std::string_view tenant_id) const;
    
    void incrementStorage(std::string_view tenant_id, int64_t bytes);
    void incrementDocuments(std::string_view tenant_id, int64_t count);
    void incrementCollections(std::string_view tenant_id, int64_t count);
    void recordRequest(std::string_view tenant_id);
    void recordQuery(std::string_view tenant_id);
    void recordBytesRead(std::string_view tenant_id, uint64_t bytes);
    void recordBytesWritten(std::string_view tenant_id, uint64_t bytes);
    void recordRateLimited(std::string_view tenant_id);
    
    // Connection tracking
    bool acquireConnection(std::string_view tenant_id);
    void releaseConnection(std::string_view tenant_id);
    
    // Query tracking
    bool acquireQuerySlot(std::string_view tenant_id);
    void releaseQuerySlot(std::string_view tenant_id);
    
    // Metrics (Prometheus format)
    std::string getMetrics() const;
    
    // Key derivation for tenant-specific encryption
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
    
    // Helper to create default tenant
    void ensureDefaultTenant();

    // Rebuild the domain_to_tenant_ index from current tenants_ map.
    // Must be called with mutex_ held.
    void rebuildDomainIndex();

    // Returns lower-case copy of `host` with optional ":port" suffix stripped.
    static std::string normaliseDomain(std::string_view host);
};

/**
 * @brief RAII guard for tenant context
 * 
 * Sets up tenant context for the current scope and cleans up on destruction.
 */
class TenantContextGuard {
public:
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
