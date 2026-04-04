package themisdb

import (
	"bytes"
	"context"
	"encoding/json"
	"fmt"
	"io"
	"net/http"
	"strings"
	"sync"
	"time"
)

// IsolationLevel represents transaction isolation levels
type IsolationLevel string

const (
	// ReadCommitted allows reading committed data from other transactions
	ReadCommitted IsolationLevel = "READ_COMMITTED"
	// Snapshot provides point-in-time consistent reads
	Snapshot IsolationLevel = "SNAPSHOT"
)

// LlmMessage represents a message in an LLM conversation
type LlmMessage struct {
	Role     string `json:"role"`
	Content  string `json:"content"`
	ImageURL string `json:"image_url,omitempty"`
}

// ReasoningStep represents a reasoning step in LLM interaction
type ReasoningStep struct {
	Type    string   `json:"type"`
	Content []string `json:"content"`
}

// LlmInteraction represents a stored LLM interaction
type LlmInteraction struct {
	ID             string                 `json:"id"`
	CreatedAt      string                 `json:"created_at"`
	Model          string                 `json:"model"`
	Messages       []LlmMessage           `json:"messages"`
	ReasoningSteps []ReasoningStep        `json:"reasoning_steps,omitempty"`
	Metadata       map[string]interface{} `json:"metadata,omitempty"`
}

// LlmInteractionResult represents the result of creating an LLM interaction
type LlmInteractionResult struct {
	ID      string `json:"id"`
	Success bool   `json:"success"`
}

// LlmInteractionOptions holds options for creating an LLM interaction
type LlmInteractionOptions struct {
	ReasoningSteps []ReasoningStep
	Metadata       map[string]interface{}
}

// ListLlmInteractionsOptions holds options for listing LLM interactions
type ListLlmInteractionsOptions struct {
	Model  string
	Limit  int
	Offset int
}

// Client is the ThemisDB client
type Client struct {
	endpoints      []string
	httpClient     *http.Client
	mu             sync.RWMutex
	activeIdx      int
	maxRetries     int
	circuitBreaker *circuitBreaker
	loggingEnabled bool
	logRequests    bool
	logResponses   bool
	logger         func(string, string)
}

// Config holds client configuration
type Config struct {
	// Endpoints is a list of ThemisDB server endpoints
	Endpoints []string
	// Timeout for HTTP requests (default: 30s)
	Timeout time.Duration
	// MaxRetries for failed requests (default: 3)
	MaxRetries int
	// CircuitBreaker configuration
	CircuitBreaker *CircuitBreakerConfig
	// Logging configuration
	Logging *LoggingConfig
}

// CircuitBreakerConfig holds circuit breaker configuration
type CircuitBreakerConfig struct {
	// Enabled turns on circuit breaker
	Enabled bool
	// FailureThreshold is the number of failures before opening circuit (default: 5)
	FailureThreshold int
	// ResetTimeout is how long to wait before attempting reset (default: 60s)
	ResetTimeout time.Duration
	// HalfOpenMaxRequests is max requests in half-open state (default: 3)
	HalfOpenMaxRequests int
}

// LoggingConfig holds logging configuration
type LoggingConfig struct {
	// Enabled turns on logging
	Enabled bool
	// LogRequests enables request logging
	LogRequests bool
	// LogResponses enables response logging
	LogResponses bool
	// Logger is a custom logger function
	Logger func(message string, level string)
}

// circuitBreakerState represents the state of a circuit breaker
type circuitBreakerState int

const (
	circuitClosed circuitBreakerState = iota
	circuitOpen
	circuitHalfOpen
)

// circuitBreaker implements the circuit breaker pattern
type circuitBreaker struct {
	state               circuitBreakerState
	failureCount        int
	successCount        int
	nextAttemptTime     time.Time
	failureThreshold    int
	resetTimeout        time.Duration
	halfOpenMaxRequests int
	mu                  sync.RWMutex
}

func newCircuitBreaker(config *CircuitBreakerConfig) *circuitBreaker {
	if config.FailureThreshold == 0 {
		config.FailureThreshold = 5
	}
	if config.ResetTimeout == 0 {
		config.ResetTimeout = 60 * time.Second
	}
	if config.HalfOpenMaxRequests == 0 {
		config.HalfOpenMaxRequests = 3
	}
	
	return &circuitBreaker{
		state:               circuitClosed,
		failureThreshold:    config.FailureThreshold,
		resetTimeout:        config.ResetTimeout,
		halfOpenMaxRequests: config.HalfOpenMaxRequests,
	}
}

func (cb *circuitBreaker) canExecute() bool {
	cb.mu.RLock()
	defer cb.mu.RUnlock()
	
	if cb.state == circuitClosed {
		return true
	}
	if cb.state == circuitOpen {
		if time.Now().After(cb.nextAttemptTime) {
			// Don't transition here, let caller handle it if request succeeds
			return true
		}
		return false
	}
	// half-open state
	return cb.successCount < cb.halfOpenMaxRequests
}

func (cb *circuitBreaker) recordSuccess() {
	cb.mu.Lock()
	defer cb.mu.Unlock()
	
	if cb.state == circuitHalfOpen {
		cb.successCount++
		if cb.successCount >= cb.halfOpenMaxRequests {
			cb.state = circuitClosed
			cb.failureCount = 0
		}
	} else if cb.state == circuitClosed {
		cb.failureCount = 0
	}
}

func (cb *circuitBreaker) recordFailure() {
	cb.mu.Lock()
	defer cb.mu.Unlock()
	
	cb.failureCount++
	if cb.failureCount >= cb.failureThreshold {
		cb.state = circuitOpen
		cb.nextAttemptTime = time.Now().Add(cb.resetTimeout)
	}
}

func (cb *circuitBreaker) transitionToHalfOpen() {
	cb.mu.Lock()
	defer cb.mu.Unlock()
	
	cb.state = circuitHalfOpen
	cb.successCount = 0
}

func (cb *circuitBreaker) getState() string {
	cb.mu.RLock()
	defer cb.mu.RUnlock()
	
	switch cb.state {
	case circuitClosed:
		return "CLOSED"
	case circuitOpen:
		return "OPEN"
	case circuitHalfOpen:
		return "HALF_OPEN"
	default:
		return "UNKNOWN"
	}
}

// NewClient creates a new ThemisDB client
func NewClient(config Config) *Client {
	if config.Timeout == 0 {
		config.Timeout = 30 * time.Second
	}
	if config.MaxRetries == 0 {
		config.MaxRetries = 3
	}
	if len(config.Endpoints) == 0 {
		config.Endpoints = []string{"http://localhost:8080"}
	}

	client := &Client{
		endpoints: config.Endpoints,
		httpClient: &http.Client{
			Timeout: config.Timeout,
		},
		activeIdx:  0,
		maxRetries: config.MaxRetries,
	}

	// Initialize circuit breaker if enabled
	if config.CircuitBreaker != nil && config.CircuitBreaker.Enabled {
		client.circuitBreaker = newCircuitBreaker(config.CircuitBreaker)
	}

	// Initialize logging
	if config.Logging != nil && config.Logging.Enabled {
		client.loggingEnabled = true
		client.logRequests = config.Logging.LogRequests
		client.logResponses = config.Logging.LogResponses
		if config.Logging.Logger != nil {
			client.logger = config.Logging.Logger
		} else {
			client.logger = func(msg, level string) {
				fmt.Printf("[ThemisDB] [%s] %s\n", level, msg)
			}
		}
	}

	return client
}

// Get retrieves an entity by UUID
func (c *Client) Get(ctx context.Context, model, collection, uuid string, result interface{}) error {
	path := fmt.Sprintf("/api/%s/%s/%s", model, collection, uuid)
	return c.request(ctx, "GET", path, nil, result, nil)
}

// Put creates or updates an entity
func (c *Client) Put(ctx context.Context, model, collection, uuid string, data interface{}) error {
	path := fmt.Sprintf("/api/%s/%s/%s", model, collection, uuid)
	return c.request(ctx, "PUT", path, data, nil, nil)
}

// Delete removes an entity by UUID
func (c *Client) Delete(ctx context.Context, model, collection, uuid string) error {
	path := fmt.Sprintf("/api/%s/%s/%s", model, collection, uuid)
	return c.request(ctx, "DELETE", path, nil, nil, nil)
}

// QueryResult holds query results
type QueryResult struct {
	Data interface{} `json:"data"`
}

// Query executes an AQL query
func (c *Client) Query(ctx context.Context, aql string, result interface{}) error {
	path := "/api/query"
	body := map[string]interface{}{
		"query": aql,
	}
	var queryResult QueryResult
	if err := c.request(ctx, "POST", path, body, &queryResult, nil); err != nil {
		return err
	}
	
	// Marshal and unmarshal to convert to result type
	data, err := json.Marshal(queryResult.Data)
	if err != nil {
		return fmt.Errorf("failed to marshal query result: %w", err)
	}
	if err := json.Unmarshal(data, result); err != nil {
		return fmt.Errorf("failed to unmarshal query result: %w", err)
	}
	return nil
}

// LlmInteraction creates an LLM interaction
func (c *Client) LlmInteraction(ctx context.Context, model string, messages []LlmMessage, opts *LlmInteractionOptions) (*LlmInteractionResult, error) {
	body := map[string]interface{}{
		"model":    model,
		"messages": messages,
	}
	
	if opts != nil {
		if opts.ReasoningSteps != nil {
			body["reasoning_steps"] = opts.ReasoningSteps
		}
		if opts.Metadata != nil {
			body["metadata"] = opts.Metadata
		}
	}
	
	var result LlmInteractionResult
	if err := c.request(ctx, "POST", "/llm/interaction", body, &result, nil); err != nil {
		return nil, fmt.Errorf("failed to create LLM interaction: %w", err)
	}
	
	return &result, nil
}

// GetLlmInteraction retrieves a specific LLM interaction by ID
func (c *Client) GetLlmInteraction(ctx context.Context, interactionID string) (*LlmInteraction, error) {
	path := fmt.Sprintf("/llm/interaction/%s", interactionID)
	
	var interaction LlmInteraction
	if err := c.request(ctx, "GET", path, nil, &interaction, nil); err != nil {
		if strings.Contains(err.Error(), "404") {
			return nil, nil
		}
		return nil, fmt.Errorf("failed to get LLM interaction: %w", err)
	}
	
	return &interaction, nil
}

// ListLlmInteractions lists LLM interactions with optional filtering
func (c *Client) ListLlmInteractions(ctx context.Context, opts *ListLlmInteractionsOptions) ([]LlmInteraction, error) {
	path := "/llm/interaction"
	
	if opts != nil {
		params := []string{}
		if opts.Model != "" {
			params = append(params, fmt.Sprintf("model=%s", opts.Model))
		}
		if opts.Limit > 0 {
			params = append(params, fmt.Sprintf("limit=%d", opts.Limit))
		}
		if opts.Offset > 0 {
			params = append(params, fmt.Sprintf("offset=%d", opts.Offset))
		}
		if len(params) > 0 {
			path = path + "?" + strings.Join(params, "&")
		}
	}
	
	var response struct {
		Interactions []LlmInteraction `json:"interactions"`
	}
	
	if err := c.request(ctx, "GET", path, nil, &response, nil); err != nil {
		return nil, fmt.Errorf("failed to list LLM interactions: %w", err)
	}
	
	if response.Interactions == nil {
		return []LlmInteraction{}, nil
	}
	
	return response.Interactions, nil
}

// request performs an HTTP request with retry and circuit breaker support
func (c *Client) request(ctx context.Context, method, path string, body interface{}, result interface{}, headers map[string]string) error {
	// Check circuit breaker
	if c.circuitBreaker != nil {
		if !c.circuitBreaker.canExecute() {
			err := fmt.Errorf("circuit breaker is OPEN")
			if c.loggingEnabled {
				c.logger(fmt.Sprintf("Circuit breaker blocked request: %s %s", method, path), "WARN")
			}
			return err
		}
		// Transition to half-open if needed (under lock)
		c.circuitBreaker.mu.Lock()
		if c.circuitBreaker.state == circuitOpen && time.Now().After(c.circuitBreaker.nextAttemptTime) {
			c.circuitBreaker.state = circuitHalfOpen
			c.circuitBreaker.successCount = 0
		}
		c.circuitBreaker.mu.Unlock()
	}

	var lastErr error
	for attempt := 0; attempt < c.maxRetries; attempt++ {
		if attempt > 0 {
			// Exponential backoff
			backoff := time.Duration(1<<uint(attempt-1)) * 100 * time.Millisecond
			if c.loggingEnabled {
				c.logger(fmt.Sprintf("Retry attempt %d after %v", attempt, backoff), "INFO")
			}
			time.Sleep(backoff)
		}

		var reqBody io.Reader
		if body != nil {
			data, err := json.Marshal(body)
			if err != nil {
				return fmt.Errorf("failed to marshal request body: %w", err)
			}
			reqBody = bytes.NewReader(data)
		}

		endpoint := c.getEndpoint()
		url := endpoint + path

		// Log request if enabled
		if c.loggingEnabled && c.logRequests {
			c.logger(fmt.Sprintf("Request: %s %s", method, url), "INFO")
		}

		req, err := http.NewRequestWithContext(ctx, method, url, reqBody)
		if err != nil {
			return fmt.Errorf("failed to create request: %w", err)
		}

		req.Header.Set("Content-Type", "application/json")
		for key, value := range headers {
			req.Header.Set(key, value)
		}

		resp, err := c.httpClient.Do(req)
		if err != nil {
			lastErr = fmt.Errorf("request failed: %w", err)
			if c.loggingEnabled {
				c.logger(fmt.Sprintf("Request error: %v", err), "ERROR")
			}
			// Record failure for circuit breaker
			if c.circuitBreaker != nil {
				c.circuitBreaker.recordFailure()
			}
			continue
		}
		defer resp.Body.Close()

		// Log response if enabled
		if c.loggingEnabled && c.logResponses {
			c.logger(fmt.Sprintf("Response: %s %s - Status: %d", method, url, resp.StatusCode), "INFO")
		}

		// Retry on 5xx errors
		if resp.StatusCode >= 500 && attempt+1 < c.maxRetries {
			bodyBytes, _ := io.ReadAll(resp.Body)
			lastErr = fmt.Errorf("request failed with status %d: %s", resp.StatusCode, string(bodyBytes))
			if c.loggingEnabled {
				c.logger(fmt.Sprintf("Server error %d, will retry", resp.StatusCode), "WARN")
			}
			// Record failure for circuit breaker
			if c.circuitBreaker != nil {
				c.circuitBreaker.recordFailure()
			}
			continue
		}

		if resp.StatusCode >= 400 {
			bodyBytes, _ := io.ReadAll(resp.Body)
			err := fmt.Errorf("request failed with status %d: %s", resp.StatusCode, string(bodyBytes))
			// Record failure for circuit breaker
			if c.circuitBreaker != nil {
				c.circuitBreaker.recordFailure()
			}
			return err
		}

		// Success - record for circuit breaker
		if c.circuitBreaker != nil {
			c.circuitBreaker.recordSuccess()
		}

		if result != nil && resp.StatusCode != http.StatusNoContent {
			if err := json.NewDecoder(resp.Body).Decode(result); err != nil {
				return fmt.Errorf("failed to decode response: %w", err)
			}
		}

		return nil
	}

	// All retries exhausted
	if c.circuitBreaker != nil {
		c.circuitBreaker.recordFailure()
	}
	if lastErr != nil {
		return fmt.Errorf("all retries exhausted: %w", lastErr)
	}
	return fmt.Errorf("request failed after %d attempts", c.maxRetries)
}

// getEndpoint returns the current active endpoint
func (c *Client) getEndpoint() string {
	c.mu.RLock()
	defer c.mu.RUnlock()
	return strings.TrimSuffix(c.endpoints[c.activeIdx], "/")
}

// TransactionOptions holds transaction configuration
type TransactionOptions struct {
	IsolationLevel IsolationLevel
	Timeout        time.Duration
}

// Transaction represents an ACID transaction
type Transaction struct {
	client        *Client
	transactionID string
	active        bool
	mu            sync.RWMutex
}

// BeginTransaction starts a new ACID transaction
func (c *Client) BeginTransaction(ctx context.Context, opts *TransactionOptions) (*Transaction, error) {
	if opts == nil {
		opts = &TransactionOptions{
			IsolationLevel: ReadCommitted,
			Timeout:        30 * time.Second,
		}
	}

	reqBody := map[string]interface{}{
		"isolation_level": string(opts.IsolationLevel),
	}
	if opts.Timeout > 0 {
		reqBody["timeout"] = opts.Timeout.Seconds()
	}

	var response struct {
		TransactionID string `json:"transaction_id"`
	}

	if err := c.request(ctx, "POST", "/transaction/begin", reqBody, &response, nil); err != nil {
		return nil, fmt.Errorf("failed to begin transaction: %w", err)
	}

	return &Transaction{
		client:        c,
		transactionID: response.TransactionID,
		active:        true,
	}, nil
}

// IsActive returns whether the transaction is still active
func (tx *Transaction) IsActive() bool {
	tx.mu.RLock()
	defer tx.mu.RUnlock()
	return tx.active
}

// TransactionID returns the transaction ID
func (tx *Transaction) TransactionID() string {
	return tx.transactionID
}

// Get retrieves an entity within the transaction
func (tx *Transaction) Get(ctx context.Context, model, collection, uuid string, result interface{}) error {
	if !tx.IsActive() {
		return ErrTransactionNotActive
	}

	path := fmt.Sprintf("/api/%s/%s/%s", model, collection, uuid)
	headers := map[string]string{
		"X-Transaction-Id": tx.transactionID,
	}
	return tx.client.request(ctx, "GET", path, nil, result, headers)
}

// Put creates or updates an entity within the transaction
func (tx *Transaction) Put(ctx context.Context, model, collection, uuid string, data interface{}) error {
	if !tx.IsActive() {
		return ErrTransactionNotActive
	}

	path := fmt.Sprintf("/api/%s/%s/%s", model, collection, uuid)
	headers := map[string]string{
		"X-Transaction-Id": tx.transactionID,
	}
	return tx.client.request(ctx, "PUT", path, data, nil, headers)
}

// Delete removes an entity within the transaction
func (tx *Transaction) Delete(ctx context.Context, model, collection, uuid string) error {
	if !tx.IsActive() {
		return ErrTransactionNotActive
	}

	path := fmt.Sprintf("/api/%s/%s/%s", model, collection, uuid)
	headers := map[string]string{
		"X-Transaction-Id": tx.transactionID,
	}
	return tx.client.request(ctx, "DELETE", path, nil, nil, headers)
}

// Query executes an AQL query within the transaction
func (tx *Transaction) Query(ctx context.Context, aql string, result interface{}) error {
	if !tx.IsActive() {
		return ErrTransactionNotActive
	}

	path := "/api/query"
	body := map[string]interface{}{
		"query": aql,
	}
	headers := map[string]string{
		"X-Transaction-Id": tx.transactionID,
	}
	var queryResult QueryResult
	if err := tx.client.request(ctx, "POST", path, body, &queryResult, headers); err != nil {
		return err
	}

	// Marshal and unmarshal to convert to result type
	data, err := json.Marshal(queryResult.Data)
	if err != nil {
		return fmt.Errorf("failed to marshal query result: %w", err)
	}
	if err := json.Unmarshal(data, result); err != nil {
		return fmt.Errorf("failed to unmarshal query result: %w", err)
	}
	return nil
}

// Commit commits the transaction
func (tx *Transaction) Commit(ctx context.Context) error {
	tx.mu.Lock()
	defer tx.mu.Unlock()

	if !tx.active {
		return ErrTransactionNotActive
	}

	reqBody := map[string]interface{}{
		"transaction_id": tx.transactionID,
	}

	if err := tx.client.request(ctx, "POST", "/transaction/commit", reqBody, nil, nil); err != nil {
		return fmt.Errorf("failed to commit transaction: %w", err)
	}

	tx.active = false
	return nil
}

// Rollback rolls back the transaction
func (tx *Transaction) Rollback(ctx context.Context) error {
	tx.mu.Lock()
	defer tx.mu.Unlock()

	if !tx.active {
		return ErrTransactionNotActive
	}

	reqBody := map[string]interface{}{
		"transaction_id": tx.transactionID,
	}

	if err := tx.client.request(ctx, "POST", "/transaction/rollback", reqBody, nil, nil); err != nil {
		return fmt.Errorf("failed to rollback transaction: %w", err)
	}

	tx.active = false
	return nil
}

// Error variables
var (
	// ErrTransactionNotActive indicates the transaction is no longer active
	ErrTransactionNotActive = fmt.Errorf("transaction is not active")
)

// GraphTraverseResult holds the result of a graph traversal
type GraphTraverseResult struct {
	Nodes   []string               `json:"nodes"`
	Visited []string               `json:"visited"`
	Edges   []map[string]interface{} `json:"edges,omitempty"`
}

// GraphTraverse performs a graph traversal starting from a node
func (c *Client) GraphTraverse(ctx context.Context, startNode string, maxDepth int, edgeType string) (*GraphTraverseResult, error) {
	path := "/graph/traverse"
	body := map[string]interface{}{
		"start":     startNode,
		"max_depth": maxDepth,
	}
	if edgeType != "" {
		body["edge_type"] = edgeType
	}

	var result GraphTraverseResult
	if err := c.request(ctx, "POST", path, body, &result, nil); err != nil {
		return nil, err
	}
	return &result, nil
}

// ShortestPath finds the shortest path between two nodes
func (c *Client) ShortestPath(ctx context.Context, from, to string, edgeType string) ([]string, error) {
	path := "/graph/shortest-path"
	body := map[string]interface{}{
		"from": from,
		"to":   to,
	}
	if edgeType != "" {
		body["edge_type"] = edgeType
	}

	var result struct {
		Path []string `json:"path"`
	}
	if err := c.request(ctx, "POST", path, body, &result, nil); err != nil {
		return nil, err
	}
	return result.Path, nil
}

// Neighbors retrieves the neighboring nodes
func (c *Client) Neighbors(ctx context.Context, nodeID string, direction string, edgeType string, limit int) ([]string, error) {
	path := "/graph/neighbors"
	body := map[string]interface{}{
		"node": nodeID,
	}
	if direction != "" {
		body["direction"] = direction
	}
	if edgeType != "" {
		body["edge_type"] = edgeType
	}
	if limit > 0 {
		body["limit"] = limit
	}

	var result struct {
		Neighbors []string `json:"neighbors"`
	}
	if err := c.request(ctx, "POST", path, body, &result, nil); err != nil {
		return nil, err
	}
	return result.Neighbors, nil
}

// VectorSearchResult holds the result of a vector search
type VectorSearchResult struct {
	ID       string                 `json:"id"`
	Score    float64                `json:"score"`
	Distance float64                `json:"distance,omitempty"`
	Metadata map[string]interface{} `json:"metadata,omitempty"`
}

// VectorSearchResponse holds the full response from vector search
type VectorSearchResponse struct {
	Results []VectorSearchResult `json:"results"`
}

// VectorSearch performs a similarity search on vectors
func (c *Client) VectorSearch(ctx context.Context, embedding []float64, topK int, filter map[string]interface{}) (*VectorSearchResponse, error) {
	path := "/vector/search"
	body := map[string]interface{}{
		"vector": embedding,
		"k":      topK,
	}
	if filter != nil {
		body["filter"] = filter
	}

	var result VectorSearchResponse
	if err := c.request(ctx, "POST", path, body, &result, nil); err != nil {
		return nil, err
	}
	return &result, nil
}

// VectorUpsert inserts or updates a vector
func (c *Client) VectorUpsert(ctx context.Context, id string, embedding []float64, metadata map[string]interface{}) error {
	path := "/vector/upsert"
	body := map[string]interface{}{
		"id":     id,
		"vector": embedding,
	}
	if metadata != nil {
		body["metadata"] = metadata
	}

	return c.request(ctx, "POST", path, body, nil, nil)
}

// VectorDelete deletes a vector by ID
func (c *Client) VectorDelete(ctx context.Context, id string) error {
	path := fmt.Sprintf("/vector/%s", id)
	return c.request(ctx, "DELETE", path, nil, nil, nil)
}

// GetCircuitBreakerState returns the current circuit breaker state or empty string if disabled
func (c *Client) GetCircuitBreakerState() string {
	if c.circuitBreaker == nil {
		return ""
	}
	return c.circuitBreaker.getState()
}
