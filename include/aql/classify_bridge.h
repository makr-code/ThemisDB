/**
 * @file classify_bridge.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace themis {
namespace aql {

struct ClassifyResult {
    std::string category = {};

    double confidence = 0.0;

    std::unordered_map<std::string, double> scores;
};

class IClassifyFn {
public:
    /**
     * @brief IClassify Fn.
     * @return Return value.
     */
    virtual ~IClassifyFn() = default;

    /**
     * @brief Classify the semantic intent of a query.
     * @param[in] text Input parameter.
     * @param[in] categories Input parameter.
     * @return Return value.
     */
    virtual ClassifyResult classify(const std::string& text,
                                    const std::vector<std::string>& categories) const = 0;
};

class NullClassifyFn final : public IClassifyFn {
public:
    ClassifyResult classify(const std::string& /*text*/,
                            const std::vector<std::string>& /*categories*/) const override {
        return ClassifyResult{};   // category = "", confidence = 0.0
    }
};

class AQLFunctionClassifyBridge final : public IClassifyFn {
public:
    ClassifyResult classify(const std::string& text,
                            const std::vector<std::string>& categories) const override;
};

/**
 * @brief Register Classify Bridge.
 */
void registerClassifyBridge();

} // namespace aql
} // namespace themis
