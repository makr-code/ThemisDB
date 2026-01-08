/**
 * ThemisDB TCO Calculator - WordPress Version
 * Calculates and compares Total Cost of Ownership for database solutions
 * Uses ES6+ features and best practices
 */

// Configuration Constants
const CONFIG = {
    YEARS: 3,
    DATA_GROWTH_RATE: 0.20, // 20% per year
    PERSONNEL_OVERHEAD: 1.30, // 30% overhead for benefits, infrastructure
    THEMISDB_ENTERPRISE_LICENSE: 50000, // €/year estimated
    THEMISDB_MINIMAL_MAX_REQUESTS: 100000, // requests/day
    THEMISDB_COMMUNITY_MAX_REQUESTS: 1000000, // requests/day
    HYPERSCALER_REQUEST_COST: 0.00025, // €/request (DynamoDB-like pricing)
    HYPERSCALER_STORAGE_MULTIPLIER: 1.5, // Hyperscalers need more storage (replication)
    GPU_COST_MONTHLY: 2000, // Additional cost for GPU server (A100/H100)
    MIN_SERVERS_HA: 2, // Minimum servers for high availability
    BACKUP_REDUNDANCY_MULTIPLIER: 2, // Backup storage redundancy factor
    GB_PER_SERVER: 1000, // Storage capacity per server in GB
    REQUESTS_PER_SERVER_DAY: 10000000, // Max requests per server per day (10M)
    KB_PER_REQUEST: 10, // Average kilobytes per request for network estimation
    KB_TO_GB: 1024 * 1024, // Conversion factor from KB to GB
    HA_ADDITIONAL_SERVERS: 2, // Additional servers for high availability
    HA_REDUNDANCY_MULTIPLIER: 1.5, // Redundancy multiplier for HA
};

// State Management
class TCOCalculator {
    constructor() {
        this.inputs = {};
        this.results = {
            themisdb: {},
            hyperscaler: {},
        };
        this.chart = null;
        this.initializeEventListeners();
        this.loadWordPressSettings();
    }

    /**
     * Load settings from WordPress
     */
    loadWordPressSettings() {
        if (typeof themisdbTCO !== 'undefined' && themisdbTCO.settings) {
            const settings = themisdbTCO.settings;
            
            // Apply default values if elements exist
            const requestsInput = document.getElementById('requestsPerDay');
            if (requestsInput && settings.defaultRequestsPerDay) {
                requestsInput.value = settings.defaultRequestsPerDay;
            }
            
            const dataSizeInput = document.getElementById('dataSize');
            if (dataSizeInput && settings.defaultDataSize) {
                dataSizeInput.value = settings.defaultDataSize;
            }
            
            const peakLoadInput = document.getElementById('peakLoad');
            if (peakLoadInput && settings.defaultPeakLoad) {
                peakLoadInput.value = settings.defaultPeakLoad;
            }
            
            const availabilityInput = document.getElementById('availability');
            if (availabilityInput && settings.defaultAvailability) {
                availabilityInput.value = settings.defaultAvailability;
            }
        }
    }

    /**
     * Initialize all event listeners
     */
    initializeEventListeners() {
        const calculateBtn = document.getElementById('calculateBtn');
        if (calculateBtn) {
            calculateBtn.addEventListener('click', () => this.calculate());
        }
        
        const resetBtn = document.getElementById('resetBtn');
        if (resetBtn) {
            resetBtn.addEventListener('click', () => this.reset());
        }
        
        const exportPDFBtn = document.getElementById('exportPDF');
        if (exportPDFBtn) {
            exportPDFBtn.addEventListener('click', () => this.exportPDF());
        }
        
        const exportCSVBtn = document.getElementById('exportCSV');
        if (exportCSVBtn) {
            exportCSVBtn.addEventListener('click', () => this.exportCSV());
        }
        
        const printBtn = document.getElementById('printBtn');
        if (printBtn) {
            printBtn.addEventListener('click', () => window.print());
        }

        // Tab switching
        document.querySelectorAll('.tab-btn').forEach(btn => {
            btn.addEventListener('click', (e) => this.switchTab(e.target.dataset.tab));
        });

        // Collapsible cost details - using event delegation
        document.addEventListener('click', (e) => {
            const header = e.target.closest('.cost-item-header[data-toggle-details]');
            if (header) {
                const detailsId = header.getAttribute('data-toggle-details');
                if (detailsId) {
                    toggleCostDetails(detailsId);
                }
            }
        });

        // Initialize sliders
        this.initializeSliders();
    }

    /**
     * Initialize slider event listeners and value displays
     */
    initializeSliders() {
        const sliders = document.querySelectorAll('.slider');
        sliders.forEach(slider => {
            const outputId = slider.id + '-value';
            const output = document.getElementById(outputId);
            
            if (output) {
                // Set initial value
                this.updateSliderValue(slider, output);
                this.updateSliderBackground(slider);
                
                // Update on input
                slider.addEventListener('input', () => {
                    this.updateSliderValue(slider, output);
                    this.updateSliderBackground(slider);
                });
            }
        });
    }

    /**
     * Update slider value display
     */
    updateSliderValue(slider, output) {
        const value = parseFloat(slider.value);
        const id = slider.id;
        
        // Format value based on slider type
        let formattedValue;
        switch (id) {
            case 'requestsPerDay':
                formattedValue = this.formatNumber(value);
                break;
            case 'dataSize':
                formattedValue = `${this.formatNumber(value)} GB`;
                break;
            case 'peakLoad':
                formattedValue = `${value}x`;
                break;
            case 'serverCost':
            case 'networkCost':
            case 'trainingCost':
            case 'supportCost':
            case 'aiApiCost':
                formattedValue = `€${this.formatNumber(value)}`;
                break;
            case 'storageCostPerGB':
            case 'backupCost':
                formattedValue = `€${value.toFixed(2)}`;
                break;
            case 'dbaCount':
            case 'devCount':
                formattedValue = `${value} FTE`;
                break;
            case 'dbaSalary':
            case 'devSalary':
                formattedValue = `€${this.formatNumber(value)}`;
                break;
            default:
                formattedValue = value;
        }
        
        output.textContent = formattedValue;
    }

    /**
     * Update slider background to show progress
     */
    updateSliderBackground(slider) {
        const value = slider.value;
        const min = slider.min || 0;
        const max = slider.max || 100;
        const percentage = ((value - min) / (max - min)) * 100;
        
        slider.style.background = `linear-gradient(to right, var(--secondary-color) 0%, var(--secondary-color) ${percentage}%, var(--light-bg) ${percentage}%, var(--light-bg) 100%)`;
    }

    /**
     * Collect all input values from the form
     */
    collectInputs() {
        this.inputs = {
            requestsPerDay: parseFloat(document.getElementById('requestsPerDay').value),
            dataSize: parseFloat(document.getElementById('dataSize').value),
            peakLoad: parseFloat(document.getElementById('peakLoad').value),
            availability: parseFloat(document.getElementById('availability').value),
            serverCost: parseFloat(document.getElementById('serverCost').value),
            storageCostPerGB: parseFloat(document.getElementById('storageCostPerGB').value),
            networkCost: parseFloat(document.getElementById('networkCost').value),
            backupCost: parseFloat(document.getElementById('backupCost').value),
            dbaCount: parseFloat(document.getElementById('dbaCount').value),
            dbaSalary: parseFloat(document.getElementById('dbaSalary').value),
            devCount: parseFloat(document.getElementById('devCount').value),
            devSalary: parseFloat(document.getElementById('devSalary').value),
            trainingCost: parseFloat(document.getElementById('trainingCost').value),
            supportCost: parseFloat(document.getElementById('supportCost').value),
            useAI: document.getElementById('useAI').value === 'true',
            aiApiCost: parseFloat(document.getElementById('aiApiCost').value),
        };
    }

    /**
     * Main calculation method
     */
    calculate() {
        this.collectInputs();
        
        // Calculate ThemisDB TCO
        this.results.themisdb = this.calculateThemisDB();
        
        // Calculate Hyperscaler TCO
        this.results.hyperscaler = this.calculateHyperscaler();
        
        // Display results
        this.displayResults();
        
        // Show results section with smooth scroll
        const resultsSection = document.getElementById('resultsSection');
        resultsSection.style.display = 'block';
        resultsSection.scrollIntoView({ behavior: 'smooth', block: 'start' });
    }

    /**
     * Calculate ThemisDB TCO
     */
    calculateThemisDB() {
        const costs = {
            infrastructure: [],
            personnel: [],
            licenses: [],
            operations: [],
            total: [],
        };

        // Determine edition based on requirements
        let edition = 'Community';
        let licenseCost = 0;

        if (this.inputs.requestsPerDay > CONFIG.THEMISDB_COMMUNITY_MAX_REQUESTS || 
            this.inputs.availability >= 99.99) {
            edition = 'Enterprise';
            licenseCost = CONFIG.THEMISDB_ENTERPRISE_LICENSE;
        } else if (this.inputs.requestsPerDay <= CONFIG.THEMISDB_MINIMAL_MAX_REQUESTS && 
                   this.inputs.dataSize <= 50) {
            edition = 'Minimal';
        }

        // Calculate for each year
        for (let year = 1; year <= CONFIG.YEARS; year++) {
            const dataGrowth = Math.pow(1 + CONFIG.DATA_GROWTH_RATE, year - 1);
            const currentDataSize = this.inputs.dataSize * dataGrowth;
            
            // Infrastructure costs
            let serverCount = this.calculateThemisDBServers(edition);
            let monthlyServerCost = serverCount * this.inputs.serverCost;
            
            // Add GPU costs if AI is enabled
            if (this.inputs.useAI) {
                monthlyServerCost += CONFIG.GPU_COST_MONTHLY;
            }
            
            const storageCost = currentDataSize * this.inputs.storageCostPerGB;
            const backupCost = currentDataSize * CONFIG.BACKUP_REDUNDANCY_MULTIPLIER * this.inputs.backupCost;
            const networkCost = this.estimateNetworkUsage() * this.inputs.networkCost / 1000;
            
            const yearlyInfra = (monthlyServerCost + storageCost + backupCost + networkCost) * 12;
            costs.infrastructure.push(yearlyInfra);
            
            // Personnel costs (with overhead)
            const dbaCost = this.inputs.dbaCount * this.inputs.dbaSalary * CONFIG.PERSONNEL_OVERHEAD;
            const devCost = this.inputs.devCount * this.inputs.devSalary * CONFIG.PERSONNEL_OVERHEAD;
            const yearlyPersonnel = dbaCost + devCost;
            costs.personnel.push(yearlyPersonnel);
            
            // License costs
            costs.licenses.push(licenseCost);
            
            // Operations costs
            const yearlyOps = this.inputs.trainingCost + this.inputs.supportCost;
            costs.operations.push(yearlyOps);
            
            // Total
            costs.total.push(yearlyInfra + yearlyPersonnel + licenseCost + yearlyOps);
        }

        return {
            edition,
            costs,
            totalCost: costs.total.reduce((a, b) => a + b, 0),
        };
    }

    /**
     * Calculate number of servers needed for ThemisDB
     */
    calculateThemisDBServers(edition) {
        const baseServers = Math.ceil(this.inputs.dataSize / CONFIG.GB_PER_SERVER);
        const loadServers = Math.ceil(this.inputs.requestsPerDay / CONFIG.REQUESTS_PER_SERVER_DAY);
        
        let servers = Math.max(CONFIG.MIN_SERVERS_HA, baseServers, loadServers);
        
        // Add servers for high availability
        if (this.inputs.availability >= 99.99) {
            servers = Math.max(
                servers + CONFIG.HA_ADDITIONAL_SERVERS, 
                servers * CONFIG.HA_REDUNDANCY_MULTIPLIER
            );
        }
        
        // Peak load consideration
        servers = Math.ceil(servers * (this.inputs.peakLoad / 2));
        
        return Math.ceil(servers);
    }

    /**
     * Calculate Hyperscaler TCO
     */
    calculateHyperscaler() {
        const costs = {
            compute: [],
            storage: [],
            network: [],
            ai: [],
            total: [],
        };

        // Calculate for each year
        for (let year = 1; year <= CONFIG.YEARS; year++) {
            const dataGrowth = Math.pow(1 + CONFIG.DATA_GROWTH_RATE, year - 1);
            const currentDataSize = this.inputs.dataSize * dataGrowth;
            
            // Compute costs (pay-per-request model)
            const yearlyRequests = this.inputs.requestsPerDay * 365;
            const computeCost = yearlyRequests * CONFIG.HYPERSCALER_REQUEST_COST;
            costs.compute.push(computeCost);
            
            // Storage costs (higher due to replication)
            const storageCost = currentDataSize * CONFIG.HYPERSCALER_STORAGE_MULTIPLIER * 
                               this.inputs.storageCostPerGB * 12;
            costs.storage.push(storageCost);
            
            // Network costs (egress charges)
            const networkUsage = this.estimateNetworkUsage();
            const networkCost = networkUsage * this.inputs.networkCost / 1000 * 12;
            costs.network.push(networkCost);
            
            // AI API costs (if enabled)
            let aiCost = 0;
            if (this.inputs.useAI) {
                aiCost = this.inputs.aiApiCost * 12;
            }
            costs.ai.push(aiCost);
            
            // Total
            costs.total.push(computeCost + storageCost + networkCost + aiCost);
        }

        return {
            costs,
            totalCost: costs.total.reduce((a, b) => a + b, 0),
        };
    }

    /**
     * Estimate monthly network usage in GB
     */
    estimateNetworkUsage() {
        const dailyGB = (this.inputs.requestsPerDay * CONFIG.KB_PER_REQUEST) / CONFIG.KB_TO_GB;
        return dailyGB * 30;
    }

    /**
     * Display calculation results
     */
    displayResults() {
        this.updateSummaryCards();
        this.createChart();
        this.createMermaidDiagram();
        this.updateBreakdownTables();
        this.generateInsights();
    }

    /**
     * Update summary cards with calculated values
     */
    updateSummaryCards() {
        const themisdb = this.results.themisdb;
        const hyperscaler = this.results.hyperscaler;
        
        // ThemisDB card
        document.getElementById('themisdbEdition').textContent = themisdb.edition;
        document.getElementById('themisdbTotal').textContent = this.formatCurrency(themisdb.totalCost);
        document.getElementById('themisdbInfra').textContent = this.formatCurrency(
            themisdb.costs.infrastructure.reduce((a, b) => a + b, 0)
        );
        document.getElementById('themisdbPersonnel').textContent = this.formatCurrency(
            themisdb.costs.personnel.reduce((a, b) => a + b, 0)
        );
        document.getElementById('themisdbLicense').textContent = this.formatCurrency(
            themisdb.costs.licenses.reduce((a, b) => a + b, 0)
        );
        document.getElementById('themisdbOps').textContent = this.formatCurrency(
            themisdb.costs.operations.reduce((a, b) => a + b, 0)
        );
        
        // Hyperscaler card
        document.getElementById('hyperscalerTotal').textContent = this.formatCurrency(hyperscaler.totalCost);
        document.getElementById('hyperscalerCompute').textContent = this.formatCurrency(
            hyperscaler.costs.compute.reduce((a, b) => a + b, 0)
        );
        document.getElementById('hyperscalerStorage').textContent = this.formatCurrency(
            hyperscaler.costs.storage.reduce((a, b) => a + b, 0)
        );
        document.getElementById('hyperscalerNetwork').textContent = this.formatCurrency(
            hyperscaler.costs.network.reduce((a, b) => a + b, 0)
        );
        document.getElementById('hyperscalerAI').textContent = this.formatCurrency(
            hyperscaler.costs.ai.reduce((a, b) => a + b, 0)
        );
        
        // Savings card
        const savings = hyperscaler.totalCost - themisdb.totalCost;
        const savingsPercent = (savings / hyperscaler.totalCost) * 100;
        
        document.getElementById('savingsAmount').textContent = this.formatCurrency(Math.abs(savings));
        
        if (savings > 0) {
            document.getElementById('savingsPercentage').textContent = 
                `${savingsPercent.toFixed(1)}% günstiger`;
            document.getElementById('savingsPercentage').style.color = 'var(--success-color)';
        } else {
            document.getElementById('savingsPercentage').textContent = 
                `${Math.abs(savingsPercent).toFixed(1)}% teurer`;
            document.getElementById('savingsPercentage').style.color = 'var(--danger-color)';
        }
        
        // Calculate ROI time
        const roiMonths = this.calculateROI();
        document.getElementById('roiTime').textContent = 
            roiMonths > 0 ? `Nach ${roiMonths} Monaten` : 'Sofort';
    }

    /**
     * Calculate ROI time in months
     */
    calculateROI() {
        const themisdb = this.results.themisdb;
        const hyperscaler = this.results.hyperscaler;
        
        const themisdbMonthly = themisdb.totalCost / (CONFIG.YEARS * 12);
        const hyperscalerMonthly = hyperscaler.totalCost / (CONFIG.YEARS * 12);
        
        if (themisdbMonthly >= hyperscalerMonthly) {
            return 0;
        }
        
        const initialInvestment = themisdb.costs.infrastructure[0] / 12 + 
                                 themisdb.costs.licenses[0] / 12;
        const monthlySavings = hyperscalerMonthly - themisdbMonthly;
        
        return Math.ceil(initialInvestment / monthlySavings);
    }

    /**
     * Create cost comparison chart
     */
    createChart() {
        if (typeof Chart === 'undefined') {
            console.warn('Chart.js not available, skipping chart rendering');
            const chartContainer = document.querySelector('.chart-container');
            if (chartContainer) {
                chartContainer.style.display = 'none';
            }
            return;
        }
        
        const canvas = document.getElementById('costChart');
        if (!canvas) {
            return;
        }
        
        const ctx = canvas.getContext('2d');
        
        if (this.chart) {
            this.chart.destroy();
        }
        
        const themisdb = this.results.themisdb;
        const hyperscaler = this.results.hyperscaler;
        
        this.chart = new Chart(ctx, {
            type: 'bar',
            data: {
                labels: ['Jahr 1', 'Jahr 2', 'Jahr 3'],
                datasets: [
                    {
                        label: 'ThemisDB',
                        data: themisdb.costs.total,
                        backgroundColor: 'rgba(39, 174, 96, 0.8)',
                        borderColor: 'rgba(39, 174, 96, 1)',
                        borderWidth: 2,
                    },
                    {
                        label: 'Hyperscaler',
                        data: hyperscaler.costs.total,
                        backgroundColor: 'rgba(243, 156, 18, 0.8)',
                        borderColor: 'rgba(243, 156, 18, 1)',
                        borderWidth: 2,
                    },
                ],
            },
            options: {
                responsive: true,
                maintainAspectRatio: true,
                plugins: {
                    legend: {
                        position: 'top',
                        labels: {
                            font: {
                                size: 14,
                                weight: 'bold',
                            },
                        },
                    },
                    tooltip: {
                        callbacks: {
                            label: (context) => {
                                return `${context.dataset.label}: ${this.formatCurrency(context.parsed.y)}`;
                            },
                        },
                    },
                },
                scales: {
                    y: {
                        beginAtZero: true,
                        ticks: {
                            callback: (value) => this.formatCurrency(value),
                        },
                    },
                },
            },
        });
    }

    /**
     * Create Mermaid diagram for cost comparison
     */
    createMermaidDiagram() {
        const mermaidContainer = document.getElementById('mermaidDiagram');
        if (!mermaidContainer) {
            return;
        }

        const themisdb = this.results.themisdb;
        const hyperscaler = this.results.hyperscaler;
        
        // Calculate percentages for better visualization
        const themisdbInfra = themisdb.costs.infrastructure.reduce((a, b) => a + b, 0);
        const themisdbPersonnel = themisdb.costs.personnel.reduce((a, b) => a + b, 0);
        const themisdbLicense = themisdb.costs.licenses.reduce((a, b) => a + b, 0);
        const themisdbOps = themisdb.costs.operations.reduce((a, b) => a + b, 0);
        
        const themisdbInfraPercent = ((themisdbInfra / themisdb.totalCost) * 100).toFixed(1);
        const themisdbPersonnelPercent = ((themisdbPersonnel / themisdb.totalCost) * 100).toFixed(1);
        const themisdbLicensePercent = ((themisdbLicense / themisdb.totalCost) * 100).toFixed(1);
        const themisdbOpsPercent = ((themisdbOps / themisdb.totalCost) * 100).toFixed(1);
        
        const hyperscalerCompute = hyperscaler.costs.compute.reduce((a, b) => a + b, 0);
        const hyperscalerStorage = hyperscaler.costs.storage.reduce((a, b) => a + b, 0);
        const hyperscalerNetwork = hyperscaler.costs.network.reduce((a, b) => a + b, 0);
        const hyperscalerAI = hyperscaler.costs.ai.reduce((a, b) => a + b, 0);
        
        const hyperscalerComputePercent = ((hyperscalerCompute / hyperscaler.totalCost) * 100).toFixed(1);
        const hyperscalerStoragePercent = ((hyperscalerStorage / hyperscaler.totalCost) * 100).toFixed(1);
        const hyperscalerNetworkPercent = ((hyperscalerNetwork / hyperscaler.totalCost) * 100).toFixed(1);
        const hyperscalerAIPercent = ((hyperscalerAI / hyperscaler.totalCost) * 100).toFixed(1);
        
        // Escape function for safe text content
        const escapeText = (text) => {
            return String(text).replace(/[<>&"']/g, (match) => {
                const escapeMap = {
                    '<': '&lt;',
                    '>': '&gt;',
                    '&': '&amp;',
                    '"': '&quot;',
                    "'": '&#x27;'
                };
                return escapeMap[match];
            });
        };
        
        // Create Mermaid diagram with escaped values
        const mermaidCode = `
graph TB
    subgraph ThemisDB["ThemisDB TCO: ${escapeText(this.formatCurrency(themisdb.totalCost))}"]
        T1["Material & Infrastruktur<br/>${escapeText(this.formatCurrency(themisdbInfra))}<br/>(${escapeText(themisdbInfraPercent)}%)"]
        T2["Personal<br/>${escapeText(this.formatCurrency(themisdbPersonnel))}<br/>(${escapeText(themisdbPersonnelPercent)}%)"]
        T3["Software & Lizenzen<br/>${escapeText(this.formatCurrency(themisdbLicense))}<br/>(${escapeText(themisdbLicensePercent)}%)"]
        T4["Betrieb & Schulung<br/>${escapeText(this.formatCurrency(themisdbOps))}<br/>(${escapeText(themisdbOpsPercent)}%)"]
    end
    
    subgraph Hyperscaler["Hyperscaler TCO: ${escapeText(this.formatCurrency(hyperscaler.totalCost))}"]
        H1["Compute Pay-per-Request<br/>${escapeText(this.formatCurrency(hyperscalerCompute))}<br/>(${escapeText(hyperscalerComputePercent)}%)"]
        H2["Storage<br/>${escapeText(this.formatCurrency(hyperscalerStorage))}<br/>(${escapeText(hyperscalerStoragePercent)}%)"]
        H3["Network Egress<br/>${escapeText(this.formatCurrency(hyperscalerNetwork))}<br/>(${escapeText(hyperscalerNetworkPercent)}%)"]
        H4["AI APIs<br/>${escapeText(this.formatCurrency(hyperscalerAI))}<br/>(${escapeText(hyperscalerAIPercent)}%)"]
    end
    
    style T1 fill:#d5f4e6,stroke:#27ae60,stroke-width:2px
    style T2 fill:#ffeaa7,stroke:#fdcb6e,stroke-width:2px
    style T3 fill:#dfe6e9,stroke:#636e72,stroke-width:2px
    style T4 fill:#74b9ff,stroke:#0984e3,stroke-width:2px
    
    style H1 fill:#fab1a0,stroke:#e17055,stroke-width:2px
    style H2 fill:#fdcb6e,stroke:#f39c12,stroke-width:2px
    style H3 fill:#a29bfe,stroke:#6c5ce7,stroke-width:2px
    style H4 fill:#fd79a8,stroke:#e84393,stroke-width:2px
    
    style ThemisDB fill:#e8f8f5,stroke:#27ae60,stroke-width:3px
    style Hyperscaler fill:#fef5e7,stroke:#f39c12,stroke-width:3px
        `;
        
        // Check if Mermaid is available
        if (typeof mermaid !== 'undefined') {
            // Clear previous content
            mermaidContainer.textContent = '';
            
            // Create div for mermaid content
            const mermaidDiv = document.createElement('div');
            mermaidDiv.className = 'mermaid';
            mermaidDiv.textContent = mermaidCode;
            mermaidContainer.appendChild(mermaidDiv);
            
            // Render with modern API
            try {
                mermaid.init(undefined, mermaidDiv);
            } catch (error) {
                console.error('Mermaid rendering error:', error);
                mermaidContainer.textContent = '';
                const errorDiv = document.createElement('div');
                errorDiv.style.cssText = 'padding: 20px; text-align: center; color: var(--text-secondary);';
                const errorP = document.createElement('p');
                errorP.textContent = '⚠️ Diagramm konnte nicht gerendert werden.';
                errorDiv.appendChild(errorP);
                mermaidContainer.appendChild(errorDiv);
            }
        } else {
            mermaidContainer.textContent = '';
            const fallbackDiv = document.createElement('div');
            fallbackDiv.style.cssText = 'padding: 20px; text-align: center; color: var(--text-secondary);';
            
            const p1 = document.createElement('p');
            p1.textContent = '⚠️ Mermaid.js wird geladen...';
            fallbackDiv.appendChild(p1);
            
            const p2 = document.createElement('p');
            p2.textContent = 'Falls das Diagramm nicht erscheint, laden Sie die Seite bitte neu.';
            fallbackDiv.appendChild(p2);
            
            mermaidContainer.appendChild(fallbackDiv);
            console.warn('Mermaid.js not available for diagram rendering');
        }
    }

    /**
     * Update detailed breakdown tables
     */
    updateBreakdownTables() {
        this.updateThemisDBBreakdown();
        this.updateHyperscalerBreakdown();
    }

    /**
     * Update ThemisDB breakdown table
     */
    updateThemisDBBreakdown() {
        const tbody = document.getElementById('themisdbBreakdownTable');
        if (!tbody) return;
        
        tbody.innerHTML = '';
        
        const themisdb = this.results.themisdb;
        const categories = [
            { name: 'Infrastruktur', data: themisdb.costs.infrastructure },
            { name: 'Personal', data: themisdb.costs.personnel },
            { name: 'Lizenzen', data: themisdb.costs.licenses },
            { name: 'Betrieb', data: themisdb.costs.operations },
        ];
        
        categories.forEach(cat => {
            const row = tbody.insertRow();
            row.innerHTML = `
                <td>${cat.name}</td>
                <td>${this.formatCurrency(cat.data[0])}</td>
                <td>${this.formatCurrency(cat.data[1])}</td>
                <td>${this.formatCurrency(cat.data[2])}</td>
                <td><strong>${this.formatCurrency(cat.data.reduce((a, b) => a + b, 0))}</strong></td>
            `;
        });
        
        const totalRow = tbody.insertRow();
        totalRow.innerHTML = `
            <td><strong>Gesamt</strong></td>
            <td><strong>${this.formatCurrency(themisdb.costs.total[0])}</strong></td>
            <td><strong>${this.formatCurrency(themisdb.costs.total[1])}</strong></td>
            <td><strong>${this.formatCurrency(themisdb.costs.total[2])}</strong></td>
            <td><strong>${this.formatCurrency(themisdb.totalCost)}</strong></td>
        `;
    }

    /**
     * Update Hyperscaler breakdown table
     */
    updateHyperscalerBreakdown() {
        const tbody = document.getElementById('hyperscalerBreakdownTable');
        if (!tbody) return;
        
        tbody.innerHTML = '';
        
        const hyperscaler = this.results.hyperscaler;
        const categories = [
            { name: 'Compute (Pay-per-Request)', data: hyperscaler.costs.compute },
            { name: 'Storage', data: hyperscaler.costs.storage },
            { name: 'Network (Egress)', data: hyperscaler.costs.network },
            { name: 'AI APIs', data: hyperscaler.costs.ai },
        ];
        
        categories.forEach(cat => {
            const row = tbody.insertRow();
            row.innerHTML = `
                <td>${cat.name}</td>
                <td>${this.formatCurrency(cat.data[0])}</td>
                <td>${this.formatCurrency(cat.data[1])}</td>
                <td>${this.formatCurrency(cat.data[2])}</td>
                <td><strong>${this.formatCurrency(cat.data.reduce((a, b) => a + b, 0))}</strong></td>
            `;
        });
        
        const totalRow = tbody.insertRow();
        totalRow.innerHTML = `
            <td><strong>Gesamt</strong></td>
            <td><strong>${this.formatCurrency(hyperscaler.costs.total[0])}</strong></td>
            <td><strong>${this.formatCurrency(hyperscaler.costs.total[1])}</strong></td>
            <td><strong>${this.formatCurrency(hyperscaler.costs.total[2])}</strong></td>
            <td><strong>${this.formatCurrency(hyperscaler.totalCost)}</strong></td>
        `;
    }

    /**
     * Generate insights based on calculation
     */
    generateInsights() {
        const insights = [];
        const themisdb = this.results.themisdb;
        const hyperscaler = this.results.hyperscaler;
        const savings = hyperscaler.totalCost - themisdb.totalCost;
        
        if (savings > 0) {
            insights.push(`💰 ThemisDB spart über 3 Jahre <strong>${this.formatCurrency(savings)}</strong> 
                          (${((savings / hyperscaler.totalCost) * 100).toFixed(1)}%) im Vergleich zu Hyperscaler-Lösungen.`);
        } else {
            insights.push(`⚠️ Bei Ihren Anforderungen ist eine Hyperscaler-Lösung möglicherweise kosteneffizienter.`);
        }
        
        if (themisdb.edition === 'Community') {
            insights.push(`✅ Sie können die <strong>kostenlose Community Edition</strong> nutzen - 
                          keine Lizenzkosten!`);
        }
        
        if (this.inputs.useAI) {
            const aiSavings = hyperscaler.costs.ai.reduce((a, b) => a + b, 0);
            insights.push(`🤖 Durch native LLM-Integration sparen Sie <strong>${this.formatCurrency(aiSavings)}</strong> 
                          an externen AI API-Kosten über 3 Jahre.`);
        }
        
        if (this.inputs.requestsPerDay > 1000000) {
            insights.push(`📈 Bei hohem Durchsatz (${this.formatNumber(this.inputs.requestsPerDay)} Anfragen/Tag) 
                          sind die vorhersagbaren Kosten von ThemisDB ein großer Vorteil.`);
        }
        
        const personnelCost = themisdb.costs.personnel.reduce((a, b) => a + b, 0);
        const personnelPercent = (personnelCost / themisdb.totalCost) * 100;
        if (personnelPercent > 50) {
            insights.push(`👥 Personalkosten machen ${personnelPercent.toFixed(0)}% der Gesamtkosten aus. 
                          Durch Automatisierung und Schulungen können hier Einsparungen erzielt werden.`);
        }
        
        insights.push(`🔒 ThemisDB bietet vollständige Datensouveränität - keine Abhängigkeit von Cloud-Anbietern.`);
        insights.push(`🎯 Multi-Model-Architektur vermeidet die Notwendigkeit mehrerer spezialisierter Datenbanken.`);
        
        const list = document.getElementById('insightsList');
        if (list) {
            list.innerHTML = insights.map(insight => `<li>${insight}</li>`).join('');
        }
    }

    /**
     * Format number as currency
     */
    formatCurrency(value) {
        return new Intl.NumberFormat('de-DE', {
            style: 'currency',
            currency: 'EUR',
            minimumFractionDigits: 0,
            maximumFractionDigits: 0,
        }).format(value);
    }

    /**
     * Format number with thousand separators
     */
    formatNumber(value) {
        return new Intl.NumberFormat('de-DE').format(value);
    }

    /**
     * Switch between tabs
     */
    switchTab(tabName) {
        document.querySelectorAll('.tab-btn').forEach(btn => btn.classList.remove('active'));
        document.querySelectorAll('.tab-content').forEach(content => content.classList.remove('active'));
        
        const tabBtn = document.querySelector(`[data-tab="${tabName}"]`);
        const tabContent = document.getElementById(tabName);
        
        if (tabBtn) tabBtn.classList.add('active');
        if (tabContent) tabContent.classList.add('active');
    }

    /**
     * Reset form to default values
     */
    reset() {
        document.getElementById('requestsPerDay').value = 1000000;
        document.getElementById('dataSize').value = 500;
        document.getElementById('peakLoad').value = 3;
        document.getElementById('availability').value = 99.999;
        document.getElementById('serverCost').value = 500;
        document.getElementById('storageCostPerGB').value = 0.10;
        document.getElementById('networkCost').value = 50;
        document.getElementById('backupCost').value = 0.05;
        document.getElementById('dbaCount').value = 2;
        document.getElementById('dbaSalary').value = 85000;
        document.getElementById('devCount').value = 5;
        document.getElementById('devSalary').value = 75000;
        document.getElementById('trainingCost').value = 20000;
        document.getElementById('supportCost').value = 50000;
        document.getElementById('useAI').value = 'false';
        document.getElementById('aiApiCost').value = 5000;
        
        // Update all slider displays
        const sliders = document.querySelectorAll('.slider');
        sliders.forEach(slider => {
            const outputId = slider.id + '-value';
            const output = document.getElementById(outputId);
            if (output) {
                this.updateSliderValue(slider, output);
                this.updateSliderBackground(slider);
            }
        });
        
        const resultsSection = document.getElementById('resultsSection');
        if (resultsSection) {
            resultsSection.style.display = 'none';
        }
    }

    /**
     * Export results as PDF (using browser print)
     */
    exportPDF() {
        window.print();
    }

    /**
     * Export results as CSV
     */
    exportCSV() {
        const themisdb = this.results.themisdb;
        const hyperscaler = this.results.hyperscaler;
        
        let csv = 'ThemisDB TCO-Analyse\n\n';
        csv += 'Jahr,ThemisDB,Hyperscaler,Differenz\n';
        
        for (let i = 0; i < CONFIG.YEARS; i++) {
            const diff = hyperscaler.costs.total[i] - themisdb.costs.total[i];
            csv += `Jahr ${i + 1},${themisdb.costs.total[i].toFixed(2)},${hyperscaler.costs.total[i].toFixed(2)},${diff.toFixed(2)}\n`;
        }
        
        csv += `\nGesamt,${themisdb.totalCost.toFixed(2)},${hyperscaler.totalCost.toFixed(2)},${(hyperscaler.totalCost - themisdb.totalCost).toFixed(2)}\n`;
        
        const blob = new Blob([csv], { type: 'text/csv' });
        const url = window.URL.createObjectURL(blob);
        const a = document.createElement('a');
        a.href = url;
        a.download = 'themisdb-tco-analysis.csv';
        document.body.appendChild(a);
        a.click();
        document.body.removeChild(a);
        window.URL.revokeObjectURL(url);
    }
}

// Initialize application when DOM is loaded
if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', () => {
        const calculator = new TCOCalculator();
        window.tcoCalculator = calculator;
        console.log('ThemisDB TCO Calculator (WordPress) initialized');
    });
} else {
    const calculator = new TCOCalculator();
    window.tcoCalculator = calculator;
    console.log('ThemisDB TCO Calculator (WordPress) initialized');
}

/**
 * Toggle cost details visibility
 * @param {string} detailsId - ID prefix of the details element
 */
function toggleCostDetails(detailsId) {
    const detailsElement = document.getElementById(detailsId + '-details');
    
    if (!detailsElement) {
        console.warn(`Details element not found: ${detailsId}-details`);
        return;
    }
    
    const parentItem = detailsElement.closest('.collapsible-item');
    
    if (parentItem) {
        parentItem.classList.toggle('collapsed');
    }
}

// Make toggle function globally available
window.toggleCostDetails = toggleCostDetails;
