/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            toolbox_composite.h                                ║
  Version:         0.1.0                                              ║
  Last Modified:   2026-04-20                                         ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

/**
 * @file toolbox_composite.h
 * @brief MIME-routing composite toolbox for multi-format pipelines.
 *
 * `ToolboxComposite` routes `extractEntities()` calls to a registered
 * `IngestionToolbox` based on MIME-type prefix matching.  This allows
 * different toolbox configurations (e.g. a legal-document toolbox vs. a
 * medical-text toolbox) to be selected automatically based on the content type
 * without any `if/else` chains in consumer code.
 *
 * ## Usage
 * @code
 * auto composite = themis::toolbox::ToolboxCompositeBuilder()
 *     .addRoute("text/",        text_toolbox)
 *     .addRoute("application/pdf", pdf_toolbox)
 *     .setFallback(default_toolbox)
 *     .build();
 *
 * // Routes to pdf_toolbox because mime starts with "application/pdf"
 * auto entities = composite->extractEntities(text, "application/pdf", "doc.pdf");
 * @endcode
 *
 * ## Routing rules
 *
 * Routes are checked in **insertion order**.  The first route whose prefix
 * matches the beginning of the `mime` argument wins.  If no route matches,
 * the fallback toolbox is used.  If no fallback is set, `extractEntities()`
 * returns an empty vector.
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
    ToolboxComposite(ToolboxComposite&&)                 = default;
    ToolboxComposite& operator=(ToolboxComposite&&)      = default;

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
    std::unique_ptr<ToolboxComposite> build();

private:
    std::vector<ToolboxComposite::Route> routes_;
    std::shared_ptr<IngestionToolbox>    fallback_;
};

} // namespace toolbox
} // namespace themis
