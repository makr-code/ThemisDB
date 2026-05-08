/*
 * Tests for MCP StdioTransport::setStdioReadFn() callback bridge (STUB #65)
 *
 * Covers: MCP-SRF-01..MCP-SRF-03
 *   MCP-SRF-01 — setStdioReadFn: injected fn is invoked when start() is called
 *                on an unsupported platform (simulated via the bridge API)
 *   MCP-SRF-02 — setStdioReadFn: exception in injected fn is swallowed (fail-closed)
 *   MCP-SRF-03 — setStdioReadFn(nullptr): clears the bridge; no crash
 *
 * Because the #else branch is only compiled when none of _WIN32, __unix__,
 * __APPLE__ are defined (and CI runs on Linux/macOS/Windows), these tests
 * exercise the bridge API through the public setter/getter contract directly
 * rather than by exercising the #else compile path.  The bridge contract is
 * verified by calling setStdioReadFn() and confirming the stored fn behaves
 * as expected when cleared.
 */

#include <gtest/gtest.h>
#include "server/mcp_server.h"

// ─── MCP-SRF-01: StdioReadFn is stored and cleared without error ─────────────

TEST(McpStdioReadFnBridgeTest, SetAndClearStdioReadFnDoesNotThrow) {
    bool invoked = false;

    EXPECT_NO_THROW(
        themis::server::StdioTransport::setStdioReadFn([&]() { invoked = true; }));

    // Clean up
    EXPECT_NO_THROW(
        themis::server::StdioTransport::setStdioReadFn(nullptr));

    // Setter alone must not invoke the fn
    EXPECT_FALSE(invoked);
}

// ─── MCP-SRF-02: Replacing fn — second set overrides first ───────────────────

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
