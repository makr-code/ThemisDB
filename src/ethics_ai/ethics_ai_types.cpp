/**
 * @file ethics_ai_types.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "ethics_ai/ethics_ai_types.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace themis {
namespace plugins {
namespace ethics {

// ========== Enum Conversion Functions ==========

const char *argumentTypeToString(ArgumentType type) {
    switch (type) {
        case ArgumentType::PRO:
            return "pro";
        case ArgumentType::CONTRA:
            return "contra";
        case ArgumentType::REBUTTAL:
            return "rebuttal";
        case ArgumentType::SYNTHESIS:
            return "synthesis";
        case ArgumentType::QUESTION:
            return "question";
        case ArgumentType::CLARIFICATION:
            return "clarification";
        default:
            return "unknown";
    }
}

ArgumentType stringToArgumentType(const std::string &str) {
    std::string lower = str;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    if (lower == "pro") {
        return ArgumentType::PRO;
    }
    if (lower == "contra") {
        return ArgumentType::CONTRA;
    }
    if (lower == "rebuttal") {
        return ArgumentType::REBUTTAL;
    }
    if (lower == "synthesis") {
        return ArgumentType::SYNTHESIS;
    }
    if (lower == "question") {
        return ArgumentType::QUESTION;
    }
    if (lower == "clarification") {
        return ArgumentType::CLARIFICATION;
    }

    throw std::invalid_argument("Invalid argument type: " + str);
}

const char *argumentStrengthToString(ArgumentStrength strength) {
    switch (strength) {
        case ArgumentStrength::WEAK:
            return "weak";
        case ArgumentStrength::MODERATE:
            return "moderate";
        case ArgumentStrength::STRONG:
            return "strong";
        case ArgumentStrength::DECISIVE:
            return "decisive";
        default:
            return "unknown";
    }
}

ArgumentStrength stringToArgumentStrength(const std::string &str) {
    std::string lower = str;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    if (lower == "weak") {
        return ArgumentStrength::WEAK;
    }
    if (lower == "moderate") {
        return ArgumentStrength::MODERATE;
    }
    if (lower == "strong") {
        return ArgumentStrength::STRONG;
    }
    if (lower == "decisive") {
        return ArgumentStrength::DECISIVE;
    }

    throw std::invalid_argument("Invalid argument strength: " + str);
}

} // namespace ethics
} // namespace plugins
} // namespace themis
