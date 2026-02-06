#include <gtest/gtest.h>
#include "governance/policy_template.h"

using namespace themis::governance;

class PolicyTemplateTest : public ::testing::Test {
protected:
    void SetUp() override {
        template_manager = std::make_unique<PolicyTemplateManager>();
    }
    
    std::unique_ptr<PolicyTemplateManager> template_manager;
};

// ========== Template Manager Tests ==========

TEST_F(PolicyTemplateTest, ListTemplates) {
    auto templates = template_manager->listTemplates();
    
    // Should have 5 built-in templates
    EXPECT_EQ(templates.size(), 5);
}

TEST_F(PolicyTemplateTest, GetTemplateById) {
    auto tmpl = template_manager->getTemplate("least_privilege");
    
    ASSERT_TRUE(tmpl.has_value());
    EXPECT_EQ((*tmpl)->id, "least_privilege");
    EXPECT_EQ((*tmpl)->name, "Least Privilege Access");
    EXPECT_EQ((*tmpl)->category, "security");
}

TEST_F(PolicyTemplateTest, GetNonexistentTemplate) {
    auto tmpl = template_manager->getTemplate("nonexistent");
    
    EXPECT_FALSE(tmpl.has_value());
}

TEST_F(PolicyTemplateTest, ListTemplatesByCategory) {
    auto security_templates = template_manager->listTemplatesByCategory("security");
    auto compliance_templates = template_manager->listTemplatesByCategory("compliance");
    
    EXPECT_GE(security_templates.size(), 2);  // least_privilege, separation_of_duties, time_based
    EXPECT_GE(compliance_templates.size(), 1);  // data_lifecycle, compliance
}

// ========== Least Privilege Template Tests ==========

TEST_F(PolicyTemplateTest, LeastPrivilegeInstantiate) {
    nlohmann::json params = {
        {"resource_pattern", "data/sensitive/*"},
        {"allowed_action", "read"},
        {"required_role", "operator"}
    };
    
    auto rule = template_manager->instantiateTemplate("least_privilege", params, "rule_lp_001");
    
    EXPECT_EQ(rule.id, "rule_lp_001");
    EXPECT_EQ(rule.resources.size(), 1);
    EXPECT_EQ(rule.resources[0], "data/sensitive/*");
    EXPECT_EQ(rule.actions.size(), 1);
    EXPECT_EQ(rule.actions[0], "read");
    EXPECT_EQ(rule.required_roles.size(), 1);
    EXPECT_EQ(rule.required_roles[0], "operator");
    EXPECT_TRUE(rule.require_encryption);
    EXPECT_FALSE(rule.allow_export);
    EXPECT_FALSE(rule.allow_cache);
    EXPECT_TRUE(rule.audit_access);
}

TEST_F(PolicyTemplateTest, LeastPrivilegeMissingRequiredParam) {
    nlohmann::json params = {
        {"resource_pattern", "data/*"}
        // Missing allowed_action and required_role
    };
    
    EXPECT_THROW(
        template_manager->instantiateTemplate("least_privilege", params, "rule_lp_002"),
        std::invalid_argument
    );
}

TEST_F(PolicyTemplateTest, LeastPrivilegeInvalidAction) {
    nlohmann::json params = {
        {"resource_pattern", "data/*"},
        {"allowed_action", "invalid_action"},  // Not in allowed_values
        {"required_role", "operator"}
    };
    
    EXPECT_THROW(
        template_manager->instantiateTemplate("least_privilege", params, "rule_lp_003"),
        std::invalid_argument
    );
}

TEST_F(PolicyTemplateTest, LeastPrivilegeWithOptionalParam) {
    nlohmann::json params = {
        {"resource_pattern", "data/*"},
        {"allowed_action", "write"},
        {"required_role", "admin"},
        {"require_encryption", false}
    };
    
    auto rule = template_manager->instantiateTemplate("least_privilege", params, "rule_lp_004");
    
    EXPECT_FALSE(rule.require_encryption);
}

// ========== Data Lifecycle Template Tests ==========

TEST_F(PolicyTemplateTest, DataLifecycleInstantiate) {
    nlohmann::json params = {
        {"resource_pattern", "data/archive/*"},
        {"retention_days", 730},
        {"classification_level", "vs-nfd"}
    };
    
    auto rule = template_manager->instantiateTemplate("data_lifecycle", params, "rule_dl_001");
    
    EXPECT_EQ(rule.id, "rule_dl_001");
    EXPECT_EQ(rule.resources[0], "data/archive/*");
    EXPECT_EQ(rule.retention_days, 730);
    EXPECT_EQ(rule.classification_level, "vs-nfd");
    EXPECT_TRUE(rule.audit_changes);
}

TEST_F(PolicyTemplateTest, DataLifecycleWithDefaults) {
    nlohmann::json params = {
        {"resource_pattern", "data/temp/*"}
        // Use defaults for retention_days and classification_level
    };
    
    auto rule = template_manager->instantiateTemplate("data_lifecycle", params, "rule_dl_002");
    
    EXPECT_EQ(rule.retention_days, 365);  // Default
    EXPECT_EQ(rule.classification_level, "vs-nfd");  // Default
}

TEST_F(PolicyTemplateTest, DataLifecycleInvalidClassification) {
    nlohmann::json params = {
        {"resource_pattern", "data/*"},
        {"classification_level", "invalid"}
    };
    
    EXPECT_THROW(
        template_manager->instantiateTemplate("data_lifecycle", params, "rule_dl_003"),
        std::invalid_argument
    );
}

// ========== Compliance Template Tests ==========

TEST_F(PolicyTemplateTest, ComplianceInstantiate) {
    nlohmann::json params = {
        {"resource_pattern", "data/pii/*"},
        {"compliance_framework", "GDPR"},
        {"redaction_level", "strict"}
    };
    
    auto rule = template_manager->instantiateTemplate("compliance", params, "rule_comp_001");
    
    EXPECT_EQ(rule.id, "rule_comp_001");
    EXPECT_TRUE(rule.name.find("GDPR") != std::string::npos);
    EXPECT_EQ(rule.resources[0], "data/pii/*");
    EXPECT_TRUE(rule.require_encryption);
    EXPECT_TRUE(rule.require_signature);
    EXPECT_TRUE(rule.audit_access);
    EXPECT_TRUE(rule.audit_changes);
    EXPECT_EQ(rule.redaction_level, "strict");
    EXPECT_FALSE(rule.allow_export);
    EXPECT_EQ(rule.priority, 200);  // High priority
}

TEST_F(PolicyTemplateTest, ComplianceMultipleFrameworks) {
    std::vector<std::string> frameworks = {"GDPR", "SOX", "HIPAA", "PCI-DSS", "ISO27001"};
    
    for (const auto& framework : frameworks) {
        nlohmann::json params = {
            {"resource_pattern", "data/*"},
            {"compliance_framework", framework}
        };
        
        auto rule = template_manager->instantiateTemplate("compliance", params, "rule_comp_" + framework);
        EXPECT_TRUE(rule.name.find(framework) != std::string::npos);
    }
}

// ========== Separation of Duties Template Tests ==========

TEST_F(PolicyTemplateTest, SeparationOfDutiesInstantiate) {
    nlohmann::json params = {
        {"resource_pattern", "data/financial/*"},
        {"restricted_action", "approve"},
        {"authorized_role", "finance_manager"}
    };
    
    auto rule = template_manager->instantiateTemplate("separation_of_duties", params, "rule_sod_001");
    
    EXPECT_EQ(rule.id, "rule_sod_001");
    EXPECT_EQ(rule.resources[0], "data/financial/*");
    EXPECT_EQ(rule.actions[0], "approve");
    EXPECT_EQ(rule.required_roles[0], "finance_manager");
    EXPECT_TRUE(rule.require_signature);
    EXPECT_TRUE(rule.audit_access);
    EXPECT_TRUE(rule.audit_changes);
}

TEST_F(PolicyTemplateTest, SeparationOfDutiesAllActions) {
    std::vector<std::string> actions = {"write", "delete", "execute", "approve"};
    
    for (const auto& action : actions) {
        nlohmann::json params = {
            {"resource_pattern", "data/*"},
            {"restricted_action", action},
            {"authorized_role", "admin"}
        };
        
        auto rule = template_manager->instantiateTemplate("separation_of_duties", params, "rule_sod_" + action);
        EXPECT_EQ(rule.actions[0], action);
    }
}

// ========== Time-Based Access Template Tests ==========

TEST_F(PolicyTemplateTest, TimeBasedAccessInstantiate) {
    nlohmann::json params = {
        {"resource_pattern", "data/project/*"},
        {"required_role", "contractor"},
        {"access_duration_days", 90}
    };
    
    auto rule = template_manager->instantiateTemplate("time_based_access", params, "rule_tba_001");
    
    EXPECT_EQ(rule.id, "rule_tba_001");
    EXPECT_EQ(rule.resources[0], "data/project/*");
    EXPECT_EQ(rule.required_roles[0], "contractor");
    EXPECT_EQ(rule.retention_days, 90);
    EXPECT_TRUE(rule.audit_access);
}

TEST_F(PolicyTemplateTest, TimeBasedAccessWithDefaults) {
    nlohmann::json params = {
        {"resource_pattern", "data/*"},
        {"required_role", "temp_user"}
    };
    
    auto rule = template_manager->instantiateTemplate("time_based_access", params, "rule_tba_002");
    
    EXPECT_EQ(rule.retention_days, 30);  // Default duration
}

// ========== Template Preview Tests ==========

TEST_F(PolicyTemplateTest, PreviewTemplate) {
    nlohmann::json params = {
        {"resource_pattern", "data/*"},
        {"allowed_action", "read"},
        {"required_role", "viewer"}
    };
    
    auto preview = template_manager->previewTemplate("least_privilege", params, "preview_rule");
    
    EXPECT_EQ(preview.id, "preview_rule");
    EXPECT_EQ(preview.resources[0], "data/*");
    EXPECT_EQ(preview.actions[0], "read");
}

// ========== Template Export Tests ==========

TEST_F(PolicyTemplateTest, ExportTemplates) {
    auto exported = template_manager->exportTemplates();
    
    EXPECT_TRUE(exported.is_array());
    EXPECT_EQ(exported.size(), 5);
    
    // Check structure of first template
    EXPECT_TRUE(exported[0].contains("id"));
    EXPECT_TRUE(exported[0].contains("name"));
    EXPECT_TRUE(exported[0].contains("description"));
    EXPECT_TRUE(exported[0].contains("category"));
    EXPECT_TRUE(exported[0].contains("parameters"));
}

// ========== Parameter Validation Tests ==========

TEST_F(PolicyTemplateTest, TemplateParameterToJson) {
    TemplateParameter param;
    param.name = "test_param";
    param.type = "string";
    param.description = "Test parameter";
    param.required = true;
    param.default_value = "default";
    param.allowed_values = {"option1", "option2"};
    
    auto json = param.toJson();
    
    EXPECT_EQ(json["name"], "test_param");
    EXPECT_EQ(json["type"], "string");
    EXPECT_TRUE(json["required"].get<bool>());
    EXPECT_EQ(json["default_value"], "default");
    EXPECT_EQ(json["allowed_values"].size(), 2);
}

TEST_F(PolicyTemplateTest, TemplateParameterFromJson) {
    nlohmann::json json = {
        {"name", "test_param"},
        {"type", "int"},
        {"description", "Test parameter"},
        {"required", false},
        {"default_value", 42}
    };
    
    auto param = TemplateParameter::fromJson(json);
    
    EXPECT_EQ(param.name, "test_param");
    EXPECT_EQ(param.type, "int");
    EXPECT_FALSE(param.required);
    EXPECT_EQ(param.default_value, 42);
}

// ========== Integration Tests ==========

TEST_F(PolicyTemplateTest, CreateMultipleRulesFromSameTemplate) {
    nlohmann::json params1 = {
        {"resource_pattern", "data/dept1/*"},
        {"allowed_action", "read"},
        {"required_role", "dept1_user"}
    };
    
    nlohmann::json params2 = {
        {"resource_pattern", "data/dept2/*"},
        {"allowed_action", "write"},
        {"required_role", "dept2_user"}
    };
    
    auto rule1 = template_manager->instantiateTemplate("least_privilege", params1, "rule_dept1");
    auto rule2 = template_manager->instantiateTemplate("least_privilege", params2, "rule_dept2");
    
    EXPECT_NE(rule1.id, rule2.id);
    EXPECT_NE(rule1.resources[0], rule2.resources[0]);
    EXPECT_NE(rule1.actions[0], rule2.actions[0]);
}

TEST_F(PolicyTemplateTest, AllTemplatesInstantiable) {
    auto templates = template_manager->listTemplates();
    
    for (const auto& tmpl : templates) {
        // Build minimal valid params for each template
        nlohmann::json params;
        
        for (const auto& param : tmpl->parameters) {
            if (param.required && param.default_value.is_null()) {
                if (param.type == "string") {
                    if (!param.allowed_values.empty()) {
                        params[param.name] = param.allowed_values[0];
                    } else {
                        params[param.name] = "test_value";
                    }
                } else if (param.type == "int") {
                    params[param.name] = 100;
                } else if (param.type == "bool") {
                    params[param.name] = true;
                }
            }
        }
        
        EXPECT_NO_THROW({
            auto rule = template_manager->instantiateTemplate(tmpl->id, params, "test_rule");
            EXPECT_EQ(rule.id, "test_rule");
            EXPECT_TRUE(rule.enabled);
        });
    }
}

// Run all tests
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
