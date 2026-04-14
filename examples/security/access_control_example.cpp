/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            access_control_example.cpp                         ║
  Version:         0.0.40                                             ║
  Last Modified:   2026-04-14 06:49:30                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     232                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file access_control_example.cpp
 * @brief Example demonstrating the Access Control Framework
 * 
 * This example shows how to:
 * 1. Initialize the access control manager
 * 2. Configure authentication middleware
 * 3. Perform access control checks
 * 4. Manage user roles
 * 5. Use custom authorization logic
 */

#include "security/access_control_manager.h"
#include "server/auth_middleware.h"
#include "utils/logger.h"
#include <iostream>

using namespace themis;
using namespace themis::security;

int main() {
    try {
        // ====================================================================
        // Step 1: Configure Access Control
        // ====================================================================
        
        AccessControlConfig config;
        config.rbac_config_path = "config/rbac_roles.json";
        config.user_role_store_path = "config/user_roles.json";
        config.enable_audit_logging = true;
        config.fail_closed = true;  // Deny access on errors (secure default)
        
        // Optional: Add custom authorization hook
        config.custom_authorizer = [](
            const SecurityContext& ctx,
            const std::string& resource,
            const std::string& action
        ) -> AccessDecision {
            // Example: Allow read-only access to public data
            if (resource == "public" && action == "read") {
                return AccessDecision::Allow("Public data is accessible to all");
            }
            
            // Example: Restrict admin operations to specific IP range
            if (ctx.hasRole("admin") && resource == "admin") {
                if (ctx.source_ip.starts_with("192.168.1.")) {
                    return AccessDecision::Allow("Admin from trusted network");
                }
                return AccessDecision::Deny("Admin access only from trusted network");
            }
            
            // Fall through to RBAC
            return AccessDecision::Deny("");
        };
        
        // Create access control manager
        auto acm = std::make_shared<AccessControlManager>(config);
        
        // ====================================================================
        // Step 2: Configure Authentication
        // ====================================================================
        
        auto auth = std::make_shared<AuthMiddleware>();
        
        // Add API tokens (for testing - use JWT in production)
        AuthMiddleware::TokenConfig admin_token;
        admin_token.token = "admin-secret-token-123";
        admin_token.user_id = "admin@example.com";
        admin_token.scopes = {"admin", "data:write", "keys:rotate"};
        auth->addToken(admin_token);
        
        AuthMiddleware::TokenConfig user_token;
        user_token.token = "user-secret-token-456";
        user_token.user_id = "user@example.com";
        user_token.scopes = {"data:read"};
        auth->addToken(user_token);
        
        // Connect auth to access control
        acm->setAuthMiddleware(auth);
        
        // Initialize
        if (!acm->initialize()) {
            std::cerr << "Failed to initialize access control" << std::endl;
            return 1;
        }
        
        std::cout << "Access control initialized successfully\n" << std::endl;
        
        // ====================================================================
        // Step 3: Role Management
        // ====================================================================
        
        std::cout << "=== Role Management ===" << std::endl;
        
        // Assign roles to users
        acm->assignRole("admin@example.com", "admin");
        acm->assignRole("user@example.com", "readonly");
        acm->assignRole("developer@example.com", "developer");
        
        // Get user roles
        auto admin_roles = acm->getUserRoles("admin@example.com");
        std::cout << "Admin roles: ";
        for (const auto& role : admin_roles) {
            std::cout << role << " ";
        }
        std::cout << std::endl;
        
        // Get user permissions
        auto admin_perms = acm->getUserPermissions("admin@example.com");
        std::cout << "Admin permissions (" << admin_perms.size() << " total):" << std::endl;
        for (size_t i = 0; i < std::min(size_t(5), admin_perms.size()); i++) {
            std::cout << "  - " << admin_perms[i].toString() << std::endl;
        }
        std::cout << std::endl;
        
        // ====================================================================
        // Step 4: Access Control Checks
        // ====================================================================
        
        std::cout << "=== Access Control Checks ===" << std::endl;
        
        // Example 1: Check admin access
        std::cout << "\n1. Admin writing data:" << std::endl;
        auto decision = acm->checkAccess(
            "admin-secret-token-123",
            "data",
            "write",
            "192.168.1.100"
        );
        std::cout << "   Decision: " << (decision.granted ? "ALLOW" : "DENY") << std::endl;
        std::cout << "   Reason: " << decision.reason << std::endl;
        
        // Example 2: Check user write access (should be denied)
        std::cout << "\n2. User writing data:" << std::endl;
        decision = acm->checkAccess(
            "user-secret-token-456",
            "data",
            "write",
            "192.168.1.101"
        );
        std::cout << "   Decision: " << (decision.granted ? "ALLOW" : "DENY") << std::endl;
        std::cout << "   Reason: " << decision.reason << std::endl;
        
        // Example 3: Check user read access (should be allowed)
        std::cout << "\n3. User reading data:" << std::endl;
        decision = acm->checkAccess(
            "user-secret-token-456",
            "data",
            "read",
            "192.168.1.101"
        );
        std::cout << "   Decision: " << (decision.granted ? "ALLOW" : "DENY") << std::endl;
        std::cout << "   Reason: " << decision.reason << std::endl;
        
        // Example 4: Check key rotation (admin only)
        std::cout << "\n4. Admin rotating keys:" << std::endl;
        decision = acm->checkAccess(
            "admin-secret-token-123",
            "keys",
            "rotate",
            "192.168.1.100"
        );
        std::cout << "   Decision: " << (decision.granted ? "ALLOW" : "DENY") << std::endl;
        std::cout << "   Reason: " << decision.reason << std::endl;
        
        // Example 5: Custom authorizer - public data access
        std::cout << "\n5. Anonymous reading public data:" << std::endl;
        SecurityContext anon_ctx;
        anon_ctx.user_id = "anonymous";
        anon_ctx.roles = {};
        decision = acm->authorize(anon_ctx, "public", "read");
        std::cout << "   Decision: " << (decision.granted ? "ALLOW" : "DENY") << std::endl;
        std::cout << "   Reason: " << decision.reason << std::endl;
        
        // ====================================================================
        // Step 5: Metrics
        // ====================================================================
        
        std::cout << "\n=== Metrics ===" << std::endl;
        const auto& metrics = acm->getMetrics();
        std::cout << "Authentication success: " << metrics.authentication_success << std::endl;
        std::cout << "Authentication failure: " << metrics.authentication_failure << std::endl;
        std::cout << "Authorization success: " << metrics.authorization_success << std::endl;
        std::cout << "Access denied: " << metrics.access_denied << std::endl;
        
        // ====================================================================
        // Step 6: Configuration Management
        // ====================================================================
        
        std::cout << "\n=== Configuration Management ===" << std::endl;
        
        // Save current configuration
        if (acm->saveConfiguration()) {
            std::cout << "Configuration saved successfully" << std::endl;
        }
        
        // In a real application, you might reload after external changes:
        // if (acm->reloadConfiguration()) {
        //     std::cout << "Configuration reloaded successfully" << std::endl;
        // }
        
        std::cout << "\nExample completed successfully!" << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
