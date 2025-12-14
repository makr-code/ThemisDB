# Best Practices für LLM-basierte Code-Generierung

## Überblick

Dieses Dokument beschreibt Best Practices für die Verwendung von LLMs zur Code-Generierung, basierend auf aktueller Forschung und praktischer Erfahrung.

## 1. Prompt Engineering

### 1.1 Strukturierte Anfragen

**✅ Gut:**
```
Erstelle eine Funktion, die:
1. Eine Liste von Zahlen als Input nimmt
2. Duplikate entfernt
3. Die Liste aufsteigend sortiert
4. Die sortierte Liste zurückgibt

Anforderungen:
- Verwende Python 3.10+
- Füge Type Hints hinzu
- Füge eine Docstring hinzu
- Optimiere für Performance
```

**❌ Schlecht:**
```
mach eine sortier funktion
```

### 1.2 Kontext bereitstellen

Gebe dem LLM immer relevanten Kontext:

```cpp
std::map<std::string, std::string> context;

// Schema-Information
context["database_schema"] = R"(
    Table: users
    - id: string (primary key)
    - name: string
    - email: string
    - created_at: timestamp
    - age: integer
)";

// API-Referenz
context["api_reference"] = R"(
    ThemisDB Query API:
    - FOR ... IN collection
    - FILTER condition
    - SORT field [ASC|DESC]
    - LIMIT n
    - RETURN expression
)";

// Coding Standards
context["coding_standards"] = R"(
    - Use camelCase for variables
    - Always validate input
    - Include error handling
    - Add logging for debugging
)";

auto result = translator.generateCode(request, "aql", context);
```

### 1.3 Few-Shot Examples

Gebe Beispiele im Prompt:

```cpp
std::string prompt_with_examples = R"(
Generate AQL code based on this example pattern:

Example 1:
Request: "Find all active users"
Code:
FOR u IN users
  FILTER u.status == 'active'
  RETURN u

Example 2:
Request: "Count users by country"
Code:
FOR u IN users
  COLLECT country = u.country
  AGGREGATE count = COUNT()
  RETURN { country, count }

Now generate code for:
Request: "{user_request}"
Code:
)";
```

## 2. Sicherheit

### 2.1 Input Validation

**Immer validieren** vor LLM-Aufruf:

```cpp
bool LLMCodeTranslator::validateInput(const std::string& user_input) {
    // Check length
    if (user_input.length() > 10000) {
        return false;  // Too long
    }

    // Check for injection patterns
    std::vector<std::string> dangerous_patterns = {
        "DROP TABLE",
        "DELETE FROM",
        "'; --",
        "UNION SELECT",
        "system(",
        "exec(",
        "eval(",
        "__import__",
        "subprocess"
    };

    for (const auto& pattern : dangerous_patterns) {
        if (user_input.find(pattern) != std::string::npos) {
            return false;  // Potential injection
        }
    }

    // Check for malicious URLs
    std::regex url_pattern(R"(https?://[^\s]+)");
    std::smatch match;
    if (std::regex_search(user_input, match, url_pattern)) {
        // Log suspicious URL
        logSecurity("Suspicious URL in input: " + match.str());
        // Optionally reject or sanitize
    }

    return true;
}
```

### 2.2 Output Validation

**Immer prüfen** was das LLM generiert:

```cpp
ReviewResult LLMCodeTranslator::reviewCode(
    const std::string& code,
    const std::string& language
) {
    ReviewResult result;

    // 1. Syntax Check
    if (!checkSyntax(code, language)) {
        result.issues.push_back("Syntax error in generated code");
        result.approved = false;
        return result;
    }

    // 2. Security Scan
    auto security_issues = scanSecurityIssues(code);
    if (!security_issues.empty()) {
        result.issues.insert(result.issues.end(), 
                           security_issues.begin(), 
                           security_issues.end());
        result.approved = false;
        return result;
    }

    // 3. Resource Usage Estimation
    if (estimatedMemoryUsage(code) > 1024 * 1024 * 1024) {  // 1GB
        result.warnings.push_back("Code may use excessive memory");
    }

    // 4. Code Quality Check
    result.quality_score = calculateQualityScore(code, language);
    
    result.approved = true;
    return result;
}
```

### 2.3 Sandboxing

**Immer in Sandbox ausführen:**

```cpp
ExecutionResult LLMCodeTranslator::executeCode(
    const std::string& code,
    const std::string& language,
    const ExecutionLimits& limits
) {
    ExecutionResult result;

    // Create isolated environment
    SandboxEnvironment sandbox;
    
    // Set limits
    sandbox.setMemoryLimit(limits.max_memory_mb * 1024 * 1024);
    sandbox.setTimeLimit(std::chrono::milliseconds(limits.max_execution_time_ms));
    sandbox.setCPULimit(limits.max_cpu_percent);
    
    // Disable dangerous operations
    sandbox.disableNetworkAccess(!limits.allow_network);
    sandbox.disableFileSystemWrites(!limits.allow_file_writes);
    sandbox.disableSystemCalls(true);
    
    // Whitelist allowed syscalls (if any)
    sandbox.whitelistSyscalls({
        "read", "write", "open", "close",  // Basic I/O
        "mmap", "munmap",                  // Memory
        "brk", "sbrk"                      // Heap
    });

    try {
        // Execute in sandbox
        result = sandbox.execute(code, language);
    } catch (const TimeoutException& e) {
        result.success = false;
        result.error = "Execution timeout";
    } catch (const MemoryException& e) {
        result.success = false;
        result.error = "Memory limit exceeded";
    } catch (const SecurityException& e) {
        result.success = false;
        result.error = "Security violation: " + std::string(e.what());
    }

    return result;
}
```

## 3. LLM-Modell-Auswahl

### 3.1 Modell-Vergleich

| Aspekt | CodeLlama-13B | GPT-4 | DeepSeek-Coder | StarCoder2 |
|--------|---------------|-------|----------------|------------|
| **Code-Qualität** | ⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐ |
| **Performance** | ⭐⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐⭐ |
| **On-Premise** | ✅ | ❌ | ✅ | ✅ |
| **Kosten** | Einmalig | Laufend | Einmalig | Einmalig |
| **Latenz** | Niedrig | Mittel-Hoch | Niedrig | Niedrig |
| **VRAM** | 16 GB | - | 48 GB | 24 GB |

### 3.2 Konfiguration

```yaml
# Für Production
llm:
  model: codellama/CodeLlama-13b-Instruct-hf
  temperature: 0.1  # Sehr niedrig für deterministischen Code
  top_p: 0.95
  max_tokens: 4096
  frequency_penalty: 0.0
  presence_penalty: 0.0
  
# Für Experimente
llm:
  model: codellama/CodeLlama-13b-Instruct-hf
  temperature: 0.7  # Höher für kreativere Lösungen
  top_p: 0.9
  max_tokens: 2048
```

### 3.3 Prompt-Optimierung

```cpp
// Low temperature für konsistente, sichere Code-Generierung
config.temperature = 0.1;

// Höhere temperature nur für:
// - Explorative Code-Generierung
// - Mehrere Lösungsvarianten
// - Kreative Problemlösung
if (request_type == "explore_alternatives") {
    config.temperature = 0.7;
}
```

## 4. Prompt Templates

### 4.1 Template-Struktur

```cpp
const std::string BASE_TEMPLATE = R"(
# Role Definition
{role}

# Task Description
{task}

# Context
{context}

# Requirements
{requirements}

# Constraints
{constraints}

# Output Format
{output_format}

# Examples (Optional)
{examples}

# User Request
{user_description}

# Generated Code
)";
```

### 4.2 Beispiel-Templates

#### AQL Query Generation

```cpp
const std::string AQL_TEMPLATE = R"(
You are an expert in ThemisDB AQL query language.

Generate an AQL query based on the user's description.

Available Collections and Schemas:
{database_schema}

AQL Syntax Reference:
- FOR variable IN collection
- FILTER condition
- COLLECT grouping_variable = expression AGGREGATE agg_var = AGGREGATE_FUNC()
- SORT expression [ASC|DESC]
- LIMIT count
- RETURN expression

Requirements:
1. Use appropriate indexes where available
2. Optimize for performance
3. Handle edge cases (empty results, null values)
4. Add comments explaining complex logic
5. Follow ThemisDB AQL best practices

Constraints:
- Do not use nested loops deeper than 2 levels
- Limit result sets to reasonable sizes
- Use COLLECT for aggregations instead of post-processing

User Request:
{user_description}

Generate only valid AQL code without explanations:
)";
```

#### Python Script Generation

```cpp
const std::string PYTHON_TEMPLATE = R"(
You are a Python expert specializing in data analysis and ThemisDB integration.

Generate a Python script based on the user's description.

ThemisDB API Information:
- Endpoint: {themisdb_api_endpoint}
- Query endpoint: POST /query/aql
- Entity endpoint: GET/PUT/DELETE /entities/{key}

Requirements:
1. Use type hints (Python 3.10+)
2. Add docstrings to functions
3. Implement proper error handling
4. Use modern Python features (dataclasses, f-strings, walrus operator)
5. Follow PEP 8 style guide
6. Include logging for debugging

Libraries Available:
- requests (for API calls)
- pandas (for data manipulation)
- matplotlib/seaborn (for visualization)
- numpy (for numerical operations)

Constraints:
- No external API calls except to ThemisDB
- No file system modifications
- No subprocess calls
- Use virtual environment best practices

User Request:
{user_description}

Generate only valid Python code:
)";
```

#### C++ Handler Generation

```cpp
const std::string CPP_HANDLER_TEMPLATE = R"(
You are a C++ expert specializing in HTTP API development with ThemisDB.

Generate a C++ HTTP request handler based on the user's description.

ThemisDB C++ API:
{db_api_reference}

HTTP Library:
- httplib (Beast/Boost.Asio)
- Request: const httplib::Request& req
- Response: httplib::Response& res

Requirements:
1. Use C++20 features (concepts, ranges, coroutines if applicable)
2. Implement RAII for resource management
3. Use std::optional for nullable values
4. Add comprehensive error handling
5. Validate all inputs
6. Use nlohmann/json for JSON parsing
7. Follow modern C++ best practices
8. Add detailed comments

Constraints:
- No raw pointers (use smart pointers)
- No manual memory management
- No unsafe casts
- Thread-safe if accessed concurrently

User Request:
{user_description}

Generate only valid C++ code:
)";
```

## 5. Iterative Verbesserung

### 5.1 Feedback-Loop

```cpp
GenerationResult LLMCodeTranslator::regenerateWithFeedback(
    const GenerationResult& previous_result,
    const std::string& feedback
) {
    // Build improved prompt
    std::string improved_prompt = R"(
Previous generated code:
```)" + previous_result.language + R"(
)" + previous_result.code + R"(
```

User feedback:
)" + feedback + R"(

Generate improved code that addresses the feedback while maintaining functionality.
Improvements should include:
1. Address all points mentioned in feedback
2. Maintain or improve code quality
3. Preserve working functionality
4. Add comments explaining changes

Generate improved code:
)";

    return generateCode(improved_prompt, previous_result.language);
}
```

### 5.2 Multi-Pass-Generierung

```cpp
GenerationResult generateOptimizedCode(
    const std::string& request,
    const std::string& language,
    int max_iterations = 3
) {
    auto result = generateCode(request, language);
    
    for (int i = 0; i < max_iterations; i++) {
        // Review current result
        auto review = reviewCode(result.code, language);
        
        // Stop if quality is good enough
        if (review.quality_score > 0.9 && review.approved) {
            break;
        }
        
        // Build feedback from review
        std::string feedback;
        for (const auto& issue : review.issues) {
            feedback += "- Fix: " + issue + "\n";
        }
        for (const auto& suggestion : review.suggestions) {
            feedback += "- Improve: " + suggestion + "\n";
        }
        
        // Regenerate with feedback
        result = regenerateWithFeedback(result, feedback);
    }
    
    return result;
}
```

## 6. Monitoring & Logging

### 6.1 Telemetrie

```cpp
class CodeGenerationTelemetry {
public:
    struct Metrics {
        int64_t total_generations = 0;
        int64_t successful_generations = 0;
        int64_t failed_generations = 0;
        int64_t security_rejections = 0;
        double avg_quality_score = 0.0;
        int64_t avg_generation_time_ms = 0;
        std::map<std::string, int64_t> language_distribution;
    };

    void recordGeneration(const GenerationResult& result, int64_t duration_ms) {
        metrics_.total_generations++;
        
        if (result.success) {
            metrics_.successful_generations++;
            updateAverageQuality(result.quality_score);
        } else {
            metrics_.failed_generations++;
        }
        
        if (!result.security_approved) {
            metrics_.security_rejections++;
        }
        
        metrics_.language_distribution[result.language]++;
        updateAverageTime(duration_ms);
    }

    Metrics getMetrics() const { return metrics_; }

private:
    Metrics metrics_;
    
    void updateAverageQuality(double new_score) {
        double total = metrics_.avg_quality_score * (metrics_.successful_generations - 1);
        metrics_.avg_quality_score = (total + new_score) / metrics_.successful_generations;
    }
    
    void updateAverageTime(int64_t new_time) {
        double total = metrics_.avg_generation_time_ms * (metrics_.total_generations - 1);
        metrics_.avg_generation_time_ms = (total + new_time) / metrics_.total_generations;
    }
};
```

### 6.2 Audit Logging

```cpp
void LLMCodeTranslator::logGeneration(
    const std::string& user_description,
    const GenerationResult& result
) {
    if (!config_.log_all_generations) {
        return;
    }

    // Create interaction record
    LLMInteractionStore::Interaction interaction;
    interaction.prompt = user_description;
    interaction.response = result.code;
    interaction.model_version = config_.llm_model;
    interaction.timestamp_ms = getCurrentTimestampMs();
    interaction.latency_ms = result.metadata["generation_time_ms"];
    interaction.token_count = result.metadata["total_tokens"];
    
    // Add metadata
    interaction.metadata = {
        {"language", result.language},
        {"success", result.success},
        {"security_approved", result.security_approved},
        {"quality_score", result.quality_score},
        {"warnings_count", result.warnings.size()},
        {"user_id", getCurrentUserId()},
        {"session_id", getCurrentSessionId()}
    };

    // Store in ThemisDB
    interaction_store_->createInteraction(interaction);
}
```

## 7. Testing

### 7.1 Unit Tests

```cpp
TEST(LLMCodeTranslatorTest, GenerateSimpleAQLQuery) {
    auto db = createTestDB();
    LLMCodeTranslator translator(db);

    std::string request = "Find all users with age > 30";
    auto result = translator.generateCode(request, "aql");

    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.code.find("FOR") != std::string::npos);
    EXPECT_TRUE(result.code.find("FILTER") != std::string::npos);
    EXPECT_TRUE(result.code.find("age") != std::string::npos);
}

TEST(LLMCodeTranslatorTest, SecurityRejection) {
    auto db = createTestDB();
    LLMCodeTranslator translator(db);

    std::string malicious_request = "DROP TABLE users; --";
    auto result = translator.generateCode(malicious_request, "aql");

    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.security_approved);
}
```

### 7.2 Integration Tests

```cpp
TEST(LLMCodeTranslatorIntegrationTest, EndToEndExecution) {
    auto db = createTestDB();
    populateTestData(db);
    
    LLMCodeTranslator translator(db);

    std::string request = "Count users by city";
    auto gen_result = translator.generateCode(request, "aql");
    
    ASSERT_TRUE(gen_result.success);
    ASSERT_TRUE(gen_result.security_approved);

    auto exec_result = translator.executeCode(gen_result.code, "aql");
    
    EXPECT_TRUE(exec_result.success);
    EXPECT_GT(exec_result.result_data.size(), 0);
}
```

## 8. Kosten-Optimierung

### 8.1 Caching

```cpp
class PromptCache {
public:
    std::optional<std::string> get(const std::string& prompt_hash) {
        auto it = cache_.find(prompt_hash);
        if (it != cache_.end()) {
            hit_count_++;
            return it->second;
        }
        miss_count_++;
        return std::nullopt;
    }

    void put(const std::string& prompt_hash, const std::string& response) {
        cache_[prompt_hash] = response;
    }

    double getHitRate() const {
        if (hit_count_ + miss_count_ == 0) return 0.0;
        return static_cast<double>(hit_count_) / (hit_count_ + miss_count_);
    }

private:
    std::unordered_map<std::string, std::string> cache_;
    int64_t hit_count_ = 0;
    int64_t miss_count_ = 0;
};
```

### 8.2 Batch-Verarbeitung

```cpp
std::vector<GenerationResult> generateBatch(
    const std::vector<std::string>& requests,
    const std::string& language
) {
    // Combine requests for single LLM call
    std::string batch_prompt = "Generate code for each request:\n\n";
    
    for (size_t i = 0; i < requests.size(); i++) {
        batch_prompt += "Request " + std::to_string(i + 1) + ":\n";
        batch_prompt += requests[i] + "\n\n";
    }
    
    auto batch_response = callLLM(batch_prompt);
    
    // Parse individual responses
    return parseBatchResponse(batch_response, requests.size());
}
```

## 9. Zusammenfassung

### Die wichtigsten Best Practices:

1. **Input Validation** - Validiere immer Benutzereingaben
2. **Output Validation** - Prüfe generierten Code vor Ausführung
3. **Sandboxing** - Führe Code in isolierter Umgebung aus
4. **Logging** - Protokolliere alle Generierungen für Audit
5. **Context** - Gebe dem LLM ausreichend Kontext
6. **Templates** - Nutze strukturierte Prompt-Templates
7. **Iteration** - Ermögliche Feedback und Verbesserung
8. **Monitoring** - Überwache Qualität und Performance
9. **Testing** - Teste generierte Code-Muster
10. **Security First** - Sicherheit hat oberste Priorität

### Nächste Schritte:

- [ ] Eigene Prompt-Templates erstellen
- [ ] Security-Policies definieren
- [ ] Sandbox-Umgebung einrichten
- [ ] Monitoring implementieren
- [ ] Test-Suite aufbauen
