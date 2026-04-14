/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            checksum_utils.cpp                                 ║
  Version:         0.0.9                                              ║
  Last Modified:   2026-04-14 11:39:17                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     93                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 57bf541b22  2026-02-24  chore(core): code audit — fix stale annotations and expli... ║
    • ce91302f75  2026-02-24  feat: erweitere die ModularBuild-Konfiguration und implem... ║
    • 31c83c7016  2026-02-23  fix(core): repair syntax errors from develop merge; resto... ║
    • 454802e88a  2026-02-23  fix(core): fix syntax errors in core headers and improve ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "utils/checksum_utils.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <openssl/sha.h>
#include <openssl/md5.h>

namespace themis {
namespace utils {

std::string calculateSHA256(const std::string& file_path) {
    std::ifstream file(file_path, std::ios::binary);
    if (!file.is_open()) {
        return "";
    }

    SHA256_CTX sha256;
    SHA256_Init(&sha256);

    constexpr size_t buffer_size = 32768;
    std::vector<char> buffer(buffer_size);

    while (file.read(buffer.data(), buffer_size) || file.gcount() > 0) {
        SHA256_Update(&sha256, buffer.data(), file.gcount());
    }

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_Final(hash, &sha256);

    std::ostringstream oss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }

    return oss.str();
}

std::string calculateMD5(const std::string& file_path) {
    std::ifstream file(file_path, std::ios::binary);
    if (!file.is_open()) {
        return "";
    }

    MD5_CTX md5;
    MD5_Init(&md5);

    constexpr size_t buffer_size = 32768;
    std::vector<char> buffer(buffer_size);

    while (file.read(buffer.data(), buffer_size) || file.gcount() > 0) {
        MD5_Update(&md5, buffer.data(), file.gcount());
    }

    unsigned char hash[MD5_DIGEST_LENGTH];
    MD5_Final(hash, &md5);

    std::ostringstream oss;
    for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }

    return oss.str();
}

} // namespace utils
} // namespace themis
