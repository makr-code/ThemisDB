/**
 * @file config_schema_validator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "config/config_errors.h"
#include <string>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>

namespace themis {
namespace config {

/**
 * ConfigSchemaValidator validates YAML and JSON config files against
 * JSON Schema (Draft 7 subset) definitions.
 *
 * Supported JSON Schema keywords:
 *   - type, properties, required, additionalProperties
 *   - minLength, maxLength, pattern, format (string)
 *   - minimum, maximum, exclusiveMinimum, exclusiveMaximum (number/integer)
 *   - minItems, maxItems, items, uniqueItems (array)
 *   - enum, const
 *   - allOf, anyOf, oneOf, not (schema composition)
 *   - $ref with local $defs / definitions lookup (JSON Pointer, RFC 6901 subset)
 *
 * YAML files are loaded via yaml-cpp and converted to an internal JSON
 * representation before validation.  JSON files are parsed directly with
 * nlohmann::json.
 *
 * Integration with ConfigPathResolver:
 *   Schema files can be located using ConfigPathResolver::tryResolve() so
 *   that the legacy-to-new path mapping applies to schema paths as well as
 *   config data files.
 *
 * Thread Safety:
 *   All public methods are stateless static functions and are thread-safe.
 */
class ConfigSchemaValidator {
public:
    /**
     * Result of a schema validation operation.
     */
    struct ValidationResult {
        bool valid = true;
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
        std::string config_path;
        std::string schema_path;

        void addError(const std::string& error) {
            valid = false;
            errors.push_back(error);
        }

        void addWarning(const std::string& warning) {
            warnings.push_back(warning);
        }

        std::string formatErrors() const {
            std::string result;
            for (const auto& e : errors) {
                result += "ERROR: " + e + "\n";
            }
            for (const auto& w : warnings) {
                result += "WARNING: " + w + "\n";
            }
            return result;
        }
    };

    /**
     * Validate a YAML or JSON config file against an inline JSON Schema object.
     *
     * Load and parse errors are reported as `ValidationResult` errors rather
     * than thrown.
     *
     * @param config_path  Path to the YAML or JSON config file.
     * @param schema       JSON Schema as a nlohmann::json object.
     * @return ValidationResult describing any schema violations.
     */
    static ValidationResult validate(const std::string& config_path,
                                     const nlohmann::json& schema);

    /**
     * Validate a YAML or JSON config file against a JSON Schema file.
     *
     * ConfigPathResolver::tryResolve() is used to find the schema_path so that
     * the legacy-to-new path mapping is applied automatically.
     *
     * Load and parse errors are reported as `ValidationResult` errors rather
     * than thrown.
     *
     * @param config_path  Path to the YAML or JSON config file.
     * @param schema_path  Path to the JSON Schema file (legacy or new path).
     * @return ValidationResult describing any schema violations.
     */
    static ValidationResult validateWithSchemaFile(const std::string& config_path,
                                                   const std::string& schema_path);

    /**
     * Load a YAML or JSON file and return its content as a nlohmann::json value.
     *
     * Detects the file format from the extension (.yaml/.yml → yaml-cpp,
     * everything else → nlohmann::json).
     *
     * @param file_path  Path to the file to load.
     * @return Parsed JSON value.
     * @throws SchemaValidationException on read or parse errors.
     */
    static nlohmann::json loadAsJson(const std::string& file_path);

    /**
     * Parse an in-memory string as YAML or JSON and return the result as a
     * nlohmann::json value.
     *
     * This overload enables in-memory schema validation without requiring
     * the content to be written to a file first.
     *
     * @param content  The raw YAML or JSON string to parse.
     * @param is_yaml  When true the content is parsed with yaml-cpp;
     *                 when false it is parsed as JSON with nlohmann::json.
     * @return Parsed JSON value.
     * @throws SchemaValidationException on parse errors.
     */
    static nlohmann::json loadAsJson(const std::string& content, bool is_yaml);

    /**
     * Validate an in-memory YAML or JSON string against an inline JSON Schema
     * object.
     *
     * This overload eliminates the need to write a config string to disk before
     * validating it, supporting dynamic config editing and runtime adaptation.
     *
     * Parse errors are reported as `ValidationResult` errors rather than thrown.
     *
     * @param content  The raw YAML or JSON config string to validate.
     * @param is_yaml  When true the content is parsed with yaml-cpp;
     *                 when false it is parsed as JSON with nlohmann::json.
     * @param schema   JSON Schema as a nlohmann::json object.
     * @return ValidationResult describing any schema violations.
     */
    static ValidationResult validateFromString(const std::string& content,
                                               bool is_yaml,
                                               const nlohmann::json& schema);

private:
    // Entry-point wrapper: uses schema itself as the root schema and an empty
    // visited-refs set.  Called by validate() and validateWithSchemaFile().
    static void validateValue(const nlohmann::json& value,
                              const nlohmann::json& schema,
                              const std::string& json_path,
                              ValidationResult& result);

    // Internal recursive implementation.
    // root_schema — top-level schema object used for $ref/$defs resolution.
    // visited_refs — current $ref resolution chain for cycle detection.
    static void validateValueImpl(const nlohmann::json& value,
                                  const nlohmann::json& schema,
                                  const std::string& json_path,
                                  ValidationResult& result,
                                  const nlohmann::json& root_schema,
                                  std::vector<std::string>& visited_refs);

    // Resolve a local $ref string (e.g. "#/$defs/Foo" or "#/definitions/Bar")
    // against root_schema using a JSON Pointer walk (RFC 6901).
    // Returns a pointer into root_schema, or nullptr on failure.
    // Only document-internal refs starting with '#' are supported.
    static const nlohmann::json* resolveRef(const std::string& ref,
                                            const nlohmann::json& root_schema);

    static void validateType(const nlohmann::json& value,
                             const std::string& expected_type,
                             const std::string& json_path,
                             ValidationResult& result);

    static void validateObject(const nlohmann::json& value,
                               const nlohmann::json& schema,
                               const std::string& json_path,
                               ValidationResult& result,
                               const nlohmann::json& root_schema,
                               std::vector<std::string>& visited_refs);

    static void validateArray(const nlohmann::json& value,
                              const nlohmann::json& schema,
                              const std::string& json_path,
                              ValidationResult& result,
                              const nlohmann::json& root_schema,
                              std::vector<std::string>& visited_refs);

    static void validateString(const nlohmann::json& value,
                               const nlohmann::json& schema,
                               const std::string& json_path,
                               ValidationResult& result);

    static void validateNumber(const nlohmann::json& value,
                               const nlohmann::json& schema,
                               const std::string& json_path,
                               ValidationResult& result);

    static void validateAllOf(const nlohmann::json& value,
                              const nlohmann::json& schemas,
                              const std::string& json_path,
                              ValidationResult& result,
                              const nlohmann::json& root_schema,
                              std::vector<std::string>& visited_refs);

    static void validateAnyOf(const nlohmann::json& value,
                              const nlohmann::json& schemas,
                              const std::string& json_path,
                              ValidationResult& result,
                              const nlohmann::json& root_schema,
                              std::vector<std::string>& visited_refs);

    static void validateOneOf(const nlohmann::json& value,
                              const nlohmann::json& schemas,
                              const std::string& json_path,
                              ValidationResult& result,
                              const nlohmann::json& root_schema,
                              std::vector<std::string>& visited_refs);

    static void validateNot(const nlohmann::json& value,
                            const nlohmann::json& not_schema,
                            const std::string& json_path,
                            ValidationResult& result,
                            const nlohmann::json& root_schema,
                            std::vector<std::string>& visited_refs);

    // Check whether a JSON value matches the given JSON Schema type string.
    static bool matchesType(const nlohmann::json& value, const std::string& type);
};

} // namespace config
} // namespace themis
