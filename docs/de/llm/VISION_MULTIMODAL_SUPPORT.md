# Vision/Multi-Modal Support Enhancement Guide

**Version:** 1.0.0  
**Status:** Production Ready  
**Last Updated:** April 2026

---

## Übersicht

Diese Dokumentation beschreibt die Verbesserungen der Vision/Multi-Modal-Unterstützung in ThemisDB, die die experimentellen LLAVA/CLIP-Features in produktionsreife Funktionen umwandeln.

## Problemstellung

Die ursprüngliche Vision-Unterstützung in ThemisDB war:
- ❌ Nur experimentell und auf spezielle Modelle/Lizenzen limitiert
- ❌ Ohne robustes API-Versioning und Stabilit ätsgarantien
- ❌ Ohne Lizenzmanagement für Model-Access
- ❌ Ohne erweiterte Monitoring/Restriktionen
- ❌ Ohne Production-Readiness

## Lösung

Die erweiterte Vision-Unterstützung bietet nun:
- ✅ **API Versioning & Stability**: Semantische Versionierung mit Deprecation-Policy
- ✅ **License Management**: Automatische Lizenzvalidierung und Compliance-Tracking
- ✅ **Resource Monitoring**: Umfassendes Monitoring mit Prometheus-Integration
- ✅ **Security & Sandboxing**: Input-Validierung, Ressourcen-Limits, optionale Sandboxing
- ✅ **Production Features**: Rate Limiting, Quotas, Audit Logging

---

## Architektur

```
┌─────────────────────────────────────────────────────────────┐
│                     Vision API Layer                          │
│  - API Versioning (v1.0.0)                                   │
│  - Stability Guarantees (stable/beta/experimental)            │
│  - Deprecation Policy                                         │
└────────────────────────┬────────────────────────────────────┘
                         │
┌────────────────────────▼────────────────────────────────────┐
│                 License Management                            │
│  - Model License Validation                                   │
│  - Commercial Use Compliance                                  │
│  - Attribution Requirements                                   │
└────────────────────────┬────────────────────────────────────┘
                         │
┌────────────────────────▼────────────────────────────────────┐
│              Resource Monitor & Control                       │
│  ┌──────────────┬──────────────┬──────────────┐            │
│  │ Rate Limiter │ Quota Tracker│ Usage Stats  │            │
│  └──────────────┴──────────────┴──────────────┘            │
└────────────────────────┬────────────────────────────────────┘
                         │
┌────────────────────────▼────────────────────────────────────┐
│                   Vision Encoder                              │
│  - Input Validation                                           │
│  - CLIP Model Loading                                         │
│  - Image Encoding                                             │
│  - Resource Tracking                                          │
└──────────────────────────────────────────────────────────────┘
```

---

## Komponenten

### 1. Vision Configuration (`vision_config.h/cpp`)

Zentrales Konfigurations-Management für alle Vision-Features.

#### Features:
- API Versioning und Stability-Level
- License Management und Compliance
- Resource Limits und Quotas
- Monitoring und Audit-Konfiguration
- Security und Sandboxing-Optionen
- Feature Flags

#### Verwendung:

```cpp
#include "llm/vision_config.h"

// Load configuration from file
auto config = VisionConfig::loadFromFile("config/vision_config.yaml");

// Check API stability
if (config->getAPIStability() == VisionAPIStability::STABLE) {
    // Safe to use in production
}

// Validate model license
if (config->validateModelUsage("llava-v1.5-7b", true)) {
    // Commercial use allowed
}

// Get resource limits
const auto& limits = config->getResourceLimits();
std::cout << "Max concurrent requests: " << limits.max_concurrent_requests << std::endl;
```

### 2. Resource Monitor (`vision_resource_monitor.h/cpp`)

Überwacht und erzwingt Ressourcen-Limits, Rate-Limits und Quotas.

#### Features:
- Rate Limiting (Token Bucket Algorithm)
- Per-User Quotas (Daily/Monthly)
- Resource Usage Tracking (Memory, VRAM)
- Prometheus Metrics Export
- Audit Logging
- Health Monitoring

#### Verwendung:

```cpp
#include "llm/vision_resource_monitor.h"

// Initialize monitor
auto config = VisionConfig::loadFromFile("config/vision_config.yaml");
auto monitor = std::make_shared<VisionResourceMonitor>(config);
monitor->initialize();

// Check if request can be accepted
if (monitor->canAcceptRequest("user123", 512)) {
    auto request_id = monitor->startRequest("user123", "llava-v1.5-7b");
    
    // Perform inference...
    
    monitor->completeRequest(request_id, true, std::chrono::milliseconds(150), 512);
} else {
    // Request rejected - rate limit or quota exceeded
}

// Get usage statistics
auto usage = monitor->getResourceUsage();
std::cout << "Active requests: " << usage.active_requests << std::endl;
std::cout << "Memory usage: " << usage.current_memory_mb << " MB" << std::endl;

// Export Prometheus metrics
std::string metrics = monitor->exportMetrics();
```

### 3. Enhanced Vision Encoder (`vision_encoder.h/cpp`)

Erweiterter Vision Encoder mit Konfiguration und Monitoring-Integration.

#### Features:
- Configuration-driven validation
- License compliance checking
- Resource monitoring integration
- Enhanced error handling
- User context tracking
- Backward compatibility

#### Verwendung:

```cpp
#include "llm/vision_encoder.h"

// New API with configuration
auto config = VisionConfig::loadFromFile("config/vision_config.yaml");
auto monitor = std::make_shared<VisionResourceMonitor>(config);
auto encoder = std::make_shared<VisionEncoder>(
    "./models/mmproj-model-f16.gguf",
    config,
    monitor
);

// Set user context for tracking
encoder->setUserContext("user123");

// Validate image before encoding
if (encoder->validateImage("/path/to/image.jpg")) {
    auto embeddings = encoder->encodeImage("/path/to/image.jpg");
    // Use embeddings...
}

// Get license information
auto license = encoder->getModelLicense();
if (license) {
    std::cout << "Model licensed under: " << license->license_name << std::endl;
}
```

---

## Konfiguration

### Haupt-Konfiguration (`config/vision_config.yaml`)

Siehe vollständige Beispiel-Konfiguration in `/config/vision_config.yaml`.

#### Wichtigste Einstellungen:

```yaml
vision:
  # API Configuration
  api:
    version: "1.0.0"
    stability:
      level: "stable"  # experimental, beta, stable, deprecated
  
  # License Management
  licensing:
    enforce_licenses: true
    allowed_licenses:
      - "MIT"
      - "Apache-2.0"
      - "Llama-2-Community"
  
  # Resource Limits
  resources:
    limits:
      max_memory_mb: 8192
      max_concurrent_requests: 16
    
    rate_limiting:
      enabled: true
      global:
        requests_per_minute: 120
    
    quotas:
      enabled: true
      default:
        daily_requests: 10000
        monthly_requests: 300000
  
  # Monitoring
  monitoring:
    enabled: true
    metrics:
      prometheus:
        enabled: true
        port: 9092
    audit:
      enabled: true
      retention_days: 90
  
  # Security
  security:
    validation:
      enabled: true
      max_image_size_mb: 25
      allowed_formats: ["JPEG", "PNG", "BMP", "WEBP"]
    
    sandboxing:
      enabled: false  # Optional, für maximale Isolation
```

### Lizenz-Datenbank (`config/vision_licenses.yaml`)

Definiert Lizenz-Informationen für alle Vision-Modelle.

```yaml
licenses:
  MIT:
    name: "MIT License"
    commercial_use: true
    modification: true
    attribution_required: true
  
  Llama-2-Community:
    name: "Llama 2 Community License Agreement"
    commercial_use: true
    restrictions:
      - "Monthly active users > 700M requires separate license"

models:
  llava-v1.5-7b:
    name: "LLaVA 1.5 (7B)"
    license: "Llama-2-Community"
    attribution: "Haotian Liu et al., LLaVA"
    production_ready: true
```

---

## API Versioning & Stability

### Stability Levels

| Level | Beschreibung | Empfehlung |
|-------|--------------|------------|
| `EXPERIMENTAL` | Features können sich ändern | Nur für Entwicklung |
| `BETA` | Meist stabil, aber noch in Testing | Staging-Umgebungen |
| `STABLE` | Produktionsreif | Production |
| `DEPRECATED` | Wird entfernt | Migration planen |

### Deprecation Policy

- **Ankündigung**: 12 Monate vor Entfernung
- **Breaking Changes**: Nur in Major-Versionen
- **Backward Compatibility**: In Minor-Versionen garantiert

### API Prefix

```
/api/v1/vision/*    - Current stable API
/api/v2/vision/*    - Future version (when needed)
```

---

## License Management

### Unterstützte Lizenzen

- **MIT**: Vollständig offen, kommerzielle Nutzung erlaubt
- **Apache-2.0**: Offen mit Patent-Grant
- **BSD-3-Clause**: Einfache permissive Lizenz
- **Llama-2-Community**: Meta's Llama-Lizenz mit Einschränkungen
- **OpenRAIL-M**: Responsible AI License mit Use-Restrictions

### License Enforcement

```cpp
// Check if license allows commercial use
if (config->validateModelUsage("llava-v1.5-7b", true)) {
    // Commercial use allowed
} else {
    // Commercial use not permitted
}

// Get required attribution
std::string attribution = config->getRequiredAttribution("llava-v1.5-7b");
// Display in UI or API response
```

### Compliance Tracking

Alle Model-Nutzung wird automatisch geloggt für Compliance-Audits:

```
2026-01-19 10:15:23 [INFO] Model loaded: llava-v1.5-7b (License: Llama-2-Community)
2026-01-19 10:15:45 [INFO] Request completed: model=llava-v1.5-7b, user=user123
```

---

## Monitoring & Observability

### Prometheus Metrics

Verfügbare Metriken:

```
# Requests
themisdb_vision_requests_total
themisdb_vision_requests_active
themisdb_vision_requests_rejected_total

# Resources
themisdb_vision_memory_bytes
themisdb_vision_vram_bytes
themisdb_vision_models_loaded

# Latency
themisdb_vision_inference_duration_seconds{quantile="0.5"}
themisdb_vision_inference_duration_seconds{quantile="0.9"}

# Rate Limiting
themisdb_vision_rate_limit_tokens_available
themisdb_vision_rate_limit_exceeded_total

# Quotas
themisdb_vision_quota_remaining{user="user123",type="daily"}
```

### Grafana Dashboard

Integration mit Grafana für Visualisierung:

```yaml
# In config/grafana/vision_dashboard.json
{
  "dashboard": {
    "panels": [
      {
        "title": "Vision Request Rate",
        "targets": [
          {
            "expr": "rate(themisdb_vision_requests_total[5m])"
          }
        ]
      }
    ]
  }
}
```

### Audit Logging

Alle sicherheitsrelevanten Events werden geloggt:

- Model Loading/Unloading
- Request Acceptance/Rejection
- License Violations
- Resource Limit Exceeded
- Security Violations

```cpp
// Get audit log
auto entries = monitor->getAuditLog(100);
for (const auto& entry : entries) {
    std::cout << entry.timestamp << " - " << entry.event_type 
              << " - User: " << entry.user_id << std::endl;
}
```

---

## Security & Sandboxing

### Input Validation

Automatische Validierung aller Eingaben:

```cpp
// Image validation includes:
// - File size check (<= 25MB default)
// - Format validation (JPEG, PNG, BMP, WEBP)
// - Resolution limits (4096x4096 default)
// - Integrity check
// - Optional malware scanning

if (encoder->validateImage("/path/to/image.jpg")) {
    // Safe to process
}
```

### Resource Limits

Konfigurierbare Limits verhindern Ressourcen-Exhaustion:

- **Memory Limits**: Pro-Request und Global
- **VRAM Limits**: Pro-Model und Global
- **Concurrent Requests**: Verhindert Überlastung
- **Queue Size**: Begrenzt Warteschlange
- **Timeouts**: Inference und Model-Loading

### Sandboxing (Optional)

Für maximale Sicherheit kann Vision-Processing in isolierten Containern/Prozessen laufen:

```yaml
security:
  sandboxing:
    enabled: true
    type: "container"  # process, container, vm
    isolate_memory: true
    isolate_network: true
    isolate_filesystem: true
    sandbox_memory_mb: 4096
    sandbox_timeout_seconds: 120
```

---

## Production Deployment

### Deployment Checklist

- [ ] Konfiguration anpassen (`vision_config.yaml`)
- [ ] Lizenzen prüfen und dokumentieren
- [ ] Resource-Limits für Hardware setzen
- [ ] Monitoring aktivieren (Prometheus/Grafana)
- [ ] Audit-Logging konfigurieren
- [ ] Rate-Limits und Quotas definieren
- [ ] Security-Validierung aktivieren
- [ ] Health-Checks implementieren
- [ ] Backup und Disaster-Recovery planen
- [ ] Dokumentation für Ops-Team

### Performance Tuning

```yaml
# High-Throughput Configuration
resources:
  limits:
    max_concurrent_requests: 32  # Erhöhen für mehr Throughput
    max_memory_mb: 16384         # Mehr Memory für größere Modelle
  
  rate_limiting:
    enabled: true
    global:
      requests_per_minute: 240   # 2x Standard

# Low-Latency Configuration
pipeline:
  preprocessing:
    cache_preprocessed: true     # Cache für schnellere Verarbeitung
    cache_ttl_seconds: 7200      # Längere Cache-Zeit
  
  error_handling:
    retry:
      enabled: true
      max_attempts: 1            # Weniger Retries für niedrigere Latenz
```

### Scaling

Horizontale Skalierung:

```yaml
# Load Balancer Configuration
upstream vision_backend {
    least_conn;  # Verbindungen gleichmäßig verteilen
    
    server vision-node-1:8080 max_fails=3 fail_timeout=30s;
    server vision-node-2:8080 max_fails=3 fail_timeout=30s;
    server vision-node-3:8080 max_fails=3 fail_timeout=30s;
}
```

---

## Migration von Experimental zu Production

### Schritt 1: Konfiguration erstellen

```bash
cp config/image_analysis.yaml config/vision_config.yaml
# Anpassen der Einstellungen
```

### Schritt 2: Code migrieren

**Alt (Experimental):**
```cpp
VisionEncoder encoder("/models/mmproj-model-f16.gguf");
auto embeddings = encoder.encodeImage("/path/to/image.jpg");
```

**Neu (Production):**
```cpp
auto config = VisionConfig::loadFromFile("config/vision_config.yaml");
auto monitor = std::make_shared<VisionResourceMonitor>(config);
monitor->initialize();

auto encoder = std::make_shared<VisionEncoder>(
    "/models/mmproj-model-f16.gguf",
    config,
    monitor
);

encoder->setUserContext("user123");
if (encoder->validateImage("/path/to/image.jpg")) {
    auto embeddings = encoder->encodeImage("/path/to/image.jpg");
}
```

### Schritt 3: Testing

```bash
# Unit Tests
./build/tests/test_vision_config
./build/tests/test_vision_resource_monitor
./build/tests/test_llm_vision_encoder

# Integration Tests
./build/tests/test_llm_vision_integration

# Load Tests
./benchmarks/bench_vision_throughput
```

---

## Troubleshooting

### Problem: Rate Limit Exceeded

```
Error: Request rejected: resource limits exceeded
```

**Lösung:**
1. Prüfe Rate-Limit-Konfiguration
2. Erhöhe `requests_per_minute` wenn nötig
3. Implementiere Request-Queueing

### Problem: Quota Exceeded

```
Error: User quota exhausted
```

**Lösung:**
1. Prüfe User-Quota in Konfiguration
2. Reset-Period prüfen (daily/monthly)
3. Quota-Limits erhöhen oder Enforcement auf "soft" setzen

### Problem: License Violation

```
Error: Model license does not permit usage
```

**Lösung:**
1. Prüfe Model-Lizenz in `vision_licenses.yaml`
2. Aktiviere Commercial-Use Flag wenn berechtigt
3. Verwende alternatives Modell mit passender Lizenz

### Problem: Memory Limit Exceeded

```
Error: Memory limit exceeded
```

**Lösung:**
1. Erhöhe `max_memory_mb` in Konfiguration
2. Reduziere `max_concurrent_requests`
3. Implementiere Model-Unloading

---

## Best Practices

### 1. Always Use Configuration

```cpp
// ✅ Good
auto config = VisionConfig::loadFromFile("config/vision_config.yaml");
auto encoder = std::make_shared<VisionEncoder>(model_path, config, monitor);

// ❌ Bad
auto encoder = std::make_shared<VisionEncoder>(model_path);  // No limits, no monitoring
```

### 2. Set User Context

```cpp
// ✅ Good
encoder->setUserContext(user_id);
auto embeddings = encoder->encodeImage(image_path);

// ❌ Bad
auto embeddings = encoder->encodeImage(image_path);  // No user tracking
```

### 3. Validate Before Processing

```cpp
// ✅ Good
if (encoder->validateImage(image_path)) {
    auto embeddings = encoder->encodeImage(image_path);
}

// ❌ Bad
auto embeddings = encoder->encodeImage(image_path);  // May process invalid images
```

### 4. Handle Errors Gracefully

```cpp
// ✅ Good
try {
    auto embeddings = encoder->encodeImage(image_path);
} catch (const std::exception& e) {
    logger->error("Vision encoding failed: {}", e.what());
    // Fallback or error response
}

// ❌ Bad
auto embeddings = encoder->encodeImage(image_path);  // Uncaught exceptions
```

### 5. Monitor Resource Usage

```cpp
// ✅ Good
auto health = monitor->getHealthStatus();
if (!health.healthy) {
    logger->warn("Vision system unhealthy: {}", health.status);
    for (const auto& issue : health.issues) {
        logger->warn("  - {}", issue);
    }
}

// ❌ Bad
// No health monitoring
```

---

## Referenzen

### Dateien

- **Konfiguration**: `/config/vision_config.yaml`
- **Lizenzen**: `/config/vision_licenses.yaml`
- **Header**: `/include/llm/vision_*.h`
- **Implementation**: `/src/llm/vision_*.cpp`
- **Tests**: `/tests/test_llm_vision_*.cpp`

### Externe Links

- [llama.cpp CLIP Integration](https://github.com/ggerganov/llama.cpp/tree/master/examples/llava)
- [OpenAI CLIP](https://github.com/openai/CLIP)
- [LLaVA Project](https://llava-vl.github.io/)
- [Prometheus Monitoring](https://prometheus.io/)

---

## Support

Bei Fragen oder Problemen:

1. Check diese Dokumentation
2. Review GitHub Issues: [Vision/Multi-Modal Label](https://github.com/makr-code/ThemisDB/issues?q=label%3Avision)
3. Community Forum: [ThemisDB Discussions](https://github.com/makr-code/ThemisDB/discussions)

---

**Version History:**
- v1.0.0 (2026-01): Initial production-ready release
