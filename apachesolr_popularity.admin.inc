<?php

/**
 * @file
 * Admin page callbacks for the Apache Solr Popularity module.
 */

/**
 * Page callback: Displays popularity of the the top pages.
 *
 * This displays the pages with the highest popularity.
 * Note: only updates ones per cron run
 *
 * @return
 *   A render array containing information about the most popular pages.
 */
function apachesolr_popularity_top_pages() {
  
  $header = array(
    array('data' => t('Node'), 'field' => 'nid'),
    array('data' => t('Popularity'), 'field' => 'popularity', 'sort' => 'desc'),
    array('data' => t('Recent Count'), 'field' => 'recentcount'),
    array('data' => t('Days Tracked'), 'field' => 'daystracked'),
  );

  $query = db_select('apachesolr_popularity_counter', 'p', array('target' => 'slave'))->extend('PagerDefault')->extend('TableSort');;
  //$query->addExpression('COUNT(path)', 'hits');
  $query->fields('p', array('nid', 'popularity', 'recentcount', 'daystracked'));
  $query
    ->groupBy('nid')
    ->limit(30)
    ->orderByHeader($header);
  $result = $query->execute();
  $rows = array();
  foreach ($result as $page) {
    $rows[] = array($page->nid, $page->popularity, $page->recentcount, $page->daystracked);
  }
  //TODO: Change nid to a link to the path
  
  drupal_set_title(t('Most popular pages'), PASS_THROUGH);
  $build['popularity_top_pages_table'] = array(
    '#theme' => 'table',
    '#header' => $header,
    '#rows' => $rows,
    '#empty' => t('No popularity statistics available.'),
  );
  $build['popularity_top_pages_pager'] = array('#theme' => 'pager');
  return $build;
}


/**
 * Form constructor for the Apache Solr Popularity administration form.
 */
function apachesolr_popularity_settings_form() {
  // Config page
  $form['settings'] = array(
    '#type' => 'fieldset',
    '#title' => t('Apache Solr Popularity Settings'),
  );
  $form['settings']['popularity_tracker'] = array(
    '#type' => 'checkbox',
    '#title' => t('Track node popularity'),
    '#default_value' => variable_get('popularity_tracker', 0),
    '#description' => t('Track node viewing and calculate a popularity metric based on view frequency.'),
  ); 
  $form['settings']['reset_tracker'] = array(
    '#type' => 'checkbox',
    '#title' => t('Reset popularity values'),
    '#default_value' => FALSE, //variable_get('reset_tracker', FALSE),
    '#description' => t('Resets popularity tracking for all nodes.'),
  );
  $form['settings']['confirm_reset_fieldset'] = array(
    '#type' => 'fieldset',
    //'#title' => t('Confirm reset? (Cannot be undone)'),
    '#states' => array(
      'visible' => array(
        ':input[name="reset_tracker"]' => array('checked' => TRUE)
       ),
    ),
  );
  $form['settings']['confirm_reset_fieldset']['confirm_reset'] = array(
    '#type' => 'checkbox',
    '#title' => t('Confirm reset? (Cannot be undone)'),
    '#default_value' => FALSE,
    '#states' => array(
      'unchecked' => array(
        ':input[name="reset_tracker"]' => array('checked' => TRUE)
       ),
      'visible' => array(
        ':input[name="reset_tracker"]' => array('checked' => TRUE)
       ),
    ),
  );
  return system_settings_form($form);
}

/**
 * Validate setting form - hook_form_validate
 */
function apachesolr_popularity_settings_form_validate($form, &$form_state) {
  if ( $form_state['values']['reset_tracker']==1 AND 
       $form_state['values']['confirm_reset']==0) {
    form_set_error('confirm_reset', t('Please confirm reset!'));
  }
}

/**
 * Process form - hook_form_submit **** verify that this hook
 */
function apachesolr_popularity_settings_form_submit($form, &$form_state) {
  
  if ( $form_state['values']['reset_tracker']==1 AND 
       $form_state['values']['confirm_reset']==1) {
    
    db_update('apachesolr_popularity_counter')
        ->fields(array(
                'popularity' => 0.0,
                'recentcount' => 0,
                'daystracked' => 0.0,
                )
        )
        ->execute();
   }
}













