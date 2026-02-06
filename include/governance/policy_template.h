#pragma once

#include "governance/policy_manager.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <nlohmann/json.hpp>

namespace themis {
namespace governance {

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
