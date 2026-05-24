/*
 * ThemisDB | File: toolbox_registry.cpp | Version: 0.1.0 | Last Modified: 2026-05-20 17:13:04
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 94
 * Open Issues: TODOs=1, Stubs=1, Gaps=3, Unimpl=0, Mock=1, Sim=0, Debt=0
 * Gap Correlation: internal=3 | external_v3=16 | delta=13 | status=divergent
 * External Severity (v3): C=0, H=14, M=2
 * PR: none
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#include "toolbox/toolbox_registry.h"

#include <mutex>
#include <stdexcept>

namespace themis {
namespace toolbox {

// ─────────────────────────────────────────────────────────────────────────────
// Internal storage
// ─────────────────────────────────────────────────────────────────────────────

namespace {

std::mutex                         g_mutex;
std::shared_ptr<IngestionToolbox>  g_instance;

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// ToolboxRegistry
// ─────────────────────────────────────────────────────────────────────────────

void ToolboxRegistry::initialize(std::shared_ptr<IngestionToolbox> toolbox) {
    if (!toolbox) {
        throw std::invalid_argument(
            "ToolboxRegistry::initialize: toolbox must not be null");
    }
    std::lock_guard<std::mutex> lk(g_mutex);
    g_instance = std::move(toolbox);
}

std::shared_ptr<IngestionToolbox> ToolboxRegistry::instance() {
    std::lock_guard<std::mutex> lk(g_mutex);
    if (!g_instance) {
        throw std::logic_error(
            "ToolboxRegistry::instance: registry not initialised — "
            "call ToolboxRegistry::initialize() at server startup");
    }
    return g_instance;
}

bool ToolboxRegistry::isInitialized() noexcept {
    std::lock_guard<std::mutex> lk(g_mutex);
    return g_instance != nullptr;
}

void ToolboxRegistry::reset() noexcept {
    std::lock_guard<std::mutex> lk(g_mutex);
    g_instance.reset();
}

// ─────────────────────────────────────────────────────────────────────────────
// Free functions
// ─────────────────────────────────────────────────────────────────────────────

void initializeToolbox(std::shared_ptr<IngestionToolbox> toolbox) {
    ToolboxRegistry::initialize(std::move(toolbox));
}

std::shared_ptr<IngestionToolbox> globalToolbox() {
    return ToolboxRegistry::instance();
}

std::vector<ingestion::BaseEntity> extractEntities(
    const std::string& text,
    const std::string& mime,
    const std::string& filename)
{
    return ToolboxRegistry::instance()->extractEntities(text, mime, filename);
}

ingestion::BaseEntitySet extractEntitySet(
    const std::string& text,
    const std::string& mime,
    const std::string& filename)
{
    return ToolboxRegistry::instance()->extractEntitySet(text, mime, filename);
}

std::string getMetricsText() {
    return ToolboxRegistry::instance()->getMetricsText();
}

} // namespace toolbox
} // namespace themis
