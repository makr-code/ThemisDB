# ThemisDB License Validation Integration Guide

## Overview

This guide explains how ThemisDB instances validate their licenses with the Enterprise Pricing Server.

## License Validation Endpoints

The pricing server provides the following license validation endpoints:

### Base URL
- **Production**: `https://service.themisdb.org:6734`
- **Development**: `http://localhost:8000`

### Endpoints

#### 1. Validate License (POST)
```
POST /license/validate
Content-Type: application/json

{
  "license_key": "THEMIS-ENT-A1B2C3D4-E5F6G7H8",
  "server_hostname": "prod-db-01.company.com",
  "server_version": "1.3.0",
  "server_nodes": 5
}
```

**Response (Success):**
```json
{
  "valid": true,
  "status": "active",
  "message": "License is valid and active",
  "tier": "enterprise",
  "license_key": "THEMIS-ENT-A1B2C3D4-E5F6G7H8",
  "organization": "Example Corporation",
  "limits": {
    "max_nodes": 100,
    "max_cores": -1,
    "max_storage_tb": -1
  },
  "start_date": "2025-01-01T00:00:00Z",
  "end_date": "2026-12-31T23:59:59Z",
  "days_remaining": 365
}
```

**Response (Invalid):**
```json
{
  "valid": false,
  "status": "expired",
  "message": "License expired on 2025-12-21T23:59:59Z",
  "tier": "enterprise",
  "expiry_date": "2025-12-21T23:59:59Z",
  "limits": null
}
```

#### 2. Validate License (GET)
```
GET /license/validate/{license_key}
X-Server-Hostname: prod-db-01.company.com
X-Server-Version: 1.3.0
```

#### 3. Check License Limits
```
POST /license/check-limits
Content-Type: application/json

{
  "license_key": "THEMIS-ENT-A1B2C3D4-E5F6G7H8",
  "current_nodes": 10,
  "current_cores": 256,
  "current_storage_tb": 50.5
}
```

**Response:**
```json
{
  "compliant": true,
  "limits_check": {
    "nodes": {
      "limit": 100,
      "current": 10,
      "compliant": true
    },
    "cores": {
      "limit": "unlimited",
      "current": 256,
      "compliant": true
    },
    "storage_tb": {
      "limit": "unlimited",
      "current": 50.5,
      "compliant": true
    }
  },
  "tier": "enterprise"
}
```

#### 4. Get License Info
```
GET /license/info/{license_key}
```

#### 5. Health Check
```
GET /license/health
```

---

## Integration in ThemisDB

### 1. Configuration File

Add license configuration to your ThemisDB `config.yaml`:

```yaml
license:
  # License key from pricing server
  key: "THEMIS-ENT-A1B2C3D4-E5F6G7H8"
  
  # License validation server
  validation_url: "https://service.themisdb.org:6734"
  
  # Validation settings
  validate_on_startup: true
  validate_interval_hours: 24
  
  # Server information for validation
  server_hostname: "auto"  # or specify manually
  server_version: "auto"   # uses ThemisDB version
  
  # Offline mode (for air-gapped deployments)
  offline_mode: false
  offline_grace_period_days: 30
```

### 2. C++ Implementation Example

```cpp
// include/themis/license/license_validator.hpp
#pragma once

#include <string>
#include <optional>
#include <chrono>
#include <nlohmann/json.hpp>

namespace themis::license {

struct LicenseLimits {
    int max_nodes;
    int max_cores;
    int max_storage_tb;
};

struct LicenseValidationResult {
    bool valid;
    std::string status;
    std::string message;
    std::string tier;
    std::optional<LicenseLimits> limits;
    std::optional<std::chrono::system_clock::time_point> expiry_date;
    std::optional<int> days_remaining;
};

class LicenseValidator {
public:
    explicit LicenseValidator(const std::string& validation_url);
    
    // Validate license with server
    LicenseValidationResult validate(
        const std::string& license_key,
        const std::string& hostname = "",
        const std::string& version = ""
    );
    
    // Check if within limits
    bool checkLimits(
        const std::string& license_key,
        int current_nodes,
        int current_cores,
        double current_storage_tb
    );
    
    // Get cached validation result (for offline mode)
    std::optional<LicenseValidationResult> getCachedResult() const;
    
private:
    std::string validation_url_;
    std::optional<LicenseValidationResult> cached_result_;
    std::chrono::system_clock::time_point last_validation_;
};

} // namespace themis::license
```

```cpp
// src/license/license_validator.cpp
#include "themis/license/license_validator.hpp"
#include <httplib.h>  // cpp-httplib for HTTP requests
#include <spdlog/spdlog.h>

namespace themis::license {

LicenseValidator::LicenseValidator(const std::string& validation_url)
    : validation_url_(validation_url) {}

LicenseValidationResult LicenseValidator::validate(
    const std::string& license_key,
    const std::string& hostname,
    const std::string& version
) {
    try {
        // Parse URL
        httplib::Client client(validation_url_);
        client.set_connection_timeout(10);
        
        // Prepare request body
        nlohmann::json request_body = {
            {"license_key", license_key},
            {"server_hostname", hostname},
            {"server_version", version}
        };
        
        // Make POST request
        auto res = client.Post("/license/validate",
                              request_body.dump(),
                              "application/json");
        
        if (!res) {
            spdlog::error("Failed to connect to license validation server");
            // Return cached result if available
            if (cached_result_) {
                return *cached_result_;
            }
            return {false, "connection_error", "Cannot connect to validation server", "", std::nullopt, std::nullopt, std::nullopt};
        }
        
        // Parse response
        auto response_json = nlohmann::json::parse(res->body);
        
        LicenseValidationResult result;
        result.valid = response_json["valid"];
        result.status = response_json["status"];
        result.message = response_json["message"];
        result.tier = response_json.value("tier", "");
        
        if (response_json.contains("limits") && !response_json["limits"].is_null()) {
            LicenseLimits limits;
            limits.max_nodes = response_json["limits"]["max_nodes"];
            limits.max_cores = response_json["limits"]["max_cores"];
            limits.max_storage_tb = response_json["limits"]["max_storage_tb"];
            result.limits = limits;
        }
        
        // Cache result
        cached_result_ = result;
        last_validation_ = std::chrono::system_clock::now();
        
        return result;
        
    } catch (const std::exception& e) {
        spdlog::error("License validation error: {}", e.what());
        if (cached_result_) {
            return *cached_result_;
        }
        return {false, "error", e.what(), "", std::nullopt, std::nullopt, std::nullopt};
    }
}

bool LicenseValidator::checkLimits(
    const std::string& license_key,
    int current_nodes,
    int current_cores,
    double current_storage_tb
) {
    try {
        httplib::Client client(validation_url_);
        
        nlohmann::json request_body = {
            {"license_key", license_key},
            {"current_nodes", current_nodes},
            {"current_cores", current_cores},
            {"current_storage_tb", current_storage_tb}
        };
        
        auto res = client.Post("/license/check-limits",
                              request_body.dump(),
                              "application/json");
        
        if (!res) {
            spdlog::warn("Cannot check license limits - server unavailable");
            return true; // Allow operation if server is down
        }
        
        auto response_json = nlohmann::json::parse(res->body);
        return response_json["compliant"];
        
    } catch (const std::exception& e) {
        spdlog::error("License limits check error: {}", e.what());
        return true; // Allow operation on error
    }
}

} // namespace themis::license
```

### 3. Integration in ThemisDB Server Startup

```cpp
// src/server/themis_server.cpp

#include "themis/license/license_validator.hpp"

int main(int argc, char** argv) {
    // ... initialization code ...
    
    // Load configuration
    auto config = themis::config::load("config.yaml");
    
    // Validate license if not community edition
    if (config.license.key != "COMMUNITY") {
        spdlog::info("Validating license...");
        
        themis::license::LicenseValidator validator(config.license.validation_url);
        
        auto result = validator.validate(
            config.license.key,
            config.license.server_hostname,
            THEMIS_VERSION
        );
        
        if (!result.valid) {
            spdlog::error("License validation failed: {}", result.message);
            spdlog::error("Status: {}", result.status);
            
            if (result.status == "expired") {
                spdlog::error("Your license has expired. Please renew at https://service.themisdb.org");
                return 1;
            }
            
            if (result.status == "not_found") {
                spdlog::error("Invalid license key. Please check your configuration.");
                return 1;
            }
            
            // For network errors, allow startup but warn
            if (result.status == "connection_error") {
                spdlog::warn("Could not validate license online. Starting in offline mode.");
                spdlog::warn("Please ensure license server is accessible: {}", config.license.validation_url);
            } else {
                return 1;
            }
        } else {
            spdlog::info("License validated successfully");
            spdlog::info("Edition: {}", result.tier);
            spdlog::info("Organization: {}", result.message);
            if (result.limits) {
                spdlog::info("Max Nodes: {}", result.limits->max_nodes == -1 ? "unlimited" : std::to_string(result.limits->max_nodes));
                spdlog::info("Max Cores: {}", result.limits->max_cores == -1 ? "unlimited" : std::to_string(result.limits->max_cores));
                spdlog::info("Max Storage: {} TB", result.limits->max_storage_tb == -1 ? "unlimited" : std::to_string(result.limits->max_storage_tb));
            }
            if (result.days_remaining) {
                spdlog::info("License expires in {} days", *result.days_remaining);
            }
        }
    } else {
        spdlog::info("Running in Community Edition mode");
    }
    
    // Start server...
}
```

### 4. Periodic Validation

```cpp
// Schedule periodic license validation
auto validation_task = std::make_shared<themis::tasks::PeriodicTask>(
    std::chrono::hours(config.license.validate_interval_hours),
    [validator, license_key]() {
        auto result = validator.validate(license_key);
        if (!result.valid && result.status != "connection_error") {
            spdlog::error("License validation failed during periodic check");
            spdlog::error("Status: {}, Message: {}", result.status, result.message);
            // Optionally: trigger graceful shutdown or notification
        }
    }
);

scheduler.schedule(validation_task);
```

---

## Error Handling

### Validation Status Codes

| Status | Meaning | Action |
|--------|---------|--------|
| `active` | License is valid and active | Continue operation |
| `invalid_format` | License key format is wrong | Check configuration |
| `not_found` | License key not in database | Contact support |
| `pending_payment` | License pending payment | Complete payment |
| `expired` | License has expired | Renew license |
| `cancelled` | License was cancelled | Contact support |
| `suspended` | License temporarily suspended | Contact support |
| `connection_error` | Cannot reach validation server | Check network/use cached result |

### Offline Mode

For air-gapped deployments, ThemisDB can cache validation results:

```cpp
// Check cache validity
auto cache_age = std::chrono::system_clock::now() - validator.getLastValidation();
auto grace_period = std::chrono::hours(24 * config.license.offline_grace_period_days);

if (cache_age > grace_period) {
    spdlog::error("License cache expired. Please validate license online.");
    return 1;
}

// Use cached result
auto cached = validator.getCachedResult();
if (cached && cached->valid) {
    spdlog::info("Using cached license validation (offline mode)");
}
```

---

## Testing

### Test License Validation

```bash
# Test with curl
curl -X POST https://service.themisdb.org:6734/license/validate \
  -H "Content-Type: application/json" \
  -d '{
    "license_key": "THEMIS-ENT-A1B2C3D4-E5F6G7H8",
    "server_hostname": "test-server",
    "server_version": "1.3.0"
  }'
```

### Test from ThemisDB

```bash
# Add to ThemisDB config
themis_server --config config.yaml --test-license

# Expected output:
# [INFO] Validating license...
# [INFO] License validated successfully
# [INFO] Edition: enterprise
# [INFO] Max Nodes: 100
# [INFO] License expires in 365 days
```

---

## Support

For license issues:
- **Email**: support@themisdb.com
- **Portal**: https://service.themisdb.org
- **Documentation**: https://docs.themisdb.org/license

## Security Notes

- License validation uses HTTPS in production
- Server hostname and version are logged for audit purposes
- Failed validation attempts are logged on the pricing server
- Rate limiting prevents abuse (60 requests/minute per license)
