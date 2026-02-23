#include "core/concerns/context_propagation.h"

namespace themis {
namespace core {
namespace concerns {

thread_local IContextPtr ContextPropagation::current_{nullptr};

} // namespace concerns
} // namespace core
} // namespace themis
