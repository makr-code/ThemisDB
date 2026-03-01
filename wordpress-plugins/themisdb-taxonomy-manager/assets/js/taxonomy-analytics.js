/**
 * Taxonomy Analytics Dashboard JavaScript
 * Handles AJAX interactions for consolidation, cleanup, and merge operations
 */

(function($) {
    'use strict';
    
    /**
     * Show result message
     */
    function showResult(message, type) {
        var $results = $('#analytics-results');
        $results.removeClass('error success').addClass(type + ' show');
        $results.html('<p>' + message + '</p>');
        
        // Auto-hide success messages after 5 seconds
        if (type === 'success') {
            setTimeout(function() {
                $results.fadeOut(function() {
                    $results.removeClass('show');
                });
            }, 5000);
        }
    }
    
    /**
     * Initialize analytics dashboard
     */
    function initAnalytics() {
        // Cleanup unused terms button
        $('#btn-cleanup').on('click', function() {
            var $btn = $(this);
            
            if (!confirm(themisdbTaxonomy.i18n.confirmCleanup || 'Delete all unused terms? This cannot be undone.')) {
                return;
            }
            
            $btn.prop('disabled', true).text(themisdbTaxonomy.i18n.processing || 'Processing...');
            
            $.ajax({
                url: themisdbTaxonomy.ajaxurl,
                type: 'POST',
                data: {
                    action: 'themisdb_cleanup_unused',
                    nonce: themisdbTaxonomy.nonce
                },
                success: function(response) {
                    if (response.success) {
                        var message = 'Deleted ' + response.data.total_deleted + ' unused terms (' + 
                                     response.data.deleted_categories + ' categories, ' + 
                                     response.data.deleted_tags + ' tags)';
                        showResult(message, 'success');
                        
                        // Reload page after 2 seconds
                        setTimeout(function() {
                            location.reload();
                        }, 2000);
                    } else {
                        showResult('Error: ' + (response.data.message || 'Unknown error'), 'error');
                        $btn.prop('disabled', false).text(themisdbTaxonomy.i18n.cleanup || 'Cleanup');
                    }
                },
                error: function() {
                    showResult('Error: AJAX request failed', 'error');
                    $btn.prop('disabled', false).text(themisdbTaxonomy.i18n.cleanup || 'Cleanup');
                }
            });
        });
        
        // Auto consolidate button
        $('#btn-auto-consolidate').on('click', function() {
            var $btn = $(this);
            
            if (!confirm(themisdbTaxonomy.i18n.confirmConsolidate || 'Automatically merge similar categories? This cannot be undone.')) {
                return;
            }
            
            $btn.prop('disabled', true).text(themisdbTaxonomy.i18n.processing || 'Processing...');
            
            $.ajax({
                url: themisdbTaxonomy.ajaxurl,
                type: 'POST',
                data: {
                    action: 'themisdb_consolidate_categories',
                    nonce: themisdbTaxonomy.nonce
                },
                success: function(response) {
                    if (response.success) {
                        var message = 'Merged ' + response.data.total_merged + ' categories';
                        if (response.data.details && response.data.details.length > 0) {
                            message += ':<br>';
                            response.data.details.forEach(function(detail) {
                                message += '<br>• Merged "' + detail.merged + '" into "' + detail.kept + '" (' + detail.posts_moved + ' posts)';
                            });
                        }
                        showResult(message, 'success');
                        
                        // Reload page after 3 seconds
                        setTimeout(function() {
                            location.reload();
                        }, 3000);
                    } else {
                        showResult('Error: ' + (response.data.message || 'Unknown error'), 'error');
                        $btn.prop('disabled', false).text(themisdbTaxonomy.i18n.autoMerge || 'Auto Merge');
                    }
                },
                error: function() {
                    showResult('Error: AJAX request failed', 'error');
                    $btn.prop('disabled', false).text(themisdbTaxonomy.i18n.autoMerge || 'Auto Merge');
                }
            });
        });
        
        // Individual merge buttons
        $('.btn-merge').on('click', function() {
            var $btn = $(this);
            var term1 = $btn.data('term1');
            var term2 = $btn.data('term2');
            
            if (!confirm(themisdbTaxonomy.i18n.confirmMerge || 'Merge these categories? This cannot be undone.')) {
                return;
            }
            
            $btn.prop('disabled', true).text(themisdbTaxonomy.i18n.merging || 'Merging...');
            
            $.ajax({
                url: themisdbTaxonomy.ajaxurl,
                type: 'POST',
                data: {
                    action: 'themisdb_merge_terms',
                    nonce: themisdbTaxonomy.nonce,
                    term1_id: term1,
                    term2_id: term2
                },
                success: function(response) {
                    if (response.success) {
                        showResult(response.data.message, 'success');
                        // Fade out the row
                        $btn.closest('tr').fadeOut(400, function() {
                            $(this).remove();
                            
                            // Check if table is now empty
                            var $tbody = $btn.closest('tbody');
                            if ($tbody.find('tr:visible').length === 0) {
                                $tbody.closest('table').prev('h2').after('<p>No consolidation suggestions. Your categories are well-organized!</p>');
                                $tbody.closest('table').remove();
                            }
                        });
                    } else {
                        showResult('Error: ' + response.data.message, 'error');
                        $btn.prop('disabled', false).text(themisdbTaxonomy.i18n.merge || 'Merge');
                    }
                },
                error: function() {
                    showResult('Error: AJAX request failed', 'error');
                    $btn.prop('disabled', false).text(themisdbTaxonomy.i18n.merge || 'Merge');
                }
            });
        });
    }
    
    // Initialize when DOM is ready
    $(document).ready(function() {
        initAnalytics();
    });
    
})(jQuery);
