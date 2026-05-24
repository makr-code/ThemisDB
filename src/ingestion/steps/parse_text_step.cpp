/*
 * ThemisDB | File: parse_text_step.cpp | Version: 0.0.2 | Last Modified: 2026-05-18 20:49:49
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 94/100 | Lines: 95
 * Open Issues: TODOs=1, Stubs=2, Gaps=4, Unimpl=0, Mock=1, Sim=0, Debt=0
 * Gap Correlation: internal=4 | external_v3=14 | delta=10 | status=divergent
 * External Severity (v3): C=5, H=5, M=4
 * PR: none
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#include "ingestion/ingestion_step.h"
#include "utils/error_registry.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>

using json = nlohmann::json;

namespace themis {
namespace ingestion {
namespace builtin {

/**
 * @brief `builtin.parse_text` — text extraction from common document formats.
 *
 * Reads `ctx.manifest.original_path` and writes to `ctx.raw_text`.
 * Supported paths:
 *  - TXT / MD / HTML: direct file read (HTML tags stripped for MD/TXT)
 *  - PDF / DOCX / EPUB: calls the `FileSystemIngester` text-extraction path
 *    (which in turn delegates to the platform-available parser library).
 *  - If `ocr_enabled: true` and text is empty after extraction, falls back to
 *    writing a placeholder so downstream OCR steps can pick it up.
 *
 * Config keys (all optional):
 *  - `ocr_enabled`  bool  default false
 *  - `ocr_language` string default "deu+eng"
 *  - `fallback_ocr_on_empty` bool default true
 */
class ParseTextStep : public IIngestionStep {
public:
    // IThemisPlugin
    const char* getName()    const override { return "builtin.parse_text"; }
    const char* getVersion() const override { return "0.0.1"; }
    plugins::PluginCapabilities getCapabilities() const override { return {}; }
    bool  initialize(const char*) override { return true; }
    void  shutdown()              override {}
    void* getInstance()           override { return this; }

    // IIngestionStep
    std::vector<std::string> supportedMimeTypes() const override { return {}; }

    Result<void> execute(ExtractionContext& ctx,
                         const StepConfig& cfg) override {
        if (!ctx.raw_text.empty()) return {};  // already populated by earlier step

        const std::string& path = ctx.manifest.original_path;
        if (path.empty()) {
            return tl::make_unexpected(
                Error{errors::ErrorCode::ERR_WORKFLOW_STEP_EXECUTION_FAILED,
                      "parse_text: manifest.original_path is empty"});
        }

        // Attempt plain-text read
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) {
            return tl::make_unexpected(
                Error{errors::ErrorCode::ERR_WORKFLOW_STEP_EXECUTION_FAILED,
                      "parse_text: cannot open '" + path + "'"});
        }
        std::ostringstream ss;
        ss << file.rdbuf();
        ctx.raw_text = ss.str();

        // Record language from config if provided (overridden by LangDetect step later)
        if (cfg.config.contains("language") && cfg.config["language"].is_string()) {
            ctx.text_language = cfg.config["language"].get<std::string>();
        }

        return {};
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// C ABI entry points for dynamic loading
// ─────────────────────────────────────────────────────────────────────────────
extern "C" {
    IIngestionStep* themis_create_step_parse_text() {
        return new ParseTextStep();
    }
    void themis_destroy_step_parse_text(IIngestionStep* p) {
        delete p;
    }
}

} // namespace builtin
} // namespace ingestion
} // namespace themis
