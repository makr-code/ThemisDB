/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            lora_rope_example.cpp                              ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:43:54                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     275                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "index/lora_rope.h"
#include <iostream>
#include <iomanip>
#include <numeric>

using namespace themis;

/**
 * Example: LoRA Integration for Dynamic Rotation Patterns
 * 
 * This example demonstrates how to use LoRA adapters with RoPE (Rotary Position Embeddings)
 * to enable dynamic, context-aware rotation patterns for different domains or tasks.
 */

// Helper function to create a normalized test embedding
std::vector<float> createTestEmbedding(size_t dim, float base_value = 1.0f) {
    std::vector<float> embedding(dim);
    std::iota(embedding.begin(), embedding.end(), base_value);
    
    // Normalize to unit length
    float norm = 0.0f;
    for (auto v : embedding) {
        norm += v * v;
    }
    norm = std::sqrt(norm);
    
    for (auto& v : embedding) {
        v /= norm;
    }
    
    return embedding;
}

// Helper function to compute cosine similarity
float cosineSimilarity(const std::vector<float>& a, const std::vector<float>& b) {
    if (a.size() != b.size()) {
      return 0.0f;
    }
    
    float dot = 0.0f, norm_a = 0.0f, norm_b = 0.0f;
    for (size_t i = 0; i < a.size(); ++i) {
        dot += a[i] * b[i];
        norm_a += a[i] * a[i];
        norm_b += b[i] * b[i];
    }
    
    if (norm_a == 0.0f || norm_b == 0.0f) {
      return 0.0f;
    }
    return dot / (std::sqrt(norm_a) * std::sqrt(norm_b));
}

int main() {
    std::cout << "=== LoRA-RoPE Integration Example ===" << std::endl;
    std::cout << std::endl;
    
    // ========================================================================
    // Step 1: Configure RoPE
    // ========================================================================
    std::cout << "Step 1: Configuring RoPE..." << std::endl;
    
    RotationConfig config;
    config.hidden_dim = 128;
    config.num_rotation_pairs = 64;
    config.base_theta = 10000.0;
    config.computeThetaCache();
    
    std::cout << "  - Hidden dimension: " << config.hidden_dim << std::endl;
    std::cout << "  - Rotation pairs: " << config.num_rotation_pairs << std::endl;
    std::cout << "  - Base theta: " << config.base_theta << std::endl;
    std::cout << std::endl;
    
    // ========================================================================
    // Step 2: Create LoRA-Enhanced RoPE
    // ========================================================================
    std::cout << "Step 2: Creating LoRA-Enhanced RoPE..." << std::endl;
    
    LoRARotaryEmbedding lora_rope(config);
    
    std::cout << "  ✓ LoRA-RoPE initialized" << std::endl;
    std::cout << std::endl;
    
    // ========================================================================
    // Step 3: Create Domain-Specific Adapters
    // ========================================================================
    std::cout << "Step 3: Creating domain-specific LoRA adapters..." << std::endl;
    
    // Medical domain adapter (for medical text embeddings)
    auto medical_adapter = LoRARopeAdapter::createRandom(
        "medical",
        "medical",
        config.num_rotation_pairs,
        8,   // rank
        0.2f // alpha
    );
    lora_rope.registerAdapter("medical", medical_adapter);
    std::cout << "  ✓ Medical adapter registered (rank=8, alpha=0.2)" << std::endl;
    
    // Legal domain adapter (for legal document embeddings)
    auto legal_adapter = LoRARopeAdapter::createRandom(
        "legal",
        "legal",
        config.num_rotation_pairs,
        8,
        0.2f
    );
    lora_rope.registerAdapter("legal", legal_adapter);
    std::cout << "  ✓ Legal adapter registered (rank=8, alpha=0.2)" << std::endl;
    
    // Technical domain adapter (for technical documentation)
    auto technical_adapter = LoRARopeAdapter::createRandom(
        "technical",
        "technical",
        config.num_rotation_pairs,
        8,
        0.2f
    );
    lora_rope.registerAdapter("technical", technical_adapter);
    std::cout << "  ✓ Technical adapter registered (rank=8, alpha=0.2)" << std::endl;
    
    std::cout << std::endl;
    
    // ========================================================================
    // Step 4: Test Base Rotation vs LoRA-Modified Rotation
    // ========================================================================
    std::cout << "Step 4: Comparing base rotation with LoRA-modified rotations..." << std::endl;
    
    auto embedding = createTestEmbedding(config.hidden_dim);
    size_t position = 10;
    
    // Base rotation (no adapter)
    auto base_rotated = lora_rope.rotate(embedding, position);
    std::cout << "  - Base rotation computed" << std::endl;
    
    // Medical domain rotation
    auto medical_rotated = lora_rope.rotateWithAdapter(embedding, position, "medical");
    float medical_similarity = cosineSimilarity(base_rotated, medical_rotated);
    std::cout << "  - Medical rotation computed (similarity to base: " 
              << std::fixed << std::setprecision(4) << medical_similarity << ")" << std::endl;
    
    // Legal domain rotation
    auto legal_rotated = lora_rope.rotateWithAdapter(embedding, position, "legal");
    float legal_similarity = cosineSimilarity(base_rotated, legal_rotated);
    std::cout << "  - Legal rotation computed (similarity to base: " 
              << std::fixed << std::setprecision(4) << legal_similarity << ")" << std::endl;
    
    // Technical domain rotation
    auto technical_rotated = lora_rope.rotateWithAdapter(embedding, position, "technical");
    float technical_similarity = cosineSimilarity(base_rotated, technical_rotated);
    std::cout << "  - Technical rotation computed (similarity to base: " 
              << std::fixed << std::setprecision(4) << technical_similarity << ")" << std::endl;
    
    std::cout << std::endl;
    
    // ========================================================================
    // Step 5: Compare Domain-Specific Rotations
    // ========================================================================
    std::cout << "Step 5: Comparing domain-specific rotations..." << std::endl;
    
    float medical_legal_sim = cosineSimilarity(medical_rotated, legal_rotated);
    float medical_technical_sim = cosineSimilarity(medical_rotated, technical_rotated);
    float legal_technical_sim = cosineSimilarity(legal_rotated, technical_rotated);
    
    std::cout << "  - Medical vs Legal: " << std::fixed << std::setprecision(4) 
              << medical_legal_sim << std::endl;
    std::cout << "  - Medical vs Technical: " << medical_technical_sim << std::endl;
    std::cout << "  - Legal vs Technical: " << legal_technical_sim << std::endl;
    std::cout << std::endl;
    
    // ========================================================================
    // Step 6: Adapter Blending (Multi-Domain Scenario)
    // ========================================================================
    std::cout << "Step 6: Demonstrating adapter blending..." << std::endl;
    
    // Blend medical and legal adapters (e.g., for medico-legal documents)
    auto blended_medico_legal = lora_rope.rotateWithAdapterBlend(
        embedding, position,
        {"medical", "legal"},
        {0.6f, 0.4f}  // 60% medical, 40% legal
    );
    
    float blend_medical_sim = cosineSimilarity(blended_medico_legal, medical_rotated);
    float blend_legal_sim = cosineSimilarity(blended_medico_legal, legal_rotated);
    
    std::cout << "  - Medico-Legal blend (60% medical, 40% legal):" << std::endl;
    std::cout << "    * Similarity to medical: " << std::fixed << std::setprecision(4) 
              << blend_medical_sim << std::endl;
    std::cout << "    * Similarity to legal: " << blend_legal_sim << std::endl;
    std::cout << std::endl;
    
    // ========================================================================
    // Step 7: Batch Processing
    // ========================================================================
    std::cout << "Step 7: Demonstrating batch processing..." << std::endl;
    
    std::vector<std::vector<float>> embeddings;
    std::vector<size_t> positions;
    
    for (size_t i = 0; i < 5; ++i) {
        embeddings.push_back(createTestEmbedding(config.hidden_dim, static_cast<float>(i + 1)));
        positions.push_back(i * 10);
    }
    
    auto medical_batch = lora_rope.rotateBatchWithAdapter(embeddings, positions, "medical");
    
    std::cout << "  ✓ Processed " << medical_batch.size() 
              << " embeddings with medical adapter" << std::endl;
    std::cout << std::endl;
    
    // ========================================================================
    // Step 8: Dynamic Adapter Management
    // ========================================================================
    std::cout << "Step 8: Demonstrating dynamic adapter management..." << std::endl;
    
    // List all adapters
    auto adapter_names = lora_rope.listAdapters();
    std::cout << "  - Registered adapters: ";
    for (size_t i = 0; i < adapter_names.size(); ++i) {
        std::cout << adapter_names[i];
        if (i < adapter_names.size() - 1) {
          std::cout << ", ";
        }
    }
    std::cout << std::endl;
    
    // Disable medical adapter
    lora_rope.setAdapterEnabled("medical", false);
    std::cout << "  - Medical adapter disabled" << std::endl;
    
    // Rotation with disabled adapter should return base rotation
    auto medical_disabled = lora_rope.rotateWithAdapter(embedding, position, "medical");
    float disabled_similarity = cosineSimilarity(base_rotated, medical_disabled);
    std::cout << "  - Similarity to base (with disabled adapter): " 
              << std::fixed << std::setprecision(4) << disabled_similarity << std::endl;
    
    // Re-enable medical adapter
    lora_rope.setAdapterEnabled("medical", true);
    std::cout << "  - Medical adapter re-enabled" << std::endl;
    std::cout << std::endl;
    
    // ========================================================================
    // Summary
    // ========================================================================
    std::cout << "=== Summary ===" << std::endl;
    std::cout << "✓ Successfully demonstrated LoRA-RoPE integration" << std::endl;
    std::cout << "✓ Created domain-specific adapters (medical, legal, technical)" << std::endl;
    std::cout << "✓ Showed how adapters modify rotation behavior" << std::endl;
    std::cout << "✓ Demonstrated adapter blending for multi-domain scenarios" << std::endl;
    std::cout << "✓ Illustrated batch processing and dynamic adapter management" << std::endl;
    std::cout << std::endl;
    std::cout << "Key Benefits:" << std::endl;
    std::cout << "  • Different rotation patterns for different domains/tasks" << std::endl;
    std::cout << "  • Lightweight: LoRA adapters require minimal memory" << std::endl;
    std::cout << "  • Dynamic: Switch adapters without retraining" << std::endl;
    std::cout << "  • Flexible: Blend multiple adapters for hybrid scenarios" << std::endl;
    std::cout << std::endl;
    
    return 0;
}
