/**
 * Neo4j Bloom-inspired Graph Navigation Overlay for ThemisDB Theme
 * 
 * Creates an interactive force-directed graph with physics-based layout
 * Displays as a transparent overlay on the page with zoom, pan, and drag
 */
(function() {
    'use strict';

    // Configuration
    const config = {
        mermaidCDN: 'https://cdn.jsdelivr.net/npm/mermaid@10/dist/mermaid.esm.min.mjs',
        d3CDN: 'https://cdn.jsdelivr.net/npm/d3@7/dist/d3.min.js',
        fallbackEnabled: true,
        animationDuration: 300,
        physics: {
            charge: -800,
            linkDistance: 120,
            linkStrength: 1,
            collisionRadius: 60,
            centerStrength: 0.05
        }
    };

    let d3Loaded = false;
    let d3 = null;
    let simulation = null;
    let transform = null;

    /**
     * Load D3.js dynamically for force-directed graph
     */
    async function loadD3() {
        if (d3Loaded && d3) return true;

        try {
            // Create script element for D3
            const script = document.createElement('script');
            script.src = config.d3CDN;
            script.async = true;
            
            await new Promise((resolve, reject) => {
                script.onload = () => {
                    d3 = window.d3;
                    d3Loaded = true;
                    console.log('D3.js loaded successfully');
                    resolve();
                };
                script.onerror = reject;
                document.head.appendChild(script);
            });

            return true;
        } catch (error) {
            console.warn('Failed to load D3.js:', error);
            
            if (config.fallbackEnabled) {
                console.log('Using fallback navigation tree');
                return false;
            }
            return false;
        }
    }

    /**
     * Get theme color based on menu item title
     */
    function getThemeColor(title, index) {
        const lowerTitle = title.toLowerCase();
        
        // Thematic color mapping
        if (lowerTitle.includes('doc') || lowerTitle.includes('guide') || lowerTitle.includes('tutorial')) {
            return { fill: '#3498db', stroke: '#2980b9', class: 'docNode', label: '📚' };
        } else if (lowerTitle.includes('feature') || lowerTitle.includes('product')) {
            return { fill: '#27ae60', stroke: '#229954', class: 'featureNode', label: '⚡' };
        } else if (lowerTitle.includes('about') || lowerTitle.includes('contact') || lowerTitle.includes('team')) {
            return { fill: '#e74c3c', stroke: '#c0392b', class: 'aboutNode', label: '👥' };
        } else if (lowerTitle.includes('blog') || lowerTitle.includes('news') || lowerTitle.includes('article')) {
            return { fill: '#f39c12', stroke: '#e67e22', class: 'blogNode', label: '📝' };
        } else if (lowerTitle.includes('api') || lowerTitle.includes('code') || lowerTitle.includes('develop')) {
            return { fill: '#9b59b6', stroke: '#8e44ad', class: 'apiNode', label: '💻' };
        } else if (lowerTitle.includes('support') || lowerTitle.includes('help') || lowerTitle.includes('faq')) {
            return { fill: '#16a085', stroke: '#138d75', class: 'supportNode', label: '🛟' };
        } else {
            // Cycle through Themis colors for other items
            const colors = [
                { fill: '#7c4dff', stroke: '#651fff', class: 'primaryNode', label: '🔷' },
                { fill: '#3498db', stroke: '#2980b9', class: 'secondaryNode', label: '🔹' },
                { fill: '#27ae60', stroke: '#229954', class: 'successNode', label: '✅' }
            ];
            return colors[index % colors.length];
        }
    }

    /**
     * Get edge label based on relationship
     */
    function getEdgeLabel(parentTitle, childTitle, level) {
        const labels = [
            'contains', 'includes', 'leads to', 'opens', 'shows',
            'displays', 'presents', 'explores', 'details', 'explains'
        ];
        
        if (level === 0) return 'navigate to';
        if (level === 1) return 'view';
        
        return labels[Math.floor(Math.random() * labels.length)];
    }

    /**
     * Validate WordPress graph data structure
     */
    function isValidWordPressData(data) {
        if (!data || typeof data !== 'object') {
            return false;
        }
        
        // Check if data has required properties
        if (!data.nodes || !Array.isArray(data.nodes)) {
            return false;
        }
        
        if (!data.links || !Array.isArray(data.links)) {
            return false;
        }
        
        // Check if nodes array has at least one node
        if (data.nodes.length === 0) {
            return false;
        }
        
        // Validate that each node has required properties
        const hasValidNodes = data.nodes.every(node => 
            node.id != null && 
            typeof node.label === 'string' && 
            typeof node.type === 'string'
        );
        
        if (!hasValidNodes) {
            return false;
        }
        
        return true;
    }
    
    /**
     * Build graph data structure for force-directed layout
     * Uses WordPress posts, pages, categories, and tags data passed from PHP
     */
    function buildGraphData() {
        // Check if WordPress data is available with thorough validation
        if (isValidWordPressData(window.themisdbGraphData)) {
            console.log('Using WordPress content data for graph');
            return processWordPressGraphData(window.themisdbGraphData);
        }
        
        // Fallback to menu-based graph if no WordPress data
        console.log('Falling back to menu-based graph');
        return buildMenuGraphData();
    }
    
    /**
     * Process WordPress content data into graph format
     */
    function processWordPressGraphData(wpData) {
        const nodes = [];
        const links = [];
        
        // Process nodes and add visual properties
        wpData.nodes.forEach((node, index) => {
            const themeColor = getThemeColorByType(node.type, index);
            
            nodes.push({
                id: node.id,
                label: node.label,
                url: node.url,
                level: node.level,
                type: node.type,
                color: themeColor.fill,
                icon: themeColor.label,
                size: getNodeSize(node.type, node.level),
                excerpt: node.excerpt || '',
                count: node.count || 0,
                date: node.date || ''
            });
        });
        
        // Process links
        wpData.links.forEach(link => {
            links.push({
                source: link.source,
                target: link.target,
                label: getLinkLabel(link.type),
                strength: getLinkStrength(link.type)
            });
        });
        
        return { nodes, links };
    }
    
    /**
     * Get node size based on type and level
     */
    function getNodeSize(type, level) {
        const sizes = {
            'home': 35,
            'category': 28,
            'tag': 25,
            'page': 26,
            'post': 22
        };
        return sizes[type] || Math.max(20, 30 - level * 5);
    }
    
    /**
     * Get theme color based on node type
     */
    function getThemeColorByType(type, index) {
        const typeColors = {
            'home': { fill: '#7c4dff', stroke: '#651fff', class: 'homeNode', label: '🏠' },
            'category': { fill: '#3498db', stroke: '#2980b9', class: 'categoryNode', label: '📁' },
            'tag': { fill: '#f39c12', stroke: '#e67e22', class: 'tagNode', label: '🏷️' },
            'page': { fill: '#27ae60', stroke: '#229954', class: 'pageNode', label: '📄' },
            'post': { fill: '#e74c3c', stroke: '#c0392b', class: 'postNode', label: '📝' }
        };
        
        return typeColors[type] || { fill: '#95a5a6', stroke: '#7f8c8d', class: 'defaultNode', label: '●' };
    }
    
    /**
     * Get link label based on relationship type
     */
    function getLinkLabel(type) {
        const labels = {
            'contains': 'contains',
            'tagged': 'tagged with',
            'has_post': 'has post',
            'has_tag': 'tagged',
            'page_of': 'page of'
        };
        return labels[type] || 'related to';
    }
    
    /**
     * Get link strength based on relationship type
     */
    function getLinkStrength(type) {
        const strengths = {
            'contains': 0.8,
            'tagged': 0.7,
            'has_post': 1.0,
            'has_tag': 0.9,
            'page_of': 0.9
        };
        return strengths[type] || 0.5;
    }
    
    /**
     * Build graph data from navigation menu (fallback)
     */
    function buildMenuGraphData() {
        const menu = document.querySelector('#primary-menu');
        if (!menu) return null;

        const nodes = [];
        const links = [];

        // Add home node (center)
        nodes.push({
            id: 'home',
            label: 'Home',
            url: '/',
            level: 0,
            type: 'home',
            color: '#7c4dff',
            icon: '🏠',
            size: 30
        });

        let nodeIdCounter = 0;

        /**
         * Recursive function to process menu items
         */
        function processMenuItem(item, parentId, level) {
            const link = item.querySelector(':scope > a');
            if (!link) return null;

            const nodeId = `node_${nodeIdCounter++}`;
            const title = link.textContent.trim();
            const url = link.getAttribute('href');
            const themeColor = getThemeColor(title, nodeIdCounter);

            // Add node
            nodes.push({
                id: nodeId,
                label: title,
                url: url,
                level: level,
                type: themeColor.class,
                color: themeColor.fill,
                icon: themeColor.label,
                size: Math.max(20, 30 - level * 5)
            });

            // Add link
            links.push({
                source: parentId,
                target: nodeId,
                label: getEdgeLabel('', title, level),
                strength: 1 / (level + 1)
            });

            // Process children recursively
            const submenu = item.querySelector(':scope > ul');
            if (submenu) {
                const children = submenu.querySelectorAll(':scope > li');
                children.forEach(child => {
                    processMenuItem(child, nodeId, level + 1);
                });
            }

            return nodeId;
        }

        // Process all top-level menu items
        const topItems = menu.querySelectorAll(':scope > li');
        topItems.forEach(item => {
            processMenuItem(item, 'home', 1);
        });

        return { nodes, links };
    }

    /**
     * Create Neo4j Bloom-inspired force-directed graph
     */
    function createForceDirectedGraph(container, graphData) {
        if (!d3) {
            console.error('D3.js not loaded');
            return;
        }

        const width = container.clientWidth;
        const height = container.clientHeight;

        // Clear container
        container.innerHTML = '';

        // Create SVG
        const svg = d3.select(container)
            .append('svg')
            .attr('width', width)
            .attr('height', height)
            .attr('class', 'bloom-graph');

        // Add zoom behavior
        const zoom = d3.zoom()
            .scaleExtent([0.1, 4])
            .on('zoom', (event) => {
                g.attr('transform', event.transform);
                transform = event.transform;
            });

        svg.call(zoom);

        // Create group for zoom/pan
        const g = svg.append('g');

        // Add definitions for gradients and glows
        const defs = svg.append('defs');

        // Glow filter
        const filter = defs.append('filter')
            .attr('id', 'glow')
            .attr('x', '-50%')
            .attr('y', '-50%')
            .attr('width', '200%')
            .attr('height', '200%');

        filter.append('feGaussianBlur')
            .attr('stdDeviation', '3')
            .attr('result', 'coloredBlur');

        const feMerge = filter.append('feMerge');
        feMerge.append('feMergeNode').attr('in', 'coloredBlur');
        feMerge.append('feMergeNode').attr('in', 'SourceGraphic');

        // Create arrow markers for links
        defs.append('marker')
            .attr('id', 'arrowhead')
            .attr('viewBox', '0 -5 10 10')
            .attr('refX', 20)
            .attr('refY', 0)
            .attr('markerWidth', 8)
            .attr('markerHeight', 8)
            .attr('orient', 'auto')
            .append('path')
            .attr('d', 'M0,-5L10,0L0,5')
            .attr('fill', '#3498db');

        // Create force simulation
        simulation = d3.forceSimulation(graphData.nodes)
            .force('link', d3.forceLink(graphData.links)
                .id(d => d.id)
                .distance(d => config.physics.linkDistance * (1 + d.source.level * 0.2))
                .strength(d => d.strength))
            .force('charge', d3.forceManyBody()
                .strength(config.physics.charge))
            .force('center', d3.forceCenter(width / 2, height / 2)
                .strength(config.physics.centerStrength))
            .force('collision', d3.forceCollide()
                .radius(d => d.size + config.physics.collisionRadius))
            .force('x', d3.forceX(width / 2).strength(0.02))
            .force('y', d3.forceY(height / 2).strength(0.02));

        // Create links
        const link = g.append('g')
            .attr('class', 'links')
            .selectAll('line')
            .data(graphData.links)
            .enter()
            .append('line')
            .attr('class', 'link')
            .attr('stroke', '#3498db')
            .attr('stroke-width', 2)
            .attr('stroke-opacity', 0.6)
            .attr('marker-end', 'url(#arrowhead)');

        // Create link labels
        const linkLabel = g.append('g')
            .attr('class', 'link-labels')
            .selectAll('text')
            .data(graphData.links)
            .enter()
            .append('text')
            .attr('class', 'link-label')
            .attr('text-anchor', 'middle')
            .attr('dy', -5)
            .style('font-size', '10px')
            .style('fill', '#ecf0f1')
            .style('opacity', 0)
            .text(d => d.label);

        // Create node groups
        const node = g.append('g')
            .attr('class', 'nodes')
            .selectAll('g')
            .data(graphData.nodes)
            .enter()
            .append('g')
            .attr('class', 'node')
            .call(d3.drag()
                .on('start', dragstarted)
                .on('drag', dragged)
                .on('end', dragended));

        // Add circles to nodes
        node.append('circle')
            .attr('r', d => d.size)
            .attr('fill', d => d.color)
            .attr('stroke', d => d3.rgb(d.color).darker(1))
            .attr('stroke-width', 2)
            .style('filter', 'url(#glow)')
            .style('cursor', 'pointer');

        // Add icons to nodes
        node.append('text')
            .attr('class', 'node-icon')
            .attr('text-anchor', 'middle')
            .attr('dy', 5)
            .style('font-size', d => `${d.size * 0.7}px`)
            .style('pointer-events', 'none')
            .text(d => d.icon);

        // Add labels below nodes
        node.append('text')
            .attr('class', 'node-label')
            .attr('text-anchor', 'middle')
            .attr('dy', d => d.size + 15)
            .style('font-size', '12px')
            .style('font-weight', '600')
            .style('fill', '#fff')
            .style('pointer-events', 'none')
            .text(d => d.label);

        // Create tooltip element
        const tooltip = d3.select(container)
            .append('div')
            .attr('class', 'graph-tooltip')
            .style('position', 'absolute')
            .style('visibility', 'hidden')
            .style('background', 'linear-gradient(135deg, rgba(44, 62, 80, 0.98) 0%, rgba(52, 73, 94, 0.98) 100%)')
            .style('color', '#fff')
            .style('padding', '16px')
            .style('border-radius', '12px')
            .style('font-size', '14px')
            .style('max-width', '320px')
            .style('min-width', '200px')
            .style('box-shadow', '0 8px 24px rgba(0,0,0,0.4)')
            .style('pointer-events', 'none')
            .style('z-index', '10000')
            .style('border', '2px solid rgba(124, 77, 255, 0.5)')
            .style('line-height', '1.6');

        // Add hover effects
        node.on('mouseover', function(event, d) {
            d3.select(this).select('circle')
                .transition()
                .duration(200)
                .attr('r', d.size * 1.3)
                .attr('stroke-width', 4);

            // Highlight connected links
            link.style('stroke-opacity', l => 
                (l.source.id === d.id || l.target.id === d.id) ? 1 : 0.2
            ).style('stroke-width', l => 
                (l.source.id === d.id || l.target.id === d.id) ? 3 : 2
            );

            // Show link labels for connected edges
            linkLabel.style('opacity', l =>
                (l.source.id === d.id || l.target.id === d.id) ? 1 : 0
            );
            
            // Show tooltip with node information and quick link
            let tooltipHTML = '<div class="graph-tooltip-header">';
            tooltipHTML += `<span class="graph-tooltip-title">${d.icon} ${d.label}</span>`;
            tooltipHTML += '</div>';
            
            // Type badge
            const typeColors = {
                'home': '#7c4dff',
                'category': '#3498db',
                'tag': '#f39c12',
                'page': '#27ae60',
                'post': '#e74c3c'
            };
            const typeColor = typeColors[d.type] || '#95a5a6';
            tooltipHTML += `<div style="margin-bottom: 12px;"><span class="graph-tooltip-badge" style="background: ${typeColor};">${d.type}</span></div>`;
            
            // Content information
            if (d.excerpt && d.excerpt.trim()) {
                tooltipHTML += `<div class="graph-tooltip-excerpt">${d.excerpt}</div>`;
            }
            
            // Metadata
            if (d.count && d.count > 0) {
                tooltipHTML += `<div class="graph-tooltip-meta">📊 <strong>${d.count}</strong> ${d.type === 'category' ? 'posts' : 'items'}</div>`;
            }
            
            if (d.date) {
                const date = new Date(d.date);
                const formattedDate = date.toLocaleDateString('de-DE', { 
                    year: 'numeric', 
                    month: 'long', 
                    day: 'numeric' 
                });
                tooltipHTML += `<div class="graph-tooltip-meta">📅 ${formattedDate}</div>`;
            }
            
            // Quick link button
            tooltipHTML += '<div class="graph-tooltip-footer">';
            tooltipHTML += '<span class="graph-tooltip-button">🔗 Klicken zum Öffnen</span>';
            tooltipHTML += '</div>';
            
            tooltip
                .html(tooltipHTML)
                .style('visibility', 'visible')
                .style('left', (event.pageX + 15) + 'px')
                .style('top', (event.pageY - 15) + 'px');
        })
        .on('mousemove', function(event) {
            tooltip
                .style('left', (event.pageX + 15) + 'px')
                .style('top', (event.pageY - 15) + 'px');
        })
        .on('mouseout', function(event, d) {
            d3.select(this).select('circle')
                .transition()
                .duration(200)
                .attr('r', d.size)
                .attr('stroke-width', 2);

            link.style('stroke-opacity', 0.6)
                .style('stroke-width', 2);

            linkLabel.style('opacity', 0);
            
            tooltip.style('visibility', 'hidden');
        })
        .on('click', function(event, d) {
            if (d.url) {
                window.location.href = d.url;
            }
        });

        // Update positions on simulation tick
        simulation.on('tick', () => {
            link
                .attr('x1', d => d.source.x)
                .attr('y1', d => d.source.y)
                .attr('x2', d => d.target.x)
                .attr('y2', d => d.target.y);

            linkLabel
                .attr('x', d => (d.source.x + d.target.x) / 2)
                .attr('y', d => (d.source.y + d.target.y) / 2);

            node.attr('transform', d => `translate(${d.x},${d.y})`);
        });

        // Drag functions
        function dragstarted(event, d) {
            if (!event.active) simulation.alphaTarget(0.3).restart();
            d.fx = d.x;
            d.fy = d.y;
        }

        function dragged(event, d) {
            d.fx = event.x;
            d.fy = event.y;
        }

        function dragended(event, d) {
            if (!event.active) simulation.alphaTarget(0);
            d.fx = null;
            d.fy = null;
        }

        // Initial zoom to fit
        const bounds = g.node().getBBox();
        const fullWidth = bounds.width;
        const fullHeight = bounds.height;
        const midX = bounds.x + fullWidth / 2;
        const midY = bounds.y + fullHeight / 2;

        if (fullWidth > 0 && fullHeight > 0) {
            const scale = 0.8 / Math.max(fullWidth / width, fullHeight / height);
            const translate = [width / 2 - scale * midX, height / 2 - scale * midY];

            svg.call(zoom.transform, d3.zoomIdentity
                .translate(translate[0], translate[1])
                .scale(scale));
        }

        return { simulation, svg, zoom };
    }

    /**
     * Create fallback HTML navigation tree (for browsers without D3)
     */
    function createFallbackTree(nodes) {
        let html = '<div class="fallback-nav-tree">';
        
        // Group nodes by level for hierarchical display
        const levels = new Map();
        nodes.forEach(node => {
            if (!levels.has(node.level)) {
                levels.set(node.level, []);
            }
            levels.get(node.level).push(node);
        });

        // Start node (home)
        const homeNode = nodes.find(n => n.id === 'home');
        if (homeNode) {
            html += '<div class="nav-level level-0">';
            html += `<div class="nav-node start-node ${homeNode.type}" data-url="${homeNode.url}" onclick="window.location.href='${homeNode.url}'">`;
            html += `<span class="node-icon">${homeNode.icon}</span>`;
            html += `<span class="node-label">${homeNode.label}</span>`;
            html += '</div>';
            html += '</div>';
        }

        // Process other levels
        levels.forEach((nodeList, level) => {
            if (level === 0) return; // Skip home level
            
            html += `<div class="nav-level level-${level}">`;
            
            nodeList.forEach((node, index) => {
                if (node.id === 'home') return;
                
                const indent = level * 40;
                
                // Edge connector
                if (index > 0 || level > 1) {
                    html += `<div class="nav-connector" style="margin-left: ${indent}px" data-level="${level}">`;
                    html += '<span class="connector-label">└─</span>';
                    html += '</div>';
                }
                
                html += `<div class="nav-node ${node.type}" data-url="${node.url}" data-level="${level}" style="margin-left: ${indent}px" onclick="window.location.href='${node.url}'">`;
                html += `<span class="node-icon">${node.icon}</span>`;
                html += `<span class="node-label">${node.label}</span>`;
                html += '</div>';
            });
            
            html += '</div>';
        });

        html += '</div>';
        return html;
    }

    /**
     * Render navigation overlay
     */
    async function renderNavigationOverlay() {
        const graphData = buildGraphData();
        if (!graphData || graphData.nodes.length === 0) {
            console.warn('No navigation menu found');
            return;
        }

        const overlay = document.getElementById('themisdb-nav-overlay');
        const container = overlay.querySelector('.nav-graph-container');

        if (d3Loaded && d3) {
            // Use D3.js force-directed graph (Neo4j Bloom style)
            createForceDirectedGraph(container, graphData);
        } else {
            // Use fallback
            container.innerHTML = createFallbackTree(graphData.nodes);
            animateFallbackTree(container);
        }
    }

    /**
     * Animate fallback tree
     */
    function animateFallbackTree(container) {
        const nodes = container.querySelectorAll('.nav-node');
        const connectors = container.querySelectorAll('.nav-connector');

        nodes.forEach((node, index) => {
            setTimeout(() => {
                node.classList.add('visible');
            }, index * 100);
        });

        setTimeout(() => {
            connectors.forEach((connector, index) => {
                setTimeout(() => {
                    connector.classList.add('visible');
                }, index * 50);
            });
        }, nodes.length * 100);
    }

    /**
     * Toggle overlay visibility
     */
    function toggleOverlay() {
        const overlay = document.getElementById('themisdb-nav-overlay');
        const button = document.getElementById('nav-graph-toggle');
        
        if (overlay.classList.contains('visible')) {
            overlay.classList.remove('visible');
            button.setAttribute('aria-expanded', 'false');
            button.innerHTML = `<svg width="24" height="24" fill="currentColor" viewBox="0 0 16 16">
                <path fill-rule="evenodd" d="M2 2.5a.5.5 0 0 1 .5-.5h3a.5.5 0 0 1 0 1H3v2.5a.5.5 0 0 1-1 0v-3zm12 12a.5.5 0 0 1-.5.5h-3a.5.5 0 0 1 0-1H13v-2.5a.5.5 0 0 1 1 0v3zM2 9.5a.5.5 0 0 1 .5.5v2.5H5a.5.5 0 0 1 0 1H2.5a.5.5 0 0 1-.5-.5v-3a.5.5 0 0 1 .5-.5zm12-7a.5.5 0 0 1 .5.5v3a.5.5 0 0 1-1 0V3h-2.5a.5.5 0 0 1 0-1h3a.5.5 0 0 1 .5.5z"/>
            </svg>`;
        } else {
            overlay.classList.add('visible');
            button.setAttribute('aria-expanded', 'true');
            button.innerHTML = `<svg width="24" height="24" fill="currentColor" viewBox="0 0 16 16">
                <path d="M2.146 2.854a.5.5 0 1 1 .708-.708L8 7.293l5.146-5.147a.5.5 0 0 1 .708.708L8.707 8l5.147 5.146a.5.5 0 0 1-.708.708L8 8.707l-5.146 5.147a.5.5 0 0 1-.708-.708L7.293 8 2.146 2.854Z"/>
            </svg>`;
        }
    }

    /**
     * Initialize navigation graph overlay
     */
    async function init() {
        // Create overlay HTML
        const overlay = document.createElement('div');
        overlay.id = 'themisdb-nav-overlay';
        overlay.className = 'nav-graph-overlay';
        overlay.innerHTML = `
            <div class="nav-graph-container">
                <div class="loading">
                    <div class="loading-spinner"></div>
                    <p>Loading interactive graph navigation...</p>
                </div>
            </div>
            <div class="graph-controls">
                <button class="control-btn" id="zoom-in" title="Zoom In">
                    <svg width="20" height="20" fill="currentColor" viewBox="0 0 16 16">
                        <path d="M8 4a.5.5 0 0 1 .5.5v3h3a.5.5 0 0 1 0 1h-3v3a.5.5 0 0 1-1 0v-3h-3a.5.5 0 0 1 0-1h3v-3A.5.5 0 0 1 8 4z"/>
                    </svg>
                </button>
                <button class="control-btn" id="zoom-out" title="Zoom Out">
                    <svg width="20" height="20" fill="currentColor" viewBox="0 0 16 16">
                        <path d="M4 8a.5.5 0 0 1 .5-.5h7a.5.5 0 0 1 0 1h-7A.5.5 0 0 1 4 8z"/>
                    </svg>
                </button>
                <button class="control-btn" id="reset-view" title="Reset View">
                    <svg width="20" height="20" fill="currentColor" viewBox="0 0 16 16">
                        <path fill-rule="evenodd" d="M8 3a5 5 0 1 0 4.546 2.914.5.5 0 0 1 .908-.417A6 6 0 1 1 8 2v1z"/>
                        <path d="M8 4.466V.534a.25.25 0 0 1 .41-.192l2.36 1.966c.12.1.12.284 0 .384L8.41 4.658A.25.25 0 0 1 8 4.466z"/>
                    </svg>
                </button>
            </div>
        `;
        document.body.appendChild(overlay);

        // Create toggle button
        const button = document.createElement('button');
        button.id = 'nav-graph-toggle';
        button.className = 'nav-graph-toggle';
        button.setAttribute('aria-label', 'Toggle Navigation Graph');
        button.setAttribute('aria-expanded', 'false');
        button.innerHTML = `<svg width="24" height="24" fill="currentColor" viewBox="0 0 16 16">
            <path fill-rule="evenodd" d="M2 2.5a.5.5 0 0 1 .5-.5h3a.5.5 0 0 1 0 1H3v2.5a.5.5 0 0 1-1 0v-3zm12 12a.5.5 0 0 1-.5.5h-3a.5.5 0 0 1 0-1H13v-2.5a.5.5 0 0 1 1 0v3zM2 9.5a.5.5 0 0 1 .5.5v2.5H5a.5.5 0 0 1 0 1H2.5a.5.5 0 0 1-.5-.5v-3a.5.5 0 0 1 .5-.5zm12-7a.5.5 0 0 1 .5.5v3a.5.5 0 0 1-1 0V3h-2.5a.5.5 0 0 1 0-1h3a.5.5 0 0 1 .5.5z"/>
        </svg>`;
        
        // Insert button in header (far left)
        const header = document.querySelector('.header-inner');
        if (header) {
            header.insertAdjacentElement('afterbegin', button);
        }

        // Add event listeners
        button.addEventListener('click', toggleOverlay);

        // Close on escape key
        document.addEventListener('keydown', (e) => {
            if (e.key === 'Escape' && overlay.classList.contains('visible')) {
                toggleOverlay();
            }
        });

        // Close when clicking outside
        overlay.addEventListener('click', (e) => {
            if (e.target === overlay) {
                toggleOverlay();
            }
        });

        // Load D3.js
        await loadD3();

        // Render navigation
        await renderNavigationOverlay();

        // Setup zoom controls if D3 is loaded
        if (d3Loaded && simulation) {
            document.getElementById('zoom-in')?.addEventListener('click', () => {
                const svg = d3.select('.bloom-graph');
                svg.transition().call(svg.__zoom.scaleBy, 1.3);
            });

            document.getElementById('zoom-out')?.addEventListener('click', () => {
                const svg = d3.select('.bloom-graph');
                svg.transition().call(svg.__zoom.scaleBy, 0.7);
            });

            document.getElementById('reset-view')?.addEventListener('click', () => {
                const svg = d3.select('.bloom-graph');
                svg.transition().call(svg.__zoom.transform, d3.zoomIdentity);
            });
        }
    }

    // Initialize when DOM is ready
    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', init);
    } else {
        init();
    }

})();
