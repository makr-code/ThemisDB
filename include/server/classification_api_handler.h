/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            classification_api_handler.h                       ║
  Version:         0.0.21                                             ║
  Last Modified:   2026-02-21 19:20:02                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     70                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <memory>
#include <string>
#include <nlohmann/json.hpp>
#include "utils/pii_detector.h"

namespace themis { namespace server {

/**
 * @brief Classification Admin API Handler
 * 
 * Provides REST endpoints for data classification:
 * - GET /api/classification/rules - List classification rules
 * - POST /api/classification/test - Test classification on sample data
 */
class ClassificationApiHandler {
public:
    /**
     * @brief Initialize with PII Detector for classification
     * @param pii_detector Shared pointer to PIIDetector instance
     */
    explicit ClassificationApiHandler(std::shared_ptr<themis::utils::PIIDetector> pii_detector);
    
    ClassificationApiHandler() = default;

    /**
     * @brief List all classification rules
     * @return JSON response: { "items": [...], "total": N }
     */
    nlohmann::json listRules();

    /**
     * @brief Test classification on sample data
     * @param body JSON with { "text": "...", "metadata": {...} }
     * @return JSON response: { "classification": "...", "confidence": 0.95, "detected_entities": [...] }
     */
    nlohmann::json testClassification(const nlohmann::json& body);
    
private:
    std::shared_ptr<themis::utils::PIIDetector> pii_detector_;
};

}} // namespace themis::server
