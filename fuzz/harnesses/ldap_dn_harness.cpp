/*
 * ThemisDB - Hybrid Database System
 *
 * File:            ldap_dn_harness.cpp
 * Description:     libFuzzer / AFL++ harness for LDAP DN injection prevention.
 *
 * Fuzz-tests LDAPAuthenticator::buildUserDN() with arbitrary username inputs
 * to verify:
 *   1. No crashes or memory errors on any byte sequence.
 *   2. The returned DN always starts with the template prefix (before {username}).
 *   3. DN special characters in the input never appear unescaped in the output.
 *
 * Build with libFuzzer:
 * @code
 *   clang++ -fsanitize=fuzzer,address,undefined -O1 -g \
 *     -DAFL_FUZZ_BUILD \
 *     -o ldap_dn_harness ldap_dn_harness.cpp \
 *     -I../../include -L../../build/lib \
 *     -lthemisdb_auth -lthemisdb_utils \
 *     -lssl -lcrypto -lspdlog
 * @endcode
 *
 * Build with AFL++:
 * @code
 *   afl-clang-lto++ -fsanitize=address,undefined -O2 -g \
 *     -DAFL_FUZZ_BUILD \
 *     -o ldap_dn_harness ldap_dn_harness.cpp \
 *     -I../../include -L../../build/lib \
 *     -lthemisdb_auth -lthemisdb_utils \
 *     -lssl -lcrypto -lspdlog
 * @endcode
 */

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <cassert>

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
    if (value.empty()) return value;
    std::string out;
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
            dn.replace(pos, ph.size(), escapeDN(username));
        return dn;
    }
private:
    LDAPConfig cfg_;
};

} // anonymous namespace
#endif // AFL_FUZZ_BUILD

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
    assert(dn.substr(0, sizeof(k_prefix) - 1) == k_prefix &&
           "LDAP DN: missing expected prefix after substitution");

    // 2. The DN must always end with the template suffix.
    assert(dn.size() >= (sizeof(k_suffix) - 1) &&
           dn.substr(dn.size() - (sizeof(k_suffix) - 1)) == k_suffix &&
           "LDAP DN: missing expected suffix after substitution");

    // 3. The substituted portion (between prefix and suffix) must not contain
    //    any unescaped DN special character.
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
        for (const char special : k_dn_special) {
            assert(c != special &&
                   "LDAP DN injection: unescaped special character in value component");
        }
    }

    return 0;
}

// ---------------------------------------------------------------------------
// AFL++ persistent-mode wrapper / stdin driver
// ---------------------------------------------------------------------------
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
    std::string line;
    std::string input;
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
