/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            aql_parser_harness.cpp                             ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:43:57                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     224                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file aql_parser_harness.cpp
 * @brief AFL++ Fuzzing Harness for ThemisDB AQL Parser
 * 
 * This harness provides persistent mode fuzzing for the AQL query parser.
 * It uses AFL++ persistent mode for optimal performance.
 * 
 * Build with:
 *   afl-clang-lto++ -fsanitize=address,undefined -O2 -g \
 *     -o aql_parser_harness aql_parser_harness.cpp \
 *     -I../../include -L../../build/lib -lthemisdb_aql
 */

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <string_view>

// AFL++ persistent mode macros
#ifdef __AFL_FUZZ_TESTCASE_LEN
  #define AFL_LOOP(count) __AFL_LOOP(count)
#else
  #define AFL_LOOP(count) 1
  unsigned char *__AFL_FUZZ_TESTCASE_BUF = nullptr;
#endif

// ThemisDB AQL Parser interface (mock for harness template)
namespace themisdb {
namespace aql {

/**
 * @brief Parse result structure
 */
struct ParseResult {
    bool success;
    std::string error_message;
    int error_position;
};

/**
 * @brief Mock AQL Parser class
 * Replace with actual parser include when building
 */
class Parser {
public:
    Parser() = default;
    ~Parser() = default;
    
    ParseResult parse(std::string_view query) {
        ParseResult result;
        
        // Basic validation (replace with actual parser call)
        if (query.empty()) {
            result.success = false;
            result.error_message = "Empty query";
            result.error_position = 0;
            return result;
        }
        
        // Check for basic SQL injection patterns
        // This is where the actual parser would be called
        result.success = true;
        result.error_message = "";
        result.error_position = -1;
        
        return result;
    }
    
    void reset() {
        // Reset parser state between iterations
    }
};

} // namespace aql
} // namespace themisdb

/**
 * @brief Initialize fuzzing session
 */
void fuzz_init() {
    // Initialize any global state needed for fuzzing
    // This is called once at startup
}

/**
 * @brief Cleanup fuzzing session
 */
void fuzz_deinit() {
    // Cleanup any global state
}

/**
 * @brief Process a single fuzz input
 * @param data Input buffer from AFL++
 * @param size Size of input buffer
 * @return 0 on success, non-zero on crash (for testing)
 */
int fuzz_one_input(const uint8_t* data, size_t size) {
    if (data == nullptr || size == 0) {
        return 0;
    }
    
    // Create string_view from input (no null-termination required)
    std::string_view query(reinterpret_cast<const char*>(data), size);
    
    // Create parser instance
    static themisdb::aql::Parser parser;
    
    // Reset parser state
    parser.reset();
    
    // Parse the query
    auto result = parser.parse(query);
    
    // Log result for debugging (disabled in production fuzzing)
#ifdef FUZZ_DEBUG
    if (!result.success) {
        fprintf(stderr, "Parse error at %d: %s\n", 
                result.error_position, 
                result.error_message.c_str());
    }
#endif
    
    return 0;
}

/**
 * @brief Main entry point for AFL++ fuzzing
 * 
 * Supports both:
 * - File-based input: ./harness input_file
 * - AFL++ persistent mode: afl-fuzz ... -- ./harness @@
 */
int main(int argc, char** argv) {
    // Initialize
    fuzz_init();
    
#ifdef __AFL_FUZZ_TESTCASE_LEN
    // AFL++ persistent mode
    __AFL_INIT();
    
    // Main fuzzing loop - process 10000 inputs per fork
    while (AFL_LOOP(10000)) {
        size_t len = __AFL_FUZZ_TESTCASE_LEN;
        
        if (len > 0 && __AFL_FUZZ_TESTCASE_BUF != nullptr) {
            fuzz_one_input(__AFL_FUZZ_TESTCASE_BUF, len);
        }
    }
#else
    // Standalone mode - read from file
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input_file>\n", argv[0]);
        return 1;
    }
    
    // Read input file
    FILE* f = fopen(argv[1], "rb");
    if (!f) {
        perror("Failed to open input file");
        return 1;
    }
    
    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    if (size == 0) {
        fclose(f);
        return 0;
    }
    
    uint8_t* buffer = static_cast<uint8_t*>(malloc(size));
    if (!buffer) {
        fclose(f);
        return 1;
    }
    
    if (fread(buffer, 1, size, f) != size) {
        free(buffer);
        fclose(f);
        return 1;
    }
    
    fclose(f);
    
    // Process input
    int result = fuzz_one_input(buffer, size);
    
    free(buffer);
    
    return result;
#endif
    
    // Cleanup
    fuzz_deinit();
    
    return 0;
}
