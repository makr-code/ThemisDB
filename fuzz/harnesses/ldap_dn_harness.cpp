/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ldap_dn_harness.cpp                                ║
  Version:         0.0.13                                             ║
  Last Modified:   2026-04-15 18:43:57                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     229                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 0f9b874f42  2026-03-12  fix(auth): address all reviewer comments on LDAP injectio... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <unistd.h>

#ifdef AFL_FUZZ_BUILD
#include "auth/ldap_authenticator.h"
using themis::auth::LDAPAuthenticator;
using themis::auth::LDAPConfig;
#else
// ---------------------------------------------------------------------------
// Minimal inline stubs used when building the harness template without the
// full ThemisDB build tree.  Replace with the real headers when linking.
// ---------------------------------------------------------------------------
#include <string>
#include <algorithm>

namespace {

static std::string escapeLDAPDNComponent(const std::string& value)
{
    if (value.empty()) {
      return value;
    }
    std::string out = {};
    out.reserve(value.size() * 2);
    for (std::size_t i = 0; i < value.size(); ++i) {
        const unsigned char c = static_cast<unsigned char>(value[i]);
        if (i == 0 && c == '#')            { out += "\\#"; continue; }
        if (c == ' ' && (i == 0 || i == value.size() - 1)) { out += "\\ "; continue; }
        switch (c) {
            case ',':  out += "\\,";  break;
            case '+':  out += "\\+";  break;
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '<':  out += "\\<";  break;
            case '>':  out += "\\>";  break;
            case ';':  out += "\\;";  break;
            case '=':  out += "\\=";  break;
            case '\0': out += "\\00"; break;
            default:   out += static_cast<char>(c); break;
        }
    }
    return out;
}

struct LDAPConfig {
    std::string server_url       = "ldap://dc.example.com:389";
    std::string bind_dn_template = "CN={username},OU=Users,DC=example,DC=com";
};

class LDAPAuthenticator {
public:
    bool initialize(const LDAPConfig& c) { cfg_ = c; return true; }
    std::string buildUserDN(const std::string& username) const {
        std::string dn = cfg_.bind_dn_template;
        const std::string ph = "{username}";
        const auto pos = dn.find(ph);
        if (pos != std::string::npos)
            dn.replace(pos, ph.size(), escapeLDAPDNComponent(username));
        return dn;
    }
private:
    LDAPConfig cfg_;
};

} // anonymous namespace
#endif // AFL_FUZZ_BUILD

// ---------------------------------------------------------------------------
// Harness invariant helper: always-on abort (survives -DNDEBUG builds).
// The message is written to stderr before trapping to aid crash diagnosis.
// ---------------------------------------------------------------------------
#define FUZZ_CHECK(cond, msg) \
    do { if (!(cond)) { \
        const char* m_ = (msg); \
        (void)::write(2, m_, __builtin_strlen(m_)); \
        (void)::write(2, "\n", 1); \
        __builtin_trap(); \
    } } while (0)

// ---------------------------------------------------------------------------
// DN special characters that must never appear unescaped in the constructed DN
// value component (i.e., in the substituted part, not the template).
// ---------------------------------------------------------------------------
static const char k_dn_special[] = ",+\"\\<>;=";

// Template components for post-substitution structural checks.
// With bind_dn_template = "CN={username},OU=Users,DC=example,DC=com"
// the prefix before {username} is "CN=" and the suffix after is
// ",OU=Users,DC=example,DC=com".
static const char k_prefix[] = "CN=";
static const char k_suffix[] = ",OU=Users,DC=example,DC=com";

// ---------------------------------------------------------------------------
// libFuzzer entry point
// ---------------------------------------------------------------------------
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    // Reject oversized inputs to keep the harness fast.  512 bytes is a
    // practical upper bound for a real username; longer inputs are extremely
    // unlikely to trigger new code paths in the escaping logic and would
    // substantially slow down the fuzzer's iteration rate.
    if (size == 0 || size > 512) {
        return 0;
    }

    const std::string username(reinterpret_cast<const char*>(data), size);

    LDAPConfig cfg;
    cfg.server_url       = "ldap://dc.example.com:389";
    cfg.bind_dn_template = "CN={username},OU=Users,DC=example,DC=com";

    LDAPAuthenticator auth;
    auth.initialize(cfg);

    // Must not crash on any input.
    const std::string dn = auth.buildUserDN(username);

    // 1. The DN must always start with the template prefix.
    FUZZ_CHECK(dn.substr(0, sizeof(k_prefix) - 1) == k_prefix,
               "LDAP DN: missing expected prefix after substitution");

    // 2. The DN must always end with the template suffix.
    FUZZ_CHECK(dn.size() >= (sizeof(k_suffix) - 1) &&
               dn.substr(dn.size() - (sizeof(k_suffix) - 1)) == k_suffix,
               "LDAP DN: missing expected suffix after substitution");

    // 3. Inspect the substituted value component (between prefix and suffix).
    const std::size_t prefix_len = sizeof(k_prefix) - 1;
    const std::size_t suffix_len = sizeof(k_suffix) - 1;
    const std::string value_part = dn.substr(prefix_len,
                                             dn.size() - prefix_len - suffix_len);

    for (std::size_t i = 0; i < value_part.size(); ++i) {
        const char c = value_part[i];
        if (c == '\\') {
            // A backslash introduces an escape sequence — skip the next char.
            ++i;
            continue;
        }

        // 3a. DN special characters must never appear unescaped.
        for (const char special : k_dn_special) {
            FUZZ_CHECK(c != special,
                       "LDAP DN injection: unescaped special character in value component");
        }
    }

    // 4. If the value component is non-empty, its first character must not be
    //    an unescaped leading '#' (RFC 4514 §2.4 requires it be escaped as \#).
    if (!value_part.empty()) {
        FUZZ_CHECK(value_part[0] != '#',
                   "LDAP DN injection: unescaped leading '#' in value component");

        // 5. The first character must not be an unescaped leading space.
        FUZZ_CHECK(value_part[0] != ' ',
                   "LDAP DN injection: unescaped leading space in value component");

        // 6. The last character must not be an unescaped trailing space.
        //    (A trailing space would be escaped as "\ " — two chars — so a lone
        //    space at the end is invalid.)
        FUZZ_CHECK(value_part.back() != ' ',
                   "LDAP DN injection: unescaped trailing space in value component");
    }

    return 0;
}

// ---------------------------------------------------------------------------
// AFL++ persistent-mode wrapper / standalone stdin driver.
//
// libFuzzer (clang -fsanitize=fuzzer) provides its own main(); defining
// another one causes a link error.  This file is never compiled into the
// production library — it is only used as a fuzzing harness — so main() is
// safe to include here for AFL++ persistent mode (__AFL_FUZZ_TESTCASE_LEN)
// and for standalone manual testing.  When building with libFuzzer, add
// -DFUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION (set automatically by
// -fsanitize=fuzzer on clang) to suppress this definition.
// ---------------------------------------------------------------------------
#if defined(__AFL_FUZZ_TESTCASE_LEN) || !defined(FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION)
int main(int argc, char** argv)
{
#ifdef __AFL_FUZZ_TESTCASE_LEN
    __AFL_INIT();
    unsigned char* buf = __AFL_FUZZ_TESTCASE_BUF;
    while (__AFL_LOOP(100000)) {
        std::size_t len = static_cast<std::size_t>(__AFL_FUZZ_TESTCASE_LEN);
        LLVMFuzzerTestOneInput(buf, len);
    }
#else
    // Single-shot: read from stdin for manual testing.
    std::string line = {};
    std::string input = {};
    while (std::getline(std::cin, line)) {
        input += line + "\n";
    }
    if (!input.empty()) {
        LLVMFuzzerTestOneInput(
            reinterpret_cast<const uint8_t*>(input.data()), input.size());
    }
    (void)argc; (void)argv;
#endif
    return 0;
}
#endif // !libFuzzer
