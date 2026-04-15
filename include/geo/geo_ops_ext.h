/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            geo_ops_ext.h                                      ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:44:52                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     40                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
