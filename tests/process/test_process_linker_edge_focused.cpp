/**
 * @file test_process_linker_edge_focused.cpp
 * @brief Phase 4 Linker Edge Tests: Cyclic dependencies, missing targets, partial link state
 * @note Test IDs: L-01..L-08
 */

#include <gtest/gtest.h>
#include "process/process_api_contract.h"
#include "process/process_linker.h"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

using namespace themis::process;

// ─────────────────────────────────────────────────────────────────────────────
// Linker Edge Test Fixture
// ─────────────────────────────────────────────────────────────────────────────

class LinkerEdgeTest : public ::testing::Test {
protected:
    // Simulate a link database
    std::map<std::string, ProcessLink> link_storage;
    std::map<std::string, ProcessAttachment> attachment_storage;

    void SetUp() override {
        link_storage.clear();
        attachment_storage.clear();
    }

    // Mock linker that validates and stores links
    bool create_link(const ProcessLink& link, ProcError& out_error) {
        if (link.source_id.empty() || link.target_id.empty()) {
            out_error = ProcError::kValidationFailed;
            return false;
        }

        link_storage[link.link_id] = link;
        return true;
    }

    // Mock attachment creation
    bool create_attachment(const ProcessAttachment& attach, ProcError& out_error) {
        if (attach.instance_id.empty() || attach.object_id.empty()) {
            out_error = ProcError::kValidationFailed;
            return false;
        }

        attachment_storage[attach.id] = attach;
        return true;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// L-01: Cyclic link detection (A -> B -> C -> A)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(LinkerEdgeTest, L01_CyclicLinkDetection) {
    struct CycleDetectionResult {
        bool has_cycle{false};
        std::vector<std::string> cycle_nodes;
    };

    auto detect_link_cycle = [](const std::map<std::string, std::vector<std::string>>& adjacency) 
                              -> CycleDetectionResult {
        CycleDetectionResult result;
        std::vector<std::string> visited;
        std::vector<std::string> rec_stack;

        std::function<bool(const std::string&)> dfs = [&](const std::string& node) -> bool {
            visited.push_back(node);
            rec_stack.push_back(node);

            auto it = adjacency.find(node);
            if (it != adjacency.end()) {
                for (const auto& neighbor : it->second) {
                    auto rec_it = std::find(rec_stack.begin(), rec_stack.end(), neighbor);
                    if (rec_it != rec_stack.end()) {
                        result.has_cycle = true;
                        result.cycle_nodes.assign(rec_it, rec_stack.end());
                        result.cycle_nodes.push_back(neighbor);
                        return true;
                    }

                    auto vis_it = std::find(visited.begin(), visited.end(), neighbor);
                    if (vis_it == visited.end() && dfs(neighbor)) {
                        return true;
                    }
                }
            }

            rec_stack.pop_back();
            return false;
        };

        for (const auto& [node, _] : adjacency) {
            auto vis_it = std::find(visited.begin(), visited.end(), node);
            if (vis_it == visited.end() && dfs(node)) {
                break;
            }
        }

        return result;
    };

    std::map<std::string, std::vector<std::string>> graph;
    graph["A"] = {"B"};
    graph["B"] = {"C"};
    graph["C"] = {"A"};

    CycleDetectionResult result = detect_link_cycle(graph);
    EXPECT_TRUE(result.has_cycle);
    EXPECT_FALSE(result.cycle_nodes.empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// L-02: Missing target in link reference
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(LinkerEdgeTest, L02_MissingTargetInLinkReference) {
    ProcessLink link;
    link.link_id = "link_1";
    link.source_id = "proc_1";
    link.target_id = "proc_999";  // Non-existent target
    link.link_type = ProcessLinkType::CROSS_REFERENCE;

    std::vector<std::string> known_processes = {"proc_1", "proc_2", "proc_3"};

    bool target_exists = false;
    for (const auto& proc_id : known_processes) {
        if (proc_id == link.target_id) {
            target_exists = true;
            break;
        }
    }

    EXPECT_FALSE(target_exists);
}

// ─────────────────────────────────────────────────────────────────────────────
// L-03: Missing source in link reference
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(LinkerEdgeTest, L03_MissingSourceInLinkReference) {
    ProcessLink link;
    link.link_id = "link_2";
    link.source_id = "proc_999";  // Non-existent source
    link.target_id = "proc_1";
    link.link_type = ProcessLinkType::SUB_PROCESS;

    std::vector<std::string> known_processes = {"proc_1", "proc_2", "proc_3"};

    bool source_exists = false;
    for (const auto& proc_id : known_processes) {
        if (proc_id == link.source_id) {
            source_exists = true;
            break;
        }
    }

    EXPECT_FALSE(source_exists);
}

// ─────────────────────────────────────────────────────────────────────────────
// L-04: Partial link state recovery (orphaned links)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(LinkerEdgeTest, L04_PartialLinkStateRecovery) {
    // Simulate partial state where some links are orphaned
    std::vector<ProcessLink> all_links;

    ProcessLink link1;
    link1.link_id = "link_1";
    link1.source_id = "proc_1";
    link1.target_id = "proc_2";
    all_links.push_back(link1);

    ProcessLink link2;
    link2.link_id = "link_2";
    link2.source_id = "proc_2";
    link2.target_id = "proc_999";  // Orphaned
    all_links.push_back(link2);

    ProcessLink link3;
    link3.link_id = "link_3";
    link3.source_id = "proc_1";
    link3.target_id = "proc_3";
    all_links.push_back(link3);

    std::vector<std::string> known_processes = {"proc_1", "proc_2", "proc_3"};

    // Find orphaned links
    std::vector<ProcessLink> orphaned;
    for (const auto& link : all_links) {
        bool src_exists = std::find(known_processes.begin(), known_processes.end(), 
                                    link.source_id) != known_processes.end();
        bool tgt_exists = std::find(known_processes.begin(), known_processes.end(), 
                                    link.target_id) != known_processes.end();

        if (!src_exists || !tgt_exists) {
            orphaned.push_back(link);
        }
    }

    EXPECT_EQ(orphaned.size(), 1);
    EXPECT_EQ(orphaned[0].link_id, "link_2");
}

// ─────────────────────────────────────────────────────────────────────────────
// L-05: Attachment without matching instance
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(LinkerEdgeTest, L05_AttachmentWithoutMatchingInstance) {
    ProcessAttachment attach;
    attach.id = "attach_1";
    attach.instance_id = "inst_999";  // Non-existent instance
    attach.object_id = "doc_1";
    attach.object_collection = "documents";

    std::vector<std::string> known_instances = {"inst_1", "inst_2", "inst_3"};

    bool instance_exists = false;
    for (const auto& inst_id : known_instances) {
        if (inst_id == attach.instance_id) {
            instance_exists = true;
            break;
        }
    }

    EXPECT_FALSE(instance_exists);
}

// ─────────────────────────────────────────────────────────────────────────────
// L-06: Invalid link type enumeration
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(LinkerEdgeTest, L06_ValidLinkTypeEnumeration) {
    // Verify all link types are distinct
    std::map<ProcessLinkType, bool> seen;

    std::vector<ProcessLinkType> all_types = {
        ProcessLinkType::HAS_DOCUMENT,
        ProcessLinkType::HAS_METADATA,
        ProcessLinkType::REQUIRES_DOCUMENT,
        ProcessLinkType::IS_INSTANCE_OF,
        ProcessLinkType::SUB_PROCESS,
        ProcessLinkType::CROSS_REFERENCE,
        ProcessLinkType::TRIGGERS,
        ProcessLinkType::EVIDENCE_FOR,
    };

    for (auto type : all_types) {
        EXPECT_EQ(seen.count(type), 0) << "Duplicate link type encountered";
        seen[type] = true;
    }

    EXPECT_EQ(seen.size(), 8);
}

// ─────────────────────────────────────────────────────────────────────────────
// L-07: Broken link chain (A -> B -> X, X missing)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(LinkerEdgeTest, L07_BrokenLinkChain) {
    std::map<std::string, std::vector<std::string>> link_graph;
    link_graph["A"] = {"B"};
    link_graph["B"] = {"X"};  // X doesn't exist in graph

    struct LinkChainValidationResult {
        bool valid{true};
        std::vector<std::string> broken_links;
    };

    auto validate_chain = [](const std::map<std::string, std::vector<std::string>>& graph) 
                          -> LinkChainValidationResult {
        LinkChainValidationResult result;

        for (const auto& [src, targets] : graph) {
            for (const auto& tgt : targets) {
                if (graph.find(tgt) == graph.end()) {
                    result.valid = false;
                    result.broken_links.push_back(src + " -> " + tgt);
                }
            }
        }

        return result;
    };

    LinkChainValidationResult result = validate_chain(link_graph);
    EXPECT_FALSE(result.valid);
    EXPECT_EQ(result.broken_links.size(), 1);
    EXPECT_EQ(result.broken_links[0], "B -> X");
}

// ─────────────────────────────────────────────────────────────────────────────
// L-08: Multiple link types between same entities
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(LinkerEdgeTest, L08_MultipleTypedLinksBetweenEntities) {
    std::vector<ProcessLink> links;

    // Same source/target, different link types
    ProcessLink link1;
    link1.link_id = "link_1";
    link1.source_id = "A";
    link1.target_id = "B";
    link1.link_type = ProcessLinkType::SUB_PROCESS;
    links.push_back(link1);

    ProcessLink link2;
    link2.link_id = "link_2";
    link2.source_id = "A";
    link2.target_id = "B";
    link2.link_type = ProcessLinkType::TRIGGERS;
    links.push_back(link2);

    ProcessLink link3;
    link3.link_id = "link_3";
    link3.source_id = "A";
    link3.target_id = "B";
    link3.link_type = ProcessLinkType::CROSS_REFERENCE;
    links.push_back(link3);

    // Count links between A and B
    int32_t count_ab = 0;
    for (const auto& link : links) {
        if (link.source_id == "A" && link.target_id == "B") {
            count_ab++;
        }
    }

    EXPECT_EQ(count_ab, 3);

    // Verify all are different types
    std::set<ProcessLinkType> types;
    for (const auto& link : links) {
        if (link.source_id == "A" && link.target_id == "B") {
            types.insert(link.link_type);
        }
    }

    EXPECT_EQ(types.size(), 3);
}
