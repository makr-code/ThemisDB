<?php
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            class-pdf-generator.php                            ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 04:08:20                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     574                                            ║
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
 * PDF Generator for ThemisDB Order Request Plugin
 * Generates PDFs for orders, contracts, and invoices
 */

if (!defined('ABSPATH')) {
    exit;
}

class ThemisDB_PDF_Generator {
    
    /**
     * Generate PDF for contract
     */
    public static function generate_contract_pdf($contract_id) {
        $contract = ThemisDB_Contract_Manager::get_contract($contract_id);
        
        if (!$contract) {
            return false;
        }
        
        // Get order data
        $order = ThemisDB_Order_Manager::get_order($contract['order_id']);
        
        // Generate HTML content
        $html = self::generate_contract_html($contract, $order);
        
        // Convert to PDF
        $pdf_data = self::html_to_pdf($html, 'contract-' . $contract['contract_number']);
        
        if ($pdf_data) {
            // Store PDF based on settings
            $storage_mode = get_option('themisdb_order_pdf_storage', 'database');
            
            if ($storage_mode === 'database') {
                // Store in database
                ThemisDB_Contract_Manager::update_contract($contract_id, array(
                    'pdf_data' => $pdf_data
                ));
            } else {
                // Store as file
                $upload_dir = wp_upload_dir();
                $pdf_dir = $upload_dir['basedir'] . '/themisdb-contracts';
                
                if (!file_exists($pdf_dir)) {
                    wp_mkdir_p($pdf_dir);
                }
                
                $filename = 'contract-' . $contract['contract_number'] . '.pdf';
                $filepath = $pdf_dir . '/' . $filename;
                
                file_put_contents($filepath, $pdf_data);
                
                // Create WordPress attachment
                $attachment = array(
                    'post_mime_type' => 'application/pdf',
                    'post_title' => 'Contract ' . $contract['contract_number'],
                    'post_content' => '',
                    'post_status' => 'inherit'
                );
                
                $attach_id = wp_insert_attachment($attachment, $filepath);
                
                if ($attach_id) {
                    ThemisDB_Contract_Manager::update_contract($contract_id, array(
                        'pdf_file_id' => $attach_id
                    ));
                }
            }
            
            return $pdf_data;
        }
        
        return false;
    }
    
    /**
     * Generate PDF for order
     */
    public static function generate_order_pdf($order_id) {
        $order = ThemisDB_Order_Manager::get_order($order_id);
        
        if (!$order) {
            return false;
        }
        
        // Generate HTML content
        $html = self::generate_order_html($order);
        
        // Convert to PDF
        return self::html_to_pdf($html, 'order-' . $order['order_number']);
    }
    
    /**
     * Generate contract HTML
     */
    private static function generate_contract_html($contract, $order) {
        ob_start();
        ?>
        <!DOCTYPE html>
        <html>
        <head>
            <meta charset="UTF-8">
            <title>Vertrag <?php echo esc_html($contract['contract_number']); ?></title>
            <style>
                body {
                    font-family: Arial, sans-serif;
                    font-size: 12pt;
                    line-height: 1.6;
                    color: #333;
                    margin: 40px;
                }
                .header {
                    text-align: center;
                    margin-bottom: 30px;
                    border-bottom: 2px solid #333;
                    padding-bottom: 20px;
                }
                .header h1 {
                    margin: 0;
                    font-size: 24pt;
                }
                .contract-info {
                    margin: 20px 0;
                }
                .contract-info table {
                    width: 100%;
                    border-collapse: collapse;
                }
                .contract-info td {
                    padding: 8px;
                    border: 1px solid #ddd;
                }
                .contract-info td:first-child {
                    font-weight: bold;
                    width: 30%;
                    background-color: #f5f5f5;
                }
                .section {
                    margin: 30px 0;
                }
                .section h2 {
                    font-size: 16pt;
                    border-bottom: 1px solid #333;
                    padding-bottom: 5px;
                    margin-bottom: 15px;
                }
                .footer {
                    margin-top: 50px;
                    border-top: 2px solid #333;
                    padding-top: 20px;
                }
                .signature-box {
                    margin-top: 50px;
                    display: inline-block;
                    width: 45%;
                }
                .signature-line {
                    border-top: 1px solid #333;
                    margin-top: 60px;
                    padding-top: 5px;
                }
            </style>
        </head>
        <body>
            <div class="header">
                <h1><?php echo esc_html(ucfirst($contract['contract_type'])); ?>vertrag</h1>
                <p>Vertragsnummer: <?php echo esc_html($contract['contract_number']); ?></p>
            </div>
            
            <div class="contract-info">
                <table>
                    <tr>
                        <td>Vertragsnummer:</td>
                        <td><?php echo esc_html($contract['contract_number']); ?></td>
                    </tr>
                    <tr>
                        <td>Bestellnummer:</td>
                        <td><?php echo esc_html($order['order_number']); ?></td>
                    </tr>
                    <tr>
                        <td>Kunde:</td>
                        <td><?php echo esc_html($order['customer_name']); ?></td>
                    </tr>
                    <?php if ($order['customer_company']): ?>
                    <tr>
                        <td>Unternehmen:</td>
                        <td><?php echo esc_html($order['customer_company']); ?></td>
                    </tr>
                    <?php endif; ?>
                    <tr>
                        <td>E-Mail:</td>
                        <td><?php echo esc_html($order['customer_email']); ?></td>
                    </tr>
                    <tr>
                        <td>Gültig von:</td>
                        <td><?php echo esc_html(date('d.m.Y', strtotime($contract['valid_from']))); ?></td>
                    </tr>
                    <?php if ($contract['valid_until']): ?>
                    <tr>
                        <td>Gültig bis:</td>
                        <td><?php echo esc_html(date('d.m.Y', strtotime($contract['valid_until']))); ?></td>
                    </tr>
                    <?php endif; ?>
                </table>
            </div>
            
            <div class="section">
                <h2>§1 Vertragsgegenstand</h2>
                <p>Gegenstand dieses Vertrages ist die Lizenzierung und Nutzung von ThemisDB <?php echo esc_html(ucfirst($order['product_edition'])); ?> Edition.</p>
            </div>
            
            <div class="section">
                <h2>§2 Produkt und Module</h2>
                <table style="width: 100%; border-collapse: collapse;">
                    <tr style="background-color: #f5f5f5;">
                        <th style="padding: 8px; border: 1px solid #ddd; text-align: left;">Produkt/Modul</th>
                        <th style="padding: 8px; border: 1px solid #ddd; text-align: right;">Preis</th>
                    </tr>
                    <tr>
                        <td style="padding: 8px; border: 1px solid #ddd;">ThemisDB <?php echo esc_html(ucfirst($order['product_edition'])); ?> Edition</td>
                        <td style="padding: 8px; border: 1px solid #ddd; text-align: right;">
                            <?php 
                            $products = ThemisDB_Order_Manager::get_products();
                            foreach ($products as $product) {
                                if ($product['edition'] === $order['product_edition']) {
                                    echo number_format($product['price'], 2, ',', '.') . ' €';
                                    break;
                                }
                            }
                            ?>
                        </td>
                    </tr>
                    <?php if (!empty($order['modules'])): ?>
                        <?php 
                        $modules = ThemisDB_Order_Manager::get_modules();
                        foreach ($modules as $module):
                            if (in_array($module['module_code'], $order['modules'])):
                        ?>
                        <tr>
                            <td style="padding: 8px; border: 1px solid #ddd;"><?php echo esc_html($module['module_name']); ?></td>
                            <td style="padding: 8px; border: 1px solid #ddd; text-align: right;"><?php echo number_format($module['price'], 2, ',', '.'); ?> €</td>
                        </tr>
                        <?php 
                            endif;
                        endforeach; 
                        ?>
                    <?php endif; ?>
                    <?php if (!empty($order['training_modules'])): ?>
                        <?php 
                        $trainings = ThemisDB_Order_Manager::get_training_modules();
                        foreach ($trainings as $training):
                            if (in_array($training['training_code'], $order['training_modules'])):
                        ?>
                        <tr>
                            <td style="padding: 8px; border: 1px solid #ddd;"><?php echo esc_html($training['training_name']); ?></td>
                            <td style="padding: 8px; border: 1px solid #ddd; text-align: right;"><?php echo number_format($training['price'], 2, ',', '.'); ?> €</td>
                        </tr>
                        <?php 
                            endif;
                        endforeach; 
                        ?>
                    <?php endif; ?>
                    <tr style="font-weight: bold; background-color: #f5f5f5;">
                        <td style="padding: 8px; border: 1px solid #ddd;">Gesamtsumme</td>
                        <td style="padding: 8px; border: 1px solid #ddd; text-align: right;"><?php echo number_format($order['total_amount'], 2, ',', '.'); ?> €</td>
                    </tr>
                </table>
            </div>
            
            <div class="section">
                <h2>§3 Nutzungsrechte</h2>
                <p>Der Auftraggeber erhält ein nicht-exklusives, nicht übertragbares Recht zur Nutzung der Software gemäß der gewählten Edition und den ausgewählten Modulen.</p>
            </div>
            
            <div class="section">
                <h2>§4 Datenschutz und Compliance</h2>
                <p>Beide Parteien verpflichten sich zur Einhaltung der geltenden Datenschutzbestimmungen, insbesondere der DSGVO. Die Verarbeitung personenbezogener Daten erfolgt ausschließlich im Rahmen der Vertragserfüllung.</p>
            </div>
            
            <div class="section">
                <h2>§5 Vertragslaufzeit und Kündigung</h2>
                <p>Der Vertrag wird auf unbestimmte Zeit geschlossen und kann von beiden Parteien mit einer Frist von 3 Monaten zum Monatsende gekündigt werden.</p>
            </div>
            
            <div class="section">
                <h2>§6 Salvatorische Klausel</h2>
                <p>Sollten einzelne Bestimmungen dieses Vertrages unwirksam sein oder werden, so wird die Wirksamkeit der übrigen Bestimmungen davon nicht berührt.</p>
            </div>
            
            <div class="footer">
                <p><strong>Erstellt am:</strong> <?php echo date('d.m.Y H:i'); ?> Uhr</p>
                
                <div style="margin-top: 50px;">
                    <div class="signature-box">
                        <div class="signature-line">
                            Ort, Datum, Unterschrift Auftragnehmer
                        </div>
                    </div>
                    <div class="signature-box" style="float: right;">
                        <div class="signature-line">
                            Ort, Datum, Unterschrift Auftraggeber
                        </div>
                    </div>
                </div>
            </div>
        </body>
        </html>
        <?php
        return ob_get_clean();
    }
    
    /**
     * Generate order HTML
     */
    private static function generate_order_html($order) {
        ob_start();
        ?>
        <!DOCTYPE html>
        <html>
        <head>
            <meta charset="UTF-8">
            <title>Bestellung <?php echo esc_html($order['order_number']); ?></title>
            <style>
                body {
                    font-family: Arial, sans-serif;
                    font-size: 12pt;
                    line-height: 1.6;
                    color: #333;
                    margin: 40px;
                }
                .header {
                    text-align: center;
                    margin-bottom: 30px;
                    border-bottom: 2px solid #333;
                    padding-bottom: 20px;
                }
                .header h1 {
                    margin: 0;
                    font-size: 24pt;
                }
                table {
                    width: 100%;
                    border-collapse: collapse;
                    margin: 20px 0;
                }
                th, td {
                    padding: 10px;
                    border: 1px solid #ddd;
                    text-align: left;
                }
                th {
                    background-color: #f5f5f5;
                    font-weight: bold;
                }
                .total {
                    font-size: 14pt;
                    font-weight: bold;
                    background-color: #f5f5f5;
                }
            </style>
        </head>
        <body>
            <div class="header">
                <h1>Bestellbestätigung</h1>
                <p>Bestellnummer: <?php echo esc_html($order['order_number']); ?></p>
            </div>
            
            <h2>Kundendaten</h2>
            <table>
                <tr>
                    <td style="width: 30%; font-weight: bold;">Name:</td>
                    <td><?php echo esc_html($order['customer_name']); ?></td>
                </tr>
                <?php if ($order['customer_company']): ?>
                <tr>
                    <td style="font-weight: bold;">Unternehmen:</td>
                    <td><?php echo esc_html($order['customer_company']); ?></td>
                </tr>
                <?php endif; ?>
                <tr>
                    <td style="font-weight: bold;">E-Mail:</td>
                    <td><?php echo esc_html($order['customer_email']); ?></td>
                </tr>
            </table>
            
            <h2>Bestelldetails</h2>
            <table>
                <thead>
                    <tr>
                        <th>Position</th>
                        <th>Beschreibung</th>
                        <th style="text-align: right;">Preis</th>
                    </tr>
                </thead>
                <tbody>
                    <tr>
                        <td>1</td>
                        <td>ThemisDB <?php echo esc_html(ucfirst($order['product_edition'])); ?> Edition</td>
                        <td style="text-align: right;">
                            <?php 
                            $products = ThemisDB_Order_Manager::get_products();
                            foreach ($products as $product) {
                                if ($product['edition'] === $order['product_edition']) {
                                    echo number_format($product['price'], 2, ',', '.') . ' €';
                                    break;
                                }
                            }
                            ?>
                        </td>
                    </tr>
                    <?php 
                    $position = 2;
                    if (!empty($order['modules'])): 
                        $modules = ThemisDB_Order_Manager::get_modules();
                        foreach ($modules as $module):
                            if (in_array($module['module_code'], $order['modules'])):
                    ?>
                    <tr>
                        <td><?php echo $position++; ?></td>
                        <td><?php echo esc_html($module['module_name']); ?></td>
                        <td style="text-align: right;"><?php echo number_format($module['price'], 2, ',', '.'); ?> €</td>
                    </tr>
                    <?php 
                            endif;
                        endforeach; 
                    endif;
                    
                    if (!empty($order['training_modules'])): 
                        $trainings = ThemisDB_Order_Manager::get_training_modules();
                        foreach ($trainings as $training):
                            if (in_array($training['training_code'], $order['training_modules'])):
                    ?>
                    <tr>
                        <td><?php echo $position++; ?></td>
                        <td><?php echo esc_html($training['training_name']); ?></td>
                        <td style="text-align: right;"><?php echo number_format($training['price'], 2, ',', '.'); ?> €</td>
                    </tr>
                    <?php 
                            endif;
                        endforeach; 
                    endif;
                    ?>
                    <tr class="total">
                        <td colspan="2" style="text-align: right;">Gesamtsumme:</td>
                        <td style="text-align: right;"><?php echo number_format($order['total_amount'], 2, ',', '.'); ?> €</td>
                    </tr>
                </tbody>
            </table>
            
            <p><strong>Status:</strong> <?php echo esc_html(ucfirst($order['status'])); ?></p>
            <p><strong>Erstellt am:</strong> <?php echo date('d.m.Y H:i', strtotime($order['created_at'])); ?> Uhr</p>
        </body>
        </html>
        <?php
        return ob_get_clean();
    }
    
    /**
     * Convert HTML to PDF
     * This is a wrapper that can use different PDF libraries
     */
    private static function html_to_pdf($html, $filename) {
        // Try to use wkhtmltopdf if available (most reliable)
        if (self::is_wkhtmltopdf_available()) {
            return self::wkhtmltopdf_convert($html, $filename);
        }
        
        // Fallback: Return HTML with instructions to install PDF library
        // In production, you would use a library like TCPDF, FPDF, or Dompdf
        return self::fallback_pdf($html, $filename);
    }
    
    /**
     * Check if wkhtmltopdf is available
     */
    private static function is_wkhtmltopdf_available() {
        $output = array();
        $return_var = 0;
        exec('which wkhtmltopdf 2>/dev/null', $output, $return_var);
        return $return_var === 0;
    }
    
    /**
     * Convert using wkhtmltopdf
     */
    private static function wkhtmltopdf_convert($html, $filename) {
        $temp_html = tempnam(sys_get_temp_dir(), 'html');
        $temp_pdf = tempnam(sys_get_temp_dir(), 'pdf');
        
        file_put_contents($temp_html, $html);
        
        $command = sprintf(
            'wkhtmltopdf --encoding UTF-8 --page-size A4 %s %s 2>&1',
            escapeshellarg($temp_html),
            escapeshellarg($temp_pdf)
        );
        
        exec($command, $output, $return_var);
        
        if ($return_var === 0 && file_exists($temp_pdf)) {
            $pdf_data = file_get_contents($temp_pdf);
            unlink($temp_html);
            unlink($temp_pdf);
            return $pdf_data;
        }
        
        unlink($temp_html);
        if (file_exists($temp_pdf)) {
            unlink($temp_pdf);
        }
        
        return false;
    }
    
    /**
     * Fallback: Return HTML as "PDF" with instructions
     */
    private static function fallback_pdf($html, $filename) {
        // In a real implementation, this would use TCPDF, FPDF, or similar
        // For now, we'll return a base64 encoded HTML as a placeholder
        return base64_encode($html);
    }
    
    /**
     * Get PDF from contract
     */
    public static function get_contract_pdf($contract_id) {
        $contract = ThemisDB_Contract_Manager::get_contract($contract_id);
        
        if (!$contract) {
            return false;
        }
        
        $storage_mode = get_option('themisdb_order_pdf_storage', 'database');
        
        if ($storage_mode === 'database') {
            return $contract['pdf_data'];
        } else {
            if ($contract['pdf_file_id']) {
                $filepath = get_attached_file($contract['pdf_file_id']);
                if (file_exists($filepath)) {
                    return file_get_contents($filepath);
                }
            }
        }
        
        // Generate if not exists
        return self::generate_contract_pdf($contract_id);
    }
}
