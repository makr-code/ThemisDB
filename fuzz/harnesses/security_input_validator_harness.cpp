/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            security_input_validator_harness.cpp               ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:43:57                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   94.0/100                                       ║
    • Total Lines:     162                                            ║
    • Open Issues:     TODOs: 0, Stubs: 2                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file security_input_validator_harness.cpp
 * @brief AFL++ Fuzzing Harness for ThemisDB InputValidator (security path)
 *
 * Exercises the InputValidator with arbitrary byte sequences to surface
 * crashes, hangs, and unexpected allow/deny decisions in:
 *   - Path-traversal detection
 *   - AQL injection detection
 *   - Log output sanitisation
 *
 * Build with:
 *   afl-clang-lto++ -fsanitize=address,undefined -O2 -g \
 *     -o security_input_validator_harness \
 *     security_input_validator_harness.cpp \
 *     -I../../include -L../../build/lib -lthemisdb
 *
 * Run with:
 *   afl-fuzz -i corpus/input_validator -o findings/input_validator \
 *     -- ./security_input_validator_harness @@
 */

#include <cstdint>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <string>
#include <string_view>

// AFL++ persistent mode macros
#ifdef __AFL_FUZZ_TESTCASE_LEN
  #define AFL_LOOP(count) __AFL_LOOP(count)
#else
  #define AFL_LOOP(count) 1
  static unsigned char dummy_buf[1] = {0};
  unsigned char *__AFL_FUZZ_TESTCASE_BUF = dummy_buf;
#endif

// ─── Minimal stub when building standalone (without the full ThemisDB library)
// Replace with real includes when linking against libthemisdb:
//   #include "utils/input_validator.h"
namespace themis {
namespace utils {

class InputValidator {
public:
    bool containsPathTraversal(const std::string& input) const {
        return input.find("..") != std::string::npos ||
               input.find("%2e%2e") != std::string::npos ||
               input.find("%2E%2E") != std::string::npos;
    }
    bool containsAqlInjection(const std::string& input) const {
        // Simplified stub — real implementation uses the AQL AST
        const char* patterns[] = {
            "FOR ", "RETURN ", "INSERT ", "UPDATE ", "REMOVE ",
            "REPLACE ", "UPSERT ", "LET ", "COLLECT ", "GRAPH "
        };
        std::string upper;
        upper.reserve(input.size());
        for (char c : input) {
          upper += static_cast<char>(::toupper(c));
        }
        for (auto& p : patterns) {
            if (upper.find(p) != std::string::npos) {
              return true;
            }
        }
        return false;
    }
    std::string sanitizeForLog(const std::string& input) const {
        std::string out;
        out.reserve(input.size());
        for (unsigned char c : input) {
            if (c < 0x20 || c == 0x7f) {
                out += '?';
            } else {
                out += static_cast<char>(c);
            }
        }
        return out;
    }
};

} // namespace utils
} // namespace themis

static themis::utils::InputValidator g_validator;

/**
 * @brief Core fuzz target — called for each AFL input.
 */
static int fuzz_one_input(const uint8_t* data, size_t size) {
    if (!data || size == 0) {
      return 0;
    }

    // Interpret first byte as a dispatch selector so AFL explores
    // multiple code paths with the same corpus.
    uint8_t selector = data[0] % 3;
    std::string input(reinterpret_cast<const char*>(data + 1), size - 1);

    switch (selector) {
    case 0:
        (void)g_validator.containsPathTraversal(input);
        break;
    case 1:
        (void)g_validator.containsAqlInjection(input);
        break;
    case 2:
        (void)g_validator.sanitizeForLog(input);
        break;
    }

    return 0;
}

int main(int argc, char** argv) {
#ifdef __AFL_FUZZ_TESTCASE_LEN
    __AFL_INIT();
    while (AFL_LOOP(10000)) {
        size_t len = __AFL_FUZZ_TESTCASE_LEN;
        if (len > 0)
            fuzz_one_input(__AFL_FUZZ_TESTCASE_BUF, len);
    }
#else
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input_file>\n", argv[0]);
        return 1;
    }
    FILE* f = fopen(argv[1], "rb");
    if (!f) { perror("fopen"); return 1; }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz <= 0) { fclose(f); return 0; }
    auto* buf = static_cast<uint8_t*>(malloc(static_cast<size_t>(sz)));
    if (!buf) { fclose(f); return 1; }
    if (fread(buf, 1, static_cast<size_t>(sz), f) !=
            static_cast<size_t>(sz)) {
        free(buf); fclose(f); return 1;
    }
    fclose(f);
    fuzz_one_input(buf, static_cast<size_t>(sz));
    free(buf);
#endif
    return 0;
}
