/**
 * @file mime_detector.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <unordered_map>
#include <set>
#include <memory>
#include "content/content_policy.h"

namespace themis {
namespace storage {
    class SecuritySignatureManager;
}
namespace content {

class MimeDetector {
public:
    explicit MimeDetector(const std::string& config_path = "",
                         std::shared_ptr<storage::SecuritySignatureManager> sig_mgr = nullptr);
    
    bool reloadConfig(const std::string& config_path = "");

    /**
     * @brief From Extension.
     * @param[in] filename Input parameter.
     * @return Return value.
     */
    std::string fromExtension(std::string_view filename) const;

    /**
     * @brief From Content.
     * @param[in] data Input parameter.
     * @return Return value.
     */
    std::string fromContent(const std::vector<uint8_t>& data) const;

    /**
     * @brief Detect.
     * @param[in] filename Input parameter.
     * @param[in] data Input parameter.
     * @return Return value.
     */
    std::string detect(std::string_view filename, const std::vector<uint8_t>& data) const;

    /**
     * @brief Is Text.
     * @param[in] mime_type Input parameter.
     * @return True when the operation succeeds.
     */
    bool isText(std::string_view mime_type) const;
    /**
     * @brief Is Image.
     * @param[in] mime_type Input parameter.
     * @return True when the operation succeeds.
     */
    bool isImage(std::string_view mime_type) const;
    /**
     * @brief Is Video.
     * @param[in] mime_type Input parameter.
     * @return True when the operation succeeds.
     */
    bool isVideo(std::string_view mime_type) const;
    /**
     * @brief Is Audio.
     * @param[in] mime_type Input parameter.
     * @return True when the operation succeeds.
     */
    bool isAudio(std::string_view mime_type) const;
    /**
     * @brief Is Archive.
     * @param[in] mime_type Input parameter.
     * @return True when the operation succeeds.
     */
    bool isArchive(std::string_view mime_type) const;
    /**
     * @brief Is Document.
     * @param[in] mime_type Input parameter.
     * @return True when the operation succeeds.
     */
    bool isDocument(std::string_view mime_type) const;
    /**
     * @brief Is Geo.
     * @param[in] mime_type Input parameter.
     * @return True when the operation succeeds.
     */
    bool isGeo(std::string_view mime_type) const;
    /**
     * @brief Is Themis.
     * @param[in] mime_type Input parameter.
     * @return True when the operation succeeds.
     */
    bool isThemis(std::string_view mime_type) const;
    /**
     * @brief Is Executable.
     * @param[in] mime_type Input parameter.
     * @return True when the operation succeeds.
     */
    bool isExecutable(std::string_view mime_type) const;
    /**
     * @brief Is Database.
     * @param[in] mime_type Input parameter.
     * @return True when the operation succeeds.
     */
    bool isDatabase(std::string_view mime_type) const;
    /**
     * @brief Is Cad.
     * @param[in] mime_type Input parameter.
     * @return True when the operation succeeds.
     */
    bool isCad(std::string_view mime_type) const;
    /**
     * @brief Is Binary Data.
     * @param[in] mime_type Input parameter.
     * @return True when the operation succeeds.
     */
    bool isBinaryData(std::string_view mime_type) const;
    
    /**
     * @brief Get Category.
     * @param[in] category_name Name of the category.
     * @return Return value.
     */
    std::vector<std::string> getCategory(const std::string& category_name) const;
    
    bool isConfigVerified() const { return config_verified_; }
    
    /**
     * @brief Validate Upload.
     * @param[in] filename Input parameter.
     * @param[in] file_size Input parameter.
     * @return Return value.
     */
    ValidationResult validateUpload(const std::string& filename, uint64_t file_size) const;

    /**
     * @brief Should Trigger Ocr.
     * @param[in] mime_type Input parameter.
     * @return True when the operation succeeds.
     */
    bool shouldTriggerOcr(std::string_view mime_type) const;

    /**
     * @brief Should Trigger Ocr.
     * @param[in] mime_type Input parameter.
     * @param[in] ocr_enabled Input parameter.
     * @return True when the operation succeeds.
     * @note Exception safety: noexcept.
     */
    bool shouldTriggerOcr(std::string_view mime_type, bool ocr_enabled) const noexcept;

    void enableOcr(bool enable = true);

private:
    // Extension -> MIME type mapping
    std::unordered_map<std::string, std::string> ext_to_mime_;

    // Magic number signatures
    struct MagicSignature {
        std::vector<uint8_t> signature;
            std::set<size_t> wildcard_positions;  // Byte positions that can vary
        std::string mime_type;
        size_t offset = 0;  // Offset in file where signature appears
    };
    std::vector<MagicSignature> magic_signatures_;

    // Category -> MIME types mapping
    std::unordered_map<std::string, std::set<std::string>> categories_;
    
    // Content Policy (whitelist/blacklist, size limits)
    ContentPolicy policy_;
    
    // Configuration
    std::string config_path_;
    
    // Security signature manager (optional)
    std::shared_ptr<storage::SecuritySignatureManager> sig_mgr_;
    bool config_verified_ = false;
    
    /**
     * @brief Compute Deterministic Hash.
     * @return Return value.
     */
    std::string computeDeterministicHash() const;
    
    // Initialization from YAML
    /**
     * @brief Load Yaml Config.
     * @param[in] config_path Path to the retention policy configuration file.
     * @return True when the operation succeeds.
     */
    bool loadYamlConfig(const std::string& config_path);
    /**
     * @brief Get Default Config Path.
     * @return Return value.
     */
    std::string getDefaultConfigPath() const;
    /**
     * @brief Matches Magic Signature.
     * @param[in] content Input parameter.
     * @param[in] sig Input parameter.
     * @return True when the operation succeeds.
     */
    bool matchesMagicSignature(const std::vector<uint8_t>& content, 
                               const MagicSignature& sig) const;
    /**
     * @brief Extract Extension.
     * @param[in] filename Input parameter.
     * @return Return value.
     */
    std::string extractExtension(std::string_view filename) const;
    /**
     * @brief Is In Category.
     * @param[in] mime_type Input parameter.
     * @param[in] category Input parameter.
     * @return True when the operation succeeds.
     */
    bool isInCategory(std::string_view mime_type, const std::string& category) const;
};

} // namespace content
} // namespace themis
