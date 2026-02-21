/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            temporal_graph.cpp                                 ║
  Version:         0.0.20                                             ║
  Last Modified:   2026-02-21 19:14:39                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     63                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "index/temporal_graph.h"
#include <sstream>
#include <iomanip>

namespace themis {

std::string TemporalStats::toString() const {
    std::ostringstream oss;
    oss << "Temporal Statistics:\n";
    oss << "  Total edges: " << edge_count << "\n";
    oss << "  Fully contained: " << fully_contained_count << "\n";
    oss << "  Bounded edges: " << bounded_edge_count << "\n";
    
    if (bounded_edge_count > 0) {
        oss << "  Average duration: " << std::fixed << std::setprecision(2) 
            << avg_duration_ms << " ms\n";
        oss << "  Total duration: " << total_duration_ms << " ms\n";
        
        if (min_duration_ms.has_value()) {
            oss << "  Min duration: " << *min_duration_ms << " ms\n";
        }
        if (max_duration_ms.has_value()) {
            oss << "  Max duration: " << *max_duration_ms << " ms\n";
        }
    }
    
    if (earliest_start.has_value()) {
        oss << "  Earliest start: " << *earliest_start << "\n";
    }
    if (latest_end.has_value()) {
        oss << "  Latest end: " << *latest_end << "\n";
    }
    
    return oss.str();
}

} // namespace themis
