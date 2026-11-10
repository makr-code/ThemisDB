/**
 * @file test_peft_subprocess_backend.cpp
 * @brief Focused tests for PeftSubprocessTrainingBackend: request validation,
 *        JSONL/job export, protocol parsing, argv building, and real
 *        subprocess runs against a small protocol-emitting Python fixture
 *        (test fixture only; it does not train anything).
 */

// Copyright 2025 ThemisDB
// Licensed under MIT License

#include "llm/lora_framework/peft_subprocess_backend.h"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <atomic>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <functional>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace fs = std::filesystem;
using json = nlohmann::json;
using themis::llm::lora::AdapterTrainingMethod;
using themis::llm::lora::PeftSubprocessBackendConfig;
using themis::llm::lora::PeftSubprocessTrainingBackend;
using themis::llm::lora::ProtocolMessage;
using themis::llm::lora::TrainingJobRequest;
using themis::llm::lora::TrainingProgress;
using Backend = PeftSubprocessTrainingBackend;

namespace {

class TempDir {
public:
    TempDir() {
        std::random_device rd;
        path_ = fs::temp_directory_path() / ("themis_peft_backend_" + std::to_string(rd()) + std::to_string(rd()));
        fs::create_directories(path_);
    }
    ~TempDir() {
        std::error_code ec;
        fs::remove_all(path_, ec);
    }
    const fs::path& path() const { return path_; }

private:
    fs::path path_;
};

void writeFile(const fs::path& p, const std::string& text) {
    std::ofstream out(p, std::ios::binary);
    out << text;
}

std::string readFile(const fs::path& p) {
    std::ifstream in(p, std::ios::binary);
    std::stringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

std::vector<std::string> readLines(const fs::path& p) {
    std::ifstream in(p);
    std::vector<std::string> lines;
    for (std::string l; std::getline(in, l);) {
        lines.push_back(l);
    }
    return lines;
}

TrainingJobRequest makeRequest(const fs::path& work_root, std::size_t n = 10) {
    TrainingJobRequest r;
    r.job_id = "job-1";
    r.adapter_id = "legal_de.v1";
    for (std::size_t i = 0; i < n; ++i) {
        themis::llm::lora::TrainingDataSample s;
        s.input = "question " + std::to_string(i);
        s.output = "answer " + std::to_string(i);
        r.data.samples.push_back(s);
    }
    r.data.dataset_name = "ds-1";
    r.base_model = "org/base";
    r.base_model_gguf = "/models/base.gguf";
    r.output_dir = (work_root / "legal_de.v1" / "job-1").string();
    r.holdout_fraction = 0.2;
    r.seed = 7;
    return r;
}

/// Locate a python3 interpreter for the subprocess tests (no PATH lookup magic).
std::string findPython() {
    for (const char* p : {"/usr/bin/python3", "/usr/local/bin/python3", "/bin/python3"}) {
        std::error_code ec;
        if (fs::is_regular_file(p, ec)) {
            return p;
        }
    }
    return {};
}

// Protocol-emitting fixture. Modes are selected via job.json "adapter_id"
// (training) or the presence of "package_dir" (convert-only).
constexpr const char* kFakeTrainer = R"PY(
import json, os, sys, time
args = sys.argv[1:]
cfg = json.load(open(args[args.index("--config") + 1]))
out = sys.stdout
def emit(prefix, obj):
    out.write(prefix + json.dumps(obj) + "\n"); out.flush()
sys.stderr.write("fixture stderr line\n"); sys.stderr.flush()
if "--convert-only" in args:
    with open(cfg["output_gguf"], "wb") as fh: fh.write(b"GGUF")
    emit("THEMIS_RESULT ", {"success": True, "error": "", "gguf_adapter_path": cfg["output_gguf"],
                            "metadata": cfg["themis_metadata"]})
    sys.exit(0)
mode = cfg["adapter_id"]
if mode == "hang":
    emit("THEMIS_PROGRESS ", {"progress": 0.1, "step": 1, "total_steps": 10, "train_loss": 2.0,
                              "eval_loss": -1.0, "phase": "train"})
    time.sleep(600)
if mode == "crash":
    sys.stderr.write("Traceback: boom\n"); sys.exit(1)
if mode == "noimprove":
    emit("THEMIS_RESULT ", {"success": False, "error": "adapter did not improve", "eval_loss": 2.0,
                            "baseline_eval_loss": 1.9, "train_samples": 8, "eval_samples": 2})
    sys.exit(3)
train = [json.loads(l) for l in open(cfg["train_file"]) if l.strip()]
evals = [json.loads(l) for l in open(cfg["eval_file"]) if l.strip()]
emit("THEMIS_PROGRESS ", {"progress": 0.5, "step": 1, "total_steps": 2, "train_loss": 1.5,
                          "eval_loss": -1.0, "phase": "train"})
print("stray stdout line")
d = cfg["output_dir"]
os.makedirs(os.path.join(d, "package"), exist_ok=True)
st = os.path.join(d, "package", "adapter_model.safetensors")
gg = os.path.join(d, "adapter.gguf")
open(st, "wb").write(b"x"); open(gg, "wb").write(b"GGUF")
emit("THEMIS_RESULT ", {"success": True, "error": "", "gguf_adapter_path": gg, "safetensors_path": st,
                        "manifest_path": os.path.join(d, "adapter.manifest.json"),
                        "peft_adapter_dir": os.path.join(d, "peft"), "train_loss": 1.25,
                        "eval_loss": 1.5, "eval_perplexity": 4.48, "baseline_eval_loss": None,
                        "train_samples": len(train), "eval_samples": len(evals),
                        "rank_pattern": {"m.q_proj": 4}, "echo_job": cfg})
)PY";

class BackendFixture : public ::testing::Test {
protected:
    void SetUp() override {
        work_root = tmp.path() / "work";
        fs::create_directories(work_root);
        script = tmp.path() / "fake_trainer.py";
        writeFile(script, kFakeTrainer);
        convert = tmp.path() / "convert_lora_to_gguf.py";
        writeFile(convert, "# placeholder path; fixture never runs it\n");
        cfg.python_executable = findPython();
        cfg.script_path = script.string();
        cfg.convert_script_path = convert.string();
        cfg.base_model = "org/base";
        cfg.work_root = work_root.string();
        cfg.kill_grace = std::chrono::seconds(1);
    }

    TempDir tmp;
    fs::path work_root;
    fs::path script;
    fs::path convert;
    PeftSubprocessBackendConfig cfg;
};

}  // namespace

// ── Validation ───────────────────────────────────────────────────────────────

TEST_F(BackendFixture, ValidRequestPasses) {
    EXPECT_EQ(Backend::validateRequest(makeRequest(work_root), cfg), "");
}

TEST_F(BackendFixture, ValidationErrors) {
    struct Case {
        const char* expect;
        std::function<void(TrainingJobRequest&, PeftSubprocessBackendConfig&)> mutate;
    };
    const std::vector<Case> cases = {
        {"adapter_id", [](auto& r, auto&) { r.adapter_id = "../evil"; }},
        {"adapter_id", [](auto& r, auto&) { r.adapter_id = std::string(129, 'a'); }},
        {"adapter_id", [](auto& r, auto&) { r.adapter_id = ""; }},
        {"at least 2", [](auto& r, auto&) { r.data.samples.resize(1); }},
        {"empty input", [](auto& r, auto&) { r.data.samples[3].output = "  "; }},
        {"holdout", [](auto& r, auto&) { r.holdout_fraction = 0.0; }},
        {"holdout", [](auto& r, auto&) { r.holdout_fraction = 0.51; }},
        {"rank", [](auto& r, auto&) { r.hyperparameters.rank = 0; }},
        {"alpha", [](auto& r, auto&) { r.hyperparameters.alpha = 0.0f; }},
        {"dropout", [](auto& r, auto&) { r.hyperparameters.dropout = 1.0f; }},
        {"learning_rate", [](auto& r, auto&) { r.hyperparameters.learning_rate = -1.0f; }},
        {"num_epochs", [](auto& r, auto&) { r.hyperparameters.num_epochs = 0; }},
        {"batch_size", [](auto& r, auto&) { r.hyperparameters.batch_size = 0; }},
        {"max_seq_length", [](auto& r, auto&) { r.hyperparameters.max_seq_length = 4; }},
        {"target_rank", [](auto& r, auto&) { r.adalora.target_rank = r.adalora.init_rank + 1; }},
        {"delta_t", [](auto& r, auto&) { r.adalora.delta_t = 0; }},
        {"beta1", [](auto& r, auto&) { r.adalora.beta1 = 1.0; }},
        {"base_model", [](auto& r, auto& c) { r.base_model.clear(); c.base_model.clear(); }},
        {"not inside work_root", [](auto& r, auto&) { r.output_dir = "/tmp/elsewhere"; }},
        {"not inside work_root", [](auto& r, auto& c) { r.output_dir = c.work_root + "/../escape"; }},
        {"not inside work_root", [](auto& r, auto& c) { r.output_dir = c.work_root; }},
        {"output_dir", [](auto& r, auto&) { r.output_dir.clear(); }},
        {"trainer script", [](auto&, auto& c) { c.script_path = "/nonexistent/train.py"; }},
        {"convert_lora_to_gguf", [](auto&, auto& c) { c.convert_script_path.clear(); }},
    };
    for (const auto& tc : cases) {
        auto r = makeRequest(work_root);
        auto c = cfg;
        tc.mutate(r, c);
        const std::string err = Backend::validateRequest(r, c);
        EXPECT_NE(err.find(tc.expect), std::string::npos) << "expected '" << tc.expect << "', got '" << err << "'";
    }
}

TEST_F(BackendFixture, AdaLoraChecksSkippedForPlainLora) {
    auto r = makeRequest(work_root);
    r.method = AdapterTrainingMethod::LoRA;
    r.adalora.delta_t = 0;
    EXPECT_EQ(Backend::validateRequest(r, cfg), "");
}

TEST_F(BackendFixture, TrainRejectsInvalidRequestWithoutSpawning) {
    Backend backend(cfg);
    auto r = makeRequest(work_root);
    r.adapter_id = "bad id";
    std::atomic<bool> cancel{false};
    const auto res = backend.train(r, {}, cancel);
    EXPECT_FALSE(res.success);
    EXPECT_NE(res.error.find("invalid training request"), std::string::npos);
    EXPECT_TRUE(std::isnan(res.eval_loss));
    EXPECT_FALSE(fs::exists(r.output_dir));
}

TEST(PeftSubprocessBackendPaths, IsWithinRoot) {
    TempDir t;
    EXPECT_TRUE(Backend::isWithinRoot(t.path() / "a" / "b", t.path()));
    EXPECT_FALSE(Backend::isWithinRoot(t.path(), t.path()));
    EXPECT_FALSE(Backend::isWithinRoot(t.path() / "a" / ".." / "..", t.path()));
    EXPECT_FALSE(Backend::isWithinRoot(t.path().string() + "-sibling", t.path()));
    EXPECT_FALSE(Backend::isWithinRoot("", t.path()));
#ifndef _WIN32
    fs::create_directories(t.path() / "outside");
    fs::create_directories(t.path() / "root");
    fs::create_directory_symlink(t.path() / "outside", t.path() / "root" / "link");
    EXPECT_FALSE(Backend::isWithinRoot(t.path() / "root" / "link" / "x", t.path() / "root"));
#endif
}

TEST(PeftSubprocessBackendIds, AdapterIdRule) {
    EXPECT_TRUE(Backend::isValidAdapterId("A-z_0.9"));
    EXPECT_TRUE(Backend::isValidAdapterId(std::string(128, 'x')));
    EXPECT_FALSE(Backend::isValidAdapterId("a/b"));
    EXPECT_FALSE(Backend::isValidAdapterId("a b"));
    EXPECT_FALSE(Backend::isValidAdapterId("ä"));
}

// ── Split & export ──────────────────────────────────────────────────────────

TEST(PeftSubprocessBackendSplit, DeterministicDisjointComplete) {
    const auto a = Backend::splitTrainEval(100, 0.1, 42);
    const auto b = Backend::splitTrainEval(100, 0.1, 42);
    const auto c = Backend::splitTrainEval(100, 0.1, 43);
    EXPECT_EQ(a.eval, b.eval);
    EXPECT_EQ(a.train, b.train);
    EXPECT_NE(a.eval, c.eval);
    ASSERT_EQ(a.eval.size(), 10u);
    ASSERT_EQ(a.train.size(), 90u);
    std::vector<bool> seen(100, false);
    for (auto i : a.train) seen[i] = true;
    for (auto i : a.eval) {
        EXPECT_FALSE(seen[i]);
        seen[i] = true;
    }
    for (bool s : seen) EXPECT_TRUE(s);
}

TEST(PeftSubprocessBackendSplit, ClampsToAtLeastOneEach) {
    const auto s = Backend::splitTrainEval(2, 0.01, 1);
    EXPECT_EQ(s.eval.size(), 1u);
    EXPECT_EQ(s.train.size(), 1u);
    EXPECT_TRUE(Backend::splitTrainEval(1, 0.1, 1).train.empty());
    EXPECT_TRUE(Backend::splitTrainEval(10, 0.6, 1).eval.empty());
}

TEST(PeftSubprocessBackendExport, JsonlFormat) {
    TempDir t;
    std::vector<themis::llm::lora::TrainingDataSample> samples(3);
    samples[0].input = "q0";
    samples[0].output = "a0";
    samples[1].input = "q1 \"quoted\"\nline";
    samples[1].output = "a1";
    samples[1].metadata = {{"context", {"doc A", "doc B"}}, {"system", "be precise"}, {"other", 1}};
    samples[2].input = "q2";
    samples[2].output = "a2";
    samples[2].metadata = {{"context", {"doc", 5}}};  // not all strings -> ignored
    std::string err;
    const auto path = t.path() / "x.jsonl";
    ASSERT_TRUE(Backend::writeJsonl(path, samples, {2, 1, 0}, &err)) << err;
    const auto lines = readLines(path);
    ASSERT_EQ(lines.size(), 3u);
    EXPECT_EQ(json::parse(lines[0]), (json{{"input", "q2"}, {"output", "a2"}}));
    EXPECT_EQ(json::parse(lines[1]), (json{{"input", "q1 \"quoted\"\nline"},
                                           {"output", "a1"},
                                           {"context", {"doc A", "doc B"}},
                                           {"system", "be precise"}}));
    EXPECT_EQ(json::parse(lines[2]), (json{{"input", "q0"}, {"output", "a0"}}));

    EXPECT_FALSE(Backend::writeJsonl(path, samples, {3}, &err));
    EXPECT_NE(err.find("out of range"), std::string::npos);
    samples[0].input = std::string("bad \xff utf8");
    EXPECT_FALSE(Backend::writeJsonl(path, samples, {0}, &err));
    EXPECT_NE(err.find("UTF-8"), std::string::npos);
}

TEST_F(BackendFixture, JobJsonFields) {
    auto r = makeRequest(work_root);
    r.hyperparameters.learning_rate = 3e-4f;
    r.hyperparameters.target_modules = {"q_proj", "v_proj"};
    r.data.metadata = {{"themis.adapter.version", "5"},
                       {"themis.training.snapshot.merkle_root", "abc"},
                       {"themis.provenance.key_id", "k1"},
                       {"themis.nested", {{"x", 1}}},
                       {"unrelated", "drop"}};
    const json j = Backend::buildJobJson(r, cfg, "/w/train.jsonl", "/w/eval.jsonl", "/w/out");
    EXPECT_EQ(j["method"], "adalora");
    EXPECT_EQ(j["adapter_version"], "5");
    EXPECT_EQ(j["dataset_id"], "ds-1");
    EXPECT_EQ(j["base_model_gguf"], "/models/base.gguf");
    EXPECT_DOUBLE_EQ(j["learning_rate"].get<double>(), 3e-4);
    EXPECT_EQ(j["target_modules"], (json{"q_proj", "v_proj"}));
    EXPECT_EQ(j["adalora"]["deltaT"], r.adalora.delta_t);
    EXPECT_EQ(j["convert_script"], cfg.convert_script_path);
    EXPECT_EQ(j["seed"], 7);
    EXPECT_EQ(j["themis_metadata"],
              (json{{"themis.adapter.version", "5"},
                    {"themis.training.snapshot.merkle_root", "abc"},
                    {"themis.provenance.key_id", "k1"}}));
    r.method = AdapterTrainingMethod::LoRA;
    EXPECT_EQ(Backend::buildJobJson(r, cfg, "a", "b", "c")["method"], "lora");
}

TEST(PeftSubprocessBackendArgv, NoShellArgvList) {
    PeftSubprocessBackendConfig c;
    c.python_executable = "/opt/py/bin/python3";
    c.script_path = "/opt/themis/scripts/lora/themis_lora_train.py";
    const auto a = Backend::buildArgv(c, "/w/dir with space; rm -rf/job.json", false);
    EXPECT_EQ(a, (std::vector<std::string>{"/opt/py/bin/python3", "/opt/themis/scripts/lora/themis_lora_train.py",
                                           "--config", "/w/dir with space; rm -rf/job.json"}));
    const auto b = Backend::buildArgv(c, "/w/c.json", true);
    EXPECT_EQ(b.back(), "--convert-only");
}

// ── Protocol parsing ────────────────────────────────────────────────────────

TEST(PeftSubprocessBackendProtocol, ParseLines) {
    auto p = Backend::parseProtocolLine(
        "THEMIS_PROGRESS {\"progress\":0.25,\"step\":5,\"total_steps\":20,\"train_loss\":1.5,"
        "\"eval_loss\":-1.0,\"phase\":\"train\"}\r\n");
    ASSERT_TRUE(p.has_value());
    EXPECT_EQ(p->kind, ProtocolMessage::Kind::Progress);
    const TrainingProgress tp = Backend::progressFromJson(p->payload);
    EXPECT_FLOAT_EQ(tp.progress, 0.25f);
    EXPECT_EQ(tp.step, 5);
    EXPECT_EQ(tp.total_steps, 20);
    EXPECT_DOUBLE_EQ(tp.train_loss, 1.5);
    EXPECT_DOUBLE_EQ(tp.eval_loss, -1.0);
    EXPECT_EQ(tp.phase, "train");

    auto r = Backend::parseProtocolLine("THEMIS_RESULT {\"success\":true}");
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(r->kind, ProtocolMessage::Kind::Result);

    EXPECT_FALSE(Backend::parseProtocolLine("").has_value());
    EXPECT_FALSE(Backend::parseProtocolLine("loading model...").has_value());
    EXPECT_FALSE(Backend::parseProtocolLine("THEMIS_PROGRESS").has_value());
    EXPECT_FALSE(Backend::parseProtocolLine("THEMIS_PROGRESS {broken").has_value());
    EXPECT_FALSE(Backend::parseProtocolLine("THEMIS_RESULT [1,2]").has_value());
    EXPECT_FALSE(Backend::parseProtocolLine(" THEMIS_RESULT {}").has_value());
}

TEST(PeftSubprocessBackendProtocol, ProgressIsLenientAndClamped) {
    const auto tp = Backend::progressFromJson(json{{"progress", 7.0}, {"step", -3}, {"eval_loss", nullptr}});
    EXPECT_FLOAT_EQ(tp.progress, 1.0f);
    EXPECT_EQ(tp.step, 0);
    EXPECT_DOUBLE_EQ(tp.eval_loss, -1.0);
    EXPECT_TRUE(std::isnan(tp.train_loss));
}

TEST(PeftSubprocessBackendProtocol, ResultNeverFabricatesMetrics) {
    const json payload = {{"success", true},
                          {"error", ""},
                          {"gguf_adapter_path", "/o/adapter.gguf"},
                          {"safetensors_path", "/o/package/adapter_model.safetensors"},
                          {"manifest_path", "/o/adapter.manifest.json"},
                          {"train_loss", 1.0},
                          {"eval_loss", nullptr},
                          {"train_samples", 9},
                          {"eval_samples", -1},
                          {"rank_pattern", {{"m", 3}}}};
    const auto res = Backend::resultFromJson(payload);
    EXPECT_TRUE(res.success);
    EXPECT_EQ(res.gguf_adapter_path, "/o/adapter.gguf");
    EXPECT_DOUBLE_EQ(res.train_loss, 1.0);
    EXPECT_TRUE(std::isnan(res.eval_loss));
    EXPECT_TRUE(std::isnan(res.eval_perplexity));
    EXPECT_TRUE(std::isnan(res.baseline_eval_loss));
    EXPECT_EQ(res.train_samples, 9u);
    EXPECT_EQ(res.eval_samples, 0u);
    EXPECT_EQ(res.rank_pattern, (json{{"m", 3}}));
    EXPECT_EQ(res.raw_report["safetensors_path"], "/o/package/adapter_model.safetensors");
    EXPECT_EQ(res.raw_report["manifest_path"], "/o/adapter.manifest.json");

    const auto bad = Backend::resultFromJson(json{{"success", "yes"}});
    EXPECT_FALSE(bad.success);
    EXPECT_FALSE(bad.error.empty());
}

// ── Real subprocess runs (POSIX; fixture script) ────────────────────────────

#ifndef _WIN32

TEST_F(BackendFixture, TrainEndToEndWithFixtureTrainer) {
    if (cfg.python_executable.empty()) GTEST_SKIP() << "python3 not installed";
    Backend backend(cfg);
    auto r = makeRequest(work_root);
    r.data.samples[0].metadata = {{"context", {"doc 1"}}};
    std::vector<TrainingProgress> seen;
    std::atomic<bool> cancel{false};
    const auto res = backend.train(r, [&](const TrainingProgress& p) { seen.push_back(p); }, cancel);
    ASSERT_TRUE(res.success) << res.error;
    EXPECT_EQ(res.train_samples, 8u);
    EXPECT_EQ(res.eval_samples, 2u);
    EXPECT_DOUBLE_EQ(res.eval_loss, 1.5);
    EXPECT_TRUE(std::isnan(res.baseline_eval_loss));  // null stays unavailable
    EXPECT_TRUE(fs::is_regular_file(res.gguf_adapter_path));
    EXPECT_TRUE(fs::is_regular_file(res.raw_report.at("safetensors_path").get<std::string>()));
    EXPECT_FALSE(res.raw_report.at("manifest_path").get<std::string>().empty());
    EXPECT_EQ(res.raw_report["backend"], "peft-subprocess");
    ASSERT_EQ(seen.size(), 1u);
    EXPECT_EQ(seen[0].phase, "train");
    EXPECT_FLOAT_EQ(seen[0].progress, 0.5f);
    // Job inputs (DB-derived text) are removed after the run.
    EXPECT_FALSE(fs::exists(fs::path(r.output_dir) / "input"));
    const json echoed = res.raw_report.at("echo_job");
    EXPECT_EQ(echoed["adapter_id"], "legal_de.v1");
    EXPECT_EQ(echoed["method"], "adalora");
}

TEST_F(BackendFixture, TrainerCrashReportsStderrTail) {
    if (cfg.python_executable.empty()) GTEST_SKIP() << "python3 not installed";
    Backend backend(cfg);
    auto r = makeRequest(work_root);
    r.adapter_id = "crash";
    std::atomic<bool> cancel{false};
    const auto res = backend.train(r, {}, cancel);
    EXPECT_FALSE(res.success);
    EXPECT_NE(res.error.find("exited with code 1"), std::string::npos) << res.error;
    EXPECT_NE(res.error.find("Traceback: boom"), std::string::npos) << res.error;
}

TEST_F(BackendFixture, NoImprovementIsAFailureWithMeasuredLosses) {
    if (cfg.python_executable.empty()) GTEST_SKIP() << "python3 not installed";
    Backend backend(cfg);
    auto r = makeRequest(work_root);
    r.adapter_id = "noimprove";
    std::atomic<bool> cancel{false};
    const auto res = backend.train(r, {}, cancel);
    EXPECT_FALSE(res.success);
    EXPECT_EQ(res.error, "adapter did not improve");
    EXPECT_DOUBLE_EQ(res.baseline_eval_loss, 1.9);
    EXPECT_EQ(res.raw_report["exit_code"], 3);
}

TEST_F(BackendFixture, CancelTerminatesChild) {
    if (cfg.python_executable.empty()) GTEST_SKIP() << "python3 not installed";
    Backend backend(cfg);
    auto r = makeRequest(work_root);
    r.adapter_id = "hang";
    std::atomic<bool> cancel{false};
    const auto start = std::chrono::steady_clock::now();
    const auto res = backend.train(r, [&](const TrainingProgress&) { cancel = true; }, cancel);
    EXPECT_FALSE(res.success);
    EXPECT_EQ(res.error, "training cancelled");
    EXPECT_LT(std::chrono::steady_clock::now() - start, std::chrono::seconds(30));
}

TEST_F(BackendFixture, TimeoutTerminatesChild) {
    if (cfg.python_executable.empty()) GTEST_SKIP() << "python3 not installed";
    cfg.timeout = std::chrono::seconds(1);
    Backend backend(cfg);
    auto r = makeRequest(work_root);
    r.adapter_id = "hang";
    std::atomic<bool> cancel{false};
    const auto res = backend.train(r, {}, cancel);
    EXPECT_FALSE(res.success);
    EXPECT_NE(res.error.find("timed out"), std::string::npos) << res.error;
}

TEST_F(BackendFixture, MissingInterpreterIsLaunchError) {
    cfg.python_executable = (tmp.path() / "no-such-python").string();
    Backend backend(cfg);
    std::atomic<bool> cancel{false};
    const auto res = backend.train(makeRequest(work_root), {}, cancel);
    EXPECT_FALSE(res.success);
    EXPECT_NE(res.error.find("cannot start trainer"), std::string::npos) << res.error;
}

TEST_F(BackendFixture, ConvertPassesThemisMetadataThrough) {
    if (cfg.python_executable.empty()) GTEST_SKIP() << "python3 not installed";
    const fs::path pkg = tmp.path() / "pkg";
    fs::create_directories(pkg);
    writeFile(pkg / "adapter_model.safetensors", "x");
    writeFile(pkg / "adapter_config.json", "{}");
    Backend backend(cfg);
    std::atomic<bool> cancel{false};
    const json meta = {{"themis.provenance.signature", "ab12"}, {"themis.training.snapshot.hlc_ts", "77"}};
    const auto out = tmp.path() / "out" / "adapter.gguf";
    fs::create_directories(out.parent_path());
    const auto res = backend.convertSafetensorsToGguf(pkg.string(), meta, out.string(), cancel);
    ASSERT_TRUE(res.success) << res.error;
    EXPECT_EQ(res.gguf_adapter_path, out.string());
    EXPECT_EQ(res.raw_report["metadata"], meta);
    EXPECT_EQ(readFile(out), "GGUF");
    // No scratch directories are left behind.
    for (const auto& e : fs::directory_iterator(work_root)) {
        EXPECT_EQ(e.path().filename().string().rfind(".themis-convert-", 0), std::string::npos);
    }
}

#endif  // !_WIN32

TEST_F(BackendFixture, ConvertRejectsBadInputsBeforeSpawning) {
    const fs::path pkg = tmp.path() / "pkg";
    fs::create_directories(pkg);
    writeFile(pkg / "adapter_model.safetensors", "x");
    writeFile(pkg / "adapter_config.json", "{}");
    Backend backend(cfg);
    std::atomic<bool> cancel{false};
    const auto out = (tmp.path() / "a.gguf").string();

    auto res = backend.convertSafetensorsToGguf((tmp.path() / "missing").string(), json::object(), out, cancel);
    EXPECT_NE(res.error.find("not found"), std::string::npos);
    res = backend.convertSafetensorsToGguf(pkg.string(), json{{"general.name", "x"}}, out, cancel);
    EXPECT_NE(res.error.find("must start with 'themis.'"), std::string::npos);
    res = backend.convertSafetensorsToGguf(pkg.string(), json{{"themis.x", json::array()}}, out, cancel);
    EXPECT_NE(res.error.find("string, number or bool"), std::string::npos);
    res = backend.convertSafetensorsToGguf(pkg.string(), json::object(), (tmp.path() / "a.bin").string(), cancel);
    EXPECT_NE(res.error.find(".gguf"), std::string::npos);
    writeFile(pkg / "adapter_model.bin", "pickle");
    res = backend.convertSafetensorsToGguf(pkg.string(), json::object(), out, cancel);
    EXPECT_FALSE(res.success);
    EXPECT_NE(res.error.find("pickle"), std::string::npos);
}
