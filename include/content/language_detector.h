/**
 * @file language_detector.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <cstdint>

namespace themis {
namespace content {

struct DetectedLanguage {
    std::string code;

    std::string name;

    float confidence = 0.0f;

    uint32_t indicator_hits = 0;
};

class LanguageDetector {
public:
    LanguageDetector() = default;

    /**
     * @brief Detect.
     * @param[in] text Input parameter.
     * @return Return value.
     */
    DetectedLanguage detect(std::string_view text) const;

    /**
     * @brief Detect Code.
     * @param[in] text Input parameter.
     * @return Return value.
     */
    std::string detectCode(std::string_view text) const;

    /**
     * @brief Routing Hint.
     * @param[in] language_code Input parameter.
     * @return Return value.
     */
    static std::string routingHint(const std::string& language_code);
};

} // namespace content
} // namespace themis
