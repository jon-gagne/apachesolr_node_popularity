<?php

/**
 * @file
 * Admin page callbacks for the Apache Solr Popularity module.
 * 
 * @author Jonathan Gagne (jongagne)
 *   @see drupal.org/user/2409764
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
    array('data' => t('Title'), 'field' => 'nid'),
    //array('data' => t('Node ID'), 'field' => 'nid'),
    array('data' => t('Popularity'), 'field' => 'popularity', 'sort' => 'desc'),
    array('data' => t('Recent Count'), 'field' => 'recentcount', 'sort' => 'desc'),
    array('data' => t('Days Tracked'), 'field' => 'daystracked'),
  );

  $query = db_select('apachesolr_popularity_counter', 'p', array('target' => 'slave'))->extend('PagerDefault')->extend('TableSort');;
  //$query->addExpression('COUNT(path)', 'hits');
  $query->fields('p', array('nid', 'popularity', 'recentcount', 'daystracked'));
  $query
    ->groupBy('nid')
    ->limit(30)
    ->orderByHeader($header)
    ->orderBy('popularity', 'desc')
    ->orderBy('recentcount', 'desc');
  
  $result = $query->execute();
  $rows = array();
  foreach ($result as $page) {
    $rows[] = array(_apachesolr_popularity_format_item($page->nid), /*$page->nid,*/ $page->popularity, $page->recentcount, $page->daystracked);
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
    '#title' => t('Basic Popularity Settings'),
  );
  $form['settings']['popularity_tracker'] = array(
    '#type' => 'checkbox',
    '#title' => t('Enable node popularity'),
    '#default_value' => variable_get('popularity_tracker', 0),
    '#description' => t('Track node viewing and calculate a popularity metric based on view frequency.'),
  ); 
  $form['forgetting_window_options'] = array(
      '#type' => 'value',
      '#value' => array('0.1', '0.5', '2.0', '5.0', '10.0', '20.0', '100', '1000')
  );  
  $popularity_influence_options = array(
      0.01 => 0.01, 
      0.1 => 0.1, 
      0.5 => 0.5, 
      1.0 => 1.0, 
      2.0 => 2.0, 
      4.0 => 4.0, 
      10.0 => 10.0
  );
  $form['settings']['apachesolr_popularity_influence'] = array(
      '#type' => 'select',
      '#title' => t('Popularity influence'),
      '#default_value' => variable_get('apachesolr_popularity_influence', 1.0),
      '#options' => $popularity_influence_options,
      '#description' => t("How much the popularity influence Apache Solr's ranking values.")
  );
  $forgetting_window_options = array( 
      0.1 => 0.1, 
      0.5 => 0.5, 
      2 => 2, 
      5 => 5, 
      10 => 10, 
      20 => 20, 
      100 => 100, 
      1000 => 1000 
  );
  $form['settings']['apachesolr_popularity_forgetting_window'] = array(
      '#type' => 'select',
      '#title' => t('Forgetting window'),
      '#default_value' => variable_get('apachesolr_popularity_forgetting_window', 5.0),
      '#options' => $forgetting_window_options,
      '#description' => t('After this many days of tracking, biasing recent popularity starts to begin.  Lower values increase recent bias.')
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
  $form['#submit'][] = 'apachesolr_popularity_settings_form_submit';
  
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
 * Process form - hook_form_submit
 */
function apachesolr_popularity_settings_form_submit($form, &$form_state) {
  
  // Sets up popularity tracking table
  if ( $form_state['values']['popularity_tracker']==1) {
    $node_create_list = db_query(
            'SELECT n.nid
             FROM {node} AS n
             LEFT JOIN {apachesolr_popularity_counter} AS apc
             ON n.nid=apc.nid
             WHERE apc.nid IS NULL'
            );

    $query = db_insert('apachesolr_popularity_counter')->fields(array('nid'));
    foreach ($node_create_list as $node) {
      $query->values(array('nid' => $node->nid));
    }
    $query->execute();
  }
  
  // Resets popularity tracking
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


/**
 * Form constructor for the Apache Solr Popularity advanced administration form.
 */
function apachesolr_popularity_advanced_form() {
  // Config page
  $form['settings'] = array(
    '#type' => 'fieldset',
    '#title' => t('Advanced Popularity Settings'),
  );
  $form['settings']['popularity_tracker'] = array(
    '#type' => 'checkbox',
    '#title' => t('Track node popularity'),
    '#default_value' => variable_get('popularity_tracker', 0),
    '#description' => t('Track node viewing and calculate a popularity metric based on view frequency.'),
  ); 
  $form['#submit'][] = 'apachesolr_popularity_advanced_form_submit';
  
  return system_settings_form($form);
}


