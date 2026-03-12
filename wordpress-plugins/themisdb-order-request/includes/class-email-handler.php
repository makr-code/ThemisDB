<?php
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            class-email-handler.php                            ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 04:08:19                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     475                                            ║
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
 * Email Handler for ThemisDB Order Request Plugin
 */

if (!defined('ABSPATH')) {
    exit;
}

class ThemisDB_Email_Handler {
    
    /**
     * Send order confirmation email
     */
    public static function send_order_confirmation($order_id) {
        $order = ThemisDB_Order_Manager::get_order($order_id);
        
        if (!$order) {
            return false;
        }
        
        $to = $order['customer_email'];
        $subject = sprintf(__('Ihre Bestellung %s wurde bestätigt', 'themisdb-order-request'), $order['order_number']);
        
        $message = self::get_order_confirmation_template($order);
        
        // Generate PDF attachment
        $pdf_data = ThemisDB_PDF_Generator::generate_order_pdf($order_id);
        
        $attachments = array();
        if ($pdf_data) {
            $temp_file = tempnam(sys_get_temp_dir(), 'order_pdf');
            file_put_contents($temp_file, $pdf_data);
            $attachments[] = $temp_file;
        }
        
        $result = self::send_email($to, $subject, $message, $attachments, $order_id, null);
        
        // Clean up temp files
        if (!empty($attachments)) {
            foreach ($attachments as $file) {
                if (file_exists($file)) {
                    unlink($file);
                }
            }
        }
        
        return $result;
    }
    
    /**
     * Send contract email
     */
    public static function send_contract_email($contract_id) {
        $contract = ThemisDB_Contract_Manager::get_contract($contract_id);
        
        if (!$contract) {
            return false;
        }
        
        $order = ThemisDB_Order_Manager::get_order($contract['order_id']);
        
        $to = $order['customer_email'];
        $subject = sprintf(__('Ihr Vertrag %s', 'themisdb-order-request'), $contract['contract_number']);
        
        $message = self::get_contract_template($contract, $order);
        
        // Get PDF from contract
        $pdf_data = ThemisDB_PDF_Generator::get_contract_pdf($contract_id);
        
        $attachments = array();
        if ($pdf_data) {
            $temp_file = tempnam(sys_get_temp_dir(), 'contract_pdf');
            file_put_contents($temp_file, $pdf_data);
            $attachments[] = $temp_file;
        }
        
        $result = self::send_email($to, $subject, $message, $attachments, $order['id'], $contract_id);
        
        // Clean up temp files
        if (!empty($attachments)) {
            foreach ($attachments as $file) {
                if (file_exists($file)) {
                    unlink($file);
                }
            }
        }
        
        return $result;
    }
    
    /**
     * Send email with logging
     */
    private static function send_email($to, $subject, $message, $attachments = array(), $order_id = null, $contract_id = null) {
        global $wpdb;
        
        // Get from email settings
        $from_email = get_option('themisdb_order_email_from', get_option('admin_email'));
        $from_name = get_option('themisdb_order_email_from_name', get_option('blogname'));
        
        // Set headers
        $headers = array(
            'Content-Type: text/html; charset=UTF-8',
            sprintf('From: %s <%s>', $from_name, $from_email)
        );
        
        // Log email
        $table_email_log = $wpdb->prefix . 'themisdb_email_log';
        $log_data = array(
            'order_id' => $order_id,
            'contract_id' => $contract_id,
            'recipient' => $to,
            'subject' => $subject,
            'body' => $message,
            'attachments' => !empty($attachments) ? json_encode($attachments) : null,
            'status' => 'pending'
        );
        
        $wpdb->insert($table_email_log, $log_data);
        $log_id = $wpdb->insert_id;
        
        // Send email
        $result = wp_mail($to, $subject, $message, $headers, $attachments);
        
        // Update log
        if ($result) {
            $wpdb->update(
                $table_email_log,
                array(
                    'status' => 'sent',
                    'sent_at' => current_time('mysql')
                ),
                array('id' => $log_id),
                null,
                array('%d')
            );
        } else {
            $wpdb->update(
                $table_email_log,
                array(
                    'status' => 'failed',
                    'error_message' => 'wp_mail returned false'
                ),
                array('id' => $log_id),
                null,
                array('%d')
            );
        }
        
        return $result;
    }
    
    /**
     * Get order confirmation email template
     */
    private static function get_order_confirmation_template($order) {
        ob_start();
        ?>
        <!DOCTYPE html>
        <html>
        <head>
            <meta charset="UTF-8">
            <style>
                body {
                    font-family: Arial, sans-serif;
                    line-height: 1.6;
                    color: #333;
                }
                .container {
                    max-width: 600px;
                    margin: 0 auto;
                    padding: 20px;
                }
                .header {
                    background-color: #0073aa;
                    color: white;
                    padding: 20px;
                    text-align: center;
                }
                .content {
                    padding: 20px;
                    background-color: #f9f9f9;
                }
                .order-details {
                    background-color: white;
                    padding: 15px;
                    margin: 20px 0;
                    border: 1px solid #ddd;
                }
                .footer {
                    text-align: center;
                    padding: 20px;
                    color: #666;
                    font-size: 12px;
                }
                table {
                    width: 100%;
                    border-collapse: collapse;
                }
                th, td {
                    padding: 8px;
                    text-align: left;
                    border-bottom: 1px solid #ddd;
                }
                th {
                    background-color: #f5f5f5;
                }
            </style>
        </head>
        <body>
            <div class="container">
                <div class="header">
                    <h1>Bestellbestätigung</h1>
                </div>
                
                <div class="content">
                    <p>Sehr geehrte(r) <?php echo esc_html($order['customer_name']); ?>,</p>
                    
                    <p>vielen Dank für Ihre Bestellung bei ThemisDB. Wir haben Ihre Bestellung mit der Nummer <strong><?php echo esc_html($order['order_number']); ?></strong> erhalten und bearbeiten diese umgehend.</p>
                    
                    <div class="order-details">
                        <h2>Bestelldetails</h2>
                        
                        <table>
                            <tr>
                                <th>Bestellnummer:</th>
                                <td><?php echo esc_html($order['order_number']); ?></td>
                            </tr>
                            <tr>
                                <th>Bestelldatum:</th>
                                <td><?php echo date('d.m.Y H:i', strtotime($order['created_at'])); ?> Uhr</td>
                            </tr>
                            <tr>
                                <th>Produkt:</th>
                                <td>ThemisDB <?php echo esc_html(ucfirst($order['product_edition'])); ?> Edition</td>
                            </tr>
                            <tr>
                                <th>Gesamtbetrag:</th>
                                <td><strong><?php echo number_format($order['total_amount'], 2, ',', '.'); ?> <?php echo esc_html($order['currency']); ?></strong></td>
                            </tr>
                        </table>
                        
                        <?php if (!empty($order['modules'])): ?>
                        <h3>Ausgewählte Module:</h3>
                        <ul>
                            <?php 
                            $modules = ThemisDB_Order_Manager::get_modules();
                            foreach ($modules as $module):
                                if (in_array($module['module_code'], $order['modules'])):
                            ?>
                            <li><?php echo esc_html($module['module_name']); ?></li>
                            <?php 
                                endif;
                            endforeach; 
                            ?>
                        </ul>
                        <?php endif; ?>
                        
                        <?php if (!empty($order['training_modules'])): ?>
                        <h3>Ausgewählte Schulungen:</h3>
                        <ul>
                            <?php 
                            $trainings = ThemisDB_Order_Manager::get_training_modules();
                            foreach ($trainings as $training):
                                if (in_array($training['training_code'], $order['training_modules'])):
                            ?>
                            <li><?php echo esc_html($training['training_name']); ?></li>
                            <?php 
                                endif;
                            endforeach; 
                            ?>
                        </ul>
                        <?php endif; ?>
                    </div>
                    
                    <p>Eine detaillierte Bestellbestätigung finden Sie im Anhang dieser E-Mail.</p>
                    
                    <p>Bei Fragen zu Ihrer Bestellung stehen wir Ihnen gerne zur Verfügung.</p>
                    
                    <p>Mit freundlichen Grüßen<br>
                    Ihr ThemisDB Team</p>
                </div>
                
                <div class="footer">
                    <p><?php echo esc_html(get_option('blogname')); ?><br>
                    <?php echo esc_html(get_option('admin_email')); ?></p>
                </div>
            </div>
        </body>
        </html>
        <?php
        return ob_get_clean();
    }
    
    /**
     * Get contract email template
     */
    private static function get_contract_template($contract, $order) {
        ob_start();
        ?>
        <!DOCTYPE html>
        <html>
        <head>
            <meta charset="UTF-8">
            <style>
                body {
                    font-family: Arial, sans-serif;
                    line-height: 1.6;
                    color: #333;
                }
                .container {
                    max-width: 600px;
                    margin: 0 auto;
                    padding: 20px;
                }
                .header {
                    background-color: #0073aa;
                    color: white;
                    padding: 20px;
                    text-align: center;
                }
                .content {
                    padding: 20px;
                    background-color: #f9f9f9;
                }
                .contract-details {
                    background-color: white;
                    padding: 15px;
                    margin: 20px 0;
                    border: 1px solid #ddd;
                }
                .footer {
                    text-align: center;
                    padding: 20px;
                    color: #666;
                    font-size: 12px;
                }
                .important {
                    background-color: #fff3cd;
                    border: 1px solid #ffc107;
                    padding: 15px;
                    margin: 20px 0;
                    border-radius: 4px;
                }
            </style>
        </head>
        <body>
            <div class="container">
                <div class="header">
                    <h1>Ihr Vertrag</h1>
                </div>
                
                <div class="content">
                    <p>Sehr geehrte(r) <?php echo esc_html($order['customer_name']); ?>,</p>
                    
                    <p>anbei erhalten Sie Ihren Vertrag mit der Nummer <strong><?php echo esc_html($contract['contract_number']); ?></strong> im Anhang.</p>
                    
                    <div class="contract-details">
                        <h2>Vertragsdetails</h2>
                        
                        <table style="width: 100%;">
                            <tr>
                                <td style="width: 40%; padding: 8px; border-bottom: 1px solid #ddd;"><strong>Vertragsnummer:</strong></td>
                                <td style="padding: 8px; border-bottom: 1px solid #ddd;"><?php echo esc_html($contract['contract_number']); ?></td>
                            </tr>
                            <tr>
                                <td style="padding: 8px; border-bottom: 1px solid #ddd;"><strong>Vertragstyp:</strong></td>
                                <td style="padding: 8px; border-bottom: 1px solid #ddd;"><?php echo esc_html(ucfirst($contract['contract_type'])); ?></td>
                            </tr>
                            <tr>
                                <td style="padding: 8px; border-bottom: 1px solid #ddd;"><strong>Gültig von:</strong></td>
                                <td style="padding: 8px; border-bottom: 1px solid #ddd;"><?php echo date('d.m.Y', strtotime($contract['valid_from'])); ?></td>
                            </tr>
                            <?php if ($contract['valid_until']): ?>
                            <tr>
                                <td style="padding: 8px; border-bottom: 1px solid #ddd;"><strong>Gültig bis:</strong></td>
                                <td style="padding: 8px; border-bottom: 1px solid #ddd;"><?php echo date('d.m.Y', strtotime($contract['valid_until'])); ?></td>
                            </tr>
                            <?php endif; ?>
                            <tr>
                                <td style="padding: 8px; border-bottom: 1px solid #ddd;"><strong>Status:</strong></td>
                                <td style="padding: 8px; border-bottom: 1px solid #ddd;"><?php echo esc_html(ucfirst($contract['status'])); ?></td>
                            </tr>
                        </table>
                    </div>
                    
                    <div class="important">
                        <strong>Wichtig:</strong> Bitte prüfen Sie den Vertrag sorgfältig. Bei Fragen oder Änderungswünschen kontaktieren Sie uns bitte umgehend.
                    </div>
                    
                    <p>Um den Vertrag rechtsverbindlich abzuschließen, senden Sie uns bitte den unterschriebenen Vertrag zurück.</p>
                    
                    <p>Mit freundlichen Grüßen<br>
                    Ihr ThemisDB Team</p>
                </div>
                
                <div class="footer">
                    <p><?php echo esc_html(get_option('blogname')); ?><br>
                    <?php echo esc_html(get_option('admin_email')); ?></p>
                </div>
            </div>
        </body>
        </html>
        <?php
        return ob_get_clean();
    }
    
    /**
     * Get email logs
     */
    public static function get_email_logs($args = array()) {
        global $wpdb;
        
        $table_email_log = $wpdb->prefix . 'themisdb_email_log';
        
        $defaults = array(
            'order_id' => null,
            'contract_id' => null,
            'status' => null,
            'limit' => 50,
            'offset' => 0,
            'orderby' => 'created_at',
            'order' => 'DESC'
        );
        
        $args = wp_parse_args($args, $defaults);
        
        $where = "1=1";
        $where_values = array();
        
        if ($args['order_id']) {
            $where .= " AND order_id = %d";
            $where_values[] = $args['order_id'];
        }
        
        if ($args['contract_id']) {
            $where .= " AND contract_id = %d";
            $where_values[] = $args['contract_id'];
        }
        
        if ($args['status']) {
            $where .= " AND status = %s";
            $where_values[] = $args['status'];
        }
        
        $query = "SELECT * FROM $table_email_log WHERE $where ORDER BY {$args['orderby']} {$args['order']} LIMIT %d OFFSET %d";
        $where_values[] = $args['limit'];
        $where_values[] = $args['offset'];
        
        return $wpdb->get_results($wpdb->prepare($query, $where_values), ARRAY_A);
    }
}
