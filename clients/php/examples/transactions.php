/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            transactions.php                                   ║
  Version:         0.0.43                                             ║
  Last Modified:   2026-04-15 04:08:25                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     196                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 65b6fc41ed  2026-02-24  fix: resolve remaining Python (34) and PHP (23) error-han... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

<?php

/**
 * Transaction Example - ThemisDB PHP SDK
 * 
 * Demonstrates ACID transactions with BEGIN/COMMIT/ROLLBACK.
 */

require_once __DIR__ . '/../vendor/autoload.php';

use ThemisDB\ThemisClient;
use ThemisDB\TransactionException;

// Create client
$client = new ThemisClient(['http://localhost:8080']);

echo "=== ThemisDB PHP SDK - Transaction Example ===\n\n";

// Setup: Create initial accounts
echo "Setup: Creating accounts...\n";
$client->put('relational', 'accounts', 'alice', ['balance' => 1000]);
$client->put('relational', 'accounts', 'bob', ['balance' => 500]);
echo "   Alice: $1000\n";
echo "   Bob: $500\n\n";

// Example 1: Successful transaction
echo "1. Successful transaction: Transfer $200 from Alice to Bob\n";
$tx = $client->beginTransaction(['isolation_level' => 'SNAPSHOT']);

try {
    // Read accounts
    $alice = $tx->get('relational', 'accounts', 'alice');
    $bob = $tx->get('relational', 'accounts', 'bob');
    
    echo "   Before: Alice = ${alice['balance']}, Bob = ${bob['balance']}\n";
    
    // Transfer money
    $alice['balance'] -= 200;
    $bob['balance'] += 200;
    
    // Write updated accounts
    $tx->put('relational', 'accounts', 'alice', $alice);
    $tx->put('relational', 'accounts', 'bob', $bob);
    
    // Commit
    $tx->commit();
    echo "   ✓ Transaction committed\n";
    
    // Verify outside transaction
    $alice = $client->get('relational', 'accounts', 'alice');
    $bob = $client->get('relational', 'accounts', 'bob');
    echo "   After: Alice = ${alice['balance']}, Bob = ${bob['balance']}\n\n";
    
} catch (Exception $e) {
    error_log($e->getMessage());
    $tx->rollback();
    echo "   ✗ Transaction rolled back: {$e->getMessage()}\n\n";
}

// Example 2: Failed transaction (insufficient funds)
echo "2. Failed transaction: Attempt to transfer $2000 from Alice to Bob\n";
$tx = $client->beginTransaction(['isolation_level' => 'SNAPSHOT']);

try {
    $alice = $tx->get('relational', 'accounts', 'alice');
    $bob = $tx->get('relational', 'accounts', 'bob');
    
    echo "   Before: Alice = ${alice['balance']}, Bob = ${bob['balance']}\n";
    
    // Check for sufficient funds
    if ($alice['balance'] < 2000) {
        throw new Exception('Insufficient funds');
    }
    
    // This won't execute
    $alice['balance'] -= 2000;
    $bob['balance'] += 2000;
    
    $tx->put('relational', 'accounts', 'alice', $alice);
    $tx->put('relational', 'accounts', 'bob', $bob);
    
    $tx->commit();
    
} catch (Exception $e) {
    error_log($e->getMessage());
    $tx->rollback();
    echo "   ✗ Transaction rolled back: {$e->getMessage()}\n";
    
    // Verify balances unchanged
    $alice = $client->get('relational', 'accounts', 'alice');
    $bob = $client->get('relational', 'accounts', 'bob');
    echo "   After rollback: Alice = ${alice['balance']}, Bob = ${bob['balance']}\n\n";
}

// Example 3: Multiple operations in one transaction
echo "3. Multi-operation transaction\n";
$tx = $client->beginTransaction();

try {
    // Create a new account
    $tx->put('relational', 'accounts', 'charlie', ['balance' => 1500]);
    
    // Transfer from Alice to Charlie
    $alice = $tx->get('relational', 'accounts', 'alice');
    $charlie = $tx->get('relational', 'accounts', 'charlie');
    
    $alice['balance'] -= 100;
    $charlie['balance'] += 100;
    
    $tx->put('relational', 'accounts', 'alice', $alice);
    $tx->put('relational', 'accounts', 'charlie', $charlie);
    
    // Query within transaction
    $result = $tx->query('FOR acc IN accounts RETURN acc');
    echo "   Accounts in transaction: " . count($result['items']) . "\n";
    
    $tx->commit();
    echo "   ✓ Transaction committed\n\n";
    
} catch (Exception $e) {
    error_log($e->getMessage());
    $tx->rollback();
    echo "   ✗ Transaction rolled back: {$e->getMessage()}\n\n";
}

// Example 4: Transaction isolation demonstration
echo "4. Isolation level demonstration\n";
echo "   Starting two concurrent transactions...\n";

$tx1 = $client->beginTransaction(['isolation_level' => 'SNAPSHOT']);
$tx2 = $client->beginTransaction(['isolation_level' => 'SNAPSHOT']);

try {
    // Both transactions read Alice's account
    $alice1 = $tx1->get('relational', 'accounts', 'alice');
    $alice2 = $tx2->get('relational', 'accounts', 'alice');
    
    echo "   TX1 sees: ${alice1['balance']}\n";
    echo "   TX2 sees: ${alice2['balance']}\n";
    
    // TX1 updates
    $alice1['balance'] += 50;
    $tx1->put('relational', 'accounts', 'alice', $alice1);
    $tx1->commit();
    echo "   TX1 committed (added $50)\n";
    
    // TX2 still sees old snapshot and updates based on it
    $alice2['balance'] += 30;
    $tx2->put('relational', 'accounts', 'alice', $alice2);
    
    // This might fail due to write-write conflict
    try {
        $tx2->commit();
        echo "   TX2 committed (added $30)\n";
    } catch (TransactionException $e) {
        error_log($e->getMessage());
        echo "   TX2 failed: Write-write conflict detected\n";
        $tx2->rollback();
    }
    
} catch (Exception $e) {
    error_log($e->getMessage());
    echo "   Error: {$e->getMessage()}\n";
}

echo "Cleanup: Deleting accounts...\n";
$client->delete('relational', 'accounts', 'alice');
$client->delete('relational', 'accounts', 'bob');
$client->delete('relational', 'accounts', 'charlie');
echo "   ✓ Cleanup complete\n\n";

echo "=== Example completed ===\n";
