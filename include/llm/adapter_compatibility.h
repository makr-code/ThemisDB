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

/// Adapter Compatibility Validator
/// Validates LoRA adapter compatibility with base models
class AdapterCompatibilityValidator {
public:
    /**
     * @brief TBD: Describe ~AdapterCompatibilityValidator.
     * @return Return value.
     */
    virtual ~AdapterCompatibilityValidator() = default;
    /// Validation level
    enum class ValidationLevel {
        STRICT,      // All checks must pass
        MODERATE,    // Critical checks only
        PERMISSIVE   // Warnings only, no errors
    };
    
    /// Compatibility check result
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
         * @brief TBD: Describe toString.
         * @return Return value.
         */
        std::string toString() const;
    };
    
    /// Full validation result
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
         * @brief TBD: Describe toFormattedString.
         * @return Return value.
         */
        std::string toFormattedString() const;
        /**
         * @brief TBD: Describe toJson.
         * @return Return value.
         */
        nlohmann::json toJson() const;
    };
    
    explicit AdapterCompatibilityValidator(
        ValidationLevel level = ValidationLevel::STRICT
    );
    
    // Validation Operations
    
    /// Validate adapter against base model
    /// @param adapter Adapter metadata
    /// @param base_model_name Base model name (e.g., "mistral-7b")
    /// @param base_model_version Optional version string
    /// @return Validation result with detailed checks
    ValidationResult validate(
        const AdapterMetadata& adapter,
        const std::string& base_model_name,
        const std::string& base_model_version = ""
    );
    
    /**
     * @brief Quick check: Can adapter be used with base model?
     * @param[in] adapter Input parameter.
     * @param[in] base_model_name Input parameter.
     * @return True on success.
     * @details @param adapter Adapter metadata @param base_model_name Base model name @return true if compatible (fast check)
     */
    bool isCompatible(
        const AdapterMetadata& adapter,
        const std::string& base_model_name
    );
    
    /// Validate adapter against another adapter (for stacking/merging)
    ValidationResult validateAdapterPair(
        const AdapterMetadata& adapter1,
        const AdapterMetadata& adapter2
    );
    
    // Model Information
    
    /// Known model architectures and their specifications
    struct ModelSpec {
        std::string architecture;  // e.g., "llama", "mistral"
        int hidden_size = 0;
        int ffn_dimension = 0;
        std::vector<std::string> supported_versions;
        std::string tokenizer_type;
        
        /**
         * @brief TBD: Describe toJson.
         * @return Return value.
         */
        nlohmann::json toJson() const;
    };
    
    /// Get known model specification
    std::optional<ModelSpec> getModelSpec(const std::string& model_name);
    
    /// Register custom model specification
    void registerModelSpec(const std::string& model_name, const ModelSpec& spec);
    
    // Configuration
    
    /// Set validation level
    void setValidationLevel(ValidationLevel level) { validation_level_ = level; }
    
    /// Get validation level
    ValidationLevel getValidationLevel() const { return validation_level_; }
    
    /// Enable/disable specific checks
    void enableCheck(CompatibilityCheck::CheckType type, bool enabled = true);
    
    /// Check if specific check is enabled
    bool isCheckEnabled(CompatibilityCheck::CheckType type) const;
    
private:
    ValidationLevel validation_level_;
    std::map<CompatibilityCheck::CheckType, bool> enabled_checks_;
    std::map<std::string, ModelSpec> known_models_;
    
    /**
     * @brief Individual check implementations
     * @param[in] adapter Input parameter.
     * @param[in] base_model_name Input parameter.
     * @return Return value.
     */
    CompatibilityCheck checkModelNameMatch(
        const AdapterMetadata& adapter,
        const std::string& base_model_name
    );
    
    /**
     * @brief TBD: Describe checkArchitectureMatch.
     * @param[in] adapter Input parameter.
     * @param[in] base_model_name Input parameter.
     * @return Return value.
     */
    CompatibilityCheck checkArchitectureMatch(
        const AdapterMetadata& adapter,
        const std::string& base_model_name
    );
    
    /**
     * @brief TBD: Describe checkDimensionCompatibility.
     * @param[in] adapter Input parameter.
     * @param[in] base_model_name Input parameter.
     * @return Return value.
     */
    CompatibilityCheck checkDimensionCompatibility(
        const AdapterMetadata& adapter,
        const std::string& base_model_name
    );
    
    /**
     * @brief TBD: Describe checkTokenizerCompatibility.
     * @param[in] adapter Input parameter.
     * @param[in] base_model_name Input parameter.
     * @return Return value.
     */
    CompatibilityCheck checkTokenizerCompatibility(
        const AdapterMetadata& adapter,
        const std::string& base_model_name
    );
    
    /**
     * @brief TBD: Describe checkVersionCompatibility.
     * @param[in] adapter Input parameter.
     * @param[in] base_model_version Input parameter.
     * @return Return value.
     */
    CompatibilityCheck checkVersionCompatibility(
        const AdapterMetadata& adapter,
        const std::string& base_model_version
    );
    
    /**
     * @brief TBD: Describe checkQuantizationCompatibility.
     * @param[in] adapter Input parameter.
     * @param[in] base_model_name Input parameter.
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
     * @param[in] model_name Input parameter.
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
     * @brief TBD: Describe parseVersion.
     * @param[in] version_str Input parameter.
     * @return Return value.
     */
    VersionParts parseVersion(const std::string& version_str) const;
    /**
     * @brief TBD: Describe areVersionsCompatible.
     * @param[in] v1 Input parameter.
     * @param[in] v2 Input parameter.
     * @return True on success.
     */
    bool areVersionsCompatible(const VersionParts& v1, const VersionParts& v2) const;
};

/// Migration Assistant - Helps migrate adapters between model versions
class ModelMigrationAssistant {
public:
    /// Migration strategy
    enum class MigrationStrategy {
        RETRAIN,           // Full retraining required
        FINE_TUNE,         // Fine-tune existing adapter
        DIMENSION_ADAPT,   // Adapt dimensions (if possible)
        NO_MIGRATION       // Not possible to migrate
    };
    
    /// Migration plan
    struct MigrationPlan {
        MigrationStrategy strategy;
        bool feasible = false;
        std::vector<std::string> steps;
        std::vector<std::string> warnings;
        double estimated_effort = 0.0;  // 0.0-1.0 (training time ratio)
        std::string recommendation;
        
        /**
         * @brief TBD: Describe toJson.
         * @return Return value.
         */
        nlohmann::json toJson() const;
    };
    
    /**
     * @brief Analyze migration from source to target model @param adapter Current adapter metadata @param source_model Current base model @param target_model Target base model @return Migration plan
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
    
    /// Get recommended migration strategy
    static MigrationStrategy recommendStrategy(
        const std::string& source_model,
        const std::string& target_model
    );
    
    /// Estimate retraining effort (as fraction of original training)
    static double estimateRetrainingEffort(
        const std::string& source_model,
        const std::string& target_model
    );
};

} // namespace llm
} // namespace themis

