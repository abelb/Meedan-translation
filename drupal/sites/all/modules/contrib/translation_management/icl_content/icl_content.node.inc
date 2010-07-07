<?php
/*
 * $Id: icl_content.block.inc
 * 
 * @file ICanLocalize content node handling
 */

function icl_content_get_nodes_for_dashboard($icl_content_dashboard_filter) {
  $languages = locale_language_list();

  if (isset($_GET['nid'])) {
    $where = 'nid=' . $_GET['nid'];
  } else {
    
    // Filter by language and "Filter further by:" settings.
    
    $where = 'language="' . $icl_content_dashboard_filter['language'] . '" ';
    
    if (isset($icl_content_dashboard_filter['status_status']) && $icl_content_dashboard_filter['status_enabled']) {
      list($column, $value) = explode('-', $icl_content_dashboard_filter['status_status']);
      if (in_array($column, array('status', 'promote', 'sticky'))) {
        $where .= " AND n.".$column." = ".$value;
      }
    }
    
    if (isset($icl_content_dashboard_filter['type_type']) && isset($icl_content_dashboard_filter['type_enabled'])) {
      if ($icl_content_dashboard_filter['type_enabled']) {
        $where .= " AND n.type = '". $icl_content_dashboard_filter['type_type'] . "'";
      }
    }
      
    if (isset($icl_content_dashboard_filter['search_text']) && isset($icl_content_dashboard_filter['search_enabled'])) {
      if ($icl_content_dashboard_filter['search_enabled'] && $icl_content_dashboard_filter['search_text'] != '') {
        $where .= " AND n.title LIKE '%%". $icl_content_dashboard_filter['search_text'] . "%%'";
      }
    }
  }
  
  $query = "
    SELECT n.nid, n.type, n.language, n.status, n.title
    FROM {node} n 
    WHERE $where
    ";
  $res = db_query($query);
 
  $matching_nids = array();
  while($row = db_fetch_array($res)){
    $status = array('id' => $row['nid'],
                    'link' => 'node/' . $row['nid'],
                    'type' => $row['type'],
                    'origin' => $row['language'],
                    'status' => $row['status'] == 1 ? t('Published') : t('Unpublished'),
                    'title' => $row['title'],
                    'targets' => array());
    foreach ($languages as $lang => $name) {
      if ($lang != $row['language']) {
        $status['targets'][$lang] = array();
  
        // see if a translation exists
        $translation_nid = db_result(db_query('SELECT nid FROM {node} WHERE language="%s" AND tnid=%d', $lang, $row['nid']));
        $status['targets'][$lang]['translated'] = $translation_nid != 0;
        
        $current_md5 = db_result(db_query('SELECT md5 FROM {icl_node} WHERE nid=%d', $row['nid']));
        
        // Find the current rid and rid_status.
        $status['targets'][$lang]['current_rid'] = 0;
        $status['targets'][$lang]['rid_status'] = 0;
        $status['targets'][$lang]['needs_update'] = 0;
        $status_res = db_query('SELECT c.rid, c.status, cs.md5
                                FROM
                                  {icl_content_status} cs
                                JOIN
                                  {icl_core_status} c
                                ON
                                  cs.rid = c.rid
                                  
                                WHERE cs.nid=%d AND c.target="%s"', $row['nid'], $lang);
        $last_md5 = '';
        while($status_row = db_fetch_array($status_res)) {
          if ($status_row['rid'] > $status['targets'][$lang]['current_rid']) {
            $status['targets'][$lang]['current_rid'] = $status_row['rid'];
            $status['targets'][$lang]['rid_status'] = $status_row['status'];
            $last_md5 = $status_row['md5'];
          }
        }
        if ($last_md5 != '') {
          // The translation needs updating when the current md5 is different
          // from the md5 when the translation was last sent.
          $status['targets'][$lang]['needs_update'] = $last_md5 != $current_md5;
        }
        
        
      }
    }
    $matching_nids[] = $status;
  }
  
  return $matching_nids;
 
}

/**
 * Extracts valuable data from a node object.
 *
 * @param object $node
 * @return array
 */
function _icl_content_extract_node($node_in, $targets = NULL) {
  $node = drupal_clone($node_in);
  // Prepare the node so the menu and taxonomy will be available.
  module_load_include('inc', 'node', 'node.pages');
  node_object_prepare($node);
  
  if ($targets != NULL) {
    $tnid = db_result(db_query("SELECT tnid FROM {node} where nid=%d", $node->nid));            
    $translations = icl_content_translation_node_get_translations($tnid);
    if ($translations != NULL) {
      foreach ($translations as $lang => $trans) {
        if ($trans->nid == $node->nid) {
          unset($translations[$lang]);
        } else {
          $translations[$lang] = node_load($trans->nid);
        }
      }
    }
  }
  
  
  $taxonomy = array ();
  if (module_exists ( 'i18ntaxonomy' )) {
    // Preprocess taxonomy for easier access    
    if (isset ( $node->taxonomy )) {
      foreach ( $node->taxonomy as $key => $value ) {
        if ($key == 'tags') {
          foreach ( $value as $vid => $terms ) {
            if (! empty ( $terms )) {
              $terms = explode ( ',', $terms );
              $terms = array_map ( 'trim', $terms );
              $terms_to_translate = array();
              foreach($terms as $name) {
                if ( ! _icl_content_do_translations_exist($name, $node->language, $node->icanlocalize)) {
                  $terms_to_translate [] = $name;
                }
                
              }
              $taxonomy ['taxonomy[' . $vid . ']'] = $terms_to_translate;
            }
          }
        } else if(is_string($value)) {  // Single value vocabulary
          $term = taxonomy_get_term($value);
          if (!empty($term)) {
            if ( ! _icl_content_do_translations_exist($term->name, $node->language, $node->icanlocalize)) {
              $taxonomy ['taxonomy['.$key.']'] = array($term->name);
            }
          }
        } else if(isset($value->tid)) {
          $term = taxonomy_get_term($value->tid);
          if (!empty($term)) {
            if ( ! _icl_content_do_translations_exist($term->name, $node->language, $node->icanlocalize)) {
              // Add term to the vocab id in taxonomy array.
              $vocab_id = $term->vid;
              if (!isset($taxonomy ['taxonomy['.$vocab_id.']'])) {
                $taxonomy ['taxonomy['.$vocab_id.']'] = array();
              }
              $taxonomy ['taxonomy['.$vocab_id.']'][] = $term->name;
            }
          }
        } else {
          $vocab = array ();
          foreach ( $value as $tid ) { // Multi-value vocabulary
            $term = taxonomy_get_term ( $tid );
            if ( ! _icl_content_do_translations_exist($term->name, $node->language, $node->icanlocalize)) {
              $vocab [] = $term->name;
            }
          }
          if (! empty ( $vocab )) {
            $taxonomy ['taxonomy[' . $key . ']'] = $vocab;
          }
        }        
      }
    }
    foreach ( $taxonomy as $key => $value ) {
      $attr = $key;
      $node->$attr = $value;
    }
  }
  
  // Add the menu if we have one.
  
  if (isset($node->menu['link_title'])) {
    $node->link_title = $node->menu['link_title'];
  }

  if (isset($node->menu['options']['attributes']['title'])) {
    if (strlen($node->menu['options']['attributes']['title']) > 0) {
      $node->link_description = $node->menu['options']['attributes']['title'];
    }
  }
  
  $node_type_fields = variable_get ( 'icl_content_node_type_fields', array (
      $node->type => array (
          'title', 
          'body' ) ) );
  $translatable = isset ( $node_type_fields [$node->type] ) ? $node_type_fields [$node->type] : array (
      'title', 
      'body' );
  
  // if the link_tilte is translatable then make the link_description translatable
  if (isset($translatable['link_title'])) {
    $translatable['link_description'] = 'link_description';
  }

  if (module_exists('content') && module_exists('flexifield')) {
    $type = content_types($node->type);
    if (isset($type['fields'])) {
      foreach ($type['fields'] as $field) {
        $field_name = $field['field_name'];
        $cck_data = $node->$field_name;
        foreach ($cck_data as $index => $data) {
          
          if (isset($data['value']) && is_array($data['value'])) {
            // a flexifield
            // We will arrange these so they appear as fields we can access directly
            // at the node level like normal cck fields.
            foreach ($data['value'] as $flexi_field_name => $flexi_field_data) {
              if (in_array($flexi_field_name, $translatable)) {
                unset($translatable[$flexi_field_name]);
                $flexi_field_name = $field_name . '[' . $flexi_field_name. ']';
                $translatable[$flexi_field_name] = $flexi_field_name;
                $node->$flexi_field_name = $flexi_field_data;
                
              }
            }
          }
        }
      }
    }
  }
  
  $data = array (
      array (
          'type' => 'nid', 
          'translate' => 0, 
          'text' => $node->nid ), 
      array (
          'type' => 'vid', 
          'translate' => 0, 
          'text' => $node->vid ) );
  foreach ( $translatable as $field ) {
    if (! empty ( $node->$field )) {
      $item = array();
      if (is_array($node->$field) && !isset ( $taxonomy [$field] )) {  // CCK or nodewords.
        if ($field == 'nodewords') {
          $nodewords = $node->$field;
          // node words
          $keys = array('abstract', 'description', 'keywords');
          foreach($keys as $node_word_field) {
            if (!empty($nodewords[$node_word_field]['value'])) {
              $data[] = array (
                  'type' => $field . '-' . $node_word_field . '- (SEO)', 
                  'translate' => 1, 
                  'text' => $nodewords[$node_word_field]['value'] );
            }            
          }
          
        } else {
          // CCK
          $content = array();
          foreach ($node->$field as $element) {
            $content[] = $element['value'];
            $line_break_converter_found = false;
            if (isset($element['format'])) {
              $formats = filter_list_format($element['format']);
              foreach($formats as $format) {
                if ($format->delta == 1) {
                  $line_break_converter_found = True;
                }
              }
            }
          }
          $item = array(
            'type' => 'CCK['.$field.']',
            'translate' => 1,
            'text' => $content,
            'format' => 'csv',
          );        
          if (!$line_break_converter_found) {
            $item['filter_line_breaks'] = 0;
          }
        }
      }
      else {  // Non-CCK
        $item = array (
            'type' => $field, 
            'translate' => 1, 
            'text' => $node->$field );
        
        // get the translations
        if ($targets != NULL && $translations != NULL) {
          foreach ($translations as $lang => $trans_node) {
            if (!isset($item['translations'])) {
              $item['translations'] = array();
            }
            $item['translations'][$lang] = $trans_node->$field;
          }
        }
        
        if (isset ( $taxonomy [$field] )) {
          $item ['format'] = 'csv';
        }
        if ($field == 'body') {
          // check the format to see if line breaks need to be converted
          $formats = filter_list_format($node->format);
          $line_break_converter_found = false;
          foreach($formats as $format) {
            if ($format->delta == 1) {
              $line_break_converter_found = True;
            }
          }
          if (!$line_break_converter_found) {
            $item['filter_line_breaks'] = 0;
          }
          
        }
      }
      if (sizeof($item) > 0) {
        $data [] = $item;
      }
    }
  }

  if (!module_exists ( 'pathauto' )) {
    // if pathauto is not available then send the url alias for translation
    
    $alias = drupal_lookup_path('alias', 'node/' . $node->nid, $node->language);
    if ( $alias ) {
      $item = array (
        'type' => 'url_alias',
        'translate' => 1,
        'text' => $alias,
        );
      
      $data [] = $item;
      
    }
    
  }

  return $data;
}

