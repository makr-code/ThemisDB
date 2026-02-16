/**
 * ThemisDB Benchmark Visualizer JavaScript
 * Based on TCO Calculator pattern with Chart.js integration
 */

(function($) {
    'use strict';

    // Global namespace
    window.ThemisDBBenchmarks = {
        chart: null,
        currentData: null,
        settings: {},

        /**
         * Initialize the visualizer
         */
        init: function() {
            // Store settings from PHP
            this.settings = themisdbBV.settings || {};

            // Set up event listeners
            this.setupEventListeners();

            // Load initial data
            this.loadData();
        },

        /**
         * Setup event listeners
         */
        setupEventListeners: function() {
            const self = this;

            // Filter changes
            $('#bv-category-filter, #bv-metric-filter, #bv-chart-type').on('change', function() {
                self.loadData();
            });

            // Refresh button
            $('#bv-refresh-data').on('click', function(e) {
                e.preventDefault();
                self.refreshData();
            });

            // Export buttons
            $('#bv-export-csv').on('click', function(e) {
                e.preventDefault();
                self.exportCSV();
            });

            $('#bv-export-pdf').on('click', function(e) {
                e.preventDefault();
                self.exportPDF();
            });

            $('#bv-print').on('click', function(e) {
                e.preventDefault();
                window.print();
            });
        },

        /**
         * Load benchmark data via AJAX
         */
        loadData: function() {
            const self = this;
            const category = $('#bv-category-filter').val() || 'all';
            const metric = $('#bv-metric-filter').val() || 'latency';

            // Show loading state
            this.showLoading();

            $.ajax({
                url: themisdbBV.ajax_url,
                type: 'POST',
                data: {
                    action: 'themisdb_bv_get_data',
                    nonce: themisdbBV.nonce,
                    category: category,
                    metric: metric
                },
                success: function(response) {
                    if (response.success) {
                        self.currentData = response.data;
                        self.renderChart();
                        self.renderResultsTable();
                        self.generateInsights();
                    } else {
                        self.showError('Failed to load benchmark data');
                    }
                    self.hideLoading();
                },
                error: function() {
                    self.showError('Error loading benchmark data');
                    self.hideLoading();
                }
            });
        },

        /**
         * Refresh data (clear cache)
         */
        refreshData: function() {
            // In production, this would make an AJAX call to clear the cache
            this.loadData();
        },

        /**
         * Render chart using Chart.js
         */
        renderChart: function() {
            if (!this.currentData) return;

            const ctx = document.getElementById('themisdb-benchmark-chart');
            if (!ctx) return;

            const chartType = $('#bv-chart-type').val() || 'bar';
            const metric = $('#bv-metric-filter').val() || 'latency';

            // Destroy existing chart
            if (this.chart) {
                this.chart.destroy();
            }

            // Chart configuration
            const config = {
                type: chartType,
                data: {
                    labels: this.currentData.labels,
                    datasets: this.currentData.datasets
                },
                options: {
                    responsive: true,
                    maintainAspectRatio: false,
                    plugins: {
                        title: {
                            display: true,
                            text: this.getChartTitle(metric),
                            font: {
                                size: 18,
                                weight: 'bold'
                            }
                        },
                        legend: {
                            display: true,
                            position: 'top',
                        },
                        tooltip: {
                            mode: 'index',
                            intersect: false,
                            callbacks: {
                                label: function(context) {
                                    let label = context.dataset.label || '';
                                    if (label) {
                                        label += ': ';
                                    }
                                    label += context.parsed.y.toFixed(2);
                                    label += ' ' + (metric === 'latency' ? 'ms' : 
                                                   metric === 'throughput' ? 'ops/sec' : 'MB');
                                    return label;
                                }
                            }
                        }
                    },
                    scales: {
                        y: {
                            beginAtZero: true,
                            title: {
                                display: true,
                                text: this.getYAxisLabel(metric)
                            }
                        },
                        x: {
                            title: {
                                display: true,
                                text: 'Operation Type'
                            }
                        }
                    }
                }
            };

            // Create chart
            this.chart = new Chart(ctx, config);
        },

        /**
         * Get chart title based on metric
         */
        getChartTitle: function(metric) {
            const titles = {
                latency: 'Performance Comparison: Latency (Lower is Better)',
                throughput: 'Performance Comparison: Throughput (Higher is Better)',
                memory: 'Performance Comparison: Memory Usage'
            };
            return titles[metric] || 'Performance Comparison';
        },

        /**
         * Get Y-axis label based on metric
         */
        getYAxisLabel: function(metric) {
            const labels = {
                latency: 'Latency (ms)',
                throughput: 'Throughput (operations/second)',
                memory: 'Memory Usage (MB)'
            };
            return labels[metric] || 'Value';
        },

        /**
         * Render results table
         */
        renderResultsTable: function() {
            if (!this.currentData) return;

            const $tableContainer = $('#bv-results-table');
            const metric = $('#bv-metric-filter').val() || 'latency';

            // Render statistics summary first
            this.renderStatsSummary();

            let html = '<table>';
            html += '<thead><tr>';
            html += '<th>Operation</th>';

            // Add column for each dataset (database)
            this.currentData.datasets.forEach(function(dataset) {
                html += '<th>' + dataset.label + '</th>';
            });

            html += '</tr></thead><tbody>';

            // Add rows for each label (operation)
            this.currentData.labels.forEach(function(label, index) {
                html += '<tr>';
                html += '<td><strong>' + label + '</strong></td>';

                // Add data for each dataset
                this.currentData.datasets.forEach(function(dataset) {
                    const value = dataset.data[index];
                    const unit = metric === 'latency' ? ' ms' : 
                                metric === 'throughput' ? ' ops/s' : ' MB';
                    html += '<td class="value-cell">' + value.toFixed(2) + unit + '</td>';
                });

                html += '</tr>';
            }.bind(this));

            html += '</tbody></table>';
            $tableContainer.html(html);
        },

        /**
         * Render statistics summary
         */
        renderStatsSummary: function() {
            if (!this.currentData || !this.currentData.summary) return;

            const $summaryContainer = $('#bv-stats-summary');
            const summary = this.currentData.summary;
            const metric = $('#bv-metric-filter').val() || 'latency';
            
            let html = '<div class="themisdb-stats-grid">';
            
            // Total benchmarks
            html += '<div class="themisdb-stat-card">';
            html += '<div class="stat-icon">📊</div>';
            html += '<div class="stat-value">' + summary.total_benchmarks + '</div>';
            html += '<div class="stat-label">Total Benchmarks';
            if (summary.displayed_benchmarks && summary.displayed_benchmarks < summary.total_benchmarks) {
                html += ' (' + summary.displayed_benchmarks + ' shown)';
            }
            html += '</div>';
            html += '</div>';
            
            // Files parsed
            if (summary.files_parsed) {
                html += '<div class="themisdb-stat-card">';
                html += '<div class="stat-icon">📁</div>';
                html += '<div class="stat-value">' + summary.files_parsed + '</div>';
                html += '<div class="stat-label">Benchmark Files</div>';
                html += '</div>';
            }
            
            // Average time
            if (summary.avg_time) {
                html += '<div class="themisdb-stat-card">';
                html += '<div class="stat-icon">⏱️</div>';
                html += '<div class="stat-value">' + summary.avg_time.toFixed(2) + '</div>';
                html += '<div class="stat-label">Average ' + (metric === 'latency' ? 'Latency (ms)' : 
                                                             metric === 'throughput' ? 'Throughput (ops/sec)' : 'Value') + '</div>';
                html += '</div>';
            }
            
            // Fastest / Best performance
            if (summary.fastest !== null) {
                html += '<div class="themisdb-stat-card success">';
                html += '<div class="stat-icon">🚀</div>';
                html += '<div class="stat-value">' + summary.fastest.toFixed(2) + '</div>';
                html += '<div class="stat-label">';
                if (metric === 'throughput') {
                    html += 'Lowest Throughput';
                } else {
                    html += 'Best Performance';
                }
                html += '</div>';
                html += '</div>';
            }
            
            // Slowest / Worst performance or Peak
            if (summary.slowest !== null) {
                html += '<div class="themisdb-stat-card warning">';
                html += '<div class="stat-icon">🐌</div>';
                html += '<div class="stat-value">' + summary.slowest.toFixed(2) + '</div>';
                html += '<div class="stat-label">';
                if (metric === 'throughput') {
                    html += 'Peak Throughput';
                } else {
                    html += 'Slowest Operation';
                }
                html += '</div>';
                html += '</div>';
            }
            
            html += '</div>';
            
            // Add note if data is limited
            if (summary.displayed_benchmarks && summary.displayed_benchmarks < summary.total_benchmarks) {
                html += '<div class="themisdb-display-note">';
                html += '<p><small>📝 Showing top ' + summary.displayed_benchmarks + ' best-performing benchmarks out of ' + 
                        summary.total_benchmarks + ' total for better visualization.</small></p>';
                html += '</div>';
            }
            
            $summaryContainer.html(html);
        },

        /**
         * Generate performance insights
         */
        generateInsights: function() {
            if (!this.currentData) return;

            const $insightsContainer = $('#bv-insights');
            const metric = $('#bv-metric-filter').val() || 'latency';
            const category = $('#bv-category-filter').val() || 'all';
            const description = this.currentData.description;

            // Generate insights HTML
            let html = '';

            // Category-specific detailed description
            if (description) {
                html += '<div class="themisdb-insight-card info">';
                html += '<h4>📊 ' + description.title + '</h4>';
                html += '<p><strong>Getestete Operationen:</strong> ' + description.tests + '</p>';
                html += '<p><strong>Ergebnisse:</strong> ' + description.results + '</p>';
                if (description.stats_summary) {
                    html += '<p><strong>Statistik:</strong> ' + description.stats_summary + '</p>';
                }
                html += '</div>';
                
                // Valid conclusions
                if (description.conclusions && description.conclusions.valid) {
                    html += '<div class="themisdb-insight-card success">';
                    html += '<h4>✅ Gültige Schlussfolgerungen</h4>';
                    html += '<ul>';
                    description.conclusions.valid.forEach(function(conclusion) {
                        html += '<li>' + conclusion + '</li>';
                    });
                    html += '</ul>';
                    html += '</div>';
                }
                
                // Invalid conclusions
                if (description.conclusions && description.conclusions.invalid) {
                    html += '<div class="themisdb-insight-card warning">';
                    html += '<h4>⚠️ Ungültige Schlussfolgerungen</h4>';
                    html += '<ul>';
                    description.conclusions.invalid.forEach(function(conclusion) {
                        html += '<li>' + conclusion + '</li>';
                    });
                    html += '</ul>';
                    html += '</div>';
                }
            } else {
                // Fallback to summary insight if no description available
                if (this.currentData.summary) {
                    const summary = this.currentData.summary;
                    
                    html += '<div class="themisdb-insight-card success">';
                    html += '<h4>📊 Benchmark Summary</h4>';
                    html += '<p><strong>' + summary.total_benchmarks + ' benchmarks</strong> were executed in the ';
                    html += '<strong>' + this.getCategoryName(category) + '</strong> category. ';
                    
                    if (metric === 'latency') {
                        html += 'Average latency: <strong>' + summary.avg_time.toFixed(2) + ' ms</strong>. ';
                        html += 'Best performance: <strong>' + summary.fastest.toFixed(2) + ' ms</strong>.';
                    } else if (metric === 'throughput') {
                        html += 'Average throughput: <strong>' + summary.avg_time.toFixed(0) + ' ops/sec</strong>. ';
                        html += 'Peak throughput: <strong>' + summary.slowest.toFixed(0) + ' ops/sec</strong>.';
                    } else {
                        html += 'Average value: <strong>' + summary.avg_time.toFixed(2) + '</strong>.';
                    }
                    html += '</p></div>';
                }

                // Performance range insight
                if (this.currentData.summary && this.currentData.summary.fastest && this.currentData.summary.slowest) {
                    const range = this.currentData.summary.slowest - this.currentData.summary.fastest;
                    const rangePercent = (range / this.currentData.summary.avg_time * 100).toFixed(1);
                    
                    html += '<div class="themisdb-insight-card info">';
                    html += '<h4>📈 Performance Variance</h4>';
                    html += '<p>Performance varies by <strong>' + rangePercent + '%</strong> across different operations. ';
                    
                    if (metric === 'latency') {
                        if (parseFloat(rangePercent) < 50) {
                            html += 'ThemisDB shows <strong>consistent low-latency</strong> performance.';
                        } else {
                            html += 'Some operations are more complex and require more time.';
                        }
                    } else {
                        html += 'Different operations have different throughput characteristics.';
                    }
                    html += '</p></div>';
                }

                // Key takeaway
                html += '<div class="themisdb-insight-card warning">';
                html += '<h4>💡 Key Takeaway</h4>';
                html += '<p>These benchmarks show real-world performance on actual hardware. ';
                html += 'Results may vary based on your specific hardware configuration, workload patterns, and data size. ';
                html += 'Use these as a reference for understanding ThemisDB\'s performance characteristics.</p></div>';
            }

            $insightsContainer.html(html);
        },

        /**
         * Get category display name
         */
        getCategoryName: function(category) {
            const names = {
                'all': 'All Operations',
                'vector_search': 'Vector Search & Embeddings',
                'graph_traversal': 'Graph Traversal & PageRank',
                'encryption': 'Encryption & HSM',
                'compression': 'Compression',
                'transaction': 'MVCC & Transactions',
                'image_analysis': 'Image Analysis',
                'advanced': 'Advanced Patterns & AQL',
                'gpu': 'GPU Backends',
                'content': 'Content Versioning & Indexing'
            };
            return names[category] || category;
        },

        /**
         * Get category-specific insights
         */
        getCategoryInsight: function(category, metric) {
            const insights = {
                'vector_search': '<div class="themisdb-insight-card success"><h4>🎯 Vector Search Performance</h4>' +
                    '<p>ThemisDB provides <strong>native vector search</strong> capabilities with competitive performance for ' +
                    'similarity search, embeddings, and nearest-neighbor queries including GNN embeddings.</p></div>',
                    
                'graph_traversal': '<div class="themisdb-insight-card success"><h4>🕸️ Graph Processing</h4>' +
                    '<p>ThemisDB\'s graph traversal algorithms are optimized for <strong>complex relationship queries</strong>, ' +
                    'BFS/DFS operations, and graph analytics like PageRank with various graph sizes.</p></div>',
                    
                'encryption': '<div class="themisdb-insight-card success"><h4>🔒 Security Performance</h4>' +
                    '<p>Encryption operations show that ThemisDB maintains <strong>strong security</strong> with HSM provider support while ' +
                    'minimizing performance overhead through optimized cryptographic implementations.</p></div>',
                    
                'compression': '<div class="themisdb-insight-card success"><h4>📦 Compression Efficiency</h4>' +
                    '<p>ThemisDB\'s compression algorithms balance <strong>storage savings</strong> with fast ' +
                    'compression and decompression speeds for optimal data management.</p></div>',
                    
                'transaction': '<div class="themisdb-insight-card success"><h4>💼 ACID Transactions</h4>' +
                    '<p>MVCC and transaction benchmarks demonstrate ThemisDB\'s <strong>reliable ACID guarantees</strong> ' +
                    'with efficient concurrency control and minimal lock contention.</p></div>',
                    
                'image_analysis': '<div class="themisdb-insight-card success"><h4>🖼️ Image Processing</h4>' +
                    '<p>ThemisDB\'s image analysis features show strong performance for <strong>AI-powered ' +
                    'image operations</strong>, enabling multimedia database applications with low latency processing.</p></div>',
                    
                'advanced': '<div class="themisdb-insight-card success"><h4>🚀 Advanced Features</h4>' +
                    '<p>Advanced patterns, hybrid AQL queries, changefeed throughput, and micro-optimization benchmarks showcase ' +
                    'ThemisDB\'s <strong>multi-model capabilities</strong>, combining different data paradigms efficiently.</p></div>',
                    
                'gpu': '<div class="themisdb-insight-card success"><h4>⚡ GPU Acceleration</h4>' +
                    '<p>GPU backend benchmarks demonstrate ThemisDB\'s ability to leverage <strong>hardware acceleration</strong> ' +
                    'for compute-intensive operations, improving performance for complex queries.</p></div>',
                    
                'content': '<div class="themisdb-insight-card success"><h4>📝 Content Management</h4>' +
                    '<p>Content versioning and index rebuild benchmarks show ThemisDB\'s <strong>robust data management</strong> ' +
                    'capabilities for applications requiring version control and efficient index maintenance.</p></div>'
            };
            
            return insights[category] || null;
        },

        /**
         * Export data as CSV
         */
        exportCSV: function() {
            if (!this.currentData) return;

            let csv = 'Operation,';
            csv += this.currentData.datasets.map(d => d.label).join(',') + '\n';

            this.currentData.labels.forEach(function(label, index) {
                csv += label + ',';
                csv += this.currentData.datasets.map(d => d.data[index]).join(',') + '\n';
            }.bind(this));

            // Create download
            const blob = new Blob([csv], { type: 'text/csv' });
            const url = window.URL.createObjectURL(blob);
            const a = document.createElement('a');
            a.href = url;
            a.download = 'themisdb-benchmarks-' + Date.now() + '.csv';
            a.click();
            window.URL.revokeObjectURL(url);
        },

        /**
         * Export as PDF (basic implementation)
         */
        exportPDF: function() {
            // For a full implementation, you would use a library like jsPDF
            // For now, we'll just trigger print which can save as PDF
            window.print();
        },

        /**
         * Show loading state
         */
        showLoading: function() {
            $('#bv-loading').show();
            $('#themisdb-benchmark-chart').css('opacity', '0.3');
        },

        /**
         * Hide loading state
         */
        hideLoading: function() {
            $('#bv-loading').hide();
            $('#themisdb-benchmark-chart').css('opacity', '1');
        },

        /**
         * Show error message
         */
        showError: function(message) {
            const $insightsContainer = $('#bv-insights');
            const html = '<div class="themisdb-insight-card warning">' +
                        '<h4>⚠️ Error</h4>' +
                        '<p>' + message + '</p>' +
                        '</div>';
            $insightsContainer.html(html);
        }
    };

    // Initialize on document ready
    $(document).ready(function() {
        if ($('.themisdb-benchmark-wrapper').length > 0) {
            window.ThemisDBBenchmarks.init();
        }
    });

})(jQuery);
