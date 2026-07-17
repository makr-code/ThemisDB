#include "security/signing.h"

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace themis {
namespace {

class MockSigningService final : public SigningService {
public:
    SigningResult sign(const std::vector<uint8_t>& data, const std::string& key_id) override {
        SigningResult result;
        result.algorithm = "RSA-4096-SHA256";

        const std::uint64_t digest = fnv1a64(data);
        const std::string sig = key_id + ":" + toHex(digest);
        result.signature.assign(sig.begin(), sig.end());
        return result;
    }

    bool verify(const std::vector<uint8_t>& data,
                const std::vector<uint8_t>& signature,
                const std::string& key_id) override {
        const std::uint64_t digest = fnv1a64(data);
        const std::string expected = key_id + ":" + toHex(digest);
        const std::string got(signature.begin(), signature.end());
        return got == expected;
    }

private:
    static std::uint64_t fnv1a64(const std::vector<uint8_t>& data) {
        std::uint64_t hash = 1469598103934665603ull;
        for (const uint8_t b : data) {
            hash ^= static_cast<std::uint64_t>(b);
            hash *= 1099511628211ull;
        }
        return hash;
    }

    static std::string toHex(std::uint64_t value) {
        static constexpr std::array<char, 16> kHex = {
            '0', '1', '2', '3', '4', '5', '6', '7',
            '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
        };

        std::string out(16, '0');
        for (int i = 15; i >= 0; --i) {
            out[static_cast<std::size_t>(i)] = kHex[static_cast<std::size_t>(value & 0xF)];
            value >>= 4;
        }
        return out;
    }
};

} // namespace

std::shared_ptr<SigningService> createMockSigningService() {
    return std::make_shared<MockSigningService>();
}

} // namespace themis
