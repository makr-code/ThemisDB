/**
 * @file adapter_compatibility.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
*
 * @note **Configuration/Metadata**: Defines configuration and tracking structures.
 *       No .cpp implementation needed. Used by consumers for configuration.
 */


#pragma once

#include "llm/adapter_registry.h"
#include <string>
#include <vector>
#include <optional>

namespace themis {
namespace llm {

class AdapterCompatibilityValidator {
public:
    /**
     * @brief Adapter Compatibility Validator.
     * @return Return value.
     */
    virtual ~AdapterCompatibilityValidator() = default;
    enum class ValidationLevel {
        STRICT,      // All checks must pass
        MODERATE,    // Critical checks only
        PERMISSIVE   // Warnings only, no errors
    };
    
    struct CompatibilityCheck {
        enum class CheckType {
            MODEL_NAME_MATCH,
            ARCHITECTURE_MATCH,
            DIMENSION_COMPATIBILITY,
            TOKENIZER_COMPATIBILITY,
            VERSION_COMPATIBILITY,
            QUANTIZATION_COMPATIBILITY
        };
        
        CheckType type;
        bool passed = false;
        std::string message;
        bool is_critical = true;  // If true, failure blocks deployment
        
        /**
         * @brief To String.
         * @return Return value.
         */
        std::string toString() const;
    };
    
    struct ValidationResult {
        bool compatible = false;
        std::vector<CompatibilityCheck> checks;
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
        std::vector<std::string> suggestions;
        
        // Summary
        size_t total_checks = 0;
        size_t passed_checks = 0;
        size_t failed_critical_checks = 0;
        
        /**
         * @brief To Formatted String.
         * @return Return value.
         */
        std::string toFormattedString() const;
        /**
         * @brief To Json.
         * @return Return value.
         */
        nlohmann::json toJson() const;
    };
    
    explicit AdapterCompatibilityValidator(
        ValidationLevel level = ValidationLevel::STRICT
    );
    
    // Validation Operations
    
    ValidationResult validate(
        const AdapterMetadata& adapter,
        const std::string& base_model_name,
        const std::string& base_model_version = ""
    );
    
    /**
     * @brief Is Compatible.
     * @param[in] adapter Input parameter.
     * @param[in] base_model_name Name of the base model.
     * @return True when the operation succeeds.
     */
    bool isCompatible(
        const AdapterMetadata& adapter,
        const std::string& base_model_name
    );
    
    /**
     * @brief Validate Adapter Pair.
     * @param[in] adapter1 Input parameter.
     * @param[in] adapter2 Input parameter.
     * @return Return value.
     */
    ValidationResult validateAdapterPair(
        const AdapterMetadata& adapter1,
        const AdapterMetadata& adapter2
    );
    
    // Model Information
    
    struct ModelSpec {
        std::string architecture;  // e.g., "llama", "mistral"
        int hidden_size = 0;
        int ffn_dimension = 0;
        std::vector<std::string> supported_versions;
        std::string tokenizer_type;
        
        /**
         * @brief To Json.
         * @return Return value.
         */
        nlohmann::json toJson() const;
    };
    
    /**
     * @brief Get Model Spec.
     * @param[in] model_name Name of the model.
     * @return Return value.
     */
    std::optional<ModelSpec> getModelSpec(const std::string& model_name);
    
    /**
     * @brief Register Model Spec.
     * @param[in] model_name Name of the model.
     * @param[in] spec Input parameter.
     */
    void registerModelSpec(const std::string& model_name, const ModelSpec& spec);
    
    // Configuration
    
    /**
     * @brief Set Validation Level.
     * @param[in] level Input parameter.
     * @details Implements setValidationLevel without additional internal calls.
     */
    void setValidationLevel(ValidationLevel level) { validation_level_ = level; }
    
    ValidationLevel getValidationLevel() const { return validation_level_; }
    
    void enableCheck(CompatibilityCheck::CheckType type, bool enabled = true);
    
    /**
     * @brief Is Check Enabled.
     * @param[in] type Input parameter.
     * @return True when the operation succeeds.
     */
    bool isCheckEnabled(CompatibilityCheck::CheckType type) const;
    
private:
    ValidationLevel validation_level_;
    std::map<CompatibilityCheck::CheckType, bool> enabled_checks_;
    std::map<std::string, ModelSpec> known_models_;
    
    // Individual check implementations
    /**
     * @brief Check Model Name Match.
     * @param[in] adapter Input parameter.
     * @param[in] base_model_name Name of the base model.
     * @return Return value.
     */
    CompatibilityCheck checkModelNameMatch(
        const AdapterMetadata& adapter,
        const std::string& base_model_name
    );
    
    /**
     * @brief Check Architecture Match.
     * @param[in] adapter Input parameter.
     * @param[in] base_model_name Name of the base model.
     * @return Return value.
     */
    CompatibilityCheck checkArchitectureMatch(
        const AdapterMetadata& adapter,
        const std::string& base_model_name
    );
    
    /**
     * @brief Check Dimension Compatibility.
     * @param[in] adapter Input parameter.
     * @param[in] base_model_name Name of the base model.
     * @return Return value.
     */
    CompatibilityCheck checkDimensionCompatibility(
        const AdapterMetadata& adapter,
        const std::string& base_model_name
    );
    
    /**
     * @brief Check Tokenizer Compatibility.
     * @param[in] adapter Input parameter.
     * @param[in] base_model_name Name of the base model.
     * @return Return value.
     */
    CompatibilityCheck checkTokenizerCompatibility(
        const AdapterMetadata& adapter,
        const std::string& base_model_name
    );
    
    /**
     * @brief Check Version Compatibility.
     * @param[in] adapter Input parameter.
     * @param[in] base_model_version Input parameter.
     * @return Return value.
     */
    CompatibilityCheck checkVersionCompatibility(
        const AdapterMetadata& adapter,
        const std::string& base_model_version
    );
    
    /**
     * @brief Check Quantization Compatibility.
     * @param[in] adapter Input parameter.
     * @param[in] base_model_name Name of the base model.
     * @return Return value.
     */
    CompatibilityCheck checkQuantizationCompatibility(
        const AdapterMetadata& adapter,
        const std::string& base_model_name
    );
    
    /**
     * @brief Helper: Initialize known model database
     */
    void initializeKnownModels();
    
    /**
     * @brief Helper: Normalize model name (handle variations)
     * @param[in] model_name Name of the model.
     * @return Return value.
     */
    std::string normalizeModelName(const std::string& model_name) const;
    
    // Helper: Parse version string
    struct VersionParts {
        std::optional<int> major;
        std::optional<int> minor;
        std::optional<int> patch;
        std::string variant;  // e.g., "instruct", "chat"
    };
    
    /**
     * @brief Parse Version.
     * @param[in] version_str Input parameter.
     * @return Return value.
     */
    VersionParts parseVersion(const std::string& version_str) const;
    /**
     * @brief Are Versions Compatible.
     * @param[in] v1 Input parameter.
     * @param[in] v2 Input parameter.
     * @return True when the operation succeeds.
     */
    bool areVersionsCompatible(const VersionParts& v1, const VersionParts& v2) const;
};

class ModelMigrationAssistant {
public:
    enum class MigrationStrategy {
        RETRAIN,           // Full retraining required
        FINE_TUNE,         // Fine-tune existing adapter
        DIMENSION_ADAPT,   // Adapt dimensions (if possible)
        NO_MIGRATION       // Not possible to migrate
    };
    
    struct MigrationPlan {
        MigrationStrategy strategy;
        bool feasible = false;
        std::vector<std::string> steps;
        std::vector<std::string> warnings;
        double estimated_effort = 0.0;  // 0.0-1.0 (training time ratio)
        std::string recommendation;
        
        /**
         * @brief To Json.
         * @return Return value.
         */
        nlohmann::json toJson() const;
    };
    
    /**
     * @brief Analyze Migration.
     * @param[in] adapter Input parameter.
     * @param[in] source_model Input parameter.
     * @param[in] target_model Input parameter.
     * @return Return value.
     */
    static MigrationPlan analyzeMigration(
        const AdapterMetadata& adapter,
        const std::string& source_model,
        const std::string& target_model
    );
    
    /**
     * @brief Recommend Strategy.
     * @param[in] source_model Input parameter.
     * @param[in] target_model Input parameter.
     * @return Return value.
     */
    static MigrationStrategy recommendStrategy(
        const std::string& source_model,
        const std::string& target_model
    );
    
    /**
     * @brief Estimate Retraining Effort.
     * @param[in] source_model Input parameter.
     * @param[in] target_model Input parameter.
     * @return Return value.
     */
    static double estimateRetrainingEffort(
        const std::string& source_model,
        const std::string& target_model
    );
};

} // namespace llm
} // namespace themis

