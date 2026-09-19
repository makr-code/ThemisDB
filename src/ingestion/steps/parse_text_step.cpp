/**
 * @file parse_text_step.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.2
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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

        /**
         * @brief Attempt plain-text read
         * @param[in] path Input parameter.
         * @param[in] binary Input parameter.
         * @return Return value.
         */
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) {
            return tl::make_unexpected(
                Error{errors::ErrorCode::ERR_WORKFLOW_STEP_EXECUTION_FAILED,
                      "parse_text: cannot open '" + path + "'"});
        }
        std::ostringstream ss = {};
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
std::shared_ptr<IIngestionStep> createParseTextStep() {
    return std::make_shared<ParseTextStep>();
}

extern "C" {
    /**
     * @brief Themis create step parse text.
     * @return Pointer to the result.
     * @details Calls: ParseTextStep().
     */
    IIngestionStep* themis_create_step_parse_text() {
        return new ParseTextStep();
    }
    /**
     * @brief Themis destroy step parse text.
     * @param[in,out] p Input/output parameter.
     * @details Implements themis_destroy_step_parse_text without additional internal calls.
     */
    void themis_destroy_step_parse_text(IIngestionStep* p) {
        delete p;
    }
}

} // namespace builtin
} // namespace ingestion
} // namespace themis
