/**
 * @file test_scraper_v12_features_focused.cpp
 * @brief Focused contract tests for scraper plugin public API.
 */

#include <gtest/gtest.h>

#include "scraper/scraper_plugin.h"

using namespace themis::scraper;

TEST(ScraperV12Contract, PluginStartsUninitialized) {
    ScraperPlugin plugin;
    EXPECT_FALSE(plugin.isInitialized());
}

TEST(ScraperV12Contract, ResetKeepsPluginInSafeState) {
    ScraperPlugin plugin;
    plugin.reset();
    EXPECT_FALSE(plugin.isInitialized());
    EXPECT_TRUE(plugin.getResults().empty());
}
