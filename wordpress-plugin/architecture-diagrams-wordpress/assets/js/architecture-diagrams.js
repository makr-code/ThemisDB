/**
 * ThemisDB Architecture Diagrams JavaScript
 * Based on TCO Calculator pattern with Mermaid.js integration
 */

(function($) {
    'use strict';

    // Global namespace
    window.ThemisDBArchitecture = {
        currentView: null,
        currentZoom: 100,
        settings: {},
        isFullscreen: false,

        /**
         * Initialize the architecture diagrams
         */
        init: function() {
            // Store settings from PHP
            this.settings = themisdbAD.settings || {};
            this.currentView = this.settings.default_view || 'high_level';

            // Initialize Mermaid
            this.initMermaid();

            // Set up event listeners
            this.setupEventListeners();

            // Load initial diagram
            this.loadDiagram(this.currentView);
        },

        /**
         * Initialize Mermaid.js
         */
        initMermaid: function() {
            if (typeof mermaid !== 'undefined') {
                mermaid.initialize({
                    startOnLoad: false,
                    theme: this.settings.theme || 'neutral',
                    securityLevel: 'loose',
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

            // View selector
            $('#ad-view-select').on('change', function() {
                const view = $(this).val();
                self.loadDiagram(view);
            });

            // Zoom controls
            $('#ad-zoom-in').on('click', function(e) {
                e.preventDefault();
                self.zoomIn();
            });

            $('#ad-zoom-out').on('click', function(e) {
                e.preventDefault();
                self.zoomOut();
            });

            $('#ad-zoom-reset').on('click', function(e) {
                e.preventDefault();
                self.zoomReset();
            });

            // Fullscreen toggle
            $('#ad-fullscreen').on('click', function(e) {
                e.preventDefault();
                self.toggleFullscreen();
            });

            // Export buttons
            $('#ad-export-svg').on('click', function(e) {
                e.preventDefault();
                self.exportSVG();
            });

            $('#ad-export-png').on('click', function(e) {
                e.preventDefault();
                self.exportPNG();
            });

            $('#ad-print').on('click', function(e) {
                e.preventDefault();
                window.print();
            });

            // Node click events (for interactivity)
            if (this.settings.interactive) {
                this.setupNodeInteractivity();
            }
        },

        /**
         * Load diagram for specified view
         */
        loadDiagram: function(view) {
            const self = this;
            this.currentView = view;

            // Show loading state
            this.showLoading();

            $.ajax({
                url: themisdbAD.ajax_url,
                type: 'POST',
                data: {
                    action: 'themisdb_ad_get_diagram',
                    nonce: themisdbAD.nonce,
                    view: view
                },
                success: function(response) {
                    if (response.success) {
                        self.renderDiagram(response.data.code);
                        self.updateDescription(view);
                    } else {
                        self.showError('Failed to load diagram');
                    }
                    self.hideLoading();
                },
                error: function() {
                    self.showError('Error loading diagram');
                    self.hideLoading();
                }
            });
        },

        /**
         * Render Mermaid diagram
         */
        renderDiagram: function(diagramCode) {
            const $container = $('#ad-mermaid-diagram');
            
            // Set the diagram code
            $container.text(diagramCode);

            // Render with Mermaid
            if (typeof mermaid !== 'undefined') {
                mermaid.run({
                    querySelector: '#ad-mermaid-diagram'
                }).then(() => {
                    // Add node interactivity after rendering
                    if (this.settings.interactive) {
                        this.setupNodeInteractivity();
                    }
                });
            }
        },

        /**
         * Update description based on view
         */
        updateDescription: function(view) {
            const descriptions = {
                'high_level': '<p>The high-level architecture shows ThemisDB\'s layered design with client interfaces, API layer, query engine, storage layer, and AI/LLM integration.</p><ul><li><strong>Client Layer:</strong> Multiple interfaces including CLI, REST, gRPC, and SDKs</li><li><strong>API Layer:</strong> RESTful and gRPC servers with authentication</li><li><strong>Query Engine:</strong> AQL parser, optimizer, and execution engine</li><li><strong>Storage Layer:</strong> RocksDB-based multi-model storage</li><li><strong>AI/LLM Layer:</strong> Integrated llama.cpp for native LLM support</li></ul>',
                'storage_layer': '<p>The storage layer architecture demonstrates ThemisDB\'s multi-model capabilities built on RocksDB foundation.</p><ul><li><strong>Index Layer:</strong> Vector (HNSW), Graph, Full-Text, and Spatial indexes</li><li><strong>Data Layer:</strong> Document, Key-Value, Time Series, and Blob storage</li><li><strong>Persistence:</strong> Write-Ahead Log, SST files, and Manifest management</li></ul>',
                'llm_integration': '<p>ThemisDB features native LLM integration using llama.cpp for in-database AI/ML operations.</p><ul><li><strong>Model Management:</strong> Dynamic model loading, caching, and quantization</li><li><strong>Inference Engine:</strong> Prompt processing, tokenization, and generation</li><li><strong>Optimization:</strong> CUDA, Metal, and SIMD acceleration support</li><li><strong>Vector Integration:</strong> Seamless connection to vector indexes for embeddings</li></ul>',
                'sharding_raid': '<p>ThemisDB supports horizontal scaling through sharding with RAID-style replication for high availability.</p><ul><li><strong>Query Router:</strong> Intelligent query distribution across shards</li><li><strong>Shard Groups:</strong> Primary node with multiple replicas</li><li><strong>Consensus:</strong> Raft protocol for distributed coordination</li><li><strong>Replication:</strong> Automatic data synchronization across replicas</li></ul>'
            };

            const $descContent = $('#ad-description-content');
            $descContent.html(descriptions[view] || descriptions['high_level']);
        },

        /**
         * Setup node interactivity
         */
        setupNodeInteractivity: function() {
            const self = this;
            
            // Wait a bit for DOM to be ready
            setTimeout(function() {
                $('.themisdb-diagram-container .node').each(function() {
                    $(this).css('cursor', 'pointer');
                    
                    $(this).off('click').on('click', function() {
                        const nodeId = $(this).attr('id');
                        const nodeText = $(this).find('text').text() || $(this).text();
                        self.showNodeDetails(nodeText, nodeId);
                    });
                });
            }, 500);
        },

        /**
         * Show node details
         */
        showNodeDetails: function(nodeName, nodeId) {
            const $panel = $('#ad-details-panel');
            const $title = $('#ad-details-title');
            const $content = $('#ad-details-content');

            $title.text(nodeName);
            $content.html('<p>Component: <strong>' + nodeName + '</strong></p><p>Click on different components in the diagram to see their details.</p>');
            
            $panel.slideDown();

            // Scroll to panel
            $('html, body').animate({
                scrollTop: $panel.offset().top - 100
            }, 500);
        },

        /**
         * Zoom controls
         */
        zoomIn: function() {
            this.currentZoom = Math.min(this.currentZoom + 10, 200);
            this.applyZoom();
        },

        zoomOut: function() {
            this.currentZoom = Math.max(this.currentZoom - 10, 50);
            this.applyZoom();
        },

        zoomReset: function() {
            this.currentZoom = 100;
            this.applyZoom();
        },

        applyZoom: function() {
            const scale = this.currentZoom / 100;
            $('#ad-mermaid-diagram').css('transform', 'scale(' + scale + ')');
        },

        /**
         * Toggle fullscreen
         */
        toggleFullscreen: function() {
            const $container = $('#ad-diagram-container');
            
            if (!this.isFullscreen) {
                $container.addClass('fullscreen');
                $('#ad-fullscreen .dashicons').removeClass('dashicons-fullscreen-alt').addClass('dashicons-fullscreen-exit-alt');
                this.isFullscreen = true;
            } else {
                $container.removeClass('fullscreen');
                $('#ad-fullscreen .dashicons').removeClass('dashicons-fullscreen-exit-alt').addClass('dashicons-fullscreen-alt');
                this.isFullscreen = false;
            }
        },

        /**
         * Export as SVG
         */
        exportSVG: function() {
            const svg = document.querySelector('#ad-mermaid-diagram svg');
            if (!svg) return;

            const svgData = new XMLSerializer().serializeToString(svg);
            const blob = new Blob([svgData], { type: 'image/svg+xml' });
            const url = URL.createObjectURL(blob);

            const a = document.createElement('a');
            a.href = url;
            a.download = 'themisdb-architecture-' + this.currentView + '.svg';
            a.click();
            URL.revokeObjectURL(url);
        },

        /**
         * Export as PNG
         */
        exportPNG: function() {
            const svg = document.querySelector('#ad-mermaid-diagram svg');
            if (!svg) return;

            const canvas = document.createElement('canvas');
            const ctx = canvas.getContext('2d');
            const svgData = new XMLSerializer().serializeToString(svg);

            const img = new Image();
            img.onload = function() {
                canvas.width = img.width;
                canvas.height = img.height;
                ctx.fillStyle = '#ffffff';
                ctx.fillRect(0, 0, canvas.width, canvas.height);
                ctx.drawImage(img, 0, 0);
                
                canvas.toBlob(function(blob) {
                    const url = URL.createObjectURL(blob);
                    const a = document.createElement('a');
                    a.href = url;
                    a.download = 'themisdb-architecture-' + this.currentView + '.png';
                    a.click();
                    URL.revokeObjectURL(url);
                }.bind(this));
            }.bind(this);

            img.src = 'data:image/svg+xml;base64,' + btoa(unescape(encodeURIComponent(svgData)));
        },

        /**
         * Show loading state
         */
        showLoading: function() {
            $('#ad-loading').show();
            $('#ad-mermaid-diagram').css('opacity', '0.3');
        },

        /**
         * Hide loading state
         */
        hideLoading: function() {
            $('#ad-loading').hide();
            $('#ad-mermaid-diagram').css('opacity', '1');
        },

        /**
         * Show error message
         */
        showError: function(message) {
            const $container = $('#ad-mermaid-diagram');
            $container.html('<div class="themisdb-error"><p><strong>⚠️ Error:</strong> ' + message + '</p></div>');
        }
    };

    // Initialize on document ready
    $(document).ready(function() {
        if ($('.themisdb-architecture-wrapper').length > 0) {
            window.ThemisDBArchitecture.init();
        }
    });

})(jQuery);
