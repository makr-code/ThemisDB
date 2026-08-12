/*
 * @file test_wire_protocol_ipv6.cpp
 * @brief Unit tests for IPv6 support in WireProtocolServer::Config.
 *
 * Tests validate:
 *  1. Default values for enable_ipv6 and ipv6_dual_stack
 *  2. Setting and reading IPv6 config fields
 *  3. IPv6 address string handling (for connection tracking)
 *  4. Dual-stack configuration semantics
 *  5. Boost.Asio IPv6 address parsing (::, ::1, explicit IPv6)
 *  6. Interaction between enable_ipv6 and the host field
 */

#include <gtest/gtest.h>
#include "network/wire_protocol_server.h"
#include <boost/asio.hpp>
#include <string>

using namespace themis::network;
namespace net = boost::asio;

// ============================================================================
// Config defaults
// ============================================================================

TEST(WireProtocolIPv6Config, IPv6DisabledByDefault) {
    WireProtocolServer::Config cfg;
    EXPECT_FALSE(cfg.enable_ipv6);
}

TEST(WireProtocolIPv6Config, DualStackEnabledByDefault) {
    WireProtocolServer::Config cfg;
    EXPECT_TRUE(cfg.ipv6_dual_stack);
}

TEST(WireProtocolIPv6Config, DefaultHostIsIPv4Any) {
    WireProtocolServer::Config cfg;
    EXPECT_EQ(cfg.host, "0.0.0.0");
}

// ============================================================================
// Config field assignment
// ============================================================================

TEST(WireProtocolIPv6Config, EnableIPv6CanBeSet) {
    WireProtocolServer::Config cfg;
    cfg.enable_ipv6 = true;
    EXPECT_TRUE(cfg.enable_ipv6);
}

TEST(WireProtocolIPv6Config, DualStackCanBeDisabled) {
    WireProtocolServer::Config cfg;
    cfg.enable_ipv6 = true;
    cfg.ipv6_dual_stack = false;
    EXPECT_TRUE(cfg.enable_ipv6);
    EXPECT_FALSE(cfg.ipv6_dual_stack);
}

TEST(WireProtocolIPv6Config, ExplicitIPv6HostCanBeSet) {
    WireProtocolServer::Config cfg;
    cfg.host = "::";
    cfg.enable_ipv6 = true;
    EXPECT_EQ(cfg.host, "::");
    EXPECT_TRUE(cfg.enable_ipv6);
}

TEST(WireProtocolIPv6Config, LoopbackIPv6HostCanBeSet) {
    WireProtocolServer::Config cfg;
    cfg.host = "::1";
    cfg.enable_ipv6 = true;
    EXPECT_EQ(cfg.host, "::1");
}

TEST(WireProtocolIPv6Config, FullIPv6AddressCanBeSet) {
    WireProtocolServer::Config cfg;
    cfg.host = "fe80::1";
    cfg.enable_ipv6 = true;
    EXPECT_EQ(cfg.host, "fe80::1");
}

// ============================================================================
// Boost.Asio IPv6 address parsing – validates that the addresses we
// store in Config can be successfully parsed by the networking stack.
// ============================================================================

TEST(WireProtocolIPv6AddressParsing, IPv4AnyParses) {
    boost::system::error_code ec;
    auto addr = net::ip::make_address("0.0.0.0", ec);
    EXPECT_FALSE(ec) << ec.message();
    EXPECT_TRUE(addr.is_v4());
    EXPECT_EQ(addr, net::ip::address_v4::any());
}

TEST(WireProtocolIPv6AddressParsing, IPv6AnyParses) {
    boost::system::error_code ec;
    auto addr = net::ip::make_address("::", ec);
    EXPECT_FALSE(ec) << ec.message();
    EXPECT_TRUE(addr.is_v6());
    EXPECT_EQ(addr, net::ip::address_v6::any());
}

TEST(WireProtocolIPv6AddressParsing, IPv6LoopbackParses) {
    boost::system::error_code ec;
    auto addr = net::ip::make_address("::1", ec);
    EXPECT_FALSE(ec) << ec.message();
    EXPECT_TRUE(addr.is_v6());
    EXPECT_EQ(addr, net::ip::address_v6::loopback());
}

TEST(WireProtocolIPv6AddressParsing, IPv4LoopbackParses) {
    boost::system::error_code ec;
    auto addr = net::ip::make_address("127.0.0.1", ec);
    EXPECT_FALSE(ec) << ec.message();
    EXPECT_TRUE(addr.is_v4());
}

TEST(WireProtocolIPv6AddressParsing, InvalidAddressReturnsError) {
    boost::system::error_code ec;
    net::ip::make_address("not-an-ip", ec);
    EXPECT_TRUE(ec);
}

// ============================================================================
// Dual-stack semantics (logic mirrors start() in wire_protocol_server.cpp)
// ============================================================================

namespace {

/// Replicate the address-selection logic from WireProtocolServer::start().
/// Returns the net::ip::address that the acceptor would be bound to.
net::ip::address resolveBindAddress(const WireProtocolServer::Config& cfg) {
    boost::system::error_code ec;
    net::ip::address addr = net::ip::make_address(cfg.host, ec);
    if (ec) {
        return cfg.enable_ipv6
            ? net::ip::address(net::ip::address_v6::any())
            : net::ip::address(net::ip::address_v4::any());
    }
    if (cfg.enable_ipv6 && addr == net::ip::address_v4::any()) {
        return net::ip::address(net::ip::address_v6::any());
    }
    return addr;
}

} // anonymous namespace

TEST(WireProtocolIPv6BindLogic, DefaultConfigBindsIPv4Any) {
    WireProtocolServer::Config cfg;
    auto addr = resolveBindAddress(cfg);
    EXPECT_TRUE(addr.is_v4());
    EXPECT_EQ(addr, net::ip::address_v4::any());
}

TEST(WireProtocolIPv6BindLogic, EnableIPv6PromotesDefaultHostToIPv6Any) {
    WireProtocolServer::Config cfg;
    cfg.enable_ipv6 = true;
    // host is still "0.0.0.0" (default) – must be promoted to "::"
    auto addr = resolveBindAddress(cfg);
    EXPECT_TRUE(addr.is_v6());
    EXPECT_EQ(addr, net::ip::address_v6::any());
}

TEST(WireProtocolIPv6BindLogic, ExplicitIPv6HostUsedDirectly) {
    WireProtocolServer::Config cfg;
    cfg.host = "::1";
    cfg.enable_ipv6 = true;
    auto addr = resolveBindAddress(cfg);
    EXPECT_TRUE(addr.is_v6());
    EXPECT_EQ(addr, net::ip::address_v6::loopback());
}

TEST(WireProtocolIPv6BindLogic, ExplicitIPv6AnyHostUsedDirectly) {
    WireProtocolServer::Config cfg;
    cfg.host = "::";
    cfg.enable_ipv6 = true;
    auto addr = resolveBindAddress(cfg);
    EXPECT_TRUE(addr.is_v6());
    EXPECT_EQ(addr, net::ip::address_v6::any());
}

TEST(WireProtocolIPv6BindLogic, IPv6HostWithoutFlagStillBindsIPv6) {
    // Even when enable_ipv6=false, an explicit IPv6 address in host is used.
    WireProtocolServer::Config cfg;
    cfg.host = "::1";
    auto addr = resolveBindAddress(cfg);
    EXPECT_TRUE(addr.is_v6());
}

TEST(WireProtocolIPv6BindLogic, ExplicitIPv4AddressNotPromoted) {
    // An explicit IPv4 address is kept even when enable_ipv6=true.
    WireProtocolServer::Config cfg;
    cfg.host = "127.0.0.1";
    cfg.enable_ipv6 = true;
    auto addr = resolveBindAddress(cfg);
    EXPECT_TRUE(addr.is_v4());
}

TEST(WireProtocolIPv6BindLogic, UnparseableHostFallsBackToIPv6WhenFlagSet) {
    WireProtocolServer::Config cfg;
    cfg.host = "";  // empty – not parseable
    cfg.enable_ipv6 = true;
    auto addr = resolveBindAddress(cfg);
    EXPECT_TRUE(addr.is_v6());
    EXPECT_EQ(addr, net::ip::address_v6::any());
}

TEST(WireProtocolIPv6BindLogic, UnparseableHostFallsBackToIPv4WhenFlagUnset) {
    WireProtocolServer::Config cfg;
    cfg.host = "";
    cfg.enable_ipv6 = false;
    auto addr = resolveBindAddress(cfg);
    EXPECT_TRUE(addr.is_v4());
    EXPECT_EQ(addr, net::ip::address_v4::any());
}

// ============================================================================
// IPv6 address string representation (connection-tracking keys)
// ============================================================================

TEST(WireProtocolIPv6AddressString, IPv6AnyToString) {
    auto addr = net::ip::address(net::ip::address_v6::any());
    EXPECT_EQ(addr.to_string(), "::");
}

TEST(WireProtocolIPv6AddressString, IPv6LoopbackToString) {
    auto addr = net::ip::address(net::ip::address_v6::loopback());
    EXPECT_EQ(addr.to_string(), "::1");
}

TEST(WireProtocolIPv6AddressString, IPv4AnyToString) {
    auto addr = net::ip::address(net::ip::address_v4::any());
    EXPECT_EQ(addr.to_string(), "0.0.0.0");
}

TEST(WireProtocolIPv6AddressString, IPv6AddressUsableAsMapKey) {
    // Verifies that IPv6 address strings (which contain colons) can be
    // used as keys in the unordered_map used by connection tracking.
    std::unordered_map<std::string, uint32_t> conn_map;
    std::string ipv6_key = "::1";
    conn_map[ipv6_key]++;
    EXPECT_EQ(conn_map[ipv6_key], 1u);
    conn_map[ipv6_key]++;
    EXPECT_EQ(conn_map[ipv6_key], 2u);
}

TEST(WireProtocolIPv6AddressString, FullIPv6AddressAsMapKey) {
    std::unordered_map<std::string, uint32_t> conn_map;
    std::string key = "2001:db8::1";
    conn_map[key] = 5;
    EXPECT_EQ(conn_map[key], 5u);
}
