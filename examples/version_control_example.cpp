/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            version_control_example.cpp                        ║
  Version:         0.0.40                                             ║
  Last Modified:   2026-04-14 06:49:30                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     372                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file version_control_example.cpp
 * @brief Example demonstrating prompt version control system
 * 
 * Shows how to:
 * - Commit versions with messages
 * - Create and manage branches
 * - Rollback to previous versions
 * - Generate diffs
 * - Merge branches
 * - Tag important versions
 * - Track performance scores
 */

#include <iostream>
#include <memory>
#include "prompt_engineering/prompt_version_control.h"

using namespace themis::prompt_engineering;

int main() {
    std::cout << "=== Prompt Version Control System Example ===\n\n";
    
    // ========================================================================
    // Step 1: Initialize Version Control
    // ========================================================================
    std::cout << "Step 1: Initializing version control system...\n";
    
    auto vc = std::make_shared<PromptVersionControl>();
    
    std::cout << "✓ Version control initialized\n\n";
    
    // ========================================================================
    // Step 2: Commit Initial Version
    // ========================================================================
    std::cout << "Step 2: Committing initial version...\n";
    
    std::string prompt_id = "query_enhancement_v1";
    std::string initial_content = R"(Task: Enhance the user's query for better search results.

Input: {query}

Instructions:
- Identify key concepts
- Add relevant context
- Improve specificity

Output: Enhanced query)";
    
    std::string v1 = vc->commit(
        prompt_id,
        initial_content,
        "Initial version of query enhancement prompt",
        "developer"
    );
    
    std::cout << "  Version ID: " << v1.substr(0, 8) << "...\n";
    std::cout << "  Branch: main\n";
    std::cout << "✓ Initial version committed\n\n";
    
    // ========================================================================
    // Step 3: Make Improvements on Main Branch
    // ========================================================================
    std::cout << "Step 3: Improving prompt on main branch...\n";
    
    std::string improved_content = R"(Task: Enhance the user's query for better search results.

Input: {query}

Instructions:
- Identify key concepts and entities
- Add relevant context and synonyms
- Improve specificity and clarity
- Consider user intent

Output: Enhanced query with explanation)";
    
    std::string v2 = vc->commit(
        prompt_id,
        improved_content,
        "Added synonym expansion and intent analysis",
        "developer"
    );
    
    std::cout << "  Version ID: " << v2.substr(0, 8) << "...\n";
    std::cout << "  Changes: Added 2 new instructions\n";
    std::cout << "✓ Improvement committed\n\n";
    
    // ========================================================================
    // Step 4: Create Experimental Branch
    // ========================================================================
    std::cout << "Step 4: Creating experimental branch...\n";
    
    bool branch_created = vc->createBranch(prompt_id, "experimental", v2);
    
    if (branch_created) {
        std::cout << "  Branch 'experimental' created from version " << v2.substr(0, 8) << "\n";
        std::cout << "✓ Branch created successfully\n\n";
    }
    
    // ========================================================================
    // Step 5: Experiment on New Branch
    // ========================================================================
    std::cout << "Step 5: Experimenting with new approach...\n";
    
    std::string experimental_content = R"(Task: Enhance query using advanced NLP techniques.

Input: {query}

Advanced Instructions:
- Extract named entities
- Identify semantic relationships
- Add domain-specific terminology
- Consider user intent and context
- Generate multiple query variations

Output: Top 3 enhanced query variations with confidence scores)";
    
    std::string exp_v1 = vc->commit(
        prompt_id,
        experimental_content,
        "Experimental: Multiple query variations with NLP",
        "researcher",
        "experimental"
    );
    
    std::cout << "  Version ID: " << exp_v1.substr(0, 8) << "...\n";
    std::cout << "  Branch: experimental\n";
    std::cout << "✓ Experimental version committed\n\n";
    
    // ========================================================================
    // Step 6: View History
    // ========================================================================
    std::cout << "Step 6: Viewing version history...\n";
    
    auto main_history = vc->getHistory(prompt_id, "main");
    std::cout << "  Main branch history (" << main_history.size() << " versions):\n";
    for (const auto& version : main_history) {
        std::cout << "    - " << version.version_id.substr(0, 8) << ": " 
                  << version.commit_message << "\n";
        std::cout << "      Author: " << version.author << "\n";
    }
    
    std::cout << "\n";
    
    auto exp_history = vc->getHistory(prompt_id, "experimental");
    std::cout << "  Experimental branch history (" << exp_history.size() << " versions):\n";
    for (const auto& version : exp_history) {
        std::cout << "    - " << version.version_id.substr(0, 8) << ": " 
                  << version.commit_message << "\n";
    }
    
    std::cout << "\n✓ History retrieved\n\n";
    
    // ========================================================================
    // Step 7: Generate Diff
    // ========================================================================
    std::cout << "Step 7: Generating diff between versions...\n";
    
    auto diff = vc->diff(v2, exp_v1);
    
    std::cout << "  Diff: " << v2.substr(0, 8) << " -> " << exp_v1.substr(0, 8) << "\n";
    std::cout << "  Additions: " << diff.additions << " lines\n";
    std::cout << "  Deletions: " << diff.deletions << " lines\n";
    std::cout << "\n  Added lines:\n";
    for (size_t i = 0; i < std::min(size_t(3), diff.added_lines.size()); ++i) {
        std::cout << "    + " << diff.added_lines[i] << "\n";
    }
    
    std::cout << "\n✓ Diff generated\n\n";
    
    // ========================================================================
    // Step 8: Update Performance Scores
    // ========================================================================
    std::cout << "Step 8: Recording performance scores...\n";
    
    // Simulate performance testing
    vc->updatePerformanceScore(v2, 0.82);      // Main branch: 82%
    vc->updatePerformanceScore(exp_v1, 0.91);  // Experimental: 91%
    
    std::cout << "  Main (v2): 82% success rate\n";
    std::cout << "  Experimental: 91% success rate\n";
    std::cout << "✓ Performance scores updated\n\n";
    
    // ========================================================================
    // Step 9: Merge Successful Experiment
    // ========================================================================
    std::cout << "Step 9: Merging experimental branch into main...\n";
    
    // Since experimental performed better, merge it
    auto merge_result = vc->merge(
        prompt_id,
        "experimental",
        "main",
        "theirs",  // Use experimental content
        "Merge experimental branch - 91% success rate"
    );
    
    if (merge_result.success) {
        std::cout << "  Merge successful!\n";
        std::cout << "  New version: " << merge_result.merged_version_id.substr(0, 8) << "\n";
        std::cout << "  Strategy: " << merge_result.strategy_used << "\n";
        std::cout << "✓ Branches merged\n\n";
    } else {
        std::cout << "  Merge conflicts detected:\n";
        for (const auto& conflict : merge_result.conflicts) {
            std::cout << "    - " << conflict << "\n";
        }
        std::cout << "✗ Manual resolution required\n\n";
    }
    
    // ========================================================================
    // Step 10: Tag Production Version
    // ========================================================================
    std::cout << "Step 10: Tagging production version...\n";
    
    auto latest = vc->getLatest(prompt_id, "main");
    if (latest.has_value()) {
        vc->tag(latest->version_id, "production");
        vc->tag(latest->version_id, "v2.0");
        
        std::cout << "  Tagged version " << latest->version_id.substr(0, 8) << " as:\n";
        std::cout << "    - production\n";
        std::cout << "    - v2.0\n";
        std::cout << "✓ Tags created\n\n";
    }
    
    // ========================================================================
    // Step 11: Simulate Regression and Rollback
    // ========================================================================
    std::cout << "Step 11: Simulating regression and rollback...\n";
    
    // Commit a bad version
    std::string bad_content = "Broken prompt";
    std::string v_bad = vc->commit(
        prompt_id,
        bad_content,
        "Attempted optimization (broken)",
        "system"
    );
    
    std::cout << "  Created bad version: " << v_bad.substr(0, 8) << "\n";
    
    // Detect regression and rollback
    std::cout << "  Regression detected! Rolling back...\n";
    
    std::string rollback_id = vc->rollbackN(prompt_id, 1);  // Rollback 1 version
    
    std::cout << "  Rollback complete: " << rollback_id.substr(0, 8) << "\n";
    
    auto current = vc->getLatest(prompt_id);
    std::cout << "  Current content restored\n";
    std::cout << "✓ Rollback successful\n\n";
    
    // ========================================================================
    // Step 12: List All Branches
    // ========================================================================
    std::cout << "Step 12: Listing all branches...\n";
    
    auto branches = vc->listBranches(prompt_id);
    
    std::cout << "  Branches (" << branches.size() << "):\n";
    for (const auto& branch : branches) {
        std::cout << "    - " << branch.name << "\n";
        std::cout << "      HEAD: " << branch.head_version.substr(0, 8) << "\n";
        std::cout << "      Commits: " << branch.commit_count << "\n";
    }
    
    std::cout << "\n✓ Branches listed\n\n";
    
    // ========================================================================
    // Step 13: View Version Genealogy
    // ========================================================================
    std::cout << "Step 13: Viewing version genealogy...\n";
    
    auto genealogy = vc->getGenealogy(prompt_id);
    
    std::cout << "  Version relationships:\n";
    for (const auto& [child, parent] : genealogy) {
        if (parent.empty()) {
            std::cout << "    - " << child.substr(0, 8) << " (root)\n";
        } else {
            std::cout << "    - " << child.substr(0, 8) << " <- " << parent.substr(0, 8) << "\n";
        }
    }
    
    std::cout << "\n✓ Genealogy retrieved\n\n";
    
    // ========================================================================
    // Step 14: Retrieve Tagged Versions
    // ========================================================================
    std::cout << "Step 14: Retrieving tagged versions...\n";
    
    auto prod_version = vc->getByTag(prompt_id, "production");
    if (prod_version.has_value()) {
        std::cout << "  Production version:\n";
        std::cout << "    Version: " << prod_version->version_id.substr(0, 8) << "\n";
        std::cout << "    Message: " << prod_version->commit_message << "\n";
        std::cout << "    Performance: " << (prod_version->performance_score * 100) << "%\n";
    }
    
    auto tags = vc->listTags(prompt_id);
    std::cout << "\n  All tags (" << tags.size() << "):\n";
    for (const auto& [tag, version] : tags) {
        std::cout << "    - " << tag << " -> " << version.substr(0, 8) << "\n";
    }
    
    std::cout << "\n✓ Tags retrieved\n\n";
    
    // ========================================================================
    // Step 15: Get Statistics
    // ========================================================================
    std::cout << "Step 15: Getting version control statistics...\n";
    
    auto stats = vc->getStats(prompt_id);
    
    std::cout << "  Statistics:\n";
    std::cout << "    Total versions: " << stats["total_versions"] << "\n";
    std::cout << "    Branches: " << stats["branch_count"] << "\n";
    std::cout << "    Tags: " << stats["tag_count"] << "\n";
    
    std::cout << "\n✓ Statistics generated\n\n";
    
    // ========================================================================
    // Complete!
    // ========================================================================
    std::cout << "=== Version Control Example Complete! ===\n\n";
    
    std::cout << "Key Capabilities Demonstrated:\n";
    std::cout << "  ✓ Committing versions with messages\n";
    std::cout << "  ✓ Creating and managing branches\n";
    std::cout << "  ✓ Viewing history and genealogy\n";
    std::cout << "  ✓ Generating diffs between versions\n";
    std::cout << "  ✓ Tracking performance scores\n";
    std::cout << "  ✓ Merging branches\n";
    std::cout << "  ✓ Tagging important versions\n";
    std::cout << "  ✓ Rolling back on regression\n";
    std::cout << "  ✓ Retrieving statistics\n\n";
    
    std::cout << "Production Workflow:\n";
    std::cout << "  1. Develop on feature branches\n";
    std::cout << "  2. Test performance of each version\n";
    std::cout << "  3. Merge successful experiments\n";
    std::cout << "  4. Tag stable versions for production\n";
    std::cout << "  5. Rollback automatically on regression\n";
    std::cout << "  6. Maintain complete version history\n\n";
    
    return 0;
}
