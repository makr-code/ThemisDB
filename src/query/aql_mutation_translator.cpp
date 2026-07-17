/**
 * @file aql_mutation_translator.cpp
 * @brief Translates MutationNode AST nodes to MutationExecutionPlan — EPIC-004 Phase 3.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 */

#include "query/aql_translator.h"
#include "query/mutation_execution_plan.h"

namespace themis {

using namespace query;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

namespace {

/// Build an AcquireLock step for the given collection.
MutationStep stepAcquireLock(const std::string& collection) {
    return {MutationStepType::AcquireLock,
            "Acquire write lock on collection '" + collection + "'",
            {{"collection", collection}}};
}

/// Build a ReleaseLock step for the given collection.
MutationStep stepReleaseLock(const std::string& collection) {
    return {MutationStepType::ReleaseLock,
            "Release write lock on collection '" + collection + "'",
            {{"collection", collection}}};
}

/// Build a GenerateKeys step.
MutationStep stepGenerateKeys(const std::string& collection) {
    return {MutationStepType::GenerateKeys,
            "Generate unique _key values for new documents",
            {{"collection", collection}, {"id_field", "_key"}}};
}

/// Build a Serialize step.
MutationStep stepSerialize() {
    return {MutationStepType::Serialize,
            "Serialize document(s) to storage format",
            {{"format", "json"}}};
}

/// Build a WriteWAL step.
MutationStep stepWriteWAL(const std::string& collection, const std::string& op) {
    return {MutationStepType::WriteWAL,
            "Write mutation record to write-ahead log",
            {{"collection", collection}, {"operation", op}}};
}

/// Build a RocksDbPut step.
MutationStep stepRocksDbPut(const std::string& collection) {
    return {MutationStepType::RocksDbPut,
            "Write serialised document to RocksDB",
            {{"collection", collection}, {"key_prefix", collection + "/"}}};
}

/// Build a RocksDbDelete step.
MutationStep stepRocksDbDelete(const std::string& collection) {
    return {MutationStepType::RocksDbDelete,
            "Delete document key-value pair from RocksDB",
            {{"collection", collection}}};
}

/// Build an UpdateIndexes step.
MutationStep stepUpdateIndexes(const std::string& collection) {
    return {MutationStepType::UpdateIndexes,
            "Update secondary indexes for collection '" + collection + "'",
            {{"collection", collection}}};
}

/// Build a ValidatePredicate step (with optional description).
MutationStep stepValidatePredicate(const std::string& description) {
    return {MutationStepType::ValidatePredicate,
            "Validate predicate: " + description,
            {{"predicate", description}}};
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// translate (dispatcher)
// ---------------------------------------------------------------------------

MutationExecutionPlan AqlMutationTranslator::translate(
    const std::shared_ptr<MutationNode>& node) const
{
    if (!node) {
        // Null node → error plan
        MutationExecutionPlan plan;
        plan.mutation_type = ASTNodeType::Insert; // placeholder
        plan.collection    = "";
        plan.estimated_latency_ms = 0;
        plan.steps.push_back(stepValidatePredicate("ERROR: null MutationNode provided"));
        return plan;
    }

    switch (node->getType()) {
        case ASTNodeType::Insert:
            return translateInsert(dynamic_cast<const InsertNode&>(*node));
        case ASTNodeType::Update:
            return translateUpdate(dynamic_cast<const UpdateNode&>(*node));
        case ASTNodeType::Remove:
            return translateRemove(dynamic_cast<const RemoveNode&>(*node));
        case ASTNodeType::Replace:
            return translateReplace(dynamic_cast<const ReplaceNode&>(*node));
        case ASTNodeType::Upsert:
            return translateUpsert(dynamic_cast<const UpsertNode&>(*node));
        default: {
            MutationExecutionPlan plan;
            plan.mutation_type = node->getType();
            plan.collection    = "";
            plan.steps.push_back(stepValidatePredicate("ERROR: unsupported mutation type"));
            return plan;
        }
    }
}

// ---------------------------------------------------------------------------
// translateInsert
// ---------------------------------------------------------------------------

MutationExecutionPlan AqlMutationTranslator::translateInsert(const InsertNode& n) const {
    MutationExecutionPlan plan;
    plan.mutation_type        = ASTNodeType::Insert;
    plan.collection           = n.collection;
    plan.estimated_latency_ms = 5;
    plan.requires_transaction = n.return_new; // RETURN NEW needs transactional context

    plan.steps.push_back(stepAcquireLock(n.collection));
    plan.steps.push_back(stepGenerateKeys(n.collection));
    plan.steps.push_back(stepSerialize());
    plan.steps.push_back(stepWriteWAL(n.collection, "INSERT"));
    plan.steps.push_back(stepRocksDbPut(n.collection));
    plan.steps.push_back(stepUpdateIndexes(n.collection));
    plan.steps.push_back(stepReleaseLock(n.collection));

    return plan;
}

// ---------------------------------------------------------------------------
// translateUpdate
// ---------------------------------------------------------------------------

MutationExecutionPlan AqlMutationTranslator::translateUpdate(const UpdateNode& n) const {
    MutationExecutionPlan plan;
    plan.mutation_type        = ASTNodeType::Update;
    plan.collection           = n.collection;
    plan.estimated_latency_ms = 8;
    plan.requires_transaction = (n.return_new || n.return_old);
    if (n.limit.has_value()) {
        plan.affected_limit = n.limit;
    }

    plan.steps.push_back(stepAcquireLock(n.collection));

    if (n.filter || n.search_expr) {
        const std::string desc = n.filter ? "FILTER/WHERE predicate"
                                           : "AQL search expression";
        plan.steps.push_back(stepValidatePredicate(desc));
    }

    plan.steps.push_back(stepWriteWAL(n.collection, "UPDATE"));
    plan.steps.push_back(stepRocksDbPut(n.collection));
    plan.steps.push_back(stepUpdateIndexes(n.collection));
    plan.steps.push_back(stepReleaseLock(n.collection));

    return plan;
}

// ---------------------------------------------------------------------------
// translateRemove
// ---------------------------------------------------------------------------

MutationExecutionPlan AqlMutationTranslator::translateRemove(const RemoveNode& n) const {
    MutationExecutionPlan plan;
    plan.mutation_type        = ASTNodeType::Remove;
    plan.collection           = n.collection;
    plan.estimated_latency_ms = 4;
    plan.requires_transaction = n.return_removed;
    if (n.limit.has_value()) {
        plan.affected_limit = n.limit;
    }

    plan.steps.push_back(stepAcquireLock(n.collection));

    if (n.filter || n.doc_expr) {
        const std::string desc = n.filter ? "FILTER/WHERE predicate"
                                           : "document expression";
        plan.steps.push_back(stepValidatePredicate(desc));
    }
    // No predicate → warn (still produce a plan, executor will proceed)

    plan.steps.push_back(stepWriteWAL(n.collection, "REMOVE"));
    plan.steps.push_back(stepRocksDbDelete(n.collection));
    plan.steps.push_back(stepUpdateIndexes(n.collection));
    plan.steps.push_back(stepReleaseLock(n.collection));

    return plan;
}

// ---------------------------------------------------------------------------
// translateReplace
// ---------------------------------------------------------------------------

MutationExecutionPlan AqlMutationTranslator::translateReplace(const ReplaceNode& n) const {
    MutationExecutionPlan plan;
    plan.mutation_type        = ASTNodeType::Replace;
    plan.collection           = n.collection;
    plan.estimated_latency_ms = 5;
    plan.requires_transaction = (n.return_new || n.return_old);

    plan.steps.push_back(stepAcquireLock(n.collection));
    plan.steps.push_back(stepSerialize());
    plan.steps.push_back(stepWriteWAL(n.collection, "REPLACE"));
    plan.steps.push_back(stepRocksDbPut(n.collection)); // overwrite semantics
    plan.steps.push_back(stepUpdateIndexes(n.collection));
    plan.steps.push_back(stepReleaseLock(n.collection));

    return plan;
}

// ---------------------------------------------------------------------------
// translateUpsert
// ---------------------------------------------------------------------------

MutationExecutionPlan AqlMutationTranslator::translateUpsert(const UpsertNode& n) const {
    MutationExecutionPlan plan;
    plan.mutation_type        = ASTNodeType::Upsert;
    plan.collection           = n.collection;
    plan.estimated_latency_ms = 10;
    plan.requires_transaction = (n.return_new || n.return_old);

    plan.steps.push_back(stepAcquireLock(n.collection));

    // Always validate the search expression
    plan.steps.push_back(stepValidatePredicate("UPSERT search_expr"));

    plan.steps.push_back(stepWriteWAL(n.collection, "UPSERT"));

    // Conditional: GenerateKeys (insert branch) or direct RocksDbPut (update branch)
    // Both branches ultimately end with a RocksDbPut — the executor decides at runtime.
    plan.steps.push_back(stepGenerateKeys(n.collection)); // no-op on update branch
    plan.steps.push_back(stepRocksDbPut(n.collection));

    plan.steps.push_back(stepUpdateIndexes(n.collection));
    plan.steps.push_back(stepReleaseLock(n.collection));

    return plan;
}

} // namespace themis
