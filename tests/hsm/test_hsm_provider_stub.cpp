// RESTORED FROM HISTORY: b90459798fae3d66d09e2054bf8201c6da352332

#include "security/hsm_provider.h"
#include <memory>

#if defined(THEMIS_TEST_BUILD)
std::shared_ptr<themis::security::HSMProvider> g_hsm_provider;
#endif
