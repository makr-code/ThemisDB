package com.themisdb.client;

import com.google.gson.Gson;
import com.google.gson.JsonObject;

import java.io.IOException;
import java.net.URI;
import java.net.http.HttpClient;
import java.net.http.HttpRequest;
import java.net.http.HttpResponse;
import java.time.Duration;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * ThemisDB Java Client
 * 
 * A Java client library for interacting with ThemisDB multi-model database.
 * Supports CRUD operations, transactions, and multiple data models (relational, document, graph, vector).
 */
public class ThemisClient {
    private final List<String> endpoints;
    private final AtomicInteger currentEndpointIndex;
    private final HttpClient httpClient;
    private final Gson gson;
    private final Duration timeout;
    private final int maxRetries;
    private final CircuitBreaker circuitBreaker;
    private final boolean loggingEnabled;
    private final boolean logRequests;
    private final boolean logResponses;
    private final ClientConfig.Logger logger;

    /**
     * Create a new ThemisDB client
     * 
     * @param endpoints List of ThemisDB server endpoints (e.g., ["http://localhost:8080"])
     */
    public ThemisClient(List<String> endpoints) {
        this(endpoints, Duration.ofSeconds(30));
    }

    /**
     * Create a new ThemisDB client with custom timeout
     * 
     * @param endpoints List of ThemisDB server endpoints
     * @param timeout Request timeout duration
     */
    public ThemisClient(List<String> endpoints, Duration timeout) {
        this(endpoints, timeout, null);
    }
    
    /**
     * Create a new ThemisDB client with full configuration
     * 
     * @param endpoints List of ThemisDB server endpoints
     * @param timeout Request timeout duration
     * @param config Client configuration (can be null for defaults)
     */
    public ThemisClient(List<String> endpoints, Duration timeout, ClientConfig config) {
        if (endpoints == null || endpoints.isEmpty()) {
            throw new IllegalArgumentException("At least one endpoint is required");
        }
        this.endpoints = endpoints;
        this.currentEndpointIndex = new AtomicInteger(0);
        this.httpClient = HttpClient.newBuilder()
                .connectTimeout(timeout)
                .build();
        this.gson = new Gson();
        this.timeout = timeout;
        
        // Apply configuration or use defaults
        if (config != null) {
            this.maxRetries = config.getMaxRetries();
            
            // Initialize circuit breaker if enabled
            if (config.getCircuitBreaker() != null && config.getCircuitBreaker().isEnabled()) {
                this.circuitBreaker = new CircuitBreaker(
                    config.getCircuitBreaker().getFailureThreshold(),
                    config.getCircuitBreaker().getResetTimeout(),
                    config.getCircuitBreaker().getHalfOpenMaxRequests()
                );
            } else {
                this.circuitBreaker = null;
            }
            
            // Initialize logging
            if (config.getLogging() != null && config.getLogging().isEnabled()) {
                this.loggingEnabled = true;
                this.logRequests = config.getLogging().isLogRequests();
                this.logResponses = config.getLogging().isLogResponses();
                this.logger = config.getLogging().getLogger();
            } else {
                this.loggingEnabled = false;
                this.logRequests = false;
                this.logResponses = false;
                this.logger = null;
            }
        } else {
            this.maxRetries = 3;
            this.circuitBreaker = null;
            this.loggingEnabled = false;
            this.logRequests = false;
            this.logResponses = false;
            this.logger = null;
        }
    }

    /**
     * Get the current endpoint URL
     * 
     * @return Current endpoint URL
     */
    public String getCurrentEndpoint() {
        int index = currentEndpointIndex.get() % endpoints.size();
        return endpoints.get(index);
    }

    /**
     * Rotate to the next endpoint (for failover)
     */
    private void rotateEndpoint() {
        currentEndpointIndex.incrementAndGet();
    }

    /**
     * Get a value from the database
     * 
     * @param model Data model (e.g., "relational", "document", "graph", "vector")
     * @param collection Collection name
     * @param uuid Record UUID
     * @param <T> Type to deserialize the response to
     * @return The retrieved value
     * @throws IOException If the request fails
     */
    public <T> T get(String model, String collection, String uuid, Class<T> clazz) throws IOException, InterruptedException {
        String url = String.format("%s/api/%s/%s/%s", getCurrentEndpoint(), model, collection, uuid);
        HttpRequest request = HttpRequest.newBuilder()
                .uri(URI.create(url))
                .timeout(timeout)
                .GET()
                .build();

        HttpResponse<String> response = sendRequestWithRetry(request);
        
        if (response.statusCode() != 200) {
            throw new IOException("GET request failed with status: " + response.statusCode());
        }

        return gson.fromJson(response.body(), clazz);
    }

    /**
     * Put a value into the database
     * 
     * @param model Data model
     * @param collection Collection name
     * @param uuid Record UUID
     * @param data Data to store
     * @throws IOException If the request fails
     */
    public void put(String model, String collection, String uuid, Object data) throws IOException, InterruptedException {
        String url = String.format("%s/api/%s/%s/%s", getCurrentEndpoint(), model, collection, uuid);
        String jsonBody = gson.toJson(data);
        
        HttpRequest request = HttpRequest.newBuilder()
                .uri(URI.create(url))
                .timeout(timeout)
                .header("Content-Type", "application/json")
                .PUT(HttpRequest.BodyPublishers.ofString(jsonBody))
                .build();

        HttpResponse<String> response = sendRequestWithRetry(request);
        
        if (response.statusCode() != 200 && response.statusCode() != 201) {
            throw new IOException("PUT request failed with status: " + response.statusCode());
        }
    }

    /**
     * Delete a value from the database
     * 
     * @param model Data model
     * @param collection Collection name
     * @param uuid Record UUID
     * @throws IOException If the request fails
     */
    public void delete(String model, String collection, String uuid) throws IOException, InterruptedException {
        String url = String.format("%s/api/%s/%s/%s", getCurrentEndpoint(), model, collection, uuid);
        
        HttpRequest request = HttpRequest.newBuilder()
                .uri(URI.create(url))
                .timeout(timeout)
                .DELETE()
                .build();

        HttpResponse<String> response = sendRequestWithRetry(request);
        
        if (response.statusCode() != 200 && response.statusCode() != 204) {
            throw new IOException("DELETE request failed with status: " + response.statusCode());
        }
    }

    /**
     * Execute a query using AQL (Advanced Query Language)
     * 
     * @param query AQL query string
     * @param <T> Type to deserialize the response to
     * @return Query results
     * @throws IOException If the request fails
     */
    public <T> T query(String query, Class<T> clazz) throws IOException, InterruptedException {
        String url = String.format("%s/query", getCurrentEndpoint());
        JsonObject requestBody = new JsonObject();
        requestBody.addProperty("query", query);
        
        HttpRequest request = HttpRequest.newBuilder()
                .uri(URI.create(url))
                .timeout(timeout)
                .header("Content-Type", "application/json")
                .POST(HttpRequest.BodyPublishers.ofString(gson.toJson(requestBody)))
                .build();

        HttpResponse<String> response = httpClient.send(request, HttpResponse.BodyHandlers.ofString());
        
        if (response.statusCode() != 200) {
            throw new IOException("QUERY request failed with status: " + response.statusCode());
        }

        return gson.fromJson(response.body(), clazz);
    }

    /**
     * Begin a new transaction
     * 
     * @return Transaction object
     * @throws IOException If the request fails
     */
    public Transaction beginTransaction() throws IOException, InterruptedException {
        return beginTransaction(new TransactionOptions());
    }

    /**
     * Begin a new transaction with options
     * 
     * @param options Transaction options (isolation level, timeout)
     * @return Transaction object
     * @throws IOException If the request fails
     */
    public Transaction beginTransaction(TransactionOptions options) throws IOException, InterruptedException {
        String url = String.format("%s/transaction/begin", getCurrentEndpoint());
        JsonObject requestBody = new JsonObject();
        requestBody.addProperty("isolation_level", options.getIsolationLevel().toString());
        if (options.getTimeout() != null) {
            requestBody.addProperty("timeout", options.getTimeout().toMillis());
        }
        
        HttpRequest request = HttpRequest.newBuilder()
                .uri(URI.create(url))
                .timeout(timeout)
                .header("Content-Type", "application/json")
                .POST(HttpRequest.BodyPublishers.ofString(gson.toJson(requestBody)))
                .build();

        HttpResponse<String> response = httpClient.send(request, HttpResponse.BodyHandlers.ofString());
        
        if (response.statusCode() != 200 && response.statusCode() != 201) {
            throw new IOException("BEGIN TRANSACTION failed with status: " + response.statusCode());
        }

        JsonObject responseBody = gson.fromJson(response.body(), JsonObject.class);
        String transactionId = responseBody.get("transaction_id").getAsString();
        
        return new Transaction(this, transactionId, httpClient, gson, timeout);
    }

    // ==================== LLM API ====================

    /**
     * Create an LLM interaction
     * 
     * @param model LLM model name (e.g., "gpt-4o", "llama-3.1")
     * @param messages List of conversation messages
     * @param reasoningSteps Optional reasoning steps
     * @param metadata Optional metadata
     * @return LlmInteractionResult with id and success status
     * @throws IOException If the request fails
     * @throws InterruptedException If the request is interrupted
     */
    public com.themisdb.client.llm.LlmInteractionResult llmInteraction(
            String model,
            List<com.themisdb.client.llm.LlmMessage> messages,
            List<com.themisdb.client.llm.ReasoningStep> reasoningSteps,
            Map<String, Object> metadata) throws IOException, InterruptedException {
        
        String url = String.format("%s/llm/interaction", getCurrentEndpoint());
        JsonObject requestBody = new JsonObject();
        requestBody.addProperty("model", model);
        requestBody.add("messages", gson.toJsonTree(messages));
        
        if (reasoningSteps != null) {
            requestBody.add("reasoning_steps", gson.toJsonTree(reasoningSteps));
        }
        if (metadata != null) {
            requestBody.add("metadata", gson.toJsonTree(metadata));
        }
        
        HttpRequest request = HttpRequest.newBuilder()
                .uri(URI.create(url))
                .timeout(timeout)
                .header("Content-Type", "application/json")
                .POST(HttpRequest.BodyPublishers.ofString(gson.toJson(requestBody)))
                .build();

        HttpResponse<String> response = httpClient.send(request, HttpResponse.BodyHandlers.ofString());
        
        if (response.statusCode() != 200 && response.statusCode() != 201) {
            throw new IOException("LLM interaction failed with status: " + response.statusCode());
        }

        return gson.fromJson(response.body(), com.themisdb.client.llm.LlmInteractionResult.class);
    }

    /**
     * Get a specific LLM interaction by ID
     * 
     * @param interactionId The interaction ID
     * @return LlmInteraction object or null if not found
     * @throws IOException If the request fails
     * @throws InterruptedException If the request is interrupted
     */
    public com.themisdb.client.llm.LlmInteraction getLlmInteraction(String interactionId) 
            throws IOException, InterruptedException {
        
        String url = String.format("%s/llm/interaction/%s", getCurrentEndpoint(), interactionId);
        HttpRequest request = HttpRequest.newBuilder()
                .uri(URI.create(url))
                .timeout(timeout)
                .GET()
                .build();

        HttpResponse<String> response = httpClient.send(request, HttpResponse.BodyHandlers.ofString());
        
        if (response.statusCode() == 404) {
            return null;
        }
        
        if (response.statusCode() != 200) {
            throw new IOException("GET LLM interaction failed with status: " + response.statusCode());
        }

        return gson.fromJson(response.body(), com.themisdb.client.llm.LlmInteraction.class);
    }

    /**
     * List LLM interactions with optional filtering
     * 
     * @param model Optional model name filter
     * @param limit Maximum number of results
     * @param offset Result offset for pagination
     * @return List of LlmInteraction objects
     * @throws IOException If the request fails
     * @throws InterruptedException If the request is interrupted
     */
    public List<com.themisdb.client.llm.LlmInteraction> listLlmInteractions(
            String model, Integer limit, Integer offset) throws IOException, InterruptedException {
        
        StringBuilder urlBuilder = new StringBuilder(String.format("%s/llm/interaction", getCurrentEndpoint()));
        List<String> params = new ArrayList<>();
        
        if (model != null && !model.isEmpty()) {
            params.add("model=" + model);
        }
        if (limit != null) {
            params.add("limit=" + limit);
        }
        if (offset != null) {
            params.add("offset=" + offset);
        }
        
        if (!params.isEmpty()) {
            urlBuilder.append("?").append(String.join("&", params));
        }
        
        HttpRequest request = HttpRequest.newBuilder()
                .uri(URI.create(urlBuilder.toString()))
                .timeout(timeout)
                .GET()
                .build();

        HttpResponse<String> response = httpClient.send(request, HttpResponse.BodyHandlers.ofString());
        
        if (response.statusCode() != 200) {
            throw new IOException("LIST LLM interactions failed with status: " + response.statusCode());
        }

        JsonObject responseBody = gson.fromJson(response.body(), JsonObject.class);
        com.themisdb.client.llm.LlmInteraction[] interactions = gson.fromJson(
            responseBody.get("interactions"),
            com.themisdb.client.llm.LlmInteraction[].class
        );
        
        return interactions != null ? java.util.Arrays.asList(interactions) : new ArrayList<>();
    }

    // ==================== Helper Methods ====================

    /**
     * Get the HTTP client
     * 
     * @return HttpClient instance
     */
    HttpClient getHttpClient() {
        return httpClient;
    }

    /**
     * Get the Gson instance
     * 
     * @return Gson instance
     */
    Gson getGson() {
        return gson;
    }

    /**
     * Get the timeout duration
     * 
     * @return Timeout duration
     */
    Duration getTimeout() {
        return timeout;
    }

    // ==================== Graph API ====================

    /**
     * Traverse a graph starting from a node
     * 
     * @param startNode Starting node identifier
     * @param maxDepth Maximum traversal depth (default 3)
     * @param edgeType Optional edge type filter
     * @return GraphTraverseResult containing nodes and visited list
     * @throws IOException If the request fails
     */
    public GraphTraverseResult graphTraverse(String startNode, int maxDepth, String edgeType) throws IOException, InterruptedException {
        String url = String.format("%s/graph/traverse", getCurrentEndpoint());
        JsonObject requestBody = new JsonObject();
        requestBody.addProperty("start", startNode);
        requestBody.addProperty("max_depth", maxDepth);
        if (edgeType != null && !edgeType.isEmpty()) {
            requestBody.addProperty("edge_type", edgeType);
        }
        
        HttpRequest request = HttpRequest.newBuilder()
                .uri(URI.create(url))
                .timeout(timeout)
                .header("Content-Type", "application/json")
                .POST(HttpRequest.BodyPublishers.ofString(gson.toJson(requestBody)))
                .build();

        HttpResponse<String> response = httpClient.send(request, HttpResponse.BodyHandlers.ofString());
        
        if (response.statusCode() != 200) {
            throw new IOException("Graph traverse failed with status: " + response.statusCode());
        }

        return gson.fromJson(response.body(), GraphTraverseResult.class);
    }

    /**
     * Traverse a graph starting from a node with default depth
     */
    public GraphTraverseResult graphTraverse(String startNode) throws IOException, InterruptedException {
        return graphTraverse(startNode, 3, null);
    }

    /**
     * Find the shortest path between two nodes
     * 
     * @param from Source node identifier
     * @param to Target node identifier
     * @param edgeType Optional edge type filter
     * @return List of node IDs in the path
     * @throws IOException If the request fails
     */
    public List<String> shortestPath(String from, String to, String edgeType) throws IOException, InterruptedException {
        String url = String.format("%s/graph/shortest-path", getCurrentEndpoint());
        JsonObject requestBody = new JsonObject();
        requestBody.addProperty("from", from);
        requestBody.addProperty("to", to);
        if (edgeType != null && !edgeType.isEmpty()) {
            requestBody.addProperty("edge_type", edgeType);
        }
        
        HttpRequest request = HttpRequest.newBuilder()
                .uri(URI.create(url))
                .timeout(timeout)
                .header("Content-Type", "application/json")
                .POST(HttpRequest.BodyPublishers.ofString(gson.toJson(requestBody)))
                .build();

        HttpResponse<String> response = httpClient.send(request, HttpResponse.BodyHandlers.ofString());
        
        if (response.statusCode() != 200) {
            throw new IOException("Shortest path query failed with status: " + response.statusCode());
        }

        ShortestPathResult result = gson.fromJson(response.body(), ShortestPathResult.class);
        return result != null ? result.getPath() : List.of();
    }

    /**
     * Find the shortest path between two nodes
     */
    public List<String> shortestPath(String from, String to) throws IOException, InterruptedException {
        return shortestPath(from, to, null);
    }

    /**
     * Get neighbors of a node
     * 
     * @param nodeId Node identifier
     * @param direction Direction filter ("in", "out", "both")
     * @param edgeType Optional edge type filter
     * @param limit Maximum number of neighbors to return
     * @return List of neighbor node IDs
     * @throws IOException If the request fails
     */
    public List<String> neighbors(String nodeId, String direction, String edgeType, Integer limit) throws IOException, InterruptedException {
        String url = String.format("%s/graph/neighbors", getCurrentEndpoint());
        JsonObject requestBody = new JsonObject();
        requestBody.addProperty("node", nodeId);
        if (direction != null && !direction.isEmpty()) {
            requestBody.addProperty("direction", direction);
        }
        if (edgeType != null && !edgeType.isEmpty()) {
            requestBody.addProperty("edge_type", edgeType);
        }
        if (limit != null) {
            requestBody.addProperty("limit", limit);
        }
        
        HttpRequest request = HttpRequest.newBuilder()
                .uri(URI.create(url))
                .timeout(timeout)
                .header("Content-Type", "application/json")
                .POST(HttpRequest.BodyPublishers.ofString(gson.toJson(requestBody)))
                .build();

        HttpResponse<String> response = httpClient.send(request, HttpResponse.BodyHandlers.ofString());
        
        if (response.statusCode() != 200) {
            throw new IOException("Neighbors query failed with status: " + response.statusCode());
        }

        NeighborsResult result = gson.fromJson(response.body(), NeighborsResult.class);
        return result != null ? result.getNeighbors() : List.of();
    }

    /**
     * Get neighbors of a node with defaults
     */
    public List<String> neighbors(String nodeId) throws IOException, InterruptedException {
        return neighbors(nodeId, null, null, null);
    }

    // ==================== Vector API ====================

    /**
     * Perform a vector similarity search
     * 
     * @param embedding The query vector
     * @param topK Number of results to return
     * @param filter Optional metadata filter
     * @return VectorSearchResult containing matched vectors
     * @throws IOException If the request fails
     */
    public VectorSearchResult vectorSearch(double[] embedding, int topK, JsonObject filter) throws IOException, InterruptedException {
        String url = String.format("%s/vector/search", getCurrentEndpoint());
        JsonObject requestBody = new JsonObject();
        requestBody.add("vector", gson.toJsonTree(embedding));
        requestBody.addProperty("k", topK);
        if (filter != null) {
            requestBody.add("filter", filter);
        }
        
        HttpRequest request = HttpRequest.newBuilder()
                .uri(URI.create(url))
                .timeout(timeout)
                .header("Content-Type", "application/json")
                .POST(HttpRequest.BodyPublishers.ofString(gson.toJson(requestBody)))
                .build();

        HttpResponse<String> response = httpClient.send(request, HttpResponse.BodyHandlers.ofString());
        
        if (response.statusCode() != 200) {
            throw new IOException("Vector search failed with status: " + response.statusCode());
        }

        return gson.fromJson(response.body(), VectorSearchResult.class);
    }

    /**
     * Perform a vector similarity search with defaults
     */
    public VectorSearchResult vectorSearch(double[] embedding, int topK) throws IOException, InterruptedException {
        return vectorSearch(embedding, topK, null);
    }

    /**
     * Upsert a vector with metadata
     * 
     * @param id Vector identifier
     * @param embedding The vector data
     * @param metadata Optional metadata
     * @throws IOException If the request fails
     */
    public void vectorUpsert(String id, double[] embedding, JsonObject metadata) throws IOException, InterruptedException {
        String url = String.format("%s/vector/upsert", getCurrentEndpoint());
        JsonObject requestBody = new JsonObject();
        requestBody.addProperty("id", id);
        requestBody.add("vector", gson.toJsonTree(embedding));
        if (metadata != null) {
            requestBody.add("metadata", metadata);
        }
        
        HttpRequest request = HttpRequest.newBuilder()
                .uri(URI.create(url))
                .timeout(timeout)
                .header("Content-Type", "application/json")
                .POST(HttpRequest.BodyPublishers.ofString(gson.toJson(requestBody)))
                .build();

        HttpResponse<String> response = httpClient.send(request, HttpResponse.BodyHandlers.ofString());
        
        if (response.statusCode() != 200 && response.statusCode() != 201) {
            throw new IOException("Vector upsert failed with status: " + response.statusCode());
        }
    }

    /**
     * Delete a vector by ID
     * 
     * @param id Vector identifier
     * @throws IOException If the request fails
     */
    public void vectorDelete(String id) throws IOException, InterruptedException {
        String url = String.format("%s/vector/%s", getCurrentEndpoint(), id);
        
        HttpRequest request = HttpRequest.newBuilder()
                .uri(URI.create(url))
                .timeout(timeout)
                .DELETE()
                .build();

        HttpResponse<String> response = httpClient.send(request, HttpResponse.BodyHandlers.ofString());
        
        if (response.statusCode() != 200 && response.statusCode() != 204) {
            throw new IOException("Vector delete failed with status: " + response.statusCode());
        }
    }

    // ==================== Supporting Classes ====================

    /**
     * Result of a graph traversal
     */
    public static class GraphTraverseResult {
        private List<String> nodes = new ArrayList<>();
        private List<String> visited = new ArrayList<>();
        
        public List<String> getNodes() { return nodes; }
        public List<String> getVisited() { return visited; }
    }

    /**
     * Result of a shortest path query
     */
    private static class ShortestPathResult {
        private List<String> path = new ArrayList<>();
        public List<String> getPath() { return path; }
    }

    /**
     * Result of a neighbors query
     */
    private static class NeighborsResult {
        private List<String> neighbors = new ArrayList<>();
        public List<String> getNeighbors() { return neighbors; }
    }

    /**
     * Result of a vector search
     */
    public static class VectorSearchResult {
        private List<VectorSearchHit> results = new ArrayList<>();
        public List<VectorSearchHit> getResults() { return results; }
    }

    /**
     * A single hit from a vector search
     */
    public static class VectorSearchHit {
        private String id;
        private double score;
        private Double distance;
        private JsonObject metadata;
        
        public String getId() { return id; }
        public double getScore() { return score; }
        public Double getDistance() { return distance; }
        public JsonObject getMetadata() { return metadata; }
    }
    
    /**
     * Helper method to send HTTP requests with retry and circuit breaker support
     */
    private HttpResponse<String> sendRequestWithRetry(HttpRequest request) throws IOException, InterruptedException {
        // Check circuit breaker
        if (circuitBreaker != null && !circuitBreaker.canExecute()) {
            if (loggingEnabled) {
                logger.log("Circuit breaker is OPEN, request blocked: " + request.uri(), 
                    ClientConfig.Logger.Level.WARN);
            }
            throw new IOException("Circuit breaker is OPEN");
        }
        
        IOException lastException = null;
        for (int attempt = 0; attempt < maxRetries; attempt++) {
            try {
                if (attempt > 0) {
                    // Exponential backoff
                    long backoffMs = (long) (Math.pow(2, attempt - 1) * 100);
                    if (loggingEnabled) {
                        logger.log(String.format("Retry attempt %d after %dms", attempt, backoffMs),
                            ClientConfig.Logger.Level.INFO);
                    }
                    Thread.sleep(backoffMs);
                }
                
                // Log request if enabled
                if (loggingEnabled && logRequests) {
                    logger.log("Request: " + request.method() + " " + request.uri(),
                        ClientConfig.Logger.Level.INFO);
                }
                
                HttpResponse<String> response = httpClient.send(request, HttpResponse.BodyHandlers.ofString());
                
                // Log response if enabled
                if (loggingEnabled && logResponses) {
                    logger.log("Response: " + request.method() + " " + request.uri() + " - Status: " + response.statusCode(),
                        ClientConfig.Logger.Level.INFO);
                }
                
                // Retry on 5xx errors
                if (response.statusCode() >= 500 && attempt + 1 < maxRetries) {
                    lastException = new IOException("Server error: " + response.statusCode());
                    if (loggingEnabled) {
                        logger.log("Server error " + response.statusCode() + ", will retry",
                            ClientConfig.Logger.Level.WARN);
                    }
                    if (circuitBreaker != null) {
                        circuitBreaker.recordFailure();
                    }
                    continue;
                }
                
                // Record success/failure for circuit breaker
                if (circuitBreaker != null) {
                    if (response.statusCode() < 400) {
                        circuitBreaker.recordSuccess();
                    } else {
                        circuitBreaker.recordFailure();
                    }
                }
                
                return response;
                
            } catch (IOException | InterruptedException e) {
                lastException = new IOException(e);
                if (loggingEnabled) {
                    logger.log("Request error: " + e.getMessage(), ClientConfig.Logger.Level.ERROR);
                }
                if (circuitBreaker != null) {
                    circuitBreaker.recordFailure();
                }
                if (attempt + 1 >= maxRetries) {
                    throw lastException;
                }
            }
        }
        
        // All retries exhausted
        if (circuitBreaker != null) {
            circuitBreaker.recordFailure();
        }
        throw lastException != null ? lastException : new IOException("Request failed after " + maxRetries + " attempts");
    }
    
    /**
     * Get the current circuit breaker state
     * 
     * @return Circuit breaker state or null if disabled
     */
    public String getCircuitBreakerState() {
        return circuitBreaker != null ? circuitBreaker.getState().name() : null;
    }
}
