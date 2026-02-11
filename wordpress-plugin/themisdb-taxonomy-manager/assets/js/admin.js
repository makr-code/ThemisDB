/* ThemisDB Taxonomy Manager - Admin JavaScript */

(function($) {
    'use strict';
    
    $(document).ready(function() {
        // Tab switching
        $('.nav-tab').on('click', function(e) {
            e.preventDefault();
            var target = $(this).attr('href');
            
            $('.nav-tab').removeClass('nav-tab-active');
            $(this).addClass('nav-tab-active');
            
            $('.tab-content').hide();
            $(target).show();
        });
        
        // Consolidation button handler
        $('#btn-consolidate').on('click', function() {
            var $btn = $(this);
            var originalText = $btn.text();
            
            $btn.prop('disabled', true).text('Processing...');
            
            $.post(themisdbTaxonomy.ajaxurl, {
                action: 'themisdb_consolidate_categories',
                nonce: themisdbTaxonomy.nonce
            }, function(response) {
                if (response.success) {
                    var html = '<h3>Consolidation Results:</h3>';
                    html += '<ul>';
                    html += '<li><strong>Consolidated:</strong> ' + response.data.consolidated + ' categories</li>';
                    html += '<li><strong>Hierarchized:</strong> ' + response.data.hierarchized + ' categories</li>';
                    html += '<li><strong>Errors:</strong> ' + response.data.errors + '</li>';
                    html += '</ul>';
                    $('#optimization-results').html(html);
                } else {
                    $('#optimization-results').html('<p class="error">' + response.data.message + '</p>');
                }
                
                $btn.prop('disabled', false).text(originalText);
            }).fail(function() {
                $('#optimization-results').html('<p class="error">Failed to consolidate categories.</p>');
                $btn.prop('disabled', false).text(originalText);
            });
        });
        
        // Recommendations button handler
        $('#btn-get-recommendations').on('click', function() {
            var $btn = $(this);
            var originalText = $btn.text();
            
            $btn.prop('disabled', true).text('Loading...');
            
            $.post(themisdbTaxonomy.ajaxurl, {
                action: 'themisdb_get_recommendations',
                nonce: themisdbTaxonomy.nonce
            }, function(response) {
                if (response.success) {
                    var html = '<h3>Optimization Recommendations:</h3>';
                    
                    if (response.data.length === 0) {
                        html += '<p><strong>Great!</strong> No recommendations. Your categories are optimized!</p>';
                    } else {
                        html += '<ul>';
                        response.data.forEach(function(rec) {
                            html += '<li><strong>' + rec.current_name + '</strong> (' + rec.post_count + ' posts):<ul>';
                            rec.actions.forEach(function(action) {
                                html += '<li><em>' + action.type + '</em>: ' + action.target + ' - <span style="color:#666;">' + action.reason + '</span></li>';
                            });
                            html += '</ul></li>';
                        });
                        html += '</ul>';
                    }
                    
                    $('#optimization-results').html(html);
                } else {
                    $('#optimization-results').html('<p class="error">' + response.data.message + '</p>');
                }
                
                $btn.prop('disabled', false).text(originalText);
            }).fail(function() {
                $('#optimization-results').html('<p class="error">Failed to get recommendations.</p>');
                $btn.prop('disabled', false).text(originalText);
            });
        });
    });
    
})(jQuery);
