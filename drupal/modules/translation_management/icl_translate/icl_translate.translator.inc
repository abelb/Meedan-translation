<?php
// $Id: icl_translate.translator.inc,v 1.1 2010/06/30 08:22:07 brucepearson Exp $

function icl_translate_translator_jobs_form_submit($form_id, $form_values) {    
    $_SESSION['icl_translator_jobs_filter'] = $form_id['filterby']['#post'];    
}
/**
 * Handles user jobs listing urls.
 * @todo 
 * 
 * @param int $uid 
 *      If not supplied current user is checked.
 */
function icl_translate_translator_jobs($uid = null) {
  drupal_add_css(drupal_get_path('module', 'icl_translate') . '/css/icl_translate.css');
  drupal_add_js(drupal_get_path('module', 'icl_translate') . '/js/icl_translate_translator.js');
  
  $params_to_pass = array('translator', 'status', 'from', 'to');
  
  if(isset($_SESSION['icl_translator_jobs_filter'])){
      $post = $_SESSION['icl_translator_jobs_filter'];      
      unset($_SESSION['icl_translator_jobs_filter']);
  }
  
  foreach($params_to_pass as $k){
    if(isset($_GET[$k]) && $_GET[$k]) $post[$k] = $_GET[$k];    
  }
  
  
  if(is_null($uid)){
    if (user_access(ICL_TRANSLATE_MANAGE_JOBS)) {
      $account = false;
    } else {
      if (user_access(ICL_TRANSLATE_DO_JOBS)) {
  global $user;
  $uid = $user->uid;
      } else {
        drupal_set_message(t('You don\'t have permission to edit translation jobs.'), 'error');
  return false;
      }
    }
  }
  
  if(!is_null($uid)){
      $uid = intval($uid);
      if ($uid == 0){
        drupal_set_message(t('User ID must be an integer.'), 'error');    
        return false;
      }
      $account = user_load($uid);
      if (empty($account)) {
        drupal_set_message(t('User not found.'), 'error');  
        return false;
      }
      if (false == _icl_translate_is_translator($account)) {
        drupal_set_message(t('!name is not a translator.', array('!name'=>$account->name)) . ' ' . l(t('Configure translator.'), 'admin/translator/'.$uid.'/edit/'), 'error');  
        return false;
      }
  }
  
  $out = '';
  if($account){  
    $out .= t('Translation jobs for !username', array('!username'=>$account->name));    
  }      
  
  $jobs = _icl_translation_get_translator_jobs($uid, $post);  
  
  $out .= drupal_get_form('icl_translate_translator_jobs_form', array('account'=>$account, 'jobs'=>$jobs, 'post'=>$post));        
  
  if(!empty($jobs)){ 
    $language_list = language_list('enabled');   
    $header = array(t('Title'), t('Language'), '&nbsp;', t('Status'));
    if(!$uid){
        $header[] = t('Translator');
    }
    foreach($jobs as $j){
        $col = array('title'=>$j['title'], 'language'=>$language_list[1][$j['origin']]->name . ' &raquo; ' . $language_list[1][$j['target']]->name, $j['edit'], $j['status']);
        if(!$uid){
          if($j['status_code'] == ICL_STATUS_INQUEUE){
            $col['translator'] = _icl_translators_dropdown($j['rid'], $j['translator_uid'], $j['origin'], $j['target']);              
          }else{
            $col['translator'] = '<a href="' . url('admin/content/translation-management/translator/') . $j['translator_uid'] . '/edit">' . $j['translator_name'] . '</a>';    
          }  
          
        }
        $jobst[] = $col;
    }
    $out .= theme('table', $header, $jobst);    
    
    $params = array();        
    foreach($params_to_pass as $k){
      if(isset($post[$k]) && $post[$k] && !isset($_GET[$k])) $params[$k] = $post[$k];    
    }
    $out .=  theme_pager(NULL, 10, 0, $params); 
  }else{
    $out .= '<center>' . t('No translation jobs found.') . '</center>';
  }
  
  return $out;
}

/**
 * Returns a select control with existing translators
 *
 */
function _icl_translators_dropdown($rid, $uid, $from_lang, $to_lang){
    static $translators = array();
    if(empty($translators)){
        $translators[] = t('Any');
        foreach(_icl_translate_get_translators(array('from_lang'=>$from_lang, 'to_lang'=>$to_lang)) as $t){
            $translators[$t['uid']] = $t['name'];
        } 
    }
    $out = '<span>';
    $out .= '<select class="icl_assign_translator" id="icl_assign_translator_rid_'.$rid.'">';
    foreach($translators as $tr_uid=>$name){
        if($tr_uid == $uid){
          $selected = ' selected="selected"';
        }else{
          $selected = '';  
        }
        $out .= '<option value="'.$tr_uid.'"'.$selected.'>'.$name.'</option>';    
    }
    $out .= '</select><span id="icl_assign_translator_buttons_'.$rid.
            '" style="display:none">&nbsp;<input type="button" class="icl_assign_translator_save" value="'.
            t('Save').'" id="icl_assign_translator_for_'.$rid.
            '" />&nbsp;<input class="icl_assign_translator_cancel" type="button" value="'.
            t('Cancel').'" id="icl_assign_translator_cancel_'.$rid.'" /></span>';
    $out .= '</span>';
    return $out;   
}

/**
 * Filter form for user jobs list
 *
 * 
 * @param object $context 
 * @param object account
 *      
 */
function icl_translate_translator_jobs_form($context, $args) {
  $account = isset($args['account']) ? $args['account'] : false;
  $jobs = isset($args['jobs']) ? $args['jobs'] : false;
  $post = isset($args['post']) ? $args['post'] : false;
  
  if (isset($_GET['finished']) && isset($_GET['type'])) {
    module_load_include('inc', 'icl_translate', 'icl_translate.editor_form');
    $txt = _icl_translate_editor_get_title($_GET['type'], $_GET['finished']);
    if ($_GET['type'] == 'node') {
      drupal_set_message($message = t('Translation for !title is complete. You can review the !link and make sure it is correct.', array('!title' => strip_tags($txt), '!link' => $txt)));
    }
    else {
      drupal_set_message($message = t('Translation for !title is complete.', array('!title' => $txt)));
    }
  }
  
  $form['filterby'] = array(
    '#type' => 'fieldset',
    '#title' => t('Filter by'),
    '#collapsible' => true
  );
    
  if(!$account){
    $translators[] = t('All');
    foreach(_icl_translate_get_translators() as $t){
        $translators[$t['uid']] = $t['name'];
    }
    $form['filterby']['translator'] = array(
        '#title' => t('Translation jobs for'),
        '#type' => 'select',
        '#options' => $translators,        
    );
    if(isset($post['translator'])){
        $form['filterby']['translator']['#value'] = $post['translator'];
    }
    
  }
  
  $form['filterby']['status'] = array(
    '#type'  => 'select',
    '#title' => t('Status'),
    '#options' => array(
        '' => t('All'),
        ICL_STATUS_INQUEUE => t('Not done'),
        ICL_STATUS_REQUESTED => t('In Progress'),
        ICL_STATUS_SUCCESSFUL => t('Complete')
    )
  );
  if(isset($post['status'])){
    $form['filterby']['status']['#value'] = $post['status'];
  }
  
  
  if(!$account){
    $language_list = language_list('enabled');
    $languages = array();
    $languages[] = t('All');
    foreach($language_list[1] as $k=>$l){
        $languages[$k] = $l->name;    
    };    
    $form['filterby']['from'] = array(
        '#title' => t('From'),
        '#type' => 'select',
        '#options' => $languages
    );
    if(isset($post['from'])){
      $form['filterby']['from']['#value'] = $post['from'];
    }
    $form['filterby']['to'] = array(
        '#title' => t('To'),
        '#type' => 'select',
        '#options' => $languages
    );
    if(isset($post['to'])){
      $form['filterby']['to']['#value'] = $post['to'];
    }
      
  }
  
  $form['filterby']['submit'] = array(
    '#type'  => 'submit',
    '#name'  => 'submit',
    '#value' => t('Refresh')
  );
  
  if(!$account){
      // jobs stats
      $josb_status = _icl_translator_get_jobs_stats();
            
      $lang_from = $post['from'] ? $language_list[1][$post['from']]->name : t('All');
      $lang_to = $post['to'] ? $language_list[1][$post['to']]->name : t('All');

      $form['summary'] = array(
        '#type' => 'fieldset',
        '#title' => t('Summary'),
        '#collapsible' => true    
      );
      $form['summary']['#value'] = '<div>';
      $form['summary']['#value'] .= t('Total in progress from !lang_from to !lang_to: !words_in_progress words in !in_progress job!plural.', 
        array(
            '!lang_from' => $lang_from,
            '!lang_to' => $lang_to,
            '!words_in_progress' => intval($josb_status[ICL_STATUS_REQUESTED]['word_count']),
            '!in_progress' => intval($josb_status[ICL_STATUS_REQUESTED]['count']),
            '!plural' => intval($josb_status[ICL_STATUS_REQUESTED]['count']) !=1 ? 's' : ''
            )
        );
      $form['summary']['#value'] .= '<br />';
      $form['summary']['#value'] .= t('Total complete from !lang_from to !lang_to: !words_complete words in !complete job!plural.', 
        array(
            '!lang_from' => $lang_from,
            '!lang_to' => $lang_to,        
            '!words_complete' => intval($josb_status[ICL_STATUS_SUCCESSFUL]['word_count']),
            '!complete' => intval($josb_status[ICL_STATUS_SUCCESSFUL]['count']),
            '!plural' => intval($josb_status[ICL_STATUS_SUCCESSFUL]['count']) !=1 ? 's' : ''
            )
        );
      $form['summary']['#value'] .= '</div>';
  }   
  
  return $form;    
}

function _icl_translator_get_jobs_stats($args=array()){
    if(isset($args['from'])){
        $where .= " AND origin='".$from."'";
    }
    if(isset($args['to'])){
        $where .= " AND origin='".$from."'";
    }
    $status = array();
    $res = db_query("SELECT status, COUNT(rid) AS c FROM {icl_core_status} GROUP BY status " . $where);
    while($row = db_fetch_array($res)){
        $status[$row['status']]['count'] = $row['c'];    
    }
    
    $query = "
        SELECT crs.rid, crs.status
        FROM {icl_translate_job} tj
        JOIN {icl_core_status} crs ON crs.rid = tj.rid 
        LEFT JOIN {users} u ON tj.uid = u.uid
        WHERE translation_service='local' $where ORDER BY crs.rid DESC";
    $res = db_query($query);
    $rows = array();
    while ($r = db_fetch_array($res)) {
      $rows[$r['rid']] = $r; 
    }
        
    if(!empty($rows)){
        
        $rids = array_keys($rows);
        
        $exclude_rids = array();
      
        // get nodes
        $q = db_query("SELECT n.nid, cs.rid, n.title FROM {node} n JOIN {icl_content_status} cs ON cs.nid = n.nid WHERE cs.rid IN (%s)", join(',', $rids));
        while ($r = db_fetch_array($q)) {
            $exclude_rids[] =  $r['rid'];
            $content = db_fetch_array(db_query("SELECT title, body FROM {node_revisions} WHERE nid='{$r['nid']}' ORDER BY vid DESC LIMIT 1"));
            $status[$rows[$r['rid']]['status']]['word_count'] += str_word_count($content['title'] . ' ' . $content['body']);      
        }    
        $rids = array_diff($rids,$exclude_rids); 
    
        // get blocks
        if(!empty($rids)){
            $q = db_query("
                SELECT bs.rid, b.title, bx.body  
                FROM {icl_block_status} bs 
                JOIN {boxes} bx ON bx.bid = bs.bid
                JOIN {blocks} b ON b.delta = bs.bid
                WHERE bs.rid IN (%s)", join(',', $rids));
            while ($r = db_fetch_array($q)) {
                $exclude_rids[] =  $r['rid'];
                $status[$rows[$r['rid']]['status']]['word_count'] += str_word_count($content['title'] . ' ' . $content['body']);      
            }
        }
        $rids = array_diff($rids,$exclude_rids); 
    
        // get contact form
        $cf = variable_get('icl_contact_form_rids', FALSE);
        if(!empty($cf)){
            foreach($cf as $l=>$r){
                if(isset($filter['to']) && !empty($filter['to']) && $filter['to'] != $l) continue; 
                if(!isset($rows[$r])) continue;
                $exclude_rids[] =  $r;
                $q = db_query("SELECT field_data FROM {icl_translate} WHERE rid = %s AND field_translate=1",$r);
                while($vals = db_fetch_array($q)){        
                    $val = unserialize($vals['field_data']);
                    if(is_string($val)){
                        $status[$rows[$r]['status']]['word_count'] += str_word_count($val);        
                    }
                }
            }
            $rids = array_diff($rids,$exclude_rids); 
        }
    
        // get strings
        if(!empty($rids)){
            //to be added
        }
    
    }
    
    return $status;
}

/**
 * Add translator's language pairs form call.
 */
function icl_translate_add_translator() {    
  return drupal_get_form('_icl_translate_translator_form', 0);
}

/**
 * Update translator's language pairs form call.
 */
function icl_translate_edit_translator($uid) {  
  return drupal_get_form('_icl_translate_translator_form', $uid);
}

/**
 * Add/update translator's language pairs form.
 * @todo Check if Drupal has this function (not found).
 * 
 * @param string $action
 *      Determines if it's add or update ('add'/'update').
 */
function _icl_translate_translator_form($context, $uid) {
  _icl_translate_add_scripts();
  drupal_add_css(drupal_get_path('module', 'icl_translate') . '/css/icl_translate_translator.css');
  drupal_add_js(drupal_get_path('module', 'icl_translate') . '/js/icl_translate_translator.js');
  if (!$uid) {
    
    // Get users that aren't translators
    $users = _icl_translate_get_non_translators();
    if (empty($users)) {
      drupal_set_message(t('All of the users are already translators. Go to <a href="@url">user list</a>', array('@url' => url('admin/user/user'))));  
      return false;
    }
    // Construct selection element
    $select[] = t('Select user');
    foreach ($users as $k => $user) {
      $select[$user->uid] = $user->name;
    }
    
    // Add selection
    $form = array();
    $form['icl_translate_jobs'] = array(
      '#type' => 'fieldset',
      '#title' => t('Add Translator'), 
      '#collapsible' => false
    );
    $form['icl_translate_jobs']['translator'] = array(
      '#type' => 'select',
      '#options' => $select,
      '#title' => t('Add translator'),
      '#name' => 'icl_translate_translator'
    );
    $form['icl_translate_jobs']['break'] = array(
      '#value' => '<br />'
    );
  
  
  }
  else{
    // Get user
    $user = user_load($uid);
    if (!$user) {
      drupal_set_message(t('Unknown user.'), 'error');  
      return;
    }
    // Get translation pairs for user
    $pairs = _icl_translate_get_translator_pairs($uid);
    $form['icl_translate_jobs'] = array(
      '#type' => 'fieldset', 
      '#title' => sprintf(t('Edit Translator %s'), $user->name), 
      '#collapsible' => false
    );
    $form['icl_translate_jobs']['translator'] = array(
      '#type' => 'hidden',
      '#name' => 'icl_translate_translator',
      '#default_value' => $uid
    );
  }
  
  // Get languages lists
  $language_list = language_list('enabled');
  $language_list = $language_list[1];
  $default_language = language_default();
  unset($language_list[$default_language->language]);
  
  
  // First set default langage
  if (isset($pairs[$default_language->language])) {
   $check = 1;
   $add_class = '';
  }
  else {
    $check = 0;
    $add_class = ' icl_translate_hidden';
  }
  
  $form['icl_translate_jobs']['default'] = array(
    '#type' => 'checkbox',
    '#title' => sprintf(t('From %s'), $default_language->name),
    '#default_value' => $check,
    '#attributes' => array('class' => 'icl_translate_translator_language_show')
  );
  
  
  
  // Set default language pairs
  $form['icl_translate_jobs']['break_1'] = array(
    '#value' => '<div class="icl_translate_language_pairs' . $add_class . '">'
  );
  
  foreach ($language_list as $lang) {
    
    if (isset($pairs[$default_language->language]) && array_key_exists($lang->language, $pairs[$default_language->language])) {
     $check = 1;
    }
    else {
      $check = 0;
    }
    $form['icl_translate_jobs']['default_to_'.$lang->language] = array(
      '#type' => 'checkbox',
      '#title' => sprintf(t('to %s'), $lang->name),
      '#name' => 'icl_translate_language_pairs[' . $default_language->language . '][' . $lang->language . ']',
      '#default_value' => $check
     );
  }
  
  $form['icl_translate_jobs']['txt_1'] = array(
    '#value' => t('Select the languages that the translator can translate to')
  );
  
  $form['icl_translate_jobs']['break_2'] = array(
    '#value' => '</div>'
  );
  
  
  
  
  // Set other languages
  foreach ($language_list as $lang) {
    
    if (isset($pairs[$lang->language])) {
     $check = 1;
     $add_class = '';
    }
    else {
      $check = 0;
      $add_class = ' icl_translate_hidden';
    }
    
    $form['icl_translate_jobs'][$lang->name] = array(
      '#type' => 'checkbox',
      '#title' => sprintf(t('From %s'), $lang->name),
      '#default_value' => $check,
      '#attributes' => array('class' => 'icl_translate_translator_language_show')
    );
    
    
    
    // Set language pairs
    $form['icl_translate_jobs']['break_1_' . $lang->language] = array(
      '#value' => '<div class="icl_translate_language_pairs' . $add_class . '">'
    );
    
    if (isset($pairs[$lang->language]) && array_key_exists($default_language->language, $pairs[$lang->language])) {
      $check = 1;
    }
    else {
      $check = 0;
    }
    
    $form['icl_translate_jobs'][$lang->name . '_to_default'] = array(
      '#type' => 'checkbox',
      '#title' => sprintf(t('to %s'), $default_language->name),
      '#name' => 'icl_translate_language_pairs[' . $lang->language . '][' . $default_language->language . ']',
      '#default_value' => $check
    );
    
    foreach ($language_list as $lang_sub) {
      
      // Skip same language
      if ($lang_sub->language == $lang->language) {
        continue;
      }
      
      if (isset($pairs[$lang->language]) && array_key_exists($lang_sub->language, $pairs[$lang->language])) {
        $check = 1;
      }
      else {
        $check = 0;
      }
      
      $form['icl_translate_jobs'][$lang->name . '_to_'.$lang_sub->name] = array(
        '#type' => 'checkbox',
        '#title' => sprintf(t('to %s'), $lang_sub->name),
        '#name' => 'icl_translate_language_pairs[' . $lang->language . '][' . $lang_sub->language . ']',
        '#default_value' => $check
      );
    }
    
    $form['icl_translate_jobs']['txt_' . $lang->language] = array(
      '#value' => t('Select the languages that the translator can translate to')
    );
    
    $form['icl_translate_jobs']['break_2_' . $lang->language] = array(
      '#value' => '</div>'
    );
  }
  
  if ($action == 'add') {
    $submit_txt = t('Add translator');
  }
  else {
    $submit_txt = t('Save');
  }
  
  $form['action'] = array(
    '#type' => 'submit',
    '#submit' => array('icl_translate_translator_submit'),
    '#value' => $submit_txt
  );
  
  return $form;
}

/**
 * Process submit translator's language pairs.
 */
function icl_translate_translator_submit() {
  if (user_access(ICL_TRANSLATE_MANAGE_JOBS)) {
    if (isset($_POST['icl_translate_translator']) && isset($_POST['icl_translate_language_pairs'])) {
      
      // Create role is necessary
      _icl_translate_assign_role($_POST['icl_translate_translator']);
      
       // Update language pairs
      _icl_translate_update_translator_pairs($_POST['icl_translate_translator'], $_POST['icl_translate_language_pairs']);
      
      drupal_set_message(t('Settings saved.'));
    }
  }
}

/**
 * Fetches users that aren't 'translators'.
 * @todo Check if Drupal has this function (not found).
 */
function _icl_translate_get_non_translators() {
  $rid = variable_get('icl_translate_role', 0);
  $q = db_query("SELECT uid FROM {users} WHERE status=%d", 1);
  $rows = array();
  while ($r = db_fetch_array($q)) {
    if ($r['uid'] == 0) {
      continue;
    }
    $user = user_load($r['uid']);    
    if (!_icl_translate_is_translator($user)) {
      $rows[] = $user;
    }
  }
  return $rows;
}

/**
 * Assign 'translator' role to specified user.
 * @todo Check if Drupal has this function (not found).
 */
function _icl_translate_assign_role($uid) {
  $rid = variable_get('icl_translate_role', 0);
  if (!db_result(db_query("SELECT uid FROM {users_roles} WHERE uid=%d AND rid=%d", $uid, $rid))) {
    $data = array('uid' => $uid, 'rid' => $rid);
    drupal_write_record('users_roles', $data);
  }
}

/**
 * Checks if user has 'translator' role.
 * @todo Check if Drupal has this function (not found).
 * 
 * @param object $user (optional)
 *      If not supplied current user is checked.
 */
function _icl_translate_is_translator($user = null) {
  $rid = variable_get('icl_translate_role', 0);
  if ($role = db_result(db_query("SELECT name FROM {role} WHERE rid=%d", $rid))) {
    if (is_null($user)) {
      global $user;
    }
    return array_search($role, $user->roles);
  }
  else {
    return false;
  }
}

/**
 * Checks if user can edit specific translation.
 */
function _icl_translate_can_translate($rid, $assign_to_me = FALSE) {
  global $user;
  // If $assign_to_me is TRUE then we should try to assign the job to the
  // translator if it is not already assigned to a translator.
  if ($assign_to_me) {
    db_query("UPDATE {icl_translate_job} SET uid=%d WHERE rid=%d AND uid=-1", $user->uid, $rid);
  }
  
  if ($user->uid == 1) {
    return TRUE; // user 1 can always edit.
  } else {
    return db_result(db_query("SELECT rid FROM {icl_translate_job} WHERE uid=%d AND rid=%d", $user->uid, $rid));
  }
}

/**
 * Set the job status to in progress.
 */

function _icl_translate_set_to_in_progress($rid) {
  db_query("UPDATE {icl_core_status} SET status=%d WHERE rid=%d", ICL_STATUS_REQUESTED, $rid);
}

/**
 * Fetches all translators assigned to specified request.
 */
function _icl_translate_get_assigned_translators($rid) {
  $q = db_query("SELECT * FROM {icl_translate_job} r JOIN {users} u ON r.uid=u.uid WHERE r.rid=%d AND u.status=%d", $rid, 1);
  $rows = array();
  while ($r = db_fetch_array($q)) {
    $rows[] = $r;
  //$rows[] = user_load($r['uid']);
  }
  return $rows;
}

/** 
 * Fetches all users with 'translator' role.
 * @todo Check if Drupal has this function (not found).
 */
function _icl_translate_get_translators($args = array()) {
  $join  = $where = '';  
  $_tjp_joined = false;
  if(isset($args['from_lang'])){
      $_tjp_joined = true;
      $join  .= " JOIN {icl_translate_job_pairs} tjp ON u.uid = tjp.uid ";
      $where .= " AND tjp.source='{$args['from_lang']}'";
  }  
  if(isset($args['to_lang'])){
      if(!$_tjp_joined) $join  .= " JOIN {icl_translate_job_pairs} tjp ON u.uid = tjp.uid ";
      $where .= " AND tjp.target='{$args['to_lang']}'";
  }
  $rid = variable_get('icl_translate_role', 0);
  $q = db_query("
            SELECT * FROM {users_roles} r 
            JOIN {users} u ON r.uid=u.uid {$join}
            WHERE r.rid=%d AND u.status=%d {$where}", 
        $rid, 1);
  $rows = array();
  while ($r = db_fetch_array($q)) {
  //$rows[] = user_load($r['uid']);
  $rows[] = $r;
  }
  return $rows;
}

/**
 * Inserts language pair for specified user.
 */
function _icl_translate_set_translator_pair($uid, $source, $target) {
  if (!db_result(db_query("SELECT uid FROM {icl_translate_job_pairs} WHERE uid=%d AND source='%s' AND target='%s'", $uid, $source, $target))) {
    $data = array('uid' => $uid, 'source' => $source, 'target' => $target);
    drupal_write_record('icl_translate_job_pairs', $data);
  }
}

/**
 * Deletes language pair for specified user.
 */
function _icl_translate_delete_translator_pair($uid, $source, $target) {
  db_query("DELETE FROM {icl_translate_job_pairs} WHERE uid=%d AND source='%s' AND target='%s'", $uid, $source, $target);
}

/**
 * Bulk insert/update language pairs for specified user.
 * 
 * @param array $data
 *      Specify in form: array('source' => array('target'))
 */
function _icl_translate_update_translator_pairs($uid, $data = array()) {
  $pairs = _icl_translate_get_translator_pairs($uid);
  
  // add some
  foreach ($data as $source => $targets) {
    foreach ($targets as $target => $v) {
      if(!isset($pairs[$source][$target])){
        _icl_translate_set_translator_pair($uid, $source, $target);
      }
    }
  }
  
  // remove some
  foreach ($pairs as $source => $targets) {
    foreach ($targets as $target => $v) {
      if(!isset($data[$source][$target])){
        _icl_translate_delete_translator_pair($uid, $source, $target);
      }
    }
  }
  
  
}

/**
 * Fetches all language pairs for specified translator.
 */
function _icl_translate_get_translator_pairs($uid) {
  $q = db_query("SELECT * FROM {icl_translate_job_pairs} WHERE uid=%d", $uid);
  $rows = array();
  while ($r = db_fetch_array($q)) {
    $rows[$r['source']][$r['target']] = 1;
  }
  return $rows;
}

/**
 * Fetches all translators with specified language pair.
 */
function _icl_translate_get_translators_for_pair($source, $target) {
  $rid = variable_get('icl_translate_role', 0);
  $q = db_query("SELECT * FROM {users_roles} u JOIN {icl_translate_job_pairs} t WHERE u.rid=%d AND t.source='%s' AND t.target='%s'", $rid, $source, $target);
  $rows = array();
  while ($r = db_fetch_array($q)) {
    $rows[] = user_load($r['uid']);
  }
  return $rows;
}

/**
* Assign a translation job to a translator
* 
* @param int $uid
* @param int $rid
* 
* @return int tjid
*/
function icl_translator_add_job($uid, $rid){
    if($uid == 0 || _icl_translate_is_translator(user_load($uid))){
        $res = db_fetch_array(db_query("SELECT tjid FROM {icl_translate_job} WHERE rid=%d LIMIT 1", $rid));
        if($res){
            $tjid = $res['tjid'];            
            db_query("UPDATE {icl_translate_job} SET uid=%d WHERE tjid=%d", $uid, $tjid);
        }else{
            $data = array('uid' => $uid, 'rid' => $rid);
            drupal_write_record('icl_translate_job', $data);    
            $tjid = db_last_insert_id('icl_translate_job','tjid');
        }
    }else{
        $tjid = 0;
    }        
    return $tjid;
}