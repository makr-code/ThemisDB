/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            http_parser_harness.cpp                            ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:43:57                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     239                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file http_parser_harness.cpp
 * @brief AFL++ Fuzzing Harness for ThemisDB HTTP request parser
 *
 * Fuzzes Boost.Beast HTTP/1.x parsing with arbitrary raw bytes.
 * Catches crashes, assertion failures, and memory errors triggered by
 * malformed request lines, headers, or body content.
 *
 * Build with AFL++ (persistent mode recommended):
 *   afl-clang-lto++ -fsanitize=address,undefined -O2 -g            \
 *     -DAFL_HARNESS                                                  \
 *     -o http_parser_harness http_parser_harness.cpp                \
 *     -I../../include                                                \
 *     -lboost_system -lssl -lcrypto
 *
 * Run:
 *   mkdir -p corpus_http && echo "GET / HTTP/1.1\r\nHost: x\r\n\r\n" > corpus_http/seed0
 *   afl-fuzz -i corpus_http -o findings_http -- ./http_parser_harness @@
 *
 * Standalone test (no AFL++):
 *   ./http_parser_harness corpus_http/seed0
 */

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <string_view>

// Boost.Beast HTTP parser – the actual target being fuzzed
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

namespace beast = boost::beast;
namespace http  = beast::http;

// AFL++ persistent mode macros
#ifdef __AFL_FUZZ_TESTCASE_LEN
  #define AFL_LOOP(count) __AFL_LOOP(count)
#else
  #define AFL_LOOP(count) 1
  unsigned char *__AFL_FUZZ_TESTCASE_BUF = nullptr;
#endif

// Hard limit: skip inputs larger than 64 KB to keep fuzzing fast
static constexpr size_t MAX_INPUT_BYTES = 65536;

/**
 * @brief Attempt to parse raw bytes as an HTTP/1.x request.
 *
 * We intentionally ignore parse errors – the goal is to ensure the
 * parser never crashes, enters an infinite loop, or corrupts memory.
 *
 * @param data  Raw fuzz input
 * @param size  Length of the input
 * @return 0 always (non-zero would indicate a detected crash)
 */
static int fuzz_one_input(const uint8_t* data, size_t size) {
    if (data == nullptr || size == 0 || size > MAX_INPUT_BYTES) {
        return 0;
    }

    try {
        // Feed fuzz bytes into a Beast flat_buffer
        beast::flat_buffer buf;
        auto mutable_buf = buf.prepare(size);
        std::memcpy(mutable_buf.data(), data, size);
        buf.commit(size);

        // Try to parse as a string-body HTTP/1.1 request
        http::request_parser<http::string_body> parser;
        // Disable body limit so we can fuzz arbitrarily large bodies
        parser.body_limit(boost::optional<std::uint64_t>(boost::none));

        beast::error_code ec;
        parser.put(buf.data(), ec);
        // ec is expected to be set for malformed inputs – that's fine.
        // What we must NOT see is a crash or memory error.

        if (!ec) {
            // If parsing succeeded, try to access the parsed fields safely
            if (parser.is_header_done()) {
                auto& msg = parser.get();
                // Access method, target, version – exercise all accessors
                (void)msg.method();
                (void)msg.method_string();
                (void)msg.target();
                (void)msg.version();
                (void)msg.keep_alive();

                // Iterate over header fields
                for (auto const& field : msg) {
                    (void)field.name_string();
                    (void)field.value();
                }

                // Access body
                if (parser.is_done()) {
                    (void)msg.body().size();
                }
            }
        }
    } catch (const std::exception& e) {
        // Exceptions from the parser are acceptable (not a crash)
        (void)e;
    } catch (...) {
        // Unknown exception – still not a crash
    }

    return 0;
}

// ---------------------------------------------------------------------------
// Seed corpus entries – used in standalone / CI mode to verify the harness
// compiles and runs without errors before handing over to AFL++.
// ---------------------------------------------------------------------------

// Store each seed with its explicit byte length so null bytes are handled correctly
struct SeedInput {
    const char* data;
    size_t      len;
};

#define SEED(s) { (s), sizeof(s) - 1 }

static const SeedInput SEED_INPUTS[] = {
    // Valid minimal request
    SEED("GET / HTTP/1.1\r\nHost: localhost\r\n\r\n"),
    // POST with body
    SEED("POST /api/entities HTTP/1.1\r\nHost: x\r\nContent-Length: 2\r\n\r\n{}"),
    // Missing CRLF terminator
    SEED("GET /health HTTP/1.1\r\nHost: localhost"),
    // Header without value
    SEED("GET / HTTP/1.1\r\nHost:\r\n\r\n"),
    // Very long header value
    SEED("GET / HTTP/1.1\r\nHost: localhost\r\nX-Custom: "
         "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
         "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
         "\r\n\r\n"),
    // Null byte in header (length includes the \x00 byte)
    SEED("GET / HTTP/1.1\r\nHost: \x00\r\n\r\n"),
    // Garbage bytes
    SEED("\x00\x01\x02\x03\x04\x05\x06\x07"),
    // HTTP/0.9 style
    SEED("GET /\n"),
    // Body longer than Content-Length declares
    SEED("POST / HTTP/1.1\r\nHost: x\r\nContent-Length: 1\r\n\r\nACTUALLYMORE"),
    // Completely empty (len == 0 – fuzz_one_input handles gracefully)
    { "", 0 },
};

#undef SEED

static constexpr int SEED_COUNT =
    static_cast<int>(sizeof(SEED_INPUTS) / sizeof(SEED_INPUTS[0]));

/**
 * @brief Main entry point
 *
 * Supports three modes:
 *  1. AFL++ persistent mode  – compiled with __AFL_FUZZ_TESTCASE_LEN
 *  2. File mode              – ./harness <path-to-file>
 *  3. Seed verification mode – ./harness (no arguments) runs built-in seeds
 */
int main(int argc, char** argv) {
#ifdef __AFL_FUZZ_TESTCASE_LEN
    // AFL++ persistent mode
    __AFL_INIT();
    while (AFL_LOOP(10000)) {
        size_t len = __AFL_FUZZ_TESTCASE_LEN;
        if (len > 0 && __AFL_FUZZ_TESTCASE_BUF != nullptr) {
            fuzz_one_input(__AFL_FUZZ_TESTCASE_BUF, len);
        }
    }
    return 0;
#else
    if (argc >= 2) {
        // File input mode
        FILE* f = fopen(argv[1], "rb");
        if (!f) {
            perror("Failed to open input file");
            return 1;
        }
        fseek(f, 0, SEEK_END);
        long fsz = ftell(f);
        fseek(f, 0, SEEK_SET);
        if (fsz <= 0) { fclose(f); return 0; }

        auto* buf = static_cast<uint8_t*>(malloc(static_cast<size_t>(fsz)));
        if (!buf) { fclose(f); return 1; }
        if (fread(buf, 1, static_cast<size_t>(fsz), f) != static_cast<size_t>(fsz)) {
            perror("Failed to read input file");
            free(buf); fclose(f); return 1;
        }
        fclose(f);

        int ret = fuzz_one_input(buf, static_cast<size_t>(fsz));
        free(buf);
        return ret;
    } else {
        // Seed verification mode: run built-in seeds
        fprintf(stderr, "http_parser_harness: running %d built-in seed inputs\n", SEED_COUNT);
        for (int i = 0; i < SEED_COUNT; ++i) {
            const SeedInput& s = SEED_INPUTS[i];
            int rc = fuzz_one_input(reinterpret_cast<const uint8_t*>(s.data), s.len);
            if (rc != 0) {
                fprintf(stderr, "Seed %d triggered error (rc=%d)\n", i, rc);
                return 1;
            }
        }
        fprintf(stderr, "All seeds passed.\n");
        return 0;
    }
#endif
}
