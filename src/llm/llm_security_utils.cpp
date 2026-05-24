/*
 * ThemisDB | File: llm_security_utils.cpp | Version: 0.0.15 | Last Modified: 2026-04-15 18:58:58
 * Author: ThemisDB Version Bot | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 28
 * Open Issues: TODOs=1, Stubs=1, Gaps=3, Unimpl=0, Mock=1, Sim=0, Debt=0
 * Gap Correlation: internal=3 | external_v3=4 | delta=1 | status=near
 * External Severity (v3): C=0, H=3, M=1
 * PR: #3629 [MODULE] llm â€“ build-system audit: register 16 missing sources, 2... (2026-03-12T07:39:34Z)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#include "llm/llm_security_utils.h"

namespace themis {
namespace llm {

std::string sanitizeApiKey(const std::string& api_key) {
    if (api_key.empty()) {
        return "<not set>";
    }
    constexpr size_t kVisible = 4;
    if (api_key.size() <= kVisible * 2) {
        return std::string(api_key.size(), '*');
    }
    return api_key.substr(0, kVisible) +
           "***...***" +
           api_key.substr(api_key.size() - kVisible);
}

} // namespace llm
} // namespace themis
