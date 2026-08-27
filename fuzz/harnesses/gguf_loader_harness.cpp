/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            gguf_loader_harness.cpp                            ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:43:57                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     148                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file gguf_loader_harness.cpp
 * @brief AFL++ Fuzzing Harness for GGUFLoader::parseFile()
 *
 * Feeds arbitrary byte sequences to GGUFLoader::parseFile() and verifies that
 * no undefined behaviour (buffer overflow, use-after-free, etc.) occurs.
 * Expected outcome for all inputs: either a valid parse result or a
 * descriptive error from getLastError() — never a crash or hang.
 *
 * Build with:
 *   afl-clang-lto++ -fsanitize=address,undefined -O2 -g \
 *     -o gguf_loader_harness gguf_loader_harness.cpp \
 *     -I../../include -L../../build/lib -lthemisdb_llm
 *
 * Run with:
 *   afl-fuzz -i fuzz/corpus/llm/gguf -o fuzz/findings/gguf \
 *     -- ./gguf_loader_harness @@
 */

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <fstream>
#include <filesystem>

// AFL++ persistent mode macros
#ifdef __AFL_FUZZ_TESTCASE_LEN
  #define AFL_LOOP(count) __AFL_LOOP(count)
#else
  #define AFL_LOOP(count) 1
  unsigned char *__AFL_FUZZ_TESTCASE_BUF = nullptr;
#endif

#include "llm/gguf_loader.h"

using namespace themis::llm;

static char g_tmpfile[256];

/**
 * @brief Write @p data to a temporary file and call GGUFLoader::parseFile().
 *
 * The GGUF loader reads from a path; we materialise the fuzz input as a
 * temporary file, parse it, then clean up.  The important invariant is that
 * parseFile() must never crash regardless of the input content.
 */
static int fuzz_one_input(const uint8_t* data, size_t size) {
    if (data == nullptr || size == 0) {
        return 0;
    }

    // Write fuzz bytes to temp file
    FILE* f = fopen(g_tmpfile, "wb");
    if (!f) {
        return 0;  // Can't write — skip rather than abort
    }
    fwrite(data, 1, size, f);
    fclose(f);

    // Attempt to parse — must not crash; may return error
    GGUFLoader loader;
    (void)loader.parseFile(g_tmpfile);
    // Access error message to ensure it is reachable (exercises string path)
    (void)loader.getLastError();

    // Cleanup
    std::remove(g_tmpfile);

    return 0;
}

int main(int argc, char** argv) {
    // Set up temp file path
    snprintf(g_tmpfile, sizeof(g_tmpfile), "/tmp/gguf_fuzz_%d.gguf",
             static_cast<int>(getpid()));

#ifdef __AFL_FUZZ_TESTCASE_LEN
    __AFL_INIT();

    while (AFL_LOOP(10000)) {
        size_t len = __AFL_FUZZ_TESTCASE_LEN;
        if (len > 0 && __AFL_FUZZ_TESTCASE_BUF != nullptr) {
            fuzz_one_input(__AFL_FUZZ_TESTCASE_BUF, len);
        }
    }
#else
    // Standalone mode: read from file argument
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
