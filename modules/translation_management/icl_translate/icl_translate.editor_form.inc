<?php
// $Id: icl_translate.editor_form.inc,v 1.1 2010/06/30 08:22:07 brucepearson Exp $

module_load_include('inc', 'icl_translate', 'icl_translate.translator');


function icl_translate_editor() {
  _icl_translate_add_scripts();
  drupal_add_css(drupal_get_path('module', 'icl_translate') . '/css/icl_translate_editor.css');
  drupal_add_js('misc/jquery.form.js');
  drupal_add_js(drupal_get_path('module', 'icl_translate') . '/js/icl_translate_editor.js');
  
  $rid = arg(1);
  global $user;
  
  // Get origin and target languages
  // @todo find better function for getting lang
  if ($core_status = _icl_translate_get_core_status($rid)) {
    $original_lang = db_fetch_array(db_query("SELECT * FROM {icl_languages} WHERE code='%s'", $core_status['origin']));
    $target_lang = db_fetch_array(db_query("SELECT * FROM {icl_languages} WHERE code='%s'", $core_status['target']));
  }
  
  if (!_icl_translate_can_translate($rid, TRUE)) {
    $form = array();
    // Display access message
    $form['message'] = array('#value' => t('Sorry, you don\'t have enough permissions to edit this translation'));
    return $form;
  }
  
  _icl_translate_set_to_in_progress($rid);
  
  if ($fields = _icl_translate_get_fields($rid)) {
    
    // Fetch previous data here, we'll need it anyway
    if ($fields[0]['previous_id']) {
      $previous_data = _icl_translate_get_fields($fields[0]['previous_id']);
    }
    
    // Start setting form
    $form = array();
    
    // Set autosaves and determine type
    $autosaves = array();
    foreach ($fields as $field) {
      if (strpos($field['field_type'], '-icl-autosave')) {
        $autosaves[$field['field_type']] = $field;
      }
      else if (!$type) {
        $type = _icl_translate_editor_get_id($field['field_type'], unserialize($field['field_data']));
      }
    }
    
    // Basic info
    $form['info'] = array(
      '#value' => '<p>' . t('You are translating !info from !origin to !target', array('!info' => _icl_translate_editor_get_title($type['type'], $type['id']), '!origin' => $original_lang['icl_name'], '!target' => $target_lang['icl_name'])) . '</p>');
    
    // Form redirection
    $form['#redirect'] = array('translator' , 'finished=' . $type['id'] . '&type=' . $type['type']);
    
    if ($type['type'] == 'node') {
        $format = _icl_translate_get_wysiwyg_support($type['id']);
      }
    
    foreach ($fields as $field) {
      
      // Skip untranslatable fields
      if (!$field['field_translate']) {
        continue;
      }
      
      // Set element name
      if (strpos($field['field_type'], 'CCK[') !== false) {
        $name = explode('[', $field['field_type']);
        $name = trim($name[1], ']');
        $name = $rid . '[CCK][' . $name . ']';
      }
      else {
        $name = $rid . '[' . $field['field_type'] . ']';
      }
      
      // Set field original data
      $field_data = _icl_translate_editor_sanitize_serialized_data($field['field_data']);
      if (empty($field_data)) {
        $field_data = '';
      }
      
      // Set field original data
      $field_data_translated = _icl_translate_editor_sanitize_serialized_data($field['field_data_translated']);
      if (!empty($field_data_translated)) {
        // Set 'this translation is finished' checkbox
        $finished = 0;
      } else { // Use original data if translation is not set
        $field_data_translated = $field_data;
        // Set 'this translation is finished' checkbox
        $finished = 0;
      }
      
      // Set fieldset holder
      $form[$field['field_type']] = array (
        '#type' => 'fieldset', 
        '#title' => strtoupper(str_replace('[', ' [', t($field['field_type']))),
        '#collapsible' => true
        );
      
      // Check if doesn't need update, compare previous requests
      if ($field['previous_id']) {
        if ($finished = _icl_editor_translate_compare_to_previous($field['field_type'], $field_data, $previous_data)) {
          $form[$field['field_type']]['1_finished_message'] = array ('#value' => '<div>' . t('This content was translated previously and the original has not changed since then.' . '</div>'));
        }
      }
      
      // Display autosave toggle if necessary
      $autosave_data = '';
      if (isset($autosaves[$field['field_type'] . '-icl-autosave'])) {
        
        $autosave_data = unserialize($autosaves[$field['field_type'] . '-icl-autosave']['field_data_translated']);
        
        if (is_array($autosave_data)) {
          $autosave_data = implode(', ', array_values($autosave_data));
        }
        // Check if not empty, compare timestamp and content
        if (!empty($autosave_data)
            && $autosaves[$field['field_type'] . '-icl-autosave']['timestamp'] > $field['timestamp']
            && !_icl_translate_editor_compare_data($autosave_data, $field_data_translated)) {
          
          $form[$field['field_type']]['autosave'] = array(
            '#value' => '<div class="icl_translate_editor_load_autosave">'
            . t('More recent autosave available (click for toggle replace)')
            . '<div class="icl_translate_hidden">' . $autosave_data . '</div></div>'
          );
        }
      }
      
      // Set 'this translation is finished' checkbox
      $form[$field['field_type']]['finished'] = array (
        '#type' => 'checkbox', 
        '#title' => t('This translation is finished'),
        '#attributes' => array('class' => 'finished'),
        '#name' => $name . '[field_finished]',
        '#default_value' => $finished
      );
      
      // Set translation content field
      $lenght = strlen(strip_tags($field_data_translated));
      $rows = round($lenght/60) + 2;
      $form[$field['field_type']]['field_data_translated']['field_data_translated'] = array(
        '#title' => sprintf(t('Translated content - %s'), $target_lang['icl_name']),
        '#name'=> $name . '[field_data_translated]',
        '#type' => 'textarea',
        '#default_value' => $field_data_translated,
        '#rows' => $rows,
      );
      
      if ($type['type'] == 'node' && $field['field_type'] == 'body') {
        $form[$field['field_type']]['field_data_translated']['format'] = filter_form($format, NULL, array($field['field_type'], 'field_data_translated_format'));
      }
      
      // Set original content field
      $form[$field['field_type']]['field_data'] = array(
        '#value' => '<div class="icl_translate_hidden original_data">'
        . sprintf(t('Original content - %s:'), $original_lang['icl_name'])
        . '<br /><div class="field_data">' . $field_data . '</div></div>'
      );
    }
    
    // Set hidden rid field
    $form['rid']= array(
      '#name'=> 'rid',
      '#type' => 'hidden',
      '#default_value' => $rid
    );
    
    // Set submit button
    $form['action'] = array(
      '#type' => 'submit',
      '#submit' => array('icl_translate_editor_submit'),
      '#value' => t('Save translation'),
      '#attributes' => array('disabled' => 'disabled')
    );
    
    // Set cancel button
    $form['cancel'] = array(
      '#type' => 'button',
      '#button_type' => 'button',
      '#submit' => FALSE,
      '#value' => t('Cancel'),
      '#name' => 'cancel',
      '#attributes' => array('onClick' => 'javascript: if(confirm(\'' . t('Your translation will be lost') . '\')) { window.location = "/translator/"; return false;  } else { return false; }')
    );
    
    return $form;
  }
}

function icl_translate_editor_submit($form, &$form_state) { 
  if (isset($_POST['rid'])) {
    $rid = $_POST['rid'];
  }
  if ($rid && _icl_translate_can_translate($rid)) {
    $rid = $_POST['rid'];
    
    // Delete autosave
    _icl_translate_editor_autosave_delete($rid);
    
    // Process fields
    foreach ($_POST[$rid] as $type => $data) {
      
      // Process CCK
      if ($type == 'CCK') {
        
        foreach ($data as $cck => $cck_data) {
          $cck_update = explode(',', $cck_data['field_data_translated']);
          
          foreach ($cck_update as $cck_update_key => $cck_update_value) {
            $cck_update[$cck_update_key] = trim($cck_update_value);
          }
          
          // Autosave
          if (isset($_REQUEST['icl_ajax'])) {
            _icl_translate_editor_autosave($rid, 'CCK[' . $cck . ']', $cck_update);
          }
          // Regular
          else {
            _icl_translate_update_field_translation($rid, 'CCK[' . $cck . ']', $cck_update);
          }
        }
        continue;
      }
      
      // Process other
      // Autosave
      if (isset($_REQUEST['icl_ajax'])) {
        _icl_translate_editor_autosave($rid, $type, $data['field_data_translated']);
      }
      // Regular
      else {
      _icl_translate_update_field_translation($rid, $type, $data['field_data_translated']);
      }
    }
    
    // Set job as finished
    if (!isset($_REQUEST['icl_ajax'])) {
      _icl_translate_finish_job($rid);
    }
  }
  
  // If AJAX autosave die()
  if (isset($_REQUEST['icl_ajax'])) {
    die();
  }
}








 /**
 * Autosave insert.
 */
function _icl_translate_editor_autosave($rid, $type, $data) {
  $data = array(
    'rid' => $rid,
    'timestamp' => time(),
    'field_type' => $type . '-icl-autosave',
    'field_data_translated' => serialize($data),
  );
  drupal_write_record('icl_translate', $data);
}

 /**
 * Autosave delete.
 */
function _icl_translate_editor_autosave_delete($rid) {
  db_query("DELETE FROM {icl_translate} WHERE field_type LIKE '%-icl-autosave%' AND rid=%d", $rid);
}

 /**
 * Determine type.
 * @todo Remove
 */
function _icl_translate_editor_get_id($field_type, $data) {
  $types = array('nid' => 'node', 'bid' => 'block');
  foreach ($types as $type_id => $type) {
    if ($field_type == $type_id) {
      return array('type' => $type, 'type_id' => $type_id, 'id'=> $data);
    }
  }
  return FALSE;
}

 /**
 * Process serialized data.
 */
function _icl_translate_editor_sanitize_serialized_data($data) {
  $data = unserialize($data);
  if (!empty($data)) {
    if (is_array($data)) {
      $data = implode(', ', array_values($data));
    }
  }
  return $data;
}

 /**
 * Compare previous version to current field.
 *
 * @param string $type
 *      Current field type (field_type)
 * @param string $data
 *      Current original field data (field_data)
 * @param array $previous
 *      Previous request data
 *
 * @return boolean
 */
function _icl_editor_translate_compare_to_previous($type, $data, $previous) {
  
  // Loop through previous fields
  foreach ($previous as $previous_field) {
    if ($previous_field['field_type'] == $type) {
      $data2 = _icl_translate_editor_sanitize_serialized_data($previous_field['field_data']);
      return _icl_translate_editor_compare_data($data2, $data);
    }
  }
  
  return FALSE;
}

 /**
 * Compare serialized data.
 */
function _icl_translate_editor_compare_data($data, $data2) {
  return ereg_replace("[\r\t\n\v]", "", trim($data)) === ereg_replace("[\r\t\n\v]", "", trim($data2));
}

function _icl_translate_get_wysiwyg_support($nid) {
  $format = db_result(db_query("SELECT format FROM {node_revisions} WHERE nid=%d", $nid));
  return $format;
}

function _icl_translate_editor_get_title($type, $id) {
  switch ($type) {
    
    case 'node':
      return '<a href="/?q=node/' . $id . '">' . db_result(db_query("SELECT title FROM {node_revisions} WHERE nid=%d", $id)) . '</a>';
      break;
    
    case 'block':
      return '<strong>' . db_result(db_query("SELECT title FROM {blocks} WHERE bid=%d", $id)) . '</strong> block';
      break;
    
    case 'contact_form':
      return t('Contact form');
      break;
  }
}
