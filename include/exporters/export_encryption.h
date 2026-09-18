/**
 * @file export_encryption.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "security/encryption.h"
#include "security/key_provider.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <istream>
#include <memory>
#include <mutex>
#include <ostream>
#include <string>
#include <vector>

namespace themis::exporters {

struct ExportEncryptionConfig {
    bool enabled = false;
    std::string kek_id = {};
    std::string job_id;
    std::shared_ptr<themis::KeyProvider> key_provider;

    bool empty() const { return kek_id.empty() || !key_provider; }
};

class ExportEncryption {
public:
    /**
     * @brief Export Encryption.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit ExportEncryption(const ExportEncryptionConfig& config);

    /**
     * @brief Encrypt File.
     * @param[in] src_path Path to the src.
     * @param[in] dst_path Path to the dst.
     */
    void encryptFile(const std::string& src_path,
                     const std::string& dst_path) const;

    /**
     * @brief Decrypt File.
     * @param[in] src_path Path to the src.
     * @param[in] dst_path Path to the dst.
     */
    void decryptFile(const std::string& src_path,
                     const std::string& dst_path) const;

    /**
     * @brief Encrypt.
     * @param[in] plaintext Input parameter.
     * @return Return value.
     */
    std::vector<uint8_t> encrypt(const std::vector<uint8_t>& plaintext) const;
    /**
     * @brief Decrypt.
     * @param[in] container Input parameter.
     * @return Return value.
     */
    std::vector<uint8_t> decrypt(const std::vector<uint8_t>& container) const;

private:
    ExportEncryptionConfig config_;

    mutable std::mutex key_provider_mutex_;

    /**
     * @brief Derive Job DEK.
     * @param[in] key_version Input parameter.
     * @return Return value.
     */
    std::vector<uint8_t> deriveJobDEK(uint32_t key_version) const;

    /**
     * @brief Build AAD.
     * @param[in] job_id Identifier of the job.
     * @param[in] kek_id Identifier of the kek.
     * @param[in] key_version Input parameter.
     * @param[in] iv Input parameter.
     * @return Return value.
     */
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

    /**
     * @brief Export Encryptor.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit ExportEncryptor(const ExportEncryptionConfig& config);

    /**
     * @brief Encrypt File.
     * @param[in] input_path Path to the input.
     * @param[in] output_path Path to the output.
     * @return Return value.
     */
    size_t encryptFile(const std::string& input_path,
                       const std::string& output_path) const;

    /**
     * @brief Decrypt File.
     * @param[in] input_path Path to the input.
     * @param[in] output_path Path to the output.
     * @return Return value.
     */
    size_t decryptFile(const std::string& input_path,
                       const std::string& output_path) const;

    const ExportEncryptionConfig& getConfig() const { return config_; }

private:
    ExportEncryptionConfig config_;

    mutable std::mutex key_provider_mutex_;

    /**
     * @brief Derive Data Key.
     * @param[in] kek Input parameter.
     * @param[in] job_id Identifier of the job.
     * @return Return value.
     */
    static std::vector<uint8_t> deriveDataKey(const std::vector<uint8_t>& kek,
                                              const std::string& job_id);

    /**
     * @brief Generate Job Id.
     * @return Return value.
     */
    static std::string generateJobId();

    /**
     * @brief Write Header.
     * @param[in,out] out Input/output parameter.
     * @param[in] kek_id Identifier of the kek.
     * @param[in] kek_version Input parameter.
     * @param[in] job_id Identifier of the job.
     * @param[in] iv Input parameter.
     * @return Return value.
     */
    static size_t writeHeader(std::ostream& out,
                              const std::string& kek_id,
                              uint32_t kek_version,
                              const std::string& job_id,
                              const std::vector<uint8_t>& iv);

    /**
     * @brief Read Header.
     * @param[in,out] in Input/output parameter.
     * @param[in,out] kek_id Identifier of the kek.
     * @param[in,out] kek_version Input/output parameter.
     * @param[in,out] job_id Identifier of the job.
     * @param[in,out] iv Input/output parameter.
     * @return True when the operation succeeds.
     */
    static bool readHeader(std::istream& in,
                           std::string& kek_id,
                           uint32_t& kek_version,
                           std::string& job_id,
                           std::vector<uint8_t>& iv);
};

} // namespace themis::exporters
