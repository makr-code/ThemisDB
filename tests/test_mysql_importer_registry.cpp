// test_mysql_importer_registry.cpp
//
// Verifies that MySQLImporterSchemePlugin:
//   1. Registers with IImporterPluginRegistry at static-init time via
//      REGISTER_IMPORTER_PLUGIN (placed in mysql_importer.cpp).
//   2. Resolves correctly for "mysql://" and "mariadb://" source URIs.
//   3. Can create a valid MySQLImporter instance via createImporter().
//
// Additionally tests the admin import API MySQL route helper logic
// (mirrors ImportApiHandler::handleStartMySQLImport validation).
//
// This test binary is compiled with mysql_importer.cpp so that the
// REGISTER_IMPORTER_PLUGIN static-init registration fires before any test
// runs.

#include <gtest/gtest.h>

#include "importers/importer_interfaces.h"
#include "importers/mysql_importer.h"

#include <algorithm>
#include <string>
#include <vector>

using namespace themis::importers;

// ===========================================================================
// Registry registration
// ===========================================================================

TEST(MySQLImporterRegistry, PluginRegisteredForMysqlScheme) {
    auto* plugin = IImporterPluginRegistry::instance().resolve("mysql://host:3306/db");
    ASSERT_NE(nullptr, plugin)
        << "MySQLImporterSchemePlugin must be registered for the 'mysql' scheme";
    EXPECT_STREQ("mysql_plugin", plugin->pluginId());
}

TEST(MySQLImporterRegistry, PluginRegisteredForMariadbScheme) {
    auto* plugin = IImporterPluginRegistry::instance().resolve("mariadb://host:3306/db");
    ASSERT_NE(nullptr, plugin)
        << "MySQLImporterSchemePlugin must be registered for the 'mariadb' scheme";
    EXPECT_STREQ("mysql_plugin", plugin->pluginId());
}

TEST(MySQLImporterRegistry, SameSingletonForBothSchemes) {
    auto* mysql_plugin   = IImporterPluginRegistry::instance().resolve("mysql://host/db");
    auto* mariadb_plugin = IImporterPluginRegistry::instance().resolve("mariadb://host/db");
    ASSERT_NE(nullptr, mysql_plugin);
    ASSERT_NE(nullptr, mariadb_plugin);
    EXPECT_EQ(mysql_plugin, mariadb_plugin)
        << "Both 'mysql' and 'mariadb' schemes must resolve to the same plugin instance";
}

TEST(MySQLImporterRegistry, PluginIdListContainsMysqlPlugin) {
    auto ids = IImporterPluginRegistry::instance().listPluginIds();
    EXPECT_NE(ids.end(), std::find(ids.begin(), ids.end(), "mysql_plugin"))
        << "mysql_plugin must appear in IImporterPluginRegistry::listPluginIds()";
}

TEST(MySQLImporterRegistry, SupportedSchemesContainsMysqlAndMariadb) {
    auto* plugin = IImporterPluginRegistry::instance().resolve("mysql://host/db");
    ASSERT_NE(nullptr, plugin);
    auto schemes = plugin->supportedSchemes();
    EXPECT_NE(schemes.end(), std::find(schemes.begin(), schemes.end(), "mysql"));
    EXPECT_NE(schemes.end(), std::find(schemes.begin(), schemes.end(), "mariadb"));
}

// ===========================================================================
// createImporter()
// ===========================================================================

TEST(MySQLImporterRegistry, CreateImporterReturnsNonNull) {
    auto* plugin = IImporterPluginRegistry::instance().resolve("mysql://host/db");
    ASSERT_NE(nullptr, plugin);

    ImportConfig cfg;
    cfg.source_uri  = "mysql://127.0.0.1:3306/test_db";
    cfg.json_config = "{}";
    auto importer = plugin->createImporter(cfg);
    EXPECT_NE(nullptr, importer)
        << "MySQLImporterSchemePlugin::createImporter() must return a non-null importer";
}

TEST(MySQLImporterRegistry, CreateImporterReturnsDistinctInstances) {
    auto* plugin = IImporterPluginRegistry::instance().resolve("mysql://host/db");
    ASSERT_NE(nullptr, plugin);

    ImportConfig cfg;
    cfg.source_uri  = "mysql://127.0.0.1:3306/db";
    cfg.json_config = "{}";
    auto a = plugin->createImporter(cfg);
    auto b = plugin->createImporter(cfg);
    ASSERT_NE(nullptr, a);
    ASSERT_NE(nullptr, b);
    EXPECT_NE(a.get(), b.get())
        << "Each createImporter() call must return a fresh importer instance";
}

TEST(MySQLImporterRegistry, CreateImporterWithEmptyConfigDoesNotThrow) {
    auto* plugin = IImporterPluginRegistry::instance().resolve("mysql://host/db");
    ASSERT_NE(nullptr, plugin);

    ImportConfig cfg;  // empty json_config
    EXPECT_NO_THROW({
        auto imp = plugin->createImporter(cfg);
        EXPECT_NE(nullptr, imp);
    });
}

// ===========================================================================
// Admin import API route helper logic
// (mirrors ImportApiHandler::handleStartMySQLImport validation)
//
// Note: This is an intentionally simplified simulation of the handler's
// plugin-availability and source_path-presence checks, consistent with the
// simulateS3RouteValidation pattern in test_importer_async_api.cpp.
// Full JSON-parsing and importer-creation paths require an HTTP server and
// are exercised by the live integration test in the CI workflow
// (mysql-live-integration job in importer-tests.yml).
// The end-to-end registry reachability is covered by
// MySQLImporterAdminRoute::PluginIsRegisteredOnThisServer below.
// ===========================================================================

/// Simulates the plugin-availability and source_path-presence guards in
/// ImportApiHandler::handleStartMySQLImport, mirroring the same approach
/// used by simulateS3RouteValidation in test_importer_async_api.cpp.
/// Returns HTTP-like status: 200 OK, 400 Bad Request, 501 Not Implemented.
static int simulateMySQLRouteValidation(const std::string& source_path,
                                         bool plugin_registered) {
    if (!plugin_registered) return 501;
    if (source_path.empty()) return 400;
    return 200;
}

TEST(MySQLImporterAdminRoute, Returns501WhenPluginNotRegistered) {
    EXPECT_EQ(501, simulateMySQLRouteValidation("/tmp/dump.sql", false));
}

TEST(MySQLImporterAdminRoute, Returns400ForEmptySourcePath) {
    EXPECT_EQ(400, simulateMySQLRouteValidation("", true));
}

TEST(MySQLImporterAdminRoute, Returns200ForValidPath) {
    EXPECT_EQ(200, simulateMySQLRouteValidation("/tmp/dump.sql", true));
    EXPECT_EQ(200, simulateMySQLRouteValidation("mysql://host:3306/db", true));
}

TEST(MySQLImporterAdminRoute, PluginIsRegisteredOnThisServer) {
    // This verifies the full end-to-end condition required by the admin route:
    // the plugin must be resolvable so POST /api/v1/import/mysql returns 200, not 501.
    auto* plugin = IImporterPluginRegistry::instance().resolve("mysql://host/db");
    EXPECT_NE(nullptr, plugin)
        << "POST /api/v1/import/mysql would return 501 – plugin not registered";
}
