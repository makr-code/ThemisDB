/**
 * ThemisDB Graph Pattern Visualizer JavaScript
 * Based on vis-network.js with Neo4j Bloom-inspired UI
 */

(function($) {
    'use strict';

    // Constants
    const MAX_VIS_LOAD_ATTEMPTS = 100;
    const VIS_CHECK_INTERVAL_MS = 100;

    // Global namespace
    window.ThemisDBGraph = {
        network: null,
        nodes: null,
        edges: null,
        groups: {},
        settings: {},
        selectedNodes: [],
        searchResults: [],

        /**
         * Initialize the graph visualizer
         */
        init: function() {
            // Store settings from PHP
            this.settings = themisdbGP.settings || {};

            // Wait for vis-network library to be loaded
            this.waitForVis().then(() => {
                // Set up event listeners
                this.setupEventListeners();

                // Load initial graph data
                this.loadGraphData();
            }).catch((error) => {
                console.error('vis-network library failed to load:', error);
                this.showError('Failed to load vis-network library from CDN.');
            });
        },

        /**
         * Wait for vis-network library to be loaded
         */
        waitForVis: function() {
            return new Promise((resolve, reject) => {
                let attempts = 0;
                
                const checkVis = () => {
                    if (typeof vis !== 'undefined' && vis.Network) {
                        console.log('vis-network library loaded successfully');
                        resolve();
                    } else if (attempts >= MAX_VIS_LOAD_ATTEMPTS) {
                        reject(new Error('vis-network library load timeout'));
                    } else {
                        attempts++;
                        setTimeout(checkVis, VIS_CHECK_INTERVAL_MS);
                    }
                };
                
                checkVis();
            });
        },

        /**
         * Load graph data via AJAX
         */
        loadGraphData: function() {
            const self = this;
            this.showLoading();

            const dataSource = $('#gp-graph-canvas').data('source') || 'default';

            $.ajax({
                url: themisdbGP.ajax_url,
                type: 'POST',
                data: {
                    action: 'themisdb_gp_get_graph_data',
                    nonce: themisdbGP.nonce,
                    data_source: dataSource
                },
                success: function(response) {
                    if (response.success) {
                        self.groups = {};
                        response.data.groups.forEach(group => {
                            self.groups[group.id] = group;
                        });
                        
                        self.renderGraph(response.data.nodes, response.data.edges);
                        self.initializeOverlay(response.data.groups);
                    } else {
                        self.showError('Failed to load graph data');
                    }
                    self.hideLoading();
                },
                error: function() {
                    self.showError('Error loading graph data');
                    self.hideLoading();
                }
            });
        },

        /**
         * Render graph using vis-network
         */
        renderGraph: function(nodesData, edgesData) {
            const container = document.getElementById('gp-graph-canvas');
            
            // Create DataSet for nodes and edges
            this.nodes = new vis.DataSet(nodesData.map(node => ({
                ...node,
                color: this.groups[node.group]?.color || '#cccccc',
                font: { color: '#24292f', size: 14 },
                borderWidth: 2,
                borderWidthSelected: 4,
                size: node.size || 25,
            })));
            
            this.edges = new vis.DataSet(edgesData.map(edge => ({
                ...edge,
                color: { color: '#d0d7de', highlight: '#2ea44f' },
                width: 2,
                smooth: this.settings.edge_smooth !== false ? {
                    type: 'cubicBezier',
                    roundness: 0.5
                } : false,
                arrows: { to: { enabled: true, scaleFactor: 0.5 } },
                font: { size: 12, align: 'middle' }
            })));

            const data = {
                nodes: this.nodes,
                edges: this.edges
            };

            const options = {
                layout: this.getLayoutOptions(),
                physics: {
                    enabled: this.settings.enable_physics !== false,
                    barnesHut: {
                        gravitationalConstant: -2000,
                        centralGravity: 0.3,
                        springLength: 120,
                        springConstant: 0.04,
                        damping: 0.09,
                        avoidOverlap: 0.5
                    },
                    stabilization: {
                        iterations: 200
                    }
                },
                interaction: {
                    hover: true,
                    tooltipDelay: 200,
                    navigationButtons: true,
                    keyboard: true,
                    multiselect: true
                },
                nodes: {
                    shape: 'dot',
                    scaling: {
                        min: 20,
                        max: 40,
                        label: {
                            enabled: true,
                            min: 14,
                            max: 18
                        }
                    },
                    font: {
                        size: 14,
                        face: '-apple-system, BlinkMacSystemFont, Segoe UI, Roboto'
                    }
                },
                edges: {
                    smooth: {
                        type: 'cubicBezier'
                    }
                }
            };

            // Create network
            this.network = new vis.Network(container, data, options);

            // Set up network event listeners
            this.setupNetworkEvents();

            // Update node count
            this.updateNodeCount();
        },

        /**
         * Get layout options based on settings
         */
        getLayoutOptions: function() {
            const layout = this.settings.default_layout || 'force_directed';
            
            const layouts = {
                force_directed: {
                    improvedLayout: true,
                    hierarchical: false
                },
                hierarchical_top: {
                    hierarchical: {
                        direction: 'UD',
                        sortMethod: 'directed',
                        levelSeparation: 150,
                        nodeSpacing: 200
                    }
                },
                hierarchical_left: {
                    hierarchical: {
                        direction: 'LR',
                        sortMethod: 'directed',
                        levelSeparation: 200,
                        nodeSpacing: 150
                    }
                },
                circular: {
                    improvedLayout: false,
                    hierarchical: false
                }
            };

            return layouts[layout] || layouts.force_directed;
        },

        /**
         * Setup network event listeners
         */
        setupNetworkEvents: function() {
            const self = this;

            // Node click
            this.network.on('click', function(params) {
                if (params.nodes.length > 0) {
                    self.showNodeDetails(params.nodes[0]);
                    self.selectedNodes = params.nodes;
                }
            });

            // Node hover
            this.network.on('hoverNode', function(params) {
                self.showTooltip(params.node, params.event);
            });

            this.network.on('blurNode', function() {
                self.hideTooltip();
            });

            // Double click to expand
            this.network.on('doubleClick', function(params) {
                if (params.nodes.length > 0) {
                    self.expandNode(params.nodes[0]);
                }
            });
        },

        /**
         * Setup UI event listeners
         */
        setupEventListeners: function() {
            const self = this;

            // Layout selector
            $('#gp-layout-select').on('change', function() {
                self.settings.default_layout = $(this).val();
                self.updateLayout();
            });

            // Zoom controls
            $('#gp-zoom-in').on('click', function(e) {
                e.preventDefault();
                self.zoomIn();
            });

            $('#gp-zoom-out').on('click', function(e) {
                e.preventDefault();
                self.zoomOut();
            });

            $('#gp-zoom-fit').on('click', function(e) {
                e.preventDefault();
                self.fit();
            });

            // Fullscreen toggle
            $('#gp-fullscreen').on('click', function(e) {
                e.preventDefault();
                self.toggleFullscreen();
            });

            // Overlay toggle
            $('#gp-toggle-overlay').on('click', function(e) {
                e.preventDefault();
                self.toggleOverlay();
            });

            $('.themisdb-overlay-close').on('click', function(e) {
                e.preventDefault();
                self.toggleOverlay();
            });

            // Search
            $('#gp-search-input').on('input', function() {
                self.handleSearch($(this).val());
            });

            // Export buttons
            $('#gp-export-png').on('click', function(e) {
                e.preventDefault();
                self.exportPNG();
            });

            $('#gp-export-json').on('click', function(e) {
                e.preventDefault();
                self.exportJSON();
            });

            // Physics toggle
            $('#gp-toggle-physics').on('change', function() {
                self.togglePhysics($(this).is(':checked'));
            });

            // Labels toggle
            $('#gp-toggle-labels').on('change', function() {
                self.toggleLabels($(this).is(':checked'));
            });

            // Node spacing slider
            $('#gp-node-spacing').on('input', function() {
                const value = $(this).val();
                $('#gp-node-spacing-value').text(value);
                self.updateNodeSpacing(value);
            });

            // Edge strength slider
            $('#gp-edge-strength').on('input', function() {
                const value = $(this).val();
                $('#gp-edge-strength-value').text(value);
                self.updateEdgeStrength(value);
            });

            // Group filter checkboxes (delegated)
            $(document).on('change', '.gp-group-filter', function() {
                const groupId = $(this).data('group');
                const visible = $(this).is(':checked');
                self.toggleGroup(groupId, visible);
            });

            // Color pickers (delegated)
            $(document).on('change', '.gp-group-color', function() {
                const groupId = $(this).data('group');
                const color = $(this).val();
                self.updateGroupColor(groupId, color);
            });
        },

        /**
         * Initialize overlay panel with groups
         */
        initializeOverlay: function(groups) {
            const $filterList = $('#gp-filter-list');
            $filterList.empty();

            groups.forEach(group => {
                const nodeCount = this.getGroupNodeCount(group.id);
                
                const html = `
                    <div class="themisdb-filter-item">
                        <label class="themisdb-filter-checkbox">
                            <input type="checkbox" 
                                   class="gp-group-filter" 
                                   data-group="${group.id}" 
                                   checked>
                            <span class="themisdb-filter-color" 
                                  style="background-color: ${group.color}"></span>
                            <span class="themisdb-filter-label">${group.label}</span>
                        </label>
                        <span class="themisdb-filter-count">${nodeCount}</span>
                        <input type="color" 
                               class="themisdb-color-picker gp-group-color" 
                               data-group="${group.id}"
                               value="${group.color}"
                               title="Change color">
                    </div>
                `;
                
                $filterList.append(html);
            });
        },

        /**
         * Get node count for a group
         */
        getGroupNodeCount: function(groupId) {
            if (!this.nodes) return 0;
            return this.nodes.get({
                filter: node => node.group === groupId
            }).length;
        },

        /**
         * Toggle group visibility
         */
        toggleGroup: function(groupId, visible) {
            if (!this.nodes) return;

            const groupNodes = this.nodes.get({
                filter: node => node.group === groupId
            });

            groupNodes.forEach(node => {
                this.nodes.update({
                    id: node.id,
                    hidden: !visible
                });
            });

            this.groups[groupId].visible = visible;
            this.updateNodeCount();
        },

        /**
         * Update group color
         */
        updateGroupColor: function(groupId, color) {
            if (!this.nodes) return;

            const groupNodes = this.nodes.get({
                filter: node => node.group === groupId
            });

            groupNodes.forEach(node => {
                this.nodes.update({
                    id: node.id,
                    color: color
                });
            });

            this.groups[groupId].color = color;
            
            // Update color indicator using data attribute
            $('.gp-group-color[data-group="' + groupId + '"]')
                .closest('.themisdb-filter-item')
                .find('.themisdb-filter-color')
                .css('background-color', color);
        },

        /**
         * Handle search
         */
        handleSearch: function(query) {
            if (!this.nodes || !query) {
                this.clearSearch();
                return;
            }

            query = query.toLowerCase();
            const results = this.nodes.get({
                filter: node => {
                    return node.label && node.label.toLowerCase().includes(query);
                }
            });

            this.searchResults = results.map(n => n.id);
            this.highlightSearchResults();
        },

        /**
         * Highlight search results
         */
        highlightSearchResults: function() {
            if (!this.network || !this.nodes) return;

            // Reset all nodes
            this.nodes.get().forEach(node => {
                this.nodes.update({
                    id: node.id,
                    borderWidth: 2,
                    opacity: this.searchResults.length === 0 ? 1 : 0.3
                });
            });

            // Highlight search results
            this.searchResults.forEach(nodeId => {
                this.nodes.update({
                    id: nodeId,
                    borderWidth: 4,
                    opacity: 1
                });
            });

            // Focus on first result if any
            if (this.searchResults.length > 0) {
                this.network.focus(this.searchResults[0], {
                    scale: 1.5,
                    animation: true
                });
            }
        },

        /**
         * Clear search
         */
        clearSearch: function() {
            this.searchResults = [];
            if (!this.nodes) return;

            this.nodes.get().forEach(node => {
                this.nodes.update({
                    id: node.id,
                    borderWidth: 2,
                    opacity: 1
                });
            });
        },

        /**
         * Show node details
         */
        showNodeDetails: function(nodeId) {
            const node = this.nodes.get(nodeId);
            if (!node) return;

            const group = this.groups[node.group];
            
            const html = `
                <div class="themisdb-node-details">
                    <h4>${node.label}</h4>
                    <div class="themisdb-node-property">
                        <div class="themisdb-node-property-key">ID:</div>
                        <div class="themisdb-node-property-value">${node.id}</div>
                    </div>
                    <div class="themisdb-node-property">
                        <div class="themisdb-node-property-key">Group:</div>
                        <div class="themisdb-node-property-value">
                            <span class="themisdb-filter-color" style="background-color: ${node.color}; display: inline-block; vertical-align: middle;"></span>
                            ${group ? group.label : node.group}
                        </div>
                    </div>
                    ${node.level ? `
                    <div class="themisdb-node-property">
                        <div class="themisdb-node-property-key">Level:</div>
                        <div class="themisdb-node-property-value">${node.level}</div>
                    </div>
                    ` : ''}
                    <div class="themisdb-node-property">
                        <div class="themisdb-node-property-key">Connections:</div>
                        <div class="themisdb-node-property-value">${this.getNodeConnections(nodeId)}</div>
                    </div>
                </div>
            `;

            $('#gp-node-details').html(html).slideDown();
        },

        /**
         * Get node connections count
         */
        getNodeConnections: function(nodeId) {
            if (!this.edges) return 0;
            return this.edges.get({
                filter: edge => edge.from === nodeId || edge.to === nodeId
            }).length;
        },

        /**
         * Show tooltip
         */
        showTooltip: function(nodeId, event) {
            const node = this.nodes.get(nodeId);
            if (!node) return;

            const tooltip = $('<div class="themisdb-tooltip"></div>')
                .text(node.label)
                .css({
                    left: event.pageX + 10 + 'px',
                    top: event.pageY + 10 + 'px'
                })
                .appendTo('body');
        },

        /**
         * Hide tooltip
         */
        hideTooltip: function() {
            $('.themisdb-tooltip').remove();
        },

        /**
         * Expand node (placeholder for future feature)
         */
        expandNode: function(nodeId) {
            console.log('Expand node:', nodeId);
            // Future: Load connected nodes dynamically
        },

        /**
         * Update layout
         */
        updateLayout: function() {
            if (!this.network) return;
            
            const options = {
                layout: this.getLayoutOptions()
            };
            
            this.network.setOptions(options);
        },

        /**
         * Zoom controls
         */
        zoomIn: function() {
            if (!this.network) return;
            const scale = this.network.getScale() * 1.2;
            this.network.moveTo({ scale: scale });
        },

        zoomOut: function() {
            if (!this.network) return;
            const scale = this.network.getScale() * 0.8;
            this.network.moveTo({ scale: scale });
        },

        fit: function() {
            if (!this.network) return;
            this.network.fit({
                animation: true
            });
        },

        /**
         * Toggle fullscreen
         */
        toggleFullscreen: function() {
            const $container = $('.themisdb-graph-container');
            $container.toggleClass('fullscreen');
            
            if ($container.hasClass('fullscreen')) {
                $('#gp-fullscreen .dashicons')
                    .removeClass('dashicons-fullscreen-alt')
                    .addClass('dashicons-fullscreen-exit-alt');
            } else {
                $('#gp-fullscreen .dashicons')
                    .removeClass('dashicons-fullscreen-exit-alt')
                    .addClass('dashicons-fullscreen-alt');
            }

            // Redraw network to fit new size
            setTimeout(() => {
                if (this.network) {
                    this.network.fit();
                }
            }, 300);
        },

        /**
         * Toggle overlay panel
         */
        toggleOverlay: function() {
            $('.themisdb-overlay-panel').toggleClass('hidden');
            $('#gp-toggle-overlay').toggleClass('hidden');
        },

        /**
         * Toggle physics
         */
        togglePhysics: function(enabled) {
            if (!this.network) return;
            this.network.setOptions({ physics: { enabled: enabled } });
        },

        /**
         * Toggle labels
         */
        toggleLabels: function(enabled) {
            if (!this.nodes) return;
            
            this.nodes.get().forEach(node => {
                this.nodes.update({
                    id: node.id,
                    font: enabled ? { color: '#24292f', size: 14 } : { size: 0 }
                });
            });
        },

        /**
         * Update node spacing
         */
        updateNodeSpacing: function(value) {
            if (!this.network) return;
            
            this.network.setOptions({
                physics: {
                    barnesHut: {
                        springLength: parseInt(value)
                    }
                }
            });
        },

        /**
         * Update edge strength
         */
        updateEdgeStrength: function(value) {
            if (!this.network) return;
            
            this.network.setOptions({
                physics: {
                    barnesHut: {
                        springConstant: parseFloat(value) / 100
                    }
                }
            });
        },

        /**
         * Export as PNG
         */
        exportPNG: function() {
            if (!this.network) return;

            const canvas = document.querySelector('#gp-graph-canvas canvas');
            if (!canvas) return;

            canvas.toBlob(function(blob) {
                const url = URL.createObjectURL(blob);
                const a = document.createElement('a');
                a.href = url;
                a.download = 'themisdb-graph-' + Date.now() + '.png';
                a.click();
                URL.revokeObjectURL(url);
            });
        },

        /**
         * Export as JSON
         */
        exportJSON: function() {
            if (!this.nodes || !this.edges) return;

            const data = {
                nodes: this.nodes.get(),
                edges: this.edges.get(),
                groups: this.groups
            };

            const json = JSON.stringify(data, null, 2);
            const blob = new Blob([json], { type: 'application/json' });
            const url = URL.createObjectURL(blob);
            const a = document.createElement('a');
            a.href = url;
            a.download = 'themisdb-graph-' + Date.now() + '.json';
            a.click();
            URL.revokeObjectURL(url);
        },

        /**
         * Update node count display
         */
        updateNodeCount: function() {
            if (!this.nodes) return;
            
            const visibleNodes = this.nodes.get({
                filter: node => !node.hidden
            }).length;
            
            const totalNodes = this.nodes.length;
            
            $('#gp-node-count').text(`${visibleNodes} / ${totalNodes} nodes`);
        },

        /**
         * Show loading state
         */
        showLoading: function() {
            $('#gp-loading').show();
            $('#gp-graph-canvas').hide();
        },

        /**
         * Hide loading state
         */
        hideLoading: function() {
            $('#gp-loading').hide();
            $('#gp-graph-canvas').show();
        },

        /**
         * Show error message
         */
        showError: function(message) {
            const $container = $('#gp-graph-canvas');
            $container.html(`
                <div style="text-align: center; padding: 40px; color: #e74c3c;">
                    <p><strong>Error:</strong> ${message}</p>
                </div>
            `);
        }
    };

    // Initialize on document ready
    $(document).ready(function() {
        if ($('#gp-graph-canvas').length > 0) {
            ThemisDBGraph.init();
        }
    });

})(jQuery);
