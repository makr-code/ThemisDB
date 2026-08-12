/**
 * @file vram_secure_clear.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <cstddef>
#include <cstdint>

namespace themis {
namespace security {

/**
 * @brief Secure memory clearing for GPU VRAM
 * 
 * This utility provides multi-pass secure clearing of GPU memory to prevent
 * cold-boot attacks and inter-process memory leakage. It performs multiple
 * passes of cudaMemset with different patterns before calling cudaFree.
 * 
 * Security rationale:
 * - Single-pass zeroing may not fully clear memory cells
 * - Multi-pass with different patterns increases security margin
 * - Critical for protecting encryption keys, model weights, embeddings
 * 
 * Compliance: GDPR Art. 32, SOC 2 CC6.1, HIPAA § 164.310
 */
class VRAMSecureClear {
public:
    /**
     * @brief Configuration for secure VRAM clearing
     */
    struct Config {
        // Number of overwrite passes (default: 3)
        // Pass 1: 0x00, Pass 2: 0xFF, Pass 3: 0xAA
        int num_passes = 3;
        
        // Whether to verify clearing (read back and check)
        bool verify_clear = false;
        
        // Whether to log clearing operations for audit
        bool audit_log = true;
    };
    
    /**
     * @brief Securely clear GPU memory with multi-pass overwrite
     * 
     * @param ptr Device pointer to clear
     * @param size_bytes Size of memory region in bytes
     * @return true if successful, false on error
     */
    static bool secureClearCUDA(void* ptr, size_t size_bytes);
    
    /**
     * @brief Securely clear GPU memory with multi-pass overwrite (with configuration)
     * 
     * @param ptr Device pointer to clear
     * @param size_bytes Size of memory region in bytes
     * @param config Clearing configuration
     * @return true if successful, false on error
     */
    static bool secureClearCUDA(void* ptr, size_t size_bytes, const Config& config);
    
    /**
     * @brief Securely clear HIP memory with multi-pass overwrite
     * 
     * @param ptr Device pointer to clear
     * @param size_bytes Size of memory region in bytes
     * @return true if successful, false on error
     */
    static bool secureClearHIP(void* ptr, size_t size_bytes);
    
    /**
     * @brief Securely clear HIP memory with multi-pass overwrite (with configuration)
     * 
     * @param ptr Device pointer to clear
     * @param size_bytes Size of memory region in bytes
     * @param config Clearing configuration
     * @return true if successful, false on error
     */
    static bool secureClearHIP(void* ptr, size_t size_bytes, const Config& config);
    
    /**
     * @brief Securely clear CPU memory (for comparison/fallback)
     * 
     * Uses volatile writes to prevent compiler optimization
     * 
     * @param ptr Memory pointer to clear
     * @param size_bytes Size of memory region in bytes
     */
    static void secureClearCPU(void* ptr, size_t size_bytes);
    
    /**
     * @brief Securely clear CPU memory with configuration (for comparison/fallback)
     * 
     * Uses volatile writes to prevent compiler optimization
     * 
     * @param ptr Memory pointer to clear
     * @param size_bytes Size of memory region in bytes
     * @param config Clearing configuration
     */
    static void secureClearCPU(void* ptr, size_t size_bytes, const Config& config);

private:
    // Overwrite patterns for multi-pass clearing
    static constexpr uint8_t PATTERNS[] = {0x00, 0xFF, 0xAA, 0x55, 0xCC, 0x33};
};

} // namespace security
} // namespace themis
