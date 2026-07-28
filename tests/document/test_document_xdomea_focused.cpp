/**
 * @file test_document_xdomea_focused.cpp
 * @brief XDOMEA connector and multi-version document deterministic fixture tests.
 *
 * @details Covers three test categories addressing Phase 4 roadmap item:
 *  "extend deterministic fixture coverage for XDOMEA and multi-version document cases"
 *
 *  - XDOMEAStore      (XS-01 … XS-08): InMemoryXDOMEAConnector CRUD operations
 *  - XDOMEAImport     (XI-01 … XI-08): importFromXML V2.1 / V3.0 parsing edge cases
 *  - XDOMEAExport     (XE-01 … XE-06): exportToXML serialization and message types
 *  - XDOMEAMultiVersion (XMV-01 … XMV-04): multi-version document lifecycle with
 *                                           schema evolution registry
 *
 * All tests use deterministic fixtures with kXDOMEACanonicalSeed = 42 where
 * randomness is needed.  No external XML library dependency is assumed; the
 * InMemoryXDOMEAConnector's minimal scanner is exercised directly.
 *
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Target: Q4 2026 hardening sprint — Phase 4 XDOMEA closure
 * @version 1.0.0
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * ThemisDB — Document Module XDOMEA Tests
 *
 * File:    test_document_xdomea_focused.cpp
 * Module:  tests/document/
 * Purpose: Deterministic fixture coverage for InMemoryXDOMEAConnector and
 *          multi-version document lifecycle with InMemoryDocumentSchemaEvolution.
 *
 * Version: 1.0.0
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "document/document_schema_evolution.h"
#include "document/document_store.h"
#include "document/xdomea_connector.h"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

using namespace themis;
using namespace themis::document;

// ─────────────────────────────────────────────────────────────────────────────
// Constants
// ─────────────────────────────────────────────────────────────────────────────

namespace {

/// @brief Canonical PRNG seed for all XDOMEA deterministic fixtures.
static constexpr uint64_t kXDOMEACanonicalSeed = 42;

// ── Minimal XML fixtures ──────────────────────────────────────────────────────

/// @brief Single-document XDOMEA 3.0 XML fragment (deterministic).
static constexpr const char* kXmlV30SingleDoc = R"(
<?xml version="1.0" encoding="UTF-8"?>
<xdomea:Nachricht xmlns:xdomea="https://www.xdomea.de/ns/xdomea/" xdomea:version="3.0.0" xdomea:nachrichtentyp="0601">
  <dokument>
    <id>DOC-SEED-42-001</id>
    <aktenzeichen>AZ-2026-001</aktenzeichen>
    <betreff>Deterministic Fixture Document 1</betreff>
    <ersteller>ThemisDB Test Suite</ersteller>
    <dateiname>fixture_001.pdf</dateiname>
    <mimetyp>application/pdf</mimetyp>
  </dokument>
</xdomea:Nachricht>
)";

/// @brief Single-document XDOMEA 2.1 XML fragment (deterministic).
static constexpr const char* kXmlV21SingleDoc = R"(
<?xml version="1.0" encoding="UTF-8"?>
<xdomea:Nachricht xmlns:xdomea="https://www.xdomea.de/ns/xdomea/" xdomea:version="2.1.0" xdomea:nachrichtentyp="0601">
  <dokument>
    <id>DOC-SEED-42-002</id>
    <aktenzeichen>AZ-2026-002</aktenzeichen>
    <betreff>Deterministic Fixture Document 2 (v2.1)</betreff>
    <ersteller>ThemisDB Test Suite</ersteller>
    <dateiname>fixture_002.pdf</dateiname>
    <mimetyp>application/pdf</mimetyp>
  </dokument>
</xdomea:Nachricht>
)";

/// @brief Multi-document XML with one Akte and one Dokument.
static constexpr const char* kXmlMultiObjects = R"(
<?xml version="1.0" encoding="UTF-8"?>
<xdomea:Nachricht xmlns:xdomea="https://www.xdomea.de/ns/xdomea/" xdomea:version="3.0.0" xdomea:nachrichtentyp="0601">
  <akte>
    <id>AKTE-SEED-42-001</id>
    <aktenzeichen>AZ-2026-AKTE-001</aktenzeichen>
    <betreff>Test Akte 1</betreff>
  </akte>
  <dokument>
    <id>DOC-SEED-42-003</id>
    <aktenzeichen>AZ-2026-003</aktenzeichen>
    <betreff>Child Document of Akte 1</betreff>
    <ersteller>ThemisDB Test Suite</ersteller>
  </dokument>
</xdomea:Nachricht>
)";

/// @brief XML with an unclosed <dokument> tag — error path fixture.
static constexpr const char* kXmlUnclosedDokument = R"(
<xdomea:Nachricht>
  <dokument>
    <id>BROKEN-001</id>
    <betreff>Unclosed document
</xdomea:Nachricht>
)";

/// @brief XML with a special-character betreff requiring XML escaping.
static constexpr const char* kXmlSpecialChars = R"(
<xdomea:Nachricht>
  <dokument>
    <id>DOC-SPECIAL-001</id>
    <betreff>Title with &lt;XML&gt; &amp; &quot;quotes&quot;</betreff>
  </dokument>
</xdomea:Nachricht>
)";

// ── Helper: build a deterministic XDOMEADocument ─────────────────────────────

XDOMEADocument makeDeterministicDoc(const std::string& id,
                                    XDOMEAObjectType   type,
                                    XDOMEARetentionCategory retention =
                                        XDOMEARetentionCategory::UNGEKLAERT,
                                    const std::optional<std::string>& parent_id = std::nullopt)
{
    XDOMEADocument doc;
    doc.id             = id;
    doc.object_type    = type;
    doc.aktenzeichen   = "AZ-" + id;
    doc.betreff        = "Fixture: " + id;
    doc.xdomea_version = "3.0.0";
    doc.author         = "seed-42-author";
    doc.retention      = retention;
    doc.parent_id      = parent_id;
    return doc;
}

} // anonymous namespace

// ═════════════════════════════════════════════════════════════════════════════
// XDOMEAStore — CRUD and query operations (XS-01 … XS-08)
// ═════════════════════════════════════════════════════════════════════════════

class XDOMEAStoreTest : public ::testing::Test {
protected:
    InMemoryXDOMEAConnector connector_;
};

/**
 * @test XS-01: storeDocument and getDocument round-trip.
 *
 * Stores a deterministic document and retrieves it by ID, verifying identity
 * and field preservation.
 */
TEST_F(XDOMEAStoreTest, XS01_StoreAndGetRoundTrip)
{
    auto doc = makeDeterministicDoc("XS01-DOC-001", XDOMEAObjectType::DOKUMENT);
    ASSERT_NO_THROW(connector_.storeDocument(doc));

    auto retrieved = connector_.getDocument("XS01-DOC-001");
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->id,           "XS01-DOC-001");
    EXPECT_EQ(retrieved->aktenzeichen, "AZ-XS01-DOC-001");
    EXPECT_EQ(retrieved->betreff,      "Fixture: XS01-DOC-001");
    EXPECT_EQ(retrieved->author,       "seed-42-author");
    EXPECT_EQ(retrieved->xdomea_version, "3.0.0");
}

/**
 * @test XS-02: getDocument returns nullopt for unknown ID.
 *
 * Verifies that querying an ID that has never been stored returns
 * std::nullopt (not an error throw).
 */
TEST_F(XDOMEAStoreTest, XS02_GetUnknownIdReturnsNullopt)
{
    auto result = connector_.getDocument("NON-EXISTENT-ID");
    EXPECT_FALSE(result.has_value());
}

/**
 * @test XS-03: Duplicate storeDocument throws std::runtime_error.
 *
 * Verifies the duplicate-ID guard is enforced and throws the expected
 * exception type.
 */
TEST_F(XDOMEAStoreTest, XS03_DuplicateStoreThrows)
{
    auto doc = makeDeterministicDoc("XS03-DOC-001", XDOMEAObjectType::DOKUMENT);
    ASSERT_NO_THROW(connector_.storeDocument(doc));
    EXPECT_THROW(connector_.storeDocument(doc), std::runtime_error);
}

/**
 * @test XS-04: storeDocument with empty ID throws std::invalid_argument.
 *
 * Verifies the empty-ID guard is enforced before attempting storage.
 */
TEST_F(XDOMEAStoreTest, XS04_EmptyIdThrowsInvalidArgument)
{
    auto doc = makeDeterministicDoc("", XDOMEAObjectType::DOKUMENT);
    EXPECT_THROW(connector_.storeDocument(doc), std::invalid_argument);
}

/**
 * @test XS-05: removeDocument removes by ID; subsequent getDocument returns nullopt.
 *
 * Verifies clean removal and that removeDocument is a no-op for unknown IDs.
 */
TEST_F(XDOMEAStoreTest, XS05_RemoveDocumentAndVerifyGone)
{
    auto doc = makeDeterministicDoc("XS05-DOC-001", XDOMEAObjectType::AKTE);
    connector_.storeDocument(doc);
    EXPECT_EQ(connector_.count(), 1u);

    connector_.removeDocument("XS05-DOC-001");
    EXPECT_EQ(connector_.count(), 0u);
    EXPECT_FALSE(connector_.getDocument("XS05-DOC-001").has_value());

    // No-op for unknown ID — must not throw.
    ASSERT_NO_THROW(connector_.removeDocument("NON-EXISTENT"));
}

/**
 * @test XS-06: listByType returns only documents matching the requested type.
 *
 * Stores three documents with different object types and validates that
 * listByType returns exactly the matching subset.
 */
TEST_F(XDOMEAStoreTest, XS06_ListByTypeFiltersCorrectly)
{
    connector_.storeDocument(makeDeterministicDoc("XS06-AKTE-001",   XDOMEAObjectType::AKTE));
    connector_.storeDocument(makeDeterministicDoc("XS06-DOC-001",    XDOMEAObjectType::DOKUMENT));
    connector_.storeDocument(makeDeterministicDoc("XS06-VORGANG-001",XDOMEAObjectType::VORGANG));

    auto akten = connector_.listByType(XDOMEAObjectType::AKTE);
    ASSERT_EQ(akten.size(), 1u);
    EXPECT_EQ(akten[0].id, "XS06-AKTE-001");

    auto docs = connector_.listByType(XDOMEAObjectType::DOKUMENT);
    ASSERT_EQ(docs.size(), 1u);
    EXPECT_EQ(docs[0].id, "XS06-DOC-001");

    auto vorgaenge = connector_.listByType(XDOMEAObjectType::VORGANG);
    ASSERT_EQ(vorgaenge.size(), 1u);
    EXPECT_EQ(vorgaenge[0].id, "XS06-VORGANG-001");
}

/**
 * @test XS-07: listByRetention returns only documents with the matching category.
 *
 * Stores documents with all three retention categories and verifies that
 * listByRetention returns exactly the matching subset for each.
 */
TEST_F(XDOMEAStoreTest, XS07_ListByRetentionFiltersCorrectly)
{
    connector_.storeDocument(makeDeterministicDoc(
        "XS07-ARCH-001", XDOMEAObjectType::DOKUMENT,
        XDOMEARetentionCategory::ARCHIVWUERDIG));
    connector_.storeDocument(makeDeterministicDoc(
        "XS07-NARCH-001", XDOMEAObjectType::DOKUMENT,
        XDOMEARetentionCategory::NICHT_ARCHIVWUERDIG));
    connector_.storeDocument(makeDeterministicDoc(
        "XS07-UNK-001", XDOMEAObjectType::DOKUMENT,
        XDOMEARetentionCategory::UNGEKLAERT));

    EXPECT_EQ(connector_.listByRetention(XDOMEARetentionCategory::ARCHIVWUERDIG).size(), 1u);
    EXPECT_EQ(connector_.listByRetention(XDOMEARetentionCategory::NICHT_ARCHIVWUERDIG).size(), 1u);
    EXPECT_EQ(connector_.listByRetention(XDOMEARetentionCategory::UNGEKLAERT).size(), 1u);
}

/**
 * @test XS-08: listChildren returns only direct children of the given parent ID.
 *
 * Stores a parent Akte and two children (one with parent set, one without),
 * and verifies listChildren returns exactly the documents whose parent_id
 * matches.
 */
TEST_F(XDOMEAStoreTest, XS08_ListChildrenFiltersCorrectly)
{
    connector_.storeDocument(
        makeDeterministicDoc("XS08-AKTE-001", XDOMEAObjectType::AKTE));
    connector_.storeDocument(
        makeDeterministicDoc("XS08-DOC-CHILD-001", XDOMEAObjectType::DOKUMENT,
                             XDOMEARetentionCategory::UNGEKLAERT, "XS08-AKTE-001"));
    connector_.storeDocument(
        makeDeterministicDoc("XS08-DOC-CHILD-002", XDOMEAObjectType::DOKUMENT,
                             XDOMEARetentionCategory::UNGEKLAERT, "XS08-AKTE-001"));
    connector_.storeDocument(
        makeDeterministicDoc("XS08-DOC-ORPHAN-001", XDOMEAObjectType::DOKUMENT));

    auto children = connector_.listChildren("XS08-AKTE-001");
    ASSERT_EQ(children.size(), 2u);

    auto orphans = connector_.listChildren("XS08-DOC-ORPHAN-001");
    EXPECT_TRUE(orphans.empty());
}

// ═════════════════════════════════════════════════════════════════════════════
// XDOMEAImport — importFromXML parsing (XI-01 … XI-08)
// ═════════════════════════════════════════════════════════════════════════════

class XDOMEAImportTest : public ::testing::Test {
protected:
    InMemoryXDOMEAConnector connector_;
};

/**
 * @test XI-01: importFromXML V3.0 single-document success path.
 *
 * Verifies that a well-formed single-document V3.0 XML is parsed into exactly
 * one document with the correct ID, aktenzeichen, and version string.
 */
TEST_F(XDOMEAImportTest, XI01_V30SingleDocumentSuccess)
{
    auto result = connector_.importFromXML(kXmlV30SingleDoc, XDOMEAVersion::V3_0);

    EXPECT_TRUE(result.success);
    ASSERT_EQ(result.documents_imported, 1u);
    ASSERT_EQ(result.documents.size(), 1u);
    EXPECT_EQ(result.documents[0].id,             "DOC-SEED-42-001");
    EXPECT_EQ(result.documents[0].aktenzeichen,   "AZ-2026-001");
    EXPECT_EQ(result.documents[0].xdomea_version, "3.0.0");
    EXPECT_EQ(result.version,                     XDOMEAVersion::V3_0);
    EXPECT_TRUE(result.errors.empty());
}

/**
 * @test XI-02: importFromXML V2.1 single-document success path.
 *
 * Verifies that a well-formed V2.1 XML produces xdomea_version = "2.1.0".
 */
TEST_F(XDOMEAImportTest, XI02_V21SingleDocumentSuccess)
{
    auto result = connector_.importFromXML(kXmlV21SingleDoc, XDOMEAVersion::V2_1);

    EXPECT_TRUE(result.success);
    ASSERT_EQ(result.documents_imported, 1u);
    EXPECT_EQ(result.documents[0].id,             "DOC-SEED-42-002");
    EXPECT_EQ(result.documents[0].xdomea_version, "2.1.0");
    EXPECT_EQ(result.version,                     XDOMEAVersion::V2_1);
}

/**
 * @test XI-03: importFromXML parses both <akte> and <dokument> elements.
 *
 * A fixture with one <akte> and one <dokument> must produce two documents
 * with the correct object_type assignments.
 */
TEST_F(XDOMEAImportTest, XI03_MultiObjectTypeParsing)
{
    auto result = connector_.importFromXML(kXmlMultiObjects, XDOMEAVersion::V3_0);

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.documents.size(), 2u);

    bool foundAkte = false;
    bool foundDoc  = false;
    for (const auto& d : result.documents) {
        if (d.object_type == XDOMEAObjectType::AKTE)     foundAkte = true;
        if (d.object_type == XDOMEAObjectType::DOKUMENT) foundDoc  = true;
    }
    EXPECT_TRUE(foundAkte) << "Expected an AKTE object in parsed result";
    EXPECT_TRUE(foundDoc)  << "Expected a DOKUMENT object in parsed result";
}

/**
 * @test XI-04: importFromXML persists parsed documents into the connector's store.
 *
 * After a successful import, documents must be retrievable by ID via getDocument().
 */
TEST_F(XDOMEAImportTest, XI04_ImportedDocumentsPersisted)
{
    connector_.importFromXML(kXmlV30SingleDoc, XDOMEAVersion::V3_0);

    auto retrieved = connector_.getDocument("DOC-SEED-42-001");
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->id, "DOC-SEED-42-001");
}

/**
 * @test XI-05: importFromXML returns empty result for empty XML.
 *
 * An empty string must not crash; result must report success=false and
 * zero documents.
 */
TEST_F(XDOMEAImportTest, XI05_EmptyXmlReturnsEmptyResult)
{
    auto result = connector_.importFromXML("", XDOMEAVersion::V3_0);

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.documents_imported, 0u);
    EXPECT_FALSE(result.errors.empty());
}

/**
 * @test XI-06: importFromXML records an error for an unclosed <dokument> element.
 *
 * Unclosed element must not crash; result must contain at least one error
 * and no successfully imported documents.
 */
TEST_F(XDOMEAImportTest, XI06_UnclosedDokumentRecordsError)
{
    auto result = connector_.importFromXML(kXmlUnclosedDokument, XDOMEAVersion::V3_0);

    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.errors.empty());
    EXPECT_EQ(result.documents_imported, 0u);
}

/**
 * @test XI-07: importFromXML handles XML with no document elements gracefully.
 *
 * Valid XML with no <dokument> or <akte> tags must produce an empty document
 * list without error.
 */
TEST_F(XDOMEAImportTest, XI07_XmlWithNoDocumentElements)
{
    static constexpr const char* kXmlEmpty =
        R"(<xdomea:Nachricht xmlns:xdomea="https://www.xdomea.de/ns/xdomea/"></xdomea:Nachricht>)";

    auto result = connector_.importFromXML(kXmlEmpty, XDOMEAVersion::V3_0);

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.documents_imported, 0u);
    EXPECT_TRUE(result.errors.empty());
}

/**
 * @test XI-08: importFromXML assigns fallback ID when <id> element is absent.
 *
 * Documents without an explicit <id> element must receive a generated ID
 * (not empty) so they can be stored and retrieved.
 */
TEST_F(XDOMEAImportTest, XI08_FallbackIdAssignedWhenMissing)
{
    static constexpr const char* kXmlNoId = R"(
<xdomea:Nachricht>
  <dokument>
    <betreff>Document without ID</betreff>
  </dokument>
</xdomea:Nachricht>
)";
    auto result = connector_.importFromXML(kXmlNoId, XDOMEAVersion::V3_0);

    ASSERT_EQ(result.documents_imported, 1u);
    EXPECT_FALSE(result.documents[0].id.empty()) << "Fallback ID must not be empty";
}

// ═════════════════════════════════════════════════════════════════════════════
// XDOMEAExport — exportToXML serialization (XE-01 … XE-06)
// ═════════════════════════════════════════════════════════════════════════════

class XDOMEAExportTest : public ::testing::Test {
protected:
    InMemoryXDOMEAConnector connector_;
};

/**
 * @test XE-01: exportToXML V3.0 produces valid envelope with correct version attribute.
 *
 * The exported XML must contain the xdomea:version="3.0.0" attribute and the
 * correct nachrichtentyp code for ERFASSUNG (0601).
 */
TEST_F(XDOMEAExportTest, XE01_V30ExportContainsVersionAttribute)
{
    auto doc = makeDeterministicDoc("XE01-DOC-001", XDOMEAObjectType::DOKUMENT);

    auto result = connector_.exportToXML(
        {doc}, XDOMEAVersion::V3_0, XDOMEAMessageType::ERFASSUNG);

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.documents_exported, 1u);
    EXPECT_NE(result.xml_output.find("3.0.0"), std::string::npos);
    EXPECT_NE(result.xml_output.find("0601"),  std::string::npos);
}

/**
 * @test XE-02: exportToXML V2.1 produces correct version attribute.
 *
 * The exported XML must contain the xdomea:version="2.1.0" attribute.
 */
TEST_F(XDOMEAExportTest, XE02_V21ExportContainsVersionAttribute)
{
    auto doc = makeDeterministicDoc("XE02-DOC-001", XDOMEAObjectType::DOKUMENT);

    auto result = connector_.exportToXML(
        {doc}, XDOMEAVersion::V2_1, XDOMEAMessageType::ERFASSUNG);

    EXPECT_TRUE(result.success);
    EXPECT_NE(result.xml_output.find("2.1.0"), std::string::npos);
}

/**
 * @test XE-03: exportToXML includes document ID in the XML output.
 *
 * The document's ID field must appear in the serialized output so a downstream
 * importer can re-identify it.
 */
TEST_F(XDOMEAExportTest, XE03_ExportContainsDocumentId)
{
    auto doc = makeDeterministicDoc("XE03-UNIQUE-ID", XDOMEAObjectType::DOKUMENT);

    auto result = connector_.exportToXML(
        {doc}, XDOMEAVersion::V3_0, XDOMEAMessageType::ERFASSUNG);

    EXPECT_NE(result.xml_output.find("XE03-UNIQUE-ID"), std::string::npos);
}

/**
 * @test XE-04: exportToXML escapes reserved XML characters in betreff.
 *
 * A betreff containing <, >, &, ' and " must be safely escaped in the XML
 * output so the result is well-formed.
 */
TEST_F(XDOMEAExportTest, XE04_XmlEscapingApplied)
{
    auto doc = makeDeterministicDoc("XE04-DOC-001", XDOMEAObjectType::DOKUMENT);
    doc.betreff = "A & B <test> \"title\" 'value'";

    auto result = connector_.exportToXML(
        {doc}, XDOMEAVersion::V3_0, XDOMEAMessageType::ERFASSUNG);

    EXPECT_TRUE(result.success);
    // Raw reserved characters must NOT appear unescaped in the output.
    EXPECT_EQ(result.xml_output.find(" & "),  std::string::npos) << "Unescaped & found";
    EXPECT_EQ(result.xml_output.find(" <te"), std::string::npos) << "Unescaped < found";
    // Escaped forms must be present.
    EXPECT_NE(result.xml_output.find("&amp;"),  std::string::npos);
    EXPECT_NE(result.xml_output.find("&lt;"),   std::string::npos);
    EXPECT_NE(result.xml_output.find("&gt;"),   std::string::npos);
    EXPECT_NE(result.xml_output.find("&quot;"), std::string::npos);
    EXPECT_NE(result.xml_output.find("&apos;"), std::string::npos);
}

/**
 * @test XE-05: exportToXML produces the correct nachrichtentyp code for AUSSONDERUNG.
 *
 * Validates that the message-type code mapping is applied correctly for a
 * non-default message type.
 */
TEST_F(XDOMEAExportTest, XE05_AussonderungMessageTypeCode)
{
    auto doc = makeDeterministicDoc("XE05-DOC-001", XDOMEAObjectType::DOKUMENT);

    auto result = connector_.exportToXML(
        {doc}, XDOMEAVersion::V3_0, XDOMEAMessageType::AUSSONDERUNG);

    EXPECT_TRUE(result.success);
    EXPECT_NE(result.xml_output.find("0203"), std::string::npos);
}

/**
 * @test XE-06: exportToXML empty document list produces valid empty envelope.
 *
 * An empty document vector must produce a valid XML envelope without error
 * and with zero documents exported.
 */
TEST_F(XDOMEAExportTest, XE06_EmptyDocumentListProducesEnvelope)
{
    auto result = connector_.exportToXML(
        {}, XDOMEAVersion::V3_0, XDOMEAMessageType::ERFASSUNG);

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.documents_exported, 0u);
    EXPECT_FALSE(result.xml_output.empty()) << "Envelope XML must not be empty";
    EXPECT_NE(result.xml_output.find("xdomea:Nachricht"), std::string::npos);
}

// ═════════════════════════════════════════════════════════════════════════════
// XDOMEAMultiVersion — multi-version document lifecycle (XMV-01 … XMV-04)
// ═════════════════════════════════════════════════════════════════════════════

/**
 * @brief Multi-version document test fixture.
 *
 * Combines InMemoryXDOMEAConnector with InMemoryDocumentSchemaEvolution to
 * exercise documents that carry an explicit schema version across store /
 * retrieve / validate lifecycle steps.
 */
class XDOMEAMultiVersionTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Register schema version 1: id + betreff (required string fields).
        SchemaDescriptor v1;
        v1.fields.push_back({"id",      SchemaFieldType::STRING, true,  {}});
        v1.fields.push_back({"betreff", SchemaFieldType::STRING, true,  {}});
        ASSERT_TRUE(schema_.registerVersion(1, v1).has_value());

        // Register schema version 2: adds an optional aktenzeichen field.
        SchemaDescriptor v2;
        v2.fields.push_back({"id",           SchemaFieldType::STRING, true,  {}});
        v2.fields.push_back({"betreff",      SchemaFieldType::STRING, true,  {}});
        v2.fields.push_back({"aktenzeichen", SchemaFieldType::STRING, false, {}});
        ASSERT_TRUE(schema_.registerVersion(2, v2).has_value());
    }

    InMemoryXDOMEAConnector       connector_;
    InMemoryDocumentSchemaEvolution schema_;
};

/**
 * @test XMV-01: V1 and V2 schema versions are registered without conflict.
 *
 * Verifies that registeredVersions() returns both versions in ascending order.
 */
TEST_F(XDOMEAMultiVersionTest, XMV01_BothSchemaVersionsRegistered)
{
    auto versions = schema_.registeredVersions();
    ASSERT_EQ(versions.size(), 2u);
    EXPECT_EQ(versions[0], 1u);
    EXPECT_EQ(versions[1], 2u);
}

/**
 * @test XMV-02: Document body valid against schema v1 passes validation.
 *
 * A JSON body containing only the required v1 fields must validate
 * cleanly against schema version 1.
 */
TEST_F(XDOMEAMultiVersionTest, XMV02_V1BodyValidatesAgainstV1Schema)
{
    nlohmann::json body;
    body["id"]      = "XMV02-DOC-001";
    body["betreff"] = "Multi-version fixture v1";

    auto report = schema_.validate("XMV02-DOC-001", body, 1);
    ASSERT_TRUE(report.has_value()) << "validate() must not return error for known version";
    EXPECT_TRUE(report->violations.empty()) << "No violations expected for complete v1 body";
}

/**
 * @test XMV-03: Document body missing optional field validates against v2.
 *
 * A body with the required fields but without the optional aktenzeichen field
 * must still pass schema v2 validation (optional = no violation).
 */
TEST_F(XDOMEAMultiVersionTest, XMV03_OptionalFieldAbsenceDoesNotViolateV2)
{
    nlohmann::json body;
    body["id"]      = "XMV03-DOC-001";
    body["betreff"] = "V2 fixture without optional field";
    // aktenzeichen intentionally absent (optional in v2)

    auto report = schema_.validate("XMV03-DOC-001", body, 2);
    ASSERT_TRUE(report.has_value());
    EXPECT_TRUE(report->violations.empty()) << "Optional absent field must not produce violation";
}

/**
 * @test XMV-04: Stored XDOMEA document coexists with schema-validated document body.
 *
 * Verifies the complete multi-version lifecycle: import an XDOMEA document,
 * persist it in the connector, construct a parallel JSON body for schema
 * validation, and confirm both layers agree on the canonical ID.
 *
 * This test models the expected integration pattern where an XDOMEA exchange
 * layer and the schema evolution registry operate independently on the same
 * logical document.
 */
TEST_F(XDOMEAMultiVersionTest, XMV04_XDOMEAAndSchemaLayerCoexistOnSameDocument)
{
    static constexpr const char* kCanonicalId = "XMV04-DOC-CANONICAL-001";

    // Layer 1: XDOMEA connector — store a V3.0 document.
    auto doc = makeDeterministicDoc(kCanonicalId, XDOMEAObjectType::DOKUMENT);
    doc.aktenzeichen = "AZ-XMV04";
    connector_.storeDocument(doc);

    auto retrieved = connector_.getDocument(kCanonicalId);
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->id, kCanonicalId);

    // Layer 2: Schema evolution — validate a corresponding document body at v2.
    nlohmann::json body;
    body["id"]           = kCanonicalId;
    body["betreff"]      = doc.betreff;
    body["aktenzeichen"] = doc.aktenzeichen;

    auto report = schema_.validate(kCanonicalId, body, 2);
    ASSERT_TRUE(report.has_value());
    EXPECT_TRUE(report->violations.empty());

    // Both layers reference the same canonical ID.
    EXPECT_EQ(report->version, 2u);
}
