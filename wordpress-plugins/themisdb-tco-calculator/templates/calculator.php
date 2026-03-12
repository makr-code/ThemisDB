<?php
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            calculator.php                                     ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 04:08:22                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     633                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 9d3ecaa0e  2026-02-28  Add ThemisDB Wiki Integration plugin with documentation i... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

<!-- ThemisDB TCO Calculator WordPress Template -->
<div class="themisdb-tco-calculator-wrapper">
     if ($atts['show_intro'] === 'yes'): ?>
    <!-- Introduction Section -->
    <section class="intro-section">
        <div class="info-box">
            <h2>📊 Über diesen TCO-Rechner</h2>
            <p>
                Dieser interaktive Rechner hilft Ihnen, die <strong>Gesamtbetriebskosten (Total Cost of Ownership)</strong> 
                verschiedener Datenbanklösungen über einen Zeitraum von 3 Jahren zu vergleichen. 
                Berücksichtigt werden nicht nur Lizenzkosten, sondern auch Infrastruktur, Personal und Betriebskosten.
            </p>
            <div class="edition-badges">
                <span class="badge badge-minimal">Minimal (Free)</span>
                <span class="badge badge-community">Community (Free)</span>
                <span class="badge badge-enterprise">Enterprise (Commercial)</span>
                <span class="badge badge-hyperscaler">Hyperscaler (Custom)</span>
            </div>
        </div>
    </section>
    <?php endif; ?>

    <!-- Input Form Section -->
    <section class="input-section">
        <h2>⚙️ Ihre Anforderungen</h2>
        
        <!-- Workload Parameters -->
        <div class="parameter-group">
            <h3 class="group-title">📊 Workload & Anforderungen</h3>
            <div class="form-grid">
                <div class="form-group slider-group">
                    <label for="requestsPerDay">
                        <span class="label-text">Anfragen pro Tag</span>
                        <span class="label-info" title="Durchschnittliche Anzahl von Datenbank-Anfragen pro Tag">ℹ️</span>
                    </label>
                    <div class="slider-container">
                        <input type="range" id="requestsPerDay" class="slider" value="1000000" min="1000" max="10000000" step="10000">
                        <output for="requestsPerDay" id="requestsPerDay-value" class="slider-value">1.000.000</output>
                    </div>
                    <small>1 Tausend bis 10 Millionen Anfragen/Tag</small>
                </div>

                <div class="form-group slider-group">
                    <label for="dataSize">
                        <span class="label-text">Datenmenge (GB)</span>
                        <span class="label-info" title="Geschätzte Gesamtgröße Ihrer Daten">ℹ️</span>
                    </label>
                    <div class="slider-container">
                        <input type="range" id="dataSize" class="slider" value="500" min="10" max="10000" step="10">
                        <output for="dataSize" id="dataSize-value" class="slider-value">500 GB</output>
                    </div>
                    <small>10 GB bis 10 TB</small>
                </div>

                <div class="form-group slider-group">
                    <label for="peakLoad">
                        <span class="label-text">Spitzenlast-Faktor</span>
                        <span class="label-info" title="Verhältnis von Spitzenlast zu Durchschnittslast">ℹ️</span>
                    </label>
                    <div class="slider-container">
                        <input type="range" id="peakLoad" class="slider" value="3" min="1" max="10" step="0.5">
                        <output for="peakLoad" id="peakLoad-value" class="slider-value">3x</output>
                    </div>
                    <small>1x bis 10x der Durchschnittslast</small>
                </div>

                <div class="form-group">
                    <label for="availability">
                        <span class="label-text">Verfügbarkeitsanforderung</span>
                        <span class="label-info" title="Benötigte Systemverfügbarkeit">ℹ️</span>
                    </label>
                    <select id="availability">
                        <option value="99">99% (Standard)</option>
                        <option value="99.9">99.9% (High)</option>
                        <option value="99.99">99.99% (Very High)</option>
                        <option value="99.999" selected>99.999% (Mission Critical)</option>
                    </select>
                </div>
            </div>
        </div>

        <!-- Infrastructure Parameters -->
        <div class="parameter-group">
            <h3 class="group-title">🖥️ Infrastruktur & Hardware</h3>
            <div class="form-grid">
                <div class="form-group slider-group">
                    <label for="serverCost">
                        <span class="label-text">Server-Kosten (€/Monat)</span>
                        <span class="label-info" title="Monatliche Kosten pro Server (Cloud oder On-Premise)">ℹ️</span>
                    </label>
                    <div class="slider-container">
                        <input type="range" id="serverCost" class="slider" value="500" min="100" max="2000" step="50">
                        <output for="serverCost" id="serverCost-value" class="slider-value">€500</output>
                    </div>
                    <small>€100 bis €2.000 pro Server</small>
                </div>

                <div class="form-group slider-group">
                    <label for="storageCostPerGB">
                        <span class="label-text">Speicher-Kosten (€/GB/Monat)</span>
                        <span class="label-info" title="Monatliche Kosten pro GB Speicher">ℹ️</span>
                    </label>
                    <div class="slider-container">
                        <input type="range" id="storageCostPerGB" class="slider" value="0.10" min="0.01" max="1" step="0.01">
                        <output for="storageCostPerGB" id="storageCostPerGB-value" class="slider-value">€0.10</output>
                    </div>
                    <small>€0.01 bis €1.00 pro GB</small>
                </div>

                <div class="form-group slider-group">
                    <label for="networkCost">
                        <span class="label-text">Netzwerk-Kosten (€/TB)</span>
                        <span class="label-info" title="Kosten für ausgehenden Datenverkehr pro TB">ℹ️</span>
                    </label>
                    <div class="slider-container">
                        <input type="range" id="networkCost" class="slider" value="50" min="10" max="200" step="10">
                        <output for="networkCost" id="networkCost-value" class="slider-value">€50</output>
                    </div>
                    <small>€10 bis €200 pro TB</small>
                </div>

                <div class="form-group slider-group">
                    <label for="backupCost">
                        <span class="label-text">Backup-Kosten (€/GB/Monat)</span>
                        <span class="label-info" title="Monatliche Kosten für Backup-Speicher">ℹ️</span>
                    </label>
                    <div class="slider-container">
                        <input type="range" id="backupCost" class="slider" value="0.05" min="0.01" max="0.5" step="0.01">
                        <output for="backupCost" id="backupCost-value" class="slider-value">€0.05</output>
                    </div>
                    <small>€0.01 bis €0.50 pro GB</small>
                </div>
            </div>
        </div>

        <!-- Personnel Costs -->
        <div class="parameter-group">
            <h3 class="group-title">👥 Personal & Team</h3>
            <div class="form-grid">
                <div class="form-group slider-group">
                    <label for="dbaCount">
                        <span class="label-text">Anzahl DBAs</span>
                        <span class="label-info" title="Anzahl Vollzeit-Datenbankadministratoren">ℹ️</span>
                    </label>
                    <div class="slider-container">
                        <input type="range" id="dbaCount" class="slider" value="2" min="0" max="10" step="0.5">
                        <output for="dbaCount" id="dbaCount-value" class="slider-value">2 FTE</output>
                    </div>
                    <small>0 bis 10 FTE</small>
                </div>

                <div class="form-group slider-group">
                    <label for="dbaSalary">
                        <span class="label-text">DBA Gehalt (€/Jahr)</span>
                        <span class="label-info" title="Durchschnittliches Jahresgehalt pro DBA">ℹ️</span>
                    </label>
                    <div class="slider-container">
                        <input type="range" id="dbaSalary" class="slider" value="85000" min="40000" max="150000" step="5000">
                        <output for="dbaSalary" id="dbaSalary-value" class="slider-value">€85.000</output>
                    </div>
                    <small>€40.000 bis €150.000</small>
                </div>

                <div class="form-group slider-group">
                    <label for="devCount">
                        <span class="label-text">Anzahl Entwickler</span>
                        <span class="label-info" title="Entwickler für Datenbankintegration">ℹ️</span>
                    </label>
                    <div class="slider-container">
                        <input type="range" id="devCount" class="slider" value="5" min="0" max="20" step="0.5">
                        <output for="devCount" id="devCount-value" class="slider-value">5 FTE</output>
                    </div>
                    <small>0 bis 20 FTE</small>
                </div>

                <div class="form-group slider-group">
                    <label for="devSalary">
                        <span class="label-text">Dev Gehalt (€/Jahr)</span>
                        <span class="label-info" title="Durchschnittliches Jahresgehalt pro Entwickler">ℹ️</span>
                    </label>
                    <div class="slider-container">
                        <input type="range" id="devSalary" class="slider" value="75000" min="35000" max="130000" step="5000">
                        <output for="devSalary" id="devSalary-value" class="slider-value">€75.000</output>
                    </div>
                    <small>€35.000 bis €130.000</small>
                </div>
            </div>
        </div>

        <!-- Additional Costs -->
        <div class="parameter-group">
            <h3 class="group-title">🔧 Betrieb & Support</h3>
            <div class="form-grid">
                <div class="form-group slider-group">
                    <label for="trainingCost">
                        <span class="label-text">Schulungskosten (€/Jahr)</span>
                        <span class="label-info" title="Jährliche Kosten für Team-Schulungen">ℹ️</span>
                    </label>
                    <div class="slider-container">
                        <input type="range" id="trainingCost" class="slider" value="20000" min="0" max="100000" step="1000">
                        <output for="trainingCost" id="trainingCost-value" class="slider-value">€20.000</output>
                    </div>
                    <small>€0 bis €100.000</small>
                </div>

                <div class="form-group slider-group">
                    <label for="supportCost">
                        <span class="label-text">Support-Kosten (€/Jahr)</span>
                        <span class="label-info" title="Jährliche Enterprise-Support-Kosten">ℹ️</span>
                    </label>
                    <div class="slider-container">
                        <input type="range" id="supportCost" class="slider" value="50000" min="0" max="200000" step="5000">
                        <output for="supportCost" id="supportCost-value" class="slider-value">€50.000</output>
                    </div>
                    <small>€0 bis €200.000</small>
                </div>
            </div>
        </div>

        <!-- AI/LLM Features -->
        <div class="parameter-group">
            <h3 class="group-title">🤖 AI & LLM Features</h3>
            <div class="form-grid">
                <div class="form-group">
                    <label for="useAI">
                        <span class="label-text">AI/LLM Features nutzen?</span>
                        <span class="label-info" title="Native LLM-Integration mit llama.cpp">ℹ️</span>
                    </label>
                    <select id="useAI">
                        <option value="false">Nein</option>
                        <option value="true">Ja (inkl. GPU)</option>
                    </select>
                </div>

                <div class="form-group slider-group">
                    <label for="aiApiCost">
                        <span class="label-text">Externe AI API Kosten (€/Monat)</span>
                        <span class="label-info" title="Monatliche Kosten für externe AI APIs (OpenAI, etc.)">ℹ️</span>
                    </label>
                    <div class="slider-container">
                        <input type="range" id="aiApiCost" class="slider" value="5000" min="0" max="20000" step="500">
                        <output for="aiApiCost" id="aiApiCost-value" class="slider-value">€5.000</output>
                    </div>
                    <small>€0 bis €20.000</small>
                </div>
            </div>
        </div>

        <div class="button-group">
            <button id="calculateBtn" class="btn btn-primary">
                💰 TCO Berechnen
            </button>
            <button id="resetBtn" class="btn btn-secondary">
                🔄 Zurücksetzen
            </button>
        </div>
    </section>

    <!-- Results Section -->
    <section id="resultsSection" class="results-section" style="display: none;">
        <h2>📈 TCO-Analyse (3 Jahre)</h2>
        
        <!-- Summary Cards -->
        <div class="summary-cards">
            <div class="card card-themisdb">
                <div class="card-header">
                    <h3>ThemisDB</h3>
                    <span id="themisdbEdition" class="badge">Community</span>
                </div>
                <div class="card-body">
                    <div class="cost-main">
                        <span class="cost-value" id="themisdbTotal">€0</span>
                        <span class="cost-label">Gesamtkosten</span>
                    </div>
                    <div class="cost-breakdown">
                        <div class="cost-item collapsible-item">
                            <div class="cost-item-header" data-toggle-details="themisdb-infra">
                                <span class="cost-item-label">
                                    <span class="collapse-icon">▼</span>
                                    Material & Infrastruktur:
                                </span>
                                <span class="cost-item-value" id="themisdbInfra">€0</span>
                            </div>
                            <div class="cost-item-details" id="themisdb-infra-details">
                                <small>Server, Storage, Netzwerk, Backups</small>
                            </div>
                        </div>
                        <div class="cost-item collapsible-item">
                            <div class="cost-item-header" data-toggle-details="themisdb-personnel">
                                <span class="cost-item-label">
                                    <span class="collapse-icon">▼</span>
                                    Personal:
                                </span>
                                <span class="cost-item-value" id="themisdbPersonnel">€0</span>
                            </div>
                            <div class="cost-item-details" id="themisdb-personnel-details">
                                <small>DBAs & Entwickler (inkl. 30% Overhead für Sozialleistungen)</small>
                            </div>
                        </div>
                        <div class="cost-item collapsible-item">
                            <div class="cost-item-header" data-toggle-details="themisdb-software">
                                <span class="cost-item-label">
                                    <span class="collapse-icon">▼</span>
                                    Software & Lizenzen:
                                </span>
                                <span class="cost-item-value" id="themisdbLicense">€0</span>
                            </div>
                            <div class="cost-item-details" id="themisdb-software-details">
                                <small>ThemisDB Lizenz (Community = kostenlos)</small>
                            </div>
                        </div>
                        <div class="cost-item collapsible-item">
                            <div class="cost-item-header" data-toggle-details="themisdb-ops">
                                <span class="cost-item-label">
                                    <span class="collapse-icon">▼</span>
                                    Betrieb & Schulung:
                                </span>
                                <span class="cost-item-value" id="themisdbOps">€0</span>
                            </div>
                            <div class="cost-item-details" id="themisdb-ops-details">
                                <small>Support, Wartung, Team-Schulungen</small>
                            </div>
                        </div>
                    </div>
                </div>
            </div>

            <div class="card card-hyperscaler">
                <div class="card-header">
                    <h3>Cloud-Hyperscaler</h3>
                    <span class="badge badge-cloud">AWS/Azure/GCP</span>
                </div>
                <div class="card-body">
                    <div class="cost-main">
                        <span class="cost-value" id="hyperscalerTotal">€0</span>
                        <span class="cost-label">Gesamtkosten</span>
                    </div>
                    <div class="cost-breakdown">
                        <div class="cost-item collapsible-item">
                            <div class="cost-item-header" data-toggle-details="hyperscaler-compute">
                                <span class="cost-item-label">
                                    <span class="collapse-icon">▼</span>
                                    Compute (Pay-per-Request):
                                </span>
                                <span class="cost-item-value" id="hyperscalerCompute">€0</span>
                            </div>
                            <div class="cost-item-details" id="hyperscaler-compute-details">
                                <small>Nutzungsbasierte Abrechnung pro Anfrage</small>
                            </div>
                        </div>
                        <div class="cost-item collapsible-item">
                            <div class="cost-item-header" data-toggle-details="hyperscaler-storage">
                                <span class="cost-item-label">
                                    <span class="collapse-icon">▼</span>
                                    Storage:
                                </span>
                                <span class="cost-item-value" id="hyperscalerStorage">€0</span>
                            </div>
                            <div class="cost-item-details" id="hyperscaler-storage-details">
                                <small>Redundante Speicherung (1.5x Multiplikator)</small>
                            </div>
                        </div>
                        <div class="cost-item collapsible-item">
                            <div class="cost-item-header" data-toggle-details="hyperscaler-network">
                                <span class="cost-item-label">
                                    <span class="collapse-icon">▼</span>
                                    Network (Egress):
                                </span>
                                <span class="cost-item-value" id="hyperscalerNetwork">€0</span>
                            </div>
                            <div class="cost-item-details" id="hyperscaler-network-details">
                                <small>Datenübertragungskosten (ausgehend)</small>
                            </div>
                        </div>
                        <div class="cost-item collapsible-item">
                            <div class="cost-item-header" data-toggle-details="hyperscaler-ai">
                                <span class="cost-item-label">
                                    <span class="collapse-icon">▼</span>
                                    AI APIs:
                                </span>
                                <span class="cost-item-value" id="hyperscalerAI">€0</span>
                            </div>
                            <div class="cost-item-details" id="hyperscaler-ai-details">
                                <small>Externe AI API Kosten (OpenAI, Anthropic, etc.)</small>
                            </div>
                        </div>
                    </div>
                </div>
            </div>

            <div class="card card-savings">
                <div class="card-header">
                    <h3>💰 Einsparungen</h3>
                </div>
                <div class="card-body">
                    <div class="cost-main">
                        <span class="cost-value savings-value" id="savingsAmount">€0</span>
                        <span class="cost-label">Über 3 Jahre</span>
                    </div>
                    <div class="savings-percentage" id="savingsPercentage">
                        0% günstiger
                    </div>
                    <div class="roi-info">
                        <strong>ROI-Zeitpunkt:</strong>
                        <span id="roiTime">Nach 12 Monaten</span>
                    </div>
                </div>
            </div>
        </div>

        <!-- Chart Container -->
        <div class="chart-container">
            <h3>📊 Kostenvergleich über 3 Jahre</h3>
            <canvas id="costChart"></canvas>
        </div>

        <!-- Mermaid Diagram Section -->
        <div class="mermaid-container">
            <h3>📊 Dynamischer Kostenvergleich (Mermaid)</h3>
            <div class="mermaid-diagram" id="mermaidDiagram">
                <!-- Mermaid diagram will be rendered here -->
            </div>
        </div>

        <!-- Polyglot Database Capabilities Radar Chart -->
        <div class="mermaid-container">
            <h3>🎯 Multi-Model Fähigkeiten: ThemisDB vs. Hyperscaler Polyglot</h3>
            <div class="info-box" style="margin-bottom: 20px;">
                <p>
                    <strong>ThemisDB</strong> ist eine Multi-Model-Datenbank, die alle Anforderungen in einem System vereint. 
                    Bei Hyperscalern benötigen Sie mehrere spezialisierte Datenbanken, was die Komplexität und Kosten erhöht.
                </p>
            </div>
            <div class="mermaid-diagram" id="polyglotDiagram">
                <!-- Polyglot radar diagram will be rendered here -->
            </div>
            <div class="polyglot-legend">
                <h4>Benötigte Datenbank-Services beim Hyperscaler:</h4>
                <ul id="polyglotServicesList">
                    <!-- Will be filled by JavaScript -->
                </ul>
            </div>
        </div>

        <!-- Performance Calculator Diagram -->
        <div class="mermaid-container">
            <h3>⚡ Performance-Vergleich: ThemisDB vs. Hyperscaler</h3>
            <div class="info-box" style="margin-bottom: 20px;">
                <p>
                    <strong>ThemisDB</strong> bietet überlegene Performance durch native Optimierungen wie RAID Sharding, 
                    integrierte LLM-Verarbeitung und Multi-Model-Engine. Hyperscaler leiden unter Netzwerk-Latenz 
                    und Overhead durch verteilte Systeme.
                </p>
            </div>
            <div class="mermaid-diagram" id="performanceDiagram">
                <!-- Performance diagram will be rendered here -->
            </div>
            <div class="polyglot-legend">
                <h4>Performance-Metriken im Detail:</h4>
                <ul id="performanceMetricsList">
                    <!-- Will be filled by JavaScript -->
                </ul>
            </div>
        </div>

        <!-- Personnel Cost Explanation -->
        <div class="info-box personnel-explanation">
            <h3>💡 Warum erscheinen die Personalkosten hoch?</h3>
            <p>
                Die Personalkosten bei ThemisDB umfassen nicht nur die reinen Gehälter, sondern auch:
            </p>
            <ul>
                <li><strong>30% Overhead</strong> für Sozialleistungen, Büroinfrastruktur, Equipment und andere Nebenkosten</li>
                <li><strong>Qualifiziertes Personal</strong>: DBAs und Entwickler für optimalen Betrieb und Integration</li>
                <li><strong>Vollständige Kontrolle</strong>: Im Gegensatz zu Hyperscalern, wo Personal versteckt in den Service-Kosten enthalten ist</li>
            </ul>
            <p>
                <strong>Wichtiger Kontext:</strong> Bei Hyperscalern zahlen Sie ebenfalls für Personal - es ist nur unsichtbar in den Service-Preisen eingerechnet. 
                Bei ThemisDB haben Sie die <em>Transparenz</em> und können durch Automatisierung und Effizienz diese Kosten optimieren.
                Zudem entfällt bei der <strong>Community Edition</strong> die Lizenzgebühr komplett, was erhebliche Einsparungen ermöglicht.
            </p>
            <div class="cost-comparison-highlight">
                <h4>💰 Der Unterschied:</h4>
                <div class="comparison-grid">
                    <div class="comparison-item">
                        <strong>ThemisDB:</strong>
                        <p>Transparente Personalkosten + Keine/Niedrige Lizenzkosten + Vorhersagbare Infrastruktur</p>
                    </div>
                    <div class="comparison-item">
                        <strong>Hyperscaler:</strong>
                        <p>Versteckte Personalkosten in Pay-per-Request + Unvorhersagbare Kosten bei Skalierung</p>
                    </div>
                </div>
            </div>
        </div>

        <!-- Detailed Breakdown -->
        <div class="detailed-breakdown">
            <h3>🔍 Detaillierte Aufschlüsselung</h3>
            <div class="tabs">
                <button class="tab-btn active" data-tab="themisdb-details">ThemisDB</button>
                <button class="tab-btn" data-tab="hyperscaler-details">Hyperscaler</button>
                <button class="tab-btn" data-tab="comparison">Vergleich</button>
            </div>
            
            <div id="themisdb-details" class="tab-content active">
                <table class="breakdown-table">
                    <thead>
                        <tr>
                            <th>Kostenart</th>
                            <th>Jahr 1</th>
                            <th>Jahr 2</th>
                            <th>Jahr 3</th>
                            <th>Gesamt</th>
                        </tr>
                    </thead>
                    <tbody id="themisdbBreakdownTable">
                        <!-- Filled by JavaScript -->
                    </tbody>
                </table>
            </div>

            <div id="hyperscaler-details" class="tab-content">
                <table class="breakdown-table">
                    <thead>
                        <tr>
                            <th>Kostenart</th>
                            <th>Jahr 1</th>
                            <th>Jahr 2</th>
                            <th>Jahr 3</th>
                            <th>Gesamt</th>
                        </tr>
                    </thead>
                    <tbody id="hyperscalerBreakdownTable">
                        <!-- Filled by JavaScript -->
                    </tbody>
                </table>
            </div>

            <div id="comparison" class="tab-content">
                <div class="comparison-insights">
                    <h4>💡 Wichtige Erkenntnisse</h4>
                    <ul id="insightsList">
                        <!-- Filled by JavaScript -->
                    </ul>
                </div>
            </div>
        </div>

        <!-- Export Options -->
        <div class="export-section">
            <h3>📥 Ergebnisse exportieren</h3>
            <div class="button-group">
                <button id="exportPDF" class="btn btn-secondary">
                    📄 Als PDF
                </button>
                <button id="exportCSV" class="btn btn-secondary">
                    📊 Als CSV
                </button>
                <button id="printBtn" class="btn btn-secondary">
                    🖨️ Drucken
                </button>
            </div>
        </div>
    </section>

    <!-- Assumptions Section -->
    <section class="assumptions-section">
        <h2>📝 Annahmen und Methodik</h2>
        <div class="assumptions-content">
            <h3>Berechnungsgrundlage</h3>
            <ul>
                <li><strong>Betrachtungszeitraum:</strong> 3 Jahre (typischer Planungshorizont)</li>
                <li><strong>Datenwachstum:</strong> 20% pro Jahr (Branchendurchschnitt)</li>
                <li><strong>ThemisDB Lizenzmodell:</strong>
                    <ul>
                        <li>Community Edition: Kostenlos (MIT Lizenz)</li>
                        <li>Enterprise Edition: ~€50.000/Jahr (geschätzt, abhängig von Nodes)</li>
                        <li>Hyperscaler Edition: Custom Pricing</li>
                    </ul>
                </li>
                <li><strong>Hyperscaler-Kosten:</strong> Basierend auf AWS DynamoDB, Azure Cosmos DB, Google Cloud Spanner</li>
                <li><strong>Personalkosten:</strong> Inkl. 30% Overhead (Sozialleistungen, Infrastruktur)</li>
                <li><strong>AI/LLM-Kosten:</strong>
                    <ul>
                        <li>ThemisDB: Native llama.cpp Integration (GPU-Hardware, kein API-Kosten)</li>
                        <li>Hyperscaler: Externe APIs (OpenAI, Anthropic, etc.)</li>
                    </ul>
                </li>
            </ul>

            <h3>Kostenvorteile von ThemisDB</h3>
            <ul>
                <li>✅ <strong>Keine Pay-per-Request Kosten:</strong> Vorhersagbare Kosten</li>
                <li>✅ <strong>Native AI Integration:</strong> Keine externen API-Kosten</li>
                <li>✅ <strong>Multi-Model:</strong> Ein System statt mehrerer spezialisierter DBs</li>
                <li>✅ <strong>Open Source:</strong> Community Edition komplett kostenlos</li>
                <li>✅ <strong>Keine Vendor Lock-in:</strong> Flexible Deployment-Optionen</li>
            </ul>

            <h3>Wann lohnt sich ThemisDB?</h3>
            <ul>
                <li>📈 <strong>Hoher Durchsatz:</strong> > 100.000 Anfragen/Tag</li>
                <li>💾 <strong>Große Datenmengen:</strong> > 100 GB aktive Daten</li>
                <li>🤖 <strong>AI/LLM-Workloads:</strong> Vermeidung von API-Kosten</li>
                <li>🎯 <strong>Multi-Model Anforderungen:</strong> Graph + Document + Vector + Timeseries</li>
                <li>🔒 <strong>Datensouveränität:</strong> On-Premise oder Private Cloud</li>
            </ul>
        </div>
    </section>
</div>
