/*
 * ThemisDB | File: plugin_system_edition.cpp | Version: 0.0.1 | Last Modified: 2026-05-20 17:15:12
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 24
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=0, L=0
 * PR History (last 5): #4256 feat(plugins): upgrade Plug... (2026-03-15) | #3634 feat(plugins): build system... (2026-03-12) | #1292 Plugin system production-re... (2026-03-11)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

/*
 * DEPRECATED - Merged into plugin_manager.cpp / plugin_manager.h
 * ==============================================================
 * The edition/license gating and utility functions previously defined here
 * have been consolidated into PluginManager:
 *
 *   PluginManager::isEditionSupported()           -- compile-time edition gate
 *   PluginManager::isLicensed()                   -- runtime license gate
 *   PluginManager::communityUnavailableMessage()  -- user-facing error text
 *   PluginManager::marketplaceInfo()              -- marketplace availability
 *   PluginManager::installationInstructions()     -- install guide
 *
 * The edition gate is now automatically applied inside loadPlugin().
 * This file is intentionally excluded from cmake/CMakeLists.txt.
 */
