#include "server/sharding_metrics_handler.h"
#include "sharding/prometheus_metrics.h"
#include "sharding/slo_monitor.h"
#include <sstream>
#include <iomanip>

namespace themis {
namespace server {

ShardingMetricsHandler::ShardingMetricsHandler(
    std::shared_ptr<sharding::PrometheusMetrics> metrics,
    std::shared_ptr<sharding::SLOMonitor> slo_monitor)
    : metrics_(metrics)
    , slo_monitor_(slo_monitor) {
}

std::string ShardingMetricsHandler::getMetrics() const {
    if (!metrics_) {
        return "";
    }
    
    // Get metrics with annotations (HELP and TYPE)
    return metrics_->getMetricsWithAnnotations();
}

std::string ShardingMetricsHandler::getMetricsPlain() const {
    if (!metrics_) {
        return "";
    }
    
    // Get plain metrics without annotations
    return metrics_->getMetrics();
}

std::string ShardingMetricsHandler::getSLOStatus() const {
    if (!slo_monitor_) {
        return R"({"error": "SLO monitoring not configured"})";
    }
    
    // Get SLO status in JSON format
    return slo_monitor_->generateSLOReportJSON();
}

std::string ShardingMetricsHandler::getSLOMetrics() const {
    if (!slo_monitor_) {
        return "";
    }
    
    std::ostringstream oss;
    
    // Add SLO metrics in Prometheus format
    oss << "# HELP themisdb_slo_availability Current availability percentage\n";
    oss << "# TYPE themisdb_slo_availability gauge\n";
    
    oss << "# HELP themisdb_slo_error_budget Remaining error budget (0-1)\n";
    oss << "# TYPE themisdb_slo_error_budget gauge\n";
    
    oss << "# HELP themisdb_slo_compliance SLO compliance status (1=met, 0=violated)\n";
    oss << "# TYPE themisdb_slo_compliance gauge\n";
    
    // Get compliance data
    auto compliance = slo_monitor_->getSLOCompliance();
    
    // Export availability
    if (compliance.find("availability") != compliance.end()) {
        oss << "themisdb_slo_availability " 
            << std::fixed << std::setprecision(6) 
            << compliance["availability"] << "\n";
    }
    
    // Export error budget
    if (compliance.find("error_budget") != compliance.end()) {
        oss << "themisdb_slo_error_budget " 
            << std::fixed << std::setprecision(6) 
            << compliance["error_budget"] << "\n";
    }
    
    // Export overall compliance (1 if all SLOs met, 0 otherwise)
    double global_error_budget = slo_monitor_->getGlobalErrorBudget();
    oss << "themisdb_slo_compliance " 
        << (global_error_budget > 0.1 ? 1.0 : 0.0) << "\n";
    
    return oss.str();
}

} // namespace server
} // namespace themis
