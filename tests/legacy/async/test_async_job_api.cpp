#include <gtest/gtest.h>

#include "server/async_job_api_handler.h"

#include <chrono>
#include <stdexcept>
#include <thread>
#include <nlohmann/json.hpp>
#include <boost/beast/http.hpp>

using namespace themis::server;
namespace http = boost::beast::http;
using json     = nlohmann::json;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/// Build a minimal POST /v2/jobs request with the given JSON body.
static http::request<http::string_body>
make_submit_req(const json& body,
                const std::string& auth = "Bearer test-token")
{
    http::request<http::string_body> req{http::verb::post, "/v2/jobs", 11};
    req.set(http::field::content_type,  "application/json");
    req.set(http::field::authorization, auth);
    req.body() = body.dump();
    req.prepare_payload();
    return req;
}

/// Build a GET /v2/jobs/{id} request.
static http::request<http::string_body>
make_status_req(const std::string& job_id)
{
    http::request<http::string_body> req{http::verb::get, "/v2/jobs/" + job_id, 11};
    req.set(http::field::authorization, "Bearer test-token");
    req.prepare_payload();
    return req;
}

/// Build a DELETE /v2/jobs/{id} request.
static http::request<http::string_body>
make_cancel_req(const std::string& job_id)
{
    http::request<http::string_body> req{http::verb::delete_, "/v2/jobs/" + job_id, 11};
    req.set(http::field::authorization, "Bearer test-token");
    req.prepare_payload();
    return req;
}

/// Build a GET /v2/jobs request.
static http::request<http::string_body>
make_list_req()
{
    http::request<http::string_body> req{http::verb::get, "/v2/jobs", 11};
    req.set(http::field::authorization, "Bearer test-token");
    req.prepare_payload();
    return req;
}

// ---------------------------------------------------------------------------
// asyncJobStatusToString
// ---------------------------------------------------------------------------

TEST(AsyncJobStatusTest, AllStatusStrings) {
    EXPECT_EQ(asyncJobStatusToString(AsyncJobStatus::PENDING),   "pending");
    EXPECT_EQ(asyncJobStatusToString(AsyncJobStatus::RUNNING),   "running");
    EXPECT_EQ(asyncJobStatusToString(AsyncJobStatus::COMPLETED), "completed");
    EXPECT_EQ(asyncJobStatusToString(AsyncJobStatus::FAILED),    "failed");
    EXPECT_EQ(asyncJobStatusToString(AsyncJobStatus::CANCELLED), "cancelled");
}

// ---------------------------------------------------------------------------
// AsyncJobRecord::toJson
// ---------------------------------------------------------------------------

TEST(AsyncJobRecordTest, ToJsonPending) {
    AsyncJobRecord rec;
    rec.id         = "job-abc";
    rec.query      = "FOR x IN col RETURN x";
    rec.status     = AsyncJobStatus::PENDING;
    rec.created_at = std::chrono::system_clock::now();
    rec.updated_at = rec.created_at;

    auto j = rec.toJson();
    EXPECT_EQ(j["job_id"].get<std::string>(), "job-abc");
    EXPECT_EQ(j["status"].get<std::string>(), "pending");
    EXPECT_EQ(j["query"].get<std::string>(), "FOR x IN col RETURN x");
    EXPECT_FALSE(j.contains("result"));
    EXPECT_FALSE(j.contains("error"));
}

TEST(AsyncJobRecordTest, ToJsonCompleted) {
    AsyncJobRecord rec;
    rec.id         = "job-done";
    rec.query      = "FOR x IN col RETURN x";
    rec.status     = AsyncJobStatus::COMPLETED;
    rec.result     = json::array({"row1", "row2"});
    rec.created_at = std::chrono::system_clock::now();
    rec.updated_at = rec.created_at;

    auto j = rec.toJson();
    EXPECT_EQ(j["status"].get<std::string>(), "completed");
    ASSERT_TRUE(j.contains("result"));
    EXPECT_EQ(j["result"].size(), 2u);
    EXPECT_FALSE(j.contains("error"));
}

TEST(AsyncJobRecordTest, ToJsonFailed) {
    AsyncJobRecord rec;
    rec.id         = "job-err";
    rec.query      = "INVALID";
    rec.status     = AsyncJobStatus::FAILED;
    rec.error      = "parse error at column 1";
    rec.created_at = std::chrono::system_clock::now();
    rec.updated_at = rec.created_at;

    auto j = rec.toJson();
    EXPECT_EQ(j["status"].get<std::string>(), "failed");
    ASSERT_TRUE(j.contains("error"));
    EXPECT_EQ(j["error"].get<std::string>(), "parse error at column 1");
    EXPECT_FALSE(j.contains("result"));
}

// ---------------------------------------------------------------------------
// AsyncJobRegistry
// ---------------------------------------------------------------------------

TEST(AsyncJobRegistryTest, AddAndGet) {
    AsyncJobRegistry reg;

    auto job    = std::make_shared<AsyncJobRecord>();
    job->id     = "job-001";
    job->status = AsyncJobStatus::PENDING;
    job->created_at = job->updated_at = std::chrono::system_clock::now();
    reg.add(job);

    auto found = reg.get("job-001");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->id, "job-001");
}

TEST(AsyncJobRegistryTest, GetMissingReturnsNullptr) {
    AsyncJobRegistry reg;
    EXPECT_EQ(reg.get("nonexistent"), nullptr);
}

TEST(AsyncJobRegistryTest, AllReturnsAllJobs) {
    AsyncJobRegistry reg;
    for (int i = 0; i < 3; ++i) {
        auto job    = std::make_shared<AsyncJobRecord>();
        job->id     = "job-" + std::to_string(i);
        job->status = AsyncJobStatus::PENDING;
        job->created_at = job->updated_at = std::chrono::system_clock::now();
        reg.add(job);
    }
    EXPECT_EQ(reg.all().size(), 3u);
}

TEST(AsyncJobRegistryTest, PruneRemovesExpiredTerminalJobs) {
    // TTL of 0 seconds – any terminal job is immediately expired
    AsyncJobRegistry reg{std::chrono::seconds(0)};

    auto job        = std::make_shared<AsyncJobRecord>();
    job->id         = "job-old";
    job->status     = AsyncJobStatus::COMPLETED;
    // updated_at 2 seconds in the past
    job->updated_at = std::chrono::system_clock::now() - std::chrono::seconds(2);
    job->created_at = job->updated_at;

    {
        // Bypass internal TTL prune by inserting directly via add() with
        // a fresh job first so the expired job is actually cleaned up.
        auto dummy    = std::make_shared<AsyncJobRecord>();
        dummy->id     = "trigger";
        dummy->status = AsyncJobStatus::PENDING;
        dummy->created_at = dummy->updated_at = std::chrono::system_clock::now();
        // Add expired job manually: re-use an explicit prune call
        reg.add(job);  // adds with prune
    }

    // After the prune triggered by add(), the expired completed job is gone.
    EXPECT_EQ(reg.get("job-old"), nullptr);
}

TEST(AsyncJobRegistryTest, PruneKeepsRunningJobs) {
    AsyncJobRegistry reg{std::chrono::seconds(0)};

    auto job        = std::make_shared<AsyncJobRecord>();
    job->id         = "running-job";
    job->status     = AsyncJobStatus::RUNNING;
    job->updated_at = std::chrono::system_clock::now() - std::chrono::seconds(2);
    job->created_at = job->updated_at;
    reg.add(job);

    // RUNNING jobs must survive even when TTL=0
    ASSERT_NE(reg.get("running-job"), nullptr);
}

// ---------------------------------------------------------------------------
// AsyncJobApiHandler – handleSubmit
// ---------------------------------------------------------------------------

class AsyncJobApiHandlerTest : public ::testing::Test {
protected:
    // Synchronous instant executor – returns a JSON array immediately.
    static AsyncJobApiHandler::AqlExecutor instant_ok_executor() {
        return [](const std::string&, const std::string&) -> json {
            return json::array({"row1", "row2"});
        };
    }

    // Executor that always throws.
    static AsyncJobApiHandler::AqlExecutor failing_executor() {
        return [](const std::string&, const std::string&) -> json {
            throw std::runtime_error("AQL parse error");
        };
    }

    // Executor that blocks until the cancel flag is set.
    static AsyncJobApiHandler::AqlExecutor slow_executor() {
        return [](const std::string&, const std::string&) -> json {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            return json::array();
        };
    }
};

TEST_F(AsyncJobApiHandlerTest, SubmitReturnsMissingQuery) {
    AsyncJobApiHandler handler{instant_ok_executor()};
    auto req = make_submit_req({{"not_a_query", "x"}});
    auto res = handler.handleSubmit(req);

    EXPECT_EQ(res.result(), http::status::bad_request);
    auto body = json::parse(res.body());
    EXPECT_TRUE(body["error"].get<bool>());
    EXPECT_NE(body["message"].get<std::string>().find("query"), std::string::npos);
}

TEST_F(AsyncJobApiHandlerTest, SubmitRejectsEmptyQuery) {
    AsyncJobApiHandler handler{instant_ok_executor()};
    auto req = make_submit_req({{"query", ""}});
    auto res = handler.handleSubmit(req);

    EXPECT_EQ(res.result(), http::status::bad_request);
    auto body = json::parse(res.body());
    EXPECT_TRUE(body["error"].get<bool>());
    EXPECT_NE(body["message"].get<std::string>().find("empty"), std::string::npos);
}

TEST_F(AsyncJobApiHandlerTest, SubmitRejectsUnsafeQuery) {
    AsyncJobApiHandler handler{instant_ok_executor()};
    auto req = make_submit_req({{"query", "FOR x IN col RETURN x; DROP TABLE users"}});
    auto res = handler.handleSubmit(req);

    EXPECT_EQ(res.result(), http::status::bad_request);
    auto body = json::parse(res.body());
    EXPECT_TRUE(body["error"].get<bool>());
}

TEST_F(AsyncJobApiHandlerTest, SubmitReturnsBadJson) {
    AsyncJobApiHandler handler{instant_ok_executor()};

    http::request<http::string_body> req{http::verb::post, "/v2/jobs", 11};
    req.set(http::field::content_type,  "application/json");
    req.set(http::field::authorization, "Bearer tok");
    req.body() = "not json";
    req.prepare_payload();

    auto res = handler.handleSubmit(req);
    EXPECT_EQ(res.result(), http::status::bad_request);
}

TEST_F(AsyncJobApiHandlerTest, SubmitReturns202WithJobId) {
    AsyncJobApiHandler handler{instant_ok_executor()};
    auto req = make_submit_req({{"query", "FOR x IN col RETURN x"}});
    auto res = handler.handleSubmit(req);

    ASSERT_EQ(res.result(), http::status::accepted);
    auto body = json::parse(res.body());
    ASSERT_TRUE(body.contains("job_id"));
    EXPECT_FALSE(body["job_id"].get<std::string>().empty());
    EXPECT_EQ(body["status"].get<std::string>(), "pending");
}

// ---------------------------------------------------------------------------
// AsyncJobApiHandler – handleGetStatus
// ---------------------------------------------------------------------------

TEST_F(AsyncJobApiHandlerTest, GetStatusNotFound) {
    AsyncJobApiHandler handler{instant_ok_executor()};
    auto res = handler.handleGetStatus(make_status_req("nonexistent-id"));

    EXPECT_EQ(res.result(), http::status::not_found);
    auto body = json::parse(res.body());
    EXPECT_TRUE(body["error"].get<bool>());
}

TEST_F(AsyncJobApiHandlerTest, GetStatusMissingId) {
    AsyncJobApiHandler handler{instant_ok_executor()};

    http::request<http::string_body> req{http::verb::get, "/v2/jobs/", 11};
    req.set(http::field::authorization, "Bearer tok");
    req.prepare_payload();

    auto res = handler.handleGetStatus(req);
    EXPECT_EQ(res.result(), http::status::bad_request);
}

TEST_F(AsyncJobApiHandlerTest, GetStatusInvalidId) {
    AsyncJobApiHandler handler{instant_ok_executor()};
    auto res = handler.handleGetStatus(make_status_req("../bad-id"));

    EXPECT_EQ(res.result(), http::status::bad_request);
}

TEST_F(AsyncJobApiHandlerTest, GetStatusEventuallyCompleted) {
    AsyncJobApiHandler handler{instant_ok_executor()};

    // Submit
    auto sub = handler.handleSubmit(
        make_submit_req({{"query", "FOR x IN col RETURN x"}}));
    ASSERT_EQ(sub.result(), http::status::accepted);
    auto job_id = json::parse(sub.body())["job_id"].get<std::string>();

    // Poll until completed (max 2 s)
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    std::string status_str;
    while (std::chrono::steady_clock::now() < deadline) {
        auto res  = handler.handleGetStatus(make_status_req(job_id));
        auto body = json::parse(res.body());
        status_str = body["status"].get<std::string>();
        if (status_str == "completed" || status_str == "failed") {
          break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_EQ(status_str, "completed");
}

TEST_F(AsyncJobApiHandlerTest, CancelInvalidId) {
    AsyncJobApiHandler handler{instant_ok_executor()};
    auto res = handler.handleCancel(make_cancel_req("../bad-id"));

    EXPECT_EQ(res.result(), http::status::bad_request);
    auto body = json::parse(res.body());
    EXPECT_TRUE(body["error"].get<bool>());
}

TEST_F(AsyncJobApiHandlerTest, GetStatusFailed) {
    AsyncJobApiHandler handler{failing_executor()};

    auto sub = handler.handleSubmit(
        make_submit_req({{"query", "INVALID"}}));
    ASSERT_EQ(sub.result(), http::status::accepted);
    auto job_id = json::parse(sub.body())["job_id"].get<std::string>();

    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    std::string status_str;
    while (std::chrono::steady_clock::now() < deadline) {
        auto res  = handler.handleGetStatus(make_status_req(job_id));
        auto body = json::parse(res.body());
        status_str = body["status"].get<std::string>();
        if (status_str == "completed" || status_str == "failed") {
          break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_EQ(status_str, "failed");
    auto res  = handler.handleGetStatus(make_status_req(job_id));
    auto body = json::parse(res.body());
    ASSERT_TRUE(body.contains("error"));
    EXPECT_FALSE(body["error"].get<std::string>().empty());
}

// ---------------------------------------------------------------------------
// AsyncJobApiHandler – handleList
// ---------------------------------------------------------------------------

TEST_F(AsyncJobApiHandlerTest, ListStartsEmpty) {
    AsyncJobApiHandler handler{instant_ok_executor()};
    auto res = handler.handleList(make_list_req());

    EXPECT_EQ(res.result(), http::status::ok);
    auto body = json::parse(res.body());
    EXPECT_TRUE(body.is_array());
    EXPECT_EQ(body.size(), 0u);
}

TEST_F(AsyncJobApiHandlerTest, ListReturnsSubmittedJob) {
    AsyncJobApiHandler handler{instant_ok_executor()};

    handler.handleSubmit(make_submit_req({{"query", "FOR x IN c RETURN x"}}));

    auto res  = handler.handleList(make_list_req());
    auto body = json::parse(res.body());
    EXPECT_GE(body.size(), 1u);
}

// ---------------------------------------------------------------------------
// AsyncJobApiHandler – handleCancel
// ---------------------------------------------------------------------------

TEST_F(AsyncJobApiHandlerTest, CancelNotFound) {
    AsyncJobApiHandler handler{instant_ok_executor()};
    auto res = handler.handleCancel(make_cancel_req("ghost-job"));
    EXPECT_EQ(res.result(), http::status::not_found);
}

TEST_F(AsyncJobApiHandlerTest, CancelMissingId) {
    AsyncJobApiHandler handler{instant_ok_executor()};

    http::request<http::string_body> req{http::verb::delete_, "/v2/jobs/", 11};
    req.set(http::field::authorization, "Bearer tok");
    req.prepare_payload();

    auto res = handler.handleCancel(req);
    EXPECT_EQ(res.result(), http::status::bad_request);
}

TEST_F(AsyncJobApiHandlerTest, CancelPendingJobImmediately) {
    // Use a slow executor so the job is still PENDING when we cancel.
    AsyncJobApiHandler handler{slow_executor()};

    // Don't submit via the handler (it launches immediately) – instead
    // add a PENDING job directly to the registry.
    auto registry = std::make_shared<AsyncJobRegistry>();
    AsyncJobApiHandler handler2{slow_executor(), nullptr, registry};

    auto job        = std::make_shared<AsyncJobRecord>();
    job->id         = "job-pending-cancel";
    job->query      = "FOR x IN c RETURN x";
    job->status     = AsyncJobStatus::PENDING;
    job->created_at = job->updated_at = std::chrono::system_clock::now();
    registry->add(job);

    auto res  = handler2.handleCancel(make_cancel_req("job-pending-cancel"));
    ASSERT_EQ(res.result(), http::status::ok);
    auto body = json::parse(res.body());
    EXPECT_EQ(body["status"].get<std::string>(), "cancelled");
}

TEST_F(AsyncJobApiHandlerTest, CancelTerminalJobReturnsConflict) {
    auto registry = std::make_shared<AsyncJobRegistry>();
    AsyncJobApiHandler handler{instant_ok_executor(), nullptr, registry};

    auto job        = std::make_shared<AsyncJobRecord>();
    job->id         = "job-done";
    job->status     = AsyncJobStatus::COMPLETED;
    job->created_at = job->updated_at = std::chrono::system_clock::now();
    registry->add(job);

    auto res = handler.handleCancel(make_cancel_req("job-done"));
    EXPECT_EQ(res.result(), http::status::conflict);
    auto body = json::parse(res.body());
    EXPECT_TRUE(body["error"].get<bool>());
}

// ---------------------------------------------------------------------------
// Multiple concurrent jobs
// ---------------------------------------------------------------------------

TEST_F(AsyncJobApiHandlerTest, MultipleConcurrentJobs) {
    AsyncJobApiHandler handler{instant_ok_executor()};

    constexpr int kJobs = 5;
    std::vector<std::string> job_ids;
    for (int i = 0; i < kJobs; ++i) {
        auto sub = handler.handleSubmit(
            make_submit_req({{"query", "FOR x IN c RETURN x"}}));
        ASSERT_EQ(sub.result(), http::status::accepted);
        job_ids.push_back(json::parse(sub.body())["job_id"].get<std::string>());
    }

    // Wait for all jobs to finish
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    for (const auto& jid : job_ids) {
        while (std::chrono::steady_clock::now() < deadline) {
            auto res  = handler.handleGetStatus(make_status_req(jid));
            auto body = json::parse(res.body());
            auto st   = body["status"].get<std::string>();
            if (st == "completed" || st == "failed" || st == "cancelled") {
              break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        auto res  = handler.handleGetStatus(make_status_req(jid));
        auto body = json::parse(res.body());
        EXPECT_EQ(body["status"].get<std::string>(), "completed")
            << "Job " << jid << " did not complete in time";
    }
}
