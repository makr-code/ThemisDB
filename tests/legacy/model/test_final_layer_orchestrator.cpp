/*
 * @file test_final_layer_orchestrator.cpp
 * @brief Unit tests for FinalLayerOrchestrator.
 */

#include <gtest/gtest.h>

#include "llm/final_layer_orchestrator.h"
#include "observability/reason_codes.h"
#include "utils/logger.h"

#include <chrono>
#include <filesystem>
#include <fstream>

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

TEST(FinalLayerOrchestratorTest, PromotionWorkflowRequiresAllowedTransitions) {
    FinalLayerOrchestrator orchestrator;
    auto pkg = makePackage();
    pkg.deployment_stage = FinalLayerDeploymentStage::DRAFT;
    ASSERT_TRUE(orchestrator.registerPackage(pkg));

    EXPECT_FALSE(orchestrator.promotePackage("pkg-legal-qa", FinalLayerDeploymentStage::PRODUCTION));
    EXPECT_TRUE(orchestrator.promotePackage("pkg-legal-qa", FinalLayerDeploymentStage::STAGING));
    EXPECT_TRUE(orchestrator.promotePackage("pkg-legal-qa", FinalLayerDeploymentStage::CANARY));
    EXPECT_TRUE(orchestrator.promotePackage("pkg-legal-qa", FinalLayerDeploymentStage::PRODUCTION));
}

TEST(FinalLayerOrchestratorTest, PromotionCompatibilityGateRejectsIncompatibleAdapter) {
    auto registry = std::make_shared<AdapterRegistry>(nullptr);
    ASSERT_TRUE(registry->registerAdapter(makeAdapter("legal-general", "llama-7b", "llama")));

    FinalLayerOrchestrator orchestrator;
    orchestrator.setAdapterRegistry(registry);

    FinalLayerTransitionPolicy policy;
    policy.require_compatibility_gate = true;
    orchestrator.setTransitionPolicy(policy);

    auto pkg = makePackage();
    pkg.deployment_stage = FinalLayerDeploymentStage::CANARY;
    ASSERT_TRUE(orchestrator.registerPackage(pkg));

    EXPECT_FALSE(orchestrator.promotePackage("pkg-legal-qa",
                                             FinalLayerDeploymentStage::PRODUCTION,
                                             "mistral-7b",
                                             "3.1"));
}

TEST(FinalLayerOrchestratorTest, RollbackDemotesSourceAndActivatesKnownGoodTarget) {
    auto registry = std::make_shared<AdapterRegistry>(nullptr);
    ASSERT_TRUE(registry->registerAdapter(makeAdapter("legal-general", "llama-7b", "llama")));
    ASSERT_TRUE(registry->registerAdapter(makeAdapter("legal-stable", "llama-7b", "llama")));

    FinalLayerOrchestrator orchestrator;
    orchestrator.setAdapterRegistry(registry);

    auto source = makePackage();
    source.package_id = "pkg-active";
    source.primary_adapter_id = "legal-general";
    source.deployment_stage = FinalLayerDeploymentStage::PRODUCTION;

    auto target = makePackage();
    target.package_id = "pkg-rollback";
    target.primary_adapter_id = "legal-stable";
    target.deployment_stage = FinalLayerDeploymentStage::CANARY;

    ASSERT_TRUE(orchestrator.registerPackage(source));
    ASSERT_TRUE(orchestrator.registerPackage(target));

    ASSERT_TRUE(orchestrator.rollbackToPackage("pkg-active", "pkg-rollback"));

    const auto packages = orchestrator.listPackages();
    bool source_ok = false;
    bool target_ok = false;
    for (const auto& pkg : packages) {
        if (pkg.package_id == "pkg-active") {
            source_ok = (pkg.status == FinalLayerPackageStatus::DEPRECATED) &&
                        (pkg.deployment_stage == FinalLayerDeploymentStage::STAGING);
        }
        if (pkg.package_id == "pkg-rollback") {
            target_ok = (pkg.status == FinalLayerPackageStatus::ACTIVE) &&
                        (pkg.deployment_stage == FinalLayerDeploymentStage::PREVIOUS_KNOWN_GOOD);
        }
    }

    EXPECT_TRUE(source_ok);
    EXPECT_TRUE(target_ok);
}

TEST(FinalLayerOrchestratorTest, ResolveRejectsNonServingDeploymentStage) {
    auto registry = std::make_shared<AdapterRegistry>(nullptr);
    ASSERT_TRUE(registry->registerAdapter(makeAdapter("legal-general", "llama-7b", "llama")));

    FinalLayerOrchestrator orchestrator;
    orchestrator.setAdapterRegistry(registry);

    auto pkg = makePackage();
    pkg.deployment_stage = FinalLayerDeploymentStage::STAGING;
    ASSERT_TRUE(orchestrator.registerPackage(pkg));

    FinalLayerRequest request;
    request.requested_package_id = "pkg-legal-qa";
    request.base_model_name = "llama-7b";
    request.base_model_version = "3.1";

    const auto resolution = orchestrator.resolve(request);
    EXPECT_FALSE(resolution.resolved);
    EXPECT_EQ(resolution.routing_reason_code,
              std::string(themis::observability::reason_codes::final_layer::kPackageNotDeployable));
    EXPECT_EQ(resolution.fallback_reason_code,
              std::string(themis::observability::reason_codes::final_layer::kFallbackPackageNotDeployable));
}

TEST(FinalLayerOrchestratorTest, StructuredLayerHandoffJsonLogEmitted) {
    const auto now_ticks = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto log_path = std::filesystem::temp_directory_path() /
                          ("themis_final_log_" + std::to_string(now_ticks) + ".log");

    themis::utils::Logger::shutdown();
    themis::utils::Logger::init(log_path.string(), themis::utils::Logger::Level::INFO);
    themis::utils::Logger::setPattern("%v");

    auto registry = std::make_shared<AdapterRegistry>(nullptr);
    ASSERT_TRUE(registry->registerAdapter(makeAdapter("legal-general", "llama-7b", "llama")));

    FinalLayerOrchestrator orchestrator;
    orchestrator.setAdapterRegistry(registry);
    ASSERT_TRUE(orchestrator.registerPackage(makePackage()));

    FinalLayerRequest request;
    request.requested_package_id = "pkg-legal-qa";
    request.base_model_name = "llama-7b";
    request.base_model_version = "3.1";
    request.correlation_id = "corr-final-log";
    const auto resolution = orchestrator.resolve(request);
    ASSERT_TRUE(resolution.resolved);

    const auto logger = themis::utils::Logger::get();
    ASSERT_NE(logger, nullptr);
    logger->flush();
    themis::utils::Logger::shutdown();

    std::ifstream in(log_path, std::ios::binary);
    ASSERT_TRUE(in.is_open());
    const std::string content((std::istreambuf_iterator<char>(in)),
                               std::istreambuf_iterator<char>());

    EXPECT_NE(content.find("\"event\":\"layer_handoff_decision\""), std::string::npos);
    EXPECT_NE(content.find("\"layer_name\":\"final_layer\""), std::string::npos);
    EXPECT_NE(content.find("\"correlation_id\":\"corr-final-log\""), std::string::npos);
    EXPECT_NE(content.find("\"routing_reason_code\":"), std::string::npos);
    EXPECT_NE(content.find("\"resolved\":true"), std::string::npos);

    std::error_code ec = {};
    std::filesystem::remove(log_path, ec);
}
