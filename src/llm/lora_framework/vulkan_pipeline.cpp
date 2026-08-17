/**
 * @file vulkan_pipeline.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=3, H=4, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/lora_framework/vulkan_pipeline.h"
#include <fstream>
#include <stdexcept>
#include <cstring>
#include <iostream>

#if THEMIS_HAS_VULKAN_HEADER

namespace themis {
namespace lora {
namespace vulkan {

VulkanComputePipeline::VulkanComputePipeline(VulkanContext* context, const std::string& shader_path)
    : context_(context)
    , shader_code_(load_shader_file(shader_path)) {
    
    if (!context_ || !context_->is_initialized()) {
        throw std::runtime_error("VulkanContext not initialized");
    }
}

VulkanComputePipeline::VulkanComputePipeline(VulkanContext* context,
                                               const std::vector<uint32_t>& shader_code)
    : context_(context)
    , shader_code_(shader_code) {
    
    if (!context_ || !context_->is_initialized()) {
        throw std::runtime_error("VulkanContext not initialized");
    }
}

VulkanComputePipeline::~VulkanComputePipeline() noexcept {
    // Phase2-LLM-B1: exception_in_destructor — context_->wait_for_fence(),
    // destroy_fence(), free_command_buffer(), and vkDestroy* may throw (Vulkan
    // validation layers or custom allocators). Suppress to honour noexcept.
    try {
        if (fence_ != VK_NULL_HANDLE) {
            context_->wait_for_fence(fence_);
            context_->destroy_fence(fence_);
            fence_ = VK_NULL_HANDLE;
        }

        if (command_buffer_ != VK_NULL_HANDLE) {
            context_->free_command_buffer(command_buffer_);
            command_buffer_ = VK_NULL_HANDLE;
        }

        if (descriptor_pool_ != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(context_->device(), descriptor_pool_, nullptr);
            descriptor_pool_ = VK_NULL_HANDLE;
        }

        if (pipeline_ != VK_NULL_HANDLE) {
            vkDestroyPipeline(context_->device(), pipeline_, nullptr);
            pipeline_ = VK_NULL_HANDLE;
        }

        if (pipeline_layout_ != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(context_->device(), pipeline_layout_, nullptr);
            pipeline_layout_ = VK_NULL_HANDLE;
        }

        if (descriptor_set_layout_ != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(context_->device(), descriptor_set_layout_, nullptr);
            descriptor_set_layout_ = VK_NULL_HANDLE;
        }

        if (shader_module_ != VK_NULL_HANDLE) {
            vkDestroyShaderModule(context_->device(), shader_module_, nullptr);
            shader_module_ = VK_NULL_HANDLE;
        }
    } catch (const std::exception& e) {
        (void)e; // Vulkan driver owns the underlying resources; suppress safely
    } catch (...) {}
}

VulkanComputePipeline::VulkanComputePipeline(VulkanComputePipeline&& other) noexcept
    : context_(other.context_)
    , shader_code_(std::move(other.shader_code_))
    , shader_module_(other.shader_module_)
    , pipeline_(other.pipeline_)
    , pipeline_layout_(other.pipeline_layout_)
    , descriptor_set_layout_(other.descriptor_set_layout_)
    , descriptor_pool_(other.descriptor_pool_)
    , descriptor_set_(other.descriptor_set_)
    , command_buffer_(other.command_buffer_)
    , fence_(other.fence_)
    , push_constant_size_(other.push_constant_size_)
    , push_constant_data_(std::move(other.push_constant_data_))
    , buffer_bindings_(std::move(other.buffer_bindings_))
    , buffer_sizes_(std::move(other.buffer_sizes_))
    , descriptors_dirty_(other.descriptors_dirty_) {
    
    other.shader_module_ = VK_NULL_HANDLE;
    other.pipeline_ = VK_NULL_HANDLE;
    other.pipeline_layout_ = VK_NULL_HANDLE;
    other.descriptor_set_layout_ = VK_NULL_HANDLE;
    other.descriptor_pool_ = VK_NULL_HANDLE;
    other.descriptor_set_ = VK_NULL_HANDLE;
    other.command_buffer_ = VK_NULL_HANDLE;
    other.fence_ = VK_NULL_HANDLE;
}

VulkanComputePipeline& VulkanComputePipeline::operator=(VulkanComputePipeline&& other) noexcept {
    if (this != &other) {
        // Cleanup current resources
        if (fence_ != VK_NULL_HANDLE) {
            context_->wait_for_fence(fence_);
            context_->destroy_fence(fence_);
        }
        if (command_buffer_ != VK_NULL_HANDLE) {
            context_->free_command_buffer(command_buffer_);
        }
        if (descriptor_pool_ != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(context_->device(), descriptor_pool_, nullptr);
        }
        if (pipeline_ != VK_NULL_HANDLE) {
            vkDestroyPipeline(context_->device(), pipeline_, nullptr);
        }
        if (pipeline_layout_ != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(context_->device(), pipeline_layout_, nullptr);
        }
        if (descriptor_set_layout_ != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(context_->device(), descriptor_set_layout_, nullptr);
        }
        if (shader_module_ != VK_NULL_HANDLE) {
            vkDestroyShaderModule(context_->device(), shader_module_, nullptr);
        }
        
        // Move from other
        context_ = other.context_;
        shader_code_ = std::move(other.shader_code_);
        shader_module_ = other.shader_module_;
        pipeline_ = other.pipeline_;
        pipeline_layout_ = other.pipeline_layout_;
        descriptor_set_layout_ = other.descriptor_set_layout_;
        descriptor_pool_ = other.descriptor_pool_;
        descriptor_set_ = other.descriptor_set_;
        command_buffer_ = other.command_buffer_;
        fence_ = other.fence_;
        push_constant_size_ = other.push_constant_size_;
        push_constant_data_ = std::move(other.push_constant_data_);
        buffer_bindings_ = std::move(other.buffer_bindings_);
        buffer_sizes_ = std::move(other.buffer_sizes_);
        descriptors_dirty_ = other.descriptors_dirty_;
        
        other.shader_module_ = VK_NULL_HANDLE;
        other.pipeline_ = VK_NULL_HANDLE;
        other.pipeline_layout_ = VK_NULL_HANDLE;
        other.descriptor_set_layout_ = VK_NULL_HANDLE;
        other.descriptor_pool_ = VK_NULL_HANDLE;
        other.descriptor_set_ = VK_NULL_HANDLE;
        other.command_buffer_ = VK_NULL_HANDLE;
        other.fence_ = VK_NULL_HANDLE;
    }
    return *this;
}

bool VulkanComputePipeline::create(size_t push_constant_size) {
    push_constant_size_ = push_constant_size;
    
    if (push_constant_size_ > 0) {
        push_constant_data_.resize(push_constant_size_);
    }
    
    // Create shader module
    shader_module_ = create_shader_module(shader_code_);
    if (shader_module_ == VK_NULL_HANDLE) {
        return false;
    }
    
    // Create descriptor set layout
    if (!create_descriptor_set_layout()) {
        return false;
    }
    
    // Create pipeline layout
    if (!create_pipeline_layout(push_constant_size)) {
        return false;
    }
    
    // Create compute pipeline
    if (!create_compute_pipeline()) {
        return false;
    }
    
    // Create descriptor pool and allocate sets
    if (!create_descriptor_pool()) {
        return false;
    }
    
    if (!allocate_descriptor_sets()) {
        return false;
    }
    
    // Allocate command buffer
    command_buffer_ = context_->allocate_command_buffer();
    
    // Create fence
    fence_ = context_->create_fence(true); // Start signaled
    
    return true;
}

std::vector<uint32_t> VulkanComputePipeline::load_shader_file(const std::string& path) {
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open shader file: " + path);
    }
    
    size_t file_size = static_cast<size_t>(file.tellg());
    std::vector<uint32_t> buffer(file_size / sizeof(uint32_t));
    
    file.seekg(0);
    file.read(reinterpret_cast<char*>(buffer.data()), file_size);
    file.close();
    
    return buffer;
}

VkShaderModule VulkanComputePipeline::create_shader_module(const std::vector<uint32_t>& code) {
    VkShaderModuleCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = code.size() * sizeof(uint32_t);
    create_info.pCode = code.data();
    
    VkShaderModule shader_module = VK_NULL_HANDLE;
    VkResult result = vkCreateShaderModule(context_->device(), &create_info,
                                            nullptr, &shader_module);
    
    if (result != VK_SUCCESS) {
        std::cerr << "Failed to create shader module: " << result << std::endl;
        return VK_NULL_HANDLE;
    }
    
    return shader_module;
}

bool VulkanComputePipeline::create_descriptor_set_layout() {
    // Create bindings for storage buffers
    // We support up to MAX_BINDINGS storage buffers
    std::vector<VkDescriptorSetLayoutBinding> bindings(MAX_BINDINGS);
    
    for (uint32_t i = 0; i < MAX_BINDINGS; i++) {
        bindings[i].binding = i;
        bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[i].descriptorCount = 1;
        bindings[i].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        bindings[i].pImmutableSamplers = nullptr;
    }
    
    VkDescriptorSetLayoutCreateInfo layout_info = {};
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_info.bindingCount = MAX_BINDINGS;
    layout_info.pBindings = bindings.data();
    
    VkResult result = vkCreateDescriptorSetLayout(context_->device(), &layout_info,
                                                    nullptr, &descriptor_set_layout_);
    
    if (result != VK_SUCCESS) {
        std::cerr << "Failed to create descriptor set layout: " << result << std::endl;
        return false;
    }
    
    return true;
}

bool VulkanComputePipeline::create_pipeline_layout(size_t push_constant_size) {
    VkPipelineLayoutCreateInfo pipeline_layout_info = {};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = 1;
    pipeline_layout_info.pSetLayouts = &descriptor_set_layout_;
    
    // Add push constant range if needed
    VkPushConstantRange push_constant_range = {};
    if (push_constant_size > 0) {
        push_constant_range.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        push_constant_range.offset = 0;
        push_constant_range.size = static_cast<uint32_t>(push_constant_size);
        
        pipeline_layout_info.pushConstantRangeCount = 1;
        pipeline_layout_info.pPushConstantRanges = &push_constant_range;
    }
    
    VkResult result = vkCreatePipelineLayout(context_->device(), &pipeline_layout_info,
                                              nullptr, &pipeline_layout_);
    
    if (result != VK_SUCCESS) {
        std::cerr << "Failed to create pipeline layout: " << result << std::endl;
        return false;
    }
    
    return true;
}

bool VulkanComputePipeline::create_compute_pipeline() {
    VkPipelineShaderStageCreateInfo shader_stage_info = {};
    shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stage_info.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    shader_stage_info.module = shader_module_;
    shader_stage_info.pName = "main";
    
    VkComputePipelineCreateInfo pipeline_info = {};
    pipeline_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipeline_info.stage = shader_stage_info;
    pipeline_info.layout = pipeline_layout_;
    
    VkResult result = vkCreateComputePipelines(context_->device(), VK_NULL_HANDLE,
                                                 1, &pipeline_info, nullptr, &pipeline_);
    
    if (result != VK_SUCCESS) {
        std::cerr << "Failed to create compute pipeline: " << result << std::endl;
        return false;
    }
    
    return true;
}

bool VulkanComputePipeline::create_descriptor_pool() {
    VkDescriptorPoolSize pool_size = {};
    pool_size.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    pool_size.descriptorCount = MAX_BINDINGS;
    
    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.poolSizeCount = 1;
    pool_info.pPoolSizes = &pool_size;
    pool_info.maxSets = 1;
    
    VkResult result = vkCreateDescriptorPool(context_->device(), &pool_info,
                                              nullptr, &descriptor_pool_);
    
    if (result != VK_SUCCESS) {
        std::cerr << "Failed to create descriptor pool: " << result << std::endl;
        return false;
    }
    
    return true;
}

bool VulkanComputePipeline::allocate_descriptor_sets() {
    VkDescriptorSetAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool = descriptor_pool_;
    alloc_info.descriptorSetCount = 1;
    alloc_info.pSetLayouts = &descriptor_set_layout_;
    
    VkResult result = vkAllocateDescriptorSets(context_->device(), &alloc_info,
                                                &descriptor_set_);
    
    if (result != VK_SUCCESS) {
        std::cerr << "Failed to allocate descriptor sets: " << result << std::endl;
        return false;
    }
    
    return true;
}

void VulkanComputePipeline::bind_buffer(uint32_t binding, const VulkanBuffer& buffer) {
    if (binding >= MAX_BINDINGS) {
        throw std::runtime_error("Binding index out of range");
    }
    
    buffer_bindings_[binding] = buffer.buffer();
    buffer_sizes_[binding] = buffer.size();
    descriptors_dirty_ = true;
}

void VulkanComputePipeline::set_push_constants(const void* data, size_t size, size_t offset) {
    if (offset + size > push_constant_size_) {
        throw std::runtime_error("Push constant size exceeds allocated size");
    }
    
    std::memcpy(push_constant_data_.data() + offset, data, size);
}

void VulkanComputePipeline::update_descriptor_sets() {
    if (!descriptors_dirty_) {
        return;
    }
    
    std::vector<VkWriteDescriptorSet> descriptor_writes;
    std::vector<VkDescriptorBufferInfo> buffer_infos;
    
    descriptor_writes.reserve(buffer_bindings_.size());
    buffer_infos.reserve(buffer_bindings_.size());
    
    for (const auto& [binding, buffer] : buffer_bindings_) {
        VkDescriptorBufferInfo buffer_info = {};
        buffer_info.buffer = buffer;
        buffer_info.offset = 0;
        buffer_info.range = buffer_sizes_[binding];
        buffer_infos.push_back(buffer_info);
        
        VkWriteDescriptorSet descriptor_write = {};
        descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptor_write.dstSet = descriptor_set_;
        descriptor_write.dstBinding = binding;
        descriptor_write.dstArrayElement = 0;
        descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptor_write.descriptorCount = 1;
        descriptor_write.pBufferInfo = &buffer_infos[descriptor_writes.size()];
        
        descriptor_writes.push_back(descriptor_write);
    }
    
    vkUpdateDescriptorSets(context_->device(),
                            static_cast<uint32_t>(descriptor_writes.size()),
                            descriptor_writes.data(), 0, nullptr);
    
    descriptors_dirty_ = false;
}

void VulkanComputePipeline::dispatch(uint32_t group_x, uint32_t group_y, uint32_t group_z) {
    // Wait for previous dispatch to complete
    wait();
    
    // Update descriptor sets if needed
    update_descriptor_sets();
    
    // Reset fence
    context_->reset_fence(fence_);
    
    // Begin command buffer
    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    
    VkResult begin_result = vkBeginCommandBuffer(command_buffer_, &begin_info);
    if (begin_result != VK_SUCCESS) {
        throw std::runtime_error(
            "VulkanComputePipeline::dispatch: vkBeginCommandBuffer failed ("
            + std::to_string(static_cast<int>(begin_result)) + ")");
    }

    // Bind pipeline
    vkCmdBindPipeline(command_buffer_, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_);
    
    // Bind descriptor sets
    vkCmdBindDescriptorSets(command_buffer_, VK_PIPELINE_BIND_POINT_COMPUTE,
                             pipeline_layout_, 0, 1, &descriptor_set_, 0, nullptr);
    
    // Set push constants if needed
    if (push_constant_size_ > 0) {
        vkCmdPushConstants(command_buffer_, pipeline_layout_,
                            VK_SHADER_STAGE_COMPUTE_BIT, 0,
                            static_cast<uint32_t>(push_constant_size_),
                            push_constant_data_.data());
    }
    
    // Dispatch
    vkCmdDispatch(command_buffer_, group_x, group_y, group_z);

    VkResult end_result = vkEndCommandBuffer(command_buffer_);
    if (end_result != VK_SUCCESS) {
        throw std::runtime_error(
            "VulkanComputePipeline::dispatch: vkEndCommandBuffer failed ("
            + std::to_string(static_cast<int>(end_result)) + ")");
    }

    // Submit command buffer
    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer_;

    VkResult submit_result = vkQueueSubmit(context_->compute_queue(), 1, &submit_info, fence_);
    if (submit_result != VK_SUCCESS) {
        throw std::runtime_error(
            "VulkanComputePipeline::dispatch: vkQueueSubmit failed ("
            + std::to_string(static_cast<int>(submit_result)) + ")");
    }
}

bool VulkanComputePipeline::wait(uint64_t timeout_ns) {
    if (fence_ == VK_NULL_HANDLE) {
        return false;
    }
    return context_->wait_for_fence(fence_, timeout_ns);
}

} // namespace vulkan
} // namespace lora
} // namespace themis

#endif // THEMIS_HAS_VULKAN_HEADER
