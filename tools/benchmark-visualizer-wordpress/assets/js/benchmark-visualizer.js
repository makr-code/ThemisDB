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
         * Generate performance insights
         */
        generateInsights: function() {
            if (!this.currentData) return;

            const $insightsContainer = $('#bv-insights');
            const metric = $('#bv-metric-filter').val() || 'latency';

            // Calculate averages for each database
            const averages = this.currentData.datasets.map(function(dataset) {
                const sum = dataset.data.reduce((a, b) => a + b, 0);
                return {
                    label: dataset.label,
                    average: sum / dataset.data.length,
                    color: dataset.backgroundColor
                };
            });

            // Sort by performance (lower is better for latency, higher for throughput)
            if (metric === 'latency' || metric === 'memory') {
                averages.sort((a, b) => a.average - b.average);
            } else {
                averages.sort((a, b) => b.average - a.average);
            }

            // Generate insights HTML
            let html = '';

            // Best performer
            html += '<div class="themisdb-insight-card success">';
            html += '<h4>🏆 Best Performance</h4>';
            html += '<p><strong>' + averages[0].label + '</strong> shows the best overall performance ';
            html += 'with an average of ' + averages[0].average.toFixed(2);
            html += ' ' + (metric === 'latency' ? 'ms' : metric === 'throughput' ? 'ops/sec' : 'MB');
            html += '</p></div>';

            // ThemisDB specific insight
            const themisdbData = averages.find(d => d.label === 'ThemisDB');
            if (themisdbData && themisdbData !== averages[0]) {
                const comparison = ((Math.abs(averages[0].average - themisdbData.average) / averages[0].average) * 100).toFixed(1);
                html += '<div class="themisdb-insight-card info">';
                html += '<h4>ℹ️ ThemisDB Performance</h4>';
                html += '<p><strong>ThemisDB</strong> is within ' + comparison + '% of the best performing database';
                html += ' across all operations.</p></div>';
            } else if (themisdbData === averages[0]) {
                html += '<div class="themisdb-insight-card success">';
                html += '<h4>🚀 ThemisDB Leads</h4>';
                html += '<p><strong>ThemisDB</strong> delivers the best performance across all tested operations!</p></div>';
            }

            // Additional insight
            html += '<div class="themisdb-insight-card warning">';
            html += '<h4>💡 Key Takeaway</h4>';
            html += '<p>Performance varies by operation type. Consider your specific workload when choosing a database.</p></div>';

            $insightsContainer.html(html);
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
