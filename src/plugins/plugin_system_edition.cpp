/**
 * @file plugin_system_edition.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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
