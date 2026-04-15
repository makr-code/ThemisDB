/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ethics_ai_types.cpp                                ║
  Version:         0.0.9                                              ║
  Last Modified:   2026-04-15 04:16:29                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     86                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 9ab72c5089  2026-03-12  refactor: flatten plugin hierarchy to src/<name>/ and inc... ║
    • acdb250dbf  2026-03-12  feat: migrate plugins to src/include with CMake switches ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
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
