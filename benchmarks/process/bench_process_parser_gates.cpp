/*
 * ThemisDB | File: bench_process_parser_gates.cpp | Version: 1.0.0
 * Phase 5: Process Module Performance & Hardening
 *
 * Parser Performance Gates (PP):
 * | Gate ID | Metric                         | Target       |
 * |---------|--------------------------------|--------------|
 * | PP-01   | BPMN Parse (100 files)         | p99 ≤ 50ms   |
 * | PP-02   | BPMN Parse (1k files)          | p99 ≤ 100ms  |
 * | PP-03   | EPK Parse (100 files)          | p99 ≤ 75ms   |
 * | PP-04   | CMMN Parse (100 files)         | p99 ≤ 60ms   |
 * | PP-05   | DMN Parse (100 files)          | p99 ≤ 40ms   |
 * | PP-06   | OCEL Parse (100 logs)          | p99 ≤ 200ms  |
 * | PP-07   | VCC/VPB Parse (100 files)      | p99 ≤ 80ms   |
 * | PP-08   | FIM Parse (100 files)          | p99 ≤ 70ms   |
 */

#include <benchmark/benchmark.h>
#include <vector>
#include <chrono>
#include <string>
#include <memory>
#include <algorithm>
#include <random>
#include <sstream>

namespace themis::process::benchmark {

// ============================================================================
// Constants
// ============================================================================

constexpr uint64_t kCanonicalRngSeed = 42;
constexpr int kSmallDatasetSize = 100;
constexpr int kMediumDatasetSize = 1000;

// ============================================================================
// Format Parsers (Simulation)
// ============================================================================

/**
 * @brief BPMN XML Parser
 */
class BpmnParser {
public:
    struct BpmnModel {
        std::string id;
        std::string name;
        std::vector<std::string> tasks;
        std::vector<std::string> gateways;
        std::vector<std::pair<std::string, std::string>> flows;
    };

    static BpmnModel parse(const std::string& xml_content) {
        BpmnModel model;
        
        // Simulate XML parsing overhead
        size_t id_pos = xml_content.find("id=\"");
        if (id_pos != std::string::npos) {
            size_t end_pos = xml_content.find("\"", id_pos + 4);
            if (end_pos != std::string::npos) {
                model.id = xml_content.substr(id_pos + 4, end_pos - id_pos - 4);
            }
        }

        // Count elements (simulates DOM traversal)
        size_t task_count = 0;
        size_t pos = 0;
        while ((pos = xml_content.find("<bpmn:task", pos)) != std::string::npos) {
            task_count++;
            pos++;
        }
        
        for (size_t i = 0; i < task_count; ++i) {
            model.tasks.push_back("task_" + std::to_string(i));
        }

        // Count gateways
        size_t gateway_count = 0;
        pos = 0;
        while ((pos = xml_content.find("<bpmn:exclusiveGateway", pos)) != std::string::npos) {
            gateway_count++;
            pos++;
        }
        
        for (size_t i = 0; i < gateway_count; ++i) {
            model.gateways.push_back("gateway_" + std::to_string(i));
        }

        // Count flows
        size_t flow_count = 0;
        pos = 0;
        while ((pos = xml_content.find("<bpmn:sequenceFlow", pos)) != std::string::npos) {
            flow_count++;
            pos++;
        }
        
        for (size_t i = 0; i < flow_count; ++i) {
            model.flows.push_back({"task_" + std::to_string(i % task_count),
                                   "task_" + std::to_string((i + 1) % task_count)});
        }

        return model;
    }
};

/**
 * @brief EPK (Event-Driven Process Chain) Parser
 */
class EpkParser {
public:
    struct EpkModel {
        std::string id;
        std::vector<std::string> events;
        std::vector<std::string> functions;
        std::vector<std::string> connectors;
    };

    static EpkModel parse(const std::string& xml_content) {
        EpkModel model;
        
        // Similar parsing simulation
        size_t event_count = 0;
        size_t pos = 0;
        while ((pos = xml_content.find("<event", pos)) != std::string::npos) {
            event_count++;
            pos++;
        }
        
        for (size_t i = 0; i < event_count; ++i) {
            model.events.push_back("event_" + std::to_string(i));
        }

        size_t func_count = 0;
        pos = 0;
        while ((pos = xml_content.find("<function", pos)) != std::string::npos) {
            func_count++;
            pos++;
        }
        
        for (size_t i = 0; i < func_count; ++i) {
            model.functions.push_back("func_" + std::to_string(i));
        }

        return model;
    }
};

/**
 * @brief CMMN (Case Management Model and Notation) Parser
 */
class CmmnParser {
public:
    struct CmmnModel {
        std::string case_id;
        std::vector<std::string> tasks;
        std::vector<std::string> stages;
        std::vector<std::string> milestones;
    };

    static CmmnModel parse(const std::string& xml_content) {
        CmmnModel model;
        
        // Simulate CMMN parsing
        size_t task_count = 0;
        size_t pos = 0;
        while ((pos = xml_content.find("<cmmn:task", pos)) != std::string::npos) {
            task_count++;
            pos++;
        }
        
        for (size_t i = 0; i < task_count; ++i) {
            model.tasks.push_back("cmmn_task_" + std::to_string(i));
        }

        size_t stage_count = 0;
        pos = 0;
        while ((pos = xml_content.find("<cmmn:stage", pos)) != std::string::npos) {
            stage_count++;
            pos++;
        }
        
        for (size_t i = 0; i < stage_count; ++i) {
            model.stages.push_back("stage_" + std::to_string(i));
        }

        return model;
    }
};

/**
 * @brief DMN (Decision Model and Notation) Parser
 */
class DmnParser {
public:
    struct DmnModel {
        std::string id;
        std::vector<std::string> decisions;
        std::vector<std::string> inputs;
        std::vector<std::string> outputs;
    };

    static DmnModel parse(const std::string& xml_content) {
        DmnModel model;
        
        size_t decision_count = 0;
        size_t pos = 0;
        while ((pos = xml_content.find("<dmn:decision", pos)) != std::string::npos) {
            decision_count++;
            pos++;
        }
        
        for (size_t i = 0; i < decision_count; ++i) {
            model.decisions.push_back("decision_" + std::to_string(i));
        }

        return model;
    }
};

/**
 * @brief OCEL (Object-Centric Event Log) Parser
 */
class OcelParser {
public:
    struct OcelLog {
        std::string log_id;
        int event_count{0};
        int object_count{0};
        std::vector<std::string> event_types;
    };

    static OcelLog parse(const std::string& json_content) {
        OcelLog log;
        
        // Simulate JSON parsing
        size_t event_count = 0;
        size_t pos = 0;
        while ((pos = json_content.find("\"ocel:event\"", pos)) != std::string::npos) {
            event_count++;
            pos++;
        }
        log.event_count = static_cast<int>(event_count);

        size_t object_count = 0;
        pos = 0;
        while ((pos = json_content.find("\"ocel:object\"", pos)) != std::string::npos) {
            object_count++;
            pos++;
        }
        log.object_count = static_cast<int>(object_count);

        return log;
    }
};

/**
 * @brief VCC/VPB Parser
 */
class VccVpbParser {
public:
    struct VccVpbModel {
        std::string id;
        int elements{0};
    };

    static VccVpbModel parse(const std::string& content) {
        VccVpbModel model;
        
        // Simulate parsing
        size_t element_count = 0;
        size_t pos = 0;
        while ((pos = content.find("<element", pos)) != std::string::npos) {
            element_count++;
            pos++;
        }
        model.elements = static_cast<int>(element_count);

        return model;
    }
};

/**
 * @brief FIM (Flexible Intermediate Model) Parser
 */
class FimParser {
public:
    struct FimModel {
        std::string id;
        std::vector<std::string> nodes;
        std::vector<std::string> edges;
    };

    static FimModel parse(const std::string& content) {
        FimModel model;
        
        size_t node_count = 0;
        size_t pos = 0;
        while ((pos = content.find("<node", pos)) != std::string::npos) {
            node_count++;
            pos++;
        }
        
        for (size_t i = 0; i < node_count; ++i) {
            model.nodes.push_back("node_" + std::to_string(i));
        }

        return model;
    }
};

// ============================================================================
// Helper Functions
// ============================================================================

/**
 * @brief Generate realistic BPMN XML content
 */
static std::string generateBpmnXml(int model_idx) {
    std::ostringstream oss;
    oss << R"(<?xml version="1.0"?>)";
    oss << R"(<bpmn:definitions xmlns:bpmn="http://www.omg.org/spec/BPMN/20100524/MODEL" id="model_)" << model_idx << R"(">)";
    oss << R"(<bpmn:process id="proc_)" << model_idx << R"(" name="Process )" << model_idx << R"(">)";
    
    int num_tasks = 5 + (model_idx % 5);
    for (int i = 0; i < num_tasks; ++i) {
        oss << R"(<bpmn:task id="task_)" << i << R"(" name="Task )" << i << R"("/>)";
    }
    
    int num_gateways = 2 + (model_idx % 3);
    for (int i = 0; i < num_gateways; ++i) {
        oss << R"(<bpmn:exclusiveGateway id="gateway_)" << i << R"(" name="Gateway )" << i << R"("/>)";
    }
    
    for (int i = 0; i < num_tasks + num_gateways - 1; ++i) {
        oss << R"(<bpmn:sequenceFlow id="flow_)" << i << R"(" sourceRef="task_)" << i 
            << R"(" targetRef="task_)" << (i + 1) % (num_tasks + num_gateways) << R"("/>)";
    }
    
    oss << R"(</bpmn:process></bpmn:definitions>)";
    return oss.str();
}

/**
 * @brief Generate EPK XML content
 */
static std::string generateEpkXml(int model_idx) {
    std::ostringstream oss;
    oss << R"(<?xml version="1.0"?>)";
    oss << R"(<epk id="epk_)" << model_idx << R"(">)";
    
    int num_events = 4 + (model_idx % 4);
    for (int i = 0; i < num_events; ++i) {
        oss << R"(<event id="event_)" << i << R"(" name="Event )" << i << R"("/>)";
    }
    
    int num_functions = 3 + (model_idx % 3);
    for (int i = 0; i < num_functions; ++i) {
        oss << R"(<function id="func_)" << i << R"(" name="Function )" << i << R"("/>)";
    }
    
    oss << R"(</epk>)";
    return oss.str();
}

/**
 * @brief Generate CMMN XML content
 */
static std::string generateCmmnXml(int model_idx) {
    std::ostringstream oss;
    oss << R"(<?xml version="1.0"?>)";
    oss << R"(<cmmn:definitions id="cmmn_)" << model_idx << R"(">)";
    oss << R"(<cmmn:case id="case_)" << model_idx << R"(">)";
    
    int num_tasks = 3 + (model_idx % 3);
    for (int i = 0; i < num_tasks; ++i) {
        oss << R"(<cmmn:task id="cmmn_task_)" << i << R"(" name="Task )" << i << R"("/>)";
    }
    
    oss << R"(</cmmn:case></cmmn:definitions>)";
    return oss.str();
}

/**
 * @brief Generate DMN XML content
 */
static std::string generateDmnXml(int model_idx) {
    std::ostringstream oss;
    oss << R"(<?xml version="1.0"?>)";
    oss << R"(<dmn:definitions id="dmn_)" << model_idx << R"(">)";
    
    int num_decisions = 2 + (model_idx % 3);
    for (int i = 0; i < num_decisions; ++i) {
        oss << R"(<dmn:decision id="decision_)" << i << R"(" name="Decision )" << i << R"("/>)";
    }
    
    oss << R"(</dmn:definitions>)";
    return oss.str();
}

/**
 * @brief Generate OCEL JSON content
 */
static std::string generateOcelJson(int log_idx) {
    std::ostringstream oss;
    oss << R"({"ocel:global": {"ocel:version": "1.0"},)";
    oss << R"("ocel:log": [)";
    
    int num_events = 50 + (log_idx % 50);
    for (int i = 0; i < num_events; ++i) {
        oss << R"({"ocel:event": {"id": "event_)" << i << R"(", "type": "activity"}))";
        if (i < num_events - 1) oss << ",";
    }
    
    oss << R"(],)";
    oss << R"("ocel:objects": [)";
    
    int num_objects = 10 + (log_idx % 10);
    for (int i = 0; i < num_objects; ++i) {
        oss << R"({"ocel:object": {"id": "obj_)" << i << R"(", "type": "order"}))";
        if (i < num_objects - 1) oss << ",";
    }
    
    oss << R"(]})";
    return oss.str();
}

/**
 * @brief Generate VCC/VPB content
 */
static std::string generateVccVpbXml(int model_idx) {
    std::ostringstream oss;
    oss << R"(<?xml version="1.0"?>)";
    oss << R"(<vccvpb id="model_)" << model_idx << R"(">)";
    
    int num_elements = 10 + (model_idx % 10);
    for (int i = 0; i < num_elements; ++i) {
        oss << R"(<element id="elem_)" << i << R"("/>)";
    }
    
    oss << R"(</vccvpb>)";
    return oss.str();
}

/**
 * @brief Generate FIM content
 */
static std::string generateFimXml(int model_idx) {
    std::ostringstream oss;
    oss << R"(<?xml version="1.0"?>)";
    oss << R"(<fim id="fim_)" << model_idx << R"(">)";
    
    int num_nodes = 8 + (model_idx % 8);
    for (int i = 0; i < num_nodes; ++i) {
        oss << R"(<node id="node_)" << i << R"(" name="Node )" << i << R"("/>)";
    }
    
    oss << R"(</fim>)";
    return oss.str();
}

// ============================================================================
// Parser Performance Benchmarks
// ============================================================================

/**
 * PP-01: BPMN Parse (100 files)
 */
static void BM_PP01_BpmnParse_Small(benchmark::State& state) {
    const int num_files = kSmallDatasetSize;
    std::vector<std::string> bpmn_files;
    
    for (int i = 0; i < num_files; ++i) {
        bpmn_files.push_back(generateBpmnXml(i));
    }

    std::vector<double> latencies;
    latencies.reserve(num_files);

    for (auto _ : state) {
        state.PauseTiming();
        latencies.clear();
        state.ResumeTiming();

        for (const auto& bpmn_xml : bpmn_files) {
            auto start = std::chrono::high_resolution_clock::now();
            auto model = BpmnParser::parse(bpmn_xml);
            auto end = std::chrono::high_resolution_clock::now();
            
            auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
            latencies.push_back(static_cast<double>(duration_ms));
            
            benchmark::DoNotOptimize(model);
        }
    }

    if (!latencies.empty()) {
        std::sort(latencies.begin(), latencies.end());
        double p99 = latencies[std::min(size_t(99), latencies.size() - 1)];
        state.counters["p99_ms"] = benchmark::Counter(p99, benchmark::Counter::kAvgIterations);
    }

    state.SetItemsProcessed(num_files * static_cast<int64_t>(state.iterations()));
}

BENCHMARK(BM_PP01_BpmnParse_Small)->Iterations(10)->ReportAggregatesOnly(true)->UseRealTime();

/**
 * PP-02: BPMN Parse (1k files)
 */
static void BM_PP02_BpmnParse_Medium(benchmark::State& state) {
    const int num_files = kMediumDatasetSize;
    std::vector<std::string> bpmn_files;
    
    for (int i = 0; i < num_files; ++i) {
        bpmn_files.push_back(generateBpmnXml(i));
    }

    int64_t parsed_count = 0;

    for (auto _ : state) {
        for (const auto& bpmn_xml : bpmn_files) {
            auto model = BpmnParser::parse(bpmn_xml);
            benchmark::DoNotOptimize(model);
            parsed_count++;
        }
    }

    state.SetItemsProcessed(parsed_count);
    state.counters["parse_rate"] = benchmark::Counter(parsed_count, benchmark::Counter::kIsRate);
}

BENCHMARK(BM_PP02_BpmnParse_Medium)->Iterations(5)->ReportAggregatesOnly(true)->UseRealTime();

/**
 * PP-03: EPK Parse (100 files)
 */
static void BM_PP03_EpkParse(benchmark::State& state) {
    const int num_files = kSmallDatasetSize;
    std::vector<std::string> epk_files;
    
    for (int i = 0; i < num_files; ++i) {
        epk_files.push_back(generateEpkXml(i));
    }

    int64_t parsed_count = 0;

    for (auto _ : state) {
        for (const auto& epk_xml : epk_files) {
            auto model = EpkParser::parse(epk_xml);
            benchmark::DoNotOptimize(model);
            parsed_count++;
        }
    }

    state.SetItemsProcessed(parsed_count);
}

BENCHMARK(BM_PP03_EpkParse)->Iterations(10)->ReportAggregatesOnly(true)->UseRealTime();

/**
 * PP-04: CMMN Parse (100 files)
 */
static void BM_PP04_CmmnParse(benchmark::State& state) {
    const int num_files = kSmallDatasetSize;
    std::vector<std::string> cmmn_files;
    
    for (int i = 0; i < num_files; ++i) {
        cmmn_files.push_back(generateCmmnXml(i));
    }

    int64_t parsed_count = 0;

    for (auto _ : state) {
        for (const auto& cmmn_xml : cmmn_files) {
            auto model = CmmnParser::parse(cmmn_xml);
            benchmark::DoNotOptimize(model);
            parsed_count++;
        }
    }

    state.SetItemsProcessed(parsed_count);
}

BENCHMARK(BM_PP04_CmmnParse)->Iterations(10)->ReportAggregatesOnly(true)->UseRealTime();

/**
 * PP-05: DMN Parse (100 files)
 */
static void BM_PP05_DmnParse(benchmark::State& state) {
    const int num_files = kSmallDatasetSize;
    std::vector<std::string> dmn_files;
    
    for (int i = 0; i < num_files; ++i) {
        dmn_files.push_back(generateDmnXml(i));
    }

    int64_t parsed_count = 0;

    for (auto _ : state) {
        for (const auto& dmn_xml : dmn_files) {
            auto model = DmnParser::parse(dmn_xml);
            benchmark::DoNotOptimize(model);
            parsed_count++;
        }
    }

    state.SetItemsProcessed(parsed_count);
}

BENCHMARK(BM_PP05_DmnParse)->Iterations(10)->ReportAggregatesOnly(true)->UseRealTime();

/**
 * PP-06: OCEL Parse (100 logs)
 */
static void BM_PP06_OcelParse(benchmark::State& state) {
    const int num_logs = kSmallDatasetSize;
    std::vector<std::string> ocel_logs;
    
    for (int i = 0; i < num_logs; ++i) {
        ocel_logs.push_back(generateOcelJson(i));
    }

    int64_t parsed_count = 0;

    for (auto _ : state) {
        for (const auto& ocel_json : ocel_logs) {
            auto log = OcelParser::parse(ocel_json);
            benchmark::DoNotOptimize(log);
            parsed_count++;
        }
    }

    state.SetItemsProcessed(parsed_count);
}

BENCHMARK(BM_PP06_OcelParse)->Iterations(10)->ReportAggregatesOnly(true)->UseRealTime();

/**
 * PP-07: VCC/VPB Parse (100 files)
 */
static void BM_PP07_VccVpbParse(benchmark::State& state) {
    const int num_files = kSmallDatasetSize;
    std::vector<std::string> vcc_vpb_files;
    
    for (int i = 0; i < num_files; ++i) {
        vcc_vpb_files.push_back(generateVccVpbXml(i));
    }

    int64_t parsed_count = 0;

    for (auto _ : state) {
        for (const auto& vcc_vpb_xml : vcc_vpb_files) {
            auto model = VccVpbParser::parse(vcc_vpb_xml);
            benchmark::DoNotOptimize(model);
            parsed_count++;
        }
    }

    state.SetItemsProcessed(parsed_count);
}

BENCHMARK(BM_PP07_VccVpbParse)->Iterations(10)->ReportAggregatesOnly(true)->UseRealTime();

/**
 * PP-08: FIM Parse (100 files)
 */
static void BM_PP08_FimParse(benchmark::State& state) {
    const int num_files = kSmallDatasetSize;
    std::vector<std::string> fim_files;
    
    for (int i = 0; i < num_files; ++i) {
        fim_files.push_back(generateFimXml(i));
    }

    int64_t parsed_count = 0;

    for (auto _ : state) {
        for (const auto& fim_xml : fim_files) {
            auto model = FimParser::parse(fim_xml);
            benchmark::DoNotOptimize(model);
            parsed_count++;
        }
    }

    state.SetItemsProcessed(parsed_count);
}

BENCHMARK(BM_PP08_FimParse)->Iterations(10)->ReportAggregatesOnly(true)->UseRealTime();

}  // namespace themis::process::benchmark

BENCHMARK_MAIN();
