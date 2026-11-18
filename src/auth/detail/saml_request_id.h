#pragma once

#include <array>
#include <stdexcept>
#include <string>

namespace themis::auth::detail {

inline std::string generateSamlRequestId(int (*random_bytes)(unsigned char*, int)) {
    if (random_bytes == nullptr) {
        throw std::invalid_argument("SAML request ID random-byte provider is missing");
    }

    std::array<unsigned char, 16> random{};
    if (random_bytes(random.data(), static_cast<int>(random.size())) != 1) {
        throw std::runtime_error("SAML: failed to generate cryptographically secure request ID");
    }

    constexpr char hex[] = "0123456789abcdef";
    std::string request_id{"_"};
    request_id.reserve(1 + random.size() * 2);
    for (const unsigned char byte : random) {
        request_id.push_back(hex[byte >> 4]);
        request_id.push_back(hex[byte & 0x0F]);
    }
    return request_id;
}

} // namespace themis::auth::detail
