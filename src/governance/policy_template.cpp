/**
 * @file policy_template.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "governance/policy_template.h"

#include <chrono>
#include <stdexcept>

#include "utils/logger.h"

namespace themis {
namespace governance {

// ========== TemplateParameter Implementation ==========

nlohmann::json TemplateParameter::toJson() const {
    nlohmann::json j;
    j["name"]        = name;
    j["type"]        = type;
    j["description"] = description;
    j["required"]    = required;
    if (!default_value.is_null()) {
        j["default_value"] = default_value;
    }
    if (!allowed_values.empty()) {
        j["allowed_values"] = allowed_values;
    }
    return j;
}

TemplateParameter TemplateParameter::fromJson(const nlohmann::json &j) {
    TemplateParameter param = {};
    if (j.contains("name")) {
        param.name = j["name"].get<std::string>();
    }
    if (j.contains("type")) {
        param.type = j["type"].get<std::string>();
    }
    if (j.contains("description")) {
        param.description = j["description"].get<std::string>();
    }
    if (j.contains("required")) {
        param.required = j["required"].get<bool>();
    }
    if (j.contains("default_value")) {
        param.default_value = j["default_value"];
    }
    if (j.contains("allowed_values")) {
        param.allowed_values = j["allowed_values"].get<std::vector<std::string>>();
    }
    return param;
}

// ========== PolicyTemplate Implementation ==========

PolicyTemplate::PolicyTemplate(const std::string &id, const std::string &name, const std::string &description,
                               const std::string &category)
    : id(id), name(name), description(description), category(category) {}

void PolicyTemplate::addParameter(const TemplateParameter &param) {
    parameters.push_back(param);
}

bool PolicyTemplate::validateParameters(const nlohmann::json &params) const {
    for (const auto &param : parameters) {
        if (param.required && !params.contains(param.name)) {
            if (param.default_value.is_null()) {
                THEMIS_ERROR("Required parameter '{}' missing in template '{}'", param.name, id);
                return false;
            }
        }

        if (params.contains(param.name) && !param.allowed_values.empty()) {
            std::string value = params[param.name].get<std::string>();
            bool found        = false;
            for (const auto &allowed : param.allowed_values) {
                if (value == allowed) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                THEMIS_ERROR("Parameter '{}' value '{}' not in allowed values for template '{}'", param.name, value,
                             id);
                return false;
            }
        }
    }

    return true;
}

PolicyRule PolicyTemplate::instantiate(const nlohmann::json &params, const std::string &rule_id) const {
    if (!validateParameters(params)) {
        throw std::invalid_argument("Invalid parameters for template " + id);
    }

    return instantiateImpl(params, rule_id);
}

PolicyRule PolicyTemplate::preview(const nlohmann::json &params, const std::string &rule_id) const {
    return instantiate(params, rule_id);
}

nlohmann::json PolicyTemplate::toJson() const {
    nlohmann::json j;
    j["id"]          = id;
    j["name"]        = name;
    j["description"] = description;
    j["category"]    = category;

    nlohmann::json params_json = nlohmann::json::array();
    for (const auto &param : parameters) {
        params_json.push_back(param.toJson());
    }
    j["parameters"] = params_json;

    return j;
}

static long long nowSeconds() {
    return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())
        .count();
}

// ========== LeastPrivilegeTemplate Implementation ==========

LeastPrivilegeTemplate::LeastPrivilegeTemplate()
    : PolicyTemplate("least_privilege", "Least Privilege Access", "Minimize permissions to only what is necessary",
                     "security") {
    TemplateParameter resource_param;
    resource_param.name        = "resource_pattern";
    resource_param.type        = "string";
    resource_param.description = "Resource pattern to protect (e.g., 'data/sensitive/*')";
    resource_param.required    = true;
    addParameter(resource_param);

    TemplateParameter action_param;
    action_param.name           = "allowed_action";
    action_param.type           = "string";
    action_param.description    = "Allowed action (read, write, delete)";
    action_param.required       = true;
    action_param.allowed_values = {"read", "write", "delete", "*"};
    addParameter(action_param);

    TemplateParameter role_param;
    role_param.name        = "required_role";
    role_param.type        = "string";
    role_param.description = "Required role for access";
    role_param.required    = true;
    addParameter(role_param);

    TemplateParameter encrypt_param;
    encrypt_param.name          = "require_encryption";
    encrypt_param.type          = "bool";
    encrypt_param.description   = "Require encryption";
    encrypt_param.default_value = true;
    encrypt_param.required      = false;
    addParameter(encrypt_param);
}

PolicyRule LeastPrivilegeTemplate::instantiateImpl(const nlohmann::json &params, const std::string &rule_id) const {
    PolicyRule rule;
    rule.id                 = rule_id;
    rule.name               = "Least Privilege: " + params["resource_pattern"].get<std::string>();
    rule.description        = "Minimized access to " + params["resource_pattern"].get<std::string>();
    rule.resources          = {params["resource_pattern"].get<std::string>()};
    rule.actions            = {params["allowed_action"].get<std::string>()};
    rule.required_roles     = {params["required_role"].get<std::string>()};
    rule.require_encryption = getParam<bool>(params, "require_encryption", true);
    rule.allow_export       = false;
    rule.allow_cache        = false;
    rule.audit_access       = true;
    rule.priority           = 100;
    rule.enabled            = true;

    auto now        = nowSeconds();
    rule.created_at = now;
    rule.updated_at = now;

    return rule;
}

// ========== DataLifecycleTemplate Implementation ==========

DataLifecycleTemplate::DataLifecycleTemplate()
    : PolicyTemplate("data_lifecycle", "Data Lifecycle Management", "Manage data retention and archival policies",
                     "compliance") {
    TemplateParameter resource_param;
    resource_param.name        = "resource_pattern";
    resource_param.type        = "string";
    resource_param.description = "Resource pattern for lifecycle management";
    resource_param.required    = true;
    addParameter(resource_param);

    TemplateParameter retention_param;
    retention_param.name          = "retention_days";
    retention_param.type          = "int";
    retention_param.description   = "Number of days to retain data";
    retention_param.default_value = 365;
    retention_param.required      = false;
    addParameter(retention_param);

    TemplateParameter classification_param;
    classification_param.name           = "classification_level";
    classification_param.type           = "string";
    classification_param.description    = "Data classification level";
    classification_param.allowed_values = {"offen", "vs-nfd", "geheim", "streng-geheim"};
    classification_param.default_value  = "vs-nfd";
    classification_param.required       = false;
    addParameter(classification_param);
}

PolicyRule DataLifecycleTemplate::instantiateImpl(const nlohmann::json &params, const std::string &rule_id) const {
    PolicyRule rule;
    rule.id                   = rule_id;
    rule.name                 = "Data Lifecycle: " + params["resource_pattern"].get<std::string>();
    rule.description          = "Retention policy for " + params["resource_pattern"].get<std::string>();
    rule.resources            = {params["resource_pattern"].get<std::string>()};
    rule.actions              = {"*"};
    rule.retention_days       = getParam<int>(params, "retention_days", 365);
    rule.classification_level = getParam<std::string>(params, "classification_level", "vs-nfd");
    rule.audit_changes        = true;
    rule.priority             = 50;
    rule.enabled              = true;

    auto now        = nowSeconds();
    rule.created_at = now;
    rule.updated_at = now;

    return rule;
}

// ========== ComplianceTemplate Implementation ==========

ComplianceTemplate::ComplianceTemplate()
    : PolicyTemplate("compliance", "Compliance Requirements",
                     "Enforce audit and encryption requirements for compliance", "compliance") {
    TemplateParameter resource_param;
    resource_param.name        = "resource_pattern";
    resource_param.type        = "string";
    resource_param.description = "Resource pattern for compliance";
    resource_param.required    = true;
    addParameter(resource_param);

    TemplateParameter framework_param;
    framework_param.name           = "compliance_framework";
    framework_param.type           = "string";
    framework_param.description    = "Compliance framework (e.g., GDPR, SOX, HIPAA)";
    framework_param.allowed_values = {"GDPR", "SOX", "HIPAA", "PCI-DSS", "ISO27001", "SOC2"};
    framework_param.required       = true;
    addParameter(framework_param);

    TemplateParameter redaction_param;
    redaction_param.name           = "redaction_level";
    redaction_param.type           = "string";
    redaction_param.description    = "Data redaction level";
    redaction_param.allowed_values = {"none", "standard", "strict"};
    redaction_param.default_value  = "standard";
    redaction_param.required       = false;
    addParameter(redaction_param);
}

PolicyRule ComplianceTemplate::instantiateImpl(const nlohmann::json &params, const std::string &rule_id) const {
    PolicyRule rule;
    rule.id                 = rule_id;
    rule.name               = params["compliance_framework"].get<std::string>() + " Compliance";
    rule.description        = "Compliance rule for " + params["resource_pattern"].get<std::string>();
    rule.resources          = {params["resource_pattern"].get<std::string>()};
    rule.actions            = {"*"};
    rule.require_encryption = true;
    rule.require_signature  = true;
    rule.audit_access       = true;
    rule.audit_changes      = true;
    rule.redaction_level    = getParam<std::string>(params, "redaction_level", "standard");
    rule.allow_export       = false;
    rule.priority           = 200;
    rule.enabled            = true;

    auto now        = nowSeconds();
    rule.created_at = now;
    rule.updated_at = now;

    return rule;
}

// ========== SeparationOfDutiesTemplate Implementation ==========

SeparationOfDutiesTemplate::SeparationOfDutiesTemplate()
    : PolicyTemplate("separation_of_duties", "Separation of Duties", "Enforce role separation for critical operations",
                     "security") {
    TemplateParameter resource_param;
    resource_param.name        = "resource_pattern";
    resource_param.type        = "string";
    resource_param.description = "Resource requiring separation";
    resource_param.required    = true;
    addParameter(resource_param);

    TemplateParameter action_param;
    action_param.name           = "restricted_action";
    action_param.type           = "string";
    action_param.description    = "Action requiring special authorization";
    action_param.allowed_values = {"write", "delete", "execute", "approve"};
    action_param.required       = true;
    addParameter(action_param);

    TemplateParameter role_param;
    role_param.name        = "authorized_role";
    role_param.type        = "string";
    role_param.description = "Role authorized for this action";
    role_param.required    = true;
    addParameter(role_param);
}

PolicyRule SeparationOfDutiesTemplate::instantiateImpl(const nlohmann::json &params, const std::string &rule_id) const {
    PolicyRule rule;
    rule.id                = rule_id;
    rule.name              = "SOD: " + params["restricted_action"].get<std::string>();
    rule.description       = "Separation of duties for " + params["resource_pattern"].get<std::string>();
    rule.resources         = {params["resource_pattern"].get<std::string>()};
    rule.actions           = {params["restricted_action"].get<std::string>()};
    rule.required_roles    = {params["authorized_role"].get<std::string>()};
    rule.require_signature = true;
    rule.audit_access      = true;
    rule.audit_changes     = true;
    rule.priority          = 150;
    rule.enabled           = true;

    auto now        = nowSeconds();
    rule.created_at = now;
    rule.updated_at = now;

    return rule;
}

// ========== TimeBasedAccessTemplate Implementation ==========

TimeBasedAccessTemplate::TimeBasedAccessTemplate()
    : PolicyTemplate("time_based_access", "Time-Based Access Control", "Temporal access restrictions", "security") {
    TemplateParameter resource_param;
    resource_param.name        = "resource_pattern";
    resource_param.type        = "string";
    resource_param.description = "Resource with time restrictions";
    resource_param.required    = true;
    addParameter(resource_param);

    TemplateParameter role_param;
    role_param.name        = "required_role";
    role_param.type        = "string";
    role_param.description = "Role required for access";
    role_param.required    = true;
    addParameter(role_param);

    TemplateParameter duration_param;
    duration_param.name          = "access_duration_days";
    duration_param.type          = "int";
    duration_param.description   = "Access duration in days";
    duration_param.default_value = 30;
    duration_param.required      = false;
    addParameter(duration_param);
}

PolicyRule TimeBasedAccessTemplate::instantiateImpl(const nlohmann::json &params, const std::string &rule_id) const {
    PolicyRule rule;
    rule.id             = rule_id;
    rule.name           = "Time-Based Access: " + params["resource_pattern"].get<std::string>();
    rule.description    = "Temporal access to " + params["resource_pattern"].get<std::string>();
    rule.resources      = {params["resource_pattern"].get<std::string>()};
    rule.actions        = {"*"};
    rule.required_roles = {params["required_role"].get<std::string>()};
    rule.retention_days = getParam<int>(params, "access_duration_days", 30);
    rule.audit_access   = true;
    rule.priority       = 75;
    rule.enabled        = true;

    auto now        = nowSeconds();
    rule.created_at = now;
    rule.updated_at = now;

    return rule;
}

// ========== Soc2ComplianceTemplate Implementation ==========

Soc2ComplianceTemplate::Soc2ComplianceTemplate()
    : PolicyTemplate("soc2_compliance", "SOC 2 Compliance",
                     "Enforce SOC 2 Trust Services Criteria controls: field-level encryption, "
                     "audit logging, change management, and role-based access",
                     "compliance") {
    TemplateParameter resource_param;
    resource_param.name        = "resource_pattern";
    resource_param.type        = "string";
    resource_param.description = "Resource pattern to protect under SOC 2 controls";
    resource_param.required    = true;
    addParameter(resource_param);

    TemplateParameter role_param;
    role_param.name        = "required_role";
    role_param.type        = "string";
    role_param.description = "Role authorized to access the resource";
    role_param.required    = true;
    addParameter(role_param);

    TemplateParameter retention_param;
    retention_param.name          = "retention_days";
    retention_param.type          = "int";
    retention_param.description   = "Data retention period in days (A1.1 availability commitment)";
    retention_param.default_value = 365;
    retention_param.required      = false;
    addParameter(retention_param);

    TemplateParameter classification_param;
    classification_param.name           = "classification_level";
    classification_param.type           = "string";
    classification_param.description    = "Data classification level";
    classification_param.allowed_values = {"offen", "vs-nfd", "geheim", "streng-geheim"};
    classification_param.default_value  = "vs-nfd";
    classification_param.required       = false;
    addParameter(classification_param);
}

PolicyRule Soc2ComplianceTemplate::instantiateImpl(const nlohmann::json &params, const std::string &rule_id) const {
    PolicyRule rule;
    rule.id   = rule_id;
    rule.name = "SOC 2: " + params["resource_pattern"].get<std::string>();
    rule.description
        = "SOC 2 Trust Services Criteria compliance rule for " + params["resource_pattern"].get<std::string>();
    rule.resources            = {params["resource_pattern"].get<std::string>()};
    rule.actions              = {"*"};
    rule.required_roles       = {params["required_role"].get<std::string>()};
    rule.classification_level = getParam<std::string>(params, "classification_level", "vs-nfd");
    rule.retention_days       = getParam<int>(params, "retention_days", 365);

    // CC6.1: Field-level encryption and role-based access control
    rule.require_encryption = true;
    // CC8.1: Authorized change procedures require digital signature
    rule.require_signature = true;
    // CC7.2: System operations – detect unauthorized access and changes
    rule.audit_access  = true;
    rule.audit_changes = true;
    // C1.1: Confidentiality – restrict export and apply redaction
    rule.allow_export    = false;
    rule.allow_cache     = false;
    rule.redaction_level = "standard";

    rule.priority = 200;
    rule.enabled  = true;

    auto now        = nowSeconds();
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

std::optional<std::shared_ptr<PolicyTemplate>>
PolicyTemplateManager::getTemplate(const std::string &template_id) const {
    auto it = templates_.find(template_id);
    if (it != templates_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<std::shared_ptr<PolicyTemplate>> PolicyTemplateManager::listTemplates() const {
    std::vector<std::shared_ptr<PolicyTemplate>> result;
    for (const auto &[id, tmpl] : templates_) {
        result.push_back(tmpl);
    }
    return result;
}

std::vector<std::shared_ptr<PolicyTemplate>>
PolicyTemplateManager::listTemplatesByCategory(const std::string &category) const {
    std::vector<std::shared_ptr<PolicyTemplate>> result;
    for (const auto &[id, tmpl] : templates_) {
        if (tmpl->category == category) {
            result.push_back(tmpl);
        }
    }
    return result;
}

PolicyRule PolicyTemplateManager::instantiateTemplate(const std::string &template_id, const nlohmann::json &params,
                                                      const std::string &rule_id) const {
    auto tmpl = getTemplate(template_id);
    if (!tmpl.has_value()) {
        throw std::invalid_argument("Template not found: " + template_id);
    }

    return (*tmpl)->instantiate(params, rule_id);
}

PolicyRule PolicyTemplateManager::previewTemplate(const std::string &template_id, const nlohmann::json &params,
                                                  const std::string &rule_id) const {
    auto tmpl = getTemplate(template_id);
    if (!tmpl.has_value()) {
        throw std::invalid_argument("Template not found: " + template_id);
    }

    return (*tmpl)->preview(params, rule_id);
}

nlohmann::json PolicyTemplateManager::exportTemplates() const {
    nlohmann::json j = nlohmann::json::array();
    for (const auto &[id, tmpl] : templates_) {
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
    registerTemplate(std::make_shared<Soc2ComplianceTemplate>());

    THEMIS_INFO("Registered {} built-in policy templates", templates_.size());
}

} // namespace governance
} // namespace themis
