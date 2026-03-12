<?php
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            class-contract-manager.php                         ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 04:08:19                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     418                                            ║
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
 * Contract Manager for ThemisDB Order Request Plugin
 * Handles CRUD operations for contracts with legal compliance
 */

if (!defined('ABSPATH')) {
    exit;
}

class ThemisDB_Contract_Manager {
    
    /**
     * Create a new contract
     */
    public static function create_contract($data) {
        global $wpdb;
        
        $table_contracts = $wpdb->prefix . 'themisdb_contracts';
        
        // Generate contract number
        $contract_number = self::generate_contract_number($data['contract_type']);
        
        $contract_data = array(
            'contract_number' => $contract_number,
            'order_id' => intval($data['order_id']),
            'customer_id' => isset($data['customer_id']) ? intval($data['customer_id']) : get_current_user_id(),
            'contract_type' => sanitize_text_field($data['contract_type']),
            'contract_data' => json_encode($data['contract_data']),
            'status' => 'draft',
            'valid_from' => isset($data['valid_from']) ? $data['valid_from'] : date('Y-m-d'),
            'valid_until' => isset($data['valid_until']) ? $data['valid_until'] : null
        );
        
        $result = $wpdb->insert($table_contracts, $contract_data);
        
        if ($result) {
            $contract_id = $wpdb->insert_id;
            
            // Create initial revision
            self::create_revision($contract_id, $data['contract_data'], get_current_user_id(), 'Initial creation');
            
            return $contract_id;
        }
        
        return false;
    }
    
    /**
     * Update an existing contract
     */
    public static function update_contract($contract_id, $data, $change_reason = '') {
        global $wpdb;
        
        $table_contracts = $wpdb->prefix . 'themisdb_contracts';
        
        // Get current contract for revision
        $current_contract = self::get_contract($contract_id);
        if (!$current_contract) {
            return false;
        }
        
        $update_data = array();
        
        if (isset($data['contract_type'])) {
            $update_data['contract_type'] = sanitize_text_field($data['contract_type']);
        }
        if (isset($data['contract_data'])) {
            $update_data['contract_data'] = json_encode($data['contract_data']);
            
            // Create revision for data changes
            self::create_revision(
                $contract_id,
                $data['contract_data'],
                get_current_user_id(),
                $change_reason
            );
        }
        if (isset($data['status'])) {
            $update_data['status'] = sanitize_text_field($data['status']);
        }
        if (isset($data['signed_at'])) {
            $update_data['signed_at'] = $data['signed_at'];
        }
        if (isset($data['valid_from'])) {
            $update_data['valid_from'] = $data['valid_from'];
        }
        if (isset($data['valid_until'])) {
            $update_data['valid_until'] = $data['valid_until'];
        }
        if (isset($data['pdf_file_id'])) {
            $update_data['pdf_file_id'] = intval($data['pdf_file_id']);
        }
        if (isset($data['pdf_data'])) {
            $update_data['pdf_data'] = $data['pdf_data'];
        }
        
        if (empty($update_data)) {
            return false;
        }
        
        $result = $wpdb->update(
            $table_contracts,
            $update_data,
            array('id' => $contract_id),
            null,
            array('%d')
        );
        
        return $result !== false;
    }
    
    /**
     * Get contract by ID
     */
    public static function get_contract($contract_id) {
        global $wpdb;
        
        $table_contracts = $wpdb->prefix . 'themisdb_contracts';
        
        $contract = $wpdb->get_row($wpdb->prepare(
            "SELECT * FROM $table_contracts WHERE id = %d",
            $contract_id
        ), ARRAY_A);
        
        if ($contract && $contract['contract_data']) {
            $contract['contract_data'] = json_decode($contract['contract_data'], true);
        }
        
        return $contract;
    }
    
    /**
     * Get contract by contract number
     */
    public static function get_contract_by_number($contract_number) {
        global $wpdb;
        
        $table_contracts = $wpdb->prefix . 'themisdb_contracts';
        
        $contract = $wpdb->get_row($wpdb->prepare(
            "SELECT * FROM $table_contracts WHERE contract_number = %s",
            $contract_number
        ), ARRAY_A);
        
        if ($contract && $contract['contract_data']) {
            $contract['contract_data'] = json_decode($contract['contract_data'], true);
        }
        
        return $contract;
    }
    
    /**
     * Get contracts by order
     */
    public static function get_contracts_by_order($order_id) {
        global $wpdb;
        
        $table_contracts = $wpdb->prefix . 'themisdb_contracts';
        
        $contracts = $wpdb->get_results($wpdb->prepare(
            "SELECT * FROM $table_contracts WHERE order_id = %d ORDER BY created_at DESC",
            $order_id
        ), ARRAY_A);
        
        foreach ($contracts as &$contract) {
            if ($contract['contract_data']) {
                $contract['contract_data'] = json_decode($contract['contract_data'], true);
            }
        }
        
        return $contracts;
    }
    
    /**
     * Get contracts by customer
     */
    public static function get_customer_contracts($customer_id, $status = null) {
        global $wpdb;
        
        $table_contracts = $wpdb->prefix . 'themisdb_contracts';
        
        if ($status) {
            $contracts = $wpdb->get_results($wpdb->prepare(
                "SELECT * FROM $table_contracts WHERE customer_id = %d AND status = %s ORDER BY created_at DESC",
                $customer_id,
                $status
            ), ARRAY_A);
        } else {
            $contracts = $wpdb->get_results($wpdb->prepare(
                "SELECT * FROM $table_contracts WHERE customer_id = %d ORDER BY created_at DESC",
                $customer_id
            ), ARRAY_A);
        }
        
        foreach ($contracts as &$contract) {
            if ($contract['contract_data']) {
                $contract['contract_data'] = json_decode($contract['contract_data'], true);
            }
        }
        
        return $contracts;
    }
    
    /**
     * Get all contracts
     */
    public static function get_all_contracts($args = array()) {
        global $wpdb;
        
        $table_contracts = $wpdb->prefix . 'themisdb_contracts';
        
        $defaults = array(
            'status' => null,
            'contract_type' => null,
            'limit' => 50,
            'offset' => 0,
            'orderby' => 'created_at',
            'order' => 'DESC'
        );
        
        $args = wp_parse_args($args, $defaults);
        
        $where = "1=1";
        $where_values = array();
        
        if ($args['status']) {
            $where .= " AND status = %s";
            $where_values[] = $args['status'];
        }
        
        if ($args['contract_type']) {
            $where .= " AND contract_type = %s";
            $where_values[] = $args['contract_type'];
        }
        
        $query = "SELECT * FROM $table_contracts WHERE $where ORDER BY {$args['orderby']} {$args['order']} LIMIT %d OFFSET %d";
        $where_values[] = $args['limit'];
        $where_values[] = $args['offset'];
        
        $contracts = $wpdb->get_results($wpdb->prepare($query, $where_values), ARRAY_A);
        
        foreach ($contracts as &$contract) {
            if ($contract['contract_data']) {
                $contract['contract_data'] = json_decode($contract['contract_data'], true);
            }
        }
        
        return $contracts;
    }
    
    /**
     * Delete a contract
     */
    public static function delete_contract($contract_id) {
        global $wpdb;
        
        $table_contracts = $wpdb->prefix . 'themisdb_contracts';
        $table_revisions = $wpdb->prefix . 'themisdb_contract_revisions';
        
        // Delete revisions first
        $wpdb->delete(
            $table_revisions,
            array('contract_id' => $contract_id),
            array('%d')
        );
        
        // Delete contract
        $result = $wpdb->delete(
            $table_contracts,
            array('id' => $contract_id),
            array('%d')
        );
        
        return $result !== false;
    }
    
    /**
     * Create a contract revision
     */
    private static function create_revision($contract_id, $contract_data, $user_id, $change_reason = '') {
        global $wpdb;
        
        $table_revisions = $wpdb->prefix . 'themisdb_contract_revisions';
        
        // Get current revision number
        $last_revision = $wpdb->get_var($wpdb->prepare(
            "SELECT MAX(revision_number) FROM $table_revisions WHERE contract_id = %d",
            $contract_id
        ));
        
        $revision_number = $last_revision ? $last_revision + 1 : 1;
        
        $revision_data = array(
            'contract_id' => $contract_id,
            'revision_number' => $revision_number,
            'contract_data' => json_encode($contract_data),
            'changed_by' => $user_id,
            'change_reason' => sanitize_text_field($change_reason)
        );
        
        return $wpdb->insert($table_revisions, $revision_data);
    }
    
    /**
     * Get contract revisions
     */
    public static function get_contract_revisions($contract_id) {
        global $wpdb;
        
        $table_revisions = $wpdb->prefix . 'themisdb_contract_revisions';
        
        $revisions = $wpdb->get_results($wpdb->prepare(
            "SELECT * FROM $table_revisions WHERE contract_id = %d ORDER BY revision_number DESC",
            $contract_id
        ), ARRAY_A);
        
        foreach ($revisions as &$revision) {
            if ($revision['contract_data']) {
                $revision['contract_data'] = json_decode($revision['contract_data'], true);
            }
        }
        
        return $revisions;
    }
    
    /**
     * Generate contract number
     */
    private static function generate_contract_number($contract_type) {
        $prefix = 'CTR';
        
        // Add type prefix
        switch ($contract_type) {
            case 'license':
                $prefix = 'LIC';
                break;
            case 'service':
                $prefix = 'SRV';
                break;
            case 'support':
                $prefix = 'SUP';
                break;
            default:
                $prefix = 'CTR';
        }
        
        $date = date('Ymd');
        $random = strtoupper(substr(md5(uniqid(rand(), true)), 0, 6));
        
        return $prefix . '-' . $date . '-' . $random;
    }
    
    /**
     * Sign contract (mark as signed)
     */
    public static function sign_contract($contract_id, $signature_data = null) {
        global $wpdb;
        
        $table_contracts = $wpdb->prefix . 'themisdb_contracts';
        
        $update_data = array(
            'status' => 'signed',
            'signed_at' => current_time('mysql')
        );
        
        if ($signature_data) {
            $current_data = self::get_contract($contract_id);
            if ($current_data) {
                $contract_data = $current_data['contract_data'];
                $contract_data['signature'] = $signature_data;
                $update_data['contract_data'] = json_encode($contract_data);
            }
        }
        
        $result = $wpdb->update(
            $table_contracts,
            $update_data,
            array('id' => $contract_id),
            null,
            array('%d')
        );
        
        if ($result !== false) {
            // Create revision for signing
            self::create_revision(
                $contract_id,
                $current_data['contract_data'],
                get_current_user_id(),
                'Contract signed'
            );
        }
        
        return $result !== false;
    }
}
