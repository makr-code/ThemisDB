#pragma once

// VulkanBackend — public alias for the Vulkan compute shader pipeline backend.
// Tests and callers use `VulkanBackend`; the class is implemented as
// `VulkanVectorBackend` inside graphics_backends.h / graphics_backends.cpp.

#include "acceleration/graphics_backends.h"

namespace themis {
namespace acceleration {

// Alias so that `#include "acceleration/vulkan_backend.h"` gives a
// `VulkanBackend` type that is identical to `VulkanVectorBackend`.
using VulkanBackend = VulkanVectorBackend;

} // namespace acceleration
} // namespace themis
