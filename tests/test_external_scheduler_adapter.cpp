/**
 * @file test_external_scheduler_adapter.cpp
 * @brief Unit tests for ExternalSchedulerAdapter (Kubernetes CronJob and Airflow integration).
 */

#include <gtest/gtest.h>
#include "scheduler/external_scheduler_adapter.h"
#include <chrono>
#include <string>

using namespace themis;
using namespace themis::scheduler;
using namespace std::chrono_literals;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

static ScheduledTask makeIntervalTask(const std::string& id,
                                      std::chrono::milliseconds interval) {
    ScheduledTask t;
    t.id          = id;
    t.name        = "Task " + id;
    t.description = "Auto-generated test task";
    t.type        = ScheduledTask::TaskType::FUNCTION;
    t.function_name = "noop";
    t.trigger_type  = ScheduledTask::TriggerType::INTERVAL;
    t.interval      = interval;
    return t;
}

static ScheduledTask makeCronTask(const std::string& id, const std::string& cron) {
    ScheduledTask t;
    t.id             = id;
    t.name           = "Cron Task " + id;
    t.description    = "Cron-scheduled test task";
    t.type           = ScheduledTask::TaskType::FUNCTION;
    t.function_name  = "noop";
    t.trigger_type   = ScheduledTask::TriggerType::CRON;
    t.cron_expression = cron;
    return t;
}

static KubernetesCronJobConfig makeK8sConfig(const std::string& url = "https://themisdb.example.com") {
    KubernetesCronJobConfig cfg;
    cfg.themisdb_base_url = url;
    cfg.k8s_namespace     = "default";
    cfg.job_image         = "curlimages/curl:8.6.0";
    return cfg;
}

static AirflowDagConfig makeAirflowConfig(const std::string& url = "https://themisdb.example.com") {
    AirflowDagConfig cfg;
    cfg.dag_id            = "test_dag";
    cfg.themisdb_base_url = url;
    cfg.http_conn_id      = "themisdb_default";
    cfg.start_date        = "2026-01-01";
    return cfg;
}

// ─────────────────────────────────────────────────────────────────────────────
// intervalToCron
// ─────────────────────────────────────────────────────────────────────────────

TEST(ExternalSchedulerAdapter, IntervalToCronSubMinuteBecomesEveryMinute) {
    EXPECT_EQ(ExternalSchedulerAdapter::intervalToCron(30s), "* * * * *");
}

TEST(ExternalSchedulerAdapter, IntervalToCronExactlyOneMinute) {
    EXPECT_EQ(ExternalSchedulerAdapter::intervalToCron(1min), "*/1 * * * *");
}

TEST(ExternalSchedulerAdapter, IntervalToCronFiveMinutes) {
    EXPECT_EQ(ExternalSchedulerAdapter::intervalToCron(5min), "*/5 * * * *");
}

TEST(ExternalSchedulerAdapter, IntervalToCronOneHour) {
    EXPECT_EQ(ExternalSchedulerAdapter::intervalToCron(60min), "0 */1 * * *");
}

TEST(ExternalSchedulerAdapter, IntervalToCronSixHours) {
    EXPECT_EQ(ExternalSchedulerAdapter::intervalToCron(6h), "0 */6 * * *");
}

TEST(ExternalSchedulerAdapter, IntervalToCronTwelveHours) {
    EXPECT_EQ(ExternalSchedulerAdapter::intervalToCron(12h), "0 */12 * * *");
}

TEST(ExternalSchedulerAdapter, IntervalToCronExactlyOneDay) {
    EXPECT_EQ(ExternalSchedulerAdapter::intervalToCron(24h), "0 0 * * *");
}

TEST(ExternalSchedulerAdapter, IntervalToCronThreeDays) {
    EXPECT_EQ(ExternalSchedulerAdapter::intervalToCron(72h), "0 0 */3 * *");
}

// ─────────────────────────────────────────────────────────────────────────────
// effectiveCronSchedule
// ─────────────────────────────────────────────────────────────────────────────

TEST(ExternalSchedulerAdapter, EffectiveCronScheduleUsesCronExpressionWhenSet) {
    auto task = makeCronTask("t1", "0 2 * * *");
    EXPECT_EQ(ExternalSchedulerAdapter::effectiveCronSchedule(task), "0 2 * * *");
}

TEST(ExternalSchedulerAdapter, EffectiveCronScheduleFallsBackToInterval) {
    auto task = makeIntervalTask("t2", 30min);
    const std::string sched = ExternalSchedulerAdapter::effectiveCronSchedule(task);
    EXPECT_FALSE(sched.empty());
    EXPECT_EQ(sched, "*/30 * * * *");
}

// ─────────────────────────────────────────────────────────────────────────────
// toK8sName
// ─────────────────────────────────────────────────────────────────────────────

TEST(ExternalSchedulerAdapter, ToK8sNameLowercasesInput) {
    EXPECT_EQ(ExternalSchedulerAdapter::toK8sName("MyTask"), "mytask");
}

TEST(ExternalSchedulerAdapter, ToK8sNameReplacesSpecialCharsWithHyphen) {
    EXPECT_EQ(ExternalSchedulerAdapter::toK8sName("compress_old_data"), "compress-old-data");
}

TEST(ExternalSchedulerAdapter, ToK8sNameCollapsesConsecutiveHyphens) {
    EXPECT_EQ(ExternalSchedulerAdapter::toK8sName("a__b"), "a-b");
}

TEST(ExternalSchedulerAdapter, ToK8sNameTrimsLeadingTrailingHyphens) {
    EXPECT_EQ(ExternalSchedulerAdapter::toK8sName("_task_"), "task");
}

TEST(ExternalSchedulerAdapter, ToK8sNameTruncatesTo52Characters) {
    const std::string long_name(100, 'a');
    const std::string result = ExternalSchedulerAdapter::toK8sName(long_name);
    EXPECT_LE(result.size(), 52u);
}

TEST(ExternalSchedulerAdapter, ToK8sNameAllSpecialCharsFallback) {
    EXPECT_EQ(ExternalSchedulerAdapter::toK8sName("!!!"), "task");
}

// ─────────────────────────────────────────────────────────────────────────────
// toKubernetesCronJobJson – validation
// ─────────────────────────────────────────────────────────────────────────────

TEST(ExternalSchedulerAdapter, ToK8sJsonThrowsOnEmptyTaskId) {
    ExternalSchedulerAdapter adapter;
    ScheduledTask task = makeIntervalTask("", 5min);
    auto cfg = makeK8sConfig();
    EXPECT_THROW(adapter.toKubernetesCronJobJson(task, cfg), std::invalid_argument);
}

TEST(ExternalSchedulerAdapter, ToK8sJsonThrowsOnEmptyBaseUrl) {
    ExternalSchedulerAdapter adapter;
    ScheduledTask task = makeIntervalTask("my-task", 5min);
    KubernetesCronJobConfig cfg;
    cfg.themisdb_base_url = ""; // empty
    EXPECT_THROW(adapter.toKubernetesCronJobJson(task, cfg), std::invalid_argument);
}

// ─────────────────────────────────────────────────────────────────────────────
// toKubernetesCronJobJson – structure
// ─────────────────────────────────────────────────────────────────────────────

TEST(ExternalSchedulerAdapter, ToK8sJsonReturnsCorrectApiVersion) {
    ExternalSchedulerAdapter adapter;
    auto task = makeIntervalTask("compress-data", 5min);
    auto manifest = adapter.toKubernetesCronJobJson(task, makeK8sConfig());

    ASSERT_TRUE(manifest.contains("apiVersion"));
    EXPECT_EQ(manifest["apiVersion"].get<std::string>(), "batch/v1");
}

TEST(ExternalSchedulerAdapter, ToK8sJsonReturnsCorrectKind) {
    ExternalSchedulerAdapter adapter;
    auto task = makeIntervalTask("compress-data", 5min);
    auto manifest = adapter.toKubernetesCronJobJson(task, makeK8sConfig());

    ASSERT_TRUE(manifest.contains("kind"));
    EXPECT_EQ(manifest["kind"].get<std::string>(), "CronJob");
}

TEST(ExternalSchedulerAdapter, ToK8sJsonMetadataContainsName) {
    ExternalSchedulerAdapter adapter;
    auto task = makeIntervalTask("compress-data", 5min);
    auto manifest = adapter.toKubernetesCronJobJson(task, makeK8sConfig());

    ASSERT_TRUE(manifest.contains("metadata"));
    const auto& meta = manifest["metadata"];
    ASSERT_TRUE(meta.contains("name"));
    const std::string name = meta["name"].get<std::string>();
    // Name must be lowercase and DNS-safe
    EXPECT_FALSE(name.empty());
    for (char c : name) {
        EXPECT_TRUE(std::isalnum(static_cast<unsigned char>(c)) || c == '-')
            << "Unexpected character in K8s name: " << c;
    }
}

TEST(ExternalSchedulerAdapter, ToK8sJsonSpecContainsCronSchedule) {
    ExternalSchedulerAdapter adapter;
    auto task = makeCronTask("daily-backup", "0 0 * * *");
    auto manifest = adapter.toKubernetesCronJobJson(task, makeK8sConfig());

    ASSERT_TRUE(manifest.contains("spec"));
    ASSERT_TRUE(manifest["spec"].contains("schedule"));
    EXPECT_EQ(manifest["spec"]["schedule"].get<std::string>(), "0 0 * * *");
}

TEST(ExternalSchedulerAdapter, ToK8sJsonIntervalTaskUsesConvertedSchedule) {
    ExternalSchedulerAdapter adapter;
    auto task = makeIntervalTask("hourly-task", 60min);
    auto manifest = adapter.toKubernetesCronJobJson(task, makeK8sConfig());

    const std::string sched = manifest["spec"]["schedule"].get<std::string>();
    EXPECT_FALSE(sched.empty());
    // Should be "0 */1 * * *" for 1-hour interval
    EXPECT_EQ(sched, "0 */1 * * *");
}

TEST(ExternalSchedulerAdapter, ToK8sJsonContainsJobTemplateWithContainer) {
    ExternalSchedulerAdapter adapter;
    auto task = makeIntervalTask("test-task", 5min);
    auto manifest = adapter.toKubernetesCronJobJson(task, makeK8sConfig());

    ASSERT_TRUE(manifest["spec"].contains("jobTemplate"));
    const auto& jt = manifest["spec"]["jobTemplate"];
    ASSERT_TRUE(jt.contains("spec"));
    const auto& containers = jt["spec"]["template"]["spec"]["containers"];
    ASSERT_TRUE(containers.is_array());
    ASSERT_FALSE(containers.empty());
    EXPECT_EQ(containers[0]["image"].get<std::string>(), "curlimages/curl:8.6.0");
}

TEST(ExternalSchedulerAdapter, ToK8sJsonContainerCommandContainsTaskId) {
    ExternalSchedulerAdapter adapter;
    auto task = makeIntervalTask("my-special-task", 5min);
    auto manifest = adapter.toKubernetesCronJobJson(task, makeK8sConfig());

    const auto& container = manifest["spec"]["jobTemplate"]["spec"]["template"]["spec"]["containers"][0];
    ASSERT_TRUE(container.contains("command"));
    const auto& cmd = container["command"];
    bool found = false;
    for (const auto& part : cmd) {
        if (part.is_string() &&
            part.get<std::string>().find("my-special-task") != std::string::npos) {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found) << "Task ID not found in container command";
}

TEST(ExternalSchedulerAdapter, ToK8sJsonWithAuthSecretAddsEnvVar) {
    ExternalSchedulerAdapter adapter;
    auto task = makeIntervalTask("secure-task", 5min);
    KubernetesCronJobConfig cfg = makeK8sConfig();
    cfg.api_token_secret_name = "themisdb-api-token";
    auto manifest = adapter.toKubernetesCronJobJson(task, cfg);

    const auto& container = manifest["spec"]["jobTemplate"]["spec"]["template"]["spec"]["containers"][0];
    ASSERT_TRUE(container.contains("env"));
    bool found_env = false;
    for (const auto& env : container["env"]) {
        if (env.contains("name") && env["name"].get<std::string>() == "THEMISDB_API_TOKEN") {
            found_env = true;
            break;
        }
    }
    EXPECT_TRUE(found_env) << "THEMISDB_API_TOKEN env var not found in manifest";
}

TEST(ExternalSchedulerAdapter, ToK8sJsonSuspendFlagPropagated) {
    ExternalSchedulerAdapter adapter;
    auto task = makeIntervalTask("paused-task", 5min);
    KubernetesCronJobConfig cfg = makeK8sConfig();
    cfg.suspend = true;
    auto manifest = adapter.toKubernetesCronJobJson(task, cfg);

    ASSERT_TRUE(manifest["spec"].contains("suspend"));
    EXPECT_TRUE(manifest["spec"]["suspend"].get<bool>());
}

TEST(ExternalSchedulerAdapter, ToK8sJsonNamespaceIsSet) {
    ExternalSchedulerAdapter adapter;
    auto task = makeIntervalTask("ns-task", 5min);
    KubernetesCronJobConfig cfg = makeK8sConfig();
    cfg.k8s_namespace = "production";
    auto manifest = adapter.toKubernetesCronJobJson(task, cfg);

    EXPECT_EQ(manifest["metadata"]["namespace"].get<std::string>(), "production");
}

TEST(ExternalSchedulerAdapter, ToK8sJsonAnnotationsContainTaskMetadata) {
    ExternalSchedulerAdapter adapter;
    ScheduledTask task = makeCronTask("annotated-task", "@daily");
    task.name        = "Annotated Task";
    task.description = "Has a description";
    auto manifest = adapter.toKubernetesCronJobJson(task, makeK8sConfig());

    const auto& ann = manifest["metadata"]["annotations"];
    EXPECT_EQ(ann["themisdb/task-name"].get<std::string>(), "Annotated Task");
    EXPECT_EQ(ann["themisdb/task-description"].get<std::string>(), "Has a description");
}

// ─────────────────────────────────────────────────────────────────────────────
// toKubernetesCronJobYaml
// ─────────────────────────────────────────────────────────────────────────────

TEST(ExternalSchedulerAdapter, ToK8sYamlContainsApiVersion) {
    ExternalSchedulerAdapter adapter;
    auto task  = makeIntervalTask("yaml-task", 5min);
    auto yaml  = adapter.toKubernetesCronJobYaml(task, makeK8sConfig());
    EXPECT_NE(yaml.find("apiVersion"), std::string::npos) << yaml;
}

TEST(ExternalSchedulerAdapter, ToK8sYamlContainsCronJob) {
    ExternalSchedulerAdapter adapter;
    auto task = makeIntervalTask("yaml-task2", 5min);
    auto yaml = adapter.toKubernetesCronJobYaml(task, makeK8sConfig());
    EXPECT_NE(yaml.find("CronJob"), std::string::npos) << yaml;
}

// ─────────────────────────────────────────────────────────────────────────────
// fromKubernetesCronJobJson
// ─────────────────────────────────────────────────────────────────────────────

TEST(ExternalSchedulerAdapter, FromK8sJsonRoundTripPreservesSchedule) {
    ExternalSchedulerAdapter adapter;
    auto original = makeCronTask("roundtrip-task", "0 3 * * 1");
    original.name        = "Round-trip";
    original.description = "Test";
    auto manifest = adapter.toKubernetesCronJobJson(original, makeK8sConfig());
    auto restored = adapter.fromKubernetesCronJobJson(manifest);

    EXPECT_EQ(restored.cron_expression, "0 3 * * 1");
    EXPECT_EQ(restored.trigger_type,    ScheduledTask::TriggerType::CRON);
}

TEST(ExternalSchedulerAdapter, FromK8sJsonPreservesTaskId) {
    ExternalSchedulerAdapter adapter;
    auto original = makeCronTask("original-id", "@daily");
    auto manifest = adapter.toKubernetesCronJobJson(original, makeK8sConfig());
    auto restored = adapter.fromKubernetesCronJobJson(manifest);

    EXPECT_EQ(restored.id, "original-id");
}

TEST(ExternalSchedulerAdapter, FromK8sJsonPreservesTaskName) {
    ExternalSchedulerAdapter adapter;
    ScheduledTask original = makeCronTask("named-task", "@daily");
    original.name = "My Named Task";
    auto manifest = adapter.toKubernetesCronJobJson(original, makeK8sConfig());
    auto restored = adapter.fromKubernetesCronJobJson(manifest);

    EXPECT_EQ(restored.name, "My Named Task");
}

TEST(ExternalSchedulerAdapter, FromK8sJsonThrowsOnMissingMetadata) {
    ExternalSchedulerAdapter adapter;
    nlohmann::json bad = {{"spec", {{"schedule", "* * * * *"}}}};
    EXPECT_THROW(adapter.fromKubernetesCronJobJson(bad), std::invalid_argument);
}

TEST(ExternalSchedulerAdapter, FromK8sJsonThrowsOnMissingSchedule) {
    ExternalSchedulerAdapter adapter;
    nlohmann::json bad = {{"metadata", {{"name", "my-job"}}},
                          {"spec", nlohmann::json::object()}};
    EXPECT_THROW(adapter.fromKubernetesCronJobJson(bad), std::invalid_argument);
}

// ─────────────────────────────────────────────────────────────────────────────
// toAirflowDagPython – validation
// ─────────────────────────────────────────────────────────────────────────────

TEST(ExternalSchedulerAdapter, ToAirflowDagThrowsOnEmptyTasks) {
    ExternalSchedulerAdapter adapter;
    EXPECT_THROW(
        adapter.toAirflowDagPython({}, makeAirflowConfig()),
        std::invalid_argument);
}

// ─────────────────────────────────────────────────────────────────────────────
// toAirflowDagPython – structure
// ─────────────────────────────────────────────────────────────────────────────

TEST(ExternalSchedulerAdapter, ToAirflowDagContainsDagId) {
    ExternalSchedulerAdapter adapter;
    auto cfg = makeAirflowConfig();
    cfg.dag_id = "my_test_dag";
    auto task = makeIntervalTask("task1", 5min);
    auto py = adapter.toAirflowDagPython({task}, cfg);
    EXPECT_NE(py.find("my_test_dag"), std::string::npos) << py;
}

TEST(ExternalSchedulerAdapter, ToAirflowDagImportsAirflow) {
    ExternalSchedulerAdapter adapter;
    auto task = makeIntervalTask("task1", 5min);
    auto py = adapter.toAirflowDagPython({task}, makeAirflowConfig());
    EXPECT_NE(py.find("from airflow import DAG"), std::string::npos) << py;
}

TEST(ExternalSchedulerAdapter, ToAirflowDagImportsSimpleHttpOperator) {
    ExternalSchedulerAdapter adapter;
    auto task = makeIntervalTask("task1", 5min);
    auto py = adapter.toAirflowDagPython({task}, makeAirflowConfig());
    EXPECT_NE(py.find("SimpleHttpOperator"), std::string::npos) << py;
}

TEST(ExternalSchedulerAdapter, ToAirflowDagContainsTaskEndpoint) {
    ExternalSchedulerAdapter adapter;
    auto task = makeIntervalTask("my_task", 5min);
    auto py = adapter.toAirflowDagPython({task}, makeAirflowConfig());
    EXPECT_NE(py.find("/api/v1/scheduler/tasks/my_task/execute"), std::string::npos) << py;
}

TEST(ExternalSchedulerAdapter, ToAirflowDagContainsHttpConnId) {
    ExternalSchedulerAdapter adapter;
    auto cfg = makeAirflowConfig();
    cfg.http_conn_id = "my_themisdb_conn";
    auto task = makeIntervalTask("task1", 5min);
    auto py = adapter.toAirflowDagPython({task}, cfg);
    EXPECT_NE(py.find("my_themisdb_conn"), std::string::npos) << py;
}

TEST(ExternalSchedulerAdapter, ToAirflowDagContainsStartDate) {
    ExternalSchedulerAdapter adapter;
    auto cfg = makeAirflowConfig();
    cfg.start_date = "2026-03-01";
    auto task = makeIntervalTask("task1", 5min);
    auto py = adapter.toAirflowDagPython({task}, cfg);
    EXPECT_NE(py.find("2026-03-01"), std::string::npos) << py;
}

TEST(ExternalSchedulerAdapter, ToAirflowDagMultipleTasksHaveOperators) {
    ExternalSchedulerAdapter adapter;
    auto t1 = makeIntervalTask("extract_task", 1h);
    auto t2 = makeIntervalTask("load_task", 1h);
    auto py = adapter.toAirflowDagPython({t1, t2}, makeAirflowConfig());
    EXPECT_NE(py.find("extract_task"), std::string::npos) << py;
    EXPECT_NE(py.find("load_task"),    std::string::npos) << py;
}

TEST(ExternalSchedulerAdapter, ToAirflowDagDependencyWiring) {
    ExternalSchedulerAdapter adapter;
    auto t1 = makeIntervalTask("extract", 1h);
    auto t2 = makeIntervalTask("transform", 1h);
    t2.dependencies = {"extract"};
    auto t3 = makeIntervalTask("load", 1h);
    t3.dependencies = {"transform"};

    auto py = adapter.toAirflowDagPython({t1, t2, t3}, makeAirflowConfig());
    // Dependency arrows should be present
    EXPECT_NE(py.find("extract >> transform"), std::string::npos) << py;
    EXPECT_NE(py.find("transform >> load"),    std::string::npos) << py;
}

TEST(ExternalSchedulerAdapter, ToAirflowDagCronScheduleUsedWhenAllSame) {
    ExternalSchedulerAdapter adapter;
    auto t1 = makeCronTask("task_a", "0 6 * * *");
    auto t2 = makeCronTask("task_b", "0 6 * * *");
    auto py = adapter.toAirflowDagPython({t1, t2}, makeAirflowConfig());
    EXPECT_NE(py.find("0 6 * * *"), std::string::npos) << py;
}

TEST(ExternalSchedulerAdapter, ToAirflowDagUsesDefaultScheduleForMixedTasks) {
    ExternalSchedulerAdapter adapter;
    auto t1 = makeCronTask("task_a", "0 6 * * *");
    auto t2 = makeCronTask("task_b", "0 9 * * *"); // different schedule
    auto cfg = makeAirflowConfig();
    cfg.default_schedule = "@daily";
    auto py = adapter.toAirflowDagPython({t1, t2}, cfg);
    EXPECT_NE(py.find("@daily"), std::string::npos) << py;
}

TEST(ExternalSchedulerAdapter, ToAirflowDagIsPausedFlagPropagated) {
    ExternalSchedulerAdapter adapter;
    auto task = makeIntervalTask("task1", 5min);
    auto cfg = makeAirflowConfig();
    cfg.is_paused_upon_creation = true;
    auto py = adapter.toAirflowDagPython({task}, cfg);
    EXPECT_NE(py.find("is_paused_upon_creation=True"), std::string::npos) << py;
}

TEST(ExternalSchedulerAdapter, ToAirflowDagTagsIncluded) {
    ExternalSchedulerAdapter adapter;
    auto task = makeIntervalTask("task1", 5min);
    auto cfg = makeAirflowConfig();
    cfg.tags = {"themisdb", "scheduler"};
    auto py = adapter.toAirflowDagPython({task}, cfg);
    EXPECT_NE(py.find("themisdb"), std::string::npos) << py;
    EXPECT_NE(py.find("scheduler"), std::string::npos) << py;
}

TEST(ExternalSchedulerAdapter, ToAirflowDagConnectionHintIncludedWhenBaseUrlSet) {
    ExternalSchedulerAdapter adapter;
    auto task = makeIntervalTask("task1", 5min);
    auto cfg = makeAirflowConfig("https://themisdb.prod.example.com");
    auto py = adapter.toAirflowDagPython({task}, cfg);
    // The generated file should contain the connection setup hint with the base URL
    EXPECT_NE(py.find("https://themisdb.prod.example.com"), std::string::npos) << py;
    EXPECT_NE(py.find("Connection setup"), std::string::npos) << py;
}

TEST(ExternalSchedulerAdapter, ToAirflowDagNoConnectionHintWhenBaseUrlEmpty) {
    ExternalSchedulerAdapter adapter;
    auto task = makeIntervalTask("task1", 5min);
    AirflowDagConfig cfg;
    cfg.dag_id       = "test_dag";
    cfg.http_conn_id = "themisdb_default";
    cfg.start_date   = "2026-01-01";
    // themisdb_base_url intentionally left empty
    auto py = adapter.toAirflowDagPython({task}, cfg);
    EXPECT_EQ(py.find("Connection setup"), std::string::npos) << py;
}
