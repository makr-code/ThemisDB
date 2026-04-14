/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            mixed_precision_inference.cpp                      ║
  Version:         0.0.42                                             ║
  Last Modified:   2026-04-14 18:48:49                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     265                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "llm/mixed_precision_inference.h"
#include <algorithm>
#include <stdexcept>

namespace themis {
namespace llm {

// Private implementation
class MixedPrecisionInference::Impl {
public:
    Impl() = default;
    ~Impl() = default;
};

MixedPrecisionInference::MixedPrecisionInference() 
    : impl_(std::make_unique<Impl>()) {}

MixedPrecisionInference::~MixedPrecisionInference() = default;

PrecisionMode MixedPrecisionInference::selectOptimalPrecision(
    size_t available_vram,
    size_t model_size,
    float tolerance
) {
    // Try precisions from highest to lowest quality
    std::vector<PrecisionMode> modes = {
        PrecisionMode::FP16,
        PrecisionMode::INT8,
        PrecisionMode::Q4,
        PrecisionMode::Q3
    };
    
    for (auto mode : modes) {
        size_t required_size = calculateModelSize(model_size / 4, mode);  // model_size is FP32
        float accuracy = calculateExpectedAccuracy(mode);
        
        if (required_size <= available_vram && (1.0f - accuracy) <= tolerance) {
            return mode;
        }
    }
    
    // If nothing fits, return Q4 (smallest)
    return PrecisionMode::Q4;
}

std::vector<MixedPrecisionInference::LayerPrecisionConfig> 
MixedPrecisionInference::getTuningSchedule(
    const ModelArchitecture& arch,
    size_t available_vram
) {
    std::vector<LayerPrecisionConfig> schedule;
    
    // Strategy: Use higher precision for critical layers (attention)
    // and lower precision for less critical layers (MLP)
    
    size_t budget = available_vram;
    
    for (size_t i = 0; i < arch.layer_types.size(); ++i) {
        LayerPrecisionConfig config;
        config.layer_id = i;
        
        const std::string& layer_type = arch.layer_types[i];
        size_t layer_size = arch.layer_sizes[i];
        
        // Attention layers use FP16, MLP layers can use INT8
        if (layer_type.find("attention") != std::string::npos) {
            config.precision = PrecisionMode::FP16;
            config.rationale = "Attention layer requires high precision";
        } else if (layer_type.find("mlp") != std::string::npos) {
            // Check if we have budget for FP16
            size_t fp16_size = layer_size / 2;  // Assuming layer_size is FP32
            if (fp16_size <= budget) {
                config.precision = PrecisionMode::FP16;
                config.rationale = "Sufficient VRAM budget for FP16";
            } else {
                config.precision = PrecisionMode::INT8;
                config.rationale = "Using INT8 to conserve VRAM";
            }
        } else {
            config.precision = PrecisionMode::FP16;
            config.rationale = "Default precision for layer type: " + layer_type;
        }
        
        // Update budget
        size_t layer_memory = calculateModelSize(layer_size / 4, config.precision);
        if (layer_memory <= budget) {
            budget -= layer_memory;
        }
        
        schedule.push_back(config);
    }
    
    return schedule;
}

size_t MixedPrecisionInference::calculateModelSize(
    size_t num_parameters,
    PrecisionMode precision
) {
    auto info = getPrecisionInfo(precision);
    
    // Handle fractional bytes for Q4 and Q3
    if (precision == PrecisionMode::Q4) {
        return num_parameters / 2;  // 0.5 bytes per parameter
    } else if (precision == PrecisionMode::Q3) {
        return (num_parameters * 3) / 8;  // 0.375 bytes per parameter
    }
    
    return num_parameters * info.bytes_per_param;
}

MixedPrecisionInference::PrecisionInfo 
MixedPrecisionInference::getPrecisionInfo(PrecisionMode precision) {
    PrecisionInfo info;
    info.mode = precision;
    
    switch (precision) {
        case PrecisionMode::FP32:
            info.accuracy_retention = 1.0f;
            info.memory_reduction = 0.0f;
            info.bytes_per_param = 4;
            info.description = "Full precision (32-bit floats)";
            break;
            
        case PrecisionMode::FP16:
            info.accuracy_retention = 0.999f;
            info.memory_reduction = 0.5f;
            info.bytes_per_param = 2;
            info.description = "Half precision (16-bit floats)";
            break;
            
        case PrecisionMode::BFLOAT16:
            info.accuracy_retention = 0.998f;
            info.memory_reduction = 0.5f;
            info.bytes_per_param = 2;
            info.description = "Brain float 16 (better dynamic range than FP16)";
            break;
            
        case PrecisionMode::INT8:
            info.accuracy_retention = 0.98f;
            info.memory_reduction = 0.75f;
            info.bytes_per_param = 1;
            info.description = "8-bit integer quantization";
            break;
            
        case PrecisionMode::Q4:
            info.accuracy_retention = 0.95f;
            info.memory_reduction = 0.875f;
            info.bytes_per_param = 1;  // Will be handled specially: 0.5 bytes
            info.description = "4-bit quantization";
            break;
            
        case PrecisionMode::Q3:
            info.accuracy_retention = 0.90f;
            info.memory_reduction = 0.9125f;
            info.bytes_per_param = 1;  // Will be handled specially: 0.375 bytes
            info.description = "3-bit quantization (experimental)";
            break;
            
        case PrecisionMode::AUTO:
            info.accuracy_retention = 0.0f;
            info.memory_reduction = 0.0f;
            info.bytes_per_param = 0;
            info.description = "Automatic precision selection";
            break;
    }
    
    return info;
}

std::vector<MixedPrecisionInference::PrecisionInfo> 
MixedPrecisionInference::getAllPrecisions() {
    return {
        getPrecisionInfo(PrecisionMode::FP32),
        getPrecisionInfo(PrecisionMode::FP16),
        getPrecisionInfo(PrecisionMode::BFLOAT16),
        getPrecisionInfo(PrecisionMode::INT8),
        getPrecisionInfo(PrecisionMode::Q4),
        getPrecisionInfo(PrecisionMode::Q3)
    };
}

float MixedPrecisionInference::calculateExpectedAccuracy(PrecisionMode precision) {
    return getPrecisionInfo(precision).accuracy_retention;
}

float MixedPrecisionInference::calculateMemoryReduction(PrecisionMode precision) {
    return getPrecisionInfo(precision).memory_reduction;
}

PrecisionMode MixedPrecisionInference::fromString(const std::string& str) {
    if (str == "FP32") return PrecisionMode::FP32;
    if (str == "FP16") return PrecisionMode::FP16;
    if (str == "BFLOAT16" || str == "BF16") return PrecisionMode::BFLOAT16;
    if (str == "INT8") return PrecisionMode::INT8;
    if (str == "Q4") return PrecisionMode::Q4;
    if (str == "Q3") return PrecisionMode::Q3;
    if (str == "AUTO") return PrecisionMode::AUTO;
    
    throw std::invalid_argument("Unknown precision mode: " + str);
}

std::string MixedPrecisionInference::toString(PrecisionMode precision) {
    switch (precision) {
        case PrecisionMode::FP32: return "FP32";
        case PrecisionMode::FP16: return "FP16";
        case PrecisionMode::BFLOAT16: return "BFLOAT16";
        case PrecisionMode::INT8: return "INT8";
        case PrecisionMode::Q4: return "Q4";
        case PrecisionMode::Q3: return "Q3";
        case PrecisionMode::AUTO: return "AUTO";
        default: return "UNKNOWN";
    }
}

bool MixedPrecisionInference::isSupported(PrecisionMode precision) {
    // Stub implementation - would check hardware capabilities
    // In production, would check CUDA compute capability, tensor cores, etc.
    
    switch (precision) {
        case PrecisionMode::FP32:
        case PrecisionMode::FP16:
        case PrecisionMode::INT8:
        case PrecisionMode::Q4:
            return true;  // Widely supported
            
        case PrecisionMode::BFLOAT16:
            // Requires Ampere or newer (SM 8.0+)
            return true;  // Assume supported
            
        case PrecisionMode::Q3:
            return false;  // Experimental
            
        case PrecisionMode::AUTO:
            return true;
            
        default:
            return false;
    }
}

} // namespace llm
} // namespace themis
