// $Id: icl_translate.js,v 1.1 2010/06/30 08:22:07 brucepearson Exp $

function iclAjaxAutoSave(form, time) {
  setInterval(function () {
        iclAjaxSubmit(form);
    }, time);
}

function iclAjaxSubmit(form) {
    $(form).ajaxSubmit({
      //target:     target, 
      //url:        'comment.php',
      type: "POST",
      data: ({icl_ajax:1}),
      success: function() { 
        //alert('Thanks for your comment!'); 
      }
    });
};

                	