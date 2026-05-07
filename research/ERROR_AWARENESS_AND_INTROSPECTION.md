# Error Awareness & Introspection für Agentic AI Self-Awareness

**Datum:** 11. Januar 2026  
**Version:** 1.0  
**Kategorie:** Self-Awareness Extension  
**Zweck:** Fehlermeldungen im Sourcecode introspektieren und für Self-Awareness nutzbar machen

---

## 📋 Übersicht

Diese Dokumentation erweitert die Agentic AI Self-Awareness um **Error-Awareness**: Die Fähigkeit der Datenbank, über ihre eigenen Fehlermeldungen, deren Ursachen und Behandlung zu kommunizieren.

**Ziel:** Nutzer können fragen:
- "Welche Fehler können auftreten?"
- "Was bedeutet Fehler X?"
- "Wie behebe ich Problem Y?"
- "Warum tritt dieser Fehler auf?"

---

## 🔍 Problem-Analyse

### Aktueller Stand

**Logging-Infrastruktur vorhanden:**
- ✅ `spdlog` als Logging-Framework
- ✅ `Logger` wrapper class (`include/utils/logger.h`)
- ✅ Spezielle Logger: `AuditLogger`, `SagaLogger`
- ✅ Log-Levels: TRACE, DEBUG, INFO, WARN, ERROR, CRITICAL
- ✅ Strukturierte Logs in ~50+ Dateien

**Beispiele aus dem Code:**
```cpp
// src/llm/model_loader.cpp
spdlog::error("Model file not found: {}", model_path);
spdlog::error("Failed to load model from file: {}", model_path);
spdlog::error("Failed to create context for model: {}", model_id);

// src/llm/multi_lora_manager.cpp
spdlog::error("LoRA not loaded: {}", lora_id);
spdlog::error("Multi-LoRA batching is disabled");
spdlog::error("Number of LoRAs ({}) doesn't match number of weights ({})", ...);
```

**Was fehlt:**
- ❌ Zentrale Fehler-Taxonomie (Error Codes)
- ❌ Metadaten zu Fehlern (Ursache, Lösung, Kategorie)
- ❌ Introspektions-API für Fehlermeldungen
- ❌ Mapping: Log-Message → Error-Code → Metadata
- ❌ Natural Language Interface für Fehler-Queries

---

## 🎯 Lösungsansatz

### Strategie 1: Error Code System (Empfohlen)

**Einführung strukturierter Error Codes:**

```cpp
// include/utils/error_codes.h
namespace themis {
namespace errors {

enum class ErrorCode {
    // Storage Errors (1000-1999)
    ERR_STORAGE_FILE_NOT_FOUND = 1000,
    ERR_STORAGE_PERMISSION_DENIED = 1001,
    ERR_STORAGE_DISK_FULL = 1002,
    ERR_STORAGE_CORRUPTION = 1003,
    
    // LLM Errors (2000-2999)
    ERR_LLM_MODEL_NOT_FOUND = 2000,
    ERR_LLM_MODEL_LOAD_FAILED = 2001,
    ERR_LLM_CONTEXT_CREATION_FAILED = 2002,
    ERR_LLM_INFERENCE_TIMEOUT = 2003,
    ERR_LLM_GPU_OOM = 2004,
    
    // LoRA Errors (2100-2199)
    ERR_LORA_NOT_LOADED = 2100,
    ERR_LORA_BATCHING_DISABLED = 2101,
    ERR_LORA_WEIGHT_MISMATCH = 2102,
    ERR_LORA_FUSION_FAILED = 2103,
    
    // MCP Errors (3000-3999)
    ERR_MCP_TRANSPORT_FAILED = 3000,
    ERR_MCP_INVALID_REQUEST = 3001,
    ERR_MCP_TOOL_NOT_FOUND = 3002,
    
    // Schema Errors (4000-4999)
    ERR_SCHEMA_TABLE_NOT_FOUND = 4000,
    ERR_SCHEMA_INVALID_TYPE = 4001,
    
    // Network Errors (5000-5999)
    ERR_NET_CONNECTION_REFUSED = 5000,
    ERR_NET_TIMEOUT = 5001,
    
    // Unknown
    ERR_UNKNOWN = 9999
};

struct ErrorMetadata {
    ErrorCode code;
    std::string category;           // "Storage", "LLM", "LoRA", "MCP", etc.
    std::string severity;           // "Critical", "Error", "Warning"
    std::string message_template;   // "Model file not found: {}"
    std::string cause;              // "Model path does not exist or is not accessible"
    std::string solution;           // "Verify model path in config, check file permissions"
    std::vector<std::string> related_docs; // Links to documentation
};

class ErrorRegistry {
public:
    static void registerError(const ErrorMetadata& metadata);
    static ErrorMetadata getError(ErrorCode code);
    static std::vector<ErrorMetadata> getErrorsByCategory(const std::string& category);
    static std::vector<ErrorMetadata> searchErrors(const std::string& query);
};

} // namespace errors
} // namespace themis
```

**Vorteile:**
- Strukturierte, maschinenlesbare Error-Informationen
- Einheitliche Error-Behandlung
- Einfache Introspection
- Versionierung möglich

---

### Strategie 2: Log Parsing & ML-basierte Analyse (Ergänzung)

**Für bestehenden Code ohne Error Codes:**

```cpp
// include/utils/log_analyzer.h
namespace themis {
namespace utils {

struct LogEntry {
    std::string timestamp;
    std::string level;        // ERROR, WARN, INFO, etc.
    std::string message;
    std::string source_file;
    int source_line;
    std::map<std::string, std::string> context;
};

struct ErrorPattern {
    std::string pattern_regex;    // "Model file not found: (.*)"
    std::string category;          // "LLM"
    std::string probable_cause;
    std::string suggested_solution;
    int error_code;
};

class LogAnalyzer {
public:
    // Parse log file and extract error entries
    std::vector<LogEntry> parseLogFile(const std::string& log_path);
    
    // Analyze error patterns
    std::vector<ErrorPattern> analyzeErrors(const std::vector<LogEntry>& entries);
    
    // LLM-assisted error analysis
    std::string explainError(const LogEntry& entry, llm::EmbeddedLLM* llm);
    
    // Find similar errors in history
    std::vector<LogEntry> findSimilarErrors(const LogEntry& entry, int limit = 10);
};

} // namespace utils
} // namespace themis
```

**Vorteile:**
- Funktioniert mit bestehendem Code
- Nutzt LLM für intelligente Analyse
- Historische Error-Patterns erkennbar

---

## 📊 Error Taxonomy

### Vorgeschlagene Kategorien

| Kategorie | Error Code Range | Beispiele |
|-----------|------------------|-----------|
| **Storage** | 1000-1999 | File not found, Disk full, Corruption |
| **LLM** | 2000-2099 | Model loading, Inference timeout, GPU OOM |
| **LoRA** | 2100-2199 | Adapter not loaded, Fusion failed |
| **MCP** | 3000-3999 | Transport failed, Invalid request |
| **Schema** | 4000-4999 | Table not found, Invalid type |
| **Network** | 5000-5999 | Connection refused, Timeout |
| **Index** | 6000-6999 | Index creation failed, Corrupted index |
| **Query** | 7000-7999 | Syntax error, Invalid query |
| **Security** | 8000-8999 | Authentication failed, Permission denied |
| **Unknown** | 9000-9999 | Unclassified errors |

---

## 🔧 Implementation Plan

### Phase 1: Error Code System (Foundation)

**Schritt 1.1: Error Registry erstellen**

**Datei:** `include/utils/error_registry.h`  
**Aktion:** NEU ERSTELLEN

```cpp
#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <nlohmann/json.hpp>

namespace themis {
namespace errors {

using json = nlohmann::json;

enum class ErrorCode {
    // Storage Errors (1000-1999)
    ERR_STORAGE_FILE_NOT_FOUND = 1000,
    ERR_STORAGE_PERMISSION_DENIED = 1001,
    ERR_STORAGE_DISK_FULL = 1002,
    ERR_STORAGE_CORRUPTION = 1003,
    
    // LLM Errors (2000-2099)
    ERR_LLM_MODEL_NOT_FOUND = 2000,
    ERR_LLM_MODEL_LOAD_FAILED = 2001,
    ERR_LLM_CONTEXT_CREATION_FAILED = 2002,
    ERR_LLM_INFERENCE_TIMEOUT = 2003,
    ERR_LLM_GPU_OOM = 2004,
    
    // LoRA Errors (2100-2199)
    ERR_LORA_NOT_LOADED = 2100,
    ERR_LORA_BATCHING_DISABLED = 2101,
    ERR_LORA_WEIGHT_MISMATCH = 2102,
    ERR_LORA_FUSION_FAILED = 2103,
    ERR_LORA_INVALID_DATA = 2104,
    
    // MCP Errors (3000-3999)
    ERR_MCP_TRANSPORT_FAILED = 3000,
    ERR_MCP_INVALID_REQUEST = 3001,
    ERR_MCP_TOOL_NOT_FOUND = 3002,
    ERR_MCP_SCHEMA_UNAVAILABLE = 3003,
    
    // Schema Errors (4000-4999)
    ERR_SCHEMA_TABLE_NOT_FOUND = 4000,
    ERR_SCHEMA_INVALID_TYPE = 4001,
    ERR_SCHEMA_CACHE_MISS = 4002,
    
    // Network Errors (5000-5999)
    ERR_NET_CONNECTION_REFUSED = 5000,
    ERR_NET_TIMEOUT = 5001,
    ERR_NET_DNS_FAILURE = 5002,
    
    // Unknown
    ERR_UNKNOWN = 9999
};

struct ErrorMetadata {
    ErrorCode code;
    std::string category;           // "Storage", "LLM", "LoRA", etc.
    std::string severity;           // "Critical", "Error", "Warning"
    std::string message_template;   // Template with {} placeholders
    std::string cause;              // Detailed cause description
    std::string solution;           // Step-by-step solution
    std::vector<std::string> related_docs;  // Documentation links
    std::vector<std::string> keywords;      // For searching
    
    json toJSON() const;
};

class ErrorRegistry {
public:
    static ErrorRegistry& getInstance();
    
    void registerError(const ErrorMetadata& metadata);
    ErrorMetadata getError(ErrorCode code) const;
    std::vector<ErrorMetadata> getErrorsByCategory(const std::string& category) const;
    std::vector<ErrorMetadata> searchErrors(const std::string& query) const;
    std::vector<std::string> getAllCategories() const;
    
    json toJSON() const;
    
private:
    ErrorRegistry();
    void registerDefaultErrors();
    
    std::unordered_map<int, ErrorMetadata> errors_;
    std::unordered_map<std::string, std::vector<int>> category_index_;
};

// Helper function to log errors with error code
template<typename... Args>
void logError(ErrorCode code, Args&&... args) {
    auto& registry = ErrorRegistry::getInstance();
    auto metadata = registry.getError(code);
    
    std::string formatted = fmt::format(metadata.message_template, 
                                       std::forward<Args>(args)...);
    spdlog::error("[{}] {}", static_cast<int>(code), formatted);
}

} // namespace errors
} // namespace themis
```

**Schritt 1.2: Error Registry implementieren**

**Datei:** `src/utils/error_registry.cpp`  
**Aktion:** NEU ERSTELLEN

```cpp
#include "utils/error_registry.h"
#include <algorithm>

namespace themis {
namespace errors {

json ErrorMetadata::toJSON() const {
    return {
        {"code", static_cast<int>(code)},
        {"category", category},
        {"severity", severity},
        {"message_template", message_template},
        {"cause", cause},
        {"solution", solution},
        {"related_docs", related_docs},
        {"keywords", keywords}
    };
}

ErrorRegistry& ErrorRegistry::getInstance() {
    static ErrorRegistry instance;
    return instance;
}

ErrorRegistry::ErrorRegistry() {
    registerDefaultErrors();
}

void ErrorRegistry::registerDefaultErrors() {
    // Storage Errors
    registerError({
        ErrorCode::ERR_STORAGE_FILE_NOT_FOUND,
        "Storage",
        "Error",
        "File not found: {}",
        "The specified file path does not exist or is not accessible.",
        "1. Verify the file path in configuration\n"
        "2. Check file system permissions\n"
        "3. Ensure the file was not moved or deleted",
        {"/docs/configuration.md", "/docs/troubleshooting.md"},
        {"file", "not found", "storage", "path"}
    });
    
    registerError({
        ErrorCode::ERR_STORAGE_DISK_FULL,
        "Storage",
        "Critical",
        "Disk full: {} bytes required, {} bytes available",
        "Insufficient disk space to complete the operation.",
        "1. Free up disk space by removing old logs or backups\n"
        "2. Configure retention policies\n"
        "3. Consider expanding storage capacity",
        {"/docs/maintenance.md", "/docs/retention.md"},
        {"disk", "full", "space", "storage"}
    });
    
    // LLM Errors
    registerError({
        ErrorCode::ERR_LLM_MODEL_NOT_FOUND,
        "LLM",
        "Error",
        "Model file not found: {}",
        "The specified LLM model file does not exist at the given path.",
        "1. Verify model path in llm_config.yaml\n"
        "2. Download the model from official sources\n"
        "3. Check file permissions and ownership\n"
        "4. Ensure model format is compatible (GGUF for llama.cpp)",
        {"/docs/llm/model_loading.md", "/docs/configuration.md"},
        {"llm", "model", "not found", "path"}
    });
    
    registerError({
        ErrorCode::ERR_LLM_GPU_OOM,
        "LLM",
        "Critical",
        "GPU out of memory: {} MB required, {} MB available",
        "Insufficient GPU VRAM to load the model or process the request.",
        "1. Use a smaller model or quantized version (Q4_K_M, Q5_K_M)\n"
        "2. Reduce context window size\n"
        "3. Enable model offloading to CPU\n"
        "4. Close other GPU applications\n"
        "5. Use multiple GPUs with tensor parallelism",
        {"/docs/llm/gpu_management.md", "/docs/llm/quantization.md"},
        {"gpu", "oom", "out of memory", "vram", "llm"}
    });
    
    // LoRA Errors
    registerError({
        ErrorCode::ERR_LORA_NOT_LOADED,
        "LoRA",
        "Error",
        "LoRA adapter not loaded: {}",
        "The requested LoRA adapter has not been loaded into memory.",
        "1. Verify LoRA adapter path in configuration\n"
        "2. Load the adapter using Multi-LoRA Manager API\n"
        "3. Check adapter compatibility with base model\n"
        "4. Verify adapter format (safetensors or GGUF)",
        {"/docs/llm/lora_management.md"},
        {"lora", "adapter", "not loaded", "multi-lora"}
    });
    
    registerError({
        ErrorCode::ERR_LORA_WEIGHT_MISMATCH,
        "LoRA",
        "Error",
        "Number of LoRAs ({}) doesn't match number of weights ({})",
        "LoRA fusion requires equal number of adapters and weight values.",
        "1. Ensure weights array length matches number of LoRA adapters\n"
        "2. Verify fusion configuration\n"
        "3. Check that all adapters are properly loaded",
        {"/docs/llm/lora_fusion.md"},
        {"lora", "fusion", "weights", "mismatch"}
    });
    
    // MCP Errors
    registerError({
        ErrorCode::ERR_MCP_SCHEMA_UNAVAILABLE,
        "MCP",
        "Error",
        "Schema discovery requires full query engine integration",
        "MCP schema introspection is not fully implemented.",
        "1. Implement SchemaManager (see DETAILED_IMPLEMENTATION_GUIDE.md)\n"
        "2. Replace MCP stub implementations\n"
        "3. Enable schema caching\n"
        "4. Verify database is initialized",
        {"/research/DETAILED_IMPLEMENTATION_GUIDE.md"},
        {"mcp", "schema", "unavailable", "stub"}
    });
    
    // Schema Errors
    registerError({
        ErrorCode::ERR_SCHEMA_TABLE_NOT_FOUND,
        "Schema",
        "Error",
        "Table not found: {}",
        "The requested table does not exist in the database schema.",
        "1. Verify table name spelling\n"
        "2. Check if table was created\n"
        "3. Refresh schema cache\n"
        "4. Use SchemaManager.getAllTables() to list available tables",
        {"/docs/schema_management.md"},
        {"schema", "table", "not found"}
    });
}

void ErrorRegistry::registerError(const ErrorMetadata& metadata) {
    int code_value = static_cast<int>(metadata.code);
    errors_[code_value] = metadata;
    category_index_[metadata.category].push_back(code_value);
}

ErrorMetadata ErrorRegistry::getError(ErrorCode code) const {
    int code_value = static_cast<int>(code);
    auto it = errors_.find(code_value);
    if (it != errors_.end()) {
        return it->second;
    }
    
    // Return default error metadata
    return {
        ErrorCode::ERR_UNKNOWN,
        "Unknown",
        "Error",
        "Unknown error",
        "Error details not available",
        "Contact support for assistance",
        {},
        {"unknown"}
    };
}

std::vector<ErrorMetadata> ErrorRegistry::getErrorsByCategory(
    const std::string& category) const {
    
    std::vector<ErrorMetadata> result;
    auto it = category_index_.find(category);
    if (it != category_index_.end()) {
        for (int code : it->second) {
            result.push_back(errors_.at(code));
        }
    }
    return result;
}

std::vector<ErrorMetadata> ErrorRegistry::searchErrors(
    const std::string& query) const {
    
    std::vector<ErrorMetadata> result;
    std::string query_lower = query;
    std::transform(query_lower.begin(), query_lower.end(), 
                  query_lower.begin(), ::tolower);
    
    for (const auto& [code, metadata] : errors_) {
        // Search in keywords
        for (const auto& keyword : metadata.keywords) {
            if (keyword.find(query_lower) != std::string::npos) {
                result.push_back(metadata);
                break;
            }
        }
    }
    
    return result;
}

std::vector<std::string> ErrorRegistry::getAllCategories() const {
    std::vector<std::string> categories;
    for (const auto& [category, _] : category_index_) {
        categories.push_back(category);
    }
    return categories;
}

json ErrorRegistry::toJSON() const {
    json result = {
        {"total_errors", errors_.size()},
        {"categories", getAllCategories()},
        {"errors", json::array()}
    };
    
    for (const auto& [code, metadata] : errors_) {
        result["errors"].push_back(metadata.toJSON());
    }
    
    return result;
}

} // namespace errors
} // namespace themis
```

---

### Phase 2: REST API für Error Introspection

**Datei:** `include/server/error_api_handler.h`  
**Aktion:** NEU ERSTELLEN

```cpp
#pragma once

#include "server/http_server.h"
#include "utils/error_registry.h"

namespace themis {
namespace server {

class ErrorApiHandler {
public:
    ErrorApiHandler() = default;
    
    void registerRoutes(HttpServer& server);
    
private:
    void handleGetErrors(const Request& req, Response& res);
    void handleGetError(const Request& req, Response& res);
    void handleGetCategories(const Request& req, Response& res);
    void handleSearchErrors(const Request& req, Response& res);
};

} // namespace server
} // namespace themis
```

**Endpoints:**
- `GET /api/v1/errors` - List all registered errors
- `GET /api/v1/errors/:code` - Get specific error by code
- `GET /api/v1/errors/categories` - List all error categories
- `GET /api/v1/errors/search?q=gpu` - Search errors by keyword

---

### Phase 3: MCP Tools für Error Awareness

**Datei:** `src/mcp/mcp_server.cpp`  
**Aktion:** METHODEN HINZUFÜGEN

```cpp
void McpServer::registerTools() {
    // Existing tools...
    
    // ADD THIS: Error introspection tools
    registerTool(
        "get_error_info",
        "Get detailed information about an error code or message",
        {
            {"type", "object"},
            {"properties", {
                {"query", {
                    {"type", "string"},
                    {"description", "Error code (e.g., '2000') or search query"}
                }}
            }},
            {"required", {"query"}}
        },
        [this](const json& args) { return toolGetErrorInfo(args); }
    );
    
    registerTool(
        "search_errors",
        "Search for errors by category, keyword, or description",
        {
            {"type", "object"},
            {"properties", {
                {"query", {
                    {"type", "string"},
                    {"description", "Search query (e.g., 'gpu', 'lora', 'storage')"}
                }},
                {"category", {
                    {"type", "string"},
                    {"description", "Filter by category (optional)"}
                }}
            }},
            {"required", {"query"}}
        },
        [this](const json& args) { return toolSearchErrors(args); }
    );
}

json McpServer::toolGetErrorInfo(const json& args) {
    std::string query = args.value("query", "");
    
    auto& registry = errors::ErrorRegistry::getInstance();
    
    // Try to parse as error code
    try {
        int code = std::stoi(query);
        auto metadata = registry.getError(static_cast<errors::ErrorCode>(code));
        
        return {
            {"status", "success"},
            {"error", metadata.toJSON()}
        };
    } catch (...) {
        // Search by query
        auto results = registry.searchErrors(query);
        
        if (results.empty()) {
            return {
                {"status", "not_found"},
                {"message", "No errors found matching query"}
            };
        }
        
        json errors_json = json::array();
        for (const auto& error : results) {
            errors_json.push_back(error.toJSON());
        }
        
        return {
            {"status", "success"},
            {"errors", errors_json},
            {"count", results.size()}
        };
    }
}

json McpServer::toolSearchErrors(const json& args) {
    std::string query = args.value("query", "");
    std::string category = args.value("category", "");
    
    auto& registry = errors::ErrorRegistry::getInstance();
    std::vector<errors::ErrorMetadata> results;
    
    if (!category.empty()) {
        results = registry.getErrorsByCategory(category);
    } else {
        results = registry.searchErrors(query);
    }
    
    json errors_json = json::array();
    for (const auto& error : results) {
        errors_json.push_back(error.toJSON());
    }
    
    return {
        {"status", "success"},
        {"errors", errors_json},
        {"count", results.size()}
    };
}
```

---

### Phase 4: Natural Language Integration

**Datei:** `src/mcp/mcp_server.cpp`  
**Aktion:** METHODE ERWEITERN

```cpp
json McpServer::toolIntrospectDatabase(const json& args) {
    std::string question = args.value("question", "");
    
    if (question.empty()) {
        return {{"status", "error"}, {"message", "No question provided"}};
    }
    
    std::string question_lower = question;
    std::transform(question_lower.begin(), question_lower.end(), 
                  question_lower.begin(), ::tolower);
    
    std::string answer;
    
    // ... existing question handlers ...
    
    // ADD THIS: Error-related questions
    if (question_lower.find("fehler") != std::string::npos ||
        question_lower.find("error") != std::string::npos ||
        question_lower.find("problem") != std::string::npos) {
        answer = generateErrorAnswer(question_lower);
    }
    
    return {
        {"status", "success"},
        {"question", question},
        {"answer", answer}
    };
}

std::string McpServer::generateErrorAnswer(const std::string& question_lower) {
    auto& registry = errors::ErrorRegistry::getInstance();
    
    // "Welche Fehler können auftreten?"
    if (question_lower.find("welche fehler") != std::string::npos ||
        question_lower.find("what errors") != std::string::npos) {
        
        auto categories = registry.getAllCategories();
        std::string answer = "Ich kann folgende Fehler-Kategorien behandeln:\n\n";
        
        for (const auto& category : categories) {
            auto errors = registry.getErrorsByCategory(category);
            answer += fmt::format("**{}** ({} Fehlertypen)\n", category, errors.size());
        }
        
        answer += "\nFrage mich nach spezifischen Fehlern, z.B. 'Was bedeutet LLM Fehler 2000?'";
        return answer;
    }
    
    // "Was bedeutet Fehler X?"
    if (question_lower.find("bedeutet") != std::string::npos ||
        question_lower.find("mean") != std::string::npos) {
        
        // Extract error code or keyword
        std::regex code_regex(R"(\b\d{4}\b)");
        std::smatch match;
        
        if (std::regex_search(question_lower, match, code_regex)) {
            int code = std::stoi(match.str());
            auto metadata = registry.getError(static_cast<errors::ErrorCode>(code));
            
            return fmt::format(
                "**Fehler {}: {}**\n\n"
                "**Kategorie:** {}\n"
                "**Schweregrad:** {}\n\n"
                "**Ursache:**\n{}\n\n"
                "**Lösung:**\n{}\n\n"
                "**Dokumentation:** {}",
                code,
                metadata.message_template,
                metadata.category,
                metadata.severity,
                metadata.cause,
                metadata.solution,
                fmt::join(metadata.related_docs, ", ")
            );
        }
    }
    
    // Fallback: Search by keywords
    auto results = registry.searchErrors(question_lower);
    if (!results.empty()) {
        std::string answer = fmt::format("Ich habe {} relevante Fehler gefunden:\n\n", 
                                        results.size());
        
        for (size_t i = 0; i < std::min(results.size(), size_t(3)); ++i) {
            const auto& error = results[i];
            answer += fmt::format(
                "**[{}] {}**\n{}\n\n",
                static_cast<int>(error.code),
                error.message_template,
                error.cause
            );
        }
        
        return answer;
    }
    
    return "Ich habe keine passenden Fehlerinformationen gefunden. "
           "Versuche es mit einer spezifischeren Frage oder einem Fehlercode.";
}
```

---

## 📖 Verwendungsbeispiele

### Beispiel 1: REST API

```bash
# Liste aller Fehler
curl http://localhost:8080/api/v1/errors

# Spezifischer Fehler
curl http://localhost:8080/api/v1/errors/2000

# Suche nach GPU-Fehlern
curl http://localhost:8080/api/v1/errors/search?q=gpu

# Alle LoRA-Fehler
curl http://localhost:8080/api/v1/errors?category=LoRA
```

### Beispiel 2: MCP Tools

```json
// Get error info
{
  "method": "tools/call",
  "params": {
    "name": "get_error_info",
    "arguments": {
      "query": "2000"
    }
  }
}

// Search errors
{
  "method": "tools/call",
  "params": {
    "name": "search_errors",
    "arguments": {
      "query": "gpu out of memory",
      "category": "LLM"
    }
  }
}
```

### Beispiel 3: Natural Language

```
User: "Welche Fehler können bei LLM-Operationen auftreten?"

ThemisDB: "Ich kann folgende LLM-Fehler behandeln:

**LLM** (5 Fehlertypen)
- [2000] Model file not found
- [2001] Model load failed
- [2002] Context creation failed
- [2003] Inference timeout
- [2004] GPU out of memory

Frage mich nach spezifischen Fehlern für Details zur Ursache und Lösung."
```

```
User: "Was bedeutet Fehler 2004?"

ThemisDB: "**Fehler 2004: GPU out of memory**

**Kategorie:** LLM
**Schweregrad:** Critical

**Ursache:**
Insufficient GPU VRAM to load the model or process the request.

**Lösung:**
1. Use a smaller model or quantized version (Q4_K_M, Q5_K_M)
2. Reduce context window size
3. Enable model offloading to CPU
4. Close other GPU applications
5. Use multiple GPUs with tensor parallelism

**Dokumentation:** /docs/llm/gpu_management.md, /docs/llm/quantization.md"
```

---

## 🎯 Migration bestehender Logs

### Automatische Code-Migration

**Script:** `tools/migrate_error_logs.py`

```python
#!/usr/bin/env python3
import re
import sys

# Mapping: Old log message → Error Code
ERROR_MAPPINGS = {
    r'Model file not found: (.*)': ('ERR_LLM_MODEL_NOT_FOUND', '2000'),
    r'LoRA not loaded: (.*)': ('ERR_LORA_NOT_LOADED', '2100'),
    r'Failed to load model from file: (.*)': ('ERR_LLM_MODEL_LOAD_FAILED', '2001'),
    # ... more mappings
}

def migrate_file(filepath):
    with open(filepath, 'r') as f:
        content = f.read()
    
    for pattern, (error_name, error_code) in ERROR_MAPPINGS.items():
        # Replace spdlog::error with logError
        old_pattern = f'spdlog::error\\("{pattern}"'
        new_pattern = f'errors::logError(errors::ErrorCode::{error_name}'
        
        content = re.sub(old_pattern, new_pattern, content)
    
    with open(filepath, 'w') as f:
        f.write(content)
    
    print(f"Migrated: {filepath}")

if __name__ == '__main__':
    for filepath in sys.argv[1:]:
        migrate_file(filepath)
```

**Usage:**
```bash
python tools/migrate_error_logs.py src/llm/*.cpp
```

---

## 📊 ROI-Analyse

### Aufwand

| Phase | LOC | Dauer | Priorität |
|-------|-----|-------|-----------|
| Phase 1: Error Registry | ~500 LOC | 1-2 Sprints | HOCH |
| Phase 2: REST API | ~200 LOC | 1 Sprint | MITTEL |
| Phase 3: MCP Tools | ~150 LOC | 1 Sprint | MITTEL |
| Phase 4: Natural Language | ~200 LOC | 1-2 Sprints | MITTEL |
| Migration bestehender Logs | ~100 LOC | 2-3 Sprints | NIEDRIG |
| **TOTAL** | **~1150 LOC** | **5-8 Sprints** | - |

### Nutzen

✅ **Self-Awareness über Fehler**
- Nutzer können Fehler selbst nachschlagen
- Reduziert Support-Anfragen
- Schnellere Problemlösung

✅ **Strukturierte Error-Behandlung**
- Einheitliche Error Codes
- Maschinenlesbare Metadaten
- Einfache Dokumentation

✅ **LLM-Integration**
- Natural Language Error-Queries
- Intelligente Fehleranalyse
- Kontextuelle Hilfe

---

## 🚀 Nächste Schritte

1. **Phase 1 implementieren** - Error Registry als Foundation
2. **Top 50 Errors definieren** - Aus bestehendem Code extrahieren
3. **REST API hinzufügen** - Für Introspection
4. **MCP Tools integrieren** - Für LLM-Zugriff
5. **Natural Language erweitern** - Error-Queries unterstützen
6. **(Optional) Logs migrieren** - Schrittweise auf Error Codes umstellen

---

## 📚 Integration mit Self-Awareness

**Update für DETAILED_IMPLEMENTATION_GUIDE.md:**

Add **Phase 7: Error Awareness** nach Phase 6:

```
Phase 7: Error Awareness (MITTEL, ~1150 LOC, 5-8 sprints)
- Error Registry mit strukturierten Error Codes
- REST API: /api/v1/errors, /api/v1/errors/:code
- MCP Tools: get_error_info, search_errors
- Natural Language Integration für Error-Queries
```

**Update für toolIntrospectDatabase():**

Erweitere um Error-Awareness:
- "Welche Fehler können auftreten?"
- "Was bedeutet Fehler X?"
- "Wie behebe ich Problem Y?"

---

**Erstellt:** 11. Januar 2026  
**Version:** 1.0  
**Status:** Design-Ready  
**Integration:** Self-Awareness Phase 7
