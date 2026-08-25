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

/**
 * @brief Verifiziert die kryptografische Integrität von LLM-Modelldateien.
 *
 * Alle öffentlichen Methoden sind thread-safe.
 */
class ModelIntegrityVerifier {
public:
    /**
     * @brief Berechnet den SHA-256-Hash einer Datei und vergleicht ihn mit dem
     *        erwarteten Hash-Wert.
     *
     * @param path           Dateisystempfad zur Modelldatei.
     * @param expected_sha256 Erwarteter Hex-String des SHA-256-Hashes (Kleinbuchstaben).
     * @return @c true wenn Hash übereinstimmt, @c false bei Abweichung oder Fehler.
     */
    static bool verifyModel(const std::string& path, const std::string& expected_sha256);

    /**
     * @brief Berechnet den SHA-256-Hash einer Datei und gibt ihn als Hex-String zurück.
     *
     * @param path Dateisystempfad der zu hashenden Datei.
     * @return SHA-256-Hash als Lowercase-Hex-String, oder leerer String bei Fehler.
     */
    static std::string computeSha256(const std::string& path);

    /**
     * @brief Lädt ein JSON-Manifest mit Modell-Hashes in den internen Cache.
     *
     * Ersetzt alle zuvor geladenen Manifest-Einträge. Fehlende oder ungültige
     * Manifeste werden als "kein Manifest vorhanden" behandelt (graceful degradation).
     *
     * @param manifest_path Pfad zur JSON-Manifestdatei.
     * @return @c true wenn das Manifest erfolgreich geladen wurde.
     */
    static bool loadManifest(const std::string& manifest_path);

    /**
     * @brief Gibt den erwarteten SHA-256-Hash für eine Modell-ID zurück.
     *
     * @param model_id Eindeutiger Bezeichner des Modells.
     * @return SHA-256-Hash als Hex-String, oder @c std::nullopt wenn nicht im Manifest.
     */
    static std::optional<std::string> getExpectedHash(const std::string& model_id);

    /**
     * @brief Leert den internen Manifest-Cache (nützlich für Tests).
     */
    static void clearManifest();

private:
    /// Mutex für thread-sicheren Manifest-Zugriff (shared für Lesezugriffe).
    static std::shared_mutex manifest_mutex_;

    /// Interne Abbildung model_id → sha256.
    static std::unordered_map<std::string, std::string> manifest_hashes_;
};

}  // namespace server
}  // namespace themis
