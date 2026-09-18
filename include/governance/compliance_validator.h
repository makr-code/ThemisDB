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

class IComplianceValidator {
public:
    /**
     * @brief ICompliance Validator.
     * @return Return value.
     */
    virtual ~IComplianceValidator() = default;
    
    /**
     * @brief Get Framework.
     * @return Return value.
     */
    virtual ComplianceFramework getFramework() const = 0;
    
    /**
     * @brief Validate Requirement.
     * @param[in] req Input parameter.
     * @param[in] ctx Input parameter.
     * @return Return value.
     */
    virtual ComplianceStatus validateRequirement(
        const ComplianceRequirement& req,
        const ComplianceContext& ctx) = 0;
    
    /**
     * @brief Validate Control.
     * @param[in] ctl Input parameter.
     * @param[in] ctx Input parameter.
     * @return Return value.
     */
    virtual ComplianceStatus validateControl(
        const ComplianceControl& ctl,
        const ComplianceContext& ctx) = 0;
    
    /**
     * @brief Collect Evidence.
     * @param[in] ctl Input parameter.
     * @param[in] ctx Input parameter.
     * @return Return value.
     */
    virtual std::vector<ComplianceEvidence> collectEvidence(
        const ComplianceControl& ctl,
        const ComplianceContext& ctx) = 0;
    
    /**
     * @brief Check Policy Satisfaction.
     * @param[in] req Input parameter.
     * @param[in] ctx Input parameter.
     * @return True when the operation succeeds.
     */
    virtual bool checkPolicySatisfaction(
        const ComplianceRequirement& req,
        const ComplianceContext& ctx) = 0;
};

// ============================================================================
// Framework-Specific Validators
// ============================================================================

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
    /**
     * @brief Validate Encryption.
     * @param[in] ctx Input parameter.
     * @return True when the operation succeeds.
     */
    bool validateEncryption(const ComplianceContext& ctx);
    /**
     * @brief Validate Access Control.
     * @param[in] ctx Input parameter.
     * @return True when the operation succeeds.
     */
    bool validateAccessControl(const ComplianceContext& ctx);
    /**
     * @brief Validate Audit Logging.
     * @param[in] ctx Input parameter.
     * @return True when the operation succeeds.
     */
    bool validateAuditLogging(const ComplianceContext& ctx);
    /**
     * @brief Validate Incident Response.
     * @param[in] ctx Input parameter.
     * @return True when the operation succeeds.
     */
    bool validateIncidentResponse(const ComplianceContext& ctx);
    /**
     * @brief Validate Backup Recovery.
     * @param[in] ctx Input parameter.
     * @return True when the operation succeeds.
     */
    bool validateBackupRecovery(const ComplianceContext& ctx);
};

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
    /**
     * @brief Validate Security Monitoring.
     * @param[in] ctx Input parameter.
     * @return True when the operation succeeds.
     */
    bool validateSecurityMonitoring(const ComplianceContext& ctx);
    /**
     * @brief Validate Change Management.
     * @param[in] ctx Input parameter.
     * @return True when the operation succeeds.
     */
    bool validateChangeManagement(const ComplianceContext& ctx);
    /**
     * @brief Validate Data Security.
     * @param[in] ctx Input parameter.
     * @return True when the operation succeeds.
     */
    bool validateDataSecurity(const ComplianceContext& ctx);
    /**
     * @brief Validate Availability.
     * @param[in] ctx Input parameter.
     * @return True when the operation succeeds.
     */
    bool validateAvailability(const ComplianceContext& ctx);
    /**
     * @brief Validate Confidentiality.
     * @param[in] ctx Input parameter.
     * @return True when the operation succeeds.
     */
    bool validateConfidentiality(const ComplianceContext& ctx);
};

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
    /**
     * @brief Validate Consent Management.
     * @param[in] ctx Input parameter.
     * @return True when the operation succeeds.
     */
    bool validateConsentManagement(const ComplianceContext& ctx);
    /**
     * @brief Validate Data Minimization.
     * @param[in] ctx Input parameter.
     * @return True when the operation succeeds.
     */
    bool validateDataMinimization(const ComplianceContext& ctx);
    /**
     * @brief Validate Subject Rights.
     * @param[in] ctx Input parameter.
     * @return True when the operation succeeds.
     */
    bool validateSubjectRights(const ComplianceContext& ctx);
    /**
     * @brief Validate Data Retention.
     * @param[in] ctx Input parameter.
     * @return True when the operation succeeds.
     */
    bool validateDataRetention(const ComplianceContext& ctx);
    /**
     * @brief Validate Dpia.
     * @param[in] ctx Input parameter.
     * @return True when the operation succeeds.
     */
    bool validateDpia(const ComplianceContext& ctx);
};

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
    /**
     * @brief Validate Consumer Rights.
     * @param[in] ctx Input parameter.
     * @return True when the operation succeeds.
     */
    bool validateConsumerRights(const ComplianceContext& ctx);
    /**
     * @brief Validate Data Sale Opt.
     * @param[in] ctx Input parameter.
     * @return True when the operation succeeds.
     */
    bool validateDataSaleOpt(const ComplianceContext& ctx);
    /**
     * @brief Validate Privacy Policy.
     * @param[in] ctx Input parameter.
     * @return True when the operation succeeds.
     */
    bool validatePrivacyPolicy(const ComplianceContext& ctx);
    /**
     * @brief Validate Data Security.
     * @param[in] ctx Input parameter.
     * @return True when the operation succeeds.
     */
    bool validateDataSecurity(const ComplianceContext& ctx);
};

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
    /**
     * @brief Validate Phi Encryption.
     * @param[in] ctx Input parameter.
     * @return True when the operation succeeds.
     */
    bool validatePhiEncryption(const ComplianceContext& ctx);
    /**
     * @brief Validate Access Control.
     * @param[in] ctx Input parameter.
     * @return True when the operation succeeds.
     */
    bool validateAccessControl(const ComplianceContext& ctx);
    /**
     * @brief Validate Audit Controls.
     * @param[in] ctx Input parameter.
     * @return True when the operation succeeds.
     */
    bool validateAuditControls(const ComplianceContext& ctx);
    /**
     * @brief Validate Integrity Controls.
     * @param[in] ctx Input parameter.
     * @return True when the operation succeeds.
     */
    bool validateIntegrityControls(const ComplianceContext& ctx);
};

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
    /**
     * @brief Validate Card Data Encryption.
     * @param[in] ctx Input parameter.
     * @return True when the operation succeeds.
     */
    bool validateCardDataEncryption(const ComplianceContext& ctx);
    /**
     * @brief Validate Access Control.
     * @param[in] ctx Input parameter.
     * @return True when the operation succeeds.
     */
    bool validateAccessControl(const ComplianceContext& ctx);
    /**
     * @brief Validate Vulnerability Management.
     * @param[in] ctx Input parameter.
     * @return True when the operation succeeds.
     */
    bool validateVulnerabilityManagement(const ComplianceContext& ctx);
    /**
     * @brief Validate Monitoring.
     * @param[in] ctx Input parameter.
     * @return True when the operation succeeds.
     */
    bool validateMonitoring(const ComplianceContext& ctx);
};

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
    /**
     * @brief Validate Risk Management.
     * @param[in] ctx Input parameter.
     * @return True when the operation succeeds.
     */
    bool validateRiskManagement(const ComplianceContext& ctx);
    /**
     * @brief Validate Transparency.
     * @param[in] ctx Input parameter.
     * @return True when the operation succeeds.
     */
    bool validateTransparency(const ComplianceContext& ctx);
    /**
     * @brief Validate Monitoring.
     * @param[in] ctx Input parameter.
     * @return True when the operation succeeds.
     */
    bool validateMonitoring(const ComplianceContext& ctx);
    /**
     * @brief Validate Human Oversight.
     * @param[in] ctx Input parameter.
     * @return True when the operation succeeds.
     */
    bool validateHumanOversight(const ComplianceContext& ctx);
};

// ============================================================================
// Main Compliance Validation Engine
// ============================================================================

class ComplianceValidationEngine {
public:
    ComplianceValidationEngine();
    ~ComplianceValidationEngine() = default;
    
    /**
     * @brief Register Validator.
     * @param[in] validator Input parameter.
     */
    void registerValidator(std::unique_ptr<IComplianceValidator> validator);
    
    /**
     * @brief Validate Requirement.
     * @param[in] req Input parameter.
     * @param[in] ctx Input parameter.
     * @return Return value.
     */
    ComplianceStatus validateRequirement(
        const ComplianceRequirement& req,
        const ComplianceContext& ctx);
    
    /**
     * @brief Validate Control.
     * @param[in] ctl Input parameter.
     * @param[in] ctx Input parameter.
     * @return Return value.
     */
    ComplianceStatus validateControl(
        const ComplianceControl& ctl,
        const ComplianceContext& ctx);
    
    /**
     * @brief Validate Framework.
     * @param[in] fw Input parameter.
     * @param[in] registry Input parameter.
     * @param[in] ctx Input parameter.
     * @return Return value.
     */
    ComplianceStatusReport validateFramework(
        ComplianceFramework fw,
        const ComplianceFrameworkRegistry& registry,
        const ComplianceContext& ctx);

    /**
     * @brief Validate All.
     * @param[in] frameworks Input parameter.
     * @param[in] registry Input parameter.
     * @param[in] ctx Input parameter.
     * @return Return value.
     * @details Implements validateAll without additional internal calls.
     */
    ComplianceValidationResult validateAll(
        const std::vector<ComplianceFramework>& frameworks,
        const std::shared_ptr<const ComplianceFrameworkRegistry>& registry,
        const ComplianceContext& ctx) {
        return validateAll(frameworks, *registry, ctx);
    }
    
    /**
     * @brief Validate All.
     * @param[in] frameworks Input parameter.
     * @param[in] registry Input parameter.
     * @param[in] ctx Input parameter.
     * @return Return value.
     */
    ComplianceValidationResult validateAll(
        const std::vector<ComplianceFramework>& frameworks,
        const ComplianceFrameworkRegistry& registry,
        const ComplianceContext& ctx);
    
    /**
     * @brief Detect Violations.
     * @param[in] result Input parameter.
     * @return Return value.
     */
    std::vector<ComplianceViolation> detectViolations(
        const ComplianceValidationResult& result);
    
    /**
     * @brief Generate Remediation Guidance.
     * @param[in] violation Input parameter.
     * @return Return value.
     */
    std::string generateRemediationGuidance(
        const ComplianceViolation& violation);

private:
    std::unordered_map<int, std::unique_ptr<IComplianceValidator>> validators_;
    mutable std::mutex mu_;
    
    /**
     * @brief Get Validator.
     * @param[in] fw Input parameter.
     * @return Pointer to the result.
     */
    IComplianceValidator* getValidator(ComplianceFramework fw);
};

} // namespace governance
} // namespace themis
