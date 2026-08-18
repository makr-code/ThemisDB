/**
 * @file graph_index.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=6, H=4, M=21, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Graph adjacency index implementation

#include "index/graph_index.h"
#include "index/connection_guard.h"  // Phase 3 A-6: Connection leak prevention
#include <stdexcept>
#include "storage/rocksdb_wrapper.h"
#include "storage/key_schema.h"
#include "storage/base_entity.h"
#include "utils/logger.h"
#include "security/encryption.h"
#include "utils/audit_logger.h"  // Phase 1: Knowledge Graph Protection

#include <queue>
#include <shared_mutex>
#include <unordered_set>
#include <algorithm>
#include <chrono>

namespace themis {

namespace {

std::optional<int64_t> parseTemporalFieldForEdge(const BaseEntity& edge,
                                                 std::string_view field,
                                                 std::string_view edge_id,
                                                 std::string_view context) {
	auto as_int = edge.getFieldAsInt(field);
	if (as_int.has_value()) return as_int;
	auto as_str = edge.getFieldAsString(field);
	if (!as_str.has_value()) return std::nullopt;
	try {
		size_t pos = 0;
		int64_t parsed = std::stoll(*as_str, &pos, 10);
		if (pos == as_str->size()) return parsed;
	} catch (const std::exception& e) {
		THEMIS_DEBUG("{}: invalid temporal field '{}' on edge {}: {}", context, field, edge_id, e.what());
	} catch (...) {
		THEMIS_DEBUG("{}: invalid temporal field '{}' on edge {} with unknown error", context, field, edge_id);
	}
	return std::nullopt;
}

} // namespace

// static
std::vector<uint8_t> GraphIndexManager::toBytes(std::string_view sv) {
	return std::vector<uint8_t>(sv.begin(), sv.end());
}

GraphIndexManager::GraphIndexManager(RocksDBWrapper& db) : db_(db) {}

// Phase 1: Set audit logger for tracking graph operations
void GraphIndexManager::setAuditLogger(std::shared_ptr<utils::AuditLogger> logger, std::string user_context) {
	audit_logger_ = std::move(logger);
	user_context_ = std::move(user_context);
}

void GraphIndexManager::setUserContext(std::string user_id) {
	user_context_ = std::move(user_id);
}

// Phase 4: Set expression evaluator for advanced filtering
void GraphIndexManager::setExpressionEvaluator(std::shared_ptr<IExpressionEvaluator> evaluator) {
	expression_evaluator_ = std::move(evaluator);
}

std::shared_ptr<IExpressionEvaluator> GraphIndexManager::getExpressionEvaluator() const {
	return expression_evaluator_;
}

// Helper: Log audit event if audit logger is configured
void GraphIndexManager::logAuditEvent_(const std::string& event_type, const std::string& resource,
                                       const std::string& operation, size_t count, int depth) const {
	if (!audit_logger_) return;
	
	try {
		nlohmann::json details = {
			{"operation", operation},
			{"resource", resource}
		};
		
		if (count > 0) {
			details["count"] = count;
		}
		if (depth > 0) {
			details["depth"] = depth;
		}
		
		// Map event_type string to SecurityEventType enum
		utils::SecurityEventType event;
		if (event_type == "GRAPH_TRAVERSAL") {
			event = utils::SecurityEventType::GRAPH_TRAVERSAL;
		} else if (event_type == "BULK_NODE_ACCESS") {
			event = utils::SecurityEventType::BULK_NODE_ACCESS;
		} else if (event_type == "BULK_EDGE_ACCESS") {
			event = utils::SecurityEventType::BULK_EDGE_ACCESS;
		} else if (event_type == "TEMPORAL_QUERY") {
			event = utils::SecurityEventType::TEMPORAL_QUERY;
		} else {
			event = utils::SecurityEventType::CUSTOM_EVENT;
			details["custom_event_type"] = event_type;
		}
		
		audit_logger_->logSecurityEvent(event, user_context_, resource, details);
	} catch (const std::exception& e) {
		// Don't fail graph operations if audit logging fails
		THEMIS_WARN("Failed to log audit event: {}", e.what());
	}
}

std::unique_ptr<RocksDBWrapper::WriteBatchWrapper> GraphIndexManager::createWriteBatch() {
	return db_.createWriteBatch();
}

GraphIndexManager::Status GraphIndexManager::addEdge(const BaseEntity& edge) {
	if (!db_.isOpen()) return Status::Error("addEdge: Datenbank ist nicht geöffnet");

	auto eidOpt = edge.getFieldAsString("id");
	auto fromOpt = edge.getFieldAsString("_from");
	auto toOpt = edge.getFieldAsString("_to");
	if (!eidOpt || !fromOpt || !toOpt) {
		return Status::Error("addEdge: Felder 'id', '_from' und '_to' sind erforderlich");
	}

	// QW-45 Guard: Fail-closed validation for empty _from/_to fields
	if (fromOpt->empty()) {
		THEMIS_ERROR("QW-45 Guard: Edge rejected - _from field is empty (fail-closed)");
		return Status::Error("addEdge: QW-45 Guard - _from node ID cannot be empty");
	}
	if (toOpt->empty()) {
		THEMIS_ERROR("QW-45 Guard: Edge rejected - _to field is empty (fail-closed)");
		return Status::Error("addEdge: QW-45 Guard - _to node ID cannot be empty");
	}
	if (eidOpt->empty()) {
		THEMIS_ERROR("QW-45 Guard: Edge rejected - id field is empty (fail-closed)");
		return Status::Error("addEdge: QW-45 Guard - edge ID cannot be empty");
	}

	// Variablen verwendet in addEdge-Überladung
	[[maybe_unused]] const std::string& eid = *eidOpt;
	[[maybe_unused]] const std::string& from = *fromOpt;
	[[maybe_unused]] const std::string& to = *toOpt;

	auto batch = db_.createWriteBatch();
	if (!batch) return Status::Error("addEdge: Konnte WriteBatch nicht erstellen");
	auto st = addEdge(edge, *batch);
	if (!st.ok) { batch->rollback(); return st; }
	if (!batch->commit()) return Status::Error("addEdge: Commit des Batches fehlgeschlagen");
	return Status::OK();
}

GraphIndexManager::Status GraphIndexManager::deleteEdge(std::string_view edgeId) {
	if (!db_.isOpen()) return Status::Error("deleteEdge: Datenbank ist nicht geöffnet");
	if (edgeId.empty()) return Status::Error("deleteEdge: edgeId darf nicht leer sein");

	// Edge laden, um _from/_to zu ermitteln
	const std::string edgeKey = KeySchema::makeGraphEdgeKey(edgeId);
	auto blob = db_.get(edgeKey);
	if (!blob) {
		// Idempotent: nichts zu löschen
		return Status::OK();
	}

	BaseEntity e = BaseEntity::deserialize(std::string(edgeId), *blob);
	auto fromOpt = e.getFieldAsString("_from");
	auto toOpt = e.getFieldAsString("_to");
	if (!fromOpt || !toOpt) {
		// Inkonsistente Kante, löschen wir zumindest den Edge-Key
		db_.del(edgeKey);
		return Status::Error("deleteEdge: Edge hat keine _from/_to Felder (inkonsistent)");
	}

	auto batch = db_.createWriteBatch();
	if (!batch) return Status::Error("deleteEdge: Konnte WriteBatch nicht erstellen");
	auto st = deleteEdge(edgeId, *batch);
	if (!st.ok) { batch->rollback(); return st; }
	if (!batch->commit()) return Status::Error("deleteEdge: Commit des Batches fehlgeschlagen");
	return Status::OK();
}

GraphIndexManager::Status GraphIndexManager::addEdge(const BaseEntity& edge, RocksDBWrapper::WriteBatchWrapper& batch) {
	// Phase 3 A-6: Connection safety verified
	// WriteBatch is RAII-compliant: automatically released on scope exit or exception.
	// No manual connection management needed.
	
	auto eidOpt = edge.getFieldAsString("id");
	auto fromOpt = edge.getFieldAsString("_from");
	auto toOpt = edge.getFieldAsString("_to");
	if (!eidOpt || !fromOpt || !toOpt) return Status::Error("addEdge(tx): Felder 'id', '_from', '_to' fehlen");
	
	// QW-45 Guard: Fail-closed validation for empty _from/_to fields
	if (fromOpt->empty()) {
		THEMIS_ERROR("QW-45 Guard: Edge rejected in WriteBatch - _from field is empty (fail-closed)");
		return Status::Error("addEdge(batch): QW-45 Guard - _from node ID cannot be empty");
	}
	if (toOpt->empty()) {
		THEMIS_ERROR("QW-45 Guard: Edge rejected in WriteBatch - _to field is empty (fail-closed)");
		return Status::Error("addEdge(batch): QW-45 Guard - _to node ID cannot be empty");
	}
	if (eidOpt->empty()) {
		THEMIS_ERROR("QW-45 Guard: Edge rejected in WriteBatch - id field is empty (fail-closed)");
		return Status::Error("addEdge(batch): QW-45 Guard - edge ID cannot be empty");
	}
	
	const std::string& eid = *eidOpt; const std::string& from = *fromOpt; const std::string& to = *toOpt;
	// If FieldEncryption is configured, optionally encrypt fields listed in
	// `encrypt_fields` (preferred) or fall back to legacy `_sensitive` boolean.
	BaseEntity stored = edge; // make a mutable copy
	try {
		if (field_encryption_) {
			std::vector<std::string> encryptList;

			// Preferred: encrypt_fields as JSON-string or comma-separated list
			auto encOpt = edge.getFieldAsString("encrypt_fields");
			if (encOpt) {
				try {
					auto j = nlohmann::json::parse(*encOpt);
					if (j.is_array()) {
						for (const auto& v : j) if (v.is_string()) encryptList.push_back(v.get<std::string>());
					}
				} catch (const std::exception& e) {
					THEMIS_DEBUG("addEdge: failed to parse encrypt_fields JSON, using CSV fallback: {}", e.what());
				} catch (...) {
					THEMIS_DEBUG("addEdge: failed to parse encrypt_fields JSON with unknown error, using CSV fallback");
					// Fallback: comma-separated
					std::string s = *encOpt;
					size_t start = 0;
					while (start < s.size()) {
						auto pos = s.find(',', start);
						std::string part = (pos == std::string::npos) ? s.substr(start) : s.substr(start, pos - start);
						// trim
						auto l = part.find_first_not_of(" \t\n\r");
						auto r = part.find_last_not_of(" \t\n\r");
						if (l != std::string::npos && r != std::string::npos) encryptList.push_back(part.substr(l, r - l + 1));
						if (pos == std::string::npos) break;
						start = pos + 1;
					}
				}
			}

			// A-2.6: Safe vector growth during iteration
			// Using index-based parsing (size_t start) allows safe push_back to encryptList
			// without iterator invalidation concerns
			// LEGACY PATH (requires human approval — INDEX-AUD-GI-01)
			// Reason: _sensitive boolean field predates encrypt_fields list (introduced in v2.1);
			//   existing edge documents written before v2.1 may only carry _sensitive=true.
			// Activation: addEdge path when encryptList is empty and _sensitive==true is present.
			// Primary Delta: v2.1+ documents use encrypt_fields list; this fallback is not used for new writes.
			// Approved By: Index module maintainer — pre-existing legacy compat path (INDEX-AUD-GI-01)
			// Removal Target: v2.6.0 (after data migration confirms no _sensitive=true records remain)
			// Backwards compat: if no explicit list and _sensitive==true, encrypt weight+metadata
			if (encryptList.empty()) {
				auto sensitiveOpt = edge.getFieldAsBool("_sensitive");
				if (sensitiveOpt && *sensitiveOpt) {
					encryptList.push_back("_weight");
					encryptList.push_back("metadata");
				}
			}

			// Perform encryption for listed fields
			for (const auto& fld : encryptList) {
				if (!edge.hasField(fld)) continue;
				if (fld == "_weight") {
					// Prefer numeric weight
					if (auto wOpt = edge.getFieldAsDouble("_weight"); wOpt) {
						std::string wstr = std::to_string(*wOpt);
						auto blob = field_encryption_->encrypt(wstr, "edges.weight");
						stored.setField("_weight", std::string(blob.toBase64()));
						continue;
					}
					// Fallback: string/int
					if (auto wStr = edge.getFieldAsString("_weight"); wStr) {
						auto blob = field_encryption_->encrypt(*wStr, "edges.weight");
						stored.setField("_weight", std::string(blob.toBase64()));
						continue;
					}
				} else if (fld == "metadata") {
					if (auto metaOpt = edge.getFieldAsString("metadata"); metaOpt) {
						auto blob = field_encryption_->encrypt(*metaOpt, "edges.metadata");
						stored.setField("metadata", std::string(blob.toBase64()));
						continue;
					}
				} else {
					// Generic: try string field
					if (auto sval = edge.getFieldAsString(fld); sval) {
						auto blob = field_encryption_->encrypt(*sval, std::string("edges.") + fld);
						stored.setField(fld, std::string(blob.toBase64()));
						continue;
					}
					// Try binary blobs
					if (auto bin = edge.getField(fld); bin && std::holds_alternative<std::vector<uint8_t>>(*bin)) {
						auto& vec = std::get<std::vector<uint8_t>>(*bin);
						auto blob = field_encryption_->encrypt(vec, std::string("edges.") + fld);
						stored.setField(fld, std::string(blob.toBase64()));
						continue;
					}
				}
			}
		}
	} catch (const std::exception& e) {
		THEMIS_ERROR("addEdge: encryption failed: {}", e.what());
		return Status::Error(std::string("addEdge: encryption failed: ") + e.what());
	}

	// Extract graphId from edge (optional field)
	std::string graphId = edge.getFieldAsString("_graph").value_or("");

	{
		// LOCK: Tier 1 (Global topology protection) — Phase 3 A-5
		std::lock_guard<std::shared_mutex> lock(topology_mutex_);
		// Edge speichern (Primärspeicher)
		batch.put(KeySchema::makeGraphEdgeKey(eid), stored.serialize());
		// Adjazenz-Indizes
		batch.put(KeySchema::makeGraphOutdexKey(from, eid), toBytes(to));
		batch.put(KeySchema::makeGraphIndexKey(to, eid), toBytes(from));

		// Update in-memory topology if loaded
		if (topologyLoaded_.load(std::memory_order_acquire)) {
			addEdgeToTopologyUnlocked_(eid, from, to, graphId);
		}
	}

	return Status::OK();
}

GraphIndexManager::Status GraphIndexManager::deleteEdge(std::string_view edgeId, RocksDBWrapper::WriteBatchWrapper& batch) {
	if (edgeId.empty()) return Status::Error("deleteEdge(tx): edgeId leer");
	const std::string edgeKey = KeySchema::makeGraphEdgeKey(edgeId);
	auto blob = db_.get(edgeKey);
	if (!blob) return Status::OK();
	BaseEntity e = BaseEntity::deserialize(std::string(edgeId), *blob);
	auto fromOpt = e.getFieldAsString("_from");
	auto toOpt = e.getFieldAsString("_to");
	std::string graphId = e.getFieldAsString("_graph").value_or("");
	{
		// LOCK: Tier 1 (Global topology protection) — Phase 3 A-5
		std::lock_guard<std::shared_mutex> lock(topology_mutex_);
		batch.del(edgeKey);
		if (fromOpt && toOpt) {
			batch.del(KeySchema::makeGraphOutdexKey(*fromOpt, std::string(edgeId)));
			batch.del(KeySchema::makeGraphIndexKey(*toOpt, std::string(edgeId)));

			// Update in-memory topology if loaded
			if (topologyLoaded_.load(std::memory_order_acquire)) {
				removeEdgeFromTopologyUnlocked_(std::string(edgeId), *fromOpt, *toOpt, graphId);
			}

			return Status::OK();
		}
	}
	return Status::Error("deleteEdge(tx): _from/_to fehlen (inkonsistent)");
}

std::pair<GraphIndexManager::Status, std::vector<std::string>>
GraphIndexManager::outNeighbors(std::string_view fromPk) const {
	if (!db_.isOpen()) return {Status::Error("outNeighbors: Datenbank ist nicht geöffnet"), {}};

	// Use in-memory topology if available (O(1) lookup)
	if (topologyLoaded_.load(std::memory_order_acquire)) {
		// LOCK: Tier 1 (Global topology protection, read-only) — Phase 3 A-5
		std::shared_lock<std::shared_mutex> lock(topology_mutex_);
		std::vector<std::string> result;
		auto it = outEdges_.find(std::string(fromPk));
		if (it != outEdges_.end()) {
			result.reserve(it->second.size());
			for (const auto& adj : it->second) {
				result.push_back(adj.targetPk);
			}
		}
		return {Status::OK(), std::move(result)};
	}

	// Fallback to RocksDB scan (O(log N))
	std::vector<std::string> result;
	const std::string prefix = std::string("graph:out:") + std::string(fromPk) + ":";
	db_.scanPrefix(prefix, [&result](std::string_view /*key*/, std::string_view val){
		result.emplace_back(std::string(val));
		return true;
	});
	
	// Phase 1: Audit log for bulk node access (threshold: 100+ neighbors)
	if (result.size() >= 100) {
		logAuditEvent_("BULK_NODE_ACCESS", std::string(fromPk), "outNeighbors", result.size(), 0);
	}
	
	return {Status::OK(), std::move(result)};
}

std::pair<GraphIndexManager::Status, std::vector<std::string>>
GraphIndexManager::inNeighbors(std::string_view toPk) const {
	if (!db_.isOpen()) return {Status::Error("inNeighbors: Datenbank ist nicht geöffnet"), {}};

	// Use in-memory topology if available (O(1) lookup)
	if (topologyLoaded_.load(std::memory_order_acquire)) {
		// LOCK: Tier 1 (Global topology protection, read-only) — Phase 3 A-5
		std::shared_lock<std::shared_mutex> lock(topology_mutex_);
		std::vector<std::string> result;
		auto it = inEdges_.find(std::string(toPk));
		if (it != inEdges_.end()) {
			result.reserve(it->second.size());
			for (const auto& adj : it->second) {
				result.push_back(adj.targetPk);
			}
		}
		return {Status::OK(), std::move(result)};
	}

	// Fallback to RocksDB scan (O(log N))
	std::vector<std::string> result;
	const std::string prefix = std::string("graph:in:") + std::string(toPk) + ":";
	db_.scanPrefix(prefix, [&result](std::string_view /*key*/, std::string_view val){
		result.emplace_back(std::string(val));
		return true;
	});
	return {Status::OK(), std::move(result)};
}

std::pair<GraphIndexManager::Status, std::vector<GraphIndexManager::AdjacencyInfo>>
GraphIndexManager::outAdjacency(std::string_view fromPk) const {
	if (!db_.isOpen()) return {Status::Error("outAdjacency: Datenbank ist nicht geöffnet"), {}};

	// In-Memory schnellpfad
	if (topologyLoaded_.load(std::memory_order_acquire)) {
		// LOCK: Tier 1 (Global topology protection, read-only) — Phase 3 A-5
		std::shared_lock<std::shared_mutex> lock(topology_mutex_);
		std::vector<AdjacencyInfo> result;
		auto it = outEdges_.find(std::string(fromPk));
		if (it != outEdges_.end()) {
			result = it->second; // Kopie bewusst
		}
		return {Status::OK(), std::move(result)};
	}

	// Fallback: RocksDB-Scan – EdgeId aus Key extrahieren
	std::vector<AdjacencyInfo> result;
	const std::string prefix = std::string("graph:out:") + std::string(fromPk) + ":";
	db_.scanPrefix(prefix, [&result](std::string_view key, std::string_view val){
		std::string keyStr(key);
		size_t lastColon = keyStr.rfind(':');
		if (lastColon != std::string::npos) {
			std::string edgeId = keyStr.substr(lastColon + 1);
			result.push_back({edgeId, std::string(val)});
		}
		return true;
	});
	return {Status::OK(), std::move(result)};
}

std::pair<GraphIndexManager::Status, std::vector<GraphIndexManager::AdjacencyInfo>>
GraphIndexManager::inAdjacency(std::string_view toPk) const {
	if (!db_.isOpen()) return {Status::Error("inAdjacency: Datenbank ist nicht geöffnet"), {}};

	if (topologyLoaded_.load(std::memory_order_acquire)) {
		// LOCK: Tier 1 (Global topology protection, read-only) — Phase 3 A-5
		std::shared_lock<std::shared_mutex> lock(topology_mutex_);
		std::vector<AdjacencyInfo> result;
		auto it = inEdges_.find(std::string(toPk));
		if (it != inEdges_.end()) {
			result = it->second; // Kopie bewusst (edgeId, fromPk)
		}
		return {Status::OK(), std::move(result)};
	}

	// Fallback: RocksDB-Scan – EdgeId aus Key extrahieren
	std::vector<AdjacencyInfo> result;
	const std::string prefix = std::string("graph:in:") + std::string(toPk) + ":";
	db_.scanPrefix(prefix, [&result](std::string_view key, std::string_view val){
		std::string keyStr(key);
		size_t lastColon = keyStr.rfind(':');
		if (lastColon != std::string::npos) {
			std::string edgeId = keyStr.substr(lastColon + 1);
			// hier ist val = fromPk
			result.push_back({edgeId, std::string(val)});
		}
		return true;
	});
	return {Status::OK(), std::move(result)};
}

std::pair<GraphIndexManager::Status, std::vector<std::string>>
GraphIndexManager::bfs(std::string_view startPk, int maxDepth) const {
	if (!db_.isOpen()) return {Status::Error("bfs: Datenbank ist nicht geöffnet"), {}};
	if (startPk.empty()) return {Status::Error("bfs: startPk darf nicht leer sein"), {}};
	if (maxDepth < 0) return {Status::Error("bfs: maxDepth muss >= 0 sein"), {}};

	std::vector<std::string> order;
	std::unordered_set<std::string> visited;
	std::queue<std::pair<std::string,int>> q;

	q.emplace(std::string(startPk), 0);
	visited.insert(std::string(startPk));

	// Use in-memory topology for faster BFS if available
	if (topologyLoaded_.load(std::memory_order_acquire)) {
		while (!q.empty()) {
			auto [node, depth] = q.front();
			q.pop();

			order.push_back(node);
			if (depth == maxDepth) continue;

   // LOCK: Tier 1 (Global topology protection, read-only) — Phase 3 A-5
			std::shared_lock<std::shared_mutex> lock(topology_mutex_);
			auto it = outEdges_.find(node);
			if (it != outEdges_.end()) {
				for (const auto& adj : it->second) {
					// Non-typed BFS: traverse all graphs
					if (!visited.count(adj.targetPk)) {
						visited.insert(adj.targetPk);
						q.emplace(adj.targetPk, depth + 1);
					}
				}
			}
		}
		return {Status::OK(), std::move(order)};
	}

	// Fallback to RocksDB scan-based BFS (all graphs)
	while (!q.empty()) {
		auto [node, depth] = q.front();
		q.pop();

		order.push_back(node);
		if (depth == maxDepth) continue;

		const std::string prefix = std::string("graph:out:");
		db_.scanPrefix(prefix, [&](std::string_view key, std::string_view val){
			// Robust parsing that supports both formats:
			// - graph:out:<graphId>:<fromPk>:<edgeId>
			// - graph:out:<fromPk>:<edgeId>
			std::string keyStr(key);
			size_t lastColon = keyStr.rfind(':');
			if (lastColon == std::string::npos) return true;
			const size_t prefixLen = std::string("graph:out:").size();
			if (keyStr.size() <= prefixLen) return true;
			std::string middle = keyStr.substr(prefixLen, lastColon - prefixLen);
			size_t innerColon = middle.rfind(':');
			std::string fromPk = (innerColon == std::string::npos) ? middle : middle.substr(innerColon + 1);
			if (fromPk != node) return true;

			std::string neigh(val);
			if (!visited.count(neigh)) {
				visited.insert(neigh);
				q.emplace(neigh, depth + 1);
			}
			return true;
		});
	}
	
	// Phase 1: Audit log for graph traversal
	logAuditEvent_("GRAPH_TRAVERSAL", std::string(startPk), "bfs", order.size(), maxDepth);
	
	return {Status::OK(), std::move(order)};
}

// BFS with edge type filtering and graph scope (server-side)
std::pair<GraphIndexManager::Status, std::vector<std::string>>
GraphIndexManager::bfs(std::string_view startPk, int maxDepth, std::string_view edge_type, std::string_view graph_id) const {
	if (!db_.isOpen()) return {Status::Error("bfs: Datenbank ist nicht geöffnet"), {}};
	if (startPk.empty()) return {Status::Error("bfs: startPk darf nicht leer sein"), {}};
	if (maxDepth < 0) return {Status::Error("bfs: maxDepth muss >= 0 sein"), {}};

	std::vector<std::string> order;
	std::unordered_set<std::string> visited;
	std::queue<std::pair<std::string,int>> q;

	q.emplace(std::string(startPk), 0);
	visited.insert(std::string(startPk));

	std::string typeFilter(edge_type);
	std::string graphFilter(graph_id);

	// Use in-memory topology for faster BFS if available
	if (topologyLoaded_.load(std::memory_order_acquire)) {
		while (!q.empty()) {
			auto [node, depth] = q.front();
			q.pop();

			order.push_back(node);
			if (depth == maxDepth) continue;

   // LOCK: Tier 1 (Global topology protection, read-only) — Phase 3 A-5
			std::shared_lock<std::shared_mutex> lock(topology_mutex_);
			auto it = outEdges_.find(node);
			if (it != outEdges_.end()) {
				for (const auto& adj : it->second) {
					// Filter by graph and edge type
					if (!graphFilter.empty() && adj.graphId != graphFilter) continue;
					std::string edgeType = getEdgeType_(adj.graphId, adj.edgeId);
					if (!typeFilter.empty() && edgeType != typeFilter) {
						continue; // Skip edges with different type
					}

					if (!visited.count(adj.targetPk)) {
						visited.insert(adj.targetPk);
						q.emplace(adj.targetPk, depth + 1);
					}
				}
			}
		}
		return {Status::OK(), std::move(order)};
	}

	// Fallback to RocksDB scan-based BFS
	while (!q.empty()) {
		auto [node, depth] = q.front();
		q.pop();

		order.push_back(node);
		if (depth == maxDepth) continue;

		if (graphFilter.empty()) {
			// graph_id required for on-disk scan in multi-graph mode
			return {Status::Error("bfs: graph_id required for scan without topology"), {}};
		}
		const std::string prefix = std::string("graph:out:") + graphFilter + ":" + std::string(node) + ":";
		db_.scanPrefix(prefix, [&](std::string_view key, std::string_view val){
			// Extract edgeId from key
			std::string gid, from, edgeId;
			if (!parseOutKey_(key, gid, from, edgeId)) return true;

			// Filter by edge type
			std::string edgeType = getEdgeType_(gid, edgeId);
			if (!typeFilter.empty() && edgeType != typeFilter) {
				return true; // Skip edges with different type
			}

			std::string neigh(val);
			if (!visited.count(neigh)) {
				visited.insert(neigh);
				q.emplace(neigh, depth + 1);
			}
			return true;
		});
	}
	return {Status::OK(), std::move(order)};
}

// ────────────────────────────────────────────────────────────────────────────
// In-Memory Topology Management
// ────────────────────────────────────────────────────────────────────────────

GraphIndexManager::Status GraphIndexManager::rebuildTopology() {
	if (!db_.isOpen()) return Status::Error("rebuildTopology: Datenbank ist nicht geöffnet");

	// LOCK: Tier 1 (Global topology protection) — Phase 3 A-5
	std::lock_guard<std::shared_mutex> lock(topology_mutex_);

	// Clear existing topology
	outEdges_.clear();
	inEdges_.clear();

	// W5: Materialize edge topology locally to avoid [this] capture and mutable state closure
	std::unordered_map<std::string, std::vector<GraphIndexManager::AdjacencyInfo>> local_out_edges;
	std::unordered_map<std::string, std::vector<GraphIndexManager::AdjacencyInfo>> local_in_edges;

	// Scan all outgoing edges: graph:out:<graph_id>:<fromPk>:<edgeId> -> toPk
	size_t out_scan_count = 0;
	// W5: Non-capturing lambda with explicit local container; db access via outer scope
	db_.scanPrefix("graph:out:", [&local_out_edges, &out_scan_count, this](std::string_view key, std::string_view val) {
		std::string graphId, fromPk, edgeId;
		const std::string keyStr(key);
		const size_t lastColon = keyStr.rfind(':');
		if (lastColon == std::string::npos) return true;
		edgeId = keyStr.substr(lastColon + 1);
		// W5: Use db_ from outer scope through this - limited to data access only
		if (auto blob = db_.get(KeySchema::makeGraphEdgeKey(edgeId)); blob.has_value()) {
			BaseEntity edge = BaseEntity::deserialize(edgeId, *blob);
			fromPk = edge.getFieldAsString("_from").value_or("");
			graphId = edge.getFieldAsString("_graph").value_or("");
		}
		if (fromPk.empty() && !parseOutKey_(key, graphId, fromPk, edgeId)) return true;
		std::string toPk(val);
		// W5: Add to local container, not member; update after scan completes
		local_out_edges[fromPk].push_back({edgeId, toPk, graphId});
		++out_scan_count;
		return true;
	});
	THEMIS_INFO("rebuildTopology: scanned {} outgoing edge keys", out_scan_count);

	// Scan all incoming edges: graph:in:<graph_id>:<toPk>:<edgeId> -> fromPk
	// W5: Non-capturing lambda with explicit local container; db access via outer scope
	db_.scanPrefix("graph:in:", [&local_in_edges, this](std::string_view key, std::string_view val) {
		std::string graphId, toPk, edgeId;
		const std::string keyStr(key);
		const size_t lastColon = keyStr.rfind(':');
		if (lastColon == std::string::npos) return true;
		edgeId = keyStr.substr(lastColon + 1);
		// W5: Use db_ from outer scope through this - limited to data access only
		if (auto blob = db_.get(KeySchema::makeGraphEdgeKey(edgeId)); blob.has_value()) {
			BaseEntity edge = BaseEntity::deserialize(edgeId, *blob);
			toPk = edge.getFieldAsString("_to").value_or("");
			graphId = edge.getFieldAsString("_graph").value_or("");
		}
		if (toPk.empty() && !parseInKey_(key, graphId, toPk, edgeId)) return true;
		std::string fromPk(val);
		// W5: Add to local container, not member; update after scan completes
		local_in_edges[toPk].push_back({edgeId, fromPk, graphId});
		return true;
	});

	// W5: Update members only after scan completes; preserve atomic topologyLoaded_ behavior
	outEdges_ = std::move(local_out_edges);
	inEdges_ = std::move(local_in_edges);

	topologyLoaded_.store(true, std::memory_order_release);
	return Status::OK();
}

void GraphIndexManager::addEdgeToTopology_(const std::string& edgeId, const std::string& fromPk, const std::string& toPk, const std::string& graphId) {
 // LOCK: Tier 1 (Global topology protection) — Phase 3 A-5
	std::lock_guard<std::shared_mutex> lock(topology_mutex_);
	addEdgeToTopologyUnlocked_(edgeId, fromPk, toPk, graphId);
}

void GraphIndexManager::removeEdgeFromTopology_(const std::string& edgeId, const std::string& fromPk, const std::string& toPk, [[maybe_unused]] const std::string& graphId) {
 // LOCK: Tier 1 (Global topology protection) — Phase 3 A-5
	std::lock_guard<std::shared_mutex> lock(topology_mutex_);
	removeEdgeFromTopologyUnlocked_(edgeId, fromPk, toPk, graphId);
}

void GraphIndexManager::addEdgeToTopologyUnlocked_(const std::string& edgeId, const std::string& fromPk, const std::string& toPk, const std::string& graphId) {
	outEdges_[fromPk].push_back({edgeId, toPk, graphId});
	inEdges_[toPk].push_back({edgeId, fromPk, graphId});
}

void GraphIndexManager::removeEdgeFromTopologyUnlocked_(const std::string& edgeId, const std::string& fromPk, const std::string& toPk, [[maybe_unused]] const std::string& graphId) {
	// Remove from outEdges_
	auto outIt = outEdges_.find(fromPk);
	if (outIt != outEdges_.end()) {
		auto& vec = outIt->second;
		vec.erase(std::remove_if(vec.begin(), vec.end(),
			[&edgeId](const AdjacencyInfo& info) { return info.edgeId == edgeId; }),
			vec.end());
		if (vec.empty()) outEdges_.erase(outIt);
	}

	// Remove from inEdges_
	auto inIt = inEdges_.find(toPk);
	if (inIt != inEdges_.end()) {
		auto& vec = inIt->second;
		vec.erase(std::remove_if(vec.begin(), vec.end(),
			[&edgeId](const AdjacencyInfo& info) { return info.edgeId == edgeId; }),
			vec.end());
		if (vec.empty()) inEdges_.erase(inIt);
	}
}

size_t GraphIndexManager::getTopologyNodeCount() const {
 // LOCK: Tier 1 (Global topology protection, read-only) — Phase 3 A-5
	std::shared_lock<std::shared_mutex> lock(topology_mutex_);
	std::unordered_set<std::string> nodes;
	for (const auto& [node, _] : outEdges_) nodes.insert(node);
	for (const auto& [node, _] : inEdges_) nodes.insert(node);
	return nodes.size();
}

std::pair<GraphIndexManager::Status, std::vector<std::string>>
GraphIndexManager::allVertices() const {
	{
  // LOCK: Tier 1 (Global topology protection, read-only) — Phase 3 A-5
		std::shared_lock<std::shared_mutex> lock(topology_mutex_);
		if (topologyLoaded_.load(std::memory_order_acquire)) {
			// Fast path: in-memory topology is populated.
			std::unordered_set<std::string> seen;
			for (const auto& [v, _] : outEdges_) seen.insert(v);
			for (const auto& [v, _] : inEdges_)  seen.insert(v);
			return {Status::OK(), std::vector<std::string>(seen.begin(), seen.end())};
		}
	}
	// Slow path: topology not yet in memory – enumerate vertices directly from
	// RocksDB by scanning the outdex and index key prefixes.
	std::unordered_set<std::string> seen;
	constexpr std::string_view kOutPrefix = "graph:out:";
	constexpr std::string_view kInPrefix  = "graph:in:";
	db_.scanPrefix(std::string(kOutPrefix), [&seen, kOutPrefix](std::string_view key, std::string_view /*val*/) {
		// key format: "graph:out:<fromPk>:<edgeId>"
		//          or "graph:out:<graphId>:<fromPk>:<edgeId>"
		// We want fromPk. Use the same logic as parseOutKey_:
		//   strip "graph:out:" prefix, then split on ':'
		const std::string_view tail = key.substr(kOutPrefix.size());
		const size_t first = tail.find(':');
		if (first == std::string_view::npos) return true;
		const size_t last  = tail.rfind(':');
		std::string fromPk;
		if (last == first) {
			// LEGACY PATH (requires human approval — INDEX-AUD-GI-03): pre-v2.0 key format without graphId segment
			// Reason: RocksDB keys written before v2.0 lack the graphId segment; must read both formats.
			// Activation: key parsing when last==first (no graphId separator present).
			// Primary Delta: v2.0+ keys include graphId; old keys do not.
			// Approved By: Index module maintainer — pre-existing legacy compat path (INDEX-AUD-GI-03)
			// Removal Target: v2.6.0 (after full data migration to v2.0+ key schema)
			fromPk = std::string(tail.substr(0, first));
		} else {
			fromPk = std::string(tail.substr(first + 1, last - first - 1));
		}
		if (!fromPk.empty()) seen.insert(std::move(fromPk));
		return true;
	});
	db_.scanPrefix(std::string(kInPrefix), [&seen, kInPrefix](std::string_view key, std::string_view /*val*/) {
		const std::string_view tail = key.substr(kInPrefix.size());
		const size_t first = tail.find(':');
		if (first == std::string_view::npos) return true;
		const size_t last  = tail.rfind(':');
		std::string toPk;
		if (last == first) {
			toPk = std::string(tail.substr(0, first));
		} else {
			toPk = std::string(tail.substr(first + 1, last - first - 1));
		}
		if (!toPk.empty()) seen.insert(std::move(toPk));
		return true;
	});
	return {Status::OK(), std::vector<std::string>(seen.begin(), seen.end())};
}

size_t GraphIndexManager::getTopologyEdgeCount() const {
 // LOCK: Tier 1 (Global topology protection, read-only) — Phase 3 A-5
	std::shared_lock<std::shared_mutex> lock(topology_mutex_);
	size_t total = 0;
	for (const auto& [_, edges] : outEdges_) {
		total += edges.size();
	}
	return total;
}

std::vector<std::string> GraphIndexManager::getAllVertices() const {
 // LOCK: Tier 1 (Global topology protection, read-only) — Phase 3 A-5
	std::shared_lock<std::shared_mutex> lock(topology_mutex_);
	std::unordered_set<std::string> nodes;
	for (const auto& [node, _] : outEdges_) nodes.insert(node);
	for (const auto& [node, _] : inEdges_) nodes.insert(node);
	return {nodes.begin(), nodes.end()};
}

// ────────────────────────────────────────────────────────────────────────────
// Shortest-Path-Algorithmen
// ────────────────────────────────────────────────────────────────────────────

double GraphIndexManager::getEdgeWeight_(std::string_view graphId, std::string_view edgeId) const {
	// Try both storage formats: with graphId (edge:<graphId>:<edgeId>) and without (edge:<edgeId>)
	const std::string edgeKeyWithGid = std::string("edge:") + std::string(graphId) + ":" + std::string(edgeId);
	auto blob = db_.get(edgeKeyWithGid);
	if (!blob.has_value()) {
		const std::string edgeKey = std::string("edge:") + std::string(edgeId);
		blob = db_.get(edgeKey);
		if (!blob.has_value()) return 1.0; // Default weight
	}

	BaseEntity edge = BaseEntity::deserialize(std::string(edgeId), *blob);
	// Prefer numeric representation
	if (auto weightOpt = edge.getFieldAsDouble("_weight"); weightOpt) return *weightOpt;

	// If stored as string it might be either a plain number or an encrypted blob
	if (auto wstrOpt = edge.getFieldAsString("_weight"); wstrOpt) {
		const std::string& wstr = *wstrOpt;
		// Try to detect encrypted blob and decrypt when possible
		try {
			auto eb = EncryptedBlob::fromBase64(wstr);
			if (field_encryption_) {
				std::string dec = field_encryption_->decryptToString(eb);
				try {
					return std::stod(dec);
				} catch (const std::exception& e) {
					THEMIS_DEBUG("getEdgeWeight_: decrypted _weight is not numeric for edge {}: {}", edgeId, e.what());
					// fallthrough
				} catch (...) {
					THEMIS_DEBUG("getEdgeWeight_: decrypted _weight parse failed for edge {} with unknown error", edgeId);
					// fallthrough
				}
			}
		} catch (const std::exception& e) {
			THEMIS_DEBUG("getEdgeWeight_: _weight is not encrypted/base64 for edge {}: {}", edgeId, e.what());
			// not an encrypted blob
		} catch (...) {
			THEMIS_DEBUG("getEdgeWeight_: _weight decode failed for edge {} with unknown error", edgeId);
			// not an encrypted blob
		}

		// Fallback: attempt to parse as number
		try {
			return std::stod(wstr);
		} catch (const std::exception& e) {
			THEMIS_DEBUG("getEdgeWeight_: _weight is not numeric for edge {}: {}", edgeId, e.what());
			return 1.0;
		} catch (...) {
			THEMIS_DEBUG("getEdgeWeight_: _weight parse failed for edge {} with unknown error", edgeId);
			return 1.0;
		}
	}

	return 1.0;
}

// Public method for retrieving edge weight with custom attribute
double GraphIndexManager::getEdgeWeight(std::string_view graphId, std::string_view edgeId, 
                                       std::string_view weightAttribute) const {
	// Try both storage formats: with graphId (edge:<graphId>:<edgeId>) and without (edge:<edgeId>)
	const std::string edgeKeyWithGid = std::string("edge:") + std::string(graphId) + ":" + std::string(edgeId);
	auto blob = db_.get(edgeKeyWithGid);
	if (!blob.has_value()) {
		const std::string edgeKey = std::string("edge:") + std::string(edgeId);
		blob = db_.get(edgeKey);
		if (!blob.has_value()) return 1.0; // Default weight
	}

	BaseEntity edge = BaseEntity::deserialize(std::string(edgeId), *blob);
	std::string attrName = std::string(weightAttribute);
	
	// Prefer numeric representation
	if (auto weightOpt = edge.getFieldAsDouble(attrName); weightOpt) return *weightOpt;

	// If stored as string it might be either a plain number or an encrypted blob
	if (auto wstrOpt = edge.getFieldAsString(attrName); wstrOpt) {
		const std::string& wstr = *wstrOpt;
		// Try to detect encrypted blob and decrypt when possible
		try {
			auto eb = EncryptedBlob::fromBase64(wstr);
			if (field_encryption_) {
				std::string dec = field_encryption_->decryptToString(eb);
				try {
					return std::stod(dec);
				} catch (const std::exception& e) {
					THEMIS_DEBUG("getEdgeWeight: decrypted {} is not numeric for edge {}: {}", attrName, edgeId, e.what());
					// fallthrough
				} catch (...) {
					THEMIS_DEBUG("getEdgeWeight: decrypted {} parse failed for edge {} with unknown error", attrName, edgeId);
					// fallthrough
				}
			}
		} catch (const std::exception& e) {
			THEMIS_DEBUG("getEdgeWeight: {} is not encrypted/base64 for edge {}: {}", attrName, edgeId, e.what());
			// not an encrypted blob
		} catch (...) {
			THEMIS_DEBUG("getEdgeWeight: {} decode failed for edge {} with unknown error", attrName, edgeId);
			// not an encrypted blob
		}

		// Fallback: attempt to parse as number
		try {
			return std::stod(wstr);
		} catch (const std::exception& e) {
			THEMIS_DEBUG("getEdgeWeight: {} is not numeric for edge {}: {}", attrName, edgeId, e.what());
			return 1.0;
		} catch (...) {
			THEMIS_DEBUG("getEdgeWeight: {} parse failed for edge {} with unknown error", attrName, edgeId);
			return 1.0;
		}
	}

	return 1.0;
}

std::optional<std::string> GraphIndexManager::getEdgeField(
	std::string_view edgeId, std::string_view fieldName) const {
	// Try both storage formats: "edge:<edgeId>" and, for in-memory topology keys,
	// iterate over all known graphIds.  Prefer the format without graphId for
	// simplicity (matches the common single-graph use-case).
	const std::string edgeKey = std::string("edge:") + std::string(edgeId);
	auto blob = db_.get(edgeKey);
	if (!blob.has_value()) {
		// Try the topology keys in the in-memory edge map to find a graphId
  // LOCK: Tier 1 (Global topology protection, read-only) — Phase 3 A-5
		std::shared_lock<std::shared_mutex> lk(topology_mutex_);
		for (const auto& [from, adj_list] : outEdges_) {
			for (const auto& adj : adj_list) {
				if (adj.edgeId == edgeId && !adj.graphId.empty()) {
					const std::string edgeKeyWithGid =
						std::string("edge:") + adj.graphId + ":" + std::string(edgeId);
					blob = db_.get(edgeKeyWithGid);
					if (blob.has_value()) break;
				}
			}
			if (blob.has_value()) break;
		}
	}
	if (!blob.has_value()) return std::nullopt;

	BaseEntity edge = BaseEntity::deserialize(std::string(edgeId), *blob);
	return edge.getFieldAsString(fieldName);
}

std::optional<std::string> GraphIndexManager::getNodeField(
	std::string_view vertexId, std::string_view fieldName) const {
	const std::string nodeKey = KeySchema::makeGraphNodeKey(vertexId);
	auto blob = db_.get(nodeKey);
	if (!blob.has_value()) return std::nullopt;
	BaseEntity vertex = BaseEntity::deserialize(std::string(vertexId), *blob);
	return vertex.getFieldAsString(fieldName);
}

std::string GraphIndexManager::getEdgeType_(std::string_view graphId, std::string_view edgeId) const {
	// Try both storage formats: with graphId (edge:<graphId>:<edgeId>) and without (edge:<edgeId>)
	const std::string edgeKeyWithGid = std::string("edge:") + std::string(graphId) + ":" + std::string(edgeId);
	auto blob = db_.get(edgeKeyWithGid);
	if (!blob.has_value()) {
		const std::string edgeKey = std::string("edge:") + std::string(edgeId);
		blob = db_.get(edgeKey);
		if (!blob.has_value()) return ""; // No type
	}

	BaseEntity edge = BaseEntity::deserialize(std::string(edgeId), *blob);
	if (auto typeOpt = edge.getFieldAsString("_type"); typeOpt) {
		const std::string& t = *typeOpt;
		// Try decrypting if it's an encrypted blob
		try {
			auto eb = EncryptedBlob::fromBase64(t);
			if (field_encryption_) {
				return field_encryption_->decryptToString(eb);
			}
		} catch (const std::exception& e) {
			THEMIS_DEBUG("getEdgeType_: _type decode/decrypt failed for edge {}: {}", edgeId, e.what());
			// not an encrypted blob
		} catch (...) {
			THEMIS_DEBUG("getEdgeType_: _type decode/decrypt failed for edge {} with unknown error", edgeId);
			// not an encrypted blob
		}
		return t;
	}
	return std::string();
}

bool GraphIndexManager::parseOutKey_(std::string_view key, std::string& graphId, std::string& fromPk, std::string& edgeId) {
	// Expect: graph:out:<graph_id>:<fromPk>:<edgeId>
	if (key.rfind("graph:out:", 0) != 0) return false;
	std::string s(key.substr(10)); // after prefix
	size_t first = s.find(':');
	if (first == std::string::npos) return false;
	size_t last = s.rfind(':');
	// Support two formats:
	// - graph:out:<graphId>:<fromPk>:<edgeId>
	// - graph:out:<fromPk>:<edgeId>  (legacy)
	if (last == first) {
		// LEGACY PATH (requires human approval — INDEX-AUD-GI-03): pre-v2.0 key format (graph:out)
		// Reason: same as above — old keys lack graphId segment.
		// Activation: out-key parsing when last==first.
		// Primary Delta: parses graph:out:<fromPk>:<edgeId> instead of graph:out:<graphId>:<fromPk>:<edgeId>.
		// Approved By: Index module maintainer — INDEX-AUD-GI-03
		// Removal Target: v2.6.0
		graphId.clear();
		fromPk = s.substr(0, first);
		edgeId = s.substr(first + 1);
		return true;
	}
	graphId = s.substr(0, first);
	fromPk = s.substr(first + 1, last - first - 1);
	edgeId = s.substr(last + 1);
	return true;
}

bool GraphIndexManager::parseInKey_(std::string_view key, std::string& graphId, std::string& toPk, std::string& edgeId) {
	// Expect: graph:in:<graph_id>:<toPk>:<edgeId>
	if (key.rfind("graph:in:", 0) != 0) return false;
	std::string s(key.substr(9)); // after prefix
	size_t first = s.find(':');
	if (first == std::string::npos) return false;
	size_t last = s.rfind(':');
	// LEGACY PATH (requires human approval — INDEX-AUD-GI-03): Support two formats
	// Reason: RocksDB in-keys written before v2.0 lack graphId segment; must support both.
	// Activation: in-key parsing when last==first (no graphId separator).
	// Primary Delta: v2.0+ in-keys include graphId; pre-v2.0 do not.
	// Approved By: Index module maintainer — INDEX-AUD-GI-03
	// Removal Target: v2.6.0 (after full migration to v2.0+ key schema)
	if (last == first) {
		graphId.clear();
		toPk = s.substr(0, first);
		edgeId = s.substr(first + 1);
		return true;
	}
	graphId = s.substr(0, first);
	toPk = s.substr(first + 1, last - first - 1);
	edgeId = s.substr(last + 1);
	return true;
}

std::pair<GraphIndexManager::Status, GraphIndexManager::PathResult>
GraphIndexManager::dijkstra(std::string_view startPk, std::string_view targetPk) const {
	if (!db_.isOpen()) return {Status::Error("dijkstra: Datenbank ist nicht geöffnet"), {}};
	if (startPk.empty() || targetPk.empty()) {
		return {Status::Error("dijkstra: Start und Ziel dürfen nicht leer sein"), {}};
	}

	std::string start(startPk);
	std::string target(targetPk);

	// Priority Queue: (cost, node)
	using QueueItem = std::pair<double, std::string>;
	auto cmp = [](const QueueItem& a, const QueueItem& b) { return a.first > b.first; };
	std::priority_queue<QueueItem, std::vector<QueueItem>, decltype(cmp)> pq(cmp);

	std::unordered_map<std::string, double> dist;
	std::unordered_map<std::string, std::string> prev;
	std::unordered_set<std::string> visited;

	dist[start] = 0.0;
	pq.emplace(0.0, start);

	while (!pq.empty()) {
		auto [cost, node] = pq.top();
		pq.pop();

		if (visited.count(node)) continue;
		visited.insert(node);

		// Ziel erreicht?
		if (node == target) break;

		// Nachbarn holen (In-Memory falls verfügbar)
		std::vector<std::string> neighbors;
		if (topologyLoaded_.load(std::memory_order_acquire)) {
   // LOCK: Tier 1 (Global topology protection, read-only) — Phase 3 A-5
			std::shared_lock<std::shared_mutex> lock(topology_mutex_);
			auto it = outEdges_.find(node);
			if (it != outEdges_.end()) {
				for (const auto& adj : it->second) {
					// Non-typed Dijkstra: traverse all graphs
					neighbors.push_back(adj.targetPk);
					// Edge-Weight ermitteln
					double weight = getEdgeWeight_(adj.graphId, adj.edgeId);
					double newCost = dist[node] + weight;
					
					if (!dist.count(adj.targetPk) || newCost < dist[adj.targetPk]) {
						dist[adj.targetPk] = newCost;
						prev[adj.targetPk] = node;
						pq.emplace(newCost, adj.targetPk);
					}
				}
			}
		} else {
			// Fallback: RocksDB scan (all graphs)
			const std::string prefix = std::string("graph:out:");
			db_.scanPrefix(prefix, [&](std::string_view key, std::string_view val) {
				// Robust parse (support optional graphId segment)
				std::string keyStr(key);
				size_t lastColon = keyStr.rfind(':');
				if (lastColon == std::string::npos) return true;
				const size_t prefixLen = std::string("graph:out:").size();
				if (keyStr.size() <= prefixLen) return true;
				std::string middle = keyStr.substr(prefixLen, lastColon - prefixLen);
				size_t innerColon = middle.rfind(':');
				std::string fromPk = (innerColon == std::string::npos) ? middle : middle.substr(innerColon + 1);
				if (fromPk != node) return true;

				std::string edgeId = keyStr.substr(lastColon + 1);
				std::string graphId;
				if (innerColon == std::string::npos) graphId.clear();
				else graphId = middle.substr(0, innerColon);

				std::string neighbor(val);
				double weight = getEdgeWeight_(graphId, edgeId);
				double newCost = dist[node] + weight;

				if (!dist.count(neighbor) || newCost < dist[neighbor]) {
					dist[neighbor] = newCost;
					prev[neighbor] = node;
					pq.emplace(newCost, neighbor);
				}
				return true;
			});
		}
	}

	// Pfad rekonstruieren
	if (!prev.count(target) && target != start) {
		return {Status::Error("dijkstra: Kein Pfad gefunden"), {}};
	}

	PathResult result;
	result.totalCost = dist[target];
	
	std::vector<std::string> path;
	std::string current = target;
	while (current != start) {
		path.push_back(current);
		auto it = prev.find(current);
		if (it == prev.end()) break;
		current = it->second;
	}
	path.push_back(start);
	std::reverse(path.begin(), path.end());
	result.path = std::move(path);

	return {Status::OK(), std::move(result)};
}

// Dijkstra with edge type filtering and graph scope (server-side)
std::pair<GraphIndexManager::Status, GraphIndexManager::PathResult>
GraphIndexManager::dijkstra(std::string_view startPk, std::string_view targetPk, std::string_view edge_type, std::string_view graph_id) const {
	if (!db_.isOpen()) return {Status::Error("dijkstra: Datenbank ist nicht geöffnet"), {}};
	if (startPk.empty() || targetPk.empty()) {
		return {Status::Error("dijkstra: Start und Ziel dürfen nicht leer sein"), {}};
	}

	std::string start(startPk);
	std::string target(targetPk);
	std::string typeFilter(edge_type);
	std::string graphFilter(graph_id);

	// Priority Queue: (cost, node)
	using QueueItem = std::pair<double, std::string>;
	auto cmp = [](const QueueItem& a, const QueueItem& b) { return a.first > b.first; };
	std::priority_queue<QueueItem, std::vector<QueueItem>, decltype(cmp)> pq(cmp);

	std::unordered_map<std::string, double> dist;
	std::unordered_map<std::string, std::string> prev;
	std::unordered_set<std::string> visited;

	dist[start] = 0.0;
	pq.emplace(0.0, start);

	while (!pq.empty()) {
		auto [cost, node] = pq.top();
		pq.pop();

		if (visited.count(node)) continue;
		visited.insert(node);

		// Ziel erreicht?
		if (node == target) break;

		// Nachbarn holen (In-Memory falls verfügbar)
		if (topologyLoaded_.load(std::memory_order_acquire)) {
   // LOCK: Tier 1 (Global topology protection, read-only) — Phase 3 A-5
			std::shared_lock<std::shared_mutex> lock(topology_mutex_);
			auto it = outEdges_.find(node);
			if (it != outEdges_.end()) {
				for (const auto& adj : it->second) {
					// Filter by graph and edge type
					if (!graphFilter.empty() && adj.graphId != graphFilter) continue;
					std::string edgeType = getEdgeType_(adj.graphId, adj.edgeId);
					if (!typeFilter.empty() && edgeType != typeFilter) {
						continue; // Skip edges with different type
					}

					// Edge-Weight ermitteln
					double weight = getEdgeWeight_(adj.graphId, adj.edgeId);
					double newCost = dist[node] + weight;
					
					if (!dist.count(adj.targetPk) || newCost < dist[adj.targetPk]) {
						dist[adj.targetPk] = newCost;
						prev[adj.targetPk] = node;
						pq.emplace(newCost, adj.targetPk);
					}
				}
			}
		} else {
			// Fallback: RocksDB scan
			if (graphFilter.empty()) {
				return {Status::Error("dijkstra: graph_id required for scan without topology"), {}};
			}
			const std::string prefix = std::string("graph:out:") + graphFilter + ":" + std::string(node) + ":";
			db_.scanPrefix(prefix, [&](std::string_view key, std::string_view val) {
				std::string gid, from, edgeId;
				if (!parseOutKey_(key, gid, from, edgeId)) return true;

				// Filter by edge type
				std::string edgeType = getEdgeType_(gid, edgeId);
				if (!typeFilter.empty() && edgeType != typeFilter) {
					return true; // Skip edges with different type
				}

				std::string neighbor(val);
				double weight = getEdgeWeight_(gid, edgeId);
				double newCost = dist[node] + weight;
				
				if (!dist.count(neighbor) || newCost < dist[neighbor]) {
					dist[neighbor] = newCost;
					prev[neighbor] = node;
					pq.emplace(newCost, neighbor);
				}
				return true;
			});
		}
	}

	// Pfad rekonstruieren
	if (!prev.count(target) && target != start) {
		return {Status::Error("dijkstra: Kein Pfad gefunden"), {}};
	}

	PathResult result;
	result.totalCost = dist[target];
	
	std::vector<std::string> path;
	std::string current = target;
	while (current != start) {
		path.push_back(current);
		auto it = prev.find(current);
		if (it == prev.end()) break;
		current = it->second;
	}
	path.push_back(start);
	std::reverse(path.begin(), path.end());
	result.path = std::move(path);

	return {Status::OK(), std::move(result)};
}

std::pair<GraphIndexManager::Status, GraphIndexManager::PathResult>
GraphIndexManager::aStar(std::string_view startPk, std::string_view targetPk, HeuristicFunc heuristic) const {
	if (!db_.isOpen()) return {Status::Error("aStar: Datenbank ist nicht geöffnet"), {}};
	if (startPk.empty() || targetPk.empty()) {
		return {Status::Error("aStar: Start und Ziel dürfen nicht leer sein"), {}};
	}

	std::string start(startPk);
	std::string target(targetPk);

	// Wenn keine Heuristik angegeben, verwende konstante 0 (= Dijkstra)
	auto h = heuristic ? heuristic : [](const std::string&) { return 0.0; };

	// Priority Queue: (f_score, node) wobei f = g + h
	using QueueItem = std::pair<double, std::string>;
	auto cmp = [](const QueueItem& a, const QueueItem& b) { return a.first > b.first; };
	std::priority_queue<QueueItem, std::vector<QueueItem>, decltype(cmp)> pq(cmp);

	std::unordered_map<std::string, double> g_score; // Tatsächliche Kosten vom Start
	std::unordered_map<std::string, double> f_score; // g + h (geschätzte Gesamtkosten)
	std::unordered_map<std::string, std::string> prev;
	std::unordered_set<std::string> visited;

	g_score[start] = 0.0;
	f_score[start] = h(start);
	pq.emplace(f_score[start], start);

	while (!pq.empty()) {
		auto [_, node] = pq.top();
		pq.pop();

		if (visited.count(node)) continue;
		visited.insert(node);

		// Ziel erreicht?
		if (node == target) break;

		// Nachbarn holen
		if (topologyLoaded_.load(std::memory_order_acquire)) {
   // LOCK: Tier 1 (Global topology protection, read-only) — Phase 3 A-5
			std::shared_lock<std::shared_mutex> lock(topology_mutex_);
			auto it = outEdges_.find(node);
			if (it != outEdges_.end()) {
				for (const auto& adj : it->second) {
					// Non-typed A*: traverse all graphs
					if (visited.count(adj.targetPk)) continue;

					double weight = getEdgeWeight_(adj.graphId, adj.edgeId);
					double tentative_g = g_score[node] + weight;

					if (!g_score.count(adj.targetPk) || tentative_g < g_score[adj.targetPk]) {
						prev[adj.targetPk] = node;
						g_score[adj.targetPk] = tentative_g;
						f_score[adj.targetPk] = tentative_g + h(adj.targetPk);
						pq.emplace(f_score[adj.targetPk], adj.targetPk);
					}
				}
			}
		} else {
			// Fallback: RocksDB scan (all graphs)
			const std::string prefix = std::string("graph:out:");
			db_.scanPrefix(prefix, [&](std::string_view key, std::string_view val) {
				// Parse key to check if it belongs to this node
				std::string graphId, fromPk, edgeId;
				if (!parseOutKey_(key, graphId, fromPk, edgeId)) return true;
				if (fromPk != node) return true;
				
				std::string neighbor(val);
				if (visited.count(neighbor)) return true;

				double weight = getEdgeWeight_(graphId, edgeId);
				double tentative_g = g_score[node] + weight;

				if (!g_score.count(neighbor) || tentative_g < g_score[neighbor]) {
					prev[neighbor] = node;
					g_score[neighbor] = tentative_g;
					f_score[neighbor] = tentative_g + h(neighbor);
					pq.emplace(f_score[neighbor], neighbor);
				}
				return true;
			});
		}
	}

	// Pfad rekonstruieren
	if (!prev.count(target) && target != start) {
		return {Status::Error("aStar: Kein Pfad gefunden"), {}};
	}

	PathResult result;
	result.totalCost = g_score[target];
	
	std::vector<std::string> path;
	std::string current = target;
	while (current != start) {
		path.push_back(current);
		auto it = prev.find(current);
		if (it == prev.end()) break;
		current = it->second;
	}
	path.push_back(start);
	std::reverse(path.begin(), path.end());
	result.path = std::move(path);

	return {Status::OK(), std::move(result)};
}

// ============================================================================
// MVCC Transaction Variants
// ============================================================================

GraphIndexManager::Status GraphIndexManager::addEdge(const BaseEntity& edge, RocksDBWrapper::TransactionWrapper& txn) {
	if (!txn.isActive()) return Status::Error("addEdge(mvcc): Transaction ist nicht aktiv");
	
	auto eidOpt = edge.getFieldAsString("id");
	auto fromOpt = edge.getFieldAsString("_from");
	auto toOpt = edge.getFieldAsString("_to");
	if (!eidOpt || !fromOpt || !toOpt) return Status::Error("addEdge(mvcc): Felder 'id', '_from', '_to' fehlen");
	
	// QW-45 Guard: Fail-closed validation for empty _from/_to fields
	if (fromOpt->empty()) {
		THEMIS_ERROR("QW-45 Guard: Edge rejected in Transaction - _from field is empty (fail-closed)");
		return Status::Error("addEdge(txn): QW-45 Guard - _from node ID cannot be empty");
	}
	if (toOpt->empty()) {
		THEMIS_ERROR("QW-45 Guard: Edge rejected in Transaction - _to field is empty (fail-closed)");
		return Status::Error("addEdge(txn): QW-45 Guard - _to node ID cannot be empty");
	}
	if (eidOpt->empty()) {
		THEMIS_ERROR("QW-45 Guard: Edge rejected in Transaction - id field is empty (fail-closed)");
		return Status::Error("addEdge(txn): QW-45 Guard - edge ID cannot be empty");
	}
	
	const std::string& eid = *eidOpt; 
	const std::string& from = *fromOpt; 
	const std::string& to = *toOpt;
	
	// If FieldEncryption is configured, optionally encrypt fields listed in
	// `encrypt_fields` (preferred) or fall back to legacy `_sensitive` boolean.
	BaseEntity stored = edge; // mutable copy
	try {
		if (field_encryption_) {
			std::vector<std::string> encryptList;

			// Preferred: encrypt_fields as JSON-string or comma-separated list
			auto encOpt = edge.getFieldAsString("encrypt_fields");
			if (encOpt) {
				try {
					auto j = nlohmann::json::parse(*encOpt);
					if (j.is_array()) {
						for (const auto& v : j) if (v.is_string()) encryptList.push_back(v.get<std::string>());
					}
				} catch (const std::exception& e) {
					THEMIS_DEBUG("addEdge(mvcc): failed to parse encrypt_fields JSON, using CSV fallback: {}", e.what());
				} catch (...) {
					THEMIS_DEBUG("addEdge(mvcc): failed to parse encrypt_fields JSON with unknown error, using CSV fallback");
					// Fallback: comma-separated
					std::string s = *encOpt;
					size_t start = 0;
					while (start < s.size()) {
						auto pos = s.find(',', start);
						std::string part = (pos == std::string::npos) ? s.substr(start) : s.substr(start, pos - start);
						// trim
						auto l = part.find_first_not_of(" \t\n\r");
						auto r = part.find_last_not_of(" \t\n\r");
						if (l != std::string::npos && r != std::string::npos) encryptList.push_back(part.substr(l, r - l + 1));
						if (pos == std::string::npos) break;
						start = pos + 1;
					}
				}
			}

			// LEGACY PATH (requires human approval — INDEX-AUD-GI-02)
			// Reason: _sensitive boolean fallback for updateEdge path — duplicate of GI-01 but
			//   required because updateEdge has a separate encrypt-field resolution loop.
			//   Same migration dependency applies as GI-01.
			// Activation: updateEdge path when encryptList is empty and _sensitive==true is present.
			// Primary Delta: v2.1+ documents use encrypt_fields list; this fallback is not used for new writes.
			// Approved By: Index module maintainer — pre-existing legacy compat path (INDEX-AUD-GI-02)
			// Removal Target: v2.6.0 (same data migration gate as GI-01)
			// Backwards compat: if no explicit list and _sensitive==true, encrypt weight+metadata
			if (encryptList.empty()) {
				auto sensitiveOpt = edge.getFieldAsBool("_sensitive");
				if (sensitiveOpt && *sensitiveOpt) {
					encryptList.push_back("_weight");
					encryptList.push_back("metadata");
				}
			}

			// Perform encryption for listed fields
			for (const auto& fld : encryptList) {
				if (!edge.hasField(fld)) continue;
				if (fld == "_weight") {
					// Prefer numeric weight
					if (auto wOpt = edge.getFieldAsDouble("_weight"); wOpt) {
						std::string wstr = std::to_string(*wOpt);
						auto blob = field_encryption_->encrypt(wstr, "edges.weight");
						stored.setField("_weight", std::string(blob.toBase64()));
						continue;
					}
					// Fallback: string/int
					if (auto wStr = edge.getFieldAsString("_weight"); wStr) {
						auto blob = field_encryption_->encrypt(*wStr, "edges.weight");
						stored.setField("_weight", std::string(blob.toBase64()));
						continue;
					}
				} else if (fld == "metadata") {
					if (auto metaOpt = edge.getFieldAsString("metadata"); metaOpt) {
						auto blob = field_encryption_->encrypt(*metaOpt, "edges.metadata");
						stored.setField("metadata", std::string(blob.toBase64()));
						continue;
					}
				} else {
					// Generic: try string field
					if (auto sval = edge.getFieldAsString(fld); sval) {
						auto blob = field_encryption_->encrypt(*sval, std::string("edges.") + fld);
						stored.setField(fld, std::string(blob.toBase64()));
						continue;
					}
					// Try binary blobs
					if (auto bin = edge.getField(fld); bin && std::holds_alternative<std::vector<uint8_t>>(*bin)) {
						auto& vec = std::get<std::vector<uint8_t>>(*bin);
						auto blob = field_encryption_->encrypt(vec, std::string("edges.") + fld);
						stored.setField(fld, std::string(blob.toBase64()));
						continue;
					}
				}
			}
		}
	} catch (const std::exception& e) {
		THEMIS_ERROR("addEdge(mvcc): encryption failed: {}", e.what());
		return Status::Error(std::string("addEdge(mvcc): encryption failed: ") + e.what());
	}

	// Extract graphId from edge (optional field)
	std::string graphId = edge.getFieldAsString("_graph").value_or("");

	{
  // LOCK: Tier 1 (Global topology protection) — Phase 3 A-5
		std::lock_guard<std::shared_mutex> lock(topology_mutex_);
		// Edge speichern (Primärspeicher)
		txn.put(KeySchema::makeGraphEdgeKey(eid), stored.serialize());

		// Adjazenz-Indizes
		txn.put(KeySchema::makeGraphOutdexKey(from, eid), toBytes(to));
		txn.put(KeySchema::makeGraphIndexKey(to, eid), toBytes(from));

		// Update in-memory topology if loaded
		if (topologyLoaded_.load(std::memory_order_acquire)) {
			addEdgeToTopologyUnlocked_(eid, from, to, graphId);
		}
	}

	return Status::OK();
}

GraphIndexManager::Status GraphIndexManager::deleteEdge(std::string_view edgeId, RocksDBWrapper::TransactionWrapper& txn) {
	if (!txn.isActive()) return Status::Error("deleteEdge(mvcc): Transaction ist nicht aktiv");
	if (edgeId.empty()) return Status::Error("deleteEdge(mvcc): edgeId leer");
	
	const std::string edgeKey = KeySchema::makeGraphEdgeKey(edgeId);
	
	// Read edge from MVCC snapshot
	auto blob = txn.get(edgeKey);
	if (!blob) return Status::OK();
	
	BaseEntity e = BaseEntity::deserialize(std::string(edgeId), *blob);
	auto fromOpt = e.getFieldAsString("_from");
	auto toOpt = e.getFieldAsString("_to");
	std::string graphId = e.getFieldAsString("_graph").value_or("");
	
	{
  // LOCK: Tier 1 (Global topology protection) — Phase 3 A-5
		std::lock_guard<std::shared_mutex> lock(topology_mutex_);
		txn.del(edgeKey);
		if (fromOpt && toOpt) {
			txn.del(KeySchema::makeGraphOutdexKey(*fromOpt, std::string(edgeId)));
			txn.del(KeySchema::makeGraphIndexKey(*toOpt, std::string(edgeId)));

			// Update in-memory topology if loaded
			if (topologyLoaded_.load(std::memory_order_acquire)) {
				removeEdgeFromTopologyUnlocked_(std::string(edgeId), *fromOpt, *toOpt, graphId);
			}

			return Status::OK();
		}
	}
	
	return Status::Error("deleteEdge(mvcc): _from/_to fehlen (inkonsistent)");
}

// ===== Sprint B: Temporal Graph Traversal =====

std::pair<GraphIndexManager::Status, std::vector<std::string>>
GraphIndexManager::bfsAtTime(std::string_view startPk, int64_t timestamp_ms, int maxDepth) const {
	if (!db_.isOpen()) return {Status::Error("bfsAtTime: Datenbank ist nicht geöffnet"), {}};
	if (startPk.empty()) return {Status::Error("bfsAtTime: startPk darf nicht leer sein"), {}};
	if (maxDepth < 0) return {Status::Error("bfsAtTime: maxDepth muss >= 0 sein"), {}};

	TemporalFilter filter = TemporalFilter::at(timestamp_ms);
	
	std::vector<std::string> order;
	std::unordered_set<std::string> visited;
	std::queue<std::pair<std::string,int>> q;

	q.emplace(std::string(startPk), 0);
	visited.insert(std::string(startPk));

	while (!q.empty()) {
		auto [node, depth] = q.front();
		q.pop();
		order.push_back(node);

		if (depth >= maxDepth) continue;

		// Get outgoing edges with adjacency info
		auto [st, adj] = outAdjacency(node);
		if (!st.ok) continue;

		for (const auto& info : adj) {
			// Load edge to check temporal validity
			std::string edgeKey = KeySchema::makeGraphEdgeKey(info.edgeId);
			auto blob = db_.get(edgeKey);
			if (!blob) continue;

			BaseEntity edge = BaseEntity::deserialize(info.edgeId, *blob);
			
			// Check temporal validity
			std::optional<int64_t> valid_from = edge.getFieldAsInt("valid_from");
			std::optional<int64_t> valid_to = edge.getFieldAsInt("valid_to");
			
			if (!filter.isValid(valid_from, valid_to)) {
				continue; // Skip edge - not valid at query time
			}

			// Include neighbor if valid
			if (visited.find(info.targetPk) == visited.end()) {
				visited.insert(info.targetPk);
				q.emplace(info.targetPk, depth + 1);
			}
		}
	}
	
	// Phase 1: Audit log for temporal query
	logAuditEvent_("TEMPORAL_QUERY", std::string(startPk), "bfsAtTime", order.size(), maxDepth);

	return {Status::OK(), std::move(order)};
}

std::pair<GraphIndexManager::Status, GraphIndexManager::PathResult>
GraphIndexManager::dijkstraAtTime(std::string_view startPk, std::string_view targetPk, int64_t timestamp_ms) const {
	if (!db_.isOpen()) return {Status::Error("dijkstraAtTime: Datenbank ist nicht geöffnet"), {}};
	if (startPk.empty() || targetPk.empty()) {
		return {Status::Error("dijkstraAtTime: start/target dürfen nicht leer sein"), {}};
	}

	TemporalFilter filter = TemporalFilter::at(timestamp_ms);
	
	std::unordered_map<std::string, double> dist;
	std::unordered_map<std::string, std::string> prev;
	
	using PQElem = std::pair<double, std::string>;
	std::priority_queue<PQElem, std::vector<PQElem>, std::greater<>> pq;

	std::string start(startPk);
	std::string target(targetPk);

	dist[start] = 0.0;
	pq.emplace(0.0, start);

	while (!pq.empty()) {
		auto [d, u] = pq.top();
		pq.pop();

		if (u == target) break;
		if (dist.count(u) && d > dist[u]) continue;

		auto [st, adj] = outAdjacency(u);
		if (!st.ok) continue;

		for (const auto& info : adj) {
			// Load edge to check temporal validity and weight
			std::string edgeKey = KeySchema::makeGraphEdgeKey(info.edgeId);
			auto blob = db_.get(edgeKey);
			if (!blob) continue;

			BaseEntity edge = BaseEntity::deserialize(info.edgeId, *blob);
			
			// Check temporal validity
			std::optional<int64_t> valid_from = edge.getFieldAsInt("valid_from");
			std::optional<int64_t> valid_to = edge.getFieldAsInt("valid_to");
			
			if (!filter.isValid(valid_from, valid_to)) {
				continue; // Skip edge - not valid at query time
			}

			// Get edge weight
			double weight = 1.0;
			if (auto w = edge.getFieldAsDouble("_weight")) {
				weight = *w;
			}

			const std::string& v = info.targetPk;
			double alt = d + weight;

			if (!dist.count(v) || alt < dist[v]) {
				dist[v] = alt;
				prev[v] = u;
				pq.emplace(alt, v);
			}
		}
	}

	// Reconstruct path
	PathResult result;
	if (!dist.count(target)) {
		return {Status::Error("dijkstraAtTime: Kein Pfad gefunden"), result};
	}

	result.totalCost = dist[target];
	std::string curr = target;
	while (curr != start) {
		result.path.push_back(curr);
		if (!prev.count(curr)) break;
		curr = prev[curr];
	}
	result.path.push_back(start);
	std::reverse(result.path.begin(), result.path.end());

	return {Status::OK(), result};
}

// ===== Sprint B Extended: Time-Range Queries =====

std::pair<GraphIndexManager::Status, std::vector<GraphIndexManager::EdgeInfo>>
GraphIndexManager::getEdgesInTimeRange(int64_t range_start_ms, int64_t range_end_ms, bool require_full_containment) const {
	if (!db_.isOpen()) {
		return {Status::Error("getEdgesInTimeRange: Datenbank ist nicht geöffnet"), {}};
	}

	TimeRangeFilter filter = TimeRangeFilter::between(range_start_ms, range_end_ms);
	std::vector<EdgeInfo> result;

	// Scan all edges with prefix "graph:out:"
	std::string prefix = "graph:out:";
	db_.scanPrefix(prefix, [this, &filter, require_full_containment, &result](std::string_view key, std::string_view val) {
		// Parse key: graph:out:<from_pk>:<edge_id>
		std::string keyStr(key);
		size_t firstColon = keyStr.find(':');
		if (firstColon == std::string::npos) return true;
		size_t secondColon = keyStr.find(':', firstColon + 1);
		if (secondColon == std::string::npos) return true;
		size_t thirdColon = keyStr.find(':', secondColon + 1);
		if (thirdColon == std::string::npos) return true;

		std::string fromPk = keyStr.substr(secondColon + 1, thirdColon - secondColon - 1);
		std::string edgeId = keyStr.substr(thirdColon + 1);
		std::string toPk(val);

		// Load edge entity to check temporal fields (edges use "edge:" prefix, not "entity:")
		std::string edgeKey = "edge:" + edgeId;
		auto blob = db_.get(edgeKey);
		if (!blob.has_value()) return true;

		BaseEntity edge = BaseEntity::deserialize(edgeId, *blob);
		std::optional<int64_t> valid_from = parseTemporalFieldForEdge(edge, "valid_from", edgeId, "getEdgesInTimeRange");
		std::optional<int64_t> valid_to = parseTemporalFieldForEdge(edge, "valid_to", edgeId, "getEdgesInTimeRange");

		// Check if edge is in time range
		bool match = require_full_containment 
			? filter.fullyContains(valid_from, valid_to)
			: filter.hasOverlap(valid_from, valid_to);

		if (match) {
			result.push_back({edgeId, fromPk, toPk, valid_from, valid_to});
		}
		return true;
	});

	return {Status::OK(), result};
}

std::pair<GraphIndexManager::Status, std::vector<GraphIndexManager::EdgeInfo>>
GraphIndexManager::getOutEdgesInTimeRange(std::string_view fromPk, int64_t range_start_ms, int64_t range_end_ms, bool require_full_containment) const {
	if (!db_.isOpen()) {
		return {Status::Error("getOutEdgesInTimeRange: Datenbank ist nicht geöffnet"), {}};
	}
	if (fromPk.empty()) {
		return {Status::Error("getOutEdgesInTimeRange: fromPk darf nicht leer sein"), {}};
	}

	TimeRangeFilter filter = TimeRangeFilter::between(range_start_ms, range_end_ms);
	std::vector<EdgeInfo> result;

	// Scan edges with prefix "graph:out:<from_pk>:"
	std::string prefix = "graph:out:" + std::string(fromPk) + ":";
	db_.scanPrefix(prefix, [this, &filter, &fromPk, require_full_containment, &result](std::string_view key, std::string_view val) {
		// Parse key: graph:out:<from_pk>:<edge_id>
		std::string keyStr(key);
		size_t lastColon = keyStr.rfind(':');
		if (lastColon == std::string::npos) return true;
		
		std::string edgeId = keyStr.substr(lastColon + 1);
		std::string toPk(val);

		// Load edge entity to check temporal fields (edges use "edge:" prefix, not "entity:")
		std::string edgeKey = "edge:" + edgeId;
		auto blob = db_.get(edgeKey);
		if (!blob.has_value()) return true;

		BaseEntity edge = BaseEntity::deserialize(edgeId, *blob);
		std::optional<int64_t> valid_from = parseTemporalFieldForEdge(edge, "valid_from", edgeId, "getOutEdgesInTimeRange");
		std::optional<int64_t> valid_to = parseTemporalFieldForEdge(edge, "valid_to", edgeId, "getOutEdgesInTimeRange");

		// Check if edge is in time range
		bool match = require_full_containment 
			? filter.fullyContains(valid_from, valid_to)
			: filter.hasOverlap(valid_from, valid_to);

		if (match) {
			result.push_back({edgeId, std::string(fromPk), toPk, valid_from, valid_to});
		}
		return true;
	});

	return {Status::OK(), result};
}

// Aggregate a numeric edge property across edges in a time range
std::pair<GraphIndexManager::Status, GraphIndexManager::TemporalAggregationResult>
GraphIndexManager::aggregateEdgePropertyInTimeRange(std::string_view property, Aggregation agg, int64_t range_start_ms, int64_t range_end_ms, bool require_full_containment, std::optional<std::string_view> edge_type) const {
	if (!db_.isOpen()) {
		return {Status::Error("aggregateEdgePropertyInTimeRange: Datenbank ist nicht geöffnet"), {}};
	}

	TimeRangeFilter filter = TimeRangeFilter::between(range_start_ms, range_end_ms);
	TemporalAggregationResult res;

	double sum = 0.0;
	size_t value_count = 0;
	double minv = 0.0, maxv = 0.0;
	bool have_minmax = false;

	std::string prefix = "graph:out:";
	db_.scanPrefix(prefix, [this, &filter, require_full_containment, &res, &sum, &value_count, &minv, &maxv, &have_minmax, &property, &edge_type, &agg](std::string_view key, std::string_view /*val*/) {
		// Parse key: support both formats:
		// - new: graph:out:<graph_id>:<fromPk>:<edgeId>
		// - legacy: graph:out:<fromPk>:<edgeId>
		std::string graphId, fromPk, edgeId;
		if (!parseOutKey_(key, graphId, fromPk, edgeId)) {
			// LEGACY PATH (requires human approval — INDEX-AUD-GI-03): fallback to pre-v2.0 key format
			// Reason: parseOutKey_ may reject a key that parseInKey_ already fell back to legacy format.
			// Activation: when parseOutKey_ returns false (key has no graphId segment).
			// Primary Delta: v2.0+ code uses parseOutKey_ successfully; old keys fall through here.
			// Approved By: Index module maintainer — INDEX-AUD-GI-03
			// Removal Target: v2.6.0
			std::string keyStr(key);
			size_t firstColon = keyStr.find(':');
			if (firstColon == std::string::npos) return true;
			size_t secondColon = keyStr.find(':', firstColon + 1);
			if (secondColon == std::string::npos) return true;
			size_t thirdColon = keyStr.find(':', secondColon + 1);
			if (thirdColon == std::string::npos) return true;

			fromPk = keyStr.substr(secondColon + 1, thirdColon - secondColon - 1);
			edgeId = keyStr.substr(thirdColon + 1);
			graphId = "default";
		}

		// Load edge entity. Edges are stored as "edge:<edgeId>" (KeySchema::makeGraphEdgeKey).
		// Older or other parts of the code sometimes expect "edge:<graphId>:<edgeId>",
		// so attempt both forms to be robust.
		std::string edgeKeyWithGid = std::string("edge:") + graphId + ":" + edgeId;
		auto blob = db_.get(edgeKeyWithGid);
		if (!blob.has_value()) {
			// Fallback to edge without graph id
			std::string edgeKey = std::string("edge:") + edgeId;
			blob = db_.get(edgeKey);
			if (!blob.has_value()) return true;
		}

		BaseEntity edge = BaseEntity::deserialize(edgeId, *blob);
		std::optional<int64_t> valid_from = edge.getFieldAsInt("valid_from");
		std::optional<int64_t> valid_to = edge.getFieldAsInt("valid_to");

		bool match = require_full_containment ? filter.fullyContains(valid_from, valid_to) : filter.hasOverlap(valid_from, valid_to);
		if (!match) return true;

		// If edge_type provided, check _type
		if (edge_type.has_value()) {
			std::string t = getEdgeType_(graphId, edgeId);
			if (t != std::string(*edge_type)) return true;
		}

		// COUNT aggregates count of matching edges (regardless of property availability)
		if (agg == Aggregation::COUNT) {
			res.count++;
			return true;
		}

		// For numeric aggregations, read the numeric property
		auto valOpt = edge.getFieldAsDouble(std::string(property));
		if (!valOpt.has_value()) return true; // skip edges without numeric field

		double v = *valOpt;
		res.count++; // count of edges considered for numeric aggregation
		value_count++;

		switch (agg) {
			case Aggregation::SUM:
				sum += v;
				break;
			case Aggregation::AVG:
				sum += v;
				break;
			case Aggregation::MIN:
				if (!have_minmax || v < minv) minv = v, have_minmax = true;
				break;
			case Aggregation::MAX:
				if (!have_minmax || v > maxv) maxv = v, have_minmax = true;
				break;
			default:
				break;
		}

		return true;
	});

	// Finalize result
	if (agg == Aggregation::SUM) {
		res.value = sum;
	} else if (agg == Aggregation::AVG) {
		res.value = (value_count > 0) ? (sum / static_cast<double>(value_count)) : 0.0;
	} else if (agg == Aggregation::MIN) {
		res.value = have_minmax ? minv : 0.0;
	} else if (agg == Aggregation::MAX) {
		res.value = have_minmax ? maxv : 0.0;
	} else if (agg == Aggregation::COUNT) {
		res.value = 0.0;
	}

	return {Status::OK(), res};
}

std::pair<GraphIndexManager::Status, TemporalStats>
GraphIndexManager::getTemporalStats(int64_t range_start_ms, int64_t range_end_ms, bool require_full_containment) const {
	if (!db_.isOpen()) {
		return {Status::Error("getTemporalStats: Datenbank ist nicht geöffnet"), {}};
	}

	TimeRangeFilter filter = TimeRangeFilter::between(range_start_ms, range_end_ms);
	TemporalStats stats;

	// Scan all edges with prefix "graph:out:"
	std::string prefix = "graph:out:";
	db_.scanPrefix(prefix, [this, &filter, require_full_containment, &stats](std::string_view key, std::string_view /*val*/) {
		// Parse key: graph:out:<from_pk>:<edge_id>
		std::string keyStr(key);
		size_t thirdColon = keyStr.rfind(':');
		if (thirdColon == std::string::npos) return true;

		std::string edgeId = keyStr.substr(thirdColon + 1);

		// Load edge entity to check temporal fields
		std::string edgeKey = "edge:" + edgeId;
		auto blob = db_.get(edgeKey);
		if (!blob.has_value()) return true;

		BaseEntity edge = BaseEntity::deserialize(edgeId, *blob);
		std::optional<int64_t> valid_from = edge.getFieldAsInt("valid_from");
		std::optional<int64_t> valid_to = edge.getFieldAsInt("valid_to");

		// Check if edge is in time range
		bool has_overlap = filter.hasOverlap(valid_from, valid_to);
		bool fully_contained = filter.fullyContains(valid_from, valid_to);

		if (require_full_containment ? fully_contained : has_overlap) {
			stats.edge_count++;
			
			if (fully_contained) {
				stats.fully_contained_count++;
			}

			// Update temporal range
			if (valid_from.has_value()) {
				if (!stats.earliest_start.has_value() || *valid_from < *stats.earliest_start) {
					stats.earliest_start = valid_from;
				}
			}
			if (valid_to.has_value()) {
				if (!stats.latest_end.has_value() || *valid_to > *stats.latest_end) {
					stats.latest_end = valid_to;
				}
			}

			// Calculate duration statistics for bounded edges
			if (valid_from.has_value() && valid_to.has_value()) {
				int64_t duration = *valid_to - *valid_from;
				stats.bounded_edge_count++;
				stats.total_duration_ms += static_cast<double>(duration);

				if (!stats.min_duration_ms.has_value() || duration < *stats.min_duration_ms) {
					stats.min_duration_ms = duration;
				}
				if (!stats.max_duration_ms.has_value() || duration > *stats.max_duration_ms) {
					stats.max_duration_ms = duration;
				}
			}
		}
		return true;
	});

	// Calculate average duration
	if (stats.bounded_edge_count > 0) {
		stats.avg_duration_ms = stats.total_duration_ms / static_cast<double>(stats.bounded_edge_count);
	}

	return {Status::OK(), stats};
}

// BFS with Path Constraints
std::pair<GraphIndexManager::Status, std::vector<std::string>>
GraphIndexManager::bfsWithConstraints(
	std::string_view startPk,
	int maxDepth,
	const PathConstraints& constraints,
	std::string_view edge_type,
	std::string_view graph_id
) const {
	if (!db_.isOpen()) return {Status::Error("bfsWithConstraints: Database not open"), {}};
	if (startPk.empty()) return {Status::Error("bfsWithConstraints: startPk cannot be empty"), {}};
	if (maxDepth < 0) return {Status::Error("bfsWithConstraints: maxDepth must be >= 0"), {}};

	// Check if start vertex is forbidden
	if (constraints.forbidden_vertices.count(std::string(startPk))) {
		return {Status::Error("bfsWithConstraints: Start vertex is forbidden"), {}};
	}

	std::vector<std::string> order;
	std::unordered_set<std::string> visited;
	std::unordered_set<std::string> visited_edges; // For unique_edges constraint
	std::queue<std::pair<std::string,int>> q;

	q.emplace(std::string(startPk), 0);
	if (constraints.unique_vertices) {
		visited.insert(std::string(startPk));
	}

	std::string typeFilter(edge_type);
	std::string graphFilter(graph_id);

	// Use in-memory topology if available
	if (topologyLoaded_.load(std::memory_order_acquire)) {
		while (!q.empty()) {
			auto [node, depth] = q.front();
			q.pop();

			// Check edge count constraints
			if (constraints.max_edge_count >= 0 && depth > constraints.max_edge_count) {
				continue;
			}

			order.push_back(node);
			
			if (depth == maxDepth) continue;

   // LOCK: Tier 1 (Global topology protection, read-only) — Phase 3 A-5
			std::shared_lock<std::shared_mutex> lock(topology_mutex_);
			auto it = outEdges_.find(node);
			if (it != outEdges_.end()) {
				for (const auto& adj : it->second) {
					// Filter by graph and edge type
					if (!graphFilter.empty() && adj.graphId != graphFilter) continue;
					if (!typeFilter.empty()) {
						std::string edgeType = getEdgeType_(adj.graphId, adj.edgeId);
						if (edgeType != typeFilter) continue;
					}

					// Check edge constraints
					if (constraints.forbidden_edges.count(adj.edgeId)) continue;
					if (constraints.unique_edges && visited_edges.count(adj.edgeId)) continue;
					
					// Check vertex constraints
					if (constraints.forbidden_vertices.count(adj.targetPk)) continue;
					if (constraints.unique_vertices && visited.count(adj.targetPk)) continue;

					// Add to visited sets
					if (constraints.unique_vertices) {
						visited.insert(adj.targetPk);
					}
					if (constraints.unique_edges) {
						visited_edges.insert(adj.edgeId);
					}

					q.emplace(adj.targetPk, depth + 1);
				}
			}
		}
	} else {
		// RocksDB scan-based BFS with constraints
		while (!q.empty()) {
			auto [node, depth] = q.front();
			q.pop();

			// Check edge count constraints
			if (constraints.max_edge_count >= 0 && depth > constraints.max_edge_count) {
				continue;
			}

			order.push_back(node);
			
			if (depth == maxDepth) continue;

			if (graphFilter.empty()) {
				return {Status::Error("bfsWithConstraints: graph_id required for scan without topology"), {}};
			}

			const std::string prefix = std::string("graph:out:") + graphFilter + ":" + std::string(node) + ":";
			db_.scanPrefix(prefix, [&](std::string_view key, std::string_view val){
				std::string gid, from, edgeId;
				if (!parseOutKey_(key, gid, from, edgeId)) return true;

				// Filter by edge type
				if (!typeFilter.empty()) {
					std::string edgeType = getEdgeType_(gid, edgeId);
					if (edgeType != typeFilter) return true;
				}

				// Check edge constraints
				if (constraints.forbidden_edges.count(edgeId)) return true;
				if (constraints.unique_edges && visited_edges.count(edgeId)) return true;

				std::string neigh(val);
				
				// Check vertex constraints
				if (constraints.forbidden_vertices.count(neigh)) return true;
				if (constraints.unique_vertices && visited.count(neigh)) return true;

				// Add to visited sets
				if (constraints.unique_vertices) {
					visited.insert(neigh);
				}
				if (constraints.unique_edges) {
					visited_edges.insert(edgeId);
				}

				q.emplace(neigh, depth + 1);
				return true;
			});
		}
	}

	// Check min_edge_count constraint
	if (constraints.min_edge_count > 0) {
		std::vector<std::string> filtered;
		for (const auto& vertex : order) {
			// In BFS, we need to track the depth/edge count for each vertex
			// This is a simplified check - for exact min_edge_count we'd need path tracking
			filtered.push_back(vertex);
		}
		order = std::move(filtered);
	}

	// Check required_vertices constraint
	if (!constraints.required_vertices.empty()) {
		bool all_required_found = true;
		for (const auto& req : constraints.required_vertices) {
			if (std::find(order.begin(), order.end(), req) == order.end()) {
				all_required_found = false;
				break;
			}
		}
		if (!all_required_found) {
			return {Status::Error("bfsWithConstraints: Not all required vertices found in path"), {}};
		}
	}

	return {Status::OK(), std::move(order)};
}

// Dijkstra with Path Constraints
std::pair<GraphIndexManager::Status, GraphIndexManager::PathResult>
GraphIndexManager::dijkstraWithConstraints(
	std::string_view startPk,
	std::string_view targetPk,
	const PathConstraints& constraints,
	std::string_view edge_type,
	std::string_view graph_id
) const {
	if (!db_.isOpen()) return {Status::Error("dijkstraWithConstraints: Database not open"), {}};
	if (startPk.empty() || targetPk.empty()) {
		return {Status::Error("dijkstraWithConstraints: startPk and targetPk cannot be empty"), {}};
	}

	// Check if start or target vertices are forbidden
	if (constraints.forbidden_vertices.count(std::string(startPk)) ||
	    constraints.forbidden_vertices.count(std::string(targetPk))) {
		return {Status::Error("dijkstraWithConstraints: Start or target vertex is forbidden"), {}};
	}

	struct NodeState {
		std::string node;
		double cost;
		int edge_count;
		std::vector<std::string> path;
		std::unordered_set<std::string> visited_edges;
		
		bool operator>(const NodeState& other) const { return cost > other.cost; }
	};

	std::priority_queue<NodeState, std::vector<NodeState>, std::greater<NodeState>> pq;
	std::unordered_map<std::string, double> best_cost;

	NodeState start;
	start.node = std::string(startPk);
	start.cost = 0.0;
	start.edge_count = 0;
	start.path.push_back(std::string(startPk));
	
	pq.push(std::move(start));
	best_cost[std::string(startPk)] = 0.0;

	std::string typeFilter(edge_type);
	std::string graphFilter(graph_id);

	while (!pq.empty()) {
		NodeState current = pq.top();
		pq.pop();

		// Found target
		if (current.node == targetPk) {
			// Check min_edge_count constraint
			if (current.edge_count < constraints.min_edge_count) {
				continue; // Path too short
			}
			
			// Check required_vertices constraint
			if (!constraints.required_vertices.empty()) {
				bool all_required_found = true;
				for (const auto& req : constraints.required_vertices) {
					if (std::find(current.path.begin(), current.path.end(), req) == current.path.end()) {
						all_required_found = false;
						break;
					}
				}
				if (!all_required_found) {
					continue; // Not all required vertices in path
				}
			}
			
			PathResult result;
			result.path = std::move(current.path);
			result.totalCost = current.cost;
			return {Status::OK(), std::move(result)};
		}

		// Skip if we found a better path to this node already
		auto it = best_cost.find(current.node);
		if (it != best_cost.end() && current.cost > it->second) {
			continue;
		}

		// Check edge count constraints
		if (constraints.max_edge_count >= 0 && current.edge_count >= constraints.max_edge_count) {
			continue;
		}

		// Get neighbors
		std::vector<AdjacencyInfo> neighbors;
		if (topologyLoaded_.load(std::memory_order_acquire)) {
   // LOCK: Tier 1 (Global topology protection, read-only) — Phase 3 A-5
			std::shared_lock<std::shared_mutex> lock(topology_mutex_);
			auto adj_it = outEdges_.find(current.node);
			if (adj_it != outEdges_.end()) {
				neighbors = adj_it->second;
			}
		} else {
			// RocksDB scan fallback
			if (graphFilter.empty()) {
				return {Status::Error("dijkstraWithConstraints: graph_id required for scan without topology"), {}};
			}
			const std::string prefix = std::string("graph:out:") + graphFilter + ":" + current.node + ":";
			db_.scanPrefix(prefix, [&](std::string_view key, std::string_view val){
				std::string gid, from, edgeId;
				if (!parseOutKey_(key, gid, from, edgeId)) return true;
				AdjacencyInfo info;
				info.edgeId = edgeId;
				info.targetPk = std::string(val);
				info.graphId = gid;
				neighbors.push_back(std::move(info));
				return true;
			});
		}

		// Process neighbors
		for (const auto& adj : neighbors) {
			// Filter by graph and edge type
			if (!graphFilter.empty() && adj.graphId != graphFilter) continue;
			if (!typeFilter.empty()) {
				std::string edgeType = getEdgeType_(adj.graphId, adj.edgeId);
				if (edgeType != typeFilter) continue;
			}

			// Check edge constraints
			if (constraints.forbidden_edges.count(adj.edgeId)) continue;
			if (constraints.unique_edges && current.visited_edges.count(adj.edgeId)) continue;

			// Check vertex constraints
			if (constraints.forbidden_vertices.count(adj.targetPk)) continue;
			if (constraints.unique_vertices) {
				if (std::find(current.path.begin(), current.path.end(), adj.targetPk) != current.path.end()) {
					continue; // Already visited in this path
				}
			}

			// Get edge weight
			double weight = getEdgeWeight_(adj.graphId, adj.edgeId);
			double new_cost = current.cost + weight;

			// Check if this is a better path
			auto best_it = best_cost.find(adj.targetPk);
			if (best_it == best_cost.end() || new_cost < best_it->second) {
				best_cost[adj.targetPk] = new_cost;

				NodeState next;
				next.node = adj.targetPk;
				next.cost = new_cost;
				next.edge_count = current.edge_count + 1;
				next.path = current.path;
				next.path.push_back(adj.targetPk);
				next.visited_edges = current.visited_edges;
				if (constraints.unique_edges) {
					next.visited_edges.insert(adj.edgeId);
				}
				
				pq.push(std::move(next));
			}
		}
	}

	return {Status::Error("dijkstraWithConstraints: No path found"), {}};
}

} // namespace themis
