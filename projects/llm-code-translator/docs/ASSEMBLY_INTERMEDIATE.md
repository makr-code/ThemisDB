# Assembler als Zwischenschritt - Alternative JIT Strategie

## Überblick

Ja, ein Zwischenschritt über **Assembler** ist nicht nur denkbar, sondern in vielen Fällen sogar **vorteilhaft**! Dies bietet mehr Kontrolle und Transparenz als direkter Maschinencode, ist aber trotzdem sehr nah an der Hardware.

## Konzept: Execution Plan → Assembler → Machine Code

```
┌─────────────────────────────────────────────────────────────────────┐
│           Assembler als Zwischenschritt                              │
├─────────────────────────────────────────────────────────────────────┤
│                                                                      │
│  User Prompt                                                         │
│       ↓                                                              │
│  LLM → Execution Plan (JSON)                                         │
│       ↓                                                              │
│  ┌──────────────────────────────────────────────────────┐          │
│  │  Plan → Assembler Code Generator (NEU!)              │          │
│  │                                                       │          │
│  │  Erzeugt lesbaren Assembler-Code:                   │          │
│  │  - x86_64 Assembly (NASM/GAS Syntax)                │          │
│  │  - ARM64 Assembly                                    │          │
│  │  - RISC-V Assembly                                   │          │
│  └──────────────────┬───────────────────────────────────┘          │
│                     ↓                                                │
│  Human-Readable Assembler Code (.asm/.s)                            │
│       ↓                                                              │
│  ┌──────────────────────────────────────────────────────┐          │
│  │  Assembler (NASM/GAS/LLVM-AS)                        │          │
│  └──────────────────┬───────────────────────────────────┘          │
│                     ↓                                                │
│  Object Code (.o)                                                    │
│       ↓                                                              │
│  ┌──────────────────────────────────────────────────────┐          │
│  │  Linker (ld/lld)                                      │          │
│  └──────────────────┬───────────────────────────────────┘          │
│                     ↓                                                │
│  Native Executable (.exe/.so)                                        │
│       ↓                                                              │
│  Direct Execution                                                    │
│                                                                      │
└─────────────────────────────────────────────────────────────────────┘
```

## Vorteile des Assembler-Zwischenschritts

### 1. **Transparenz & Debugging**
```asm
; Assembler-Code ist lesbar und debugbar!
; Statt binärer Maschinencode können Entwickler sehen was passiert

.section .text
.global executeQuery

executeQuery:
    ; Function prologue
    push    rbp
    mov     rbp, rsp
    sub     rsp, 64
    
    ; Load database pointer (first parameter)
    mov     r12, rdi
    
    ; Load parameters pointer (second parameter)
    mov     r13, rsi
    
    ; Call database scan function
    mov     rdi, r12              ; db pointer
    lea     rsi, [rel table_name] ; "sensor_readings"
    call    db_scan_table
    
    ; Store result
    mov     [rbp-8], rax
    
    ; Function epilogue
    mov     rsp, rbp
    pop     rbp
    ret

table_name:
    .asciz "sensor_readings"
```

### 2. **Optimierung & Inspektion**
- Assembler-Code kann von Menschen inspiziert werden
- Performance-Optimierungen können manuell vorgenommen werden
- Register-Allokation ist sichtbar
- Kompilierungs-Probleme sind leichter zu debuggen

### 3. **Portabilität zwischen Architekturen**
```asm
; x86_64 Assembly (Intel Syntax)
mov rax, [rbx + 8]
add rax, rcx

; ARM64 Assembly
ldr x0, [x1, #8]
add x0, x0, x2

; RISC-V Assembly
ld a0, 8(a1)
add a0, a0, a2
```

### 4. **Integration mit bestehenden Tools**
- Standard-Assembler (NASM, GAS, MASM)
- Standard-Debugger (GDB, LLDB)
- Profiling-Tools (perf, VTune)
- Disassembler (objdump, IDA)

## Implementierung

### Plan-zu-Assembler Generator

```cpp
#ifndef THEMIS_ASSEMBLY_GENERATOR_H
#define THEMIS_ASSEMBLY_GENERATOR_H

#include "direct_execution_engine.h"
#include <string>
#include <sstream>

namespace themis::assembly {

/**
 * @brief Assembly-Syntaxe
 */
enum class AssemblySyntax {
    NASM,       // NASM Syntax (Intel-Style)
    GAS_INTEL,  // GNU Assembler (Intel Syntax)
    GAS_ATT,    // GNU Assembler (AT&T Syntax)
    MASM        // Microsoft Assembler
};

/**
 * @brief Ziel-Architektur
 */
enum class TargetArch {
    X86_64,
    ARM64,
    RISCV64
};

/**
 * @brief Assembly Code Generator
 * 
 * Übersetzt Execution Plans in lesbaren Assembler-Code
 */
class AssemblyGenerator {
public:
    struct Config {
        AssemblySyntax syntax = AssemblySyntax::NASM;
        TargetArch target = TargetArch::X86_64;
        bool optimize = true;
        bool add_comments = true;  // Hilfreiche Kommentare
        bool add_debug_info = false;
    };
    
    explicit AssemblyGenerator(const Config& config = Config{});
    
    /**
     * @brief Generiere Assembler-Code aus Execution Plan
     * @param plan Execution plan
     * @return Assembler-Code als String
     */
    std::string generate(const direct_execution::ExecutionPlan& plan);
    
    /**
     * @brief Schreibe Assembler-Code in Datei
     * @param plan Execution plan
     * @param output_path Pfad zur .asm/.s Datei
     * @return Pfad zur generierten Datei
     */
    std::string generateToFile(
        const direct_execution::ExecutionPlan& plan,
        const std::string& output_path
    );

private:
    Config config_;
    std::stringstream asm_output_;
    int label_counter_ = 0;
    
    // Code-Generierung für verschiedene Operationen
    void generateQuery(const nlohmann::json& params);
    void generateAggregate(const nlohmann::json& params);
    void generateFilter(const nlohmann::json& filter);
    
    // Architektur-spezifische Code-Generierung
    void emitFunctionPrologue();
    void emitFunctionEpilogue();
    void emitLoadRegister(const std::string& reg, const std::string& source);
    void emitStoreRegister(const std::string& reg, const std::string& dest);
    void emitCall(const std::string& function);
    
    std::string getNewLabel(const std::string& prefix);
};

/**
 * @brief Assembler & Linker
 * 
 * Kompiliert generierten Assembler-Code zu ausführbarem Code
 */
class AssemblerLinker {
public:
    struct Config {
        std::string assembler = "nasm";  // oder "as", "clang"
        std::string linker = "ld";       // oder "lld", "link"
        bool keep_intermediate = false;  // .o Dateien behalten?
        std::string temp_dir = "/tmp";
    };
    
    explicit AssemblerLinker(const Config& config = Config{});
    
    /**
     * @brief Assembliere und linke zu ausführbarer Datei
     * @param asm_file Pfad zur .asm/.s Datei
     * @param output_exe Pfad zur ausführbaren Datei
     * @return Pfad zur generierten Datei
     */
    std::string assembleAndLink(
        const std::string& asm_file,
        const std::string& output_exe
    );
    
    /**
     * @brief Nur assemblieren (zu .o Object File)
     * @param asm_file Pfad zur .asm Datei
     * @param output_obj Pfad zur .o Datei
     * @return Pfad zur generierten Object-Datei
     */
    std::string assemble(
        const std::string& asm_file,
        const std::string& output_obj
    );

private:
    Config config_;
    
    std::string runAssembler(const std::string& input, const std::string& output);
    std::string runLinker(const std::string& input, const std::string& output);
};

/**
 * @brief Assembly-Enhanced Execution Engine
 * 
 * Erweitert DirectExecutionEngine um Assembler-basierten Workflow
 */
class AssemblyExecutionEngine : public direct_execution::DirectExecutionEngine {
public:
    struct Config : direct_execution::DirectExecutionEngine::Config {
        AssemblyGenerator::Config asm_config;
        AssemblerLinker::Config linker_config;
        bool save_assembly = true;  // .asm Dateien speichern?
        std::string asm_output_dir = "./generated_asm";
    };
    
    explicit AssemblyExecutionEngine(
        rocksdb::TransactionDB* db,
        const Config& config = Config{}
    );
    
    /**
     * @brief Kompiliere Prompt zu Executable via Assembler
     * 
     * Flow: Prompt → Plan → Assembly → Object → Executable
     */
    std::string compileViaAssembly(
        const std::string& user_prompt,
        const std::string& output_exe
    );
    
    /**
     * @brief Zeige generierten Assembler-Code an
     */
    std::string showAssembly(const std::string& user_prompt);
    
    /**
     * @brief Optimiere Assembler-Code
     */
    std::string optimizeAssembly(const std::string& asm_code);

private:
    Config config_;
    std::unique_ptr<AssemblyGenerator> asm_generator_;
    std::unique_ptr<AssemblerLinker> assembler_;
};

} // namespace themis::assembly

#endif // THEMIS_ASSEMBLY_GENERATOR_H
```

### Beispiel: Query zu x86_64 Assembly

```cpp
std::string AssemblyGenerator::generate(const ExecutionPlan& plan) {
    asm_output_.str("");  // Clear
    
    if (config_.syntax == AssemblySyntax::NASM) {
        // NASM Header
        asm_output_ << "section .text\n";
        asm_output_ << "global executeQuery\n\n";
        
        if (config_.add_comments) {
            asm_output_ << "; Generated from Execution Plan\n";
            asm_output_ << "; Operation: " << operationToString(plan.operation) << "\n";
            asm_output_ << "\n";
        }
        
        asm_output_ << "executeQuery:\n";
        
        emitFunctionPrologue();
        
        switch (plan.operation) {
            case ExecutionPlan::OperationType::QUERY:
                generateQuery(plan.parameters);
                break;
            case ExecutionPlan::OperationType::AGGREGATE:
                generateAggregate(plan.parameters);
                break;
            // ...
        }
        
        emitFunctionEpilogue();
        
        // Data section
        asm_output_ << "\nsection .data\n";
        asm_output_ << "table_name: db \"" 
                    << plan.parameters["datasource"].get<std::string>() 
                    << "\", 0\n";
    }
    
    return asm_output_.str();
}

void AssemblyGenerator::generateQuery(const nlohmann::json& params) {
    if (config_.add_comments) {
        asm_output_ << "    ; Query operation\n";
        asm_output_ << "    ; Table: " << params["datasource"] << "\n";
    }
    
    // Load database pointer (rdi = first parameter)
    asm_output_ << "    mov r12, rdi          ; Save db pointer\n";
    asm_output_ << "    mov r13, rsi          ; Save params pointer\n";
    asm_output_ << "\n";
    
    // Call db_scan_table
    asm_output_ << "    mov rdi, r12          ; db pointer\n";
    asm_output_ << "    lea rsi, [rel table_name] ; table name\n";
    asm_output_ << "    call db_scan_table    ; Scan table\n";
    asm_output_ << "    mov r14, rax          ; Save result set\n";
    asm_output_ << "\n";
    
    // Apply filters
    if (params.contains("filters") && !params["filters"].empty()) {
        if (config_.add_comments) {
            asm_output_ << "    ; Apply filters\n";
        }
        
        for (const auto& filter : params["filters"]) {
            generateFilter(filter);
        }
    }
    
    // Return results
    asm_output_ << "    mov rax, r14          ; Return result set\n";
}

void AssemblyGenerator::generateFilter(const nlohmann::json& filter) {
    std::string field = filter["field"];
    std::string op = filter["op"];
    
    if (config_.add_comments) {
        asm_output_ << "    ; Filter: " << field << " " << op << " ...\n";
    }
    
    std::string loop_label = getNewLabel("filter_loop");
    std::string end_label = getNewLabel("filter_end");
    
    asm_output_ << loop_label << ":\n";
    asm_output_ << "    ; Get next row from result set\n";
    asm_output_ << "    mov rdi, r14\n";
    asm_output_ << "    call result_next_row\n";
    asm_output_ << "    test rax, rax\n";
    asm_output_ << "    jz " << end_label << "     ; No more rows\n";
    asm_output_ << "\n";
    asm_output_ << "    ; Extract field value\n";
    asm_output_ << "    mov rdi, rax          ; Row pointer\n";
    asm_output_ << "    lea rsi, [rel field_name_" << label_counter_ << "]\n";
    asm_output_ << "    call row_get_field\n";
    asm_output_ << "\n";
    asm_output_ << "    ; Apply comparison\n";
    
    if (op == ">") {
        asm_output_ << "    cmp rax, " << filter["value"] << "\n";
        asm_output_ << "    jle " << loop_label << "  ; Skip if not greater\n";
    } else if (op == "==") {
        asm_output_ << "    cmp rax, " << filter["value"] << "\n";
        asm_output_ << "    jne " << loop_label << "  ; Skip if not equal\n";
    }
    // ... weitere Operatoren
    
    asm_output_ << "    jmp " << loop_label << "\n";
    asm_output_ << end_label << ":\n";
}
```

### Generierter Assembler-Code Beispiel

**Execution Plan:**
```json
{
  "operation": "QUERY",
  "datasource": "sensor_readings",
  "filters": [
    {"field": "temperature", "op": ">", "value": 50}
  ],
  "return": "entities"
}
```

**Generierter x86_64 Assembly (NASM Syntax):**
```asm
section .text
global executeQuery

; Generated from Execution Plan
; Operation: QUERY
; Table: sensor_readings

executeQuery:
    ; Function prologue
    push    rbp
    mov     rbp, rsp
    sub     rsp, 64                ; Allocate stack space
    
    ; Save parameters
    mov     r12, rdi               ; Save db pointer
    mov     r13, rsi               ; Save params pointer
    
    ; Query operation
    ; Table: sensor_readings
    mov     rdi, r12               ; db pointer
    lea     rsi, [rel table_name]  ; table name
    call    db_scan_table          ; Scan table
    mov     r14, rax               ; Save result set
    
    ; Apply filters
    ; Filter: temperature > 50
filter_loop_1:
    ; Get next row from result set
    mov     rdi, r14
    call    result_next_row
    test    rax, rax
    jz      filter_end_1           ; No more rows
    
    ; Extract field value
    mov     rdi, rax               ; Row pointer
    lea     rsi, [rel field_temperature]
    call    row_get_field
    
    ; Apply comparison: temperature > 50
    cmp     rax, 50
    jle     filter_loop_1          ; Skip if not greater
    
    ; Row matches filter, add to results
    mov     rdi, r14
    mov     rsi, rax
    call    result_add_row
    
    jmp     filter_loop_1
    
filter_end_1:
    ; Return results
    mov     rax, r14               ; Return result set
    
    ; Function epilogue
    mov     rsp, rbp
    pop     rbp
    ret

section .data
    table_name: db "sensor_readings", 0
    field_temperature: db "temperature", 0
```

## Workflow-Beispiele

### 1. Inspektion des generierten Codes

```cpp
AssemblyExecutionEngine engine(db);

std::string prompt = "Find sensors with temperature > 50°C";

// Zeige generierten Assembly-Code
std::string asm_code = engine.showAssembly(prompt);

std::cout << "Generated Assembly Code:\n";
std::cout << asm_code << std::endl;

// Entwickler kann Code inspizieren und ggf. manuell optimieren
```

### 2. Debugging mit GDB

```bash
# Kompiliere mit Debug-Symbolen
$ themis-compiler --to-asm --debug query.json -o query.asm

# Assembliere mit Debug-Info
$ nasm -f elf64 -g -F dwarf query.asm -o query.o

# Linke
$ ld query.o -o query

# Debugge mit GDB
$ gdb query
(gdb) break executeQuery
(gdb) run
(gdb) disassemble
(gdb) info registers
(gdb) step
```

### 3. Performance-Analyse

```bash
# Assembliere
$ nasm -f elf64 query.asm -o query.o

# Linke
$ ld query.o -o query

# Disassemble für Inspektion
$ objdump -d query

# Performance-Analyse
$ perf record ./query
$ perf report

# Instruction-Level Profiling
$ perf annotate executeQuery
```

### 4. Manuelle Optimierung

```cpp
// 1. Generiere Assembler
auto asm_code = generator.generate(plan);

// 2. Schreibe in Datei
generator.generateToFile(plan, "query_unoptimized.asm");

// 3. Entwickler optimiert manuell (z.B. Loop Unrolling)
// 4. Assembliere optimierte Version
assembler.assembleAndLink("query_optimized.asm", "query_optimized");
```

## Vergleich: Direkt vs. Assembler vs. High-Level

```
┌──────────────────────┬─────────────┬──────────────┬────────────┬─────────────┐
│ Ansatz               │ Transparenz │ Optimierung  │ Debug      │ Performance │
├──────────────────────┼─────────────┼──────────────┼────────────┼─────────────┤
│ Direct Machine Code  │ ⭐          │ ⭐⭐⭐       │ ⭐         │ ⭐⭐⭐⭐⭐  │
│ Via Assembler        │ ⭐⭐⭐⭐    │ ⭐⭐⭐⭐     │ ⭐⭐⭐⭐   │ ⭐⭐⭐⭐    │
│ Via LLVM IR          │ ⭐⭐⭐      │ ⭐⭐⭐⭐⭐   │ ⭐⭐⭐     │ ⭐⭐⭐⭐⭐  │
│ Interpretation       │ ⭐⭐⭐⭐⭐  │ ⭐           │ ⭐⭐⭐⭐⭐  │ ⭐⭐        │
└──────────────────────┴─────────────┴──────────────┴────────────┴─────────────┘
```

## Integration in das Projekt

```cpp
namespace themis::direct_execution {

class DirectExecutionEngine {
public:
    struct Config {
        // Existing fields...
        
        // Assembly compilation options
        bool enable_assembly_output = false;
        AssemblyGenerator::Config asm_config;
        std::string asm_output_dir = "./generated_asm";
    };
    
    /**
     * @brief Kompiliere via Assembler
     */
    std::string compileViaAssembly(
        const std::string& user_prompt,
        const std::string& output_path
    ) {
        // 1. Translate to execution plan
        auto plan = translator_->translate(user_prompt);
        
        // 2. Generate assembly code
        auto asm_code = asm_generator_->generate(plan);
        
        // 3. Save assembly file (optional)
        if (config_.enable_assembly_output) {
            std::string asm_file = config_.asm_output_dir + "/query.asm";
            asm_generator_->generateToFile(plan, asm_file);
        }
        
        // 4. Assemble and link
        std::string temp_asm = "/tmp/query.asm";
        writeToFile(temp_asm, asm_code);
        
        return assembler_->assembleAndLink(temp_asm, output_path);
    }
};

} // namespace themis::direct_execution
```

## Zusammenfassung

### Ja, Assembler als Zwischenschritt ist **sehr sinnvoll**!

**Vorteile:**
- ✅ **Lesbar** - Menschen können Code verstehen
- ✅ **Debugbar** - Standard-Tools (GDB, LLDB) funktionieren
- ✅ **Inspizierbar** - Performance-Analyse möglich
- ✅ **Portabel** - Verschiedene Architekturen (x86, ARM, RISC-V)
- ✅ **Optimierbar** - Manuelle Optimierungen möglich
- ✅ **Standard-Tools** - NASM, GAS, Disassembler, etc.

**Workflow:**
```
Prompt → Plan → Assembly → Object → Executable
```

**Best of both worlds:**
- Schneller als Interpretation
- Transparenter als direkter Maschinencode
- Optimierbar wie traditioneller Compiler
- Aber: Kein Quellcode-Generierung nötig!
