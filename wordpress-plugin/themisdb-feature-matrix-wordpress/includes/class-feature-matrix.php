<?php
/**
 * ThemisDB Feature Matrix Core Class
 * 
 * Handles feature data structure and retrieval
 */

if (!defined('ABSPATH')) {
    exit;
}

class ThemisDB_Feature_Matrix_Core {
    
    /**
     * Get all feature data
     * 
     * @return array Complete feature data structure
     */
    public static function get_features() {
        return array(
            'data_models' => array(
                'name' => 'Data Models',
                'features' => array(
                    'relational' => array(
                        'name' => 'Relational (SQL)',
                        'themisdb' => 'full',
                        'postgresql' => 'full',
                        'mongodb' => 'limited',
                        'neo4j' => 'no',
                        'tooltip' => 'Full SQL support with ACID transactions'
                    ),
                    'graph' => array(
                        'name' => 'Graph Database',
                        'themisdb' => 'full',
                        'postgresql' => 'limited',
                        'mongodb' => 'limited',
                        'neo4j' => 'full',
                        'tooltip' => 'Native graph storage and Cypher query support'
                    ),
                    'document' => array(
                        'name' => 'Document (NoSQL)',
                        'themisdb' => 'full',
                        'postgresql' => 'limited',
                        'mongodb' => 'full',
                        'neo4j' => 'no',
                        'tooltip' => 'Schema-less JSON document storage'
                    ),
                    'vector' => array(
                        'name' => 'Vector/Embeddings',
                        'themisdb' => 'full',
                        'postgresql' => 'limited',
                        'mongodb' => 'limited',
                        'neo4j' => 'no',
                        'tooltip' => 'Native vector storage for AI/ML embeddings',
                        'highlight' => true
                    ),
                    'timeseries' => array(
                        'name' => 'Time-Series',
                        'themisdb' => 'full',
                        'postgresql' => 'limited',
                        'mongodb' => 'limited',
                        'neo4j' => 'no'
                    ),
                    'key_value' => array(
                        'name' => 'Key-Value Store',
                        'themisdb' => 'full',
                        'postgresql' => 'no',
                        'mongodb' => 'limited',
                        'neo4j' => 'no'
                    )
                )
            ),
            'ai_ml' => array(
                'name' => 'AI/ML Features',
                'features' => array(
                    'embedded_llm' => array(
                        'name' => 'Embedded LLM',
                        'themisdb' => 'full',
                        'postgresql' => 'no',
                        'mongodb' => 'no',
                        'neo4j' => 'no',
                        'highlight' => true,
                        'tooltip' => 'Built-in LLM via llama.cpp - no external API needed'
                    ),
                    'vector_search' => array(
                        'name' => 'Vector Similarity Search',
                        'themisdb' => 'full',
                        'postgresql' => 'limited',
                        'mongodb' => 'limited',
                        'neo4j' => 'no'
                    ),
                    'rag_support' => array(
                        'name' => 'RAG (Retrieval-Augmented Generation)',
                        'themisdb' => 'full',
                        'postgresql' => 'no',
                        'mongodb' => 'no',
                        'neo4j' => 'no',
                        'highlight' => true
                    ),
                    'gpu_acceleration' => array(
                        'name' => 'GPU Acceleration',
                        'themisdb' => 'full',
                        'postgresql' => 'no',
                        'mongodb' => 'no',
                        'neo4j' => 'no',
                        'highlight' => true
                    )
                )
            ),
            'performance' => array(
                'name' => 'Performance & Scaling',
                'features' => array(
                    'horizontal_scaling' => array(
                        'name' => 'Horizontal Scaling',
                        'themisdb' => 'full',
                        'postgresql' => 'limited',
                        'mongodb' => 'full',
                        'neo4j' => 'full'
                    ),
                    'sharding' => array(
                        'name' => 'Auto-Sharding',
                        'themisdb' => 'full',
                        'postgresql' => 'limited',
                        'mongodb' => 'full',
                        'neo4j' => 'limited'
                    ),
                    'replication' => array(
                        'name' => 'Replication',
                        'themisdb' => 'full',
                        'postgresql' => 'full',
                        'mongodb' => 'full',
                        'neo4j' => 'full'
                    ),
                    'caching' => array(
                        'name' => 'Built-in Caching',
                        'themisdb' => 'full',
                        'postgresql' => 'limited',
                        'mongodb' => 'full',
                        'neo4j' => 'limited'
                    )
                )
            ),
            'compatibility' => array(
                'name' => 'Protocol Compatibility',
                'features' => array(
                    'sql_protocol' => array(
                        'name' => 'SQL Protocol',
                        'themisdb' => 'full',
                        'postgresql' => 'full',
                        'mongodb' => 'no',
                        'neo4j' => 'no'
                    ),
                    'mongodb_protocol' => array(
                        'name' => 'MongoDB Protocol',
                        'themisdb' => 'full',
                        'postgresql' => 'no',
                        'mongodb' => 'full',
                        'neo4j' => 'no'
                    ),
                    'cypher' => array(
                        'name' => 'Cypher (Graph)',
                        'themisdb' => 'full',
                        'postgresql' => 'no',
                        'mongodb' => 'no',
                        'neo4j' => 'full'
                    ),
                    'rest_api' => array(
                        'name' => 'REST API',
                        'themisdb' => 'full',
                        'postgresql' => 'limited',
                        'mongodb' => 'full',
                        'neo4j' => 'full'
                    ),
                    'graphql' => array(
                        'name' => 'GraphQL API',
                        'themisdb' => 'full',
                        'postgresql' => 'limited',
                        'mongodb' => 'limited',
                        'neo4j' => 'full'
                    )
                )
            ),
            'pricing' => array(
                'name' => 'Licensing & Cost',
                'features' => array(
                    'license' => array(
                        'name' => 'License',
                        'themisdb' => 'MIT',
                        'postgresql' => 'PostgreSQL',
                        'mongodb' => 'SSPL',
                        'neo4j' => 'GPL/Comm.',
                        'display_text' => true
                    ),
                    'commercial_use' => array(
                        'name' => 'Free for Commercial Use',
                        'themisdb' => 'full',
                        'postgresql' => 'full',
                        'mongodb' => 'limited',
                        'neo4j' => 'limited'
                    )
                )
            )
        );
    }
    
    /**
     * Get filtered features by category
     * 
     * @param string $category Category filter (all, data_models, ai_ml, etc.)
     * @return array Filtered feature data
     */
    public static function get_filtered_features($category = 'all') {
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
     * Get status display info
     * 
     * @param string $status Status code (full, limited, no)
     * @return array Status information
     */
    public static function get_status_info($status) {
        $status_map = array(
            'full' => array(
                'icon' => '✓',
                'class' => 'status-full',
                'label' => 'Full Support'
            ),
            'limited' => array(
                'icon' => '◐',
                'class' => 'status-limited',
                'label' => 'Limited Support'
            ),
            'no' => array(
                'icon' => '✗',
                'class' => 'status-no',
                'label' => 'No Support'
            )
        );
        
        return isset($status_map[$status]) ? $status_map[$status] : $status_map['no'];
    }
}
