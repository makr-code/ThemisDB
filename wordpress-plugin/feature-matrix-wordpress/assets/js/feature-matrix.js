/**
 * ThemisDB Feature Matrix JavaScript
 * Based on TCO Calculator pattern with Mermaid.js integration
 */

(function($) {
    'use strict';

    // Global namespace
    window.ThemisDBFeatureMatrix = {
        currentData: null,
        settings: {},

        /**
         * Initialize the feature matrix
         */
        init: function() {
            // Store settings from PHP
            this.settings = themisdbFM.settings || {};

            // Initialize Mermaid
            this.initMermaid();

            // Set up event listeners
            this.setupEventListeners();

            // Load initial data
            this.loadFeatures();
        },

        /**
         * Initialize Mermaid.js
         */
        initMermaid: function() {
            if (typeof mermaid !== 'undefined') {
                mermaid.initialize({
                    startOnLoad: false,
                    theme: 'neutral',
                    securityLevel: 'strict',
                    flowchart: {
                        useMaxWidth: true,
                        htmlLabels: true,
                        curve: 'basis'
                    },
                    themeVariables: {
                        primaryColor: '#2ea44f',
                        primaryTextColor: '#fff',
                        primaryBorderColor: '#2c974b',
                        lineColor: '#57606a',
                        secondaryColor: '#f6f8fa',
                        tertiaryColor: '#3498db'
                    }
                });
            }
        },

        /**
         * Setup event listeners
         */
        setupEventListeners: function() {
            const self = this;

            // Filter changes
            $('#fm-category-filter, #fm-view-type').on('change', function() {
                self.loadFeatures();
            });

            // Refresh button
            $('#fm-refresh-data').on('click', function(e) {
                e.preventDefault();
                self.refreshFeatures();
            });

            // Export buttons
            $('#fm-export-csv').on('click', function(e) {
                e.preventDefault();
                self.exportCSV();
            });

            $('#fm-export-pdf').on('click', function(e) {
                e.preventDefault();
                self.exportPDF();
            });

            $('#fm-print').on('click', function(e) {
                e.preventDefault();
                window.print();
            });
        },

        /**
         * Load feature data via AJAX
         */
        loadFeatures: function() {
            const self = this;
            const category = $('#fm-category-filter').val() || 'all';

            // Show loading state
            this.showLoading();

            $.ajax({
                url: themisdbFM.ajax_url,
                type: 'POST',
                data: {
                    action: 'themisdb_fm_get_features',
                    nonce: themisdbFM.nonce,
                    category: category
                },
                success: function(response) {
                    if (response.success) {
                        self.currentData = response.data;
                        self.renderTable();
                        if (self.settings.show_mermaid) {
                            self.renderMermaidDiagram();
                        }
                    } else {
                        self.showError('Failed to load feature data');
                    }
                    self.hideLoading();
                },
                error: function() {
                    self.showError('Error loading feature data');
                    self.hideLoading();
                }
            });
        },

        /**
         * Refresh features (clear cache)
         */
        refreshFeatures: function() {
            this.loadFeatures();
        },

        /**
         * Render feature comparison table
         */
        renderTable: function() {
            if (!this.currentData) return;

            const $tableContainer = $('#fm-matrix-table');
            const viewType = $('#fm-view-type').val() || 'detailed';

            let html = '<table>';
            html += '<thead><tr>';
            html += '<th>' + this.translate('Feature') + '</th>';

            // Add column for each database
            this.currentData.databases.forEach(function(db) {
                const dbName = db.charAt(0).toUpperCase() + db.slice(1);
                html += '<th>' + dbName + '</th>';
            });

            html += '</tr></thead><tbody>';

            // Add rows for each feature
            this.currentData.features.forEach(function(feature) {
                html += '<tr>';
                
                // Feature name column
                html += '<td>';
                html += '<div class="feature-name">' + feature.name + '</div>';
                
                if (viewType === 'detailed' && feature.description) {
                    html += '<div class="feature-description">' + feature.description + '</div>';
                }
                
                html += '</td>';

                // Status columns for each database
                this.currentData.databases.forEach(function(db) {
                    const status = feature[db] || 'not_available';
                    const statusInfo = this.getStatusInfo(status);
                    
                    html += '<td class="text-center">';
                    
                    if (this.settings.enable_tooltips) {
                        html += '<span class="themisdb-status-badge status-' + status + '" data-tooltip="' + statusInfo.text + '">';
                    } else {
                        html += '<span class="themisdb-status-badge status-' + status + '">';
                    }
                    
                    html += statusInfo.icon + '</span>';
                    html += '</td>';
                }.bind(this));

                html += '</tr>';
            }.bind(this));

            html += '</tbody></table>';
            $tableContainer.html(html);
        },

        /**
         * Get status information
         */
        getStatusInfo: function(status) {
            const statusMap = {
                'available': {
                    icon: '✅',
                    text: this.translate('Fully available natively')
                },
                'partial': {
                    icon: '⚠️',
                    text: this.translate('Partially available')
                },
                'limited': {
                    icon: '🔧',
                    text: this.translate('Limited or requires extension')
                },
                'not_available': {
                    icon: '❌',
                    text: this.translate('Not available')
                }
            };

            return statusMap[status] || statusMap['not_available'];
        },

        /**
         * Render Mermaid diagram
         */
        renderMermaidDiagram: function() {
            if (!this.currentData || typeof mermaid === 'undefined') return;

            const diagramCode = this.generateMermaidCode();
            const $diagramContainer = $('#fm-feature-diagram');

            // Set the Mermaid code
            $diagramContainer.text(diagramCode);
            
            // Remove data-processed attribute for re-rendering
            $diagramContainer.removeAttr('data-processed');

            // Render the diagram
            mermaid.run({
                nodes: [$diagramContainer.get(0)]
            }).catch((error) => {
                console.error('Mermaid rendering error:', error);
            });
        },

        /**
         * Generate Mermaid diagram code
         */
        generateMermaidCode: function() {
            let code = 'mindmap\n';
            code += '  root((ThemisDB Features))\n';

            // Group features by category
            const categories = {};
            this.currentData.features.forEach(function(feature) {
                if (!categories[feature.category]) {
                    categories[feature.category] = [];
                }
                categories[feature.category].push(feature);
            });

            // Generate diagram for each category
            Object.keys(categories).forEach(function(category) {
                const categoryName = category.replace(/_/g, ' ').replace(/\b\w/g, l => l.toUpperCase());
                code += '    ' + categoryName + '\n';

                categories[category].forEach(function(feature) {
                    code += '      ' + feature.name + '\n';
                });
            });

            return code;
        },

        /**
         * Export data as CSV
         */
        exportCSV: function() {
            if (!this.currentData) return;

            let csv = 'Feature,';
            csv += this.currentData.databases.map(db => db.charAt(0).toUpperCase() + db.slice(1)).join(',');
            csv += ',Category,Description\n';

            this.currentData.features.forEach(function(feature) {
                csv += '"' + feature.name + '",';
                csv += this.currentData.databases.map(db => {
                    const status = feature[db] || 'not_available';
                    return this.getStatusInfo(status).text;
                }.bind(this)).join(',');
                csv += ',"' + feature.category + '",';
                csv += '"' + (feature.description || '') + '"\n';
            }.bind(this));

            // Create download
            const blob = new Blob([csv], { type: 'text/csv' });
            const url = window.URL.createObjectURL(blob);
            const a = document.createElement('a');
            a.href = url;
            a.download = 'themisdb-feature-matrix-' + Date.now() + '.csv';
            a.click();
            window.URL.revokeObjectURL(url);
        },

        /**
         * Export as PDF
         */
        exportPDF: function() {
            window.print();
        },

        /**
         * Show loading state
         */
        showLoading: function() {
            $('#fm-loading').show();
            $('#fm-matrix-table').css('opacity', '0.3');
        },

        /**
         * Hide loading state
         */
        hideLoading: function() {
            $('#fm-loading').hide();
            $('#fm-matrix-table').css('opacity', '1');
        },

        /**
         * Show error message
         */
        showError: function(message) {
            const $tableContainer = $('#fm-matrix-table');
            const html = '<div class="themisdb-error">' +
                        '<p><strong>⚠️ Error:</strong> ' + message + '</p>' +
                        '</div>';
            $tableContainer.html(html);
        },

        /**
         * Simple translation helper
         */
        translate: function(text) {
            // In production, this would use WordPress i18n
            return text;
        }
    };

    // Initialize on document ready
    $(document).ready(function() {
        if ($('.themisdb-feature-wrapper').length > 0) {
            window.ThemisDBFeatureMatrix.init();
        }
    });

})(jQuery);
