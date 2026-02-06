#include "governance/policy_template.h"
#include "utils/logger.h"

#include <algorithm>
#include <fstream>
#include <chrono>
#include <regex>
#include <chrono>

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
    if (!default_value.is_null()) {
        j["default_value"] = default_value;
    }
    if (!allowed_values.empty()) {
        j["allowed_values"] = allowed_values;
    }
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
    TemplateParameter p;
    if (j.contains("name")) p.name = j["name"].get<std::string>();
    if (j.contains("type")) p.type = j["type"].get<std::string>();
    if (j.contains("description")) p.description = j["description"].get<std::string>();
    if (j.contains("required")) p.required = j["required"].get<bool>();
    if (j.contains("default_value")) p.default_value = j["default_value"];
    if (j.contains("allowed_values")) p.allowed_values = j["allowed_values"].get<std::vector<std::string>>();
    return p;
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
PolicyTemplate::PolicyTemplate(
    const std::string& id,
    const std::string& name,
    const std::string& description,
    const std::string& category
)
    : id(id)
    , name(name)
    , description(description)
    , category(category)
{
}

void PolicyTemplate::addParameter(const TemplateParameter& param) {
    parameters.push_back(param);
}

bool PolicyTemplate::validateParameters(const nlohmann::json& params) const {
    // Check all required parameters are present
    for (const auto& param : parameters) {
        if (param.required && !params.contains(param.name)) {
            // Check if parameter has default value
            if (param.default_value.is_null()) {
                THEMIS_ERROR("Required parameter '{}' missing in template '{}'", param.name, id);
                return false;
            }
        }
        
        // Validate value if present
        if (params.contains(param.name)) {
            // Check allowed values constraint
            if (!param.allowed_values.empty()) {
                std::string value = params[param.name].get<std::string>();
                bool found = false;
                for (const auto& allowed : param.allowed_values) {
                    if (value == allowed) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    THEMIS_ERROR("Parameter '{}' value '{}' not in allowed values for template '{}'",
                        param.name, value, id);
                    return false;
                }
            }
        }
    }
    
    return true;
}

PolicyRule PolicyTemplate::instantiate(
    const nlohmann::json& params,
    const std::string& rule_id
) const {
    if (!validateParameters(params)) {
        throw std::invalid_argument("Invalid parameters for template " + id);
    }
    
    return instantiateImpl(params, rule_id);
}

PolicyRule PolicyTemplate::preview(
    const nlohmann::json& params,
    const std::string& rule_id
) const {
    // Preview is same as instantiate but doesn't persist
    return instantiate(params, rule_id);
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
    
    nlohmann::json params_json = nlohmann::json::array();
    for (const auto& param : parameters) {
        params_json.push_back(param.toJson());
    }
    j["parameters"] = params_json;
    
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
    // Note: This creates a base template, subclasses need custom deserialization
    PolicyTemplate t;
    if (j.contains("id")) t.id = j["id"].get<std::string>();
    if (j.contains("name")) t.name = j["name"].get<std::string>();
    if (j.contains("description")) t.description = j["description"].get<std::string>();
    if (j.contains("category")) t.category = j["category"].get<std::string>();
    
    if (j.contains("parameters")) {
        for (const auto& param_json : j["parameters"]) {
            t.parameters.push_back(TemplateParameter::fromJson(param_json));
        }
    }
    
    return t;
}

// ========== LeastPrivilegeTemplate Implementation ==========

LeastPrivilegeTemplate::LeastPrivilegeTemplate()
    : PolicyTemplate(
        "least_privilege",
        "Least Privilege Access",
        "Minimize permissions to only what is necessary",
        "security"
    )
{
    TemplateParameter resource_param;
    resource_param.name = "resource_pattern";
    resource_param.type = "string";
    resource_param.description = "Resource pattern to protect (e.g., 'data/sensitive/*')";
    resource_param.required = true;
    addParameter(resource_param);
    
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
    action_param.description = "Single allowed action";
    action_param.allowed_values = {"read", "write", "delete", "execute"};
    action_param.required = true;
    addParameter(action_param);
    
    TemplateParameter role_param;
    role_param.name = "required_role";
    role_param.type = "string";
    role_param.description = "Role required for access";
    role_param.required = true;
    addParameter(role_param);
    
    TemplateParameter encrypt_param;
    encrypt_param.name = "require_encryption";
    encrypt_param.type = "bool";
    encrypt_param.description = "Require encryption";
    encrypt_param.default_value = true;
    encrypt_param.required = false;
    addParameter(encrypt_param);
}

PolicyRule LeastPrivilegeTemplate::instantiateImpl(
    const nlohmann::json& params,
    const std::string& rule_id
) const {
    PolicyRule rule;
    rule.id = rule_id;
    rule.name = "Least Privilege: " + params["resource_pattern"].get<std::string>();
    rule.description = "Minimized access to " + params["resource_pattern"].get<std::string>();
    rule.resources = {params["resource_pattern"].get<std::string>()};
    rule.actions = {params["allowed_action"].get<std::string>()};
    rule.required_roles = {params["required_role"].get<std::string>()};
    rule.require_encryption = getParam<bool>(params, "require_encryption", true);
    rule.allow_export = false;  // Restrictive by default
    rule.allow_cache = false;   // Restrictive by default
    rule.audit_access = true;   // Always audit for least privilege
    rule.priority = 100;
    rule.enabled = true;
    
    auto now = std::chrono::system_clock::now().time_since_epoch().count() / 1000000000;
    rule.created_at = now;
    rule.updated_at = now;
    
    return rule;
}

// ========== DataLifecycleTemplate Implementation ==========

DataLifecycleTemplate::DataLifecycleTemplate()
    : PolicyTemplate(
        "data_lifecycle",
        "Data Lifecycle Management",
        "Manage data retention and archival policies",
        "compliance"
    )
{
    TemplateParameter resource_param;
    resource_param.name = "resource_pattern";
    resource_param.type = "string";
    resource_param.description = "Resource pattern for lifecycle management";
    resource_param.required = true;
    addParameter(resource_param);
    
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
    retention_param.description = "Data retention period in days";
    retention_param.default_value = 365;
    retention_param.required = false;
    addParameter(retention_param);
    
    TemplateParameter classification_param;
    classification_param.name = "classification_level";
    classification_param.type = "string";
    classification_param.description = "Data classification level";
    classification_param.allowed_values = {"offen", "vs-nfd", "geheim", "streng-geheim"};
    classification_param.default_value = "vs-nfd";
    classification_param.required = false;
    addParameter(classification_param);
}

PolicyRule DataLifecycleTemplate::instantiateImpl(
    const nlohmann::json& params,
    const std::string& rule_id
) const {
    PolicyRule rule;
    rule.id = rule_id;
    rule.name = "Data Lifecycle: " + params["resource_pattern"].get<std::string>();
    rule.description = "Retention policy for " + params["resource_pattern"].get<std::string>();
    rule.resources = {params["resource_pattern"].get<std::string>()};
    rule.actions = {"*"};  // Applies to all actions
    rule.retention_days = getParam<int>(params, "retention_days", 365);
    rule.classification_level = getParam<std::string>(params, "classification_level", "vs-nfd");
    rule.audit_changes = true;  // Track lifecycle events
    rule.priority = 50;
    rule.enabled = true;
    
    auto now = std::chrono::system_clock::now().time_since_epoch().count() / 1000000000;
    rule.created_at = now;
    rule.updated_at = now;
    
    return rule;
}

// ========== ComplianceTemplate Implementation ==========

ComplianceTemplate::ComplianceTemplate()
    : PolicyTemplate(
        "compliance",
        "Compliance Requirements",
        "Enforce audit and encryption requirements for compliance",
        "compliance"
    )
{
    TemplateParameter resource_param;
    resource_param.name = "resource_pattern";
    resource_param.type = "string";
    resource_param.description = "Resource pattern for compliance";
    resource_param.required = true;
    addParameter(resource_param);
    
    TemplateParameter framework_param;
    framework_param.name = "compliance_framework";
    framework_param.type = "string";
    framework_param.description = "Compliance framework (e.g., GDPR, SOX, HIPAA)";
    framework_param.allowed_values = {"GDPR", "SOX", "HIPAA", "PCI-DSS", "ISO27001"};
    framework_param.required = true;
    addParameter(framework_param);
    
    TemplateParameter redaction_param;
    redaction_param.name = "redaction_level";
    redaction_param.type = "string";
    redaction_param.description = "Data redaction level";
    redaction_param.allowed_values = {"none", "standard", "strict"};
    redaction_param.default_value = "standard";
    redaction_param.required = false;
    addParameter(redaction_param);
}

PolicyRule ComplianceTemplate::instantiateImpl(
    const nlohmann::json& params,
    const std::string& rule_id
) const {
    PolicyRule rule;
    rule.id = rule_id;
    rule.name = params["compliance_framework"].get<std::string>() + " Compliance";
    rule.description = "Compliance rule for " + params["resource_pattern"].get<std::string>();
    rule.resources = {params["resource_pattern"].get<std::string>()};
    rule.actions = {"*"};
    rule.require_encryption = true;
    rule.require_signature = true;
    rule.audit_access = true;
    rule.audit_changes = true;
    rule.redaction_level = getParam<std::string>(params, "redaction_level", "standard");
    rule.allow_export = false;  // Compliance typically restricts export
    rule.priority = 200;  // High priority for compliance
    rule.enabled = true;
    
    auto now = std::chrono::system_clock::now().time_since_epoch().count() / 1000000000;
    rule.created_at = now;
    rule.updated_at = now;
    
    return rule;
}

// ========== SeparationOfDutiesTemplate Implementation ==========

SeparationOfDutiesTemplate::SeparationOfDutiesTemplate()
    : PolicyTemplate(
        "separation_of_duties",
        "Separation of Duties",
        "Enforce role separation for critical operations",
        "security"
    )
{
    TemplateParameter resource_param;
    resource_param.name = "resource_pattern";
    resource_param.type = "string";
    resource_param.description = "Resource requiring separation";
    resource_param.required = true;
    addParameter(resource_param);
    
    TemplateParameter action_param;
    action_param.name = "restricted_action";
    action_param.type = "string";
    action_param.description = "Action requiring special authorization";
    action_param.allowed_values = {"write", "delete", "execute", "approve"};
    action_param.required = true;
    addParameter(action_param);
    
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
    addParameter(role_param);
}

PolicyRule SeparationOfDutiesTemplate::instantiateImpl(
    const nlohmann::json& params,
    const std::string& rule_id
) const {
    PolicyRule rule;
    rule.id = rule_id;
    rule.name = "SOD: " + params["restricted_action"].get<std::string>();
    rule.description = "Separation of duties for " + params["resource_pattern"].get<std::string>();
    rule.resources = {params["resource_pattern"].get<std::string>()};
    rule.actions = {params["restricted_action"].get<std::string>()};
    rule.required_roles = {params["authorized_role"].get<std::string>()};
    rule.require_signature = true;  // Require signature for SOD
    rule.audit_access = true;
    rule.audit_changes = true;
    rule.priority = 150;
    rule.enabled = true;
    
    auto now = std::chrono::system_clock::now().time_since_epoch().count() / 1000000000;
    rule.created_at = now;
    rule.updated_at = now;
    
    return rule;
}

// ========== TimeBasedAccessTemplate Implementation ==========

TimeBasedAccessTemplate::TimeBasedAccessTemplate()
    : PolicyTemplate(
        "time_based_access",
        "Time-Based Access Control",
        "Temporal access restrictions",
        "security"
    )
{
    TemplateParameter resource_param;
    resource_param.name = "resource_pattern";
    resource_param.type = "string";
    resource_param.description = "Resource with time restrictions";
    resource_param.required = true;
    addParameter(resource_param);
    
    TemplateParameter role_param;
    role_param.name = "required_role";
    role_param.type = "string";
    role_param.description = "Role required for access";
    role_param.required = true;
    addParameter(role_param);
    
    TemplateParameter duration_param;
    duration_param.name = "access_duration_days";
    duration_param.type = "int";
    duration_param.description = "Access duration in days";
    duration_param.default_value = 30;
    duration_param.required = false;
    addParameter(duration_param);
}

PolicyRule TimeBasedAccessTemplate::instantiateImpl(
    const nlohmann::json& params,
    const std::string& rule_id
) const {
    PolicyRule rule;
    rule.id = rule_id;
    rule.name = "Time-Based Access: " + params["resource_pattern"].get<std::string>();
    rule.description = "Temporal access to " + params["resource_pattern"].get<std::string>();
    rule.resources = {params["resource_pattern"].get<std::string>()};
    rule.actions = {"*"};
    rule.required_roles = {params["required_role"].get<std::string>()};
    rule.retention_days = getParam<int>(params, "access_duration_days", 30);
    rule.audit_access = true;
    rule.priority = 75;
    rule.enabled = true;
    
    auto now = std::chrono::system_clock::now().time_since_epoch().count() / 1000000000;
    rule.created_at = now;
    rule.updated_at = now;
    
    return rule;
}

// ========== PolicyTemplateManager Implementation ==========

PolicyTemplateManager::PolicyTemplateManager() {
    registerBuiltInTemplates();
}

void PolicyTemplateManager::registerTemplate(std::shared_ptr<PolicyTemplate> tmpl) {
    templates_[tmpl->id] = tmpl;
    THEMIS_INFO("Registered policy template: {}", tmpl->id);
}

std::optional<std::shared_ptr<PolicyTemplate>> PolicyTemplateManager::getTemplate(
    const std::string& template_id
) const {
    auto it = templates_.find(template_id);
    if (it != templates_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<std::shared_ptr<PolicyTemplate>> PolicyTemplateManager::listTemplates() const {
    std::vector<std::shared_ptr<PolicyTemplate>> result;
    for (const auto& [id, tmpl] : templates_) {
        result.push_back(tmpl);
    }
    return result;
}

std::vector<std::shared_ptr<PolicyTemplate>> PolicyTemplateManager::listTemplatesByCategory(
    const std::string& category
) const {
    std::vector<std::shared_ptr<PolicyTemplate>> result;
    for (const auto& [id, tmpl] : templates_) {
        if (tmpl->category == category) {
            result.push_back(tmpl);
        }
    }
    return result;
}

PolicyRule PolicyTemplateManager::instantiateTemplate(
    const std::string& template_id,
    const nlohmann::json& params,
    const std::string& rule_id
) const {
    auto tmpl = getTemplate(template_id);
    if (!tmpl.has_value()) {
        throw std::invalid_argument("Template not found: " + template_id);
    }
    
    return (*tmpl)->instantiate(params, rule_id);
}

PolicyRule PolicyTemplateManager::previewTemplate(
    const std::string& template_id,
    const nlohmann::json& params,
    const std::string& rule_id
) const {
    auto tmpl = getTemplate(template_id);
    if (!tmpl.has_value()) {
        throw std::invalid_argument("Template not found: " + template_id);
    }
    
    return (*tmpl)->preview(params, rule_id);
}

nlohmann::json PolicyTemplateManager::exportTemplates() const {
    nlohmann::json j = nlohmann::json::array();
    for (const auto& [id, tmpl] : templates_) {
        j.push_back(tmpl->toJson());
    }
    return j;
}

void PolicyTemplateManager::registerBuiltInTemplates() {
    registerTemplate(std::make_shared<LeastPrivilegeTemplate>());
    registerTemplate(std::make_shared<DataLifecycleTemplate>());
    registerTemplate(std::make_shared<ComplianceTemplate>());
    registerTemplate(std::make_shared<SeparationOfDutiesTemplate>());
    registerTemplate(std::make_shared<TimeBasedAccessTemplate>());
    
    THEMIS_INFO("Registered {} built-in policy templates", templates_.size());
}

} // namespace governance
} // namespace themis
