/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ethics_ai_types.cpp                                ║
  Version:         0.0.22                                             ║
  Last Modified:   2026-02-21 19:28:56                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     88                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 31e8b8df0  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "plugins/ethics_ai/ethics_ai_types.h"
#include <stdexcept>
#include <algorithm>
#include <cctype>

namespace themis {
namespace plugins {
namespace ethics {

// ========== Enum Conversion Functions ==========

const char* argumentTypeToString(ArgumentType type) {
    switch (type) {
        case ArgumentType::PRO: return "pro";
        case ArgumentType::CONTRA: return "contra";
        case ArgumentType::REBUTTAL: return "rebuttal";
        case ArgumentType::SYNTHESIS: return "synthesis";
        case ArgumentType::QUESTION: return "question";
        case ArgumentType::CLARIFICATION: return "clarification";
        default: return "unknown";
    }
}

ArgumentType stringToArgumentType(const std::string& str) {
    std::string lower = str;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    
    if (lower == "pro") return ArgumentType::PRO;
    if (lower == "contra") return ArgumentType::CONTRA;
    if (lower == "rebuttal") return ArgumentType::REBUTTAL;
    if (lower == "synthesis") return ArgumentType::SYNTHESIS;
    if (lower == "question") return ArgumentType::QUESTION;
    if (lower == "clarification") return ArgumentType::CLARIFICATION;
    
    throw std::invalid_argument("Invalid argument type: " + str);
}

const char* argumentStrengthToString(ArgumentStrength strength) {
    switch (strength) {
        case ArgumentStrength::WEAK: return "weak";
        case ArgumentStrength::MODERATE: return "moderate";
        case ArgumentStrength::STRONG: return "strong";
        case ArgumentStrength::DECISIVE: return "decisive";
        default: return "unknown";
    }
}

ArgumentStrength stringToArgumentStrength(const std::string& str) {
    std::string lower = str;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    
    if (lower == "weak") return ArgumentStrength::WEAK;
    if (lower == "moderate") return ArgumentStrength::MODERATE;
    if (lower == "strong") return ArgumentStrength::STRONG;
    if (lower == "decisive") return ArgumentStrength::DECISIVE;
    
    throw std::invalid_argument("Invalid argument strength: " + str);
}

} // namespace ethics
} // namespace plugins
} // namespace themis
