<?php
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            class-database.php                                 ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 04:08:19                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     419                                            ║
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
 * Database handler for ThemisDB Order Request Plugin
 */

if (!defined('ABSPATH')) {
    exit;
}

class ThemisDB_Order_Database {
    
    /**
     * Initialize database
     */
    public static function init() {
        // Nothing to do on init
    }
    
    /**
     * Create database tables
     */
    public static function create_tables() {
        global $wpdb;
        
        $charset_collate = $wpdb->get_charset_collate();
        
        // Orders table
        $table_orders = $wpdb->prefix . 'themisdb_orders';
        $sql_orders = "CREATE TABLE IF NOT EXISTS $table_orders (
            id bigint(20) NOT NULL AUTO_INCREMENT,
            order_number varchar(50) NOT NULL UNIQUE,
            customer_id bigint(20) NOT NULL,
            customer_email varchar(100) NOT NULL,
            customer_name varchar(255) NOT NULL,
            customer_company varchar(255) DEFAULT NULL,
            product_type varchar(50) NOT NULL,
            product_edition varchar(50) NOT NULL,
            modules longtext DEFAULT NULL,
            training_modules longtext DEFAULT NULL,
            total_amount decimal(10,2) NOT NULL DEFAULT 0.00,
            currency varchar(10) NOT NULL DEFAULT 'EUR',
            status varchar(50) NOT NULL DEFAULT 'draft',
            step int(11) NOT NULL DEFAULT 1,
            created_at datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
            updated_at datetime NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
            PRIMARY KEY  (id),
            KEY customer_id (customer_id),
            KEY status (status),
            KEY created_at (created_at)
        ) $charset_collate;";
        
        // Contracts table
        $table_contracts = $wpdb->prefix . 'themisdb_contracts';
        $sql_contracts = "CREATE TABLE IF NOT EXISTS $table_contracts (
            id bigint(20) NOT NULL AUTO_INCREMENT,
            contract_number varchar(50) NOT NULL UNIQUE,
            order_id bigint(20) NOT NULL,
            customer_id bigint(20) NOT NULL,
            contract_type varchar(50) NOT NULL,
            contract_data longtext NOT NULL,
            pdf_file_id bigint(20) DEFAULT NULL,
            pdf_data longblob DEFAULT NULL,
            status varchar(50) NOT NULL DEFAULT 'draft',
            signed_at datetime DEFAULT NULL,
            valid_from date DEFAULT NULL,
            valid_until date DEFAULT NULL,
            created_at datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
            updated_at datetime NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
            PRIMARY KEY  (id),
            KEY order_id (order_id),
            KEY customer_id (customer_id),
            KEY status (status),
            KEY valid_from (valid_from),
            KEY valid_until (valid_until)
        ) $charset_collate;";
        
        // Contract revisions table (for legal compliance)
        $table_revisions = $wpdb->prefix . 'themisdb_contract_revisions';
        $sql_revisions = "CREATE TABLE IF NOT EXISTS $table_revisions (
            id bigint(20) NOT NULL AUTO_INCREMENT,
            contract_id bigint(20) NOT NULL,
            revision_number int(11) NOT NULL,
            contract_data longtext NOT NULL,
            pdf_data longblob DEFAULT NULL,
            changed_by bigint(20) NOT NULL,
            change_reason text DEFAULT NULL,
            created_at datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
            PRIMARY KEY  (id),
            KEY contract_id (contract_id),
            KEY created_at (created_at)
        ) $charset_collate;";
        
        // Products master data (synced with epServer)
        $table_products = $wpdb->prefix . 'themisdb_products';
        $sql_products = "CREATE TABLE IF NOT EXISTS $table_products (
            id bigint(20) NOT NULL AUTO_INCREMENT,
            product_code varchar(50) NOT NULL UNIQUE,
            product_name varchar(255) NOT NULL,
            product_type varchar(50) NOT NULL,
            edition varchar(50) NOT NULL,
            description text DEFAULT NULL,
            price decimal(10,2) NOT NULL,
            currency varchar(10) NOT NULL DEFAULT 'EUR',
            is_active tinyint(1) NOT NULL DEFAULT 1,
            epserver_id varchar(100) DEFAULT NULL,
            metadata longtext DEFAULT NULL,
            created_at datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
            updated_at datetime NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
            PRIMARY KEY  (id),
            KEY product_type (product_type),
            KEY edition (edition),
            KEY is_active (is_active)
        ) $charset_collate;";
        
        // Modules master data
        $table_modules = $wpdb->prefix . 'themisdb_modules';
        $sql_modules = "CREATE TABLE IF NOT EXISTS $table_modules (
            id bigint(20) NOT NULL AUTO_INCREMENT,
            module_code varchar(50) NOT NULL UNIQUE,
            module_name varchar(255) NOT NULL,
            module_category varchar(50) NOT NULL,
            description text DEFAULT NULL,
            price decimal(10,2) NOT NULL,
            currency varchar(10) NOT NULL DEFAULT 'EUR',
            is_active tinyint(1) NOT NULL DEFAULT 1,
            epserver_id varchar(100) DEFAULT NULL,
            metadata longtext DEFAULT NULL,
            created_at datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
            updated_at datetime NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
            PRIMARY KEY  (id),
            KEY module_category (module_category),
            KEY is_active (is_active)
        ) $charset_collate;";
        
        // Training modules master data
        $table_training = $wpdb->prefix . 'themisdb_training_modules';
        $sql_training = "CREATE TABLE IF NOT EXISTS $table_training (
            id bigint(20) NOT NULL AUTO_INCREMENT,
            training_code varchar(50) NOT NULL UNIQUE,
            training_name varchar(255) NOT NULL,
            training_type varchar(50) NOT NULL,
            duration_hours int(11) DEFAULT NULL,
            description text DEFAULT NULL,
            price decimal(10,2) NOT NULL,
            currency varchar(10) NOT NULL DEFAULT 'EUR',
            is_active tinyint(1) NOT NULL DEFAULT 1,
            epserver_id varchar(100) DEFAULT NULL,
            metadata longtext DEFAULT NULL,
            created_at datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
            updated_at datetime NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
            PRIMARY KEY  (id),
            KEY training_type (training_type),
            KEY is_active (is_active)
        ) $charset_collate;";
        
        // Email log table
        $table_email_log = $wpdb->prefix . 'themisdb_email_log';
        $sql_email_log = "CREATE TABLE IF NOT EXISTS $table_email_log (
            id bigint(20) NOT NULL AUTO_INCREMENT,
            order_id bigint(20) DEFAULT NULL,
            contract_id bigint(20) DEFAULT NULL,
            recipient varchar(255) NOT NULL,
            subject varchar(255) NOT NULL,
            body longtext NOT NULL,
            attachments longtext DEFAULT NULL,
            status varchar(50) NOT NULL DEFAULT 'pending',
            sent_at datetime DEFAULT NULL,
            error_message text DEFAULT NULL,
            created_at datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
            PRIMARY KEY  (id),
            KEY order_id (order_id),
            KEY contract_id (contract_id),
            KEY status (status),
            KEY created_at (created_at)
        ) $charset_collate;";
        
        // Payments table
        $table_payments = $wpdb->prefix . 'themisdb_payments';
        $sql_payments = "CREATE TABLE IF NOT EXISTS $table_payments (
            id bigint(20) NOT NULL AUTO_INCREMENT,
            payment_number varchar(50) NOT NULL UNIQUE,
            order_id bigint(20) NOT NULL,
            contract_id bigint(20) DEFAULT NULL,
            amount decimal(10,2) NOT NULL,
            currency varchar(10) NOT NULL DEFAULT 'EUR',
            payment_method varchar(50) NOT NULL,
            payment_status varchar(50) NOT NULL DEFAULT 'pending',
            transaction_id varchar(255) DEFAULT NULL,
            payment_date datetime DEFAULT NULL,
            verified_at datetime DEFAULT NULL,
            verified_by bigint(20) DEFAULT NULL,
            notes text DEFAULT NULL,
            epserver_payment_id varchar(100) DEFAULT NULL,
            metadata longtext DEFAULT NULL,
            created_at datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
            updated_at datetime NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
            PRIMARY KEY  (id),
            KEY order_id (order_id),
            KEY contract_id (contract_id),
            KEY payment_status (payment_status),
            KEY payment_date (payment_date),
            KEY created_at (created_at)
        ) $charset_collate;";
        
        // Licenses table
        $table_licenses = $wpdb->prefix . 'themisdb_licenses';
        $sql_licenses = "CREATE TABLE IF NOT EXISTS $table_licenses (
            id bigint(20) NOT NULL AUTO_INCREMENT,
            license_key varchar(255) NOT NULL UNIQUE,
            order_id bigint(20) NOT NULL,
            contract_id bigint(20) NOT NULL,
            customer_id bigint(20) NOT NULL,
            product_edition varchar(50) NOT NULL,
            license_type varchar(50) NOT NULL DEFAULT 'standard',
            max_nodes int(11) DEFAULT 1,
            max_cores int(11) DEFAULT NULL,
            max_storage_gb int(11) DEFAULT NULL,
            license_status varchar(50) NOT NULL DEFAULT 'pending',
            activation_date datetime DEFAULT NULL,
            expiry_date datetime DEFAULT NULL,
            last_check datetime DEFAULT NULL,
            usage_data longtext DEFAULT NULL,
            license_file_path varchar(255) DEFAULT NULL,
            license_file_data longtext DEFAULT NULL,
            epserver_subscription_id varchar(100) DEFAULT NULL,
            created_at datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
            updated_at datetime NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
            PRIMARY KEY  (id),
            KEY order_id (order_id),
            KEY contract_id (contract_id),
            KEY customer_id (customer_id),
            KEY license_status (license_status),
            KEY expiry_date (expiry_date),
            KEY created_at (created_at)
        ) $charset_collate;";
        
        // License authentication log
        $table_license_auth = $wpdb->prefix . 'themisdb_license_auth_log';
        $sql_license_auth = "CREATE TABLE IF NOT EXISTS $table_license_auth (
            id bigint(20) NOT NULL AUTO_INCREMENT,
            license_id bigint(20) NOT NULL,
            auth_method varchar(50) NOT NULL,
            auth_status varchar(50) NOT NULL,
            ip_address varchar(45) DEFAULT NULL,
            user_agent text DEFAULT NULL,
            auth_data longtext DEFAULT NULL,
            created_at datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
            PRIMARY KEY  (id),
            KEY license_id (license_id),
            KEY auth_status (auth_status),
            KEY created_at (created_at)
        ) $charset_collate;";
        
        require_once(ABSPATH . 'wp-admin/includes/upgrade.php');
        
        dbDelta($sql_orders);
        dbDelta($sql_contracts);
        dbDelta($sql_revisions);
        dbDelta($sql_products);
        dbDelta($sql_modules);
        dbDelta($sql_training);
        dbDelta($sql_email_log);
        dbDelta($sql_payments);
        dbDelta($sql_licenses);
        dbDelta($sql_license_auth);
        
        // Insert default product data
        self::insert_default_data();
    }
    
    /**
     * Insert default product data
     */
    private static function insert_default_data() {
        global $wpdb;
        
        $table_products = $wpdb->prefix . 'themisdb_products';
        
        // Check if data already exists
        $count = $wpdb->get_var("SELECT COUNT(*) FROM $table_products");
        if ($count > 0) {
            return;
        }
        
        // Default ThemisDB products
        $products = array(
            array(
                'product_code' => 'THEMIS-COMMUNITY',
                'product_name' => 'ThemisDB Community Edition',
                'product_type' => 'database',
                'edition' => 'community',
                'description' => 'Kostenlose Single-Node Edition',
                'price' => 0.00,
                'currency' => 'EUR'
            ),
            array(
                'product_code' => 'THEMIS-ENTERPRISE',
                'product_name' => 'ThemisDB Enterprise Edition',
                'product_type' => 'database',
                'edition' => 'enterprise',
                'description' => 'Enterprise Edition bis 100 Nodes',
                'price' => 5000.00,
                'currency' => 'EUR'
            ),
            array(
                'product_code' => 'THEMIS-HYPERSCALER',
                'product_name' => 'ThemisDB Hyperscaler Edition',
                'product_type' => 'database',
                'edition' => 'hyperscaler',
                'description' => 'Hyperscaler Edition unbegrenzte Nodes',
                'price' => 25000.00,
                'currency' => 'EUR'
            )
        );
        
        foreach ($products as $product) {
            $wpdb->insert($table_products, $product);
        }
        
        // Default modules
        $table_modules = $wpdb->prefix . 'themisdb_modules';
        $modules = array(
            array(
                'module_code' => 'MOD-VECTOR-SEARCH',
                'module_name' => 'Vector Search & HNSW',
                'module_category' => 'ai-ml',
                'description' => 'Hochleistungs-Vektorsuche mit HNSW-Index',
                'price' => 0.00,
                'currency' => 'EUR'
            ),
            array(
                'module_code' => 'MOD-LLM-INTEGRATION',
                'module_name' => 'LLM Integration (llama.cpp)',
                'module_category' => 'ai-ml',
                'description' => 'Embedded LLM-Integration ohne API-Kosten',
                'price' => 0.00,
                'currency' => 'EUR'
            ),
            array(
                'module_code' => 'MOD-GRAPH-DB',
                'module_name' => 'Graph Database',
                'module_category' => 'storage',
                'description' => 'Native Graph-Datenbank-Funktionalität',
                'price' => 0.00,
                'currency' => 'EUR'
            ),
            array(
                'module_code' => 'MOD-SHARDING',
                'module_name' => 'Sharding & RAID',
                'module_category' => 'scaling',
                'description' => 'Horizontale Skalierung mit RAID 0/1/5',
                'price' => 1000.00,
                'currency' => 'EUR'
            )
        );
        
        foreach ($modules as $module) {
            $wpdb->insert($table_modules, $module);
        }
        
        // Default training modules
        $table_training = $wpdb->prefix . 'themisdb_training_modules';
        $trainings = array(
            array(
                'training_code' => 'TRAIN-BASIC',
                'training_name' => 'ThemisDB Grundlagen',
                'training_type' => 'online',
                'duration_hours' => 4,
                'description' => 'Einführung in ThemisDB und grundlegende Konzepte',
                'price' => 500.00,
                'currency' => 'EUR'
            ),
            array(
                'training_code' => 'TRAIN-ADMIN',
                'training_name' => 'ThemisDB Administration',
                'training_type' => 'onsite',
                'duration_hours' => 8,
                'description' => 'Administration und Wartung von ThemisDB',
                'price' => 2000.00,
                'currency' => 'EUR'
            ),
            array(
                'training_code' => 'TRAIN-DEVELOPER',
                'training_name' => 'ThemisDB für Entwickler',
                'training_type' => 'online',
                'duration_hours' => 6,
                'description' => 'API-Nutzung und Entwicklung mit ThemisDB',
                'price' => 1500.00,
                'currency' => 'EUR'
            )
        );
        
        foreach ($trainings as $training) {
            $wpdb->insert($table_training, $training);
        }
    }
}
