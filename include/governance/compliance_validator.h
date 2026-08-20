/**
 * @file compliance_validator.h
 * @brief Compliance validation engine for ThemisDB governance module
 * @version 1.0.0
 * @date 2026-08-18
 * 
 * @details
 * Implements automated compliance validation logic with:
 * - Policy state checking against requirements
 * - Control implementation verification
 * - Evidence collection and linking
 * - Violation detection and reporting
 * - Performance optimization with caching
 * 
 * @note Performance targets:
 *       - Single check ≤1s
 *       - Report generation ≤5s
 *       - Cache hit latency ≤10ms
 */

#pragma once

#include "governance/compliance_framework.h"
#include "governance/policy_manager.h"
#include <memory>
#include <chrono>
#include <unordered_map>
#include <mutex>

namespace themis {
namespace governance {

// ============================================================================
// Compliance Validator Interface
// ============================================================================

/**
 * @brief Base interface for framework-specific validators
 * 
 * Each compliance framework has a validator that implements
 * requirement checking and control verification.
 */
class IComplianceValidator {
public:
    virtual ~IComplianceValidator() = default;
    
    /// Get the framework this validator handles
    virtual ComplianceFramework getFramework() const = 0;
    
    /// Validate a single requirement
    virtual ComplianceStatus validateRequirement(
        const ComplianceRequirement& req,
        const ComplianceContext& ctx) = 0;
    
    /// Validate a single control
    virtual ComplianceStatus validateControl(
        const ComplianceControl& ctl,
        const ComplianceContext& ctx) = 0;
    
    /// Collect evidence for a control
    virtual std::vector<ComplianceEvidence> collectEvidence(
        const ComplianceControl& ctl,
        const ComplianceContext& ctx) = 0;
    
    /// Check if system policies satisfy requirement
    virtual bool checkPolicySatisfaction(
        const ComplianceRequirement& req,
        const ComplianceContext& ctx) = 0;
};

// ============================================================================
// Framework-Specific Validators
// ============================================================================

/**
 * @brief ISO 27001 compliance validator
 */
class Iso27001Validator : public IComplianceValidator {
public:
    ComplianceFramework getFramework() const override;
    
    ComplianceStatus validateRequirement(
        const ComplianceRequirement& req,
        const ComplianceContext& ctx) override;
    
    ComplianceStatus validateControl(
        const ComplianceControl& ctl,
        const ComplianceContext& ctx) override;
    
    std::vector<ComplianceEvidence> collectEvidence(
        const ComplianceControl& ctl,
        const ComplianceContext& ctx) override;
    
    bool checkPolicySatisfaction(
        const ComplianceRequirement& req,
        const ComplianceContext& ctx) override;

private:
    bool validateEncryption(const ComplianceContext& ctx);
    bool validateAccessControl(const ComplianceContext& ctx);
    bool validateAuditLogging(const ComplianceContext& ctx);
    bool validateIncidentResponse(const ComplianceContext& ctx);
    bool validateBackupRecovery(const ComplianceContext& ctx);
};

/**
 * @brief SOC 2 compliance validator
 */
class Soc2Validator : public IComplianceValidator {
public:
    ComplianceFramework getFramework() const override;
    
    ComplianceStatus validateRequirement(
        const ComplianceRequirement& req,
        const ComplianceContext& ctx) override;
    
    ComplianceStatus validateControl(
        const ComplianceControl& ctl,
        const ComplianceContext& ctx) override;
    
    std::vector<ComplianceEvidence> collectEvidence(
        const ComplianceControl& ctl,
        const ComplianceContext& ctx) override;
    
    bool checkPolicySatisfaction(
        const ComplianceRequirement& req,
        const ComplianceContext& ctx) override;

private:
    bool validateSecurityMonitoring(const ComplianceContext& ctx);
    bool validateChangeManagement(const ComplianceContext& ctx);
    bool validateDataSecurity(const ComplianceContext& ctx);
    bool validateAvailability(const ComplianceContext& ctx);
    bool validateConfidentiality(const ComplianceContext& ctx);
};

/**
 * @brief GDPR compliance validator
 */
class GdprValidator : public IComplianceValidator {
public:
    ComplianceFramework getFramework() const override;
    
    ComplianceStatus validateRequirement(
        const ComplianceRequirement& req,
        const ComplianceContext& ctx) override;
    
    ComplianceStatus validateControl(
        const ComplianceControl& ctl,
        const ComplianceContext& ctx) override;
    
    std::vector<ComplianceEvidence> collectEvidence(
        const ComplianceControl& ctl,
        const ComplianceContext& ctx) override;
    
    bool checkPolicySatisfaction(
        const ComplianceRequirement& req,
        const ComplianceContext& ctx) override;

private:
    bool validateConsentManagement(const ComplianceContext& ctx);
    bool validateDataMinimization(const ComplianceContext& ctx);
    bool validateSubjectRights(const ComplianceContext& ctx);
    bool validateDataRetention(const ComplianceContext& ctx);
    bool validateDpia(const ComplianceContext& ctx);
};

/**
 * @brief CCPA compliance validator
 */
class CcpaValidator : public IComplianceValidator {
public:
    ComplianceFramework getFramework() const override;
    
    ComplianceStatus validateRequirement(
        const ComplianceRequirement& req,
        const ComplianceContext& ctx) override;
    
    ComplianceStatus validateControl(
        const ComplianceControl& ctl,
        const ComplianceContext& ctx) override;
    
    std::vector<ComplianceEvidence> collectEvidence(
        const ComplianceControl& ctl,
        const ComplianceContext& ctx) override;
    
    bool checkPolicySatisfaction(
        const ComplianceRequirement& req,
        const ComplianceContext& ctx) override;

private:
    bool validateConsumerRights(const ComplianceContext& ctx);
    bool validateDataSaleOpt(const ComplianceContext& ctx);
    bool validatePrivacyPolicy(const ComplianceContext& ctx);
    bool validateDataSecurity(const ComplianceContext& ctx);
};

/**
 * @brief HIPAA compliance validator
 */
class HipaaValidator : public IComplianceValidator {
public:
    ComplianceFramework getFramework() const override;
    
    ComplianceStatus validateRequirement(
        const ComplianceRequirement& req,
        const ComplianceContext& ctx) override;
    
    ComplianceStatus validateControl(
        const ComplianceControl& ctl,
        const ComplianceContext& ctx) override;
    
    std::vector<ComplianceEvidence> collectEvidence(
        const ComplianceControl& ctl,
        const ComplianceContext& ctx) override;
    
    bool checkPolicySatisfaction(
        const ComplianceRequirement& req,
        const ComplianceContext& ctx) override;

private:
    bool validatePhiEncryption(const ComplianceContext& ctx);
    bool validateAccessControl(const ComplianceContext& ctx);
    bool validateAuditControls(const ComplianceContext& ctx);
    bool validateIntegrityControls(const ComplianceContext& ctx);
};

/**
 * @brief PCI-DSS compliance validator
 */
class PciDssValidator : public IComplianceValidator {
public:
    ComplianceFramework getFramework() const override;
    
    ComplianceStatus validateRequirement(
        const ComplianceRequirement& req,
        const ComplianceContext& ctx) override;
    
    ComplianceStatus validateControl(
        const ComplianceControl& ctl,
        const ComplianceContext& ctx) override;
    
    std::vector<ComplianceEvidence> collectEvidence(
        const ComplianceControl& ctl,
        const ComplianceContext& ctx) override;
    
    bool checkPolicySatisfaction(
        const ComplianceRequirement& req,
        const ComplianceContext& ctx) override;

private:
    bool validateCardDataEncryption(const ComplianceContext& ctx);
    bool validateAccessControl(const ComplianceContext& ctx);
    bool validateVulnerabilityManagement(const ComplianceContext& ctx);
    bool validateMonitoring(const ComplianceContext& ctx);
};

/**
 * @brief EU AI Act compliance validator
 */
class EuAiActValidator : public IComplianceValidator {
public:
    ComplianceFramework getFramework() const override;
    
    ComplianceStatus validateRequirement(
        const ComplianceRequirement& req,
        const ComplianceContext& ctx) override;
    
    ComplianceStatus validateControl(
        const ComplianceControl& ctl,
        const ComplianceContext& ctx) override;
    
    std::vector<ComplianceEvidence> collectEvidence(
        const ComplianceControl& ctl,
        const ComplianceContext& ctx) override;
    
    bool checkPolicySatisfaction(
        const ComplianceRequirement& req,
        const ComplianceContext& ctx) override;

private:
    bool validateRiskManagement(const ComplianceContext& ctx);
    bool validateTransparency(const ComplianceContext& ctx);
    bool validateMonitoring(const ComplianceContext& ctx);
    bool validateHumanOversight(const ComplianceContext& ctx);
};

// ============================================================================
// Main Compliance Validation Engine
// ============================================================================

/**
 * @brief Orchestrates compliance validation across frameworks
 * 
 * Manages validators, coordinates multi-framework validation,
 * aggregates results, and generates compliance reports.
 */
class ComplianceValidationEngine {
public:
    ComplianceValidationEngine();
    ~ComplianceValidationEngine() = default;
    
    /// Register a validator for a framework
    void registerValidator(std::unique_ptr<IComplianceValidator> validator);
    
    /// Validate single requirement
    ComplianceStatus validateRequirement(
        const ComplianceRequirement& req,
        const ComplianceContext& ctx);
    
    /// Validate single control
    ComplianceStatus validateControl(
        const ComplianceControl& ctl,
        const ComplianceContext& ctx);
    
    /// Perform comprehensive validation for framework
    ComplianceStatusReport validateFramework(
        ComplianceFramework fw,
        const ComplianceFrameworkRegistry& registry,
        const ComplianceContext& ctx);

    /// Validate all enabled frameworks using a shared registry pointer
    ComplianceValidationResult validateAll(
        const std::vector<ComplianceFramework>& frameworks,
        const std::shared_ptr<const ComplianceFrameworkRegistry>& registry,
        const ComplianceContext& ctx) {
        return validateAll(frameworks, *registry, ctx);
    }
    
    /// Validate all enabled frameworks
    ComplianceValidationResult validateAll(
        const std::vector<ComplianceFramework>& frameworks,
        const ComplianceFrameworkRegistry& registry,
        const ComplianceContext& ctx);
    
    /// Detect compliance violations
    std::vector<ComplianceViolation> detectViolations(
        const ComplianceValidationResult& result);
    
    /// Generate remediation guidance for violation
    std::string generateRemediationGuidance(
        const ComplianceViolation& violation);

private:
    std::unordered_map<int, std::unique_ptr<IComplianceValidator>> validators_;
    mutable std::mutex mu_;
    
    /// Get validator for framework
    IComplianceValidator* getValidator(ComplianceFramework fw);
};

} // namespace governance
} // namespace themis
