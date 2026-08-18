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

namespace themis {
namespace governance {

// ============================================================================
// Compliance Framework Loader
// ============================================================================

/**
 * @brief Loads and validates compliance framework definitions
 * 
 * Loads requirement and control definitions from authoritative sources
 * and validates internal consistency and completeness.
 */
class ComplianceFrameworkLoader {
public:
    /// Load all supported frameworks
    static std::shared_ptr<ComplianceFrameworkRegistry> loadAllFrameworks();
    
    /// Load specific framework
    static std::shared_ptr<ComplianceFrameworkRegistry> loadFramework(
        ComplianceFramework fw);
    
    /// Load framework from JSON file
    static std::shared_ptr<ComplianceFrameworkRegistry> loadFromFile(
        const std::string& filepath);
    
    /// Validate framework consistency
    static bool validateFramework(const ComplianceFrameworkRegistry& registry);
    
    /// Get framework version
    static std::string getFrameworkVersion(ComplianceFramework fw);

private:
    static std::shared_ptr<ComplianceFrameworkRegistry> loadIso27001();
    static std::shared_ptr<ComplianceFrameworkRegistry> loadSoc2();
    static std::shared_ptr<ComplianceFrameworkRegistry> loadGdpr();
    static std::shared_ptr<ComplianceFrameworkRegistry> loadCcpa();
    static std::shared_ptr<ComplianceFrameworkRegistry> loadHipaa();
    static std::shared_ptr<ComplianceFrameworkRegistry> loadPciDss();
    static std::shared_ptr<ComplianceFrameworkRegistry> loadEuAiAct();
};

// ============================================================================
// ISO 27001:2022 Mappings
// ============================================================================

namespace iso27001 {

/// A.5 Organization controls
namespace organization_controls {
    constexpr auto kAccessControlPolicy = "A.5.1";
    constexpr auto kInfoSecurityRoles = "A.5.2";
    constexpr auto kSegregationOfDuties = "A.5.3";
}

/// A.6 People controls
namespace people_controls {
    constexpr auto kScreening = "A.6.1";
    constexpr auto kTermsAndConditions = "A.6.2";
    constexpr auto kAwarenessTraining = "A.6.3";
}

/// A.7 Physical controls
namespace physical_controls {
    constexpr auto kPerimeterSecurity = "A.7.1";
    constexpr auto kPhysicalEntry = "A.7.2";
    constexpr auto kOfficeSuppliesAccess = "A.7.3";
}

/// A.8 Technological controls
namespace technological_controls {
    constexpr auto kUserEndpointDevices = "A.8.1";
    constexpr auto kServerRoom = "A.8.2";
    constexpr auto kNetworking = "A.8.3";
    constexpr auto kCryptography = "A.8.4";
    constexpr auto kPhysicalCryptographicMedia = "A.8.5";
}

/// A.9 Technical controls
namespace technical_controls {
    constexpr auto kAccessControl = "A.9.1";
    constexpr auto kUserManagement = "A.9.2";
    constexpr auto kSpecialAccessRights = "A.9.3";
    constexpr auto kAccessMeasurement = "A.9.4";
    constexpr auto kEncryptionKeys = "A.9.5";
}

/// A.10 Cryptography
namespace cryptography_controls {
    constexpr auto kPolicyAndPlans = "A.10.1";
    constexpr auto kKeyManagement = "A.10.2";
}

/// A.11 Physical and environmental
namespace physical_environmental_controls {
    constexpr auto kPerimeterSecurityControls = "A.11.1";
    constexpr auto kFacilitySecurityControls = "A.11.2";
}

/// A.12 Operations
namespace operations_controls {
    constexpr auto kOperationalProcedures = "A.12.1";
    constexpr auto kChangeManagement = "A.12.2";
    constexpr auto kCapacityAndResourceManagement = "A.12.3";
    constexpr auto kSeparationOfDevelopmentAndProduction = "A.12.4";
    constexpr auto kAccessControlToOperationalFacilities = "A.12.5";
    constexpr auto kSegmentationOfNetworks = "A.12.6";
}

/// A.13 Communications
namespace communications_controls {
    constexpr auto kNetworkSecurityControls = "A.13.1";
    constexpr auto kDataTransportSecurity = "A.13.2";
}

/// A.14 System Acquisition
namespace system_acquisition_controls {
    constexpr auto kInfoSecurityRequirements = "A.14.1";
    constexpr auto kSecureInstallation = "A.14.2";
}

/// A.15 Supplier relationships
namespace supplier_controls {
    constexpr auto kSupplierPolicies = "A.15.1";
    constexpr auto kSupplierSecurityManagement = "A.15.2";
}

/// A.16 Information security incident management
namespace incident_management_controls {
    constexpr auto kIncidentResponseManagement = "A.16.1";
}

/// A.17 Business continuity
namespace business_continuity_controls {
    constexpr auto kBcmObjectivesAndPlanning = "A.17.1";
    constexpr auto kImplementingBcm = "A.17.2";
    constexpr auto kTestingEvaluationAndImprovingBcm = "A.17.3";
}

/// A.18 Compliance
namespace compliance_controls {
    constexpr auto kComplianceWithLaws = "A.18.1";
    constexpr auto kInfoSecurityReviews = "A.18.2";
}

} // namespace iso27001

// ============================================================================
// SOC 2 Mappings
// ============================================================================

namespace soc2 {

/// Trust Service Criteria
namespace criteria {
    /// CC - Common Criteria
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
    
    /// C - Security
    namespace security {
        constexpr auto kLogicalAndPhysicalAccess = "C1";
        constexpr auto kSystem = "C2";
    }
    
    /// A - Availability
    namespace availability {
        constexpr auto kAvailability = "A1";
    }
    
    /// P - Processing Integrity
    namespace processing {
        constexpr auto kProcessingIntegrity = "P1";
    }
    
    /// CF - Confidentiality
    namespace confidentiality {
        constexpr auto kConfidentiality = "CF1";
    }
    
    /// PF - Privacy
    namespace privacy {
        constexpr auto kPrivacy = "PF1";
    }
}

} // namespace soc2

// ============================================================================
// GDPR Mappings
// ============================================================================

namespace gdpr {

/// GDPR Articles
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

/// CCPA Sections
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

/// HIPAA Technical Safeguards
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

/// PCI-DSS Requirements
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

/// EU AI Act Articles
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

/**
 * @brief Maps equivalent requirements across frameworks
 * 
 * For example, GDPR Article 32 (Security) is equivalent to
 * ISO 27001 A.13 (Communications) in certain aspects.
 */
class CrossFrameworkMapping {
public:
    /// Get equivalent requirements in another framework
    std::vector<std::string> getEquivalentRequirements(
        const std::string& requirement_id,
        ComplianceFramework from_fw,
        ComplianceFramework to_fw) const;
    
    /// Check if two requirements are equivalent
    bool areEquivalent(
        const std::string& req1,
        ComplianceFramework fw1,
        const std::string& req2,
        ComplianceFramework fw2) const;
    
    /// Get all equivalent requirements across all frameworks
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

/**
 * @brief Fluent builder for compliance requirements
 */
class ComplianceRequirementBuilder {
public:
    ComplianceRequirementBuilder& withId(const std::string& id);
    ComplianceRequirementBuilder& withFramework(ComplianceFramework fw);
    ComplianceRequirementBuilder& withText(const std::string& text);
    ComplianceRequirementBuilder& withSection(const std::string& section);
    ComplianceRequirementBuilder& withSeverity(ComplianceSeverity sev);
    ComplianceRequirementBuilder& withControl(const std::string& control_id);
    ComplianceRequirementBuilder& withCategory(const std::string& cat);
    ComplianceRequirementBuilder& withMandatory(bool mandatory);
    
    ComplianceRequirement build() const;

private:
    ComplianceRequirement req_;
};

// ============================================================================
// Compliance Control Builder
// ============================================================================

/**
 * @brief Fluent builder for compliance controls
 */
class ComplianceControlBuilder {
public:
    ComplianceControlBuilder& withId(const std::string& id);
    ComplianceControlBuilder& withFramework(ComplianceFramework fw);
    ComplianceControlBuilder& withName(const std::string& name);
    ComplianceControlBuilder& withDescription(const std::string& desc);
    ComplianceControlBuilder& withImplementation(const std::string& impl);
    ComplianceControlBuilder& automated(bool is_automated);
    ComplianceControlBuilder& withPolicyRule(const std::string& rule_id);
    ComplianceControlBuilder& withEvidenceType(const std::string& type);
    
    ComplianceControl build() const;

private:
    ComplianceControl ctl_;
};

} // namespace governance
} // namespace themis
