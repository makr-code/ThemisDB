---
category: "⚙️ Infrastructure"
version: "v1.3.0"
status: "✅"
date: "22.12.2025"
---

# 🏢 Multi-Tenancy Support

Isolierte Mandanten-Umgebungen mit Ressourcen-Quotas, Feature-Flags und Verschlüsselungsschlüsseln.

## 📋 Inhaltsverzeichnis

- [📋 Übersicht](#-übersicht)
- [✨ Features](#-features)
- [🚀 Schnellstart](#-schnellstart)
- [📖 Detaillierte Dokumentation](#-detaillierte-dokumentation)
- [💡 Best Practices](#-best-practices)
- [🔧 Troubleshooting](#-troubleshooting)
- [📚 Siehe auch](#-siehe-auch)
- [📝 Changelog](#-changelog)

---

## 📋 Übersicht

ThemisDB unterstützt Multi-Tenancy für isolierte Mandanten-Umgebungen mit vollständiger Ressourcen- und Konfigurationstrennung:

- **Tenant Lifecycle Management**: Erstellen, aktualisieren, löschen und aktivieren/deaktivieren
- **Tenant-Identifikation**: Header-basiert, Pfad-basiert oder Default-Tenant
- **Ressourcen-Quotas**: Storage, Dokumenten, Collections, Queries und Verbindungen
- **Rate Limiting**: Requests pro Sekunde mit Token-Bucket-Algorithmus
- **Feature-Flags**: GPU, Vector Search, Graph Queries, Timeseries, Geo und Full-Text

## Features

### ✅ Implementiert

- **Tenant Lifecycle Management**
  - Tenant erstellen/aktualisieren/löschen
  - Tenant aktivieren/deaktivieren
  - Metadata und Konfiguration

- **Tenant-Identifikation**
  - Header-basiert (`X-Tenant-ID`)
  - Pfad-basiert (`/tenants/{tenant_id}/...`)
  - Default-Tenant für Single-Tenant-Deployments

- **Ressourcen-Quotas**
  - Storage-Limit (Bytes)
  - Dokument-Limit
  - Collection-Limit
  - Concurrent Queries
  - Verbindungslimit

- **Rate Limiting**
  - Requests pro Sekunde
  - Burst-Size (Token Bucket)

- **Feature-Flags**
  - GPU Acceleration
  - Vector Search
  - Graph Queries
  - Timeseries
  - Geo Queries
  - Full-Text Search

- **Verschlüsselung**
  - Tenant-spezifische Encryption Keys
  - Optionale Pflicht-Verschlüsselung

- **Usage Tracking**
  - Storage-Nutzung
  - Request-Zähler
  - Bytes gelesen/geschrieben
  - Rate-Limited Requests

- **Prometheus Metrics**
  - Tenant-spezifische Metriken
  - Resource-Usage Monitoring

## Verwendung

### Tenant erstellen

```cpp
#include "server/tenant_manager.h"

auto& tm = TenantManager::instance();

TenantConfig config;
config.tenant_id = "acme-corp";
config.display_name = "ACME Corporation";
config.max_storage_bytes = 10ULL * 1024 * 1024 * 1024;  // 10 GB
config.max_documents = 1000000;
config.max_concurrent_queries = 50;
config.requests_per_second = 500;
config.allow_gpu_acceleration = true;

auto result = tm.createTenant(config);
if (result == TenantManager::CreateResult::Success) {
    std::cout << "Tenant erstellt!" << std::endl;
}
```

### Tenant-Context aus Request auflösen

```cpp
// HTTP-Request verarbeiten
std::unordered_map<std::string, std::string> headers;
headers["X-Tenant-ID"] = "acme-corp";

auto ctx = tm.resolveContext(headers, "/api/documents", "user@acme.com");
if (ctx) {
    std::cout << "Tenant: " << ctx->tenant_id << std::endl;
    std::cout << "User: " << ctx->user_id << std::endl;
    std::cout << "GPU erlaubt: " << ctx->gpu_allowed << std::endl;
}
```

### RAII Context Guard

```cpp
// Automatisches Connection-Tracking
{
    TenantContextGuard guard(*ctx);
    
    if (guard.hasConnection()) {
        // Request verarbeiten
        guard.acquireQuerySlot();
        // Query ausführen
    }
}  // Connection wird automatisch freigegeben
```

### Quota prüfen

```cpp
auto check = tm.checkQuota("acme-corp", "storage", 1000000);
if (!check.allowed) {
    std::cerr << "Quota überschritten: " << check.reason << std::endl;
}
```

### Usage tracken

```cpp
tm.recordRequest("acme-corp");
tm.recordBytesRead("acme-corp", 4096);
tm.incrementDocuments("acme-corp", 1);
```

## REST API

### Tenant-Header

Alle API-Requests können den `X-Tenant-ID` Header verwenden:

```http
GET /api/documents HTTP/1.1
Host: localhost:8080
X-Tenant-ID: acme-corp
Authorization: Bearer <token>
```

### Pfad-basierte Routing

Alternative über URL-Pfad:

```http
GET /tenants/acme-corp/api/documents HTTP/1.1
Host: localhost:8080
Authorization: Bearer <token>
```

## Konfiguration

### TenantManager Configuration

```cpp
TenantManager::Config config;
config.tenant_header = "X-Tenant-ID";
config.tenant_path_prefix = "/tenants/";
config.default_tenant_id = "default";
config.allow_default_tenant = true;
config.global_max_tenants = 1000;
config.enforce_quotas = true;

TenantManager::instance().configure(config);
```

### YAML Configuration (geplant)

```yaml
multi_tenancy:
  enabled: true
  default_tenant: "default"
  tenant_header: "X-Tenant-ID"
  enforce_quotas: true
  max_tenants: 1000
  
  default_quotas:
    max_storage_bytes: 10737418240  # 10 GB
    max_documents: 1000000
    max_connections: 50
    requests_per_second: 1000
```

## Metriken

Prometheus-Format:

```prometheus
# HELP themis_tenant_count Total number of tenants
# TYPE themis_tenant_count gauge
themis_tenant_count 5

# HELP themis_tenant_storage_bytes Storage used by tenant
# TYPE themis_tenant_storage_bytes gauge
themis_tenant_storage_bytes{tenant="acme-corp"} 1073741824

# HELP themis_tenant_requests_total Total requests for tenant
# TYPE themis_tenant_requests_total counter
themis_tenant_requests_total{tenant="acme-corp"} 150000
```

## Best Practices

### 1. Tenant-Isolation

- Jeder Tenant hat isolierte Daten
- Kein Cross-Tenant-Zugriff möglich
- Separate Encryption Keys pro Tenant

### 2. Quota-Management

- Sinnvolle Limits setzen
- Monitoring für Quota-Warnungen
- Graceful Degradation bei Limit-Erreichen

### 3. Performance

- Connection-Pooling pro Tenant
- Query-Throttling für faire Ressourcenverteilung
- Caching mit Tenant-Awareness

### 4. Security

- Tenant-ID immer validieren
- Encryption Keys sicher verwalten
- Audit-Logging für Tenant-Operationen

## Limitationen

- Keine automatische Tenant-Provisioning (manuell oder via Admin API)
- Keine dynamische Quota-Anpassung zur Laufzeit (Restart erforderlich)
- Keine Cross-Tenant-Queries

## Roadmap

- [ ] Admin API für Tenant-Management
- [ ] Billing Integration
- [ ] Dynamic Quota Adjustment
- [ ] Tenant-Level Backup/Restore
- [ ] Tenant Migration Tools

---

**Letzte Aktualisierung:** 30. November 2025  
**Maintainer:** ThemisDB Team
