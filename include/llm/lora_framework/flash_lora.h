/**
 * @file flash_lora.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "llm/lora_framework/gpu_tensor.h"
#include <cstddef>
#include <tuple>

namespace themis {
namespace llm {
namespace lora {

class FlashLoRA {
public:
    struct Config {
        size_t tile_size_m = 128;      // Tile size for batch/sequence dimension
        size_t tile_size_k = 64;       // Tile size for hidden/rank dimension
        bool use_fp16 = false;         // Use FP16 for faster compute (if supported)
        bool fuse_with_attention = false;  // Fuse LoRA with attention (future)
        
        Config() = default;
        
        /**
         * @brief Auto tune for device.
         * @param[in] device_name Name of the device.
         */
        void auto_tune_for_device(const std::string& device_name);
    };
    
    /**
     * @brief Forward.
     * @param[in] input Input parameter.
     * @param[in] B Input parameter.
     * @param[in] A Input parameter.
     * @param[in] scaling Input parameter.
     * @return Return value.
     */
    static GPUTensor forward(
        const GPUTensor& input,
        const GPUTensor& B,
        const GPUTensor& A,
        float scaling
    );
    /**
     * @brief Forward.
     * @param[in] input Input parameter.
     * @param[in] B Input parameter.
     * @param[in] A Input parameter.
     * @param[in] scaling Input parameter.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    static GPUTensor forward(
        const GPUTensor& input,
        const GPUTensor& B,
        const GPUTensor& A,
        float scaling,
        const Config& config
    );
    
    static std::tuple<GPUTensor, GPUTensor, GPUTensor> backward(
        const GPUTensor& grad_output,
        const GPUTensor& input,
        const GPUTensor& B,
        const GPUTensor& A,
        float scaling
    );
    
    static std::tuple<GPUTensor, GPUTensor, GPUTensor> backward(
        const GPUTensor& grad_output,
        const GPUTensor& input,
        const GPUTensor& B,
        const GPUTensor& A,
        float scaling,
        const Config& config
    );
    
    /**
     * @brief Is available.
     * @param[in] device Input parameter.
     * @return True when the operation succeeds.
     */
    static bool is_available(const Device& device);
    
    /**
     * @brief Get recommended config.
     * @param[in] device Input parameter.
     * @param[in] rank Input parameter.
     * @param[in] seq_len Input parameter.
     * @return Return value.
     */
    static Config get_recommended_config(
        const Device& device,
        size_t rank,
        size_t seq_len
    );
    
private:
    /**
     * @brief Internal helper for shape validation
     * @param[in] input Input parameter.
     * @param[in] B Input parameter.
     * @param[in] A Input parameter.
     */
    static void validate_shapes(
        const GPUTensor& input,
        const GPUTensor& B,
        const GPUTensor& A
    );
};

} // namespace lora
} // namespace llm
} // namespace themis
