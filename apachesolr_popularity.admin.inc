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
    '#title' => t('Enable Apache Solr Node Popularity'),
    '#default_value' => variable_get('popularity_tracker', 0),
    '#description' => t('Track node viewing and calculate a popularity metric based on view frequency.'),
  ); 
  
  $popularity_influence = variable_get('apachesolr_popularity_influence', 1.0);
  $popularity_influence_options = array( 0.01, 0.1, 0.5, 1, 2, 4, 10 );
  $form['settings']['apachesolr_popularity_influence'] = array(
      '#type' => 'select',
      '#title' => t('Popularity influence'),
      '#default_value' => array_search($popularity_influence, $popularity_influence_options), //,
      '#options' => $popularity_influence_options,
      '#description' => t("Amount that node popularity influences Apache Solr's ranking values.")
  );
  
  $forgetting_window = variable_get('apachesolr_popularity_forgetting_window', 5.0);
  $forgetting_window_options = array( 0.1, 0.5, 2, 5, 10, 20, 100, 1000 );
  $form['settings']['apachesolr_popularity_forgetting_window'] = array(
      '#type' => 'select',
      '#title' => t('Forgetting window'),
      '#default_value' => array_search($forgetting_window, $forgetting_window_options),
      '#options' => $forgetting_window_options,
      '#description' => t('After this many days of tracking, biasing recent popularity begins to fade on. <br> The lower the value is, the more that newer popularity measurements are biased.')
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
  
  $form['settings']['enable_advanced settings'] = array(
    '#type' => 'checkbox',
    '#title' => t('Enables advanced settings'),
    '#default_value' => variable_get('enable_advanced settings', FALSE),
    '#description' => t('For advanced users only'),
  );
  
  $init_damping = variable_get('apachesolr_popularity_init_damping', 0.2);
  $init_damping_options = array( 0.0, 0.05, 0.2, 0.5, 1, 2, 5 );
  $form['settings']['apachesolr_popularity_init_damping'] = array(
      '#type' => 'select',
      '#title' => t('Initial dampening'),
      '#default_value' => array_search($init_damping, $init_damping_options), //,
      '#options' => $init_damping_options,
      '#description' => t("Smoothes initial popularity estimates.<br> 0 = None, 0.2 = Moderate, 2 = High."),
      '#states' => array(
        'disabled' => array(
          ':input[name="enable_advanced settings"]' => array('checked' => FALSE)
        ),
      ),
  );
  
  $init_popularity = variable_get('apachesolr_popularity_init_popularity', 20);
  $init_popularity_options = array( 0, 10, 20, 50, 100 );
  $form['settings']['apachesolr_popularity_init_popularity'] = array(
      '#type' => 'select',
      '#title' => t('Initial node popularity'),
      '#default_value' => array_search($init_popularity, $init_popularity_options), //,
      '#options' => $init_popularity_options,
      '#description' => t("Sets inital popularity for newly tracked nodes.<br>Starting popularity: 0 = Lowest, 20 = Moderate, 100 = Highest."),
      '#states' => array(
        'disabled' => array(
          ':input[name="enable_advanced settings"]' => array('checked' => FALSE)
        ),
      ),
  );
    
  $compression = variable_get('apachesolr_popularity_compression', 'Sqrt');
  $compression_options = array( 'Linear', 'Sqrt', 'Log' );
  $form['settings']['apachesolr_popularity_compression'] = array(
      '#type' => 'select',
      '#title' => t('Popularity compression on high traffic nodes'),
      '#default_value' => array_search($compression, $compression_options), //,
      '#options' => $compression_options,
      '#description' => t("Prevent high traffic nodes from dominating the search results.<br>Compression type: Linear = None, Sqrt = Moderate, Log = High."),
      '#states' => array(
        'disabled' => array(
          ':input[name="enable_advanced settings"]' => array('checked' => FALSE)
        ),
      ),
  );
    
  $low_pop_influence = variable_get('apachesolr_popularity_low_pop_influence', 1);
  $low_pop_influence_options = array( 0.0, 0.2, 0.5, 1, 2);
  $form['settings']['apachesolr_popularity_low_pop_influence'] = array(
      '#type' => 'select',
      '#title' => t('Low popularity influence'),
      '#default_value' => array_search($low_pop_influence, $low_pop_influence_options), //,
      '#options' => $low_pop_influence_options,
      '#description' => t("Allows unpopular pages to still still be found in a search.<br> 0 = Hard to find unpopular page, 1 = Typical, 2 = Found easier."),
      '#states' => array(
        'disabled' => array(
          ':input[name="enable_advanced settings"]' => array('checked' => FALSE)
        ),
      ),
  );
  
  $form['#submit'][] = 'apachesolr_popularity_advanced_form_submit';
  
  return system_settings_form($form);
}


