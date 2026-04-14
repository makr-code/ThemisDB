/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            scraper_js_renderer.h                              ║
  Version:         0.0.4                                              ║
  Last Modified:   2026-04-14 06:58:34                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     156                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • c2cc8e90ab  2026-04-02  feat(plugins/scraper): add agentic scraper plugin with go... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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

struct JsRenderRequest {
    std::string url;
    int         timeout_ms      = 10000;
    /// CSS selector to wait for before capturing HTML (empty = DOMContentLoaded)
    std::string wait_selector;
    std::map<std::string, std::string> headers;
    /// Extra arguments forwarded verbatim to the renderer command
    std::vector<std::string> extra_args;
};

struct JsRenderResult {
    bool        success     = false;
    std::string html;
    std::string error;
    int         status_code = 0;
    long        elapsed_ms  = 0;
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
    virtual JsRenderResult render(const JsRenderRequest& req) = 0;
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
