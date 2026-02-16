/**
 * ThemisDB Query Playground JavaScript
 */

(function($) {
    'use strict';

    window.ThemisDBQueryPlayground = {
        editor: null,
        currentView: 'table',
        lastResults: null,

        init: function() {
            this.initEditor();
            this.setupEventListeners();
            this.loadExamples();
        },

        initEditor: function() {
            const textarea = document.getElementById('qp-editor');
            if (!textarea) return;

            this.editor = CodeMirror.fromTextArea(textarea, {
                mode: 'text/x-sql',
                theme: themisdbQP.settings.theme || 'monokai',
                lineNumbers: true,
                lineWrapping: true,
                autofocus: true,
                extraKeys: {
                    'Ctrl-Enter': () => this.executeQuery(),
                    'Cmd-Enter': () => this.executeQuery()
                }
            });

            this.editor.on('cursorActivity', () => this.updateEditorInfo());
            this.editor.on('change', () => this.updateEditorInfo());
        },

        setupEventListeners: function() {
            const self = this;

            $('#qp-execute').on('click', () => self.executeQuery());
            $('#qp-clear').on('click', () => self.clearEditor());
            $('#qp-format').on('click', () => self.formatQuery());

            $('.qp-load-example').on('click', function() {
                const category = $(this).data('category');
                self.showExamplesForCategory(category);
                $('.qp-load-example').removeClass('active');
                $(this).addClass('active');
            });

            $('.qp-view-btn').on('click', function() {
                const view = $(this).data('view');
                self.switchView(view);
            });

            $('#qp-export-json').on('click', () => self.exportJSON());
            $('#qp-export-csv').on('click', () => self.exportCSV());
        },

        loadExamples: function() {
            $.ajax({
                url: themisdbQP.ajax_url,
                type: 'POST',
                data: {
                    action: 'themisdb_qp_get_examples',
                    nonce: themisdbQP.nonce
                },
                success: (response) => {
                    if (response.success) {
                        this.examples = response.data;
                    }
                }
            });
        },

        showExamplesForCategory: function(category) {
            if (!this.examples) return;

            const filtered = this.examples.filter(ex => ex.category === category);
            const $list = $('#qp-example-list');
            
            if (filtered.length === 0) {
                $list.hide();
                return;
            }

            let html = '';
            filtered.forEach(ex => {
                html += `<div class="themisdb-example-item" data-query="${this.escapeHtml(ex.query)}">
                    <h4>${ex.name}</h4>
                    <p>${ex.description}</p>
                </div>`;
            });

            $list.html(html).show();

            $list.find('.themisdb-example-item').on('click', function() {
                const query = $(this).data('query');
                this.setEditorValue(query);
                $list.hide();
            }.bind(this));
        },

        executeQuery: function() {
            if (!themisdbQP.settings.enable_execution) {
                this.showStatus('Query execution is disabled', 'error');
                return;
            }

            const query = this.editor.getValue().trim();
            if (!query) {
                this.showStatus('Please enter a query', 'error');
                return;
            }

            this.showStatus('Executing query...', 'info');
            $('#qp-execute').prop('disabled', true);

            $.ajax({
                url: themisdbQP.ajax_url,
                type: 'POST',
                data: {
                    action: 'themisdb_qp_execute_query',
                    nonce: themisdbQP.nonce,
                    query: query
                },
                success: (response) => {
                    if (response.success) {
                        this.displayResults(response.data);
                        this.showStatus('Query executed successfully', 'success');
                    } else {
                        this.showStatus('Error: ' + response.data.message, 'error');
                    }
                },
                error: () => {
                    this.showStatus('Network error occurred', 'error');
                },
                complete: () => {
                    $('#qp-execute').prop('disabled', false);
                }
            });
        },

        displayResults: function(data) {
            this.lastResults = data;
            
            $('#qp-result-count').text(data.count + ' result' + (data.count !== 1 ? 's' : ''));
            $('#qp-execution-time').text(data.execution_time + ' ms');
            
            $('#qp-results-section').show();
            
            this.switchView(this.currentView);
        },

        switchView: function(view) {
            this.currentView = view;
            $('.qp-view-btn').removeClass('active');
            $(`.qp-view-btn[data-view="${view}"]`).addClass('active');

            if (!this.lastResults) return;

            const $container = $('#qp-results-container');

            switch(view) {
                case 'table':
                    $container.html(this.renderTable(this.lastResults.items));
                    break;
                case 'json':
                    $container.html(this.renderJSON(this.lastResults.items));
                    break;
                case 'chart':
                    $container.html('<p>Chart view coming soon...</p>');
                    break;
            }
        },

        renderTable: function(items) {
            if (!items || items.length === 0) {
                return '<p>No results</p>';
            }

            const keys = Object.keys(items[0]);
            let html = '<table class="themisdb-results-table"><thead><tr>';
            
            keys.forEach(key => {
                html += `<th>${this.escapeHtml(key)}</th>`;
            });
            html += '</tr></thead><tbody>';

            items.forEach(item => {
                html += '<tr>';
                keys.forEach(key => {
                    const value = item[key];
                    html += `<td>${this.formatValue(value)}</td>`;
                });
                html += '</tr>';
            });

            html += '</tbody></table>';
            return html;
        },

        renderJSON: function(items) {
            const json = JSON.stringify(items, null, 2);
            return `<div class="themisdb-results-json"><pre>${this.escapeHtml(json)}</pre></div>`;
        },

        formatValue: function(value) {
            if (value === null) return '<em>null</em>';
            if (typeof value === 'object') return this.escapeHtml(JSON.stringify(value));
            return this.escapeHtml(String(value));
        },

        clearEditor: function() {
            this.editor.setValue('');
            this.editor.focus();
        },

        formatQuery: function() {
            const query = this.editor.getValue();
            // Basic formatting
            const formatted = query
                .replace(/\s+/g, ' ')
                .replace(/\s*([\(\),])\s*/g, '$1 ')
                .trim();
            this.editor.setValue(formatted);
        },

        setEditorValue: function(value) {
            this.editor.setValue(value);
            this.editor.focus();
        },

        updateEditorInfo: function() {
            const cursor = this.editor.getCursor();
            $('#qp-line-info').text(`Line ${cursor.line + 1}, Col ${cursor.ch + 1}`);
            $('#qp-char-count').text(this.editor.getValue().length + ' characters');
        },

        showStatus: function(message, type) {
            const $status = $('#qp-status');
            const $content = $('#qp-status-content');
            
            $status.removeClass('success error info').addClass(type);
            $content.html(this.escapeHtml(message));
            $status.show();

            if (type === 'success') {
                setTimeout(() => $status.fadeOut(), 3000);
            }
        },

        exportJSON: function() {
            if (!this.lastResults) return;

            const json = JSON.stringify(this.lastResults.items, null, 2);
            const blob = new Blob([json], { type: 'application/json' });
            this.downloadBlob(blob, 'query-results.json');
        },

        exportCSV: function() {
            if (!this.lastResults || !this.lastResults.items.length) return;

            const items = this.lastResults.items;
            const keys = Object.keys(items[0]);
            
            let csv = keys.join(',') + '\n';
            items.forEach(item => {
                const row = keys.map(key => {
                    const value = item[key];
                    return '"' + String(value).replace(/"/g, '""') + '"';
                });
                csv += row.join(',') + '\n';
            });

            const blob = new Blob([csv], { type: 'text/csv' });
            this.downloadBlob(blob, 'query-results.csv');
        },

        downloadBlob: function(blob, filename) {
            const url = URL.createObjectURL(blob);
            const a = document.createElement('a');
            a.href = url;
            a.download = filename;
            a.click();
            URL.revokeObjectURL(url);
        },

        escapeHtml: function(text) {
            const div = document.createElement('div');
            div.textContent = text;
            return div.innerHTML;
        }
    };

    $(document).ready(function() {
        if ($('.themisdb-query-wrapper').length > 0) {
            window.ThemisDBQueryPlayground.init();
        }
    });

})(jQuery);
