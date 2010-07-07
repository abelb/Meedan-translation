<?php 

module_load_include('inc', 'icl_translate', 'icl_translate.translator');

function icl_translator_manage_translators_form_cb(){
  return drupal_get_form('icl_translator_manage_translators_form');
}

function theme_icl_translator_translators_form($form) {
  $header = array(t('Name'), t('Languages'), t('Edit'));
  $rows = array();
  foreach (element_children($form['name']) as $key) {
    $row = array();
    $row[] = drupal_render($form['name'][$key]);
    $row[] = drupal_render($form['langs'][$key]);
    $row[] = drupal_render($form['edit'][$key]);
    $rows[] = $row;
  }
    
  $output = theme('table', $header, $rows);

  return $output;    
}

function icl_translator_manage_translators_form(){
  
  $form['add'] = array(
      '#type' => 'markup',
      '#value' => t('Translators are special Drupal users who can translate between language pairs that you choose.
                    <br />
                    <br />
                    You can give translator privileges to existing users by clicking on <a href="@url">add translator</a>.
                    If your translators aren\'t yet a Drupal user, you must first <a href="@create_url">create new Drupal users</a> for them.',
                    array('@url' => url('admin/content/translation-management/add-translator'),
                          '@create_url' => url('admin/user/user/create'))),
  );
  
  $form['users'] = array(
    'name' => array(),
    'langs' => array(),
    'edit' => array(),
    '#theme' => 'icl_translator_translators_form',
  );

  $translators = _icl_translate_get_translators();
  $languages_list = language_list('enabled');
  $languages_list = $languages_list[1];
  
  foreach ($translators as $trans) {
    $form['users']['name'][] = array('#value' => $trans['name']);
    
    $pairs = _icl_translate_get_translator_pairs($trans['uid']);
    $langs = '';
    foreach ($pairs as $from => $to_list) {
      foreach ($to_list as $to => $temp) {
        if ($langs != '') {
          $langs .= '<br />';
        }
        $langs .= $languages_list[$from]->name . t(' to ') . $languages_list[$to]->name;
      }
    }
    
    $form['users']['langs'][] = array('#value' => $langs);
    $form['users']['edit'][] = array('#value' =>  l(t('edit languages'), 'admin/content/translation-management/translator/' . $trans['uid'] . '/edit'));
  }
  

  return $form;
    
}