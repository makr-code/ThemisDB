#include <iostream>
#include <vector>
#include <string>
#include <cstdint>
#include "utils/zstd_codec.h"

int main() {
    std::string pattern = "ThemisDB GPU-Accelerated Compression test data pattern. ";
    std::vector<uint8_t> v;
    for (int i = 0; i < 20000; ++i) {
        v.insert(v.end(), pattern.begin(), pattern.end());
    }

    auto c = themis::utils::zstd_compress(v, 3);
    auto d = themis::utils::zstd_decompress(c);
    std::cout << "orig=" << v.size() << " comp=" << c.size() << " ratio=" << (double)v.size() / (c.empty() ? 1.0 : c.size()) << "\n";
    std::cout << "roundtrip=" << d.size() << " eq=" << (d == v) << "\n";
    return 0;
}
