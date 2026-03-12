<?php
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            class-payment-manager.php                          ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 04:08:19                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     318                                            ║
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
 * Payment Manager for ThemisDB Order Request Plugin
 * Handles payment verification and tracking
 */

if (!defined('ABSPATH')) {
    exit;
}

class ThemisDB_Payment_Manager {
    
    /**
     * Create a new payment record
     */
    public static function create_payment($data) {
        global $wpdb;
        
        $table_payments = $wpdb->prefix . 'themisdb_payments';
        
        // Generate payment number
        $payment_number = self::generate_payment_number();
        
        $payment_data = array(
            'payment_number' => $payment_number,
            'order_id' => intval($data['order_id']),
            'contract_id' => isset($data['contract_id']) ? intval($data['contract_id']) : null,
            'amount' => floatval($data['amount']),
            'currency' => isset($data['currency']) ? sanitize_text_field($data['currency']) : 'EUR',
            'payment_method' => sanitize_text_field($data['payment_method']),
            'payment_status' => 'pending',
            'transaction_id' => isset($data['transaction_id']) ? sanitize_text_field($data['transaction_id']) : null,
            'notes' => isset($data['notes']) ? sanitize_textarea_field($data['notes']) : null,
            'metadata' => isset($data['metadata']) ? json_encode($data['metadata']) : null
        );
        
        $result = $wpdb->insert($table_payments, $payment_data);
        
        if ($result) {
            return $wpdb->insert_id;
        }
        
        return false;
    }
    
    /**
     * Verify payment
     */
    public static function verify_payment($payment_id, $verified_by = null) {
        global $wpdb;
        
        $table_payments = $wpdb->prefix . 'themisdb_payments';
        
        $update_data = array(
            'payment_status' => 'verified',
            'verified_at' => current_time('mysql'),
            'verified_by' => $verified_by ? $verified_by : get_current_user_id()
        );
        
        $result = $wpdb->update(
            $table_payments,
            $update_data,
            array('id' => $payment_id),
            null,
            array('%d')
        );
        
        if ($result !== false) {
            // Get payment details
            $payment = self::get_payment($payment_id);
            
            if ($payment) {
                // Update order status
                ThemisDB_Order_Manager::update_order($payment['order_id'], array(
                    'status' => 'paid'
                ));
                
                // Activate license if exists
                if ($payment['contract_id']) {
                    $license = ThemisDB_License_Manager::get_license_by_contract($payment['contract_id']);
                    if ($license) {
                        ThemisDB_License_Manager::activate_license($license['id']);
                    }
                }
            }
        }
        
        return $result !== false;
    }
    
    /**
     * Mark payment as failed
     */
    public static function mark_payment_failed($payment_id, $reason = '') {
        global $wpdb;
        
        $table_payments = $wpdb->prefix . 'themisdb_payments';
        
        $update_data = array(
            'payment_status' => 'failed',
            'notes' => $reason
        );
        
        return $wpdb->update(
            $table_payments,
            $update_data,
            array('id' => $payment_id),
            null,
            array('%d')
        ) !== false;
    }
    
    /**
     * Get payment by ID
     */
    public static function get_payment($payment_id) {
        global $wpdb;
        
        $table_payments = $wpdb->prefix . 'themisdb_payments';
        
        $payment = $wpdb->get_row($wpdb->prepare(
            "SELECT * FROM $table_payments WHERE id = %d",
            $payment_id
        ), ARRAY_A);
        
        if ($payment && $payment['metadata']) {
            $payment['metadata'] = json_decode($payment['metadata'], true);
        }
        
        return $payment;
    }
    
    /**
     * Get payment by order
     */
    public static function get_payments_by_order($order_id) {
        global $wpdb;
        
        $table_payments = $wpdb->prefix . 'themisdb_payments';
        
        $payments = $wpdb->get_results($wpdb->prepare(
            "SELECT * FROM $table_payments WHERE order_id = %d ORDER BY created_at DESC",
            $order_id
        ), ARRAY_A);
        
        foreach ($payments as &$payment) {
            if ($payment['metadata']) {
                $payment['metadata'] = json_decode($payment['metadata'], true);
            }
        }
        
        return $payments;
    }
    
    /**
     * Get all payments
     */
    public static function get_all_payments($args = array()) {
        global $wpdb;
        
        $table_payments = $wpdb->prefix . 'themisdb_payments';
        
        $defaults = array(
            'status' => null,
            'limit' => 50,
            'offset' => 0,
            'orderby' => 'created_at',
            'order' => 'DESC'
        );
        
        $args = wp_parse_args($args, $defaults);
        
        $where = "1=1";
        $where_values = array();
        
        if ($args['status']) {
            $where .= " AND payment_status = %s";
            $where_values[] = $args['status'];
        }
        
        $query = "SELECT * FROM $table_payments WHERE $where ORDER BY {$args['orderby']} {$args['order']} LIMIT %d OFFSET %d";
        $where_values[] = $args['limit'];
        $where_values[] = $args['offset'];
        
        $payments = $wpdb->get_results($wpdb->prepare($query, $where_values), ARRAY_A);
        
        foreach ($payments as &$payment) {
            if ($payment['metadata']) {
                $payment['metadata'] = json_decode($payment['metadata'], true);
            }
        }
        
        return $payments;
    }
    
    /**
     * Check payment with epServer
     */
    public static function check_payment_with_epserver($payment_id) {
        $payment = self::get_payment($payment_id);
        
        if (!$payment || !$payment['epserver_payment_id']) {
            return false;
        }
        
        // Query epServer for payment status
        $response = ThemisDB_EPServer_API::check_payment_status($payment['epserver_payment_id']);
        
        if ($response && isset($response['status'])) {
            // Update payment status based on epServer response
            global $wpdb;
            $table_payments = $wpdb->prefix . 'themisdb_payments';
            
            $update_data = array(
                'payment_status' => $response['status']
            );
            
            if ($response['status'] === 'completed' || $response['status'] === 'verified') {
                $update_data['payment_status'] = 'verified';
                $update_data['verified_at'] = current_time('mysql');
                $update_data['payment_date'] = isset($response['payment_date']) ? $response['payment_date'] : current_time('mysql');
            }
            
            $wpdb->update(
                $table_payments,
                $update_data,
                array('id' => $payment_id),
                null,
                array('%d')
            );
            
            return $response;
        }
        
        return false;
    }
    
    /**
     * Generate payment number
     */
    private static function generate_payment_number() {
        $prefix = 'PAY';
        $date = date('Ymd');
        $random = strtoupper(bin2hex(random_bytes(3)));
        
        return $prefix . '-' . $date . '-' . $random;
    }
    
    /**
     * Get payment statistics
     */
    public static function get_payment_stats() {
        global $wpdb;
        
        $table_payments = $wpdb->prefix . 'themisdb_payments';
        
        $stats = array(
            'total_payments' => 0,
            'verified_payments' => 0,
            'pending_payments' => 0,
            'failed_payments' => 0,
            'total_amount' => 0,
            'verified_amount' => 0
        );
        
        $results = $wpdb->get_results(
            "SELECT 
                payment_status,
                COUNT(*) as count,
                SUM(amount) as total
            FROM $table_payments
            GROUP BY payment_status",
            ARRAY_A
        );
        
        foreach ($results as $row) {
            $stats['total_payments'] += $row['count'];
            $stats['total_amount'] += floatval($row['total']);
            
            switch ($row['payment_status']) {
                case 'verified':
                    $stats['verified_payments'] = $row['count'];
                    $stats['verified_amount'] = floatval($row['total']);
                    break;
                case 'pending':
                    $stats['pending_payments'] = $row['count'];
                    break;
                case 'failed':
                    $stats['failed_payments'] = $row['count'];
                    break;
            }
        }
        
        return $stats;
    }
}
