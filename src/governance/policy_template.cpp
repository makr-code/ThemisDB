#include "governance/policy_template.h"
#include "utils/logger.h"

#include <algorithm>
#include <fstream>
#include <chrono>
#include <regex>

namespace themis {
namespace governance {

// ========== TemplateParameter Implementation ==========

nlohmann::json TemplateParameter::toJson() const {
    nlohmann::json j;
    j["name"] = name;
    j["type"] = type;
    j["description"] = description;
    j["required"] = required;
    j["default_value"] = default_value;
    j["allowed_values"] = allowed_values;
    return j;
}

TemplateParameter TemplateParameter::fromJson(const nlohmann::json& j) {
    TemplateParameter param;
    if (j.contains("name")) param.name = j["name"].get<std::string>();
    if (j.contains("type")) param.type = j["type"].get<std::string>();
    if (j.contains("description")) param.description = j["description"].get<std::string>();
    if (j.contains("required")) param.required = j["required"].get<bool>();
    if (j.contains("default_value")) param.default_value = j["default_value"].get<std::string>();
    if (j.contains("allowed_values")) param.allowed_values = j["allowed_values"].get<std::vector<std::string>>();
    return param;
}

// ========== PolicyTemplate Implementation ==========

PolicyTemplate::ValidationResult PolicyTemplate::validateParameters(
    const std::unordered_map<std::string, std::string>& values) const {
    ValidationResult result;
    result.valid = true;
    
    // Check required parameters
    for (const auto& param : parameters) {
        if (param.required && values.find(param.name) == values.end()) {
            result.valid = false;
            result.errors.push_back("Missing required parameter: " + param.name);
            continue;
        }
        
        auto it = values.find(param.name);
        if (it == values.end()) {
            continue; // Optional parameter not provided
        }
        
        const std::string& value = it->second;
        
        // Type validation
        if (param.type == "int") {
            try {
                std::stoi(value);
            } catch (...) {
                result.valid = false;
                result.errors.push_back("Parameter '" + param.name + "' must be an integer");
            }
        } else if (param.type == "bool") {
            if (value != "true" && value != "false") {
                result.valid = false;
                result.errors.push_back("Parameter '" + param.name + "' must be 'true' or 'false'");
            }
        }
        
        // Allowed values validation
        if (!param.allowed_values.empty()) {
            bool found = false;
            for (const auto& allowed : param.allowed_values) {
                if (value == allowed) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                result.valid = false;
                result.errors.push_back("Parameter '" + param.name + "' has invalid value. Allowed: " + 
                                       [&]() {
                                           std::string s;
                                           for (size_t i = 0; i < param.allowed_values.size(); i++) {
                                               if (i > 0) s += ", ";
                                               s += param.allowed_values[i];
                                           }
                                           return s;
                                       }());
            }
        }
    }
    
    return result;
}

std::string PolicyTemplate::substituteParameters(
    const std::string& template_str,
    const std::unordered_map<std::string, std::string>& values) const {
    std::string result = template_str;
    
    // Replace {{param_name}} with actual values
    for (const auto& [name, value] : values) {
        std::string placeholder = "{{" + name + "}}";
        size_t pos = 0;
        while ((pos = result.find(placeholder, pos)) != std::string::npos) {
            result.replace(pos, placeholder.length(), value);
            pos += value.length();
        }
    }
    
    // Fill in default values for optional parameters
    for (const auto& param : parameters) {
        if (!param.required && values.find(param.name) == values.end() && !param.default_value.empty()) {
            std::string placeholder = "{{" + param.name + "}}";
            size_t pos = 0;
            while ((pos = result.find(placeholder, pos)) != std::string::npos) {
                result.replace(pos, placeholder.length(), param.default_value);
                pos += param.default_value.length();
            }
        }
    }
    
    return result;
}

std::vector<std::string> PolicyTemplate::substituteParametersInVector(
    const std::vector<std::string>& template_vec,
    const std::unordered_map<std::string, std::string>& values) const {
    std::vector<std::string> result;
    for (const auto& tmpl : template_vec) {
        result.push_back(substituteParameters(tmpl, values));
    }
    return result;
}

std::optional<PolicyRule> PolicyTemplate::instantiate(
    const std::string& rule_id,
    const std::unordered_map<std::string, std::string>& parameter_values,
    const std::string& created_by) const {
    
    // Validate parameters
    auto validation = validateParameters(parameter_values);
    if (!validation.valid) {
        THEMIS_ERROR("Template instantiation failed validation: {}", validation.errors[0]);
        return std::nullopt;
    }
    
    // Create rule from template
    PolicyRule rule;
    rule.id = rule_id;
    rule.name = substituteParameters(name_template, parameter_values);
    rule.description = substituteParameters(description_template, parameter_values);
    rule.classification_level = classification_level;
    rule.resources = substituteParametersInVector(resources_template, parameter_values);
    rule.actions = substituteParametersInVector(actions_template, parameter_values);
    rule.required_roles = substituteParametersInVector(required_roles_template, parameter_values);
    rule.require_encryption = require_encryption;
    rule.require_signature = require_signature;
    rule.allow_export = allow_export;
    rule.allow_cache = allow_cache;
    rule.retention_days = retention_days;
    rule.redaction_level = redaction_level;
    rule.audit_access = audit_access;
    rule.audit_changes = audit_changes;
    rule.priority = priority;
    rule.created_by = created_by;
    rule.created_at = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    rule.updated_at = rule.created_at;
    rule.version = "1.0.0";
    rule.enabled = true;
    
    return rule;
}

PolicyTemplate::PreviewResult PolicyTemplate::preview(
    const std::string& rule_id,
    const std::unordered_map<std::string, std::string>& parameter_values) const {
    
    PreviewResult result;
    result.valid = true;
    
    // Validate parameters
    auto validation = validateParameters(parameter_values);
    result.valid = validation.valid;
    if (!validation.valid) {
        for (const auto& error : validation.errors) {
            result.warnings.push_back(error);
        }
    }
    
    // Create preview rule (even if invalid, for preview purposes)
    auto rule_opt = instantiate(rule_id, parameter_values, "preview_user");
    if (rule_opt) {
        result.rule = *rule_opt;
    }
    
    return result;
}

nlohmann::json PolicyTemplate::toJson() const {
    nlohmann::json j;
    j["id"] = id;
    j["name"] = name;
    j["description"] = description;
    j["category"] = category;
    j["parameters"] = nlohmann::json::array();
    for (const auto& param : parameters) {
        j["parameters"].push_back(param.toJson());
    }
    j["name_template"] = name_template;
    j["description_template"] = description_template;
    j["classification_level"] = classification_level;
    j["resources_template"] = resources_template;
    j["actions_template"] = actions_template;
    j["required_roles_template"] = required_roles_template;
    j["require_encryption"] = require_encryption;
    j["require_signature"] = require_signature;
    j["allow_export"] = allow_export;
    j["allow_cache"] = allow_cache;
    j["retention_days"] = retention_days;
    j["redaction_level"] = redaction_level;
    j["audit_access"] = audit_access;
    j["audit_changes"] = audit_changes;
    j["priority"] = priority;
    return j;
}

PolicyTemplate PolicyTemplate::fromJson(const nlohmann::json& j) {
    PolicyTemplate tmpl;
    if (j.contains("id")) tmpl.id = j["id"].get<std::string>();
    if (j.contains("name")) tmpl.name = j["name"].get<std::string>();
    if (j.contains("description")) tmpl.description = j["description"].get<std::string>();
    if (j.contains("category")) tmpl.category = j["category"].get<std::string>();
    if (j.contains("parameters")) {
        for (const auto& param_json : j["parameters"]) {
            tmpl.parameters.push_back(TemplateParameter::fromJson(param_json));
        }
    }
    if (j.contains("name_template")) tmpl.name_template = j["name_template"].get<std::string>();
    if (j.contains("description_template")) tmpl.description_template = j["description_template"].get<std::string>();
    if (j.contains("classification_level")) tmpl.classification_level = j["classification_level"].get<std::string>();
    if (j.contains("resources_template")) tmpl.resources_template = j["resources_template"].get<std::vector<std::string>>();
    if (j.contains("actions_template")) tmpl.actions_template = j["actions_template"].get<std::vector<std::string>>();
    if (j.contains("required_roles_template")) tmpl.required_roles_template = j["required_roles_template"].get<std::vector<std::string>>();
    if (j.contains("require_encryption")) tmpl.require_encryption = j["require_encryption"].get<bool>();
    if (j.contains("require_signature")) tmpl.require_signature = j["require_signature"].get<bool>();
    if (j.contains("allow_export")) tmpl.allow_export = j["allow_export"].get<bool>();
    if (j.contains("allow_cache")) tmpl.allow_cache = j["allow_cache"].get<bool>();
    if (j.contains("retention_days")) tmpl.retention_days = j["retention_days"].get<int>();
    if (j.contains("redaction_level")) tmpl.redaction_level = j["redaction_level"].get<std::string>();
    if (j.contains("audit_access")) tmpl.audit_access = j["audit_access"].get<bool>();
    if (j.contains("audit_changes")) tmpl.audit_changes = j["audit_changes"].get<bool>();
    if (j.contains("priority")) tmpl.priority = j["priority"].get<int>();
    return tmpl;
}

// ========== PolicyTemplateManager Implementation ==========

PolicyTemplateManager::PolicyTemplateManager() {
    loadBuiltInTemplates();
}

void PolicyTemplateManager::addTemplate(const PolicyTemplate& tmpl) {
    std::lock_guard<std::mutex> lock(mutex_);
    templates_[tmpl.id] = tmpl;
    THEMIS_DEBUG("Added policy template: {} ({})", tmpl.id, tmpl.name);
}

void PolicyTemplateManager::removeTemplate(const std::string& template_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    templates_.erase(template_id);
    THEMIS_DEBUG("Removed policy template: {}", template_id);
}

std::optional<PolicyTemplate> PolicyTemplateManager::getTemplate(const std::string& template_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = templates_.find(template_id);
    if (it != templates_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<PolicyTemplate> PolicyTemplateManager::listTemplates() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<PolicyTemplate> result;
    result.reserve(templates_.size());
    for (const auto& [id, tmpl] : templates_) {
        result.push_back(tmpl);
    }
    return result;
}

std::vector<PolicyTemplate> PolicyTemplateManager::listTemplatesByCategory(const std::string& category) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<PolicyTemplate> result;
    for (const auto& [id, tmpl] : templates_) {
        if (tmpl.category == category) {
            result.push_back(tmpl);
        }
    }
    return result;
}

bool PolicyTemplateManager::loadTemplates(const std::string& path) {
    try {
        std::ifstream file(path);
        if (!file.is_open()) {
            THEMIS_WARN("Template file not found: {}", path);
            return false;
        }
        
        nlohmann::json j;
        file >> j;
        
        return importTemplates(j);
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Failed to load templates from {}: {}", path, e.what());
        return false;
    }
}

bool PolicyTemplateManager::saveTemplates(const std::string& path) const {
    try {
        std::ofstream file(path);
        if (!file.is_open()) {
            THEMIS_ERROR("Failed to open file for writing: {}", path);
            return false;
        }
        
        nlohmann::json j = exportTemplates();
        file << j.dump(2);
        
        THEMIS_INFO("Saved {} policy templates to {}", templates_.size(), path);
        return true;
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Failed to save templates to {}: {}", path, e.what());
        return false;
    }
}

std::optional<PolicyRule> PolicyTemplateManager::instantiateTemplate(
    const std::string& template_id,
    const std::string& rule_id,
    const std::unordered_map<std::string, std::string>& parameter_values,
    const std::string& created_by) const {
    
    auto tmpl = getTemplate(template_id);
    if (!tmpl) {
        THEMIS_WARN("Template not found: {}", template_id);
        return std::nullopt;
    }
    
    return tmpl->instantiate(rule_id, parameter_values, created_by);
}

PolicyTemplate::PreviewResult PolicyTemplateManager::previewTemplate(
    const std::string& template_id,
    const std::string& rule_id,
    const std::unordered_map<std::string, std::string>& parameter_values) const {
    
    PolicyTemplate::PreviewResult result;
    result.valid = false;
    
    auto tmpl = getTemplate(template_id);
    if (!tmpl) {
        result.warnings.push_back("Template not found: " + template_id);
        return result;
    }
    
    return tmpl->preview(rule_id, parameter_values);
}

nlohmann::json PolicyTemplateManager::exportTemplates() const {
    std::lock_guard<std::mutex> lock(mutex_);
    nlohmann::json j;
    j["version"] = "1.0";
    j["templates"] = nlohmann::json::array();
    
    for (const auto& [id, tmpl] : templates_) {
        j["templates"].push_back(tmpl.toJson());
    }
    
    return j;
}

bool PolicyTemplateManager::importTemplates(const nlohmann::json& j) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    try {
        if (!j.contains("templates")) {
            THEMIS_ERROR("Invalid template JSON: missing 'templates' field");
            return false;
        }
        
        templates_.clear();
        
        for (const auto& tmpl_json : j["templates"]) {
            PolicyTemplate tmpl = PolicyTemplate::fromJson(tmpl_json);
            templates_[tmpl.id] = tmpl;
        }
        
        THEMIS_INFO("Imported {} policy templates", templates_.size());
        return true;
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Failed to import templates: {}", e.what());
        return false;
    }
}

// ========== Built-in Templates ==========

void PolicyTemplateManager::loadBuiltInTemplates() {
    addTemplate(createLeastPrivilegeTemplate());
    addTemplate(createDataLifecycleTemplate());
    addTemplate(createComplianceTemplate());
    addTemplate(createSeparationOfDutiesTemplate());
    addTemplate(createTimeBasedAccessTemplate());
    
    THEMIS_INFO("Loaded {} built-in policy templates", templates_.size());
}

PolicyTemplate PolicyTemplateManager::createLeastPrivilegeTemplate() {
    PolicyTemplate tmpl;
    tmpl.id = "least_privilege";
    tmpl.name = "Least Privilege Access";
    tmpl.description = "Implements least privilege principle by limiting access to specific resources and actions";
    tmpl.category = "security";
    
    // Parameters
    TemplateParameter resource_param;
    resource_param.name = "resource_path";
    resource_param.type = "string";
    resource_param.description = "Resource path pattern (e.g., data/sensitive/*)";
    resource_param.required = true;
    tmpl.parameters.push_back(resource_param);
    
    TemplateParameter action_param;
    action_param.name = "allowed_action";
    action_param.type = "string";
    action_param.description = "Allowed action (read, write, delete)";
    action_param.required = true;
    action_param.allowed_values = {"read", "write", "delete", "*"};
    tmpl.parameters.push_back(action_param);
    
    TemplateParameter role_param;
    role_param.name = "role";
    role_param.type = "string";
    role_param.description = "Required role for access";
    role_param.required = true;
    tmpl.parameters.push_back(role_param);
    
    // Template structure
    tmpl.name_template = "Least Privilege: {{resource_path}} [{{allowed_action}}]";
    tmpl.description_template = "Allows {{allowed_action}} access to {{resource_path}} for role {{role}}";
    tmpl.classification_level = "vs-nfd";
    tmpl.resources_template = {"{{resource_path}}"};
    tmpl.actions_template = {"{{allowed_action}}"};
    tmpl.required_roles_template = {"{{role}}"};
    tmpl.require_encryption = true;
    tmpl.allow_export = false;
    tmpl.audit_access = true;
    tmpl.priority = 100;
    
    return tmpl;
}

PolicyTemplate PolicyTemplateManager::createDataLifecycleTemplate() {
    PolicyTemplate tmpl;
    tmpl.id = "data_lifecycle";
    tmpl.name = "Data Lifecycle Management";
    tmpl.description = "Manages data retention and archival policies";
    tmpl.category = "compliance";
    
    // Parameters
    TemplateParameter resource_param;
    resource_param.name = "data_type";
    resource_param.type = "string";
    resource_param.description = "Type of data (logs, documents, backups)";
    resource_param.required = true;
    tmpl.parameters.push_back(resource_param);
    
    TemplateParameter retention_param;
    retention_param.name = "retention_days";
    retention_param.type = "int";
    retention_param.description = "Number of days to retain data";
    retention_param.required = true;
    tmpl.parameters.push_back(retention_param);
    
    // Template structure
    tmpl.name_template = "Data Lifecycle: {{data_type}}";
    tmpl.description_template = "Manages {{data_type}} with {{retention_days}} day retention";
    tmpl.classification_level = "offen";
    tmpl.resources_template = {"{{data_type}}/*"};
    tmpl.actions_template = {"*"};
    tmpl.require_encryption = false;
    tmpl.allow_export = true;
    tmpl.audit_changes = true;
    tmpl.retention_days = 365; // Will be overridden by parameter
    tmpl.priority = 50;
    
    return tmpl;
}

PolicyTemplate PolicyTemplateManager::createComplianceTemplate() {
    PolicyTemplate tmpl;
    tmpl.id = "compliance_audit";
    tmpl.name = "Compliance Audit Requirements";
    tmpl.description = "Enforces audit logging and encryption for compliance";
    tmpl.category = "compliance";
    
    // Parameters
    TemplateParameter resource_param;
    resource_param.name = "resource_category";
    resource_param.type = "string";
    resource_param.description = "Resource category (pii, financial, medical)";
    resource_param.required = true;
    resource_param.allowed_values = {"pii", "financial", "medical", "legal"};
    tmpl.parameters.push_back(resource_param);
    
    TemplateParameter classification_param;
    classification_param.name = "classification";
    classification_param.type = "string";
    classification_param.description = "Classification level";
    classification_param.required = true;
    classification_param.allowed_values = {"offen", "vs-nfd", "geheim", "streng-geheim"};
    tmpl.parameters.push_back(classification_param);
    
    // Template structure
    tmpl.name_template = "Compliance: {{resource_category}}";
    tmpl.description_template = "Compliance requirements for {{resource_category}} data";
    tmpl.classification_level = "vs-nfd"; // Will be overridden by parameter
    tmpl.resources_template = {"{{resource_category}}/*"};
    tmpl.actions_template = {"*"};
    tmpl.require_encryption = true;
    tmpl.require_signature = true;
    tmpl.allow_export = false;
    tmpl.audit_access = true;
    tmpl.audit_changes = true;
    tmpl.retention_days = 2555; // 7 years for compliance
    tmpl.priority = 200;
    
    return tmpl;
}

PolicyTemplate PolicyTemplateManager::createSeparationOfDutiesTemplate() {
    PolicyTemplate tmpl;
    tmpl.id = "separation_of_duties";
    tmpl.name = "Separation of Duties";
    tmpl.description = "Ensures different roles are required for different actions";
    tmpl.category = "security";
    
    // Parameters
    TemplateParameter resource_param;
    resource_param.name = "resource";
    resource_param.type = "string";
    resource_param.description = "Protected resource path";
    resource_param.required = true;
    tmpl.parameters.push_back(resource_param);
    
    TemplateParameter action_param;
    action_param.name = "action";
    action_param.type = "string";
    action_param.description = "Protected action";
    action_param.required = true;
    action_param.allowed_values = {"read", "write", "delete", "approve"};
    tmpl.parameters.push_back(action_param);
    
    TemplateParameter role_param;
    role_param.name = "authorized_role";
    role_param.type = "string";
    role_param.description = "Role authorized for this action";
    role_param.required = true;
    tmpl.parameters.push_back(role_param);
    
    // Template structure
    tmpl.name_template = "Separation of Duties: {{resource}} [{{action}}]";
    tmpl.description_template = "Only {{authorized_role}} can {{action}} on {{resource}}";
    tmpl.classification_level = "vs-nfd";
    tmpl.resources_template = {"{{resource}}"};
    tmpl.actions_template = {"{{action}}"};
    tmpl.required_roles_template = {"{{authorized_role}}"};
    tmpl.require_encryption = true;
    tmpl.audit_access = true;
    tmpl.audit_changes = true;
    tmpl.priority = 150;
    
    return tmpl;
}

PolicyTemplate PolicyTemplateManager::createTimeBasedAccessTemplate() {
    PolicyTemplate tmpl;
    tmpl.id = "time_based_access";
    tmpl.name = "Time-Based Access Control";
    tmpl.description = "Temporary access with expiration";
    tmpl.category = "security";
    
    // Parameters
    TemplateParameter resource_param;
    resource_param.name = "resource";
    resource_param.type = "string";
    resource_param.description = "Resource path for temporary access";
    resource_param.required = true;
    tmpl.parameters.push_back(resource_param);
    
    TemplateParameter role_param;
    role_param.name = "temp_role";
    role_param.type = "string";
    role_param.description = "Temporary role";
    role_param.required = true;
    tmpl.parameters.push_back(role_param);
    
    TemplateParameter duration_param;
    duration_param.name = "duration_days";
    duration_param.type = "int";
    duration_param.description = "Access duration in days";
    duration_param.required = true;
    tmpl.parameters.push_back(duration_param);
    
    // Template structure
    tmpl.name_template = "Temporary Access: {{resource}} for {{temp_role}}";
    tmpl.description_template = "Temporary {{duration_days}}-day access to {{resource}} for {{temp_role}}";
    tmpl.classification_level = "offen";
    tmpl.resources_template = {"{{resource}}"};
    tmpl.actions_template = {"read"};
    tmpl.required_roles_template = {"{{temp_role}}"};
    tmpl.require_encryption = false;
    tmpl.audit_access = true;
    tmpl.retention_days = 90; // Will be overridden by parameter
    tmpl.priority = 75;
    
    return tmpl;
}

} // namespace governance
} // namespace themis
