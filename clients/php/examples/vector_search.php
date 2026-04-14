/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            vector_search.php                                  ║
  Version:         0.0.40                                             ║
  Last Modified:   2026-04-14 06:49:12                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     187                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

<?php

/**
 * Vector Search Example - ThemisDB PHP SDK
 * 
 * Demonstrates vector operations for LLM/AI applications.
 */

require_once __DIR__ . '/../vendor/autoload.php';

use ThemisDB\ThemisClient;

// Create client
$client = new ThemisClient(['http://localhost:8080']);

echo "=== ThemisDB PHP SDK - Vector Search Example ===\n\n";

/**
 * Simulate generating an embedding (in real use, you'd call an LLM API)
 */
function generateMockEmbedding(string $text, int $dimensions = 384): array {
    // Simple mock: hash the text and generate deterministic values
    $hash = crc32($text);
    srand($hash);
    
    $embedding = [];
    for ($i = 0; $i < $dimensions; $i++) {
        $embedding[] = (rand() / getrandmax()) * 2 - 1;  // Random between -1 and 1
    }
    
    return $embedding;
}

// Example documents
$documents = [
    'doc1' => [
        'content' => 'Machine learning is a subset of artificial intelligence.',
        'category' => 'AI',
        'title' => 'Introduction to ML'
    ],
    'doc2' => [
        'content' => 'ThemisDB is a multi-model database with native LLM support.',
        'category' => 'Database',
        'title' => 'ThemisDB Overview'
    ],
    'doc3' => [
        'content' => 'Neural networks are the foundation of deep learning.',
        'category' => 'AI',
        'title' => 'Deep Learning Basics'
    ],
    'doc4' => [
        'content' => 'Vector databases enable efficient similarity search for AI applications.',
        'category' => 'Database',
        'title' => 'Vector Databases'
    ],
    'doc5' => [
        'content' => 'PHP is a popular server-side scripting language for web development.',
        'category' => 'Programming',
        'title' => 'PHP Introduction'
    ]
];

// 1. Index documents with embeddings
echo "1. Indexing documents with embeddings...\n";
foreach ($documents as $docId => $doc) {
    // Generate embedding from content
    $embedding = generateMockEmbedding($doc['content']);
    
    // Store document
    $client->put('document', 'docs', $docId, $doc);
    
    // Store embedding for similarity search
    $client->vectorUpsert($docId, $embedding, [
        'category' => $doc['category'],
        'title' => $doc['title']
    ]);
    
    echo "   ✓ Indexed: {$doc['title']}\n";
}
echo "\n";

// 2. Search for similar documents
echo "2. Searching for documents similar to 'artificial intelligence and machine learning'...\n";
$query = 'artificial intelligence and machine learning';
$queryEmbedding = generateMockEmbedding($query);

$results = $client->vectorSearch($queryEmbedding, 3);  // Top 3 results

echo "   Found " . count($results['results']) . " results:\n";
foreach ($results['results'] as $i => $result) {
    $docId = $result['id'];
    $score = $result['score'] ?? $result['distance'] ?? 0;
    $doc = $client->get('document', 'docs', $docId);
    
    echo "   " . ($i + 1) . ". {$doc['title']} (score: " . number_format($score, 4) . ")\n";
    echo "      Category: {$doc['category']}\n";
    echo "      Content: " . substr($doc['content'], 0, 60) . "...\n";
}
echo "\n";

// 3. Search with metadata filter
echo "3. Searching for AI-related documents only...\n";
$results = $client->vectorSearch(
    $queryEmbedding, 
    5,
    ['category' => 'AI']  // Metadata filter
);

echo "   Found " . count($results['results']) . " AI documents:\n";
foreach ($results['results'] as $result) {
    $docId = $result['id'];
    $doc = $client->get('document', 'docs', $docId);
    echo "   - {$doc['title']}\n";
}
echo "\n";

// 4. RAG Example: Retrieve relevant context for a question
echo "4. RAG Example: Answering 'What is a vector database?'\n";
$question = 'What is a vector database?';
$questionEmbedding = generateMockEmbedding($question);

$results = $client->vectorSearch($questionEmbedding, 2);  // Top 2 most relevant docs

echo "   Retrieved context:\n";
$context = [];
foreach ($results['results'] as $result) {
    $docId = $result['id'];
    $doc = $client->get('document', 'docs', $docId);
    $context[] = $doc['content'];
    echo "   - {$doc['content']}\n";
}
echo "\n";

// In a real application, you would now send this context to an LLM
$ragPrompt = "Context:\n" . implode("\n\n", $context) . "\n\nQuestion: {$question}\n\nAnswer:";
echo "   Generated RAG prompt:\n";
echo "   " . str_replace("\n", "\n   ", $ragPrompt) . "\n\n";

// 5. Update an embedding
echo "5. Updating document embedding...\n";
$newContent = 'Vector databases like ThemisDB enable efficient similarity search for AI and LLM applications.';
$documents['doc4']['content'] = $newContent;
$client->put('document', 'docs', 'doc4', $documents['doc4']);

$newEmbedding = generateMockEmbedding($newContent);
$client->vectorUpsert('doc4', $newEmbedding, [
    'category' => 'Database',
    'title' => 'Vector Databases'
]);
echo "   ✓ Updated doc4\n\n";

// 6. Delete vectors
echo "6. Cleaning up...\n";
foreach (array_keys($documents) as $docId) {
    $client->vectorDelete($docId);
    $client->delete('document', 'docs', $docId);
}
echo "   ✓ All documents and vectors deleted\n\n";

echo "=== Example completed ===\n";
echo "\nNote: In production, use a real embedding model like:\n";
echo "  - OpenAI's text-embedding-ada-002\n";
echo "  - Sentence Transformers (all-MiniLM-L6-v2)\n";
echo "  - ThemisDB's native LLM integration (v1.5.0+)\n";
