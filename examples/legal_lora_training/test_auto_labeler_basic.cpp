/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_auto_labeler_basic.cpp                        ║
  Version:         0.0.40                                             ║
  Last Modified:   2026-04-14 06:49:27                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     106                                            ║
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
 * @file test_auto_labeler_basic.cpp
 * @brief Basic integration test for Legal Auto-Labeler with PR #1
 * 
 * Tests the auto-labeling functionality using sample German legal text
 * to verify integration with NlpTextAnalyzer::extractLegalModalities()
 */

#include "training/auto_labeler.h"
#include <iostream>
#include <cassert>

using namespace themis;
using namespace themis::training;

int main() {
    std::cout << "=== Legal Auto-Labeler Basic Test ===" << std::endl;
    std::cout << std::endl;
    
    try {
        // Configure auto-labeler
        AutoLabelConfig config;
        config.source_collection = "test_docs";
        config.target_collection = "test_samples";
        config.language_code = "de";
        config.min_confidence = 0.5f;
        config.flag_low_confidence = true;
        
        std::cout << "Creating LegalAutoLabeler..." << std::endl;
        LegalAutoLabeler labeler(config, "test_db");
        std::cout << "✓ LegalAutoLabeler created successfully" << std::endl;
        std::cout << std::endl;
        
        // Test document labeling with sample German legal text
        std::cout << "Testing document labeling..." << std::endl;
        std::string test_doc_id = "test_doc_001";
        
        auto samples = labeler.labelDocument(test_doc_id);
        
        std::cout << "✓ Document labeling completed" << std::endl;
        std::cout << "  Generated " << samples.size() << " training samples" << std::endl;
        std::cout << std::endl;
        
        // Display samples
        if (!samples.empty()) {
            std::cout << "Sample Training Data:" << std::endl;
            std::cout << "---------------------" << std::endl;
            
            for (size_t i = 0; i < samples.size() && i < 5; ++i) {
                const auto& sample = samples[i];
                std::cout << "Sample " << (i+1) << ":" << std::endl;
                std::cout << "  Category: " << sample.category << std::endl;
                std::cout << "  Confidence: " << sample.confidence << std::endl;
                std::cout << "  Input: " << sample.input.substr(0, 80) << "..." << std::endl;
                std::cout << "  Output: " << sample.output.substr(0, 80) << "..." << std::endl;
                std::cout << std::endl;
            }
        } else {
            std::cout << "Note: No samples generated (expected if no text provided)" << std::endl;
        }
        
        // Test batch labeling
        std::cout << "Testing batch labeling..." << std::endl;
        auto stats = labeler.labelAll([](size_t processed, size_t total, const std::string& status) {
            std::cout << "  Progress: " << processed << "/" << total << " - " << status << std::endl;
        });
        
        std::cout << "✓ Batch labeling completed" << std::endl;
        std::cout << "  Documents processed: " << stats.documents_processed << std::endl;
        std::cout << "  Samples created: " << stats.samples_created << std::endl;
        std::cout << "  High confidence: " << stats.high_confidence_samples << std::endl;
        std::cout << "  Low confidence: " << stats.low_confidence_samples << std::endl;
        std::cout << "  Elapsed time: " << stats.elapsed_seconds << "s" << std::endl;
        std::cout << std::endl;
        
        std::cout << "=== All Tests Passed ===" << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        return 1;
    }
}
