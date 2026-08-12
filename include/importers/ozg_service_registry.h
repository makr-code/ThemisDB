/**
 * @file ozg_service_registry.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB — OZG Service Schema Registry
 *
 * Registry for Onlinezugangsgesetz (OZG) service schemas, providing
 * structured access to the German federal OZG service catalog
 * (Leistungskatalog). Each entry describes a digitised administrative
 * service (Verwaltungsleistung) with its required data-fields, legal
 * basis, responsible authority type, and compliance tags.
 *
 * Implementations shipped in this header-only file:
 *   - OZGServiceEntry        — value type for a single service record
 *   - IOZGServiceRegistry    — abstract registry interface
 *   - InMemoryOZGServiceRegistry — thread-safe in-memory registry
 *
 * Standards references:
 *   - OZG 2.0 (2024), BGBl. I — Onlinezugangsgesetz (Neufassung)
 *   - FITKO OZG-Umsetzungskatalog 2024
 *   - EU Single Digital Gateway Regulation (SDGR) 2018/1724
 *   - FIM (Föderales Informationsmanagement) — FITKO, 2024
 *
 * Copyright (c) 2026 ThemisDB Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <algorithm>
#include <map>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace themis {
namespace importers {

// ── OZGServiceStatus ──────────────────────────────────────────────────────────

/**
 * @brief Digitisation maturity level of an OZG service.
 *
 * Mirrors the four-level OZG maturity model (Reifegrade) defined by the
 * IT-Planungsrat.
 */
enum class OZGServiceStatus {
    NOT_STARTED,        ///< Service not yet digitised
    INFORMATION_ONLY,   ///< Reifegard 1 – information available online
    FORM_AVAILABLE,     ///< Reifegrad 2 – downloadable / fillable form
    ONLINE_TRANSACTION, ///< Reifegrad 3 – fully online transaction
    PROACTIVE_SERVICE,  ///< Reifegrad 4 – proactive / once-only delivery
};

// ── OZGFederalLevel ───────────────────────────────────────────────────────────

/**
 * @brief Jurisdiction level responsible for providing the OZG service.
 */
enum class OZGFederalLevel {
    FEDERAL,    ///< Bundesbehörde (federal authority)
    STATE,      ///< Landesbehörde (state authority)
    MUNICIPAL,  ///< Kommunalbehörde (municipal authority)
    SUPRANATIONAL, ///< EU / supranational authority
};

// ── OZGFieldType ──────────────────────────────────────────────────────────────

/**
 * @brief Semantic type of a data field in an OZG service schema.
 *
 * Derived from FIM data-field catalogue (Datenfeldkatalog).
 */
enum class OZGFieldType {
    TEXT,           ///< Free-form text
    DATE,           ///< Calendar date (ISO 8601)
    BOOLEAN,        ///< Yes / No flag
    INTEGER,        ///< Whole number
    DECIMAL,        ///< Decimal number
    ENUM,           ///< Enumeration value (codelist)
    ADDRESS,        ///< Structured postal address (XMELD format)
    PERSON_NAME,    ///< Full name (given name + surname, XPERSONENSTAND)
    DOCUMENT_REF,   ///< Reference to an attached document
    IDENTIFIER,     ///< Unique identifier (e.g., Steuer-ID, Sozialversicherungsnummer)
};

// ── OZGDataField ─────────────────────────────────────────────────────────────

/**
 * @brief A single data field in an OZG service schema.
 *
 * Corresponds to a FIM Datenfeld (data field) with OZG-specific metadata.
 */
struct OZGDataField {
    std::string id;             ///< Unique field identifier (e.g. "F60000017")
    std::string name;           ///< Human-readable German label
    OZGFieldType type{OZGFieldType::TEXT}; ///< Semantic data type
    bool required{true};        ///< Whether the field is mandatory
    std::string description;    ///< Extended description / tooltip text
    std::string fim_reference;  ///< Reference to FIM data-field catalogue entry
    std::vector<std::string> allowed_values; ///< Non-empty for ENUM fields
};

// ── OZGServiceEntry ───────────────────────────────────────────────────────────

/**
 * @brief Describes a single OZG administrative service (Verwaltungsleistung).
 *
 * Derived from the FITKO OZG service catalogue.  Each entry uniquely
 * identifies a service, its legal foundation, the responsible authority
 * type, and the required data schema for a digital submission.
 */
struct OZGServiceEntry {
    // ── Identity ──────────────────────────────────────────────────────────────
    std::string id;                     ///< OZG service ID (e.g. "99026004017000")
    std::string short_name;             ///< Abbreviated service name
    std::string name;                   ///< Full German service name
    std::string description;            ///< Service description (plain text)

    // ── Jurisdiction ──────────────────────────────────────────────────────────
    OZGFederalLevel level{OZGFederalLevel::MUNICIPAL};
    // Legacy compatibility field used by older tests and fixtures.
    std::string responsible_authority;
    std::vector<std::string> applicable_states; ///< ISO 3166-2:DE state codes ("DE-BY", …)

    // ── Legal basis ───────────────────────────────────────────────────────────
    std::vector<std::string> legal_basis; ///< Statutes (§§ references), e.g. {"§ 17 Abs. 1 BWahlG"}
    std::vector<std::string> compliance_tags; ///< Semantic tags for compliance queries

    // ── Digitisation status ───────────────────────────────────────────────────
    OZGServiceStatus status{OZGServiceStatus::NOT_STARTED};
    bool sdg_relevant{false};           ///< Subject to EU Single Digital Gateway Regulation

    // ── Data schema ───────────────────────────────────────────────────────────
    std::vector<OZGDataField> fields;   ///< Required input fields for this service

    // ── Catalogue metadata ────────────────────────────────────────────────────
    std::string fim_process_id;         ///< FIM Prozess-ID from the FIM library
    std::string catalog_version;        ///< OZG catalogue version this entry was sourced from
};

// ── IOZGServiceRegistry ───────────────────────────────────────────────────────

/**
 * @brief Abstract interface for the OZG service schema registry.
 *
 * Implementations MUST be thread-safe.
 */
class IOZGServiceRegistry {
public:
    virtual ~IOZGServiceRegistry() = default;

    /**
     * @brief Register a new OZG service entry.
     *
     * @throws std::invalid_argument  if entry.id is empty.
     * @throws std::runtime_error     if a service with the same ID is already
     *                                registered.
     */
    virtual void registerService(const OZGServiceEntry& entry) = 0;

    /**
     * @brief Update an existing service entry (replace by ID).
     *
     * @throws std::invalid_argument  if entry.id is empty or the ID is not
     *                                registered.
     */
    virtual void updateService(const OZGServiceEntry& entry) = 0;

    /**
     * @brief Remove a service entry by ID.  No-op if ID is not registered.
     */
    virtual void removeService(std::string_view id) = 0;

    /**
     * @brief Look up a service by its OZG service ID.
     *
     * @return The matching entry, or std::nullopt if not found.
     */
    virtual std::optional<OZGServiceEntry> findById(std::string_view id) const = 0;

    /**
     * @brief Return all services whose status matches @p status.
     */
    virtual std::vector<OZGServiceEntry>
    findByStatus(OZGServiceStatus status) const = 0;

    /**
     * @brief Return all services applicable to a given federal state.
     *
     * @param state_code  ISO 3166-2:DE code, e.g. "DE-BY".
     */
    virtual std::vector<OZGServiceEntry>
    findByState(std::string_view state_code) const = 0;

    /**
     * @brief Return all services that carry the given compliance tag.
     */
    virtual std::vector<OZGServiceEntry>
    findByComplianceTag(std::string_view tag) const = 0;

    /**
     * @brief Return all registered services.
     */
    virtual std::vector<OZGServiceEntry> all() const = 0;

    /**
     * @brief Return the total number of registered services.
     */
    virtual std::size_t size() const = 0;

    /**
     * @brief Return true if the registry contains no entries.
     */
    virtual bool empty() const = 0;
};

// ── InMemoryOZGServiceRegistry ────────────────────────────────────────────────

/**
 * @brief Thread-safe in-memory implementation of IOZGServiceRegistry.
 *
 * Stores entries in a std::map keyed by OZG service ID.  All operations
 * acquire the same mutex; read-heavy workloads should wrap this with a
 * read-copy-update strategy if hot-path latency becomes a concern.
 */
class InMemoryOZGServiceRegistry : public IOZGServiceRegistry {
public:
    // ── Mutation ──────────────────────────────────────────────────────────────

    void registerService(const OZGServiceEntry& entry) override {
        if (entry.id.empty()) {
            throw std::invalid_argument("OZGServiceEntry::id must not be empty");
        }
        std::unique_lock<std::mutex> lk(mutex_);
        if (entries_.count(entry.id)) {
            throw std::runtime_error("OZG service already registered: " + entry.id);
        }
        entries_[entry.id] = entry;
    }

    void updateService(const OZGServiceEntry& entry) override {
        if (entry.id.empty()) {
            throw std::invalid_argument("OZGServiceEntry::id must not be empty");
        }
        std::unique_lock<std::mutex> lk(mutex_);
        if (!entries_.count(entry.id)) {
            throw std::invalid_argument("OZG service not registered: " + entry.id);
        }
        entries_[entry.id] = entry;
    }

    void removeService(std::string_view id) override {
        std::unique_lock<std::mutex> lk(mutex_);
        entries_.erase(std::string(id));
    }

    // ── Queries ───────────────────────────────────────────────────────────────

    std::optional<OZGServiceEntry> findById(std::string_view id) const override {
        std::unique_lock<std::mutex> lk(mutex_);
        auto it = entries_.find(std::string(id));
        if (it == entries_.end()) return std::nullopt;
        return it->second;
    }

    std::vector<OZGServiceEntry> findByStatus(OZGServiceStatus status) const override {
        std::unique_lock<std::mutex> lk(mutex_);
        std::vector<OZGServiceEntry> result;
        for (const auto& [id, e] : entries_) {
            if (e.status == status) result.push_back(e);
        }
        return result;
    }

    std::vector<OZGServiceEntry> findByState(std::string_view state_code) const override {
        std::unique_lock<std::mutex> lk(mutex_);
        std::vector<OZGServiceEntry> result;
        const std::string sc(state_code);
        for (const auto& [id, e] : entries_) {
            for (const auto& s : e.applicable_states) {
                if (s == sc) { result.push_back(e); break; }
            }
        }
        return result;
    }

    std::vector<OZGServiceEntry> findByComplianceTag(std::string_view tag) const override {
        std::unique_lock<std::mutex> lk(mutex_);
        std::vector<OZGServiceEntry> result;
        const std::string t(tag);
        for (const auto& [id, e] : entries_) {
            for (const auto& ct : e.compliance_tags) {
                if (ct == t) { result.push_back(e); break; }
            }
        }
        return result;
    }

    std::vector<OZGServiceEntry> all() const override {
        std::unique_lock<std::mutex> lk(mutex_);
        std::vector<OZGServiceEntry> result;
        result.reserve(entries_.size());
        for (const auto& [id, e] : entries_) result.push_back(e);
        return result;
    }

    std::size_t size() const override {
        std::unique_lock<std::mutex> lk(mutex_);
        return entries_.size();
    }

    bool empty() const override {
        std::unique_lock<std::mutex> lk(mutex_);
        return entries_.empty();
    }

private:
    mutable std::mutex mutex_;
    std::map<std::string, OZGServiceEntry> entries_;
};

} // namespace importers
} // namespace themis
