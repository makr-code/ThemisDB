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

<figure>

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
        REST[REST API<br/>/api/v1/llm/docs/*]
        AQL[AQL Funktionen<br/>DOCS_QUERY()]
        CLI[ThemisDB CLI<br/>--docs-query]
        SDK[C++ SDK<br/>DocsAssistant]
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

<figcaption><b>Abb. 42.0:</b> Dokumentations-Assistent Architektur nach Benutzerrolle</figcaption>

</figure>

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
