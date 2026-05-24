/*
 * ThemisDB | File: adapter_serving.cpp | Version: 0.0.10 | Last Modified: 2026-05-20 17:13:04
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 26
 * Open Issues: TODOs=1, Stubs=1, Gaps=3, Unimpl=0, Mock=1, Sim=0, Debt=0
 * Gap Correlation: internal=3 | external_v3=3 | delta=0 | status=aligned
 * External Severity (v3): C=0, H=2, M=1
 * PR: #5082 [Docs][training] Update module docs across src/include with API, ru... (2026-05-13T11:01:23Z)
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
