#include <iostream>
#include <vector>
#include <string>
#include <cstdint>
#include "utils/zstd_codec.h"
int main(){
    std::string pattern = "ThemisDB GPU-Accelerated Compression test data pattern. ";
    std::vector<uint8_t> v;
    for (int i=0;i<20000;i++) { v.insert(v.end(), pattern.begin(), pattern.end()); }
    auto c = themis::utils::zstd_compress(v, 3);
    std::cout << "orig=" << v.size() << " comp=" << c.size() << " ratio=" << (double)v.size()/c.size() << "\n";
    auto d = themis::utils::zstd_decompress(c);
    std::cout << "roundtrip=" << d.size() << " eq=" << (d==v) << "\n";
    return 0;
}
