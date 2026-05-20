/*
 * ThemisDB | File: hsm_provider_global.cpp | Version: 0.0.15
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#include "security/hsm_provider.h"

#include <memory>

std::shared_ptr<themis::security::HSMProvider> g_hsm_provider;
