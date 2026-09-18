/**
 * @file config_schema_validator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

class ConfigSchemaValidator {
public:
    struct ValidationResult {
        bool valid = true;
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
        std::string config_path;
        std::string schema_path;

        /**
         * @brief Add Error.
         * @param[in] error Input parameter.
         * @details Calls: push_back().
         */
        void addError(const std::string& error) {
            valid = false;
            errors.push_back(error);
        }

        /**
         * @brief Add Warning.
         * @param[in] warning Input parameter.
         * @details Calls: push_back().
         */
        void addWarning(const std::string& warning) {
            warnings.push_back(warning);
        }

        std::string formatErrors() const {
            std::string result = {};
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
     * @brief Validate.
     * @param[in] config_path Path to the retention policy configuration file.
     * @param[in] schema Input parameter.
     * @return Return value.
     */
    static ValidationResult validate(const std::string& config_path,
                                     const nlohmann::json& schema);

    /**
     * @brief Validate With Schema File.
     * @param[in] config_path Path to the retention policy configuration file.
     * @param[in] schema_path Path to the schema.
     * @return Return value.
     */
    static ValidationResult validateWithSchemaFile(const std::string& config_path,
                                                   const std::string& schema_path);

    /**
     * @brief Load As Json.
     * @param[in] file_path Path to the file.
     * @return Return value.
     */
    static nlohmann::json loadAsJson(const std::string& file_path);

    /**
     * @brief Load As Json.
     * @param[in] content Input parameter.
     * @param[in] is_yaml Input parameter.
     * @return Return value.
     */
    static nlohmann::json loadAsJson(const std::string& content, bool is_yaml);

    /**
     * @brief Validate From String.
     * @param[in] content Input parameter.
     * @param[in] is_yaml Input parameter.
     * @param[in] schema Input parameter.
     * @return Return value.
     */
    static ValidationResult validateFromString(const std::string& content,
                                               bool is_yaml,
                                               const nlohmann::json& schema);

private:
    /**
     * @brief Entry-point wrapper: uses schema itself as the root schema and an empty visited-refs set.
     * @param[in] value Input parameter.
     * @param[in] schema Input parameter.
     * @param[in] json_path Path to the json.
     * @param[in,out] result Input/output parameter.
     * @details Called by validate() and validateWithSchemaFile().
     */
    static void validateValue(const nlohmann::json& value,
                              const nlohmann::json& schema,
                              const std::string& json_path,
                              ValidationResult& result);

    /**
     * @brief Internal recursive implementation.
     * @param[in] value Input parameter.
     * @param[in] schema Input parameter.
     * @param[in] json_path Path to the json.
     * @param[in,out] result Input/output parameter.
     * @param[in] root_schema Input parameter.
     * @param[in,out] visited_refs Input/output parameter.
     * @details root_schema — top-level schema object used for $ref/$defs resolution. visited_refs — current $ref resolution chain for cycle detection.
     */
    static void validateValueImpl(const nlohmann::json& value,
                                  const nlohmann::json& schema,
                                  const std::string& json_path,
                                  ValidationResult& result,
                                  const nlohmann::json& root_schema,
                                  std::vector<std::string>& visited_refs);

    /**
     * @brief Resolve a local $ref string (e.
     * @param[in] ref Input parameter.
     * @param[in] root_schema Input parameter.
     * @return Pointer to the result.
     * @details g. "#/$defs/Foo" or "#/definitions/Bar") against root_schema using a JSON Pointer walk (RFC 6901). Returns a pointer into root_schema, or nullptr on failure. Only document-internal refs starting with '#' are supported.
     */
    static const nlohmann::json* resolveRef(const std::string& ref,
                                            const nlohmann::json& root_schema);

    /**
     * @brief Validate Type.
     * @param[in] value Input parameter.
     * @param[in] expected_type Input parameter.
     * @param[in] json_path Path to the json.
     * @param[in,out] result Input/output parameter.
     */
    static void validateType(const nlohmann::json& value,
                             const std::string& expected_type,
                             const std::string& json_path,
                             ValidationResult& result);

    /**
     * @brief Validate Object.
     * @param[in] value Input parameter.
     * @param[in] schema Input parameter.
     * @param[in] json_path Path to the json.
     * @param[in,out] result Input/output parameter.
     * @param[in] root_schema Input parameter.
     * @param[in,out] visited_refs Input/output parameter.
     */
    static void validateObject(const nlohmann::json& value,
                               const nlohmann::json& schema,
                               const std::string& json_path,
                               ValidationResult& result,
                               const nlohmann::json& root_schema,
                               std::vector<std::string>& visited_refs);

    /**
     * @brief Validate Array.
     * @param[in] value Input parameter.
     * @param[in] schema Input parameter.
     * @param[in] json_path Path to the json.
     * @param[in,out] result Input/output parameter.
     * @param[in] root_schema Input parameter.
     * @param[in,out] visited_refs Input/output parameter.
     */
    static void validateArray(const nlohmann::json& value,
                              const nlohmann::json& schema,
                              const std::string& json_path,
                              ValidationResult& result,
                              const nlohmann::json& root_schema,
                              std::vector<std::string>& visited_refs);

    /**
     * @brief Validate String.
     * @param[in] value Input parameter.
     * @param[in] schema Input parameter.
     * @param[in] json_path Path to the json.
     * @param[in,out] result Input/output parameter.
     */
    static void validateString(const nlohmann::json& value,
                               const nlohmann::json& schema,
                               const std::string& json_path,
                               ValidationResult& result);

    /**
     * @brief Validate Number.
     * @param[in] value Input parameter.
     * @param[in] schema Input parameter.
     * @param[in] json_path Path to the json.
     * @param[in,out] result Input/output parameter.
     */
    static void validateNumber(const nlohmann::json& value,
                               const nlohmann::json& schema,
                               const std::string& json_path,
                               ValidationResult& result);

    /**
     * @brief Validate All Of.
     * @param[in] value Input parameter.
     * @param[in] schemas Input parameter.
     * @param[in] json_path Path to the json.
     * @param[in,out] result Input/output parameter.
     * @param[in] root_schema Input parameter.
     * @param[in,out] visited_refs Input/output parameter.
     */
    static void validateAllOf(const nlohmann::json& value,
                              const nlohmann::json& schemas,
                              const std::string& json_path,
                              ValidationResult& result,
                              const nlohmann::json& root_schema,
                              std::vector<std::string>& visited_refs);

    /**
     * @brief Validate Any Of.
     * @param[in] value Input parameter.
     * @param[in] schemas Input parameter.
     * @param[in] json_path Path to the json.
     * @param[in,out] result Input/output parameter.
     * @param[in] root_schema Input parameter.
     * @param[in,out] visited_refs Input/output parameter.
     */
    static void validateAnyOf(const nlohmann::json& value,
                              const nlohmann::json& schemas,
                              const std::string& json_path,
                              ValidationResult& result,
                              const nlohmann::json& root_schema,
                              std::vector<std::string>& visited_refs);

    /**
     * @brief Validate One Of.
     * @param[in] value Input parameter.
     * @param[in] schemas Input parameter.
     * @param[in] json_path Path to the json.
     * @param[in,out] result Input/output parameter.
     * @param[in] root_schema Input parameter.
     * @param[in,out] visited_refs Input/output parameter.
     */
    static void validateOneOf(const nlohmann::json& value,
                              const nlohmann::json& schemas,
                              const std::string& json_path,
                              ValidationResult& result,
                              const nlohmann::json& root_schema,
                              std::vector<std::string>& visited_refs);

    /**
     * @brief Validate Not.
     * @param[in] value Input parameter.
     * @param[in] not_schema Input parameter.
     * @param[in] json_path Path to the json.
     * @param[in,out] result Input/output parameter.
     * @param[in] root_schema Input parameter.
     * @param[in,out] visited_refs Input/output parameter.
     */
    static void validateNot(const nlohmann::json& value,
                            const nlohmann::json& not_schema,
                            const std::string& json_path,
                            ValidationResult& result,
                            const nlohmann::json& root_schema,
                            std::vector<std::string>& visited_refs);

    /**
     * @brief Check whether a JSON value matches the given JSON Schema type string.
     * @param[in] value Input parameter.
     * @param[in] type Input parameter.
     * @return True when the operation succeeds.
     */
    static bool matchesType(const nlohmann::json& value, const std::string& type);
};

} // namespace config
} // namespace themis
