/*
 * ThemisDB | File: temporal_graph.cpp | Version: 0.0.47 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 46
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=0, L=0
 * PR History (last 5): #3114 [graph] Add time-range trav... (2026-03-12)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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
