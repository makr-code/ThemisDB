# Kapitel 42: Dokumentations-Assistent – Verwendung für Admin, Superuser und User

> "KI-gestützte Hilfe: Der integrierte LLM-Dokumentations-Assistent beantwortet Konfigurations- und Troubleshooting-Fragen in Echtzeit."

---

## Überblick

Der ThemisDB Dokumentations-Assistent nutzt llama.cpp und eine vorkompilierte RocksDB-Datenbank mit 1.151 Dokumenten, um Administratoren, Superusern und normalen Benutzern kontextbewusste Hilfe zu bieten. Alle Dokumentationen aus `./docs` und `./compendium` sind über mehrere Datenmodelle indexiert.

**Automatische Konfiguration:**
Der Dokumentations-Assistent findet die Dokumentationsdatenbank automatisch beim Start. Keine manuelle Konfiguration erforderlich!

**Suchpfade (Auto-Discovery):**
1. `data/docs.db` (RocksDB - empfohlen)
2. `data/docs_database.json` (JSON - Fallback)
3. `./docs.db` (aktuelles Verzeichnis)
4. `./docs_database.json`
5. `../data/docs.db` (parent directory)

**Manuelle Konfiguration (optional):**
```yaml
# config/docs_assistant.yaml
docs_assistant:
  enabled: true
  database:
    path: "data/docs.db"  # Expliziter Pfad
    type: "rocksdb"        # json oder rocksdb
    auto_discover: false   # Deaktiviert Auto-Discovery
```

**Zielgruppen:**
- **Administratoren**: Vollzugriff auf Konfiguration, Troubleshooting und Systemoptimierung
- **Superuser**: Erweiterte Abfragen, Performance-Tuning, erweiterte Features
- **User**: Grundlegende Queries, Dokumentationssuche, Anwendungshilfe

**Zugriffsmethoden:**
- REST API (HTTP/1.1)
- AQL-Funktionen (nativ)
- ThemisDB CLI
- C++ SDK

---

```mermaid
graph TB
    subgraph "Benutzertypen"
        Admin[👤 Administrator<br/>Vollzugriff]
        Super[👤 Superuser<br/>Erweitert]
        User[👤 User<br/>Basis]
    end
    
    subgraph "Dokumentations-Datenbank"
        DB[(RocksDB<br/>7 Column Families<br/>1151 Dokumente)]
    end
    
    subgraph "Zugriffsmethoden"
        REST["REST API<br/>/api/v1/llm/docs/*"]
        AQL["AQL Funktionen<br/>DOCS_QUERY()"]
        CLI["ThemisDB CLI<br/>docs-query"]
        SDK["C++ SDK<br/>DocsAssistant"]
    end
    
    subgraph "LLM Engine"
        LLM[llama.cpp<br/>RAG-basiert]
    end
    
    Admin --> REST
    Admin --> AQL
    Admin --> CLI
    Admin --> SDK
    
    Super --> REST
    Super --> AQL
    Super --> CLI
    
    User --> REST
    User --> CLI
    
    REST --> LLM
    AQL --> LLM
    CLI --> LLM
    SDK --> LLM
    
    LLM --> DB
    
    style Admin fill:#ff6b6b
    style Super fill:#ffd93d
    style User fill:#6bcf7f
    style DB fill:#4d96ff
    style LLM fill:#a78bfa
```

Abb. 42.0: Dokumentations-Assistent Architektur nach Benutzerrolle

---

## 42.1 Für Administratoren: Vollständige Systemkontrolle

### 42.1.0 Konfiguration und Auto-Discovery

**Automatische Konfiguration (empfohlen für Entwicklung):**

Der Dokumentations-Assistent kann die Datenbank automatisch beim Server-Start finden, aber für Produktionsumgebungen wird die explizite YAML-Konfiguration empfohlen.

```bash
# Generiere Datenbank
python3 scripts/generate_docs_rocksdb.py --output data/docs.db

# Entwicklung: Auto-Discovery (nur wenn keine YAML-Config vorhanden)
./themis_server

# Produktion: Explizite YAML-Konfiguration (empfohlen!)
# Siehe config/docs_assistant.yaml
```

**Manuelle Konfiguration via YAML (empfohlen für Produktion):**

Die YAML-Konfiguration ist der bevorzugte Weg für Produktionsumgebungen und bietet bessere Kontrolle:

```yaml
# config/docs_assistant.yaml
docs_assistant:
  enabled: true
  
  database:
    path: "/var/lib/themisdb/docs.db"  # Expliziter Pfad (empfohlen!)
    type: "rocksdb"                     # json oder rocksdb
    auto_discover: false                # Deaktiviert Auto-Discovery (sicherer)
  
  llm:
    max_context_docs: 10                # Admin braucht mehr Kontext
    enable_caching: true
    cache_ttl_seconds: 300
  
  access:
    enable_rbac: true
    default_role: "user"

# Server-Log:
# [INFO] Documentation Assistant: Loading database from config: /var/lib/themisdb/docs.db
# [INFO] Documentation Assistant: Loaded 1151 documents from 7 column families
```

**Wichtige Sicherheitshinweise:**

⚠️ **Security Considerations:**
- **Explizite Konfiguration bevorzugen**: Auto-Discovery ist praktisch für Entwicklung, aber in Produktion sollte immer ein expliziter Pfad gesetzt werden
- **Zugriffskontrolle**: Stellen Sie sicher, dass nur autorisierte Benutzer auf docs.db zugreifen können
- **Zukünftige Datenbanken**: Admins können später nutzungsspezifische Datenbanken hinzufügen - siehe TODO für Security-Folgenabschätzung
- **Dateisystemberechtigungen**: `chmod 600 /var/lib/themisdb/docs.db` (nur Owner-Zugriff)

📋 **TODO: Security-Folgenabschätzung**
- Bewertung der Risiken bei mehreren benutzerdefinierten Dokumentationsdatenbanken
- Validierung der Datenbank-Inhalte beim Laden
- Sandbox-Mechanismus für Drittanbieter-Datenbanken
- Audit-Trail für Datenbankzugriffe
- Rate-Limiting pro Datenbank

**Umgebungsvariablen (höchste Priorität):**

```bash
# Überschreibt YAML-Konfiguration
export THEMIS_DOCS_DATABASE_PATH="/custom/path/docs.db"
export THEMIS_DOCS_DATABASE_TYPE="rocksdb"
export THEMIS_DOCS_AUTO_DISCOVER="false"  # Explizit deaktivieren

./themis_server
```

**Konfigurationspriorität:**
1. Umgebungsvariablen (höchste)
2. **YAML-Konfiguration (empfohlen für Produktion)** ⭐
3. Auto-Discovery (nur für Entwicklung)
4. Fallback: `data/docs_database.json`

**Verifizierung:**

```bash
# Prüfe welche Datenbank geladen wurde
curl -X GET http://localhost:8765/api/v1/llm/health \
  -H "Authorization: Bearer ${ADMIN_TOKEN}"

# Antwort:
{
  "status": "healthy",
  "docs_assistant": {
    "enabled": true,
    "database_path": "/var/lib/themisdb/docs.db",
    "database_type": "rocksdb",
    "documents_loaded": 1151,
    "column_families": 7,
    "discovery_method": "yaml_config"  # yaml_config, env, auto, fallback
  }
}
```

**Best Practice: YAML-Konfiguration in Produktion**

```yaml
# config/docs_assistant.yaml - Produktionskonfiguration
docs_assistant:
  enabled: true
  
  # Primäre Dokumentationsdatenbank (System)
  database:
    path: "/var/lib/themisdb/docs.db"
    type: "rocksdb"
    auto_discover: false  # Sicherheit: Explizit deaktiviert
    read_only: true       # Verhindert Änderungen an der Datenbank
  
  # TODO: Zukünftige Feature - Zusätzliche Datenbanken
  # additional_databases:
  #   - name: "custom_docs"
  #     path: "/var/lib/themisdb/custom_docs.db"
  #     enabled: true
  #     requires_role: "administrator"  # Nur für Admins
  #     security_validated: true        # Muss validiert sein
  
  access:
    enable_rbac: true
    require_auth: true
    
  monitoring:
    enable_audit_log: true
    audit_log_path: "/var/log/themisdb/docs_assistant_audit.log"
```

### 42.1.1 Anwendungsfälle für Administratoren

Administratoren benötigen Hilfe bei:
- **Systemkonfiguration**: Sharding, Replikation, Performance-Tuning
- **Troubleshooting**: Fehlerdiagnose, Log-Analyse, Deadlock-Behebung
- **Sicherheit**: RBAC-Setup, Verschlüsselung, HSM-Integration
- **Deployment**: Docker, Kubernetes, Produktionsumgebung
- **Monitoring**: Metrics, Alerts, Observability

### 42.1.2 REST API Zugriff (Admin)

#### Allgemeine Dokumentationsabfrage

```bash
# Admin fragt nach RAID-Sharding-Konfiguration
curl -X POST https://themisdb.example.com:8765/api/v1/llm/docs/query \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer ${ADMIN_TOKEN}" \
  -d '{
    "query": "Wie konfiguriere ich RAID 6 Sharding mit 9 Shards und Replikationsfaktor 6?"
  }'

# Antwort:
{
  "query": "Wie konfiguriere ich RAID 6 Sharding...",
  "answer": "Um RAID 6 Sharding zu konfigurieren:\n\n1. Setzen Sie Umgebungsvariablen:\n   - THEMIS_ENABLE_SHARDING=true\n   - THEMIS_SHARD_COUNT=9\n   - THEMIS_SHARD_REPLICATION_FACTOR=6\n\n2. Konfigurieren Sie config/shards.yaml:\n   shards:\n     - id: 1\n       endpoint: themis-shard-1:8765\n       weight: 1.0\n     - id: 2\n       endpoint: themis-shard-2:8765\n       weight: 1.0\n   [...]\n\n3. Starten Sie den Coordinator:\n   ./themis_server --mode coordinator --config config/shards.yaml\n\nWichtig: RAID 6 benötigt mindestens 4 Shards. Mit 9 Shards und RF=6 haben Sie optimale Fehlertoleranz.\n\nReferenz: docs/en/features/RAID_SHARDING.md",
  "confidence_score": 0.94,
  "documents_searched": 1151,
  "documents_used": 5,
  "search_time_ms": 12,
  "generation_time_ms": 1840,
  "relevant_documents": [
    {
      "file_name": "RAID_SHARDING.md",
      "relevance_score": 0.96,
      "content_preview": "# RAID Sharding in ThemisDB\n\nRAID-style sharding provides data redundancy..."
    }
  ]
}
```

#### Konfigurationshilfe

```bash
# Admin benötigt Hilfe zur Sicherheits-Konfiguration
curl -X POST https://themisdb.example.com:8765/api/v1/llm/docs/config \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer ${ADMIN_TOKEN}" \
  -d '{
    "topic": "security"
  }'

# Antwort:
{
  "topic": "security",
  "configuration_help": "ThemisDB Sicherheits-Konfiguration:\n\n**Basis-Sicherheit:**\n- THEMIS_ENABLE_TLS=true\n- THEMIS_TLS_CERT_PATH=/path/to/cert.pem\n- THEMIS_TLS_KEY_PATH=/path/to/key.pem\n\n**RBAC (Role-Based Access Control):**\n- THEMIS_ENABLE_RBAC=true\n- THEMIS_RBAC_CONFIG=/path/to/rbac.yaml\n\n**Verschlüsselung:**\n- THEMIS_ENABLE_ENCRYPTION_AT_REST=true\n- THEMIS_ENCRYPTION_KEY_PATH=/path/to/master.key\n\n**HSM-Integration (Optional):**\n- THEMIS_ENABLE_HSM_REAL=ON (CMake)\n- THEMIS_HSM_PROVIDER=pkcs11\n- THEMIS_HSM_LIB_PATH=/usr/lib/softhsm/libsofthsm2.so\n\n**Best Practices:**\n1. Verwenden Sie starke TLS-Zertifikate (mind. 2048-bit RSA)\n2. Rotieren Sie Encryption Keys regelmäßig\n3. Aktivieren Sie Audit-Logging\n4. Implementieren Sie Network Policies (Firewall)\n\nReferenzen:\n- docs/en/features/SECURITY.md\n- docs/en/configuration/RBAC_SETUP.md",
  "confidence_score": 0.91,
  "documents_used": 7
}
```

#### Troubleshooting-Hilfe

```bash
# Admin diagnostiziert einen Deadlock
curl -X POST https://themisdb.example.com:8765/api/v1/llm/docs/troubleshoot \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer ${ADMIN_TOKEN}" \
  -d '{
    "error": "Server hängt beim Start bei \"Adaptive Index Manager initialized\". RAID Sharding ist aktiviert."
  }'

# Antwort:
{
  "error": "Server hängt beim Start...",
  "troubleshooting_help": "**Diagnose: RAID Sharding Deadlock (bekannter Bug in v1.3.3)**\n\nUrsache: AdaptiveIndexManager versucht, Column Families zu öffnen, bevor ShardingManager initialisiert ist.\n\n**Lösung:**\n\n1. **Sofort-Fix:** Update auf v1.3.4+\n   ```bash\n   git pull origin main\n   cmake --build build --target themis_server\n   ```\n\n2. **Workaround für v1.3.3:**\n   - Deaktivieren Sie temporär Sharding\n   - Starten Sie Server neu\n   - Aktivieren Sie Sharding wieder\n\n3. **Prüfen Sie docker-compose.yml:**\n   Stellen Sie sicher, dass Port-Mappings korrekt sind:\n   ```yaml\n   ports:\n     - \"8081:8765\"  # NICHT 8081:8080\n   ```\n\n4. **Überprüfen Sie Logs:**\n   ```bash\n   tail -f logs/themis_server.log | grep -i \"shard\\|deadlock\"\n   ```\n\n**Weitere Informationen:**\n- docs/RAID_SHARDING_DEADLOCK_HOTFIX.md\n- GitHub Issue #234",
  "confidence_score": 0.97,
  "documents_used": 3
}
```

### 42.1.3 AQL-Funktionen (Admin)

```aql
-- Admin sucht nach Performance-Tuning-Dokumentation
RETURN DOCS_SEARCH('performance tuning query optimization', 10)

-- Kombinierte Abfrage: Suche + Antwort
LET docs = DOCS_SEARCH('replication setup', 5)
LET answer = DOCS_QUERY('How do I set up multi-master replication?')
RETURN {
    answer: answer,
    sources: docs,
    query_time: DATE_NOW()
}

-- Batch-Abfragen für mehrere Themen
FOR topic IN ['sharding', 'replication', 'security', 'monitoring']
    RETURN {
        topic: topic,
        help: DOCS_CONFIG_HELP(topic)
    }

-- Troubleshooting aus :document Collection
FOR doc IN :document
    FILTER doc.type == 'documentation'
    FILTER CONTAINS(doc.title, 'TROUBLESHOOT') OR CONTAINS(doc.title, 'ERROR')
    RETURN {
        title: doc.title,
        source: doc.source,
        preview: SUBSTRING(doc.content, 0, 300)
    }
    LIMIT 20
```

### 42.1.4 CLI-Zugriff (Admin)

```bash
# Admin verwendet ThemisDB CLI
themis_cli --database themisdb.db --execute "RETURN DOCS_QUERY('How to enable GPU acceleration?')"

# Output:
# ============================================================
# ThemisDB Documentation Assistant
# ============================================================
# Query: How to enable GPU acceleration?
# 
# Answer:
# To enable GPU acceleration in ThemisDB:
# 
# 1. CMake Configuration:
#    cmake -B build -DTHEMIS_ENABLE_GPU=ON -DTHEMIS_ENABLE_CUDA=ON
# 
# 2. Environment Variables:
#    export THEMIS_GPU_DEVICE=0  # GPU device ID
#    export THEMIS_GPU_MEMORY_LIMIT=8GB
# 
# 3. Verify GPU Support:
#    ./themis_server --check-gpu
# 
# Supported Backends:
# - CUDA (NVIDIA)
# - HIP (AMD)
# - Vulkan (Cross-platform)
# 
# References:
# - docs/en/features/GPU_ACCELERATION.md
# - docs/en/configuration/GPU_SETUP.md
# ============================================================

# Interaktiver Modus
themis_cli --database themisdb.db --interactive

themis> RETURN DOCS_QUERY('explain vector embeddings')
themis> RETURN DOCS_CONFIG_HELP('llm')
themis> RETURN DOCS_TROUBLESHOOT('out of memory error')
themis> EXIT
```

### 42.1.5 C++ SDK (Admin)

```cpp
#include "llm/docs_assistant.h"
#include <iostream>

int main() {
    // Initialisiere Dokumentations-Assistenten
    themis::llm::DocsAssistantConfig config;
    config.docs_database_path = "/var/lib/themisdb/docs.db";
    config.database_type = "rocksdb";
    config.max_context_docs = 10;  // Admin braucht mehr Kontext
    config.enable_semantic_search = true;
    config.enable_caching = true;
    
    themis::llm::DocsAssistant assistant(config);
    
    if (!assistant.loadDatabase()) {
        std::cerr << "Failed to load documentation database\n";
        return 1;
    }
    
    // Beispiel 1: Konfigurationshilfe
    auto config_result = assistant.getConfigHelp("sharding");
    std::cout << "Configuration Help:\n"
              << config_result.generated_answer << "\n\n";
    
    // Beispiel 2: Troubleshooting
    auto trouble_result = assistant.getTroubleshootingHelp(
        "Database crashes with segmentation fault on startup"
    );
    std::cout << "Troubleshooting:\n"
              << trouble_result.generated_answer << "\n\n";
    
    // Beispiel 3: Dokumentensuche
    auto search_results = assistant.searchDocs("RAID configuration", 5);
    std::cout << "Found " << search_results.size() << " relevant documents:\n";
    for (const auto& doc : search_results) {
        std::cout << "  - " << doc.file_name 
                  << " (relevance: " << (doc.relevance_score * 100) << "%)\n";
    }
    
    // Beispiel 4: Statistiken
    auto stats = assistant.getStats();
    std::cout << "\nDatabase Statistics:\n"
              << stats.dump(2) << "\n";
    
    return 0;
}
```

---

## 42.2 Für Superuser: Erweiterte Features und Performance

### 42.2.1 Anwendungsfälle für Superuser

Superuser fokussieren sich auf:
- **Query-Optimierung**: EXPLAIN ANALYZE, Index-Tuning
- **Erweiterte Features**: Vector-Suche, Graph-Traversierung, Multi-Model
- **Performance-Analyse**: Profiling, Bottleneck-Identifikation
- **Datenmodellierung**: Best Practices, Schema-Design
- **Batch-Operationen**: Bulk-Insert, ETL-Pipelines

### 42.2.2 REST API Zugriff (Superuser)

```bash
# Superuser fragt nach Query-Optimierung
curl -X POST https://themisdb.example.com:8765/api/v1/llm/docs/query \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer ${SUPERUSER_TOKEN}" \
  -d '{
    "query": "How can I optimize a slow JOIN query between graph and relational collections?"
  }'

# Antwort mit spezifischen Optimierungstipps
{
  "answer": "**Query-Optimierung für Multi-Model JOINs:**\n\n1. **Verwenden Sie sekundäre Indizes:**\n   ```aql\n   -- Erstelle Indizes (via separate CREATE INDEX Statements)\n   -- Index auf users.user_id wird erstellt\n   -- Index auf friends._from wird erstellt\n   ```\n\n2. **Optimieren Sie Join-Reihenfolge:**\n   ```aql\n   -- Schlecht: Graph zuerst, dann Filter\n   FOR user IN users\n     FOR friend IN friends\n       FILTER friend._from == user._id\n       RETURN friend\n   \n   -- Gut: Filter zuerst, dann Join\n   FOR user IN users\n     FILTER user.age > 25\n     FOR friend IN friends\n       FILTER friend._from == user._id\n       RETURN friend\n   ```\n\n3. **Nutzen Sie EXPLAIN:**\n   ```aql\n   EXPLAIN\n   FOR user IN users\n     FOR friend IN friends\n       FILTER friend._from == user._id\n       RETURN {user: user, friend: friend}\n   ```\n\n4. **Verwenden Sie LET für Subqueries:**\n   ```aql\n   FOR user IN users\n     FILTER user.age > 25\n     LET user_friends = (\n       FOR friend IN friends\n         FILTER friend._from == user._id\n         RETURN friend\n     )\n     RETURN {user: user, friends: user_friends}\n   ```\n\n**Performance-Metriken:**\n- Index-Scan: ~1-5ms\n- Full Collection Scan: ~50-500ms\n- Optimized Join: ~10-50ms\n- Unoptimized Join: ~500-5000ms\n\nReferenzen:\n- docs/en/features/QUERY_OPTIMIZATION.md\n- compendium/chapter_34_query_optimization.md",
  "confidence_score": 0.89,
  "documents_used": 6
}
```

### 42.2.3 AQL-Funktionen (Superuser)

```aql
-- Superuser analysiert Performance-Dokumentation
FOR doc IN :document
    FILTER doc.type == 'documentation'
    FILTER CONTAINS(doc.content, 'performance') OR CONTAINS(doc.content, 'optimization')
    LET relevance = (
        LENGTH(SPLIT(LOWER(doc.content), 'performance')) +
        LENGTH(SPLIT(LOWER(doc.content), 'optimization')) +
        LENGTH(SPLIT(LOWER(doc.content), 'index'))
    )
    SORT relevance DESC
    RETURN {
        title: doc.title,
        source: doc.source,
        relevance: relevance,
        preview: SUBSTRING(doc.content, 0, 200)
    }
    LIMIT 15

-- Kombinierte Suche mit Aggregation
LET vector_docs = DOCS_SEARCH('vector search embeddings', 5)
LET graph_docs = DOCS_SEARCH('graph traversal', 5)
LET relational_docs = DOCS_SEARCH('relational queries', 5)
RETURN {
    vector: vector_docs,
    graph: graph_docs,
    relational: relational_docs,
    total: LENGTH(vector_docs) + LENGTH(graph_docs) + LENGTH(relational_docs)
}
```

### 42.2.4 Performance-Analyse mit Docs-Assistent

```bash
# Superuser fragt nach Profiling-Tools
curl -X POST https://themisdb.example.com:8765/api/v1/llm/docs/query \
  -H "Authorization: Bearer ${SUPERUSER_TOKEN}" \
  -d '{
    "query": "What profiling tools are available for performance analysis?"
  }'

# CLI-basierte Performance-Analyse mit AQL
themis_cli --database themisdb.db \
    --execute "RETURN DOCS_QUERY('explain PROFILE and EXPLAIN in AQL')"
```

---

## 42.3 Für User: Grundlegende Dokumentationssuche

### 42.3.1 Anwendungsfälle für User

Normale User benötigen Hilfe bei:
- **Basis-Queries**: Einfache FOR...FILTER...RETURN
- **Datentypen**: JSON, Arrays, Nested Objects
- **Built-in Funktionen**: String, Math, Date/Time
- **Fehlerbehandlung**: Verständliche Fehlermeldungen
- **API-Nutzung**: HTTP Endpoints, Request/Response-Format

### 42.3.2 REST API Zugriff (User)

```bash
# User fragt nach einfacher Query-Syntax
curl -X POST https://themisdb.example.com:8765/api/v1/llm/docs/query \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer ${USER_TOKEN}" \
  -d '{
    "query": "How do I select all users older than 30?"
  }'

# Antwort mit einfachem Beispiel
{
  "answer": "Um alle Benutzer über 30 zu selektieren, verwenden Sie:\n\n```aql\nFOR user IN users\n    FILTER user.age > 30\n    RETURN user\n```\n\n**Erklärung:**\n- `FOR user IN users` iteriert über die 'users' Collection\n- `FILTER user.age > 30` filtert Benutzer mit age > 30\n- `RETURN user` gibt die gefilterten Benutzer zurück\n\n**Beispiel-Ausgabe:**\n```json\n[\n  {\"_key\": \"user1\", \"name\": \"Alice\", \"age\": 35},\n  {\"_key\": \"user2\", \"name\": \"Bob\", \"age\": 42}\n]\n```\n\nSie können auch spezifische Felder zurückgeben:\n```aql\nFOR user IN users\n    FILTER user.age > 30\n    RETURN {name: user.name, age: user.age}\n```",
  "confidence_score": 0.95,
  "documents_used": 3
}
```

### 42.3.3 CLI-Zugriff (User)

```bash
# User verwendet vereinfachte CLI-Befehle
themis_cli --docs-search "how to insert data"

# Output:
# ============================================================
# Documentation Search Results
# ============================================================
# Found 8 relevant documents:
# 
# 1. GETTING_STARTED.md (relevance: 87%)
#    "Getting Started with ThemisDB... To insert data, use FOR...INSERT..."
# 
# 2. AQL_BASICS.md (relevance: 82%)
#    "AQL Basics... FOR doc IN [{name: 'Alice', age: 30}] INSERT doc INTO users..."
# 
# 3. REST_API.md (relevance: 75%)
#    "REST API Documentation... POST /api/v1/query with AQL statement..."
# 
# [...]
# ============================================================

# User stellt Follow-up-Frage
themis_cli --execute "RETURN DOCS_QUERY('show me an example of inserting multiple records')"
```

### 42.3.4 Web-Interface (User) - Geplant

```html
<!-- Geplantes Feature: Web-basiertes Docs-Interface -->
<div class="themis-docs-assistant">
    <input type="text" 
           placeholder="Frage zur ThemisDB-Dokumentation..." 
           id="docsQuery">
    <button onclick="queryDocs()">Suchen</button>
    
    <div id="docsResults">
        <!-- Antwort wird hier angezeigt -->
    </div>
</div>

<script>
async function queryDocs() {
    const query = document.getElementById('docsQuery').value;
    const response = await fetch('/api/v1/llm/docs/query', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json',
            'Authorization': 'Bearer ' + getUserToken()
        },
        body: JSON.stringify({ query })
    });
    
    const result = await response.json();
    document.getElementById('docsResults').innerHTML = 
        `<div class="answer">${result.answer}</div>`;
}
</script>
```

---

## 42.4 Berechtigungsmodell und Zugriffskontrolle

### 42.4.0 YAML-Konfiguration - Detaillierte Dokumentation

**Vollständige config/docs_assistant.yaml mit allen Optionen:**

```yaml
# ============================================================
# ThemisDB Documentation Assistant - Vollständige Konfiguration
# ============================================================
# Empfohlen für Produktionsumgebungen
# Auto-Discovery ist nur für Entwicklung gedacht

docs_assistant:
  # Grundeinstellungen
  enabled: true
  
  # ============================================================
  # Datenbank-Konfiguration (KRITISCH)
  # ============================================================
  database:
    # Pfad zur Dokumentationsdatenbank
    # SICHERHEIT: Immer explizit setzen in Produktion!
    path: "/var/lib/themisdb/docs.db"
    
    # Datenbanktyp: "rocksdb" oder "json"
    type: "rocksdb"
    
    # Auto-Discovery deaktivieren (empfohlen für Produktion)
    # Verhindert automatisches Laden von unbekannten Datenbanken
    auto_discover: false
    
    # Read-Only-Modus (empfohlen)
    # Verhindert versehentliche Änderungen an der Datenbank
    read_only: true
    
    # Validierung beim Laden
    validate_on_load: true
    
    # Maximale Datenbankgröße (MB)
    max_size_mb: 500
    
    # TODO: Zusätzliche Datenbanken (zukünftiges Feature)
    # Erfordert Security-Folgenabschätzung vor Implementierung
    # additional_databases:
    #   - name: "custom_legal_docs"
    #     path: "/var/lib/themisdb/legal_docs.db"
    #     type: "rocksdb"
    #     enabled: true
    #     requires_role: "administrator"
    #     security_validated: true
    #     checksum: "sha256:abc123..."
    #     signed_by: "security@example.com"
  
  # ============================================================
  # LLM-Konfiguration
  # ============================================================
  llm:
    # Modell-ID (leer = Standardmodell verwenden)
    model_id: ""
    
    # Maximale Anzahl Dokumente im RAG-Kontext
    max_context_docs: 5
    
    # Zeichen pro Dokument im Kontext
    context_preview_length: 1000
    
    # Semantische Suche aktivieren (benötigt Vector Embeddings)
    enable_semantic_search: true
    
    # Response-Caching
    enable_caching: true
    cache_ttl_seconds: 300
  
  # ============================================================
  # Zugriffskontrolle (RBAC)
  # ============================================================
  access:
    # RBAC aktivieren
    enable_rbac: true
    
    # Standardrolle wenn nicht authentifiziert
    default_role: "anonymous"
    
    # Authentifizierung erzwingen
    require_auth: true
    
    # Rollenbasierte Limits (siehe 42.4.1)
    roles:
      administrator:
        requests_per_minute: 100
        max_context_docs: 10
        cache_ttl_seconds: 300
        allowed_endpoints:
          - query
          - search
          - config
          - troubleshoot
          - stats
        # TODO: Zugriff auf zusätzliche Datenbanken
        additional_databases_access: true
      
      superuser:
        requests_per_minute: 50
        max_context_docs: 7
        cache_ttl_seconds: 600
        allowed_endpoints:
          - query
          - search
          - config  # read-only
      
      user:
        requests_per_minute: 20
        max_context_docs: 3
        cache_ttl_seconds: 900
        allowed_endpoints:
          - query
          - search
      
      anonymous:
        requests_per_minute: 5
        max_context_docs: 1
        cache_ttl_seconds: 1800
        allowed_endpoints:
          - search  # nur Suche, keine LLM-Queries
  
  # ============================================================
  # REST API
  # ============================================================
  api:
    enabled: true
    base_path: "/api/v1/llm/docs"
    require_auth: true
    enable_cors: true
    cors_origins:
      - "https://app.example.com"
      - "https://admin.example.com"
  
  # ============================================================
  # AQL-Funktionen
  # ============================================================
  aql:
    enabled: true
    function_prefix: "DOCS_"
    functions:
      - QUERY        # DOCS_QUERY()
      - SEARCH       # DOCS_SEARCH()
      - CONFIG_HELP  # DOCS_CONFIG_HELP()
      - TROUBLESHOOT # DOCS_TROUBLESHOOT()
  
  # ============================================================
  # Monitoring und Audit
  # ============================================================
  monitoring:
    # Prometheus-Metriken
    enable_metrics: true
    metrics_prefix: "themis_docs_"
    
    # Audit-Logging (WICHTIG für Compliance)
    enable_audit_log: true
    audit_log_path: "/var/log/themisdb/docs_assistant_audit.log"
    
    # Was wird geloggt
    log_queries: false  # WARNUNG: Kann sensible Daten enthalten
    log_failed_queries: true
    log_access_denied: true
    log_database_access: true
  
  # ============================================================
  # Performance
  # ============================================================
  performance:
    worker_threads: 4
    search_timeout_ms: 5000
    generation_timeout_ms: 30000
    enable_async: true
    async_queue_size: 100

# ============================================================
# TODO: Security-Folgenabschätzung
# ============================================================
# Bevor zusätzliche Datenbanken implementiert werden:
# 
# 1. Risikobewertung:
#    - Kann Admin bösartige Datenbanken einhängen?
#    - Sandboxing-Mechanismus erforderlich?
#    - Validierung der Datenbank-Inhalte?
#
# 2. Authentifizierung:
#    - Digitale Signaturen für Datenbanken
#    - Checksummen-Validierung
#    - Whitelisting von Datenbankquellen
#
# 3. Zugriffskontrolle:
#    - Welche Rollen dürfen Datenbanken hinzufügen?
#    - Separate Berechtigungen pro Datenbank
#    - Isolation zwischen Datenbanken
#
# 4. Audit und Compliance:
#    - Vollständiger Audit-Trail
#    - Wer hat wann welche Datenbank hinzugefügt?
#    - Welche Abfragen wurden auf welcher Datenbank ausgeführt?
#
# 5. Rate-Limiting:
#    - Pro Datenbank getrennte Limits?
#    - Globale Limits über alle Datenbanken?
#
# 6. Datenbank-Validierung:
#    - Schema-Validierung
#    - Content-Filtering (keine Malware, Scripts, etc.)
#    - Maximale Größenbeschränkungen
#
# Siehe: docs/en/security/DOCS_ASSISTANT_SECURITY_ASSESSMENT.md
```

**Umgebungsvariablen-Überschreibung:**

```bash
# Alle YAML-Optionen können via Env-Vars überschrieben werden
export THEMIS_DOCS_ENABLED=true
export THEMIS_DOCS_DATABASE_PATH="/var/lib/themisdb/docs.db"
export THEMIS_DOCS_DATABASE_TYPE="rocksdb"
export THEMIS_DOCS_AUTO_DISCOVER=false
export THEMIS_DOCS_READ_ONLY=true
export THEMIS_DOCS_REQUIRE_AUTH=true
export THEMIS_DOCS_ENABLE_AUDIT_LOG=true
export THEMIS_DOCS_AUDIT_LOG_PATH="/var/log/themisdb/audit.log"

./themis_server
```

**Deployment-Szenarien:**

**Szenario 1: Entwicklung (lokale Maschine)**
```yaml
docs_assistant:
  enabled: true
  database:
    path: "data/docs.db"
    auto_discover: true  # OK für Entwicklung
  access:
    require_auth: false  # Nur für lokale Entwicklung!
```

**Szenario 2: Staging (Test-Umgebung)**
```yaml
docs_assistant:
  enabled: true
  database:
    path: "/opt/themisdb/docs.db"
    auto_discover: false
    read_only: true
  access:
    require_auth: true
    enable_rbac: true
  monitoring:
    enable_audit_log: true
```

**Szenario 3: Produktion (empfohlen)**
```yaml
docs_assistant:
  enabled: true
  database:
    path: "/var/lib/themisdb/docs.db"
    type: "rocksdb"
    auto_discover: false  # KRITISCH!
    read_only: true       # KRITISCH!
    validate_on_load: true
  access:
    require_auth: true
    enable_rbac: true
  monitoring:
    enable_audit_log: true
    audit_log_path: "/var/log/themisdb/docs_assistant_audit.log"
    log_access_denied: true
    log_database_access: true
```

### 42.4.1 RBAC-Integration

```yaml
# config/rbac_docs_assistant.yaml
roles:
  administrator:
    permissions:
      - docs:query:*
      - docs:search:*
      - docs:config:*
      - docs:troubleshoot:*
      - docs:stats:*
    rate_limit:
      requests_per_minute: 100
      max_context_docs: 10
  
  superuser:
    permissions:
      - docs:query:*
      - docs:search:*
      - docs:config:read
    rate_limit:
      requests_per_minute: 50
      max_context_docs: 7
  
  user:
    permissions:
      - docs:query:basic
      - docs:search:*
    rate_limit:
      requests_per_minute: 20
      max_context_docs: 3
    restrictions:
      - no_system_config_docs
      - no_internal_implementation_docs
```

### 42.4.2 API-Key-Basierte Authentifizierung

```bash
# Admin erstellt API Keys für verschiedene Rollen
themis_cli --create-api-key \
    --role administrator \
    --name "admin-docs-key" \
    --expires 90d

# Output:
# API Key created: themis_admin_abc123xyz...
# Role: administrator
# Expires: 2026-04-11

# User verwendet den API Key
curl -X POST https://themisdb.example.com:8765/api/v1/llm/docs/query \
  -H "X-API-Key: themis_user_def456uvw..." \
  -d '{"query": "how to filter data?"}'
```

### 42.4.3 Audit-Logging

```cpp
// Docs-Assistent loggt alle Anfragen
struct DocsAuditLog {
    std::string timestamp;
    std::string user_id;
    std::string role;
    std::string query;
    std::string endpoint;  // "query", "search", "config", "troubleshoot"
    int documents_accessed;
    float response_time_ms;
    bool success;
};

// Beispiel-Log-Eintrag
{
    "timestamp": "2026-01-11T09:30:45Z",
    "user_id": "admin@example.com",
    "role": "administrator",
    "query": "How to configure RAID sharding?",
    "endpoint": "config",
    "documents_accessed": 5,
    "response_time_ms": 1840.5,
    "success": true
}
```

---

## 42.5 Rate-Limiting und Resource-Management

### 42.5.1 Rate-Limits nach Rolle

| Rolle | Requests/Min | Max Context Docs | Cache-TTL | Priority |
|-------|--------------|------------------|-----------|----------|
| Administrator | 100 | 10 | 5 min | High |
| Superuser | 50 | 7 | 10 min | Medium |
| User | 20 | 3 | 15 min | Low |
| Anonymous | 5 | 1 | 30 min | Lowest |

### 42.5.2 Konfiguration

```yaml
# config/docs_assistant.yaml
rate_limiting:
  enabled: true
  backend: redis  # oder "memory"
  
  roles:
    administrator:
      requests_per_minute: 100
      burst: 20
      max_concurrent: 10
    
    superuser:
      requests_per_minute: 50
      burst: 10
      max_concurrent: 5
    
    user:
      requests_per_minute: 20
      burst: 5
      max_concurrent: 2

resource_limits:
  max_context_docs:
    administrator: 10
    superuser: 7
    user: 3
  
  llm_timeout_seconds:
    administrator: 30
    superuser: 20
    user: 10
  
  cache_ttl_seconds:
    administrator: 300
    superuser: 600
    user: 900
```

---

## 42.6 Best Practices und Tipps

### 42.6.1 Für Administratoren

✅ **DOs:**
- Nutzen Sie `DOCS_CONFIG_HELP()` für schnelle Konfigurationsreferenzen
- Kombinieren Sie Docs-Assistent mit `EXPLAIN ANALYZE` für Troubleshooting
- Cachen Sie häufige Queries mit `enable_caching=true`
- Verwenden Sie spezifische Queries statt generische Fragen

❌ **DON'Ts:**
- Verlassen Sie sich nicht ausschließlich auf den Assistenten für kritische Entscheidungen
- Vermeiden Sie extrem lange Queries (>500 Zeichen)
- Ignorieren Sie nicht die Konfidenz-Scores (<0.5 = unsicher)

### 42.6.2 Für Superuser

✅ **DOs:**
- Nutzen Sie `DOCS_SEARCH()` für gezielte Dokumentensuche
- Kombinieren Sie mit `FOR ... IN :document` für Custom-Analysen
- Verwenden Sie Batch-Queries für multiple Topics

❌ **DON'Ts:**
- Überschreiten Sie nicht Ihre Rate-Limits (50 req/min)
- Vermeiden Sie redundante Queries (nutzen Sie Cache)

### 42.6.3 Für User

✅ **DOs:**
- Formulieren Sie klare, spezifische Fragen
- Nutzen Sie die CLI für schnelle Lookups
- Prüfen Sie `relevant_documents` für zusätzliche Infos

❌ **DON'Ts:**
- Stellen Sie keine Fragen zu internen Implementierungen
- Überschreiten Sie nicht Ihre Rate-Limits (20 req/min)

---

## 42.7 Troubleshooting und Häufige Probleme

### 42.7.1 Docs-Datenbank nicht gefunden

**Problem:**
```
Error: Documentation database not loaded. Please ensure docs_database.json is available.
```

**Lösung (Admin):**
```bash
# 1. Generiere Dokumentations-Datenbank
python3 scripts/generate_docs_rocksdb.py --output data/docs.db

# 2. Verifiziere Datenbank
ls -lh data/docs.db
# Erwartete Größe: ~2-3 MB

# 3. Setze Umgebungsvariable
export THEMIS_DOCS_DATABASE_PATH=/var/lib/themisdb/data/docs.db
export THEMIS_DOCS_DATABASE_TYPE=rocksdb

# 4. Starte Server neu
systemctl restart themisdb
```

### 42.7.2 Rate-Limit überschritten

**Problem:**
```json
{
  "error": "Rate limit exceeded",
  "message": "You have exceeded 20 requests per minute. Please wait 45 seconds.",
  "retry_after": 45
}
```

**Lösung (User):**
```bash
# Warte bis Rate-Limit zurückgesetzt wird
sleep 45

# Oder: Verwende Caching
curl -X POST /api/v1/llm/docs/query \
  -H "X-Use-Cache: true" \
  -d '{"query": "..."}'
```

### 42.7.3 Niedrige Konfidenz-Scores

**Problem:**
```json
{
  "answer": "...",
  "confidence_score": 0.32
}
```

**Lösung:**
```bash
# Verbessere Query-Formulierung
# Schlecht: "wie geht das?"
# Gut: "How do I configure RAID 6 sharding with 9 shards?"

# Nutze DOCS_SEARCH() für manuelle Dokumentensuche
themis_cli --execute "RETURN DOCS_SEARCH('RAID sharding configuration', 10)"
```

---

## 42.8 Monitoring und Metriken

### 42.8.1 Prometheus-Metriken

```promql
# Docs-Assistent Request-Rate
rate(themis_docs_requests_total[5m])

# Durchschnittliche Response-Zeit
histogram_quantile(0.95, themis_docs_response_time_seconds_bucket)

# Konfidenz-Score-Verteilung
histogram_quantile(0.5, themis_docs_confidence_score_bucket)

# Cache-Hit-Rate
rate(themis_docs_cache_hits_total[5m]) / rate(themis_docs_requests_total[5m])
```

### 42.8.2 Grafana-Dashboard

```yaml
# grafana/dashboards/docs_assistant.json
{
  "title": "ThemisDB Docs Assistant",
  "panels": [
    {
      "title": "Requests by Role",
      "targets": [
        {
          "expr": "sum(rate(themis_docs_requests_total[5m])) by (role)"
        }
      ]
    },
    {
      "title": "Response Time (p95)",
      "targets": [
        {
          "expr": "histogram_quantile(0.95, themis_docs_response_time_seconds_bucket)"
        }
      ]
    }
  ]
}
```

---

## 42.9 Migration und Upgrades

### 42.9.1 Upgrade von JSON zu RocksDB

```bash
# Alte JSON-basierte Datenbank
# Größe: 4.2 MB
# Load-Zeit: 42 ms

# Neue RocksDB-basierte Datenbank
python3 scripts/generate_docs_rocksdb.py --output data/docs.db

# Größe: 2.8 MB (-33%)
# Load-Zeit: 7 ms (-83%)

# Update Konfiguration
# config/themis.yaml
llm:
  docs_assistant:
    database_type: rocksdb  # war: json
    database_path: data/docs.db  # war: data/docs_database.json
```

### 42.9.2 Dokumentations-Update

```bash
# Nach Änderungen an ./docs oder ./compendium
cd /path/to/ThemisDB

# Regeneriere Datenbank
python3 scripts/generate_docs_database.py
python3 scripts/generate_docs_rocksdb.py

# Restart Server (Hot-Reload nicht unterstützt)
systemctl restart themisdb

# Verifiziere neue Dokumente
themis_cli --execute "
  FOR meta IN docs_metadata
    FILTER meta.key == 'generation_time'
    RETURN meta.value
"
```

---

## 42.9.3 FAQ: Typische Fragen und erwartete Antworten

Diese FAQ zeigt realistische Beispiele für Fragen, die an den Dokumentations-Assistenten gestellt werden können, und die zu erwartenden Antworten.

### 42.9.3.1 Konfiguration & Setup

**Frage 1: "Wie aktiviere ich Sharding in ThemisDB?"**

```aql
RETURN DOCS_QUERY('Wie aktiviere ich Sharding in ThemisDB?')
```

**Erwartete Antwort:**
```
Um Sharding in ThemisDB zu aktivieren:

1. Konfiguration in config/themis.yaml:
   sharding:
     enabled: true
     mode: "consistent_hash"  # oder "range", "round_robin"
     shards: 3
     replication_factor: 2

2. Shard-Keys definieren:
   CREATE COLLECTION users SHARD_KEY user_id;

3. Server neu starten:
   systemctl restart themisdb

4. Verifizierung:
   RETURN CURRENT_SHARDING_TOPOLOGY()

Weitere Details: Kapitel 16 (Sharding & Partitioning)
Verwandte Themen: RAID-Konfiguration, Replication, Load-Balancing
```

---

**Frage 2: "Was sind die Hardware-Mindestanforderungen für ThemisDB?"**

```aql
RETURN DOCS_CONFIG_HELP('hardware requirements')
```

**Erwartete Antwort:**
```
ThemisDB Hardware-Anforderungen:

MINIMUM (Development):
- CPU: 2 Cores (x86_64 oder ARM64)
- RAM: 4 GB
- Disk: 20 GB SSD
- Netzwerk: 100 Mbit/s

EMPFOHLEN (Production):
- CPU: 8+ Cores, AVX2-Support
- RAM: 32 GB+
- Disk: 500 GB NVMe SSD (RAID 10)
- Netzwerk: 1 Gbit/s+

ENTERPRISE (High-Performance):
- CPU: 32+ Cores, AVX-512
- RAM: 128 GB+
- Disk: 2 TB+ NVMe SSD (RAID 10)
- Netzwerk: 10 Gbit/s+
- GPU: NVIDIA A100 (optional für ML)

Quelle: Kapitel 4 (Installation), Kapitel 25 (DevOps & Infrastructure)
```

---

**Frage 3: "Wie konfiguriere ich SSL/TLS für sichere Verbindungen?"**

```aql
RETURN DOCS_CONFIG_HELP('SSL/TLS configuration')
```

**Erwartete Antwort:**
```
SSL/TLS-Konfiguration für ThemisDB:

1. Zertifikate generieren:
   openssl req -x509 -newkey rsa:4096 -keyout key.pem -out cert.pem -days 365

2. config/themis.yaml anpassen:
   server:
     ssl:
       enabled: true
       cert_file: "/path/to/cert.pem"
       key_file: "/path/to/key.pem"
       ca_file: "/path/to/ca.pem"  # optional
       min_tls_version: "1.2"
       ciphers: "HIGH:!aNULL:!MD5"

3. Client-Konfiguration:
   themis_cli --ssl --ssl-cert /path/to/cert.pem

4. Verifizierung:
   curl https://localhost:8765/health --cacert ca.pem

Weitere Details: Kapitel 21 (Authentifizierung & Autorisierung)
Verwandte Themen: mTLS, Certificate Rotation, LDAP-Integration
```

---

### 42.9.3.2 Troubleshooting & Performance

**Frage 4: "Server startet nicht - 'Port already in use' Fehler"**

```aql
RETURN DOCS_TROUBLESHOOT('Server startet nicht - Port already in use')
```

**Erwartete Antwort:**
```
Lösung für 'Port already in use' Fehler:

DIAGNOSE:
1. Prüfen, welcher Prozess Port 8765 nutzt:
   sudo lsof -i :8765
   # oder
   sudo netstat -tulpn | grep 8765

LÖSUNGEN:
A) Prozess beenden:
   sudo kill -9 <PID>

B) Anderen Port konfigurieren (config/themis.yaml):
   server:
     port: 8766  # Alternativer Port

C) Zombie-Prozesse bereinigen:
   pkill -9 themisdb
   systemctl reset-failed themisdb

PRÄVENTION:
- Graceful Shutdown verwenden: systemctl stop themisdb
- Startup-Script mit Port-Check ergänzen
- Monitoring für Port-Konflikte einrichten

Verwandte Dokumente: Kapitel 27 (Troubleshooting), Appendix I
```

---

**Frage 5: "Warum sind meine Queries so langsam?"**

```aql
RETURN DOCS_TROUBLESHOOT('Queries sind langsam - Performance-Probleme')
```

**Erwartete Antwort:**
```
Performance-Optimierung für langsame Queries:

DIAGNOSE:
1. Query-Profiling aktivieren:
   EXPLAIN <your_query>

2. Metriken prüfen:
   curl http://localhost:8765/metrics | grep query_duration

HÄUFIGE URSACHEN & LÖSUNGEN:

A) Fehlende Indizes:
   - Problem: Full-Table-Scans
   - Lösung: CREATE INDEX idx_name ON collection(field)

B) Suboptimale Query-Struktur:
   - Problem: Nested loops ohne LIMIT
   - Lösung: LIMIT hinzufügen, Filter optimieren

C) Große Datenmengen ohne Paging:
   - Problem: Millionen Rows im Result-Set
   - Lösung: LIMIT + OFFSET verwenden (Batch-Processing)

D) Unzureichende Hardware:
   - Problem: RAM < Arbeitsspeicherbedarf
   - Lösung: RAM erhöhen, Caching aktivieren

E) Fragmentierung:
   - Problem: Viele Updates/Deletes
   - Lösung: COMPACT_COLLECTION()

QUICK-WINS:
- Indizes auf Filter-Feldern: +80% schneller
- LIMIT 1000 statt unbegrenzt: +95% schneller
- Parallele Queries aktivieren: +60% schneller

Details: Kapitel 34 (Query Optimization), Kapitel 39 (Performance Tuning)
```

---

**Frage 6: "Out of Memory - Server crashed"**

```aql
RETURN DOCS_TROUBLESHOOT('Out of Memory Error - Server crashed')
```

**Erwartete Antwort:**
```
Lösung für Out-of-Memory (OOM) Fehler:

SOFORTMASSNAHMEN:
1. Server neustarten:
   systemctl restart themisdb

2. Memory-Limit erhöhen (config/themis.yaml):
   system:
     max_memory: "16GB"  # erhöhen von z.B. 8GB
     max_query_memory: "2GB"

LANGFRISTIGE LÖSUNGEN:

A) Memory-Profiling:
   - Aktivieren: monitoring.memory_profiling = true
   - Analyse: curl http://localhost:8765/debug/memory

B) Query-Optimierungen:
   - Große Result-Sets vermeiden (LIMIT verwenden)
   - Streaming für große Datenmengen aktivieren
   - Materialized Views für häufige Aggregationen

C) Cache-Konfiguration anpassen:
   cache:
     query_cache_size: "4GB"  # reduzieren wenn nötig
     block_cache_size: "8GB"

D) Hardware-Upgrade:
   - RAM verdoppeln (z.B. 32GB → 64GB)
   - Swap-Space aktivieren (nur als Fallback)

MONITORING:
- Grafana Dashboard: Memory Usage Alert bei >85%
- Log-Überwachung: grep "out of memory" /var/log/themisdb/*

Details: Kapitel 19 (Monitoring), Kapitel 27 (Troubleshooting)
```

---

### 42.9.3.3 Datenmodellierung & AQL

**Frage 7: "Wie modelliere ich Many-to-Many Beziehungen?"**

```aql
RETURN DOCS_QUERY('Wie modelliere ich Many-to-Many Beziehungen in ThemisDB?')
```

**Erwartete Antwort:**
```
Many-to-Many Beziehungen in ThemisDB modellieren:

METHODE 1: Edge-Collection (Graph-Modell - empfohlen)

1. Collections erstellen:
   CREATE COLLECTION users;
   CREATE COLLECTION projects;
   CREATE EDGE COLLECTION user_projects;

2. Daten einfügen:
   INSERT {_key: "u1", name: "Alice"} INTO users;
   INSERT {_key: "p1", name: "Project A"} INTO projects;
   INSERT {_from: "users/u1", _to: "projects/p1", role: "lead"} 
     INTO user_projects;

3. Abfragen:
   FOR u IN users
     FOR proj IN OUTBOUND u user_projects
       RETURN {user: u.name, project: proj.name}

METHODE 2: Junction-Table (Relational-Modell)

1. Collections:
   CREATE COLLECTION users;
   CREATE COLLECTION projects;
   CREATE COLLECTION user_project_junction;

2. Daten:
   INSERT {user_id: "u1", project_id: "p1", role: "lead"} 
     INTO user_project_junction;

3. Abfragen mit JOIN:
   FOR uj IN user_project_junction
     LET u = DOCUMENT(CONCAT("users/", uj.user_id))
     LET p = DOCUMENT(CONCAT("projects/", uj.project_id))
     RETURN {user: u.name, project: p.name, role: uj.role}

BEST PRACTICE: Graph-Modell für:
- Häufige Traversierungen
- Komplexe Beziehungen (>2 Entitäten)
- Pfad-Abfragen (SHORTEST_PATH)

Details: Kapitel 6 (Graph-Modell), Kapitel 35 (Data Modeling Patterns)
```

---

**Frage 8: "Wie verwende ich Vector-Embeddings für Ähnlichkeitssuche?"**

```aql
RETURN DOCS_QUERY('Wie verwende ich Vector-Embeddings für Ähnlichkeitssuche?')
```

**Erwartete Antwort:**
```
Vector-Embeddings für Ähnlichkeitssuche in ThemisDB:

SETUP:

1. Collection mit Vector-Index:
   CREATE COLLECTION documents;
   CREATE VECTOR INDEX vec_idx ON documents(embedding) 
     DIMENSION 768 METRIC cosine;

2. Embeddings generieren (z.B. mit Sentence-Transformers):
   import sentence_transformers
   model = sentence_transformers.SentenceTransformer('all-MiniLM-L6-v2')
   embedding = model.encode("Your text here").tolist()

3. Dokument mit Embedding einfügen:
   INSERT {
     text: "ThemisDB is a multi-model database",
     embedding: [0.12, -0.45, 0.78, ...]  // 768 Dimensionen
   } INTO documents;

QUERIES:

A) K-Nearest-Neighbors (KNN):
   LET query_vec = [0.15, -0.42, 0.81, ...]
   FOR doc IN documents
     SORT COSINE_SIMILARITY(doc.embedding, query_vec) DESC
     LIMIT 10
     RETURN {text: doc.text, score: COSINE_SIMILARITY(doc.embedding, query_vec)}

B) Similarity-Threshold:
   FOR doc IN documents
     LET sim = COSINE_SIMILARITY(doc.embedding, @query_embedding)
     FILTER sim > 0.85
     RETURN {text: doc.text, similarity: sim}

C) Hybrid-Search (Text + Vector):
   FOR doc IN FULLTEXT(documents, "database", 0.8)
     LET vec_sim = COSINE_SIMILARITY(doc.embedding, @vec)
     LET combined_score = 0.7 * doc.score + 0.3 * vec_sim
     SORT combined_score DESC
     LIMIT 10
     RETURN doc

METRIKEN:
- cosine: Für normalisierte Vektoren (Standard)
- euclidean: Für nicht-normalisierte Vektoren
- dot_product: Für Performance-kritische Anwendungen

Details: Kapitel 8 (Vector-Modell), Kapitel 16 (ML-Integration)
```

---

### 42.9.3.4 Sicherheit & Compliance

**Frage 9: "Wie implementiere ich Row-Level-Security?"**

```aql
RETURN DOCS_CONFIG_HELP('Row-Level Security implementation')
```

**Erwartete Antwort:**
```
Row-Level-Security (RLS) in ThemisDB implementieren:

METHODE 1: AQL-basierte Policy (empfohlen)

1. Security-Funktion definieren:
   CREATE FUNCTION can_access_row(doc, user)
     RETURN (
       doc.owner_id == user.id OR
       user.role == "admin" OR
       doc.public == true
     );

2. In Queries verwenden:
   FOR doc IN documents
     FILTER can_access_row(doc, CURRENT_USER())
     RETURN doc

3. View mit Policy:
   CREATE VIEW secure_documents AS
     FOR doc IN documents
       FILTER can_access_row(doc, CURRENT_USER())
       RETURN doc

METHODE 2: Collection-Level-Policies

1. Policy erstellen (config/policies.yaml):
   policies:
     documents:
       read:
         condition: "doc.owner_id == @user_id OR doc.public == true"
       write:
         condition: "doc.owner_id == @user_id OR @user_role == 'admin'"

2. Policy aktivieren:
   ALTER COLLECTION documents SET POLICY read_policy;

3. Automatische Enforcement:
   FOR doc IN documents  // Policy automatisch angewendet
     RETURN doc

METHODE 3: Tenant-Isolation (Multi-Tenancy)

1. Tenant-ID in jedem Dokument:
   INSERT {tenant_id: "tenant_a", data: {...}} INTO documents;

2. Middleware mit Tenant-Filter:
   app.use((req, res, next) => {
     req.aql_filter = `doc.tenant_id == "${req.user.tenant_id}"`;
   });

AUDIT-LOGGING:
   monitoring:
     audit_log:
       enabled: true
       log_access_violations: true

Details: Kapitel 21 (Auth), Kapitel 36 (Security Hardening)
```

---

**Frage 10: "Wie erfülle ich GDPR-Anforderungen (Recht auf Vergessen)?"**

```aql
RETURN DOCS_QUERY('Wie erfülle ich GDPR-Anforderungen in ThemisDB?')
```

**Erwartete Antwort:**
```
GDPR-Compliance in ThemisDB (Recht auf Vergessen):

1. DATEN-LÖSCHUNG (Right to be Forgotten):

   A) Vollständige Löschung:
      // Alle Daten eines Users löschen
      FOR doc IN users
        FILTER doc.email == @user_email
        REMOVE doc IN users;
      
      // Related Collections durchsuchen
      FOR collection IN ["orders", "preferences", "logs"]
        FOR doc IN collection
          FILTER doc.user_id == @user_id
          REMOVE doc IN collection;

   B) Anonymisierung (Alternative):
      UPDATE {_key: @user_key} WITH {
        email: "anonymized@deleted.user",
        name: "[DELETED]",
        phone: null,
        address: null,
        gdpr_deleted_at: DATE_NOW()
      } IN users;

2. AUDIT-TRAIL für GDPR-Requests:

   CREATE COLLECTION gdpr_requests;
   INSERT {
     user_id: @user_id,
     request_type: "deletion",  // oder "export", "correction"
     timestamp: DATE_NOW(),
     processed_by: CURRENT_USER(),
     status: "completed"
   } INTO gdpr_requests;

3. DATEN-EXPORT (Right to Data Portability):

   // JSON-Export aller User-Daten
   LET user_data = DOCUMENT("users/@user_key")
   LET orders = (FOR o IN orders FILTER o.user_id == @user_id RETURN o)
   LET preferences = DOCUMENT("preferences/@user_key")
   
   RETURN {
     user: user_data,
     orders: orders,
     preferences: preferences,
     export_date: DATE_NOW()
   }

4. DATEN-RETENTION-POLICY:

   // Automatische Löschung alter Daten
   FOR doc IN logs
     FILTER doc.created_at < DATE_SUBTRACT(DATE_NOW(), 90, "days")
     REMOVE doc IN logs;

5. VERSCHLÜSSELUNG:

   config/themis.yaml:
     encryption:
       at_rest: true
       algorithm: "AES-256-GCM"
       key_rotation: "monthly"

COMPLIANCE-CHECKLIST:
✅ Recht auf Löschung implementiert
✅ Recht auf Export implementiert
✅ Audit-Trail für alle Requests
✅ Verschlüsselung (at-rest + in-transit)
✅ Zugriffskontrollen (RBAC)
✅ Daten-Minimierung (nur notwendige Daten)
✅ Anonymisierung von Logs

Details: Kapitel 40 (Data Governance & Compliance)
```

---

### 42.9.3.5 Integration & DevOps

**Frage 11: "Wie integriere ich ThemisDB mit Kubernetes?"**

```aql
RETURN DOCS_QUERY('Wie deploye ich ThemisDB in Kubernetes?')
```

**Erwartete Antwort:**
```
ThemisDB Kubernetes Deployment:

1. STATEFULSET für ThemisDB (Persistence):

   apiVersion: apps/v1
   kind: StatefulSet
   metadata:
     name: themisdb
   spec:
     serviceName: themisdb
     replicas: 3
     selector:
       matchLabels:
         app: themisdb
     template:
       metadata:
         labels:
           app: themisdb
       spec:
         containers:
         - name: themisdb
           image: themisdb/themisdb:v1.4.0
           ports:
           - containerPort: 8765
           volumeMounts:
           - name: data
             mountPath: /var/lib/themisdb
           env:
           - name: THEMIS_CLUSTER_MODE
             value: "true"
           - name: THEMIS_RAFT_PEERS
             value: "themisdb-0.themisdb:7687,themisdb-1.themisdb:7687"
     volumeClaimTemplates:
     - metadata:
         name: data
       spec:
         accessModes: ["ReadWriteOnce"]
         resources:
           requests:
             storage: 100Gi
         storageClassName: fast-ssd

2. SERVICE (ClusterIP):

   apiVersion: v1
   kind: Service
   metadata:
     name: themisdb
   spec:
     selector:
       app: themisdb
     ports:
     - protocol: TCP
       port: 8765
       targetPort: 8765
     type: ClusterIP

3. CONFIGMAP:

   apiVersion: v1
   kind: ConfigMap
   metadata:
     name: themisdb-config
   data:
     themis.yaml: |
       server:
         port: 8765
         threads: 8
       storage:
         path: /var/lib/themisdb
       cluster:
         enabled: true

4. HELM-CHART (Empfohlen):

   helm repo add themisdb https://charts.themisdb.io
   helm install my-themisdb themisdb/themisdb \
     --set replicaCount=3 \
     --set persistence.size=100Gi \
     --set resources.limits.memory=32Gi

MONITORING:
- Prometheus Operator: ServiceMonitor für Metriken
- Grafana: Dashboard für Cluster-Health
- Alertmanager: Alerts für Node-Failures

Details: Kapitel 25 (DevOps), Kapitel 30 (Deployment & Operations)
```

---

**Frage 12: "Wie richte ich Backups mit Point-in-Time-Recovery ein?"**

```aql
RETURN DOCS_CONFIG_HELP('backup and point-in-time recovery')
```

**Erwartete Antwort:**
```
Backup & Point-in-Time-Recovery (PITR) in ThemisDB:

1. AUTOMATISCHE BACKUPS konfigurieren:

   config/themis.yaml:
     backup:
       enabled: true
       schedule: "0 2 * * *"  # Täglich um 2:00 Uhr
       retention_days: 30
       destination: "s3://my-bucket/themisdb-backups"
       compression: "zstd"
       encryption: true

2. MANUELLES BACKUP:

   # Full Backup
   themis_admin backup create \
     --type full \
     --output /backup/themisdb-full-$(date +%Y%m%d).tar.zst

   # Incremental Backup
   themis_admin backup create \
     --type incremental \
     --base-backup /backup/themisdb-full-20260101.tar.zst \
     --output /backup/themisdb-incr-$(date +%Y%m%d).tar.zst

3. WRITE-AHEAD-LOG (WAL) für PITR:

   config/themis.yaml:
     wal:
       enabled: true
       retention_hours: 72  # 3 Tage
       archive_dir: "/var/lib/themisdb/wal_archive"

4. POINT-IN-TIME-RESTORE:

   # Restore zu spezifischem Zeitpunkt
   themis_admin restore \
     --backup /backup/themisdb-full-20260101.tar.zst \
     --wal-archive /var/lib/themisdb/wal_archive \
     --point-in-time "2026-01-10 14:30:00" \
     --target-dir /var/lib/themisdb/restore

5. RESTORE-PROZESS:

   # 1. Server stoppen
   systemctl stop themisdb

   # 2. Daten sichern (Sicherheitskopie)
   mv /var/lib/themisdb/data /var/lib/themisdb/data.old

   # 3. Restore durchführen
   themis_admin restore --backup /backup/latest.tar.zst

   # 4. Server starten
   systemctl start themisdb

   # 5. Verifizieren
   themis_cli --execute "RETURN COUNT(FOR doc IN users RETURN 1)"

6. S3-BACKUP (Cloud):

   # AWS S3 konfigurieren
   export AWS_ACCESS_KEY_ID=...
   export AWS_SECRET_ACCESS_KEY=...
   
   themis_admin backup create \
     --destination s3://my-bucket/backups/ \
     --encryption AES256

BACKUP-STRATEGIE:
- Full: Täglich (2:00 Uhr)
- Incremental: Stündlich (während Geschäftszeiten)
- WAL-Archive: Kontinuierlich
- Retention: 30 Tage Full, 7 Tage Incremental
- Off-Site: S3 + Glacier (90 Tage)

Details: Kapitel 20 (Backup & Recovery)
```

---

### 42.9.3.6 Erweiterte Features

**Frage 13: "Wie verwende ich die Zeitreihen-Funktionen?"**

```aql
RETURN DOCS_QUERY('Wie verwende ich Zeitreihen-Features in ThemisDB?')
```

**Erwartete Antwort:**
```
Zeitreihen-Features in ThemisDB:

1. TIMESERIES-COLLECTION erstellen:

   CREATE TIMESERIES COLLECTION sensor_data
     TIME_FIELD timestamp
     GRANULARITY hour
     RETENTION 90  # Tage

2. DATEN EINFÜGEN:

   INSERT {
     timestamp: DATE_NOW(),
     sensor_id: "temp_001",
     temperature: 23.5,
     humidity: 65.2,
     location: "room_a"
   } INTO sensor_data;

3. ZEITREIHEN-QUERIES:

   A) Aggregation über Zeitfenster:
      FOR data IN sensor_data
        FILTER data.timestamp >= DATE_SUBTRACT(DATE_NOW(), 24, "hours")
        COLLECT time_bucket = DATE_TRUNC(data.timestamp, "hour")
        AGGREGATE avg_temp = AVG(data.temperature),
                  max_temp = MAX(data.temperature),
                  min_temp = MIN(data.temperature)
        RETURN {
          hour: time_bucket,
          avg: avg_temp,
          max: max_temp,
          min: min_temp
        }

   B) Downsampling (Daten-Verdichtung):
      FOR data IN sensor_data
        FILTER data.timestamp < DATE_SUBTRACT(DATE_NOW(), 30, "days")
        COLLECT bucket = DATE_TRUNC(data.timestamp, "day"),
                sensor = data.sensor_id
        AGGREGATE avg_temp = AVG(data.temperature)
        INSERT {
          timestamp: bucket,
          sensor_id: sensor,
          temperature_avg: avg_temp,
          aggregated: true
        } INTO sensor_data_daily;

   C) Gap-Filling (fehlende Daten interpolieren):
      LET time_range = DATE_RANGE(
        DATE_SUBTRACT(DATE_NOW(), 7, "days"),
        DATE_NOW(),
        "PT1H"  # 1-Stunden-Intervalle
      )
      FOR t IN time_range
        LET data = FIRST(
          FOR d IN sensor_data
            FILTER d.timestamp >= DATE_SUBTRACT(t, 30, "minutes")
            FILTER d.timestamp <= DATE_ADD(t, 30, "minutes")
            LIMIT 1
            RETURN d.temperature
        )
        RETURN {
          timestamp: t,
          temperature: data != null ? data : INTERPOLATE(...)
        }

4. AUTOMATISCHE ROLLUPS:

   config/themis.yaml:
     timeseries:
       rollups:
         - name: "hourly_stats"
           source: "sensor_data"
           target: "sensor_data_hourly"
           interval: "1 hour"
           aggregations:
             - field: "temperature"
               functions: ["avg", "min", "max", "stddev"]

5. RETENTION-POLICIES:

   ALTER TIMESERIES COLLECTION sensor_data
     SET RETENTION 90 DAYS
     SET DOWNSAMPLING_POLICY (
       AFTER 30 DAYS AGGREGATE BY 1 HOUR,
       AFTER 90 DAYS AGGREGATE BY 1 DAY
     );

PERFORMANCE-TIPPS:
- Partitionierung nach Zeit für schnellere Queries
- Indizes auf sensor_id + timestamp
- Bulk-Inserts verwenden (1000+ pro Request)
- Materialized Views für häufige Aggregationen

Details: Kapitel 9 (Timeseries-Modell)
```

---

**Frage 14: "Wie aktiviere ich GPU-Beschleunigung für ML-Workloads?"**

```aql
RETURN DOCS_CONFIG_HELP('GPU acceleration for ML')
```

**Erwartete Antwort:**
```
GPU-Beschleunigung für ML-Workloads in ThemisDB:

1. VORAUSSETZUNGEN:

   Hardware:
   - NVIDIA GPU (CUDA 11.8+): Tesla V100, A100, H100
   - Mindestens 16 GB VRAM
   - PCIe 4.0 x16

   Software:
   - NVIDIA Driver 525.x+
   - CUDA Toolkit 11.8+
   - cuDNN 8.9+
   - TensorRT (optional, für Inferenz)

2. THEMISDB GPU-SUPPORT AKTIVIEREN:

   config/themis.yaml:
     ml:
       gpu:
         enabled: true
         device_id: 0  # GPU 0 verwenden
         memory_fraction: 0.8  # 80% VRAM für ThemisDB
         allow_growth: true  # Dynamische VRAM-Allokation

3. GPU-BESCHLEUNIGTE OPERATIONEN:

   A) Vector-Similarity-Search (GPU):
      CREATE VECTOR INDEX vec_idx ON documents(embedding)
        DIMENSION 768
        METRIC cosine
        ACCELERATOR gpu;  # GPU-Beschleunigung

      // Query wird automatisch auf GPU ausgeführt
      FOR doc IN documents
        SORT COSINE_SIMILARITY(doc.embedding, @query_vec) DESC
        LIMIT 100
        RETURN doc;

   B) ML-Inferenz (GPU):
      // Model laden
      REGISTER ML_MODEL my_classifier
        FROM "/models/bert_classifier.onnx"
        ACCELERATOR gpu
        BATCH_SIZE 32;

      // Batch-Inferenz
      FOR doc IN documents
        LIMIT 10000
        LET prediction = ML_PREDICT("my_classifier", doc.text)
        UPDATE doc WITH {category: prediction.label} IN documents;

   C) Matrix-Operationen (GPU):
      LET matrix_a = [...];  // 1000x1000 Matrix
      LET matrix_b = [...];
      RETURN MATMUL(matrix_a, matrix_b, {device: "gpu"});

4. MULTI-GPU-SETUP (Cluster):

   config/themis.yaml:
     ml:
       gpu:
         enabled: true
         devices: [0, 1, 2, 3]  # 4 GPUs
         distribution: "data_parallel"  # oder "model_parallel"

5. MONITORING:

   # GPU-Auslastung
   curl http://localhost:8765/metrics/gpu

   # Output:
   # themisdb_gpu_utilization{device="0"} 85.2
   # themisdb_gpu_memory_used{device="0"} 13.5GB
   # themisdb_gpu_temperature{device="0"} 72C

6. PERFORMANCE-BENCHMARKS:

   Operation               | CPU      | GPU (A100) | Speedup
   ----------------------- | -------- | ---------- | -------
   Vector Search (100k)    | 450ms    | 12ms       | 37.5x
   Matrix Multiply (1kx1k) | 850ms    | 8ms        | 106x
   ML Inference (BERT)     | 120ms    | 9ms        | 13.3x
   Embedding Generation    | 2.5s     | 45ms       | 55x

TROUBLESHOOTING:
- CUDA out of memory: memory_fraction reduzieren
- Langsame Inferenz: batch_size erhöhen
- GPU nicht erkannt: nvidia-smi prüfen

Details: Kapitel 16 (ML-Integration), Kapitel 39 (Performance Tuning)
```

---

### 42.9.3.7 Best Practices & Patterns

**Frage 15: "Was sind die wichtigsten Performance-Best-Practices?"**

```aql
RETURN DOCS_QUERY('Performance Best Practices für ThemisDB')
```

**Erwartete Antwort:**
```
Top 10 Performance-Best-Practices für ThemisDB:

1. INDIZIERUNG:
   ✅ DO: Indizes auf Filter- und Sort-Felder
      CREATE INDEX idx_user_email ON users(email);
   ❌ DON'T: Zu viele Indizes (max 5-7 pro Collection)

2. QUERY-OPTIMIERUNG:
   ✅ DO: Filter VOR Joins/Subqueries
      FOR u IN users
        FILTER u.active == true  // Filter zuerst!
        FOR o IN orders
          FILTER o.user_id == u._key
          RETURN o
   
   ❌ DON'T: Subqueries in Projektion
      FOR u IN users
        RETURN {
          user: u,
          orders: (FOR o IN orders FILTER o.user_id == u._key RETURN o)  // LANGSAM!
        }

3. LIMIT & PAGING:
   ✅ DO: Immer LIMIT verwenden
      FOR doc IN collection
        FILTER doc.type == "active"
        LIMIT 100
        RETURN doc
   
   ❌ DON'T: Unbegrenzte Result-Sets
      FOR doc IN collection RETURN doc  // Millionen Rows!

4. BULK-OPERATIONEN:
   ✅ DO: Batch-Inserts (1000+ pro Request)
      INSERT [
        {name: "Alice"},
        {name: "Bob"},
        ... // 1000 Dokumente
      ] INTO users OPTIONS {overwriteMode: "ignore"};
   
   ❌ DON'T: Einzelne Inserts in Schleife
      FOR i IN 1..10000
        INSERT {_key: i} INTO collection  // SEHR LANGSAM!

5. CACHING:
   ✅ DO: Query-Cache für häufige Queries
      FOR doc IN documents
        FILTER doc.category == @category
        OPTIONS {cache: true}
        RETURN doc
   
   config/themis.yaml:
     cache:
       query_cache_size: "4GB"

6. PROJECTION:
   ✅ DO: Nur benötigte Felder zurückgeben
      FOR doc IN documents
        RETURN {id: doc._key, title: doc.title}
   
   ❌ DON'T: Vollständige Dokumente bei großen Feldern
      FOR doc IN documents
        RETURN doc  // inkl. 10MB Binärdaten!

7. PARTITIONIERUNG:
   ✅ DO: Sharding nach Access-Pattern
      CREATE COLLECTION orders SHARD_KEY date;  // Zeit-basiert
   
   ❌ DON'T: Random Sharding bei Zeitreihen

8. AGGREGATIONEN:
   ✅ DO: Materialized Views für häufige Aggregationen
      CREATE VIEW user_stats AS
        FOR u IN users
          COLLECT WITH COUNT INTO total
          RETURN total
   
   ❌ DON'T: COUNT(*) auf großen Collections

9. TRANSACTIONS:
   ✅ DO: Kurze Transaktionen (<100ms)
      BEGIN TRANSACTION
        INSERT ... INTO users
        UPDATE ... IN orders
      COMMIT
   
   ❌ DON'T: Long-Running-Transactions (>10s)

10. MONITORING:
    ✅ DO: Slow-Query-Log aktivieren
       monitoring:
         slow_query_threshold: 100ms  # Log queries >100ms
    
    ✅ DO: EXPLAIN verwenden
       EXPLAIN FOR doc IN documents FILTER doc.field == @val RETURN doc

QUICK-WINS:
- Indizes hinzufügen: +80% schneller
- LIMIT verwenden: +95% schneller
- Bulk-Operationen: +90% schneller
- Query-Cache: +99% schneller (bei Cache-Hit)

Details: Kapitel 34 (Query Optimization), Kapitel 39 (Performance Tuning)
```

---

### 42.9.3.8 Verwendung des Dokumentations-Assistenten

**Meta-Frage: "Wie verwende ich den Dokumentations-Assistenten effektiv?"**

```aql
RETURN DOCS_QUERY('Tipps für effektive Nutzung des Dokumentations-Assistenten')
```

**Erwartete Antwort:**
```
Effektive Nutzung des ThemisDB Dokumentations-Assistenten:

1. FRAGESTELLUNG:

   ✅ GUTE FRAGEN (spezifisch, kontextreich):
   - "Wie konfiguriere ich SSL/TLS mit selbst-signierten Zertifikaten?"
   - "Warum sind meine Vector-Similarity-Queries langsam bei 1M Dokumenten?"
   - "Wie implementiere ich Multi-Tenancy mit Row-Level-Security?"

   ❌ SCHLECHTE FRAGEN (zu allgemein):
   - "Hilfe"
   - "Performance"
   - "Wie funktioniert Datenbank?"

2. KONTEXT ANGEBEN:

   ✅ MIT Kontext:
      "Ich habe 10M Dokumente in einer Collection und meine Aggregations-Query 
       dauert 30 Sekunden. Wie kann ich das optimieren?"

   ❌ OHNE Kontext:
      "Query ist langsam"

3. AQL-FUNKTIONEN NUTZEN:

   // Allgemeine Fragen
   RETURN DOCS_QUERY('Deine Frage hier')

   // Suche mit Relevanz-Ranking
   RETURN DOCS_SEARCH('sharding performance', 10)

   // Konfigurationshilfe
   RETURN DOCS_CONFIG_HELP('backup')

   // Troubleshooting
   RETURN DOCS_TROUBLESHOOT('Server startet nicht - Port 8765 already in use')

4. FOLLOW-UP-FRAGEN:

   ✅ DO: Aufbauende Fragen stellen
      1. "Wie aktiviere ich Sharding?"
      2. "Welche Shard-Keys sind für Zeitreihen optimal?"
      3. "Wie migriere ich bestehende Daten zu geshardeten Collections?"

5. FEHLERME LDUNGEN EINFÜGEN:

   ✅ DO: Vollständige Fehlermeldung kopieren
      RETURN DOCS_TROUBLESHOOT('
        ERROR: Out of memory
        Process: themisdb (PID 12345)
        Memory: 15.8 GB / 16 GB
        Stack trace: ...
      ')

6. ERWEITERTE SUCHE:

   // Nach Kapiteln filtern
   FOR doc IN :document
     FILTER doc.type == 'documentation'
     FILTER CONTAINS(doc.source, 'chapter_16')  // ML-Integration
     FILTER CONTAINS(doc.content, @search_term)
     RETURN doc

   // Multi-Model-Zugriff
   FOR node IN docs_graph_nodes
     FOR edge IN OUTBOUND node docs_graph_edges
       FILTER edge.type == 'related'
       RETURN edge._to

7. RATE-LIMITS BEACHTEN:

   Rolle      | Limit (req/min) | Context Docs
   ---------- | --------------- | ------------
   Admin      | 100             | 10
   Superuser  | 50              | 7
   User       | 20              | 3

8. BEST PRACTICES:

   ✅ DO:
   - Spezifische, klar formulierte Fragen
   - Fehler mit vollständigem Kontext beschreiben
   - AQL-Funktionen für verschiedene Zwecke nutzen
   - Verwandte Kapitel und Dokumente durchlesen

   ❌ DON'T:
   - Mehrere unzusammenhängende Fragen auf einmal
   - Erwartung, dass LLM Code schreibt (nur Anleitung!)
   - Rate-Limits ignorieren
   - Sicherheitskritische Daten in Queries einfügen

9. OFFLINE-ZUGRIFF:

   // Dokumentation auch ohne LLM durchsuchbar
   FOR doc IN :document
     FILTER doc.type == 'documentation'
     FILTER FULLTEXT(doc, @query)
     SORT BM25(doc) DESC
     LIMIT 10
     RETURN {
       title: doc.title,
       source: doc.source,
       snippet: SUBSTRING(doc.content, 0, 200)
     }

RESSOURCEN:
- Kapitel 42: Diese Anleitung
- docs/en/features/DOCS_ASSISTANT.md: Technische Details
- TODO_DOCS_DATABASE_BUILD.md: Roadmap und Features

Feedback & Verbesserungen: github.com/makr-code/ThemisDB/issues
```

---

## 42.10 Zusammenfassung

### 42.10.1 Feature-Matrix nach Rolle

| Feature | Admin | Superuser | User |
|---------|-------|-----------|------|
| REST API Query | ✅ | ✅ | ✅ |
| REST API Config | ✅ | 🟡 Read-Only | ❌ |
| REST API Troubleshoot | ✅ | ✅ | 🟡 Basic |
| AQL Functions | ✅ | ✅ | 🟡 Limited |
| CLI Access | ✅ | ✅ | ✅ |
| C++ SDK | ✅ | ✅ | ❌ |
| Max Context Docs | 10 | 7 | 3 |
| Rate Limit (req/min) | 100 | 50 | 20 |
| Cache TTL | 5 min | 10 min | 15 min |
| System Config Docs | ✅ | ❌ | ❌ |
| Internal Impl Docs | ✅ | ❌ | ❌ |

### 42.10.2 Wichtige Befehle

```bash
# Docs-Datenbank generieren
python3 scripts/generate_docs_rocksdb.py

# Query via CLI (AQL)
themis_cli --execute "RETURN DOCS_QUERY('your question here')"

# Query via REST API
curl -X POST /api/v1/llm/docs/query -H "Authorization: Bearer TOKEN" -d '{"query": "..."}'

# Search via AQL
RETURN DOCS_SEARCH('search term', 10)

# Config Help
RETURN DOCS_CONFIG_HELP('topic')

# Troubleshoot
RETURN DOCS_TROUBLESHOOT('error description')
```

### 42.10.3 Nächste Schritte

1. **Administratoren**: Integrieren Sie den Docs-Assistent in Ihr Monitoring
2. **Superuser**: Nutzen Sie AQL-Funktionen für Custom-Workflows
3. **User**: Erkunden Sie die interaktive CLI

**Weitere Ressourcen:**
- Kapitel 33: Best Practices
- Kapitel 38: Observability & SRE
- Kapitel 41: Hands-on Labs
- `docs/en/features/DOCS_ASSISTANT.md`
- `docs/en/features/DOCS_ROCKSDB_DATABASE.md`
- `TODO_DOCS_DATABASE_BUILD.md`

---

**Ende Kapitel 42**
