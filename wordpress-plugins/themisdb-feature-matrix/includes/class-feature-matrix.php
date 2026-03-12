<?php
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            class-feature-matrix.php                           ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 04:08:18                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     402                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 9d3ecaa0e  2026-02-28  Add ThemisDB Wiki Integration plugin with documentation i... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */


/**
 * Feature Matrix Class
 * 
 * Handles feature data and comparison logic
 */

if (!defined('ABSPATH')) {
    exit;
}

class ThemisDB_Feature_Matrix_Data {
    
    /**
     * Get all feature data
     * 
     * @return array Feature data organized by category
     */
    public static function get_features() {
        return array(
            'data_models' => array(
                'name' => 'Data Models',
                'features' => array(
                    array(
                        'name' => 'Relational SQL',
                        'themisdb' => 'full',
                        'postgresql' => 'full',
                        'mongodb' => 'limited',
                        'neo4j' => 'no',
                        'description' => 'Full SQL support with ACID transactions and complex queries',
                        'tooltip' => 'Standard SQL database capabilities with joins, transactions, and constraints'
                    ),
                    array(
                        'name' => 'Graph Database',
                        'themisdb' => 'full',
                        'postgresql' => 'limited',
                        'mongodb' => 'limited',
                        'neo4j' => 'full',
                        'description' => 'Native graph data model with efficient traversal',
                        'tooltip' => 'Store and query connected data using nodes, edges, and relationships'
                    ),
                    array(
                        'name' => 'Document Store',
                        'themisdb' => 'full',
                        'postgresql' => 'limited',
                        'mongodb' => 'full',
                        'neo4j' => 'no',
                        'description' => 'Schema-flexible JSON document storage',
                        'tooltip' => 'Store semi-structured data as documents without rigid schema'
                    ),
                    array(
                        'name' => 'Vector/Embeddings',
                        'themisdb' => 'full',
                        'postgresql' => 'limited',
                        'mongodb' => 'limited',
                        'neo4j' => 'no',
                        'description' => 'Native vector storage and similarity search',
                        'tooltip' => 'Store and query high-dimensional vectors for AI/ML applications',
                        'highlight' => true
                    ),
                    array(
                        'name' => 'Time-Series',
                        'themisdb' => 'full',
                        'postgresql' => 'limited',
                        'mongodb' => 'limited',
                        'neo4j' => 'no',
                        'description' => 'Optimized time-series data storage and queries',
                        'tooltip' => 'Efficiently store and analyze time-stamped data'
                    ),
                    array(
                        'name' => 'Key-Value',
                        'themisdb' => 'full',
                        'postgresql' => 'no',
                        'mongodb' => 'limited',
                        'neo4j' => 'no',
                        'description' => 'Simple key-value storage for caching',
                        'tooltip' => 'Fast key-value operations for caching and simple lookups'
                    )
                )
            ),
            'ai_ml' => array(
                'name' => 'AI/ML Features',
                'features' => array(
                    array(
                        'name' => 'Embedded LLM',
                        'themisdb' => 'full',
                        'postgresql' => 'no',
                        'mongodb' => 'no',
                        'neo4j' => 'no',
                        'description' => 'Run LLaMA models directly in the database',
                        'tooltip' => 'Native LLM integration with llama.cpp - no external services needed',
                        'highlight' => true,
                        'exclusive' => true
                    ),
                    array(
                        'name' => 'Vector Similarity Search',
                        'themisdb' => 'full',
                        'postgresql' => 'limited',
                        'mongodb' => 'limited',
                        'neo4j' => 'no',
                        'description' => 'HNSW algorithm for fast similarity search',
                        'tooltip' => 'High-performance nearest neighbor search for embeddings',
                        'highlight' => true
                    ),
                    array(
                        'name' => 'RAG Support',
                        'themisdb' => 'full',
                        'postgresql' => 'no',
                        'mongodb' => 'no',
                        'neo4j' => 'no',
                        'description' => 'Retrieval-Augmented Generation built-in',
                        'tooltip' => 'Native support for RAG workflows with context retrieval',
                        'highlight' => true,
                        'exclusive' => true
                    ),
                    array(
                        'name' => 'GPU Acceleration',
                        'themisdb' => 'full',
                        'postgresql' => 'no',
                        'mongodb' => 'no',
                        'neo4j' => 'no',
                        'description' => 'Hardware acceleration for ML workloads',
                        'tooltip' => 'Leverage GPU power for faster ML inference and training',
                        'highlight' => true,
                        'exclusive' => true
                    )
                )
            ),
            'performance' => array(
                'name' => 'Performance',
                'features' => array(
                    array(
                        'name' => 'Horizontal Scaling',
                        'themisdb' => 'full',
                        'postgresql' => 'limited',
                        'mongodb' => 'full',
                        'neo4j' => 'full',
                        'description' => 'Scale out across multiple nodes',
                        'tooltip' => 'Add more servers to handle increased load'
                    ),
                    array(
                        'name' => 'Auto-Sharding',
                        'themisdb' => 'full',
                        'postgresql' => 'limited',
                        'mongodb' => 'full',
                        'neo4j' => 'limited',
                        'description' => 'Automatic data partitioning',
                        'tooltip' => 'Automatically distribute data across shards'
                    ),
                    array(
                        'name' => 'Replication',
                        'themisdb' => 'full',
                        'postgresql' => 'full',
                        'mongodb' => 'full',
                        'neo4j' => 'full',
                        'description' => 'Multi-master and replica sets',
                        'tooltip' => 'Data redundancy and high availability'
                    ),
                    array(
                        'name' => 'Built-in Caching',
                        'themisdb' => 'full',
                        'postgresql' => 'limited',
                        'mongodb' => 'limited',
                        'neo4j' => 'limited',
                        'description' => 'Intelligent query result caching',
                        'tooltip' => 'Automatic caching layer for improved performance'
                    )
                )
            ),
            'compatibility' => array(
                'name' => 'Compatibility',
                'features' => array(
                    array(
                        'name' => 'SQL Protocol',
                        'themisdb' => 'full',
                        'postgresql' => 'full',
                        'mongodb' => 'no',
                        'neo4j' => 'no',
                        'description' => 'Standard SQL interface',
                        'tooltip' => 'Connect using standard SQL clients and tools'
                    ),
                    array(
                        'name' => 'MongoDB Protocol',
                        'themisdb' => 'full',
                        'postgresql' => 'no',
                        'mongodb' => 'full',
                        'neo4j' => 'no',
                        'description' => 'MongoDB wire protocol compatibility',
                        'tooltip' => 'Use MongoDB drivers and tools directly'
                    ),
                    array(
                        'name' => 'Cypher (Graph)',
                        'themisdb' => 'full',
                        'postgresql' => 'no',
                        'mongodb' => 'no',
                        'neo4j' => 'full',
                        'description' => 'Graph query language support',
                        'tooltip' => 'Query graph data using Cypher syntax'
                    ),
                    array(
                        'name' => 'REST API',
                        'themisdb' => 'full',
                        'postgresql' => 'limited',
                        'mongodb' => 'full',
                        'neo4j' => 'full',
                        'description' => 'RESTful HTTP API',
                        'tooltip' => 'Access database via standard REST endpoints'
                    ),
                    array(
                        'name' => 'GraphQL API',
                        'themisdb' => 'full',
                        'postgresql' => 'no',
                        'mongodb' => 'limited',
                        'neo4j' => 'limited',
                        'description' => 'GraphQL query interface',
                        'tooltip' => 'Modern API with GraphQL for flexible queries'
                    )
                )
            ),
            'licensing' => array(
                'name' => 'Licensing',
                'features' => array(
                    array(
                        'name' => 'License Type',
                        'themisdb' => 'MIT',
                        'postgresql' => 'PostgreSQL License',
                        'mongodb' => 'SSPL',
                        'neo4j' => 'GPL/Commercial',
                        'description' => 'Software license',
                        'tooltip' => 'Type of license governing the database use',
                        'is_text' => true
                    ),
                    array(
                        'name' => 'Free for Commercial Use',
                        'themisdb' => 'full',
                        'postgresql' => 'full',
                        'mongodb' => 'limited',
                        'neo4j' => 'limited',
                        'description' => 'Can be used commercially without fees',
                        'tooltip' => 'No licensing fees or restrictions for commercial deployment'
                    ),
                    array(
                        'name' => 'Cloud Vendor Lock-in',
                        'themisdb' => 'no',
                        'postgresql' => 'no',
                        'mongodb' => 'limited',
                        'neo4j' => 'limited',
                        'description' => 'Avoid proprietary cloud dependencies',
                        'tooltip' => 'Can be deployed anywhere without vendor restrictions',
                        'inverted' => true
                    )
                )
            )
        );
    }
    
    /**
     * Get features by category
     * 
     * @param string $category Category slug or 'all'
     * @return array Filtered features
     */
    public static function get_features_by_category($category = 'all') {
        $all_features = self::get_features();
        
        if ($category === 'all') {
            return $all_features;
        }
        
        if (isset($all_features[$category])) {
            return array($category => $all_features[$category]);
        }
        
        return array();
    }
    
    /**
     * Get flat feature list
     * 
     * @param string $category Category slug or 'all'
     * @return array Flat array of features
     */
    public static function get_flat_features($category = 'all') {
        $categorized = self::get_features_by_category($category);
        $flat = array();
        
        foreach ($categorized as $cat_slug => $cat_data) {
            foreach ($cat_data['features'] as $feature) {
                $feature['category'] = $cat_slug;
                $feature['category_name'] = $cat_data['name'];
                $flat[] = $feature;
            }
        }
        
        return $flat;
    }
    
    /**
     * Get database list
     * 
     * @return array Database information
     */
    public static function get_databases() {
        return array(
            'themisdb' => array(
                'name' => 'ThemisDB',
                'slug' => 'themisdb',
                'logo' => 'themisdb-logo.svg'
            ),
            'postgresql' => array(
                'name' => 'PostgreSQL',
                'slug' => 'postgresql',
                'logo' => 'postgresql-logo.svg'
            ),
            'mongodb' => array(
                'name' => 'MongoDB',
                'slug' => 'mongodb',
                'logo' => 'mongodb-logo.svg'
            ),
            'neo4j' => array(
                'name' => 'Neo4j',
                'slug' => 'neo4j',
                'logo' => 'neo4j-logo.svg'
            )
        );
    }
    
    /**
     * Get status label
     * 
     * @param string $status Status value
     * @param bool $inverted Whether this is an inverted metric
     * @return array Status information
     */
    public static function get_status_info($status, $inverted = false) {
        // For text values, return as-is
        if (!in_array($status, array('full', 'limited', 'no'))) {
            return array(
                'label' => $status,
                'class' => 'status-text',
                'icon' => '',
                'score' => 0
            );
        }
        
        $status_map = array(
            'full' => array(
                'label' => 'Full Support',
                'class' => 'status-full',
                'icon' => '✓',
                'score' => 2
            ),
            'limited' => array(
                'label' => 'Limited Support',
                'class' => 'status-limited',
                'icon' => '◐',
                'score' => 1
            ),
            'no' => array(
                'label' => 'Not Available',
                'class' => 'status-no',
                'icon' => '✗',
                'score' => 0
            )
        );
        
        $info = isset($status_map[$status]) ? $status_map[$status] : $status_map['no'];
        
        // For inverted metrics, flip the class
        if ($inverted) {
            if ($status === 'full') {
                $info['class'] = 'status-no';
            } elseif ($status === 'no') {
                $info['class'] = 'status-full';
            }
        }
        
        return $info;
    }
}
