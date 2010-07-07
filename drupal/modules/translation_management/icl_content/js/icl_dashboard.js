var element_count;

jQuery(document).ready( function() {
        $().ajaxComplete(function(ev, xhr, s) {
                // Show any replaced data returned by the ajax call
                $('div[id^=result-]').each(function(){
                        var temp = $(this).html();
                        if($(this).html() != "<!--- This will be replaced --->"){
                                $(this).parent().parent().show();
                        }
                        
                })
        });
        icl_tb_init('.icl_thickbox');
        icl_tb_set_size('.icl_thickbox');
        
        $.get(Drupal.settings.ican_fetch.ican_url);
        
        element_count = jQuery('*').length;
        
        setInterval(check_new_elements, 1000);
        
        $('#icl_menu_dismiss_all').click(icl_menu_dismiss_all);

        $('a[id^=icl_menu_dismiss-]').click(icl_menu_dismiss_type);
        
        // align the selectors
        x = $('#edit-search-text').offset().left;
        x = Math.max(x, $('#edit-status-status').offset().left);
        x = Math.max(x, $('#edit-type-type').offset().left);

        $('#edit-search-text').css({"margin-left": x - $('#edit-search-text').offset().left});
        $('#edit-status-status').css({"margin-left": x - $('#edit-status-status').offset().left});
        $('#edit-type-type').css({"margin-left": x - $('#edit-type-type').offset().left});

        x = $('#edit-language').offset().left;
        x = Math.max(x, $('#edit-to').offset().left);
        x = Math.max(x, $('#edit-translation').offset().left);
        $('#edit-language').css({'margin-left' : x - $('#edit-language').offset().left});
        $('#edit-to').css({'margin-left' : x - $('#edit-to').offset().left});
        $('#edit-translation').css({'margin-left' : x - $('#edit-translation').offset().left});

     
});

function check_new_elements() {

        if (element_count != jQuery('*').length) {
                element_count = jQuery('*').length;
                icl_tb_init('.icl_thickbox');
                icl_tb_set_size('.icl_thickbox');
                
        }
        
}

function icl_menu_dismiss_all() {
        $('#icl_menu_warning').hide();

        jQuery.ajax({
                type: "POST",
                url: Drupal.settings.ican_ajax.ican_dismiss_warning_url,
                data: "command=dismiss_all",
                async: true,
                success: function(msg){
                }
        });
}

function icl_menu_dismiss_type() {
        node_type = $(this).attr('id').substring(17);
        $('#icl_row-' + node_type).remove();

        jQuery.ajax({
                type: "POST",
                url: Drupal.settings.ican_ajax.ican_dismiss_warning_url,
                data: "command=dismiss_type&type=" + node_type,
                async: true,
                success: function(msg){
                }
        });

        if ($('tr[id^=icl_row-').length == 0) {
                $('#icl_menu_warning').hide();
        }
}
