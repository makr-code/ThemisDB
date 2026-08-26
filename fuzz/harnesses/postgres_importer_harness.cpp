/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            postgres_importer_harness.cpp                      ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:43:57                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     301                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file postgres_importer_harness.cpp
 * @brief AFL++ / LibFuzzer harness for the ThemisDB PostgreSQL pg_dump importer
 *
 * Exercises the four public parsing helpers of PostgreSQLImporter that consume
 * untrusted input:
 *   - parseCopyRow      (COPY text-format row tokeniser + unescaper)
 *   - parseInsertValues (INSERT VALUES clause tokeniser)
 *   - parseCreateTable  (CREATE TABLE DDL parser)
 *   - isValidUtf8       (byte-level UTF-8 validator)
 *
 * All four functions must:
 *   - never crash, abort, or access out-of-bounds memory
 *   - never loop infinitely on any finite input
 *   - return a valid (possibly empty) result for every input
 *
 * The harness also feeds synthetic COPY data blocks through a temporary file
 * exercising the full parseCopy() / parseDumpFile() pipeline so that file I/O
 * and the bounded streamReadLine() path are covered.
 *
 * Build with AFL++ (persistent mode, ASan + UBSan):
 * @code
 *   afl-clang-lto++ -fsanitize=address,undefined -O2 -g          \
 *     -DAFL_HARNESS                                               \
 *     -o postgres_importer_harness postgres_importer_harness.cpp \
 *     -I../../include                                             \
 *     -L../../build/lib -lthemisdb_importers
 * @endcode
 *
 * Build as a standalone LibFuzzer target:
 * @code
 *   clang++ -fsanitize=fuzzer,address,undefined -O2 -g            \
 *     -DLIBFUZZER_HARNESS                                          \
 *     -o postgres_importer_fuzz postgres_importer_harness.cpp     \
 *     -I../../include                                              \
 *     -L../../build/lib -lthemisdb_importers
 * @endcode
 *
 * Standalone (no fuzzer – for regression testing):
 * @code
 *   clang++ -O2 -g                                                 \
 *     -o postgres_importer_harness_standalone                      \
 *        postgres_importer_harness.cpp                             \
 *     -I../../include                                              \
 *     -L../../build/lib -lthemisdb_importers
 *   ./postgres_importer_harness_standalone fuzz/corpus/importer/seed_copy.sql
 * @endcode
 */

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

// Include the importer interface and implementation
#include "importers/postgres_importer.h"

namespace {

// Hard limits to keep fuzzing fast
static constexpr size_t kMaxInputBytes  = 256 * 1024;   // 256 KB
static constexpr size_t kMaxRowBytes    = 16  * 1024;   //  16 KB

/**
 * @brief Core fuzz function – called for each test case.
 *
 * We partition the raw fuzz bytes into four sub-exercises based on the first
 * byte (a "selector") so AFL++ coverage feedback steers generation toward all
 * four parsers.
 */
static int fuzz_one_input(const uint8_t* data, size_t size) {
    if (data == nullptr || size == 0 || size > kMaxInputBytes) return 0;

    using namespace themis::importers;

    // Build a string_view of the payload (everything after the selector byte)
    uint8_t selector = data[0];
    std::string_view payload(reinterpret_cast<const char*>(data + 1),
                             size > 1 ? size - 1 : 0);
    std::string payload_str(payload);

    // Create a fresh importer for each test case
    PostgreSQLImporter importer;
    importer.initialize("{}");

    switch (selector & 0x03) {

    // ------------------------------------------------------------------ //
    // Selector 0: parseCopyRow                                            //
    // ------------------------------------------------------------------ //
    case 0: {
        // parseCopyRow is a const method – access via the public test helper
        // (declared as a friend in the test build or via a thin wrapper).
        // Here we exercise it indirectly through a synthetic COPY dump.
        //
        // Build a minimal dump:
        //   CREATE TABLE fuzz (a text, b text, c text);
        //   COPY fuzz (a, b, c) FROM stdin;
        //   <payload>
        //   \.
        std::string dump =
            "-- PostgreSQL database dump\n"
            "CREATE TABLE fuzz (a text, b text, c text);\n"
            "COPY fuzz (a, b, c) FROM stdin;\n";
        dump += payload_str + "\n";
        dump += "\\.\n";

        // Write to a temp file
        char tmp[] = "/tmp/pg_fuzz_XXXXXX";
        int fd = mkstemp(tmp);
        if (fd < 0) return 0;
        (void)write(fd, dump.data(), dump.size());
        close(fd);

        ImportOptions opts;
        opts.max_row_size_bytes    = kMaxRowBytes;
        opts.max_statement_size_bytes = kMaxInputBytes;
        opts.continue_on_error     = true;
        opts.enforce_utf8          = false;  // UTF-8 test is in selector 3

        ImportStats stats;
        importer.importData(tmp, opts, nullptr);
        remove(tmp);
        break;
    }

    // ------------------------------------------------------------------ //
    // Selector 1: parseInsertValues via INSERT INTO dump                  //
    // ------------------------------------------------------------------ //
    case 1: {
        std::string dump =
            "-- PostgreSQL database dump\n"
            "CREATE TABLE fuzz (a text, b integer);\n"
            "INSERT INTO fuzz (a, b) VALUES (";
        dump += payload_str;
        dump += ");\n";

        char tmp[] = "/tmp/pg_fuzz_XXXXXX";
        int fd = mkstemp(tmp);
        if (fd < 0) return 0;
        (void)write(fd, dump.data(), dump.size());
        close(fd);

        ImportOptions opts;
        opts.max_statement_size_bytes = kMaxInputBytes;
        opts.continue_on_error        = true;

        importer.importData(tmp, opts, nullptr);
        remove(tmp);
        break;
    }

    // ------------------------------------------------------------------ //
    // Selector 2: parseCreateTable via CREATE TABLE dump                  //
    // ------------------------------------------------------------------ //
    case 2: {
        std::string dump =
            "-- PostgreSQL database dump\n"
            "CREATE TABLE fuzz (";
        dump += payload_str;
        dump += ");\n";

        char tmp[] = "/tmp/pg_fuzz_XXXXXX";
        int fd = mkstemp(tmp);
        if (fd < 0) return 0;
        (void)write(fd, dump.data(), dump.size());
        close(fd);

        ImportOptions opts;
        opts.max_statement_size_bytes = kMaxInputBytes;
        opts.continue_on_error        = true;

        std::vector<std::string> errors;
        importer.validateSource(tmp, errors);
        importer.importData(tmp, opts, nullptr);
        remove(tmp);
        break;
    }

    // ------------------------------------------------------------------ //
    // Selector 3: isValidUtf8 + enforce_utf8 COPY pipeline               //
    // ------------------------------------------------------------------ //
    default: /* case 3 */ {
        std::string dump =
            "-- PostgreSQL database dump\n"
            "CREATE TABLE fuzz (a text);\n"
            "COPY fuzz (a) FROM stdin;\n";
        dump += payload_str + "\n";
        dump += "\\.\n";

        char tmp[] = "/tmp/pg_fuzz_XXXXXX";
        int fd = mkstemp(tmp);
        if (fd < 0) return 0;
        (void)write(fd, dump.data(), dump.size());
        close(fd);

        ImportOptions opts;
        opts.max_row_size_bytes    = kMaxRowBytes;
        opts.continue_on_error     = true;
        opts.enforce_utf8          = true;  // Stress UTF-8 validator path

        importer.importData(tmp, opts, nullptr);
        remove(tmp);
        break;
    }
    }  // switch

    return 0;
}

}  // anonymous namespace


// ============================================================================
// Entry points
// ============================================================================

#if defined(LIBFUZZER_HARNESS)
// ---- LibFuzzer entry point -------------------------------------------------
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    return fuzz_one_input(data, size);
}

#elif defined(AFL_HARNESS)
// ---- AFL++ persistent-mode entry point ------------------------------------
#ifdef __AFL_FUZZ_TESTCASE_LEN
#define AFL_LOOP(n) __AFL_LOOP(n)
#else
#define AFL_LOOP(n) 1
unsigned char* __AFL_FUZZ_TESTCASE_BUF = nullptr;
#endif

int main(int /*argc*/, char** /*argv*/) {
    while (AFL_LOOP(10000)) {
        if (__AFL_FUZZ_TESTCASE_BUF == nullptr) break;
        size_t len = *reinterpret_cast<const size_t*>(__AFL_FUZZ_TESTCASE_BUF);
        const uint8_t* buf = __AFL_FUZZ_TESTCASE_BUF + sizeof(size_t);
        fuzz_one_input(buf, len);
    }
    return 0;
}

#else
// ---- Standalone / regression mode -----------------------------------------
// Reads a file from argv[1] and runs it through fuzz_one_input once.
#include <cerrno>
int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input_file>\n", argv[0]);
        return 2;
    }
    FILE* fp = fopen(argv[1], "rb");
    if (!fp) {
        perror("fopen");
        return 1;
    }
    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp);
    rewind(fp);
    if (fsize <= 0 || static_cast<size_t>(fsize) > kMaxInputBytes) {
        fprintf(stderr, "Input file size %ld out of bounds\n", fsize);
        fclose(fp);
        return 1;
    }
    std::vector<uint8_t> buf(static_cast<size_t>(fsize));
    size_t read_bytes = fread(buf.data(), 1, buf.size(), fp);
    fclose(fp);
    if (read_bytes != buf.size()) {
        fprintf(stderr, "Short read\n");
        return 1;
    }
    return fuzz_one_input(buf.data(), buf.size());
}
#endif  // entry point selection
