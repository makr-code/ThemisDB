/**
 * @file policy_template.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


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

struct TemplateParameter {
    std::string name;                                  // Parameter name
    std::string type;                                  // "string", "int", "bool", "list"
    std::string description;                           // Parameter description
    nlohmann::json default_value;                      // Default value (optional)
    bool required = true;                              // Whether parameter is required
    std::vector<std::string> allowed_values;           // Allowed values (optional constraint)

    /**
     * @brief To Json.
     * @return Return value.
     */
    nlohmann::json toJson() const;
    /**
     * @brief From Json.
     * @param[in] j Input parameter.
     * @return Return value.
     */
    static TemplateParameter fromJson(const nlohmann::json& j);
};

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

    /**
     * @brief Add Parameter.
     * @param[in] param Input parameter.
     */
    void addParameter(const TemplateParameter& param);

    /**
     * @brief Validate Parameters.
     * @param[in] params Input parameter.
     * @return True when the operation succeeds.
     */
    bool validateParameters(const nlohmann::json& params) const;

    /**
     * @brief Instantiate.
     * @param[in] params Input parameter.
     * @param[in] rule_id Identifier of the rule.
     * @return Return value.
     */
    PolicyRule instantiate(
        const nlohmann::json& params,
        const std::string& rule_id
    ) const;

    /**
     * @brief Preview.
     * @param[in] params Input parameter.
     * @param[in] rule_id Identifier of the rule.
     * @return Return value.
     */
    PolicyRule preview(
        const nlohmann::json& params,
        const std::string& rule_id
    ) const;

    /**
     * @brief To Json.
     * @return Return value.
     */
    nlohmann::json toJson() const;

protected:
    /**
     * @brief Instantiate Impl.
     * @param[in] params Input parameter.
     * @param[in] rule_id Identifier of the rule.
     * @return Return value.
     */
    virtual PolicyRule instantiateImpl(
        const nlohmann::json& params,
        const std::string& rule_id
    ) const = 0;

    template<typename T>
    T getParam(const nlohmann::json& params, const std::string& name, const T& default_val) const {
        if (params.contains(name)) {
            return params[name].get<T>();
        }

        for (const auto& param : parameters) {
            if (param.name == name && !param.default_value.is_null()) {
                return param.default_value.get<T>();
            }
        }

        return default_val;
    }
};

class LeastPrivilegeTemplate : public PolicyTemplate {
public:
    LeastPrivilegeTemplate();

protected:
    PolicyRule instantiateImpl(
        const nlohmann::json& params,
        const std::string& rule_id
    ) const override;
};

class DataLifecycleTemplate : public PolicyTemplate {
public:
    DataLifecycleTemplate();

protected:
    PolicyRule instantiateImpl(
        const nlohmann::json& params,
        const std::string& rule_id
    ) const override;
};

class ComplianceTemplate : public PolicyTemplate {
public:
    ComplianceTemplate();

protected:
    PolicyRule instantiateImpl(
        const nlohmann::json& params,
        const std::string& rule_id
    ) const override;
};

class SeparationOfDutiesTemplate : public PolicyTemplate {
public:
    SeparationOfDutiesTemplate();

protected:
    PolicyRule instantiateImpl(
        const nlohmann::json& params,
        const std::string& rule_id
    ) const override;
};

class TimeBasedAccessTemplate : public PolicyTemplate {
public:
    TimeBasedAccessTemplate();

protected:
    PolicyRule instantiateImpl(
        const nlohmann::json& params,
        const std::string& rule_id
    ) const override;
};

class Soc2ComplianceTemplate : public PolicyTemplate {
public:
    Soc2ComplianceTemplate();

protected:
    PolicyRule instantiateImpl(
        const nlohmann::json& params,
        const std::string& rule_id
    ) const override;
};

class PolicyTemplateManager {
public:
    PolicyTemplateManager();

    /**
     * @brief Register Template.
     * @param[in] tmpl Input parameter.
     */
    void registerTemplate(std::shared_ptr<PolicyTemplate> tmpl);

    /**
     * @brief Get Template.
     * @param[in] template_id Identifier of the template.
     * @return Return value.
     */
    std::optional<std::shared_ptr<PolicyTemplate>> getTemplate(const std::string& template_id) const;

    /**
     * @brief List Templates.
     * @return Return value.
     */
    std::vector<std::shared_ptr<PolicyTemplate>> listTemplates() const;

    /**
     * @brief List Templates By Category.
     * @param[in] category Input parameter.
     * @return Return value.
     */
    std::vector<std::shared_ptr<PolicyTemplate>> listTemplatesByCategory(
        const std::string& category
    ) const;

    /**
     * @brief Instantiate Template.
     * @param[in] template_id Identifier of the template.
     * @param[in] params Input parameter.
     * @param[in] rule_id Identifier of the rule.
     * @return Return value.
     */
    PolicyRule instantiateTemplate(
        const std::string& template_id,
        const nlohmann::json& params,
        const std::string& rule_id
    ) const;

    /**
     * @brief Preview Template.
     * @param[in] template_id Identifier of the template.
     * @param[in] params Input parameter.
     * @param[in] rule_id Identifier of the rule.
     * @return Return value.
     */
    PolicyRule previewTemplate(
        const std::string& template_id,
        const nlohmann::json& params,
        const std::string& rule_id
    ) const;

    /**
     * @brief Export Templates.
     * @return Return value.
     */
    nlohmann::json exportTemplates() const;

private:
    std::unordered_map<std::string, std::shared_ptr<PolicyTemplate>> templates_;

    /**
     * @brief Register Built In Templates.
     */
    void registerBuiltInTemplates();
};

} // namespace governance
} // namespace themis
