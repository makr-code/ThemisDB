/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            llm_security_utils.cpp                             ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-03-02 03:58:33                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     33                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • d0fa9e609  2026-02-28  feat(llm): implement prompt injection mitigation and secu... ║
    • 522e9ae57  2026-02-24  feat(core): implement OTel tracer adapter flush() via Tra... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
