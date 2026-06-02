/*
 * ThemisDB | File: llm_security_utils.cpp | Version: 0.0.15 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 29
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=0, L=0
 * PR History (last 5): #3629 [MODULE] llm â€“ build-syst... (2026-03-12) | #3284 [llm] Implement prompt inje... (2026-03-12)
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
