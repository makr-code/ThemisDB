/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            Transaction.php                                    ║
  Version:         0.0.38                                             ║
  Last Modified:   2026-04-13 04:12:38                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     367                                            ║
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

/**
 * Represents an ACID transaction in ThemisDB.
 * 
 * Supports BEGIN/COMMIT/ROLLBACK semantics with snapshot isolation.
 * All operations within a transaction see a consistent view of the database.
 * 
 * @package ThemisDB
 */
class Transaction
{
    private ThemisClient $client;
    private string $transactionId;
    private bool $committed = false;
    private bool $rolledBack = false;

    /**
     * Create a new transaction.
     *
     * @internal Use ThemisClient::beginTransaction() instead
     * 
     * @param ThemisClient $client The client instance
     * @param string $transactionId Transaction identifier
     */
    public function __construct(ThemisClient $client, string $transactionId)
    {
        $this->client = $client;
        $this->transactionId = $transactionId;
    }

    /**
     * Get the transaction ID.
     *
     * @return string Transaction identifier
     */
    public function getTransactionId(): string
    {
        return $this->transactionId;
    }

    /**
     * Check if the transaction is still active.
     *
     * @return bool True if transaction is active (not committed or rolled back)
     */
    public function isActive(): bool
    {
        return !$this->committed && !$this->rolledBack;
    }

    /**
     * Get an entity within the transaction.
     *
     * @param string $model Model name
     * @param string $collection Collection name
     * @param string $uuid Entity UUID
     * @return mixed Entity data or null if not found
     * @throws TransactionException If transaction is not active
     * @throws \RuntimeException If request fails
     */
    public function get(string $model, string $collection, string $uuid)
    {
        $this->ensureActive();
        
        $urn = $this->client->buildUrn($model, $collection, $uuid);
        $key = $this->client->buildEntityKey($model, $collection, $uuid);
        $endpoint = $this->client->resolveEndpoint($urn);
        
        try {
            $response = $this->txRequest('GET', "{$endpoint}/entities/{$key}");
            
            if (isset($response['entity'])) {
                return $response['entity'];
            }
            
            if (isset($response['blob'])) {
                return $this->decodeBlob($response['blob']);
            }
            
            return $response;
        } catch (NotFoundException $e) {
            error_log("Transaction entity not found: " . $e->getMessage());
            return null;
        }
    }

    /**
     * Create or update an entity within the transaction.
     *
     * @param string $model Model name
     * @param string $collection Collection name
     * @param string $uuid Entity UUID
     * @param mixed $data Entity data
     * @return bool True on success
     * @throws TransactionException If transaction is not active
     * @throws \RuntimeException If request fails
     */
    public function put(string $model, string $collection, string $uuid, $data): bool
    {
        $this->ensureActive();
        
        $urn = $this->client->buildUrn($model, $collection, $uuid);
        $key = $this->client->buildEntityKey($model, $collection, $uuid);
        $endpoint = $this->client->resolveEndpoint($urn);
        
        $body = ['blob' => $this->encodeBlob($data)];
        $this->txRequest('PUT', "{$endpoint}/entities/{$key}", $body);
        
        return true;
    }

    /**
     * Delete an entity within the transaction.
     *
     * @param string $model Model name
     * @param string $collection Collection name
     * @param string $uuid Entity UUID
     * @return bool True if deleted, false if not found
     * @throws TransactionException If transaction is not active
     * @throws \RuntimeException If request fails
     */
    public function delete(string $model, string $collection, string $uuid): bool
    {
        $this->ensureActive();
        
        $urn = $this->client->buildUrn($model, $collection, $uuid);
        $key = $this->client->buildEntityKey($model, $collection, $uuid);
        $endpoint = $this->client->resolveEndpoint($urn);
        
        try {
            $this->txRequest('DELETE', "{$endpoint}/entities/{$key}");
            return true;
        } catch (NotFoundException $e) {
            error_log("Transaction entity not found during delete: " . $e->getMessage());
            return false;
        }
    }

    /**
     * Execute an AQL query within the transaction.
     *
     * @param string $aql AQL query string
     * @param array $options Query options (same as ThemisClient::query)
     * @return array Query result
     * @throws TransactionException If transaction is not active
     * @throws \RuntimeException If query fails
     */
    public function query(string $aql, array $options = []): array
    {
        $this->ensureActive();
        
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
            ? [$this->client->resolveQueryEndpoint($aql)]
            : $this->client->currentEndpoints();

        $partials = [];
        foreach ($endpoints as $endpoint) {
            $response = $this->txRequest('POST', "{$endpoint}/query/aql", $payload);
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
     * Commit the transaction.
     *
     * @throws TransactionException If transaction is not active or commit fails
     */
    public function commit(): void
    {
        $this->ensureActive();
        
        $endpoint = $this->client->getEndpoints()[0];
        $body = ['transaction_id' => $this->transactionId];
        
        $this->txRequest('POST', "{$endpoint}/transaction/commit", $body);
        $this->committed = true;
    }

    /**
     * Rollback the transaction.
     *
     * @throws TransactionException If transaction is not active or rollback fails
     */
    public function rollback(): void
    {
        $this->ensureActive();
        
        $endpoint = $this->client->getEndpoints()[0];
        $body = ['transaction_id' => $this->transactionId];
        
        $this->txRequest('POST', "{$endpoint}/transaction/rollback", $body);
        $this->rolledBack = true;
    }

    /**
     * Ensure the transaction is still active.
     *
     * @throws TransactionException If transaction is not active
     */
    private function ensureActive(): void
    {
        if ($this->committed) {
            throw new TransactionException('Transaction already committed');
        }
        if ($this->rolledBack) {
            throw new TransactionException('Transaction already rolled back');
        }
    }

    /**
     * Make a request within the transaction context.
     *
     * @internal
     */
    private function txRequest(string $method, string $url, ?array $body = null): array
    {
        $headers = ["X-Transaction-Id: {$this->transactionId}"];
        return $this->client->request($method, $url, $body, $headers);
    }

    /**
     * Decode blob field.
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
     * Encode data as blob.
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
}
