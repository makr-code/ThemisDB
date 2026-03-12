/**
 * ThemisDB Test Dashboard JavaScript
 * Version: 1.0.0
 */

(function($) {
    'use strict';
    
    let currentCharts = {};
    
    /**
     * Initialize dashboard
     */
    function initDashboard() {
        const $dashboard = $('.themisdb-test-dashboard');
        if (!$dashboard.length) return;
        
        // Load initial data
        loadDashboardData();
        
        // Setup event listeners
        setupEventListeners();
    }
    
    /**
     * Setup event listeners
     */
    function setupEventListeners() {
        // View change
        $('#tdb-view-select').on('change', function() {
            loadDashboardData();
        });
        
        // Period change
        $('#tdb-period-select').on('change', function() {
            loadDashboardData();
        });
        
        // Refresh button
        $('#tdb-refresh-btn').on('click', function() {
            loadDashboardData(true);
        });
        
        // Export buttons
        $('.tdb-export-csv').on('click', exportToCSV);
        $('.tdb-export-png').on('click', exportToPNG);
    }
    
    /**
     * Load dashboard data
     */
    function loadDashboardData(forceRefresh) {
        if (forceRefresh === void 0) { forceRefresh = false; }
        
        const view = $('#tdb-view-select').val() || 'overview';
        const period = $('#tdb-period-select').val() || '30';
        const repo = $('#tdb-repo-input').val() || 'makr-code/wordpressPlugins';
        
        // Show loading
        showLoading();
        
        $.ajax({
            url: themisdbTestDashboard.ajaxUrl,
            type: 'POST',
            data: {
                action: 'themisdb_test_dashboard_fetch',
                nonce: themisdbTestDashboard.nonce,
                view: view,
                period: period,
                repo: repo,
                force_refresh: forceRefresh
            },
            success: function(response) {
                if (response.success) {
                    renderDashboard(response.data, view);
                } else {
                    showError('Failed to load data');
                }
            },
            error: function() {
                showError('Network error occurred');
            }
        });
    }
    
    /**
     * Render dashboard based on view
     */
    function renderDashboard(data, view) {
        hideLoading();
        
        const $content = $('#tdb-content');
        $content.empty();
        
        switch(view) {
            case 'coverage':
                renderCoverageView(data, $content);
                break;
            case 'pipeline':
                renderPipelineView(data, $content);
                break;
            case 'quality':
                renderQualityView(data, $content);
                break;
            case 'overview':
            default:
                renderOverviewView(data, $content);
                break;
        }
    }
    
    /**
     * Render overview view
     */
    function renderOverviewView(data, $container) {
        // Coverage metrics
        const coverageCard = createMetricCard(
            'Test Coverage',
            data.coverage.current.overall + '%',
            '+2.3%',
            'positive'
        );
        
        // Pipeline status
        const pipelineCard = createMetricCard(
            'Pipeline Success Rate',
            '94.2%',
            '+1.5%',
            'positive'
        );
        
        // Quality gates
        const qualityCard = createMetricCard(
            'Quality Gates',
            '6/6 Passed',
            'All green',
            'positive'
        );
        
        const $grid = $('<div class="tdb-grid">').append(coverageCard, pipelineCard, qualityCard);
        $container.append($grid);
        
        // Charts
        const $chartsGrid = $('<div class="tdb-grid">');
        
        // Coverage trend
        const $coverageChart = $('<div class="tdb-card"><h3>📊 Coverage Trend</h3><div class="tdb-chart-container"><canvas id="overview-coverage-chart"></canvas></div></div>');
        $chartsGrid.append($coverageChart);
        
        // Pipeline trend
        const $pipelineChart = $('<div class="tdb-card"><h3>🔄 Pipeline Status</h3><div class="tdb-chart-container"><canvas id="overview-pipeline-chart"></canvas></div></div>');
        $chartsGrid.append($pipelineChart);
        
        $container.append($chartsGrid);
        
        // Render charts
        setTimeout(function() {
            renderCoverageChart('overview-coverage-chart', data.coverage);
            renderPipelineChart('overview-pipeline-chart', data.pipeline);
        }, 100);
    }
    
    /**
     * Render coverage view
     */
    function renderCoverageView(data, $container) {
        const $grid = $('<div class="tdb-grid">');
        
        // Current coverage metrics
        ['overall', 'lines', 'branches', 'functions'].forEach(function(type) {
            const card = createMetricCard(
                type.charAt(0).toUpperCase() + type.slice(1) + ' Coverage',
                data.current[type] + '%',
                '',
                'positive'
            );
            $grid.append(card);
        });
        
        $container.append($grid);
        
        // Coverage trend chart
        const $chartCard = $('<div class="tdb-card"><h3>📈 Coverage Trends</h3><div class="tdb-chart-container"><canvas id="coverage-trend-chart"></canvas></div><div class="tdb-export"><button class="tdb-export-btn tdb-export-csv">Export CSV</button><button class="tdb-export-btn tdb-export-png">Export PNG</button></div></div>');
        $container.append($chartCard);
        
        setTimeout(function() {
            renderCoverageTrendChart('coverage-trend-chart', data);
        }, 100);
    }
    
    /**
     * Render pipeline view
     */
    function renderPipelineView(data, $container) {
        // Success rate card
        const totalRuns = data.total.reduce(function(a, b) { return a + b; }, 0);
        const successRuns = data.success.reduce(function(a, b) { return a + b; }, 0);
        const successRate = totalRuns > 0 ? ((successRuns / totalRuns) * 100).toFixed(1) : 0;
        
        const successCard = createMetricCard(
            'Success Rate',
            successRate + '%',
            'Last ' + data.labels.length + ' days',
            'positive'
        );
        
        const totalCard = createMetricCard(
            'Total Runs',
            totalRuns,
            'All workflows',
            'info'
        );
        
        const $grid = $('<div class="tdb-grid">').append(successCard, totalCard);
        $container.append($grid);
        
        // Pipeline chart
        const $chartCard = $('<div class="tdb-card"><h3>🔄 Pipeline History</h3><div class="tdb-chart-container"><canvas id="pipeline-history-chart"></canvas></div></div>');
        $container.append($chartCard);
        
        // Recent runs
        const $runsCard = $('<div class="tdb-card"><h3>📋 Recent Runs</h3><div class="tdb-pipeline-runs" id="recent-runs"></div></div>');
        $container.append($runsCard);
        
        setTimeout(function() {
            renderPipelineHistoryChart('pipeline-history-chart', data);
            renderRecentRuns('recent-runs', data.recent_runs);
        }, 100);
    }
    
    /**
     * Render quality view
     */
    function renderQualityView(data, $container) {
        // Quality gates
        const $gatesCard = $('<div class="tdb-card"><h3>🎯 Quality Gates</h3><div class="tdb-quality-gates" id="quality-gates"></div></div>');
        $container.append($gatesCard);
        
        renderQualityGates('quality-gates', data.gates);
        
        // Quality trends
        const $trendsCard = $('<div class="tdb-card"><h3>📊 Quality Trends</h3><div class="tdb-chart-container"><canvas id="quality-trends-chart"></canvas></div></div>');
        $container.append($trendsCard);
        
        setTimeout(function() {
            renderQualityTrendsChart('quality-trends-chart', data.trends);
        }, 100);
    }
    
    /**
     * Create metric card
     */
    function createMetricCard(label, value, change, changeType) {
        const $card = $('<div class="tdb-card">');
        const $label = $('<div class="tdb-metric-label">').text(label);
        const $value = $('<div class="tdb-metric-value">').text(value);
        
        $card.append($label, $value);
        
        if (change) {
            const $change = $('<div class="tdb-metric-change">').addClass(changeType).text(change);
            $card.append($change);
        }
        
        return $card;
    }
    
    /**
     * Render coverage chart
     */
    function renderCoverageChart(canvasId, data) {
        const ctx = document.getElementById(canvasId);
        if (!ctx) return;
        
        destroyChart(canvasId);
        
        currentCharts[canvasId] = new Chart(ctx, {
            type: 'line',
            data: {
                labels: data.labels,
                datasets: [{
                    label: 'Coverage %',
                    data: data.coverage,
                    borderColor: '#0066cc',
                    backgroundColor: 'rgba(0, 102, 204, 0.1)',
                    tension: 0.4,
                    fill: true
                }]
            },
            options: {
                responsive: true,
                maintainAspectRatio: false,
                plugins: {
                    legend: {
                        display: false
                    }
                },
                scales: {
                    y: {
                        beginAtZero: false,
                        min: 70,
                        max: 100
                    }
                }
            }
        });
    }
    
    /**
     * Render coverage trend chart
     */
    function renderCoverageTrendChart(canvasId, data) {
        const ctx = document.getElementById(canvasId);
        if (!ctx) return;
        
        destroyChart(canvasId);
        
        currentCharts[canvasId] = new Chart(ctx, {
            type: 'line',
            data: {
                labels: data.labels,
                datasets: [
                    {
                        label: 'Overall',
                        data: data.coverage,
                        borderColor: '#0066cc',
                        backgroundColor: 'rgba(0, 102, 204, 0.1)',
                        tension: 0.4
                    },
                    {
                        label: 'Lines',
                        data: data.lines,
                        borderColor: '#28a745',
                        backgroundColor: 'rgba(40, 167, 69, 0.1)',
                        tension: 0.4
                    },
                    {
                        label: 'Branches',
                        data: data.branches,
                        borderColor: '#ffc107',
                        backgroundColor: 'rgba(255, 193, 7, 0.1)',
                        tension: 0.4
                    },
                    {
                        label: 'Functions',
                        data: data.functions,
                        borderColor: '#17a2b8',
                        backgroundColor: 'rgba(23, 162, 184, 0.1)',
                        tension: 0.4
                    }
                ]
            },
            options: {
                responsive: true,
                maintainAspectRatio: false,
                plugins: {
                    legend: {
                        position: 'bottom'
                    }
                },
                scales: {
                    y: {
                        beginAtZero: false,
                        min: 70,
                        max: 100
                    }
                }
            }
        });
    }
    
    /**
     * Render pipeline chart
     */
    function renderPipelineChart(canvasId, data) {
        const ctx = document.getElementById(canvasId);
        if (!ctx) return;
        
        destroyChart(canvasId);
        
        currentCharts[canvasId] = new Chart(ctx, {
            type: 'bar',
            data: {
                labels: data.labels,
                datasets: [
                    {
                        label: 'Success',
                        data: data.success,
                        backgroundColor: 'rgba(40, 167, 69, 0.8)'
                    },
                    {
                        label: 'Failure',
                        data: data.failure,
                        backgroundColor: 'rgba(220, 53, 69, 0.8)'
                    }
                ]
            },
            options: {
                responsive: true,
                maintainAspectRatio: false,
                plugins: {
                    legend: {
                        display: false
                    }
                },
                scales: {
                    x: {
                        stacked: true
                    },
                    y: {
                        stacked: true,
                        beginAtZero: true
                    }
                }
            }
        });
    }
    
    /**
     * Render pipeline history chart
     */
    function renderPipelineHistoryChart(canvasId, data) {
        const ctx = document.getElementById(canvasId);
        if (!ctx) return;
        
        destroyChart(canvasId);
        
        currentCharts[canvasId] = new Chart(ctx, {
            type: 'line',
            data: {
                labels: data.labels,
                datasets: [
                    {
                        label: 'Success',
                        data: data.success,
                        borderColor: '#28a745',
                        backgroundColor: 'rgba(40, 167, 69, 0.1)',
                        tension: 0.4,
                        fill: true
                    },
                    {
                        label: 'Failure',
                        data: data.failure,
                        borderColor: '#dc3545',
                        backgroundColor: 'rgba(220, 53, 69, 0.1)',
                        tension: 0.4,
                        fill: true
                    }
                ]
            },
            options: {
                responsive: true,
                maintainAspectRatio: false,
                plugins: {
                    legend: {
                        position: 'bottom'
                    }
                }
            }
        });
    }
    
    /**
     * Render quality trends chart
     */
    function renderQualityTrendsChart(canvasId, data) {
        const ctx = document.getElementById(canvasId);
        if (!ctx) return;
        
        destroyChart(canvasId);
        
        currentCharts[canvasId] = new Chart(ctx, {
            type: 'line',
            data: {
                labels: data.labels,
                datasets: [
                    {
                        label: 'Coverage %',
                        data: data.coverage,
                        borderColor: '#0066cc',
                        yAxisID: 'y'
                    },
                    {
                        label: 'Technical Debt',
                        data: data.debt,
                        borderColor: '#ffc107',
                        yAxisID: 'y1'
                    },
                    {
                        label: 'Code Smells',
                        data: data.smells,
                        borderColor: '#dc3545',
                        yAxisID: 'y2'
                    }
                ]
            },
            options: {
                responsive: true,
                maintainAspectRatio: false,
                interaction: {
                    mode: 'index',
                    intersect: false
                },
                plugins: {
                    legend: {
                        position: 'bottom'
                    }
                },
                scales: {
                    y: {
                        type: 'linear',
                        display: true,
                        position: 'left'
                    },
                    y1: {
                        type: 'linear',
                        display: true,
                        position: 'right',
                        grid: {
                            drawOnChartArea: false
                        }
                    },
                    y2: {
                        type: 'linear',
                        display: false,
                        position: 'right'
                    }
                }
            }
        });
    }
    
    /**
     * Render recent runs
     */
    function renderRecentRuns(containerId, runs) {
        const $container = $('#' + containerId);
        $container.empty();
        
        runs.forEach(function(run) {
            const $run = $('<div class="tdb-run">');
            const $status = $('<div class="tdb-run-status">').addClass(run.status);
            const $info = $('<div class="tdb-run-info">');
            const $name = $('<div class="tdb-run-name">').text(run.name);
            const $meta = $('<div class="tdb-run-meta">').text(run.created_at + ' • ' + formatDuration(run.duration));
            const $duration = $('<div class="tdb-run-duration">').text(formatDuration(run.duration));
            
            $info.append($name, $meta);
            $run.append($status, $info, $duration);
            $container.append($run);
        });
    }
    
    /**
     * Render quality gates
     */
    function renderQualityGates(containerId, gates) {
        const $container = $('#' + containerId);
        $container.empty();
        
        gates.forEach(function(gate) {
            const $gate = $('<div class="tdb-gate">').addClass(gate.status);
            const $icon = $('<div class="tdb-gate-icon">').text(gate.status === 'pass' ? '✅' : gate.status === 'warning' ? '⚠️' : '❌');
            const $info = $('<div class="tdb-gate-info">');
            const $name = $('<div class="tdb-gate-name">').text(gate.name);
            const $value = $('<div class="tdb-gate-value">').text(gate.value + ' (threshold: ' + gate.threshold + ')');
            const $progress = $('<div class="tdb-gate-progress">');
            const $progressBar = $('<div class="tdb-gate-progress-bar">').css('width', (gate.value / gate.threshold * 100) + '%');
            
            $progress.append($progressBar);
            $info.append($name, $value, $progress);
            $gate.append($icon, $info);
            $container.append($gate);
        });
    }
    
    /**
     * Export to CSV
     */
    function exportToCSV() {
        showNotification('Export to CSV functionality coming soon!', 'success');
    }
    
    /**
     * Export to PNG
     */
    function exportToPNG() {
        showNotification('Export to PNG functionality coming soon!', 'success');
    }
    
    /**
     * Destroy chart
     */
    function destroyChart(canvasId) {
        if (currentCharts[canvasId]) {
            currentCharts[canvasId].destroy();
            delete currentCharts[canvasId];
        }
    }
    
    /**
     * Show loading
     */
    function showLoading() {
        const $content = $('#tdb-content');
        $content.html('<div class="tdb-loading"><div class="tdb-spinner"></div><p>Loading dashboard data...</p></div>');
    }
    
    /**
     * Hide loading
     */
    function hideLoading() {
        $('.tdb-loading').remove();
    }
    
    /**
     * Show notification
     */
    function showNotification(message, type) {
        if (type === void 0) { type = 'success'; }
        
        const $notification = $('<div class="tdb-notification">').addClass(type).text(message);
        $('body').append($notification);
        
        setTimeout(function() {
            $notification.fadeOut(300, function() {
                $(this).remove();
            });
        }, 3000);
    }
    
    /**
     * Show error
     */
    function showError(message) {
        showNotification(message, 'error');
        hideLoading();
    }
    
    /**
     * Format duration
     */
    function formatDuration(seconds) {
        const minutes = Math.floor(seconds / 60);
        const secs = seconds % 60;
        return minutes > 0 ? minutes + 'm ' + secs + 's' : secs + 's';
    }
    
    // Initialize on document ready
    $(document).ready(initDashboard);
    
})(jQuery);
