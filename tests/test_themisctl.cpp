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
#include <optional>

// Include the CLI implementation (all static helpers become available)
#include "../tools/themisctl.cpp"

using json = nlohmann::json;

// ─────────────────────────────────────────────────────────────────────────────
// Local test helpers: flag() and optval() wrappers
// ─────────────────────────────────────────────────────────────────────────────

/// Returns true if @p key appears in @p args.
static bool flag(const std::vector<std::string>& args, const std::string& key) {
    return std::find(args.begin(), args.end(), key) != args.end();
}

/// Returns the value following @p key in @p args, or @p def if not found / at end.
static std::string optval(const std::vector<std::string>& args,
                           const std::string& key,
                           const std::string& def = "") {
    for (std::size_t i = 0; i + 1 < args.size(); ++i) {
        if (args[i] == key) {
          return args[i + 1];
        }
    }
    return def;
}

/// Extract the first JSON object from mixed stderr/stdout text.
static std::optional<json> extractFirstJsonObject(const std::string& text) {
    const auto start = text.find('{');
    if (start == std::string::npos) {
        return std::nullopt;
    }
    try {
        return json::parse(text.substr(start));
    } catch (...) {
        return std::nullopt;
    }
}

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

        // Config validate endpoint (dry-run — returns validated config, no mutation)
        s_srv_.Post("/config/validate", [](const httplib::Request& req, httplib::Response& res) {
            try {
                json body = json::parse(req.body);
                // Check for explicitly invalid keys to exercise error path
                if (body.contains("invalid_key")) {
                    res.status = 400;
                    res.set_content(R"({"error":"unknown key: invalid_key"})",
                                    "application/json");
                    return;
                }
                // Echo validated (merged) config
                json validated = {
                    {"server",   {{"port",8765},{"threads",4},{"request_timeout_ms",30000}}},
                    {"features", {{"semantic_cache",false},{"llm_store",false},{"cdc",false},{"timeseries",false}}},
                    {"logging",  {{"level","info"},{"format","text"}}}
                };
                // Apply proposed values to the validated output
                for (const auto& [key, val] : body.items()) {
                    if (validated.contains(key) && validated[key].is_object() && val.is_object()) {
                        for (const auto& [inner, inner_val] : val.items()) {
                            validated[key][inner] = inner_val;
                        }
                    } else {
                        validated[key] = val;
                    }
                }
                res.status = 200;
                res.set_content(validated.dump(), "application/json");
            } catch (...) {
                res.status = 400;
                res.set_content(R"({"error":"invalid json"})", "application/json");
            }
        });


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

        // RAG query endpoint — POST /api/v1/llm/rag
        // TRQ-01..06 server fixture: returns a canned answer; rejects missing query.
        s_srv_.Post("/api/v1/llm/rag", [](const httplib::Request& req, httplib::Response& res) {
            try {
                json body = json::parse(req.body);
                if (!body.contains("query") || body["query"].get<std::string>().empty()) {
                    res.status = 400;
                    res.set_content(R"({"error":"Missing 'query' field"})", "application/json");
                    return;
                }
                std::string query = body["query"].get<std::string>();
                std::string collection = body.value("collection", std::string{});
                int top_k_effective = body.value("top_k", 5);
                int max_context_tokens_effective = body.value("max_context_tokens", 0);
                int response_budget_tokens_effective = body.value("response_budget_tokens", 512);
                int max_tokens_requested = body.value("max_tokens", 512);
                std::string rag_mode_effective = body.value("rag_mode", std::string{"text"});

                if (max_tokens_requested <= 0) {
                    json err = {
                        {"error", "max_tokens must be greater than 0"},
                        {"status", 400}
                    };
                    res.status = 400;
                    res.set_content(err.dump(), "application/json");
                    return;
                }

                const int normalized_response_budget =
                    response_budget_tokens_effective <= 0 ? 1 : response_budget_tokens_effective;
                response_budget_tokens_effective = std::min(normalized_response_budget, max_tokens_requested);

                if (top_k_effective < 1 || top_k_effective > 100) {
                    json err = {
                        {"error", "top_k out of range"},
                        {"details", "top_k must be between 1 and 100"},
                        {"status", 400}
                    };
                    res.status = 400;
                    res.set_content(err.dump(), "application/json");
                    return;
                }

                if (collection == "missing_engine") {
                    json err = {
                        {"error", "RAG retrieval engine not configured"},
                        {"details", "Call setQueryEngine() before using /api/v1/llm/rag"},
                        {"status", 503}
                    };
                    res.status = 503;
                    res.set_content(err.dump(), "application/json");
                    return;
                }

                if (collection == "invalid_collection") {
                    json err = {
                        {"error", "RAG retrieval failed"},
                        {"details", "collection not found: invalid_collection"},
                        {"status", 503}
                    };
                    res.status = 503;
                    res.set_content(err.dump(), "application/json");
                    return;
                }

                if (collection == "empty_collection") {
                    json err = {
                        {"error", "RAG retrieval returned no usable documents"},
                        {"details", "No documents matched the retrieval query (retrieved=0, rejected=0)"},
                        {"status", 503}
                    };
                    res.status = 503;
                    res.set_content(err.dump(), "application/json");
                    return;
                }

                json resp = {
                    {"text",               "Das Bauamt benötigt noch den Lageplan und die Baugenehmigung."},
                    {"query",              query},
                    {"collection_effective", collection},
                    {"rag_mode_effective",  rag_mode_effective},
                    {"retrieval_attempted", !collection.empty()},
                    {"documents_retrieved",3},
                    {"documents_rejected", 0},
                    {"top_k_effective", top_k_effective},
                    {"max_context_tokens_effective", max_context_tokens_effective < 0 ? 0 : max_context_tokens_effective},
                    {"response_budget_tokens_effective", response_budget_tokens_effective},
                    {"tokens_generated",   42},
                    {"inference_time_ms",  17},
                    {"cache_hit",          false}
                };
                res.status = 200;
                res.set_content(resp.dump(), "application/json");
            } catch (...) {
                res.status = 400;
                res.set_content(R"({"error":"invalid json"})", "application/json");
            }
        });

        // Bind and start server
        s_port_ = s_srv_.bind_to_any_port("localhost");
        s_thread_ = std::thread([] { s_srv_.listen_after_bind(); });
    }

    static void TearDownTestSuite() {
        s_srv_.stop();
        if (s_thread_.joinable()) {
          s_thread_.join();
        }
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

// ── config validate ───────────────────────────────────────────────────────────
// CVL-01..06: themisctl config validate — dry-run + diff

// CVL-01: validate with no proposed keys → passes, prints no-changes message
TEST_F(ThemisctlHttpTest, CVL01_ConfigValidateNoArgs) {
    std::ostringstream capOut;
    auto* old = std::cout.rdbuf(capOut.rdbuf());
    int rc = cmdConfig({"validate"});
    std::cout.rdbuf(old);
    EXPECT_EQ(rc, 0);
    EXPECT_NE(capOut.str().find("no changes"), std::string::npos);
}

// CVL-02: validate with a dotted key change → passes, stdout contains diff
TEST_F(ThemisctlHttpTest, CVL02_ConfigValidateDottedKey) {
    std::ostringstream capOut;
    auto* old = std::cout.rdbuf(capOut.rdbuf());
    int rc = cmdConfig({"validate", "logging.level=debug"});
    std::cout.rdbuf(old);
    EXPECT_EQ(rc, 0);
    EXPECT_NE(capOut.str().find("dry-run"), std::string::npos);
}

// CVL-03: validate with multiple keys → passes, output shows diff header
TEST_F(ThemisctlHttpTest, CVL03_ConfigValidateMultipleKeys) {
    std::ostringstream capOut;
    auto* old = std::cout.rdbuf(capOut.rdbuf());
    int rc = cmdConfig({"validate", "logging.level=warn", "features.cdc=true"});
    std::cout.rdbuf(old);
    EXPECT_EQ(rc, 0);
    EXPECT_NE(capOut.str().find("Diff"), std::string::npos);
}

// CVL-04: validate with raw_json returns parseable JSON
TEST_F(ThemisctlHttpTest, CVL04_ConfigValidateRawJson) {
    g_ctx.raw_json = true;
    std::ostringstream capOut;
    auto* old = std::cout.rdbuf(capOut.rdbuf());
    int rc = cmdConfig({"validate", "logging.level=info"});
    std::cout.rdbuf(old);
    EXPECT_EQ(rc, 0);
    // Output should be parseable JSON
    EXPECT_NO_THROW({
        json j = json::parse(capOut.str());
        EXPECT_TRUE(j.contains("logging"));
    });
}

// CVL-05: validate with invalid key → server returns 400, command returns 1
TEST_F(ThemisctlHttpTest, CVL05_ConfigValidateInvalidKey_ReturnsError) {
    std::ostringstream capErr;
    auto* oldErr = std::cerr.rdbuf(capErr.rdbuf());
    std::ostringstream capOut;
    auto* oldOut = std::cout.rdbuf(capOut.rdbuf());
    int rc = cmdConfig({"validate", "invalid_key=bad"});
    std::cerr.rdbuf(oldErr);
    std::cout.rdbuf(oldOut);
    EXPECT_EQ(rc, 1);
    EXPECT_NE(capErr.str().find("Validation failed"), std::string::npos);
}

// CVL-06: validate with malformed key=value pair → usage error (rc=2), no HTTP call
TEST_F(ThemisctlHttpTest, CVL06_ConfigValidateMalformedPair_ReturnsUsageError) {
    std::ostringstream capErr;
    auto* old = std::cerr.rdbuf(capErr.rdbuf());
    int rc = cmdConfig({"validate", "no-equals-sign"});
    std::cerr.rdbuf(old);
    EXPECT_EQ(rc, 2);
}

// ── rag query ─────────────────────────────────────────────────────────────────
// TRQ-01..06: themisctl rag query — AgenticRAG natural-language query

// TRQ-01: missing question → usage error (rc=2), no HTTP call
TEST_F(ThemisctlHttpTest, TRQ01_RagQuery_MissingQuestion_ReturnsUsageError) {
    std::ostringstream capErr;
    auto* old = std::cerr.rdbuf(capErr.rdbuf());
    int rc = cmdRag({"query"});
    std::cerr.rdbuf(old);
    EXPECT_EQ(rc, 2);
}

// TRQ-02: no sub-command → usage error (rc=2)
TEST_F(ThemisctlHttpTest, TRQ02_RagNoSubCommand_ReturnsUsageError) {
    std::ostringstream capErr;
    auto* old = std::cerr.rdbuf(capErr.rdbuf());
    int rc = cmdRag({});
    std::cerr.rdbuf(old);
    EXPECT_EQ(rc, 2);
}

// TRQ-03: successful query → rc=0, stdout contains answer text
TEST_F(ThemisctlHttpTest, TRQ03_RagQuery_Success_PrintsAnswer) {
    std::ostringstream capOut;
    auto* old = std::cout.rdbuf(capOut.rdbuf());
    int rc = cmdRag({"query", "Was fehlt noch für den Bauantrag?"});
    std::cout.rdbuf(old);
    EXPECT_EQ(rc, 0);
    EXPECT_NE(capOut.str().find("Bauamt"), std::string::npos);
}

// TRQ-04: --collection and --top-k flags are forwarded, rc=0
TEST_F(ThemisctlHttpTest, TRQ04_RagQuery_WithCollectionAndTopK) {
    std::ostringstream capOut;
    auto* old = std::cout.rdbuf(capOut.rdbuf());
    int rc = cmdRag({"query", "--collection", "procs", "--top-k", "10", "What is the next step?"});
    std::cout.rdbuf(old);
    EXPECT_EQ(rc, 0);
    EXPECT_NE(capOut.str().find("Answer"), std::string::npos);
}

// TRQ-05: raw_json mode returns parseable JSON with expected fields
TEST_F(ThemisctlHttpTest, TRQ05_RagQuery_RawJson_ReturnsParseable) {
    g_ctx.raw_json = true;
    std::ostringstream capOut;
    auto* old = std::cout.rdbuf(capOut.rdbuf());
    int rc = cmdRag({"query", "List the required documents."});
    std::cout.rdbuf(old);
    g_ctx.raw_json = false;
    EXPECT_EQ(rc, 0);
    EXPECT_NO_THROW({
        json j = json::parse(capOut.str());
        EXPECT_TRUE(j.contains("text"));
        EXPECT_TRUE(j.contains("documents_retrieved"));
        EXPECT_TRUE(j.contains("collection_effective"));
        EXPECT_TRUE(j.contains("rag_mode_effective"));
        EXPECT_TRUE(j.contains("retrieval_attempted"));
        EXPECT_TRUE(j.contains("documents_rejected"));
        EXPECT_TRUE(j.contains("top_k_effective"));
        EXPECT_TRUE(j.contains("max_context_tokens_effective"));
        EXPECT_TRUE(j.contains("response_budget_tokens_effective"));
    });
}

// TRQ-07: raw_json with collection/top-k validates effective RAG contract fields
TEST_F(ThemisctlHttpTest, TRQ07_RagQuery_RawJson_EffectiveFieldsMatchFlags) {
    g_ctx.raw_json = true;
    std::ostringstream capOut;
    auto* old = std::cout.rdbuf(capOut.rdbuf());
    int rc = cmdRag({"query", "--collection", "procs", "--top-k", "10", "What is the next step?"});
    std::cout.rdbuf(old);
    g_ctx.raw_json = false;

    EXPECT_EQ(rc, 0);
    EXPECT_NO_THROW({
        json j = json::parse(capOut.str());
        EXPECT_EQ(j.value("collection_effective", std::string{}), "procs");
        EXPECT_EQ(j.value("rag_mode_effective", std::string{}), "text");
        EXPECT_TRUE(j.value("retrieval_attempted", false));
        EXPECT_EQ(j.value("top_k_effective", 0), 10);
        EXPECT_GE(j.value("max_context_tokens_effective", -1), 0);
        EXPECT_GT(j.value("response_budget_tokens_effective", 0), 0);
        EXPECT_GE(j.value("documents_rejected", -1), 0);
    });
}

// TRQ-06: invalid top-k value (non-integer) → usage error (rc=2)
TEST_F(ThemisctlHttpTest, TRQ06_RagQuery_InvalidTopK_ReturnsUsageError) {
    std::ostringstream capErr;
    auto* old = std::cerr.rdbuf(capErr.rdbuf());
    int rc = cmdRag({"query", "--top-k", "notanumber", "some question"});
    std::cerr.rdbuf(old);
    EXPECT_EQ(rc, 2);
}

TEST_F(ThemisctlHttpTest, TRQ16_RagQuery_InvalidMaxTokens_ReturnsUsageError) {
    std::ostringstream capErr;
    auto* old = std::cerr.rdbuf(capErr.rdbuf());
    int rc = cmdRag({"query", "--max-tokens", "notanumber", "some question"});
    std::cerr.rdbuf(old);
    EXPECT_EQ(rc, 2);
}

TEST_F(ThemisctlHttpTest, TRQ17_RagQuery_InvalidResponseBudgetTokens_ReturnsUsageError) {
    std::ostringstream capErr;
    auto* old = std::cerr.rdbuf(capErr.rdbuf());
    int rc = cmdRag({"query", "--response-budget-tokens", "notanumber", "some question"});
    std::cerr.rdbuf(old);
    EXPECT_EQ(rc, 2);
}

TEST_F(ThemisctlHttpTest, TRQ20_RagQuery_InvalidRagMode_ReturnsUsageError) {
    std::ostringstream capErr;
    auto* old = std::cerr.rdbuf(capErr.rdbuf());
    int rc = cmdRag({"query", "--rag-mode", "invalid_mode", "some question"});
    std::cerr.rdbuf(old);

    EXPECT_EQ(rc, 2);
    EXPECT_NE(capErr.str().find("--rag-mode must be one of: text, iterative, map_reduce"), std::string::npos);
}

// TRQ-18: non-positive response budget is normalized by server contract and capped by max_tokens
TEST_F(ThemisctlHttpTest, TRQ18_RagQuery_CliFlags_NonPositiveResponseBudgetNormalizesToOne) {
    g_ctx.raw_json = true;
    std::ostringstream capOut;
    auto* old = std::cout.rdbuf(capOut.rdbuf());
    int rc = cmdRag({"query",
                     "--collection", "procs",
                     "--response-budget-tokens", "0",
                     "--max-tokens", "64",
                     "Check normalization"});
    std::cout.rdbuf(old);
    g_ctx.raw_json = false;

    EXPECT_EQ(rc, 0);
    json j;
    ASSERT_NO_THROW(j = json::parse(capOut.str()));
    ASSERT_TRUE(j.contains("response_budget_tokens_effective"));
    EXPECT_EQ(j.value("response_budget_tokens_effective", 0), 1);
}

// TRQ-19: max_tokens <= 0 is rejected fail-closed by RAG contract
TEST_F(ThemisctlHttpTest, TRQ19_RagQuery_CliFlags_ZeroMaxTokens_ServerRejectsWith400) {
    std::ostringstream capErr;
    auto* old = std::cerr.rdbuf(capErr.rdbuf());
    int rc = cmdRag({"query", "--max-tokens", "0", "some question"});
    std::cerr.rdbuf(old);

    EXPECT_EQ(rc, 1);
    EXPECT_NE(capErr.str().find("HTTP 400"), std::string::npos);
    EXPECT_NE(capErr.str().find("max_tokens must be greater than 0"), std::string::npos);

    const auto err_json = extractFirstJsonObject(capErr.str());
    ASSERT_TRUE(err_json.has_value());
    EXPECT_EQ(err_json->value("error", std::string{}), "max_tokens must be greater than 0");
    EXPECT_EQ(err_json->value("status", 0), 400);
}

// TRQ-08: top_k=0 is syntactically valid for CLI but rejected by server contract (rc=1)
TEST_F(ThemisctlHttpTest, TRQ08_RagQuery_TopKZero_ServerRejectsWith400) {
    std::ostringstream capErr;
    auto* old = std::cerr.rdbuf(capErr.rdbuf());
    int rc = cmdRag({"query", "--top-k", "0", "some question"});
    std::cerr.rdbuf(old);

    EXPECT_EQ(rc, 1);
    EXPECT_NE(capErr.str().find("HTTP 400"), std::string::npos);
    EXPECT_NE(capErr.str().find("top_k out of range"), std::string::npos);

    const auto err_json = extractFirstJsonObject(capErr.str());
    ASSERT_TRUE(err_json.has_value());
    EXPECT_EQ(err_json->value("error", std::string{}), "top_k out of range");
    EXPECT_EQ(err_json->value("status", 0), 400);
    EXPECT_TRUE(err_json->contains("details"));
}

// TRQ-09: missing retrieval engine must fail closed with explicit 503
TEST_F(ThemisctlHttpTest, TRQ09_RagQuery_MissingEngine_ServerRejectsWith503) {
    std::ostringstream capErr;
    auto* old = std::cerr.rdbuf(capErr.rdbuf());
    int rc = cmdRag({"query", "--collection", "missing_engine", "some question"});
    std::cerr.rdbuf(old);

    EXPECT_EQ(rc, 1);
    EXPECT_NE(capErr.str().find("HTTP 503"), std::string::npos);
    EXPECT_NE(capErr.str().find("RAG retrieval engine not configured"), std::string::npos);

    const auto err_json = extractFirstJsonObject(capErr.str());
    ASSERT_TRUE(err_json.has_value());
    EXPECT_EQ(err_json->value("error", std::string{}), "RAG retrieval engine not configured");
    EXPECT_EQ(err_json->value("status", 0), 503);
    EXPECT_TRUE(err_json->contains("details"));
}

// TRQ-10: invalid collection must fail closed with explicit 503
TEST_F(ThemisctlHttpTest, TRQ10_RagQuery_InvalidCollection_ServerRejectsWith503) {
    std::ostringstream capErr;
    auto* old = std::cerr.rdbuf(capErr.rdbuf());
    int rc = cmdRag({"query", "--collection", "invalid_collection", "some question"});
    std::cerr.rdbuf(old);

    EXPECT_EQ(rc, 1);
    EXPECT_NE(capErr.str().find("HTTP 503"), std::string::npos);
    EXPECT_NE(capErr.str().find("RAG retrieval failed"), std::string::npos);

    const auto err_json = extractFirstJsonObject(capErr.str());
    ASSERT_TRUE(err_json.has_value());
    EXPECT_EQ(err_json->value("error", std::string{}), "RAG retrieval failed");
    EXPECT_EQ(err_json->value("status", 0), 503);
    EXPECT_TRUE(err_json->contains("details"));
}

// TRQ-11: empty retrieval must fail closed with no-usable-documents reason
TEST_F(ThemisctlHttpTest, TRQ11_RagQuery_EmptyRetrieval_ServerRejectsWith503) {
    std::ostringstream capErr;
    auto* old = std::cerr.rdbuf(capErr.rdbuf());
    int rc = cmdRag({"query", "--collection", "empty_collection", "some question"});
    std::cerr.rdbuf(old);

    EXPECT_EQ(rc, 1);
    EXPECT_NE(capErr.str().find("HTTP 503"), std::string::npos);
    EXPECT_NE(capErr.str().find("RAG retrieval returned no usable documents"), std::string::npos);

    const auto err_json = extractFirstJsonObject(capErr.str());
    ASSERT_TRUE(err_json.has_value());
    EXPECT_EQ(err_json->value("error", std::string{}), "RAG retrieval returned no usable documents");
    EXPECT_EQ(err_json->value("status", 0), 503);
    EXPECT_TRUE(err_json->contains("details"));
}

// TRQ-12: empty query payload must fail closed with HTTP 400 from server
TEST_F(ThemisctlHttpTest, TRQ12_RagQuery_EmptyStringQuestion_ServerRejectsWith400) {
    std::ostringstream capErr;
    auto* old = std::cerr.rdbuf(capErr.rdbuf());
    int rc = cmdRag({"query", ""});
    std::cerr.rdbuf(old);

    EXPECT_EQ(rc, 1);
    EXPECT_NE(capErr.str().find("HTTP 400"), std::string::npos);
    EXPECT_NE(capErr.str().find("Missing 'query' field"), std::string::npos);

    const auto err_json = extractFirstJsonObject(capErr.str());
    ASSERT_TRUE(err_json.has_value());
    EXPECT_EQ(err_json->value("error", std::string{}), "Missing 'query' field");
    EXPECT_EQ(err_json->value("status", 0), 400);
}

// TRQ-13: response budget is capped by explicit max_tokens in RAG contract response
TEST_F(ThemisctlHttpTest, TRQ13_RagQuery_RawJson_ResponseBudgetCappedByMaxTokens) {
    Response r = httpPost(
        "/api/v1/llm/rag",
        json{{"query", "budget cap check"},
             {"collection", "procs"},
             {"top_k", 3},
             {"response_budget_tokens", 300},
             {"max_tokens", 64}}.dump());

    ASSERT_EQ(r.status, 200) << r.body;
    json j;
    ASSERT_NO_THROW(j = json::parse(r.body));
    ASSERT_TRUE(j.contains("response_budget_tokens_effective"));
    EXPECT_EQ(j.value("response_budget_tokens_effective", 0), 64);
}

// TRQ-14: iterative rag_mode keeps budget cap semantics (effective budget <= max_tokens)
TEST_F(ThemisctlHttpTest, TRQ14_RagQuery_RawJson_IterativeMode_ResponseBudgetCappedByMaxTokens) {
    Response r = httpPost(
        "/api/v1/llm/rag",
        json{{"query", "iterative budget cap check"},
             {"collection", "procs"},
             {"rag_mode", "iterative"},
             {"response_budget_tokens", 400},
             {"max_tokens", 32}}.dump());

    ASSERT_EQ(r.status, 200) << r.body;
    json j;
    ASSERT_NO_THROW(j = json::parse(r.body));
    ASSERT_TRUE(j.contains("rag_mode_effective"));
    ASSERT_TRUE(j.contains("response_budget_tokens_effective"));
    EXPECT_EQ(j.value("rag_mode_effective", std::string{}), "iterative");
    EXPECT_EQ(j.value("response_budget_tokens_effective", 0), 32);
}

// TRQ-15: cmdRag forwards rag-mode and budget flags; server contract returns capped effective budget
TEST_F(ThemisctlHttpTest, TRQ15_RagQuery_CliFlags_IterativeBudgetForwarding) {
    g_ctx.raw_json = true;
    std::ostringstream capOut;
    auto* old = std::cout.rdbuf(capOut.rdbuf());
    int rc = cmdRag({"query",
                     "--collection", "procs",
                     "--rag-mode", "iterative",
                     "--response-budget-tokens", "400",
                     "--max-tokens", "32",
                     "Check forwarding"});
    std::cout.rdbuf(old);
    g_ctx.raw_json = false;

    EXPECT_EQ(rc, 0);
    json j;
    ASSERT_NO_THROW(j = json::parse(capOut.str()));
    EXPECT_EQ(j.value("rag_mode_effective", std::string{}), "iterative");
    EXPECT_EQ(j.value("response_budget_tokens_effective", 0), 32);
}

TEST_F(ThemisctlHttpTest, TRQ21_RagQuery_CliFlags_RagModeCaseInsensitiveNormalization) {
    g_ctx.raw_json = true;
    std::ostringstream capOut;
    auto* old = std::cout.rdbuf(capOut.rdbuf());
    int rc = cmdRag({"query",
                     "--collection", "procs",
                     "--rag-mode", "Iterative",
                     "Case normalization"});
    std::cout.rdbuf(old);
    g_ctx.raw_json = false;

    EXPECT_EQ(rc, 0);
    json j;
    ASSERT_NO_THROW(j = json::parse(capOut.str()));
    EXPECT_EQ(j.value("rag_mode_effective", std::string{}), "iterative");
}

TEST_F(ThemisctlHttpTest, TRQ22_RagQuery_CliFlags_RagModeMapReduceAliasNormalization) {
    g_ctx.raw_json = true;
    std::ostringstream capOut;
    auto* old = std::cout.rdbuf(capOut.rdbuf());
    int rc = cmdRag({"query",
                     "--collection", "procs",
                     "--rag-mode", "map-reduce",
                     "Alias normalization"});
    std::cout.rdbuf(old);
    g_ctx.raw_json = false;

    EXPECT_EQ(rc, 0);
    json j;
    ASSERT_NO_THROW(j = json::parse(capOut.str()));
    EXPECT_EQ(j.value("rag_mode_effective", std::string{}), "map_reduce");
}

TEST_F(ThemisctlHttpTest, TRQ23_RagQuery_CliFlags_RagModeMapReduceWordAliasNormalization) {
    g_ctx.raw_json = true;
    std::ostringstream capOut;
    auto* old = std::cout.rdbuf(capOut.rdbuf());
    int rc = cmdRag({"query",
                     "--collection", "procs",
                     "--rag-mode", "mapreduce",
                     "Word alias normalization"});
    std::cout.rdbuf(old);
    g_ctx.raw_json = false;

    EXPECT_EQ(rc, 0);
    json j;
    ASSERT_NO_THROW(j = json::parse(capOut.str()));
    EXPECT_EQ(j.value("rag_mode_effective", std::string{}), "map_reduce");
}
