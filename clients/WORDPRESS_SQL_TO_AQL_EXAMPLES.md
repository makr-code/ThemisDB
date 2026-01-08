# SQL zu AQL Translation - Kritische Beispiele

## Übersicht

Diese Datei zeigt konkrete Beispiele für die schwierigsten SQL→AQL Übersetzungen in WordPress.

---

## 1. WordPress Taxonomy Query (Multi-JOIN)

### Original WordPress SQL

```sql
SELECT DISTINCT p.* 
FROM wp_posts p
INNER JOIN wp_term_relationships tr ON p.ID = tr.object_id
INNER JOIN wp_term_taxonomy tt ON tr.term_taxonomy_id = tt.term_taxonomy_id
INNER JOIN wp_terms t ON tt.term_id = t.term_id
WHERE tt.taxonomy = 'category'
  AND t.slug = 'wordpress'
  AND p.post_status = 'publish'
  AND p.post_type = 'post'
ORDER BY p.post_date DESC
LIMIT 10
```

### Problem-Analyse

**Schwierigkeiten:**
- 3-facher INNER JOIN
- DISTINCT Keyword
- Multiple WHERE Conditions
- Optimaler Index-Nutzung

### AQL Translation (Variante 1: Naiv)

```aql
FOR post IN posts
    FILTER post.post_status == 'publish'
    FILTER post.post_type == 'post'
    
    FOR relationship IN term_relationships
        FILTER relationship.object_id == post._key
        
        FOR taxonomy IN term_taxonomy
            FILTER taxonomy.term_taxonomy_id == relationship.term_taxonomy_id
            FILTER taxonomy.taxonomy == 'category'
            
            FOR term IN terms
                FILTER term.term_id == taxonomy.term_id
                FILTER term.slug == 'wordpress'
                
                SORT post.post_date DESC
                LIMIT 10
                RETURN DISTINCT post
```

**Performance:** 🔴 **SEHR SCHLECHT**
- N * M * O * P Iterationen
- Keine Index-Nutzung
- Wiederholte Sorts
- DISTINCT am Ende ineffizient

### AQL Translation (Variante 2: Optimiert)

```aql
LET wordpress_term = FIRST(
    FOR t IN terms
        FILTER t.slug == 'wordpress'
        LIMIT 1
        RETURN t
)

LET category_taxonomies = (
    FOR tt IN term_taxonomy
        FILTER tt.term_id == wordpress_term.term_id
        FILTER tt.taxonomy == 'category'
        RETURN tt.term_taxonomy_id
)

LET post_ids = (
    FOR tr IN term_relationships
        FILTER tr.term_taxonomy_id IN category_taxonomies
        RETURN DISTINCT tr.object_id
)

FOR post IN posts
    FILTER post._key IN post_ids
    FILTER post.post_status == 'publish'
    FILTER post.post_type == 'post'
    SORT post.post_date DESC
    LIMIT 10
    RETURN post
```

**Performance:** 🟡 **AKZEPTABEL**
- 4 separate Schritte statt verschachtelt
- Nutzt Subqueries mit LET
- Frühe Filterung reduziert Datenmenge
- Immer noch langsamer als SQL

### Implementierungs-Schwierigkeit

```php
/**
 * JOIN Translation Complexity
 */
class JoinTranslator {
    
    /**
     * Translate Multi-JOIN SQL to AQL
     * 
     * @param array $ast SQL AST with JOINs
     * @return string AQL Query
     */
    public function translateMultiJoin($ast) {
        // Step 1: Analyze JOIN tree
        $join_tree = $this->buildJoinTree($ast);
        
        // Step 2: Determine optimal order (CRITICAL!)
        $optimal_order = $this->optimizeJoinOrder($join_tree);
        
        // Step 3: Generate AQL
        if ($this->canUseSubqueries($optimal_order)) {
            // Variante 2: Mit Subqueries
            return $this->generateOptimizedAQL($optimal_order);
        } else {
            // Variante 1: Verschachtelte FORs
            return $this->generateNestedAQL($optimal_order);
        }
    }
    
    /**
     * PROBLEM: Optimale Join-Reihenfolge bestimmen
     * 
     * MySQL Query Optimizer macht das automatisch in Millisekunden.
     * Wir müssen es manuell implementieren!
     */
    private function optimizeJoinOrder($join_tree) {
        // TODO: Cardinality estimation
        // TODO: Index availability check
        // TODO: Cost-based optimization
        
        // Simplified: Smallest table first
        usort($join_tree, function($a, $b) {
            return $this->estimateTableSize($a) <=> $this->estimateTableSize($b);
        });
        
        return $join_tree;
    }
    
    /**
     * PROBLEM: Tabellengrößen schätzen ohne Statistics
     */
    private function estimateTableSize($table) {
        // MySQL hat INFORMATION_SCHEMA
        // ThemisDB hat... nichts
        
        // Fallback: Heuristics
        $size_map = [
            'posts' => 10000,
            'postmeta' => 50000,
            'terms' => 1000,
            'term_taxonomy' => 1000,
            'term_relationships' => 20000,
        ];
        
        return $size_map[$table['name']] ?? 5000;
    }
}
```

**Schwierigkeit:** 🔴 **9/10**  
**Zeilen Code:** ~1,000-2,000  
**Zeit:** 3-4 Wochen

---

## 2. WooCommerce Product Query

### Original SQL

```sql
SELECT DISTINCT
    p.ID,
    p.post_title,
    pm1.meta_value AS price,
    pm2.meta_value AS stock,
    pm3.meta_value AS sku
FROM wp_posts p
INNER JOIN wp_postmeta pm1 ON p.ID = pm1.post_id AND pm1.meta_key = '_price'
INNER JOIN wp_postmeta pm2 ON p.ID = pm2.post_id AND pm2.meta_key = '_stock'
LEFT JOIN wp_postmeta pm3 ON p.ID = pm3.post_id AND pm3.meta_key = '_sku'
WHERE p.post_type = 'product'
  AND p.post_status = 'publish'
  AND CAST(pm1.meta_value AS DECIMAL) BETWEEN 10 AND 100
  AND CAST(pm2.meta_value AS SIGNED) > 0
ORDER BY CAST(pm1.meta_value AS DECIMAL) ASC
LIMIT 20
```

### Problem-Analyse

**Schwierigkeiten:**
- Multiple self-JOINs auf gleiche Tabelle (wp_postmeta)
- LEFT JOIN mit NULL-Handling
- CAST für Type Conversion
- BETWEEN Operator
- Decimal Math

### AQL Translation

```aql
FOR post IN posts
    FILTER post.post_type == 'product'
    FILTER post.post_status == 'publish'
    
    LET price_meta = FIRST(
        FOR pm IN postmeta
            FILTER pm.post_id == post._key
            FILTER pm.meta_key == '_price'
            RETURN TO_NUMBER(pm.meta_value)
    )
    
    LET stock_meta = FIRST(
        FOR pm IN postmeta
            FILTER pm.post_id == post._key
            FILTER pm.meta_key == '_stock'
            RETURN TO_NUMBER(pm.meta_value)
    )
    
    LET sku_meta = FIRST(
        FOR pm IN postmeta
            FILTER pm.post_id == post._key
            FILTER pm.meta_key == '_sku'
            RETURN pm.meta_value
    )
    
    FILTER price_meta != null
    FILTER price_meta >= 10 AND price_meta <= 100
    FILTER stock_meta != null
    FILTER stock_meta > 0
    
    SORT price_meta ASC
    LIMIT 20
    
    RETURN {
        ID: post._key,
        post_title: post.post_title,
        price: price_meta,
        stock: stock_meta,
        sku: sku_meta
    }
```

### Performance-Problem

**SQL Execution:**
```
1 Query mit JOINs = 1 Roundtrip
Execution Time: ~5-20ms
```

**AQL Execution (oben):**
```
1 Main Query
+ 3 Subqueries PER POST
= 1 + (3 * N) Operations

Für 20 Posts: 1 + 60 = 61 Operations
Execution Time: ~100-500ms
```

**Performance-Ratio:** 🔴 **10-25x langsamer**

### Optimierte AQL Variante

```aql
// Pre-aggregate metadata
LET product_meta = (
    FOR pm IN postmeta
        FILTER pm.meta_key IN ['_price', '_stock', '_sku']
        COLLECT post_id = pm.post_id INTO meta_items = {
            key: pm.meta_key,
            value: pm.meta_value
        }
        RETURN {
            post_id: post_id,
            price: FIRST(meta_items[* FILTER CURRENT.key == '_price'].value),
            stock: FIRST(meta_items[* FILTER CURRENT.key == '_stock'].value),
            sku: FIRST(meta_items[* FILTER CURRENT.key == '_sku'].value)
        }
)

FOR post IN posts
    FILTER post.post_type == 'product'
    FILTER post.post_status == 'publish'
    
    LET meta = FIRST(product_meta[* FILTER CURRENT.post_id == post._key])
    
    FILTER meta != null
    FILTER TO_NUMBER(meta.price) >= 10
    FILTER TO_NUMBER(meta.price) <= 100
    FILTER TO_NUMBER(meta.stock) > 0
    
    SORT TO_NUMBER(meta.price) ASC
    LIMIT 20
    
    RETURN {
        ID: post._key,
        post_title: post.post_title,
        price: TO_NUMBER(meta.price),
        stock: TO_NUMBER(meta.stock),
        sku: meta.sku
    }
```

**Besser, aber:**
- Immer noch 2 Main Queries (statt 1 in SQL)
- COLLECT ist teuer für große Datasets
- TO_NUMBER wird mehrfach aufgerufen

**Schwierigkeit:** 🔴 **10/10**  
**Zeilen Code:** ~500-1,000 für generischen Translator  
**Zeit:** 2-3 Wochen

---

## 3. WordPress Meta Query (Complex OR)

### Original SQL

```sql
SELECT p.ID
FROM wp_posts p
INNER JOIN wp_postmeta pm1 ON p.ID = pm1.post_id
INNER JOIN wp_postmeta pm2 ON p.ID = pm2.post_id
WHERE p.post_type = 'post'
  AND (
    (pm1.meta_key = 'featured' AND pm1.meta_value = '1')
    OR
    (pm2.meta_key = 'priority' AND CAST(pm2.meta_value AS UNSIGNED) > 5)
  )
GROUP BY p.ID
```

### Problem-Analyse

**Schwierigkeiten:**
- OR Condition über JOINs
- Muss als GROUP BY umgesetzt werden (sonst Duplikate)
- Self-JOIN auf postmeta
- Type Casting

### AQL Translation (Naiv - FALSCH!)

```aql
FOR post IN posts
    FILTER post.post_type == 'post'
    
    FOR pm1 IN postmeta
        FILTER pm1.post_id == post._key
        FILTER pm1.meta_key == 'featured'
        FILTER pm1.meta_value == '1'
        RETURN post.ID
    
    OR  // ❌ AQL hat kein OR zwischen FOR-Blöcken!
    
    FOR pm2 IN postmeta
        FILTER pm2.post_id == post._key
        FILTER pm2.meta_key == 'priority'
        FILTER TO_NUMBER(pm2.meta_value) > 5
        RETURN post.ID
```

**Das funktioniert NICHT!**

### AQL Translation (Korrekt - Komplex)

```aql
FOR post IN posts
    FILTER post.post_type == 'post'
    
    LET has_featured = LENGTH(
        FOR pm IN postmeta
            FILTER pm.post_id == post._key
            FILTER pm.meta_key == 'featured'
            FILTER pm.meta_value == '1'
            LIMIT 1
            RETURN 1
    ) > 0
    
    LET has_priority = LENGTH(
        FOR pm IN postmeta
            FILTER pm.post_id == post._key
            FILTER pm.meta_key == 'priority'
            FILTER TO_NUMBER(pm.meta_value) > 5
            LIMIT 1
            RETURN 1
    ) > 0
    
    FILTER has_featured OR has_priority
    
    RETURN post.ID
```

**Problem:**
- 2 Subqueries pro Post
- LENGTH() nur um Boolean zu erhalten
- Keine GROUP BY nötig (DISTINCT implizit)

### Implementierungs-Challenge

```php
/**
 * OR Condition Translator
 */
class OrConditionTranslator {
    
    /**
     * Translate SQL OR with JOINs
     * 
     * This is ONE OF THE HARDEST CASES!
     */
    public function translateOrWithJoins($ast) {
        // SQL: (JOIN ... WHERE A) OR (JOIN ... WHERE B)
        // AQL: Muss in separate LET-Blöcke aufgeteilt werden
        
        $or_branches = $this->extractOrBranches($ast['where']);
        
        $aql = "FOR doc IN " . $ast['from'] . "\n";
        
        // Generate LET for each OR branch
        foreach ($or_branches as $i => $branch) {
            $aql .= "  LET condition_{$i} = (\n";
            $aql .= $this->translateBranch($branch);
            $aql .= "  )\n";
        }
        
        // Combine with OR
        $conditions = array_map(function($i) {
            return "condition_{$i}";
        }, range(0, count($or_branches) - 1));
        
        $aql .= "  FILTER " . implode(' OR ', $conditions) . "\n";
        $aql .= "  RETURN doc";
        
        return $aql;
    }
    
    /**
     * PROBLEM: Detecting OR branches that involve JOINs
     * 
     * MySQL Optimizer handles this elegantly.
     * We need complex AST analysis!
     */
    private function extractOrBranches($where_ast) {
        // TODO: Deep AST traversal
        // TODO: Identify JOIN dependencies
        // TODO: Extract independent branches
        
        throw new Exception("Complex OR detection not implemented");
    }
}
```

**Schwierigkeit:** 🔴 **10/10**  
**Zeilen Code:** ~1,000-1,500  
**Zeit:** 2-3 Wochen

---

## 4. Transaction with Savepoint (WooCommerce Checkout)

### Original WordPress/WooCommerce Code

```php
// Start transaction
$wpdb->query('START TRANSACTION');

try {
    // Create order
    $order_id = $wpdb->insert('wp_wc_orders', [
        'customer_id' => $customer_id,
        'total' => $total,
        'status' => 'pending'
    ]);
    
    // Savepoint before items
    $wpdb->query('SAVEPOINT before_items');
    
    // Insert order items
    foreach ($items as $item) {
        $result = $wpdb->insert('wp_wc_order_items', [
            'order_id' => $order_id,
            'product_id' => $item['product_id'],
            'quantity' => $item['quantity']
        ]);
        
        if (!$result) {
            // Rollback to savepoint
            $wpdb->query('ROLLBACK TO SAVEPOINT before_items');
            // Try alternative...
        }
    }
    
    // Commit
    $wpdb->query('COMMIT');
    
} catch (Exception $e) {
    $wpdb->query('ROLLBACK');
}
```

### ThemisDB Limitation

```php
// ThemisDB DOES NOT SUPPORT SAVEPOINT!

$tx = $client->beginTransaction();

try {
    // Create order
    $order_id = generate_id();
    $tx->put('relational', 'orders', $order_id, [
        'customer_id' => $customer_id,
        'total' => $total,
        'status' => 'pending'
    ]);
    
    // ❌ NO SAVEPOINT!
    // $tx->savepoint('before_items');  // DOES NOT EXIST
    
    // Insert order items
    foreach ($items as $item) {
        try {
            $tx->put('relational', 'order_items', generate_id(), [
                'order_id' => $order_id,
                'product_id' => $item['product_id'],
                'quantity' => $item['quantity']
            ]);
        } catch (Exception $e) {
            // ❌ CAN'T ROLLBACK TO SAVEPOINT
            // Must rollback entire transaction!
            throw $e;
        }
    }
    
    $tx->commit();
    
} catch (Exception $e) {
    $tx->rollback();  // Rolls back EVERYTHING
}
```

### Workaround (Komplex)

```php
/**
 * Simulate Savepoints with Manual State Tracking
 */
class SavepointSimulator {
    
    private $transaction;
    private $savepoints = [];
    private $operations = [];
    
    public function savepoint($name) {
        // Save current state
        $this->savepoints[$name] = [
            'operation_count' => count($this->operations),
            'timestamp' => microtime(true)
        ];
    }
    
    public function rollbackToSavepoint($name) {
        if (!isset($this->savepoints[$name])) {
            throw new Exception("Savepoint not found: {$name}");
        }
        
        $savepoint = $this->savepoints[$name];
        $ops_to_undo = array_slice(
            $this->operations,
            $savepoint['operation_count']
        );
        
        // ❌ PROBLEM: ThemisDB Transaction kann nicht teilweise rückgängig gemacht werden!
        // Müssen komplett neu starten
        
        // Lösung: Rollback entire transaction and replay
        $this->transaction->rollback();
        
        // Start new transaction
        $this->transaction = $client->beginTransaction();
        
        // Replay operations UP TO savepoint
        $ops_to_replay = array_slice(
            $this->operations,
            0,
            $savepoint['operation_count']
        );
        
        foreach ($ops_to_replay as $op) {
            $this->executeOperation($op);
        }
        
        // Remove operations after savepoint
        $this->operations = $ops_to_replay;
    }
}
```

**PROBLEM mit diesem Workaround:**
- 🔴 Sehr langsam (komplette Transaction replay)
- 🔴 Race Conditions möglich
- 🔴 Nicht für Hochlast geeignet
- 🔴 Komplexe Fehlerbehandlung

**Schwierigkeit:** 🔴 **9/10**  
**Zeilen Code:** ~500-800  
**Zeit:** 1-2 Wochen

---

## 5. LAST_INSERT_ID() mit Race Conditions

### Problem

```php
// WordPress Code (atomisch und sicher)
$wpdb->insert('wp_posts', ['post_title' => 'Test']);
$post_id = $wpdb->insert_id;  // Guaranteed unique and correct

// Weiterer Code nutzt $post_id...
$wpdb->insert('wp_postmeta', [
    'post_id' => $post_id,
    'meta_key' => 'views',
    'meta_value' => '0'
]);
```

### ThemisDB Problem

```php
// ThemisDB - NICHT atomisch!

// Request 1:
$max_id = get_max_post_id();  // Returns 999
$new_id = $max_id + 1;        // 1000

// Request 2 (parallel!):
$max_id = get_max_post_id();  // Returns 999 (noch nicht committed!)
$new_id = $max_id + 1;        // 1000 (KONFLIKT!)

// Beide versuchen ID 1000 zu nutzen!
```

### Lösungsansätze

#### Option 1: UUID (Empfohlen)

```php
// Nutze UUIDs statt AUTO_INCREMENT
$post_id = $this->generateUUID();  // z.B. "550e8400-e29b-41d4-a716-446655440000"

$client->put('relational', 'posts', $post_id, [
    'post_title' => 'Test'
]);
```

**Problem:** WordPress erwartet Integer IDs!
```php
// WordPress Code überall:
$post_id = (int)$post_id;  // UUID kann nicht zu Integer gecastet werden!
```

#### Option 2: Distributed ID Generator

```php
/**
 * Snowflake-style ID Generator
 * 
 * ID Format (64 bits):
 * - 41 bits: Timestamp
 * - 10 bits: Machine ID
 * - 12 bits: Sequence
 */
class DistributedIDGenerator {
    
    private $machine_id;
    private $sequence = 0;
    private $last_timestamp = 0;
    
    public function generateID() {
        $timestamp = $this->getCurrentTimestamp();
        
        if ($timestamp < $this->last_timestamp) {
            throw new Exception("Clock moved backwards!");
        }
        
        if ($timestamp == $this->last_timestamp) {
            $this->sequence = ($this->sequence + 1) & 0xFFF;
            if ($this->sequence == 0) {
                // Sequence overflow, wait for next millisecond
                $timestamp = $this->waitNextMillis($timestamp);
            }
        } else {
            $this->sequence = 0;
        }
        
        $this->last_timestamp = $timestamp;
        
        // Build ID
        $id = (($timestamp - 1609459200000) << 22)  // Epoch: 2021-01-01
            | ($this->machine_id << 12)
            | $this->sequence;
        
        return $id;
    }
}
```

**Problem:** Komplexe Implementierung, erfordert Clock-Synchronisation

#### Option 3: ThemisDB Sequence Support (Ideal)

```aql
// Feature Request für ThemisDB:
LET next_id = SEQUENCE('posts_id_seq')
INSERT {_key: TO_STRING(next_id), ...} INTO posts
RETURN next_id
```

**Aktuell nicht verfügbar!**

**Schwierigkeit:** 🔴 **8/10**  
**Zeilen Code:** ~300-500  
**Zeit:** 1-2 Wochen

---

## 6. Performance Killer: N+1 Query Problem

### WordPress Loop (optimiert mit SQL)

```php
// 1 Query für Posts
$posts = get_posts(['posts_per_page' => 10]);

// 1 Query für alle Meta-Daten (mit WHERE IN)
$post_ids = wp_list_pluck($posts, 'ID');
$meta = $wpdb->get_results("
    SELECT post_id, meta_key, meta_value
    FROM wp_postmeta
    WHERE post_id IN (" . implode(',', $post_ids) . ")
");

// Total: 2 Queries
```

### ThemisDB Naive Implementation

```php
// 1 Query für Posts
$result = $client->query("FOR post IN posts LIMIT 10 RETURN post");
$posts = $result['items'];

// 10 Queries für Meta-Daten (N+1 Problem!)
foreach ($posts as $post) {
    $meta = $client->query("
        FOR m IN postmeta
            FILTER m.post_id == @post_id
            RETURN m
    ", ['params' => ['post_id' => $post['_key']]]);
}

// Total: 1 + 10 = 11 Queries
```

**Performance:** 🔴 **5-10x langsamer**

### Optimierte ThemisDB Implementation

```php
// 1 Query für Posts
$result = $client->query("FOR post IN posts LIMIT 10 RETURN post");
$posts = $result['items'];
$post_ids = array_column($posts, '_key');

// 1 Query für alle Meta-Daten (Batch)
$meta = $client->query("
    FOR m IN postmeta
        FILTER m.post_id IN @post_ids
        RETURN m
", ['params' => ['post_ids' => $post_ids]]);

// Group by post_id
$meta_by_post = [];
foreach ($meta['items'] as $m) {
    $meta_by_post[$m['post_id']][] = $m;
}

// Total: 2 Queries (same as MySQL!)
```

**Schwierigkeit:** 🟡 **5/10**  
**Implementierung:** Query Collector Pattern

```php
class QueryCollector {
    private $collected_queries = [];
    
    public function collect($type, $params) {
        $this->collected_queries[] = ['type' => $type, 'params' => $params];
    }
    
    public function executeBatch() {
        // Group similar queries
        $batches = $this->groupQueries($this->collected_queries);
        
        // Execute as batch
        foreach ($batches as $batch) {
            $this->executeBatchQuery($batch);
        }
    }
}
```

---

## Zusammenfassung: Kritische Stolpersteine

| # | Stolperstein | Schwierigkeit | Lösbarkeit | Zeit |
|---|--------------|---------------|------------|------|
| 1 | Multi-JOIN Translation | 🔴 9/10 | ⚠️ Schwierig | 3-4 Wochen |
| 2 | Self-JOIN auf Meta Tables | 🔴 10/10 | ⚠️ Sehr schwierig | 2-3 Wochen |
| 3 | OR mit JOINs | 🔴 10/10 | ⚠️ Sehr schwierig | 2-3 Wochen |
| 4 | Savepoint Simulation | 🔴 9/10 | 🟡 Möglich mit Einschränkungen | 1-2 Wochen |
| 5 | LAST_INSERT_ID() Race | 🔴 8/10 | ✅ Lösbar mit UUID/Snowflake | 1-2 Wochen |
| 6 | N+1 Query Prevention | 🟡 5/10 | ✅ Lösbar mit Query Collector | 1 Woche |

**Gesamt:** ~11-17 Wochen nur für die kritischsten Probleme!

---

## Fazit

Ein 1:1 Ersatz ist **technisch möglich**, aber:

1. ❌ **Zu komplex** - Monate an Entwicklung
2. ❌ **Performance-Probleme** - 5-25x langsamer für viele Queries
3. ❌ **Unvollständige Abdeckung** - Viele Edge Cases
4. ❌ **Hoher Wartungsaufwand** - Jedes WordPress Update kann brechen
5. ❌ **Plugin-Inkompatibilität** - 40-60% funktionieren nicht out-of-the-box

**Empfehlung bleibt:** ✅ **Hybrid-Ansatz** (WordPress auf MySQL + ThemisDB für LLM)

---

**Erstellt:** Januar 2026  
**Status:** Technische Deep-Dive  
**Autor:** ThemisDB Team
