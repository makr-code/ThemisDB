/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            vulkan_backend.h                                   ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-02-22 11:29:18                                ║
  Author:          copilot-swe-agent[bot]                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     39                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • c97d719  2026-02-22  Add parallel multi-source BFS/DFS implementation (graph/p... ║
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
