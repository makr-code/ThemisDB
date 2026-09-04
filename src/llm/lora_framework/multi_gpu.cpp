/**
 * @file multi_gpu.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/lora_framework/multi_gpu.h"
#include "llm/lora_framework/cuda_kernels.h"
#include "llm/lora_framework/hip_kernels.h"
#include <spdlog/spdlog.h>
#include <stdexcept>

namespace themis {
namespace llm {
namespace lora {

MultiGPUContext::MultiGPUContext(int num_gpus, const std::vector<int>& gpu_ids) 
    : gpu_type_(DeviceType::CPU), is_homogeneous_(true) {
    
    detect_gpus(num_gpus, gpu_ids);
    
    if (devices_.empty()) {
        spdlog::warn("No GPUs detected, multi-GPU context created with 0 devices");
        return;
    }
    
    spdlog::info("MultiGPUContext created with {} GPUs",static_cast<int>(devices_.size()));
    for (size_t i = 0; i <static_cast<int>(devices_.size()); ++i) {
        spdlog::info("  Rank {}: Device {} ({})", 
            i, devices_[i].device_id, 
            devices_[i].type == DeviceType::CUDA ? "CUDA" : 
            devices_[i].type == DeviceType::HIP ? "HIP" : "Unknown");
    }
}

void MultiGPUContext::detect_gpus(int num_gpus, const std::vector<int>& gpu_ids) {
    devices_.clear();
    
    // If specific GPU IDs are provided, use them
    if (!gpu_ids.empty()) {
        DeviceType first_type = DeviceType::CPU;
        
        for (int gpu_id : gpu_ids) {
            Device device = Device::cuda(gpu_id);
            auto backends = GPUMemoryManager::detect_backends();
            bool has_cuda = false, has_hip = false;
            for (const auto& backend : backends) {
                if (backend.type == acceleration::BackendType::CUDA && backend.available) {
                  has_cuda = true;
                }
                if (backend.type == acceleration::BackendType::HIP && backend.available) {
                  has_hip = true;
                }
            }
            
            // Try CUDA first
            if (has_cuda) {
                device = Device::cuda(gpu_id);
                if (first_type == DeviceType::CPU) {
                    first_type = DeviceType::CUDA;
                    gpu_type_ = DeviceType::CUDA;
                } else if (first_type != DeviceType::CUDA) {
                    is_homogeneous_ = false;
                }
            }
            // Try HIP if CUDA not available
            else if (has_hip) {
                device = Device::hip(gpu_id);
                if (first_type == DeviceType::CPU) {
                    first_type = DeviceType::HIP;
                    gpu_type_ = DeviceType::HIP;
                } else if (first_type != DeviceType::HIP) {
                    is_homogeneous_ = false;
                }
            }
            else {
                spdlog::warn("GPU {} not available, skipping", gpu_id);
                continue;
            }
            
            devices_.push_back(device);
        }
        return;
    }
    
    // Auto-detect GPUs
    int available_gpus = 0;
    DeviceType detected_type = DeviceType::CPU;
    
    // Check CUDA
    auto backends = GPUMemoryManager::detect_backends();
    bool has_cuda = false, has_hip = false;
    for (const auto& backend : backends) {
        if (backend.type == acceleration::BackendType::CUDA && backend.available) {
          has_cuda = true;
        }
        if (backend.type == acceleration::BackendType::HIP && backend.available) {
          has_hip = true;
        }
    }
    
    if (has_cuda) {
#ifdef THEMIS_ENABLE_CUDA
        int cuda_device_count = 0;
        cudaError_t err = cudaGetDeviceCount(&cuda_device_count);
        if (err != cudaSuccess) {
            spdlog::warn("Failed to get CUDA device count: {}", cudaGetErrorString(err));
            cuda_device_count = 0;
        }
        available_gpus = cuda_device_count;
        detected_type = DeviceType::CUDA;
        gpu_type_ = DeviceType::CUDA;
        spdlog::info("Detected {} CUDA GPUs", available_gpus);
#endif
    }
    // Check HIP if CUDA not available
    else if (has_hip) {
#ifdef THEMIS_ENABLE_HIP
        int hip_device_count = 0;
        hipError_t err = hipGetDeviceCount(&hip_device_count);
        if (err != hipSuccess) {
            spdlog::warn("Failed to get HIP device count: {}", hipGetErrorString(err));
            hip_device_count = 0;
        }
        available_gpus = hip_device_count;
        detected_type = DeviceType::HIP;
        gpu_type_ = DeviceType::HIP;
        spdlog::info("Detected {} HIP GPUs", available_gpus);
#endif
    }
    
    if (available_gpus == 0) {
        spdlog::warn("No GPUs available");
        return;
    }
    
    // Determine how many GPUs to use
    int gpus_to_use = (num_gpus <= 0 || num_gpus > available_gpus) ? available_gpus : num_gpus;
    
    // Create devices
    for (int i = 0; i < gpus_to_use; ++i) {
        if (detected_type == DeviceType::CUDA) {
            devices_.push_back(Device::cuda(i));
        } else if (detected_type == DeviceType::HIP) {
            devices_.push_back(Device::hip(i));
        }
    }
}

Device MultiGPUContext::get_device([[maybe_unused]] int rank) const {
    if (rank < 0 || rank >= num_gpus()) {
        throw std::out_of_range("Invalid rank: " + std::to_string(rank));
    }
    return devices_[rank];
}

void MultiGPUContext::synchronize_all() const {
#if defined(THEMIS_ENABLE_CUDA) || defined(THEMIS_ENABLE_HIP)
    for (const auto& device : devices_) {
#ifdef THEMIS_ENABLE_CUDA
        if (device.type == DeviceType::CUDA) {
            // REL-34: check cudaSetDevice return value before synchronize
            cudaError_t set_err = cudaSetDevice(device.device_id);
            if (set_err != cudaSuccess) {
                spdlog::warn("MultiGPUContext::synchronize_all: cudaSetDevice({}) failed: {}",
                             device.device_id, cudaGetErrorString(set_err));
                continue;
            }
            // REL-62: check cudaDeviceSynchronize return value
            cudaError_t sync_err = cudaDeviceSynchronize();
            if (sync_err != cudaSuccess) {
                spdlog::warn("MultiGPUContext::synchronize_all: cudaDeviceSynchronize({}) failed: {}",
                             device.device_id, cudaGetErrorString(sync_err));
            }
        }
#endif
#ifdef THEMIS_ENABLE_HIP
        if (device.type == DeviceType::HIP) {
            // REL-35: check hipSetDevice return value before synchronize
            hipError_t set_err = hipSetDevice(device.device_id);
            if (set_err != hipSuccess) {
                spdlog::warn("MultiGPUContext::synchronize_all: hipSetDevice({}) failed: {}",
                             device.device_id, hipGetErrorString(set_err));
                continue;
            }
            // REL-63: check hipDeviceSynchronize return value
            hipError_t sync_err = hipDeviceSynchronize();
            if (sync_err != hipSuccess) {
                spdlog::warn("MultiGPUContext::synchronize_all: hipDeviceSynchronize({}) failed: {}",
                             device.device_id, hipGetErrorString(sync_err));
            }
        }
#endif
    }
#else
    static_cast<void>(devices_);
#endif
}

GPUTopology GPUTopology::detect(const std::vector<Device>& devices) {
    GPUTopology topology;
    topology.num_gpus = static_cast<int>(devices.size());
    
    if (topology.num_gpus == 0) {
        return topology;
    }
    
    // Initialize bandwidth matrix
    topology.bandwidth_matrix.resize(topology.num_gpus, 
                                     std::vector<float>(topology.num_gpus, 0.0f));
    
#ifdef THEMIS_ENABLE_CUDA
    if (devices[0].type == DeviceType::CUDA) {
        // Check for NVLink
        for (int i = 0; i < topology.num_gpus; ++i) {
            for (int j = 0; j < topology.num_gpus; ++j) {
                if (i == j) {
                    topology.bandwidth_matrix[i][j] = 0.0f;
                    continue;
                }
                
                int can_access_peer = 0;
                // REL-36: capture and check cudaDeviceCanAccessPeer return value
                cudaError_t cap_err = cudaDeviceCanAccessPeer(
                    &can_access_peer, devices[i].device_id, devices[j].device_id);
                if (cap_err != cudaSuccess) {
                    spdlog::warn("GPUTopology: cudaDeviceCanAccessPeer({},{}) failed: {}",
                                 devices[i].device_id, devices[j].device_id,
                                 cudaGetErrorString(cap_err));
                    can_access_peer = 0;
                }
                
                if (can_access_peer) {
                    topology.has_pcie_p2p = true;
                    // Assume PCIe bandwidth (adjust based on actual hardware)
                    topology.bandwidth_matrix[i][j] = 16.0f;  // GB/s
                    
                    // Try to detect NVLink (higher bandwidth)
                    // Note: This is simplified; real detection requires NVML
                    int nvlink_links = 0;
                    // cudaDeviceGetNvLinkCapability() would go here
                    if (nvlink_links > 0) {
                        topology.has_nvlink = true;
                        topology.bandwidth_matrix[i][j] = 300.0f;  // NVLink bandwidth
                    }
                } else {
                    // PCIe through CPU
                    topology.bandwidth_matrix[i][j] = 8.0f;  // GB/s
                }
            }
        }
        
        spdlog::info("GPU topology: {} GPUs, NVLink={}, PCIe P2P={}", 
                    topology.num_gpus, topology.has_nvlink, topology.has_pcie_p2p);
    }
#endif
    
#ifdef THEMIS_ENABLE_HIP
    if (devices[0].type == DeviceType::HIP) {
        // Similar detection for AMD GPUs with Infinity Fabric
        for (int i = 0; i < topology.num_gpus; ++i) {
            for (int j = 0; j < topology.num_gpus; ++j) {
                if (i == j) {
                    topology.bandwidth_matrix[i][j] = 0.0f;
                    continue;
                }
                
                int can_access_peer = 0;
                // REL-37: capture and check hipDeviceCanAccessPeer return value
                hipError_t cap_err = hipDeviceCanAccessPeer(
                    &can_access_peer, devices[i].device_id, devices[j].device_id);
                if (cap_err != hipSuccess) {
                    spdlog::warn("GPUTopology: hipDeviceCanAccessPeer({},{}) failed: {}",
                                 devices[i].device_id, devices[j].device_id,
                                 hipGetErrorString(cap_err));
                    can_access_peer = 0;
                }
                
                if (can_access_peer) {
                    topology.has_pcie_p2p = true;
                    topology.bandwidth_matrix[i][j] = 16.0f;  // PCIe bandwidth
                } else {
                    topology.bandwidth_matrix[i][j] = 8.0f;
                }
            }
        }
        
        spdlog::info("GPU topology: {} GPUs, PCIe P2P={}", 
                    topology.num_gpus, topology.has_pcie_p2p);
    }
#endif
    
    return topology;
}

} // namespace lora
} // namespace llm
} // namespace themis
