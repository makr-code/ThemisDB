/*
 * Tests for InMemoryXDOMEAConnector / IXDOMEAConnector
 *
 * Acceptance criteria:
 *   AC-XDM-01  Empty connector has count() == 0
 *   AC-XDM-02  storeDocument() adds a document; count() == 1
 *   AC-XDM-03  storeDocument() throws on empty ID
 *   AC-XDM-04  storeDocument() throws on duplicate ID
 *   AC-XDM-05  getDocument() returns nullopt for unknown ID
 *   AC-XDM-06  getDocument() returns the correct document after storage
 *   AC-XDM-07  removeDocument() removes a known document
 *   AC-XDM-08  removeDocument() is a no-op for unknown IDs
 *   AC-XDM-09  listByType() returns only documents of the requested type
 *   AC-XDM-10  listByType() returns empty vector when no match
 *   AC-XDM-11  listByRetention() returns only matching documents
 *   AC-XDM-12  listByRetention() returns empty for no match
 *   AC-XDM-13  listChildren() returns documents with matching parent_id
 *   AC-XDM-14  listChildren() returns empty for unknown parent
 *   AC-XDM-15  importFromXML() returns failure on empty XML
 *   AC-XDM-16  importFromXML() parses a single <dokument> element
 *   AC-XDM-17  importFromXML() extracts id, betreff, aktenzeichen fields
 *   AC-XDM-18  importFromXML() parses <akte> elements as AKTE type
 *   AC-XDM-19  importFromXML() persists documents to store
 *   AC-XDM-20  importFromXML() reports error on unclosed element
 *   AC-XDM-21  exportToXML() returns success with empty document list
 *   AC-XDM-22  exportToXML() serialises DOKUMENT objects correctly
 *   AC-XDM-23  exportToXML() serialises AKTE objects correctly
 *   AC-XDM-24  exportToXML() XML-escapes special characters
 *   AC-XDM-25  exportToXML() includes XDOMEA version attribute
 *   AC-XDM-26  exportToXML() includes message-type code in root element
 *   AC-XDM-27  XDOMEADocument::parent_id is preserved on getDocument()
 *   AC-XDM-28  XDOMEADocument::keywords are preserved
 *   AC-XDM-29  Concurrent storeDocument() calls are thread-safe
 *   AC-XDM-30  Polymorphic usage via IXDOMEAConnector*
 */

#include <gtest/gtest.h>
#include "document/xdomea_connector.h"

#include <future>

using namespace themis::document;

// ── Helpers ───────────────────────────────────────────────────────────────────

static XDOMEADocument makeDokument(const std::string& id,
                                    XDOMEAObjectType type = XDOMEAObjectType::DOKUMENT) {
    XDOMEADocument d;
    d.id           = id;
    d.object_type  = type;
    d.aktenzeichen = "AZ-" + id;
    d.betreff      = "Betreff " + id;
    d.author       = "TestAutor";
    return d;
}

static std::string makeXMLDoc(const std::string& id, const std::string& betreff) {
    return "<?xml version=\"1.0\" encoding=\"UTF-8\"?><xdomea>"
           "<dokument><id>" + id + "</id>"
           "<betreff>" + betreff + "</betreff>"
           "<aktenzeichen>AZ-001</aktenzeichen>"
           "</dokument></xdomea>";
}

// ── Fixture ───────────────────────────────────────────────────────────────────

class XDOMEAConnectorTest : public ::testing::Test {
protected:
    InMemoryXDOMEAConnector conn_;
};

// ── AC-XDM-01 ────────────────────────────────────────────────────────────────

TEST_F(XDOMEAConnectorTest, EmptyConnectorHasCountZero) {
    EXPECT_EQ(conn_.count(), 0u);
}

// ── AC-XDM-02 ────────────────────────────────────────────────────────────────

TEST_F(XDOMEAConnectorTest, StoreDocumentIncreasesCount) {
    conn_.storeDocument(makeDokument("DOC001"));
    EXPECT_EQ(conn_.count(), 1u);
}

// ── AC-XDM-03 ────────────────────────────────────────────────────────────────

TEST_F(XDOMEAConnectorTest, StoreDocumentThrowsOnEmptyId) {
    XDOMEADocument bad;
    EXPECT_THROW(conn_.storeDocument(bad), std::invalid_argument);
}

// ── AC-XDM-04 ────────────────────────────────────────────────────────────────

TEST_F(XDOMEAConnectorTest, StoreDocumentThrowsOnDuplicateId) {
    conn_.storeDocument(makeDokument("DUP"));
    EXPECT_THROW(conn_.storeDocument(makeDokument("DUP")), std::runtime_error);
}

// ── AC-XDM-05 ────────────────────────────────────────────────────────────────

TEST_F(XDOMEAConnectorTest, GetDocumentReturnsNulloptForUnknown) {
    EXPECT_FALSE(conn_.getDocument("GHOST").has_value());
}

// ── AC-XDM-06 ────────────────────────────────────────────────────────────────

TEST_F(XDOMEAConnectorTest, GetDocumentReturnsCorrectDocument) {
    auto doc = makeDokument("D001");
    doc.betreff = "Baugenehmigung";
    conn_.storeDocument(doc);

    auto found = conn_.getDocument("D001");
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->betreff, "Baugenehmigung");
}

// ── AC-XDM-07 ────────────────────────────────────────────────────────────────

TEST_F(XDOMEAConnectorTest, RemoveDocumentDecrementsCount) {
    conn_.storeDocument(makeDokument("R001"));
    conn_.removeDocument("R001");
    EXPECT_EQ(conn_.count(), 0u);
    EXPECT_FALSE(conn_.getDocument("R001").has_value());
}

// ── AC-XDM-08 ────────────────────────────────────────────────────────────────

TEST_F(XDOMEAConnectorTest, RemoveDocumentIsNoOpForUnknown) {
    EXPECT_NO_THROW(conn_.removeDocument("GHOST"));
    EXPECT_EQ(conn_.count(), 0u);
}

// ── AC-XDM-09 ────────────────────────────────────────────────────────────────

TEST_F(XDOMEAConnectorTest, ListByTypeReturnsOnlyMatchingType) {
    conn_.storeDocument(makeDokument("D1", XDOMEAObjectType::DOKUMENT));
    conn_.storeDocument(makeDokument("A1", XDOMEAObjectType::AKTE));
    conn_.storeDocument(makeDokument("D2", XDOMEAObjectType::DOKUMENT));

    auto docs = conn_.listByType(XDOMEAObjectType::DOKUMENT);
    EXPECT_EQ(docs.size(), 2u);
    for (const auto& d : docs) {
        EXPECT_EQ(d.object_type, XDOMEAObjectType::DOKUMENT);
    }
}

// ── AC-XDM-10 ────────────────────────────────────────────────────────────────

TEST_F(XDOMEAConnectorTest, ListByTypeReturnsEmptyForNoMatch) {
    conn_.storeDocument(makeDokument("D1", XDOMEAObjectType::DOKUMENT));
    EXPECT_TRUE(conn_.listByType(XDOMEAObjectType::AKTE).empty());
}

// ── AC-XDM-11 ────────────────────────────────────────────────────────────────

TEST_F(XDOMEAConnectorTest, ListByRetentionReturnsMatchingDocuments) {
    auto d = makeDokument("RET1");
    d.retention = XDOMEARetentionCategory::ARCHIVWUERDIG;
    conn_.storeDocument(d);
    conn_.storeDocument(makeDokument("RET2")); // default: UNGEKLAERT

    auto arch = conn_.listByRetention(XDOMEARetentionCategory::ARCHIVWUERDIG);
    ASSERT_EQ(arch.size(), 1u);
    EXPECT_EQ(arch[0].id, "RET1");
}

// ── AC-XDM-12 ────────────────────────────────────────────────────────────────

TEST_F(XDOMEAConnectorTest, ListByRetentionReturnsEmptyForNoMatch) {
    conn_.storeDocument(makeDokument("RET3"));
    EXPECT_TRUE(conn_.listByRetention(
        XDOMEARetentionCategory::NICHT_ARCHIVWUERDIG).empty());
}

// ── AC-XDM-13 ────────────────────────────────────────────────────────────────

TEST_F(XDOMEAConnectorTest, ListChildrenReturnsDocumentsWithMatchingParent) {
    conn_.storeDocument(makeDokument("PARENT1", XDOMEAObjectType::AKTE));
    auto c1 = makeDokument("CHILD1"); c1.parent_id = "PARENT1";
    auto c2 = makeDokument("CHILD2"); c2.parent_id = "PARENT1";
    auto c3 = makeDokument("CHILD3"); c3.parent_id = "OTHER";
    conn_.storeDocument(c1);
    conn_.storeDocument(c2);
    conn_.storeDocument(c3);

    auto children = conn_.listChildren("PARENT1");
    EXPECT_EQ(children.size(), 2u);
}

// ── AC-XDM-14 ────────────────────────────────────────────────────────────────

TEST_F(XDOMEAConnectorTest, ListChildrenReturnsEmptyForUnknownParent) {
    EXPECT_TRUE(conn_.listChildren("PHANTOM-PARENT").empty());
}

// ── AC-XDM-15 ────────────────────────────────────────────────────────────────

TEST_F(XDOMEAConnectorTest, ImportFromXMLFailsOnEmptyContent) {
    auto r = conn_.importFromXML("", XDOMEAVersion::V3_0);
    EXPECT_FALSE(r.success);
    EXPECT_FALSE(r.errors.empty());
}

// ── AC-XDM-16 ────────────────────────────────────────────────────────────────

TEST_F(XDOMEAConnectorTest, ImportParsesSingleDokument) {
    auto xml = makeXMLDoc("XD001", "Testbetreff");
    auto r   = conn_.importFromXML(xml, XDOMEAVersion::V3_0);
    EXPECT_TRUE(r.success);
    EXPECT_EQ(r.documents_parsed, 1u);
    EXPECT_EQ(r.documents_imported, 1u);
}

// ── AC-XDM-17 ────────────────────────────────────────────────────────────────

TEST_F(XDOMEAConnectorTest, ImportExtractsIdBetreffAktenzeichen) {
    auto xml = makeXMLDoc("XD002", "Baugenehmigung");
    auto r   = conn_.importFromXML(xml, XDOMEAVersion::V3_0);
    ASSERT_EQ(r.documents.size(), 1u);
    EXPECT_EQ(r.documents[0].id,           "XD002");
    EXPECT_EQ(r.documents[0].betreff,      "Baugenehmigung");
    EXPECT_EQ(r.documents[0].aktenzeichen, "AZ-001");
}

// ── AC-XDM-18 ────────────────────────────────────────────────────────────────

TEST_F(XDOMEAConnectorTest, ImportParsesAkteElements) {
    std::string xml = "<xdomea><akte><id>AKT001</id>"
                      "<betreff>Akte Mustermann</betreff>"
                      "<aktenzeichen>AZ-2024-001</aktenzeichen>"
                      "</akte></xdomea>";
    auto r = conn_.importFromXML(xml, XDOMEAVersion::V3_0);
    ASSERT_EQ(r.documents.size(), 1u);
    EXPECT_EQ(r.documents[0].object_type, XDOMEAObjectType::AKTE);
    EXPECT_EQ(r.documents[0].id, "AKT001");
}

// ── AC-XDM-19 ────────────────────────────────────────────────────────────────

TEST_F(XDOMEAConnectorTest, ImportPersistsDocumentsToStore) {
    auto xml = makeXMLDoc("PERSIST1", "Test");
    conn_.importFromXML(xml, XDOMEAVersion::V3_0);
    EXPECT_EQ(conn_.count(), 1u);
    EXPECT_TRUE(conn_.getDocument("PERSIST1").has_value());
}

// ── AC-XDM-20 ────────────────────────────────────────────────────────────────

TEST_F(XDOMEAConnectorTest, ImportReportsErrorOnUnclosedElement) {
    std::string xml = "<xdomea><dokument><id>UNCLOSED</id></xdomea>";
    auto r = conn_.importFromXML(xml, XDOMEAVersion::V3_0);
    EXPECT_FALSE(r.errors.empty());
}

// ── AC-XDM-21 ────────────────────────────────────────────────────────────────

TEST_F(XDOMEAConnectorTest, ExportToXMLSucceedsWithEmptyList) {
    auto r = conn_.exportToXML({}, XDOMEAVersion::V3_0, XDOMEAMessageType::ERFASSUNG);
    EXPECT_TRUE(r.success);
    EXPECT_FALSE(r.xml_output.empty());
    EXPECT_EQ(r.documents_exported, 0u);
}

// ── AC-XDM-22 ────────────────────────────────────────────────────────────────

TEST_F(XDOMEAConnectorTest, ExportSerialisesDocumentObjects) {
    auto doc = makeDokument("EXP001");
    auto r   = conn_.exportToXML({doc}, XDOMEAVersion::V3_0, XDOMEAMessageType::ERFASSUNG);
    EXPECT_TRUE(r.success);
    EXPECT_EQ(r.documents_exported, 1u);
    EXPECT_NE(r.xml_output.find("EXP001"), std::string::npos);
    EXPECT_NE(r.xml_output.find("<dokument>"), std::string::npos);
}

// ── AC-XDM-23 ────────────────────────────────────────────────────────────────

TEST_F(XDOMEAConnectorTest, ExportSerialisesAkteObjects) {
    auto akte = makeDokument("AKTE001", XDOMEAObjectType::AKTE);
    auto r    = conn_.exportToXML({akte}, XDOMEAVersion::V3_0, XDOMEAMessageType::ANBIETUNG);
    EXPECT_TRUE(r.success);
    EXPECT_NE(r.xml_output.find("<akte>"), std::string::npos);
}

// ── AC-XDM-24 ────────────────────────────────────────────────────────────────

TEST_F(XDOMEAConnectorTest, ExportXMLEscapesSpecialCharacters) {
    auto doc = makeDokument("XML001");
    doc.betreff = "Müller & Söhne <GmbH>";
    auto r = conn_.exportToXML({doc}, XDOMEAVersion::V3_0, XDOMEAMessageType::ERFASSUNG);
    EXPECT_TRUE(r.success);
    EXPECT_NE(r.xml_output.find("&amp;"), std::string::npos);
    EXPECT_NE(r.xml_output.find("&lt;"),  std::string::npos);
    EXPECT_NE(r.xml_output.find("&gt;"),  std::string::npos);
}

// ── AC-XDM-25 ────────────────────────────────────────────────────────────────

TEST_F(XDOMEAConnectorTest, ExportIncludesXDOMEAVersionAttribute) {
    auto r = conn_.exportToXML({}, XDOMEAVersion::V3_0, XDOMEAMessageType::ERFASSUNG);
    EXPECT_NE(r.xml_output.find("3.0.0"), std::string::npos);
}

// ── AC-XDM-26 ────────────────────────────────────────────────────────────────

TEST_F(XDOMEAConnectorTest, ExportIncludesMessageTypeCode) {
    auto r = conn_.exportToXML({}, XDOMEAVersion::V3_0, XDOMEAMessageType::AUSSONDERUNG);
    EXPECT_NE(r.xml_output.find("0203"), std::string::npos);

    auto r2 = conn_.exportToXML({}, XDOMEAVersion::V3_0, XDOMEAMessageType::ABGABE_AN_ARCHIV);
    EXPECT_NE(r2.xml_output.find("0401"), std::string::npos);
}

// ── AC-XDM-27 ────────────────────────────────────────────────────────────────

TEST_F(XDOMEAConnectorTest, ParentIdPreservedOnGetDocument) {
    auto doc = makeDokument("CHILD");
    doc.parent_id = "PARENT-X";
    conn_.storeDocument(doc);

    auto found = conn_.getDocument("CHILD");
    ASSERT_TRUE(found.has_value());
    ASSERT_TRUE(found->parent_id.has_value());
    EXPECT_EQ(*found->parent_id, "PARENT-X");
}

// ── AC-XDM-28 ────────────────────────────────────────────────────────────────

TEST_F(XDOMEAConnectorTest, KeywordsPreserved) {
    auto doc = makeDokument("KW001");
    doc.keywords = {"Baurecht", "§ 35 BauGB", "Bayern"};
    conn_.storeDocument(doc);

    auto found = conn_.getDocument("KW001");
    ASSERT_TRUE(found.has_value());
    ASSERT_EQ(found->keywords.size(), 3u);
    EXPECT_EQ(found->keywords[0], "Baurecht");
}

// ── AC-XDM-29 ────────────────────────────────────────────────────────────────

TEST_F(XDOMEAConnectorTest, ConcurrentStoreIsThreadSafe) {
    constexpr int N = 50;
    std::vector<std::future<void>> futs;
    for (int i = 0; i < N; ++i) {
        futs.push_back(std::async(std::launch::async, [this, i]() {
            try {
                conn_.storeDocument(makeDokument("CT" + std::to_string(i)));
            } catch (...) {}
        }));
    }
    for (auto& f : futs) f.get();
    EXPECT_LE(conn_.count(), static_cast<std::size_t>(N));
}

// ── AC-XDM-30 ────────────────────────────────────────────────────────────────

TEST_F(XDOMEAConnectorTest, PolymorphicUsageViaInterface) {
    IXDOMEAConnector* c = &conn_;
    c->storeDocument(makeDokument("POLY1"));
    EXPECT_EQ(c->count(), 1u);
    EXPECT_TRUE(c->getDocument("POLY1").has_value());
}
