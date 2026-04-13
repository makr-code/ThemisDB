/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ThemisClient.php                                   ║
  Version:         0.0.38                                             ║
  Last Modified:   2026-04-13 04:12:38                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     1250                                           ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 65b6fc41ed  2026-02-24  fix: resolve remaining Python (34) and PHP (23) error-han... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

<?php

namespace ThemisDB;

use Exception;
use RuntimeException;

/**
 * ThemisDB PHP Client - Official PHP SDK for ThemisDB
 * 
 * A high-performance multi-model database client with topology-aware routing,
 * ACID transactions, and support for relational, graph, vector, and document models.
 * 
 * @package ThemisDB
 * @version 1.0.0
 * @license MIT
 */
class ThemisClient
{
    private const DEFAULT_METADATA_PATH = '/_admin/cluster/topology';
    private const HEALTH_PATH = '/health';
    private const VERSION = '1.0.0';
    
    // Circuit breaker states
    private const CIRCUIT_CLOSED = 'closed';
    private const CIRCUIT_OPEN = 'open';
    private const CIRCUIT_HALF_OPEN = 'half_open';

    private array $endpoints;
    private string $namespace;
    private float $timeout;
    private int $maxRetries;
    private ?string $metadataEndpoint;
    private string $metadataPath;
    private ?array $topologyCache = null;
    private array $shardEndpoints;
    
    // Circuit breaker state
    private bool $circuitBreakerEnabled = false;
    private int $circuitBreakerFailureThreshold = 5;
    private int $circuitBreakerResetTimeout = 60;
    private int $circuitBreakerHalfOpenMax = 3;
    private string $circuitBreakerState = self::CIRCUIT_CLOSED;
    private int $circuitBreakerFailureCount = 0;
    private int $circuitBreakerSuccessCount = 0;
    private ?float $circuitBreakerNextAttemptTime = null;
    
    // Logging state
    private bool $loggingEnabled = false;
    private bool $logRequests = false;
    private bool $logResponses = false;
    
    /**
     * Create a new ThemisDB client instance.
     *
     * @param array $endpoints List of ThemisDB server endpoints (e.g., ['http://localhost:8080'])
     * @param array $options Configuration options:
     *   - namespace: Namespace for entities (default: 'default')
     *   - timeout: Request timeout in seconds (default: 30.0)
     *   - max_retries: Maximum retry attempts (default: 3)
     *   - metadata_endpoint: Custom metadata endpoint (optional)
     *   - metadata_path: Metadata path (default: '/_admin/cluster/topology')
     *   - circuit_breaker: Circuit breaker configuration (optional):
     *     - enabled: Enable circuit breaker (default: false)
     *     - failure_threshold: Failures before opening (default: 5)
     *     - reset_timeout: Seconds before retry (default: 60)
     *     - half_open_max_requests: Max requests in half-open state (default: 3)
     *   - logging: Logging configuration (optional):
     *     - enabled: Enable logging (default: false)
     *     - log_requests: Log HTTP requests (default: false)
     *     - log_responses: Log HTTP responses (default: false)
     * 
     * @throws \InvalidArgumentException If endpoints is empty
     */
    public function __construct(array $endpoints, array $options = [])
    {
        if (empty($endpoints)) {
            throw new \InvalidArgumentException('endpoints must not be empty');
        }

        $this->endpoints = array_map(fn($e) => rtrim($e, '/'), $endpoints);
        $this->namespace = $options['namespace'] ?? 'default';
        $this->timeout = $options['timeout'] ?? 30.0;
        $this->maxRetries = max(1, $options['max_retries'] ?? 3);
        $this->metadataEndpoint = $options['metadata_endpoint'] ?? null;
        $this->metadataPath = $options['metadata_path'] ?? self::DEFAULT_METADATA_PATH;
        $this->shardEndpoints = $this->endpoints;
        
        // Circuit breaker configuration
        if (isset($options['circuit_breaker']) && is_array($options['circuit_breaker'])) {
            $cb = $options['circuit_breaker'];
            $this->circuitBreakerEnabled = $cb['enabled'] ?? false;
            $this->circuitBreakerFailureThreshold = $cb['failure_threshold'] ?? 5;
            $this->circuitBreakerResetTimeout = $cb['reset_timeout'] ?? 60;
            $this->circuitBreakerHalfOpenMax = $cb['half_open_max_requests'] ?? 3;
        }
        
        // Logging configuration
        if (isset($options['logging']) && is_array($options['logging'])) {
            $log = $options['logging'];
            $this->loggingEnabled = $log['enabled'] ?? false;
            $this->logRequests = $log['log_requests'] ?? false;
            $this->logResponses = $log['log_responses'] ?? false;
        }
    }
    
    /**
     * Get the current circuit breaker state.
     *
     * @return string|null Circuit breaker state ('closed', 'open', 'half_open') or null if disabled
     */
    public function getCircuitBreakerState(): ?string
    {
        return $this->circuitBreakerEnabled ? $this->circuitBreakerState : null;
    }

    /**
     * Check server health.
     *
     * @param string|null $endpoint Specific endpoint to check (uses first endpoint if null)
     * @return array Health status response
     * @throws RuntimeException If health check fails
     */
    public function health(?string $endpoint = null): array
    {
        $target = $endpoint ? rtrim($endpoint, '/') : $this->endpoints[0];
        $response = $this->request('GET', $target . self::HEALTH_PATH);
        return $response;
    }

    /**
     * Get an entity by model, collection, and UUID.
     *
     * @param string $model Model name (e.g., 'relational', 'graph', 'vector')
     * @param string $collection Collection name
     * @param string $uuid Entity UUID
     * @return mixed Entity data or null if not found
     * @throws RuntimeException If request fails
     */
    public function get(string $model, string $collection, string $uuid)
    {
        $urn = $this->buildUrn($model, $collection, $uuid);
        $key = $this->buildEntityKey($model, $collection, $uuid);
        $endpoint = $this->resolveEndpoint($urn);
        
        try {
            $response = $this->request('GET', "{$endpoint}/entities/{$key}");
            
            // Handle decrypt=true payload
            if (isset($response['entity'])) {
                return $response['entity'];
            }
            
            // Decode blob
            if (isset($response['blob'])) {
                return $this->decodeBlob($response['blob']);
            }
            
            return $response;
        } catch (NotFoundException $e) {
            error_log("Entity not found: " . $e->getMessage());
            return null;
        }
    }

    /**
     * Create or update an entity.
     *
     * @param string $model Model name
     * @param string $collection Collection name
     * @param string $uuid Entity UUID
     * @param mixed $data Entity data (will be JSON encoded)
     * @return bool True on success
     * @throws RuntimeException If request fails
     */
    public function put(string $model, string $collection, string $uuid, $data): bool
    {
        $urn = $this->buildUrn($model, $collection, $uuid);
        $key = $this->buildEntityKey($model, $collection, $uuid);
        $endpoint = $this->resolveEndpoint($urn);
        
        $body = ['blob' => $this->encodeBlob($data)];
        $this->request('PUT', "{$endpoint}/entities/{$key}", $body);
        
        return true;
    }

    /**
     * Delete an entity.
     *
     * @param string $model Model name
     * @param string $collection Collection name
     * @param string $uuid Entity UUID
     * @return bool True if entity was deleted, false if not found
     * @throws RuntimeException If request fails
     */
    public function delete(string $model, string $collection, string $uuid): bool
    {
        $urn = $this->buildUrn($model, $collection, $uuid);
        $key = $this->buildEntityKey($model, $collection, $uuid);
        $endpoint = $this->resolveEndpoint($urn);
        
        try {
            $this->request('DELETE', "{$endpoint}/entities/{$key}");
            return true;
        } catch (NotFoundException $e) {
            error_log("Entity not found during delete: " . $e->getMessage());
            return false;
        }
    }

    /**
     * Batch get multiple entities.
     *
     * @param string $model Model name
     * @param string $collection Collection name
     * @param array $uuids Array of UUIDs to fetch
     * @return array Result with 'found', 'missing', and 'errors' keys
     */
    public function batchGet(string $model, string $collection, array $uuids): array
    {
        $result = [
            'found' => [],
            'missing' => [],
            'errors' => []
        ];

        if (empty($uuids)) {
            return $result;
        }

        foreach ($uuids as $uuid) {
            try {
                $entity = $this->get($model, $collection, $uuid);
                if ($entity === null) {
                    $result['missing'][] = $uuid;
                } else {
                    $result['found'][$uuid] = $entity;
                }
            } catch (Exception $e) {
                error_log("Batch get error for {$uuid}: " . $e->getMessage());
                $result['errors'][$uuid] = $e->getMessage();
            }
        }

        return $result;
    }

    /**
     * Batch put multiple entities.
     *
     * @param string $model Model name
     * @param string $collection Collection name
     * @param array $items Associative array of uuid => data
     * @return array Result with 'succeeded' and 'failed' keys
     */
    public function batchPut(string $model, string $collection, array $items): array
    {
        $result = [
            'succeeded' => [],
            'failed' => []
        ];

        if (empty($items)) {
            return $result;
        }

        foreach ($items as $uuid => $data) {
            try {
                if ($this->put($model, $collection, $uuid, $data)) {
                    $result['succeeded'][] = $uuid;
                } else {
                    $result['failed'][$uuid] = 'operation returned false';
                }
            } catch (Exception $e) {
                error_log("Batch put error for {$uuid}: " . $e->getMessage());
                $result['failed'][$uuid] = $e->getMessage();
            }
        }

        return $result;
    }

    /**
     * Batch delete multiple entities.
     *
     * @param string $model Model name
     * @param string $collection Collection name
     * @param array $uuids Array of UUIDs to delete
     * @return array Result with 'succeeded' and 'failed' keys
     */
    public function batchDelete(string $model, string $collection, array $uuids): array
    {
        $result = [
            'succeeded' => [],
            'failed' => []
        ];

        if (empty($uuids)) {
            return $result;
        }

        foreach ($uuids as $uuid) {
            try {
                if ($this->delete($model, $collection, $uuid)) {
                    $result['succeeded'][] = $uuid;
                } else {
                    $result['failed'][$uuid] = 'operation returned false';
                }
            } catch (Exception $e) {
                error_log("Batch delete error for {$uuid}: " . $e->getMessage());
                $result['failed'][$uuid] = $e->getMessage();
            }
        }

        return $result;
    }

    /**
     * Execute an AQL (Advanced Query Language) query.
     *
     * @param string $aql AQL query string
     * @param array $options Query options:
     *   - params: Query parameters (optional)
     *   - use_cursor: Enable cursor-based pagination (default: false)
     *   - cursor: Cursor for pagination (optional)
     *   - batch_size: Batch size for results (optional)
     * @return array Query result with 'items', 'has_more', 'next_cursor', and 'raw' keys
     * @throws RuntimeException If query fails
     */
    public function query(string $aql, array $options = []): array
    {
        $payload = ['query' => $aql];
        
        if (isset($options['params'])) {
            $payload['params'] = $options['params'];
        }
        if (isset($options['use_cursor']) && $options['use_cursor']) {
            $payload['use_cursor'] = true;
        }
        if (isset($options['cursor'])) {
            $payload['cursor'] = $options['cursor'];
        }
        if (isset($options['batch_size'])) {
            $payload['batch_size'] = $options['batch_size'];
        }

        $endpoints = $this->isSingleShardQuery($aql) 
            ? [$this->resolveQueryEndpoint($aql)]
            : $this->currentEndpoints();

        $partials = [];
        foreach ($endpoints as $endpoint) {
            $response = $this->request('POST', "{$endpoint}/query/aql", $payload);
            $partials[] = $this->parseQueryPayload($response);
        }

        if (empty($partials)) {
            return [
                'items' => [],
                'has_more' => false,
                'next_cursor' => null,
                'raw' => []
            ];
        }

        if (count($partials) === 1) {
            return $partials[0];
        }

        // Merge results from multiple shards
        $mergedItems = [];
        $anyHasMore = false;
        foreach ($partials as $part) {
            $mergedItems = array_merge($mergedItems, $part['items']);
            $anyHasMore = $anyHasMore || $part['has_more'];
        }

        return [
            'items' => $mergedItems,
            'has_more' => $anyHasMore,
            'next_cursor' => null,
            'raw' => ['partials' => array_map(fn($p) => $p['raw'], $partials)]
        ];
    }

    /**
     * Traverse a graph starting from a node.
     *
     * @param string $startNode Starting node URN or key
     * @param int $maxDepth Maximum traversal depth (default: 3)
     * @param string|null $edgeType Optional edge type filter
     * @return array Array of visited node keys
     * @throws RuntimeException If traversal fails
     */
    public function graphTraverse(string $startNode, int $maxDepth = 3, ?string $edgeType = null): array
    {
        $endpoint = $this->resolveEndpoint($startNode);
        $payload = [
            'start' => $startNode,
            'max_depth' => $maxDepth
        ];
        
        if ($edgeType !== null) {
            $payload['edge_type'] = $edgeType;
        }

        $response = $this->request('POST', "{$endpoint}/graph/traverse", $payload);
        
        // Support different response formats
        if (isset($response['nodes'])) {
            return $response['nodes'];
        }
        if (isset($response['visited'])) {
            return $response['visited'];
        }
        
        return [];
    }

    /**
     * Find shortest path between two nodes.
     *
     * @param string $startNode Starting node URN or key
     * @param string $endNode End node URN or key
     * @param string|null $edgeType Optional edge type filter
     * @return array|null Array of nodes in path, or null if no path found
     * @throws RuntimeException If request fails
     */
    public function graphShortestPath(string $startNode, string $endNode, ?string $edgeType = null): ?array
    {
        $endpoint = $this->resolveEndpoint($startNode);
        $payload = [
            'start' => $startNode,
            'end' => $endNode
        ];
        
        if ($edgeType !== null) {
            $payload['edge_type'] = $edgeType;
        }

        $response = $this->request('POST', "{$endpoint}/graph/shortest-path", $payload);
        
        if (isset($response['path'])) {
            return $response['path'];
        }
        
        return null;
    }

    /**
     * Get neighbors of a node.
     *
     * @param string $node Node URN or key
     * @param string|null $edgeType Optional edge type filter
     * @param string $direction Direction: 'in', 'out', or 'both' (default: 'both')
     * @return array Array of neighbor node keys
     * @throws RuntimeException If request fails
     */
    public function graphNeighbors(string $node, ?string $edgeType = null, string $direction = 'both'): array
    {
        $endpoint = $this->resolveEndpoint($node);
        $payload = [
            'node' => $node,
            'direction' => $direction
        ];
        
        if ($edgeType !== null) {
            $payload['edge_type'] = $edgeType;
        }

        $response = $this->request('POST', "{$endpoint}/graph/neighbors", $payload);
        
        if (isset($response['neighbors'])) {
            return $response['neighbors'];
        }
        
        return [];
    }

    /**
     * Search for similar vectors.
     *
     * @param array $embedding Query vector embedding (array of floats)
     * @param int $topK Number of results to return (default: 10)
     * @param array|null $metadataFilter Optional metadata filter
     * @param array $options Additional options:
     *   - use_cursor: Enable cursor-based pagination (default: false)
     *   - cursor: Cursor for pagination (optional)
     * @return array Search results with 'results' key
     * @throws RuntimeException If search fails
     */
    public function vectorSearch(array $embedding, int $topK = 10, ?array $metadataFilter = null, array $options = []): array
    {
        $payload = [
            'vector' => $embedding,
            'k' => $topK
        ];
        
        if ($metadataFilter !== null) {
            $payload['filter'] = $metadataFilter;
        }
        if (isset($options['use_cursor']) && $options['use_cursor']) {
            $payload['use_cursor'] = true;
        }
        if (isset($options['cursor'])) {
            $payload['cursor'] = $options['cursor'];
        }

        $responses = [];
        foreach ($this->currentEndpoints() as $endpoint) {
            try {
                $response = $this->request('POST', "{$endpoint}/vector/search", $payload);
                $responses[] = $response;
            } catch (Exception $e) {
                error_log("Vector search failed on endpoint: " . $e->getMessage());
                // Continue with other endpoints
            }
        }

        if (empty($responses)) {
            return ['results' => []];
        }

        if (count($responses) === 1) {
            return $responses[0];
        }

        # Merge and sort results from multiple shards
        $mergedHits = [];
        foreach ($responses as $response) {
            // Support both 'results' (vector search) and 'items' (cursor-based) formats
            if (isset($response['results'])) {
                $mergedHits = array_merge($mergedHits, $response['results']);
            } elseif (isset($response['items'])) {
                $mergedHits = array_merge($mergedHits, $response['items']);
            }
        }

        // Sort by score (descending) or distance (ascending)
        usort($mergedHits, function($a, $b) {
            $scoreA = $a['score'] ?? $a['distance'] ?? 0;
            $scoreB = $b['score'] ?? $b['distance'] ?? 0;
            
            // If using score, higher is better; if using distance, lower is better
            if (isset($a['score'])) {
                return $scoreB <=> $scoreA;
            } else {
                return $scoreA <=> $scoreB;
            }
        });

        return [
            'results' => array_slice($mergedHits, 0, $topK),
            'partials' => $responses
        ];
    }

    /**
     * Upsert a vector into the vector index.
     *
     * @param string $id Vector ID
     * @param array $embedding Vector embedding (array of floats)
     * @param array|null $metadata Optional metadata
     * @return bool True on success
     * @throws RuntimeException If upsert fails
     */
    public function vectorUpsert(string $id, array $embedding, ?array $metadata = null): bool
    {
        $endpoint = $this->resolveEndpoint($id);
        $payload = [
            'id' => $id,
            'vector' => $embedding
        ];
        
        if ($metadata !== null) {
            $payload['metadata'] = $metadata;
        }

        $this->request('POST', "{$endpoint}/vector/upsert", $payload);
        return true;
    }

    /**
     * Delete a vector from the vector index.
     *
     * @param string $id Vector ID
     * @return bool True if deleted, false if not found
     * @throws RuntimeException If request fails
     */
    public function vectorDelete(string $id): bool
    {
        $endpoint = $this->resolveEndpoint($id);
        
        try {
            $this->request('DELETE', "{$endpoint}/vector/{$id}");
            return true;
        } catch (NotFoundException $e) {
            error_log("Vector not found during delete: " . $e->getMessage());
            return false;
        }
    }

    /**
     * Begin a new ACID transaction.
     *
     * @param array $options Transaction options:
     *   - isolation_level: 'READ_COMMITTED' or 'SNAPSHOT' (default: 'READ_COMMITTED')
     *   - timeout: Transaction timeout in seconds (optional)
     * @return Transaction Transaction object
     * @throws TransactionException If transaction cannot be started
     */
    public function beginTransaction(array $options = []): Transaction
    {
        $endpoint = $this->endpoints[0];
        $isolationLevel = $options['isolation_level'] ?? 'READ_COMMITTED';
        
        $body = [];
        if ($isolationLevel === 'SNAPSHOT') {
            $body['isolation'] = 'snapshot';
        } elseif ($isolationLevel === 'READ_COMMITTED') {
            $body['isolation'] = 'read_committed';
        } else {
            throw new \InvalidArgumentException("Invalid isolation level: {$isolationLevel}");
        }
        
        if (isset($options['timeout'])) {
            $body['timeout'] = $options['timeout'];
        }

        $response = $this->request('POST', "{$endpoint}/transaction/begin", $body);
        
        if (!isset($response['transaction_id'])) {
            throw new TransactionException('Server did not return transaction_id');
        }

        return new Transaction($this, $response['transaction_id']);
    }

    // ==================== LLM API ====================

    /**
     * Create an LLM interaction.
     *
     * @param string $model LLM model name (e.g., 'gpt-4o', 'llama-3.1')
     * @param array $messages List of LlmMessage objects or arrays
     * @param array|null $reasoningSteps Optional reasoning steps
     * @param array|null $metadata Optional metadata
     * @return Llm\LlmInteractionResult
     * @throws RuntimeException If request fails
     */
    public function llmInteraction(
        string $model,
        array $messages,
        ?array $reasoningSteps = null,
        ?array $metadata = null
    ): Llm\LlmInteractionResult {
        $endpoint = $this->endpoints[0];
        
        $body = [
            'model' => $model,
            'messages' => array_map(function($msg) {
                return $msg instanceof Llm\LlmMessage ? $msg->toArray() : $msg;
            }, $messages),
        ];
        
        if ($reasoningSteps !== null) {
            $body['reasoning_steps'] = array_map(function($step) {
                return $step instanceof Llm\ReasoningStep ? $step->toArray() : $step;
            }, $reasoningSteps);
        }
        
        if ($metadata !== null) {
            $body['metadata'] = $metadata;
        }
        
        $response = $this->request('POST', "{$endpoint}/llm/interaction", $body);
        
        return Llm\LlmInteractionResult::fromArray($response);
    }

    /**
     * Get a specific LLM interaction by ID.
     *
     * @param string $interactionId The interaction ID
     * @return Llm\LlmInteraction|null
     * @throws RuntimeException If request fails
     */
    public function getLlmInteraction(string $interactionId): ?Llm\LlmInteraction
    {
        $endpoint = $this->endpoints[0];
        
        try {
            $response = $this->request('GET', "{$endpoint}/llm/interaction/{$interactionId}");
            return Llm\LlmInteraction::fromArray($response);
        } catch (NotFoundException $e) {
            error_log("LLM interaction not found: " . $e->getMessage());
            return null;
        }    }

    /**
     * List LLM interactions with optional filtering.
     *
     * @param string|null $model Optional model name filter
     * @param int|null $limit Maximum number of results
     * @param int|null $offset Result offset for pagination
     * @return array List of LlmInteraction objects
     * @throws RuntimeException If request fails
     */
    public function listLlmInteractions(
        ?string $model = null,
        ?int $limit = null,
        ?int $offset = null
    ): array {
        $endpoint = $this->endpoints[0];
        $params = [];
        
        if ($model !== null) {
            $params[] = 'model=' . urlencode($model);
        }
        
        if ($limit !== null) {
            $params[] = 'limit=' . $limit;
        }
        
        if ($offset !== null) {
            $params[] = 'offset=' . $offset;
        }
        
        $url = "{$endpoint}/llm/interaction";
        if (!empty($params)) {
            $url .= '?' . implode('&', $params);
        }
        
        $response = $this->request('GET', $url);
        
        $interactions = [];
        foreach ($response['interactions'] ?? [] as $data) {
            $interactions[] = Llm\LlmInteraction::fromArray($data);
        }
        
        return $interactions;
    }

    /**
     * Make an HTTP request with retry logic.
     *
     * @internal
     */
    public function request(string $method, string $url, ?array $body = null, array $headers = []): array
    {
        // Check circuit breaker
        if ($this->circuitBreakerEnabled && !$this->canExecuteRequest()) {
            $this->log('ERROR', "Circuit breaker is OPEN for {$url}");
            throw new RuntimeException('Circuit breaker is OPEN');
        }
        
        // Log request
        if ($this->logRequests) {
            $this->log('INFO', "{$method} {$url}");
        }
        
        $defaultHeaders = [
            'Content-Type: application/json',
            'User-Agent: themisdb-php-sdk/' . self::VERSION,
            'Accept: application/json'
        ];

        $allHeaders = array_merge($defaultHeaders, $headers);
        $lastError = null;

        for ($attempt = 1; $attempt <= $this->maxRetries; $attempt++) {
            $ch = curl_init($url);
            
            curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
            curl_setopt($ch, CURLOPT_TIMEOUT, (int)$this->timeout);
            curl_setopt($ch, CURLOPT_HTTPHEADER, $allHeaders);
            curl_setopt($ch, CURLOPT_CUSTOMREQUEST, $method);
            
            if ($body !== null) {
                curl_setopt($ch, CURLOPT_POSTFIELDS, json_encode($body));
            }

            $response = curl_exec($ch);
            $statusCode = curl_getinfo($ch, CURLINFO_HTTP_CODE);
            $error = curl_error($ch);
            curl_close($ch);
            
            // Log response
            if ($this->logResponses) {
                $this->log('INFO', "{$method} {$url} -> {$statusCode}");
            }

            if ($error) {
                $this->recordCircuitBreakerFailure();
                $this->log('ERROR', "cURL error: {$error}");
                $lastError = new RuntimeException("cURL error: {$error}");
                if ($attempt < $this->maxRetries) {
                    usleep((int)(pow(2, $attempt) * 100000)); // Exponential backoff
                }
                continue;
            }

            if ($statusCode === 404) {
                $this->recordCircuitBreakerFailure();
                throw new NotFoundException("Resource not found: {$url}");
            }

            if ($statusCode >= 500) {
                $this->recordCircuitBreakerFailure();
                $this->log('WARN', "Server error {$statusCode}, retrying...");
                if ($attempt === $this->maxRetries) {
                    throw new RuntimeException("HTTP {$statusCode}: {$response}");
                }
                $lastError = new RuntimeException("HTTP {$statusCode}: {$response}");
                usleep((int)(pow(2, $attempt) * 100000)); // Exponential backoff
                continue;
            }

            if ($statusCode >= 400) {
                $this->recordCircuitBreakerFailure();
                throw new RuntimeException("HTTP {$statusCode}: {$response}");
            }

            // Success
            $this->recordCircuitBreakerSuccess();

            if ($response === '' || $response === false) {
                return [];
            }

            $decoded = json_decode($response, true);
            if (json_last_error() !== JSON_ERROR_NONE) {
                throw new RuntimeException('Failed to decode JSON response: ' . json_last_error_msg());
            }

            return $decoded;
        }

        if ($lastError !== null) {
            throw $lastError;
        }

        throw new RuntimeException('Request failed without specific error');
    }

    /**
     * Get bootstrap endpoints.
     *
     * @internal
     */
    public function getEndpoints(): array
    {
        return $this->endpoints;
    }

    /**
     * Build URN for an entity.
     *
     * @internal
     */
    public function buildUrn(string $model, string $collection, string $uuid): string
    {
        return "urn:themis:{$model}:{$this->namespace}:{$collection}:{$uuid}";
    }

    /**
     * Build entity key for API calls.
     *
     * @internal
     */
    public function buildEntityKey(string $model, string $collection, string $uuid): string
    {
        $table = "{$model}.{$this->namespace}.{$collection}";
        return "{$table}:{$uuid}";
    }

    /**
     * Resolve endpoint for a URN using consistent hashing.
     *
     * @internal
     */
    public function resolveEndpoint(string $urn): string
    {
        $this->ensureTopology();
        $endpoints = $this->currentEndpoints();
        
        if (empty($endpoints)) {
            throw new TopologyException('No endpoints available for request');
        }

        $hash = $this->stableHash($urn);
        $index = $hash % count($endpoints);
        
        return $endpoints[$index];
    }

    /**
     * Resolve endpoint for a query.
     *
     * @internal
     */
    public function resolveQueryEndpoint(string $aql): string
    {
        $this->ensureTopology();
        $endpoints = $this->currentEndpoints();
        
        if (empty($endpoints)) {
            throw new TopologyException('No endpoints available for query');
        }

        $hash = $this->stableHash($aql);
        $index = $hash % count($endpoints);
        
        return $endpoints[$index];
    }

    /**
     * Get current endpoints (from topology or bootstrap).
     *
     * @internal
     */
    public function currentEndpoints(): array
    {
        return !empty($this->shardEndpoints) ? $this->shardEndpoints : $this->endpoints;
    }

    /**
     * Ensure topology is loaded.
     *
     * @internal
     */
    private function ensureTopology(): void
    {
        if ($this->topologyCache !== null) {
            return;
        }

        try {
            $this->refreshTopology();
        } catch (TopologyException $e) {
            error_log("Topology refresh failed, using bootstrap endpoints: " . $e->getMessage());
            // Fallback to bootstrap endpoints
            $this->shardEndpoints = $this->endpoints;
        }
    }

    /**
     * Refresh topology from metadata endpoint.
     *
     * @internal
     */
    private function refreshTopology(): void
    {
        $url = $this->metadataUrl();
        
        try {
            $payload = $this->request('GET', $url);
        } catch (Exception $e) {
            throw new TopologyException('Failed to fetch shard topology', 0, $e);
        }

        $endpoints = $this->extractEndpoints($payload);
        
        if (empty($endpoints)) {
            throw new TopologyException('No shard endpoints found in topology response');
        }

        $this->topologyCache = $payload;
        $this->shardEndpoints = $endpoints;
    }

    /**
     * Get metadata URL.
     *
     * @internal
     */
    private function metadataUrl(): string
    {
        if ($this->metadataEndpoint !== null) {
            if (strpos($this->metadataEndpoint, 'http') === 0) {
                return $this->metadataEndpoint;
            }
            return $this->endpoints[0] . $this->metadataEndpoint;
        }
        
        return $this->endpoints[0] . $this->metadataPath;
    }

    /**
     * Extract endpoints from topology payload.
     *
     * @internal
     */
    private function extractEndpoints(array $payload): array
    {
        if (!isset($payload['shards']) || !is_array($payload['shards'])) {
            return [];
        }

        $result = [];
        foreach ($payload['shards'] as $shard) {
            if (is_string($shard)) {
                $normalized = rtrim($shard, '/');
                if (!in_array($normalized, $result)) {
                    $result[] = $normalized;
                }
                continue;
            }

            if (!is_array($shard)) {
                continue;
            }

            $candidates = [];
            if (isset($shard['endpoint'])) {
                $candidates[] = $shard['endpoint'];
            }
            if (isset($shard['http_endpoint'])) {
                $candidates[] = $shard['http_endpoint'];
            }
            if (isset($shard['endpoints']) && is_array($shard['endpoints'])) {
                $candidates = array_merge($candidates, $shard['endpoints']);
            }

            foreach ($candidates as $candidate) {
                if (is_string($candidate)) {
                    $normalized = rtrim($candidate, '/');
                    if (!in_array($normalized, $result)) {
                        $result[] = $normalized;
                    }
                }
            }
        }

        return $result;
    }

    /**
     * Compute stable hash for consistent routing.
     *
     * @internal
     */
    private function stableHash(string $value): int
    {
        $hash = hash('fnv1a32', $value);
        return hexdec($hash);
    }

    /**
     * Check if query targets a single shard.
     *
     * @internal
     */
    private function isSingleShardQuery(string $aql): bool
    {
        return stripos($aql, 'urn:themis:') !== false;
    }

    /**
     * Parse query response payload.
     *
     * @internal
     */
    private function parseQueryPayload(array $payload): array
    {
        if (isset($payload['entities'])) {
            $items = array_map([$this, 'decodeBlob'], $payload['entities']);
            return [
                'items' => $items,
                'has_more' => false,
                'next_cursor' => null,
                'raw' => $payload,
                'count' => $payload['count'] ?? null,
                'table' => $payload['table'] ?? null
            ];
        }

        if (isset($payload['items'])) {
            $items = array_map([$this, 'decodeBlob'], $payload['items']);
            return [
                'items' => $items,
                'has_more' => $payload['has_more'] ?? false,
                'next_cursor' => $payload['next_cursor'] ?? null,
                'raw' => $payload,
                'table' => $payload['table'] ?? null
            ];
        }

        return [
            'items' => [],
            'has_more' => false,
            'next_cursor' => null,
            'raw' => $payload
        ];
    }

    /**
     * Decode blob field (JSON string or raw value).
     *
     * @internal
     */
    private function decodeBlob($blob)
    {
        if (is_string($blob)) {
            $decoded = json_decode($blob, true);
            if (json_last_error() === JSON_ERROR_NONE) {
                return $decoded;
            }
            return $blob;
        }
        return $blob;
    }

    /**
     * Encode data as blob (JSON string).
     *
     * @internal
     */
    private function encodeBlob($data): string
    {
        if (is_string($data)) {
            return $data;
        }
        return json_encode($data);
    }
    
    /**
     * Log a message.
     *
     * @internal
     */
    private function log(string $level, string $message): void
    {
        if ($this->loggingEnabled) {
            echo "[ThemisDB] [{$level}] {$message}\n";
        }
    }
    
    /**
     * Check if a request can be executed (circuit breaker check).
     *
     * @internal
     */
    private function canExecuteRequest(): bool
    {
        if (!$this->circuitBreakerEnabled) {
            return true;
        }
        
        switch ($this->circuitBreakerState) {
            case self::CIRCUIT_CLOSED:
                return true;
                
            case self::CIRCUIT_OPEN:
                if (microtime(true) >= $this->circuitBreakerNextAttemptTime) {
                    $this->circuitBreakerState = self::CIRCUIT_HALF_OPEN;
                    $this->circuitBreakerSuccessCount = 0;
                    return true;
                }
                return false;
                
            case self::CIRCUIT_HALF_OPEN:
                return $this->circuitBreakerSuccessCount < $this->circuitBreakerHalfOpenMax;
                
            default:
                return true;
        }
    }
    
    /**
     * Record a successful request (circuit breaker).
     *
     * @internal
     */
    private function recordCircuitBreakerSuccess(): void
    {
        if (!$this->circuitBreakerEnabled) {
            return;
        }
        
        $this->circuitBreakerFailureCount = 0;
        
        if ($this->circuitBreakerState === self::CIRCUIT_HALF_OPEN) {
            $this->circuitBreakerSuccessCount++;
            if ($this->circuitBreakerSuccessCount >= $this->circuitBreakerHalfOpenMax) {
                $this->circuitBreakerState = self::CIRCUIT_CLOSED;
            }
        }
    }
    
    /**
     * Record a failed request (circuit breaker).
     *
     * @internal
     */
    private function recordCircuitBreakerFailure(): void
    {
        if (!$this->circuitBreakerEnabled) {
            return;
        }
        
        $this->circuitBreakerFailureCount++;
        $this->circuitBreakerSuccessCount = 0;
        
        if ($this->circuitBreakerFailureCount >= $this->circuitBreakerFailureThreshold) {
            $this->circuitBreakerState = self::CIRCUIT_OPEN;
            $this->circuitBreakerNextAttemptTime = microtime(true) + $this->circuitBreakerResetTimeout;
        }
    }
}

/**
 * Exception thrown when a resource is not found (404).
 */
class NotFoundException extends RuntimeException {}

/**
 * Exception thrown when topology operations fail.
 */
class TopologyException extends RuntimeException {}

/**
 * Exception thrown when transaction operations fail.
 */
class TransactionException extends RuntimeException {}
