/**
 * ThemisDB Feature Matrix JavaScript
 * Version 1.0.0 - Interactive feature comparison with sorting, filtering, CSV export, and mobile card view
 */

(function($) {
    'use strict';

    // Global namespace
    window.ThemisDBFeatureMatrix = {
        currentData: null,
        settings: {},
        currentCategory: 'all',
        sortColumn: null,
        sortDirection: 'desc',

        /**
         * Initialize the matrix
         */
        init: function() {
            // Store settings from PHP
            this.settings = themisdbFM.settings || {};

            // Set up event listeners
            this.setupEventListeners();

            // Load initial data
            this.loadFeatures();
            
            // Check window size and switch views
            this.checkMobileView();
            $(window).on('resize', this.checkMobileView.bind(this));
        },

        /**
         * Check if mobile view should be used
         */
        checkMobileView: function() {
            const isMobile = $(window).width() < 768;
            if (isMobile) {
                $('.themisdb-matrix-table table').hide();
                this.renderMobileCardView();
            } else {
                $('.themisdb-matrix-table table').show();
                $('.mobile-card-view').hide();
            }
        },

        /**
         * Setup event listeners
         */
        setupEventListeners: function() {
            const self = this;

            // Category filter buttons
            $('.category-filter-btn').on('click', function(e) {
                e.preventDefault();
                const category = $(this).data('category');
                self.filterByCategory(category);
            });

            // Filter changes (dropdown)
            $('#fm-category-filter').on('change', function() {
                self.currentCategory = $(this).val();
                self.loadFeatures();
            });

            // Refresh button
            $('#fm-refresh-data').on('click', function(e) {
                e.preventDefault();
                self.loadFeatures();
            });

            // Export CSV button
            $('#fm-export-csv, .fm-export-csv').on('click', function(e) {
                e.preventDefault();
                self.exportCSV();
            });

            // Print button
            $('#fm-print').on('click', function(e) {
                e.preventDefault();
                window.print();
            });
            
            // Column sorting (delegated event for dynamically added headers)
            // Support both click and keyboard navigation
            $(document).on('click keydown', '.sortable-header', function(e) {
                // For keyboard: only respond to Enter or Space
                if (e.type === 'keydown' && (e.key !== 'Enter' && e.key !== ' ')) {
                    return;
                }
                
                // Prevent default for keyboard events
                if (e.type === 'keydown') {
                    e.preventDefault();
                }
                
                const column = $(this).data('column');
                self.sortByColumn(column);
            });
        },

        /**
         * Filter by category
         */
        filterByCategory: function(category) {
            this.currentCategory = category;
            
            // Update active button
            $('.category-filter-btn').removeClass('active');
            $('.category-filter-btn[data-category="' + category + '"]').addClass('active');
            
            // Update dropdown if exists
            $('#fm-category-filter').val(category);
            
            // Reload features
            this.loadFeatures();
        },

        /**
         * Load feature data via AJAX
         */
        loadFeatures: function() {
            const self = this;

            // Show loading state
            this.showLoading();

            $.ajax({
                url: themisdbFM.ajax_url,
                type: 'POST',
                data: {
                    action: 'themisdb_fm_get_features',
                    nonce: themisdbFM.nonce,
                    category: self.currentCategory
                },
                success: function(response) {
                    if (response.success) {
                        self.currentData = response.data;
                        self.renderTable();
                        self.checkMobileView(); // Render appropriate view
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
         * Sort table by column
         */
        sortByColumn: function(column) {
            if (!this.currentData || !this.currentData.features) return;
            
            // Toggle sort direction if same column
            if (this.sortColumn === column) {
                this.sortDirection = this.sortDirection === 'asc' ? 'desc' : 'asc';
            } else {
                this.sortColumn = column;
                this.sortDirection = 'desc'; // Default to descending (full > limited > no)
            }
            
            // Sort features
            const self = this;
            this.currentData.features.sort(function(a, b) {
                const statusA = a[column] || 'no';
                const statusB = b[column] || 'no';
                
                // Convert status to score for sorting
                const scoreA = self.getStatusScore(statusA);
                const scoreB = self.getStatusScore(statusB);
                
                if (self.sortDirection === 'asc') {
                    return scoreA - scoreB;
                } else {
                    return scoreB - scoreA;
                }
            });
            
            // Re-render table
            this.renderTable();
            this.checkMobileView();
        },
        
        /**
         * Get numeric score for status
         */
        getStatusScore: function(status) {
            const scores = {
                'full': 2,
                'limited': 1,
                'no': 0
            };
            return scores[status] !== undefined ? scores[status] : 0;
        },

        /**
         * Render the table
         */
        renderTable: function() {
            if (!this.currentData) return;

            const $tableContainer = $('#fm-matrix-table');
            const highlightThemis = this.settings.show_themis_highlight === 'yes';
            const stickyHeader = this.settings.sticky_header === 'yes';
            const enableTooltips = this.settings.enable_tooltips === 'yes';

            let tableClass = 'themisdb-matrix-table';
            if (highlightThemis) tableClass += ' highlight-themis';
            if (stickyHeader) tableClass += ' sticky-header';

            let html = '<div class="' + tableClass + '"><table role="table">';
            html += '<thead><tr>';
            html += '<th scope="col">' + this.translate('Feature') + '</th>';

            // Add column for each database with sortable headers
            const self = this;
            this.currentData.databases.forEach(function(db) {
                const dbInfo = self.currentData.database_info[db];
                const dbName = dbInfo ? dbInfo.name : db.charAt(0).toUpperCase() + db.slice(1);
                const dbClass = 'db-' + db + (db === 'themisdb' && highlightThemis ? ' db-themisdb' : '');
                const sortClass = self.sortColumn === db ? ' sorted-' + self.sortDirection : '';
                const ariaSort = self.sortColumn === db ? self.sortDirection : 'none';
                const ariaLabel = dbName + ' - ' + self.translate('click to sort');
                
                html += '<th scope="col" class="sortable-header ' + dbClass + sortClass + '" data-column="' + db + '" role="button" tabindex="0" aria-sort="' + ariaSort + '" aria-label="' + self.escapeHtml(ariaLabel) + '">';
                html += dbName;
                html += '</th>';
            });

            html += '</tr></thead><tbody>';

            // Add rows for each feature
            this.currentData.features.forEach(function(feature) {
                const rowClass = feature.exclusive ? 'feature-row exclusive' : 'feature-row';
                html += '<tr class="' + rowClass + '">';
                
                // Feature name column
                html += '<td>';
                html += '<div class="feature-name">' + self.escapeHtml(feature.name);
                
                if (enableTooltips && feature.tooltip) {
                    html += ' <span class="info-icon" data-tooltip="' + self.escapeHtml(feature.tooltip) + '"></span>';
                }
                
                html += '</div>';
                
                if (feature.description) {
                    html += '<div class="feature-description">' + self.escapeHtml(feature.description) + '</div>';
                }

                // Status columns for each database
                self.currentData.databases.forEach(function(db) {
                    const status = feature[db] || 'no';
                    const isInverted = feature.inverted || false;
                    const isText = feature.is_text || false;
                    const cellClass = 'db-' + db + (db === 'themisdb' && highlightThemis ? ' db-themisdb' : '');
                    
                    html += '<td class="text-center ' + cellClass + '">';
                    
                    if (isText) {
                        // Display as text for licensing fields
                        html += '<span class="themisdb-status-badge status-text">' + self.escapeHtml(status) + '</span>';
                    } else {
                        const statusClass = self.getStatusClass(status, isInverted);
                        const statusIcon = self.getStatusIcon(status);
                        
                        if (enableTooltips) {
                            const tooltipText = self.getStatusLabel(status, isInverted);
                            html += '<span class="themisdb-status-badge ' + statusClass + '" data-tooltip="' + tooltipText + '">';
                        } else {
                            html += '<span class="themisdb-status-badge ' + statusClass + '">';
                        }
                        
                        html += '<span class="status-icon ' + statusClass + '">' + statusIcon + '</span>';
                        html += '</span>';
                    }
                    
                    html += '</td>';
                });

                html += '</tr>';
            });

            html += '</tbody></table></div>';
            $tableContainer.html(html);
        },

        /**
         * Render mobile card view
         */
        renderMobileCardView: function() {
            if (!this.currentData) return;

            let $container = $('.mobile-card-view');
            if ($container.length === 0) {
                $('#fm-matrix-table').append('<div class="mobile-card-view"></div>');
                $container = $('.mobile-card-view');
            }

            const self = this;
            const enableTooltips = this.settings.enable_tooltips === 'yes';
            let html = '';

            this.currentData.features.forEach(function(feature) {
                const cardClass = feature.exclusive ? 'feature-card exclusive' : 'feature-card';
                html += '<div class="' + cardClass + '" role="article">';
                
                // Header
                html += '<div class="feature-card-header">';
                html += self.escapeHtml(feature.name);
                if (feature.exclusive) {
                    html += ' <span style="color: #f39c12;">⭐</span>';
                }
                html += '</div>';
                
                // Description
                if (feature.description) {
                    html += '<div class="feature-card-description">' + self.escapeHtml(feature.description) + '</div>';
                }
                
                // Database comparisons
                html += '<div class="feature-card-databases">';
                
                self.currentData.databases.forEach(function(db) {
                    const dbInfo = self.currentData.database_info[db];
                    const dbName = dbInfo ? dbInfo.name : db.charAt(0).toUpperCase() + db.slice(1);
                    const status = feature[db] || 'no';
                    const isText = feature.is_text || false;
                    
                    html += '<div class="db-comparison-item">';
                    html += '<div class="db-name">' + dbName + '</div>';
                    html += '<div class="db-status">';
                    
                    if (isText) {
                        html += '<span class="themisdb-status-badge status-text">' + self.escapeHtml(status) + '</span>';
                    } else {
                        const statusClass = self.getStatusClass(status, feature.inverted);
                        const statusIcon = self.getStatusIcon(status);
                        html += '<span class="themisdb-status-badge ' + statusClass + '">';
                        html += '<span class="status-icon ' + statusClass + '">' + statusIcon + '</span>';
                        html += '</span>';
                    }
                    
                    html += '</div>';
                    html += '</div>';
                });
                
                html += '</div>';
                html += '</div>';
            });

            $container.html(html).show();
        },

        /**
         * Get status class
         */
        getStatusClass: function(status, inverted) {
            const baseClass = 'status-' + status;
            
            // For inverted metrics (like "Cloud Vendor Lock-in"), flip the color
            if (inverted) {
                if (status === 'full') return 'status-no';
                if (status === 'no') return 'status-full';
            }
            
            return baseClass;
        },

        /**
         * Get status icon
         */
        getStatusIcon: function(status) {
            const icons = {
                'full': '✓',
                'limited': '◐',
                'no': '✗'
            };
            return icons[status] || icons['no'];
        },

        /**
         * Get status label
         */
        getStatusLabel: function(status, inverted) {
            const labels = {
                'full': 'Full Support',
                'limited': 'Limited Support',
                'no': 'Not Available'
            };
            
            let label = labels[status] || labels['no'];
            
            if (inverted) {
                if (status === 'full') label = 'High (negative)';
                if (status === 'no') label = 'None (positive)';
            }
            
            return label;
        },

        /**
         * Get status display information
         */
        exportCSV: function() {
            if (!this.currentData) return;

            let csv = 'Feature,';
            const self = this;
            
            // Header row with database names
            csv += this.currentData.databases.map(function(db) {
                const dbInfo = self.currentData.database_info[db];
                return dbInfo ? dbInfo.name : db.charAt(0).toUpperCase() + db.slice(1);
            }).join(',');
            csv += ',Category,Description\n';

            // Data rows
            this.currentData.features.forEach(function(feature) {
                // Escape and quote feature name
                csv += '"' + self.escapeCSV(feature.name) + '",';
                
                // Add status for each database
                csv += self.currentData.databases.map(function(db) {
                    const status = feature[db] || 'no';
                    if (feature.is_text) {
                        return '"' + self.escapeCSV(status) + '"';
                    }
                    return self.getStatusLabel(status, feature.inverted);
                }).join(',');
                
                // Add category and description
                csv += ',"' + self.escapeCSV(feature.category_name || feature.category) + '",';
                csv += '"' + self.escapeCSV(feature.description || '') + '"\n';
            });

            // Create download
            const blob = new Blob([csv], { type: 'text/csv;charset=utf-8;' });
            const url = window.URL.createObjectURL(blob);
            const a = document.createElement('a');
            a.href = url;
            a.download = 'themisdb-feature-matrix-' + Date.now() + '.csv';
            document.body.appendChild(a);
            a.click();
            document.body.removeChild(a);
            window.URL.revokeObjectURL(url);
        },

        /**
         * Escape CSV values
         */
        escapeCSV: function(str) {
            if (!str) return '';
            return String(str).replace(/"/g, '""');
        },

        /**
         * Escape HTML
         */
        escapeHtml: function(text) {
            if (!text) return '';
            const div = document.createElement('div');
            div.textContent = text;
            return div.innerHTML;
        },

            let csv = 'Feature,ThemisDB,PostgreSQL,MongoDB,Neo4j,Category\n';

            for (const categoryKey in this.features) {
                const category = this.features[categoryKey];

                for (const featureKey in category.features) {
                    const feature = category.features[featureKey];
                    
                    csv += '"' + feature.name + '",';
                    csv += feature.themisdb + ',';
                    csv += feature.postgresql + ',';
                    csv += feature.mongodb + ',';
                    csv += feature.neo4j + ',';
                    csv += '"' + category.name + '"\n';
                }
            }

            // Create download
            const blob = new Blob([csv], {type: 'text/csv;charset=utf-8;'});
            const link = document.createElement('a');
            const url = URL.createObjectURL(blob);
            
            const today = new Date().toISOString().split('T')[0];
            link.setAttribute('href', url);
            link.setAttribute('download', 'themisdb-feature-comparison-' + today + '.csv');
            link.style.visibility = 'hidden';
            
            document.body.appendChild(link);
            link.click();
            document.body.removeChild(link);
        },

        /**
         * Check if mobile view should be shown
         */
        showError: function(message) {
            const $tableContainer = $('#fm-matrix-table');
            const html = '<div class="themisdb-error" role="alert">' +
                        '<p><strong>⚠️ Error:</strong> ' + this.escapeHtml(message) + '</p>' +
                        '</div>';
            $tableContainer.html(html);
        },

        /**
         * Escape HTML to prevent XSS
         */
        escapeHtml: function(text) {
            const map = {
                '&': '&amp;',
                '<': '&lt;',
                '>': '&gt;',
                '"': '&quot;',
                "'": '&#039;'
            };
            return String(text).replace(/[&<>"']/g, function(m) { return map[m]; });
        }
    };

    // Initialize on document ready
    $(document).ready(function() {
        if ($('.matrix-table').length > 0) {
            FeatureMatrix.init();
        }
    });

})(jQuery);
