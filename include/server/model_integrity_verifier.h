/**
 * @file model_integrity_verifier.h
 * @brief Kryptografische SHA-256-Verifikation von LLM-Modelldateien vor dem Laden.
 *
 * Stellt sicher, dass nur Modelle geladen werden, deren SHA-256-Prüfsumme mit einem
 * signierten Manifest übereinstimmt. Verhindert das Laden manipulierter oder
 * unverifizierter Modelle.
 *
 * Manifest-Format:
 * @code{.json}
 * {
 *   "models": {
 *     "my-model-v1": {
 *       "sha256": "abcdef1234...",
 *       "path": "/models/my-model-v1.bin"
 *     }
 *   }
 * }
 * @endcode
 *
 * @note Thread-safe: std::shared_mutex schützt Manifest-Zugriffe.
 */

#pragma once

#include <optional>
#include <shared_mutex>
#include <string>
#include <unordered_map>

#include <nlohmann/json.hpp>

namespace themis {
namespace server {

class ModelIntegrityVerifier {
public:
    /**
     * @brief Verify Model.
     * @param[in] path Input parameter.
     * @param[in] expected_sha256 Input parameter.
     * @return True when the operation succeeds.
     */
    static bool verifyModel(const std::string& path, const std::string& expected_sha256);

    /**
     * @brief Compute Sha256.
     * @param[in] path Input parameter.
     * @return Return value.
     */
    static std::string computeSha256(const std::string& path);

    /**
     * @brief Load Manifest.
     * @param[in] manifest_path Path to the manifest.
     * @return True when the operation succeeds.
     */
    static bool loadManifest(const std::string& manifest_path);

    /**
     * @brief Get Expected Hash.
     * @param[in] model_id Identifier of the model.
     * @return Return value.
     */
    static std::optional<std::string> getExpectedHash(const std::string& model_id);

    /**
     * @brief Clear Manifest.
     */
    static void clearManifest();

private:
    static std::shared_mutex manifest_mutex_;

    static std::unordered_map<std::string, std::string> manifest_hashes_;
};

}  // namespace server
}  // namespace themis
