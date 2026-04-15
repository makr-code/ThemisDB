/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ThemisClientTest.php                               ║
  Version:         0.0.43                                             ║
  Last Modified:   2026-04-15 04:08:26                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     149                                            ║
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

namespace ThemisDB\Tests;

use PHPUnit\Framework\TestCase;
use ThemisDB\ThemisClient;
use ThemisDB\NotFoundException;
use ThemisDB\TopologyException;
use ThemisDB\TransactionException;

/**
 * Basic unit tests for ThemisDB PHP SDK
 * 
 * Note: These are unit tests that don't require a running ThemisDB server.
 * For integration tests, run the examples against a real server.
 */
class ThemisClientTest extends TestCase
{
    public function testClientConstruction(): void
    {
        $client = new ThemisClient(['http://localhost:8080']);
        $this->assertInstanceOf(ThemisClient::class, $client);
    }

    public function testClientConstructionWithOptions(): void
    {
        $client = new ThemisClient(
            ['http://localhost:8080', 'http://localhost:8081'],
            [
                'namespace' => 'test',
                'timeout' => 60.0,
                'max_retries' => 5
            ]
        );
        $this->assertInstanceOf(ThemisClient::class, $client);
    }

    public function testClientConstructionFailsWithEmptyEndpoints(): void
    {
        $this->expectException(\InvalidArgumentException::class);
        $this->expectExceptionMessage('endpoints must not be empty');
        new ThemisClient([]);
    }

    public function testBuildUrn(): void
    {
        $client = new ThemisClient(['http://localhost:8080']);
        $urn = $client->buildUrn('relational', 'users', 'alice');
        $this->assertEquals('urn:themis:relational:default:users:alice', $urn);
    }

    public function testBuildUrnWithCustomNamespace(): void
    {
        $client = new ThemisClient(
            ['http://localhost:8080'],
            ['namespace' => 'production']
        );
        $urn = $client->buildUrn('relational', 'users', 'alice');
        $this->assertEquals('urn:themis:relational:production:users:alice', $urn);
    }

    public function testBuildEntityKey(): void
    {
        $client = new ThemisClient(['http://localhost:8080']);
        $key = $client->buildEntityKey('relational', 'users', 'alice');
        $this->assertEquals('relational.default.users:alice', $key);
    }

    public function testBatchGetEmptyArray(): void
    {
        $client = new ThemisClient(['http://localhost:8080']);
        $result = $client->batchGet('relational', 'users', []);
        
        $this->assertIsArray($result);
        $this->assertArrayHasKey('found', $result);
        $this->assertArrayHasKey('missing', $result);
        $this->assertArrayHasKey('errors', $result);
        $this->assertEmpty($result['found']);
        $this->assertEmpty($result['missing']);
        $this->assertEmpty($result['errors']);
    }

    public function testBatchPutEmptyArray(): void
    {
        $client = new ThemisClient(['http://localhost:8080']);
        $result = $client->batchPut('relational', 'users', []);
        
        $this->assertIsArray($result);
        $this->assertArrayHasKey('succeeded', $result);
        $this->assertArrayHasKey('failed', $result);
        $this->assertEmpty($result['succeeded']);
        $this->assertEmpty($result['failed']);
    }

    public function testBatchDeleteEmptyArray(): void
    {
        $client = new ThemisClient(['http://localhost:8080']);
        $result = $client->batchDelete('relational', 'users', []);
        
        $this->assertIsArray($result);
        $this->assertArrayHasKey('succeeded', $result);
        $this->assertArrayHasKey('failed', $result);
        $this->assertEmpty($result['succeeded']);
        $this->assertEmpty($result['failed']);
    }

    public function testTransactionConstruction(): void
    {
        $client = new ThemisClient(['http://localhost:8080']);
        $transaction = new \ThemisDB\Transaction($client, 'test-tx-123');
        
        $this->assertEquals('test-tx-123', $transaction->getTransactionId());
        $this->assertTrue($transaction->isActive());
    }

    public function testExceptionClasses(): void
    {
        $this->assertTrue(class_exists(NotFoundException::class));
        $this->assertTrue(class_exists(TopologyException::class));
        $this->assertTrue(class_exists(TransactionException::class));
        
        $this->assertInstanceOf(\RuntimeException::class, new NotFoundException());
        $this->assertInstanceOf(\RuntimeException::class, new TopologyException());
        $this->assertInstanceOf(\RuntimeException::class, new TransactionException());
    }
}
