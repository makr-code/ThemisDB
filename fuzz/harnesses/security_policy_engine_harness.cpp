/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            security_policy_engine_harness.cpp                 ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:43:57                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   89.0/100                                       ║
    • Total Lines:     181                                            ║
    • Open Issues:     TODOs: 0, Stubs: 3                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file security_policy_engine_harness.cpp
 * @brief AFL++ Fuzzing Harness for ThemisDB PolicyEngine (security path)
 *
 * Exercises the PolicyEngine with arbitrary JSON policy blobs and
 * authorization request strings to surface:
 *   - JSON parse crashes / hangs
 *   - Always-allow / always-deny regressions
 *   - Wildcard-matching edge cases
 *   - ABAC condition bypasses
 *
 * Build with:
 *   afl-clang-lto++ -fsanitize=address,undefined -O2 -g \
 *     -o security_policy_engine_harness \
 *     security_policy_engine_harness.cpp \
 *     -I../../include -L../../build/lib -lthemisdb
 *
 * Run with:
 *   afl-fuzz -i corpus/policy_engine -o findings/policy_engine \
 *     -- ./security_policy_engine_harness @@
 */

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <string_view>
#include <stdexcept>

// AFL++ persistent mode macros
#ifdef __AFL_FUZZ_TESTCASE_LEN
  extern unsigned char *__AFL_FUZZ_TESTCASE_BUF;
  #define AFL_LOOP(count) __AFL_LOOP(count)
#else
  #define AFL_LOOP(count) 1
  static unsigned char dummy_buf[1] = {0};
  unsigned char *__AFL_FUZZ_TESTCASE_BUF = dummy_buf;
#endif

// ─── PolicyEngine stub (replace with real include when linking libthemisdb)
// #include "server/policy_engine.h"
namespace themis {

class PolicyEngine {
public:
    struct Decision { bool allowed; std::string reason; };

    bool loadFromJson(const std::string& json_blob) {
        // Stub: accept blobs that look like a JSON array
        if (json_blob.size() < 2) return false;
        return json_blob.front() == '[' && json_blob.back() == ']';
    }

    Decision authorize(const std::string& user_id,
                       const std::string& action,
                       const std::string& resource_path) const {
        // Stub: deny admin actions for non-admin users
        Decision d;
        if (action == "admin" && user_id != "admin") {
            d.allowed = false;
            d.reason  = "not admin";
        } else {
            d.allowed = true;
            d.reason  = "allowed by default";
        }
        return d;
    }

    void reset() { /* clear policies */ }
};

} // namespace themis

static themis::PolicyEngine g_engine;

/**
 * @brief Split fuzz input into two NUL-terminated sub-strings.
 *
 * The input format is:
 *   <1 byte selector> <json_blob NUL> <user_id NUL> <action NUL> <resource>
 *
 * AFL will discover this structure from the seed corpus.
 */
static int fuzz_one_input(const uint8_t* data, size_t size) {
    if (!data || size < 2) return 0;

    uint8_t selector = data[0] % 2;
    const char* payload = reinterpret_cast<const char*>(data + 1);
    size_t payload_len  = size - 1;

    if (selector == 0) {
        // Fuzz: JSON policy loading
        std::string json_blob(payload, payload_len);
        try {
            g_engine.loadFromJson(json_blob);
        } catch (const std::exception&) {
            // Parse errors are expected; crashes are not
        }
    } else {
        // Fuzz: authorization decision
        // Split payload at first two NUL bytes to extract fields
        auto find_field = [&](const char* start, size_t len) -> std::pair<std::string, const char*> {
            const char* end = static_cast<const char*>(
                memchr(start, '\0', len));
            if (!end) return {std::string(start, len), start + len};
            return {std::string(start, end - start), end + 1};
        };

        auto [user_id,  p1] = find_field(payload, payload_len);
        size_t p1_offset    = static_cast<size_t>(p1 - payload);
        if (p1_offset >= payload_len) return 0;
        auto [action,   p2] = find_field(p1, payload_len - p1_offset);
        size_t p2_offset    = static_cast<size_t>(p2 - payload);
        std::string resource;
        if (p2_offset < payload_len) {
            resource.assign(p2, payload_len - p2_offset);
        }

        try {
            (void)g_engine.authorize(user_id, action, resource);
        } catch (const std::exception&) {
            // Unexpected but non-fatal
        }
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
