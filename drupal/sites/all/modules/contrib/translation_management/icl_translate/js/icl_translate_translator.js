// $Id: icl_translate_translator.js,v 1.1 2010/06/30 08:22:07 brucepearson Exp $

$(document).ready(function(){
  $('.icl_translate_translator_language_show').click(function(){
    if ($(this).attr('checked')) {
      $(this).parent().parent().next('.icl_translate_language_pairs').slideDown();
    } else {
      var hide;
      $(this).parent().parent().next('.icl_translate_language_pairs').slideUp();
      $(this).parent().parent().next('.icl_translate_language_pairs').children('.form-item').children('.option').children('input').attr('checked', 0);
    }
  });
  
  $('.icl_assign_translator').change(icl_set_translator_for_job);
  $('.icl_assign_translator_cancel').click(icl_set_translator_for_job_cancel);
  $('.icl_assign_translator_save').click(icl_set_translator_for_job_save);
  
  
});


function icl_set_translator_for_job(){
    var rid = $(this).attr('id').replace(/^icl_assign_translator_rid_/,'');
    $('#icl_assign_translator_buttons_'+rid).show();
}

function icl_set_translator_for_job_cancel(){
    var rid = $(this).attr('id').replace(/^icl_assign_translator_cancel_/,'');
    $('#icl_assign_translator_buttons_'+rid).hide();
    $('#icl_assign_translator_rid_'+rid).val('0');  
}

function icl_set_translator_for_job_save(){    
    var rid = $(this).attr('id').replace(/^icl_assign_translator_for_/,'');
    $('#icl_assign_translator_cancel_'+rid+', #icl_assign_translator_for_'+rid).attr('disabled', 'disabled');
    var uid = $('#icl_assign_translator_rid_'+rid).val();  
    var data = {icl_translator_ajx_action:'set_translator', uid:uid, rid:rid};
    jQuery.ajax({
        type: "POST",
        url: location.pathname,
        data: data,
        cache: false,
        dataType: 'json',
        success: function(msg){
            if(!msg.error){                
                $('#icl_assign_translator_cancel_'+rid+', #icl_assign_translator_for_'+rid).removeAttr('disabled', 'disabled').fadeOut();
            }            
        }
    }); 
    
}
