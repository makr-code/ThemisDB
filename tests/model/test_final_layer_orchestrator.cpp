/*
 * @file test_final_layer_orchestrator.cpp
 * @brief Unit tests for FinalLayerOrchestrator.
 */

#include <gtest/gtest.h>

#include "llm/final_layer_orchestrator.h"

using namespace themis::llm;

namespace {

AdapterMetadata makeAdapter(std::string adapter_id,
                            std::string base_model,
                            std::string architecture,
                            AdapterRole role = AdapterRole::GENERAL,
                            AdapterMetadata::Status status = AdapterMetadata::Status::DEPLOYED) {
    AdapterMetadata meta;
    meta.adapter_id = std::move(adapter_id);
    meta.base_model_name = std::move(base_model);
    meta.architecture = std::move(architecture);
    meta.role = role;
    meta.status = status;
    return meta;
}

FinalLayerPackage makePackage() {
    FinalLayerPackage pkg;
    pkg.package_id = "pkg-legal-qa";
    pkg.target_model_id = "llama-7b";
    pkg.model_family = "llama";
    pkg.base_model_version = "3.1";
    pkg.primary_adapter_id = "legal-general";
    pkg.domain = "legal";
    pkg.task_type = "question-answering";
    return pkg;
}

} // namespace

TEST(FinalLayerOrchestratorTest, RegisterAndListPackages) {
    FinalLayerOrchestrator orchestrator;
    ASSERT_TRUE(orchestrator.registerPackage(makePackage()));

    const auto packages = orchestrator.listPackages();
    ASSERT_EQ(packages.size(), 1u);
    EXPECT_EQ(packages[0].package_id, "pkg-legal-qa");
}

TEST(FinalLayerOrchestratorTest, ResolveRequestedPackageUsesPrimaryAdapter) {
    auto registry = std::make_shared<AdapterRegistry>(nullptr);
    ASSERT_TRUE(registry->registerAdapter(makeAdapter("legal-general", "llama-7b", "llama")));

    FinalLayerOrchestrator orchestrator;
    orchestrator.setAdapterRegistry(registry);
    ASSERT_TRUE(orchestrator.registerPackage(makePackage()));

    FinalLayerRequest request;
    request.requested_package_id = "pkg-legal-qa";
    request.base_model_name = "llama-7b";
    request.base_model_version = "3.1";

    const auto resolution = orchestrator.resolve(request);
    EXPECT_TRUE(resolution.resolved);
    EXPECT_EQ(resolution.primary_adapter_id, "legal-general");
    EXPECT_EQ(resolution.package_id, "pkg-legal-qa");
}

TEST(FinalLayerOrchestratorTest, RouterSelectsPackageByModelId) {
    auto registry = std::make_shared<AdapterRegistry>(nullptr);
    ASSERT_TRUE(registry->registerAdapter(makeAdapter("legal-general", "llama-7b", "llama")));

    auto router = std::make_shared<ModelRouter>();
    RoutingRule rule;
    rule.id = "legal-route";
    rule.target_model_id = "llama-7b";
    rule.metadata_tags = {"legal"};
    router->addRule(rule);

    FinalLayerOrchestrator orchestrator;
    orchestrator.setAdapterRegistry(registry);
    orchestrator.setModelRouter(router);
    ASSERT_TRUE(orchestrator.registerPackage(makePackage()));

    FinalLayerRequest request;
    request.prompt = "analyze contract";
    request.base_model_name = "llama-7b";
    request.base_model_version = "3.1";
    request.metadata["tags"] = nlohmann::json::array({"legal"});

    const auto resolution = orchestrator.resolve(request);
    EXPECT_TRUE(resolution.resolved);
    EXPECT_EQ(resolution.model_id, "llama-7b");
    EXPECT_EQ(resolution.package_id, "pkg-legal-qa");
}

TEST(FinalLayerOrchestratorTest, DraftAdapterAutoDiscoveredFromRegistry) {
    auto registry = std::make_shared<AdapterRegistry>(nullptr);
    ASSERT_TRUE(registry->registerAdapter(makeAdapter("legal-general", "llama-7b", "llama")));
    ASSERT_TRUE(registry->registerAdapter(
        makeAdapter("llama-draft", "llama-0.5b", "llama", AdapterRole::DRAFT)));

    FinalLayerOrchestrator orchestrator;
    orchestrator.setAdapterRegistry(registry);
    ASSERT_TRUE(orchestrator.registerPackage(makePackage()));

    FinalLayerRequest request;
    request.requested_package_id = "pkg-legal-qa";
    request.base_model_name = "llama-7b";
    request.base_model_version = "3.1";
    request.allow_draft_adapter = true;

    const auto resolution = orchestrator.resolve(request);
    EXPECT_TRUE(resolution.resolved);
    EXPECT_EQ(resolution.draft_adapter_id, "llama-draft");
}

TEST(FinalLayerOrchestratorTest, CompatibilityMatrixContainsPrimaryAndDraftRows) {
    auto registry = std::make_shared<AdapterRegistry>(nullptr);
    ASSERT_TRUE(registry->registerAdapter(makeAdapter("legal-general", "llama-7b", "llama")));
    ASSERT_TRUE(registry->registerAdapter(
        makeAdapter("llama-draft", "llama-0.5b", "llama", AdapterRole::DRAFT)));

    auto pkg = makePackage();
    pkg.draft_adapter_id = "llama-draft";

    FinalLayerOrchestrator orchestrator;
    orchestrator.setAdapterRegistry(registry);
    ASSERT_TRUE(orchestrator.registerPackage(pkg));

    const auto matrix = orchestrator.buildCompatibilityMatrix("pkg-legal-qa");
    ASSERT_EQ(matrix.size(), 2u);
    EXPECT_EQ(matrix[0].target_model_id, "llama-7b");
}
