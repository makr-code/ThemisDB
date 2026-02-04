#pragma once

#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <memory>
#include "utils/expected.h"

namespace themis {
namespace observability {

/**
 * Health status levels for system components
 */
enum class HealthStatus {
    HEALTHY,      // Component is fully operational
    DEGRADED,     // Component is operational but performance is reduced
    UNHEALTHY,    // Component is not operational
    UNKNOWN       // Component status cannot be determined
};

/**
 * Individual health check result
 */
struct HealthCheckResult {
    std::string component_name;        // Name of the checked component
    HealthStatus status;                // Current health status
    std::string message;                // Human-readable status message
    std::map<std::string, std::string> details;  // Additional details
    std::chrono::system_clock::time_point timestamp;  // Check timestamp
    double response_time_ms;            // Time taken for the check
    
    HealthCheckResult() 
        : status(HealthStatus::UNKNOWN)
        , timestamp(std::chrono::system_clock::now())
        , response_time_ms(0.0) {}
};

/**
 * Aggregated system health report
 */
struct SystemHealthReport {
    HealthStatus overall_status;        // Overall system health
    std::vector<HealthCheckResult> component_checks;  // Individual checks
    std::chrono::system_clock::time_point report_time;
    int healthy_count;
    int degraded_count;
    int unhealthy_count;
    int unknown_count;
    
    SystemHealthReport()
        : overall_status(HealthStatus::UNKNOWN)
        , report_time(std::chrono::system_clock::now())
        , healthy_count(0)
        , degraded_count(0)
        , unhealthy_count(0)
        , unknown_count(0) {}
};

/**
 * HealthCheck interface for system observability
 * 
 * Provides standardized health checking for all ThemisDB components:
 * - Database storage engine
 * - Network connectivity
 * - Shard replication
 * - LLM engine (if enabled)
 * - Memory and disk resources
 * 
 * Designed for integration with:
 * - Kubernetes liveness/readiness probes
 * - Prometheus/Grafana alerting
 * - External monitoring systems
 * 
 * GAP-008: Base structure for observability automation
 */
class HealthCheck {
public:
    HealthCheck() = default;
    virtual ~HealthCheck() = default;
    
    /**
     * Perform comprehensive system health check
     * @return SystemHealthReport with all component statuses
     */
    virtual SystemHealthReport checkSystemHealth() = 0;
    
    /**
     * Check specific component health
     * @param component_name: Name of component to check
     * @return HealthCheckResult for the specific component
     */
    virtual HealthCheckResult checkComponent(const std::string& component_name) = 0;
    
    /**
     * Get health check endpoint URL
     * @return URL for HTTP health check endpoint
     */
    virtual std::string getHealthEndpoint() const = 0;
    
    /**
     * Check if system is ready to accept traffic (Kubernetes readiness probe)
     * @return true if ready, false otherwise
     */
    virtual bool isReady() const = 0;
    
    /**
     * Check if system is alive (Kubernetes liveness probe)
     * @return true if alive, false otherwise
     */
    virtual bool isAlive() const = 0;
};

/**
 * Basic health check implementation for ThemisDB
 */
class ThemisHealthCheck : public HealthCheck {
public:
    ThemisHealthCheck();
    ~ThemisHealthCheck() override = default;
    
    SystemHealthReport checkSystemHealth() override;
    HealthCheckResult checkComponent(const std::string& component_name) override;
    std::string getHealthEndpoint() const override;
    bool isReady() const override;
    bool isAlive() const override;
    
private:
    // Individual component checks
    HealthCheckResult checkDatabase();
    HealthCheckResult checkNetwork();
    HealthCheckResult checkStorage();
    HealthCheckResult checkMemory();
    HealthCheckResult checkReplication();
    HealthCheckResult checkLLM();
    
    // Helper: Convert status to string
    static std::string statusToString(HealthStatus status);
    
    // Helper: Determine overall status from components
    static HealthStatus determineOverallStatus(const std::vector<HealthCheckResult>& checks);
    
    // Configuration
    std::string health_endpoint_;
    bool ready_;
    bool alive_;
};

} // namespace observability
} // namespace themis
