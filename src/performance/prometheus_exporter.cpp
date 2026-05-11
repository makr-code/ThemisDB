/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            prometheus_exporter.cpp                            ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:49:56                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     127                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
        std::ostringstream oss;
        
        // Write HELP and TYPE for each metric
        oss << "# HELP themis_hnsw_search_cycles HNSW vector search cycles\n";
        oss << "# TYPE themis_hnsw_search_cycles gauge\n";
        
        oss << "# HELP themis_pointer_passing_cycles Pointer passing overhead cycles\n";
        oss << "# TYPE themis_pointer_passing_cycles gauge\n";
        
        oss << "# HELP themis_llm_inference_cycles LLM inference cycles\n";
        oss << "# TYPE themis_llm_inference_cycles gauge\n";
        
        oss << "# HELP themis_cache_miss_cycles Cache miss cycles\n";
        oss << "# TYPE themis_cache_miss_cycles gauge\n";
        
        oss << "# HELP themis_pcie_transfer_cycles PCIe transfer cycles\n";
        oss << "# TYPE themis_pcie_transfer_cycles gauge\n";
        
        oss << "# HELP themis_cpu_efficiency_ratio CPU efficiency ratio\n";
        oss << "# TYPE themis_cpu_efficiency_ratio gauge\n";
        
        oss << "# HELP themis_total_operation_cycles Total operation cycles\n";
        oss << "# TYPE themis_total_operation_cycles gauge\n";
        
        // Aggregate metrics by operation
        std::map<std::string, std::vector<const OperationCycleMetrics*>> aggregated;
        for (const auto& entry : metrics_list) {
            aggregated[entry.operation_name].push_back(&entry.metrics);
        }
        
        // Export aggregated values
        for (const auto& [operation, metrics_vec] : aggregated) {
            if (metrics_vec.empty()) continue;
            
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
            oss << "themis_pointer_passing_cycles{operation=\"" << operation << "\"} " << avg_pointer << "\n";
            oss << "themis_llm_inference_cycles{operation=\"" << operation << "\"} " << avg_llm << "\n";
            oss << "themis_cache_miss_cycles{operation=\"" << operation << "\"} " << avg_cache << "\n";
            oss << "themis_pcie_transfer_cycles{operation=\"" << operation << "\",direction=\"h2d\"} " << avg_pcie_h2d << "\n";
            oss << "themis_pcie_transfer_cycles{operation=\"" << operation << "\",direction=\"d2h\"} " << avg_pcie_d2h << "\n";
            oss << "themis_cpu_efficiency_ratio{operation=\"" << operation << "\"} " << std::fixed << std::setprecision(4) << avg_cpu_eff << "\n";
            oss << "themis_total_operation_cycles{operation=\"" << operation << "\"} " << avg_total << "\n";
        }
        
        return oss.str();
    }
};

  std::string exportPrometheusMetrics(const std::vector<MetricsEntry>& metrics_list) {
    return PrometheusExporter::exportMetrics(metrics_list);
  }

} // namespace performance
} // namespace themis
