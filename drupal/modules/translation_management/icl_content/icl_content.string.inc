<?php
/*
 * $Id: icl_content.string.inc
 * 
 * @file ICanLocalize string handling
 */

module_load_include ( 'inc', 'icl_core', 'icl_core.module' );
module_load_include ( 'inc', 'icl_content', 'icl_content.dashboard' );

// String translation using ICanLocalize server

/*
 $strings - an array of strings to be sent for translation
 array('string_id' => 111, 'string_type' => 'locale')
 
 $target_languages - an array of languages to translate to
*/
function icl_content_send_strings($strings, $target_languages) {
    // send to each language
    foreach($target_languages as $target){
      $strings_waiting_for_translation = variable_get('icl_strings_for_translation', array());
      $strings_waiting_for_translation[$target][] = $strings;
  
      variable_set('icl_strings_for_translation', $strings_waiting_for_translation);
    }
}

function _icl_content_send_strings($strings, $target) {
    
    $source_lang = language_default()->language;
    
    $data = _icl_content_get_strings($strings, $target);

        if (sizeof($data) > 0) {
    
            $langs = icl_core_available_languages ();
            $langs_targets = _icl_core_available_targets_for_origin ( $source_lang );
    
            $rids_sent = icl_core_send_content_for_translation('icl_content',
                                                  $data,
                                                  $langs[$source_lang],
                                                  array($langs_targets[$target]),
                                                  null,
                                                  "",
                                                  "String translations"
                                                  );
            
            
            foreach ($rids_sent as $rid) {
              if ($rid != 0) {
                foreach($string_values as $st_id => $st){
                    db_query ( "INSERT INTO {icl_string_status} VALUES(%d, '%s', %d, %d, '%s')", $rid, $st['type'], $st_id, time (), md5($st['value']) );
                }
              }
            }
        }
}

function icl_decode_translation_status_id($status){
    switch($status){
        case CMS_TARGET_LANGUAGE_CREATED: $st = t('Waiting for translator');break;
        case CMS_TARGET_LANGUAGE_ASSIGNED: $st = t('In progress');break; 
        case CMS_TARGET_LANGUAGE_TRANSLATED: $st = t('Translation received');break;
        case CMS_TARGET_LANGUAGE_DONE:
        case ICL_STATUS_SUCCESSFUL:
          $st = t('Translation complete');break;
        case ICL_STATUS_FAILED: $st = t('Request failed');break;
        default: $st = t('Not translated');
    }
    
    return $st;
}


function icl_content_get_string_translation_status($string, $decode = true) {
  $sql = "
          SELECT
              cs.status , cs.target
          FROM
              {icl_core_status} cs
          JOIN 
             {icl_string_status} ss
          ON
             ss.rid = cs.rid
          WHERE
              ss.string_id=%d
              AND
              ss.string_type='%s'
          ";   

  $query = db_query($sql, $string['string_id'], $string['string_type']);
  while ( $item = db_fetch_object($query)) {
    if ($decode) {
      $status[$item->target] = icl_decode_translation_status_id($item->status);
    } else {
      $status[$item->target] = $item->status;
    }
  }

  return $status;
      
}


function icl_content_alter_locale_translate_seek_form(&$form, $form_state) {
    
  if (variable_get ( 'icl_core_website_id', '' ) != '' && variable_get ( 'icl_core_accesskey', '' ) != '') {
    
    drupal_add_js ( drupal_get_path ( 'module', 'icl_core' ) . '/js/icl_reminders.js' );
    
    drupal_add_js(array('text_group' => t('Text group')), 'setting');
    if (!module_exists('l10n_client')) {
      $ican_string_status_url = url('icl_content/icl_string_status');
      drupal_add_js(
                    array('ican_url' =>
                          array('ican_string_status_url' => $ican_string_status_url)),
                          'setting');
      
  
      drupal_add_js(drupal_get_path('module', 'icl_content') . '/js/icl_string.js');
    }
    
    $form['action'] = icl_content_dashboard_action_form($form, true);
    $form['action']['operations']['#title'] = t('ICanLocalize translation options');
    $form['action']['#weight'] = -10; // put at the top.
    
    $form['action']['icl_check'] = array(
      '#type' => 'checkbox',
      '#title' => '',
      '#attributes' => array('style' => "display:none;"),
    );
    
    if (isset($form['action']['operations']['translate_request'])) {
      $form['action']['operations']['translate_request']['#validate'] = array('icl_content_locale_translate_validate');
      $form['action']['operations']['translate_request']['#submit'] = array('icl_content_locale_translate_submit');
      $form['action']['operations']['translate_request']['#value'] = t('Translate selected strings');
    }
    
  }    
}

function icl_content_locale_translate_validate($form, &$form_state) {
}

function icl_content_locale_translate_submit($form, &$form_state) {
  $strings = array();
  if (isset($_POST['icl_lid'])) {
    foreach($_POST['icl_lid'] as $id => $state) {
      if ($state == '1') {
        $strings[] = array('string_id' => $id, 'string_type' => 'locale');
      }
    }
    
    $targets = array();
    foreach($form_state['values']['targets'] as $lang => $state) {
      if ($state == $lang) {
        $targets[] = $lang;
      }
    }
    if (sizeof($strings) > 0 && sizeof($targets) > 0) {
      icl_content_send_strings($strings, $targets);
    }
    
    icl_content_set_string_queue_message();
    
  }
}

function icl_content_string_translation($content, $request_id) {
  foreach ( $content as $code => $data ) {
    $strings = array();
    foreach($data as $item) {
      $parts = explode('-', $item['type']);
      $strings[$parts[1]][$parts[2]] = $item['data'];
    }
    
    foreach($strings as $id => $string) {
      if (strstr($id, 'locale') == 0) {
        $lid = substr($id, 6);
        icl_content_update_locale_translation($lid, $code, $string['value']);
      }
    }
  }
  
  return TRUE;
}

function icl_content_is_string_queued($id, $lang) {
  $strings_waiting_for_translation = variable_get('icl_strings_for_translation', array());
  
  if (isset($strings_waiting_for_translation[$lang])) {
    $strings = $strings_waiting_for_translation[$lang];
    foreach($strings as $string_group) {
      foreach($string_group as $string) {
        if ($string['string_id'] == $id) {
          return TRUE;
        }
        
      }
    }
    
    
    
  }
  
  return FALSE;
}
function icl_content_string_status() {

  global $language;
  
  if ($_POST['icl_ajx_action'] == 'icl_string_status') {
    
    $source_lang = language_default()->language;
    $langs_targets = _icl_core_available_targets_for_origin ( $source_lang );
    
    $strings = explode(',', $_POST['string_ids']);
    foreach ($strings as $st_id) {
      $string['string_id'] = $st_id;
      $string['string_type'] = 'locale';
      
      foreach($langs_targets as $target => $data) {
        if (icl_content_is_string_queued($st_id, $target)) {
          echo $st_id . ',' . $target . ',' . t('In queue') . '|';
        }
      }
      
      $status = icl_content_get_string_translation_status($string);
      if ($status) {
      
        foreach($status as $lang => $stat) {
          echo $st_id . ',' . $lang . ',' . $stat . '|';
        }
      }
    }
  } else if ($_POST['icl_ajx_action'] == 'icl_single_string_status') {
    $st_id = db_result(db_query("SELECT lid FROM {locales_source} WHERE source = '%s' AND textgroup = '%s'", $_POST['source'], 'default'));
    
    if (icl_content_is_string_queued($st_id, $language->language)) {
      echo 1 . '|' . t("In queue");
    } else {
      $string['string_id'] = $st_id;
      $string['string_type'] = 'locale';
      
      $status = icl_content_get_string_translation_status($string, false);
      
      if ($status) {
      
        foreach($status as $lang => $stat) {
          if ($lang == $language->language) {
            echo $stat . '|' . icl_decode_translation_status_id($stat);
          }
        }
      }
    }
  } else if ($_POST['icl_ajx_action'] == 'icl_any_queued_strings') {
    $strings_waiting_for_translation = variable_get('icl_strings_for_translation', array());
    if (sizeof($strings_waiting_for_translation) > 0) {
      echo '1';
    } else {
      echo '0';
    }
    
  }  
  
}

function icl_content_string_send() {
  global $language;
  
  $string = $_POST['string'];
  $st_id = db_result(db_query("SELECT lid FROM {locales_source} WHERE source = '%s' AND textgroup = '%s'", $string, 'default'));

  if (!$st_id) {
    db_query("INSERT INTO {locales_source} (location, source, textgroup) VALUES ('%s', '%s', '%s')", NULL, $string, 'default');
    $st_id = db_result(db_query("SELECT lid FROM {locales_source} WHERE source = '%s' AND textgroup = '%s'", $source, 'default'));
  }
  
  if ($st_id === false) {
    echo '0|' . t('The source was not found.');
  } else {
    $strings[] = array('string_id' => $st_id, 'string_type' => 'locale');
    
    $targets = array($language->language);
    icl_content_send_strings($strings, $targets);
    
    echo '1|' . $st_id;
  }
}

function theme_icl_content_queued_strings($form) {
  drupal_add_css(drupal_get_path('module', 'icl_content').'/css/dashboard.css');
  
  $select_header = theme('table_select_header_cell', array());
  $header = array($select_header, t('String'), t('Target'));
  
  foreach (element_children($form['id']) as $key) {
    $row = array();      
    $row[] = drupal_render($form['strings'][$key]);
    $row[] = drupal_render($form['string'][$key]);
    $row[] = drupal_render($form['target'][$key]);
    $rows[] = $row;
  }
  
  $output = theme('table', $header, $rows);
  
  return $output;
}

function icl_content_string_form_table() {
   $form = array(
    'id' => array(),
    'source' => array(),
    'string' => array(),
    'target' => array(),
  );
  $form['#theme'] = 'icl_content_queued_strings';
  $strings = array();
  $source_lang = language_default()->language;
  $langs_targets = _icl_core_available_targets_for_origin ( $source_lang );

  $strings_waiting_for_translation = variable_get('icl_strings_for_translation', array());
  foreach ($strings_waiting_for_translation as $target => $st_item) {
    foreach ($st_item as $st) {
      foreach ($st as $st_data) {
        $id = 'locale-' . $st_data['string_id'] . '-' . $target;
        $form['id'][$id] = array('#value' => $st_data['string_id']);
        
        if ($st_data['string_type'] == 'locale') {
          $string_source = db_result(db_query("SELECT source FROM {locales_source} WHERE lid=%d", $st_data['string_id']));
          
          $form['string'][$id] = array('#value' => $string_source);
          $strings[$id] = $id;
        }
        
        $form['source'][$id] = array('#value' => $st_data['string_type']);
        $form['target'][$id] = array('#value' => $langs_targets[$target]->name);
        
      }
    }
  }
  
  $form['strings'] = array('#type' => 'checkboxes', '#options' => $strings, '#default_value' => array_keys($strings));
  return $form;
}

function icl_content_send_queued_strings() {
 
  drupal_add_js ( drupal_get_path ( 'module', 'icl_core' ) . '/js/icl_reminders.js' );
  
  $form['icl_support'] =  array(
    '#type' => 'markup',
    '#prefix' => '<div class="icl_cyan_box">',
    '#suffix' => '</div>',
    '#value' => icl_core_get_icl_support(),
  );

  
  $form['string_table'] = icl_content_string_form_table();
  
  if (sizeof(variable_get('icl_strings_for_translation', array())) == 0) {
    $form['no_strings'] = array (
          '#type' => 'markup', 
          '#prefix' => '<div class="status"><b>', 
          '#suffix' => '</b></div>', 
          '#value' => t('There are no strings queued for translation') );
  
    
    
  } else {
    
    $form['action'] = icl_content_dashboard_action_form($form, true);
    unset($form['action']['operations']['targets']);
    $form['action']['operations']['#title'] = t('ICanLocalize translation options');
    
    
    if (isset($form['action']['operations']['translate_request'])) {
      $form['action']['operations']['translate_request']['#validate'] = array('icl_content_send_queued_strings_validate');
      $form['action']['operations']['translate_request']['#submit'] = array('icl_content_send_queued_strings_submit');
      $form['action']['operations']['translate_request']['#value'] = t('Send selected strings for translation');
      $form['action']['operations']['translate_request']['#weight'] = -2;
      
      $form['action']['operations']['cancel_request'] = array(
        '#type' => 'submit',
        '#validate' => array('icl_content_send_queued_strings_validate'),
        '#submit' => array('icl_content_cancel_queued_strings_submit'),
        '#value' => t('Cancel selected strings'),
        '#weight' => -1,
        );
      
    }
  }   
  
  return $form;
}

function icl_content_send_queued_strings_validate($form, &$form_state) {
}

function _icl_content_string_get_parts($id) {
  $parts = explode('-', $id);
  if (sizeof($parts) > 3) {
    // Combine the language code parts that have been split.
    // eg. pt-pt, pt-br, zh-hans, zh-hant
    $parts[2] .= '-' . $parts[3];
  }
  
  return $parts;
    
}
function icl_content_send_queued_strings_submit($form, &$form_state) {
  $strings = array();
  if (isset($_POST['strings'])) {

    $strings_waiting_for_translation = variable_get('icl_strings_for_translation', array());
    
    foreach($_POST['strings'] as $id => $state) {
      $parts = _icl_content_string_get_parts($id);

      $strings[$parts[2]][] = array('string_id' => $parts[1], 'string_type' => $parts[0]);
      
      // remove from the waiting list.
      foreach ($strings_waiting_for_translation as $target => $st_item) {
        if ($target == $parts[2]) {
          foreach ($st_item as $index_1 => $st) {
            foreach ($st as $index_2 => $st_data) {
              if($st_data['string_type'] == $parts[0] && $st_data['string_id'] == $parts[1]) {
                unset($strings_waiting_for_translation[$target][$index_1][$index_2]);
                break;
              }
            }            
          }
        }
      }
    }

    
    foreach ($strings as $target => $st) {
      _icl_content_send_strings($st, $target);
    }

    // save the strings waiting.
    _icl_content_save_waiting_strings($strings_waiting_for_translation);
   
  }
}

function _icl_content_save_waiting_strings($strings_waiting_for_translation) {
  foreach ($strings_waiting_for_translation as $target => $st_item) {
    foreach ($st_item as $index_1 => $st) {
      if (sizeof($strings_waiting_for_translation[$target][$index_1]) == 0) {
        unset($strings_waiting_for_translation[$target][$index_1]);
      }
      
    }

    if (sizeof($strings_waiting_for_translation[$target]) == 0) {
      unset($strings_waiting_for_translation[$target]);
    }
  }
  
  if (sizeof($strings_waiting_for_translation) == 0) {
    variable_del('icl_strings_for_translation');
  } else {
    variable_set('icl_strings_for_translation', $strings_waiting_for_translation);
  }
}

function icl_content_cancel_queued_strings_submit($form, &$form_state) {
  if (isset($_POST['strings'])) {
    
    // remove the selected strings for the waiting list.

    $strings_waiting_for_translation = variable_get('icl_strings_for_translation', array());
    
    foreach($_POST['strings'] as $id => $state) {
      $parts = _icl_content_string_get_parts($id);

      // remove from the waiting list.
      foreach ($strings_waiting_for_translation as $target => $st_item) {
        if ($target == $parts[2]) {
          foreach ($st_item as $index_1 => $st) {
            foreach ($st as $index_2 => $st_data) {
              if($st_data['string_type'] == $parts[0] && $st_data['string_id'] == $parts[1]) {
                unset($strings_waiting_for_translation[$target][$index_1][$index_2]);
                break;
              }
            }            
          }
        }
      }
    }

    // save the strings waiting.
    _icl_content_save_waiting_strings($strings_waiting_for_translation);
    
  }
}

function icl_content_set_string_queue_message() {
  $link = '<a href="' . url('icl_content/icl_send_queued_strings') . '">';
  
  // remove any existing message
  if (isset($_SESSION['messages']['status'])) {
    foreach($_SESSION['messages']['status'] as $key => $value) {
      if (strpos($value, t('queued for translation by ICanLocalize')) !== FALSE) {
        unset($_SESSION['messages']['status'][$key]);
      }
    }
    if (sizeof($_SESSION['messages']['status']) == 0) {
      unset($_SESSION['messages']['status']);
    }
  }
  
  if (strpos(request_uri(), 'icl_content/icl_send_queued_strings') === FALSE) {
    // Add message about waiting string translations.
    $strings_waiting_for_translation = variable_get('icl_strings_for_translation', '');
    
    if ($strings_waiting_for_translation != '') {
      $count = 0;
      foreach ($strings_waiting_for_translation as $target => $st_item) {
        foreach ($st_item as $st) {
          $count += sizeof($st);
        }
      }
      
      if ($count == 1) {
        $message = t('There is 1 string queued for translation by ICanLocalize.<br />!linkSend to translation or edit queue</a>', array('!link' => $link));
      } else {
        $message = t('There are !count strings queued for translation by ICanLocalize.<br />!linkSend to translation or edit queue</a>', array('!count' => $count, '!link' => $link));
      }
  
      drupal_set_message($message, 'status', FALSE);
    }
  }
}

function _icl_content_get_strings($strings, $target) {
  
  // get all the untranslated strings
  $untranslated = array();
  foreach($strings as $st) {
    $st_id = $st['string_id'];
    if ($st['string_type'] == 'locale') {
      if (db_result(db_query("SELECT COUNT(lid) FROM {locales_target} WHERE lid=%d AND language='%s'", $st_id, $target)) == 0) {
    // make sure it's not in progress.
        $sql = "
                  SELECT
                      cs.rid
                  FROM
                      {icl_core_status} cs
                  JOIN 
                     {icl_string_status} ss
                  ON
                     ss.rid = cs.rid
                  WHERE
                      ss.string_id=%d
                      AND
                      ss.string_type='locale'
                      AND
                      cs.target='%s'
                  ";
        $cms_rid = db_result(db_query($sql, $st_id, $target));
        
        if ($cms_rid === false) {
          $untranslated[] = $st;
        }
      }
       } else {
      $untranslated[] = $st;
    }
  }
  
  $data = array();
  
  if (sizeof($untranslated) >  0) {
    // Something to translate.
    $string_values = array();
    
    foreach($untranslated as $st) {
      $st_id = $st['string_id'];
      if ($st['string_type'] == 'locale') {
        $string = db_fetch_object(db_query("SELECT location, source FROM {locales_source} WHERE lid={$st_id}"));
      }
      if ($string) {
        $string_values[$st_id] = array(
          'value' => $string->source,
          'type' => $st['string_type']
          );
        $data[] = array(
          'translate'=>0,
          'text'=>$string->location,
          'format'=>'base64',
          'type'=>'string-'.$st['string_type'].$st_id.'-name'
          );
        $data[] = array(
          'translate'=>1,
          'text'=>$string->source,
          'format'=>'base64',
          'type'=>'string-'.$st['string_type'].$st_id.'-value'
          );
      }
    }
  }
  
  return $data;
}
