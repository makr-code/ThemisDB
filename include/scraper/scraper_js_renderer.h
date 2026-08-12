/**
 * @file scraper_js_renderer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.11
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=2, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>

namespace themis {
namespace scraper {

// ============================================================================
// JS renderer types
// ============================================================================

/**
 * @brief Parameters for a single JavaScript page rendering request.
 */
struct JsRenderRequest {
    std::string url;
    int         timeout_ms      = 10000;
    /// CSS selector to wait for before capturing HTML (empty = DOMContentLoaded)
    std::string wait_selector;
    std::map<std::string, std::string> headers;
    /// Extra arguments forwarded verbatim to the renderer command
    std::vector<std::string> extra_args;
};

/**
 * @brief Result of a single JavaScript page rendering request.
 *
 * When success == false, html is empty and error contains a diagnostic
 * string (e.g. timeout message or non-zero exit code).
 */
struct JsRenderResult {
    bool        success     = false;  ///< True when rendering succeeded and HTML was captured
    std::string html;                 ///< Fully-rendered HTML (empty on failure)
    std::string error;                ///< Non-empty diagnostic string when success == false
    int         status_code = 0;      ///< HTTP status code reported by the renderer (0 = unknown)
    long        elapsed_ms  = 0;      ///< Wall-clock rendering time in milliseconds
};

// ============================================================================
// Interface
// ============================================================================

/**
 * @brief Abstraction for JavaScript-capable page rendering.
 *
 * Used when a target page is a React/Vue/Angular SPA or when content is
 * injected by webpack-dev-server / Vite / Next.js at runtime.
 *
 * The default production implementation launches an external renderer
 * process (SubprocessJSRenderer).  Tests use InMemoryJSRenderer.
 */
class IScraperJSRenderer {
public:
    virtual ~IScraperJSRenderer() = default;

    /**
     * @brief Render a URL using a headless browser and return the resulting HTML.
     * @param req  Render request with URL, timeout, and optional CSS wait selector.
     * @return JsRenderResult — success==false with an error message on timeout
     *         or subprocess failure.  Never throws.
     */
    virtual JsRenderResult render(const JsRenderRequest& req) = 0;

    /**
     * @brief Returns true when the renderer backend is available and usable.
     *
     * For SubprocessJSRenderer this means the renderer command is non-empty
     * and its first token resolves to an executable on PATH.  Call before
     * ScraperPlugin::initialize() to validate JS rendering mode.
     */
    virtual bool isAvailable() const = 0;
};

// ============================================================================
// Subprocess renderer (production)
// ============================================================================

/**
 * @brief Invokes an external headless browser renderer via subprocess.
 *
 * The command line called is:
 *   <renderer_cmd> <url> [--timeout <ms>] [--wait-for <selector>] [extra_args…]
 *
 * The renderer must:
 *  - Write the fully-rendered HTML to stdout.
 *  - Exit 0 on success, non-zero on failure.
 *  - Write an error message to stderr on failure.
 *
 * Compatible renderers: a Puppeteer/Playwright Node.js script, a
 * chromium-headless wrapper, etc.
 *
 * Example renderer_cmd:
 *   "node /opt/themis/scripts/renderer.js"
 */
class SubprocessJSRenderer : public IScraperJSRenderer {
public:
    explicit SubprocessJSRenderer(std::string renderer_cmd);
    ~SubprocessJSRenderer() override = default;

    SubprocessJSRenderer(const SubprocessJSRenderer&) = delete;
    SubprocessJSRenderer& operator=(const SubprocessJSRenderer&) = delete;

    JsRenderResult render(const JsRenderRequest& req) override;

    /// Returns true when renderer_cmd is non-empty and the first token of the
    /// command resolves to an executable on PATH.
    bool isAvailable() const override;

private:
    std::string renderer_cmd_;

    /// Build the shell command string from the request.
    std::string buildCommand(const JsRenderRequest& req) const;
};

// ============================================================================
// In-memory mock (tests)
// ============================================================================

/**
 * @brief Test double that returns pre-injected render results.
 */
class InMemoryJSRenderer : public IScraperJSRenderer {
public:
    InMemoryJSRenderer() = default;

    void injectResult(JsRenderResult result) {
        injected_.push_back(std::move(result));
    }
    void clearInjections() {
        injected_.clear();
        call_count_ = 0;
    }
    int callCount() const { return call_count_; }
    bool isAvailable() const override { return true; }

    JsRenderResult render(const JsRenderRequest& /*req*/) override {
        ++call_count_;
        if (idx_ < static_cast<int>(injected_.size())) {
            return injected_[idx_++];
        }
        JsRenderResult r;
        r.success = false;
        r.error   = "InMemoryJSRenderer: no more injected results";
        return r;
    }

private:
    std::vector<JsRenderResult> injected_;
    mutable int call_count_ = 0;
    mutable int idx_        = 0;
};

} // namespace scraper
} // namespace themis

