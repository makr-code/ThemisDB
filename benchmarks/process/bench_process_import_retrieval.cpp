/**
 * @file bench_process_import_retrieval.cpp
 * @brief Import-time, retrieval-latency, and prompt-size benchmarks for the process module
 *
 * Validates:
 *   PROCESS-PERF: Import-Zeit, Retrieval-Latenz, Prompt-Size Benchmark
 *
 * Scenarios:
 *   - BPMN XML parsing / import throughput (varying node count)
 *   - EPK text import throughput
 *   - ProcessModelManager: save() + load() round-trip latency
 *   - ProcessModelManager: list() scan throughput over N stored models
 *   - LlmProcessDescriptor: generate() prompt assembly time
 *   - LlmProcessDescriptor: buildSystemPrompt() size vs. generation latency
 *   - LlmProcessDescriptor: summarizeList() over N models
 *
 * Self-contained — ProcessModelManager tests use a temporary RocksDB path under /tmp.
 */

#include <benchmark/benchmark.h>
#include "process/bpmn_serializer.h"
#include "process/epk_serializer.h"
#include "process/process_model_manager.h"
#include "process/llm_process_descriptor.h"
#include "storage/rocksdb_wrapper.h"

#include <filesystem>
#include <sstream>
#include <string>
#ifndef _WIN32
#  include <unistd.h>
#endif
#include <vector>

namespace fs = std::filesystem;
using namespace themis::process;
using namespace themis;

// ─── helpers ─────────────────────────────────────────────────────────────────

/// Generate a minimal, well-formed BPMN 2.0 XML fragment.
static std::string makeBpmnXml(int model_idx, int node_count) {
    std::ostringstream oss;
    oss << R"(<?xml version="1.0" encoding="UTF-8"?>)"
        << R"(<definitions xmlns="http://www.omg.org/spec/BPMN/20100524/MODEL")"
        << R"( targetNamespace="http://themisdb.io/bench">)"
        << "<process id=\"proc_" << model_idx << "\" name=\"Verwaltungsvorgang "
        << model_idx << "\" isExecutable=\"false\">"
        << "<startEvent id=\"start_" << model_idx << "\" name=\"Eingang\"/>";

    for (int i = 0; i < node_count; ++i) {
        oss << "<userTask id=\"task_" << model_idx << "_" << i
            << "\" name=\"Pruefschritt " << i << "\"/>"
            << "<sequenceFlow id=\"sf_" << model_idx << "_" << i
            << "\" sourceRef=\"" << (i == 0 ? "start_" + std::to_string(model_idx)
                                             : "task_" + std::to_string(model_idx) +
                                               "_" + std::to_string(i - 1))
            << "\" targetRef=\"task_" << model_idx << "_" << i << "\"/>";
    }

    oss << "<endEvent id=\"end_" << model_idx << "\" name=\"Abschluss\"/>"
        << "<sequenceFlow id=\"sf_" << model_idx << "_end"
        << "\" sourceRef=\"task_" << model_idx << "_" << (node_count - 1)
        << "\" targetRef=\"end_" << model_idx << "\"/>"
        << "</process>"
        << "</definitions>";

    return oss.str();
}

/// Generate a minimal EPK text fragment (simple line-based notation).
static std::string makeEpkText(int model_idx, int event_count) {
    std::ostringstream oss;
    oss << "PROCESS: epk_" << model_idx << " \"EPK Prozess " << model_idx << "\"\n";
    oss << "EVENT: \"Eingang " << model_idx << "\"\n";
    for (int i = 0; i < event_count - 1; ++i) {
        oss << "FUNCTION: \"Schritt " << i << "\"\n";
        oss << "EVENT: \"Zwischenergebnis " << i << "\"\n";
    }
    oss << "EVENT: \"Abschluss " << model_idx << "\"\n";
    return oss.str();
}

/// Build a synthetic ProcessModelRecord for testing descriptor generation.
static ProcessModelRecord makeRecord(int idx, int node_count = 5) {
    ProcessModelRecord rec;
    rec.id               = "bench_proc_" + std::to_string(idx);
    rec.name             = "Verwaltungsvorgang " + std::to_string(idx);
    rec.description      = "Automatisierter Prozess fuer Benchmark-Zwecke. "
                           "Dieser Prozess enthaelt " + std::to_string(node_count) +
                           " Schritte und ist gemaess DSGVO und VwVfG ausgelegt.";
    rec.notation         = ProcessNotation::BPMN_2_0;
    rec.domain           = ProcessDomain::ADMINISTRATION;
    rec.state            = ProcessModelState::ACTIVE;
    rec.version          = "1." + std::to_string(idx) + ".0";
    rec.raw_payload      = makeBpmnXml(idx, node_count);
    rec.compliance_tags  = {"DSGVO", "VwVfG", "§34 BauO"};
    return rec;
}

// ─── 1. BPMN import throughput — varying node counts ─────────────────────────

static void BM_BpmnImport_NodeCount(benchmark::State& state) {
    const int kNodes = static_cast<int>(state.range(0));

    for (auto _ : state) {
        std::string xml = makeBpmnXml(0, kNodes);
        auto result = BpmnSerializer::importXml(xml);
        benchmark::DoNotOptimize(result.ok);
    }

    state.SetItemsProcessed(state.iterations());
    state.SetLabel("BPMN importXml " + std::to_string(kNodes) + " nodes");
}
BENCHMARK(BM_BpmnImport_NodeCount)
    ->Arg(5)->Arg(20)->Arg(50)->Arg(100)->Arg(200)
    ->Unit(benchmark::kMicrosecond);

// ─── 2. BPMN export throughput ────────────────────────────────────────────────

static void BM_BpmnExport(benchmark::State& state) {
    const int kNodes = static_cast<int>(state.range(0));
    std::string xml = makeBpmnXml(0, kNodes);
    auto result = BpmnSerializer::importXml(xml);

    if (!result.ok) {
        state.SkipWithError("BPMN importXml failed in setup");
        return;
    }

    for (auto _ : state) {
        auto out = BpmnSerializer::exportXml(
            result.process_id, result.process_name, result.nodes, result.edges);
        benchmark::DoNotOptimize(out.size());
    }

    state.SetItemsProcessed(state.iterations());
    state.SetLabel("BPMN exportXml " + std::to_string(kNodes) + " nodes");
}
BENCHMARK(BM_BpmnExport)->Arg(5)->Arg(20)->Arg(100)->Unit(benchmark::kMicrosecond);

// ─── 3. EPK text import throughput ───────────────────────────────────────────

static void BM_EpkImport_EventCount(benchmark::State& state) {
    const int kEvents = static_cast<int>(state.range(0));

    for (auto _ : state) {
        std::string epk_text = makeEpkText(0, kEvents);
        auto result = EpkSerializer::importText(epk_text);
        benchmark::DoNotOptimize(result.ok);
    }

    state.SetItemsProcessed(state.iterations());
    state.SetLabel("EPK importText " + std::to_string(kEvents) + " events");
}
BENCHMARK(BM_EpkImport_EventCount)
    ->Arg(5)->Arg(20)->Arg(50)->Arg(100)
    ->Unit(benchmark::kMicrosecond);

// ─── 4. ProcessModelManager: save() + load() round-trip ─────────────────────

class ProcessManagerFixture : public benchmark::Fixture {
public:
    void SetUp(const benchmark::State& /*s*/) override {
        db_path_ = "/tmp/bench_process_mgr_" + std::to_string(::getpid());
        fs::remove_all(db_path_);

        RocksDBWrapper::Config cfg;
        cfg.db_path    = db_path_;
        cfg.enable_wal = false;
        db_ = std::make_unique<RocksDBWrapper>(cfg);
        if (!db_->open()) {
            throw std::runtime_error("Cannot open RocksDB for process benchmark");
        }

        mgr_ = std::make_unique<ProcessModelManager>(*db_);
    }

    void TearDown(const benchmark::State& /*s*/) override {
        mgr_.reset();
        db_->close();
        db_.reset();
        fs::remove_all(db_path_);
    }

    std::string                          db_path_;
    std::unique_ptr<RocksDBWrapper>      db_;
    std::unique_ptr<ProcessModelManager> mgr_;
};

BENCHMARK_F(ProcessManagerFixture, SaveLoad_RoundTrip)(benchmark::State& state) {
    auto rec = makeRecord(0);

    for (auto _ : state) {
        auto save_result = mgr_->save(rec);
        benchmark::DoNotOptimize(save_result.ok);
        auto loaded = mgr_->load(rec.id);
        benchmark::DoNotOptimize(loaded.has_value());
    }

    state.SetItemsProcessed(state.iterations());
    state.SetLabel("save() + load() round-trip via RocksDB");
}

BENCHMARK_F(ProcessManagerFixture, Save_Throughput)(benchmark::State& state) {
    int idx = 0;

    for (auto _ : state) {
        auto rec = makeRecord(idx++);
        auto ok  = mgr_->save(rec);
        benchmark::DoNotOptimize(ok.ok);
    }

    state.SetItemsProcessed(state.iterations());
    state.SetLabel("save() throughput (sequential)");
}

BENCHMARK_F(ProcessManagerFixture, List_Scan)(benchmark::State& state) {
    const int kModels = static_cast<int>(state.range(0));

    // Pre-populate
    for (int i = 0; i < kModels; ++i) {
        mgr_->save(makeRecord(i));
    }

    for (auto _ : state) {
        auto records = mgr_->list();
        benchmark::DoNotOptimize(records.size());
    }

    state.SetItemsProcessed(state.iterations());
    state.SetLabel("list() over " + std::to_string(kModels) + " models");
}
BENCHMARK_REGISTER_F(ProcessManagerFixture, List_Scan)
    ->Arg(10)->Arg(100)->Arg(500)
    ->Unit(benchmark::kMillisecond);

// ─── 5. LlmProcessDescriptor: generate() prompt assembly ─────────────────────

static void BM_LlmDescriptor_Generate(benchmark::State& state) {
    const int kNodes = static_cast<int>(state.range(0));
    auto rec = makeRecord(0, kNodes);

    for (auto _ : state) {
        auto desc = LlmProcessDescriptor::generate(rec);
        benchmark::DoNotOptimize(desc.size());
    }

    state.SetItemsProcessed(state.iterations());
    state.SetLabel("LlmProcessDescriptor::generate() nodes=" + std::to_string(kNodes));
}
BENCHMARK(BM_LlmDescriptor_Generate)->Arg(5)->Arg(20)->Arg(50)->Arg(100);

// ─── 6. buildSystemPrompt() — size vs. latency ───────────────────────────────

static void BM_BuildSystemPrompt(benchmark::State& state) {
    const int kNodes = static_cast<int>(state.range(0));
    auto rec  = makeRecord(0, kNodes);
    auto desc = LlmProcessDescriptor::generate(rec);

    for (auto _ : state) {
        auto prompt = LlmProcessDescriptor::buildSystemPrompt(desc);
        benchmark::DoNotOptimize(prompt.size());
    }

    state.SetLabel("buildSystemPrompt() nodes=" + std::to_string(kNodes));
}
BENCHMARK(BM_BuildSystemPrompt)->Arg(5)->Arg(20)->Arg(100);

// ─── 7. buildConformancePrompt() ─────────────────────────────────────────────

static void BM_BuildConformancePrompt(benchmark::State& state) {
    auto rec  = makeRecord(0, 10);
    auto desc = LlmProcessDescriptor::generate(rec);

    nlohmann::json trace = nlohmann::json::array();
    const int kTraceLen = static_cast<int>(state.range(0));
    for (int i = 0; i < kTraceLen; ++i) {
        trace.push_back("task_0_" + std::to_string(i % 10));
    }

    for (auto _ : state) {
        auto prompt = LlmProcessDescriptor::buildConformancePrompt(desc, trace);
        benchmark::DoNotOptimize(prompt.size());
    }

    state.SetLabel("buildConformancePrompt() trace_len=" + std::to_string(kTraceLen));
}
BENCHMARK(BM_BuildConformancePrompt)->Arg(5)->Arg(20)->Arg(50);

// ─── 8. summarizeList() over N models ────────────────────────────────────────

static void BM_SummarizeList(benchmark::State& state) {
    const int kModels = static_cast<int>(state.range(0));
    std::vector<ProcessModelRecord> records;
    records.reserve(static_cast<size_t>(kModels));
    for (int i = 0; i < kModels; ++i) {
        records.push_back(makeRecord(i));
    }

    for (auto _ : state) {
        auto summary = LlmProcessDescriptor::summarizeList(records);
        benchmark::DoNotOptimize(summary.size());
    }

    state.SetItemsProcessed(state.iterations() * kModels);
    state.SetLabel("summarizeList() over " + std::to_string(kModels) + " models");
}
BENCHMARK(BM_SummarizeList)->Arg(1)->Arg(10)->Arg(50)->Arg(100)->Unit(benchmark::kMicrosecond);
