# ThemisDB PHP SDK

Official PHP client for ThemisDB - A high-performance multi-model database with native LLM integration.

## Features

- ✅ **Full Type Safety** - Modern PHP with type hints
- ✅ **Transaction Support** - BEGIN/COMMIT/ROLLBACK with isolation levels
- ✅ **Multi-Model** - Relational, Graph, Vector operations
- ✅ **Query Support** - AQL (Advanced Query Language)
- ✅ **Topology-Aware** - Automatic shard routing with consistent hashing
- ✅ **Batch Operations** - Efficient bulk operations
- ✅ **Vector Search** - Similarity search for LLM/AI applications
- ✅ **Graph Operations** - Traverse, shortest path, neighbors
- ✅ **Retry Logic** - Automatic retries for failed requests
- ✅ **Connection Pooling** - Efficient HTTP connection management

## Requirements

- PHP >= 7.4
- ext-curl
- ext-json

## Installation

Install via Composer:

```bash
composer require themisdb/themisdb-php
```

Or add to your `composer.json`:

```json
{
    "require": {
        "themisdb/themisdb-php": "^1.0"
    }
}
```

## Quick Start

```php
<?php

require_once 'vendor/autoload.php';

use ThemisDB\ThemisClient;

// Create client
$client = new ThemisClient(['http://localhost:8080']);

// Basic CRUD
$client->put('relational', 'users', 'user1', ['name' => 'Alice', 'age' => 30]);
$user = $client->get('relational', 'users', 'user1');
print_r($user);

// Delete
$client->delete('relational', 'users', 'user1');
```

## Transaction Support

ThemisDB supports ACID transactions with BEGIN/COMMIT/ROLLBACK semantics.

### Basic Transaction Usage

```php
<?php

use ThemisDB\ThemisClient;

$client = new ThemisClient(['http://localhost:8080']);

// Begin a transaction
$tx = $client->beginTransaction();

try {
    // Perform operations within the transaction
    $tx->put('relational', 'accounts', 'acc1', ['balance' => 1000]);
    $tx->put('relational', 'accounts', 'acc2', ['balance' => 500]);
    
    // Read within transaction
    $acc1 = $tx->get('relational', 'accounts', 'acc1');
    echo "Account 1 balance: {$acc1['balance']}\n";
    
    // Commit the transaction
    $tx->commit();
} catch (Exception $e) {
    // Rollback on error
    $tx->rollback();
    throw $e;
}
```

### Isolation Levels

ThemisDB supports three isolation levels:

- `READ_COMMITTED` (default) – Prevents dirty reads. Non-repeatable reads and phantom reads are possible.
- `SNAPSHOT` – Provides a consistent snapshot of the database as of transaction start.

  > ⚠️ **Write-skew and phantom-read anomalies are possible at SNAPSHOT isolation.**
  > Two concurrent SNAPSHOT transactions reading the same rows and writing disjoint
  > keys can both commit even when their combined effect violates an application
  > invariant (e.g. double-booking, over-withdrawal). Use `SERIALIZABLE` for strict
  > correctness.

- `SERIALIZABLE` – Full serializability via SSI / predicate locking. Prevents write skew
  and phantom reads. May abort more transactions and has higher latency than SNAPSHOT.

```php
<?php

// Use SNAPSHOT isolation for repeatable reads
// WARNING: write skew and phantom reads are possible at this level
$tx = $client->beginTransaction(['isolation_level' => 'SNAPSHOT']);

// Use SERIALIZABLE to prevent write skew and phantom reads
$tx = $client->beginTransaction(['isolation_level' => 'SERIALIZABLE']);

try {
    $user1 = $tx->get('relational', 'users', 'user1');
    $user2 = $tx->get('relational', 'users', 'user2');
    
    // These reads are from the same snapshot
    // even if other transactions modify the data
    
    $tx->commit();
} catch (Exception $e) {
    $tx->rollback();
    throw $e;
}
```

### Money Transfer Example

```php
<?php

function transferMoney($client, $fromAccount, $toAccount, $amount) {
    $tx = $client->beginTransaction(['isolation_level' => 'SNAPSHOT']);
    
    try {
        // Read both accounts
        $fromAcc = $tx->get('relational', 'accounts', $fromAccount);
        $toAcc = $tx->get('relational', 'accounts', $toAccount);
        
        if (!$fromAcc || !$toAcc) {
            throw new Exception('Account not found');
        }
        
        if ($fromAcc['balance'] < $amount) {
            throw new Exception('Insufficient funds');
        }
        
        // Update balances
        $fromAcc['balance'] -= $amount;
        $toAcc['balance'] += $amount;
        
        $tx->put('relational', 'accounts', $fromAccount, $fromAcc);
        $tx->put('relational', 'accounts', $toAccount, $toAcc);
        
        $tx->commit();
    } catch (Exception $e) {
        $tx->rollback();
        throw $e;
    }
}

// Usage
$client = new ThemisClient(['http://localhost:8080']);
transferMoney($client, 'alice', 'bob', 100.0);
```

## Batch Operations

Efficiently process multiple entities in a single call:

```php
<?php

use ThemisDB\ThemisClient;

$client = new ThemisClient(['http://localhost:8080']);

// Batch Put
$items = [
    'user1' => ['name' => 'Alice', 'age' => 30],
    'user2' => ['name' => 'Bob', 'age' => 25],
    'user3' => ['name' => 'Charlie', 'age' => 35]
];

$result = $client->batchPut('relational', 'users', $items);
echo "Succeeded: " . count($result['succeeded']) . "\n";
echo "Failed: " . count($result['failed']) . "\n";

// Batch Get
$uuids = ['user1', 'user2', 'user3'];
$result = $client->batchGet('relational', 'users', $uuids);

print_r($result['found']);    // Array of found entities
print_r($result['missing']);  // Array of missing UUIDs
print_r($result['errors']);   // Array of errors

// Batch Delete
$result = $client->batchDelete('relational', 'users', ['user1', 'user2']);
```

## AQL Queries

Execute queries using ThemisDB's Advanced Query Language:

```php
<?php

use ThemisDB\ThemisClient;

$client = new ThemisClient(['http://localhost:8080']);

// Simple query
$result = $client->query('FOR user IN users FILTER user.age > 25 RETURN user');

foreach ($result['items'] as $user) {
    echo "{$user['name']} is {$user['age']} years old\n";
}

// Parameterized query
$result = $client->query(
    'FOR user IN users FILTER user.city == @city RETURN user',
    ['params' => ['city' => 'Berlin']]
);

// Cursor-based pagination
$result = $client->query(
    'FOR user IN users RETURN user',
    [
        'use_cursor' => true,
        'batch_size' => 100
    ]
);

if ($result['has_more']) {
    $nextPage = $client->query(
        'FOR user IN users RETURN user',
        [
            'use_cursor' => true,
            'cursor' => $result['next_cursor'],
            'batch_size' => 100
        ]
    );
}
```

## Graph Operations

Perform graph traversals and path finding:

```php
<?php

use ThemisDB\ThemisClient;

$client = new ThemisClient(['http://localhost:8080']);

// Traverse graph from a starting node
$nodes = $client->graphTraverse('user:alice', 3);  // max depth 3
print_r($nodes);

// Find shortest path between two nodes
$path = $client->graphShortestPath('user:alice', 'user:charlie');
if ($path) {
    echo "Path found: " . implode(' -> ', $path) . "\n";
}

// Get neighbors of a node
$neighbors = $client->graphNeighbors('user:alice', null, 'both');  // both directions
print_r($neighbors);

// Filter by edge type
$friends = $client->graphNeighbors('user:alice', 'FRIEND', 'out');
print_r($friends);
```

## Vector Operations

Perfect for LLM and AI applications:

```php
<?php

use ThemisDB\ThemisClient;

$client = new ThemisClient(['http://localhost:8080']);

// Upsert a vector
$embedding = array_fill(0, 768, 0.1);  // 768-dimensional embedding
$client->vectorUpsert('doc1', $embedding, ['title' => 'AI Research Paper']);

// Search for similar vectors
$queryEmbedding = array_fill(0, 768, 0.15);
$results = $client->vectorSearch($queryEmbedding, 10);  // top 10 results

foreach ($results['results'] as $result) {
    echo "Document: {$result['id']}, Score: {$result['score']}\n";
}

// Search with metadata filter
$results = $client->vectorSearch(
    $queryEmbedding,
    10,
    ['category' => 'research']  // metadata filter
);

// Delete a vector
$client->vectorDelete('doc1');
```

## Configuration Options

```php
<?php

use ThemisDB\ThemisClient;

$client = new ThemisClient(
    ['http://localhost:8080', 'http://localhost:8081'],  // Multiple endpoints
    [
        'namespace' => 'production',           // Default: 'default'
        'timeout' => 30.0,                     // Request timeout in seconds
        'max_retries' => 3,                    // Maximum retry attempts
        'metadata_endpoint' => null,           // Custom metadata endpoint
        'metadata_path' => '/_admin/cluster/topology'  // Topology path
    ]
);
```

## Error Handling

```php
<?php

use ThemisDB\ThemisClient;
use ThemisDB\NotFoundException;
use ThemisDB\TopologyException;
use ThemisDB\TransactionException;

$client = new ThemisClient(['http://localhost:8080']);

try {
    $user = $client->get('relational', 'users', 'nonexistent');
    // $user will be null for not found
    
    if ($user === null) {
        echo "User not found\n";
    }
} catch (TopologyException $e) {
    echo "Topology error: {$e->getMessage()}\n";
} catch (RuntimeException $e) {
    echo "Request failed: {$e->getMessage()}\n";
}

// Transaction errors
try {
    $tx = $client->beginTransaction();
    $tx->put('relational', 'users', 'user1', ['name' => 'Alice']);
    $tx->commit();
    $tx->commit();  // Error: already committed
} catch (TransactionException $e) {
    echo "Transaction error: {$e->getMessage()}\n";
}
```

## Health Check

```php
<?php

use ThemisDB\ThemisClient;

$client = new ThemisClient(['http://localhost:8080']);

// Check server health
$health = $client->health();
print_r($health);

// Check specific endpoint
$health = $client->health('http://localhost:8081');
print_r($health);
```

## Advanced: Topology-Aware Routing

The client automatically routes requests to the appropriate shard using consistent hashing:

```php
<?php

use ThemisDB\ThemisClient;

// Client automatically fetches topology and routes requests
$client = new ThemisClient(['http://localhost:8080']);

// This request is routed to the correct shard based on the URN
$user = $client->get('relational', 'users', 'alice');

// For distributed deployments, provide all shard endpoints
$client = new ThemisClient([
    'http://shard1.example.com:8080',
    'http://shard2.example.com:8080',
    'http://shard3.example.com:8080'
]);
```

## LLM Integration Example

Perfect for websites with LLM support (WordPress, Laravel, etc.):

```php
<?php

use ThemisDB\ThemisClient;

$client = new ThemisClient(['http://localhost:8080']);

// Store document embeddings from LLM
function storeDocument($client, $docId, $content, $embedding) {
    // Store document
    $client->put('document', 'docs', $docId, [
        'content' => $content,
        'created_at' => date('c')
    ]);
    
    // Store embedding for similarity search
    $client->vectorUpsert($docId, $embedding, [
        'doc_id' => $docId,
        'type' => 'document'
    ]);
}

// Retrieve similar documents for RAG (Retrieval-Augmented Generation)
function findSimilarDocuments($client, $queryEmbedding, $limit = 5) {
    $results = $client->vectorSearch($queryEmbedding, $limit);
    
    $documents = [];
    foreach ($results['results'] as $result) {
        $docId = $result['id'];
        $doc = $client->get('document', 'docs', $docId);
        $documents[] = [
            'content' => $doc['content'],
            'similarity' => $result['score']
        ];
    }
    
    return $documents;
}

// Example usage
$docEmbedding = generateEmbedding("Your document content here");
storeDocument($client, 'doc123', 'Your document content here', $docEmbedding);

$queryEmbedding = generateEmbedding("User query");
$similarDocs = findSimilarDocuments($client, $queryEmbedding);

// Use similar documents for context in LLM prompt
$context = implode("\n\n", array_column($similarDocs, 'content'));
$prompt = "Context:\n{$context}\n\nQuestion: How do I...?";
```

## WordPress Integration Example

```php
<?php

// In your WordPress plugin or theme

use ThemisDB\ThemisClient;

// Initialize client
$themis = new ThemisClient(['http://localhost:8080']);

// Store post with metadata
add_action('save_post', function($post_id, $post) use ($themis) {
    if ($post->post_type !== 'post' || $post->post_status !== 'publish') {
        return;
    }
    
    $data = [
        'title' => $post->post_title,
        'content' => $post->post_content,
        'author' => $post->post_author,
        'date' => $post->post_date
    ];
    
    $themis->put('relational', 'posts', (string)$post_id, $data);
}, 10, 2);

// Search posts using AQL
function search_posts_themis($query) {
    global $themis;
    
    $result = $themis->query(
        'FOR post IN posts FILTER CONTAINS(post.title, @query) OR CONTAINS(post.content, @query) RETURN post',
        ['params' => ['query' => $query]]
    );
    
    return $result['items'];
}
```

## Development

```bash
# Install dependencies
composer install

# Run tests
composer test

# Run static analysis
composer phpstan

# Check code style
composer cs-check

# Fix code style
composer cs-fix
```

## API Reference

### ThemisClient

#### Constructor

```php
new ThemisClient(array $endpoints, array $options = [])
```

**Options:**
- `namespace` (string) - Namespace for entities (default: 'default')
- `timeout` (float) - Request timeout in seconds (default: 30.0)
- `max_retries` (int) - Maximum retry attempts (default: 3)
- `metadata_endpoint` (string|null) - Custom metadata endpoint
- `metadata_path` (string) - Metadata path

#### Methods

- `get(string $model, string $collection, string $uuid): mixed`
- `put(string $model, string $collection, string $uuid, $data): bool`
- `delete(string $model, string $collection, string $uuid): bool`
- `batchGet(string $model, string $collection, array $uuids): array`
- `batchPut(string $model, string $collection, array $items): array`
- `batchDelete(string $model, string $collection, array $uuids): array`
- `query(string $aql, array $options = []): array`
- `graphTraverse(string $startNode, int $maxDepth = 3, ?string $edgeType = null): array`
- `graphShortestPath(string $startNode, string $endNode, ?string $edgeType = null): ?array`
- `graphNeighbors(string $node, ?string $edgeType = null, string $direction = 'both'): array`
- `vectorSearch(array $embedding, int $topK = 10, ?array $metadataFilter = null, array $options = []): array`
- `vectorUpsert(string $id, array $embedding, ?array $metadata = null): bool`
- `vectorDelete(string $id): bool`
- `beginTransaction(array $options = []): Transaction`
- `health(?string $endpoint = null): array`

### Transaction

#### Properties

- `getTransactionId(): string` - Get transaction ID
- `isActive(): bool` - Check if transaction is active

#### Methods

- `get(string $model, string $collection, string $uuid): mixed`
- `put(string $model, string $collection, string $uuid, $data): bool`
- `delete(string $model, string $collection, string $uuid): bool`
- `query(string $aql, array $options = []): array`
- `commit(): void`
- `rollback(): void`

## License

MIT

## Support

- **Documentation**: [https://makr-code.github.io/ThemisDB/](https://makr-code.github.io/ThemisDB/)
- **GitHub Issues**: [Report bugs or request features](https://github.com/makr-code/ThemisDB/issues)
- **Discussions**: [Community discussions](https://github.com/makr-code/ThemisDB/discussions)

