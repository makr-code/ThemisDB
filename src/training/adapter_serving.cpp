/*
 * ThemisDB | File: adapter_serving.cpp | Version: 0.0.10 | Last Modified: 2026-05-20 17:27:23
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 27
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=0, L=0
 * PR History (last 5): #5082 [Docs][training] Update mod... (2026-05-13) | #4519 [WIP] Update developer docu... (2026-04-12)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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
