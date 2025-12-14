# LLM Code Translator - Architecture Documentation

## Überblick

Das LLM Code Translator System ermöglicht die Übersetzung von natürlichsprachlichen Beschreibungen in ausführbaren Maschinencode. Diese Architektur-Dokumentation beschreibt die technischen Details der Implementierung.

## System-Architektur

### High-Level Komponenten

```
┌─────────────────────────────────────────────────────────────────────────┐
│                         LLM Code Translator                              │
│                                                                          │
│  ┌──────────────────┐                                                   │
│  │  User Interface  │                                                   │
│  │  - CLI           │                                                   │
│  │  - API           │                                                   │
│  │  - IDE Plugin    │                                                   │
│  └────────┬─────────┘                                                   │
│           │                                                              │
│           v                                                              │
│  ┌──────────────────────────────────────────────────────────────┐      │
│  │               Core Translation Engine                        │      │
│  │  ┌────────────┐  ┌────────────┐  ┌─────────────┐            │      │
│  │  │  Input     │  │  Prompt    │  │  LLM        │            │      │
│  │  │  Validator │→ │  Builder   │→ │  Client     │            │      │
│  │  └────────────┘  └────────────┘  └──────┬──────┘            │      │
│  │                                          v                   │      │
│  │                                  ┌─────────────┐             │      │
│  │                                  │  Response   │             │      │
│  │                                  │  Parser     │             │      │
│  │                                  └──────┬──────┘             │      │
│  └─────────────────────────────────────────┼────────────────────┘      │
│                                             v                            │
│  ┌──────────────────────────────────────────────────────────────┐      │
│  │               Code Validation & Review                       │      │
│  │  ┌────────────┐  ┌────────────┐  ┌─────────────┐            │      │
│  │  │  Syntax    │  │  Security  │  │  Quality    │            │      │
│  │  │  Checker   │→ │  Scanner   │→ │  Analyzer   │            │      │
│  │  └────────────┘  └────────────┘  └──────┬──────┘            │      │
│  └─────────────────────────────────────────┼────────────────────┘      │
│                                             v                            │
│  ┌──────────────────────────────────────────────────────────────┐      │
│  │               Execution Engine                               │      │
│  │  ┌────────────┐  ┌────────────┐  ┌─────────────┐            │      │
│  │  │  Sandbox   │  │  Runtime   │  │  Result     │            │      │
│  │  │  Setup     │→ │  Executor  │→ │  Collector  │            │      │
│  │  └────────────┘  └────────────┘  └──────┬──────┘            │      │
│  └─────────────────────────────────────────┼────────────────────┘      │
│                                             v                            │
│  ┌──────────────────────────────────────────────────────────────┐      │
│  │               Logging & Storage                              │      │
│  │  ┌────────────┐  ┌────────────┐  ┌─────────────┐            │      │
│  │  │  Audit     │  │  Metrics   │  │  ThemisDB   │            │      │
│  │  │  Logger    │  │  Collector │  │  Store      │            │      │
│  │  └────────────┘  └────────────┘  └─────────────┘            │      │
│  └──────────────────────────────────────────────────────────────┘      │
│                                                                          │
└─────────────────────────────────────────────────────────────────────────┘

External Dependencies:
┌─────────────┐  ┌─────────────┐  ┌─────────────┐
│ LLM Service │  │  ThemisDB   │  │  Compiler/  │
│ (vLLM/GPT)  │  │  Backend    │  │  Interpreter│
└─────────────┘  └─────────────┘  └─────────────┘
```

## Kernkomponenten

### 1. Input Validator

**Zweck:** Validiert und sanitisiert Benutzereingaben vor der Verarbeitung

**Funktionen:**
- Input-Längen-Prüfung
- Injection-Pattern-Erkennung
- URL/Link-Validierung
- Encoding-Validierung

**Implementierung:**
```cpp
class InputValidator {
public:
    struct ValidationResult {
        bool valid;
        std::string error;
        std::vector<std::string> warnings;
    };

    ValidationResult validate(const std::string& input) {
        ValidationResult result{true, "", {}};
        
        // Length check
        if (input.length() > max_input_length_) {
            result.valid = false;
            result.error = "Input exceeds maximum length";
            return result;
        }
        
        // Pattern scanning
        for (const auto& pattern : dangerous_patterns_) {
            if (input.find(pattern) != std::string::npos) {
                result.valid = false;
                result.error = "Dangerous pattern detected: " + pattern;
                return result;
            }
        }
        
        // Encoding validation
        if (!isValidUTF8(input)) {
            result.warnings.push_back("Non-UTF8 characters detected");
        }
        
        return result;
    }

private:
    size_t max_input_length_;
    std::vector<std::string> dangerous_patterns_;
};
```

### 2. Prompt Builder

**Zweck:** Erstellt strukturierte Prompts für das LLM basierend auf Templates

**Template-System:**
```cpp
class PromptBuilder {
public:
    struct PromptContext {
        std::string user_description;
        std::string target_language;
        std::map<std::string, std::string> context_data;
        std::vector<std::string> examples;
    };

    std::string buildPrompt(const PromptContext& ctx) {
        // Load template for language
        std::string template_text = loadTemplate(ctx.target_language);
        
        // Replace placeholders
        template_text = replacePlaceholder(template_text, 
                                          "{user_description}", 
                                          ctx.user_description);
        
        // Add context
        for (const auto& [key, value] : ctx.context_data) {
            template_text = replacePlaceholder(template_text, 
                                              "{" + key + "}", 
                                              value);
        }
        
        // Add examples if provided
        if (!ctx.examples.empty()) {
            std::string examples_section = buildExamplesSection(ctx.examples);
            template_text = replacePlaceholder(template_text, 
                                              "{examples}", 
                                              examples_section);
        }
        
        return template_text;
    }

private:
    std::map<std::string, std::string> templates_;
    
    std::string loadTemplate(const std::string& language);
    std::string replacePlaceholder(const std::string& text, 
                                   const std::string& placeholder, 
                                   const std::string& value);
    std::string buildExamplesSection(const std::vector<std::string>& examples);
};
```

### 3. LLM Client

**Zweck:** Kommunikation mit dem LLM-Service (vLLM, OpenAI, etc.)

**API-Abstraktion:**
```cpp
class LLMClient {
public:
    struct GenerationConfig {
        double temperature = 0.2;
        int max_tokens = 4096;
        double top_p = 0.95;
        double frequency_penalty = 0.0;
        double presence_penalty = 0.0;
    };

    struct GenerationResponse {
        std::string text;
        int total_tokens;
        int prompt_tokens;
        int completion_tokens;
        int64_t latency_ms;
    };

    GenerationResponse generate(
        const std::string& prompt,
        const GenerationConfig& config
    ) {
        // Build request
        nlohmann::json request = {
            {"model", model_},
            {"prompt", prompt},
            {"temperature", config.temperature},
            {"max_tokens", config.max_tokens},
            {"top_p", config.top_p}
        };
        
        auto start = std::chrono::steady_clock::now();
        
        // Call LLM API
        auto response = callAPI(endpoint_, request);
        
        auto end = std::chrono::steady_clock::now();
        auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(
            end - start
        ).count();
        
        // Parse response
        return parseResponse(response, latency);
    }

private:
    std::string endpoint_;
    std::string model_;
    std::string api_key_;
    
    nlohmann::json callAPI(const std::string& endpoint, 
                          const nlohmann::json& request);
    GenerationResponse parseResponse(const nlohmann::json& response, 
                                    int64_t latency);
};
```

### 4. Code Validator

**Zweck:** Validiert generierten Code auf Syntax, Sicherheit und Qualität

**Validierungs-Pipeline:**
```cpp
class CodeValidator {
public:
    struct ValidationResult {
        bool syntax_valid;
        bool security_approved;
        double quality_score;
        std::vector<std::string> syntax_errors;
        std::vector<std::string> security_issues;
        std::vector<std::string> quality_warnings;
    };

    ValidationResult validate(const std::string& code, 
                             const std::string& language) {
        ValidationResult result;
        
        // 1. Syntax validation
        result.syntax_valid = checkSyntax(code, language);
        if (!result.syntax_valid) {
            result.syntax_errors = getSyntaxErrors();
            return result;  // Early exit if syntax invalid
        }
        
        // 2. Security scanning
        result.security_issues = scanSecurityIssues(code);
        result.security_approved = result.security_issues.empty();
        
        // 3. Quality analysis
        result.quality_score = calculateQualityScore(code, language);
        result.quality_warnings = getQualityWarnings(code, language);
        
        return result;
    }

private:
    bool checkSyntax(const std::string& code, const std::string& language);
    std::vector<std::string> scanSecurityIssues(const std::string& code);
    double calculateQualityScore(const std::string& code, 
                                 const std::string& language);
    std::vector<std::string> getQualityWarnings(const std::string& code, 
                                                const std::string& language);
};
```

### 5. Sandbox Executor

**Zweck:** Sichere Ausführung von generiertem Code in isolierter Umgebung

**Sandbox-Implementierung:**
```cpp
class SandboxExecutor {
public:
    struct SandboxConfig {
        size_t memory_limit_mb;
        int64_t time_limit_ms;
        int cpu_limit_percent;
        bool allow_network;
        bool allow_file_writes;
        std::vector<std::string> allowed_syscalls;
    };

    struct ExecutionResult {
        bool success;
        std::string output;
        std::string error;
        int64_t duration_ms;
        size_t memory_used_kb;
        int exit_code;
    };

    ExecutionResult execute(const std::string& code,
                           const std::string& language,
                           const SandboxConfig& config) {
        ExecutionResult result;
        
        // 1. Create isolated environment
        auto sandbox = createSandbox(config);
        
        // 2. Compile/prepare code
        auto prepared_code = prepareCode(code, language);
        
        // 3. Set resource limits
        sandbox.setMemoryLimit(config.memory_limit_mb * 1024 * 1024);
        sandbox.setTimeLimit(config.time_limit_ms);
        sandbox.setCPULimit(config.cpu_limit_percent);
        
        // 4. Configure restrictions
        if (!config.allow_network) {
            sandbox.disableNetworkAccess();
        }
        if (!config.allow_file_writes) {
            sandbox.disableFileSystemWrites();
        }
        sandbox.whitelistSyscalls(config.allowed_syscalls);
        
        // 5. Execute
        try {
            auto start = std::chrono::steady_clock::now();
            
            result = sandbox.run(prepared_code, language);
            
            auto end = std::chrono::steady_clock::now();
            result.duration_ms = std::chrono::duration_cast<
                std::chrono::milliseconds>(end - start).count();
            
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

private:
    // Platform-specific sandbox implementations
    #ifdef __linux__
    // Use namespaces, cgroups, seccomp
    #elif _WIN32
    // Use job objects, app containers
    #endif
};
```

## Datenfluss

### Code-Generierung

```
1. User Input
   "Find all users active in last 7 days"
   │
   v
2. Input Validation
   - Check length: OK (35 chars)
   - Check patterns: OK (no dangerous patterns)
   - Encoding: OK (valid UTF-8)
   │
   v
3. Prompt Building
   Template: AQL_QUERY_GENERATION
   Context: {database_schema, aql_syntax_reference}
   Output: Full structured prompt
   │
   v
4. LLM Generation
   Endpoint: http://localhost:8000
   Model: CodeLlama-13B
   Temperature: 0.2
   Output: Generated AQL code
   │
   v
5. Response Parsing
   Extract code from LLM response
   Clean formatting
   Remove explanations
   │
   v
6. Code Validation
   - Syntax check: PASS (valid AQL)
   - Security scan: PASS (no SQL injection, no system calls)
   - Quality score: 0.87 (good)
   │
   v
7. Output
   GenerationResult{
     success: true,
     code: "FOR u IN users FILTER u.last_active > DATE_NOW() - 7*24*60*60*1000 RETURN u",
     security_approved: true,
     quality_score: 0.87
   }
```

### Code-Ausführung

```
1. Generated Code
   "FOR u IN users FILTER ..."
   │
   v
2. Pre-Execution Review
   - Syntax: ✓
   - Security: ✓
   - Manual approval: ✓
   │
   v
3. Sandbox Creation
   - Memory limit: 512 MB
   - Time limit: 30s
   - No network
   - No file writes
   │
   v
4. Execution
   - AQL query → ThemisDB
   - Monitor resources
   - Capture output
   │
   v
5. Post-Execution
   - Collect results
   - Measure duration: 23ms
   - Measure memory: 12 MB
   │
   v
6. Audit Logging
   Store in ThemisDB:
   - User request
   - Generated code
   - Execution result
   - Timestamp
   - User ID
```

## Sicherheitsmodell

### Defense-in-Depth Strategie

```
┌─────────────────────────────────────────────────────────┐
│ Layer 1: Input Validation                               │
│ - Length limits                                         │
│ - Pattern matching (SQL injection, command injection)   │
│ - Encoding validation                                   │
└───────────────────────┬─────────────────────────────────┘
                        │
                        v
┌─────────────────────────────────────────────────────────┐
│ Layer 2: Prompt Sanitization                           │
│ - Escape special characters                            │
│ - Remove/neutralize dangerous patterns                  │
│ - Context isolation                                     │
└───────────────────────┬─────────────────────────────────┘
                        │
                        v
┌─────────────────────────────────────────────────────────┐
│ Layer 3: Code Validation                               │
│ - Syntax checking                                       │
│ - AST analysis                                          │
│ - Static security analysis                              │
└───────────────────────┬─────────────────────────────────┘
                        │
                        v
┌─────────────────────────────────────────────────────────┐
│ Layer 4: Runtime Sandboxing                            │
│ - Resource limits (memory, CPU, time)                   │
│ - System call filtering (seccomp/AppContainer)          │
│ - Network isolation                                     │
│ - Filesystem restrictions                               │
└───────────────────────┬─────────────────────────────────┘
                        │
                        v
┌─────────────────────────────────────────────────────────┐
│ Layer 5: Audit Logging                                 │
│ - All inputs logged                                     │
│ - All generated code logged                             │
│ - All executions logged                                 │
│ - Compliance audit trail                                │
└─────────────────────────────────────────────────────────┘
```

## Performance-Optimierungen

### 1. Prompt Caching

```cpp
class PromptCache {
    std::string getCacheKey(const std::string& request, 
                           const std::string& language) {
        // Hash request + language + template_version
        return sha256(request + language + getTemplateVersion(language));
    }

    std::optional<std::string> get(const std::string& cache_key) {
        // Check cache
        if (cache_.contains(cache_key)) {
            // Update access time for LRU
            updateAccessTime(cache_key);
            return cache_[cache_key];
        }
        return std::nullopt;
    }
};
```

### 2. Batch Processing

```cpp
// Process multiple requests in single LLM call
std::vector<GenerationResult> generateBatch(
    const std::vector<std::string>& requests
) {
    // Combine into single prompt
    std::string batch_prompt = buildBatchPrompt(requests);
    
    // Single LLM call
    auto response = llm_client_->generate(batch_prompt, config);
    
    // Parse individual results
    return parseBatchResponse(response, requests.size());
}
```

### 3. Asynchrone Verarbeitung

```cpp
std::future<GenerationResult> generateAsync(
    const std::string& request,
    const std::string& language
) {
    return std::async(std::launch::async, [this, request, language]() {
        return this->generateCode(request, language);
    });
}
```

## Erweiterbarkeit

### Plugin-Architektur

```cpp
class CodeGeneratorPlugin {
public:
    virtual ~CodeGeneratorPlugin() = default;
    
    virtual std::string getName() const = 0;
    virtual std::string getLanguage() const = 0;
    virtual GenerationResult generate(const std::string& description) = 0;
    virtual bool validate(const std::string& code) = 0;
    virtual ExecutionResult execute(const std::string& code) = 0;
};

class PluginManager {
public:
    void registerPlugin(std::unique_ptr<CodeGeneratorPlugin> plugin) {
        plugins_[plugin->getLanguage()] = std::move(plugin);
    }
    
    CodeGeneratorPlugin* getPlugin(const std::string& language) {
        auto it = plugins_.find(language);
        return it != plugins_.end() ? it->second.get() : nullptr;
    }

private:
    std::map<std::string, std::unique_ptr<CodeGeneratorPlugin>> plugins_;
};
```

## Deployment-Szenarien

### 1. On-Premise mit vLLM

```yaml
services:
  themisdb:
    image: themisdb/themisdb:latest
    
  vllm:
    image: vllm/vllm-openai:latest
    command: |
      --model codellama/CodeLlama-13b-Instruct-hf
      --gpu-memory-utilization 0.9
      
  code-translator:
    build: .
    environment:
      LLM_ENDPOINT: http://vllm:8000
      THEMISDB_ENDPOINT: http://themisdb:8765
```

### 2. Cloud mit OpenAI API

```yaml
code_translator:
  llm:
    endpoint: "https://api.openai.com/v1"
    model: "gpt-4"
    api_key: "${OPENAI_API_KEY}"
```

### 3. Hybrid (lokale Validierung, Cloud LLM)

```yaml
code_translator:
  llm:
    endpoint: "https://api.openai.com/v1"
  security:
    enable_local_validation: true
    require_manual_approval: true
```

## Monitoring & Observability

### Metriken

```cpp
// Prometheus Metrics
counter("code_generations_total", {{"language", "aql"}});
counter("code_generation_errors", {{"error_type", "syntax"}});
histogram("code_generation_duration_ms", duration);
gauge("code_quality_score", quality_score);
gauge("security_approval_rate", approval_rate);
```

### Tracing

```cpp
// OpenTelemetry Tracing
auto span = tracer->StartSpan("generate_code");
span->SetAttribute("language", language);
span->SetAttribute("user_id", user_id);

// ... generation logic ...

span->SetAttribute("quality_score", result.quality_score);
span->SetAttribute("security_approved", result.security_approved);
span->End();
```

## Zusammenfassung

Die LLM Code Translator Architektur bietet:

✅ **Sicherheit** - Multi-Layer Defense-in-Depth  
✅ **Performance** - Caching, Batching, Async  
✅ **Erweiterbarkeit** - Plugin-System  
✅ **Observability** - Metrics, Logging, Tracing  
✅ **Flexibilität** - Multiple Deployment-Szenarien  

Nächste Schritte: Siehe [Implementation Guide](./IMPLEMENTATION.md)
