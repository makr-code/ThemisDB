/*
 * ThemisDB — Document Module Tests
 *
 * Tests for:
 *   InMemoryDocumentStore        (IDocumentStore)
 *   InMemoryDocumentManager      (IDocumentManager + IEncryptedDocumentEntity)
 *   IDocumentLifecycleHook       (via TestHook adapter)
 *   InMemoryDocumentSchemaEvolution (IDocumentSchemaEvolution)
 *   InMemoryDocumentDiffMerge    (IDocumentDiffMerge)
 *
 * Acceptance criteria:
 *
 * DocumentStore (DS)
 *   DS-01  Empty store: count() == 0 for any collection
 *   DS-02  put() increments count() by 1
 *   DS-03  put() returns the document id on success
 *   DS-04  put() returns ERR_DOC_INVALID_ID for empty id
 *   DS-05  put() returns ERR_DOC_ALREADY_EXISTS on duplicate id
 *   DS-06  get() returns nullopt for unknown document
 *   DS-07  get() returns the correct document after put()
 *   DS-08  update() modifies body; get() reflects the change
 *   DS-09  update() returns ERR_DOC_NOT_FOUND for missing document
 *   DS-10  remove() removes a known document; count() decrements
 *   DS-11  remove() is a no-op (success) for unknown document
 *   DS-12  list() returns all ids in a collection
 *   DS-13  list() returns empty vector for unknown collection
 *   DS-14  count() is collection-scoped (other collections unaffected)
 *   DS-15  Concurrent put() calls across collections are thread-safe
 *
 * DocumentManager (DM)
 *   DM-01  create() succeeds; get() returns the stored body
 *   DM-02  create() returns ERR_DOC_INVALID_ID for empty id
 *   DM-03  create() returns ERR_DOC_ALREADY_EXISTS on duplicate
 *   DM-04  update() replaces body; get() reflects change
 *   DM-05  update() returns ERR_DOC_NOT_FOUND for missing document
 *   DM-06  remove() removes; subsequent get() returns nullopt
 *   DM-07  createEncrypted() returns opaque handle; collectionId() correct
 *   DM-08  createEncrypted() returns ERR_DOC_INVALID_ID for empty id
 *
 * DocumentLifecycle (DL)
 *   DL-01  beforeCreate fires before storage write
 *   DL-02  afterCreate fires after successful create()
 *   DL-03  beforeDelete and afterDelete both fire on remove()
 *   DL-04  beforeUpdate and afterUpdate fire on update()
 *   DL-05  Unregistered hook no longer receives events
 *
 * DocumentSchemaEvolution (DSE)
 *   DSE-01  registerVersion() succeeds; registeredVersions() lists version
 *   DSE-02  registerVersion() returns ERR_DOC_SCHEMA_VERSION_EXISTS on dup
 *   DSE-03  seal() prevents further registerVersion(); returns ERR_DOC_SCHEMA_SEALED
 *   DSE-04  isSealed() returns false before seal(), true after
 *   DSE-05  validate() returns valid report for compliant document
 *   DSE-06  validate() reports MISSING_REQUIRED_FIELD for absent required field
 *   DSE-07  validate() reports TYPE_MISMATCH for wrong type
 *   DSE-08  validate() returns ERR_DOC_SCHEMA_VERSION_NOT_FOUND for unknown version
 *
 * DocumentDiffMerge (DDM)
 *   DDM-01  diff() returns empty DocumentDiff for identical documents
 *   DDM-02  diff() reports added fields correctly
 *   DDM-03  diff() reports removed fields correctly
 *   DDM-04  diff() reports modified fields with old and new value
 *   DDM-05  diff() returns ERR_DOC_DIFF_NOT_FOUND for unknown base
 *   DDM-06  merge() produces clean merge when branches modify different fields
 *   DDM-07  merge() lists conflicts when both branches modify the same field
 *   DDM-08  merge() with OURS_WINS resolves conflicts using "ours" value
 *   DDM-09  merge() with FAIL returns ERR_DOC_MERGE_CONFLICT on conflict
 *   DDM-10  IEncryptedDocumentEntity::reencrypt() succeeds with valid key ids
 *   DDM-11  IEncryptedDocumentEntity::reencrypt() returns ERR_DOC_INVALID_ARGUMENT for empty new_key_id
 */

#include <gtest/gtest.h>

#include "document/document_diff_merge.h"
#include "document/document_lifecycle.h"
#include "document/document_manager.h"
#include "document/document_schema_evolution.h"
#include "document/document_store.h"

#include <future>
#include <thread>

using namespace themis;
using namespace themis::document;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

static nlohmann::json makeBody(const std::string& tag = "test") {
    return nlohmann::json{{"tag", tag}, {"value", 42}};
}

// ─────────────────────────────────────────────────────────────────────────────
// DocumentStore tests  (DS-01 … DS-15)
// ─────────────────────────────────────────────────────────────────────────────

class DocumentStoreTest : public ::testing::Test {
protected:
    InMemoryDocumentStore store_;
    const CollectionId    kCol{"docs"};
};

// DS-01
TEST_F(DocumentStoreTest, EmptyStoreCountZero) {
    auto r = store_.count(kCol);
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(*r, 0u);
}

// DS-02
TEST_F(DocumentStoreTest, PutIncrementsCount) {
    DocumentRecord rec{"id-001", kCol, makeBody()};
    ASSERT_TRUE(store_.put(rec).has_value());
    EXPECT_EQ(*store_.count(kCol), 1u);
}

// DS-03
TEST_F(DocumentStoreTest, PutReturnsDocumentId) {
    DocumentRecord rec{"id-002", kCol, makeBody()};
    auto r = store_.put(rec);
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(*r, "id-002");
}

// DS-04
TEST_F(DocumentStoreTest, PutReturnsErrInvalidIdForEmptyId) {
    DocumentRecord rec{"", kCol, makeBody()};
    auto r = store_.put(rec);
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error().code(), errors::ErrorCode::ERR_DOC_INVALID_ID);
}

// DS-05
TEST_F(DocumentStoreTest, PutReturnsErrAlreadyExistsOnDuplicate) {
    DocumentRecord rec{"id-003", kCol, makeBody()};
    ASSERT_TRUE(store_.put(rec).has_value());
    auto r = store_.put(rec);
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error().code(), errors::ErrorCode::ERR_DOC_ALREADY_EXISTS);
}

// DS-06
TEST_F(DocumentStoreTest, GetReturnsNulloptForUnknown) {
    auto r = store_.get(kCol, "unknown");
    ASSERT_TRUE(r.has_value());
    EXPECT_FALSE(r.value().has_value());
}

// DS-07
TEST_F(DocumentStoreTest, GetReturnsCorrectDocumentAfterPut) {
    nlohmann::json body{{"key", "hello"}};
    DocumentRecord rec{"id-004", kCol, body};
    ASSERT_TRUE(store_.put(rec).has_value());

    auto r = store_.get(kCol, "id-004");
    ASSERT_TRUE(r.has_value());
    ASSERT_TRUE(r.value().has_value());
    EXPECT_EQ(r.value().value().body["key"], "hello");
}

// DS-08
TEST_F(DocumentStoreTest, UpdateModifiesBody) {
    DocumentRecord rec{"id-005", kCol, makeBody("original")};
    ASSERT_TRUE(store_.put(rec).has_value());

    nlohmann::json updated{{"key", "updated"}};
    ASSERT_TRUE(store_.update(kCol, "id-005", updated).has_value());

    auto r = store_.get(kCol, "id-005");
    ASSERT_TRUE(r.has_value() && r.value().has_value());
    EXPECT_EQ(r.value().value().body["key"], "updated");
}

// DS-09
TEST_F(DocumentStoreTest, UpdateReturnsErrNotFoundForMissing) {
    auto r = store_.update(kCol, "ghost", makeBody());
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error().code(), errors::ErrorCode::ERR_DOC_NOT_FOUND);
}

// DS-10
TEST_F(DocumentStoreTest, RemoveDecreasesCount) {
    DocumentRecord rec{"id-006", kCol, makeBody()};
    ASSERT_TRUE(store_.put(rec).has_value());
    ASSERT_TRUE(store_.remove(kCol, "id-006").has_value());
    EXPECT_EQ(*store_.count(kCol), 0u);
}

// DS-11
TEST_F(DocumentStoreTest, RemoveIsNoOpForUnknown) {
    auto r = store_.remove(kCol, "does-not-exist");
    EXPECT_TRUE(r.has_value());
}

// DS-12
TEST_F(DocumentStoreTest, ListReturnsAllIds) {
    for (int i = 0; i < 3; ++i) {
        EXPECT_TRUE(store_.put({"id-list-" + std::to_string(i), kCol, makeBody()}).has_value());
    }
    auto r = store_.list(kCol);
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(r.value().size(), 3u);
}

// DS-13
TEST_F(DocumentStoreTest, ListReturnsEmptyForUnknownCollection) {
    auto r = store_.list("nonexistent");
    ASSERT_TRUE(r.has_value());
    EXPECT_TRUE(r.value().empty());
}

// DS-14
TEST_F(DocumentStoreTest, CountIsCollectionScoped) {
    EXPECT_TRUE(store_.put({"a1", "colA", makeBody()}).has_value());
    EXPECT_TRUE(store_.put({"b1", "colB", makeBody()}).has_value());
    EXPECT_TRUE(store_.put({"b2", "colB", makeBody()}).has_value());
    EXPECT_EQ(*store_.count("colA"), 1u);
    EXPECT_EQ(*store_.count("colB"), 2u);
}

// DS-15
TEST_F(DocumentStoreTest, ConcurrentPutIsThreadSafe) {
    constexpr int N = 40;
    std::vector<std::future<void>> futs;
    futs.reserve(N);
    for (int i = 0; i < N; ++i) {
        futs.push_back(std::async(std::launch::async, [&, i]() {
            static_cast<void>(store_.put({"thr-" + std::to_string(i), kCol, makeBody()}));
        }));
    }
    for (auto& f : futs) {
      f.get();
    }
    auto cnt = store_.count(kCol);
    ASSERT_TRUE(cnt.has_value());
    EXPECT_LE(*cnt, static_cast<std::size_t>(N));
}

// ─────────────────────────────────────────────────────────────────────────────
// DocumentManager tests  (DM-01 … DM-08)
// ─────────────────────────────────────────────────────────────────────────────

class DocumentManagerTest : public ::testing::Test {
protected:
    InMemoryDocumentManager mgr_;
    const CollectionId      kCol{"mgr-col"};
};

// DM-01
TEST_F(DocumentManagerTest, CreateAndGet) {
    nlohmann::json body{{"x", 1}};
    ASSERT_TRUE(mgr_.create(kCol, "dm-001", body).has_value());

    auto r = mgr_.get(kCol, "dm-001");
    ASSERT_TRUE(r.has_value() && r.value().has_value());
    EXPECT_EQ(r.value().value()["x"], 1);
}

// DM-02
TEST_F(DocumentManagerTest, CreateReturnsErrInvalidIdForEmpty) {
    auto r = mgr_.create(kCol, "", makeBody());
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error().code(), errors::ErrorCode::ERR_DOC_INVALID_ID);
}

// DM-03
TEST_F(DocumentManagerTest, CreateReturnsErrAlreadyExistsOnDuplicate) {
    ASSERT_TRUE(mgr_.create(kCol, "dm-003", makeBody()).has_value());
    auto r = mgr_.create(kCol, "dm-003", makeBody());
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error().code(), errors::ErrorCode::ERR_DOC_ALREADY_EXISTS);
}

// DM-04
TEST_F(DocumentManagerTest, UpdateReplacesBody) {
    ASSERT_TRUE(mgr_.create(kCol, "dm-004", nlohmann::json{{"v", 1}}).has_value());
    ASSERT_TRUE(mgr_.update(kCol, "dm-004", nlohmann::json{{"v", 2}}).has_value());

    auto r = mgr_.get(kCol, "dm-004");
    ASSERT_TRUE(r.has_value() && r.value().has_value());
    EXPECT_EQ(r.value().value()["v"], 2);
}

// DM-05
TEST_F(DocumentManagerTest, UpdateReturnsErrNotFound) {
    auto r = mgr_.update(kCol, "ghost", makeBody());
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error().code(), errors::ErrorCode::ERR_DOC_NOT_FOUND);
}

// DM-06
TEST_F(DocumentManagerTest, RemoveClearsDocument) {
    ASSERT_TRUE(mgr_.create(kCol, "dm-006", makeBody()).has_value());
    ASSERT_TRUE(mgr_.remove(kCol, "dm-006").has_value());

    auto r = mgr_.get(kCol, "dm-006");
    ASSERT_TRUE(r.has_value());
    EXPECT_FALSE(r.value().has_value());
}

// DM-07
TEST_F(DocumentManagerTest, CreateEncryptedReturnsOpaqueHandle) {
    auto r = mgr_.createEncrypted(kCol, "enc-001", makeBody());
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(r.value()->documentId(), "enc-001");
    EXPECT_EQ(r.value()->collectionId(), kCol);
}

// DM-08
TEST_F(DocumentManagerTest, CreateEncryptedReturnsErrInvalidId) {
    auto r = mgr_.createEncrypted(kCol, "", makeBody());
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error().code(), errors::ErrorCode::ERR_DOC_INVALID_ID);
}

// ─────────────────────────────────────────────────────────────────────────────
// DocumentLifecycle tests  (DL-01 … DL-05)
// ─────────────────────────────────────────────────────────────────────────────

struct TestHook final : public IDocumentLifecycleHook {
    std::vector<DocumentEventType> events;

    void beforeCreate(const DocumentLifecycleEvent& evt) noexcept override {
        events.push_back(evt.type);
    }
    void afterCreate(const DocumentLifecycleEvent& evt) noexcept override {
        events.push_back(evt.type);
    }
    void beforeUpdate(const DocumentLifecycleEvent& evt) noexcept override {
        events.push_back(evt.type);
    }
    void afterUpdate(const DocumentLifecycleEvent& evt) noexcept override {
        events.push_back(evt.type);
    }
    void beforeDelete(const DocumentLifecycleEvent& evt) noexcept override {
        events.push_back(evt.type);
    }
    void afterDelete(const DocumentLifecycleEvent& evt) noexcept override {
        events.push_back(evt.type);
    }
};

class DocumentLifecycleTest : public ::testing::Test {
protected:
    InMemoryDocumentManager mgr_;
    TestHook                hook_;
    const CollectionId      kCol{"lc-col"};

    void SetUp() override {
        mgr_.registerLifecycleHook(hook_);
    }
};

// DL-01 + DL-02
TEST_F(DocumentLifecycleTest, BeforeAndAfterCreateBothFire) {
    ASSERT_TRUE(mgr_.create(kCol, "lc-001", makeBody()).has_value());
    ASSERT_EQ(hook_.events.size(), 2u);
    EXPECT_EQ(hook_.events[0], DocumentEventType::BEFORE_CREATE);
    EXPECT_EQ(hook_.events[1], DocumentEventType::AFTER_CREATE);
}

// DL-03
TEST_F(DocumentLifecycleTest, BeforeAndAfterDeleteBothFire) {
    ASSERT_TRUE(mgr_.create(kCol, "lc-002", makeBody()).has_value());
    hook_.events.clear();

    ASSERT_TRUE(mgr_.remove(kCol, "lc-002").has_value());
    ASSERT_EQ(hook_.events.size(), 2u);
    EXPECT_EQ(hook_.events[0], DocumentEventType::BEFORE_DELETE);
    EXPECT_EQ(hook_.events[1], DocumentEventType::AFTER_DELETE);
}

// DL-04
TEST_F(DocumentLifecycleTest, BeforeAndAfterUpdateBothFire) {
    ASSERT_TRUE(mgr_.create(kCol, "lc-003", makeBody()).has_value());
    hook_.events.clear();

    ASSERT_TRUE(mgr_.update(kCol, "lc-003", makeBody("updated")).has_value());
    ASSERT_EQ(hook_.events.size(), 2u);
    EXPECT_EQ(hook_.events[0], DocumentEventType::BEFORE_UPDATE);
    EXPECT_EQ(hook_.events[1], DocumentEventType::AFTER_UPDATE);
}

// DL-05
TEST_F(DocumentLifecycleTest, UnregisteredHookNoLongerReceivesEvents) {
    mgr_.unregisterLifecycleHook(hook_);
    ASSERT_TRUE(mgr_.create(kCol, "lc-004", makeBody()).has_value());
    EXPECT_TRUE(hook_.events.empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// DocumentSchemaEvolution tests  (DSE-01 … DSE-08)
// ─────────────────────────────────────────────────────────────────────────────

class DocumentSchemaEvolutionTest : public ::testing::Test {
protected:
    InMemoryDocumentSchemaEvolution evo_;
};

static SchemaDescriptor makeSchema() {
    SchemaDescriptor sd;
    sd.fields.push_back({"name",  SchemaFieldType::STRING, true,  {}});
    sd.fields.push_back({"age",   SchemaFieldType::NUMBER, false, {}});
    sd.fields.push_back({"active",SchemaFieldType::BOOLEAN,true,  {}});
    return sd;
}

// DSE-01
TEST_F(DocumentSchemaEvolutionTest, RegisterVersionSucceeds) {
    ASSERT_TRUE(evo_.registerVersion(1, makeSchema()).has_value());
    auto vs = evo_.registeredVersions();
    ASSERT_EQ(vs.size(), 1u);
    EXPECT_EQ(vs[0], 1u);
}

// DSE-02
TEST_F(DocumentSchemaEvolutionTest, RegisterVersionErrOnDuplicate) {
    ASSERT_TRUE(evo_.registerVersion(1, makeSchema()).has_value());
    auto r = evo_.registerVersion(1, makeSchema());
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error().code(), errors::ErrorCode::ERR_DOC_SCHEMA_VERSION_EXISTS);
}

// DSE-03
TEST_F(DocumentSchemaEvolutionTest, SealedRegistryRejectsNewVersion) {
    ASSERT_TRUE(evo_.registerVersion(1, makeSchema()).has_value());
    evo_.seal();
    auto r = evo_.registerVersion(2, makeSchema());
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error().code(), errors::ErrorCode::ERR_DOC_SCHEMA_SEALED);
}

// DSE-04
TEST_F(DocumentSchemaEvolutionTest, IsSealedTransition) {
    EXPECT_FALSE(evo_.isSealed());
    (void)evo_.seal();
    EXPECT_TRUE(evo_.isSealed());
}

// DSE-05
TEST_F(DocumentSchemaEvolutionTest, ValidateCompliantDocument) {
    ASSERT_TRUE(evo_.registerVersion(1, makeSchema()).has_value());
    nlohmann::json doc{{"name", "Alice"}, {"active", true}};
    auto r = evo_.validate("doc-valid", doc, 1);
    ASSERT_TRUE(r.has_value());
    EXPECT_TRUE(r.value().isValid());
}

// DSE-06
TEST_F(DocumentSchemaEvolutionTest, ValidateReportsMissingRequired) {
    ASSERT_TRUE(evo_.registerVersion(1, makeSchema()).has_value());
    nlohmann::json doc{{"name", "Bob"}}; // missing "active"
    auto r = evo_.validate("doc-missing", doc, 1);
    ASSERT_TRUE(r.has_value());
    EXPECT_FALSE(r.value().isValid());
    ASSERT_EQ(r.value().violations.size(), 1u);
    EXPECT_EQ(r.value().violations[0].kind, FieldViolationKind::MISSING_REQUIRED_FIELD);
    EXPECT_EQ(r.value().violations[0].field_name, "active");
}

// DSE-07
TEST_F(DocumentSchemaEvolutionTest, ValidateReportsTypeMismatch) {
    ASSERT_TRUE(evo_.registerVersion(1, makeSchema()).has_value());
    nlohmann::json doc{{"name", 123}, {"active", true}}; // name should be STRING
    auto r = evo_.validate("doc-type", doc, 1);
    ASSERT_TRUE(r.has_value());
    EXPECT_FALSE(r.value().isValid());
    bool found = false;
    for (const auto& v : r.value().violations) {
        if (v.field_name == "name" &&
            v.kind == FieldViolationKind::TYPE_MISMATCH) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// DSE-08
TEST_F(DocumentSchemaEvolutionTest, ValidateErrUnknownVersion) {
    auto r = evo_.validate("doc-x", nlohmann::json{}, 99);
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error().code(),
              errors::ErrorCode::ERR_DOC_SCHEMA_VERSION_NOT_FOUND);
}

// ─────────────────────────────────────────────────────────────────────────────
// DocumentDiffMerge tests  (DDM-01 … DDM-11)
// ─────────────────────────────────────────────────────────────────────────────

class DocumentDiffMergeTest : public ::testing::Test {
protected:
    InMemoryDocumentStore    store_;
    InMemoryDocumentDiffMerge dm_{store_};
    const CollectionId       kCol{"diff-col"};

    void put(const std::string& id, const nlohmann::json& body) {
        (void)store_.put({id, kCol, body});
    }
};

// DDM-01
TEST_F(DocumentDiffMergeTest, DiffIdenticalDocumentsEmpty) {
    put("d1", nlohmann::json{{"a", 1}});
    put("d2", nlohmann::json{{"a", 1}});
    auto r = dm_.diff(kCol, "d1", "d2");
    ASSERT_TRUE(r.has_value());
    EXPECT_TRUE(r.value().isEmpty());
}

// DDM-02
TEST_F(DocumentDiffMergeTest, DiffReportsAddedFields) {
    put("base", nlohmann::json{{"a", 1}});
    put("tgt",  nlohmann::json{{"a", 1}, {"b", 2}});
    auto r = dm_.diff(kCol, "base", "tgt");
    ASSERT_TRUE(r.has_value());
    ASSERT_EQ(r.value().added_fields.size(), 1u);
    EXPECT_EQ(r.value().added_fields[0], "b");
}

// DDM-03
TEST_F(DocumentDiffMergeTest, DiffReportsRemovedFields) {
    put("base2", nlohmann::json{{"a", 1}, {"c", 3}});
    put("tgt2",  nlohmann::json{{"a", 1}});
    auto r = dm_.diff(kCol, "base2", "tgt2");
    ASSERT_TRUE(r.has_value());
    ASSERT_EQ(r.value().removed_fields.size(), 1u);
    EXPECT_EQ(r.value().removed_fields[0], "c");
}

// DDM-04
TEST_F(DocumentDiffMergeTest, DiffReportsModifiedFields) {
    put("base3", nlohmann::json{{"x", "old"}});
    put("tgt3",  nlohmann::json{{"x", "new"}});
    auto r = dm_.diff(kCol, "base3", "tgt3");
    ASSERT_TRUE(r.has_value());
    ASSERT_EQ(r.value().modified_fields.size(), 1u);
    EXPECT_EQ(r.value().modified_fields[0].field_name, "x");
    EXPECT_EQ(r.value().modified_fields[0].old_value, "old");
    EXPECT_EQ(r.value().modified_fields[0].new_value, "new");
}

// DDM-05
TEST_F(DocumentDiffMergeTest, DiffErrNotFoundForUnknownBase) {
    put("tgt-only", nlohmann::json{{"k", 1}});
    auto r = dm_.diff(kCol, "no-such-base", "tgt-only");
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error().code(), errors::ErrorCode::ERR_DOC_DIFF_NOT_FOUND);
}

// DDM-06
TEST_F(DocumentDiffMergeTest, MergeCleanNonOverlapping) {
    put("base-m",   nlohmann::json{{"shared", 1}});
    put("ours-m",   nlohmann::json{{"shared", 1}, {"ours_field", "O"}});
    put("theirs-m", nlohmann::json{{"shared", 1}, {"theirs_field", "T"}});

    auto r = dm_.merge(kCol, "base-m", "ours-m", "theirs-m");
    ASSERT_TRUE(r.has_value());
    EXPECT_TRUE(r.value().conflicts.empty());
    EXPECT_TRUE(r.value().merged_body.contains("ours_field"));
    EXPECT_TRUE(r.value().merged_body.contains("theirs_field"));
}

// DDM-07
TEST_F(DocumentDiffMergeTest, MergeListsConflicts) {
    put("base-c",   nlohmann::json{{"x", 1}});
    put("ours-c",   nlohmann::json{{"x", 2}});
    put("theirs-c", nlohmann::json{{"x", 3}});

    auto r = dm_.merge(kCol, "base-c", "ours-c", "theirs-c",
                       MergeStrategy::OURS_WINS);
    ASSERT_TRUE(r.has_value());
    ASSERT_EQ(r.value().conflicts.size(), 1u);
    EXPECT_EQ(r.value().conflicts[0].field_name, "x");
}

// DDM-08
TEST_F(DocumentDiffMergeTest, MergeOursWinsResolvesConflicts) {
    put("base-ow",   nlohmann::json{{"v", 0}});
    put("ours-ow",   nlohmann::json{{"v", 1}});
    put("theirs-ow", nlohmann::json{{"v", 2}});

    auto r = dm_.merge(kCol, "base-ow", "ours-ow", "theirs-ow",
                       MergeStrategy::OURS_WINS);
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(r.value().merged_body["v"], 1);
}

// DDM-09
TEST_F(DocumentDiffMergeTest, MergeFailStrategyReturnsError) {
    put("base-f",   nlohmann::json{{"v", 0}});
    put("ours-f",   nlohmann::json{{"v", 10}});
    put("theirs-f", nlohmann::json{{"v", 20}});

    auto r = dm_.merge(kCol, "base-f", "ours-f", "theirs-f",
                       MergeStrategy::FAIL);
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error().code(), errors::ErrorCode::ERR_DOC_MERGE_CONFLICT);
}

// DDM-10
TEST_F(DocumentDiffMergeTest, ReencryptSucceedsWithValidKeyIds) {
    InMemoryDocumentManager mgr;
    auto enc = mgr.createEncrypted(kCol, "re-001", makeBody());
    ASSERT_TRUE(enc.has_value());
    KeyRotationDescriptor desc{"old-key", "new-key", 0};
    EXPECT_TRUE(enc.value()->reencrypt(desc).has_value());
}

// DDM-11
TEST_F(DocumentDiffMergeTest, ReencryptErrInvalidArgumentForEmptyNewKey) {
    InMemoryDocumentManager mgr;
    auto enc = mgr.createEncrypted(kCol, "re-002", makeBody());
    ASSERT_TRUE(enc.has_value());
    KeyRotationDescriptor desc{"old-key", "", 0};
    auto r = enc.value()->reencrypt(desc);
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error().code(), errors::ErrorCode::ERR_DOC_INVALID_ARGUMENT);
}

// DDM-12
TEST_F(DocumentDiffMergeTest, ReencryptUpdatesCurrentKeyId)
{
    InMemoryDocumentManager mgr;
    auto enc = mgr.createEncrypted(kCol, "re-003", makeBody());
    ASSERT_TRUE(enc.has_value());

    // Cast to the concrete type to access currentKeyId()
    auto* entity = dynamic_cast<InMemoryEncryptedEntity*>(enc.value().get());
    ASSERT_NE(entity, nullptr);

    // currentKeyId() is empty before any rotation
    EXPECT_TRUE(entity->currentKeyId().empty());

    KeyRotationDescriptor desc{"old-key", "new-key-v2", 0};
    ASSERT_TRUE(entity->reencrypt(desc).has_value());

    // After rotation the new key_id should be reflected
    EXPECT_EQ(entity->currentKeyId(), "new-key-v2");

    // A second rotation advances the key_id
    KeyRotationDescriptor desc2{"new-key-v2", "new-key-v3", 1};
    ASSERT_TRUE(entity->reencrypt(desc2).has_value());
    EXPECT_EQ(entity->currentKeyId(), "new-key-v3");
}
