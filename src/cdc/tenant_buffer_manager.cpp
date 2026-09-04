/**
 * @file tenant_buffer_manager.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=7, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "cdc/tenant_buffer_manager.h"

#include <algorithm>

#include "utils/logger.h"

namespace themis {
namespace cdc {

TenantBufferManager::TenantBufferManager(Changefeed *changefeed, const ChangefeedBufferConfig &default_config)
    : changefeed_(changefeed), default_config_(default_config) {
    if (!changefeed_) {
        throw error::invalidArgument("changefeed", "Cannot be null");
    }
}

TenantBufferManager::~TenantBufferManager() noexcept {
    try {
        stop();
    } catch (...) {
        // Destructors must not throw while stopping tenant-owned buffers.
    }
}

void TenantBufferManager::start() {
    if (running_.exchange(true)) {
        THEMIS_WARN("TenantBufferManager already running");
        return;
    }

    std::lock_guard<std::mutex> lock(buffers_mutex_);
    for (auto &[tenant_id, state] : tenant_buffers_) {
        if (state.buffer && state.enabled) {
            state.buffer->start();
        }
    }

    THEMIS_INFO("TenantBufferManager started with {} tenants",static_cast<int>(tenant_buffers_.size()));
}

void TenantBufferManager::stop() {
    if (!running_.exchange(false)) {
        return; // Already stopped
    }

    std::lock_guard<std::mutex> lock(buffers_mutex_);

    // Stop all tenant buffers
    for (auto &[tenant_id, state] : tenant_buffers_) {
        if (state.buffer) {
            state.buffer->stop();
        }
    }

    THEMIS_INFO("TenantBufferManager stopped");
}

Changefeed::ChangeEvent TenantBufferManager::recordEvent(const std::string &tenant_id, Changefeed::ChangeEvent event) {
    if (!running_) {
        throw CDCException(ErrorCode::BUFFER_NOT_RUNNING, ErrorSeverity::ERROR, "TenantBufferManager not running",
                           "tenant_id=" + tenant_id);
    }

    if (tenant_id.empty()) {
        throw error::invalidArgument("tenant_id", "Cannot be empty");
    }

    std::lock_guard<std::mutex> lock(buffers_mutex_);

    // Get or create tenant buffer
    auto &state = getOrCreateTenantBuffer(tenant_id);

    // Check if tenant is enabled
    if (!state.enabled) {
        throw CDCException(ErrorCode::TENANT_UNAUTHORIZED, ErrorSeverity::ERROR, "Tenant is disabled",
                           "tenant_id=" + tenant_id);
    }

    // Check quota if enabled
    if (state.config.enable_quotas) {
        if (!checkTenantQuota(tenant_id, state)) {
            state.stats.quota_violations++;
            throw CDCException(ErrorCode::TENANT_QUOTA_EXCEEDED, ErrorSeverity::WARNING, "Tenant quota exceeded",
                               "tenant_id=" + tenant_id);
        }
    }

    // Record event
    try {
        auto recorded_event = state.buffer->recordEvent(std::move(event));

        // Update stats
        state.stats.events_recorded++;
        state.last_event_time = std::chrono::steady_clock::now();
        updateTenantStats(tenant_id, state);

        return recorded_event;
    } catch (const std::string&) {
        state.stats.errors++;
        throw;
    } catch (const char*) {
        state.stats.errors++;
        throw;
    } catch (...) {
        state.stats.errors++;
        throw;
    }
}

size_t TenantBufferManager::flushTenant(const std::string &tenant_id) {
    std::lock_guard<std::mutex> lock(buffers_mutex_);

    auto it = tenant_buffers_.find(tenant_id);
    if (it == tenant_buffers_.end()) {
        return 0;
    }

    auto &state = it->second;
    if (!state.buffer) {
        return 0;
    }

    size_t flushed = state.buffer->flush();
    state.stats.events_flushed += flushed;
    updateTenantStats(tenant_id, state);

    return flushed;
}

size_t TenantBufferManager::flushAll() {
    std::lock_guard<std::mutex> lock(buffers_mutex_);

    size_t total_flushed = 0;
    for (auto &[tenant_id, state] : tenant_buffers_) {
        if (state.buffer) {
            size_t flushed = state.buffer->flush();
            state.stats.events_flushed += flushed;
            updateTenantStats(tenant_id, state);
            total_flushed += flushed;
        }
    }

    return total_flushed;
}

void TenantBufferManager::configureTenant(const TenantConfig &config) {
    if (config.tenant_id.empty()) {
        throw error::invalidArgument("tenant_id", "Cannot be empty");
    }

    std::lock_guard<std::mutex> lock(buffers_mutex_);

    auto it = tenant_buffers_.find(config.tenant_id);
    if (it != tenant_buffers_.end()) {
        // Update existing tenant config
        it->second.config  = config;
        it->second.enabled = config.enabled;

        // Note: buffer config changes take effect on next buffer creation/restart
        THEMIS_INFO("Updated config for tenant: {}", config.tenant_id);
    } else {
        // Create the buffer before publishing it into tenant state so ownership
        // stays within a local std::unique_ptr if construction/startup throws.
        auto buffer = std::make_unique<ChangefeedBuffer>(changefeed_, config.buffer_config);
        if (running_ && config.enabled) {
            buffer->start();
        }

        auto [new_it, inserted] = tenant_buffers_.try_emplace(config.tenant_id);
        auto &state             = new_it->second;
        state.config            = config;
        state.stats.tenant_id   = config.tenant_id;
        state.enabled           = config.enabled;
        state.buffer            = std::move(buffer);

        THEMIS_INFO("Created new tenant: {}", config.tenant_id);
    }
}

std::optional<TenantConfig> TenantBufferManager::getTenantConfig(const std::string &tenant_id) const {
    std::lock_guard<std::mutex> lock(buffers_mutex_);

    auto it = tenant_buffers_.find(tenant_id);
    if (it != tenant_buffers_.end()) {
        return it->second.config;
    }
    return std::nullopt;
}

std::optional<TenantStats> TenantBufferManager::getTenantStats(const std::string &tenant_id) const {
    std::lock_guard<std::mutex> lock(buffers_mutex_);

    auto it = tenant_buffers_.find(tenant_id);
    if (it != tenant_buffers_.end()) {
        return it->second.stats;
    }
    return std::nullopt;
}

std::optional<std::reference_wrapper<const CDCMetrics>>
TenantBufferManager::getTenantMetrics(const std::string &tenant_id) const {
    std::lock_guard<std::mutex> lock(buffers_mutex_);

    auto it = tenant_buffers_.find(tenant_id);
    if (it != tenant_buffers_.end() && it->second.buffer) {
        return std::cref(it->second.buffer->getMetrics());
    }
    return std::nullopt;
}

nlohmann::json TenantBufferManager::getGlobalMetrics() const {
    std::lock_guard<std::mutex> lock(buffers_mutex_);

    uint64_t events_recorded     = 0;
    uint64_t events_flushed      = 0;
    uint64_t flush_count         = 0;
    uint64_t compression_count   = 0;
    uint64_t decompression_count = 0;
    uint64_t errors              = 0;
    uint64_t retries             = 0;

    for (const auto &[tenant_id, state] : tenant_buffers_) {
        if (!state.buffer) {
            continue;
        }

        const auto &tenant_metrics = state.buffer->getMetrics();

        // Aggregate counters
        events_recorded += tenant_metrics.events_recorded.load();
        events_flushed += tenant_metrics.events_flushed.load();
        flush_count += tenant_metrics.flush_count.load();
        compression_count += tenant_metrics.compression_count.load();
        decompression_count += tenant_metrics.decompression_count.load();
        errors += tenant_metrics.errors.load();
        retries += tenant_metrics.retries.load();

        // Note: Histograms and throughput don't aggregate meaningfully
        // Would need weighted averaging or separate tracking
    }

    return {{"events_recorded", events_recorded},
            {"events_flushed", events_flushed},
            {"flush_count", flush_count},
            {"compression_count", compression_count},
            {"decompression_count", decompression_count},
            {"errors", errors},
            {"retries", retries}};
}

std::map<std::string, TenantStats> TenantBufferManager::getAllTenantStats() const {
    std::lock_guard<std::mutex> lock(buffers_mutex_);

    std::map<std::string, TenantStats> all_stats = {};

    for (const auto &[tenant_id, state] : tenant_buffers_) {
        all_stats[tenant_id] = state.stats;
    }

    return all_stats;
}

std::vector<std::string> TenantBufferManager::getActiveTenants() const {
    std::lock_guard<std::mutex> lock(buffers_mutex_);

    std::vector<std::string> tenants = {};

    tenants.reserve(tenant_buffers_.size());

    for (const auto &[tenant_id, state] : tenant_buffers_) {
        if (state.enabled) {
            tenants.push_back(tenant_id);
        }
    }

    return tenants;
}

bool TenantBufferManager::hasTenant(const std::string &tenant_id) const {
    std::lock_guard<std::mutex> lock(buffers_mutex_);
    return tenant_buffers_.find(tenant_id) != tenant_buffers_.end();
}

void TenantBufferManager::disableTenant(const std::string &tenant_id) {
    std::lock_guard<std::mutex> lock(buffers_mutex_);

    auto it = tenant_buffers_.find(tenant_id);
    if (it != tenant_buffers_.end()) {
        it->second.enabled        = false;
        it->second.config.enabled = false;

        if (it->second.buffer) {
            it->second.buffer->stop();
        }

        THEMIS_INFO("Disabled tenant: {}", tenant_id);
    }
}

void TenantBufferManager::enableTenant(const std::string &tenant_id) {
    std::lock_guard<std::mutex> lock(buffers_mutex_);

    auto it = tenant_buffers_.find(tenant_id);
    if (it != tenant_buffers_.end()) {
        it->second.enabled        = true;
        it->second.config.enabled = true;

        if (it->second.buffer && running_) {
            it->second.buffer->start();
        }

        THEMIS_INFO("Enabled tenant: {}", tenant_id);
    }
}

void TenantBufferManager::removeTenant(const std::string &tenant_id) {
    std::lock_guard<std::mutex> lock(buffers_mutex_);

    auto it = tenant_buffers_.find(tenant_id);
    if (it != tenant_buffers_.end()) {
        // Flush before removing
        if (it->second.buffer) {
            it->second.buffer->flush();
            it->second.buffer->stop();
        }

        tenant_buffers_.erase(it);

        THEMIS_INFO("Removed tenant: {}", tenant_id);
    }
}

// Private helper methods

TenantBufferManager::TenantBufferState &TenantBufferManager::getOrCreateTenantBuffer(const std::string &tenant_id) {
    // Must be called with lock held

    auto it = tenant_buffers_.find(tenant_id);
    if (it != tenant_buffers_.end()) {
        return it->second;
    }

    // Create the buffer up front so any constructor/start failure occurs before
    // ownership is published into the tenant state map.
    auto buffer = std::make_unique<ChangefeedBuffer>(changefeed_, default_config_);
    if (running_) {
        buffer->start();
    }

    auto [inserted_it, inserted] = tenant_buffers_.try_emplace(tenant_id);
    auto &state                  = inserted_it->second;
    state.config.tenant_id       = tenant_id;
    state.config.buffer_config   = default_config_;
    state.stats.tenant_id        = tenant_id;
    state.buffer                 = std::move(buffer);

    THEMIS_INFO("Auto-created buffer for new tenant: {}", tenant_id);
    return inserted_it->second;
}

bool TenantBufferManager::checkTenantQuota(const std::string &tenant_id, TenantBufferState &state) {
    // Must be called with lock held

    if (!state.config.enable_quotas) {
        return true;
    }

    // Check buffer size quota
    if (state.config.max_buffered_events > 0) {
        const auto &buffer_stats = state.buffer->getStats();
        if (buffer_stats.current_buffer_size >= state.config.max_buffered_events) {
            THEMIS_WARN("Tenant {} exceeded max buffered events quota: {} >= {}", tenant_id,
                        buffer_stats.current_buffer_size, state.config.max_buffered_events);
            return false;
        }
    }

    // Check memory quota
    if (state.config.max_memory_bytes > 0) {
        const auto &buffer_stats = state.buffer->getStats();
        if (buffer_stats.current_buffer_memory >= state.config.max_memory_bytes) {
            THEMIS_WARN("Tenant {} exceeded memory quota: {} >= {} bytes", tenant_id,
                        buffer_stats.current_buffer_memory, state.config.max_memory_bytes);
            return false;
        }
    }

    // Rate limiting is handled by buffer's internal rate limiter
    // Could add additional per-tenant rate limiting here

    return true;
}

void TenantBufferManager::updateTenantStats([[maybe_unused]] const std::string &tenant_id, TenantBufferState &state) {
    // Must be called with lock held

    if (!state.buffer) {
        return;
    }

    const auto &buffer_stats = state.buffer->getStats();
    const auto &metrics      = state.buffer->getMetrics();

    // Update stats from buffer
    state.stats.current_buffer_size  = buffer_stats.current_buffer_size;
    state.stats.current_memory_bytes = buffer_stats.current_buffer_memory;

    // Calculate rates
    state.stats.events_per_second = metrics.throughput.eventsPerSecond();

    // Calculate memory usage percent
    if (state.config.max_memory_bytes > 0) {
        state.stats.memory_usage_percent
            = (static_cast<double>(state.stats.current_memory_bytes) / state.config.max_memory_bytes) * 100.0;
    }
}

} // namespace cdc
} // namespace themis

