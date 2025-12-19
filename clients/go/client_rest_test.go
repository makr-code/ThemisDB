package themisdb

import (
	"context"
	"encoding/json"
	"net/http"
	"net/http/httptest"
	"testing"
	"time"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"
)

// TestClient_CRUD tests all CRUD operations via REST API
func TestClient_CRUD(t *testing.T) {
	tests := []struct {
		name           string
		method         string
		path           string
		requestBody    interface{}
		responseBody   interface{}
		responseStatus int
		operation      func(*Client, context.Context) error
		wantErr        bool
	}{
		{
			name:           "successful GET operation",
			method:         "GET",
			path:           "/api/relational/users/user-123",
			responseBody:   map[string]interface{}{"id": "user-123", "name": "Alice"},
			responseStatus: http.StatusOK,
			operation: func(c *Client, ctx context.Context) error {
				var result map[string]interface{}
				return c.Get(ctx, "relational", "users", "user-123", &result)
			},
			wantErr: false,
		},
		{
			name:           "GET with 404 error",
			method:         "GET",
			path:           "/api/relational/users/not-found",
			responseStatus: http.StatusNotFound,
			operation: func(c *Client, ctx context.Context) error {
				var result map[string]interface{}
				return c.Get(ctx, "relational", "users", "not-found", &result)
			},
			wantErr: true,
		},
		{
			name:           "successful PUT operation",
			method:         "PUT",
			path:           "/api/relational/users/user-456",
			requestBody:    map[string]interface{}{"name": "Bob", "email": "bob@example.com"},
			responseStatus: http.StatusOK,
			operation: func(c *Client, ctx context.Context) error {
				return c.Put(ctx, "relational", "users", "user-456", map[string]interface{}{"name": "Bob", "email": "bob@example.com"})
			},
			wantErr: false,
		},
		{
			name:           "successful DELETE operation",
			method:         "DELETE",
			path:           "/api/relational/users/user-789",
			responseStatus: http.StatusNoContent,
			operation: func(c *Client, ctx context.Context) error {
				return c.Delete(ctx, "relational", "users", "user-789")
			},
			wantErr: false,
		},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
				assert.Equal(t, tt.method, r.Method)
				assert.Equal(t, tt.path, r.URL.Path)
				assert.Equal(t, "application/json", r.Header.Get("Content-Type"))

				w.WriteHeader(tt.responseStatus)
				if tt.responseBody != nil {
					json.NewEncoder(w).Encode(tt.responseBody)
				}
			}))
			defer server.Close()

			client := NewClient(Config{
				Endpoints: []string{server.URL},
			})
			ctx := context.Background()

			err := tt.operation(client, ctx)
			if tt.wantErr {
				assert.Error(t, err)
			} else {
				assert.NoError(t, err)
			}
		})
	}
}

// TestClient_Query tests the AQL query functionality
func TestClient_Query(t *testing.T) {
	tests := []struct {
		name         string
		query        string
		responseData interface{}
		expectedErr  bool
	}{
		{
			name:  "simple query",
			query: "SELECT * FROM users",
			responseData: map[string]interface{}{
				"data": []interface{}{
					map[string]interface{}{"id": "1", "name": "Alice"},
					map[string]interface{}{"id": "2", "name": "Bob"},
				},
			},
			expectedErr: false,
		},
		{
			name:  "query with parameters",
			query: "SELECT * FROM users WHERE age > 25",
			responseData: map[string]interface{}{
				"data": []interface{}{
					map[string]interface{}{"id": "3", "name": "Charlie", "age": 30},
				},
			},
			expectedErr: false,
		},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
				assert.Equal(t, "POST", r.Method)
				assert.Equal(t, "/api/query", r.URL.Path)

				var reqBody map[string]interface{}
				json.NewDecoder(r.Body).Decode(&reqBody)
				assert.Equal(t, tt.query, reqBody["query"])

				w.WriteHeader(http.StatusOK)
				json.NewEncoder(w).Encode(tt.responseData)
			}))
			defer server.Close()

			client := NewClient(Config{
				Endpoints: []string{server.URL},
			})
			ctx := context.Background()

			var result []map[string]interface{}
			err := client.Query(ctx, tt.query, &result)

			if tt.expectedErr {
				assert.Error(t, err)
			} else {
				assert.NoError(t, err)
				assert.NotNil(t, result)
			}
		})
	}
}

// TestClient_GraphOperations tests graph-related REST API endpoints
func TestClient_GraphOperations(t *testing.T) {
	t.Run("GraphTraverse", func(t *testing.T) {
		expectedResponse := GraphTraverseResult{
			Nodes:   []string{"node1", "node2", "node3"},
			Visited: []string{"node1", "node2", "node3"},
		}

		server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
			assert.Equal(t, "POST", r.Method)
			assert.Equal(t, "/graph/traverse", r.URL.Path)

			var reqBody map[string]interface{}
			json.NewDecoder(r.Body).Decode(&reqBody)
			assert.Equal(t, "node1", reqBody["start"])
			assert.Equal(t, float64(3), reqBody["max_depth"])

			w.WriteHeader(http.StatusOK)
			json.NewEncoder(w).Encode(expectedResponse)
		}))
		defer server.Close()

		client := NewClient(Config{Endpoints: []string{server.URL}})
		ctx := context.Background()

		result, err := client.GraphTraverse(ctx, "node1", 3, "")
		require.NoError(t, err)
		assert.Equal(t, expectedResponse.Nodes, result.Nodes)
		assert.Equal(t, expectedResponse.Visited, result.Visited)
	})

	t.Run("ShortestPath", func(t *testing.T) {
		expectedPath := []string{"node1", "node2", "node3"}

		server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
			assert.Equal(t, "POST", r.Method)
			assert.Equal(t, "/graph/shortest-path", r.URL.Path)

			var reqBody map[string]interface{}
			json.NewDecoder(r.Body).Decode(&reqBody)
			assert.Equal(t, "node1", reqBody["from"])
			assert.Equal(t, "node3", reqBody["to"])

			w.WriteHeader(http.StatusOK)
			json.NewEncoder(w).Encode(map[string]interface{}{"path": expectedPath})
		}))
		defer server.Close()

		client := NewClient(Config{Endpoints: []string{server.URL}})
		ctx := context.Background()

		path, err := client.ShortestPath(ctx, "node1", "node3", "")
		require.NoError(t, err)
		assert.Equal(t, expectedPath, path)
	})

	t.Run("Neighbors", func(t *testing.T) {
		expectedNeighbors := []string{"node2", "node3", "node4"}

		server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
			assert.Equal(t, "POST", r.Method)
			assert.Equal(t, "/graph/neighbors", r.URL.Path)

			var reqBody map[string]interface{}
			json.NewDecoder(r.Body).Decode(&reqBody)
			assert.Equal(t, "node1", reqBody["node"])

			w.WriteHeader(http.StatusOK)
			json.NewEncoder(w).Encode(map[string]interface{}{"neighbors": expectedNeighbors})
		}))
		defer server.Close()

		client := NewClient(Config{Endpoints: []string{server.URL}})
		ctx := context.Background()

		neighbors, err := client.Neighbors(ctx, "node1", "outgoing", "", 10)
		require.NoError(t, err)
		assert.Equal(t, expectedNeighbors, neighbors)
	})
}

// TestClient_VectorOperations tests vector-related REST API endpoints
func TestClient_VectorOperations(t *testing.T) {
	t.Run("VectorSearch", func(t *testing.T) {
		embedding := []float64{0.1, 0.2, 0.3}
		expectedResults := &VectorSearchResponse{
			Results: []VectorSearchResult{
				{ID: "vec1", Score: 0.95},
				{ID: "vec2", Score: 0.88},
			},
		}

		server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
			assert.Equal(t, "POST", r.Method)
			assert.Equal(t, "/vector/search", r.URL.Path)

			var reqBody map[string]interface{}
			json.NewDecoder(r.Body).Decode(&reqBody)
			assert.Equal(t, float64(5), reqBody["k"])

			w.WriteHeader(http.StatusOK)
			json.NewEncoder(w).Encode(expectedResults)
		}))
		defer server.Close()

		client := NewClient(Config{Endpoints: []string{server.URL}})
		ctx := context.Background()

		results, err := client.VectorSearch(ctx, embedding, 5, nil)
		require.NoError(t, err)
		assert.Len(t, results.Results, 2)
		assert.Equal(t, "vec1", results.Results[0].ID)
	})

	t.Run("VectorUpsert", func(t *testing.T) {
		embedding := []float64{0.1, 0.2, 0.3}
		metadata := map[string]interface{}{"category": "test"}

		server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
			assert.Equal(t, "POST", r.Method)
			assert.Equal(t, "/vector/upsert", r.URL.Path)

			var reqBody map[string]interface{}
			json.NewDecoder(r.Body).Decode(&reqBody)
			assert.Equal(t, "vec-123", reqBody["id"])

			w.WriteHeader(http.StatusOK)
		}))
		defer server.Close()

		client := NewClient(Config{Endpoints: []string{server.URL}})
		ctx := context.Background()

		err := client.VectorUpsert(ctx, "vec-123", embedding, metadata)
		require.NoError(t, err)
	})

	t.Run("VectorDelete", func(t *testing.T) {
		server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
			assert.Equal(t, "DELETE", r.Method)
			assert.Equal(t, "/vector/vec-123", r.URL.Path)

			w.WriteHeader(http.StatusNoContent)
		}))
		defer server.Close()

		client := NewClient(Config{Endpoints: []string{server.URL}})
		ctx := context.Background()

		err := client.VectorDelete(ctx, "vec-123")
		require.NoError(t, err)
	})
}

// TestClient_Transaction tests transaction-related REST API endpoints
func TestClient_Transaction(t *testing.T) {
	t.Run("BeginTransaction", func(t *testing.T) {
		server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
			assert.Equal(t, "POST", r.Method)
			assert.Equal(t, "/transaction/begin", r.URL.Path)

			var reqBody map[string]interface{}
			json.NewDecoder(r.Body).Decode(&reqBody)
			assert.Equal(t, "SNAPSHOT", reqBody["isolation_level"])

			w.WriteHeader(http.StatusOK)
			json.NewEncoder(w).Encode(map[string]interface{}{
				"transaction_id": "tx-12345",
			})
		}))
		defer server.Close()

		client := NewClient(Config{Endpoints: []string{server.URL}})
		ctx := context.Background()

		tx, err := client.BeginTransaction(ctx, &TransactionOptions{
			IsolationLevel: Snapshot,
			Timeout:        30 * time.Second,
		})
		require.NoError(t, err)
		assert.NotNil(t, tx)
		assert.True(t, tx.IsActive())
		assert.Equal(t, "tx-12345", tx.TransactionID())
	})

	t.Run("TransactionOperations", func(t *testing.T) {
		var receivedTxID string
		server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
			receivedTxID = r.Header.Get("X-Transaction-Id")

			if r.URL.Path == "/api/relational/users/user-1" && r.Method == "PUT" {
				w.WriteHeader(http.StatusOK)
			} else if r.URL.Path == "/api/relational/users/user-1" && r.Method == "GET" {
				w.WriteHeader(http.StatusOK)
				json.NewEncoder(w).Encode(map[string]interface{}{"id": "user-1", "name": "Test"})
			} else if r.URL.Path == "/transaction/commit" {
				w.WriteHeader(http.StatusOK)
			}
		}))
		defer server.Close()

		client := NewClient(Config{Endpoints: []string{server.URL}})
		ctx := context.Background()

		tx := &Transaction{
			client:        client,
			transactionID: "tx-12345",
			active:        true,
		}

		// Test Put within transaction
		err := tx.Put(ctx, "relational", "users", "user-1", map[string]interface{}{"name": "Test"})
		require.NoError(t, err)
		assert.Equal(t, "tx-12345", receivedTxID)

		// Test Get within transaction
		var result map[string]interface{}
		err = tx.Get(ctx, "relational", "users", "user-1", &result)
		require.NoError(t, err)
		assert.Equal(t, "tx-12345", receivedTxID)

		// Test Commit
		err = tx.Commit(ctx)
		require.NoError(t, err)
		assert.False(t, tx.IsActive())
	})

	t.Run("TransactionRollback", func(t *testing.T) {
		server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
			if r.URL.Path == "/transaction/rollback" {
				w.WriteHeader(http.StatusOK)
			}
		}))
		defer server.Close()

		client := NewClient(Config{Endpoints: []string{server.URL}})
		ctx := context.Background()

		tx := &Transaction{
			client:        client,
			transactionID: "tx-12345",
			active:        true,
		}

		err := tx.Rollback(ctx)
		require.NoError(t, err)
		assert.False(t, tx.IsActive())
	})
}

// TestClient_ErrorHandling tests error scenarios
func TestClient_ErrorHandling(t *testing.T) {
	t.Run("network error", func(t *testing.T) {
		client := NewClient(Config{
			Endpoints: []string{"http://invalid-server:9999"},
			Timeout:   1 * time.Second,
		})
		ctx := context.Background()

		var result map[string]interface{}
		err := client.Get(ctx, "relational", "users", "123", &result)
		assert.Error(t, err)
	})

	t.Run("timeout", func(t *testing.T) {
		server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
			time.Sleep(2 * time.Second)
			w.WriteHeader(http.StatusOK)
		}))
		defer server.Close()

		client := NewClient(Config{
			Endpoints: []string{server.URL},
			Timeout:   100 * time.Millisecond,
		})
		ctx := context.Background()

		var result map[string]interface{}
		err := client.Get(ctx, "relational", "users", "123", &result)
		assert.Error(t, err)
	})

	t.Run("invalid JSON response", func(t *testing.T) {
		server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
			w.WriteHeader(http.StatusOK)
			w.Write([]byte("invalid json"))
		}))
		defer server.Close()

		client := NewClient(Config{Endpoints: []string{server.URL}})
		ctx := context.Background()

		var result map[string]interface{}
		err := client.Get(ctx, "relational", "users", "123", &result)
		assert.Error(t, err)
	})

	t.Run("HTTP 500 error", func(t *testing.T) {
		server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
			w.WriteHeader(http.StatusInternalServerError)
			w.Write([]byte("Internal Server Error"))
		}))
		defer server.Close()

		client := NewClient(Config{Endpoints: []string{server.URL}})
		ctx := context.Background()

		err := client.Put(ctx, "relational", "users", "123", map[string]interface{}{"name": "Test"})
		assert.Error(t, err)
		assert.Contains(t, err.Error(), "500")
	})
}

// TestClient_ContextCancellation tests context cancellation
func TestClient_ContextCancellation(t *testing.T) {
	server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		time.Sleep(2 * time.Second)
		w.WriteHeader(http.StatusOK)
	}))
	defer server.Close()

	client := NewClient(Config{Endpoints: []string{server.URL}})
	ctx, cancel := context.WithTimeout(context.Background(), 100*time.Millisecond)
	defer cancel()

	var result map[string]interface{}
	err := client.Get(ctx, "relational", "users", "123", &result)
	assert.Error(t, err)
}

// TestClient_EndpointSelection tests endpoint management
func TestClient_EndpointSelection(t *testing.T) {
	client := NewClient(Config{
		Endpoints: []string{
			"http://server1:8080",
			"http://server2:8080",
			"http://server3:8080",
		},
	})

	endpoint := client.getEndpoint()
	assert.Contains(t, []string{
		"http://server1:8080",
		"http://server2:8080",
		"http://server3:8080",
	}, endpoint)
}
