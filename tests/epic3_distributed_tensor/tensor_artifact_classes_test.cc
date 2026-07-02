/// @file tensor_artifact_classes_test.cc
/// @brief Comprehensive tests for tensor artifact classification and lifecycle
/// @author ThemisDB EPIC 3 Implementation Team
/// @date 2026-07-01

#include "src/distributed_tensor/include/tensor_artifact_classes.h"

#include <gtest/gtest.h>
#include <memory>

namespace themis {
namespace distributed_tensor {

// ============================================================================
// Test Fixtures
// ============================================================================

class ArtifactLifecyclePolicyTest : public ::testing::Test {
 protected:
  ArtifactLifecyclePolicyTest() = default;
};

class ArtifactClassifierTest : public ::testing::Test {
 protected:
  ArtifactClassifierTest() = default;
};

class InMemoryArtifactRegistryTest : public ::testing::Test {
 protected:
  void SetUp() override {
    registry_ = std::make_unique<InMemoryArtifactRegistry>();
  }

  std::unique_ptr<InMemoryArtifactRegistry> registry_;
};

// ============================================================================
// ArtifactLifecyclePolicy Tests
// ============================================================================

TEST_F(ArtifactLifecyclePolicyTest, ValidTransitionsFromCreated) {
  EXPECT_TRUE(ArtifactLifecyclePolicy::isValidTransition(
      LifecycleState::CREATED, LifecycleState::ACTIVE));
  EXPECT_TRUE(ArtifactLifecyclePolicy::isValidTransition(
      LifecycleState::CREATED, LifecycleState::INVALIDATED));
  EXPECT_FALSE(ArtifactLifecyclePolicy::isValidTransition(
      LifecycleState::CREATED, LifecycleState::STALE));
  EXPECT_FALSE(ArtifactLifecyclePolicy::isValidTransition(
      LifecycleState::CREATED, LifecycleState::DELETED));
}

TEST_F(ArtifactLifecyclePolicyTest, ValidTransitionsFromActive) {
  EXPECT_TRUE(ArtifactLifecyclePolicy::isValidTransition(
      LifecycleState::ACTIVE, LifecycleState::STALE));
  EXPECT_TRUE(ArtifactLifecyclePolicy::isValidTransition(
      LifecycleState::ACTIVE, LifecycleState::INVALIDATED));
  EXPECT_TRUE(ArtifactLifecyclePolicy::isValidTransition(
      LifecycleState::ACTIVE, LifecycleState::DELETED));
  EXPECT_FALSE(ArtifactLifecyclePolicy::isValidTransition(
      LifecycleState::ACTIVE, LifecycleState::CREATED));
  EXPECT_FALSE(ArtifactLifecyclePolicy::isValidTransition(
      LifecycleState::ACTIVE, LifecycleState::REBUILT));
}

TEST_F(ArtifactLifecyclePolicyTest, ValidTransitionsFromStale) {
  EXPECT_TRUE(ArtifactLifecyclePolicy::isValidTransition(
      LifecycleState::STALE, LifecycleState::REBUILT));
  EXPECT_TRUE(ArtifactLifecyclePolicy::isValidTransition(
      LifecycleState::STALE, LifecycleState::INVALIDATED));
  EXPECT_TRUE(ArtifactLifecyclePolicy::isValidTransition(
      LifecycleState::STALE, LifecycleState::DELETED));
  EXPECT_FALSE(ArtifactLifecyclePolicy::isValidTransition(
      LifecycleState::STALE, LifecycleState::ACTIVE));
}

TEST_F(ArtifactLifecyclePolicyTest, ValidTransitionsFromInvalidated) {
  EXPECT_TRUE(ArtifactLifecyclePolicy::isValidTransition(
      LifecycleState::INVALIDATED, LifecycleState::REBUILT));
  EXPECT_TRUE(ArtifactLifecyclePolicy::isValidTransition(
      LifecycleState::INVALIDATED, LifecycleState::DELETED));
  EXPECT_FALSE(ArtifactLifecyclePolicy::isValidTransition(
      LifecycleState::INVALIDATED, LifecycleState::ACTIVE));
  EXPECT_FALSE(ArtifactLifecyclePolicy::isValidTransition(
      LifecycleState::INVALIDATED, LifecycleState::STALE));
}

TEST_F(ArtifactLifecyclePolicyTest, ValidTransitionsFromRebuilt) {
  EXPECT_TRUE(ArtifactLifecyclePolicy::isValidTransition(
      LifecycleState::REBUILT, LifecycleState::ACTIVE));
  EXPECT_FALSE(ArtifactLifecyclePolicy::isValidTransition(
      LifecycleState::REBUILT, LifecycleState::STALE));
  EXPECT_FALSE(ArtifactLifecyclePolicy::isValidTransition(
      LifecycleState::REBUILT, LifecycleState::INVALIDATED));
  EXPECT_FALSE(ArtifactLifecyclePolicy::isValidTransition(
      LifecycleState::REBUILT, LifecycleState::DELETED));
}

TEST_F(ArtifactLifecyclePolicyTest, DeletedIsTerminal) {
  EXPECT_FALSE(ArtifactLifecyclePolicy::isValidTransition(
      LifecycleState::DELETED, LifecycleState::ACTIVE));
  EXPECT_FALSE(ArtifactLifecyclePolicy::isValidTransition(
      LifecycleState::DELETED, LifecycleState::STALE));
  EXPECT_FALSE(ArtifactLifecyclePolicy::isValidTransition(
      LifecycleState::DELETED, LifecycleState::INVALIDATED));
  EXPECT_FALSE(ArtifactLifecyclePolicy::isValidTransition(
      LifecycleState::DELETED, LifecycleState::REBUILT));
}

TEST_F(ArtifactLifecyclePolicyTest, StateToString) {
  EXPECT_EQ(ArtifactLifecyclePolicy::stateToString(LifecycleState::CREATED), "CREATED");
  EXPECT_EQ(ArtifactLifecyclePolicy::stateToString(LifecycleState::ACTIVE), "ACTIVE");
  EXPECT_EQ(ArtifactLifecyclePolicy::stateToString(LifecycleState::STALE), "STALE");
  EXPECT_EQ(ArtifactLifecyclePolicy::stateToString(LifecycleState::INVALIDATED),
            "INVALIDATED");
  EXPECT_EQ(ArtifactLifecyclePolicy::stateToString(LifecycleState::REBUILT), "REBUILT");
  EXPECT_EQ(ArtifactLifecyclePolicy::stateToString(LifecycleState::DELETED), "DELETED");
}

TEST_F(ArtifactLifecyclePolicyTest, StringToState) {
  auto created = ArtifactLifecyclePolicy::stringToState("CREATED");
  ASSERT_TRUE(created.has_value());
  EXPECT_EQ(created.value(), LifecycleState::CREATED);

  auto active = ArtifactLifecyclePolicy::stringToState("ACTIVE");
  ASSERT_TRUE(active.has_value());
  EXPECT_EQ(active.value(), LifecycleState::ACTIVE);

  auto invalid = ArtifactLifecyclePolicy::stringToState("INVALID_STATE");
  EXPECT_FALSE(invalid.has_value());
}

TEST_F(ArtifactLifecyclePolicyTest, IsUsable) {
  EXPECT_TRUE(ArtifactLifecyclePolicy::isUsable(LifecycleState::ACTIVE));
  EXPECT_TRUE(ArtifactLifecyclePolicy::isUsable(LifecycleState::STALE));
  EXPECT_FALSE(ArtifactLifecyclePolicy::isUsable(LifecycleState::CREATED));
  EXPECT_FALSE(ArtifactLifecyclePolicy::isUsable(LifecycleState::INVALIDATED));
  EXPECT_FALSE(ArtifactLifecyclePolicy::isUsable(LifecycleState::REBUILT));
  EXPECT_FALSE(ArtifactLifecyclePolicy::isUsable(LifecycleState::DELETED));
}

TEST_F(ArtifactLifecyclePolicyTest, IsTerminal) {
  EXPECT_TRUE(ArtifactLifecyclePolicy::isTerminal(LifecycleState::DELETED));
  EXPECT_FALSE(ArtifactLifecyclePolicy::isTerminal(LifecycleState::CREATED));
  EXPECT_FALSE(ArtifactLifecyclePolicy::isTerminal(LifecycleState::ACTIVE));
  EXPECT_FALSE(ArtifactLifecyclePolicy::isTerminal(LifecycleState::STALE));
  EXPECT_FALSE(ArtifactLifecyclePolicy::isTerminal(LifecycleState::INVALIDATED));
  EXPECT_FALSE(ArtifactLifecyclePolicy::isTerminal(LifecycleState::REBUILT));
}

TEST_F(ArtifactLifecyclePolicyTest, RequiresVerification) {
  EXPECT_TRUE(ArtifactLifecyclePolicy::requiresVerification(LifecycleState::CREATED));
  EXPECT_TRUE(ArtifactLifecyclePolicy::requiresVerification(LifecycleState::REBUILT));
  EXPECT_FALSE(ArtifactLifecyclePolicy::requiresVerification(LifecycleState::ACTIVE));
  EXPECT_FALSE(ArtifactLifecyclePolicy::requiresVerification(LifecycleState::STALE));
  EXPECT_FALSE(ArtifactLifecyclePolicy::requiresVerification(LifecycleState::INVALIDATED));
  EXPECT_FALSE(ArtifactLifecyclePolicy::requiresVerification(LifecycleState::DELETED));
}

// ============================================================================
// ArtifactClassifier Tests
// ============================================================================

TEST_F(ArtifactClassifierTest, ClassToString) {
  EXPECT_EQ(ArtifactClassifier::classToString(ArtifactClass::PRIMARY), "PRIMARY");
  EXPECT_EQ(ArtifactClassifier::classToString(ArtifactClass::DERIVED), "DERIVED");
  EXPECT_EQ(ArtifactClassifier::classToString(ArtifactClass::EPHEMERAL), "EPHEMERAL");
  EXPECT_EQ(ArtifactClassifier::classToString(ArtifactClass::ADVISORY_ONLY), "ADVISORY_ONLY");
}

TEST_F(ArtifactClassifierTest, StringToClass) {
  auto primary = ArtifactClassifier::stringToClass("PRIMARY");
  ASSERT_TRUE(primary.has_value());
  EXPECT_EQ(primary.value(), ArtifactClass::PRIMARY);

  auto derived = ArtifactClassifier::stringToClass("DERIVED");
  ASSERT_TRUE(derived.has_value());
  EXPECT_EQ(derived.value(), ArtifactClass::DERIVED);

  auto invalid = ArtifactClassifier::stringToClass("INVALID_CLASS");
  EXPECT_FALSE(invalid.has_value());
}

TEST_F(ArtifactClassifierTest, SemanticToString) {
  EXPECT_EQ(ArtifactClassifier::semanticToString(TruthSemantic::SOURCE_OF_TRUTH),
            "SOURCE_OF_TRUTH");
  EXPECT_EQ(ArtifactClassifier::semanticToString(TruthSemantic::TRUTH_ADJACENT),
            "TRUTH_ADJACENT");
  EXPECT_EQ(ArtifactClassifier::semanticToString(TruthSemantic::ADVISORY), "ADVISORY");
}

TEST_F(ArtifactClassifierTest, StringToSemantic) {
  auto sot = ArtifactClassifier::stringToSemantic("SOURCE_OF_TRUTH");
  ASSERT_TRUE(sot.has_value());
  EXPECT_EQ(sot.value(), TruthSemantic::SOURCE_OF_TRUTH);

  auto ta = ArtifactClassifier::stringToSemantic("TRUTH_ADJACENT");
  ASSERT_TRUE(ta.has_value());
  EXPECT_EQ(ta.value(), TruthSemantic::TRUTH_ADJACENT);

  auto invalid = ArtifactClassifier::stringToSemantic("INVALID_SEMANTIC");
  EXPECT_FALSE(invalid.has_value());
}

TEST_F(ArtifactClassifierTest, ValidCombinations) {
  // Primary artifacts: SOURCE_OF_TRUTH or TRUTH_ADJACENT
  EXPECT_TRUE(ArtifactClassifier::isValidCombination(ArtifactClass::PRIMARY,
                                                      TruthSemantic::SOURCE_OF_TRUTH));
  EXPECT_TRUE(ArtifactClassifier::isValidCombination(ArtifactClass::PRIMARY,
                                                      TruthSemantic::TRUTH_ADJACENT));
  EXPECT_FALSE(ArtifactClassifier::isValidCombination(ArtifactClass::PRIMARY,
                                                       TruthSemantic::ADVISORY));

  // Derived artifacts: TRUTH_ADJACENT or ADVISORY
  EXPECT_TRUE(ArtifactClassifier::isValidCombination(ArtifactClass::DERIVED,
                                                      TruthSemantic::TRUTH_ADJACENT));
  EXPECT_TRUE(
      ArtifactClassifier::isValidCombination(ArtifactClass::DERIVED, TruthSemantic::ADVISORY));
  EXPECT_FALSE(ArtifactClassifier::isValidCombination(ArtifactClass::DERIVED,
                                                       TruthSemantic::SOURCE_OF_TRUTH));

  // Ephemeral: any semantic
  EXPECT_TRUE(ArtifactClassifier::isValidCombination(ArtifactClass::EPHEMERAL,
                                                      TruthSemantic::SOURCE_OF_TRUTH));
  EXPECT_TRUE(ArtifactClassifier::isValidCombination(ArtifactClass::EPHEMERAL,
                                                      TruthSemantic::TRUTH_ADJACENT));
  EXPECT_TRUE(
      ArtifactClassifier::isValidCombination(ArtifactClass::EPHEMERAL, TruthSemantic::ADVISORY));

  // AdvisoryOnly: ADVISORY only
  EXPECT_TRUE(ArtifactClassifier::isValidCombination(ArtifactClass::ADVISORY_ONLY,
                                                      TruthSemantic::ADVISORY));
  EXPECT_FALSE(ArtifactClassifier::isValidCombination(ArtifactClass::ADVISORY_ONLY,
                                                       TruthSemantic::SOURCE_OF_TRUTH));
  EXPECT_FALSE(ArtifactClassifier::isValidCombination(ArtifactClass::ADVISORY_ONLY,
                                                       TruthSemantic::TRUTH_ADJACENT));
}

TEST_F(ArtifactClassifierTest, AdvisoryOnlyCheck) {
  // AdvisoryOnly class is always advisory
  EXPECT_TRUE(ArtifactClassifier::isAdvisoryOnly(ArtifactClass::ADVISORY_ONLY,
                                                  TruthSemantic::ADVISORY));

  // Advisory semantic always means advisory
  EXPECT_TRUE(
      ArtifactClassifier::isAdvisoryOnly(ArtifactClass::EPHEMERAL, TruthSemantic::ADVISORY));
  EXPECT_TRUE(ArtifactClassifier::isAdvisoryOnly(ArtifactClass::DERIVED, TruthSemantic::ADVISORY));

  // Truth-bearing combinations are not advisory
  EXPECT_FALSE(ArtifactClassifier::isAdvisoryOnly(ArtifactClass::PRIMARY,
                                                   TruthSemantic::SOURCE_OF_TRUTH));
  EXPECT_FALSE(ArtifactClassifier::isAdvisoryOnly(ArtifactClass::DERIVED,
                                                   TruthSemantic::TRUTH_ADJACENT));
}

TEST_F(ArtifactClassifierTest, TruthBearingCheck) {
  // Source-of-truth is always truth-bearing
  EXPECT_TRUE(ArtifactClassifier::isTruthBearing(ArtifactClass::PRIMARY,
                                                  TruthSemantic::SOURCE_OF_TRUTH));

  // Truth-adjacent is truth-bearing (except for advisory-only)
  EXPECT_TRUE(ArtifactClassifier::isTruthBearing(ArtifactClass::PRIMARY,
                                                  TruthSemantic::TRUTH_ADJACENT));
  EXPECT_TRUE(ArtifactClassifier::isTruthBearing(ArtifactClass::DERIVED,
                                                  TruthSemantic::TRUTH_ADJACENT));
  EXPECT_TRUE(ArtifactClassifier::isTruthBearing(ArtifactClass::EPHEMERAL,
                                                  TruthSemantic::TRUTH_ADJACENT));

  // Advisory is never truth-bearing
  EXPECT_FALSE(
      ArtifactClassifier::isTruthBearing(ArtifactClass::ADVISORY_ONLY, TruthSemantic::ADVISORY));
  EXPECT_FALSE(
      ArtifactClassifier::isTruthBearing(ArtifactClass::EPHEMERAL, TruthSemantic::ADVISORY));

  // AdvisoryOnly class is never truth-bearing
  EXPECT_FALSE(ArtifactClassifier::isTruthBearing(ArtifactClass::ADVISORY_ONLY,
                                                   TruthSemantic::ADVISORY));
}

// ============================================================================
// InMemoryArtifactRegistry Tests
// ============================================================================

TEST_F(InMemoryArtifactRegistryTest, RegisterAndLookup) {
  ArtifactMetadata metadata;
  metadata.artifact_id = "lora:v1.0:adapter:model-x";
  metadata.artifact_class = ArtifactClass::PRIMARY;
  metadata.lifecycle_state = LifecycleState::CREATED;
  metadata.truth_semantic = TruthSemantic::SOURCE_OF_TRUTH;
  metadata.version = "1.0";
  metadata.content_hash = "abc123";

  EXPECT_TRUE(registry_->registerArtifact(metadata));

  auto lookup = registry_->lookup("lora:v1.0:adapter:model-x");
  ASSERT_TRUE(lookup.has_value());
  EXPECT_EQ(lookup.value().artifact_id, "lora:v1.0:adapter:model-x");
  EXPECT_EQ(lookup.value().artifact_class, ArtifactClass::PRIMARY);
}

TEST_F(InMemoryArtifactRegistryTest, DuplicateRegistration) {
  ArtifactMetadata metadata;
  metadata.artifact_id = "tensor:v1";
  metadata.artifact_class = ArtifactClass::PRIMARY;
  metadata.lifecycle_state = LifecycleState::CREATED;
  metadata.truth_semantic = TruthSemantic::SOURCE_OF_TRUTH;

  EXPECT_TRUE(registry_->registerArtifact(metadata));
  EXPECT_FALSE(registry_->registerArtifact(metadata));  // Duplicate
}

TEST_F(InMemoryArtifactRegistryTest, InvalidClassSemanticCombination) {
  ArtifactMetadata metadata;
  metadata.artifact_id = "invalid:v1";
  metadata.artifact_class = ArtifactClass::PRIMARY;
  metadata.lifecycle_state = LifecycleState::CREATED;
  metadata.truth_semantic = TruthSemantic::ADVISORY;  // Invalid for PRIMARY

  EXPECT_FALSE(registry_->registerArtifact(metadata));
}

TEST_F(InMemoryArtifactRegistryTest, TransitionState) {
  ArtifactMetadata metadata;
  metadata.artifact_id = "tensor:v1";
  metadata.artifact_class = ArtifactClass::PRIMARY;
  metadata.lifecycle_state = LifecycleState::CREATED;
  metadata.truth_semantic = TruthSemantic::SOURCE_OF_TRUTH;

  EXPECT_TRUE(registry_->registerArtifact(metadata));

  // Transition from CREATED to ACTIVE
  EXPECT_TRUE(registry_->transitionState("tensor:v1", LifecycleState::ACTIVE));

  auto lookup = registry_->lookup("tensor:v1");
  ASSERT_TRUE(lookup.has_value());
  EXPECT_EQ(lookup.value().lifecycle_state, LifecycleState::ACTIVE);

  // Invalid transition
  EXPECT_FALSE(registry_->transitionState("tensor:v1", LifecycleState::CREATED));
}

TEST_F(InMemoryArtifactRegistryTest, ListByClass) {
  // Register multiple artifacts with different classes
  ArtifactMetadata primary;
  primary.artifact_id = "primary:v1";
  primary.artifact_class = ArtifactClass::PRIMARY;
  primary.lifecycle_state = LifecycleState::CREATED;
  primary.truth_semantic = TruthSemantic::SOURCE_OF_TRUTH;
  registry_->registerArtifact(primary);

  ArtifactMetadata derived;
  derived.artifact_id = "derived:v1";
  derived.artifact_class = ArtifactClass::DERIVED;
  derived.lifecycle_state = LifecycleState::CREATED;
  derived.truth_semantic = TruthSemantic::TRUTH_ADJACENT;
  registry_->registerArtifact(derived);

  ArtifactMetadata derived2;
  derived2.artifact_id = "derived:v2";
  derived2.artifact_class = ArtifactClass::DERIVED;
  derived2.lifecycle_state = LifecycleState::CREATED;
  derived2.truth_semantic = TruthSemantic::TRUTH_ADJACENT;
  registry_->registerArtifact(derived2);

  auto primary_list = registry_->listByClass(ArtifactClass::PRIMARY);
  EXPECT_EQ(primary_list.size(), 1);
  EXPECT_EQ(primary_list[0], "primary:v1");

  auto derived_list = registry_->listByClass(ArtifactClass::DERIVED);
  EXPECT_EQ(derived_list.size(), 2);
}

TEST_F(InMemoryArtifactRegistryTest, ListByState) {
  ArtifactMetadata active;
  active.artifact_id = "tensor:active";
  active.artifact_class = ArtifactClass::PRIMARY;
  active.lifecycle_state = LifecycleState::ACTIVE;
  active.truth_semantic = TruthSemantic::SOURCE_OF_TRUTH;
  registry_->registerArtifact(active);

  ArtifactMetadata created;
  created.artifact_id = "tensor:created";
  created.artifact_class = ArtifactClass::PRIMARY;
  created.lifecycle_state = LifecycleState::CREATED;
  created.truth_semantic = TruthSemantic::SOURCE_OF_TRUTH;
  registry_->registerArtifact(created);

  auto active_list = registry_->listByState(LifecycleState::ACTIVE);
  EXPECT_EQ(active_list.size(), 1);

  auto created_list = registry_->listByState(LifecycleState::CREATED);
  EXPECT_EQ(created_list.size(), 1);
}

TEST_F(InMemoryArtifactRegistryTest, ListBySemantic) {
  ArtifactMetadata sot;
  sot.artifact_id = "tensor:sot";
  sot.artifact_class = ArtifactClass::PRIMARY;
  sot.lifecycle_state = LifecycleState::CREATED;
  sot.truth_semantic = TruthSemantic::SOURCE_OF_TRUTH;
  registry_->registerArtifact(sot);

  ArtifactMetadata ta;
  ta.artifact_id = "tensor:ta";
  ta.artifact_class = ArtifactClass::DERIVED;
  ta.lifecycle_state = LifecycleState::CREATED;
  ta.truth_semantic = TruthSemantic::TRUTH_ADJACENT;
  registry_->registerArtifact(ta);

  auto sot_list = registry_->listBySemantic(TruthSemantic::SOURCE_OF_TRUTH);
  EXPECT_EQ(sot_list.size(), 1);

  auto ta_list = registry_->listBySemantic(TruthSemantic::TRUTH_ADJACENT);
  EXPECT_EQ(ta_list.size(), 1);
}

TEST_F(InMemoryArtifactRegistryTest, Count) {
  EXPECT_EQ(registry_->count(), 0);

  ArtifactMetadata m1;
  m1.artifact_id = "tensor:v1";
  m1.artifact_class = ArtifactClass::PRIMARY;
  m1.lifecycle_state = LifecycleState::CREATED;
  m1.truth_semantic = TruthSemantic::SOURCE_OF_TRUTH;
  registry_->registerArtifact(m1);
  EXPECT_EQ(registry_->count(), 1);

  ArtifactMetadata m2;
  m2.artifact_id = "tensor:v2";
  m2.artifact_class = ArtifactClass::DERIVED;
  m2.lifecycle_state = LifecycleState::CREATED;
  m2.truth_semantic = TruthSemantic::TRUTH_ADJACENT;
  registry_->registerArtifact(m2);
  EXPECT_EQ(registry_->count(), 2);
}

TEST_F(InMemoryArtifactRegistryTest, Remove) {
  ArtifactMetadata metadata;
  metadata.artifact_id = "tensor:v1";
  metadata.artifact_class = ArtifactClass::PRIMARY;
  metadata.lifecycle_state = LifecycleState::CREATED;
  metadata.truth_semantic = TruthSemantic::SOURCE_OF_TRUTH;

  registry_->registerArtifact(metadata);
  EXPECT_EQ(registry_->count(), 1);

  EXPECT_TRUE(registry_->remove("tensor:v1"));
  EXPECT_EQ(registry_->count(), 0);

  EXPECT_FALSE(registry_->remove("tensor:v1"));  // Already removed
}

TEST_F(InMemoryArtifactRegistryTest, Clear) {
  ArtifactMetadata m1;
  m1.artifact_id = "tensor:v1";
  m1.artifact_class = ArtifactClass::PRIMARY;
  m1.lifecycle_state = LifecycleState::CREATED;
  m1.truth_semantic = TruthSemantic::SOURCE_OF_TRUTH;
  registry_->registerArtifact(m1);

  ArtifactMetadata m2;
  m2.artifact_id = "tensor:v2";
  m2.artifact_class = ArtifactClass::DERIVED;
  m2.lifecycle_state = LifecycleState::CREATED;
  m2.truth_semantic = TruthSemantic::TRUTH_ADJACENT;
  registry_->registerArtifact(m2);

  EXPECT_EQ(registry_->count(), 2);
  registry_->clear();
  EXPECT_EQ(registry_->count(), 0);
}

// ============================================================================
// Integration Tests: Lifecycle Transitions
// ============================================================================

TEST_F(InMemoryArtifactRegistryTest, LifecycleTransitionSequence) {
  // Test a complete lifecycle: CREATED -> ACTIVE -> STALE -> REBUILT -> ACTIVE
  ArtifactMetadata metadata;
  metadata.artifact_id = "tensor:lifecycle-test";
  metadata.artifact_class = ArtifactClass::DERIVED;
  metadata.lifecycle_state = LifecycleState::CREATED;
  metadata.truth_semantic = TruthSemantic::TRUTH_ADJACENT;
  registry_->registerArtifact(metadata);

  // CREATED -> ACTIVE
  EXPECT_TRUE(registry_->transitionState("tensor:lifecycle-test", LifecycleState::ACTIVE));

  // ACTIVE -> STALE
  EXPECT_TRUE(registry_->transitionState("tensor:lifecycle-test", LifecycleState::STALE));

  // STALE -> REBUILT
  EXPECT_TRUE(registry_->transitionState("tensor:lifecycle-test", LifecycleState::REBUILT));

  // REBUILT -> ACTIVE
  EXPECT_TRUE(registry_->transitionState("tensor:lifecycle-test", LifecycleState::ACTIVE));

  auto lookup = registry_->lookup("tensor:lifecycle-test");
  ASSERT_TRUE(lookup.has_value());
  EXPECT_EQ(lookup.value().lifecycle_state, LifecycleState::ACTIVE);
}

TEST_F(InMemoryArtifactRegistryTest, AdvisoryOnlyEnforcement) {
  // Test that AdvisoryOnly artifacts enforce correct semantics
  ArtifactMetadata advisory;
  advisory.artifact_id = "hint:cost-estimate";
  advisory.artifact_class = ArtifactClass::ADVISORY_ONLY;
  advisory.lifecycle_state = LifecycleState::CREATED;
  advisory.truth_semantic = TruthSemantic::ADVISORY;

  EXPECT_TRUE(registry_->registerArtifact(advisory));

  auto lookup = registry_->lookup("hint:cost-estimate");
  ASSERT_TRUE(lookup.has_value());

  // Verify it's flagged as advisory
  EXPECT_TRUE(ArtifactClassifier::isAdvisoryOnly(lookup.value().artifact_class,
                                                  lookup.value().truth_semantic));
  EXPECT_FALSE(
      ArtifactClassifier::isTruthBearing(lookup.value().artifact_class,
                                          lookup.value().truth_semantic));
}

TEST_F(InMemoryArtifactRegistryTest, EphemeralToAdvisoryTransition) {
  // Ephemeral artifacts can be advisory (e.g., session-local routing hints)
  ArtifactMetadata ephemeral;
  ephemeral.artifact_id = "session:routing-hint";
  ephemeral.artifact_class = ArtifactClass::EPHEMERAL;
  ephemeral.lifecycle_state = LifecycleState::CREATED;
  ephemeral.truth_semantic = TruthSemantic::ADVISORY;

  EXPECT_TRUE(registry_->registerArtifact(ephemeral));
  EXPECT_TRUE(registry_->transitionState("session:routing-hint", LifecycleState::ACTIVE));

  auto lookup = registry_->lookup("session:routing-hint");
  ASSERT_TRUE(lookup.has_value());
  EXPECT_TRUE(ArtifactClassifier::isAdvisoryOnly(lookup.value().artifact_class,
                                                  lookup.value().truth_semantic));
}

}  // namespace distributed_tensor
}  // namespace themis
