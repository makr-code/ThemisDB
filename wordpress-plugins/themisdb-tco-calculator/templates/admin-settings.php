<?php
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            admin-settings.php                                 ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 04:08:22                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     229                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 9d3ecaa0e  2026-02-28  Add ThemisDB Wiki Integration plugin with documentation i... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

<div class="wrap">
    <h1> echo esc_html(get_admin_page_title()); ?></h1>
    
    <div class="notice notice-info">
        <p>
            <strong>Verwendung:</strong> Fügen Sie den Shortcode <code>[themisdb_tco_calculator]</code> 
            in eine beliebige Seite oder einen Beitrag ein, um den TCO-Rechner anzuzeigen.
        </p>
        <p>
            <strong>Optionale Parameter:</strong>
            <ul>
                <li><code>[themisdb_tco_calculator show_intro="no"]</code> - Verbirgt die Einführungssektion</li>
                <li><code>[themisdb_tco_calculator title="Mein TCO-Rechner"]</code> - Angepasster Titel</li>
            </ul>
        </p>
    </div>
    
    <?php
    if (isset($_GET['settings-updated'])) {
        add_settings_error(
            'themisdb_tco_messages',
            'themisdb_tco_message',
            __('Einstellungen gespeichert', 'themisdb-tco-calculator'),
            'updated'
        );
    }
    
    settings_errors('themisdb_tco_messages');
    ?>
    
    <form action="options.php" method="post">
        <?php
        settings_fields('themisdb_tco_options');
        ?>
        
        <table class="form-table" role="presentation">
            <tbody>
                <tr>
                    <th scope="row">
                        <label for="themisdb_tco_enable_ai_features">
                            <?php _e('AI Features aktivieren', 'themisdb-tco-calculator'); ?>
                        </label>
                    </th>
                    <td>
                        <input type="checkbox" 
                               id="themisdb_tco_enable_ai_features" 
                               name="themisdb_tco_enable_ai_features" 
                               value="1" 
                               <?php checked(1, get_option('themisdb_tco_enable_ai_features', true)); ?>>
                        <p class="description">
                            <?php _e('Ermöglicht AI/LLM Features im Rechner', 'themisdb-tco-calculator'); ?>
                        </p>
                    </td>
                </tr>
                
                <tr>
                    <th scope="row">
                        <label for="themisdb_tco_default_requests_per_day">
                            <?php _e('Standard Anfragen/Tag', 'themisdb-tco-calculator'); ?>
                        </label>
                    </th>
                    <td>
                        <input type="number" 
                               id="themisdb_tco_default_requests_per_day" 
                               name="themisdb_tco_default_requests_per_day" 
                               value="<?php echo esc_attr(get_option('themisdb_tco_default_requests_per_day', 1000000)); ?>"
                               class="regular-text"
                               min="1000"
                               step="10000">
                        <p class="description">
                            <?php _e('Standardwert für durchschnittliche Anfragen pro Tag', 'themisdb-tco-calculator'); ?>
                        </p>
                    </td>
                </tr>
                
                <tr>
                    <th scope="row">
                        <label for="themisdb_tco_default_data_size">
                            <?php _e('Standard Datengröße (GB)', 'themisdb-tco-calculator'); ?>
                        </label>
                    </th>
                    <td>
                        <input type="number" 
                               id="themisdb_tco_default_data_size" 
                               name="themisdb_tco_default_data_size" 
                               value="<?php echo esc_attr(get_option('themisdb_tco_default_data_size', 500)); ?>"
                               class="regular-text"
                               min="1"
                               step="10">
                        <p class="description">
                            <?php _e('Standardwert für Datenmenge in Gigabyte', 'themisdb-tco-calculator'); ?>
                        </p>
                    </td>
                </tr>
                
                <tr>
                    <th scope="row">
                        <label for="themisdb_tco_default_peak_load">
                            <?php _e('Standard Spitzenlast-Faktor', 'themisdb-tco-calculator'); ?>
                        </label>
                    </th>
                    <td>
                        <input type="number" 
                               id="themisdb_tco_default_peak_load" 
                               name="themisdb_tco_default_peak_load" 
                               value="<?php echo esc_attr(get_option('themisdb_tco_default_peak_load', 3)); ?>"
                               class="regular-text"
                               min="1"
                               max="10"
                               step="0.5">
                        <p class="description">
                            <?php _e('Verhältnis Spitzenlast zu Durchschnittslast (1-10x)', 'themisdb-tco-calculator'); ?>
                        </p>
                    </td>
                </tr>
                
                <tr>
                    <th scope="row">
                        <label for="themisdb_tco_default_availability">
                            <?php _e('Standard Verfügbarkeit (%)', 'themisdb-tco-calculator'); ?>
                        </label>
                    </th>
                    <td>
                        <select id="themisdb_tco_default_availability" 
                                name="themisdb_tco_default_availability"
                                class="regular-text">
                            <option value="99" <?php selected(get_option('themisdb_tco_default_availability', 99.999), 99); ?>>
                                99% (Standard)
                            </option>
                            <option value="99.9" <?php selected(get_option('themisdb_tco_default_availability', 99.999), 99.9); ?>>
                                99.9% (High)
                            </option>
                            <option value="99.99" <?php selected(get_option('themisdb_tco_default_availability', 99.999), 99.99); ?>>
                                99.99% (Very High)
                            </option>
                            <option value="99.999" <?php selected(get_option('themisdb_tco_default_availability', 99.999), 99.999); ?>>
                                99.999% (Mission Critical)
                            </option>
                        </select>
                        <p class="description">
                            <?php _e('Standardwert für Verfügbarkeitsanforderung', 'themisdb-tco-calculator'); ?>
                        </p>
                    </td>
                </tr>
            </tbody>
        </table>
        
        <?php submit_button(__('Einstellungen speichern', 'themisdb-tco-calculator')); ?>
    </form>
    
    <hr>
    
    <h2><?php _e('Plugin-Updates', 'themisdb-tco-calculator'); ?></h2>
    <div class="notice notice-info inline">
        <p>
            <strong>🔄 Automatische Updates von GitHub</strong><br>
            Dieses Plugin unterstützt automatische Updates direkt von GitHub. 
            Neue Versionen werden automatisch unter <strong>Dashboard → Aktualisierungen</strong> angezeigt.
        </p>
        <p>
            <strong>Aktuell installierte Version:</strong> <?php echo THEMISDB_TCO_VERSION; ?><br>
            <strong>GitHub Repository:</strong> 
            <a href="https://github.com/<?php echo THEMISDB_TCO_GITHUB_REPO; ?>" target="_blank">
                <?php echo THEMISDB_TCO_GITHUB_REPO; ?>
            </a>
        </p>
        <?php
        // Check for update manually
        $latest_release_info = get_transient('themisdb_tco_github_release');
        if (!$latest_release_info) {
            echo '<p><em>Prüfen Sie unter Dashboard → Aktualisierungen auf neue Versionen.</em></p>';
        } else {
            $latest_version = isset($latest_release_info->tag_name) ? $latest_release_info->tag_name : 'Unbekannt';
            if (version_compare(THEMISDB_TCO_VERSION, $latest_version, '<')) {
                echo '<p style="color: #d63638;"><strong>⚠️ Eine neue Version (' . esc_html($latest_version) . ') ist verfügbar!</strong></p>';
            } else {
                echo '<p style="color: #00a32a;"><strong>✅ Plugin ist auf dem neuesten Stand.</strong></p>';
            }
        }
        ?>
    </div>
    
    <hr>
    
    <h2><?php _e('Über dieses Plugin', 'themisdb-tco-calculator'); ?></h2>
    <p>
        <strong>Version:</strong> <?php echo THEMISDB_TCO_VERSION; ?><br>
        <strong>Lizenz:</strong> MIT License<br>
        <strong>GitHub:</strong> <a href="https://github.com/makr-code/wordpressPlugins" target="_blank">makr-code/wordpressPlugins</a>
    </p>
    
    <h3><?php _e('Features', 'themisdb-tco-calculator'); ?></h3>
    <ul>
        <li>✅ Interaktive TCO-Berechnung für ThemisDB vs. Cloud-Hyperscaler</li>
        <li>✅ Umfassende Kostenanalyse (Infrastruktur, Personal, Lizenzen, Betrieb)</li>
        <li>✅ Visuelle Darstellung mit Chart.js</li>
        <li>✅ Export-Funktionen (PDF, CSV)</li>
        <li>✅ Responsive Design</li>
        <li>✅ WordPress-Integration via Shortcode</li>
    </ul>
    
    <h3><?php _e('Support', 'themisdb-tco-calculator'); ?></h3>
    <p>
        <?php _e('Bei Fragen oder Problemen erstellen Sie bitte ein Issue auf', 'themisdb-tco-calculator'); ?> 
        <a href="https://github.com/makr-code/wordpressPlugins/issues" target="_blank">GitHub</a>.
    </p>
</div>
