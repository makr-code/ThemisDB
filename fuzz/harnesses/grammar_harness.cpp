/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            grammar_harness.cpp                                ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:43:57                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     169                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file grammar_harness.cpp
 * @brief AFL++ Fuzzing Harness for Grammar::compile() (both constructors)
 *
 * Feeds arbitrary byte sequences to Grammar constructors and verifies that
 * no undefined behaviour (crash, heap overflow, etc.) occurs regardless of
 * the EBNF text or start symbol supplied.  The expected outcome for every
 * input is: either a valid Grammar (isValid() == true) or a Grammar with a
 * descriptive error (isValid() == false, getError() non-empty) — never a
 * crash.
 *
 * The harness splits the fuzz input at the first NUL byte (\\x00):
 *   bytes before NUL → EBNF text
 *   bytes after NUL  → start symbol
 * If no NUL is found, the entire buffer is used as EBNF text and
 * "root" is used as start symbol.
 *
 * Build with:
 *   afl-clang-lto++ -fsanitize=address,undefined -O2 -g \
 *     -o grammar_harness grammar_harness.cpp \
 *     -I../../include -L../../build/lib -lthemisdb_llm
 *
 * Run with:
 *   afl-fuzz -i fuzz/corpus/llm/grammar -o fuzz/findings/grammar \
 *     -- ./grammar_harness @@
 */

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

// AFL++ persistent mode macros
#ifdef __AFL_FUZZ_TESTCASE_LEN
  #define AFL_LOOP(count) __AFL_LOOP(count)
#else
  #define AFL_LOOP(count) 1
  unsigned char *__AFL_FUZZ_TESTCASE_BUF = nullptr;
#endif

#include "llm/grammar.h"

using namespace themis::llm;

/**
 * @brief Exercise both Grammar constructors with the fuzz input.
 *
 * Invariants checked:
 *  - Constructors must not crash.
 *  - isValid() must return a consistent bool.
 *  - getError() must be accessible (exercises the string copy path).
 *  - getHandle() must be safe to call (either valid ptr or nullptr).
 *  - Move constructor must not crash.
 */
static int fuzz_one_input(const uint8_t* data, size_t size) {
    if (data == nullptr || size == 0) {
        return 0;
    }

    // Split at first NUL byte to produce EBNF + start-symbol
    const uint8_t* nul = static_cast<const uint8_t*>(memchr(data, 0, size));
    std::string ebnf, start;
    if (nul) {
        ebnf  = std::string(reinterpret_cast<const char*>(data),
                            static_cast<size_t>(nul - data));
        start = std::string(reinterpret_cast<const char*>(nul + 1),
                            size - static_cast<size_t>(nul - data) - 1);
    } else {
        ebnf  = std::string(reinterpret_cast<const char*>(data), size);
        start = "root";
    }

    // ── Constructor 1: basic (no model) ──────────────────────────────────
    {
        Grammar g(ebnf, start);
        (void)g.isValid();
        (void)g.getError();
        (void)g.getEBNFText();
        (void)g.getStartSymbol();
        (void)g.getHandle();

        // Move
        Grammar g2(std::move(g));
        (void)g2.isValid();
    }

    // ── Constructor 2: null model (hard-error path) ───────────────────────
    {
        Grammar g(ebnf, start, nullptr);
        (void)g.isValid();
        (void)g.getError();
        (void)g.getHandle();
    }

    return 0;
}

int main(int argc, char** argv) {
#ifdef __AFL_FUZZ_TESTCASE_LEN
    __AFL_INIT();

    while (AFL_LOOP(10000)) {
        size_t len = __AFL_FUZZ_TESTCASE_LEN;
        if (len > 0 && __AFL_FUZZ_TESTCASE_BUF != nullptr) {
            fuzz_one_input(__AFL_FUZZ_TESTCASE_BUF, len);
        }
    }
#else
    // Standalone mode
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input_file>\n", argv[0]);
        return 1;
    }

    FILE* f = fopen(argv[1], "rb");
    if (!f) {
        perror("Failed to open input file");
        return 1;
    }

    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (fsize <= 0) {
        fclose(f);
        return 0;
    }

    uint8_t* buf = static_cast<uint8_t*>(malloc(static_cast<size_t>(fsize)));
    if (!buf) {
        fclose(f);
        return 1;
    }

    size_t n = fread(buf, 1, static_cast<size_t>(fsize), f);
    fclose(f);

    int result = fuzz_one_input(buf, n);
    free(buf);
    return result;
#endif

    return 0;
}
