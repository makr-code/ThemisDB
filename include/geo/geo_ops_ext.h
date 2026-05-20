/*
 * ThemisDB | File: geo_ops_ext.h | Version: 0.0.47 | Last Modified: 2026-05-20 17:13:04
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 26
 * Open Issues: TODOs=1, Stubs=1, Gaps=3, Unimpl=0, Mock=1, Sim=0, Debt=0
 * Gap Correlation: internal=3 | external_v3=n/a | delta=n/a | status=no_external_data
 * External Severity (v3): C=n/a, H=n/a, M=n/a
 * PR: none
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

#include <string>
#include <vector>

namespace themis {
namespace geo {

// Extension interface for providing additional ST_* operations via plugins
class IGeoOpsExtension {
public:
    virtual ~IGeoOpsExtension() = default;
    virtual const char* name() const noexcept = 0;
    virtual bool supports(const std::string& op_name) const noexcept = 0; // e.g., "ST_Buffer"
};

} // namespace geo
} // namespace themis
