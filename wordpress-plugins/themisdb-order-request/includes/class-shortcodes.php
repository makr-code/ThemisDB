<?php
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            class-shortcodes.php                               ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 04:08:20                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     668                                            ║
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
 * Shortcodes for ThemisDB Order Request Plugin
 * Provides dialog-based order flow
 */

if (!defined('ABSPATH')) {
    exit;
}

class ThemisDB_Order_Shortcodes {
    
    public function __construct() {
        add_shortcode('themisdb_order_flow', array($this, 'order_flow_shortcode'));
        add_shortcode('themisdb_my_orders', array($this, 'my_orders_shortcode'));
        add_shortcode('themisdb_my_contracts', array($this, 'my_contracts_shortcode'));
        
        // AJAX handlers
        add_action('wp_ajax_themisdb_save_order_step', array($this, 'ajax_save_order_step'));
        add_action('wp_ajax_nopriv_themisdb_save_order_step', array($this, 'ajax_save_order_step'));
        add_action('wp_ajax_themisdb_calculate_total', array($this, 'ajax_calculate_total'));
        add_action('wp_ajax_nopriv_themisdb_calculate_total', array($this, 'ajax_calculate_total'));
        add_action('wp_ajax_themisdb_submit_order', array($this, 'ajax_submit_order'));
        add_action('wp_ajax_nopriv_themisdb_submit_order', array($this, 'ajax_submit_order'));
    }
    
    /**
     * Order flow shortcode - Dialog-based order process
     */
    public function order_flow_shortcode($atts) {
        $atts = shortcode_atts(array(
            'step' => 1
        ), $atts);
        
        ob_start();
        $this->render_order_flow(intval($atts['step']));
        return ob_get_clean();
    }
    
    /**
     * Render order flow
     */
    private function render_order_flow($current_step = 1) {
        // Get or create order session
        $order_id = isset($_SESSION['themisdb_order_id']) ? $_SESSION['themisdb_order_id'] : null;
        $order = null;
        
        if ($order_id) {
            $order = ThemisDB_Order_Manager::get_order($order_id);
            if ($order) {
                $current_step = $order['step'];
            }
        }
        
        ?>
        <div class="themisdb-order-flow">
            <!-- Progress Steps -->
            <div class="order-steps">
                <div class="step <?php echo $current_step >= 1 ? 'active' : ''; ?> <?php echo $current_step > 1 ? 'completed' : ''; ?>">
                    <span class="step-number">1</span>
                    <span class="step-title"><?php _e('Produkt wählen', 'themisdb-order-request'); ?></span>
                </div>
                <div class="step <?php echo $current_step >= 2 ? 'active' : ''; ?> <?php echo $current_step > 2 ? 'completed' : ''; ?>">
                    <span class="step-number">2</span>
                    <span class="step-title"><?php _e('Module auswählen', 'themisdb-order-request'); ?></span>
                </div>
                <div class="step <?php echo $current_step >= 3 ? 'active' : ''; ?> <?php echo $current_step > 3 ? 'completed' : ''; ?>">
                    <span class="step-number">3</span>
                    <span class="step-title"><?php _e('Schulungen', 'themisdb-order-request'); ?></span>
                </div>
                <div class="step <?php echo $current_step >= 4 ? 'active' : ''; ?> <?php echo $current_step > 4 ? 'completed' : ''; ?>">
                    <span class="step-number">4</span>
                    <span class="step-title"><?php _e('Kundendaten', 'themisdb-order-request'); ?></span>
                </div>
                <div class="step <?php echo $current_step >= 5 ? 'active' : ''; ?>">
                    <span class="step-number">5</span>
                    <span class="step-title"><?php _e('Zusammenfassung', 'themisdb-order-request'); ?></span>
                </div>
            </div>
            
            <!-- Step Content -->
            <div class="order-content">
                <?php
                switch ($current_step) {
                    case 1:
                        $this->render_step_product($order);
                        break;
                    case 2:
                        $this->render_step_modules($order);
                        break;
                    case 3:
                        $this->render_step_training($order);
                        break;
                    case 4:
                        $this->render_step_customer($order);
                        break;
                    case 5:
                        $this->render_step_summary($order);
                        break;
                    default:
                        $this->render_step_product($order);
                }
                ?>
            </div>
        </div>
        <?php
    }
    
    /**
     * Render Step 1: Product Selection
     */
    private function render_step_product($order) {
        $products = ThemisDB_Order_Manager::get_products();
        $selected_edition = $order ? $order['product_edition'] : '';
        
        ?>
        <div class="order-step-content" data-step="1">
            <h2><?php _e('Wählen Sie Ihre ThemisDB Edition', 'themisdb-order-request'); ?></h2>
            <p><?php _e('Wählen Sie die Edition, die am besten zu Ihren Anforderungen passt.', 'themisdb-order-request'); ?></p>
            
            <div class="product-selection">
                <?php foreach ($products as $product): ?>
                <div class="product-card <?php echo $selected_edition === $product['edition'] ? 'selected' : ''; ?>">
                    <label>
                        <input type="radio" name="product_edition" value="<?php echo esc_attr($product['edition']); ?>" 
                               <?php checked($selected_edition, $product['edition']); ?>>
                        <div class="product-details">
                            <h3><?php echo esc_html($product['product_name']); ?></h3>
                            <p class="product-description"><?php echo esc_html($product['description']); ?></p>
                            <p class="product-price">
                                <?php if ($product['price'] == 0): ?>
                                    <strong><?php _e('Kostenlos', 'themisdb-order-request'); ?></strong>
                                <?php else: ?>
                                    <strong><?php echo number_format($product['price'], 2, ',', '.'); ?> €</strong> / <?php _e('Monat', 'themisdb-order-request'); ?>
                                <?php endif; ?>
                            </p>
                        </div>
                    </label>
                </div>
                <?php endforeach; ?>
            </div>
            
            <div class="order-navigation">
                <button type="button" class="button button-primary button-next" data-step="1">
                    <?php _e('Weiter', 'themisdb-order-request'); ?> →
                </button>
            </div>
        </div>
        <?php
    }
    
    /**
     * Render Step 2: Module Selection
     */
    private function render_step_modules($order) {
        $modules = ThemisDB_Order_Manager::get_modules();
        $selected_modules = $order && !empty($order['modules']) ? $order['modules'] : array();
        
        // Group modules by category
        $grouped_modules = array();
        foreach ($modules as $module) {
            $grouped_modules[$module['module_category']][] = $module;
        }
        
        ?>
        <div class="order-step-content" data-step="2">
            <h2><?php _e('Wählen Sie Ihre Module', 'themisdb-order-request'); ?></h2>
            <p><?php _e('Erweitern Sie Ihre ThemisDB-Installation mit zusätzlichen Modulen.', 'themisdb-order-request'); ?></p>
            
            <?php foreach ($grouped_modules as $category => $category_modules): ?>
            <div class="module-category">
                <h3><?php echo esc_html(ucfirst($category)); ?></h3>
                <div class="module-selection">
                    <?php foreach ($category_modules as $module): ?>
                    <div class="module-card">
                        <label>
                            <input type="checkbox" name="modules[]" value="<?php echo esc_attr($module['module_code']); ?>"
                                   <?php checked(in_array($module['module_code'], $selected_modules)); ?>>
                            <div class="module-details">
                                <h4><?php echo esc_html($module['module_name']); ?></h4>
                                <p><?php echo esc_html($module['description']); ?></p>
                                <p class="module-price">
                                    <?php if ($module['price'] == 0): ?>
                                        <em><?php _e('Inklusive', 'themisdb-order-request'); ?></em>
                                    <?php else: ?>
                                        <strong>+<?php echo number_format($module['price'], 2, ',', '.'); ?> €</strong>
                                    <?php endif; ?>
                                </p>
                            </div>
                        </label>
                    </div>
                    <?php endforeach; ?>
                </div>
            </div>
            <?php endforeach; ?>
            
            <div class="order-navigation">
                <button type="button" class="button button-secondary button-prev" data-step="2">
                    ← <?php _e('Zurück', 'themisdb-order-request'); ?>
                </button>
                <button type="button" class="button button-primary button-next" data-step="2">
                    <?php _e('Weiter', 'themisdb-order-request'); ?> →
                </button>
            </div>
        </div>
        <?php
    }
    
    /**
     * Render Step 3: Training Selection
     */
    private function render_step_training($order) {
        $trainings = ThemisDB_Order_Manager::get_training_modules();
        $selected_trainings = $order && !empty($order['training_modules']) ? $order['training_modules'] : array();
        
        // Group trainings by type
        $grouped_trainings = array();
        foreach ($trainings as $training) {
            $grouped_trainings[$training['training_type']][] = $training;
        }
        
        ?>
        <div class="order-step-content" data-step="3">
            <h2><?php _e('Wählen Sie Ihre Schulungen', 'themisdb-order-request'); ?></h2>
            <p><?php _e('Profitieren Sie von professionellen Schulungen für Ihr Team.', 'themisdb-order-request'); ?></p>
            
            <?php foreach ($grouped_trainings as $type => $type_trainings): ?>
            <div class="training-category">
                <h3><?php echo esc_html(ucfirst($type)); ?></h3>
                <div class="training-selection">
                    <?php foreach ($type_trainings as $training): ?>
                    <div class="training-card">
                        <label>
                            <input type="checkbox" name="training_modules[]" value="<?php echo esc_attr($training['training_code']); ?>"
                                   <?php checked(in_array($training['training_code'], $selected_trainings)); ?>>
                            <div class="training-details">
                                <h4><?php echo esc_html($training['training_name']); ?></h4>
                                <p><?php echo esc_html($training['description']); ?></p>
                                <p class="training-meta">
                                    <span class="training-duration">
                                        <strong><?php echo absint($training['duration_hours']); ?></strong> <?php _e('Stunden', 'themisdb-order-request'); ?>
                                    </span>
                                    <span class="training-price">
                                        <strong>+<?php echo number_format($training['price'], 2, ',', '.'); ?> €</strong>
                                    </span>
                                </p>
                            </div>
                        </label>
                    </div>
                    <?php endforeach; ?>
                </div>
            </div>
            <?php endforeach; ?>
            
            <div class="order-navigation">
                <button type="button" class="button button-secondary button-prev" data-step="3">
                    ← <?php _e('Zurück', 'themisdb-order-request'); ?>
                </button>
                <button type="button" class="button button-primary button-next" data-step="3">
                    <?php _e('Weiter', 'themisdb-order-request'); ?> →
                </button>
            </div>
        </div>
        <?php
    }
    
    /**
     * Render Step 4: Customer Data
     */
    private function render_step_customer($order) {
        $customer_name = $order ? $order['customer_name'] : '';
        $customer_email = $order ? $order['customer_email'] : '';
        $customer_company = $order ? $order['customer_company'] : '';
        
        ?>
        <div class="order-step-content" data-step="4">
            <h2><?php _e('Ihre Kontaktdaten', 'themisdb-order-request'); ?></h2>
            <p><?php _e('Bitte geben Sie Ihre Kontaktdaten ein.', 'themisdb-order-request'); ?></p>
            
            <div class="customer-form">
                <div class="form-group">
                    <label for="customer_name"><?php _e('Name', 'themisdb-order-request'); ?> *</label>
                    <input type="text" id="customer_name" name="customer_name" 
                           value="<?php echo esc_attr($customer_name); ?>" required>
                </div>
                
                <div class="form-group">
                    <label for="customer_email"><?php _e('E-Mail', 'themisdb-order-request'); ?> *</label>
                    <input type="email" id="customer_email" name="customer_email" 
                           value="<?php echo esc_attr($customer_email); ?>" required>
                </div>
                
                <div class="form-group">
                    <label for="customer_company"><?php _e('Unternehmen', 'themisdb-order-request'); ?></label>
                    <input type="text" id="customer_company" name="customer_company" 
                           value="<?php echo esc_attr($customer_company); ?>">
                </div>
                
                <div class="form-group">
                    <label>
                        <input type="checkbox" name="accept_terms" required>
                        <?php _e('Ich akzeptiere die AGB und Datenschutzerklärung', 'themisdb-order-request'); ?> *
                    </label>
                </div>
            </div>
            
            <div class="order-navigation">
                <button type="button" class="button button-secondary button-prev" data-step="4">
                    ← <?php _e('Zurück', 'themisdb-order-request'); ?>
                </button>
                <button type="button" class="button button-primary button-next" data-step="4">
                    <?php _e('Weiter', 'themisdb-order-request'); ?> →
                </button>
            </div>
        </div>
        <?php
    }
    
    /**
     * Render Step 5: Summary
     */
    private function render_step_summary($order) {
        if (!$order) {
            echo '<p>' . __('Keine Bestelldaten gefunden.', 'themisdb-order-request') . '</p>';
            return;
        }
        
        $products = ThemisDB_Order_Manager::get_products();
        $modules = ThemisDB_Order_Manager::get_modules();
        $trainings = ThemisDB_Order_Manager::get_training_modules();
        
        ?>
        <div class="order-step-content" data-step="5">
            <h2><?php _e('Zusammenfassung Ihrer Bestellung', 'themisdb-order-request'); ?></h2>
            
            <div class="order-summary">
                <div class="summary-section">
                    <h3><?php _e('Produkt', 'themisdb-order-request'); ?></h3>
                    <?php foreach ($products as $product): ?>
                        <?php if ($product['edition'] === $order['product_edition']): ?>
                            <p><strong><?php echo esc_html($product['product_name']); ?></strong><br>
                            <?php echo esc_html($product['description']); ?></p>
                        <?php endif; ?>
                    <?php endforeach; ?>
                </div>
                
                <?php if (!empty($order['modules'])): ?>
                <div class="summary-section">
                    <h3><?php _e('Module', 'themisdb-order-request'); ?></h3>
                    <ul>
                        <?php foreach ($modules as $module): ?>
                            <?php if (in_array($module['module_code'], $order['modules'])): ?>
                                <li><?php echo esc_html($module['module_name']); ?></li>
                            <?php endif; ?>
                        <?php endforeach; ?>
                    </ul>
                </div>
                <?php endif; ?>
                
                <?php if (!empty($order['training_modules'])): ?>
                <div class="summary-section">
                    <h3><?php _e('Schulungen', 'themisdb-order-request'); ?></h3>
                    <ul>
                        <?php foreach ($trainings as $training): ?>
                            <?php if (in_array($training['training_code'], $order['training_modules'])): ?>
                                <li><?php echo esc_html($training['training_name']); ?></li>
                            <?php endif; ?>
                        <?php endforeach; ?>
                    </ul>
                </div>
                <?php endif; ?>
                
                <div class="summary-section">
                    <h3><?php _e('Kontaktdaten', 'themisdb-order-request'); ?></h3>
                    <p>
                        <strong><?php echo esc_html($order['customer_name']); ?></strong><br>
                        <?php if ($order['customer_company']): ?>
                            <?php echo esc_html($order['customer_company']); ?><br>
                        <?php endif; ?>
                        <?php echo esc_html($order['customer_email']); ?>
                    </p>
                </div>
                
                <div class="summary-total">
                    <h3><?php _e('Gesamtbetrag', 'themisdb-order-request'); ?></h3>
                    <p class="total-amount">
                        <strong><?php echo number_format($order['total_amount'], 2, ',', '.'); ?> <?php echo esc_html($order['currency']); ?></strong>
                    </p>
                </div>
            </div>
            
            <div class="order-navigation">
                <button type="button" class="button button-secondary button-prev" data-step="5">
                    ← <?php _e('Zurück', 'themisdb-order-request'); ?>
                </button>
                <button type="button" class="button button-primary button-submit">
                    <?php _e('Bestellung absenden', 'themisdb-order-request'); ?>
                </button>
            </div>
        </div>
        <?php
    }
    
    /**
     * My orders shortcode
     */
    public function my_orders_shortcode() {
        if (!is_user_logged_in()) {
            return '<p>' . __('Bitte melden Sie sich an, um Ihre Bestellungen zu sehen.', 'themisdb-order-request') . '</p>';
        }
        
        $user_id = get_current_user_id();
        $orders = ThemisDB_Order_Manager::get_customer_orders($user_id);
        
        ob_start();
        ?>
        <div class="themisdb-my-orders">
            <h2><?php _e('Meine Bestellungen', 'themisdb-order-request'); ?></h2>
            
            <?php if (empty($orders)): ?>
                <p><?php _e('Sie haben noch keine Bestellungen.', 'themisdb-order-request'); ?></p>
            <?php else: ?>
                <table class="orders-table">
                    <thead>
                        <tr>
                            <th><?php _e('Bestellnummer', 'themisdb-order-request'); ?></th>
                            <th><?php _e('Produkt', 'themisdb-order-request'); ?></th>
                            <th><?php _e('Betrag', 'themisdb-order-request'); ?></th>
                            <th><?php _e('Status', 'themisdb-order-request'); ?></th>
                            <th><?php _e('Datum', 'themisdb-order-request'); ?></th>
                        </tr>
                    </thead>
                    <tbody>
                        <?php foreach ($orders as $order): ?>
                        <tr>
                            <td><?php echo esc_html($order['order_number']); ?></td>
                            <td>ThemisDB <?php echo esc_html(ucfirst($order['product_edition'])); ?></td>
                            <td><?php echo number_format($order['total_amount'], 2, ',', '.'); ?> <?php echo esc_html($order['currency']); ?></td>
                            <td><?php echo esc_html(ucfirst($order['status'])); ?></td>
                            <td><?php echo date('d.m.Y', strtotime($order['created_at'])); ?></td>
                        </tr>
                        <?php endforeach; ?>
                    </tbody>
                </table>
            <?php endif; ?>
        </div>
        <?php
        return ob_get_clean();
    }
    
    /**
     * My contracts shortcode
     */
    public function my_contracts_shortcode() {
        if (!is_user_logged_in()) {
            return '<p>' . __('Bitte melden Sie sich an, um Ihre Verträge zu sehen.', 'themisdb-order-request') . '</p>';
        }
        
        $user_id = get_current_user_id();
        $contracts = ThemisDB_Contract_Manager::get_customer_contracts($user_id);
        
        ob_start();
        ?>
        <div class="themisdb-my-contracts">
            <h2><?php _e('Meine Verträge', 'themisdb-order-request'); ?></h2>
            
            <?php if (empty($contracts)): ?>
                <p><?php _e('Sie haben noch keine Verträge.', 'themisdb-order-request'); ?></p>
            <?php else: ?>
                <table class="contracts-table">
                    <thead>
                        <tr>
                            <th><?php _e('Vertragsnummer', 'themisdb-order-request'); ?></th>
                            <th><?php _e('Typ', 'themisdb-order-request'); ?></th>
                            <th><?php _e('Status', 'themisdb-order-request'); ?></th>
                            <th><?php _e('Gültig bis', 'themisdb-order-request'); ?></th>
                        </tr>
                    </thead>
                    <tbody>
                        <?php foreach ($contracts as $contract): ?>
                        <tr>
                            <td><?php echo esc_html($contract['contract_number']); ?></td>
                            <td><?php echo esc_html(ucfirst($contract['contract_type'])); ?></td>
                            <td><?php echo esc_html(ucfirst($contract['status'])); ?></td>
                            <td><?php echo $contract['valid_until'] ? date('d.m.Y', strtotime($contract['valid_until'])) : '-'; ?></td>
                        </tr>
                        <?php endforeach; ?>
                    </tbody>
                </table>
            <?php endif; ?>
        </div>
        <?php
        return ob_get_clean();
    }
    
    /**
     * AJAX: Save order step
     */
    public function ajax_save_order_step() {
        check_ajax_referer('themisdb_order_nonce', 'nonce');
        
        $step = isset($_POST['step']) ? intval($_POST['step']) : 0;
        $data = isset($_POST['data']) ? $_POST['data'] : array();
        
        // Sanitize the data array
        $sanitized_data = array();
        foreach ($data as $key => $value) {
            if (is_array($value)) {
                // Sanitize array values (for modules and training_modules)
                $sanitized_data[$key] = array_map('sanitize_text_field', $value);
            } else {
                // Sanitize scalar values
                $sanitized_data[$key] = sanitize_text_field($value);
            }
        }
        $data = $sanitized_data;
        
        // Start session if not started
        if (!session_id()) {
            session_start();
        }
        
        $order_id = isset($_SESSION['themisdb_order_id']) ? $_SESSION['themisdb_order_id'] : null;
        
        if (!$order_id) {
            // Create new order
            $order_data = array(
                'customer_email' => isset($data['customer_email']) ? $data['customer_email'] : '',
                'customer_name' => isset($data['customer_name']) ? $data['customer_name'] : '',
                'customer_company' => isset($data['customer_company']) ? $data['customer_company'] : '',
                'product_type' => 'database',
                'product_edition' => isset($data['product_edition']) ? $data['product_edition'] : 'community'
            );
            
            $order_id = ThemisDB_Order_Manager::create_order($order_data);
            $_SESSION['themisdb_order_id'] = $order_id;
        } else {
            // Update existing order
            ThemisDB_Order_Manager::update_order($order_id, $data);
        }
        
        // Calculate total
        if (isset($data['product_edition']) || isset($data['modules']) || isset($data['training_modules'])) {
            $order = ThemisDB_Order_Manager::get_order($order_id);
            $total = ThemisDB_Order_Manager::calculate_total(
                $order['product_edition'],
                isset($data['modules']) ? $data['modules'] : ($order['modules'] ?: array()),
                isset($data['training_modules']) ? $data['training_modules'] : ($order['training_modules'] ?: array())
            );
            
            ThemisDB_Order_Manager::update_order($order_id, array('total_amount' => $total, 'step' => $step + 1));
        }
        
        wp_send_json_success(array(
            'order_id' => $order_id,
            'next_step' => $step + 1
        ));
    }
    
    /**
     * AJAX: Calculate total
     */
    public function ajax_calculate_total() {
        check_ajax_referer('themisdb_order_nonce', 'nonce');
        
        $product_edition = isset($_POST['product_edition']) ? sanitize_text_field($_POST['product_edition']) : '';
        $modules = isset($_POST['modules']) && is_array($_POST['modules']) ? array_map('sanitize_text_field', $_POST['modules']) : array();
        $training_modules = isset($_POST['training_modules']) && is_array($_POST['training_modules']) ? array_map('sanitize_text_field', $_POST['training_modules']) : array();
        
        $total = ThemisDB_Order_Manager::calculate_total($product_edition, $modules, $training_modules);
        
        wp_send_json_success(array('total' => $total));
    }
    
    /**
     * AJAX: Submit order
     */
    public function ajax_submit_order() {
        check_ajax_referer('themisdb_order_nonce', 'nonce');
        
        if (!session_id()) {
            session_start();
        }
        
        $order_id = isset($_SESSION['themisdb_order_id']) ? $_SESSION['themisdb_order_id'] : null;
        
        if (!$order_id) {
            wp_send_json_error(array('message' => __('Keine Bestellung gefunden', 'themisdb-order-request')));
            return;
        }
        
        // Update order status
        ThemisDB_Order_Manager::update_order($order_id, array('status' => 'pending'));
        
        // Send confirmation email
        ThemisDB_Email_Handler::send_order_confirmation($order_id);
        
        // Create contract
        $order = ThemisDB_Order_Manager::get_order($order_id);
        $contract_data = array(
            'order_id' => $order_id,
            'customer_id' => $order['customer_id'],
            'contract_type' => 'license',
            'contract_data' => $order
        );
        
        $contract_id = ThemisDB_Contract_Manager::create_contract($contract_data);
        
        if ($contract_id) {
            // Generate PDF
            ThemisDB_PDF_Generator::generate_contract_pdf($contract_id);
            
            // Send contract email
            ThemisDB_Email_Handler::send_contract_email($contract_id);
            
            // Create payment record
            $payment_data = array(
                'order_id' => $order_id,
                'contract_id' => $contract_id,
                'amount' => $order['total_amount'],
                'currency' => $order['currency'],
                'payment_method' => 'bank_transfer'
            );
            $payment_id = ThemisDB_Payment_Manager::create_payment($payment_data);
            
            // Create license
            $license_data = array(
                'order_id' => $order_id,
                'contract_id' => $contract_id,
                'customer_id' => $order['customer_id'],
                'product_edition' => $order['product_edition'],
                'license_type' => 'standard'
                // Limits will be set automatically based on tier in create_license()
            );
            $license_id = ThemisDB_License_Manager::create_license($license_data);
        }
        
        // Clear session
        unset($_SESSION['themisdb_order_id']);
        
        wp_send_json_success(array(
            'order_number' => $order['order_number'],
            'message' => __('Ihre Bestellung wurde erfolgreich übermittelt!', 'themisdb-order-request')
        ));
    }
}
