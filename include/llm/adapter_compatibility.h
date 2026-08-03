/**
 * @file adapter_compatibility.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
*
 * @note **Configuration/Metadata**: Defines configuration and tracking structures.
 *       No .cpp implementation needed. Used by consumers for configuration.
 */

/*
 * ThemisDB | File: adapter_compatibility.h | Version: 0.0.47 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 236
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): #114 Add complete PEFT training ... (2026-03-11)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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
        
        std::string toFormattedString() const;
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
    
    /// Quick check: Can adapter be used with base model?
    /// @param adapter Adapter metadata
    /// @param base_model_name Base model name
    /// @return true if compatible (fast check)
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
    
    // Individual check implementations
    CompatibilityCheck checkModelNameMatch(
        const AdapterMetadata& adapter,
        const std::string& base_model_name
    );
    
    CompatibilityCheck checkArchitectureMatch(
        const AdapterMetadata& adapter,
        const std::string& base_model_name
    );
    
    CompatibilityCheck checkDimensionCompatibility(
        const AdapterMetadata& adapter,
        const std::string& base_model_name
    );
    
    CompatibilityCheck checkTokenizerCompatibility(
        const AdapterMetadata& adapter,
        const std::string& base_model_name
    );
    
    CompatibilityCheck checkVersionCompatibility(
        const AdapterMetadata& adapter,
        const std::string& base_model_version
    );
    
    CompatibilityCheck checkQuantizationCompatibility(
        const AdapterMetadata& adapter,
        const std::string& base_model_name
    );
    
    // Helper: Initialize known model database
    void initializeKnownModels();
    
    // Helper: Normalize model name (handle variations)
    std::string normalizeModelName(const std::string& model_name) const;
    
    // Helper: Parse version string
    struct VersionParts {
        std::optional<int> major;
        std::optional<int> minor;
        std::optional<int> patch;
        std::string variant;  // e.g., "instruct", "chat"
    };
    
    VersionParts parseVersion(const std::string& version_str) const;
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
        
        nlohmann::json toJson() const;
    };
    
    /// Analyze migration from source to target model
    /// @param adapter Current adapter metadata
    /// @param source_model Current base model
    /// @param target_model Target base model
    /// @return Migration plan
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

