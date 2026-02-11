/**
 * ThemisDB Feature Matrix JavaScript
 * Interactive filtering, sorting, tooltips, and CSV export
 */

(function($) {
    'use strict';

    // Global state
    const FeatureMatrix = {
        currentCategory: 'all',
        sortColumn: null,
        sortDirection: 'asc',
        features: null,

        /**
         * Initialize the matrix
         */
        init: function() {
            this.loadFeatures();
            this.setupEventListeners();
            this.checkMobileView();
        },

        /**
         * Load feature data
         */
        loadFeatures: function() {
            const features = window.themisdbFeatureData || {};
            this.features = features;
            this.renderTable();
        },

        /**
         * Setup event listeners
         */
        setupEventListeners: function() {
            const self = this;

            // Category filtering
            $('.category-btn').on('click', function() {
                $('.category-btn').removeClass('active');
                $(this).addClass('active');
                self.currentCategory = $(this).data('category');
                self.renderTable();
            });

            // Column sorting
            $('.matrix-table thead th.sortable').on('click', function() {
                const column = $(this).data('column');
                self.toggleSort(column);
            });

            // CSV export
            $('.export-btn').on('click', function(e) {
                e.preventDefault();
                self.exportToCSV();
            });

            // Window resize for mobile view
            $(window).on('resize', function() {
                self.checkMobileView();
            });

            // Keyboard navigation
            $('.category-btn, .matrix-table th.sortable').on('keydown', function(e) {
                if (e.key === 'Enter' || e.key === ' ') {
                    e.preventDefault();
                    $(this).trigger('click');
                }
            });
        },

        /**
         * Toggle sort direction
         */
        toggleSort: function(column) {
            if (this.sortColumn === column) {
                this.sortDirection = this.sortDirection === 'asc' ? 'desc' : 'asc';
            } else {
                this.sortColumn = column;
                this.sortDirection = 'asc';
            }

            // Update UI
            $('.matrix-table thead th').removeClass('sort-asc sort-desc');
            $('.matrix-table thead th[data-column="' + column + '"]')
                .addClass('sort-' + this.sortDirection);

            this.renderTable();
        },

        /**
         * Render the table
         */
        renderTable: function() {
            if (!this.features) return;

            const $tbody = $('.matrix-table tbody');
            $tbody.empty();

            // Filter features by category
            let filteredFeatures = this.features;
            if (this.currentCategory !== 'all') {
                filteredFeatures = {};
                if (this.features[this.currentCategory]) {
                    filteredFeatures[this.currentCategory] = this.features[this.currentCategory];
                }
            }

            // Render categories and features
            for (const categoryKey in filteredFeatures) {
                const category = filteredFeatures[categoryKey];
                
                // Category header
                $tbody.append(
                    '<tr class="category-header">' +
                    '<td colspan="5">' + this.escapeHtml(category.name) + '</td>' +
                    '</tr>'
                );

                // Sort features if needed
                let features = category.features;
                if (this.sortColumn) {
                    features = this.sortFeatures(features, this.sortColumn);
                }

                // Feature rows
                for (const featureKey in features) {
                    const feature = features[featureKey];
                    const isHighlighted = feature.highlight || false;
                    const rowClass = isHighlighted ? 'highlight' : '';

                    let row = '<tr class="' + rowClass + '">';
                    
                    // Feature name with tooltip
                    row += '<td><div class="feature-name">' + this.escapeHtml(feature.name);
                    if (feature.tooltip) {
                        row += ' <span class="info-icon tooltip">ℹ️<span class="tooltiptext">' + 
                               this.escapeHtml(feature.tooltip) + '</span></span>';
                    }
                    row += '</div></td>';

                    // Database columns
                    const databases = ['themisdb', 'postgresql', 'mongodb', 'neo4j'];
                    databases.forEach(function(db) {
                        const status = feature[db] || 'no';
                        const statusInfo = this.getStatusInfo(status);
                        
                        row += '<td class="text-center">';
                        if (feature.display_text) {
                            row += '<span class="status-text">' + this.escapeHtml(status) + '</span>';
                        } else {
                            row += '<span class="status-badge status-' + status + '" ' +
                                   'role="img" aria-label="' + statusInfo.label + '">' +
                                   statusInfo.icon + '</span>';
                        }
                        row += '</td>';
                    }.bind(this));

                    row += '</tr>';
                    $tbody.append(row);
                }
            }

            // Render mobile card view
            this.renderMobileCards(filteredFeatures);
        },

        /**
         * Render mobile card view
         */
        renderMobileCards: function(filteredFeatures) {
            const $cards = $('.matrix-cards');
            $cards.empty();

            for (const categoryKey in filteredFeatures) {
                const category = filteredFeatures[categoryKey];

                for (const featureKey in category.features) {
                    const feature = category.features[featureKey];
                    
                    let card = '<div class="feature-card">';
                    card += '<h4>' + this.escapeHtml(feature.name) + '</h4>';
                    
                    if (feature.tooltip) {
                        card += '<p class="feature-tooltip">' + this.escapeHtml(feature.tooltip) + '</p>';
                    }

                    const databases = [
                        {key: 'themisdb', name: 'ThemisDB'},
                        {key: 'postgresql', name: 'PostgreSQL'},
                        {key: 'mongodb', name: 'MongoDB'},
                        {key: 'neo4j', name: 'Neo4j'}
                    ];

                    databases.forEach(function(db) {
                        const status = feature[db.key] || 'no';
                        const statusInfo = this.getStatusInfo(status);
                        
                        card += '<div class="db-comparison">';
                        card += '<span class="db-name">' + db.name + '</span>';
                        if (feature.display_text) {
                            card += '<span class="status-text">' + this.escapeHtml(status) + '</span>';
                        } else {
                            card += '<span class="status-badge status-' + status + '">' +
                                   statusInfo.icon + '</span>';
                        }
                        card += '</div>';
                    }.bind(this));

                    card += '</div>';
                    $cards.append(card);
                }
            }
        },

        /**
         * Sort features by column
         */
        sortFeatures: function(features, column) {
            const featuresArray = Object.entries(features);
            const self = this;

            featuresArray.sort(function(a, b) {
                const aValue = a[1][column] || '';
                const bValue = b[1][column] || '';
                
                // Map status to numeric values for sorting
                const statusOrder = {'full': 3, 'limited': 2, 'no': 1};
                const aOrder = statusOrder[aValue] || 0;
                const bOrder = statusOrder[bValue] || 0;

                if (self.sortDirection === 'asc') {
                    return bOrder - aOrder; // Higher status first
                } else {
                    return aOrder - bOrder; // Lower status first
                }
            });

            // Convert back to object
            const sorted = {};
            featuresArray.forEach(function(entry) {
                sorted[entry[0]] = entry[1];
            });
            return sorted;
        },

        /**
         * Get status display information
         */
        getStatusInfo: function(status) {
            const statusMap = {
                'full': {icon: '✓', label: 'Full Support'},
                'limited': {icon: '◐', label: 'Limited Support'},
                'no': {icon: '✗', label: 'No Support'}
            };
            return statusMap[status] || statusMap['no'];
        },

        /**
         * Export to CSV
         */
        exportToCSV: function() {
            if (!this.features) return;

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
        checkMobileView: function() {
            const isMobile = window.innerWidth < 768;
            if (isMobile) {
                $('.matrix-table').hide();
                $('.matrix-cards').show();
            } else {
                $('.matrix-table').show();
                $('.matrix-cards').hide();
            }
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
