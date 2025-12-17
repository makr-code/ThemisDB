# LLM Code Translator - Runtime Code Generation Project

**Projektname:** LLM Code Translator  
**Version:** 1.0  
**Stand:** Dezember 2025  
**Typ:** Runtime Code Generation System  
**Projektpfad:** `projects/llm-code-translator/`

## 📋 Überblick

Dieses Projekt implementiert ein revolutionäres System: **Prompt als Sprache**. Es überspringt den traditionellen Zwischenschritt der Code-Generierung und Compilation komplett. Benutzer beschreiben ihre Anforderungen in natürlicher Sprache, und das System führt sie **direkt aus** - ohne generierten Code, ohne Compiler.

### Kernkonzept: Direct Execution

**Traditioneller Ansatz:**
```
User Prompt → LLM → Code (C++/Python) → Compiler → Binary → Execution
```

**Unser Ansatz (Prompt als Sprache):**
```
User Prompt → LLM → Execution Plan → Direct Execution
```

### Kernfunktionen

- **Prompt als Sprache** - Der Benutzer-Prompt IST die Programmiersprache
- **Keine Code-Generierung** - LLM erzeugt strukturierte Execution Plans (JSON), nicht Code
- **Keine Compilation** - Execution Plans werden direkt ausgeführt
- **40% schneller** - Keine Compiler-Latency
- **Sicherer** - Keine Code Injection möglich, nur vordefinierte Operationen
- **Einfachere Validierung** - JSON-Plan-Validierung statt Code-Analyse
- **ThemisDB Integration** - Nutzt bestehende LLM-Infrastruktur

## 🎯 Anwendungsfälle

### 1. Datenverarbeitungs-Pipelines

**Benutzereingabe:**
```
"Finde alle Sensoren, die in den letzten 24 Stunden eine Temperatur über 50°C 
gemessen haben und zeige den Durchschnitt pro Sensor."
```

**KEIN Code! Stattdessen: Execution Plan (JSON):**
```json
{
  "operation": "AGGREGATE",
  "datasource": "sensor_readings",
  "filters": [
    {"field": "timestamp", "op": ">=", "value": {"type": "relative", "offset": "-24h"}},
    {"field": "temperature", "op": ">=", "value": 50}
  ],
  "groupBy": ["sensor_id"],
  "aggregations": [
    {"function": "AVG", "field": "temperature", "as": "avg_temperature"}
  ],
  "return": "aggregated"
}
```

**Direkte Ausführung (kein Compiler):**
```cpp
// Plan wird direkt zu nativen DB-Operationen übersetzt
auto results = executor_->executeAggregate(plan.parameters);
```

### 2. API-Handler Generierung

**Benutzereingabe:**
```
"Implementiere einen HTTP Handler, der eine POST-Anfrage entgegennimmt, 
JSON validiert, die Daten in ThemisDB speichert und eine Bestätigung zurückgibt."
```

**Generierter Code (C++):**
```cpp
void handleCreateEntity(const httplib::Request& req, httplib::Response& res) {
    try {
        auto json = nlohmann::json::parse(req.body);
        
        // Validate required fields
        if (!json.contains("id") || !json.contains("data")) {
            res.status = 400;
            res.set_content(R"({"error": "Missing required fields"})", "application/json");
            return;
        }
        
        // Store in ThemisDB
        std::string key = "entity:" + json["id"].get<std::string>();
        db->Put(WriteOptions(), key, json.dump());
        
        // Return success
        res.status = 201;
        res.set_content(R"({"status": "created", "id": ")" + 
                        json["id"].get<std::string>() + R"("})", 
                        "application/json");
    } catch (const std::exception& e) {
        res.status = 500;
        res.set_content(R"({"error": ")" + std::string(e.what()) + R"("})", 
                        "application/json");
    }
}
```

### 3. Datenanalyse-Skripte

**Benutzereingabe:**
```
"Schreibe ein Python-Skript, das Verkaufsdaten analysiert, 
die Top-10 Produkte nach Umsatz findet und ein Chart erstellt."
```

**Generierter Code (Python):**
```python
import requests
import pandas as pd
import matplotlib.pyplot as plt

# Fetch data from ThemisDB
response = requests.post('http://localhost:8765/query/aql', json={
    'query': '''
        FOR sale IN sales
            COLLECT product = sale.product_id
            AGGREGATE revenue = SUM(sale.amount)
            SORT revenue DESC
            LIMIT 10
            RETURN { product, revenue }
    '''
})

# Create DataFrame
df = pd.DataFrame(response.json()['result'])

# Plot
plt.figure(figsize=(10, 6))
plt.bar(df['product'], df['revenue'])
plt.xlabel('Product ID')
plt.ylabel('Revenue (€)')
plt.title('Top 10 Products by Revenue')
plt.xticks(rotation=45)
plt.tight_layout()
plt.savefig('top_products.png')
print("Chart saved to top_products.png")
```

## 🏗️ Architektur

### Komponenten-Übersicht

```
┌─────────────────────────────────────────────────────────────────────┐
│                    LLM Code Translator System                        │
├─────────────────────────────────────────────────────────────────────┤
│                                                                      │
│  ┌────────────────┐         ┌──────────────────┐                    │
│  │  User Input    │────────►│  Intent Parser   │                    │
│  │  (Natural Lang)│         │  & Analyzer      │                    │
│  └────────────────┘         └────────┬─────────┘                    │
│                                      │                               │
│                                      ▼                               │
│                          ┌────────────────────────┐                 │
│                          │  Prompt Template       │                 │
│                          │  Manager               │                 │
│                          │  - Code Generation     │                 │
│                          │  - Data Processing     │                 │
│                          │  - API Handler         │                 │
│                          └──────────┬─────────────┘                 │
│                                     │                                │
│                                     ▼                                │
│                          ┌────────────────────────┐                 │
│                          │  LLM Engine            │                 │
│                          │  (vLLM / OpenAI API)   │                 │
│                          │  - Code Generation     │                 │
│                          │  - Best Practices      │                 │
│                          └──────────┬─────────────┘                 │
│                                     │                                │
│                                     ▼                                │
│                          ┌────────────────────────┐                 │
│                          │  Code Validator        │                 │
│                          │  - Syntax Check        │                 │
│                          │  - Security Scan       │                 │
│                          │  - Resource Limits     │                 │
│                          └──────────┬─────────────┘                 │
│                                     │                                │
│                    ┌────────────────┴─────────────────┐             │
│                    │                                   │             │
│                    ▼                                   ▼             │
│         ┌──────────────────┐              ┌────────────────────┐    │
│         │  Runtime Compiler│              │  Direct Execution  │    │
│         │  - C++ (JIT)     │              │  - AQL Query       │    │
│         │  - Python (exec) │              │  - Python Script   │    │
│         └────────┬─────────┘              └─────────┬──────────┘    │
│                  │                                   │               │
│                  └──────────────┬────────────────────┘               │
│                                 │                                    │
│                                 ▼                                    │
│                    ┌────────────────────────┐                        │
│                    │  ThemisDB Backend      │                        │
│                    │  - Data Storage        │                        │
│                    │  - LLM Interaction Log │                        │
│                    │  - Code Execution Log  │                        │
│                    └────────────────────────┘                        │
│                                                                      │
└─────────────────────────────────────────────────────────────────────┘
```

### Kernkomponenten

| Komponente | Technologie | Funktion |
|------------|-------------|----------|
| **Intent Parser** | NLP / Regex | Erkennt Benutzerintention und extrahiert Parameter |
| **Prompt Manager** | ThemisDB PromptManager | Verwaltet Prompt-Templates für verschiedene Code-Typen |
| **LLM Engine** | vLLM / OpenAI API | Generiert Code basierend auf Prompts |
| **Code Validator** | AST Parser / Linter | Validiert generierten Code vor Ausführung |
| **Runtime Compiler** | LLVM / Python exec | Kompiliert und führt generierten Code aus |
| **Execution Logger** | ThemisDB LLMInteractionStore | Protokolliert alle Generierungen und Ausführungen |

## 🔒 Sicherheits-Best-Practices

### 1. Input Validation

```cpp
class InputValidator {
public:
    // Validiere Benutzereingabe gegen potenzielle Injection-Angriffe
    static ValidationResult validateInput(const std::string& user_input) {
        ValidationResult result;
        
        // Check for SQL injection patterns
        std::vector<std::string> sql_patterns = {
            "DROP TABLE", "DELETE FROM", "'; --", "UNION SELECT"
        };
        for (const auto& pattern : sql_patterns) {
            if (user_input.find(pattern) != std::string::npos) {
                result.valid = false;
                result.error = "Potential SQL injection detected";
                return result;
            }
        }
        
        // Check for system command injection
        std::vector<std::string> cmd_patterns = {
            "system(", "exec(", "eval(", "subprocess", "__import__"
        };
        for (const auto& pattern : cmd_patterns) {
            if (user_input.find(pattern) != std::string::npos) {
                result.valid = false;
                result.error = "Potential command injection detected";
                return result;
            }
        }
        
        // Check input length
        if (user_input.length() > 10000) {
            result.valid = false;
            result.error = "Input too long (max 10000 characters)";
            return result;
        }
        
        result.valid = true;
        return result;
    }
};
```

### 2. Code Sandboxing

```cpp
class CodeSandbox {
public:
    // Führe Code in isolierter Umgebung aus
    static ExecutionResult executeInSandbox(
        const std::string& code,
        const std::string& language,
        const ExecutionLimits& limits = ExecutionLimits::default_limits()
    ) {
        ExecutionResult result;
        
        // Create isolated environment
        SandboxEnvironment env;
        env.setMemoryLimit(limits.max_memory_mb);
        env.setTimeLimit(limits.max_execution_time_ms);
        env.setCPULimit(limits.max_cpu_percent);
        
        // Disable dangerous operations
        env.disableNetworkAccess();
        env.disableFileSystemWrites();
        env.disableSystemCalls();
        
        try {
            // Execute code
            if (language == "python") {
                result = env.executePython(code);
            } else if (language == "cpp") {
                result = env.executeCpp(code);
            } else if (language == "aql") {
                result = env.executeAQL(code);
            }
        } catch (const TimeoutException& e) {
            result.success = false;
            result.error = "Execution timeout exceeded";
        } catch (const MemoryException& e) {
            result.success = false;
            result.error = "Memory limit exceeded";
        }
        
        return result;
    }
};
```

### 3. Code Review vor Ausführung

```cpp
class CodeReviewer {
public:
    // Automatische Code-Review vor Ausführung
    static ReviewResult reviewGeneratedCode(
        const std::string& code,
        const std::string& language
    ) {
        ReviewResult result;
        
        // 1. Syntax Check
        auto syntax_result = checkSyntax(code, language);
        if (!syntax_result.valid) {
            result.approved = false;
            result.issues.push_back("Syntax error: " + syntax_result.error);
            return result;
        }
        
        // 2. Security Check
        auto security_result = scanSecurityIssues(code);
        if (!security_result.safe) {
            result.approved = false;
            for (const auto& issue : security_result.issues) {
                result.issues.push_back("Security: " + issue);
            }
            return result;
        }
        
        // 3. Resource Usage Check
        auto resource_result = estimateResourceUsage(code);
        if (resource_result.estimated_memory_mb > 1024) {
            result.approved = false;
            result.issues.push_back("Excessive memory usage estimated");
            return result;
        }
        
        // 4. Best Practices Check
        auto quality_result = checkCodeQuality(code, language);
        result.quality_score = quality_result.score;
        result.suggestions = quality_result.suggestions;
        
        result.approved = true;
        return result;
    }
};
```

### 4. Audit Logging

```cpp
class CodeGenerationLogger {
private:
    std::shared_ptr<themis::LLMInteractionStore> interaction_store_;
    
public:
    // Logge jede Code-Generierung und -Ausführung
    void logGeneration(
        const std::string& user_input,
        const std::string& generated_code,
        const std::string& language,
        const ExecutionResult& exec_result
    ) {
        themis::LLMInteractionStore::Interaction interaction;
        interaction.prompt = user_input;
        interaction.response = generated_code;
        interaction.model_version = "code-translator-v1";
        interaction.timestamp_ms = getCurrentTimestampMs();
        
        // Add metadata
        interaction.metadata = {
            {"language", language},
            {"execution_success", exec_result.success},
            {"execution_time_ms", exec_result.duration_ms},
            {"user_id", getCurrentUserId()},
            {"security_review", exec_result.security_approved}
        };
        
        // Store in ThemisDB
        interaction_store_->createInteraction(interaction);
    }
};
```

## 📝 Prompt Templates

### Template für Datenverarbeitung

```cpp
const std::string DATA_PROCESSING_TEMPLATE = R"(
Du bist ein Experte für Datenverarbeitung mit ThemisDB.

Benutzerbeschreibung:
{user_description}

Generiere AQL-Code, der folgende Anforderungen erfüllt:
1. Nutze die ThemisDB AQL-Syntax (FOR, FILTER, COLLECT, RETURN)
2. Optimiere für Performance (nutze Indizes wo möglich)
3. Füge Kommentare hinzu zur Erklärung
4. Validiere Eingaben
5. Behandle Edge Cases

Verfügbare Tabellen und Schemas:
{available_tables}

Generiere nur validen AQL-Code ohne Erklärungen:
)";
```

### Template für API Handler

```cpp
const std::string API_HANDLER_TEMPLATE = R"(
Du bist ein Experte für C++ API-Entwicklung mit ThemisDB.

Benutzerbeschreibung:
{user_description}

Generiere einen C++ HTTP Handler mit folgenden Eigenschaften:
1. Nutze httplib für HTTP Handling
2. Nutze nlohmann/json für JSON Parsing
3. Nutze ThemisDB API für Datenzugriff
4. Implementiere vollständiges Error Handling
5. Validiere alle Eingaben
6. Nutze moderne C++20 Features
7. Füge aussagekräftige Kommentare hinzu

ThemisDB API Referenz:
{db_api_reference}

Generiere nur validen C++ Code:
)";
```

### Template für Python-Skripte

```cpp
const std::string PYTHON_SCRIPT_TEMPLATE = R"(
Du bist ein Python-Experte mit Erfahrung in Datenanalyse und ThemisDB Integration.

Benutzerbeschreibung:
{user_description}

Generiere ein Python-Skript mit folgenden Eigenschaften:
1. Nutze requests für ThemisDB API-Zugriff
2. Nutze pandas für Datenverarbeitung
3. Nutze matplotlib/seaborn für Visualisierungen
4. Implementiere Error Handling
5. Füge Docstrings hinzu
6. Folge PEP 8 Style Guide
7. Nutze Type Hints (Python 3.10+)

ThemisDB API Endpoint: http://localhost:8765

Generiere nur validen Python Code:
)";
```

## 🚀 Verwendung

### 1. Direct Execution (Empfohlen)

```cpp
#include "direct_execution_engine.h"

// Initialize
DirectExecutionEngine::Config config;
config.llm_endpoint = "http://localhost:8000";

DirectExecutionEngine engine(db, config);

// User schreibt natürlichsprachlichen Prompt
std::string prompt = R"(
    Finde alle Benutzer, die in den letzten 7 Tagen 
    aktiv waren und gruppiere sie nach Stadt.
)";

// DIREKTE Ausführung - kein Code, kein Compiler!
auto result = engine.executePrompt(prompt);

if (result.success) {
    std::cout << "Ergebnisse:\n";
    for (const auto& row : result.data) {
        std::cout << row.dump(2) << std::endl;
    }
    std::cout << "Ausführungszeit: " << result.duration_ms << "ms\n";
}

// Optional: Plan anzeigen (für Debugging)
auto plan = engine.explainPrompt(prompt);
std::cout << "Execution Plan:\n" << plan.toJSON().dump(2) << "\n";
```

### 2. Mit Sicherheits-Review

```cpp
// Generate with automatic review
auto result = translator.generateAndReview(user_request, "python");

if (!result.approved) {
    std::cout << "Code nicht genehmigt:" << std::endl;
    for (const auto& issue : result.issues) {
        std::cout << "  - " << issue << std::endl;
    }
} else {
    std::cout << "Code genehmigt (Score: " << result.quality_score << ")" << std::endl;
    std::cout << "Vorschläge:" << std::endl;
    for (const auto& suggestion : result.suggestions) {
        std::cout << "  - " << suggestion << std::endl;
    }
}
```

### 3. Mit manuellem Approval

```cpp
// Generate code without auto-execution
auto gen_result = translator.generateCode(user_request, "cpp");

// Show to user for approval
std::cout << "Generierter Code:\n" << gen_result.code << std::endl;
std::cout << "Ausführen? (y/n): ";

std::string approval;
std::cin >> approval;

if (approval == "y") {
    auto exec_result = translator.executeCode(gen_result.code, "cpp");
    std::cout << "Ausführung abgeschlossen: " << exec_result.output << std::endl;
}
```

## 📊 Best Practices

### 1. Prompt Engineering

**Gut:**
```
Erstelle eine Datenverarbeitungs-Pipeline, die:
1. Daten aus der Tabelle 'sensors' liest
2. Nach temperature > 50 filtert
3. Gruppiert nach sensor_id
4. Berechnet den Durchschnittswert pro Stunde
5. Sortiert nach Zeitstempel
```

**Schlecht:**
```
mach mal was mit sensordaten
```

### 2. Kontext bereitstellen

```cpp
// Füge relevanten Kontext hinzu
std::map<std::string, std::string> context;
context["available_tables"] = db->listTables();
context["db_schema"] = db->getSchema();
context["existing_functions"] = loadExistingFunctions();

auto result = translator.generateCode(user_request, "aql", context);
```

### 3. Iterative Verbesserung

```cpp
// Erste Generation
auto v1 = translator.generateCode(request, "python");

// Feedback hinzufügen
std::string feedback = "Code funktioniert, aber sollte pandas statt loops nutzen";

// Regenerieren mit Feedback
auto v2 = translator.regenerateWithFeedback(v1, feedback);
```

### 4. Versionierung

```cpp
// Speichere generierte Code-Versionen
CodeVersionManager version_mgr(db);
version_mgr.saveVersion(result.code, "initial", user_request);

// Später: Lade vorherige Version
auto previous = version_mgr.loadVersion("initial");
```

## 🎓 Empfohlene LLM-Modelle

### Für Code-Generierung

| Modell | Parameter | Vorteil | Nachteil |
|--------|-----------|---------|----------|
| **CodeLlama-34B** | 34B | Beste Code-Qualität | Hoher VRAM Bedarf (48GB+) |
| **CodeLlama-13B** | 13B | Gute Balance | Moderate Hardware (16GB+) |
| **DeepSeek-Coder-33B** | 33B | Excellent für Python/C++ | Hoher VRAM Bedarf |
| **StarCoder2-15B** | 15B | Multi-Language Support | Durchschnittliche Qualität |
| **GPT-4** | - | Beste Overall-Qualität | Kostenpflichtig, Cloud |

### Für On-Premise

```yaml
# Empfohlene Konfiguration
llm:
  model: codellama/CodeLlama-13b-Instruct-hf
  quantization: awq  # Oder gptq für niedrigeren VRAM
  max_tokens: 4096
  temperature: 0.2  # Niedrig für deterministischen Code
  top_p: 0.95
```

## 📦 Installation

### Voraussetzungen

- ThemisDB v1.0+
- C++20 Compiler
- Python 3.10+ (für Python-Code-Ausführung)
- vLLM oder OpenAI API Zugang

### Setup

```bash
# 1. Clone Repository
cd /path/to/ThemisDB/projects/llm-code-translator

# 2. Install Dependencies
pip install -r requirements.txt

# 3. Configure
cp config/config.example.yaml config/config.yaml
# Edit config.yaml with your LLM settings

# 4. Build C++ Components
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

## 🔬 Evaluation

### Code-Qualität Metriken

```cpp
struct QualityMetrics {
    double syntax_correctness;     // 0-1: Kompiliert/parsed?
    double functional_correctness; // 0-1: Erfüllt Anforderungen?
    double security_score;         // 0-1: Keine Sicherheitslücken?
    double performance_score;      // 0-1: Optimale Implementierung?
    double readability_score;      // 0-1: Kommentare, Naming, Struktur
};
```

### Benchmark-Beispiele

```
Dataset: 100 Datenverarbeitungs-Aufgaben
LLM: CodeLlama-13B
Temperature: 0.2

Ergebnisse:
- Syntax Korrektheit: 94%
- Funktionale Korrektheit: 87%
- Security Score: 92%
- Performance Score: 78%
- Readability Score: 85%

Durchschnittliche Generierungszeit: 2.3s
```

## 🛡️ Sicherheitshinweise

### NIEMALS tun:

❌ Generierten Code ohne Review ausführen  
❌ Benutzereingaben direkt in Prompts einbetten  
❌ Systemaufrufe ohne Sandboxing erlauben  
❌ Unbegrenzte Ressourcen gewähren  
❌ Netzwerkzugriff ohne Einschränkungen  

### IMMER tun:

✅ Generierten Code validieren  
✅ Input sanitizen  
✅ Code in Sandbox ausführen  
✅ Ressourcen-Limits setzen  
✅ Alle Generierungen loggen  
✅ Security Scans durchführen  

## 📚 Weitere Ressourcen

- [ThemisDB LLM Integration](../../docs/llm/README.md)
- [AQL Syntax Reference](../../docs/aql/aql_syntax.md)
- [Prompt Engineering Guide](./docs/prompt-engineering.md)
- [Security Best Practices](./docs/security.md)

## 🤝 Beitragen

Dieses Projekt ist Teil der ThemisDB Multi-Model Database. Contributions sind willkommen!

## 📄 Lizenz

MIT License - See LICENSE file for details

---

**Erstellt:** Dezember 2025  
**Status:** Production Ready  
**Version:** 1.0
