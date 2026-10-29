#include <gtest/gtest.h>

#include "core/concerns/inmemory_secrets.h"
#include "core/concerns/metric_labels.h"

#include <string>

namespace {

using themis::core::concerns::InMemorySecrets;
using themis::core::concerns::MetricLabels;

TEST(CoreConcernsBoundaryTest, LabelInputsAndExportedSnapshotsHaveIndependentLifetimes) {
    MetricLabels labels;
    std::string key = "result";
    std::string value = "success";
    labels.add(key, value);
    key.assign("changed");
    value.assign("failure");
    EXPECT_EQ(labels.toLabels().at("result"), "success");

    auto snapshot = labels.toLabels();
    labels.add("result", "updated");
    EXPECT_EQ(labels.size(), 1u);
    EXPECT_EQ(snapshot.at("result"), "success");
    snapshot["result"] = "local";
    EXPECT_EQ(labels.toLabels().at("result"), "updated");
}

TEST(CoreConcernsBoundaryTest, EmptySecretIsPresentAndRetrievedValueSurvivesRemoval) {
    InMemorySecrets secrets;
    secrets.setSecret("test-entry", "");
    EXPECT_TRUE(secrets.hasSecret("test-entry"));
    const auto empty = secrets.getSecret("test-entry");
    ASSERT_TRUE(empty.has_value());
    EXPECT_TRUE(empty->empty());

    secrets.setSecret("test-entry", "test-value");
    const auto snapshot = secrets.getSecret("test-entry");
    ASSERT_TRUE(snapshot.has_value());
    EXPECT_EQ(secrets.size(), 1u);
    EXPECT_TRUE(secrets.removeSecret("test-entry"));
    EXPECT_FALSE(secrets.removeSecret("test-entry"));
    EXPECT_FALSE(secrets.getSecret("test-entry").has_value());
    EXPECT_EQ(*snapshot, "test-value");
    EXPECT_TRUE(secrets.listSecretNames().empty());
}

} // namespace
