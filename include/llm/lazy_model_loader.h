#pragma once

// Unify LazyModelLoader by delegating to the full implementation.
// Tests and plugins should include this header, which forwards to model_loader.h.

#include "llm/model_loader.h"
