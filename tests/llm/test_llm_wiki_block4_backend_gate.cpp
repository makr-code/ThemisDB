// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include "wikipedia/llm_wiki_plugin_impl.h"

namespace {

using themis::plugins::llm_wiki::LLMWikiPluginImpl;
using themis::plugins::llm_wiki::Status;

#ifndef THEMISDB_LLM_WIKI_ENTERPRISE_ENABLED
TEST(LLMWikiBlock4BackendGateTest, InitializeDeniedWhenEditionGateClosed) {
    LLMWikiPluginImpl plugin;
    const Status st = plugin.initialize(std::string("{}"));
    EXPECT_EQ(st.code, Status::Code::PermissionDenied);
}
#else
TEST(LLMWikiBlock4BackendGateTest, RocksDbConfiguredFailsClosedByDefault) {
    LLMWikiPluginImpl plugin;
    const std::string cfg =
        R"({"rocksdb_dir":"/proc/themisdb_block4_wiki_store"})";
    const Status st = plugin.initialize(cfg);
    EXPECT_FALSE(st.ok());
    EXPECT_EQ(st.code, Status::Code::Error);
}

TEST(LLMWikiBlock4BackendGateTest, RocksDbConfiguredAllowsExplicitDegradedMode) {
    LLMWikiPluginImpl plugin;
    const std::string cfg =
        R"({"rocksdb_dir":"/proc/themisdb_block4_wiki_store","fail_open":true})";
    const Status st = plugin.initialize(cfg);
    ASSERT_TRUE(st.ok()) << st.message;

    const auto stats = plugin.stats({});
    EXPECT_FALSE(stats.rocksdb_backed);
}

TEST(LLMWikiBlock4BackendGateTest, WikipediaDumpRequiresRuntimeLicenseFlag) {
    LLMWikiPluginImpl plugin;
    ASSERT_TRUE(plugin.initialize(std::string("{}")).ok());

    const auto res = plugin.ingestWikipediaDump("/tmp/nonexistent_dump.xml.bz2", {});
    EXPECT_GT(res.errors, 0);
    ASSERT_FALSE(res.failed_files.empty());
    EXPECT_NE(res.failed_files.front().find("permission_denied"), std::string::npos);
}
#endif

}  // namespace
