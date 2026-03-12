<?php
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            class-order-manager.php                            ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 04:08:19                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     385                                            ║
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
 * Order Manager for ThemisDB Order Request Plugin
 */

if (!defined('ABSPATH')) {
    exit;
}

class ThemisDB_Order_Manager {
    
    /**
     * Create a new order
     */
    public static function create_order($data) {
        global $wpdb;
        
        $table_orders = $wpdb->prefix . 'themisdb_orders';
        
        // Generate order number
        $order_number = self::generate_order_number();
        
        $order_data = array(
            'order_number' => $order_number,
            'customer_id' => isset($data['customer_id']) ? intval($data['customer_id']) : get_current_user_id(),
            'customer_email' => sanitize_email($data['customer_email']),
            'customer_name' => sanitize_text_field($data['customer_name']),
            'customer_company' => isset($data['customer_company']) ? sanitize_text_field($data['customer_company']) : null,
            'product_type' => sanitize_text_field($data['product_type']),
            'product_edition' => sanitize_text_field($data['product_edition']),
            'modules' => isset($data['modules']) ? json_encode($data['modules']) : null,
            'training_modules' => isset($data['training_modules']) ? json_encode($data['training_modules']) : null,
            'total_amount' => isset($data['total_amount']) ? floatval($data['total_amount']) : 0.00,
            'currency' => isset($data['currency']) ? sanitize_text_field($data['currency']) : 'EUR',
            'status' => 'draft',
            'step' => 1
        );
        
        $result = $wpdb->insert($table_orders, $order_data);
        
        if ($result) {
            return $wpdb->insert_id;
        }
        
        return false;
    }
    
    /**
     * Update an existing order
     */
    public static function update_order($order_id, $data) {
        global $wpdb;
        
        $table_orders = $wpdb->prefix . 'themisdb_orders';
        
        $update_data = array();
        
        if (isset($data['customer_email'])) {
            $update_data['customer_email'] = sanitize_email($data['customer_email']);
        }
        if (isset($data['customer_name'])) {
            $update_data['customer_name'] = sanitize_text_field($data['customer_name']);
        }
        if (isset($data['customer_company'])) {
            $update_data['customer_company'] = sanitize_text_field($data['customer_company']);
        }
        if (isset($data['product_type'])) {
            $update_data['product_type'] = sanitize_text_field($data['product_type']);
        }
        if (isset($data['product_edition'])) {
            $update_data['product_edition'] = sanitize_text_field($data['product_edition']);
        }
        if (isset($data['modules'])) {
            $update_data['modules'] = json_encode($data['modules']);
        }
        if (isset($data['training_modules'])) {
            $update_data['training_modules'] = json_encode($data['training_modules']);
        }
        if (isset($data['total_amount'])) {
            $update_data['total_amount'] = floatval($data['total_amount']);
        }
        if (isset($data['currency'])) {
            $update_data['currency'] = sanitize_text_field($data['currency']);
        }
        if (isset($data['status'])) {
            $update_data['status'] = sanitize_text_field($data['status']);
        }
        if (isset($data['step'])) {
            $update_data['step'] = intval($data['step']);
        }
        
        if (empty($update_data)) {
            return false;
        }
        
        $result = $wpdb->update(
            $table_orders,
            $update_data,
            array('id' => $order_id),
            null,
            array('%d')
        );
        
        return $result !== false;
    }
    
    /**
     * Get order by ID
     */
    public static function get_order($order_id) {
        global $wpdb;
        
        $table_orders = $wpdb->prefix . 'themisdb_orders';
        
        $order = $wpdb->get_row($wpdb->prepare(
            "SELECT * FROM $table_orders WHERE id = %d",
            $order_id
        ), ARRAY_A);
        
        if ($order) {
            // Decode JSON fields
            if ($order['modules']) {
                $order['modules'] = json_decode($order['modules'], true);
            }
            if ($order['training_modules']) {
                $order['training_modules'] = json_decode($order['training_modules'], true);
            }
        }
        
        return $order;
    }
    
    /**
     * Get order by order number
     */
    public static function get_order_by_number($order_number) {
        global $wpdb;
        
        $table_orders = $wpdb->prefix . 'themisdb_orders';
        
        $order = $wpdb->get_row($wpdb->prepare(
            "SELECT * FROM $table_orders WHERE order_number = %s",
            $order_number
        ), ARRAY_A);
        
        if ($order) {
            // Decode JSON fields
            if ($order['modules']) {
                $order['modules'] = json_decode($order['modules'], true);
            }
            if ($order['training_modules']) {
                $order['training_modules'] = json_decode($order['training_modules'], true);
            }
        }
        
        return $order;
    }
    
    /**
     * Get orders by customer
     */
    public static function get_customer_orders($customer_id, $status = null) {
        global $wpdb;
        
        $table_orders = $wpdb->prefix . 'themisdb_orders';
        
        if ($status) {
            $orders = $wpdb->get_results($wpdb->prepare(
                "SELECT * FROM $table_orders WHERE customer_id = %d AND status = %s ORDER BY created_at DESC",
                $customer_id,
                $status
            ), ARRAY_A);
        } else {
            $orders = $wpdb->get_results($wpdb->prepare(
                "SELECT * FROM $table_orders WHERE customer_id = %d ORDER BY created_at DESC",
                $customer_id
            ), ARRAY_A);
        }
        
        foreach ($orders as &$order) {
            if ($order['modules']) {
                $order['modules'] = json_decode($order['modules'], true);
            }
            if ($order['training_modules']) {
                $order['training_modules'] = json_decode($order['training_modules'], true);
            }
        }
        
        return $orders;
    }
    
    /**
     * Get all orders
     */
    public static function get_all_orders($args = array()) {
        global $wpdb;
        
        $table_orders = $wpdb->prefix . 'themisdb_orders';
        
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
            $where .= " AND status = %s";
            $where_values[] = $args['status'];
        }
        
        $query = "SELECT * FROM $table_orders WHERE $where ORDER BY {$args['orderby']} {$args['order']} LIMIT %d OFFSET %d";
        $where_values[] = $args['limit'];
        $where_values[] = $args['offset'];
        
        $orders = $wpdb->get_results($wpdb->prepare($query, $where_values), ARRAY_A);
        
        foreach ($orders as &$order) {
            if ($order['modules']) {
                $order['modules'] = json_decode($order['modules'], true);
            }
            if ($order['training_modules']) {
                $order['training_modules'] = json_decode($order['training_modules'], true);
            }
        }
        
        return $orders;
    }
    
    /**
     * Delete an order
     */
    public static function delete_order($order_id) {
        global $wpdb;
        
        $table_orders = $wpdb->prefix . 'themisdb_orders';
        
        $result = $wpdb->delete(
            $table_orders,
            array('id' => $order_id),
            array('%d')
        );
        
        return $result !== false;
    }
    
    /**
     * Calculate order total
     */
    public static function calculate_total($product_edition, $modules = array(), $training_modules = array()) {
        global $wpdb;
        
        $total = 0.00;
        
        // Get product price
        $table_products = $wpdb->prefix . 'themisdb_products';
        $product = $wpdb->get_row($wpdb->prepare(
            "SELECT price FROM $table_products WHERE edition = %s AND is_active = 1",
            $product_edition
        ));
        
        if ($product) {
            $total += floatval($product->price);
        }
        
        // Get modules prices
        if (!empty($modules)) {
            $table_modules = $wpdb->prefix . 'themisdb_modules';
            $placeholders = implode(',', array_fill(0, count($modules), '%s'));
            $query = "SELECT SUM(price) as total FROM $table_modules WHERE module_code IN ($placeholders) AND is_active = 1";
            $module_total = $wpdb->get_var($wpdb->prepare($query, $modules));
            if ($module_total) {
                $total += floatval($module_total);
            }
        }
        
        // Get training prices
        if (!empty($training_modules)) {
            $table_training = $wpdb->prefix . 'themisdb_training_modules';
            $placeholders = implode(',', array_fill(0, count($training_modules), '%s'));
            $query = "SELECT SUM(price) as total FROM $table_training WHERE training_code IN ($placeholders) AND is_active = 1";
            $training_total = $wpdb->get_var($wpdb->prepare($query, $training_modules));
            if ($training_total) {
                $total += floatval($training_total);
            }
        }
        
        return $total;
    }
    
    /**
     * Generate order number
     */
    private static function generate_order_number() {
        $prefix = 'TDB';
        $date = date('Ymd');
        $random = strtoupper(substr(md5(uniqid(rand(), true)), 0, 6));
        
        return $prefix . '-' . $date . '-' . $random;
    }
    
    /**
     * Get products
     */
    public static function get_products() {
        global $wpdb;
        
        $table_products = $wpdb->prefix . 'themisdb_products';
        
        return $wpdb->get_results(
            "SELECT * FROM $table_products WHERE is_active = 1 ORDER BY price ASC",
            ARRAY_A
        );
    }
    
    /**
     * Get modules
     */
    public static function get_modules($category = null) {
        global $wpdb;
        
        $table_modules = $wpdb->prefix . 'themisdb_modules';
        
        if ($category) {
            return $wpdb->get_results($wpdb->prepare(
                "SELECT * FROM $table_modules WHERE is_active = 1 AND module_category = %s ORDER BY module_name ASC",
                $category
            ), ARRAY_A);
        }
        
        return $wpdb->get_results(
            "SELECT * FROM $table_modules WHERE is_active = 1 ORDER BY module_category, module_name ASC",
            ARRAY_A
        );
    }
    
    /**
     * Get training modules
     */
    public static function get_training_modules($type = null) {
        global $wpdb;
        
        $table_training = $wpdb->prefix . 'themisdb_training_modules';
        
        if ($type) {
            return $wpdb->get_results($wpdb->prepare(
                "SELECT * FROM $table_training WHERE is_active = 1 AND training_type = %s ORDER BY training_name ASC",
                $type
            ), ARRAY_A);
        }
        
        return $wpdb->get_results(
            "SELECT * FROM $table_training WHERE is_active = 1 ORDER BY training_type, training_name ASC",
            ARRAY_A
        );
    }
}
