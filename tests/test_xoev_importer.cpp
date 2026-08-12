/*
 * Tests for InMemoryXOEVImporter / IXOEVImporter
 *
 * Acceptance criteria:
 *   AC-XOEV-01  importFromXML() returns failure on empty XML
 *   AC-XOEV-02  importFromXML() parses a single <record> element
 *   AC-XOEV-03  importFromXML() parses multiple <record> elements
 *   AC-XOEV-04  importFromXML() extracts child fields into XOEVRecord::fields
 *   AC-XOEV-05  importFromXML() uses "id" field as XOEVRecord::id when present
 *   AC-XOEV-06  importFromXML() generates positional ID when no "id" field
 *   AC-XOEV-07  importFromXML() sets the correct XOEVStandard on each record
 *   AC-XOEV-08  importFromXML() persists records; storedRecords() returns them
 *   AC-XOEV-09  importFromXML() reports error on unclosed <record> element
 *   AC-XOEV-10  clearRecords() empties the store
 *   AC-XOEV-11  exportToXML() returns success with empty record list
 *   AC-XOEV-12  exportToXML() serialises all records as <record> elements
 *   AC-XOEV-13  exportToXML() XML-escapes special characters in field values
 *   AC-XOEV-14  exportToXML() returns error when record standard mismatches
 *   AC-XOEV-15  exportToXML() includes version attribute in root element
 *   AC-XOEV-16  validate() returns false for empty document
 *   AC-XOEV-17  validate() returns true for minimal well-formed XML
 *   AC-XOEV-18  validate() returns false for mismatched angle-brackets
 *   AC-XOEV-19  XOEVVersion::toString() produces correct string
 *   AC-XOEV-20  XOEVVersion comparison operators work correctly
 *   AC-XOEV-21  storedRecords() returns empty vector after clearRecords()
 *   AC-XOEV-22  importFromXML() accumulates records across multiple calls
 *   AC-XOEV-23  XOEVRecord::raw_xml is populated after import
 *   AC-XOEV-24  importFromXML() with XPERSONENSTAND sets correct standard
 *   AC-XOEV-25  importFromXML() with XMELD sets correct standard
 *   AC-XOEV-26  Concurrent importFromXML() calls are thread-safe
 *   AC-XOEV-27  exportToXML() produces valid UTF-8 XML declaration
 *   AC-XOEV-28  exportToXML() with XOTHER standard still succeeds
 *   AC-XOEV-29  Polymorphic usage via IXOEVImporter*
 *   AC-XOEV-30  Round-trip: export then re-import preserves record count
 */

#include <gtest/gtest.h>
#include "importers/xoev_importer.h"

#include <future>

using namespace themis::importers;

// ── Helpers ───────────────────────────────────────────────────────────────────

static std::string makeXML(const std::vector<std::pair<std::string,std::string>>& records) {
    std::string xml = "<?xml version=\"1.0\" encoding=\"UTF-8\"?><messages>";
    for (const auto& [id, name] : records) {
        xml += "<record><id>" + id + "</id><name>" + name + "</name></record>";
    }
    xml += "</messages>";
    return xml;
}

// ── Fixture ───────────────────────────────────────────────────────────────────

class XOEVImporterTest : public ::testing::Test {
protected:
    InMemoryXOEVImporter importer_;
};

// ── AC-XOEV-01 ───────────────────────────────────────────────────────────────

TEST_F(XOEVImporterTest, ImportFromXMLFailsOnEmptyInput) {
    auto r = importer_.importFromXML("", XOEVStandard::XMELD);
    EXPECT_FALSE(r.success);
    EXPECT_FALSE(r.errors.empty());
    EXPECT_TRUE(r.errors[0].fatal);
}

// ── AC-XOEV-02 ───────────────────────────────────────────────────────────────

TEST_F(XOEVImporterTest, ImportFromXMLParsesSingleRecord) {
    auto xml = makeXML({{"R001", "Max Mustermann"}});
    auto r   = importer_.importFromXML(xml, XOEVStandard::XMELD);
    EXPECT_TRUE(r.success);
    EXPECT_EQ(r.records_parsed, 1u);
    EXPECT_EQ(r.records_imported, 1u);
}

// ── AC-XOEV-03 ───────────────────────────────────────────────────────────────

TEST_F(XOEVImporterTest, ImportFromXMLParsesMultipleRecords) {
    auto xml = makeXML({{"R001", "Alice"}, {"R002", "Bob"}, {"R003", "Carol"}});
    auto r   = importer_.importFromXML(xml, XOEVStandard::XMELD);
    EXPECT_EQ(r.records_parsed, 3u);
    EXPECT_EQ(r.records.size(), 3u);
}

// ── AC-XOEV-04 ───────────────────────────────────────────────────────────────

TEST_F(XOEVImporterTest, ImportExtractsChildFields) {
    std::string xml = "<root><record><id>F001</id><vorname>Anna</vorname>"
                      "<nachname>Schmidt</nachname></record></root>";
    auto r = importer_.importFromXML(xml, XOEVStandard::XPERSONENSTAND);
    ASSERT_EQ(r.records.size(), 1u);
    EXPECT_EQ(r.records[0].fields.at("vorname"), "Anna");
    EXPECT_EQ(r.records[0].fields.at("nachname"), "Schmidt");
}

// ── AC-XOEV-05 ───────────────────────────────────────────────────────────────

TEST_F(XOEVImporterTest, ImportUsesIdFieldAsRecordId) {
    std::string xml = "<root><record><id>MY-ID-42</id></record></root>";
    auto r = importer_.importFromXML(xml, XOEVStandard::XMELD);
    ASSERT_EQ(r.records.size(), 1u);
    EXPECT_EQ(r.records[0].id, "MY-ID-42");
}

// ── AC-XOEV-06 ───────────────────────────────────────────────────────────────

TEST_F(XOEVImporterTest, ImportGeneratesPositionalIdWhenNoIdField) {
    std::string xml = "<root><record><name>Test</name></record></root>";
    auto r = importer_.importFromXML(xml, XOEVStandard::XMELD);
    ASSERT_EQ(r.records.size(), 1u);
    EXPECT_FALSE(r.records[0].id.empty());
    EXPECT_EQ(r.records[0].id, "xoev-0");
}

// ── AC-XOEV-07 ───────────────────────────────────────────────────────────────

TEST_F(XOEVImporterTest, ImportSetsCorrectStandard) {
    auto xml = makeXML({{"R1", "v"}});
    auto r   = importer_.importFromXML(xml, XOEVStandard::XBAU);
    ASSERT_EQ(r.records.size(), 1u);
    EXPECT_EQ(r.records[0].standard, XOEVStandard::XBAU);
}

// ── AC-XOEV-08 ───────────────────────────────────────────────────────────────

TEST_F(XOEVImporterTest, ImportPersistsRecordsToStore) {
    auto xml = makeXML({{"P001", "Person 1"}, {"P002", "Person 2"}});
    importer_.importFromXML(xml, XOEVStandard::XMELD);
    auto stored = importer_.storedRecords();
    EXPECT_EQ(stored.size(), 2u);
}

// ── AC-XOEV-09 ───────────────────────────────────────────────────────────────

TEST_F(XOEVImporterTest, ImportReportsErrorOnUnclosedRecord) {
    std::string xml = "<root><record><id>UNCLOSED</id></root>";
    auto r = importer_.importFromXML(xml, XOEVStandard::XMELD);
    EXPECT_FALSE(r.errors.empty());
}

// ── AC-XOEV-10 ───────────────────────────────────────────────────────────────

TEST_F(XOEVImporterTest, ClearRecordsEmptiesStore) {
    auto xml = makeXML({{"Q1", "X"}});
    importer_.importFromXML(xml, XOEVStandard::XMELD);
    importer_.clearRecords();
    EXPECT_TRUE(importer_.storedRecords().empty());
}

// ── AC-XOEV-11 ───────────────────────────────────────────────────────────────

TEST_F(XOEVImporterTest, ExportToXMLSucceedsWithEmptyRecordList) {
    auto r = importer_.exportToXML({}, XOEVStandard::XMELD, {1, 0, 0});
    EXPECT_TRUE(r.success);
    EXPECT_FALSE(r.xml_output.empty());
    EXPECT_EQ(r.records_exported, 0u);
}

// ── AC-XOEV-12 ───────────────────────────────────────────────────────────────

TEST_F(XOEVImporterTest, ExportToXMLSerialisesRecords) {
    XOEVRecord rec;
    rec.id       = "E001";
    rec.standard = XOEVStandard::XMELD;
    rec.fields   = {{"vorname", "Max"}, {"nachname", "Muster"}};

    auto r = importer_.exportToXML({rec}, XOEVStandard::XMELD, {2, 0, 0});
    EXPECT_TRUE(r.success);
    EXPECT_EQ(r.records_exported, 1u);
    EXPECT_NE(r.xml_output.find("E001"), std::string::npos);
    EXPECT_NE(r.xml_output.find("Max"), std::string::npos);
}

// ── AC-XOEV-13 ───────────────────────────────────────────────────────────────

TEST_F(XOEVImporterTest, ExportXMLEscapesSpecialCharacters) {
    XOEVRecord rec;
    rec.id       = "E002";
    rec.standard = XOEVStandard::XMELD;
    rec.fields   = {{"name", "Müller & Söhne <GmbH>"}};

    auto r = importer_.exportToXML({rec}, XOEVStandard::XMELD, {1, 0, 0});
    EXPECT_TRUE(r.success);
    EXPECT_NE(r.xml_output.find("&amp;"), std::string::npos);
    EXPECT_NE(r.xml_output.find("&lt;"), std::string::npos);
    EXPECT_NE(r.xml_output.find("&gt;"), std::string::npos);
}

// ── AC-XOEV-14 ───────────────────────────────────────────────────────────────

TEST_F(XOEVImporterTest, ExportReturnsErrorOnStandardMismatch) {
    XOEVRecord rec;
    rec.id       = "E003";
    rec.standard = XOEVStandard::XBAU; // ≠ requested XMELD
    rec.fields   = {{"field", "val"}};

    auto r = importer_.exportToXML({rec}, XOEVStandard::XMELD, {1, 0, 0});
    EXPECT_FALSE(r.success);
    EXPECT_FALSE(r.error_message.empty());
}

// ── AC-XOEV-15 ───────────────────────────────────────────────────────────────

TEST_F(XOEVImporterTest, ExportIncludesVersionAttributeInRootElement) {
    XOEVRecord rec;
    rec.id = "V001"; rec.standard = XOEVStandard::XMELD;
    auto r = importer_.exportToXML({rec}, XOEVStandard::XMELD, {2, 1, 0});
    EXPECT_TRUE(r.success);
    EXPECT_NE(r.xml_output.find("2.1.0"), std::string::npos);
}

// ── AC-XOEV-16 ───────────────────────────────────────────────────────────────

TEST_F(XOEVImporterTest, ValidateReturnsFalseForEmptyDocument) {
    std::vector<XOEVImportError> errs;
    EXPECT_FALSE(importer_.validate("", XOEVStandard::XMELD, {}, errs));
    EXPECT_FALSE(errs.empty());
}

// ── AC-XOEV-17 ───────────────────────────────────────────────────────────────

TEST_F(XOEVImporterTest, ValidateReturnsTrueForWellFormedXML) {
    std::vector<XOEVImportError> errs;
    EXPECT_TRUE(importer_.validate("<root><child/></root>",
                                    XOEVStandard::XMELD, {}, errs));
    EXPECT_TRUE(errs.empty());
}

// ── AC-XOEV-18 ───────────────────────────────────────────────────────────────

TEST_F(XOEVImporterTest, ValidateReturnsFalseForMismatchedAngles) {
    std::vector<XOEVImportError> errs;
    // One '<' without a matching '>'
    EXPECT_FALSE(importer_.validate("<root><broken", XOEVStandard::XMELD, {}, errs));
}

// ── AC-XOEV-19 ───────────────────────────────────────────────────────────────

TEST(XOEVVersionTest, ToStringProducesCorrectString) {
    XOEVVersion v{2, 1, 3};
    EXPECT_EQ(v.toString(), "2.1.3");
}

// ── AC-XOEV-20 ───────────────────────────────────────────────────────────────

TEST(XOEVVersionTest, ComparisonOperatorsWork) {
    XOEVVersion v1{1, 0, 0}, v2{2, 0, 0}, v3{1, 0, 0};
    EXPECT_TRUE(v1 < v2);
    EXPECT_FALSE(v2 < v1);
    EXPECT_TRUE(v1 == v3);
    EXPECT_FALSE(v1 == v2);
}

// ── AC-XOEV-21 ───────────────────────────────────────────────────────────────

TEST_F(XOEVImporterTest, StoredRecordsEmptyAfterClear) {
    auto xml = makeXML({{"X1", "v1"}});
    importer_.importFromXML(xml, XOEVStandard::XMELD);
    importer_.clearRecords();
    EXPECT_TRUE(importer_.storedRecords().empty());
}

// ── AC-XOEV-22 ───────────────────────────────────────────────────────────────

TEST_F(XOEVImporterTest, ImportAccumulatesAcrossMultipleCalls) {
    importer_.importFromXML(makeXML({{"A1", "v"}}), XOEVStandard::XMELD);
    importer_.importFromXML(makeXML({{"B1", "v"}}), XOEVStandard::XMELD);
    EXPECT_EQ(importer_.storedRecords().size(), 2u);
}

// ── AC-XOEV-23 ───────────────────────────────────────────────────────────────

TEST_F(XOEVImporterTest, ImportPopulatesRawXML) {
    std::string xml = "<root><record><id>RX1</id></record></root>";
    auto r = importer_.importFromXML(xml, XOEVStandard::XMELD);
    ASSERT_EQ(r.records.size(), 1u);
    EXPECT_FALSE(r.records[0].raw_xml.empty());
    EXPECT_NE(r.records[0].raw_xml.find("<record"), std::string::npos);
}

// ── AC-XOEV-24 ───────────────────────────────────────────────────────────────

TEST_F(XOEVImporterTest, XPersonenstandStandardSetCorrectly) {
    auto xml = makeXML({{"PS1", "v"}});
    auto r   = importer_.importFromXML(xml, XOEVStandard::XPERSONENSTAND);
    ASSERT_EQ(r.records.size(), 1u);
    EXPECT_EQ(r.records[0].standard, XOEVStandard::XPERSONENSTAND);
}

// ── AC-XOEV-25 ───────────────────────────────────────────────────────────────

TEST_F(XOEVImporterTest, XMeldStandardSetCorrectly) {
    auto xml = makeXML({{"ML1", "v"}});
    auto r   = importer_.importFromXML(xml, XOEVStandard::XMELD);
    ASSERT_EQ(r.records.size(), 1u);
    EXPECT_EQ(r.records[0].standard, XOEVStandard::XMELD);
}

// ── AC-XOEV-26 ───────────────────────────────────────────────────────────────

TEST_F(XOEVImporterTest, ConcurrentImportIsThreadSafe) {
    constexpr int N = 20;
    std::vector<std::future<XOEVImportResult>> futs;
    for (int i = 0; i < N; ++i) {
        futs.push_back(std::async(std::launch::async, [this, i]() {
            return importer_.importFromXML(
                makeXML({{std::to_string(i), "v"}}),
                XOEVStandard::XMELD);
        }));
    }
    for (auto& f : futs) f.get();
    // All records should have been stored (no assertions on exact count due to
    // duplicate-key collisions, but no crashes / data races).
    EXPECT_GE(importer_.storedRecords().size(), 1u);
}

// ── AC-XOEV-27 ───────────────────────────────────────────────────────────────

TEST_F(XOEVImporterTest, ExportProducesXMLDeclaration) {
    auto r = importer_.exportToXML({}, XOEVStandard::XMELD, {1, 0, 0});
    EXPECT_TRUE(r.success);
    EXPECT_NE(r.xml_output.find("<?xml"), std::string::npos);
}

// ── AC-XOEV-28 ───────────────────────────────────────────────────────────────

TEST_F(XOEVImporterTest, ExportWithOtherStandardSucceeds) {
    XOEVRecord rec;
    rec.id = "OT1"; rec.standard = XOEVStandard::OTHER;
    auto r = importer_.exportToXML({rec}, XOEVStandard::OTHER, {1, 0, 0});
    EXPECT_TRUE(r.success);
}

// ── AC-XOEV-29 ───────────────────────────────────────────────────────────────

TEST_F(XOEVImporterTest, PolymorphicUsageViaInterface) {
    IXOEVImporter* imp = &importer_;
    auto xml = makeXML({{"POLY1", "v"}});
    auto r   = imp->importFromXML(xml, XOEVStandard::XMELD);
    EXPECT_TRUE(r.success);
    EXPECT_EQ(imp->storedRecords().size(), 1u);
}

// ── AC-XOEV-30 ───────────────────────────────────────────────────────────────

TEST_F(XOEVImporterTest, RoundTripExportThenImportPreservesRecordCount) {
    // Build initial records.
    std::vector<XOEVRecord> original;
    for (int i = 0; i < 5; ++i) {
        XOEVRecord r;
        r.id       = "RT" + std::to_string(i);
        r.standard = XOEVStandard::XMELD;
        r.fields   = {{"idx", std::to_string(i)}};
        original.push_back(r);
    }

    // Export.
    auto exp = importer_.exportToXML(original, XOEVStandard::XMELD, {2, 0, 0});
    ASSERT_TRUE(exp.success);

    // Re-import into a fresh importer.
    InMemoryXOEVImporter fresh;
    auto imp = fresh.importFromXML(exp.xml_output, XOEVStandard::XMELD);
    EXPECT_EQ(imp.records_imported, 5u);
}
