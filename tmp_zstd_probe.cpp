#include <iostream>
#include <vector>
#include "utils/zstd_codec.h"

int main() {
    std::vector<uint8_t> v(1024 * 1024);
    const char pattern[] = "ThemisDB GPU-Accelerated Compression test data pattern. ";
    const size_t pat_len = sizeof(pattern) - 1;
    for (size_t i = 0; i < v.size(); ++i) {
        v[i] = static_cast<uint8_t>(pattern[i % pat_len]);
    }
    auto c = themis::utils::zstd_compress(v.data(), v.size(), 3);
    std::cout << "orig=" << v.size() << " comp=" << c.size() << " ratio=" << (float)v.size() / (float)(c.empty() ? 1 : c.size()) << "\n";
    return 0;
}
