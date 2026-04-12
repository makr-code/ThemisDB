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
