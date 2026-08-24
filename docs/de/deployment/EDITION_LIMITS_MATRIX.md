# Edition Limits & Features Matrix

**Stand:** 23. April 2026  
**Version:** v1.4.0  
**Kategorie:** 🏢 Editions  
**Status:** Single Source of Truth

---

## 📑 Inhaltsverzeichnis

- [Quick Comparison Table](#-quick-comparison-table)
- [MINIMAL Edition](#-minimal-edition-1-node)
- [COMMUNITY Edition](#-community-edition-5-nodes)
- [ENTERPRISE Edition](#-enterprise-edition-100-nodes)
- [HYPERSCALER Edition](#-hyperscaler-edition-unlimited)
- [Feature Availability Table](#-feature-availability-table)
- [Hardware Limits Details](#-hardware-limits-details)
- [License Information](#-license-information)
- [Upgrade Paths](#-upgrade-paths)

---

## 📊 Quick Comparison Table

| Aspect | MINIMAL | COMMUNITY | MILITARY | ENTERPRISE | HYPERSCALER |
|--------|---------|-----------|----------|------------|-------------|
| **Max Nodes** | 1 | **5** ✅ | **100** ✅ | ∞ |
| **GPU VRAM** | 0 GB | 16 GB (1× T4) | 80 GB (2× A100 40G) | 320 GB (4× A100 80G) | ∞ |
| **Max Shards** | 0 | 0 | 100 | ∞ |
| **License** | Optional | Optional | Required (Release) ⚠️ | Mandatory ❌ |
| **License (Debug)** | Optional | Optional | Optional | Mandatory |
| **Lizenzkosten** | Kostenlos | Kostenlos | Ab €5.000/Monat | OEM Deal |
| **Support** | Community | Community | Commercial 24/7 | Dedicated Team |
| **Use Case** | Embedded, IoT | Dev, Test, Startups | Production Clusters | Enterprise Scale |
| **Target Users** | Hobbyisten, Edge | Entwickler, kleine Teams | Mittlere bis große Unternehmen | Cloud Provider, OEM |
| **Typical Deployment** | Raspberry Pi, Edge | 1-5 Server | 10-100 Server | 100-10000+ Server |

---

## 🔹 MINIMAL Edition (1 Node)

### Übersicht

**Single-Node, Lightweight, Embedded-Optimized**

Die MINIMAL Edition ist für **Embedded Systems, IoT und Edge Computing** optimiert:

```
┌─────────────────────────────────────────────┐
│          MINIMAL EDITION                    │
├─────────────────────────────────────────────┤
│ Max Nodes:        1                         │
│ GPU VRAM:         0 GB (Kein GPU Support)   │
│ License:          Optional                  │
│ Binärgröße:       ~80-120 MB               │
│ Memory Footprint: ~50-100 MB               │
├─────────────────────────────────────────────┤
│ ✅ JSON, Document Store                     │
│ ✅ Graph Database                           │
│ ✅ Time-Series                              │
│ ✅ Full-Text Search                         │
│ ✅ Geo-Spatial Queries                      │
│ ❌ GPU Acceleration                         │
│ ❌ LLM Support                              │
│ ❌ Multi-Node Sharding                      │
│ ❌ Enterprise Features                      │
└─────────────────────────────────────────────┘
```

### Hardware Requirements

| Komponente | Minimum | Empfohlen |
|------------|---------|-----------|
| **CPU** | 1 Core (ARMv7+) | 2 Cores (ARMv8) |
| **RAM** | 256 MB | 512 MB - 1 GB |
| **Storage** | 100 MB | 500 MB |
| **Network** | 10 Mbps | 100 Mbps |

### Use Cases

✅ **Ideal für:**
- 🔌 Raspberry Pi Zero / Pi 3/4/5
- 📱 Edge Computing Devices
- 🏠 Home Server (Single-Node)
- 🔬 Prototyping & Research
- 💡 IoT Gateways

❌ **Nicht geeignet für:**
- Multi-Node Deployments
- GPU-beschleunigte Workloads
- LLM/AI Embeddings
- Production Clusters

### Build Example

```bash
cmake -B build -S . \
  -DCMAKE_BUILD_TYPE=MinSizeRel \
  -DTHEMIS_EDITION=MINIMAL \
  -DTHEMIS_EMBEDDED=ON \
  -DTHEMIS_BUILD_TESTS=OFF \
  -DTHEMIS_BUILD_BENCHMARKS=OFF

cmake --build build --parallel 4
```

---

## 🟢 COMMUNITY Edition (5 Nodes)

### Übersicht

**Open Source, Development & Small Production**

Die COMMUNITY Edition ermöglicht **bis zu 5 Nodes** für kleine Cluster:

```
┌─────────────────────────────────────────────┐
│        COMMUNITY EDITION (v1.4.0)           │
├─────────────────────────────────────────────┤
│ Max Nodes:        5 ✅ (NEU: 1 → 5)         │
│ GPU VRAM:         16 GB (1× Tesla T4)                     │
│ License:          Optional                  │
│ Lizenz:           MIT (Open Source)         │
│ Support:          Community (GitHub)        │
├─────────────────────────────────────────────┤
│ ✅ Alle Kern-Features                       │
│ ✅ GPU Acceleration (bis 16 GB, 1× Tesla T4)            │
│ ✅ LLM Support (Embedding, Inference)       │
│ ✅ gRPC, HTTP/2, HTTP/3                     │
│ ✅ WebSocket, SSE, GraphQL                  │
│ ✅ Multi-Node (bis 5 Nodes)                │
│ ❌ Auto-Sharding                            │
│ ❌ Enterprise Plugins                       │
│ ❌ RBAC, Field Encryption                   │
│ ❌ Multi-Master Replication                 │
└─────────────────────────────────────────────┘
```

### Hardware Requirements

| Komponente | Minimum | Empfohlen (5 Nodes) |
|------------|---------|---------------------|
| **CPU** | 4 Cores | 8+ Cores pro Node |
| **RAM** | 8 GB | 16-32 GB pro Node |
| **GPU VRAM** | Optional | 16 GB (NVIDIA Tesla T4) |
| **Storage** | 100 GB | 500 GB - 1 TB SSD |
| **Network** | 1 Gbps | 10 Gbps |

### Node Limits (NEW v1.4.0)

**WICHTIG:** Community Edition erlaubt jetzt **bis zu 5 Nodes**:

```
┌─────────────────────────────────────────┐
│   COMMUNITY: 1 Node → 5 Nodes ✅        │
├─────────────────────────────────────────┤
│ Alte Version (v1.3.x):  1 Node          │
│ Neue Version (v1.4.0):  5 Nodes ✅      │
│                                         │
│ Use Cases:                              │
│ - Development Cluster (3 Nodes)         │
│ - High Availability (3 Nodes)           │
│ - Read Replicas (1 Master + 4 Replicas)│
│ - Multi-Region (5 Regions)              │
└─────────────────────────────────────────┘
```

### Use Cases

✅ **Ideal für:**
- 💻 Entwicklung & Testing
- 🚀 Startups & kleine Teams (bis 10 Entwickler)
- 🎓 Universitäten & Forschung
- 🏢 Kleine Produktionsumgebungen (1-5 Nodes)
- 📊 Analytics & BI (GPU-beschleunigt)
- 🤖 LLM Embeddings (bis 10M Vektoren)

❌ **Nicht geeignet für:**
- Große Production Clusters (>5 Nodes)
- Enterprise Compliance (RBAC, Audit)
- Multi-Master Replication
- Custom Plugins

### GPU VRAM Limit: 16 GB (1× Tesla T4)

**16 GB (1× Tesla T4) ist ausreichend für:
- ✅ 1-10 Millionen Embeddings (OpenAI ada-002: 1536 dims)
- ✅ Real-Time Vector Search (Top-1000 Candidates)
- ✅ GPU-beschleunigte Sortierung (10B+ rows)
- ✅ Batch Inference (100-1000 Vektoren/Batch)
- ✅ Die meisten SaaS & Startup Use Cases

**Überschreitung bei:**
- ❌ Fine-Tuning großer LLMs (benötigt 40GB+)
- ❌ 100M+ Embeddings auf Single GPU
- ❌ Real-Time Inference (1000+ Concurrent Requests)
- ❌ Interactive Queries auf Petabyte-Datasets

### Build Example

```bash
# Full Build mit LLM + GPU
cmake -B build -S . \
  -DCMAKE_BUILD_TYPE=Release \
  -DTHEMIS_EDITION=COMMUNITY \
  -DTHEMIS_ENABLE_LLM=ON \
  -DTHEMIS_ENABLE_GPU=ON \
  -DTHEMIS_ENABLE_GRPC=ON \
  -DTHEMIS_ENABLE_HTTP2=ON

cmake --build build --parallel 8
```

### Multi-Node Setup

```yaml
# docker-compose.yml (3-Node Cluster)
version: '3.8'
services:
  themis-node-1:
    image: themisdb:latest
    environment:
      THEMIS_NODE_ID: 1
      THEMIS_CLUSTER_NODES: "node-1,node-2,node-3"
  
  themis-node-2:
    image: themisdb:latest
    environment:
      THEMIS_NODE_ID: 2
      THEMIS_CLUSTER_NODES: "node-1,node-2,node-3"
  
  themis-node-3:
    image: themisdb:latest
    environment:
      THEMIS_NODE_ID: 3
      THEMIS_CLUSTER_NODES: "node-1,node-2,node-3"
```

---

## 🔷 ENTERPRISE Edition (100 Nodes)

### Übersicht

**Production Clusters, Enterprise Features, Commercial Support**

Die ENTERPRISE Edition unterstützt **bis zu 100 Nodes** für große Production-Umgebungen:

```
┌─────────────────────────────────────────────┐
│       ENTERPRISE EDITION (v1.4.0)           │
├─────────────────────────────────────────────┤
│ Max Nodes:        100 ✅ (NEU: 16 → 100)    │
│ GPU VRAM:         320 GB (4× A100 80 GB)                    │
│ License:          Required (Release) ⚠️     │
│ License (Debug):  Optional ✅                │
│ Support:          Commercial 24/7           │
│ Preis:            Ab €5.000/Monat          │
├─────────────────────────────────────────────┤
│ ✅ Alle Community Features                  │
│ ✅ Auto-Sharding (1-100 Nodes)              │
│ ✅ Multi-Master Replication                 │
│ ✅ RBAC & User Management                   │
│ ✅ Field-Level Encryption                   │
│ ✅ HSM Integration (PKCS#11)                │
│ ✅ Enterprise Plugins                       │
│ ✅ Advanced Monitoring                      │
│ ✅ Compliance Audit Logging                 │
│ ❌ Custom OEM Features                      │
└─────────────────────────────────────────────┘
```

### Hardware Requirements

| Komponente | Minimum | Empfohlen (100 Nodes) |
|------------|---------|----------------------|
| **CPU** | 8 Cores | 16+ Cores pro Node |
| **RAM** | 32 GB | 64-128 GB pro Node |
| **GPU VRAM** | Optional | 80-320 GB (Multi-GPU, NVIDIA A100) |
| **Storage** | 1 TB SSD | 10+ TB NVMe pro Node |
| **Network** | 10 Gbps | 25-100 Gbps |

### Node Limits (NEW v1.4.0)

**WICHTIG:** Enterprise Edition erlaubt jetzt **bis zu 100 Nodes**:

```
┌─────────────────────────────────────────┐
│  ENTERPRISE: 16 Nodes → 100 Nodes ✅    │
├─────────────────────────────────────────┤
│ Alte Version (v1.3.x):  16 Nodes        │
│ Neue Version (v1.4.0):  100 Nodes ✅    │
│                                         │
│ Use Cases:                              │
│ - Large Production Clusters (50 Nodes)  │
│ - Multi-Region HA (20 Regions)          │
│ - Read Replicas (1 Master + 99 Replicas)│
│ - Auto-Sharding (100 Shards)            │
└─────────────────────────────────────────┘
```

### License Requirements ⚠️

**WICHTIG:** ENTERPRISE benötigt **Lizenz für Release Builds**:

```bash
# ❌ FEHLER: Release Build ohne Lizenz
cmake -B build -S . \
  -DTHEMIS_EDITION=ENTERPRISE \
  -DCMAKE_BUILD_TYPE=Release
# ERROR: ENTERPRISE Release builds require license!

# ✅ SUCCESS: Release Build mit Lizenz
cmake -B build -S . \
  -DTHEMIS_EDITION=ENTERPRISE \
  -DCMAKE_BUILD_TYPE=Release \
  -DTHEMIS_LICENSE_FILE=/path/to/license.json

# ✅ SUCCESS: Debug Build (Lizenz optional)
cmake -B build -S . \
  -DTHEMIS_EDITION=ENTERPRISE \
  -DCMAKE_BUILD_TYPE=Debug
# Kein Fehler - Development-Flexibilität
```

**Siehe:** [License Requirements](LICENSE_REQUIREMENTS.md) für Details.

### Use Cases

✅ **Ideal für:**
- 🏢 Mittlere bis große Unternehmen
- 🌐 Multi-Node Production Clusters (10-100 Nodes)
- 🔐 Compliance-Anforderungen (GDPR, HIPAA, SOC2)
- 💼 24/7 Commercial Support erforderlich
- 🚀 High Availability & Disaster Recovery
- 📊 Large-Scale Analytics (320 GB GPU (4× A100 80 GB))

❌ **Nicht geeignet für:**
- Massive Scale (>100 Nodes) → Use HYPERSCALER
- Custom OEM Features → Use HYPERSCALER

### GPU VRAM Limit: 320 GB (4× A100 80 GB)

**320 GB (4× A100 80 GB) ermöglicht:
- ✅ Größere Models (7B, 13B Parameter LLMs)
- ✅ 100M+ Embeddings auf Single GPU
- ✅ 1000+ Concurrent Requests
- ✅ Larger Batch Inference
- ✅ Multi-GPU Koordination (4x A100 80GB)

### Enterprise Features

| Feature | COMMUNITY | ENTERPRISE |
|---------|-----------|------------|
| **Auto-Sharding** | ❌ | ✅ (1-100 Nodes) |
| **Multi-Master Replication** | ❌ | ✅ |
| **Cross-Shard Joins** | ❌ | ✅ |
| **RBAC** | ❌ | ✅ |
| **Field Encryption** | ❌ | ✅ |
| **HSM Integration** | ❌ | ✅ |
| **Audit Logging** | Basic | Advanced |
| **Plugin System** | ❌ | ✅ |
| **Commercial Support** | ❌ | ✅ 24/7 |

### Build Example

```bash
# Enterprise Release Build (License erforderlich)
cmake -B build -S . \
  -DCMAKE_BUILD_TYPE=Release \
  -DTHEMIS_EDITION=ENTERPRISE \
  -DTHEMIS_LICENSE_FILE=/path/to/enterprise-license.json \
  -DTHEMIS_ENABLE_MULTI_MASTER=ON \
  -DTHEMIS_ENABLE_RBAC=ON \
  -DTHEMIS_ENABLE_FIELD_ENCRYPTION=ON \
  -DTHEMIS_ENABLE_HSM=ON

cmake --build build --parallel 16
```

### Build Script

```powershell
# Automatisierter Enterprise Build
.\scripts\build-enterprise-release.ps1 `
  -Environment production `
  -Configuration Release `
  -LicenseFile "C:\licenses\enterprise.json"
```

---

## 🔶 HYPERSCALER Edition (Unlimited)

### Übersicht

**OEM, Cloud Provider, Unlimited Scale**

Die HYPERSCALER Edition ermöglicht **unbegrenzte Nodes** für massive Deployments:

```
┌─────────────────────────────────────────────┐
│        HYPERSCALER EDITION                  │
├─────────────────────────────────────────────┤
│ Max Nodes:        ∞ (Unlimited)             │
│ GPU VRAM:         ∞ (Unlimited)             │
│ License:          Mandatory ❌ (all builds)  │
│ Support:          Dedicated Engineering     │
│ Preis:            Custom OEM Deal           │
├─────────────────────────────────────────────┤
│ ✅ Alle Enterprise Features                 │
│ ✅ Unlimited Nodes (1000-10000+)            │
│ ✅ Unlimited GPU VRAM                       │
│ ✅ Custom Optimizations                     │
│ ✅ OEM Branding                             │
│ ✅ Source Code Access (Optional)            │
│ ✅ Dedicated Support Team                   │
│ ✅ Custom Engineering                       │
│ ✅ Multi-DC Coordination                    │
└─────────────────────────────────────────────┘
```

### Hardware Requirements

| Komponente | Empfohlen |
|------------|-----------|
| **CPU** | 32+ Cores pro Node |
| **RAM** | 256+ GB pro Node |
| **GPU VRAM** | 80+ GB (8x A100/H100) |
| **Storage** | 100+ TB NVMe Cluster |
| **Network** | 100+ Gbps |

### License Requirements ❌

**WICHTIG:** HYPERSCALER benötigt **Lizenz für ALLE Builds**:

```bash
# ❌ FEHLER: Jeder Build ohne Lizenz
cmake -B build -S . \
  -DTHEMIS_EDITION=HYPERSCALER
# ERROR: HYPERSCALER requires license (MANDATORY)!

# ✅ SUCCESS: Mit Lizenz
cmake -B build -S . \
  -DTHEMIS_EDITION=HYPERSCALER \
  -DTHEMIS_LICENSE_FILE=/path/to/hyperscaler-license.json

# ❌ FEHLER: Debug Build ohne Lizenz
cmake -B build -S . \
  -DTHEMIS_EDITION=HYPERSCALER \
  -DCMAKE_BUILD_TYPE=Debug
# ERROR: HYPERSCALER requires license (no debug exception)!
```

**Keine Debug-Ausnahme:** HYPERSCALER Edition benötigt Lizenz auch für Debug Builds.

### Use Cases

✅ **Ideal für:**
- ☁️ Cloud Provider (AWS, Azure, GCP)
- 🏭 OEM Deployments
- 🌍 Global Scale (1000-10000+ Nodes)
- 🎯 Custom Engineering Support
- 💼 White-Label Solutions
- 🔧 Custom Feature Development

### Hyperscaler Features

| Feature | ENTERPRISE | HYPERSCALER |
|---------|------------|-------------|
| **Max Nodes** | 100 | ∞ |
| **GPU VRAM** | 320 GB (4× A100 80G) | ∞ |
| **License (Debug)** | Optional | Mandatory |
| **Custom Features** | ❌ | ✅ |
| **OEM Branding** | ❌ | ✅ |
| **Source Access** | ❌ | ✅ (Optional) |
| **Dedicated Team** | ❌ | ✅ |

### Build Example

```bash
cmake -B build -S . \
  -DCMAKE_BUILD_TYPE=Release \
  -DTHEMIS_EDITION=HYPERSCALER \
  -DTHEMIS_LICENSE_FILE=/path/to/hyperscaler-license.json \
  -DTHEMIS_ENABLE_HYPERSCALER_OPTIMIZATION=ON \
  -DTHEMIS_ENABLE_GPU_CLUSTER=ON \
  -DTHEMIS_ENABLE_ADVANCED_CDC=ON

cmake --build build --parallel 32
```

---

## 📋 Feature Availability Table

### Core Features

| Feature | MINIMAL | COMMUNITY | ENTERPRISE | HYPERSCALER |
|---------|---------|-----------|------------|-------------|
| **JSON Document Store** | ✅ | ✅ | ✅ | ✅ |
| **Graph Database** | ✅ | ✅ | ✅ | ✅ |
| **Time-Series** | ✅ | ✅ | ✅ | ✅ |
| **Full-Text Search** | ✅ | ✅ | ✅ | ✅ |
| **Geo-Spatial** | ✅ | ✅ | ✅ | ✅ |
| **Vector Search** | ❌ | ✅ | ✅ | ✅ |

### LLM & AI

| Feature | MINIMAL | COMMUNITY | ENTERPRISE | HYPERSCALER |
|---------|---------|-----------|------------|-------------|
| **LLM Embeddings** | ❌ | ✅ | ✅ | ✅ |
| **Similarity Search** | ❌ | ✅ | ✅ | ✅ |
| **Inference** | ❌ | ✅ | ✅ | ✅ |
| **Fine-Tuning** | ❌ | ❌ | ✅ | ✅ |
| **Model Management** | ❌ | ❌ | ✅ | ✅ |

### GPU Acceleration

| Feature | MINIMAL | COMMUNITY | ENTERPRISE | HYPERSCALER |
|---------|---------|-----------|------------|-------------|
| **GPU Support** | ❌ | ✅ | ✅ | ✅ |
| **Max GPU VRAM** | 0 GB | 16 GB | 80 GB | 320 GB | ∞ |
| **CUDA** | ❌ | ✅ | ✅ | ✅ |
| **Vulkan** | ❌ | ✅ | ✅ | ✅ |
| **Multi-GPU** | ❌ | ❌ | ✅ | ✅ |
| **GPU Cluster** | ❌ | ❌ | ❌ | ✅ |

### Networking

| Feature | MINIMAL | COMMUNITY | ENTERPRISE | HYPERSCALER |
|---------|---------|-----------|------------|-------------|
| **HTTP/1.1** | ✅ | ✅ | ✅ | ✅ |
| **gRPC** | ✅ | ✅ | ✅ | ✅ |
| **HTTP/2** | ❌ | ✅ | ✅ | ✅ |
| **HTTP/3** | ❌ | ✅ | ✅ | ✅ |
| **WebSocket** | ✅ | ✅ | ✅ | ✅ |
| **MQTT** | ❌ | ✅ | ✅ | ✅ |

### Clustering

| Feature | MINIMAL | COMMUNITY | ENTERPRISE | HYPERSCALER |
|---------|---------|-----------|------------|-------------|
| **Max Nodes** | 1 | 5 | 100 | ∞ |
| **Auto-Sharding** | ❌ | ❌ | ✅ | ✅ |
| **Multi-Master** | ❌ | ❌ | ✅ | ✅ |
| **Cross-Shard Joins** | ❌ | ❌ | ✅ | ✅ |
| **Geo-Replication** | ❌ | ❌ | ✅ | ✅ |

### Security & Compliance

| Feature | MINIMAL | COMMUNITY | ENTERPRISE | HYPERSCALER |
|---------|---------|-----------|------------|-------------|
| **TLS/SSL** | ✅ | ✅ | ✅ | ✅ |
| **Basic Auth** | ✅ | ✅ | ✅ | ✅ |
| **RBAC** | ❌ | ❌ | ✅ | ✅ |
| **Field Encryption** | ❌ | ❌ | ✅ | ✅ |
| **HSM Integration** | ❌ | ❌ | ✅ | ✅ |
| **Audit Logging** | Basic | Basic | Advanced | Advanced |

### Support & Licensing

| Feature | MINIMAL | COMMUNITY | ENTERPRISE | HYPERSCALER |
|---------|---------|-----------|------------|-------------|
| **License (Release)** | Optional | Optional | Required | Mandatory |
| **License (Debug)** | Optional | Optional | Optional | Mandatory |
| **Support** | Community | Community | 24/7 Commercial | Dedicated Team |
| **SLA** | ❌ | ❌ | ✅ 99.9% | ✅ 99.99% |
| **Custom Features** | ❌ | ❌ | ❌ | ✅ |
| **Source Access** | ✅ (MIT) | ✅ (MIT) | ❌ | ✅ (Optional) |

---

## 🔧 Hardware Limits Details

### Node Limits

| Edition | Max Nodes | Typical Deployment | Scale |
|---------|-----------|-------------------|-------|
| **MINIMAL** | 1 | Single Device | Embedded |
| **COMMUNITY** | **5** ✅ | 1-5 Servers | Small |
| **ENTERPRISE** | **100** ✅ | 10-100 Servers | Large |
| **HYPERSCALER** | ∞ | 100-10000+ Servers | Massive |

### GPU VRAM Limits

| Edition | Max GPU VRAM | Embeddings (1536d) | Use Case |
|---------|-------------|-------------------|----------|
| **MINIMAL** | 0 GB | 0 | Keine GPU |
| **COMMUNITY** | 16 GB | 1-10M | Startups, SaaS |
| **ENTERPRISE** | 320 GB | 10-100M | Large Production |
| **HYPERSCALER** | ∞ | 100M-1B+ | Global Scale |

### Storage Limits

| Edition | Recommended Storage | Max Data Size |
|---------|-------------------|--------------|
| **MINIMAL** | 100 MB - 10 GB | 1-100 GB |
| **COMMUNITY** | 100 GB - 1 TB | 1-10 TB |
| **ENTERPRISE** | 1 TB - 100 TB | 10 TB - 1 PB |
| **HYPERSCALER** | 100 TB - 10 PB | 1 PB - 100 PB |

### Network Requirements

| Edition | Minimum | Recommended | Max Throughput |
|---------|---------|-------------|----------------|
| **MINIMAL** | 10 Mbps | 100 Mbps | 1 Gbps |
| **COMMUNITY** | 1 Gbps | 10 Gbps | 25 Gbps |
| **ENTERPRISE** | 10 Gbps | 25 Gbps | 100 Gbps |
| **HYPERSCALER** | 25 Gbps | 100 Gbps | 400 Gbps |

---

## 🔐 License Information

### License Requirements Matrix

| Edition | Debug Build | Release Build | Contact |
|---------|------------|---------------|---------|
| **MINIMAL** | ✅ Optional | ✅ Optional | N/A (Open Source) |
| **COMMUNITY** | ✅ Optional | ✅ Optional | N/A (Open Source) |
| **ENTERPRISE** | ✅ Optional | ❌ **Required** ⚠️ | service@themisdb.org |
| **HYPERSCALER** | ❌ **Mandatory** | ❌ **Mandatory** | service@themisdb.org |

### How to Get a License

**ENTERPRISE:**
1. Kontaktiere: service@themisdb.org
2. Preis: Ab €5.000/Monat (abhängig von Nodes)
3. Trial: 30 Tage kostenlos
4. Support: 24/7 Commercial

**HYPERSCALER:**
1. Kontaktiere: service@themisdb.org
2. Preis: Custom OEM Deal
3. Features: Unlimited + Custom Engineering
4. Support: Dedicated Team

### License Integration

```bash
# ENTERPRISE Release (License erforderlich)
cmake -B build -S . \
  -DTHEMIS_EDITION=ENTERPRISE \
  -DCMAKE_BUILD_TYPE=Release \
  -DTHEMIS_LICENSE_FILE=/path/to/license.json

# HYPERSCALER (License immer erforderlich)
cmake -B build -S . \
  -DTHEMIS_EDITION=HYPERSCALER \
  -DTHEMIS_LICENSE_FILE=/path/to/hyperscaler-license.json
```

**Siehe:** [License Requirements](LICENSE_REQUIREMENTS.md) für vollständige Details.

---

## 🔄 Upgrade Paths

### Migration Scenarios

```
┌─────────────────────────────────────────────┐
│          Upgrade Path Matrix                │
├─────────────────────────────────────────────┤
│                                             │
│  MINIMAL ──→ COMMUNITY  (Free)              │
│  MINIMAL ──→ ENTERPRISE (€5k/mo)            │
│                                             │
│  COMMUNITY ──→ ENTERPRISE (€5k/mo)          │
│  COMMUNITY ──→ HYPERSCALER (OEM Deal)       │
│                                             │
│  ENTERPRISE ──→ HYPERSCALER (OEM Deal)      │
│                                             │
└─────────────────────────────────────────────┘
```

### Scenario 1: MINIMAL → COMMUNITY

**Trigger:** Mehr als 1 Node benötigt

```bash
# Vorher: MINIMAL
cmake -B build -S . -DTHEMIS_EDITION=MINIMAL

# Nachher: COMMUNITY (5 Nodes)
cmake -B build -S . -DTHEMIS_EDITION=COMMUNITY
```

**Aufwand:** Keine Lizenz erforderlich, kostenlos

### Scenario 2: COMMUNITY → ENTERPRISE

**Trigger:** Mehr als 5 Nodes oder Enterprise Features benötigt

```bash
# Vorher: COMMUNITY (5 Nodes)
cmake -B build -S . -DTHEMIS_EDITION=COMMUNITY

# Nachher: ENTERPRISE (100 Nodes, License erforderlich)
cmake -B build -S . \
  -DTHEMIS_EDITION=ENTERPRISE \
  -DCMAKE_BUILD_TYPE=Release \
  -DTHEMIS_LICENSE_FILE=/path/to/license.json
```

**Aufwand:** License erforderlich (€5k+/Monat), 24/7 Support

### Scenario 3: ENTERPRISE → HYPERSCALER

**Trigger:** Mehr als 100 Nodes oder Custom Features benötigt

```bash
# Vorher: ENTERPRISE (100 Nodes)
cmake -B build -S . \
  -DTHEMIS_EDITION=ENTERPRISE \
  -DTHEMIS_LICENSE_FILE=/path/to/enterprise.json

# Nachher: HYPERSCALER (Unlimited)
cmake -B build -S . \
  -DTHEMIS_EDITION=HYPERSCALER \
  -DTHEMIS_LICENSE_FILE=/path/to/hyperscaler.json
```

**Aufwand:** OEM Deal, Custom Engineering, Dedicated Team

### Migration Checklist

- [ ] Backup-Strategie planen
- [ ] Test-Migration durchführen
- [ ] Lizenz beschaffen (ENTERPRISE/HYPERSCALER)
- [ ] Build-Konfiguration anpassen
- [ ] Tests durchführen
- [ ] Production-Migration planen
- [ ] Rollback-Plan erstellen

---

## 🔗 Verwandte Dokumentation

### Build & Deployment
- [CMake Build System Overview](CMAKE_BUILD_SYSTEM_OVERVIEW.md) - Architektur
- [Build Options Reference](BUILD_OPTIONEN_REFERENZ.md) - Alle CMake-Optionen
- [Deployment Strategy](deployment_strategy.md) - Strategie & Workflows

### Licensing
- [License Requirements](LICENSE_REQUIREMENTS.md) - Wann ist Lizenz erforderlich?
- [Edition Control Mechanisms](EDITION_CONTROL_MECHANISMS.md) - Technische Details

### Guides
- [Build Guide](../guides/guides_build.md) - Quick Start
- [Cross-Compile Guide](../guides/CROSS_COMPILE_COMPLETE.md) - Cross-Compilation

---

## 📊 Zusammenfassung

**Edition Limits (v1.4.0):**
- MINIMAL: 1 Node, 0 GB GPU, Optional License
- COMMUNITY: **5 Nodes** ✅, 16 GB GPU (1× T4), Optional License
- ENTERPRISE: **100 Nodes** ✅, 320 GB GPU (4× A100 80 GB), **Required License (Release)** ⚠️
- HYPERSCALER: ∞ Nodes, ∞ GPU, **Mandatory License** ❌

**Wichtigste Änderungen:**
- COMMUNITY: 1 → **5 Nodes** (v1.4.0)
- ENTERPRISE: 16 → **100 Nodes** (v1.4.0)
- ENTERPRISE: License **nur für Release** erforderlich
- HYPERSCALER: License **immer** erforderlich

**Siehe auch:** [License Requirements](LICENSE_REQUIREMENTS.md) für vollständige Lizenz-Details.

---

**Letzte Aktualisierung:** 23. April 2026  
**Version:** v1.4.0  
**Status:** Single Source of Truth ✅
