/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            export_encryption.h                                ║
  Version:         0.0.13                                             ║
  Last Modified:   2026-04-15 07:06:36                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     110                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "security/encryption.h"
#include "security/key_provider.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <istream>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

namespace themis::exporters {

struct ExportEncryptionConfig {
    bool enabled = false;
    std::string kek_id;
    std::string job_id;
    std::shared_ptr<themis::KeyProvider> key_provider;

    bool empty() const { return kek_id.empty() || !key_provider; }
};

class ExportEncryption {
public:
    explicit ExportEncryption(const ExportEncryptionConfig& config);

    void encryptFile(const std::string& src_path,
                     const std::string& dst_path) const;

    void decryptFile(const std::string& src_path,
                     const std::string& dst_path) const;

    std::vector<uint8_t> encrypt(const std::vector<uint8_t>& plaintext) const;
    std::vector<uint8_t> decrypt(const std::vector<uint8_t>& container) const;

private:
    ExportEncryptionConfig config_;

    std::vector<uint8_t> deriveJobDEK(uint32_t key_version) const;

    static std::vector<uint8_t> buildAAD(const std::string& job_id,
                                         const std::string& kek_id,
                                         uint32_t key_version,
                                         const std::vector<uint8_t>& iv);
};

class ExportEncryptor {
public:
    static constexpr uint8_t kFormatVersion = 1;
    static constexpr char kMagic[4] = {'T', 'M', 'E', 'X'};
    static constexpr size_t kChunkSize = 65536;

    explicit ExportEncryptor(const ExportEncryptionConfig& config);

    size_t encryptFile(const std::string& input_path,
                       const std::string& output_path) const;

    size_t decryptFile(const std::string& input_path,
                       const std::string& output_path) const;

    const ExportEncryptionConfig& getConfig() const { return config_; }

private:
    ExportEncryptionConfig config_;

    static std::vector<uint8_t> deriveDataKey(const std::vector<uint8_t>& kek,
                                              const std::string& job_id);

    static std::string generateJobId();

    static size_t writeHeader(std::ostream& out,
                              const std::string& kek_id,
                              uint32_t kek_version,
                              const std::string& job_id,
                              const std::vector<uint8_t>& iv);

    static bool readHeader(std::istream& in,
                           std::string& kek_id,
                           uint32_t& kek_version,
                           std::string& job_id,
                           std::vector<uint8_t>& iv);
};

} // namespace themis::exporters
