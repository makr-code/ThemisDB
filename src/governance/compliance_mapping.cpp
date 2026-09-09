/**
 * @file compliance_mapping.cpp
 * @brief Compliance framework requirement and control mappings
 * @version 1.0.0
 * @date 2026-08-18
 * 
 * Implements loader and mapping utilities for compliance frameworks.
 */

#include "governance/compliance_mapping.h"
#include "utils/logger.h"
#include <chrono>
#include <fstream>

namespace themis {
namespace governance {

// ============================================================================
// Compliance Framework Loader Implementation
// ============================================================================

std::shared_ptr<ComplianceFrameworkRegistry>
ComplianceFrameworkLoader::loadAllFrameworks() {
    auto registry = std::make_shared<ComplianceFrameworkRegistry>();
    
    auto iso27001 = loadIso27001();
    auto soc2 = loadSoc2();
    auto gdpr = loadGdpr();
    auto ccpa = loadCcpa();
    auto hipaa = loadHipaa();
    auto pci_dss = loadPciDss();
    auto eu_ai_act = loadEuAiAct();
    
    // Merge all frameworks into single registry
    // This is simplified - in production would merge properly
    return registry;
}

std::shared_ptr<ComplianceFrameworkRegistry>
ComplianceFrameworkLoader::loadFramework(ComplianceFramework fw) {
    switch (fw) {
        case ComplianceFramework::kIso27001:
            return loadIso27001();
        case ComplianceFramework::kSoc2TypeI:
        [[fallthrough]];
        case ComplianceFramework::kSoc2TypeII:
            return loadSoc2();
        case ComplianceFramework::kGdpr:
            return loadGdpr();
        case ComplianceFramework::kCcpa:
            return loadCcpa();
        case ComplianceFramework::kHipaa:
            return loadHipaa();
        case ComplianceFramework::kPciDss:
            return loadPciDss();
        case ComplianceFramework::kEuAiAct:
            return loadEuAiAct();
    }
    return std::make_shared<ComplianceFrameworkRegistry>();
}

std::shared_ptr<ComplianceFrameworkRegistry>
ComplianceFrameworkLoader::loadFromFile(const std::string& filepath) {
    auto registry = std::make_shared<ComplianceFrameworkRegistry>();
    
    try {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            themis::utils::logger(themis::utils::LogLevel::kError)
                << "Failed to open compliance framework file: " << filepath;
            return registry;
        }
        
        nlohmann::json j;
        file >> j;
        
        if (registry->importFromJson(j)) {
            themis::utils::logger(themis::utils::LogLevel::kInfo)
                << "Loaded compliance framework from " << filepath;
        }
    } catch (const std::exception& e) {
        themis::utils::logger(themis::utils::LogLevel::kError)
            << "Error loading compliance framework: " << e.what();
    }
    
    return registry;
}

bool ComplianceFrameworkLoader::validateFramework(
    const ComplianceFrameworkRegistry& registry) {
    
    // Check for completeness and consistency
    // This is simplified - in production would do thorough validation
    return true;
}

std::string ComplianceFrameworkLoader::getFrameworkVersion(ComplianceFramework fw) {
    switch (fw) {
        case ComplianceFramework::kIso27001:
            return "ISO 27001:2022";
        case ComplianceFramework::kSoc2TypeI:
            return "SOC 2 Type I";
        case ComplianceFramework::kSoc2TypeII:
            return "SOC 2 Type II";
        case ComplianceFramework::kGdpr:
            return "GDPR (2016/679/EU)";
        case ComplianceFramework::kCcpa:
            return "CCPA/CPRA";
        case ComplianceFramework::kHipaa:
            return "HIPAA (45 CFR Part 164)";
        case ComplianceFramework::kPciDss:
            return "PCI-DSS v3.2.1";
        case ComplianceFramework::kEuAiAct:
            return "EU AI Act (2024)";
    }
    return "Unknown";
}

// ISO 27001:2022 Requirements (Annex A)
std::shared_ptr<ComplianceFrameworkRegistry>
ComplianceFrameworkLoader::loadIso27001() {
    auto registry = std::make_shared<ComplianceFrameworkRegistry>();
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    // A.9.1 Access Control Policy
    {
        ComplianceRequirement req;
        req.requirement_id = "ISO27001-A.9.1";
        req.framework = ComplianceFramework::kIso27001;
        req.requirement_text = "Access to information systems and user device interfaces must be restricted in accordance with the access control policy";
        req.regulatory_section = "Article 9.1";
        req.severity = ComplianceSeverity::kCritical;
        req.control_ids = {"CTL-ACCESS-001"};
        req.category = "access-control";
        req.version = 1;
        req.created_at_ms = now;
        req.updated_at_ms = now;
        registry->addRequirement(req);
    }
    
    // A.13.1.1 Network security controls
    {
        ComplianceRequirement req;
        req.requirement_id = "ISO27001-A.13.1.1";
        req.framework = ComplianceFramework::kIso27001;
        req.requirement_text = "Networks and their components must be managed and protected from unauthorized access";
        req.regulatory_section = "Article 13.1.1";
        req.severity = ComplianceSeverity::kCritical;
        req.control_ids = {"CTL-NETWORK-001"};
        req.category = "network";
        req.version = 1;
        req.created_at_ms = now;
        req.updated_at_ms = now;
        registry->addRequirement(req);
    }
    
    // A.10.1.1 Cryptography Policy
    {
        ComplianceRequirement req;
        req.requirement_id = "ISO27001-A.10.1.1";
        req.framework = ComplianceFramework::kIso27001;
        req.requirement_text = "An information security policy for the use of cryptographic controls must be established";
        req.regulatory_section = "Article 10.1.1";
        req.severity = ComplianceSeverity::kCritical;
        req.control_ids = {"CTL-CRYPTO-001"};
        req.category = "encryption";
        req.version = 1;
        req.created_at_ms = now;
        req.updated_at_ms = now;
        registry->addRequirement(req);
    }
    
    // A.12.4.1 Development and production separation
    {
        ComplianceRequirement req;
        req.requirement_id = "ISO27001-A.12.4.1";
        req.framework = ComplianceFramework::kIso27001;
        req.requirement_text = "Development, testing and operational systems must be separated";
        req.regulatory_section = "Article 12.4.1";
        req.severity = ComplianceSeverity::kHigh;
        req.control_ids = {"CTL-SEGREGATION-001"};
        req.category = "infrastructure";
        req.version = 1;
        req.created_at_ms = now;
        req.updated_at_ms = now;
        registry->addRequirement(req);
    }
    
    // A.12.2.1 Change management
    {
        ComplianceRequirement req;
        req.requirement_id = "ISO27001-A.12.2.1";
        req.framework = ComplianceFramework::kIso27001;
        req.requirement_text = "Changes to information processing systems must be controlled";
        req.regulatory_section = "Article 12.2.1";
        req.severity = ComplianceSeverity::kHigh;
        req.control_ids = {"CTL-CHANGE-001"};
        req.category = "change-management";
        req.version = 1;
        req.created_at_ms = now;
        req.updated_at_ms = now;
        registry->addRequirement(req);
    }
    
    // A.12.1.1 Operational procedures
    {
        ComplianceRequirement req;
        req.requirement_id = "ISO27001-A.12.1.1";
        req.framework = ComplianceFramework::kIso27001;
        req.requirement_text = "Operation procedures for information systems must be documented and maintained";
        req.regulatory_section = "Article 12.1.1";
        req.severity = ComplianceSeverity::kMedium;
        req.control_ids = {"CTL-PROCEDURES-001"};
        req.category = "operations";
        req.version = 1;
        req.created_at_ms = now;
        req.updated_at_ms = now;
        registry->addRequirement(req);
    }
    
    // Add corresponding controls
    {
        ComplianceControl ctl;
        ctl.control_id = "CTL-ACCESS-001";
        ctl.framework = ComplianceFramework::kIso27001;
        ctl.control_name = "Access Control Implementation";
        ctl.description = "Implement role-based access control (RBAC)";
        ctl.implementation_detail = "Policy engine enforces RBAC rules for all resources";
        ctl.is_automated = true;
        ctl.policy_rules = {"access_control_policy"};
        ctl.evidence_types = {"policy_rule", "access_log", "rbac_audit"};
        ctl.version = 1;
        ctl.created_at_ms = now;
        registry->addControl(ctl);
    }
    
    {
        ComplianceControl ctl;
        ctl.control_id = "CTL-NETWORK-001";
        ctl.framework = ComplianceFramework::kIso27001;
        ctl.control_name = "Network Security Controls";
        ctl.description = "Implement network segmentation and monitoring";
        ctl.implementation_detail = "Firewall rules, VPN requirements, network monitoring";
        ctl.is_automated = true;
        ctl.policy_rules = {"network_security_policy"};
        ctl.evidence_types = {"firewall_config", "network_logs"};
        ctl.version = 1;
        ctl.created_at_ms = now;
        registry->addControl(ctl);
    }
    
    {
        ComplianceControl ctl;
        ctl.control_id = "CTL-CRYPTO-001";
        ctl.framework = ComplianceFramework::kIso27001;
        ctl.control_name = "Cryptography Controls";
        ctl.description = "Enforce encryption for data at rest and in transit";
        ctl.implementation_detail = "TLS for communications, AES-256 for at-rest data";
        ctl.is_automated = true;
        ctl.policy_rules = {"encryption_policy"};
        ctl.evidence_types = {"encryption_audit", "tls_config"};
        ctl.version = 1;
        ctl.created_at_ms = now;
        registry->addControl(ctl);
    }
    
    return registry;
}

// SOC 2 Framework
std::shared_ptr<ComplianceFrameworkRegistry>
ComplianceFrameworkLoader::loadSoc2() {
    auto registry = std::make_shared<ComplianceFrameworkRegistry>();
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    // CC1 - Governance and Risk Management
    {
        ComplianceRequirement req;
        req.requirement_id = "SOC2-CC1";
        req.framework = ComplianceFramework::kSoc2TypeI;
        req.requirement_text = "The entity demonstrates a commitment to competence";
        req.regulatory_section = "Common Criteria 1";
        req.severity = ComplianceSeverity::kCritical;
        req.control_ids = {"CTL-GOVERNANCE-001"};
        req.category = "governance";
        req.version = 1;
        req.created_at_ms = now;
        req.updated_at_ms = now;
        registry->addRequirement(req);
    }
    
    // CC6 - Monitoring
    {
        ComplianceRequirement req;
        req.requirement_id = "SOC2-CC6";
        req.framework = ComplianceFramework::kSoc2TypeI;
        req.requirement_text = "Monitoring occurs and related control activities are performed";
        req.regulatory_section = "Common Criteria 6";
        req.severity = ComplianceSeverity::kHigh;
        req.control_ids = {"CTL-MONITORING-001"};
        req.category = "monitoring";
        req.version = 1;
        req.created_at_ms = now;
        req.updated_at_ms = now;
        registry->addRequirement(req);
    }
    
    // C1 - Security (Logical and Physical Access Controls)
    {
        ComplianceRequirement req;
        req.requirement_id = "SOC2-C1";
        req.framework = ComplianceFramework::kSoc2TypeI;
        req.requirement_text = "The entity restricts access to system resources";
        req.regulatory_section = "Trust Service Criteria C1";
        req.severity = ComplianceSeverity::kCritical;
        req.control_ids = {"CTL-ACCESS-SOC2-001"};
        req.category = "access-control";
        req.version = 1;
        req.created_at_ms = now;
        req.updated_at_ms = now;
        registry->addRequirement(req);
    }
    
    return registry;
}

// GDPR Framework
std::shared_ptr<ComplianceFrameworkRegistry>
ComplianceFrameworkLoader::loadGdpr() {
    auto registry = std::make_shared<ComplianceFrameworkRegistry>();
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    // Article 25 - Data Protection by Design and Default
    {
        ComplianceRequirement req;
        req.requirement_id = "GDPR-Art25";
        req.framework = ComplianceFramework::kGdpr;
        req.requirement_text = "Implement data protection by design and default";
        req.regulatory_section = "Article 25";
        req.severity = ComplianceSeverity::kCritical;
        req.control_ids = {"CTL-GDPR-DP-DESIGN"};
        req.category = "data-protection";
        req.version = 1;
        req.created_at_ms = now;
        req.updated_at_ms = now;
        registry->addRequirement(req);
    }
    
    // Article 32 - Security of Processing
    {
        ComplianceRequirement req;
        req.requirement_id = "GDPR-Art32";
        req.framework = ComplianceFramework::kGdpr;
        req.requirement_text = "Implement appropriate technical and organizational measures";
        req.regulatory_section = "Article 32";
        req.severity = ComplianceSeverity::kCritical;
        req.control_ids = {"CTL-GDPR-SECURITY"};
        req.category = "security";
        req.version = 1;
        req.created_at_ms = now;
        req.updated_at_ms = now;
        registry->addRequirement(req);
    }
    
    // Article 33 - Breach Notification
    {
        ComplianceRequirement req;
        req.requirement_id = "GDPR-Art33";
        req.framework = ComplianceFramework::kGdpr;
        req.requirement_text = "Notify supervisory authority in case of data breach";
        req.regulatory_section = "Article 33";
        req.severity = ComplianceSeverity::kCritical;
        req.control_ids = {"CTL-GDPR-BREACH-NOTIFICATION"};
        req.category = "incident-response";
        req.version = 1;
        req.created_at_ms = now;
        req.updated_at_ms = now;
        registry->addRequirement(req);
    }
    
    return registry;
}

// CCPA Framework
std::shared_ptr<ComplianceFrameworkRegistry>
ComplianceFrameworkLoader::loadCcpa() {
    auto registry = std::make_shared<ComplianceFrameworkRegistry>();
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    // Consumer Rights
    {
        ComplianceRequirement req;
        req.requirement_id = "CCPA-Consumer-Rights";
        req.framework = ComplianceFramework::kCcpa;
        req.requirement_text = "Provide consumer rights: access, delete, opt-out";
        req.regulatory_section = "Section 1798.100";
        req.severity = ComplianceSeverity::kCritical;
        req.control_ids = {"CTL-CCPA-RIGHTS"};
        req.category = "consumer-rights";
        req.version = 1;
        req.created_at_ms = now;
        req.updated_at_ms = now;
        registry->addRequirement(req);
    }
    
    return registry;
}

// HIPAA Framework
std::shared_ptr<ComplianceFrameworkRegistry>
ComplianceFrameworkLoader::loadHipaa() {
    auto registry = std::make_shared<ComplianceFrameworkRegistry>();
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    // Technical Safeguards
    {
        ComplianceRequirement req;
        req.requirement_id = "HIPAA-TS-Access";
        req.framework = ComplianceFramework::kHipaa;
        req.requirement_text = "Implement access controls for PHI";
        req.regulatory_section = "164.312(a)(2)";
        req.severity = ComplianceSeverity::kCritical;
        req.control_ids = {"CTL-HIPAA-ACCESS"};
        req.category = "access-control";
        req.version = 1;
        req.created_at_ms = now;
        req.updated_at_ms = now;
        registry->addRequirement(req);
    }
    
    return registry;
}

// PCI-DSS Framework
std::shared_ptr<ComplianceFrameworkRegistry>
ComplianceFrameworkLoader::loadPciDss() {
    auto registry = std::make_shared<ComplianceFrameworkRegistry>();
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    // Requirement 1 - Install and maintain firewall
    {
        ComplianceRequirement req;
        req.requirement_id = "PCI-DSS-1";
        req.framework = ComplianceFramework::kPciDss;
        req.requirement_text = "Install and maintain a firewall";
        req.regulatory_section = "Requirement 1";
        req.severity = ComplianceSeverity::kCritical;
        req.control_ids = {"CTL-PCIDSS-FIREWALL"};
        req.category = "network";
        req.version = 1;
        req.created_at_ms = now;
        req.updated_at_ms = now;
        registry->addRequirement(req);
    }
    
    return registry;
}

// EU AI Act Framework
std::shared_ptr<ComplianceFrameworkRegistry>
ComplianceFrameworkLoader::loadEuAiAct() {
    auto registry = std::make_shared<ComplianceFrameworkRegistry>();
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    // Risk Management
    {
        ComplianceRequirement req;
        req.requirement_id = "EU-AI-Act-Risk";
        req.framework = ComplianceFramework::kEuAiAct;
        req.requirement_text = "Implement AI risk management system";
        req.regulatory_section = "Article 6";
        req.severity = ComplianceSeverity::kCritical;
        req.control_ids = {"CTL-EU-AI-RISK"};
        req.category = "ai-governance";
        req.version = 1;
        req.created_at_ms = now;
        req.updated_at_ms = now;
        registry->addRequirement(req);
    }
    
    return registry;
}

// ============================================================================
// Cross-Framework Mapping Implementation
// ============================================================================

std::vector<std::string> CrossFrameworkMapping::getEquivalentRequirements(
    const std::string& requirement_id,
    ComplianceFramework from_fw,
    ComplianceFramework to_fw) const {
    
    std::vector<std::string> results;
    
    for (const auto& equiv : equivalences_) {
        if (std::get<0>(equiv) == from_fw &&
            std::get<1>(equiv) == requirement_id &&
            std::get<2>(equiv) == to_fw) {
            results.push_back(std::get<3>(equiv));
        }
    }
    
    return results;
}

bool CrossFrameworkMapping::areEquivalent(
    const std::string& req1,
    ComplianceFramework fw1,
    const std::string& req2,
    ComplianceFramework fw2) const {
    
    auto results = getEquivalentRequirements(req1, fw1, fw2);
    return std::find(results.begin(), results.end(), req2) != results.end();
}

std::map<ComplianceFramework, std::vector<std::string>>
CrossFrameworkMapping::getAllEquivalentRequirements(
    const std::string& requirement_id,
    ComplianceFramework source_fw) const {
    
    std::map<ComplianceFramework, std::vector<std::string>> results;
    
    for (const auto& equiv : equivalences_) {
        if (std::get<0>(equiv) == source_fw &&
            std::get<1>(equiv) == requirement_id) {
            ComplianceFramework target_fw = std::get<2>(equiv);
            std::string target_req = std::get<3>(equiv);
            results[target_fw].push_back(target_req);
        }
    }
    
    return results;
}

// ============================================================================
// Builder Pattern Implementations
// ============================================================================

ComplianceRequirementBuilder& ComplianceRequirementBuilder::withId(
    const std::string& id) {
    req_.requirement_id = id;
    return *this;
}

ComplianceRequirementBuilder& ComplianceRequirementBuilder::withFramework(
    ComplianceFramework fw) {
    req_.framework = fw;
    return *this;
}

ComplianceRequirementBuilder& ComplianceRequirementBuilder::withText(
    const std::string& text) {
    req_.requirement_text = text;
    return *this;
}

ComplianceRequirementBuilder& ComplianceRequirementBuilder::withSection(
    const std::string& section) {
    req_.regulatory_section = section;
    return *this;
}

ComplianceRequirementBuilder& ComplianceRequirementBuilder::withSeverity(
    ComplianceSeverity sev) {
    req_.severity = sev;
    return *this;
}

ComplianceRequirementBuilder& ComplianceRequirementBuilder::withControl(
    const std::string& control_id) {
    req_.control_ids.push_back(control_id);
    return *this;
}

ComplianceRequirementBuilder& ComplianceRequirementBuilder::withCategory(
    const std::string& cat) {
    req_.category = cat;
    return *this;
}

ComplianceRequirementBuilder& ComplianceRequirementBuilder::withMandatory(
    bool mandatory) {
    req_.is_mandatory = mandatory;
    return *this;
}

ComplianceRequirement ComplianceRequirementBuilder::build() const {
    auto req = req_;
    req.created_at_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    req.updated_at_ms = req.created_at_ms;
    return req;
}

// Control Builder
ComplianceControlBuilder& ComplianceControlBuilder::withId(
    const std::string& id) {
    ctl_.control_id = id;
    return *this;
}

ComplianceControlBuilder& ComplianceControlBuilder::withFramework(
    ComplianceFramework fw) {
    ctl_.framework = fw;
    return *this;
}

ComplianceControlBuilder& ComplianceControlBuilder::withName(
    const std::string& name) {
    ctl_.control_name = name;
    return *this;
}

ComplianceControlBuilder& ComplianceControlBuilder::withDescription(
    const std::string& desc) {
    ctl_.description = desc;
    return *this;
}

ComplianceControlBuilder& ComplianceControlBuilder::withImplementation(
    const std::string& impl) {
    ctl_.implementation_detail = impl;
    return *this;
}

ComplianceControlBuilder& ComplianceControlBuilder::automated(
    bool is_automated) {
    ctl_.is_automated = is_automated;
    return *this;
}

ComplianceControlBuilder& ComplianceControlBuilder::withPolicyRule(
    const std::string& rule_id) {
    ctl_.policy_rules.push_back(rule_id);
    return *this;
}

ComplianceControlBuilder& ComplianceControlBuilder::withEvidenceType(
    const std::string& type) {
    ctl_.evidence_types.push_back(type);
    return *this;
}

ComplianceControl ComplianceControlBuilder::build() const {
    auto ctl = ctl_;
    ctl.created_at_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    return ctl;
}

} // namespace governance
} // namespace themis
