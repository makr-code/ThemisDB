/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            classification_api_handler.h                       ║
  Version:         0.0.4                                              ║
  Last Modified:   2026-02-21 08:34:56                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     63                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 20c4e9c84  2025-11-02  feat: Complete feature set - Auth, Governance, Compliance... ║
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
