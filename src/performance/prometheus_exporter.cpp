/**
 * @file prometheus_exporter.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=9, H=0, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "performance/cycle_metrics.h"
#include "performance/lockfree_metrics_buffer.h"
#include "performance/runtime_config.h"
#include <sstream>
#include <iomanip>
#include <cmath>
#include <vector>
#include <map>

namespace themis {
namespace performance {

/**
 * @brief Prometheus text format exporter
 */
class PrometheusExporter {
public:
    /**
     * @brief Export metrics in Prometheus text format
     * @param metrics_list List of operation metrics
     * @return Prometheus formatted string
     */
    static std::string exportMetrics(const std::vector<MetricsEntry>& metrics_list) {
        std::ostringstream oss = {};
        try {
            // Write HELP and TYPE for each metric
            oss << "# HELP themis_hnsw_search_cycles HNSW vector search cycles\n";
            if (!oss.good()) {
              return "";
            }
            oss << "# TYPE themis_hnsw_search_cycles gauge\n";
            if (!oss.good()) {
              return "";
            }
            
            oss << "# HELP themis_pointer_passing_cycles Pointer passing overhead cycles\n";
            if (!oss.good()) {
              return "";
            }
            oss << "# TYPE themis_pointer_passing_cycles gauge\n";
            if (!oss.good()) {
              return "";
            }
            
            oss << "# HELP themis_llm_inference_cycles LLM inference cycles\n";
            if (!oss.good()) {
              return "";
            }
            oss << "# TYPE themis_llm_inference_cycles gauge\n";
            if (!oss.good()) {
              return "";
            }
            
            oss << "# HELP themis_cache_miss_cycles Cache miss cycles\n";
            if (!oss.good()) {
              return "";
            }
            oss << "# TYPE themis_cache_miss_cycles gauge\n";
            if (!oss.good()) {
              return "";
            }
            
            oss << "# HELP themis_pcie_transfer_cycles PCIe transfer cycles\n";
            if (!oss.good()) {
              return "";
            }
            oss << "# TYPE themis_pcie_transfer_cycles gauge\n";
            if (!oss.good()) {
              return "";
            }
            
            oss << "# HELP themis_cpu_efficiency_ratio CPU efficiency ratio\n";
            if (!oss.good()) {
              return "";
            }
            oss << "# TYPE themis_cpu_efficiency_ratio gauge\n";
            if (!oss.good()) {
              return "";
            }
            
            oss << "# HELP themis_total_operation_cycles Total operation cycles\n";
            if (!oss.good()) {
              return "";
            }
            oss << "# TYPE themis_total_operation_cycles gauge\n";
            if (!oss.good()) {
              return "";
            }
            
            // Aggregate metrics by operation
            std::map<std::string, std::vector<const OperationCycleMetrics*>> aggregated;
            for (const auto& entry : metrics_list) {
                aggregated[entry.operation_name].push_back(&entry.metrics);
            }
            
            // Export aggregated values
            for (const auto& [operation, metrics_vec] : aggregated) {
                if (metrics_vec.empty()) {
                  continue;
                }
                
                // Calculate averages
                uint64_t avg_hnsw = 0;
                uint64_t avg_pointer = 0;
                uint64_t avg_llm = 0;
                uint64_t avg_cache = 0;
                uint64_t avg_pcie_h2d = 0;
                uint64_t avg_pcie_d2h = 0;
                uint64_t avg_total = 0;
                double avg_cpu_eff = 0.0;
                
                for (const auto* m : metrics_vec) {
                    avg_hnsw += m->hnsw_search_cycles;
                    avg_pointer += m->pointer_passing_cycles;
                    avg_llm += m->llm_inference_cycles;
                    avg_cache += m->cache_miss_cycles;
                    avg_pcie_h2d += m->pcie_host_to_device_cycles;
                    avg_pcie_d2h += m->pcie_device_to_host_cycles;
                    avg_total += m->total_cycles;
                    avg_cpu_eff += m->cpu_efficiency_ratio;
                }
                
                size_t count = metrics_vec.size();
                if (count == 0) {
                  continue;
                }
                
                avg_hnsw /= count;
                avg_pointer /= count;
                avg_llm /= count;
                avg_cache /= count;
                avg_pcie_h2d /= count;
                avg_pcie_d2h /= count;
                avg_total /= count;
                avg_cpu_eff /= count;
                
                // Export metrics with labels
                oss << "themis_hnsw_search_cycles{operation=\"" << operation << "\"} " << avg_hnsw << "\n";
                if (!oss.good()) {
                  return "";
                }
                oss << "themis_pointer_passing_cycles{operation=\"" << operation << "\"} " << avg_pointer << "\n";
                if (!oss.good()) {
                  return "";
                }
                oss << "themis_llm_inference_cycles{operation=\"" << operation << "\"} " << avg_llm << "\n";
                if (!oss.good()) {
                  return "";
                }
                oss << "themis_cache_miss_cycles{operation=\"" << operation << "\"} " << avg_cache << "\n";
                if (!oss.good()) {
                  return "";
                }
                oss << "themis_pcie_transfer_cycles{operation=\"" << operation << "\",direction=\"h2d\"} " << avg_pcie_h2d << "\n";
                if (!oss.good()) {
                  return "";
                }
                oss << "themis_pcie_transfer_cycles{operation=\"" << operation << "\",direction=\"d2h\"} " << avg_pcie_d2h << "\n";
                if (!oss.good()) {
                  return "";
                }
                oss << "themis_cpu_efficiency_ratio{operation=\"" << operation << "\"} " << std::fixed << std::setprecision(4) << avg_cpu_eff << "\n";
                if (!oss.good()) {
                  return "";
                }
                oss << "themis_total_operation_cycles{operation=\"" << operation << "\"} " << avg_total << "\n";
                if (!oss.good()) {
                  return "";
                }
            }
            
            return oss.str();
        } catch (const std::exception&) {
            return "";
        } catch (...) {
            return "";
        }
    }
};

  std::string exportPrometheusMetrics(const std::vector<MetricsEntry>& metrics_list) {
    return PrometheusExporter::exportMetrics(metrics_list);
  }

} // namespace performance
} // namespace themis
