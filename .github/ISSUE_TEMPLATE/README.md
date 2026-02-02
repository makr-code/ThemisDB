# ThemisDB GitHub Issue Templates

Willkommen bei den ThemisDB Issue Templates! / Welcome to the ThemisDB Issue Templates!

Diese Templates helfen bei der strukturierten Erstellung von Issues für verschiedene Zwecke. / These templates help create structured issues for various purposes.

## 📋 Template-Kategorien / Template Categories

### 👥 Für Benutzer / For Users

**Standard-Issue-Templates für alle Benutzer:**

- **`bug_report.md`** - Fehlerberichte / Bug reports
- **`feature_request.md`** - Feature-Wünsche / Feature requests
- **`documentation_improvement.md`** - Dokumentationsverbesserungen / Documentation improvements
- **`documentation_issue.md`** - Dokumentationsprobleme / Documentation issues

### 🤖 KI-gestützte Systematische Reviews / AI-Powered Systematic Reviews

**Wiederholbare Templates für regelmäßige Komponenten-Reviews (quartalsweise empfohlen):**

- **`ai-review-component-template.md`** - Universal-Template für jede Komponente / Universal template for any component
- **`ai-review-database-components.md`** - Storage, Transaction, Query, Index, AQL
- **`ai-review-llm-components.md`** - LLM, Embeddings, RAG, Voice, Ethics
- **`ai-review-distributed-systems.md`** - Sharding, Replication, Consensus, CDC
- **`ai-review-network-api.md`** - HTTP, gRPC, WebSocket, MQTT, PostgreSQL Wire

**Was wird überprüft? / What is reviewed?**
- ✅ Best Practices & Code-Qualität / Best practices & code quality
- ✅ Stand der Technik & Forschung / State-of-the-art & research
- ✅ Dokumentation & Lücken / Documentation & gaps
- ✅ Sicherheit & Compliance (BSI C5, ISO 27001, DSGVO, NIS2) / Security & compliance
- ✅ Performance & Optimierung / Performance & optimization
- ✅ Testing & Qualität / Testing & quality
- ✅ Roadmap & Technische Schulden / Roadmap & technical debt

📖 **Siehe:** `_guides/systematic-review-guide.md` für Details / See `_guides/systematic-review-guide.md` for details

### 🔒 Sicherheitsanalyse / Security Analysis

**Templates für systematische Sicherheitsbewertung:**

- **`security-attack-network.md`** - Netzwerk-Angriffsvektoren (HTTP, gRPC, WebSocket, MQTT) / Network attack vectors
- **`security-attack-authentication.md`** - Authentifizierung & Autorisierung / Authentication & authorization
- **`security-attack-injection.md`** - Injection-Angriffe (AQL, NoSQL, Command, LLM Prompt) / Injection attacks
- **`security-attack-cryptography.md`** - Kryptographische Schwachstellen / Cryptographic vulnerabilities
- **`security-attack-distributed.md`** - Verteilte System-Angriffe / Distributed system attacks
- **`security-compliance-investigation.md`** - Compliance-Untersuchung (BSI C5, ISO 27001, DSGVO, NIS2) / Compliance investigation

📖 **Siehe:** `_guides/security-templates-guide.md` für Details / See `_guides/security-templates-guide.md` for details

### 🔬 Forschung & State-of-the-Art / Research & State-of-the-Art

**Templates für Forschungsanalysen:**

- **`research_paper_investigation.md`** - Allgemeine Forschungsuntersuchung / General research investigation
- **`research-gpu-indexing.md`** - GPU-optimierte Indexierung / GPU-optimized indexing
- **`research-learned-indexes.md`** - Gelernte Index-Strukturen / Learned index structures
- **`research-product-quantization.md`** - Product Quantization / Product quantization
- **`research-vector-indexing.md`** - Vektor-Indexierung / Vector indexing

📖 **Siehe:** `_guides/research-templates-guide.md` für Details / See `_guides/research-templates-guide.md` for details

### ⚙️ Implementierungs-Aufgaben / Implementation Tasks

**Spezifische Feature- und Implementierungs-Templates:**

#### Verteilte Systeme / Distributed Systems
- **`task-consensus-implementation.md`** - Konsens-Implementierung (Raft/Paxos/Gossip)
- **`task-transaction-implementation.md`** - Transaktionsprotokolle (2PC/3PC/SAGA)
- **`task-sharding-bug-report.md`** - Sharding-Fehler
- **`task-sharding-feature.md`** - Sharding-Features
- **`task-sharding-performance.md`** - Sharding-Performance

#### Video-Verarbeitung / Video Processing
- **`task-video-batch-processing.md`** - Batch-Verarbeitung
- **`task-video-hardware-acceleration.md`** - Hardware-Beschleunigung
- **`task-video-jpeg-encoding.md`** - JPEG-Kodierung
- **`task-video-multiple-thumbnails.md`** - Mehrere Thumbnails
- **`task-video-scene-detection.md`** - Szenen-Erkennung
- **`task-video-streaming-support.md`** - Streaming-Support
- **`task-video-subtitle-extraction.md`** - Untertitel-Extraktion

#### RoPE (Rotary Position Embeddings)
- **`task-rope-cuda-hip-kernels.md`** - GPU-Beschleunigung (CUDA/HIP)
- **`task-rope-learned-parameters.md`** - Gelernte Parameter
- **`task-rope-lora-integration.md`** - LoRA-Integration
- **`task-rope-rest-api.md`** - REST-API
- **`task-rope-visualization.md`** - Visualisierung

#### Weitere Aufgaben / Other Tasks
- **`task-faiss-migration.md`** - FAISS-Migration
- **`task-ethics-plugin-implementation.md`** - Ethics-Plugin

#### Ethics AI Tasks
- **`ethics-ai-tasks/`** - Spezielle Ethics-Aufgaben / Special ethics tasks

📖 **Siehe:** `_guides/sharding-templates-guide.md` für Sharding-Details / See `_guides/sharding-templates-guide.md` for sharding details

## 🚀 Wie benutze ich die Templates? / How to Use Templates?

### Deutsch:
1. **Gehe zu GitHub Issues** → Neues Issue erstellen
2. **Wähle das passende Template** (siehe Kategorien oben)
3. **Fülle alle Abschnitte aus**
4. **Erstelle das Issue**

**Hilfe bei der Template-Auswahl:**
- User? → `bug_report.md`, `feature_request.md`, oder `documentation_*`
- Komponenten-Review? → `ai-review-*` Templates (siehe Quick Guide)
- Sicherheit? → `security-*` Templates
- Forschung? → `research-*` Templates  
- Feature-Implementierung? → `task-*` Templates

### English:
1. **Go to GitHub Issues** → Create new issue
2. **Select appropriate template** (see categories above)
3. **Fill in all sections**
4. **Create the issue**

**Need help choosing?**
- User? → `bug_report.md`, `feature_request.md`, or `documentation_*`
- Component review? → `ai-review-*` templates (see quick guide)
- Security? → `security-*` templates
- Research? → `research-*` templates
- Feature implementation? → `task-*` templates

## 📚 Dokumentation / Documentation

Alle Guides und Dokumentation befinden sich im **`_guides/`** Unterverzeichnis: / All guides and documentation are in the **`_guides/`** subdirectory:

### Übersichts-Guides / Overview Guides
- **`_guides/templates-overview.md`** - Vollständige Template-Übersicht / Complete template overview
- **`_guides/template-selection-guide.md`** - Template-Auswahlhilfe (English)
- **`_guides/template-auswahl-guide-de.md`** - Template-Auswahlhilfe (Deutsch)

### Detaillierte Guides / Detailed Guides
- **`_guides/systematic-review-guide.md`** - Systematische Review-Anleitung / Systematic review guide
- **`_guides/research-templates-guide.md`** - Forschungs-Templates / Research templates
- **`_guides/security-templates-guide.md`** - Sicherheits-Templates / Security templates
- **`_guides/sharding-templates-guide.md`** - Sharding-Templates / Sharding templates

### Beispiele / Examples
- **`_guides/example-review.md`** - Vollständiges Review-Beispiel / Complete review example
- **`_guides/systematic-investigation-example.md`** - Untersuchungs-Beispiel / Investigation example

### Zusammenfassungen / Summaries
- **`_guides/implementation-summary.md`** - Implementierungsstatus / Implementation status

## 🎯 Quick-Reference: Template-Auswahl / Template Selection

### Nach Komponenten-Pfad / By Component Path

| Komponente / Component | Template |
|------------------------|----------|
| `src/storage/`, `src/transaction/`, `src/query/`, `src/index/`, `src/aql/` | `ai-review-database-components.md` |
| `src/llm/`, `src/embeddings/`, `src/rag/`, `src/voice/`, `src/ethics/` | `ai-review-llm-components.md` |
| `src/sharding/`, `src/replication/`, `src/cdc/` | `ai-review-distributed-systems.md` |
| `src/api/`, `src/network/`, GraphQL plugin | `ai-review-network-api.md` |
| Andere Komponenten / Other components | `ai-review-component-template.md` |

### Nach Zweck / By Purpose

| Zweck / Purpose | Template-Prefix |
|-----------------|-----------------|
| Bug melden / Report bug | `bug_report.md` |
| Feature wünschen / Request feature | `feature_request.md` |
| Komponenten-Review / Component review | `ai-review-*` |
| Sicherheitsanalyse / Security analysis | `security-*` |
| Forschung / Research | `research-*` |
| Implementierung / Implementation | `task-*` |

## 🔄 Empfohlene Review-Häufigkeit / Recommended Review Frequency

- **Quartalsweise / Quarterly** - Kern-Komponenten (Storage, Transaction, Query, Security)
- **Halbjährlich / Bi-annual** - Stabile Komponenten
- **Nach großen Änderungen / After major changes** - Neue Features, Refactoring
- **Vor Releases / Pre-release** - Komponenten mit signifikanten Änderungen

## ✨ Namenskonventionen / Naming Conventions

Templates folgen einem klaren Präfix-System: / Templates follow a clear prefix system:

- **Kein Präfix / No prefix** → User-facing templates
- **`ai-review-`** → Repeatable AI systematic review templates
- **`task-`** → Specific implementation tasks
- **`security-`** → Security analysis templates
- **`research-`** → Research investigation templates

## 📊 Template-Übersicht / Template Overview

| Kategorie / Category | Anzahl / Count | Zweck / Purpose |
|----------------------|----------------|-----------------|
| 👥 User-Facing | 4 | Bugs, Features, Dokumentation |
| 🤖 AI Reviews | 5 | Systematische Komponenten-Reviews |
| 🔒 Security | 6 | Sicherheit & Compliance |
| 🔬 Research | 5 | Forschung & State-of-the-Art |
| ⚙️ Implementation Tasks | 19 | Feature-Implementierungen |
| 📚 Documentation | 11 | Guides & Beispiele (in `_guides/`) |
| **Gesamt / Total** | **50** | |

## 🆕 Neue Features / New Features (2026-02-02)

✅ **Klare Namenskonventionen** mit Präfix-System / Clear naming conventions with prefix system
✅ **Getrennte Dokumentation** im `_guides/` Verzeichnis / Separated documentation in `_guides/` directory
✅ **Verbesserte Beschreibungen** in jedem Template / Enhanced descriptions in each template
✅ **Bilinguale Unterstützung** (Deutsch/English) / Bilingual support (German/English)
✅ **Schnellauswahl-Guide** für einfache Template-Auswahl / Quick selection guide for easy template choice

## ❓ Hilfe benötigt? / Need Help?

- 📖 Lies die Guides in `_guides/` / Read guides in `_guides/`
- 💬 Frage das Technical Lead Team / Ask the Technical Lead team
- 🔍 Siehe Beispiele in `_guides/example-*.md` / See examples in `_guides/example-*.md`

---

**Version:** 2.0.0  
**Letzte Aktualisierung / Last Updated:** 2026-02-02  
**Gepflegt von / Maintained by:** ThemisDB Core Team

**Änderungen / Changes:**
- ✅ Neue Namenskonventionen mit Präfixen / New naming conventions with prefixes
- ✅ Dokumentation nach `_guides/` verschoben / Documentation moved to `_guides/`
- ✅ Verbesserte Template-Beschreibungen / Enhanced template descriptions
- ✅ Klarere Kategorisierung / Clearer categorization
