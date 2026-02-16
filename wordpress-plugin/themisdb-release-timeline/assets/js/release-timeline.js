/**
 * ThemisDB Release Timeline Visualizer - JavaScript
 * Version: 1.0.0
 * License: MIT
 */

(function($) {
    'use strict';
    
    // Initialize Mermaid
    if (typeof mermaid !== 'undefined') {
        mermaid.initialize({
            startOnLoad: false,
            theme: 'neutral',
            securityLevel: 'loose',
            flowchart: {
                useMaxWidth: true,
                htmlLabels: true
            }
        });
    }
    
    // Timeline Visualizer Class
    class ReleaseTimelineVisualizer {
        constructor(container) {
            this.container = container;
            this.currentView = container.dataset.view || 'chronological';
            this.currentTheme = container.dataset.theme || 'neutral';
            this.source = container.dataset.source || 'github';
            this.releases = [];
            this.zoomLevel = 1.0;
            
            this.init();
        }
        
        init() {
            this.bindEvents();
            this.loadData();
        }
        
        bindEvents() {
            const self = this;
            
            // View change
            this.container.querySelectorAll('.themisdb-rt-view-selector').forEach(btn => {
                btn.addEventListener('click', function() {
                    self.currentView = this.dataset.view;
                    self.render();
                });
            });
            
            // Theme change
            const themeSelect = this.container.querySelector('.themisdb-rt-theme-select');
            if (themeSelect) {
                themeSelect.addEventListener('change', function() {
                    self.currentTheme = this.value;
                    if (typeof mermaid !== 'undefined') {
                        mermaid.initialize({ theme: self.currentTheme });
                    }
                    self.render();
                });
            }
            
            // Reload button
            const reloadBtn = this.container.querySelector('.themisdb-rt-reload');
            if (reloadBtn) {
                reloadBtn.addEventListener('click', () => this.loadData(true));
            }
            
            // Export buttons
            const exportSvgBtn = this.container.querySelector('.themisdb-rt-export-svg');
            if (exportSvgBtn) {
                exportSvgBtn.addEventListener('click', () => this.exportSVG());
            }
            
            const exportPngBtn = this.container.querySelector('.themisdb-rt-export-png');
            if (exportPngBtn) {
                exportPngBtn.addEventListener('click', () => this.exportPNG());
            }
            
            // Zoom controls
            const zoomInBtn = this.container.querySelector('.themisdb-rt-zoom-in');
            if (zoomInBtn) {
                zoomInBtn.addEventListener('click', () => this.zoom(0.1));
            }
            
            const zoomOutBtn = this.container.querySelector('.themisdb-rt-zoom-out');
            if (zoomOutBtn) {
                zoomOutBtn.addEventListener('click', () => this.zoom(-0.1));
            }
            
            const zoomResetBtn = this.container.querySelector('.themisdb-rt-zoom-reset');
            if (zoomResetBtn) {
                zoomResetBtn.addEventListener('click', () => this.resetZoom());
            }
            
            // Fullscreen
            const fullscreenBtn = this.container.querySelector('.themisdb-rt-fullscreen');
            if (fullscreenBtn) {
                fullscreenBtn.addEventListener('click', () => this.toggleFullscreen());
            }
        }
        
        loadData(forceReload = false) {
            this.showLoading();
            
            $.ajax({
                url: themisdbRTData.ajaxUrl,
                method: 'POST',
                data: {
                    action: 'themisdb_rt_load_data',
                    nonce: themisdbRTData.nonce,
                    source: this.source,
                    view: this.currentView,
                    releases: this.container.dataset.releases || 10,
                    force_reload: forceReload
                },
                success: (response) => {
                    if (response.success) {
                        this.releases = response.data;
                        this.render();
                    } else {
                        this.showError('Failed to load release data');
                    }
                },
                error: () => {
                    this.showError('Network error occurred');
                }
            });
        }
        
        render() {
            const content = this.container.querySelector('.themisdb-rt-content');
            if (!content) return;
            
            let diagram = '';
            
            switch (this.currentView) {
                case 'chronological':
                    diagram = this.generateChronological();
                    break;
                case 'gantt':
                    diagram = this.generateGantt();
                    break;
                case 'mindmap':
                    diagram = this.generateMindmap();
                    break;
                default:
                    diagram = this.generateChronological();
            }
            
            this.renderMermaid(content, diagram);
        }
        
        generateChronological() {
            let diagram = 'timeline\n';
            diagram += '    title ThemisDB Release Timeline\n';
            
            this.releases.forEach(release => {
                const breaking = release.breaking ? ' ⚠️' : '';
                diagram += `    ${release.date} : ${release.version}${breaking}\n`;
                
                if (release.features && release.features.length > 0) {
                    diagram += `                : ${release.features[0]}\n`;
                }
            });
            
            return diagram;
        }
        
        generateGantt() {
            let diagram = 'gantt\n';
            diagram += '    title ThemisDB Release Schedule\n';
            diagram += '    dateFormat YYYY-MM-DD\n';
            diagram += '    section Releases\n';
            
            this.releases.forEach((release, index) => {
                const nextDate = this.releases[index + 1] ? this.releases[index + 1].date : release.date;
                const status = release.breaking ? 'crit' : 'done';
                diagram += `    ${release.version} : ${status}, ${release.date}, ${nextDate}\n`;
            });
            
            return diagram;
        }
        
        generateMindmap() {
            let diagram = 'mindmap\n';
            diagram += '  root((ThemisDB Releases))\n';
            
            this.releases.slice(0, 5).forEach(release => {
                diagram += `    ${release.version}\n`;
                
                if (release.features && release.features.length > 0) {
                    release.features.slice(0, 3).forEach(feature => {
                        const cleanFeature = feature.replace(/[()]/g, '').substring(0, 30);
                        diagram += `      ${cleanFeature}\n`;
                    });
                }
            });
            
            return diagram;
        }
        
        renderMermaid(container, diagram) {
            container.innerHTML = `<div class="themisdb-rt-diagram"><pre class="mermaid">${diagram}</pre></div>`;
            
            if (typeof mermaid !== 'undefined') {
                mermaid.run({
                    querySelector: '.themisdb-rt-diagram .mermaid'
                });
            }
        }
        
        showLoading() {
            const content = this.container.querySelector('.themisdb-rt-content');
            if (content) {
                content.innerHTML = `
                    <div class="themisdb-rt-loading">
                        <div class="themisdb-rt-spinner"></div>
                        <p>Loading release timeline...</p>
                    </div>
                `;
            }
        }
        
        showError(message) {
            const content = this.container.querySelector('.themisdb-rt-content');
            if (content) {
                content.innerHTML = `
                    <div class="themisdb-rt-error">
                        <strong>Error:</strong> ${message}
                    </div>
                `;
            }
        }
        
        zoom(delta) {
            this.zoomLevel = Math.max(0.5, Math.min(2.0, this.zoomLevel + delta));
            const diagram = this.container.querySelector('.themisdb-rt-diagram');
            if (diagram) {
                diagram.style.transform = `scale(${this.zoomLevel})`;
                diagram.style.transformOrigin = 'top left';
            }
        }
        
        resetZoom() {
            this.zoomLevel = 1.0;
            const diagram = this.container.querySelector('.themisdb-rt-diagram');
            if (diagram) {
                diagram.style.transform = 'scale(1)';
            }
        }
        
        exportSVG() {
            const svg = this.container.querySelector('.themisdb-rt-diagram svg');
            if (!svg) {
                this.showNotification('No diagram available to export', 'warning');
                return;
            }
            
            const serializer = new XMLSerializer();
            const svgString = serializer.serializeToString(svg);
            const blob = new Blob([svgString], { type: 'image/svg+xml' });
            const url = URL.createObjectURL(blob);
            
            const a = document.createElement('a');
            a.href = url;
            a.download = `themisdb-release-timeline-${Date.now()}.svg`;
            a.click();
            
            URL.revokeObjectURL(url);
        }
        
        exportPNG() {
            const svg = this.container.querySelector('.themisdb-rt-diagram svg');
            if (!svg) {
                this.showNotification('No diagram available to export', 'warning');
                return;
            }
            
            const canvas = document.createElement('canvas');
            const ctx = canvas.getContext('2d');
            const svgData = new XMLSerializer().serializeToString(svg);
            const img = new Image();
            
            img.onload = function() {
                canvas.width = img.width;
                canvas.height = img.height;
                ctx.drawImage(img, 0, 0);
                
                canvas.toBlob(function(blob) {
                    const url = URL.createObjectURL(blob);
                    const a = document.createElement('a');
                    a.href = url;
                    a.download = `themisdb-release-timeline-${Date.now()}.png`;
                    a.click();
                    URL.revokeObjectURL(url);
                });
            };
            
            // Use modern encoding without deprecated unescape()
            const blob = new Blob([svgData], { type: 'image/svg+xml;charset=utf-8' });
            img.src = URL.createObjectURL(blob);
        }
        
        showNotification(message, type = 'info') {
            const notification = document.createElement('div');
            notification.className = `themisdb-rt-notification ${type}`;
            notification.textContent = message;
            notification.style.cssText = `
                position: fixed;
                top: 20px;
                right: 20px;
                padding: 1rem 1.5rem;
                background: ${type === 'warning' ? '#f59e0b' : '#10b981'};
                color: white;
                border-radius: 8px;
                box-shadow: 0 4px 12px rgba(0,0,0,0.15);
                z-index: 10000;
                animation: slideIn 0.3s ease-out;
            `;
            
            document.body.appendChild(notification);
            
            setTimeout(() => {
                notification.style.animation = 'slideOut 0.3s ease-in';
                setTimeout(() => notification.remove(), 300);
            }, 3000);
        }
        
        toggleFullscreen() {
            this.container.classList.toggle('themisdb-rt-fullscreen');
            
            if (this.container.classList.contains('themisdb-rt-fullscreen')) {
                const closeBtn = document.createElement('button');
                closeBtn.className = 'themisdb-rt-fullscreen-close';
                closeBtn.textContent = 'Exit Fullscreen';
                closeBtn.onclick = () => this.toggleFullscreen();
                this.container.appendChild(closeBtn);
            } else {
                const closeBtn = this.container.querySelector('.themisdb-rt-fullscreen-close');
                if (closeBtn) closeBtn.remove();
            }
        }
    }
    
    // Initialize on DOM ready
    $(document).ready(function() {
        $('.themisdb-rt-container').each(function() {
            new ReleaseTimelineVisualizer(this);
        });
    });
    
})(jQuery);
