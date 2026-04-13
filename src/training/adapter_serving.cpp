/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            adapter_serving.cpp                                ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-04-13 20:37:20                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     40                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • ac63c2ec8d  2026-04-12  [WIP] Update developer documentation for module training ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#include "training/adapter_serving.h"

// This translation unit anchors the out-of-line members for ILLMRouter.
// All logic lives in the header / in IncrementalLoRATrainer::Impl.

namespace themis {
namespace training {

// Provide an out-of-line body so that the vtable is emitted exactly once.
// The destructor body is intentionally empty; derived classes clean up their
// own state.
ILLMRouter::~ILLMRouter() {}

} // namespace training
} // namespace themis
