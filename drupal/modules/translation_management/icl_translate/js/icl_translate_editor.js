// $Id: icl_translate_editor.js,v 1.1 2010/06/30 08:22:07 brucepearson Exp $

$(document).ready(function() {
  
  // Bind focus to fields
  // Show/hide original content
  $('textarea, input[id^=edit-field-data-translated]').bind('focus', function() {
    var hide = $(this).parents('fieldset').find('.original_data');
    if (hide.is(':hidden')) {
      $('.original_data:visible').slideUp();
      hide.slideDown();
    }
  });
  
  // Check if all translated
  // Focus to first untranslated field
  iclTranslateEditorCheckFinished();
  
  // Sets autosave interval
  iclAjaxAutoSave('#icl-translate-editor', 60000); // 60 sec.

  // Binds click to 'finished'
  // Submits AJAX autosave
  // Focuses and scrolls to next/first untranslated field
  $('#icl-translate-editor .finished').click(function() {
    iclAjaxSubmit('#icl-translate-editor');
    
    // If unchecked focus to it's field
    if ($(this).is(':checked') == false) {
       $(this).parents('fieldset').find('[id^=edit-field-data-translated]').focus();
       $('#edit-action').attr('disabled', 1);
    // Else find next field
    } else {
      var fieldset = $(this).parents('fieldset').next('fieldset');
      var finished = fieldset.find('.finished');
      
      // If next fieldset not found check all/find first untranslated field
      if (finished.length == 0) {
        iclTranslateEditorCheckFinished();
      }
      // If next not marked as translated focus
      else if (finished.is(':checked') == false) {
        iclTranslateFocus(fieldset.find('[id^=edit-field-data-translated]'));
        $('#edit-action').attr('disabled', 1);
      // Check other fields
      } else {
        iclTranslateEditorCheckFinished();
      }
    }
  });
  
  
  
  // Replaces translated conent with autosaved content
  $('.icl_translate_editor_load_autosave').click(function() {
    var toLoad = $(this).children('div');
    var toLoadData = toLoad.html();
    var toReplace = $(this).parent().children().find('[id^=edit-field-data-translated]');
    var toReplaceData = toReplace.val();
    toReplace.val('...');
    var timerCallback = function() {
      toReplace.val(toLoadData);
      toLoad.html(toReplaceData);
    };
    setTimeout(timerCallback, 200);
  });
  
  // Disable format change
  $('[id^=edit-body-field-data-translated-format]').find('input').each(function() {
       $(this).attr('disabled', 'disabled');
      $(this).unbind('click');
      if ($(this).is(':checked') == false) {
        $(this).parent().parent().css('display','none');
      }
    }
  );
  
});

 /**
 * Checks if all translations are finished
 * Enables submit button
 * Focuses/scrolls to first untranslated field
 */
function iclTranslateEditorCheckFinished() {
  $('#icl-translate-editor .finished').each(function(e) {
    if ($(this).is(':checked') == false) {
      $('#edit-action').attr('disabled', 1);
      iclTranslateFocus($(this).parents('fieldset').find('[id^=edit-field-data-translated]'));
      return false;
    } else {
      $('#edit-action').attr('disabled', 0);
    }
  });
}

/**
 * Focuses and scrolls to field
 */
function iclTranslateFocus(focusTo) {
  var x = focusTo.offset().top - 100; // 100 provides buffer in viewport
  $('html,body').animate({scrollTop: x}, 500);
  focusTo.focus();
}