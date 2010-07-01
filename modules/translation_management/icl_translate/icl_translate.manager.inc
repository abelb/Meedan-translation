<?php
// $Id: icl_translate.manager.inc,v 1.1 2010/06/30 08:22:07 brucepearson Exp $

function icl_translate_manager() {
  global $base_url;
  $path = $base_url . '/' . drupal_get_path('module', 'icl_core') . '/images/web_logo_large.png';

  $output = '<img src="' . $path . '"  style="margin: 0px 0px -5px 0px; float: left; "><br clear="all" />';
  $output .= t('<h1>Translation managagement system by ICanLocalize</h1>');
  
  module_load_include('inc', 'system', 'system.admin');
  
  $output .= system_admin_menu_block_page();
  
  return $output;
}