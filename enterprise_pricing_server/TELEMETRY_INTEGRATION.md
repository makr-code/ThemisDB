# ThemisDB Telemetry Integration Guide

## Overview

This guide explains how ThemisDB instances can report telemetry data to the central pricing server at `https://service.themisdb.org:6734`.

---

## API Endpoint

**URL:** `https://service.themisdb.org:6734/telemetry/heartbeat`  
**Method:** `POST`  
**Content-Type:** `application/json`  
**Recommended Interval:** Every 5-15 minutes

---

## Request Format

```json
{
  "license_key": "THEMIS-ENT-ABCD1234-XYZ789",
  "instance_id": "550e8400-e29b-41d4-a716-446655440000",
  "hostname": "prod-db-01.company.com",
  "version": "1.5.0",
  "metrics": {
    "nodes": 5,
    "total_cores": 80,
    "used_storage_tb": 2.5,
    "uptime_seconds": 86400,
    "query_count_24h": 1500000
  },
  "country": "DE",
  "region": "eu-west-1"
}
```

### Fields

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `license_key` | string | Yes | Your ThemisDB license key |
| `instance_id` | string | Yes | Unique instance identifier (UUID recommended) |
| `hostname` | string | No | Instance hostname |
| `version` | string | Yes | ThemisDB version (e.g., "1.5.0") |
| `metrics.nodes` | int | Yes | Number of active nodes |
| `metrics.total_cores` | int | Yes | Total CPU cores |
| `metrics.used_storage_tb` | float | Yes | Used storage in TB |
| `metrics.uptime_seconds` | int | Yes | Instance uptime in seconds |
| `metrics.query_count_24h` | int | Yes | Query count in last 24h |
| `country` | string | No | Country code (ISO 3166-1 alpha-2) |
| `region` | string | No | Cloud region identifier |

---

## Response Format

### Success Response (200 OK)

```json
{
  "success": true,
  "message": "Heartbeat recorded successfully",
  "instance_count": 3
}
```

### Error Responses

**404 Not Found** - License key not found:
```json
{
  "detail": "License key not found: THEMIS-ENT-..."
}
```

**403 Forbidden** - License suspended:
```json
{
  "detail": "License suspended: THEMIS-ENT-..."
}
```

---

## C++ Implementation

### Option 1: Using libcurl (Recommended)

```cpp
#include <curl/curl.h>
#include <string>
#include <iostream>
#include <uuid/uuid.h>

class ThemisDBTelemetry {
private:
    std::string license_key_;
    std::string instance_id_;
    std::string version_;
    std::string telemetry_url_;
    
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
        ((std::string*)userp)->append((char*)contents, size * nmemb);
        return size * nmemb;
    }
    
    std::string generate_uuid() {
        uuid_t uuid;
        char uuid_str[37];
        uuid_generate(uuid);
        uuid_unparse(uuid, uuid_str);
        return std::string(uuid_str);
    }

public:
    ThemisDBTelemetry(const std::string& license_key, const std::string& version)
        : license_key_(license_key),
          version_(version),
          telemetry_url_("https://service.themisdb.org:6734/telemetry/heartbeat") {
        
        // Generate or load persistent instance ID
        instance_id_ = generate_uuid();
    }
    
    bool send_heartbeat(int nodes, int cores, double storage_tb, 
                       int uptime_sec, int query_count) {
        CURL* curl = curl_easy_init();
        if (!curl) {
            return false;
        }
        
        // Build JSON payload
        std::string json = R"({
            "license_key": ")" + license_key_ + R"(",
            "instance_id": ")" + instance_id_ + R"(",
            "hostname": ")" + get_hostname() + R"(",
            "version": ")" + version_ + R"(",
            "metrics": {
                "nodes": )" + std::to_string(nodes) + R"(,
                "total_cores": )" + std::to_string(cores) + R"(,
                "used_storage_tb": )" + std::to_string(storage_tb) + R"(,
                "uptime_seconds": )" + std::to_string(uptime_sec) + R"(,
                "query_count_24h": )" + std::to_string(query_count) + R"(
            }
        })";
        
        std::string response;
        
        curl_easy_setopt(curl, CURLOPT_URL, telemetry_url_.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        
        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        
        // SSL verification
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
        
        // Timeout settings
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
        
        CURLcode res = curl_easy_perform(curl);
        
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        
        if (res != CURLE_OK) {
            std::cerr << "Telemetry failed: " << curl_easy_strerror(res) << std::endl;
            return false;
        }
        
        if (http_code == 200) {
            std::cout << "Telemetry sent successfully: " << response << std::endl;
            return true;
        } else {
            std::cerr << "Telemetry failed with HTTP " << http_code 
                      << ": " << response << std::endl;
            return false;
        }
    }
    
    std::string get_hostname() {
        char hostname[256];
        gethostname(hostname, sizeof(hostname));
        return std::string(hostname);
    }
};

// Usage Example
int main() {
    ThemisDBTelemetry telemetry("THEMIS-ENT-ABCD1234-XYZ789", "1.5.0");
    
    // Send heartbeat every 5 minutes
    while (true) {
        int nodes = 5;
        int cores = 80;
        double storage_tb = 2.5;
        int uptime_sec = get_uptime();
        int query_count = get_query_count_24h();
        
        telemetry.send_heartbeat(nodes, cores, storage_tb, uptime_sec, query_count);
        
        std::this_thread::sleep_for(std::chrono::minutes(5));
    }
    
    return 0;
}
```

### Compilation

```bash
g++ -o telemetry themis_telemetry.cpp -lcurl -luuid -std=c++17
```

---

### Option 2: Using cpp-httplib (Header-only)

```cpp
#include "httplib.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class ThemisDBTelemetry {
public:
    bool send_heartbeat(const std::string& license_key, 
                       const std::string& instance_id,
                       int nodes, int cores, double storage_tb) {
        
        httplib::SSLClient cli("service.themisdb.org", 6734);
        
        json payload = {
            {"license_key", license_key},
            {"instance_id", instance_id},
            {"version", "1.5.0"},
            {"metrics", {
                {"nodes", nodes},
                {"total_cores", cores},
                {"used_storage_tb", storage_tb},
                {"uptime_seconds", get_uptime()},
                {"query_count_24h", get_query_count()}
            }}
        };
        
        auto res = cli.Post("/telemetry/heartbeat", 
                           payload.dump(), 
                           "application/json");
        
        if (res && res->status == 200) {
            std::cout << "Telemetry sent: " << res->body << std::endl;
            return true;
        }
        
        return false;
    }
};
```

---

### Option 3: Using cpr (Modern C++ wrapper)

```cpp
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

bool send_telemetry(const std::string& license_key, 
                   const std::string& instance_id) {
    
    json payload = {
        {"license_key", license_key},
        {"instance_id", instance_id},
        {"version", "1.5.0"},
        {"metrics", {
            {"nodes", 5},
            {"total_cores", 80},
            {"used_storage_tb", 2.5},
            {"uptime_seconds", 86400},
            {"query_count_24h", 1500000}
        }}
    };
    
    auto response = cpr::Post(
        cpr::Url{"https://service.themisdb.org:6734/telemetry/heartbeat"},
        cpr::Body{payload.dump()},
        cpr::Header{{"Content-Type", "application/json"}},
        cpr::VerifySsl{true},
        cpr::Timeout{30000}
    );
    
    if (response.status_code == 200) {
        std::cout << "Telemetry sent: " << response.text << std::endl;
        return true;
    } else {
        std::cerr << "Telemetry failed: " << response.status_code 
                  << " - " << response.text << std::endl;
        return false;
    }
}
```

---

## Python Client Example

```python
import requests
import uuid
import socket
import time

class ThemisDBTelemetry:
    def __init__(self, license_key: str, version: str):
        self.license_key = license_key
        self.version = version
        self.instance_id = str(uuid.uuid4())
        self.url = "https://service.themisdb.org:6734/telemetry/heartbeat"
    
    def send_heartbeat(self, nodes: int, cores: int, storage_tb: float,
                      uptime_sec: int, query_count: int) -> bool:
        payload = {
            "license_key": self.license_key,
            "instance_id": self.instance_id,
            "hostname": socket.gethostname(),
            "version": self.version,
            "metrics": {
                "nodes": nodes,
                "total_cores": cores,
                "used_storage_tb": storage_tb,
                "uptime_seconds": uptime_sec,
                "query_count_24h": query_count
            }
        }
        
        try:
            response = requests.post(
                self.url,
                json=payload,
                timeout=30,
                verify=True  # SSL verification
            )
            
            if response.status_code == 200:
                print(f"Telemetry sent: {response.json()}")
                return True
            else:
                print(f"Telemetry failed: {response.status_code} - {response.text}")
                return False
        
        except Exception as e:
            print(f"Telemetry error: {e}")
            return False

# Usage
telemetry = ThemisDBTelemetry("THEMIS-ENT-ABCD1234-XYZ789", "1.5.0")

# Send every 5 minutes
while True:
    telemetry.send_heartbeat(
        nodes=5,
        cores=80,
        storage_tb=2.5,
        uptime_sec=86400,
        query_count=1500000
    )
    time.sleep(300)  # 5 minutes
```

---

## Configuration

### ThemisDB config.yaml

Add telemetry settings:

```yaml
telemetry:
  enabled: true
  interval_minutes: 5
  url: https://service.themisdb.org:6734/telemetry/heartbeat
  timeout_seconds: 30
  retry_attempts: 3
```

### Opt-Out

To disable telemetry:

```yaml
telemetry:
  enabled: false
```

Or via environment variable:

```bash
THEMISDB_TELEMETRY_ENABLED=false
```

---

## Privacy & Security

### Data Collected

**Only essential metrics:**
- ✅ Version number
- ✅ Node count
- ✅ CPU cores
- ✅ Storage usage
- ✅ Uptime
- ✅ Query count

**NOT collected:**
- ❌ Query content
- ❌ Customer data
- ❌ Database names
- ❌ Schema information
- ❌ User credentials

### Security

- **HTTPS Only:** All communication encrypted via TLS
- **SSL Verification:** Certificate validation enforced
- **Rate Limited:** Max 1 request per 5 minutes
- **No PII:** No personally identifiable information transmitted

---

## Testing

### Test Endpoint

```bash
curl -X POST https://service.themisdb.org:6734/telemetry/heartbeat \
  -H "Content-Type: application/json" \
  -d '{
    "license_key": "THEMIS-ENT-TEST-1234",
    "instance_id": "test-instance-001",
    "version": "1.5.0",
    "metrics": {
      "nodes": 1,
      "total_cores": 8,
      "used_storage_tb": 0.1,
      "uptime_seconds": 3600,
      "query_count_24h": 1000
    }
  }'
```

### View Statistics

```bash
curl https://service.themisdb.org:6734/telemetry/statistics
```

---

## Troubleshooting

### Connection Refused

- Check firewall rules for outbound HTTPS (port 443)
- Verify DNS resolution: `nslookup service.themisdb.org`
- Test connectivity: `telnet service.themisdb.org 6734`

### SSL Certificate Errors

- Ensure system CA certificates are up to date
- Check system time is synchronized (NTP)

### License Key Not Found

- Verify license key is correct
- Check subscription is active: `/license/validate/{key}`

### Rate Limiting

- Ensure interval is at least 5 minutes between requests
- Check for multiple instances using same instance_id

---

## Support

For telemetry issues, contact: support@themisdb.org

API Documentation: https://service.themisdb.org:6734/docs
