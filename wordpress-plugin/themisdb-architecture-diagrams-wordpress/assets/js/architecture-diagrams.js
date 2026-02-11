/**
 * ThemisDB Architecture Diagrams JavaScript
 * Based on TCO Calculator pattern with Mermaid.js integration
 */

(function($) {
    'use strict';

    // Constants
    const MAX_MERMAID_LOAD_ATTEMPTS = 100; // Maximum attempts to wait for Mermaid library (10 seconds)
    const MERMAID_CHECK_INTERVAL_MS = 100; // Interval between checks in milliseconds

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

            // Wait for Mermaid library to be loaded
            this.waitForMermaid().then(() => {
                // Initialize Mermaid
                this.initMermaid();

                // Set up event listeners
                this.setupEventListeners();

                // Initialize lazy loading or load diagram immediately
                this.initLazyLoading();
            }).catch((error) => {
                console.error('Mermaid library failed to load:', error);
                this.showError('Failed to load Mermaid library from CDN. This may be due to:<br>' +
                    '• Network connectivity issues<br>' +
                    '• Content blockers or ad blockers<br>' +
                    '• Firewall restrictions<br><br>' +
                    'Please check your network connection and try refreshing the page. ' +
                    'If the problem persists, contact your site administrator.');
            });
        },

        /**
         * Wait for Mermaid library to be loaded
         */
        waitForMermaid: function() {
            return new Promise((resolve, reject) => {
                let attempts = 0;
                
                const checkMermaid = () => {
                    if (typeof mermaid !== 'undefined') {
                        console.log('Mermaid library loaded successfully after ' + attempts + ' attempts');
                        resolve();
                    } else if (attempts >= MAX_MERMAID_LOAD_ATTEMPTS) {
                        console.error('Mermaid library load timeout after ' + attempts + ' attempts');
                        reject(new Error('Mermaid library load timeout'));
                    } else {
                        attempts++;
                        setTimeout(checkMermaid, MERMAID_CHECK_INTERVAL_MS);
                    }
                };
                
                checkMermaid();
            });
        },

        /**
         * Initialize Mermaid.js
         */
        initMermaid: function() {
            if (typeof mermaid !== 'undefined') {
                // Detect color scheme for theming
                const colorScheme = this.detectColorScheme();
                const themeVars = this.getThemeVariables(colorScheme);
                
                mermaid.initialize({
                    startOnLoad: false,
                    theme: 'base',
                    themeVariables: themeVars,
                    flowchart: {
                        htmlLabels: true,
                        curve: 'basis',
                        nodeSpacing: 50,
                        rankSpacing: 50
                    },
                    securityLevel: 'loose'
                });
            }
        },
        
        /**
         * Detect color scheme
         */
        detectColorScheme: function() {
            // Check if dark mode is set via localized script
            if (typeof themisdbAD !== 'undefined' && themisdbAD.colorScheme) {
                return themisdbAD.colorScheme;
            }
            
            // Check system preference
            if (window.matchMedia && window.matchMedia('(prefers-color-scheme: dark)').matches) {
                return 'dark';
            }
            
            // Check body class
            if (document.body.classList.contains('dark-mode') || 
                document.documentElement.getAttribute('data-theme') === 'dark') {
                return 'dark';
            }
            
            return 'light';
        },
        
        /**
         * Get theme variables based on color scheme
         */
        getThemeVariables: function(colorScheme) {
            if (colorScheme === 'dark') {
                return {
                    primaryColor: '#3498db',
                    primaryTextColor: '#ffffff',
                    primaryBorderColor: '#7c4dff',
                    secondaryColor: '#7c4dff',
                    tertiaryColor: '#27ae60',
                    lineColor: '#7c4dff',
                    textColor: '#ecf0f1',
                    mainBkg: '#1a252f',
                    secondBkg: '#2c3e50',
                    nodeBorder: '#3498db',
                    clusterBkg: '#2c3e50',
                    clusterBorder: '#3498db',
                    defaultLinkColor: '#7c4dff',
                    titleColor: '#ecf0f1',
                    edgeLabelBackground: '#1a252f',
                    errorBkgColor: '#e74c3c',
                    errorTextColor: '#ffffff'
                };
            }
            
            // Light mode - Themis Brand Colors
            return {
                primaryColor: '#2c3e50',
                primaryTextColor: '#ffffff',
                primaryBorderColor: '#3498db',
                secondaryColor: '#7c4dff',
                tertiaryColor: '#27ae60',
                lineColor: '#3498db',
                textColor: '#2c3e50',
                mainBkg: '#ffffff',
                secondBkg: '#ecf0f1',
                nodeBorder: '#3498db',
                clusterBkg: '#ecf0f1',
                clusterBorder: '#2c3e50',
                defaultLinkColor: '#3498db',
                titleColor: '#2c3e50',
                edgeLabelBackground: '#ffffff',
                errorBkgColor: '#e74c3c',
                errorTextColor: '#ffffff'
            };
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
            
            // Export Mermaid code button
            $('#ad-export-code').on('click', function(e) {
                e.preventDefault();
                self.exportMermaidCode();
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
            
            // Clear previous content
            $container.empty();
            
            // Set the diagram code as text content
            $container.text(diagramCode);
            
            // Remove any data-processed attribute from previous renders
            $container.removeAttr('data-processed');

            // Render with Mermaid
            if (typeof mermaid !== 'undefined') {
                mermaid.run({
                    nodes: [$container[0]]
                }).then(() => {
                    // Add node interactivity after rendering
                    if (this.settings.interactive) {
                        this.setupNodeInteractivity();
                    }
                    // Initialize touch gestures after render
                    this.initTouchGestures();
                }).catch((error) => {
                    console.error('Mermaid rendering error:', error);
                    this.showError('Failed to render diagram: ' + error.message);
                });
            } else {
                // Safety check - should not reach here due to waitForMermaid() in init()
                this.showError('Mermaid library not loaded');
            }
        },

        /**
         * Update description based on view
         */
        updateDescription: function(view) {
            const descriptions = {
                'high_level': '<p>The layered architecture shows ThemisDB\'s unified design with a central Query Layer serving as the abstract programming layer.</p><ul><li><strong>Client Layer:</strong> Multiple interfaces including CLI, REST, gRPC, and SDKs</li><li><strong>API & Server Layer:</strong> HTTP/REST and gRPC servers with authentication and rate limiting</li><li><strong>Query Layer (Unified):</strong> AQL parser, optimizer, execution engine with optional LLM functions - this is the abstract programming layer that unifies all functionality</li><li><strong>Transaction Layer:</strong> MVCC, transaction manager, WAL management</li><li><strong>Index Layer:</strong> Vector, Graph, Secondary, and Fulltext indexes</li><li><strong>Storage Layer:</strong> RocksDB LSM-tree with compression and snapshot management</li></ul>',
                'storage_layer': '<p>The storage layer architecture demonstrates ThemisDB\'s multi-model capabilities built on RocksDB foundation.</p><ul><li><strong>Index Layer:</strong> Vector (HNSW), Graph, Full-Text, and Spatial indexes</li><li><strong>Data Layer:</strong> Document, Key-Value, Time Series, and Blob storage</li><li><strong>Persistence:</strong> Write-Ahead Log, SST files, and Manifest management</li></ul>',
                'llm_integration': '<p>ThemisDB\'s optional LLM plugin integrates into the unified Query Layer as specialized functions.</p><ul><li><strong>Integration Point:</strong> LLM functions (llm_generate(), llm_embed()) are part of the Query Layer function libraries</li><li><strong>Model Management:</strong> Dynamic model loading, caching, and quantization (Q4/Q5/Q8)</li><li><strong>Inference Engine:</strong> Prompt processing, tokenization, and generation</li><li><strong>Hardware Optimization:</strong> CUDA, Metal GPU acceleration, and CPU SIMD</li><li><strong>Storage Integration:</strong> Embeddings cached and indexed in the vector index</li></ul>',
                'sharding_raid': '<p>ThemisDB supports horizontal scaling through sharding with RAID-style replication for high availability.</p><ul><li><strong>Query Router:</strong> Intelligent query distribution across shards</li><li><strong>Shard Groups:</strong> Primary node with multiple replicas</li><li><strong>Consensus:</strong> Raft protocol for distributed coordination</li><li><strong>Replication:</strong> Automatic data synchronization across replicas</li></ul>',
                'database_comparison': '<p>Comparison of ThemisDB with other popular databases shows ThemisDB\'s unique advantages.</p><ul><li><strong>Multi-Model Support:</strong> ThemisDB natively supports relational, graph, vector, and document models in a single unified API</li><li><strong>Embedded LLM:</strong> Only ThemisDB includes native LLM integration without external API costs</li><li><strong>GPU Acceleration:</strong> Native GPU support for vector search and LLM inference</li><li><strong>Unified API:</strong> Compatible with PostgreSQL, MongoDB, and Neo4j protocols</li></ul>',
                'llm_comparison': '<p>ThemisDB\'s embedded LLM approach offers significant advantages over cloud-based LLM services.</p><ul><li><strong>Zero Latency:</strong> No network calls - LLM runs in-process with 0ms API latency</li><li><strong>Zero Runtime Cost:</strong> No per-token charges - one-time hardware investment</li><li><strong>Complete Privacy:</strong> Data never leaves your server - full GDPR/HIPAA compliance</li><li><strong>GPU Acceleration:</strong> Direct CUDA/Metal support for maximum performance</li><li><strong>Quantization:</strong> Q4/Q5/Q8 models for efficient memory usage</li></ul>',
                'hardware_architecture': '<p>ThemisDB is designed to leverage modern hardware capabilities for optimal performance.</p><ul><li><strong>CPU Optimization:</strong> SIMD instructions (AVX2/AVX-512) for fast processing</li><li><strong>GPU Acceleration:</strong> CUDA/Metal/Vulkan for vector search and LLM inference</li><li><strong>Memory Hierarchy:</strong> Efficient use of CPU cache, RAM, and GPU VRAM</li><li><strong>Storage Tiers:</strong> NVMe SSD for hot data, HDD for cold storage</li><li><strong>Network:</strong> High-speed NICs with optional RDMA for distributed deployments</li></ul>',
                'performance_comparison': '<p>Performance scales dramatically with hardware investment, showing clear ROI at each tier.</p><ul><li><strong>CPU Only:</strong> Good performance at lowest cost - ideal for development and small deployments</li><li><strong>Mid-Range GPU:</strong> 5-10x performance boost with RTX 4090 - best price/performance ratio</li><li><strong>High-End GPU:</strong> 20-40x performance with A100/H100 - for production workloads</li><li><strong>vs Cloud Services:</strong> Self-hosted can be 10x more cost-effective at scale while maintaining data privacy</li></ul>',
                'tco_comparison': '<p>Total Cost of Ownership analysis over 1, 3, and 5 years shows dramatic cost differences.</p><ul><li><strong>Year 1:</strong> Initial hardware investment for ThemisDB ($26K) vs cloud services ($185K)</li><li><strong>Year 3:</strong> ThemisDB cumulative cost $38K vs PostgreSQL+LLM API $213K vs cloud $545K</li><li><strong>Year 5:</strong> ThemisDB $60K (best ROI) vs PostgreSQL+LLM $345K vs cloud $905K (highest cost)</li><li><strong>Break-even:</strong> Self-hosted ThemisDB pays for itself in under 6 months compared to cloud services</li></ul>',
                'feature_matrix': '<p>Comprehensive feature comparison across all major database systems.</p><ul><li><strong>✅ Full Support:</strong> ThemisDB leads with native support for all features</li><li><strong>⚠️ Partial Support:</strong> Competitors require extensions or have limitations</li><li><strong>❌ Not Supported:</strong> Critical features missing in competitors</li><li><strong>Key Advantage:</strong> Only ThemisDB combines multi-model, embedded LLM, and GPU acceleration natively</li></ul>',
                'deployment_options': '<p>ThemisDB supports flexible deployment models to match your requirements.</p><ul><li><strong>On-Premise:</strong> Maximum privacy and performance, your hardware, CapEx model</li><li><strong>Cloud:</strong> Flexible scaling, managed infrastructure, OpEx model</li><li><strong>Hybrid:</strong> Best of both worlds - sensitive data on-prem, archives in cloud</li><li><strong>vs SaaS:</strong> Self-hosted ThemisDB avoids vendor lock-in and unpredictable costs</li></ul>',
                'use_case_recommendations': '<p>Recommended database choices for different application scenarios.</p><ul><li><strong>AI/ML Applications:</strong> ThemisDB ideal for RAG, embeddings, semantic search with zero LLM API costs</li><li><strong>Graph Applications:</strong> ThemisDB for multi-model needs, Neo4j for graph-only</li><li><strong>Complex Data:</strong> Only ThemisDB handles mixed relational, graph, vector, document in one database</li><li><strong>Privacy/Compliance:</strong> ThemisDB on-premise ensures data never leaves your infrastructure</li></ul>',
                'migration_paths': '<p>Clear migration paths from legacy databases to ThemisDB.</p><ul><li><strong>From PostgreSQL:</strong> Keep ACID and SQL compatibility, gain LLM + vector + graph capabilities</li><li><strong>From MongoDB:</strong> Keep document flexibility, gain ACID + LLM + multi-model</li><li><strong>From Neo4j:</strong> Keep graph queries, gain LLM + vector + relational models</li><li><strong>From Legacy:</strong> Modernize with all features while maintaining data integrity</li></ul>'
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
        },
        
        /**
         * Initialize lazy loading for diagrams
         */
        initLazyLoading: function() {
            // Check if lazy loading is enabled
            if (typeof themisdbAD === 'undefined' || !themisdbAD.settings.enableLazyLoading) {
                // Load initial diagram immediately
                this.loadDiagram(this.currentView);
                return;
            }
            
            // Use IntersectionObserver for lazy loading
            const self = this;
            const diagramContainer = document.getElementById('ad-diagram-container');
            
            if (!diagramContainer || !('IntersectionObserver' in window)) {
                // Fallback: load immediately if IntersectionObserver not supported
                this.loadDiagram(this.currentView);
                return;
            }
            
            const observerOptions = {
                root: null,
                rootMargin: '50px',
                threshold: 0.1
            };
            
            const observer = new IntersectionObserver(function(entries) {
                entries.forEach(function(entry) {
                    if (entry.isIntersecting && !diagramContainer.dataset.loaded) {
                        diagramContainer.dataset.loaded = 'true';
                        self.loadDiagram(self.currentView);
                        observer.unobserve(diagramContainer);
                    }
                });
            }, observerOptions);
            
            observer.observe(diagramContainer);
        },
        
        /**
         * Render single diagram (for re-rendering)
         */
        renderSingleDiagram: function(node) {
            // Remove data-processed attribute if exists (for re-rendering)
            node.removeAttribute('data-processed');
            
            // Render with Mermaid
            if (typeof mermaid !== 'undefined') {
                mermaid.run({ nodes: [node] }).catch(function(err) {
                    console.error('Mermaid rendering error:', err);
                    node.innerHTML = '<div class="mermaid-error">Failed to render diagram</div>';
                });
            }
        },
        
        /**
         * Export Mermaid source code as .mmd file
         * .mmd is the standard Mermaid diagram file extension
         */
        exportMermaidCode: function() {
            const mermaidDiv = document.querySelector('#ad-mermaid-diagram');
            if (!mermaidDiv) return;
            
            const code = mermaidDiv.textContent;
            const blob = new Blob([code], { type: 'text/plain' });
            const url = URL.createObjectURL(blob);
            
            const a = document.createElement('a');
            a.href = url;
            // .mmd extension for Mermaid diagram files
            a.download = 'themisdb-diagram-' + this.currentView + '-' + Date.now() + '.mmd';
            a.click();
            URL.revokeObjectURL(url);
        },
        
        /**
         * Initialize touch gestures for mobile
         */
        initTouchGestures: function() {
            // Only on mobile and if enabled
            if (!window.matchMedia('(max-width: 768px)').matches) {
                return;
            }
            
            const diagrams = document.querySelectorAll('.mermaid svg');
            diagrams.forEach(function(svg) {
                let currentScale = 1;
                let lastTap = 0;
                
                // Double-tap to zoom
                svg.addEventListener('touchend', function(e) {
                    const currentTime = new Date().getTime();
                    const tapLength = currentTime - lastTap;
                    
                    if (tapLength < 300 && tapLength > 0) {
                        // Double tap detected
                        e.preventDefault();
                        if (currentScale === 1) {
                            currentScale = 2;
                        } else {
                            currentScale = 1;
                        }
                        svg.style.transform = 'scale(' + currentScale + ')';
                        svg.style.transition = 'transform 0.3s ease';
                    }
                    lastTap = currentTime;
                });
            });
        }
    };

    // Initialize on document ready
    $(document).ready(function() {
        if ($('.themisdb-architecture-wrapper').length > 0) {
            window.ThemisDBArchitecture.init();
        }
    });

})(jQuery);
