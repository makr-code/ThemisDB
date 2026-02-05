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
};

} // namespace governance
} // namespace themis
