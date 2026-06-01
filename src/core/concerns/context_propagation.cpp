/*
 * ThemisDB | File: context_propagation.cpp | Version: 0.0.15 | Last Modified: 2026-05-31 11:10:47
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 20
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): #2678 feat(core): context propaga... (2026-03-12)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#include "core/concerns/context_propagation.h"

namespace themis {
namespace core {
namespace concerns {

thread_local IContextPtr ContextPropagation::current_{nullptr};

} // namespace concerns
} // namespace core
} // namespace themis
