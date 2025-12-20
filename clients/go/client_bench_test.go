package themisdb

import (
	"context"
	"encoding/json"
	"net/http"
	"net/http/httptest"
	"testing"
)

// BenchmarkClient_Get benchmarks GET operations
func BenchmarkClient_Get(b *testing.B) {
	server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		w.WriteHeader(http.StatusOK)
		json.NewEncoder(w).Encode(map[string]interface{}{
			"id":    "user-123",
			"name":  "Alice",
			"email": "alice@example.com",
		})
	}))
	defer server.Close()

	client := NewClient(Config{Endpoints: []string{server.URL}})
	ctx := context.Background()

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		var result map[string]interface{}
		_ = client.Get(ctx, "relational", "users", "user-123", &result)
	}
}

// BenchmarkClient_Put benchmarks PUT operations
func BenchmarkClient_Put(b *testing.B) {
	server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		w.WriteHeader(http.StatusOK)
	}))
	defer server.Close()

	client := NewClient(Config{Endpoints: []string{server.URL}})
	ctx := context.Background()
	data := map[string]interface{}{
		"name":  "Alice",
		"email": "alice@example.com",
		"age":   30,
	}

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		_ = client.Put(ctx, "relational", "users", "user-123", data)
	}
}

// BenchmarkClient_Delete benchmarks DELETE operations
func BenchmarkClient_Delete(b *testing.B) {
	server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		w.WriteHeader(http.StatusNoContent)
	}))
	defer server.Close()

	client := NewClient(Config{Endpoints: []string{server.URL}})
	ctx := context.Background()

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		_ = client.Delete(ctx, "relational", "users", "user-123")
	}
}

// BenchmarkClient_Query benchmarks AQL query operations
func BenchmarkClient_Query(b *testing.B) {
	server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		w.WriteHeader(http.StatusOK)
		response := map[string]interface{}{
			"data": []interface{}{
				map[string]interface{}{"id": "1", "name": "Alice"},
				map[string]interface{}{"id": "2", "name": "Bob"},
				map[string]interface{}{"id": "3", "name": "Charlie"},
			},
		}
		json.NewEncoder(w).Encode(response)
	}))
	defer server.Close()

	client := NewClient(Config{Endpoints: []string{server.URL}})
	ctx := context.Background()
	query := "SELECT * FROM users WHERE age > 25"

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		var result []map[string]interface{}
		_ = client.Query(ctx, query, &result)
	}
}

// BenchmarkClient_GraphTraverse benchmarks graph traversal operations
func BenchmarkClient_GraphTraverse(b *testing.B) {
	server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		w.WriteHeader(http.StatusOK)
		response := GraphTraverseResult{
			Nodes:   []string{"node1", "node2", "node3", "node4", "node5"},
			Visited: []string{"node1", "node2", "node3", "node4", "node5"},
		}
		json.NewEncoder(w).Encode(response)
	}))
	defer server.Close()

	client := NewClient(Config{Endpoints: []string{server.URL}})
	ctx := context.Background()

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		_, _ = client.GraphTraverse(ctx, "node1", 5, "follows")
	}
}

// BenchmarkClient_ShortestPath benchmarks shortest path computation
func BenchmarkClient_ShortestPath(b *testing.B) {
	server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		w.WriteHeader(http.StatusOK)
		response := map[string]interface{}{
			"path": []string{"node1", "node2", "node3"},
		}
		json.NewEncoder(w).Encode(response)
	}))
	defer server.Close()

	client := NewClient(Config{Endpoints: []string{server.URL}})
	ctx := context.Background()

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		_, _ = client.ShortestPath(ctx, "node1", "node3", "")
	}
}

// BenchmarkClient_Neighbors benchmarks neighbor retrieval
func BenchmarkClient_Neighbors(b *testing.B) {
	server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		w.WriteHeader(http.StatusOK)
		response := map[string]interface{}{
			"neighbors": []string{"node2", "node3", "node4", "node5"},
		}
		json.NewEncoder(w).Encode(response)
	}))
	defer server.Close()

	client := NewClient(Config{Endpoints: []string{server.URL}})
	ctx := context.Background()

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		_, _ = client.Neighbors(ctx, "node1", "outgoing", "", 10)
	}
}

// BenchmarkClient_VectorSearch benchmarks vector similarity search
func BenchmarkClient_VectorSearch(b *testing.B) {
	server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		w.WriteHeader(http.StatusOK)
		response := VectorSearchResponse{
			Results: []VectorSearchResult{
				{ID: "vec1", Score: 0.95},
				{ID: "vec2", Score: 0.88},
				{ID: "vec3", Score: 0.75},
			},
		}
		json.NewEncoder(w).Encode(response)
	}))
	defer server.Close()

	client := NewClient(Config{Endpoints: []string{server.URL}})
	ctx := context.Background()
	embedding := make([]float64, 128)
	for i := range embedding {
		embedding[i] = float64(i) / 128.0
	}

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		_, _ = client.VectorSearch(ctx, embedding, 10, nil)
	}
}

// BenchmarkClient_VectorUpsert benchmarks vector insert/update operations
func BenchmarkClient_VectorUpsert(b *testing.B) {
	server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		w.WriteHeader(http.StatusOK)
	}))
	defer server.Close()

	client := NewClient(Config{Endpoints: []string{server.URL}})
	ctx := context.Background()
	embedding := make([]float64, 128)
	for i := range embedding {
		embedding[i] = float64(i) / 128.0
	}
	metadata := map[string]interface{}{"category": "test"}

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		_ = client.VectorUpsert(ctx, "vec-123", embedding, metadata)
	}
}

// BenchmarkClient_Transaction benchmarks transaction operations
func BenchmarkClient_Transaction(b *testing.B) {
	server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		if r.URL.Path == "/transaction/begin" {
			w.WriteHeader(http.StatusOK)
			json.NewEncoder(w).Encode(map[string]interface{}{
				"transaction_id": "tx-12345",
			})
		} else if r.URL.Path == "/transaction/commit" {
			w.WriteHeader(http.StatusOK)
		} else {
			w.WriteHeader(http.StatusOK)
		}
	}))
	defer server.Close()

	client := NewClient(Config{Endpoints: []string{server.URL}})
	ctx := context.Background()

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		tx, _ := client.BeginTransaction(ctx, nil)
		_ = tx.Put(ctx, "relational", "users", "user-1", map[string]interface{}{"name": "Test"})
		_ = tx.Commit(ctx)
	}
}

// BenchmarkClient_ParallelRequests benchmarks concurrent request handling
func BenchmarkClient_ParallelRequests(b *testing.B) {
	server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		w.WriteHeader(http.StatusOK)
		json.NewEncoder(w).Encode(map[string]interface{}{
			"id":   "user-123",
			"name": "Alice",
		})
	}))
	defer server.Close()

	client := NewClient(Config{Endpoints: []string{server.URL}})
	ctx := context.Background()

	b.ResetTimer()
	b.RunParallel(func(pb *testing.PB) {
		for pb.Next() {
			var result map[string]interface{}
			_ = client.Get(ctx, "relational", "users", "user-123", &result)
		}
	})
}

// BenchmarkClient_LargePayload benchmarks handling of large JSON payloads
func BenchmarkClient_LargePayload(b *testing.B) {
	// Create a large payload
	largeData := make(map[string]interface{})
	for i := 0; i < 100; i++ {
		key := string(rune('a'+i%26)) + string(rune('0'+i/26))
		largeData[key] = map[string]interface{}{
			"field1": "value1",
			"field2": "value2",
			"field3": 12345,
			"nested": map[string]interface{}{
				"key1": "val1",
				"key2": "val2",
			},
		}
	}

	server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		w.WriteHeader(http.StatusOK)
		json.NewEncoder(w).Encode(largeData)
	}))
	defer server.Close()

	client := NewClient(Config{Endpoints: []string{server.URL}})
	ctx := context.Background()

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		var result map[string]interface{}
		_ = client.Get(ctx, "relational", "collection", "id", &result)
	}
}

// BenchmarkJSON_Marshal benchmarks JSON marshaling
func BenchmarkJSON_Marshal(b *testing.B) {
	data := map[string]interface{}{
		"id":    "user-123",
		"name":  "Alice",
		"email": "alice@example.com",
		"age":   30,
		"metadata": map[string]interface{}{
			"created_at": "2024-01-01T00:00:00Z",
			"updated_at": "2024-01-02T00:00:00Z",
		},
	}

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		_, _ = json.Marshal(data)
	}
}

// BenchmarkJSON_Unmarshal benchmarks JSON unmarshaling
func BenchmarkJSON_Unmarshal(b *testing.B) {
	jsonData := []byte(`{
		"id": "user-123",
		"name": "Alice",
		"email": "alice@example.com",
		"age": 30,
		"metadata": {
			"created_at": "2024-01-01T00:00:00Z",
			"updated_at": "2024-01-02T00:00:00Z"
		}
	}`)

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		var result map[string]interface{}
		_ = json.Unmarshal(jsonData, &result)
	}
}
