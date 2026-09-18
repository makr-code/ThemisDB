/**
 * @file compliance_mapping.h
 * @brief Regulatory requirement to technical control mapping
 * @version 1.0.0
 * @date 2026-08-18
 * 
 * @details
 * Maintains authoritative mappings between regulatory requirements
 * and technical controls with:
 * - Framework definition loader
 * - Requirement/control versioning
 * - Cross-framework requirement linking
 * - Mapping validation and consistency checking
 * 
 * @note All mappings are based on official framework documents
 * @note Versioning supports framework updates (e.g., ISO 27001:2022)
 */

#pragma once

#include "governance/compliance_framework.h"
#include <map>
#include <memory>
#include <mutex>
#include <tuple>

namespace themis {
namespace governance {

// ============================================================================
// Compliance Framework Loader
// ============================================================================

class ComplianceFrameworkLoader {
public:
    /**
     * @brief Load All Frameworks.
     * @return Return value.
     */
    static std::shared_ptr<ComplianceFrameworkRegistry> loadAllFrameworks();
    
    /**
     * @brief Load Framework.
     * @param[in] fw Input parameter.
     * @return Return value.
     */
    static std::shared_ptr<ComplianceFrameworkRegistry> loadFramework(
        ComplianceFramework fw);
    
    /**
     * @brief Load From File.
     * @param[in] filepath Input parameter.
     * @return Return value.
     */
    static std::shared_ptr<ComplianceFrameworkRegistry> loadFromFile(
        const std::string& filepath);
    
    /**
     * @brief Validate Framework.
     * @param[in] registry Input parameter.
     * @return True when the operation succeeds.
     */
    static bool validateFramework(const ComplianceFrameworkRegistry& registry);
    
    /**
     * @brief Get Framework Version.
     * @param[in] fw Input parameter.
     * @return Return value.
     */
    static std::string getFrameworkVersion(ComplianceFramework fw);

private:
    /**
     * @brief Load Iso27001.
     * @return Return value.
     */
    static std::shared_ptr<ComplianceFrameworkRegistry> loadIso27001();
    /**
     * @brief Load Soc2.
     * @return Return value.
     */
    static std::shared_ptr<ComplianceFrameworkRegistry> loadSoc2();
    /**
     * @brief Load Gdpr.
     * @return Return value.
     */
    static std::shared_ptr<ComplianceFrameworkRegistry> loadGdpr();
    /**
     * @brief Load Ccpa.
     * @return Return value.
     */
    static std::shared_ptr<ComplianceFrameworkRegistry> loadCcpa();
    /**
     * @brief Load Hipaa.
     * @return Return value.
     */
    static std::shared_ptr<ComplianceFrameworkRegistry> loadHipaa();
    /**
     * @brief Load Pci Dss.
     * @return Return value.
     */
    static std::shared_ptr<ComplianceFrameworkRegistry> loadPciDss();
    /**
     * @brief Load Eu Ai Act.
     * @return Return value.
     */
    static std::shared_ptr<ComplianceFrameworkRegistry> loadEuAiAct();
};

// ============================================================================
// ISO 27001:2022 Mappings
// ============================================================================

namespace iso27001 {

namespace organization_controls {
    constexpr auto kAccessControlPolicy = "A.5.1";
    constexpr auto kInfoSecurityRoles = "A.5.2";
    constexpr auto kSegregationOfDuties = "A.5.3";
}

namespace people_controls {
    constexpr auto kScreening = "A.6.1";
    constexpr auto kTermsAndConditions = "A.6.2";
    constexpr auto kAwarenessTraining = "A.6.3";
}

namespace physical_controls {
    constexpr auto kPerimeterSecurity = "A.7.1";
    constexpr auto kPhysicalEntry = "A.7.2";
    constexpr auto kOfficeSuppliesAccess = "A.7.3";
}

namespace technological_controls {
    constexpr auto kUserEndpointDevices = "A.8.1";
    constexpr auto kServerRoom = "A.8.2";
    constexpr auto kNetworking = "A.8.3";
    constexpr auto kCryptography = "A.8.4";
    constexpr auto kPhysicalCryptographicMedia = "A.8.5";
}

namespace technical_controls {
    constexpr auto kAccessControl = "A.9.1";
    constexpr auto kUserManagement = "A.9.2";
    constexpr auto kSpecialAccessRights = "A.9.3";
    constexpr auto kAccessMeasurement = "A.9.4";
    constexpr auto kEncryptionKeys = "A.9.5";
}

namespace cryptography_controls {
    constexpr auto kPolicyAndPlans = "A.10.1";
    constexpr auto kKeyManagement = "A.10.2";
}

namespace physical_environmental_controls {
    constexpr auto kPerimeterSecurityControls = "A.11.1";
    constexpr auto kFacilitySecurityControls = "A.11.2";
}

namespace operations_controls {
    constexpr auto kOperationalProcedures = "A.12.1";
    constexpr auto kChangeManagement = "A.12.2";
    constexpr auto kCapacityAndResourceManagement = "A.12.3";
    constexpr auto kSeparationOfDevelopmentAndProduction = "A.12.4";
    constexpr auto kAccessControlToOperationalFacilities = "A.12.5";
    constexpr auto kSegmentationOfNetworks = "A.12.6";
}

namespace communications_controls {
    constexpr auto kNetworkSecurityControls = "A.13.1";
    constexpr auto kDataTransportSecurity = "A.13.2";
}

namespace system_acquisition_controls {
    constexpr auto kInfoSecurityRequirements = "A.14.1";
    constexpr auto kSecureInstallation = "A.14.2";
}

namespace supplier_controls {
    constexpr auto kSupplierPolicies = "A.15.1";
    constexpr auto kSupplierSecurityManagement = "A.15.2";
}

namespace incident_management_controls {
    constexpr auto kIncidentResponseManagement = "A.16.1";
}

namespace business_continuity_controls {
    constexpr auto kBcmObjectivesAndPlanning = "A.17.1";
    constexpr auto kImplementingBcm = "A.17.2";
    constexpr auto kTestingEvaluationAndImprovingBcm = "A.17.3";
}

namespace compliance_controls {
    constexpr auto kComplianceWithLaws = "A.18.1";
    constexpr auto kInfoSecurityReviews = "A.18.2";
}

} // namespace iso27001

// ============================================================================
// SOC 2 Mappings
// ============================================================================

namespace soc2 {

namespace criteria {
    namespace common {
        constexpr auto kEnvironmentAndResources = "CC1";
        constexpr auto kGoalsAndObjectives = "CC2";
        constexpr auto kResponsibility = "CC3";
        constexpr auto kCompetence = "CC4";
        constexpr auto kBehavior = "CC5";
        constexpr auto kReporting = "CC6";
        constexpr auto kMonitoring = "CC7";
        constexpr auto kEvaluationOfControl = "CC8";
        constexpr auto kRoleOfInternalAudit = "CC9";
    }
    
    namespace security {
        constexpr auto kLogicalAndPhysicalAccess = "C1";
        constexpr auto kSystem = "C2";
    }
    
    namespace availability {
        constexpr auto kAvailability = "A1";
    }
    
    namespace processing {
        constexpr auto kProcessingIntegrity = "P1";
    }
    
    namespace confidentiality {
        constexpr auto kConfidentiality = "CF1";
    }
    
    namespace privacy {
        constexpr auto kPrivacy = "PF1";
    }
}

} // namespace soc2

// ============================================================================
// GDPR Mappings
// ============================================================================

namespace gdpr {

namespace articles {
    constexpr auto kDataProtectionByDesign = "Article 25";
    constexpr auto kSecurity = "Article 32";
    constexpr auto kDataBreach = "Article 33";
    constexpr auto kNotification = "Article 34";
    constexpr auto kDpia = "Article 35";
    constexpr auto kConsent = "Article 7";
    constexpr auto kLawfulness = "Article 6";
    constexpr auto kMinimization = "Article 5";
    constexpr auto kTransparency = "Article 13";
    constexpr auto kSubjectRights = "Chapter III";
    constexpr auto kDataRetention = "Article 5(1)(e)";
}

} // namespace gdpr

// ============================================================================
// CCPA Mappings
// ============================================================================

namespace ccpa {

namespace sections {
    constexpr auto kPrivacyPolicy = "Section 1798.100";
    constexpr auto kConsumerRights = "Section 1798.100";
    constexpr auto kSaleOfPersonalInformation = "Section 1798.115";
    constexpr auto kDataSecurity = "Section 1798.100";
    constexpr auto kVerification = "Section 1798.130";
    constexpr auto kOptOut = "Section 1798.120";
}

} // namespace ccpa

// ============================================================================
// HIPAA Mappings
// ============================================================================

namespace hipaa {

namespace technical_safeguards {
    constexpr auto kAccessControl = "164.312(a)(2)";
    constexpr auto kAuditControls = "164.312(b)";
    constexpr auto kIntegrityControls = "164.312(c)";
    constexpr auto kTransmissionSecurity = "164.312(e)";
    constexpr auto kEncryption = "164.312(a)(2)(ii)";
}

} // namespace hipaa

// ============================================================================
// PCI-DSS Mappings
// ============================================================================

namespace pci_dss {

namespace requirements {
    constexpr auto kInstallFirewall = "1";
    constexpr auto kChangeDefaultPasswords = "2";
    constexpr auto kProtectData = "3";
    constexpr auto kEncryption = "4";
    constexpr auto kVulnerabilityManagement = "6";
    constexpr auto kAccessControl = "7";
    constexpr auto kIdentification = "8";
    constexpr auto kMonitoring = "10";
    constexpr auto kIncidentResponse = "12";
}

} // namespace pci_dss

// ============================================================================
// EU AI Act Mappings
// ============================================================================

namespace eu_ai_act {

namespace articles {
    constexpr auto kRiskCategorization = "Article 6";
    constexpr auto kHighRiskSystems = "Article 8";
    constexpr auto kTransparency = "Article 13";
    constexpr auto kHumanOversight = "Article 14";
    constexpr auto kMonitoring = "Article 26";
    constexpr auto kRiskManagement = "Annex I";
    constexpr auto kDocumentation = "Annex V";
}

} // namespace eu_ai_act

// ============================================================================
// Cross-Framework Mappings
// ============================================================================

class CrossFrameworkMapping {
public:
    /**
     * @brief Get Equivalent Requirements.
     * @param[in] requirement_id Identifier of the requirement.
     * @param[in] from_fw Input parameter.
     * @param[in] to_fw Input parameter.
     * @return Return value.
     */
    std::vector<std::string> getEquivalentRequirements(
        const std::string& requirement_id,
        ComplianceFramework from_fw,
        ComplianceFramework to_fw) const;
    
    /**
     * @brief Are Equivalent.
     * @param[in] req1 Input parameter.
     * @param[in] fw1 Input parameter.
     * @param[in] req2 Input parameter.
     * @param[in] fw2 Input parameter.
     * @return True when the operation succeeds.
     */
    bool areEquivalent(
        const std::string& req1,
        ComplianceFramework fw1,
        const std::string& req2,
        ComplianceFramework fw2) const;
    
    std::map<ComplianceFramework, std::vector<std::string>>
    getAllEquivalentRequirements(
        const std::string& requirement_id,
        ComplianceFramework source_fw) const;

private:
    // Equivalence mapping: (fw1, req1, fw2, req2)
    std::vector<std::tuple<ComplianceFramework, std::string,
                          ComplianceFramework, std::string>>
    equivalences_;
};

// ============================================================================
// Compliance Requirement Builder
// ============================================================================

class ComplianceRequirementBuilder {
public:
    /**
     * @brief With Id.
     * @param[in] id Input parameter.
     * @return Return value.
     */
    ComplianceRequirementBuilder& withId(const std::string& id);
    /**
     * @brief With Framework.
     * @param[in] fw Input parameter.
     * @return Return value.
     */
    ComplianceRequirementBuilder& withFramework(ComplianceFramework fw);
    /**
     * @brief With Text.
     * @param[in] text Input parameter.
     * @return Return value.
     */
    ComplianceRequirementBuilder& withText(const std::string& text);
    /**
     * @brief With Section.
     * @param[in] section Input parameter.
     * @return Return value.
     */
    ComplianceRequirementBuilder& withSection(const std::string& section);
    /**
     * @brief With Severity.
     * @param[in] sev Input parameter.
     * @return Return value.
     */
    ComplianceRequirementBuilder& withSeverity(ComplianceSeverity sev);
    /**
     * @brief With Control.
     * @param[in] control_id Identifier of the control.
     * @return Return value.
     */
    ComplianceRequirementBuilder& withControl(const std::string& control_id);
    /**
     * @brief With Category.
     * @param[in] cat Input parameter.
     * @return Return value.
     */
    ComplianceRequirementBuilder& withCategory(const std::string& cat);
    /**
     * @brief With Mandatory.
     * @param[in] mandatory Input parameter.
     * @return Return value.
     */
    ComplianceRequirementBuilder& withMandatory(bool mandatory);
    
    /**
     * @brief Build.
     * @return Return value.
     */
    ComplianceRequirement build() const;

private:
    ComplianceRequirement req_;
};

// ============================================================================
// Compliance Control Builder
// ============================================================================

class ComplianceControlBuilder {
public:
    /**
     * @brief With Id.
     * @param[in] id Input parameter.
     * @return Return value.
     */
    ComplianceControlBuilder& withId(const std::string& id);
    /**
     * @brief With Framework.
     * @param[in] fw Input parameter.
     * @return Return value.
     */
    ComplianceControlBuilder& withFramework(ComplianceFramework fw);
    /**
     * @brief With Name.
     * @param[in] name Input parameter.
     * @return Return value.
     */
    ComplianceControlBuilder& withName(const std::string& name);
    /**
     * @brief With Description.
     * @param[in] desc Input parameter.
     * @return Return value.
     */
    ComplianceControlBuilder& withDescription(const std::string& desc);
    /**
     * @brief With Implementation.
     * @param[in] impl Input parameter.
     * @return Return value.
     */
    ComplianceControlBuilder& withImplementation(const std::string& impl);
    /**
     * @brief Automated.
     * @param[in] is_automated Input parameter.
     * @return Return value.
     */
    ComplianceControlBuilder& automated(bool is_automated);
    /**
     * @brief With Policy Rule.
     * @param[in] rule_id Identifier of the rule.
     * @return Return value.
     */
    ComplianceControlBuilder& withPolicyRule(const std::string& rule_id);
    /**
     * @brief With Evidence Type.
     * @param[in] type Input parameter.
     * @return Return value.
     */
    ComplianceControlBuilder& withEvidenceType(const std::string& type);
    
    /**
     * @brief Build.
     * @return Return value.
     */
    ComplianceControl build() const;

private:
    ComplianceControl ctl_;
};

} // namespace governance
} // namespace themis
