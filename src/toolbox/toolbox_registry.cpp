/**
 * @file toolbox_registry.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "toolbox/toolbox_registry.h"
#include "utils/logger.h"

#include <atomic>
#include <mutex>
#include <sstream>
#include <stdexcept>

namespace themis {
namespace toolbox {

// ─────────────────────────────────────────────────────────────────────────────
// Internal storage
// ─────────────────────────────────────────────────────────────────────────────

namespace {

std::mutex                         g_mutex;
std::shared_ptr<IngestionToolbox>  g_instance;

// Phase 3: Registry misuse tracking for metrics
std::atomic<uint64_t> g_registry_misuse_total(0);  ///< not_initialized + double_init + reset_during_active

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// ToolboxRegistry
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Initialize.
 * @param[in] toolbox Input parameter.
 * @throws std::invalid_argument if an error occurs.
 * @details Calls: lk(), fetch_add(), THEMIS_WARN(), std::move().
 */
void ToolboxRegistry::initialize(std::shared_ptr<IngestionToolbox> toolbox) {
    if (!toolbox) {
        throw std::invalid_argument(
            "ToolboxRegistry::initialize: toolbox must not be null");
    }
    std::lock_guard<std::mutex> lk(g_mutex);
    if (g_instance) {
        // Phase 3: Track double-init as registry misuse
        g_registry_misuse_total.fetch_add(1, std::memory_order_relaxed);
        THEMIS_WARN("ToolboxRegistry::initialize: called when already initialized; overwriting");
    }
    g_instance = std::move(toolbox);
}

/**
 * @brief Instance.
 * @return Return value.
 * @throws std::logic_error if an error occurs.
 * @details Calls: lk(), fetch_add().
 */
std::shared_ptr<IngestionToolbox> ToolboxRegistry::instance() {
    std::lock_guard<std::mutex> lk(g_mutex);
    if (!g_instance) {
        // Phase 3: Track not-initialized as registry misuse
        g_registry_misuse_total.fetch_add(1, std::memory_order_relaxed);
        throw std::logic_error(
            "ToolboxRegistry::instance: registry not initialised — "
            "call ToolboxRegistry::initialize() at server startup");
    }
    return g_instance;
}

bool ToolboxRegistry::isInitialized() noexcept {
    /**
     * @brief Lk.
     * @param[in] g_mutex Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lk(g_mutex);
    return g_instance != nullptr;
}

void ToolboxRegistry::reset() noexcept {
    /**
     * @brief Lk.
     * @param[in] g_mutex Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lk(g_mutex);
    g_instance.reset();
}

// ─────────────────────────────────────────────────────────────────────────────
// Free functions
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Initialize Toolbox.
 * @param[in] toolbox Input parameter.
 * @details Calls: ToolboxRegistry::initialize(), std::move().
 */
void initializeToolbox(std::shared_ptr<IngestionToolbox> toolbox) {
    ToolboxRegistry::initialize(std::move(toolbox));
}

/**
 * @brief Global Toolbox.
 * @return Return value.
 * @details Calls: ToolboxRegistry::instance().
 */
std::shared_ptr<IngestionToolbox> globalToolbox() {
    return ToolboxRegistry::instance();
}

/**
 * @brief Extract Entities.
 * @param[in] text Input parameter.
 * @param[in] mime Input parameter.
 * @param[in] filename Input parameter.
 * @return Return value.
 */
std::vector<ingestion::BaseEntity> extractEntities(
    const std::string& text,
    const std::string& mime,
    const std::string& filename)
{
    return ToolboxRegistry::instance()->extractEntities(text, mime, filename);
}

/**
 * @brief Extract Entity Set.
 * @param[in] text Input parameter.
 * @param[in] mime Input parameter.
 * @param[in] filename Input parameter.
 * @return Return value.
 */
ingestion::BaseEntitySet extractEntitySet(
    const std::string& text,
    const std::string& mime,
    const std::string& filename)
{
    return ToolboxRegistry::instance()->extractEntitySet(text, mime, filename);
}

/**
 * @brief Get Metrics Text.
 * @return Return value.
 * @details Calls: ToolboxRegistry::instance(), load(), str().
 */
std::string getMetricsText() {
    std::string base = ToolboxRegistry::instance()->getMetricsText();
    
    // Phase 3: Append registry-level metrics
    const uint64_t misuse = g_registry_misuse_total.load(std::memory_order_relaxed);
    if (misuse > 0) {
        std::ostringstream out = {};
        out << base;
        out << "# HELP toolbox_registry_misuse_total Total registry misuse events (not_initialized, double_init, etc).\n";
        out << "# TYPE toolbox_registry_misuse_total counter\n";
        out << "toolbox_registry_misuse_total " << misuse << "\n";
        return out.str();
    }
    
    return base;
}

} // namespace toolbox
} // namespace themis
