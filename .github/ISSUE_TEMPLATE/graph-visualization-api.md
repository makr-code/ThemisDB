---
name: ✨ Feature: Graph Visualization API
about: Add REST API endpoints for graph visualization and export
title: "[GRAPH] Visualization and Export API"
labels: priority:P2, type:enhancement, area:graph, area:api, effort:medium
assignees: ''
---

## ✨ Feature Enhancement - Graph Visualization

**Current Status:** No visualization API, proposal phase  
**Priority:** P2 (Medium)  
**Effort:** 2 weeks  
**Target Version:** v1.5.0  
**Related Files:**
- `include/index/property_graph.h`
- `src/server/graph_api_handler.h` (new)
- `src/index/property_graph.cpp`

---

## 📋 Problem Description

Currently, ThemisDB's PropertyGraphManager provides powerful graph storage and query capabilities, but there's no built-in way to:
- **Visualize** graph topology
- **Export** graph data for external tools
- **Generate** graph layouts for rendering
- **Query** graph statistics for dashboards

Users must manually extract data and use external tools, which is:
- Time-consuming
- Error-prone
- Inefficient (multiple API calls)
- Lacks standardization

---

## 🎯 Requirements

### Must Have (P2)

- [ ] **Graph Export Formats**
  
  **GET /api/v1/graph/{graph_id}/export**
  - Query parameters: `format` (json, graphml, gexf, dot, cypher)
  - Returns complete graph in specified format
  - Supports filtering by labels/types
  
  ```json
  // JSON format example
  {
    "nodes": [
      {"id": "A", "labels": ["Person"], "properties": {"name": "Alice"}},
      {"id": "B", "labels": ["Person"], "properties": {"name": "Bob"}}
    ],
    "edges": [
      {"id": "e1", "from": "A", "to": "B", "type": "FOLLOWS", 
       "properties": {"since": 2020}}
    ]
  }
  ```

- [ ] **Subgraph Extraction**
  
  **POST /api/v1/graph/{graph_id}/subgraph**
  - Extract subgraph around specific nodes
  - Parameters: `center_nodes`, `max_depth`, `direction` (in/out/both)
  - Returns subgraph with neighborhood
  
  ```json
  {
    "center_nodes": ["A"],
    "max_depth": 2,
    "direction": "both"
  }
  ```

- [ ] **Graph Statistics**
  
  **GET /api/v1/graph/{graph_id}/stats**
  - Returns graph metrics:
    - Node count (total and by label)
    - Edge count (total and by type)
    - Average degree
    - Density
    - Connected components
    - Diameter (optional, expensive)
  
  ```json
  {
    "nodes": {
      "total": 1000,
      "by_label": {"Person": 800, "Company": 200}
    },
    "edges": {
      "total": 2500,
      "by_type": {"WORKS_AT": 800, "FOLLOWS": 1700}
    },
    "metrics": {
      "avg_degree": 2.5,
      "density": 0.0025,
      "components": 1
    }
  }
  ```

### Should Have (P3)

- [ ] **Layout Algorithms**
  
  **POST /api/v1/graph/{graph_id}/layout**
  - Generate node positions for visualization
  - Algorithms: force-directed, hierarchical, circular, random
  - Returns coordinates for each node
  
  ```json
  {
    "algorithm": "force-directed",
    "iterations": 100,
    "nodes": [
      {"id": "A", "x": 100, "y": 200},
      {"id": "B", "x": 150, "y": 220}
    ]
  }
  ```

- [ ] **Path Visualization**
  
  **GET /api/v1/graph/{graph_id}/path**
  - Find and visualize paths between nodes
  - Parameters: `from`, `to`, `max_length`, `algorithm` (shortest, all)
  
  ```json
  {
    "from": "A",
    "to": "C",
    "paths": [
      {"length": 2, "nodes": ["A", "B", "C"], "edges": ["e1", "e2"]},
      {"length": 1, "nodes": ["A", "C"], "edges": ["e3"]}
    ]
  }
  ```

- [ ] **Community Detection**
  
  **POST /api/v1/graph/{graph_id}/communities**
  - Detect communities/clusters
  - Algorithms: louvain, label-propagation, modularity
  - Returns node-to-community mapping

### Could Have (P4)

- [ ] **Real-time Updates**
  - WebSocket endpoint for live graph changes
  - Push notifications for modifications
  - Delta updates (only changed nodes/edges)

- [ ] **Interactive Queries**
  - Cypher-like query language
  - Pattern matching
  - Aggregations and filtering

---

## 🔧 Implementation Details

### Export Handler

```cpp
class GraphAPIHandler {
public:
    // Export graph in various formats
    Response exportGraph(
        const Request& req,
        std::string_view graph_id,
        std::string_view format
    ) {
        auto& pgm = getPropertyGraphManager();
        
        // Get all nodes and edges
        auto [st1, nodes] = pgm.getAllNodes(graph_id);
        auto [st2, edges] = pgm.getAllEdges(graph_id);
        
        // Convert to requested format
        switch (format) {
            case "json":
                return exportAsJSON(nodes, edges);
            case "graphml":
                return exportAsGraphML(nodes, edges);
            case "dot":
                return exportAsDOT(nodes, edges);
            default:
                return Response::error("Unsupported format");
        }
    }
    
    // Extract subgraph
    Response extractSubgraph(
        const Request& req,
        std::string_view graph_id
    ) {
        auto body = req.json();
        auto center_nodes = body["center_nodes"];
        auto max_depth = body["max_depth"].get<int>();
        
        auto& pgm = getPropertyGraphManager();
        
        // BFS from center nodes
        std::unordered_set<std::string> visited;
        std::queue<std::pair<std::string, int>> queue;
        
        for (const auto& node : center_nodes) {
            queue.push({node, 0});
        }
        
        while (!queue.empty()) {
            auto [node, depth] = queue.front();
            queue.pop();
            
            if (depth >= max_depth || visited.count(node)) continue;
            visited.insert(node);
            
            // Get neighbors
            auto [st, neighbors] = pgm.getNeighbors(node, graph_id);
            for (const auto& neighbor : neighbors) {
                queue.push({neighbor, depth + 1});
            }
        }
        
        // Extract subgraph
        return buildSubgraphResponse(visited, pgm, graph_id);
    }
};
```

### Statistics Computation

```cpp
struct GraphStatistics {
    size_t node_count;
    size_t edge_count;
    std::map<std::string, size_t> nodes_by_label;
    std::map<std::string, size_t> edges_by_type;
    double avg_degree;
    double density;
    size_t connected_components;
    
    static GraphStatistics compute(
        PropertyGraphManager& pgm,
        std::string_view graph_id
    ) {
        GraphStatistics stats;
        
        // Get counts
        auto [st1, nodes] = pgm.getAllNodes(graph_id);
        auto [st2, edges] = pgm.getAllEdges(graph_id);
        
        stats.node_count = nodes.size();
        stats.edge_count = edges.size();
        
        // Count by label/type
        for (const auto& node : nodes) {
            auto labels = pgm.getNodeLabels(node.pk, graph_id);
            for (const auto& label : labels.second) {
                stats.nodes_by_label[label]++;
            }
        }
        
        for (const auto& edge : edges) {
            auto type = pgm.getEdgeType(edge.edgeId, graph_id);
            if (type.first.ok) {
                stats.edges_by_type[type.second]++;
            }
        }
        
        // Calculate metrics
        stats.avg_degree = stats.node_count > 0 
            ? (2.0 * stats.edge_count) / stats.node_count 
            : 0.0;
        
        stats.density = stats.node_count > 1
            ? (2.0 * stats.edge_count) / (stats.node_count * (stats.node_count - 1))
            : 0.0;
        
        // Connected components (DFS)
        stats.connected_components = countComponents(pgm, graph_id);
        
        return stats;
    }
};
```

---

## 🧪 Test Cases

```cpp
TEST_F(GraphAPITest, ExportJSON) {
    // Create sample graph
    pgm_->addNode(createNode("A"));
    pgm_->addNode(createNode("B"));
    pgm_->addEdge(createEdge("A", "B", "e1"));
    
    // Export as JSON
    auto response = api_->exportGraph("default", "json");
    
    ASSERT_EQ(response.status, 200);
    auto json = nlohmann::json::parse(response.body);
    EXPECT_EQ(json["nodes"].size(), 2);
    EXPECT_EQ(json["edges"].size(), 1);
}

TEST_F(GraphAPITest, SubgraphExtraction) {
    // Create graph: A → B → C → D
    for (char c = 'A'; c <= 'D'; c++) {
        pgm_->addNode(createNode(std::string(1, c)));
    }
    pgm_->addEdge(createEdge("A", "B", "e1"));
    pgm_->addEdge(createEdge("B", "C", "e2"));
    pgm_->addEdge(createEdge("C", "D", "e3"));
    
    // Extract subgraph around B with depth 1
    auto response = api_->extractSubgraph("default", {"B"}, 1, "both");
    
    auto json = nlohmann::json::parse(response.body);
    auto nodes = json["nodes"];
    
    // Should include A, B, C (not D - too far)
    EXPECT_TRUE(containsNode(nodes, "A"));
    EXPECT_TRUE(containsNode(nodes, "B"));
    EXPECT_TRUE(containsNode(nodes, "C"));
    EXPECT_FALSE(containsNode(nodes, "D"));
}

TEST_F(GraphAPITest, GraphStatistics) {
    // Create graph with known properties
    createTestGraph();  // 100 nodes, 200 edges
    
    auto stats = GraphStatistics::compute(*pgm_, "default");
    
    EXPECT_EQ(stats.node_count, 100);
    EXPECT_EQ(stats.edge_count, 200);
    EXPECT_NEAR(stats.avg_degree, 4.0, 0.1);
    EXPECT_GT(stats.connected_components, 0);
}
```

---

## 📊 Performance Considerations

**Optimization Strategies:**
- Cache statistics (invalidate on modifications)
- Paginate large exports (streaming)
- Index for fast subgraph extraction
- Parallel layout computation
- WebSocket for real-time updates

**Scalability Limits:**
- Export: up to 1M nodes (paginated)
- Subgraph: max depth 5
- Statistics: cache for 1M+ nodes
- Layout: client-side for >10K nodes

---

## 📚 Documentation Updates

- [ ] API reference with examples
- [ ] Format specifications (GraphML, GEXF, DOT)
- [ ] Performance tuning guide
- [ ] Integration guide for viz tools (D3.js, Cytoscape, Gephi)

---

## 🔗 Integration Examples

### D3.js Integration
```javascript
// Fetch graph data
const response = await fetch('/api/v1/graph/social/export?format=json');
const data = await response.json();

// Render with D3
const svg = d3.select('#graph');
const simulation = d3.forceSimulation(data.nodes)
    .force('link', d3.forceLink(data.edges).id(d => d.id))
    .force('charge', d3.forceManyBody())
    .force('center', d3.forceCenter(width/2, height/2));
```

### Cytoscape Integration
```javascript
const cy = cytoscape({
  container: document.getElementById('cy'),
  elements: await fetchGraphData('/api/v1/graph/social/export?format=json'),
  layout: { name: 'cose' }
});
```

---

## ✅ Acceptance Criteria

- [ ] Export API supports JSON, GraphML, DOT formats
- [ ] Subgraph extraction works with configurable depth
- [ ] Statistics API returns accurate metrics
- [ ] Layout algorithms generate valid coordinates
- [ ] Performance acceptable for 100K node graphs
- [ ] Documentation with integration examples
- [ ] Unit tests for all endpoints
- [ ] Integration tests with real graphs
