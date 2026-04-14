/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            train_legal_lora.cpp                               ║
  Version:         0.0.40                                             ║
  Last Modified:   2026-04-14 06:49:27                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     226                                            ║
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
 * @file train_legal_lora.cpp
 * @brief Main executable for Legal LoRA Training Pipeline
 * 
 * Complete example demonstrating:
 * 1. Multi-source document ingestion (HuggingFace + filesystem)
 * 2. Auto-labeling with Legal Modality Analyzer (PR #1)
 * 3. Knowledge graph enrichment
 * 4. LoRA adapter training
 * 5. Incremental updates
 */

#include "ingestion/ingestion_manager.h"
#include "ingestion/huggingface_connector.h"
#include "ingestion/filesystem_ingester.h"
#include "training/auto_labeler.h"
#include "training/knowledge_graph_enricher.h"
#include "training/incremental_lora_trainer.h"

#include <iostream>
#include <string>
#include <memory>

using namespace themis;

void printProgress(const std::string& phase, size_t current, size_t total, const std::string& status) {
    std::cout << "[" << phase << "] " << current << "/" << total << " - " << status << std::endl;
}

int main(int argc, char** argv) {
    std::cout << "=== Legal LoRA Training Pipeline ===" << std::endl;
    std::cout << "German Administrative Law Domain Adaptation" << std::endl;
    std::cout << std::endl;
    
    // Database connection (adjust for your environment)
    std::string db_connection = "http://localhost:8529/_db/legal_training";
    
    try {
        // ========================================================================
        // Phase 1: Multi-Source Document Ingestion
        // ========================================================================
        std::cout << "Phase 1: Ingesting legal documents..." << std::endl;
        
        ingestion::IngestionManager ingestion_mgr(db_connection);
        
        // Register HuggingFace source
        ingestion::SourceConfig hf_source;
        hf_source.source_id = "huggingface_legal";
        hf_source.type = ingestion::SourceType::HUGGINGFACE;
        hf_source.location = "lexlms/ger_legal_data";
        hf_source.priority = 5;
        hf_source.options["split"] = "train";
        hf_source.options["streaming"] = "true";
        
        if (!ingestion_mgr.registerSource(hf_source)) {
            std::cerr << "Failed to register HuggingFace source" << std::endl;
            return 1;
        }
        
        // Register filesystem source (custom documents)
        ingestion::SourceConfig fs_source;
        fs_source.source_id = "custom_docs";
        fs_source.type = ingestion::SourceType::FILESYSTEM;
        fs_source.location = "/mnt/verwaltung/vorschriften";
        fs_source.priority = 10;  // Higher priority than public data
        fs_source.options["ocr_enabled"] = "true";
        fs_source.options["ocr_language"] = "deu";
        
        if (!ingestion_mgr.registerSource(fs_source)) {
            std::cerr << "Failed to register filesystem source" << std::endl;
            return 1;
        }
        
        // Ingest all sources
        ingestion_mgr.setTargetCollection("legal_documents");
        ingestion_mgr.setParallelProcessing(true, 4);
        
        auto ingestion_report = ingestion_mgr.ingestAll(
            [](const std::string& source_id, size_t processed, size_t total, const std::string& status) {
                printProgress("Ingestion", processed, total, source_id + ": " + status);
            }
        );
        
        std::cout << "Ingestion complete:" << std::endl;
        std::cout << "  Total documents: " << ingestion_report.total_documents << std::endl;
        std::cout << "  Total failures: " << ingestion_report.total_failures << std::endl;
        std::cout << "  Total time: " << ingestion_report.total_time_seconds << "s" << std::endl;
        std::cout << std::endl;
        
        // ========================================================================
        // Phase 2: Auto-Labeling with Legal Modality Analyzer (PR #1)
        // ========================================================================
        std::cout << "Phase 2: Auto-labeling with Legal Modality Analyzer..." << std::endl;
        
        training::AutoLabelConfig label_config;
        label_config.source_collection = "legal_documents";
        label_config.target_collection = "legal_training_samples";
        label_config.language_code = "de";
        label_config.min_confidence = 0.6f;
        label_config.batch_size = 100;
        
        training::LegalAutoLabeler labeler(label_config, db_connection);
        
        auto labeling_stats = labeler.labelAll(
            [](size_t processed, size_t total, const std::string& status) {
                printProgress("Auto-Labeling", processed, total, status);
            }
        );
        
        std::cout << "Auto-labeling complete:" << std::endl;
        std::cout << "  Documents processed: " << labeling_stats.documents_processed << std::endl;
        std::cout << "  Samples created: " << labeling_stats.samples_created << std::endl;
        std::cout << "  High confidence: " << labeling_stats.high_confidence_samples << std::endl;
        std::cout << "  Low confidence: " << labeling_stats.low_confidence_samples << std::endl;
        std::cout << std::endl;
        
        // ========================================================================
        // Phase 3: Knowledge Graph Enrichment
        // ========================================================================
        std::cout << "Phase 3: Enriching with knowledge graph context..." << std::endl;
        
        training::EnrichmentConfig enrich_config;
        enrich_config.target_collection = "legal_training_samples";
        enrich_config.graph_name = "legal_knowledge_graph";
        enrich_config.max_related_items = 5;
        enrich_config.traversal_depth = 2;
        enrich_config.similarity_threshold = 0.7f;
        
        training::KnowledgeGraphEnricher enricher(enrich_config, db_connection);
        
        auto enrichment_stats = enricher.enrichAll(
            [](size_t processed, size_t total, const std::string& status) {
                printProgress("Enrichment", processed, total, status);
            }
        );
        
        std::cout << "Enrichment complete:" << std::endl;
        std::cout << "  Samples processed: " << enrichment_stats.samples_processed << std::endl;
        std::cout << "  Samples enriched: " << enrichment_stats.samples_enriched << std::endl;
        std::cout << "  Context items added: " << enrichment_stats.context_items_added << std::endl;
        std::cout << std::endl;
        
        // ========================================================================
        // Phase 4: LoRA Adapter Training
        // ========================================================================
        std::cout << "Phase 4: Training LoRA adapter..." << std::endl;
        
        training::IncrementalTrainingConfig train_config;
        train_config.training_data_collection = "legal_training_samples";
        train_config.base_model_path = "models/llama-2-7b-chat.gguf";
        train_config.adapter_version = "";  // New adapter
        train_config.rank = 16;
        train_config.alpha = 32.0f;
        train_config.learning_rate = 0.0003f;
        train_config.batch_size = 4;
        train_config.num_epochs = 3;
        train_config.device = "cuda";
        
        training::IncrementalLoRATrainer trainer(train_config, db_connection);
        trainer.setCheckpointing(true, 100);
        
        auto training_result = trainer.train(
            training::TrainingMode::INITIAL,
            [](size_t epoch, size_t step, double loss, const std::string& status) {
                std::cout << "[Training] Epoch " << epoch << " Step " << step 
                         << " Loss: " << loss << " - " << status << std::endl;
            }
        );
        
        if (training_result.success) {
            std::cout << "Training complete:" << std::endl;
            std::cout << "  Version: " << training_result.version << std::endl;
            std::cout << "  Adapter ID: " << training_result.adapter_id << std::endl;
            std::cout << "  Training loss: " << training_result.training_loss << std::endl;
            std::cout << "  Validation loss: " << training_result.validation_loss << std::endl;
            std::cout << "  Accuracy: " << training_result.accuracy << std::endl;
            std::cout << "  Samples trained: " << training_result.samples_trained << std::endl;
            std::cout << "  Training time: " << training_result.training_time_seconds << "s" << std::endl;
            std::cout << std::endl;
            
            // Deploy to production (A/B test with 10% traffic)
            std::cout << "Deploying adapter with 10% traffic split..." << std::endl;
            if (trainer.deployVersion(training_result.version, 0.1f)) {
                std::cout << "Deployment successful!" << std::endl;
            } else {
                std::cerr << "Deployment failed!" << std::endl;
            }
        } else {
            std::cerr << "Training failed: " << training_result.error_message << std::endl;
            return 1;
        }
        
        std::cout << std::endl;
        std::cout << "=== Pipeline Complete ===" << std::endl;
        std::cout << "Legal LoRA adapter trained and deployed successfully!" << std::endl;
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
