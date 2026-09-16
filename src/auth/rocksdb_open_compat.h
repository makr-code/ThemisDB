#pragma once

#include "utils/rocksdb_open_compat.h"

namespace themis::auth::detail {

using themis::storage::detail::openDbForReadOnlyCompat;
using themis::storage::detail::openDbWithColumnFamiliesCompat;

} // namespace themis::auth::detail
