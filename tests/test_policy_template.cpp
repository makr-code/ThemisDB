#include <gtest/gtest.h>
#include "governance/policy_template.h"
#include <filesystem>

using namespace themis::governance;

class PolicyTemplateTest : public ::testing::Test {
protected:
    void SetUp() override {
        manager = std::make_unique<PolicyTemplateManager>();
        test_dir = std::filesystem::temp_directory_path() / "themis_template_test";
        std::filesystem::create_directories(test_dir);
    }
    
    void TearDown() override {
        std::filesystem::remove_all(test_dir);
    }
    
    std::unique_ptr<PolicyTemplateManager> manager;
    std::filesystem::path test_dir;
};

// ========== Built-in Templates Tests ==========

TEST_F(PolicyTemplateTest, BuiltInTemplatesLoaded) {
    auto templates = manager->listTemplates();
    EXPECT_GE(templates.size(), 5); // At least 5 built-in templates
    
    // Check for specific templates
    EXPECT_TRUE(manager->getTemplate("least_privilege").has_value());
    EXPECT_TRUE(manager->getTemplate("data_lifecycle").has_value());
    EXPECT_TRUE(manager->getTemplate("compliance_audit").has_value());
    EXPECT_TRUE(manager->getTemplate("separation_of_duties").has_value());
    EXPECT_TRUE(manager->getTemplate("time_based_access").has_value());
}

TEST_F(PolicyTemplateTest, ListTemplatesByCategory) {
    auto security_templates = manager->listTemplatesByCategory("security");
    EXPECT_GT(security_templates.size(), 0);
    
    auto compliance_templates = manager->listTemplatesByCategory("compliance");
    EXPECT_GT(compliance_templates.size(), 0);
}

// ========== Template Instantiation Tests ==========

TEST_F(PolicyTemplateTest, InstantiateLeastPrivilegeTemplate) {
    std::unordered_map<std::string, std::string> params;
    params["resource_path"] = "data/sensitive/*";
    params["allowed_action"] = "read";
    params["role"] = "operator";
    
    auto rule = manager->instantiateTemplate("least_privilege", "rule_lp_001", params, "admin");
    
    ASSERT_TRUE(rule.has_value());
    EXPECT_EQ(rule->id, "rule_lp_001");
    EXPECT_EQ(rule->name, "Least Privilege: data/sensitive/* [read]");
    EXPECT_EQ(rule->resources.size(), 1);
    EXPECT_EQ(rule->resources[0], "data/sensitive/*");
    EXPECT_EQ(rule->actions.size(), 1);
    EXPECT_EQ(rule->actions[0], "read");
    EXPECT_EQ(rule->required_roles.size(), 1);
    EXPECT_EQ(rule->required_roles[0], "operator");
    EXPECT_TRUE(rule->require_encryption);
    EXPECT_FALSE(rule->allow_export);
    EXPECT_TRUE(rule->audit_access);
}

TEST_F(PolicyTemplateTest, InstantiateDataLifecycleTemplate) {
    std::unordered_map<std::string, std::string> params;
    params["data_type"] = "logs";
    params["retention_days"] = "90";
    
    auto rule = manager->instantiateTemplate("data_lifecycle", "rule_dl_001", params, "admin");
    
    ASSERT_TRUE(rule.has_value());
    EXPECT_EQ(rule->name, "Data Lifecycle: logs");
    EXPECT_EQ(rule->resources[0], "logs/*");
    EXPECT_TRUE(rule->audit_changes);
}

TEST_F(PolicyTemplateTest, InstantiateComplianceTemplate) {
    std::unordered_map<std::string, std::string> params;
    params["resource_category"] = "pii";
    params["classification"] = "geheim";
    
    auto rule = manager->instantiateTemplate("compliance_audit", "rule_c_001", params, "admin");
    
    ASSERT_TRUE(rule.has_value());
    EXPECT_EQ(rule->name, "Compliance: pii");
    EXPECT_EQ(rule->resources[0], "pii/*");
    EXPECT_TRUE(rule->require_encryption);
    EXPECT_TRUE(rule->require_signature);
    EXPECT_FALSE(rule->allow_export);
    EXPECT_TRUE(rule->audit_access);
    EXPECT_TRUE(rule->audit_changes);
    EXPECT_EQ(rule->retention_days, 2555); // 7 years
}

TEST_F(PolicyTemplateTest, InstantiateSeparationOfDutiesTemplate) {
    std::unordered_map<std::string, std::string> params;
    params["resource"] = "financial/transactions";
    params["action"] = "approve";
    params["authorized_role"] = "approver";
    
    auto rule = manager->instantiateTemplate("separation_of_duties", "rule_sod_001", params, "admin");
    
    ASSERT_TRUE(rule.has_value());
    EXPECT_EQ(rule->name, "Separation of Duties: financial/transactions [approve]");
    EXPECT_EQ(rule->resources[0], "financial/transactions");
    EXPECT_EQ(rule->actions[0], "approve");
    EXPECT_EQ(rule->required_roles[0], "approver");
}

TEST_F(PolicyTemplateTest, InstantiateTimeBasedAccessTemplate) {
    std::unordered_map<std::string, std::string> params;
    params["resource"] = "project/alpha";
    params["temp_role"] = "contractor";
    params["duration_days"] = "30";
    
    auto rule = manager->instantiateTemplate("time_based_access", "rule_tba_001", params, "admin");
    
    ASSERT_TRUE(rule.has_value());
    EXPECT_EQ(rule->name, "Temporary Access: project/alpha for contractor");
    EXPECT_EQ(rule->resources[0], "project/alpha");
    EXPECT_EQ(rule->required_roles[0], "contractor");
}

// ========== Parameter Validation Tests ==========

TEST_F(PolicyTemplateTest, MissingRequiredParameter) {
    std::unordered_map<std::string, std::string> params;
    params["resource_path"] = "data/*";
    // Missing required_action and role
    
    auto rule = manager->instantiateTemplate("least_privilege", "rule_fail_001", params, "admin");
    EXPECT_FALSE(rule.has_value());
}

TEST_F(PolicyTemplateTest, InvalidParameterType) {
    std::unordered_map<std::string, std::string> params;
    params["data_type"] = "logs";
    params["retention_days"] = "invalid"; // Should be int
    
    auto rule = manager->instantiateTemplate("data_lifecycle", "rule_fail_002", params, "admin");
    EXPECT_FALSE(rule.has_value());
}

TEST_F(PolicyTemplateTest, InvalidAllowedValue) {
    std::unordered_map<std::string, std::string> params;
    params["resource_path"] = "data/*";
    params["allowed_action"] = "invalid_action"; // Not in allowed values
    params["role"] = "operator";
    
    auto rule = manager->instantiateTemplate("least_privilege", "rule_fail_003", params, "admin");
    EXPECT_FALSE(rule.has_value());
}

TEST_F(PolicyTemplateTest, ValidParameterValidation) {
    auto tmpl = manager->getTemplate("least_privilege");
    ASSERT_TRUE(tmpl.has_value());
    
    std::unordered_map<std::string, std::string> params;
    params["resource_path"] = "data/*";
    params["allowed_action"] = "read";
    params["role"] = "operator";
    
    auto validation = tmpl->validateParameters(params);
    EXPECT_TRUE(validation.valid);
    EXPECT_EQ(validation.errors.size(), 0);
}

TEST_F(PolicyTemplateTest, InvalidParameterValidation) {
    auto tmpl = manager->getTemplate("least_privilege");
    ASSERT_TRUE(tmpl.has_value());
    
    std::unordered_map<std::string, std::string> params;
    params["resource_path"] = "data/*";
    // Missing required parameters
    
    auto validation = tmpl->validateParameters(params);
    EXPECT_FALSE(validation.valid);
    EXPECT_GT(validation.errors.size(), 0);
}

// ========== Preview Tests ==========

TEST_F(PolicyTemplateTest, PreviewValidTemplate) {
    std::unordered_map<std::string, std::string> params;
    params["resource_path"] = "test/*";
    params["allowed_action"] = "write";
    params["role"] = "tester";
    
    auto preview = manager->previewTemplate("least_privilege", "preview_001", params);
    
    EXPECT_TRUE(preview.valid);
    EXPECT_EQ(preview.warnings.size(), 0);
    EXPECT_EQ(preview.rule.name, "Least Privilege: test/* [write]");
}

TEST_F(PolicyTemplateTest, PreviewInvalidTemplate) {
    std::unordered_map<std::string, std::string> params;
    params["resource_path"] = "test/*";
    // Missing required parameters
    
    auto preview = manager->previewTemplate("least_privilege", "preview_002", params);
    
    EXPECT_FALSE(preview.valid);
    EXPECT_GT(preview.warnings.size(), 0);
}

TEST_F(PolicyTemplateTest, PreviewNonExistentTemplate) {
    std::unordered_map<std::string, std::string> params;
    
    auto preview = manager->previewTemplate("nonexistent", "preview_003", params);
    
    EXPECT_FALSE(preview.valid);
    EXPECT_GT(preview.warnings.size(), 0);
}

// ========== Custom Template Tests ==========

TEST_F(PolicyTemplateTest, AddCustomTemplate) {
    PolicyTemplate custom;
    custom.id = "custom_template";
    custom.name = "Custom Test Template";
    custom.description = "A custom template for testing";
    custom.category = "test";
    
    TemplateParameter param;
    param.name = "test_param";
    param.type = "string";
    param.description = "Test parameter";
    param.required = true;
    custom.parameters.push_back(param);
    
    custom.name_template = "Custom: {{test_param}}";
    custom.description_template = "Custom rule for {{test_param}}";
    custom.resources_template = {"{{test_param}}/*"};
    custom.actions_template = {"read"};
    
    manager->addTemplate(custom);
    
    auto retrieved = manager->getTemplate("custom_template");
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->name, "Custom Test Template");
}

TEST_F(PolicyTemplateTest, RemoveTemplate) {
    // Add a custom template
    PolicyTemplate custom;
    custom.id = "removable";
    custom.name = "Removable Template";
    custom.category = "test";
    
    manager->addTemplate(custom);
    ASSERT_TRUE(manager->getTemplate("removable").has_value());
    
    manager->removeTemplate("removable");
    EXPECT_FALSE(manager->getTemplate("removable").has_value());
}

// ========== Serialization Tests ==========

TEST_F(PolicyTemplateTest, ExportAndImportTemplates) {
    auto exported = manager->exportTemplates();
    EXPECT_TRUE(exported.contains("templates"));
    EXPECT_GT(exported["templates"].size(), 0);
    
    auto new_manager = std::make_unique<PolicyTemplateManager>();
    // Clear built-in templates first
    for (const auto& tmpl : new_manager->listTemplates()) {
        new_manager->removeTemplate(tmpl.id);
    }
    
    ASSERT_TRUE(new_manager->importTemplates(exported));
    
    auto imported_templates = new_manager->listTemplates();
    EXPECT_EQ(imported_templates.size(), manager->listTemplates().size());
}

TEST_F(PolicyTemplateTest, SaveAndLoadTemplates) {
    auto save_path = test_dir / "templates.json";
    ASSERT_TRUE(manager->saveTemplates(save_path.string()));
    
    auto new_manager = std::make_unique<PolicyTemplateManager>();
    // Clear built-in templates
    for (const auto& tmpl : new_manager->listTemplates()) {
        new_manager->removeTemplate(tmpl.id);
    }
    
    ASSERT_TRUE(new_manager->loadTemplates(save_path.string()));
    
    auto loaded = new_manager->listTemplates();
    EXPECT_GE(loaded.size(), 5); // At least the built-in templates
}

// ========== Parameter Substitution Tests ==========

TEST_F(PolicyTemplateTest, ParameterSubstitution) {
    auto tmpl = manager->getTemplate("least_privilege");
    ASSERT_TRUE(tmpl.has_value());
    
    std::unordered_map<std::string, std::string> params;
    params["resource_path"] = "custom/path";
    params["allowed_action"] = "delete";
    params["role"] = "admin";
    
    auto rule = tmpl->instantiate("sub_test_001", params, "user1");
    
    ASSERT_TRUE(rule.has_value());
    EXPECT_TRUE(rule->name.find("custom/path") != std::string::npos);
    EXPECT_TRUE(rule->name.find("delete") != std::string::npos);
    EXPECT_EQ(rule->resources[0], "custom/path");
    EXPECT_EQ(rule->actions[0], "delete");
    EXPECT_EQ(rule->required_roles[0], "admin");
}

TEST_F(PolicyTemplateTest, MultipleParametersInSameField) {
    PolicyTemplate custom;
    custom.id = "multi_param";
    custom.name = "Multi Parameter Template";
    custom.category = "test";
    
    TemplateParameter param1;
    param1.name = "dept";
    param1.type = "string";
    param1.required = true;
    custom.parameters.push_back(param1);
    
    TemplateParameter param2;
    param2.name = "level";
    param2.type = "string";
    param2.required = true;
    custom.parameters.push_back(param2);
    
    custom.name_template = "Access: {{dept}} - {{level}}";
    custom.resources_template = {"{{dept}}/{{level}}/*"};
    custom.actions_template = {"read"};
    
    manager->addTemplate(custom);
    
    std::unordered_map<std::string, std::string> params;
    params["dept"] = "engineering";
    params["level"] = "senior";
    
    auto rule = manager->instantiateTemplate("multi_param", "multi_001", params, "admin");
    
    ASSERT_TRUE(rule.has_value());
    EXPECT_EQ(rule->name, "Access: engineering - senior");
    EXPECT_EQ(rule->resources[0], "engineering/senior/*");
}

// ========== Template Metadata Tests ==========

TEST_F(PolicyTemplateTest, TemplateHasCorrectMetadata) {
    auto tmpl = manager->getTemplate("least_privilege");
    ASSERT_TRUE(tmpl.has_value());
    
    EXPECT_EQ(tmpl->id, "least_privilege");
    EXPECT_FALSE(tmpl->name.empty());
    EXPECT_FALSE(tmpl->description.empty());
    EXPECT_EQ(tmpl->category, "security");
    EXPECT_GT(tmpl->parameters.size(), 0);
}

TEST_F(PolicyTemplateTest, ParameterMetadata) {
    auto tmpl = manager->getTemplate("least_privilege");
    ASSERT_TRUE(tmpl.has_value());
    
    bool found_resource_param = false;
    for (const auto& param : tmpl->parameters) {
        if (param.name == "resource_path") {
            found_resource_param = true;
            EXPECT_EQ(param.type, "string");
            EXPECT_TRUE(param.required);
            EXPECT_FALSE(param.description.empty());
        }
    }
    
    EXPECT_TRUE(found_resource_param);
}

// ========== Integration Tests ==========

TEST_F(PolicyTemplateTest, InstantiateAndUseWithPolicyManager) {
    PolicyManager policy_mgr;
    
    std::unordered_map<std::string, std::string> params;
    params["resource_path"] = "secure/data/*";
    params["allowed_action"] = "read";
    params["role"] = "viewer";
    
    auto rule = manager->instantiateTemplate("least_privilege", "integration_001", params, "admin");
    ASSERT_TRUE(rule.has_value());
    
    policy_mgr.addRule(*rule);
    
    auto retrieved = policy_mgr.getRule("integration_001");
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->name, rule->name);
    EXPECT_EQ(retrieved->resources, rule->resources);
}

TEST_F(PolicyTemplateTest, AllowedValuesValidation) {
    auto tmpl = manager->getTemplate("compliance_audit");
    ASSERT_TRUE(tmpl.has_value());
    
    // Valid value
    std::unordered_map<std::string, std::string> valid_params;
    valid_params["resource_category"] = "pii";
    valid_params["classification"] = "geheim";
    
    auto validation_valid = tmpl->validateParameters(valid_params);
    EXPECT_TRUE(validation_valid.valid);
    
    // Invalid value
    std::unordered_map<std::string, std::string> invalid_params;
    invalid_params["resource_category"] = "invalid_category";
    invalid_params["classification"] = "geheim";
    
    auto validation_invalid = tmpl->validateParameters(invalid_params);
    EXPECT_FALSE(validation_invalid.valid);
}
