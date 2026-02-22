/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            vulkan_backend.h                                   ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-02-22 08:12:05                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     17                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2f0d63150  2026-02-21  feat(acceleration): implement Vulkan compute shader pipel... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

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
