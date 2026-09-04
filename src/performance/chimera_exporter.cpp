/**
 * @file chimera_exporter.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=4, H=0, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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
        
        try {
            // Start JSON
            oss << "{\n";
            if (!oss.good()) {
              return "";
            }
            
            // Timestamp
            auto now = std::time(nullptr);
            oss << "  \"timestamp\": " << now << ",\n";
            if (!oss.good()) {
              return "";
            }
            
            auto* gm_time = std::gmtime(&now);
            if (!gm_time) {
              return "";
            }
            oss << "  \"timestamp_iso\": \"" << std::put_time(gm_time, "%Y-%m-%dT%H:%M:%SZ") << "\",\n";
            if (!oss.good()) {
              return "";
            }
            
            // System info
            oss << "  \"system\": {\n";
            if (!oss.good()) {
              return "";
            }
            oss << "    \"cpu_model\": \"" << HardwareCycleCounter::cpu_model() << "\",\n";
            if (!oss.good()) {
              return "";
            }
            oss << "    \"cpu_frequency_hz\": " << HardwareCycleCounter::cpu_frequency_hz() << ",\n";
            if (!oss.good()) {
              return "";
            }
            oss << "    \"architecture\": \"";
#if defined(__x86_64__) || defined(_M_X64)
            oss << "x86_64";
#elif defined(__aarch64__) || defined(_M_ARM64)
            oss << "arm64";
#else
            oss << "unknown";
#endif
            oss << "\"\n";
            if (!oss.good()) {
              return "";
            }
            oss << "  },\n";
            if (!oss.good()) {
              return "";
            }
            
            // Metrics by operation
            oss << "  \"operations\": [\n";
            if (!oss.good()) {
              return "";
            }
            
            // Aggregate by operation name
            std::map<std::string, std::vector<const OperationCycleMetrics*>> aggregated;
            for (const auto& entry : metrics_list) {
                aggregated[entry.operation_name].push_back(&entry.metrics);
            }
            
            bool first_op = true;
            for (const auto& [operation, metrics_vec] : aggregated) {
                if (!first_op) {
                    oss << ",\n";
                    if (!oss.good()) {
                      return "";
                    }
                }
                first_op = false;
                
                if (metrics_vec.empty()) {
                  continue;
                }
                
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
                if (count == 0) {
                  continue;
                }
                
                avg_hnsw /= count;
                avg_pointer /= count;
                avg_llm /= count;
                avg_total /= count;
                
                oss << "    {\n";
                if (!oss.good()) {
                  return "";
                }
                oss << "      \"name\": \"" << operation << "\",\n";
                if (!oss.good()) {
                  return "";
                }
                oss << "      \"count\": " << count << ",\n";
                if (!oss.good()) {
                  return "";
                }
                oss << "      \"cycles\": {\n";
                if (!oss.good()) {
                  return "";
                }
                oss << "        \"hnsw_search\": " << avg_hnsw << ",\n";
                if (!oss.good()) {
                  return "";
                }
                oss << "        \"pointer_passing\": " << avg_pointer << ",\n";
                if (!oss.good()) {
                  return "";
                }
                oss << "        \"llm_inference\": " << avg_llm << ",\n";
                if (!oss.good()) {
                  return "";
                }
                oss << "        \"total\": " << avg_total << "\n";
                if (!oss.good()) {
                  return "";
                }
                oss << "      },\n";
                if (!oss.good()) {
                  return "";
                }
                
                // Expected values and deviations
                oss << "      \"expected\": {\n";
                if (!oss.good()) {
                  return "";
                }
                oss << "        \"pointer_passing\": " << ExpectedCycles::POINTER_PASSING << "\n";
                if (!oss.good()) {
                  return "";
                }
                oss << "      },\n";
                if (!oss.good()) {
                  return "";
                }
                oss << "      \"deviation_percent\": {\n";
                if (!oss.good()) {
                  return "";
                }
                oss << "        \"pointer_passing\": " << std::fixed << std::setprecision(2) 
                    << ExpectedCycles::deviation_percent(avg_pointer, ExpectedCycles::POINTER_PASSING) << "\n";
                if (!oss.good()) {
                  return "";
                }
                oss << "      },\n";
                if (!oss.good()) {
                  return "";
                }
                
                // Breakdown percentages
                if (avg_total > 0) {
                    oss << "      \"breakdown_percent\": {\n";
                    if (!oss.good()) {
                      return "";
                    }
                    oss << "        \"hnsw_search\": " << std::fixed << std::setprecision(2) 
                        << ((double)avg_hnsw / avg_total * 100.0) << ",\n";
                    if (!oss.good()) {
                      return "";
                    }
                    oss << "        \"pointer_passing\": " << std::fixed << std::setprecision(6) 
                        << ((double)avg_pointer / avg_total * 100.0) << ",\n";
                    if (!oss.good()) {
                      return "";
                    }
                    oss << "        \"llm_inference\": " << std::fixed << std::setprecision(2) 
                        << ((double)avg_llm / avg_total * 100.0) << "\n";
                    if (!oss.good()) {
                      return "";
                    }
                    oss << "      }\n";
                    if (!oss.good()) {
                      return "";
                    }
                } else {
                    oss << "      \"breakdown_percent\": {}\n";
                    if (!oss.good()) {
                      return "";
                    }
                }
                
                oss << "    }";
                if (!oss.good()) {
                  return "";
                }
            }
            
            oss << "\n  ]\n";
            if (!oss.good()) {
              return "";
            }
            oss << "}\n";
            if (!oss.good()) {
              return "";
            }
            
            return oss.str();
        } catch (const std::exception&) {
            return "";
        } catch (...) {
            return "";
        }
    }
};

std::string exportChimeraMetrics(const std::vector<MetricsEntry>& metrics_list) {
    return CHIMERAExporter::exportMetrics(metrics_list);
}

} // namespace performance
} // namespace themis
