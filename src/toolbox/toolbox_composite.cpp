/**
 * @file toolbox_composite.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "toolbox/toolbox_composite.h"

#include <algorithm>
#include <stdexcept>

namespace themis {
namespace toolbox {

// ─────────────────────────────────────────────────────────────────────────────
// ToolboxComposite
// ─────────────────────────────────────────────────────────────────────────────

ToolboxComposite::ToolboxComposite(
    std::vector<Route>                routes,
    std::shared_ptr<IngestionToolbox> fallback)
    : routes_(std::move(routes))
    , fallback_(std::move(fallback))
{}

ToolboxComposite::~ToolboxComposite() = default;

std::shared_ptr<IngestionToolbox> ToolboxComposite::resolve(
    const std::string& mime) const
{
    for (const auto& [prefix, toolbox] : routes_) {
        if (mime.size() >= prefix.size() &&
            mime.compare(0,prefix.size(), prefix) == 0)
        {
            return toolbox;
        }
    }
    return fallback_;
}

/**
 * @brief Extract Entities.
 * @param[in] text Input parameter.
 * @param[in] mime Input parameter.
 * @param[in] filename Input parameter.
 * @return Return value.
 */
std::vector<ingestion::BaseEntity> ToolboxComposite::extractEntities(
    const std::string& text,
    const std::string& mime,
    const std::string& filename)
{
    auto toolbox = resolve(mime);
    if (!toolbox) {
        return {};
    }
    return toolbox->extractEntities(text, mime, filename);
}

const std::vector<ToolboxComposite::Route>& ToolboxComposite::routes() const noexcept {
    return routes_;
}

std::shared_ptr<IngestionToolbox> ToolboxComposite::fallback() const noexcept {
    return fallback_;
}

// ─────────────────────────────────────────────────────────────────────────────
// ToolboxCompositeBuilder
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Add Route.
 * @param[in] mime_prefix Input parameter.
 * @param[in] toolbox Input parameter.
 * @return Return value.
 */
ToolboxCompositeBuilder& ToolboxCompositeBuilder::addRoute(
    std::string                       mime_prefix,
    std::shared_ptr<IngestionToolbox> toolbox)
{
    if (!toolbox) {
        throw std::invalid_argument(
            "ToolboxCompositeBuilder::addRoute: toolbox must not be null");
    }
    routes_.emplace_back(std::move(mime_prefix), std::move(toolbox));
    return *this;
}

/**
 * @brief Set Fallback.
 * @param[in] toolbox Input parameter.
 * @return Return value.
 */
ToolboxCompositeBuilder& ToolboxCompositeBuilder::setFallback(
    std::shared_ptr<IngestionToolbox> toolbox)
{
    fallback_ = std::move(toolbox);
    return *this;
}

/**
 * @brief Build.
 * @return Return value.
 * @throws std::logic_error if an error occurs.
 * @details Calls: empty(), std::move().
 */
std::unique_ptr<ToolboxComposite> ToolboxCompositeBuilder::build() {
    if (routes_.empty() && !fallback_) {
        throw std::logic_error(
            "ToolboxCompositeBuilder::build: no routes and no fallback added");
    }
    return std::make_unique<ToolboxComposite>(std::move(routes_), std::move(fallback_));
}

} // namespace toolbox
} // namespace themis
