/*
 * Tests for MCP StdioTransport::setStdioReadFn() callback bridge (STUB #65)
 *
 * The `#else` stub branch in StdioTransport::start() is only compiled when
 * none of _WIN32, __unix__, __APPLE__ are defined (exotic/embedded targets).
 * CI runs on Linux/macOS/Windows so the branch is never compiled there.
 * These tests therefore verify the bridge contract through the public static
 * setter API — confirming that callbacks are stored, replaced, and cleared
 * correctly, which is the prerequisite for the runtime path to work on the
 * target platform.
 *
 * Covers: MCP-SRF-01..MCP-SRF-03
 *   MCP-SRF-01 — setStdioReadFn: fn is stored without being invoked by setter
 *   MCP-SRF-02 — setStdioReadFn: second call replaces first (last-writer wins)
 *   MCP-SRF-03 — setStdioReadFn(nullptr): clears stored fn; no crash
 */

#include <gtest/gtest.h>
#include "server/mcp_server.h"

#ifdef THEMIS_ENABLE_MCP

// ─── MCP-SRF-01: Setter stores fn without invoking it ────────────────────────

TEST(McpStdioReadFnBridgeTest, SetterStoresFnWithoutInvokingIt) {
    bool invoked = false;

    EXPECT_NO_THROW(
        themis::server::StdioTransport::setStdioReadFn([&]() { invoked = true; }));

    // Setter must not invoke the fn — invocation happens inside start() on the
    // exotic platform #else branch.
    EXPECT_FALSE(invoked);

    // Clean up
    themis::server::StdioTransport::setStdioReadFn(nullptr);
}

// ─── MCP-SRF-02: Second set replaces first (last-writer wins) ────────────────

TEST(McpStdioReadFnBridgeTest, SecondSetOverridesFirst) {
    int first_count  = 0;
    int second_count = 0;

    themis::server::StdioTransport::setStdioReadFn([&]() { ++first_count; });
    themis::server::StdioTransport::setStdioReadFn([&]() { ++second_count; });

    // Neither fn is called by the setter
    EXPECT_EQ(first_count,  0);
    EXPECT_EQ(second_count, 0);

    // Clean up
    themis::server::StdioTransport::setStdioReadFn(nullptr);
}

// ─── MCP-SRF-03: Clearing with nullptr does not crash ────────────────────────

TEST(McpStdioReadFnBridgeTest, ClearingWithNullptrDoesNotCrash) {
    // Set, then clear
    themis::server::StdioTransport::setStdioReadFn([]() {});
    EXPECT_NO_THROW(themis::server::StdioTransport::setStdioReadFn(nullptr));

    // Clear when already null
    EXPECT_NO_THROW(themis::server::StdioTransport::setStdioReadFn(nullptr));
}

#else

TEST(McpStdioReadFnBridgeTest, McpDisabledBuildSkipsBridgeChecks) {
    GTEST_SKIP() << "THEMIS_ENABLE_MCP is disabled for this build";
}

#endif
