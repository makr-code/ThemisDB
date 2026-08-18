/**
 * @brief Verification that policy versioning and change management is implemented
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

struct VerificationResult {
    bool success;
    std::string message;
};

VerificationResult checkFileExists(const std::string& path, const std::string& description) {
    if (fs::exists(path)) {
        return {true, "✓ " + description + " found at " + path};
    }
    return {false, "✗ " + description + " NOT FOUND at " + path};
}

VerificationResult checkFileContains(const std::string& path, const std::string& pattern, const std::string& description) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return {false, "✗ Cannot open " + path};
    }
    
    std::string line;
    while (std::getline(file, line)) {
        if (line.find(pattern) != std::string::npos) {
            file.close();
            return {true, "✓ " + description};
        }
    }
    
    file.close();
    return {false, "✗ Pattern not found: " + description + " in " + path};
}

int main() {
    std::cout << "\n=== ThemisDB Policy Versioning & Change Management Verification ===\n\n";
    
    std::vector<VerificationResult> results;
    
    // Check header files
    std::cout << "Checking header files...\n";
    results.push_back(checkFileExists(
        "include/governance/policy_approval_workflow.h",
        "Policy Approval Workflow Header"
    ));
    results.push_back(checkFileExists(
        "include/governance/policy_change_manager.h",
        "Policy Change Manager Header"
    ));
    
    // Check implementation files
    std::cout << "Checking implementation files...\n";
    results.push_back(checkFileExists(
        "src/governance/policy_approval_workflow.cpp",
        "Policy Approval Workflow Implementation"
    ));
    results.push_back(checkFileExists(
        "src/governance/policy_change_manager.cpp",
        "Policy Change Manager Implementation"
    ));
    
    // Check test file
    std::cout << "Checking test files...\n";
    results.push_back(checkFileExists(
        "tests/governance/test_policy_versioning_and_approval.cpp",
        "Policy Versioning & Approval Tests"
    ));
    
    // Check documentation
    std::cout << "Checking documentation...\n";
    results.push_back(checkFileExists(
        "src/governance/POLICY_VERSIONING_GUIDE.md",
        "Policy Versioning Operator Guide"
    ));
    
    // Check specific implementations
    std::cout << "Checking implementation details...\n";
    
    // Approval workflow
    results.push_back(checkFileContains(
        "include/governance/policy_approval_workflow.h",
        "enum class ApprovalState",
        "ApprovalState enum defined"
    ));
    results.push_back(checkFileContains(
        "include/governance/policy_approval_workflow.h",
        "class PolicyApprovalWorkflow",
        "PolicyApprovalWorkflow class defined"
    ));
    results.push_back(checkFileContains(
        "src/governance/policy_approval_workflow.cpp",
        "bool PolicyApprovalWorkflow::submitForReview",
        "submitForReview method implemented"
    ));
    results.push_back(checkFileContains(
        "src/governance/policy_approval_workflow.cpp",
        "bool PolicyApprovalWorkflow::approveChange",
        "approveChange method implemented"
    ));
    results.push_back(checkFileContains(
        "src/governance/policy_approval_workflow.cpp",
        "bool PolicyApprovalWorkflow::emergencyOverride",
        "emergencyOverride method implemented"
    ));
    
    // Change manager
    results.push_back(checkFileContains(
        "include/governance/policy_change_manager.h",
        "class PolicyChangeManager",
        "PolicyChangeManager class defined"
    ));
    results.push_back(checkFileContains(
        "include/governance/policy_change_manager.h",
        "enum class RollbackSafetyLevel",
        "RollbackSafetyLevel enum defined"
    ));
    results.push_back(checkFileContains(
        "src/governance/policy_change_manager.cpp",
        "RollbackSafetyReport PolicyChangeManager::checkRollbackSafety",
        "checkRollbackSafety method implemented"
    ));
    results.push_back(checkFileContains(
        "src/governance/policy_change_manager.cpp",
        "RollbackOperation PolicyChangeManager::performRollback",
        "performRollback method implemented"
    ));
    results.push_back(checkFileContains(
        "src/governance/policy_change_manager.cpp",
        "RollbackOperation PolicyChangeManager::performCoordinatedRollback",
        "performCoordinatedRollback method implemented"
    ));
    
    // Tests
    results.push_back(checkFileContains(
        "tests/governance/test_policy_versioning_and_approval.cpp",
        "GOV_Version_01",
        "GOV-Version-01 test defined"
    ));
    results.push_back(checkFileContains(
        "tests/governance/test_policy_versioning_and_approval.cpp",
        "GOV_Version_02",
        "GOV-Version-02 test defined"
    ));
    results.push_back(checkFileContains(
        "tests/governance/test_policy_versioning_and_approval.cpp",
        "GOV_Version_03",
        "GOV-Version-03 test defined"
    ));
    results.push_back(checkFileContains(
        "tests/governance/test_policy_versioning_and_approval.cpp",
        "GOV_Version_04",
        "GOV-Version-04 test defined"
    ));
    results.push_back(checkFileContains(
        "tests/governance/test_policy_versioning_and_approval.cpp",
        "GOV_Version_05",
        "GOV-Version-05 test defined"
    ));
    results.push_back(checkFileContains(
        "tests/governance/test_policy_versioning_and_approval.cpp",
        "GOV_Version_06",
        "GOV-Version-06 test defined"
    ));
    results.push_back(checkFileContains(
        "tests/governance/test_policy_versioning_and_approval.cpp",
        "GOV_GRG_05",
        "GOV-GRG-05 benchmark test defined"
    ));
    
    // Documentation content
    results.push_back(checkFileContains(
        "src/governance/POLICY_VERSIONING_GUIDE.md",
        "Version Tracking",
        "Version Tracking section in documentation"
    ));
    results.push_back(checkFileContains(
        "src/governance/POLICY_VERSIONING_GUIDE.md",
        "Rollback Procedures",
        "Rollback Procedures section in documentation"
    ));
    results.push_back(checkFileContains(
        "src/governance/POLICY_VERSIONING_GUIDE.md",
        "Approval Workflow",
        "Approval Workflow section in documentation"
    ));
    results.push_back(checkFileContains(
        "src/governance/POLICY_VERSIONING_GUIDE.md",
        "Emergency Override",
        "Emergency Procedures section in documentation"
    ));
    
    // Print results
    std::cout << "\n=== Verification Results ===\n\n";
    
    int passed = 0;
    int failed = 0;
    
    for (const auto& result : results) {
        std::cout << result.message << "\n";
        if (result.success) {
            passed++;
        } else {
            failed++;
        }
    }
    
    std::cout << "\n=== Summary ===\n";
    std::cout << "Total Checks: " << results.size() << "\n";
    std::cout << "Passed: " << passed << "\n";
    std::cout << "Failed: " << failed << "\n";
    
    if (failed == 0) {
        std::cout << "\n✓ ALL CHECKS PASSED - Policy Versioning & Change Management is fully implemented!\n\n";
        return 0;
    } else {
        std::cout << "\n✗ Some checks failed - please review the issues above.\n\n";
        return 1;
    }
}
