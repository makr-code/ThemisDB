/**
 * @file timestamp.h
 * @brief Compatibility shim exposing HLCTimestamp as `core::HLCTimestamp`.
 *
 * Historically some components included "core/timestamp.h" and referenced
 * `core::HLCTimestamp`. The canonical definition is `themis::HLCTimestamp`
 * in `include/storage/hlc.h`. This header provides a lightweight alias to
 * preserve existing include paths without moving the original type.
 */

#pragma once

#include "storage/hlc.h"

namespace core {
using HLCTimestamp = ::themis::HLCTimestamp;
}
