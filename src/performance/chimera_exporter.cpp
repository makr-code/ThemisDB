/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            chimera_exporter.cpp                               ║
  Version:         0.0.45                                             ║
  Last Modified:   2026-04-15 07:13:00                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     150                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "performance/cycle_metrics.h"
#include "performance/lockfree_metrics_buffer.h"
#include "performance/expected_cycles.h"
#include <sstream>
#include <iomanip>
#include <ctime>
#include <vector>
#include <map>

namespace themis {
namespace performance {

/**
 * @brief CHIMERA JSON format exporter
 */
class CHIMERAExporter {
public:
    /**
     * @brief Export metrics in CHIMERA JSON format
     * @param metrics_list List of operation metrics
     * @return JSON formatted string
     */
    static std::string exportMetrics(const std::vector<MetricsEntry>& metrics_list) {
        std::ostringstream oss;
        
        // Start JSON
        oss << "{\n";
        
        // Timestamp
        auto now = std::time(nullptr);
        oss << "  \"timestamp\": " << now << ",\n";
        oss << "  \"timestamp_iso\": \"" << std::put_time(std::gmtime(&now), "%Y-%m-%dT%H:%M:%SZ") << "\",\n";
        
        // System info
        oss << "  \"system\": {\n";
        oss << "    \"cpu_model\": \"" << HardwareCycleCounter::cpu_model() << "\",\n";
        oss << "    \"cpu_frequency_hz\": " << HardwareCycleCounter::cpu_frequency_hz() << ",\n";
        oss << "    \"architecture\": \"";
#if defined(__x86_64__) || defined(_M_X64)
        oss << "x86_64";
#elif defined(__aarch64__) || defined(_M_ARM64)
        oss << "arm64";
#else
        oss << "unknown";
#endif
        oss << "\"\n";
        oss << "  },\n";
        
        // Metrics by operation
        oss << "  \"operations\": [\n";
        
        // Aggregate by operation name
        std::map<std::string, std::vector<const OperationCycleMetrics*>> aggregated;
        for (const auto& entry : metrics_list) {
            aggregated[entry.operation_name].push_back(&entry.metrics);
        }
        
        bool first_op = true;
        for (const auto& [operation, metrics_vec] : aggregated) {
            if (!first_op) oss << ",\n";
            first_op = false;
            
            if (metrics_vec.empty()) continue;
            
            // Calculate statistics
            uint64_t avg_hnsw = 0;
            uint64_t avg_pointer = 0;
            uint64_t avg_llm = 0;
            uint64_t avg_total = 0;
            
            for (const auto* m : metrics_vec) {
                avg_hnsw += m->hnsw_search_cycles;
                avg_pointer += m->pointer_passing_cycles;
                avg_llm += m->llm_inference_cycles;
                avg_total += m->total_cycles;
            }
            
            size_t count = metrics_vec.size();
            avg_hnsw /= count;
            avg_pointer /= count;
            avg_llm /= count;
            avg_total /= count;
            
            oss << "    {\n";
            oss << "      \"name\": \"" << operation << "\",\n";
            oss << "      \"count\": " << count << ",\n";
            oss << "      \"cycles\": {\n";
            oss << "        \"hnsw_search\": " << avg_hnsw << ",\n";
            oss << "        \"pointer_passing\": " << avg_pointer << ",\n";
            oss << "        \"llm_inference\": " << avg_llm << ",\n";
            oss << "        \"total\": " << avg_total << "\n";
            oss << "      },\n";
            
            // Expected values and deviations
            oss << "      \"expected\": {\n";
            oss << "        \"pointer_passing\": " << ExpectedCycles::POINTER_PASSING << "\n";
            oss << "      },\n";
            oss << "      \"deviation_percent\": {\n";
            oss << "        \"pointer_passing\": " << std::fixed << std::setprecision(2) 
                << ExpectedCycles::deviation_percent(avg_pointer, ExpectedCycles::POINTER_PASSING) << "\n";
            oss << "      },\n";
            
            // Breakdown percentages
            if (avg_total > 0) {
                oss << "      \"breakdown_percent\": {\n";
                oss << "        \"hnsw_search\": " << std::fixed << std::setprecision(2) 
                    << ((double)avg_hnsw / avg_total * 100.0) << ",\n";
                oss << "        \"pointer_passing\": " << std::fixed << std::setprecision(6) 
                    << ((double)avg_pointer / avg_total * 100.0) << ",\n";
                oss << "        \"llm_inference\": " << std::fixed << std::setprecision(2) 
                    << ((double)avg_llm / avg_total * 100.0) << "\n";
                oss << "      }\n";
            } else {
                oss << "      \"breakdown_percent\": {}\n";
            }
            
            oss << "    }";
        }
        
        oss << "\n  ]\n";
        oss << "}\n";
        
        return oss.str();
    }
};

} // namespace performance
} // namespace themis
