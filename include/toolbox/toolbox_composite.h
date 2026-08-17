/**
 * @file toolbox_composite.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "toolbox/ingestion_toolbox.h"
#include "ingestion/base_entity.h"
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace themis {
namespace toolbox {

// ─────────────────────────────────────────────────────────────────────────────
// ToolboxComposite
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief MIME-routing composite that dispatches entity extraction to the
 *        appropriate `IngestionToolbox` based on MIME-type prefix.
 *
 * Thread-safety: the routing table is immutable after construction;
 * concurrent `extractEntities()` calls are safe.
 */
class ToolboxComposite {
public:
    using Route = std::pair<std::string, std::shared_ptr<IngestionToolbox>>;

    /**
     * @brief Construct with a routing table and optional fallback.
     *
     * @param routes   Ordered list of (mime_prefix, toolbox) pairs.
     * @param fallback Toolbox to use when no route matches.  May be null.
     */
    explicit ToolboxComposite(
        std::vector<Route>                  routes,
        std::shared_ptr<IngestionToolbox>   fallback = nullptr);

    ~ToolboxComposite();

    ToolboxComposite(const ToolboxComposite&)            = delete;
    ToolboxComposite& operator=(const ToolboxComposite&) = delete;
    ToolboxComposite(ToolboxComposite&&)                 noexcept = default;
    ToolboxComposite& operator=(ToolboxComposite&&)      noexcept = default;

    /**
     * @brief Extract entities, dispatching to the matching toolbox.
     *
     * Finds the first route whose prefix matches the start of @p mime and
     * delegates to its `IngestionToolbox::extractEntities()`.  Falls back to
     * the fallback toolbox if no route matches.
     *
     * @param text      UTF-8 text to process.
     * @param mime      MIME type used for routing (e.g. "text/plain",
     *                  "application/pdf").
     * @param filename  Filename hint forwarded to the selected toolbox.
     * @return Extracted entities, or an empty vector when no toolbox is
     *         available for the given MIME type.
     */
    std::vector<ingestion::BaseEntity> extractEntities(
        const std::string& text,
        const std::string& mime     = "text/plain",
        const std::string& filename = "input.txt");

    /**
     * @brief Return the registered routes (for inspection / testing).
     */
    const std::vector<Route>& routes() const noexcept;

    /**
     * @brief Return the fallback toolbox (may be null).
     */
    std::shared_ptr<IngestionToolbox> fallback() const noexcept;

    /**
     * @brief Look up the toolbox that would be used for @p mime.
     *
     * Returns `nullptr` when no route and no fallback match.
     */
    std::shared_ptr<IngestionToolbox> resolve(const std::string& mime) const;

private:
    std::vector<Route>                routes_;
    std::shared_ptr<IngestionToolbox> fallback_;
};

// ─────────────────────────────────────────────────────────────────────────────
// ToolboxCompositeBuilder
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Fluent builder for `ToolboxComposite`.
 *
 * Routes are stored in insertion order; call `addRoute()` multiple times.
 * At least one route or a fallback must be provided before `build()`.
 *
 * @code
 * auto composite = ToolboxCompositeBuilder()
 *     .addRoute("text/html",  html_toolbox)
 *     .addRoute("text/",      text_toolbox)
 *     .setFallback(default_toolbox)
 *     .build();
 * @endcode
 */
class ToolboxCompositeBuilder {
public:
    ToolboxCompositeBuilder() = default;

    /**
     * @brief Register a MIME-prefix → toolbox route.
     *
     * @param mime_prefix  Prefix to match against the `mime` argument of
     *                     `extractEntities()`.  E.g. `"text/"` matches any
     *                     `"text/plain"`, `"text/html"`, …
     * @param toolbox      Toolbox to use when the prefix matches.  Must not
     *                     be null.
     */
    ToolboxCompositeBuilder& addRoute(
        std::string                       mime_prefix,
        std::shared_ptr<IngestionToolbox> toolbox);

    /**
     * @brief Set the fallback toolbox used when no route matches.
     *
     * @param toolbox  May be null (disables the fallback).
     */
    ToolboxCompositeBuilder& setFallback(
        std::shared_ptr<IngestionToolbox> toolbox);

    /**
     * @brief Build the `ToolboxComposite`.
     *
     * @throws std::logic_error when no routes and no fallback have been added.
     */

#pragma once
    std::unique_ptr<ToolboxComposite> build();

private:
    std::vector<ToolboxComposite::Route> routes_;
    std::shared_ptr<IngestionToolbox>    fallback_;
};

} // namespace toolbox
} // namespace themis
