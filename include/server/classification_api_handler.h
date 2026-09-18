/**
 * @file classification_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <memory>
#include <string>
#include <nlohmann/json.hpp>
#include "utils/pii_detector.h"

namespace themis { namespace server {

class ClassificationApiHandler {
public:
    /**
     * @brief Classification Api Handler.
     * @param[in] pii_detector Input parameter.
     * @return Return value.
     */
    explicit ClassificationApiHandler(std::shared_ptr<themis::utils::PIIDetector> pii_detector);
    
    ClassificationApiHandler() = default;

    /**
     * @brief List Rules.
     * @return Return value.
     */
    nlohmann::json listRules();

    /**
     * @brief Test Classification.
     * @param[in] body Input parameter.
     * @return Return value.
     */
    nlohmann::json testClassification(const nlohmann::json& body);
    
private:
    std::shared_ptr<themis::utils::PIIDetector> pii_detector_;
};

}} // namespace themis::server
