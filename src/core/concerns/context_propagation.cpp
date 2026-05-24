/*
 * ThemisDB | File: context_propagation.cpp | Version: 0.0.15
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0
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
