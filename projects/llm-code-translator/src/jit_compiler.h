/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            jit_compiler.h                                     ║
  Version:         0.0.43                                             ║
  Last Modified:   2026-04-15 04:15:23                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     473                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#ifndef THEMIS_JIT_COMPILER_H
#define THEMIS_JIT_COMPILER_H

#include "direct_execution_engine.h"
#include <string>
#include <memory>
#include <functional>
#include <map>

namespace themis::jit {

/**
 * @brief JIT Compiler - Kompiliert Execution Plans zu nativem Maschinencode
 * 
 * Dieser Compiler übersetzt Execution Plans zur Laufzeit in nativen Maschinencode,
 * ähnlich wie ein traditioneller Compiler, aber ohne Zwischenschritt über Quellcode.
 * 
 * Flow: Execution Plan → IR → Optimization → Native Machine Code
 * 
 * Unterstützte Backends:
 * - LLVM (empfohlen für Production)
 * - LibJIT (leichtgewichtig)
 * - Custom (direkter x86_64/ARM Code-Generator)
 */

/**
 * @brief Compiled function signature
 * 
 * Alle kompilierten Funktionen haben diese Signatur:
 * void* executeQuery(void* db, void* params)
 */
using CompiledFunction = void* (*)(void* db, void* params);

/**
 * @brief JIT Backend Types
 */
enum class JITBackend {
    LLVM,       // LLVM JIT Compiler (beste Optimierung)
    LibJIT,     // LibJIT (schnellere Compilation)
    Custom,     // Custom x86_64/ARM Generator
    Auto        // Automatische Auswahl
};

/**
 * @brief Compilation Mode
 */
enum class CompilationMode {
    INTERPRET,      // Keine JIT, direkte Interpretation (Default)
    JIT_MEMORY,     // JIT zu Memory, einmalige Ausführung
    JIT_CACHE,      // JIT zu Memory, cachen für Wiederverwendung
    JIT_FILE        // JIT zu .exe/.so Datei
};

/**
 * @brief Optimization Level
 */
enum class OptimizationLevel {
    O0,  // Keine Optimierung (schnellste Compilation)
    O1,  // Basis-Optimierung
    O2,  // Standard-Optimierung (empfohlen)
    O3   // Aggressive Optimierung (langsamere Compilation)
};

/**
 * @brief Compiled code metadata
 */
struct CompiledCode {
    CompiledFunction function = nullptr;
    size_t code_size_bytes = 0;
    std::string target_arch;      // z.B. "x86_64", "arm64"
    OptimizationLevel opt_level;
    int64_t compilation_time_ms = 0;
    bool is_cached = false;
};

/**
 * @brief JIT Compiler Configuration
 */
struct JITConfig {
    JITBackend backend = JITBackend::LLVM;
    OptimizationLevel opt_level = OptimizationLevel::O2;
    
    // Memory limits
    size_t max_code_cache_size_mb = 256;
    size_t max_executable_size_mb = 64;
    
    // Adaptive JIT
    bool enable_adaptive_jit = true;
    int adaptive_threshold = 10;  // Nach wie vielen Ausführungen JIT?
    
    // Tiered compilation
    bool enable_tiered_compilation = true;
    OptimizationLevel tier1_opt = OptimizationLevel::O0;
    OptimizationLevel tier2_opt = OptimizationLevel::O2;
    
    // Debug
    bool generate_debug_symbols = false;
    bool dump_ir = false;          // Dump intermediate representation
    bool dump_asm = false;         // Dump assembly code
    std::string dump_dir = "./jit_dumps";
};

/**
 * @brief Abstract JIT Compiler Interface
 */
class IJITCompiler {
public:
    virtual ~IJITCompiler() = default;
    
    /**
     * @brief Compile execution plan to native code
     * @param plan Execution plan to compile
     * @return Compiled native function
     */
    virtual CompiledCode compile(
        const direct_execution::ExecutionPlan& plan
    ) = 0;
    
    /**
     * @brief Compile to executable file
     * @param plan Execution plan
     * @param output_path Path to output executable
     * @return Path to generated executable
     */
    virtual std::string compileToFile(
        const direct_execution::ExecutionPlan& plan,
        const std::string& output_path
    ) = 0;
    
    /**
     * @brief Compile to shared library
     * @param plan Execution plan
     * @param output_path Path to output library (.so/.dll)
     * @return Path to generated library
     */
    virtual std::string compileToSharedLibrary(
        const direct_execution::ExecutionPlan& plan,
        const std::string& output_path
    ) = 0;
    
    /**
     * @brief Get compiler backend name
     */
    virtual std::string getBackendName() const = 0;
};

/**
 * @brief LLVM-based JIT Compiler
 * 
 * Verwendet LLVM für JIT Compilation mit exzellenten Optimierungen.
 * Empfohlen für Production-Einsatz.
 */
class LLVMJITCompiler : public IJITCompiler {
public:
    explicit LLVMJITCompiler(const JITConfig& config = JITConfig{});
    ~LLVMJITCompiler() override;
    
    CompiledCode compile(
        const direct_execution::ExecutionPlan& plan
    ) override;
    
    std::string compileToFile(
        const direct_execution::ExecutionPlan& plan,
        const std::string& output_path
    ) override;
    
    std::string compileToSharedLibrary(
        const direct_execution::ExecutionPlan& plan,
        const std::string& output_path
    ) override;
    
    std::string getBackendName() const override { return "LLVM"; }

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief LibJIT-based Compiler (lightweight alternative)
 */
class LibJITCompiler : public IJITCompiler {
public:
    explicit LibJITCompiler(const JITConfig& config = JITConfig{});
    ~LibJITCompiler() override;
    
    CompiledCode compile(
        const direct_execution::ExecutionPlan& plan
    ) override;
    
    std::string compileToFile(
        const direct_execution::ExecutionPlan& plan,
        const std::string& output_path
    ) override;
    
    std::string compileToSharedLibrary(
        const direct_execution::ExecutionPlan& plan,
        const std::string& output_path
    ) override;
    
    std::string getBackendName() const override { return "LibJIT"; }

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief Custom x86_64/ARM Code Generator
 * 
 * Direkter Maschinencode-Generator ohne externe Dependencies.
 * Für maximale Kontrolle und minimale Binär-Größe.
 */
class CustomJITCompiler : public IJITCompiler {
public:
    explicit CustomJITCompiler(const JITConfig& config = JITConfig{});
    ~CustomJITCompiler() override;
    
    CompiledCode compile(
        const direct_execution::ExecutionPlan& plan
    ) override;
    
    std::string compileToFile(
        const direct_execution::ExecutionPlan& plan,
        const std::string& output_path
    ) override;
    
    std::string compileToSharedLibrary(
        const direct_execution::ExecutionPlan& plan,
        const std::string& output_path
    ) override;
    
    std::string getBackendName() const override { return "Custom"; }

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief JIT Code Cache
 * 
 * Cached kompilierte Funktionen für Wiederverwendung.
 */
class JITCache {
public:
    struct CacheConfig {
        size_t max_size_mb = 256;
        int64_t ttl_seconds = -1;  // -1 = never expire
    };
    
    explicit JITCache(const CacheConfig& config = CacheConfig{});
    
    /**
     * @brief Get compiled code from cache
     * @param plan_hash Hash of execution plan
     * @return Compiled code if found
     */
    std::optional<CompiledCode> get(const std::string& plan_hash);
    
    /**
     * @brief Put compiled code in cache
     * @param plan_hash Hash of execution plan
     * @param code Compiled code
     */
    void put(const std::string& plan_hash, const CompiledCode& code);
    
    /**
     * @brief Clear cache
     */
    void clear();
    
    /**
     * @brief Get cache statistics
     */
    struct Stats {
        size_t entries = 0;
        size_t total_size_bytes = 0;
        size_t hits = 0;
        size_t misses = 0;
        double hit_rate = 0.0;
    };
    Stats getStats() const;

private:
    CacheConfig config_;
    std::map<std::string, CompiledCode> cache_;
    Stats stats_;
};

/**
 * @brief Adaptive JIT Engine
 * 
 * Entscheidet automatisch, wann JIT-Compilation sinnvoll ist.
 * "Hot paths" werden kompiliert, "cold paths" interpretiert.
 */
class AdaptiveJITEngine {
public:
    struct Config {
        int hot_threshold = 10;      // Nach wie vielen Ausführungen JIT?
        bool enable_tiered = true;   // Tiered compilation?
        JITConfig jit_config;
    };
    
    explicit AdaptiveJITEngine(
        rocksdb::TransactionDB* db,
        const Config& config
    );
    
    /**
     * @brief Execute execution plan
     * 
     * Automatische Entscheidung:
     * - Erste Ausführungen: Interpretation
     * - Nach Threshold: JIT Compilation
     * - Wiederholte Ausführungen: Native Code
     */
    direct_execution::ExecutionResult execute(
        const direct_execution::ExecutionPlan& plan
    );
    
    /**
     * @brief Get engine statistics
     */
    struct Stats {
        size_t total_executions = 0;
        size_t interpreted_executions = 0;
        size_t jit_executions = 0;
        size_t compilation_count = 0;
        double avg_interpretation_time_ms = 0.0;
        double avg_jit_execution_time_ms = 0.0;
        double avg_compilation_time_ms = 0.0;
    };
    Stats getStats() const;

private:
    rocksdb::TransactionDB* db_;
    Config config_;
    
    std::unique_ptr<IJITCompiler> jit_compiler_;
    std::unique_ptr<JITCache> cache_;
    std::unique_ptr<direct_execution::DirectExecutor> interpreter_;
    
    // Execution frequency tracking
    std::map<std::string, int> execution_counts_;
    
    Stats stats_;
    
    bool shouldCompile(const std::string& plan_hash);
    std::string hashPlan(const direct_execution::ExecutionPlan& plan);
};

/**
 * @brief JIT-Enhanced Direct Execution Engine
 * 
 * Erweitert DirectExecutionEngine um JIT Compilation Fähigkeiten.
 */
class JITDirectExecutionEngine : public direct_execution::DirectExecutionEngine {
public:
    struct Config : direct_execution::DirectExecutionEngine::Config {
        JITConfig jit_config;
        CompilationMode compilation_mode = CompilationMode::JIT_CACHE;
        std::string jit_cache_dir = "./jit_cache";
    };
    
    explicit JITDirectExecutionEngine(
        rocksdb::TransactionDB* db,
        const Config& config
    );
    
    /**
     * @brief Execute prompt with JIT compilation
     */
    direct_execution::ExecutionResult executePromptJIT(
        const std::string& user_prompt
    );
    
    /**
     * @brief Compile prompt to executable file
     * @param user_prompt Natural language prompt
     * @param output_path Path to output executable
     * @return Path to generated executable
     */
    std::string compileToExecutable(
        const std::string& user_prompt,
        const std::string& output_path
    );
    
    /**
     * @brief Compile prompt to shared library
     * @param user_prompt Natural language prompt
     * @param output_path Path to output library
     * @return Path to generated library
     */
    std::string compileToSharedLibrary(
        const std::string& user_prompt,
        const std::string& output_path
    );
    
    /**
     * @brief Get JIT statistics
     */
    struct JITStats {
        size_t total_compilations = 0;
        size_t cache_hits = 0;
        size_t cache_misses = 0;
        double avg_compilation_time_ms = 0.0;
        double avg_execution_time_jit_ms = 0.0;
        double avg_execution_time_interpreted_ms = 0.0;
        size_t total_code_size_bytes = 0;
    };
    JITStats getJITStats() const;

private:
    Config config_;
    std::unique_ptr<IJITCompiler> jit_compiler_;
    std::unique_ptr<JITCache> jit_cache_;
    
    JITStats jit_stats_;
    
    std::string hashPrompt(const std::string& prompt);
    CompiledCode getOrCompile(const direct_execution::ExecutionPlan& plan);
};

/**
 * @brief Factory for creating JIT compilers
 */
class JITCompilerFactory {
public:
    /**
     * @brief Create JIT compiler based on backend type
     */
    static std::unique_ptr<IJITCompiler> create(
        JITBackend backend,
        const JITConfig& config = JITConfig{}
    );
    
    /**
     * @brief Auto-detect best available backend
     */
    static JITBackend detectBestBackend();
    
    /**
     * @brief Check if backend is available
     */
    static bool isBackendAvailable(JITBackend backend);
};

} // namespace themis::jit

#endif // THEMIS_JIT_COMPILER_H
