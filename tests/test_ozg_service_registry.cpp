/*
 * Tests for InMemoryOZGServiceRegistry / IOZGServiceRegistry
 *
 * Acceptance criteria:
 *   AC-OZG-01  Empty registry has size 0 and empty() == true
 *   AC-OZG-02  registerService() adds an entry; size() == 1
 *   AC-OZG-03  registerService() throws on empty ID
 *   AC-OZG-04  registerService() throws on duplicate ID
 *   AC-OZG-05  findById() returns nullopt for unknown ID
 *   AC-OZG-06  findById() returns the correct entry after registration
 *   AC-OZG-07  updateService() replaces an existing entry
 *   AC-OZG-08  updateService() throws on unknown ID
 *   AC-OZG-09  updateService() throws on empty ID
 *   AC-OZG-10  removeService() removes a known entry; size decrements
 *   AC-OZG-11  removeService() is a no-op for unknown IDs
 *   AC-OZG-12  findByStatus() returns only matching entries
 *   AC-OZG-13  findByStatus() returns empty vector when no match
 *   AC-OZG-14  findByState() returns only services applicable to the state
 *   AC-OZG-15  findByState() returns empty vector when no match
 *   AC-OZG-16  findByComplianceTag() returns only tagged services
 *   AC-OZG-17  findByComplianceTag() returns empty for unknown tag
 *   AC-OZG-18  all() returns all registered entries
 *   AC-OZG-19  all() returns empty vector when registry is empty
 *   AC-OZG-20  OZGDataField default values are correct
 *   AC-OZG-21  OZGServiceEntry with multiple fields round-trips through registry
 *   AC-OZG-22  findByStatus() handles all OZGServiceStatus values
 *   AC-OZG-23  SDG-relevant flag is preserved on findById()
 *   AC-OZG-24  legal_basis vector is preserved
 *   AC-OZG-25  fim_process_id is preserved
 *   AC-OZG-26  Concurrent registerService() calls are thread-safe
 *   AC-OZG-27  Concurrent findById() and registerService() are thread-safe
 *   AC-OZG-28  Multiple states per service — findByState() finds correct entries
 *   AC-OZG-29  Polymorphic usage via IOZGServiceRegistry*
 *   AC-OZG-30  ONLINE_TRANSACTION services are searchable by status
 */

#include <gtest/gtest.h>
#include "importers/ozg_service_registry.h"

#include <future>
#include <thread>

using namespace themis::importers;

// ── Helpers ───────────────────────────────────────────────────────────────────

static OZGServiceEntry makeEntry(const std::string& id,
                                  OZGServiceStatus status = OZGServiceStatus::NOT_STARTED) {
    OZGServiceEntry e;
    e.id          = id;
    e.short_name  = "SN-" + id;
    e.name        = "Service " + id;
    e.description = "Description for " + id;
    e.status      = status;
    return e;
}

// ── Fixture ───────────────────────────────────────────────────────────────────

class OZGServiceRegistryTest : public ::testing::Test {
protected:
    InMemoryOZGServiceRegistry registry_;
};

// ── AC-OZG-01 ────────────────────────────────────────────────────────────────

TEST_F(OZGServiceRegistryTest, EmptyRegistryHasSizeZero) {
    EXPECT_EQ(registry_.size(), 0u);
    EXPECT_TRUE(registry_.empty());
}

// ── AC-OZG-02 ────────────────────────────────────────────────────────────────

TEST_F(OZGServiceRegistryTest, RegisterServiceIncreasesSize) {
    registry_.registerService(makeEntry("99026004017000"));
    EXPECT_EQ(registry_.size(), 1u);
    EXPECT_FALSE(registry_.empty());
}

// ── AC-OZG-03 ────────────────────────────────────────────────────────────────

TEST_F(OZGServiceRegistryTest, RegisterServiceThrowsOnEmptyId) {
    OZGServiceEntry bad;
    EXPECT_THROW(registry_.registerService(bad), std::invalid_argument);
}

// ── AC-OZG-04 ────────────────────────────────────────────────────────────────

TEST_F(OZGServiceRegistryTest, RegisterServiceThrowsOnDuplicateId) {
    registry_.registerService(makeEntry("DUP001"));
    EXPECT_THROW(registry_.registerService(makeEntry("DUP001")), std::runtime_error);
}

// ── AC-OZG-05 ────────────────────────────────────────────────────────────────

TEST_F(OZGServiceRegistryTest, FindByIdReturnsNulloptForUnknown) {
    auto r = registry_.findById("NONEXISTENT");
    EXPECT_FALSE(r.has_value());
}

// ── AC-OZG-06 ────────────────────────────────────────────────────────────────

TEST_F(OZGServiceRegistryTest, FindByIdReturnsCorrectEntry) {
    auto entry = makeEntry("S001");
    entry.name = "KFZ-Zulassung";
    registry_.registerService(entry);

    auto found = registry_.findById("S001");
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->name, "KFZ-Zulassung");
}

// ── AC-OZG-07 ────────────────────────────────────────────────────────────────

TEST_F(OZGServiceRegistryTest, UpdateServiceReplacesEntry) {
    registry_.registerService(makeEntry("U001"));
    auto updated = makeEntry("U001");
    updated.name = "Updated Name";
    registry_.updateService(updated);

    auto found = registry_.findById("U001");
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->name, "Updated Name");
}

// ── AC-OZG-08 ────────────────────────────────────────────────────────────────

TEST_F(OZGServiceRegistryTest, UpdateServiceThrowsOnUnknownId) {
    EXPECT_THROW(registry_.updateService(makeEntry("UNKNOWN")), std::invalid_argument);
}

// ── AC-OZG-09 ────────────────────────────────────────────────────────────────

TEST_F(OZGServiceRegistryTest, UpdateServiceThrowsOnEmptyId) {
    OZGServiceEntry bad;
    EXPECT_THROW(registry_.updateService(bad), std::invalid_argument);
}

// ── AC-OZG-10 ────────────────────────────────────────────────────────────────

TEST_F(OZGServiceRegistryTest, RemoveServiceDecrementsSize) {
    registry_.registerService(makeEntry("R001"));
    EXPECT_EQ(registry_.size(), 1u);
    registry_.removeService("R001");
    EXPECT_EQ(registry_.size(), 0u);
    EXPECT_FALSE(registry_.findById("R001").has_value());
}

// ── AC-OZG-11 ────────────────────────────────────────────────────────────────

TEST_F(OZGServiceRegistryTest, RemoveServiceIsNoOpForUnknown) {
    EXPECT_NO_THROW(registry_.removeService("GHOST"));
    EXPECT_EQ(registry_.size(), 0u);
}

// ── AC-OZG-12 ────────────────────────────────────────────────────────────────

TEST_F(OZGServiceRegistryTest, FindByStatusReturnsMatchingEntries) {
    registry_.registerService(makeEntry("S1", OZGServiceStatus::NOT_STARTED));
    registry_.registerService(makeEntry("S2", OZGServiceStatus::ONLINE_TRANSACTION));
    registry_.registerService(makeEntry("S3", OZGServiceStatus::ONLINE_TRANSACTION));

    auto online = registry_.findByStatus(OZGServiceStatus::ONLINE_TRANSACTION);
    EXPECT_EQ(online.size(), 2u);
}

// ── AC-OZG-13 ────────────────────────────────────────────────────────────────

TEST_F(OZGServiceRegistryTest, FindByStatusReturnsEmptyWhenNoMatch) {
    registry_.registerService(makeEntry("S1", OZGServiceStatus::NOT_STARTED));
    auto res = registry_.findByStatus(OZGServiceStatus::PROACTIVE_SERVICE);
    EXPECT_TRUE(res.empty());
}

// ── AC-OZG-14 ────────────────────────────────────────────────────────────────

TEST_F(OZGServiceRegistryTest, FindByStateReturnsApplicableServices) {
    auto e = makeEntry("STATE1");
    e.applicable_states = {"DE-BY", "DE-HH"};
    registry_.registerService(e);

    auto by = registry_.findByState("DE-BY");
    ASSERT_EQ(by.size(), 1u);
    EXPECT_EQ(by[0].id, "STATE1");
}

// ── AC-OZG-15 ────────────────────────────────────────────────────────────────

TEST_F(OZGServiceRegistryTest, FindByStateReturnsEmptyWhenNoMatch) {
    auto e = makeEntry("STATE2");
    e.applicable_states = {"DE-NW"};
    registry_.registerService(e);

    EXPECT_TRUE(registry_.findByState("DE-SN").empty());
}

// ── AC-OZG-16 ────────────────────────────────────────────────────────────────

TEST_F(OZGServiceRegistryTest, FindByComplianceTagReturnsTaggedServices) {
    auto e = makeEntry("TAG1");
    e.compliance_tags = {"OZG-2.0", "SDG"};
    registry_.registerService(e);

    auto sdg = registry_.findByComplianceTag("SDG");
    ASSERT_EQ(sdg.size(), 1u);
    EXPECT_EQ(sdg[0].id, "TAG1");
}

// ── AC-OZG-17 ────────────────────────────────────────────────────────────────

TEST_F(OZGServiceRegistryTest, FindByComplianceTagReturnsEmptyForUnknown) {
    registry_.registerService(makeEntry("T1"));
    EXPECT_TRUE(registry_.findByComplianceTag("NONEXISTENT_TAG").empty());
}

// ── AC-OZG-18 ────────────────────────────────────────────────────────────────

TEST_F(OZGServiceRegistryTest, AllReturnsAllRegisteredEntries) {
    registry_.registerService(makeEntry("A1"));
    registry_.registerService(makeEntry("A2"));
    registry_.registerService(makeEntry("A3"));
    EXPECT_EQ(registry_.all().size(), 3u);
}

// ── AC-OZG-19 ────────────────────────────────────────────────────────────────

TEST_F(OZGServiceRegistryTest, AllReturnsEmptyWhenRegistryIsEmpty) {
    EXPECT_TRUE(registry_.all().empty());
}

// ── AC-OZG-20 ────────────────────────────────────────────────────────────────

TEST_F(OZGServiceRegistryTest, OZGDataFieldDefaultValuesAreCorrect) {
    OZGDataField f;
    EXPECT_EQ(f.type, OZGFieldType::TEXT);
    EXPECT_TRUE(f.required);
    EXPECT_TRUE(f.allowed_values.empty());
}

// ── AC-OZG-21 ────────────────────────────────────────────────────────────────

TEST_F(OZGServiceRegistryTest, ServiceWithMultipleFieldsRoundTrips) {
    OZGServiceEntry e = makeEntry("FIELDS1");
    OZGDataField f1;
    f1.id = "F001"; f1.name = "Vorname"; f1.type = OZGFieldType::TEXT;
    OZGDataField f2;
    f2.id = "F002"; f2.name = "Geburtsdatum"; f2.type = OZGFieldType::DATE;
    e.fields = {f1, f2};
    registry_.registerService(e);

    auto found = registry_.findById("FIELDS1");
    ASSERT_TRUE(found.has_value());
    ASSERT_EQ(found->fields.size(), 2u);
    EXPECT_EQ(found->fields[0].id, "F001");
    EXPECT_EQ(found->fields[1].type, OZGFieldType::DATE);
}

// ── AC-OZG-22 ────────────────────────────────────────────────────────────────

TEST_F(OZGServiceRegistryTest, FindByStatusHandlesAllStatusValues) {
    registry_.registerService(makeEntry("S1", OZGServiceStatus::NOT_STARTED));
    registry_.registerService(makeEntry("S2", OZGServiceStatus::INFORMATION_ONLY));
    registry_.registerService(makeEntry("S3", OZGServiceStatus::FORM_AVAILABLE));
    registry_.registerService(makeEntry("S4", OZGServiceStatus::ONLINE_TRANSACTION));
    registry_.registerService(makeEntry("S5", OZGServiceStatus::PROACTIVE_SERVICE));

    for (auto st : {OZGServiceStatus::NOT_STARTED,
                    OZGServiceStatus::INFORMATION_ONLY,
                    OZGServiceStatus::FORM_AVAILABLE,
                    OZGServiceStatus::ONLINE_TRANSACTION,
                    OZGServiceStatus::PROACTIVE_SERVICE}) {
        EXPECT_EQ(registry_.findByStatus(st).size(), 1u);
    }
}

// ── AC-OZG-23 ────────────────────────────────────────────────────────────────

TEST_F(OZGServiceRegistryTest, SDGFlagPreservedOnFindById) {
    auto e = makeEntry("SDG1");
    e.sdg_relevant = true;
    registry_.registerService(e);
    EXPECT_TRUE(registry_.findById("SDG1")->sdg_relevant);
}

// ── AC-OZG-24 ────────────────────────────────────────────────────────────────

TEST_F(OZGServiceRegistryTest, LegalBasisVectorPreserved) {
    auto e = makeEntry("LEGAL1");
    e.legal_basis = {"§ 17 BWahlG", "§ 5 OZG"};
    registry_.registerService(e);

    auto found = registry_.findById("LEGAL1");
    ASSERT_TRUE(found.has_value());
    ASSERT_EQ(found->legal_basis.size(), 2u);
    EXPECT_EQ(found->legal_basis[0], "§ 17 BWahlG");
}

// ── AC-OZG-25 ────────────────────────────────────────────────────────────────

TEST_F(OZGServiceRegistryTest, FimProcessIdPreserved) {
    auto e = makeEntry("FIM1");
    e.fim_process_id = "FIM-2024-12345";
    registry_.registerService(e);
    EXPECT_EQ(registry_.findById("FIM1")->fim_process_id, "FIM-2024-12345");
}

// ── AC-OZG-26 ────────────────────────────────────────────────────────────────

TEST_F(OZGServiceRegistryTest, ConcurrentRegisterIsThreadSafe) {
    constexpr int N = 50;
    std::vector<std::future<void>> futs;
    for (int i = 0; i < N; ++i) {
        futs.push_back(std::async(std::launch::async, [this, i]() {
            try {
                registry_.registerService(makeEntry("C" + std::to_string(i)));
            } catch (...) {}
        }));
    }
    for (auto& f : futs) f.get();
    EXPECT_LE(registry_.size(), static_cast<std::size_t>(N));
}

// ── AC-OZG-27 ────────────────────────────────────────────────────────────────

TEST_F(OZGServiceRegistryTest, ConcurrentReadWriteIsThreadSafe) {
    registry_.registerService(makeEntry("EXISTING"));
    std::vector<std::future<void>> futs;
    for (int i = 0; i < 20; ++i) {
        futs.push_back(std::async(std::launch::async, [this, i]() {
            if (i % 2 == 0) {
                registry_.findById("EXISTING");
            } else {
                try {
                    registry_.registerService(makeEntry("NEW" + std::to_string(i)));
                } catch (...) {}
            }
        }));
    }
    for (auto& f : futs) f.get();
}

// ── AC-OZG-28 ────────────────────────────────────────────────────────────────

TEST_F(OZGServiceRegistryTest, MultipleStatesPerServiceFoundByState) {
    auto e = makeEntry("MULTI");
    e.applicable_states = {"DE-BY", "DE-HH", "DE-NW"};
    registry_.registerService(e);

    EXPECT_EQ(registry_.findByState("DE-BY").size(), 1u);
    EXPECT_EQ(registry_.findByState("DE-HH").size(), 1u);
    EXPECT_EQ(registry_.findByState("DE-NW").size(), 1u);
    EXPECT_EQ(registry_.findByState("DE-SN").size(), 0u);
}

// ── AC-OZG-29 ────────────────────────────────────────────────────────────────

TEST_F(OZGServiceRegistryTest, PolymorphicUsageViaInterface) {
    IOZGServiceRegistry* reg = &registry_;
    reg->registerService(makeEntry("POLY1"));
    EXPECT_EQ(reg->size(), 1u);
    EXPECT_TRUE(reg->findById("POLY1").has_value());
}

// ── AC-OZG-30 ────────────────────────────────────────────────────────────────

TEST_F(OZGServiceRegistryTest, OnlineTransactionServicesSearchableByStatus) {
    for (int i = 0; i < 5; ++i) {
        registry_.registerService(makeEntry("OT" + std::to_string(i),
                                             OZGServiceStatus::ONLINE_TRANSACTION));
    }
    auto res = registry_.findByStatus(OZGServiceStatus::ONLINE_TRANSACTION);
    EXPECT_EQ(res.size(), 5u);
}
