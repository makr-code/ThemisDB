#pragma once

#include "governance/policy_manager.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <memory>
#include <functional>
#include <nlohmann/json.hpp>

namespace themis {
namespace governance {

/// Parameter definition for policy templates
struct TemplateParameter {
    std::string name;                              // Parameter name
    std::string type;                              // "string", "int", "bool", "string_list"
    std::string description;                       // Parameter description
    bool required = true;                          // Whether parameter is required
    std::string default_value;                     // Default value if not required
    std::vector<std::string> allowed_values;       // Allowed values (empty = any)
/// Parameter definition for a template
struct TemplateParameter {
    std::string name;                                  // Parameter name
    std::string type;                                  // "string", "int", "bool", "list"
    std::string description;                           // Parameter description
    nlohmann::json default_value;                      // Default value (optional)
    bool required = true;                              // Whether parameter is required
    std::vector<std::string> allowed_values;           // Allowed values (optional constraint)
    
    nlohmann::json toJson() const;
    static TemplateParameter fromJson(const nlohmann::json& j);
};

/// PolicyTemplate represents a parameterized policy rule template
class PolicyTemplate {
public:
    std::string id;                                // Unique template identifier
    std::string name;                              // Human-readable name
    std::string description;                       // Template description
    std::string category;                          // Category (security, compliance, etc.)
    std::vector<TemplateParameter> parameters;     // Template parameters
    
    // Template structure (will be filled with parameter values)
    std::string name_template;
    std::string description_template;
    std::string classification_level;
    std::vector<std::string> resources_template;
    std::vector<std::string> actions_template;
    std::vector<std::string> required_roles_template;
    bool require_encryption = false;
    bool require_signature = false;
    bool allow_export = true;
    bool allow_cache = true;
    int retention_days = 365;
    std::string redaction_level = "standard";
    bool audit_access = false;
    bool audit_changes = false;
    int priority = 0;
    
    /// Validate parameter values against parameter definitions
    struct ValidationResult {
        bool valid = true;
        std::vector<std::string> errors;
    };
    ValidationResult validateParameters(const std::unordered_map<std::string, std::string>& values) const;
    
    /// Instantiate template with parameter values to create a PolicyRule
    std::optional<PolicyRule> instantiate(
        const std::string& rule_id,
        const std::unordered_map<std::string, std::string>& parameter_values,
        const std::string& created_by
    ) const;
    
    /// Preview instantiation without creating the rule
    struct PreviewResult {
        PolicyRule rule;
        bool valid;
        std::vector<std::string> warnings;
    };
    PreviewResult preview(
        const std::string& rule_id,
        const std::unordered_map<std::string, std::string>& parameter_values
/// Policy template for creating rules from patterns
class PolicyTemplate {
public:
    std::string id;                                    // Unique template identifier
    std::string name;                                  // Human-readable name
    std::string description;                           // Template description
    std::string category;                              // Category (e.g., "security", "compliance")
    std::vector<TemplateParameter> parameters;         // Template parameters
    
    PolicyTemplate() = default;
    PolicyTemplate(
        const std::string& id,
        const std::string& name,
        const std::string& description,
        const std::string& category
    );
    
    /// Add a parameter to the template
    void addParameter(const TemplateParameter& param);
    
    /// Validate provided parameter values
    /// @param params Parameter values to validate
    /// @return True if all required parameters are present and valid
    bool validateParameters(const nlohmann::json& params) const;
    
    /// Instantiate a PolicyRule from this template
    /// @param params Parameter values
    /// @param rule_id ID for the new rule
    /// @return Generated PolicyRule
    PolicyRule instantiate(
        const nlohmann::json& params,
        const std::string& rule_id
    ) const;
    
    /// Preview what rule would be generated (without creating it)
    /// @param params Parameter values
    /// @param rule_id ID for the preview rule
    /// @return Preview of the PolicyRule that would be created
    PolicyRule preview(
        const nlohmann::json& params,
        const std::string& rule_id
    ) const;
    
    nlohmann::json toJson() const;
    static PolicyTemplate fromJson(const nlohmann::json& j);
    
private:
    /// Substitute parameters in a string template
    std::string substituteParameters(
        const std::string& template_str,
        const std::unordered_map<std::string, std::string>& values
    ) const;
    
    /// Substitute parameters in a vector template
    std::vector<std::string> substituteParametersInVector(
        const std::vector<std::string>& template_vec,
        const std::unordered_map<std::string, std::string>& values
    ) const;
};

/// PolicyTemplateManager manages a library of policy templates
class PolicyTemplateManager {
public:
    PolicyTemplateManager();
    
    /// Add a template to the library
    void addTemplate(const PolicyTemplate& tmpl);
    
    /// Remove a template
    void removeTemplate(const std::string& template_id);
    
    /// Get a template by ID
    std::optional<PolicyTemplate> getTemplate(const std::string& template_id) const;
    
    /// List all templates
    std::vector<PolicyTemplate> listTemplates() const;
    
    /// List templates by category
    std::vector<PolicyTemplate> listTemplatesByCategory(const std::string& category) const;
    
    /// Load built-in templates
    void loadBuiltInTemplates();
    
    /// Load templates from JSON file
    bool loadTemplates(const std::string& path);
    
    /// Save templates to JSON file
    bool saveTemplates(const std::string& path) const;
    
    /// Create a policy rule from a template
    std::optional<PolicyRule> instantiateTemplate(
        const std::string& template_id,
        const std::string& rule_id,
        const std::unordered_map<std::string, std::string>& parameter_values,
        const std::string& created_by
    ) const;
    
    /// Preview template instantiation
    PolicyTemplate::PreviewResult previewTemplate(
        const std::string& template_id,
        const std::string& rule_id,
        const std::unordered_map<std::string, std::string>& parameter_values
    ) const;
    
    /// Export templates as JSON
    nlohmann::json exportTemplates() const;
    
    /// Import templates from JSON
    bool importTemplates(const nlohmann::json& j);
    
private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, PolicyTemplate> templates_;
    
    /// Built-in template creators
    PolicyTemplate createLeastPrivilegeTemplate();
    PolicyTemplate createDataLifecycleTemplate();
    PolicyTemplate createComplianceTemplate();
    PolicyTemplate createSeparationOfDutiesTemplate();
    PolicyTemplate createTimeBasedAccessTemplate();
protected:
    /// Override this in subclasses to define template-specific instantiation logic
    virtual PolicyRule instantiateImpl(
        const nlohmann::json& params,
        const std::string& rule_id
    ) const = 0;
    
    /// Helper: Get parameter value with default fallback
    template<typename T>
    T getParam(const nlohmann::json& params, const std::string& name, const T& default_val) const {
        if (params.contains(name)) {
            return params[name].get<T>();
        }
        
        // Check template default
        for (const auto& param : parameters) {
            if (param.name == name && !param.default_value.is_null()) {
                return param.default_value.get<T>();
            }
        }
        
        return default_val;
    }
};

/// Template: Least Privilege - Minimize permissions
class LeastPrivilegeTemplate : public PolicyTemplate {
public:
    LeastPrivilegeTemplate();
    
protected:
    PolicyRule instantiateImpl(
        const nlohmann::json& params,
        const std::string& rule_id
    ) const override;
};

/// Template: Data Lifecycle - Retention and archival
class DataLifecycleTemplate : public PolicyTemplate {
public:
    DataLifecycleTemplate();
    
protected:
    PolicyRule instantiateImpl(
        const nlohmann::json& params,
        const std::string& rule_id
    ) const override;
};

/// Template: Compliance - Audit and encryption requirements
class ComplianceTemplate : public PolicyTemplate {
public:
    ComplianceTemplate();
    
protected:
    PolicyRule instantiateImpl(
        const nlohmann::json& params,
        const std::string& rule_id
    ) const override;
};

/// Template: Separation of Duties - Enforce role separation
class SeparationOfDutiesTemplate : public PolicyTemplate {
public:
    SeparationOfDutiesTemplate();
    
protected:
    PolicyRule instantiateImpl(
        const nlohmann::json& params,
        const std::string& rule_id
    ) const override;
};

/// Template: Time-based Access - Temporal access control
class TimeBasedAccessTemplate : public PolicyTemplate {
public:
    TimeBasedAccessTemplate();
    
protected:
    PolicyRule instantiateImpl(
        const nlohmann::json& params,
        const std::string& rule_id
    ) const override;
};

/// Manager for policy templates
class PolicyTemplateManager {
public:
    PolicyTemplateManager();
    
    /// Register a template
    void registerTemplate(std::shared_ptr<PolicyTemplate> tmpl);
    
    /// Get a template by ID
    std::optional<std::shared_ptr<PolicyTemplate>> getTemplate(const std::string& template_id) const;
    
    /// List all available templates
    std::vector<std::shared_ptr<PolicyTemplate>> listTemplates() const;
    
    /// List templates by category
    std::vector<std::shared_ptr<PolicyTemplate>> listTemplatesByCategory(
        const std::string& category
    ) const;
    
    /// Instantiate a rule from a template
    /// @param template_id Template identifier
    /// @param params Parameter values
    /// @param rule_id ID for the new rule
    /// @return Generated PolicyRule
    PolicyRule instantiateTemplate(
        const std::string& template_id,
        const nlohmann::json& params,
        const std::string& rule_id
    ) const;
    
    /// Preview template instantiation
    /// @param template_id Template identifier
    /// @param params Parameter values
    /// @param rule_id ID for the preview
    /// @return Preview of the PolicyRule
    PolicyRule previewTemplate(
        const std::string& template_id,
        const nlohmann::json& params,
        const std::string& rule_id
    ) const;
    
    /// Export all templates as JSON
    nlohmann::json exportTemplates() const;
    
private:
    std::unordered_map<std::string, std::shared_ptr<PolicyTemplate>> templates_;
    
    /// Helper: Register built-in templates
    void registerBuiltInTemplates();
};

} // namespace governance
} // namespace themis
