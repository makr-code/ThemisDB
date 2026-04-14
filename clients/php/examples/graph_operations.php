/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            graph_operations.php                               ║
  Version:         0.0.42                                             ║
  Last Modified:   2026-04-14 18:35:53                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     204                                            ║
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

/**
 * Graph Operations Example - ThemisDB PHP SDK
 * 
 * Demonstrates graph traversal and path finding operations.
 */

require_once __DIR__ . '/../vendor/autoload.php';

use ThemisDB\ThemisClient;

// Create client
$client = new ThemisClient(['http://localhost:8080']);

echo "=== ThemisDB PHP SDK - Graph Operations Example ===\n\n";

// Setup: Create a social network graph
echo "Setup: Creating social network graph...\n";

// Create users
$users = [
    'alice' => ['name' => 'Alice', 'city' => 'Berlin'],
    'bob' => ['name' => 'Bob', 'city' => 'Munich'],
    'charlie' => ['name' => 'Charlie', 'city' => 'Hamburg'],
    'diana' => ['name' => 'Diana', 'city' => 'Berlin'],
    'eve' => ['name' => 'Eve', 'city' => 'Frankfurt'],
    'frank' => ['name' => 'Frank', 'city' => 'Berlin']
];

foreach ($users as $userId => $userData) {
    $client->put('graph', 'users', $userId, $userData);
    echo "   ✓ Created user: {$userData['name']}\n";
}

// Create friendships (edges)
echo "\nCreating friendships...\n";
$friendships = [
    ['alice', 'bob'],
    ['alice', 'charlie'],
    ['bob', 'charlie'],
    ['bob', 'diana'],
    ['charlie', 'eve'],
    ['diana', 'eve'],
    ['diana', 'frank'],
    ['eve', 'frank']
];

foreach ($friendships as [$user1, $user2]) {
    // In ThemisDB, you might create edges differently based on your schema
    // This is a simplified example
    $edgeId = "{$user1}_{$user2}";
    $client->put('graph', 'friendships', $edgeId, [
        'from' => $user1,
        'to' => $user2,
        'type' => 'FRIEND',
        'since' => date('Y-m-d')
    ]);
    echo "   ✓ {$users[$user1]['name']} ↔ {$users[$user2]['name']}\n";
}

echo "\nGraph structure:\n";
echo "   Alice ─── Bob ─── Diana ─── Frank\n";
echo "      \\      /         \\      /\n";
echo "       Charlie ─────── Eve\n\n";

// 1. Graph Traversal
echo "1. Traversing from Alice (max depth 3)...\n";
try {
    $nodes = $client->graphTraverse('user:alice', 3);
    echo "   Visited " . count($nodes) . " nodes:\n";
    foreach ($nodes as $node) {
        echo "   - {$node}\n";
    }
} catch (Exception $e) {
    error_log($e->getMessage());
    echo "   Note: Graph traversal requires graph edges to be properly configured\n";
    echo "   This is a simplified example. See ThemisDB docs for full graph setup.\n";
}
echo "\n";

// 2. Shortest Path
echo "2. Finding shortest path from Alice to Frank...\n";
try {
    $path = $client->graphShortestPath('user:alice', 'user:frank');
    if ($path) {
        echo "   Path found: " . implode(' → ', $path) . "\n";
        echo "   Distance: " . (count($path) - 1) . " hops\n";
    } else {
        echo "   No path found\n";
    }
} catch (Exception $e) {
    error_log($e->getMessage());
    echo "   Expected path: alice → bob → diana → frank (3 hops)\n";
    echo "   Note: Requires proper graph configuration\n";
}
echo "\n";

// 3. Get Neighbors
echo "3. Getting Alice's neighbors...\n";
try {
    $neighbors = $client->graphNeighbors('user:alice', 'FRIEND', 'both');
    echo "   Alice has " . count($neighbors) . " friends:\n";
    foreach ($neighbors as $neighbor) {
        echo "   - {$neighbor}\n";
    }
} catch (Exception $e) {
    error_log($e->getMessage());
    echo "   Expected neighbors: bob, charlie\n";
    echo "   Note: Requires proper graph configuration\n";
}
echo "\n";

// 4. Advanced: Find friends of friends
echo "4. Finding friends of friends (2nd degree connections)...\n";
echo "   Starting from Alice:\n";
try {
    // Get direct friends
    $directFriends = $client->graphNeighbors('user:alice', 'FRIEND', 'out');
    echo "   Direct friends: " . implode(', ', $directFriends) . "\n";
    
    // Get friends of each friend
    $friendsOfFriends = [];
    foreach ($directFriends as $friend) {
        $secondDegree = $client->graphNeighbors($friend, 'FRIEND', 'out');
        foreach ($secondDegree as $fof) {
            if ($fof !== 'user:alice' && !in_array($fof, $directFriends)) {
                $friendsOfFriends[] = $fof;
            }
        }
    }
    
    $friendsOfFriends = array_unique($friendsOfFriends);
    echo "   Friends of friends: " . implode(', ', $friendsOfFriends) . "\n";
    
} catch (Exception $e) {
    error_log($e->getMessage());
    echo "   Expected: diana, eve\n";
    echo "   Note: Requires proper graph configuration\n";
}
echo "\n";

// 5. AQL Query for graph analysis
echo "5. Using AQL to find users in the same city...\n";
$result = $client->query(
    'FOR user IN users FILTER user.city == @city RETURN user',
    ['params' => ['city' => 'Berlin']]
);

echo "   Users in Berlin:\n";
foreach ($result['items'] as $user) {
    echo "   - {$user['name']}\n";
}
echo "\n";

// 6. Find mutual friends
echo "6. Finding mutual friends of Alice and Bob...\n";
echo "   (Using query instead of graph operations for this example)\n";

$aliceFriends = ['bob', 'charlie'];  // Simplified
$bobFriends = ['alice', 'charlie', 'diana'];  // Simplified

$mutualFriends = array_intersect($aliceFriends, $bobFriends);
echo "   Mutual friends: " . implode(', ', $mutualFriends) . "\n\n";

// Cleanup
echo "Cleanup: Deleting graph data...\n";
foreach (array_keys($users) as $userId) {
    $client->delete('graph', 'users', $userId);
}
foreach ($friendships as [$user1, $user2]) {
    $edgeId = "{$user1}_{$user2}";
    $client->delete('graph', 'friendships', $edgeId);
}
echo "   ✓ Cleanup complete\n\n";

echo "=== Example completed ===\n";
echo "\nNote: This example shows simplified graph operations.\n";
echo "For full graph functionality, configure proper edge types and indexes.\n";
echo "See ThemisDB documentation for complete graph database setup.\n";
