/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            basic_crud.php                                     ║
  Version:         0.0.8                                              ║
  Last Modified:   2026-02-21 12:08:39                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     117                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

<?php

/**
 * Basic CRUD Example - ThemisDB PHP SDK
 * 
 * Demonstrates basic create, read, update, and delete operations.
 */

require_once __DIR__ . '/../vendor/autoload.php';

use ThemisDB\ThemisClient;

// Create client
$client = new ThemisClient(['http://localhost:8080']);

echo "=== ThemisDB PHP SDK - Basic CRUD Example ===\n\n";

// 1. Create (PUT) an entity
echo "1. Creating user 'alice'...\n";
$client->put('relational', 'users', 'alice', [
    'name' => 'Alice Smith',
    'age' => 30,
    'city' => 'Berlin',
    'email' => 'alice@example.com'
]);
echo "   ✓ User created\n\n";

// 2. Read (GET) the entity
echo "2. Reading user 'alice'...\n";
$user = $client->get('relational', 'users', 'alice');
echo "   Name: {$user['name']}\n";
echo "   Age: {$user['age']}\n";
echo "   City: {$user['city']}\n";
echo "   Email: {$user['email']}\n\n";

// 3. Update the entity
echo "3. Updating user 'alice'...\n";
$user['age'] = 31;
$user['city'] = 'Munich';
$client->put('relational', 'users', 'alice', $user);
echo "   ✓ User updated\n\n";

// 4. Read again to verify update
echo "4. Reading updated user 'alice'...\n";
$updatedUser = $client->get('relational', 'users', 'alice');
echo "   Age: {$updatedUser['age']}\n";
echo "   City: {$updatedUser['city']}\n\n";

// 5. Create more users for batch example
echo "5. Creating more users...\n";
$client->put('relational', 'users', 'bob', [
    'name' => 'Bob Johnson',
    'age' => 25,
    'city' => 'Hamburg'
]);
$client->put('relational', 'users', 'charlie', [
    'name' => 'Charlie Brown',
    'age' => 35,
    'city' => 'Frankfurt'
]);
echo "   ✓ Users created\n\n";

// 6. Batch get
echo "6. Batch getting all users...\n";
$result = $client->batchGet('relational', 'users', ['alice', 'bob', 'charlie', 'nonexistent']);
echo "   Found: " . count($result['found']) . " users\n";
echo "   Missing: " . count($result['missing']) . " users\n";
foreach ($result['found'] as $uuid => $userData) {
    echo "   - {$userData['name']} ({$uuid})\n";
}
echo "\n";

// 7. Delete an entity
echo "7. Deleting user 'alice'...\n";
$deleted = $client->delete('relational', 'users', 'alice');
echo $deleted ? "   ✓ User deleted\n" : "   ✗ User not found\n";
echo "\n";

// 8. Try to get deleted entity
echo "8. Trying to get deleted user 'alice'...\n";
$deletedUser = $client->get('relational', 'users', 'alice');
echo $deletedUser === null ? "   ✓ User not found (as expected)\n" : "   ✗ User still exists\n";
echo "\n";

// 9. Batch delete
echo "9. Batch deleting remaining users...\n";
$result = $client->batchDelete('relational', 'users', ['bob', 'charlie']);
echo "   Succeeded: " . count($result['succeeded']) . "\n";
echo "   Failed: " . count($result['failed']) . "\n\n";

echo "=== Example completed ===\n";
