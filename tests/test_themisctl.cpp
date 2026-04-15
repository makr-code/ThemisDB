/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_themisctl.cpp                                 ║
  Version:         0.0.12                                             ║
  Last Modified:   2026-04-15 18:18:30                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     915                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 30763c38a6  2026-04-13  feat(metadata): complete Automatic Indexing Recommendatio... ║
    • 97d8d09e74  2026-03-15  feat(tools/themisctl): config command, REPL mode, shell c... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file test_themisctl.cpp
 * @brief Unit tests for the themisctl unified CLI tool.
 *
 * Tests cover:
 *  - Argument parsing helpers (flag, optval)
 *  - Global-option extraction from argv arrays
 *  - Response::ok() status classification
 *  - JSON output helper (printJson round-trip)
 *  - Command dispatch routing
 *  - HTTP round-trips via an in-process httplib::Server
 */

// Pull in the CLI source in test mode (no main(), no ANSI side-effects)
// THEMISCTL_TEST_BUILD is defined via compiler flag (-DTHEMISCTL_TEST_BUILD)
// Disable OpenSSL in tests — plain HTTP is sufficient (no TLS needed for in-process server)
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
#undef CPPHTTPLIB_OPENSSL_SUPPORT
#endif

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <httplib.h>
#include <sstream>
#include <string>
#include <vector>
#include <thread>
#include <atomic>

// Include the CLI implementation (all static helpers become available)
#include "../tools/themisctl.cpp"

using json = nlohmann::json;

// ─────────────────────────────────────────────────────────────────────────────
// Helper: Argument parsing utilities
// ─────────────────────────────────────────────────────────────────────────────

class ThemisctlArgParsingTest : public ::testing::Test {};

TEST_F(ThemisctlArgParsingTest, FlagFound) {
    std::vector<std::string> args = {"--json", "--no-color"};
    EXPECT_TRUE(flag(args, "--json"));
    EXPECT_TRUE(flag(args, "--no-color"));
    EXPECT_FALSE(flag(args, "--help"));
}

TEST_F(ThemisctlArgParsingTest, FlagNotFound) {
    std::vector<std::string> args = {"--host", "db.example.com"};
    EXPECT_FALSE(flag(args, "--json"));
}

TEST_F(ThemisctlArgParsingTest, FlagEmptyArgs) {
    std::vector<std::string> args;
    EXPECT_FALSE(flag(args, "--json"));
}

TEST_F(ThemisctlArgParsingTest, OptvalFound) {
    std::vector<std::string> args = {"--host", "myhost", "--port", "9000"};
    EXPECT_EQ(optval(args, "--host"), "myhost");
    EXPECT_EQ(optval(args, "--port"), "9000");
}

TEST_F(ThemisctlArgParsingTest, OptvalDefault) {
    std::vector<std::string> args = {"--host", "myhost"};
    EXPECT_EQ(optval(args, "--port", "8765"), "8765");
    EXPECT_EQ(optval(args, "--missing"), "");
}

TEST_F(ThemisctlArgParsingTest, OptvalAtEnd) {
    // Key at the end without a value — returns default
    std::vector<std::string> args = {"--host"};
    EXPECT_EQ(optval(args, "--host", "fallback"), "fallback");
}

// ─────────────────────────────────────────────────────────────────────────────
// Helper: Response::ok() classification
// ─────────────────────────────────────────────────────────────────────────────

class ThemisctlResponseTest : public ::testing::Test {};

TEST_F(ThemisctlResponseTest, OkFor2xx) {
    Response r200{200, "body"}; EXPECT_TRUE(r200.ok());
    Response r201{201, ""};     EXPECT_TRUE(r201.ok());
    Response r204{204, ""};     EXPECT_TRUE(r204.ok());
    Response r299{299, ""};     EXPECT_TRUE(r299.ok());
}

TEST_F(ThemisctlResponseTest, NotOkFor4xx5xx) {
    Response r400{400, "bad request"};  EXPECT_FALSE(r400.ok());
    Response r404{404, "not found"};    EXPECT_FALSE(r404.ok());
    Response r500{500, "server error"}; EXPECT_FALSE(r500.ok());
    Response r503{503, "unavailable"};  EXPECT_FALSE(r503.ok());
}

TEST_F(ThemisctlResponseTest, NotOkForConnectionError) {
    Response r{-1, "connection error"}; EXPECT_FALSE(r.ok());
}

// ─────────────────────────────────────────────────────────────────────────────
// Helper: ANSI color toggling
// ─────────────────────────────────────────────────────────────────────────────

class ThemisctlColorTest : public ::testing::Test {
protected:
    void SetUp() override    { g_use_color = true;  }
    void TearDown() override { g_use_color = true;  }
};

TEST_F(ThemisctlColorTest, ColorEnabled) {
    std::string s = col(Color::Green, "OK");
    EXPECT_NE(s.find("OK"), std::string::npos);
    // ANSI codes present when color is enabled
    EXPECT_NE(s.find("\033["), std::string::npos);
}

TEST_F(ThemisctlColorTest, ColorDisabled) {
    g_use_color = false;
    std::string s = col(Color::Green, "OK");
    EXPECT_EQ(s, "OK");
    EXPECT_EQ(s.find("\033["), std::string::npos);
}

// ─────────────────────────────────────────────────────────────────────────────
// Minimal in-process HTTP server fixture
// ─────────────────────────────────────────────────────────────────────────────

// ─────────────────────────────────────────────────────────────────────────────
// Minimal in-process HTTP server — shared across all ThemisctlHttpTest tests
// ─────────────────────────────────────────────────────────────────────────────

// Thread-safe entity store for the test server
static std::mutex                                    s_entities_mutex;
static std::unordered_map<std::string, std::string>  s_entities;

class ThemisctlHttpTest : public ::testing::Test {
protected:
    // ── Class-level server lifecycle ──────────────────────────────────────────
    static void SetUpTestSuite() {
        // Pre-populate a known entity
        {
            std::lock_guard<std::mutex> lk(s_entities_mutex);
            s_entities["user:1"] = R"({"id":"user:1","name":"Alice"})";
        }

        // Health endpoints
        s_srv_.Get("/health/live",  [](const httplib::Request&, httplib::Response& res) {
            res.status = 200;
            res.set_content(R"({"status":"ok"})", "application/json");
        });
        s_srv_.Get("/health/ready", [](const httplib::Request&, httplib::Response& res) {
            res.status = 200;
            res.set_content(R"({"status":"ok"})", "application/json");
        });

        // Version endpoint
        s_srv_.Get("/version", [](const httplib::Request&, httplib::Response& res) {
            res.status = 200;
            res.set_content(
                R"({"version":"1.0.0-test","build":"test-build","commit":"abc1234"})",
                "application/json");
        });

        // AQL query endpoint
        s_srv_.Post("/api/aql", [](const httplib::Request& req, httplib::Response& res) {
            try {
                json body = json::parse(req.body);
                std::string q = body.value("query", "");
                json resp;
                resp["result"] = json::array();
                resp["query"]  = q;
                resp["rows"]   = 0;
                res.status = 200;
                res.set_content(resp.dump(), "application/json");
            } catch (...) {
                res.status = 400;
                res.set_content(R"({"error":"invalid json"})", "application/json");
            }
        });

        // Entity endpoints
        s_srv_.Get(R"(/entities/(.+))", [](const httplib::Request& req, httplib::Response& res) {
            std::string id = req.matches[1];
            std::lock_guard<std::mutex> lk(s_entities_mutex);
            auto it = s_entities.find(id);
            if (it == s_entities.end()) {
                res.status = 404;
                res.set_content(R"({"error":"not found"})", "application/json");
            } else {
                res.status = 200;
                res.set_content(it->second, "application/json");
            }
        });
        s_srv_.Put(R"(/entities/(.+))", [](const httplib::Request& req, httplib::Response& res) {
            std::string id = req.matches[1];
            std::lock_guard<std::mutex> lk(s_entities_mutex);
            s_entities[id] = req.body;
            res.status = 200;
            res.set_content(R"({"ok":true})", "application/json");
        });
        s_srv_.Delete(R"(/entities/(.+))", [](const httplib::Request& req, httplib::Response& res) {
            std::string id = req.matches[1];
            std::lock_guard<std::mutex> lk(s_entities_mutex);
            if (s_entities.erase(id) == 0) {
                res.status = 404;
                res.set_content(R"({"error":"not found"})", "application/json");
            } else {
                res.status = 200;
                res.set_content(R"({"ok":true})", "application/json");
            }
        });

        // Schema endpoint
        s_srv_.Get("/api/v1/schema", [](const httplib::Request&, httplib::Response& res) {
            res.status = 200;
            res.set_content(R"({"tables":["users","orders"]})", "application/json");
        });
        s_srv_.Get(R"(/api/v1/schema/tables/(.+))",
                   [](const httplib::Request& req, httplib::Response& res) {
            std::string table = req.matches[1];
            if (table == "users") {
                res.status = 200;
                res.set_content(R"({"table":"users","columns":["id","name"]})",
                                "application/json");
            } else {
                res.status = 404;
                res.set_content(R"({"error":"not found"})", "application/json");
            }
        });

        // Branch endpoints
        s_srv_.Get("/api/v1/branches", [](const httplib::Request&, httplib::Response& res) {
            res.status = 200;
            res.set_content(R"({"branches":["main","develop"]})", "application/json");
        });
        s_srv_.Post("/api/v1/branches", [](const httplib::Request& req, httplib::Response& res) {
            try {
                json body = json::parse(req.body);
                res.status = 201;
                json resp; resp["created"] = body.value("name","");
                res.set_content(resp.dump(), "application/json");
            } catch (...) {
                res.status = 400;
            }
        });
        s_srv_.Post(R"(/api/v1/branches/(.+)/switch)",
                    [](const httplib::Request&, httplib::Response& res) {
            res.status = 200;
            res.set_content(R"({"ok":true})", "application/json");
        });
        s_srv_.Delete(R"(/api/v1/branches/(.+))",
                      [](const httplib::Request&, httplib::Response& res) {
            res.status = 200;
            res.set_content(R"({"ok":true})", "application/json");
        });

        // Snapshot endpoints
        s_srv_.Get("/api/v1/snapshots/tags", [](const httplib::Request&, httplib::Response& res) {
            res.status = 200;
            res.set_content(R"({"tags":["v1.0","v1.1"]})", "application/json");
        });
        s_srv_.Post("/api/v1/snapshots/tags", [](const httplib::Request&, httplib::Response& res) {
            res.status = 201;
            res.set_content(R"({"ok":true})", "application/json");
        });

        // Observability / admin endpoints
        s_srv_.Get("/api/v1/observability/health",
                   [](const httplib::Request&, httplib::Response& res) {
            res.status = 200;
            res.set_content(
                R"({"status":"healthy","uptime":"42h","nodes":3,"version":"1.0.0-test"})",
                "application/json");
        });
        s_srv_.Get("/v1/admin/cache/health",
                   [](const httplib::Request&, httplib::Response& res) {
            res.status = 200;
            res.set_content(R"({"hit_rate":0.95,"entries":1024})", "application/json");
        });

        // Config endpoint (GET + POST hot-reload)
        s_srv_.Get("/config", [](const httplib::Request&, httplib::Response& res) {
            res.status = 200;
            res.set_content(R"({
                "server":   {"port":8765,"threads":4,"request_timeout_ms":30000},
                "features": {"semantic_cache":false,"llm_store":false,"cdc":false,"timeseries":false},
                "logging":  {"level":"info","format":"text"}
            })", "application/json");
        });
        s_srv_.Post("/config", [](const httplib::Request& req, httplib::Response& res) {
            try {
                json body = json::parse(req.body);
                // Echo back the accepted patch merged into a minimal config
                json reply = {
                    {"server",   {{"port",8765},{"threads",4},{"request_timeout_ms",30000}}},
                    {"features", {{"semantic_cache",false},{"llm_store",false},{"cdc",false},{"timeseries",false}}},
                    {"logging",  {{"level","info"},{"format","text"}}},
                    {"applied",  body}
                };
                res.status = 200;
                res.set_content(reply.dump(), "application/json");
            } catch (...) {
                res.status = 400;
                res.set_content(R"({"error":"invalid json"})", "application/json");
            }
        });

        // Index recommendations endpoint (all tables)
        s_srv_.Get("/api/v1/metadata/index_recommendations",
                   [](const httplib::Request&, httplib::Response& res) {
            json j = {
                {"status", "success"},
                {"recommendations", {
                    {"users", json::array({
                        {{"table_name","users"},{"column_name","email"},
                         {"index_type","regular"},{"action","ADD"},
                         {"benefit_score",75.0},
                         {"rationale","Column 'email' appeared in 100 filter(s)"}}
                    })},
                    {"orders", json::array()}
                }}
            };
            res.status = 200;
            res.set_content(j.dump(), "application/json");
        });
        // Index recommendations endpoint (single table)
        s_srv_.Get(R"(/api/v1/metadata/index_recommendations/(.+))",
                   [](const httplib::Request& req, httplib::Response& res) {
            std::string table = req.matches[1];
            if (table == "users") {
                json j = {
                    {"status", "success"},
                    {"table_name", "users"},
                    {"recommendations", json::array({
                        {{"table_name","users"},{"column_name","email"},
                         {"index_type","regular"},{"action","ADD"},
                         {"benefit_score",75.0},
                         {"rationale","Column 'email' appeared in 100 filter(s)"}}
                    })}
                };
                res.status = 200;
                res.set_content(j.dump(), "application/json");
            } else if (table == "empty_table") {
                json j = {
                    {"status", "success"},
                    {"table_name", "empty_table"},
                    {"recommendations", json::array()}
                };
                res.status = 200;
                res.set_content(j.dump(), "application/json");
            } else {
                res.status = 404;
                res.set_content(R"({"error":"table not found"})", "application/json");
            }
        });

        // Bind and start server
        s_port_ = s_srv_.bind_to_any_port("localhost");
        s_thread_ = std::thread([] { s_srv_.listen_after_bind(); });
    }

    static void TearDownTestSuite() {
        s_srv_.stop();
        if (s_thread_.joinable()) s_thread_.join();
    }

    // ── Per-test setup ────────────────────────────────────────────────────────
    void SetUp() override {
        g_ctx.host     = "localhost";
        g_ctx.port     = s_port_;
        g_ctx.token    = "";
        g_ctx.timeout  = 5;
        g_ctx.raw_json = false;
        g_use_color    = false;
    }

    // Static shared server
    static httplib::Server s_srv_;
    static int             s_port_;
    static std::thread     s_thread_;
};

// Static member definitions
httplib::Server ThemisctlHttpTest::s_srv_;
int             ThemisctlHttpTest::s_port_ = 0;
std::thread     ThemisctlHttpTest::s_thread_;

// ── health ────────────────────────────────────────────────────────────────────

TEST_F(ThemisctlHttpTest, HealthReturnsZeroOnHealthyServer) {
    int rc = cmdHealth({});
    EXPECT_EQ(rc, 0);
}

// ── version ───────────────────────────────────────────────────────────────────

TEST_F(ThemisctlHttpTest, VersionReturnsZero) {
    g_ctx.raw_json = true;
    std::ostringstream cap;
    auto* old = std::cout.rdbuf(cap.rdbuf());
    int rc = cmdVersion({});
    std::cout.rdbuf(old);
    EXPECT_EQ(rc, 0);
    json j = json::parse(cap.str());
    EXPECT_EQ(j["version"], "1.0.0-test");
}

// ── query ─────────────────────────────────────────────────────────────────────

TEST_F(ThemisctlHttpTest, QueryEmptyArgsReturnsUsageError) {
    int rc = cmdQuery({});
    EXPECT_EQ(rc, 2);
}

TEST_F(ThemisctlHttpTest, QueryExecutesAql) {
    g_ctx.raw_json = true;
    std::ostringstream cap;
    auto* old = std::cout.rdbuf(cap.rdbuf());
    int rc = cmdQuery({"FOR d IN users RETURN d"});
    std::cout.rdbuf(old);
    EXPECT_EQ(rc, 0);
    json j = json::parse(cap.str());
    EXPECT_EQ(j["query"], "FOR d IN users RETURN d");
}

// ── get ───────────────────────────────────────────────────────────────────────

TEST_F(ThemisctlHttpTest, GetExistingEntity) {
    // user:1 is pre-populated by SetUpTestSuite
    g_ctx.raw_json = true;
    std::ostringstream cap;
    auto* old = std::cout.rdbuf(cap.rdbuf());
    int rc = cmdGet({"user:1"});
    std::cout.rdbuf(old);
    EXPECT_EQ(rc, 0);
    json j = json::parse(cap.str());
    EXPECT_EQ(j["name"], "Alice");
}

TEST_F(ThemisctlHttpTest, GetMissingEntityReturnsOne) {
    int rc = cmdGet({"user:999"});
    EXPECT_EQ(rc, 1);
}

TEST_F(ThemisctlHttpTest, GetMissingArgReturnsUsageError) {
    int rc = cmdGet({});
    EXPECT_EQ(rc, 2);
}

// ── put ───────────────────────────────────────────────────────────────────────

TEST_F(ThemisctlHttpTest, PutValidEntity) {
    int rc = cmdPut({"user:put1", R"({"name":"Bob","active":true})"});
    EXPECT_EQ(rc, 0);
}

TEST_F(ThemisctlHttpTest, PutThenGetPersistsEntity) {
    // Store a new entity with a unique key
    ASSERT_EQ(cmdPut({"user:ptg1", R"({"name":"Carol","role":"admin"})"}), 0);
    // Retrieve it and verify the content
    g_ctx.raw_json = true;
    std::ostringstream cap;
    auto* old = std::cout.rdbuf(cap.rdbuf());
    int rc = cmdGet({"user:ptg1"});
    std::cout.rdbuf(old);
    EXPECT_EQ(rc, 0);
    json j = json::parse(cap.str());
    EXPECT_EQ(j["name"], "Carol");
    EXPECT_EQ(j["role"], "admin");
}

TEST_F(ThemisctlHttpTest, PutInvalidJsonReturnsUsageError) {
    int rc = cmdPut({"user:2", "not-json"});
    EXPECT_EQ(rc, 2);
}

TEST_F(ThemisctlHttpTest, PutMissingArgReturnsUsageError) {
    EXPECT_EQ(cmdPut({}), 2);
    EXPECT_EQ(cmdPut({"user:2"}), 2);
}

// ── delete ────────────────────────────────────────────────────────────────────

TEST_F(ThemisctlHttpTest, DeleteExistingEntity) {
    // Put an entity first, then delete it with a unique key to avoid order dependencies
    ASSERT_EQ(cmdPut({"user:del1", R"({"name":"ToDelete"})"}), 0);
    int rc = cmdDelete({"user:del1"});
    EXPECT_EQ(rc, 0);
}

TEST_F(ThemisctlHttpTest, DeleteMissingEntityReturnsOne) {
    int rc = cmdDelete({"user:999"});
    EXPECT_EQ(rc, 1);
}

TEST_F(ThemisctlHttpTest, DeleteMissingArgReturnsUsageError) {
    EXPECT_EQ(cmdDelete({}), 2);
}

// ── schema ────────────────────────────────────────────────────────────────────

TEST_F(ThemisctlHttpTest, SchemaAll) {
    g_ctx.raw_json = true;
    std::ostringstream cap;
    auto* old = std::cout.rdbuf(cap.rdbuf());
    int rc = cmdSchema({});
    std::cout.rdbuf(old);
    EXPECT_EQ(rc, 0);
    json j = json::parse(cap.str());
    EXPECT_TRUE(j.contains("tables"));
}

TEST_F(ThemisctlHttpTest, SchemaTable) {
    g_ctx.raw_json = true;
    std::ostringstream cap;
    auto* old = std::cout.rdbuf(cap.rdbuf());
    int rc = cmdSchema({"users"});
    std::cout.rdbuf(old);
    EXPECT_EQ(rc, 0);
    json j = json::parse(cap.str());
    EXPECT_EQ(j["table"], "users");
}

TEST_F(ThemisctlHttpTest, SchemaMissingTableReturnsOne) {
    int rc = cmdSchema({"nonexistent"});
    EXPECT_EQ(rc, 1);
}

// ── branch ────────────────────────────────────────────────────────────────────

TEST_F(ThemisctlHttpTest, BranchList) {
    g_ctx.raw_json = true;
    std::ostringstream cap;
    auto* old = std::cout.rdbuf(cap.rdbuf());
    int rc = cmdBranch({"list"});
    std::cout.rdbuf(old);
    EXPECT_EQ(rc, 0);
    json j = json::parse(cap.str());
    EXPECT_TRUE(j.contains("branches"));
}

TEST_F(ThemisctlHttpTest, BranchCreate) {
    int rc = cmdBranch({"create", "feature-x"});
    EXPECT_EQ(rc, 0);
}

TEST_F(ThemisctlHttpTest, BranchCreateMissingName) {
    EXPECT_EQ(cmdBranch({"create"}), 2);
}

TEST_F(ThemisctlHttpTest, BranchSwitch) {
    int rc = cmdBranch({"switch", "develop"});
    EXPECT_EQ(rc, 0);
}

TEST_F(ThemisctlHttpTest, BranchSwitchMissingName) {
    EXPECT_EQ(cmdBranch({"switch"}), 2);
}

TEST_F(ThemisctlHttpTest, BranchDelete) {
    int rc = cmdBranch({"delete", "feature-x"});
    EXPECT_EQ(rc, 0);
}

TEST_F(ThemisctlHttpTest, BranchDeleteMissingName) {
    EXPECT_EQ(cmdBranch({"delete"}), 2);
}

TEST_F(ThemisctlHttpTest, BranchUnknownSubcommand) {
    EXPECT_EQ(cmdBranch({"frobnicate"}), 2);
}

// ── snapshot ──────────────────────────────────────────────────────────────────

TEST_F(ThemisctlHttpTest, SnapshotList) {
    g_ctx.raw_json = true;
    std::ostringstream cap;
    auto* old = std::cout.rdbuf(cap.rdbuf());
    int rc = cmdSnapshot({"list"});
    std::cout.rdbuf(old);
    EXPECT_EQ(rc, 0);
    json j = json::parse(cap.str());
    EXPECT_TRUE(j.contains("tags"));
}

TEST_F(ThemisctlHttpTest, SnapshotCreate) {
    int rc = cmdSnapshot({"create", "v2.0"});
    EXPECT_EQ(rc, 0);
}

TEST_F(ThemisctlHttpTest, SnapshotCreateNoTag) {
    int rc = cmdSnapshot({"create"});
    EXPECT_EQ(rc, 0);
}

TEST_F(ThemisctlHttpTest, SnapshotUnknownSubcommand) {
    EXPECT_EQ(cmdSnapshot({"drop"}), 2);
}

// ── admin ─────────────────────────────────────────────────────────────────────

TEST_F(ThemisctlHttpTest, AdminStats) {
    g_ctx.raw_json = true;
    std::ostringstream cap;
    auto* old = std::cout.rdbuf(cap.rdbuf());
    int rc = cmdAdmin({"stats"});
    std::cout.rdbuf(old);
    EXPECT_EQ(rc, 0);
    json j = json::parse(cap.str());
    EXPECT_EQ(j["status"], "healthy");
    EXPECT_EQ(j["nodes"], 3);
}

TEST_F(ThemisctlHttpTest, AdminCache) {
    g_ctx.raw_json = true;
    std::ostringstream cap;
    auto* old = std::cout.rdbuf(cap.rdbuf());
    int rc = cmdAdmin({"cache"});
    std::cout.rdbuf(old);
    EXPECT_EQ(rc, 0);
    json j = json::parse(cap.str());
    EXPECT_TRUE(j.contains("hit_rate"));
}

TEST_F(ThemisctlHttpTest, AdminUnknownSubcommand) {
    EXPECT_EQ(cmdAdmin({"frobnicate"}), 2);
}

// ── index ─────────────────────────────────────────────────────────────────────

TEST_F(ThemisctlHttpTest, IndexRecommendAllTablesRawJson) {
    g_ctx.raw_json = true;
    std::ostringstream cap;
    auto* old = std::cout.rdbuf(cap.rdbuf());
    int rc = cmdIndex({"recommend"});
    std::cout.rdbuf(old);
    EXPECT_EQ(rc, 0);
    json j = json::parse(cap.str());
    EXPECT_TRUE(j.contains("recommendations"));
}

TEST_F(ThemisctlHttpTest, IndexRecommendNoSubAllTablesRawJson) {
    // "index" with no sub-command defaults to recommending all tables
    g_ctx.raw_json = true;
    std::ostringstream cap;
    auto* old = std::cout.rdbuf(cap.rdbuf());
    int rc = cmdIndex({});
    std::cout.rdbuf(old);
    EXPECT_EQ(rc, 0);
    json j = json::parse(cap.str());
    EXPECT_TRUE(j.contains("recommendations"));
}

TEST_F(ThemisctlHttpTest, IndexRecommendSingleTableRawJson) {
    g_ctx.raw_json = true;
    std::ostringstream cap;
    auto* old = std::cout.rdbuf(cap.rdbuf());
    int rc = cmdIndex({"recommend", "users"});
    std::cout.rdbuf(old);
    EXPECT_EQ(rc, 0);
    json j = json::parse(cap.str());
    EXPECT_TRUE(j.contains("recommendations"));
    EXPECT_EQ(j["table_name"], "users");
    ASSERT_TRUE(j["recommendations"].is_array());
    ASSERT_FALSE(j["recommendations"].empty());
    EXPECT_EQ(j["recommendations"][0]["action"], "ADD");
    EXPECT_EQ(j["recommendations"][0]["column_name"], "email");
}

TEST_F(ThemisctlHttpTest, IndexRecommendEmptyTableRawJson) {
    g_ctx.raw_json = true;
    std::ostringstream cap;
    auto* old = std::cout.rdbuf(cap.rdbuf());
    int rc = cmdIndex({"recommend", "empty_table"});
    std::cout.rdbuf(old);
    EXPECT_EQ(rc, 0);
    json j = json::parse(cap.str());
    EXPECT_TRUE(j["recommendations"].is_array());
    EXPECT_TRUE(j["recommendations"].empty());
}

TEST_F(ThemisctlHttpTest, IndexRecommendPrettyPrintAllTables) {
    g_ctx.raw_json = false;
    std::ostringstream cap;
    auto* old = std::cout.rdbuf(cap.rdbuf());
    int rc = cmdIndex({"recommend"});
    std::cout.rdbuf(old);
    EXPECT_EQ(rc, 0);
    EXPECT_NE(cap.str().find("email"), std::string::npos);
}

TEST_F(ThemisctlHttpTest, IndexRecommendPrettyPrintSingleTable) {
    g_ctx.raw_json = false;
    std::ostringstream cap;
    auto* old = std::cout.rdbuf(cap.rdbuf());
    int rc = cmdIndex({"recommend", "users"});
    std::cout.rdbuf(old);
    EXPECT_EQ(rc, 0);
    EXPECT_NE(cap.str().find("users"), std::string::npos);
}

TEST_F(ThemisctlHttpTest, IndexRecommendNotFoundTableReturnsOne) {
    int rc = cmdIndex({"recommend", "nonexistent_table"});
    EXPECT_EQ(rc, 1);
}

TEST_F(ThemisctlHttpTest, IndexUnknownSubcommandReturnsTwo) {
    EXPECT_EQ(cmdIndex({"frobnicate"}), 2);
}

// ── config ────────────────────────────────────────────────────────────────────

TEST_F(ThemisctlHttpTest, ConfigGet) {
    g_ctx.raw_json = true;
    std::ostringstream cap;
    auto* old = std::cout.rdbuf(cap.rdbuf());
    int rc = cmdConfig({"get"});
    std::cout.rdbuf(old);
    EXPECT_EQ(rc, 0);
    json j = json::parse(cap.str());
    EXPECT_TRUE(j.contains("server"));
    EXPECT_TRUE(j.contains("features"));
}

TEST_F(ThemisctlHttpTest, ConfigGetDefaultSub) {
    // "config" with no sub-command defaults to "get"
    g_ctx.raw_json = true;
    std::ostringstream cap;
    auto* old = std::cout.rdbuf(cap.rdbuf());
    int rc = cmdConfig({});
    std::cout.rdbuf(old);
    EXPECT_EQ(rc, 0);
}

TEST_F(ThemisctlHttpTest, ConfigSetSingleKey) {
    int rc = cmdConfig({"set", "request_timeout_ms=60000"});
    EXPECT_EQ(rc, 0);
}

TEST_F(ThemisctlHttpTest, ConfigSetDottedKey) {
    int rc = cmdConfig({"set", "logging.level=debug"});
    EXPECT_EQ(rc, 0);
}

TEST_F(ThemisctlHttpTest, ConfigSetMultipleKeys) {
    int rc = cmdConfig({"set", "logging.level=warn", "features.cdc=true"});
    EXPECT_EQ(rc, 0);
}

TEST_F(ThemisctlHttpTest, ConfigSetVerifyPatch) {
    g_ctx.raw_json = true;
    std::ostringstream cap;
    auto* old = std::cout.rdbuf(cap.rdbuf());
    int rc = cmdConfig({"set", "logging.level=debug", "request_timeout_ms=60000"});
    std::cout.rdbuf(old);
    EXPECT_EQ(rc, 0);
    json j = json::parse(cap.str());
    // The test server echoes the applied patch
    EXPECT_TRUE(j.contains("applied"));
    EXPECT_TRUE(j["applied"].contains("logging"));
    EXPECT_EQ(j["applied"]["logging"]["level"], "debug");
}

TEST_F(ThemisctlHttpTest, ConfigSetMissingValueReturnsUsageError) {
    EXPECT_EQ(cmdConfig({"set"}), 2);
}

TEST_F(ThemisctlHttpTest, ConfigSetInvalidPairReturnsUsageError) {
    EXPECT_EQ(cmdConfig({"set", "no-equals-sign"}), 2);
}

TEST_F(ThemisctlHttpTest, ConfigUnknownSubcommand) {
    EXPECT_EQ(cmdConfig({"frobnicate"}), 2);
}

// ── REPL tokeniser ────────────────────────────────────────────────────────────

class ThemisctlTokenizerTest : public ::testing::Test {};

TEST_F(ThemisctlTokenizerTest, SimpleWords) {
    std::vector<std::string> tokens;
    std::string err;
    ASSERT_TRUE(tokenizeLine("health", tokens, err));
    ASSERT_EQ(tokens.size(), 1u);
    EXPECT_EQ(tokens[0], "health");
}

TEST_F(ThemisctlTokenizerTest, MultipleWords) {
    std::vector<std::string> tokens;
    std::string err;
    ASSERT_TRUE(tokenizeLine("config set logging.level=debug", tokens, err));
    ASSERT_EQ(tokens.size(), 3u);
    EXPECT_EQ(tokens[0], "config");
    EXPECT_EQ(tokens[1], "set");
    EXPECT_EQ(tokens[2], "logging.level=debug");
}

TEST_F(ThemisctlTokenizerTest, SingleQuotedToken) {
    std::vector<std::string> tokens;
    std::string err;
    ASSERT_TRUE(tokenizeLine("query 'FOR d IN col RETURN d'", tokens, err));
    ASSERT_EQ(tokens.size(), 2u);
    EXPECT_EQ(tokens[1], "FOR d IN col RETURN d");
}

TEST_F(ThemisctlTokenizerTest, DoubleQuotedToken) {
    std::vector<std::string> tokens;
    std::string err;
    ASSERT_TRUE(tokenizeLine("get \"user:1\"", tokens, err));
    ASSERT_EQ(tokens.size(), 2u);
    EXPECT_EQ(tokens[1], "user:1");
}

TEST_F(ThemisctlTokenizerTest, ExtraWhitespace) {
    std::vector<std::string> tokens;
    std::string err;
    ASSERT_TRUE(tokenizeLine("  health  ", tokens, err));
    ASSERT_EQ(tokens.size(), 1u);
    EXPECT_EQ(tokens[0], "health");
}

TEST_F(ThemisctlTokenizerTest, EmptyLine) {
    std::vector<std::string> tokens;
    std::string err;
    ASSERT_TRUE(tokenizeLine("", tokens, err));
    EXPECT_TRUE(tokens.empty());
}

TEST_F(ThemisctlTokenizerTest, UnterminatedSingleQuote) {
    std::vector<std::string> tokens;
    std::string err;
    EXPECT_FALSE(tokenizeLine("get 'user:1", tokens, err));
    EXPECT_FALSE(err.empty());
}

TEST_F(ThemisctlTokenizerTest, UnterminatedDoubleQuote) {
    std::vector<std::string> tokens;
    std::string err;
    EXPECT_FALSE(tokenizeLine("get \"user:1", tokens, err));
    EXPECT_FALSE(err.empty());
}

TEST_F(ThemisctlTokenizerTest, MixedQuotes) {
    std::vector<std::string> tokens;
    std::string err;
    ASSERT_TRUE(tokenizeLine("config set 'logging.level=debug'", tokens, err));
    ASSERT_EQ(tokens.size(), 3u);
    EXPECT_EQ(tokens[2], "logging.level=debug");
}

// ── dispatch helper ───────────────────────────────────────────────────────────

TEST(ThemisctlDispatchTest, UnknownCommandReturnsTwo) {
    g_ctx.host    = "localhost";
    g_ctx.port    = 19999;
    g_ctx.timeout = 1;
    g_use_color   = false;
    std::ostringstream capErr;
    auto* old = std::cerr.rdbuf(capErr.rdbuf());
    int rc = dispatchCommand("frobnicate", {});
    std::cerr.rdbuf(old);
    EXPECT_EQ(rc, 2);
}

TEST(ThemisctlDispatchTest, HelpReturnsZero) {
    g_ctx.host    = "localhost";
    g_ctx.port    = 19999;
    g_ctx.timeout = 1;
    g_use_color   = false;
    std::ostringstream capOut;
    auto* old = std::cout.rdbuf(capOut.rdbuf());
    int rc = dispatchCommand("help", {});
    std::cout.rdbuf(old);
    EXPECT_EQ(rc, 0);
    EXPECT_NE(capOut.str().find("themisctl"), std::string::npos);
}

// ── connection failure ────────────────────────────────────────────────────────

TEST(ThemisctlConnFailTest, HealthReturnsThreeOnConnectionRefused) {
    g_ctx.host    = "localhost";
    g_ctx.port    = 19999;   // nothing listening here
    g_ctx.timeout = 1;
    g_use_color   = false;
    // Capture stderr so test output stays clean
    std::ostringstream capErr;
    auto* old = std::cerr.rdbuf(capErr.rdbuf());
    int rc = cmdHealth({});
    std::cerr.rdbuf(old);
    EXPECT_EQ(rc, 3);
}
